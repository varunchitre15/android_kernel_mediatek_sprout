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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/random.h>
#include <linux/fs.h>

#include "core/met_drv.h"
#include "core/trace.h"

extern int ptp_opp_num(void);
extern void ptp_opp_freq(unsigned int *freq);
extern void ptp_opp_status(unsigned int *temp, unsigned int *volt);

extern struct metdevice met_ptpod;
static struct delayed_work dwork;

#define MAX_OPP_COUNT 10
static int opp_num;
unsigned int opp_freq[MAX_OPP_COUNT];

static char help[] = "  --ptpod                               Meature PMIC buck currents\n";

/*
 * Called from "met-cmd --start"
 */
static void ptpod_start(void)
{
	cancel_delayed_work_sync(&dwork); // jobs are triggered by ptpod_polling
	return;
}

/*
 * Called from "met-cmd --stop"
 */
static void ptpod_stop(void)
{
	cancel_delayed_work_sync(&dwork);
	return;
}

#define PTPOD_FMT	"%5lu.%06lu,%d"
#define PTPOD_VAL	(unsigned long)(stamp), nano_rem/1000, temp
void ptpod(struct work_struct *work)
{
	unsigned long long stamp;
	unsigned long nano_rem;
	unsigned int temp;
	unsigned int value[MAX_OPP_COUNT];

	stamp = cpu_clock(smp_processor_id());
	nano_rem = do_div(stamp, 1000000000);

	ptp_opp_status(&temp, value);
	switch (opp_num) {
	case 1: trace_printk(PTPOD_FMT FMT1, PTPOD_VAL VAL1); break;
	case 2: trace_printk(PTPOD_FMT FMT2, PTPOD_VAL VAL2); break;
	case 3: trace_printk(PTPOD_FMT FMT3, PTPOD_VAL VAL3); break;
	case 4: trace_printk(PTPOD_FMT FMT4, PTPOD_VAL VAL4); break;
	case 5: trace_printk(PTPOD_FMT FMT5, PTPOD_VAL VAL5); break;
	case 6: trace_printk(PTPOD_FMT FMT6, PTPOD_VAL VAL6); break;
	case 7: trace_printk(PTPOD_FMT FMT7, PTPOD_VAL VAL7); break;
	case 8: trace_printk(PTPOD_FMT FMT8, PTPOD_VAL VAL8); break;
	case 9: trace_printk(PTPOD_FMT FMT9, PTPOD_VAL VAL9); break;
	default: break;
	}
}

static void ptpod_polling(unsigned long long stamp, int cpu)
{
	schedule_delayed_work(&dwork, 0);
}

/*
 * Called from "met-cmd --help"
 */
static int ptpod_print_help(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, help);
	return 0;
}

static char header[] = "met-info [000] 0.0: ms_ud_sys_header: ptpod,timestamp,temp";

/*
 * It will be called back when run "met-cmd --extract" and mode is 1
 */
static int ptpod_print_header(char *buf, int len)
{
	int i, size, total_size;

	cancel_delayed_work_sync(&dwork);
	met_ptpod.mode = 0;

	size = snprintf(buf, PAGE_SIZE, header);
	total_size = size;
	buf += size;

	for (i=0; i<opp_num; i++) {
		size = snprintf(buf, PAGE_SIZE, ",%d", opp_freq[i]);
		total_size += size;
		buf += size;
	}

	// temperature
	size = snprintf(buf, PAGE_SIZE, ",d");
	total_size += size;
	buf += size;

	// volt
	for (i=0; i<opp_num; i++) {
		size = snprintf(buf, PAGE_SIZE, ",x");
		total_size += size;
		buf += size;
	}
	size = snprintf(buf, PAGE_SIZE, "\n");
	total_size += size;

	return total_size;
}

/*
 * "met-cmd --start --ptpod"
 */
static int ptpod_process_argument(const char *arg, int len)
{
	cancel_delayed_work_sync(&dwork);
	met_ptpod.mode = 1;
	return 0;
}

static int ptpod_create_subfs(struct kobject *parent)
{
	opp_num = ptp_opp_num();
	if ((opp_num <=0) || (opp_num > MAX_OPP_COUNT)) {
		return -1;
	}
	ptp_opp_freq(opp_freq);

	INIT_DELAYED_WORK(&dwork, ptpod);
	return 0;
}

static void ptpod_delete_subfs(void)
{
	cancel_delayed_work_sync(&dwork);
}

struct metdevice met_ptpod = {
	.name = "ptpod",
	.owner = THIS_MODULE,
	.type = MET_TYPE_BUS,
	.cpu_related = 0,
	.create_subfs = ptpod_create_subfs,
	.delete_subfs = ptpod_delete_subfs,
	.start = ptpod_start,
	.stop = ptpod_stop,
	.polling_interval = 1000, // In "ms"
	.timed_polling = ptpod_polling,
	.tagged_polling = ptpod_polling,
	.print_help = ptpod_print_help,
	.print_header = ptpod_print_header,
	.process_argument = ptpod_process_argument
};

EXPORT_SYMBOL(met_ptpod);
