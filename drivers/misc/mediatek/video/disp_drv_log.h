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

#ifndef __DISP_DRV_LOG_H__
#define __DISP_DRV_LOG_H__

///for kernel
#include <linux/xlog.h>

#define DISP_LOG_PRINT(level, sub_module, fmt, arg...)      \
    do {                                                    \
        xlog_printk(level, "DISP/"sub_module, fmt, ##arg);  \
    }while(0)
    
#define LOG_PRINT(level, module, fmt, arg...)               \
    do {                                                    \
        xlog_printk(level, module, fmt, ##arg);             \
    }while(0)

#endif // __DISP_DRV_LOG_H__
