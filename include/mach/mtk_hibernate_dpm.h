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

#ifndef __MTK_HIBERNATE_DPM_H
#define __MTK_HIBERNATE_DPM_H


//-------------other configure-------------------------//
#define MAX_CB_FUNCS	  (10)

//-------------error code define-----------------------//
#define E_NO_EXIST		  (-1)
#define E_PARAM			  (-2)

typedef enum {
    ID_M_BEGIN  = 0,
    ID_M_DEVAPC = 0,
    ID_M_VCODEC = 1,
    ID_M_SPC    = 2,
    ID_M_MSDC   = 3,
    ID_M_MALI   = 4,
    ID_M_MJC    = 5,
    ID_M_CONNSYS = 6,
    ID_M_END = MAX_CB_FUNCS,
} KERN_FUNC_ID;

//-------------structure define------------------------//
typedef int (*swsusp_cb_func_t)(struct device *device);
typedef struct {
	KERN_FUNC_ID		id;
	swsusp_cb_func_t    func;
    struct device       *device;
} swsusp_cb_func_info;

//-----------------export function declaration----------------------------//
int register_swsusp_restore_noirq_func(unsigned int id, swsusp_cb_func_t func, struct device *device);
int unregister_swsusp_restore_noirq_func(unsigned int id);
int exec_swsusp_restore_noirq_func(unsigned int id);

#endif
