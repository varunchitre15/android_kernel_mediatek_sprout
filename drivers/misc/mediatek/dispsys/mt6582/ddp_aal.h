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

#ifndef __DDP_AAL_H__
#define __DDP_AAL_H__


#define PWM_DUTY_LEVEL      1024
#define PWM_DUTY_STEP       1
#define PWM_DUTY_MAX        1023

#define BLS_HIST_BIN        32

#define LUMA_HIST_BIN       17
#define LUMA_HIST_STEP      32
#define LUMA_HIST_MAX       511

#define LUMA_CURVE_POINT    17
#define LUMA_CURVE_STEP     32
#define LUMA_CURVE_MAX      511


enum
{
    ENUM_FUNC_NONE = 0,
    ENUM_FUNC_GAMMA = 0x1,
    ENUM_FUNC_AAL = 0x2,
    ENUM_FUNC_BLS = 0x4,
};

typedef struct {
    unsigned long histogram[LUMA_HIST_BIN];
    unsigned long BLSHist[BLS_HIST_BIN];
    unsigned long ChromHist;
} DISP_AAL_STATISTICS;

typedef struct {
    unsigned long lumaCurve[LUMA_CURVE_POINT];
    unsigned long pwmDuty;
    
    // for BLS
    unsigned long setting;
    unsigned long maxClrLimit;
    unsigned long maxClrDistThd;
    unsigned long preDistLimit;
    unsigned long preDistThd;
} DISP_AAL_PARAM;

//IOCTL , for AAL service to wait vsync and get latest histogram
int disp_wait_hist_update(unsigned long u4TimeOut_ms);

//IOCTL , for AAL service to enable vsync notification
void disp_set_aal_alarm(unsigned int u4En);

//Called by interrupt to check if aal need to be notified
void on_disp_aal_alarm_set(void);
unsigned int is_disp_aal_alarm_on(void);

//Called by interrupt to wake up aal
int disp_needWakeUp(void);

//Called by interrupt to wake up aal
void disp_wakeup_aal(void);

//IOCTL , for AAL service to config AAL
DISP_AAL_PARAM * get_aal_config(void);

//Called by tasklet to config registers
void disp_onConfig_aal(int i4FrameUpdate);

//Called by BLS backlight function
int disp_is_aal_config(void);

void disp_aal_reset(void);

#endif
