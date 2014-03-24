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
 *   DUMA
 *
 * Description:  
 * ------------
 *   Header file of Sensor driver
 *   
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
/* SENSOR FULL SIZE */
#ifndef __5EA_SENSOR_H
#define __5EA_SENSOR_H


#define S5K5EAYX_TEST_PATTERN_CHECKSUM (0x7ba87eae)
typedef enum S5K5EAYX_CAMCO_MODE
{
  S5K5EAYX_CAM_PREVIEW=0,//Camera Preview

  S5K5EAYX_CAM_CAPTURE,//Camera Capture

  S5K5EAYX_VIDEO_MPEG4,//Video Mode
  S5K5EAYX_VIDEO_MJPEG,

  S5K5EAYX_WEBCAM_CAPTURE,//WebCam

  S5K5EAYX_VIDEO_MAX
} S5K5EAYX_Camco_MODE;


struct S5K5EAYX_sensor_struct
{
        kal_uint16 sensor_id;

        kal_uint16 Dummy_Pixels;
        kal_uint16 Dummy_Lines;
        kal_uint32 Preview_PClk;
        kal_uint32 video_PClk;
        kal_uint32 capture_PClk;

        kal_uint32 Preview_Lines_In_Frame;
        kal_uint32 Capture_Lines_In_Frame;

        kal_uint32 Preview_Pixels_In_Line;
        kal_uint32 Capture_Pixels_In_Line;
        kal_uint16 Preview_Shutter;
        kal_uint16 Capture_Shutter;

        kal_uint16 StartX;
        kal_uint16 StartY;
        kal_uint16 iGrabWidth;
        kal_uint16 iGrabheight;

        kal_uint16 Capture_Size_Width;
        kal_uint16 Capture_Size_Height;
        kal_uint32 Digital_Zoom_Factor;

        kal_uint16 Max_Zoom_Factor;

        kal_uint32 Min_Frame_Rate;
        kal_uint32 Max_Frame_Rate;
        kal_uint32 Fixed_Frame_Rate;
        //kal_bool Night_Mode;
        S5K5EAYX_Camco_MODE Camco_mode;
        AE_FLICKER_MODE_T Banding;

        kal_bool Night_Mode;
};


#define S5K5EAYX_IMAGE_SENSOR_PV_WIDTH (1280-16)
#define S5K5EAYX_IMAGE_SENSOR_PV_HEIGHT (960-12)

#define S5K5EAYX_IMAGE_SENSOR_VIDEO_WIDTH (1280-16)      
#define S5K5EAYX_IMAGE_SENSOR_VIDEO_HEIGHT (960-12)       


#define S5K5EAYX_IMAGE_SENSOR_FULL_WIDTH   (2560-32)//view angle
#define S5K5EAYX_IMAGE_SENSOR_FULL_HEIGHT  (1920-24)


#define S5K5EAYX_PV_PERIOD_PIXEL_NUMS 1280
#define S5K5EAYX_PV_PERIOD_LINE_NUMS 960

#define S5K5EAYX_VIDEO_PERIOD_PIXEL_NUMS 1280
#define S5K5EAYX_VIDEO_PERIOD_LINE_NUMS 960

#define S5K5EAYX_CAP_PERIOD_PIXEL_NUMS 2560
#define S5K5EAYX_CAP_PERIOD_LINE_NUMS 1920


/* SENSOR START/END POSITION */
#define S5K5EAYX_PV_X_START                     4
#define S5K5EAYX_PV_Y_START                     4


#define S5K5EAYX_FULL_X_START                  8
#define S5K5EAYX_FULL_Y_START                  8


#define S5K5EAYX_PREVIEW_PCLK 156000000
#define S5K5EAYX_VIDEO_PCLK 156000000
#define S5K5EAYX_CAPTURE_PCLK 130000000


#define S5K5EAYX_WRITE_ID                     0xac
#define S5K5EAYX_READ_ID                      0xad


#define S5K5EAYX_SENSOR_ID 					0x5EA1

/* Flicker factor to calculate tha minimal shutter width step for 50Hz and 60Hz  */
#define MACRO_50HZ                                                      (100)
#define MACRO_60HZ                                                      (120)

#define FACTOR_50HZ                                                     (MACRO_50HZ * 1000)
#define FACTOR_60HZ                                                     (MACRO_60HZ * 1000)

#endif /* __SENSOR_H */
