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

// use accdet + EINT solution
#define ACCDET_EINT
// support multi_key feature
#define ACCDET_MULTI_KEY_FEATURE
// after 5s disable accdet
#define ACCDET_LOW_POWER

#define ACCDET_PIN_RECOGNIZATION
#define ACCDET_28V_MODE

#define ACCDET_SHORT_PLUGOUT_DEBOUNCE
#define ACCDET_SHORT_PLUGOUT_DEBOUNCE_CN 20

//extern struct headset_mode_settings* get_cust_headset_settings(void);
//extern int get_long_press_time_cust(void);