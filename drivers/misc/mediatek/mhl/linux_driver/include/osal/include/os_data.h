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

#ifndef _OS_DATA_H
#define _OS_DATA_H
#ifdef PLATFORM3
#error This file is not designed for use on the specified platform.
#endif
#if !defined(__KERNEL__)
#include <sys/time.h>
#include <bits/local_lim.h>
#include <pthread.h>
#else
#include <linux/time.h>
#endif
#define OS_API_NAME_SIZE    16                      
#define OS_PRIV_NAME_SIZE   (OS_API_NAME_SIZE + 16) 
#define OS_NAME_SIZE        (OS_PRIV_NAME_SIZE + 16) 
#define SII_OS_STACK_SIZE_MIN    PTHREAD_STACK_MIN   
struct _SiiOsTaskInfo_t;
#define OS_CURRENT_TASK    ((struct _SiiOsTaskInfo_t *) 0)
typedef struct _SiiOsSemInfo_t * SiiOsSemaphore_t;
typedef struct _SiiOsQueueInfo_t * SiiOsQueue_t;
typedef struct _SiiOsTaskInfo_t * SiiOsTask_t;
typedef struct
{
    struct timeval theTime;
} SiiOsTime_t;
typedef struct _SiiOsTimerInfo_t * SiiOsTimer_t;
#endif 
