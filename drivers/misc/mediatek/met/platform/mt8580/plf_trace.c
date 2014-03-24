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

#define MP_2P_FMT	"%5lu.%06lu"
#define MP_2P_VAL	(unsigned long)(timestamp), nano_rem/1000
void mp_2p(unsigned long long timestamp, unsigned char cnt, unsigned int *value)
{
	unsigned long nano_rem = do_div(timestamp, 1000000000);
	switch (cnt) {
	case 1: trace_printk(MP_2P_FMT FMT1, MP_2P_VAL VAL1); break;
	case 2: trace_printk(MP_2P_FMT FMT2, MP_2P_VAL VAL2); break;
	case 3: trace_printk(MP_2P_FMT FMT3, MP_2P_VAL VAL3); break;
	case 4: trace_printk(MP_2P_FMT FMT4, MP_2P_VAL VAL4); break;
	case 5: trace_printk(MP_2P_FMT FMT5, MP_2P_VAL VAL5); break;
	case 6: trace_printk(MP_2P_FMT FMT6, MP_2P_VAL VAL6); break;
	case 7: trace_printk(MP_2P_FMT FMT7, MP_2P_VAL VAL7); break;
	case 8: trace_printk(MP_2P_FMT FMT8, MP_2P_VAL VAL8); break;
	case 9: trace_printk(MP_2P_FMT FMT9, MP_2P_VAL VAL9); break;
	}
}

void mp_2pr(unsigned long long timestamp, unsigned char cnt, unsigned int *value)
{
	unsigned long nano_rem = do_div(timestamp, 1000000000);
	switch (cnt) {
	case 1: trace_printk(MP_2P_FMT FMT1, MP_2P_VAL VAL1); break;
	case 2: trace_printk(MP_2P_FMT FMT2, MP_2P_VAL VAL2); break;
	case 3: trace_printk(MP_2P_FMT FMT3, MP_2P_VAL VAL3); break;
	case 4: trace_printk(MP_2P_FMT FMT4, MP_2P_VAL VAL4); break;
	case 5: trace_printk(MP_2P_FMT FMT5, MP_2P_VAL VAL5); break;
	case 6: trace_printk(MP_2P_FMT FMT6, MP_2P_VAL VAL6); break;
	case 7: trace_printk(MP_2P_FMT FMT7, MP_2P_VAL VAL7); break;
	case 8: trace_printk(MP_2P_FMT FMT8, MP_2P_VAL VAL8); break;
	case 9: trace_printk(MP_2P_FMT FMT9, MP_2P_VAL VAL9); break;
	}
}

void mp_2pw(unsigned long long timestamp, unsigned char cnt, unsigned int *value)
{
	unsigned long nano_rem = do_div(timestamp, 1000000000);
	switch (cnt) {
	case 1: trace_printk(MP_2P_FMT FMT1, MP_2P_VAL VAL1); break;
	case 2: trace_printk(MP_2P_FMT FMT2, MP_2P_VAL VAL2); break;
	case 3: trace_printk(MP_2P_FMT FMT3, MP_2P_VAL VAL3); break;
	case 4: trace_printk(MP_2P_FMT FMT4, MP_2P_VAL VAL4); break;
	case 5: trace_printk(MP_2P_FMT FMT5, MP_2P_VAL VAL5); break;
	case 6: trace_printk(MP_2P_FMT FMT6, MP_2P_VAL VAL6); break;
	case 7: trace_printk(MP_2P_FMT FMT7, MP_2P_VAL VAL7); break;
	case 8: trace_printk(MP_2P_FMT FMT8, MP_2P_VAL VAL8); break;
	case 9: trace_printk(MP_2P_FMT FMT9, MP_2P_VAL VAL9); break;
	}
}

