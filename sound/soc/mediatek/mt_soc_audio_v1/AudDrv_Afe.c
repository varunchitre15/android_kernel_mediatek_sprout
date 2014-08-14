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
 *   AudioAfe.h
 *
 * Project:
 * --------
 *   MT6583  Audio Driver Afe Register setting
 *
 * Description:
 * ------------
 *   Audio register
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

#include "AudDrv_Common.h"
#include "AudDrv_Afe.h"
#include "AudDrv_Clk.h"
#include "AudDrv_Def.h"

/*****************************************************************************
 *                         D A T A   T Y P E S
 *****************************************************************************/


/*****************************************************************************
 *                         FUNCTION IMPLEMENTATION
 *****************************************************************************/


/*****************************************************************************
 * FUNCTION
 *  Auddrv_Reg_map
 *
 * DESCRIPTION
 * Auddrv_Reg_map
 *
 *****************************************************************************
 */

/*
 *    global variable control
 */

/* address for ioremap audio hardware register */
void *AFE_BASE_ADDRESS = 0;
void *AFE_SRAM_ADDRESS = 0;
void *AFE_TOP_ADDRESS = 0;
void *AFE_CLK_ADDRESS = 0;

void Auddrv_Reg_map(void)
{
	AFE_SRAM_ADDRESS = ioremap_nocache(AFE_INTERNAL_SRAM_PHY_BASE, AFE_INTERNAL_SRAM_SIZE);
	AFE_BASE_ADDRESS = ioremap_nocache(AUDIO_HW_PHYSICAL_BASE, 0x6000);

	/* temp for hardawre code  set 0x1000629c = 0xd */
	AFE_TOP_ADDRESS = ioremap_nocache(AUDIO_POWER_TOP, 0x1000);

	/* temp for hardawre code  set clg cfg */
	AFE_CLK_ADDRESS = ioremap_nocache(AUDIO_CLKCFG_PHYSICAL_BASE, 0x1000);
}

void *Get_Afe_Base_Pointer()
{
	return AFE_BASE_ADDRESS;
}

void *Get_Afe_SramBase_Pointer()
{
	return AFE_SRAM_ADDRESS;
}

void *Get_Afe_Powertop_Pointer()
{
	return AFE_TOP_ADDRESS;
}

void *Get_AudClk_Pointer()
{
	return AFE_CLK_ADDRESS;
}

void Afe_Set_Reg(uint32 offset, uint32 value, uint32 mask)
{
#ifdef AUDIO_MEM_IOREMAP
	extern void *AFE_BASE_ADDRESS;
	/* PRINTK_AUDDRV("Afe_Set_Reg AUDIO_MEM_IOREMAP AFE_BASE_ADDRESS = %p\n",AFE_BASE_ADDRESS); */
	volatile uint32 address = (uint32) ((char *)AFE_BASE_ADDRESS + offset);
#else
	volatile uint32 address = (AFE_BASE + offset);
#endif

	volatile uint32 *AFE_Register = (volatile uint32 *)address;
	volatile uint32 val_tmp;

	/* PRINTK_AFE_REG("Afe_Set_Reg offset=%x, value=%x, mask=%x\n",offset,value,mask); */
	val_tmp = Afe_Get_Reg(offset);
	val_tmp &= (~mask);
	val_tmp |= (value & mask);
	mt_reg_sync_writel(val_tmp, AFE_Register);
}

uint32 Afe_Get_Reg(uint32 offset)
{
#ifdef AUDIO_MEM_IOREMAP
	extern void *AFE_BASE_ADDRESS;
	/* PRINTK_AUDDRV("Afe_Get_Reg AUDIO_MEM_IOREMAP AFE_BASE_ADDRESS = %p\ offset = %xn",AFE_BASE_ADDRESS,offset); */
	volatile uint32 address = (uint32) ((char *)AFE_BASE_ADDRESS + offset);
#else
	volatile uint32 address = (AFE_BASE + offset);
#endif
	volatile uint32 *value;
	value = (volatile uint32 *)(address);
	/* PRINTK_AFE_REG("Afe_Get_Reg offset=%x address = %x value = 0x%x\n",offset,address,*value); */
	return *value;
}

/* function to Set Cfg */
uint32 GetClkCfg(uint32 offset)
{
	volatile uint32 address = (uint32) ((char *)AFE_CLK_ADDRESS + offset);
	volatile uint32 *value;
	value = (volatile uint32 *)(address);
	pr_debug("GetClkCfg offset=%x address = %x value = 0x%x\n", offset, address, *value);
	return *value;
}

void SetClkCfg(uint32 offset, uint32 value, uint32 mask)
{
	volatile uint32 address = (uint32) ((char *)AFE_CLK_ADDRESS + offset);
	volatile uint32 *AFE_Register = (volatile uint32 *)address;
	volatile uint32 val_tmp;
	pr_debug("SetClkCfg offset=%x, value=%x, mask=%x\n", offset, value, mask);
	val_tmp = GetClkCfg(offset);
	val_tmp &= (~mask);
	val_tmp |= (value & mask);
	mt_reg_sync_writel(val_tmp, AFE_Register);
}

/* function to Set pll */
uint32 GetpllCfg(uint32 offset)
{
	volatile uint32 address = (APLL_BASE + offset);
	volatile uint32 *value;
	value = (volatile uint32 *)(address);
	pr_debug("GetClkCfg offset=%x address = %x value = 0x%x\n", offset, address, *value);
	return *value;
}

void SetpllCfg(uint32 offset, uint32 value, uint32 mask)
{
	volatile uint32 address = (APLL_BASE + offset);
	volatile uint32 *AFE_Register = (volatile uint32 *)address;
	volatile uint32 val_tmp;
	pr_debug("SetpllCfg offset=%x, value=%x, mask=%x\n", offset, value, mask);
	val_tmp = GetClkCfg(offset);
	val_tmp &= (~mask);
	val_tmp |= (value & mask);
	mt_reg_sync_writel(val_tmp, AFE_Register);
}

void Afe_Log_Print(void)
{
	AudDrv_Clk_On();
	pr_debug("+AudDrv Afe_Log_Print\n");
	pr_debug("AUDIOAFE_TOP_CON0  = 0x%x\n", Afe_Get_Reg(AUDIO_TOP_CON0));
	pr_debug("AUDIO_TOP_CON1  = 0x%x\n", Afe_Get_Reg(AUDIO_TOP_CON1));
	pr_debug("AUDIO_TOP_CON2  = 0x%x\n", Afe_Get_Reg(AUDIO_TOP_CON2));
	pr_debug("AUDIO_TOP_CON3  = 0x%x\n", Afe_Get_Reg(AUDIO_TOP_CON3));
	pr_debug("AFE_DAC_CON0  = 0x%x\n", Afe_Get_Reg(AFE_DAC_CON0));
	pr_debug("AFE_DAC_CON1  = 0x%x\n", Afe_Get_Reg(AFE_DAC_CON1));
	pr_debug("AFE_I2S_CON  = 0x%x\n", Afe_Get_Reg(AFE_I2S_CON));
	pr_debug("AFE_DAIBT_CON0  = 0x%x\n", Afe_Get_Reg(AFE_DAIBT_CON0));

	pr_debug("AFE_CONN0  = 0x%x\n", Afe_Get_Reg(AFE_CONN0));
	pr_debug("AFE_CONN1  = 0x%x\n", Afe_Get_Reg(AFE_CONN1));
	pr_debug("AFE_CONN2  = 0x%x\n", Afe_Get_Reg(AFE_CONN2));
	pr_debug("AFE_CONN3  = 0x%x\n", Afe_Get_Reg(AFE_CONN3));
	pr_debug("AFE_CONN4  = 0x%x\n", Afe_Get_Reg(AFE_CONN4));
	pr_debug("AFE_I2S_CON1  = 0x%x\n", Afe_Get_Reg(AFE_I2S_CON1));
	pr_debug("AFE_I2S_CON2  = 0x%x\n", Afe_Get_Reg(AFE_I2S_CON2));
	pr_debug("AFE_MRGIF_CON  = 0x%x\n", Afe_Get_Reg(AFE_MRGIF_CON));

	pr_debug("AFE_DL1_BASE  = 0x%x\n", Afe_Get_Reg(AFE_DL1_BASE));
	pr_debug("AFE_DL1_CUR  = 0x%x\n", Afe_Get_Reg(AFE_DL1_CUR));
	pr_debug("AFE_DL1_END  = 0x%x\n", Afe_Get_Reg(AFE_DL1_END));
	pr_debug("AFE_I2S_CON3  = 0x%x\n", Afe_Get_Reg(AFE_I2S_CON3));

	pr_debug("AFE_DL2_BASE  = 0x%x\n", Afe_Get_Reg(AFE_DL2_BASE));
	pr_debug("AFE_DL2_CUR  = 0x%x\n", Afe_Get_Reg(AFE_DL2_CUR));
	pr_debug("AFE_DL2_END  = 0x%x\n", Afe_Get_Reg(AFE_DL2_END));
	pr_debug("AFE_CONN5  = 0x%x\n", Afe_Get_Reg(AFE_CONN5));
	pr_debug("AFE_CONN_24BIT  = 0x%x\n", Afe_Get_Reg(AFE_CONN_24BIT));
	pr_debug("AFE_AWB_BASE  = 0x%x\n", Afe_Get_Reg(AFE_AWB_BASE));
	pr_debug("AFE_AWB_END  = 0x%x\n", Afe_Get_Reg(AFE_AWB_END));
	pr_debug("AFE_AWB_CUR  = 0x%x\n", Afe_Get_Reg(AFE_AWB_CUR));
	pr_debug("AFE_VUL_BASE  = 0x%x\n", Afe_Get_Reg(AFE_VUL_BASE));
	pr_debug("AFE_VUL_END  = 0x%x\n", Afe_Get_Reg(AFE_VUL_END));
	pr_debug("AFE_VUL_CUR  = 0x%x\n", Afe_Get_Reg(AFE_VUL_CUR));
	pr_debug("AFE_CONN6  = 0x%x\n", Afe_Get_Reg(AFE_CONN6));
	pr_debug("AFE_DAI_BASE  = 0x%x\n", Afe_Get_Reg(AFE_DAI_BASE));
	pr_debug("AFE_DAI_END  = 0x%x\n", Afe_Get_Reg(AFE_DAI_END));
	pr_debug("AFE_DAI_CUR  = 0x%x\n", Afe_Get_Reg(AFE_DAI_CUR));
	pr_debug("AFE_MEMIF_MSB  = 0x%x\n", Afe_Get_Reg(AFE_MEMIF_MSB));

	pr_debug("AFE_MEMIF_MON0  = 0x%x\n", Afe_Get_Reg(AFE_MEMIF_MON0));
	pr_debug("AFE_MEMIF_MON1  = 0x%x\n", Afe_Get_Reg(AFE_MEMIF_MON1));
	pr_debug("AFE_MEMIF_MON2  = 0x%x\n", Afe_Get_Reg(AFE_MEMIF_MON2));
	/* pr_debug("AFE_MEMIF_MON3  = 0x%x\n",Afe_Get_Reg(AFE_MEMIF_MON3)); */
	pr_debug("AFE_MEMIF_MON4  = 0x%x\n", Afe_Get_Reg(AFE_MEMIF_MON4));

	pr_debug("AFE_ADDA_DL_SRC2_CON0  = 0x%x\n", Afe_Get_Reg(AFE_ADDA_DL_SRC2_CON0));
	pr_debug("AFE_ADDA_DL_SRC2_CON1  = 0x%x\n", Afe_Get_Reg(AFE_ADDA_DL_SRC2_CON1));
	pr_debug("AFE_ADDA_UL_SRC_CON0  = 0x%x\n", Afe_Get_Reg(AFE_ADDA_UL_SRC_CON0));
	pr_debug("AFE_ADDA_UL_SRC_CON1  = 0x%x\n", Afe_Get_Reg(AFE_ADDA_UL_SRC_CON1));
	pr_debug("AFE_ADDA_TOP_CON0  = 0x%x\n", Afe_Get_Reg(AFE_ADDA_TOP_CON0));
	pr_debug("AFE_ADDA_UL_DL_CON0  = 0x%x\n", Afe_Get_Reg(AFE_ADDA_UL_DL_CON0));
	pr_debug("AFE_ADDA_SRC_DEBUG  = 0x%x\n", Afe_Get_Reg(AFE_ADDA_SRC_DEBUG));
	pr_debug("AFE_ADDA_SRC_DEBUG_MON0  = 0x%x\n", Afe_Get_Reg(AFE_ADDA_SRC_DEBUG_MON0));
	pr_debug("AFE_ADDA_SRC_DEBUG_MON1  = 0x%x\n", Afe_Get_Reg(AFE_ADDA_SRC_DEBUG_MON1));
	pr_debug("AFE_ADDA_NEWIF_CFG0  = 0x%x\n", Afe_Get_Reg(AFE_ADDA_NEWIF_CFG0));
	pr_debug("AFE_ADDA_NEWIF_CFG1  = 0x%x\n", Afe_Get_Reg(AFE_ADDA_NEWIF_CFG1));

	pr_debug("AFE_SIDETONE_DEBUG  = 0x%x\n", Afe_Get_Reg(AFE_SIDETONE_DEBUG));
	pr_debug("AFE_SIDETONE_MON  = 0x%x\n", Afe_Get_Reg(AFE_SIDETONE_MON));
	pr_debug("AFE_SIDETONE_CON0  = 0x%x\n", Afe_Get_Reg(AFE_SIDETONE_CON0));
	pr_debug("AFE_SIDETONE_COEFF  = 0x%x\n", Afe_Get_Reg(AFE_SIDETONE_COEFF));
	pr_debug("AFE_SIDETONE_CON1  = 0x%x\n", Afe_Get_Reg(AFE_SIDETONE_CON1));
	pr_debug("AFE_SIDETONE_GAIN  = 0x%x\n", Afe_Get_Reg(AFE_SIDETONE_GAIN));
	pr_debug("AFE_SGEN_CON0  = 0x%x\n", Afe_Get_Reg(AFE_SGEN_CON0));
	pr_debug("AFE_SGEN_CON0  = 0x%x\n", Afe_Get_Reg(AFE_SGEN_CON1));
	pr_debug("AFE_TOP_CON0  = 0x%x\n", Afe_Get_Reg(AFE_TOP_CON0));

	pr_debug("AFE_ADDA_PREDIS_CON0  = 0x%x\n", Afe_Get_Reg(AFE_ADDA_PREDIS_CON0));
	pr_debug("AFE_ADDA_PREDIS_CON1  = 0x%x\n", Afe_Get_Reg(AFE_ADDA_PREDIS_CON1));

	pr_debug("AFE_MRG_MON0  = 0x%x\n", Afe_Get_Reg(AFE_MRGIF_MON0));
	pr_debug("AFE_MRG_MON1  = 0x%x\n", Afe_Get_Reg(AFE_MRGIF_MON1));
	pr_debug("AFE_MRG_MON2  = 0x%x\n", Afe_Get_Reg(AFE_MRGIF_MON2));

	pr_debug("AFE_MOD_DAI_BASE  = 0x%x\n", Afe_Get_Reg(AFE_MOD_DAI_BASE));
	pr_debug("AFE_MOD_DAI_END  = 0x%x\n", Afe_Get_Reg(AFE_MOD_DAI_END));
	pr_debug("AFE_MOD_DAI_CUR  = 0x%x\n", Afe_Get_Reg(AFE_MOD_DAI_CUR));

	pr_debug("AFE_HDMI_OUT_CON0  = 0x%x\n", Afe_Get_Reg(AFE_HDMI_OUT_CON0));
	pr_debug("AFE_HDMI_BASE  = 0x%x\n", Afe_Get_Reg(AFE_HDMI_BASE));
	pr_debug("AFE_HDMI_CUR  = 0x%x\n", Afe_Get_Reg(AFE_HDMI_CUR));
	pr_debug("AFE_HDMI_END  = 0x%x\n", Afe_Get_Reg(AFE_HDMI_END));
	pr_debug("AFE_HDMI_CONN0  = 0x%x\n", Afe_Get_Reg(AFE_HDMI_CONN0));
	/* pr_debug("AFE_IRQ_CON  = 0x%x\n",Afe_Get_Reg(AFE_IRQ_CON)); */
	pr_debug("AFE_IRQ_MCU_CON  = 0x%x\n", Afe_Get_Reg(AFE_IRQ_MCU_CON));
	pr_debug("AFE_IRQ_STATUS  = 0x%x\n", Afe_Get_Reg(AFE_IRQ_MCU_STATUS));
	pr_debug("AFE_IRQ_CLR  = 0x%x\n", Afe_Get_Reg(AFE_IRQ_MCU_CLR));
	pr_debug("AFE_IRQ_MCU_CNT1  = 0x%x\n", Afe_Get_Reg(AFE_IRQ_MCU_CNT1));
	pr_debug("AFE_IRQ_MCU_CNT2  = 0x%x\n", Afe_Get_Reg(AFE_IRQ_MCU_CNT2));
	pr_debug("AFE_IRQ_MCU_EN  = 0x%x\n", Afe_Get_Reg(AFE_IRQ_MCU_EN));
	pr_debug("AFE_IRQ_MON2  = 0x%x\n", Afe_Get_Reg(AFE_IRQ_MCU_MON2));
	/* pr_debug("AFE_IRQ_CNT5  = 0x%x\n",Afe_Get_Reg(AFE_IRQ_CNT5));MT6582 */
	pr_debug("AFE_IRQ1_CNT_MON  = 0x%x\n", Afe_Get_Reg(AFE_IRQ1_MCU_CNT_MON));
	pr_debug("AFE_IRQ2_CNT_MON  = 0x%x\n", Afe_Get_Reg(AFE_IRQ2_MCU_CNT_MON));
	pr_debug("AFE_IRQ1_EN_CNT_MON  = 0x%x\n", Afe_Get_Reg(AFE_IRQ1_MCU_EN_CNT_MON));
	/* pr_debug("AFE_IRQ5_MCU_EN_CNT_MON  = 0x%x\n",Afe_Get_Reg(AFE_IRQ5_MCU_EN_CNT_MON)); */
	pr_debug("AFE_MEMIF_MAXLEN  = 0x%x\n", Afe_Get_Reg(AFE_MEMIF_MAXLEN));
	pr_debug("AFE_MEMIF_PBUF_SIZE  = 0x%x\n", Afe_Get_Reg(AFE_MEMIF_PBUF_SIZE));

	pr_debug("AFE_GAIN1_CON0  = 0x%x\n", Afe_Get_Reg(AFE_GAIN1_CON0));
	pr_debug("AFE_GAIN1_CON1  = 0x%x\n", Afe_Get_Reg(AFE_GAIN1_CON1));
	pr_debug("AFE_GAIN1_CON2  = 0x%x\n", Afe_Get_Reg(AFE_GAIN1_CON2));
	pr_debug("AFE_GAIN1_CON3  = 0x%x\n", Afe_Get_Reg(AFE_GAIN1_CON3));
	pr_debug("AFE_GAIN1_CONN  = 0x%x\n", Afe_Get_Reg(AFE_GAIN1_CONN));
	pr_debug("AFE_GAIN1_CUR  = 0x%x\n", Afe_Get_Reg(AFE_GAIN1_CUR));
	pr_debug("AFE_GAIN2_CON0  = 0x%x\n", Afe_Get_Reg(AFE_GAIN2_CON0));
	pr_debug("AFE_GAIN2_CON1  = 0x%x\n", Afe_Get_Reg(AFE_GAIN2_CON1));
	pr_debug("AFE_GAIN2_CON2  = 0x%x\n", Afe_Get_Reg(AFE_GAIN2_CON2));
	pr_debug("AFE_GAIN2_CON3  = 0x%x\n", Afe_Get_Reg(AFE_GAIN2_CON3));
	pr_debug("AFE_GAIN2_CONN  = 0x%x\n", Afe_Get_Reg(AFE_GAIN2_CONN));
	pr_debug("AFE_GAIN2_CUR  = 0x%x\n", Afe_Get_Reg(AFE_GAIN2_CUR));
	pr_debug("AFE_GAIN2_CONN2  = 0x%x\n", Afe_Get_Reg(AFE_GAIN2_CONN2));


	pr_debug("FPGA_CFG2  = 0x%x\n", Afe_Get_Reg(FPGA_CFG2));
	pr_debug("FPGA_CFG3  = 0x%x\n", Afe_Get_Reg(FPGA_CFG3));
	pr_debug("FPGA_CFG0  = 0x%x\n", Afe_Get_Reg(FPGA_CFG0));
	pr_debug("FPGA_CFG1  = 0x%x\n", Afe_Get_Reg(FPGA_CFG1));
	pr_debug("FPGA_STC  = 0x%x\n", Afe_Get_Reg(FPGA_STC));

	pr_debug("AFE_ASRC_CON0  = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON0));
	pr_debug("AFE_ASRC_CON1  = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON1));
	pr_debug("AFE_ASRC_CON2  = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON2));
	pr_debug("AFE_ASRC_CON3  = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON3));
	pr_debug("AFE_ASRC_CON4  = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON4));
	pr_debug("AFE_ASRC_CON5  = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON5));
	pr_debug("AFE_ASRC_CON6  = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON6));
	pr_debug("AFE_ASRC_CON7  = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON7));
	pr_debug("AFE_ASRC_CON8  = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON8));
	pr_debug("AFE_ASRC_CON9  = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON9));
	pr_debug("AFE_ASRC_CON10  = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON10));
	pr_debug("AFE_ASRC_CON11  = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON11));
	pr_debug("PCM_INTF_CON1  = 0x%x\n", Afe_Get_Reg(PCM_INTF_CON));
	pr_debug("PCM_INTF_CON2  = 0x%x\n", Afe_Get_Reg(PCM_INTF_CON2));
	pr_debug("PCM2_INTF_CON  = 0x%x\n", Afe_Get_Reg(PCM2_INTF_CON));

	pr_debug("AFE_ASRC_CON13  = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON13));
	pr_debug("AFE_ASRC_CON14  = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON14));
	pr_debug("AFE_ASRC_CON15  = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON15));
	pr_debug("AFE_ASRC_CON16  = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON16));
	pr_debug("AFE_ASRC_CON17  = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON17));
	pr_debug("AFE_ASRC_CON18  = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON18));
	pr_debug("AFE_ASRC_CON19  = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON19));
	pr_debug("AFE_ASRC_CON20  = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON20));
	pr_debug("AFE_ASRC_CON21  = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON21));

	AudDrv_Clk_Off();
	pr_debug("-AudDrv Afe_Log_Print\n");
}



/* export symbols for other module using */
EXPORT_SYMBOL(Afe_Set_Reg);
EXPORT_SYMBOL(Afe_Get_Reg);

EXPORT_SYMBOL(GetClkCfg);
EXPORT_SYMBOL(SetClkCfg);

EXPORT_SYMBOL(Afe_Log_Print);
