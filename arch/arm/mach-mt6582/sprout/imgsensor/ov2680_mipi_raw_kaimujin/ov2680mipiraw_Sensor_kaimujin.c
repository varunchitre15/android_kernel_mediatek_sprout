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
 *   Sensor.c
 *
 * Project:
 * --------
 *   DUMA
 *
 * Description:
 * ------------
 *   Image sensor driver function
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/xlog.h>
#include <asm/system.h>
#include <linux/proc_fs.h>
#include <linux/dma-mapping.h>


#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>

#include "ov2680mipiraw_Sensor_kaimujin.h"
#include "ov2680mipiraw_Camera_Sensor_para_kaimujin.h"
#include "ov2680mipiraw_CameraCustomized_kaimujin.h"

#define OV2680KAI_DEBUG
#define OV2680KAI_DRIVER_TRACE
#define LOG_TAG "[OV2680KAIMIPIRaw]"
#ifdef OV2680KAI_DEBUG
#define SENSORDB(fmt,arg...) printk(LOG_TAG "%s: " fmt "\n", __FUNCTION__ ,##arg)
#else
#define SENSORDB(fmt,arg...)
#endif

//#define OV2680KAI_TEST_PATTERN_CHECKSUM 0x86da3e5a
#define OV2680KAI_TEST_PATTERN_CHECKSUM 0x92dcaca1


//kal_bool OV2680KAI_during_testpattern = KAL_FALSE;


//#define ACDK
extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
extern int iMultiWriteReg(u8 *pData, u16 lens, u16 i2cId);

//#define USE_I2C_MULTIWRITE
#define OV2680KAI_multi_write_cmos_sensor(pData, lens) iMultiWriteReg((u8*) pData, (u16) lens, OV2680KAI_SLAVE_WRITE_ID_1)


static MSDK_SCENARIO_ID_ENUM OV2680KAICurrentScenarioId = MSDK_SCENARIO_ID_CAMERA_PREVIEW;
static OV2680KAI_sensor_struct OV2680KAI_sensor =
{
  .eng =
  {
    .reg = OV2680_CAMERA_SENSOR_REG_DEFAULT_VALUE,
    .cct = OV2680_CAMERA_SENSOR_CCT_DEFAULT_VALUE,
  },
  .eng_info =
  {
    .SensorId = OV2680MIPI_SENSOR_ID_KAIMUJIN,
    .SensorType = CMOS_SENSOR,
    .SensorOutputDataFormat = OV2680KAI_COLOR_FORMAT,
  },
  .shutter = 0x20,
  .gain = 0x20,
  .pv_pclk = OV2680KAI_PREVIEW_CLK,
  .cap_pclk = OV2680KAI_CAPTURE_CLK,
  .pclk = OV2680KAI_PREVIEW_CLK,
  .frame_height = OV2680KAI_PV_PERIOD_LINE_NUMS,
  .line_length = OV2680KAI_PV_PERIOD_PIXEL_NUMS,
  .is_zsd = KAL_FALSE, //for zsd
  .dummy_pixels = 0,
  .dummy_lines = 0,  //for zsd
  .is_autofliker = KAL_FALSE,
};

static DEFINE_SPINLOCK(OV2680KAI_drv_lock);

kal_uint16 OV2680KAI_read_cmos_sensor(kal_uint32 addr)
{
    kal_uint16 get_byte=0;
    char puSendCmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
    iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,1,OV2680KAI_sensor.write_id);
#ifdef OV2680KAI_DRIVER_TRACE
    //SENSORDB("OV2680KAI_read_cmos_sensor, addr:%x;get_byte:%x \n",addr,get_byte);
#endif
    return get_byte;
}

kal_uint16 OV2680KAI_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
    //kal_uint16 reg_tmp;

    char puSendCmd[3] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para & 0xFF)};

    iWriteRegI2C(puSendCmd , 3,OV2680KAI_sensor.write_id);
    return ERROR_NONE;
}

/*****************************  OTP Feature  **********************************/
//#define OV2680KAI_USE_OTP



/*****************************  OTP Feature  End**********************************/

void OV2680KAI_Write_Shutter(kal_uint16 ishutter)
{

    kal_uint16 extra_shutter = 0;
    kal_uint16 realtime_fp = 0;
    kal_uint16 frame_height = 0;
    kal_uint16 line_length = 0;

    unsigned long flags;
#ifdef OV2680KAI_DRIVER_TRACE
    SENSORDB("OV2680KAI_write_shutter:%x \n",ishutter);
#endif
    if (!ishutter) ishutter = 1; /* avoid 0 */
    //if(ishutter > 0xfff)

    frame_height = OV2680KAI_PV_PERIOD_LINE_NUMS + OV2680KAI_sensor.dummy_lines;

    if(ishutter > (frame_height -4))
    {
        extra_shutter = ishutter - frame_height + 4;
        SENSORDB("[shutter > frame_height] frame_height:%x extra_shutter:%x \n",frame_height,extra_shutter);
    }
    else
    {
        extra_shutter = 0;
    }
    frame_height += extra_shutter;
    OV2680KAI_sensor.frame_height = frame_height;
    SENSORDB("OV2680KAI_sensor.is_autofliker:%x, OV2680KAI_sensor.frame_height: %x \n",OV2680KAI_sensor.is_autofliker,OV2680KAI_sensor.frame_height);
    #if 1
    if(OV2680KAI_sensor.is_autofliker == KAL_TRUE)
    {
        realtime_fp = OV2680KAI_sensor.pclk *10 / ((OV2680KAI_sensor.line_length/OV2680KAI_MIPI_LANE_NUM) * OV2680KAI_sensor.frame_height);
        SENSORDB("[OV2680KAI_Write_Shutter]pv_clk:%d\n",OV2680KAI_sensor.pclk);
        SENSORDB("[OV2680KAI_Write_Shutter]line_length/4:%d\n",(OV2680KAI_sensor.line_length/OV2680KAI_MIPI_LANE_NUM));
        SENSORDB("[OV2680KAI_Write_Shutter]frame_height:%d\n",OV2680KAI_sensor.frame_height);
        SENSORDB("[OV2680KAI_Write_Shutter]framerate(10base):%d\n",realtime_fp);

        if((realtime_fp >= 297)&&(realtime_fp <= 303))
        {
            realtime_fp = 296;
            spin_lock_irqsave(&OV2680KAI_drv_lock,flags);
            OV2680KAI_sensor.frame_height = OV2680KAI_sensor.pclk *10 / ((OV2680KAI_sensor.line_length/OV2680KAI_MIPI_LANE_NUM) * realtime_fp);
            spin_unlock_irqrestore(&OV2680KAI_drv_lock,flags);

            SENSORDB("[autofliker realtime_fp=30,extern heights slowdown to 29.6fps][height:%d]",OV2680KAI_sensor.frame_height);
        }
      else if((realtime_fp >= 147)&&(realtime_fp <= 153))
        {
            realtime_fp = 146;
            spin_lock_irqsave(&OV2680KAI_drv_lock,flags);
            OV2680KAI_sensor.frame_height = OV2680KAI_sensor.pclk *10 / ((OV2680KAI_sensor.line_length/OV2680KAI_MIPI_LANE_NUM) * realtime_fp);
            spin_unlock_irqrestore(&OV2680KAI_drv_lock,flags);
            SENSORDB("[autofliker realtime_fp=15,extern heights slowdown to 14.6fps][height:%d]",OV2680KAI_sensor.frame_height);
        }
    //OV2680KAI_sensor.frame_height = OV2680KAI_sensor.frame_height +(OV2680KAI_sensor.frame_height>>7);

    }
    #endif
    OV2680KAI_write_cmos_sensor(0x380e, (OV2680KAI_sensor.frame_height>>8)&0xFF);
    OV2680KAI_write_cmos_sensor(0x380f, (OV2680KAI_sensor.frame_height)&0xFF);

    OV2680KAI_write_cmos_sensor(0x3500, (ishutter >> 12) & 0xF);
    OV2680KAI_write_cmos_sensor(0x3501, (ishutter >> 4) & 0xFF);
    OV2680KAI_write_cmos_sensor(0x3502, (ishutter << 4) & 0xFF);

}


/*************************************************************************
* FUNCTION
*   OV2680KAI_Set_Dummy
*
* DESCRIPTION
*   This function set dummy pixel or dummy line of OV2680KAI
*
* PARAMETERS
*   iPixels : dummy pixel
*   iLines :  dummy linel
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/

static void OV2680KAI_Set_Dummy(const kal_uint16 iPixels, const kal_uint16 iLines)
{
    kal_uint16 line_length, frame_height;
#ifdef OV2680KAI_DRIVER_TRACE
    SENSORDB("OV2680KAI_Set_Dummy:iPixels:%x; iLines:%x \n",iPixels,iLines);
#endif


    OV2680KAI_sensor.dummy_lines = iLines;
    OV2680KAI_sensor.dummy_pixels = iPixels;

    line_length = OV2680KAI_PV_PERIOD_PIXEL_NUMS + iPixels;
    frame_height = OV2680KAI_PV_PERIOD_LINE_NUMS + iLines;

#ifdef OV2680KAI_DRIVER_TRACE
    SENSORDB("line_length:%x; frame_height:%x \n",line_length,frame_height);
#endif

    if ((line_length >= 0xFFFF)||(frame_height >= 0xFFFF)) // need check
    {
        #ifdef OV2680KAI_DRIVER_TRACE
        SENSORDB("ERROR: line length or frame height is overflow!!!!!!!!  \n");
        #endif
        return ERROR_NONE;
    }
//    if((line_length == OV2680KAI_sensor.line_length)&&(frame_height == OV2680KAI_sensor.frame_height))
//        return ;
    spin_lock(&OV2680KAI_drv_lock);
    OV2680KAI_sensor.line_length = line_length;
    OV2680KAI_sensor.frame_height = frame_height;
    spin_unlock(&OV2680KAI_drv_lock);

    SENSORDB("line_length:%x; frame_height:%x \n",line_length,frame_height);
    SENSORDB("write to register line_length/4:%x; frame_height:%x \n",(line_length/4),frame_height);

    /*  Add dummy pixels: */
    /* 0x380c [0:4], 0x380d defines the PCLKs in one line of OV2680KAI  */
    /* Add dummy lines:*/
    /* 0x380e [0:1], 0x380f defines total lines in one frame of OV2680KAI */
    OV2680KAI_write_cmos_sensor(0x380c, (line_length/OV2680KAI_MIPI_LANE_NUM) >> 8);
    OV2680KAI_write_cmos_sensor(0x380d, (line_length/OV2680KAI_MIPI_LANE_NUM) & 0xFF);
    OV2680KAI_write_cmos_sensor(0x380e, frame_height >> 8);
    OV2680KAI_write_cmos_sensor(0x380f, frame_height & 0xFF);
    return ERROR_NONE;
}   /*  OV2680KAI_Set_Dummy    */


/*************************************************************************
* FUNCTION
*    OV2680KAI_SetShutter
*
* DESCRIPTION
*    This function set e-shutter of OV2680KAI to change exposure time.
*
* PARAMETERS
*   iShutter : exposured lines
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/


void set_OV2680KAI_shutter(kal_uint16 iShutter)
{

    unsigned long flags;
#ifdef OV2680KAI_DRIVER_TRACE
    SENSORDB("set_OV2680KAI_shutter:%x \n",iShutter);
#endif

    spin_lock_irqsave(&OV2680KAI_drv_lock,flags);
    OV2680KAI_sensor.shutter = iShutter;
    spin_unlock_irqrestore(&OV2680KAI_drv_lock,flags);

    OV2680KAI_Write_Shutter(iShutter);

}   /*  Set_OV2680KAI_Shutter */

 kal_uint16 OV2680KAIGain2Reg(const kal_uint16 iGain)
{
    kal_uint16 iReg = 0x00;

    //iReg = ((iGain / BASEGAIN) << 4) + ((iGain % BASEGAIN) * 16 / BASEGAIN);
    iReg = iGain *16 / BASEGAIN;

    iReg = iReg & 0xFF;
#ifdef OV2680KAI_DRIVER_TRACE
    SENSORDB("OV2680KAIGain2Reg:iGain:%x; iReg:%x \n",iGain,iReg);
#endif
    return iReg;
}


kal_uint16 OV2680KAI_SetGain(kal_uint16 iGain)
{
   kal_uint16 i;
   kal_uint16 gain_reg = 0;
#ifdef OV2680KAI_DRIVER_TRACE
   SENSORDB("OV2680KAI_SetGain:iGain = %x;\n",iGain);
   SENSORDB("OV2680KAI_SetGain:gain_reg 0 = %x;\n",gain_reg);

#endif

    if(!iGain) {
        SENSORDB("OV2680KAI_SetGain[ERROR]:gain is zero, iGain = %x;\n",iGain);
        iGain = 64;   // 1x base
        }
    gain_reg = iGain/4; //sensor gain base 1x= 16, IReg = iGain/64*16;
    gain_reg &= 0x3ff;

    OV2680KAI_write_cmos_sensor(0x350b, (gain_reg&0xff));
    OV2680KAI_write_cmos_sensor(0x3508, (gain_reg>>8));
    return ERROR_NONE;
}




/*************************************************************************
* FUNCTION
*    OV2680KAI_SetGain
*
* DESCRIPTION
*    This function is to set global gain to sensor.
*
* PARAMETERS
*   iGain : sensor global gain(base: 0x40)
*
* RETURNS
*    the actually gain set to sensor.
*
* GLOBALS AFFECTED
*
*************************************************************************/

#if 0
void OV2680KAI_set_isp_driving_current(kal_uint16 current)
{
#ifdef OV2680KAI_DRIVER_TRACE
   SENSORDB("OV2680KAI_set_isp_driving_current:current:%x;\n",current);
#endif
  //iowrite32((0x2 << 12)|(0<<28)|(0x8880888), 0xF0001500);
}
#endif

/*************************************************************************
* FUNCTION
*    OV2680KAI_NightMode
*
* DESCRIPTION
*    This function night mode of OV2680KAI.
*
* PARAMETERS
*    bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void OV2680KAI_night_mode(kal_bool enable)
{
}   /*  OV2680KAI_NightMode    */


/* write camera_para to sensor register */
static void OV2680KAI_camera_para_to_sensor(void)
{
    kal_uint32 i;
#ifdef OV2680KAI_DRIVER_TRACE
    SENSORDB("OV2680KAI_camera_para_to_sensor\n");
#endif
  for (i = 0; 0xFFFFFFFF != OV2680KAI_sensor.eng.reg[i].Addr; i++)
  {
    OV2680KAI_write_cmos_sensor(OV2680KAI_sensor.eng.reg[i].Addr, OV2680KAI_sensor.eng.reg[i].Para);
  }
  for (i = OV2680KAI_FACTORY_START_ADDR; 0xFFFFFFFF != OV2680KAI_sensor.eng.reg[i].Addr; i++)
  {
    OV2680KAI_write_cmos_sensor(OV2680KAI_sensor.eng.reg[i].Addr, OV2680KAI_sensor.eng.reg[i].Para);
  }
  OV2680KAI_SetGain(OV2680KAI_sensor.gain); /* update gain */
}

/* update camera_para from sensor register */
static void OV2680KAI_sensor_to_camera_para(void)
{
  kal_uint32 i;
  kal_uint32 temp_data;

#ifdef OV2680KAI_DRIVER_TRACE
   SENSORDB("OV2680KAI_sensor_to_camera_para\n");
#endif
  for (i = 0; 0xFFFFFFFF != OV2680KAI_sensor.eng.reg[i].Addr; i++)
  {
    temp_data = OV2680KAI_read_cmos_sensor(OV2680KAI_sensor.eng.reg[i].Addr);

    spin_lock(&OV2680KAI_drv_lock);
    OV2680KAI_sensor.eng.reg[i].Para = temp_data;
    spin_unlock(&OV2680KAI_drv_lock);

    }
  for (i = OV2680KAI_FACTORY_START_ADDR; 0xFFFFFFFF != OV2680KAI_sensor.eng.reg[i].Addr; i++)
  {
    temp_data = OV2680KAI_read_cmos_sensor(OV2680KAI_sensor.eng.reg[i].Addr);

    spin_lock(&OV2680KAI_drv_lock);
    OV2680KAI_sensor.eng.reg[i].Para = temp_data;
    spin_unlock(&OV2680KAI_drv_lock);
  }
}

/* ------------------------ Engineer mode ------------------------ */
inline static void OV2680KAI_get_sensor_group_count(kal_int32 *sensor_count_ptr)
{
#ifdef OV2680KAI_DRIVER_TRACE
   SENSORDB("OV2680KAI_get_sensor_group_count\n");
#endif
  *sensor_count_ptr = OV2680KAI_GROUP_TOTAL_NUMS;
}

inline static void OV2680KAI_get_sensor_group_info(MSDK_SENSOR_GROUP_INFO_STRUCT *para)
{
#ifdef OV2680KAI_DRIVER_TRACE
   SENSORDB("OV2680KAI_get_sensor_group_info\n");
#endif
  switch (para->GroupIdx)
  {
  case OV2680KAI_PRE_GAIN:
    sprintf(para->GroupNamePtr, "CCT");
    para->ItemCount = 5;
    break;
  case OV2680KAI_CMMCLK_CURRENT:
    sprintf(para->GroupNamePtr, "CMMCLK Current");
    para->ItemCount = 1;
    break;
  case OV2680KAI_FRAME_RATE_LIMITATION:
    sprintf(para->GroupNamePtr, "Frame Rate Limitation");
    para->ItemCount = 2;
    break;
  case OV2680KAI_REGISTER_EDITOR:
    sprintf(para->GroupNamePtr, "Register Editor");
    para->ItemCount = 2;
    break;
  default:
    ASSERT(0);
  }
}

inline static void OV2680KAI_get_sensor_item_info(MSDK_SENSOR_ITEM_INFO_STRUCT *para)
{

  const static kal_char *cct_item_name[] = {"SENSOR_BASEGAIN", "Pregain-R", "Pregain-Gr", "Pregain-Gb", "Pregain-B"};
  const static kal_char *editer_item_name[] = {"REG addr", "REG value"};

#ifdef OV2680KAI_DRIVER_TRACE
     SENSORDB("OV2680KAI_get_sensor_item_info\n");
#endif
  switch (para->GroupIdx)
  {
  case OV2680KAI_PRE_GAIN:
    switch (para->ItemIdx)
    {
    case OV2680KAI_SENSOR_BASEGAIN:
    case OV2680KAI_PRE_GAIN_R_INDEX:
    case OV2680KAI_PRE_GAIN_Gr_INDEX:
    case OV2680KAI_PRE_GAIN_Gb_INDEX:
    case OV2680KAI_PRE_GAIN_B_INDEX:
      break;
    default:
      ASSERT(0);
    }
    sprintf(para->ItemNamePtr, cct_item_name[para->ItemIdx - OV2680KAI_SENSOR_BASEGAIN]);
    para->ItemValue = OV2680KAI_sensor.eng.cct[para->ItemIdx].Para * 1000 / BASEGAIN;
    para->IsTrueFalse = para->IsReadOnly = para->IsNeedRestart = KAL_FALSE;
    para->Min = OV2680KAI_MIN_ANALOG_GAIN * 1000;
    para->Max = OV2680KAI_MAX_ANALOG_GAIN * 1000;
    break;
  case OV2680KAI_CMMCLK_CURRENT:
    switch (para->ItemIdx)
    {
    case 0:
      sprintf(para->ItemNamePtr, "Drv Cur[2,4,6,8]mA");
      switch (OV2680KAI_sensor.eng.reg[OV2680KAI_CMMCLK_CURRENT_INDEX].Para)
      {
      case ISP_DRIVING_2MA:
        para->ItemValue = 2;
        break;
      case ISP_DRIVING_4MA:
        para->ItemValue = 4;
        break;
      case ISP_DRIVING_6MA:
        para->ItemValue = 6;
        break;
      case ISP_DRIVING_8MA:
        para->ItemValue = 8;
        break;
      default:
        ASSERT(0);
      }
      para->IsTrueFalse = para->IsReadOnly = KAL_FALSE;
      para->IsNeedRestart = KAL_TRUE;
      para->Min = 2;
      para->Max = 8;
      break;
    default:
      ASSERT(0);
    }
    break;
  case OV2680KAI_FRAME_RATE_LIMITATION:
    switch (para->ItemIdx)
    {
    case 0:
      sprintf(para->ItemNamePtr, "Max Exposure Lines");
      para->ItemValue = 5998;
      break;
    case 1:
      sprintf(para->ItemNamePtr, "Min Frame Rate");
      para->ItemValue = 5;
      break;
    default:
      ASSERT(0);
    }
    para->IsTrueFalse = para->IsNeedRestart = KAL_FALSE;
    para->IsReadOnly = KAL_TRUE;
    para->Min = para->Max = 0;
    break;
  case OV2680KAI_REGISTER_EDITOR:
    switch (para->ItemIdx)
    {
    case 0:
    case 1:
      sprintf(para->ItemNamePtr, editer_item_name[para->ItemIdx]);
      para->ItemValue = 0;
      para->IsTrueFalse = para->IsReadOnly = para->IsNeedRestart = KAL_FALSE;
      para->Min = 0;
      para->Max = (para->ItemIdx == 0 ? 0xFFFF : 0xFF);
      break;
    default:
      ASSERT(0);
    }
    break;
  default:
    ASSERT(0);
  }
}

inline static kal_bool OV2680KAI_set_sensor_item_info(MSDK_SENSOR_ITEM_INFO_STRUCT *para)
{
  kal_uint16 temp_para;
#ifdef OV2680KAI_DRIVER_TRACE
   SENSORDB("OV2680KAI_set_sensor_item_info\n");
#endif
  switch (para->GroupIdx)
  {
  case OV2680KAI_PRE_GAIN:
    switch (para->ItemIdx)
    {
    case OV2680KAI_SENSOR_BASEGAIN:
    case OV2680KAI_PRE_GAIN_R_INDEX:
    case OV2680KAI_PRE_GAIN_Gr_INDEX:
    case OV2680KAI_PRE_GAIN_Gb_INDEX:
    case OV2680KAI_PRE_GAIN_B_INDEX:
        spin_lock(&OV2680KAI_drv_lock);
        OV2680KAI_sensor.eng.cct[para->ItemIdx].Para = para->ItemValue * BASEGAIN / 1000;
        spin_unlock(&OV2680KAI_drv_lock);

        OV2680KAI_SetGain(OV2680KAI_sensor.gain); /* update gain */
        break;
    default:
        ASSERT(0);
    }
    break;
  case OV2680KAI_CMMCLK_CURRENT:
    switch (para->ItemIdx)
    {
    case 0:
      switch (para->ItemValue)
      {
      case 2:
        temp_para = ISP_DRIVING_2MA;
        break;
      case 3:
      case 4:
        temp_para = ISP_DRIVING_4MA;
        break;
      case 5:
      case 6:
        temp_para = ISP_DRIVING_6MA;
        break;
      default:
        temp_para = ISP_DRIVING_8MA;
        break;
      }
        //OV2680KAI_set_isp_driving_current((kal_uint16)temp_para);
        spin_lock(&OV2680KAI_drv_lock);
        OV2680KAI_sensor.eng.reg[OV2680KAI_CMMCLK_CURRENT_INDEX].Para = temp_para;
        spin_unlock(&OV2680KAI_drv_lock);
      break;
    default:
      ASSERT(0);
    }
    break;
  case OV2680KAI_FRAME_RATE_LIMITATION:
    ASSERT(0);
    break;
  case OV2680KAI_REGISTER_EDITOR:
    switch (para->ItemIdx)
    {
      static kal_uint32 fac_sensor_reg;
    case 0:
      if (para->ItemValue < 0 || para->ItemValue > 0xFFFF) return KAL_FALSE;
      fac_sensor_reg = para->ItemValue;
      break;
    case 1:
      if (para->ItemValue < 0 || para->ItemValue > 0xFF) return KAL_FALSE;
      OV2680KAI_write_cmos_sensor(fac_sensor_reg, para->ItemValue);
      break;
    default:
      ASSERT(0);
    }
    break;
  default:
    ASSERT(0);
  }
  return KAL_TRUE;
}



#ifdef USE_I2C_MULTIWRITE
static kal_uint8 OV2680KAI_init[]={

    //0x01, 0x03, 0x01,
    0x30, 0x02, 0x00,
    0x30, 0x16, 0x1c,
    0x30, 0x18, 0x44,
    0x30, 0x20, 0x00,
    0x30, 0x80, 0x02,
    0x30, 0x82, 0x37,
    0x30, 0x84, 0x09,
    0x30, 0x85, 0x04,
    0x30, 0x86, 0x01,
    0x35, 0x01, 0x26,
    0x35, 0x02, 0x40,
    0x35, 0x03, 0x03,
    0x35, 0x0b, 0x36,
    0x36, 0x00, 0xb4,
    0x36, 0x03, 0x39,
    0x36, 0x04, 0x24,
    0x36, 0x05, 0x00,
    0x36, 0x20, 0x26,
    0x36, 0x21, 0x37,
    0x36, 0x22, 0x04,
    0x36, 0x28, 0x00,
    0x37, 0x05, 0x3c,
    0x37, 0x0c, 0x50,
    0x37, 0x0d, 0xc0,
    0x37, 0x18, 0x88,
    0x37, 0x20, 0x00,
    0x37, 0x21, 0x00,
    0x37, 0x22, 0x00,
    0x37, 0x23, 0x00,
    0x37, 0x38, 0x00,
    0x37, 0x0a, 0x23,
    0x37, 0x17, 0x58,
    0x37, 0x81, 0x80,
    0x37, 0x89, 0x60,
    0x38, 0x00, 0x00,
    0x38, 0x01, 0x00,
    0x38, 0x02, 0x00,
    0x38, 0x03, 0x00,
    0x38, 0x04, 0x06,
    0x38, 0x05, 0x4f,
    0x38, 0x06, 0x04,
    0x38, 0x07, 0xbf,
    0x38, 0x08, 0x03,
    0x38, 0x09, 0x20,
    0x38, 0x0a, 0x02,
    0x38, 0x0b, 0x58,
    0x38, 0x0c, 0x06,
    0x38, 0x0d, 0xac,
    0x38, 0x0e, 0x02,
    0x38, 0x0f, 0x84,
    0x38, 0x10, 0x00,
    0x38, 0x11, 0x04,
    0x38, 0x12, 0x00,
    0x38, 0x13, 0x04,
    0x38, 0x14, 0x31,
    0x38, 0x15, 0x31,
    0x38, 0x19, 0x04,
    0x38, 0x20, 0xc2,
    0x38, 0x21, 0x01,
    0x40, 0x00, 0x81,
    0x40, 0x01, 0x40,
    0x40, 0x08, 0x00,
    0x40, 0x09, 0x03,
    0x46, 0x02, 0x02,
    0x48, 0x1f, 0x36,
    0x48, 0x25, 0x36,
    0x48, 0x37, 0x30,
    0x50, 0x02, 0x30,
    0x50, 0x80, 0x00,
    0x50, 0x81, 0x41,
    0x01, 0x00, 0x01,

};

static kal_uint8 OV2680KAI_preview[]={
    0x01, 0x00, 0x00,
    0x30, 0x86, 0x00,
    0x35, 0x01, 0x4e,
    0x35, 0x02, 0xe0,
    0x36, 0x20, 0x26,
    0x36, 0x21, 0x37,
    0x36, 0x22, 0x04,
    0x37, 0x0a, 0x21,
    0x37, 0x0d, 0xc0,
    0x37, 0x18, 0x88,
    0x37, 0x21, 0x00,
    0x37, 0x22, 0x00,
    0x37, 0x23, 0x00,
    0x37, 0x38, 0x00,
    0x38, 0x03, 0x00,
    0x38, 0x07, 0xbf,
    0x38, 0x08, 0x06,
    0x38, 0x09, 0x40,
    0x38, 0x0a, 0x04,
    0x38, 0x0b, 0xb0,
    0x38, 0x0c, 0x06,
    0x38, 0x0d, 0xa4,
    0x38, 0x0e, 0x05,
    0x38, 0x0f, 0x0e,
    0x38, 0x11, 0x08,
    0x38, 0x13, 0x08,
    0x38, 0x14, 0x11,
    0x38, 0x15, 0x11,
    0x38, 0x20, 0xc0,
    0x38, 0x21, 0x00,
    0x40, 0x08, 0x02,
    0x40, 0x09, 0x09,
    0x48, 0x37, 0x18,
    0x01, 0x00, 0x01,

};

static kal_uint8 OV2680KAI_fullsize_15fps[]={
    0x01, 0x00, 0x00,
    0x30, 0x86, 0x01,
    0x35, 0x01, 0x4e,
    0x35, 0x02, 0xe0,
    0x36, 0x20, 0x24,
    0x36, 0x21, 0x34,
    0x36, 0x22, 0x03,
    0x37, 0x0a, 0x21,
    0x37, 0x0d, 0x00,
    0x37, 0x18, 0x80,
    0x37, 0x21, 0x09,
    0x37, 0x22, 0x0b,
    0x37, 0x23, 0x48,
    0x37, 0x38, 0x99,
    0x38, 0x03, 0x00,
    0x38, 0x07, 0xbf,
    0x38, 0x08, 0x06,
    0x38, 0x09, 0x40,
    0x38, 0x0a, 0x04,
    0x38, 0x0b, 0xb0,
    0x38, 0x0c, 0x06,
    0x38, 0x0d, 0xa4,
    0x38, 0x0e, 0x05,
    0x38, 0x0f, 0x0e,
    0x38, 0x11, 0x08,
    0x38, 0x13, 0x08,
    0x38, 0x14, 0x11,
    0x38, 0x15, 0x11,
    0x38, 0x20, 0xc0,
    0x38, 0x21, 0x00,
    0x40, 0x08, 0x02,
    0x40, 0x09, 0x09,
    0x48, 0x37, 0x30,
    0x01, 0x00, 0x01,

};

//static kal_uint8 OV2680KAI_capture[]={
//    };

#else
static void OV2680KAIMIPI_Sensor_Init(void)
{
    /*
    @@Initial - MIPI 4-Lane 4608x3456 10-bit 15fps_640Mbps_lane
    100 99 4608 3456
    100 98 1 1
    102 81 0 ffff
    102 84 1 ffff
    102 3601 5dc
    102 3f00 da2
    102 910 31
    */

    //;Reset
    OV2680KAI_write_cmos_sensor(0x0103, 0x01);
    mdelay(50);
    OV2680KAI_write_cmos_sensor(0x3002, 0x00);
    OV2680KAI_write_cmos_sensor(0x3016, 0x1c);
    OV2680KAI_write_cmos_sensor(0x3018, 0x44);
    OV2680KAI_write_cmos_sensor(0x3020, 0x00);
    OV2680KAI_write_cmos_sensor(0x3080, 0x02);
    OV2680KAI_write_cmos_sensor(0x3082, 0x37);
    OV2680KAI_write_cmos_sensor(0x3084, 0x09);
    OV2680KAI_write_cmos_sensor(0x3085, 0x04);
    OV2680KAI_write_cmos_sensor(0x3086, 0x01);
    OV2680KAI_write_cmos_sensor(0x3501, 0x26);
    OV2680KAI_write_cmos_sensor(0x3502, 0x40);
    OV2680KAI_write_cmos_sensor(0x3503, 0x03);
    OV2680KAI_write_cmos_sensor(0x350b, 0x36);
    OV2680KAI_write_cmos_sensor(0x3600, 0xb4);
    OV2680KAI_write_cmos_sensor(0x3603, 0x39);
    OV2680KAI_write_cmos_sensor(0x3604, 0x24);
    OV2680KAI_write_cmos_sensor(0x3605, 0x00);
    OV2680KAI_write_cmos_sensor(0x3620, 0x26);
    OV2680KAI_write_cmos_sensor(0x3621, 0x37);
    OV2680KAI_write_cmos_sensor(0x3622, 0x04);
    OV2680KAI_write_cmos_sensor(0x3628, 0x00);
    OV2680KAI_write_cmos_sensor(0x3705, 0x3c);
    OV2680KAI_write_cmos_sensor(0x370c, 0x50);
    OV2680KAI_write_cmos_sensor(0x370d, 0xc0);
    OV2680KAI_write_cmos_sensor(0x3718, 0x88);
    OV2680KAI_write_cmos_sensor(0x3720, 0x00);
    OV2680KAI_write_cmos_sensor(0x3721, 0x00);
    OV2680KAI_write_cmos_sensor(0x3722, 0x00);
    OV2680KAI_write_cmos_sensor(0x3723, 0x00);
    OV2680KAI_write_cmos_sensor(0x3738, 0x00);
    OV2680KAI_write_cmos_sensor(0x370a, 0x23);
    OV2680KAI_write_cmos_sensor(0x3717, 0x58);
    OV2680KAI_write_cmos_sensor(0x3781, 0x80);
    OV2680KAI_write_cmos_sensor(0x3784, 0x0c); //
    OV2680KAI_write_cmos_sensor(0x3789, 0x60);
    OV2680KAI_write_cmos_sensor(0x3800, 0x00);
    OV2680KAI_write_cmos_sensor(0x3801, 0x00);
    OV2680KAI_write_cmos_sensor(0x3802, 0x00);
    OV2680KAI_write_cmos_sensor(0x3803, 0x00);
    OV2680KAI_write_cmos_sensor(0x3804, 0x06);
    OV2680KAI_write_cmos_sensor(0x3805, 0x4f);
    OV2680KAI_write_cmos_sensor(0x3806, 0x04);
    OV2680KAI_write_cmos_sensor(0x3807, 0xbf);
    OV2680KAI_write_cmos_sensor(0x3808, 0x03);
    OV2680KAI_write_cmos_sensor(0x3809, 0x20);
    OV2680KAI_write_cmos_sensor(0x380a, 0x02);
    OV2680KAI_write_cmos_sensor(0x380b, 0x58);
    OV2680KAI_write_cmos_sensor(0x380c, 0x06);
    OV2680KAI_write_cmos_sensor(0x380d, 0xac);
    OV2680KAI_write_cmos_sensor(0x380e, 0x02);
    OV2680KAI_write_cmos_sensor(0x380f, 0x84);
    OV2680KAI_write_cmos_sensor(0x3810, 0x00);
    OV2680KAI_write_cmos_sensor(0x3811, 0x04);
    OV2680KAI_write_cmos_sensor(0x3812, 0x00);
    OV2680KAI_write_cmos_sensor(0x3813, 0x04);
    OV2680KAI_write_cmos_sensor(0x3814, 0x31);
    OV2680KAI_write_cmos_sensor(0x3815, 0x31);
    OV2680KAI_write_cmos_sensor(0x3819, 0x04);
    OV2680KAI_write_cmos_sensor(0x3820, 0xc4);
    OV2680KAI_write_cmos_sensor(0x3821, 0x04);
    OV2680KAI_write_cmos_sensor(0x4000, 0x81);
    OV2680KAI_write_cmos_sensor(0x4001, 0x40);
    OV2680KAI_write_cmos_sensor(0x4008, 0x00);
    OV2680KAI_write_cmos_sensor(0x4009, 0x03);
    OV2680KAI_write_cmos_sensor(0x4602, 0x02);
    OV2680KAI_write_cmos_sensor(0x481f, 0x36);
    OV2680KAI_write_cmos_sensor(0x4825, 0x36);
    OV2680KAI_write_cmos_sensor(0x4837, 0x30);
    OV2680KAI_write_cmos_sensor(0x5002, 0x30);
    OV2680KAI_write_cmos_sensor(0x5080, 0x00);
    OV2680KAI_write_cmos_sensor(0x5081, 0x41);
    OV2680KAI_write_cmos_sensor(0x3021, 0x23);
    OV2680KAI_write_cmos_sensor(0x0100, 0x01);


    mdelay(40);


}   /*  OV2680KAIMIPI_Sensor_Init  */   /*  OV2680KAIMIPI_Sensor_Init  */

static void OV2680KAIMIPI_Sensor_preview(void)
{
    //-------------------------------------------------------------------------------


    SENSORDB("OV2680KAIMIPIPreview Setting \n");

    OV2680KAI_write_cmos_sensor(0x0100, 0x00);
    OV2680KAI_write_cmos_sensor(0x3086, 0x00);
    OV2680KAI_write_cmos_sensor(0x3501, 0x4e);
    OV2680KAI_write_cmos_sensor(0x3502, 0xe0);
    OV2680KAI_write_cmos_sensor(0x3620, 0x26);
    OV2680KAI_write_cmos_sensor(0x3621, 0x37);
    OV2680KAI_write_cmos_sensor(0x3622, 0x04);
    OV2680KAI_write_cmos_sensor(0x370a, 0x21);
    OV2680KAI_write_cmos_sensor(0x370d, 0xc0);
    OV2680KAI_write_cmos_sensor(0x3718, 0x88);
    OV2680KAI_write_cmos_sensor(0x3721, 0x00);
    OV2680KAI_write_cmos_sensor(0x3722, 0x00);
    OV2680KAI_write_cmos_sensor(0x3723, 0x00);
    OV2680KAI_write_cmos_sensor(0x3738, 0x00);
    OV2680KAI_write_cmos_sensor(0x3803, 0x00);
    OV2680KAI_write_cmos_sensor(0x3807, 0xbf);
    OV2680KAI_write_cmos_sensor(0x3808, 0x06);
    OV2680KAI_write_cmos_sensor(0x3809, 0x40);
    OV2680KAI_write_cmos_sensor(0x380a, 0x04);
    OV2680KAI_write_cmos_sensor(0x380b, 0xb0);
    OV2680KAI_write_cmos_sensor(0x380c, 0x06);
    OV2680KAI_write_cmos_sensor(0x380d, 0xa4);
    OV2680KAI_write_cmos_sensor(0x380e, 0x05);
    OV2680KAI_write_cmos_sensor(0x380f, 0x0e);
    OV2680KAI_write_cmos_sensor(0x3811, 0x08);
    OV2680KAI_write_cmos_sensor(0x3813, 0x08);
    OV2680KAI_write_cmos_sensor(0x3814, 0x11);
    OV2680KAI_write_cmos_sensor(0x3815, 0x11);
    OV2680KAI_write_cmos_sensor(0x3820,((OV2680KAI_read_cmos_sensor(0x3820) & 0x04) | 0xc0));//keep the mirror and flip
    OV2680KAI_write_cmos_sensor(0x3821,((OV2680KAI_read_cmos_sensor(0x3821) & 0x04) | 0x00));
    OV2680KAI_write_cmos_sensor(0x4008, 0x02);
    OV2680KAI_write_cmos_sensor(0x4009, 0x09);
    OV2680KAI_write_cmos_sensor(0x4837, 0x18);
    OV2680KAI_write_cmos_sensor(0x0100, 0x01);
    mdelay(60);

}
static void OV2680KAIMIPI_Sensor_2M_15fps(void)
{
    //-------------------------------------------------------------------------------


    SENSORDB("OV2680KAIMIPIPreview Setting \n");

    OV2680KAI_write_cmos_sensor(0x0100, 0x00);
    OV2680KAI_write_cmos_sensor(0x3086, 0x01);
    OV2680KAI_write_cmos_sensor(0x3501, 0x4e);
    OV2680KAI_write_cmos_sensor(0x3502, 0xe0);
    OV2680KAI_write_cmos_sensor(0x3620, 0x24);
    OV2680KAI_write_cmos_sensor(0x3621, 0x34);
    OV2680KAI_write_cmos_sensor(0x3622, 0x03);
    OV2680KAI_write_cmos_sensor(0x370a, 0x21);
    OV2680KAI_write_cmos_sensor(0x370d, 0x00);
    OV2680KAI_write_cmos_sensor(0x3718, 0x80);
    OV2680KAI_write_cmos_sensor(0x3721, 0x09);
    OV2680KAI_write_cmos_sensor(0x3722, 0x0b);
    OV2680KAI_write_cmos_sensor(0x3723, 0x48);
    OV2680KAI_write_cmos_sensor(0x3738, 0x99);
    OV2680KAI_write_cmos_sensor(0x3803, 0x00);
    OV2680KAI_write_cmos_sensor(0x3807, 0xbf);
    OV2680KAI_write_cmos_sensor(0x3808, 0x06);
    OV2680KAI_write_cmos_sensor(0x3809, 0x40);
    OV2680KAI_write_cmos_sensor(0x380a, 0x04);
    OV2680KAI_write_cmos_sensor(0x380b, 0xb0);
    OV2680KAI_write_cmos_sensor(0x380c, 0x06);
    OV2680KAI_write_cmos_sensor(0x380d, 0xa4);
    OV2680KAI_write_cmos_sensor(0x380e, 0x05);
    OV2680KAI_write_cmos_sensor(0x380f, 0x0e);
    OV2680KAI_write_cmos_sensor(0x3811, 0x08);
    OV2680KAI_write_cmos_sensor(0x3813, 0x08);
    OV2680KAI_write_cmos_sensor(0x3814, 0x11);
    OV2680KAI_write_cmos_sensor(0x3815, 0x11);
    OV2680KAI_write_cmos_sensor(0x3820,((OV2680KAI_read_cmos_sensor(0x3820) & 0x04) | 0xc0));//keep the mirror and flip
    OV2680KAI_write_cmos_sensor(0x3821,((OV2680KAI_read_cmos_sensor(0x3821) & 0x04) | 0x00));
    OV2680KAI_write_cmos_sensor(0x4008, 0x02);
    OV2680KAI_write_cmos_sensor(0x4009, 0x09);
    OV2680KAI_write_cmos_sensor(0x4837, 0x30);
    OV2680KAI_write_cmos_sensor(0x0100, 0x01);
    mdelay(60);

}


#endif
static UINT32 OV2680KAIMultiWrite(kal_uint8* pData, int iLen)
{
    int totalCnt = 0, len = 0;
    int transfer_len, transac_len=3;
    kal_uint8* pBuf=NULL;
    dma_addr_t dmaHandle;
    pBuf = (kal_uint8*)kmalloc(1024, GFP_KERNEL);

    totalCnt = iLen;
    transfer_len = totalCnt / transac_len;
    len = (transfer_len<<8)|transac_len;

    SENSORDB("Total Count = %d, Len = 0x%x\n", totalCnt,len);
    memcpy(pBuf, pData, totalCnt );
    dmaHandle = dma_map_single(NULL, pBuf, 1024, DMA_TO_DEVICE);
    OV2680KAI_multi_write_cmos_sensor(dmaHandle, len);
    dma_unmap_single(NULL, dmaHandle, 1024, DMA_TO_DEVICE);

    SENSORDB("OV2680KAIMultiWrite exit :\n ");

}

UINT32 OV2680KAIOpen(void)
{
    kal_uint16 sensor_id=0;
    int i, iLen;
    const kal_uint16 sccb_writeid[] = {OV2680KAI_SLAVE_WRITE_ID_1,OV2680KAI_SLAVE_WRITE_ID_2};
    SENSORDB("OV2680KAIOpen\n");


   spin_lock(&OV2680KAI_drv_lock);
   OV2680KAI_sensor.is_zsd = KAL_FALSE;  //for zsd full size preview
   OV2680KAI_sensor.is_zsd_cap = KAL_FALSE;
   OV2680KAI_sensor.is_autofliker = KAL_FALSE; //for autofliker.
   OV2680KAI_sensor.pv_mode = KAL_FALSE;
   OV2680KAI_sensor.pclk = OV2680KAI_PREVIEW_CLK;
   spin_unlock(&OV2680KAI_drv_lock);

  for(i = 0; i <(sizeof(sccb_writeid)/sizeof(sccb_writeid[0])); i++)
    {
        spin_lock(&OV2680KAI_drv_lock);
        OV2680KAI_sensor.write_id = sccb_writeid[i];
        OV2680KAI_sensor.read_id = (sccb_writeid[i]|0x01);
        spin_unlock(&OV2680KAI_drv_lock);

        sensor_id=((OV2680KAI_read_cmos_sensor(0x300A) << 8) | OV2680KAI_read_cmos_sensor(0x300B));

#ifdef OV2680KAI_DRIVER_TRACE
        SENSORDB("OV2680KAIOpen, sensor_id:%x \n",sensor_id);
#endif
        if(OV2680MIPI_SENSOR_ID == sensor_id)
        {
            SENSORDB("OV2680KAI slave write id:%x \n",OV2680KAI_sensor.write_id);
            break;
        }
    }

    // check if sensor ID correct
    if (sensor_id != OV2680MIPI_SENSOR_ID)
    {
        SENSORDB("OV2680KAI Check ID fails! \n");

        SENSORDB("[Warning]OV2680KAIGetSensorID, sensor_id:%x \n",sensor_id);
        //sensor_id = OV2680KAI_SENSOR_ID;
        sensor_id = 0xFFFFFFFF;
        return ERROR_SENSOR_CONNECT_FAIL;
    }

    //OV2680KAI_global_setting();
    #ifdef USE_I2C_MULTIWRITE
    OV2680KAI_write_cmos_sensor(0x0103, 0x01);
    mdelay(30);
    iLen = ARRAY_SIZE(OV2680KAI_init);
    OV2680KAIMultiWrite(OV2680KAI_init, iLen);
    mdelay(40);

    iLen = ARRAY_SIZE(OV2680KAI_preview);
    OV2680KAIMultiWrite(OV2680KAI_preview, iLen);
    mdelay(60);

    #else
    OV2680KAIMIPI_Sensor_Init();
    OV2680KAIMIPI_Sensor_preview();

    #endif

    spin_lock(&OV2680KAI_drv_lock);
    OV2680KAI_sensor.dummy_pixels = 0;
    OV2680KAI_sensor.dummy_lines = 0;
    OV2680KAI_sensor.line_length = OV2680KAI_PV_PERIOD_PIXEL_NUMS;
    OV2680KAI_sensor.frame_height = OV2680KAI_PV_PERIOD_LINE_NUMS;
    OV2680KAI_sensor.pclk = OV2680KAI_PREVIEW_CLK;
    spin_unlock(&OV2680KAI_drv_lock);

    OV2680KAI_Set_Dummy(0, 0); /* modify dummy_pixel must gen AE table again */

    SENSORDB("open End \n");

   return ERROR_NONE;
}   /* OV2680KAIOpen  */

/*************************************************************************
* FUNCTION
*   OV5642GetSensorID
*
* DESCRIPTION
*   This function get the sensor ID
*
* PARAMETERS
*   *sensorID : return the sensor ID
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
#define GPIO_SUB_CAM_ID   GPIO19
#define GPIO_SUB_CAM_ID_M_GPIO   GPIO_MODE_00
UINT32 OV2680KAIGetSensorID(UINT32 *sensorID)
{

   int i;
   kal_uint16 hw_id=0;
   const kal_uint16 sccb_writeid[] = {OV2680KAI_SLAVE_WRITE_ID_1, OV2680KAI_SLAVE_WRITE_ID_2};

 SENSORDB("OV2680KAIGetSensorID enter,\n");
    for(i = 0; i <(sizeof(sccb_writeid)/sizeof(sccb_writeid[0])); i++)
    {
        spin_lock(&OV2680KAI_drv_lock);
        OV2680KAI_sensor.write_id = sccb_writeid[i];
        OV2680KAI_sensor.read_id = (sccb_writeid[i]|0x01);
        spin_unlock(&OV2680KAI_drv_lock);

        *sensorID=((OV2680KAI_read_cmos_sensor(0x300A) << 8) | OV2680KAI_read_cmos_sensor(0x300B));

#ifdef OV2680KAI_DRIVER_TRACE
        SENSORDB("OV2680KAIGetSensorID, sensor_id:%x \n",*sensorID);
#endif
        if(OV2680MIPI_SENSOR_ID == *sensorID)
        {
            SENSORDB("OV2680KAI slave write id:%x \n",OV2680KAI_sensor.write_id);
            break;
        }
    }
    //mt_set_gpio_mode(GPIO_SUB_CAM_ID,GPIO_SUB_CAM_ID_M_GPIO);
    //mt_set_gpio_dir(GPIO_SUB_CAM_ID,GPIO_DIR_IN);
    //mt_set_gpio_pull_enable(GPIO_SUB_CAM_ID, GPIO_PULL_ENABLE);
    hw_id=mt_get_gpio_in(GPIO_UART_UCTS2_PIN); // GPIO 19
    mdelay(1);
    //SENSORDB("OV2680 hw_id:%x \n",hw_id);

    // check if sensor ID correct
    if ((*sensorID == OV2680MIPI_SENSOR_ID) && (0x00 == hw_id))
    {
        *sensorID = OV2680MIPI_SENSOR_ID_KAIMUJIN;
        SENSORDB("This his is ov2680 kaimujin camera module.");
    }
    else
    {
             SENSORDB("[Warning]OV2680GetSensorID, sensor_id:%x(hw_id=%d) \n",*sensorID,hw_id);
            *sensorID = 0xFFFFFFFF;
            return ERROR_SENSOR_CONNECT_FAIL;
    }
    SENSORDB("OV2680KAIGetSensorID exit,\n");
   return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*    OV2680KAIClose
*
* DESCRIPTION
*    This function is to turn off sensor module power.
*
* PARAMETERS
*    None
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 OV2680KAIClose(void)
{
#ifdef OV2680KAI_DRIVER_TRACE
   SENSORDB("OV2680KAIClose\n");
#endif

    return ERROR_NONE;
}   /* OV2680KAIClose */

/*************************************************************************
* FUNCTION
* OV2680KAIPreview
*
* DESCRIPTION
*    This function start the sensor preview.
*
* PARAMETERS
*    *image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 OV2680KAIPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
        MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    kal_uint16 dummy_line;
    int iLen;
#ifdef OV2680KAI_DRIVER_TRACE
    SENSORDB("OV2680KAIPreview \n");
#endif
    spin_lock(&OV2680KAI_drv_lock);
    OV2680KAI_sensor.pv_mode = KAL_TRUE;
    OV2680KAI_sensor.is_zsd = KAL_FALSE;
    spin_unlock(&OV2680KAI_drv_lock);

    spin_lock(&OV2680KAI_drv_lock);
    OV2680KAI_sensor.video_mode = KAL_FALSE;
    spin_unlock(&OV2680KAI_drv_lock);
    dummy_line = 0;
    //OV2680KAI_sensor.frame_height = OV2680KAI_PV_PERIOD_LINE_NUMS;
    OV2680KAI_sensor.dummy_pixels = 0;
    OV2680KAI_sensor.dummy_lines = 0;
    OV2680KAI_sensor.line_length = OV2680KAI_PV_PERIOD_PIXEL_NUMS;
    OV2680KAI_sensor.frame_height = OV2680KAI_PV_PERIOD_LINE_NUMS + dummy_line;
    OV2680KAI_sensor.pclk = OV2680KAI_PREVIEW_CLK;

    OV2680KAI_Set_Dummy(0, dummy_line); /* modify dummy_pixel must gen AE table again */

    return ERROR_NONE;

}   /*  OV2680KAIPreview   */


/*************************************************************************
* FUNCTION
* OV2680KAIVIDEO
*
* DESCRIPTION
*    This function start the sensor Video preview.
*
* PARAMETERS
*    *image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/

UINT32 OV2680KAIVIDEO(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
        MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    kal_uint16 dummy_line;
    kal_uint16 ret;
    int iLen;
#ifdef OV2680KAI_DRIVER_TRACE
    SENSORDB("OV2680KAIVIDEO \n");
#endif
    spin_lock(&OV2680KAI_drv_lock);
    OV2680KAI_sensor.pv_mode = KAL_FALSE;
    OV2680KAI_sensor.is_zsd = KAL_FALSE;
    spin_unlock(&OV2680KAI_drv_lock);

    spin_lock(&OV2680KAI_drv_lock);
    OV2680KAI_sensor.video_mode = KAL_TRUE;
    spin_unlock(&OV2680KAI_drv_lock);
    dummy_line = 0;

    //OV2680KAI_sensor.frame_height = OV2680KAI_PV_PERIOD_LINE_NUMS;
    OV2680KAI_sensor.dummy_pixels = 0;
    OV2680KAI_sensor.dummy_lines = 0;
    OV2680KAI_sensor.line_length = OV2680KAI_PV_PERIOD_PIXEL_NUMS;
    OV2680KAI_sensor.frame_height = OV2680KAI_PV_PERIOD_LINE_NUMS + dummy_line;
    OV2680KAI_sensor.pclk = OV2680KAI_PREVIEW_CLK;

    OV2680KAI_Set_Dummy(0, dummy_line); /* modify dummy_pixel must gen AE table again */

    return ERROR_NONE;

}   /*  OV2680KAIVIDEO   */


/*************************************************************************
* FUNCTION
*    OV2680KAIZsdPreview
*
* DESCRIPTION
*    This function setup the CMOS sensor in Full Size output  mode
*
* PARAMETERS
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 OV2680KAIZsdPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
    MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)

{
    kal_uint16 dummy_pixel = 0;
    kal_uint16 dummy_line = 0;
    kal_uint16 ret;
    int iLen;
#ifdef OV2680KAI_DRIVER_TRACE
    SENSORDB("OV2680KAIZsdPreview \n");
#endif

    spin_lock(&OV2680KAI_drv_lock);
    OV2680KAI_sensor.pv_mode = KAL_FALSE;
    OV2680KAI_sensor.is_zsd = KAL_TRUE;
    spin_unlock(&OV2680KAI_drv_lock);

    spin_lock(&OV2680KAI_drv_lock);
    OV2680KAI_sensor.video_mode = KAL_FALSE;
    spin_unlock(&OV2680KAI_drv_lock);
        dummy_line = 0;
    //OV2680KAI_sensor.frame_height = OV2680KAI_PV_PERIOD_LINE_NUMS;
    OV2680KAI_sensor.dummy_pixels = 0;
    OV2680KAI_sensor.dummy_lines = 0;
    OV2680KAI_sensor.line_length = OV2680KAI_PV_PERIOD_PIXEL_NUMS;
    OV2680KAI_sensor.frame_height = OV2680KAI_PV_PERIOD_LINE_NUMS + dummy_line;
    OV2680KAI_sensor.pclk = OV2680KAI_PREVIEW_CLK;

    OV2680KAI_Set_Dummy(0, dummy_line); /* modify dummy_pixel must gen AE table again */
    return ERROR_NONE;
}



/*************************************************************************
* FUNCTION
*OV2680KAICapture
*
* DESCRIPTION
*    This function setup the CMOS sensor in capture MY_OUTPUT mode
*
* PARAMETERS
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 OV2680KAICapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
        MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    kal_uint16 dummy_pixel = 0;
    kal_uint16 dummy_line = 0;
    kal_uint16 ret;
    int iLen;
#ifdef OV2680KAI_DRIVER_TRACE
    SENSORDB("OV2680KAICapture start test1\n");
#endif

    spin_lock(&OV2680KAI_drv_lock);
    OV2680KAI_sensor.video_mode = KAL_FALSE;
    //OV2680KAI_sensor.is_autofliker = KAL_FALSE;
    OV2680KAI_sensor.pv_mode = KAL_FALSE;
    spin_unlock(&OV2680KAI_drv_lock);

    dummy_line = 0;
    //OV2680KAI_sensor.frame_height = OV2680KAI_PV_PERIOD_LINE_NUMS;
    OV2680KAI_sensor.dummy_pixels = 0;
    OV2680KAI_sensor.dummy_lines = 0;
    OV2680KAI_sensor.line_length = OV2680KAI_PV_PERIOD_PIXEL_NUMS;
    OV2680KAI_sensor.frame_height = OV2680KAI_PV_PERIOD_LINE_NUMS + dummy_line;
    OV2680KAI_sensor.pclk = OV2680KAI_PREVIEW_CLK;

    OV2680KAI_Set_Dummy(0, dummy_line); /* modify dummy_pixel must gen AE table again */


    return ERROR_NONE;
}   /* OV2680KAI_Capture() */


UINT32 OV2680KAI3DPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    kal_uint16 dummy_line;
    kal_uint16 ret;
    int iLen;
#ifdef OV2680KAI_DRIVER_TRACE
    SENSORDB("OV2680KAI3DPreview \n");
#endif

    spin_lock(&OV2680KAI_drv_lock);
    OV2680KAI_sensor.pv_mode = KAL_TRUE;
    spin_unlock(&OV2680KAI_drv_lock);

      spin_lock(&OV2680KAI_drv_lock);
    OV2680KAI_sensor.video_mode = KAL_FALSE;
    spin_unlock(&OV2680KAI_drv_lock);

    return ERROR_NONE;

}


UINT32 OV2680KAIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
#ifdef OV2680KAI_DRIVER_TRACE
    SENSORDB("OV2680KAIGetResolution \n");
#endif
    pSensorResolution->SensorFullWidth=OV2680KAI_IMAGE_SENSOR_FULL_WIDTH;
    pSensorResolution->SensorFullHeight=OV2680KAI_IMAGE_SENSOR_FULL_HEIGHT;
    pSensorResolution->SensorPreviewWidth=OV2680KAI_IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorPreviewHeight=OV2680KAI_IMAGE_SENSOR_PV_HEIGHT;
    pSensorResolution->SensorVideoWidth=OV2680KAI_IMAGE_SENSOR_VIDEO_WIDTH;
    pSensorResolution->SensorVideoHeight=OV2680KAI_IMAGE_SENSOR_VIDEO_HEIGHT;
    pSensorResolution->Sensor3DFullWidth=OV2680KAI_IMAGE_SENSOR_3D_FULL_WIDTH;
    pSensorResolution->Sensor3DFullHeight=OV2680KAI_IMAGE_SENSOR_3D_FULL_HEIGHT;
    pSensorResolution->Sensor3DPreviewWidth=OV2680KAI_IMAGE_SENSOR_3D_PV_WIDTH;
    pSensorResolution->Sensor3DPreviewHeight=OV2680KAI_IMAGE_SENSOR_3D_PV_HEIGHT;
    pSensorResolution->Sensor3DVideoWidth=OV2680KAI_IMAGE_SENSOR_3D_VIDEO_WIDTH;
    pSensorResolution->Sensor3DVideoHeight=OV2680KAI_IMAGE_SENSOR_3D_VIDEO_HEIGHT;
    return ERROR_NONE;
}/* OV2680KAIGetResolution() */

UINT32 OV2680KAIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
        MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
        MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
#ifdef OV2680KAI_DRIVER_TRACE
    SENSORDB("OV2680KAIGetInfo£¬FeatureId:%d\n",ScenarioId);
#endif

    switch(ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_ZSD:
            pSensorInfo->SensorPreviewResolutionX=OV2680KAI_IMAGE_SENSOR_FULL_WIDTH;
            pSensorInfo->SensorPreviewResolutionY=OV2680KAI_IMAGE_SENSOR_FULL_HEIGHT;
            pSensorInfo->SensorFullResolutionX=OV2680KAI_IMAGE_SENSOR_FULL_WIDTH;
            pSensorInfo->SensorFullResolutionY=OV2680KAI_IMAGE_SENSOR_FULL_HEIGHT;
            pSensorInfo->SensorCameraPreviewFrameRate = 30;
            break;
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
            pSensorInfo->SensorPreviewResolutionX=OV2680KAI_IMAGE_SENSOR_PV_WIDTH;
            pSensorInfo->SensorPreviewResolutionY=OV2680KAI_IMAGE_SENSOR_PV_HEIGHT;
            pSensorInfo->SensorFullResolutionX=OV2680KAI_IMAGE_SENSOR_FULL_WIDTH;
            pSensorInfo->SensorFullResolutionY=OV2680KAI_IMAGE_SENSOR_FULL_HEIGHT;
            pSensorInfo->SensorCameraPreviewFrameRate=30;
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
            pSensorInfo->SensorPreviewResolutionX=OV2680KAI_IMAGE_SENSOR_VIDEO_WIDTH;
            pSensorInfo->SensorPreviewResolutionY=OV2680KAI_IMAGE_SENSOR_VIDEO_HEIGHT;
            pSensorInfo->SensorFullResolutionX=OV2680KAI_IMAGE_SENSOR_FULL_WIDTH;
            pSensorInfo->SensorFullResolutionY=OV2680KAI_IMAGE_SENSOR_FULL_HEIGHT;
            pSensorInfo->SensorCameraPreviewFrameRate=30;
            break;
      case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
      case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
      case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added
           pSensorInfo->SensorPreviewResolutionX=OV2680KAI_IMAGE_SENSOR_3D_VIDEO_WIDTH;
           pSensorInfo->SensorPreviewResolutionY=OV2680KAI_IMAGE_SENSOR_3D_VIDEO_HEIGHT;
           pSensorInfo->SensorFullResolutionX=OV2680KAI_IMAGE_SENSOR_3D_FULL_WIDTH;
           pSensorInfo->SensorFullResolutionY=OV2680KAI_IMAGE_SENSOR_3D_FULL_HEIGHT;
           pSensorInfo->SensorCameraPreviewFrameRate=30;
          break;
        default:
            pSensorInfo->SensorPreviewResolutionX=OV2680KAI_IMAGE_SENSOR_PV_WIDTH;
            pSensorInfo->SensorPreviewResolutionY=OV2680KAI_IMAGE_SENSOR_PV_HEIGHT;
            pSensorInfo->SensorFullResolutionX=OV2680KAI_IMAGE_SENSOR_FULL_WIDTH;
            pSensorInfo->SensorFullResolutionY=OV2680KAI_IMAGE_SENSOR_FULL_HEIGHT;
            pSensorInfo->SensorCameraPreviewFrameRate = 30;
            break;
    }

    //pSensorInfo->SensorCameraPreviewFrameRate=30;
    pSensorInfo->SensorVideoFrameRate=30;
    pSensorInfo->SensorStillCaptureFrameRate=30;
    pSensorInfo->SensorWebCamCaptureFrameRate=15;
    pSensorInfo->SensorResetActiveHigh=FALSE; //low active
    pSensorInfo->SensorResetDelayCount=5;

    pSensorInfo->SensorOutputDataFormat=OV2680KAI_COLOR_FORMAT;
    pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorInterruptDelayLines = 4;
    pSensorInfo->SensroInterfaceType        = SENSOR_INTERFACE_TYPE_MIPI;
    pSensorInfo->CaptureDelayFrame = 3; //1;
    pSensorInfo->PreviewDelayFrame = 2;
    pSensorInfo->VideoDelayFrame = 1;

    pSensorInfo->SensorMasterClockSwitch = 0;
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_4MA;
    pSensorInfo->AEShutDelayFrame = 0;   /* The frame of setting shutter default 0 for TG int */
    pSensorInfo->AESensorGainDelayFrame = 0;/* The frame of setting sensor gain */
    pSensorInfo->AEISPGainDelayFrame = 2;
    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockDividCount=3;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = OV2680KAI_PV_X_START;
            pSensorInfo->SensorGrabStartY = OV2680KAI_PV_Y_START;

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 26;
            pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockDividCount=    3;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = OV2680KAI_PV_X_START;
            pSensorInfo->SensorGrabStartY = OV2680KAI_PV_Y_START;

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 26;
            pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x
            pSensorInfo->SensorPacketECCOrder = 1;

            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        case MSDK_SCENARIO_ID_CAMERA_ZSD:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockDividCount= 3;
            pSensorInfo->SensorClockRisingCount=0;
            pSensorInfo->SensorClockFallingCount=2;
            pSensorInfo->SensorPixelClockCount=3;
            pSensorInfo->SensorDataLatchCount=2;
            pSensorInfo->SensorGrabStartX = OV2680KAI_FULL_X_START;
            pSensorInfo->SensorGrabStartY = OV2680KAI_FULL_Y_START;
            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;    //Hesong Modify 10/25  SENSOR_MIPI_2_LANE
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 26;
            pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x
            pSensorInfo->SensorPacketECCOrder = 1;
        break;

        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockDividCount=    3;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX=  OV2680KAI_3D_PV_X_START;
            pSensorInfo->SensorGrabStartY = OV2680KAI_3D_PV_Y_START;
            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;    //Hesong Modify 10/25  SENSOR_MIPI_2_LANE
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 26;
            pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x
            pSensorInfo->SensorPacketECCOrder = 1;

            break;
        default:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockDividCount=3;
            pSensorInfo->SensorClockRisingCount=0;
            pSensorInfo->SensorClockFallingCount=2;
            pSensorInfo->SensorPixelClockCount=3;
            pSensorInfo->SensorDataLatchCount=2;
            pSensorInfo->SensorGrabStartX = OV2680KAI_PV_X_START;
            pSensorInfo->SensorGrabStartY = OV2680KAI_PV_Y_START;

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 26;
            pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
    }
  return ERROR_NONE;
}    /* OV2680KAIGetInfo() */


UINT32 OV2680KAIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
        MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
#ifdef OV2680KAI_DRIVER_TRACE
    SENSORDB("OV2680KAIControl£¬ScenarioId:%d\n",ScenarioId);
#endif

    spin_lock(&OV2680KAI_drv_lock);
    OV2680KAICurrentScenarioId = ScenarioId;
    spin_unlock(&OV2680KAI_drv_lock);

    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            OV2680KAIPreview(pImageWindow, pSensorConfigData);
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
            OV2680KAIVIDEO(pImageWindow, pSensorConfigData);
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
            OV2680KAICapture(pImageWindow, pSensorConfigData);
        break;
        case MSDK_SCENARIO_ID_CAMERA_ZSD:
            OV2680KAIZsdPreview(pImageWindow, pSensorConfigData);
        break;
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added
            OV2680KAI3DPreview(pImageWindow, pSensorConfigData);
            break;
        default:
            return ERROR_INVALID_SCENARIO_ID;
    }
    return ERROR_NONE;
}    /* OV2680KAIControl() */

UINT32 OV2680KAISetAutoFlickerMode(kal_bool bEnable, UINT16 u2FrameRate)
{

    //kal_uint32 pv_max_frame_rate_lines = OV2680KAI_sensor.dummy_lines;

    SENSORDB("[OV2680KAISetAutoFlickerMode] bEnable = d%, frame rate(10base) = %d\n", bEnable, u2FrameRate);

    if(bEnable)
    {
        spin_lock(&OV2680KAI_drv_lock);
        OV2680KAI_sensor.is_autofliker = KAL_TRUE;
        spin_unlock(&OV2680KAI_drv_lock);
    }
    else
    {
        spin_lock(&OV2680KAI_drv_lock);
        OV2680KAI_sensor.is_autofliker = KAL_FALSE;
        spin_unlock(&OV2680KAI_drv_lock);
    }
    SENSORDB("[OV2680KAISetAutoFlickerMode]bEnable:%x \n",bEnable);
    return ERROR_NONE;
}


UINT32 OV2680KAISetCalData(PSET_SENSOR_CALIBRATION_DATA_STRUCT pSetSensorCalData)
{
    UINT32 i;
    SENSORDB("OV2680KAI Sensor write calibration data num = %d \r\n", pSetSensorCalData->DataSize);
    SENSORDB("OV2680KAI Sensor write calibration data format = %x \r\n", pSetSensorCalData->DataFormat);
    if(pSetSensorCalData->DataSize <= MAX_SHADING_DATA_TBL){
        for (i = 0; i < pSetSensorCalData->DataSize; i++){
            if (((pSetSensorCalData->DataFormat & 0xFFFF) == 1) && ((pSetSensorCalData->DataFormat >> 16) == 1)){
                SENSORDB("OV2680KAI Sensor write calibration data: address = %x, value = %x \r\n",(pSetSensorCalData->ShadingData[i])>>16,(pSetSensorCalData->ShadingData[i])&0xFFFF);
                OV2680KAI_write_cmos_sensor((pSetSensorCalData->ShadingData[i])>>16, (pSetSensorCalData->ShadingData[i])&0xFFFF);
            }
        }
    }
    return ERROR_NONE;
}

UINT32 OV2680KAISetTestPatternMode(kal_bool bEnable)
{
    SENSORDB("[OV2680KAISetTestPatternMode] Test pattern enable:%d\n", bEnable);
    if(bEnable == KAL_TRUE)
    {
        //OV2680KAI_during_testpattern = KAL_TRUE;
        OV2680KAI_write_cmos_sensor(0x5080,0x80);

        SENSORDB("[OV2680KAISetTestPatternMode] Test pattern enable==TRUE:\n");
    }
    else
    {
        //OV2680KAI_during_testpattern = KAL_FALSE;
        OV2680KAI_write_cmos_sensor(0x5080,0x00);

        SENSORDB("[OV2680KAISetTestPatternMode] Test pattern enable==FALSE:\n");
    }

    return ERROR_NONE;
}

UINT32 OV2680KAISetMaxFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 frameRate)
{
    kal_uint32 pclk;
    kal_int16 dummyLine;
    kal_uint16 lineLength,frameHeight;

    SENSORDB("OV2680KAISetMaxFramerateByScenario: scenarioId = %d, frame rate = %d\n",scenarioId,frameRate);
    switch (scenarioId) {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            pclk = OV2680KAI_PREVIEW_CLK;
            lineLength = OV2680KAI_PV_PERIOD_PIXEL_NUMS;
            frameHeight = (10 * pclk)/frameRate/(lineLength/OV2680KAI_MIPI_LANE_NUM);
            dummyLine = frameHeight - OV2680KAI_PV_PERIOD_LINE_NUMS;
            if (dummyLine < 0){
            dummyLine = 0;
            }
            OV2680KAI_Set_Dummy(0, dummyLine);
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
            pclk = OV2680KAI_VIDEO_CLK;
            lineLength = OV2680KAI_VIDEO_PERIOD_PIXEL_NUMS;
            frameHeight = (10 * pclk)/frameRate/(lineLength/OV2680KAI_MIPI_LANE_NUM);
            dummyLine = frameHeight - OV2680KAI_VIDEO_PERIOD_LINE_NUMS;
            if (dummyLine < 0){
            dummyLine = 0;
            }
            OV2680KAI_Set_Dummy(0, dummyLine);
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        case MSDK_SCENARIO_ID_CAMERA_ZSD:
            pclk = OV2680KAI_CAPTURE_CLK;
            lineLength = OV2680KAI_FULL_PERIOD_PIXEL_NUMS;
            frameHeight = (10 * pclk)/frameRate/(lineLength/OV2680KAI_MIPI_LANE_NUM);
            if(frameHeight < OV2680KAI_FULL_PERIOD_LINE_NUMS)
            frameHeight = OV2680KAI_FULL_PERIOD_LINE_NUMS;
            dummyLine = frameHeight - OV2680KAI_FULL_PERIOD_LINE_NUMS;

            SENSORDB("OV2680KAISetMaxFramerateByScenario: scenarioId = %d, frame rate calculate = %d\n",((10 * pclk)/frameHeight/lineLength));
            OV2680KAI_Set_Dummy(0, dummyLine);
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
        return ERROR_NONE;
}


UINT32 OV2680KAIGetDefaultFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 *pframeRate)
{
    switch (scenarioId) {
    case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
        *pframeRate = 300;
        break;
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
    case MSDK_SCENARIO_ID_CAMERA_ZSD:
        *pframeRate = 300;
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

UINT32 OV2680KAISetVideoMode(UINT16 u2FrameRate)
{
    kal_int16 dummy_line;
    /* to fix VSYNC, to fix frame rate */
#ifdef OV2680KAI_DRIVER_TRACE
    SENSORDB("OV2680KAISetVideoMode£¬u2FrameRate:%d\n",u2FrameRate);
#endif


    if((30 == u2FrameRate)||(15 == u2FrameRate)||(24 == u2FrameRate))
    {
        dummy_line = OV2680KAI_sensor.pclk / u2FrameRate / (OV2680KAI_sensor.line_length/OV2680KAI_MIPI_LANE_NUM) - OV2680KAI_sensor.frame_height;
        if (dummy_line < 0)
            dummy_line = 0;
#ifdef OV2680KAI_DRIVER_TRACE
        SENSORDB("dummy_line %d\n", dummy_line);
#endif
        OV2680KAI_Set_Dummy(0, dummy_line); /* modify dummy_pixel must gen AE table again */
        spin_lock(&OV2680KAI_drv_lock);
        OV2680KAI_sensor.video_mode = KAL_TRUE;
        spin_unlock(&OV2680KAI_drv_lock);
    }
    else if(0 == u2FrameRate)
    {
        spin_lock(&OV2680KAI_drv_lock);
        OV2680KAI_sensor.video_mode = KAL_FALSE;
        spin_unlock(&OV2680KAI_drv_lock);

        SENSORDB("disable video mode, dynamic frame rate\n");
    }
    else{
        SENSORDB("[OV2680KAISetVideoMode],Error Framerate, u2FrameRate=%d",u2FrameRate);
    }
    return ERROR_NONE;
}

static void OV2680MIPI_Set_Mirror_Flip(kal_uint8 image_mirror, kal_uint8 image_flip)
{
    kal_uint8 HV;
    // SENSORDB("image_mirror = %d,flip = %d", image_mirror, image_flip);
    /********************************************************
       *
       *   0x3820[2] ISP Vertical flip
       *   0x3820[1] Sensor Vertical flip
       *
       *   0x3821[2] ISP Horizontal mirror
       *   0x3821[1] Sensor Horizontal mirror
       *
       *   ISP and Sensor flip or mirror register bit should be the same!!
       *
       ********************************************************/

    HV = image_mirror | (image_flip << 1);
    SENSORDB("image_mirror = %d,flip = %d HV = %d", image_mirror, image_flip, HV);
     switch (HV)
    {
        case IMAGE_NORMAL:
            OV2680KAI_write_cmos_sensor(0x3820,((OV2680KAI_read_cmos_sensor(0x3820) & 0xFB) | 0x00));
            OV2680KAI_write_cmos_sensor(0x3821,((OV2680KAI_read_cmos_sensor(0x3821) & 0xFB) | 0x00));
            break;
        case IMAGE_H_MIRROR:
            OV2680KAI_write_cmos_sensor(0x3820,((OV2680KAI_read_cmos_sensor(0x3820) & 0xFB) | 0x04));
            OV2680KAI_write_cmos_sensor(0x3821,((OV2680KAI_read_cmos_sensor(0x3821) & 0xFB) | 0x00));
            break;
        case IMAGE_V_MIRROR:
            OV2680KAI_write_cmos_sensor(0x3820,((OV2680KAI_read_cmos_sensor(0x3820) & 0xFB) | 0x00));
            OV2680KAI_write_cmos_sensor(0x3821,((OV2680KAI_read_cmos_sensor(0x3821) & 0xFB) | 0x04));
            break;
        case IMAGE_HV_MIRROR:
            OV2680KAI_write_cmos_sensor(0x3820,((OV2680KAI_read_cmos_sensor(0x3820) & 0xFB) | 0x04));
            OV2680KAI_write_cmos_sensor(0x3821,((OV2680KAI_read_cmos_sensor(0x3821) & 0xFB) | 0x04));
            break;
        default:
            SENSORDB("Error image_mirror setting");
    }

}

UINT32 OV2680KAIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
        UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
    UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
    UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
    UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
    UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
    UINT32 OV2680KAISensorRegNumber;
    UINT32 i;
    //PNVRAM_SENSOR_DATA_STRUCT pSensorDefaultData=(PNVRAM_SENSOR_DATA_STRUCT) pFeaturePara;
    //MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
    MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;
    //MSDK_SENSOR_GROUP_INFO_STRUCT *pSensorGroupInfo=(MSDK_SENSOR_GROUP_INFO_STRUCT *) pFeaturePara;
    //MSDK_SENSOR_ITEM_INFO_STRUCT *pSensorItemInfo=(MSDK_SENSOR_ITEM_INFO_STRUCT *) pFeaturePara;
    //MSDK_SENSOR_ENG_INFO_STRUCT    *pSensorEngInfo=(MSDK_SENSOR_ENG_INFO_STRUCT *) pFeaturePara;
    PSET_SENSOR_CALIBRATION_DATA_STRUCT pSetSensorCalData=(PSET_SENSOR_CALIBRATION_DATA_STRUCT)pFeaturePara;

#ifdef OV2680KAI_DRIVER_TRACE
    //SENSORDB("OV2680KAIFeatureControl£¬FeatureId:%d\n",FeatureId);
#endif
    switch (FeatureId)
    {
        case SENSOR_FEATURE_GET_RESOLUTION:
            *pFeatureReturnPara16++=OV2680KAI_IMAGE_SENSOR_FULL_WIDTH;
            *pFeatureReturnPara16=OV2680KAI_IMAGE_SENSOR_FULL_HEIGHT;
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_PERIOD:/* 3 */
            *pFeatureReturnPara16++= (OV2680KAI_sensor.line_length/OV2680KAI_MIPI_LANE_NUM);
            *pFeatureReturnPara16= OV2680KAI_sensor.frame_height;
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:  /* 3 */
            *pFeatureReturnPara32 = OV2680KAI_sensor.pclk;
            break;
        case SENSOR_FEATURE_SET_ESHUTTER:    /* 4 */
            set_OV2680KAI_shutter(*pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_NIGHTMODE:
            //OV2680KAI_night_mode((BOOL) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_GAIN:    /* 6 */
            OV2680KAI_SetGain((UINT16) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_FLASHLIGHT:
            break;
        case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
            break;
        case SENSOR_FEATURE_SET_REGISTER:
            OV2680KAI_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
            break;
        case SENSOR_FEATURE_GET_REGISTER:
            pSensorRegData->RegData = OV2680KAI_read_cmos_sensor(pSensorRegData->RegAddr);
            break;
        case SENSOR_FEATURE_SET_CCT_REGISTER:
            //memcpy(&OV2680KAI_sensor.eng.cct, pFeaturePara, sizeof(OV2680KAI_sensor.eng.cct));
            OV2680KAISensorRegNumber = OV2680KAI_FACTORY_END_ADDR;
            for (i=0;i<OV2680KAISensorRegNumber;i++)
            {
                spin_lock(&OV2680KAI_drv_lock);
                OV2680KAI_sensor.eng.cct[i].Addr=*pFeatureData32++;
                OV2680KAI_sensor.eng.cct[i].Para=*pFeatureData32++;
                spin_unlock(&OV2680KAI_drv_lock);
            }

            break;
        case SENSOR_FEATURE_GET_CCT_REGISTER:/* 12 */
            if (*pFeatureParaLen >= sizeof(OV2680KAI_sensor.eng.cct) + sizeof(kal_uint32))
            {
                *((kal_uint32 *)pFeaturePara++) = sizeof(OV2680KAI_sensor.eng.cct);
                memcpy(pFeaturePara, &OV2680KAI_sensor.eng.cct, sizeof(OV2680KAI_sensor.eng.cct));
            }
            break;
        case SENSOR_FEATURE_SET_ENG_REGISTER:
            //memcpy(&OV2680KAI_sensor.eng.reg, pFeaturePara, sizeof(OV2680KAI_sensor.eng.reg));
            OV2680KAISensorRegNumber = OV2680KAI_ENGINEER_END;
            for (i=0;i<OV2680KAISensorRegNumber;i++)
            {
                spin_lock(&OV2680KAI_drv_lock);
                OV2680KAI_sensor.eng.reg[i].Addr=*pFeatureData32++;
                OV2680KAI_sensor.eng.reg[i].Para=*pFeatureData32++;
                spin_unlock(&OV2680KAI_drv_lock);
            }
            break;
        case SENSOR_FEATURE_GET_ENG_REGISTER:    /* 14 */
            if (*pFeatureParaLen >= sizeof(OV2680KAI_sensor.eng.reg) + sizeof(kal_uint32))
            {
                *((kal_uint32 *)pFeaturePara++) = sizeof(OV2680KAI_sensor.eng.reg);
                memcpy(pFeaturePara, &OV2680KAI_sensor.eng.reg, sizeof(OV2680KAI_sensor.eng.reg));
            }
            break;
        case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
            ((PNVRAM_SENSOR_DATA_STRUCT)pFeaturePara)->Version = NVRAM_CAMERA_SENSOR_FILE_VERSION;
            ((PNVRAM_SENSOR_DATA_STRUCT)pFeaturePara)->SensorId = OV2680MIPI_SENSOR_ID_KAIMUJIN;
            memcpy(((PNVRAM_SENSOR_DATA_STRUCT)pFeaturePara)->SensorEngReg, &OV2680KAI_sensor.eng.reg, sizeof(OV2680KAI_sensor.eng.reg));
            memcpy(((PNVRAM_SENSOR_DATA_STRUCT)pFeaturePara)->SensorCCTReg, &OV2680KAI_sensor.eng.cct, sizeof(OV2680KAI_sensor.eng.cct));
            *pFeatureParaLen = sizeof(NVRAM_SENSOR_DATA_STRUCT);
            break;
        case SENSOR_FEATURE_GET_CONFIG_PARA:
            memcpy(pFeaturePara, &OV2680KAI_sensor.cfg_data, sizeof(OV2680KAI_sensor.cfg_data));
            *pFeatureParaLen = sizeof(OV2680KAI_sensor.cfg_data);
            break;
        case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
            OV2680KAI_camera_para_to_sensor();
            break;
        case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
            OV2680KAI_sensor_to_camera_para();
            break;
        case SENSOR_FEATURE_GET_GROUP_COUNT:
            OV2680KAI_get_sensor_group_count((kal_uint32 *)pFeaturePara);
            *pFeatureParaLen = 4;
            break;
        case SENSOR_FEATURE_GET_GROUP_INFO:
            OV2680KAI_get_sensor_group_info((MSDK_SENSOR_GROUP_INFO_STRUCT *)pFeaturePara);
            *pFeatureParaLen = sizeof(MSDK_SENSOR_GROUP_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_ITEM_INFO:
            OV2680KAI_get_sensor_item_info((MSDK_SENSOR_ITEM_INFO_STRUCT *)pFeaturePara);
            *pFeatureParaLen = sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_SET_ITEM_INFO:
            OV2680KAI_set_sensor_item_info((MSDK_SENSOR_ITEM_INFO_STRUCT *)pFeaturePara);
            *pFeatureParaLen = sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_ENG_INFO:
            memcpy(pFeaturePara, &OV2680KAI_sensor.eng_info, sizeof(OV2680KAI_sensor.eng_info));
            *pFeatureParaLen = sizeof(OV2680KAI_sensor.eng_info);
            break;
        case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
            // get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
            // if EEPROM does not exist in camera module.
            *pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_SET_VIDEO_MODE:
            OV2680KAISetVideoMode(*pFeatureData16);
            break;
        case SENSOR_FEATURE_CHECK_SENSOR_ID:
            OV2680KAIGetSensorID(pFeatureReturnPara32);
            break;
        case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
            OV2680KAISetAutoFlickerMode((BOOL)*pFeatureData16,*(pFeatureData16+1));
            break;
        case SENSOR_FEATURE_SET_CALIBRATION_DATA:
            OV2680KAISetCalData(pSetSensorCalData);
            break;
        case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
            OV2680KAISetMaxFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, *(pFeatureData32+1));
            break;
        case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
            OV2680KAIGetDefaultFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, (MUINT32 *)(*(pFeatureData32+1)));
            break;
        case SENSOR_FEATURE_SET_TEST_PATTERN:
            OV2680KAISetTestPatternMode((BOOL)*pFeatureData16);
            break;
        case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE://for factory mode auto testing
            *pFeatureReturnPara32=OV2680KAI_TEST_PATTERN_CHECKSUM;
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_SET_MIRROR_FLIP:
            //SENSORDB("[SENSOR_FEATURE_SET_MIRROR_FLIP]Mirror:%d, Flip:%d\n", *pFeatureData32,*(pFeatureData32+1));
            OV2680MIPI_Set_Mirror_Flip(*pFeatureData32, *(pFeatureData32+1));
            break;
        default:
            break;
    }
    return ERROR_NONE;
}/* OV2680KAIFeatureControl() */
SENSOR_FUNCTION_STRUCT SensorFuncOV2680KAI=
{
    OV2680KAIOpen,
    OV2680KAIGetInfo,
    OV2680KAIGetResolution,
    OV2680KAIFeatureControl,
    OV2680KAIControl,
    OV2680KAIClose
};

UINT32 OV2680KAIMIPISensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
//UINT32 OV2680KAISensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
/* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&SensorFuncOV2680KAI;

    return ERROR_NONE;
}/* SensorInit() */



