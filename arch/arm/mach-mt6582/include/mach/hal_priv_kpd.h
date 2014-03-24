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

#ifndef KPD_PRIV_H
#define KPD_PRIV_H

/* Keypad registers */
#define KP_STA			(KP_BASE + 0x0000)
#define KP_MEM1			(KP_BASE + 0x0004)
#define KP_MEM2			(KP_BASE + 0x0008)
#define KP_MEM3			(KP_BASE + 0x000c)
#define KP_MEM4			(KP_BASE + 0x0010)
#define KP_MEM5			(KP_BASE + 0x0014)
#define KP_DEBOUNCE		(KP_BASE + 0x0018)
#define KP_SCAN_TIMING		(KP_BASE + 0x001C)
#define KP_SEL			(KP_BASE + 0x0020)
#define KP_EN			(KP_BASE + 0x0024)

#define KPD_DEBOUNCE_MASK	((1U << 14) - 1)

#endif
