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

#include "tpd.h"
#include <linux/interrupt.h>
#include <cust_eint.h>
#include <linux/i2c.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>

#include "tpd_custom_ft6x06.h"

#include <mach/mt_pm_ldo.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_boot.h>

#if defined(MT6577)
#include <mach/mt_pm_ldo.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_boot.h>
#elif defined(MT6575)
#include <mach/mt6575_pm_ldo.h>
#include <mach/mt6575_typedefs.h>
#include <mach/mt6575_boot.h>
#elif defined(CONFIG_ARCH_MT6573)
#include <mach/mt6573_boot.h>
#endif

#include "cust_gpio_usage.h"

#ifdef  CONFIG_TOUCHSCREEN_FT6X05_DISABLE_KEY_WHEN_SLIDE
#include <linux/timer.h>

#define KEY_STORE_MAX 20
struct keystorevalue{
        unsigned int   code;
    unsigned int   value;
//    unsigned long time;
};

struct keystore{
        struct keystorevalue value[KEY_STORE_MAX];
    unsigned int    head;
    unsigned int    tail;
    unsigned char tp_key_init_flag;
    struct timer_list tp_key_delay_timer;
    struct work_struct tp_queue;
    struct workqueue_struct *tp_key_wq;
    struct mutex tp_key_mutex;
    struct input_dev *dev;
};

static struct keystore *tp_keystore;

static unsigned long curtime=0;

#define Is_tp_buf_empty ((tp_keystore->head==tp_keystore->tail)?1:0)
#define Is_tp_buf_full ((((tp_keystore->head+1)%KEY_STORE_MAX)==tp_keystore->tail)?1:0)
static void tp_buf_clear(void)
{
    tp_keystore->head=tp_keystore->tail=0;
}

static int tp_buf_put(int keycode,int keyvalue)
{
    int ret=0;
    CTP_DBG("head=%d,tail=%d!\r\n",tp_keystore->head,tp_keystore->tail);
    mutex_lock(&tp_keystore->tp_key_mutex);
    if(!Is_tp_buf_full){
        tp_keystore->value[tp_keystore->head].code=keycode;
        tp_keystore->value[tp_keystore->head].value=keyvalue;
        tp_keystore->head=(tp_keystore->head+1)%KEY_STORE_MAX;
    }
    else
        ret=-1;
    mutex_unlock(&tp_keystore->tp_key_mutex);
    CTP_DBG("1 head=%d,tail=%d!\r\n",tp_keystore->head,tp_keystore->tail);
    return ret;
}

static int tp_buf_get(struct keystorevalue * key)
{
    int ret=0;
    mutex_lock(&tp_keystore->tp_key_mutex);
    if(!Is_tp_buf_empty){
        key->code=tp_keystore->value[tp_keystore->tail].code;
        key->value=tp_keystore->value[tp_keystore->tail].value;
        tp_keystore->tail=(tp_keystore->tail+1)%KEY_STORE_MAX;
    }
    else
        ret=-1;
    mutex_unlock(&tp_keystore->tp_key_mutex);
    return ret;
}

static int tp_key_handle(int keycode,int keyvalue)
{
    int ret =0;



    if(Is_tp_buf_empty){
         CTP_DBG("tp mod_timer!\r\n");
        mod_timer(&tp_keystore->tp_key_delay_timer,jiffies + HZ/5);
    }
    tp_buf_put(keycode,keyvalue);


    return ret;
}

static void tp_delay_process(struct work_struct *work)
{
      struct keystorevalue  key;
         CTP_DBG("tp_delay_process!\r\n");
     while(!tp_buf_get(&key)){
         CTP_DBG("key code=%d,value=%d!\r\n",key.code,key.value);

         input_report_key( tp_keystore->dev, key.code, key.value);
        input_sync(tp_keystore->dev);
        msleep(100);
     }
}

static void tp_key_delay_timer_handler(unsigned long dev_addr)
{

      queue_work(tp_keystore->tp_key_wq, &tp_keystore->tp_queue);
}

static int tp_key_init(struct input_dev *dev)
{
    tp_keystore = kzalloc(sizeof(*tp_keystore), GFP_KERNEL);
    if (tp_keystore == NULL) {
        return -ENOMEM;

    }
    memset(tp_keystore, 0, sizeof(*tp_keystore));
    tp_keystore->head=tp_keystore->tail=0;
    tp_keystore->dev=dev;
    tp_keystore->tp_key_init_flag=0;
    tp_keystore->tp_key_wq = create_singlethread_workqueue("tp_key_wq");
    if (!tp_keystore->tp_key_wq){
        kfree(tp_keystore);
        return -ENOMEM;
    }
    mutex_init(&tp_keystore->tp_key_mutex);
    INIT_WORK(&tp_keystore->tp_queue, tp_delay_process);

    init_timer(&tp_keystore->tp_key_delay_timer);
    tp_keystore->tp_key_delay_timer.data = (unsigned long)dev;
    tp_keystore->tp_key_delay_timer.function = tp_key_delay_timer_handler;
    tp_keystore->tp_key_delay_timer.expires = jiffies +  HZ/5;
    add_timer(&tp_keystore->tp_key_delay_timer);
    tp_keystore->tp_key_init_flag=1;

    return 0;
}

int fts_6x06_key_cancel(void)
{
    CTP_DBG();
    tp_buf_clear();
    if(tp_keystore->tp_key_init_flag){
        del_timer(&tp_keystore->tp_key_delay_timer);
        cancel_work_sync(&tp_keystore->tp_queue);
    }
    return 0;
}
static int tp_key_exit(void)
{
    CTP_DBG();
    fts_6x06_key_cancel();
    if (tp_keystore->tp_key_wq)
        destroy_workqueue(tp_keystore->tp_key_wq);
    kfree(tp_keystore);
    return 0;
}

static  unsigned long get_time (void)
{
    struct timeval tv;
    do_gettimeofday(&tv);
    return (1000000 * tv.tv_sec + tv.tv_usec);
}
#endif

int fts_6x06_parase_keys(tinno_ts_data *ts, fts_report_data_t *pReportData)
{
    const unsigned int izKeyCode[] = {KEY_HOME, KEY_MENU, KEY_BACK, KEY_SEARCH};
    int iKeyCode = izKeyCode[pReportData->gesture - 0xF0];
    //CTP_DBG("The touch key is (%d) !", iKeyCode);
#ifdef CONFIG_TOUCHSCREEN_FT5X05_DISABLE_KEY_WHEN_SLIDE
    curtime=get_time();
#endif
    if ( FTS_EF_DOWN == pReportData->xy_data[0].event_flag ){
        if ( TKS_DOWNED == ts->key_state
            ||  TKS_MOVING == ts->key_state ){
            CTP_DBG("The touch key state(%d) is error, we should append a key up event for last key!", ts->key_state);
#ifdef CONFIG_TOUCHSCREEN_FT6X05_DISABLE_KEY_WHEN_SLIDE
            tp_key_handle(iKeyCode,0);
#else
            input_report_key( ts->keys_dev, ts->mLastKeyCode, 0 );
            input_sync(ts->keys_dev);
#endif
            ts->key_state = TKS_IDLE;
        }else if ( TKS_UPPED == ts->key_state ){
            ts->key_state = TKS_IDLE;
        }

        if ( TKS_IDLE == ts->key_state ){
            ts->key_state = TKS_DOWNED;
#ifdef CONFIG_TOUCHSCREEN_FT6X05_DISABLE_KEY_WHEN_SLIDE
            tp_key_handle(iKeyCode,1);
#else
            input_report_key( ts->keys_dev, iKeyCode, 1 );
            input_sync(ts->keys_dev);
#endif
            CTP_DBG("The touch key(%d) DOWM!", iKeyCode);
        }
    }else if ( FTS_EF_CONTACT == pReportData->xy_data[0].event_flag ){
        if ( TKS_IDLE == ts->key_state ){
            CTP_DBG("The touch key state(%d) is error, we should append a key down event!", ts->key_state);
            ts->key_state = TKS_DOWNED;
#ifdef CONFIG_TOUCHSCREEN_FT6X05_DISABLE_KEY_WHEN_SLIDE
            tp_key_handle(iKeyCode,1);
#else
            input_report_key( ts->keys_dev, iKeyCode, 1 );
            input_sync(ts->keys_dev);
#endif
        }

        if ( TKS_DOWNED == ts->key_state ){
            ts->key_state = TKS_MOVING;
        }

        CTP_DBG("The touch key is moving, we should ignore this event!");
    }else if ( FTS_EF_UP == pReportData->xy_data[0].event_flag ){
        if ( TKS_MOVING == ts->key_state
            || TKS_DOWNED == ts->key_state ){
            ts->key_state = TKS_UPPED;
#ifdef CONFIG_TOUCHSCREEN_FT6X05_DISABLE_KEY_WHEN_SLIDE
            tp_key_handle(iKeyCode,0);
#else
            input_report_key( ts->keys_dev, iKeyCode, 0 );
            input_sync(ts->keys_dev);
#endif
            CTP_DBG("The touch key(%d) UP!", iKeyCode);
        }
    }

    if ( TKS_UPPED == ts->key_state ){
        ts->key_state = TKS_IDLE;
    }

    ts->mLastKeyCode = iKeyCode;
    return 0;
}

int fts_keys_init(tinno_ts_data *ts)
{
    int ret = 0;
    ts->keys_dev = input_allocate_device();
    if (ts->keys_dev == NULL) {
        ret = -ENOMEM;
        dev_err(&ts->client->dev,"Failed to allocate input key device");
        goto err_input_key_alloc_failed;
    }

    ts->keys_dev->name = "Tinno-TouchKey";
    input_set_capability(ts->keys_dev, EV_KEY, KEY_MENU);
    input_set_capability(ts->keys_dev, EV_KEY, KEY_BACK);
    input_set_capability(ts->keys_dev, EV_KEY, KEY_HOME);
    input_set_capability(ts->keys_dev, EV_KEY, KEY_SEARCH);
    input_set_drvdata(ts->keys_dev, ts);
    ret = input_register_device(ts->keys_dev);
    if (ret) {
        dev_err(&ts->client->dev,"Unable to register %s input device", ts->keys_dev->name);
        goto err_input_register_key_device_failed;
    }

    tp_key_init(ts->keys_dev);
    return 0;
err_input_register_key_device_failed:
    input_free_device(ts->keys_dev);
err_input_key_alloc_failed:
    return ret;
}

void fts_keys_deinit(void)
{
    tp_key_exit();
}


