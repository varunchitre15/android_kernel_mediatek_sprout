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

/*****************************************************************************
 *
 * Filename:
 * ---------
 *    charging_hw_common.c
 *
 * Project:
 * --------
 *   ALPS_Software
 *
 * Description:
 * ------------
 *   This file implements the common interface for charging hw.
 *
 * Author:
 * -------
 *  Owen Chen
 *
 *============================================================================
  * $Revision:   1.0  $
 * $Modtime:   6 Oct 2014 09:52:00  $
 * $Log:   //mtkvs01/vmdata/Maui_sw/archives/mcu/hal/peripheral/inc/bmt_chr_setting.h-arc  $
 *             HISTORY
 * Below this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#include <mach/charging.h>
#include "bq24158.h"
#include <mach/upmu_common.h>
#include <mach/mt_gpio.h>
#include <cust_gpio_usage.h>
#include <mach/upmu_hw.h>
#include <linux/xlog.h>
#include <linux/delay.h>
#include <mach/mt_sleep.h>
#include <mach/mt_boot.h>
#include <mach/system.h>
#include <cust_charging.h>
#include <mach/battery_ssb.h>
#include <mach/charging_hw_common.h>

 // ============================================================ //
 //global variable
 // ============================================================ //
static CHARGER_TYPE g_charger_type = CHARGER_UNKNOWN;
#if defined(MTK_WIRELESS_CHARGER_SUPPORT)
int wireless_charger_gpio_number   = (168 | 0x80000000);
#endif
kal_bool charging_type_det_done = KAL_TRUE;

int gpio_off_dir  = GPIO_DIR_OUT;
int gpio_off_out  = GPIO_OUT_ONE;
int gpio_on_dir   = GPIO_DIR_OUT;
int gpio_on_out   = GPIO_OUT_ZERO;


 // ============================================================ //
 //external function
 // ============================================================ //
extern kal_uint32 (*charging_func_pmic[CHARGING_CMD_NUMBER])(void *data);
extern kal_uint32 (*charging_func_bq24158[CHARGING_CMD_NUMBER])(void *data);
extern kal_uint32 (*charging_func_bq24196[CHARGING_CMD_NUMBER])(void *data);
extern kal_uint32 (*charging_func_fan5405[CHARGING_CMD_NUMBER])(void *data);
extern kal_uint32 (*charging_func_hw6333[CHARGING_CMD_NUMBER])(void *data);
extern void mt_power_off(void);

kal_uint32 charging_value_to_parameter(const kal_uint32 *parameter, const kal_uint32 array_size, const kal_uint32 val)
{
	if (val < array_size)
	{
		return parameter[val];
	}
	else
	{
		battery_xlog_printk(BAT_LOG_CRTI, "Can't find the parameter \r\n");
		return parameter[0];
	}
}


kal_uint32 charging_parameter_to_value(const kal_uint32 *parameter, const kal_uint32 array_size, const kal_uint32 val)
{
	kal_uint32 i;

	for(i=0;i<array_size;i++)
	{
		if (val == *(parameter + i))
		{
				return i;
		}
	}

    battery_xlog_printk(BAT_LOG_CRTI, "NO register value match \r\n");
	//TODO: ASSERT(0);	// not find the value
	return 0;
}


kal_uint32 bmt_find_closest_level(const kal_uint32 *pList,kal_uint32 number,kal_uint32 level)
{
	kal_uint32 i;
	kal_uint32 max_value_in_last_element;

	if(pList[0] < pList[1])
		max_value_in_last_element = KAL_TRUE;
	else
		max_value_in_last_element = KAL_FALSE;

	if(max_value_in_last_element == KAL_TRUE) {
		for(i = (number-1); i != 0; i--) {	 //max value in the last element
			if(pList[i] <= level) {
				return pList[i];
			}
		}


		battery_xlog_printk(BAT_LOG_CRTI, "Can't find closest level, small value first \r\n");
		return pList[0];
		//return CHARGE_CURRENT_0_00_MA;
	} else {
		for(i = 0; i< number; i++) {  // max value in the first element
			if(pList[i] <= level) {
				return pList[i];
			}
		}

		battery_xlog_printk(BAT_LOG_CRTI, "Can't find closest level, large value first \r\n");
		return pList[number -1];
		//return CHARGE_CURRENT_0_00_MA;
	}
}

kal_uint32 charging_get_battery_status(void *data)
{
	kal_uint32 status = STATUS_OK;

	upmu_set_baton_tdet_en(1);
	upmu_set_rg_baton_en(1);
	*(kal_bool*)(data) = upmu_get_rgs_baton_undet();

	return status;
}


kal_uint32 charging_get_charger_det_status(void *data)
{
	kal_uint32 status = STATUS_OK;

	*(kal_bool*)(data) = upmu_get_rgs_chrdet();

	if( upmu_get_rgs_chrdet() == 0 )
		g_charger_type = CHARGER_UNKNOWN;

	return status;
}


kal_bool charging_type_detection_done(void)
{
	 return charging_type_det_done;
}

kal_uint32 charging_get_charger_type(void *data)
{
	kal_uint32 status = STATUS_OK;

#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
    *(CHARGER_TYPE*)(data) = STANDARD_HOST;
#else

#if defined(MTK_WIRELESS_CHARGER_SUPPORT)
    int wireless_state = 0;
    if(wireless_charger_gpio_number!=0)
    {
        wireless_state = mt_get_gpio_in(wireless_charger_gpio_number);
        if(wireless_state == WIRELESS_CHARGER_EXIST_STATE)
        {
            *(CHARGER_TYPE*)(data) = WIRELESS_CHARGER;
            battery_xlog_printk(BAT_LOG_CRTI, "WIRELESS_CHARGER!\n");
            return status;
        }
    }
    else
    {
        battery_xlog_printk(BAT_LOG_CRTI, "wireless_charger_gpio_number=%d\n", wireless_charger_gpio_number);
    }

    if(g_charger_type!=CHARGER_UNKNOWN && g_charger_type!=WIRELESS_CHARGER)
    {
        *(CHARGER_TYPE*)(data) = g_charger_type;
        battery_xlog_printk(BAT_LOG_CRTI, "return %d!\n", g_charger_type);
        return status;
    }
#endif

    if(upmu_get_rgs_chrdet()==0)
    {
        g_charger_type = CHARGER_UNKNOWN;
        *(CHARGER_TYPE*)(data) = CHARGER_UNKNOWN;
        battery_xlog_printk(BAT_LOG_CRTI, "[charging_get_charger_type] return CHARGER_UNKNOWN\n");
        return status;
    }

    charging_type_det_done = KAL_FALSE;

    *(CHARGER_TYPE*)(data) = hw_charging_get_charger_type();
    //*(CHARGER_TYPE*)(data) = STANDARD_HOST;
    //*(CHARGER_TYPE*)(data) = STANDARD_CHARGER;

    charging_type_det_done = KAL_TRUE;

    g_charger_type = *(CHARGER_TYPE*)(data);

#endif

    return status;
}

kal_uint32 charging_get_is_pcm_timer_trigger(void *data)
{
    kal_uint32 status = STATUS_OK;

    if(slp_get_wake_reason() == WR_PCM_TIMER)
        *(kal_bool*)(data) = KAL_TRUE;
    else
        *(kal_bool*)(data) = KAL_FALSE;

    battery_xlog_printk(BAT_LOG_CRTI, "slp_get_wake_reason=%d\n", slp_get_wake_reason());

    return status;
}

kal_uint32 charging_set_platform_reset(void *data)
{
    kal_uint32 status = STATUS_OK;

    battery_xlog_printk(BAT_LOG_CRTI, "charging_set_platform_reset\n");

    arch_reset(0,NULL);

    return status;
}

kal_uint32 charging_get_platfrom_boot_mode(void *data)
{
    kal_uint32 status = STATUS_OK;

    *(kal_uint32*)(data) = get_boot_mode();

    battery_xlog_printk(BAT_LOG_CRTI, "get_boot_mode=%d\n", get_boot_mode());

    return status;
}


kal_uint32 charging_set_power_off(void *data)
{
	kal_uint32 status = STATUS_OK;

	battery_xlog_printk(BAT_LOG_CRTI, "charging_set_power_off\n");
	mt_power_off();

	return status;
}
 /*
 * FUNCTION
 *		Internal_chr_control_handler
 *
 * DESCRIPTION
 *		 This function is called to set the charger hw
 *
 * CALLS
 *
 * PARAMETERS
 *		None
 *
 * RETURNS
 *
 *
 * GLOBALS AFFECTED
 *	   None
 */
 kal_int32 chr_control_interface(CHARGING_CTRL_CMD cmd, void *data)
 {
	 kal_int32 status;

	 if(cmd < CHARGING_CMD_NUMBER) {
		 switch (ext_chr_ic_id) {
			case EXT_NONE:
				status = charging_func_pmic[cmd](data);
				chargin_hw_init_done = KAL_TRUE;
				battery_xlog_printk(BAT_LOG_CRTI, "charging_func_pmic\n");
				break;
			case EXT_BQ24158:
				status = charging_func_bq24158[cmd](data);
				battery_xlog_printk(BAT_LOG_CRTI, "charging_func_bq24158\n");
				break;
			case EXT_BQ24196:
				status = charging_func_bq24196[cmd](data);
				battery_xlog_printk(BAT_LOG_CRTI, "charging_func_bq24196\n");
				break;
			case EXT_FAN5405:
				status = charging_func_fan5405[cmd](data);
				battery_xlog_printk(BAT_LOG_CRTI, "charging_func_fan5405\n");
				break;
			case EXT_HW6333:
				status = charging_func_hw6333[cmd](data);
				battery_xlog_printk(BAT_LOG_CRTI, "charging_func_hw6333\n");
				break;
			default:
				status = charging_func_pmic[cmd](data);
				battery_xlog_printk(BAT_LOG_CRTI, "charging_func_pmic\n");
				break;
		 }
	 } else {
		 return STATUS_UNSUPPORTED;
	 }
	 return status;
 }

