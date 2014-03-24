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

#ifndef __LZ4K_H__
#define __LZ4K_H__

#include <linux/types.h>

#define LZ4K_TAG 1261722188 // "LZ4K"

int lz4k_compress(const unsigned char *src, size_t src_len,
            unsigned char *dst, size_t *dst_len, void *wrkmem);

int lz4k_decompress_safe(const unsigned char *src, size_t src_len,
            unsigned char *dst, size_t *dst_len);

#ifdef CONFIG_UBIFS_FS
int lz4k_decompress_ubifs(const unsigned char *src, size_t src_len, 
            unsigned char *dst, size_t *dst_len);
#endif // CONFIG_UBIFS_FS

#endif // __LZ4K_H__

