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

#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <asm/irq_regs.h>
#include <linux/types.h>

struct sample {
	struct list_head list;
	struct task_struct *task;
	unsigned long long stamp;
	unsigned long pc;
	int cpu;
	unsigned int pmu_value[7];
	unsigned char count;

	unsigned long cookie;
	off_t off;
};

void add_sample(struct pt_regs *regs, int cpu);
void sync_samples(int cpu);
void sync_all_samples(void);

//#define MET_DEBUG_COOKIE
#ifdef MET_DEBUG_COOKIE
#define dbg_cookie_tprintk(fmt, a...) \
		trace_printk("[DEBUG_COOKIE]%s, %d: "fmt, \
			__FUNCTION__, __LINE__ , ##a)
#else // MET_DEBUG_COOKIE
#define dbg_cookie_tprintk(fmt, a...) \
		no_printk(fmt, ##a)
#endif // MET_DEBUG_COOKIE

#endif // _BUFFER_H_
