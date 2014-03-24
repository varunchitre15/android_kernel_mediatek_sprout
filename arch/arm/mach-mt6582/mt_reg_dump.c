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

#include "mach/mt_reg_base.h"
#include "mach/mt_reg_dump.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <mach/sync_write.h>
#include <mach/dbg_dump.h>
#include <linux/kallsyms.h>
#include <linux/init.h>

static struct reg_dump_driver_data reg_dump_driver_data =
{
    .mcu_regs = (MCUSYS_CFGREG_BASE + 0x300),
};

static struct platform_device reg_dump_device = 
{    
    .name = "dbg_reg_dump",
    .dev = 
    {
        .platform_data = &(reg_dump_driver_data),
    },
};

/*
 * mt_reg_dump_init: initialize driver.
 * Always return 0.
 */

static int __init mt_reg_dump_init(void)
{
	int err;
	
  err = platform_device_register(&(reg_dump_device));
  if (err) {
      pr_err("Fail to register reg_dump_device");
      return err;
  }  
	
  return 0;
}

arch_initcall(mt_reg_dump_init);
