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
#include <linux/module.h>

#include "core/trace.h"


#define MS_EMI_FMT	"%5lu.%06lu"
#define MS_EMI_VAL	(unsigned long)(timestamp), nano_rem/1000
#define FMT15	",%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n"
#define VAL15	,value[0],value[1],value[2],value[3],value[4],value[5],value[6],value[7],value[8],value[9]\
                ,value[10],value[11],value[12],value[13],value[14]

void ms_emi(unsigned long long timestamp, unsigned char cnt, unsigned int *value)
{
	unsigned long nano_rem = do_div(timestamp, 1000000000);
	switch (cnt) {
	case 15: trace_printk(MS_EMI_FMT FMT15, MS_EMI_VAL VAL15); break;
	}
}

#define MS_SMI_FMT	"%5lu.%06lu"
#define MS_SMI_VAL	(unsigned long)(timestamp), nano_rem/1000
void ms_smi(unsigned long long timestamp, unsigned char cnt, unsigned int *value)
{
	unsigned long nano_rem = do_div(timestamp, 1000000000);
	switch (cnt) {
	case 18: trace_printk(MS_SMI_FMT FMT18, MS_SMI_VAL VAL18); break;
	case 30: trace_printk(MS_SMI_FMT FMT30, MS_SMI_VAL VAL30); break;
	case 34: trace_printk(MS_SMI_FMT FMT34, MS_SMI_VAL VAL34); break;
	case 37: trace_printk(MS_SMI_FMT FMT37, MS_SMI_VAL VAL37); break;
	case 44: trace_printk(MS_SMI_FMT FMT44, MS_SMI_VAL VAL44); break;
	case 50: trace_printk(MS_SMI_FMT FMT50, MS_SMI_VAL VAL50); break;
	case 66: trace_printk(MS_SMI_FMT FMT66, MS_SMI_VAL VAL66); break;
	case 82: trace_printk(MS_SMI_FMT FMT82, MS_SMI_VAL VAL82); break;
	}
}

void ms_smit(unsigned long long timestamp, unsigned char cnt, unsigned int *value)
{
	unsigned long nano_rem = do_div(timestamp, 1000000000);
	switch (cnt) {
	case 10: trace_printk(MS_SMI_FMT FMT10, MS_SMI_VAL VAL10); break;
	case 19: trace_printk(MS_SMI_FMT FMT19, MS_SMI_VAL VAL19); break;
	case 14: trace_printk(MS_SMI_FMT FMT14, MS_SMI_VAL VAL14); break;
	}
}

#define MS_TH_FMT	"%5lu.%06lu"
#define MS_TH_VAL	(unsigned long)(timestamp), nano_rem/1000
#define MS_TH_UD_FMT4	",%d,%d,%d,%d\n"
#define MS_TH_UD_VAL4	,value[0],value[1],value[2],value[3]
#define MS_TH_UD_FMT7	",%d,%d,%d,%d,%d,%d,%d\n"
#define MS_TH_UD_VAL7	,value[0],value[1],value[2],value[3],value[4],value[5],value[6]

void ms_th(unsigned long long timestamp, unsigned char cnt, unsigned int *value)
{
	unsigned long nano_rem = do_div(timestamp, 1000000000);
	switch (cnt) {
	case 4: trace_printk(MS_TH_FMT MS_TH_UD_FMT4, MS_TH_VAL MS_TH_UD_VAL4); break;
	case 7: trace_printk(MS_TH_FMT MS_TH_UD_FMT7, MS_TH_VAL MS_TH_UD_VAL7); break;
	}
}

#define MS_DRAMC_UD_FMT	"%x,%x,%x,%x\n"
#define MS_DRAMC_UD_VAL	value[0],value[1],value[2],value[3]
void ms_dramc(unsigned long long timestamp, unsigned char cnt, unsigned int *value)
{
	switch (cnt) {
	case 4: trace_printk(MS_DRAMC_UD_FMT, MS_DRAMC_UD_VAL); break;
	}
}