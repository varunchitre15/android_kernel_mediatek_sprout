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

#ifndef _SENSOR_TUNING_DATA_
#define _SENSOR_TUNING_DATA_
/* Support sensor numbers */
#define ACC_NUM   2
#define MAG_NUM   2
#define ALSPS_NUM 2
#define GYRO_NUM  3

struct acc_hw_ssb {
   unsigned char name[16];
   unsigned int i2c_addr;
   unsigned int i2c_num;
   unsigned int direction;
   unsigned int firlen;
};

struct mag_hw_ssb {
   unsigned char name[16];
   unsigned int i2c_addr;
   unsigned int i2c_num;
   unsigned int direction;
};

struct alsps_hw_ssb {
   unsigned char name[16];
   unsigned int  i2c_addr[4];
   unsigned int i2c_num;
   unsigned int ps_threshold_high;
   unsigned int ps_threshold_low;
   unsigned int als_level[15];
   unsigned int als_value[16];
};

struct gyro_hw_ssb {
   unsigned char name[16];
   unsigned int i2c_addr;
   unsigned int i2c_num;
   unsigned int direction;
   unsigned int firlen;
};

struct sensor_tuning_data{
    unsigned int version;  /* First version is '1'*/
    unsigned int acc_num;
    unsigned int mag_num;
    unsigned int alsps_num;
    unsigned int gyro_num;
    struct acc_hw_ssb acc_data[ACC_NUM];
    struct mag_hw_ssb mag_data[MAG_NUM];
    struct alsps_hw_ssb  alsps_data[ALSPS_NUM];
    struct gyro_hw_ssb gyro_data[GYRO_NUM];
  };

struct acc_hw_ssb * find_acc_data(const char *name);
struct alsps_hw_ssb * find_alsps_data(const char *name);
struct mag_hw_ssb * find_mag_data(const char *name);
struct gyro_hw_ssb * find_gyro_data(const char *name);
#endif
