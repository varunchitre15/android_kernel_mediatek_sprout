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
//#define TPD_TYPE_CAPACITIVE
//#define TPD_TYPE_RESISTIVE
//#define TPD_POWER_SOURCE         MT6575_POWER_VGP2
//#define TPD_I2C_NUMBER           0
//#define TPD_WAKEUP_TRIAL         60
//#define TPD_WAKEUP_DELAY         100

//#define TPD_DELAY                (2*HZ/100)
//#define TPD_RES_X                480
//#define TPD_RES_Y                800
#define TPD_CALIBRATION_MATRIX  {0,2458,0,-4096,-1638,3276800,0,0};

#define TPD_HAVE_CALIBRATION
//#define TPD_HAVE_BUTTON
//#define TPD_HAVE_TREMBLE_ELIMINATION

//#define TPD_NO_GPIO
//#define TPD_RESET_PIN_ADDR   (PERICFG_BASE + 0xC000)

#endif /* TOUCHPANEL_H__ */
