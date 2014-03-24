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

#ifndef __MTK_RAM_CONSOLE_H__
#define __MTK_RAM_CONSOLE_H__

#include <linux/console.h>

typedef enum {
	AEE_FIQ_STEP_FIQ_ISR_BASE = 1,
	AEE_FIQ_STEP_WDT_FIQ_INFO = 4,
	AEE_FIQ_STEP_WDT_FIQ_STACK,
	AEE_FIQ_STEP_WDT_FIQ_LOOP,
	AEE_FIQ_STEP_WDT_FIQ_DONE,
	AEE_FIQ_STEP_WDT_IRQ_INFO = 8,
	AEE_FIQ_STEP_WDT_IRQ_KICK,
	AEE_FIQ_STEP_WDT_IRQ_SMP_STOP,
	AEE_FIQ_STEP_WDT_IRQ_STACK,
	AEE_FIQ_STEP_WDT_IRQ_TIME,
	AEE_FIQ_STEP_WDT_IRQ_GIC,
	AEE_FIQ_STEP_WDT_IRQ_LOCALTIMER,
	AEE_FIQ_STEP_WDT_IRQ_IDLE,
	AEE_FIQ_STEP_WDT_IRQ_SCHED,
	AEE_FIQ_STEP_WDT_IRQ_DONE,
	AEE_FIQ_STEP_KE_WDT_INFO = 20,
	AEE_FIQ_STEP_KE_WDT_PERCPU,
	AEE_FIQ_STEP_KE_WDT_LOG,
	AEE_FIQ_STEP_KE_SCHED_DEBUG,
	AEE_FIQ_STEP_KE_WDT_DONE,
	AEE_FIQ_STEP_KE_IPANIC_START = 32,
	AEE_FIQ_STEP_KE_IPANIC_OOP_HEADER,
	AEE_FIQ_STEP_KE_IPANIC_DETAIL,
	AEE_FIQ_STEP_KE_IPANIC_CONSOLE,
	AEE_FIQ_STEP_KE_IPANIC_USERSPACE,
	AEE_FIQ_STEP_KE_IPANIC_ANDROID,
	AEE_FIQ_STEP_KE_IPANIC_MMPROFILE,
	AEE_FIQ_STEP_KE_IPANIC_HEADER,
	AEE_FIQ_STEP_KE_IPANIC_DONE,
} AEE_FIQ_STEP_NUM;

#ifdef CONFIG_MTK_RAM_CONSOLE

extern void aee_rr_rec_fiq_step(u8 i);
extern void aee_rr_rec_reboot_mode(u8 mode);
extern void aee_rr_rec_kdump_params(void *params);
extern void aee_rr_rec_last_irq_enter(int cpu, int irq, u64 j);
extern void aee_rr_rec_last_irq_exit(int cpu, int irq, u64 j);
extern void aee_rr_rec_last_sched_jiffies(int cpu, u64 j, const char *comm);
extern void aee_rr_rec_hoplug(int cpu, u8 data1, u8 data2);

extern void aee_sram_fiq_log(const char *msg);
extern void ram_console_write(struct console *console, const char *s, unsigned int count);
extern void aee_sram_fiq_save_bin(const char *buffer, size_t len);

#else

static inline void aee_rr_rec_fiq_step(u8 i) {}
static inline void aee_rr_rec_reboot_mode(u8 mode) {}
static inline void aee_rr_rec_kdump_params(void *params) {}
static inline void aee_rr_rec_last_irq_enter(int cpu, int irq, u64 j) {}
static inline void aee_rr_rec_last_irq_exit(int cpu, int irq, u64 j) {}
static inline void aee_rr_rec_last_sched_jiffies(int cpu, u64 j, const char *comm) {}
static inline void aee_rr_rec_hoplug(int cpu, u8 data1, u8 data2) {}

static inline void aee_sram_fiq_log(const char *msg) {}
static inline void ram_console_write(struct console *console, const char *s, unsigned int count){}
static inline void aee_sram_fiq_save_bin(unsigned char *buffer, size_t len) {}


#endif


#endif
