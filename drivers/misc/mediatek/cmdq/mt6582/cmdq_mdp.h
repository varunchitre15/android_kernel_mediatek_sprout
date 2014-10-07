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

#ifndef __CMDQ_MDP_H__
#define __CMDQ_MDP_H__

#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif

	int32_t cmdqMdpClockOn(uint64_t engineFlag);

	int32_t cmdqMdpDumpInfo(uint64_t engineFlag, int level);

	int32_t cmdqVEncDumpInfo(uint64_t engineFlag, int level);

	int32_t cmdqMdpResetEng(uint64_t engineFlag);

	int32_t cmdqMdpClockOff(uint64_t engineFlag);

	const uint32_t cmdq_mdp_rdma_get_reg_offset_src_addr(void);
	const uint32_t cmdq_mdp_wrot_get_reg_offset_dst_addr(void);
	const uint32_t cmdq_mdp_wdma_get_reg_offset_dst_addr(void);

	void testcase_clkmgr_mdp(void);
#ifdef __cplusplus
}
#endif
#endif				/* __CMDQ_MDP_H__ */
