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

#ifndef __CUST_ACC_H__
#define __CUST_ACC_H__

struct acc_hw {
    int i2c_addr;
    int i2c_num;    /*!< the i2c bus used by the chip */
    int direction;  /*!< the direction of the chip */
    int power_id;   /*!< the LDO ID of the chip, MT6516_POWER_NONE means the power is always on*/
    int power_vol;  /*!< the Power Voltage used by the chip */
    int firlen;     /*!< the length of low pass filter */
    int (*power)(struct acc_hw *hw, unsigned int on, char *devname);
    int is_batch_supported;
};

extern struct acc_hw* get_cust_acc_hw(void);
#endif 
