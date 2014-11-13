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
#include <mach/charging_hw_common.h>
#include <mach/battery_ssb.h>

 // ============================================================ //
 //global variable
 // ============================================================ //

const kal_uint32 VBAT_CV_VTH_BQ24158[]=
{
	BATTERY_VOLT_03_500000_V,   BATTERY_VOLT_03_520000_V,	BATTERY_VOLT_03_540000_V,   BATTERY_VOLT_03_560000_V,
	BATTERY_VOLT_03_580000_V,   BATTERY_VOLT_03_600000_V,	BATTERY_VOLT_03_620000_V,   BATTERY_VOLT_03_640000_V,
	BATTERY_VOLT_03_660000_V,	BATTERY_VOLT_03_680000_V,	BATTERY_VOLT_03_700000_V,	BATTERY_VOLT_03_720000_V,
	BATTERY_VOLT_03_740000_V,	BATTERY_VOLT_03_760000_V,	BATTERY_VOLT_03_780000_V,	BATTERY_VOLT_03_800000_V,
	BATTERY_VOLT_03_820000_V,	BATTERY_VOLT_03_840000_V,	BATTERY_VOLT_03_860000_V,	BATTERY_VOLT_03_880000_V,
	BATTERY_VOLT_03_900000_V,	BATTERY_VOLT_03_920000_V,	BATTERY_VOLT_03_940000_V,	BATTERY_VOLT_03_960000_V,
	BATTERY_VOLT_03_980000_V,	BATTERY_VOLT_04_000000_V,	BATTERY_VOLT_04_020000_V,	BATTERY_VOLT_04_040000_V,
	BATTERY_VOLT_04_060000_V,	BATTERY_VOLT_04_080000_V,	BATTERY_VOLT_04_100000_V,	BATTERY_VOLT_04_120000_V,
	BATTERY_VOLT_04_140000_V,   BATTERY_VOLT_04_160000_V,	BATTERY_VOLT_04_180000_V,   BATTERY_VOLT_04_200000_V,
	BATTERY_VOLT_04_220000_V,   BATTERY_VOLT_04_240000_V,	BATTERY_VOLT_04_260000_V,   BATTERY_VOLT_04_280000_V,
	BATTERY_VOLT_04_300000_V,   BATTERY_VOLT_04_320000_V,	BATTERY_VOLT_04_340000_V,   BATTERY_VOLT_04_360000_V,
	BATTERY_VOLT_04_380000_V,   BATTERY_VOLT_04_400000_V,	BATTERY_VOLT_04_420000_V,   BATTERY_VOLT_04_440000_V

};

const kal_uint32 CS_VTH_BQ24158[]=
{
	CHARGE_CURRENT_550_00_MA,   CHARGE_CURRENT_650_00_MA,	CHARGE_CURRENT_750_00_MA, CHARGE_CURRENT_850_00_MA,
	CHARGE_CURRENT_950_00_MA,   CHARGE_CURRENT_1050_00_MA,	CHARGE_CURRENT_1150_00_MA, CHARGE_CURRENT_1250_00_MA
};

 const kal_uint32 INPUT_CS_VTH_BQ24158[]=
 {
	 CHARGE_CURRENT_100_00_MA,	 CHARGE_CURRENT_500_00_MA,	 CHARGE_CURRENT_800_00_MA, CHARGE_CURRENT_MAX
 };

 const kal_uint32 VCDT_HV_VTH_BQ24158[]=
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
extern bool mt_usb_is_device(void);

static kal_uint32 charging_hw_init_bq24158(void *data)
{
	kal_uint32 status = STATUS_OK;
	static bool charging_init_flag = KAL_FALSE;
	int gpio_number;
	int gpio_off_mode;
	int gpio_on_mode;

	gpio_number = GPIO_SWCHARGER_EN_PIN;
	gpio_off_mode = GPIO_SWCHARGER_EN_PIN_M_GPIO;
	gpio_on_mode = GPIO_SWCHARGER_EN_PIN_M_GPIO;

	mt_set_gpio_mode(gpio_number,gpio_on_mode);
	mt_set_gpio_dir(gpio_number,gpio_on_dir);
	mt_set_gpio_out(gpio_number,gpio_on_out);
#if defined(MTK_WIRELESS_CHARGER_SUPPORT)
	mt_set_gpio_mode(wireless_charger_gpio_number,0); // 0:GPIO mode
	mt_set_gpio_dir(wireless_charger_gpio_number,0); // 0: input, 1: output
#endif
	battery_xlog_printk(BAT_LOG_FULL, "gpio_number=0x%x,gpio_on_mode=%d,gpio_off_mode=%d\n", gpio_number, gpio_on_mode, gpio_off_mode);

	upmu_set_rg_usbdl_rst(1);		//force leave USBDL mode

	if (high_battery_volt_enable) {
		battery_xlog_printk(BAT_LOG_CRTI, "1 high_battery_volt_enable=%d\n", high_battery_volt_enable);
		bq24158_config_interface_liao(0x06,0x77); // ISAFE = 1250mA, VSAFE = 4.34V
		bq24158_config_interface_liao(0x02,0xaa); // 4.34
	} else {
		battery_xlog_printk(BAT_LOG_CRTI, "2 high_battery_volt_enable=%d\n", high_battery_volt_enable);
		bq24158_config_interface_liao(0x06,0x70);
		bq24158_config_interface_liao(0x02,0x8e); // 4.2
	}
	bq24158_config_interface_liao(0x00,0xC0);	//kick chip watch dog
	bq24158_config_interface_liao(0x01,0xb8);	//TE=1, CE=0, HZ_MODE=0, OPA_MODE=0
	bq24158_config_interface_liao(0x05,0x02);

	bq24158_config_interface_liao(0x04,0x1A); //146mA
	if ( !charging_init_flag ) {
		bq24158_config_interface_liao(0x04,0x1A); //146mA
		charging_init_flag = KAL_TRUE;
	}
	return status;
}


static kal_uint32 charging_dump_register_bq24158(void *data)
{
	kal_uint32 status = STATUS_OK;

	bq24158_dump_register();

	return status;
}


static kal_uint32 charging_enable_bq24158(void *data)
{
	kal_uint32 status = STATUS_OK;
	kal_uint32 enable = *(kal_uint32*)(data);
	int gpio_number;
	int gpio_off_mode;
	int gpio_on_mode;

	gpio_number = GPIO_SWCHARGER_EN_PIN;
	gpio_off_mode = GPIO_SWCHARGER_EN_PIN_M_GPIO;
	gpio_on_mode = GPIO_SWCHARGER_EN_PIN_M_GPIO;

	mt_set_gpio_mode(gpio_number,gpio_on_mode);
	mt_set_gpio_dir(gpio_number,gpio_on_dir);
	mt_set_gpio_out(gpio_number,gpio_on_out);

	if(KAL_TRUE == enable)
	{
		bq24158_set_ce(0);
		bq24158_set_hz_mode(0);
		bq24158_set_opa_mode(0);
	}
	else
	{

#if defined(CONFIG_USB_MTK_HDRC_HCD)
		if(mt_usb_is_device())
#endif
		{
			mt_set_gpio_mode(gpio_number,gpio_off_mode);
			mt_set_gpio_dir(gpio_number,gpio_off_dir);
			mt_set_gpio_out(gpio_number,gpio_off_out);

			// bq24158_set_ce(1);
		}
	}

	return status;
}


static kal_uint32 charging_set_cv_voltage_bq24158(void *data)
{
	kal_uint32 status = STATUS_OK;
	kal_uint16 register_value;
	kal_uint32 cv_value = *(kal_uint32 *)(data);

	register_value = charging_parameter_to_value(VBAT_CV_VTH_BQ24158, GETARRAYNUM(VBAT_CV_VTH_BQ24158) ,*(kal_uint32 *)(data));
	bq24158_set_oreg(register_value);

	return status;
}


static kal_uint32 charging_get_current_bq24158(void *data)
{
    kal_uint32 status = STATUS_OK;
    kal_uint32 array_size;
    kal_uint8 reg_value;

    //Get current level
    array_size = GETARRAYNUM(CS_VTH_BQ24158);
    bq24158_read_interface(0x1, &reg_value, 0x3, 0x6);	//IINLIM
    *(kal_uint32 *)data = charging_value_to_parameter(CS_VTH_BQ24158,array_size,reg_value);

    return status;
}



static kal_uint32 charging_set_current_bq24158(void *data)
{
	kal_uint32 status = STATUS_OK;
	kal_uint32 set_chr_current;
	kal_uint32 array_size;
	kal_uint32 register_value;
	kal_uint32 current_value = *(kal_uint32 *)data;

	if(current_value <= CHARGE_CURRENT_350_00_MA)
	{
		bq24158_set_io_level(1);
	}
	else
	{
		bq24158_set_io_level(0);
		array_size = GETARRAYNUM(CS_VTH_BQ24158);
		set_chr_current = bmt_find_closest_level(CS_VTH_BQ24158, array_size, current_value);
		register_value = charging_parameter_to_value(CS_VTH_BQ24158, array_size ,set_chr_current);
		bq24158_set_iocharge(register_value);
	}
	return status;
}


static kal_uint32 charging_set_input_current_bq24158(void *data)
{
	kal_uint32 status = STATUS_OK;
	kal_uint32 set_chr_current;
	kal_uint32 array_size;
	kal_uint32 register_value;

	if(*(kal_uint32 *)data > CHARGE_CURRENT_500_00_MA)
	{
		register_value = 0x3;
	}
	else
	{
		array_size = GETARRAYNUM(INPUT_CS_VTH_BQ24158);
		set_chr_current = bmt_find_closest_level(INPUT_CS_VTH_BQ24158, array_size, *(kal_uint32 *)data);
		register_value = charging_parameter_to_value(INPUT_CS_VTH_BQ24158, array_size ,set_chr_current);
	}

	bq24158_set_input_charging_current(register_value);

	return status;
}


static kal_uint32 charging_get_charging_status_bq24158(void *data)
{
	kal_uint32 status = STATUS_OK;
	kal_uint32 ret_val;

	ret_val = bq24158_get_chip_status();

	if(ret_val == 0x2)
		*(kal_uint32 *)data = KAL_TRUE;
	else
		*(kal_uint32 *)data = KAL_FALSE;

	return status;
}


static kal_uint32 charging_reset_watch_dog_timer_bq24158(void *data)
{
	 kal_uint32 status = STATUS_OK;

	 bq24158_set_tmr_rst(1);

	 return status;
}


static kal_uint32 charging_set_hv_threshold_bq24158(void *data)
{
	 kal_uint32 status = STATUS_OK;

	 kal_uint32 set_hv_voltage;
	 kal_uint32 array_size;
	 kal_uint16 register_value;
	 kal_uint32 voltage = *(kal_uint32*)(data);

	 array_size = GETARRAYNUM(VCDT_HV_VTH_BQ24158);
	 set_hv_voltage = bmt_find_closest_level(VCDT_HV_VTH_BQ24158, array_size, voltage);
	 register_value = charging_parameter_to_value(VCDT_HV_VTH_BQ24158, array_size ,set_hv_voltage);
	 upmu_set_rg_vcdt_hv_vth(register_value);

	 return status;
}


static kal_uint32 charging_get_hv_status_bq24158(void *data)
{
	kal_uint32 status = STATUS_OK;

	*(kal_bool*)(data) = upmu_get_rgs_vcdt_hv_det();

	return status;
}

kal_uint32 (* const charging_func_bq24158[CHARGING_CMD_NUMBER])(void *data)=
{
	charging_hw_init_bq24158
	,charging_dump_register_bq24158
	,charging_enable_bq24158
	,charging_set_cv_voltage_bq24158
	,charging_get_current_bq24158
	,charging_set_current_bq24158
	,charging_set_input_current_bq24158
	,charging_get_charging_status_bq24158
	,charging_reset_watch_dog_timer_bq24158
	,charging_set_hv_threshold_bq24158
	,charging_get_hv_status_bq24158
	,charging_get_battery_status
	,charging_get_charger_det_status
	,charging_get_charger_type
	,charging_get_is_pcm_timer_trigger
	,charging_set_platform_reset
	,charging_get_platfrom_boot_mode
	,charging_set_power_off
};


