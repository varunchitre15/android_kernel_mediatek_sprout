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

#ifndef _MT_SYNC_WRITE_H
#define _MT_SYNC_WRITE_H

#if defined(__KERNEL__)

#include <linux/io.h>
#include <asm/cacheflush.h>
#include <asm/system.h>

/*
 * Define macros.
 */

#define mt65xx_reg_sync_writel(v, a) \
        do {    \
            writel((v), (void*)(a));   \
            dsb();  \
        } while (0)

#define mt65xx_reg_sync_writew(v, a) \
        do {    \
            writew((v), (a));   \
            dsb();  \
        } while (0)

#define mt65xx_reg_sync_writeb(v, a) \
        do {    \
            writeb((v), (a));   \
            dsb();  \
        } while (0)

#else   /* __KERNEL__ */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define dsb()   \
        do {    \
            __asm__ __volatile__ ("dsb" : : : "memory"); \
        } while (0)

#define mt65xx_reg_sync_writel(v, a) \
        do {    \
            *(volatile unsigned int *)(a) = (v);    \
            dsb(); \
        } while (0)

#define mt65xx_reg_sync_writew(v, a) \
        do {    \
            *(volatile unsigned short *)(a) = (v);    \
            dsb(); \
        } while (0)

#define mt65xx_reg_sync_writeb(v, a) \
        do {    \
            *(volatile unsigned char *)(a) = (v);    \
            dsb(); \
        } while (0)

#endif  /* __KERNEL__ */

#endif  /* !_MT_SYNC_WRITE_H */
