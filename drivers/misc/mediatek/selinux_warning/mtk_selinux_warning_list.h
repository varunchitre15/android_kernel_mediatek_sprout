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

#ifndef __MTK_SELINUX_WARNING_LIST_H__
#define __MTK_SELINUX_WARNING_LIST_H__

#ifdef SELINUX_WARNING_C
    #define AUTOEXT
#else //SELINUX_WARNING_C
    #define AUTOEXT  extern
#endif //SELINUX_WARNING_C

#define AEE_FILTER_NUM 10
AUTOEXT const char *aee_filter_list[AEE_FILTER_NUM] = 
{ 
"u:r:zygote:s0",
"u:r:netd:s0",
"u:r:installd:s0",
"u:r:vold:s0",
};
	
#endif
	
