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

#include <linux/kernel.h>
#include <linux/string.h>
#include <mach/sensors_ssb.h>

#define SSB_SENSOR_VERSION  1

/* Get sensor tuning data from mt_devs.c  */
extern struct sensor_tuning_data *sensors_tuning_data;

struct acc_hw_ssb * find_acc_data(const char *name)
{
    int acc_num,i;

    if ((sensors_tuning_data)&&(sensors_tuning_data->version == SSB_SENSOR_VERSION)) {
        acc_num = sensors_tuning_data->acc_num;
        if (acc_num > ACC_NUM) {
            pr_err("[%s] Get failed acc_num, Now just support %d acc numbers\n", __func__,ACC_NUM);
            return NULL;
        }
        for(i=0; i<acc_num; i++) {
            if (!strcmp(sensors_tuning_data->acc_data[i].name ,name))
                return &sensors_tuning_data->acc_data[i];
        }
        if (i >= acc_num) {
            pr_err("[%s] No match acc tuning data,i=%d\n",__func__,i);
            return NULL;
        }
    }
    else {
        pr_err("[%s]: get tuning data fail\n",__func__);
    }
    return NULL;
}

struct alsps_hw_ssb * find_alsps_data(const char *name)
{
    int alsps_num,i;
    if ((sensors_tuning_data)&&(sensors_tuning_data->version == SSB_SENSOR_VERSION)) {
        alsps_num = sensors_tuning_data->alsps_num;
        if (alsps_num > ALSPS_NUM) {
            pr_err("[%s] Get failed alsps_num, Now just support %d alsps numbers\n", __func__,ALSPS_NUM);
            return NULL;
        }
        for(i=0; i<alsps_num; i++) {
            if (!strcmp(sensors_tuning_data->alsps_data[i].name ,name))
                return &sensors_tuning_data->alsps_data[i];
        }
        if (i >= alsps_num) {
            pr_err("[%s] No match alsps tuning data,i=%d\n",__func__,i);
            return NULL;
        }
    }
    else {
        pr_err("[%s]: get tuning data fail\n",__func__);
    }
    return NULL;
}

struct mag_hw_ssb * find_mag_data(const char *name)
{
    int mag_num,i;
    if ((sensors_tuning_data)&&(sensors_tuning_data->version == SSB_SENSOR_VERSION)) {
        mag_num = sensors_tuning_data->mag_num;
        if (mag_num > MAG_NUM) {
            pr_err("[%s] Get failed mag_num, Now just support %d mag numbers\n", __func__,MAG_NUM);
            return NULL;
        }
        for(i=0; i<mag_num; i++) {
            if (!strcmp(sensors_tuning_data->mag_data[i].name ,name))
                return &sensors_tuning_data->mag_data[i];
        }
        if (i >= mag_num) {
            pr_err("[%s] No match mag tuning data,i=%d\n",__func__,i);
            return NULL;
        }
    }
    else {
        pr_err("[%s]: get tuning data fail\n",__func__);
    }
    return NULL;
}

struct gyro_hw_ssb * find_gyro_data(const char *name)
{
    int gyro_num,i;
    if ((sensors_tuning_data)&&(sensors_tuning_data->version == SSB_SENSOR_VERSION)) {
        gyro_num = sensors_tuning_data->gyro_num;
        if (gyro_num > GYRO_NUM) {
            pr_err("[%s] Get failed gyro_num, Now just support %d gyro numbers\n", __func__,GYRO_NUM);
            return NULL;
        }
        for(i=0; i<gyro_num; i++) {
            if (!strcmp(sensors_tuning_data->gyro_data[i].name ,name))
                return &sensors_tuning_data->gyro_data[i];
        }
        if (i >= gyro_num) {
            pr_err("[%s] No match gyro tuning data,i=%d\n",__func__,i);
            return NULL;
        }
    }
    else {
        pr_err("[%s]: get tuning data fail\n",__func__);
    }
    return NULL;
}


