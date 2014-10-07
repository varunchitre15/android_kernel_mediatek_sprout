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

#ifndef __CMDQ_MDP_COMMON_H__
#define __CMDQ_MDP_COMMON_H__

#include "cmdq_def.h"

#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif


#include <mach/mt_clkmgr.h>

	void cmdq_mdp_enable(uint64_t engineFlag,
			     enum cg_clk_id gateId, CMDQ_ENG_ENUM engine, const char *name);

	int cmdq_mdp_loop_reset(enum cg_clk_id clkId,
				const uint32_t resetReg,
				const uint32_t resetStateReg,
				const uint32_t resetMask,
				const uint32_t resetValue, const char *name, const bool pollInitResult);

	void cmdq_mdp_loop_off(enum cg_clk_id clkId,
			       const uint32_t resetReg,
			       const uint32_t resetStateReg,
			       const uint32_t resetMask,
			       const uint32_t resetValue, const char *name, const bool pollInitResult);

	void cmdq_mdp_dump_venc(const unsigned long base, const char *label);
	void cmdq_mdp_dump_rdma(const unsigned long base, const char *label);
	void cmdq_mdp_dump_rsz(const unsigned long base, const char *label);
	void cmdq_mdp_dump_rot(const unsigned long base, const char *label);
	void cmdq_mdp_dump_tdshp(const unsigned long base, const char *label);
	void cmdq_mdp_dump_wdma(const unsigned long base, const char *label);

#ifdef __cplusplus
}
#endif
#endif				/* __CMDQ_MDP_COMMON_H__ */
