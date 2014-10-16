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

#ifndef _MT_TOUCH_SSB_CUST_H_
#define _MT_TOUCH_SSB_CUST_H_

#define NAME_LENGTH 20
#define TOUCH_DRIVER_NUM 20

struct tag_para_touch_ssb_data_single
{
    char identifier[NAME_LENGTH];
    unsigned int i2c_number;
    unsigned int i2c_addr;
    unsigned int power_id;
    int tpd_key_local[3];
    int tpd_key_dim_local[3][4];
    int tpd_resolution[2];
    int use_tpd_button;
    unsigned int endflag;
};

struct tag_para_touch_ssb_data
{
    unsigned int version;
    struct tag_para_touch_ssb_data_single touch_ssb_data[TOUCH_DRIVER_NUM];
    unsigned int endflag;
};

#endif

extern struct tag_para_touch_ssb_data touch_cust_ssb_data;
