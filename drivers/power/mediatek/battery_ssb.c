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

#include <linux/slab.h>
#include <linux/string.h>
#include <mach/mt_typedefs.h>
#include <mach/battery_common.h>
#include "mach/battery_ssb.h"
#include <mach/battery_meter.h>
#include <mach/charging.h>
#include <mach/battery_meter_hal.h>
#include "cust_battery_meter.h"
#include "cust_battery_meter_table.h"
#include "cust_charging.h"


battery_header battery_hdr;
battery_data_type battery_cust_data;
unsigned int battery_cust_buf[MAX_BATTERY_PARA_SIZE];

kal_int32 fg_vbat_avg_size = 18;
/*wake up voltage {VBAT_NORMAL_WAKEUP, VBAT_LOW_POWER_WAKEUP}*/

kal_int32 v_normal_wakeup = VBAT_NORMAL_WAKEUP;
kal_int32 v_low_power_wakeup = VBAT_LOW_POWER_WAKEUP;

/*pull up voltage for NTC temperature calculation*/
kal_int32 v_pull_up = RBAT_PULL_UP_VOLT;

/* Linear Charging Threshold*/
kal_int32 v_pre2cc = V_PRE2CC_THRES;
kal_int32 v_cc2cv = V_CC2TOPOFF_THRES;
kal_int32 v_recharge = RECHARGING_VOLTAGE;

/* charger error check*/
kal_int32 v_chr_max = V_CHARGER_MAX;
kal_int32 v_chr_min = V_CHARGER_MIN;

/* stop charging while in talking mode*/
kal_int32 v_recharge_at_talking = TALKING_RECHARGE_VOLTAGE;

/* start 0 percent tracking when reach this value */
kal_int32 v_0percent_sync = V_0PERCENT_TRACKING;

/* JEITA parameter */
kal_int32 cv_above_pos_60 = JEITA_TEMP_ABOVE_POS_60_CV_VOLTAGE;
kal_int32 cv_pos_45_60 = JEITA_TEMP_POS_45_TO_POS_60_CV_VOLTAGE;
kal_int32 cv_pos_10_45 = JEITA_TEMP_POS_10_TO_POS_45_CV_VOLTAGE;
kal_int32 cv_pos_0_10 = JEITA_TEMP_POS_0_TO_POS_10_CV_VOLTAGE;
kal_int32 cv_neg_10_0 = JEITA_TEMP_NEG_10_TO_POS_0_CV_VOLTAGE;
kal_int32 cv_below_neg_10 = JEITA_TEMP_BELOW_NEG_10_CV_VOLTAGE;


/* For JEITA Linear Charging only */
kal_int32 v_recharge_pos_45_60 = JEITA_TEMP_POS_45_TO_POS_60_RECHARGE_VOLTAGE;
kal_int32 v_recharge_pos_10_45 = JEITA_TEMP_POS_10_TO_POS_45_RECHARGE_VOLTAGE;
kal_int32 v_recharge_cv_pos_0_10  = JEITA_TEMP_POS_0_TO_POS_10_RECHARGE_VOLTAGE;
kal_int32 v_recharge_neg_10_0 = JEITA_TEMP_NEG_10_TO_POS_0_RECHARGE_VOLTAGE;
kal_int32 cc2cv_pos_45_60 = JEITA_TEMP_POS_45_TO_POS_60_CC2TOPOFF_THRESHOLD;
kal_int32 cc2cv_pos_10_45  = JEITA_TEMP_POS_10_TO_POS_45_CC2TOPOFF_THRESHOLD;
kal_int32 cc2cv_pos_0_10 = JEITA_TEMP_POS_0_TO_POS_10_CC2TOPOFF_THRESHOLD;
kal_int32 cc2cv_neg_10_0 = JEITA_TEMP_NEG_10_TO_POS_0_CC2TOPOFF_THRESHOLD;

/* Charging Current Setting */
kal_int32 cur_usb_suspend = USB_CHARGER_CURRENT_SUSPEND;
kal_int32 cur_usb_unconfigured = USB_CHARGER_CURRENT_UNCONFIGURED;
kal_int32 cur_usb_configured = USB_CHARGER_CURRENT_CONFIGURED;

/* Charging Current Setting */
kal_int32 cur_usb_charger = USB_CHARGER_CURRENT;
kal_int32 cur_charging_host = CHARGING_HOST_CHARGER_CURRENT;
kal_int32 cur_no_std_charger = NON_STD_AC_CHARGER_CURRENT;
kal_int32 cur_ac_charger = AC_CHARGER_CURRENT;
kal_int32 cur_apple_2_1A = APPLE_2_1A_CHARGER_CURRENT;
kal_int32 cur_apple_1A = APPLE_1_0A_CHARGER_CURRENT;
kal_int32 cur_apple_0_5A = APPLE_0_5A_CHARGER_CURRENT;

/* For JEITA Linear Charging only */
kal_int32 cur_jeita_neg_10_to_0 = JEITA_NEG_10_TO_POS_0_CURRENT;
kal_int32 cur_jeita_pos_0_to_10 = JEITA_POS_0_TO_POS_10_CURRENT;
kal_int32 cur_jeita_pos_45_to_60 = JEITA_POS_45_TO_POS_60_CURRENT;
kal_int32 cur_terminate_neg_10 = JEITA_NEG_10_TO_POS_0_FULL_CURRENT;

/* mapping table measured with temperature fixed */
kal_int32 t0_zone = TEMPERATURE_T0;
kal_int32 t1_zone = TEMPERATURE_T1;
kal_int32 t2_zone = TEMPERATURE_T2;
kal_int32 t3_zone = TEMPERATURE_T3;

/* Battery Temperature Protection */
kal_int32 t_high_discharge_zone = TEMP_POS_60_THRESHOLD;
kal_int32 t_high_recharge_zone = TEMP_POS_60_THRES_MINUS_X_DEGREE;
kal_int32 t_high_zone = TEMP_POS_45_THRESHOLD;
kal_int32 t_high2middle_zone = TEMP_POS_45_THRES_MINUS_X_DEGREE;
kal_int32 t_middle2low_zone = TEMP_POS_10_THRESHOLD;
kal_int32 t_low_zone = TEMP_POS_10_THRES_PLUS_X_DEGREE;
kal_int32 t_low_discharge_zone = TEMP_POS_0_THRESHOLD;
kal_int32 t_low_recharge_zone = TEMP_POS_0_THRES_PLUS_X_DEGREE;
kal_int32 t_freeze_zone = TEMP_NEG_10_THRESHOLD;
kal_int32 t_freeze2low_zone = TEMP_NEG_10_THRES_PLUS_X_DEGREE;


/* auxadc setting */
int g_R_BAT_SENSE = R_BAT_SENSE;
int g_R_I_SENSE = R_I_SENSE;
int g_R_CHARGER_1 = R_CHARGER_1;
int g_R_CHARGER_2 = R_CHARGER_2;
int g_R_CUST_SENSE = CUST_R_SENSE;

/* fuel gauge resistor value */
int g_R_FG_VALUE = R_FG_VALUE;
int g_R_FG_METER = FG_METER_RESISTANCE;

/* NTC resistor value for pull up & down  */
int g_R_PULL_UP = RBAT_PULL_UP_R;
int g_R_PULL_DOWN = RBAT_PULL_DOWN_R;

/* N percent tracking algorithm */
kal_int32 g_tracking_point = CUST_TRACKING_POINT;

/* Power on capacitydecision making setting */
kal_int32 poweron_hw_percent_diff = CUST_POWERON_DELTA_CAPACITY_TOLRANCE;
kal_int32 poweron_lowest_percent = CUST_POWERON_LOW_CAPACITY_TOLRANCE;
kal_int32 poweron_max_percent = CUST_POWERON_MAX_VBAT_TOLRANCE;
kal_int32 poweron_sw_percent_diff = CUST_POWERON_DELTA_VBAT_TOLRANCE;


/* Dynamic change wake up period of battery thread when suspend (sec) */
kal_int32 normal_wakeup_period = NORMAL_WAKEUP_PERIOD;
kal_int32 low_power_wakeup_period = LOW_POWER_WAKEUP_PERIOD;
kal_int32 close_poweroff_wakeup_period = CLOSE_POWEROFF_WAKEUP_PERIOD;

/* stop charging while in talking mode */
kal_int32 talking_sync_time = TALKING_SYNC_TIME;

/* Tracking TIME (sec) */
kal_int32 t_100percent_sync = ONEHUNDRED_PERCENT_TRACKING_TIME;
kal_int32 t_npercent_sync = NPERCENT_TRACKING_TIME;
kal_int32 t_real_percent_sync = SYNC_TO_REAL_TRACKING_TIME;
kal_int32 t_jeita_sync = CUST_SOC_JEITA_SYNC_TIME;

/*  Fuel gague relative setting */
kal_int32 aging_tuning_value = AGING_TUNING_VALUE;
kal_int32 oam_d5 = OAM_D5;
kal_int32 car_tune_value = CAR_TUNE_VALUE;

/* HW Fuel Gauge specific setting */
kal_int32 current_detect_r_fg = CURRENT_DETECT_R_FG;
kal_int32 min_error_offset = MinErrorOffset;
kal_int32 cust_r_fg_offset = CUST_R_FG_OFFSET;
kal_int32 ocv_board_compesate = OCV_BOARD_COMPESATE;


/* Qmax for battery   */

kal_int32 qmax_pos_50 = Q_MAX_POS_50;
kal_int32 qmax_pos_25 = Q_MAX_POS_25;
kal_int32 qmax_0 = Q_MAX_POS_0;
kal_int32 qmax_neg_10 = Q_MAX_NEG_10;
kal_int32 qmax_pos_50_hcur = Q_MAX_POS_50_H_CURRENT;
kal_int32 qmax_pos_25_hcur = Q_MAX_POS_25_H_CURRENT;
kal_int32 qmax_0_hcur = Q_MAX_POS_0_H_CURRENT;
kal_int32 qmax_neg_10_hcur = Q_MAX_NEG_10_H_CURRENT;

/* fuel gauge algorithm select by hw design {0: SOC_BY_AUXADC, 1: SOC_BY_HW_FG, 2:SOC_BY_SW_FG} */
kal_int32 fg_soc_method = SOC_BY_SW_FG;


BATT_TEMPERATURE *batt_temperature_table;
//BATTERY_PROFILE_STRUC **percent_voltage_table;
BATTERY_PROFILE_STRUC **pv_profile_temperature;
BATTERY_PROFILE_STRUC *pv_empty_profile;
R_PROFILE_STRUC **rv_profile_temperature;
R_PROFILE_STRUC *rv_empty_profile;


/* feature option enable */
kal_uint32 talking_stop_charging_enable = 1;
kal_uint32 low_temperature_protect_enable = 0;
kal_uint32 temperature_recharge_enable = 1;
kal_uint32 low_charge_volt_protect_enable = 0;
kal_uint32 jeita_enable = 1;
kal_uint32 high_battery_volt_enable = 1;
kal_uint32 notify_chr_volt_high_enable = 1;
kal_uint32 notify_temperature_high_enable = 1;
kal_uint32 notify_current_high_enable = 0;
kal_uint32 notify_bat_volt_enable = 0;
kal_uint32 notify_chr_time_enable = 0;
kal_uint32 ext_chr_ic_id = EXT_NONE;


/* others */
kal_uint32 temperature_table_len = 0;
kal_uint32 pv_len = 0;
kal_uint32 rv_len = 0;

kal_uint32 get_ext_chr_id_done = KAL_FALSE;
void ext_chr_para_init(void)
{
	if (battery_cust_data.feature_enable_para[0].label == FEATURE_LABEL_CODE) {
		ext_chr_ic_id =  battery_cust_data.feature_enable_para[0].feature_para[EXT_CHR_SUPPORT_ID];
	}

	get_ext_chr_id_done = KAL_TRUE;
}
kal_int32 batmet_para_init(void)
{
	kal_uint32 i, j;

	/*wake up voltage {VBAT_NORMAL_WAKEUP, VBAT_LOW_POWER_WAKEUP}*/
	if (battery_cust_data.voltage_para[WAKEUP_VOLTAGE].label == VOLTAGE_LABEL_CODE + WAKEUP_VOLTAGE) {
		v_normal_wakeup = battery_cust_data.voltage_para[WAKEUP_VOLTAGE].battery_para[0];
		v_low_power_wakeup = battery_cust_data.voltage_para[WAKEUP_VOLTAGE].battery_para[1];
		printk("WAKEUP_VOLTAGE %d %d\n", v_normal_wakeup, v_low_power_wakeup);
	}
	/*pull up voltage for NTC temperature calculation*/
	if (battery_cust_data.voltage_para[PULL_UP_VOLTAGE].label ==  VOLTAGE_LABEL_CODE + PULL_UP_VOLTAGE) {
		v_pull_up = battery_cust_data.voltage_para[PULL_UP_VOLTAGE].battery_para[0];
		printk(" PULL_UP_VOLTAGE %d\n", v_pull_up);
	}

	/* Linear Charging Threshold*/
	if (battery_cust_data.voltage_para[CHR_STAGE_VOLTAGE].label ==  VOLTAGE_LABEL_CODE + CHR_STAGE_VOLTAGE) {
		v_pre2cc = battery_cust_data.voltage_para[CHR_STAGE_VOLTAGE].battery_para[0];
		v_cc2cv = battery_cust_data.voltage_para[CHR_STAGE_VOLTAGE].battery_para[1];
		v_recharge = battery_cust_data.voltage_para[CHR_STAGE_VOLTAGE].battery_para[2];
		printk(" CHR_STAGE_VOLTAGE %d %d %d\n", v_pre2cc, v_cc2cv, v_recharge);
	}

	/* charger error check*/
	if (battery_cust_data.voltage_para[CHR_THR_VOLTAGE].label ==  VOLTAGE_LABEL_CODE + CHR_THR_VOLTAGE) {
		v_chr_max = battery_cust_data.voltage_para[CHR_THR_VOLTAGE].battery_para[0];
		v_chr_min = battery_cust_data.voltage_para[CHR_THR_VOLTAGE].battery_para[1];
		printk(" CHR_THR_VOLTAGE %d %d\n", v_chr_max, v_chr_min);
	}

	/* stop charging while in talking mode*/
	if (battery_cust_data.voltage_para[TALKING_MODE_RECHARGE_VOLTAGE].label ==  VOLTAGE_LABEL_CODE + TALKING_MODE_RECHARGE_VOLTAGE ) {
		v_recharge_at_talking =  battery_cust_data.voltage_para[TALKING_MODE_RECHARGE_VOLTAGE].battery_para[0];
		printk(" TALKING_MODE_RECHARGE_VOLTAGE %d \n", v_recharge_at_talking);
	}

	/* start 0 percent tracking when reach this value */
	if (battery_cust_data.voltage_para[ZERO_PERCENT_VOLTAGE].label ==  VOLTAGE_LABEL_CODE + ZERO_PERCENT_VOLTAGE  ) {
		v_0percent_sync = battery_cust_data.voltage_para[ZERO_PERCENT_VOLTAGE].battery_para[0];
		printk(" ZERO_PERCENT_VOLTAGE %d \n", v_0percent_sync);
	}

	/* JEITA parameter */
	if (battery_cust_data.voltage_para[JEITA_MODE_CV_VOLTAGE].label ==  VOLTAGE_LABEL_CODE + JEITA_MODE_CV_VOLTAGE  ) {
		cv_above_pos_60 = battery_cust_data.voltage_para[JEITA_MODE_CV_VOLTAGE].battery_para[0];
		cv_pos_45_60 = battery_cust_data.voltage_para[JEITA_MODE_CV_VOLTAGE].battery_para[1];
		cv_pos_10_45 = battery_cust_data.voltage_para[JEITA_MODE_CV_VOLTAGE].battery_para[2];
		cv_pos_0_10 = battery_cust_data.voltage_para[JEITA_MODE_CV_VOLTAGE].battery_para[3];
		cv_neg_10_0 = battery_cust_data.voltage_para[JEITA_MODE_CV_VOLTAGE].battery_para[4];
		cv_below_neg_10 = battery_cust_data.voltage_para[JEITA_MODE_CV_VOLTAGE].battery_para[5];
		printk(" JEITA_MODE_CV_VOLTAGE %d %d %d %d %d %d\n", cv_above_pos_60, cv_pos_45_60, cv_pos_10_45, cv_pos_0_10, cv_neg_10_0, cv_below_neg_10);
	}

	/* For JEITA Linear Charging only */
	if (battery_cust_data.voltage_para[JEITA_LINEAR_VOLTAGE].label ==  VOLTAGE_LABEL_CODE + JEITA_LINEAR_VOLTAGE  ) {
		v_recharge_pos_45_60 = battery_cust_data.voltage_para[JEITA_LINEAR_VOLTAGE].battery_para[0];
		v_recharge_pos_10_45 = battery_cust_data.voltage_para[JEITA_LINEAR_VOLTAGE].battery_para[1];
		v_recharge_cv_pos_0_10  = battery_cust_data.voltage_para[JEITA_LINEAR_VOLTAGE].battery_para[2];
		v_recharge_neg_10_0 = battery_cust_data.voltage_para[JEITA_LINEAR_VOLTAGE].battery_para[3];
		cc2cv_pos_45_60 = battery_cust_data.voltage_para[JEITA_LINEAR_VOLTAGE].battery_para[4];
		cc2cv_pos_10_45  = battery_cust_data.voltage_para[JEITA_LINEAR_VOLTAGE].battery_para[5];
		cc2cv_pos_0_10 = battery_cust_data.voltage_para[JEITA_LINEAR_VOLTAGE].battery_para[6];
		cc2cv_neg_10_0 = battery_cust_data.voltage_para[JEITA_LINEAR_VOLTAGE].battery_para[7];
		printk(" JEITA_MODE_CV_VOLTAGE %d %d %d %d %d %d %d %d\n", v_recharge_pos_45_60, v_recharge_pos_10_45,
			v_recharge_cv_pos_0_10, v_recharge_neg_10_0,  cc2cv_pos_45_60, cc2cv_pos_10_45, cc2cv_pos_0_10, cc2cv_neg_10_0);
	}

	/* Charging Current Setting */
	if (battery_cust_data.current_para[USB_CONF_CURRENT].label == CURRENT_LABEL_CODE +  USB_CONF_CURRENT ) {
		cur_usb_suspend = battery_cust_data.current_para[USB_CONF_CURRENT].battery_para[0];
		cur_usb_unconfigured = battery_cust_data.current_para[USB_CONF_CURRENT].battery_para[1];
		cur_usb_configured = battery_cust_data.current_para[USB_CONF_CURRENT].battery_para[2];
		printk(" USB_CONF_CURRENT %d %d %d\n", cur_usb_suspend, cur_usb_unconfigured, cur_usb_configured);
	}

	/* Charging Current Setting */
	if (battery_cust_data.current_para[USB_TYPE_CURRENT].label == CURRENT_LABEL_CODE +  USB_TYPE_CURRENT  ) {
		cur_usb_charger =  battery_cust_data.current_para[USB_TYPE_CURRENT].battery_para[0];
		cur_charging_host =  battery_cust_data.current_para[USB_TYPE_CURRENT].battery_para[1];
		cur_no_std_charger =  battery_cust_data.current_para[USB_TYPE_CURRENT].battery_para[2];
		cur_ac_charger =  battery_cust_data.current_para[USB_TYPE_CURRENT].battery_para[3];
		cur_apple_2_1A =  battery_cust_data.current_para[USB_TYPE_CURRENT].battery_para[4];
		cur_apple_1A =  battery_cust_data.current_para[USB_TYPE_CURRENT].battery_para[5];
		cur_apple_0_5A =  battery_cust_data.current_para[USB_TYPE_CURRENT].battery_para[6];
		printk(" USB_TYPE_CURRENT %d %d %d %d %d %d %d\n", cur_usb_charger, cur_charging_host, cur_no_std_charger,
			cur_ac_charger, cur_apple_2_1A, cur_apple_1A, cur_apple_0_5A);
	}

	/* For JEITA Linear Charging only */
	if (battery_cust_data.current_para[JEITA_CURRENT].label == CURRENT_LABEL_CODE +  JEITA_CURRENT  ) {
		cur_jeita_neg_10_to_0 = battery_cust_data.current_para[JEITA_CURRENT].battery_para[0];
		cur_jeita_pos_0_to_10 = battery_cust_data.current_para[JEITA_CURRENT].battery_para[1];
		cur_jeita_pos_45_to_60 = battery_cust_data.current_para[JEITA_CURRENT].battery_para[2];
		cur_terminate_neg_10 = battery_cust_data.current_para[JEITA_CURRENT].battery_para[3];
		printk(" JEITA_CURRENT %d %d %d %d\n", cur_jeita_neg_10_to_0, cur_jeita_pos_0_to_10, cur_jeita_pos_45_to_60, cur_terminate_neg_10);
	}

	/* mapping table measured with temperature fixed */
	if (battery_cust_data.temperature_para[TEMPER_FIX_TABLE].label == TEMPER_LABEL_CODE +  TEMPER_FIX_TABLE  ) {
		t0_zone = battery_cust_data.temperature_para[TEMPER_FIX_TABLE].battery_para[0];
		t1_zone = battery_cust_data.temperature_para[TEMPER_FIX_TABLE].battery_para[1];
		t2_zone = battery_cust_data.temperature_para[TEMPER_FIX_TABLE].battery_para[2];
		t3_zone = battery_cust_data.temperature_para[TEMPER_FIX_TABLE].battery_para[3];
		printk(" TEMPER_FIX_TABLE %d %d %d %d\n", t0_zone, t1_zone, t2_zone, t3_zone);
	}

	/* Battery Temperature Protection */
	if (battery_cust_data.temperature_para[TEMPER_PROTECTION].label == TEMPER_LABEL_CODE +  TEMPER_PROTECTION  ) {
		t_high_discharge_zone = battery_cust_data.temperature_para[TEMPER_PROTECTION].battery_para[0];
		t_high_recharge_zone = battery_cust_data.temperature_para[TEMPER_PROTECTION].battery_para[1];
		t_high_zone = battery_cust_data.temperature_para[TEMPER_PROTECTION].battery_para[2];
		t_high2middle_zone = battery_cust_data.temperature_para[TEMPER_PROTECTION].battery_para[3];
		t_middle2low_zone = battery_cust_data.temperature_para[TEMPER_PROTECTION].battery_para[4];
		t_low_zone = battery_cust_data.temperature_para[TEMPER_PROTECTION].battery_para[5];
		t_low_discharge_zone = battery_cust_data.temperature_para[TEMPER_PROTECTION].battery_para[6];
		t_low_recharge_zone = battery_cust_data.temperature_para[TEMPER_PROTECTION].battery_para[7];
		t_freeze_zone = battery_cust_data.temperature_para[TEMPER_PROTECTION].battery_para[8];
		t_freeze2low_zone = battery_cust_data.temperature_para[TEMPER_PROTECTION].battery_para[9];
		printk(" TEMPER_PROTECTION %d %d %d %d %d %d %d %d %d %d\n", t_high_discharge_zone, t_high_recharge_zone,
			t_high_zone, t_high2middle_zone,  t_middle2low_zone, t_low_zone, t_low_discharge_zone, t_low_recharge_zone,
			t_freeze_zone, t_freeze2low_zone);
	}

	/* ADC resistor  */
	if (battery_cust_data.resistor_para[ADC_RESISTOR].label == RESISTOR_LABEL_CODE +  ADC_RESISTOR  ) {
		g_R_BAT_SENSE 	= battery_cust_data.resistor_para[ADC_RESISTOR].battery_para[0];
		g_R_I_SENSE 	= battery_cust_data.resistor_para[ADC_RESISTOR].battery_para[1];
		g_R_CHARGER_1 = battery_cust_data.resistor_para[ADC_RESISTOR].battery_para[2];
		g_R_CHARGER_2 = battery_cust_data.resistor_para[ADC_RESISTOR].battery_para[3];
		g_R_CUST_SENSE = battery_cust_data.resistor_para[ADC_RESISTOR].battery_para[4];
		printk(" ADC_RESISTOR %d %d %d %d %d\n", g_R_BAT_SENSE, g_R_I_SENSE, g_R_CHARGER_1, g_R_CHARGER_2, g_R_CUST_SENSE);
	}

	/* fuel gauge resistor value */
	if (battery_cust_data.resistor_para[FG_METER_RESISTOR].label == RESISTOR_LABEL_CODE +  FG_METER_RESISTOR  ) {
		g_R_FG_VALUE =  battery_cust_data.resistor_para[FG_METER_RESISTOR].battery_para[0];
		g_R_FG_METER =  battery_cust_data.resistor_para[FG_METER_RESISTOR].battery_para[1];
		printk(" FG_METER_RESISTOR %d %d \n", g_R_FG_VALUE, g_R_FG_METER);
	}


	/* NTC resistor value for pull up & down  */
	if (battery_cust_data.resistor_para[PULL_RESISTOR].label == RESISTOR_LABEL_CODE +  PULL_RESISTOR  ) {
		g_R_PULL_UP = battery_cust_data.resistor_para[PULL_RESISTOR].battery_para[0];
		g_R_PULL_DOWN = battery_cust_data.resistor_para[PULL_RESISTOR].battery_para[1];
		printk(" PULL_RESISTOR %d %d\n", g_R_PULL_UP, g_R_PULL_DOWN);
	}

	/* N percent tracking algorithm */
	if (battery_cust_data.percent_para[TRACKING_PERCNET].label == PERCENTAGE_LABEL_CODE +  TRACKING_PERCNET  ) {
		g_tracking_point = battery_cust_data.percent_para[TRACKING_PERCNET].battery_para[0];
		printk(" TRACKING_PERCNET  %d\n", g_tracking_point);
	}

	/* Power on capacitydecision making setting */
	if (battery_cust_data.percent_para[FG_COMP_PERCENT].label == PERCENTAGE_LABEL_CODE +  FG_COMP_PERCENT  ) {
		poweron_hw_percent_diff = battery_cust_data.percent_para[FG_COMP_PERCENT].battery_para[0];
		poweron_lowest_percent = battery_cust_data.percent_para[FG_COMP_PERCENT].battery_para[1];
		poweron_max_percent = battery_cust_data.percent_para[FG_COMP_PERCENT].battery_para[2];
		poweron_sw_percent_diff = battery_cust_data.percent_para[FG_COMP_PERCENT].battery_para[3];
		printk(" FG_COMP_PERCENT %d %d %d %d\n", poweron_hw_percent_diff, poweron_lowest_percent, poweron_max_percent, poweron_sw_percent_diff);
	}

	/* Dynamic change wake up period of battery thread when suspend (sec) */
	if (battery_cust_data.timer_para[WAKEUP_TIMER].label == TIMER_LABEL_CODE +  WAKEUP_TIMER  ) {
		normal_wakeup_period = battery_cust_data.timer_para[WAKEUP_TIMER].battery_para[0];
		low_power_wakeup_period = battery_cust_data.timer_para[WAKEUP_TIMER].battery_para[1];
		close_poweroff_wakeup_period = battery_cust_data.timer_para[WAKEUP_TIMER].battery_para[2];
		printk(" WAKEUP_TIMER %d %d %d\n", normal_wakeup_period, low_power_wakeup_period, close_poweroff_wakeup_period);
	}
	/* stop charging while in talking mode */
	if (battery_cust_data.timer_para[TALKING_TIMER].label == TIMER_LABEL_CODE +  TALKING_TIMER  ) {
		talking_sync_time = battery_cust_data.timer_para[TALKING_TIMER].battery_para[0];
		printk(" TALKING_TIMER  %d\n", talking_sync_time);
	}

	/* Tracking TIME (sec) */
	if (battery_cust_data.timer_para[PERCENT_TRACKING_TIMER].label == TIMER_LABEL_CODE +  PERCENT_TRACKING_TIMER  ) {
		t_100percent_sync = battery_cust_data.timer_para[PERCENT_TRACKING_TIMER].battery_para[0];
		t_npercent_sync = battery_cust_data.timer_para[PERCENT_TRACKING_TIMER].battery_para[1];
		t_real_percent_sync = battery_cust_data.timer_para[PERCENT_TRACKING_TIMER].battery_para[2];
		t_jeita_sync = battery_cust_data.timer_para[PERCENT_TRACKING_TIMER].battery_para[3];
		printk(" PERCENT_TRACKING_TIMER %d %d %d %d\n", t_100percent_sync, t_npercent_sync, t_real_percent_sync, t_jeita_sync);
	}
	/*  Fuel gague relative setting */
	if (battery_cust_data.fg_tuning_para[FUEL_GAUGE_TUNING].label == FG_TUNING_LABEL_CODE +  FUEL_GAUGE_TUNING  ) {
		aging_tuning_value = battery_cust_data.fg_tuning_para[FUEL_GAUGE_TUNING].battery_para[0];
		oam_d5 = battery_cust_data.fg_tuning_para[FUEL_GAUGE_TUNING].battery_para[1];
		car_tune_value = battery_cust_data.fg_tuning_para[FUEL_GAUGE_TUNING].battery_para[2];
		printk(" FUEL_GAUGE_TUNING %d %d %d\n", aging_tuning_value, oam_d5, car_tune_value);
	}

	/* HW Fuel Gauge specific setting */
	if (battery_cust_data.fg_tuning_para[HW_FG_TUNING].label == FG_TUNING_LABEL_CODE +  HW_FG_TUNING  ) {
		current_detect_r_fg = battery_cust_data.fg_tuning_para[HW_FG_TUNING].battery_para[0];
		min_error_offset = battery_cust_data.fg_tuning_para[HW_FG_TUNING].battery_para[1];
		cust_r_fg_offset = battery_cust_data.fg_tuning_para[HW_FG_TUNING].battery_para[2];
		ocv_board_compesate = battery_cust_data.fg_tuning_para[HW_FG_TUNING].battery_para[3];
		printk(" HW_FG_TUNING %d %d %d %d\n", current_detect_r_fg, min_error_offset, cust_r_fg_offset, ocv_board_compesate);
	}

	/* Qmax for battery   */
	if (battery_cust_data.fg_tuning_para[CAPACITY_TUNING].label == FG_TUNING_LABEL_CODE +  CAPACITY_TUNING  ) {
		qmax_pos_50 = battery_cust_data.fg_tuning_para[CAPACITY_TUNING].battery_para[0]; 
		qmax_pos_25 = battery_cust_data.fg_tuning_para[CAPACITY_TUNING].battery_para[1];
		qmax_0 = battery_cust_data.fg_tuning_para[CAPACITY_TUNING].battery_para[2];
		qmax_neg_10 = battery_cust_data.fg_tuning_para[CAPACITY_TUNING].battery_para[3];
		qmax_pos_50_hcur = battery_cust_data.fg_tuning_para[CAPACITY_TUNING].battery_para[4];  
		qmax_pos_25_hcur = battery_cust_data.fg_tuning_para[CAPACITY_TUNING].battery_para[5];
		qmax_0_hcur = battery_cust_data.fg_tuning_para[CAPACITY_TUNING].battery_para[6];
		qmax_neg_10_hcur = battery_cust_data.fg_tuning_para[CAPACITY_TUNING].battery_para[7];
		printk(" CAPACITY_TUNING %d %d %d %d %d %d %d %d\n", qmax_pos_50, qmax_pos_25, qmax_0, qmax_neg_10,
			qmax_pos_50_hcur, qmax_pos_25_hcur, qmax_0_hcur, qmax_neg_10_hcur);
	}

	/* fuel gauge algorithm select by hw design {0: SOC_BY_AUXADC, 1: SOC_BY_HW_FG, 2:SOC_BY_SW_FG} */
	if (battery_cust_data.fg_tuning_para[GAUGE_SELECT].label == FG_TUNING_LABEL_CODE +  GAUGE_SELECT  ) {
		fg_soc_method = battery_cust_data.fg_tuning_para[GAUGE_SELECT].battery_para[0];
		printk(" GAUGE_SELECT %d\n", fg_soc_method);
	}

	if (battery_cust_data.temperature_resist_table[0].label == TR_TABLE_LABEL_CODE) {
		temperature_table_len = battery_cust_data.temperature_resist_table[0].len;
		batt_temperature_table = kmalloc(temperature_table_len* sizeof (BATT_TEMPERATURE), GFP_KERNEL);
		for (i=0; i< temperature_table_len; i++) {
			batt_temperature_table[i].BatteryTemp = battery_cust_data.temperature_resist_table[0].mapping_data[i].para_first;
			batt_temperature_table[i].TemperatureR = battery_cust_data.temperature_resist_table[0].mapping_data[i].para_second;
			printk(" TR_TABLE_LABEL_CODE [%d]= ( %d, %d)\n", i, batt_temperature_table[i].BatteryTemp, batt_temperature_table[i].TemperatureR);

		}
	} else {
		temperature_table_len = sizeof(Batt_Temperature_Table) / sizeof(BATT_TEMPERATURE);
		batt_temperature_table = kmalloc(temperature_table_len* sizeof (BATT_TEMPERATURE), GFP_KERNEL);
		batt_temperature_table = Batt_Temperature_Table;

	}


	pv_profile_temperature = kmalloc(T_NUM * sizeof (BATTERY_PROFILE_STRUC *), GFP_KERNEL);
	for (i=0; i< T_NUM; i++) {
		pv_profile_temperature[i] = kmalloc(battery_cust_data.percent_volt_table[i].len* sizeof (BATTERY_PROFILE_STRUC), GFP_KERNEL);

		if (battery_cust_data.percent_volt_table[i].label == PV_TABLE_LABEL_CODE + i ) {
			pv_len = battery_cust_data.percent_volt_table[0].len;
			for (j=0; j< pv_len; j++) {

				pv_profile_temperature[i][j].percentage = battery_cust_data.percent_volt_table[i].mapping_data[j].para_first;
				pv_profile_temperature[i][j].voltage = battery_cust_data.percent_volt_table[i].mapping_data[j].para_second;
				//printk(" PV_TABLE_LABEL_CODE [%d, %d]= ( %d, %d)\n", i, j, pv_profile_temperature[i][j].percentage, pv_profile_temperature[i][j].voltage );
			}
		} else {
			pv_profile_temperature[0] = battery_profile_t0;
			pv_profile_temperature[1] = battery_profile_t1;
			pv_profile_temperature[2] = battery_profile_t2;
			pv_profile_temperature[3] = battery_profile_t3;
			pv_len = sizeof(battery_profile_t0) / sizeof(BATTERY_PROFILE_STRUC);

		}
	}
	pv_empty_profile = kmalloc(pv_len* sizeof (BATTERY_PROFILE_STRUC), GFP_KERNEL);
	memset(pv_empty_profile, 0, pv_len* sizeof (BATTERY_PROFILE_STRUC));

	rv_profile_temperature = kmalloc(T_NUM * sizeof (R_PROFILE_STRUC *), GFP_KERNEL);

	for (i=0; i< T_NUM; i++) {
		rv_profile_temperature[i] = kmalloc(battery_cust_data.percent_volt_table[i].len* sizeof (R_PROFILE_STRUC), GFP_KERNEL);
		if (battery_cust_data.resist_volt_table[i].label == RV_TABLE_LABEL_CODE + i ) {
			for (j=0; j< battery_cust_data.percent_volt_table[i].len; j++) {
				rv_profile_temperature[i][j].resistance = battery_cust_data.resist_volt_table[i].mapping_data[j].para_first;
				rv_profile_temperature[i][j].voltage= battery_cust_data.resist_volt_table[i].mapping_data[j].para_second;
				//printk(" RV_TABLE_LABEL_CODE [%d, %d]= ( %d, %d)\n", i, j, rv_profile_temperature[i][j].resistance, rv_profile_temperature[i][j].voltage);
			}
			rv_len = battery_cust_data.resist_volt_table[0].len;
		} else {
			rv_profile_temperature[0] = r_profile_t0;
			rv_profile_temperature[1] = r_profile_t1;
			rv_profile_temperature[2] = r_profile_t2;
			rv_profile_temperature[3] = r_profile_t3;
			rv_len = sizeof(r_profile_t0) / sizeof(R_PROFILE_STRUC);
		}
	}

	rv_empty_profile = kmalloc(rv_len* sizeof (R_PROFILE_STRUC), GFP_KERNEL);
	memset(rv_empty_profile, 0, rv_len* sizeof (R_PROFILE_STRUC));

	if (battery_cust_data.feature_enable_para[0].label == FEATURE_LABEL_CODE) {
		talking_stop_charging_enable = battery_cust_data.feature_enable_para[0].feature_para[TALKING_STOP_CHARGE_ENABLE];
		low_temperature_protect_enable = battery_cust_data.feature_enable_para[0].feature_para[LOW_TEMPERATURE_ENABLE];
		temperature_recharge_enable = battery_cust_data.feature_enable_para[0].feature_para[TEMPERAURE_RECHARGE_ENABLE];
		low_charge_volt_protect_enable = battery_cust_data.feature_enable_para[0].feature_para[LOW_CHARGE_VOLT_ENABLE];
		jeita_enable = battery_cust_data.feature_enable_para[0].feature_para[JEITA_ENABLE];
		high_battery_volt_enable = battery_cust_data.feature_enable_para[0].feature_para[HIGH_BATTERY_ENABLE];
		notify_chr_volt_high_enable = battery_cust_data.feature_enable_para[0].feature_para[NOTIFY_CHR_VOLT_HIGH];
		notify_temperature_high_enable = battery_cust_data.feature_enable_para[0].feature_para[NOTIFY_TEMP_HIGH];
		notify_current_high_enable = battery_cust_data.feature_enable_para[0].feature_para[NOTIFY_CURRENT_HIGH];
		notify_bat_volt_enable = battery_cust_data.feature_enable_para[0].feature_para[NOTIFY_BAT_VOLT_HIGH];
		notify_chr_time_enable = battery_cust_data.feature_enable_para[0].feature_para[NOTIFY_CHR_TIME_LONG];
		ext_chr_ic_id =  battery_cust_data.feature_enable_para[0].feature_para[EXT_CHR_SUPPORT_ID];
	}

	return 0;
}

void parse_sub_para_buf(feature_enable_type *feature_data, battery_store_type *store_data, mapping_table_type *table_data, unsigned int *buf, unsigned int type, unsigned int label_code,  unsigned int num)
{
	U32 i, j, index;
	U32 next_sub_label=0;
	U32 sub_label;
	U32 sub_para_len;

	for (i=0; i< num; i++) {
		sub_label = buf[next_sub_label];
		sub_para_len = buf[next_sub_label+1];
		index = (sub_label - label_code);

		printk( "[parse_sub_para_buf] label = %d, len = %d, index = %d\n", sub_label, sub_para_len, index);
		if ( type ==0 ) {
			feature_data[index].label = sub_label;
			feature_data[index].para_len = sub_para_len;
			for (j=0; j<sub_para_len; j++) {
				feature_data[index].feature_para[j] = buf[next_sub_label+2+j];
				printk( "[parse_sub_para_buf] type 0\n");
				printk( "[%d] = %d \n", j, feature_data[index].feature_para[j]);
			}
		} else if (type == 1) {
			store_data[index].label= sub_label;
			store_data[index].para_len= sub_para_len;
			for (j=0; j<sub_para_len; j++) {
				store_data[index].battery_para[j] = buf[next_sub_label+2+j];
				//printk( "[parse_sub_para_buf] type 1\n");
				//printk( "[%d] = %d \n", j, store_data[index].battery_para[j]);
			}
		} else if (type == 2) {
			table_data[index].label= sub_label;
			table_data[index].len= sub_para_len;
			for (j=0; j<sub_para_len; j++) {
				table_data[index].mapping_data[j].para_first= buf[next_sub_label+2+j*2];
				table_data[index].mapping_data[j].para_second= buf[next_sub_label+3+j*2];
				//printk( "[parse_sub_para_buf] type 2\n");
				//printk( "[%d] = (%d %d) \n", j, table_data[index].mapping_data[j].para_first, table_data[index].mapping_data[j].para_second);
			}
			j *= 2;
		} else {
			printk( "[parse_sub_para_buf] type error\n");
			return ;
		}
		next_sub_label += 2+j;
		printk( "[parse_sub_para_buf] next_sub_label = %d\n", next_sub_label);
	}
}

int parsing_battery_init_para(U32 label_code)
{
	U32 version;
	U32 i;
	U32 label, index;

	version = battery_cust_buf[0];
	//memset (battery_cust_data, 0xff, sizeof(battery_cust_data));
	printk( "[parsing_battery_init_para] version = %d\n", version);
	if (version == 1) {
		for (i=0; i< BATTERY_NODE_CNT; i++) {
			label = battery_cust_buf[i*2+1];
			index = battery_cust_buf[i*2+2] / 4;
			printk("[parsing_battery_init_para] label = %d, len = %d\n", label, index);
			if (label_code == NULL || label == label_code) {
				switch (label) {
				case FEATURE_LABEL_CODE:
					parse_sub_para_buf( battery_cust_data.feature_enable_para, NULL, NULL, battery_cust_buf + index, 0, FEATURE_LABEL_CODE, 1);
					ext_chr_para_init();
					break;
				case VOLTAGE_LABEL_CODE:
					parse_sub_para_buf( NULL, battery_cust_data.voltage_para, NULL, battery_cust_buf + index, 1, VOLTAGE_LABEL_CODE, VOLTAGE_NUM);
					break;
				case CURRENT_LABEL_CODE:
					parse_sub_para_buf( NULL, battery_cust_data.current_para, NULL, battery_cust_buf + index, 1, CURRENT_LABEL_CODE, CURRENT_NUM);
					break;
				case TEMPER_LABEL_CODE:
					parse_sub_para_buf( NULL, battery_cust_data.temperature_para, NULL, battery_cust_buf + index, 1, TEMPER_LABEL_CODE, TEMPER_NUM);
					break;
				case TIMER_LABEL_CODE:
					parse_sub_para_buf( NULL, battery_cust_data.timer_para, NULL, battery_cust_buf + index, 1, TIMER_LABEL_CODE, TIMER_NUM);
					break;
				case PERCENTAGE_LABEL_CODE:
					parse_sub_para_buf( NULL, battery_cust_data.percent_para, NULL, battery_cust_buf + index, 1, PERCENTAGE_LABEL_CODE, PERCENT_NUM);
					break;
				case RESISTOR_LABEL_CODE:
					parse_sub_para_buf( NULL, battery_cust_data.resistor_para, NULL, battery_cust_buf + index, 1, RESISTOR_LABEL_CODE, RESISTOR_NUM);
					break;
				case FG_TUNING_LABEL_CODE:
					parse_sub_para_buf( NULL, battery_cust_data.fg_tuning_para, NULL, battery_cust_buf + index, 1, FG_TUNING_LABEL_CODE, FG_NUM);
					break;
				case TR_TABLE_LABEL_CODE:
					parse_sub_para_buf( NULL, NULL, battery_cust_data.temperature_resist_table, battery_cust_buf + index, 2, TR_TABLE_LABEL_CODE, 1);
					break;
				case PV_TABLE_LABEL_CODE:
					parse_sub_para_buf( NULL, NULL, battery_cust_data.percent_volt_table, battery_cust_buf + index, 2, PV_TABLE_LABEL_CODE, T_NUM);
					break;
				case RV_TABLE_LABEL_CODE:
					parse_sub_para_buf( NULL, NULL, battery_cust_data.resist_volt_table, battery_cust_buf + index, 2, RV_TABLE_LABEL_CODE, T_NUM);
					break;
				default:
					printk("[parsing_battery_init_para] no label matched\n" );
					break;
				}
			}
		}
	}
	return 0;
}

