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

#ifndef __MT_UNCOMPRESS_H
#define __MT_UNCOMPRESS_H

#include <asm/mach/arch.h>
#include <asm/mach/time.h>
#include <asm/mach/map.h>
#include <asm/mach-types.h>
#include <asm/page.h>
#include "mt_reg_base.h"

#define MT_UART_PHY_BASE IO_VIRT_TO_PHYS(UART1_BASE)

#define MT_UART_LSR *(volatile unsigned char *)(MT_UART_PHY_BASE+0x14)
#define MT_UART_THR *(volatile unsigned char *)(MT_UART_PHY_BASE+0x0)
#define MT_UART_LCR *(volatile unsigned char *)(MT_UART_PHY_BASE+0xc)
#define MT_UART_DLL *(volatile unsigned char *)(MT_UART_PHY_BASE+0x0)
#define MT_UART_DLH *(volatile unsigned char *)(MT_UART_PHY_BASE+0x4)
#define MT_UART_FCR *(volatile unsigned char *)(MT_UART_PHY_BASE+0x8)
#define MT_UART_MCR *(volatile unsigned char *)(MT_UART_PHY_BASE+0x10)
#define MT_UART_SPEED *(volatile unsigned char *)(MT_UART_PHY_BASE+0x24)


static void arch_decomp_setup(void)
{
    unsigned char tmp __attribute__((unused));

#if defined(CONFIG_MT_FPGA)
    MT_UART_LCR = 0x3;
    tmp = MT_UART_LCR;
    MT_UART_LCR = (tmp | 0x80);
    MT_UART_SPEED = 0x0;
    MT_UART_DLL = 0x0E;
    MT_UART_DLH = 0;
    MT_UART_LCR = tmp;
    MT_UART_FCR = 0x0047;
    MT_UART_MCR = (0x1 | 0x2);
#endif
}

/*
 * This does not append a newline
 */
static inline void putc(int c)
{
    while (!(MT_UART_LSR & 0x20));    
    MT_UART_THR = c;        
}

static inline void flush(void)
{
}

/*
 * nothing to do
 */
#define arch_decomp_wdog()

#endif

