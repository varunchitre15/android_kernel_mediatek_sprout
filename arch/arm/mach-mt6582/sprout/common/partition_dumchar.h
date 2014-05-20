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

#include <linux/module.h>
#include "partition_define.h"
struct excel_info PartInfo[PART_NUM]={
			{"preloader",6291456,0x0, EMMC, 0,BOOT_1},
			{"mbr",524288,0x600000, EMMC, 0,USER},
			{"ebr1",524288,0x680000, EMMC, 1,USER},
			{"pro_info",3145728,0x700000, EMMC, 0,USER},
			{"nvram",5242880,0xa00000, EMMC, 0,USER},
			{"protect_f",10485760,0xf00000, EMMC, 2,USER},
			{"protect_s",10485760,0x1900000, EMMC, 3,USER},
			{"seccfg",131072,0x2300000, EMMC, 0,USER},
			{"uboot",393216,0x2320000, EMMC, 0,USER},
			{"bootimg",6291456,0x2380000, EMMC, 0,USER},
			{"recovery",6291456,0x2980000, EMMC, 0,USER},
			{"sec_ro",6291456,0x2f80000, EMMC, 4,USER},
			{"misc",524288,0x3580000, EMMC, 0,USER},
			{"logo",3145728,0x3600000, EMMC, 0,USER},
			{"ebr2",524288,0x3900000, EMMC, 0,USER},
			{"expdb",10485760,0x3980000, EMMC, 0,USER},
			{"android",734003200,0x4380000, EMMC, 5,USER},
			{"cache",132120576,0x2ff80000, EMMC, 6,USER},
			{"usrdata",1073741824,0x37d80000, EMMC, 7,USER},
			{"fat",0,0x77d80000, EMMC, 8,USER},
			{"bmtpool",22020096,0xFFFF00a8, EMMC, 0,USER},
 };
EXPORT_SYMBOL(PartInfo);

#ifdef  CONFIG_MTK_EMMC_SUPPORT
struct MBR_EBR_struct MBR_EBR_px[MBR_COUNT]={
	{"mbr", {1, 2, 3, 4, }},
	{"ebr1", {5, 6, 7, }},
	{"ebr2", {8, }},
};

EXPORT_SYMBOL(MBR_EBR_px);
#endif

