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

#ifndef _TOUCHPANEL_H__
#define _TOUCHPANEL_H__

#include <mach/mt_boot.h>
#include <mach/mt_gpio.h>
/* Pre-defined definition */
#define TPD_TYPE_CAPACITIVE

#define TPD_POWER_SOURCE         MT65XX_POWER_LDO_VGP2
#define TPD_I2C_NUMBER           0
#define TPD_WAKEUP_TRIAL         60
#define TPD_WAKEUP_DELAY         100

#define TPD_RES_X                480			//根据LCD分辨率设置
#define TPD_RES_Y                800			//根据LCD分辨率设置

#define TPD_CALIBRATION_MATRIX  {-256, 0, 1310720, 0, 287, 0,0,0};

//#define TPD_HAVE_CALIBRATION

#define TPD_HAVE_BUTTON

#ifdef TPD_HAVE_BUTTON
//#define TPD_KEY_COUNT           2
//#define TPD_KEYS                {KEY_MENU,KEY_BACK}
//#define TPD_KEYS_DIM            {{40,850,60,60},{280,850,60,60}}										//根据LCD分辨率设置
#define TPD_KEY_COUNT           3
#define TPD_KEYS                    {KEY_MENU,KEY_HOMEPAGE,KEY_BACK}
#define TPD_KEYS_DIM            {{40,850,80,80},{200,850,80,80},{360,850,80,80}}		//根据LCD分辨率设置
#endif

//#define	TPD_XY_INVERT						//交换X和Y
//#define	TPD_X_INVERT						//X翻转(对称)
//#define	TPD_Y_INVERT						//Y翻转(对称)

//#define	TPD_RSTPIN_1V8						//RESET PIN为1.8V时，要打开这个
#define	GPIO_CTP_RST_PIN_M_GPIO		0			//RESET PIN的IO口编组

//#define TPD_CLOSE_POWER_IN_SLEEP				//是否关闭TP电源,一般不要打开
//#define TP_DEBUG  							//调试信息开关
#define TP_FIRMWARE_UPDATE						//T卡升级功能开关,一般都要打开
//#define TP_PROXIMITY_SENSOR					//贴脸熄屏功能开关,需要时可以打开


#define TPD_HOME_KEY_LONG_PRESS //add by lisong

#endif /* _TOUCHPANEL_H__ */
