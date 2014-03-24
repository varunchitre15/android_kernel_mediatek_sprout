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

#include <linux/i2c.h>

extern int synaptics_ts_read(struct i2c_client *client, u8 reg, int num, u8 *buf);
extern int synaptics_ts_read_f54(struct i2c_client *client, u8 reg, int num, u8 *buf);
extern int synaptics_ts_write(struct i2c_client *client, u8 reg, u8 * buf, int len);
extern struct i2c_client* ds4_i2c_client;

void device_I2C_read(unsigned char add, unsigned char *value, unsigned short len);
void device_I2C_write(unsigned char add, unsigned char *value, unsigned short len);
void InitPage(void);
void SetPage(unsigned char page);
void readRMI(unsigned short add, unsigned char *value, unsigned short len);
void longReadRMI(unsigned short add, unsigned char *value, unsigned short len);
void writeRMI(unsigned short add, unsigned char *value, unsigned short len);
void delayMS(int val);
void cleanExit(int code);
int waitATTN(int code, int time);
void write_log(char *data);