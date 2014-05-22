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

#ifndef TOUCHPANEL_H
#define TOUCHPANEL_H

#include <linux/wakelock.h>
#include <linux/ioctl.h>

#include <mach/mt_typedefs.h>

#define TPD_TYPE_CAPACITIVE
#define CONFIG_TOUCHSCREEN_FT6X05_DISABLE_KEY_WHEN_SLIDE

//#define CONFIG_TOUCHSCREEN_POWER_DOWN_WHEN_SLEEP
//#define TPD_PROXIMITY

#if (defined(TPD_TYPE_CAPACITIVE))
#define TPD_POWER_SOURCE        MT65XX_POWER_COUNT_END
#define TPD_I2C_NUMBER           0
#define TPD_WAKEUP_TRIAL         15
#define TPD_WAKEUP_DELAY         100
#endif

#define TPD_HAVE_BUTTON
#ifdef TPD_HAVE_BUTTON

#define TPD_YMAX_NB            900
#define TPD_YMAX_BYD        900  //LINE <tp> <DATE20130508> <TP YMAX> zhangxiaofei
#define TPD_BUTTON_HEIGHT    854
#define TPD_Y_OFFSET        6
#define DEBUG_ON            1

#define KEYCODE_APP_SWITCH KEY_F17

#define TPD_KEY_COUNT           3
#define TPD_KEYS                {KEY_MENU, KEY_HOMEPAGE, KEY_BACK}

#define TPD_BUTTON_SIZE_HEIGHT_NB  (TPD_YMAX_NB - TPD_BUTTON_HEIGHT - TPD_Y_OFFSET)
#define TPD_BUTTON_Y_CENTER_NB       (TPD_BUTTON_HEIGHT + (TPD_YMAX_NB - TPD_BUTTON_HEIGHT)/2 + TPD_Y_OFFSET)

#define TPD_BUTTON_SIZE_HEIGHT_BYD  (TPD_YMAX_BYD - TPD_BUTTON_HEIGHT - TPD_Y_OFFSET)
#define TPD_BUTTON_Y_CENTER_BYD       (TPD_BUTTON_HEIGHT + (TPD_YMAX_BYD - TPD_BUTTON_HEIGHT)/2 + TPD_Y_OFFSET)


#define TPD_POWER_SOURCE_CUSTOM     MT6323_POWER_LDO_VGP1

//TP virtual key customization

// |                                                                                                                       |
// |                                                                                                                       |  Touch Pad area ( H < TPD_BUTTON_HEIGHT)
//  ---------------------------------------------------------------------------------------------------
// |                                           TPD_Y_OFFSET                                                       |  Virtual key area ( H > TPD_BUTTON_HEIGHT)
// |---------------------------------------------------------------------------------------------------
// |TPD_B1_FP | [TPD_B1_W] | TPD_B2_FP | [TPD_B2_W] | TPD_B3_FP | [TPD_B3_W]   |
// -----------------------------------------------------------------------------------------------------

//BEGIN <S6030> <DATE20121220> <S6030 TP> zhangxiaofei
  #define TPD_B1_FP    40    //35        //Button 1 pad space
  #define TPD_B1_W    100    //50        //Button 1 Width
  #define TPD_B2_FP    40    //50        //Button 2 pad space
  #define TPD_B2_W    100    //50        //Button 2 Width
  #define TPD_B3_FP    40    //50        //Button 3 pad space
  #define TPD_B3_W    100    //50        //Button 3 Width
//END <S6030> <DATE20121220> <S6030 TP> zhangxiaofei

//-------------------------------------------------------------------------
#define TPD_BUTTON1_X_CENTER    TPD_B1_FP + TPD_B1_W/2
#define TPD_BUTTON2_X_CENTER    TPD_B1_FP + TPD_B1_W + TPD_B2_FP + TPD_B2_W/2
#define TPD_BUTTON3_X_CENTER    TPD_B1_FP + TPD_B1_W + TPD_B2_FP + TPD_B2_W + TPD_B3_FP + TPD_B3_W/2


#define TPD_KEYS_DIM_NB    {{TPD_BUTTON1_X_CENTER, TPD_BUTTON_Y_CENTER_NB, TPD_B1_W, TPD_BUTTON_SIZE_HEIGHT_NB},    \
                             {TPD_BUTTON2_X_CENTER, TPD_BUTTON_Y_CENTER_NB, TPD_B2_W, TPD_BUTTON_SIZE_HEIGHT_NB},    \
                            {TPD_BUTTON3_X_CENTER, TPD_BUTTON_Y_CENTER_NB, TPD_B3_W, TPD_BUTTON_SIZE_HEIGHT_NB}}

#define TPD_KEYS_DIM_BYD  {{TPD_BUTTON1_X_CENTER, TPD_BUTTON_Y_CENTER_BYD, TPD_B1_W, TPD_BUTTON_SIZE_HEIGHT_BYD},    \
                             {TPD_BUTTON2_X_CENTER, TPD_BUTTON_Y_CENTER_BYD, TPD_B2_W, TPD_BUTTON_SIZE_HEIGHT_BYD},    \
                            {TPD_BUTTON3_X_CENTER, TPD_BUTTON_Y_CENTER_BYD, TPD_B3_W, TPD_BUTTON_SIZE_HEIGHT_BYD}}
extern void tpd_button(unsigned int x, unsigned int y, unsigned int down) ;

#endif

#define TPD_I2C_GROUP_ID   0

//For Android 4.0
#define TPD_I2C_SLAVE_ADDR (0x70 >> 1)
//#define TPD_I2C_SLAVE_ADDR (0x72 >> 1)

#define MAX_TRANSACTION_LENGTH 8
#define MAX_I2C_TRANSFER_SIZE 6
#define I2C_MASTER_CLOCK       200

#define FTS_EF_DOWN (0)
#define FTS_EF_UP (1)
#define FTS_EF_CONTACT (2)
#define FTS_EF_RESERVED (3)

#define FTS_INVALID_DATA (-1)

#define FTS_IS_TOUCH(p) ( (FTS_EF_DOWN==(p)) || (FTS_EF_CONTACT==(p)) )

//LINE <ft6x06> <DATE20130507> <modify for ft6x06 touch panel driver> zhangxiaofei
//#define TINNO_TOUCH_TRACK_IDS 5
#define TINNO_TOUCH_TRACK_IDS 2

typedef struct _tinno_ts_point{
    int x, y, pressure, flag;
}tinno_ts_point;

typedef struct {
    uint8_t    x_h: 4,
        reserved_1: 2,
        event_flag: 2;
    uint8_t    x_l;
    uint8_t    y_h: 4,
        touch_id: 4;
    uint8_t    y_l;
    uint8_t pressure;
    uint8_t speed: 2,
        direction: 2,
        aera:4;
} xy_data_t;

typedef struct {
    uint8_t    reserved_1: 4,
        device_mode: 3,
        reserved_2: 1;
    uint8_t    gesture;
    uint8_t    fingers: 4,
        frame_remaining: 4;
    xy_data_t     xy_data[TINNO_TOUCH_TRACK_IDS];
} fts_report_data_t;

#define FTS_PROTOCOL_LEN (sizeof(fts_report_data_t))

/* Touch Key State */
typedef enum {
    TKS_IDLE,
    TKS_DOWNED,
    TKS_MOVING,
    TKS_UPPED,
} key_state_t;

typedef struct {
    uint8_t start_reg;
    uint8_t buffer[FTS_PROTOCOL_LEN];
    struct i2c_client *client;
    unsigned long fingers_flag;
    uint8_t last_fingers;
    tinno_ts_point touch_point_pre[TINNO_TOUCH_TRACK_IDS];
    struct task_struct *thread;
    atomic_t isp_opened;
    atomic_t ts_sleepState;
    uint8_t *isp_pBuffer;
    struct wake_lock wake_lock;
    struct mutex mutex;
    struct input_dev *keys_dev;
    key_state_t key_state;
    int mLastKeyCode;
    struct task_struct *thread_isp;
    int isp_code_count;
}tinno_ts_data;


#define FTS_MODE_OPRATE (0x00)
#define FTS_MODE_UPDATE (0x01)
#define FTS_MODE_SYSTEM (0x02)

#define TOUCH_IO_MAGIC ('F')
#define FT6X06_IOCTL_RESET                 _IO(TOUCH_IO_MAGIC, 0x00)
#define FT6X06_IOCTL_SWITCH_TO         _IOW(TOUCH_IO_MAGIC, 0x01, int)
#define FT6X06_IOCTL_WRITE_PROTECT     _IOW(TOUCH_IO_MAGIC, 0x02, int)
#define FT6X06_IOCTL_ERASE                 _IO(TOUCH_IO_MAGIC, 0x03)
#define FT6X06_IOCTL_GET_STATUS        _IOR(TOUCH_IO_MAGIC, 0x04, int)
#define FT6X06_IOCTL_GET_CHECKSUM    _IOR(TOUCH_IO_MAGIC, 0x05, int)
#define FT6X06_IOCTL_GET_TPID            _IOR(TOUCH_IO_MAGIC, 0x06, int)
#define FT6X06_IOCTL_AUTO_CAL             _IO(TOUCH_IO_MAGIC, 0x07)
#define FT6X06_IOCTL_GET_VENDOR_VERSION _IOR(TOUCH_IO_MAGIC, 0x08, int)
#define FT6X06_IOC_MAXNR                (0x09)

//LINE <ft6x06> <DATE20130507> <modify for ft6x06 touch panel driver> zhangxiaofei
#define FTS_CTP_FIRWARE_ID (0x7908)   //0x7903---FT6X06, 0x7907---FT5X16

#define FTS_CTP_VENDOR_BYD          (0x59)
#define FTS_CTP_VENDOR_TRULY        (0x5A)
#define FTS_CTP_VENDOR_NANBO        (0x5B)
#define FTS_CTP_VENDOR_BAOMING      (0x5D)
#define FTS_CTP_VENDOR_JIEMIAN      (0x8B)
#define FTS_CTP_VENDOR_YEJI         (0x80)
#define FTS_CTP_VENDOR_HUARUICHUANG (0x43)
#define FTS_CTP_VENDOR_DEFAULT      (0x79)

int fts_6x06_isp_init( tinno_ts_data *ts);
void fts_6x06_isp_exit(void);
void fts_6x06_hw_reset(void);
int fts_6x06_key_cancel(void);
int fts_6x06_parase_keys(tinno_ts_data *ts, fts_report_data_t *pReportData);
int fts_keys_init(tinno_ts_data *ts);
void fts_keys_deinit(void);
int fts_iic_init( tinno_ts_data *ts );
int tpd_read_touchinfo(tinno_ts_data *ts);
int fts_write_reg(u8 addr, u8 para);
int fts_read_reg(u8 addr, unsigned char *pdata);
int fts_i2c_write_block( u8 *txbuf, int len );
u8 fts_cmd_write(u8 btcmd,u8 btPara1,u8 btPara2,u8 btPara3,u8 num);
int ft6x06_get_vendor_version(tinno_ts_data *ts, uint8_t *pfw_vendor, uint8_t *pfw_version);
void ft6x06_complete_unfinished_event( void );
//LINE <add changing flag> <DATE20130330> <add changing flag> zhangxiaofei
int fts_ft6x06_switch_charger_status(kal_uint8 charger_flag);

//line<touch panel><date20131021><tp auto update>yinhuiyong
void ft6x06_tp_upgrade(const char * ftbin_buf, int buf_len);
#if DEBUG_ON
#define CTP_DBG(fmt, arg...) \
    printk("[CTP-FT6X06] %s (line:%d) :" fmt "\r\n", __func__, __LINE__, ## arg)
#define TPD_PROXIMITY_DBG(fmt, arg...) \
    printk("[CTP-proxi_6x06] %s (line:%d) :" fmt "\r\n", __func__, __LINE__, ## arg)
#else
#define CTP_DBG(fmt, arg...) do {} while (0)
#define TPD_PROXIMITY_DBG(fmt, arg...) do {} while (0)
#endif
#endif

