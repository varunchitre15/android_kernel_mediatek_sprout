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

#ifndef __KERNEL__
#include "si_c99support.h"
#include "si_memsegsupport.h"
#else
#include "si_platform.h"
#endif
#define	MHL_DEV_LD_DISPLAY					(0x01 << 0)
#define	MHL_DEV_LD_VIDEO					(0x01 << 1)
#define	MHL_DEV_LD_AUDIO					(0x01 << 2)
#define	MHL_DEV_LD_MEDIA					(0x01 << 3)
#define	MHL_DEV_LD_TUNER					(0x01 << 4)
#define	MHL_DEV_LD_RECORD					(0x01 << 5)
#define	MHL_DEV_LD_SPEAKER					(0x01 << 6)
#define	MHL_DEV_LD_GUI						(0x01 << 7)
#define	MHL_MAX_RCP_KEY_CODE	(0x7F + 1)	
PLACE_IN_CODE_SEG uint8_t rcpSupportTable [MHL_MAX_RCP_KEY_CODE] = {
	(MHL_DEV_LD_GUI),		
	(MHL_DEV_LD_GUI),		
	(MHL_DEV_LD_GUI),		
	(MHL_DEV_LD_GUI),		
	(MHL_DEV_LD_GUI),		
	0, 0, 0, 0,				
	(MHL_DEV_LD_GUI),		
	0, 0, 0,				
	(MHL_DEV_LD_GUI),		
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),	
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),
	0,						
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),	
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),	
	0, 0, 0,				
	(MHL_DEV_LD_TUNER),		
	(MHL_DEV_LD_TUNER),		
	(MHL_DEV_LD_TUNER),		
	(MHL_DEV_LD_AUDIO),		
	0,						
	0,						
	0,						
	0,						
	0,						
	0, 0, 0, 0, 0, 0, 0,	
	0,						
	(MHL_DEV_LD_SPEAKER),	
	(MHL_DEV_LD_SPEAKER),	
	(MHL_DEV_LD_SPEAKER),	
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO),	
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_RECORD),	
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_RECORD),	
	(MHL_DEV_LD_RECORD),	
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO),	
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO),	
	(MHL_DEV_LD_MEDIA),		
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA),	
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA),	
	0, 0, 0,				
	0,						
	0,						
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO),	
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO),	
	(MHL_DEV_LD_RECORD),	
	(MHL_DEV_LD_RECORD),	
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_RECORD),	
	(MHL_DEV_LD_SPEAKER),	
	(MHL_DEV_LD_SPEAKER),	
	0, 0, 0, 0, 0, 0, 0, 0, 0, 	                        
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 		
};
