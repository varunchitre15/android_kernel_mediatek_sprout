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

#ifndef _MT_DORMANT_
#define _MT_DORMANT_

#define MAX_CPUS 1

/* 
 * Power status
 */
#define STATUS_RUN      0
#define STATUS_STANDBY  1
#define STATUS_DORMANT  2
#define STATUS_SHUTDOWN 3
#define DORMANT_MODE  STATUS_DORMANT
#define SHUTDOWN_MODE STATUS_SHUTDOWN

/* 
 * switch to amp / smp
 */
#define switch_to_amp()                         \
do {                                            \
    isb();                                      \
    dsb();                                      \
    asm volatile(                               \
        "MRC p15,0,r0,c1,c0,1\n"                \
        "BIC r0,r0,#0x00000040\n"               \
        "MCR p15,0,r0,c1,c0,1\n"                \
        :                                       \
        :                                       \
        :"r0"                                   \
    );                                          \
    isb();                                      \
    dsb();                                      \
} while (0)

#define switch_to_smp()                         \
do {                                            \
    isb();                                      \
    dsb();                                      \
    asm volatile(                               \
        "MRC p15,0,r0,c1,c0,1\n"                \
        "ORR r0,r0,#0x00000040\n"               \
        "MCR p15,0,r0,c1,c0,1\n"                \
        :                                       \
        :                                       \
        :"r0"                                   \
    );                                          \
    isb();                                      \
    dsb();                                      \
} while (0)

/* input value:
 * DORMANT_MODE / SHUTDOWN_MODE
 *
 * return value: 
 * 0 : execute wfi then power down
 * 1 : wake up and resume
 */
extern int cpu_power_down(int mode);
extern void cpu_dormant_init(void);
extern void cpu_check_dormant_abort(void);

#endif

