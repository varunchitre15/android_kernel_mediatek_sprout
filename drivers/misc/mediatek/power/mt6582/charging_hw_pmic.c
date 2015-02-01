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
#include <mach/upmu_common.h>
#include <mach/upmu_hw.h>
#include <linux/xlog.h>
#include <linux/delay.h>
#include <mach/mt_sleep.h>
#include <mach/mt_boot.h>
#include <mach/system.h>

#include "cust_battery_meter.h"
#include <cust_charging.h>
#include "mach/battery_ssb.h"
#include <mach/charging_hw_common.h>

 // ============================================================ //
 //global variable
 // ============================================================ //

const kal_uint32 VBAT_CV_VTH_PMIC[]=
{
	BATTERY_VOLT_04_200000_V,   BATTERY_VOLT_04_212500_V,	BATTERY_VOLT_04_225000_V,   BATTERY_VOLT_04_237500_V,
	BATTERY_VOLT_04_250000_V,   BATTERY_VOLT_04_262500_V,	BATTERY_VOLT_04_275000_V,   BATTERY_VOLT_04_300000_V,
	BATTERY_VOLT_04_325000_V,	BATTERY_VOLT_04_350000_V,	BATTERY_VOLT_04_375000_V,	BATTERY_VOLT_04_400000_V,
	BATTERY_VOLT_04_425000_V,   BATTERY_VOLT_04_162500_V,	BATTERY_VOLT_04_175000_V,   BATTERY_VOLT_02_200000_V,
	BATTERY_VOLT_04_050000_V,   BATTERY_VOLT_04_100000_V,	BATTERY_VOLT_04_125000_V,   BATTERY_VOLT_03_775000_V,	
	BATTERY_VOLT_03_800000_V,	BATTERY_VOLT_03_850000_V,	BATTERY_VOLT_03_900000_V,	BATTERY_VOLT_04_000000_V,
	BATTERY_VOLT_04_050000_V,	BATTERY_VOLT_04_100000_V,	BATTERY_VOLT_04_125000_V,	BATTERY_VOLT_04_137500_V,
	BATTERY_VOLT_04_150000_V,	BATTERY_VOLT_04_162500_V,	BATTERY_VOLT_04_175000_V,	BATTERY_VOLT_04_187500_V

};

const kal_uint32 CS_VTH_PMIC[]=
{
	CHARGE_CURRENT_1600_00_MA,   CHARGE_CURRENT_1500_00_MA,	CHARGE_CURRENT_1400_00_MA, CHARGE_CURRENT_1300_00_MA,
	CHARGE_CURRENT_1200_00_MA,   CHARGE_CURRENT_1100_00_MA,	CHARGE_CURRENT_1000_00_MA, CHARGE_CURRENT_900_00_MA,
	CHARGE_CURRENT_800_00_MA,   CHARGE_CURRENT_700_00_MA,	CHARGE_CURRENT_650_00_MA, CHARGE_CURRENT_550_00_MA,
	CHARGE_CURRENT_450_00_MA,   CHARGE_CURRENT_300_00_MA,	CHARGE_CURRENT_200_00_MA, CHARGE_CURRENT_70_00_MA
}; 


const kal_uint32 VCDT_HV_VTH_PMIC[]=
{
	 BATTERY_VOLT_04_200000_V, BATTERY_VOLT_04_250000_V,	 BATTERY_VOLT_04_300000_V,	 BATTERY_VOLT_04_350000_V,
	 BATTERY_VOLT_04_400000_V, BATTERY_VOLT_04_450000_V,	 BATTERY_VOLT_04_500000_V,	 BATTERY_VOLT_04_550000_V,
	 BATTERY_VOLT_04_600000_V, BATTERY_VOLT_06_000000_V,	 BATTERY_VOLT_06_500000_V,	 BATTERY_VOLT_07_000000_V,
	 BATTERY_VOLT_07_500000_V, BATTERY_VOLT_08_500000_V,	 BATTERY_VOLT_09_500000_V,	 BATTERY_VOLT_10_500000_V		 
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
 extern kal_uint32 upmu_get_reg_value(kal_uint32 reg);
 extern void Charger_Detect_Init(void);
 extern void Charger_Detect_Release(void);
 extern int PMIC_IMM_GetOneChannelValue(int dwChannel, int deCount, int trimd);
 extern void mt_power_off(void);
 
 // ============================================================ //

static kal_uint32 charging_hw_init_pmic(void *data)
{
	kal_uint32 status = STATUS_OK;
#if defined(MTK_WIRELESS_CHARGER_SUPPORT)
	mt_set_gpio_mode(wireless_charger_gpio_number,0); // 0:GPIO mode
	mt_set_gpio_dir(wireless_charger_gpio_number,0); // 0: input, 1: output
#endif
	upmu_set_rg_chrwdt_td(0x0);           // CHRWDT_TD, 4s
	upmu_set_rg_chrwdt_int_en(1);         // CHRWDT_INT_EN
	upmu_set_rg_chrwdt_en(1);             // CHRWDT_EN
	upmu_set_rg_chrwdt_wr(1);             // CHRWDT_WR

	upmu_set_rg_vcdt_mode(0);       //VCDT_MODE
	upmu_set_rg_vcdt_hv_en(1);      //VCDT_HV_EN

	upmu_set_rg_usbdl_set(0);       //force leave USBDL mode
	upmu_set_rg_usbdl_rst(1);		//force leave USBDL mode

	upmu_set_rg_bc11_bb_ctrl(1);    //BC11_BB_CTRL
	upmu_set_rg_bc11_rst(1);        //BC11_RST

	upmu_set_rg_csdac_mode(1);      //CSDAC_MODE
	upmu_set_rg_vbat_ov_en(1);      //VBAT_OV_EN
	if (high_battery_volt_enable == 1)
		upmu_set_rg_vbat_ov_vth(0x2);   //VBAT_OV_VTH, 4.4V,
	else
		upmu_set_rg_vbat_ov_vth(0x1);   //VBAT_OV_VTH, 4.3V,

	upmu_set_rg_baton_en(1);        //BATON_EN

	//Tim, for TBAT
	//upmu_set_rg_buf_pwd_b(1);       //RG_BUF_PWD_B
	upmu_set_rg_baton_ht_en(0);     //BATON_HT_EN

	upmu_set_rg_ulc_det_en(1);      // RG_ULC_DET_EN=1
	upmu_set_rg_low_ich_db(1);      // RG_LOW_ICH_DB=000001'b

	return status;
}


 static kal_uint32 charging_dump_register_pmic(void *data)
 {
 	kal_uint32 status = STATUS_OK;

	kal_uint32 reg_val = 0;
	kal_uint32 reg_num = CHR_CON0;
	kal_uint32 i = 0;

	for(i=reg_num ; i<=CHR_CON29 ; i+=2) {
		reg_val = upmu_get_reg_value(i);
		battery_xlog_printk(BAT_LOG_FULL, "Chr Reg[0x%x]=0x%x \r\n", i, reg_val);
	}

	return status;
}

static kal_uint32 charging_enable_pmic(void *data)
{
 	kal_uint32 status = STATUS_OK;
	kal_uint32 enable = *(kal_uint32*)(data);

	if(KAL_TRUE == enable)
	{
		upmu_set_rg_csdac_dly(0x4);             // CSDAC_DLY
		upmu_set_rg_csdac_stp(0x1);             // CSDAC_STP
		upmu_set_rg_csdac_stp_inc(0x1);         // CSDAC_STP_INC
		upmu_set_rg_csdac_stp_dec(0x2);         // CSDAC_STP_DEC
		upmu_set_rg_cs_en(1);                   // CS_EN, check me

		upmu_set_rg_hwcv_en(1);

		upmu_set_rg_vbat_cv_en(1);              // CV_EN
		upmu_set_rg_csdac_en(1);                // CSDAC_EN

		upmu_set_rg_chr_en(1); 				// CHR_EN

		if(Enable_BATDRV_LOG == BAT_LOG_FULL)
			charging_dump_register_pmic(NULL);
	} else {
		upmu_set_rg_chrwdt_int_en(0);    // CHRWDT_INT_EN
		upmu_set_rg_chrwdt_en(0);        // CHRWDT_EN
		upmu_set_rg_chrwdt_flag_wr(0);   // CHRWDT_FLAG

		upmu_set_rg_csdac_en(0);         // CSDAC_EN
		upmu_set_rg_chr_en(0);           // CHR_EN
		upmu_set_rg_hwcv_en(0);          // RG_HWCV_EN
	}
	return status;
}


static kal_uint32 charging_set_cv_voltage_pmic(void *data)
{
 	kal_uint32 status = STATUS_OK;
	kal_uint16 register_value;
	
	register_value = charging_parameter_to_value(VBAT_CV_VTH_PMIC, GETARRAYNUM(VBAT_CV_VTH_PMIC) ,*(kal_uint32 *)(data));
	upmu_set_rg_vbat_cv_vth(register_value); 

	return status;
}


static kal_uint32 charging_get_current_pmic(void *data)
{
	kal_uint32 status = STATUS_OK;
	kal_uint32 array_size;
	kal_uint32 reg_value;
	
	array_size = GETARRAYNUM(CS_VTH_PMIC);
	reg_value=upmu_get_reg_value(0x8);	//RG_CS_VTH_PMIC
	*(kal_uint32 *)data = charging_value_to_parameter(CS_VTH_PMIC,array_size,reg_value);
	
	return status;
}

static kal_uint32 charging_set_current_pmic(void *data)
{
	kal_uint32 status = STATUS_OK;
	kal_uint32 set_chr_current;
	kal_uint32 array_size;
	kal_uint32 register_value;

	array_size = GETARRAYNUM(CS_VTH_PMIC);
	set_chr_current = bmt_find_closest_level(CS_VTH_PMIC, array_size, *(kal_uint32 *)data);
	register_value = charging_parameter_to_value(CS_VTH_PMIC, array_size ,set_chr_current);
	upmu_set_rg_cs_vth(register_value);

	return status;
}


static kal_uint32 charging_set_input_current_pmic(void *data)
{
	kal_uint32 status = STATUS_OK;
	return status;
}

static kal_uint32 charging_get_charging_status_pmic(void *data)
{
	kal_uint32 status = STATUS_OK;
	return status;
}

static kal_uint32 charging_reset_watch_dog_timer_pmic(void *data)
{
	kal_uint32 status = STATUS_OK;

	upmu_set_rg_chrwdt_td(0x0);           // CHRWDT_TD, 4s
	upmu_set_rg_chrwdt_wr(1);             // CHRWDT_WR
	upmu_set_rg_chrwdt_int_en(1);         // CHRWDT_INT_EN
	upmu_set_rg_chrwdt_en(1);             // CHRWDT_EN
	upmu_set_rg_chrwdt_flag_wr(1);        // CHRWDT_WR

	return status;
}


static kal_uint32 charging_set_hv_threshold_pmic(void *data)
{
	kal_uint32 status = STATUS_OK;

	kal_uint32 set_hv_voltage;
	kal_uint32 array_size;
	kal_uint16 register_value;
	kal_uint32 voltage = *(kal_uint32*)(data);

	array_size = GETARRAYNUM(VCDT_HV_VTH_PMIC);
	set_hv_voltage = bmt_find_closest_level(VCDT_HV_VTH_PMIC, array_size, voltage);
	register_value = charging_parameter_to_value(VCDT_HV_VTH_PMIC, array_size ,set_hv_voltage);
	upmu_set_rg_vcdt_hv_vth(register_value);

	return status;
}


static kal_uint32 charging_get_hv_status_pmic(void *data)
{
	kal_uint32 status = STATUS_OK;

	*(kal_bool*)(data) = upmu_get_rgs_vcdt_hv_det();

	return status;
}

kal_uint32 (*charging_func_pmic[CHARGING_CMD_NUMBER])(void *data)=
{
 	charging_hw_init_pmic
	,charging_dump_register_pmic
	,charging_enable_pmic
	,charging_set_cv_voltage_pmic
	,charging_get_current_pmic
	,charging_set_current_pmic
	,charging_set_input_current_pmic		// not support, empty function
	,charging_get_charging_status_pmic	// not support, empty function
	,charging_reset_watch_dog_timer_pmic
	,charging_set_hv_threshold_pmic
	,charging_get_hv_status_pmic
	,charging_get_battery_status
	,charging_get_charger_det_status
	,charging_get_charger_type
	,charging_get_is_pcm_timer_trigger
	,charging_set_platform_reset
	,charging_get_platfrom_boot_mode
	,charging_set_power_off
};


