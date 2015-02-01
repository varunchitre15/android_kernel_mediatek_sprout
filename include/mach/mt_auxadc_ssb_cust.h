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

#ifndef _MT_AUXADC_SSB_CUST_H_
#define _MT_AUXADC_SSB_CUST_H_

struct tag_para_auxadc_ssb_data
{
    int version;
    int TEMPERATURE_CHANNEL;
    int ADC_FDD_RF_PARAMS_DYNAMIC_CUSTOM_CH_CHANNEL;
    int HF_MIC_CHANNEL;
    int LCM_VOLTAGE;
    int endflag;
};

#endif

extern struct tag_para_auxadc_ssb_data auxadc_cust_ssb_data;


