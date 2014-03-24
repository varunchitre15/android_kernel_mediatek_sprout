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

#ifndef _WMT_DEV_H_
#define _WMT_DEV_H_


#include "osal.h"

extern VOID wmt_dev_rx_event_cb (VOID);
extern INT32 wmt_dev_rx_timeout (P_OSAL_EVENT pEvent);
extern INT32 wmt_dev_patch_get (UCHAR *pPatchName, osal_firmware **ppPatch,INT32 padSzBuf);
extern INT32 wmt_dev_patch_put(osal_firmware **ppPatch);
extern VOID wmt_dev_patch_info_free(VOID);


#endif /*_WMT_DEV_H_*/
