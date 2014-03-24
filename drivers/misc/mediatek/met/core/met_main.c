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

#include <linux/module.h>
#include <linux/hrtimer.h>
#include <linux/cpu.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/profile.h>
#include <linux/dcache.h>
#include <linux/types.h>
#include <linux/dcookies.h>
#include <linux/sched.h>
#include <linux/fs.h>

#include <asm/irq_regs.h>

#include "buffer.h"
#include "met_struct.h"
#include "met_drv.h"

DEFINE_PER_CPU(struct met_cpu_struct, met_cpu);
//struct met_cpu_struct met_sys;

int fs_reg(void);
void fs_unreg(void);

static int __init met_pmu_init(void)
{
	int cpu;
	struct met_cpu_struct *met_cpu_ptr;

	for_each_possible_cpu(cpu) {
		met_cpu_ptr = &per_cpu(met_cpu, cpu);

		snprintf(&(met_cpu_ptr->name[0]), sizeof(met_cpu_ptr->name), "met%02d", cpu);
		met_cpu_ptr->cachep =
			kmem_cache_create(met_cpu_ptr->name, sizeof(struct sample), __alignof__(struct sample), 0, NULL);

		spin_lock_init(&met_cpu_ptr->list_lock);
		mutex_init(&met_cpu_ptr->list_sync_lock);
		INIT_LIST_HEAD(&met_cpu_ptr->sample_head);
		met_cpu_ptr->cpu = cpu;
	}

	fs_reg();
	return 0;
}

static void __exit met_pmu_exit(void)
{
	int cpu;

	fs_unreg();
	for_each_possible_cpu(cpu) {
		kmem_cache_destroy(per_cpu(met_cpu, cpu).cachep);
	}
}

module_init(met_pmu_init);
module_exit(met_pmu_exit);

MODULE_AUTHOR("DT_DM5");
MODULE_DESCRIPTION("MET_CORE");
MODULE_LICENSE("GPL");
