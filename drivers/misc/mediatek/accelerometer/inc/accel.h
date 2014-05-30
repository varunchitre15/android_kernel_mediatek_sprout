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
#ifndef __ACC_H__
#define __ACC_H__


#include <linux/wakelock.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/hwmsensor.h>
#include <linux/earlysuspend.h>
#include <linux/hwmsen_dev.h>


#define ACC_TAG                    "<ACCELEROMETER> "
#define ACC_FUN(f)                printk(KERN_ERR ACC_TAG"%s\n", __func__)
#define ACC_ERR(fmt, args...)        printk(KERN_ERR ACC_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define ACC_LOG(fmt, args...)        printk(KERN_ERR ACC_TAG fmt, ##args)
#define ACC_VER(fmt, args...)       printk(KERN_ERR ACC_TAG"%s: "fmt, __func__, ##args) //((void)0)

#define OP_ACC_DELAY    0X01
#define    OP_ACC_ENABLE    0X02
#define    OP_ACC_GET_DATA    0X04

#define ACC_INVALID_VALUE -1

#define EVENT_TYPE_ACCEL_X                  ABS_X
#define EVENT_TYPE_ACCEL_Y                  ABS_Y
#define EVENT_TYPE_ACCEL_Z                  ABS_Z
#define EVENT_TYPE_ACCEL_STATUS     ABS_WHEEL
#define EVENT_TYPE_ACCEL_DIV        ABS_GAS
#define EVENT_TYPE_SIGNIFICANT_MOTION     ABS_THROTTLE


#define ACC_VALUE_MAX (32767)
#define ACC_VALUE_MIN (-32768)
#define ACC_STATUS_MIN (0)
#define ACC_STATUS_MAX (64)
#define ACC_DIV_MAX (32767)
#define ACC_DIV_MIN (1)


#define MAX_CHOOSE_G_NUM 5

struct acc_control_path
{
    int (*open_report_data)(int open);//open data rerport to HAL
    int (*enable_nodata)(int en);//only enable not report event to HAL
    int (*set_delay)(u64 delay);
    int (*access_data_fifo)(void);//version2.used for flush operate
    int (*enable_significant_motion)(int en);
    bool is_report_input_direct;
    bool is_support_batch;//version2.used for batch mode support flag
};

struct acc_data_path
{
    int (*get_data)(int *x,int *y, int *z,int *status);
    int vender_div;
};

struct acc_init_info
{
    char *name;
    int (*init)(void);
    int (*uninit)(void);
    struct platform_driver* platform_diver_addr;
};

struct acc_data{
    hwm_sensor_data acc_data ;
    int data_updata;
};

struct acc_drv_obj {
    void *self;
    int polling;
    int (*acc_operate)(void* self, uint32_t command, void* buff_in, int size_in,
        void* buff_out, int size_out, int* actualout);
};

struct acc_context {
    struct input_dev   *idev;
    struct miscdevice   mdev;
    struct work_struct  report;
    struct mutex acc_op_mutex;
    int		        delay; /*polling period for reporting input event*/
    atomic_t            wake;  /*user-space request to wake-up, used with stop*/
    struct timer_list   timer;  /* polling timer */
    atomic_t            trace;
    struct early_suspend    early_drv;
    atomic_t                early_suspend;
    struct acc_data       drv_data;
    struct acc_control_path   acc_ctl;
    struct acc_data_path   acc_data;
    bool            is_active_nodata;        // Active, but HAL don't need data sensor. such as orientation need
    bool            is_active_data;        // Active and HAL need data .
    bool is_first_data_after_enable;
    bool is_polling_run;
    bool is_batch_enable;    //version2.this is used for judging whether sensor is in batch mode
};


//for auto detect
extern int acc_driver_add(struct acc_init_info* obj) ;
extern int acc_data_report(int x, int y, int z,int status);
extern int acc_register_control_path(struct acc_control_path *ctl);
extern int acc_register_data_path(struct acc_data_path *data);

extern int acc_report_motion_detect(int motion);




#endif
