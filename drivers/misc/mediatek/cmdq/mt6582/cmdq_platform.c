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

#include "cmdq_platform.h"
#include "cmdq_core.h"
#include "cmdq_reg.h"
#include "mach/mt_clkmgr.h"

#include <linux/vmalloc.h>
#include <mach/mt_boot.h>
#include <linux/seq_file.h>

extern void smi_dumpDebugMsg(void);

#define MMSYS_CONFIG_BASE (DISPSYS_BASE)

typedef struct RegDef {
	int offset;
	const char *name;
} RegDef;

const bool cmdq_core_support_sync_non_suspendable(void)
{
	return false;
}

const bool cmdq_core_support_wait_and_receive_event_in_same_tick(void)
{
	return true;
}

const uint32_t cmdq_core_get_subsys_LSB_in_argA(void)
{
	return 22;
}

bool cmdq_core_is_disp_scenario(const CMDQ_SCENARIO_ENUM scenario)
{
	return false;
}

bool cmdq_core_should_enable_prefetch(CMDQ_SCENARIO_ENUM scenario)
{
	return false;
}

bool cmdq_core_should_profile(CMDQ_SCENARIO_ENUM scenario)
{
	/* note command profile method impl depends on GPR */
	/* and CMDQ dose not support GPR in this platform */
	return false;
}

int cmdq_core_disp_thread_index_from_scenario(CMDQ_SCENARIO_ENUM scenario)
{
	if (cmdq_core_should_enable_prefetch(scenario)) {
		return 0;
	}

	/* there is no higher priority client in this platform */
	/* freely dispatch */
	return CMDQ_INVALID_THREAD;
}

CMDQ_HW_THREAD_PRIORITY_ENUM cmdq_core_priority_from_scenario(CMDQ_SCENARIO_ENUM scenario)
{
	/* there is no higher priority client in this platform */
	return CMDQ_THR_PRIO_NORMAL;
}


void cmdq_core_get_reg_id_from_hwflag(uint64_t hwflag,
				      CMDQ_DATA_REGISTER_ENUM *valueRegId,
				      CMDQ_DATA_REGISTER_ENUM *destRegId,
				      CMDQ_EVENT_ENUM *regAccessToken)
{
	/* not support GPR in this platform*/
	*regAccessToken = CMDQ_SYNC_TOKEN_INVALID;
	return;
}


const char *cmdq_core_module_from_event_id(CMDQ_EVENT_ENUM event, uint32_t instA, uint32_t instB)
{
	const char *module = "CMDQ";

	switch (event) {
	case CMDQ_EVENT_DISP_RDMA0_SOF:
	case CMDQ_EVENT_DISP_RDMA0_EOF:
		module = "DISP_RDMA";
		break;

	case CMDQ_EVENT_DISP_WDMA_SOF:
	case CMDQ_EVENT_DISP_WDMA_EOF:
		module = "DISP_WDMA";
		break;

	case CMDQ_EVENT_DISP_OVL_SOF:
	case CMDQ_EVENT_DISP_OVL_EOF:
		module = "DISP_OVL";
		break;

	case CMDQ_EVENT_DISP_BLS_EOF:
	case CMDQ_EVENT_DISP_COLOR_EOF:
	case CMDQ_EVENT_DISP_BLS_SOF:
	case CMDQ_EVENT_DISP_COLOR_SOF:
		module = "DISP";
		break;

	case CMDQ_EVENT_MDP_TDSHP_EOF:
	case CMDQ_EVENT_MDP_RSZ0_EOF:
	case CMDQ_EVENT_MDP_RSZ1_EOF:
	case CMDQ_EVENT_MDP_RDMA_EOF:
	case CMDQ_EVENT_MDP_WDMA_EOF:
	case CMDQ_EVENT_MDP_WROT_EOF:
	case CMDQ_EVENT_MDP_TDSHP_SOF:
	case CMDQ_EVENT_MDP_RSZ0_SOF:
	case CMDQ_EVENT_MDP_RSZ1_SOF:
	case CMDQ_EVENT_MDP_RDMA_SOF:
	case CMDQ_EVENT_MDP_WDMA_SOF:
	case CMDQ_EVENT_MDP_WROT_SOF:
	case CMDQ_EVENT_CAM_MDP_SOF:
		module = "MDP";
		break;

	case CMDQ_EVENT_ISP1_EOF:
	case CMDQ_EVENT_ISP2_EOF:
		module = "ISP";
		break;

	case CMDQ_EVENT_MUTEX0 ... CMDQ_EVENT_MUTEX7:
		module = "MUTEX";
		break;

	case CMDQ_EVENT_RESERVED1 ... CMDQ_EVENT_RESERVED4:
		module = "Reserved";
		break;

	default:
		module = "CMDQ";
		break;
	}

	return module;
}

const char *cmdq_core_parse_module_from_reg_addr(uint32_t reg_addr)
{
	const uint32_t addr_base_and_page = (reg_addr & 0xFFFFF000);
	const uint32_t addr_base_shifted = (reg_addr & 0xFFFF0000) >> 16;
	const char *module = "CMDQ";

	/* for well-known base, we check them with 12-bit mask */
	/* defined in mt_reg_base.h */
#define DECLARE_REG_RANGE(base, name) case IO_VIRT_TO_PHYS(base): return #name;
	switch (addr_base_and_page) {
		DECLARE_REG_RANGE(MDP_RDMA_BASE, MDP);
		DECLARE_REG_RANGE(MDP_RSZ0_BASE, MDP);
		DECLARE_REG_RANGE(MDP_RSZ1_BASE, MDP);
		DECLARE_REG_RANGE(MDP_WDMA_BASE, MDP);
		DECLARE_REG_RANGE(MDP_WROT_BASE, MDP);
		DECLARE_REG_RANGE(MDP_TDSHP_BASE, MDP);

		DECLARE_REG_RANGE(OVL0_BASE, OVL0);
		DECLARE_REG_RANGE(DISP_RDMA_BASE, DISP_RDMA);
		DECLARE_REG_RANGE(DISP_WDMA_BASE, DISP_WDMA);
		DECLARE_REG_RANGE(DISP_BLS_BASE, DISP_BLS);
		DECLARE_REG_RANGE(DISP_COLOR_BASE, COLOR);
		DECLARE_REG_RANGE(DPI_BASE, DISP_DSI);
	}
#undef DECLARE_REG_RANGE

	/* for other register address we rely on GCE subsys to group them with */
	/* 16-bit mask. */
#undef DECLARE_CMDQ_SUBSYS
#define DECLARE_CMDQ_SUBSYS(msb, id, grp, base) case msb: return #grp;
	switch (addr_base_shifted) {
#include "cmdq_subsys.h"
	}
#undef DECLARE_CMDQ_SUBSYS
	return module;
}

const int32_t cmdq_core_can_module_entry_suspend(EngineStruct *engineList)
{
	int32_t status = 0;
	int i;
	CMDQ_ENG_ENUM e = 0;

	CMDQ_ENG_ENUM mdpEngines[] ={
		CMDQ_ENG_ISP_IMGI,
		CMDQ_ENG_MDP_RDMA0,
		CMDQ_ENG_MDP_RSZ0,
		CMDQ_ENG_MDP_RSZ1,
		CMDQ_ENG_MDP_TDSHP0,
		CMDQ_ENG_MDP_WROT0,
		CMDQ_ENG_MDP_WDMA
	};

	for (i = 0; i < (sizeof(mdpEngines) / sizeof(CMDQ_ENG_ENUM)); ++i) {
		e = mdpEngines[i];
		if (0 != engineList[e].userCount) {
			CMDQ_ERR("suspend but engine %d has userCount %d, owner=%d\n",
				 e, engineList[e].userCount, engineList[e].currOwner);
			status = -EBUSY;
		}
	}

	return status;
}

ssize_t cmdq_core_print_status_clock(char *buf)
{
	int32_t length = 0;
	char *pBuffer = buf;

#ifdef CMDQ_PWR_AWARE
	pBuffer += sprintf(pBuffer, "MT_CG_DISP0_MM_CMDQ: %d, MT_CG_DISP0_MUTEX: %d\n",
				   clock_is_on(MT_CG_DISP0_MM_CMDQ), clock_is_on(MT_CG_DISP0_MUTEX));
#endif
	length = pBuffer - buf;
	return length;
}

void cmdq_core_print_status_seq_clock(struct seq_file *m)
{
#ifdef CMDQ_PWR_AWARE
	seq_printf(m, "MT_CG_DISP0_MM_CMDQ: %d, MT_CG_DISP0_MUTEX: %d\n",
				clock_is_on(MT_CG_DISP0_MM_CMDQ), clock_is_on(MT_CG_DISP0_MUTEX));
#endif

}

void cmdq_core_enable_common_clock_locked_impl(bool enable)
{
#ifdef CMDQ_PWR_AWARE

	if (enable) {
		CMDQ_LOG("[CLOCK] Enable CMDQ Clock\n");
		cmdq_core_enable_cmdq_clock_locked_impl(enable, CMDQ_DRIVER_DEVICE_NAME);

		CMDQ_LOG("[CLOCK] Enable SMI & LARB0 Clock\n");
		enable_clock(MT_CG_DISP0_SMI_COMMON, "CMDQ_MDP");
		enable_clock(MT_CG_DISP0_SMI_LARB0, "CMDQ_MDP");

		CMDQ_LOG("[CLOCK] enable MT_CG_DISP0_MUTEX\n");
		enable_clock(MT_CG_DISP0_MUTEX, "CMDQ_MDP");
	} else {
		CMDQ_LOG("[CLOCK] Disable CMDQ Clock\n");
		cmdq_core_enable_cmdq_clock_locked_impl(enable, CMDQ_DRIVER_DEVICE_NAME);

		CMDQ_LOG("[CLOCK] Disable SMI & LARB0 Clock\n");
		/* disable, reverse the sequence */
		disable_clock(MT_CG_DISP0_SMI_LARB0, "CMDQ_MDP");
		disable_clock(MT_CG_DISP0_SMI_COMMON, "CMDQ_MDP");

		CMDQ_LOG("[CLOCK] disable MT_CG_DISP0_MUTEX\n");
		disable_clock(MT_CG_DISP0_MUTEX, "CMDQ_MDP");
	}



#endif /* CMDQ_PWR_AWARE */
}

void cmdq_core_enable_cmdq_clock_locked_impl(bool enable, char *deviceName)
{
#ifdef CMDQ_PWR_AWARE
	if (enable) {
		enable_clock(MT_CG_DISP0_MM_CMDQ, deviceName);

	} else {
		disable_clock(MT_CG_DISP0_MM_CMDQ, deviceName);
	}
#endif /* CMDQ_PWR_AWARE */
}

const char* cmdq_core_parse_error_module_by_hwflag_impl(struct TaskStruct *pTask)
{
	/* do nothing */
	/* other case, we need to analysis instruction for more detail */
	return NULL;
}

void cmdq_core_dump_mmsys_config(void)
{
	int i = 0;
	uint32_t value = 0;

	const static struct RegDef configRegisters[] = {
		{0x01C, "CAM_MDP_MOUT_EN"},
		{0x020, "MDP_RDMA_MOUT_EN"},
		{0x024, "MDP_PRZ0_MOUT_EN"},
		{0x028, "MDP_PRZ1_MOUT_EN"},
		{0x02C, "MDP_TDSHP_MOUT_EN"},
		{0x030, "DISP_OVL0_MOUT_EN"},
		{0x038, "MDP_PRZ0_SEL_IN"},
		{0x03C, "MDP_PRZ1_SEL_IN"},
		{0x040, "MDP_TDSHP_SEL_IN"},
		{0x044, "MDP_WROT_SEL_IN"},
		{0x048, "MDP_WDMA_SEL_IN"},
		{0x04C, "DISP_OUT_SEL"},
		/* ACK and REQ related */
		{0x860, "MMSYS_DL_VALID_0"},
		{0x864, "MMSYS_DL_VALID_1"},
		{0x868, "MMSYS_DL_READY0"},
		{0x86C, "MMSYS_DL_READY1"},
	};

	for (i = 0; i < sizeof(configRegisters) / sizeof(configRegisters[0]); ++i) {
		value = CMDQ_REG_GET16(MMSYS_CONFIG_BASE + configRegisters[i].offset);
		CMDQ_ERR("%s: 0x%08x\n", configRegisters[i].name, value);
	}

	return;
}

void cmdq_core_dump_clock_gating(void)
{
	uint32_t value[3] = { 0 };

	value[0] = CMDQ_REG_GET32(MMSYS_CONFIG_BASE + 0x100);
	value[1] = CMDQ_REG_GET32(MMSYS_CONFIG_BASE + 0x110);
	value[2] = CMDQ_REG_GET32(MMSYS_CONFIG_BASE + 0x890);
	CMDQ_ERR("MMSYS_CG_CON0(deprecated): 0x%08x, MMSYS_CG_CON1: 0x%08x\n", value[0], value[1]);
	CMDQ_ERR("MMSYS_DUMMY_REG: 0x%08x\n", value[2]);
	CMDQ_ERR("ISPSys clock state %d\n", subsys_is_on(SYS_ISP));
	CMDQ_ERR("DisSys clock state %d\n", subsys_is_on(SYS_DIS));
}

int cmdq_core_dump_smi(const int showSmiDump)
{
	int isSMIHang = 0;

	/* do not care showSmiDump param in this platform */

#ifndef CONFIG_MTK_FPGA

	int32_t i = 0;

	/* dump 5 times to observe timing issue */
	for (i = 0; i < 5; i++)
	{
		CMDQ_ERR("=============== [CMDQ] SMI Dump %d ===============\n", i);
		smi_dumpDebugMsg();
	}
#endif

	return isSMIHang;
}

uint64_t cmdq_rec_flag_from_scenario(CMDQ_SCENARIO_ENUM scn)
{
	uint64_t flag = 0;

	switch (scn) {
	case CMDQ_SCENARIO_PRIMARY_DISP:
	case CMDQ_SCENARIO_PRIMARY_MEMOUT:
	case CMDQ_SCENARIO_PRIMARY_ALL:
		flag = (1LL << CMDQ_ENG_FAKE_DISP_OVL0);
		break;
	case CMDQ_SCENARIO_SUB_DISP:
		flag = (1LL << CMDQ_ENG_FAKE_DISP_OVL1);
		break;
	case CMDQ_SCENARIO_TRIGGER_LOOP:
		/* Trigger loop does not related to any HW by itself. */
		flag = 0LL;
		break;

	case CMDQ_SCENARIO_USER_SPACE:
		/* user space case, engine flag is passed seprately */
		flag = 0LL;
		break;

	case CMDQ_SCENARIO_DEBUG:
	case CMDQ_SCENARIO_DEBUG_PREFETCH:
		flag = 0LL;
		break;

	default:
		CMDQ_ERR("Unknown scenario type %d\n", scn);
		flag = 0LL;
		break;
	}

	return flag;
}

void cmdq_core_gpr_dump(void)
{
	/* do nothing becuase not support GPR in this platform */
	return;
}

void cmdq_test_setup(void)
{
	return; /* do nothing */
}

void cmdq_test_cleanup(void)
{
	return; /* do nothing */
}

extern void testcase_clkmgr_impl(enum cg_clk_id gateId,
			const char *name,
			const uint32_t testWriteReg,
			const uint32_t testWriteValue,
			const uint32_t testReadReg,
			const bool verifyWriteResult);

void testcase_clkmgr_cmdq(void)
{
	testcase_clkmgr_impl(MT_CG_DISP0_MM_CMDQ,
			"CMDQ_TEST",
			CMDQ_THR_EXEC_CNT(14),
			0xFFFFDEAD,
			CMDQ_THR_EXEC_CNT(14),
			true);
}
