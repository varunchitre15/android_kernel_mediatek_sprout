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

#ifndef __EINT_DRV_H
#define __EINT_DRV_H

#include <mach/eint.h>

struct mt_eint_driver
{
    struct platform_driver driver;
    int (*eint_max_channel)(void);
    void (*enable)(unsigned int eint_num);
    void (*disable)(unsigned int eint_num);
    unsigned int (*is_disable)(unsigned int eint_num);
    unsigned int (*get_sens)(unsigned int eint_num);
    unsigned int (*set_sens)(unsigned int eint_num, unsigned int sens);
    unsigned int (*get_polarity)(unsigned int eint_num);
    void (*set_polarity)(unsigned int eint_num, unsigned int pol);
    unsigned int (*get_debounce_cnt)(unsigned int eint_num);
    void (*set_debounce_cnt)(unsigned int eint_num, unsigned int ms);
    int (*is_debounce_en)(unsigned int eint_num);
    void (*enable_debounce)(unsigned int eint_num);
    void (*disable_debounce)(unsigned int eint_num);
    unsigned int (*get_count)(unsigned int eint_num);
};

struct mt_eint_driver *get_mt_eint_drv(void);

extern int eint_drv_get_max_channel(void);
extern unsigned int eint_drv_get_count(unsigned int eint_num);

#endif
