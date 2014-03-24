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
#include <linux/device.h>
#include <linux/module.h>

#include "core/met_drv.h"
#include "core/trace.h"

#include "mt_typedefs.h"
#include "mt_reg_base.h"
#include "mt_emi_bm.h"
#include "sync_write.h"
#include "plf_trace.h"

#define DEF_BM_RW_TYPE      (BM_BOTH_READ_WRITE)
extern struct metdevice met_emi;
static struct kobject *kobj_emi = NULL;
static volatile int rwtype = BM_BOTH_READ_WRITE;

static ssize_t rwtype_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t rwtype_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t n);
static ssize_t emi_clock_rate_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t dcm_ctrl_show(struct kobject *kobj,
				struct kobj_attribute *attr,
				char *buf);
static ssize_t dcm_ctrl_store(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char *buf,
				size_t n);

static struct kobj_attribute rwtype_attr = __ATTR(rwtype, 0644, rwtype_show, rwtype_store);
static struct kobj_attribute emi_clock_rate_attr = __ATTR_RO(emi_clock_rate);
static struct kobj_attribute emi_dcm_ctrl_attr = __ATTR(emi_dcm_ctrl,
						0644,
						dcm_ctrl_show,
						dcm_ctrl_store);

extern unsigned int mt_get_emi_freq(void);
static ssize_t emi_clock_rate_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int i;
	i = snprintf(buf, PAGE_SIZE, "%d\n", mt_get_emi_freq());
	return i;
}

static ssize_t rwtype_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int i;
	i = snprintf(buf, PAGE_SIZE, "%d\n", rwtype);
	return i;
}

static ssize_t rwtype_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t n)
{
	int value;

	if ((n == 0) || (buf == NULL)) {
		return -EINVAL;
	}
	if (sscanf(buf, "%d", &value) != 1) {
		return -EINVAL;
	}
	if (value < 0 && value > BM_BOTH_READ_WRITE) {
		return -EINVAL;
	}
	rwtype = value;
	return n;
}

static ssize_t dcm_ctrl_show(struct kobject *kobj,
		struct kobj_attribute *attr,
		char *buf)
{
    return snprintf(buf, PAGE_SIZE, "EMI DCM is %s\n", BM_GetEmiDcm() ? "OFF" : "ON");
}

static ssize_t dcm_ctrl_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n)
{
    if (!strncmp(buf, "OFF", strlen("OFF"))) {
        BM_SetEmiDcm(0xff);
    } else if (!strncmp(buf, "ON", strlen("ON"))) {
        BM_SetEmiDcm(0x0);
    } else {
        printk("invalid event\n");
    }

    return n;
}

static void emi_init(void)
{
	BM_Init();
	/* init EMI bus monitor */
	//BM_SetReadWriteType(DEF_BM_RW_TYPE);
	BM_SetReadWriteType(rwtype);
	BM_SetMonitorCounter(1, BM_MASTER_MM, BM_TRANS_TYPE_4BEAT | BM_TRANS_TYPE_8Byte | BM_TRANS_TYPE_BURST_WRAP);
	BM_SetMonitorCounter(2, BM_MASTER_AP_MCU, BM_TRANS_TYPE_4BEAT | BM_TRANS_TYPE_8Byte | BM_TRANS_TYPE_BURST_WRAP);
	BM_SetMonitorCounter(3, BM_MASTER_MD_MCU | BM_MASTER_2G_3G_MDDMA, BM_TRANS_TYPE_4BEAT | BM_TRANS_TYPE_8Byte | BM_TRANS_TYPE_BURST_WRAP);
	BM_SetMonitorCounter(4, BM_MASTER_PERI, BM_TRANS_TYPE_4BEAT | BM_TRANS_TYPE_8Byte | BM_TRANS_TYPE_BURST_WRAP);

	BM_SetLatencyCounter();
}

static void emi_start(void)
{
	BM_Enable(1);
}

static void emi_stop(void)
{
	BM_Enable(0);
}

static int do_emi(void)
{
	return met_emi.mode;
}

static unsigned int emi_polling(unsigned int *emi_value)
{
	int i;
	int j=-1;

	BM_Pause();

	// read counter
	//Get Bus Cycle Count
	emi_value[++j] = BM_GetBusCycCount();
	//Get Word Count
	for (i=0; i<4; i++) {
		emi_value[++j] = BM_GetWordCount(i+1);
	}

	emi_value[++j] = BM_GetLatencyCycle(1);
	emi_value[++j] = BM_GetLatencyCycle(2);
	emi_value[++j] = BM_GetLatencyCycle(3);
	emi_value[++j] = BM_GetLatencyCycle(4);
	emi_value[++j] = BM_GetLatencyCycle(5);

	emi_value[++j] = BM_GetLatencyCycle(9);
	emi_value[++j] = BM_GetLatencyCycle(10);
	emi_value[++j] = BM_GetLatencyCycle(11);
	emi_value[++j] = BM_GetLatencyCycle(12);
	emi_value[++j] = BM_GetLatencyCycle(13);

#if 0
	mt6577_mon_log_buff[cur].BM_WSCT = BM_GetWordCount(1);	/*MM SUBSYS MEMORY*/
	mt6577_mon_log_buff[cur].BM_WSCT2 = BM_GetWordCount(2); /*APMCU MEMORY*/
	mt6577_mon_log_buff[cur].BM_WSCT3 = BM_GetWordCount(3); /*MD SUBSYS MEMORY*/
	mt6577_mon_log_buff[cur].BM_WSCT4 = BM_GetWordCount(4); /*PERIPHERAL SUBSYS*/
#endif
	//printk("[%d, %d, %d, %d, %d]\n", bcnt,
	//	wsctp[0], wsctp[1], wsctp[2], wsctp[3]);

	BM_Enable(0);
	BM_Enable(1);

	return j+1;
}

static void emi_uninit(void)
{
	BM_DeInit();
}

//static struct kobject *emi_parent;
static int met_emi_create(struct kobject *parent)
{
	int ret = 0;

	kobj_emi = parent;

	ret = sysfs_create_file(kobj_emi, &rwtype_attr.attr);
	if (ret != 0) {
		pr_err("Failed to create rwtype in sysfs\n");
		return ret;
	}
	ret = sysfs_create_file(kobj_emi, &emi_clock_rate_attr.attr);
	if (ret != 0) {
		pr_err("Failed to create emi_clock_rate in sysfs\n");
		return ret;
	}
	ret = sysfs_create_file(kobj_emi, &emi_dcm_ctrl_attr.attr);
	if (ret != 0) {
		pr_err("Failed to create emi_dcm_ctrl in sysfs\n");
		return ret;
	}
    return ret;
}


static void met_emi_delete(void)
{
	sysfs_remove_file(kobj_emi, &rwtype_attr.attr);
	sysfs_remove_file(kobj_emi, &emi_clock_rate_attr.attr);
	sysfs_remove_file(kobj_emi, &emi_dcm_ctrl_attr.attr);
	kobj_emi = NULL;
}

static void met_emi_start(void)
{
	if (do_emi()) {
		emi_init();
		emi_stop();
		emi_start();
	}
}

static void met_emi_stop(void)
{
	if (do_emi()) {
		emi_stop();
		emi_uninit();
	}

}

static void met_emi_polling(unsigned long long stamp, int cpu)
{

	unsigned char count;
	unsigned int emi_value[17];

	if (do_emi()) {
		count = emi_polling(emi_value);
		if (count) {
			ms_emi(stamp, count, emi_value);
		}
	}
}

static char help[] = "  --emi                                 monitor EMI banwidth\n";
static char header[] =
"met-info [000] 0.0: met_emi_clockrate: %d000\n"
"# ms_emi: timestamp,BUS_CYCLE,MM,APMCU,MD,PERIPHERAL,APMCU_LATENCY,MM_LATENCY,PERIPHERAL_LATENCY,MD_MCU_LATENCY,MD_2G3G_LATENCY,APMCU_TRANS,MM_TRANS,PERIPHERAL_TRANS,MD_MCU_TRANS,MD_2G3G_TRANS\n"
"met-info [000] 0.0: met_emi_header:BUS_CYCLE,MM,APMCU,MD,PERIPHERAL,APMCU_LATENCY,MM_LATENCY,PERIPHERAL_LATENCY,MD_MCU_LATENCY,MD_2G3G_LATENCY,APMCU_TRANS,MM_TRANS,PERIPHERAL_TRANS,MD_MCU_TRANS,MD_2G3G_TRANS\n";

static int emi_print_help(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, help);
}

static int emi_print_header(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, header, mt_get_emi_freq());
}

struct metdevice met_emi = {
	.name = "emi",
	.owner = THIS_MODULE,
	.type = MET_TYPE_BUS,
	.create_subfs = met_emi_create,
	.delete_subfs = met_emi_delete,
	.cpu_related = 0,
	.start = met_emi_start,
	.stop = met_emi_stop,
	.timed_polling = met_emi_polling,
	.print_help = emi_print_help,
	.print_header = emi_print_header,
};

