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

#include <linux/cpufreq.h>

#include "power.h"
#include "trace.h"

void force_power_log(int cpu)
{
	struct cpufreq_policy *p;

	if (cpu == POWER_LOG_ALL) {
		for_each_possible_cpu(cpu) {
			p = cpufreq_cpu_get(cpu);
			if (p != NULL) {
				cpu_frequency(p->cur, cpu);
				cpufreq_cpu_put(p);
			} else {
				cpu_frequency(0, cpu);
			}
		}
	} else {
		p = cpufreq_cpu_get(cpu);
		if (p != NULL) {
			cpu_frequency(p->cur, cpu);
			cpufreq_cpu_put(p);
		} else {
			cpu_frequency(0, cpu);
		}
	}
}
