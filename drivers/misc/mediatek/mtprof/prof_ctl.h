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

#include <linux/sched.h>
#include "prof_main.h"
extern struct mt_proc_struct *mt_proc_head;
extern int proc_count;
extern int mtsched_enabled;
extern unsigned long long prof_start_ts, prof_end_ts, prof_dur_ts;

extern struct mt_cpu_info* mt_cpu_info_head;
extern int mt_cpu_num;

extern void mt_task_times(struct task_struct *p, cputime_t *ut, cputime_t *st);

extern unsigned long long mtprof_get_cpu_idle(int cpu);
extern unsigned long long mtprof_get_cpu_iowait(int cpu);

extern void save_mtproc_info(struct task_struct *p, unsigned long long ts);
extern void end_mtproc_info(struct task_struct *p);
extern void start_record_task(void);
extern void stop_record_task(void);
extern void reset_record_task(void);
extern void mt_cputime_switch(int on);
extern void mt_memprof_switch(int on);
