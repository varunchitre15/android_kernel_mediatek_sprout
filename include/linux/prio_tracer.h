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

#ifndef _PRIO_TRACER_H
#define _PRIO_TRACER_H

#include <linux/sched.h>

#define PTS_DEFAULT_PRIO (-101)

#define PTS_USER 0
#define PTS_KRNL 1
#define PTS_BNDR 2

extern void create_prio_tracer(pid_t tid);
extern void delete_prio_tracer(pid_t tid);

extern void update_prio_tracer(pid_t tid, int prio, int policy, int kernel);

extern void set_user_nice_syscall(struct task_struct *p, long nice);
extern void set_user_nice_binder(struct task_struct *p, long nice);
extern int sched_setscheduler_syscall(struct task_struct *, int,
				      const struct sched_param *);
extern int sched_setscheduler_nocheck_binder(struct task_struct *, int,
					     const struct sched_param *);
#endif
