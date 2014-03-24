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

#ifndef __ETM_H
#define __ETM_H

struct etm_driver_data
{
	void __iomem *etm_regs;
	int is_ptm;
	const int *pwr_down;
};

struct etb_driver_data
{
	void __iomem *etb_regs;
	void __iomem *funnel_regs;
	void __iomem *tpiu_regs;
	void __iomem *dem_regs;
	int use_etr;
	u32 etr_len;
	u32 etr_virt;
	dma_addr_t etr_phys;
};

extern void trace_start_by_cpus(const struct cpumask *mask, int init_etb);

#endif
