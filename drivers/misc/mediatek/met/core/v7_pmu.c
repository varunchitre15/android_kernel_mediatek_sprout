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
#include <linux/slab.h>

#include "trace.h"
#include "v7_pmu.h"
#include "v7_pmu_hw.h"
#include "v7_pmu_name.h"
#include "met_drv.h"

struct metdevice met_armv7pmu;

static int nr_counters = 0;

static struct kobject *kobj_cpu = NULL;
static struct met_pmu *pmu = NULL;
static int nr_arg=0;

struct chip_pmu {
	enum ARM_TYPE type;
	struct pmu_desc *desc;
	unsigned int count;
};

static struct chip_pmu chips[] ={
	{ CORTEX_A7, a7_pmu_desc, A7_PMU_DESC_COUNT },
	{ CORTEX_A9, a9_pmu_desc, A9_PMU_DESC_COUNT },
	{ CORTEX_A12, a7_pmu_desc, A7_PMU_DESC_COUNT },
	{ CORTEX_A15, a7_pmu_desc, A7_PMU_DESC_COUNT },
};
static struct chip_pmu chip_unknown = { CHIP_UNKNOWN, NULL, 0 };

#define CHIP_PMU_COUNT (sizeof(chips) / sizeof(struct chip_pmu))

static struct chip_pmu *chip;

static inline struct met_pmu *lookup_pmu(struct kobject *kobj)
{
	int i;
	for (i=0; i<nr_counters; i++) {
		if (pmu[i].kobj_cpu_pmu == kobj) {
			return &pmu[i];
		}
	}
	return NULL;
}

static ssize_t count_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", nr_counters-1);
}

static ssize_t count_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t n)
{
	return -EINVAL;
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
	int i;
	struct met_pmu *p = lookup_pmu(kobj);
	unsigned short event;

	if (p != NULL) {
		if (sscanf(buf, "0x%hx", &event) != 1) {
			return -EINVAL;
		}

		if (p == &(pmu[nr_counters-1])) { // cycle counter
			if (event != 0xff) {
				return -EINVAL;
			}
		} else {
			for (i=0; i<chip->count; i++) {
				if (chip->desc[i].event == event) {
					break;
				}
			}
			if (i == chip->count) {
				return -EINVAL;
			}
		}

		p->event = event;
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
		if (mode <= 2) {
			p->mode = mode;
			if(mode>0)
				met_armv7pmu.mode = 1;
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

static struct kobj_attribute count_attr = __ATTR(count, 0644, count_show, count_store);
static struct kobj_attribute event_attr = __ATTR(event, 0644, event_show, event_store);
static struct kobj_attribute mode_attr = __ATTR(mode, 0644, mode_show, mode_store);
static struct kobj_attribute freq_attr = __ATTR(freq, 0644, freq_show, freq_store);

static int v7_pmu_create_subfs(struct kobject *parent)
{
	int ret = 0;
	int i;
	char buf[16];
	enum ARM_TYPE type;

	type = armv7_get_ic();
	for (i=0; i<CHIP_PMU_COUNT; i++) {
		if (chips[i].type == type) {
			chip = &(chips[i]);
			break;
		}
	}
	if (i == CHIP_PMU_COUNT) {
		chip = &chip_unknown;
	}

	nr_counters = armv7_pmu_hw_get_counters() + 1;
	pmu = (struct met_pmu *) kzalloc(sizeof(struct met_pmu) * nr_counters, GFP_KERNEL);
	if (pmu == NULL) {
		pr_err("can not create kobject: kobj_cpu\n");
		ret = -ENOMEM;
		goto out;
	}

	kobj_cpu = parent;

	ret = sysfs_create_file(kobj_cpu, &count_attr.attr);
	if (ret != 0) {
		pr_err("Failed to create count in sysfs\n");
		goto out;
	}

	for (i=0; i<nr_counters; i++) {
		snprintf(buf, sizeof(buf), "%d", i);
		pmu[i].kobj_cpu_pmu = kobject_create_and_add(buf, kobj_cpu);

		ret = sysfs_create_file(pmu[i].kobj_cpu_pmu, &event_attr.attr);
		if (ret != 0) {
			pr_err("Failed to create event in sysfs\n");
			goto out;
		}

		ret = sysfs_create_file(pmu[i].kobj_cpu_pmu, &mode_attr.attr);
		if (ret != 0) {
			pr_err("Failed to create mode in sysfs\n");
			goto out;
		}

		ret = sysfs_create_file(pmu[i].kobj_cpu_pmu, &freq_attr.attr);
		if (ret != 0) {
			pr_err("Failed to create freq in sysfs\n");
			goto out;
		}
	}

out:
	if (ret != 0) {
		if (pmu != NULL) {
			kfree(pmu);
			pmu = NULL;
		}
	}
	return ret;
}

static void v7_pmu_delete_subfs(void)
{
	int i;

	if (kobj_cpu != NULL) {
		for (i=0; i<nr_counters; i++) {
			sysfs_remove_file(pmu[i].kobj_cpu_pmu, &event_attr.attr);
			sysfs_remove_file(pmu[i].kobj_cpu_pmu, &mode_attr.attr);
			sysfs_remove_file(pmu[i].kobj_cpu_pmu, &freq_attr.attr);
			kobject_del(pmu[i].kobj_cpu_pmu);
		}
		sysfs_remove_file(kobj_cpu, &count_attr.attr);
		kobj_cpu = NULL;
	}

	if (pmu != NULL) {
		kfree(pmu);
		pmu = NULL;
	}
}


extern unsigned int ctrl_flags;
static void v7_pmu_start(void)
{
	nr_arg = 0;

	if (ctrl_flags & 0x1)
		mp_cp_ptr = &mp_cp;
	else
		mp_cp_ptr = &mp_cp2;

	armv7_pmu_hw_start(pmu, nr_counters);
}

static void v7_pmu_stop(void)
{
	armv7_pmu_hw_stop(nr_counters);
}

unsigned int v7_pmu_polling(unsigned int *pmu_value)
{
	return armv7_pmu_hw_polling(pmu, nr_counters, pmu_value);
}


static char header[] =
"# mp_cp: timestamp, task_id, pc, cookie, offset, pmu_value1, ...\n"
"met-info [000] 0.0: met_cp_header: ";

static char help[] =
"  --pmu-cpu-evt=EVENT                   select CPU-PMU events. in Cortex-A9,\n"
"                                        you can enable at most \"%d general purpose events\"\n"
"                                        plus \"one special 0xff (CPU_CYCLE) event\"\n";

static int v7_pmu_print_help(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, help, nr_counters-1);
}

static int v7_pmu_print_header(char *buf, int len)
{
	int i, ret, rlen;

	ret = snprintf(buf, PAGE_SIZE, header);
	for (i=0; i<nr_counters; i++) {
		if (i < (nr_counters - 1))
			rlen = snprintf(buf + ret, 16, "0x%x,", pmu[i].event);
		else
			rlen = snprintf(buf + ret, 16, "0x%x\n", pmu[i].event);

		ret += rlen;
	}
	return ret;
}

/*
 * "met-cmd --start --pmu_cpu_evt=0x3"
 */

static int v7_pmu_process_argument(const char *arg, int len)
{
	int i;
	unsigned int value;
	extern int met_parse_num(const char *str, unsigned int *value, int len);

	if (met_parse_num(arg, &value, len)<0)
		return -EINVAL;

	if (value > 0xff)
		goto arg_out;

	if (value == 0xff) {
		pmu[nr_counters-1].mode = MODE_POLLING;
		pmu[nr_counters-1].event = 0xff;
		pmu[nr_counters-1].freq = 0;
	}
	else {
		if (nr_arg >= (nr_counters - 1))
			goto arg_out;
		pmu[nr_arg].mode = MODE_POLLING;
		pmu[nr_arg].event = value;
		pmu[nr_arg].freq = 0;
		nr_arg++;
	}

	met_armv7pmu.mode = 1;
	return 0;

arg_out:
	met_armv7pmu.mode = 0;
	nr_arg = 0;
	for (i=0; i<nr_counters; i++) {

	}
	return -EINVAL;
}

struct metdevice met_armv7pmu = {
	.name = "cpu",
	.type = MET_TYPE_PMU,
	.cpu_related = 1,
	.create_subfs = v7_pmu_create_subfs,
	.delete_subfs = v7_pmu_delete_subfs,
	.start = v7_pmu_start,
	.stop = v7_pmu_stop,
	.print_help = v7_pmu_print_help,
	.print_header = v7_pmu_print_header,
	.process_argument = v7_pmu_process_argument
};
