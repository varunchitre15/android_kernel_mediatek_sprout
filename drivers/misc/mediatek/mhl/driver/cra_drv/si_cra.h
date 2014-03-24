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

#ifndef __SI_CRA_H__
#define __SI_CRA_H__
#include "si_common.h"
typedef uint16_t    SiiReg_t;
bool_t              SiiCraInitialize( void );
SiiResultCodes_t    SiiCraGetLastResult( void );
bool_t              SiiRegInstanceSet( SiiReg_t virtualAddress, prefuint_t newInstance );
void    SiiRegReadBlock ( SiiReg_t virtualAddr, uint8_t *pBuffer, uint16_t count );
uint8_t SiiRegRead ( SiiReg_t virtualAddr );
void    SiiRegWriteBlock ( SiiReg_t virtualAddr, const uint8_t *pBuffer, uint16_t count );
void    SiiRegWrite ( SiiReg_t virtualAddr, uint8_t value );
void    SiiRegModify ( SiiReg_t virtualAddr, uint8_t mask, uint8_t value);
void    SiiRegBitsSet ( SiiReg_t virtualAddr, uint8_t bitMask, bool_t setBits );
void    SiiRegBitsSetNew ( SiiReg_t virtualAddr, uint8_t bitMask, bool_t setBits );
void    SiiRegEdidReadBlock ( SiiReg_t segmentAddr, SiiReg_t virtualAddr, uint8_t *pBuffer, uint16_t count );
#endif  
