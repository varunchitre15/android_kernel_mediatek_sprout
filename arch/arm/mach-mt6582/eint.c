/*
* Copyright (C) 2011-2014 MediaTek Inc.
* 
* This program is free software: you can redistribute it and/or modify it under the terms of the 
* GNU General Public License version 2 as published by the Free Software Foundation.
* 
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/wakelock.h>
#include <linux/module.h>
#include <linux/device.h>
//#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <asm/setup.h>
#include <asm/delay.h>
#include <mach/mt_reg_base.h>
#include <mach/eint.h>
#include <mach/eint_drv.h>
#include <mach/mt_gpio.h>
#include <mach/mt_gpio_core.h>
#include <mach/irqs.h>
#include <mach/sync_write.h>
#include <linux/irqdomain.h>
#include <linux/irq.h>
#include <asm/mach/irq.h>
#include <linux/sched.h>

#define EINT_DEBUG 0
#if(EINT_DEBUG == 1)
#define dbgmsg printk
#else
#define dbgmsg(...)
#endif

/* Check if NR_IRQS is enough */
#if (EINT_IRQ_BASE + EINT_MAX_CHANNEL) > (NR_IRQS)
#error NR_IRQS not enough.
#endif

//#define EINT_TEST_V2

#define MD_EINT
#define EINT_TEST
//#define EINT_TEST_SOFT
//#define DEINT_SUPPORT

#ifdef MD_EINT
#include <cust_eint.h>
#include <cust_eint_md1.h>
//#include <cust_eint_md2.h>

typedef enum
{
    SIM_HOT_PLUG_EINT_NUMBER,
    SIM_HOT_PLUG_EINT_DEBOUNCETIME,
    SIM_HOT_PLUG_EINT_POLARITY,
    SIM_HOT_PLUG_EINT_SENSITIVITY,
    SIM_HOT_PLUG_EINT_SOCKETTYPE,
}sim_hot_plug_eint_queryType;

typedef enum
{
    ERR_SIM_HOT_PLUG_NULL_POINTER=-13,
    ERR_SIM_HOT_PLUG_QUERY_TYPE,
    ERR_SIM_HOT_PLUG_QUERY_STRING,
}sim_hot_plug_eint_queryErr;
#endif


typedef struct 
{
    void (*eint_func[EINT_MAX_CHANNEL]) (void);
    unsigned int eint_auto_umask[EINT_MAX_CHANNEL];
    /*is_deb_en: 1 means enable, 0 means disable */
    unsigned int is_deb_en[EINT_MAX_CHANNEL];
    unsigned int deb_time[EINT_MAX_CHANNEL];
#if defined(EINT_TEST)
    unsigned int softisr_called[EINT_MAX_CHANNEL];
#endif
    struct timer_list eint_sw_deb_timer[EINT_MAX_CHANNEL];
    unsigned int count[EINT_MAX_CHANNEL];
} eint_func;

static eint_func EINT_FUNC;

#ifdef DEINT_SUPPORT
/*
typedef struct
{
    void (*deint_func[DEINT_MAX_CHANNEL]) (void);
}deint_func;

static deint_func DEINT_FUNC;
*/
#endif

static const unsigned int EINT_IRQ = MT_EINT_IRQ_ID;
struct wake_lock EINT_suspend_lock;
#if defined(EINT_TEST_SOFT)
static unsigned int cur_eint_num;
#endif
static DEFINE_SPINLOCK(eint_lock);

/*
 * mt_eint_get_mask: To get the eint mask
 * @eint_num: the EINT number to get
 */
static unsigned int mt_eint_get_mask(unsigned int eint_num)
{
    unsigned int base;
    unsigned int st;
    unsigned int bit = 1 << (eint_num % 32);

    if (eint_num < EINT_AP_MAXNUMBER) {
        base = (eint_num / 32) * 4 + EINT_MASK_BASE;
    } else {
        dbgmsg("Error in %s [EINT] num:%d is larger than EINT_AP_MAXNUMBER\n", __func__, eint_num);
        return 0;
    }

    st = readl(IOMEM(base));
    if (st & bit) {
        st = 1; //masked
    } else {
        st = 0; //unmasked
    }

    return st;
}

#if 0
void mt_eint_mask_all(void)
{
    unsigned int base;
    unsigned int val = 0xFFFFFFFF, ap_cnt = (EINT_MAX_CHANNEL / 32), i;
    if (EINT_MAX_CHANNEL % 32)
        ap_cnt++;
    dbgmsg("[EINT] cnt:%d\n", ap_cnt);

    base = EINT_MASK_SET_BASE;
    for (i = 0; i < ap_cnt; i++) {
        writel(val, IOMEM(base + (i * 4)));
        dbgmsg("[EINT] mask addr:%x = %x\n", EINT_MASK_BASE + (i * 4),
               readl(IOMEM(EINT_MASK_BASE + (i * 4))));
    }
}

/*
 * mt_eint_unmask_all: Mask the specified EINT number.
 */
void mt_eint_unmask_all(void)
{
    unsigned int base;
    unsigned int val = 0xFFFFFFFF, ap_cnt = (EINT_MAX_CHANNEL / 32), i;
    if (EINT_MAX_CHANNEL % 32)
        ap_cnt++;
    dbgmsg("[EINT] cnt:%d\n", ap_cnt);

    base = EINT_MASK_CLR_BASE;
    for (i = 0; i < ap_cnt; i++) {
        writel(val, IOMEM(base + (i * 4)));
        dbgmsg("[EINT] unmask addr:%x = %x\n", EINT_MASK_BASE + (i * 4),
               readl(IOMEM(EINT_MASK_BASE + (i * 4))));
    }
}

/*
 * mt_eint_get_soft: To get the eint mask
 * @eint_num: the EINT number to get
 */
unsigned int mt_eint_get_soft(unsigned int eint_num)
{
    unsigned int base;
    unsigned int st;

    if (eint_num < EINT_AP_MAXNUMBER) {
        base = (eint_num / 32) * 4 + EINT_SOFT_BASE;
    } else {
            dbgmsg("Error in %s [EINT] num:%d is larger than EINT_AP_MAXNUMBER\n", __func__, eint_num);
        return 0;
    }
    st = readl(IOMEM(base));

    return st;
}
#endif

#if 0
/*
 * mt_eint_emu_set: Trigger the specified EINT number.
 * @eint_num: EINT number to set
 */
void mt_eint_emu_set(unsigned int eint_num)
{
        unsigned int base = 0;
        unsigned int bit = 1 << (eint_num % 32);
    unsigned int value = 0;

        if (eint_num < EINT_AP_MAXNUMBER) {
                base = (eint_num / 32) * 4 + EINT_EMUL_BASE;
        } else {
            dbgmsg("Error in %s [EINT] num:%d is larger than EINT_AP_MAXNUMBER\n", __func__, eint_num);
        return;
        }
    value = readl(IOMEM(base));
    value = bit | value;
        writel(value, IOMEM(base));
    value = readl(IOMEM(base));

        dbgmsg("[EINT] emul set addr:%x = %x, bit=%x\n", base, value, bit);


}

/*
 * mt_eint_emu_clr: Trigger the specified EINT number.
 * @eint_num: EINT number to clr
 */
void mt_eint_emu_clr(unsigned int eint_num)
{
        unsigned int base = 0;
        unsigned int bit = 1 << (eint_num % 32);
    unsigned int value = 0;

        if (eint_num < EINT_AP_MAXNUMBER) {
                base = (eint_num / 32) * 4 + EINT_EMUL_BASE;
        } else {
            dbgmsg("Error in %s [EINT] num:%d is larger than EINT_AP_MAXNUMBER\n", __func__, eint_num);
        return;
        }
    value = readl(IOMEM(base));
    value = (~bit) & value;
        writel(value, IOMEM(base));
    value = readl(IOMEM(base));

        dbgmsg("[EINT] emul clr addr:%x = %x, bit=%x\n", base, value, bit);

}

/*
 * eint_send_pulse: Trigger the specified EINT number.
 * @eint_num: EINT number to send
 */
inline void mt_eint_send_pulse(unsigned int eint_num)
{
    unsigned int base_set = (eint_num / 32) * 4 + EINT_SOFT_SET_BASE;
    unsigned int base_clr = (eint_num / 32) * 4 + EINT_SOFT_CLR_BASE;
    unsigned int bit = 1 << (eint_num % 32);
    if (eint_num < EINT_AP_MAXNUMBER) {
        base_set = (eint_num / 32) * 4 + EINT_SOFT_SET_BASE;
        base_clr = (eint_num / 32) * 4 + EINT_SOFT_CLR_BASE;
    } else {
            dbgmsg("Error in %s [EINT] num:%d is larger than EINT_AP_MAXNUMBER\n", __func__, eint_num);
        return;
    }

    writel(bit, IOMEM(base_set));
    writel(bit, IOMEM(base_clr));
}
#endif

#if defined(EINT_TEST)
/*
 * mt_eint_soft_set: Trigger the specified EINT number.
 * @eint_num: EINT number to set
 */
void mt_eint_soft_set(unsigned int eint_num)
{

    unsigned int base;
    unsigned int bit = 1 << (eint_num % 32);

    if (eint_num < EINT_AP_MAXNUMBER) {
        base = (eint_num / 32) * 4 + EINT_SOFT_SET_BASE;
    } else {
            dbgmsg("Error in %s [EINT] num:%d is larger than EINT_AP_MAXNUMBER\n", __func__, eint_num);
        return;
    }
    writel(bit, IOMEM(base));

    dbgmsg("[EINT] soft set addr:%x = %x\n", base, bit);

}

/*
 * mt_eint_soft_clr: Unmask the specified EINT number.
 * @eint_num: EINT number to clear
 */
static void mt_eint_soft_clr(unsigned int eint_num)
{
    unsigned int base;
    unsigned int bit = 1 << (eint_num % 32);

    if (eint_num < EINT_AP_MAXNUMBER) {
        base = (eint_num / 32) * 4 + EINT_SOFT_CLR_BASE;
    } else {
            dbgmsg("Error in %s [EINT] num:%d is larger than EINT_AP_MAXNUMBER\n", __func__, eint_num);
        return;
    }
    writel(bit, IOMEM(base));

    dbgmsg("[EINT] soft clr addr:%x = %x\n", base, bit);

}
#endif

/*
 * mt_eint_mask: Mask the specified EINT number.
 * @eint_num: EINT number to mask
 */
void mt_eint_mask(unsigned int eint_num)
{
    unsigned int base;
    unsigned int bit = 1 << (eint_num % 32);

    if (eint_num < EINT_AP_MAXNUMBER) {
        base = (eint_num / 32) * 4 + EINT_MASK_SET_BASE;
    } else {
        dbgmsg("Error in %s [EINT] num:%d is larger than EINT_AP_MAXNUMBER\n", __func__, eint_num);
        return;
    }
    mt65xx_reg_sync_writel(bit, base);

    dbgmsg("[EINT] mask addr:%x = %x\n", base, bit);
}

/*
 * mt_eint_unmask: Unmask the specified EINT number.
 * @eint_num: EINT number to unmask
 */
void mt_eint_unmask(unsigned int eint_num)
{
    unsigned int base;
    unsigned int bit = 1 << (eint_num % 32);

    if (eint_num < EINT_AP_MAXNUMBER) {
        base = (eint_num / 32) * 4 + EINT_MASK_CLR_BASE;
    } else {
        dbgmsg("Error in %s [EINT] num:%d is larger than EINT_AP_MAXNUMBER\n", __func__, eint_num);
        return;
    }
    mt65xx_reg_sync_writel(bit, base);

    dbgmsg("[EINT] unmask addr:%x = %x\n", base, bit);
}

/*
 * mt_eint_set_polarity: Set the polarity for the EINT number.
 * @eint_num: EINT number to set
 * @pol: polarity to set
 */
void mt_eint_set_polarity(unsigned int eint_num, unsigned int pol)
{
    unsigned int count;
    unsigned int base;
    unsigned int bit = 1 << (eint_num % 32);

    if (pol == MT_EINT_POL_NEG) {
        if (eint_num < EINT_AP_MAXNUMBER) {
            base = (eint_num / 32) * 4 + EINT_POL_CLR_BASE;
        } else {
            dbgmsg("Error in %s [EINT] num:%d is larger than EINT_AP_MAXNUMBER\n", __func__, eint_num);
            return;
        }
    } else {
        if (eint_num < EINT_AP_MAXNUMBER) {
            base = (eint_num / 32) * 4 + EINT_POL_SET_BASE;
        } else {
            dbgmsg("Error in %s [EINT] num:%d is larger than EINT_AP_MAXNUMBER\n", __func__, eint_num);
            return;
        }
    }
    mt65xx_reg_sync_writel(bit, base);
    dbgmsg("[EINT] %s :%x, bit: %x\n", __func__, base, bit);

    for (count = 0; count < 250; count++) ;

    if (eint_num < EINT_AP_MAXNUMBER) {
        base = (eint_num / 32) * 4 + EINT_INTACK_BASE;
    } else {
        dbgmsg("Error in %s [EINT] num:%d is larger than EINT_AP_MAXNUMBER\n", __func__, eint_num);
        return;
    }
    mt65xx_reg_sync_writel(bit, base);
}

/*
 * mt_eint_get_polarity: Set the polarity for the EINT number.
 * @eint_num: EINT number to get
 * Return: polarity type
 */
unsigned int mt_eint_get_polarity(unsigned int eint_num)
{
    unsigned int val;
    unsigned int base;
    unsigned int bit = 1 << (eint_num % 32);
    unsigned int pol;

    if (eint_num < EINT_AP_MAXNUMBER) {
        base = (eint_num / 32) * 4 + EINT_POL_BASE;
    } else {
        dbgmsg("Error in %s [EINT] num:%d is larger than EINT_AP_MAXNUMBER\n", __func__, eint_num);
        return 0;
    }
    val = readl(IOMEM(base));

    dbgmsg("[EINT] %s :%x, bit:%x, val:%x\n", __func__, base, bit, val);
    if (val & bit) {
        pol = MT_EINT_POL_POS;
    } else {
        pol = MT_EINT_POL_NEG;
    }
    return pol;  
}

/*
 * mt_eint_set_sens: Set the sensitivity for the EINT number.
 * @eint_num: EINT number to set
 * @sens: sensitivity to set
 * Always return 0.
 */
unsigned int mt_eint_set_sens(unsigned int eint_num, unsigned int sens)
{
    unsigned int base;
    unsigned int bit = 1 << (eint_num % 32);

    if (sens == MT_EDGE_SENSITIVE) {
        if (eint_num < EINT_AP_MAXNUMBER) {
            base = (eint_num / 32) * 4 + EINT_SENS_CLR_BASE;
        } else {
            dbgmsg("Error in %s [EINT] num:%d is larger than EINT_AP_MAXNUMBER\n", __func__, eint_num);
            return 0;
        }
    } else if (sens == MT_LEVEL_SENSITIVE) {
        if (eint_num < EINT_AP_MAXNUMBER) {
            base = (eint_num / 32) * 4 + EINT_SENS_SET_BASE;
        } else {
            dbgmsg("Error in %s [EINT] num:%d is larger than EINT_AP_MAXNUMBER\n", __func__, eint_num);
            return 0;
        }
    } else {
        printk("%s invalid sensitivity value\n", __func__);
        return 0;
    }
    mt65xx_reg_sync_writel(bit, base);
    dbgmsg("[EINT] %s :%x, bit: %x\n", __func__, base, bit);
    return 0;
}

/*
 * mt_eint_get_sens: To get the eint sens
 * @eint_num: the EINT number to get
 */
static unsigned int mt_eint_get_sens(unsigned int eint_num)
{
    unsigned int base, sens;
    unsigned int bit = 1 << (eint_num % 32), st;

    if (eint_num < EINT_AP_MAXNUMBER) {
        base = (eint_num / 32) * 4 + EINT_SENS_BASE;
    } else {
        dbgmsg("Error in %s [EINT] num:%d is larger than EINT_AP_MAXNUMBER\n", __func__, eint_num);
        return 0;
    }
    st = readl(IOMEM(base));
    if (st & bit) {
        sens = MT_LEVEL_SENSITIVE;
    } else {
        sens = MT_EDGE_SENSITIVE;
    }
    return sens;
}

/*
 * mt_eint_ack: To ack the interrupt 
 * @eint_num: the EINT number to set
 */
unsigned int mt_eint_ack(unsigned int eint_num)
{
    unsigned int base;
    unsigned int bit = 1 << (eint_num % 32);

    if (eint_num < EINT_AP_MAXNUMBER) {
        base = (eint_num / 32) * 4 + EINT_INTACK_BASE;
    } else {
        dbgmsg("Error in %s [EINT] num:%d is larger than EINT_AP_MAXNUMBER\n", __func__, eint_num);
        return 0;
    }
    mt65xx_reg_sync_writel(bit, base);

    dbgmsg("[EINT] %s :%x, bit: %x\n", __func__, base, bit);
    return 0;
}

/*
 * mt_eint_read_status: To read the interrupt status
 * @eint_num: the EINT number to set
 */
static unsigned int mt_eint_read_status(unsigned int eint_num)
{
    unsigned int base;
    unsigned int st;
    unsigned int bit = 1 << (eint_num % 32);

    if (eint_num < EINT_AP_MAXNUMBER) {
        base = (eint_num / 32) * 4 + EINT_STA_BASE;
    } else {
        dbgmsg("Error in %s [EINT] num:%d is larger than EINT_AP_MAXNUMBER\n", __func__, eint_num);
        return 0;
    }
    st = readl(IOMEM(base));

    return (st & bit);
}

/*
 * mt_eint_get_status: To get the interrupt status 
 * @eint_num: the EINT number to get
 */
static unsigned int mt_eint_get_status(unsigned int eint_num)
{
    unsigned int base;
    unsigned int st;

    if (eint_num < EINT_AP_MAXNUMBER) {
        base = (eint_num / 32) * 4 + EINT_STA_BASE;
    } else {
        dbgmsg("Error in %s [EINT] num:%d is larger than EINT_AP_MAXNUMBER\n", __func__, eint_num);
        return 0;
    }

    st = readl(IOMEM(base));
    return st;
}

/*
 * mt_eint_en_hw_debounce: To enable hw debounce
 * @eint_num: the EINT number to set
 */
static void mt_eint_en_hw_debounce(unsigned int eint_num)
{
    unsigned int base, bit;
    base = (eint_num / 4) * 4 + EINT_DBNC_SET_BASE;
    bit = (EINT_DBNC_SET_EN << EINT_DBNC_SET_EN_BITS) << ((eint_num % 4) * 8);
    writel(bit, IOMEM(base)); 
    EINT_FUNC.is_deb_en[eint_num] = 1;
}

/*
 * mt_eint_dis_hw_debounce: To disable hw debounce
 * @eint_num: the EINT number to set
 */
static void mt_eint_dis_hw_debounce(unsigned int eint_num)
{
    unsigned int clr_base, bit;
    clr_base = (eint_num / 4) * 4 + EINT_DBNC_CLR_BASE;
    bit = (EINT_DBNC_CLR_EN << EINT_DBNC_CLR_EN_BITS) << ((eint_num % 4) * 8);
    writel(bit, IOMEM(clr_base));
    EINT_FUNC.is_deb_en[eint_num] = 0;
}

/*
 * mt_eint_dis_sw_debounce: To set EINT_FUNC.is_deb_en[eint_num] disable
 * @eint_num: the EINT number to set
 */
static void mt_eint_dis_sw_debounce(unsigned int eint_num)
{
    if(eint_num < EINT_MAX_CHANNEL)
        EINT_FUNC.is_deb_en[eint_num] = 0;
}

/*
 * mt_eint_en_sw_debounce: To set EINT_FUNC.is_deb_en[eint_num] enable
 * @eint_num: the EINT number to set
 */
static void mt_eint_en_sw_debounce(unsigned int eint_num)
{
    if(eint_num < EINT_MAX_CHANNEL)    
        EINT_FUNC.is_deb_en[eint_num] = 1;
}

/*
 * mt_can_en_debounce: Check the EINT number is able to enable debounce or not
 * @eint_num: the EINT number to set
 */
static unsigned int mt_can_en_debounce(unsigned int eint_num)
{
    unsigned int sens = mt_eint_get_sens(eint_num);
    /* debounce: debounce time is not 0 && it is not edge sensitive */
    if (EINT_FUNC.deb_time[eint_num] != 0 && sens != MT_EDGE_SENSITIVE)
        return 1;
    else {
        dbgmsg
            ("Can't enable debounce of eint_num:%d, deb_time:%d, sens:%d\n",
             eint_num, EINT_FUNC.deb_time[eint_num], sens);
        return 0;
    }
}

/*
 * mt_eint_set_hw_debounce: Set the de-bounce time for the specified EINT number.
 * @eint_num: EINT number to acknowledge
 * @ms: the de-bounce time to set (in miliseconds)
 */
void mt_eint_set_hw_debounce(unsigned int eint_num, unsigned int ms)
{
    unsigned int dbnc, base, bit, clr_bit, clr_base, rst, unmask = 0;

    base = (eint_num / 4) * 4 + EINT_DBNC_SET_BASE;
    clr_base = (eint_num / 4) * 4 + EINT_DBNC_CLR_BASE;
    EINT_FUNC.deb_time[eint_num] = ms;

    /* 
     * Don't enable debounce once debounce time is 0 or 
     * its type is edge sensitive.
     */
    if (!mt_can_en_debounce(eint_num)) {
        dbgmsg("Can't enable debounce of eint_num:%d in %s\n", eint_num,
               __func__);
        return;
    }

    if (ms == 0) {
        dbnc = 0;
        dbgmsg("ms should not be 0. eint_num:%d in %s\n", eint_num,
               __func__);
    } else if (ms <= 1) {
        dbnc = 1;
    } else if (ms <= 16) {
        dbnc = 2;
    } else if (ms <= 32) {
        dbnc = 3;
    } else if (ms <= 64) {
        dbnc = 4;
    } else if (ms <= 128) {
        dbnc = 5;
    } else if (ms <= 256) {
        dbnc = 6;
    } else {
        dbnc = 7;
    }

    /* setp 1: mask the EINT */
    if(!mt_eint_get_mask(eint_num)) {
        mt_eint_mask(eint_num);
        unmask = 1;
    }
    /* step 2: Check hw debouce number to decide which type should be used */
    if (eint_num >= MAX_HW_DEBOUNCE_CNT)
        mt_eint_en_sw_debounce(eint_num);
    else {
        /* step 2.1: set hw debounce flag*/
        EINT_FUNC.is_deb_en[eint_num] = 1;
        
        /* step 2.2: disable hw debounce */
        clr_bit = 0xFF << ((eint_num % 4) * 8);
        mt65xx_reg_sync_writel(clr_bit, clr_base);
        
        /* step 2.3: set new debounce value */
        bit =
            ((dbnc << EINT_DBNC_SET_DBNC_BITS) |
             (EINT_DBNC_SET_EN << EINT_DBNC_SET_EN_BITS)) << 
                    ((eint_num % 4) * 8);
        mt65xx_reg_sync_writel(bit, base);

        /* step 2.4: Delay a while (more than 2T) to wait for hw debounce enable work correctly */
        udelay(500);
        
        /* step 2.5: Reset hw debounce counter to avoid unexpected interrupt */
        rst = (EINT_DBNC_RST_BIT << EINT_DBNC_SET_RST_BITS) <<
          ((eint_num % 4) * 8);
        mt65xx_reg_sync_writel(rst, base);

        /* step 2.6: Delay a while (more than 2T) to wait for hw debounce counter reset work correctly */
        udelay(500);   
    }
    /* step 3: unmask the EINT */
    if(unmask == 1)
        mt_eint_unmask(eint_num);
}

/*
 * eint_do_tasklet: EINT tasklet function.
 * @unused: not use.
 */
static void eint_do_tasklet(unsigned long unused)
{
    wake_lock_timeout(&EINT_suspend_lock, HZ / 2);
}

DECLARE_TASKLET(eint_tasklet, eint_do_tasklet, 0);

/*
 * mt_eint_timer_event_handler: EINT sw debounce handler
 * @eint_num: the EINT number and use unsigned long to prevent 
 *            compile warning of timer usage.
 */
static void mt_eint_timer_event_handler(unsigned long eint_num)
{
    unsigned int status;
    unsigned long flags;

    if(eint_num >= EINT_MAX_CHANNEL)
        return;

    /* disable interrupt for core 0 and it will run on core 0 only */
    local_irq_save(flags);
    mt_eint_unmask(eint_num);
    status = mt_eint_read_status(eint_num);
    dbgmsg("EINT Module - EINT_STA = 0x%x, in %s\n", status, __func__);
    if (!EINT_FUNC.eint_func[eint_num])
    {
        dbgmsg("EINT Module_IRQ - EINT_STA = 0x%x, in %s\n", status, __func__);
        generic_handle_irq(EINT_IRQ(eint_num));
        local_irq_restore(flags);
    }   
    else
    {
        if (status) {
            mt_eint_mask(eint_num);
            if (EINT_FUNC.eint_func[eint_num])
                EINT_FUNC.eint_func[eint_num] ();
            mt_eint_ack(eint_num);
        }
        local_irq_restore(flags);
        if (EINT_FUNC.eint_auto_umask[eint_num])
            mt_eint_unmask(eint_num);
    }

    return;
}

/*
 * mt_eint_set_timer_event: To set a timer event for sw debounce.
 * @eint_num: the EINT number to set
 */
static void mt_eint_set_timer_event(unsigned int eint_num)
{
    struct timer_list *eint_timer = &EINT_FUNC.eint_sw_deb_timer[eint_num];
    /* assign this handler to execute on core 0 */
    int cpu = 0;

    /* register timer for this sw debounce eint */
    eint_timer->expires =
        jiffies + msecs_to_jiffies(EINT_FUNC.deb_time[eint_num]);
    dbgmsg("EINT Module - expires:%llu, jiffies:%llu, deb_in_jiffies:%llu, ", eint_timer->expires, jiffies, msecs_to_jiffies(EINT_FUNC.deb_time[eint_num]));
    dbgmsg("deb:%d, in %s\n", EINT_FUNC.deb_time[eint_num], __func__);
    eint_timer->data = eint_num;
    eint_timer->function = &mt_eint_timer_event_handler;
    init_timer(eint_timer);
    add_timer_on(eint_timer, cpu);
}

/*
 * mt_eint_isr: EINT interrupt service routine.
 * @irq: EINT IRQ number
 * @desc: EINT IRQ descriptor
 * Return IRQ returned code.
 */
static irqreturn_t mt_eint_demux(unsigned irq, struct irq_desc *desc)
{
    unsigned int index, rst, base;
    unsigned int status = 0;
    unsigned int status_check;
    unsigned int reg_base,offset;
    unsigned long long t1, t2;
    int mask_status=0;
    struct irq_chip *chip = irq_get_chip(irq);
    chained_irq_enter(chip, desc);
        
     /* 
     * NoteXXX: Need to get the wake up for 0.5 seconds when an EINT intr tirggers.
     *          This is used to prevent system from suspend such that other drivers
     *          or applications can have enough time to obtain their own wake lock.
     *          (This information is gotten from the power management owner.)
     */

    tasklet_schedule(&eint_tasklet);
    dbgmsg("EINT Module - %s ISR Start\n", __func__);
    //printk("EINT acitve: %s, acitve status: %d\n", mt_irq_is_active(EINT_IRQ) ? "yes" : "no", mt_irq_is_active(EINT_IRQ));

    for (reg_base = 0; reg_base < EINT_MAX_CHANNEL; reg_base+=32) {
            /* read status register every 32 interrupts */
            status = mt_eint_get_status(reg_base);
            if(status){
                dbgmsg("EINT Module - index:%d,EINT_STA = 0x%x\n",
                  reg_base, status);
            }
            else{
                continue;
            }
        for(offset = 0; offset < 32; offset++){
            index = reg_base + offset;
            if (index >= EINT_MAX_CHANNEL) break;

            status_check = status & (1 << (index % 32));
            if (status_check) {
                dbgmsg("Got eint:%d\n",index);
                mt_eint_mask(index);

                EINT_FUNC.count[index]++;

                // deal with EINT from request_irq()
                if (!EINT_FUNC.eint_func[index])
                {
                    dbgmsg("EINT %d: go with new mt_eint\n", index);
                    if ((EINT_FUNC.is_deb_en[index] == 1) &&
                            (index >= MAX_HW_DEBOUNCE_CNT)) {
                        /* if its debounce is enable and it is a sw debounce */
                        mt_eint_set_timer_event(index);
                    } else {
                        t1 = sched_clock();
                        generic_handle_irq(index + EINT_IRQ_BASE);
                        t2 = sched_clock();
                        if(mt_eint_get_mask(index)==1)
                        {
                            mask_status=1;
                        }
                        mt_eint_mask(index); 

                        /* Don't need to use reset ? */
                        /* reset debounce counter */
                        base = (index / 4) * 4 + EINT_DBNC_SET_BASE;
                        rst = (EINT_DBNC_RST_BIT << EINT_DBNC_SET_RST_BITS) <<
                            ((index % 4) * 8);
                        mt65xx_reg_sync_writel(rst, base);

#if(EINT_DEBUG == 1)
                        status = mt_eint_get_status(index);
                        dbgmsg("EINT Module - EINT_STA after ack = 0x%x\n", status);
#endif

                        if ((t2-t1) > 1000000)
                            printk("[EINT]Warn!EINT:%d run too long,s:%llu,e:%llu,total:%llu\n", index, t1, t2, (t2 - t1));
                        if(mask_status==0)
                            mt_eint_unmask(index);
                    }
                }
                // deal with EINT from mt_eint_registration()
                else
                {
                    dbgmsg("EINT %d: go with original mt_eint\n", index);
                    if ((EINT_FUNC.is_deb_en[index] == 1) &&
                            (index >= MAX_HW_DEBOUNCE_CNT)) {
                        /* if its debounce is enable and it is a sw debounce */
                        mt_eint_set_timer_event(index);
                    } else {
                        /* HW debounce or no use debounce */
                        t1 = sched_clock();
                        if (EINT_FUNC.eint_func[index]) {
                            EINT_FUNC.eint_func[index] ();
                        }
                        t2 = sched_clock();
                        mt_eint_ack(index);
#if 1                   /* Don't need to use reset ? */
                        /* reset debounce counter */
                        base = (index / 4) * 4 + EINT_DBNC_SET_BASE;
                        rst =
                            (EINT_DBNC_RST_BIT << EINT_DBNC_SET_RST_BITS) <<
                            ((index % 4) * 8);
                        mt65xx_reg_sync_writel(rst, base);
#endif
#if(EINT_DEBUG == 1)
                        status = mt_eint_get_status(index);
                        dbgmsg("EINT Module - EINT_STA after ack = 0x%x\n", status);
#endif
                        if (EINT_FUNC.eint_auto_umask[index]) {
                            mt_eint_unmask(index);
                        }

                        if ((t2-t1) > 1000000)
                            printk("[EINT]Warn!EINT:%d run too long,s:%llu,e:%llu,total:%llu\n", index, t1, t2, (t2 - t1));
                    }
                }
            }
        }
    }

    dbgmsg("EINT Module - %s ISR END\n", __func__);
    chained_irq_exit(chip, desc);
    return IRQ_HANDLED;
}

static int mt_eint_max_channel(void)
{
    return EINT_MAX_CHANNEL;
}

/*
 * mt_eint_dis_debounce: To disable debounce.
 * @eint_num: the EINT number to disable
 */
static void mt_eint_dis_debounce(unsigned int eint_num)
{
    /* This function is used to disable debounce whether hw or sw */
    if (eint_num < MAX_HW_DEBOUNCE_CNT)
        mt_eint_dis_hw_debounce(eint_num);
    else
        mt_eint_dis_sw_debounce(eint_num);
}

/*
 * mt_eint_registration: register a EINT.
 * @eint_num: the EINT number to register
 * @flag: the interrupt line behaviour to select
 * @EINT_FUNC_PTR: the ISR callback function
 * @is_auto_unmask: the indication flag of auto unmasking after ISR callback is processed
 */
void mt_eint_registration(unsigned int eint_num, unsigned int flag, 
              void (EINT_FUNC_PTR) (void), unsigned int is_auto_umask)
{
    if (eint_num < EINT_MAX_CHANNEL) {
        printk("eint register for %d\n", eint_num);
        spin_lock(&eint_lock);
        mt_eint_mask(eint_num);
        
        if (flag & (EINTF_TRIGGER_RISING | EINTF_TRIGGER_FALLING)) {
            mt_eint_set_polarity(eint_num, (flag & EINTF_TRIGGER_FALLING) ? MT_EINT_POL_NEG : MT_EINT_POL_POS);
            mt_eint_set_sens(eint_num, MT_EDGE_SENSITIVE);
        } else if (flag & (EINTF_TRIGGER_HIGH | EINTF_TRIGGER_LOW)) {
            mt_eint_set_polarity(eint_num, (flag & EINTF_TRIGGER_LOW) ? MT_EINT_POL_NEG : MT_EINT_POL_POS);
            mt_eint_set_sens(eint_num, MT_LEVEL_SENSITIVE);    
        } else {
            printk("[EINT]: Wrong EINT Pol/Sens Setting 0x%x\n", flag);
            spin_unlock(&eint_lock);
            return ;
        }
        
        EINT_FUNC.eint_func[eint_num] = EINT_FUNC_PTR;
        spin_unlock(&eint_lock);
        EINT_FUNC.eint_auto_umask[eint_num] = is_auto_umask;
        mt_eint_ack(eint_num);
        mt_eint_unmask(eint_num);
    } else {
        printk("[EINT]: Wrong EINT Number %d\n", eint_num);
    }
}


#ifdef DEINT_SUPPORT
/*
static irqreturn_t  mt_deint_isr(int irq, void *dev_id)
{
    int deint_num = irq - MT_EINT_DIRECT0_IRQ_ID;
    printk("IRQ = %d\n", irq);
    if (deint_num < 0) {
        printk(KERN_ERR "DEINT IRQ Number %d IS NOT VALID!! \n", deint_num);
    }
    
    if (DEINT_FUNC.deint_func[deint_num]) { 
        DEINT_FUNC.deint_func[deint_num]();
    }else
       printk("NULL EINT POINTER\n");
    
    
    printk("EXIT DEINT ISR\n");
    return IRQ_HANDLED;
}
*/

/*
 * mt_deint_registration: register a DEINT.
 * @eint_num: the DEINT number to register
 * @flag: the interrupt line behaviour to select
 * @DEINT_FUNC_PTR: the ISR callback function
 * THIS FUNCTION IS INTERNAL USE ONLY, DON'T EXPORT
 * 
 * NOTE: this function is not exported to our customer for now
 *       but if we want to export it in the future, we must let them know that
 *       DEINT should register its ISR with prototype same as ordinal ISR
 *       unlike EINT's void (*func) (void)
 */

static void mt_deint_registration(unsigned int deint_num, unsigned int flag,
              //void (DEINT_FUNC_PTR) (void))
              irqreturn_t (DEINT_FUNC_PTR) (int, void *))
{
    int num = deint_num;
    if (deint_num < DEINT_MAX_CHANNEL) {

        // deint and eint numbers are not linear mapping
        if(deint_num > 11)
            num = deint_num + 2;
        /*
        if(!mt_eint_get_mask(num)) {
            printk("[Wrong DEint-%d] has been registered as EINT pin.!!! \n", deint_num);
            return ;
        }
        */

        spin_lock(&eint_lock);
        if(EINT_FUNC.eint_func[num] != NULL){
            printk("[Wrong DEint-%d] has been registered as EINT pin.!!! \n", deint_num);
            spin_unlock(&eint_lock);
            return;
        }
        spin_unlock(&eint_lock);

        mt_eint_mask(num);

        if (flag & (EINTF_TRIGGER_RISING | EINTF_TRIGGER_FALLING)) {
            mt_eint_set_polarity(num, (flag & EINTF_TRIGGER_FALLING) ? MT_EINT_POL_NEG : MT_EINT_POL_POS);
            mt_eint_set_sens(num, MT_EDGE_SENSITIVE);
        } else if (flag & (EINTF_TRIGGER_HIGH | EINTF_TRIGGER_LOW)) {
            mt_eint_set_polarity(num, (flag & EINTF_TRIGGER_LOW) ? MT_EINT_POL_NEG : MT_EINT_POL_POS);
            mt_eint_set_sens(num, MT_LEVEL_SENSITIVE);
        } else {
            printk("[DEINT]: Wrong DEINT Pol/Sens Setting 0x%x\n", flag);
            return ;
        }

        //DEINT_FUNC.deint_func[deint_num] = DEINT_FUNC_PTR;

        /* direct EINT IRQs registration */
        //if (request_irq(MT_EINT_DIRECT0_IRQ_ID + deint_num, mt_deint_isr, IRQF_TRIGGER_HIGH, "DEINT", NULL )) {
        if (request_irq(MT_EINT_DIRECT0_IRQ_ID + deint_num, DEINT_FUNC_PTR, IRQF_TRIGGER_HIGH, "DEINT", NULL )) {
            printk(KERN_ERR "DEINT IRQ LINE NOT AVAILABLE!!\n");
        }
          
    } else {
        printk("[DEINT]: Wrong DEINT Number %d\n", deint_num);
    }
}
#endif

static unsigned int mt_eint_get_debounce_cnt(unsigned int cur_eint_num)
{
    unsigned int dbnc, deb, base;
    base = (cur_eint_num / 4) * 4 + EINT_DBNC_BASE;

    if(cur_eint_num >= EINT_MAX_CHANNEL)
        return 0;

    if (cur_eint_num >= MAX_HW_DEBOUNCE_CNT)
        deb = EINT_FUNC.deb_time[cur_eint_num];
    else {
        dbnc = readl(IOMEM(base));
        dbnc = ((dbnc >> EINT_DBNC_SET_DBNC_BITS) >> ((cur_eint_num % 4) * 8) & EINT_DBNC);

        switch (dbnc) {
        case 0:
            deb = 0;    /* 0.5 actually, but we don't allow user to set. */
            dbgmsg(KERN_CRIT"ms should not be 0. eint_num:%d in %s\n",
                   cur_eint_num, __func__);
            break;
        case 1:
            deb = 1;
            break;
        case 2:
            deb = 16;
            break;
        case 3:
            deb = 32;
            break;
        case 4:
            deb = 64;
            break;
        case 5:
            deb = 128;
            break;
        case 6:
            deb = 256;
            break;
        case 7:
            deb = 512;
            break;
        default:
            deb = 0;
            printk("invalid deb time in the EIN_CON register, dbnc:%d, deb:%d\n", dbnc, deb);
            break;
        }
    }

    return deb;
}

static int mt_eint_is_debounce_en(unsigned int cur_eint_num)
{
    unsigned int base, val, en;
    if (cur_eint_num < MAX_HW_DEBOUNCE_CNT) {
        base = (cur_eint_num / 4) * 4 + EINT_DBNC_BASE;
        val = readl(IOMEM(base));
        val = val >> ((cur_eint_num % 4) * 8);
        if (val & EINT_DBNC_EN_BIT) {
            en = 1;
        } else {
            en = 0;
        }
    } else {
        en = EINT_FUNC.is_deb_en[cur_eint_num];
    }

    return en;
}

static void mt_eint_enable_debounce(unsigned int cur_eint_num)
{
    mt_eint_mask(cur_eint_num);
    if (cur_eint_num < MAX_HW_DEBOUNCE_CNT) {
        /* HW debounce */
        mt_eint_en_hw_debounce(cur_eint_num);
    } else {
        /* SW debounce */
        mt_eint_en_sw_debounce(cur_eint_num);
    }
    mt_eint_unmask(cur_eint_num);
}

static void mt_eint_disable_debounce(unsigned int cur_eint_num)
{
    mt_eint_mask(cur_eint_num);
    if (cur_eint_num < MAX_HW_DEBOUNCE_CNT) {
        /* HW debounce */
        mt_eint_dis_hw_debounce(cur_eint_num);
    } else {
        /* SW debounce */
        mt_eint_dis_sw_debounce(cur_eint_num);
    }
    mt_eint_unmask(cur_eint_num);
}

#if defined(EINT_TEST_SOFT)
static ssize_t cur_eint_soft_set_show(struct device_driver *driver, char *buf)
{
    unsigned int ret = EINT_FUNC.softisr_called[cur_eint_num];
    /* reset to 0 for the next testing. */
    EINT_FUNC.softisr_called[cur_eint_num] = 0;
    return snprintf(buf, PAGE_SIZE, "%d\n", ret);
}
/*
 * set 1 to trigger and set 0 to clr this interrupt
 */
static ssize_t cur_eint_soft_set_store(struct device_driver *driver, const char *buf, size_t count)
{
    char *p = (char *)buf;
    unsigned int num;

    num = simple_strtoul(p, &p, 10);
    if (num == 1) {
        mt_eint_soft_set(cur_eint_num);
    } else if (num == 0) {
        mt_eint_soft_clr(cur_eint_num);
    } else {
        printk("invalid number:%d it should be 1 to trigger interrupt or 0 to clr.\n", num);
    }

    return count;
}
DRIVER_ATTR(current_eint_soft_set, 0664, cur_eint_soft_set_show, cur_eint_soft_set_store);
static void mt_eint_soft_isr(void)
{
    EINT_FUNC.softisr_called[cur_eint_num] = 1;
    dbgmsg("in mt_eint_soft_isr\n");
    mt_eint_soft_clr(cur_eint_num);
    return;
}
static ssize_t cur_eint_reg_isr_show(struct device_driver *driver, char *buf)
{
    /* if ISR has been registered return 1
     * else return 0
     */
    unsigned int sens, pol, deb, autounmask, base, dbnc;
    base = (cur_eint_num / 4) * 4 + EINT_DBNC_BASE;
    sens = mt_eint_get_sens(cur_eint_num);
    pol = mt_eint_get_polarity(cur_eint_num);
    autounmask = EINT_FUNC.eint_auto_umask[cur_eint_num];

    if (cur_eint_num >= MAX_HW_DEBOUNCE_CNT)
        deb = EINT_FUNC.deb_time[cur_eint_num];
    else {
        dbnc = readl(IOMEM(base));
        dbnc = ((dbnc >> EINT_DBNC_SET_DBNC_BITS) >> ((cur_eint_num % 4) * 8) & EINT_DBNC);

        switch (dbnc) {
        case 0:
            deb = 0;/* 0.5 actually, but we don't allow user to set. */
            dbgmsg("ms should not be 0. eint_num:%d in %s\n",
                   cur_eint_num, __func__);
            break;
        case 1:
            deb = 1;
            break;
        case 2:
            deb = 16;
            break;
        case 3:
            deb = 32;
            break;
        case 4:
            deb = 64;
            break;
        case 5:
            deb = 128;
            break;
        case 6:
            deb = 256;
            break;
        case 7:
            deb = 512;
            break;
        default:
            deb = 0;
            printk("invalid deb time in the EIN_CON register, dbnc:%d, deb:%d\n", dbnc, deb);
            break;
        }
    }
    dbgmsg("sens:%d, pol:%d, deb:%d, isr:%p, autounmask:%d\n", sens, pol, deb, EINT_FUNC.eint_func[cur_eint_num], autounmask);
    return snprintf(buf, PAGE_SIZE, "%d\n", deb);
}
static ssize_t cur_eint_reg_isr_store(struct device_driver *driver,
                     const char *buf, size_t count)
{
    /* Get eint number */
    char *p = (char *)buf;
    unsigned int num;
    num = simple_strtoul(p, &p, 10);
    if (num != 1) {
        //dbgmsg("Unregister soft isr\n");
        printk("Unregister soft isr\n");
        mt_eint_mask(cur_eint_num);
    } else {
        /* register its ISR: mt_eint_soft_isr */
        /* level, high, deb time: 64ms, not auto unmask */
        mt_eint_set_sens(cur_eint_num, MT_LEVEL_SENSITIVE); 
        mt_eint_set_hw_debounce(cur_eint_num, EINT_FUNC.deb_time[cur_eint_num] );
        mt_eint_registration(cur_eint_num, EINTF_TRIGGER_HIGH, mt_eint_soft_isr, 0);
    }
    return count;
}
DRIVER_ATTR(current_eint_reg_isr, 0644, cur_eint_reg_isr_show, cur_eint_reg_isr_store);
#endif /*!EINT_TEST_SOFT */

/*
 * mt_eint_setdomain0: set all eint_num to domain 0.
 */
static void mt_eint_setdomain0(void)
{
    unsigned int base;
    unsigned int val = 0xFFFFFFFF, ap_cnt = (EINT_MAX_CHANNEL / 32), i;
    if (EINT_MAX_CHANNEL % 32)
        ap_cnt++;
    dbgmsg("[EINT] cnt:%d\n", ap_cnt);

    base = EINT_D0_EN_BASE;
    for (i = 0; i < ap_cnt; i++) {
        mt65xx_reg_sync_writel(val, base + (i * 4));
        dbgmsg("[EINT] domain addr:%x = %x\n", base, readl(IOMEM(base)));
    }
}

#ifdef MD_EINT
typedef struct{
  char  name[24];
  int   eint_num;
  int   eint_deb;
  int   eint_pol;
  int   eint_sens;
  int   socket_type;
}MD_SIM_HOTPLUG_INFO;

#define MD_SIM_MAX 16
MD_SIM_HOTPLUG_INFO md_sim_info[MD_SIM_MAX];
unsigned int md_sim_counter = 0;

int get_eint_attribute(char *name, unsigned int name_len, unsigned int type, char *result, unsigned int *len)
{
    int i;
    int ret = 0;
    int *sim_info = (int *)result;
    printk("in %s\n",__func__);
    //printk("[EINT]CUST_EINT_MD1_CNT:%d,CUST_EINT_MD2_CNT:%d\n",CUST_EINT_MD1_CNT,CUST_EINT_MD2_CNT);
    printk("[EINT]CUST_EINT_MD1_CNT:%d",CUST_EINT_MD1_CNT);
    printk("query info: name:%s, type:%d, len:%d\n", name,type,name_len);
    if (len == NULL || name == NULL || result == NULL)
    	return ERR_SIM_HOT_PLUG_NULL_POINTER;

    for (i = 0; i < md_sim_counter; i++){
        printk("compare string:%s\n", md_sim_info[i].name);
        if (!strncmp(name, md_sim_info[i].name, name_len))
        {
            switch(type)
            {
                case SIM_HOT_PLUG_EINT_NUMBER:
                    *len = sizeof(md_sim_info[i].eint_num);
                    memcpy(sim_info, &md_sim_info[i].eint_num, *len);
                    printk("[EINT]eint_num:%d\n", md_sim_info[i].eint_num);
                    break;

                case SIM_HOT_PLUG_EINT_DEBOUNCETIME:
                    *len = sizeof(md_sim_info[i].eint_deb);
                    memcpy(sim_info, &md_sim_info[i].eint_deb, *len);
                    printk("[EINT]eint_deb:%d\n", md_sim_info[i].eint_deb);
                    break;

                case SIM_HOT_PLUG_EINT_POLARITY:
                    *len = sizeof(md_sim_info[i].eint_pol);
                    memcpy(sim_info, &md_sim_info[i].eint_pol, *len);
                    printk("[EINT]eint_pol:%d\n", md_sim_info[i].eint_pol);
                    break;

                case SIM_HOT_PLUG_EINT_SENSITIVITY:
                    *len = sizeof(md_sim_info[i].eint_sens);
                    memcpy(sim_info, &md_sim_info[i].eint_sens, *len);
                    printk("[EINT]eint_sens:%d\n", md_sim_info[i].eint_sens);
                    break;

                case SIM_HOT_PLUG_EINT_SOCKETTYPE:
                    *len = sizeof(md_sim_info[i].socket_type);
                    memcpy(sim_info, &md_sim_info[i].socket_type, *len);
                    printk("[EINT]socket_type:%d\n", md_sim_info[i].socket_type);
                    break;
  
                default:
                    ret = ERR_SIM_HOT_PLUG_QUERY_TYPE;
                    *len = sizeof(int);
                    memset(sim_info, 0xff, *len);
                    break;
            }
            return ret;
        }
    }

    *len = sizeof(int);
    memset(sim_info, 0xff, *len);
 
    return ERR_SIM_HOT_PLUG_QUERY_STRING;
}
int get_type(char *name)
{

        int type1 = 0x0;
        int type2 = 0x0;
#if defined(CONFIG_MTK_SIM1_SOCKET_TYPE) || defined(CONFIG_MTK_SIM2_SOCKET_TYPE)
    char *p;
#endif

#ifdef CONFIG_MTK_SIM1_SOCKET_TYPE
    p = (char *)CONFIG_MTK_SIM1_SOCKET_TYPE;
    type1 = simple_strtoul(p, &p, 10);
#endif
#ifdef CONFIG_MTK_SIM2_SOCKET_TYPE
    p = (char *)CONFIG_MTK_SIM2_SOCKET_TYPE;
    type2 = simple_strtoul(p, &p, 10);
#endif
    if (!strncmp(name, "MD1_SIM1_HOT_PLUG_EINT", strlen("MD1_SIM1_HOT_PLUG_EINT")))
        return type1;
    else if (!strncmp(name, "MD1_SIM1_HOT_PLUG_EINT", strlen("MD1_SIM1_HOT_PLUG_EINT")))
        return type1;
    else if (!strncmp(name, "MD2_SIM2_HOT_PLUG_EINT", strlen("MD2_SIM2_HOT_PLUG_EINT")))
        return type2;
    else if (!strncmp(name, "MD2_SIM2_HOT_PLUG_EINT", strlen("MD2_SIM2_HOT_PLUG_EINT")))
        return type2;
    else 
        return 0;
}
#endif 

static void setup_MD_eint(void)
{
#ifdef MD_EINT
 //printk("[EINT]CUST_EINT_MD1_CNT:%d,CUST_EINT_MD2_CNT:%d\n",CUST_EINT_MD1_CNT,CUST_EINT_MD2_CNT);
 printk("[EINT]CUST_EINT_MD1_CNT:%d",CUST_EINT_MD1_CNT);

#if defined(CUST_EINT_MD1_0_NAME)
        sprintf(md_sim_info[md_sim_counter].name, CUST_EINT_MD1_0_NAME);
        md_sim_info[md_sim_counter].eint_num = CUST_EINT_MD1_0_NUM;
        md_sim_info[md_sim_counter].eint_pol= CUST_EINT_MD1_0_POLARITY;
        md_sim_info[md_sim_counter].eint_sens = CUST_EINT_MD1_0_SENSITIVE;
        md_sim_info[md_sim_counter].socket_type = get_type(md_sim_info[md_sim_counter].name);
        md_sim_info[md_sim_counter].eint_deb = CUST_EINT_MD1_0_DEBOUNCE_CN;
        printk("[EINT] MD1 name = %s\n", md_sim_info[md_sim_counter].name);
        printk("[EINT] MD1 type = %d\n", md_sim_info[md_sim_counter].socket_type);
        md_sim_counter++;
#endif
#if defined(CUST_EINT_MD1_1_NAME)
        sprintf(md_sim_info[md_sim_counter].name, CUST_EINT_MD1_1_NAME);
        md_sim_info[md_sim_counter].eint_num = CUST_EINT_MD1_1_NUM;
        md_sim_info[md_sim_counter].eint_pol= CUST_EINT_MD1_1_POLARITY;
        md_sim_info[md_sim_counter].eint_sens = CUST_EINT_MD1_1_SENSITIVE;
        md_sim_info[md_sim_counter].socket_type = get_type(md_sim_info[md_sim_counter].name);
        md_sim_info[md_sim_counter].eint_deb = CUST_EINT_MD1_1_DEBOUNCE_CN;
        printk("[EINT] MD1 name = %s\n", md_sim_info[md_sim_counter].name);
        printk("[EINT] MD1 type = %d\n", md_sim_info[md_sim_counter].socket_type);
        md_sim_counter++;
#endif
#if defined(CUST_EINT_MD1_2_NAME)
        sprintf(md_sim_info[md_sim_counter].name, CUST_EINT_MD1_2_NAME);
        md_sim_info[md_sim_counter].eint_num = CUST_EINT_MD1_2_NUM;
        md_sim_info[md_sim_counter].eint_pol= CUST_EINT_MD1_2_POLARITY;
        md_sim_info[md_sim_counter].eint_sens = CUST_EINT_MD1_2_SENSITIVE;
        md_sim_info[md_sim_counter].socket_type = get_type(md_sim_info[md_sim_counter].name);
        md_sim_info[md_sim_counter].eint_deb = CUST_EINT_MD1_2_DEBOUNCE_CN;
        printk("[EINT] MD1 name = %s\n", md_sim_info[md_sim_counter].name);
        printk("[EINT] MD1 type = %d\n", md_sim_info[md_sim_counter].socket_type);
        md_sim_counter++;
#endif
#if defined(CUST_EINT_MD1_3_NAME)
        sprintf(md_sim_info[md_sim_counter].name, CUST_EINT_MD1_3_NAME);
        md_sim_info[md_sim_counter].eint_num = CUST_EINT_MD1_3_NUM;
        md_sim_info[md_sim_counter].eint_pol= CUST_EINT_MD1_3_POLARITY;
        md_sim_info[md_sim_counter].eint_sens = CUST_EINT_MD1_3_SENSITIVE;
        md_sim_info[md_sim_counter].socket_type = get_type(md_sim_info[md_sim_counter].name);
        md_sim_info[md_sim_counter].eint_deb = CUST_EINT_MD1_3_DEBOUNCE_CN;
        printk("[EINT] MD1 name = %s\n", md_sim_info[md_sim_counter].name);
        printk("[EINT] MD1 type = %d\n", md_sim_info[md_sim_counter].socket_type);
        md_sim_counter++;
#endif
#if defined(CUST_EINT_MD1_4_NAME)
        sprintf(md_sim_info[md_sim_counter].name, CUST_EINT_MD1_4_NAME);
        md_sim_info[md_sim_counter].eint_num = CUST_EINT_MD1_4_NUM;
        md_sim_info[md_sim_counter].eint_pol= CUST_EINT_MD1_4_POLARITY;
        md_sim_info[md_sim_counter].eint_sens = CUST_EINT_MD1_4_SENSITIVE;
        md_sim_info[md_sim_counter].socket_type = get_type(md_sim_info[md_sim_counter].name);
        md_sim_info[md_sim_counter].eint_deb = CUST_EINT_MD1_4_DEBOUNCE_CN;
        printk("[EINT] MD1 name = %s\n", md_sim_info[md_sim_counter].name);
        printk("[EINT] MD1 type = %d\n", md_sim_info[md_sim_counter].socket_type);
        md_sim_counter++;
#endif

#if defined(CUST_EINT_MD2_0_NAME)
        sprintf(md_sim_info[md_sim_counter].name, CUST_EINT_MD2_0_NAME);
        md_sim_info[md_sim_counter].eint_num = CUST_EINT_MD2_0_NUM;
        md_sim_info[md_sim_counter].eint_pol= CUST_EINT_MD2_0_POLARITY;
        md_sim_info[md_sim_counter].eint_sens = CUST_EINT_MD2_0_SENSITIVE;
        md_sim_info[md_sim_counter].socket_type = get_type(md_sim_info[md_sim_counter].name);
        md_sim_info[md_sim_counter].eint_deb = CUST_EINT_MD2_0_DEBOUNCE_CN;
        printk("[EINT] MD2 name = %s\n", md_sim_info[md_sim_counter].name);
        printk("[EINT] MD2 type = %d\n", md_sim_info[md_sim_counter].socket_type);
        md_sim_counter++;
#endif
#if defined(CUST_EINT_MD2_1_NAME)
        sprintf(md_sim_info[md_sim_counter].name, CUST_EINT_MD2_1_NAME);
        md_sim_info[md_sim_counter].eint_num = CUST_EINT_MD2_1_NUM;
        md_sim_info[md_sim_counter].eint_pol= CUST_EINT_MD2_1_POLARITY;
        md_sim_info[md_sim_counter].eint_sens = CUST_EINT_MD2_1_SENSITIVE;
        md_sim_info[md_sim_counter].socket_type = get_type(md_sim_info[md_sim_counter].name);
        md_sim_info[md_sim_counter].eint_deb = CUST_EINT_MD2_1_DEBOUNCE_CN;
        printk("[EINT] MD2 name = %s\n", md_sim_info[md_sim_counter].name);
        printk("[EINT] MD2 type = %d\n", md_sim_info[md_sim_counter].socket_type);
        md_sim_counter++;
#endif
#if defined(CUST_EINT_MD2_2_NAME)
        sprintf(md_sim_info[md_sim_counter].name, CUST_EINT_MD2_2_NAME);
        md_sim_info[md_sim_counter].eint_num = CUST_EINT_MD2_2_NUM;
        md_sim_info[md_sim_counter].eint_pol= CUST_EINT_MD2_2_POLARITY;
        md_sim_info[md_sim_counter].eint_sens = CUST_EINT_MD2_2_SENSITIVE;
        md_sim_info[md_sim_counter].socket_type = get_type(md_sim_info[md_sim_counter].name);
        md_sim_info[md_sim_counter].eint_deb = CUST_EINT_MD2_2_DEBOUNCE_CN;
        dbgmsg("[EINT] MD2 name = %s\n", md_sim_info[md_sim_counter].name);
        dbgmsg("[EINT] MD2 type = %d\n", md_sim_info[md_sim_counter].socket_type);
        md_sim_counter++;
#endif
#if defined(CUST_EINT_MD2_3_NAME)
        sprintf(md_sim_info[md_sim_counter].name, CUST_EINT_MD2_3_NAME);
        md_sim_info[md_sim_counter].eint_num = CUST_EINT_MD2_3_NUM;
        md_sim_info[md_sim_counter].eint_pol= CUST_EINT_MD2_3_POLARITY;
        md_sim_info[md_sim_counter].eint_sens = CUST_EINT_MD2_3_SENSITIVE;
        md_sim_info[md_sim_counter].socket_type = get_type(md_sim_info[md_sim_counter].name);
        md_sim_info[md_sim_counter].eint_deb = CUST_EINT_MD2_3_DEBOUNCE_CN;
        printk("[EINT] MD2 name = %s\n", md_sim_info[md_sim_counter].name);
        printk("[EINT] MD2 type = %d\n", md_sim_info[md_sim_counter].socket_type);
        md_sim_counter++;
#endif
#if defined(CUST_EINT_MD2_4_NAME)
        sprintf(md_sim_info[md_sim_counter].name, CUST_EINT_MD2_4_NAME);
        md_sim_info[md_sim_counter].eint_num = CUST_EINT_MD2_4_NUM;
        md_sim_info[md_sim_counter].eint_pol= CUST_EINT_MD2_4_POLARITY;
        md_sim_info[md_sim_counter].eint_sens = CUST_EINT_MD2_4_SENSITIVE;
        md_sim_info[md_sim_counter].socket_type = get_type(md_sim_info[md_sim_counter].name);
        md_sim_info[md_sim_counter].eint_deb = CUST_EINT_MD2_4_DEBOUNCE_CN;
        printk("[EINT] MD2 name = %s\n", md_sim_info[md_sim_counter].name);
        printk("[EINT] MD2 type = %d\n", md_sim_info[md_sim_counter].socket_type);
        md_sim_counter++;
#endif
#endif //MD_EINT
}

static unsigned int mt_eint_get_count(unsigned int eint_num)
{
    if(eint_num < EINT_MAX_CHANNEL)
        return EINT_FUNC.count[eint_num];

    return 0;
}

#if defined(EINT_TEST_V2)
unsigned long long hw_debounce_start;
unsigned long long hw_debounce_end;
int EINT_waiting;
static struct platform_driver eint_driver = {
    .driver = {
        .name = "eint_test",
        .bus = &platform_bus_type,
        .owner = THIS_MODULE,
    }
};

static void gpio_set_debounce(unsigned gpio, unsigned debounce);

static unsigned int hw_debounce_irq = 3;

static irqreturn_t mt_eint_soft_isr(unsigned irq, struct irq_desc *desc)
{
    int eint_num=0;
    eint_num=DEMUX_EINT_IRQ(irq);
    printk("======EINT_SOFT_ISR======\n");
    printk("EINT %d, in mt_eint_soft_isr\n",eint_num);
    mt_eint_soft_clr(eint_num);
    printk("======EINT_SOFT_ISR_END======\n");
    return IRQ_HANDLED;
}

static irqreturn_t mt_eint_hw_debounce_isr(unsigned irq, struct irq_desc *desc)
{
    int eint_num=0;
    eint_num=DEMUX_EINT_IRQ(irq);
    hw_debounce_end=sched_clock();
    EINT_waiting=0;
    printk("======EINT_HW_DEBOUNCE_ISR======\n");
    printk("EINT %d, in mt_eint_hw_debounce_isr\n",eint_num);
    printk("======EINT_HW_DEBOUNCE_ISR_END======\n");
    mt_set_gpio_out(1, 0);
    return IRQ_HANDLED;
}

typedef void (*EINT_FUNC_PTR)(void);

static void mt_eint_test(void)
{
    int eint_num=0;
    int sens = 1;
    int pol = 1;
    int is_auto_umask = 0;
    int ret=0;
    mt_eint_mask(eint_num);
    mt_eint_set_polarity(eint_num, pol);
    mt_eint_set_sens(eint_num, sens);
    mt_eint_registration(eint_num, IRQF_TRIGGER_HIGH, (EINT_FUNC_PTR)mt_eint_soft_isr, is_auto_umask);
    if(ret>0)
    {
      printk(KERN_ERR "EINT IRQ LINE NOT AVAILABLE!!\n");
    }
    mt_eint_unmask(eint_num);
}

static void mt_eint_autounmask_test(void)
{
     int eint_num=0;
     int ret=0;
     struct irq_desc *desc;
     printk("%s for EINT %d\n", __func__, eint_num);
     ret=request_irq(EINT_IRQ(eint_num),(irq_handler_t)mt_eint_soft_isr,IRQF_TRIGGER_HIGH ,"EINT-0",NULL);
     if(ret>0) {
         printk(KERN_ERR "EINT IRQ LINE NOT AVAILABLE!!\n");
         return;
     }
     desc=irq_to_desc(EINT_IRQ(eint_num));
     printk("EINT %d request_irq done\n",eint_num);
     mt_eint_soft_set(eint_num);
     printk("trigger EINT %d done\n",eint_num);
     printk("EINT %d, MASK 0x%d\n",eint_num,mt_eint_get_mask(eint_num));
     free_irq(EINT_IRQ(eint_num),NULL);
}

static struct EINT_HEADER header = {
    .is_autounmask = 0,
};

static int mt_eint_non_autounmask_test(void)
{
     int eint_num=0;
     int ret = 0;
     //mt_eint_save_all_config();
     //test_init();
     //EINT_wq=create_workqueue("EINT_events");
     //setup_level_trigger_env();
     EINT_FUNC.eint_func[eint_num]=NULL;
     printk("%s for EINT %d\n", __func__, eint_num);
     ret=request_irq(EINT_IRQ(eint_num), (irq_handler_t)mt_eint_soft_isr,IRQF_TRIGGER_HIGH ,"EINT-0", &header);
     if(ret>0) {
         printk(KERN_ERR "EINT IRQ LINE NOT AVAILABLE!!\n");
         return -1;
     }
     printk("EINT %d request_irq done\n",eint_num);
     mt_eint_soft_set(eint_num);
//     printk("delay 5s to see mask status\n");
//     mdelay(5000);
     printk("trigger EINT %d done\n",eint_num);
     printk("EINT %d, MASK 0x%d\n",eint_num,mt_eint_get_mask(eint_num));
     free_irq(EINT_IRQ(eint_num),NULL);
     //mt_eint_store_all_config();
     return 1;
}

static void mt_eint_normal_test_basedon_sw_debounce(void)
{
     int eint_num=0;
     int ret=0;
     unsigned int debounce_time=30;
     printk("%s for EINT %d\n", __func__, eint_num);
     gpio_set_debounce(EINT_GPIO(eint_num),debounce_time);
     printk("EINT %d debounce enable %d\n",eint_num,mt_eint_is_debounce_en(eint_num));
     ret=request_irq(EINT_IRQ(eint_num), (irq_handler_t)mt_eint_soft_isr,IRQF_TRIGGER_HIGH ,"EINT-0",NULL);
     if(ret>0)
         printk(KERN_ERR "EINT IRQ LINE NOT AVAILABLE!!\n");
     printk("EINT %d request_irq done\n",eint_num);
     mt_eint_soft_set(eint_num);
     printk("trigger EINT %d done\n",eint_num);
     printk("EINT %d, MASK 0x%d\n",eint_num,mt_eint_get_mask(eint_num));
     free_irq(EINT_IRQ(eint_num),NULL);
}

static void mt_eint_normal_test_basedon_hw_debounce(void)
{
     int eint_num=hw_debounce_irq;
     int ret=0;
     unsigned int debounce_time=16;
     unsigned long long delay_start;
     unsigned long long delay_end;
     printk("%s for EINT %d\n", __func__, eint_num);
     gpio_set_debounce(EINT_GPIO(eint_num),debounce_time);

     /* GPIO setting */
     mt_set_gpio_mode(1, GPIO_MODE_00);
     mt_set_gpio_dir(1, GPIO_DIR_OUT);
     mt_set_gpio_pull_enable(1, 1);
     //mt_set_gpio_pull_select(1, 1);
     mt_set_gpio_out(1, 0);
     //mt_set_gpio_mode(0, GPIO_MODE_00);
     //mt_set_gpio_dir(0, GPIO_DIR_OUT);
     //mt_set_gpio_pull_enable(0, 1);
     //mt_set_gpio_pull_select(0, 1);
     //mt_set_gpio_out(0, 1);
     mt_set_gpio_mode(3, GPIO_MODE_00);
     mt_set_gpio_dir(3, GPIO_DIR_IN);
  
     //printk("1 eint num %d raw satatus:%d\n",eint_num,mt_eint_get_raw_status(eint_num));
     //printk("*********** \nEINT hw_debounce TEST\n*********** \n");
     //printk("[EINT LEVEL]eint num %d HW Debounce 16 \n",eint_num);
     //printk("2 eint num %d raw satatus:%d\n",eint_num,mt_eint_get_raw_status(eint_num));
     //printk("3 eint num %d raw satatus:%d\n",eint_num,mt_eint_get_raw_status(eint_num));
     ret=request_irq(EINT_IRQ(eint_num), (irq_handler_t)mt_eint_hw_debounce_isr,IRQF_TRIGGER_HIGH ,"EINT-3",NULL);
     if(ret>0)
         printk(KERN_ERR "EINT IRQ LINE NOT AVAILABLE!!\n");
     printk("EINT %d request_irq done\n",eint_num);
     //printk("4 eint num %d raw satatus:%d\n",eint_num,mt_eint_get_raw_status(eint_num));

     EINT_waiting=1;
     //for(i=0;i<10;i++)
     //    printk("eint num %d raw satatus:%d\n",eint_num,mt_eint_get_raw_status(eint_num));
     delay_start=sched_clock();
     mt_set_gpio_out(1, 1);
     printk("trigger EINT %d done\n",eint_num);     
     delay_end=delay_start+0x2000000;    
     hw_debounce_start=delay_start;
     mt_eint_unmask(eint_num);
    
     while(delay_start < delay_end)
     {
         if(!EINT_waiting)
             break;
         delay_start=sched_clock();
     }
     printk("hw_debounce time t1=%llu ,t2=%llu, %llu ns\n",hw_debounce_start,hw_debounce_end,(hw_debounce_end-hw_debounce_start));
     printk("duration time t1=%llu ,t2=%llu, %llu ns\n",hw_debounce_start,delay_start,(delay_start-hw_debounce_start));
     if(!EINT_waiting){
         printk("...pass\n");
     }
     else{
         printk("...failed\n");
     }
     printk("EINT %d, MASK 0x%d\n",eint_num,mt_eint_get_mask(eint_num));
     free_irq(EINT_IRQ(eint_num),NULL);
}

//static ssize_t test_show(struct device* dev, struct device_attribute* attr, char* buf)
static ssize_t test_show(struct device_driver *driver, char *buf)
{
   return snprintf(buf, PAGE_SIZE, "==EINT test==\n"
                                   "1.EINT original registration test\n"
                                   "2.EINT new design test\n"
                                   "3.EINT new design hw debounce test\n"
                                   "4.EINT new design sw debounce test\n"
                                   "5.EINT new design without auto-unmask\n"
   );
}

//static ssize_t test_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
static ssize_t test_store(struct device_driver *driver, const char *buf,size_t count)
{
    char *p = (char *)buf;
    unsigned int num;

    num = simple_strtoul(p, &p, 10);
        switch(num){
            /* Test APDMA Normal Function */
            case 1:
                mt_eint_test();
                break;
            case 2:
                mt_eint_autounmask_test();
                break;
            case 3:
                mt_eint_normal_test_basedon_hw_debounce();
                break;
            case 4:
                mt_eint_normal_test_basedon_sw_debounce();
                break;
            case 5:
                mt_eint_non_autounmask_test();
                break;
            default:
                break;
        }

    return count;
}

DRIVER_ATTR(eint_test_suit, 0664, test_show, test_store);

//DEVICE_ATTR(eint_config, 0666, test_show, test_store);

//static struct miscdevice mt_eint_dev = {
//    .minor      = MISC_DYNAMIC_MINOR,
//    .name       = "mt_eint",
//    //.fops       = &dev_fops,
//    .mode       = S_IRUGO | S_IWUGO,
//};
#endif
#if 0
void mt_eint_test()
{
    int cur_eint_num = 200;
    int sens = 1;
    int pol = 1;
    int is_en_db = 0;
    int is_auto_umask = 0;
    
    mt_eint_mask(cur_eint_num);
    mt_eint_set_polarity(cur_eint_num, pol);
    mt_eint_set_sens(cur_eint_num, sens);
    mt_eint_registration(cur_eint_num, EINTF_TRIGGER_HIGH, mt_eint_soft_isr, is_auto_umask);
    mt_eint_unmask(cur_eint_num);
    mt_eint_emu_set(cur_eint_num);
}
#endif

static void gpio_set_debounce(unsigned gpio, unsigned debounce)
{
    mt_eint_set_hw_debounce(gpio, debounce); 
}


/*
 * mt_eint_print_status: Print the EINT status register.
 */
void mt_eint_print_status(void)
{
    unsigned int status,index;
    unsigned int offset,reg_base,status_check;
    printk(KERN_DEBUG"EINT_STA:");
     for (reg_base = 0; reg_base < EINT_MAX_CHANNEL; reg_base+=32) {
            /* read status register every 32 interrupts */
            status = mt_eint_get_status(reg_base);
            if(status){
                //dbgmsg(KERN_DEBUG"EINT Module - index:%d,EINT_STA = 0x%x\n",
                //  reg_base, status);
            }
            else{
                continue;
            }
            for(offset = 0; offset < 32; offset++){
                index = reg_base + offset;
                if (index >= EINT_MAX_CHANNEL) break;

                status_check = status & (1 << offset);
                if (status_check) 
                        printk(KERN_DEBUG" %d",index);
            }

    }
    printk(KERN_DEBUG"\n");
}
#ifdef DEINT_SUPPORT
void mt_eint_test(void)
{
    printk("test for eint re-registration\n");
}

static irqreturn_t  mt_deint_test_isr(int irq, void *dev_id)
{
    printk("test for deint isr\n");
    //mt_irq_is_active(irq);
}
#endif

extern struct irq_chip mt_irq_eint;

/*
 * mt_eint_init: initialize EINT driver.
 * Always return 0.
 */
static int __init mt_eint_init(void)
{
    unsigned int i;
    int irq_base;
    struct mt_eint_driver *eint_drv;
    struct irq_domain *domain;
#if defined(EINT_TEST)
    int ret;
#endif

    /* assign to domain 0 for AP */
    mt_eint_setdomain0();

    wake_lock_init(&EINT_suspend_lock, WAKE_LOCK_SUSPEND, "EINT wakelock");

    //if (request_irq(EINT_IRQ, (irq_handler_t)mt_eint_demux, IRQF_TRIGGER_HIGH, "EINT", NULL)) {
    //    printk(KERN_ERR "EINT IRQ LINE NOT AVAILABLE!!\n");
    //}
    
    setup_MD_eint();
    for (i = 0; i < EINT_MAX_CHANNEL; i++) {
        EINT_FUNC.eint_func[i] = NULL;
        EINT_FUNC.is_deb_en[i] = 0;
        EINT_FUNC.deb_time[i] = 0;
        EINT_FUNC.eint_sw_deb_timer[i].expires = 0;
        EINT_FUNC.eint_sw_deb_timer[i].data = 0;
        EINT_FUNC.eint_sw_deb_timer[i].function = NULL;
#if defined(EINT_TEST)
        EINT_FUNC.softisr_called[i] = 0;
#endif
    }

    /* register EINT driver */
    eint_drv = get_mt_eint_drv();
    eint_drv->eint_max_channel = mt_eint_max_channel;
    eint_drv->enable = mt_eint_unmask;
    eint_drv->disable = mt_eint_mask;
    eint_drv->is_disable = mt_eint_get_mask;
    eint_drv->get_sens =  mt_eint_get_sens;
    eint_drv->set_sens = mt_eint_set_sens;
    eint_drv->get_polarity = mt_eint_get_polarity;
    eint_drv->set_polarity = mt_eint_set_polarity;
    eint_drv->get_debounce_cnt =  mt_eint_get_debounce_cnt;
    eint_drv->set_debounce_cnt = mt_eint_set_hw_debounce;
    eint_drv->is_debounce_en = mt_eint_is_debounce_en;
    eint_drv->enable_debounce = mt_eint_enable_debounce;
    eint_drv->disable_debounce = mt_eint_disable_debounce;
    eint_drv->get_count = mt_eint_get_count;

    /* Register Linux IRQ interface */
    irq_base = irq_alloc_descs(EINT_IRQ_BASE, EINT_IRQ_BASE, EINT_MAX_CHANNEL, numa_node_id());
    if (irq_base != EINT_IRQ_BASE)
        printk(KERN_ERR "EINT alloc desc error %d\n", irq_base);

    for (i = 0 ; i < EINT_MAX_CHANNEL; i++) {
            irq_set_chip_and_handler(i + EINT_IRQ_BASE, &mt_irq_eint,
                                     handle_level_irq);
            set_irq_flags(i + EINT_IRQ_BASE, IRQF_VALID);
    }

    domain = irq_domain_add_legacy(NULL, EINT_MAX_CHANNEL, EINT_IRQ_BASE, 0,
                                &irq_domain_simple_ops, eint_drv);
    if (!domain)
        printk(KERN_ERR "EINT domain add error\n");

    irq_set_chained_handler(EINT_IRQ, (irq_flow_handler_t)mt_eint_demux);
    /* Test */ 
    //mt_eint_registration(11, EINTF_TRIGGER_LOW, mt_eint_test, true);
    //mt_deint_registration(11, EINTF_TRIGGER_HIGH, mt_deint_test_isr);
#if defined(EINT_TEST_V2)
    //int ret;
    //ret = misc_register(&mt_eint_dev);

    //if(ret < 0) {
    //    printk("EINT MISC Register fail, ret = %d\n", ret);
    //    return ret;
    //}

    //device_create_file(mt_eint_dev.this_device, &dev_attr_eint_config);

    ret = platform_driver_register(&eint_driver);
    if (ret) {
        pr_err("Fail to register eint_driver");
    }
    ret |= driver_create_file(&eint_driver.driver, &driver_attr_eint_test_suit);
    if (ret) {
        pr_err("Fail to create eint_driver sysfs files");
    }
#endif

    return 0;
}
#if 0
void mt_eint_dump_status(unsigned int eint)
{
   if (eint >= EINT_MAX_CHANNEL)
       return;
   printk("[EINT] eint:%d,mask:%x,pol:%x,deb:%x,sens:%x\n",eint,mt_eint_get_mask(eint),mt_eint_get_polarity(eint),mt_eint_get_debounce_cnt(eint),mt_eint_get_sens(eint));
}
#endif

arch_initcall(mt_eint_init);

EXPORT_SYMBOL(mt_eint_dis_debounce);
EXPORT_SYMBOL(mt_eint_registration);
EXPORT_SYMBOL(mt_eint_set_hw_debounce);
EXPORT_SYMBOL(mt_eint_set_polarity);
EXPORT_SYMBOL(mt_eint_set_sens);
EXPORT_SYMBOL(mt_eint_mask);
EXPORT_SYMBOL(mt_eint_unmask);
EXPORT_SYMBOL(mt_eint_print_status);

#if defined(EINT_TEST)
EXPORT_SYMBOL(mt_eint_soft_set);
#endif

/* first MP list */
struct cust_eint cust_eint_chr_stat;
struct cust_eint cust_eint_als;
struct cust_eint cust_eint_touch_panel;
struct cust_eint cust_eint_gse_2;
struct cust_eint cust_eint_accdet;
struct cust_eint cust_eint_gse_1;
struct cust_eint cust_eint_mt6323_pmic;

static int cust_eint_sanity_check(int num, int debounce_cn, int type, int debounce_en)
{
	if ((num < 0) || (num > EINT_MAX_CHANNEL)) {
		pr_err("In %s: num(%d) out of range\n", __func__, num);
		return -1;
	}

	if ((type != CUST_EINTF_TRIGGER_RISING) && (type != CUST_EINTF_TRIGGER_FALLING) &&
		(type != CUST_EINTF_TRIGGER_HIGH) && (type != CUST_EINTF_TRIGGER_LOW)) {
		pr_err("In %s: type(%d) out of range\n", __func__, type);
		return -1;
	}

	if ((debounce_en != CUST_EINT_DEBOUNCE_ENABLE) && (debounce_en != CUST_EINT_DEBOUNCE_DISABLE)) {
		pr_err("In %s: debounce_en(%d) out of range\n", __func__, debounce_en);
		return -1;
	}

	return 0;
}

static inline int cust_eint_dev_init(struct cust_eint *dev,
	int num, int debounce_cn, int type, int debounce_en)
{
	if (unlikely(!dev)) {
		pr_err("In %s:%d: eint dev does not exist\n", __func__, __LINE__);
		return -1;
	}

	if (unlikely(cust_eint_sanity_check(num, debounce_cn, type, debounce_en) != 0)) {
		pr_err("In %s:%d: %s has no proper attribute num = %d, debounce_cn = %d, type = %d, debounce_en = %d\n",
			__func__, __LINE__, dev->name, num, debounce_cn, type, debounce_en);
		return -1;
	}

	dev->num = num;
	dev->debounce_cn = debounce_cn;
	dev->type = type;
	dev->debounce_en = debounce_en;

	return 0;
}

/*
    This function is for those loads without SSB info,
    setup each device's attributes of EINT as the first MP setting
*/
void __init eint_fixup_default(void)
{
	pr_notice("EINT set default settings!!\n");

	strncpy(cust_eint_chr_stat.name, "CHR_STAT", sizeof(cust_eint_chr_stat.name));
	cust_eint_dev_init(&cust_eint_chr_stat, 0, 0, CUST_EINTF_TRIGGER_LOW, CUST_EINT_DEBOUNCE_DISABLE);

	strncpy(cust_eint_als.name, "ALS", sizeof(cust_eint_als.name));
	cust_eint_dev_init(&cust_eint_als, 1, 0, CUST_EINTF_TRIGGER_FALLING, CUST_EINT_DEBOUNCE_DISABLE);
	
	strncpy(cust_eint_touch_panel.name, "TOUCH_PANEL", sizeof(cust_eint_touch_panel.name));
	cust_eint_dev_init(&cust_eint_touch_panel, 2, 0, CUST_EINTF_TRIGGER_FALLING, CUST_EINT_DEBOUNCE_DISABLE);

	strncpy(cust_eint_gse_2.name, "GSE_2", sizeof(cust_eint_gse_2.name));
	cust_eint_dev_init(&cust_eint_gse_2, 3, 0, CUST_EINTF_TRIGGER_RISING, CUST_EINT_DEBOUNCE_DISABLE);

	strncpy(cust_eint_accdet.name, "ACCDET", sizeof(cust_eint_accdet.name));
	cust_eint_dev_init(&cust_eint_accdet, 4, 256, CUST_EINTF_TRIGGER_LOW, CUST_EINT_DEBOUNCE_ENABLE);

	strncpy(cust_eint_gse_1.name, "GSE_1", sizeof(cust_eint_gse_1.name));
	cust_eint_dev_init(&cust_eint_gse_1, 5, 0, CUST_EINTF_TRIGGER_FALLING, CUST_EINT_DEBOUNCE_DISABLE);

	strncpy(cust_eint_mt6323_pmic.name, "MT6323_PMIC", sizeof(cust_eint_mt6323_pmic.name));
	cust_eint_dev_init(&cust_eint_mt6323_pmic, 25, 1, CUST_EINTF_TRIGGER_HIGH, CUST_EINT_DEBOUNCE_ENABLE);
}

struct cust_eint_item {
	char name[64];
	struct cust_eint *dev;
};

struct cust_eint_item cust_eint_items[] = {
	{ "CHR_STAT", &cust_eint_chr_stat },
	{ "ALS", &cust_eint_als },
	{ "TOUCH_PANEL", &cust_eint_touch_panel },
	{ "GSE_2", &cust_eint_gse_2 },
	{ "ACCDET", &cust_eint_accdet },
	{ "GSE_1", &cust_eint_gse_1 },
	{ "MT6323_PMIC", &cust_eint_mt6323_pmic },	
};

struct cust_eint *find_cust_eint(const char *name)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(cust_eint_items) ; ++i) {
		if (strcmp(cust_eint_items[i].name, name) == 0) {
			return cust_eint_items[i].dev;
		}
	}

	return NULL;
}

int __init parse_tag_eint_ssb_fixup(const struct tag *tags)
{
	const struct tag_eint_data *data = &tags->u.eint_data;
	int i = 0;

	pr_notice("EINT magic: %c%c%c%c\n", data->magic[0], data->magic[1], data->magic[2], data->magic[3]);
	pr_notice("COUNT = %d\n", data->count);

	for (i = 0; i < data->count; ++i) {
		struct cust_eint *setting = (struct cust_eint *)&data->payload[i*sizeof(struct cust_eint)];
		struct cust_eint *dev = NULL;

		if ((dev = find_cust_eint(setting->name)) == NULL) {
			pr_warn("EINT Not found: NAME = %s, num = %d, debounce_cn = %d, type = %d, debounce_en = %d\n",
				setting->name, setting->num, setting->debounce_cn, setting->type, setting->debounce_en);
			continue;
		}

		strncpy(dev->name, setting->name, sizeof(dev->name));
		cust_eint_dev_init(dev, setting->num,
					setting->debounce_cn,
					setting->type,
					setting->debounce_en);

		pr_notice("EINT GOT NAME = %s, num = %d, debounce_cn = %d, type = %d, debounce_en = %d\n",
				dev->name, dev->num, dev->debounce_cn, dev->type, dev->debounce_en);
	}

	return 0;
}
