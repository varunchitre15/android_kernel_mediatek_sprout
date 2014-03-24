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

#ifndef _CAM_CAL_DATA_H
#define _CAM_CAL_DATA_H


typedef struct{
    u32 u4Offset;
    u32 u4Length;
    u8 *  pu1Params;
}stCAM_CAL_INFO_STRUCT, *stPCAM_CAL_INFO_STRUCT;
#endif //_CAM_CAL_DATA_H
