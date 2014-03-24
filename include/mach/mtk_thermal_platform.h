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

#ifndef _MTK_THERMAL_PLATFORM_H
#define _MTK_THERMAL_PLATFORM_H

#include <linux/thermal.h>

extern 
int mtk_thermal_get_cpu_info(
    int *nocores, 
    int **cpufreq, 
    int **cpuloading);
    
extern 
int mtk_thermal_get_gpu_info(
    int *nocores,
    int **gpufreq,
    int **gpuloading);
    
extern 
int mtk_thermal_get_batt_info(
    int *batt_voltage,
    int *batt_current,
    int *batt_temp);
    
extern 
int mtk_thermal_get_extra_info(
    int *no_extra_attr,
    char ***attr_names, 
    int **attr_values, 
    char ***attr_unit);

extern 
int mtk_thermal_force_get_batt_temp(
    void);
    

enum {
    MTK_THERMAL_SCEN_CALL = 0x1
}; 

extern 
unsigned int mtk_thermal_set_user_scenarios(
    unsigned int mask);
    
extern 
unsigned int mtk_thermal_clear_user_scenarios(
    unsigned int mask);
    

#endif // _MTK_THERMAL_PLATFORM_H
