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

#ifndef _MTK_ADC_SW_H
#define _MTK_ADC_SW_H

#define ADC_CHANNEL_MAX 16

#define MT_PDN_PERI_AUXADC MT_CG_PERI_AUXADC

extern int IMM_auxadc_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);
extern int IMM_auxadc_GetOneChannelValue_Cali(int Channel, int*voltage);
extern void mt_auxadc_hal_init(void);
extern void mt_auxadc_hal_suspend(void);
extern void mt_auxadc_hal_resume(void);
extern int mt_auxadc_dump_register(char *buf);

#endif   /*_MTK_ADC_SW_H*/

