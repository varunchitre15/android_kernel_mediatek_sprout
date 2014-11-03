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

#ifndef __CUST_GPIO_USAGE_H__
#define __CUST_GPIO_USAGE_H__

#include <mach/gpio_usage_para.h>

extern struct _gpio_usage *gpio_usage;

#define GPIO_EINT_CHG_STAT_PIN			(gpio_usage->eint_chg_stat_pin | 0x80000000)
#define GPIO_EINT_CHG_STAT_PIN_M_GPIO			gpio_usage->eint_chg_stat_pin_m_gpio
#define GPIO_EINT_CHG_STAT_PIN_M_PWM			gpio_usage->eint_chg_stat_pin_m_pwm
#define GPIO_EINT_CHG_STAT_PIN_M_EINT			gpio_usage->eint_chg_stat_pin_m_eint
#define GPIO_ALS_EINT_PIN            (gpio_usage->als_eint_pin | 0x80000000)
#define GPIO_ALS_EINT_PIN_M_GPIO            gpio_usage->als_eint_pin_m_gpio
#define GPIO_ALS_EINT_PIN_M_MDEINT            gpio_usage->als_eint_pin_m_mdeint
#define GPIO_ALS_EINT_PIN_M_PWM            gpio_usage->als_eint_pin_m_pwm
#define GPIO_ALS_EINT_PIN_M_EINT            gpio_usage->als_eint_pin_m_eint
#define GPIO_CTP_EINT_PIN            (gpio_usage->ctp_eint_pin | 0x80000000)
#define GPIO_CTP_EINT_PIN_M_GPIO            gpio_usage->ctp_eint_pin_m_gpio
#define GPIO_CTP_EINT_PIN_M_CLK            gpio_usage->ctp_eint_pin_m_clk
#define GPIO_CTP_EINT_PIN_M_MDEINT            gpio_usage->ctp_eint_pin_m_mdeint
#define GPIO_CTP_EINT_PIN_M_KCOL            gpio_usage->ctp_eint_pin_m_kcol
#define GPIO_CTP_EINT_PIN_M_EINT            gpio_usage->ctp_eint_pin_m_eint
#define GPIO_CTP_EINT_PIN_CLK            gpio_usage->ctp_eint_pin_clk
#define GPIO_CTP_EINT_PIN_FREQ            gpio_usage->ctp_eint_pin_freq
#define GPIO_GSE_2_EINT_PIN            (gpio_usage->gse_2_eint_pin | 0x80000000)
#define GPIO_GSE_2_EINT_PIN_M_GPIO            gpio_usage->gse_2_eint_pin_m_gpio
#define GPIO_GSE_2_EINT_PIN_M_CLK            gpio_usage->gse_2_eint_pin_m_clk
#define GPIO_GSE_2_EINT_PIN_M_MDEINT            gpio_usage->gse_2_eint_pin_m_mdeint
#define GPIO_GSE_2_EINT_PIN_M_KCOL            gpio_usage->gse_2_eint_pin_m_kcol
#define GPIO_GSE_2_EINT_PIN_M_EINT            gpio_usage->gse_2_eint_pin_m_eint
#define GPIO_GSE_2_EINT_PIN_CLK            gpio_usage->gse_2_eint_pin_clk
#define GPIO_GSE_2_EINT_PIN_FREQ            gpio_usage->gse_2_eint_pin_freq
#define GPIO_ACCDET_EINT_PIN            (gpio_usage->accdet_eint_pin | 0x80000000)
#define GPIO_ACCDET_EINT_PIN_M_GPIO            gpio_usage->accdet_eint_pin_m_gpio
#define GPIO_ACCDET_EINT_PIN_M_CLK            gpio_usage->accdet_eint_pin_m_clk
#define GPIO_ACCDET_EINT_PIN_M_EINT            gpio_usage->accdet_eint_pin_m_eint
#define GPIO_ACCDET_EINT_PIN_CLK            gpio_usage->accdet_eint_pin_clk
#define GPIO_ACCDET_EINT_PIN_FREQ            gpio_usage->accdet_eint_pin_freq
#define GPIO_GYRO_EINT_PIN            (gpio_usage->gyro_eint_pin | 0x80000000)
#define GPIO_GYRO_EINT_PIN_M_GPIO            gpio_usage->gyro_eint_pin_m_gpio
#define GPIO_GYRO_EINT_PIN_M_KCOL            gpio_usage->gyro_eint_pin_m_kcol
#define GPIO_GYRO_EINT_PIN_M_EINT            gpio_usage->gyro_eint_pin_m_eint
#define GPIO_SWCHARGER_EN_PIN			(gpio_usage->swcharger_en_pin | 0x80000000)
#define GPIO_SWCHARGER_EN_PIN_M_GPIO			gpio_usage->swcharger_en_pin_m_gpio
#define GPIO_CAMERA_CMRST_PIN            (gpio_usage->camera_cmrst_pin | 0x80000000)
#define GPIO_CAMERA_CMRST_PIN_M_GPIO            gpio_usage->camera_cmrst_pin_m_gpio
#define GPIO_CAMERA_CMRST_PIN_M_CLK            gpio_usage->camera_cmrst_pin_m_clk
#define GPIO_CAMERA_CMRST_PIN_M_MDEINT            gpio_usage->camera_cmrst_pin_m_mdeint
#define GPIO_CAMERA_CMRST_PIN_M_SDA            gpio_usage->camera_cmrst_pin_m_sda
#define GPIO_CAMERA_CMRST_PIN_M_EXT_FRAME_SYNC            gpio_usage->camera_cmrst_pin_m_ext_frame_sync
#define GPIO_CAMERA_CMRST_PIN_M_NWEB            gpio_usage->camera_cmrst_pin_m_nweb
#define GPIO_CAMERA_CMRST_PIN_CLK            gpio_usage->camera_cmrst_pin_clk
#define GPIO_CAMERA_CMRST_PIN_FREQ            gpio_usage->camera_cmrst_pin_freq
#define GPIO_CAMERA_CMPDN_PIN            (gpio_usage->camera_cmpdn_pin | 0x80000000)
#define GPIO_CAMERA_CMPDN_PIN_M_GPIO            gpio_usage->camera_cmpdn_pin_m_gpio
#define GPIO_CAMERA_CMPDN_PIN_M_CLK            gpio_usage->camera_cmpdn_pin_m_clk
#define GPIO_CAMERA_CMPDN_PIN_M_MDEINT            gpio_usage->camera_cmpdn_pin_m_mdeint
#define GPIO_CAMERA_CMPDN_PIN_M_EINT            gpio_usage->camera_cmpdn_pin_m_eint
#define GPIO_CAMERA_CMPDN_PIN_CLK            gpio_usage->camera_cmpdn_pin_clk
#define GPIO_CAMERA_CMPDN_PIN_FREQ            gpio_usage->camera_cmpdn_pin_freq
#define GPIO_CAMERA_CMPDN_PIN_NCE            gpio_usage->camera_cmpdn_pin_nce
#define GPIO_GSE_1_EINT_PIN            (gpio_usage->gse_1_eint_pin | 0x80000000)
#define GPIO_GSE_1_EINT_PIN_M_GPIO            gpio_usage->gse_1_eint_pin_m_gpio
#define GPIO_GSE_1_EINT_PIN_M_CLK            gpio_usage->gse_1_eint_pin_m_clk
#define GPIO_GSE_1_EINT_PIN_M_KROW            gpio_usage->gse_1_eint_pin_m_krow
#define GPIO_GSE_1_EINT_PIN_M_PWM            gpio_usage->gse_1_eint_pin_m_pwm
#define GPIO_GSE_1_EINT_PIN_M_EINT            gpio_usage->gse_1_eint_pin_m_eint
#define GPIO_GSE_1_EINT_PIN_CLK            gpio_usage->gse_1_eint_pin_clk
#define GPIO_GSE_1_EINT_PIN_FREQ            gpio_usage->gse_1_eint_pin_freq
#define GPIO_CAMERA_FLASH_EN_PIN            (gpio_usage->camera_flash_en_pin | 0x80000000)
#define GPIO_CAMERA_FLASH_EN_PIN_M_GPIO            gpio_usage->camera_flash_en_pin_m_gpio
#define GPIO_CAMERA_FLASH_EN_PIN_M_CLK            gpio_usage->camera_flash_en_pin_m_clk
#define GPIO_CAMERA_FLASH_EN_PIN_M_KCOL            gpio_usage->camera_flash_en_pin_m_kcol
#define GPIO_CAMERA_FLASH_EN_PIN_M_PWM            gpio_usage->camera_flash_en_pin_m_pwm
#define GPIO_CAMERA_FLASH_EN_PIN_CLK            gpio_usage->camera_flash_en_pin_clk
#define GPIO_CAMERA_FLASH_EN_PIN_FREQ            gpio_usage->camera_flash_en_pin_freq
#define GPIO_CAMERA_FLASH_MODE_PIN            (gpio_usage->camera_flash_mode_pin | 0x80000000)
#define GPIO_CAMERA_FLASH_MODE_PIN_M_GPIO            gpio_usage->camera_flash_mode_pin_m_gpio
#define GPIO_CAMERA_FLASH_MODE_PIN_M_MDEINT            gpio_usage->camera_flash_mode_pin_m_mdeint
#define GPIO_CTP_RST_PIN            (gpio_usage->ctp_rst_pin | 0x80000000)
#define GPIO_CTP_RST_PIN_M_GPIO            gpio_usage->ctp_rst_pin_m_gpio
#define GPIO_CTP_RST_PIN_M_MDEINT            gpio_usage->ctp_rst_pin_m_mdeint
#define GPIO_CTP_RST_PIN_M_DAC_DAT_OUT            gpio_usage->ctp_rst_pin_m_dac_dat_out
#define GPIO_CTP_RST_PIN_M_ANT_SEL            gpio_usage->ctp_rst_pin_m_ant_sel
#define GPIO_CTP_RST_PIN_M_CONN_MCU_DBGACK_N            gpio_usage->ctp_rst_pin_m_conn_mcu_dbgack_n
#define GPIO_CTP_RST_PIN_M_DBG_MON_B            gpio_usage->ctp_rst_pin_m_dbg_mon_b
#define GPIO_CAMERA_CMRST1_PIN            (gpio_usage->camera_cmrst1_pin | 0x80000000)
#define GPIO_CAMERA_CMRST1_PIN_M_GPIO            gpio_usage->camera_cmrst1_pin_m_gpio
#define GPIO_MAIN_CAM_ID_PIN			(gpio_usage->main_cam_id_pin | 0x80000000)
#define GPIO_MAIN_CAM_ID_PIN_M_GPIO			gpio_usage->main_cam_id_pin_m_gpio
#define GPIO_MAIN_CAM_ID_PIN_M_CLK			gpio_usage->main_cam_id_pin_m_clk
#define GPIO_MAIN_CAM_ID_PIN_CLK			gpio_usage->main_cam_id_pin_clk
#define GPIO_MAIN_CAM_ID_PIN_FREQ			gpio_usage->main_cam_id_pin_freq
#define GPIO_UART_UCTS1_PIN			(gpio_usage->uart_ucts1_pin | 0x80000000)
#define GPIO_UART_UCTS1_PIN_M_GPIO			gpio_usage->uart_ucts1_pin_m_gpio
#define GPIO_UART_UCTS1_PIN_M_CLK			gpio_usage->uart_ucts1_pin_m_clk
#define GPIO_UART_UCTS1_PIN_CLK			gpio_usage->uart_ucts1_pin_clk
#define GPIO_UART_UCTS1_PIN_FREQ			gpio_usage->uart_ucts1_pin_freq
#define GPIO_CAMERA_CMPDN1_PIN			(gpio_usage->camera_cmpdn1_pin | 0x80000000)
#define GPIO_CAMERA_CMPDN1_PIN_M_GPIO			gpio_usage->camera_cmpdn1_pin_m_gpio
#define GPIO_UART_URTS1_PIN			(gpio_usage->uart_urts1_pin | 0x80000000)
#define GPIO_UART_URTS1_PIN_M_GPIO			gpio_usage->uart_urts1_pin_m_gpio
#define GPIO_SUB_CAM_ID_PIN			(gpio_usage->sub_cam_id_pin | 0x80000000)
#define GPIO_SUB_CAM_ID_PIN_M_GPIO			gpio_usage->sub_cam_id_pin_m_gpio
#define GPIO_SUB_CAM_ID_PIN_M_CLK			gpio_usage->sub_cam_id_pin_m_clk
#define GPIO_SUB_CAM_ID_PIN_CLK			gpio_usage->sub_cam_id_pin_clk
#define GPIO_SUB_CAM_ID_PIN_FREQ			gpio_usage->sub_cam_id_pin_freq
#define GPIO_UART_UCTS2_PIN			(gpio_usage->uart_ucts2_pin | 0x80000000)
#define GPIO_UART_UCTS2_PIN_M_GPIO			gpio_usage->uart_ucts2_pin_m_gpio
#define GPIO_UART_UCTS2_PIN_M_CLK			gpio_usage->uart_ucts2_pin_m_clk
#define GPIO_UART_UCTS2_PIN_CLK			gpio_usage->uart_ucts2_pin_clk
#define GPIO_UART_UCTS2_PIN_FREQ			gpio_usage->uart_ucts2_pin_freq
#define GPIO_UART_URTS2_PIN			(gpio_usage->uart_urts2_pin | 0x80000000)
#define GPIO_UART_URTS2_PIN_M_GPIO			gpio_usage->uart_urts2_pin_m_gpio
#define GPIO_UART_URTS2_PIN_M_CLK			gpio_usage->uart_urts2_pin_m_clk
#define GPIO_UART_URTS2_PIN_CLK			gpio_usage->uart_urts2_pin_clk
#define GPIO_UART_URTS2_PIN_FREQ			gpio_usage->uart_urts2_pin_freq
#define GPIO_PMIC_EINT_PIN			(gpio_usage->pmic_eint_pin | 0x80000000)
#define GPIO_PMIC_EINT_PIN_M_GPIO			gpio_usage->pmic_eint_pin_m_gpio
#define GPIO_PMIC_EINT_PIN_M_EINT			gpio_usage->pmic_eint_pin_m_eint
#define GPIO_SIM1_SCLK			(gpio_usage->sim1_sclk | 0x80000000)
#define GPIO_SIM1_SCLK_M_GPIO			gpio_usage->sim1_sclk_m_gpio
#define GPIO_SIM1_SCLK_M_CLK			gpio_usage->sim1_sclk_m_clk
#define GPIO_SIM1_SCLK_M_MD1_SIM2_SCLK			gpio_usage->sim1_sclk_m_md1_sim2_sclk
#define GPIO_SIM1_SRST			(gpio_usage->sim1_srst | 0x80000000)
#define GPIO_SIM1_SRST_M_GPIO			gpio_usage->sim1_srst_m_gpio
#define GPIO_SIM1_SRST_M_PWM			gpio_usage->sim1_srst_m_pwm
#define GPIO_SIM1_SRST_M_MD1_SIM1_SRST			gpio_usage->sim1_srst_m_md1_sim1_srst
#define GPIO_SIM1_SRST_M_MD1_SIM2_SRST			gpio_usage->sim1_srst_m_md1_sim2_srst
#define GPIO_SIM1_SIO			(gpio_usage->sim1_sio | 0x80000000)
#define GPIO_SIM1_SIO_M_GPIO			gpio_usage->sim1_sio_m_gpio
#define GPIO_SIM1_SIO_M_MD1_SIM1_SDAT			gpio_usage->sim1_sio_m_md1_sim1_sdat
#define GPIO_SIM1_SIO_M_MD1_SIM2_SDAT			gpio_usage->sim1_sio_m_md1_sim2_sdat
#define GPIO_SIM2_SCLK			(gpio_usage->sim2_sclk | 0x80000000)
#define GPIO_SIM2_SCLK_M_GPIO			gpio_usage->sim2_sclk_m_gpio
#define GPIO_SIM2_SCLK_M_CLK			gpio_usage->sim2_sclk_m_clk
#define GPIO_SIM2_SCLK_M_MD1_SIM1_SCLK			gpio_usage->sim2_sclk_m_md1_sim1_sclk
#define GPIO_SIM2_SRST			(gpio_usage->sim2_srst | 0x80000000)
#define GPIO_SIM2_SRST_M_GPIO			gpio_usage->sim2_srst_m_gpio
#define GPIO_SIM2_SRST_M_PWM			gpio_usage->sim2_srst_m_pwm
#define GPIO_SIM2_SRST_M_MD1_SIM2_SRST			gpio_usage->sim2_srst_m_md1_sim2_srst
#define GPIO_SIM2_SRST_M_MD1_SIM1_SRST			gpio_usage->sim2_srst_m_md1_sim1_srst
#define GPIO_SIM2_SIO			(gpio_usage->sim2_sio | 0x80000000)
#define GPIO_SIM2_SIO_M_GPIO			gpio_usage->sim2_sio_m_gpio
#define GPIO_SIM2_SIO_M_MD1_SIM2_SDAT			gpio_usage->sim2_sio_m_md1_sim2_sdat
#define GPIO_SIM2_SIO_M_MD1_SIM1_SDAT			gpio_usage->sim2_sio_m_md1_sim1_sdat
#define GPIO_SIM2_SIO_M_DBG_MON_B			gpio_usage->sim2_sio_m_dbg_mon_b
#define GPIO_UART_URXD3_PIN			(gpio_usage->uart_urxd3_pin | 0x80000000)
#define GPIO_UART_URXD3_PIN_M_GPIO			gpio_usage->uart_urxd3_pin_m_gpio
#define GPIO_UART_URXD3_PIN_M_CLK			gpio_usage->uart_urxd3_pin_m_clk
#define GPIO_UART_URXD3_PIN_M_KROW			gpio_usage->uart_urxd3_pin_m_krow
#define GPIO_UART_URXD3_PIN_M_URXD			gpio_usage->uart_urxd3_pin_m_urxd
#define GPIO_UART_URXD3_PIN_M_DPI_HSYNC			gpio_usage->uart_urxd3_pin_m_dpi_hsync
#define GPIO_UART_URXD3_PIN_M_UTXD			gpio_usage->uart_urxd3_pin_m_utxd
#define GPIO_UART_URXD3_PIN_M_MD_URXD			gpio_usage->uart_urxd3_pin_m_md_urxd
#define GPIO_UART_UTXD3_PIN			(gpio_usage->uart_utxd3_pin | 0x80000000)
#define GPIO_UART_UTXD3_PIN_M_GPIO			gpio_usage->uart_utxd3_pin_m_gpio
#define GPIO_UART_UTXD3_PIN_M_KROW			gpio_usage->uart_utxd3_pin_m_krow
#define GPIO_UART_UTXD3_PIN_M_UTXD			gpio_usage->uart_utxd3_pin_m_utxd
#define GPIO_UART_UTXD3_PIN_M_DPI_VSYNC			gpio_usage->uart_utxd3_pin_m_dpi_vsync
#define GPIO_UART_UTXD3_PIN_M_URXD			gpio_usage->uart_utxd3_pin_m_urxd
#define GPIO_UART_UTXD3_PIN_M_MD_UTXD			gpio_usage->uart_utxd3_pin_m_md_utxd
#define GPIO_UART_UTXD3_PIN_M_TDD_TXD			gpio_usage->uart_utxd3_pin_m_tdd_txd
#define GPIO_UART_URXD4_PIN			(gpio_usage->uart_urxd4_pin | 0x80000000)
#define GPIO_UART_URXD4_PIN_M_GPIO			gpio_usage->uart_urxd4_pin_m_gpio
#define GPIO_UART_URXD4_PIN_M_KROW			gpio_usage->uart_urxd4_pin_m_krow
#define GPIO_UART_URXD4_PIN_M_PWM			gpio_usage->uart_urxd4_pin_m_pwm
#define GPIO_UART_URXD4_PIN_M_URXD			gpio_usage->uart_urxd4_pin_m_urxd
#define GPIO_UART_URXD4_PIN_M_DPI_CK			gpio_usage->uart_urxd4_pin_m_dpi_ck
#define GPIO_UART_URXD4_PIN_M_UTXD			gpio_usage->uart_urxd4_pin_m_utxd
#define GPIO_UART_URXD4_PIN_M_UCTS			gpio_usage->uart_urxd4_pin_m_ucts
#define GPIO_UART_UTXD4_PIN			(gpio_usage->uart_utxd4_pin | 0x80000000)
#define GPIO_UART_UTXD4_PIN_M_GPIO			gpio_usage->uart_utxd4_pin_m_gpio
#define GPIO_UART_UTXD4_PIN_M_KROW			gpio_usage->uart_utxd4_pin_m_krow
#define GPIO_UART_UTXD4_PIN_M_PWM			gpio_usage->uart_utxd4_pin_m_pwm
#define GPIO_UART_UTXD4_PIN_M_UTXD			gpio_usage->uart_utxd4_pin_m_utxd
#define GPIO_UART_UTXD4_PIN_M_DPI_DE			gpio_usage->uart_utxd4_pin_m_dpi_de
#define GPIO_UART_UTXD4_PIN_M_URXD			gpio_usage->uart_utxd4_pin_m_urxd
#define GPIO_UART_UTXD4_PIN_M_URTS			gpio_usage->uart_utxd4_pin_m_urts
#define GPIO_FDD_BAND_SUPPORT_DETECT_1ST_PIN			(gpio_usage->fdd_band_support_detect_1st_pin | 0x80000000)
#define GPIO_FDD_BAND_SUPPORT_DETECT_1ST_PIN_M_GPIO			gpio_usage->fdd_band_support_detect_1st_pin_m_gpio
#define GPIO_FDD_BAND_SUPPORT_DETECT_1ST_PIN_M_CLK			gpio_usage->fdd_band_support_detect_1st_pin_m_clk
#define GPIO_FDD_BAND_SUPPORT_DETECT_2ND_PIN			(gpio_usage->fdd_band_support_detect_2nd_pin | 0x80000000)
#define GPIO_FDD_BAND_SUPPORT_DETECT_2ND_PIN_M_GPIO			gpio_usage->fdd_band_support_detect_2nd_pin_m_gpio
#define GPIO_FDD_BAND_SUPPORT_DETECT_3RD_PIN			(gpio_usage->fdd_band_support_detect_3rd_pin | 0x80000000)
#define GPIO_FDD_BAND_SUPPORT_DETECT_3RD_PIN_M_GPIO			gpio_usage->fdd_band_support_detect_3rd_pin_m_gpio
#define GPIO_FDD_BAND_SUPPORT_DETECT_4TH_PIN			(gpio_usage->fdd_band_support_detect_4th_pin | 0x80000000)
#define GPIO_FDD_BAND_SUPPORT_DETECT_4TH_PIN_M_GPIO			gpio_usage->fdd_band_support_detect_4th_pin_m_gpio
#define GPIO_GPS_LNA_PIN			(gpio_usage->gps_lna_pin | 0x80000000)
#define GPIO_GPS_LNA_PIN_M_GPIO			gpio_usage->gps_lna_pin_m_gpio
#define GPIO_GPS_LNA_PIN_M_PWM			gpio_usage->gps_lna_pin_m_pwm
#define GPIO_GPS_LNA_PIN_M_ANT_SEL			gpio_usage->gps_lna_pin_m_ant_sel
#define GPIO_GPS_LNA_PIN_M_CONN_MCU_DBGACK_N			gpio_usage->gps_lna_pin_m_conn_mcu_dbgack_n
#define GPIO_GPS_LNA_PIN_M_DBG_MON_A			gpio_usage->gps_lna_pin_m_dbg_mon_a
#define GPIO_KPD_KROW0_PIN			(gpio_usage->kpd_krow0_pin | 0x80000000)
#define GPIO_KPD_KROW0_PIN_M_GPIO			gpio_usage->kpd_krow0_pin_m_gpio
#define GPIO_KPD_KROW0_PIN_M_KROW			gpio_usage->kpd_krow0_pin_m_krow
#define GPIO_KPD_KCOL0_PIN			(gpio_usage->kpd_kcol0_pin | 0x80000000)
#define GPIO_KPD_KCOL0_PIN_M_GPIO			gpio_usage->kpd_kcol0_pin_m_gpio
#define GPIO_KPD_KCOL0_PIN_M_KCOL			gpio_usage->kpd_kcol0_pin_m_kcol
#define GPIO_SPEAKER_EN_PIN			(gpio_usage->speaker_en_pin | 0x80000000)
#define GPIO_SPEAKER_EN_PIN_M_GPIO			gpio_usage->speaker_en_pin_m_gpio
#define GPIO_I2C0_SDA_PIN            (gpio_usage->i2c0_sda_pin | 0x80000000)
#define GPIO_I2C0_SDA_PIN_M_GPIO            gpio_usage->i2c0_sda_pin_m_gpio
#define GPIO_I2C0_SDA_PIN_M_SDA            gpio_usage->i2c0_sda_pin_m_sda
#define GPIO_I2C0_SCA_PIN            (gpio_usage->i2c0_sca_pin | 0x80000000)
#define GPIO_I2C0_SCA_PIN_M_GPIO            gpio_usage->i2c0_sca_pin_m_gpio
#define GPIO_I2C0_SCA_PIN_M_SCL            gpio_usage->i2c0_sca_pin_m_scl
#define GPIO_I2C1_SDA_PIN            (gpio_usage->i2c1_sda_pin | 0x80000000)
#define GPIO_I2C1_SDA_PIN_M_GPIO            gpio_usage->i2c1_sda_pin_m_gpio
#define GPIO_I2C1_SDA_PIN_M_SDA            gpio_usage->i2c1_sda_pin_m_sda
#define GPIO_I2C1_SCA_PIN            (gpio_usage->i2c1_sca_pin | 0x80000000)
#define GPIO_I2C1_SCA_PIN_M_GPIO            gpio_usage->i2c1_sca_pin_m_gpio
#define GPIO_I2C1_SCA_PIN_M_SCL            gpio_usage->i2c1_sca_pin_m_scl
#define GPIO_I2C2_SDA_PIN            (gpio_usage->i2c2_sda_pin | 0x80000000)
#define GPIO_I2C2_SDA_PIN_M_GPIO            gpio_usage->i2c2_sda_pin_m_gpio
#define GPIO_I2C2_SDA_PIN_M_PWM            gpio_usage->i2c2_sda_pin_m_pwm
#define GPIO_I2C2_SDA_PIN_M_SDA            gpio_usage->i2c2_sda_pin_m_sda
#define GPIO_I2C2_SCA_PIN            (gpio_usage->i2c2_sca_pin | 0x80000000)
#define GPIO_I2C2_SCA_PIN_M_GPIO            gpio_usage->i2c2_sca_pin_m_gpio
#define GPIO_I2C2_SCA_PIN_M_PWM            gpio_usage->i2c2_sca_pin_m_pwm
#define GPIO_I2C2_SCA_PIN_M_SCL            gpio_usage->i2c2_sca_pin_m_scl
#define GPIO_UART_URXD1_PIN            (gpio_usage->uart_urxd1_pin | 0x80000000)
#define GPIO_UART_URXD1_PIN_M_GPIO            gpio_usage->uart_urxd1_pin_m_gpio
#define GPIO_UART_URXD1_PIN_M_URXD            gpio_usage->uart_urxd1_pin_m_urxd
#define GPIO_UART_URXD1_PIN_M_UTXD            gpio_usage->uart_urxd1_pin_m_utxd
#define GPIO_UART_URXD1_PIN_M_MD_URXD            gpio_usage->uart_urxd1_pin_m_md_urxd
#define GPIO_UART_URXD1_PIN_M_DBG_MON_B            gpio_usage->uart_urxd1_pin_m_dbg_mon_b
#define GPIO_UART_UTXD1_PIN            (gpio_usage->uart_utxd1_pin | 0x80000000)
#define GPIO_UART_UTXD1_PIN_M_GPIO            gpio_usage->uart_utxd1_pin_m_gpio
#define GPIO_UART_UTXD1_PIN_M_UTXD            gpio_usage->uart_utxd1_pin_m_utxd
#define GPIO_UART_UTXD1_PIN_M_URXD            gpio_usage->uart_utxd1_pin_m_urxd
#define GPIO_UART_UTXD1_PIN_M_MD_UTXD            gpio_usage->uart_utxd1_pin_m_md_utxd
#define GPIO_UART_UTXD1_PIN_M_TDD_TXD            gpio_usage->uart_utxd1_pin_m_tdd_txd
#define GPIO_UART_UTXD1_PIN_M_DBG_MON_B            gpio_usage->uart_utxd1_pin_m_dbg_mon_b
#define GPIO_UART_URXD2_PIN            (gpio_usage->uart_urxd2_pin | 0x80000000)
#define GPIO_UART_URXD2_PIN_M_GPIO            gpio_usage->uart_urxd2_pin_m_gpio
#define GPIO_UART_URXD2_PIN_M_URXD            gpio_usage->uart_urxd2_pin_m_urxd
#define GPIO_UART_URXD2_PIN_M_UTXD            gpio_usage->uart_urxd2_pin_m_utxd
#define GPIO_UART_URXD2_PIN_M_MD_URXD            gpio_usage->uart_urxd2_pin_m_md_urxd
#define GPIO_UART_URXD2_PIN_M_DBG_MON_B            gpio_usage->uart_urxd2_pin_m_dbg_mon_b
#define GPIO_UART_UTXD2_PIN            (gpio_usage->uart_utxd2_pin | 0x80000000)
#define GPIO_UART_UTXD2_PIN_M_GPIO            gpio_usage->uart_utxd2_pin_m_gpio
#define GPIO_UART_UTXD2_PIN_M_UTXD            gpio_usage->uart_utxd2_pin_m_utxd
#define GPIO_UART_UTXD2_PIN_M_URXD            gpio_usage->uart_utxd2_pin_m_urxd
#define GPIO_UART_UTXD2_PIN_M_MD_UTXD            gpio_usage->uart_utxd2_pin_m_md_utxd
#define GPIO_UART_UTXD2_PIN_M_TDD_TXD            gpio_usage->uart_utxd2_pin_m_tdd_txd
#define GPIO_UART_UTXD2_PIN_M_DBG_MON_B            gpio_usage->uart_utxd2_pin_m_dbg_mon_b
#define GPIO_MSDC1_CMD            (gpio_usage->msdc1_cmd | 0x80000000)
#define GPIO_MSDC1_CMD_M_GPIO            gpio_usage->msdc1_cmd_m_gpio
#define GPIO_MSDC1_CMD_M_MSDC1_CMD            gpio_usage->msdc1_cmd_m_msdc1_cmd
#define GPIO_MSDC1_CLK            (gpio_usage->msdc1_clk | 0x80000000)
#define GPIO_MSDC1_CLK_M_GPIO            gpio_usage->msdc1_clk_m_gpio
#define GPIO_MSDC1_CLK_M_CLK            gpio_usage->msdc1_clk_m_clk
#define GPIO_MSDC1_DAT0            (gpio_usage->msdc1_dat0 | 0x80000000)
#define GPIO_MSDC1_DAT0_M_GPIO            gpio_usage->msdc1_dat0_m_gpio
#define GPIO_MSDC1_DAT0_M_MSDC1_DAT            gpio_usage->msdc1_dat0_m_msdc1_dat
#define GPIO_MSDC1_DAT1            (gpio_usage->msdc1_dat1 | 0x80000000)
#define GPIO_MSDC1_DAT1_M_GPIO            gpio_usage->msdc1_dat1_m_gpio
#define GPIO_MSDC1_DAT1_M_MSDC1_DAT            gpio_usage->msdc1_dat1_m_msdc1_dat
#define GPIO_MSDC1_DAT2            (gpio_usage->msdc1_dat2 | 0x80000000)
#define GPIO_MSDC1_DAT2_M_GPIO            gpio_usage->msdc1_dat2_m_gpio
#define GPIO_MSDC1_DAT2_M_MSDC1_DAT            gpio_usage->msdc1_dat2_m_msdc1_dat
#define GPIO_MSDC1_DAT3            (gpio_usage->msdc1_dat3 | 0x80000000)
#define GPIO_MSDC1_DAT3_M_GPIO            gpio_usage->msdc1_dat3_m_gpio
#define GPIO_MSDC1_DAT3_M_MSDC1_DAT            gpio_usage->msdc1_dat3_m_msdc1_dat


/*Output for default variable names*/
/*@XXX_XX_PIN in gpio.cmp          */



#endif /* __CUST_GPIO_USAGE_H__ */


