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
 *    charging_pmic.c
 *
 * Project:
 * --------
 *   ALPS_Software
 *
 * Description:
 * ------------
 *   This file implements the interface between BMT and ADC scheduler.
 *
 * Author:
 * -------
 *  Oscar Liu
 *
 *============================================================================
  * $Revision:   1.0  $
 * $Modtime:   11 Aug 2005 10:28:16  $
 * $Log:   //mtkvs01/vmdata/Maui_sw/archives/mcu/hal/peripheral/inc/bmt_chr_setting.h-arc  $
 *             HISTORY
 * Below this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#include <mach/charging.h>
#include "bq24196.h"
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
#include <mach/charging_hw_common.h>

 // ============================================================ //
 //define
 // ============================================================ //

 // ============================================================ //
 //global variable
 // ============================================================ //
const kal_uint32 VBAT_CV_VTH_BQ24196[]=
{
	3504000,    3520000,    3536000,    3552000,
	3568000,    3584000,    3600000,    3616000,
	3632000,    3648000,    3664000,    3680000,
	3696000,    3712000,    3728000,    3744000,
	3760000,    3776000,    3792000,    3808000,
	3824000,    3840000,    3856000,    3872000,
	3888000,    3904000,    3920000,    3936000,
	3952000,    3968000,    3984000,    4000000,
	4016000,    4032000,    4048000,    4064000,
	4080000,    4096000,    4112000,    4128000,
	4144000,    4160000,    4176000,    4192000,
	4208000,    4224000,    4240000,    4256000
};

const kal_uint32 CS_VTH_BQ24196[]=
{
	51200,  57600,  64000,  70400,
	76800,  83200,  89600,  96000,
	102400, 108800, 115200, 121600,
	128000, 134400, 140800, 147200,
	153600, 160000, 166400, 172800,
	179200, 185600, 192000, 198400,
	204800, 211200, 217600, 224000
};

const kal_uint32 INPUT_CS_VTH_BQ24196[]=
{
	CHARGE_CURRENT_100_00_MA,  CHARGE_CURRENT_150_00_MA,  CHARGE_CURRENT_500_00_MA,  CHARGE_CURRENT_900_00_MA,
	CHARGE_CURRENT_1200_00_MA, CHARGE_CURRENT_1500_00_MA,  CHARGE_CURRENT_2000_00_MA,  CHARGE_CURRENT_MAX
};

const kal_uint32 VCDT_HV_VTH_BQ24196[]=
{
	  BATTERY_VOLT_04_200000_V, BATTERY_VOLT_04_250000_V,	  BATTERY_VOLT_04_300000_V,   BATTERY_VOLT_04_350000_V,
	  BATTERY_VOLT_04_400000_V, BATTERY_VOLT_04_450000_V,	  BATTERY_VOLT_04_500000_V,   BATTERY_VOLT_04_550000_V,
	  BATTERY_VOLT_04_600000_V, BATTERY_VOLT_06_000000_V,	  BATTERY_VOLT_06_500000_V,   BATTERY_VOLT_07_000000_V,
	  BATTERY_VOLT_07_500000_V, BATTERY_VOLT_08_500000_V,	  BATTERY_VOLT_09_500000_V,   BATTERY_VOLT_10_500000_V
};

// ============================================================ //
// function prototype
// ============================================================ //


// ============================================================ //
//extern variable
// ============================================================ //

// ============================================================ //
//extern function
// ============================================================ //

// ============================================================ //
static kal_uint32 charging_hw_init_bq24196(void *data)
{
	kal_uint32 status = STATUS_OK;

	upmu_set_rg_bc11_bb_ctrl(1);    //BC11_BB_CTRL
	upmu_set_rg_bc11_rst(1);        //BC11_RST

	bq24196_set_en_hiz(0x0);
	bq24196_set_vindpm(0xA); //VIN DPM check 4.68V
	bq24196_set_reg_rst(0x0);
	bq24196_set_wdt_rst(0x1); //Kick watchdog
	bq24196_set_sys_min(0x5); //Minimum system voltage 3.5V
	bq24196_set_iprechg(0x3); //Precharge current 512mA
	bq24196_set_iterm(0x0); //Termination current 128mA

	bq24196_set_vreg(0x2C); //VREG 4.208V

	bq24196_set_batlowv(0x1); //BATLOWV 3.0V
	bq24196_set_vrechg(0x0); //VRECHG 0.1V (4.108V)
	bq24196_set_en_term(0x1); //Enable termination
	bq24196_set_term_stat(0x0); //Match ITERM
	bq24196_set_watchdog(0x1); //WDT 40s
	bq24196_set_en_timer(0x0); //Disable charge timer
	bq24196_set_int_mask(0x0); //Disable fault interrupt

	return status;
}


static kal_uint32 charging_dump_register_bq24196(void *data)
{
	kal_uint32 status = STATUS_OK;

	battery_xlog_printk(BAT_LOG_CRTI, "charging_dump_register\r\n");

	bq24196_dump_register();

	return status;
}


static kal_uint32 charging_enable_bq24196(void *data)
{
	kal_uint32 status = STATUS_OK;
	kal_uint32 enable = *(kal_uint32*)(data);

	if(KAL_TRUE == enable)
	{
		bq24196_set_en_hiz(0x0);
		bq24196_set_chg_config(0x1); // charger enable
	}
	else
	{
#if defined(CONFIG_USB_MTK_HDRC_HCD)
		if(mt_usb_is_device())
#endif
		{
			bq24196_set_chg_config(0x0);
		}
	}

	return status;
}


static kal_uint32 charging_set_cv_voltage_bq24196(void *data)
{
	kal_uint32 status = STATUS_OK;
	kal_uint16 register_value;
	kal_uint32 cv_value = *(kal_uint32 *)(data);

	if(cv_value == BATTERY_VOLT_04_200000_V)
	{
		//use nearest value
		cv_value = 4208000;
	}
	register_value = charging_parameter_to_value(VBAT_CV_VTH_BQ24196, GETARRAYNUM(VBAT_CV_VTH_BQ24196), cv_value);
	bq24196_set_vreg(register_value);

	return status;
}


static kal_uint32 charging_get_current_bq24196(void *data)
{
    kal_uint32 status = STATUS_OK;
    //kal_uint32 array_size;
    //kal_uint8 reg_value;

    kal_uint8 ret_val=0;
    kal_uint8 ret_force_20pct=0;

    //Get current level
    bq24196_read_interface(bq24196_CON2, &ret_val, CON2_ICHG_MASK, CON2_ICHG_SHIFT);

    //Get Force 20% option
    bq24196_read_interface(bq24196_CON2, &ret_force_20pct, CON2_FORCE_20PCT_MASK, CON2_FORCE_20PCT_SHIFT);

    //Parsing
    ret_val = (ret_val*64) + 512;

    if (ret_force_20pct == 0)
    {
        //Get current level
        //array_size = GETARRAYNUM(CS_VTH_BQ24196);
        //*(kal_uint32 *)data = charging_value_to_parameter(CS_VTH_BQ24196,array_size,reg_value);
        *(kal_uint32 *)data = ret_val;
    }
    else
    {
        //Get current level
        //array_size = GETARRAYNUM(CS_VTH_20PCT);
        //*(kal_uint32 *)data = charging_value_to_parameter(CS_VTH_BQ24196,array_size,reg_value);
        //return (int)(ret_val<<1)/10;
        *(kal_uint32 *)data = (int)(ret_val<<1)/10;
    }

    return status;
}



static kal_uint32 charging_set_current_bq24196(void *data)
{
	kal_uint32 status = STATUS_OK;
	kal_uint32 set_chr_current;
	kal_uint32 array_size;
	kal_uint32 register_value;
	kal_uint32 current_value = *(kal_uint32 *)data;

	array_size = GETARRAYNUM(CS_VTH_BQ24196);
	set_chr_current = bmt_find_closest_level(CS_VTH_BQ24196, array_size, current_value);
	register_value = charging_parameter_to_value(CS_VTH_BQ24196, array_size ,set_chr_current);
	bq24196_set_ichg(register_value);

	return status;
}


static kal_uint32 charging_set_input_current_bq24196(void *data)
{
	kal_uint32 status = STATUS_OK;
	kal_uint32 set_chr_current;
	kal_uint32 array_size;
	kal_uint32 register_value;

	array_size = GETARRAYNUM(INPUT_CS_VTH_BQ24196);
	set_chr_current = bmt_find_closest_level(INPUT_CS_VTH_BQ24196, array_size, *(kal_uint32 *)data);
	register_value = charging_parameter_to_value(INPUT_CS_VTH_BQ24196, array_size ,set_chr_current);

	bq24196_set_iinlim(register_value);

	return status;
}


static kal_uint32 charging_get_charging_status_bq24196(void *data)
{
	kal_uint32 status = STATUS_OK;
	kal_uint32 ret_val;

	ret_val = bq24196_get_chrg_stat();

	if(ret_val == 0x3)
		*(kal_uint32 *)data = KAL_TRUE;
	else
		*(kal_uint32 *)data = KAL_FALSE;

	return status;
}


static kal_uint32 charging_reset_watch_dog_timer_bq24196(void *data)
{
	kal_uint32 status = STATUS_OK;

	battery_xlog_printk(BAT_LOG_CRTI, "charging_reset_watch_dog_timer\r\n");

	bq24196_set_wdt_rst(0x1); //Kick watchdog

	return status;
}

static kal_uint32 charging_set_hv_threshold_bq24196(void *data)
{
	 kal_uint32 status = STATUS_OK;

	 kal_uint32 set_hv_voltage;
	 kal_uint32 array_size;
	 kal_uint16 register_value;
	 kal_uint32 voltage = *(kal_uint32*)(data);

	 array_size = GETARRAYNUM(VCDT_HV_VTH_BQ24196);
	 set_hv_voltage = bmt_find_closest_level(VCDT_HV_VTH_BQ24196, array_size, voltage);
	 register_value = charging_parameter_to_value(VCDT_HV_VTH_BQ24196, array_size ,set_hv_voltage);
	 upmu_set_rg_vcdt_hv_vth(register_value);

	 return status;
}


static kal_uint32 charging_get_hv_status_bq24196(void *data)
{
	kal_uint32 status = STATUS_OK;

	*(kal_bool*)(data) = upmu_get_rgs_vcdt_hv_det();

	return status;
}

kal_uint32 (* const charging_func_bq24196[CHARGING_CMD_NUMBER])(void *data)=
{
	charging_hw_init_bq24196
	,charging_dump_register_bq24196
	,charging_enable_bq24196
	,charging_set_cv_voltage_bq24196
	,charging_get_current_bq24196
	,charging_set_current_bq24196
	,charging_set_input_current_bq24196
	,charging_get_charging_status_bq24196
	,charging_reset_watch_dog_timer_bq24196
	,charging_set_hv_threshold_bq24196
	,charging_get_hv_status_bq24196
	,charging_get_battery_status
	,charging_get_charger_det_status
	,charging_get_charger_type
	,charging_get_is_pcm_timer_trigger
	,charging_set_platform_reset
	,charging_get_platfrom_boot_mode
	,charging_set_power_off
};

