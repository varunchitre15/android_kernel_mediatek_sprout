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

#ifndef _MTK_LINUX_ION_H
#define _MTK_LINUX_ION_H

/* #include <linux/ion.h> */

enum mtk_ion_heap_type {
	/* ION_HEAP_TYPE_MULTIMEDIA = ION_HEAP_TYPE_CUSTOM + 1, */
	ION_HEAP_TYPE_MULTIMEDIA = 10,
};

#define ION_HEAP_MULTIMEDIA_MASK    (1 << ION_HEAP_TYPE_MULTIMEDIA)

#define ION_NUM_HEAP_IDS		sizeof(unsigned int) * 8

#endif				/* _MTK_LINUX_ION_H */
