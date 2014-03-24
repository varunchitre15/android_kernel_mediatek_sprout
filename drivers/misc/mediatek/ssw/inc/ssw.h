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

#ifndef __SSW_H__
#define __SSW_H__
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/delay.h>

#include <linux/kdev_t.h>
#include <linux/ctype.h>
#include <linux/cdev.h>
#include <linux/string.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include <mach/mt_typedefs.h>
#include <mach/mtk_ccci_helper.h>
#include <mach/mt_reg_base.h>
#include <mach/mt_gpio.h>


/*-------------------------debug log define--------------------------------*/
static int dbg_en = 1;
#define SSW_DBG(format, args...) do{ \
	if(dbg_en) \
	{\
		printk(KERN_ERR "[SSW] "format,##args);\
	}\
}while(0)


/*-------------------------variable define----------------------------------*/
#if 0
#ifndef SSW_DUAL_TALK
#define SSW_DUAL_TALK 0
#endif

#ifndef SSW_SING_TALK
#define SSW_SING_TALK 1
#endif
#endif

/*------------------------Error Code---------------------------------------*/
#define SSW_SUCCESS 			(0)
#define SSW_INVALID_PARA		(-1)

enum {
	SSW_INVALID = 0xFFFFFFFF,
	SSW_INTERN = 0,
	SSW_EXT_FXLA2203 = 1,
};

#endif


