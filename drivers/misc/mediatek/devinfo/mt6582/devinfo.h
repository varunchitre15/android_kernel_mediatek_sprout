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

#ifndef DEVFINO_H
#define DEVFINO_H

 
 /***************************************************************************** 
 * MODULE DEFINITION 
 *****************************************************************************/
#define MODULE_NAME	    "[devinfo]"
#define DEV_NAME        "devmap"
#define MAJOR_DEV_NUM    196
 
 /***************************************************************************** 
 * IOCTL DEFINITION 
 *****************************************************************************/
#define DEV_IOC_MAGIC       'd'
#define READ_DEV_DATA       _IOR(DEV_IOC_MAGIC,  1, unsigned int)

#define DEV_IOC_MAXNR       (10)

/***************************************************************************** 
* EXPORT DEFINITION 
*****************************************************************************/
extern u32 g_devinfo_data_size;
extern u32 get_devinfo_with_index(u32 index);;


#endif /* end of DEVFINO_H */

