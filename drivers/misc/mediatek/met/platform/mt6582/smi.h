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

#ifndef _SMI_H_
#define _SMI_H_

#include <linux/device.h>

struct met_smi {
	unsigned long mode;
	unsigned long master;
	unsigned long port;
	unsigned long rwtype;//0 for R+W, 1 for read, 2 for write
	unsigned long desttype;//0 for EMI+internal mem, 1 for EMI, 3 for internal mem
	unsigned long bustype;//0 for GMC, 1 for AXI
	//unsigned long requesttype;// 0:All, 1:ultra high, 2:pre-ultrahigh, 3:normal.
	struct kobject *kobj_bus_smi;
};

struct smi_cfg {
	unsigned long master;
	unsigned long port;
	unsigned long rwtype;//0 for R+W, 1 for read, 2 for write
	unsigned long desttype;//0 for EMI+internal mem, 1 for EMI, 3 for internal mem
	unsigned long bustype;//0 for GMC, 1 for AXI
	//unsigned long requesttype;// 0:All, 1:ultra high, 2:pre-ultrahigh, 3:normal.
};

struct smi_mon_con {
	unsigned long requesttype;// 0:All, 1:ultra high, 2:pre-ultrahigh, 3:normal.
};

/*
void smi_init(void);
void smi_uninit(void);

void smi_start(void);
void smi_stop(void);

int do_smi(void);
unsigned int smi_polling(unsigned int *smi_value);
*/

#endif // _SMI_H_
