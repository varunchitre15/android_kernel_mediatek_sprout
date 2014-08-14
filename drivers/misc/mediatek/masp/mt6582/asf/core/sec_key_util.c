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

#include "sec_typedef.h"
#include "sec_boot.h"

/**************************************************************************
 *  DEFINITIONS
 **************************************************************************/
#define MOD                         "SEC_KEY_UTIL"

/**************************************************************************
 *  KEY SECRET
 **************************************************************************/
#define ENCODE_MAGIC                (0x1)

void sec_decode_key(uchar* key, uint32 key_len, uchar* seed, uint32 seed_len)
{
    uint32 i = 0;

    for(i=0; i<key_len; i++)
    {
        key[i] -= seed[i%seed_len];
        key[i] -= ENCODE_MAGIC;
    }
}
