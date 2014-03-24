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

#ifndef __MT_DEVINFO_H
#define __MT_DEVFINO_H

#include <linux/types.h>

/*device information data*/
#define ATAG_DEVINFO_DATA 0x41000804
#define ATAG_DEVINFO_DATA_SIZE 22

struct tag_devinfo_data {
    u32 devinfo_data[ATAG_DEVINFO_DATA_SIZE]; 	/* device information */
    u32 devinfo_data_size;                      /* device information size */
};

#endif