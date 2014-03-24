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

#include <asm/page.h>
#include <linux/module.h> 

// return the size of memory from start pfn to max pfn,
// _NOTE_
// the memory area may be discontinuous
extern unsigned long max_pfn;
unsigned int get_memory_size (void)
{
    return (unsigned int)(max_pfn << PAGE_SHIFT);
}
EXPORT_SYMBOL(get_memory_size);

// return the actual physical DRAM size
// wrapper function of mtk_get_max_DRAM_size
extern unsigned int mtk_get_max_DRAM_size(void);
unsigned int get_max_DRAM_size(void)
{
        return mtk_get_max_DRAM_size();
}
EXPORT_SYMBOL(get_max_DRAM_size);
