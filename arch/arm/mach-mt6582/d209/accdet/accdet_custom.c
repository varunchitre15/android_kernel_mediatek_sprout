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

#include "accdet_custom_def.h"
#include <accdet_custom.h>

//key press customization: long press time
struct headset_key_custom headset_key_custom_setting = {
	2000
};

struct headset_key_custom* get_headset_key_custom_setting(void)
{
	return &headset_key_custom_setting;
}

#ifdef  ACCDET_MULTI_KEY_FEATURE
static struct headset_mode_settings cust_headset_settings = {
	0x900, 0x400, 1, 0x3f0, 0x800, 0x800, 0x20
};
#else
//headset mode register settings(for MT6575)
static struct headset_mode_settings cust_headset_settings = {
	0x900, 0x400, 1, 0x3f0, 0x3000, 0x3000, 0x20
};
#endif

struct headset_mode_settings* get_cust_headset_settings(void)
{
	return &cust_headset_settings;
}