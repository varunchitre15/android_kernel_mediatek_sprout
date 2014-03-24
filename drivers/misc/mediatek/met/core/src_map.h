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

#ifndef _SRC_MAP_H_
#define _SRC_MAP_H_

#include <linux/mm.h>

#define INVALID_COOKIE ~0UL
#define NO_COOKIE 0UL

//#define MET_DEBUG_MCOOKIE

#ifdef MET_DEBUG_MCOOKIE

#define dbg_mcookie_tprintk(fmt, a...) \
		trace_printk("[DEBUG_MCOOKIE]%s, %d: "fmt, \
			__FUNCTION__, __LINE__ , ##a)

#define dbg_mcookie_printk(fmt, a...) \
		printk("[DEBUG_MCOOKIE]%s, %d: "fmt, \
			__FUNCTION__, __LINE__ , ##a)

#else // MET_DEBUG_MCOOKIE

#define dbg_mcookie_tprintk(fmt, a...) \
		no_printk(fmt, ##a)

#define dbg_mcookie_printk(fmt, a...) \
		no_printk(fmt, ##a)

#endif // MET_DEBUG_MCOOKIE

struct mm_struct *take_tasks_mm(struct task_struct *task);
void release_mm(struct mm_struct *mm);
unsigned long lookup_dcookie(struct mm_struct *mm, unsigned long addr, off_t *offset);
int src_map_start(void);
void src_map_stop(void);
void src_map_stop_dcookie(void);

void mark_done(int cpu);

/* support met cookie */
unsigned int get_mcookie(const char *name);
int dump_mcookie(char *buf, unsigned int size);
int init_met_cookie(void);
int uninit_met_cookie(void);

#endif //_SRC_MAP_H_
