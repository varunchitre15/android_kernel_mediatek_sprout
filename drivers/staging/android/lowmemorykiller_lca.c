/* drivers/misc/lowmemorykiller.c
 *
 * The lowmemorykiller driver lets user-space specify a set of memory thresholds
 * where processes with a range of oom_score_adj values will get killed. Specify
 * the minimum oom_score_adj values in
 * /sys/module/lowmemorykiller/parameters/adj and the number of free pages in
 * /sys/module/lowmemorykiller/parameters/minfree. Both files take a comma
 * separated list of numbers in ascending order.
 *
 * For example, write "0,8" to /sys/module/lowmemorykiller/parameters/adj and
 * "1024,4096" to /sys/module/lowmemorykiller/parameters/minfree to kill
 * processes with a oom_score_adj value of 8 or higher when the free memory
 * drops below 4096 pages and kill processes with a oom_score_adj value of 0 or
 * higher when the free memory drops below 1024 pages.
 *
 * The driver considers memory used for caches to be free, but if a large
 * percentage of the cached memory is locked this can be very inaccurate
 * and processes may not get killed until the normal oom killer is triggered.
 *
 * Copyright (C) 2007-2008 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/oom.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/swap.h>
#include <linux/rcupdate.h>
#include <linux/sched.h>
#include <linux/notifier.h>

#if defined (CONFIG_MTK_AEE_FEATURE) && defined (CONFIG_MT_ENG_BUILD)
#include <linux/aee.h>
#include <linux/disp_assert_layer.h>
int lowmem_indicator = 1;
int in_lowmem = 0;
#endif

extern void show_free_areas_minimum(void);

#ifdef CONFIG_ZRAM
extern void mlog(int type);
#endif

static uint32_t lowmem_debug_level = 2;
#ifdef CONFIG_ANDROID_LOW_MEMORY_KILLER_AUTODETECT_OOM_ADJ_VALUES
#define CONVERT_ADJ(x) (x * 1000 / 17)
#define REVERT_ADJ(x) (x * 35 / 2000) // * 0.017 round to nearest
#else
#define CONVERT_ADJ(x) (x)
#define REVERT_ADJ(x) (x)
#endif // CONFIG_ANDROID_LOW_MEMORY_KILLER_AUTODETECT_OOM_ADJ_VALUES

#ifdef CONFIG_MT_ENG_BUILD
static uint32_t lowmem_debug_adj = CONVERT_ADJ(1);
#endif

#ifdef CONFIG_ZRAM
static int lowmem_adj[9] = {
	CONVERT_ADJ(0),
	CONVERT_ADJ(1),
	CONVERT_ADJ(2),
	CONVERT_ADJ(4),
	CONVERT_ADJ(6),
	CONVERT_ADJ(8),
	CONVERT_ADJ(9),
	CONVERT_ADJ(12),
	CONVERT_ADJ(15),
};
static int lowmem_adj_size = 9;
static int lowmem_minfree[9] = {
	4 * 256,	/*  0 ->  4MB */
	12 * 256,	/*  1 -> 12MB */
	16 * 256,	/*  2 -> 16MB */
	24 * 256,	/*  4 -> 24MB */
	28 * 256,	/*  6 -> 28MB */
	32 * 256,	/*  8 -> 32MB */
	36 * 256,	/*  9 -> 36MB */
	40 * 256,	/* 12 -> 40MB */
	48 * 256,	/* 15 -> 48MB */
};
static int lowmem_minfree_size = 9;
#else // CONFIG_ZRAM
static int lowmem_adj[6] = {
	CONVERT_ADJ(0),
	CONVERT_ADJ(1),
	CONVERT_ADJ(6),
	CONVERT_ADJ(12),
};
static int lowmem_adj_size = 4;
static int lowmem_minfree[6] = {
	3 * 512,	/* 6MB */
	2 * 1024,	/* 8MB */
	4 * 1024,	/* 16MB */
	16 * 1024,	/* 64MB */
};
static int lowmem_minfree_size = 4;
#endif // CONFIG_ZRAM

static struct task_struct *lowmem_deathpending;
static unsigned long lowmem_deathpending_timeout;

#define lowmem_print(level, x...)			\
	do {						\
		if (lowmem_debug_level >= (level))	\
			printk(x);			\
	} while (0)

static int
task_notify_func(struct notifier_block *self, unsigned long val, void *data);

static struct notifier_block task_nb = {
	.notifier_call  = task_notify_func,
};

static int
task_notify_func(struct notifier_block *self, unsigned long val, void *data)
{
	struct task_struct *task = data;

	if (task == lowmem_deathpending)
		lowmem_deathpending = NULL;

	return NOTIFY_DONE;
}

static int lowmem_shrink(struct shrinker *s, struct shrink_control *sc)
{
	struct task_struct *tsk;
	struct task_struct *selected = NULL;
	int rem = 0;
	int tasksize;
	int i;
	int min_score_adj = OOM_SCORE_ADJ_MAX + 1;
	int minfree = 0;
	int selected_tasksize = 0;
	int selected_oom_score_adj;
	int array_size = ARRAY_SIZE(lowmem_adj);
	int other_free = global_page_state(NR_FREE_PAGES) - totalreserve_pages;
	int other_file = global_page_state(NR_FILE_PAGES) -
						global_page_state(NR_SHMEM);
#ifdef CONFIG_MT_ENG_BUILD
        int print_extra_info = 0;
        static unsigned long lowmem_print_extra_info_timeout = 0;
#endif // CONFIG_MT_ENG_BUILD

#ifdef CONFIG_ZRAM
	int other_anon = global_page_state(NR_INACTIVE_ANON) - global_page_state(NR_ACTIVE_ANON);
        other_file -= total_swapcache_pages;
#endif
	/*
	 * If we already have a death outstanding, then
	 * bail out right away; indicating to vmscan
	 * that we have nothing further to offer on
	 * this pass.
	 *
	 */
	if (lowmem_deathpending &&
		time_before_eq(jiffies, lowmem_deathpending_timeout))
		return 0;

	if (lowmem_adj_size < array_size)
		array_size = lowmem_adj_size;
	if (lowmem_minfree_size < array_size)
		array_size = lowmem_minfree_size;

	for (i = 0; i < array_size; i++) {
		minfree = lowmem_minfree[i];
		if (other_free < minfree && other_file < minfree) {
			min_score_adj = lowmem_adj[i];
			break;
		}
	}
#ifdef CONFIG_ZRAM
	// For GB3 CR ALPS00602722: walkaround CTS issue
	if (min_score_adj < 9 && other_anon > 70 * 256) {
		// if other_anon > 70MB, don't kill adj <= 8
		min_score_adj = 9;
	}
#endif

	if (sc->nr_to_scan > 0)
		lowmem_print(3, "lowmem_shrink %lu, %x, ofree %d %d, ma %d\n",
				sc->nr_to_scan, sc->gfp_mask, other_free,
				other_file, min_score_adj);
	rem = global_page_state(NR_ACTIVE_ANON) +
		global_page_state(NR_ACTIVE_FILE) +
		global_page_state(NR_INACTIVE_ANON) +
		global_page_state(NR_INACTIVE_FILE);
	if (sc->nr_to_scan <= 0 || min_score_adj == OOM_SCORE_ADJ_MAX + 1) {
		lowmem_print(5, "lowmem_shrink %lu, %x, return %d\n",
			     sc->nr_to_scan, sc->gfp_mask, rem);
    /*
     * disable indication if low memory
     */
#if defined (CONFIG_MTK_AEE_FEATURE) && defined (CONFIG_MT_ENG_BUILD)
		if (in_lowmem) {
			in_lowmem = 0;
			lowmem_indicator = 1;
			DAL_LowMemoryOff();
		}
#endif
		return rem;
	}

	selected_oom_score_adj = min_score_adj;
	// add debug log
#ifdef CONFIG_MT_ENG_BUILD
	if (min_score_adj <= lowmem_debug_adj) {
		if (lowmem_print_extra_info_timeout == 0) {
			lowmem_print_extra_info_timeout = jiffies;
		}
                if (time_after_eq(jiffies, lowmem_print_extra_info_timeout)) {
                        lowmem_print_extra_info_timeout = jiffies + HZ;
                        print_extra_info = 1;
                }
        }
	if (print_extra_info) {
		lowmem_print(1, "======low memory killer=====\n");
		lowmem_print(1, "Free memory other_free: %d, other_file:%d pages\n", other_free, other_file);
	}		
#endif

	rcu_read_lock();
	for_each_process(tsk) {
		struct task_struct *p;
		int oom_score_adj;

		if (tsk->flags & PF_KTHREAD)
			continue;

		p = find_lock_task_mm(tsk);
		if (!p)
			continue;

		if (test_tsk_thread_flag(p, TIF_MEMDIE) &&
		    time_before_eq(jiffies, lowmem_deathpending_timeout)) {
			static pid_t last_dying_pid = 0;
			if (last_dying_pid != p->pid) {
				lowmem_print(1, "lowmem_shrink return directly, due to  %d (%s) is dying\n",
					p->pid, p->comm);
				last_dying_pid = p->pid;
			}
			task_unlock(p);
			rcu_read_unlock();
			return 0;
		}
#ifdef CONFIG_ANDROID_LOW_MEMORY_KILLER_AUTODETECT_OOM_ADJ_VALUES
		oom_score_adj = p->signal->oom_score_adj;
#else
		oom_score_adj = p->signal->oom_adj;
#endif
#ifdef CONFIG_MT_ENG_BUILD
		if (print_extra_info) {
#ifdef CONFIG_ZRAM
			lowmem_print(1, "Candidate %d (%s), oom_score_adj %d, oom_adj %d, rss %lu, rswap %lu, to kill\n",
				     p->pid, p->comm, oom_score_adj, REVERT_ADJ(oom_score_adj), get_mm_rss(p->mm),
				     get_mm_counter(p->mm, MM_SWAPENTS));
#else // CONFIG_ZRAM
			lowmem_print(1, "Candidate %d (%s), oom_score_adj %d, oom_adj %d, rss %lu, to kill\n",
				     p->pid, p->comm, oom_score_adj, REVERT_ADJ(oom_score_adj), get_mm_rss(p->mm));
#endif // CONFIG_ZRAM
                }
#endif // CONFIG_MT_ENG_BUILD
		if (oom_score_adj < min_score_adj) {
			task_unlock(p);
			continue;
		}
#ifdef CONFIG_ZRAM
		tasksize = get_mm_rss(p->mm) + get_mm_counter(p->mm, MM_SWAPENTS);
#else
		tasksize = get_mm_rss(p->mm);
#endif
		task_unlock(p);
		if (tasksize <= 0)
			continue;
		if (selected) {
			if (oom_score_adj < selected_oom_score_adj)
				continue;
			if (oom_score_adj == selected_oom_score_adj &&
			    tasksize <= selected_tasksize)
				continue;
		}
		selected = p;
		selected_tasksize = tasksize;
		selected_oom_score_adj = oom_score_adj;
		lowmem_print(4, "select '%s' (%d), oom_score_adj %d, oom_adj %d, size %d, to kill\n",
			     p->comm, p->pid, oom_score_adj, REVERT_ADJ(oom_score_adj), tasksize);
	}
	if (selected) {
		lowmem_print(1, "Killing '%s' (%d), oom_score_adj %d (oom_adj %d),\n" \
				"   to free %ldkB on behalf of '%s' (%d) because\n" \
				"   cache %ldkB is below limit %ldkB for oom_score_adj %d\n" \
				"   Free memory is %ldkB above reserved\n",
			     selected->comm, selected->pid,
			     selected_oom_score_adj,
			     REVERT_ADJ(selected_oom_score_adj),
			     selected_tasksize * (long)(PAGE_SIZE / 1024),
			     current->comm, current->pid,
			     other_file * (long)(PAGE_SIZE / 1024),
			     minfree * (long)(PAGE_SIZE / 1024),
			     min_score_adj,
			     other_free * (long)(PAGE_SIZE / 1024));
		lowmem_deathpending = selected;
		lowmem_deathpending_timeout = jiffies + HZ;
#ifdef CONFIG_MT_ENG_BUILD
		if (print_extra_info) {
		    lowmem_print(1, "low memory info:\n");
		    show_free_areas_minimum();
        }
#endif

        /*
		 * when kill adj=0 process trigger kernel warning, only in MTK internal eng load
		 */
#if defined (CONFIG_MTK_AEE_FEATURE) && defined (CONFIG_MT_ENG_BUILD) && \
    !defined (PARTIAL_BUILD) //mtk internal   
        	if (selected_oom_score_adj <= 0) { // can set 16 for test 
		            lowmem_print(1, "low memory trigger kernel warning\n");
				aee_kernel_warning_api("LMK", 0, DB_OPT_DEFAULT|DB_OPT_DUMPSYS_ACTIVITY|DB_OPT_LOW_MEMORY_KILLER,
						"Framework low memory", "please contact AP/AF memory module owner\n");
	        }
#endif
		/*
		 * show an indication if low memory
		 */
#if defined (CONFIG_MTK_AEE_FEATURE) && defined (CONFIG_MT_ENG_BUILD)
		if (lowmem_indicator && selected_oom_score_adj <= 1) {
			lowmem_print(5, "low memory: raise aee warning\n");
			in_lowmem = 1;
			lowmem_indicator = 0;
			DAL_LowMemoryOn();
			//aee_kernel_warning(module_name, lowmem_warning);
		}
#endif

#ifdef CONFIG_ZRAM
        mlog(1);
#endif
		send_sig(SIGKILL, selected, 0);
		set_tsk_thread_flag(selected, TIF_MEMDIE);
		rem -= selected_tasksize;
	}
	lowmem_print(4, "lowmem_shrink %lu, %x, return %d\n",
		     sc->nr_to_scan, sc->gfp_mask, rem);
	rcu_read_unlock();
	return rem;
}

static struct shrinker lowmem_shrinker = {
	.shrink = lowmem_shrink,
	.seeks = DEFAULT_SEEKS * 16
};

static int __init lowmem_init(void)
{
#ifdef CONFIG_ZRAM
	vm_swappiness = 100;
#endif
	task_free_register(&task_nb);
	register_shrinker(&lowmem_shrinker);
	return 0;
}

static void __exit lowmem_exit(void)
{
	unregister_shrinker(&lowmem_shrinker);
	task_free_unregister(&task_nb);
}

#ifdef CONFIG_ANDROID_LOW_MEMORY_KILLER_AUTODETECT_OOM_ADJ_VALUES
static int lowmem_oom_adj_to_oom_score_adj(int oom_adj)
{
	if (oom_adj == OOM_ADJUST_MAX)
		return OOM_SCORE_ADJ_MAX;
	else
		return (oom_adj * OOM_SCORE_ADJ_MAX) / -OOM_DISABLE;
}

static void lowmem_autodetect_oom_adj_values(void)
{
	int i;
	int oom_adj;
	int oom_score_adj;
	int array_size = ARRAY_SIZE(lowmem_adj);

	if (lowmem_adj_size < array_size)
		array_size = lowmem_adj_size;

	if (array_size <= 0)
		return;

	oom_adj = lowmem_adj[array_size - 1];
	if (oom_adj > OOM_ADJUST_MAX)
		return;

	oom_score_adj = lowmem_oom_adj_to_oom_score_adj(oom_adj);
	if (oom_score_adj <= OOM_ADJUST_MAX)
		return;

	lowmem_print(1, "lowmem_shrink: convert oom_adj to oom_score_adj:\n");
	for (i = 0; i < array_size; i++) {
		oom_adj = lowmem_adj[i];
		oom_score_adj = lowmem_oom_adj_to_oom_score_adj(oom_adj);
		lowmem_adj[i] = oom_score_adj;
		lowmem_print(1, "oom_adj %d => oom_score_adj %d\n",
			     oom_adj, oom_score_adj);
	}
}

static int lowmem_adj_array_set(const char *val, const struct kernel_param *kp)
{
	int ret;

	ret = param_array_ops.set(val, kp);

	/* HACK: Autodetect oom_adj values in lowmem_adj array */
	lowmem_autodetect_oom_adj_values();

	return ret;
}

static int lowmem_adj_array_get(char *buffer, const struct kernel_param *kp)
{
	return param_array_ops.get(buffer, kp);
}

static void lowmem_adj_array_free(void *arg)
{
	param_array_ops.free(arg);
}

static struct kernel_param_ops lowmem_adj_array_ops = {
	.set = lowmem_adj_array_set,
	.get = lowmem_adj_array_get,
	.free = lowmem_adj_array_free,
};

static const struct kparam_array __param_arr_adj = {
	.max = ARRAY_SIZE(lowmem_adj),
	.num = &lowmem_adj_size,
	.ops = &param_ops_int,
	.elemsize = sizeof(lowmem_adj[0]),
	.elem = lowmem_adj,
};
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
/* Called by routine in earlysuspend to recycle memory */
void kick_lmk_from_compaction(gfp_t gfp_mask)
{
	long recycled;
	struct shrink_control shrink = {
		.gfp_mask = gfp_mask,
		.nr_to_scan = 128,
	};
	int other_free = global_page_state(NR_FREE_PAGES) - totalreserve_pages;
	int other_file = global_page_state(NR_FILE_PAGES) - global_page_state(NR_SHMEM);
	int shift = max(other_free, other_file);
	int i = 0;

	printk("@@@@@@@@@@@@ [%s] @@@@@@@@@@@@\n",__FUNCTION__);

	/* Apply shift to lowmem_minfree (Exclude ADJ-0) */
	for (i = 1; i < lowmem_minfree_size; ++i) {
		lowmem_minfree[i] += shift; 
	}

	recycled = lowmem_shrink(&lowmem_shrinker, &shrink);
	
	/* Revert shift */
	for (i = 1; i < lowmem_minfree_size; ++i) {
		lowmem_minfree[i] -= shift; 
	}
}
#endif


/*
 * get_min_free_pages
 * returns the low memory killer watermark of the given pid,
 * When the system free memory is lower than the watermark, the LMK (low memory
 * killer) may try to kill processes.
 */
int get_min_free_pages(pid_t pid)
{
    struct task_struct *p = 0;
    int target_oom_adj = 0;
    int i = 0;
    int array_size = ARRAY_SIZE(lowmem_adj);

    if (lowmem_adj_size < array_size)
            array_size = lowmem_adj_size;
    if (lowmem_minfree_size < array_size)
            array_size = lowmem_minfree_size;

    for_each_process(p) {
        /* search pid */
        if (p->pid == pid) {
            task_lock(p);
            target_oom_adj = p->signal->oom_adj;
            task_unlock(p);
            /* get min_free value of the pid */
            for (i = array_size - 1; i >= 0; i--) {
                if (target_oom_adj >= lowmem_adj[i]) {
                    lowmem_print(3, KERN_INFO"pid: %d, target_oom_adj = %d, "
                            "lowmem_adj[%d] = %d, lowmem_minfree[%d] = %d\n",
                            pid, target_oom_adj, i, lowmem_adj[i], i,
                            lowmem_minfree[i]);
                    return lowmem_minfree[i];
                }
            }
            goto out; 
        }
    }

out:
    lowmem_print(3, KERN_ALERT"[%s]pid: %d, adj: %d, lowmem_minfree = 0\n", 
            __FUNCTION__, pid, p->signal->oom_adj);
    return 0;
}
EXPORT_SYMBOL(get_min_free_pages);

/* Query LMK minfree settings */
/* To query default value, you can input index with value -1. */
size_t query_lmk_minfree(int index)
{
	int which;

	/* Invalid input index, return default value */
	if (index < 0) {
		return lowmem_minfree[2];
	}
	
	/* Find a corresponding output */
	which = 5;
	do {
		if (lowmem_adj[which] <= index) {
			break;
		}
	} while (--which >= 0);

	/* Fix underflow bug */
	which = (which < 0)? 0 : which;

	return lowmem_minfree[which];
}
EXPORT_SYMBOL(query_lmk_minfree);

module_param_named(cost, lowmem_shrinker.seeks, int, S_IRUGO | S_IWUSR);
#ifdef CONFIG_ANDROID_LOW_MEMORY_KILLER_AUTODETECT_OOM_ADJ_VALUES
__module_param_call(MODULE_PARAM_PREFIX, adj,
		    &lowmem_adj_array_ops,
		    .arr = &__param_arr_adj,
		    S_IRUGO | S_IWUSR, -1);
__MODULE_PARM_TYPE(adj, "array of int");
#else
module_param_array_named(adj, lowmem_adj, int, &lowmem_adj_size,
			 S_IRUGO | S_IWUSR);
#endif
module_param_array_named(minfree, lowmem_minfree, uint, &lowmem_minfree_size,
			 S_IRUGO | S_IWUSR);
module_param_named(debug_level, lowmem_debug_level, uint, S_IRUGO | S_IWUSR);
#if defined (CONFIG_MTK_AEE_FEATURE) && defined (CONFIG_MT_ENG_BUILD)
module_param_named(lowmem_indicator, lowmem_indicator, uint, S_IRUGO | S_IWUSR);
#endif
#ifdef CONFIG_MT_ENG_BUILD
module_param_named(debug_adj, lowmem_debug_adj, uint, S_IRUGO | S_IWUSR);
#endif

module_init(lowmem_init);
module_exit(lowmem_exit);

MODULE_LICENSE("GPL");

