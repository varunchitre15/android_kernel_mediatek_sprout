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
 *   sensor.h
 *
 * Project:
 * --------
 *   ALPS
 *
 * Description:
 * ------------
 *   Header file of Sensor driver
 *
 *
 * Author:
 * -------
 *   Anyuan Huang (MTK70663)
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Revision:$
 * $Modtime:$
 * $Log:$
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
/* SENSOR FULL SIZE */
#ifndef __SENSOR_H
#define __SENSOR_H

#define HI253_WRITE_ID        0x40

#define HI253_GRAB_START_X    (1)
#define HI253_GRAB_START_Y    (1)
#define HI253_PV_WIDTH        (800 - 8)
#define HI253_PV_HEIGHT       (600 - 6)
#define HI253_FULL_WIDTH      (1600 - 16)
#define HI253_FULL_HEIGHT     (1200 - 12)

/* Sesnor Pixel/Line Numbers in One Period */  
#define HI253_PV_PERIOD_PIXEL_NUMS      (824)    /* Default preview line length */
#define HI253_PV_PERIOD_LINE_NUMS       (632)     /* Default preview frame length */
#define HI253_FULL_PERIOD_PIXEL_NUMS    (1640)    /* Default full size line length */
#define HI253_FULL_PERIOD_LINE_NUMS     (1248)    /* Default full size frame length */

/* Sensor Exposure Line Limitation */
#define HI253_PV_EXPOSURE_LIMITATION        (0x750)
#define HI253_FULL_EXPOSURE_LIMITATION      (0xfa0)

#define HI253_FRAME_RATE_UNIT         10
#define HI253_FPS(x)                  (HI253_FRAME_RATE_UNIT * (x))
#define HI253_MAX_FPS                 (HI253_FRAME_RATE_UNIT * 30)

UINT32 HI253Open(void);
UINT32 HI253GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 HI253GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 HI253Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 HI253FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
UINT32 HI253Close(void);
#endif /* __SENSOR_H */
