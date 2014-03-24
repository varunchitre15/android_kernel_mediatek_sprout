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

#ifndef _VAL_LOG_H_
#define _VAL_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/kernel.h>
#include <linux/xlog.h>

#define MFV_LOG_ERROR   //error
#ifdef MFV_LOG_ERROR
#define MFV_LOGE(...) xlog_printk(ANDROID_LOG_ERROR, "VDO_LOG", __VA_ARGS__);
#else
#define MFV_LOGE(...)
#endif

#define MFV_LOG_WARNING //warning
#ifdef MFV_LOG_WARNING
#define MFV_LOGW(...) xlog_printk(ANDROID_LOG_WARN, "VDO_LOG", __VA_ARGS__);
#else
#define MFV_LOGW(...)
#endif


#define MFV_LOG_DEBUG   //debug information
#ifdef MFV_LOG_DEBUG
#define MFV_LOGD(...) xlog_printk(ANDROID_LOG_DEBUG, "VDO_LOG", __VA_ARGS__);
#else
#define MFV_LOGD(...)
#endif

#ifdef __cplusplus
}
#endif

#endif // #ifndef _VAL_LOG_H_
