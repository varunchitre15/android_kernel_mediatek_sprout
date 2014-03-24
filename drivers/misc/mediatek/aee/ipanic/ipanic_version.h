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

#if !defined(__AEE_IPANIC_VERSION_H__)
#define __AEE_IPANIC_VERSION_H__

#include <linux/version.h>

struct ipanic_log_index {
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 5, 0)
    unsigned value;
#else
    u32 idx;
    u64 seq;
#endif
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 5, 0)
extern unsigned log_end;
extern int log_buf_copy2(char *dest, int dest_len, int log_copy_start, int log_copy_end);
#else
#include <linux/kmsg_dump.h>
extern u32 log_first_idx;
extern u64 log_first_seq;
extern u32 log_next_idx;
extern u64 log_next_seq;
int ipanic_kmsg_dump3(struct kmsg_dumper *dumper, char *buf, size_t len);
#endif

#endif
