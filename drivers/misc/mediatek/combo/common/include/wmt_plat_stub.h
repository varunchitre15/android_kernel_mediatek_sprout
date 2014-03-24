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

#ifndef _WMT_PLAT_STUB_H_
#define _WMT_PLAT_STUB_H_


INT32
wmt_plat_audio_ctrl (
    CMB_STUB_AIF_X state,
    CMB_STUB_AIF_CTRL ctrl
    );

INT32 wmt_plat_stub_init (void);

#endif /*_WMT_PLAT_STUB_H_*/