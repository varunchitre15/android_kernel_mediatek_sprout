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

#ifndef _MT_MSDC_SSB_CUST_H_
#define _MT_MSDC_SSB_CUST_H_

struct tag_msdc_hw_para {
	unsigned int   version;             /* msdc structure version info */
	unsigned char  clk_src;          /* host clock source */
	unsigned char  cmd_edge;         /* command latch edge */
	unsigned char  rdata_edge;        /* read data latch edge */
	unsigned char  wdata_edge;        /* write data latch edge */
	unsigned char  clk_drv;          /* clock pad driving */
	unsigned char  cmd_drv;          /* command pad driving */
	unsigned char  dat_drv;          /* data pad driving */
	unsigned char  clk_drv_sd_18;          /* clock pad driving for SD card at 1.8v sdr104 mode */
	unsigned char  cmd_drv_sd_18;          /* command pad driving for SD card at 1.8v sdr104 mode */
	unsigned char  dat_drv_sd_18;          /* data pad driving for SD card at 1.8v sdr104 mode */
	unsigned char  clk_drv_sd_18_sdr50;    /* clock pad driving for SD card at 1.8v sdr50 mode */
	unsigned char  cmd_drv_sd_18_sdr50;    /* command pad driving for SD card at 1.8v sdr50 mode */
	unsigned char  dat_drv_sd_18_sdr50;    /* data pad driving for SD card at 1.8v sdr50 mode */
	unsigned char  clk_drv_sd_18_ddr50;    /* clock pad driving for SD card at 1.8v ddr50 mode */
	unsigned char  cmd_drv_sd_18_ddr50;    /* command pad driving for SD card at 1.8v ddr50 mode */
	unsigned char  dat_drv_sd_18_ddr50;    /* data pad driving for SD card at 1.8v ddr50 mode */
	unsigned int   flags;            /* hardware capability flags */
	unsigned int   data_pins;        /* data pins */
	unsigned int   data_offset;      /* data address offset */

	unsigned char  ddlsel;    // data line delay line fine tune selecion
	unsigned char  rdsplsel;  // read: data line rising or falling latch fine tune selection
	unsigned char  wdsplsel;  // write: data line rising or falling latch fine tune selection

	unsigned char  dat0rddly; //read; range: 0~31
	unsigned char  dat1rddly; //read; range: 0~31
	unsigned char  dat2rddly; //read; range: 0~31
	unsigned char  dat3rddly; //read; range: 0~31
	unsigned char  dat4rddly; //read; range: 0~31
	unsigned char  dat5rddly; //read; range: 0~31
	unsigned char  dat6rddly; //read; range: 0~31
	unsigned char  dat7rddly; //read; range: 0~31
	unsigned char  datwrddly; //write; range: 0~31
	unsigned char  cmdrrddly; //cmd; range: 0~31
	unsigned char  cmdrddly;  //cmd; range: 0~31
	unsigned int   host_function; /* define host function*/
	unsigned char  boot;          /* define boot host*/
	unsigned char  cd_level;      /* card detection level*/
};

#endif
