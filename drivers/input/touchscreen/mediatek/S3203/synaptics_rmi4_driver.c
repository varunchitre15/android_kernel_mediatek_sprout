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

#include <linux/interrupt.h>
#include <cust_eint.h>
#include <linux/i2c.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/delay.h>
#include "tpd_custom_synaptics_rmi4.h"
#include "cust_gpio_usage.h"
#include "tpd.h"
#include "synaptics_dsx_rmi4_i2c.h"
#include "SynaImage.h"

#undef TPD_DMESG
#define TPD_DMESG(a,arg...)

#include <mach/mt_pm_ldo.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_boot.h>
#include <mach/mt_touch_ssb_cust.h>

//#define  SYNAPTICS_DEBUG_IF
#ifdef  SYNAPTICS_DEBUG_IF
#include <linux/dma-mapping.h>
static unsigned char *tpDMABuf_va = NULL;
static u32 tpDMABuf_pa = 0;
#define  MAX_DATA_READ  100
extern atomic_t debug_flag;
#define FORCE_USE_DMA
#endif

//#define MULTI_TOUCH_PROTOCOL_TYPE_B

#ifdef MULTI_TOUCH_PROTOCOL_TYPE_B
#include <linux/input/mt.h>
#define  FTS_SUPPORT_TRACK_ID
#endif
#define  SYNAPTICS_TOUCH_TRACK_IDS  11


/* < DTS2012031404176  linghai 20120314 begin */
#ifdef TPD_HAVE_BUTTON
static int tpd_keys_local[TPD_KEY_COUNT]=TPD_KEYS;
static int tpd_keys_dim_local[TPD_KEY_COUNT][4]=TPD_KEYS_DIM;
#endif
/* DTS2012031404176  linghai 20120314 end> */
//add by huxin
#ifdef HAVE_TOUCH_KEY
const u16 touch_key_array[] = { KEY_BACK, KEY_HOMEPAGE, KEY_MENU};
#define MAX_KEY_NUM ( sizeof( touch_key_array )/sizeof( touch_key_array[0] ) )
#endif


#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
static int tpd_wb_start_local[TPD_WARP_CNT] = TPD_WARP_START;
static int tpd_wb_end_local[TPD_WARP_CNT]   = TPD_WARP_END;
#endif
#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
static int tpd_calmat_local[8]     = TPD_CALIBRATION_MATRIX;
static int tpd_def_calmat_local[8] = TPD_CALIBRATION_MATRIX;
#endif

static struct tag_para_touch_ssb_data_single touch_ssb_data = {0};

static struct point {
    int x;
    int raw_x;
    int y;
    int raw_y;
    int z;
    int status;
    int per_status;
    int down_count;
};

struct function_descriptor {
    u16 query_base;
    u16 cmd_base;
    u16 ctrl_base;
    u16 data_base;
    u8 intSrc;
#define FUNCTION_VERSION(x) ((x >> 5) & 3)
#define INTERRUPT_SOURCE_COUNT(x) (x & 7)

    u8 functionNumber;
};

struct tpd_data {
    struct i2c_client *client;
    struct function_descriptor f01;
    struct function_descriptor f11;
    struct function_descriptor f1a;

    u8 fn11_mask;
    u8 fn1a_mask;


    struct point *cur_points;
    struct point *pre_points;
    struct mutex io_ctrl_mutex;
    struct work_struct work;
    int f11_max_x, f11_max_y;
    u8 points_supported;
    u8 data_length;
    u8 current_page;
};

struct tpd_debug {
    u8 button_0d_enabled;
};

#define USE_THREAD_HANDLER

#ifdef USE_THREAD_HANDLER
struct task_struct *thread = NULL;
static int tpd_flag = 0;
static DECLARE_WAIT_QUEUE_HEAD(waiter);
#endif
struct point G_point[10];
int should_up_flag=0;


static int lcd_x = 0;
static int lcd_y = 0;

atomic_t suspend_flag=ATOMIC_INIT(0);
extern struct tpd_device *tpd;
static struct tpd_data *ts = NULL;
static struct tpd_debug *td = NULL;
static struct workqueue_struct *mtk_tpd_wq;
static u8 boot_mode;

struct delayed_work det_work;
struct workqueue_struct *det_workqueue;

/* Function extern */
void Config_Touch_GPIO(bool  work_or_stop);
static void tpd_eint_handler(void);
static int __devinit tpd_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tpd_detect(struct i2c_client *client,  struct i2c_board_info *info);
static int __devexit tpd_remove(struct i2c_client *client);

#ifdef USE_THREAD_HANDLER
static int tpd_work_func(void *unused);
#else
static void tpd_work_func(struct work_struct *work);
#endif

extern int tpd_i2c_read_data(struct i2c_client *client, unsigned short addr, unsigned char *data, unsigned short length);
extern int tpd_i2c_write_data(struct i2c_client *client, unsigned short addr, unsigned char *data, unsigned short length);

static void tpd_down(int x, int y, int p,int id);
static void tpd_up(int x, int y,int id);

static int tpd_sw_power(struct i2c_client *client, int on);
static int tpd_clear_interrupt(struct i2c_client *client);
extern int synaptics_fw_updater_s3203(unsigned char *fw_data);
#if defined(TINNO_ANDROID_S9121)
//extern int synaptics_fw_version_updater();
#endif
//static u8 get_config_version(void);


static const struct i2c_device_id tpd_id[] = {{"S3203",0},{}};
static struct i2c_board_info __initdata i2c_tpd={ I2C_BOARD_INFO("S3203", 0x39)};

struct mutex tp_mutex;

static struct i2c_driver tpd_i2c_driver = {
    .driver = {
        .name = "S3203",
    },
    .probe = tpd_probe,
    .remove = __devexit_p(tpd_remove),
    .id_table = tpd_id,
    .detect = tpd_detect,
};


#ifdef CONFIG_HAS_EARLYSUSPEND
static ssize_t synaptics_rmi4_full_pm_cycle_show(struct device *dev,
        struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_full_pm_cycle_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count);

#endif

#if PROXIMITY
static ssize_t synaptics_rmi4_f51_enables_show(struct device *dev,
        struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_f51_enables_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count);
#endif

static ssize_t synaptics_rmi4_f01_reset_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count);

static ssize_t synaptics_rmi4_f01_productinfo_show(struct device *dev,
        struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_f01_flashprog_show(struct device *dev,
        struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_0dbutton_show(struct device *dev,
        struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_0dbutton_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count);


struct kobject *properties_kobj_synap;
struct kobject *properties_kobj_driver;


static struct device_attribute attrs[] = {
#ifdef CONFIG_HAS_EARLYSUSPEND
    __ATTR(full_pm_cycle, 0660,
            synaptics_rmi4_full_pm_cycle_show,
            synaptics_rmi4_full_pm_cycle_store),
#endif
#if PROXIMITY
    __ATTR(proximity_enables, 0660,
            synaptics_rmi4_f51_enables_show,
            synaptics_rmi4_f51_enables_store),
#endif
    __ATTR(reset, 0660,
            synaptics_rmi4_show_error,
            synaptics_rmi4_f01_reset_store),
    __ATTR(productinfo, 0660,
            synaptics_rmi4_f01_productinfo_show,
            synaptics_rmi4_store_error),
    __ATTR(flashprog, 0660,
            synaptics_rmi4_f01_flashprog_show,
            synaptics_rmi4_store_error),
    __ATTR(0dbutton, 0660,
            synaptics_rmi4_0dbutton_show,
            synaptics_rmi4_0dbutton_store),
};

static bool exp_fn_inited;
static struct mutex exp_fn_list_mutex;
static struct list_head exp_fn_list;

#if PROXIMITY
static struct synaptics_rmi4_f51_handle *f51;
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static ssize_t synaptics_rmi4_full_pm_cycle_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

    return snprintf(buf, PAGE_SIZE, "%u\n",
            rmi4_data->full_pm_cycle);
}

static ssize_t synaptics_rmi4_full_pm_cycle_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned int input;
    struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

    if (sscanf(buf, "%u", &input) != 1)
        return -EINVAL;

    rmi4_data->full_pm_cycle = input > 0 ? 1 : 0;

    return count;
}
#endif

#if PROXIMITY
static ssize_t synaptics_rmi4_f51_enables_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    int retval;
    unsigned char proximity_enables;

    if (!f51)
            return -ENODEV;

    retval = synaptics_rmi4_i2c_read(f51->rmi4_data,
            f51->proximity_enables_addr,
            &proximity_enables,
            sizeof(proximity_enables));
    if (retval <= 0) {
        dev_err(dev,
                "%s: Failed to read proximity enables, error = %d\n",
                __func__, retval);
        return retval;
    }

    return snprintf(buf, PAGE_SIZE, "0x%02x\n",
            proximity_enables);
}

static ssize_t synaptics_rmi4_f51_enables_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    int retval;
    unsigned int input;
    unsigned char proximity_enables;

    if (!f51)
            return -ENODEV;

    if (sscanf(buf, "%x", &input) != 1)
        return -EINVAL;

    proximity_enables = input;

    retval = synaptics_rmi4_i2c_write(f51->rmi4_data,
            f51->proximity_enables_addr,
            &proximity_enables,
            sizeof(proximity_enables));
    if (retval < 0) {
        dev_err(dev,
                "%s: Failed to write proximity enables, error = %d\n",
                __func__, retval);
        return retval;
    }

    return count;
}
#endif

static ssize_t synaptics_rmi4_f01_reset_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
/*    int retval;
    unsigned int reset;
    struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

    if (sscanf(buf, "%u", &reset) != 1)
        return -EINVAL;

    if (reset != 1)
        return -EINVAL;

    retval = synaptics_rmi4_reset_device(rmi4_data);
    if (retval < 0) {
        dev_err(dev,
                "%s: Failed to issue reset command, error = %d\n",
                __func__, retval);
        return retval;
    }

    return count;*/
    return 0;
}

static ssize_t synaptics_rmi4_f01_productinfo_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    /*struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

    return snprintf(buf, PAGE_SIZE, "0x%02x 0x%02x\n",
            (rmi4_data->rmi4_mod_info.product_info[0]),
            (rmi4_data->rmi4_mod_info.product_info[1]));*/

    return 0;
}

static ssize_t synaptics_rmi4_f01_flashprog_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    /*int retval;
    struct synaptics_rmi4_f01_device_status device_status;
    struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

    retval = synaptics_rmi4_i2c_read(rmi4_data,
            rmi4_data->f01_data_base_addr,
            device_status.data,
            sizeof(device_status.data));
    if (retval < 0) {
        dev_err(dev,
                "%s: Failed to read device status, error = %d\n",
                __func__, retval);
        return retval;
    }

    return snprintf(buf, PAGE_SIZE, "%u\n",
            device_status.flash_prog);*/
    return 0;
}

static ssize_t synaptics_rmi4_0dbutton_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
/*    struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);
    return snprintf(buf, PAGE_SIZE, "%d\n",
            rmi4_data->button_0d_enabled);*/
    return 0;
}

static ssize_t synaptics_rmi4_0dbutton_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
#if 0
    int retval;
    unsigned int input;
    unsigned char ii;
    unsigned char intr_enable;
    struct synaptics_rmi4_fn *fhandler;
    struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);
    struct synaptics_rmi4_device_info *rmi;

    rmi = &(rmi4_data->rmi4_mod_info);

    if (sscanf(buf, "%u", &input) != 1)
        return -EINVAL;

    input = input > 0 ? 1 : 0;

    if (rmi4_data->button_0d_enabled == input)
        return count;

    list_for_each_entry(fhandler, &rmi->support_fn_list, link) {
        if (fhandler->fn_number == SYNAPTICS_RMI4_F1A) {
            ii = fhandler->intr_reg_num;

            retval = synaptics_rmi4_i2c_read(rmi4_data,
                    rmi4_data->f01_ctrl_base_addr + 1 + ii,
                    &intr_enable,
                    sizeof(intr_enable));
            if (retval < 0)
                return retval;

            if (input == 1)
                intr_enable |= fhandler->intr_mask;
            else
                intr_enable &= ~fhandler->intr_mask;

            retval = synaptics_rmi4_i2c_write(rmi4_data,
                    rmi4_data->f01_ctrl_base_addr + 1 + ii,
                    &intr_enable,
                    sizeof(intr_enable));
            if (retval < 0)
                return retval;
        }
    }

    rmi4_data->button_0d_enabled = input;
#endif
    return 0;
}


static int tpd_set_page(struct i2c_client *client,unsigned int address)
{
    int retval = 0;
    unsigned char retry;
    unsigned char buf[PAGE_SELECT_LEN];
    unsigned char page;

    page = ((address >> 8) & MASK_8BIT);
    if (page != ts->current_page) {
        buf[0] = MASK_8BIT;
        buf[1] = page;
        for (retry = 0; retry < SYN_I2C_RETRY_TIMES; retry++) {
            retval = i2c_master_send(client, buf, PAGE_SELECT_LEN);

            if (retval != PAGE_SELECT_LEN) {
                dev_err(&client->dev,
                        "%s: I2C retry %d\n",
                        __func__, retry + 1);
                msleep(20);
            } else {
                ts->current_page = page;
                break;
            }
        }
    } else {
        retval = PAGE_SELECT_LEN;
    }

    return retval;
}


#define DMA_CONTROL
static u8 *gpwDMABuf_va = NULL;
static u32 gpwDMABuf_pa = NULL;

static u8 *gprDMABuf_va = NULL;
static u32 gprDMABuf_pa = NULL;

#define BUFFER_SIZE 252
struct st_i2c_msgs
{
    struct i2c_msg *msg;
    int count;
} i2c_msgs;


#ifdef DMA_CONTROL
int tpd_i2c_read_data_dma(struct i2c_client *client,
        unsigned short addr, unsigned char *data, unsigned short length)
{
    int ii;
    int retval;
    unsigned char retry;
    unsigned char buf;
    unsigned char *buf_va = NULL;
    int message_count = ((length - 1) / BUFFER_SIZE) + 2;
    int message_rest_count = length % BUFFER_SIZE;
    int data_len;

    if (i2c_msgs.msg == NULL || i2c_msgs.count < message_count)
    {
        if (i2c_msgs.msg != NULL)
            kfree(i2c_msgs.msg);
        i2c_msgs.msg = (struct i2c_msg*)kcalloc(message_count, sizeof(struct i2c_msg), GFP_KERNEL);
        i2c_msgs.count = message_count;
    }

    mutex_lock(&(ts->io_ctrl_mutex));

     gprDMABuf_va = (u8 *)dma_alloc_coherent(NULL, 4096, &gprDMABuf_pa, GFP_KERNEL);
    if (!gprDMABuf_va)
        dev_err(&client->dev,
                "%s: [Error] Allocate DMA I2C buffer failed!\n",
                __func__);

    buf_va = gprDMABuf_va;

    i2c_msgs.msg[0].addr = client->addr;
    i2c_msgs.msg[0].flags = 0;
    i2c_msgs.msg[0].len = 1;
    i2c_msgs.msg[0].buf = &buf;

    if (!message_rest_count)
        message_rest_count = BUFFER_SIZE;
    for (ii = 0; ii < message_count - 1; ii++) {
        if (ii == (message_count - 2)) {
            data_len = message_rest_count;
        } else {
            data_len = BUFFER_SIZE;
        }
        i2c_msgs.msg[ii + 1].addr = client->addr;
        i2c_msgs.msg[ii + 1].flags = I2C_M_RD;
        i2c_msgs.msg[ii + 1].len = data_len;
        i2c_msgs.msg[ii + 1].buf = gprDMABuf_pa + BUFFER_SIZE * ii;
        i2c_msgs.msg[ii + 1].ext_flag = (I2C_ENEXT_FLAG | I2C_DMA_FLAG);
        i2c_msgs.msg[ii + 1].timing = 400;
    }

    buf = addr & MASK_8BIT;

    retval = tpd_set_page(client, addr);
    if (retval != PAGE_SELECT_LEN)
        goto exit;


    for (retry = 0; retry < SYN_I2C_RETRY_TIMES; retry++) {
        if (i2c_transfer(client->adapter, i2c_msgs.msg, message_count) == message_count) {
            retval = length;
            break;
        }
        dev_err(&client->dev,
                "%s: I2C retry %d\n",
                __func__, retry + 1);
        msleep(20);
    }

    if (retry == SYN_I2C_RETRY_TIMES) {
        dev_err(&client->dev,
                "%s: I2C read over retry limit\n",
                __func__);
        retval = -EIO;
    }

    memcpy(data, buf_va, length);

exit:

    if(gprDMABuf_va) {
        dma_free_coherent(NULL, 4096, gprDMABuf_va, gprDMABuf_pa);
        gprDMABuf_va = NULL;
        gprDMABuf_pa = NULL;
    }
    mutex_unlock(&(ts->io_ctrl_mutex));
    return retval;
}
#endif
int tpd_i2c_read_data_fifo(struct i2c_client *client,
                unsigned short addr, unsigned char *data, unsigned short length)
{
        int retval=0;
        u8 retry = 0;
        u8 *pData = data;
        int tmp_addr = addr;
        int left_len = length;

        mutex_lock(&(ts->io_ctrl_mutex));

        retval = tpd_set_page(client, addr);
        if (retval != PAGE_SELECT_LEN)
                goto exit;

        u16 old_flag = client->ext_flag;
        client->addr = client->addr & I2C_MASK_FLAG ;
        client->ext_flag =client->ext_flag | I2C_WR_FLAG | I2C_RS_FLAG | I2C_ENEXT_FLAG;

        while (left_len > 0) {
                pData[0] = tmp_addr;

                for (retry = 0; retry < SYN_I2C_RETRY_TIMES; retry++) {
                        if (left_len > 8) {
                                retval = i2c_master_send(client, pData, (8 << 8 | 1));
                        } else {
                                retval = i2c_master_send(client, pData, (left_len << 8 | 1));
                        }

                        if (retval > 0) {
                                break;
                        } else {
                                dev_err(&client->dev, "%s: I2C retry %d\n", __func__, retry + 1);
                                msleep(20);
                        }
                }

                left_len -= 8;
                pData += 8;
                tmp_addr += 8;
        }

        client->ext_flag = old_flag;

exit:
        mutex_unlock(&(ts->io_ctrl_mutex));

        return retval;
}

int tpd_i2c_read_data(struct i2c_client *client,
                unsigned short addr, unsigned char *data, unsigned short length)
{
#ifdef DMA_CONTROL
    if (length >= 8)
        return tpd_i2c_read_data_dma(client, addr, data, length);
    else
#endif
        return tpd_i2c_read_data_fifo(client, addr, data, length);
}
EXPORT_SYMBOL(tpd_i2c_read_data);

#ifdef DMA_CONTROL
int tpd_i2c_write_data_dma(struct i2c_client *client,
        unsigned short addr, unsigned char *data, unsigned short length)
{
    int retval;
    unsigned char retry;
    unsigned char *buf_va = NULL;
    mutex_lock(&(ts->io_ctrl_mutex));

    gpwDMABuf_va = (u8 *)dma_alloc_coherent(NULL, 1024, &gpwDMABuf_pa, GFP_KERNEL);
    if(!gpwDMABuf_va)
        dev_err(&client->dev,
                "%s: [Error] Allocate DMA I2C buffer failed!\n",
                __func__);

    buf_va = gpwDMABuf_va;

    struct i2c_msg msg[] = {
        {
            .addr = client->addr,
            .flags = 0,
            .len = length + 1,
            .buf = gpwDMABuf_pa,
            .ext_flag = (I2C_ENEXT_FLAG | I2C_DMA_FLAG),
            .timing = 400,
        }
    };

    retval = tpd_set_page(client, addr);
    if (retval != PAGE_SELECT_LEN) {
        TPD_DMESG("tpd_set_page fail, retval = %d\n", retval);
        retval = -EIO;
        goto exit;
    }

    buf_va[0] = addr & MASK_8BIT;

    memcpy(&buf_va[1], &data[0], length);

    for (retry = 0; retry < SYN_I2C_RETRY_TIMES; retry++) {
        if (i2c_transfer(client->adapter, msg, 1) == 1) {
            retval = length;
            break;
        }
        TPD_DMESG("%s: I2C retry %d\n", __func__, retry + 1);
        msleep(20);
    }

    if (retry == SYN_I2C_RETRY_TIMES)
        retval = -EIO;

exit:

    if (gpwDMABuf_va) {
        dma_free_coherent(NULL, 1024, gpwDMABuf_va, gpwDMABuf_pa);
        gpwDMABuf_va = NULL;
        gpwDMABuf_pa = NULL;
    }
    mutex_unlock(&(ts->io_ctrl_mutex));

    return retval;
}
#endif
int tpd_i2c_write_data_fifo(struct i2c_client *client,
                unsigned short addr, unsigned char *data, unsigned short length)
{
    int retval=0;
    u8 retry = 0;
    u8 *pData = data;
    u8 buf[5] = {0};
    int tmp_addr = addr;
    int left_len = length;

    mutex_lock(&(ts->io_ctrl_mutex));

    retval = tpd_set_page(client, addr);
    if (retval != PAGE_SELECT_LEN) {
        TPD_DMESG("tpd_set_page fail, retval = %d\n", retval);
        retval = -EIO;
        goto exit;
    }

    while (left_len > 0) {
        buf[0] = tmp_addr;
        for (retry = 0; retry < SYN_I2C_RETRY_TIMES; retry++) {
            if (left_len > 4) {
                memcpy(buf+1, pData, 4);
                retval = i2c_master_send(client, buf, 5);
            } else {
                memcpy(buf+1, pData, left_len);
                retval = i2c_master_send(client, buf, left_len + 1);
            }

            if (retval > 0) {
                break;
            } else {
                TPD_DMESG("%s: I2C retry %d\n", __func__, retry + 1);
                msleep(20);
            }
        }

        left_len -= 4;
        pData += 4;
        tmp_addr += 4;
        }

exit:
        mutex_unlock(&(ts->io_ctrl_mutex));

        return retval;
}

int tpd_i2c_write_data(struct i2c_client *client,
                unsigned short addr, unsigned char *data, unsigned short length)
{
#ifdef DMA_CONTROL
    if (length >= 8)
        return tpd_i2c_write_data_dma(client, addr, data, length);
    else
#endif
        return tpd_i2c_write_data_fifo(client, addr, data, length);
}
EXPORT_SYMBOL(tpd_i2c_write_data);


#if PROXIMITY
static int synaptics_rmi4_f51_report(struct synaptics_rmi4_data *rmi4_data,
        struct synaptics_rmi4_fn *fhandler)
{
    int retval;
    unsigned char touch_count = 0; /* number of touch points */
    unsigned short data_base_addr;
    int x;
    int y;
    int z;
    struct synaptics_rmi4_f51_data *data_reg;

    data_base_addr = fhandler->full_addr.data_base;
    data_reg = (struct synaptics_rmi4_f51_data *)fhandler->data;

    retval =tpd_i2c_read(rmi4_data,
            data_base_addr,
            data_reg->data,
            sizeof(data_reg->data));
    if (retval < 0)
        return 0;

    if (data_reg->data[0] == 0x00)
        return 0;

/**/
#if sdfsdfadf
    if (data_reg->finger_hover_det) {
        if (data_reg->hover_finger_z > 0) {
            x = (data_reg->hover_finger_x_4__11 << 4) |
                    (data_reg->hover_finger_xy_0__3 & 0x0f);
            y = (data_reg->hover_finger_y_4__11 << 4) |
                    (data_reg->hover_finger_xy_0__3 >> 4);
            z = HOVER_Z_MAX - data_reg->hover_finger_z;

            dev_dbg(&rmi4_data->i2c_client->dev,
                    "%s: Hover finger:\n"
                    "x = %d\n"
                    "y = %d\n"
                    "z = %d\n",
                    __func__, x, y, z);

            input_report_abs(tpd->dev,
                    ABS_MT_POSITION_X, x);
            input_report_abs(tpd->dev,
                    ABS_MT_POSITION_Y, y);
#ifdef INPUT_MULTITOUCH
            input_report_abs(tpd->dev,
                    ABS_MT_DISTANCE, z);
#endif
            input_mt_sync(tpd->dev);

            touch_count++;
        }
    }

    if (data_reg->air_swipe_det) {
        dev_dbg(&rmi4_data->i2c_client->dev,
                "%s: Swipe direction 0 = %d\n",
                __func__, data_reg->air_swipe_dir_0);
        dev_dbg(&rmi4_data->i2c_client->dev,
                "%s: Swipe direction 1 = %d\n",
                __func__, data_reg->air_swipe_dir_1);
    }

    if (data_reg->large_obj_det) {
        dev_dbg(&rmi4_data->i2c_client->dev,
                "%s: Large object activity = %d\n",
                __func__, data_reg->large_obj_act);
    }

    if (data_reg->hover_pinch_det) {
        dev_dbg(&rmi4_data->i2c_client->dev,
                "%s: Hover pinch direction = %d\n",
                __func__, data_reg->hover_pinch_dir);
    }
#endif

    if (!touch_count)
        input_mt_sync(tpd->dev);

    input_sync(tpd->dev);

    return touch_count;
}
#endif

#if PROXIMITY
static int synaptics_rmi4_f51_init(struct synaptics_rmi4_data *rmi4_data,
        struct synaptics_rmi4_fn *fhandler,
        struct synaptics_rmi4_fn_desc *fd,
        unsigned int intr_count)
{
    int retval;
    unsigned char ii;
    unsigned short intr_offset;
    unsigned char proximity_enable_mask = PROXIMITY_ENABLE;
    struct synaptics_rmi4_f51_query query_register;
    struct synaptics_rmi4_f51_data *data_register;

    fhandler->fn_number = fd->fn_number;
    fhandler->num_of_data_sources = fd->intr_src_count;

    fhandler->intr_reg_num = (intr_count + 7) / 8;
    if (fhandler->intr_reg_num != 0)
        fhandler->intr_reg_num -= 1;

    /* Set an enable bit for each data source */
    intr_offset = intr_count % 8;
    fhandler->intr_mask = 0;
    for (ii = intr_offset;
            ii < ((fd->intr_src_count & MASK_3BIT) +
            intr_offset);
            ii++)
        fhandler->intr_mask |= 1 << ii;

    retval = synaptics_rmi4_i2c_read(rmi4_data,
            fhandler->full_addr.query_base,
            query_register.data,
            sizeof(query_register.data));
    if (retval <= 0)
        return retval;

    fhandler->data_size = sizeof(data_register->data);
    data_register = kmalloc(fhandler->data_size, GFP_KERNEL);
    fhandler->data = (void *)data_register;

    retval = synaptics_rmi4_i2c_write(rmi4_data,
            fhandler->full_addr.ctrl_base +
            query_register.control_register_count - 1,
            &proximity_enable_mask,
            sizeof(proximity_enable_mask));
    if (retval <= 0)
        return retval;

    f51 = kmalloc(sizeof(*f51), GFP_KERNEL);
    f51->rmi4_data = rmi4_data;
    f51->proximity_enables_addr = fhandler->full_addr.ctrl_base +
            query_register.control_register_count - 1;

    return 1;
}

int synaptics_rmi4_proximity_enables(unsigned char enables)
{
    int retval;
    unsigned char proximity_enables = enables;

    if (!f51)
        return -ENODEV;

    retval = synaptics_rmi4_i2c_write(f51->rmi4_data,
            f51->proximity_enables_addr,
            &proximity_enables,
            sizeof(proximity_enables));
    if (retval < 0)
        return retval;

    return 0;
}
EXPORT_SYMBOL(synaptics_rmi4_proximity_enables);
#endif

static int tpd_rmi4_read_pdt(struct tpd_data *ts)
{
    int retval;
    unsigned char ii;
    unsigned char offset=0;
    unsigned char page_number;
    unsigned char intr_count = 0;
    unsigned char data_sources = 0;
    unsigned char f01_query[F01_STD_QUERY_LEN];
    unsigned char f11_query[F11_STD_QUERY_LEN];
    U32 f11_max_xy;
    u8  point_length;
    unsigned short pdt_entry_addr;
    unsigned short intr_addr;
    static u8 intsrc = 1;
    //struct synaptics_rmi4_f01_device_status status;
    struct synaptics_rmi4_fn_desc rmi_fd;

    /* Scan the page description tables of the pages to service */
    for (page_number = 0; page_number < PAGES_TO_SERVICE; page_number++) {
        for (pdt_entry_addr = PDT_START; pdt_entry_addr > PDT_END;
                pdt_entry_addr -= PDT_ENTRY_SIZE) {
            pdt_entry_addr |= (page_number << 8);

            retval = tpd_i2c_read_data(ts->client,
                    pdt_entry_addr,
                    (unsigned char *)&rmi_fd,
                    sizeof(rmi_fd));
            if (retval <= 0)
                return retval;

            if (rmi_fd.fn_number == 0) {
                dev_dbg(&ts->client->dev,
                        "%s: Reached end of PDT\n",
                        __func__);
                break;
            }

            dev_dbg(&ts->client->dev,
                    "%s: F%02x found (page %d)\n",
                    __func__, rmi_fd.fn_number,
                    page_number);

            switch (rmi_fd.fn_number) {
            case SYNAPTICS_RMI4_F01:

                ts->f01.query_base = rmi_fd.query_base_addr;
                ts->f01.ctrl_base = rmi_fd.ctrl_base_addr;
                ts->f01.cmd_base = rmi_fd.cmd_base_addr;
                ts->f01.data_base =page_number | rmi_fd.data_base_addr;
                ts->f01.intSrc = intsrc++;
                ts->f01.functionNumber = rmi_fd.fn_number;

                break;

            case SYNAPTICS_RMI4_F11:
                if (rmi_fd.intr_src_count == 0)
                    break;

                ts->f11.query_base = rmi_fd.query_base_addr;
                ts->f11.ctrl_base = rmi_fd.ctrl_base_addr;
                ts->f11.cmd_base = rmi_fd.cmd_base_addr;
                ts->f11.data_base = page_number |rmi_fd.data_base_addr;
                ts->f11.intSrc = intsrc++;
                ts->f11.functionNumber = rmi_fd.fn_number;

                ts->fn11_mask = 0;
                offset = intr_count%8;
                for(ii=offset;ii<(rmi_fd.intr_src_count+offset);ii++)
                    ts->fn11_mask |= 1 <<ii;

                retval = tpd_i2c_read_data(ts->client,ts->f11.query_base,f11_query,sizeof(f11_query));
                if (retval <= 0)
                    return retval;
                TPD_DMESG("f11 query base=%d\n",ts->f11.query_base);
                /* Maximum number of fingers supported */
                if ((f11_query[1] & MASK_3BIT) <= 4){
                    ts->points_supported = (f11_query[1] & MASK_3BIT) + 1;
                    TPD_DMESG("points_supported=%d\n",ts->points_supported);
                    }
                else if ((f11_query[1] & MASK_3BIT) == 5){
                    ts->points_supported = 10;
                    TPD_DMESG("points_supported=%d\n",ts->points_supported);
                    }
                retval = tpd_i2c_read_data(ts->client,ts->f11.ctrl_base+6, (char *)(&f11_max_xy), sizeof(f11_max_xy));
                if (retval <= 0)
                    return retval;

                /* Maximum x and y */
                ts->f11_max_x = f11_max_xy & 0xFFF;
                ts->f11_max_y = (f11_max_xy >> 16) & 0xFFF;


                ts->pre_points = kzalloc(ts->points_supported * sizeof(struct point), GFP_KERNEL);
                if (ts->pre_points == NULL) {
                    TPD_DMESG("Error zalloc failed!\n");
                    retval = -ENOMEM;
                    return retval;
                }

                ts->cur_points = kzalloc(ts->points_supported * sizeof(struct point), GFP_KERNEL);
                if (ts->cur_points == NULL) {
                    TPD_DMESG("Error zalloc failed!\n");
                    retval = -ENOMEM;
                    return retval;
                }

                ts->data_length = 3 + (2 * ((f11_query[5] & MASK_2BIT) == 0 ? 1 : 0));
                break;

            case SYNAPTICS_RMI4_F12:
                /*if (rmi_fd.intr_src_count == 0)
                    break;

                retval = synaptics_rmi4_alloc_fh(&fhandler,
                        &rmi_fd, page_number);
                if (retval < 0) {
                    dev_err(&rmi4_data->i2c_client->dev,
                            "%s: Failed to alloc for F%d\n",
                            __func__,
                            rmi_fd.fn_number);
                    return retval;
                }

                retval = synaptics_rmi4_f12_init(rmi4_data,
                        fhandler, &rmi_fd, intr_count);
                if (retval < 0)
                    return retval;*/
                break;
            case SYNAPTICS_RMI4_F1A:
                if (rmi_fd.intr_src_count == 0)
                    break;

                ts->f1a.query_base = rmi_fd.query_base_addr;
                ts->f1a.ctrl_base = rmi_fd.ctrl_base_addr;
                ts->f1a.cmd_base = rmi_fd.cmd_base_addr;
                //ts->f1a.data_base = page_number | rmi_fd.data_base_addr;
                ts->f1a.data_base =page_number<<8|rmi_fd.data_base_addr;
                ts->f01.intSrc = intsrc++;
                ts->f01.functionNumber = rmi_fd.fn_number;

                td->button_0d_enabled = 1;

                ts->fn1a_mask = 0;
                offset = intr_count%8;
                for(ii=offset;ii<(rmi_fd.intr_src_count+offset);ii++)
                    ts->fn1a_mask |= 1 <<ii;

                break;

#if PROXIMITY
            case SYNAPTICS_RMI4_F51:
                if (rmi_fd.intr_src_count == 0)
                    break;

                /*retval = synaptics_rmi4_alloc_fh(&fhandler,
                        &rmi_fd, page_number);
                if (retval < 0) {
                    dev_err(&rmi4_data->i2c_client->dev,
                            "%s: Failed to alloc for F%d\n",
                            __func__,
                            rmi_fd.fn_number);
                    return retval;
                }

                retval = synaptics_rmi4_f51_init(rmi4_data,
                        fhandler, &rmi_fd, intr_count);
                if (retval < 0)
                    return retval;*/
                break;
#endif
            }
            if(rmi_fd.intr_src_count&0x03){
                intr_count += rmi_fd.intr_src_count&0x03;
            }

        }
    }

#if 0
flash_prog_mode:
    rmi4_data->num_of_intr_regs = (intr_count + 7) / 8;
    dev_dbg(&rmi4_data->i2c_client->dev,
            "%s: Number of interrupt registers = %d\n",
            __func__, rmi4_data->num_of_intr_regs);

    retval = synaptics_rmi4_i2c_read(rmi4_data,
            rmi4_data->f01_query_base_addr,
            f01_query,
            sizeof(f01_query));
    if (retval < 0)
        return retval;

    /* RMI Version 4.0 currently supported */
    rmi->version_major = 4;
    rmi->version_minor = 0;

    rmi->manufacturer_id = f01_query[0];
    rmi->product_props = f01_query[1];
    rmi->product_info[0] = f01_query[2] & MASK_7BIT;
    rmi->product_info[1] = f01_query[3] & MASK_7BIT;
    rmi->date_code[0] = f01_query[4] & MASK_5BIT;
    rmi->date_code[1] = f01_query[5] & MASK_4BIT;
    rmi->date_code[2] = f01_query[6] & MASK_5BIT;
    rmi->tester_id = ((f01_query[7] & MASK_7BIT) << 8) |
            (f01_query[8] & MASK_7BIT);
    rmi->serial_number = ((f01_query[9] & MASK_7BIT) << 8) |
            (f01_query[10] & MASK_7BIT);
    memcpy(rmi->product_id_string, &f01_query[11], 10);

    if (rmi->manufacturer_id != 1) {
        dev_err(&rmi4_data->i2c_client->dev,
                "%s: Non-Synaptics device found, manufacturer ID = %d\n",
                __func__, rmi->manufacturer_id);
    }

    memset(rmi4_data->intr_mask, 0x00, sizeof(rmi4_data->intr_mask));

    /*
     * Map out the interrupt bit masks for the interrupt sources
     * from the registered function handlers.
     */
    list_for_each_entry(fhandler, &rmi->support_fn_list, link)
        data_sources += fhandler->num_of_data_sources;
    if (data_sources) {
        list_for_each_entry(fhandler, &rmi->support_fn_list, link) {
            if (fhandler->num_of_data_sources) {
                rmi4_data->intr_mask[fhandler->intr_reg_num] |=
                        fhandler->intr_mask;
            }
        }
    }

    /* Enable the interrupt sources */
    for (ii = 0; ii < rmi4_data->num_of_intr_regs; ii++) {
        if (rmi4_data->intr_mask[ii] != 0x00) {
            dev_dbg(&rmi4_data->i2c_client->dev,
                    "%s: Interrupt enable mask %d = 0x%02x\n",
                    __func__, ii, rmi4_data->intr_mask[ii]);
            intr_addr = rmi4_data->f01_ctrl_base_addr + 1 + ii;
            retval = synaptics_rmi4_i2c_write(rmi4_data,
                    intr_addr,
                    &(rmi4_data->intr_mask[ii]),
                    sizeof(rmi4_data->intr_mask[ii]));
            if (retval < 0)
                return retval;
        }
    }
#endif

    return 1;
}


/**
* synaptics_rmi4_detection_work()
*
* Called by the kernel at the scheduled time.
*
* This function is a self-rearming work thread that checks for the
* insertion and removal of other expansion Function modules such as
* rmi_dev and calls their initialization and removal callback functions
* accordingly.
*/
static void synaptics_rmi4_detection_work(struct work_struct *work)
{
    struct synaptics_rmi4_exp_fn *exp_fhandler, *next_list_entry;

    //queue_delayed_work(det_workqueue,&det_work,msecs_to_jiffies(EXP_FN_DET_INTERVAL));

    mutex_lock(&exp_fn_list_mutex);
    if (!list_empty(&exp_fn_list)) {
        list_for_each_entry_safe(exp_fhandler,
                next_list_entry,
                &exp_fn_list,
                link) {
            if ((exp_fhandler->func_init != NULL) &&
                    (exp_fhandler->inserted == false)) {
                exp_fhandler->func_init(ts->client);
                exp_fhandler->inserted = true;
            } else if ((exp_fhandler->func_init == NULL) &&
                    (exp_fhandler->inserted == true)) {
                exp_fhandler->func_remove(ts->client);
                list_del(&exp_fhandler->link);
                kfree(exp_fhandler);
            }
        }
    }
    mutex_unlock(&exp_fn_list_mutex);

    return;
}

/**
* synaptics_rmi4_new_function_s3203()
*
* Called by other expansion Function modules in their module init and
* module exit functions.
*
* This function is used by other expansion Function modules such as
* rmi_dev to register themselves with the driver by providing their
* initialization and removal callback function pointers so that they
* can be inserted or removed dynamically at module init and exit times,
* respectively.
*/
void synaptics_rmi4_new_function_s3203(enum exp_fn fn_type, bool insert,
        int (*func_init)(struct i2c_client *client),
        void (*func_remove)(struct i2c_client *client),
        void (*func_attn)(struct i2c_client *client,
        unsigned char intr_mask))
{
    struct synaptics_rmi4_exp_fn *exp_fhandler;

    if (!exp_fn_inited) {
        mutex_init(&exp_fn_list_mutex);
        INIT_LIST_HEAD(&exp_fn_list);
        exp_fn_inited = 1;
    }

    mutex_lock(&exp_fn_list_mutex);
    if (insert) {
        exp_fhandler = kzalloc(sizeof(*exp_fhandler), GFP_KERNEL);
        if (!exp_fhandler) {
            pr_err("%s: Failed to alloc mem for expansion function\n",
                    __func__);
            goto exit;
        }
        exp_fhandler->fn_type = fn_type;
        exp_fhandler->func_init = func_init;
        exp_fhandler->func_attn = func_attn;
        exp_fhandler->func_remove = func_remove;
        exp_fhandler->inserted = false;
        list_add_tail(&exp_fhandler->link, &exp_fn_list);
    } else {
        list_for_each_entry(exp_fhandler, &exp_fn_list, link) {
            if (exp_fhandler->func_init == func_init) {
                exp_fhandler->inserted = false;
                exp_fhandler->func_init = NULL;
                exp_fhandler->func_attn = NULL;
                goto exit;
            }
        }
    }

exit:
    mutex_unlock(&exp_fn_list_mutex);

    return;
}
EXPORT_SYMBOL(synaptics_rmi4_new_function_s3203);

#ifdef MULTI_TOUCH_PROTOCOL_TYPE_B
static void tpd_down(int x, int y, int p ,int id)
{
     input_mt_slot(tpd->dev, id);
    input_mt_report_slot_state(tpd->dev, MT_TOOL_FINGER, 1);
    input_report_abs(tpd->dev, ABS_MT_POSITION_X, x);
    input_report_abs(tpd->dev, ABS_MT_POSITION_Y, y);
    input_report_abs(tpd->dev, ABS_MT_PRESSURE, p);
    input_report_abs(tpd->dev, ABS_MT_WIDTH_MAJOR, p);
    input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, p);
    input_mt_report_pointer_emulation(tpd->dev, true);


    if(touch_ssb_data.use_tpd_button == 1){
        /*BEGIN PN: DTS2012051505359 ,modified by s00179437 , 2012-05-31*/
        if (FACTORY_BOOT == boot_mode || RECOVERY_BOOT == boot_mode)
        /*END PN: DTS2012051505359 ,modified by s00179437 , 2012-05-31*/
        {
            tpd_button(x, y, 1);
        }
    }
}
#else
static void tpd_down(int x, int y, int p,int id)
{
    input_report_abs(tpd->dev, ABS_MT_PRESSURE, p);
    input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 1);
    input_report_abs(tpd->dev, ABS_MT_TRACKING_ID, id);
    input_report_key(tpd->dev, BTN_TOUCH, 1);
    input_report_abs(tpd->dev, ABS_MT_POSITION_X, x);
    input_report_abs(tpd->dev, ABS_MT_POSITION_Y, y);
    input_mt_sync(tpd->dev);

    if(touch_ssb_data.use_tpd_button == 1){
        /*BEGIN PN: DTS2012051505359 ,modified by s00179437 , 2012-05-31*/
        if (FACTORY_BOOT == boot_mode || RECOVERY_BOOT == boot_mode)
        /*END PN: DTS2012051505359 ,modified by s00179437 , 2012-05-31*/
        {
            tpd_button(x, y, 1);
        }
    }

    /* < DTS2012042803609 gkf61766 20120428 begin */
    TPD_DMESG("=================>D---[%4d %4d %4d]\n", x, y, p);
    /* DTS2012042803609 gkf61766 20120428 end > */
    TPD_DOWN_DEBUG_TRACK(x,y);
}

#endif

#ifdef MULTI_TOUCH_PROTOCOL_TYPE_B
static void tpd_up(int x, int y,int id)
{

        input_mt_slot(tpd->dev, id);
        input_mt_report_slot_state(tpd->dev, MT_TOOL_FINGER, 0);
        input_mt_report_pointer_emulation(tpd->dev, true);


    if(touch_ssb_data.use_tpd_button == 1){
        /*BEGIN PN: DTS2012051505359 ,modified by s00179437 , 2012-05-31*/
        if (FACTORY_BOOT == boot_mode || RECOVERY_BOOT == boot_mode)
        /*END PN: DTS2012051505359 ,modified by s00179437 , 2012-05-31*/
        {
            tpd_button(x, y, 1);
        }
    }

}
#else
static void tpd_up(int x, int y ,int id)
{

        input_report_key(tpd->dev, BTN_TOUCH, 0);
        input_mt_sync(tpd->dev);

    if(touch_ssb_data.use_tpd_button == 1){
        /*BEGIN PN: DTS2012051505359 ,modified by s00179437 , 2012-05-31*/
        if (FACTORY_BOOT == boot_mode || RECOVERY_BOOT == boot_mode)
        /*END PN: DTS2012051505359 ,modified by s00179437 , 2012-05-31*/
        {
            tpd_button(x, y, 0);
        }
    }

    /* < DTS2012042803609 gkf61766 20120428 begin */
    TPD_DMESG("==================>U---[%4d %4d %4d]\n", x, y, 0);
    /* DTS2012042803609 gkf61766 20120428 end > */
    TPD_UP_DEBUG_TRACK(x,y);
}
#endif



static int tpd_process()
{    u16 temp=0;
    u8 i = 0 ;
    u8 status = 0;
    int retval = 0;
    u8 finger_status = 0;
    u8 finger_status_reg[3];
    u8 data[F11_STD_DATA_LEN];
    u8 num_of_finger_status_regs = 0;
    struct point *ppt = NULL;
    int point_count;
    int ret=0;
    TPD_DMESG("tpd_process read data to clear interrupt!!\n");

    //clear interrupt bit
    retval = tpd_i2c_read_data(ts->client, ts->f01.data_base + 1, &status, 1);
    if (retval <= 0)
    {
        TPD_DMESG("tpd_i2c_read_data retval error\n");
        ret=0;
        goto exit;
    }

    //TPD_DMESG("tpd_process status =%d,ts->fn1a_mask =%d, ts->fn11_mask =%d ts->f1a.data_base =%d \n",status,ts->fn1a_mask,  ts->fn11_mask,ts->f1a.data_base);
       if(status==0)
       {
           ret=0;
        goto exit;
    }
    #ifdef HAVE_TOUCH_KEY
    if (status & ts->fn1a_mask) { /* key */
        u8 button = 0;
        static u8 id = 0;
        retval = tpd_i2c_read_data(ts->client, ts->f1a.data_base, &button, 1);

        TPD_DMESG("======>hx read button!!!! 0x%x \n",button);
        if (button)
        {
            for (i = 0; i < MAX_KEY_NUM; i++)
            {
              if((button & (0x01 << i))==1){

                  if (FACTORY_BOOT == boot_mode || RECOVERY_BOOT == boot_mode)
                  {
                      input_report_key(tpd->dev, touch_key_array[i], button & (0x01 << i));
                  }else{
                      tpd_down(100,1320,50,SYNAPTICS_TOUCH_TRACK_IDS-1);
                  }

                  id = i;
              }
              if((button & (0x01 << i))==2){
              if(FACTORY_BOOT == boot_mode || RECOVERY_BOOT == boot_mode)
                  {
                      input_report_key(tpd->dev, touch_key_array[i], button & (0x01 << i));
                  }else{
                      tpd_down(250,1320,50,SYNAPTICS_TOUCH_TRACK_IDS-1);
                  }
                  id = i;
              }
              if((button & (0x01 << i))==4){
              if (FACTORY_BOOT == boot_mode || RECOVERY_BOOT == boot_mode)
              {
                input_report_key(tpd->dev, touch_key_array[i], button & (0x01 << i));
              }else{
                    tpd_down(400,1320,50,SYNAPTICS_TOUCH_TRACK_IDS-1);
              }
              id = i;
              }
            }
        }else {

             if (FACTORY_BOOT == boot_mode || RECOVERY_BOOT == boot_mode)
             {
                  input_report_key(tpd->dev, touch_key_array[id], 0);
             }else{
                 tpd_up(0,0,SYNAPTICS_TOUCH_TRACK_IDS-1);
             }
        }
        input_sync(tpd->dev);
        ret=1;
        goto exit;
    }
    #endif

    TPD_DMESG("status:%d!! ts->fn1a_mask=0x%x \n",status,ts->fn1a_mask);

    if (status & ts->fn11_mask) {
        tpd_i2c_read_data(ts->client, ts->f11.data_base, finger_status_reg, (ts->points_supported + 3) / 4);
        num_of_finger_status_regs = (ts->points_supported + 3) / 4;

        TPD_DMESG("finger regs num=:%d!!\n",num_of_finger_status_regs);

        for (i = 0; i < ts->points_supported; i++) {

            finger_status = finger_status_reg[i / 4];
            finger_status = (finger_status >> ((i % 4) * 2)) & 3;
            ppt = &ts->cur_points[i];
            ppt->status = finger_status;

            if (0x01==finger_status/* || 0x02 == finger_status*/) {


                temp=ts->f11.data_base + num_of_finger_status_regs + i * ts->data_length;

                TPD_DMESG("finger addr=:%d!!\n",temp);

                tpd_i2c_read_data(ts->client, ts->f11.data_base + num_of_finger_status_regs + i * ts->data_length,
                            data, ts->data_length);

                ppt->raw_x = ppt->x = (((u16)(data[0]) << 4) | (data[2] & 0x0F));
                ppt->raw_y = ppt->y = (((u16)(data[1]) << 4) | ((data[2] >> 4) & 0x0F));
                ppt->z = data[4];

                TPD_DMESG("========>[index:%d     x:%4d  y:%4d  status =%d]\n",(i+1),ppt->raw_x,ppt->raw_y,ppt->status);
#ifndef MULTI_TOUCH_PROTOCOL_TYPE_B
                G_point[i].x=ppt->x;
                G_point[i].y=ppt->y;
                G_point[i].z=ppt->z;
                G_point[i].status=ppt->status;
#endif

#ifdef MULTI_TOUCH_PROTOCOL_TYPE_B
                  tpd_down(ppt->x, ppt->y, ppt->z,i);
#endif
                TPD_EM_PRINT(ppt->raw_x, ppt->raw_y, ppt->x, ppt->y, ppt->z, 1);
            } else if(0x0==finger_status){
#ifndef MULTI_TOUCH_PROTOCOL_TYPE_B
                G_point[i].status=0;
                G_point[i].per_status=G_point[i].status;
                G_point[i].down_count=0;
#endif
                ppt = &ts->pre_points[i];
                if (ppt->status) {
                tpd_up(ppt->x, ppt->y,i);
                TPD_EM_PRINT(ppt->raw_x, ppt->raw_y, ppt->x, ppt->y, ppt->z, 0);
                }
            }
        }
 #ifndef MULTI_TOUCH_PROTOCOL_TYPE_B
         for(point_count=0;point_count<10;point_count++)
              {
                 if(G_point[point_count].status)
                 {
                      tpd_down(G_point[point_count].x, G_point[point_count].y, G_point[point_count].z,point_count);
           }
        }

#endif
        input_sync(tpd->dev);
        ppt = ts->pre_points;
        ts->pre_points = ts->cur_points;
        ts->cur_points = ppt;
    }
           ret=1;
exit:
return ret;
}

void Config_Eint_As_Gpio_In()
{
    mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
    mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_CTP_EINT_PIN, GPIO_PULL_ENABLE);
    mt_set_gpio_pull_select(GPIO_CTP_EINT_PIN, GPIO_PULL_UP);
}

#ifdef SYNAPTICS_DEBUG_IF
static void baseline()
{
unsigned char baseline[14][26];
unsigned char data[728];
unsigned char buf[10];
unsigned int i,j,flag=0;
unsigned int k = 0;

buf[0]=0x09;

buf[1]=0x0;
buf[2]=0x01;
buf[3]=0x04;
buf[4]=0x01;
buf[5]=0x0;

tpd_i2c_write_data(ts->client, 0x100, &buf[0],1);

tpd_i2c_write_data(ts->client, 0x5e, &buf[1],1);

tpd_i2c_write_data(ts->client, 0x15e, &buf[2],1);

tpd_i2c_write_data(ts->client, 0x172, &buf[3],1);

while(flag)
{
     tpd_i2c_read_data(ts->client, 0x172, &flag,1);
}

tpd_i2c_write_data(ts->client, 0x172, &buf[4],1);

flag = 1;

while(flag)
{
     tpd_i2c_read_data(ts->client, 0x172, &flag,1);
}


tpd_i2c_write_data(ts->client, 0x101, &buf[5],1);
tpd_i2c_write_data(ts->client, 0x102, &buf[5],1);

tpd_i2c_read_data(ts->client, 0x103, data,728);

for(i=0;i<14;i++)
{
     for(j=0;j<26;j++)
     {
          baseline[i][j] = data[k] | (data[k]<<8);
          k = k + 2;
     }
}

for(i=0;i<14;i++)
{
     for(j=0;j<26;j++)
     {
               TPD_DMESG("%2d",baseline[i][j]);
     }
     TPD_DMESG("\n ");
}
}
#endif

#ifdef USE_THREAD_HANDLER
static int tpd_work_func(void *unused)
#else
static void tpd_work_func(struct work_struct *work)
#endif
{
#ifdef USE_THREAD_HANDLER
    struct sched_param param = { .sched_priority = RTPM_PRIO_TPD };
    sched_setscheduler(current, SCHED_RR, &param);
     do
     {
        set_current_state(TASK_INTERRUPTIBLE);
        wait_event_interruptible(waiter,tpd_flag!=0);
        tpd_flag = 0;
        set_current_state(TASK_RUNNING);
        mutex_lock(&tp_mutex);
        if(!atomic_read(&suspend_flag))
        {
        #ifdef SYNAPTICS_DEBUG_IF
            if(!atomic_read(&debug_flag))
                {
        #endif
            tpd_process();
            mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
#ifdef SYNAPTICS_DEBUG_IF
            }
#endif
        }
        mutex_unlock(&tp_mutex);
      } while(!kthread_should_stop());
     return 0;
#else

mutex_lock(&tp_mutex);
if(!atomic_read(&suspend_flag))
{
    tpd_process();
    mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
}
mutex_unlock(&tp_mutex);
#endif
}

static void tpd_eint_handler(void)
{
    //TPD_DEBUG("TPD interrupt has been triggered\n");
    #ifdef USE_THREAD_HANDLER
    tpd_flag = 1;
    wake_up_interruptible(&waiter);
    #else
    queue_work(mtk_tpd_wq, &ts->work);
    #endif
}

static int tpd_sw_power(struct i2c_client *client, int on)
{
    int retval = 0;
    u8 device_ctrl = 0;

    retval = tpd_i2c_read_data(client,
                ts->f01.ctrl_base,
                &device_ctrl,
                sizeof(device_ctrl));
    if (retval <= 0) {
        TPD_DMESG("Error sensor can not wake up\n");
        goto out;
    }

    if (on)
    {
        device_ctrl = (device_ctrl & ~MASK_3BIT);
        device_ctrl = (device_ctrl | NO_SLEEP_OFF | NORMAL_OPERATION);

        retval = tpd_i2c_write_data(client,
                ts->f01.ctrl_base,
                &device_ctrl,
                sizeof(device_ctrl));
        if (retval <= 0) {
            TPD_DMESG("Error touch can not leave very-low power state\n");
            goto out;
        }

    } else {

        device_ctrl = (device_ctrl & ~MASK_3BIT);
        device_ctrl = (device_ctrl | NO_SLEEP_OFF | SENSOR_SLEEP);

        retval = tpd_i2c_write_data(client,
                ts->f01.ctrl_base,
                &device_ctrl,
                sizeof(device_ctrl));
        if (retval <= 0) {
            TPD_DMESG("Error touch can not enter very-low power state\n");
            goto out;
        }
    }

out:
    return retval;
}

static int tpd_detect (struct i2c_client *client,  struct i2c_board_info *info)
{
    strcpy(info->type, TPD_DEVICE);
    return 0;
}

static int __devexit tpd_remove(struct i2c_client *client)
{
    TPD_DEBUG("TPD removed\n");
    return 0;
}

static int tpd_clear_interrupt(struct i2c_client *client)
{
    int retval = 0;
    u8 status = 0;
    retval = tpd_i2c_read_data(client, ts->f01.data_base + 1, &status, 1);
    if (retval <= 0){
        dev_err(&client->dev,
                "%s: Failed to enable attention interrupt\n",
                __func__);
    }
    return retval;
}

void Config_Touch_GPIO(bool  work_or_stop)
{
   if(work_or_stop)//work
   {
    mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_EINT);
    mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_CTP_EINT_PIN, GPIO_PULL_ENABLE);
    mt_set_gpio_pull_select(GPIO_CTP_EINT_PIN, GPIO_PULL_UP);

    mt_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_TYPE, tpd_eint_handler, 1);
    mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
   }else{//stop
    mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
    mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_OUT);
   }

}

void synaptics_chip_reset()
{
    mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_OUT);

    //mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
    //mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
    //mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);

    mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_EINT);
    mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_CTP_EINT_PIN, GPIO_PULL_ENABLE);
    mt_set_gpio_pull_select(GPIO_CTP_EINT_PIN, GPIO_PULL_UP);

    mt_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_TYPE, tpd_eint_handler, 1);
}
EXPORT_SYMBOL(synaptics_chip_reset);

struct task_struct *firmware_thread = NULL;
static int tpd_firmware_updater(void *unused)
{
    int retval = 0;

    synaptics_fw_updater_s3203(synaImage);

    retval = tpd_rmi4_read_pdt(ts);
    if (retval <= 0) {
        TPD_DMESG("TPD_UPDATE_FIRMWARE Failed to tpd_rmi4_read_pdt\n");
    }
    return retval;
}

static int tpd_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    u8 ii;
    u8 attr_count,buf[2];
    u8 status = 0;
    int retval;
    int i;
    u16 TP_Max_X =0;
    u16 TP_Max_Y =0;
    u16 tp_x_for_lcd=0;
    u16 tp_y_for_lcd=0;

    struct synaptics_rmi4_fn *fhandler;
    struct synaptics_rmi4_data *rmi4_data;
    struct synaptics_rmi4_device_info *rmi;
    TPD_DMESG("%s:enter \n",__func__);
    mutex_init(&tp_mutex);

    synaptics_chip_reset();

    if (!i2c_check_functionality(client->adapter,
            I2C_FUNC_SMBUS_BYTE_DATA)) {
        TPD_DMESG("SMBus byte data not supported\n");
        return -EIO;
    }

    ts = kzalloc(sizeof(struct tpd_data), GFP_KERNEL);
    if (!ts) {
        TPD_DMESG("Failed to alloc mem for tpd_data\n");
        return -ENOMEM;
    }

    td = kzalloc(sizeof(struct tpd_debug), GFP_KERNEL);
    if (!td) {
        TPD_DMESG("Failed to alloc mem for tpd_debug\n");
    }

    ts->client = client;
    mutex_init(&(ts->io_ctrl_mutex));

    retval = tpd_rmi4_read_pdt(ts);
    if (retval <= 0) {
        TPD_DMESG("Failed to query device\n");
        goto err_query_device;
    }

    if (!exp_fn_inited) {
        mutex_init(&exp_fn_list_mutex);
        INIT_LIST_HEAD(&exp_fn_list);
        exp_fn_inited = 1;
    }
#ifdef USE_THREAD_HANDLER
    thread = kthread_run(tpd_work_func, 0, TPD_DEVICE);
    if (IS_ERR(thread))
    {
        retval = PTR_ERR(thread);
        TPD_DMESG(TPD_DEVICE " failed to create kernel thread: %d\n", retval);
    }
#endif
    INIT_WORK(&ts->work, tpd_work_func);
    mtk_tpd_wq = create_singlethread_workqueue("mtk_tpd_wq");
    if (!mtk_tpd_wq)
    {
        TPD_DMESG("Error Could not create work queue mtk_tpd_wq: no memory");
        retval = -ENOMEM;
        goto error_wq_creat_failed;
    }

    tpd_clear_interrupt(client);

    properties_kobj_synap = kobject_create_and_add("synapics", NULL);


#ifdef HAVE_TOUCH_KEY
#if 1
         set_bit(EV_KEY, tpd->dev->evbit);
        for(i=0;i<MAX_KEY_NUM;i++)
       __set_bit(touch_key_array[i], tpd->dev->keybit);
#endif
#endif

#ifdef MULTI_TOUCH_PROTOCOL_TYPE_B
    input_mt_init_slots(tpd->dev, SYNAPTICS_TOUCH_TRACK_IDS);
    set_bit(BTN_TOOL_FINGER, tpd->dev->keybit);
    set_bit(BTN_TOOL_DOUBLETAP, tpd->dev->keybit);
    set_bit(BTN_TOOL_TRIPLETAP, tpd->dev->keybit);
    set_bit(BTN_TOOL_QUADTAP, tpd->dev->keybit);
    set_bit(BTN_TOOL_QUINTTAP, tpd->dev->keybit);
    input_set_abs_params(tpd->dev, ABS_MT_TOOL_TYPE,
                0, MT_TOOL_MAX, 0, 0);
#endif
    synaptics_rmi4_detection_work(NULL);


    tpd_i2c_read_data(client, ts->f11.ctrl_base+6, &TP_Max_X,2);
    tpd_i2c_read_data(client, ts->f11.ctrl_base+8,&TP_Max_Y,2);

    TPD_DMESG("tpd_probe   TP_Max_X=:%d!!TP_Max_Y=%d\n",TP_Max_X,TP_Max_Y);
    tp_x_for_lcd = LCD_X;
    tp_y_for_lcd = (LCD_Y*TPD_BUTTON_HEIGH) /(TP_Max_Y-TPD_BUTTON_HEIGH)+LCD_Y;


    TPD_DMESG("after TP_Max_X = %d, TP_Max_Y = %d\n", TP_Max_X, TP_Max_Y);

    properties_kobj_driver = kobject_create_and_add("driver", properties_kobj_synap);

    for (attr_count = 0; attr_count < ARRAY_SIZE(attrs); attr_count++) {
        retval = sysfs_create_file(properties_kobj_driver, &attrs[attr_count].attr);

        if (retval < 0) {
            dev_err(&client->dev, "%s: Failed to create sysfs attributes\n", __func__);
            goto err_sysfs;
        }
    }
    Config_Touch_GPIO(true);
    tpd_load_status = 1;
    TPD_DMESG("%s: TouchPanel Device Probe %s\n", __func__, (retval < 0) ? "FAIL" : "PASS");
    return 0;


err_sysfs:
    for (attr_count--; attr_count >= 0; attr_count--) {
        sysfs_remove_file(properties_kobj_driver, &attrs[attr_count].attr);
    }

error_wq_creat_failed:
err_query_device:
    kfree(td);

err_tpd_data:

    kfree(ts);
    return retval;
}

static int tpd_local_init(void)
{
    TPD_DMESG("Synaptics I2C Touchscreen Driver (Built %s @ %s)\n", __DATE__, __TIME__);

    if(i2c_add_driver(&tpd_i2c_driver)!=0)
    {
        TPD_DMESG("Error unable to add i2c driver.\n");
        return -1;
    }

    if(touch_ssb_data.use_tpd_button == 1)
         tpd_button_setting(TPD_KEY_COUNT, touch_ssb_data.tpd_key_local, touch_ssb_data.tpd_key_dim_local);

#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
    TPD_DO_WARP = 1;
    memcpy(tpd_wb_start, tpd_wb_start_local, TPD_WARP_CNT*4);
    memcpy(tpd_wb_end, tpd_wb_start_local, TPD_WARP_CNT*4);
#endif

#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
    memcpy(tpd_calmat, tpd_def_calmat_local, 8*4);
    memcpy(tpd_def_calmat, tpd_def_calmat_local, 8*4);
#endif

    boot_mode = get_boot_mode();
    if (boot_mode == 3) boot_mode = NORMAL_BOOT;

    TPD_DMESG("end %s, %d\n", __FUNCTION__, __LINE__);
    tpd_type_cap = 1;
    return 0;
 }

extern atomic_t Firmware_Update_Flag;

static void tpd_resume(struct early_suspend *h)
{
    TPD_DEBUG("TPD wake up\n");

    if(atomic_read(&suspend_flag))
    {
        tpd_sw_power(ts->client, 1);
        tpd_clear_interrupt(ts->client);
        Config_Touch_GPIO(true);
        mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
        atomic_set(&suspend_flag,0);
    }
    return;
}

static void tpd_suspend(struct early_suspend *h)
{
    TPD_DEBUG("TPD enter sleep\n");
    if(atomic_read(&Firmware_Update_Flag))
    {
       TPD_DEBUG("Firmware_Update_Flag no need suspend \n");
          return;
    }
    mutex_lock(&tp_mutex);

    mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);

    tpd_sw_power(ts->client, 0);
    Config_Touch_GPIO(false);


    atomic_set(&suspend_flag,1);
    mutex_unlock(&tp_mutex);
     return;
 }


static struct tpd_driver_t tpd_device_driver = {
    .tpd_device_name = "synaptics_s3203",
    .tpd_local_init = tpd_local_init,
    .suspend = tpd_suspend,
    .resume = tpd_resume,
#ifdef TPD_HAVE_BUTTON
    .tpd_have_button = 1,
#else
    .tpd_have_button = 0,
#endif
};

static int __init tpd_driver_init(void)
{
#ifdef  SYNAPTICS_DEBUG_IF
    tpDMABuf_va = (unsigned char *)dma_alloc_coherent(NULL, 4096, &tpDMABuf_pa, GFP_KERNEL);
    if(!tpDMABuf_va){
        TPD_DMESG("xxxx Allocate DMA I2C Buffer failed!xxxx\n");
        return -1;
    }
#endif
    int err = 0;
    char name[20] = "s3203";
    printk("s3203 synaptics touch panel driver init\n");
    err = tpd_ssb_data_match(name, &touch_ssb_data);
    if(err != 0){
        printk("touch tpd_ssb_data_match error\n");
        return -1;
    }
    printk("s3203 touch_ssb_data:: name:(%s), endflag:0x%x, i2c_number:0x%x, i2c_addr:0x%x,power_id:%d, use_tpd_button:%d\n",
    touch_ssb_data.identifier,
    touch_ssb_data.endflag,
    touch_ssb_data.i2c_number,
    touch_ssb_data.i2c_addr,
    touch_ssb_data.power_id,
    touch_ssb_data.use_tpd_button
    );


    i2c_tpd.addr = touch_ssb_data.i2c_addr;

    i2c_register_board_info(touch_ssb_data.i2c_number, &i2c_tpd, 1);

    //add for ssb support
    tpd_device_driver.tpd_have_button = touch_ssb_data.use_tpd_button;

    if(tpd_driver_add(&tpd_device_driver) < 0)
        TPD_DMESG("Error Add synaptics driver failed\n");
    return 0;
}

static void __exit tpd_driver_exit(void)
{
    TPD_DMESG("synaptics touch panel driver exit\n");
    tpd_driver_remove(&tpd_device_driver);
}

module_init(tpd_driver_init);
module_exit(tpd_driver_exit);

MODULE_DESCRIPTION("Mediatek synaptics Driver");

