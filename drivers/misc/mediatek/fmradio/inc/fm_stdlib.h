/* 
 * (C) Copyright 2014
 * MediaTek <www.MediaTek.com>
 * Run <Run.Liu@MediaTek.com>
 *
 * FM Radio Driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __FM_STDLIB_H__
#define __FM_STDLIB_H__

#include "fm_typedef.h"
#include <linux/string.h>

#if 1
#define fm_memset(buf, a, len)  \
({                                    \
    void *__ret = (void*)0;              \
    __ret = memset((buf), (a), (len)); \
    __ret;                          \
})

#define fm_memcpy(dst, src, len)  \
({                                    \
    void *__ret = (void*)0;              \
    __ret = memcpy((dst), (src), (len)); \
    __ret;                          \
})

#define fm_malloc(len)  \
({                                    \
    void *__ret = (void*)0;              \
    __ret = kmalloc(len, GFP_KERNEL); \
    __ret;                          \
})

#define fm_zalloc(len)  \
({                                    \
    void *__ret = (void*)0;              \
    __ret = kzalloc(len, GFP_KERNEL); \
    __ret;                          \
})

#define fm_free(ptr)  kfree(ptr)

#define fm_vmalloc(len)  \
({                                    \
    void *__ret = (void*)0;              \
    __ret = vmalloc(len); \
    __ret;                          \
})

#define fm_vfree(ptr)  vfree(ptr)

#else
inline void* fm_memset(void *buf, fm_s8 val, fm_s32 len)
{
    return memset(buf, val, len);
}

inline void* fm_memcpy(void *dst, const void *src, fm_s32 len)
{
    return memcpy(dst, src, len);
}

#endif

#endif //__FM_STDLIB_H__

