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
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/mm.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <linux/hrtimer.h>

#include "core/met_drv.h"
#include "core/trace.h"

#include "thermal.h"
#include "plf_trace.h"

extern struct metdevice met_thermal;
struct delayed_work dwork;

static int do_thermal(void)
{
	static int do_thermal = -1;

	if (do_thermal != -1) {
		return do_thermal;
	}

	if (met_thermal.mode == 0) {
		do_thermal = 0;
	} else {
        do_thermal = met_thermal.mode;
    }
	return do_thermal;
}

#if NO_MTK_THERMAL_GET_TEMP == 0
static unsigned int get_thermal(unsigned int *value)
{
	int j = -1;

	/*
	int i;
	for (i=0; i<MTK_THERMAL_SENSOR_COUNT;i++) {
		value[++j]=mtk_thermal_get_temp(i);
	}
	*/
	value[++j]=mtk_thermal_get_temp(MTK_THERMAL_SENSOR_CPU); //CPU
	value[++j]=mtk_thermal_get_temp(MTK_THERMAL_SENSOR_ABB); //GPU
	value[++j]=mtk_thermal_get_temp(MTK_THERMAL_SENSOR_PMIC); //PMIC
	value[++j]=mtk_thermal_get_temp(MTK_THERMAL_SENSOR_BATTERY); //BATTERY

	return j+1;
}
#endif

static void wq_get_thermal(struct work_struct *work)
{
	unsigned char count=0;
	unsigned int thermal_value[MTK_THERMAL_SENSOR_COUNT];  //Note here

	int cpu;
	unsigned long long stamp;
	//return;
	cpu = smp_processor_id();
	if (do_thermal()) {
		stamp = cpu_clock(cpu);
#if NO_MTK_THERMAL_GET_TEMP == 0
		count = get_thermal(thermal_value);
#endif
		if (count) {
			ms_th(stamp, count, thermal_value);
		}
	}

}

static void thermal_start(void)
{
	INIT_DELAYED_WORK(&dwork, wq_get_thermal);
	return;
}

static void thermal_stop(void)
{
	cancel_delayed_work_sync(&dwork);
	return;
}

static void thermal_polling(unsigned long long stamp, int cpu)
{
	schedule_delayed_work(&dwork, 0);
}

static char header[] =
"met-info [000] 0.0: ms_ud_sys_header: ms_th,timestamp,cpu,gpu,pmic,battery,d,d,d,d\n";
static char help[] = "  --thermal                             monitor thermal\n";

static int thermal_print_help(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, help);
}

static int thermal_print_header(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, header);
}

/*
static int thermal_process_argument(const char *arg, int len)
{
	printk("Thermal Argument(l=%d):%s\n", len, arg);
	return 0;
}
*/

struct metdevice met_thermal = {
	.name = "thermal",
	.owner = THIS_MODULE,
	.type = MET_TYPE_BUS,
	.cpu_related = 0,
	.start = thermal_start,
	.stop = thermal_stop,
	.polling_interval = 1000,//ms
	.timed_polling = thermal_polling,
	.tagged_polling = thermal_polling,
	.print_help = thermal_print_help,
	.print_header = thermal_print_header,
//	.process_argument = thermal_process_argument
};
