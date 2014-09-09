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

#include <linux/types.h>
#include <mach/mt_pm_ldo.h>
#include <cust_gyro.h>

/*---------------------------------------------------------------------------*/
/*
int cust_gyro_power(struct gyro_hw *hw, unsigned int on, char* devname)
{
    if (hw->power_id == MT65XX_POWER_NONE)
        return 0;
    if (on)
        return hwPowerOn(hw->power_id, hw->power_vol, devname);
    else
        return hwPowerDown(hw->power_id, devname);
}
*/
/*---------------------------------------------------------------------------*/
static struct gyro_hw cust_gyro_hw = {
    .addr =0x68,
    .i2c_num = 2,
    .direction = 2,
    .power_id = MT65XX_POWER_NONE,  /*!< LDO is not used */
    .power_vol= VOL_DEFAULT,        /*!< LDO is not used */
    .firlen = 16,                   /*!< don't enable low pass fileter */
   // .power = cust_gyro_power,
};
/*---------------------------------------------------------------------------*/
struct gyro_hw* bmg160_get_cust_gyro_hw(void)
{
    return &cust_gyro_hw;
}
