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

#ifndef _MT_IDLE_H
#define _MT_IDLE_H

extern void enable_dpidle_by_bit(int id);
extern void disable_dpidle_by_bit(int id);
extern void enable_mcidle_by_bit(int id);
extern void disable_mcidle_by_bit(int id);


#endif
