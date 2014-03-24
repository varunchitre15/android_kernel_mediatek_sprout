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

#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/cpu.h>
#include <linux/module.h>

#include "buffer.h"
#include "src_map.h"
#include "met_struct.h"
#include "v7_pmu.h"
#include "trace.h"

#ifdef CONFIG_CPU_FREQ
#include "power.h"

extern volatile int do_dvfs;
#endif

void add_sample(struct pt_regs *regs, int cpu)
{
	struct met_cpu_struct *met_cpu_ptr;
	struct sample *s;
	unsigned long flags;

	met_cpu_ptr = &per_cpu(met_cpu, cpu);

	s = (struct sample *) kmem_cache_alloc(met_cpu_ptr->cachep, GFP_ATOMIC);
	if (s == NULL) {
		printk("WHY?\n");
	} else {
		s->cpu = cpu;
		s->stamp = cpu_clock(cpu);
		s->task = current;
		s->count = v7_pmu_polling(s->pmu_value);

		if (regs) {
			s->pc = profile_pc(regs);
			dbg_cookie_tprintk("%s\n",
					!user_mode(regs) ?
					"in Kernel mode" : "in User mode");
		} else {
			s->pc = 0x0;
		}
		dbg_cookie_tprintk("pid is %d, "
				"tgid is %d, "
				"comm is %s\n",
				s->task->pid,
				s->task->tgid,
				s->task->comm);
		dbg_cookie_tprintk("pc is 0x%x\n", (unsigned int)s->pc);

		INIT_LIST_HEAD(&s->list);
	}

	spin_lock_irqsave(&met_cpu_ptr->list_lock, flags);
	list_add_tail(&s->list, &met_cpu_ptr->sample_head);
	spin_unlock_irqrestore(&met_cpu_ptr->list_lock, flags);
}

void cpu_sync_sample(void *data)
{
	struct sample *s = (struct sample *)data;
	mp_cp_ptr(s->stamp, s->task, s->pc, s->cookie, s->off, s->count, s->pmu_value);
}

void sync_samples(int cpu)
{
	struct met_cpu_struct *met_cpu_ptr;
	unsigned long flags;
	struct sample *s, *next;
	struct list_head head;
	struct mm_struct *mm;
	struct module *mod;

#ifdef CONFIG_CPU_FREQ
	static int force_power_num = 0;

	if (do_dvfs != 0) {
		if (cpu == 0) {
			force_power_num++;
			if (force_power_num == 50) {
				force_power_log(POWER_LOG_ALL);
				force_power_num = 0;
			}
		}
	}
#endif

	met_cpu_ptr = &per_cpu(met_cpu, cpu);

	INIT_LIST_HEAD(&head);

	mutex_lock(&met_cpu_ptr->list_sync_lock);

	spin_lock_irqsave(&met_cpu_ptr->list_lock, flags);
	list_splice_init(&met_cpu_ptr->sample_head, &head);
	spin_unlock_irqrestore(&met_cpu_ptr->list_lock, flags);

	list_for_each_entry_safe(s, next, &head, list) {
		list_del(&s->list);

		if (s->pc) {
			mm = take_tasks_mm(s->task);
			if (mm != NULL) {
				dbg_cookie_tprintk("Get dcookie\n");

				s->cookie = lookup_dcookie(mm, s->pc, &(s->off));
				release_mm(mm);
			} else {
				dbg_cookie_tprintk("Can't find dcookie\n");

				mod = __module_address(s->pc);
				if (mod) {
					s->cookie = get_mcookie(mod->name);
					s->off = s->pc - (unsigned long)mod->module_core;

					dbg_cookie_tprintk("mod->name is %s, pc is 0x%x\n",
							mod->name, (unsigned int)s->pc);
					dbg_cookie_tprintk("s->cookie is 0x%x\n",
						(unsigned int)s->cookie);
					dbg_cookie_tprintk("s->off is 0x%x\n",
						(unsigned int)s->off);
				} else {
					s->cookie = INVALID_COOKIE;
					s->off = 0;
				}
			}
		} else {
			dbg_cookie_tprintk("Shouldn't happen\n");

			s->cookie = NO_COOKIE;
			s->off = 0;
		}
		dbg_cookie_tprintk("pid is %d, "
				"tgid is %d, "
				"comm is %s\n",
				s->task->pid,
				s->task->tgid,
				s->task->comm);
		dbg_cookie_tprintk("pc is 0x%x\n", (unsigned int)s->pc);

		smp_call_function_single(cpu, cpu_sync_sample, s, 1);
		kmem_cache_free(met_cpu_ptr->cachep, s);
	}
	mutex_unlock(&met_cpu_ptr->list_sync_lock);

	mark_done(cpu);
}

void sync_all_samples(void)
{
	int cpu;
	for_each_possible_cpu(cpu) {
		sync_samples(cpu);
	}
}
