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

#ifndef _CONFIG_H_
#define _CONFIG_H_

#define ETM_NO (4)
#define ETM0_BASE ( 0x1017C000)
#define ETM1_BASE ( 0x1017D000)
#define ETM2_BASE ( 0x1017E000)
#define ETM3_BASE ( 0x1017F000)
//#define ETB_BASE        (0xF0111000)
//#define ETB_BASE_PHY (0x10111000)
/* ETR base */
#define ETB_BASE_PHY (0x10113000)

#define ETR_BASE_PHY (0x10113000)
#define FUNNEL_BASE_PHY	(0x10114000)
//Virtual addr
#define DEM_Unlock (0xF011afb0)
#define DEM_Reset (0xF011a028)

#endif

