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

#ifndef __MT_BOOT_H__
#define __MT_BOOT_H__
#include <mach/mt_boot_common.h>

/*META COM port type*/
 typedef enum
{
    META_UNKNOWN_COM = 0,
    META_UART_COM,
    META_USB_COM
} META_COM_TYPE;

#define BOOT_DEV_NAME           "BOOT"
#define BOOT_SYSFS              "boot"
#define BOOT_SYSFS_ATTR         "boot_mode"
#define MD_SYSFS_ATTR           "md"
#define INFO_SYSFS_ATTR         "info"

typedef enum {
    CHIP_SW_VER_01 = 0x0000,
    CHIP_SW_VER_02 = 0x0001
} CHIP_SW_VER;

extern META_COM_TYPE g_meta_com_type;
extern unsigned int g_meta_com_id;

extern void set_meta_com(META_COM_TYPE type, unsigned int id);
extern META_COM_TYPE get_meta_com_type(void);
extern unsigned int get_meta_com_id(void);

extern unsigned int get_chip_code(void);
extern unsigned int get_chip_hw_subcode(void);
extern unsigned int get_chip_hw_ver_code(void);
extern unsigned int get_chip_sw_ver_code(void);
extern unsigned int mt_get_chip_id(void);
extern CHIP_SW_VER  mt_get_chip_sw_ver(void);

#endif 

