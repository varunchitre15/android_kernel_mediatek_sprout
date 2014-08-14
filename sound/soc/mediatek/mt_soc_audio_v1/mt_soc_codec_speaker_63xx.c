/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*******************************************************************************
 *
 * Filename:
 * ---------
 *   mtk_codec_speaker_63xx
 *
 * Project:
 * --------
 *
 *
 * Description:
 * ------------
 *   Audio codec stub file
 *
 * Author:
 * -------
 * Chipeng Chang
 *
 *------------------------------------------------------------------------------
 * $Revision: #1 $
 * $Modtime:$
 * $Log:$
 *
 *
 *******************************************************************************/


/*****************************************************************************
 *                     C O M P I L E R   F L A G S
 *****************************************************************************/


/*****************************************************************************
 *                E X T E R N A L   R E F E R E N C E S
 *****************************************************************************/

#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>

#include "AudDrv_Common.h"
#include "AudDrv_Def.h"
#include "AudDrv_Afe.h"
#include "AudDrv_Ana.h"
#include "AudDrv_Clk.h"
#include "mt_soc_analog_type.h"
#include <mach/pmic_mt6331_6332_sw.h>

static DEFINE_MUTEX(Speaker_Ctrl_Mutex);
static DEFINE_SPINLOCK(Speaker_lock);

static int Speaker_Counter;

extern kal_uint32 mt6332_upmu_get_swcid(void);

static void BoostOpen(void)
{
	Ana_Set_Reg(0x809a, 0x6025, 0x002a);	/* clk enable */
	Ana_Set_Reg(0x8554, 0x0012, 0xffff);	/* VSBST_CON21 */
	Ana_Set_Reg(0x853a, 0x0000, 0xffff);	/* VSBST_CON8  , 1 enable */
	Ana_Set_Reg(0x8536, 0x0000, 0xffff);	/* VSBST_CON6 */
	Ana_Set_Reg(0x8532, 0x01c0, 0xffff);	/* VSBST_CON4 */

	Ana_Set_Reg(0x854e, 0x0008, 0xffff);	/* VSBST_CON18 */
	Ana_Set_Reg(0x853e, 0x0021, 0xffff);	/* VSBST_CON10 , 1 enable */
	/* wiat for boost ready */
	msleep(30);
	Ana_Set_Reg(0x854e, 0x0000, 0xffff);	/* VSBST_CON18 */
	Ana_Set_Reg(0x8538, 0x0000, 0xffff);	/* VSBST_CON7 */
	Ana_Set_Reg(0x8534, 0x0300, 0xffff);	/* VSBST_CON5 */
	Ana_Set_Reg(0x8542, 0x003a, 0xffff);	/* VSBST_CON12 */
}

static void BoostClose(void)
{
	Ana_Set_Reg(0x853e, 0x0000, 0xffff);	/* VSBST_CON10 , 1 enable */
	Ana_Set_Reg(0x809a, 0x002a, 0x002a);	/* clk enable */
}


static void Enable_Speaker_Clk(bool benable)
{
	if (benable) {
		Speaker_Counter++;
		Ana_Set_Reg(SPK_TOP_CKPDN_CON0, 0x0000, 0x0007);
		if (mt6332_upmu_get_swcid() == PMIC6332_E1_CID_CODE) {
			Ana_Set_Reg(0x8552, 0x0005, 0xffff);
		}
	} else {
		Speaker_Counter--;
		Ana_Set_Reg(SPK_TOP_CKPDN_CON0, 0x0007, 0x0007);
	}
}

void Speaker_ClassD_Open(void)
{
	pr_debug("%s\n", __func__);
	BoostOpen();
	Enable_Speaker_Clk(true);
	Ana_Set_Reg(SPK_CON2, 0x0404, 0xffff);
	Ana_Set_Reg(SPK_CON9, 0x0a00, 0xffff);
	Ana_Set_Reg(SPK_CON13, 0x1900, 0xffff);

	Ana_Set_Reg(SPK_CON0, 0x3000, 0xffff);
	Ana_Set_Reg(SPK_CON0, 0x3408, 0xffff);
	Ana_Set_Reg(SPK_CON0, 0x3409, 0xffff);
	msleep(20);
	Ana_Set_Reg(SPK_CON13, 0x0100, 0xff00);
	Ana_Set_Reg(SPK_CON0, 0x3000, 0xffff);
	Ana_Set_Reg(SPK_CON0, 0x3001, 0xffff);

}

void Speaker_ClassD_close(void)
{
	pr_debug("%s\n", __func__);
	Ana_Set_Reg(SPK_CON0, 0x0000, 0xffff);
	BoostClose();
	Enable_Speaker_Clk(false);
}

void Speaker_ClassAB_Open(void)
{
	pr_debug("%s\n", __func__);
	BoostOpen();
	Enable_Speaker_Clk(true);
	Ana_Set_Reg(SPK_CON2, 0x0204, 0xffff);
	Ana_Set_Reg(SPK_CON9, 0x0a00, 0xffff);
	Ana_Set_Reg(SPK_CON13, 0x3B00, 0xffff);

	Ana_Set_Reg(SPK_CON0, 0x3000, 0xffff);
	Ana_Set_Reg(SPK_CON0, 0x3408, 0xffff);
	Ana_Set_Reg(SPK_CON0, 0x3409, 0xffff);
	msleep(20);
	Ana_Set_Reg(SPK_CON13, 0x2300, 0xff00);
	Ana_Set_Reg(SPK_CON0, 0x3000, 0xffff);
	Ana_Set_Reg(SPK_CON0, 0x3005, 0xffff);
}

void Speaker_ClassAB_close(void)
{
	pr_debug("%s\n", __func__);
	Ana_Set_Reg(SPK_CON0, 0x0000, 0xffff);
	BoostClose();
	Enable_Speaker_Clk(false);
}

void Speaker_ReveiverMode_Open(void)
{
	pr_debug("%s\n", __func__);
	BoostOpen();
	Enable_Speaker_Clk(true);
	Ana_Set_Reg(SPK_CON2, 0x0204, 0xffff);
	Ana_Set_Reg(SPK_CON9, 0x0a00, 0xffff);
	Ana_Set_Reg(SPK_CON13, 0x1B00, 0xffff);

	Ana_Set_Reg(SPK_CON0, 0x1000, 0xffff);
	Ana_Set_Reg(SPK_CON0, 0x1408, 0xffff);
	Ana_Set_Reg(SPK_CON0, 0x1409, 0xffff);
	msleep(20);
	Ana_Set_Reg(SPK_CON13, 0x0300, 0xff00);
	Ana_Set_Reg(SPK_CON0, 0x1000, 0xffff);
	Ana_Set_Reg(SPK_CON0, 0x1005, 0xffff);
}

void Speaker_ReveiverMode_close(void)
{
	pr_debug("%s\n", __func__);
	Ana_Set_Reg(SPK_CON0, 0x0000, 0xffff);
	BoostClose();
	Enable_Speaker_Clk(false);
}

bool GetSpeakerOcFlag(void)
{
	unsigned int OCregister = 0;
	unsigned int bitmask = 1;
	bool DmodeFlag = false;
	bool ABmodeFlag = false;
	Ana_Set_Reg(0x80D4, 0x0000, 0xffff);
	OCregister = Ana_Get_Reg(0x8CFE);
	DmodeFlag = OCregister & (bitmask << 14);	/* ; no.14 bit is SPK_D_OC_L_DEG */
	ABmodeFlag = OCregister & (bitmask << 15);	/* ; no.15 bit is SPK_AB_OC_L_DEG */
	pr_debug("OCregister = %d\n", OCregister);
	return (DmodeFlag | ABmodeFlag);
}
