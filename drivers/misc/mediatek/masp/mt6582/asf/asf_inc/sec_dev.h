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

#ifndef DEV_H
#define DEV_H

/******************************************************************************
 *  EXPORT FUNCTION
 ******************************************************************************/
extern void sec_dev_dump_part(void);
extern void sec_dev_find_parts(void);
extern int sec_dev_read_rom_info(void);
extern int sec_dev_read_secroimg(void);
extern int sec_dev_read_secroimg_v5(unsigned int index);
extern unsigned int sec_dev_read_image(char* part_name, char* buf, u64 off, unsigned int sz, unsigned int image_type);


#endif  // DEV_H
