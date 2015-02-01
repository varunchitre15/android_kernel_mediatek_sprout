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

#include <linux/jiffies.h>

#ifdef TPD_PROXIMITY
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#endif

#include "focaltech_ctl.h"
#include <cust_gpio_usage.h>

#ifdef  FTS_AUTO_TP_UPGRADE
#include "ftbin_HRC.h"
#include "ftbin_yeji.h"
#include "ftbin_jiemian.h"
#endif

#define TPD_OK 0
//#define  FTS_AUTO_TP_UPGRADE
#define FTS_SUPPORT_TRACK_ID
#define APS_ERR(fmt, args...)    printk(KERN_ERR  "%d : "fmt, __FUNCTION__, __LINE__, ##args)

extern struct tpd_device *tpd;

static DECLARE_WAIT_QUEUE_HEAD(waiter);
static void tpd_eint_interrupt_handler(void);
extern void mt_eint_unmask(unsigned int line);
extern void mt_eint_mask(unsigned int line);
extern void mt_eint_set_hw_debounce(unsigned int eint_num, unsigned int ms);
extern unsigned int mt_eint_set_sens(unsigned int eint_num, unsigned int sens);
extern void mt_eint_registration(unsigned int eint_num, unsigned int flag,
              void (EINT_FUNC_PTR) (void), unsigned int is_auto_umask);

static int __devinit tpd_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tpd_detect(struct i2c_client *client, struct i2c_board_info *info);
static int __devexit tpd_remove(struct i2c_client *client);
static int touch_event_handler(void *unused);

static struct tag_para_touch_ssb_data_single touch_ssb_data = {0};


static int tpd_keys_local[TPD_KEY_COUNT] = TPD_KEYS;
static int tpd_keys_dim_local_BYD[TPD_KEY_COUNT][4] = TPD_KEYS_DIM_BYD;
static int tpd_keys_dim_local_NB[TPD_KEY_COUNT][4] = TPD_KEYS_DIM_NB;

kal_uint8 g_pre_tp_charger_flag = 0;
kal_uint8 g_tp_charger_flag = 0;

//BEIN <tp> <DATE20130514> <tp proximity> zhangxiaofei
#ifdef TPD_PROXIMITY

#define TPD_PROXIMITY_ENABLE_REG                  0xB0
#define TPD_PROXIMITY_CLOSE_VALUE                 0xC0
#define TPD_PROXIMITY_FARAWAY_VALUE               0xE0

static u8 tpd_proximity_flag             = 0;
static u8 tpd_proximity_flag_one         = 0; //add for tpd_proximity by wangdongfang
static u8 tpd_proximity_detect             = 1; //0-->close ; 1--> far away

#endif
//END <tp> <DATE20130514> <tp proximity> zhangxiaofei

static void tinno_update_tp_button_dim(int panel_vendor)
{
    if ( FTS_CTP_VENDOR_NANBO == panel_vendor ){
        tpd_button_setting(TPD_KEY_COUNT, touch_ssb_data.tpd_key_local, touch_ssb_data.tpd_key_dim_local);
    }else{
        tpd_button_setting(TPD_KEY_COUNT, touch_ssb_data.tpd_key_local, touch_ssb_data.tpd_key_dim_local);
    }
}


static char tpd_desc[50];
static int tpd_fw_version;

static tinno_ts_data *g_pts = NULL;
static volatile    int tpd_flag;

#define DRIVER_NAME "ft6x06"

static const struct i2c_device_id ft6x06_tpd_id[] = {{DRIVER_NAME,0},{}};
static struct i2c_board_info __initdata ft6x06_i2c_tpd[]={{I2C_BOARD_INFO(DRIVER_NAME, TPD_I2C_SLAVE_ADDR)}};

static struct i2c_driver tpd_i2c_driver = {
    .driver = {
         .name = DRIVER_NAME,
    },
    .probe = tpd_probe,
    .remove = __devexit_p(tpd_remove),
    .id_table = ft6x06_tpd_id,
    .detect = tpd_detect,
};

static  void tpd_down(tinno_ts_data *ts, int x, int y, int pressure, int trackID)
{
    CTP_DBG("x=%03d, y=%03d, pressure=%03d, ID=%03d", x, y, pressure, trackID);
    input_report_abs(tpd->dev, ABS_PRESSURE, pressure);
    input_report_abs(tpd->dev, ABS_MT_PRESSURE, pressure);
    input_report_key(tpd->dev, BTN_TOUCH, 1);
    input_report_abs(tpd->dev, ABS_MT_POSITION_X, x);
    input_report_abs(tpd->dev, ABS_MT_POSITION_Y, y);
#ifdef FTS_SUPPORT_TRACK_ID
    input_report_abs(tpd->dev, ABS_MT_TRACKING_ID, trackID);
#endif
    input_report_abs(tpd->dev, ABS_MT_WIDTH_MAJOR, pressure*pressure/112);
    input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, pressure*pressure/112);
    input_mt_sync(tpd->dev);
    __set_bit(trackID, &ts->fingers_flag);
    ts->touch_point_pre[trackID].x=x;
    ts->touch_point_pre[trackID].y=y;
    if (FACTORY_BOOT == get_boot_mode() || RECOVERY_BOOT == get_boot_mode()) {
        tpd_button(x, y, 1);
    }
    TPD_DOWN_DEBUG_TRACK(x,y);
 }

static  int tpd_up(tinno_ts_data *ts, int x, int y, int pressure, int trackID)
{
    if (FACTORY_BOOT == get_boot_mode() || RECOVERY_BOOT == get_boot_mode()) {
        CTP_DBG("x=%03d, y=%03d, ID=%03d", x, y, trackID);
        input_report_abs(tpd->dev, ABS_PRESSURE, 0);
        input_report_abs(tpd->dev, ABS_MT_PRESSURE, 0);
        input_report_key(tpd->dev, BTN_TOUCH, 0);
        input_report_abs(tpd->dev, ABS_MT_POSITION_X, x);
        input_report_abs(tpd->dev, ABS_MT_POSITION_Y, y);
#ifdef FTS_SUPPORT_TRACK_ID
        input_report_abs(tpd->dev, ABS_MT_TRACKING_ID, trackID);
#endif
        input_report_abs(tpd->dev, ABS_MT_WIDTH_MAJOR, 0);
        input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 0);// This must be placed at the last one.
        input_mt_sync(tpd->dev);
    }else{//Android 4.0 don't need to report these up events.
        int i, have_down_cnt = 0;
        for ( i=0; i < TINNO_TOUCH_TRACK_IDS; i++ ){
            if ( test_bit(i, &ts->fingers_flag) ){
                ++have_down_cnt;
            }
        }
        if ( have_down_cnt < 2 ){
            input_mt_sync(tpd->dev);
        }
        CTP_DBG("x=%03d, y=%03d, ID=%03d, have_down=%d", x, y, trackID, have_down_cnt);
    }

    __clear_bit(trackID, &ts->fingers_flag);
    TPD_UP_DEBUG_TRACK(x,y);
    if (FACTORY_BOOT == get_boot_mode() || RECOVERY_BOOT == get_boot_mode()) {
        tpd_button(x, y, 0);
    }
    return 0;
 }

 static void tpd_dump_touchinfo(tinno_ts_data *ts)
 {
     uint8_t *pdata = ts->buffer;
    CTP_DBG("0x%02x 0x%02x 0x%02x"
        "   0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x"
        "   0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x"
        "   0x%02x 0x%02x  0x%02x 0x%02x 0x%02x 0x%02x"
        "   0x%02x 0x%02x  0x%02x 0x%02x 0x%02x 0x%02x"
              ,
          pdata[0],   pdata[1],  pdata[2],
          pdata[3],   pdata[4],  pdata[5],   pdata[6],  pdata[7], pdata[8],
          pdata[9],  pdata[10], pdata[11], pdata[12], pdata[13], pdata[15],
          pdata[15], pdata[16], pdata[17], pdata[18], pdata[19], pdata[20],
          pdata[21], pdata[22], pdata[23], pdata[24], pdata[25], pdata[26]);
 }

 static void ft_map_coordinate(int *pX, int *pY)
 {
     int x = *pX, y = *pY;
    *pX = x * 540 / 760;
    *pY = y * 960 / 1280;
 }


 static int tpd_touchinfo(tinno_ts_data *ts, tinno_ts_point *touch_point)
 {
    int i = 0;
    int iInvalidTrackIDs = 0;
    int iTouchID, iSearchDeep;
    fts_report_data_t *pReportData = (fts_report_data_t *)ts->buffer;

    if ( tpd_read_touchinfo(ts) ){
        CTP_DBG("Read touch information error. \n");
        return -EAGAIN;
    }

    if ( 0 != pReportData->device_mode ){
        CTP_DBG("device mode is %d\n", pReportData->device_mode);
        return -EPERM;
    }

    //We need only valid points...
    if ( pReportData->fingers > TINNO_TOUCH_TRACK_IDS ){
        CTP_DBG("fingers is %d\n", pReportData->fingers);
        return -EAGAIN;
    }

    // For processing gestures.
    if (pReportData->gesture >= 0xF0 && pReportData->gesture <= 0xF3) {
        //fts_6x06_parase_keys(ts, pReportData);
    }
    iSearchDeep = 0;

#ifdef FTS_SUPPORT_TRACK_ID
    for ( i = 0; i < TINNO_TOUCH_TRACK_IDS; i++ ){
        iSearchDeep += ((pReportData->xy_data[i].event_flag != FTS_EF_RESERVED)?1:0);
    }
#else
    if (pReportData->fingers >= ts->last_fingers ){
        iSearchDeep = pReportData->fingers;
    }else{
        iSearchDeep = ts->last_fingers;
    }
    ts->last_fingers = pReportData->fingers;
#endif

    if ( iSearchDeep ) {
#ifdef FTS_SUPPORT_TRACK_ID
        for ( i=0; i < TINNO_TOUCH_TRACK_IDS; i++ ){
#else
        for ( i=0; i < iSearchDeep; i++ ){
#endif
            if (pReportData->xy_data[i].event_flag != FTS_EF_RESERVED) {
#ifdef FTS_SUPPORT_TRACK_ID
                iTouchID = pReportData->xy_data[i].touch_id;
                if ( iTouchID >= TINNO_TOUCH_TRACK_IDS )
                {
                    //CTP_DBG("i: Invalied Track ID(%d)\n!", i, iTouchID);
                    iInvalidTrackIDs++;
                    continue;
                }
#else
                iTouchID = i;
#endif
                touch_point[iTouchID].flag = pReportData->xy_data[i].event_flag;
                touch_point[iTouchID].x = pReportData->xy_data[i].x_h << 8 | pReportData->xy_data[i].x_l;
                touch_point[iTouchID].y = pReportData->xy_data[i].y_h << 8 | pReportData->xy_data[i].y_l;
                touch_point[iTouchID].pressure = pReportData->xy_data[i].pressure;
#ifdef TPD_FIRST_FIRWARE
                ft_map_coordinate(&(touch_point[iTouchID].x), &(touch_point[iTouchID].y));
#endif
            }else{
                //CTP_DBG("We got a invalied point, we take it the same as a up event!");
                //CTP_DBG("As it has no valid track ID, we assume it's order is the same as it's layout in the memory!");
                //touch_point[i].flag = FTS_EF_RESERVED;
            }
        }
        if ( TINNO_TOUCH_TRACK_IDS == iInvalidTrackIDs ){
            CTP_DBG("All points are Invalied, Ignore the interrupt!\n");
            return -EAGAIN;
        }
    }

    CTP_DBG("p0_flag=0x%x x0=0x%03x y0=0x%03x pressure0=0x%03x "
                  "p1_flag=0x%x x1=0x%03x y1=0x%03x pressure1=0x%03x "
                  "gesture = 0x%x fingers=0x%x",
           touch_point[0].flag, touch_point[0].x, touch_point[0].y, touch_point[0].pressure,
           touch_point[1].flag, touch_point[1].x, touch_point[1].y, touch_point[1].pressure,
           pReportData->gesture, pReportData->fingers);

     return 0;

 };


//BEGIN <touch panel> <DATE20130831> <tp proximity> zhangxiaofei
#if defined TPD_PROXIMITY
int tpd_read_ps(void)
{
    tpd_proximity_detect;
    return 0;
}

static int tpd_get_ps_value(void)
{
    return tpd_proximity_detect;
}

static int tpd_enable_ps(int enable)
{
    u8 state;
    int ret = -1;

    i2c_smbus_read_i2c_block_data(g_pts->client, TPD_PROXIMITY_ENABLE_REG, 1, &state);
    printk("[proxi_5206]read: 999 0xb0's value is 0x%02X\n", state);
    if (enable){
        state |= 0x01;
        tpd_proximity_flag = 1;
        TPD_PROXIMITY_DBG("[proxi_5206]ps function is on\n");
    }else{
        state &= 0x00;
        tpd_proximity_flag = 0;
        TPD_PROXIMITY_DBG("[proxi_5206]ps function is off\n");
    }

    ret = i2c_smbus_write_i2c_block_data(g_pts->client, TPD_PROXIMITY_ENABLE_REG, 1, &state);
    TPD_PROXIMITY_DBG("[proxi_5206]write: 0xB0's value is 0x%02X\n", state);
    return 0;
}

int tpd_ps_operate(void* self, uint32_t command, void* buff_in, int size_in,
        void* buff_out, int size_out, int* actualout)
{
    int err = 0;
    int value;

    hwm_sensor_data *sensor_data;
    TPD_DEBUG("[proxi_5206]command = 0x%02X\n", command);
    switch (command)
    {
        case SENSOR_DELAY:
            if((buff_in == NULL) || (size_in < sizeof(int)))
            {
                APS_ERR("Set delay parameter error!\n");
                err = -EINVAL;
            }
            // Do nothing
            break;

        case SENSOR_ENABLE:
            if((buff_in == NULL) || (size_in < sizeof(int)))
            {
                APS_ERR("Enable sensor parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                value = *(int *)buff_in;
                if(value)
                {
                    if((tpd_enable_ps(1) != 0))
                    {
                        APS_ERR("enable ps fail: %d\n", err);
                        return -1;
                    }
                }
                else
                {
                    if((tpd_enable_ps(0) != 0))
                    {
                        APS_ERR("disable ps fail: %d\n", err);
                        return -1;
                    }
                }
            }
            break;

        case SENSOR_GET_DATA:
            if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
            {
                APS_ERR("get sensor data parameter error!\n");
                err = -EINVAL;
            }
            else
            {

                sensor_data = (hwm_sensor_data *)buff_out;

                if((err = tpd_read_ps()))
                {
                    err = -1;;
                }
                else
                {
                    sensor_data->values[0] = tpd_get_ps_value();
                    TPD_PROXIMITY_DBG("huang sensor_data->values[0] 1082 = %d\n", sensor_data->values[0]);
                    sensor_data->value_divide = 1;
                    sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
                }

            }
            break;
        default:
            APS_ERR("proxmy sensor operate function no this parameter %d!\n", command);
            err = -1;
            break;
    }

    return err;
}
#endif
//END <touch panel> <DATE20130831> <tp proximity> zhangxiaofei

 static int touch_event_handler(void *para)
 {
     int i;
    tinno_ts_point touch_point[TINNO_TOUCH_TRACK_IDS];
    struct sched_param param = { .sched_priority = RTPM_PRIO_TPD };
    tinno_ts_data *ts = (tinno_ts_data *)para;
    sched_setscheduler(current, SCHED_RR, &param);

    //BEGIN <touch panel> <DATE20130831> <tp proximity> zhangxiaofei
    #if defined TPD_PROXIMITY
    int err;
    hwm_sensor_data sensor_data;
    u8 proximity_status;
    u8 state;
    #endif
    //END <touch panel> <DATE20130831> <tp proximity> zhangxiaofei
    do {
        set_current_state(TASK_INTERRUPTIBLE);
        wait_event_interruptible(waiter, tpd_flag!=0);
        tpd_flag = 0;
        memset(touch_point, FTS_INVALID_DATA, sizeof(touch_point));
        set_current_state(TASK_RUNNING);

        //BEGIN <touch panel> <DATE20130831> <tp proximity> zhangxiaofei
        #if defined TPD_PROXIMITY
        if (tpd_proximity_flag == 1)
        {
            i2c_smbus_read_i2c_block_data(g_pts->client, TPD_PROXIMITY_ENABLE_REG, 1, &state);
            TPD_PROXIMITY_DBG("proxi_5206 0xB0 state value is 1131 0x%02X\n", state);

            if(!(state&0x01))
            {
                tpd_enable_ps(1);
            }

            i2c_smbus_read_i2c_block_data(g_pts->client, 0x01, 1, &proximity_status);
            TPD_PROXIMITY_DBG("proxi_5206 0x01 value is 1139 0x%02X\n", proximity_status);

            if (proximity_status == TPD_PROXIMITY_CLOSE_VALUE)
            {
                tpd_proximity_detect = 0;
            }
            else if(proximity_status == TPD_PROXIMITY_FARAWAY_VALUE)
            {
                tpd_proximity_detect = 1;
            }

            TPD_PROXIMITY_DBG("tpd_proximity_detect 1149 = %d\n", tpd_proximity_detect);

            if ((err = tpd_read_ps()))
            {
                TPD_PROXIMITY_DBG("proxi_5206 read ps data 1156: %d\n", err);
            }
            sensor_data.values[0] = tpd_get_ps_value();
            sensor_data.value_divide = 1;
            sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
            if ((err = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data)))
            {
                TPD_PROXIMITY_DBG(" proxi_5206 call hwmsen_get_interrupt_data failed= %d\n", err);
            }
        }
#endif
//END <touch panel> <DATE20130831> <tp proximity> zhangxiaofei
        //BEGIN <add changing flag> <DATE20130330> <add changing flag> zhangxiaofei
        if(g_tp_charger_flag != g_pre_tp_charger_flag){
            g_pre_tp_charger_flag = g_tp_charger_flag;
            fts_ft6x06_switch_charger_status(g_tp_charger_flag);
        }
        //END <add changing flag> <DATE20130330> <add changing flag> zhangxiaofei

        if (!tpd_touchinfo(ts, &touch_point)) {
            //report muti point then
            for ( i=0; i < TINNO_TOUCH_TRACK_IDS; i++ ){
                if ( FTS_INVALID_DATA != touch_point[i].x ){
                    if ( FTS_EF_UP == touch_point[i].flag ){
                        if( test_bit(i, &ts->fingers_flag) ){
                            tpd_up(ts, ts->touch_point_pre[i].x, ts->touch_point_pre[i].y,
                                touch_point[i].pressure, i);
                    }else{
                            CTP_DBG("This is a invalid up event.(%d)", i);
                        }
                    }else{//FTS_EF_CONTACT or FTS_EF_DOWN
                        if ( test_bit(i, &ts->fingers_flag)
                            && (FTS_EF_DOWN == touch_point[i].flag) ){
                            CTP_DBG("Ignore a invalid down event.(%d)", i);
                            continue;
                        }
                        tpd_down(ts, touch_point[i].x, touch_point[i].y,
                            touch_point[i].pressure, i);
                    }
                }else if (  test_bit(i, &ts->fingers_flag) ){
                    CTP_DBG("Complete a invalid down or move event.(%d)", i);
                    tpd_up(ts, ts->touch_point_pre[i].x, ts->touch_point_pre[i].y,
                        touch_point[i].pressure, i);
                }
            }
            input_sync(tpd->dev);
        }
        mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
    }while(!kthread_should_stop());
    mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
    return 0;
 }

static int tpd_detect (struct i2c_client *client, struct i2c_board_info *info)
{
    strcpy(info->type, TPD_DEVICE);
    return 0;
}

static void tpd_eint_interrupt_handler(void)
{
    if ( 0 == tpd_load_status ){
        return;
    }
    tpd_flag = 1;
    wake_up_interruptible(&waiter);
}

void fts_6x06_hw_reset(void)
{
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
    msleep(3);//mdelay(1);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
    msleep(3);//mdelay(15);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
    msleep(40);//mdelay(1);
}

static void fts_6x06_hw_init(void)
{
    hwPowerOn(touch_ssb_data.power_id, VOL_2800, "TP");

    hwPowerOn(MT6323_POWER_LDO_VGP1, VOL_2800, "TP");
    hwPowerOn(MT6323_POWER_LDO_VGP2, VOL_2800, "TP");
    hwPowerOn(MT6323_POWER_LDO_VGP3, VOL_2800, "TP");

    hwPowerOn(MT65XX_POWER_LDO_VGP1, VOL_2800, "TP");
    hwPowerOn(MT65XX_POWER_LDO_VGP2, VOL_2800, "TP");
    hwPowerOn(MT65XX_POWER_LDO_VGP3, VOL_2800, "TP");

    msleep(2);

    //Reset CTP
    mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
    msleep(30);
}

static char *fts_get_vendor_name(int vendor_id)
{
    switch(vendor_id){
        case FTS_CTP_VENDOR_BYD:          return "BYD";            break;
        case FTS_CTP_VENDOR_TRULY:          return "TRULY";        break;
        case FTS_CTP_VENDOR_NANBO:          return "NANBO";        break;
        case FTS_CTP_VENDOR_BAOMING:      return "BAOMING";        break;
        case FTS_CTP_VENDOR_JIEMIAN:      return "JIEMIAN";        break;
        case FTS_CTP_VENDOR_YEJI:          return "YEJI";        break;
        case FTS_CTP_VENDOR_HUARUICHUANG: return "HUARUICHUANG";break;
        case FTS_CTP_VENDOR_DEFAULT:      return "DEFAULT";        break;
        default:                          return "UNKNOWN";        break;
    }
    return "UNKNOWN";
}
//BEGIN<touch panel><date20131021><tp auto update>yinhuiyong
#ifdef  FTS_AUTO_TP_UPGRADE
static struct task_struct * focaltech_update_thread;
static  int update_firmware_thread(void *priv)
{
    TPD_DEBUG("[Focaltech] enter update_firmware_thread\n");
    //fts_ctpm_fw_upgrade_with_i_file();
    if( 0 == memcmp(tpd_desc, "HUARUICHUANG", 12))
    {
        ft6x06_tp_upgrade(ftbin_HRC, sizeof(ftbin_HRC));
    }else if(0 == memcmp(tpd_desc, "YEJI",4))
    {
        ft6x06_tp_upgrade(ftbin_YEJI, sizeof(ftbin_YEJI));
    }else if(0 == memcmp(tpd_desc, "JIEMIAN",7))
    {
        ft6x06_tp_upgrade(ftbin_JIEMIAN, sizeof(ftbin_JIEMIAN));
    }
    kthread_should_stop();
    return NULL;
}

int focaltech_auto_upgrade(void)
{
    int err;
    focaltech_update_thread = kthread_run(update_firmware_thread, 0, TPD_DEVICE);
    if (IS_ERR(focaltech_update_thread)) {
            err = PTR_ERR(focaltech_update_thread);
            TPD_DEBUG(TPD_DEVICE " failed to create update_firmware_thread thread: %d\n", err);
        }
    return err;
}
#endif
//END<touch panel><date20131021><tp auto update>yinhuiyong

 static int __devinit tpd_probe(struct i2c_client *client, const struct i2c_device_id *id)
 {
    int retval = TPD_OK;
    int panel_version = 0;
    int panel_vendor = 0;
    int iRetry = 3;
    tinno_ts_data *ts;
    int ret = 0;

    if ( tpd_load_status ){
        CTP_DBG("Already probed a TP, needn't to probe any more!");
        return -1;
    }

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        dev_err(&client->dev,"need I2C_FUNC_I2C");
        ret = -ENODEV;
        goto err_check_functionality_failed;
    }

    ts = kzalloc(sizeof(*ts), GFP_KERNEL);
    if (ts == NULL) {
        ret = -ENOMEM;
        goto err_alloc_data_failed;
    }

    CTP_DBG("TPD enter tpd_probe ts=0x%p, TPD_RES_X=%d, TPD_RES_Y=%d, addr=0x%x\n", ts, TPD_RES_X, TPD_RES_Y, client->addr);
    memset(ts, 0, sizeof(*ts));
    g_pts = ts;

    client->timing = I2C_MASTER_CLOCK;
    ts->client = client;
    ts->start_reg = 0x00;
    atomic_set( &ts->ts_sleepState, 0 );
    mutex_init(&ts->mutex);

    i2c_set_clientdata(client, ts);

    fts_6x06_hw_init();
    msleep(120);

    fts_iic_init(ts);

    if ( fts_6x06_isp_init(ts) ){
        goto err_isp_register;
    }

    while (iRetry) {
        ret = ft6x06_get_vendor_version(ts, &panel_vendor, &panel_version);
        if ( panel_version < 0 || panel_vendor<0 || ret<0 ){
            TPD_DMESG("Product version is %d\n", panel_version);
            fts_6x06_hw_reset();
        }else{
            break;
        }
        iRetry--;
        msleep(15);
    }
    if ( panel_version < 0 || panel_vendor<0 || ret<0 ){
        goto err_get_version;
    }

    if(touch_ssb_data.use_tpd_button == 1)
        tinno_update_tp_button_dim(panel_vendor);

#ifdef CONFIG_TOUCHSCREEN_FT5X05_DISABLE_KEY_WHEN_SLIDE
    if ( fts_keys_init(ts) ){
        fts_keys_deinit();
        goto err_get_version;
    }
#endif

    mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_EINT);
    mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_CTP_EINT_PIN, GPIO_PULL_ENABLE);
    mt_set_gpio_pull_select(GPIO_CTP_EINT_PIN, GPIO_PULL_UP);

    mt_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_TYPE, tpd_eint_interrupt_handler, 1);

    ts->thread = kthread_run(touch_event_handler, ts, TPD_DEVICE);
     if (IS_ERR(ts->thread)){
          retval = PTR_ERR(ts->thread);
          TPD_DMESG(TPD_DEVICE " failed to create kernel thread: %d\n", retval);
            goto err_start_touch_kthread;
    }

    tpd_load_status = 1;
    mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);

    CTP_DBG("Touch Panel Device(%s) Probe PASS\n", fts_get_vendor_name(panel_vendor));
//BEGIN <tp> <DATE20130507> <tp version> zhangxiaofei

    sprintf(tpd_desc, "%s", fts_get_vendor_name(panel_vendor));
    tpd_fw_version = panel_version;

//END <tp> <DATE20130507> <tp version> zhangxiaofei

//LINE<tp><DATE20130619><add for focaltech debug>zhangxiaofei
#ifdef FTS_CTL_IIC
        if (ft_rw_iic_drv_init(client) < 0)
            dev_err(&client->dev, "%s:[FTS] create fts control iic driver failed\n",
                    __func__);
#endif

//BEGIN <touch panel> <DATE20130831> <tp proximity> zhangxiaofei
#if defined TPD_PROXIMITY
    struct hwmsen_object obj_ps;
    int err=0;

    obj_ps.polling = 0;//interrupt mode
    obj_ps.sensor_operate = tpd_ps_operate;
    if((err = hwmsen_attach(ID_PROXIMITY, &obj_ps)))
    {
        APS_ERR("proxi_fts attach fail = %d\n", err);
    }
    else
    {
        APS_ERR("proxi_fts attach ok = %d\n", err);
    }
#endif
//END <touch panel> <DATE20130831> <tp proximity> zhangxiaofei

//BEGIN<touch panel><date20131028><tp auto update>yinhuiyong
#ifdef  FTS_AUTO_TP_UPGRADE
        focaltech_auto_upgrade();
#endif
//END<touch panel><date20131028><tp auto update>yinhuiyong
    return 0;

err_start_touch_kthread:
    mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
err_get_version:
err_isp_register:
  #ifdef CONFIG_TOUCHSCREEN_POWER_DOWN_WHEN_SLEEP
    hwPowerDown(MT65XX_POWER_LDO_VGP2, "touch");
  #endif
    fts_6x06_isp_exit();
    mutex_destroy(&ts->mutex);
    g_pts = NULL;
    kfree(ts);
err_alloc_data_failed:
err_check_functionality_failed:
    CTP_DBG("Touch Panel Device Probe FAIL\n");
    return -1;
 }

 static int __devexit tpd_remove(struct i2c_client *client)
{
    CTP_DBG("TPD removed\n");

//LINE<tp><DATE20130619><add for focaltech debug>zhangxiaofei
#ifdef FTS_CTL_IIC
    ft_rw_iic_drv_exit();
#endif
    return 0;
}

 static int tpd_local_init(void)
{
    TPD_DMESG("Focaltech FT6x06 I2C Touchscreen Driver (Built %s @ %s)\n", __DATE__, __TIME__);
    if(i2c_add_driver(&tpd_i2c_driver)!=0)
    {
        TPD_DMESG("unable to add i2c driver.\n");
        return -1;
    }

    if(touch_ssb_data.use_tpd_button == 1)
        tinno_update_tp_button_dim(FTS_CTP_VENDOR_NANBO);

    TPD_DMESG("end %s, %d\n", __FUNCTION__, __LINE__);
    tpd_type_cap = 1;
    return 0;
}

static void tpd_resume(struct early_suspend *h)
{
    char data;

#ifdef TPD_PROXIMITY
    if (tpd_proximity_flag == 1)
    {
        if(tpd_proximity_flag_one == 1)
        {
            tpd_proximity_flag_one = 0;
            TPD_DMESG(TPD_DEVICE " tpd_proximity_flag_one \n");
            return;
        }
    }
#endif

    if ( g_pts ){
        CTP_DBG("TPD wake up\n");
        if (atomic_read(&g_pts->isp_opened)){
            CTP_DBG("isp is already opened.");
            return;
        }

#ifdef CONFIG_TOUCHSCREEN_POWER_DOWN_WHEN_SLEEP
        fts_6x06_hw_init();
#else
//BEGIN <tp> <DATE20130507> <tp resume> zhangxiaofei
    // reset ctp
    mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
    msleep(10);
    mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
    msleep(200);//add this line
    CTP_DBG("TPD wake up done\n");
//END <tp> <DATE20130507> <tp resume> zhangxiaofei

#endif//CONFIG_TOUCHSCREEN_POWER_DOWN_WHEN_SLEEP
        mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
        atomic_set( &g_pts->ts_sleepState, 0 );
    }
 }

//Clear the unfinished touch event, simulate a up event if there this a pen down or move event.
void ft6x06_complete_unfinished_event( void )
{
    int i = 0;
    for ( i=0; i < TINNO_TOUCH_TRACK_IDS; i++ ){
        if (  test_bit(i, &g_pts->fingers_flag) ){
            tpd_up(g_pts, g_pts->touch_point_pre[i].x, g_pts->touch_point_pre[i].y,
                g_pts->touch_point_pre[i].pressure, i);
        }
    }
    input_sync(tpd->dev);
}

static void tpd_suspend(struct early_suspend *h)
 {
    int ret = 0;
    int iRetry = 5;
    const char data = 0x3;
//NEGIN <touch panel> <DATE20130831> <tp proximity> zhangxiaofei
#ifdef TPD_PROXIMITY
    if (tpd_proximity_flag == 1)
    {
        tpd_proximity_flag_one = 1;
        return;
    }
#endif
//END <touch panel> <DATE20130831> <tp proximity> zhangxiaofei
    if ( g_pts ){
         CTP_DBG("TPD enter sleep\n");
        if (atomic_read(&g_pts->isp_opened)){
            CTP_DBG("isp is already opened.");
            return;
        }

        mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);

#ifdef CONFIG_TOUCHSCREEN_FT5X05_DISABLE_KEY_WHEN_SLIDE
        fts_6x06_key_cancel();
#endif

#ifdef CONFIG_TOUCHSCREEN_POWER_DOWN_WHEN_SLEEP
    mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
    msleep(2);

#else //!CONFIG_TOUCHSCREEN_POWER_DOWN_WHEN_SLEEP
        //make sure the WakeUp is high before it enter sleep mode,
        //otherwise the touch can't be resumed.
        //mt_set_gpio_mode(GPIO_CTP_EN_PIN, GPIO_CTP_EN_PIN_M_GPIO);
        //mt_set_gpio_dir(GPIO_CTP_EN_PIN, GPIO_DIR_OUT);
        //mt_set_gpio_out(GPIO_CTP_EN_PIN, GPIO_OUT_ONE);
        //msleep(1);

        while (iRetry) {
            ret = i2c_smbus_write_i2c_block_data(g_pts->client, 0xA5, 1, &data);  //TP enter sleep mode
            if ( ret < 0 ){
                TPD_DMESG("Enter sleep mode is %d\n", ret);

                msleep(2);
                //fts_6x06_hw_init();
            }else{
                break;
            }
            iRetry--;
            msleep(100);
        }
#endif//CONFIG_TOUCHSCREEN_POWER_DOWN_WHEN_SLEEP
        atomic_set( &g_pts->ts_sleepState, 1 );
    }
 }



//BEGIN <touch panel> <DATE20130909> <touch panel version info> zhangxiaofei
extern int get_fw_version_ext(void);
//extern char tpd_desc[50];
int ft6x06_tpd_get_fw_version( void )
{
    return get_fw_version_ext();
}

void ft6x06_tpd_get_fw_vendor_name(char * fw_vendor_name)
{
    sprintf(fw_vendor_name, "%s", tpd_desc);
}
//END <touch panel> <DATE20130909> <touch panel version info> zhangxiaofei

 static struct tpd_driver_t tpd_device_driver = {
    .tpd_device_name = "FT6x06",
    .tpd_local_init = tpd_local_init,
    .suspend = tpd_suspend,
    .resume = tpd_resume,
#ifdef TPD_HAVE_BUTTON
    .tpd_have_button = 1,
#else
    .tpd_have_button = 0,
#endif
 };

 /* called when loaded into kernel */
 static int __init tpd_driver_init(void)
 {
    int err = 0;
    char name[20] = "ft6x06";
    printk("MediaTek FT6x06 touch panel driver init\n");
    err = tpd_ssb_data_match(name, &touch_ssb_data);
    if(err != 0){
        printk("touch tpd_ssb_data_match error\n");
        return -1;
    }
    printk("ft6x06 touch_ssb_data:: name:(%s), endflag:0x%x, i2c_number:0x%x, i2c_addr:0x%x,power_id:%d, use_tpd_button:%d\n",
    touch_ssb_data.identifier,
    touch_ssb_data.endflag,
    touch_ssb_data.i2c_number,
    touch_ssb_data.i2c_addr,
    touch_ssb_data.power_id,
    touch_ssb_data.use_tpd_button
    );

    ft6x06_i2c_tpd[0].addr = touch_ssb_data.i2c_addr;

    i2c_register_board_info(touch_ssb_data.i2c_number, &ft6x06_i2c_tpd, sizeof(ft6x06_i2c_tpd)/sizeof(ft6x06_i2c_tpd[0]));

    //add for ssb support
    tpd_device_driver.tpd_have_button = touch_ssb_data.use_tpd_button;

    if(tpd_driver_add(&tpd_device_driver) < 0)
        TPD_DMESG("add FT6x06 driver failed\n");
    return 0;
 }

 /* should never be called */
static void __exit tpd_driver_exit(void)
{
    TPD_DMESG("MediaTek FT6x06 touch panel driver exit\n");
    tpd_driver_remove(&tpd_device_driver);
}

 module_init(tpd_driver_init);
 module_exit(tpd_driver_exit);
