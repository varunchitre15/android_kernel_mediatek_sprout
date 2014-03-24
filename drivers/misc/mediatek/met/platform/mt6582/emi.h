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

#ifndef _EMI_H_
#define _EMI_H_

#include <linux/device.h>

#if 0
int emi_reg(struct device *this_device);
void emi_unreg(struct device *this_device);

void emi_init(void);
void emi_uninit(void);

void emi_start(void);
void emi_stop(void);

int do_emi(void);
unsigned int emi_polling(unsigned int *emi_value);
#endif
#endif // _EMI_H_
