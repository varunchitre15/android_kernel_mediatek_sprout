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

#ifndef MTD_UTILS_H
#define MTD_UTILS_H

/**************************************************************************
*  PARTITION RECORD
**************************************************************************/
typedef struct _MtdPart 
{
    char name[16];
    unsigned int sz;    
    unsigned int off;

} MtdPart;

/**************************************************************************
*  MTD CONFIGURATION
**************************************************************************/
/* partition table read from /proc/mtd */
#define MAX_MTD_PARTITIONS              (25)

/* search region and off */
//#ifdef EMMC_PROJECT
/* work for nand and emmc */
#define ROM_INFO_SEARCH_START           (0x0)
//#else
//#define ROM_INFO_SEARCH_START         (0x20000)
//#endif

/**************************************************************************
 *  EXPORT VARIABLES
 **************************************************************************/
extern MtdPart                          mtd_part_map[];

/**************************************************************************
 *  UTILITY
 **************************************************************************/
char* mtd2pl (char* part_name);
char* pl2mtd (char* part_name);

#endif  // MTD_UTILS_H
