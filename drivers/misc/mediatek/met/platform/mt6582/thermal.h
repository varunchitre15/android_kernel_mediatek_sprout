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

#ifndef _THERMAL_H_

struct th_zone {
	char path[256];
	char name[32];
	int  tm;
};


typedef enum
{
    MTK_THERMAL_SENSOR_CPU = 0,
    MTK_THERMAL_SENSOR_ABB,
    MTK_THERMAL_SENSOR_PMIC,
    MTK_THERMAL_SENSOR_BATTERY,
    MTK_THERMAL_SENSOR_MD1,
    MTK_THERMAL_SENSOR_MD2,
    MTK_THERMAL_SENSOR_WIFI,

    MTK_THERMAL_SENSOR_COUNT
} MTK_THERMAL_SENSOR_ID;

#if NO_MTK_THERMAL_GET_TEMP == 0
extern int mtk_thermal_get_temp(MTK_THERMAL_SENSOR_ID id);
#endif

#define _THERMAL_H_


#endif // _THERMAL_H_
