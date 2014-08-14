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

#ifndef _MT_GPIO_FPGA_H_
#define _MT_GPIO_FPGA_H_

#define  MT_GPIO_BASE_START 0
typedef enum GPIO_PIN
{    
    GPIO_UNSUPPORTED = -1,    
	GPIO0 = MT_GPIO_BASE_START,
    GPIO1  , GPIO2  , GPIO3  , GPIO4  , GPIO5  , GPIO6  , GPIO7  , 
    MT_GPIO_BASE_MAX
}GPIO_PIN;    

#endif
