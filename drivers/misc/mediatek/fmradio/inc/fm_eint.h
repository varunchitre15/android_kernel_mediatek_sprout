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
#ifndef __FM_EINT_H__
#define __FM_EINT_H__

#include "fm_typedef.h"

enum{
	FM_EINT_PIN_EINT_MODE,
	FM_EINT_PIN_GPIO_MODE,
	FM_EINT_PIN_MAX_MODE
};

extern fm_s32 fm_enable_eint(void);
extern fm_s32 fm_disable_eint(void);
extern fm_s32 fm_request_eint(void (*parser)(void));  
extern fm_s32 fm_eint_pin_cfg(fm_s32 mode);

#endif //__FM_EINT_H__

