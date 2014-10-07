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

#include "cmdq_device.h"
#include "cmdq_core.h"
#include <mach/mt_irq.h>

#if 0
/* device tree */
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/io.h>
#endif

typedef struct CmdqDeviceStruct {
	struct device *pDev;
#if 0
	long regBaseVA;		/* considering 64 bit kernel, use long */
	long regBasePA;
	uint32_t irqId;
	uint32_t irqSecId;
#endif
} CmdqDeviceStruct;

static CmdqDeviceStruct gCmdqDev;

struct device *cmdq_dev_get(void)
{
	return gCmdqDev.pDev;
}

const uint32_t cmdq_dev_get_irq_id(void)
{
	return MT6582_DISP_CMDQ_IRQ_ID;
}

const uint32_t cmdq_dev_get_irq_secure_id(void)
{
	return MT6582_DISP_CMDQ_SECURE_IRQ_ID;
}

void cmdq_dev_init_module_base_VA(void)
{
	return;
}

void cmdq_dev_deinit_module_base_VA(void)
{
	return;
}

void cmdq_dev_init(struct platform_device *pDevice)
{
	memset(&gCmdqDev, 0x0, sizeof(CmdqDeviceStruct));

	if (pDevice) {
		gCmdqDev.pDev = &pDevice->dev;
	}

	return;
}

void cmdq_dev_deinit(void)
{
	return;
}
