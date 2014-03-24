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

#ifndef _SI_DRV_EXT_H_
#define _SI_DRV_EXT_H_
#include "si_platform.h"
#ifndef __KERNEL__
#include <string.h>
#include "hal_local.h"
#include "si_common.h"
#else
#include <linux/types.h>
#endif
bool_t CheckExtVideo(void);
unsigned char  GetExt_AudioType(void);
uint8_t GetExt_inputColorSpace(void);
uint8_t GetExt_inputVideoCode(void);
uint8_t GetExt_inputcolorimetryAspectRatio(void);
uint8_t GetExt_inputAR(void);   
void InitExtVideo(void);
void TriggerExtInt(void);
#endif 
