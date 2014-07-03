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

#ifndef __CUST_MAG_H__
#define __CUST_MAG_H__

#include <linux/types.h>

#define M_CUST_I2C_ADDR_NUM 2

struct mag_hw {
    int i2c_num;
    int direction;
    int power_id;
    int power_vol;
    unsigned char    i2c_addr[M_CUST_I2C_ADDR_NUM]; /*!< i2c address list,for chips which has different addresses with different HW layout */
    int power_vio_id;
    int power_vio_vol;
    int is_batch_supported;
};

extern struct mag_hw* get_cust_mag_hw(void);
#endif 
