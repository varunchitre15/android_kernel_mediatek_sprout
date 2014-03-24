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

#ifndef _EEPROM_H
#define _EEPROM_H

#include <linux/ioctl.h>


#define EEPROMAGIC 'i'
//IOCTRL(inode * ,file * ,cmd ,arg )
//S means "set through a ptr"
//T means "tell by a arg value"
//G means "get by a ptr"
//Q means "get by return a value"
//X means "switch G and S atomically"
//H means "switch T and Q atomically"

/*******************************************************************************
*
********************************************************************************/

//EEPROM write
#define EEPROMIOC_S_WRITE            _IOW(EEPROMAGIC,0,stEEPROM_INFO_STRUCT)
//EEPROM read
#define EEPROMIOC_G_READ            _IOWR(EEPROMAGIC,5,stPEEPROM_INFO_STRUCT)

#endif //_EEPROM_H


