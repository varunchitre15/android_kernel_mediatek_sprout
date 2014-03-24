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

#ifndef _SI_DRVISRCONFIG_H_
#define _SI_DRVISRCONFIG_H_
void SiiMhlTxDeviceIsr( void );
void SiiExtDeviceIsr(void);
#ifdef __KERNEL__
uint8_t SiiCheckDevice(uint8_t dev);
#endif
#define SII_MHL_TX_ISR SiiMhlTxDeviceIsr
#define SII_EXT_ISR SiiExtDeviceIsr
void SiiMhlTxDeviceTimerIsr(uint8_t timerIndex);
#define CALL_SII_MHL_TX_DEVICE_TIMER_ISR(index) 
#endif
