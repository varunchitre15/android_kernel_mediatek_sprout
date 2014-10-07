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

#ifndef __CMDQ_DEVICE_H__
#define __CMDQ_DEVICE_H__

#include <linux/platform_device.h>
#include <linux/device.h>

struct device *cmdq_dev_get(void);
const uint32_t cmdq_dev_get_irq_id(void);
const uint32_t cmdq_dev_get_irq_secure_id(void);

const long cmdq_dev_alloc_module_base_VA_by_name(const char *name);
void cmdq_dev_free_module_base_VA(const long VA);

void cmdq_dev_init(struct platform_device *pDevice);
void cmdq_dev_deinit(void);

#endif				/* __CMDQ_DEVICE_H__ */
