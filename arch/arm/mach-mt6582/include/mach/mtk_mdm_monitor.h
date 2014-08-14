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

#ifndef _MTK_MDM_MONITOR_H
#define _MTK_MDM_MONITOR_H

struct md_info{
		char *attribute;
		int value;
		char *unit;
		int invalid_value;
		int index;
};

extern
int mtk_mdm_get_md_info(
	struct md_info** p_inf, 
	int *size
);

extern
int mtk_mdm_start_query(void);

extern
int mtk_mdm_stop_query(void);

extern 
int mtk_mdm_set_signal_period(int second);

extern 
int mtk_mdm_set_md1_signal_period(int second);

extern 
int mtk_mdm_set_md2_signal_period(int second);
#endif 
