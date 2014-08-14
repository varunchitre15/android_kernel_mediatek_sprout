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
 *   ov5648mipi_Sensor_sunny.h
 *
 * Project:
 * --------
 *   ALPS
 *
 * Description:
 * ------------
 *   CMOS sensor header file
 *
 ****************************************************************************/
#ifndef _OV5648MIPITRULY_SENSOR_H
#define _OV5648MIPITRULY_SENSOR_H

#define OV5648MIPITRULY_FACTORY_START_ADDR 0
#define OV5648MIPITRULY_ENGINEER_START_ADDR 10

typedef enum OV5648MIPITRULY_group_enum
{
  OV5648MIPITRULY_PRE_GAIN = 0,
  OV5648MIPITRULY_CMMCLK_CURRENT,
  OV5648MIPITRULY_FRAME_RATE_LIMITATION,
  OV5648MIPITRULY_REGISTER_EDITOR,
  OV5648MIPITRULY_GROUP_TOTAL_NUMS
} OV5648MIPITRULY_FACTORY_GROUP_ENUM;

typedef enum OV5648MIPITRULY_register_index
{
  OV5648MIPITRULY_SENSOR_BASEGAIN = OV5648MIPITRULY_FACTORY_START_ADDR,
  OV5648MIPITRULY_PRE_GAIN_R_INDEX,
  OV5648MIPITRULY_PRE_GAIN_Gr_INDEX,
  OV5648MIPITRULY_PRE_GAIN_Gb_INDEX,
  OV5648MIPITRULY_PRE_GAIN_B_INDEX,
  OV5648MIPITRULY_FACTORY_END_ADDR
} OV5648MIPITRULY_FACTORY_REGISTER_INDEX;

typedef enum OV5648MIPITRULY_engineer_index
{
  OV5648MIPITRULY_CMMCLK_CURRENT_INDEX = OV5648MIPITRULY_ENGINEER_START_ADDR,
  OV5648MIPITRULY_ENGINEER_END
} OV5648MIPITRULY_FACTORY_ENGINEER_INDEX;

typedef struct _sensor_data_struct
{
  SENSOR_REG_STRUCT reg[OV5648MIPITRULY_ENGINEER_END];
  SENSOR_REG_STRUCT cct[OV5648MIPITRULY_FACTORY_END_ADDR];
} sensor_data_struct;


//#define OV5648MIPITRULY_USE_OTP


/* SENSOR PREVIEW/CAPTURE VT CLOCK */
#define OV5648MIPITRULY_PREVIEW_CLK                      84000000
#define OV5648MIPITRULY_CAPTURE_CLK                      84000000
#define OV5648MIPITRULY_VIDEO_CLK                        84000000


/* Data Format */
#define OV5648MIPITRULY_COLOR_FORMAT                     SENSOR_OUTPUT_FORMAT_RAW_B


#define OV5648MIPITRULY_MIN_ANALOG_GAIN                  1   /* 1x */
#define OV5648MIPITRULY_MAX_ANALOG_GAIN                  32  /* 32x */

#define OV5648MIPITRULY_FULL_START_X                    (4)
#define OV5648MIPITRULY_FULL_START_Y                    (8)
#define OV5648MIPITRULY_IMAGE_SENSOR_FULL_WIDTH         (2560)
#define OV5648MIPITRULY_IMAGE_SENSOR_FULL_HEIGHT        (1920)

#define OV5648MIPITRULY_PV_START_X                      (2)
#define OV5648MIPITRULY_PV_START_Y                      (2)
#define OV5648MIPITRULY_IMAGE_SENSOR_PV_WIDTH           (1280)
#define OV5648MIPITRULY_IMAGE_SENSOR_PV_HEIGHT          (960)

#define OV5648MIPITRULY_VIDEO_START_X                   (2)
#define OV5648MIPITRULY_VIDEO_START_Y                   (2)
#define OV5648MIPITRULY_IMAGE_SENSOR_VIDEO_WIDTH        (1280)
#define OV5648MIPITRULY_IMAGE_SENSOR_VIDEO_HEIGHT       (960)


/* SENSOR PIXEL/LINE NUMBERS IN ONE PERIOD */
#define OV5648MIPITRULY_FULL_PERIOD_PIXEL_NUMS          (2816) /* 15 fps */
#define OV5648MIPITRULY_FULL_PERIOD_LINE_NUMS           (1984)

#define OV5648MIPITRULY_PV_PERIOD_PIXEL_NUMS            (2816) /* 30 fps */
#define OV5648MIPITRULY_PV_PERIOD_LINE_NUMS             (992)

#define OV5648MIPITRULY_VIDEO_PERIOD_PIXEL_NUMS         (2816) /* 30 fps */
#define OV5648MIPITRULY_VIDEO_PERIOD_LINE_NUMS          (992)

/* SENSOR READ/WRITE ID */
#define OV5648MIPITRULY_WRITE_ID (0x6c)
#define OV5648MIPITRULY_READ_ID  (0x6d)

/* FRAME RATE UNIT */
#define OV5648MIPITRULY_FPS(x)                          (10 * (x))


//OTP Code Start
/*
#ifdef OV5648MIPITRULY_USE_OTP
struct OV5648MIPITRULY_otp_struct
{
    // Data
    kal_uint8 module_id_yy;
    kal_uint8 module_id_mm;
    kal_uint8 module_id_dd;

    kal_uint8 module_id_code;
    kal_uint8 module_id_vendor_code;
    kal_uint8 module_id_version;

    //wb
    kal_uint16 rg_ratio;
    kal_uint16 bg_ratio;
    kal_uint16 gb_gr_ratio;
};
#define RG_Typical 0x293
#define BG_Typical 0x2F4
#define GB_GR_Typical 0x3FC
#endif
*/
//OTP Code End



/* EXPORT FUNCTIONS */
UINT32 OV5648MIPITrulyOpen(void);
UINT32 OV5648MIPITrulyControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 OV5648MIPITrulyFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
UINT32 OV5648MIPITrulyGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 OV5648MIPITrulyGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 OV5648MIPITrulyClose(void);

#define Sleep(ms) mdelay(ms)

#endif
