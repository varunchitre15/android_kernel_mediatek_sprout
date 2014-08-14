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

#ifndef TOUCHPANEL_H__
#define TOUCHPANEL_H__

/* Pre-defined definition */

/* Register */
#define FD_ADDR_MAX        0xE9
#define FD_ADDR_MIN        0xDD
#define FD_BYTE_COUNT     6

#define CUSTOM_MAX_WIDTH (480)
#define CUSTOM_MAX_HEIGHT (854)

#define TPD_UPDATE_FIRMWARE

//#define TPD_HAVE_BUTTON
#define TPD_BUTTON_HEIGH        (100)
#define TPD_KEY_COUNT           3
#define TPD_KEYS                { KEY_BACK, KEY_HOMEPAGE, KEY_MENU}
#define TPD_KEYS_DIM        {{80,854,100,TPD_BUTTON_HEIGH},\
                            {240,854,100,TPD_BUTTON_HEIGH},\
                            {400,854,100,TPD_BUTTON_HEIGH}}

#define TPD_POWER_SOURCE_CUSTOM         MT6323_POWER_LDO_VGP1//MT65XX_POWER_LDO_VGP1
#define LCD_X           480
#define LCD_Y           854

#define TPD_UPDATE_FIRMWARE
//#define HAVE_TOUCH_KEY


//#define TPD_HAVE_CALIBRATION
//#define TPD_CALIBRATION_MATRIX  {2680,0,0,0,2760,0,0,0};
//#define TPD_WARP_START
//#define TPD_WARP_END

//#define TPD_RESET_ISSUE_WORKAROUND
//#define TPD_MAX_RESET_COUNT 3
//#define TPD_WARP_Y(y) ( TPD_Y_RES - 1 - y )
//#define TPD_WARP_X(x) ( x )

#endif /* TOUCHPANEL_H__ */
