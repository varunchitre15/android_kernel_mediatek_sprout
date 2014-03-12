/* 
 * (C) Copyright 2014
 * MediaTek <www.MediaTek.com>
 * Run <Run.Liu@MediaTek.com>
 *
 * FM Radio Driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

