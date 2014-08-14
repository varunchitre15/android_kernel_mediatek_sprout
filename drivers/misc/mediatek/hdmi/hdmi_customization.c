/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*******************************************************************************
 *
 * Filename:
 * ---------
 * hdmi_customization.h
 *
 * Project:
 * --------
 *   Android
 *
 * Description:
 * ------------
 *   This file implements Customization base function
 *
 *******************************************************************************/

#include <cust_gpio_usage.h>
#include <hdmi_customization.h>
#include <mach/mt_gpio.h>

/******************************************************************
** Basic define
******************************************************************/
#ifndef s32
	#define s32 signed int
#endif
#ifndef s64
	#define s64 signed long long
#endif
static u32 SetGPIOMode(u32 pin, u32 mode)    //set GPIO to I2S
{
	u32 temp = -1;
	temp = mt_get_gpio_mode(pin);
	printk("MHL GPIO Mode 1 is %d", temp);
	
	if(temp != mode)
		temp = mt_set_gpio_mode(pin, mode);
	printk("MHL GPIO Mode 2 is %d", temp);
	
	return temp;
}

static u32 ResetI2S(u32 pin, u32 mode)          //set I2S to GPIO and pull down
{
	u32 temp = -1;
	temp = mt_get_gpio_mode(pin);
	printk("MHL GPIO Mode 3 is %d", temp);
	if(temp == mode)
	{
		temp = mt_set_gpio_mode(pin, GPIO_MODE_00);
		printk("MHL GPIO Mode 4 is %d", temp);
		temp = mt_set_gpio_pull_enable(pin, GPIO_PULL_DISABLE);
	}
	return temp;
}


/******************************************************************
** MHL GPIO Customization
******************************************************************/
void ChangeGPIOToI2S()
{
	u32 temp = -1;
#ifdef GPIO_MHL_I2S_OUT_CK_PIN
	temp = SetGPIOMode(GPIO_MHL_I2S_OUT_CK_PIN, GPIO_MODE_03);
#endif

#ifdef GPIO_MHL_I2S_OUT_WS_PIN
	temp = SetGPIOMode(GPIO_MHL_I2S_OUT_WS_PIN, GPIO_MODE_03);
#endif

#ifdef GPIO_MHL_I2S_OUT_DAT_PIN
	temp = SetGPIOMode(GPIO_MHL_I2S_OUT_DAT_PIN, GPIO_MODE_03);
#endif
}
void ChangeI2SToGPIO()
{
	u32 temp = -1;
#ifdef GPIO_MHL_I2S_OUT_CK_PIN
	temp = ResetI2S(GPIO_MHL_I2S_OUT_CK_PIN, GPIO_MODE_03);
#endif
	
#ifdef GPIO_MHL_I2S_OUT_WS_PIN
	temp = ResetI2S(GPIO_MHL_I2S_OUT_WS_PIN, GPIO_MODE_03);
#endif
	
#ifdef GPIO_MHL_I2S_OUT_DAT_PIN
	temp = ResetI2S(GPIO_MHL_I2S_OUT_DAT_PIN, GPIO_MODE_03);
#endif
}

