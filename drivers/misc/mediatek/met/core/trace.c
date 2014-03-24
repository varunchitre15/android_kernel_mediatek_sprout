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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include "trace.h"

#define MP_CP_FMT	"%5lu.%06lu,%d,0x%lx,0x%lx,0x%lx"
#define MP_CP_VAL	(unsigned long)(timestamp), nano_rem/1000, \
	task->pid, program_counter, dcookie, offset
void mp_cp(unsigned long long timestamp,
	       struct task_struct *task,
	       unsigned long program_counter,
	       unsigned long dcookie,
	       unsigned long offset,
	       unsigned char cnt, unsigned int *value)
{
	unsigned long nano_rem = do_div(timestamp, 1000000000);
	switch (cnt) {
	case 1: trace_printk(MP_CP_FMT FMT1, MP_CP_VAL VAL1); break;
	case 2: trace_printk(MP_CP_FMT FMT2, MP_CP_VAL VAL2); break;
	case 3: trace_printk(MP_CP_FMT FMT3, MP_CP_VAL VAL3); break;
	case 4: trace_printk(MP_CP_FMT FMT4, MP_CP_VAL VAL4); break;
	case 5: trace_printk(MP_CP_FMT FMT5, MP_CP_VAL VAL5); break;
	case 6: trace_printk(MP_CP_FMT FMT6, MP_CP_VAL VAL6); break;
	case 7: trace_printk(MP_CP_FMT FMT7, MP_CP_VAL VAL7); break;
	case 8: trace_printk(MP_CP_FMT FMT8, MP_CP_VAL VAL8); break;
	case 9: trace_printk(MP_CP_FMT FMT9, MP_CP_VAL VAL9); break;
	}
}

#define MP_CP_FMT2	"%5lu.%06lu,%d,%s,0x%lx,0x%lx,0x%lx"
#define MP_CP_VAL2	(unsigned long)(timestamp), nano_rem/1000, \
	task->pid, task->comm, program_counter, dcookie, offset
void mp_cp2(unsigned long long timestamp,
	       struct task_struct *task,
	       unsigned long program_counter,
	       unsigned long dcookie,
	       unsigned long offset,
	       unsigned char cnt, unsigned int *value)
{
	unsigned long nano_rem = do_div(timestamp, 1000000000);
	switch (cnt) {
	case 1: trace_printk(MP_CP_FMT2 FMT1, MP_CP_VAL2 VAL1); break;
	case 2: trace_printk(MP_CP_FMT2 FMT2, MP_CP_VAL2 VAL2); break;
	case 3: trace_printk(MP_CP_FMT2 FMT3, MP_CP_VAL2 VAL3); break;
	case 4: trace_printk(MP_CP_FMT2 FMT4, MP_CP_VAL2 VAL4); break;
	case 5: trace_printk(MP_CP_FMT2 FMT5, MP_CP_VAL2 VAL5); break;
	case 6: trace_printk(MP_CP_FMT2 FMT6, MP_CP_VAL2 VAL6); break;
	case 7: trace_printk(MP_CP_FMT2 FMT7, MP_CP_VAL2 VAL7); break;
	case 8: trace_printk(MP_CP_FMT2 FMT8, MP_CP_VAL2 VAL8); break;
	case 9: trace_printk(MP_CP_FMT2 FMT9, MP_CP_VAL2 VAL9); break;
	}
}

void cpu_frequency(unsigned int frequency, unsigned int cpu_id)
{
	trace_printk("state=%d cpu_id=%d\n", frequency, cpu_id);
}

void (*mp_cp_ptr)(unsigned long long timestamp,
	       struct task_struct *task,
	       unsigned long program_counter,
	       unsigned long dcookie,
	       unsigned long offset,
	       unsigned char cnt, unsigned int *value) = &mp_cp;
