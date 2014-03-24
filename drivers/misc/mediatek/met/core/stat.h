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

#ifndef _STAT_H_
#define _STAT_H_

#include <linux/device.h>

int stat_reg(struct kobject *parent);
void stat_unreg(void);

void stat_start(void);
void stat_stop(void);
void stat_polling(unsigned long long stamp, int cpu);

#endif // _STAT_H_
