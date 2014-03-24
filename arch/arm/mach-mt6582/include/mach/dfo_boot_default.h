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

#ifndef DFO_BOOT_DEFAULT_H
#define DFO_BOOT_DEFAULT_H

tag_dfo_boot dfo_boot_default =
{
    // name array
    {
        "MD5_SIZE",
        "MD5_SMEM_SIZE",
        "MTK_MD5_SUPPORT",
        "MTK_ENABLE_MD5",
        "MTK_ENABLE_MD1",
        "MTK_ENABLE_MD2",
        "MD1_SIZE",
        "MD2_SIZE",
        "MD1_SMEM_SIZE",
        "MD2_SMEM_SIZE",
        "MTK_MD1_SUPPORT",
        "MTK_MD2_SUPPORT",
        "LCM_FAKE_WIDTH",
        "LCM_FAKE_HEIGHT"
    },

    // value array
    {
        0x01600000,
        0x00200000,
        5,
        0,
        1,
        0,
        0x01600000,
        0x01600000,
        0x00200000,
        0x00200000,
        3,
        4,
        0,
        0
    }
};

#endif
