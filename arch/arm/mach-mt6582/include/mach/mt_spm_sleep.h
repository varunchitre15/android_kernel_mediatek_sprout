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

#ifndef _MT_SPM_SLEEP_
#define _MT_SPM_SLEEP_

#include <linux/kernel.h>

typedef enum {
    WR_NONE         = 0,
    WR_UART_BUSY    = 1,
    WR_PCM_ASSERT   = 2,
    WR_PCM_TIMER    = 3,
    WR_PCM_WDT      = 4,
    WR_WAKE_SRC     = 5,
    WR_UNKNOWN      = 6,
} wake_reason_t;

/*
 * for suspend
 */
extern int spm_set_sleep_wakesrc(u32 wakesrc, bool enable, bool replace);
extern wake_reason_t spm_go_to_sleep(bool cpu_pdn, bool infra_pdn, int pwake_time);
extern wake_reason_t spm_go_to_sleep_dpidle(bool cpu_pdn, u16 pwrlevel, int pwake_time);


/*
 * for deep idle
 */
extern void spm_dpidle_before_wfi(void);        /* can be redefined */
extern void spm_dpidle_after_wfi(void);         /* can be redefined */
extern wake_reason_t spm_go_to_dpidle(bool cpu_pdn, u16 pwrlevel);


extern bool spm_is_md_sleep(void);
extern bool spm_is_conn_sleep(void);

extern void spm_output_sleep_option(void);

#endif
