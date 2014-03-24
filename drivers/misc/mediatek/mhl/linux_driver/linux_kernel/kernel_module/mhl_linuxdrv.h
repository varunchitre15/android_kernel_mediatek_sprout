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

#if !defined(MHL_DRIVER_H)
#define MHL_DRIVER_H
#include "sii_hal.h"
#ifdef __cplusplus 
extern "C" { 
#endif  
#if defined(MAKE_8338_DRIVER)
#define MHL_PART_NAME   "Sil_MHL"
#define MHL_DRIVER_NAME "sii8338drv"
#define MHL_DRIVER_DESC "SiliconImage CP8338 Tx Driver"
#define MHL_DEVICE_NAME "Sii-Iceman8338"
#else   
#error "Need to add name and description strings for new drivers here!"
#endif
#define MHL_DRIVER_MINOR_MAX   1
#define EVENT_POLL_INTERVAL_MS	50
#define MHL_STATE_FLAG_CONNECTED	0x01	
#define MHL_STATE_FLAG_RCP_READY	0x02	
#define MHL_STATE_FLAG_RCP_SENT		0x04	
#define MHL_STATE_FLAG_RCP_RECEIVED	0x08	
#define MHL_STATE_FLAG_RCP_ACK		0x10	
#define MHL_STATE_FLAG_RCP_NAK		0x20	
typedef struct {
	struct device		*pDevice;			
	uint8_t				flags;				
	uint8_t				keyCode;			
	uint8_t				errCode;			
	uint8_t				devCapOffset;		
} MHL_DRIVER_CONTEXT_T, *PMHL_DRIVER_CONTEXT_T;
extern MHL_DRIVER_CONTEXT_T gDriverContext;
int32_t SiiMhlOpen(struct inode *pInode, struct file *pFile);
int32_t SiiMhlRelease(struct inode *pInode, struct file *pFile);
long SiiMhlIoctl(struct file *pFile, unsigned int ioctlCode,
					unsigned long ioctlParam);
#ifdef __cplusplus
}
#endif  
#endif 
