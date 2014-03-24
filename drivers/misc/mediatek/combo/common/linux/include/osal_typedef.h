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

/*! \file
    \brief  Declaration of library functions

    Any definitions in this file will be shared among GLUE Layer and internal Driver Stack.
*/


#ifndef _OSAL_TYPEDEF_H_
#define _OSAL_TYPEDEF_H_

typedef void VOID;
typedef void *PVOID;

typedef char CHAR;
typedef char *PCHAR;
typedef signed char INT8;
typedef signed char *PINT8;
typedef unsigned char UINT8;
typedef unsigned char *PUINT8;
typedef unsigned char UCHAR;
typedef unsigned char  *PUCHAR;

typedef signed short INT16;
typedef signed short *PINT16;
typedef unsigned short UINT16;
typedef unsigned short *PUINT16;

typedef signed long LONG;
typedef signed long *PLONG;

typedef signed int INT32;
typedef signed int *PINT32;
typedef unsigned int UINT32;
typedef unsigned int *PUINT32;

typedef unsigned long ULONG;
typedef unsigned long  *PULONG;

typedef int MTK_WCN_BOOL;
#ifndef MTK_WCN_BOOL_TRUE
#define MTK_WCN_BOOL_FALSE               ((MTK_WCN_BOOL) 0)
#define MTK_WCN_BOOL_TRUE                ((MTK_WCN_BOOL) 1)
#endif

#endif /*_OSAL_TYPEDEF_H_*/

