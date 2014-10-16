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

#ifndef _MT_KEYPAD_SSB_CUST_H_
#define _MT_KEYPAD_SSB_CUST_H_

struct tag_para_keypad_ssb_data
{
    int version;
    u16 kpd_keymap_cust[72];
    int volume_up;
    int volume_down;
    int pmic_rst;
    int endflag;
};

#endif

extern struct tag_para_keypad_ssb_data keypad_cust_ssb_data;

