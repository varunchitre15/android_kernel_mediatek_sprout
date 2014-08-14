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

#ifndef __MT_REG_DUMP_H
#define __MT_REG_DUMP_H

#define CORE0_PC (MCUSYS_CFGREG_BASE + 0x300)
#define CORE0_FP (MCUSYS_CFGREG_BASE + 0x304)
#define CORE0_SP (MCUSYS_CFGREG_BASE + 0x308)
#define CORE1_PC (MCUSYS_CFGREG_BASE + 0x310)
#define CORE1_FP (MCUSYS_CFGREG_BASE + 0x314)
#define CORE1_SP (MCUSYS_CFGREG_BASE + 0x318)
#define CORE2_PC (MCUSYS_CFGREG_BASE + 0x320)
#define CORE2_FP (MCUSYS_CFGREG_BASE + 0x324)
#define CORE2_SP (MCUSYS_CFGREG_BASE + 0x328)
#define CORE3_PC (MCUSYS_CFGREG_BASE + 0x330)
#define CORE3_FP (MCUSYS_CFGREG_BASE + 0x334)
#define CORE3_SP (MCUSYS_CFGREG_BASE + 0x338)

struct mt_reg_dump {
	unsigned int pc;
	unsigned int fp;
	unsigned int sp;
	unsigned int core_id;
};

extern int mt_reg_dump(char *buf);

#endif

