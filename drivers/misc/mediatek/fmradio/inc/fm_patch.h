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

#ifndef __FM_PATCH_H__
#define __FM_PATCH_H__

enum {
    FM_ROM_V1 = 0,
    FM_ROM_V2 = 1,
    FM_ROM_V3 = 2,
    FM_ROM_V4 = 3,
    FM_ROM_V5 = 4,
    FM_ROM_MAX
};

struct fm_patch_tbl {
    fm_s32 idx;
    fm_s8 *patch;
    fm_s8 *coeff;
    fm_s8 *rom;
    fm_s8 *hwcoeff;
};

extern fm_s32 fm_file_exist(const fm_s8 *filename);

extern fm_s32 fm_file_read(const fm_s8 *filename, fm_u8* dst, fm_s32 len, fm_s32 position);

extern fm_s32 fm_file_write(const fm_s8 *filename, fm_u8* dst, fm_s32 len, fm_s32 *ppos);


#endif //__FM_PATCH_H__

