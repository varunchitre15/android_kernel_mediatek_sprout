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
#include <linux/xlog.h>
#include <linux/kernel.h>
#include <mach/battery_common.h>
#include <mach/charging.h>
#include "cust_charging.h"
#include <mach/mt_boot.h>
#include <linux/delay.h>
#include <mach/battery_meter.h>
#include <mach/battery_ssb.h>

 /* ============================================================ // */
 /* define */
 /* ============================================================ // */
 /* cut off to full */
#define POST_CHARGING_TIME	 30 * 60	/* 30mins */



 /* ============================================================ // */
 /* global variable */
 /* ============================================================ // */
CHR_CURRENT_ENUM g_temp_CC_value_linear = CHARGE_CURRENT_0_00_MA;
kal_uint32 charging_full_current = CHARGING_FULL_CURRENT;	/* mA */

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

kal_uint32 get_charging_setting_current_linear(void)
{
	return g_temp_CC_value_linear;
}

static BATTERY_VOLTAGE_ENUM select_jeita_cv_linear(void)
{
	BATTERY_VOLTAGE_ENUM cv_voltage;

	if (g_temp_status == TEMP_ABOVE_POS_60) {
		cv_voltage = cv_above_pos_60;
	} else if (g_temp_status == TEMP_POS_45_TO_POS_60) {
		cv_voltage = cv_pos_45_60;
	} else if (g_temp_status == TEMP_POS_10_TO_POS_45) {
		if (high_battery_volt_enable) {
			cv_voltage = BATTERY_VOLT_04_350000_V;
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

PMU_STATUS do_jeita_state_machine_linear(void)
{
	int previous_g_temp_status;
	BATTERY_VOLTAGE_ENUM cv_voltage;

	previous_g_temp_status = g_temp_status;
	/* JEITA battery temp Standard */
	if (BMT_status.temperature >= t_high_discharge_zone) {
		battery_xlog_printk(BAT_LOG_CRTI,
				    "[BATTERY] Battery Over high Temperature(%d) !!\n\r",
				    t_high_discharge_zone);
		g_temp_status = TEMP_ABOVE_POS_60;
		return PMU_STATUS_FAIL;
	} else if (BMT_status.temperature > t_high_zone) {
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
			g_jeita_recharging_voltage = v_recharge_pos_45_60;
			v_cc2cv = cc2cv_pos_45_60;
			charging_full_current = CHARGING_FULL_CURRENT;
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
			if (high_battery_volt_enable == 1) {
				g_jeita_recharging_voltage = v_recharge_pos_10_45;
			} else {
				g_jeita_recharging_voltage = v_recharge_pos_10_45;
			}
			
			v_cc2cv = cc2cv_pos_10_45;
			charging_full_current = CHARGING_FULL_CURRENT;
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
			g_jeita_recharging_voltage = v_recharge_cv_pos_0_10;
			v_cc2cv = cc2cv_pos_0_10;
			charging_full_current = CHARGING_FULL_CURRENT;
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
			g_jeita_recharging_voltage = v_recharge_neg_10_0;
			v_cc2cv = cc2cv_neg_10_0;
			charging_full_current = cur_terminate_neg_10;
		}
	} else {
		battery_xlog_printk(BAT_LOG_CRTI,
				    "[BATTERY] Battery below low Temperature(%d) !!\n\r",
				    t_freeze_zone);
		g_temp_status = TEMP_BELOW_NEG_10;
		return PMU_STATUS_FAIL;
	}

	/* set CV after temperature changed */
	cv_voltage = select_jeita_cv_linear();
	battery_charging_control(CHARGING_CMD_SET_CV_VOLTAGE,&cv_voltage);

	return PMU_STATUS_OK;
}


static void set_jeita_charging_current_linear(void)
{
#ifdef CONFIG_USB_IF
	if (BMT_status.charger_type == STANDARD_HOST)
		return;
#endif

	if(g_temp_status == TEMP_POS_10_TO_POS_45) {
		return;	
	} else if(g_temp_status == TEMP_NEG_10_TO_POS_0) {
		g_temp_CC_value_linear = cur_jeita_neg_10_to_0;   //for low temp	
		battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] JEITA set charging current : %d\r\n", g_temp_CC_value_linear);
	} else if(g_temp_status == TEMP_POS_0_TO_POS_10) {
		g_temp_CC_value_linear = cur_jeita_pos_0_to_10;   //for low temp	
		battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] JEITA set charging current : %d\r\n", g_temp_CC_value_linear);
	} else if(g_temp_status == TEMP_POS_45_TO_POS_60) {
		g_temp_CC_value_linear = cur_jeita_pos_45_to_60;   //for low temp	
		battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] JEITA set charging current : %d\r\n", g_temp_CC_value_linear);
	}
}

void select_charging_curret_bcct_linear(void)
{
	/* done on set_bat_charging_current_limit */
}

kal_uint32 set_bat_charging_current_limit_linear(int current_limit)
{
	battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] set_bat_charging_current_limit (%d)\r\n",
			    current_limit);

	if (current_limit != -1) {
		g_bcct_flag = 1;

		if (current_limit < 70)
			g_temp_CC_value_linear = CHARGE_CURRENT_0_00_MA;
		else if (current_limit < 200)
			g_temp_CC_value_linear = CHARGE_CURRENT_70_00_MA;
		else if (current_limit < 300)
			g_temp_CC_value_linear = CHARGE_CURRENT_200_00_MA;
		else if (current_limit < 400)
			g_temp_CC_value_linear = CHARGE_CURRENT_300_00_MA;
		else if (current_limit < 450)
			g_temp_CC_value_linear = CHARGE_CURRENT_400_00_MA;
		else if (current_limit < 550)
			g_temp_CC_value_linear = CHARGE_CURRENT_450_00_MA;
		else if (current_limit < 650)
			g_temp_CC_value_linear = CHARGE_CURRENT_550_00_MA;
		else if (current_limit < 700)
			g_temp_CC_value_linear = CHARGE_CURRENT_650_00_MA;
		else if (current_limit < 800)
			g_temp_CC_value_linear = CHARGE_CURRENT_700_00_MA;
		else if (current_limit < 900)
			g_temp_CC_value_linear = CHARGE_CURRENT_800_00_MA;
		else if (current_limit < 1000)
			g_temp_CC_value_linear = CHARGE_CURRENT_900_00_MA;
		else if (current_limit < 1100)
			g_temp_CC_value_linear = CHARGE_CURRENT_1000_00_MA;
		else if (current_limit < 1200)
			g_temp_CC_value_linear = CHARGE_CURRENT_1100_00_MA;
		else if (current_limit < 1300)
			g_temp_CC_value_linear = CHARGE_CURRENT_1200_00_MA;
		else if (current_limit < 1400)
			g_temp_CC_value_linear = CHARGE_CURRENT_1300_00_MA;
		else if (current_limit < 1500)
			g_temp_CC_value_linear = CHARGE_CURRENT_1400_00_MA;
		else if (current_limit < 1600)
			g_temp_CC_value_linear = CHARGE_CURRENT_1500_00_MA;
		else if (current_limit == 1600)
			g_temp_CC_value_linear = CHARGE_CURRENT_1600_00_MA;
		else
			g_temp_CC_value_linear = CHARGE_CURRENT_450_00_MA;
	} else {
		/* change to default current setting */
		g_bcct_flag = 0;
	}

	wake_up_bat();

	return g_bcct_flag;
}


void select_charging_curret_linear(void)
{
	if (g_ftm_battery_flag) {
		battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] FTM charging : %d\r\n",
				    charging_level_data[0]);
		g_temp_CC_value_linear = charging_level_data[0];
	} else {
		if (BMT_status.charger_type == STANDARD_HOST) {
#ifdef CONFIG_USB_IF
			{
				if (g_usb_state == USB_SUSPEND) {
					g_temp_CC_value_linear = cur_usb_suspend;
				} else if (g_usb_state == USB_UNCONFIGURED) {
					g_temp_CC_value_linear = cur_usb_unconfigured;
				} else if (g_usb_state == USB_CONFIGURED) {
					g_temp_CC_value_linear = cur_usb_configured;
				} else {
					g_temp_CC_value_linear = cur_usb_unconfigured;
				}

				battery_xlog_printk(BAT_LOG_CRTI,
						    "[BATTERY] STANDARD_HOST CC mode charging : %d on %d state\r\n",
						    g_temp_CC_value_linear, g_usb_state);
			}
#else
			{
				g_temp_CC_value_linear = cur_usb_charger;
			}
#endif
		} else if (BMT_status.charger_type == NONSTANDARD_CHARGER) {
			g_temp_CC_value_linear = cur_no_std_charger;
		} else if (BMT_status.charger_type == STANDARD_CHARGER) {
			g_temp_CC_value_linear = cur_ac_charger;
		} else if (BMT_status.charger_type == CHARGING_HOST) {
			g_temp_CC_value_linear = cur_charging_host;
		} else if (BMT_status.charger_type == APPLE_2_1A_CHARGER) {
			g_temp_CC_value_linear = cur_apple_2_1A;
		} else if (BMT_status.charger_type == APPLE_1_0A_CHARGER) {
			g_temp_CC_value_linear = cur_apple_1A;
		} else if (BMT_status.charger_type == APPLE_0_5A_CHARGER) {
			g_temp_CC_value_linear = cur_apple_0_5A;
		} else {
			g_temp_CC_value_linear = CHARGE_CURRENT_70_00_MA;
		}

		battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] Default CC mode charging : %d\r\n",
				    g_temp_CC_value_linear);

		if (jeita_enable == 1) {
			set_jeita_charging_current_linear();
		}
	}
}




static kal_uint32 charging_full_check_linear(void)
{
	kal_uint32 status = KAL_FALSE;

#if defined(POST_TIME_ENABLE)
	static kal_uint32 post_charging_time;

	if (post_charging_time >= POST_CHARGING_TIME) {
		status = KAL_TRUE;
		post_charging_time = 0;

		battery_xlog_printk(BAT_LOG_CRTI,
				    "[BATTERY] Battery real full and disable charging on %d mA\n",
				    BMT_status.ICharging);
	} else if (post_charging_time > 0) {
		post_charging_time += BAT_TASK_PERIOD;
		battery_xlog_printk(BAT_LOG_CRTI,
				    "[BATTERY] post_charging_time=%d,POST_CHARGING_TIME=%d\n",
				    post_charging_time, POST_CHARGING_TIME);
	} else if ((BMT_status.TOPOFF_charging_time > 60)
		   && (BMT_status.ICharging <= charging_full_current)) {
		post_charging_time = BAT_TASK_PERIOD;
		battery_xlog_printk(BAT_LOG_CRTI,
				    "[BATTERY] Enter Post charge, post_charging_time=%d,POST_CHARGING_TIME=%d\n",
				    post_charging_time, POST_CHARGING_TIME);
	} else {
		post_charging_time = 0;
	}
#else
	static kal_uint8 full_check_count;

	if (BMT_status.ICharging <= charging_full_current) {
		full_check_count++;
		if (6 == full_check_count) {
			status = KAL_TRUE;
			full_check_count = 0;
			battery_xlog_printk(BAT_LOG_CRTI,
					    "[BATTERY] Battery full and disable charging on %d mA\n",
					    BMT_status.ICharging);
		}
	} else {
		full_check_count = 0;
	}
#endif

	return status;
}


static void charging_current_calibration_linear(void)
{
	kal_int32 bat_isense_offset;
	bat_isense_offset = 0;
	battery_meter_sync(bat_isense_offset);
}

static void pchr_turn_on_charging_linear(void)
{
	BATTERY_VOLTAGE_ENUM cv_voltage;
	kal_uint32 charging_enable = KAL_TRUE;

	battery_xlog_printk(BAT_LOG_FULL, "[BATTERY] pchr_turn_on_charging_linear()!\r\n");

	if (BMT_status.bat_charging_state == CHR_ERROR) {
		battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] Charger Error, turn OFF charging !\n");

		charging_enable = KAL_FALSE;
	} else if ((g_platform_boot_mode == META_BOOT) || (g_platform_boot_mode == ADVMETA_BOOT)) {
		battery_xlog_printk(BAT_LOG_CRTI,
				    "[BATTERY] In meta or advanced meta mode, disable charging.\n");
		charging_enable = KAL_FALSE;
	} else {
		/*HW initialization */
		battery_xlog_printk(BAT_LOG_FULL, "charging_hw_init\n");
		battery_charging_control(CHARGING_CMD_INIT, NULL);


		/* Set Charging Current */
		if (g_bcct_flag == 1) {
			battery_xlog_printk(BAT_LOG_FULL,
				    "[BATTERY] select_charging_curret_bcct !\n");
			select_charging_curret_bcct_linear();
		} else {
			battery_xlog_printk(BAT_LOG_FULL, "[BATTERY] select_charging_current !\n");
			select_charging_curret_linear();
		}

		if (g_temp_CC_value_linear == CHARGE_CURRENT_0_00_MA) {
			charging_enable = KAL_FALSE;
			battery_xlog_printk(BAT_LOG_CRTI,
					    "[BATTERY] charging current is set 0mA, turn off charging !\r\n");
		} else {

			battery_charging_control(CHARGING_CMD_SET_CURRENT, &g_temp_CC_value_linear);

			/* Set CV */
			if (jeita_enable == 0) {
				if (high_battery_volt_enable == 1) {
					cv_voltage = BATTERY_VOLT_04_350000_V;
				} else {
					cv_voltage = BATTERY_VOLT_04_200000_V;
				}
				battery_charging_control(CHARGING_CMD_SET_CV_VOLTAGE, &cv_voltage);
			}
		}
	}

	/* enable/disable charging */
	battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] pchr_turn_on_charging_linear(), enable =%d \r\n",
			    charging_enable);
	battery_charging_control(CHARGING_CMD_ENABLE, &charging_enable);


}


PMU_STATUS BAT_PreChargeModeAction_linear(void)
{
	kal_bool charging_enable = KAL_FALSE;

	battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] Pre-CC mode charge, timer=%d on %d !!\n\r",
			    BMT_status.PRE_charging_time, BMT_status.total_charging_time);

	BMT_status.PRE_charging_time += BAT_TASK_PERIOD;
	BMT_status.CC_charging_time = 0;
	BMT_status.TOPOFF_charging_time = 0;
	BMT_status.total_charging_time += BAT_TASK_PERIOD;

	if (BMT_status.UI_SOC == 100) {
		BMT_status.bat_charging_state = CHR_BATFULL;
		BMT_status.bat_full = KAL_TRUE;
		g_charging_full_reset_bat_meter = KAL_TRUE;
	} else if (BMT_status.bat_vol > v_pre2cc) {
		BMT_status.bat_charging_state = CHR_CC;
	}

	/*Charging 9s and discharging 1s : start */
	battery_charging_control(CHARGING_CMD_ENABLE, &charging_enable);

	msleep(1000);

	charging_current_calibration_linear();

	pchr_turn_on_charging_linear();

	return PMU_STATUS_OK;
}


PMU_STATUS BAT_ConstantCurrentModeAction_linear(void)
{
	kal_bool charging_enable = KAL_FALSE;

	battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] CC mode charge, timer=%d on %d !!\n\r",
			    BMT_status.CC_charging_time, BMT_status.total_charging_time);

	BMT_status.PRE_charging_time = 0;
	BMT_status.CC_charging_time += BAT_TASK_PERIOD;
	BMT_status.TOPOFF_charging_time = 0;
	BMT_status.total_charging_time += BAT_TASK_PERIOD;

	if (BMT_status.bat_vol > v_cc2cv) {
		BMT_status.bat_charging_state = CHR_TOP_OFF;
	}

	/*Charging 9s and discharging 1s : start */
	battery_charging_control(CHARGING_CMD_ENABLE, &charging_enable);

	msleep(1000);

	charging_current_calibration_linear();

	pchr_turn_on_charging_linear();

	return PMU_STATUS_OK;
}


PMU_STATUS BAT_TopOffModeAction_linear(void)
{
	kal_uint32 charging_enable = KAL_FALSE;

	battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] Top Off mode charge, timer=%d on %d !!\n\r",
			    BMT_status.TOPOFF_charging_time, BMT_status.total_charging_time);

	BMT_status.PRE_charging_time = 0;
	BMT_status.CC_charging_time = 0;
	BMT_status.TOPOFF_charging_time += BAT_TASK_PERIOD;
	BMT_status.total_charging_time += BAT_TASK_PERIOD;

	pchr_turn_on_charging_linear();

	if ((BMT_status.TOPOFF_charging_time >= MAX_CV_CHARGING_TIME)
	    || (charging_full_check_linear() == KAL_TRUE)) {
		BMT_status.bat_charging_state = CHR_BATFULL;
		BMT_status.bat_full = KAL_TRUE;
		g_charging_full_reset_bat_meter = KAL_TRUE;

		/*  Disable charging */
		battery_charging_control(CHARGING_CMD_ENABLE, &charging_enable);
	}

	return PMU_STATUS_OK;
}


PMU_STATUS BAT_BatteryFullAction_linear(void)
{
	kal_uint32 charging_enable = KAL_FALSE;

	battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] Battery full !!\n\r");

	BMT_status.bat_full = KAL_TRUE;
	BMT_status.total_charging_time = 0;
	BMT_status.PRE_charging_time = 0;
	BMT_status.CC_charging_time = 0;
	BMT_status.TOPOFF_charging_time = 0;
	BMT_status.POSTFULL_charging_time = 0;
	BMT_status.bat_in_recharging_state = KAL_FALSE;

	if (jeita_enable == 1 && BMT_status.bat_vol < g_jeita_recharging_voltage) {
		battery_xlog_printk(BAT_LOG_CRTI,
				    "[BATTERY] Battery Enter Re-charging!! , vbat=(%d)\n\r",
				    BMT_status.bat_vol);

		BMT_status.bat_in_recharging_state = KAL_TRUE;
		BMT_status.bat_charging_state = CHR_CC;
	}


	if (jeita_enable == 0 && BMT_status.bat_vol < v_recharge) {
		battery_xlog_printk(BAT_LOG_CRTI,
				    "[BATTERY] Battery Enter Re-charging!! , vbat=(%d)\n\r",
				    BMT_status.bat_vol);

		BMT_status.bat_in_recharging_state = KAL_TRUE;
		BMT_status.bat_charging_state = CHR_CC;
	}

	/*  Disable charging */
	battery_charging_control(CHARGING_CMD_ENABLE, &charging_enable);

	return PMU_STATUS_OK;
}


PMU_STATUS BAT_BatteryHoldAction_linear(void)
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


PMU_STATUS BAT_BatteryStatusFailAction_linear(void)
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


void mt_battery_charging_algorithm_linear(void)
{
	switch (BMT_status.bat_charging_state) {
	case CHR_PRE:
		BAT_PreChargeModeAction_linear();
		break;

	case CHR_CC:
		BAT_ConstantCurrentModeAction_linear();
		break;

	case CHR_TOP_OFF:
		BAT_TopOffModeAction_linear();
		break;

	case CHR_BATFULL:
		BAT_BatteryFullAction_linear();
		break;

	case CHR_HOLD:
		BAT_BatteryHoldAction_linear();
		break;

	case CHR_ERROR:
		BAT_BatteryStatusFailAction_linear();
		break;
	}

}
