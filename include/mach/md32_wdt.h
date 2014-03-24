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

#ifndef __MT6582_MD32INT_H__
#define __MT6582_MD32INT_H__



#define MD32_MAX_USER  20
#define MD2HOST_IPCR  0x1005001C

/*Define MD32 IRQ Type*/   
#define MD32_IPC_INT 0x100
#define WDT_INT 0x200
#define PMEM_DISP_INT 0x400
#define DMEM_DISP_INT 0x800
/*Define Watchdog Register*/
#define WDT_CON 0x10050084
#define WDT_KICT 0x10050088

typedef struct
{
    void (*wdt_func[MD32_MAX_USER]) (void *);
    void (*reset_func[MD32_MAX_USER]) (void *);
    char MODULE_NAME[MD32_MAX_USER][100];
    void *private_data[MD32_MAX_USER];
    int in_use[MD32_MAX_USER];
} md32_wdt_func;

typedef struct
{
    void (*assert_func[MD32_MAX_USER]) (void *);
    void (*reset_func[MD32_MAX_USER]) (void *);
    char MODULE_NAME[MD32_MAX_USER][100];
    void *private_data[MD32_MAX_USER];
    int in_use[MD32_MAX_USER];
} md32_assert_func;

#endif

