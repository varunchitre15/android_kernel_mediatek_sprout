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

#ifndef __FM_ERR_H__
#define __FM_ERR_H__

#include <linux/kernel.h> //for printk()

#define FM_ERR_BASE 1000
typedef enum fm_drv_err_t {
    FM_EOK = FM_ERR_BASE,
    FM_EBUF,
    FM_EPARA,
    FM_ELINK,
    FM_ELOCK,
    FM_EFW,
    FM_ECRC,
    FM_EWRST, //wholechip reset
    FM_ESRST, //subsystem reset
    FM_EPATCH,
    FM_ENOMEM,
    FM_EINUSE, //other client is using this object
    FM_EMAX
} fm_drv_err_t;

#define FMR_ASSERT(a) { \
			if ((a) == NULL) { \
				printk("%s,invalid pointer\n", __func__);\
				return -FM_EPARA; \
			} \
		}

#endif //__FM_ERR_H__

