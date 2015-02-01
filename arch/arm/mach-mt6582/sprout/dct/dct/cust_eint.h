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

#ifndef __CUST_EINTH
#define __CUST_EINTH
#ifdef __cplusplus
extern "C" {
#endif

#define CUST_EINTF_TRIGGER_RISING						 1
#define CUST_EINTF_TRIGGER_FALLING                      2
#define CUST_EINTF_TRIGGER_HIGH                         4
#define CUST_EINTF_TRIGGER_LOW                          8



#define CUST_EINT_DEBOUNCE_DISABLE          0
#define CUST_EINT_DEBOUNCE_ENABLE           1
////////////////////////////////////////////////////

struct cust_eint{
    char name[64];
    int num;
	int debounce_cn;
    int type;
    int debounce_en;
};

extern struct cust_eint cust_eint_chr_stat;

#define CUST_EINT_CHR_STAT_NUM					cust_eint_chr_stat.num
#define CUST_EINT_CHR_STAT_DEBOUNCE_CN			cust_eint_chr_stat.debounce_cn
#define CUST_EINT_CHR_STAT_TYPE					cust_eint_chr_stat.type
#define CUST_EINT_CHR_STAT_DEBOUNCE_EN			cust_eint_chr_stat.debounce_en

extern struct cust_eint cust_eint_als;

#define CUST_EINT_ALS_NUM					cust_eint_als.num
#define CUST_EINT_ALS_DEBOUNCE_CN			cust_eint_als.debounce_cn
#define CUST_EINT_ALS_TYPE					cust_eint_als.type
#define CUST_EINT_ALS_DEBOUNCE_EN			cust_eint_als.debounce_en

extern struct cust_eint cust_eint_touch_panel;

#define CUST_EINT_TOUCH_PANEL_NUM					cust_eint_touch_panel.num
#define CUST_EINT_TOUCH_PANEL_DEBOUNCE_CN			cust_eint_touch_panel.debounce_cn
#define CUST_EINT_TOUCH_PANEL_TYPE					cust_eint_touch_panel.type
#define CUST_EINT_TOUCH_PANEL_DEBOUNCE_EN			cust_eint_touch_panel.debounce_en

extern struct cust_eint cust_eint_gse_2;

#define CUST_EINT_GSE_2_NUM					cust_eint_gse_2.num
#define CUST_EINT_GSE_2_DEBOUNCE_CN			cust_eint_gse_2.debounce_cn
#define CUST_EINT_GSE_2_TYPE					cust_eint_gse_2.type
#define CUST_EINT_GSE_2_DEBOUNCE_EN			cust_eint_gse_2.debounce_en

extern struct cust_eint cust_eint_accdet;

#define CUST_EINT_ACCDET_NUM					cust_eint_accdet.num
#define CUST_EINT_ACCDET_DEBOUNCE_CN			cust_eint_accdet.debounce_cn
#define CUST_EINT_ACCDET_TYPE					cust_eint_accdet.type
#define CUST_EINT_ACCDET_DEBOUNCE_EN			cust_eint_accdet.debounce_en

extern struct cust_eint cust_eint_gse_1;

#define CUST_EINT_GSE_1_NUM					cust_eint_gse_1.num
#define CUST_EINT_GSE_1_DEBOUNCE_CN			cust_eint_gse_1.debounce_cn
#define CUST_EINT_GSE_1_TYPE					cust_eint_gse_1.type
#define CUST_EINT_GSE_1_DEBOUNCE_EN			cust_eint_gse_1.debounce_en

extern struct cust_eint cust_eint_mt6323_pmic;

#define CUST_EINT_MT6323_PMIC_NUM					cust_eint_mt6323_pmic.num
#define CUST_EINT_MT6323_PMIC_DEBOUNCE_CN			cust_eint_mt6323_pmic.debounce_cn
#define CUST_EINT_MT6323_PMIC_TYPE					cust_eint_mt6323_pmic.type
#define CUST_EINT_MT6323_PMIC_DEBOUNCE_EN			cust_eint_mt6323_pmic.debounce_en

////////////////////////////////////////////////////

#ifdef __cplusplus
}

#endif
#endif
