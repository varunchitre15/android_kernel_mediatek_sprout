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

#ifndef __M4U_H__
#define __M4U_H__
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <mach/m4u_port.h>
#include <linux/scatterlist.h>

typedef int M4U_PORT_ID_ENUM;
typedef int M4U_MODULE_ID_ENUM;

typedef enum
{
	RT_RANGE_HIGH_PRIORITY=0,
	SEQ_RANGE_LOW_PRIORITY=1
} M4U_RANGE_PRIORITY_ENUM;


// port related: virtuality, security, distance
typedef struct _M4U_PORT
{  
	M4U_PORT_ID_ENUM ePortID;		   //hardware port ID, defined in M4U_PORT_ID_ENUM
	unsigned int Virtuality;						   
	unsigned int Security;
    unsigned int domain;            //domain : 0 1 2 3
	unsigned int Distance;
	unsigned int Direction;         //0:- 1:+
}M4U_PORT_STRUCT;

typedef enum
{
	ROTATE_0=0,
	ROTATE_90,
	ROTATE_180,
	ROTATE_270,
	ROTATE_HFLIP_0,
	ROTATE_HFLIP_90,
	ROTATE_HFLIP_180,
	ROTATE_HFLIP_270
} M4U_ROTATOR_ENUM;

typedef struct _M4U_PORT_ROTATOR
{  
	M4U_PORT_ID_ENUM ePortID;		   // hardware port ID, defined in M4U_PORT_ID_ENUM
	unsigned int Virtuality;						   
	unsigned int Security;
	// unsigned int Distance;      // will be caculated actomatically inside M4U driver
	// unsigned int Direction;
  unsigned int MVAStart; 
  unsigned int BufAddr;
  unsigned int BufSize;  
  M4U_ROTATOR_ENUM angle;	
}M4U_PORT_STRUCT_ROTATOR;

typedef enum
{
    M4U_CACHE_FLUSH_BEFORE_HW_READ_MEM = 0,  // optimized, recommand to use
    M4U_CACHE_FLUSH_BEFORE_HW_WRITE_MEM = 1, // optimized, recommand to use
    M4U_CACHE_CLEAN_BEFORE_HW_READ_MEM = 2,
    M4U_CACHE_INVALID_AFTER_HW_WRITE_MEM = 3,
    M4U_NONE_OP = 4,
} M4U_CACHE_SYNC_ENUM;


// for kernel direct call --------------------------------------------
int m4u_dump_reg(int m4u_index);
int m4u_dump_info(int m4u_index);
int m4u_power_on(int m4u_index);
int m4u_power_off(int m4u_index);

int m4u_alloc_mva(M4U_MODULE_ID_ENUM eModuleID, 
                    const unsigned int BufAddr, 
                    const unsigned int BufSize, 
                    int security,
                    int cache_coherent,
                    unsigned int *pRetMVABuf);

int m4u_alloc_mva_sg(M4U_MODULE_ID_ENUM eModuleID, 
								  struct sg_table *sg_table, 
								  const unsigned int BufSize, 
								  int security,
								  int cache_coherent,
								  unsigned int *pRetMVABuf);


int m4u_dealloc_mva(M4U_MODULE_ID_ENUM eModuleID, 
                    const unsigned int BufAddr, 
                    const unsigned int BufSize,
                    const unsigned int MVA);	

int m4u_dealloc_mva_sg(M4U_MODULE_ID_ENUM eModuleID, 
									struct sg_table* sg_table,
									const unsigned int BufSize, 
									const unsigned int MVA);

int m4u_invalid_wrapped_range(M4U_MODULE_ID_ENUM eModuleID, 
                              M4U_PORT_ID_ENUM portID,
                              unsigned int MVAStart, 
                              unsigned int MVAEnd);
                                                                                             
int m4u_insert_seq_range(M4U_MODULE_ID_ENUM eModuleID, 
                             unsigned int MVAStart, 
                             unsigned int MVAEnd, 
                             M4U_RANGE_PRIORITY_ENUM ePriority,
                             unsigned int entryCount); //0:disable multi-entry, 1,2,4,8,16: enable multi-entry
                      
int m4u_invalid_seq_range(M4U_MODULE_ID_ENUM eModuleID,
                    unsigned int MVAStart,
                    unsigned int MVAEnd);  

int m4u_config_port(M4U_PORT_STRUCT* pM4uPort); //native
//int m4u_config_port_rotator(M4U_PORT_STRUCT_ROTATOR *pM4uPort);
int m4u_monitor_start(int m4u_id);
int m4u_monitor_stop(int m4u_id);


int m4u_dma_cache_maint(M4U_MODULE_ID_ENUM eModuleID,
    const void *va,
    size_t size, 
    int direction);

int m4u_mau_check_pagetable(unsigned int start_addr, unsigned int end_addr);
int m4u_mau_get_physical_port(unsigned int* engineMask);
								  
int m4u_log_on(void);
int m4u_log_off(void);
int m4u_debug_command(unsigned int command);
int m4u_mva_map_kernel(unsigned int mva, unsigned int size, int sec,
                        unsigned int* map_va, unsigned int* map_size);
int m4u_mva_unmap_kernel(unsigned int mva, unsigned int size, unsigned int va);
int m4u_fill_linear_pagetable(unsigned int pa, unsigned int size);
// m4u driver internal use ---------------------------------------------------
//

#endif

