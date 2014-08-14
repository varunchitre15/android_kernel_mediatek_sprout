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

#ifndef _MTK_ADC_HW_H
#define _MTK_ADC_HW_H

#include <mach/mt_reg_base.h>

#define AUXADC_CON0                     (AUXADC_BASE + 0x000)
#define AUXADC_CON1                     (AUXADC_BASE + 0x004)
#define AUXADC_CON2                     (AUXADC_BASE + 0x010)
#define AUXADC_DAT0                     (AUXADC_BASE + 0x014)
#define AUXADC_TP_CMD            (AUXADC_BASE + 0x005c)
#define AUXADC_TP_ADDR           (AUXADC_BASE + 0x0060)
#define AUXADC_TP_CON0           (AUXADC_BASE + 0x0064)
#define AUXADC_TP_DATA0          (AUXADC_BASE + 0x0074)

#define PAD_AUX_XP				13
#define TP_CMD_ADDR_X			0x0005

#define AUXADC_CON_RTP		(APMIXEDSYS_BASE + 0x0404)

#endif   /*_MTK_ADC_HW_H*/

