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

#if !defined(__ION_PROFILE_H__)
#define __ION_PROFILE_H__
#include <linux/mmprofile.h>

typedef enum {
    PROFILE_ALLOC=0,
    PROFILE_FREE,
    PROFILE_SHARE,
    PROFILE_IMPORT,
    PROFILE_MAP_KERNEL,
    PROFILE_UNMAP_KERNEL,
    PROFILE_MAP_USER,
    PROFILE_UNMAP_USER,
    PROFILE_CUSTOM_IOC,
    PROFILE_GET_PHYS,
    PROFILE_DMA_CLEAN_RANGE,
    PROFILE_DMA_FLUSH_RANGE,
    PROFILE_DMA_INVALID_RANGE,
    PROFILE_DMA_CLEAN_ALL,
    PROFILE_DMA_FLUSH_ALL,
    PROFILE_DMA_INVALID_ALL,
    PROFILE_MAX,
}ION_PROFILE_TYPE;

extern MMP_Event ION_MMP_Events[PROFILE_MAX];

extern void ion_profile_init(void);


#endif 

