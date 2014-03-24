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

#define SII_HAL_LINUX_INIT_C
#include <linux/i2c.h>
#include "sii_hal.h"
#include "sii_hal_priv.h"
bool gHalInitedFlag = false;
DEFINE_SEMAPHORE(gIsrLock); 
mhlDeviceContext_t gMhlDevice;
halReturn_t HalInitCheck(void)
{
	if (!(gHalInitedFlag))
	{
		SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,"Error: Hal layer not currently initialize!\n");
		return HAL_RET_NOT_INITIALIZED;
	}
	return HAL_RET_SUCCESS;
}
halReturn_t HalInit(void)
{
	//halReturn_t	status;
	if (gHalInitedFlag)
	{
		SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,"Error: Hal layer already inited!\n");
		return HAL_RET_ALREADY_INITIALIZED;
	}
	gMhlDevice.driver.driver.name = NULL;
	gMhlDevice.driver.id_table = NULL;
	gMhlDevice.driver.probe = NULL;
	gMhlDevice.driver.remove = NULL;
	gMhlDevice.pI2cClient = NULL;
	gMhlDevice.irqHandler = NULL;
#ifdef RGB_BOARD
	gMhlDevice.ExtDeviceirqHandler = NULL;
#endif
	/*
	status = HalGpioInit();
	if(status != HAL_RET_SUCCESS)
	{
		return status;
	}
	*/
	gHalInitedFlag = true;
	return HAL_RET_SUCCESS;
}
halReturn_t HalTerm(void)
{
	halReturn_t		retStatus;
	retStatus = HalInitCheck();
	if (retStatus != HAL_RET_SUCCESS)
	{
		return retStatus;
	}
	//HalGpioTerm();
	gHalInitedFlag = false;
	return retStatus;
}
halReturn_t HalAcquireIsrLock()
{
	halReturn_t		retStatus;
	int				status;
	retStatus = HalInitCheck();
	if (retStatus != HAL_RET_SUCCESS)
	{
		return retStatus;
	}
	status = down_interruptible(&gIsrLock);
	if (status != 0)
	{
		SII_DEBUG_PRINT(SII_OSAL_DEBUG_TRACE,"HalAcquireIsrLock failed to acquire lock\n");
		return HAL_RET_FAILURE;
	}
	return HAL_RET_SUCCESS;
}
halReturn_t HalReleaseIsrLock()
{
	halReturn_t		retStatus;
	retStatus = HalInitCheck();
	if (retStatus != HAL_RET_SUCCESS)
	{
		return retStatus;
	}
	up(&gIsrLock);
	return retStatus;
}
