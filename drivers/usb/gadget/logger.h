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

#ifndef _LOGGER_H_
#define _LOGGER_H_

#ifdef ENABLE_USB_LOGGER

#include <mach/mt_storage_logger.h>

#define USB_LOGGER(msg_id, func_name, args...) \
	do { \
		if(unlikely(is_dump_usb_gadget())) { \
			ADD_USB_TRACE(msg_id,func_name, args); \
		} \
	}while(0)

#else

#define USB_LOGGER(msg_id,func_name, args...) do{} while(0)

#endif

#endif /* _LOGGER_H_*/

