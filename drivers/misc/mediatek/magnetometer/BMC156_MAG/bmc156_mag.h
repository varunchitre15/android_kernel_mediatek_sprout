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

/*
 * Definitions for bmm050 magnetic sensor chip.
 */
#ifndef __BMM050_H__
#define __BMM050_H__

#include <linux/ioctl.h>

#define CALIBRATION_DATA_SIZE    12

/* 7-bit addr:
*    0x12 (SDO connected to GND, CSB2 connected to VDDIO)
*    0x13 (SDO connected to VDDIO, CSB2 connected to VDDIO)
*/
#define BMM050_I2C_ADDR        0x12

// conversion of magnetic data (for bmm050) to uT units
// conversion of magnetic data to uT units
// 32768 = 1Guass = 100 uT
// 100 / 32768 = 25 / 8096
// 65536 = 360Degree
// 360 / 65536 = 45 / 8192


#define CONVERT_M            1
#define CONVERT_M_DIV        4
#define CONVERT_O            1
#define CONVERT_O_DIV        71        //(C_PI_F32X * AXIS_RESOLUTION_FACTOR / 180)
#define CONVERT_G            1
#define CONVERT_G_DIV        938
#define CONVERT_VRV            1
#define CONVERT_VRV_DIV    (0x40000000)
#define CONVERT_VLA_DIV    16384
#define CONVERT_VG_DIV 16384

#endif /* __BMM050_H__ */

