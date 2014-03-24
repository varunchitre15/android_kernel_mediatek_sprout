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

#ifndef __L_WD_DRV_H
#define __L_WD_DRV_H
#include <mach/mt_typedefs.h>
#include "wd_api.h"


struct local_wd_driver
{
    //struct device_driver driver;
    //const struct platform_device_id *id_table;
    void (*set_time_out_value)(unsigned int value);
	void (*mode_config)(	BOOL dual_mode_en, 
					BOOL irq, 
					BOOL ext_en, 
					BOOL ext_pol, 
					BOOL wdt_en );
	int  (*enable)(enum wk_wdt_en en);
	void (*restart)(enum wd_restart_type type);
	void (*dump_reg)(void);
	void (*arch_reset)(char mode);
	int  (*swsysret_config)(int bit,int set_value);
};

struct local_wd_driver *get_local_wd_drv(void);


#endif

