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

#ifndef _MTK_SCHED_H
#define _MTK_SCHED_H

#define _GNU_SOURCE
#include <linux/ioctl.h>
#include <sched.h>
#include <linux/mt_sched_ioctl.h>

#ifdef __cplusplus                
extern "C"{
#endif

int mt_sched_setaffinity(pid_t pid, size_t cpusetsize, cpu_set_t *mask);
int mt_sched_getaffinity(pid_t pid, size_t cpusetsize, cpu_set_t *mask, cpu_set_t *mt_mask);
int mt_sched_exitaffinity(pid_t pid);

#ifdef __cplusplus
}               
#endif

#endif
