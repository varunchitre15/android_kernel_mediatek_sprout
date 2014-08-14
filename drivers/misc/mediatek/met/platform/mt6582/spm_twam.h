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

#ifndef _SPM_TWAM_H_
#define _SPM_TWAM_H_

struct sig_desc_t {
	unsigned long sig;
	char name[40];
};

/* mt6582 */
struct sig_desc_t twam_sig[] = {
	{ 0,	"axi_idle_to_scpsys"},
	{ 1,	"faxi_all_axi_idle"},
	{ 2,	"emi_idle"},
	{ 3,	"disp_req"},
	{ 4,	"mfg_req"},
	{ 5,	"core0_wfi"},
	{ 6,	"core1_wfi"},
	{ 7,	"core2_wfi"},
	{ 8,	"core3_wfi"},
	{ 9,	"mcu_i2c_idle"},
	{ 10,	"mcu_scu_idle"},
	{ 11,	"dram_sref"},
	{ 12,	"md_srcclkena"},
	{ 13,	"md_apsrc_req"},
	{ 14,	"conn_srcclkena"},
	{ 15,	"conn_apsrc_req"}
};

#endif // _SPM_TWAM_H_
