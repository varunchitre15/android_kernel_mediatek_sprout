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

#include "cmdq_mdp.h"
#include "cmdq_core.h"
#include "cmdq_reg.h"
#include "cmdq_mdp_common.h"

#include <mach/mt_clkmgr.h>
#include <mach/mt_reg_base.h>

#include <linux/met_drv.h>

#define MMSYS_CONFIG_BASE (DISPSYS_BASE)

int32_t cmdqMdpClockOn(uint64_t engineFlag)
{
	CMDQ_MSG("Enable MDP(0x%llx) clock begin\n", engineFlag);

#ifdef CMDQ_PWR_AWARE

	// TODO: in camera 3.0, how to handle ISP clock on/off....?
	// TODO: camera 3.0 exist ISP only pass (ISPI, ISPO, ISP2O)
	#if 1
	cmdq_mdp_enable(engineFlag, MT_CG_DISP0_CAM_MDP, CMDQ_ENG_MDP_CAMIN, "CAM_MDP");
	#else
	if (engineFlag & (1LL << CMDQ_ENG_ISP_IMGI)) {

		// orignal 82 desgin, isp clockon depends on if (engineFlag & (0x1 << tIMGI))

		CMDQ_MSG("Enable %s clock\n", "CAM_MDP");

		enable_clock(MT_CG_IMAGE_CAM_SMI, "CAMERA");
		enable_clock(MT_CG_IMAGE_CAM_CAM, "CAMERA");
		enable_clock(MT_CG_IMAGE_SEN_TG,  "CAMERA");
		enable_clock(MT_CG_IMAGE_SEN_CAM, "CAMERA");
		enable_clock(MT_CG_IMAGE_LARB2_SMI, "CAMERA");

		enable_clock(MT_CG_DISP0_CAM_MDP, "CAM_MDP");
	}
	#endif

	cmdq_mdp_enable(engineFlag, MT_CG_DISP0_MDP_RDMA, CMDQ_ENG_MDP_RDMA0, "MDP_RDMA0");

	cmdq_mdp_enable(engineFlag, MT_CG_DISP0_MDP_RSZ0, CMDQ_ENG_MDP_RSZ0, "MDP_RSZ0");
	cmdq_mdp_enable(engineFlag, MT_CG_DISP0_MDP_RSZ1, CMDQ_ENG_MDP_RSZ1, "MDP_RSZ1");

	cmdq_mdp_enable(engineFlag, MT_CG_DISP0_MDP_TDSHP, CMDQ_ENG_MDP_TDSHP0, "MDP_TDSHP");

	cmdq_mdp_enable(engineFlag, MT_CG_DISP0_MDP_WROT, CMDQ_ENG_MDP_WROT0, "MDP_WROT");

	cmdq_mdp_enable(engineFlag, MT_CG_DISP0_MDP_WDMA, CMDQ_ENG_MDP_WDMA, "MDP_WDMA");

#else /*CMDQ_PWR_AWARE*/
	CMDQ_MSG("Force MDP clock all on\n");

	/* enable all bits in MMSYS_CG_CLR0 and MMSYS_CG_CLR1 */
	CMDQ_REG_SET32(MMSYS_CONFIG_BASE + 0x108, 0xFFFFFFFF);
	CMDQ_REG_SET32(MMSYS_CONFIG_BASE + 0x118, 0xFFFFFFFF);

#endif				/* #ifdef CMDQ_PWR_AWARE */

	CMDQ_MSG("Enable MDP(0x%llx) clock end\n", engineFlag);

	return 0;
}

typedef struct MODULE_BASE {
	uint64_t engine;
	long base;		/* considering 64 bit kernel, use long type to store base addr */
	const char *name;
} MODULE_BASE;

#define DEFINE_MODULE(eng, base) {eng, base, #eng}

int32_t cmdqVEncDumpInfo(uint64_t engineFlag, int logLevel)
{
	CMDQ_ERR("%s failed since not support CMDQ-VENC in this platform\n", __func__);
	return 0;
}

int32_t cmdqMdpDumpInfo(uint64_t engineFlag, int logLevel)
{
#define MM_MUTEX_EN(id) (MMSYS_MUTEX_BASE + 0x20 + id * 0x20)
#define MM_MUTEX_ACC(id) (MMSYS_MUTEX_BASE + 0x24 + id * 0x20)
#define MM_MUTEX_RST(id) (MMSYS_MUTEX_BASE + 0x28 + id * 0x20)
#define MM_MUTEX_MOD(id) (MMSYS_MUTEX_BASE + 0x2C + id * 0x20)
#define MM_MUTEX_SOF(id) (MMSYS_MUTEX_BASE + 0x30 + id * 0x20)

#define MM_MUTEX_INTEN (MMSYS_MUTEX_BASE + 0x00)
#define MM_MUTEX_INTSTA (MMSYS_MUTEX_BASE + 0x04)
#define MM_MUTEX_TIMEOUT (MMSYS_MUTEX_BASE + 0x08)
#define MM_MUTEX_COMMIT (MMSYS_MUTEX_BASE + 0x0C)

	const uint32_t mutexStartIndex = 4;
	const uint32_t mutexMaxCount = 8;
	int32_t i = 0;

	if (engineFlag & (1LL << CMDQ_ENG_MDP_RDMA0)) {
		cmdq_mdp_dump_rdma(MDP_RDMA_BASE, "RDMA0");
	}

	if (engineFlag & (1LL << CMDQ_ENG_MDP_RSZ0)) {
		cmdq_mdp_dump_rsz(MDP_RSZ0_BASE, "RSZ0");
	}

	if (engineFlag & (1LL << CMDQ_ENG_MDP_RSZ1)) {
		cmdq_mdp_dump_rsz(MDP_RSZ1_BASE, "RSZ1");
	}

	if (engineFlag & (1LL << CMDQ_ENG_MDP_TDSHP0)) {
		cmdq_mdp_dump_tdshp(MDP_TDSHP_BASE, "TDSHP0");
	}

	if (engineFlag & (1LL << CMDQ_ENG_MDP_WROT0)) {
		cmdq_mdp_dump_rot(MDP_WROT_BASE, "WROT0");
	}


	if (engineFlag & (1LL << CMDQ_ENG_MDP_WDMA)) {
		cmdq_mdp_dump_wdma(MDP_WDMA_BASE, "WDMA");
	}


	/* verbose case, dump entire 1KB HW register region */
	/* for each enabled HW module. */
	if (logLevel >= 1) {
		int inner = 0;

		static const MODULE_BASE bases[] = {
			DEFINE_MODULE(CMDQ_ENG_MDP_RDMA0, MDP_RDMA_BASE),

			DEFINE_MODULE(CMDQ_ENG_MDP_RSZ0, MDP_RSZ0_BASE),
			DEFINE_MODULE(CMDQ_ENG_MDP_RSZ1, MDP_RSZ1_BASE),

			DEFINE_MODULE(CMDQ_ENG_MDP_TDSHP0, MDP_TDSHP_BASE),

			DEFINE_MODULE(CMDQ_ENG_MDP_WROT0, MDP_WROT_BASE),

			DEFINE_MODULE(CMDQ_ENG_MDP_WDMA, MDP_WDMA_BASE),
		};

		for (inner = 0; inner < (sizeof(bases) / sizeof(bases[0])); ++inner) {
			if (engineFlag & (1LL << bases[inner].engine)) {
				CMDQ_ERR("========= [CMDQ] %s dump base 0x%lx ========\n",
					 bases[inner].name, bases[inner].base);
				print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
					       (void *)bases[inner].base, 1024, false);
			}
		}
	}

	/* error dump for MDP to identify whether ISP's tile setting is correct for ISP path */
	CMDQ_ERR("0x1400E000 = 0x%08x\n", CMDQ_REG_GET32(MM_MUTEX_INTEN));
	CMDQ_ERR("0x1400E004 = 0x%08x\n", CMDQ_REG_GET32(MM_MUTEX_INTSTA));
	CMDQ_ERR("0x1400E008 = 0x%08x\n", CMDQ_REG_GET32(MM_MUTEX_TIMEOUT));
	CMDQ_ERR("0x1400E00C = 0x%08x\n", CMDQ_REG_GET32(MM_MUTEX_COMMIT));

	for (i = mutexStartIndex; i < mutexMaxCount; i++ ) {
		CMDQ_ERR("MM_MUTEX_EN(%d) : 0x%08x = 0x%08x\n",
			i, MM_MUTEX_EN(i), CMDQ_REG_GET32(MM_MUTEX_EN(i)));
		CMDQ_ERR("MM_MUTEX_ACC(%d): 0x%08x = 0x%08x\n",
			i, MM_MUTEX_ACC(i), CMDQ_REG_GET32(MM_MUTEX_ACC(i)));
		CMDQ_ERR("MM_MUTEX_RST(%d): 0x%08x = 0x%08x\n",
			i, MM_MUTEX_RST(i), CMDQ_REG_GET32(MM_MUTEX_RST(i)));
		CMDQ_ERR("MM_MUTEX_MOD(%d): 0x%08x = 0x%08x\n",
			i, MM_MUTEX_MOD(i), CMDQ_REG_GET32(MM_MUTEX_MOD(i)));
		CMDQ_ERR("MM_MUTEX_SOF(%d): 0x%08x = 0x%08x\n",
			i, MM_MUTEX_SOF(i), CMDQ_REG_GET32(MM_MUTEX_SOF(i)));
	}

	CMDQ_ERR("0x15004000 = 0x%08x\n", CMDQ_REG_GET32(0xF5004000));

	return 0;
}

typedef enum MOUT_BITS {
	MOUT_BITS_CAM_MDP = 0,	/* bit  0: ISP_MDP mutiple outupt reset */
	MOUT_BITS_MDP_RDMA = 1,	/* bit  1: MDP_RDMA0 mutiple outupt reset */
	MOUT_BITS_MDP_RSZ0 = 2,	/* bit  2: MDP_PRZ0 mutiple outupt reset */
	MOUT_BITS_MDP_RSZ1 = 3,	/* bit  3: MDP_PRZ1 mutiple outupt reset */
	MOUT_BITS_MDP_TDSHP0 = 4,	/* bit  4: MDP_TDSHP0 mutiple outupt reset */
} MOUT_BITS;

int32_t cmdq_mdp_reset_with_mmsys(const uint64_t engineToResetAgain)
{
	static const long MMSYS_SW0_RST_B_REG = MMSYS_CONFIG_BASE + (0x138);
	int i = 0;
	uint32_t reset_bits = 0L;
	const static int engineResetBit[32] = {
		-1,	/* bit  0 : SMI COMMON */
		-1,	/* bit  1 : SMI LARB0 */
		-1,	/* bit  2 : MM CMDQ */
		-1, /* bit  3 : MUTEX */
		-1,	/* bit  4 : DISP_COLOR */
		-1,	/* bit  5 : DISP_BLS */
		-1,	/* bit  6 : DISP_WDMA */
		-1, /* bit  7 : DISP_RDMA */
		-1,	/* bit  8 : DISP_OVL */
		CMDQ_ENG_MDP_TDSHP0, /* bit  9 : MDP_TDSHP */
		CMDQ_ENG_MDP_WROT0,	 /* bit 10 : MDP_WROT0 */
		CMDQ_ENG_MDP_WDMA,	 /* bit 11 : MDP_WDMA */
		CMDQ_ENG_MDP_RSZ1,	 /* bit	12 : MDP_RSZ1 */
		CMDQ_ENG_MDP_RSZ0,	 /* bit	13 : MDP_RSZ0 */
		CMDQ_ENG_MDP_RDMA0,  /* bit	14 : MDP_RDMA0 */
		[15 ... 31] = -1
	};

	for (i = 0; i < 32; ++i) {
		if (0 > engineResetBit[i])
			continue;

		if (engineToResetAgain & (1LL << engineResetBit[i])) {
			reset_bits |= (1 << i);
		}
	}

	if (0 != reset_bits) {
		/* 0: reset */
		/* 1: not reset */
		/* so we need to reverse the bits */
		reset_bits = ~reset_bits;

		CMDQ_REG_SET32(MMSYS_SW0_RST_B_REG, reset_bits);
		CMDQ_REG_SET32(MMSYS_SW0_RST_B_REG, ~0);
		/* This takes effect immediately, no need to poll state */
	}

	return 0;
}

int32_t cmdqMdpResetEng(uint64_t engineFlag)
{
#ifndef CMDQ_PWR_AWARE
	return 0;
#else
	int status = 0;
	int64_t engineToResetAgain = 0LL;
	uint32_t mout_bits_old = 0L;
	uint32_t mout_bits = 0L;
	static const long MMSYS_MOUT_RST_REG = MMSYS_CONFIG_BASE + (0x034);

	CMDQ_PROF_START(0, "MDP_Rst");
	CMDQ_VERBOSE("Reset MDP(0x%llx) begin\n", engineFlag);

	/* After resetting each component, */
	/* we need also reset corresponding MOUT config. */
	mout_bits_old = CMDQ_REG_GET32(MMSYS_MOUT_RST_REG);
	mout_bits = 0;

	if (engineFlag & (1LL << CMDQ_ENG_MDP_RDMA0)) {
		mout_bits |= (1 << MOUT_BITS_MDP_RDMA);

		status = cmdq_mdp_loop_reset(MT_CG_DISP0_MDP_RDMA,
					     MDP_RDMA_BASE + 0x8,
					     MDP_RDMA_BASE + 0x408, 0x7FF00, 0x100, "RDMA", false);
		if (0 != status) {
			engineToResetAgain |= (1 << CMDQ_ENG_MDP_RDMA0);
		}
	}

	if (engineFlag & (1LL << CMDQ_ENG_MDP_RSZ0)) {
		mout_bits |= (1 << MOUT_BITS_MDP_RSZ0);
		if (clock_is_on(MT_CG_DISP0_MDP_RSZ0)) {
			CMDQ_REG_SET32(MDP_RSZ0_BASE, 0x0);
			CMDQ_REG_SET32(MDP_RSZ0_BASE, 0x10000);
			CMDQ_REG_SET32(MDP_RSZ0_BASE, 0x0);
		}
	}

	if (engineFlag & (1LL << CMDQ_ENG_MDP_RSZ1)) {
		mout_bits |= (1 << MOUT_BITS_MDP_RSZ1);
		if (clock_is_on(MT_CG_DISP0_MDP_RSZ1)) {
			CMDQ_REG_SET32(MDP_RSZ1_BASE, 0x0);
			CMDQ_REG_SET32(MDP_RSZ1_BASE, 0x10000);
			CMDQ_REG_SET32(MDP_RSZ1_BASE, 0x0);
		}
	}

	if (engineFlag & (1LL << CMDQ_ENG_MDP_TDSHP0)) {
		mout_bits |= (1 << MOUT_BITS_MDP_TDSHP0);
		if (clock_is_on(MT_CG_DISP0_MDP_TDSHP)) {
			CMDQ_REG_SET32(MDP_TDSHP_BASE + 0x100, 0x0);
			CMDQ_REG_SET32(MDP_TDSHP_BASE + 0x100, 0x2);
			CMDQ_REG_SET32(MDP_TDSHP_BASE + 0x100, 0x0);
		}
	}

	if (engineFlag & (1LL << CMDQ_ENG_MDP_WROT0)) {
		status = cmdq_mdp_loop_reset(MT_CG_DISP0_MDP_WROT,
					     MDP_WROT_BASE + 0x010,
					     MDP_WROT_BASE + 0x014, 0x1, 0x0, "WROT", false);
		if (0 != status) {
			engineToResetAgain |= (1LL << CMDQ_ENG_MDP_WROT0);
		}
	}


	if (engineFlag & (1LL << CMDQ_ENG_MDP_WDMA)) {
		status = cmdq_mdp_loop_reset(MT_CG_DISP0_MDP_WDMA,
					     MDP_WDMA_BASE + 0x00C,
					     MDP_WDMA_BASE + 0x0A0, 0x3FF, 0x1, "WDMA", false);
		if (0 != status) {
			engineToResetAgain |= (1LL << CMDQ_ENG_MDP_WDMA);
		}
	}

	if (engineFlag & (1LL << CMDQ_ENG_MDP_CAMIN)) {
		/* MDP_CAMIN can only reset by mmsys, */
		/* so this is not a "error" */

		// TODO: way to reset camin in 82
	#if 0
			cmdq_mdp_reset_with_mmsys((1LL << CMDQ_ENG_MDP_CAMIN));
	#endif
	}

	/*
	   when MDP engines fail to reset,
	   1. print SMI debug log
	   2. try resetting from MMSYS to restore system state
	   3. report to QA by raising AEE warning
	   this reset will reset all registers to power on state.
	   but DpFramework always reconfigures register values,
	   so there is no need to backup registers.
	 */
	if (0 != engineToResetAgain) {
		/* check SMI state immediately */
		/* if (1 == is_smi_larb_busy(0)) */
		/* { */
		/* smi_hanging_debug(5); */
		/* } */

		CMDQ_ERR("Reset failed MDP engines(0x%llx), reset again with MMSYS_SW0_RST_B\n",
			 engineToResetAgain);

		cmdq_mdp_reset_with_mmsys(engineToResetAgain);

		/* finally, raise AEE warning to report normal reset fail. */
		/* we hope that reset MMSYS. */
		CMDQ_AEE("MDP", "Disable 0x%llx engine failed\n", engineToResetAgain);

		status = -EFAULT;
	}

	/* Reset MOUT bits */
	/* MOUT configuration reset */
	CMDQ_REG_SET32(MMSYS_MOUT_RST_REG, (mout_bits_old & (~mout_bits)));
	CMDQ_REG_SET32(MMSYS_MOUT_RST_REG, (mout_bits_old | mout_bits));
	CMDQ_REG_SET32(MMSYS_MOUT_RST_REG, (mout_bits_old & (~mout_bits)));

	CMDQ_MSG("Reset MDP(0x%llx) end\n", engineFlag);
	CMDQ_PROF_END(0, "MDP_Rst");

	return status;

#endif				/* #ifdef CMDQ_PWR_AWARE */

}

int32_t cmdqMdpClockOff(uint64_t engineFlag)
{
#ifdef CMDQ_PWR_AWARE
	CMDQ_MSG("Disable MDP(0x%llx) clock begin\n", engineFlag);

	if (engineFlag & (1LL << CMDQ_ENG_MDP_WDMA)) {
		cmdq_mdp_loop_off(MT_CG_DISP0_MDP_WDMA,
				  MDP_WDMA_BASE + 0x00C,
				  MDP_WDMA_BASE + 0x0A0, 0x3FF, 0x1, "MDP_WDMA", false);
	}

	if (engineFlag & (1LL << CMDQ_ENG_MDP_WROT0)) {
		cmdq_mdp_loop_off(MT_CG_DISP0_MDP_WROT,
				  MDP_WROT_BASE + 0x010,
				  MDP_WROT_BASE + 0x014, 0x1, 0x0, "MDP_WROT0", false);
	}

	if (engineFlag & (1LL << CMDQ_ENG_MDP_TDSHP0)) {
		if (clock_is_on(MT_CG_DISP0_MDP_TDSHP)) {
			CMDQ_REG_SET32(MDP_TDSHP_BASE + 0x100, 0x0);
			CMDQ_REG_SET32(MDP_TDSHP_BASE + 0x100, 0x2);
			CMDQ_REG_SET32(MDP_TDSHP_BASE + 0x100, 0x0);
			CMDQ_MSG("Disable MDP_TDSHP0 clock\n");
			disable_clock(MT_CG_DISP0_MDP_TDSHP, "MDP_TDSHP0");
		}
	}

	if (engineFlag & (1LL << CMDQ_ENG_MDP_RSZ1)) {
		if (clock_is_on(MT_CG_DISP0_MDP_RSZ1)) {
			CMDQ_REG_SET32(MDP_RSZ1_BASE, 0x0);
			CMDQ_REG_SET32(MDP_RSZ1_BASE, 0x10000);
			CMDQ_REG_SET32(MDP_RSZ1_BASE, 0x0);

			CMDQ_MSG("Disable MDP_RSZ1 clock\n");

			disable_clock(MT_CG_DISP0_MDP_RSZ1, "MDP_RSZ1");
		}
	}

	if (engineFlag & (1LL << CMDQ_ENG_MDP_RSZ0)) {
		if (clock_is_on(MT_CG_DISP0_MDP_RSZ0)) {
			CMDQ_REG_SET32(MDP_RSZ0_BASE, 0x0);
			CMDQ_REG_SET32(MDP_RSZ0_BASE, 0x10000);
			CMDQ_REG_SET32(MDP_RSZ0_BASE, 0x0);

			CMDQ_MSG("Disable MDP_RSZ0 clock\n");

			disable_clock(MT_CG_DISP0_MDP_RSZ0, "MDP_RSZ0");
		}
	}

	if (engineFlag & (1LL << CMDQ_ENG_MDP_RDMA0)) {
		cmdq_mdp_loop_off(MT_CG_DISP0_MDP_RDMA,
				  MDP_RDMA_BASE + 0x008,
				  MDP_RDMA_BASE + 0x408, 0x7FF00, 0x100, "MDP_RDMA0", false);
	}


#if 1
	if (engineFlag & (1LL << CMDQ_ENG_MDP_CAMIN)) {
		if (clock_is_on(MT_CG_DISP0_CAM_MDP)) {

			CMDQ_MSG("Disable MDP_CAMIN clock\n");
			disable_clock(MT_CG_DISP0_CAM_MDP, "CAM_MDP");
		}
	}
#else
	if (engineFlag & (1LL << CMDQ_ENG_MDP_CAMIN)) {
		CMDQ_MSG("Enable %s clock\n", "CAM_MDP");

		disable_clock(MT_CG_DISP0_CAM_MDP, "CAM_MDP");

		disable_clock(MT_CG_IMAGE_CAM_SMI, "CAMERA");
		disable_clock(MT_CG_IMAGE_CAM_CAM, "CAMERA");
		disable_clock(MT_CG_IMAGE_SEN_TG,  "CAMERA");
		disable_clock(MT_CG_IMAGE_SEN_CAM, "CAMERA");
		disable_clock(MT_CG_IMAGE_LARB2_SMI, "CAMERA");
	}
#endif




	if (engineFlag & (1LL << CMDQ_ENG_MDP_CAMIN)) {
		if (clock_is_on(MT_CG_DISP0_CAM_MDP)) {

			cmdq_mdp_reset_with_mmsys((1LL << CMDQ_ENG_MDP_CAMIN));

			CMDQ_MSG("Disable MDP_CAMIN clock\n");
			disable_clock(MT_CG_DISP0_CAM_MDP, "CAM_MDP");
		}
	}

	CMDQ_MSG("Disable MDP(0x%llx) clock end\n", engineFlag);
#endif				/* #ifdef CMDQ_PWR_AWARE */

	return 0;
}

const uint32_t cmdq_mdp_rdma_get_reg_offset_src_addr(void)
{
	return 0x40;
}

const uint32_t cmdq_mdp_wrot_get_reg_offset_dst_addr(void)
{
	return 0x28;
}

const uint32_t cmdq_mdp_wdma_get_reg_offset_dst_addr(void)
{
	return 0xF00;
}

extern void testcase_clkmgr_impl(enum cg_clk_id gateId,
			const char *name,
			const uint32_t testWriteReg,
			const uint32_t testWriteValue,
			const uint32_t testReadReg,
			const bool verifyWriteResult);

void testcase_clkmgr_mdp(void)
{
	/* RDMA clk test with src buffer addr */
	testcase_clkmgr_impl(MT_CG_DISP0_MDP_RDMA,
			"CMDQ_TEST_MDP_RDMA",
			MDP_RDMA_BASE + cmdq_mdp_rdma_get_reg_offset_src_addr(),
			0xAACCBBDD,
			MDP_RDMA_BASE + cmdq_mdp_rdma_get_reg_offset_src_addr(),
			true);

	/* WDMA clk test with dst buffer addr */
	testcase_clkmgr_impl(MT_CG_DISP0_MDP_WDMA,
			"CMDQ_TEST_MDP_WDMA",
			MDP_WDMA_BASE + cmdq_mdp_wdma_get_reg_offset_dst_addr(),
			0xAACCBBDD,
			MDP_WDMA_BASE + cmdq_mdp_wdma_get_reg_offset_dst_addr(),
			true);

	/* WROT clk test with dst buffer addr */
	testcase_clkmgr_impl(MT_CG_DISP0_MDP_WROT,
			"CMDQ_TEST_MDP_WROT",
			MDP_WROT_BASE + cmdq_mdp_wrot_get_reg_offset_dst_addr(),
			0xAACCBBDD,
			MDP_WROT_BASE + cmdq_mdp_wrot_get_reg_offset_dst_addr(),
			true);

	/* TDSHP clk test with input size*/
	testcase_clkmgr_impl(MT_CG_DISP0_MDP_TDSHP,
			"CMDQ_TEST_MDP_TDSHP",
			MDP_TDSHP_BASE + 0x244,
			0xAACCBBDD,
			MDP_TDSHP_BASE + 0x244,
			true);

	/* RSZ clk test with debug port */
	testcase_clkmgr_impl(MT_CG_DISP0_MDP_RSZ0,
			"CMDQ_TEST_MDP_RSZ0",
			MDP_RSZ0_BASE + 0x040,
			0x00000001,
			MDP_RSZ0_BASE + 0x044,
			false);

	testcase_clkmgr_impl(MT_CG_DISP0_MDP_RSZ1,
			"CMDQ_TEST_MDP_RSZ1",
			MDP_RSZ1_BASE + 0x040,
			0x00000001,
			MDP_RSZ1_BASE + 0x044,
			false);
}
