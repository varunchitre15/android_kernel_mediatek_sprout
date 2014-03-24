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

/******************************************************************************
 *  INCLUDE LIBRARY
 ******************************************************************************/
#include "sec_boot_lib.h"

/******************************************************************************
 * CONSTANT DEFINITIONS                                                       
 ******************************************************************************/
#define MOD                         "ASF"

/******************************************************************************
 *  EXTERNAL VARIABLES
 ******************************************************************************/
extern uchar                        sha1sum[];

/******************************************************************************
 *  EXTERNAL FUNCTION
 ******************************************************************************/

/**************************************************************************
 *  INTERNAL UTILITES
 **************************************************************************/
void * mcpy(void *dest, const void *src, int  cnt)
{
    char *tmp = dest;
    const char *s = src;

    while (cnt--)
        *tmp++ = *s++;
    return dest;
}

int mcmp (const void *cs, const void *ct, int cnt)
{
    const uchar *su1, *su2;
    int res = 0;

    for (su1 = cs, su2 = ct; 0 < cnt; ++su1, ++su2, cnt--)
        if ((res = *su1 - *su2) != 0)
            break;
    return res;
}

void dump_buf(uchar* buf, uint32 len)
{
    uint32 i = 1;

    for (i =1; i <len+1; i++)
    {   
        if(0 != buf[i-1])
        {
            SMSG(true,"0x%x,",buf[i-1]);
            
            if(0 == i%8)        
            {
                SMSG(true,"\n");
            }
        }
    }
    
    SMSG(true,"\n");    
}

/******************************************************************************
 *  IMAGE HASH DUMP
 ******************************************************************************/
void img_hash_dump (uchar *buf, uint32 size)
{
    uint32 i = 0;

    for (i = 0 ; i < size ; i++)
    {
        if(i % 4 ==0)
        {   
            SMSG(true,"\n");
        }

        if(buf[i] < 0x10)
        {
            SMSG(true,"0x0%x, ",buf[i]);
        }
        else
        {   
            SMSG(true,"0x%x, ",buf[i]);
        }
    }
}

/******************************************************************************
 *  IMAGE HASH CALCULATION
 ******************************************************************************/
void img_hash_compute (uchar *buf, uint32 size)
{
    SEC_ASSERT(0);
}

/******************************************************************************
 *  IMAGE HASH UPDATE
 ******************************************************************************/
uint32 img_hash_update (char* part_name)
{
    uint32 ret = SEC_OK;
    
    SEC_ASSERT(0);

    return ret;
}


/******************************************************************************
 *  IMAGE HASH CHECK
 ******************************************************************************/
uint32 img_hash_check (char* part_name)
{
    uint32 ret = SEC_OK;
    
    SEC_ASSERT(0);
    
    return ret;
}


/******************************************************************************
 *  GET BUILD INFORMATION
 ******************************************************************************/
char* asf_get_build_info(void)
{
    return BUILD_TIME""BUILD_BRANCH;
}

