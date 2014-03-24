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
#include <asm/io.h>
#include <mach/mt_reg_base.h>
#include <mach/wd_api.h>
#include <linux/memblock.h>
#include <linux/mrdump.h>
#include <linux/reboot.h>

#define MRDUMP_CB_ADDR 0x81F00000
#define MRDUMP_CB_SIZE 0x1000

#define LK_LOAD_ADDR 0x81E00000
#define LK_LOAD_SIZE 0x100000

static void mrdump_hw_enable(bool enabled)
{
}

static void mrdump_reboot(void)
{
	emergency_restart();
}

const struct mrdump_platform mrdump_mt6582_platform = {
	.hw_enable = mrdump_hw_enable,
	.reboot = mrdump_reboot
};

void mrdump_reserve_memory(void)
{
	struct mrdump_control_block *cblock = NULL;

	/* We must reserved the lk block, can we pass it from lk? */    
	memblock_reserve(LK_LOAD_ADDR, LK_LOAD_SIZE);

	memblock_reserve(MRDUMP_CB_ADDR, MRDUMP_CB_SIZE);
	cblock = (struct mrdump_control_block *)__va(MRDUMP_CB_ADDR);

	mrdump_platform_init(cblock, &mrdump_mt6582_platform);
}
