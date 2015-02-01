#ifndef _BATTERY_SSB_H
#define _BATTERY_SSB_H

#include <mach/mt_typedefs.h>
#include <mach/battery_meter.h>

// ============================================================
// define
// ============================================================

#define FEATURE_LABEL_CODE	0x4300
#define VOLTAGE_LABEL_CODE	0x4400
#define CURRENT_LABEL_CODE	0x4500
#define TEMPER_LABEL_CODE	0x4600
#define TIMER_LABEL_CODE	0x4700
#define PERCENTAGE_LABEL_CODE	0x4800
#define RESISTOR_LABEL_CODE	0x4900
#define FG_TUNING_LABEL_CODE	0x4a00
#define TR_TABLE_LABEL_CODE	0x4b00
#define PV_TABLE_LABEL_CODE	0x4c00
#define RV_TABLE_LABEL_CODE	0x4d00

#define BATTERY_NODE_CNT 11

// ============================================================
// ENUM
// ============================================================

enum {
	T0_NEG_10C,
	T1_0C,
	T2_25C,
	T3_50C,
	T_NUM
};

enum {
	TALKING_STOP_CHARGE_ENABLE,
	LOW_TEMPERATURE_ENABLE,
	TEMPERAURE_RECHARGE_ENABLE,
	LOW_CHARGE_VOLT_ENABLE,
	JEITA_ENABLE,
	HIGH_BATTERY_ENABLE,
	NOTIFY_CHR_VOLT_HIGH,
	NOTIFY_TEMP_HIGH,
	NOTIFY_CURRENT_HIGH,
	NOTIFY_BAT_VOLT_HIGH,
	NOTIFY_CHR_TIME_LONG,
	EXT_CHR_SUPPORT_ID,
	FEATURE_NUM
};

enum {
	WAKEUP_VOLTAGE,
	PULL_UP_VOLTAGE,
	CHR_STAGE_VOLTAGE,
	CHR_THR_VOLTAGE,
	TALKING_MODE_RECHARGE_VOLTAGE,
	ZERO_PERCENT_VOLTAGE,
	JEITA_MODE_CV_VOLTAGE,
	JEITA_LINEAR_VOLTAGE,
	VOLTAGE_NUM
};

enum {
	USB_CONF_CURRENT,
	USB_TYPE_CURRENT,
	JEITA_CURRENT,
	CURRENT_NUM
};

enum {
	TEMPER_FIX_TABLE,
	TEMPER_PROTECTION,
	TEMPER_NUM
};

enum {
	ADC_RESISTOR,
	FG_METER_RESISTOR,
	PULL_RESISTOR,
	RESISTOR_NUM
};

enum {
	TRACKING_PERCNET,
	FG_COMP_PERCENT,
	PERCENT_NUM
};

enum {
	WAKEUP_TIMER,
	TALKING_TIMER,
	PERCENT_TRACKING_TIMER,
	TIMER_NUM
};

enum {
	FUEL_GAUGE_TUNING,
	HW_FG_TUNING,
	CAPACITY_TUNING,
	GAUGE_SELECT,
	FG_NUM
};


// ============================================================
// typedef
// ============================================================
typedef struct {
	unsigned int	label;
	unsigned int	offset;
} battery_list;

typedef struct {
	unsigned int	label;
	unsigned int	para_len;
	unsigned int	battery_para[10];
} battery_store_type;

typedef struct {
	int		para_first;
	int		para_second;
} table_type;

typedef struct  {
	unsigned int		label;
	unsigned int		len;
	table_type		mapping_data[80];
} mapping_table_type;

typedef struct {
	unsigned int		label;
	unsigned int		para_len;
	unsigned int		feature_para[FEATURE_NUM];
} feature_enable_type;

typedef struct {
	battery_store_type			voltage_para[VOLTAGE_NUM];
	battery_store_type			current_para[CURRENT_NUM];
	battery_store_type			temperature_para[TEMPER_NUM];
	battery_store_type			resistor_para[RESISTOR_NUM];
	battery_store_type			percent_para[PERCENT_NUM];
	battery_store_type			timer_para[TIMER_NUM];
	battery_store_type			fg_tuning_para[FG_NUM];
	mapping_table_type		temperature_resist_table[1];
	mapping_table_type		percent_volt_table[T_NUM];
	mapping_table_type		resist_volt_table[T_NUM];
	feature_enable_type		feature_enable_para[1];
} battery_data_type;

typedef struct {
	unsigned int version;
	battery_list  battery_node[BATTERY_NODE_CNT];
} battery_header;

/* ============================================================ */
/* External Variables */
/* ============================================================ */
extern battery_header battery_hdr;
extern battery_data_type battery_cust_data;
extern unsigned int battery_cust_buf[MAX_BATTERY_PARA_SIZE];

/*wake up voltage {VBAT_NORMAL_WAKEUP, VBAT_LOW_POWER_WAKEUP}*/

extern kal_int32 v_normal_wakeup;
extern kal_int32 v_low_power_wakeup;

/*pull up voltage for NTC temperature calculation*/
extern kal_int32 v_pull_up;

/* Linear Charging Threshold*/
extern kal_int32 v_pre2cc;
extern kal_int32 v_cc2cv;
extern kal_int32 v_recharge;

/* charger error check*/
extern kal_int32 v_chr_max;
extern kal_int32 v_chr_min;

/* stop charging while in talking mode*/
extern kal_int32 v_recharge_at_talking;

/* start 0 percent tracking when reach this value */
extern kal_int32 v_0percent_sync;

/* JEITA parameter */
extern kal_int32 cv_above_pos_60;
extern kal_int32 cv_pos_45_60;
extern kal_int32 cv_pos_10_45;
extern kal_int32 cv_pos_0_10;
extern kal_int32 cv_neg_10_0;
extern kal_int32 cv_below_neg_10;


/* For JEITA Linear Charging only */
extern kal_int32 v_recharge_pos_45_60;
extern kal_int32 v_recharge_pos_10_45;
extern kal_int32 v_recharge_cv_pos_0_10;
extern kal_int32 v_recharge_neg_10_0;
extern kal_int32 cc2cv_pos_45_60;
extern kal_int32 cc2cv_pos_10_45;
extern kal_int32 cc2cv_pos_0_10;
extern kal_int32 cc2cv_neg_10_0;

/* Charging Current Setting */
extern kal_int32 cur_usb_suspend;
extern kal_int32 cur_usb_unconfigured;
extern kal_int32 cur_usb_configured;

/* Charging Current Setting */
extern kal_int32 cur_usb_charger;
extern kal_int32 cur_charging_host;
extern kal_int32 cur_no_std_charger;
extern kal_int32 cur_ac_charger;
extern kal_int32 cur_apple_2_1A;
extern kal_int32 cur_apple_1A;
extern kal_int32 cur_apple_0_5A;

/* For JEITA Linear Charging only */
extern kal_int32 cur_jeita_neg_10_to_0;
extern kal_int32 cur_jeita_pos_0_to_10;
extern kal_int32 cur_jeita_pos_45_to_60;
extern kal_int32 cur_terminate_neg_10;

/* mapping table measured with temperature fixed */
extern kal_int32 t0_zone;
extern kal_int32 t1_zone;
extern kal_int32 t2_zone;
extern kal_int32 t3_zone;

/* Battery Temperature Protection */
extern kal_int32 t_high_discharge_zone;
extern kal_int32 t_high_recharge_zone;
extern kal_int32 t_high_zone;
extern kal_int32 t_high2middle_zone;
extern kal_int32 t_middle2low_zone;
extern kal_int32 t_low_zone;
extern kal_int32 t_low_discharge_zone;
extern kal_int32 t_low_recharge_zone;
extern kal_int32 t_freeze_zone;
extern kal_int32 t_freeze2low_zone;


/* auxadc setting */
extern int g_R_BAT_SENSE;
extern int g_R_I_SENSE;
extern int g_R_CHARGER_1;
extern int g_R_CHARGER_2;
extern int g_R_CUST_SENSE;

/* fuel gauge resistor value */
extern int g_R_FG_VALUE;
extern int g_R_FG_METER;

/* NTC resistor value for pull up & down  */
extern int g_R_PULL_UP;
extern int g_R_PULL_DOWN;

/* N percent tracking algorithm */
extern kal_int32 g_tracking_point;

/* Power on capacitydecision making setting */
extern kal_int32 poweron_hw_percent_diff;
extern kal_int32 poweron_lowest_percent;
extern kal_int32 poweron_max_percent;
extern kal_int32 poweron_sw_percent_diff;


/* Dynamic change wake up period of battery thread when suspend (sec) */
extern kal_int32 normal_wakeup_period;
extern kal_int32 low_power_wakeup_period;
extern kal_int32 close_poweroff_wakeup_period;

/* stop charging while in talking mode */
extern kal_int32 talking_sync_time;

/* Tracking TIME (sec) */
extern kal_int32 t_100percent_sync;
extern kal_int32 t_npercent_sync;
extern kal_int32 t_real_percent_sync;
extern kal_int32 t_jeita_sync;
/*  Fuel gague relative setting */
extern kal_int32 aging_tuning_value;
extern kal_int32 oam_d5;
extern kal_int32 car_tune_value;

/* HW Fuel Gauge specific setting */
extern kal_int32 current_detect_r_fg;
extern kal_int32 min_error_offset;
extern kal_int32 fg_vbat_avg_size;
extern kal_int32 cust_r_fg_offset;
extern kal_int32 ocv_board_compesate;


/* Qmax for battery   */

extern kal_int32 qmax_pos_50;
extern kal_int32 qmax_pos_25;
extern kal_int32 qmax_0;
extern kal_int32 qmax_neg_10;
extern kal_int32 qmax_pos_50_hcur;
extern kal_int32 qmax_pos_25_hcur;
extern kal_int32 qmax_0_hcur;
extern kal_int32 qmax_neg_10_hcur;

/* fuel gauge algorithm select by hw design {0: SOC_BY_AUXADC, 1: SOC_BY_HW_FG, 2:SOC_BY_SW_FG} */
extern kal_int32 fg_soc_method;

extern BATT_TEMPERATURE *batt_temperature_table;
extern BATTERY_PROFILE_STRUC **pv_profile_temperature;
extern R_PROFILE_STRUC **rv_profile_temperature;
extern BATTERY_PROFILE_STRUC *pv_empty_profile;
extern R_PROFILE_STRUC *rv_empty_profile;

/* feature option enable */
extern kal_uint32 talking_stop_charging_enable;
extern kal_uint32 low_temperature_protect_enable;
extern kal_uint32 temperature_recharge_enable;
extern kal_uint32 low_charge_volt_protect_enable;
extern kal_uint32 jeita_enable;
extern kal_uint32 high_battery_volt_enable;
extern kal_uint32 notify_chr_volt_high_enable;
extern kal_uint32 notify_temperature_high_enable;
extern kal_uint32 notify_current_high_enable;
extern kal_uint32 notify_bat_volt_enable;
extern kal_uint32 notify_chr_time_enable;
extern kal_uint32 ext_chr_ic_id;

/* others */
extern kal_uint32 temperature_table_len;
extern kal_uint32 pv_len;
extern kal_uint32 rv_len;
extern kal_uint32 ext_chr_id_done;

/* ============================================================ */
/* External function */
/* ============================================================ */
extern kal_int32 batmet_para_init(void);
extern void parse_sub_para_buf(feature_enable_type *feature_data, battery_store_type *store_data,
		mapping_table_type *table_data, unsigned int *buf, unsigned int type, unsigned int label_code,  unsigned int num);
extern int parsing_battery_init_para(U32 label_code);


#endif				/* #ifndef _BATTERY_METER_H */

