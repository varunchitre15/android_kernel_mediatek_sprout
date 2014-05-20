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

/*****************************************************************************
 *
 * Filename:
 * ---------
 *   sensor.c
 *   
 * Project:
 * --------
 *   DUMA
 *
 * Description:
 * ------------
 *   Source code of Sensor driver
 *      
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/atomic.h>
#include <asm/uaccess.h> 
#include <linux/miscdevice.h>
//#include <mach/mt6516_pll.h>
#include <linux/kernel.h>//for printk

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"


#include "s5k5eayx_yuv_Sensor.h"
#include "s5k5eayx_yuv_Camera_Sensor_para.h"
#include "s5k5eayx_yuv_CameraCustomized.h"


#define S5K5EAYXYUV_DEBUG
#ifdef S5K5EAYXYUV_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif

#define S5K5EA_PRE_AFRangeWidth (1280)
#define S5K5EA_PRE_AFRangeHeight (960)
#define S5K5EA_CAP_AFRangeWidth (2560)
#define S5K5EA_CAP_AFRangeHeight (1920)

#define S5K5EA_PRV_RATIO_WIDTH (240)
#define S5K5EA_PRV_RATIO_HEIGHT (240)
#define S5K5EA_CAP_RATIO_WIDTH (480)
#define S5K5EA_CAP_RATIO_HEIGHT (480)
#define S5K5EA_FAF_TOLERANCE    (40)

#define S5K5EA_READ_SHUTTER_RATIO (4)
#define S5K5EA_SET_SHUTTER_RATIO (1)
#define S5K5EA_SET_GAIN_RATIO (1)

#define S5K5EZYX_NightMode_Off (0)
#define S5K5EZYX_NightMode_On (1)

kal_bool S5K5EAYX_video_mode = KAL_FALSE;
kal_uint32 zoom_factor = 0; 

typedef enum
{
    S5K5EAYX_SENSORMODE_PREVIEW=0,
	S5K5EAYX_SENSORMODE_VIDEO,
	S5K5EAYX_SENSORMODE_CAPTURE,
	S5K5EAYX_SENSORMODE_ZSD,
} SensorMode;
SensorMode s5k5eayx_sensor_mode = S5K5EAYX_SENSORMODE_PREVIEW;

typedef enum
{
    S5K5EAYX_CAP_AEAWB_CLOSE=0,
	S5K5EAYX_CAP_AEAWB_OPEN,
} S5K5EAYX_CAP_AEAWB_STATUS;
S5K5EAYX_CAP_AEAWB_STATUS s5k5eayx_cap_aeawb_status = S5K5EAYX_CAP_AEAWB_CLOSE;


static MSDK_SENSOR_CONFIG_STRUCT S5K5EAYXSensorConfigData;
kal_uint8 S5K5EAYXYUV_sensor_write_I2C_address = S5K5EAYX_WRITE_ID;
kal_uint8 S5K5EAYXYUV_sensor_read_I2C_address = S5K5EAYX_READ_ID;

struct S5K5EAYX_sensor_struct S5K5EAYX_Sensor_Driver;
MSDK_SCENARIO_ID_ENUM S5K5EAYXCurrentScenarioId = MSDK_SCENARIO_ID_CAMERA_PREVIEW;


typedef struct
{
  UINT16  iSensorVersion;
  UINT16  iNightMode;
  UINT16  iSceneMode;
  UINT16  iWB;
  UINT16  iEffect;
  UINT16  iEV;
  UINT16  iBanding;
  UINT16  iMirror;
  UINT16  iFrameRate;
  kal_uint32  iShutter;
  kal_uint32  iGain;
  kal_bool HDRFixed_AE_Done;
} S5K5EAYX_Status;
S5K5EAYX_Status S5K5EAYX_CurrentStatus;


extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
extern int iMultiWriteReg(u8 *pData, u16 lens, u16 i2cId);

static DEFINE_SPINLOCK(s5k5eayx_drv_lock);


static kal_uint16 S5K5EAYX_write_cmos_sensor_wID(kal_uint32 addr, kal_uint32 para, kal_uint32 id)
{
	char puSendCmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para >> 8),(char)(para & 0xFF)};
	
	iWriteRegI2C(puSendCmd , 4,id);

}
static kal_uint16 S5K5EAYX_read_cmos_sensor_wID(kal_uint32 addr, kal_uint32 id)
{
	kal_uint16 get_byte=0;
    char puSendCmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,2,id);
    return ((get_byte<<8)&0xff00)|((get_byte>>8)&0x00ff);
}

static kal_uint16 S5K5EAYX_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
    char puSendCmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,2,S5K5EAYXYUV_sensor_write_I2C_address);
    return ((get_byte<<8)&0xff00)|((get_byte>>8)&0x00ff);
}
static kal_uint16 S5K5EAYX_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char puSendCmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para >> 8),(char)(para & 0xFF)};
	
	iWriteRegI2C(puSendCmd , 4,S5K5EAYXYUV_sensor_write_I2C_address);
}


#define FLASH_BV_THRESHOLD 0x000A
static void S5K5EAYX_FlashTriggerCheck(unsigned int *pFeatureReturnPara32)
{
    unsigned int NormBr;

    S5K5EAYX_write_cmos_sensor(0x002C,0x2000);
    S5K5EAYX_write_cmos_sensor(0x002E,0x2188);
    NormBr = S5K5EAYX_read_cmos_sensor(0x0F12);

    SENSORDB(" S5K5EAYX_FlashTriggerCheck NormBr=%x\n",NormBr);
	
    if (NormBr > FLASH_BV_THRESHOLD)
    {
       *pFeatureReturnPara32 = FALSE;
        return;
    }

    *pFeatureReturnPara32 = TRUE;
    return;
}

static void S5K5EAYX_InitialPara(void)
{
  spin_lock(&s5k5eayx_drv_lock);

  S5K5EAYX_CurrentStatus.iNightMode = 0;
  S5K5EAYX_CurrentStatus.iSceneMode = SCENE_MODE_OFF;
  S5K5EAYX_CurrentStatus.iWB = AWB_MODE_AUTO;
  S5K5EAYX_CurrentStatus.iEffect = MEFFECT_OFF;
  S5K5EAYX_CurrentStatus.iBanding = AE_FLICKER_MODE_50HZ;
  S5K5EAYX_CurrentStatus.iEV = AE_EV_COMP_00;
  S5K5EAYX_CurrentStatus.iMirror = IMAGE_NORMAL;
  S5K5EAYX_CurrentStatus.iFrameRate = 0;
  S5K5EAYX_CurrentStatus.iShutter= 0;
  S5K5EAYX_CurrentStatus.iGain= 0;
  S5K5EAYX_CurrentStatus.HDRFixed_AE_Done=KAL_FALSE;
  
  S5K5EAYX_video_mode = KAL_FALSE;
  s5k5eayx_sensor_mode = S5K5EAYX_SENSORMODE_PREVIEW;
  spin_unlock(&s5k5eayx_drv_lock);
  
}


static void S5K5EAYX_Init_Setting(void)
{
    SENSORDB("[5EA] :S5K5EAYX_Init_Setting \n");
    // FOR 5EA EVT1.1
    S5K5EAYX_write_cmos_sensor(0xFCFC, 0xD000);
    S5K5EAYX_write_cmos_sensor(0x0010, 0x0001);
    S5K5EAYX_write_cmos_sensor(0x1030, 0x0000);
    S5K5EAYX_write_cmos_sensor(0x0014, 0x0001);
    mdelay(50);

    S5K5EAYX_write_cmos_sensor(0x0028, 0x2000);
    S5K5EAYX_write_cmos_sensor(0x002A, 0x31E4);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xB570);    // 200031E4         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4E17);    // 200031E6       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x257F);    // 200031E8       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4C17);    // 200031EA       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 200031EC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF000);    // 200031EE       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFA69);   // 200031F0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x6266);    // 200031F2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x62A5);    // 200031F4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4C16);    // 200031F6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4814);    // 200031F8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x63E0);    // 200031FA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4816);    // 200031FC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4915);    // 200031FE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x6101);    // 20003200 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4916);    // 20003202 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4816);    // 20003204 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF000);    // 20003206 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFA63);    // 20003208 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4916);    // 2000320A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4816);    // 2000320C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF000);    // 2000320E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFA5F);    // 20003210 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4916);    // 20003212 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4816);    // 20003214 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF000);    // 20003216 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFA5B);    // 20003218 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4916);    // 2000321A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 2000321C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8008);    // 2000321E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4915);    // 20003220 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4816);    // 20003222 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF000);    // 20003224 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFA54);    // 20003226 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4912);    // 20003228 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1D09);    // 2000322A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x6008);    // 2000322C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4620);    // 2000322E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4913);    // 20003230 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3080);    // 20003232 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x6141);    // 20003234 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4813);    // 20003236 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x67E0);    // 20003238 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4913);    // 2000323A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4813);    // 2000323C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF000);    // 2000323E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFA47);    // 20003240 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBD70);    // 20003242 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // 20003244 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5EA1);    // 20003246 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2F34);    // 20003248 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 2000324A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3299);    // 2000324C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 2000324E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0008);    // 20003250 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 20003252 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x32FF);    // 20003254 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 20003256 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0148);    // 20003258 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 2000325A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x33A3);    // 2000325C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 2000325E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5361);    // 20003260 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // 20003262 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x33C3);    // 20003264 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 20003266 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xCCC1);    // 20003268 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // 2000326A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x34C5);    // 2000326C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 2000326E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x67D7);    // 20003270 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // 20003272 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3798);    // 20003274 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 20003276 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3521);    // 20003278 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 2000327A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x745B);    // 2000327C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // 2000327E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x35D3);    // 20003280 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 20003282 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x360F);    // 20003284 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 20003286 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3637);    // 20003288 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 2000328A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x50F9);    // 2000328C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // 2000328E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBA40);    // 20003290 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4770);    // 20003292 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBAC0);    // 20003294 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4770);    // 20003296 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xB570);    // 20003298 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x48F5);    // 2000329A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8C00);    // 2000329C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x07C0);    // 2000329E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD028);    // 200032A0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x49F4);    // 200032A2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8848);    // 200032A4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8809);    // 200032A6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0400);    // 200032A8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4308);    // 200032AA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4CF2);    // 200032AC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8A25);    // 200032AE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4AF2);    // 200032B0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2101);    // 200032B2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8151);    // 200032B4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4621);    // 200032B6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3920);    // 200032B8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x6989);    // 200032BA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x233D);    // 200032BC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5C5E);    // 200032BE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4BEF);    // 200032C0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2E00);    // 200032C2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD005);    // 200032C4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3140);    // 200032C6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8B89);    // 200032C8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2902);    // 200032CA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD015);    // 200032CC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2100);    // 200032CE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8219);    // 200032D0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4EEB);    // 200032D2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2100);    // 200032D4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3620);    // 200032D6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8031);    // 200032D8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03C6);    // 200032DA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C36);    // 200032DC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x825E);    // 200032DE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8151);    // 200032E0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2106);    // 200032E2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF000);    // 200032E4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF9FA);    // 200032E6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8220);    // 200032E8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x207D);    // 200032EA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0180);    // 200032EC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF000);    // 200032EE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF9FB);    // 200032F0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8225);    // 200032F2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF000);    // 200032F4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF9FE);    // 200032F6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBD70);    // 200032F8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2101);    // 200032FA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xE7E8);    // 200032FC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xB5F8);    // 200032FE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x49E0);    // 20003300 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x880A);    // 20003302 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2A00);    // 20003304 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD114);    // 20003306 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4DDC);    // 20003308 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x89AA);    // 2000330A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2A00);    // 2000330C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD010);    // 2000330E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2700);    // 20003310 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2800);    // 20003312 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD00E);    // 20003314 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8848);    // 20003316 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF000);    // 20003318 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF9E6);    // 2000331A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x48D8);    // 2000331C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3020);    // 2000331E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8007);    // 20003320 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x200A);    // 20003322 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF000);    // 20003324 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF9E0);    // 20003326 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2001);    // 20003328 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8168);    // 2000332A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x200A);    // 2000332C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF000);    // 2000332E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF9DB);    // 20003330 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBDF8);    // 20003332 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x48CF);    // 20003334 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8841);    // 20003336 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8800);    // 20003338 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0409);    // 2000333A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4301);    // 2000333C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0348);    // 2000333E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C00);    // 20003340 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4CCD);    // 20003342 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8A26);    // 20003344 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x49CE);    // 20003346 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8248);    // 20003348 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x816F);    // 2000334A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4ACC);    // 2000334C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2101);    // 2000334E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3220);    // 20003350 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8011);    // 20003352 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8220);    // 20003354 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x207D);    // 20003356 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00C0);    // 20003358 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF000);    // 2000335A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF9C5);    // 2000335C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8226);    // 2000335E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBDF8);    // 20003360 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xB570);    // 20003362 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4605);    // 20003364 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4CC8);    // 20003366 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8820);    // 20003368 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2800);    // 2000336A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD10E);    // 2000336C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x48C7);    // 2000336E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2108);    // 20003370 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8041);    // 20003372 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2601);    // 20003374 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8006);    // 20003376 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF000);    // 20003378 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF9C2);    // 2000337A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x48BE);    // 2000337C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x49C4);    // 2000337E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8201);    // 20003380 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x48C4);    // 20003382 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8BC0);    // 20003384 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF000);    // 20003386 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF9AF);    // 20003388 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8026);    // 2000338A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x48BF);    // 2000338C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3040);    // 2000338E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8880);    // 20003390 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2D00);    // 20003392 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD002);    // 20003394 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2102);    // 20003396 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4008);    // 20003398 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBD70);    // 2000339A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x07C0);    // 2000339C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0FC0);    // 2000339E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBD70);    // 200033A0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xB570);    // 200033A2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4CB9);    // 200033A4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3460);    // 200033A6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8A25);    // 200033A8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2004);    // 200033AA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4385);    // 200033AC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 200033AE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF7FF);    // 200033B0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFD7);    // 200033B2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0080);    // 200033B4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4328);    // 200033B6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8220);    // 200033B8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2021);    // 200033BA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0200);    // 200033BC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8060);    // 200033BE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBD70);    // 200033C0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xB5F8);    // 200033C2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2001);    // 200033C4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF7FF);    // 200033C6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFCC);    // 200033C8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4CB2);    // 200033CA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2101);    // 200033CC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3420);    // 200033CE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2802);    // 200033D0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD01C);    // 200033D2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 200033D4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x81A0);    // 200033D6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x48AC);    // 200033D8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x89A2);    // 200033DA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8042);    // 200033DC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2101);    // 200033DE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8001);    // 200033E0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4FAC);    // 200033E2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8BF8);    // 200033E4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF000);    // 200033E6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF97F);    // 200033E8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4EA8);    // 200033EA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3620);    // 200033EC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8A71);    // 200033EE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x48A9);    // 200033F0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4DAA);    // 200033F2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2900);    // 200033F4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD002);    // 200033F6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8902);    // 200033F8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2A00);    // 200033FA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD009);    // 200033FC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2101);    // 200033FE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03C9);    // 20003400 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8069);    // 20003402 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x80E9);    // 20003404 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2100);    // 20003406 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x80A9);    // 20003408 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8129);    // 2000340A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xE01E);    // 2000340C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x81A1);    // 2000340E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xE7E2);    // 20003410 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8069);    // 20003412 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8AB1);    // 20003414 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x80E9);    // 20003416 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8AF1);    // 20003418 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xB209);    // 2000341A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x80A9);    // 2000341C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8B32);    // 2000341E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xB212);    // 20003420 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x812A);    // 20003422 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8B73);    // 20003424 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x469C);    // 20003426 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8BB3);    // 20003428 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x469E);    // 2000342A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4663);    // 2000342C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2B00);    // 2000342E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD104);    // 20003430 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x17CB);    // 20003432 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0F5B);    // 20003434 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1859);    // 20003436 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x10C9);    // 20003438 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x80A9);    // 2000343A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4671);    // 2000343C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2900);    // 2000343E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD104);    // 20003440 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x17D1);    // 20003442 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0F49);    // 20003444 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1889);    // 20003446 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x10C9);    // 20003448 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8129);    // 2000344A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8940);    // 2000344C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2800);    // 2000344E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD006);    // 20003450 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2001);    // 20003452 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0280);    // 20003454 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x81A8);    // 20003456 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8168);    // 20003458 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8228);    // 2000345A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x81E8);    // 2000345C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBDF8);    // 2000345E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2001);    // 20003460 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x81A0);    // 20003462 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4989);    // 20003464 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8048);    // 20003466 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8008);    // 20003468 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8BF8);    // 2000346A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF000);    // 2000346C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF93C);    // 2000346E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8BF0);    // 20003470 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2800);    // 20003472 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD009);    // 20003474 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x81A8);    // 20003476 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4884);    // 20003478 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3040);    // 2000347A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8801);    // 2000347C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8169);    // 2000347E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8841);    // 20003480 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8229);    // 20003482 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8880);    // 20003484 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x81E8);    // 20003486 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBDF8);    // 20003488 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 2000348A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x81A0);    // 2000348C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x497F);    // 2000348E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8048);    // 20003490 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2001);    // 20003492 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8008);    // 20003494 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8BF8);    // 20003496 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF000);    // 20003498 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF926);    // 2000349A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8BF0);    // 2000349C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2800);    // 2000349E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD009);    // 200034A0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x81A8);    // 200034A2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4879);    // 200034A4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3040);    // 200034A6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8801);    // 200034A8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8169);    // 200034AA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8841);    // 200034AC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8229);    // 200034AE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8880);    // 200034B0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x81E8);    // 200034B2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBDF8);    // 200034B4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2001);    // 200034B6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0280);    // 200034B8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x81A8);    // 200034BA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8168);    // 200034BC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8228);    // 200034BE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x81E8);    // 200034C0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBDF8);    // 200034C2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xB5F3);    // 200034C4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xB091);    // 200034C6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4607);    // 200034C8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2220);    // 200034CA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4974);    // 200034CC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xA809);    // 200034CE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF000);    // 200034D0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF91C);    // 200034D2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2220);    // 200034D4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4972);    // 200034D6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3120);    // 200034D8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xA801);    // 200034DA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF000);    // 200034DC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF916);    // 200034DE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2400);    // 200034E0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4E70);    // 200034E2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00A0);    // 200034E4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xAA01);    // 200034E6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5E11);    // 200034E8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1883);    // 200034EA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2202);    // 200034EC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5E9A);    // 200034EE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x9B12);    // 200034F0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4379);    // 200034F2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x435A);    // 200034F4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1889);    // 200034F6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x17CA);    // 200034F8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0E92);    // 200034FA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1851);    // 200034FC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1189);    // 200034FE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xAA09);    // 20003500 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5A13);    // 20003502 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x199D);    // 20003504 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1880);    // 20003506 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8842);    // 20003508 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x7828);    // 2000350A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1840);    // 2000350C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2100);    // 2000350E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF000);    // 20003510 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF902);    // 20003512 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x7028);    // 20003514 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1C64);    // 20003516 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2C08);    // 20003518 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD3E3);    // 2000351A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xB013);    // 2000351C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBDF0);    // 2000351E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xB5F3);    // 20003520 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xB081);    // 20003522 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4C58);    // 20003524 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4A60);    // 20003526 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1D24);    // 20003528 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x6820);    // 2000352A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2701);    // 2000352C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0901);    // 2000352E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0049);    // 20003530 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0705);    // 20003532 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5A53);    // 20003534 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0F2D);    // 20003536 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4638);    // 20003538 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x40A8);    // 2000353A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4383);    // 2000353C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5253);    // 2000353E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x484B);    // 20003540 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x78C0);    // 20003542 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2800);    // 20003544 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD032);    // 20003546 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4858);    // 20003548 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4D59);    // 2000354A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8D81);    // 2000354C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2900);    // 2000354E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD025);    // 20003550 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x88A0);    // 20003552 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2800);    // 20003554 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD122);    // 20003556 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4A54);    // 20003558 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x6DA8);    // 2000355A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8DD2);    // 2000355C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C03);    // 2000355E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD006);    // 20003560 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A03);    // 20003562 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4359);    // 20003564 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x60A1);    // 20003566 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x60E0);    // 20003568 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4353);    // 2000356A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x6123);    // 2000356C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xE006);    // 2000356E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4341);    // 20003570 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A09);    // 20003572 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x60A1);    // 20003574 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x60E0);    // 20003576 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4350);    // 20003578 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A00);    // 2000357A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x6120);    // 2000357C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x6920);    // 2000357E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x65A8);    // 20003580 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4941);    // 20003582 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x9801);    // 20003584 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3128);    // 20003586 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF000);    // 20003588 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF8CC);    // 2000358A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4629);    // 2000358C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3158);    // 2000358E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xC94E);    // 20003590 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x483D);    // 20003592 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3018);    // 20003594 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xC04E);    // 20003596 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x68E0);    // 20003598 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x65A8);    // 2000359A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x80A7);    // 2000359C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x88A0);    // 2000359E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2802);    // 200035A0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD106);    // 200035A2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x68A0);    // 200035A4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x65A8);    // 200035A6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2003);    // 200035A8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x80A0);    // 200035AA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xE001);    // 200035AC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 200035AE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x80A0);    // 200035B0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x9902);    // 200035B2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x9801);    // 200035B4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF000);    // 200035B6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF8B5);    // 200035B8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x6820);    // 200035BA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4B3A);    // 200035BC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0901);    // 200035BE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0049);    // 200035C0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0704);    // 200035C2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5A5A);    // 200035C4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0F24);    // 200035C6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4638);    // 200035C8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x40A0);    // 200035CA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4302);    // 200035CC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x525A);    // 200035CE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBDFE);    // 200035D0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xB570);    // 200035D2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2001);    // 200035D4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF000);    // 200035D6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF8AB);    // 200035D8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4C2B);    // 200035DA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1D24);    // 200035DC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x88A0);    // 200035DE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2801);    // 200035E0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD10F);    // 200035E2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4620);    // 200035E4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3024);    // 200035E6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF000);    // 200035E8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF8A8);    // 200035EA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1F21);    // 200035EC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4830);    // 200035EE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x6A0D);    // 200035F0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x69CB);    // 200035F2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x698A);    // 200035F4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x6A49);    // 200035F6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x6605);    // 200035F8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x65C3);    // 200035FA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x6582);    // 200035FC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x6641);    // 200035FE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2002);    // 20003600 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x80A0);    // 20003602 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4929);    // 20003604 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8E08);    // 20003606 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1C40);    // 20003608 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8608);    // 2000360A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBD70);    // 2000360C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xB570);    // 2000360E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4604);    // 20003610 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4D28);    // 20003612 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x7828);    // 20003614 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4320);    // 20003616 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD00C);    // 20003618 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x20C1);    // 2000361A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4622);    // 2000361C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2101);    // 2000361E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0200);    // 20003620 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF000);    // 20003622 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF891);    // 20003624 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2C00);    // 20003626 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD103);    // 20003628 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x20FF);    // 2000362A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4922);    // 2000362C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3001);    // 2000362E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8048);    // 20003630 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x702C);    // 20003632 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBD70);    // 20003634 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xB5F8);    // 20003636 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4605);    // 20003638 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x460E);    // 2000363A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4617);    // 2000363C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x461C);    // 2000363E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x480D);    // 20003640 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2181);    // 20003642 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3820);    // 20003644 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x6980);    // 20003646 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5C09);    // 20003648 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0789);    // 2000364A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD509);    // 2000364C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3020);    // 2000364E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x7801);    // 20003650 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x481A);    // 20003652 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8880);    // 20003654 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF000);    // 20003656 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF841);    // 20003658 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2802);    // 2000365A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD900);    // 2000365C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1E80);    // 2000365E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1904);    // 20003660 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4817);    // 20003662 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8047);    // 20003664 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8084);    // 20003666 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x80C5);    // 20003668 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8106);    // 2000366A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBDF8);    // 2000366C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // 2000366E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2370);    // 20003670 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 20003672 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01E4);    // 20003674 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 20003676 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2EBC);    // 20003678 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 2000367A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0140);    // 2000367C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD000);    // 2000367E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xB080);    // 20003680 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD000);    // 20003682 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x12CC);    // 20003684 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 20003686 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3798);    // 20003688 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 2000368A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xA000);    // 2000368C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD000);    // 2000368E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0BB8);    // 20003690 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // 20003692 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A10);    // 20003694 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 20003696 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1224);    // 20003698 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 2000369A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2590);    // 2000369C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 2000369E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3754);    // 200036A0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 200036A2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2242);    // 200036A4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 200036A6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1100);    // 200036A8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD000);    // 200036AA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2F34);    // 200036AC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 200036AE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2138);    // 200036B0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 200036B2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x256A);    // 200036B4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 200036B6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xC100);    // 200036B8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD000);    // 200036BA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0D98);    // 200036BC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    // 200036BE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3200);    // 200036C0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD000);    // 200036C2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xB403);    // 200036C4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4801);    // 200036C6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x9001);    // 200036C8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBD01);    // 200036CA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0895);    // 200036CC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);    // 200036CE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xB403);    // 200036D0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4801);    // 200036D2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x9001);    // 200036D4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBD01);    // 200036D6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x092B);    // 200036D8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);    // 200036DA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xB403);    // 200036DC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4801);    // 200036DE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x9001);    // 200036E0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBD01);    // 200036E2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0DF1);    // 200036E4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);    // 200036E6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xB403);    // 200036E8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4801);    // 200036EA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x9001);    // 200036EC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBD01);    // 200036EE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFCBF);    // 200036F0 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // 200036F2 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xB403);    // 200036F4 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4801);    // 200036F6 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x9001);    // 200036F8 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBD01);    // 200036FA 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0895);    // 200036FC 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // 200036FE 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xB403);    // 20003700 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4801);    // 20003702 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x9001);    // 20003704 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBD01);    // 20003706 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFC2D);    // 20003708 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // 2000370A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xB403);    // 2000370C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4801);    // 2000370E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x9001);    // 20003710 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBD01);    // 20003712 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0371);    // 20003714 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // 20003716 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xB403);    // 20003718 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4801);    // 2000371A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x9001);    // 2000371C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBD01);    // 2000371E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x9F27);    // 20003720 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // 20003722 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xB403);    // 20003724 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4801);    // 20003726 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x9001);    // 20003728 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBD01);    // 2000372A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x745B);    // 2000372C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // 2000372E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xB403);    // 20003730 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4801);    // 20003732 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x9001);    // 20003734 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBD01);    // 20003736 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x990D);    // 20003738 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // 2000373A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xB403);    // 2000373C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4801);    // 2000373E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x9001);    // 20003740 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBD01);    // 20003742 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x7447);    // 20003744 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // 20003746 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xB403);    // 20003748 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4801);    // 2000374A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x9001);    // 2000374C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBD01);    // 2000374E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xA1DD);    // 20003750 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // 20003752 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0083);    // 20003754 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00FF);    // 20003756 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0084);    // 20003758 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00FF);    // 2000375A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0038);    // 2000375C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x007F);    // 2000375E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0039);    // 20003760 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x007F);    // 20003762 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x003A);    // 20003764 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x007F);    // 20003766 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x003B);    // 20003768 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x007F);    // 2000376A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x003C);    // 2000376C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0007);    // 2000376E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x003D);    // 20003770 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0007);    // 20003772 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0080);    // 20003774 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // 20003776 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0080);    // 20003778 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // 2000377A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFC0);    // 2000377C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // 2000377E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFC0);    // 20003780 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // 20003782 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFC0);    // 20003784 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // 20003786 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFC0);    // 20003788 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // 2000378A 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFFC);    // 2000378C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // 2000378E 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFFC);    // 20003790 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // 20003792 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x007F);    // 20003794 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // 20003796 

	// End of Patch Data(Last : 20003796h)      
	// Total Size 1460 (0x05B4)                 
	// Addr : 31E4 , Size : 1458(5B2h)          
	                                            
	// TNP_WAKEUP_MIPI2LANE_ULPS		            
	// TNP_GAS_OTP_PAGE_SELECT			            
	// TNP_AWB_MODUL_COMP				                
	// TNP_USER_SHARP_BLUR				              
	// TNP_AE_HDR_CONTROL									      
	// TNP_FLS_FRAME_SKIP_FIX			              
	// TNP_EOL_WA_2ND_VER                       
    S5K5EAYX_write_cmos_sensor(0x0028, 0x2000);                                         
    S5K5EAYX_write_cmos_sensor(0x002A, 0x122C);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);                                         
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0A32);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);                                         
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0A2C);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0002);                                         
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0A2E);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);

    S5K5EAYX_write_cmos_sensor(0x0028, 0xD000);
    S5K5EAYX_write_cmos_sensor(0x002A, 0x1000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);

//==================================================================================
// 03.Analog Setting & ASP Control
//==================================================================================
//This register is for FACTORY ONLY.
//If you change it without prior notification
//YOU are RESPONSIBLE for the FAILURE that will happen in the future
//WARNING : Before REG_TC_IPRM_InitParamsUpdated, Do not Write at HW Registers directly.
//if Fw has Register's value, it will be updated by FW default.
//Move to 20-1.HW Direct Setting due to Timing constraint.      

    S5K5EAYX_write_cmos_sensor(0x002A, 0xF400);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x443F);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2020);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0B0D);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8008);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0003);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0010);                                         
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF410);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5777);                                         
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF414);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0007);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    

	S5K5EAYX_write_cmos_sensor(0x002A, 0xF41E);  //Add patch ,andy
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1111);  
	
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF424);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5300);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0209);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1037);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0007);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0081);                                         
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF432);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0508);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0509);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x08F9);    //VPIX 80F9                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1002);                                         
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF5B8);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0010);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0020);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0040);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0050);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0070);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0080);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00A0);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00B0);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x002F);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x005F);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x005F);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x008F);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x008F);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00BE);                                         
    S5K5EAYX_write_cmos_sensor(0x002A, 0xE502);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0820);    // bpr_ob                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // bpr_active                        
    S5K5EAYX_write_cmos_sensor(0x002A, 0xE600);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0010);    // adlc_config                       
    S5K5EAYX_write_cmos_sensor(0x002A, 0xE606);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0125);    // adlc_enable                       
    S5K5EAYX_write_cmos_sensor(0x002A, 0xE602);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0040);    // adlc_data_pedestal                
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1FC0);    // adlc_data_depedestal off          
    S5K5EAYX_write_cmos_sensor(0x002A, 0xE61E);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // ptune_gain_total                  
    S5K5EAYX_write_cmos_sensor(0x002A, 0xE628);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // ptune_offset_total                
    S5K5EAYX_write_cmos_sensor(0x002A, 0xE614);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2010);    // adlc_fadlc_filter_co              
    S5K5EAYX_write_cmos_sensor(0x002A, 0xE62E);                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0007);    // adlc_fadlc_filter_config          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // adlc_refresh_level_diff_threshold 
    
	// Start of Analog setting
	// revision history
	// 2012.6. 4 1st draft
	// 2012.6.19 Modified the ADC SAT
	// MS off
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF482);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0613);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0010);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0613);
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF48A);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0615);
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF48E);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0617);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0205);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0258);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x000F);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0617);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0204);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0615);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0205);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0613);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0204);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0278);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0206);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x02BE);
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF4AA);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00AF);
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF4AE);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00BE);
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF4B2);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00C1);
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF4B6);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0204);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0258);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0273);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00C0);
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF5F4);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00C1);
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF4CA);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0615);
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF4CE);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0104);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0204);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0301);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0611);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0206);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x021E);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x020E);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0226);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0216);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0226);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0206);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0208);
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF4EA);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x020E);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0226);
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF4F2);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0206);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0208);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0104);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0175);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0185);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0204);
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF500);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0301);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x047A);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x048A);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0611);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0176);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x017D);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x020A);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0212);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x021F);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0227);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x047B);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0482);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0615);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x061D);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0178);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x017E);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x020C);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0213);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0221);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0228);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x047D);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0483);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0617);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x061E);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0179);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x017E);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x020D);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0213);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0222);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0228);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x047E);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0483);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0618);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x061E);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0176);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0178);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0206);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0208);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x021D);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x021F);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x047B);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x047D);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0613);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0615);
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF55C);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x017C);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x017F);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x020C);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0211);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0223);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0228);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0481);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0484);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0619);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x061E);
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF574);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0176);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0178);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0206);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0208);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x021D);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x021F);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x047B);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x047D);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0613);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0615);
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF58A);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x017C);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x020C);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0481);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0619);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0103);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0205);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0300);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0612);
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF59E);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A6B);
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF5FA);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0206);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0208);
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF456);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0206);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0222);
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF5A2);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00C0);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0617);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00C1);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00C3);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0206);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0209);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0613);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0616);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0209);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A60);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A70);

 // additional option   
    S5K5EAYX_write_cmos_sensor(0x002A, 0xC342);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A72);
    S5K5EAYX_write_cmos_sensor(0x002A, 0xC200);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A17);
    S5K5EAYX_write_cmos_sensor(0x002A, 0xE300);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0007);
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF430);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0E10);
    S5K5EAYX_write_cmos_sensor(0x002A, 0xC202);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0400);
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF422);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x000E);
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF2AA);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF40E);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0071);
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF42E);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00A6);
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF412);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00C8);
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF420);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1000);
    S5K5EAYX_write_cmos_sensor(0x002A, 0xF40C);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0700);
	// End of Analog setting  

	//For subsampling Size
	// For Capture
	//ETC  
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0054);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x002D);
	//Forbidden
	//WRITE #senHal_shutAreaStart_1_   FC44
	//WRITE #senHal_sRightFobiddenStart_1_ 0BB8

	//Start Offset    
    S5K5EAYX_write_cmos_sensor(0x0028, 0x2000);
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0D04);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0044);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0034);
	//Line length pck     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0072); //Full mode: adding line line_length_pck 2608 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x082A); // 	//Bin mode: min line_length_pck 2090 //   
    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0CF8);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);//0:sub sampling, 1: average subsampling 

	//SHBN Comp_bias control      
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0D9E);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0040); //gain > x2, use Tune2 register  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000); //comp1_bias=0d                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0007); //comp1_bias=7d                  
 
 
	//==================================================================================
	//04.ETC Setting
	//==================================================================================
	//WRITE #oif_bBypassMipiSleepSeq 1  //for non ulps mode   
    S5K5EAYX_write_cmos_sensor(0x002A, 0x09DE);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);
    S5K5EAYX_write_cmos_sensor(0x002A, 0x09E4);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF400);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x005A);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF5B8);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0078);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xE502);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0004);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xE600);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x003E);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF482);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x013C);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xE300);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0002);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF5F4);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0002);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);
    
 
    //==================================================================================
    //Gas_Anti Shading_Otp.no OTP)
    //==================================================================================
    S5K5EAYX_write_cmos_sensor(0x002A, 0x14AC);	                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);	//#ash_bUseGasAlpha   
    S5K5EAYX_write_cmos_sensor(0x002A, 0x149C);	                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);	//#ash_bUseAutoStart	
    S5K5EAYX_write_cmos_sensor(0x002A, 0x1498);	                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);	//#ash_bUseGainCal	  
    S5K5EAYX_write_cmos_sensor(0x002A, 0x149E);	                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);	//#ash_bWriteEdgemode 
    
//WRITE	#skl_OTP_usWaitTime	0100	// This register should be positioned in fornt of D0001000
//WRITE	#skl_bUseOTPfunc	0001	// This is OTP on/off function
//WRITE	#ash_bUseOTPData	0000	//
//WRITE	#awbb_otp_disable	0001	//
//WRITE	#ash_bUseGasAlphaOTP	0000	//

    S5K5EAYX_write_cmos_sensor(0x002A, 0x1478);	                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);	//wbt_bUseOutdoorASH   
        
// Refer Mon_AWB_RotGain
    S5K5EAYX_write_cmos_sensor(0x002A, 0x1480);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00C0);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00DF);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x011F);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01A2);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01FC);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01FF);
    
// GAS Alpha Table    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x14AE);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4000); //4300
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3500);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3500); //4000
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3500); //4000
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4000);

// Outdoor GAS Alpha      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4000); // R   //by 7500K 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4000); // GR             
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4000); // GB             
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4000); // B              

// TVAR_ash_pGAS_high      
    S5K5EAYX_write_cmos_sensor(0x002A, 0x1860);
// TVAR_ash_pGAS_high    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);

    
    // TVAR_ash_pGAS_low
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x343F);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF8D9);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x089B);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFD19);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xEE4F);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2354);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFBF4);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFC0F);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF9BC);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0AAE);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x04A8);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xEAEC);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF85C);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x06DB);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x051A);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF794);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF439);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1EBD);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0E8C);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF9BB);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFEC5);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03FF);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x05C9);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xE8FC);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFAA8);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFE2F);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF19);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03BA);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0192);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x06CB);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFB89);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x089E);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFE44);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFB4E);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFD2E);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0212);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3720);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFA7A);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x087D);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF5F1);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01AE);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0E91);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFCFC);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF786);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFE59);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A89);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFB6A);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF957);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF5DC);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x08E2);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0025);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFB7B);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF966);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1092);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1095);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFBA4);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0166);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0006);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x029C);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF622);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFC0D);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFBE1);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFCE2);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x06A8);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0485);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFC4F);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF8C9);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x08C1);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01E2);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF6E3);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFE31);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x054E);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x300F);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF924);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0B8E);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF2E1);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0177);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1241);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFC8B);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFB29);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFD46);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0852);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFD64);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF70F);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFB41);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x082F);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00E5);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFB87);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFA82);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x11E4);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x08A2);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF8A5);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFE86);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0594);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFED4);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF310);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF17);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00B1);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x014E);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFB3);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03A5);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0522);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFA9D);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x058E);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFD53);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF03);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFBCA);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0181);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3474);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF955);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0852);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFA9B);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF50C);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1D11);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFCAE);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF8A0);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFECA);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0993);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFBB4);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF6A2);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF89F);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0AC8);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFE49);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFB6B);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFBC7);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1175);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A5B);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF9B4);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFF4);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03A0);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x007D);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF325);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0226);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFA07);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03D6);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01F1);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFB8);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x05AE);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF642);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C05);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFA56);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFC60);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00C3);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFC9);
    
//WRITE #ash_bGasBypass 1

//==================================================================================
// 06.AF Setting
//==================================================================================    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x01E8);                                                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);	//REG_TC_IPRM_LedGpio	                                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0003);	//REG_TC_IPRM_CM_Init_AfModeType                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);	//REG_TC_IPRM_CM_Init_PwmConfig1                                         
    S5K5EAYX_write_cmos_sensor(0x002A, 0x01F0);	                                                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0041);	//REG_TC_IPRM_CM_Init_GpioConfig1                                        
    S5K5EAYX_write_cmos_sensor(0x002A, 0x01F8);                                                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x120C);	//REG_TC_IPRM_CM_Init_Mi2cBits [Data:Clock:ID] GPIO 1,2                  
    S5K5EAYX_write_cmos_sensor(0x002A, 0x01FC);	                                                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0064);	//REG_TC_IPRM_CM_Init_Mi2cRateKhz IIC Speed 100Khz                       
    S5K5EAYX_write_cmos_sensor(0x002A, 0x02D0);                                                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);	//REG_TC_AF_FstWinStartX                                                 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00E3);	//REG_TC_AF_FstWinStartY                                                 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0200);	//REG_TC_AF_FstWinSizeX                                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0238);	//REG_TC_AF_FstWinSizeY                                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01C6);	//REG_TC_AF_ScndWinStartX                                                
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0166);	//REG_TC_AF_ScndWinStartY                                                
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0074);	//REG_TC_AF_ScndWinSizeX                                                 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0132);	//REG_TC_AF_ScndWinSizeY                                                 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);	//REG_TC_AF_WinSizesUpdated                                              
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0A1A);                                                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00FF);	//skl_af_StatOvlpExpFactor                                               
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0A2A);                                                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);	//skl_af_bAfStatOff                                                      
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0660);                                                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);	//af_search_usAeStable                                                   
    S5K5EAYX_write_cmos_sensor(0x002A, 0x066C);                                                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1002);	//af_search_usSingleAfFlags,  Double peak , Fine search enable,          
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0676);                                                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0002);	//af_search_usFinePeakCount                                              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);	//af_search_usFineMaxScale                                               
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0670);                                                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0003);	//af_search_usMinPeakSamples                                             
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0662);                                                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00E5);	//af_search_usPeakThr,  Full search (E5 90%)                             
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0098);	//af_search_usPeakThrLow                                                 
    S5K5EAYX_write_cmos_sensor(0x002A, 0x06BE);                                                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF95);	//af_search_usConfCheckOrder_1_                                          
    S5K5EAYX_write_cmos_sensor(0x002A, 0x068E);                                                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0280);	//af_search_usConfThr_4_                                                 
    S5K5EAYX_write_cmos_sensor(0x002A, 0x069A);                                                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03A0);	//af_search_usConfThr_10_                                                
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0320);	//af_search_usConfThr_11_                                                
    S5K5EAYX_write_cmos_sensor(0x002A, 0x06E0);                                                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0030);	//af_stat_usMinStatVal                                                   
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0710);                                                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0060);	//af_scene_usSceneLowNormBrThr                                           
    S5K5EAYX_write_cmos_sensor(0x002A, 0x06F8);                                                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0010);	//af_stat_usBpfThresh                                                    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x067A);                                                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);	//af_search_usCapturePolicy                                              
    S5K5EAYX_write_cmos_sensor(0x002A, 0x05D4);                                                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0086);	//af_pos_usCaptureFixedPos                                               
    S5K5EAYX_write_cmos_sensor(0x002A, 0x05B8);                                                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0080);	//af_pos_usHomePos                                                       
    S5K5EAYX_write_cmos_sensor(0x002A, 0x05BC);                                                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0080);	//af_pos_usLowConfPos Macro: 372                                         
    S5K5EAYX_write_cmos_sensor(0x002A, 0x05BE);                                                                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0174);  //af_pos_usLowConfPos Macro: 372                                        
    S5K5EAYX_write_cmos_sensor(0x002A, 0x05C0);                                                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0120);	//af_pos_usMiddlePos                                                     
    S5K5EAYX_write_cmos_sensor(0x002A, 0x05C2);                                                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0120);                                                                          
    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x05D8);
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0014); //af_pos_usTableLastInd  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0086); //af_pos_usTable_0_      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0094);  //af_pos_usTable_1_     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00A2); //af_pos_usTable_2_      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00B0); //af_pos_usTable_3_      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00BE); //af_pos_usTable_4_      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00CC); //af_pos_usTable_5_      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00DA); //af_pos_usTable_6_      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00E8); //af_pos_usTable_7_      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00F6); //af_pos_usTable_8_      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0104); //af_pos_usTable_9_      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0112); //af_pos_usTable_10_     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0120); //af_pos_usTable_11_     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x012E); //af_pos_usTable_12_     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x013C); //af_pos_usTable_13_     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x014A); //af_pos_usTable_14_     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0158); //af_pos_usTable_15_     D
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0166); //af_pos_usTable_16_     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0174); //af_pos_usTable_17_     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0182); //af_pos_usTable_18_     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0190); //af_pos_usTable_19_     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x019A); //af_pos_usTable_20_     

// Continuous AF    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x071A);                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5050);//af_refocus_usFlUpLow    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8080);//af_refocus_usFlLcUpLow  

// Scene Change Detection Sensitivity
//WRITE	#af_scd_usResetNWaitFr	000A	// x4 frames. (40 frames. This value should be same as af_refocus_usFlFrames)
// 0 ~ 0x400. Bigger value is more sensitivity.        
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0752);                                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03E0); //Sensitivity for Normal scene         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03C0); //Sensitivity for Low light scene      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03E0); //Sensitivity forOutdoor scene     

// AF driver         
    S5K5EAYX_write_cmos_sensor(0x002A, 0x056A);                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8000); //	power down flag.[15] bit set 1.      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0004); //  Shift                                
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3FF0); //  Mask                             
        
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0574);                                                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0020); 	//  Slow motion delay                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0030); 	//  Moving distanceD threshold for Slow motion delay     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0010); 	//  Signal shaping delay time                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0040); 	//                                                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0080); 	//                                                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00C0); 	//                                                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00E0); 	//                                                    
        
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0A3A);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0002); // skl_ThumbStartY_OffsetAF       


//==================================================================================
// 07.Flash Setting
//==================================================================================
    S5K5EAYX_write_cmos_sensor(0x002A, 0x04C0);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0002);	//capture flash on                
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0C7C);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);	//one frame AE                    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0C4E);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x023C);	//AWB R point                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0248);	//AWB B point                     
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0C82);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);	// Fls AE tune start              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);	//Rin                             
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0120);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0180);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0200);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0400);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0800);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A00);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1000);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);	//Rout                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00A0);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0090);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0080);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0070);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0045);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0030);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0010);                                   
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0CC4);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);	// flash NB default               
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);                                   
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0C68);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);	//flash WP_Weight default         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00C0);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0080);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x000A);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0030);	//flash WP_Lei_Thres default      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0040);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0048);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0050);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0060);                                   
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0CD4);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0120);	//Fls  BRIn                       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0150);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0200);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x003C);	// Fls  BROut                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x003B);                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0026);	//                                

//==================================================================================
// 08.Auto Flicker Detection
//==================================================================================    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x1276);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);    

// Auto Flicker (60Mhz start)      
    S5K5EAYX_write_cmos_sensor(0x002A, 0x1270);                                       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFC_Default BIT[0] 1:60Hz 0:50Hz 
    S5K5EAYX_write_cmos_sensor(0x002A, 0x051C);                                       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x077F);    //                                 

//==================================================================================
// 09.AE Setting
//==================================================================================
//AE Target    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0B20);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x003f);  //0032  

//ae_StatMode bit[3] BLC has to be bypassed to prevent AE weight change especially backlight scene             
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0B26);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x000F);     
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0854);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0002);    

//AE_state      
    S5K5EAYX_write_cmos_sensor(0x002A, 0x081C);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0111);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00EF);    

//AE Concept     
    S5K5EAYX_write_cmos_sensor(0x002A, 0x08D4);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);    

//Exposure     
    S5K5EAYX_write_cmos_sensor(0x002A, 0x08DC);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A3C);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0D05);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4008);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x7000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x9C00);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xAD00);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xF1D4);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0002);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xDC00);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0005);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xDC00);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0005);    
    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A3C);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0D05);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3408);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3408);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x6B10);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8214);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xC350);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD4C0);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xD4C0);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);   

//Gain       
    S5K5EAYX_write_cmos_sensor(0x002A, 0x08D8);         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A00);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);     
    S5K5EAYX_write_cmos_sensor(0x002A, 0x086E);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A00);     

// Lei Control    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0984);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3380);     
    S5K5EAYX_write_cmos_sensor(0x002A, 0x097A);         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x000C);    //lt_uMaxLei

//==================================================================================
// 10.AE Weight (Normal)
//==================================================================================
    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0B2E);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0201);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0102);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0202);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0202);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0201);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0302);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0203);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0102);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0201);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0302);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0203);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0102);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0202);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0202);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0201);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0102);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);     

//==================================================================================
// 11.AWB-BASIC setting
//==================================================================================
// AWB init Start point    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x121C);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0580);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0428);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x07B0);    
    
// AWB Convergence Speed    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0008);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0190);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00A0);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0004);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0002);     
    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x1208);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFE0);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);     
    S5K5EAYX_write_cmos_sensor(0x002A, 0x1210);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00C2);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0002);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0074);     
    S5K5EAYX_write_cmos_sensor(0x002A, 0x121A);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);     

//// White Locus    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0F88);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0120);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0121);     
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0FA2);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x041D);     
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0FA6);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0771);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03A4);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0036);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0002);     
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0FBC);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0032);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x001E);     
    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0F8C);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x02DF);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0314);    


////SceneDetectionMap                                 
////awbb_SCDetectionMap_SEC_SceneDetectionMap[0]     S5K5EAYX_write_cmos_sensor(0x0F12, 0x8F01);    // 70003C70 
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0FD4);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0500);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5555);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5455);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xAA55);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xAAAA);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBF54);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFFF);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x54FE);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF6F);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFEFF);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1B54);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFFF);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x54FE);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF06);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFEFF);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0154);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBFBF);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x54BE);     
    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFEF7);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0021);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0AF0);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0AF0);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x018F);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0096);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x000E);    
     
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0FC0);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00C0);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0010);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0002);     
    S5K5EAYX_write_cmos_sensor(0x002A, 0x2BAE);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0006);     
    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0F94);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x000C);     
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0FA0);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0020);     

//// Indoor Zone                               
//	param_start	awbb_IndoorGrZones_m_BGrid     S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C00);    // 70003CD2 
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0DB4);                                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0324);    //awbb_IndoorGrZones_m_BGrid[0] 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03B8);    //awbb_IndoorGrZones_m_BGrid[1] 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x02E8);    //awbb_IndoorGrZones_m_BGrid[2] 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03E0);    //awbb_IndoorGrZones_m_BGrid[3] 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x02BA);    //awbb_IndoorGrZones_m_BGrid[4] 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03F6);    //awbb_IndoorGrZones_m_BGrid[5] 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x02A0);    //awbb_IndoorGrZones_m_BGrid[6] 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03DC);    //awbb_IndoorGrZones_m_BGrid[7] 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x028E);    //awbb_IndoorGrZones_m_BGrid[8] 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03C4);    //awbb_IndoorGrZones_m_BGrid[9] 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x027A);    //awbb_IndoorGrZones_m_BGrid[10]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03A6);    //awbb_IndoorGrZones_m_BGrid[11]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0262);    //awbb_IndoorGrZones_m_BGrid[12]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0382);    //awbb_IndoorGrZones_m_BGrid[13]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0250);    //awbb_IndoorGrZones_m_BGrid[14]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0318);    //awbb_IndoorGrZones_m_BGrid[15]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0236);    //awbb_IndoorGrZones_m_BGrid[16]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x02F4);    //awbb_IndoorGrZones_m_BGrid[17]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0212);    //awbb_IndoorGrZones_m_BGrid[18]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x02AC);    //awbb_IndoorGrZones_m_BGrid[19]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01FE);    //awbb_IndoorGrZones_m_BGrid[20]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0296);    //awbb_IndoorGrZones_m_BGrid[21]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01F0);    //awbb_IndoorGrZones_m_BGrid[22]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0288);    //awbb_IndoorGrZones_m_BGrid[23]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01E0);    //awbb_IndoorGrZones_m_BGrid[24]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0274);    //awbb_IndoorGrZones_m_BGrid[25]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01D2);    //awbb_IndoorGrZones_m_BGrid[26]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x026E);    //awbb_IndoorGrZones_m_BGrid[27]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01C6);    //awbb_IndoorGrZones_m_BGrid[28]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0258);    //awbb_IndoorGrZones_m_BGrid[29]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01BC);    //awbb_IndoorGrZones_m_BGrid[30]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0246);    //awbb_IndoorGrZones_m_BGrid[31]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01B4);    //awbb_IndoorGrZones_m_BGrid[32]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x022E);    //awbb_IndoorGrZones_m_BGrid[33]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01B4);    //awbb_IndoorGrZones_m_BGrid[34]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0214);    //awbb_IndoorGrZones_m_BGrid[35]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01B2);    //awbb_IndoorGrZones_m_BGrid[36]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01F6);    //awbb_IndoorGrZones_m_BGrid[37]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01B0);    //awbb_IndoorGrZones_m_BGrid[38]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01DA);    //awbb_IndoorGrZones_m_BGrid[39]
//	param_end	awbb_IndoorGrZones_m_BGrid           
//	param_start	awbb_IndoorGrZones_m_GridStep      S5K5EAYX_write_cmos_sensor(0x0F12, 0x2900);    // 70003D26 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0005);    //awbb_IndoorGrZones_m_GridStep[0] 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //awbb_IndoorGrZones_m_GridStep[1]
//	param_end	awbb_IndoorGrZones_m_GridStep           
//	param_start	awbb_IndoorGrZones_ZInfo_m_GridSz    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFB45);    // 70003D2C 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0014);   //awbb_IndoorGrZones_ZInfo_m_GridSz[0] 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);   //awbb_IndoorGrZones_ZInfo_m_GridSz[1] 
//	param_end	awbb_IndoorGrZones_ZInfo_m_GridSz
//	param_start	awbb_IndoorGrZones_m_Boffs    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x010A);    //awbb_IndoorGrZones_m_Boffs[0]  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //awbb_IndoorGrZones_m_Boffs[1]  
//	param_end	awbb_IndoorGrZones_m_Boffs           

// Outdoor Zone
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0E10);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0272);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x02A0);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x025A);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x02BC);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x024A);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x02C0);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x023C);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x02BE);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x022E);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x02BC);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0224);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x02B6);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0218);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x02AA);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0210);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x02A0);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x020C);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0296);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x020A);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x028C);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0212);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x027E);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0234);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0256);    
    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0004);    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0E44);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x000C);    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0E48);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01D8);    

// Low Brightness Zone     
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0E4C);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0350);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0422);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x02C4);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0452);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0278);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x041C);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0230);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03EE);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01F0);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0392);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01C0);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0340);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0194);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0302);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x016E);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x02C2);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0148);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0286);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x018A);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0242);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0006);    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0E80);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x000A);    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0E84);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0108);    

//// Low Temp. Zone      
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0E88);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0380);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0168);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2D90);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    

////AWB - GridCorrection       
    S5K5EAYX_write_cmos_sensor(0x002A, 0x11EC);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x02CE);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0347);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03C2);    
     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x10A0);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x10A1);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1185);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1186);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x11E5);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x11E6);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x11E6);    
    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00AB);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00BF);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00D2);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0093);    

// Indoor Grid Offset     
    S5K5EAYX_write_cmos_sensor(0x002A, 0x113C);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFCE);  //0000  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFCE);   //0000 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFCE);  //0000  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFCE);  //0000  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFCE); //0000   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFCE);  //0000  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    

// Outdoor Grid Offset     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFD0);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFD0);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF66);     
//==================================================================================
// 12.CCM Setting
//==================================================================================
    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x146A);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00E4);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00F0);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0120);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0150);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0180);    
    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);    
    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x145C);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4800);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x1464);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x48D8);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);   
     
    S5K5EAYX_write_cmos_sensor(0x002A, 0x4800);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0208); //TVAR_wbt_pBaseCcms[0] // Horizon         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFB5); //TVAR_wbt_pBaseCcms[1]                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFE8); //TVAR_wbt_pBaseCcms[2]                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF20); //TVAR_wbt_pBaseCcms[3]                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01BF); //TVAR_wbt_pBaseCcms[4]                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF53); //TVAR_wbt_pBaseCcms[5]                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0022); //TVAR_wbt_pBaseCcms[6]                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFEA); //TVAR_wbt_pBaseCcms[7]                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01C2); //TVAR_wbt_pBaseCcms[8]                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00C6); //TVAR_wbt_pBaseCcms[9]                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0095); //TVAR_wbt_pBaseCcms[10]                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFEFD); //TVAR_wbt_pBaseCcms[11]                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0206); //TVAR_wbt_pBaseCcms[12]                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF7F); //TVAR_wbt_pBaseCcms[13]                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0191); //TVAR_wbt_pBaseCcms[14]                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF06); //TVAR_wbt_pBaseCcms[15]                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01BA); //TVAR_wbt_pBaseCcms[16]                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0108); //TVAR_wbt_pBaseCcms[17]                   
    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0208);    //TVAR_wbt_pBaseCcms[18] // IncandA   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFB5);    //TVAR_wbt_pBaseCcms[19]              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFE8);    //TVAR_wbt_pBaseCcms[20]              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF20);    //TVAR_wbt_pBaseCcms[21]              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01BF);    //TVAR_wbt_pBaseCcms[22]              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF53);    //TVAR_wbt_pBaseCcms[23]              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0022);    //TVAR_wbt_pBaseCcms[24]              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFEA);    //TVAR_wbt_pBaseCcms[25]              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01C2);    //TVAR_wbt_pBaseCcms[26]              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00C6);    //TVAR_wbt_pBaseCcms[27]              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0095);    //TVAR_wbt_pBaseCcms[28]              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFEFD);    //TVAR_wbt_pBaseCcms[29]              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0206);    //TVAR_wbt_pBaseCcms[30]              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF7F);    //TVAR_wbt_pBaseCcms[31]              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0191);    //TVAR_wbt_pBaseCcms[32]              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF06);    //TVAR_wbt_pBaseCcms[33]              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01BA);    //TVAR_wbt_pBaseCcms[34]              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0108);    //TVAR_wbt_pBaseCcms[35]                 
    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0208);   //TVAR_wbt_pBaseCcms[36] // WW 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFB5);   //TVAR_wbt_pBaseCcms[37]       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFE8);   //TVAR_wbt_pBaseCcms[38]       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF20);   //TVAR_wbt_pBaseCcms[39]       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01BF);   //TVAR_wbt_pBaseCcms[40]       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF53);   //TVAR_wbt_pBaseCcms[41]       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0022);   //TVAR_wbt_pBaseCcms[42]       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFEA);   //TVAR_wbt_pBaseCcms[43]       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01C2);   //TVAR_wbt_pBaseCcms[44]       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00C6);   //TVAR_wbt_pBaseCcms[45]       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0095);   //TVAR_wbt_pBaseCcms[46]       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFEFD);   //TVAR_wbt_pBaseCcms[47]       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0206);   //TVAR_wbt_pBaseCcms[48]       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF7F);   //TVAR_wbt_pBaseCcms[49]       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0191);   //TVAR_wbt_pBaseCcms[50]       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF06);   //TVAR_wbt_pBaseCcms[51]       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01BA);   //TVAR_wbt_pBaseCcms[52]       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0108);   //TVAR_wbt_pBaseCcms[53]       
    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0204);    //TVAR_wbt_pBaseCcms[54] // CW    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFB2);    //TVAR_wbt_pBaseCcms[55]          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFF5);    //TVAR_wbt_pBaseCcms[56]          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFEF1);    //TVAR_wbt_pBaseCcms[57]          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x014E);    //TVAR_wbt_pBaseCcms[58]          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF18);    //TVAR_wbt_pBaseCcms[59]          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFE6);    //TVAR_wbt_pBaseCcms[60]          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFDD);    //TVAR_wbt_pBaseCcms[61]          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01B2);    //TVAR_wbt_pBaseCcms[62]          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00F2);    //TVAR_wbt_pBaseCcms[63]          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00CA);    //TVAR_wbt_pBaseCcms[64]          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF48);    //TVAR_wbt_pBaseCcms[65]          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0151);    //TVAR_wbt_pBaseCcms[66]          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF50);    //TVAR_wbt_pBaseCcms[67]          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0147);    //TVAR_wbt_pBaseCcms[68]          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF75);    //TVAR_wbt_pBaseCcms[69]          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0187);    //TVAR_wbt_pBaseCcms[70]          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01BF);    //TVAR_wbt_pBaseCcms[71]          
    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0204);    //TVAR_wbt_pBaseCcms[72] // D50  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFB2);    //TVAR_wbt_pBaseCcms[73]         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFF5);    //TVAR_wbt_pBaseCcms[74]         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFEF1);    //TVAR_wbt_pBaseCcms[75]         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x014E);    //TVAR_wbt_pBaseCcms[76]         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF18);    //TVAR_wbt_pBaseCcms[77]         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFE6);    //TVAR_wbt_pBaseCcms[78]         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFDD);    //TVAR_wbt_pBaseCcms[79]         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01B2);    //TVAR_wbt_pBaseCcms[80]         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00F2);    //TVAR_wbt_pBaseCcms[81]         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00CA);    //TVAR_wbt_pBaseCcms[82]         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF48);    //TVAR_wbt_pBaseCcms[83]         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0151);    //TVAR_wbt_pBaseCcms[84]         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF50);    //TVAR_wbt_pBaseCcms[85]         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0147);    //TVAR_wbt_pBaseCcms[86]         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF75);    //TVAR_wbt_pBaseCcms[87]         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0187);    //TVAR_wbt_pBaseCcms[88]         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01BF);    //TVAR_wbt_pBaseCcms[89]         
     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0204);    //TVAR_wbt_pBaseCcms[90] // D65    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFB2);    //TVAR_wbt_pBaseCcms[91]           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFF5);    //TVAR_wbt_pBaseCcms[92]           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFEF1);    //TVAR_wbt_pBaseCcms[93]           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x014E);    //TVAR_wbt_pBaseCcms[94]           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF18);    //TVAR_wbt_pBaseCcms[95]           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFE6);    //TVAR_wbt_pBaseCcms[96]           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFDD);    //TVAR_wbt_pBaseCcms[97]           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01B2);    //TVAR_wbt_pBaseCcms[98]           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00F2);    //TVAR_wbt_pBaseCcms[99]           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00CA);    //TVAR_wbt_pBaseCcms[100]          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF48);    //TVAR_wbt_pBaseCcms[101]          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0151);    //TVAR_wbt_pBaseCcms[102]          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF50);    //TVAR_wbt_pBaseCcms[103]          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0147);    //TVAR_wbt_pBaseCcms[104]          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF75);    //TVAR_wbt_pBaseCcms[105]          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0187);    //TVAR_wbt_pBaseCcms[106]          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01BF);    //TVAR_wbt_pBaseCcms[107]          
    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01E5);    //TVAR_wbt_pOutdoorCcm[0] 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFA4);    //TVAR_wbt_pOutdoorCcm[1] 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFDC);    //TVAR_wbt_pOutdoorCcm[2] 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFE90);    //TVAR_wbt_pOutdoorCcm[3] 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x013F);    //TVAR_wbt_pOutdoorCcm[4] 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF1B);    //TVAR_wbt_pOutdoorCcm[5] 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFD2);    //TVAR_wbt_pOutdoorCcm[6] 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFDF);    //TVAR_wbt_pOutdoorCcm[7] 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0236);    //TVAR_wbt_pOutdoorCcm[8] 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00EC);    //TVAR_wbt_pOutdoorCcm[9] 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00F8);    //TVAR_wbt_pOutdoorCcm[10]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF34);    //TVAR_wbt_pOutdoorCcm[11]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01CE);    //TVAR_wbt_pOutdoorCcm[12]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF83);    //TVAR_wbt_pOutdoorCcm[13]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0195);    //TVAR_wbt_pOutdoorCcm[14]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFEF3);    //TVAR_wbt_pOutdoorCcm[15]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0126);    //TVAR_wbt_pOutdoorCcm[16]
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0162);    //TVAR_wbt_pOutdoorCcm[17]
 
 
 //==================================================================================
// 13.GAMMA
//==================================================================================
// Indoor Gamma   
    S5K5EAYX_write_cmos_sensor(0x002A, 0x12F4);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x000A);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0016);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0030);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0066);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00D5);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0138);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0163);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0189);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01C6);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01F8);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0222);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x023D);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x026E);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x029C);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x02EC);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x032D);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x036E);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03B2);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03FF);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x000A);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0016);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0030);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0066);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00D5);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0138);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0163);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0189);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01C6);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01F8);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0222);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x023D);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x026E);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x029C);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x02EC);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x032D);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x036E);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03B2);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03FF);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x000A);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0016);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0030);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0066);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00D5);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0138);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0163);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0189);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01C6);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01F8);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0222);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x023D);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x026E);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x029C);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x02EC);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x032D);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x036E);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03B2);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03FF);    
// Outdoor Gamma      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x000A);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0016);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0030);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0066);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00D5);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0138);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0163);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0189);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01C6);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01F8);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0222);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x023D);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x026E);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x029C);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x02EC);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x032D);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x036E);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03B2);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03FF);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x000A);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0016);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0030);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0066);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00D5);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0138);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0163);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0189);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01C6);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01F8);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0222);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x023D);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x026E);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x029C);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x02EC);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x032D);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x036E);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03B2);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03FF);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x000A);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0016);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0030);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0066);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00D5);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0138);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0163);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0189);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01C6);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01F8);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0222);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x023D);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x026E);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x029C);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x02EC);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x032D);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x036E);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03B2);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03FF);    


//==================================================================================
// 14.AFIT
//==================================================================================
//Tune//5EA //FPGA    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x14FC);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x003F);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0083);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x012F);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01F0);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0255);    
     
    S5K5EAYX_write_cmos_sensor(0x002A, 0x14F0);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    // on/off AFIT by NB option  
     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0014);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00D2);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0384);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x07D0);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1388);     
    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x152E);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0070);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0005);     
    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0180);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0196);     

//	AFIT 0      
    S5K5EAYX_write_cmos_sensor(0x002A, 0x1538);             
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);  //AFIT16_BRIGHTNESS                                                                                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);  //AFIT16_CONTRAST                                                                                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);  //AFIT16_SATURATION                                                                                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);  //AFIT16_SHARP_BLUR                                                                                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);  //AFIT16_GLAMOUR                                                                                       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0064);  //AFIT16_EE_iFlatBoundary                                                                              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0020);  //AFIT16_Yuvemix_mNegRanges_0                                                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0050);  //AFIT16_Yuvemix_mNegRanges_1                                                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0080);  //AFIT16_Yuvemix_mNegRanges_2                                                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0008);  //AFIT16_Yuvemix_mPosRanges_0                                                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0020);  //AFIT16_Yuvemix_mPosRanges_1                                                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0050);  //AFIT16_Yuvemix_mPosRanges_2                                                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2F18);  //AFIT8_Dspcl_edge_low [7:0] AFIT8_Dspcl_edge_high [15:8]                                              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x001A);  //AFIT8_Dspcl_repl_thresh [7:0] AFIT8_Dspcl_iConnectedThresh [15:8]                                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3C24);  //AFIT8_Dspcl_iPlainLevel [7:0] AFIT8_Dspcl_iSatThresh [15:8]                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C01);  //AFIT8_Dspcl_iPlainReference_H [7:0] AFIT8_Dspcl_iVarianceMultThresh_H [15:8]                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x100C);  //AFIT8_Dspcl_iVariancePlainMax_H [7:0] AFIT8_Dspcl_iVarianceLimitMax_H [15:8]                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);  //AFIT8_Dspcl_nClustLevel_C [7:0] AFIT8_Dspcl_iPlainReference_C [15:8]                                 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C0C);  //AFIT8_Dspcl_iVarianceMultThresh_C [7:0] AFIT8_Dspcl_iVariancePlainMax_C [15:8]                       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3010);  //AFIT8_Dspcl_iVarianceLimitMax_C [7:0] AFIT8_EE_iShVLowRegion [15:8]                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x100C);  //AFIT8_EE_iFSmagPosPwrLow [7:0] AFIT8_EE_iFSmagPosPwrHigh [15:8]                                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x100C);  //AFIT8_EE_iFSmagNegPwrLow [7:0] AFIT8_EE_iFSmagNegPwrHigh [15:8]                                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);  //AFIT8_EE_iFSThLow [7:0] AFIT8_EE_iFSThHigh [15:8]                                                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C08);  //AFIT8_EE_iXformTh_High [7:0] AFIT8_EE_iXformTh_Low [15:8]                                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C0C);  //AFIT8_EE_iVLowFSmagPower [7:0] AFIT8_EE_iVLowiXformTh [15:8]                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0610);  //AFIT8_EE_iReduceNoiseRatio [7:0] AFIT8_EE_iFlatSpan [15:8]                                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1008);  //AFIT8_EE_iMSharpenLow [7:0] AFIT8_EE_iMSharpenHigh [15:8]                                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x105A);  //AFIT8_EE_iFlatMean [7:0] AFIT8_EE_iFlatOffset [15:8]                                                 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);  //AFIT8_EE_iMShThLow [7:0] AFIT8_EE_iMShThHigh [15:8]                                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0406);  //AFIT8_EE_iMShDirThLow [7:0] AFIT8_EE_iMShDirThHigh [15:8]                                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0608);  //AFIT8_EE_iMShVLowPwr [7:0] AFIT8_EE_iMShVLowThrld [15:8]                                             
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x180C);  //AFIT8_EE_iWSharpenPosLow [7:0] AFIT8_EE_iWSharpenPosHigh [15:8]                                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x180C);  //AFIT8_EE_iWSharpenNegLow [7:0] AFIT8_EE_iWSharpenNegHigh [15:8]                                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0204);  //AFIT8_EE_iWShThLow [7:0] AFIT8_EE_iWShThHigh [15:8]                                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x040C);  //AFIT8_EE_iWShVLowPwr [7:0] AFIT8_EE_iWShVLowThrld [15:8]                                             
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5A00);  //AFIT8_EE_iReduceNegative [7:0] AFIT8_EE_iRadialLimitSh [15:8]                                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4003);  //AFIT8_EE_iRadialPowerSh [7:0] AFIT8_Bdns_iDispTH_L [15:8]                                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0440);  //AFIT8_Bdns_iDispTH_H [7:0] AFIT8_Bdns_iDispLimit_L [15:8]                                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A04);  //AFIT8_Bdns_iDispLimit_H [7:0] AFIT8_Bdns_iDispTH4HF [15:8]                                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);  //AFIT8_Bdns_iDispLimit4HF_L [7:0] AFIT8_Bdns_iDispLimit4HF_H [15:8]                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x283C);  //AFIT8_Bdns_iDenoiseTH_G_L [7:0] AFIT8_Bdns_iDenoiseTH_G_H [15:8]                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x283C);  //AFIT8_Bdns_iDenoiseTH_NG_L [7:0] AFIT8_Bdns_iDenoiseTH_NG_H [15:8]                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0601);  //AFIT8_Bdns_iDistSigmaMin [7:0] AFIT8_Bdns_iDistSigmaMax [15:8]                                       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3C3C);  //AFIT8_Bdns_iDenoiseTH_Add_Plain [7:0] AFIT8_Bdns_iDenoiseTH_Add_Direc [15:8]                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5004);  //AFIT8_Bdns_iDirConfidenceMin [7:0] AFIT8_Bdns_iDirConfidenceMax [15:8]                               
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x7808);  //AFIT8_Bdns_iPatternTH_MIN [7:0] AFIT8_Bdns_iPatternTH_MAX [15:8]                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3C00);  //AFIT8_Bdns_iNRTune [7:0] AFIT8_Bdns_iLowMaxSlopeAllowed [15:8]                                       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5A3C);  //AFIT8_Bdns_iHighMaxSlopeAllowed [7:0] AFIT8_Bdns_iRadialLimitNR [15:8]                               
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C04);  //AFIT8_Bdns_iRadialPowerNR [7:0] AFIT8_Dmsc_iEnhThresh [15:8]                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0F08);  //AFIT8_Dmsc_iDesatThresh [7:0] AFIT8_Dmsc_iDemBlurLow [15:8]                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x050F);  //AFIT8_Dmsc_iDemBlurHigh [7:0] AFIT8_Dmsc_iDemBlurRange [15:8]                                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8006);  //AFIT8_Dmsc_iDecisionThresh [7:0] AFIT8_Dmsc_iCentGrad [15:8]                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0020);  //AFIT8_Dmsc_iMonochrom [7:0] AFIT8_Dmsc_iGRDenoiseVal [15:8]                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);  //AFIT8_Dmsc_iGBDenoiseVal [7:0] AFIT8_Dmsc_iEdgeDesatThrLow [15:8]                                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1800);  //AFIT8_Dmsc_iEdgeDesatThrHigh [7:0] AFIT8_Dmsc_iEdgeDesat [15:8]                                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);  //AFIT8_Dmsc_iEdgeDesatLimit [7:0] AFIT8_Dmsc_iNearGrayDesat [15:8]                                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xE119);  //AFIT8_Postdmsc_iLowBright [7:0] AFIT8_Postdmsc_iHighBright [15:8]                                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x7D0D);  //AFIT8_Postdmsc_iLowSat [7:0] AFIT8_Postdmsc_iHighSat [15:8]                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1E10);  //AFIT8_Postdmsc_iBCoeff [7:0] AFIT8_Postdmsc_iGCoeff [15:8]                                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1C09);  //AFIT8_Postdmsc_iWideMult [7:0] AFIT8_Postdmsc_iTune [15:8]                                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A00);  //AFIT8_Postdmsc_NoisePower_Low [7:0] AFIT8_Postdmsc_NoisePower_High [15:8]                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A00);  //AFIT8_Postdmsc_NoisePower_VLow [7:0] AFIT8_Postdmsc_NoiseLimit_Low [15:8]                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A0A);  //AFIT8_Postdmsc_NoiseLimit_High [7:0] AFIT8_Postdmsc_iSkinNS [15:8]                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0308);  //AFIT8_Postdmsc_iReduceNS_EdgeTh [7:0] AFIT8_Postdmsc_iReduceNS_Slope [15:8]                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0700);  //AFIT8_Yuvemix_mNegSlopes_0 [7:0] AFIT8_Yuvemix_mNegSlopes_1 [15:8]                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0506);  //AFIT8_Yuvemix_mNegSlopes_2 [7:0] AFIT8_Yuvemix_mNegSlopes_3 [15:8]                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0700);  //AFIT8_Yuvemix_mPosSlopes_0 [7:0] AFIT8_Yuvemix_mPosSlopes_1 [15:8]                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0506);  //AFIT8_Yuvemix_mPosSlopes_2 [7:0] AFIT8_Yuvemix_mPosSlopes_3 [15:8]                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1414);  //AFIT8_Yuviirnr_iYThreshL [7:0] AFIT8_Yuviirnr_iYThreshH [15:8]                                       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1212);  //AFIT8_Yuviirnr_iYNRStrengthL [7:0] AFIT8_Yuviirnr_iYNRStrengthH [15:8]                               
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x080C);  //AFIT8_Yuviirnr_iUVThreshL [7:0] AFIT8_Yuviirnr_iUVThreshH [15:8]                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C10);  //AFIT8_Yuviirnr_iDiffThreshL_UV [7:0] AFIT8_Yuviirnr_iDiffThreshH_UV [15:8]                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0406);  //AFIT8_Yuviirnr_iMaxThreshL_UV [7:0] AFIT8_Yuviirnr_iMaxThreshH_UV [15:8]                             
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1F1F);  //AFIT8_Yuviirnr_iUVNRStrengthL [7:0] AFIT8_Yuviirnr_iUVNRStrengthH [15:8]                             
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8080);  //AFIT8_byr_gras_iShadingPower [7:0] AFIT8_RGBGamma2_iLinearity [15:8]                                 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8000);  //AFIT8_RGBGamma2_iDarkReduce [7:0] AFIT8_ccm_oscar_iSaturation [15:8]                                 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8000);  //AFIT8_RGB2YUV_iYOffset [7:0] AFIT8_RGB2YUV_iRGBGain [15:8]                                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2001);  //AFIT8_Dspcl_nClustLevel_H [7:0] AFIT8_EE_iLowSharpPower [15:8]                                       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0140);  //AFIT8_EE_iHighSharpPower [7:0] AFIT8_Dspcl_nClustLevel_H_Bin [15:8]                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4020);  //AFIT8_EE_iLowSharpPower_Bin [7:0] AFIT8_EE_iHighSharpPower_Bin [15:8]                       

//	AFIT 1     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT16_BRIGHTNESS                                                                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT16_CONTRAST                                                                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT16_SATURATION                                                                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT16_SHARP_BLUR                                                                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT16_GLAMOUR                                                                                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0050);    //AFIT16_EE_iFlatBoundary                                                                             
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0020);    //AFIT16_Yuvemix_mNegRanges_0                                                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0050);    //AFIT16_Yuvemix_mNegRanges_1                                                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0080);    //AFIT16_Yuvemix_mNegRanges_2                                                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0008);    //AFIT16_Yuvemix_mPosRanges_0                                                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0020);    //AFIT16_Yuvemix_mPosRanges_1                                                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0050);    //AFIT16_Yuvemix_mPosRanges_2                                                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2F18);    //AFIT8_Dspcl_edge_low [7:0] AFIT8_Dspcl_edge_high [15:8]                                             
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x001A);    //AFIT8_Dspcl_repl_thresh [7:0] AFIT8_Dspcl_iConnectedThresh [15:8]                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3C24);    //AFIT8_Dspcl_iPlainLevel [7:0] AFIT8_Dspcl_iSatThresh [15:8]                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C01);    //AFIT8_Dspcl_iPlainReference_H [7:0] AFIT8_Dspcl_iVarianceMultThresh_H [15:8]                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x100C);    //AFIT8_Dspcl_iVariancePlainMax_H [7:0] AFIT8_Dspcl_iVarianceLimitMax_H [15:8]                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);    //AFIT8_Dspcl_nClustLevel_C [7:0] AFIT8_Dspcl_iPlainReference_C [15:8]                                
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C0C);    //AFIT8_Dspcl_iVarianceMultThresh_C [7:0] AFIT8_Dspcl_iVariancePlainMax_C [15:8]                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3010);    //AFIT8_Dspcl_iVarianceLimitMax_C [7:0] AFIT8_EE_iShVLowRegion [15:8]                                 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x100C);    //AFIT8_EE_iFSmagPosPwrLow [7:0] AFIT8_EE_iFSmagPosPwrHigh [15:8]                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1410);    //AFIT8_EE_iFSmagNegPwrLow [7:0] AFIT8_EE_iFSmagNegPwrHigh [15:8]                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT8_EE_iFSThLow [7:0] AFIT8_EE_iFSThHigh [15:8]                                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A04);    //AFIT8_EE_iXformTh_High [7:0] AFIT8_EE_iXformTh_Low [15:8]                                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A0C);    //AFIT8_EE_iVLowFSmagPower [7:0] AFIT8_EE_iVLowiXformTh [15:8]                                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0510);    //AFIT8_EE_iReduceNoiseRatio [7:0] AFIT8_EE_iFlatSpan [15:8]                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1408);    //AFIT8_EE_iMSharpenLow [7:0] AFIT8_EE_iMSharpenHigh [15:8]                                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x183C);    //AFIT8_EE_iFlatMean [7:0] AFIT8_EE_iFlatOffset [15:8]                                                
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT8_EE_iMShThLow [7:0] AFIT8_EE_iMShThHigh [15:8]                                                 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0406);    //AFIT8_EE_iMShDirThLow [7:0] AFIT8_EE_iMShDirThHigh [15:8]                                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0608);    //AFIT8_EE_iMShVLowPwr [7:0] AFIT8_EE_iMShVLowThrld [15:8]                                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x140C);    //AFIT8_EE_iWSharpenPosLow [7:0] AFIT8_EE_iWSharpenPosHigh [15:8]                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x180C);    //AFIT8_EE_iWSharpenNegLow [7:0] AFIT8_EE_iWSharpenNegHigh [15:8]                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0204);    //AFIT8_EE_iWShThLow [7:0] AFIT8_EE_iWShThHigh [15:8]                                                 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x040C);    //AFIT8_EE_iWShVLowPwr [7:0] AFIT8_EE_iWShVLowThrld [15:8]                                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5A00);    //AFIT8_EE_iReduceNegative [7:0] AFIT8_EE_iRadialLimitSh [15:8]                                       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4003);    //AFIT8_EE_iRadialPowerSh [7:0] AFIT8_Bdns_iDispTH_L [15:8]                                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0440);    //AFIT8_Bdns_iDispTH_H [7:0] AFIT8_Bdns_iDispLimit_L [15:8]                                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A04);    //AFIT8_Bdns_iDispLimit_H [7:0] AFIT8_Bdns_iDispTH4HF [15:8]                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);    //AFIT8_Bdns_iDispLimit4HF_L [7:0] AFIT8_Bdns_iDispLimit4HF_H [15:8]                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1E28);    //AFIT8_Bdns_iDenoiseTH_G_L [7:0] AFIT8_Bdns_iDenoiseTH_G_H [15:8]                                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1E28);    //AFIT8_Bdns_iDenoiseTH_NG_L [7:0] AFIT8_Bdns_iDenoiseTH_NG_H [15:8]                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0500);    //AFIT8_Bdns_iDistSigmaMin [7:0] AFIT8_Bdns_iDistSigmaMax [15:8]                                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3C3C);    //AFIT8_Bdns_iDenoiseTH_Add_Plain [7:0] AFIT8_Bdns_iDenoiseTH_Add_Direc [15:8]                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3204);    //AFIT8_Bdns_iDirConfidenceMin [7:0] AFIT8_Bdns_iDirConfidenceMax [15:8]                              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5008);    //AFIT8_Bdns_iPatternTH_MIN [7:0] AFIT8_Bdns_iPatternTH_MAX [15:8]                                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    //AFIT8_Bdns_iNRTune [7:0] AFIT8_Bdns_iLowMaxSlopeAllowed [15:8]                                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5A20);    //AFIT8_Bdns_iHighMaxSlopeAllowed [7:0] AFIT8_Bdns_iRadialLimitNR [15:8]                              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C04);    //AFIT8_Bdns_iRadialPowerNR [7:0] AFIT8_Dmsc_iEnhThresh [15:8]                                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0F08);    //AFIT8_Dmsc_iDesatThresh [7:0] AFIT8_Dmsc_iDemBlurLow [15:8]                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x050F);    //AFIT8_Dmsc_iDemBlurHigh [7:0] AFIT8_Dmsc_iDemBlurRange [15:8]                                       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8006);    //AFIT8_Dmsc_iDecisionThresh [7:0] AFIT8_Dmsc_iCentGrad [15:8]                                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0020);    //AFIT8_Dmsc_iMonochrom [7:0] AFIT8_Dmsc_iGRDenoiseVal [15:8]                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT8_Dmsc_iGBDenoiseVal [7:0] AFIT8_Dmsc_iEdgeDesatThrLow [15:8]                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1800);    //AFIT8_Dmsc_iEdgeDesatThrHigh [7:0] AFIT8_Dmsc_iEdgeDesat [15:8]                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT8_Dmsc_iEdgeDesatLimit [7:0] AFIT8_Dmsc_iNearGrayDesat [15:8]                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xE119);    //AFIT8_Postdmsc_iLowBright [7:0] AFIT8_Postdmsc_iHighBright [15:8]                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x7D0D);    //AFIT8_Postdmsc_iLowSat [7:0] AFIT8_Postdmsc_iHighSat [15:8]                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1E10);    //AFIT8_Postdmsc_iBCoeff [7:0] AFIT8_Postdmsc_iGCoeff [15:8]                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1C0B);    //AFIT8_Postdmsc_iWideMult [7:0] AFIT8_Postdmsc_iTune [15:8]                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A02);    //AFIT8_Postdmsc_NoisePower_Low [7:0] AFIT8_Postdmsc_NoisePower_High [15:8]                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C04);    //AFIT8_Postdmsc_NoisePower_VLow [7:0] AFIT8_Postdmsc_NoiseLimit_Low [15:8]                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A0C);    //AFIT8_Postdmsc_NoiseLimit_High [7:0] AFIT8_Postdmsc_iSkinNS [15:8]                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0308);    //AFIT8_Postdmsc_iReduceNS_EdgeTh [7:0] AFIT8_Postdmsc_iReduceNS_Slope [15:8]                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0700);    //AFIT8_Yuvemix_mNegSlopes_0 [7:0] AFIT8_Yuvemix_mNegSlopes_1 [15:8]                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0506);    //AFIT8_Yuvemix_mNegSlopes_2 [7:0] AFIT8_Yuvemix_mNegSlopes_3 [15:8]                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0700);    //AFIT8_Yuvemix_mPosSlopes_0 [7:0] AFIT8_Yuvemix_mPosSlopes_1 [15:8]                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0506);    //AFIT8_Yuvemix_mPosSlopes_2 [7:0] AFIT8_Yuvemix_mPosSlopes_3 [15:8]                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1414);    //AFIT8_Yuviirnr_iYThreshL [7:0] AFIT8_Yuviirnr_iYThreshH [15:8]                                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1212);    //AFIT8_Yuviirnr_iYNRStrengthL [7:0] AFIT8_Yuviirnr_iYNRStrengthH [15:8]                              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x080C);    //AFIT8_Yuviirnr_iUVThreshL [7:0] AFIT8_Yuviirnr_iUVThreshH [15:8]                                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C10);    //AFIT8_Yuviirnr_iDiffThreshL_UV [7:0] AFIT8_Yuviirnr_iDiffThreshH_UV [15:8]                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0406);    //AFIT8_Yuviirnr_iMaxThreshL_UV [7:0] AFIT8_Yuviirnr_iMaxThreshH_UV [15:8]                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1F1F);    //AFIT8_Yuviirnr_iUVNRStrengthL [7:0] AFIT8_Yuviirnr_iUVNRStrengthH [15:8]                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8080);    //AFIT8_byr_gras_iShadingPower [7:0] AFIT8_RGBGamma2_iLinearity [15:8]                                
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8000);    //AFIT8_RGBGamma2_iDarkReduce [7:0] AFIT8_ccm_oscar_iSaturation [15:8]                                
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8000);    //AFIT8_RGB2YUV_iYOffset [7:0] AFIT8_RGB2YUV_iRGBGain [15:8]                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3001);    //AFIT8_Dspcl_nClustLevel_H [7:0] AFIT8_EE_iLowSharpPower [15:8]                                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0140);    //AFIT8_EE_iHighSharpPower [7:0] AFIT8_Dspcl_nClustLevel_H_Bin [15:8]                                 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4030);    //AFIT8_EE_iLowSharpPower_Bin [7:0] AFIT8_EE_iHighSharpPower_Bin [15:8]                               

//AFIT 2    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT16_BRIGHTNESS                                                                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT16_CONTRAST                                                                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT16_SATURATION                                                                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT16_SHARP_BLUR                                                                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT16_GLAMOUR                                                                                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0050);    //AFIT16_EE_iFlatBoundary                                                                             
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0030);    //AFIT16_Yuvemix_mNegRanges_0                                                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0060);    //AFIT16_Yuvemix_mNegRanges_1                                                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0080);    //AFIT16_Yuvemix_mNegRanges_2                                                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x000C);    //AFIT16_Yuvemix_mPosRanges_0                                                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0030);    //AFIT16_Yuvemix_mPosRanges_1                                                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0080);    //AFIT16_Yuvemix_mPosRanges_2                                                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2F18);    //AFIT8_Dspcl_edge_low [7:0] AFIT8_Dspcl_edge_high [15:8]                                             
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0420);    //AFIT8_Dspcl_repl_thresh [7:0] AFIT8_Dspcl_iConnectedThresh [15:8]                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3C24);    //AFIT8_Dspcl_iPlainLevel [7:0] AFIT8_Dspcl_iSatThresh [15:8]                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C01);    //AFIT8_Dspcl_iPlainReference_H [7:0] AFIT8_Dspcl_iVarianceMultThresh_H [15:8]                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x100C);    //AFIT8_Dspcl_iVariancePlainMax_H [7:0] AFIT8_Dspcl_iVarianceLimitMax_H [15:8]                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);    //AFIT8_Dspcl_nClustLevel_C [7:0] AFIT8_Dspcl_iPlainReference_C [15:8]                                
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C0C);    //AFIT8_Dspcl_iVarianceMultThresh_C [7:0] AFIT8_Dspcl_iVariancePlainMax_C [15:8]                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3010);    //AFIT8_Dspcl_iVarianceLimitMax_C [7:0] AFIT8_EE_iShVLowRegion [15:8]                                 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x100C);    //AFIT8_EE_iFSmagPosPwrLow [7:0] AFIT8_EE_iFSmagPosPwrHigh [15:8]                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1410);    //AFIT8_EE_iFSmagNegPwrLow [7:0] AFIT8_EE_iFSmagNegPwrHigh [15:8]                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT8_EE_iFSThLow [7:0] AFIT8_EE_iFSThHigh [15:8]                                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0602);    //AFIT8_EE_iXformTh_High [7:0] AFIT8_EE_iXformTh_Low [15:8]                                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x060C);    //AFIT8_EE_iVLowFSmagPower [7:0] AFIT8_EE_iVLowiXformTh [15:8]                                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0510);    //AFIT8_EE_iReduceNoiseRatio [7:0] AFIT8_EE_iFlatSpan [15:8]                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1008);    //AFIT8_EE_iMSharpenLow [7:0] AFIT8_EE_iMSharpenHigh [15:8]                                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);    //AFIT8_EE_iFlatMean [7:0] AFIT8_EE_iFlatOffset [15:8]                                                
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT8_EE_iMShThLow [7:0] AFIT8_EE_iMShThHigh [15:8]                                                 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0408);    //AFIT8_EE_iMShDirThLow [7:0] AFIT8_EE_iMShDirThHigh [15:8]                                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0808);    //AFIT8_EE_iMShVLowPwr [7:0] AFIT8_EE_iMShVLowThrld [15:8]                                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x100C);    //AFIT8_EE_iWSharpenPosLow [7:0] AFIT8_EE_iWSharpenPosHigh [15:8]                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x140C);    //AFIT8_EE_iWSharpenNegLow [7:0] AFIT8_EE_iWSharpenNegHigh [15:8]                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0204);    //AFIT8_EE_iWShThLow [7:0] AFIT8_EE_iWShThHigh [15:8]                                                 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x040C);    //AFIT8_EE_iWShVLowPwr [7:0] AFIT8_EE_iWShVLowThrld [15:8]                                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5A00);    //AFIT8_EE_iReduceNegative [7:0] AFIT8_EE_iRadialLimitSh [15:8]                                       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4002);    //AFIT8_EE_iRadialPowerSh [7:0] AFIT8_Bdns_iDispTH_L [15:8]                                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0440);    //AFIT8_Bdns_iDispTH_H [7:0] AFIT8_Bdns_iDispLimit_L [15:8]                                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A04);    //AFIT8_Bdns_iDispLimit_H [7:0] AFIT8_Bdns_iDispTH4HF [15:8]                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);    //AFIT8_Bdns_iDispLimit4HF_L [7:0] AFIT8_Bdns_iDispLimit4HF_H [15:8]                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1824);    //AFIT8_Bdns_iDenoiseTH_G_L [7:0] AFIT8_Bdns_iDenoiseTH_G_H [15:8]                                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1824);    //AFIT8_Bdns_iDenoiseTH_NG_L [7:0] AFIT8_Bdns_iDenoiseTH_NG_H [15:8]                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0500);    //AFIT8_Bdns_iDistSigmaMin [7:0] AFIT8_Bdns_iDistSigmaMax [15:8]                                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3C3C);    //AFIT8_Bdns_iDenoiseTH_Add_Plain [7:0] AFIT8_Bdns_iDenoiseTH_Add_Direc [15:8]                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1E04);    //AFIT8_Bdns_iDirConfidenceMin [7:0] AFIT8_Bdns_iDirConfidenceMax [15:8]                              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2808);    //AFIT8_Bdns_iPatternTH_MIN [7:0] AFIT8_Bdns_iPatternTH_MAX [15:8]                                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    //AFIT8_Bdns_iNRTune [7:0] AFIT8_Bdns_iLowMaxSlopeAllowed [15:8]                                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5A20);    //AFIT8_Bdns_iHighMaxSlopeAllowed [7:0] AFIT8_Bdns_iRadialLimitNR [15:8]                              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C02);    //AFIT8_Bdns_iRadialPowerNR [7:0] AFIT8_Dmsc_iEnhThresh [15:8]                                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0F08);    //AFIT8_Dmsc_iDesatThresh [7:0] AFIT8_Dmsc_iDemBlurLow [15:8]                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x050F);    //AFIT8_Dmsc_iDemBlurHigh [7:0] AFIT8_Dmsc_iDemBlurRange [15:8]                                       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8006);    //AFIT8_Dmsc_iDecisionThresh [7:0] AFIT8_Dmsc_iCentGrad [15:8]                                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0020);    //AFIT8_Dmsc_iMonochrom [7:0] AFIT8_Dmsc_iGRDenoiseVal [15:8]                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT8_Dmsc_iGBDenoiseVal [7:0] AFIT8_Dmsc_iEdgeDesatThrLow [15:8]                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1800);    //AFIT8_Dmsc_iEdgeDesatThrHigh [7:0] AFIT8_Dmsc_iEdgeDesat [15:8]                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT8_Dmsc_iEdgeDesatLimit [7:0] AFIT8_Dmsc_iNearGrayDesat [15:8]                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xE119);    //AFIT8_Postdmsc_iLowBright [7:0] AFIT8_Postdmsc_iHighBright [15:8]                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x7D0D);    //AFIT8_Postdmsc_iLowSat [7:0] AFIT8_Postdmsc_iHighSat [15:8]                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1E10);    //AFIT8_Postdmsc_iBCoeff [7:0] AFIT8_Postdmsc_iGCoeff [15:8]                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1C0B);    //AFIT8_Postdmsc_iWideMult [7:0] AFIT8_Postdmsc_iTune [15:8]                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C04);    //AFIT8_Postdmsc_NoisePower_Low [7:0] AFIT8_Postdmsc_NoisePower_High [15:8]                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1004);    //AFIT8_Postdmsc_NoisePower_VLow [7:0] AFIT8_Postdmsc_NoiseLimit_Low [15:8]                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A10);    //AFIT8_Postdmsc_NoiseLimit_High [7:0] AFIT8_Postdmsc_iSkinNS [15:8]                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0308);    //AFIT8_Postdmsc_iReduceNS_EdgeTh [7:0] AFIT8_Postdmsc_iReduceNS_Slope [15:8]                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0700);    //AFIT8_Yuvemix_mNegSlopes_0 [7:0] AFIT8_Yuvemix_mNegSlopes_1 [15:8]                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0506);    //AFIT8_Yuvemix_mNegSlopes_2 [7:0] AFIT8_Yuvemix_mNegSlopes_3 [15:8]                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0700);    //AFIT8_Yuvemix_mPosSlopes_0 [7:0] AFIT8_Yuvemix_mPosSlopes_1 [15:8]                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0506);    //AFIT8_Yuvemix_mPosSlopes_2 [7:0] AFIT8_Yuvemix_mPosSlopes_3 [15:8]                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1414);    //AFIT8_Yuviirnr_iYThreshL [7:0] AFIT8_Yuviirnr_iYThreshH [15:8]                                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C0C);    //AFIT8_Yuviirnr_iYNRStrengthL [7:0] AFIT8_Yuviirnr_iYNRStrengthH [15:8]                              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x080A);    //AFIT8_Yuviirnr_iUVThreshL [7:0] AFIT8_Yuviirnr_iUVThreshH [15:8]                                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C10);    //AFIT8_Yuviirnr_iDiffThreshL_UV [7:0] AFIT8_Yuviirnr_iDiffThreshH_UV [15:8]                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0406);    //AFIT8_Yuviirnr_iMaxThreshL_UV [7:0] AFIT8_Yuviirnr_iMaxThreshH_UV [15:8]                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1C1F);    //AFIT8_Yuviirnr_iUVNRStrengthL [7:0] AFIT8_Yuviirnr_iUVNRStrengthH [15:8]                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8080);    //AFIT8_byr_gras_iShadingPower [7:0] AFIT8_RGBGamma2_iLinearity [15:8]                                
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8000);    //AFIT8_RGBGamma2_iDarkReduce [7:0] AFIT8_ccm_oscar_iSaturation [15:8]                                
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8000);    //AFIT8_RGB2YUV_iYOffset [7:0] AFIT8_RGB2YUV_iRGBGain [15:8]                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3601);    //AFIT8_Dspcl_nClustLevel_H [7:0] AFIT8_EE_iLowSharpPower [15:8]                                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0146);    //AFIT8_EE_iHighSharpPower [7:0] AFIT8_Dspcl_nClustLevel_H_Bin [15:8]                                 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4636);    //AFIT8_EE_iLowSharpPower_Bin [7:0] AFIT8_EE_iHighSharpPower_Bin [15:8]
 
 //AFIT 3    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT16_BRIGHTNESS                                                                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT16_CONTRAST                                                                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT16_SATURATION                                                                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT16_SHARP_BLUR                                                                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT16_GLAMOUR                                                                                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0050);    //AFIT16_EE_iFlatBoundary                                                                             
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0030);    //AFIT16_Yuvemix_mNegRanges_0                                                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0060);    //AFIT16_Yuvemix_mNegRanges_1                                                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0080);    //AFIT16_Yuvemix_mNegRanges_2                                                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x000C);    //AFIT16_Yuvemix_mPosRanges_0                                                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0030);    //AFIT16_Yuvemix_mPosRanges_1                                                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0080);    //AFIT16_Yuvemix_mPosRanges_2                                                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2F18);    //AFIT8_Dspcl_edge_low [7:0] AFIT8_Dspcl_edge_high [15:8]                                             
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0420);    //AFIT8_Dspcl_repl_thresh [7:0] AFIT8_Dspcl_iConnectedThresh [15:8]                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3C20);    //AFIT8_Dspcl_iPlainLevel [7:0] AFIT8_Dspcl_iSatThresh [15:8]                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C01);    //AFIT8_Dspcl_iPlainReference_H [7:0] AFIT8_Dspcl_iVarianceMultThresh_H [15:8]                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x100C);    //AFIT8_Dspcl_iVariancePlainMax_H [7:0] AFIT8_Dspcl_iVarianceLimitMax_H [15:8]                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);    //AFIT8_Dspcl_nClustLevel_C [7:0] AFIT8_Dspcl_iPlainReference_C [15:8]                                
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C0C);    //AFIT8_Dspcl_iVarianceMultThresh_C [7:0] AFIT8_Dspcl_iVariancePlainMax_C [15:8]                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3010);    //AFIT8_Dspcl_iVarianceLimitMax_C [7:0] AFIT8_EE_iShVLowRegion [15:8]                                 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x100C);    //AFIT8_EE_iFSmagPosPwrLow [7:0] AFIT8_EE_iFSmagPosPwrHigh [15:8]                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1810);    //AFIT8_EE_iFSmagNegPwrLow [7:0] AFIT8_EE_iFSmagNegPwrHigh [15:8]                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT8_EE_iFSThLow [7:0] AFIT8_EE_iFSThHigh [15:8]                                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0602);    //AFIT8_EE_iXformTh_High [7:0] AFIT8_EE_iXformTh_Low [15:8]                                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x060C);    //AFIT8_EE_iVLowFSmagPower [7:0] AFIT8_EE_iVLowiXformTh [15:8]                                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0510);    //AFIT8_EE_iReduceNoiseRatio [7:0] AFIT8_EE_iFlatSpan [15:8]                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1008);    //AFIT8_EE_iMSharpenLow [7:0] AFIT8_EE_iMSharpenHigh [15:8]                                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);    //AFIT8_EE_iFlatMean [7:0] AFIT8_EE_iFlatOffset [15:8]                                                
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT8_EE_iMShThLow [7:0] AFIT8_EE_iMShThHigh [15:8]                                                 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0406);    //AFIT8_EE_iMShDirThLow [7:0] AFIT8_EE_iMShDirThHigh [15:8]                                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0608);    //AFIT8_EE_iMShVLowPwr [7:0] AFIT8_EE_iMShVLowThrld [15:8]                                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x140C);    //AFIT8_EE_iWSharpenPosLow [7:0] AFIT8_EE_iWSharpenPosHigh [15:8]                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x180C);    //AFIT8_EE_iWSharpenNegLow [7:0] AFIT8_EE_iWSharpenNegHigh [15:8]                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0204);    //AFIT8_EE_iWShThLow [7:0] AFIT8_EE_iWShThHigh [15:8]                                                 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x040C);    //AFIT8_EE_iWShVLowPwr [7:0] AFIT8_EE_iWShVLowThrld [15:8]                                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5A00);    //AFIT8_EE_iReduceNegative [7:0] AFIT8_EE_iRadialLimitSh [15:8]                                       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4002);    //AFIT8_EE_iRadialPowerSh [7:0] AFIT8_Bdns_iDispTH_L [15:8]                                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0440);    //AFIT8_Bdns_iDispTH_H [7:0] AFIT8_Bdns_iDispLimit_L [15:8]                                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A04);    //AFIT8_Bdns_iDispLimit_H [7:0] AFIT8_Bdns_iDispTH4HF [15:8]                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);    //AFIT8_Bdns_iDispLimit4HF_L [7:0] AFIT8_Bdns_iDispLimit4HF_H [15:8]                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1420);    //AFIT8_Bdns_iDenoiseTH_G_L [7:0] AFIT8_Bdns_iDenoiseTH_G_H [15:8]                                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1420);    //AFIT8_Bdns_iDenoiseTH_NG_L [7:0] AFIT8_Bdns_iDenoiseTH_NG_H [15:8]                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0500);    //AFIT8_Bdns_iDistSigmaMin [7:0] AFIT8_Bdns_iDistSigmaMax [15:8]                                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3C3C);    //AFIT8_Bdns_iDenoiseTH_Add_Plain [7:0] AFIT8_Bdns_iDenoiseTH_Add_Direc [15:8]                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1E04);    //AFIT8_Bdns_iDirConfidenceMin [7:0] AFIT8_Bdns_iDirConfidenceMax [15:8]                              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2808);    //AFIT8_Bdns_iPatternTH_MIN [7:0] AFIT8_Bdns_iPatternTH_MAX [15:8]                                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2000);    //AFIT8_Bdns_iNRTune [7:0] AFIT8_Bdns_iLowMaxSlopeAllowed [15:8]                                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5A20);    //AFIT8_Bdns_iHighMaxSlopeAllowed [7:0] AFIT8_Bdns_iRadialLimitNR [15:8]                              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C02);    //AFIT8_Bdns_iRadialPowerNR [7:0] AFIT8_Dmsc_iEnhThresh [15:8]                                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0F08);    //AFIT8_Dmsc_iDesatThresh [7:0] AFIT8_Dmsc_iDemBlurLow [15:8]                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x050F);    //AFIT8_Dmsc_iDemBlurHigh [7:0] AFIT8_Dmsc_iDemBlurRange [15:8]                                       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8006);    //AFIT8_Dmsc_iDecisionThresh [7:0] AFIT8_Dmsc_iCentGrad [15:8]                                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0020);    //AFIT8_Dmsc_iMonochrom [7:0] AFIT8_Dmsc_iGRDenoiseVal [15:8]                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT8_Dmsc_iGBDenoiseVal [7:0] AFIT8_Dmsc_iEdgeDesatThrLow [15:8]                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1800);    //AFIT8_Dmsc_iEdgeDesatThrHigh [7:0] AFIT8_Dmsc_iEdgeDesat [15:8]                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT8_Dmsc_iEdgeDesatLimit [7:0] AFIT8_Dmsc_iNearGrayDesat [15:8]                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xE119);    //AFIT8_Postdmsc_iLowBright [7:0] AFIT8_Postdmsc_iHighBright [15:8]                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x7D0D);    //AFIT8_Postdmsc_iLowSat [7:0] AFIT8_Postdmsc_iHighSat [15:8]            #endif                             
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1E10);    //AFIT8_Postdmsc_iBCoeff [7:0] AFIT8_Postdmsc_iGCoeff [15:8]                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1C0B);    //AFIT8_Postdmsc_iWideMult [7:0] AFIT8_Postdmsc_iTune [15:8]                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C08);    //AFIT8_Postdmsc_NoisePower_Low [7:0] AFIT8_Postdmsc_NoisePower_High [15:8]                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1406);    //AFIT8_Postdmsc_NoisePower_VLow [7:0] AFIT8_Postdmsc_NoiseLimit_Low [15:8]                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A14);    //AFIT8_Postdmsc_NoiseLimit_High [7:0] AFIT8_Postdmsc_iSkinNS [15:8]                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0308);    //AFIT8_Postdmsc_iReduceNS_EdgeTh [7:0] AFIT8_Postdmsc_iReduceNS_Slope [15:8]                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0700);    //AFIT8_Yuvemix_mNegSlopes_0 [7:0] AFIT8_Yuvemix_mNegSlopes_1 [15:8]                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0506);    //AFIT8_Yuvemix_mNegSlopes_2 [7:0] AFIT8_Yuvemix_mNegSlopes_3 [15:8]                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0700);    //AFIT8_Yuvemix_mPosSlopes_0 [7:0] AFIT8_Yuvemix_mPosSlopes_1 [15:8]                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0506);    //AFIT8_Yuvemix_mPosSlopes_2 [7:0] AFIT8_Yuvemix_mPosSlopes_3 [15:8]                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1414);    //AFIT8_Yuviirnr_iYThreshL [7:0] AFIT8_Yuviirnr_iYThreshH [15:8]                                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C0C);    //AFIT8_Yuviirnr_iYNRStrengthL [7:0] AFIT8_Yuviirnr_iYNRStrengthH [15:8]                              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x080A);    //AFIT8_Yuviirnr_iUVThreshL [7:0] AFIT8_Yuviirnr_iUVThreshH [15:8]                                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C10);    //AFIT8_Yuviirnr_iDiffThreshL_UV [7:0] AFIT8_Yuviirnr_iDiffThreshH_UV [15:8]                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0406);    //AFIT8_Yuviirnr_iMaxThreshL_UV [7:0] AFIT8_Yuviirnr_iMaxThreshH_UV [15:8]                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1C1F);    //AFIT8_Yuviirnr_iUVNRStrengthL [7:0] AFIT8_Yuviirnr_iUVNRStrengthH [15:8]                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8080);    //AFIT8_byr_gras_iShadingPower [7:0] AFIT8_RGBGamma2_iLinearity [15:8]                                
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8000);    //AFIT8_RGBGamma2_iDarkReduce [7:0] AFIT8_ccm_oscar_iSaturation [15:8]                                
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8000);    //AFIT8_RGB2YUV_iYOffset [7:0] AFIT8_RGB2YUV_iRGBGain [15:8]                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3A01);    //AFIT8_Dspcl_nClustLevel_H [7:0] AFIT8_EE_iLowSharpPower [15:8]                                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0146);    //AFIT8_EE_iHighSharpPower [7:0] AFIT8_Dspcl_nClustLevel_H_Bin [15:8]                                 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x463A);    //AFIT8_EE_iLowSharpPower_Bin [7:0] AFIT8_EE_iHighSharpPower_Bin [15:8]
 
  //AFIT 4    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT16_BRIGHTNESS                                                                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT16_CONTRAST                                                                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT16_SATURATION                                                                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT16_SHARP_BLUR                                                                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT16_GLAMOUR                                                                                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x003C);    //AFIT16_EE_iFlatBoundary                                                                             
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0030);    //AFIT16_Yuvemix_mNegRanges_0                                                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0060);    //AFIT16_Yuvemix_mNegRanges_1                                                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0080);    //AFIT16_Yuvemix_mNegRanges_2                                                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x000C);    //AFIT16_Yuvemix_mPosRanges_0                                                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0030);    //AFIT16_Yuvemix_mPosRanges_1                                                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0080);    //AFIT16_Yuvemix_mPosRanges_2                                                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2F18);    //AFIT8_Dspcl_edge_low [7:0] AFIT8_Dspcl_edge_high [15:8]                                             
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0420);    //AFIT8_Dspcl_repl_thresh [7:0] AFIT8_Dspcl_iConnectedThresh [15:8]                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3C20);    //AFIT8_Dspcl_iPlainLevel [7:0] AFIT8_Dspcl_iSatThresh [15:8]                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C01);    //AFIT8_Dspcl_iPlainReference_H [7:0] AFIT8_Dspcl_iVarianceMultThresh_H [15:8]                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x100C);    //AFIT8_Dspcl_iVariancePlainMax_H [7:0] AFIT8_Dspcl_iVarianceLimitMax_H [15:8]                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);    //AFIT8_Dspcl_nClustLevel_C [7:0] AFIT8_Dspcl_iPlainReference_C [15:8]                                
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C0C);    //AFIT8_Dspcl_iVarianceMultThresh_C [7:0] AFIT8_Dspcl_iVariancePlainMax_C [15:8]                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3010);    //AFIT8_Dspcl_iVarianceLimitMax_C [7:0] AFIT8_EE_iShVLowRegion [15:8]                                 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x180C);    //AFIT8_EE_iFSmagPosPwrLow [7:0] AFIT8_EE_iFSmagPosPwrHigh [15:8]                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1810);    //AFIT8_EE_iFSmagNegPwrLow [7:0] AFIT8_EE_iFSmagNegPwrHigh [15:8]                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT8_EE_iFSThLow [7:0] AFIT8_EE_iFSThHigh [15:8]                                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0402);    //AFIT8_EE_iXformTh_High [7:0] AFIT8_EE_iXformTh_Low [15:8]                                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x040C);    //AFIT8_EE_iVLowFSmagPower [7:0] AFIT8_EE_iVLowiXformTh [15:8]                                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x050C);    //AFIT8_EE_iReduceNoiseRatio [7:0] AFIT8_EE_iFlatSpan [15:8]                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x140C);    //AFIT8_EE_iMSharpenLow [7:0] AFIT8_EE_iMSharpenHigh [15:8]                                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0101);    //AFIT8_EE_iFlatMean [7:0] AFIT8_EE_iFlatOffset [15:8]                                                
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT8_EE_iMShThLow [7:0] AFIT8_EE_iMShThHigh [15:8]                                                 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0205);    //AFIT8_EE_iMShDirThLow [7:0] AFIT8_EE_iMShDirThHigh [15:8]                                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x050C);    //AFIT8_EE_iMShVLowPwr [7:0] AFIT8_EE_iMShVLowThrld [15:8]                                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1810);    //AFIT8_EE_iWSharpenPosLow [7:0] AFIT8_EE_iWSharpenPosHigh [15:8]                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2010);    //AFIT8_EE_iWSharpenNegLow [7:0] AFIT8_EE_iWSharpenNegHigh [15:8]                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0102);    //AFIT8_EE_iWShThLow [7:0] AFIT8_EE_iWShThHigh [15:8]                                                 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0210);    //AFIT8_EE_iWShVLowPwr [7:0] AFIT8_EE_iWShVLowThrld [15:8]                                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5A00);    //AFIT8_EE_iReduceNegative [7:0] AFIT8_EE_iRadialLimitSh [15:8]                                       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4001);    //AFIT8_EE_iRadialPowerSh [7:0] AFIT8_Bdns_iDispTH_L [15:8]                                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0240);    //AFIT8_Bdns_iDispTH_H [7:0] AFIT8_Bdns_iDispLimit_L [15:8]                                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0402);    //AFIT8_Bdns_iDispLimit_H [7:0] AFIT8_Bdns_iDispTH4HF [15:8]                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT8_Bdns_iDispLimit4HF_L [7:0] AFIT8_Bdns_iDispLimit4HF_H [15:8]                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C18);    //AFIT8_Bdns_iDenoiseTH_G_L [7:0] AFIT8_Bdns_iDenoiseTH_G_H [15:8]                                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C18);    //AFIT8_Bdns_iDenoiseTH_NG_L [7:0] AFIT8_Bdns_iDenoiseTH_NG_H [15:8]                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0300);    //AFIT8_Bdns_iDistSigmaMin [7:0] AFIT8_Bdns_iDistSigmaMax [15:8]                                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3C32);    //AFIT8_Bdns_iDenoiseTH_Add_Plain [7:0] AFIT8_Bdns_iDenoiseTH_Add_Direc [15:8]                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2802);    //AFIT8_Bdns_iDirConfidenceMin [7:0] AFIT8_Bdns_iDirConfidenceMax [15:8]                              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x2806);    //AFIT8_Bdns_iPatternTH_MIN [7:0] AFIT8_Bdns_iPatternTH_MAX [15:8]                                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1000);    //AFIT8_Bdns_iNRTune [7:0] AFIT8_Bdns_iLowMaxSlopeAllowed [15:8]                                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5A10);    //AFIT8_Bdns_iHighMaxSlopeAllowed [7:0] AFIT8_Bdns_iRadialLimitNR [15:8]                              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0801);    //AFIT8_Bdns_iRadialPowerNR [7:0] AFIT8_Dmsc_iEnhThresh [15:8]                                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0808);    //AFIT8_Dmsc_iDesatThresh [7:0] AFIT8_Dmsc_iDemBlurLow [15:8]                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0508);    //AFIT8_Dmsc_iDemBlurHigh [7:0] AFIT8_Dmsc_iDemBlurRange [15:8]                                       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8006);    //AFIT8_Dmsc_iDecisionThresh [7:0] AFIT8_Dmsc_iCentGrad [15:8]                                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0020);    //AFIT8_Dmsc_iMonochrom [7:0] AFIT8_Dmsc_iGRDenoiseVal [15:8]                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT8_Dmsc_iGBDenoiseVal [7:0] AFIT8_Dmsc_iEdgeDesatThrLow [15:8]                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0C00);    //AFIT8_Dmsc_iEdgeDesatThrHigh [7:0] AFIT8_Dmsc_iEdgeDesat [15:8]                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //AFIT8_Dmsc_iEdgeDesatLimit [7:0] AFIT8_Dmsc_iNearGrayDesat [15:8]                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xE119);    //AFIT8_Postdmsc_iLowBright [7:0] AFIT8_Postdmsc_iHighBright [15:8]                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x7D0D);    //AFIT8_Postdmsc_iLowSat [7:0] AFIT8_Postdmsc_iHighSat [15:8]                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1E10);    //AFIT8_Postdmsc_iBCoeff [7:0] AFIT8_Postdmsc_iGCoeff [15:8]                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1C0B);    //AFIT8_Postdmsc_iWideMult [7:0] AFIT8_Postdmsc_iTune [15:8]                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1008);    //AFIT8_Postdmsc_NoisePower_Low [7:0] AFIT8_Postdmsc_NoisePower_High [15:8]                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1408);    //AFIT8_Postdmsc_NoisePower_VLow [7:0] AFIT8_Postdmsc_NoiseLimit_Low [15:8]                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A14);    //AFIT8_Postdmsc_NoiseLimit_High [7:0] AFIT8_Postdmsc_iSkinNS [15:8]                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0308);    //AFIT8_Postdmsc_iReduceNS_EdgeTh [7:0] AFIT8_Postdmsc_iReduceNS_Slope [15:8]                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0700);    //AFIT8_Yuvemix_mNegSlopes_0 [7:0] AFIT8_Yuvemix_mNegSlopes_1 [15:8]                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0506);    //AFIT8_Yuvemix_mNegSlopes_2 [7:0] AFIT8_Yuvemix_mNegSlopes_3 [15:8]                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0700);    //AFIT8_Yuvemix_mPosSlopes_0 [7:0] AFIT8_Yuvemix_mPosSlopes_1 [15:8]                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0506);    //AFIT8_Yuvemix_mPosSlopes_2 [7:0] AFIT8_Yuvemix_mPosSlopes_3 [15:8]                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1010);    //AFIT8_Yuviirnr_iYThreshL [7:0] AFIT8_Yuviirnr_iYThreshH [15:8]                                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0808);    //AFIT8_Yuviirnr_iYNRStrengthL [7:0] AFIT8_Yuviirnr_iYNRStrengthH [15:8]                              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x080A);    //AFIT8_Yuviirnr_iUVThreshL [7:0] AFIT8_Yuviirnr_iUVThreshH [15:8]                                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x080C);    //AFIT8_Yuviirnr_iDiffThreshL_UV [7:0] AFIT8_Yuviirnr_iDiffThreshH_UV [15:8]                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0406);    //AFIT8_Yuviirnr_iMaxThreshL_UV [7:0] AFIT8_Yuviirnr_iMaxThreshH_UV [15:8]                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x1C1F);    //AFIT8_Yuviirnr_iUVNRStrengthL [7:0] AFIT8_Yuviirnr_iUVNRStrengthH [15:8]                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8080);    //AFIT8_byr_gras_iShadingPower [7:0] AFIT8_RGBGamma2_iLinearity [15:8]                                
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8000);    //AFIT8_RGBGamma2_iDarkReduce [7:0] AFIT8_ccm_oscar_iSaturation [15:8]                                
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8000);    //AFIT8_RGB2YUV_iYOffset [7:0] AFIT8_RGB2YUV_iRGBGain [15:8]                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x4001);    //AFIT8_Dspcl_nClustLevel_H [7:0] AFIT8_EE_iLowSharpPower [15:8]                                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x018C);    //AFIT8_EE_iHighSharpPower [7:0] AFIT8_Dspcl_nClustLevel_H_Bin [15:8]                                 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x8C40);    //AFIT8_EE_iLowSharpPower_Bin [7:0] AFIT8_EE_iHighSharpPower_Bin [15:8]
    
//CAFIT             
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFEE);    //[0]CAFITB_Dspcl_bypass          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x3376);    //[0]CAFITB_EE_bReduceNegative    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0xBC3F);    //[0]CAFITB_Dmsc_bEnhThresh       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0337);    //[0]CAFITB_Postdmsc_bNoiseLimit  

//==================================================================================
// 15.Clock Setting
//==================================================================================     
    S5K5EAYX_write_cmos_sensor(0x002A, 0x01E4);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x5DC0);   
    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0200);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0002);   
     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0006);   //67Mhz
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0086);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0005);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0004);   
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0214);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0002);   
    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0006);    // 81Mhz 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x00A2);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0006);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0004);     
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0228);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0002);    

//==================================================================================
// 16.JPEG Thumnail Setting
//==================================================================================
     
    S5K5EAYX_write_cmos_sensor(0x002A, 0x04B4);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x005F);   //REG_TC_BRC_usPrevQuality      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x005F);   //REG_TC_BRC_usCaptureQuality   

// JPEG Thumnail    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);    //REG_TC_THUMB_Thumb_bActive 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0280);    //REG_TC_THUMB_Thumb_uWidth  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01E0);    //REG_TC_THUMB_Thumb_uHeight 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0005);    //REG_TC_THUMB_Thumb_Format  
    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x12C8);                                       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0054);    	//jpeg_ManualMBCV                 
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0D24);                                       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x001C);    	//senHal_bExtraAddLine            
    S5K5EAYX_write_cmos_sensor(0x002A, 0x02BE);                                       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);    	//REG_TC_GP_bBypassScalerJpg      
    S5K5EAYX_write_cmos_sensor(0x002A, 0x02C4);                                       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    	//REG_TC_GP_bUse1FrameCaptureMode 

//==================================================================================
// 17.Input Size Setting
//==================================================================================
//Input Size    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x028A);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A00);  //REG_TC_GP_PrevReqInputWidth 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0780);  //REG_TC_GP_PrevReqInputHeight
    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0292);    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A00);    //REG_TC_GP_CapReqInputWidth 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0780);    //REG_TC_GP_CapReqInputHeight
    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x029C);     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);    //REG_TC_GP_bUseReqInputInPre
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);    //REG_TC_GP_bUseReqInputInCap
    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x04D0);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A00);    //REG_TC_PZOOM_PrevZoomReqInputWidth    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0780);    //REG_TC_PZOOM_PrevZoomReqInputHeight   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //REG_TC_PZOOM_PrevZoomReqInputWidthOfs 
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //REG_TC_PZOOM_PrevZoomReqInputHeightOfs
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A00);    //REG_TC_PZOOM_CapZoomReqInputWidth     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0780);    //REG_TC_PZOOM_CapZoomReqInputHeight    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //REG_TC_PZOOM_CapZoomReqInputWidthOfs  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);    //REG_TC_PZOOM_CapZoomReqInputHeightOfs 


//==================================================================================
// 18.Preview & Capture Configration Setting
//==================================================================================  
       //Preview config[0]
       //81MHz, 1280x960, Dynamic 7.5~30fps
    S5K5EAYX_write_cmos_sensor(0x002A, 0x02E2);   	// REG_PrevConfigControls_0_                                                
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0500);   //REG_0TC_PCFG_usWidth	1280                                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03C0);   //REG_0TC_PCFG_usHeight	960                                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0005);   //REG_0TC_PCFG_Format		YUV                                                   
    S5K5EAYX_write_cmos_sensor(0x002A, 0x02EC);   //                                                                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);   //REG_0TC_PCFG_OutClkPerPix88			                                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0300);   //REG_0TC_PCFG_uBpp88	

	
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0040);   //REG_0TC_PCFG_PVIMask	
    //S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);   //REG_0TC_PCFG_PVIMask //mipi clk change LP from HS
	
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);   //REG_0TC_PCFG_OIFMask						                                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01E0);   //REG_0TC_PCFG_usJpegPacketSize		                                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);   //REG_0TC_PCFG_usJpegTotalPackets	                                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);   //REG_0TC_PCFG_uClockInd					 81                                         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);   //0: Dynamic, 1:Not Accurate, 2: Fixed                                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);   //0: Dynamic 1: BEST FrameRate(Binning), 2: BEST QUALITY(No Binning)          
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0535);   //REG_0TC_PCFG_usMaxFrTimeMsecMult10 10fps                                    
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x014D);   	//REG_0TC_PCFG_usMinFrTimeMsecMult10 30fps                                  
    S5K5EAYX_write_cmos_sensor(0x002A, 0x030C);   //                                                                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0003);   //REG_0TC_PCFG_uPrevMirror                                                    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0003);   //REG_0TC_PCFG_uCaptureMirror                                                 

       //Preview config[1]
       //81MHz, 1280x960, Dynamic 5~30fps
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0312);  	//REG_PrevConfigControls_1_                                               
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0500);  //REG_1TC_PCFG_usWidth	1280                                                
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03C0);  //REG_1TC_PCFG_usHeight	960                                               
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0005);  //REG_1TC_PCFG_Format		JPEG                                              
    S5K5EAYX_write_cmos_sensor(0x002A, 0x031C);                                                                             
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);  //REG_1TC_PCFG_OutClkPerPix88			                                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0300);  //REG_1TC_PCFG_uBpp88		
    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0040);   //REG_0TC_PCFG_PVIMask	
   //S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);   //REG_0TC_PCFG_PVIMask //mipi clk change LP from HS
	
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);  //REG_1TC_PCFG_OIFMask						                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01E0);  //REG_1TC_PCFG_usJpegPacketSize		                                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);  //REG_1TC_PCFG_usJpegTotalPackets                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);  //REG_1TC_PCFG_uClockInd 81                                                
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);  //0: Dynamic, 1:Not Accurate, 2: Fixed                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);  //0: Dynamic 1: BEST FrameRate(Binning), 2: BEST QUALITY(No Binning)       
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x07D0);  //5fps                                                                     
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x014D);  //30fps                                                                    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x033C);                                                                             
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0003);  //REG_1TC_PCFG_uPrevMirror		                                              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0003);  //REG_1TC_PCFG_uCaptureMirror                                              

       //Preview config[2]
       //81MHz, 1280x960, Fix 30fps
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0342); 	//REG_PrevConfigControls_2_                                            
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0500); //REG_2TC_PCFG_usWidth	1280                                             
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x03C0); //REG_2TC_PCFG_usHeight	960                                              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0005); //REG_2TC_PCFG_Format		YUV                                              
    S5K5EAYX_write_cmos_sensor(0x002A, 0x034C);                                                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100); //REG_2TC_PCFG_OutClkPerPix88			                                       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0300); //REG_2TC_PCFG_uBpp88	
    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0040);   //REG_0TC_PCFG_PVIMask	
    //S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);   //REG_0TC_PCFG_PVIMask //mipi clk change LP from HS
	
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000); //REG_2TC_PCFG_OIFMask						                                       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01E0); //REG_2TC_PCFG_usJpegPacketSize		                                       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000); //REG_2TC_PCFG_usJpegTotalPackets	                                       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001); //REG_2TC_PCFG_uClockInd					41                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0002); //0: Dynamic, 1:Not Accurate, 2: Fixed                                   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001); //0: Dynamic 1: BEST FrameRate(Binning), 2: BEST QUALITY(No Binning)     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x01f4); //REG_2TC_PCFG_usMaxFrTimeMsecMult10 30fps                               
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x014D); //REG_2TC_PCFG_usMinFrTimeMsecMult10 30fps                               
    S5K5EAYX_write_cmos_sensor(0x002A, 0x036C);                                                                          
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0003); //REG_2TC_PCFG_uPrevMirror		                                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0003); //REG_2TC_PCFG_uCaptureMirror                                            

	//Preview config[3]
	//81MHz, 1280x960, fixed 15fps
	S5K5EAYX_write_cmos_sensor(0x002A, 0x0372);
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0500); //REG_3TC_PCFG_usWidth
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x03C0); //REG_3TC_PCFG_usHeight 960 											 
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0005); //REG_3TC_PCFG_Format	
	
	S5K5EAYX_write_cmos_sensor(0x002A, 0x037C); 	 
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100); //REG_3TC_PCFG_OutClkPerPix88																				   
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0300); //REG_3TC_PCFG_uBpp88		
	
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0040);   //REG_0TC_PCFG_PVIMask	
    //S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);   //REG_0TC_PCFG_PVIMask //mipi clk change LP from HS
	
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000); //REG_3TC_PCFG_OIFMask																							   
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x01E0); //REG_3TC_PCFG_usJpegPacketSize 																		   
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000); //REG_3TC_PCFG_usJpegTotalPackets																		   
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001); //REG_3TC_PCFG_uClockInd																						   
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0002); //REG_3TC_PCFG_usFrTimeType 				0: Dynamic, 1:Not Accurate, 2: Fixed								 
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001); //0: Dynamic 1: BEST FrameRate(Binning), 2: BEST QUALITY(No Binning)	 
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x029A); //REG_3TC_PCFG_usMaxFrTimeMsecMult10 15fps																 
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x029A); //REG_3TC_PCFG_usMinFrTimeMsecMult10 15fps																 
	S5K5EAYX_write_cmos_sensor(0x002A, 0x039C); 																										 
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0003); //REG_3TC_PCFG_uPrevMirror																				   
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0003); //REG_3TC_PCFG_uCaptureMirror	

       //Capture config[0] 15~7.5fps
    S5K5EAYX_write_cmos_sensor(0x002A, 0x03D2);  //REG_CapConfigControls
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);	//AE AWB close		  
    //S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);  //AE AWB open            
    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A00);  //REG_0TC_CCFG_usWidth	2560                                                                              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0780);  //REG_0TC_CCFG_usHeight	1920         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0005);  //REG_0TC_CCFG_Format	JPEG    
    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x03DE);                                                                                                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);  //REG_0TC_CCFG_OutClkPerPix88			                                                                      
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0300);  //REG_0TC_CCFG_uBpp88			
    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0040);   //REG_0TC_PCFG_PVIMask	
    //S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);   //REG_0TC_PCFG_PVIMask 
	
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0040);  //REG_0TC_CCFG_OIFMask						  //JPEG8                                                               
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0810);  //REG_0TC_CCFG_usJpegPacketSize		                                                                      
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);  //REG_0TC_CCFG_usJpegTotalPackets	 //SPOOF                                                              

	//S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);  //REG_0TC_CCFG_uClockInd					                                                                        
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);  //REG_0TC_CCFG_uClockInd					                                                                        

	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);  //REG_0TC_CCFG_usFrTimeType					0: Dynamic, 1:Not Accurate, 2: Fixed                                
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0002);  //REG_0TC_CCFG_FrRateQualityType		0: Dynamic 1: BEST FrameRate(Binning), 2: BEST QUALITY(No Binning)    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0535);  //REG_0TC_CCFG_usMaxFrTimeMsecMult10 7.5fps                                                               

	//S5K5EAYX_write_cmos_sensor(0x0F12, 0x0340);  //REG_0TC_CCFG_usMinFrTimeMsecMult10 15fps                                                               
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0535);  //REG_0TC_CCFG_usMinFrTimeMsecMult10 15fps->7.5fps                                                               


    //Capture config[1] 5~15fps
    S5K5EAYX_write_cmos_sensor(0x002A, 0x03FE); 	// REG_CapConfigControls_1_                                                                          
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);	//AE AWB close		  
    //S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);  //AE AWB open            
    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A00); // REG_1TC_CCFG_usWidth	  2560*1920                                                                       
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0780); // REG_1TC_CCFG_usHeight	     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0005); // REG_1TC_CCFG_Format		
    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x040A);                                                                                                           
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100); //REG_1TC_CCFG_OutClkPerPix88			                                                                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0300); //REG_1TC_CCFG_uBpp88		
    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0040);   //REG_0TC_PCFG_PVIMask	
    //S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);   //REG_0TC_PCFG_PVIMask //mipi clk change LP from HS
	
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0040); //REG_1TC_CCFG_OIFMask						  //JPEG8                                                               
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0810); //REG_1TC_CCFG_usJpegPacketSize		                                                                        
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000); //REG_1TC_CCFG_usJpegTotalPackets	 //SPOOF 
    
	//S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);  //REG_1TC_CCFG_uClockInd					                                                                        
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);  //REG_1TC_CCFG_uClockInd					                                                                        
    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000); //REG_1TC_CCFG_usFrTimeType					0: Dynamic, 1:Not Accurate, 2: Fixed                                  
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0002); //REG_1TC_CCFG_FrRateQualityType		0: Dynamic 1: BEST FrameRate(Binning), 2: BEST QUALITY(No Binning)    
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x07D0); //REG_1TC_CCFG_usMaxFrTimeMsecMult10 5fps                                                               

	//S5K5EAYX_write_cmos_sensor(0x0F12, 0x0340); //REG_1TC_CCFG_usMinFrTimeMsecMult10 15fps                                                             
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0535); //REG_1TC_CCFG_usMinFrTimeMsecMult10 15fps->7.5fps                                                             

//WRITE #REG_TC_GP_bUse1FrameCaptureMode 1
//==================================================================================
// 19. Update Default Configuration
//==================================================================================    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x02A0);                                              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);  //REG_TC_GP_ActivePrevConfig                
    S5K5EAYX_write_cmos_sensor(0x002A, 0x02A8);                                              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);  //REG_TC_GP_ActiveCapConfig                 
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0266);                                              
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);   //REG_TC_IPRM_InitParamsUpdated            

//==================================================================================
// 20. Update Sen HW Default(After Sensor init) Directly
//==================================================================================
//WRITE D000F59E  0A6B  // aig_lp_dbs_ptr0
//WRITE D000F42E  00A6  // clp_sl_ctrl[7:6]=00
//WRITE D000F2AA  0020  // gain

//==================================================================================
// 21. Select Cofigration Display
//==================================================================================    
//preview
    S5K5EAYX_write_cmos_sensor(0x002A, 0x02A4);                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);   //REG_TC_GP_PrevOpenAfterChange   
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0288);                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);   //REG_TC_GP_NewConfigSync         
    S5K5EAYX_write_cmos_sensor(0x002A, 0x0278);                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);   //REG_TC_GP_EnablePreview         
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);   //REG_TC_GP_EnablePreviewChanged  

////WRITE	#REG_TC_AF    
    S5K5EAYX_write_cmos_sensor(0x002A, 0x02CA);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);   
    S5K5EAYX_write_cmos_sensor(0x002A, 0x02C8);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0003);   //REG_TC_AF_AfCmd 
 	mdelay(100);  
     S5K5EAYX_write_cmos_sensor(0x002A, 0x02CA);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);   
    S5K5EAYX_write_cmos_sensor(0x002A, 0x02C8);   
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0006);   //REG_TC_AF_AfCmd 
      
	S5K5EAYX_write_cmos_sensor(0x002A, 0x0278);
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);  
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);  
}


/*************************************************************************
* FUNCTION
*    S5K5EAYXReadShutter
*
* DESCRIPTION
*    This function Read Shutter from sensor
*
* PARAMETERS
*    Shutter: integration time
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static kal_uint32 S5K5EAYXReadShutter(void)
{
	kal_uint32 Shutter_lowIndex=0,Shutter_highIndex=0,Shutter=0;
	
	S5K5EAYX_write_cmos_sensor(0x002C,0x2000);
	S5K5EAYX_write_cmos_sensor(0x002E,0x2340);
	Shutter_lowIndex=S5K5EAYX_read_cmos_sensor(0x0F12);

	S5K5EAYX_write_cmos_sensor(0x002C,0x2000);
	S5K5EAYX_write_cmos_sensor(0x002E,0x2342);
	Shutter_highIndex=S5K5EAYX_read_cmos_sensor(0x0F12);

	Shutter = (Shutter_highIndex<<4)|Shutter_lowIndex;
    //SENSORDB("[5EA]: S5K5EAYXReadShutter shutter-org =%d \n",Shutter);

    //shutter valud divide 400 to get exposure time(ms), just get a ratio here;
	Shutter=Shutter/S5K5EA_READ_SHUTTER_RATIO;
	if(Shutter <1)
		Shutter=1;
		
    SENSORDB("[5EA]: S5K5EAYXReadShutter shutter =%d \n",Shutter);

	return Shutter;
}

/*************************************************************************
* FUNCTION
*    S5K5EAYXReadGain
*
* DESCRIPTION
*    This function get gain from sensor
*
* PARAMETERS
*    None
*
* RETURNS
*    Gain: base on 0x40
*
* LOCAL AFFECTED
*
*************************************************************************/
static kal_uint32 S5K5EAYXReadGain(void)
{
	kal_uint32 Reg=0 ;

	S5K5EAYX_write_cmos_sensor(0x002C,0x2000);
	S5K5EAYX_write_cmos_sensor(0x002E,0x2144);
	Reg=S5K5EAYX_read_cmos_sensor(0x0F12);	
    //SENSORDB("[5EA]:S5K5EAYXReadGain gain-org =%d \n",Reg);

    //sensor gain base is 256;
	Reg=Reg/2;
	if(Reg<1)
	{
		Reg=1;
	}
    SENSORDB("[5EA]: S5K5EAYXReadGain gain =%d \n",Reg);

	return Reg; 
}



static kal_uint32 S5K5EAYXReadAwb_RGain(void)
{

	kal_uint32 RGain=0 ;
	
	S5K5EAYX_write_cmos_sensor(0x002C,0x2000);
	S5K5EAYX_write_cmos_sensor(0x002E,0x214c);
	RGain=S5K5EAYX_read_cmos_sensor(0x0F12);	
	//SENSORDB("[5EA]: S5K5EAYXReadAwb_RGain RGain-org =%d \n",RGain);

    //sensor gain base is 1024;
    RGain=RGain/8;

	if(RGain<1)
		RGain=1;
	
	SENSORDB("[5EA]: S5K5EAYXReadAwb_RGain RGain =%d \n",RGain);
	return RGain;
}

static kal_uint32 S5K5EAYXReadAwb_BGain(void)
{

	kal_uint32 BGain=0 ;
	
	S5K5EAYX_write_cmos_sensor(0x002C,0x2000);
	S5K5EAYX_write_cmos_sensor(0x002E,0x2150);
	BGain=S5K5EAYX_read_cmos_sensor(0x0F12);	
	//SENSORDB("[5EA]: S5K5EAYXReadAwb_RGain BGain-org =%d \n",BGain);

    //sensor gain base is 1024;
    BGain=BGain/8;

	if(BGain<1)
		BGain=1;
	
	SENSORDB("[5EA]: S5K5EAYXReadAwb_RGain BGain =%d \n",BGain);
	return BGain;
}



/*************************************************************************
* FUNCTION
*    S5K5EAYXGetEvAwbRef
*
* DESCRIPTION
*    This function get sensor Ev/Awb (EV05/EV13) for auto scene detect
*
* PARAMETERS
*    Ref
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void S5K5EAYXGetEvAwbRef(PSENSOR_AE_AWB_REF_STRUCT Ref)//checked in lab
{
    Ref->SensorAERef.AeRefLV05Shutter = 9000; 
    Ref->SensorAERef.AeRefLV05Gain = 732; /*  128 base */
    Ref->SensorAERef.AeRefLV13Shutter = 238;
    Ref->SensorAERef.AeRefLV13Gain = 128; /*  128 base */
    Ref->SensorAwbGainRef.AwbRefD65Rgain = 172; /* 128 base */
    Ref->SensorAwbGainRef.AwbRefD65Bgain = 142; /* 128 base */
    Ref->SensorAwbGainRef.AwbRefCWFRgain = 159; /* 128 base */
    Ref->SensorAwbGainRef.AwbRefCWFBgain = 256;/* 128 base */
}
/*************************************************************************
* FUNCTION
*    S5K5EAYXGetCurAeAwbInfo
*
* DESCRIPTION
*    This function get sensor cur Ae/Awb for auto scene detect
*
* PARAMETERS
*    Info
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void S5K5EAYXGetCurAeAwbInfo(PSENSOR_AE_AWB_CUR_STRUCT Info)
{
    Info->SensorAECur.AeCurShutter = S5K5EAYXReadShutter();
    Info->SensorAECur.AeCurGain = S5K5EAYXReadGain(); /* 128 base */

	Info->SensorAwbGainCur.AwbCurRgain=S5K5EAYXReadAwb_RGain; /* 128 base */
    Info->SensorAwbGainCur.AwbCurBgain = S5K5EAYXReadAwb_BGain; /* 128 base */
}


static void S5K5EAYX_Get_AF_Max_Num_Focus_Areas(UINT32 *pFeatureReturnPara32)
{ 	
    
    *pFeatureReturnPara32 = 0;    
    SENSORDB("[5EA]: S5K5EAYX_Get_AF_Max_Num_Focus_Areas: *pFeatureReturnPara32 = %d\n",  *pFeatureReturnPara32);	
}

static void S5K5EAYX_Get_AE_Max_Num_Metering_Areas(UINT32 *pFeatureReturnPara32)
{ 	
    
    *pFeatureReturnPara32 = 0; 
    SENSORDB("[5EA]: S5K5EAYX_Get_AE_Max_Num_Metering_Areas: *pFeatureReturnPara32 = %d\n",  *pFeatureReturnPara32);	
}


void S5K5EAYX_CaptureCfg0_FixFps(UINT16 FixFps)
{
	UINT16 ExposureTime =0;

	if(FixFps<5||FixFps>15)
		return;

    ExposureTime = 10000/FixFps;
	
	//Capture config[0] (org :15~7.5fps)==>fixfps
	S5K5EAYX_write_cmos_sensor(0x002A, 0x03D2);  //REG_CapConfigControls

	S5K5EAYX_write_cmos_sensor(0x0F12, S5K5EAYX_CAP_AEAWB_OPEN);  //AE AWB open	for zsd		

	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A00);  //REG_0TC_CCFG_usWidth 2560																			  
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0780);  //REG_0TC_CCFG_usHeight	1920		 
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0005);  //REG_0TC_CCFG_Format	JPEG	
	
	S5K5EAYX_write_cmos_sensor(0x002A, 0x03DE); 																										  
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);  //REG_0TC_CCFG_OutClkPerPix88																				  
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0300);  //REG_0TC_CCFG_uBpp88			
	
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0040);   //REG_0TC_PCFG_PVIMask	
	
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0040);  //REG_0TC_CCFG_OIFMask 					  //JPEG8																
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0810);  //REG_0TC_CCFG_usJpegPacketSize																			  
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);  //REG_0TC_CCFG_usJpegTotalPackets	 //SPOOF															  

	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);  //REG_0TC_CCFG_uClockInd																							

	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);  //REG_0TC_CCFG_usFrTimeType					0: Dynamic, 1:Not Accurate, 2: Fixed								
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0002);  //REG_0TC_CCFG_FrRateQualityType		0: Dynamic 1: BEST FrameRate(Binning), 2: BEST QUALITY(No Binning)	  

	S5K5EAYX_write_cmos_sensor(0x0F12, ExposureTime);  //REG_0TC_CCFG_usMaxFrTimeMsecMult10 10fps 															  
	S5K5EAYX_write_cmos_sensor(0x0F12, ExposureTime);  //REG_0TC_CCFG_usMinFrTimeMsecMult10 15fps 															  
}


void S5K5EAYX_CaptureCfg0_FixFps_Restore(void)
{
	//Capture config[0] (org :15~7.5fps)
	S5K5EAYX_write_cmos_sensor(0x002A, 0x03D2);  //REG_CapConfigControls

	S5K5EAYX_write_cmos_sensor(0x0F12, S5K5EAYX_CAP_AEAWB_OPEN);  //AE AWB unlock for zsd	

	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0A00);  //REG_0TC_CCFG_usWidth 2560																			  
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0780);  //REG_0TC_CCFG_usHeight	1920		 
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0005);  //REG_0TC_CCFG_Format	JPEG	
	
	S5K5EAYX_write_cmos_sensor(0x002A, 0x03DE); 																										  
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);  //REG_0TC_CCFG_OutClkPerPix88																				  
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0300);  //REG_0TC_CCFG_uBpp88			
	
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0040);   //REG_0TC_PCFG_PVIMask	
	
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0040);  //REG_0TC_CCFG_OIFMask 					  //JPEG8																
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0810);  //REG_0TC_CCFG_usJpegPacketSize																			  
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);  //REG_0TC_CCFG_usJpegTotalPackets	 //SPOOF															  

	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);  //REG_0TC_CCFG_uClockInd																							

	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);  //REG_0TC_CCFG_usFrTimeType					0: Dynamic, 1:Not Accurate, 2: Fixed								
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0002);  //REG_0TC_CCFG_FrRateQualityType		0: Dynamic 1: BEST FrameRate(Binning), 2: BEST QUALITY(No Binning)	  

	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0535);  //REG_0TC_CCFG_usMaxFrTimeMsecMult10 7.5fps 															  
	S5K5EAYX_write_cmos_sensor(0x0F12, 0x0535);  //REG_0TC_CCFG_usMinFrTimeMsecMult10 15fps->7.5fps 															  
}


void S5K5EAYX_CaptureCfg_Close_AEAWB(bool status)
{
	   //Capture config[0] 15~7.5fps
	S5K5EAYX_write_cmos_sensor(0x002A, 0x03D2);  //REG_CapConfigControls
	if(status == S5K5EAYX_CAP_AEAWB_CLOSE)
		S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000); //AE AWB close		
	else
	    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);  //AE AWB open			

	//Capture config[1] 5~15fps
	S5K5EAYX_write_cmos_sensor(0x002A, 0x03FE); 	// REG_CapConfigControls_1_ 																		 
	if(status == S5K5EAYX_CAP_AEAWB_CLOSE)
		S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000); //AE AWB close		
	else
	    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);  //AE AWB open			

}


static void S5K5EAYX_Mode_Config(kal_bool NightModeEnable)
{
	SENSORDB("[5EA]:S5K5EAYX_Mode_Config: nightmode=%d\r\n", NightModeEnable);
    kal_uint8 ConfigIndex =0,readout_index=0;

	spin_lock(&s5k5eayx_drv_lock);
	S5K5EAYX_CurrentStatus.iNightMode = NightModeEnable;
	spin_unlock(&s5k5eayx_drv_lock);

    if((s5k5eayx_sensor_mode == S5K5EAYX_SENSORMODE_PREVIEW)||
		(s5k5eayx_sensor_mode == S5K5EAYX_SENSORMODE_VIDEO))
	{
	    if(NightModeEnable)
	    {
	       if(S5K5EAYX_video_mode == KAL_FALSE)
		   	 ConfigIndex = 1;//night  preview mode 5~30fps
		   else
			 ConfigIndex = 3;//night  video  mode 15fps

		}
		else
		{
			if(S5K5EAYX_video_mode == KAL_FALSE)
			  ConfigIndex = 0;//normal  preview mode 10~30fps
			else
			  ConfigIndex = 2;//normal  video mode 30fps
		}
		SENSORDB("[5EA]:S5K5EAYX_Mode_Config(preview or video): ConfigIndex=%d\r\n", ConfigIndex);

		S5K5EAYX_write_cmos_sensor(0xFCFC,0xd000);
		S5K5EAYX_write_cmos_sensor(0x0028,0x2000);

	    S5K5EAYX_write_cmos_sensor(0x002A, 0x02A0);   
		//night :5-30fps Index 1
	    //normal :10~30fps Index 0
	    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000+ConfigIndex);  //REG_TC_GP_ActivePrevConfig 
		S5K5EAYX_write_cmos_sensor(0x002A, 0x02A2); 									
		S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);    
	    S5K5EAYX_write_cmos_sensor(0x002A, 0x02A4);                                     
	    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);   //REG_TC_GP_PrevOpenAfterChange   
	    S5K5EAYX_write_cmos_sensor(0x002A, 0x0288);                                     
	    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);   //REG_TC_GP_NewConfigSync      
	    
	    S5K5EAYX_write_cmos_sensor(0x002A, 0x0278);                                     
	    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);   //REG_TC_GP_EnablePreview         
	    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);   //REG_TC_GP_EnablePreviewChanged
		mdelay(50);
	}
	else if((s5k5eayx_sensor_mode == S5K5EAYX_SENSORMODE_CAPTURE)||
		(s5k5eayx_sensor_mode == S5K5EAYX_SENSORMODE_ZSD))
	{
	    if(NightModeEnable)
			ConfigIndex = 1; //7.5~15 FPS->5~7.5FPS
		else
			ConfigIndex = 0; //10~15 FPS->7.5FPS
		
		SENSORDB(" [5EA]:S5K5EAYX_Mode_Config(capture or ZSD): ConfigIndex=%d\r\n", ConfigIndex);
		
		S5K5EAYX_write_cmos_sensor(0xFCFC, 0xd000);
		S5K5EAYX_write_cmos_sensor(0x0028, 0x2000);
		
		S5K5EAYX_write_cmos_sensor(0x002a, 0x02A8);
		S5K5EAYX_write_cmos_sensor(0x0f12, 0x0000+ConfigIndex);	
		
		S5K5EAYX_write_cmos_sensor(0x002A, 0x02AA);
		S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
		
		S5K5EAYX_write_cmos_sensor(0x002A, 0x0288);
		S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
		
		S5K5EAYX_write_cmos_sensor(0x002A, 0x027C);
		S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
		S5K5EAYX_write_cmos_sensor(0x002A, 0x027E);
		S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001); 
		mdelay(50);
	}
}


kal_bool S5K5EAYX_CheckHDRstatus(void)
{
    if((S5K5EAYX_CurrentStatus.iSceneMode == SCENE_MODE_HDR)
		&&(s5k5eayx_sensor_mode == S5K5EAYX_SENSORMODE_CAPTURE))
    {
		return KAL_TRUE;
	}
	else 
		return KAL_FALSE;
}

void S5K5EAYXHdr_Part1(void)
{
	//read org shutter/gain
    kal_uint32 shutter=0,gain=0;

	shutter= S5K5EAYXReadShutter();
	gain= S5K5EAYXReadGain()*2;
	
	spin_lock(&s5k5eayx_drv_lock);
	S5K5EAYX_CurrentStatus.iShutter= shutter;
	S5K5EAYX_CurrentStatus.iGain= gain;
	spin_unlock(&s5k5eayx_drv_lock);

     SENSORDB("[5EA]: S5K5EAYXHdr_Part1:S5K5EAYX_CurrentStatus.iShutter = %d,S5K5EAYX_CurrentStatus.iGain = %d\r\n",  
	 	       S5K5EAYX_CurrentStatus.iShutter,S5K5EAYX_CurrentStatus.iGain);	

}

void S5K5EAYXHdr_Part2(UINT16 para)
{

	kal_uint32 HDRShutter =0,HDRGain=0;

    //multiply 100 could get exposure time(ms) for writing to register;
    //use a ratio here ,corresponding to ReadShutter();
	HDRShutter = S5K5EAYX_CurrentStatus.iShutter*S5K5EA_SET_SHUTTER_RATIO;
	HDRGain = S5K5EAYX_CurrentStatus.iGain*S5K5EA_SET_GAIN_RATIO;

	switch (para)
	{
	   case AE_EV_COMP_20:
       	   case AE_EV_COMP_10:
		   HDRGain = HDRGain<<1;
           	   HDRShutter = HDRShutter<<1;
           	   SENSORDB("[5EA]:[S5K5EAYX] HDR AE+20\n");
		 	break;
	   case AE_EV_COMP_00:
           	   SENSORDB("[5EA]:[S5K5EAYX] HDR AE00\n");
		 	break;
	   case AE_EV_COMP_n10:
	   case AE_EV_COMP_n20:
		   HDRGain = HDRGain >> 1;
           	   HDRShutter = HDRShutter >> 1;
           	   SENSORDB("[5EA]:[S5K5EAYX] HDR AE-20\n");
		 	break;
	   default:
		 break; 
	}

    //shutter is limited by fps,200ms*100=>0x4E20
    //Total 10x gain, 10*256=>0x0A00
	HDRShutter = (HDRShutter<0x4E20)? HDRShutter:0x4E20;
	HDRGain = (HDRGain<0x0A00)? HDRGain:0x0A00;

	SENSORDB("[5EA]: S5K5EAYXHdr_Part2:HDRShutter = %d,HDRGain = %d\r\n",HDRShutter,HDRGain);   

	//Write shutter
    S5K5EAYX_write_cmos_sensor(0x0028,0x2000);
	S5K5EAYX_write_cmos_sensor(0x002A,0x051C);
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0779);
	
	S5K5EAYX_write_cmos_sensor(0x002A,0x04E8);
	S5K5EAYX_write_cmos_sensor(0x0F12,HDRShutter & 0x0000ffff);
	S5K5EAYX_write_cmos_sensor(0x002A,0x04EA);
	S5K5EAYX_write_cmos_sensor(0x0F12,HDRShutter & 0xffff0000);
	
	S5K5EAYX_write_cmos_sensor(0x002A,0x04EC);
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0001);
	
    //Write Gain
    S5K5EAYX_write_cmos_sensor(0x0028,0x2000);
	S5K5EAYX_write_cmos_sensor(0x002A,0x051C);
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0779);
	
	S5K5EAYX_write_cmos_sensor(0x002A,0x0504);
	S5K5EAYX_write_cmos_sensor(0x0F12,HDRGain);
	S5K5EAYX_write_cmos_sensor(0x002A,0x0506);
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0001);

	spin_lock(&s5k5eayx_drv_lock);
	S5K5EAYX_CurrentStatus.HDRFixed_AE_Done=KAL_TRUE;
	spin_unlock(&s5k5eayx_drv_lock);

    //debug mode
	//SENSORDB(" [5EA]:part2 S5K5EAYXHdr_Part2 debug:HDRShutter = %d,HDRGain = %d\r\n",HDRShutter,HDRGain);   
    //mdelay(500);
	//S5K5EAYXReadShutter();
	//S5K5EAYXReadGain();
}


void S5K5EAYXHdr_Part3()
{
	SENSORDB("[5EA]:S5K5EAYXHdr_Part3:\r\n");   
    S5K5EAYX_CurrentStatus.HDRFixed_AE_Done=KAL_FALSE;

    //open auto AE
	S5K5EAYX_write_cmos_sensor(0x0028,0x2000);
	S5K5EAYX_write_cmos_sensor(0x002A,0x051C);
	S5K5EAYX_write_cmos_sensor(0x0F12,0x077F);
		
	spin_lock(&s5k5eayx_drv_lock);
	S5K5EAYX_CurrentStatus.iShutter= 0;
	S5K5EAYX_CurrentStatus.iGain= 0;
	spin_unlock(&s5k5eayx_drv_lock);
}



/*************************************************************************
* FUNCTION
*	S5K5EAOpen
*
* DESCRIPTION
*	This function initialize the registers of CMOS sensor
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
 
UINT32 S5K5EAYXOpen(void)
{
	kal_uint16 sensor_id=0;
  
    SENSORDB("[5EA]:S5K5EAYXOpen :\r\n");
    
    S5K5EAYX_write_cmos_sensor(0xFCFC,0xD000);
    S5K5EAYX_write_cmos_sensor(0x002C,0x2000);
    S5K5EAYX_write_cmos_sensor(0x002E,0x0000);//id register
	sensor_id = S5K5EAYX_read_cmos_sensor(0x0F12);
	SENSORDB("[5EA]:Read Sensor ID = %x\n", sensor_id); 
	
    if (sensor_id != S5K5EAYX_SENSOR_ID)
        return ERROR_SENSOR_CONNECT_FAIL;

	S5K5EAYX_InitialPara();
    S5K5EAYX_Init_Setting();

	return ERROR_NONE;
}	/* S5K5EAYXOpen() */


UINT32 S5K5EAYXGetSensorID(UINT32 *sensorID)
{
	int  retry = 2; 
    SENSORDB("[5EA]: S5K5EAYXGetSensorID \n");
	
	do {
	    S5K5EAYX_write_cmos_sensor(0xFCFC,0xD000);
	    S5K5EAYX_write_cmos_sensor(0x002C,0x2000);
	    S5K5EAYX_write_cmos_sensor(0x002E,0x0000);//id register
		*sensorID = S5K5EAYX_read_cmos_sensor(0x0F12);	
		
		if (*sensorID == S5K5EAYX_SENSOR_ID)
			break; 
		SENSORDB("[5EA]:Read Sensor ID Fail = %x\n", *sensorID); 
		retry--; 
	} while (retry > 0);

	if (*sensorID != S5K5EAYX_SENSOR_ID) {
		*sensorID = 0xFFFFFFFF; 
		return ERROR_SENSOR_CONNECT_FAIL;
	}
    
	return ERROR_NONE;
}	/* S5K5EAYXGetSensorID() */


/*************************************************************************
* FUNCTION
*	S5K5EAYXClose
*
* DESCRIPTION
*	This function is to turn off sensor module power.
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 S5K5EAYXClose(void)
{

	return ERROR_NONE;
}	


static void S5K5EAYX_HVMirror(kal_uint8 image_mirror)
{
/********************************************************
Preview:Mirror: 0x02d0 bit[0],Flip :    0x02d0 bit[1]
Capture:Mirror: 0x02d2 bit[0],Flip :    0x02d2 bit[1]
*********************************************************/

    SENSORDB("[5EA]:[Enter]:Mirror = image_mirror %d \r\n",image_mirror);
	S5K5EAYX_write_cmos_sensor(0xFCFC,0xd000);
	S5K5EAYX_write_cmos_sensor(0x0028,0x2000);

	switch (0) {
	case IMAGE_HV_MIRROR:
		SENSORDB("case 3 mirror +flip");
			S5K5EAYX_write_cmos_sensor(0x002A,	0x030C);
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0000);	//#REG_0TC_PCFG_uPrevMirror
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0000);	//#REG_0TC_PCFG_uCaptureMirror


			S5K5EAYX_write_cmos_sensor(0x002A,	0x033C);
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0000);	//#REG_1TC_PCFG_uPrevMirror
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0000);	//#REG_1TC_PCFG_uCaptureMirror
			
			S5K5EAYX_write_cmos_sensor(0x002A,	0x036C);
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0000);	//#REG_2TC_PCFG_uPrevMirror
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0000);	//#REG_2TC_PCFG_uCaptureMirror
			
			S5K5EAYX_write_cmos_sensor(0x002A,	0x039C);
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0000);	//#REG_3TC_PCFG_uPrevMirror
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0000);	//#REG_3TC_PCFG_uCaptureMirror

			S5K5EAYX_write_cmos_sensor(0x002A,	0x03CC);
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0000);	//#REG_4TC_PCFG_uPrevMirror
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0000);	//#REG_4TC_PCFG_uCaptureMirror

            break;
		case IMAGE_H_MIRROR:
			S5K5EAYX_write_cmos_sensor(0x002A,	0x030C);
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0001);	//#REG_0TC_PCFG_uPrevMirror
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0001);	//#REG_0TC_PCFG_uCaptureMirror


			S5K5EAYX_write_cmos_sensor(0x002A,	0x033C);
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0001);	//#REG_1TC_PCFG_uPrevMirror
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0001);	//#REG_1TC_PCFG_uCaptureMirror
			
			S5K5EAYX_write_cmos_sensor(0x002A,	0x036C);
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0001);	//#REG_2TC_PCFG_uPrevMirror
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0001);	//#REG_2TC_PCFG_uCaptureMirror
			
			S5K5EAYX_write_cmos_sensor(0x002A,	0x039C);
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0001);	//#REG_3TC_PCFG_uPrevMirror
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0001);	//#REG_3TC_PCFG_uCaptureMirror

			S5K5EAYX_write_cmos_sensor(0x002A,	0x03CC);
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0001);	//#REG_4TC_PCFG_uPrevMirror
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0001);	//#REG_4TC_PCFG_uCaptureMirror
            break;
		case IMAGE_V_MIRROR:
			S5K5EAYX_write_cmos_sensor(0x002A,	0x030C);
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0002);	//#REG_0TC_PCFG_uPrevMirror
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0002);	//#REG_0TC_PCFG_uCaptureMirror


			S5K5EAYX_write_cmos_sensor(0x002A,	0x033C);
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0002);	//#REG_1TC_PCFG_uPrevMirror
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0002);	//#REG_1TC_PCFG_uCaptureMirror
			
			S5K5EAYX_write_cmos_sensor(0x002A,	0x036C);
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0002);	//#REG_2TC_PCFG_uPrevMirror
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0002);	//#REG_2TC_PCFG_uCaptureMirror
			
			S5K5EAYX_write_cmos_sensor(0x002A,	0x039C);
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0002);	//#REG_3TC_PCFG_uPrevMirror
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0002);	//#REG_3TC_PCFG_uCaptureMirror

			S5K5EAYX_write_cmos_sensor(0x002A,	0x03CC);
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0002);	//#REG_4TC_PCFG_uPrevMirror
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0002);	//#REG_4TC_PCFG_uCaptureMirror
            break;
		case IMAGE_NORMAL:
			SENSORDB("case 0 normal");
			S5K5EAYX_write_cmos_sensor(0x002A,	0x030C);
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0003);	//#REG_0TC_PCFG_uPrevMirror
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0003);	//#REG_0TC_PCFG_uCaptureMirror


			S5K5EAYX_write_cmos_sensor(0x002A,	0x033C);
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0003);	//#REG_1TC_PCFG_uPrevMirror
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0003);	//#REG_1TC_PCFG_uCaptureMirror
			
			S5K5EAYX_write_cmos_sensor(0x002A,	0x036C);
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0003);	//#REG_2TC_PCFG_uPrevMirror
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0003);	//#REG_2TC_PCFG_uCaptureMirror
			
			S5K5EAYX_write_cmos_sensor(0x002A,	0x039C);
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0003);	//#REG_3TC_PCFG_uPrevMirror
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0003);	//#REG_3TC_PCFG_uCaptureMirror

			S5K5EAYX_write_cmos_sensor(0x002A,	0x03CC);
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0003);	//#REG_4TC_PCFG_uPrevMirror
			S5K5EAYX_write_cmos_sensor(0x0F12,	0x0003);	//#REG_4TC_PCFG_uCaptureMirror
            break;
	}
       
    S5K5EAYX_write_cmos_sensor(0x002A, 0x027A);                                     
    S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);   //REG_TC_GP_EnablePreviewChanged
    
}


/*************************************************************************
* FUNCTION
*	S5K5EAYXPreview
*
* DESCRIPTION
*	This function start the sensor preview.
*
* PARAMETERS
*	*image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static UINT32 S5K5EAYXPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{	
     
	SENSORDB("[5EA]:S5K5EAYX preview func:\n ");	

	spin_lock(&s5k5eayx_drv_lock);
	S5K5EAYX_video_mode = KAL_FALSE;
	s5k5eayx_sensor_mode = S5K5EAYX_SENSORMODE_PREVIEW;
	spin_unlock(&s5k5eayx_drv_lock);

    S5K5EAYX_Mode_Config(S5K5EAYX_CurrentStatus.iNightMode);
	S5K5EAYX_HVMirror(sensor_config_data->SensorImageMirror);

	image_window->ExposureWindowWidth = S5K5EAYX_IMAGE_SENSOR_PV_WIDTH;
	image_window->ExposureWindowHeight = S5K5EAYX_IMAGE_SENSOR_PV_HEIGHT;
	
	memcpy(&S5K5EAYXSensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    
    return ERROR_NONE; 
}	/* S5K5EAYXPreview */



static UINT32 S5K5EAYXVideo(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{	
     
	SENSORDB("[5EA]:S5K5EAYX video func:\n ");	

	spin_lock(&s5k5eayx_drv_lock);
	S5K5EAYX_video_mode = KAL_TRUE;
	s5k5eayx_sensor_mode = S5K5EAYX_SENSORMODE_VIDEO;
	spin_unlock(&s5k5eayx_drv_lock);
	
    S5K5EAYX_Mode_Config(S5K5EAYX_CurrentStatus.iNightMode);
	S5K5EAYX_HVMirror(sensor_config_data->SensorImageMirror);

	image_window->ExposureWindowWidth = S5K5EAYX_IMAGE_SENSOR_PV_WIDTH;
	image_window->ExposureWindowHeight = S5K5EAYX_IMAGE_SENSOR_PV_HEIGHT;
	
	memcpy(&S5K5EAYXSensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    
    return ERROR_NONE; 
}	/* S5K5EAYXVideo */



UINT32 S5K5EAYXCapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

		SENSORDB("[5EA]:S5K5EAYXCapture func:\n ");	  

		spin_lock(&s5k5eayx_drv_lock);
		S5K5EAYX_video_mode = KAL_FALSE;
		spin_unlock(&s5k5eayx_drv_lock);
		
		if((s5k5eayx_sensor_mode == S5K5EAYX_SENSORMODE_PREVIEW)
			||(s5k5eayx_sensor_mode == S5K5EAYX_SENSORMODE_VIDEO))
		{
			spin_lock(&s5k5eayx_drv_lock);
			s5k5eayx_sensor_mode = S5K5EAYX_SENSORMODE_CAPTURE;//for mode config
			spin_unlock(&s5k5eayx_drv_lock);

			S5K5EAYX_CaptureCfg_Close_AEAWB(S5K5EAYX_CAP_AEAWB_CLOSE);
			S5K5EAYX_Mode_Config(S5K5EAYX_CurrentStatus.iNightMode);
		}

		S5K5EAYX_HVMirror(sensor_config_data->SensorImageMirror);

		image_window->GrabStartX = S5K5EAYX_FULL_X_START;
		image_window->GrabStartY = S5K5EAYX_FULL_Y_START;
		image_window->ExposureWindowWidth = S5K5EAYX_IMAGE_SENSOR_FULL_WIDTH;
		image_window->ExposureWindowHeight = S5K5EAYX_IMAGE_SENSOR_FULL_HEIGHT;

		spin_lock(&s5k5eayx_drv_lock);
		s5k5eayx_sensor_mode = S5K5EAYX_SENSORMODE_CAPTURE;//for zsd HDR
		spin_unlock(&s5k5eayx_drv_lock);
		
        if(S5K5EAYX_CheckHDRstatus())//capture & ZSD mode could get in
        {
            S5K5EAYXHdr_Part1();
		}

		return ERROR_NONE; 
}


UINT32 S5K5EAYXZsd(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

		SENSORDB("[5EA]: S5K5EAYXZsd func:\n ");	  
		
		spin_lock(&s5k5eayx_drv_lock);
		S5K5EAYX_video_mode = KAL_FALSE;
		s5k5eayx_sensor_mode = S5K5EAYX_SENSORMODE_ZSD;
		spin_unlock(&s5k5eayx_drv_lock);

		S5K5EAYX_CaptureCfg_Close_AEAWB(S5K5EAYX_CAP_AEAWB_OPEN);
		S5K5EAYX_Mode_Config(S5K5EAYX_CurrentStatus.iNightMode);

		S5K5EAYX_HVMirror(sensor_config_data->SensorImageMirror);

		image_window->GrabStartX = S5K5EAYX_FULL_X_START;
		image_window->GrabStartY = S5K5EAYX_FULL_Y_START;
		image_window->ExposureWindowWidth = S5K5EAYX_IMAGE_SENSOR_FULL_WIDTH;
		image_window->ExposureWindowHeight = S5K5EAYX_IMAGE_SENSOR_FULL_HEIGHT;
		
		return ERROR_NONE; 
}



UINT32 S5K5EAYXGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
    SENSORDB("[5EA]:S5K5EAYX get Resolution func\n");
	
	pSensorResolution->SensorFullWidth=S5K5EAYX_IMAGE_SENSOR_FULL_WIDTH;  
	pSensorResolution->SensorFullHeight=S5K5EAYX_IMAGE_SENSOR_FULL_HEIGHT;
	
	pSensorResolution->SensorPreviewWidth=S5K5EAYX_IMAGE_SENSOR_PV_WIDTH;
	pSensorResolution->SensorPreviewHeight=S5K5EAYX_IMAGE_SENSOR_PV_HEIGHT;
	
	pSensorResolution->SensorVideoWidth=S5K5EAYX_IMAGE_SENSOR_VIDEO_WIDTH;
	pSensorResolution->SensorVideoHeight=S5K5EAYX_IMAGE_SENSOR_VIDEO_HEIGHT;
	
	return ERROR_NONE;
}	

UINT32 S5K5EAYXGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
					  MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    	SENSORDB("[5EA]: S5K5EAYXGetInfo func\n");
   
        pSensorInfo->SensorCameraPreviewFrameRate=30;
        pSensorInfo->SensorVideoFrameRate=30;
        pSensorInfo->SensorStillCaptureFrameRate=15;
        pSensorInfo->SensorWebCamCaptureFrameRate=15;
        pSensorInfo->SensorResetActiveHigh=FALSE;
        pSensorInfo->SensorResetDelayCount=4;
        
		pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_YUYV;

        pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;	
        pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
        pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
        pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_HIGH;
        pSensorInfo->SensorInterruptDelayLines = 1; 
		
		pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_MIPI;

		pSensorInfo->CaptureDelayFrame = 2; 
		pSensorInfo->PreviewDelayFrame = 2; 
        pSensorInfo->VideoDelayFrame = 2; 
        pSensorInfo->SensorMasterClockSwitch = 0; 
        pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_8MA;   

		pSensorInfo->YUVAwbDelayFrame = 2; 
		pSensorInfo->YUVEffectDelayFrame = 2;   
	
	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			pSensorInfo->SensorClockFreq=24;
			pSensorInfo->SensorClockDividCount=	5;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 3;
			pSensorInfo->SensorDataLatchCount= 2;
			
			pSensorInfo->SensorGrabStartX = S5K5EAYX_PV_X_START; 
			pSensorInfo->SensorGrabStartY = S5K5EAYX_PV_Y_START;  
			
			pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;			
			pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
			pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
			pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
			pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
			pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
			pSensorInfo->SensorPacketECCOrder = 1;
			
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockDividCount=	5;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = S5K5EAYX_FULL_X_START; 
            pSensorInfo->SensorGrabStartY = S5K5EAYX_FULL_Y_START;			
		    
			pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;			
			pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
			pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
			pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
			pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
			pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
			pSensorInfo->SensorPacketECCOrder = 1;
			
		break;
		default:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockDividCount=	5;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount=3;
            pSensorInfo->SensorDataLatchCount=2;
            pSensorInfo->SensorGrabStartX = S5K5EAYX_PV_X_START; 
            pSensorInfo->SensorGrabStartY = S5K5EAYX_PV_Y_START;  			
		break;
	}
	
	return ERROR_NONE;
}	


/*************************************************************************
* FUNCTION
*	S5K5EAYX_set_param_effect
*
* DESCRIPTION
*	effect setting.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
BOOL S5K5EAYX_set_param_effect(UINT16 para)
{

   SENSORDB("[5EA]:S5K5EAYX set_param_effect func:para = %d,MEFFECT_OFF =%d\n",para,MEFFECT_OFF);
   switch (para)
	{
		case MEFFECT_OFF:
		{
			S5K5EAYX_write_cmos_sensor(0xFCFC,0xD000);					 
			S5K5EAYX_write_cmos_sensor(0x0028,0x2000);					 
			S5K5EAYX_write_cmos_sensor(0x002A,0x0276);					 
			S5K5EAYX_write_cmos_sensor(0x0F12,0x0000);	//REG_TC_GP_SpecialEffects 	
        }
	        break;
		case MEFFECT_NEGATIVE:
			S5K5EAYX_write_cmos_sensor(0xFCFC,0xD000);					 
			S5K5EAYX_write_cmos_sensor(0x0028,0x2000);					 
			S5K5EAYX_write_cmos_sensor(0x002A,0x0276);					 
			S5K5EAYX_write_cmos_sensor(0x0F12,0x0002);	//REG_TC_GP_SpecialEffects 	
			break;
		case MEFFECT_SEPIA:
		{
			S5K5EAYX_write_cmos_sensor(0xFCFC,0xD000);					 
			S5K5EAYX_write_cmos_sensor(0x0028,0x2000);					 
			S5K5EAYX_write_cmos_sensor(0x002A,0x0276);					 
			S5K5EAYX_write_cmos_sensor(0x0F12,0x0004);	//REG_TC_GP_SpecialEffects 	
        }	
			break;  
		case MEFFECT_SEPIABLUE:
		{
			S5K5EAYX_write_cmos_sensor(0xFCFC,0xD000);					 
			S5K5EAYX_write_cmos_sensor(0x0028,0x2000);					 
			S5K5EAYX_write_cmos_sensor(0x002A,0x0276);					 
			S5K5EAYX_write_cmos_sensor(0x0F12,0x0007);	//REG_TC_GP_SpecialEffects 	
	    }     
			break;        
		case MEFFECT_SEPIAGREEN:		
		{
			S5K5EAYX_write_cmos_sensor(0xFCFC,0xD000);					 
			S5K5EAYX_write_cmos_sensor(0x0028,0x2000);					 
			S5K5EAYX_write_cmos_sensor(0x002A,0x0276);					 
			S5K5EAYX_write_cmos_sensor(0x0F12,0x0008);	//REG_TC_GP_SpecialEffects 	
	    }     
			break;        
		case MEFFECT_MONO:			
		{
			S5K5EAYX_write_cmos_sensor(0xFCFC,0xD000);					 
			S5K5EAYX_write_cmos_sensor(0x0028,0x2000);					 
			S5K5EAYX_write_cmos_sensor(0x002A,0x0276);					 
			S5K5EAYX_write_cmos_sensor(0x0F12,0x0001);	//REG_TC_GP_SpecialEffects 	
        }
			break;

		default:
			return KAL_FALSE;
	}

	return KAL_TRUE;

} /* S5K5EAYX_set_param_effect */

UINT32 S5K5EAYXControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{

   spin_lock(&s5k5eayx_drv_lock);
   S5K5EAYXCurrentScenarioId = ScenarioId;
   spin_unlock(&s5k5eayx_drv_lock);
   
   SENSORDB("[5EA]: S5K5EAYXControl:ScenarioId = %d \n",S5K5EAYXCurrentScenarioId);
   
	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:// 0
			 S5K5EAYXPreview(pImageWindow, pSensorConfigData);
			 break;

		case MSDK_SCENARIO_ID_VIDEO_PREVIEW: // 2
			 S5K5EAYXVideo(pImageWindow, pSensorConfigData);
			 break;
			 
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:// 1
			 S5K5EAYXCapture(pImageWindow, pSensorConfigData);
			 break;			
		case MSDK_SCENARIO_ID_CAMERA_ZSD:// 4
			 S5K5EAYXZsd(pImageWindow, pSensorConfigData);
			 break;
		default:
		     break; 
	}

	return ERROR_NONE;
}	/* S5K5EAYXControl() */


/*************************************************************************
* FUNCTION
*	S5K5EAYX_set_param_wb
*
* DESCRIPTION
*	wb setting.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
BOOL S5K5EAYX_set_param_wb(UINT16 para)
{
	
    //This sensor need more time to balance AWB, 
    //we suggest higher fps or drop some frame to avoid garbage color when preview initial
	kal_uint16 Status_3A=0;

	SENSORDB("[5EA]:S5K5EAYX set_param_wb func:para = %d,AWB_MODE_AUTO =%d\n",para,AWB_MODE_AUTO);
	while(Status_3A==0)
	{
		S5K5EAYX_write_cmos_sensor(0xFCFC,0xd000);
		S5K5EAYX_write_cmos_sensor(0x002C,0x2000);
		S5K5EAYX_write_cmos_sensor(0x002E,0x051C);
		Status_3A=S5K5EAYX_read_cmos_sensor(0x0F12); //Index number of active capture configuration //Normal capture// 
		mdelay(10);
	}

	switch (para)
	{            
		case AWB_MODE_AUTO:
			{
			Status_3A = (Status_3A | 0x8); // Enable AWB
			S5K5EAYX_write_cmos_sensor(0xFCFC, 0xD000);
			S5K5EAYX_write_cmos_sensor(0x0028, 0x2000);
			S5K5EAYX_write_cmos_sensor(0x002a, 0x051C);
			S5K5EAYX_write_cmos_sensor(0x0F12, Status_3A);
            }   
		    break;
		case AWB_MODE_OFF:
	         {
			 Status_3A = (Status_3A & 0xFFF7); // Disable AWB
			 S5K5EAYX_write_cmos_sensor(0xFCFC, 0xD000);
			 S5K5EAYX_write_cmos_sensor(0x0028, 0x2000);
			 S5K5EAYX_write_cmos_sensor(0x002A, 0x051C);
			 S5K5EAYX_write_cmos_sensor(0x0F12, Status_3A);
		     }
			 break;
		case AWB_MODE_CLOUDY_DAYLIGHT:
			{
			Status_3A = (Status_3A & 0xFFF7); // Disable AWB
			S5K5EAYX_write_cmos_sensor(0xFCFC, 0xD000);
			S5K5EAYX_write_cmos_sensor(0x0028, 0x2000);
			S5K5EAYX_write_cmos_sensor(0x002A, 0x051C);
			S5K5EAYX_write_cmos_sensor(0x0F12, Status_3A);
			//S5K5EAYX_write_cmos_sensor(0x0F12, 0x0777);
			S5K5EAYX_write_cmos_sensor(0x002A, 0x04F6); 
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0740); //Reg_sf_user_Rgain
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_RgainChanged
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x03D0); //0400Reg_sf_user_Ggain
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_GgainChanged
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x04D0); //0460Reg_sf_user_Bgain
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_BgainChanged
	        }			   
		    break;
		case AWB_MODE_DAYLIGHT:
		    {
			Status_3A = (Status_3A & 0xFFF7); // Disable AWB
			S5K5EAYX_write_cmos_sensor(0xFCFC, 0xD000);
			S5K5EAYX_write_cmos_sensor(0x0028, 0x2000);
			S5K5EAYX_write_cmos_sensor(0x002A, 0x051C);
			S5K5EAYX_write_cmos_sensor(0x0F12, Status_3A);
			//S5K5EAYX_write_cmos_sensor(0x0F12, 0x0777);
			S5K5EAYX_write_cmos_sensor(0x002A, 0x04F6); 
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x06c5); //05E0Reg_sf_user_Rgain
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_RgainChanged
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0400); //0400Reg_sf_user_Ggain
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_GgainChanged
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x04d3); //0530Reg_sf_user_Bgain
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_BgainChanged
            }      
		    break;
		case AWB_MODE_INCANDESCENT:	
		    {
			Status_3A = (Status_3A & 0xFFF7); // Disable AWB
			S5K5EAYX_write_cmos_sensor(0xFCFC, 0xD000);
			S5K5EAYX_write_cmos_sensor(0x0028, 0x2000);
			S5K5EAYX_write_cmos_sensor(0x002A, 0x051C);
			S5K5EAYX_write_cmos_sensor(0x0F12, Status_3A);
			//S5K5EAYX_write_cmos_sensor(0x0F12, 0x0777);
			S5K5EAYX_write_cmos_sensor(0x002A, 0x04F6); 
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0401); //0575Reg_sf_user_Rgain
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_RgainChanged
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0400); //0400Reg_sf_user_Ggain
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_GgainChanged
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0957); //0800Reg_sf_user_Bgain
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_BgainChanged
            }		
		    break;  
		case AWB_MODE_FLUORESCENT:
		    {
			Status_3A = (Status_3A & 0xFFF7); // Disable AWB
			S5K5EAYX_write_cmos_sensor(0xFCFC, 0xD000);
			S5K5EAYX_write_cmos_sensor(0x0028, 0x2000);
			S5K5EAYX_write_cmos_sensor(0x002A, 0x051C);
			S5K5EAYX_write_cmos_sensor(0x0F12, Status_3A);
			//S5K5EAYX_write_cmos_sensor(0x0F12, 0x0777);
			S5K5EAYX_write_cmos_sensor(0x002A, 0x04F6); 
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x05a1); //0400Reg_sf_user_Rgain
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_RgainChanged
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0400); //0400Reg_sf_user_Ggain
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_GgainChanged
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x08e7); //Reg_sf_user_Bgain
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_BgainChanged
            }	
		    break;  
		case AWB_MODE_TUNGSTEN:
		    {
			Status_3A = (Status_3A & 0xFFF7); // Disable AWB
			S5K5EAYX_write_cmos_sensor(0xFCFC, 0xD000);
			S5K5EAYX_write_cmos_sensor(0x0028, 0x2000);
			S5K5EAYX_write_cmos_sensor(0x002A, 0x051C);
			S5K5EAYX_write_cmos_sensor(0x0F12, Status_3A);
			//S5K5EAYX_write_cmos_sensor(0x0F12, 0x0777);
			S5K5EAYX_write_cmos_sensor(0x002A, 0x04F6); 
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0200); //0400Reg_sf_user_Rgain
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_RgainChanged
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0200); //0400Reg_sf_user_Ggain
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_GgainChanged
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x04A0); //Reg_sf_user_Bgain
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_BgainChanged
            }	
		    break;  			    
		default:
			return FALSE;
	}
        //SENSORDB("Status_3A = 0x%x\n",Status_3A);
	return TRUE;
} /* S5K5EAYX_set_param_wb */


/*************************************************************************
* FUNCTION
*	S5K5EAYX_set_param_banding
*
* DESCRIPTION
*	banding setting.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
BOOL S5K5EAYX_set_param_banding(UINT16 para)
{
	SENSORDB("[5EA]:S5K5EAYX set_param_banding func:para = %d,AE_FLICKER_MODE_50HZ=%d\n",para,AE_FLICKER_MODE_50HZ);
	kal_uint16 Status_3A=0;
	while(Status_3A==0)
	{
		S5K5EAYX_write_cmos_sensor(0xFCFC,0xd000);
		S5K5EAYX_write_cmos_sensor(0x002C,0x2000);
		S5K5EAYX_write_cmos_sensor(0x002E,0x051C);
		Status_3A=S5K5EAYX_read_cmos_sensor(0x0F12); //Index number of active capture configuration //Normal capture// 
		mdelay(10);
	}
	switch (para)
	{
		case AE_FLICKER_MODE_60HZ:
	   		 {
			Status_3A = (Status_3A & 0xFFDF); // disable auto-flicker
			S5K5EAYX_write_cmos_sensor(0x0028, 0x2000);   
			S5K5EAYX_write_cmos_sensor(0x002a, 0x051C);   
			S5K5EAYX_write_cmos_sensor(0x0f12, Status_3A);   
			S5K5EAYX_write_cmos_sensor(0x002a, 0x0512);   
			S5K5EAYX_write_cmos_sensor(0x0f12, 0x0002);   
			S5K5EAYX_write_cmos_sensor(0x0f12, 0x0001); 
	    	}
			break;

		case AE_FLICKER_MODE_50HZ:
		default:
				{
				Status_3A = (Status_3A & 0xFFDF); // disable auto-flicker
				S5K5EAYX_write_cmos_sensor(0x0028, 0x2000);   
				S5K5EAYX_write_cmos_sensor(0x002a, 0x051C);   
				S5K5EAYX_write_cmos_sensor(0x0f12, Status_3A);	 
				//S5K5EAYX_write_cmos_sensor(0x0f12, 0x075f);	
				S5K5EAYX_write_cmos_sensor(0x002a, 0x0512);   
				S5K5EAYX_write_cmos_sensor(0x0f12, 0x0001);   
				S5K5EAYX_write_cmos_sensor(0x0f12, 0x0001);
				}
				break;
	}
	return KAL_TRUE;
} /* S5K5EAYX_set_param_banding */


/*************************************************************************
* FUNCTION
*	S5K5EAYX_set_param_exposure
*
* DESCRIPTION
*	exposure setting.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
BOOL S5K5EAYX_set_param_exposure(UINT16 para)
{
	SENSORDB("[5EA]:S5K5EAYX set_param_exposure func:para = %d\n",para);

    if(S5K5EAYX_CheckHDRstatus())
    {
		S5K5EAYXHdr_Part2(para);
        return true;
	}

	
	switch (para)
	{
       case AE_EV_COMP_n20://-2
		   S5K5EAYX_write_cmos_sensor(0xFCFC,0xD000);					
		   S5K5EAYX_write_cmos_sensor(0x0028,0x2000);					
		   S5K5EAYX_write_cmos_sensor(0x002A,0x0274);					
		   S5K5EAYX_write_cmos_sensor(0x0F12,0x0040);  	
           break;
	   case AE_EV_COMP_n10://-1
		   S5K5EAYX_write_cmos_sensor(0xFCFC,0xD000);					
		   S5K5EAYX_write_cmos_sensor(0x0028,0x2000);					
		   S5K5EAYX_write_cmos_sensor(0x002A,0x0274);					
		   S5K5EAYX_write_cmos_sensor(0x0F12,0x0080);  	
		   break;	 
	   case AE_EV_COMP_00:	// +0 EV
		   S5K5EAYX_write_cmos_sensor(0xFCFC,0xD000);					
		   S5K5EAYX_write_cmos_sensor(0x0028,0x2000);					
		   S5K5EAYX_write_cmos_sensor(0x002A,0x0274);					
		   S5K5EAYX_write_cmos_sensor(0x0F12,0x0130);  	
		   break;	 
	   case AE_EV_COMP_10://+1
		  S5K5EAYX_write_cmos_sensor(0xFCFC,0xD000);					
		  S5K5EAYX_write_cmos_sensor(0x0028,0x2000);					
		  S5K5EAYX_write_cmos_sensor(0x002A,0x0274);					
		  S5K5EAYX_write_cmos_sensor(0x0F12,0x0170);  	
		   break;
	   case AE_EV_COMP_20://+2
		   S5K5EAYX_write_cmos_sensor(0xFCFC,0xD000);					
		   S5K5EAYX_write_cmos_sensor(0x0028,0x2000);					
		   S5K5EAYX_write_cmos_sensor(0x002A,0x0274);					
		   S5K5EAYX_write_cmos_sensor(0x0F12,0x0200);  	
		   break;	 
	   default:
		   return FALSE;

	}
	return TRUE;
	
} 


#if 1//scene mode
void S5K5EAYX_SceneMode_SetDefault(void)
{
	S5K5EAYX_write_cmos_sensor(0x0028,0x2000);//								
	S5K5EAYX_write_cmos_sensor(0x002A,0x051C);//								
	S5K5EAYX_write_cmos_sensor(0x0F12,0x077F);//  //REG_TC_DBG_AutoAlgEnBits	
																				
	S5K5EAYX_write_cmos_sensor(0x002A,0x02C8);//								
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0005);//  //REG_TC_AF_AfCmd 			
	S5K5EAYX_write_cmos_sensor(0x002A,0x050C);//								
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0002);//  //REG_SF_USER_IsoType 		
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0100);//  //REG_SF_USER_IsoVal			  
	S5K5EAYX_write_cmos_sensor(0x002A,0x026A);//								
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0000);//  //REG_TC_UserBrightness		
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0000);//  //REG_TC_UserContrast 		  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0000);//  //REG_TC_UserSaturation		
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0000);//  //REG_TC_UserSharpBlur	
	
	//S5K5EAYX_write_cmos_sensor(0x002A,0x0276);//								
	//S5K5EAYX_write_cmos_sensor(0x0F12,0x0000);//  //REG_TC_GP_SpecialEffects;don't open this ,MTK
	
	S5K5EAYX_write_cmos_sensor(0x002A,0x24BC);//								
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_0_		  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_1_		  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_2_		  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_3_		  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_4_		  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_5_		  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_6_		  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_7_		  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_8_		  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_9_		  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_10_ 	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_11_ 	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_12_ 	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_13_ 	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_14_ 	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_15_ 	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_16_ 	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_17_ 	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_18_ 	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_19_ 	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_20_ 	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_21_ 	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_22_ 	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_23_ 	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_24_ 	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_25_ 	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_26_ 	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_27_ 	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_28_ 	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_29_ 	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_30_ 	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);//  //Mon_AE_WeightTbl_16_31_ 	  
}


void S5K5EAYX_SceneMode_PORTRAIT(void)
{
	S5K5EAYX_write_cmos_sensor(0x0028,0x2000);// 
	S5K5EAYX_write_cmos_sensor(0x002A,0x051C);// 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x077F);// //REG_TC_DBG_AutoAlgEnBits
	S5K5EAYX_write_cmos_sensor(0x002A,0x02C8);// 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0005);// //REG_TC_AF_AfCmd	  
	S5K5EAYX_write_cmos_sensor(0x002A,0x050C);// 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0002);// //REG_SF_USER_IsoType	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0100);// //REG_SF_USER_IsoVal		  
	S5K5EAYX_write_cmos_sensor(0x002A,0x026A);// 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0000);// //REG_TC_UserBrightness
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0000);// //REG_TC_UserContrast
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0000);// //REG_TC_UserSaturation
	S5K5EAYX_write_cmos_sensor(0x0F12,0xFF41);// //REG_TC_UserSharpBlur   
	
	//S5K5EAYX_write_cmos_sensor(0x002A,0x0276);//
	//S5K5EAYX_write_cmos_sensor(0x0F12,0x0000);// ////REG_TC_UserSharpBlur;
	
	S5K5EAYX_write_cmos_sensor(0x002A,0x24BC);// 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_0_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_1_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_2_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_3_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_4_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_5_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_6_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_7_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_8_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_9_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_10_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_11_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_12_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0f01);// //Mon_AE_WeightTbl_16_13_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x010f);// //Mon_AE_WeightTbl_16_14_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_15_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_16_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0f01);// //Mon_AE_WeightTbl_16_17_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x010f);// //Mon_AE_WeightTbl_16_18_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_19_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_20_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_21_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_22_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_23_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_24_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_25_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_26_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_27_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_28_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_29_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_30_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_31_	
}


void S5K5EAYX_SceneMode_LANDSCAPE(void)
{
	S5K5EAYX_write_cmos_sensor(0x0028,0x2000);//
	S5K5EAYX_write_cmos_sensor(0x002A,0x051C);//
	S5K5EAYX_write_cmos_sensor(0x0F12,0x077F);// //REG_TC_DBG_AutoAlgEnBits   
	S5K5EAYX_write_cmos_sensor(0x002A,0x02C8);//  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0005);// // REG_TC_AF_AfCmd   
	S5K5EAYX_write_cmos_sensor(0x002A,0x050C);// 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0002);// //REG_SF_USER_IsoType	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0100);// //REG_SF_USER_IsoVal 
	S5K5EAYX_write_cmos_sensor(0x002A,0x026A);// 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0000);// //REG_TC_UserBrightness  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0040);// //REG_TC_UserContrast		
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0040);// //REG_TC_UserSaturation  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0040);// //REG_TC_UserSharpBlur   
	
	//S5K5EAYX_write_cmos_sensor(0x002A,0x0276);// 
	//S5K5EAYX_write_cmos_sensor(0x0F12,0x0000);// //REG_TC_GP_SpecialEffects   
	
	S5K5EAYX_write_cmos_sensor(0x002A,0x24BC);// 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_0_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_1_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_2_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_3_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_4_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_5_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_6_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_7_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_8_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_9_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_10_   
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_11_   
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_12_   
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_13_   
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_14_   
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_15_   
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_16_   
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_17_   
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_18_   
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_19_   
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_20_   
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_21_   
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_22_   
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_23_   
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_24_   
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_25_   
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_26_   
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_27_   
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_28_   
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_29_   
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_30_   
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_31_   
	
}

void S5K5EAYX_SceneMode_SUNSET(void)
{
	S5K5EAYX_write_cmos_sensor(0x0028,0x2000);// 
	S5K5EAYX_write_cmos_sensor(0x002A,0x051C);// 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0777);// //REG_TC_DBG_AutoAlgEnBits
	
	S5K5EAYX_write_cmos_sensor(0x002A,0x02C8);// 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0005);// //REG_TC_AF_AfCmd

	S5K5EAYX_write_cmos_sensor(0x002A,0x04F6);
	S5K5EAYX_write_cmos_sensor(0x0F12,0x05E0);//REG_SF_USER_Rgain
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0001);//REG_SF_USER_RgainChanged
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0400);//REG_SF_USER_Ggain
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0001);//REG_SF_USER_GgainChanged
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0530); //REG_SF_USER_Bgain
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0001);//REG_SF_USER_BgainChanged 
	
	S5K5EAYX_write_cmos_sensor(0x002A,0x050C);// 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0002);// //REG_SF_USER_IsoType
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0100);// //REG_SF_USER_IsoVal
	S5K5EAYX_write_cmos_sensor(0x002A,0x026A);// 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0000);// //REG_TC_UserBrightness
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0000);// //REG_TC_UserContrast
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0000);// //REG_TC_UserSaturation
	S5K5EAYX_write_cmos_sensor(0x0F12,0xFF41);// //REG_TC_UserSharpBlur
	
	//S5K5EAYX_write_cmos_sensor(0x002A,0x0276);//
	//S5K5EAYX_write_cmos_sensor(0x0F12,0x0000);// //REG_TC_GP_SpecialEffects
	
	S5K5EAYX_write_cmos_sensor(0x002A,0x24BC);// 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_0_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_1_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_2_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_3_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_4_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_5_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_6_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_7_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_8_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_9_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_10_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_11_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_12_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0f01);// //Mon_AE_WeightTbl_16_13_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x010f);// //Mon_AE_WeightTbl_16_14_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_15_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_16_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0f01);// //Mon_AE_WeightTbl_16_17_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x010f);// //Mon_AE_WeightTbl_16_18_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_19_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_20_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_21_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_22_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_23_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_24_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_25_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_26_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_27_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_28_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_29_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_30_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// //Mon_AE_WeightTbl_16_31_	 
}

void S5K5EAYX_SceneMode_SPORTS(void)
{
	S5K5EAYX_write_cmos_sensor(0x0028,0x2000);//
	S5K5EAYX_write_cmos_sensor(0x002A,0x051C);//
	S5K5EAYX_write_cmos_sensor(0x0F12,0x077F);//  //REG_TC_DBG_AutoAlgEnBits
	S5K5EAYX_write_cmos_sensor(0x02CA,0x02CA);//  //
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0000);//  //
	S5K5EAYX_write_cmos_sensor(0x002A,0x02C8);//  //
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0004);//  //REG_TC_AF_AfCmd
	S5K5EAYX_write_cmos_sensor(0x002A,0x050C);//  //
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0002);//  //REG_SF_USER_IsoType
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0400);//  //REG_SF_USER_IsoVal
	S5K5EAYX_write_cmos_sensor(0x002A,0x026A);//  //
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0000);//  //REG_TC_UserBrightness
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0000);//  //REG_TC_UserContrast
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0000);//  //REG_TC_UserSaturation
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0000);//  //REG_TC_UserSharpBlur
	
	//S5K5EAYX_write_cmos_sensor(0x002A,0x0276);//  //
	//S5K5EAYX_write_cmos_sensor(0x0F12,0x0000);//  //REG_TC_GP_SpecialEffects
	
	S5K5EAYX_write_cmos_sensor(0x002A,0x24BC);//  //
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_0_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_1_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_2_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_3_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_4_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_5_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_6_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_7_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_8_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_9_	  
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_10_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_11_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_12_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0f01);// // Mon_AE_WeightTbl_16_13_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x010f);// // Mon_AE_WeightTbl_16_14_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_15_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_16_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0f01);// // Mon_AE_WeightTbl_16_17_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x010f);// // Mon_AE_WeightTbl_16_18_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_19_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_20_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_21_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_22_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_23_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_24_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_25_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_26_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_27_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_28_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_29_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_30_ 
	S5K5EAYX_write_cmos_sensor(0x0F12,0x0101);// // Mon_AE_WeightTbl_16_31_ 
}
#endif

void S5K5EAYX_set_scene_mode(UINT16 para)
{
	SENSORDB("[5EA]:S5K5EAYX S5K5EAYX_set_scene_mode func:para = %d\n",para);
	//SENSORDB(" [5EA]:S5K5EAYX S5K5EAYX_set_scene_mode func:SCENE_MODE_NIGHTSCENE = %d\n",SCENE_MODE_NIGHTSCENE);
	//SENSORDB("[5EA]:S5K5EAYX S5K5EAYX_set_scene_mode func:SCENE_MODE_OFF = %d\n",SCENE_MODE_OFF);
	//SENSORDB(" [5EA]:S5K5EAYX S5K5EAYX_set_scene_mode func:SCENE_MODE_HDR \n",SCENE_MODE_HDR);

	S5K5EAYX_CurrentStatus.iSceneMode = para;

	S5K5EAYX_SceneMode_SetDefault();
	S5K5EAYX_CaptureCfg0_FixFps_Restore();
	
    switch (para)
    { 

	case SCENE_MODE_NIGHTSCENE:
             		S5K5EAYX_Mode_Config(S5K5EZYX_NightMode_On); 
			 break;
        case SCENE_MODE_PORTRAIT:
			 S5K5EAYX_SceneMode_PORTRAIT();
             		S5K5EAYX_Mode_Config(S5K5EZYX_NightMode_Off); 
             		break;
        case SCENE_MODE_LANDSCAPE:
			 S5K5EAYX_SceneMode_LANDSCAPE();
            		 S5K5EAYX_Mode_Config(S5K5EZYX_NightMode_Off); 
            		 break;
        case SCENE_MODE_SUNSET:
			 S5K5EAYX_SceneMode_SUNSET();
            		 S5K5EAYX_Mode_Config(S5K5EZYX_NightMode_Off); 
            		 break;
        case SCENE_MODE_SPORTS://control fps
        	 S5K5EAYX_SceneMode_SPORTS();
			 
			 if((s5k5eayx_sensor_mode == S5K5EAYX_SENSORMODE_PREVIEW)
			 	||(s5k5eayx_sensor_mode == S5K5EAYX_SENSORMODE_VIDEO))
			 {
			     //active video normal mode ,30fps
				 S5K5EAYX_write_cmos_sensor(0x002A,0x02A0);// //
				 S5K5EAYX_write_cmos_sensor(0x0F12,0x0002);//  //REG_TC_GP_ActivePrevConfig    
				 S5K5EAYX_write_cmos_sensor(0x0F12,0x0001);//  //REG_TC_GP_PrevConfigChanged   
			 }
			 else
			 {
				S5K5EAYX_CaptureCfg0_FixFps(7.5);//15

				//activeCaptureConfig 0
				 S5K5EAYX_write_cmos_sensor(0xFCFC, 0xd000);
				 S5K5EAYX_write_cmos_sensor(0x0028, 0x2000);
				 
				 S5K5EAYX_write_cmos_sensor(0x002a, 0x02A8);
				 S5K5EAYX_write_cmos_sensor(0x0f12, 0x0000+0); 
				 
				 S5K5EAYX_write_cmos_sensor(0x002A, 0x02AA);
				 S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
				 
				 S5K5EAYX_write_cmos_sensor(0x002A, 0x0288);
				 S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
				 
				 S5K5EAYX_write_cmos_sensor(0x002A, 0x027C);
				 S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
				 S5K5EAYX_write_cmos_sensor(0x002A, 0x027E);
				 S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001); 
			 }
			 
             break;
        case SCENE_MODE_HDR:
			if(S5K5EAYX_CurrentStatus.HDRFixed_AE_Done==KAL_TRUE)
			{
				S5K5EAYXHdr_Part3();
			}
			break;
        case SCENE_MODE_OFF://AUTO
        default:
			S5K5EAYX_Mode_Config(S5K5EZYX_NightMode_Off); 
			break;
    }

	return;
}


UINT32 S5K5EAYXYUVSetVideoMode(UINT16 u2FrameRate)
{
	
	SENSORDB("[5EA]:S5K5EAYX Set Video Mode:FrameRate= %d\n",u2FrameRate);
    //set fps in FUNC S5K5EAYX_Mode_Config(xxxx);
		
    return ERROR_NONE;
}

static void S5K5EAYX_Get_AF_Status(UINT32 *pFeatureReturnPara32)
{
	UINT32 state = 0;

	S5K5EAYX_write_cmos_sensor(0xFCFC,0xd000);
	S5K5EAYX_write_cmos_sensor(0x002C,0x2000);
	S5K5EAYX_write_cmos_sensor(0x002E,0x1F92);
	state = S5K5EAYX_read_cmos_sensor(0x0F12);

    if(0x00 == state)
    {
        *pFeatureReturnPara32 = SENSOR_AF_IDLE;
	
    }
    else if(0x01 == state)//focusing
    {
        *pFeatureReturnPara32 = SENSOR_AF_FOCUSING;
		
    }
    else if(0x02 == state)//success
    {
        *pFeatureReturnPara32 = SENSOR_AF_FOCUSED;
			
    }
    else
    {
        *pFeatureReturnPara32 = SENSOR_AF_ERROR;
			   
    }
   //SENSORDB("[5EA]:S5K5EAYX_Get_AF_Status = 0x%x\n", state); 

}	

static void S5K5EAYX_Get_AF_Inf(UINT32 *pFeatureReturnPara32)
{
    *pFeatureReturnPara32 = 0;
}


static void S5K5EAYX_Get_AF_Macro(UINT32 *pFeatureReturnPara32)
{
    *pFeatureReturnPara32 = 255;
}

static void S5K5EAYX_Single_Focus(void)
{
   
    S5K5EAYX_write_cmos_sensor(0x0028,0x2000);
    S5K5EAYX_write_cmos_sensor(0x002a,0x02C8);
    S5K5EAYX_write_cmos_sensor(0x0f12,0x0005); // single AF 

    SENSORDB("[5EA]:S5K5EAYX_Single_Focus\r\n");
    return;
}	
static void S5K5EAYX_Constant_Focus(void)
{

	S5K5EAYX_write_cmos_sensor(0x0028,0x2000);
    S5K5EAYX_write_cmos_sensor(0x002a,0x02C8);
    S5K5EAYX_write_cmos_sensor(0x0f12,0x0006);
    SENSORDB("[5EA]:S5K5EAYX_Constant_Focus \r\n");
    return;
}

 static void S5K5EAYX_Cancel_Focus(void)
 {
	S5K5EAYX_write_cmos_sensor(0x0FCFC,0xD000);	
	S5K5EAYX_write_cmos_sensor(0x0028,0x2000);

	S5K5EAYX_write_cmos_sensor(0x002a,0x02C8);
	S5K5EAYX_write_cmos_sensor(0x0f12,0x0001);
	
 	
    SENSORDB("[5EA]:S5K5EAYX_Cancel_Focus \r\n");
 	return;
 }

void S5K5EAYX_set_saturation(UINT16 para)
{
	SENSORDB("[5EA]:S5K5EAYX S5K5EAYX_set_saturation func:para = %d\n",para);

    switch (para)
    {
        case ISP_SAT_HIGH:
			S5K5EAYX_write_cmos_sensor(0x0028, 0x2000);
			S5K5EAYX_write_cmos_sensor(0x002a, 0x026E);
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0070);
             break;
			 
        case ISP_SAT_LOW:
			S5K5EAYX_write_cmos_sensor(0x0028, 0x2000);
			S5K5EAYX_write_cmos_sensor(0x002a, 0x026E);
			S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF81);
             break;
			 
        case ISP_SAT_MIDDLE:
        default:
			S5K5EAYX_write_cmos_sensor(0x0028, 0x2000);
			S5K5EAYX_write_cmos_sensor(0x002a, 0x026E);
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);
             break;
    }
     return;
}


void S5K5EAYX_set_contrast(UINT16 para)
{
	SENSORDB("[5EA]:S5K5EAYX S5K5EAYX_set_contrast func:para = %d\n",para);

	switch (para)
	{
		case ISP_CONTRAST_LOW:
			S5K5EAYX_write_cmos_sensor(0x0028, 0x2000);
			S5K5EAYX_write_cmos_sensor(0x002a, 0x026C);
			S5K5EAYX_write_cmos_sensor(0x0F12, 0xFF81);
			 break;
			 
		case ISP_CONTRAST_HIGH:
			S5K5EAYX_write_cmos_sensor(0x0028, 0x2000);
			S5K5EAYX_write_cmos_sensor(0x002a, 0x026C);
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x007F);
			 break;
			 
		case ISP_CONTRAST_MIDDLE:
		default:
			S5K5EAYX_write_cmos_sensor(0x0028, 0x2000);
			S5K5EAYX_write_cmos_sensor(0x002a, 0x026C);
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);
			 break;
	}
	
	return;
}


void S5K5EAYX_set_brightness(UINT16 para)
{
	SENSORDB("[5EA]:S5K5EAYX S5K5EAYX_set_brightness func:para = %d\n",para);
    switch (para)
    {
        case ISP_BRIGHT_LOW:
			S5K5EAYX_write_cmos_sensor(0x0028, 0x2000);
			S5K5EAYX_write_cmos_sensor(0x002a, 0x026A);
			S5K5EAYX_write_cmos_sensor(0x0F12, 0xFFC0);
             break;
        case ISP_BRIGHT_HIGH:
			S5K5EAYX_write_cmos_sensor(0x0028, 0x2000);
			S5K5EAYX_write_cmos_sensor(0x002a, 0x026A);
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0040);
             break;
        case ISP_BRIGHT_MIDDLE:
        default:
			S5K5EAYX_write_cmos_sensor(0x0028, 0x2000);
			S5K5EAYX_write_cmos_sensor(0x002a, 0x026A);
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);
             break;
    }

    return;
}

void S5K5EAYX_set_iso(UINT16 para)
{
	SENSORDB("[5EA]:S5K5EAYX S5K5EAYX_set_iso func:para = %d\n",para);
    switch (para)
    {
        case AE_ISO_100:
			S5K5EAYX_write_cmos_sensor(0x0028, 0x2000);
			S5K5EAYX_write_cmos_sensor(0x002a, 0x050C);
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0100);
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
			S5K5EAYX_write_cmos_sensor(0x002a, 0x098E);
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0200);
             break;
        case AE_ISO_200:
			S5K5EAYX_write_cmos_sensor(0x0028, 0x2000);
			S5K5EAYX_write_cmos_sensor(0x002a, 0x050C);
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0340);
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
			S5K5EAYX_write_cmos_sensor(0x002a, 0x098E);
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0200);
             break;
        case AE_ISO_400:
			S5K5EAYX_write_cmos_sensor(0x0028, 0x2000);
			S5K5EAYX_write_cmos_sensor(0x002a, 0x050C);
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0710);
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
			S5K5EAYX_write_cmos_sensor(0x002a, 0x098E);
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0200);
             break;
        default:
        case AE_ISO_AUTO:
			S5K5EAYX_write_cmos_sensor(0x0028, 0x2000);
			S5K5EAYX_write_cmos_sensor(0x002a, 0x050C);
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
			S5K5EAYX_write_cmos_sensor(0x002a, 0x098E);
			S5K5EAYX_write_cmos_sensor(0x0F12, 0x0200);
             break;
    }
    return;
}


void S5K5EAYX_AE_Lock_Enable(UINT16 iPara)
{
    if(iPara == AE_MODE_OFF)//turn off AE
    {
		S5K5EAYX_write_cmos_sensor(0x0028, 0x2000);
		S5K5EAYX_write_cmos_sensor(0x002a, 0x051C);
		S5K5EAYX_write_cmos_sensor(0x0F12, 0x0779);
	}
	else//turn on AE
	{
		S5K5EAYX_write_cmos_sensor(0x0028, 0x2000);
		S5K5EAYX_write_cmos_sensor(0x002a, 0x051C);
		S5K5EAYX_write_cmos_sensor(0x0F12, 0x077F);
	}
}


void S5K5EAYX_set_hue(UINT16 para)
{
	SENSORDB("[5EA]:S5K5EAYX S5K5EAYX_set_hue func:para = %d,ISP_HUE_MIDDLE=%d\n",para,ISP_HUE_MIDDLE);
	switch (para)
	{
		case ISP_HUE_LOW:
			S5K5EAYX_write_cmos_sensor(0x0028,0x2000);
			S5K5EAYX_write_cmos_sensor(0x002A,0x21DC);  //Hue1
			S5K5EAYX_write_cmos_sensor(0x0F12,0x0000);

			S5K5EAYX_write_cmos_sensor(0x002A,0x2206); 
			S5K5EAYX_write_cmos_sensor(0x0F12,0x01BB); 
			S5K5EAYX_write_cmos_sensor(0x0F12,0xFF35);
			S5K5EAYX_write_cmos_sensor(0x0F12,0x00BB);
			S5K5EAYX_write_cmos_sensor(0x0F12,0xFFB8);
			S5K5EAYX_write_cmos_sensor(0x0F12,0x0122);
			S5K5EAYX_write_cmos_sensor(0x0F12,0xFE7D);
			S5K5EAYX_write_cmos_sensor(0x0F12,0xFF70);
			S5K5EAYX_write_cmos_sensor(0x0F12,0x007C);
			S5K5EAYX_write_cmos_sensor(0x0F12,0x0188);
			S5K5EAYX_write_cmos_sensor(0x0F12,0x014E);
			S5K5EAYX_write_cmos_sensor(0x0F12,0x003B);
			S5K5EAYX_write_cmos_sensor(0x0F12,0xFF7A);
			S5K5EAYX_write_cmos_sensor(0x0F12,0x00A3);
			S5K5EAYX_write_cmos_sensor(0x0F12,0xFF7D);
			S5K5EAYX_write_cmos_sensor(0x0F12,0x01C8);
			S5K5EAYX_write_cmos_sensor(0x0F12,0xFF99);
			S5K5EAYX_write_cmos_sensor(0x0F12,0x021E);
			S5K5EAYX_write_cmos_sensor(0x0F12,0x0103);			 
			break;
		case ISP_HUE_HIGH:
			S5K5EAYX_write_cmos_sensor(0x0028,0x2000);
			S5K5EAYX_write_cmos_sensor(0x002A,0x21DC);	//Hue3
			S5K5EAYX_write_cmos_sensor(0x0F12,0x0000);
			
			S5K5EAYX_write_cmos_sensor(0x002A,0x2206); 
			S5K5EAYX_write_cmos_sensor(0x0F12,0x01E8); 
			S5K5EAYX_write_cmos_sensor(0x0F12,0x0061);
			S5K5EAYX_write_cmos_sensor(0x0F12,0xFF62);
			S5K5EAYX_write_cmos_sensor(0x0F12,0xFE6D);
			S5K5EAYX_write_cmos_sensor(0x0F12,0x0112);
			S5K5EAYX_write_cmos_sensor(0x0F12,0xFFD7);
			S5K5EAYX_write_cmos_sensor(0x0F12,0x007C);
			S5K5EAYX_write_cmos_sensor(0x0F12,0xFF70);
			S5K5EAYX_write_cmos_sensor(0x0F12,0x0188);
			S5K5EAYX_write_cmos_sensor(0x0F12,0x0072);
			S5K5EAYX_write_cmos_sensor(0x0F12,0x0133);
			S5K5EAYX_write_cmos_sensor(0x0F12,0xFF5F);
			S5K5EAYX_write_cmos_sensor(0x0F12,0x01C8);
			S5K5EAYX_write_cmos_sensor(0x0F12,0xFF7D);
			S5K5EAYX_write_cmos_sensor(0x0F12,0x00A2);
			S5K5EAYX_write_cmos_sensor(0x0F12,0xFFB3);
			S5K5EAYX_write_cmos_sensor(0x0F12,0x00CF);
			S5K5EAYX_write_cmos_sensor(0x0F12,0x0238);
			 break;
		case ISP_HUE_MIDDLE:
		default:
			S5K5EAYX_write_cmos_sensor(0x0028,0x2000);
			S5K5EAYX_write_cmos_sensor(0x002A,0x21DC); //REG_TC_UserSaturation
			S5K5EAYX_write_cmos_sensor(0x0F12,0x0001); //Hue 2
			 break;

	}
	return;
}


UINT32 S5K5EAYXYUVSensorSetting(FEATURE_ID iCmd, UINT32 iPara)
{
        
    SENSORDB("[5EA]:S5K5EAYXYUVSensorSetting func:cmd = %d\n",iCmd);

	switch (iCmd) {

	case FID_COLOR_EFFECT:
           	S5K5EAYX_set_param_effect(iPara);
	     break;

	case FID_AE_EV:	    	    
           	S5K5EAYX_set_param_exposure(iPara);
	     break;

	case FID_SCENE_MODE:
		S5K5EAYX_set_scene_mode(iPara);
	     break; 	    
		 
	case FID_AWB_MODE:
           	S5K5EAYX_set_param_wb(iPara);
	     break;
		 
	case FID_AE_FLICKER:	    	    	    
           	S5K5EAYX_set_param_banding(iPara);
	     break;

    	case FID_ISP_SAT:
        	S5K5EAYX_set_saturation(iPara);
		break;

	case FID_ISP_CONTRAST:
		S5K5EAYX_set_contrast(iPara);
		break;
		 
	 case FID_ISP_BRIGHT:
		 S5K5EAYX_set_brightness(iPara);
		 break;

	case FID_AE_ISO:
		S5K5EAYX_set_iso(iPara);
		break;

	case FID_AE_SCENE_MODE:
        	S5K5EAYX_AE_Lock_Enable(iPara);
		break;

	//case FID_ISP_HUE:
	//	S5K5EAYX_set_hue(iPara);// problem remove 
	//	 break;
		 
	case FID_ZOOM_FACTOR:
		spin_lock(&s5k5eayx_drv_lock);
		zoom_factor = iPara; 
		spin_unlock(&s5k5eayx_drv_lock);
		break; 

	default:
	     break;
	}
	return TRUE;
}

void S5K5EAYX_Get_AEAWB_lock_info(UINT32 *pAElockRet32,UINT32 *pAWBlockRet32)
{
    *pAElockRet32 = 1;
    *pAWBlockRet32 = 1;
    SENSORDB("GetAEAWBLock,AE=%d ,AWB=%d\n,",*pAElockRet32,*pAWBlockRet32);
}


void S5K5EAYX_GetDelayInfo(UINT32 delayAddr)
{
    SENSOR_DELAY_INFO_STRUCT* pDelayInfo = (SENSOR_DELAY_INFO_STRUCT*)delayAddr;
    pDelayInfo->InitDelay = 3;
    pDelayInfo->EffectDelay = 2;
    pDelayInfo->AwbDelay = 4;
    pDelayInfo->AFSwitchDelayFrame = 50;
}



void S5K5EA_GetExifInfo(UINT32 exifAddr)
{
    SENSOR_EXIF_INFO_STRUCT* pExifInfo = (SENSOR_EXIF_INFO_STRUCT*)exifAddr;
    pExifInfo->FNumber = 28;
    pExifInfo->AEISOSpeed = AE_ISO_100;
    pExifInfo->AWBMode = AWB_MODE_AUTO;
    pExifInfo->CapExposureTime = 0;
    pExifInfo->FlashLightTimeus = 0;
    pExifInfo->RealISOValue = AE_ISO_100;
}	

UINT32 S5K5EAYX_SetTestPatternMode(kal_bool bEnable)
{
    SENSORDB("[5EA]:[S5K5EAYX_SetTestPatternMode] Test pattern enable:%d\n", bEnable);

	if(bEnable) 
	{
		S5K5EAYX_write_cmos_sensor(0x0028, 0xD000);
		S5K5EAYX_write_cmos_sensor(0x002a, 0x3100);
		S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
	}
	else        
	{
		S5K5EAYX_write_cmos_sensor(0x0028, 0xD000);
		S5K5EAYX_write_cmos_sensor(0x002a, 0x3100);
		S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);
	}
    return ERROR_NONE;

}

UINT32 S5K5EAYX_SetMaxFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 frameRate)
{
    kal_uint32 pclk;
    kal_int16 dummyLine;
    kal_uint16 lineLength,frameHeight;
#if 0
    SENSORDB("S5K5EAYX_SetMaxFramerateByScenario: scenarioId = %d, frame rate = %d\n",scenarioId,frameRate);
    switch (scenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
             pclk = S5K5EAYX_PREVIEW_PCLK;
             lineLength = S5K5EAYX_PV_PERIOD_PIXEL_NUMS;
             frameHeight = (10 * pclk)/frameRate/lineLength;
             dummyLine = frameHeight - S5K5EAYX_PV_PERIOD_LINE_NUMS;
             //s5k5eayx_sensor_mode = S5K5EAYX_SENSORMODE_PREVIEW;
            //S5K5EAYX_SetDummy(0, dummyLine);
             break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
             pclk = S5K5EAYX_VIDEO_PCLK;
             lineLength = S5K5EAYX_VIDEO_PERIOD_PIXEL_NUMS;
             frameHeight = (10 * pclk)/frameRate/lineLength;
             dummyLine = frameHeight - S5K5EAYX_VIDEO_PERIOD_LINE_NUMS;
             //s5k5eayx_sensor_mode = S5K5EAYX_SENSORMODE_VIDEO;
            // S5K5EAYX_SetDummy(0, dummyLine);
             break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        case MSDK_SCENARIO_ID_CAMERA_ZSD:
             pclk = S5K5EAYX_CAPTURE_PCLK;
             lineLength = S5K5EAYX_CAP_PERIOD_PIXEL_NUMS;
             frameHeight = (10 * pclk)/frameRate/lineLength;
             dummyLine = frameHeight - S5K5EAYX_CAP_PERIOD_LINE_NUMS;
             //s5k5eayx_sensor_mode= S5K5EAYX_SENSORMODE_CAPTURE;
             //S5K5EAYX_SetDummy(0, dummyLine);
             break;
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
             break;
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
             break;
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added
             break;
        default:
             break;
    }
#endif
  return ERROR_NONE;
}


UINT32 S5K5EAYX_GetDefaultFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 *pframeRate)
{
    switch (scenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
             *pframeRate = 300;
             break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        case MSDK_SCENARIO_ID_CAMERA_ZSD:
             *pframeRate = 150;
             break;
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added
             *pframeRate = 300;
             break;
        default:
          break;
    }

  return ERROR_NONE;
}


void S5K5EAYX_AutoTestCmd(UINT32 *cmd,UINT32 *para)
{
	SENSORDB("[S5K5EAYX]enter S5K5EAYX_AutoTestCmd function:\n ");
	switch(*cmd)
	{
		case YUV_AUTOTEST_SET_SHADDING:
			SENSORDB("YUV_AUTOTEST_SET_SHADDING:para=%d\n",*para);
			break;
		case YUV_AUTOTEST_SET_GAMMA:
			SENSORDB("YUV_AUTOTEST_SET_GAMMA:para=%d\n",*para);
			break;
		case YUV_AUTOTEST_SET_AE:
			SENSORDB("YUV_AUTOTEST_SET_AE:para=%d\n",*para);
			break;
		case YUV_AUTOTEST_SET_SHUTTER:
			SENSORDB("YUV_AUTOTEST_SET_SHUTTER:para=%d\n",*para);
			break;
		case YUV_AUTOTEST_SET_GAIN:
			SENSORDB("YUV_AUTOTEST_SET_GAIN:para=%d\n",*para);
			break;
		case YUV_AUTOTEST_GET_SHUTTER_RANGE:
			*para=8228;
			break;
		default:
			SENSORDB("YUV AUTOTEST NOT SUPPORT CMD:%d\n",*cmd);
			break;	
	}
	SENSORDB("[S5K5EAYX]exit OV5645MIPI_AutoTestCmd function:\n ");
#if 0 //factory mode funtion back up
		//Gamma enable
		S5K5EAYX_write_cmos_sensor(0x0028, 0xD000);
		S5K5EAYX_write_cmos_sensor(0x002a, 0x6700);
		S5K5EAYX_write_cmos_sensor(0x0F12, 0x0000);
	
		//Gamma disable
		S5K5EAYX_write_cmos_sensor(0x0028, 0xD000);
		S5K5EAYX_write_cmos_sensor(0x002a, 0x6700);
		S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);
		//Shading enable
		S5K5EAYX_write_cmos_sensor(0x0028, 0xD000);
		S5K5EAYX_write_cmos_sensor(0x002a, 0x3400); 
		S5K5EAYX_write_cmos_sensor(0x0F12, 0x0006);
	
		//Shading disable
		S5K5EAYX_write_cmos_sensor(0x0028, 0xD000);
		S5K5EAYX_write_cmos_sensor(0x002a, 0x3400);
		S5K5EAYX_write_cmos_sensor(0x0F12, 0x0007);
#endif
	
}


void S5K5EAYX_3ACtrl(ACDK_SENSOR_3A_LOCK_ENUM action)
{
   switch (action)
   {
      case SENSOR_3A_AE_LOCK:
          S5K5EAYX_AE_Lock_Enable(AE_MODE_OFF);
      	  break;
	  
      case SENSOR_3A_AE_UNLOCK:
          S5K5EAYX_AE_Lock_Enable(AE_MODE_AUTO);
      	  break;

      case SENSOR_3A_AWB_LOCK:
          S5K5EAYX_set_param_wb(AWB_MODE_OFF);
      	  break;

      case SENSOR_3A_AWB_UNLOCK:
          S5K5EAYX_set_param_wb(AWB_MODE_AUTO);
      	  break;
      default:
      	  break;
   }
   return;
}


static UINT32 S5K5EAYX_FOCUS_Move_to(UINT32 a_u2MovePosition)
{
//
}

static void S5K5EAYX_AF_Set_Window(UINT32 zone_addr)
{
	  UINT32 FD_XS = 4;
	  UINT32 FD_YS = 3;  
	  UINT32 x0, y0, x1, y1;
	  UINT32* zone = (UINT32*)zone_addr;
	  UINT32 width_ratio, height_ratio, start_x, start_y;  
	  static UINT32 LastPosition=400;
  
	  x0 = *zone;
	  y0 = *(zone + 1);
	  x1 = *(zone + 2);
	  y1 = *(zone + 3); 	  
	  FD_XS = *(zone + 4);
	  FD_YS = *(zone + 5);
  
  	SENSORDB(" [5EA]:[S5K5EAYX]enter S5K5EAYX_AF_Set_Window function:\n ");

    //Do CAF
	if(x0==x1)
		return;

	//check if we really need to update AF window
	if((LastPosition<x0 && LastPosition+S5K5EA_FAF_TOLERANCE>x0)||
		(LastPosition>x0 && x0+S5K5EA_FAF_TOLERANCE>LastPosition))
	{
		LastPosition = x0;
		return;
	}

	LastPosition = x0;
    if(s5k5eayx_sensor_mode != S5K5EAYX_SENSORMODE_ZSD)
	{
		//width_ratio =  1024*(x1 - x0)/FD_XS;
		//height_ratio = 1024*(y1 - y0)/FD_YS;
        width_ratio = S5K5EA_PRV_RATIO_WIDTH;
		height_ratio = S5K5EA_PRV_RATIO_HEIGHT;

		start_x = x0*S5K5EA_PRE_AFRangeWidth/FD_XS;
		start_y = y0*S5K5EA_PRE_AFRangeHeight/FD_YS;
		
        if(start_x+S5K5EA_PRV_RATIO_WIDTH > S5K5EA_PRE_AFRangeWidth)
			start_x = S5K5EA_PRE_AFRangeWidth-S5K5EA_PRV_RATIO_WIDTH;
		if(start_y+S5K5EA_PRV_RATIO_HEIGHT>S5K5EA_PRE_AFRangeHeight)
			start_y = S5K5EA_PRE_AFRangeHeight-S5K5EA_PRV_RATIO_HEIGHT;
	}
	else
	{
	      //ZSD
		  //width_ratio =  2048*(x1 - x0)/FD_XS;
		  //height_ratio = 2048*(y1 - y0)/FD_YS;
		  width_ratio = S5K5EA_CAP_RATIO_WIDTH;
		  height_ratio = S5K5EA_CAP_RATIO_HEIGHT;

		  start_x = x0*S5K5EA_CAP_AFRangeWidth/FD_XS;
		  start_y = y0*S5K5EA_CAP_AFRangeHeight/FD_YS;

		if(start_x+S5K5EA_CAP_RATIO_WIDTH > S5K5EA_CAP_AFRangeWidth)
			start_x = S5K5EA_CAP_AFRangeWidth-S5K5EA_CAP_RATIO_WIDTH;
		if(start_y+S5K5EA_CAP_RATIO_HEIGHT>S5K5EA_CAP_AFRangeHeight)
			start_y = S5K5EA_CAP_AFRangeHeight-S5K5EA_CAP_RATIO_HEIGHT;

	}
  
	  SENSORDB("[5EA]:af x0=%d,y0=%d,x1=%d,y1=%d,FD_XS=%d,FD_YS=%d\n",
					 x0, y0, x1, y1, FD_XS, FD_YS);		 
	  SENSORDB("[5EA]:af start_x=%d,start_y=%d,width_ratio=%d,height_ratio=%d\n",
					 start_x, start_y, width_ratio, height_ratio);	
  
	  S5K5EAYX_write_cmos_sensor(0x002A,		 0x02D0);
	  S5K5EAYX_write_cmos_sensor(0x0F12,	  start_x); 		//#REG_TC_AF_FstWinStartX
	  S5K5EAYX_write_cmos_sensor(0x0F12,	  start_y); 		//#REG_TC_AF_FstWinStartY
	  S5K5EAYX_write_cmos_sensor(0x0F12,   width_ratio);		//#REG_TC_AF_FstWinSizeX
	  S5K5EAYX_write_cmos_sensor(0x0F12, height_ratio); 		//#REG_TC_AF_FstWinSizeY
	  S5K5EAYX_write_cmos_sensor(0x0F12,	  start_x); 		//#REG_TC_AF_ScndWinStartX
	  S5K5EAYX_write_cmos_sensor(0x0F12,	  start_y); 		//#REG_TC_AF_ScndWinStartY
	  S5K5EAYX_write_cmos_sensor(0x0F12,   width_ratio);		//#REG_TC_AF_ScndWinSizeX
	  S5K5EAYX_write_cmos_sensor(0x0F12, height_ratio); 		//#REG_TC_AF_ScndWinSizeY
	  S5K5EAYX_write_cmos_sensor(0x0F12, 0x0001);		  //#REG_TC_AF_WinSizesUpdated
      msleep(150);//need 1  fps time to update 
}


void S5K5EAYX_AE_Set_Window(UINT32 zone_addr)
{
//don't support
}

UINT32 S5K5EAYXFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
							 UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
    UINT16 u2Temp = 0; 
	UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
	UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
	UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
	UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
	MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
	MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;

   // SENSORDB(" [5EA]:S5K5EAYXFeatureControl,FeatureId=%d \n",FeatureId);
	switch (FeatureId)
	{
		case SENSOR_FEATURE_GET_RESOLUTION:
			*pFeatureReturnPara16++=S5K5EAYX_IMAGE_SENSOR_FULL_WIDTH;
			*pFeatureReturnPara16=S5K5EAYX_IMAGE_SENSOR_FULL_HEIGHT;
			*pFeatureParaLen=4;
		     break;
		case SENSOR_FEATURE_GET_PERIOD:
			switch(S5K5EAYXCurrentScenarioId)
			{
				case MSDK_SCENARIO_ID_CAMERA_ZSD:
					*pFeatureReturnPara16++=S5K5EAYX_CAP_PERIOD_PIXEL_NUMS;
					*pFeatureReturnPara16=S5K5EAYX_CAP_PERIOD_LINE_NUMS;
					*pFeatureParaLen=4;
					break;
				default:
					*pFeatureReturnPara16++=S5K5EAYX_VIDEO_PERIOD_PIXEL_NUMS;
					*pFeatureReturnPara16=S5K5EAYX_VIDEO_PERIOD_LINE_NUMS;
					*pFeatureParaLen=4;
					break;
			}
		     break;
		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ://3003
			switch(S5K5EAYXCurrentScenarioId)
			{
				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
					*pFeatureReturnPara32 = S5K5EAYX_PREVIEW_PCLK;
					*pFeatureParaLen=4;
					break;
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					*pFeatureReturnPara32 = S5K5EAYX_VIDEO_PCLK;
					*pFeatureParaLen=4;
					break;
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
				case MSDK_SCENARIO_ID_CAMERA_ZSD:
					*pFeatureReturnPara32 = S5K5EAYX_CAPTURE_PCLK;
					*pFeatureParaLen=4;
					break;
				default:
					*pFeatureReturnPara32 = S5K5EAYX_PREVIEW_PCLK;
					*pFeatureParaLen=4;
					break;
			}
		    break;
		case SENSOR_FEATURE_SET_ESHUTTER:
		     break;
		case SENSOR_FEATURE_SET_NIGHTMODE:
			 S5K5EAYX_Mode_Config((BOOL) *pFeatureData16);
		     break;
		case SENSOR_FEATURE_SET_GAIN:
			 break; 
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
		     break;
		case SENSOR_FEATURE_SET_REGISTER:
			 S5K5EAYX_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
		     break;
		case SENSOR_FEATURE_GET_REGISTER:
			 pSensorRegData->RegData = S5K5EAYX_read_cmos_sensor(pSensorRegData->RegAddr);
			 break;
		case SENSOR_FEATURE_GET_CONFIG_PARA:
			 memcpy(pSensorConfigData, &S5K5EAYXSensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
			 *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
		     break;
		case SENSOR_FEATURE_SET_CCT_REGISTER:
		case SENSOR_FEATURE_GET_CCT_REGISTER:
		case SENSOR_FEATURE_SET_ENG_REGISTER:
		case SENSOR_FEATURE_GET_ENG_REGISTER:
		case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
		case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
		case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
		case SENSOR_FEATURE_GET_GROUP_INFO:
		case SENSOR_FEATURE_GET_ITEM_INFO:
		case SENSOR_FEATURE_SET_ITEM_INFO:
		case SENSOR_FEATURE_GET_ENG_INFO:
		     break;
		case SENSOR_FEATURE_GET_GROUP_COUNT:
	         *pFeatureReturnPara32++=0;
			 *pFeatureParaLen=4;
		     break; 
		
		case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
			// get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
			// if EEPROM does not exist in camera module.
			*pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
			*pFeatureParaLen=4;
		     break;

        case SENSOR_FEATURE_SET_TEST_PATTERN:
             S5K5EAYX_SetTestPatternMode((BOOL)*pFeatureData16);
             break;
			 
		case SENSOR_FEATURE_CHECK_SENSOR_ID:
             S5K5EAYXGetSensorID(pFeatureReturnPara32); 
            break;     

		case SENSOR_FEATURE_SET_YUV_CMD:
			 S5K5EAYXYUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));
		     break;	
		case SENSOR_FEATURE_SET_VIDEO_MODE:
		     S5K5EAYXYUVSetVideoMode(*pFeatureData16);
		     break; 

		case SENSOR_FEATURE_GET_EV_AWB_REF:
		    S5K5EAYXGetEvAwbRef(*pFeatureData32);
		    break;
		
		case SENSOR_FEATURE_GET_SHUTTER_GAIN_AWB_GAIN:
		    S5K5EAYXGetCurAeAwbInfo(*pFeatureData32);
		    break;

		case SENSOR_FEATURE_AUTOTEST_CMD:
			SENSORDB("SENSOR_FEATURE_AUTOTEST_CMD\n");
			S5K5EAYX_AutoTestCmd(*pFeatureData32,*(pFeatureData32+1));
			break;

		case SENSOR_FEATURE_SET_YUV_3A_CMD:
            S5K5EAYX_3ACtrl((ACDK_SENSOR_3A_LOCK_ENUM)*pFeatureData32);
            break;

		case SENSOR_FEATURE_INITIALIZE_AF:   //3029   
			SENSORDB(" SENSOR_FEATURE_INITIALIZE_AF \n");
			break; 
			
		case SENSOR_FEATURE_MOVE_FOCUS_LENS://3031
			SENSORDB(" SENSOR_FEATURE_MOVE_FOCUS_LENS \n");
			S5K5EAYX_FOCUS_Move_to(*pFeatureData16);
			break;
			
        case SENSOR_FEATURE_GET_AF_STATUS://3032              
			SENSORDB(" SENSOR_FEATURE_GET_AF_STATUS \n");
            S5K5EAYX_Get_AF_Status(pFeatureReturnPara32);               
            *pFeatureParaLen=4;
            break;
			
		case SENSOR_FEATURE_GET_AF_INF: //3033
			SENSORDB(" SENSOR_FEATURE_GET_AF_INF \n");
			S5K5EAYX_Get_AF_Inf( pFeatureReturnPara32); 				  
			*pFeatureParaLen=4; 		   
			break;
			
		case SENSOR_FEATURE_GET_AF_MACRO: //3034
			SENSORDB(" SENSOR_FEATURE_GET_AF_MACRO \n");
			S5K5EAYX_Get_AF_Macro(pFeatureReturnPara32);			   
			*pFeatureParaLen=4; 		   
			break;	
			
		case SENSOR_FEATURE_CONSTANT_AF://3030
			SENSORDB(" SENSOR_FEATURE_CONSTANT_AF \n");
			S5K5EAYX_Constant_Focus(); 
			break;	
			
		case SENSOR_FEATURE_SET_AF_WINDOW://3041
			SENSORDB(" SENSOR_FEATURE_SET_AF_WINDOW \n");
			S5K5EAYX_AF_Set_Window(*pFeatureData32);
			 break;
			 
        case SENSOR_FEATURE_SINGLE_FOCUS_MODE://3039
			SENSORDB(" SENSOR_FEATURE_SINGLE_FOCUS_MODE \n");
            S5K5EAYX_Single_Focus(); 
            break;	
			
        case SENSOR_FEATURE_CANCEL_AF://3040
			SENSORDB(" SENSOR_FEATURE_CANCEL_AF \n");
            S5K5EAYX_Cancel_Focus();
            break;

        case SENSOR_FEATURE_SET_AE_WINDOW:
			 S5K5EAYX_AE_Set_Window(*pFeatureData32);
			 break;
			
        case SENSOR_FEATURE_GET_AF_MAX_NUM_FOCUS_AREAS:
            S5K5EAYX_Get_AF_Max_Num_Focus_Areas(pFeatureReturnPara32);            
            *pFeatureParaLen=4;
            break;        
        case SENSOR_FEATURE_GET_AE_MAX_NUM_METERING_AREAS:
            S5K5EAYX_Get_AE_Max_Num_Metering_Areas(pFeatureReturnPara32);            
            *pFeatureParaLen=4;
            break;

		case SENSOR_FEATURE_GET_EXIF_INFO:
			S5K5EA_GetExifInfo(*pFeatureData32);
			break;
			
        case SENSOR_FEATURE_GET_AE_AWB_LOCK_INFO:
             S5K5EAYX_Get_AEAWB_lock_info((*pFeatureData32),*(pFeatureData32+1));

        case SENSOR_FEATURE_GET_DELAY_INFO:
             S5K5EAYX_GetDelayInfo(*pFeatureData32);
             break;

        case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
             S5K5EAYX_SetMaxFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, *(pFeatureData32+1));
             break;

        case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
             S5K5EAYX_GetDefaultFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, (MUINT32 *)(*(pFeatureData32+1)));
             break;

		case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
			*pFeatureReturnPara32=S5K5EAYX_TEST_PATTERN_CHECKSUM;
			*pFeatureParaLen=4;
			break;

        /**********************Strobe Ctrl Start *******************************/
        case SENSOR_FEATURE_GET_AE_FLASHLIGHT_INFO:
             SENSORDB("[5EA] F_GET_AE_FLASHLIGHT_INFO: Not Support\n");
             break;

        case SENSOR_FEATURE_GET_TRIGGER_FLASHLIGHT_INFO:
             S5K5EAYX_FlashTriggerCheck(pFeatureData32);
             SENSORDB("[5EA] F_GET_TRIGGER_FLASHLIGHT_INFO: %d\n", pFeatureData32);
             break;

        case SENSOR_FEATURE_SET_FLASHLIGHT:
             SENSORDB("S5K45EAYX SENSOR_FEATURE_SET_FLASHLIGHT\n");
             break;
        /**********************Strobe Ctrl End *******************************/

		default:
			 break;			
	}

	return ERROR_NONE;
}	/* S5K5EAYXFeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncS5K5EAYX=
{
	S5K5EAYXOpen,            
	S5K5EAYXGetInfo,          
	S5K5EAYXGetResolution,    
	S5K5EAYXFeatureControl,   
	S5K5EAYXControl,         
	S5K5EAYXClose           
};

UINT32 S5K5EAYX_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&SensorFuncS5K5EAYX;

	return ERROR_NONE;
}	/* SensorInit() */


