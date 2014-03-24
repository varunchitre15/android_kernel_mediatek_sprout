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

#ifndef _ETB_CONTROL_H_
#define _ETB_CONTROL_H_

/** formatter mode */
enum {
	FORMATTER_MODE_BYPASS = 0, /**< bypass mode */
	FORMATTER_MODE_NORMAL, /**< normal mode */
	FORMATTER_MODE_CONTINUOUS, /**< continuous mode */
};

/** ETB control value */ 
struct etb_control_t
{
	int formatter_mode; /**< ETB formattor mode */
	unsigned int formatter_control_value; /**< ETB formattor control value */
};

#endif

