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
 *   mtk_soc_codec_63xx
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
#include <mach/mt_gpio.h>

#include "AudDrv_Common.h"
#include "AudDrv_Def.h"
#include "AudDrv_Afe.h"
#include "AudDrv_Ana.h"
#include "AudDrv_Clk.h"
#include "mt_soc_analog_type.h"
#include <mach/mt_clkbuf_ctl.h>
#include <sound/mt_soc_audio.h>
//#include <mach/vow_api.h> //temp mark for early porting
#ifdef CONFIG_MTK_SPEAKER
#include "mt_soc_codec_speaker_63xx.h"
#endif

#include "mt_soc_pcm_common.h"

#if 0 //temp mark for early porting
extern void mt6331_upmu_set_rg_audmicbias1lowpen(kal_uint32 val);
extern void mt6331_upmu_set_rg_audmicbias1dcswnen(kal_uint32 val);
extern void mt6331_upmu_set_rg_audmicbias1dcswpen(kal_uint32 val);
extern void mt6331_upmu_set_rg_audpwdbmicbias1(kal_uint32 val);

extern void mt6331_upmu_set_rg_audmicbias0lowpen(kal_uint32 val);
extern void mt6331_upmu_set_rg_audmicbias0dcswnen(kal_uint32 val);
extern void mt6331_upmu_set_rg_audmicbias0dcswpen(kal_uint32 val);
extern void mt6331_upmu_set_rg_audpwdbmicbias0(kal_uint32 val);
#else
void mt6331_upmu_set_rg_audmicbias1lowpen(kal_uint32 val)
{

}
void mt6331_upmu_set_rg_audmicbias1dcswnen(kal_uint32 val)
{

}
void mt6331_upmu_set_rg_audmicbias1dcswpen(kal_uint32 val)
{

}
void mt6331_upmu_set_rg_audpwdbmicbias1(kal_uint32 val)
{

}
void mt6331_upmu_set_rg_audmicbias0lowpen(kal_uint32 val)
{

}
void mt6331_upmu_set_rg_audmicbias0dcswnen(kal_uint32 val)
{

}
void mt6331_upmu_set_rg_audmicbias0dcswpen(kal_uint32 val)
{

}
void mt6331_upmu_set_rg_audpwdbmicbias0(kal_uint32 val)
{

}

#endif


// static function declaration
static void HeadsetRVolumeSet(void);
static void HeadsetLVolumeSet(void);
static bool AudioPreAmp1_Sel(int Mul_Sel);
static bool GetAdcStatus(void);
static void Apply_Speaker_Gain(void);
static bool TurnOnVOWDigitalHW(bool enable);
static void TurnOffDacPower(void);
static void TurnOnDacPower(void);
static void OpenClassH(void);

static bool TurnOnVOWADcPowerACC(int MicType, bool enable);


extern int PMIC_IMM_GetOneChannelValue(int dwChannel, int deCount, int trimd);

static mt6331_Codec_Data_Priv *mCodec_data = NULL;
static uint32 mBlockSampleRate[AUDIO_ANALOG_DEVICE_INOUT_MAX] = {48000, 48000, 48000};
static DEFINE_MUTEX(Ana_Ctrl_Mutex);
static DEFINE_MUTEX(Ana_buf_Ctrl_Mutex);
static DEFINE_MUTEX(Ana_Clk_Mutex);
static DEFINE_MUTEX(Ana_Power_Mutex);
static DEFINE_MUTEX(AudAna_lock);

static int mAudio_Analog_Mic1_mode  = AUDIO_ANALOGUL_MODE_ACC;
static int mAudio_Analog_Mic2_mode  = AUDIO_ANALOGUL_MODE_ACC;
static int mAudio_Analog_Mic3_mode  = AUDIO_ANALOGUL_MODE_ACC;
static int mAudio_Analog_Mic4_mode  = AUDIO_ANALOGUL_MODE_ACC;

static int mAudio_Vow_Analog_Func_Enable = false;
static int mAudio_Vow_Digital_Func_Enable = false;

static int TrimOffset = 2048;
static const int DC1unit_in_uv = 21576; // in uv
static const int DC1devider = 8; // in uv

#ifdef CONFIG_MTK_SPEAKER
static int Speaker_mode = AUDIO_SPEAKER_MODE_AB;
static unsigned int Speaker_pga_gain = 1 ; // default 0Db.
static bool mSpeaker_Ocflag = false;
#endif
static int mAdc_Power_Mode = 0;
static unsigned int dAuxAdcChannel = 16;
static const int mDcOffsetTrimChannel = 9;
static bool mInitCodec = false;

static int reg_AFE_VOW_CFG0 = 0xffff;   //VOW AMPREF Setting
static int reg_AFE_VOW_CFG1 = 0x0200;   //VOW A,B timeout initial value
static int reg_AFE_VOW_CFG2 = 0x6464;   //VOW A,B value setting
static int reg_AFE_VOW_CFG3 = 0xDBAC;   //alhpa and beta K value setting
static int reg_AFE_VOW_CFG4 = 0x006E;   //gamma K value setting
static int reg_AFE_VOW_CFG5 = 0x0000;   //N mini value setting

//VOW using
typedef enum
{
    AUDIO_VOW_MIC_TYPE_Handset_AMIC = 0,
    AUDIO_VOW_MIC_TYPE_Headset_MIC,
    AUDIO_VOW_MIC_TYPE_Handset_DMIC,
} AUDIO_VOW_MIC_TYPE;

static int mAudio_VOW_Mic_type  = AUDIO_VOW_MIC_TYPE_Handset_AMIC;
static void Audio_Amp_Change(int channels , bool enable);
static void  SavePowerState(void)
{
    int i = 0;
    for (i = 0; i < AUDIO_ANALOG_VOLUME_TYPE_MAX ; i++)
    {
        mCodec_data->mAudio_BackUpAna_DevicePower[i] = mCodec_data->mAudio_Ana_DevicePower[i];
    }
}

static void  RestorePowerState(void)
{
    int i = 0;
    for (i = 0; i < AUDIO_ANALOG_VOLUME_TYPE_MAX ; i++)
    {
        mCodec_data->mAudio_Ana_DevicePower[i] = mCodec_data->mAudio_BackUpAna_DevicePower[i];
    }
}

static bool GetDLStatus(void)
{
    int i = 0;
    for (i = 0; i < AUDIO_ANALOG_DEVICE_2IN1_SPK ; i++)
    {
        if (mCodec_data->mAudio_Ana_DevicePower[i] == true)
        {
            return true;
        }
    }
    return false;
}

static bool mAnaSuspend = false;
#if 0 //K2 TODO
void SetAnalogSuspend(bool bEnable)
{
    printk("%s bEnable ==%d mAnaSuspend = %d \n", __func__, bEnable, mAnaSuspend);
    if ((bEnable == true) && (mAnaSuspend == false))
    {
        Ana_Log_Print();
        SavePowerState();
        if (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTL] == true)
        {
            mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTL] = false;
            Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_LEFT1 , false);
        }
        if (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTR] == true)
        {
            mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTR] = false;
            Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_RIGHT1 , false);
        }
        if (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HSOUTL] == true)
        {
            mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HSOUTL] = false;
            Voice_Amp_Change(false);
        }
        if (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_SPKL] == true)
        {
            mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_SPKL] = false;
            Speaker_Amp_Change(false);
        }
        Ana_Log_Print();
        mAnaSuspend = true;
    }
    else if ((bEnable == false) && (mAnaSuspend == true))
    {
        Ana_Log_Print();
        if (mCodec_data->mAudio_BackUpAna_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTL] == true)
        {
            Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_LEFT1 , true);
            mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTL] = true;
        }
        if (mCodec_data->mAudio_BackUpAna_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTR] == true)
        {
            Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_RIGHT1 , true);
            mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTR] = false;
        }
        if (mCodec_data->mAudio_BackUpAna_DevicePower[AUDIO_ANALOG_VOLUME_HSOUTL] == true)
        {
            Voice_Amp_Change(true);
            mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HSOUTL] = false;
        }
        if (mCodec_data->mAudio_BackUpAna_DevicePower[AUDIO_ANALOG_VOLUME_SPKL] == true)
        {
            Speaker_Amp_Change(true);
            mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_SPKL] = false;
        }
        RestorePowerState();
        Ana_Log_Print();
        mAnaSuspend = false;
    }
}
#endif

static int audck_buf_Count = 0;
void audckbufEnable(bool enable)
{
    printk("audckbufEnable audck_buf_Count = %d enable = %d \n", audck_buf_Count, enable);
    mutex_lock(&Ana_buf_Ctrl_Mutex);
    if (enable)
    {
        if (audck_buf_Count == 0)
        {
            printk("+clk_buf_ctrl(CLK_BUF_AUDIO,true)\n");
            clk_buf_ctrl(CLK_BUF_AUDIO, true);
            printk("-clk_buf_ctrl(CLK_BUF_AUDIO,true)\n");
        }
        audck_buf_Count++;
    }
    else
    {
        audck_buf_Count--;
        if (audck_buf_Count == 0)
        {
            printk("+clk_buf_ctrl(CLK_BUF_AUDIO,false)\n");
            clk_buf_ctrl(CLK_BUF_AUDIO, false);
            printk("-clk_buf_ctrl(CLK_BUF_AUDIO,false)\n");
        }
        if (audck_buf_Count < 0)
        {
            printk("audck_buf_Count count <0 \n");
            audck_buf_Count = 0;
        }
    }
    mutex_unlock(&Ana_buf_Ctrl_Mutex);
}

static int ClsqAuxCount = 0;
static void ClsqAuxEnable(bool enable)
{
    printk("ClsqAuxEnable ClsqAuxCount = %d enable = %d \n", ClsqAuxCount, enable);
    mutex_lock(& AudAna_lock);
    if (enable)
    {
        if (ClsqAuxCount == 0)
        {
            Ana_Set_Reg(TOP_CLKSQ, 0x0020, 0x0020); //Enable CLKSQ for AUXADC
        }
        ClsqAuxCount++;
    }
    else
    {
        ClsqAuxCount--;
        if (ClsqAuxCount < 0)
        {
            printk("ClsqAuxEnable count <0 \n");
            ClsqAuxCount = 0;
        }
        if (ClsqAuxCount == 0)
        {
            Ana_Set_Reg(TOP_CLKSQ, 0x0000, 0x0020);
        }
    }
    mutex_unlock(& AudAna_lock);
}

static int ClsqCount = 0;
static void ClsqEnable(bool enable)
{
    printk("ClsqEnable ClsqAuxCount = %d enable = %d \n", ClsqCount, enable);
    mutex_lock(& AudAna_lock);
    if (enable)
    {
        if (ClsqCount == 0)
        {
            Ana_Set_Reg(TOP_CLKSQ, 0x0001, 0x0001); //Enable CLKSQ 26MHz
            Ana_Set_Reg(TOP_CLKSQ_SET, 0x0001, 0x0001); //Turn on 26MHz source clock
        }
        ClsqCount++;
    }
    else
    {
        ClsqCount--;
        if (ClsqCount < 0)
        {
            printk("ClsqEnable count <0 \n");
            ClsqCount = 0;
        }
        if (ClsqCount == 0)
        {
            Ana_Set_Reg(TOP_CLKSQ_CLR, 0x0001, 0x0001); //Turn off 26MHz source clock
            Ana_Set_Reg(TOP_CLKSQ, 0x0000, 0x0001); //Disable CLKSQ 26MHz
        }
    }
    mutex_unlock(& AudAna_lock);
}

static int TopCkCount = 0;
static void Topck_Enable(bool enable)
{
    printk("Topck_Enable enable = %d TopCkCount = %d \n", enable, TopCkCount);
    mutex_lock(&Ana_Clk_Mutex);
    if (enable == true)
    {
        if (TopCkCount == 0)
        {
            Ana_Set_Reg(TOP_CKPDN_CON0_CLR, 0x3800, 0x3800);  //Turn on AUDNCP_CLKDIV engine clock,Turn on AUD 26M
        }
        TopCkCount++;
    }
    else
    {
        TopCkCount--;
        if (TopCkCount == 0)
        {
            Ana_Set_Reg(TOP_CKPDN_CON0_SET, 0x3800, 0x3800); //Turn off AUDNCP_CLKDIV engine clock,Turn off AUD 26M
        }
        if (TopCkCount <= 0)
        {
            printk("TopCkCount <0 =%d\n ", TopCkCount);
            TopCkCount = 0;
        }
    }
    mutex_unlock(&Ana_Clk_Mutex);
}

static int NvRegCount = 0;
static void NvregEnable(bool enable)
{
    printk("NvregEnable NvRegCount == %d enable = %d \n", NvRegCount, enable);
    mutex_lock(&Ana_Clk_Mutex);
    if (enable == true)
    {
        if (NvRegCount == 0)
        {
            Ana_Set_Reg(AUDDEC_ANA_CON8, 0x0000, 0x0002); // Enable AUDGLB
        }
        NvRegCount++;
    }
    else
    {
        NvRegCount--;
        if (NvRegCount == 0)
        {
            Ana_Set_Reg(AUDDEC_ANA_CON8, 0x0002, 0x0002); // disable AUDGLB
        }
        if (NvRegCount < 0)
        {
            printk("NvRegCount <0 =%d\n ", NvRegCount);
            NvRegCount = 0;
        }
    }
    mutex_unlock(&Ana_Clk_Mutex);
}

static int AdcClockCount = 0;
static void AdcClockEnable(bool enable)
{
    mutex_lock(&Ana_Clk_Mutex);
    if (enable == true)
    {
        if (AdcClockCount == 0)
        {
            Ana_Set_Reg(TOP_CKPDN_CON0_CLR, 0x3000, 0x3000);  //AUD clock power down released
        }
        AdcClockCount++;
    }
    else
    {
        AdcClockCount--;
        if (AdcClockCount == 0)
        {
            Ana_Set_Reg(TOP_CKPDN_CON0_SET, 0x3000, 0x3000);
        }
        if (AdcClockCount <= 0)
        {
            printk("TopCkCount <0 =%d\n ", AdcClockCount);
            AdcClockCount = 0;
        }
    }
    mutex_unlock(&Ana_Clk_Mutex);
}

void vow_irq_handler(void)
{
    printk("vow_irq_handler,audio irq event....\n");
    //TurnOnVOWADcPowerACC(AUDIO_ANALOG_DEVICE_IN_ADC1, false);
    //TurnOnVOWDigitalHW(false);
#if defined(VOW_TONE_TEST)
    EnableSideGenHw(Soc_Aud_InterConnectionOutput_O03, Soc_Aud_MemIF_Direction_DIRECTION_OUTPUT, true);
#endif
    //VowDrv_ChangeStatus();
}

static int LowPowerAdcClockCount = 0;
static void LowPowerAdcClockEnable(bool enable)
{
    mutex_lock(&Ana_Clk_Mutex);
    if (enable == true)
    {
        if (LowPowerAdcClockCount == 0)
        {
            Ana_Set_Reg(TOP_CKPDN_CON0_CLR, 0x8000, 0xffff);  //Lowpower AUD clock power down released
        }
        LowPowerAdcClockCount++;
    }
    else
    {
        LowPowerAdcClockCount--;
        if (LowPowerAdcClockCount == 0)
        {
            Ana_Set_Reg(TOP_CKPDN_CON0_SET, 0x8000, 0xffff);
        }
        if (LowPowerAdcClockCount < 0)
        {
            printk("LowPowerAdcClockCount <0 =%d\n ", LowPowerAdcClockCount);
            LowPowerAdcClockCount = 0;
        }
    }
    mutex_unlock(&Ana_Clk_Mutex);
}


#ifdef CONFIG_MTK_SPEAKER
static void Apply_Speaker_Gain(void)
{
    printk("%s Speaker_pga_gain= %d\n", __func__, Speaker_pga_gain);

    Ana_Set_Reg(SPK_ANA_CON0,  Speaker_pga_gain << 11, 0x7800);
}
#else
static void Apply_Speaker_Gain(void)
{

}
#endif

void setOffsetTrimMux(unsigned int Mux)
{
    printk("%s Mux = %d\n", __func__, Mux);
#if 0
    Ana_Set_Reg(AUDBUF_CFG7 , Mux << 12, 0xf << 12); // buffer mux select
#else //K2 TODO

#endif
}

void setOffsetTrimBufferGain(unsigned int gain)
{
#if 0
    Ana_Set_Reg(AUDBUF_CFG7 , (gain * 6) << 10, 0x3 << 10); // buffer mux select
#else //K2 TODO

#endif

}

static int mHplTrimOffset = 2048;
static int mHprTrimOffset = 2048;

void SetHplTrimOffset(int Offset)
{
    mHplTrimOffset = Offset;
    if ((Offset > 2200) || (Offset < 1900))
    {
        mHplTrimOffset = 2048;
        printk("SetHplTrimOffset offset may be invalid offset = %d\n", Offset);
    }
}

void SetHprTrimOffset(int Offset)
{
    mHprTrimOffset = Offset;
    if ((Offset > 2200) || (Offset < 1900))
    {
        mHplTrimOffset = 2048;
        printk("SetHplTrimOffset offset may be invalid offset = %d\n", Offset);
    }
}

void EnableTrimbuffer(bool benable)
{
#if 0
    if (benable == true)
    {
        Ana_Set_Reg(AUDBUF_CFG8 , 0x1 << 13, 0x1 << 13); // trim buffer enable
    }
    else
    {
        Ana_Set_Reg(AUDBUF_CFG8 , 0x0 << 13, 0x1 << 13); // trim buffer enable
    }
#else //K2 TODO

#endif
}

void OpenAnalogTrimHardware(bool enable)
{
#if 0
    if (enable)
    {
        TurnOnDacPower();
        printk("%s \n", __func__);
        //Ana_Set_Reg(AUDNVREGGLB_CFG0, 0x0000, 0xffff); // Enable AUDGLB
        OpenClassH();
        Ana_Set_Reg(AUDLDO_NVREG_CFG0,   0x0028, 0xffff); // Enable cap-less LDOs (1.6V)
        Ana_Set_Reg(AUDLDO_NVREG_CFG0,   0x0068, 0xffff); // Enable NV regulator (-1.6V)
        Ana_Set_Reg(AUDBUF_CFG5, 0x001f, 0xffff); // enable HP bias circuits
        msleep(1);
        Ana_Set_Reg(ZCD_CON0,   0x0000, 0xffff); // Disable AUD_ZCD_CFG0
        Ana_Set_Reg(AUDBUF_CFG0,   0xE008, 0xffff); // Disable headphone, voice and short-ckt protection.
        Ana_Set_Reg(IBIASDIST_CFG0,   0x0092, 0xffff); //Enable IBIST
        Ana_Set_Reg(ZCD_CON2,  0x0F9F , 0xffff); // Set HPR/HPL gain as 0dB, step by step
        Ana_Set_Reg(ZCD_CON3,  0x001F , 0xffff); //Set voice gain as minimum (~ -40dB)
        Ana_Set_Reg(AUDBUF_CFG1,  0x0900 , 0xffff); //De_OSC of HP and enable output STBENH
        Ana_Set_Reg(AUDBUF_CFG2,  0x0022 , 0xffff); //De_OSC of voice, enable output STBENH
        Ana_Set_Reg(AUDBUF_CFG0,  0xE009 , 0xffff); //Enable voice driver
        Ana_Set_Reg(AUDBUF_CFG1,  0x0940 , 0xffff); //Enable pre-charge buffer
        Ana_Set_Reg(AUDCLKGEN_CFG0, 0x5501 , 0xffff); //Enable AUD_CLK
        Ana_Set_Reg(AUDDAC_CFG0, 0x000c , 0x000c); //Enable Audio DAC
        Ana_Set_Reg(AUDDAC_CFG0, 0x0003 , 0x0003); //Enable Audio DAC
    }
    else
    {
        printk("Audio_Amp_Change off amp\n");
        Ana_Set_Reg(AUDDAC_CFG0, 0x0000, 0xffff); // Disable Audio DAC
        Ana_Set_Reg(AUDCLKGEN_CFG0, 0x5500, 0xffff); // Disable AUD_CLK
        Ana_Set_Reg(IBIASDIST_CFG0, 0x0192, 0xffff); // Disable IBIST
        Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0028, 0xffff); // Disable NV regulator (-1.6V)
        Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0000, 0xffff); // Disable cap-less LDOs (1.6V)
        Ana_Set_Reg(AUDDAC_CFG0, 0x000e, 0xffff); // enable audio bias. enable audio DAC, HP buffers
        Ana_Set_Reg(AUDDAC_CFG0, 0x000d, 0xffff); // enable audio bias. enable audio DAC, HP buffers
        Ana_Set_Reg(AFE_DL_SRC2_CON0_L , 0x1800 , 0xffffffff);
        Ana_Set_Reg(AFE_UL_DL_CON0, 0x0000, 0xffff);   //turn off afe
        TurnOffDacPower();
    }
#else //K2 TODO

#endif
}

void OpenAnalogHeadphone(bool bEnable)
{
    printk("OpenAnalogHeadphone bEnable = %d", bEnable);
    if (bEnable)
    {
        mBlockSampleRate[AUDIO_ANALOG_DEVICE_OUT_DAC] = 44100;
        Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_LEFT1 , true);
        mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTL] = true;
        Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_RIGHT1 , true);
        mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTR] = true;
    }
    else
    {
        mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTL] = false;
        Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_LEFT1 , false);
        mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTR] = false;
        Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_RIGHT1 , false);
    }
}

bool OpenHeadPhoneImpedanceSetting(bool bEnable)
{
    printk("%s benable = %d\n", __func__, bEnable);
#if 0 //K2 TODO 
    if (GetDLStatus() == true)
    {
        return false;
    }

    if (bEnable == true)
    {
        TurnOnDacPower();
        OpenClassAB();
        Ana_Set_Reg(AUDBUF_CFG5, 0x0003, 0x0003); // enable HP bias circuits
        Ana_Set_Reg(AUDLDO_NVREG_CFG0,   0x0028, 0xffff); // Enable cap-less LDOs (1.6V)
        Ana_Set_Reg(AUDLDO_NVREG_CFG0,   0x0068, 0xffff); // Enable NV regulator (-1.6V)
        Ana_Set_Reg(ZCD_CON0, 0x0000, 0xffff);
        Ana_Set_Reg(AUDBUF_CFG0, 0xE000, 0xffff);
        Ana_Set_Reg(AUDBUF_CFG1, 0x0000, 0xffff);
        Ana_Set_Reg(AUDBUF_CFG8, 0x4000, 0xffff);
        Ana_Set_Reg(IBIASDIST_CFG0, 0x0092, 0xffff);
        Ana_Set_Reg(AUDCLKGEN_CFG0, 0x5501, 0xffff);
        Ana_Set_Reg(AUDDAC_CFG0, 0x0009, 0xffff);
        Ana_Set_Reg(AUDBUF_CFG6, 0x4800, 0xffff);
    }
    else
    {
        Ana_Set_Reg(AUDDAC_CFG0, 0x0000, 0xffff); // Disable Audio DAC
        Ana_Set_Reg(AUDCLKGEN_CFG0, 0x5500, 0xffff); // Disable AUD_CLK
        Ana_Set_Reg(IBIASDIST_CFG0, 0x0192, 0xffff); // Disable IBIST
        Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0028, 0xffff); // Disable NV regulator (-1.6V)
        Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0000, 0xffff); // Disable cap-less LDOs (1.6V)
        Ana_Set_Reg(AUDBUF_CFG8, 0x0000, 0xffff);
        Ana_Set_Reg(AUDBUF_CFG5, 0x0000, 0x0003); // disable HP bias circuits
        TurnOffDacPower();
        Ana_Set_Reg(AUDBUF_CFG6, 0x0000, 0xffff);
    }
#endif
    return true;
}

void setHpGainZero(void)
{
    Ana_Set_Reg(ZCD_CON2, 0x8 << 7, 0x0f80);
    Ana_Set_Reg(ZCD_CON2, 0x8 , 0x001f);
}

void SetSdmLevel(unsigned int level)
{
    Ana_Set_Reg(AFE_DL_SDM_CON1, level, 0xffffffff);
}


static void SetHprOffset(int OffsetTrimming)
{
    short Dccompsentation = 0;
    int DCoffsetValue = 0;
    unsigned short RegValue = 0;
    //printk("%s OffsetTrimming = %d \n",__func__,OffsetTrimming);
    DCoffsetValue = OffsetTrimming * 1000000; // in uv
    DCoffsetValue = (DCoffsetValue / DC1devider);  // in uv
    DCoffsetValue = (DCoffsetValue / DC1unit_in_uv);
    Dccompsentation = DCoffsetValue;
    RegValue = Dccompsentation;
    //printk("%s RegValue = %d",__func__,RegValue);
    Ana_Set_Reg(AFE_DL_DC_COMP_CFG1, RegValue , 0xffff);
}

static void SetHplOffset(int OffsetTrimming)
{
    short Dccompsentation = 0;
    int DCoffsetValue = 0;
    unsigned short RegValue = 0;
    //printk("%s OffsetTrimming = %d \n",__func__,OffsetTrimming);
    DCoffsetValue = OffsetTrimming * 1000000; // in uv
    DCoffsetValue = (DCoffsetValue / DC1devider);  // in uv
    DCoffsetValue = (DCoffsetValue / DC1unit_in_uv);
    Dccompsentation = DCoffsetValue;
    RegValue = Dccompsentation;
    //printk("%s RegValue = %d",__func__,RegValue);
    Ana_Set_Reg(AFE_DL_DC_COMP_CFG0, RegValue  , 0xffff);
}

static void EnableDcCompensation(bool bEnable)
{
    Ana_Set_Reg(AFE_DL_DC_COMP_CFG2, bEnable , 0x1);
}

static void SetHprOffsetTrim(void)
{
    int OffsetTrimming = mHprTrimOffset - TrimOffset;
    SetHprOffset(OffsetTrimming);
}

static void SetHpLOffsetTrim(void)
{
    int OffsetTrimming = mHplTrimOffset - TrimOffset;
    SetHplOffset(OffsetTrimming);
}

static void SetDcCompenSation(void)
{
#if 0
    SetHprOffsetTrim();
    SetHpLOffsetTrim();
    EnableDcCompensation(true);
#else //K2 TODO

#endif
}

static void OpenClassH(void)
{
#if 0 //K2 removed
    Ana_Set_Reg(AFE_CLASSH_CFG7, 0xAD2D, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG8, 0x1313, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG9, 0x132d, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG10, 0x2d13, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG11, 0x1315, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG12, 0x6464, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG13, 0x2a2a, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG14, 0x009c, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG26, 0x9313, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG27, 0x1313, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG28, 0x1315, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG29, 0x1515, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG30, 0x1515, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG1, 0xBF04, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG2, 0x5fbf, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG3, 0x1104, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG4, 0x2412, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG5, 0x0201, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG6, 0x2800, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG14, 0x009c, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG21, 0x2108, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG22, 0x06db, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG23, 0xffff , 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG24, 0x0bd6, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG25, 0x1740, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG0,   0xd608, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG0,   0xd20d, 0xffff); // Classh CK fix 591KHz
    //Ana_Set_Reg(0x0388,   0x0300, 0xffff);
#endif
}

static void OpenClassAB(void)
{
#if 0 //K2 removed
    Ana_Set_Reg(AFE_CLASSH_CFG7, 0x8909, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG8, 0x0909, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG9, 0x1309, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG10, 0x0909, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG11, 0x0915, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG12, 0x1414, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG13, 0x1414, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG14, 0x009c, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG26, 0x9313, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG27, 0x1313, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG28, 0x1315, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG29, 0x1515, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG30, 0x1515, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG1, 0x0024, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG2, 0x2f90, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG3, 0x1104, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG4, 0x2412, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG5, 0x0201, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG6, 0x2800, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG21, 0xa108, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG22, 0x06db, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG23, 0x0bd6, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG24, 0x1492, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG25, 0x1740, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG0,   0xd419, 0xffff); // Classh CK fix 591KHz
    Ana_Set_Reg(AFE_CLASSH_CFG1,   0x0021, 0xffff); // Classh CK fix 591KHz
#endif
}

static void SetDCcoupleNP(int ADCType, int mode)
{
    printk("%s ADCType= %d mode = %d\n", __func__, ADCType, mode);
    switch (mode)
    {
        case AUDIO_ANALOGUL_MODE_ACC:
        case AUDIO_ANALOGUL_MODE_DCC:
        case AUDIO_ANALOGUL_MODE_DMIC:
        {
            if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC1)
            {
                mt6331_upmu_set_rg_audmicbias0dcswnen(false); // mic0 DC N external
                mt6331_upmu_set_rg_audmicbias0dcswpen(false); // mic0 DC P external
            }
            else if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC2)
            {
                mt6331_upmu_set_rg_audmicbias1dcswnen(false); // mic0 DC N external
                mt6331_upmu_set_rg_audmicbias1dcswpen(false); // mic0 DC P external
            }
        }
        break;
        case AUDIO_ANALOGUL_MODE_DCCECMDIFF:
        {
            if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC1)
            {
                mt6331_upmu_set_rg_audmicbias0dcswnen(true); // mic0 DC N internal
                mt6331_upmu_set_rg_audmicbias0dcswpen(true); // mic0 DC P internal
            }
            else if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC2)
            {
                mt6331_upmu_set_rg_audmicbias1dcswnen(true); // mic0 DC N internal
                mt6331_upmu_set_rg_audmicbias1dcswpen(true); // mic0 DC P internal
            }
        }
        break;
        case AUDIO_ANALOGUL_MODE_DCCECMSINGLE:
        {
            if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC1)
            {
                mt6331_upmu_set_rg_audmicbias0dcswnen(false); // mic0 DC N internal
                mt6331_upmu_set_rg_audmicbias0dcswpen(true); // mic0 DC P internal
            }
            else if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC2)
            {
                mt6331_upmu_set_rg_audmicbias1dcswnen(true); // mic0 DC N internal
                mt6331_upmu_set_rg_audmicbias1dcswpen(false); // mic0 DC P internal
            }
        }
        break;
        default:
            break;
    }
}

static void OpenMicbias3(bool bEnable)
{
#if 0
    Ana_Set_Reg(AUDMICBIAS_CFG1, (bEnable << 7), (0x1 << 7));
#else //K2 TODO

#endif
}

static void OpenMicbias2(bool bEnable)
{
#if 0
    Ana_Set_Reg(AUDMICBIAS_CFG1, (bEnable << 0), (0x1 << 0));
#else //K2 TODO

#endif

}

static void SetMicVref2(uint32_t vRef)
{
#if 0
    Ana_Set_Reg(AUDMICBIAS_CFG1, vRef << 1, 0x7 << 1);
#else //K2 TODO

#endif

}

static void SetMicVref3(uint32_t vRef)
{
#if 0
    Ana_Set_Reg(AUDMICBIAS_CFG1, vRef << 8, 0x7 << 8);
#else //K2 TODO

#endif

}
static void EnableMicBias(uint32_t Mic, bool bEnable)
{
#if 0//K2 TODO
    if (bEnable == true)
    {
        switch (Mic)
        {
            case AUDIO_ANALOG_DEVICE_IN_ADC1:
                OpenMicbias0(true);
                OpenMicbias1(true);
                break;
            case AUDIO_ANALOG_DEVICE_IN_ADC2:
                OpenMicbias2(true);
                break;
            case AUDIO_ANALOG_DEVICE_IN_ADC3:
            case AUDIO_ANALOG_DEVICE_IN_ADC4:
                OpenMicbias3(true);
                break;
        }
    }
    else
    {
        switch (Mic)
        {
            case AUDIO_ANALOG_DEVICE_IN_ADC1:
                OpenMicbias0(false);
                OpenMicbias1(false);
                break;
            case AUDIO_ANALOG_DEVICE_IN_ADC2:
                OpenMicbias2(false);
                break;
            case AUDIO_ANALOG_DEVICE_IN_ADC3:
            case AUDIO_ANALOG_DEVICE_IN_ADC4:
                OpenMicbias3(false);
                break;
        }
    }
#endif
}

static void SetMic2DCcoupleSwitch(bool internal)
{
#if 0
    Ana_Set_Reg(AUDMICBIAS_CFG1, internal << 4, 0x1 << 4);
    Ana_Set_Reg(AUDMICBIAS_CFG1, internal << 5, 0x1 << 5);
#else //K2 TODO

#endif
}

static void SetMic3DCcoupleSwitch(bool internal)
{
#if 0
    Ana_Set_Reg(AUDMICBIAS_CFG1, internal << 11, 0x1 << 4);
    Ana_Set_Reg(AUDMICBIAS_CFG1, internal << 12, 0x1 << 5);
#else //K2 TODO

#endif
}

static void SetMic2DCcoupleSwitchSingle(bool internal)
{
#if 0
    Ana_Set_Reg(AUDMICBIAS_CFG1, 1 << 4, 0x1 << 4);
    Ana_Set_Reg(AUDMICBIAS_CFG1, 0 << 5, 0x1 << 5);
#else //K2 TODO

#endif
}

static void SetMic3DCcoupleSwitchSingle(bool internal)
{
#if 0
    Ana_Set_Reg(AUDMICBIAS_CFG1, 1 << 11, 0x1 << 4);
    Ana_Set_Reg(AUDMICBIAS_CFG1, 0 << 12, 0x1 << 5);
#else //K2 TODO

#endif
}

static void SetMic2powermode(bool lowpower)
{
#if 0
    Ana_Set_Reg(AUDMICBIAS_CFG1, lowpower << 6, 0x1 << 6);
#else //K2 TODO

#endif

}

static void SetMic3powermode(bool lowpower)
{
#if 0
    Ana_Set_Reg(AUDMICBIAS_CFG1, lowpower << 13, 0x1 << 13);
#else //K2 TODO

#endif

}

#if 0//K2 TODO
static void OpenMicbias1(bool bEnable)
{
    printk("%s bEnable = %d \n", __func__, bEnable);

    if (bEnable == true)
    {
        mt6331_upmu_set_rg_audpwdbmicbias1(true); // mic bias power 1 on
    }
    else
    {
        mt6331_upmu_set_rg_audmicbias1lowpen(true); // mic 1 low power mode
        mt6331_upmu_set_rg_audpwdbmicbias1(false); // mic bias power 1 off
    }
}

static void SetMicbias1lowpower(bool benable)
{
    mt6331_upmu_set_rg_audmicbias1lowpen(benable); // mic 1 power mode
}

static void OpenMicbias0(bool bEanble)
{

    printk("%s bEanble = %d\n", __func__, bEanble);
    if (bEanble == true)
    {
        mt6331_upmu_set_rg_audpwdbmicbias0(true); // mic bias power 0 on
        mt6331_upmu_set_rg_audmicbias0vref(0x2); // set to 1.9V
    }
    else
    {
        mt6331_upmu_set_rg_audmicbias0lowpen(true); // mic 0 low power mode
        mt6331_upmu_set_rg_audpwdbmicbias0(false); // mic bias power 0 off
    }
}

static void SetMicbias0lowpower(bool benable)
{
    mt6331_upmu_set_rg_audmicbias0lowpen(benable); // mic 1 power mode
}
#endif
static bool Dl_Hpdet_impedence(void)
{
#if 0
    ClsqAuxEnable(true);
    ClsqEnable(true);
    Topck_Enable(true);
    NvregEnable(true);
    Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0000, 0xffff);   //power on clock
    OpenClassH();
    Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0028, 0xffff); //Enable cap-less LDOs (1.6V)
    Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0068, 0xffff); //Enable NV regulator (-1.6V)
    Ana_Set_Reg(AUDBUF_CFG5, 0x001f, 0xffff); // enable HP bias circuits
    Ana_Set_Reg(ZCD_CON0, 0x0000, 0xffff); //Disable AUD_ZCD
    Ana_Set_Reg(AUDBUF_CFG0, 0xE000, 0xffff); //Disable headphone, voice and short-ckt protection.
    Ana_Set_Reg(AUDBUF_CFG1, 0x0000, 0xffff); //De_OSC of HP and output STBENH disable
    Ana_Set_Reg(AUDBUF_CFG8, 0x4000, 0xffff); //HPDET circuit enable
    Ana_Set_Reg(IBIASDIST_CFG0, 0x0092, 0xffff); //Enable IBIST
    Ana_Set_Reg(AUDCLKGEN_CFG0, 0x5501, 0xffff); //Enable AUD_CLK
    Ana_Set_Reg(AUDDAC_CFG0, 0x0009, 0xffff); //Enable Audio DAC
    Ana_Set_Reg(AUDCLKGEN_CFG0, 0x4800, 0xffff); //Select HPR as HPDET output and select DACLP as HPDET circuit input
    //Hereafter, use AUXADC for HP impedance detection , start ADC....


    ClsqAuxEnable(false);
    ClsqEnable(false);
    Topck_Enable(false);
    NvregEnable(false);
#else //K2 TODO

#endif
    return true;
}


static uint32 GetULNewIFFrequency(uint32 frequency)
{
    uint32 Reg_value = 0;
    switch (frequency)
    {
        case 8000:
        case 16000:
        case 32000:
            Reg_value = 1;
            break;
        case 48000:
            Reg_value = 3;
        default:
            break;
    }
    return Reg_value;
}

uint32 GetULFrequency(uint32 frequency)
{
    uint32 Reg_value = 0;
    printk("%s frequency =%d\n", __func__, frequency);
    switch (frequency)
    {
        case 8000:
        case 16000:
        case 32000:
            Reg_value = 0x0;
            break;
        case 48000:
            Reg_value = 0x1;
        default:
            break;

    }
    return Reg_value;
}


uint32 ULSampleRateTransform(uint32 SampleRate)
{
    switch (SampleRate)
    {
        case 8000:
            return 0;
        case 16000:
            return 1;
        case 32000:
            return 2;
        case 48000:
            return 3;
        default:
            break;
    }
    return 0;
}


static int mt63xx_codec_startup(struct snd_pcm_substream *substream , struct snd_soc_dai *Daiport)
{
    //printk("+mt63xx_codec_startup name = %s number = %d\n", substream->name, substream->number);
    if (substream->stream == SNDRV_PCM_STREAM_CAPTURE && substream->runtime->rate)
    {
        //printk("mt63xx_codec_startup set up SNDRV_PCM_STREAM_CAPTURE rate = %d\n", substream->runtime->rate);
        mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC] = substream->runtime->rate;

    }
    else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK && substream->runtime->rate)
    {
        //printk("mt63xx_codec_startup set up SNDRV_PCM_STREAM_PLAYBACK rate = %d\n", substream->runtime->rate);
        mBlockSampleRate[AUDIO_ANALOG_DEVICE_OUT_DAC] = substream->runtime->rate;
    }
    //printk("-mt63xx_codec_startup name = %s number = %d\n", substream->name, substream->number);
    return 0;
}

static int mt63xx_codec_prepare(struct snd_pcm_substream *substream , struct snd_soc_dai *Daiport)
{
    if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
    {
        printk("mt63xx_codec_prepare set up SNDRV_PCM_STREAM_CAPTURE rate = %d\n", substream->runtime->rate);
        mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC] = substream->runtime->rate;

    }
    else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
    {
        printk("mt63xx_codec_prepare set up SNDRV_PCM_STREAM_PLAYBACK rate = %d\n", substream->runtime->rate);
        mBlockSampleRate[AUDIO_ANALOG_DEVICE_OUT_DAC] = substream->runtime->rate;
    }
    return 0;
}

static int mt6323_codec_trigger(struct snd_pcm_substream *substream , int command , struct snd_soc_dai *Daiport)
{
    switch (command)
    {
        case SNDRV_PCM_TRIGGER_START:
        case SNDRV_PCM_TRIGGER_RESUME:
        case SNDRV_PCM_TRIGGER_STOP:
        case SNDRV_PCM_TRIGGER_SUSPEND:
            break;
    }

    //printk("mt6323_codec_trigger command = %d\n ", command);
    return 0;
}

static const struct snd_soc_dai_ops mt6323_aif1_dai_ops =
{
    .startup    = mt63xx_codec_startup,
    .prepare   = mt63xx_codec_prepare,
    .trigger     = mt6323_codec_trigger,
};

static struct snd_soc_dai_driver mtk_6331_dai_codecs[] =
{
    {
        .name = MT_SOC_CODEC_TXDAI_NAME,
        .ops = &mt6323_aif1_dai_ops,
        .playback = {
            .stream_name = MT_SOC_DL1_STREAM_NAME,
            .channels_min = 1,
            .channels_max = 2,
            .rates = SNDRV_PCM_RATE_8000_48000,
            .formats = SND_SOC_ADV_MT_FMTS,
        },
    },
    {
        .name = MT_SOC_CODEC_RXDAI_NAME,
        .ops = &mt6323_aif1_dai_ops,
        .capture = {
            .stream_name = MT_SOC_UL1_STREAM_NAME,
            .channels_min = 1,
            .channels_max = 2,
            .rates = SNDRV_PCM_RATE_8000_48000,
            .formats = SND_SOC_ADV_MT_FMTS,
        },
    },
    {
        .name = MT_SOC_CODEC_TDMRX_DAI_NAME,
        .ops = &mt6323_aif1_dai_ops,
        .capture = {
            .stream_name = MT_SOC_TDM_CAPTURE_STREAM_NAME,
            .channels_min = 2,
            .channels_max = 8,
            .rates = SNDRV_PCM_RATE_8000_192000,
            .formats = (SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S8 |
            SNDRV_PCM_FMTBIT_U16_LE | SNDRV_PCM_FMTBIT_S16_LE |
            SNDRV_PCM_FMTBIT_U16_BE | SNDRV_PCM_FMTBIT_S16_BE |
            SNDRV_PCM_FMTBIT_U24_LE | SNDRV_PCM_FMTBIT_S24_LE |
            SNDRV_PCM_FMTBIT_U24_BE | SNDRV_PCM_FMTBIT_S24_BE |
            SNDRV_PCM_FMTBIT_U24_3LE | SNDRV_PCM_FMTBIT_S24_3LE |
            SNDRV_PCM_FMTBIT_U24_3BE | SNDRV_PCM_FMTBIT_S24_3BE |
            SNDRV_PCM_FMTBIT_U32_LE | SNDRV_PCM_FMTBIT_S32_LE |
            SNDRV_PCM_FMTBIT_U32_BE | SNDRV_PCM_FMTBIT_S32_BE),
        },
    },
    {
        .name = MT_SOC_CODEC_I2S0TXDAI_NAME,
        .ops = &mt6323_aif1_dai_ops,
        .playback = {
            .stream_name = MT_SOC_I2SDL1_STREAM_NAME,
            .channels_min = 1,
            .channels_max = 2,
            .rates = SNDRV_PCM_RATE_8000_48000,
            .formats = SND_SOC_ADV_MT_FMTS,
        }
    },
    {
        .name = MT_SOC_CODEC_VOICE_MD1DAI_NAME,
        .ops = &mt6323_aif1_dai_ops,
        .playback = {
            .stream_name = MT_SOC_VOICE_MD1_STREAM_NAME,
            .channels_min = 1,
            .channels_max = 2,
            .rates = SNDRV_PCM_RATE_8000_48000,
            .formats = SND_SOC_ADV_MT_FMTS,
        },
        .capture = {
            .stream_name = MT_SOC_VOICE_MD1_STREAM_NAME,
            .channels_min = 1,
            .channels_max = 2,
            .rates = SNDRV_PCM_RATE_8000_48000,
            .formats = SND_SOC_ADV_MT_FMTS,
        },
    },
    {
        .name = MT_SOC_CODEC_VOICE_MD2DAI_NAME,
        .ops = &mt6323_aif1_dai_ops,
        .playback = {
            .stream_name = MT_SOC_VOICE_MD2_STREAM_NAME,
            .channels_min = 1,
            .channels_max = 2,
            .rates = SNDRV_PCM_RATE_8000_48000,
            .formats = SND_SOC_ADV_MT_FMTS,
        },
        .capture = {
            .stream_name = MT_SOC_VOICE_MD2_STREAM_NAME,
            .channels_min = 1,
            .channels_max = 2,
            .rates = SNDRV_PCM_RATE_8000_48000,
            .formats = SND_SOC_ADV_MT_FMTS,
        },
    },
    {
        .name = MT_SOC_CODEC_FMI2S2RXDAI_NAME,
        .ops = &mt6323_aif1_dai_ops,
        .playback = {
            .stream_name = MT_SOC_FM_I2S2_STREAM_NAME,
            .channels_min = 1,
            .channels_max = 2,
            .rates = SNDRV_PCM_RATE_8000_48000,
            .formats = SND_SOC_ADV_MT_FMTS,
        },
        .capture = {
            .stream_name = MT_SOC_FM_I2S2_RECORD_STREAM_NAME,
            .channels_min = 1,
            .channels_max = 2,
            .rates = SNDRV_PCM_RATE_8000_48000,
            .formats = SND_SOC_ADV_MT_FMTS,
        },
    },
    {
        .name = MT_SOC_CODEC_FMMRGTXDAI_DUMMY_DAI_NAME,
        .ops = &mt6323_aif1_dai_ops,
        .playback = {
            .stream_name = MT_SOC_FM_MRGTX_STREAM_NAME,
            .channels_min = 1,
            .channels_max = 2,
            .rates = SNDRV_PCM_RATE_44100,
            .formats = SND_SOC_ADV_MT_FMTS,
        },
    },
    {
        .name = MT_SOC_CODEC_ULDLLOOPBACK_NAME,
        .ops = &mt6323_aif1_dai_ops,
        .playback = {
            .stream_name = MT_SOC_ULDLLOOPBACK_STREAM_NAME,
            .channels_min = 1,
            .channels_max = 2,
            .rates = SNDRV_PCM_RATE_8000_48000,
            .formats = SND_SOC_ADV_MT_FMTS,
        },
        .capture = {
            .stream_name = MT_SOC_ULDLLOOPBACK_STREAM_NAME,
            .channels_min = 1,
            .channels_max = 2,
            .rates = SNDRV_PCM_RATE_8000_48000,
            .formats = SND_SOC_ADV_MT_FMTS,
        },
    },
    {
        .name = MT_SOC_CODEC_STUB_NAME,
        .ops = &mt6323_aif1_dai_ops,
        .playback = {
            .stream_name = MT_SOC_ROUTING_STREAM_NAME,
            .channels_min = 1,
            .channels_max = 2,
            .rates = SNDRV_PCM_RATE_8000_48000,
            .formats = SND_SOC_ADV_MT_FMTS,
        },
    },
    {
        .name = MT_SOC_CODEC_RXDAI2_NAME,
        .capture = {
            .stream_name = MT_SOC_UL1DATA2_STREAM_NAME,
            .channels_min = 1,
            .channels_max = 2,
            .rates = SNDRV_PCM_RATE_8000_48000,
            .formats = SND_SOC_ADV_MT_FMTS,
        },
    },
    {
        .name = MT_SOC_CODEC_MRGRX_DAI_NAME,
        .ops = &mt6323_aif1_dai_ops,
        .playback = {
            .stream_name = MT_SOC_MRGRX_STREAM_NAME,
            .channels_min = 1,
            .channels_max = 8,
            .rates = SNDRV_PCM_RATE_8000_192000,
            .formats = SND_SOC_ADV_MT_FMTS,
        },
        .capture = {
            .stream_name = MT_SOC_MRGRX_STREAM_NAME,
            .channels_min = 1,
            .channels_max = 8,
            .rates = SNDRV_PCM_RATE_8000_192000,
            .formats = SND_SOC_ADV_MT_FMTS,
        },
    },
    {
        .name = MT_SOC_CODEC_HP_IMPEDANCE_NAME,
        .ops = &mt6323_aif1_dai_ops,
        .playback = {
            .stream_name = MT_SOC_HP_IMPEDANCE_STREAM_NAME,
            .channels_min = 1,
            .channels_max = 2,
            .rates = SNDRV_PCM_RATE_8000_48000,
            .formats = SND_SOC_ADV_MT_FMTS,
        },
    },
};


uint32 GetDLNewIFFrequency(unsigned int frequency)
{
    uint32 Reg_value = 0;
    //printk("AudioPlatformDevice ApplyDLNewIFFrequency ApplyDLNewIFFrequency = %d", frequency);
    switch (frequency)
    {
        case 8000:
            Reg_value = 0;
            break;
        case 11025:
            Reg_value = 1;
            break;
        case 12000:
            Reg_value = 2;
            break;
        case 16000:
            Reg_value = 3;
            break;
        case 22050:
            Reg_value = 4;
            break;
        case 24000:
            Reg_value = 5;
            break;
        case 32000:
            Reg_value = 6;
            break;
        case 44100:
            Reg_value = 7;
            break;
        case 48000:
            Reg_value = 8;
        default:
            printk("ApplyDLNewIFFrequency with frequency = %d", frequency);
    }
    return Reg_value;
}

static void TurnOnDacPower(void)
{
    printk("TurnOnDacPower\n");
    audckbufEnable(true);
    ClsqEnable(true);
    Topck_Enable(true);
    udelay(250);
    NvregEnable(true); //K2 moved to 0x0CEE
#if 1 //for bringup, enable all
    Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0000, 0xffff);
#else
    if (GetAdcStatus() == false)
    {
        Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x003a, 0xffff);   //Audio system digital clock power down release
    }
    else
    {
        Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0000, 0xffff);   //Audio system digital clock power down release
    }
#endif

    //set digital part
    Ana_Set_Reg(AFUNC_AUD_CON2, 0x0006, 0xffff); //sdm audio fifo clock power on
    Ana_Set_Reg(AFUNC_AUD_CON0, 0xc3a1, 0xffff); //scrambler clock on enable
    Ana_Set_Reg(AFUNC_AUD_CON2, 0x0003, 0xffff); //sdm power on
    Ana_Set_Reg(AFUNC_AUD_CON2, 0x000b, 0xffff); //sdm fifo enable
    Ana_Set_Reg(AFE_DL_SDM_CON1, 0x001e, 0xffff); //set attenuation gain
    Ana_Set_Reg(AFE_UL_DL_CON0 , 0x0001, 0xffff); //[0] afe enable

    Ana_Set_Reg(AFE_PMIC_NEWIF_CFG0 , GetDLNewIFFrequency(mBlockSampleRate[AUDIO_ANALOG_DEVICE_OUT_DAC]) << 12 | 0x330 , 0xffff);
    Ana_Set_Reg(AFE_DL_SRC2_CON0_H , GetDLNewIFFrequency(mBlockSampleRate[AUDIO_ANALOG_DEVICE_OUT_DAC]) << 12 | 0x300 , 0xffff); //K2

    Ana_Set_Reg(AFE_DL_SRC2_CON0_L , 0x0001 , 0xffff); //turn on dl
    Ana_Set_Reg(PMIC_AFE_TOP_CON0 , 0x0000 , 0xffff); //set DL in normal path, not from sine gen table
}

static void TurnOffDacPower(void)
{
    printk("TurnOffDacPower\n");

    Ana_Set_Reg(AFE_DL_SRC2_CON0_L , 0x0000 , 0xffff); //bit0, Turn off down-link
    if (GetAdcStatus() == false)
    {
        Ana_Set_Reg(AFE_UL_DL_CON0, 0x0000, 0xffff);   //turn off afe
    }
    udelay(250);

    Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0000, 0x0040); //down-link power down

    Ana_Set_Reg(AUDNCP_CLKDIV_CON0, 0x0000, 0xffff); //Toggle RG_DIVCKS_CHG
    Ana_Set_Reg(AUDNCP_CLKDIV_CON1, 0x0000, 0xffff); //Turn off DA_600K_NCP_VA18
    Ana_Set_Reg(AUDNCP_CLKDIV_CON3, 0x0001, 0xffff); // Disable NCP
    NvregEnable(false);
    ClsqEnable(false);
    Topck_Enable(false);
    audckbufEnable(false);
}

static void Audio_Amp_Change(int channels , bool enable)
{
    if (enable)
    {
        if (GetDLStatus() == false)
        {
            TurnOnDacPower();
        }
        // here pmic analog control
        if (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTL] == false &&
            mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTR] == false)
        {
            printk("%s \n", __func__);

            //set analog part (HP playback)
            Ana_Set_Reg(AUDNCP_CLKDIV_CON1, 0x0001, 0xffff); //Turn on DA_600K_NCP_VA18
            Ana_Set_Reg(AUDNCP_CLKDIV_CON2, 0x002B, 0xffff); //Set NCP clock as 604kHz // 26MHz/43 = 604KHz
            Ana_Set_Reg(AUDNCP_CLKDIV_CON0, 0x0001, 0xffff); //Toggle RG_DIVCKS_CHG
            Ana_Set_Reg(AUDNCP_CLKDIV_CON4, 0x0000, 0xffff); //Set NCP soft start mode as default mode
            Ana_Set_Reg(AUDNCP_CLKDIV_CON3, 0x0000, 0xffff); //Enable NCP
            Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0A41, 0xfeeb); //Enable cap-less HC LDO (1.5V) & LDO VA33REFGEN
            Ana_Set_Reg(AUDDEC_ANA_CON6, 0x2AC1, 0xfeeb); //Enable cap-less LC LDO (1.5V)
            Ana_Set_Reg(AUDDEC_ANA_CON7, 0x8000, 0x8000); //Enable NV regulator (-1.5V)
            Ana_Set_Reg(ZCD_CON0, 0x0000, 0xffff); //Disable AUD_ZCD
            Ana_Set_Reg(AUDDEC_ANA_CON0, 0xE080, 0xffff); //Disable headphone, voice and short-ckt protection. HP MUX is opened, voice MUX is set mute
            Ana_Set_Reg(AUDDEC_ANA_CON6, 0x2AC0, 0xfeeb); //Enable IBIST
            Ana_Set_Reg(AUDDEC_ANA_CON4, 0x0700, 0xffff); //Enable HP & HS drivers bias circuit
            Ana_Set_Reg(AUDDEC_ANA_CON5, 0x5490, 0xffff); //HP/HS ibias & DR bias current optimization
            udelay(50);
            Ana_Set_Reg(ZCD_CON2, 0x0F9F, 0xffff); //Set HPR/HPL gain as minimum (~ -40dB)
            Ana_Set_Reg(ZCD_CON3, 0x001F, 0xffff); //Set voice gain as minimum (~ -40dB)
            Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0480, 0xffff); //De_OSC of HP and enable output STBENH
            Ana_Set_Reg(AUDDEC_ANA_CON1, 0x1480, 0xffff); //De_OSC of voice, enable output STBENH
            Ana_Set_Reg(AUDDEC_ANA_CON0, 0xE090, 0xffff); //Enable voice driver
            Ana_Set_Reg(AUDDEC_ANA_CON1, 0x14A0, 0xffff); //Enable pre-charge buffer
            udelay(50);
            Ana_Set_Reg(AUDDEC_ANA_CON6, 0x2AC2, 0xfeeb); //Enable AUD_CLK
            Ana_Set_Reg(AUDDEC_ANA_CON0, 0xE09F, 0xffff); //Enable Audio DAC

            //Apply digital DC compensation value to DAC
            Ana_Set_Reg(AUDDEC_ANA_CON0, 0xF49F, 0xffff); //Switch HP MUX to audio DAC
            // here may cause pop
            //msleep(1); //K2 removed
            Ana_Set_Reg(AUDDEC_ANA_CON0, 0xF4FF, 0xffff); //Enable HPR/HPL
            udelay(50);
            Ana_Set_Reg(AUDDEC_ANA_CON1, 0x1480, 0xffff); //Disable pre-charge buffer
            Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0480, 0xffff); //Disable De_OSC of voice
            Ana_Set_Reg(AUDDEC_ANA_CON0, 0xF46F, 0xffff); //Disable voice buffer & Open HS input MUX
            Ana_Set_Reg(AUDDEC_ANA_CON4, 0x0300, 0xffff); //Disable HS drivers bias circuit
            Ana_Set_Reg(ZCD_CON2, 0x0489, 0xffff); //Set HPR/HPL gain to 0dB, step by step

            // apply volume setting
            HeadsetRVolumeSet();//K2 mark temporarily for SMT
            HeadsetLVolumeSet();//K2 mark temporarily for SMT
        }
        else if (channels == AUDIO_ANALOG_CHANNELS_LEFT1)
        {
            //K2 TODO:
            //Ana_Set_Reg(AUDDAC_CFG0, 0x000f, 0xffff); // enable audio bias. enable audio DAC, HP buffers
        }
        else if (channels == AUDIO_ANALOG_CHANNELS_RIGHT1)
        {
            //K2 TODO:
            //Ana_Set_Reg(AUDDAC_CFG0, 0x000f, 0xffff); // enable audio bias. enable audio DAC, HP buffers
        }
    }
    else
    {
        if (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTL] == false &&
            mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTR] == false)
        {
            printk("Audio_Amp_Change off amp\n");
            Ana_Set_Reg(AUDDEC_ANA_CON0, 0xF40F, 0xffff); //Disable HPR/HPL
        }
        else if (channels == AUDIO_ANALOG_CHANNELS_LEFT1)
        {
            //K2 TODO:
            //Ana_Set_Reg(AUDDAC_CFG0, 0x000e, 0xffff); // enable audio bias. enable audio DAC, HP buffers
        }
        else if (channels == AUDIO_ANALOG_CHANNELS_RIGHT1)
        {
            //K2 TODO:
            //Ana_Set_Reg(AUDDAC_CFG0, 0x000d, 0xffff); // enable audio bias. enable audio DAC, HP buffers
        }
        if (GetDLStatus() == false)
        {
            Ana_Set_Reg(AUDDEC_ANA_CON4, 0x0000, 0xffff); //Disable drivers bias circuit
            Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0000, 0xffff); //Disable Audio DAC
            Ana_Set_Reg(AUDDEC_ANA_CON6, 0x2AC0, 0xfeeb); //Disable AUD_CLK, bit2/4/8 is for ADC, do not set
            Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0000, 0x8000); //Disable NV regulator (-1.5V)
            Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0001, 0xfeeb); //Disable cap-less LDOs (1.5V) & Disable IBIST

            TurnOffDacPower();
        }
    }
}

static int Audio_AmpL_Get(struct snd_kcontrol *kcontrol,
                          struct snd_ctl_elem_value *ucontrol)
{
    printk("Audio_AmpL_Get = %d\n", mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTL]);
    ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTL];
    return 0;
}

static int Audio_AmpL_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    mutex_lock(&Ana_Ctrl_Mutex);

    printk("%s() gain = %ld \n ", __func__, ucontrol->value.integer.value[0]);
    if ((ucontrol->value.integer.value[0]  == true) && (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTL]  == false))
    {
        Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_LEFT1 , true);
        mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTL] = ucontrol->value.integer.value[0];
    }
    else if ((ucontrol->value.integer.value[0]  == false) && (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTL]  == true))
    {
        mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTL] = ucontrol->value.integer.value[0];
        Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_LEFT1 , false);
    }
    mutex_unlock(&Ana_Ctrl_Mutex);
    return 0;
}

static int Audio_AmpR_Get(struct snd_kcontrol *kcontrol,
                          struct snd_ctl_elem_value *ucontrol)
{
    printk("Audio_AmpR_Get = %d\n", mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTR]);
    ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTR];
    return 0;
}

static int Audio_AmpR_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    mutex_lock(&Ana_Ctrl_Mutex);

    printk("%s()\n", __func__);
    if ((ucontrol->value.integer.value[0]  == true) && (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTR]  == false))
    {
        Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_RIGHT1 , true);
        mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTR] = ucontrol->value.integer.value[0];
    }
    else if ((ucontrol->value.integer.value[0]  == false) && (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTR]  == true))
    {
        mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTR] = ucontrol->value.integer.value[0];
        Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_RIGHT1 , false);
    }
    mutex_unlock(&Ana_Ctrl_Mutex);
    return 0;
}

static void  SetVoiceAmpVolume(void)
{
    int index;
    printk("%s\n", __func__);
    index =  mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HSOUTL];
    Ana_Set_Reg(ZCD_CON3, index , 0x001f);
}

static void Voice_Amp_Change(bool enable)
{
    if (enable)
    {
        printk("%s \n", __func__);
        if (GetDLStatus() == false)
        {
            //Ana_Set_Reg(TOP_CKSEL_CON_CLR, 0x0001, 0x0001); //use internal 26M,  TODO: K2 needed?
            TurnOnDacPower();
            printk("Voice_Amp_Change on amp\n");

            //set analog part (voice HS playback)
            Ana_Set_Reg(AUDNCP_CLKDIV_CON1, 0x0001, 0xffff); //Turn on DA_600K_NCP_VA18
            Ana_Set_Reg(AUDNCP_CLKDIV_CON2, 0x002B, 0xffff); //Set NCP clock as 604kHz // 26MHz/43 = 604KHz
            Ana_Set_Reg(AUDNCP_CLKDIV_CON0, 0x0001, 0xffff); //Toggle RG_DIVCKS_CHG
            Ana_Set_Reg(AUDNCP_CLKDIV_CON4, 0x0000, 0xffff); //Set NCP soft start mode as default mode
            Ana_Set_Reg(AUDNCP_CLKDIV_CON3, 0x0000, 0xffff); //Enable NCP
            Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0A40, 0xfeeb); //Enable cap-less HC LDO (1.5V)
            Ana_Set_Reg(AUDDEC_ANA_CON6, 0x2AC0, 0xfeeb); //Enable cap-less LC LDO (1.5V)
            Ana_Set_Reg(AUDDEC_ANA_CON7, 0x8000, 0x8000); //Enable NV regulator (-1.5V)
            Ana_Set_Reg(ZCD_CON0, 0x0000, 0xffff); //Disable AUD_ZCD
            Ana_Set_Reg(AUDDEC_ANA_CON0, 0xE080, 0xffff); //Disable headphone, voice and short-ckt protection. HP MUX is opened, voice MUX is set mute
            Ana_Set_Reg(AUDDEC_ANA_CON6, 0x2AC0, 0xfeeb); //Enable IBIST
            Ana_Set_Reg(AUDDEC_ANA_CON4, 0x0400, 0xffff); //Enable HS drivers bias circuit
            Ana_Set_Reg(AUDDEC_ANA_CON5, 0x5490, 0xffff); //HP/HS ibias & DR bias current optimization
            udelay(50);
            Ana_Set_Reg(ZCD_CON3, 0x001F, 0xffff); //Set voice gain as minimum (~ -40dB)
            Ana_Set_Reg(AUDDEC_ANA_CON1, 0x1000, 0xffff); //De_OSC of voice, enable output STBENH
            //Ana_Set_Reg(AUDDEC_ANA_CON6, 0x2AC2, 0xfeeb); //Enable AUD_CLK
            Ana_Set_Reg(AUDDEC_ANA_CON6, 0x2BC6, 0xffff); //Enable AUD_CLK //ccc bringup temp
            Ana_Set_Reg(AUDDEC_ANA_CON0, 0xE089, 0xffff); //Enable Audio DAC

            //Apply digital DC compensation value to DAC
            Ana_Set_Reg(AUDDEC_ANA_CON0, 0xE109, 0xffff); //Switch HS MUX to audio DAC
            Ana_Set_Reg(AUDDEC_ANA_CON0, 0xE119, 0xffff); //Enable voice driver
            udelay(50);
            Ana_Set_Reg(ZCD_CON3, 0x0000, 0xffff); //Set HS gain to +8dB(for SMT), step by step
        }
    }
    else
    {
        printk("turn off ampL\n");
        Ana_Set_Reg(AUDDEC_ANA_CON0,  0xE109 , 0xffff); //Disable voice driver

        if (GetDLStatus() == false)
        {
            Ana_Set_Reg(AUDDEC_ANA_CON4, 0x0000, 0xffff); //Disable drivers bias circuit
            Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0000, 0xffff); //Disable Audio DAC
            Ana_Set_Reg(AUDDEC_ANA_CON6, 0x2AC0, 0xfeeb); //Disable AUD_CLK, bit2/4/8 is for ADC, do not set
            Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0000, 0x8000); //Disable NV regulator (-1.5V)
            Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0001, 0xfeeb); //Disable cap-less LDOs (1.5V) & Disable IBIST

            TurnOffDacPower();
        }
    }
}

static int Voice_Amp_Get(struct snd_kcontrol *kcontrol,
                         struct snd_ctl_elem_value *ucontrol)
{
    printk("Voice_Amp_Get = %d\n", mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HSOUTL]);
    ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HSOUTL];
    return 0;
}

static int Voice_Amp_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    mutex_lock(&Ana_Ctrl_Mutex);
    printk("%s()\n", __func__);
    if ((ucontrol->value.integer.value[0]  == true) && (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HSOUTL]  == false))
    {
        Voice_Amp_Change(true);
        mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HSOUTL] = ucontrol->value.integer.value[0];
    }
    else if ((ucontrol->value.integer.value[0]  == false) && (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HSOUTL]  == true))
    {
        mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HSOUTL] = ucontrol->value.integer.value[0];
        Voice_Amp_Change(false);
    }
    mutex_unlock(&Ana_Ctrl_Mutex);
    return 0;
}

static void Speaker_Amp_Change(bool enable)
{
    if (enable)
    {
        if (GetDLStatus() == false)
        {
            TurnOnDacPower();
        }
        printk("%s \n", __func__);
        // ClassAB spk pmic analog control
        Ana_Set_Reg(AUDNCP_CLKDIV_CON1, 0x0001, 0xffff); //Turn on DA_600K_NCP_VA18
        Ana_Set_Reg(AUDNCP_CLKDIV_CON2, 0x002B, 0xffff); //Set NCP clock as 604kHz // 26MHz/43 = 604KHz
        Ana_Set_Reg(AUDNCP_CLKDIV_CON0, 0x0001, 0xffff); //Toggle RG_DIVCKS_CHG
        Ana_Set_Reg(AUDNCP_CLKDIV_CON4, 0x0000, 0xffff); //Set NCP soft start mode as default mode
        Ana_Set_Reg(AUDNCP_CLKDIV_CON3, 0x0000, 0xffff); //Enable NCP
        Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0A41, 0xfeeb); //Enable cap-less HC LDO (1.5V)
        Ana_Set_Reg(AUDDEC_ANA_CON6, 0x2AC1, 0xfeeb); //Enable cap-less LC LDO (1.5V)
        Ana_Set_Reg(AUDDEC_ANA_CON7, 0x8000, 0x8000); //Enable NV regulator (-1.5V)
        Ana_Set_Reg(ZCD_CON0, 0x0000, 0xffff); //Disable AUD_ZCD

        Ana_Set_Reg(AUDDEC_ANA_CON6, 0x2AC0, 0xfeeb); //Enable IBIST
        Ana_Set_Reg(AUDDEC_ANA_CON6, 0x2AC2, 0xfeeb); //Enable AUD_CLK
        Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0009, 0x0009); //Enable Audio DAC Lch

        Ana_Set_Reg(ZCD_CON0, 0x0400, 0xffff); //Disable IVBUF_ZCD
        Ana_Set_Reg(ZCD_CON4, 0x0705, 0xffff); //Set 0dB IV buffer gain
        Ana_Set_Reg(SPK_ANA_CON3, 0x0100, 0xffff); //Set IV buffer MUX select
        Ana_Set_Reg(SPK_ANA_CON3, 0x0110, 0xffff); //Enable IV buffer
        Ana_Set_Reg(TOP_CKPDN_CON2_CLR, 0x0003, 0xffff); //Enable ClassAB/D clock

#ifdef CONFIG_MTK_SPEAKER
        if (Speaker_mode == AUDIO_SPEAKER_MODE_D)
        {
            Speaker_ClassD_Open();
        }
        else if (Speaker_mode == AUDIO_SPEAKER_MODE_AB)
        {
            Speaker_ClassAB_Open();
        }
        else if (Speaker_mode == AUDIO_SPEAKER_MODE_RECEIVER)
        {
            Speaker_ReveiverMode_Open();
        }
#endif
        Apply_Speaker_Gain();
    }
    else
    {
        printk("turn off Speaker_Amp_Change \n");
#ifdef CONFIG_MTK_SPEAKER
        if (Speaker_mode == AUDIO_SPEAKER_MODE_D)
        {
            Speaker_ClassD_close();
        }
        else if (Speaker_mode == AUDIO_SPEAKER_MODE_AB)
        {
            Speaker_ClassAB_close();
        }
        else if (Speaker_mode == AUDIO_SPEAKER_MODE_RECEIVER)
        {
            Speaker_ReveiverMode_close();
        }
#endif
        Ana_Set_Reg(TOP_CKPDN_CON2_SET, 0x0003, 0xffff); //Disable ClassAB/D clock
        Ana_Set_Reg(SPK_ANA_CON3, 0x0, 0xffff); //Disable IV buffer, Set IV buffer MUX select open/open
        Ana_Set_Reg(ZCD_CON4, 0x0707, 0xffff); //Set min -2dB IV buffer gain

        if (GetDLStatus() == false)
        {
            Ana_Set_Reg(AUDDEC_ANA_CON4, 0x0000, 0xffff); //Disable drivers bias circuit
            Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0000, 0xffff); //Disable Audio DAC
            Ana_Set_Reg(AUDDEC_ANA_CON6, 0x2AC0, 0xfeeb); //Disable AUD_CLK, bit2/4/8 is for ADC, do not set
            Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0000, 0x8000); //Disable NV regulator (-1.5V)
            Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0001, 0xfeeb); //Disable cap-less LDOs (1.5V) & Disable IBIST

            TurnOffDacPower();
        }
    }
}

static int Speaker_Amp_Get(struct snd_kcontrol *kcontrol,
                           struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()\n", __func__);
    ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_SPKL] ;
    return 0;
}

static int Speaker_Amp_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    printk("%s() value = %ld \n ", __func__, ucontrol->value.integer.value[0]);
    if ((ucontrol->value.integer.value[0] == true) && (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_SPKL]  == false))
    {
        Speaker_Amp_Change(true);
        mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_SPKL] = ucontrol->value.integer.value[0];
    }
    else if ((ucontrol->value.integer.value[0] == false) && (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_SPKL]  == true))
    {
        mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_SPKL] = ucontrol->value.integer.value[0];
        Speaker_Amp_Change(false);
    }
    return 0;
}

static void Ext_Speaker_Amp_Change(bool enable)
{
#define SPK_WARM_UP_TIME        (10) //unit is ms

    if (enable)
    {
        printk("Ext_Speaker_Amp_Change ON+ \n");
#ifndef CONFIG_MTK_SPEAKER
        printk("Ext_Speaker_Amp_Change ON set GPIO \n");
        mt_set_gpio_mode(GPIO_EXT_SPKAMP_EN_PIN, GPIO_MODE_00); //GPIO117: DPI_D3, mode 0
        mt_set_gpio_pull_enable(GPIO_EXT_SPKAMP_EN_PIN, GPIO_PULL_ENABLE);
        mt_set_gpio_dir(GPIO_EXT_SPKAMP_EN_PIN, GPIO_DIR_OUT); // output
        mt_set_gpio_out(GPIO_EXT_SPKAMP_EN_PIN, GPIO_OUT_ZERO); // low disable
        udelay(1000);
        mt_set_gpio_dir(GPIO_EXT_SPKAMP_EN_PIN, GPIO_DIR_OUT); // output
        mt_set_gpio_out(GPIO_EXT_SPKAMP_EN_PIN, GPIO_OUT_ONE); // high enable
        msleep(SPK_WARM_UP_TIME);
#endif
        printk("Ext_Speaker_Amp_Change ON- \n");
    }
    else
    {
        printk("Ext_Speaker_Amp_Change OFF+ \n");
#ifndef CONFIG_MTK_SPEAKER
        //mt_set_gpio_mode(GPIO_EXT_SPKAMP_EN_PIN, GPIO_MODE_00); //GPIO117: DPI_D3, mode 0
        mt_set_gpio_dir(GPIO_EXT_SPKAMP_EN_PIN, GPIO_DIR_OUT); // output
        mt_set_gpio_out(GPIO_EXT_SPKAMP_EN_PIN, GPIO_OUT_ZERO); // low disbale
        udelay(500);
#endif
        printk("Ext_Speaker_Amp_Change OFF- \n");
    }
}

static int Ext_Speaker_Amp_Get(struct snd_kcontrol *kcontrol,
                               struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()\n", __func__);
    ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_EXTSPKAMP] ;
    return 0;
}

static int Ext_Speaker_Amp_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{

    printk("%s() gain = %ld \n ", __func__, ucontrol->value.integer.value[0]);
    if (ucontrol->value.integer.value[0])
    {
        Ext_Speaker_Amp_Change(true);
        mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_EXTSPKAMP] = ucontrol->value.integer.value[0];
    }
    else
    {
        mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_EXTSPKAMP] = ucontrol->value.integer.value[0];
        Ext_Speaker_Amp_Change(false);
    }
    return 0;
}

static void Headset_Speaker_Amp_Change(bool enable)
{
    if (enable)
    {
        if (GetDLStatus() == false)
        {
            TurnOnDacPower();
        }
        printk("turn on Speaker_Amp_Change \n");
        // here pmic analog control
        //set analog part (HP playback)
        Ana_Set_Reg(AUDNCP_CLKDIV_CON1, 0x0001, 0xffff); //Turn on DA_600K_NCP_VA18
        Ana_Set_Reg(AUDNCP_CLKDIV_CON2, 0x002B, 0xffff); //Set NCP clock as 604kHz // 26MHz/43 = 604KHz
        Ana_Set_Reg(AUDNCP_CLKDIV_CON0, 0x0001, 0xffff); //Toggle RG_DIVCKS_CHG
        Ana_Set_Reg(AUDNCP_CLKDIV_CON4, 0x0000, 0xffff); //Set NCP soft start mode as default mode
        Ana_Set_Reg(AUDNCP_CLKDIV_CON3, 0x0000, 0xffff); //Enable NCP
        Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0A41, 0xfeeb); //Enable cap-less HC LDO (1.5V) & LDO VA33REFGEN
        Ana_Set_Reg(AUDDEC_ANA_CON6, 0x2AC1, 0xfeeb); //Enable cap-less LC LDO (1.5V)
        Ana_Set_Reg(AUDDEC_ANA_CON7, 0x8000, 0x8000); //Enable NV regulator (-1.5V)
        Ana_Set_Reg(ZCD_CON0, 0x0000, 0xffff); //Disable AUD_ZCD
        Ana_Set_Reg(AUDDEC_ANA_CON0, 0xE080, 0xffff); //Disable headphone, voice and short-ckt protection. HP MUX is opened, voice MUX is set mute
        Ana_Set_Reg(AUDDEC_ANA_CON6, 0x2AC0, 0xfeeb); //Enable IBIST
        Ana_Set_Reg(AUDDEC_ANA_CON4, 0x0700, 0xffff); //Enable HP & HS drivers bias circuit
        Ana_Set_Reg(AUDDEC_ANA_CON5, 0x5490, 0xffff); //HP/HS ibias & DR bias current optimization
        udelay(50);
        Ana_Set_Reg(ZCD_CON2, 0x0F9F, 0xffff); //Set HPR/HPL gain as minimum (~ -40dB)
        Ana_Set_Reg(ZCD_CON3, 0x001F, 0xffff); //Set voice gain as minimum (~ -40dB)
        Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0480, 0xffff); //De_OSC of HP and enable output STBENH
        Ana_Set_Reg(AUDDEC_ANA_CON1, 0x1480, 0xffff); //De_OSC of voice, enable output STBENH
        Ana_Set_Reg(AUDDEC_ANA_CON0, 0xE090, 0xffff); //Enable voice driver
        Ana_Set_Reg(AUDDEC_ANA_CON1, 0x14A0, 0xffff); //Enable pre-charge buffer
        udelay(50);
        Ana_Set_Reg(AUDDEC_ANA_CON6, 0x2AC2, 0xfeeb); //Enable AUD_CLK
        Ana_Set_Reg(AUDDEC_ANA_CON0, 0xE09F, 0xffff); //Enable Audio DAC

        //Apply digital DC compensation value to DAC
        Ana_Set_Reg(AUDDEC_ANA_CON0, 0xF29F, 0xffff); //R hp input mux select "Audio playback", L hp input mux select "LoudSPK playback"
        // here may cause pop
        //msleep(1); //K2 removed
        Ana_Set_Reg(AUDDEC_ANA_CON0, 0xF2FF, 0xffff); //Enable HPR/HPL
        udelay(50);
        Ana_Set_Reg(AUDDEC_ANA_CON1, 0x1480, 0xffff); //Disable pre-charge buffer
        Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0480, 0xffff); //Disable De_OSC of voice
        Ana_Set_Reg(AUDDEC_ANA_CON0, 0xF26F, 0xffff); //Disable voice buffer & Open HS input MUX
        Ana_Set_Reg(AUDDEC_ANA_CON4, 0x0300, 0xffff); //Disable HS drivers bias circuit
        Ana_Set_Reg(ZCD_CON2, 0x0489, 0xffff); //Set HPR/HPL gain to 0dB, step by step

        // ClassAB spk pmic analog control

        Ana_Set_Reg(ZCD_CON0, 0x0400, 0xffff); //Disable IVBUF_ZCD
        Ana_Set_Reg(ZCD_CON4, 0x0705, 0xffff); //Set 0dB IV buffer gain
        Ana_Set_Reg(SPK_ANA_CON3, 0x0100, 0xffff); //Set IV buffer MUX select
        Ana_Set_Reg(SPK_ANA_CON3, 0x0110, 0xffff); //Enable IV buffer
        Ana_Set_Reg(TOP_CKPDN_CON2_CLR, 0x0003, 0xffff); //Enable ClassAB/D clock

#ifdef CONFIG_MTK_SPEAKER
        if (Speaker_mode == AUDIO_SPEAKER_MODE_D)
        {
            Speaker_ClassD_Open();
        }
        else if (Speaker_mode == AUDIO_SPEAKER_MODE_AB)
        {
            Speaker_ClassAB_Open();
        }
        else if (Speaker_mode == AUDIO_SPEAKER_MODE_RECEIVER)
        {
            Speaker_ReveiverMode_Open();
        }
#endif
        // apply volume setting
        HeadsetRVolumeSet();
        HeadsetLVolumeSet();
        Apply_Speaker_Gain();
    }
    else
    {
#ifdef CONFIG_MTK_SPEAKER
        if (Speaker_mode == AUDIO_SPEAKER_MODE_D)
        {
            Speaker_ClassD_close();
        }
        else if (Speaker_mode == AUDIO_SPEAKER_MODE_AB)
        {
            Speaker_ClassAB_close();
        }
        else if (Speaker_mode == AUDIO_SPEAKER_MODE_RECEIVER)
        {
            Speaker_ReveiverMode_close();
        }
#endif
        Ana_Set_Reg(TOP_CKPDN_CON2_SET, 0x0003, 0xffff); //Disable ClassAB/D clock
        Ana_Set_Reg(SPK_ANA_CON3, 0x0, 0xffff); //Disable IV buffer, Set IV buffer MUX select open/open
        Ana_Set_Reg(ZCD_CON4, 0x0707, 0xffff); //Set min -2dB IV buffer gain

        Ana_Set_Reg(AUDDEC_ANA_CON0, 0xF20F, 0xffff); //Disable HPR/HPL

        if (GetDLStatus() == false)
        {
            Ana_Set_Reg(AUDDEC_ANA_CON4, 0x0000, 0xffff); //Disable drivers bias circuit
            Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0000, 0xffff); //Disable Audio DAC
            Ana_Set_Reg(AUDDEC_ANA_CON6, 0x2AC0, 0xfeeb); //Disable AUD_CLK, bit2/4/8 is for ADC, do not set
            Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0000, 0x8000); //Disable NV regulator (-1.5V)
            Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0001, 0xfeeb); //Disable cap-less LDOs (1.5V) & Disable IBIST

            TurnOffDacPower();
        }
    }
}


static int Headset_Speaker_Amp_Get(struct snd_kcontrol *kcontrol,
                                   struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()\n", __func__);
    ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_SPEAKER_HEADSET_R] ;
    return 0;
}

static int Headset_Speaker_Amp_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    //struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

    printk("%s() gain = %lu \n ", __func__, ucontrol->value.integer.value[0]);
    if ((ucontrol->value.integer.value[0]  == true) && (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_SPEAKER_HEADSET_R]  == false))
    {
        Headset_Speaker_Amp_Change(true);
        mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_SPEAKER_HEADSET_R] = ucontrol->value.integer.value[0];
    }
    else if ((ucontrol->value.integer.value[0]  == false) && (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_SPEAKER_HEADSET_R]  == true))
    {
        mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_SPEAKER_HEADSET_R] = ucontrol->value.integer.value[0];
        Headset_Speaker_Amp_Change(false);
    }
    return 0;
}

#ifdef CONFIG_MTK_SPEAKER
static const char *speaker_amp_function[] = {"CALSSD", "CLASSAB", "RECEIVER"};
static const char *speaker_PGA_function[] = {"MUTE", "0Db", "4Db", "5Db", "6Db", "7Db", "8Db", "9Db", "10Db",
                                             "11Db", "12Db", "13Db", "14Db", "15Db", "16Db", "17Db"
                                            };
static const char *speaker_OC_function[] = {"Off", "On"};
static const char *speaker_CS_function[] = {"Off", "On"};
static const char *speaker_CSPeakDetecReset_function[] = {"Off", "On"};

static int Audio_Speaker_Class_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    mutex_lock(&Ana_Ctrl_Mutex);
    Speaker_mode = ucontrol->value.integer.value[0];
    mutex_unlock(&Ana_Ctrl_Mutex);
    return 0;
}

static int Audio_Speaker_Class_Get(struct snd_kcontrol *kcontrol,
                                   struct snd_ctl_elem_value *ucontrol)
{
    ucontrol->value.integer.value[0] = Speaker_mode ;
    return 0;
}

static int Audio_Speaker_Pga_Gain_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    Speaker_pga_gain = ucontrol->value.integer.value[0];

    printk("%s Speaker_pga_gain= %d\n", __func__, Speaker_pga_gain);
    Ana_Set_Reg(SPK_ANA_CON0,  Speaker_pga_gain << 11, 0x7800);
    return 0;
}

static int Audio_Speaker_OcFlag_Get(struct snd_kcontrol *kcontrol,
                                    struct snd_ctl_elem_value *ucontrol)
{
    mSpeaker_Ocflag =  GetSpeakerOcFlag();
    ucontrol->value.integer.value[0] = mSpeaker_Ocflag ;
    return 0;
}

static int Audio_Speaker_OcFlag_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    printk("%s is not support setting \n", __func__);
    return 0;
}

static int Audio_Speaker_Pga_Gain_Get(struct snd_kcontrol *kcontrol,
                                      struct snd_ctl_elem_value *ucontrol)
{
    ucontrol->value.integer.value[0] = Speaker_pga_gain ;
    return 0;
}

static int Audio_Speaker_Current_Sensing_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{

    if (ucontrol->value.integer.value[0])
    {
        Ana_Set_Reg(SPK_CON12,  0x9300, 0xff00);
    }
    else
    {
        Ana_Set_Reg(SPK_CON12,  0x1300, 0xff00);
    }
    return 0;
}

static int Audio_Speaker_Current_Sensing_Get(struct snd_kcontrol *kcontrol,
                                             struct snd_ctl_elem_value *ucontrol)
{
    ucontrol->value.integer.value[0] = (Ana_Get_Reg(SPK_CON12) >> 15) & 0x01;
    return 0;
}

static int Audio_Speaker_Current_Sensing_Peak_Detector_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{

    if (ucontrol->value.integer.value[0])
    {
        Ana_Set_Reg(SPK_CON12,  1 << 14, 1 << 14);
    }
    else
    {
        Ana_Set_Reg(SPK_CON12,  0, 1 << 14);
    }
    return 0;
}

static int Audio_Speaker_Current_Sensing_Peak_Detector_Get(struct snd_kcontrol *kcontrol,
                                                           struct snd_ctl_elem_value *ucontrol)
{
    ucontrol->value.integer.value[0] = (Ana_Get_Reg(SPK_CON12) >> 14) & 0x01;
    return 0;
}


static const struct soc_enum Audio_Speaker_Enum[] =
{
    // speaker class setting
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(speaker_amp_function), speaker_amp_function),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(speaker_PGA_function), speaker_PGA_function),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(speaker_OC_function), speaker_OC_function),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(speaker_CS_function), speaker_CS_function),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(speaker_CSPeakDetecReset_function), speaker_CSPeakDetecReset_function),
};

static const struct snd_kcontrol_new mt6331_snd_Speaker_controls[] =
{
    SOC_ENUM_EXT("Audio_Speaker_class_Switch", Audio_Speaker_Enum[0], Audio_Speaker_Class_Get, Audio_Speaker_Class_Set),
    SOC_ENUM_EXT("Audio_Speaker_PGA_gain", Audio_Speaker_Enum[1], Audio_Speaker_Pga_Gain_Get, Audio_Speaker_Pga_Gain_Set),
    SOC_ENUM_EXT("Audio_Speaker_OC_Falg", Audio_Speaker_Enum[2], Audio_Speaker_OcFlag_Get, Audio_Speaker_OcFlag_Set),
    SOC_ENUM_EXT("Audio_Speaker_CurrentSensing", Audio_Speaker_Enum[3], Audio_Speaker_Current_Sensing_Get, Audio_Speaker_Current_Sensing_Set),
    SOC_ENUM_EXT("Audio_Speaker_CurrentPeakDetector", Audio_Speaker_Enum[4], Audio_Speaker_Current_Sensing_Peak_Detector_Get, Audio_Speaker_Current_Sensing_Peak_Detector_Set),
};

int PMIC_IMM_GetOneChannelValue(int dwChannel, int deCount, int trimd);

int Audio_AuxAdcData_Get_ext(void)
{
    int dRetValue = PMIC_IMM_GetOneChannelValue(8, 1, 0);
    printk("%s dRetValue 0x%x \n", __func__, dRetValue);
    return dRetValue;
}


#endif

static int Audio_AuxAdcData_Get(struct snd_kcontrol *kcontrol,
                                struct snd_ctl_elem_value *ucontrol)
{

#ifdef CONFIG_MTK_SPEAKER
    ucontrol->value.integer.value[0] = Audio_AuxAdcData_Get_ext();//PMIC_IMM_GetSPK_THR_IOneChannelValue(0x001B, 1, 0);
#else
    ucontrol->value.integer.value[0] = 0;
#endif
    printk("%s dMax = 0x%lx \n", __func__, ucontrol->value.integer.value[0]);
    return 0;

}

static int Audio_AuxAdcData_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    dAuxAdcChannel = ucontrol->value.integer.value[0];
    printk("%s dAuxAdcChannel = 0x%x \n", __func__, dAuxAdcChannel);
    return 0;
}


static const struct snd_kcontrol_new Audio_snd_auxadc_controls[] =
{
    SOC_SINGLE_EXT("Audio AUXADC Data", SND_SOC_NOPM, 0, 0x80000, 0, Audio_AuxAdcData_Get, Audio_AuxAdcData_Set),
};


static const char *amp_function[] = {"Off", "On"};
static const char *aud_clk_buf_function[] = {"Off", "On"};

//static const char *DAC_SampleRate_function[] = {"8000", "11025", "16000", "24000", "32000", "44100", "48000"};
static const char *DAC_DL_PGA_Headset_GAIN[] = {"8Db", "7Db", "6Db", "5Db", "4Db", "3Db", "2Db", "1Db", "0Db", "-1Db", "-2Db", "-3Db",
                                                "-4Db", "-5Db", "-6Db", "-7Db", "-8Db", "-9Db", "-10Db" , "-40Db"
                                               };
static const char *DAC_DL_PGA_Handset_GAIN[] = {"8Db", "7Db", "6Db", "5Db", "4Db", "3Db", "2Db", "1Db", "0Db", "-1Db", "-2Db", "-3Db",
                                                "-4Db", "-5Db", "-6Db", "-7Db", "-8Db", "-9Db", "-10Db" , "-40Db"
                                               };

static const char *DAC_DL_PGA_Speaker_GAIN[] = {"8Db", "7Db", "6Db", "5Db", "4Db", "3Db", "2Db", "1Db", "0Db", "-1Db", "-2Db", "-3Db",
                                                "-4Db", "-5Db", "-6Db", "-7Db", "-8Db", "-9Db", "-10Db" , "-40Db"
                                               };

static const char *Voice_Mux_function[] = {"Voice", "Speaker"};

static int Lineout_PGAL_Get(struct snd_kcontrol *kcontrol,
                            struct snd_ctl_elem_value *ucontrol)
{
    printk("Speaker_PGA_Get = %d\n", mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_SPKL]);
    ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_SPKL];
    return 0;
}

static int Lineout_PGAL_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    int index = 0;
    printk("%s(), index = %d\n", __func__, ucontrol->value.enumerated.item[0]);

    if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(DAC_DL_PGA_Speaker_GAIN))
    {
        printk("return -EINVAL\n");
        return -EINVAL;
    }
    index = ucontrol->value.integer.value[0];
    if (ucontrol->value.enumerated.item[0] == (ARRAY_SIZE(DAC_DL_PGA_Speaker_GAIN) - 1))
    {
        index = 0x1f;
    }
    Ana_Set_Reg(ZCD_CON1, index , 0x001f);
    mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_SPKL] = ucontrol->value.integer.value[0];
    return 0;
}

static int Lineout_PGAR_Get(struct snd_kcontrol *kcontrol,
                            struct snd_ctl_elem_value *ucontrol)
{
    printk("%s  = %d\n", __func__, mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_SPKR]);
    ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_SPKR];
    return 0;
}

static int Lineout_PGAR_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    //    struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
    int index = 0;
    printk("%s(), index = %d\n", __func__, ucontrol->value.enumerated.item[0]);

    if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(DAC_DL_PGA_Speaker_GAIN))
    {
        printk("return -EINVAL\n");
        return -EINVAL;
    }
    index = ucontrol->value.integer.value[0];
    if (ucontrol->value.enumerated.item[0] == (ARRAY_SIZE(DAC_DL_PGA_Speaker_GAIN) - 1))
    {
        index = 0x1f;
    }
    Ana_Set_Reg(ZCD_CON1, index << 7 , 0x0f10);
    mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_SPKR] = ucontrol->value.integer.value[0];
    return 0;
}

static int Handset_PGA_Get(struct snd_kcontrol *kcontrol,
                           struct snd_ctl_elem_value *ucontrol)
{
    printk("Handset_PGA_Get = %d\n", mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HSOUTL]);
    ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HSOUTL];
    return 0;
}

static int Handset_PGA_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    //    struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
    int index = 0;

    printk("%s(), index = %d\n", __func__, ucontrol->value.enumerated.item[0]);

    if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(DAC_DL_PGA_Handset_GAIN))
    {
        printk("return -EINVAL\n");
        return -EINVAL;
    }
    index = ucontrol->value.integer.value[0];
    if (ucontrol->value.enumerated.item[0] == (ARRAY_SIZE(DAC_DL_PGA_Handset_GAIN) - 1))
    {
        index = 0x1f;
    }
    Ana_Set_Reg(ZCD_CON3, index , 0x001f);
    mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HSOUTL] = ucontrol->value.integer.value[0];
    return 0;
}


static void HeadsetLVolumeSet(void)
{
    int index = 0;
    printk("%s\n", __func__);
    index =   mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTL];
    Ana_Set_Reg(ZCD_CON2, index , 0x001f);
}

static int Headset_PGAL_Get(struct snd_kcontrol *kcontrol,
                            struct snd_ctl_elem_value *ucontrol)
{
    printk("Headset_PGAL_Get = %d\n", mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTL]);
    ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTL];
    return 0;
}

static int Headset_PGAL_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    //    struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
    int index = 0;

   // printk("%s(), index = %d arraysize = %d \n", __func__, ucontrol->value.enumerated.item[0], ARRAY_SIZE(DAC_DL_PGA_Headset_GAIN));

    if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(DAC_DL_PGA_Headset_GAIN))
    {
        printk("return -EINVAL\n");
        return -EINVAL;
    }
    index = ucontrol->value.integer.value[0];
    if (ucontrol->value.enumerated.item[0] == (ARRAY_SIZE(DAC_DL_PGA_Headset_GAIN) - 1))
    {
        index = 0x1f;
    }
    Ana_Set_Reg(ZCD_CON2, index , 0x001f);
    mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTL] = ucontrol->value.integer.value[0];
    return 0;
}

static void HeadsetRVolumeSet(void)
{
    int index = 0;
    printk("%s\n", __func__);
    index =   mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTR];
    Ana_Set_Reg(ZCD_CON2, index << 7, 0x0f80);
}

static int Headset_PGAR_Get(struct snd_kcontrol *kcontrol,
                            struct snd_ctl_elem_value *ucontrol)
{
    printk("Headset_PGAR_Get = %d\n", mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTR]);
    ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTR];
    return 0;
}

static int Headset_PGAR_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    //    struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
    int index = 0;

    printk("%s(), index = %d\n", __func__, ucontrol->value.enumerated.item[0]);

    if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(DAC_DL_PGA_Headset_GAIN))
    {
        printk("return -EINVAL\n");
        return -EINVAL;
    }
    index = ucontrol->value.integer.value[0];
    if (ucontrol->value.enumerated.item[0] == (ARRAY_SIZE(DAC_DL_PGA_Headset_GAIN) - 1))
    {
        index = 0x1f;
    }
    Ana_Set_Reg(ZCD_CON2, index << 7, 0x0f80);
    mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTR] = ucontrol->value.integer.value[0];
    return 0;
}


static int Voice_Mux_Get(struct snd_kcontrol *kcontrol,
                         struct snd_ctl_elem_value *ucontrol)
{
    printk("Voice_Mux_Get = %d\n", mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_VOICE]);
    ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_VOICE];
    return 0;
}

static int Voice_Mux_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{

    struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
    printk("%s()\n", __func__);
    if (ucontrol->value.integer.value[0])
    {
        printk("%s()\n", __func__);
        snd_soc_dapm_disable_pin(&codec->dapm, "SPEAKER");
        snd_soc_dapm_disable_pin(&codec->dapm, "RX_BIAS");
        snd_soc_dapm_sync(&codec->dapm);
    }
    else
    {
        printk("%s()\n", __func__);
        snd_soc_dapm_enable_pin(&codec->dapm, "SPEAKER");
        snd_soc_dapm_enable_pin(&codec->dapm, "RX_BIAS");
        snd_soc_dapm_sync(&codec->dapm);
    }

    mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_VOICE] = ucontrol->value.integer.value[0];
    return 0;
}

static uint32 mHp_Impedance = 32;

static int Audio_Hp_Impedance_Get(struct snd_kcontrol *kcontrol,
                                  struct snd_ctl_elem_value *ucontrol)
{
    printk("Audio_Hp_Impedance_Get = %d\n", mHp_Impedance);
    ucontrol->value.integer.value[0] = mHp_Impedance;
    return 0;

}

static int Audio_Hp_Impedance_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    mHp_Impedance = ucontrol->value.integer.value[0];
    printk("%s Audio_Hp_Impedance_Set = 0x%x \n", __func__, mHp_Impedance);
    return 0;
}

static int Aud_Clk_Buf_Get(struct snd_kcontrol *kcontrol,
                           struct snd_ctl_elem_value *ucontrol)
{
    printk("\%s n", __func__);
    ucontrol->value.integer.value[0] = audck_buf_Count;
    return 0;
}

static int Aud_Clk_Buf_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
//    int index = 0;
    printk("%s(), value = %d\n", __func__, ucontrol->value.enumerated.item[0]);
    if (ucontrol->value.integer.value[0])
    {
        audckbufEnable(true);
    }
    else
    {
        audckbufEnable(false);
    }
    return 0;
}


static const struct soc_enum Audio_DL_Enum[] =
{
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(amp_function), amp_function),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(amp_function), amp_function),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(amp_function), amp_function),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(amp_function), amp_function),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(amp_function), amp_function),
    // here comes pga gain setting
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(DAC_DL_PGA_Headset_GAIN), DAC_DL_PGA_Headset_GAIN),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(DAC_DL_PGA_Headset_GAIN), DAC_DL_PGA_Headset_GAIN),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(DAC_DL_PGA_Handset_GAIN), DAC_DL_PGA_Handset_GAIN),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(DAC_DL_PGA_Speaker_GAIN), DAC_DL_PGA_Speaker_GAIN),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(DAC_DL_PGA_Speaker_GAIN), DAC_DL_PGA_Speaker_GAIN),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(aud_clk_buf_function), aud_clk_buf_function),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(amp_function), amp_function),
};

static const struct snd_kcontrol_new mt6331_snd_controls[] =
{
    SOC_ENUM_EXT("Audio_Amp_R_Switch", Audio_DL_Enum[0], Audio_AmpR_Get, Audio_AmpR_Set),
    SOC_ENUM_EXT("Audio_Amp_L_Switch", Audio_DL_Enum[1], Audio_AmpL_Get, Audio_AmpL_Set),
    SOC_ENUM_EXT("Voice_Amp_Switch", Audio_DL_Enum[2], Voice_Amp_Get, Voice_Amp_Set),
    SOC_ENUM_EXT("Speaker_Amp_Switch", Audio_DL_Enum[3], Speaker_Amp_Get, Speaker_Amp_Set),
    SOC_ENUM_EXT("Headset_Speaker_Amp_Switch", Audio_DL_Enum[4], Headset_Speaker_Amp_Get, Headset_Speaker_Amp_Set),
    SOC_ENUM_EXT("Headset_PGAL_GAIN", Audio_DL_Enum[5], Headset_PGAL_Get, Headset_PGAL_Set),
    SOC_ENUM_EXT("Headset_PGAR_GAIN", Audio_DL_Enum[6], Headset_PGAR_Get, Headset_PGAR_Set),
    SOC_ENUM_EXT("Handset_PGA_GAIN", Audio_DL_Enum[7], Handset_PGA_Get, Handset_PGA_Set),
    SOC_ENUM_EXT("Lineout_PGAR_GAIN", Audio_DL_Enum[8], Lineout_PGAR_Get, Lineout_PGAR_Set),
    SOC_ENUM_EXT("Lineout_PGAL_GAIN", Audio_DL_Enum[9], Lineout_PGAL_Get, Lineout_PGAL_Set),
    SOC_ENUM_EXT("AUD_CLK_BUF_Switch", Audio_DL_Enum[10], Aud_Clk_Buf_Get, Aud_Clk_Buf_Set),
    SOC_ENUM_EXT("Ext_Speaker_Amp_Switch", Audio_DL_Enum[11], Ext_Speaker_Amp_Get, Ext_Speaker_Amp_Set),
    SOC_SINGLE_EXT("Audio HP Impedance", SND_SOC_NOPM, 0, 512, 0, Audio_Hp_Impedance_Get, Audio_Hp_Impedance_Set),
};

static const struct snd_kcontrol_new mt6331_Voice_Switch[] =
{
    //SOC_DAPM_ENUM_EXT("Voice Mux", Audio_DL_Enum[10], Voice_Mux_Get, Voice_Mux_Set),
};

void SetMicPGAGain(void)
{
    int index = 0;
    index = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP1];
    Ana_Set_Reg(AUDENC_ANA_CON15, index , 0x0007);
    index = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP2];
    Ana_Set_Reg(AUDENC_ANA_CON15, index << 4, 0x0070);
}

static bool GetAdcStatus(void)
{
    int i = 0;
    for (i = AUDIO_ANALOG_DEVICE_IN_ADC1 ; i < AUDIO_ANALOG_DEVICE_MAX ; i++)
    {
        if (mCodec_data->mAudio_Ana_DevicePower[i] == true)
        {
            return true;
        }
    }
    return false;
}

static bool GetDacStatus(void)
{
    int i = 0;
    for (i = AUDIO_ANALOG_DEVICE_OUT_EARPIECER ; i < AUDIO_ANALOG_DEVICE_2IN1_SPK ; i++)
    {
        if (mCodec_data->mAudio_Ana_DevicePower[i] == true)
        {
            return true;
        }
    }
    return false;
}


static bool TurnOnADcPowerACC(int ADCType, bool enable)
{
    printk("%s ADCType = %d enable = %d \n", __func__, ADCType, enable);
    if (enable)
    {
//        uint32 ULIndex = GetULFrequency(mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC]);
        uint32 SampleRate_VUL1 = mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC];
//        uint32 SampleRate_VUL2 = mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC_2];
        if (GetAdcStatus() == false)
        {
            audckbufEnable(true);
            Ana_Set_Reg(LDO_VCON1, 0x0301, 0xffff); //VA28 remote sense
            Ana_Set_Reg(LDO_CON2, 0x8102, 0xffff); // LDO enable control by RG_VAUD28_EN, Enable AVDD28_LDO (Default on)

            if (mAdc_Power_Mode == true)
            {
                //Ana_Set_Reg(AUDBUF_CFG4, 0x0001, 0x0001);  // Set AVDD32_AUD lowpower mode, K2 todo
            }
            else
            {
                //Ana_Set_Reg(AUDBUF_CFG4, 0x0000, 0x0001);  // Set AVDD32_AUD lowpower mode, K2 todo
            }
            NvregEnable(true);
            //ClsqAuxEnable(true);
            ClsqEnable(true);
            if (mAdc_Power_Mode == true)
            {
                //Ana_Set_Reg(AUDADC_CFG0, 0x4400, 0xffff);    // Enable ADC CLK,K2 todo
            }
            else
            {
                //Ana_Set_Reg(AUDADC_CFG0, 0x0400, 0xffff);    // Enable ADC CLK,K2 todo
            }

            Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0004, 0x0004); //Enable audio ADC CLKGEN
            Ana_Set_Reg(AUDENC_ANA_CON3, 0x0000, 0xffff); //ADC CLK from CLKGEN (13MHz)
            Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0104, 0x0104); //Enable  LCLDO_ENC 1P8V
            Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0006, 0x0006); //LCLDO_ENC remote sense
            Ana_Set_Reg(AUDENC_ANA_CON6, 0x1515, 0xffff); //default value
            Ana_Set_Reg(AUDENC_ANA_CON4, 0x0800, 0xffff); //default value

            if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC1) //main and headset mic
            {
                if (mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] == 0) //"ADC1", main_mic
                {
                    Ana_Set_Reg(AUDENC_ANA_CON9, 0x0201, 0xff0f); //Enable MICBIAS0, MISBIAS0 = 1P9V
                }
                else if (mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] == 1) //"ADC2", headset mic
                {
                    Ana_Set_Reg(AUDENC_ANA_CON9, 0x0710, 0xfff0); //Enable MICBIAS1, MISBIAS1 = 2P5V
                }
                Ana_Set_Reg(AUDENC_ANA_CON15, 0x0003, 0x000f); //Audio L PGA 18 dB gain(SMT)
            }
            else if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC2) //ref mic
            {
                Ana_Set_Reg(AUDENC_ANA_CON9, 0x0201, 0xff0f); //Enable MICBIAS0, MISBIAS0 = 1P9V
#if 0
                Ana_Set_Reg(AUDENC_ANA_CON15, 0x0030, 0x00f0); //Audio R PGA 18 dB gain(SMT)
#else //ccc 0618 test using ADC1
                Ana_Set_Reg(AUDENC_ANA_CON15, 0x0003, 0x000f); //Audio L PGA 18 dB gain(SMT)
#endif
            }

            if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC1) //main and headset mic
            {
                Ana_Set_Reg(AUDENC_ANA_CON3, 0x0800, 0xf900); //PGA stb enhance

                if (mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] == 0) //"ADC1", main_mic
                {
                    Ana_Set_Reg(AUDENC_ANA_CON0, 0x0041, 0x00C1); //Audio L preamplifier input sel : AIN0. Enable audio L PGA
                    Ana_Set_Reg(AUDENC_ANA_CON0, 0x0541, 0xffff); //Audio L ADC input sel : L PGA. Enable audio L ADC
                }
                else if (mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] == 1) //"ADC2", headset mic
                {
                    Ana_Set_Reg(AUDENC_ANA_CON0, 0x0500, 0xffff); //Audio L ADC input sel : L PGA. Enable audio L ADC
                    Ana_Set_Reg(AUDENC_ANA_CON0, 0x0581, 0xffff); //Audio L preamplifier input sel : AIN1. Enable audio L PGA

                }
            }
            else   if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC2) //ref mic
            {
                Ana_Set_Reg(AUDENC_ANA_CON3, 0x0800, 0xf900); //PGA stb enhance //ccc 0618
#if 0
                Ana_Set_Reg(AUDENC_ANA_CON1, 0x00C1, 0x00C1); //Audio R preamplifier input sel : AIN2. Enable audio R PGA
                Ana_Set_Reg(AUDENC_ANA_CON1, 0x05C1, 0xffff); //Audio R ADC input sel : R PGA. Enable audio R ADC
#else //ccc 0618 test using ADC1
                Ana_Set_Reg(AUDENC_ANA_CON0, 0x00c1, 0x00C1); //Audio L preamplifier input sel : AIN2. Enable audio L PGA
                Ana_Set_Reg(AUDENC_ANA_CON0, 0x05c1, 0xffff); //Audio L ADC input sel : L PGA. Enable audio L ADC
#endif
            }

#if 0 //K2 need?
            SetDCcoupleNP(AUDIO_ANALOG_DEVICE_IN_ADC1, mAudio_Analog_Mic1_mode);
            SetDCcoupleNP(AUDIO_ANALOG_DEVICE_IN_ADC2, mAudio_Analog_Mic2_mode);

            //OpenMicbias1();
            //OpenMicbias0();
            if (mAdc_Power_Mode == false)
            {
                SetMicbias1lowpower(false);
                SetMicbias0lowpower(false);
            }
            else
            {
                SetMicbias1lowpower(true);
                SetMicbias0lowpower(true);
            }

            //Ana_Set_Reg(AUDMICBIAS_CFG1, 0x78F, 0xffff);   //Enable MICBIAS0,1 (2.7V)
            SetMicVref2(0x2);// 1.9V
            SetMicVref3(0x2); // 1.9V
            SetMic2DCcoupleSwitch(false);
            SetMic3DCcoupleSwitch(false);
            if (mAdc_Power_Mode == false)
            {
                SetMic2powermode(false);
                SetMic3powermode(false);
            }
            else
            {
                SetMic2powermode(true);
                SetMic3powermode(true);
            }
            //OpenMicbias3(true);
            //OpenMicbias2(true);
            if (mAdc_Power_Mode == false)
            {
                Ana_Set_Reg(AUDLDO_NVREG_CFG1, 0x0007, 0xffff);   //Enable LCLDO18_ENC (1.8V), Remote-Sense
                Ana_Set_Reg(AUDLDO_NVREG_CFG2, 0x0000, 0x8888);   //Enable  LCLDO19_ADCCH0_1, Remote-Sense
            }
            else
            {
                Ana_Set_Reg(AUDLDO_NVREG_CFG1, 0x000f, 0xffff);   //Enable LCLDO18_ENC (1.8V), Remote-Sense
                Ana_Set_Reg(AUDLDO_NVREG_CFG2, 0x8888, 0x8888);   //Enable  LCLDO19_ADCCH0_1, Remote-Sense
            }
            //Ana_Set_Reg(AUDPREAMPGAIN_CFG0, 0x3333, 0xffff);   //Set PGA CH0_1 gain = 18dB
#endif
            //SetMicPGAGain(); //K2 mark temp for SMT
            //Ana_Set_Reg(AUDPREAMP_CFG0, 0x0051, 0x001f);   //Enable PGA CH0_1 (CH0 in)
            //Ana_Set_Reg(AUDPREAMP_CFG1, 0x16d5, 0xffff);   //Enable ADC CH0_1 (PGA in)

            //here to set digital part
            Topck_Enable(true);
            //AdcClockEnable(true);

            Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0000, 0xffff);    //Audio system digital clock power down release
            Ana_Set_Reg(PMIC_AFE_TOP_CON0, 0x0000, 0xffff);   //configure ADC setting
            //Ana_Set_Reg(AFUNC_AUD_CON2, 0x0006, 0xffff); //sdm audio fifo clock power on
            //Ana_Set_Reg(AFUNC_AUD_CON0, 0xc3a1, 0xffff); //scrambler clock on enable
            //Ana_Set_Reg(AFUNC_AUD_CON2, 0x0003, 0xffff); //sdm power on
            //Ana_Set_Reg(AFUNC_AUD_CON2, 0x000b, 0xffff); //sdm fifo enable
            //Ana_Set_Reg(AFE_DL_SDM_CON1, 0x001e, 0xffff); //set attenuation gain
            Ana_Set_Reg(AFE_UL_DL_CON0 , 0x0001, 0xffff); //[0] afe enable

            Ana_Set_Reg(AFE_UL_SRC0_CON0_H , (ULSampleRateTransform(SampleRate_VUL1) << 3 | ULSampleRateTransform(SampleRate_VUL1) << 1) , 0x001f); //UL sample rate and mode configure
            Ana_Set_Reg(AFE_UL_SRC0_CON0_L , 0x0001, 0xffff); //UL turn on
        }
    }
    else
    {
        if (GetAdcStatus() == false)
        {
            Ana_Set_Reg(AFE_UL_SRC0_CON0_L, 0x0000, 0xffff);   //UL turn off
            Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0020, 0x0020);   //up-link power down

            if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC1) //main and headset mic
            {
                if (mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] == 0) //"ADC1", main_mic
                {
                    Ana_Set_Reg(AUDENC_ANA_CON0, 0x0041, 0xffff);   //Audio L ADC input sel : off, disable audio L ADC
                    Ana_Set_Reg(AUDENC_ANA_CON3, 0x0000, 0xffff);   //PGA stb enhance off
                    Ana_Set_Reg(AUDENC_ANA_CON15, 0x0000, 0x000f); //Audio L PGA 0 dB gain
                    Ana_Set_Reg(AUDENC_ANA_CON0, 0x0000, 0xffff);   //Audio L preamplifier input sel : off, disable audio L PGA
                }
                else if (mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] == 1) //"ADC2", headset mic
                {
                    Ana_Set_Reg(AUDENC_ANA_CON0, 0x0500, 0xffff);   //Audio L preamplifier input sel : off, disable audio L PGA
                    Ana_Set_Reg(AUDENC_ANA_CON0, 0x0000, 0xffff);   //Audio L ADC input sel : off, disable audio L ADC
                    Ana_Set_Reg(AUDENC_ANA_CON3, 0x0000, 0xffff);   //PGA stb enhance off
                    Ana_Set_Reg(AUDENC_ANA_CON15, 0x0000, 0x000f); //Audio L PGA 0 dB gain
                }

                Ana_Set_Reg(AUDENC_ANA_CON4, 0x0000, 0xffff);   //
                Ana_Set_Reg(AUDENC_ANA_CON6, 0x2020, 0xffff);   //

                if (mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] == 0) //"ADC1", main_mic
                {
                    Ana_Set_Reg(AUDENC_ANA_CON9, 0x0000, 0xff0f);   //disable MICBIAS0
                }
                else if (mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] == 1) //"ADC2", headset mic
                {
                    Ana_Set_Reg(AUDENC_ANA_CON9, 0x0000, 0xfff0); //disable MICBIAS1
                }
            }
            else if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC2) //ref mic
            {
#if 0
                Ana_Set_Reg(AUDENC_ANA_CON1, 0x00C1, 0xffff);   //Audio R ADC input sel : off, disable audio R ADC
                Ana_Set_Reg(AUDENC_ANA_CON3, 0x0000, 0xffff);   //PGA stb enhance off
                Ana_Set_Reg(AUDENC_ANA_CON15, 0x0000, 0x00f0); //Audio R PGA 0 dB gain
                Ana_Set_Reg(AUDENC_ANA_CON1, 0x0000, 0xffff);   //Audio R preamplifier input sel : off, disable audio R PGA
#else //ccc 0618 test using ADC1
                Ana_Set_Reg(AUDENC_ANA_CON0, 0x00c1, 0xffff);   //Audio L ADC input sel : off, disable audio L ADC
                Ana_Set_Reg(AUDENC_ANA_CON3, 0x0000, 0xffff);   //PGA stb enhance off
                Ana_Set_Reg(AUDENC_ANA_CON15, 0x0000, 0x000f); //Audio L PGA 0 dB gain
                Ana_Set_Reg(AUDENC_ANA_CON0, 0x0000, 0xffff);   //Audio L preamplifier input sel : off, disable audio L PGA
#endif
                Ana_Set_Reg(AUDENC_ANA_CON4, 0x0000, 0xffff);   //
                Ana_Set_Reg(AUDENC_ANA_CON6, 0x2020, 0xffff);   //

                Ana_Set_Reg(AUDENC_ANA_CON9, 0x0000, 0xff0f); //disable MICBIAS0
            }

            Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0000, 0x0006);   //LCLDO_ENC remote sense off
            Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0004, 0x0104);   //disable LCLDO_ENC 1P8V
            Ana_Set_Reg(AUDENC_ANA_CON3, 0x0000, 0xffff);   //disable ADC CLK from CLKGEN (13MHz)
            Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0000, 0x0104);   //disable audio ADC CLKGEN

            if (GetDLStatus() == false)
            {
                Ana_Set_Reg(AFE_UL_DL_CON0, 0x0000, 0xffff);   //afe disable
                Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0084, 0x0084);   //afe power down and total audio clk disable
            }

            //AdcClockEnable(false);
            Topck_Enable(false);
            //ClsqAuxEnable(false);
            ClsqEnable(false);
            NvregEnable(false);
            audckbufEnable(false);
        }
    }
    return true;
}

static bool TurnOnADcPowerDmic(int ADCType, bool enable)
{
    printk("%s ADCType = %d enable = %d \n", __func__, ADCType, enable);
    if (enable)
    {
        uint32 ULIndex = GetULFrequency(mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC]);
        uint32 SampleRate_VUL1 = mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC];
//        uint32 SampleRate_VUL2 = mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC_2];
        if (GetAdcStatus() == false)
        {
            audckbufEnable(true);
            Ana_Set_Reg(LDO_CON2, 0x8102, 0xffff); // LDO enable control by RG_VAUD28_EN, Enable AVDD28_LDO (Default on)

            if (mAdc_Power_Mode == true)
            {
                //Ana_Set_Reg(AUDBUF_CFG4, 0x0001, 0x0001);  // Set AVDD32_AUD lowpower mode, K2 todo
            }
            else
            {
                //Ana_Set_Reg(AUDBUF_CFG4, 0x0000, 0x0001);  // Set AVDD32_AUD lowpower mode, K2 todo
            }
            NvregEnable(true);
            //ClsqAuxEnable(true);
            ClsqEnable(true);
            if (mAdc_Power_Mode == true)
            {
                //Ana_Set_Reg(AUDADC_CFG0, 0x4400, 0xffff);    // Enable ADC CLK,K2 todo
            }
            else
            {
                //Ana_Set_Reg(AUDADC_CFG0, 0x0400, 0xffff);    // Enable ADC CLK,K2 todo
            }
            Ana_Set_Reg(AUDENC_ANA_CON9, 0x0201, 0xff0f); //Enable MICBIAS0, MISBIAS0 = 1P9V
            Ana_Set_Reg(AUDENC_ANA_CON8, 0x0005, 0xffff); //DMIC enable

            //here to set digital part
            Topck_Enable(true);
            //AdcClockEnable(true);

            Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0000, 0xffff);    //Audio system digital clock power down release
            Ana_Set_Reg(PMIC_AFE_TOP_CON0, (ULIndex << 7) | (ULIndex << 6), 0xffff); //dmic sample rate
            Ana_Set_Reg(AFE_TOP_CON0, 0x00C0, 0xffff);   //DMIC configure, ch1 and ch2 set to 3.25MHz 48k
            Ana_Set_Reg(AFE_UL_DL_CON0 , 0x0001, 0xffff); //[0] afe enable

            Ana_Set_Reg(AFE_UL_SRC0_CON0_H , (ULSampleRateTransform(SampleRate_VUL1) << 3 | ULSampleRateTransform(SampleRate_VUL1) << 1) , 0x001f); //UL sample rate and mode configure
            Ana_Set_Reg(AFE_UL_SRC0_CON0_H , 0x0060  , 0xffe0); //ch1 and ch2 digital mic ON
            Ana_Set_Reg(AFE_UL_SRC0_CON0_L , 0x0003, 0xffff); //select SDM 3-level mode, UL turn on
        }
    }
    else
    {
        if (GetAdcStatus() == false)
        {
            Ana_Set_Reg(AFE_UL_SRC0_CON0_L, 0x0000, 0xffff);   //UL turn off
            Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0020, 0x0020);   //up-link power down

            Ana_Set_Reg(AUDENC_ANA_CON8, 0x0000, 0xffff); //DMIC enable
            Ana_Set_Reg(AUDENC_ANA_CON9, 0x0000, 0xff0f);   //MICBIAS0(1.7v), powen down

            if (GetDLStatus() == false)
            {
                Ana_Set_Reg(AFE_UL_DL_CON0, 0x0000, 0xffff);   //afe disable
                Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0084, 0x0084);   //afe power down and total audio clk disable
            }

            //AdcClockEnable(false);
            Topck_Enable(false);
            //ClsqAuxEnable(false);
            ClsqEnable(false);
            NvregEnable(false);
            audckbufEnable(false);
        }
    }
    return true;
}

static bool TurnOnADcPowerDCC(int ADCType, bool enable)
{
    printk("%s ADCType = %d enable = %d \n", __func__, ADCType, enable);
    if (enable)
    {
//        uint32 ULIndex = GetULFrequency(mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC]);
        uint32 SampleRate_VUL1 = mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC];
//        uint32 SampleRate_VUL2 = mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC_2];
        if (GetAdcStatus() == false)
        {
            audckbufEnable(true);
            Ana_Set_Reg(LDO_VCON1, 0x0301, 0xffff); //VA28 remote sense
            Ana_Set_Reg(LDO_CON2, 0x8102, 0xffff); // LDO enable control by RG_VAUD28_EN, Enable AVDD28_LDO (Default on)

            if (mAdc_Power_Mode == true)
            {
                //Ana_Set_Reg(AUDBUF_CFG4, 0x0001, 0x0001);  // Set AVDD32_AUD lowpower mode, K2 todo
            }
            else
            {
                //Ana_Set_Reg(AUDBUF_CFG4, 0x0000, 0x0001);  // Set AVDD32_AUD lowpower mode, K2 todo
            }
            NvregEnable(true);
            //ClsqAuxEnable(true);
            ClsqEnable(true);
            if (mAdc_Power_Mode == true)
            {
                //Ana_Set_Reg(AUDADC_CFG0, 0x4400, 0xffff);    // Enable ADC CLK,K2 todo
            }
            else
            {
                //Ana_Set_Reg(AUDADC_CFG0, 0x0400, 0xffff);    // Enable ADC CLK,K2 todo
            }

            Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0004, 0x0004); //Enable audio ADC CLKGEN
            Ana_Set_Reg(AUDENC_ANA_CON3, 0x0000, 0xffff); //ADC CLK from CLKGEN (13MHz)
            Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0104, 0x0104); //Enable LCLDO_ENC 1P8V

            Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0006, 0x0006); //LCLDO_ENC remote sense

            //DCC 50k CLK(from 26M
            Ana_Set_Reg(TOP_CLKSQ_SET, 0x0003, 0x0003); //
            Ana_Set_Reg(TOP_CKPDN_CON0, 0x0000, 0x2000); //bit[13] AUD_CK power down=0

            Ana_Set_Reg(AFE_DCCLK_CFG0, 0x2062, 0x0002); //dcclk_div=11'b00100000011, dcclk_ref_ck_sel=2'b00
            Ana_Set_Reg(AFE_DCCLK_CFG0, 0x2060, 0xffff); //dcclk_pdn=1'b0
            Ana_Set_Reg(AFE_DCCLK_CFG0, 0x2061, 0xffff); //dcclk_gen_on=1'b1
            //
            Ana_Set_Reg(AUDENC_ANA_CON6, 0x1515, 0xffff); //default value
            Ana_Set_Reg(AUDENC_ANA_CON4, 0x0800, 0xffff); //default value

            if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC1) //main and headset mic
            {
                if (mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] == 0) //"ADC1", main_mic
                {
                    Ana_Set_Reg(AUDENC_ANA_CON9, 0x0201, 0xff0f); //Enable MICBIAS0, MISBIAS0 = 1P9V
                    Ana_Set_Reg(AUDENC_ANA_CON9, 0x0207, 0xff0f); //MICBIAS0 DCC SwithP/N on //DCC
                    Ana_Set_Reg(AUDENC_ANA_CON0, 0x0004, 0xffff); //Audio L preamplifier DCC precharge
                    Ana_Set_Reg(AUDENC_ANA_CON0, 0x0045, 0xffff); //Audio L preamplifier input sel : AIN0. Enable audio L PGA
                    Ana_Set_Reg(AUDENC_ANA_CON15, 0x0002, 0x000f); //Audio L PGA 12 dB gain
                    Ana_Set_Reg(AUDENC_ANA_CON0, 0x0047, 0xffff); //Audio L preamplifier DCCEN
                }
                else if (mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] == 1) //"ADC2", headset mic
                {
                    Ana_Set_Reg(AUDENC_ANA_CON9, 0x0710, 0xfff0); //Enable MICBIAS1, MISBIAS1 = 2P5V
                    Ana_Set_Reg(AUDENC_ANA_CON9, 0x0770, 0xfff0); //MICBIAS1 DCC SwithP/N on //DCC
                    Ana_Set_Reg(AUDENC_ANA_CON0, 0x0004, 0xffff); //Audio L preamplifier DCC precharge
                    Ana_Set_Reg(AUDENC_ANA_CON0, 0x0085, 0xffff); //Audio L preamplifier input sel : AIN1. Enable audio L PGA
                    Ana_Set_Reg(AUDENC_ANA_CON15, 0x0002, 0x000f); //Audio L PGA 12 dB gain
                    Ana_Set_Reg(AUDENC_ANA_CON0, 0x0087, 0xffff); //Audio L preamplifier DCCEN
                }
            }
            else if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC2) //ref mic
            {
                Ana_Set_Reg(AUDENC_ANA_CON9, 0x0201, 0xff0f); //Enable MICBIAS0, MISBIAS0 = 1P9V
                Ana_Set_Reg(AUDENC_ANA_CON9, 0x0207, 0xff0f); //MICBIAS0 DCC SwithP/N on //DCC
                Ana_Set_Reg(AUDENC_ANA_CON1, 0x0004, 0xffff); //Audio R preamplifier DCC precharge
                Ana_Set_Reg(AUDENC_ANA_CON1, 0x00C5, 0xffff); //Audio R preamplifier input sel : AIN2. Enable audio R PGA
                Ana_Set_Reg(AUDENC_ANA_CON15, 0x0020, 0x00f0); //Audio R PGA 12 dB gain
                Ana_Set_Reg(AUDENC_ANA_CON1, 0x00C7, 0xffff); //Audio R preamplifier DCCEN
            }

            Ana_Set_Reg(AUDENC_ANA_CON3, 0x0800, 0xf900); //PGA stb enhance

            if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC1) //main and headset mic
            {

                if (mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] == 0) //"ADC1", main_mic
                {
                    Ana_Set_Reg(AUDENC_ANA_CON0, 0x0547, 0xffff); //Audio L ADC input sel : L PGA. Enable audio L ADC
                    udelay(100);
                    Ana_Set_Reg(AUDENC_ANA_CON0, 0x0543, 0xffff); //Audio L preamplifier DCC precharge off
                }
                else if (mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] == 1) //"ADC2", headset mic
                {
                    Ana_Set_Reg(AUDENC_ANA_CON0, 0x0587, 0xffff); //Audio L ADC input sel : L PGA. Enable audio L ADC
                    udelay(100);
                    Ana_Set_Reg(AUDENC_ANA_CON0, 0x0583, 0xffff); //Audio L preamplifier DCC precharge off
                }
            }
            else if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC2) //ref mic
            {
                Ana_Set_Reg(AUDENC_ANA_CON1, 0x05C7, 0xffff); //Audio R ADC input sel : R PGA. Enable audio R ADC
                udelay(100);
                Ana_Set_Reg(AUDENC_ANA_CON1, 0x05C3, 0xffff); //Audio R preamplifier DCC precharge off
            }

            //SetMicPGAGain(); //K2 mark temp for SMT

            //here to set digital part
            //Ana_Set_Reg(TOP_CKPDN_CON0, 0xCEFC, 0x3000);   //Audio clock power down release
            Topck_Enable(true);
            //AdcClockEnable(true);

            Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0000, 0xffff);    //Audio system digital clock power down release
            Ana_Set_Reg(PMIC_AFE_TOP_CON0, 0x0000, 0xffff);   //configure ADC setting
            Ana_Set_Reg(AFE_UL_DL_CON0 , 0x0001, 0xffff); //[0] afe enable

            Ana_Set_Reg(AFE_UL_SRC0_CON0_H , (ULSampleRateTransform(SampleRate_VUL1) << 3 | ULSampleRateTransform(SampleRate_VUL1) << 1) , 0x001f); //UL sample rate and mode configure
            Ana_Set_Reg(AFE_UL_SRC0_CON0_L , 0x0001, 0xffff); //UL turn on
        }
    }
    else
    {
        if (GetAdcStatus() == false)
        {
            Ana_Set_Reg(AFE_UL_SRC0_CON0_L, 0x0000, 0xffff);   //UL turn off
            Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0020, 0x0020);   //up-link power down

            if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC1) //main and headset mic
            {
                if (mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] == 0) //"ADC1", main_mic
                {
                    Ana_Set_Reg(AUDENC_ANA_CON0, 0x0043, 0xffff);   //Audio L ADC input sel : off, disable audio L ADC
                    Ana_Set_Reg(AUDENC_ANA_CON3, 0x0000, 0xffff);   //PGA stb enhance off
                    Ana_Set_Reg(AUDENC_ANA_CON0, 0x0041, 0xffff);   //Audio L preamplifier DCCEN disable
                    Ana_Set_Reg(AUDENC_ANA_CON15, 0x0000, 0x000f); //Audio L PGA 0 dB gain
                    Ana_Set_Reg(AUDENC_ANA_CON0, 0x0000, 0xffff);   //Audio L preamplifier input sel : off, disable audio L PGA
                }
                else if (mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] == 1) //"ADC2", headset mic
                {
                    Ana_Set_Reg(AUDENC_ANA_CON0, 0x0083, 0xffff);   //Audio L preamplifier input sel : off, disable audio L PGA
                    Ana_Set_Reg(AUDENC_ANA_CON3, 0x0000, 0xffff);   //PGA stb enhance off
                    Ana_Set_Reg(AUDENC_ANA_CON0, 0x0081, 0xffff);   //Audio L preamplifier DCCEN disable
                    Ana_Set_Reg(AUDENC_ANA_CON15, 0x0000, 0x000f); //Audio L PGA 0 dB gain
                    Ana_Set_Reg(AUDENC_ANA_CON0, 0x0000, 0xffff);   //Audio L ADC input sel : off, disable audio L ADC
                }

                Ana_Set_Reg(AUDENC_ANA_CON4, 0x0000, 0xffff);   //
                Ana_Set_Reg(AUDENC_ANA_CON6, 0x2020, 0xffff);   //

                if (mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] == 0) //"ADC1", main_mic
                {
                    Ana_Set_Reg(AUDENC_ANA_CON9, 0x0000, 0xff0f);   //disable MICBIAS0
                }
                else if (mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] == 1) //"ADC2", headset mic
                {
                    Ana_Set_Reg(AUDENC_ANA_CON9, 0x0000, 0xfff0); //disable MICBIAS1
                }
            }
            else if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC2) //ref mic
            {
                Ana_Set_Reg(AUDENC_ANA_CON1, 0x00C3, 0xffff);   //Audio R ADC input sel : off, disable audio R ADC
                Ana_Set_Reg(AUDENC_ANA_CON3, 0x0000, 0xffff);   //PGA stb enhance off
                Ana_Set_Reg(AUDENC_ANA_CON1, 0x00C1, 0xffff);   //Audio R preamplifier DCCEN disable
                Ana_Set_Reg(AUDENC_ANA_CON15, 0x0000, 0x00f0); //Audio R PGA 0 dB gain
                Ana_Set_Reg(AUDENC_ANA_CON1, 0x0000, 0xffff);   //Audio R preamplifier input sel : off, disable audio R PGA

                Ana_Set_Reg(AUDENC_ANA_CON4, 0x0000, 0xffff);   //
                Ana_Set_Reg(AUDENC_ANA_CON6, 0x2020, 0xffff);   //

                Ana_Set_Reg(AUDENC_ANA_CON9, 0x0000, 0xff0f); //disable MICBIAS0
            }

            Ana_Set_Reg(AFE_DCCLK_CFG0, 0x2060, 0xffff); //dcclk_gen_on=1'b0
            Ana_Set_Reg(AFE_DCCLK_CFG0, 0x2062, 0x0002); //dcclk_pdn=1'b0

            Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0000, 0x0006);   //LCLDO_ENC remote sense off
            Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0004, 0x0104);   //disable LCLDO_ENC 1P8V
            Ana_Set_Reg(AUDENC_ANA_CON3, 0x0000, 0xffff);   //disable ADC CLK from CLKGEN (13MHz)
            Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0000, 0x0104);   //disable audio ADC CLKGEN

            if (GetDLStatus() == false)
            {
                Ana_Set_Reg(AFE_UL_DL_CON0, 0x0000, 0xffff);   //afe disable
                Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0084, 0x0084);   //afe power down and total audio clk disable
            }

            //AdcClockEnable(false);
            Topck_Enable(false);
            //ClsqAuxEnable(false);
            ClsqEnable(false);
            NvregEnable(false);
            audckbufEnable(false);
        }
    }
    return true;
}


static bool TurnOnADcPowerDCCECM(int ADCType, bool enable)
{
    //K2 todo???
    return true;
}

static bool TurnOnVOWDigitalHW(bool enable)
{
    printk("%s enable = %d \n", __func__, enable);
    if (enable)
    {
        if (mAudio_VOW_Mic_type == AUDIO_VOW_MIC_TYPE_Handset_DMIC)
        {
            Ana_Set_Reg(AFE_VOW_TOP, 0x6850, 0xffff);   //VOW enable
        }
        else
        {
            Ana_Set_Reg(AFE_VOW_TOP, 0x4810, 0xffff);   //VOW enable
        }
    }
    else
    {
        //disable VOW interrupt here?

        Ana_Set_Reg(AFE_VOW_TOP, 0x4010, 0xffff);   //VOW disable
        Ana_Set_Reg(AFE_VOW_TOP, 0xC010, 0xffff);   //VOW clock power down
    }
    return true;
}

static bool TurnOnVOWADcPowerACC(int MicType, bool enable)
{
    printk("%s MicType = %d enable = %d \n", __func__, MicType, enable);
    if (enable)
    {
#if defined(VOW_TONE_TEST)
        OpenAfeDigitaldl1(false);
        OpenAnalogHeadphone(false);
        EnableSideGenHw(Soc_Aud_InterConnectionOutput_O03, Soc_Aud_InterConnectionOutput_Num_Output, false);
        AudDrv_Clk_Off();
#endif
#if defined (MTK_VOW_SUPPORT)
        //Set VOW driver status first
        VowDrv_EnableHW(true);
#endif
        //uint32 ULIndex = GetULFrequency(mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC]);
//        uint32 SampleRate = 8000;//mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC];
        switch (MicType)
        {
            case AUDIO_VOW_MIC_TYPE_Headset_MIC:
                printk("%s, case AUDIO_VOW_MIC_TYPE_Headset_MIC \n", __func__);
                //analog part
                Ana_Set_Reg(LDO_VCON1, 0x0301, 0xffff); //VA28 remote sense
                Ana_Set_Reg(LDO_CON2, 0x8103, 0xffff); // LDO enable control by RG_VAUD28_EN, Enable AVDD28_LDO (Default on), LPW control by VAUD28_MODE_SET Enable low power mode
                NvregEnable(false); //Disable audio globe bias (Default on)
                Ana_Set_Reg(AUDENC_ANA_CON2, 0x0003, 0xffff); //Enable audio uplink VOW LPW globe bias, Enable audio uplink LPW mode
                Ana_Set_Reg(AUDENC_ANA_CON12, 0x0719, 0xffff); //Enable fbdiv relatch (low jitter), Set DCKO = 1/4 F_PLL, Enable VOWPLL CLK
                Ana_Set_Reg(AUDENC_ANA_CON14, 0x0023, 0xffff); //PLL VCOBAND
                Ana_Set_Reg(AUDENC_ANA_CON12, 0x06F9, 0xffff); //PLL devider ratio
                Ana_Set_Reg(AUDENC_ANA_CON13, 0x8180, 0xffff); //PLL low power
                Ana_Set_Reg(AUDENC_ANA_CON3, 0x0009, 0xffff); //ADC CLK from VOWPLL (12.85/4MHz), Enable Audio ADC FBDAC 0.25FS LPW
                Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0101, 0xffff); //Enable  LCLDO_ENC 1P8V
                Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0006, 0xffff); //LCLDO_ENC remote sense
                Ana_Set_Reg(AUDENC_ANA_CON6, 0x1515, 0xffff);
                Ana_Set_Reg(AUDENC_ANA_CON4, 0x0800, 0xffff);
                Ana_Set_Reg(AUDENC_ANA_CON9, 0x0F10, 0xffff); //Enable MICBIAS1 lowpower mode, MISBIAS1 = 2P5V
                Ana_Set_Reg(AUDENC_ANA_CON0, 0x0081, 0xffff); //Audio L preamplifier input sel : AIN1, Enable audio L PGA
                Ana_Set_Reg(AUDENC_ANA_CON15, 0x0003, 0xffff); //Audio L PGA 18 dB gain
                Ana_Set_Reg(AUDENC_ANA_CON3, 0x0809, 0xffff); //PGA stb enhance
                Ana_Set_Reg(AUDENC_ANA_CON0, 0x0581, 0xffff); //Audio L ADC input sel : L PGA, Enable audio L ADC

                //here to set digital part
                Ana_Set_Reg(TOP_CKPDN_CON0, 0x6EFC, 0xffff); //VOW clock power down disable
                //need to enable VOW interrpt?
                Ana_Set_Reg(INT_CON0, 0x0815, 0xffff); //enable VOW interrupt
                //set GPIO
                //Enable VOW_CLK_MISO
                //mt_set_gpio_mode(GPIO_VOW_CLK_MISO_PIN, GPIO_MODE_01); //GPIO148:  mode 1
                //Enable VOW_DAT_MISO
                //mt_set_gpio_mode(GPIO_AUD_DAT_MISO_PIN, GPIO_MODE_02); //GPIO25:  mode 2
                //set PMIC GPIO
                //Ana_Set_Reg(GPIO_MODE3, 0x1251, 0xffff); //GPIO Set to VOW data

                break;

            case AUDIO_VOW_MIC_TYPE_Handset_DMIC:
                printk("%s, case AUDIO_VOW_MIC_TYPE_Handset_DMIC \n", __func__);

                //analog part
                Ana_Set_Reg(LDO_CON2, 0x8103, 0xffff); // LDO enable control by RG_VAUD28_EN, Enable AVDD28_LDO (Default on), LPW control by VAUD28_MODE_SET Enable low power mode
                Ana_Set_Reg(AUDENC_ANA_CON12, 0x0719, 0xffff); //Enable fbdiv relatch (low jitter), Set DCKO = 1/4 F_PLL, Enable VOWPLL CLK
                Ana_Set_Reg(AUDENC_ANA_CON14, 0x0023, 0xffff); //PLL VCOBAND
                Ana_Set_Reg(AUDENC_ANA_CON12, 0x06F9, 0xffff); //PLL devider ratio
                Ana_Set_Reg(AUDENC_ANA_CON13, 0x8180, 0xffff); //PLL low power
                NvregEnable(false); //Disable audio globe bias (Default on)
                Ana_Set_Reg(AUDENC_ANA_CON2, 0x0002, 0xffff); //Enable audio uplink VOW LPW globe bias
                Ana_Set_Reg(AUDENC_ANA_CON9, 0x0A01, 0xffff); //Enable MICBIAS0 lowpower mode, MISBIAS0 = 1P9V
                Ana_Set_Reg(AUDENC_ANA_CON8, 0x0005, 0xffff); //DMIC enable

                //here to set digital part
                Ana_Set_Reg(TOP_CKPDN_CON0, 0x6EFC, 0xffff); //VOW clock power down disable
                //need to enable VOW interrpt?
                Ana_Set_Reg(INT_CON0, 0x0815, 0xffff); //enable VOW interrupt
                //set GPIO
                //Enable VOW_CLK_MISO
                //mt_set_gpio_mode(GPIO_VOW_CLK_MISO_PIN, GPIO_MODE_01); //GPIO148:  mode 1
                //Enable VOW_DAT_MISO
                //mt_set_gpio_mode(GPIO_AUD_DAT_MISO_PIN, GPIO_MODE_02); //GPIO25:  mode 2
                //set PMIC GPIO
                //Ana_Set_Reg(GPIO_MODE3, 0x1251, 0xffff); //GPIO Set to VOW data

                break;

            case AUDIO_VOW_MIC_TYPE_Handset_AMIC:
            default:
                printk("%s, case AUDIO_VOW_MIC_TYPE_Handset_AMIC \n", __func__);
                //analog part
                Ana_Set_Reg(LDO_VCON1, 0x0301, 0xffff); //VA28 remote sense
                Ana_Set_Reg(LDO_CON2, 0x8103, 0xffff); // LDO enable control by RG_VAUD28_EN, Enable AVDD28_LDO (Default on), LPW control by VAUD28_MODE_SET Enable low power mode
                NvregEnable(false); //Disable audio globe bias (Default on)
                Ana_Set_Reg(AUDENC_ANA_CON2, 0x0003, 0xffff); //Enable audio uplink VOW LPW globe bias, Enable audio uplink LPW mode
                Ana_Set_Reg(AUDENC_ANA_CON12, 0x0719, 0xffff); //Enable fbdiv relatch (low jitter), Set DCKO = 1/4 F_PLL, Enable VOWPLL CLK
                Ana_Set_Reg(AUDENC_ANA_CON14, 0x0023, 0xffff); //PLL VCOBAND
                Ana_Set_Reg(AUDENC_ANA_CON12, 0x06F9, 0xffff); //PLL devider ratio
                Ana_Set_Reg(AUDENC_ANA_CON13, 0x8180, 0xffff); //PLL low power
                Ana_Set_Reg(AUDENC_ANA_CON3, 0x0009, 0xffff); //ADC CLK from VOWPLL (12.85/4MHz), Enable Audio ADC FBDAC 0.25FS LPW
                Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0101, 0xffff); //Enable  LCLDO_ENC 1P8V
                Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0006, 0xffff); //LCLDO_ENC remote sense
                Ana_Set_Reg(AUDENC_ANA_CON6, 0x1515, 0xffff);
                Ana_Set_Reg(AUDENC_ANA_CON4, 0x0800, 0xffff);
                Ana_Set_Reg(AUDENC_ANA_CON9, 0x0A01, 0xffff); //Enable MICBIAS0 lowpower mode, MISBIAS0 = 1P9V
                Ana_Set_Reg(AUDENC_ANA_CON0, 0x0041, 0xffff); //Audio L preamplifier input sel : AIN0, Enable audio L PGA
                Ana_Set_Reg(AUDENC_ANA_CON15, 0x0003, 0xffff); //Audio L PGA 18 dB gain
                Ana_Set_Reg(AUDENC_ANA_CON3, 0x0809, 0xffff); //PGA stb enhance
                Ana_Set_Reg(AUDENC_ANA_CON0, 0x0541, 0xffff); //Audio L ADC input sel : L PGA, Enable audio L ADC

                //here to set digital part
                Ana_Set_Reg(TOP_CKPDN_CON0, 0x6EFC, 0xffff); //VOW clock power down disable
                //need to enable VOW interrpt?
                Ana_Set_Reg(INT_CON0, 0x0815, 0xffff); //enable VOW interrupt
                //set GPIO
                //AP side
                //Enable VOW_CLK_MISO
                //mt_set_gpio_mode(GPIO_VOW_CLK_MISO_PIN, GPIO_MODE_01); //GPIO148:  mode 1
                //Enable VOW_DAT_MISO
                //mt_set_gpio_mode(GPIO_AUD_DAT_MISO_PIN, GPIO_MODE_02); //GPIO25:  mode 2
                //set PMIC GPIO
                //Ana_Set_Reg(GPIO_MODE3, 0x1251, 0xffff); //GPIO Set to VOW data
                break;
        }

        //[Todo]Enable VOW INT (has alredy done in pmic.c)
        //enable VOW INT in pmic driver
        //~
#if 1  //Set by HAL
        Ana_Set_Reg(AFE_VOW_CFG0, reg_AFE_VOW_CFG0, 0xffff);   //VOW AMPREF Setting
        Ana_Set_Reg(AFE_VOW_CFG1, reg_AFE_VOW_CFG1, 0xffff);   //VOW A,B timeout initial value
        Ana_Set_Reg(AFE_VOW_CFG2, reg_AFE_VOW_CFG2, 0xffff);   //VOW A,B value setting
        Ana_Set_Reg(AFE_VOW_CFG3, reg_AFE_VOW_CFG3, 0xffff);   //alhpa and beta K value setting
        Ana_Set_Reg(AFE_VOW_CFG4, reg_AFE_VOW_CFG4, 0xffff);   //gamma K value setting
        Ana_Set_Reg(AFE_VOW_CFG5, reg_AFE_VOW_CFG5, 0xffff);   //N mini value setting
#endif
        //move to another digital control
        //Ana_Set_Reg(AFE_VOW_TOP, 0x4810, 0xffff);   //VOW enable

#if defined(VOW_TONE_TEST)
        //test output
        AudDrv_Clk_On();
        OpenAfeDigitaldl1(true);
        OpenAnalogHeadphone(true);
#endif

    }
    else
    {
#if defined (MTK_VOW_SUPPORT)
        //Set VOW driver status first
        VowDrv_EnableHW(false);
#endif
        switch (MicType)
        {
            case AUDIO_VOW_MIC_TYPE_Headset_MIC:
                printk("%s, case AUDIO_VOW_MIC_TYPE_Headset_MIC close\n", __func__);

                // turn off digital first
                //disable VOW interrupt here or when digital power off


                //GPIO set to back to normal record
                //disable VOW_CLK_MISO
                //mt_set_gpio_mode(GPIO_VOW_CLK_MISO_PIN, GPIO_MODE_00); //GPIO148:  mode 0

                //mt_set_gpio_mode(GPIO_AUD_DAT_MISO_PIN, GPIO_MODE_01); //GPIO25:  mode 1
                //set PMIC GPIO
                //Ana_Set_Reg(GPIO_MODE3, 0x1249, 0xffff); //GPIO Set to VOW data

                //turn off analog part
                Ana_Set_Reg(AUDENC_ANA_CON0, 0x0500, 0xffff); //Audio L preamplifier input sel : off, disable audio L PGA
                Ana_Set_Reg(AUDENC_ANA_CON0, 0x0000, 0xffff); //Audio L ADC input sel : off, disable audio L ADC
                Ana_Set_Reg(AUDENC_ANA_CON3, 0x0009, 0xffff); //PGA stb enhance off
                Ana_Set_Reg(AUDENC_ANA_CON15, 0x0000, 0xffff); //Audio L PGA 0 dB gain
                Ana_Set_Reg(AUDENC_ANA_CON4, 0x0000, 0xffff);
                Ana_Set_Reg(AUDENC_ANA_CON6, 0x2020, 0xffff);
                Ana_Set_Reg(AUDENC_ANA_CON9, 0x0000, 0xffff);  //disable MICBIAS0 lowpower mode, MISBIAS0 = 1P7V
                Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0000, 0xffff);  //LCLDO_ENC remote sense off
                Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0001, 0xffff);  //disable  LCLDO_ENC 1P8V
                Ana_Set_Reg(AUDENC_ANA_CON3, 0x0000, 0xffff);  //ADC CLK from VOWPLL (12.85/4MHz) off, Enable Audio ADC FBDAC 0.25FS LPW off
                Ana_Set_Reg(AUDENC_ANA_CON13, 0x0180, 0xffff);  //PLL low power off
                Ana_Set_Reg(AUDENC_ANA_CON12, 0x02F0, 0xffff);  //disable fbdiv relatch (low jitter),Set DCKO = 1/4 F_PLL off, disable VOWPLL CLK
                Ana_Set_Reg(AUDENC_ANA_CON2, 0x0000, 0xffff);  //disable audio uplink VOW LPW globe bias, disable audio uplink LPW mode
                //Ana_Set_Reg(AUDDEC_ANA_CON8, 0x0000, 0xffff);  //enable audio globe bias (Default on)
                NvregEnable(true); //enable audio globe bias (Default on)
                Ana_Set_Reg(LDO_CON2, 0x8102, 0xffff); //LDO enable control by RG_VAUD28_EN, Enable AVDD28_LDO (Default on), LPW control by VAUD28_MODE_SET, disable low power mode

                break;

            case AUDIO_VOW_MIC_TYPE_Handset_DMIC:
                printk("%s, case AUDIO_VOW_MIC_TYPE_Handset_DMIC close\n", __func__);

                // turn off digital first
                //disable VOW interrupt here or when digital power off


                //GPIO set to back to normal record
                //disable VOW_CLK_MISO
                //mt_set_gpio_mode(GPIO_VOW_CLK_MISO_PIN, GPIO_MODE_00); //GPIO148:  mode 0

                //mt_set_gpio_mode(GPIO_AUD_DAT_MISO_PIN, GPIO_MODE_01); //GPIO25:  mode 1
                //set PMIC GPIO
                //Ana_Set_Reg(GPIO_MODE3, 0x1249, 0xffff); //GPIO Set to VOW data

                //turn off analog part
                Ana_Set_Reg(AUDENC_ANA_CON8, 0x0000, 0xffff); //DMIC disable
                Ana_Set_Reg(AUDENC_ANA_CON9, 0x0800, 0xffff); //disable MICBIAS0 lowpower mode, MISBIAS0 = 1P7V
                Ana_Set_Reg(AUDENC_ANA_CON2, 0x0000, 0xffff); //disable audio uplink VOW LPW globe bias
                NvregEnable(true); //enable audio globe bias (Default on)
                Ana_Set_Reg(AUDENC_ANA_CON13, 0x0180, 0xffff);  //PLL low power off
                Ana_Set_Reg(AUDENC_ANA_CON12, 0x02F0, 0xffff);  //disable fbdiv relatch (low jitter),Set DCKO = 1/4 F_PLL off, disable VOWPLL CLK
                Ana_Set_Reg(LDO_CON2, 0x8102, 0xffff); //LDO enable control by RG_VAUD28_EN, Enable AVDD28_LDO (Default on), LPW control by VAUD28_MODE_SET, disable low power mode

                break;

            case AUDIO_VOW_MIC_TYPE_Handset_AMIC:
            default:
                printk("%s, case AUDIO_VOW_MIC_TYPE_Handset_AMIC close\n", __func__);
                // turn off digital first
                //disable VOW interrupt here or when digital power off


                //GPIO set to back to normal record
                //disable VOW_CLK_MISO
                //mt_set_gpio_mode(GPIO_VOW_CLK_MISO_PIN, GPIO_MODE_00); //GPIO148:  mode 0

                //mt_set_gpio_mode(GPIO_AUD_DAT_MISO_PIN, GPIO_MODE_01); //GPIO25:  mode 1
                //set PMIC GPIO
                //Ana_Set_Reg(GPIO_MODE3, 0x1249, 0xffff); //GPIO Set to VOW data

                //turn off analog part
                Ana_Set_Reg(AUDENC_ANA_CON0, 0x0500, 0xffff); //Audio L preamplifier input sel : off, disable audio L PGA
                Ana_Set_Reg(AUDENC_ANA_CON0, 0x0000, 0xffff); //Audio L ADC input sel : off, disable audio L ADC
                Ana_Set_Reg(AUDENC_ANA_CON3, 0x0009, 0xffff); //PGA stb enhance off
                Ana_Set_Reg(AUDENC_ANA_CON15, 0x0000, 0xffff); //Audio L PGA 0 dB gain
                Ana_Set_Reg(AUDENC_ANA_CON4, 0x0000, 0xffff);
                Ana_Set_Reg(AUDENC_ANA_CON6, 0x2020, 0xffff);
                Ana_Set_Reg(AUDENC_ANA_CON9, 0x0000, 0xffff);  //disable MICBIAS0 lowpower mode, MISBIAS0 = 1P7V
                Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0000, 0xffff);  //LCLDO_ENC remote sense off
                Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0001, 0xffff);  //disable  LCLDO_ENC 1P8V
                Ana_Set_Reg(AUDENC_ANA_CON3, 0x0000, 0xffff);  //ADC CLK from VOWPLL (12.85/4MHz) off, Enable Audio ADC FBDAC 0.25FS LPW off
                Ana_Set_Reg(AUDENC_ANA_CON13, 0x0180, 0xffff);  //PLL low power off
                Ana_Set_Reg(AUDENC_ANA_CON12, 0x02F0, 0xffff);  //disable fbdiv relatch (low jitter),Set DCKO = 1/4 F_PLL off, disable VOWPLL CLK
                Ana_Set_Reg(AUDENC_ANA_CON2, 0x0000, 0xffff);  //disable audio uplink VOW LPW globe bias, disable audio uplink LPW mode
                //Ana_Set_Reg(AUDDEC_ANA_CON8, 0x0000, 0xffff);  //enable audio globe bias (Default on)
                NvregEnable(true); //enable audio globe bias (Default on)
                Ana_Set_Reg(LDO_CON2, 0x8102, 0xffff); //LDO enable control by RG_VAUD28_EN, Enable AVDD28_LDO (Default on), LPW control by VAUD28_MODE_SET, disable low power mode

                break;
        }
    }
    return true;
}



// here start uplink power function
static const char *ADC_function[] = {"Off", "On"};
static const char *ADC_power_mode[] = {"normal", "lowpower"};
static const char *PreAmp_Mux_function[] = {"OPEN", "IN_ADC1", "IN_ADC2"};
static const char *ADC_UL_PGA_GAIN[] = { "0Db", "6Db", "12Db", "18Db", "24Db", "30Db"};
static const char *Pmic_Digital_Mux[] = { "ADC1", "ADC2", "ADC3", "ADC4"};
static const char *Adc_Input_Sel[] = { "idle", "AIN", "Preamp"};
static const char *Audio_AnalogMic_Mode[] = { "ACCMODE", "DCCMODE", "DMIC", "DCCECMDIFFMODE", "DCCECMSINGLEMODE"};
static const char *Audio_VOW_ADC_Function[] = {"Off", "On"};
static const char *Audio_VOW_Digital_Function[] = {"Off", "On"};
static const char *Audio_VOW_MIC_Type[] = {"HandsetAMIC", "HeadsetMIC", "HandsetDMIC"};


static const struct soc_enum Audio_UL_Enum[] =
{
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ADC_function), ADC_function),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ADC_function), ADC_function),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ADC_function), ADC_function),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ADC_function), ADC_function),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(PreAmp_Mux_function), PreAmp_Mux_function),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Adc_Input_Sel), Adc_Input_Sel),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Adc_Input_Sel), Adc_Input_Sel),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Adc_Input_Sel), Adc_Input_Sel),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Adc_Input_Sel), Adc_Input_Sel),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ADC_UL_PGA_GAIN), ADC_UL_PGA_GAIN),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ADC_UL_PGA_GAIN), ADC_UL_PGA_GAIN),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ADC_UL_PGA_GAIN), ADC_UL_PGA_GAIN),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ADC_UL_PGA_GAIN), ADC_UL_PGA_GAIN),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Pmic_Digital_Mux), Pmic_Digital_Mux),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Pmic_Digital_Mux), Pmic_Digital_Mux),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Pmic_Digital_Mux), Pmic_Digital_Mux),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Pmic_Digital_Mux), Pmic_Digital_Mux),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Audio_AnalogMic_Mode), Audio_AnalogMic_Mode),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Audio_AnalogMic_Mode), Audio_AnalogMic_Mode),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Audio_AnalogMic_Mode), Audio_AnalogMic_Mode),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Audio_AnalogMic_Mode), Audio_AnalogMic_Mode),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ADC_power_mode), ADC_power_mode),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Audio_VOW_ADC_Function), Audio_VOW_ADC_Function),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(PreAmp_Mux_function), PreAmp_Mux_function),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Audio_VOW_Digital_Function), Audio_VOW_Digital_Function),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Audio_VOW_MIC_Type), Audio_VOW_MIC_Type),
};

static int Audio_ADC1_Get(struct snd_kcontrol *kcontrol,
                          struct snd_ctl_elem_value *ucontrol)
{
    printk("Audio_ADC1_Get = %d\n", mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC1]);
    ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC1];
    return 0;
}

static int Audio_ADC1_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()\n", __func__);
    mutex_lock(&Ana_Power_Mutex);
    if (ucontrol->value.integer.value[0])
    {
        if (mAudio_Analog_Mic1_mode == AUDIO_ANALOGUL_MODE_ACC)
        {
            TurnOnADcPowerACC(AUDIO_ANALOG_DEVICE_IN_ADC1 , true);
        }
        else if (mAudio_Analog_Mic1_mode == AUDIO_ANALOGUL_MODE_DCC)
        {
            TurnOnADcPowerDCC(AUDIO_ANALOG_DEVICE_IN_ADC1 , true);
        }
        else if (mAudio_Analog_Mic1_mode == AUDIO_ANALOGUL_MODE_DMIC)
        {
            TurnOnADcPowerDmic(AUDIO_ANALOG_DEVICE_IN_ADC1, true);
        }
        else if (mAudio_Analog_Mic1_mode == AUDIO_ANALOGUL_MODE_DCCECMDIFF || mAudio_Analog_Mic1_mode == AUDIO_ANALOGUL_MODE_DCCECMSINGLE)
        {
            TurnOnADcPowerDCCECM(AUDIO_ANALOG_DEVICE_IN_ADC1 , true);
        }
        mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC1] = ucontrol->value.integer.value[0];
    }
    else
    {
        mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC1] = ucontrol->value.integer.value[0];
        if (mAudio_Analog_Mic1_mode == AUDIO_ANALOGUL_MODE_ACC)
        {
            TurnOnADcPowerACC(AUDIO_ANALOG_DEVICE_IN_ADC1 , false);
        }
        else if (mAudio_Analog_Mic1_mode == AUDIO_ANALOGUL_MODE_DCC)
        {
            TurnOnADcPowerDCC(AUDIO_ANALOG_DEVICE_IN_ADC1 , false);
        }
        else if (mAudio_Analog_Mic1_mode == AUDIO_ANALOGUL_MODE_DMIC)
        {
            TurnOnADcPowerDmic(AUDIO_ANALOG_DEVICE_IN_ADC1, false);
        }
        else if (mAudio_Analog_Mic1_mode == AUDIO_ANALOGUL_MODE_DCCECMDIFF || mAudio_Analog_Mic1_mode == AUDIO_ANALOGUL_MODE_DCCECMSINGLE)
        {
            TurnOnADcPowerDCCECM(AUDIO_ANALOG_DEVICE_IN_ADC1, false);
        }
    }
    mutex_unlock(&Ana_Power_Mutex);
    return 0;
}

static int Audio_ADC2_Get(struct snd_kcontrol *kcontrol,
                          struct snd_ctl_elem_value *ucontrol)
{
    printk("Audio_ADC2_Get = %d\n", mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC2]);
    ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC2];
    return 0;
}

static int Audio_ADC2_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()\n", __func__);
    mutex_lock(&Ana_Power_Mutex);
    if (ucontrol->value.integer.value[0])
    {
        if (mAudio_Analog_Mic2_mode == AUDIO_ANALOGUL_MODE_ACC)
        {
            TurnOnADcPowerACC(AUDIO_ANALOG_DEVICE_IN_ADC2 , true);
        }
        else if (mAudio_Analog_Mic2_mode == AUDIO_ANALOGUL_MODE_DCC)
        {
            TurnOnADcPowerDCC(AUDIO_ANALOG_DEVICE_IN_ADC2 , true);
        }
        else if (mAudio_Analog_Mic2_mode == AUDIO_ANALOGUL_MODE_DMIC)
        {
            TurnOnADcPowerDmic(AUDIO_ANALOG_DEVICE_IN_ADC2 , true);
        }
        else if (mAudio_Analog_Mic2_mode == AUDIO_ANALOGUL_MODE_DCCECMDIFF || mAudio_Analog_Mic2_mode == AUDIO_ANALOGUL_MODE_DCCECMSINGLE)
        {
            TurnOnADcPowerDCCECM(AUDIO_ANALOG_DEVICE_IN_ADC2 , true);
        }
        mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC2] = ucontrol->value.integer.value[0];
    }
    else
    {
        mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC2] = ucontrol->value.integer.value[0];
        if (mAudio_Analog_Mic2_mode == AUDIO_ANALOGUL_MODE_ACC)
        {
            TurnOnADcPowerACC(AUDIO_ANALOG_DEVICE_IN_ADC2 , false);
        }
        else if (mAudio_Analog_Mic2_mode == AUDIO_ANALOGUL_MODE_DCC)
        {
            TurnOnADcPowerDCC(AUDIO_ANALOG_DEVICE_IN_ADC2 , false);
        }
        else if (mAudio_Analog_Mic2_mode == AUDIO_ANALOGUL_MODE_DMIC)
        {
            TurnOnADcPowerDmic(AUDIO_ANALOG_DEVICE_IN_ADC2 , false);
        }
        else if (mAudio_Analog_Mic2_mode == AUDIO_ANALOGUL_MODE_DCCECMDIFF || mAudio_Analog_Mic2_mode == AUDIO_ANALOGUL_MODE_DCCECMSINGLE)
        {
            TurnOnADcPowerDCCECM(AUDIO_ANALOG_DEVICE_IN_ADC2 , false);
        }
    }
    mutex_unlock(&Ana_Power_Mutex);
    return 0;
}

static int Audio_ADC3_Get(struct snd_kcontrol *kcontrol,
                          struct snd_ctl_elem_value *ucontrol)
{
#if 0 //K2 removed
    printk("Audio_ADC3_Get = %d\n", mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC3]);
    ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC3];
#endif
    return 0;
}

static int Audio_ADC3_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
#if 0 //K2 removed
    printk("%s()\n", __func__);
    mutex_lock(&Ana_Power_Mutex);
    if (ucontrol->value.integer.value[0])
    {
        if (mAudio_Analog_Mic3_mode == AUDIO_ANALOGUL_MODE_ACC)
        {
            TurnOnADcPowerACC(AUDIO_ANALOG_DEVICE_IN_ADC3 , true);
        }
        else if (mAudio_Analog_Mic3_mode == AUDIO_ANALOGUL_MODE_DCC)
        {
            TurnOnADcPowerDCC(AUDIO_ANALOG_DEVICE_IN_ADC3 , true);
        }
        else if (mAudio_Analog_Mic3_mode == AUDIO_ANALOGUL_MODE_DMIC)
        {
            TurnOnADcPowerDmic(AUDIO_ANALOG_DEVICE_IN_ADC3 , true);
        }
        else if (mAudio_Analog_Mic3_mode == AUDIO_ANALOGUL_MODE_DCCECMDIFF || mAudio_Analog_Mic3_mode == AUDIO_ANALOGUL_MODE_DCCECMSINGLE)
        {
            TurnOnADcPowerDCCECM(AUDIO_ANALOG_DEVICE_IN_ADC3 , true);
        }
        mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC3] = ucontrol->value.integer.value[0];
    }
    else
    {
        mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC3] = ucontrol->value.integer.value[0];

        if (mAudio_Analog_Mic3_mode == AUDIO_ANALOGUL_MODE_ACC)
        {
            TurnOnADcPowerACC(AUDIO_ANALOG_DEVICE_IN_ADC3 , false);
        }
        else if (mAudio_Analog_Mic3_mode == AUDIO_ANALOGUL_MODE_DCC)
        {
            TurnOnADcPowerDCC(AUDIO_ANALOG_DEVICE_IN_ADC3 , false);
        }
        else if (mAudio_Analog_Mic3_mode == AUDIO_ANALOGUL_MODE_DMIC)
        {
            TurnOnADcPowerDmic(AUDIO_ANALOG_DEVICE_IN_ADC3 , false);
        }
        else if (mAudio_Analog_Mic3_mode == AUDIO_ANALOGUL_MODE_DCCECMDIFF || mAudio_Analog_Mic3_mode == AUDIO_ANALOGUL_MODE_DCCECMSINGLE)
        {
            TurnOnADcPowerDCCECM(AUDIO_ANALOG_DEVICE_IN_ADC3 , false);
        }
    }
    mutex_unlock(&Ana_Power_Mutex);
#endif
    return 0;
}

static int Audio_ADC4_Get(struct snd_kcontrol *kcontrol,
                          struct snd_ctl_elem_value *ucontrol)
{
#if 0 //K2 removed
    printk("Audio_ADC4_Get = %d\n", mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC4]);
    ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC4];
#endif
    return 0;
}

static int Audio_ADC4_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
#if 0 //K2 removed
    printk("%s()\n", __func__);
    mutex_lock(&Ana_Power_Mutex);
    if (ucontrol->value.integer.value[0])
    {
        if (mAudio_Analog_Mic4_mode == AUDIO_ANALOGUL_MODE_ACC)
        {
            TurnOnADcPowerACC(AUDIO_ANALOG_DEVICE_IN_ADC4 , true);
        }
        else if (mAudio_Analog_Mic4_mode == AUDIO_ANALOGUL_MODE_DCC)
        {
            TurnOnADcPowerDCC(AUDIO_ANALOG_DEVICE_IN_ADC4 , true);
        }
        else if (mAudio_Analog_Mic4_mode == AUDIO_ANALOGUL_MODE_DMIC)
        {
            TurnOnADcPowerDmic(AUDIO_ANALOG_DEVICE_IN_ADC4 , true);
        }
        else if (mAudio_Analog_Mic4_mode == AUDIO_ANALOGUL_MODE_DCCECMDIFF || mAudio_Analog_Mic4_mode == AUDIO_ANALOGUL_MODE_DCCECMSINGLE)
        {
            TurnOnADcPowerDCCECM(AUDIO_ANALOG_DEVICE_IN_ADC4 , true);
        }
        mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC4] = ucontrol->value.integer.value[0];
    }
    else
    {
        mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC4] = ucontrol->value.integer.value[0];
        if (mAudio_Analog_Mic4_mode == AUDIO_ANALOGUL_MODE_ACC)
        {
            TurnOnADcPowerACC(AUDIO_ANALOG_DEVICE_IN_ADC4 , false);
        }
        else if (mAudio_Analog_Mic4_mode == AUDIO_ANALOGUL_MODE_DCC)
        {
            TurnOnADcPowerDCC(AUDIO_ANALOG_DEVICE_IN_ADC4 , false);
        }
        else if (mAudio_Analog_Mic4_mode == AUDIO_ANALOGUL_MODE_DMIC)
        {
            TurnOnADcPowerDmic(AUDIO_ANALOG_DEVICE_IN_ADC4 , false);
        }
        else if (mAudio_Analog_Mic4_mode == AUDIO_ANALOGUL_MODE_DCCECMDIFF || mAudio_Analog_Mic4_mode == AUDIO_ANALOGUL_MODE_DCCECMSINGLE)
        {
            TurnOnADcPowerDCCECM(AUDIO_ANALOG_DEVICE_IN_ADC4 , false);
        }
    }
    mutex_unlock(&Ana_Power_Mutex);
#endif
    return 0;
}

static int Audio_ADC1_Sel_Get(struct snd_kcontrol *kcontrol,
                              struct snd_ctl_elem_value *ucontrol)
{
    printk("%s() = %d\n", __func__, mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC1]);
    ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC1];
    return 0;
}

static int Audio_ADC1_Sel_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()\n", __func__);

    if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Adc_Input_Sel))
    {
        printk("return -EINVAL\n");
        return -EINVAL;
    }
    if (ucontrol->value.integer.value[0] == 0)
    {
        Ana_Set_Reg(AUDENC_ANA_CON0, (0x0000 << 9), 0x0600);  // pinumx sel
    }
    else if (ucontrol->value.integer.value[0] == 1)
    {
        Ana_Set_Reg(AUDENC_ANA_CON0, (0x0001 << 9), 0x0600); //AIN0
    }
    // ADC2
    else if (ucontrol->value.integer.value[0] == 2)
    {
        Ana_Set_Reg(AUDENC_ANA_CON0, (0x0002 << 9), 0x0600); //Left preamp
    }
    else
    {
        printk("%s() warning \n ", __func__);
    }
    printk("%s() done \n", __func__);
    mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC1] = ucontrol->value.integer.value[0];
    return 0;
}

static int Audio_ADC2_Sel_Get(struct snd_kcontrol *kcontrol,
                              struct snd_ctl_elem_value *ucontrol)
{
    printk("%s() = %d\n", __func__, mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC2]);
    ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC2];
    return 0;
}

static int Audio_ADC2_Sel_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()\n", __func__);
    if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Adc_Input_Sel))
    {
        printk("return -EINVAL\n");
        return -EINVAL;
    }
    if (ucontrol->value.integer.value[0] == 0)
    {
        Ana_Set_Reg(AUDENC_ANA_CON1, (0x0000 << 9), 0x0600);  // pinumx sel
    }
    else if (ucontrol->value.integer.value[0] == 1)
    {
        Ana_Set_Reg(AUDENC_ANA_CON1, (0x0001 << 9), 0x0600); //AIN2
    }
    else if (ucontrol->value.integer.value[0] == 2) //Right preamp
    {
        Ana_Set_Reg(AUDENC_ANA_CON1, (0x0002 << 9), 0x0600);
    }
    else
    {
        printk("%s() warning \n ", __func__);
    }
    printk("%s() done \n", __func__);
    mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC2] = ucontrol->value.integer.value[0];
    return 0;
}


static int Audio_ADC3_Sel_Get(struct snd_kcontrol *kcontrol,
                              struct snd_ctl_elem_value *ucontrol)
{
#if 0 //K2 removed
    printk("%s() = %d\n", __func__, mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC3]);
    ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC3];
#endif
    return 0;
}

static int Audio_ADC3_Sel_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
#if 0 //K2 removed
    printk("%s()\n", __func__);

    if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Adc_Input_Sel))
    {
        printk("return -EINVAL\n");
        return -EINVAL;
    }

    if (ucontrol->value.integer.value[0] == 0)
    {
        Ana_Set_Reg(AUDPREAMP_CFG1, (0x0000 << 8), 0x0300);  // pinumx sel
    }
    else if (ucontrol->value.integer.value[0] == 1)
    {
        Ana_Set_Reg(AUDPREAMP_CFG1, (0x0001 << 8), 0x0300);
    }
    else if (ucontrol->value.integer.value[0] == 2)
    {
        Ana_Set_Reg(AUDPREAMP_CFG1, (0x0002 << 8), 0x0300);
    }
    else
    {
        printk("%s() warning \n ", __func__);
    }
    printk("%s() done \n", __func__);
    mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC3] = ucontrol->value.integer.value[0];
#endif
    return 0;
}


static int Audio_ADC4_Sel_Get(struct snd_kcontrol *kcontrol,
                              struct snd_ctl_elem_value *ucontrol)
{
#if 0 //K2 removed
    printk("%s() = %d\n", __func__, mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC4]);
    ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC4];
#endif
    return 0;
}

static int Audio_ADC4_Sel_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
#if 0 //K2 removed
    printk("%s()\n", __func__);

    if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Adc_Input_Sel))
    {
        printk("return -EINVAL\n");
        return -EINVAL;
    }

    if (ucontrol->value.integer.value[0] == 0)
    {
        Ana_Set_Reg(AUDPREAMP_CFG1, (0x0000 << 12), 0x1800);  // pinumx sel
    }
    else if (ucontrol->value.integer.value[0] == 1)
    {
        Ana_Set_Reg(AUDPREAMP_CFG1, (0x0001 << 12), 0x1800);
    }
    else if (ucontrol->value.integer.value[0] == 2)
    {
        Ana_Set_Reg(AUDPREAMP_CFG1, (0x0002 << 12), 0x1800);
    }
    else
    {
        printk("%s() warning \n ", __func__);
    }
    printk("%s() done \n", __func__);
    mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC4] = ucontrol->value.integer.value[0];
#endif
    return 0;
}


static bool AudioPreAmp1_Sel(int Mul_Sel)
{
    printk("%s Mul_Sel = %d ", __func__, Mul_Sel);
    if (Mul_Sel == 0)
    {
        Ana_Set_Reg(AUDENC_ANA_CON0, 0x0000, 0x00C0); // pinumx open
    }
    else if (Mul_Sel == 1)
    {
        Ana_Set_Reg(AUDENC_ANA_CON0, 0x0040, 0x00C0); // AIN0
    }
    else if (Mul_Sel == 2)
    {
        Ana_Set_Reg(AUDENC_ANA_CON0, 0x0080, 0x00C0); // AIN1
    }
    else
    {
        printk("AudioPreAmp1_Sel warning");
    }
    return true;
}


static int Audio_PreAmp1_Get(struct snd_kcontrol *kcontrol,
                             struct snd_ctl_elem_value *ucontrol)
{
    printk("%s() mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_PREAMP_1]; = %d\n", __func__, mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_PREAMP_1]);
    ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_PREAMP_1];
    return 0;
}

static int Audio_PreAmp1_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()\n", __func__);

    if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(PreAmp_Mux_function))
    {
        printk("return -EINVAL\n");
        return -EINVAL;
    }
    mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_PREAMP_1] = ucontrol->value.integer.value[0];
    AudioPreAmp1_Sel(mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_PREAMP_1]);
    printk("%s() done \n", __func__);
    return 0;
}

static bool AudioPreAmp2_Sel(int Mul_Sel)
{
    printk("%s Mul_Sel = %d ", __func__, Mul_Sel);
    if (Mul_Sel == 0)
    {
        Ana_Set_Reg(AUDENC_ANA_CON1, 0x0000, 0x00C0); // pinumx open
    }
    else if (Mul_Sel == 1)
    {
        Ana_Set_Reg(AUDENC_ANA_CON1, 0x00C0, 0x00C0); // AIN2
    }
    else if (Mul_Sel == 2)
    {
        Ana_Set_Reg(AUDENC_ANA_CON1, 0x0080, 0x00C0); // AIN1
    }
    else if (Mul_Sel == 3)
    {
        Ana_Set_Reg(AUDENC_ANA_CON1, 0x0040, 0x00C0); // AIN0
    }
    else
    {
        printk("AudioPreAmp1_Sel warning");
    }
    return true;
}


static int Audio_PreAmp2_Get(struct snd_kcontrol *kcontrol,
                             struct snd_ctl_elem_value *ucontrol)
{
    printk("%s() mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_PREAMP_2]; = %d\n", __func__, mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_PREAMP_2]);
    ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_PREAMP_2];
    return 0;
}

static int Audio_PreAmp2_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()\n", __func__);

    if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(PreAmp_Mux_function))
    {
        printk("return -EINVAL\n");
        return -EINVAL;
    }
    mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_PREAMP_2] = ucontrol->value.integer.value[0];
    AudioPreAmp2_Sel(mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_PREAMP_2]);
    printk("%s() done \n", __func__);
    return 0;
}

//PGA1: PGA_L
static int Audio_PGA1_Get(struct snd_kcontrol *kcontrol,
                          struct snd_ctl_elem_value *ucontrol)
{
    printk("Audio_AmpR_Get = %d\n", mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP1]);
    ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP1];
    return 0;
}

static int Audio_PGA1_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    int index = 0;
    printk("%s()\n", __func__);
    if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(ADC_UL_PGA_GAIN))
    {
        printk("return -EINVAL\n");
        return -EINVAL;
    }
    index = ucontrol->value.integer.value[0];
    Ana_Set_Reg(AUDENC_ANA_CON15, index , 0x0007);
    mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP1] = ucontrol->value.integer.value[0];
    return 0;
}

//PGA2: PGA_R
static int Audio_PGA2_Get(struct snd_kcontrol *kcontrol,
                          struct snd_ctl_elem_value *ucontrol)
{
    printk("Audio_PGA2_Get = %d\n", mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP2]);
    ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP2];
    return 0;
}

static int Audio_PGA2_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    int index = 0;
    printk("%s()\n", __func__);
    if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(ADC_UL_PGA_GAIN))
    {
        printk("return -EINVAL\n");
        return -EINVAL;
    }
    index = ucontrol->value.integer.value[0];
    Ana_Set_Reg(AUDENC_ANA_CON15, index << 4, 0x0070);
    mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP2] = ucontrol->value.integer.value[0];
    return 0;
}


static int Audio_PGA3_Get(struct snd_kcontrol *kcontrol,
                          struct snd_ctl_elem_value *ucontrol)
{
#if 0 //K2 removed
    printk("Audio_AmpR_Get = %d\n", mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP3]);
    ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP3];
#endif
    return 0;
}

static int Audio_PGA3_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
#if 0 //K2 removed
    int index = 0;
    printk("%s()\n", __func__);
    if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(ADC_UL_PGA_GAIN))
    {
        printk("return -EINVAL\n");
        return -EINVAL;
    }
    index = ucontrol->value.integer.value[0];
    Ana_Set_Reg(AUDPREAMPGAIN_CFG0, index << 8, 0x0700);
    mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP3] = ucontrol->value.integer.value[0];
#endif
    return 0;
}

static int Audio_PGA4_Get(struct snd_kcontrol *kcontrol,
                          struct snd_ctl_elem_value *ucontrol)
{
#if 0 //K2 removed
    printk("Audio_AmpR_Get = %d \n", mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP4]);
    ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP4];
#endif
    return 0;
}

static int Audio_PGA4_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
#if 0 //K2 removed
    int index = 0;
    printk("%s()\n", __func__);
    if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(ADC_UL_PGA_GAIN))
    {
        printk("return -EINVAL\n");
        return -EINVAL;
    }
    index = ucontrol->value.integer.value[0];
    Ana_Set_Reg(AUDPREAMPGAIN_CFG0, index << 12, 0x7000);
    mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP4] = ucontrol->value.integer.value[0];
#endif
    return 0;
}

static int Audio_MicSource1_Get(struct snd_kcontrol *kcontrol,
                                struct snd_ctl_elem_value *ucontrol)
{
    printk("Audio_MicSource1_Get = %d\n", mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1]);
    ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1];
    return 0;
}

static int Audio_MicSource1_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
#if 0 //K2 removed
    int index = 0;
    printk("%s()\n", __func__);
    if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Pmic_Digital_Mux))
    {
        printk("return -EINVAL\n");
        return -EINVAL;
    }
    index = ucontrol->value.integer.value[0];
    printk("%s() index = %d done \n", __func__, index);
    Ana_Set_Reg(AFE_MIC_ARRAY_CFG, index | index << 8, 0x0303);
    mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] = ucontrol->value.integer.value[0];
#else //K2 used for ADC1 Mic source selection, "ADC1" is main_mic, "ADC2" is headset_mic
    int index = 0;
    printk("%s()\n", __func__);
    if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Pmic_Digital_Mux))
    {
        printk("return -EINVAL\n");
        return -EINVAL;
    }
    index = ucontrol->value.integer.value[0];
    printk("%s() index = %d done \n", __func__, index);
    mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] = ucontrol->value.integer.value[0];
#endif
    return 0;
}

static int Audio_MicSource2_Get(struct snd_kcontrol *kcontrol,
                                struct snd_ctl_elem_value *ucontrol)
{
#if 0 //K2 removed
    printk("%s() = %d\n", __func__, mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_2]);
    ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_2];
#endif
    return 0;
}

static int Audio_MicSource2_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
#if 0 //K2 removed
    int index = 0;
    printk("%s()\n", __func__);
    if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Pmic_Digital_Mux))
    {
        printk("return -EINVAL\n");
        return -EINVAL;
    }
    index = ucontrol->value.integer.value[0];
    printk("%s() done \n", __func__);
    Ana_Set_Reg(AFE_MIC_ARRAY_CFG, index << 2 | index << 10, 0x0c0c);
    mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_2] = ucontrol->value.integer.value[0];
#endif
    return 0;
}


static int Audio_MicSource3_Get(struct snd_kcontrol *kcontrol,
                                struct snd_ctl_elem_value *ucontrol)
{
#if 0  //K2 removed
    printk("%s() = %d\n", __func__, mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_3]);
    ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_3];
#endif
    return 0;
}

static int Audio_MicSource3_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
#if 0  //K2 removed
    int index = 0;
    printk("%s()\n", __func__);
    if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Pmic_Digital_Mux))
    {
        printk("return -EINVAL\n");
        return -EINVAL;
    }
    index = ucontrol->value.integer.value[0];
    printk("%s() done \n", __func__);
    Ana_Set_Reg(AFE_MIC_ARRAY_CFG, index << 4 | index << 12, 0x3030);
    mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_3] = ucontrol->value.integer.value[0];
#endif
    return 0;
}


static int Audio_MicSource4_Get(struct snd_kcontrol *kcontrol,
                                struct snd_ctl_elem_value *ucontrol)
{
#if 0  //K2 removed
    printk("%s() = %d\n", __func__, mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_4]);
    ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_4];
#endif
    return 0;
}

static int Audio_MicSource4_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
#if 0  //K2 removed
    int index = 0;
    printk("%s()\n", __func__);
    if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Pmic_Digital_Mux))
    {
        printk("return -EINVAL\n");
        return -EINVAL;
    }
    index = ucontrol->value.integer.value[0];
    printk("%s() done \n", __func__);
    Ana_Set_Reg(AFE_MIC_ARRAY_CFG, index << 6 | index << 14, 0xc0c0);
    mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_4] = ucontrol->value.integer.value[0];
#endif
    return 0;
}

// Mic ACC/DCC Mode Setting
static int Audio_Mic1_Mode_Select_Get(struct snd_kcontrol *kcontrol,
                                      struct snd_ctl_elem_value *ucontrol)
{
    printk("%s() mAudio_Analog_Mic1_mode = %d\n", __func__, mAudio_Analog_Mic1_mode);
    ucontrol->value.integer.value[0] = mAudio_Analog_Mic1_mode;
    return 0;
}

static int Audio_Mic1_Mode_Select_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()\n", __func__);
    if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Audio_AnalogMic_Mode))
    {
        printk("return -EINVAL\n");
        return -EINVAL;
    }
    mAudio_Analog_Mic1_mode = ucontrol->value.integer.value[0];
    printk("%s() mAudio_Analog_Mic1_mode = %d \n", __func__, mAudio_Analog_Mic1_mode);
    return 0;
}

static int Audio_Mic2_Mode_Select_Get(struct snd_kcontrol *kcontrol,
                                      struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()  = %d\n", __func__, mAudio_Analog_Mic2_mode);
    ucontrol->value.integer.value[0] = mAudio_Analog_Mic2_mode;
    return 0;
}

static int Audio_Mic2_Mode_Select_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()\n", __func__);
    if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Audio_AnalogMic_Mode))
    {
        printk("return -EINVAL\n");
        return -EINVAL;
    }
    mAudio_Analog_Mic2_mode = ucontrol->value.integer.value[0];
    printk("%s() mAudio_Analog_Mic1_mode = %d \n", __func__, mAudio_Analog_Mic2_mode);
    return 0;
}


static int Audio_Mic3_Mode_Select_Get(struct snd_kcontrol *kcontrol,
                                      struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()  = %d\n", __func__, mAudio_Analog_Mic3_mode);
    ucontrol->value.integer.value[0] = mAudio_Analog_Mic3_mode;
    return 0;
}

static int Audio_Mic3_Mode_Select_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()\n", __func__);
    if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Audio_AnalogMic_Mode))
    {
        printk("return -EINVAL\n");
        return -EINVAL;
    }
    mAudio_Analog_Mic3_mode = ucontrol->value.integer.value[0];
    printk("%s() mAudio_Analog_Mic1_mode = %d \n", __func__, mAudio_Analog_Mic3_mode);
    return 0;
}

static int Audio_Mic4_Mode_Select_Get(struct snd_kcontrol *kcontrol,
                                      struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()  = %d\n", __func__, mAudio_Analog_Mic4_mode);
    ucontrol->value.integer.value[0] = mAudio_Analog_Mic4_mode;
    return 0;
}

static int Audio_Mic4_Mode_Select_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()\n", __func__);
    if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Audio_AnalogMic_Mode))
    {
        printk("return -EINVAL\n");
        return -EINVAL;
    }
    mAudio_Analog_Mic4_mode = ucontrol->value.integer.value[0];
    printk("%s() mAudio_Analog_Mic1_mode = %d \n", __func__, mAudio_Analog_Mic4_mode);
    return 0;
}

static int Audio_Adc_Power_Mode_Get(struct snd_kcontrol *kcontrol,
                                    struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()  = %d\n", __func__, mAdc_Power_Mode);
    ucontrol->value.integer.value[0] = mAdc_Power_Mode;
    return 0;
}

static int Audio_Adc_Power_Mode_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()\n", __func__);
    if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(ADC_power_mode))
    {
        printk("return -EINVAL\n");
        return -EINVAL;
    }
    mAdc_Power_Mode = ucontrol->value.integer.value[0];
    printk("%s() mAudio_Analog_Mic1_mode = %d \n", __func__, mAdc_Power_Mode);
    return 0;
}


static int Audio_Vow_ADC_Func_Switch_Get(struct snd_kcontrol *kcontrol,
                                         struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()  = %d\n", __func__, mAudio_Vow_Analog_Func_Enable);
    ucontrol->value.integer.value[0] = mAudio_Vow_Analog_Func_Enable;
    return 0;
}

static int Audio_Vow_ADC_Func_Switch_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()\n", __func__);
    if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Audio_VOW_ADC_Function))
    {
        printk("return -EINVAL\n");
        return -EINVAL;
    }

    if (ucontrol->value.integer.value[0])
    {
        TurnOnVOWADcPowerACC(mAudio_VOW_Mic_type, true);
    }
    else
    {
        TurnOnVOWADcPowerACC(mAudio_VOW_Mic_type, false);
    }

    mAudio_Vow_Analog_Func_Enable = ucontrol->value.integer.value[0];
    printk("%s() mAudio_Analog_Mic1_mode = %d \n", __func__, mAudio_Vow_Analog_Func_Enable);
    return 0;
}

static int Audio_Vow_Digital_Func_Switch_Get(struct snd_kcontrol *kcontrol,
                                             struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()  = %d\n", __func__, mAudio_Vow_Digital_Func_Enable);
    ucontrol->value.integer.value[0] = mAudio_Vow_Digital_Func_Enable;
    return 0;
}

static int Audio_Vow_Digital_Func_Switch_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()\n", __func__);
    if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Audio_VOW_Digital_Function))
    {
        printk("return -EINVAL\n");
        return -EINVAL;
    }

    if (ucontrol->value.integer.value[0])
    {
        TurnOnVOWDigitalHW(true);
    }
    else
    {
        TurnOnVOWDigitalHW(false);
    }

    mAudio_Vow_Digital_Func_Enable = ucontrol->value.integer.value[0];
    printk("%s() mAudio_Analog_Mic1_mode = %d \n", __func__, mAudio_Vow_Digital_Func_Enable);
    return 0;
}


static int Audio_Vow_MIC_Type_Select_Get(struct snd_kcontrol *kcontrol,
                                         struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()  = %d\n", __func__, mAudio_VOW_Mic_type);
    ucontrol->value.integer.value[0] = mAudio_VOW_Mic_type;
    return 0;
}

static int Audio_Vow_MIC_Type_Select_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()\n", __func__);
    if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Audio_VOW_MIC_Type))
    {
        printk("return -EINVAL\n");
        return -EINVAL;
    }
    mAudio_VOW_Mic_type = ucontrol->value.integer.value[0];
    printk("%s() mAudio_VOW_Mic_type = %d \n", __func__, mAudio_VOW_Mic_type);
    return 0;
}


static int Audio_Vow_Cfg0_Get(struct snd_kcontrol *kcontrol,
                              struct snd_ctl_elem_value *ucontrol)
{
    int value = /*Ana_Get_Reg(AFE_VOW_CFG0)*/reg_AFE_VOW_CFG0;
    printk("%s()  = %d\n", __func__, value);
    ucontrol->value.integer.value[0] = value;
    return 0;
}

static int Audio_Vow_Cfg0_Set(struct snd_kcontrol *kcontrol,
                              struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()  = %ld\n", __func__, ucontrol->value.integer.value[0]);
    //Ana_Set_Reg(AFE_VOW_CFG0, ucontrol->value.integer.value[0], 0xffff);
    reg_AFE_VOW_CFG0 = ucontrol->value.integer.value[0];
    return 0;
}

static int Audio_Vow_Cfg1_Get(struct snd_kcontrol *kcontrol,
                              struct snd_ctl_elem_value *ucontrol)
{
    int value = /*Ana_Get_Reg(AFE_VOW_CFG1)*/reg_AFE_VOW_CFG1;
    printk("%s()  = %d\n", __func__, value);
    ucontrol->value.integer.value[0] = value;
    return 0;
}

static int Audio_Vow_Cfg1_Set(struct snd_kcontrol *kcontrol,
                              struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()  = %ld\n", __func__, ucontrol->value.integer.value[0]);
    //Ana_Set_Reg(AFE_VOW_CFG1, ucontrol->value.integer.value[0], 0xffff);
    reg_AFE_VOW_CFG1 = ucontrol->value.integer.value[0];
    return 0;
}

static int Audio_Vow_Cfg2_Get(struct snd_kcontrol *kcontrol,
                              struct snd_ctl_elem_value *ucontrol)
{
    int value = /*Ana_Get_Reg(AFE_VOW_CFG2)*/reg_AFE_VOW_CFG2;
    printk("%s()  = %d\n", __func__, value);
    ucontrol->value.integer.value[0] = value;
    return 0;
}

static int Audio_Vow_Cfg2_Set(struct snd_kcontrol *kcontrol,
                              struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()  = %ld\n", __func__, ucontrol->value.integer.value[0]);
    //Ana_Set_Reg(AFE_VOW_CFG2, ucontrol->value.integer.value[0], 0xffff);
    reg_AFE_VOW_CFG2 = ucontrol->value.integer.value[0];
    return 0;
}

static int Audio_Vow_Cfg3_Get(struct snd_kcontrol *kcontrol,
                              struct snd_ctl_elem_value *ucontrol)
{
    int value = /*Ana_Get_Reg(AFE_VOW_CFG3)*/reg_AFE_VOW_CFG3;
    printk("%s()  = %d\n", __func__, value);
    ucontrol->value.integer.value[0] = value;
    return 0;
}

static int Audio_Vow_Cfg3_Set(struct snd_kcontrol *kcontrol,
                              struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()  = %ld\n", __func__, ucontrol->value.integer.value[0]);
    //Ana_Set_Reg(AFE_VOW_CFG3, ucontrol->value.integer.value[0], 0xffff);
    reg_AFE_VOW_CFG3 = ucontrol->value.integer.value[0];
    return 0;
}

static int Audio_Vow_Cfg4_Get(struct snd_kcontrol *kcontrol,
                              struct snd_ctl_elem_value *ucontrol)
{
    int value = /*Ana_Get_Reg(AFE_VOW_CFG4)*/reg_AFE_VOW_CFG4;
    printk("%s()  = %d\n", __func__, value);
    ucontrol->value.integer.value[0] = value;
    return 0;
}

static int Audio_Vow_Cfg4_Set(struct snd_kcontrol *kcontrol,
                              struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()  = %ld\n", __func__, ucontrol->value.integer.value[0]);
    //Ana_Set_Reg(AFE_VOW_CFG4, ucontrol->value.integer.value[0], 0xffff);
    reg_AFE_VOW_CFG4 = ucontrol->value.integer.value[0];
    return 0;
}

static int Audio_Vow_Cfg5_Get(struct snd_kcontrol *kcontrol,
                              struct snd_ctl_elem_value *ucontrol)
{
    int value = /*Ana_Get_Reg(AFE_VOW_CFG5)*/reg_AFE_VOW_CFG5;
    printk("%s()  = %d\n", __func__, value);
    ucontrol->value.integer.value[0] = value;
    return 0;
}

static int Audio_Vow_Cfg5_Set(struct snd_kcontrol *kcontrol,
                              struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()  = %ld\n", __func__, ucontrol->value.integer.value[0]);
    //Ana_Set_Reg(AFE_VOW_CFG5, ucontrol->value.integer.value[0], 0xffff);
    reg_AFE_VOW_CFG5 = ucontrol->value.integer.value[0];
    return 0;
}


static bool SineTable_DAC_HP_flag = false;
static bool SineTable_UL2_flag = false;

static int SineTable_UL2_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    if (ucontrol->value.integer.value[0])
    {
        Ana_Set_Reg(PMIC_AFE_TOP_CON0 , 0x0002 , 0x2); //set DL sine gen table
        Ana_Set_Reg(AFE_SGEN_CFG0 , 0x0080 , 0xffff);
        Ana_Set_Reg(AFE_SGEN_CFG1 , 0x0101 , 0xffff);
    }
    else
    {
        Ana_Set_Reg(PMIC_AFE_TOP_CON0 , 0x0002 , 0x2); //set DL sine gen table
        Ana_Set_Reg(AFE_SGEN_CFG0 , 0x0000 , 0xffff);
        Ana_Set_Reg(AFE_SGEN_CFG1 , 0x0101 , 0xffff);
    }
    return 0;
}

static int SineTable_UL2_Get(struct snd_kcontrol *kcontrol,
                             struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()\n", __func__);
    ucontrol->value.integer.value[0] = SineTable_UL2_flag;
    return 0;
}

static int SineTable_DAC_HP_Get(struct snd_kcontrol *kcontrol,
                                struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()\n", __func__);
    ucontrol->value.integer.value[0] = SineTable_DAC_HP_flag;
    return 0;
}

static int SineTable_DAC_HP_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()\n", __func__);
#if 0
    if (ucontrol->value.integer.value[0])
    {
        SineTable_DAC_HP_flag = ucontrol->value.integer.value[0];
        printk("TurnOnDacPower\n");
        audckbufEnable(true);
        ClsqEnable(true);
        Topck_Enable(true);
        NvregEnable(true);
        Ana_Set_Reg(AFUNC_AUD_CON2, 0x0006, 0xffffffff);
        Ana_Set_Reg(AFUNC_AUD_CON0, 0xc3a1, 0xffffffff); //sdm audio fifo clock power on
        Ana_Set_Reg(AFUNC_AUD_CON2, 0x0003, 0xffffffff); //sdm power on
        Ana_Set_Reg(AFUNC_AUD_CON2, 0x000b, 0xffffffff); //sdm fifo enable
        Ana_Set_Reg(AFE_DL_SDM_CON1, 0x001e, 0xffffffff); //set attenuation gain
        Ana_Set_Reg(AFE_UL_DL_CON0 , 0x0001, 0xffffffff); //[0] afe enable

        Ana_Set_Reg(AFE_PMIC_NEWIF_CFG0 , 0x8330 , 0xffffffff);
        Ana_Set_Reg(AFE_DL_SRC2_CON0_H , 0x8330, 0xffff000f);

        Ana_Set_Reg(AFE_DL_SRC2_CON0_L , 0x1801 , 0xffffffff); //turn off mute function and turn on dl
        Ana_Set_Reg(PMIC_AFE_TOP_CON0 , 0x0001 , 0xffffffff); //set DL  sine gen table
        Ana_Set_Reg(AFE_SGEN_CFG0 , 0x0080 , 0xffffffff);
        Ana_Set_Reg(AFE_SGEN_CFG1 , 0x0101 , 0xffffffff);

        Ana_Set_Reg(0x0680, 0x0000, 0xffff); // Enable AUDGLB
        OpenClassAB();
        Ana_Set_Reg(AUDLDO_NVREG_CFG0,   0x0028, 0xffff); // Enable cap-less LDOs (1.6V)
        Ana_Set_Reg(AUDLDO_NVREG_CFG0,   0x0068, 0xffff); // Enable NV regulator (-1.6V)
        Ana_Set_Reg(AUDBUF_CFG5, 0x001f, 0xffff); // enable HP bias circuits
        Ana_Set_Reg(ZCD_CON0,   0x0700, 0xffff); // Disable AUD_ZCD
        Ana_Set_Reg(AUDBUF_CFG0,   0xE008, 0xffff); // Disable headphone, voice and short-ckt protection.
        Ana_Set_Reg(IBIASDIST_CFG0,   0x0092, 0xffff); //Enable IBIST
        Ana_Set_Reg(ZCD_CON2,  0x0F9F , 0xffff); //Set HPR/HPL gain as minimum (~ -40dB)
        Ana_Set_Reg(ZCD_CON3,  0x001F , 0xffff); //Set voice gain as minimum (~ -40dB)
        Ana_Set_Reg(AUDBUF_CFG1,  0x0900 , 0xffff); //De_OSC of HP and enable output STBENH
        Ana_Set_Reg(AUDBUF_CFG2,  0x0022 , 0xffff); //De_OSC of voice, enable output STBENH
        Ana_Set_Reg(AUDBUF_CFG0,  0xE009 , 0xffff); //Enable voice driver
        Ana_Set_Reg(AUDBUF_CFG1,  0x0940 , 0xffff); //Enable pre-charge buffer
        msleep(1);
        Ana_Set_Reg(AUDCLKGEN_CFG0, 0x5501 , 0xffff); //Enable AUD_CLK
        Ana_Set_Reg(AUDDAC_CFG0, 0x000f , 0xffff); //Enable Audio DAC
        SetDcCompenSation();

        Ana_Set_Reg(AUDBUF_CFG0, 0xE149 , 0xffff); // Switch HP MUX to audio DAC
        Ana_Set_Reg(AUDBUF_CFG0, 0xE14F , 0xffff); // Enable HPR/HPL
        Ana_Set_Reg(AUDBUF_CFG1, 0x0900 , 0xffff); // Disable pre-charge buffer
        Ana_Set_Reg(AUDBUF_CFG2, 0x0020 , 0xffff); // Disable De_OSC of voice
        Ana_Set_Reg(AUDBUF_CFG0, 0xE14E , 0xffff); // Disable voice buffer
        Ana_Set_Reg(ZCD_CON2,       0x0489 , 0xffff); // Set HPR/HPL gain as 0dB, step by step

    }
    else
    {
        SineTable_DAC_HP_flag = ucontrol->value.integer.value[0];
        if (GetDLStatus() == false)
        {
            Ana_Set_Reg(AUDBUF_CFG0, 0xE149, 0xffff); // Disable HPR/HPL
            Ana_Set_Reg(AUDDAC_CFG0, 0x0000, 0xffff); // Disable Audio DAC
            Ana_Set_Reg(AUDCLKGEN_CFG0, 0x5500, 0xffff); // Disable AUD_CLK
            Ana_Set_Reg(IBIASDIST_CFG0, 0x0192, 0xffff); // Disable IBIST
            Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0028, 0xffff); // Disable NV regulator (-1.6V)
            Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0000, 0xffff); // Disable cap-less LDOs (1.6V)
            Ana_Set_Reg(AFE_CLASSH_CFG0, 0xd518, 0xffff); // ClassH off
            Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0518, 0xffff); // NCP offset
            Ana_Set_Reg(PMIC_AFE_TOP_CON0 , 0x0000 , 0xffffffff); //set DL normal
        }
    }
#else //K2 TODO
#endif
    return 0;
}

static void ADC_LOOP_DAC_Func(int command)
{
#if 0
    if (command == AUDIO_ANALOG_DAC_LOOP_DAC_HS_ON || command == AUDIO_ANALOG_DAC_LOOP_DAC_HP_ON)
    {
        audckbufEnable(true);
        ClsqEnable(true);
        Topck_Enable(true);
        NvregEnable(true);
        Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0000, 0xffff);   //power on clock
        Ana_Set_Reg(AUDADC_CFG0, 0x0400, 0xffff);      // Enable ADC CLK

        //Ana_Set_Reg(AUDMICBIAS_CFG0, 0x78F, 0xffff);   //Enable MICBIAS0,1 (2.7V)
        OpenMicbias1(true);
        SetMicbias1lowpower(false);
        OpenMicbias0(true);
        SetMicbias0lowpower(false);

        Ana_Set_Reg(AUDMICBIAS_CFG1, 0x285, 0xffff);   //Enable MICBIAS2,3 (2.7V)
        Ana_Set_Reg(AUDLDO_NVREG_CFG1, 0x0007, 0xffff);   //Enable LCLDO18_ENC (1.8V), Remote-Sense
        Ana_Set_Reg(AUDLDO_NVREG_CFG2, 0x2277, 0xffff);   //Enable LCLDO19_ADCCH0_1, Remote-Sense
        Ana_Set_Reg(AUDPREAMPGAIN_CFG0, 0x0022, 0xffff);   //Set PGA CH0_1 gain = 12dB
        SetMicPGAGain();
        Ana_Set_Reg(AUDPREAMP_CFG0, 0x0051, 0xffff);   //Enable PGA CH0_1 (CH0 in)
        Ana_Set_Reg(AUDPREAMP_CFG1, 0x0055, 0xffff);   //Enable ADC CH0_1 (PGA in)

        Ana_Set_Reg(AFE_ADDA2_UL_SRC_CON1_L, 0x0000, 0xffffffff); //power on ADC clk
        Ana_Set_Reg(AFE_TOP_CON0, 0x4000, 0xffffffff); //AFE[14] loopback test1 ( UL tx sdata to DL rx)
        Ana_Set_Reg(AFUNC_AUD_CON2, 0x0006, 0xffffffff);
        Ana_Set_Reg(AFUNC_AUD_CON0, 0xc3a1, 0xffffffff); //sdm audio fifo clock power on
        Ana_Set_Reg(AFUNC_AUD_CON2, 0x0003, 0xffffffff); //sdm power on
        Ana_Set_Reg(AFUNC_AUD_CON2, 0x000b, 0xffffffff); //sdm fifo enable
        Ana_Set_Reg(AFE_DL_SDM_CON1, 0x001e, 0xffffffff); //set attenuation gain
        Ana_Set_Reg(AFE_UL_DL_CON0 , 0x0001, 0xffffffff); //[0] afe enable

        Ana_Set_Reg(AFE_UL_SRC0_CON0_H, 0x0000 , 0x0010); // UL1

        Ana_Set_Reg(AFE_UL_SRC0_CON0_L, 0x0001, 0xffff);   //power on uplink
        Ana_Set_Reg(AFE_PMIC_NEWIF_CFG0, 0x0380, 0xffff); //MTKIF
        Ana_Set_Reg(AFE_DL_SRC2_CON0_H, 0x0800, 0xffff);   //DL
        Ana_Set_Reg(AFE_DL_SRC2_CON0_L, 0x0001, 0xffff); //DL

        // here to start analog part
        //Ana_Set_Reg(AUDNVREGGLB_CFG0, 0x0000, 0xffff); //Enable AUDGLB
        OpenClassAB();

        Ana_Set_Reg(AUDLDO_NVREG_CFG0,   0x0028, 0xffff); // Enable cap-less LDOs (1.6V)
        Ana_Set_Reg(AUDLDO_NVREG_CFG0,   0x0068, 0xffff); // Enable NV regulator (-1.6V)
        Ana_Set_Reg(AUDBUF_CFG5, 0x001f, 0xffff); // enable HP bias circuits
        Ana_Set_Reg(ZCD_CON0,   0x0700, 0xffff); // Disable AUD_ZCD
        Ana_Set_Reg(AUDBUF_CFG0,   0xE008, 0xffff); // Disable headphone, voice and short-ckt protection.
        Ana_Set_Reg(IBIASDIST_CFG0,   0x0092, 0xffff); //Enable IBIST
        if (command == AUDIO_ANALOG_DAC_LOOP_DAC_HS_ON)
        {
            Ana_Set_Reg(ZCD_CON3,  0x001f , 0xffff); //Set voice gain as minimum (~ -40dB)
            Ana_Set_Reg(AUDBUF_CFG2,  0x0022 , 0xffff); //De_OSC of voice, enable output STBENH
            Ana_Set_Reg(AUDCLKGEN_CFG0, 0x5501 , 0xffff); //Enable AUD_CLK
            Ana_Set_Reg(AUDDAC_CFG0, 0x0009 , 0xffff); //Enable Audio DAC
            SetDcCompenSation();

            Ana_Set_Reg(AUDBUF_CFG0, 0xE010 , 0xffff); // Switch HP MUX to audio DAC
            Ana_Set_Reg(AUDBUF_CFG0, 0xE011 , 0xffff); // Enable HPR/HPL
            Ana_Set_Reg(ZCD_CON3,  0x0009 , 0xffff); // Set HPR/HPL gain as 0dB, step by step
        }
        else if (command == AUDIO_ANALOG_DAC_LOOP_DAC_HP_ON)
        {
            Ana_Set_Reg(ZCD_CON2,  0x0F9F , 0xffff); //Set HPR/HPL gain as minimum (~ -40dB)
            Ana_Set_Reg(ZCD_CON3,  0x001f , 0xffff); //Set voice gain as minimum (~ -40dB)
            Ana_Set_Reg(AUDBUF_CFG1,  0x0900 , 0xffff); //De_OSC of HP and enable output STBENH
            Ana_Set_Reg(AUDBUF_CFG2,  0x0022 , 0xffff); //De_OSC of voice, enable output STBENH
            Ana_Set_Reg(AUDBUF_CFG0,  0xE009 , 0xffff); //Enable voice driver
            Ana_Set_Reg(AUDBUF_CFG1,  0x0940 , 0xffff); //De_OSC of HP and enable output STBENH
            Ana_Set_Reg(AUDCLKGEN_CFG0, 0x5501 , 0xffff); //Enable AUD_CLK
            Ana_Set_Reg(AUDDAC_CFG0, 0x000F , 0xffff); //Enable Audio DAC
            SetDcCompenSation();

            Ana_Set_Reg(AUDBUF_CFG0, 0xE149 , 0xffff); // Switch HP MUX to audio DAC
            Ana_Set_Reg(AUDBUF_CFG0, 0xE14F , 0xffff); // Enable HPR/HPL
            Ana_Set_Reg(AUDBUF_CFG1, 0x0900 , 0xffff); // Enable HPR/HPL
            Ana_Set_Reg(AUDBUF_CFG2, 0x0020 , 0xffff); // Enable HPR/HPL
            Ana_Set_Reg(AUDBUF_CFG0, 0xE14E , 0xffff); // Enable HPR/HPL
            Ana_Set_Reg(ZCD_CON2,  0x0489 , 0xffff); // Set HPR/HPL gain as 0dB, step by step
        }
    }
    else
    {
        if (command == AUDIO_ANALOG_DAC_LOOP_DAC_HS_ON)
        {
            Ana_Set_Reg(AUDBUF_CFG0,  0xe010 , 0xffff); // Disable voice driver
            Ana_Set_Reg(AUDDAC_CFG0,  0x0000, 0xffff); // Disable L-ch Audio DAC
        }
        else if (command == AUDIO_ANALOG_DAC_LOOP_DAC_HP_ON)
        {
            Ana_Set_Reg(AUDBUF_CFG0,  0xE149 , 0xffff); // Disable voice DRIVERMODE_CODEC_ONLY
            Ana_Set_Reg(AUDDAC_CFG0,  0x0000, 0xffff); // Disable L-ch Audio DAC
        }
        Ana_Set_Reg(AUDCLKGEN_CFG0,  0x5500, 0xffff); // Disable AUD_CLK
        Ana_Set_Reg(IBIASDIST_CFG0,  0x0192, 0xffff); //Disable IBIST
        Ana_Set_Reg(AUDLDO_NVREG_CFG0,  0x0028, 0xffff); //Disable NV regulator (-1.6V)
        Ana_Set_Reg(AUDLDO_NVREG_CFG0,  0x0000, 0xffff); //Disable cap-less LDOs (1.6V)
        Ana_Set_Reg(AFE_CLASSH_CFG0, 0xd518, 0xffff); // ClassH offset
        Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0518, 0xffff); // NCP offset
    }
#else //K2 TODO
#endif
}

static bool DAC_LOOP_DAC_HS_flag = false;
static int ADC_LOOP_DAC_HS_Get(struct snd_kcontrol *kcontrol,
                               struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()\n", __func__);
    ucontrol->value.integer.value[0] = DAC_LOOP_DAC_HS_flag;
    return 0;
}

static int ADC_LOOP_DAC_HS_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()\n", __func__);
    if (ucontrol->value.integer.value[0])
    {
        DAC_LOOP_DAC_HS_flag = ucontrol->value.integer.value[0];
        ADC_LOOP_DAC_Func(AUDIO_ANALOG_DAC_LOOP_DAC_HS_ON);
    }
    else
    {
        DAC_LOOP_DAC_HS_flag = ucontrol->value.integer.value[0];
        ADC_LOOP_DAC_Func(AUDIO_ANALOG_DAC_LOOP_DAC_HS_OFF);
    }
    return 0;
}

static bool DAC_LOOP_DAC_HP_flag = false;
static int ADC_LOOP_DAC_HP_Get(struct snd_kcontrol *kcontrol,
                               struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()\n", __func__);
    ucontrol->value.integer.value[0] = DAC_LOOP_DAC_HP_flag;
    return 0;
}

static int ADC_LOOP_DAC_HP_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{

    printk("%s()\n", __func__);
    if (ucontrol->value.integer.value[0])
    {
        DAC_LOOP_DAC_HP_flag = ucontrol->value.integer.value[0];
        ADC_LOOP_DAC_Func(AUDIO_ANALOG_DAC_LOOP_DAC_HP_ON);
    }
    else
    {
        DAC_LOOP_DAC_HP_flag = ucontrol->value.integer.value[0];
        ADC_LOOP_DAC_Func(AUDIO_ANALOG_DAC_LOOP_DAC_HP_OFF);
    }
    return 0;
}

static bool Voice_Call_DAC_DAC_HS_flag = false;
static int Voice_Call_DAC_DAC_HS_Get(struct snd_kcontrol *kcontrol,
                                     struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()\n", __func__);
    ucontrol->value.integer.value[0] = Voice_Call_DAC_DAC_HS_flag;
    return 0;
}

static int Voice_Call_DAC_DAC_HS_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    printk("%s()\n", __func__);
#if 0
    if (ucontrol->value.integer.value[0])
    {
        Voice_Call_DAC_DAC_HS_flag = ucontrol->value.integer.value[0];
        // here to set voice call 16L setting...
        Ana_Set_Reg(AUDNVREGGLB_CFG0,  0x0000 , 0xffff); //  RG_AUDGLB_PWRDN_VA32 = 1'b0
        Ana_Set_Reg(TOP_CLKSQ,  0x0001, 0xffff); // CKSQ Enable
        Ana_Set_Reg(AUDADC_CFG0,  0x0400, 0xffff); // Enable ADC CLK26CALI
        //Ana_Set_Reg(AUDMICBIAS_CFG0,  0x78f, 0xffff); //  Enable MICBIAS0 (2.7V)
        OpenMicbias1(true);
        SetMicbias1lowpower(false);
        OpenMicbias0(true);
        SetMicbias0lowpower(false);

        Ana_Set_Reg(AUDMICBIAS_CFG1,  0x285, 0xffff); //  Enable MICBIAS2 (2.7V)
        Ana_Set_Reg(AUDLDO_NVREG_CFG1,  0x0007, 0xffff); //   Enable LCLDO18_ENC (1.8V), Remote-Sense ; Set LCLDO19_ADC voltage 1.9V
        Ana_Set_Reg(AUDLDO_NVREG_CFG2,  0x2277, 0xffff); // Enable LCLDO19_ADCCH0_1, Remote-Sense ; Enable LCLDO19_ADCCH_2, Remote-Sense
        Ana_Set_Reg(AUDPREAMPGAIN_CFG0, 0x033, 0xffff); // Set PGA CH0_1 gain = 18dB ; Set PGA CH_2 gain = 18dB
        SetMicPGAGain();
        Ana_Set_Reg(AUDPREAMP_CFG0, 0x051, 0xffff); // Enable PGA CH0_1 (CH0 in) ; Enable PGA CH_2
        Ana_Set_Reg(AUDPREAMP_CFG1, 0x055, 0xffff); // Enable ADC CH0_1 (PGA in) ; Enable ADC CH_2 (PGA in)

        Ana_Set_Reg(TOP_CLKSQ_SET, 0x0003, 0xffff); //CKSQ Enable
        Ana_Set_Reg(TOP_CKPDN_CON0_CLR, 0x3000, 0xffff); //AUD clock power down released
        Ana_Set_Reg(TOP_CKSEL_CON_CLR, 0x0001, 0x0001); //use internal 26M

        Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0000, 0xffff);   //power on clock

        Ana_Set_Reg(AFE_ADDA2_UL_SRC_CON1_L, 0x0000, 0xffff); // power on ADC clk
        Ana_Set_Reg(AFUNC_AUD_CON2, 0x0006, 0xffff); // sdm audio fifo clock power on
        Ana_Set_Reg(AFUNC_AUD_CON0, 0xc3a1, 0xffff); // scrambler clock on enable
        Ana_Set_Reg(AFUNC_AUD_CON2, 0x0003, 0xffff); // sdm power on
        Ana_Set_Reg(AFUNC_AUD_CON2, 0x000b, 0xffff); // sdm fifo enable
        Ana_Set_Reg(AFE_DL_SDM_CON1, 0x001e, 0xffff); // set attenuation gain
        Ana_Set_Reg(AFE_UL_DL_CON0, 0x0001, 0xffff); // afe enable
        Ana_Set_Reg(AFE_PMIC_NEWIF_CFG0, 0x3330, 0xffff); //time slot1= 47, time slot2=24 @ 384K interval.
        Ana_Set_Reg(AFE_DL_SRC2_CON0_H, 0x3330, 0xffff); //16k samplerate
        Ana_Set_Reg(AFE_DL_SRC2_CON0_L, 0x1801, 0xffff); //turn off mute function and turn on dl
        Ana_Set_Reg(AFE_UL_SRC0_CON0_H, 0x000a, 0xffff); //UL1
        Ana_Set_Reg(AFE_UL_SRC0_CON0_L, 0x0001, 0xffff); //power on uplink

        //============sine gen table============
        Ana_Set_Reg(PMIC_AFE_TOP_CON0, 0x0000, 0xffff); //no loopback
        Ana_Set_Reg(AFE_SGEN_CFG0, 0x0080, 0xffff); //L/R-ch @ sample rate = 8*8K for tone = 0dB of 1K Hz example.
        Ana_Set_Reg(AFE_SGEN_CFG1, 0x0101, 0xffff); //L/R-ch @ sample rate = 8*8K for tone = 0dB of 1K Hz example.

        // ======================here set analog part (audio HP playback)=========================
        Ana_Set_Reg(AUDNVREGGLB_CFG0, 0x0000, 0xffff); // [0] RG_AUDGLB_PWRDN_VA32 = 1'b0

        Ana_Set_Reg(AFE_CLASSH_CFG7, 0x8909, 0xffff); // Classh CK fix 591KHz
        Ana_Set_Reg(AFE_CLASSH_CFG8, 0x0d0d, 0xffff); // Classh CK fix 591KHz
        Ana_Set_Reg(AFE_CLASSH_CFG9, 0x0d10, 0xffff); // Classh CK fix 591KHz
        Ana_Set_Reg(AFE_CLASSH_CFG10, 0x1010, 0xffff); // Classh CK fix 591KHz
        Ana_Set_Reg(AFE_CLASSH_CFG11, 0x1010, 0xffff); // Classh CK fix 591KHz
        Ana_Set_Reg(AFE_CLASSH_CFG12, 0x0000, 0xffff); // Classh CK fix 591KHz
        Ana_Set_Reg(AFE_CLASSH_CFG13, 0x0000, 0xffff); // Classh CK fix 591KHz
        Ana_Set_Reg(AFE_CLASSH_CFG14, 0x009c, 0xffff); // Classh CK fix 591KHz
        Ana_Set_Reg(AFE_CLASSH_CFG26, 0x8d0d, 0xffff); // Classh CK fix 591KHz
        Ana_Set_Reg(AFE_CLASSH_CFG27, 0x0d0d, 0xffff); // Classh CK fix 591KHz
        Ana_Set_Reg(AFE_CLASSH_CFG28, 0x0d10, 0xffff); // Classh CK fix 591KHz
        Ana_Set_Reg(AFE_CLASSH_CFG29, 0x1010, 0xffff); // Classh CK fix 591KHz
        Ana_Set_Reg(AFE_CLASSH_CFG30, 0x1010, 0xffff); // Classh CK fix 591KHz
        Ana_Set_Reg(AFE_CLASSH_CFG1, 0x0024, 0xffff); // Classh CK fix 591KHz
        Ana_Set_Reg(AFE_CLASSH_CFG2, 0x2f90, 0xffff); // Classh CK fix 591KHz
        Ana_Set_Reg(AFE_CLASSH_CFG3, 0x1104, 0xffff); // Classh CK fix 591KHz
        Ana_Set_Reg(AFE_CLASSH_CFG4, 0x2412, 0xffff); // Classh CK fix 591KHz
        Ana_Set_Reg(AFE_CLASSH_CFG5, 0x0201, 0xffff); // Classh CK fix 591KHz
        Ana_Set_Reg(AFE_CLASSH_CFG6, 0x2800, 0xffff); // Classh CK fix 591KHz
        Ana_Set_Reg(AFE_CLASSH_CFG21, 0xa108, 0xffff); // Classh CK fix 591KHz
        Ana_Set_Reg(AFE_CLASSH_CFG22, 0x06db, 0xffff); // Classh CK fix 591KHz
        Ana_Set_Reg(AFE_CLASSH_CFG23, 0x0bd6, 0xffff); // Classh CK fix 591KHz
        Ana_Set_Reg(AFE_CLASSH_CFG24, 0x1492, 0xffff); // Classh CK fix 591KHz
        Ana_Set_Reg(AFE_CLASSH_CFG25, 0x1740, 0xffff); // Classh CK fix 591KHz
        Ana_Set_Reg(AFE_CLASSH_CFG0,   0xd518, 0xffff); // Classh CK fix 591KHz
        msleep(1);
        Ana_Set_Reg(AFE_CLASSH_CFG0,   0xd419, 0xffff); // Classh CK fix 591KHz
        msleep(1);
        Ana_Set_Reg(AFE_CLASSH_CFG1,   0x0021, 0xffff); // Classh CK fix 591KHz
        msleep(1);

        Ana_Set_Reg(AUDLDO_NVREG_CFG0,   0x0028, 0xffff); // Enable cap-less LDOs (1.6V)
        Ana_Set_Reg(AUDLDO_NVREG_CFG0,   0x0068, 0xffff); // Enable NV regulator (-1.6V)
        Ana_Set_Reg(AUDBUF_CFG5, 0x001f, 0xffff); // enable HP bias circuits
        Ana_Set_Reg(ZCD_CON0,   0x0700, 0xffff); // Disable AUD_ZCD
        Ana_Set_Reg(AUDBUF_CFG0,   0xE008, 0xffff); // Disable headphone, voice and short-ckt protection.
        Ana_Set_Reg(IBIASDIST_CFG0,   0x0092, 0xffff); //Enable IBIST

        Ana_Set_Reg(ZCD_CON2,  0x0F9F , 0xffff); //Set HPR/HPL gain as minimum (~ -40dB)
        Ana_Set_Reg(ZCD_CON3,  0x001f , 0xffff); //Set voice gain as minimum (~ -40dB)
        Ana_Set_Reg(AUDBUF_CFG1,  0x0900 , 0xffff); //De_OSC of HP and enable output STBENH
        Ana_Set_Reg(AUDBUF_CFG2,  0x0022 , 0xffff); //De_OSC of voice, enable output STBENH
        Ana_Set_Reg(AUDBUF_CFG0,  0xE009 , 0xffff); //Enable voice driver
        Ana_Set_Reg(AUDBUF_CFG1,  0x0940 , 0xffff); //De_OSC of HP and enable output STBENH
        Ana_Set_Reg(AUDCLKGEN_CFG0, 0x5501 , 0xffff); //Enable AUD_CLK
        Ana_Set_Reg(AUDDAC_CFG0, 0x000F , 0xffff); //Enable Audio DAC
        SetDcCompenSation();

        Ana_Set_Reg(AUDBUF_CFG0, 0xE010 , 0xffff); // Switch HP MUX to audio DAC
        Ana_Set_Reg(AUDBUF_CFG0, 0xE011 , 0xffff);
        Ana_Set_Reg(AUDBUF_CFG1, 0x0900 , 0xffff);
        Ana_Set_Reg(AUDBUF_CFG2, 0x0020 , 0xffff);
        //Ana_Set_Reg(AUDBUF_CFG0, 0xE146 , 0xffff); // Enable HPR/HPL
        Ana_Set_Reg(ZCD_CON2,  0x0489 , 0xffff); // Set HPR/HPL gain as 0dB, step by step
        Ana_Set_Reg(ZCD_CON3,  0x0489 , 0xffff); // Set HPR/HPL gain as 0dB, step by step

        //Phone_Call_16k_Vioce_mode_DL_UL

    }
    else
    {
        Voice_Call_DAC_DAC_HS_flag = ucontrol->value.integer.value[0];
    }
#else //K2 TODO
#endif
    return 0;
}


static bool GetLoopbackStatus(void)
{
    printk("%s DAC_LOOP_DAC_HP_flag = %d DAC_LOOP_DAC_HP_flag = %d \n", __func__, DAC_LOOP_DAC_HP_flag, DAC_LOOP_DAC_HP_flag);
    return (DAC_LOOP_DAC_HP_flag || DAC_LOOP_DAC_HP_flag);
}


// here start uplink power function
static const char *Pmic_Test_function[] = {"Off", "On"};

static const struct soc_enum Pmic_Test_Enum[] =
{
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Pmic_Test_function), Pmic_Test_function),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Pmic_Test_function), Pmic_Test_function),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Pmic_Test_function), Pmic_Test_function),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Pmic_Test_function), Pmic_Test_function),
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Pmic_Test_function), Pmic_Test_function),
};

static const struct snd_kcontrol_new mt6331_pmic_Test_controls[] =
{
    SOC_ENUM_EXT("SineTable_DAC_HP", Pmic_Test_Enum[0], SineTable_DAC_HP_Get, SineTable_DAC_HP_Set),
    SOC_ENUM_EXT("DAC_LOOP_DAC_HS", Pmic_Test_Enum[1], ADC_LOOP_DAC_HS_Get, ADC_LOOP_DAC_HS_Set),
    SOC_ENUM_EXT("DAC_LOOP_DAC_HP", Pmic_Test_Enum[2], ADC_LOOP_DAC_HP_Get, ADC_LOOP_DAC_HP_Set),
    SOC_ENUM_EXT("Voice_Call_DAC_DAC_HS", Pmic_Test_Enum[3], Voice_Call_DAC_DAC_HS_Get, Voice_Call_DAC_DAC_HS_Set),
    SOC_ENUM_EXT("SineTable_UL2", Pmic_Test_Enum[4], SineTable_UL2_Get, SineTable_UL2_Set),
};

static const struct snd_kcontrol_new mt6331_UL_Codec_controls[] =
{
    SOC_ENUM_EXT("Audio_ADC_1_Switch", Audio_UL_Enum[0], Audio_ADC1_Get, Audio_ADC1_Set),
    SOC_ENUM_EXT("Audio_ADC_2_Switch", Audio_UL_Enum[1], Audio_ADC2_Get, Audio_ADC2_Set),
    SOC_ENUM_EXT("Audio_ADC_3_Switch", Audio_UL_Enum[2], Audio_ADC3_Get, Audio_ADC3_Set),
    SOC_ENUM_EXT("Audio_ADC_4_Switch", Audio_UL_Enum[3], Audio_ADC4_Get, Audio_ADC4_Set),
    SOC_ENUM_EXT("Audio_Preamp1_Switch", Audio_UL_Enum[4], Audio_PreAmp1_Get, Audio_PreAmp1_Set),
    SOC_ENUM_EXT("Audio_ADC_1_Sel", Audio_UL_Enum[5], Audio_ADC1_Sel_Get, Audio_ADC1_Sel_Set),
    SOC_ENUM_EXT("Audio_ADC_2_Sel", Audio_UL_Enum[6], Audio_ADC2_Sel_Get, Audio_ADC2_Sel_Set),
    SOC_ENUM_EXT("Audio_ADC_3_Sel", Audio_UL_Enum[7], Audio_ADC3_Sel_Get, Audio_ADC3_Sel_Set),
    SOC_ENUM_EXT("Audio_ADC_4_Sel", Audio_UL_Enum[8], Audio_ADC4_Sel_Get, Audio_ADC4_Sel_Set),
    SOC_ENUM_EXT("Audio_PGA1_Setting", Audio_UL_Enum[9], Audio_PGA1_Get, Audio_PGA1_Set),
    SOC_ENUM_EXT("Audio_PGA2_Setting", Audio_UL_Enum[10], Audio_PGA2_Get, Audio_PGA2_Set),
    SOC_ENUM_EXT("Audio_PGA3_Setting", Audio_UL_Enum[11], Audio_PGA3_Get, Audio_PGA3_Set),
    SOC_ENUM_EXT("Audio_PGA4_Setting", Audio_UL_Enum[12], Audio_PGA4_Get, Audio_PGA4_Set),
    SOC_ENUM_EXT("Audio_MicSource1_Setting", Audio_UL_Enum[13], Audio_MicSource1_Get, Audio_MicSource1_Set),
    SOC_ENUM_EXT("Audio_MicSource2_Setting", Audio_UL_Enum[14], Audio_MicSource2_Get, Audio_MicSource2_Set),
    SOC_ENUM_EXT("Audio_MicSource3_Setting", Audio_UL_Enum[15], Audio_MicSource3_Get, Audio_MicSource3_Set),
    SOC_ENUM_EXT("Audio_MicSource4_Setting", Audio_UL_Enum[16], Audio_MicSource4_Get, Audio_MicSource4_Set),
    SOC_ENUM_EXT("Audio_MIC1_Mode_Select", Audio_UL_Enum[17], Audio_Mic1_Mode_Select_Get, Audio_Mic1_Mode_Select_Set),
    SOC_ENUM_EXT("Audio_MIC2_Mode_Select", Audio_UL_Enum[18], Audio_Mic2_Mode_Select_Get, Audio_Mic2_Mode_Select_Set),
    SOC_ENUM_EXT("Audio_MIC3_Mode_Select", Audio_UL_Enum[19], Audio_Mic3_Mode_Select_Get, Audio_Mic3_Mode_Select_Set),
    SOC_ENUM_EXT("Audio_MIC4_Mode_Select", Audio_UL_Enum[20], Audio_Mic4_Mode_Select_Get, Audio_Mic4_Mode_Select_Set),
    SOC_ENUM_EXT("Audio_Mic_Power_Mode", Audio_UL_Enum[21], Audio_Adc_Power_Mode_Get, Audio_Adc_Power_Mode_Set),
    SOC_ENUM_EXT("Audio_Vow_ADC_Func_Switch", Audio_UL_Enum[22], Audio_Vow_ADC_Func_Switch_Get, Audio_Vow_ADC_Func_Switch_Set),
    SOC_ENUM_EXT("Audio_Preamp2_Switch", Audio_UL_Enum[23], Audio_PreAmp2_Get, Audio_PreAmp2_Set),
    SOC_ENUM_EXT("Audio_Vow_Digital_Func_Switch", Audio_UL_Enum[24], Audio_Vow_Digital_Func_Switch_Get, Audio_Vow_Digital_Func_Switch_Set),
    SOC_ENUM_EXT("Audio_Vow_MIC_Type_Select", Audio_UL_Enum[25], Audio_Vow_MIC_Type_Select_Get, Audio_Vow_MIC_Type_Select_Set),
    SOC_SINGLE_EXT("Audio VOWCFG0 Data", SND_SOC_NOPM, 0, 0x80000, 0, Audio_Vow_Cfg0_Get, Audio_Vow_Cfg0_Set),
    SOC_SINGLE_EXT("Audio VOWCFG1 Data", SND_SOC_NOPM, 0, 0x80000, 0, Audio_Vow_Cfg1_Get, Audio_Vow_Cfg1_Set),
    SOC_SINGLE_EXT("Audio VOWCFG2 Data", SND_SOC_NOPM, 0, 0x80000, 0, Audio_Vow_Cfg2_Get, Audio_Vow_Cfg2_Set),
    SOC_SINGLE_EXT("Audio VOWCFG3 Data", SND_SOC_NOPM, 0, 0x80000, 0, Audio_Vow_Cfg3_Get, Audio_Vow_Cfg3_Set),
    SOC_SINGLE_EXT("Audio VOWCFG4 Data", SND_SOC_NOPM, 0, 0x80000, 0, Audio_Vow_Cfg4_Get, Audio_Vow_Cfg4_Set),
    SOC_SINGLE_EXT("Audio VOWCFG5 Data", SND_SOC_NOPM, 0, 0x80000, 0, Audio_Vow_Cfg5_Get, Audio_Vow_Cfg5_Set),
};

static void speaker_event(struct snd_soc_dapm_widget *w,
                          struct snd_kcontrol *kcontrol, int event)
{
    printk("speaker_event = %d\n", event);
    switch (event)
    {
        case SND_SOC_DAPM_PRE_PMU:
            printk("%s SND_SOC_DAPM_PRE_PMU", __func__);
            break;
        case SND_SOC_DAPM_POST_PMU:
            printk("%s SND_SOC_DAPM_POST_PMU", __func__);
            break;
        case SND_SOC_DAPM_PRE_PMD:
            printk("%s SND_SOC_DAPM_PRE_PMD", __func__);
            break;
        case SND_SOC_DAPM_POST_PMD:
            printk("%s SND_SOC_DAPM_POST_PMD", __func__);
        case SND_SOC_DAPM_PRE_REG:
            printk("%s SND_SOC_DAPM_PRE_REG", __func__);
        case SND_SOC_DAPM_POST_REG:
            printk("%s SND_SOC_DAPM_POST_REG", __func__);
            break;
    }
}


/* The register address is the same as other codec so it can use resmgr */
static int codec_enable_rx_bias(struct snd_soc_dapm_widget *w,
                                struct snd_kcontrol *kcontrol, int event)
{
    printk("codec_enable_rx_bias = %d\n", event);
    switch (event)
    {
        case SND_SOC_DAPM_PRE_PMU:
            printk("%s SND_SOC_DAPM_PRE_PMU", __func__);
            break;
        case SND_SOC_DAPM_POST_PMU:
            printk("%s SND_SOC_DAPM_POST_PMU", __func__);
            break;
        case SND_SOC_DAPM_PRE_PMD:
            printk("%s SND_SOC_DAPM_PRE_PMD", __func__);
            break;
        case SND_SOC_DAPM_POST_PMD:
            printk("%s SND_SOC_DAPM_POST_PMD", __func__);
        case SND_SOC_DAPM_PRE_REG:
            printk("%s SND_SOC_DAPM_PRE_REG", __func__);
        case SND_SOC_DAPM_POST_REG:
            printk("%s SND_SOC_DAPM_POST_REG", __func__);
            break;
    }
    return 0;
}

static const struct snd_soc_dapm_widget mt6331_dapm_widgets[] =
{
    /* Outputs */
    SND_SOC_DAPM_OUTPUT("EARPIECE"),
    SND_SOC_DAPM_OUTPUT("HEADSET"),
    SND_SOC_DAPM_OUTPUT("SPEAKER"),
    /*
    SND_SOC_DAPM_MUX_E("VOICE_Mux_E", SND_SOC_NOPM, 0, 0  , &mt6331_Voice_Switch, codec_enable_rx_bias,
    SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD | SND_SOC_DAPM_POST_PMD |
    SND_SOC_DAPM_PRE_REG | SND_SOC_DAPM_POST_REG),
    */

};

static const struct snd_soc_dapm_route mtk_audio_map[] =
{
    {"VOICE_Mux_E", "Voice Mux", "SPEAKER PGA"},
};

static void mt6331_codec_init_reg(struct snd_soc_codec *codec)
{
    printk("%s  \n", __func__);
    Ana_Set_Reg(TOP_CLKSQ, 0x0 , 0x0001); //Disable CLKSQ 26MHz
    Ana_Set_Reg(AUDDEC_ANA_CON8, 0x0002, 0x0002); // disable AUDGLB
    Ana_Set_Reg(TOP_CKPDN_CON0_SET, 0x3800, 0x3800); //Turn off AUDNCP_CLKDIV engine clock,Turn off AUD 26M
#if 0
    Ana_Set_Reg(AUDBUF_CFG0,  0xE000 , 0xe000); //Disable voice DriverVer_type
    // set to lowe power mode
    mt6331_upmu_set_rg_audmicbias1lowpen(true); // mic 1 low power mode
    mt6331_upmu_set_rg_audmicbias0lowpen(true); // mic 1 low power mode
    Ana_Set_Reg(AUDMICBIAS_CFG1, 0x2020, 0xffff);   //power on clock
    Ana_Set_Reg(AFE_ADDA2_UL_SRC_CON1_L, 0xA000, 0xffff);   //power on clock
#else //K2 TODO

#endif
}

void InitCodecDefault(void)
{
    printk("%s\n", __func__);
    mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP1] = 3;
    mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP2] = 3;
    mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP3] = 3;
    mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP4] = 3;

    mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC1] = AUDIO_ANALOG_AUDIOANALOG_INPUT_PREAMP;
    mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC2] = AUDIO_ANALOG_AUDIOANALOG_INPUT_PREAMP;
    mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC3] = AUDIO_ANALOG_AUDIOANALOG_INPUT_PREAMP;
    mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC4] = AUDIO_ANALOG_AUDIOANALOG_INPUT_PREAMP;
}

static int mt6331_codec_probe(struct snd_soc_codec *codec)
{
    struct snd_soc_dapm_context *dapm = &codec->dapm;
    printk("%s()\n", __func__);
    if (mInitCodec == true)
    {
        return 0;
    }

    snd_soc_dapm_new_controls(dapm, mt6331_dapm_widgets,
                              ARRAY_SIZE(mt6331_dapm_widgets));
    snd_soc_dapm_add_routes(dapm, mtk_audio_map,
                            ARRAY_SIZE(mtk_audio_map));

    //add codec controls
    snd_soc_add_codec_controls(codec, mt6331_snd_controls,
                               ARRAY_SIZE(mt6331_snd_controls));
    snd_soc_add_codec_controls(codec, mt6331_UL_Codec_controls,
                               ARRAY_SIZE(mt6331_UL_Codec_controls));
    snd_soc_add_codec_controls(codec, mt6331_Voice_Switch,
                               ARRAY_SIZE(mt6331_Voice_Switch));
    snd_soc_add_codec_controls(codec, mt6331_pmic_Test_controls,
                               ARRAY_SIZE(mt6331_pmic_Test_controls));

#ifdef CONFIG_MTK_SPEAKER
    snd_soc_add_codec_controls(codec, mt6331_snd_Speaker_controls,
                               ARRAY_SIZE(mt6331_snd_Speaker_controls));
#endif

    snd_soc_add_codec_controls(codec, Audio_snd_auxadc_controls,
                               ARRAY_SIZE(Audio_snd_auxadc_controls));

    // here to set  private data
    mCodec_data = kzalloc(sizeof(mt6331_Codec_Data_Priv), GFP_KERNEL);
    if (!mCodec_data)
    {
        printk("Failed to allocate private data\n");
        return -ENOMEM;
    }
    snd_soc_codec_set_drvdata(codec, mCodec_data);

    memset((void *)mCodec_data , 0 , sizeof(mt6331_Codec_Data_Priv));
    mt6331_codec_init_reg(codec);
    InitCodecDefault();
    mInitCodec = true;

    return 0;
}

static int mt6331_codec_remove(struct snd_soc_codec *codec)
{
    printk("%s()\n", __func__);
    return 0;
}

static unsigned int mt6331_read(struct snd_soc_codec *codec,
                                unsigned int reg)
{
    printk("mt6331_read reg = 0x%x", reg);
    Ana_Get_Reg(reg);
    return 0;
}

static int mt6331_write(struct snd_soc_codec *codec, unsigned int reg,
                        unsigned int value)
{
    printk("mt6331_write reg = 0x%x  value= 0x%x\n", reg, value);
    Ana_Set_Reg(reg , value , 0xffffffff);
    return 0;
}

static int mt6331_Readable_registers(struct snd_soc_codec *codec,
                                     unsigned int reg)
{
    return 1;
}

static int mt6331_volatile_registers(struct snd_soc_codec *codec,
                                     unsigned int reg)
{
    return 1;
}

static struct snd_soc_codec_driver soc_mtk_codec =
{
    .probe    = mt6331_codec_probe,
    .remove = mt6331_codec_remove,

    .read = mt6331_read,
    .write = mt6331_write,

    .readable_register = mt6331_Readable_registers,
    .volatile_register = mt6331_volatile_registers,

    // use add control to replace
    //.controls = mt6331_snd_controls,
    //.num_controls = ARRAY_SIZE(mt6331_snd_controls),

    .dapm_widgets = mt6331_dapm_widgets,
    .num_dapm_widgets = ARRAY_SIZE(mt6331_dapm_widgets),
    .dapm_routes = mtk_audio_map,
    .num_dapm_routes = ARRAY_SIZE(mtk_audio_map),

};

static int mtk_mt6331_codec_dev_probe(struct platform_device *pdev)
{
    if (pdev->dev.of_node)
    {
        dev_set_name(&pdev->dev, "%s.%d", "msm-stub-codec", 1);
    }

    dev_err(&pdev->dev, "dev name %s\n", dev_name(&pdev->dev));

    return snd_soc_register_codec(&pdev->dev,
                                  &soc_mtk_codec, mtk_6331_dai_codecs, ARRAY_SIZE(mtk_6331_dai_codecs));
}

static int mtk_mt6331_codec_dev_remove(struct platform_device *pdev)
{
    printk("%s:\n", __func__);

    snd_soc_unregister_codec(&pdev->dev);
    return 0;

}

static struct platform_driver mtk_codec_6331_driver =
{
    .driver = {
        .name = MT_SOC_CODEC_NAME,
        .owner = THIS_MODULE,
    },
    .probe  = mtk_mt6331_codec_dev_probe,
    .remove = mtk_mt6331_codec_dev_remove,
};

static struct platform_device *soc_mtk_codec6331_dev;

static int __init mtk_mt6331_codec_init(void)
{
    int ret = 0;
    printk("%s:\n", __func__);

    soc_mtk_codec6331_dev = platform_device_alloc(MT_SOC_CODEC_NAME, -1);
    if (!soc_mtk_codec6331_dev)
    {
        return -ENOMEM;
    }

    ret = platform_device_add(soc_mtk_codec6331_dev);
    if (ret != 0)
    {
        platform_device_put(soc_mtk_codec6331_dev);
        return ret;
    }

    return platform_driver_register(&mtk_codec_6331_driver);
}
module_init(mtk_mt6331_codec_init);

static void __exit mtk_mt6331_codec_exit(void)
{
    printk("%s:\n", __func__);

    platform_driver_unregister(&mtk_codec_6331_driver);
}

module_exit(mtk_mt6331_codec_exit);

/* Module information */
MODULE_DESCRIPTION("MTK  codec driver");
MODULE_LICENSE("GPL v2");

