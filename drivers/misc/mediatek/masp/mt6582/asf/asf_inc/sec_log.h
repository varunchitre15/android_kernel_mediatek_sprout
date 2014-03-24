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

#ifndef LOGGING_H
#define LOGGING_H

/**************************************************************************
*  DEBUG CONTROL
**************************************************************************/
#include <mach/sec_osal.h>  
#include "sec_osal_light.h"
#define NEED_TO_PRINT(flag)         ((flag) == true)
#define SMSG(debug_level, ...) do \
                                    { \
                                        if(NEED_TO_PRINT(debug_level)) \
                                            printk(__VA_ARGS__); \
                                    } while(0);

/**************************************************************************
 *  EXTERNAL VARIABLE
 **************************************************************************/
extern bool                         bMsg;

#endif /* LOGGING_H */

