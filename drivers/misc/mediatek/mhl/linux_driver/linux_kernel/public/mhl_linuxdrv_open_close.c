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

#define MHL_LINUXDRV_MAIN_C
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include "mhl_linuxdrv.h"
#include "osal/include/osal.h"
#include "si_mhl_tx_api.h"
static bool	bTxOpen = false;	
int32_t SiiMhlOpen(struct inode *pInode, struct file *pFile)
{
	if(bTxOpen) {
		SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,
				"Driver already open, failing open request\n");
		return -EBUSY;
	}
	bTxOpen = true;		
    return 0;
}
int32_t SiiMhlRelease(struct inode *pInode, struct file *pFile)
{
	SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE, "Close %s\n", MHL_DRIVER_NAME);
	bTxOpen = false;	
	return 0;
}
