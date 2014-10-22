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

#ifndef _CHARGING_HW_COMMO_H_
#define _CHARGING_HW_COMMO_H_

#include <mach/mt_typedefs.h>

/* ============================================================ */
/* define */
/* ============================================================ */

// ============================================================ //
//Charging Function
// ============================================================ //
#define STATUS_OK 0
#define STATUS_UNSUPPORTED -1
#define GETARRAYNUM(array) (sizeof(array)/sizeof(array[0]))
#if defined(MTK_WIRELESS_CHARGER_SUPPORT)
#define WIRELESS_CHARGER_EXIST_STATE 0
#endif

/* ============================================================ */
/* External Variables */
/* ============================================================ */
extern int Enable_BATDRV_LOG;
extern kal_bool chargin_hw_init_done;
#if defined(MTK_WIRELESS_CHARGER_SUPPORT)
extern int wireless_charger_gpio_number;
#endif

extern int gpio_off_dir;
extern int gpio_off_out;
extern int gpio_on_dir;
extern int gpio_on_out;

/* ============================================================ */
/* External function */
/* ============================================================ */
extern kal_uint32 charging_value_to_parameter(const kal_uint32 *parameter, const kal_uint32 array_size, const kal_uint32 val);
extern kal_uint32 charging_parameter_to_value(const kal_uint32 *parameter, const kal_uint32 array_size, const kal_uint32 val);
extern kal_uint32 bmt_find_closest_level(const kal_uint32 *pList,kal_uint32 number,kal_uint32 level);
extern kal_uint32 charging_get_battery_status(void *data);
extern kal_uint32 charging_get_charger_det_status(void *data);
extern kal_bool charging_type_detection_done(void);
extern kal_uint32 charging_get_charger_type(void *data);
extern kal_uint32 charging_get_is_pcm_timer_trigger(void *data);
extern kal_uint32 charging_set_platform_reset(void *data);
extern kal_uint32 charging_get_platfrom_boot_mode(void *data);
extern kal_uint32 charging_set_power_off(void *data);
extern int hw_charging_get_charger_type(void);
extern bool mt_usb_is_device(void);

#endif // _CHARGING_HW_COMMO_H_

