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

#ifndef __ARCH_ARM_MACH_MT6573_INCLUDE_MACHINE_MT6573_BT_H
#define __ARCH_ARM_MACH_MT6573_INCLUDE_MACHINE_MT6573_BT_H

#include <linux/types.h>
#include <linux/rfkill.h>

#define MT_BT_OK    (0)
#define MT_BT_FAIL  (-1)

typedef void (*btpm_handler_t)(void*);  /* external irq handler */

extern void mt_bt_power_on(void);
extern void mt_bt_power_off(void);
extern int mt_bt_suspend(pm_message_t state);
extern int mt_bt_resume(pm_message_t state);

extern void mt_bt_eirq_handler(void* par);
extern void mt_bt_pm_init(void* hdev);
extern void mt_bt_pm_deinit(void* hdev);

extern void mt_bt_enable_irq(void);
extern void mt_bt_disable_irq(void);

#endif

