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

#ifndef _CAM_CAL_H
#define _CAM_CAL_H

#include <linux/ioctl.h>


#define CAM_CALAGIC 'i'
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

//CAM_CAL write
#define CAM_CALIOC_S_WRITE            _IOW(CAM_CALAGIC,0,stCAM_CAL_INFO_STRUCT)
//CAM_CAL read
#define CAM_CALIOC_G_READ            _IOWR(CAM_CALAGIC,5,stPCAM_CAL_INFO_STRUCT)

#endif //_CAM_CAL_H


