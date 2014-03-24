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

#include <asm/page.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/string.h>

#include "core/met_drv.h"
#include "pl310_pmu.h"
#include "pl310_pmu_hw.h"
#include "plf_trace.h"

#define COUNTERS 2

static struct kobject *kobj_cpu = NULL;
static struct met_pmu pmu[COUNTERS];
static int toggle_mode = 0;

static inline struct met_pmu *lookup_pmu(struct kobject *kobj)
{
	int i;
	for (i=0; i<COUNTERS; i++) {
		if (pmu[i].kobj_cpu_pmu == kobj) {
			return &pmu[i];
		}
	}
	return NULL;
}

static ssize_t event_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct met_pmu *p = lookup_pmu(kobj);
	if (p != NULL) {
		return snprintf(buf, PAGE_SIZE, "0x%hx\n", p->event);
	}
	return -EINVAL;
}

static ssize_t event_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t n)
{
	struct met_pmu *p = lookup_pmu(kobj);
	if (p != NULL) {
		if (sscanf(buf, "0x%hx", &(p->event)) != 1) {
			return -EINVAL;
		}
		return n;
	}
	return -EINVAL;
}

static ssize_t mode_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct met_pmu *p = lookup_pmu(kobj);
	if (p != NULL) {
		switch (p->mode) {
		case 0:
			return snprintf(buf, PAGE_SIZE, "%hhd (disabled)\n", p->mode);
		case 1:
			return snprintf(buf, PAGE_SIZE, "%hhd (interrupt)\n", p->mode);
		case 2:
			return snprintf(buf, PAGE_SIZE, "%hhd (polling)\n", p->mode);
		}

	}
	return -EINVAL;
}

static ssize_t mode_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t n)
{
	unsigned char mode;
	struct met_pmu *p = lookup_pmu(kobj);
	if (p != NULL) {
		if (sscanf(buf, "%hhd", &mode) != 1) {
			return -EINVAL;
		}
		if ((mode == 0) || (mode == 2)) { // do not support interrupt mode
			p->mode = mode;
			return n;
		}
	}
	return -EINVAL;
}

static ssize_t freq_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct met_pmu *p = lookup_pmu(kobj);
	if (p != NULL) {
		return snprintf(buf, PAGE_SIZE, "%ld\n", p->freq);
	}
	return -EINVAL;
}

static ssize_t freq_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t n)
{
	struct met_pmu *p = lookup_pmu(kobj);
	if (p != NULL) {
		if (sscanf(buf, "%ld", &(p->freq)) != 1) {
			return -EINVAL;
		}
		return n;
	}
	return -EINVAL;
}

static ssize_t toggle_mode_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", toggle_mode);
}

static ssize_t toggle_mode_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t n)
{
	int t;
	if (sscanf(buf, "%d", &t) != 1) {
		return -EINVAL;
	}
	if ((t == 0) || (t == 1)) {
		toggle_mode = t;
		return n;
	}
	return -EINVAL;
}


static struct kobj_attribute event_attr = __ATTR(event, 0644, event_show, event_store);
static struct kobj_attribute mode_attr = __ATTR(mode, 0644, mode_show, mode_store);
static struct kobj_attribute freq_attr = __ATTR(freq, 0644, freq_show, freq_store);

static struct kobj_attribute toggle_mode_attr = __ATTR(toggle_mode, 0644, toggle_mode_show, toggle_mode_store);

static int met_pl310_create(struct kobject *parent)
{
	int ret = 0;
	int i;
	char buf[16];

	memset(pmu, 0, sizeof(struct met_pmu) * COUNTERS);
	kobj_cpu = parent;

	ret = sysfs_create_file(kobj_cpu, &toggle_mode_attr.attr);
	if (ret != 0) {
		pr_err("Failed to create toggle_mode in sysfs\n");
		return ret;
	}


	for (i=0; i<COUNTERS; i++) {
		snprintf(buf, sizeof(buf), "%d", i);
		pmu[i].kobj_cpu_pmu = kobject_create_and_add(buf, kobj_cpu);

		ret = sysfs_create_file(pmu[i].kobj_cpu_pmu, &event_attr.attr);
		if (ret != 0) {
			pr_err("Failed to create event in sysfs\n");
		}

		ret = sysfs_create_file(pmu[i].kobj_cpu_pmu, &mode_attr.attr);
		if (ret != 0) {
			pr_err("Failed to create mode in sysfs\n");
		}

		ret = sysfs_create_file(pmu[i].kobj_cpu_pmu, &freq_attr.attr);
		if (ret != 0) {
			pr_err("Failed to create freq in sysfs\n");
		}
	}

	return ret;
}

static void met_pl310_delete(void)
{
	int i;

	if (kobj_cpu != NULL) {
		for (i=0; i<COUNTERS; i++) {
			sysfs_remove_file(pmu[i].kobj_cpu_pmu, &event_attr.attr);
			sysfs_remove_file(pmu[i].kobj_cpu_pmu, &mode_attr.attr);
			sysfs_remove_file(pmu[i].kobj_cpu_pmu, &freq_attr.attr);
			kobject_del(pmu[i].kobj_cpu_pmu);
		}

		sysfs_remove_file(kobj_cpu, &toggle_mode_attr.attr);
		kobj_cpu = NULL;
	}
}

static unsigned int do_pl310(void)
{
	static int do_pl310 = -1;
	int i;

	if (do_pl310 != -1) {
		return do_pl310;
	}

	if (toggle_mode != 0) {
		do_pl310 = 2;
		return do_pl310;
	}

	do_pl310 = 0;
	for (i=0; i<COUNTERS; i++) {
		if (pmu[i].mode == MODE_POLLING) {
			do_pl310++;
		}
	}

	return do_pl310;
}

static void pl310_pmu_start(void)
{
	if (do_pl310()) {
		pl310_pmu_hw_start(pmu, COUNTERS, toggle_mode);
	}
}

static void pl310_pmu_stop(void)
{
	if (do_pl310()) {
		pl310_pmu_hw_stop(COUNTERS);
	}
}

static void pl310_pmu_polling(unsigned long long stamp, int cpu)
{
	unsigned char count;
	unsigned int l2_pmu_value[2];

	if (do_pl310()) {
		count = pl310_pmu_hw_polling(pmu, COUNTERS, l2_pmu_value);
		if (count) {
			if (count == 3) {
				mp_2pr(stamp, 2, l2_pmu_value);
			} else if (count == 5) {
				mp_2pw(stamp, 2, l2_pmu_value);
			} else {
				mp_2p(stamp, count, l2_pmu_value);
			}
		}
	}
}

struct metdevice met_pl310 = {
	.name = "l2",
	.owner = THIS_MODULE,
	.type = MET_TYPE_PMU,
	.create_subfs = met_pl310_create,
	.delete_subfs = met_pl310_delete,
	.cpu_related = 0,
	.start = pl310_pmu_start,
	.stop = pl310_pmu_stop,
	.timed_polling = pl310_pmu_polling,
	.tagged_polling = pl310_pmu_polling
};
