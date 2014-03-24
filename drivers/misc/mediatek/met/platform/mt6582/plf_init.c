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

#include "core/met_drv.h"

static const char strTopology[] = "LITTLE:0,1,2,3";

extern struct metdevice met_emi;
extern struct metdevice met_smi;
extern struct metdevice met_dramc;
extern struct metdevice met_thermal;
extern struct metdevice met_pmic;
extern struct metdevice met_ptpod;
#if NO_SPM_TWAM_REGISTER_HANDLER == 0
extern struct metdevice met_spmtwam;
#endif

#ifndef NO_MET_SET_TOPOLOGY
#define NO_MET_SET_TOPOLOGY 1
#endif

int met_reg_ext(void);
int met_dereg_ext(void);

static int __init met_plf_init(void)
{
	met_register(&met_emi);
	met_register(&met_smi);
	met_register(&met_dramc);
	met_register(&met_thermal);
	met_reg_ext();
	met_register(&met_pmic);
	met_register(&met_ptpod);
#if NO_SPM_TWAM_REGISTER_HANDLER == 0
	met_register(&met_spmtwam);
#endif
	met_set_platform("mt6582", 1);
#if NO_MET_SET_TOPOLOGY == 0
	met_set_topology(strTopology, 1);
#endif
	return 0;
}

static void __exit met_plf_exit(void)
{
	met_dereg_ext();
	met_deregister(&met_emi);
	met_deregister(&met_smi);
	met_deregister(&met_dramc);
	met_deregister(&met_thermal);
	met_deregister(&met_pmic);
	met_deregister(&met_ptpod);
#if NO_SPM_TWAM_REGISTER_HANDLER == 0
	met_deregister(&met_spmtwam);
#endif
	met_set_platform(NULL, 0);
#if NO_MET_SET_TOPOLOGY == 0
	met_set_topology(NULL, 0);
#endif
}

module_init(met_plf_init);
module_exit(met_plf_exit);
MODULE_AUTHOR("DT_DM5");
MODULE_DESCRIPTION("MET_MT6582");
MODULE_LICENSE("GPL");
