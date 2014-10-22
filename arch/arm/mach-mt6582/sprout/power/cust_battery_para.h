#ifndef __CUST_BATTERY_PARA_H__
#define __CUST_BATTERY_PARA_H__


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
	table_type		mapping_data[59];
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

#endif