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

#ifndef _FM50AF_H
#define _FM50AF_H

#include <linux/ioctl.h>
//#include "kd_imgsensor.h"

#define FM50AF_MAGIC 'A'
//IOCTRL(inode * ,file * ,cmd ,arg )


//Structures
typedef struct {
//current position
unsigned long u4CurrentPosition;
//macro position
unsigned long u4MacroPosition;
//Infiniti position
unsigned long u4InfPosition;
//Motor Status
bool          bIsMotorMoving;
//Motor Open?
bool          bIsMotorOpen;
//Support SR?
bool          bIsSupportSR;
} stFM50AF_MotorInfo;

//Control commnad
//S means "set through a ptr"
//T means "tell by a arg value"
//G means "get by a ptr"             
//Q means "get by return a value"
//X means "switch G and S atomically"
//H means "switch T and Q atomically"
#define FM50AFIOC_G_MOTORINFO _IOR(FM50AF_MAGIC,0,stFM50AF_MotorInfo)

#define FM50AFIOC_T_MOVETO _IOW(FM50AF_MAGIC,1,unsigned long)

#define FM50AFIOC_T_SETINFPOS _IOW(FM50AF_MAGIC,2,unsigned long)

#define FM50AFIOC_T_SETMACROPOS _IOW(FM50AF_MAGIC,3,unsigned long)

#else
#endif
