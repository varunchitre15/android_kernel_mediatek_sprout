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
#ifndef _MT_AUDIO_SSB_CUST_H_
#define _MT_AUDIO_SSB_CUST_H_
#define AUDIOPA_NAME_LEN   (8)

struct tag_audiopa_data {
    u32 version;
    int pa_type;
    char stName[AUDIOPA_NAME_LEN];
};
#endif
