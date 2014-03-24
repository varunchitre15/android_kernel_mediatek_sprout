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

#ifndef __SI_CRA_DRV_INTERNAL_H__
#define __SI_CRA_DRV_INTERNAL_H__
typedef enum _SiiDrvCraError_t
{
    RESULT_CRA_SUCCESS,             
    RESULT_CRA_FAIL,                
    RESULT_CRA_INVALID_PARAMETER,   
} SiiDrvCraError_t;
typedef struct _CraInstanceData_t
{
    int                 structVersion;
    int                 instanceIndex;
    SiiDrvCraError_t    lastResultCode;     
    uint16_t            statusFlags;
}	CraInstanceData_t;
extern CraInstanceData_t craInstance;
#endif 
