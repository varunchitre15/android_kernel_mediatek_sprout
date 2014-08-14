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

#ifndef _CUST_LEDS_DEF_H
#define _CUST_LEDS_DEF_H

//#define CUST_LEDS_BACKLIGHT_PMIC_PARA /* parallel */
//#define CUST_LEDS_BACKLIGHT_PMIC_SERI /* series */
enum mt65xx_led_mode
{
	MT65XX_LED_MODE_NONE,
	MT65XX_LED_MODE_PWM,
	MT65XX_LED_MODE_GPIO,
	MT65XX_LED_MODE_PMIC,
	//MT65XX_LED_MODE_CUST,
	MT65XX_LED_MODE_CUST_LCM,	
	MT65XX_LED_MODE_CUST_BLS_PWM
};

enum mt65xx_led_pmic
{
	MT65XX_LED_PMIC_LCD_ISINK=0,
	MT65XX_LED_PMIC_NLED_ISINK0,
	MT65XX_LED_PMIC_NLED_ISINK1,
	MT65XX_LED_PMIC_NLED_ISINK2,
	MT65XX_LED_PMIC_NLED_ISINK3
};
struct PWM_config
{
	int clock_source;
	int div;
	int low_duration;
	int High_duration;
	BOOL pmic_pad;
};
typedef int (*cust_brightness_set)(int level, int div);
typedef int (*cust_set_brightness)(int level);

/*
 * name : must the same as lights HAL
 * mode : control mode
 * data :
 *    PWM:  pwm number
 *    GPIO: gpio id
 *    PMIC: enum mt65xx_led_pmic
 *    CUST: custom set brightness function pointer
*/
struct cust_mt65xx_led {
	char                 *name;
	enum mt65xx_led_mode  mode;
	int                   data;
 struct PWM_config config_data;
};

extern struct cust_mt65xx_led *get_cust_led_list(void);
#endif /* _CUST_LEDS_DEF_H */
