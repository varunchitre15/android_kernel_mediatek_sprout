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

/*
 * MD218A voice coil motor driver
 *
 *
 */

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>


//#define SENSOR_DRIVE_LENS_DEBUG
#ifdef SENSOR_DRIVE_LENS_DEBUG
#define SENSOR_DRIVE_LENSDB printk
#else
#define SENSOR_DRIVE_LENSDB(x,...)
#endif

static int __init SENSOR_DRIVE_LENS_i2C_init(void)
{
    return 0;
}

static void __exit SENSOR_DRIVE_LENS_i2C_exit(void)
{

}

module_init(SENSOR_DRIVE_LENS_i2C_init);
module_exit(SENSOR_DRIVE_LENS_i2C_exit);

MODULE_DESCRIPTION("Sensor Drive lens module driver");
MODULE_AUTHOR("KY Chen <ky.chen@Mediatek.com>");
MODULE_LICENSE("GPL");


