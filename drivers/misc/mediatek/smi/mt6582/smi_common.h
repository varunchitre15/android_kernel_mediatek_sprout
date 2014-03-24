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

#ifndef __SMI_COMMON_H__
#define __SMI_COMMON_H__

#include <linux/xlog.h>
#include <linux/aee.h>


#define SMIMSG(string, args...)	xlog_printk(ANDROID_LOG_INFO, SMI_LOG_TAG, "[pid=%d]"string,current->tgid,##args)
#define SMITMP(string, args...)  xlog_printk(ANDROID_LOG_INFO, SMI_LOG_TAG, "[pid=%d]"string,current->tgid,##args)
#define SMIERR(string, args...) do{\
	xlog_printk(ANDROID_LOG_ERROR,  SMI_LOG_TAG, "error: "string, ##args); \
	aee_kernel_warning(SMI_LOG_TAG, "error: "string, ##args);  \
}while(0)

#define smi_aee_print(string, args...) do{\
    char smi_name[100];\
    snprintf(smi_name,100, "["SMI_LOG_TAG"]"string, ##args); \
  aee_kernel_warning(smi_name, "["SMI_LOG_TAG"]error:"string,##args);  \
}while(0)


#define MAU_ENTRY_NR    3
#define SMI_LARB_NR     3


extern unsigned int gLarbBaseAddr[SMI_LARB_NR];
extern char *smi_port_name[][17];


int larb_clock_on(int larb_id);
int larb_clock_off(int larb_id);

void smi_dumpDebugMsg(void);

int mau_init(void);
void SMI_DBG_Init(void);


#endif

