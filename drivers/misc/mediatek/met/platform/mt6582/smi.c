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

#include <linux/module.h>

#include "core/met_drv.h"
#include "core/trace.h"

//#include "mt_reg_base.h"
#include "mt_smi.h"
//#include "sync_write.h"
//#include "mt_typedefs.h"
#include "smi.h"
#include "smi_name.h"
#include "plf_trace.h"

extern struct metdevice met_smi;
static int enable_master_cnt = 0;

static int count = SMI_LARB_NUMBER+SMI_COMM_NUMBER;
static int portnum = SMI_ALLPORT_COUNT;

static struct kobject *kobj_smi = NULL;
static struct kobject *kobj_smi_mon_con = NULL;
static struct met_smi smi_larb[SMI_LARB_NUMBER];
static struct met_smi smi_comm[SMI_COMM_NUMBER];

static int toggle_idx = 0;
static int toggle_cnt = 1000;
static int toggle_master = 0;
static int toggle_master_min = -1;
static int toggle_master_max = -1;

static struct smi_cfg allport[SMI_ALLPORT_COUNT*4];
static SMIBMCfg_Ext monitorctrl;

struct chip_smi {
	unsigned int master;
	struct smi_desc *desc;
	unsigned int count;
};

static struct chip_smi smi_map[] ={
	{ 0, larb0_desc, SMI_LARB0_DESC_COUNT }, //larb0
	{ 1, larb1_desc, SMI_LARB1_DESC_COUNT }, //larb1
	{ 2, larb2_desc, SMI_LARB2_DESC_COUNT }, //larb2
//	{ 3, larb3_desc, SMI_LARB3_DESC_COUNT }, //larb3
//	{ 4, larb4_desc, SMI_LARB4_DESC_COUNT }, //larb4
	{ SMI_LARB_NUMBER, common_desc, SMI_COMMON_DESC_COUNT } //common
};

static inline struct met_smi *lookup_smi(struct kobject *kobj)
{
	int i;

	for (i=0; i<SMI_LARB_NUMBER; i++) {
		if (smi_larb[i].kobj_bus_smi == kobj) {
			return &smi_larb[i];
		}
	}
	for (i=0; i<SMI_COMM_NUMBER; i++) {
		if (smi_comm[i].kobj_bus_smi == kobj) {
			return &smi_comm[i];
		}
	}

	return NULL;
}

static ssize_t on_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct met_smi *p = lookup_smi(kobj);
	if (p != NULL) {
		return snprintf(buf, PAGE_SIZE, "%lu\n", p->mode);
	}
	return -EINVAL;
}

static ssize_t on_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t n)
{
//	int i;
	struct met_smi *p = lookup_smi(kobj);
	unsigned long mode;

	if (p != NULL) {
        //printk("port store begin %lu\n", p->port);
		if (sscanf(buf, "%lu", &(mode)) != 1) {
			return -EINVAL;
		}
        //printk("port store end %lu\n", p->port);
		p->mode = mode;
		return n;
	}
	return -EINVAL;
}

static ssize_t port_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct met_smi *p = lookup_smi(kobj);
	if (p != NULL) {
		return snprintf(buf, PAGE_SIZE, "%lu\n", p->port);
	}
	return -EINVAL;
}

static ssize_t port_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t n)
{
	int i;
	struct met_smi *p = lookup_smi(kobj);
	unsigned long port;

	if (p != NULL) {
        //printk("port store begin %lu\n", p->port);
		if (sscanf(buf, "%lu", &(port)) != 1) {
			return -EINVAL;
		} else {
			for (i=0; i<smi_map[p->master].count; i++) {
				if (smi_map[p->master].desc[i].port == port) {
					break;
				}
			}
			if (i == smi_map[p->master].count) {
				return -EINVAL;
			}
		}
        //printk("port store end %lu\n", p->port);
		p->port = port;
		return n;
	}
	return -EINVAL;
}

static ssize_t bustype_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct met_smi *p = lookup_smi(kobj);
	if (p != NULL) {
		return snprintf(buf, PAGE_SIZE, "%lu\n", p->bustype);
	}
	return -EINVAL;
}

static ssize_t bustype_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t n)
{
	int i;
	struct met_smi *p = lookup_smi(kobj);
	unsigned long bustype;

	if (p != NULL) {
		if (sscanf(buf, "%lu", &(bustype)) != 1) {
			return -EINVAL;
		} else {
			for (i=0; i<smi_map[p->master].count; i++) {
				if (smi_map[p->master].desc[i].port == p->port) {
					if (SMI_BUS_NONE == smi_map[p->master].desc[i].bustype) {
						break;
					} else if (smi_map[p->master].desc[i].bustype == bustype) {
						break;
					}
				}
			}
			if (i == smi_map[p->master].count) {
				return -EINVAL;
			}
		}
		p->bustype = bustype;
		return n;
	}
	return -EINVAL;
}

static ssize_t desttype_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct met_smi *p = lookup_smi(kobj);
	if (p != NULL) {
		return snprintf(buf, PAGE_SIZE, "%lu\n", p->desttype);
	}
	return -EINVAL;
}

static ssize_t desttype_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t n)
{
	int i;
	struct met_smi *p = lookup_smi(kobj);
	unsigned long desttype;

	if (p != NULL) {
		if (sscanf(buf, "%lu", &(desttype)) != 1) {
			return -EINVAL;
		} else {
			for (i=0; i<smi_map[p->master].count; i++) {
				if (smi_map[p->master].desc[i].port == p->port) {
					if (SMI_DEST_NONE == smi_map[p->master].desc[i].desttype) {
						break;
					} else if ( SMI_DEST_ALL == smi_map[p->master].desc[i].desttype) {
						if (desttype == SMI_DEST_ALL ||
						    desttype == SMI_DEST_EMI ||
						    desttype == SMI_DEST_INTERNAL)
						{ break; }
					} else if (desttype == smi_map[p->master].desc[i].desttype) {
						break;
					}
				}
			}
			if (i == smi_map[p->master].count) {
				return -EINVAL;
			}
		}
		p->desttype = desttype;
		return n;
	}
	return -EINVAL;
}

static ssize_t rwtype_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct met_smi *p = lookup_smi(kobj);
	if (p != NULL) {
		return snprintf(buf, PAGE_SIZE, "%lu\n", p->rwtype);
	}
	return -EINVAL;
}

static ssize_t rwtype_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t n)
{
	int i;
	struct met_smi *p = lookup_smi(kobj);
	unsigned long rwtype;

	if (p != NULL) {
		if (sscanf(buf, "%lu", &(rwtype)) != 1) {
			return -EINVAL;
		} else {
			for (i=0; i<smi_map[p->master].count; i++) {
				if (smi_map[p->master].desc[i].port == p->port) {
					if ( SMI_RW_ALL == smi_map[p->master].desc[i].rwtype) {
						if (rwtype == SMI_RW_ALL ||
						    rwtype == SMI_READ_ONLY ||
						    rwtype == SMI_WRITE_ONLY)
						{ break; }
					} else if ( SMI_RW_RESPECTIVE == smi_map[p->master].desc[i].rwtype) {
						if (rwtype == SMI_READ_ONLY ||
						    rwtype == SMI_WRITE_ONLY)
						{ break; }
					}
				}
			}
			if (i == smi_map[p->master].count) {
				return -EINVAL;
			}
		}
		p->rwtype = rwtype;
		return n;
	}
	return -EINVAL;
}

static ssize_t requesttype_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", (int)monitorctrl.bRequestSelection);
}

static ssize_t requesttype_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t n)
{
	unsigned long requesttype;
	if (sscanf(buf, "%lu", &(requesttype)) != 1) {
		return -EINVAL;
	}
	monitorctrl.bRequestSelection = requesttype;
	return n;
}


static ssize_t toggle_cnt_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", toggle_cnt);
}

static ssize_t toggle_cnt_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t n)
{
	if (sscanf(buf, "%d", &(toggle_cnt)) != 1) {
			return -EINVAL;
	}
	return n;
}

static ssize_t toggle_master_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", toggle_master);
}

static ssize_t toggle_master_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t n)
{
	int t;
	if (sscanf(buf, "%d", &t) != 1) {
		return -EINVAL;
	}
	if ( t >= 0 && t < SMI_LARB_NUMBER+SMI_COMM_NUMBER) {
		toggle_master = t;
		return n;
	}
	return -EINVAL;
}

static ssize_t count_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", count);
}

static ssize_t count_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t n)
{
	return -EINVAL;
}

static ssize_t portnum_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", portnum);
}

static ssize_t portnum_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t n)
{
	return -EINVAL;
}

static struct kobj_attribute on_attr = __ATTR(mode, 0644, on_show, on_store);
static struct kobj_attribute port_attr = __ATTR(port, 0644, port_show, port_store);
static struct kobj_attribute bustype_attr = __ATTR(bustype, 0644, bustype_show, bustype_store);
static struct kobj_attribute desttype_attr = __ATTR(desttype, 0644, desttype_show, desttype_store);
static struct kobj_attribute rwtype_attr = __ATTR(rwtype, 0644, rwtype_show, rwtype_store);
static struct kobj_attribute requesttype_attr = __ATTR(requesttype, 0644, requesttype_show, requesttype_store);
static struct kobj_attribute toggle_cnt_attr = __ATTR(toggle_cnt, 0644, toggle_cnt_show, toggle_cnt_store);
static struct kobj_attribute toggle_master_attr = __ATTR(toggle_master, 0644, toggle_master_show, toggle_master_store);
static struct kobj_attribute count_attr = __ATTR(count, 0644, count_show, count_store);
static struct kobj_attribute portnum_attr = __ATTR(portnum, 0644, portnum_show, portnum_store);

static int do_smi(void)
{
#if 1
	return met_smi.mode;
#else
	static int do_smi = -1;

	if (do_smi != -1) {
		return do_smi;
	}

	if (met_smi.mode == 0) {
		do_smi = 0;
	} else {
		do_smi = met_smi.mode;
	}
	return do_smi;
#endif
}

static void toggle_port(int toggle_idx)
{
	int i;
	i = allport[toggle_idx].master;
	if (i < SMI_LARB_NUMBER) {
		smi_larb[i].port = allport[toggle_idx].port;
		smi_larb[i].rwtype = allport[toggle_idx].rwtype;
		smi_larb[i].desttype = allport[toggle_idx].desttype;
		smi_larb[i].bustype = allport[toggle_idx].bustype;
		SMI_Disable(i);
		SMI_Clear(i);
		SMI_SetSMIBMCfg(i, smi_larb[i].port, smi_larb[i].desttype, smi_larb[i].rwtype);
		//SMI_SetMonitorControl(0);  //SMIBMCfgEx default
		SMI_SetMonitorControl(&monitorctrl);
		SMI_Enable(i, smi_larb[i].bustype);
	} else {
		i = i - SMI_LARB_NUMBER;
		smi_comm[i].port = allport[toggle_idx].port;
		smi_comm[i].rwtype = allport[toggle_idx].rwtype;
		smi_comm[i].desttype = allport[toggle_idx].desttype;
		smi_comm[i].bustype = allport[toggle_idx].bustype;
		SMI_Comm_Disable(i);
		SMI_Comm_Clear(i);
		SMI_SetCommBMCfg(i, smi_comm[i].port, smi_comm[i].desttype, smi_comm[i].rwtype);
		SMI_Comm_Enable(i);
	};
}

static void smi_init_value(void)
{
	int i;
	int m1=0,m2=0,m3=0/*,m4=0,m5=0*/,m6=0;

	printk("smi_init_value\n");

	monitorctrl.bIdleSelection = 1;
	monitorctrl.uIdleOutStandingThresh = 3;
	monitorctrl.bDPSelection = 1;
	monitorctrl.bMaxPhaseSelection = 1;
	monitorctrl.bRequestSelection = SMI_REQ_ALL;
	monitorctrl.bStarvationEn = 1;
	monitorctrl.uStarvationTime = 8;

	for (i=0; i<SMI_LARB_NUMBER; i++) {
		smi_larb[i].mode = 0;
		smi_larb[i].master = i;
		smi_larb[i].port = 0;
		smi_larb[i].desttype = SMI_DEST_EMI;
		smi_larb[i].rwtype = SMI_RW_ALL;
		smi_larb[i].bustype = SMI_BUS_GMC;
	}
	for (i=0; i<SMI_COMM_NUMBER; i++) {
		smi_comm[i].mode = 0;
		smi_comm[i].master = SMI_LARB_NUMBER + i;
		smi_comm[i].port = 4;	//GPU
		smi_comm[i].desttype = SMI_DEST_EMI;	//EMI
		smi_comm[i].rwtype = SMI_READ_ONLY;
		smi_comm[i].bustype = SMI_BUS_NONE;
	}

	for (i=0;i<SMI_ALLPORT_COUNT*4;i++) {
		if(i<SMI_LARB0_DESC_COUNT*4) {
			allport[i].master = 0;
			allport[i].port = (m1/4);
			allport[i].bustype = 0;
			allport[i].desttype = ((m1%2) ? 3 : 1);
			allport[i].rwtype = ((m1&0x2)? 2: 1);
			m1++;
		} else if(i<(SMI_LARB0_DESC_COUNT+SMI_LARB1_DESC_COUNT)*4) {
			allport[i].master = 1;
			allport[i].port = (m2/4);
			allport[i].bustype = 0;
			allport[i].desttype = ((m2%2) ? 3 : 1);
			allport[i].rwtype = ((m2&0x2)? 2: 1);
			m2++;
		} else if(i<(SMI_LARB0_DESC_COUNT+SMI_LARB1_DESC_COUNT+SMI_LARB2_DESC_COUNT)*4) {
			allport[i].master = 2;
			allport[i].port = (m3/4);
			allport[i].bustype = 0;
			allport[i].desttype = ((m3%2) ? 3 : 1);
			allport[i].rwtype = ((m3&0x2)? 2: 1);
			m3++;
		} else if(i<SMI_ALLPORT_COUNT*4) {
			allport[i].master = 3;
			allport[i].port = (m6/4);
			allport[i].bustype = 1;
			allport[i].desttype = ((m6%2) ? 3 : 1);
			allport[i].rwtype = ((m6&0x2)? 2: 1);
			m6++;
		} else {
			printk("Error: SMI Index overbound");
		}
	}
}

static void smi_init(void)
{
	int i;
	SMI_Init();
	SMI_PowerOn();

	if (do_smi() == 1) {
		for (i=0; i< SMI_LARB_NUMBER; i++) {
			SMI_Disable(i);
			SMI_SetSMIBMCfg(i, smi_larb[i].port, smi_larb[i].desttype, smi_larb[i].rwtype);
			if (smi_larb[i].mode == 1) {
				enable_master_cnt += 1;
			}
		}
		//SMI_SetMonitorControl(0);  //SMIBMCfgEx default
		SMI_SetMonitorControl(&monitorctrl);

		for (i=0; i< SMI_COMM_NUMBER; i++) {
			SMI_Comm_Disable(i);
			SMI_SetCommBMCfg(i, smi_comm[i].port, smi_comm[i].desttype, smi_comm[i].rwtype);
			if (smi_comm[i].mode == 1) {
				enable_master_cnt += 1;
			}
		}
	} else if (do_smi() == 2) {
		toggle_idx = 0;
		toggle_port(toggle_idx);
	} else if (do_smi() == 3) {
		toggle_master_max = toggle_master_min = -1;
		for (i=0; i<SMI_ALLPORT_COUNT*4; i++) {
			if (allport[i].master == toggle_master) {
				if (toggle_master_min == -1) {
					toggle_master_max = i;
					toggle_master_min = i;
				}
				if (i > toggle_master_max) {
					toggle_master_max = i;
				}
				if (i < toggle_master_min) {
					toggle_master_min = i;
				}
			}
		}
		printk("smi toggle min=%d, max=%d\n",toggle_master_min,toggle_master_max);
		if (toggle_master_min >=0 ) {
			toggle_idx = toggle_master_min;
			toggle_port(toggle_idx);
		}
	} else if (do_smi() == 4) {

	}
}

static void smi_start(void)
{
    int i;
    for (i=0; i< SMI_LARB_NUMBER; i++) {
        SMI_Enable(i, smi_larb[i].bustype);
    }
    for (i=0; i< SMI_COMM_NUMBER; i++) {
        SMI_Comm_Enable(i);
    }

}

static void smi_start_master(int i)
{
    if (i < SMI_LARB_NUMBER) {
        SMI_Enable(i, smi_larb[i].bustype);
    } else {
        SMI_Comm_Enable(i-SMI_LARB_NUMBER);
    }
}

static void smi_stop(void)
{
    int i;
    for (i=0; i< SMI_LARB_NUMBER; i++) {
        SMI_Clear(i);
    }
    for (i=0; i< SMI_COMM_NUMBER; i++) {
        SMI_Comm_Clear(i);
    }
}

static void smi_stop_master(int i)
{
    if (i < SMI_LARB_NUMBER) {
        SMI_Clear(i);
    } else {
        SMI_Comm_Clear(i-SMI_LARB_NUMBER);
    }
}


static unsigned int smi_polling(unsigned int *smi_value)
{
	int i=0,j=-1;

	//return 0;
	for (i=0; i<SMI_LARB_NUMBER; i++) {
		SMI_Pause(i);
	}

	for (i=0; i<SMI_COMM_NUMBER; i++) {
		SMI_Comm_Disable(i);
	}
#if 0
	smi_value[++j] = SMI_LARB_NUMBER+SMI_COMM_NUMBER;
	smi_value[++j] = 7;
	// read counter
	for (i=0; i<SMI_LARB_NUMBER; i++) {
		if (smi_larb[i].mode == 1) {
			smi_value[++j] = smi_larb[i].master;   //master
			smi_value[++j] = smi_larb[i].port;   //portNo
			smi_value[++j] = SMI_GetActiveCnt(i);    //ActiveCnt
			smi_value[++j] = SMI_GetRequestCnt(i);    //RequestCnt
			smi_value[++j] = SMI_GetIdleCnt(i);      //IdleCnt
			smi_value[++j] = SMI_GetBeatCnt(i);    //BeatCnt
			smi_value[++j] = SMI_GetByteCnt(i);    //ByteCnt
		}
	}
	for (i=0; i<SMI_COMM_NUMBER; i++) {
		if (smi_comm[i].mode == 1) {
			smi_value[++j] = SMI_LARB_NUMBER+i;   //fake master
			smi_value[++j] = smi_comm[i].port;   //portNo
			smi_value[++j] = SMI_Comm_GetActiveCnt(i);    //ActiveCnt
			smi_value[++j] = SMI_Comm_GetRequestCnt(i);    //RequestCnt
			smi_value[++j] = SMI_Comm_GetIdleCnt(i);      //IdleCnt
			smi_value[++j] = SMI_Comm_GetBeatCnt(i);    //BeatCnt
			smi_value[++j] = SMI_Comm_GetByteCnt(i);    //ByteCnt
		}
	}
#else
	smi_value[++j] = enable_master_cnt;
	smi_value[++j] = 16;
	// read counter
	for (i=0; i<SMI_LARB_NUMBER; i++) {
		if (smi_larb[i].mode == 1 ) {
			smi_value[++j] = smi_larb[i].master;   //master
			smi_value[++j] = smi_larb[i].port;   //portNo
			smi_value[++j] = SMI_GetActiveCnt(i);    //ActiveCnt
			smi_value[++j] = SMI_GetRequestCnt(i);    //RequestCnt
			smi_value[++j] = SMI_GetIdleCnt(i);      //IdleCnt
			smi_value[++j] = SMI_GetBeatCnt(i);    //BeatCnt
			smi_value[++j] = SMI_GetByteCnt(i);    //ByteCnt

			smi_value[++j] = SMI_GetCPCnt(i);    //CPCnt
			smi_value[++j] = SMI_GetDPCnt(i);    //DPCnt
			smi_value[++j] = SMI_GetCDP_MAX(i);    //CDP_MAX
			smi_value[++j] = SMI_GetCOS_MAX(i);    //COS_MAX
			smi_value[++j] = SMI_GetBUS_REQ0(i);    //BUS_REQ0
			smi_value[++j] = SMI_GetBUS_REQ1(i);    //BUS_REQ1
			smi_value[++j] = SMI_GetWDTCnt(i);    //WDTCnt
			smi_value[++j] = SMI_GetRDTCnt(i);    //RDTCnt
			smi_value[++j] = SMI_GetOSTCnt(i);    //OSTCnt
		}
	}
	for (i=0; i<SMI_COMM_NUMBER; i++) {
		if (smi_comm[i].mode == 1) {
			smi_value[++j] = SMI_LARB_NUMBER+i;   //fake master
			smi_value[++j] = smi_comm[i].port;   //portNo
			smi_value[++j] = SMI_Comm_GetActiveCnt(i);    //ActiveCnt
			smi_value[++j] = SMI_Comm_GetRequestCnt(i);    //RequestCnt
			smi_value[++j] = SMI_Comm_GetIdleCnt(i);      //IdleCnt
			smi_value[++j] = SMI_Comm_GetBeatCnt(i);    //BeatCnt
			smi_value[++j] = SMI_Comm_GetByteCnt(i);    //ByteCnt

			smi_value[++j] = SMI_Comm_GetCPCnt(i);    //CPCnt
			smi_value[++j] = SMI_Comm_GetDPCnt(i);    //DPCnt
			smi_value[++j] = SMI_Comm_GetCDP_MAX(i);    //CDP_MAX
			smi_value[++j] = SMI_Comm_GetCOS_MAX(i);    //COS_MAX
			smi_value[++j] = 0;    //BUS_REQ0
			smi_value[++j] = 0;    //BUS_REQ1
			smi_value[++j] = 0;    //WDTCnt
			smi_value[++j] = 0;    //RDTCnt
			smi_value[++j] = 0;    //OSTCnt
		}
	}
#endif

	smi_stop();
	smi_start();

	return j+1;
}

static unsigned int smi_dump_polling(unsigned int *smi_value)
{
	int j = -1;
	int i ;

	i = allport[toggle_idx].master;
	//return 0;
	if (allport[toggle_idx].master < SMI_LARB_NUMBER ) {
		SMI_Pause(i);
		smi_value[++j] = allport[toggle_idx].master;   //larb master
		smi_value[++j] = allport[toggle_idx].port;   //portNo
		smi_value[++j] = allport[toggle_idx].rwtype;
		smi_value[++j] = allport[toggle_idx].desttype;
		smi_value[++j] = allport[toggle_idx].bustype;

		smi_value[++j] = SMI_GetActiveCnt(i);    //ActiveCnt
		smi_value[++j] = SMI_GetRequestCnt(i);    //RequestCnt
		smi_value[++j] = SMI_GetIdleCnt(i);      //IdleCnt
		smi_value[++j] = SMI_GetBeatCnt(i);    //BeatCnt
		smi_value[++j] = SMI_GetByteCnt(i);    //ByteCnt
		smi_value[++j] = SMI_GetCPCnt(i);    //CPCnt
		smi_value[++j] = SMI_GetDPCnt(i);    //DPCnt
		smi_value[++j] = SMI_GetCDP_MAX(i);    //CDP_MAX
		smi_value[++j] = SMI_GetCOS_MAX(i);    //COS_MAX
		smi_value[++j] = SMI_GetBUS_REQ0(i);    //BUS_REQ0
		smi_value[++j] = SMI_GetBUS_REQ1(i);    //BUS_REQ1
		smi_value[++j] = SMI_GetWDTCnt(i);    //WDTCnt
		smi_value[++j] = SMI_GetRDTCnt(i);    //RDTCnt
		smi_value[++j] = SMI_GetOSTCnt(i);    //OSTCnt

	} else {
		i = allport[toggle_idx].master - SMI_LARB_NUMBER;
		SMI_Comm_Disable(i);
		smi_value[++j] = allport[toggle_idx].master;   //fake master
		smi_value[++j] = allport[toggle_idx].port;   //portNo
		smi_value[++j] = allport[toggle_idx].rwtype;
		smi_value[++j] = allport[toggle_idx].desttype;
		smi_value[++j] = allport[toggle_idx].bustype;

		smi_value[++j] = SMI_Comm_GetActiveCnt(i);    //ActiveCnt
		smi_value[++j] = SMI_Comm_GetRequestCnt(i);    //RequestCnt
		smi_value[++j] = SMI_Comm_GetIdleCnt(i);      //IdleCnt
		smi_value[++j] = SMI_Comm_GetBeatCnt(i);    //BeatCnt
		smi_value[++j] = SMI_Comm_GetByteCnt(i);    //ByteCnt
		smi_value[++j] = SMI_Comm_GetCPCnt(i);    //CPCnt
		smi_value[++j] = SMI_Comm_GetDPCnt(i);    //DPCnt
		smi_value[++j] = SMI_Comm_GetCDP_MAX(i);    //CDP_MAX
		smi_value[++j] = SMI_Comm_GetCOS_MAX(i);    //COS_MAX
	}
	smi_stop_master(allport[toggle_idx].master);
	smi_start_master(allport[toggle_idx].master);
	return j+1;
}

static void smi_uninit(void)
{
	//SMI_DeInit();
	SMI_PowerOff();
}

static int met_smi_create(struct kobject *parent)
{
	int ret = 0;
	int i;
	char buf[16];

	smi_init_value();

	kobj_smi = parent;

	ret = sysfs_create_file(kobj_smi, &toggle_cnt_attr.attr);
	if (ret != 0) {
		pr_err("Failed to create toggle_cnt in sysfs\n");
		return ret;
	}

	ret = sysfs_create_file(kobj_smi, &toggle_master_attr.attr);
	if (ret != 0) {
		pr_err("Failed to create toggle_master in sysfs\n");
		return ret;
	}

	ret = sysfs_create_file(kobj_smi, &count_attr.attr);
	if (ret != 0) {
		pr_err("Failed to create count in sysfs\n");
		return ret;
	}

	ret = sysfs_create_file(kobj_smi, &portnum_attr.attr);
	if (ret != 0) {
		pr_err("Failed to create portnum in sysfs\n");
		return ret;
	}

	snprintf(buf, sizeof(buf), "monitorctrl");
	kobj_smi_mon_con = kobject_create_and_add(buf, kobj_smi);
	ret = sysfs_create_file(kobj_smi_mon_con, &requesttype_attr.attr);
	if (ret != 0) {
		pr_err("Failed to create requesttype in sysfs\n");
	}

	for (i=0; i<SMI_LARB_NUMBER; i++) {
		snprintf(buf, sizeof(buf), "%d", i);
		smi_larb[i].kobj_bus_smi = kobject_create_and_add(buf, kobj_smi);

		ret = sysfs_create_file(smi_larb[i].kobj_bus_smi, &on_attr.attr);
		if (ret != 0) {
			pr_err("Failed to create mode in sysfs\n");
		}

		ret = sysfs_create_file(smi_larb[i].kobj_bus_smi, &port_attr.attr);
		if (ret != 0) {
			pr_err("Failed to create port in sysfs\n");
		}

		ret = sysfs_create_file(smi_larb[i].kobj_bus_smi, &bustype_attr.attr);
		if (ret != 0) {
			pr_err("Failed to create bustype in sysfs\n");
		}

		ret = sysfs_create_file(smi_larb[i].kobj_bus_smi, &desttype_attr.attr);
		if (ret != 0) {
			pr_err("Failed to create desttype in sysfs\n");
		}
		ret = sysfs_create_file(smi_larb[i].kobj_bus_smi, &rwtype_attr.attr);
		if (ret != 0) {
			pr_err("Failed to create rwtype in sysfs\n");
		}
		//ret = sysfs_create_file(smi_larb[i].kobj_bus_smi, &requesttype_attr.attr);
		//if (ret != 0) {
		//	pr_err("Failed to create requesttype in sysfs\n");
		//}

	}
	for (i=0; i<SMI_COMM_NUMBER; i++) {
		snprintf(buf, sizeof(buf), "%d", SMI_LARB_NUMBER+i);
		smi_comm[i].kobj_bus_smi = kobject_create_and_add(buf, kobj_smi);

		ret = sysfs_create_file(smi_comm[i].kobj_bus_smi, &on_attr.attr);
		if (ret != 0) {
			pr_err("Failed to create mode in sysfs\n");
		}

		ret = sysfs_create_file(smi_comm[i].kobj_bus_smi, &port_attr.attr);
		if (ret != 0) {
			pr_err("Failed to create port in sysfs\n");
		}

		ret = sysfs_create_file(smi_comm[i].kobj_bus_smi, &bustype_attr.attr);
		if (ret != 0) {
			pr_err("Failed to create bustype in sysfs\n");
		}

		ret = sysfs_create_file(smi_comm[i].kobj_bus_smi, &desttype_attr.attr);
		if (ret != 0) {
			pr_err("Failed to create desttype in sysfs\n");
		}
		ret = sysfs_create_file(smi_comm[i].kobj_bus_smi, &rwtype_attr.attr);
		if (ret != 0) {
			pr_err("Failed to create rwtype in sysfs\n");
		}

	}

	return ret;
}

void met_smi_delete(void)
{
	int i;

	if (kobj_smi != NULL) {
		for (i=0; i<SMI_LARB_NUMBER; i++) {
			sysfs_remove_file(smi_larb[i].kobj_bus_smi, &port_attr.attr);
			sysfs_remove_file(smi_larb[i].kobj_bus_smi, &bustype_attr.attr);
			sysfs_remove_file(smi_larb[i].kobj_bus_smi, &desttype_attr.attr);
			sysfs_remove_file(smi_larb[i].kobj_bus_smi, &rwtype_attr.attr);
			kobject_del(smi_larb[i].kobj_bus_smi);
		}
		for (i=0; i<SMI_COMM_NUMBER; i++) {
			sysfs_remove_file(smi_comm[i].kobj_bus_smi, &port_attr.attr);
			sysfs_remove_file(smi_comm[i].kobj_bus_smi, &bustype_attr.attr);
			sysfs_remove_file(smi_comm[i].kobj_bus_smi, &desttype_attr.attr);
			sysfs_remove_file(smi_comm[i].kobj_bus_smi, &rwtype_attr.attr);
			kobject_del(smi_comm[i].kobj_bus_smi);
		}

		sysfs_remove_file(kobj_smi_mon_con, &requesttype_attr.attr);
		kobject_del(kobj_smi_mon_con);

		sysfs_remove_file(kobj_smi, &toggle_cnt_attr.attr);
		sysfs_remove_file(kobj_smi, &toggle_master_attr.attr);
		sysfs_remove_file(kobj_smi, &count_attr.attr);
		sysfs_remove_file(kobj_smi, &portnum_attr.attr);
		kobj_smi = NULL;
	}
}

static void met_smi_start(void)
{
	//return;
	if (do_smi()) {
        //printk("do smi\n");
		smi_init();
		smi_stop();
		smi_start();
	}
}

static void met_smi_stop(void)
{
	//return;
	if (do_smi()) {
		smi_stop();
		smi_uninit();
	}
}

static void met_smi_polling(unsigned long long stamp, int cpu)
{
	unsigned char count=0;
	unsigned int smi_value[100];  //Note here
	static int times=0;
	static int toggle_stop=0;

	//return;
	if (do_smi() == 1) { //single port polling
		count = smi_polling(smi_value);
		//printk("smi_polling result count=%d\n",count);
		if (count) {
			ms_smi(stamp, count, smi_value);
		}
	} else if (toggle_stop == 0) {
		if (do_smi() == 2) { //all-toggling
			//count = smi_toggle_polling(smi_value);
			count = smi_dump_polling(smi_value);
			if (count) {
				//printk("smi_polling result count=%d\n",count);
				ms_smit(stamp, count, smi_value);
			}
			if (times == toggle_cnt) {//switch port
				toggle_idx = (toggle_idx + 1) % (SMI_ALLPORT_COUNT*4);
				//toggle_idx = toggle_idx + 1;
				//if (toggle_idx < SMI_ALLPORT_COUNT*4) {
					toggle_port(toggle_idx);
				//}
				/* else {
					toggle_stop = 1;	//stop smi polling
					return;
				} */
				times = 0;
			} else {
				times++;
			}
		} else if (do_smi() == 3) { //per-master toggling
			//count = smi_toggle_polling(smi_value);
			count = smi_dump_polling(smi_value);
			if (count) {
				//printk("smi_polling result count=%d\n",count);
				ms_smit(stamp, count, smi_value);
			}
			if (times == toggle_cnt) {//switch port
				toggle_idx = toggle_idx + 1;
				if (toggle_idx > toggle_master_max) {
					toggle_idx = toggle_master_min;
				}
				toggle_port(toggle_idx);
				times = 0;
			} else {
				times++;
			}
		} else if (do_smi() == 4) { //toggle all and stop
			count = smi_dump_polling(smi_value);
			if (count) {
				ms_smit(stamp, count, smi_value);
			}
			if (times == toggle_cnt) {//switch port
				//toggle_idx = (toggle_idx + 1) % (SMI_ALLPORT_COUNT*4);
				toggle_idx = toggle_idx + 1;
				if (toggle_idx < SMI_ALLPORT_COUNT*4) {
					toggle_port(toggle_idx);
				} else {
					toggle_stop = 1;	//stop smi polling
					return;
				}
				times = 0;
			} else {
				times++;
			}
		}
	}
}

struct metdevice met_smi = {
	.name = "smi",
	.owner = THIS_MODULE,
	.type = MET_TYPE_BUS,
	.create_subfs = met_smi_create,
	.delete_subfs = met_smi_delete,
	.cpu_related = 0,
	.start = met_smi_start,
	.stop = met_smi_stop,
	.polling_interval = 0,
	.timed_polling = met_smi_polling,
	.tagged_polling = met_smi_polling
};



