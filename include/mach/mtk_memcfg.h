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

#ifndef __MTK_MEMCFG_H__
#define __MTK_MEMCFG_H__

#ifdef CONFIG_MTK_MEMCFG

#define MTK_MEMCFG_LOG_AND_PRINTK(fmt, arg...)  \
    do {    \
        printk(fmt, ##arg); \
        mtk_memcfg_write_memory_layout_buf(fmt, ##arg); \
    } while (0)

extern void mtk_memcfg_write_memory_layout_buf(char *, ...); 
extern unsigned long mtk_memcfg_get_force_inode_gfp_lowmem(void);
extern unsigned long mtk_memcfg_set_force_inode_gfp_lowmem(unsigned long);
extern void mtk_memcfg_late_warning(void);
#ifdef CONFIG_SLUB_DEBUG
extern unsigned long mtk_memcfg_get_bypass_slub_debug_flag(void);
extern unsigned long mtk_memcfg_set_bypass_slub_debug_flag(unsigned long);
#else
#define mtk_memcfg_get_bypass_slub_debug_flag()  do { } while (0)
#define mtk_memcfg_set_bypass_slub_debug_flag(flag)  do { } while (0)

#endif /* end CONFIG_SLUB_DEBUG */

#else

#define MTK_MEMCFG_LOG_AND_PRINTK(fmt, arg...)  \
    do {    \
        printk(fmt, ##arg); \
    } while (0)

#define mtk_memcfg_get_force_inode_gfp_lowmem()  do { } while (0)
#define mtk_memcfg_set_force_inode_gfp_lowmem(flag)  do { } while (0)
#define mtk_memcfg_get_bypass_slub_debug_flag()  do { } while (0)
#define mtk_memcfg_set_bypass_slub_debug_flag(flag)  do { } while (0)
#define mtk_memcfg_write_memory_layout_buf(fmt, arg...) do { } while (0)
#define mtk_memcfg_late_warning() do { } while (0)

#endif /* end CONFIG_MTK_MEMCFG */

#endif /* end __MTK_MEMCFG_H__ */
