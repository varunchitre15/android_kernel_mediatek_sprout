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

struct mt_proc_struct{
    int pid;
    int tgid;
    int index;
    u64 cputime;
    u64 cputime_init;
    u64 prof_start;
    u64 prof_end;
	u64 cost_cputime;
	u32 cputime_percen_6;
	u64 isr_time;
	int   isr_count;
	struct mtk_isr_info  *mtk_isr;

    cputime_t utime_init;
    cputime_t utime;
    cputime_t stime_init;
    cputime_t stime;
    char comm[TASK_COMM_LEN];
    struct mt_proc_struct * next;
};

struct mt_cpu_info{
	unsigned long long cpu_idletime_start;
	unsigned long long cpu_idletime_end;
	unsigned long long cpu_iowait_start;
	unsigned long long cpu_iowait_end;		
};
