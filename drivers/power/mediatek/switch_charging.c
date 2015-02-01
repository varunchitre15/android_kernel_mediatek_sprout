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
 *    linear_charging.c
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
#include <linux/kernel.h>
#include <mach/battery_common.h>
#include <mach/charging.h>
#include "cust_charging.h"
#include <mach/mt_boot.h>
#include <mach/battery_meter.h>
#include <mach/battery_ssb.h>

 /* ============================================================ // */
 /* define */
 /* ============================================================ // */
#define FULL_CHECK_TIMES		6

 /* ============================================================ // */
 /* global variable */
 /* ============================================================ // */
kal_uint32 g_full_check_count = 0;
CHR_CURRENT_ENUM g_temp_CC_value_switch = CHARGE_CURRENT_0_00_MA;
CHR_CURRENT_ENUM g_temp_input_CC_value_switch = CHARGE_CURRENT_0_00_MA;

 /* ============================================================ // */
 /* function prototype */
 /* ============================================================ // */


 /* ============================================================ // */
 /* extern variable */
 /* ============================================================ // */
extern int g_platform_boot_mode;

 /* ============================================================ // */
 /* extern function */
 /* ============================================================ // */


 /* ============================================================ // */
void BATTERY_SetUSBState_switch(int usb_state_value)
{
#if defined(CONFIG_POWER_EXT)
	battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY_SetUSBState] in FPGA/EVB, no service\r\n");
#else
	if ((usb_state_value < USB_SUSPEND) || ((usb_state_value > USB_CONFIGURED))) {
		battery_xlog_printk(BAT_LOG_CRTI,
				    "[BATTERY] BAT_SetUSBState Fail! Restore to default value\r\n");
		usb_state_value = USB_UNCONFIGURED;
	} else {
		battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] BAT_SetUSBState Success! Set %d\r\n",
				    usb_state_value);
		g_usb_state = usb_state_value;
	}
#endif
}


kal_uint32 get_charging_setting_current_switch(void)
{
	return g_temp_CC_value_switch;
}




static BATTERY_VOLTAGE_ENUM select_jeita_cv_switch(void)
{
	BATTERY_VOLTAGE_ENUM cv_voltage;

	if (g_temp_status == TEMP_ABOVE_POS_60) {
		cv_voltage = cv_above_pos_60;
	} else if (g_temp_status == TEMP_POS_45_TO_POS_60) {
		cv_voltage = cv_pos_45_60;
	} else if (g_temp_status == TEMP_POS_10_TO_POS_45) {
		if (high_battery_volt_enable == 1) {
			cv_voltage = BATTERY_VOLT_04_340000_V;
		} else {
			cv_voltage = cv_pos_10_45;
		}
	} else if (g_temp_status == TEMP_POS_0_TO_POS_10) {
		cv_voltage = cv_pos_0_10;
	} else if (g_temp_status == TEMP_NEG_10_TO_POS_0) {
		cv_voltage = cv_neg_10_0;
	} else if (g_temp_status == TEMP_BELOW_NEG_10) {
		cv_voltage = cv_below_neg_10;
	} else {
		cv_voltage = BATTERY_VOLT_04_200000_V;
	}

	return cv_voltage;
}

PMU_STATUS do_jeita_state_machine_switch(void)
{
	BATTERY_VOLTAGE_ENUM cv_voltage;

	/* JEITA battery temp Standard */

	if (BMT_status.temperature >= t_high_discharge_zone) {
		battery_xlog_printk(BAT_LOG_CRTI,
				    "[BATTERY] Battery Over high Temperature(%d) !!\n\r",
				    t_high_discharge_zone);

		g_temp_status = TEMP_ABOVE_POS_60;

		return PMU_STATUS_FAIL;
	} else if (BMT_status.temperature > t_high_zone)	/* control 45c to normal behavior */
	{
		if ((g_temp_status == TEMP_ABOVE_POS_60)
		    && (BMT_status.temperature >= t_high_recharge_zone)) {
			battery_xlog_printk(BAT_LOG_CRTI,
					    "[BATTERY] Battery Temperature between %d and %d,not allow charging yet!!\n\r",
					    t_high_recharge_zone,
					    t_high_discharge_zone);

			return PMU_STATUS_FAIL;
		} else {
			battery_xlog_printk(BAT_LOG_CRTI,
					    "[BATTERY] Battery Temperature between %d and %d !!\n\r",
					    t_high_zone, t_high_discharge_zone);

			g_temp_status = TEMP_POS_45_TO_POS_60;
		}
	} else if (BMT_status.temperature >= t_middle2low_zone) {
		if (((g_temp_status == TEMP_POS_45_TO_POS_60)
		     && (BMT_status.temperature >= t_high2middle_zone))
		    || ((g_temp_status == TEMP_POS_0_TO_POS_10)
			&& (BMT_status.temperature <= t_low_zone ))) {
			battery_xlog_printk(BAT_LOG_CRTI,
					    "[BATTERY] Battery Temperature not recovery to normal temperature charging mode yet!!\n\r");
		} else {
			battery_xlog_printk(BAT_LOG_CRTI,
					    "[BATTERY] Battery Normal Temperature between %d and %d !!\n\r",
					    t_middle2low_zone, t_high_zone);
			g_temp_status = TEMP_POS_10_TO_POS_45;
		}
	} else if (BMT_status.temperature >= t_low_discharge_zone) {
		if ((g_temp_status == TEMP_NEG_10_TO_POS_0 || g_temp_status == TEMP_BELOW_NEG_10)
		    && (BMT_status.temperature <= t_low_recharge_zone)) {
			if (g_temp_status == TEMP_NEG_10_TO_POS_0) {
				battery_xlog_printk(BAT_LOG_CRTI,
						    "[BATTERY] Battery Temperature between %d and %d !!\n\r",
						    t_low_recharge_zone,
						    t_middle2low_zone);
			}
			if (g_temp_status == TEMP_BELOW_NEG_10) {
				battery_xlog_printk(BAT_LOG_CRTI,
						    "[BATTERY] Battery Temperature between %d and %d,not allow charging yet!!\n\r",
						    t_low_discharge_zone,
						    t_low_recharge_zone);
				return PMU_STATUS_FAIL;
			}
		} else {
			battery_xlog_printk(BAT_LOG_CRTI,
					    "[BATTERY] Battery Temperature between %d and %d !!\n\r",
					    t_low_discharge_zone, t_middle2low_zone);

			g_temp_status = TEMP_POS_0_TO_POS_10;
		}
	} else if (BMT_status.temperature >= t_freeze_zone) {
		if ((g_temp_status == TEMP_BELOW_NEG_10)
		    && (BMT_status.temperature <= t_freeze2low_zone)) {
			battery_xlog_printk(BAT_LOG_CRTI,
					    "[BATTERY] Battery Temperature between %d and %d,not allow charging yet!!\n\r",
					    t_freeze_zone, t_freeze2low_zone);

			return PMU_STATUS_FAIL;
		} else {
			battery_xlog_printk(BAT_LOG_CRTI,
					    "[BATTERY] Battery Temperature between %d and %d !!\n\r",
					    t_freeze_zone, t_low_discharge_zone);

			g_temp_status = TEMP_NEG_10_TO_POS_0;
		}
	} else {
		battery_xlog_printk(BAT_LOG_CRTI,
				    "[BATTERY] Battery below low Temperature(%d) !!\n\r",
				    t_freeze_zone);
		g_temp_status = TEMP_BELOW_NEG_10;

		return PMU_STATUS_FAIL;
	}

	/* set CV after temperature changed */

	cv_voltage = select_jeita_cv_switch();
	battery_charging_control(CHARGING_CMD_SET_CV_VOLTAGE, &cv_voltage);

	return PMU_STATUS_OK;
}


static void set_jeita_charging_current_switch(void)
{
#ifdef CONFIG_USB_IF
	if (BMT_status.charger_type == STANDARD_HOST)
		return;
#endif

	if(g_temp_status == TEMP_POS_10_TO_POS_45) {
		return;	
	} else if(g_temp_status == TEMP_NEG_10_TO_POS_0) {
		g_temp_CC_value_switch = cur_jeita_neg_10_to_0;   //for low temp	
		battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] JEITA set charging current switch: %d\r\n", g_temp_CC_value_switch);
	} else if(g_temp_status == TEMP_POS_0_TO_POS_10) {
		g_temp_CC_value_switch = cur_jeita_pos_0_to_10;   //for low temp	
		battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] JEITA set charging current switch: %d\r\n", g_temp_CC_value_switch);
	} else if(g_temp_status == TEMP_POS_45_TO_POS_60) {
		g_temp_CC_value_switch = cur_jeita_pos_45_to_60;   //for low temp	
		battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] JEITA set charging current switch: %d\r\n", g_temp_CC_value_switch);
	}
}

void select_charging_curret_bcct_switch(void)
{
	if ((BMT_status.charger_type == STANDARD_HOST) ||
	    (BMT_status.charger_type == NONSTANDARD_CHARGER)) {
		if (g_bcct_value < 100)
			g_temp_input_CC_value_switch = CHARGE_CURRENT_0_00_MA;
		else if (g_bcct_value < 500)
			g_temp_input_CC_value_switch = CHARGE_CURRENT_100_00_MA;
		else if (g_bcct_value < 800)
			g_temp_input_CC_value_switch = CHARGE_CURRENT_500_00_MA;
		else if (g_bcct_value == 800)
			g_temp_input_CC_value_switch = CHARGE_CURRENT_800_00_MA;
		else
			g_temp_input_CC_value_switch = CHARGE_CURRENT_500_00_MA;
	} else if ((BMT_status.charger_type == STANDARD_CHARGER) ||
		   (BMT_status.charger_type == CHARGING_HOST)) {
		g_temp_input_CC_value_switch = CHARGE_CURRENT_MAX;

		/* --------------------------------------------------- */
		/* set IOCHARGE */
		if (g_bcct_value < 550)
			g_temp_CC_value_switch = CHARGE_CURRENT_0_00_MA;
		else if (g_bcct_value < 650)
			g_temp_CC_value_switch = CHARGE_CURRENT_550_00_MA;
		else if (g_bcct_value < 750)
			g_temp_CC_value_switch = CHARGE_CURRENT_650_00_MA;
		else if (g_bcct_value < 850)
			g_temp_CC_value_switch = CHARGE_CURRENT_750_00_MA;
		else if (g_bcct_value < 950)
			g_temp_CC_value_switch = CHARGE_CURRENT_850_00_MA;
		else if (g_bcct_value < 1050)
			g_temp_CC_value_switch = CHARGE_CURRENT_950_00_MA;
		else if (g_bcct_value < 1150)
			g_temp_CC_value_switch = CHARGE_CURRENT_1050_00_MA;
		else if (g_bcct_value < 1250)
			g_temp_CC_value_switch = CHARGE_CURRENT_1150_00_MA;
		else if (g_bcct_value == 1250)
			g_temp_CC_value_switch = CHARGE_CURRENT_1250_00_MA;
		else
			g_temp_CC_value_switch = CHARGE_CURRENT_650_00_MA;
		/* --------------------------------------------------- */

	} else {
		g_temp_input_CC_value_switch = CHARGE_CURRENT_500_00_MA;
	}
}

static void pchr_turn_on_charging_switch(void);
kal_uint32 set_bat_charging_current_limit_switch(int current_limit)
{
	battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] set_bat_charging_current_limit_switch (%d)\r\n",
			    current_limit);

	if (current_limit != -1) {
		g_bcct_flag = 1;
		g_bcct_value = current_limit;

		if (current_limit < 70)
			g_temp_CC_value_switch = CHARGE_CURRENT_0_00_MA;
		else if (current_limit < 200)
			g_temp_CC_value_switch = CHARGE_CURRENT_70_00_MA;
		else if (current_limit < 300)
			g_temp_CC_value_switch = CHARGE_CURRENT_200_00_MA;
		else if (current_limit < 400)
			g_temp_CC_value_switch = CHARGE_CURRENT_300_00_MA;
		else if (current_limit < 450)
			g_temp_CC_value_switch = CHARGE_CURRENT_400_00_MA;
		else if (current_limit < 550)
			g_temp_CC_value_switch = CHARGE_CURRENT_450_00_MA;
		else if (current_limit < 650)
			g_temp_CC_value_switch = CHARGE_CURRENT_550_00_MA;
		else if (current_limit < 700)
			g_temp_CC_value_switch = CHARGE_CURRENT_650_00_MA;
		else if (current_limit < 800)
			g_temp_CC_value_switch = CHARGE_CURRENT_700_00_MA;
		else if (current_limit < 900)
			g_temp_CC_value_switch = CHARGE_CURRENT_800_00_MA;
		else if (current_limit < 1000)
			g_temp_CC_value_switch = CHARGE_CURRENT_900_00_MA;
		else if (current_limit < 1100)
			g_temp_CC_value_switch = CHARGE_CURRENT_1000_00_MA;
		else if (current_limit < 1200)
			g_temp_CC_value_switch = CHARGE_CURRENT_1100_00_MA;
		else if (current_limit < 1300)
			g_temp_CC_value_switch = CHARGE_CURRENT_1200_00_MA;
		else if (current_limit < 1400)
			g_temp_CC_value_switch = CHARGE_CURRENT_1300_00_MA;
		else if (current_limit < 1500)
			g_temp_CC_value_switch = CHARGE_CURRENT_1400_00_MA;
		else if (current_limit < 1600)
			g_temp_CC_value_switch = CHARGE_CURRENT_1500_00_MA;
		else if (current_limit == 1600)
			g_temp_CC_value_switch = CHARGE_CURRENT_1600_00_MA;
		else
			g_temp_CC_value_switch = CHARGE_CURRENT_450_00_MA;
	} else {
		/* change to default current setting */
		g_bcct_flag = 0;
	}

	/* wake_up_bat(); */
	pchr_turn_on_charging_switch();

	return g_bcct_flag;
}


void select_charging_curret_switch(void)
{
	if (g_ftm_battery_flag) {
		battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] FTM charging : %d\r\n",
				    charging_level_data[0]);
		g_temp_CC_value_switch = charging_level_data[0];

		if (g_temp_CC_value_switch == CHARGE_CURRENT_450_00_MA) {
			g_temp_input_CC_value_switch = CHARGE_CURRENT_500_00_MA;
		} else {
			g_temp_input_CC_value_switch = CHARGE_CURRENT_MAX;
			g_temp_CC_value_switch = cur_ac_charger;

			battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] set_ac_current \r\n");
		}
	} else {
		if (BMT_status.charger_type == STANDARD_HOST) {
#ifdef CONFIG_USB_IF
			{
				g_temp_input_CC_value_switch = CHARGE_CURRENT_MAX;
				if (g_usb_state == USB_SUSPEND) {
					g_temp_CC_value_switch = cur_usb_suspend;
				} else if (g_usb_state == USB_UNCONFIGURED) {
					g_temp_CC_value_switch = cur_usb_unconfigured;
				} else if (g_usb_state == USB_CONFIGURED) {
					g_temp_CC_value_switch = cur_usb_configured;
				} else {
					g_temp_CC_value_switch = cur_usb_unconfigured;
				}

				battery_xlog_printk(BAT_LOG_CRTI,
						    "[BATTERY] STANDARD_HOST CC mode charging : %d on %d state\r\n",
						    g_temp_CC_value_switch, g_usb_state);
			}
#else
			{
				g_temp_input_CC_value_switch = cur_usb_charger;
				g_temp_CC_value_switch = cur_usb_charger;
			}
#endif
		} else if (BMT_status.charger_type == NONSTANDARD_CHARGER) {
			g_temp_input_CC_value_switch = cur_no_std_charger;
			g_temp_CC_value_switch = cur_no_std_charger;

		} else if (BMT_status.charger_type == STANDARD_CHARGER) {
			g_temp_input_CC_value_switch = cur_ac_charger;
			g_temp_CC_value_switch = cur_ac_charger;
		} else if (BMT_status.charger_type == CHARGING_HOST) {
			g_temp_input_CC_value_switch = cur_charging_host;
			g_temp_CC_value_switch = cur_charging_host;
		} else if (BMT_status.charger_type == APPLE_2_1A_CHARGER) {
			g_temp_input_CC_value_switch = cur_apple_2_1A;
			g_temp_CC_value_switch = cur_apple_2_1A;
		} else if (BMT_status.charger_type == APPLE_1_0A_CHARGER) {
			g_temp_input_CC_value_switch = cur_apple_1A;
			g_temp_CC_value_switch = cur_apple_1A;
		} else if (BMT_status.charger_type == APPLE_0_5A_CHARGER) {
			g_temp_input_CC_value_switch = cur_apple_0_5A;
			g_temp_CC_value_switch = cur_apple_0_5A;
		} else {
			g_temp_input_CC_value_switch = CHARGE_CURRENT_500_00_MA;
			g_temp_CC_value_switch = CHARGE_CURRENT_500_00_MA;
		}

		if (jeita_enable == 1) {
			set_jeita_charging_current_switch();
		}
	}


}


static kal_uint32 charging_full_check_switch(void)
{
	kal_uint32 status;

	battery_charging_control(CHARGING_CMD_GET_CHARGING_STATUS, &status);
	if (status == KAL_TRUE) {
		g_full_check_count++;
		if (g_full_check_count >= FULL_CHECK_TIMES) {
			return KAL_TRUE;
		} else
			return KAL_FALSE;
	} else {
		g_full_check_count = 0;
		return status;
	}
}


static void pchr_turn_on_charging_switch(void)
{
	BATTERY_VOLTAGE_ENUM cv_voltage;

	kal_uint32 charging_enable = KAL_TRUE;

	if (BMT_status.bat_charging_state == CHR_ERROR) {
		battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] Charger Error, turn OFF charging !\n");

		charging_enable = KAL_FALSE;

	} else if ((g_platform_boot_mode == META_BOOT) || (g_platform_boot_mode == ADVMETA_BOOT)) {
		battery_xlog_printk(BAT_LOG_CRTI,
				    "[BATTERY] In meta or advanced meta mode, disable charging.\n");
		charging_enable = KAL_FALSE;
	} else {
		/*HW initialization */
		battery_charging_control(CHARGING_CMD_INIT, NULL);

		battery_xlog_printk(BAT_LOG_FULL, "charging_hw_init\n");

		/* Set Charging Current */
		if (g_bcct_flag == 1) {
			select_charging_curret_bcct_switch();

			battery_xlog_printk(BAT_LOG_FULL,
					    "[BATTERY] select_charging_curret_bcct_switch !\n");
		} else {
			select_charging_curret_switch();

			battery_xlog_printk(BAT_LOG_FULL, "[BATTERY] select_charging_curret_switch !\n");
		}
		battery_xlog_printk(BAT_LOG_CRTI,
				    "[BATTERY] Default CC mode charging : %d, input current = %d\r\n",
				    g_temp_CC_value_switch, g_temp_input_CC_value_switch);
		if (g_temp_CC_value_switch == CHARGE_CURRENT_0_00_MA
		    || g_temp_input_CC_value_switch == CHARGE_CURRENT_0_00_MA) {

			charging_enable = KAL_FALSE;

			battery_xlog_printk(BAT_LOG_CRTI,
					    "[BATTERY] charging current is set 0mA, turn off charging !\r\n");
		} else {
			battery_charging_control(CHARGING_CMD_SET_INPUT_CURRENT,
						 &g_temp_input_CC_value_switch);
			battery_charging_control(CHARGING_CMD_SET_CURRENT, &g_temp_CC_value_switch);

			/*Set CV Voltage */
			if (jeita_enable == 0) {
				if(high_battery_volt_enable == 1) { 
					cv_voltage = BATTERY_VOLT_04_340000_V;
				} else {
					cv_voltage = BATTERY_VOLT_04_200000_V;
				}
				battery_charging_control(CHARGING_CMD_SET_CV_VOLTAGE, &cv_voltage);
			}
		}
	}

	/* enable/disable charging */
	battery_charging_control(CHARGING_CMD_ENABLE, &charging_enable);

	battery_xlog_printk(BAT_LOG_FULL, "[BATTERY] pchr_turn_on_charging(), enable =%d !\r\n",
			    charging_enable);
}


PMU_STATUS BAT_PreChargeModeAction_switch(void)
{
	battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] Pre-CC mode charge, timer=%d on %d !!\n\r",
			    BMT_status.PRE_charging_time, BMT_status.total_charging_time);

	BMT_status.PRE_charging_time += BAT_TASK_PERIOD;
	BMT_status.CC_charging_time = 0;
	BMT_status.TOPOFF_charging_time = 0;
	BMT_status.total_charging_time += BAT_TASK_PERIOD;

	/*  Enable charger */
	pchr_turn_on_charging_switch();

	if (BMT_status.UI_SOC == 100) {
		BMT_status.bat_charging_state = CHR_BATFULL;
		BMT_status.bat_full = KAL_TRUE;
		g_charging_full_reset_bat_meter = KAL_TRUE;
	} else if (BMT_status.bat_vol > v_pre2cc) {
		BMT_status.bat_charging_state = CHR_CC;
	}



	return PMU_STATUS_OK;
}


PMU_STATUS BAT_ConstantCurrentModeAction_switch(void)
{
	battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] CC mode charge, timer=%d on %d !!\n\r",
			    BMT_status.CC_charging_time, BMT_status.total_charging_time);

	BMT_status.PRE_charging_time = 0;
	BMT_status.CC_charging_time += BAT_TASK_PERIOD;
	BMT_status.TOPOFF_charging_time = 0;
	BMT_status.total_charging_time += BAT_TASK_PERIOD;

	/*  Enable charger */
	pchr_turn_on_charging_switch();

	if (charging_full_check_switch() == KAL_TRUE) {
		BMT_status.bat_charging_state = CHR_BATFULL;
		BMT_status.bat_full = KAL_TRUE;
		g_charging_full_reset_bat_meter = KAL_TRUE;
	}

	return PMU_STATUS_OK;
}


PMU_STATUS BAT_BatteryFullAction_switch(void)
{
	battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] Battery full !!\n\r");

	BMT_status.bat_full = KAL_TRUE;
	BMT_status.total_charging_time = 0;
	BMT_status.PRE_charging_time = 0;
	BMT_status.CC_charging_time = 0;
	BMT_status.TOPOFF_charging_time = 0;
	BMT_status.POSTFULL_charging_time = 0;
	BMT_status.bat_in_recharging_state = KAL_FALSE;

	if (charging_full_check_switch() == KAL_FALSE) {
		battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] Battery Re-charging !!\n\r");

		BMT_status.bat_in_recharging_state = KAL_TRUE;
		BMT_status.bat_charging_state = CHR_CC;
		battery_meter_reset();
	}


	return PMU_STATUS_OK;
}


PMU_STATUS BAT_BatteryHoldAction_switch(void)
{
	kal_uint32 charging_enable;

	battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] Hold mode !!\n\r");

	if (BMT_status.bat_vol < v_recharge_at_talking || g_call_state == CALL_IDLE) {
		BMT_status.bat_charging_state = CHR_CC;
		battery_xlog_printk(BAT_LOG_CRTI,
				    "[BATTERY] Exit Hold mode and Enter CC mode !!\n\r");
	}

	/*  Disable charger */
	charging_enable = KAL_FALSE;
	battery_charging_control(CHARGING_CMD_ENABLE, &charging_enable);

	return PMU_STATUS_OK;
}


PMU_STATUS BAT_BatteryStatusFailAction_switch(void)
{
	kal_uint32 charging_enable;

	battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] BAD Battery status... Charging Stop !!\n\r");

	if (jeita_enable == 1) {
		if ((g_temp_status == TEMP_ABOVE_POS_60) || (g_temp_status == TEMP_BELOW_NEG_10)) {
			temp_error_recovery_chr_flag = KAL_FALSE;
		}
		if ((temp_error_recovery_chr_flag == KAL_FALSE) && (g_temp_status != TEMP_ABOVE_POS_60)
		    && (g_temp_status != TEMP_BELOW_NEG_10)) {
			temp_error_recovery_chr_flag = KAL_TRUE;
			BMT_status.bat_charging_state = CHR_PRE;
		}
	}

	BMT_status.total_charging_time = 0;
	BMT_status.PRE_charging_time = 0;
	BMT_status.CC_charging_time = 0;
	BMT_status.TOPOFF_charging_time = 0;
	BMT_status.POSTFULL_charging_time = 0;

	/*  Disable charger */
	charging_enable = KAL_FALSE;
	battery_charging_control(CHARGING_CMD_ENABLE, &charging_enable);

	return PMU_STATUS_OK;
}


void mt_battery_charging_algorithm_switch(void)
{
	battery_charging_control(CHARGING_CMD_RESET_WATCH_DOG_TIMER, NULL);

	switch (BMT_status.bat_charging_state) {
	case CHR_PRE:
		BAT_PreChargeModeAction_switch();
		break;

	case CHR_CC:
		BAT_ConstantCurrentModeAction_switch();
		break;

	case CHR_BATFULL:
		BAT_BatteryFullAction_switch();
		break;

	case CHR_HOLD:
		BAT_BatteryHoldAction_switch();
		break;

	case CHR_ERROR:
		BAT_BatteryStatusFailAction_switch();
		break;
	}

	battery_charging_control(CHARGING_CMD_DUMP_REGISTER, NULL);
}
