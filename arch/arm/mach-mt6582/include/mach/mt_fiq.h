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

#ifndef __MT6577_FIQ_H__
#define __MT6577_FIQ_H__

#include <asm/fiq.h>
#include <asm/fiq_debugger.h>
#include <asm/fiq_glue.h>
#include <asm/hardware/gic.h>

#define THREAD_INFO(sp) ((struct thread_info *) \
		((unsigned long)(sp) & ~(THREAD_SIZE - 1)))

extern int request_fiq(int fiq, struct fiq_glue_handler *h);
extern int free_fiq(int fiq);

#endif

