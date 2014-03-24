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

#ifndef _INNODEV_REGS_V2_H_
#define _INNODEV_REGS_V2_H_

// V1 REGs Bank 0

// V1 REGs Bank 1

// V1 REGs Bank 2
#define MMIS_MODE							0x200
#define MMIS_CONFIG							0x201
#define INT_ENABLE							0x202	/* MMIS interrupt enable */
#define INT_STATUS							0x203	/* MMIS interrupt status */

#define LG0_LEN_LOW						0x243
#define LG0_LEN_MID							0x244
#define LG0_LEN_HIGH						0x245
#define LG1_LEN_LOW						0x24b
#define LG1_LEN_MID							0x24c
#define LG1_LEN_HIGH						0x24d


// V1 REGs Bank 3

/////////////////////////
#define MMIS_CONFIG_SCLK_POL				(0x1<<0)
#define MMIS_CONFIG_SCLK_PHA				(0x1<<1)
#define MMIS_CONFIG_DATA_LENGHT_MASK	(0x3<<3)

#endif	//_INNODEV_REGS_V2_H_
