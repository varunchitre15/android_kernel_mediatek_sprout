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

#ifndef _MT_POWER_GS_H
#define _MT_POWER_GS_H

#include <linux/module.h>
#include <linux/proc_fs.h>

/*****************
* extern variable 
******************/
extern struct proc_dir_entry *mt_power_gs_dir;

/*****************
* extern function 
******************/
extern void mt_power_gs_compare(char *scenario, \
                                unsigned int *mt6582_power_gs, unsigned int mt6582_power_gs_len, \
                                unsigned int *mt6323_power_gs, unsigned int mt6323_power_gs_len, \
                                unsigned int *mt6333_power_gs, unsigned int mt6333_power_gs_len);

extern void mt_power_gs_dump_dpidle(void);
extern void mt_power_gs_dump_idle(void);

#endif
