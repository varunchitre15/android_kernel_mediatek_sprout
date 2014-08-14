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

#include <mach/mt_typedefs.h>
#include <mtk_kpd.h>		/* custom file */

#if KPD_DRV_CTRL_BACKLIGHT
void kpd_enable_backlight(void)
{
	/*mt6326_kpled_dim_duty_Full();
	mt6326_kpled_Enable();*/
}

void kpd_disable_backlight(void)
{
	/*mt6326_kpled_dim_duty_0();
	mt6326_kpled_Disable();*/
}
#endif

/* for META tool */
void kpd_set_backlight(bool onoff, void *val1, void *val2)
{
	/*u8 div = *(u8 *)val1;
	u8 duty = *(u8 *)val2;

	if (div > 15)
		div = 15;
	pmic_kp_dim_div(div);

	if (duty > 31)
		duty = 31;
	pmic_kp_dim_duty(duty);

	if (onoff)
		mt6326_kpled_Enable();
	else
		mt6326_kpled_Disable();*/
}
