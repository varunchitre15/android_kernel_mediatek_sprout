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

/**************************************************************************
 *  INCLUDE LIBRARY
 **************************************************************************/
#include "sec_boot_lib.h"

/**************************************************************************
 *  MACRO
 *************************************************************************/
#define MOD                                "ASF.DEV"

/**************************************************************************
 *  EXTERNAL VARIABLES
 *************************************************************************/
extern SECURE_INFO                         sec_info;

/**************************************************************************
 *  PART NAME QUERY (MTD OR USIF)
 **************************************************************************/
char* part2pl (char* part_name)
{

    if(TRUE == sec_usif_enabled())
    {
        return usif2pl(part_name);
    }
    else
    {
        return mtd2pl(part_name);
    }
}

char* pl2part (char* part_name)
{

    if(TRUE == sec_usif_enabled())
    {
        return pl2usif(part_name);
    }
    else
    {
        return pl2mtd(part_name);
    }
}

/**************************************************************************
 *  GET ANDROID NAME
 **************************************************************************/
char* get_android_name(void)
{
    return pl2part(mtd2pl(MTD_ANDSYSIMG));
}

/**************************************************************************
 *  GET SECRO NAME
 **************************************************************************/
char* get_secro_name(void)
{
    return pl2part(mtd2pl(MTD_SECRO));
}

