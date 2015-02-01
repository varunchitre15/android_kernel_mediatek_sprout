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

#include <mach/charging.h>
#include <mach/mt6333.h>
#include <mach/upmu_common.h>
#include <mach/mt_gpio.h>
#include <cust_gpio_usage.h>
#include <mach/upmu_hw.h>
#include <linux/xlog.h>
#include <linux/delay.h>
#include <mach/mt_sleep.h>
#include <mach/mt_boot.h>
#include <mach/system.h>
#include <cust_charging.h>
#include <mach/charging_hw_common.h>
#include <mach/battery_ssb.h>

// ============================================================ //
//define
// ============================================================ //

#define MTK_SWCHR_SW_RECHR
#define SWCHR_HW_RECHR_VOLTAGE 4050

// ============================================================ //
//global variable
// ============================================================ //

const kal_uint32 VBAT_CV_VTH_HW6333[]=
{
    BATTERY_VOLT_04_450000_V, BATTERY_VOLT_04_350000_V,	BATTERY_VOLT_04_300000_V, BATTERY_VOLT_04_200000_V,
    BATTERY_VOLT_04_100000_V, BATTERY_VOLT_04_000000_V,	BATTERY_VOLT_03_900000_V, BATTERY_VOLT_03_800000_V,
    BATTERY_VOLT_03_700000_V, BATTERY_VOLT_03_600000_V
};

const kal_uint32 CS_VTH_HW6333[]=
{
    CHARGE_CURRENT_100_00_MA,   CHARGE_CURRENT_200_00_MA,	CHARGE_CURRENT_300_00_MA,  CHARGE_CURRENT_400_00_MA,
    CHARGE_CURRENT_500_00_MA,   CHARGE_CURRENT_600_00_MA,	CHARGE_CURRENT_700_00_MA,  CHARGE_CURRENT_800_00_MA,
    CHARGE_CURRENT_900_00_MA,   CHARGE_CURRENT_1000_00_MA,	CHARGE_CURRENT_1100_00_MA, CHARGE_CURRENT_1200_00_MA,
    CHARGE_CURRENT_1300_00_MA,  CHARGE_CURRENT_1400_00_MA,	CHARGE_CURRENT_1500_00_MA, CHARGE_CURRENT_1600_00_MA
}; 

const kal_uint32 INPUT_CS_VTH_HW6333[]=
{
    CHARGE_CURRENT_100_00_MA,   CHARGE_CURRENT_200_00_MA,	CHARGE_CURRENT_300_00_MA,  CHARGE_CURRENT_400_00_MA,
    CHARGE_CURRENT_500_00_MA,   CHARGE_CURRENT_600_00_MA,	CHARGE_CURRENT_700_00_MA,  CHARGE_CURRENT_800_00_MA,
    CHARGE_CURRENT_900_00_MA,   CHARGE_CURRENT_1000_00_MA,	CHARGE_CURRENT_1100_00_MA, CHARGE_CURRENT_1200_00_MA,
    CHARGE_CURRENT_1300_00_MA,  CHARGE_CURRENT_1400_00_MA,	CHARGE_CURRENT_1500_00_MA, CHARGE_CURRENT_1600_00_MA
}; 

const kal_uint32 VCDT_HV_VTH[]=
{
    BATTERY_VOLT_10_000000_V, BATTERY_VOLT_09_000000_V,	  BATTERY_VOLT_08_000000_V,   BATTERY_VOLT_07_000000_V		  
};

// ============================================================ //
// function prototype
// ============================================================ //

// ============================================================ //
//extern variable
// ============================================================ //

// ============================================================ //
//extern function
// ============================================================ //
extern bool mt_usb_is_device(void);
extern int PMIC_IMM_GetOneChannelValue(int dwChannel, int deCount, int trimd);
extern kal_uint32 mt6333_get_reg_value(kal_uint32 reg);

// ============================================================ //
static kal_uint32 charging_hw_init_hw6333(void *data)
{
    kal_uint32 status = STATUS_OK;
    int polling_timeout_value = 10;
    int polling_time = 0;

#if defined(MTK_WIRELESS_CHARGER_SUPPORT)
	mt_set_gpio_mode(wireless_charger_gpio_number,0); // 0:GPIO mode
	mt_set_gpio_dir(wireless_charger_gpio_number,0); // 0: input, 1: output
#endif
    mt6333_set_rg_usbdl_mode_b(1);

    while(mt6333_get_rgs_power_on_ready() != 1)
    {
        if(polling_time++ >= polling_timeout_value)
        {
            battery_xlog_printk(BAT_LOG_FULL, "check rgs_power_on_ready fail\n");
            break;
        }
    }
    battery_xlog_printk(BAT_LOG_CRTI, "polling_time=%d of rgs_power_on_ready\n", polling_time);

    if (high_battery_volt_enable) {
        mt6333_set_rg_cv_sel(0);
        battery_xlog_printk(BAT_LOG_CRTI, "HIGH_BATTERY_VOLTAGE_SUPPORT\n");
    }

    return status;
}

static kal_uint32 charging_dump_register_hw6333(void *data)
{
    kal_uint32 status = STATUS_OK;

    //mt6333_dump_register();

    return status;
}

static kal_uint32 charging_enable_hw6333(void *data)
{
    kal_uint32 status = STATUS_OK;
    kal_uint32 enable = *(kal_uint32*)(data);
    kal_uint32 ret = 0;

    if(KAL_TRUE == enable)
    {
        //mt6333_set_rg_chr_en(1);
        ret=mt6333_config_interface(0x04, 0x95, 0xFF, 0x0);
    }
    else
    {

#if defined(CONFIG_USB_MTK_HDRC_HCD)
        if(mt_usb_is_device())
#endif 			
        {
            //mt6333_set_rg_chr_en(0);
            ret=mt6333_config_interface(0x04, 0x94, 0xFF, 0x0);
        }
    }
	
    return status;
}

static kal_uint32 charging_set_cv_voltage_hw6333(void *data)
{
    kal_uint32 status = STATUS_OK;
    kal_uint16 register_value;

    register_value = charging_parameter_to_value(VBAT_CV_VTH_HW6333, GETARRAYNUM(VBAT_CV_VTH_HW6333) ,*(kal_uint32 *)(data));

    mt6333_set_rg_cv_sel(register_value); 
    
    return status;
}

static kal_uint32 charging_get_current_hw6333(void *data)
{
    kal_uint32 status = STATUS_OK;
    kal_uint32 array_size;

    array_size = GETARRAYNUM(CS_VTH_HW6333);
    *(kal_uint32 *)data = bmt_find_closest_level(CS_VTH_HW6333, array_size, *(kal_uint32 *)data);

    return status;
}  

static kal_uint32 charging_set_current_hw6333(void *data)
{
    kal_uint32 status = STATUS_OK;
    kal_uint32 set_chr_current;
    kal_uint32 array_size;
    kal_uint32 register_value;
    kal_uint32 current_value = *(kal_uint32 *)data;

    array_size = GETARRAYNUM(CS_VTH_HW6333);
    set_chr_current = bmt_find_closest_level(CS_VTH_HW6333, array_size, current_value);
    register_value = charging_parameter_to_value(CS_VTH_HW6333, array_size ,set_chr_current);

    mt6333_set_rg_ich_sel(register_value);

    return status;
}

static kal_uint32 charging_set_input_current_hw6333(void *data)
{
    kal_uint32 status = STATUS_OK;
    kal_uint32 set_chr_current;
    kal_uint32 array_size;
    kal_uint32 register_value;

    //20130604, Quick charging by Tim
    mt6333_set_rg_input_cc_reg(1);

    array_size = GETARRAYNUM(INPUT_CS_VTH_HW6333);
    set_chr_current = bmt_find_closest_level(INPUT_CS_VTH_HW6333, array_size, *(kal_uint32 *)data);
    register_value = charging_parameter_to_value(INPUT_CS_VTH_HW6333, array_size ,set_chr_current);

    mt6333_set_rg_ich_sel(register_value);

    return status;
}

static kal_uint32 charging_get_charging_status_hw6333(void *data)
{
    kal_uint32 status = STATUS_OK;
    kal_uint32 ret_val = 0;

#ifdef MTK_SWCHR_SW_RECHR

    int vbat_bal = 0;
    kal_uint32 ret = 0;
    ret=mt6333_config_interface(0x2E, 0x00, 0xFF, 0x0);

    ret_val = mt6333_get_rgs_charge_complete_hw();
    if(ret_val == 0x1)
    {
        *(kal_uint32 *)data = KAL_TRUE;
        ret=mt6333_config_interface(0x04, 0x94, 0xFF, 0x0);

        vbat_bal = PMIC_IMM_GetOneChannelValue(7,3,1);
        if(vbat_bal <= SWCHR_HW_RECHR_VOLTAGE)
        {
            ret=mt6333_config_interface(0x04, 0x95, 0xFF, 0x0);
        }

        battery_xlog_printk(BAT_LOG_CRTI, "[MTK_SWCHR_SW_RECHR] Reg[0x04]=0x%x, Reg[0x19]=0x%x, Reg[0x2E]=0x%x, vbat_bal=%d, SWCHR_HW_RECHR_VOLTAGE=%d\n", 
            mt6333_get_reg_value(0x04), mt6333_get_reg_value(0x19), mt6333_get_reg_value(0x2E), vbat_bal, SWCHR_HW_RECHR_VOLTAGE);
    }
    else
        *(kal_uint32 *)data = KAL_FALSE;   

#else

    ret_val = mt6333_get_rgs_charge_complete_hw();    
    if(ret_val == 0x1)
        *(kal_uint32 *)data = KAL_TRUE;
    else
        *(kal_uint32 *)data = KAL_FALSE;
    
#endif
    
    return status;
}

static kal_uint32 charging_reset_watch_dog_timer_hw6333(void *data)
{
    kal_uint32 status = STATUS_OK;
    kal_uint32 ret = 0;
    kal_uint8 val=0;
    int vbat_bal = 0;
    int check_vbat = 3950;

#if 1
    // cable plug-out sw workaround
    ret=mt6333_read_interface(0x04, (&val),0x1, 0);
    if(val==1)
    {
        vbat_bal = PMIC_IMM_GetOneChannelValue(7,3,1);
        if(vbat_bal > check_vbat)
        {
            ret=mt6333_config_interface(0x04, 0x94, 0xFF, 0x0);
            mdelay(50);
            ret=mt6333_config_interface(0x04, 0x95, 0xFF, 0x0);        
        }
    }
    battery_xlog_printk(BAT_LOG_CRTI, "chr_en=%d, curr_vbat=%d, check_vbat=%d\n", val, vbat_bal, check_vbat);
#endif    
    
    mt6333_set_rg_chrwdt_wr(1); // write 1 to kick chr wdt
    mt6333_set_rg_chrwdt_td(1); // 32s
    mt6333_set_rg_chrwdt_en(1); 
    
    return status;
}

static kal_uint32 charging_set_hv_threshold_hw6333(void *data)
{
    kal_uint32 status = STATUS_OK;

    kal_uint32 set_hv_voltage;
    kal_uint32 array_size;
    kal_uint16 register_value;
    kal_uint32 voltage = *(kal_uint32*)(data);

    array_size = GETARRAYNUM(VCDT_HV_VTH);
    set_hv_voltage = bmt_find_closest_level(VCDT_HV_VTH, array_size, voltage);
    register_value = charging_parameter_to_value(VCDT_HV_VTH, array_size ,set_hv_voltage);

    mt6333_set_rg_chrin_hv_vth(register_value);

    return status;
}

static kal_uint32 charging_get_hv_status_hw6333(void *data)
{
    kal_uint32 status = STATUS_OK;

    *(kal_bool*)(data) = mt6333_get_rgs_chr_hv_det();

    return status;
}

kal_uint32 (* const charging_func_hw6333[CHARGING_CMD_NUMBER])(void *data)=
{
    charging_hw_init_hw6333
    ,charging_dump_register_hw6333
    ,charging_enable_hw6333
    ,charging_set_cv_voltage_hw6333
    ,charging_get_current_hw6333
    ,charging_set_current_hw6333
    ,charging_set_input_current_hw6333
    ,charging_get_charging_status_hw6333
    ,charging_reset_watch_dog_timer_hw6333
    ,charging_set_hv_threshold_hw6333
    ,charging_get_hv_status_hw6333
    ,charging_get_battery_status
    ,charging_get_charger_det_status
    ,charging_get_charger_type
    ,charging_get_is_pcm_timer_trigger
    ,charging_set_platform_reset
    ,charging_get_platfrom_boot_mode
    ,charging_set_power_off
};

