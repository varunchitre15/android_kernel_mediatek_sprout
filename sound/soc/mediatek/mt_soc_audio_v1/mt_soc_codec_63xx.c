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
 *   mtk_codec_stub
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

#ifdef CONFIG_MTK_SPEAKER
#include "mt_soc_codec_speaker_63xx.h"
#endif

#include "mt_soc_pcm_common.h"


extern void mt6331_upmu_set_rg_audmicbias1lowpen(kal_uint32 val);
extern void mt6331_upmu_set_rg_audmicbias1dcswnen(kal_uint32 val);
extern void mt6331_upmu_set_rg_audmicbias1dcswpen(kal_uint32 val);
extern void mt6331_upmu_set_rg_audpwdbmicbias1(kal_uint32 val);

extern void mt6331_upmu_set_rg_audmicbias0lowpen(kal_uint32 val);
extern void mt6331_upmu_set_rg_audmicbias0dcswnen(kal_uint32 val);
extern void mt6331_upmu_set_rg_audmicbias0dcswpen(kal_uint32 val);
extern void mt6331_upmu_set_rg_audpwdbmicbias0(kal_uint32 val);


/* static function declaration */
static void HeadsetRVolumeSet(void);
static void HeadsetLVolumeSet(void);
static bool AudioPreAmp1_Sel(int Mul_Sel);
static bool GetAdcStatus(void);
static void Apply_Speaker_Gain(void);


static mt6331_Codec_Data_Priv *mCodec_data;
static uint32 mBlockSampleRate[AUDIO_ANALOG_DEVICE_INOUT_MAX] = { 48000, 48000, 48000 };

static DEFINE_MUTEX(Ana_Ctrl_Mutex);
static DEFINE_SPINLOCK(AudAna_lock);

static int mAudio_Analog_Mic1_mode = AUDIO_ANALOGUL_MODE_ACC;
static int mAudio_Analog_Mic2_mode = AUDIO_ANALOGUL_MODE_ACC;
static int mAudio_Analog_Mic3_mode = AUDIO_ANALOGUL_MODE_ACC;
static int mAudio_Analog_Mic4_mode = AUDIO_ANALOGUL_MODE_ACC;

#ifdef CONFIG_MTK_SPEAKER
static int Speaker_mode = AUDIO_SPEAKER_MODE_AB;
static unsigned int Speaker_pga_gain = 1;	/* default 0Db. */
static bool mSpeaker_Ocflag;

#endif
static unsigned int dAuxAdcChannel = 16;


static int ClsqAuxCount;
static void ClsqAuxEnable(bool enable)
{
	pr_debug("ClsqAuxEnable ClsqAuxCount = %d enable = %d\n", ClsqAuxCount, enable);
	spin_lock(&AudAna_lock);
	if (enable) {
		if (ClsqAuxCount == 0) {
			Ana_Set_Reg(TOP_CLKSQ, 0x0002, 0x0002);	/* CKSQ Enable */
		}
		ClsqAuxCount++;
	} else {
		ClsqAuxCount--;
		if (ClsqAuxCount < 0) {
			pr_debug("ClsqAuxEnable count <0\n");
			ClsqAuxCount = 0;
		}
		if (ClsqAuxCount == 0) {
			Ana_Set_Reg(TOP_CLKSQ, 0x0000, 0x0002);
		}
	}
	spin_unlock(&AudAna_lock);
}

static int ClsqCount;
static void ClsqEnable(bool enable)
{
	pr_debug("ClsqEnable ClsqAuxCount = %d enable = %d\n", ClsqCount, enable);
	spin_lock(&AudAna_lock);
	if (enable) {
		if (ClsqCount == 0) {
			Ana_Set_Reg(TOP_CLKSQ, 0x0001, 0x0001);	/* CKSQ Enable */
		}
		ClsqCount++;
	} else {
		ClsqCount--;
		if (ClsqCount < 0) {
			pr_debug("ClsqEnable count <0\n");
			ClsqCount = 0;
		}
		if (ClsqCount == 0) {
			Ana_Set_Reg(TOP_CLKSQ, 0x0000, 0x0001);
		}
	}
	spin_unlock(&AudAna_lock);
}

static int TopCkCount;
static void Topck_Enable(bool enable)
{
	pr_debug("Topck_Enable enable = %d TopCkCount = %d ", enable, TopCkCount);
	if (enable == true) {
		if (TopCkCount == 0) {
			Ana_Set_Reg(TOP_CKPDN_CON0_CLR, 0x3000, 0x3000);	/* AUD clock power down released */
		}
		TopCkCount++;
	} else {
		TopCkCount--;
		if (TopCkCount == 0) {
			Ana_Set_Reg(TOP_CKPDN_CON0_SET, 0x3000, 0x3000);
		}
		if (TopCkCount <= 0) {
			pr_debug("TopCkCount <0 =%d\n ", TopCkCount);
			TopCkCount = 0;
		}
	}
}

static int NvRegCount;
static void NvregEnable(bool enable)
{
	if (enable == true) {
		if (NvRegCount == 0) {
			Ana_Set_Reg(AUDNVREGGLB_CFG0, 0x0000, 0xffff);	/* AUD clock power down released */
		}
		NvRegCount++;
	} else {
		NvRegCount--;
		if (NvRegCount == 0) {
			Ana_Set_Reg(AUDNVREGGLB_CFG0, 0x0001, 0xffff);
		}
		if (NvRegCount <= 0) {
			pr_debug("TopCkCount <0 =%d\n ", TopCkCount);
			NvRegCount = 0;
		}
	}
}


static int AdcClockCount;
static void AdcClockEnable(bool enable)
{
	if (enable == true) {
		if (AdcClockCount == 0) {
			Ana_Set_Reg(TOP_CKPDN_CON0_CLR, 0x3000, 0xffff);	/* AUD clock power down released */
		}
		AdcClockCount++;
	} else {
		AdcClockCount--;
		if (AdcClockCount == 0) {
			Ana_Set_Reg(TOP_CKPDN_CON0_CLR, 0x0000, 0xffff);
		}
		if (AdcClockCount <= 0) {
			pr_debug("TopCkCount <0 =%d\n ", AdcClockCount);
			AdcClockCount = 0;
		}
	}
}

#ifdef CONFIG_MTK_SPEAKER
static void Apply_Speaker_Gain(void)
{
	Ana_Set_Reg(SPK_CON9, Speaker_pga_gain << 8, 0xf << 8);
}
#else
static void Apply_Speaker_Gain(void)
{
}
#endif


static void SetDcCompenSation(void)
{
}

static void OpenClassH(void)
{
	Ana_Set_Reg(AFE_CLASSH_CFG7, 0xAD2D, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG8, 0x1313, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG9, 0x132d, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG10, 0x2d13, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG11, 0x1315, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG12, 0x6464, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG13, 0x2a2a, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG14, 0x009c, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG26, 0x9313, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG27, 0x1313, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG28, 0x1315, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG29, 0x1515, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG30, 0x1515, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG1, 0xBF04, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG2, 0x5fbf, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG3, 0x1104, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG4, 0x2412, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG5, 0x0201, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG6, 0x2800, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG14, 0x009c, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG21, 0x2108, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG22, 0x06db, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG23, 0xffff, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG24, 0x0bd6, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG25, 0x1740, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG0, 0xd608, 0xffff);	/* Classh CK fix 591KHz */
	msleep(1);
	Ana_Set_Reg(AFE_CLASSH_CFG0, 0xd20d, 0xffff);	/* Classh CK fix 591KHz */
	msleep(1);
	/* Ana_Set_Reg(0x0388,   0x0300, 0xffff); */
}

static void OpenClassAB(void)
{
	Ana_Set_Reg(AFE_CLASSH_CFG7, 0x8909, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG8, 0x0909, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG9, 0x1309, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG10, 0x0909, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG11, 0x0915, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG12, 0x1414, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG13, 0x1414, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG14, 0x009c, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG26, 0x9313, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG27, 0x1313, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG28, 0x1315, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG29, 0x1515, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG30, 0x1515, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG1, 0x0024, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG2, 0x2f90, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG3, 0x1104, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG4, 0x2412, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG5, 0x0201, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG6, 0x2800, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG21, 0xa108, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG22, 0x06db, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG23, 0x0bd6, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG24, 0x1492, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG25, 0x1740, 0xffff);	/* Classh CK fix 591KHz */
	Ana_Set_Reg(AFE_CLASSH_CFG0, 0xd518, 0xffff);	/* Classh CK fix 591KHz */
	msleep(1);
	Ana_Set_Reg(AFE_CLASSH_CFG0, 0xd419, 0xffff);	/* Classh CK fix 591KHz */
	msleep(1);
	Ana_Set_Reg(AFE_CLASSH_CFG1, 0x0021, 0xffff);	/* Classh CK fix 591KHz */
	msleep(1);
}


static void OpenMicbias1(void)
{
	pr_debug("%s\n", __func__);
	mt6331_upmu_set_rg_audmicbias1lowpen(false);	/* mic 1 high power mode */
	mt6331_upmu_set_rg_audmicbias1dcswnen(false);	/* mic1 DC N external */
	mt6331_upmu_set_rg_audmicbias1dcswpen(false);	/* mic1 DC P external */
	mt6331_upmu_set_rg_audpwdbmicbias1(true);	/* mic bias power 1 on */
}

static void CloseMicbias1(void)
{
	pr_debug("%s\n", __func__);

	mt6331_upmu_set_rg_audmicbias1lowpen(true);	/* mic 1 low power mode */
	mt6331_upmu_set_rg_audpwdbmicbias1(false);	/* mic bias power 1 off */
}


static void OpenMicbias0(void)
{
	pr_debug("%s\n", __func__);

	mt6331_upmu_set_rg_audmicbias0lowpen(false);	/* mic 0 high power mode */
	mt6331_upmu_set_rg_audmicbias0dcswnen(false);	/* mic0 DC N external */
	mt6331_upmu_set_rg_audmicbias0dcswpen(false);	/* mic0 DC P external */
	mt6331_upmu_set_rg_audpwdbmicbias0(true);	/* mic bias power 0 on */
}

static void CloseMicbias0(void)
{
	pr_debug("%s\n", __func__);

	mt6331_upmu_set_rg_audmicbias0lowpen(true);	/* mic 0 low power mode */
	mt6331_upmu_set_rg_audpwdbmicbias0(false);	/* mic bias power 0 off */
}

/*
static bool Dl_Hpdet_impedence(void)
{
    ClsqAuxEnable(true);
    ClsqEnable(true);
    Topck_Enable(true);
    NvregEnable(true);
    Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0000, 0xffff);   //power on clock
    OpenClassH();
    Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0028, 0xffff); //Enable cap-less LDOs (1.6V)
    Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0068, 0xffff); //Enable NV regulator (-1.6V)
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
    return true;
}*/

static uint32 GetULNewIFFrequency(uint32 frequency)
{
	uint32 Reg_value = 0;
	switch (frequency) {
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
	pr_debug("%s frequency =%d\n", __func__, frequency);
	switch (frequency) {
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
	switch (SampleRate) {
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


static int mt63xx_codec_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *Daiport)
{
	pr_debug("+mt63xx_codec_startup name = %s number = %d\n", substream->name,
		 substream->number);
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE && substream->runtime->rate) {
		pr_debug("mt63xx_codec_startup set up SNDRV_PCM_STREAM_CAPTURE rate = %d\n",
			 substream->runtime->rate);
		mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC] = substream->runtime->rate;

	} else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK && substream->runtime->rate) {
		pr_debug("mt63xx_codec_startup set up SNDRV_PCM_STREAM_PLAYBACK rate = %d\n",
			 substream->runtime->rate);
		mBlockSampleRate[AUDIO_ANALOG_DEVICE_OUT_DAC] = substream->runtime->rate;
	}
	pr_debug("-mt63xx_codec_startup name = %s number = %d\n", substream->name,
		 substream->number);
	return 0;
}

static int mt63xx_codec_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *Daiport)
{
	pr_debug("mt63xx_codec_prepare\n ");
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		pr_debug("mt63xx_codec_prepare set up SNDRV_PCM_STREAM_CAPTURE rate = %d\n",
			 substream->runtime->rate);
		mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC] = substream->runtime->rate;

	} else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		pr_debug("mt63xx_codec_prepare set up SNDRV_PCM_STREAM_PLAYBACK rate = %d\n",
			 substream->runtime->rate);
		mBlockSampleRate[AUDIO_ANALOG_DEVICE_OUT_DAC] = substream->runtime->rate;
	}
	return 0;
}

static int mt6323_codec_trigger(struct snd_pcm_substream *substream, int command,
				struct snd_soc_dai *Daiport)
{
	switch (command) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		break;
	}

	/* pr_debug("mt6323_codec_trigger command = %d\n ", command); */
	return 0;
}

static const struct snd_soc_dai_ops mt6323_aif1_dai_ops = {
	.startup = mt63xx_codec_startup,
	.prepare = mt63xx_codec_prepare,
	.trigger = mt6323_codec_trigger,
};

static struct snd_soc_dai_driver mtk_6331_dai_codecs[] = {
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
	 .name = MT_SOC_CODEC_PCMTXDAI_NAME,
	 .ops = &mt6323_aif1_dai_ops,
	 .playback = {
		      .stream_name = MT_SOC_PCM2_STREAM_NAME,
		      .channels_min = 1,
		      .channels_max = 2,
		      .rates = SNDRV_PCM_RATE_8000_48000,
		      .formats = SND_SOC_ADV_MT_FMTS,
		      },
	 .capture = {
		     .stream_name = MT_SOC_PCM2_STREAM_NAME,
		     .channels_min = 1,
		     .channels_max = 2,
		     .rates = SNDRV_PCM_RATE_8000_48000,
		     .formats = SND_SOC_ADV_MT_FMTS,
		     },
	 },
	{
	 .name = MT_SOC_CODEC_PCMRXDAI_NAME,
	 .ops = &mt6323_aif1_dai_ops,
	 .playback = {
		      .stream_name = MT_SOC_PCM2_STREAM_NAME,
		      .channels_min = 1,
		      .channels_max = 2,
		      .rates = SNDRV_PCM_RATE_8000_48000,
		      .formats = SND_SOC_ADV_MT_FMTS,
		      },
	 .capture = {
		     .stream_name = MT_SOC_PCM2_STREAM_NAME,
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
};


uint32 GetDLNewIFFrequency(unsigned int frequency)
{
	uint32 Reg_value = 0;
	pr_debug("AudioPlatformDevice ApplyDLNewIFFrequency ApplyDLNewIFFrequency = %d", frequency);
	switch (frequency) {
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
		pr_debug("ApplyDLNewIFFrequency with frequency = %d", frequency);
	}
	return Reg_value;
}

static bool GetDLStatus(void)
{
	int i = 0;
	for (i = 0; i < AUDIO_ANALOG_DEVICE_2IN1_SPK; i++) {
		if (mCodec_data->mAudio_Ana_DevicePower[i] == true) {
			return true;
		}
	}
	return false;
}

static void TurnOnDacPower(void)
{
	pr_debug("TurnOnDacPower\n");
	ClsqEnable(true);
	Topck_Enable(true);
	NvregEnable(true);
	Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0000, 0xffff);	/* power on clock */
	Ana_Set_Reg(AFUNC_AUD_CON2, 0x0006, 0xffffffff);
	Ana_Set_Reg(AFUNC_AUD_CON0, 0xc3a1, 0xffffffff);	/* sdm audio fifo clock power on */
	Ana_Set_Reg(AFUNC_AUD_CON2, 0x0003, 0xffffffff);	/* sdm power on */
	Ana_Set_Reg(AFUNC_AUD_CON2, 0x000b, 0xffffffff);	/* sdm fifo enable */
	Ana_Set_Reg(AFE_DL_SDM_CON1, 0x001e, 0xffffffff);	/* set attenuation gain */
	Ana_Set_Reg(AFE_UL_DL_CON0, 0x0001, 0xffffffff);	/* [0] afe enable */

	Ana_Set_Reg(AFE_PMIC_NEWIF_CFG0,
		    GetDLNewIFFrequency(mBlockSampleRate[AUDIO_ANALOG_DEVICE_OUT_DAC]) << 12 |
		    0x330, 0xffffffff);
	Ana_Set_Reg(AFE_DL_SRC2_CON0_H,
		    GetDLNewIFFrequency(mBlockSampleRate[AUDIO_ANALOG_DEVICE_OUT_DAC]) << 12 |
		    0x330, 0xffffffff);

	Ana_Set_Reg(AFE_DL_SRC2_CON0_L, 0x1801, 0xffffffff);	/* turn off mute function and turn on dl */
	Ana_Set_Reg(PMIC_AFE_TOP_CON0, 0x0000, 0xffffffff);	/* set DL in normal path, not from sine gen table */
}

static void TurnOffDacPower(void)
{
	pr_debug("TurnOffDacPower\n");
	Ana_Set_Reg(AFE_CLASSH_CFG1, 0x24, 0xffff);
	Ana_Set_Reg(AFE_CLASSH_CFG0, 0xd518, 0xffff);	/* ClassH off */
	Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0, 0xffff);	/* NCP off */
	NvregEnable(false);
	ClsqEnable(false);
	Topck_Enable(false);
}

static void Audio_Amp_Change(int channels, bool enable)
{
	if (enable) {
		if (GetDLStatus() == false) {
			TurnOnDacPower();
		}
		/* here pmic analog control */
		if (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTL] == false &&
		    mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTR] == false) {
			pr_debug("Audio_Amp_Change on amp\n");
			/* Ana_Set_Reg(AUDNVREGGLB_CFG0, 0x0000, 0xffff); // Enable AUDGLB */
			OpenClassH();
			Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0028, 0xffff);	/* Enable cap-less LDOs (1.6V) */
			Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0068, 0xffff);	/* Enable NV regulator (-1.6V) */
			Ana_Set_Reg(ZCD_CON0, 0x0000, 0xffff);	/* Disable AUD_ZCD */
			Ana_Set_Reg(AUDBUF_CFG0, 0xE008, 0xffff);	/* Disable headphone, voice and short-ckt protection. */
			Ana_Set_Reg(IBIASDIST_CFG0, 0x0092, 0xffff);	/* Enable IBIST */
			Ana_Set_Reg(ZCD_CON2, 0x0F9F, 0xffff);	/* Set HPR/HPL gain as minimum (~ -40dB) */
			Ana_Set_Reg(ZCD_CON3, 0x001F, 0xffff);	/* Set voice gain as minimum (~ -40dB) */
			Ana_Set_Reg(AUDBUF_CFG1, 0x0900, 0xffff);	/* De_OSC of HP and enable output STBENH */
			Ana_Set_Reg(AUDBUF_CFG2, 0x0022, 0xffff);	/* De_OSC of voice, enable output STBENH */
			Ana_Set_Reg(AUDBUF_CFG0, 0xE009, 0xffff);	/* Enable voice driver */
			Ana_Set_Reg(AUDBUF_CFG1, 0x0940, 0xffff);	/* Enable pre-charge buffer */
			msleep(1);
			Ana_Set_Reg(AUDCLKGEN_CFG0, 0x5501, 0xffff);	/* Enable AUD_CLK */
			if (channels == AUDIO_ANALOG_CHANNELS_LEFT1) {
				Ana_Set_Reg(AUDDAC_CFG0, 0x000d, 0xffff);	/* Enable Audio DAC */
			} else {
				Ana_Set_Reg(AUDDAC_CFG0, 0x000e, 0xffff);	/* Enable Audio DAC */
			}
			/* remove this.... */
			Ana_Set_Reg(AUDDAC_CFG0, 0x000f, 0xffff);	/* Enable Audio DAC */
			SetDcCompenSation();

			Ana_Set_Reg(AUDBUF_CFG0, 0xE149, 0xffff);	/* Switch HP MUX to audio DAC */
			Ana_Set_Reg(AUDBUF_CFG0, 0xE14F, 0xffff);	/* Enable HPR/HPL */
			Ana_Set_Reg(AUDBUF_CFG1, 0x0100, 0xffff);	/* Disable pre-charge buffer */
			Ana_Set_Reg(AUDBUF_CFG2, 0x0020, 0xffff);	/* Disable De_OSC of voice */
			Ana_Set_Reg(AUDBUF_CFG0, 0xE146, 0xffff);	/* Disable voice buffer */
			Ana_Set_Reg(ZCD_CON2, 0x0183, 0xffff);	/* Set HPR/HPL gain as 0dB, step by step */

			/* apply volume setting */
			HeadsetRVolumeSet();
			HeadsetLVolumeSet();

		} else if (channels == AUDIO_ANALOG_CHANNELS_LEFT1) {
			Ana_Set_Reg(AUDDAC_CFG0, 0x000f, 0xffff);	/* enable audio bias. enable audio DAC, HP buffers */

		} else if (channels == AUDIO_ANALOG_CHANNELS_RIGHT1) {
			Ana_Set_Reg(AUDDAC_CFG0, 0x000f, 0xffff);	/* enable audio bias. enable audio DAC, HP buffers */
		}
	} else {

		if (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTL] == false &&
		    mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTR] == false) {
			pr_debug("Audio_Amp_Change off amp\n");
			Ana_Set_Reg(AUDBUF_CFG0, 0xE149, 0xffff);	/* Disable HPR/HPL */
			Ana_Set_Reg(AUDDAC_CFG0, 0x0000, 0xffff);	/* Disable Audio DAC */
			Ana_Set_Reg(AUDCLKGEN_CFG0, 0x5500, 0xffff);	/* Disable AUD_CLK */
			Ana_Set_Reg(IBIASDIST_CFG0, 0x0192, 0xffff);	/* Disable IBIST */
			Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0028, 0xffff);	/* Disable NV regulator (-1.6V) */
			Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0000, 0xffff);	/* Disable cap-less LDOs (1.6V) */
		} else if (channels == AUDIO_ANALOG_CHANNELS_LEFT1) {
			Ana_Set_Reg(AUDDAC_CFG0, 0x000e, 0xffff);	/* enable audio bias. enable audio DAC, HP buffers */
		} else if (channels == AUDIO_ANALOG_CHANNELS_RIGHT1) {
			Ana_Set_Reg(AUDDAC_CFG0, 0x000d, 0xffff);	/* enable audio bias. enable audio DAC, HP buffers */
		}
		if (GetDLStatus() == false) {
			Ana_Set_Reg(AFE_DL_SRC2_CON0_L, 0x1800, 0xffffffff);
			if (GetAdcStatus() == false) {
				Ana_Set_Reg(AFE_UL_DL_CON0, 0x0000, 0xffff);	/* turn off afe */
			}
			TurnOffDacPower();
		}
	}
}

static int Audio_AmpL_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("Audio_AmpL_Get = %d\n",
		 mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTL]);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTL];
	return 0;
}

static int Audio_AmpL_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	mutex_lock(&Ana_Ctrl_Mutex);

	pr_debug("%s() gain = %ld\n ", __func__, ucontrol->value.integer.value[0]);
	if (ucontrol->value.integer.value[0]) {
		Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_LEFT1, true);
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTL] =
		    ucontrol->value.integer.value[0];
	} else {
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTL] =
		    ucontrol->value.integer.value[0];
		Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_LEFT1, false);
	}
	mutex_unlock(&Ana_Ctrl_Mutex);
	return 0;
}

static int Audio_AmpR_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("Audio_AmpR_Get = %d\n",
		 mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTR]);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTR];
	return 0;
}

static int Audio_AmpR_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	mutex_lock(&Ana_Ctrl_Mutex);

	pr_debug("%s()\n", __func__);
	if (ucontrol->value.integer.value[0]) {
		Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_RIGHT1, true);
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTR] =
		    ucontrol->value.integer.value[0];
	} else {
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HPOUTR] =
		    ucontrol->value.integer.value[0];
		Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_RIGHT1, false);
	}
	mutex_unlock(&Ana_Ctrl_Mutex);
	return 0;
}

/*
static void  SetVoiceAmpVolume(void)
{
    int index;
    pr_debug("%s\n", __func__);
    index =  mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HSOUTL];
    Ana_Set_Reg(ZCD_CON3, index , 0x001f);
}*/

static void Voice_Amp_Change(bool enable)
{
	if (enable) {
		pr_debug("turn on ampL\n");
		if (GetDLStatus() == false) {
			Ana_Set_Reg(0x0680, 0x0000, 0xffff);	/* Enable AUDGLB */
			Ana_Set_Reg(TOP_CKSEL_CON_CLR, 0x0001, 0x0001);	/* use internal 26M */
			TurnOnDacPower();
			pr_debug("Voice_Amp_Change on amp\n");
			OpenClassAB();
			Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0028, 0xffff);	/* Enable cap-less LDOs (1.6V) */
			Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0068, 0xffff);	/* Enable NV regulator (-1.6V) */
			Ana_Set_Reg(ZCD_CON0, 0x0700, 0xffff);	/* Disable AUD_ZCD */
			Ana_Set_Reg(AUDBUF_CFG0, 0xE008, 0xffff);	/* Disable headphone, voice and short-ckt protection. */
			Ana_Set_Reg(IBIASDIST_CFG0, 0x0092, 0xffff);	/* Enable IBIST */
			Ana_Set_Reg(ZCD_CON3, 0x001F, 0xffff);	/* Set voice gain as minimum (~ -40dB) */
			Ana_Set_Reg(AUDBUF_CFG2, 0x0022, 0xffff);	/* De_OSC of voice, enable output STBENH */
			Ana_Set_Reg(AUDCLKGEN_CFG0, 0x5501, 0xffff);	/* Enable voice driver */
			Ana_Set_Reg(AUDDAC_CFG0, 0x0009, 0xffff);	/* Switch voice MUX to audio DAC */
			Ana_Set_Reg(AUDBUF_CFG0, 0xE010, 0xffff);	/* Enable voice driver */
			Ana_Set_Reg(AUDBUF_CFG0, 0xE011, 0xffff);	/* Enable voice driver */
			Ana_Set_Reg(ZCD_CON3, 0x0009, 0xffff);	/* Set voice gain as 0dB */
		}
	} else {
		pr_debug("turn off ampL\n");
		if (GetDLStatus() == false) {
			TurnOffDacPower();
			Ana_Set_Reg(AUDBUF_CFG0, 0xE010, 0xffff);	/* Disable voice driver */
			Ana_Set_Reg(AUDDAC_CFG0, 0x0000, 0xffff);	/* Disable L-ch Audio DAC */
			Ana_Set_Reg(AUDCLKGEN_CFG0, 0x5500, 0xffff);	/* Disable AUD_CLK */
			Ana_Set_Reg(IBIASDIST_CFG0, 0x0192, 0xffff);	/* Disable IBIST */
			Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0028, 0xffff);	/* Disable NV regulator (-1.6V) */
			Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0000, 0xffff);	/* Disable cap-less LDOs (1.6V) */
		}
	}
}

static int Voice_Amp_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("Voice_Amp_Get = %d\n",
		 mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HSOUTL]);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HSOUTL];
	return 0;
}

static int Voice_Amp_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	mutex_lock(&Ana_Ctrl_Mutex);
	pr_debug("%s()\n", __func__);
	if (ucontrol->value.integer.value[0]) {
		Voice_Amp_Change(true);
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HSOUTL] =
		    ucontrol->value.integer.value[0];
	} else {
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_HSOUTL] =
		    ucontrol->value.integer.value[0];
		Voice_Amp_Change(false);
	}
	mutex_unlock(&Ana_Ctrl_Mutex);
	return 0;
}


static void Speaker_Amp_Change(bool enable)
{
	if (enable) {
		if (GetDLStatus() == false) {
			TurnOnDacPower();
		}
		pr_debug("turn on Speaker_Amp_Change\n");
		/* here pmic analog control */
		/* Ana_Set_Reg(AUDNVREGGLB_CFG0  , 0x0000 , 0xffffffff); */
		NvregEnable(true);
		OpenClassAB();
		Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0028, 0xffffffff);	/* Enable cap-less LDOs (1.6V) */
		Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0068, 0xffffffff);	/* Enable NV regulator (-1.6V) */
		Ana_Set_Reg(ZCD_CON0, 0x0700, 0xffffffff);	/* Disable AUD_ZCD */
		Ana_Set_Reg(AUDBUF_CFG6, 0x00C0, 0xffffffff);	/* Disable line-out and short-ckt protection. LO MUX is opened */
		Ana_Set_Reg(IBIASDIST_CFG0, 0x0092, 0xffffffff);	/* Enable IBIST */
		Ana_Set_Reg(ZCD_CON1, 0x0F9F, 0xffffffff);	/* Set LOR/LOL gain as minimum (~ -40dB) */
		Ana_Set_Reg(AUDBUF_CFG7, 0x0102, 0xffffffff);	/* De_OSC of LO and enable output STBENH */
		Ana_Set_Reg(AUDCLKGEN_CFG0, 0x5501, 0xffffffff);	/* Enable AUD_CLK */
		Ana_Set_Reg(AUDDAC_CFG0, 0x000F, 0xffffffff);	/* Enable Audio DAC */
		SetDcCompenSation();
		Ana_Set_Reg(AUDBUF_CFG6, 0x00E8, 0xffffffff);	/* Switch LO MUX to audio DAC */
		Ana_Set_Reg(AUDBUF_CFG6, 0x00EB, 0xffffffff);	/* Enable LOR/LOL */
		Ana_Set_Reg(ZCD_CON1, 0x0489, 0xffffffff);	/* Set LOR/LOL gain as 0dB */
#ifdef CONFIG_MTK_SPEAKER
		if (Speaker_mode == AUDIO_SPEAKER_MODE_D) {
			Speaker_ClassD_Open();
		} else if (Speaker_mode == AUDIO_SPEAKER_MODE_AB) {
			Speaker_ClassAB_Open();
		} else if (Speaker_mode == AUDIO_SPEAKER_MODE_RECEIVER) {
			Speaker_ReveiverMode_Open();
		}
#endif
		Apply_Speaker_Gain();
	} else {
		pr_debug("turn off Speaker_Amp_Change\n");
#ifdef CONFIG_MTK_SPEAKER
		if (Speaker_mode == AUDIO_SPEAKER_MODE_D) {
			Speaker_ClassD_close();
		} else if (Speaker_mode == AUDIO_SPEAKER_MODE_AB) {
			Speaker_ClassAB_close();
		} else if (Speaker_mode == AUDIO_SPEAKER_MODE_RECEIVER) {
			Speaker_ReveiverMode_close();
		}
#endif
		if (GetDLStatus() == false) {
			TurnOffDacPower();
			NvregEnable(false);
		}
		Ana_Set_Reg(AUDBUF_CFG6, 0x00E8, 0xffffffff);	/* Disable LOR/LOL */
		Ana_Set_Reg(AUDDAC_CFG0, 0x0000, 0xffffffff);	/* Disable Audio DAC */
		Ana_Set_Reg(AUDCLKGEN_CFG0, 0x5500, 0xffffffff);	/* Disable AUD_CLK */
		Ana_Set_Reg(IBIASDIST_CFG0, 0x0192, 0xffffffff);	/* Disable IBIST */
		Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0028, 0xffffffff);	/* Disable NV regulator (-1.6V) */
		Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0000, 0xffffffff);	/* Disable cap-less LDOs (1.6V) */
	}
}

static int Speaker_Amp_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()\n", __func__);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_SPKL];
	return 0;
}

static int Speaker_Amp_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{

	pr_debug("%s() gain = %ld\n ", __func__, ucontrol->value.integer.value[0]);
	if (ucontrol->value.integer.value[0]) {
		Speaker_Amp_Change(true);
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_SPKL] =
		    ucontrol->value.integer.value[0];
	} else {
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_SPKL] =
		    ucontrol->value.integer.value[0];
		Speaker_Amp_Change(false);
	}
	return 0;
}

static void Headset_Speaker_Amp_Change(bool enable)
{
	if (enable) {
		if (GetDLStatus() == false) {
			TurnOnDacPower();
		}
		pr_debug("turn on Speaker_Amp_Change\n");
		/* here pmic analog control */
		/* Ana_Set_Reg(AUDNVREGGLB_CFG0  , 0x0000 , 0xffffffff); */
		OpenClassAB();

		Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0028, 0xffffffff);	/* Enable cap-less LDOs (1.6V) */
		Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0068, 0xffffffff);	/* Enable NV regulator (-1.6V) */
		Ana_Set_Reg(ZCD_CON0, 0x0700, 0xffffffff);	/* Disable AUD_ZCD */

		Ana_Set_Reg(AUDBUF_CFG0, 0xE008, 0xffffffff);	/* Disable headphone, voice and short-ckt protection. */
		Ana_Set_Reg(AUDBUF_CFG6, 0x00C0, 0xffffffff);	/* Disable line-out and short-ckt protection. LO MUX is opened */

		Ana_Set_Reg(IBIASDIST_CFG0, 0x0092, 0xffffffff);	/* Enable IBIST */
		Ana_Set_Reg(ZCD_CON2, 0x0F9F, 0xffffffff);	/* Set LOR/LOL gain as minimum (~ -40dB) */
		Ana_Set_Reg(ZCD_CON1, 0x0F9F, 0xffffffff);	/* Set LOR/LOL gain as minimum (~ -40dB) */
		Ana_Set_Reg(ZCD_CON3, 0x001F, 0xffffffff);	/* Set voice gain as minimum (~ -40dB) */

		Ana_Set_Reg(AUDBUF_CFG1, 0x0900, 0xffffffff);	/* De_OSC of HP and enable output STBENH */
		Ana_Set_Reg(AUDBUF_CFG7, 0x0102, 0xffffffff);	/* De_OSC of LO and enable output STBENH */
		Ana_Set_Reg(AUDBUF_CFG2, 0x0022, 0xffffffff);	/* De_OSC of voice, enable output STBENH */
		Ana_Set_Reg(AUDBUF_CFG0, 0xE009, 0xffffffff);	/* Enable voice driver */
		Ana_Set_Reg(AUDBUF_CFG1, 0x0940, 0xffffffff);	/* Enable pre-charge buffer_map_state */
		msleep(1);

		Ana_Set_Reg(AUDCLKGEN_CFG0, 0x5501, 0xffffffff);	/* Enable AUD_CLK */
		Ana_Set_Reg(AUDDAC_CFG0, 0x000F, 0xffffffff);	/* Enable Audio DAC */
		SetDcCompenSation();
		Ana_Set_Reg(AUDBUF_CFG6, 0x00E8, 0xffffffff);	/* Switch LO MUX to audio DAC */
		Ana_Set_Reg(AUDBUF_CFG6, 0x00EB, 0xffffffff);	/* Enable LOR/LOL */
		Ana_Set_Reg(ZCD_CON1, 0x0489, 0xffffffff);	/* Set LOR/LOL gain as 0dB */

		Ana_Set_Reg(AUDBUF_CFG0, 0xE0A9, 0xffffffff);	/* Switch HP MUX to audio DAC */
		Ana_Set_Reg(AUDBUF_CFG0, 0xE0AF, 0xffffffff);	/* Enable HPR/HPL */
		Ana_Set_Reg(AUDBUF_CFG1, 0x0900, 0xffffffff);	/* Disable pre-charge buffer */
		Ana_Set_Reg(AUDBUF_CFG2, 0x0020, 0xffffffff);	/* Disable De_OSC of voice */
		Ana_Set_Reg(AUDBUF_CFG0, 0xE0AE, 0xffffffff);	/* Disable voice buffer */
		Ana_Set_Reg(AUDBUF_CFG2, 0x0489, 0xffffffff);	/* Set HPR/HPL gain as 0dB */

		/* volume setting */
		Apply_Speaker_Gain();
	} else {
		pr_debug("turn off Speaker_Amp_Change\n");
		if (GetDLStatus() == false) {
			TurnOffDacPower();
		}
		Ana_Set_Reg(AUDDAC_CFG0, 0xE149, 0xffffffff);	/* Disable HPR/HPL */
		Ana_Set_Reg(AUDBUF_CFG6, 0x00E8, 0xffffffff);	/* Disable LOR/LOL */
		Ana_Set_Reg(AUDDAC_CFG0, 0x0000, 0xffffffff);	/* Disable Audio DAC */
		Ana_Set_Reg(AUDCLKGEN_CFG0, 0x5500, 0xffffffff);	/* Disable AUD_CLK */
		Ana_Set_Reg(IBIASDIST_CFG0, 0x0192, 0xffffffff);	/* Disable IBIST */
		Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0028, 0xffffffff);	/* Disable NV regulator (-1.6V) */
		Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0000, 0xffffffff);	/* Disable cap-less LDOs (1.6V) */
	}

}


static int Headset_Speaker_Amp_Get(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()\n", __func__);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_SPEAKER_HEADSET_R];
	return 0;
}

static int Headset_Speaker_Amp_Set(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	/* struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol); */

	pr_debug("%s() gain = %lu\n ", __func__, ucontrol->value.integer.value[0]);
	if (ucontrol->value.integer.value[0]) {
		Headset_Speaker_Amp_Change(true);
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_SPEAKER_HEADSET_R] =
		    ucontrol->value.integer.value[0];
	} else {
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_VOLUME_SPEAKER_HEADSET_R] =
		    ucontrol->value.integer.value[0];
		Headset_Speaker_Amp_Change(false);
	}
	return 0;
}

#ifdef CONFIG_MTK_SPEAKER
static const char *speaker_amp_function[] = { "CALSSD", "CLASSAB", "RECEIVER" };

static const char *speaker_PGA_function[] =
    { "MUTE", "0Db", "1Db", "2Db", "3Db", "4Db", "5Db", "6Db", "7Db", "8Db", "9Db", "10Db",
	"11Db", "12Db", "13Db", "14Db", "15Db", "16Db", "17Db"
};
static const char *speaker_OC_function[] = { "Off", "On" };
static const char *speaker_CS_function[] = { "Off", "On" };
static const char *speaker_CSPeakDetecReset_function[] = { "Off", "On" };



static int Audio_Speaker_Class_Set(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	mutex_lock(&Ana_Ctrl_Mutex);
	Speaker_mode = ucontrol->value.integer.value[0];
	mutex_unlock(&Ana_Ctrl_Mutex);
	return 0;
}

static int Audio_Speaker_Class_Get(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = Speaker_mode;
	return 0;
}

static int Audio_Speaker_Pga_Gain_Set(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	Speaker_pga_gain = ucontrol->value.integer.value[0];
	Ana_Set_Reg(SPK_CON9, Speaker_pga_gain << 8, 0xf << 8);
	return 0;
}

static int Audio_Speaker_OcFlag_Get(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	mSpeaker_Ocflag = GetSpeakerOcFlag();
	ucontrol->value.integer.value[0] = mSpeaker_Ocflag;
	return 0;
}

static int Audio_Speaker_OcFlag_Set(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s is not support setting\n", __func__);
	return 0;
}

static int Audio_Speaker_Pga_Gain_Get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = Speaker_pga_gain;
	return 0;
}

static int Audio_Speaker_Current_Sensing_Set(struct snd_kcontrol *kcontrol,
					     struct snd_ctl_elem_value *ucontrol)
{
	if (ucontrol->value.integer.value[0]) {
		/* Ana_Set_Reg (SPK_CON12,  0x9300, 0xffff);CON12 invalid @ 6332 */
		Ana_Set_Reg(SPK_CON16, 0x8000, 0x8000);	/* [15]ISENSE enable */
		Ana_Set_Reg(SPK_CON14, 0x0040, 0x0040);	/* [6]VSENSE enable */
	}
	{
		/* Ana_Set_Reg (SPK_CON12,  0x1300, 0xffff); CON12 invalid @ 6332 */
		Ana_Set_Reg(SPK_CON16, 0x0, 0x8000);	/* [15]ISENSE disable */
		Ana_Set_Reg(SPK_CON14, 0x0, 0x0040);	/* [6]VSENSE enable */
	}
	return 0;
}

static int Audio_Speaker_Current_Sensing_Get(struct snd_kcontrol *kcontrol,
					     struct snd_ctl_elem_value *ucontrol)
{
	/* ucontrol->value.integer.value[0] = (Ana_Get_Reg (SPK_CON12)>>15)&0x01; */
	ucontrol->value.integer.value[0] = (Ana_Get_Reg(SPK_CON16) >> 15) & 0x01;	/* [15]ISENSE */
	return 0;
}

static int Audio_Speaker_Current_Sensing_Peak_Detector_Set(struct snd_kcontrol *kcontrol,
							   struct snd_ctl_elem_value *ucontrol)
{
	/*
	   if (ucontrol->value.integer.value[0])
	   Ana_Set_Reg (SPK_CON12,  1<<14, 1<<14);
	   else
	   Ana_Set_Reg (SPK_CON12,  0, 1<<14);
	 */
	return 0;
}

static int Audio_Speaker_Current_Sensing_Peak_Detector_Get(struct snd_kcontrol *kcontrol,
							   struct snd_ctl_elem_value *ucontrol)
{
	/*
	   ucontrol->value.integer.value[0] = (Ana_Get_Reg (SPK_CON12)>>14)&0x01;
	 */
	return 0;
}


static const struct soc_enum Audio_Speaker_Enum[] = {
	/* speaker class setting */
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(speaker_amp_function), speaker_amp_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(speaker_PGA_function), speaker_PGA_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(speaker_OC_function), speaker_OC_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(speaker_CS_function), speaker_CS_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(speaker_CSPeakDetecReset_function),
			    speaker_CSPeakDetecReset_function),
};

static const struct snd_kcontrol_new mt6331_snd_Speaker_controls[] = {
	SOC_ENUM_EXT("Audio_Speaker_class_Switch", Audio_Speaker_Enum[0], Audio_Speaker_Class_Get,
		     Audio_Speaker_Class_Set),
	SOC_ENUM_EXT("Audio_Speaker_PGA_gain", Audio_Speaker_Enum[1], Audio_Speaker_Pga_Gain_Get,
		     Audio_Speaker_Pga_Gain_Set),
	SOC_ENUM_EXT("Audio_Speaker_OC_Falg", Audio_Speaker_Enum[2], Audio_Speaker_OcFlag_Get,
		     Audio_Speaker_OcFlag_Set),
	SOC_ENUM_EXT("Audio_Speaker_CurrentSensing", Audio_Speaker_Enum[3],
		     Audio_Speaker_Current_Sensing_Get, Audio_Speaker_Current_Sensing_Set),
	SOC_ENUM_EXT("Audio_Speaker_CurrentPeakDetector", Audio_Speaker_Enum[4],
		     Audio_Speaker_Current_Sensing_Peak_Detector_Get,
		     Audio_Speaker_Current_Sensing_Peak_Detector_Set),
};

#endif
/* int PMIC_IMM_GetOneChannelValue(int dwChannel, int deCount, int trimd); */

#define HW_BUFFER_LENGTH 21
const uint32 V_Buffer_Table[HW_BUFFER_LENGTH] = {
	MT6332_AUXADC_ADC19,
	MT6332_AUXADC_ADC20,
	MT6332_AUXADC_ADC21,
	MT6332_AUXADC_ADC22,
	MT6332_AUXADC_ADC23,
	MT6332_AUXADC_ADC24,
	MT6332_AUXADC_ADC25,
	MT6332_AUXADC_ADC26,
	MT6332_AUXADC_ADC27,
	MT6332_AUXADC_ADC28,
	MT6332_AUXADC_ADC29,
	MT6332_AUXADC_ADC30,
	MT6332_AUXADC_ADC0,
	MT6332_AUXADC_ADC1,
	MT6332_AUXADC_ADC2,
	MT6332_AUXADC_ADC3,
	MT6332_AUXADC_ADC4,
	MT6332_AUXADC_ADC5,
	MT6332_AUXADC_ADC6,
	MT6332_AUXADC_ADC7,
	MT6332_AUXADC_ADC8
};

const uint32 I_Buffer_Table[HW_BUFFER_LENGTH] = {
	MT6332_AUXADC_ADC31,
	MT6332_AUXADC_ADC32,
	MT6332_AUXADC_ADC33,
	MT6332_AUXADC_ADC34,
	MT6332_AUXADC_ADC35,
	MT6332_AUXADC_ADC36,
	MT6332_AUXADC_ADC37,
	MT6332_AUXADC_ADC38,
	MT6332_AUXADC_ADC39,
	MT6332_AUXADC_ADC40,
	MT6332_AUXADC_ADC41,
	MT6332_AUXADC_ADC42,
	MT6332_AUXADC_ADC9,
	MT6332_AUXADC_ADC10,
	MT6332_AUXADC_ADC11,
	MT6332_AUXADC_ADC12,
	MT6332_AUXADC_ADC13,
	MT6332_AUXADC_ADC14,
	MT6332_AUXADC_ADC15,
	MT6332_AUXADC_ADC16,
	MT6332_AUXADC_ADC17
};


static int Audio_AuxAdcData_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	/*
	   pr_debug("Audio_AuxAdcData_Get dAuxAdcChannel = %d\n", dAuxAdcChannel);
	   ucontrol->value.integer.value[0] = PMIC_IMM_GetOneChannelValue(dAuxAdcChannel, 1, 0);
	   pr_debug("Audio_AuxAdcData_Get Data = 0x%lx\n", ucontrol->value.integer.value[0]);
	 */
#if 1
	pr_debug("+Audio_AuxAdcData_Get : TODO");
	ucontrol->value.integer.value[0] = 0;
#else
	const int dRecCount = 2048;
	int dRecReadIndex = 0;
	int cnt1, cnt2, iv_queue, v_cnt, i_cnt, cnt, ov_flag, i, hw_read_idx;
	int dMax, dCurValue;
	pr_debug("+Audio_AuxAdcData_Get");
	Ana_Set_Reg(MT6332_AUXADC_CON13, 20, 0x01FF);	/* [0:8]: period */
	Ana_Set_Reg(MT6332_AUXADC_CON12, 21, 0x007F);	/* Set Buffer Length */
	Ana_Set_Reg(MT6332_AUXADC_CON36, 0xAB, 0x01FF);	/* Set Channel 10 & 11 for V and I */
	Ana_Set_Reg(MT6332_AUXADC_CON12, 0x8000, 0x8000);	/* [15]Set Speaker mode */
	Ana_Set_Reg(MT6332_AUXADC_CON13, 0x0200, 0x0200);	/* [9]: enable */
	dMax = dCurValue = 0;
	hw_read_idx = 0;
	pr_debug("MT6332_AUXADC_CON12 [%x]\n", Ana_Get_Reg(MT6332_AUXADC_CON12));
	pr_debug("MT6332_AUXADC_CON13 [%x]\n", Ana_Get_Reg(MT6332_AUXADC_CON13));
	pr_debug("MT6332_AUXADC_CON36 [%x]\n", Ana_Get_Reg(MT6332_AUXADC_CON36));

	do {

		iv_queue = Ana_Get_Reg(MT6332_AUXADC_CON33);
		/* pr_debug("MT6332_AUXADC_CON33 [%x]\n",Ana_Get_Reg(MT6332_AUXADC_CON33)); */
		v_cnt = (iv_queue >> 8) & 0x3F;
		i_cnt = iv_queue & 0x3F;
		ov_flag = iv_queue & 0x8000;

		if (ov_flag != 0) {
			pr_debug("%s overflow\n", __func__);
			break;
		}

		if ((v_cnt > 0) && (i_cnt > 0)) {
			/*  */
			if (v_cnt > i_cnt) {
				cnt = i_cnt;
			} else {
				cnt = v_cnt;
			}

			pr_debug("cnt [%d] dRecReadIndex [%d]\n", cnt, dRecReadIndex);
			if (cnt + hw_read_idx >= HW_BUFFER_LENGTH) {
				cnt1 = HW_BUFFER_LENGTH - hw_read_idx;
				cnt2 = cnt - cnt1;
			} else {
				cnt1 = cnt;
				cnt2 = 0;
			}

			for (i = 0; i < cnt1; i++) {
				int v_tmp, i_tmp;

				i_tmp = Ana_Get_Reg(I_Buffer_Table[hw_read_idx]);
				v_tmp = Ana_Get_Reg(V_Buffer_Table[hw_read_idx]);
				/*
				   if( hw_read_idx == 19)
				   {
				   bufferBase[ring_write_idx++] = ((v_tmp >> 3) & 0xFFF); //LSB 15 bits
				   }
				   else
				   {
				   bufferBase[ring_write_idx++] = (v_tmp & 0xFFF); //LSB 12 bits
				   }
				 */
				if (hw_read_idx == 17 || hw_read_idx == 18 || hw_read_idx == 19) {
					dCurValue = ((i_tmp >> 3) & 0xFFF);	/* LSB 15 bits */
				} else {
					dCurValue = (i_tmp & 0xFFF);	/* LSB 12 bits */
				}

				if (/*(v_tmp & 0x8000) == 0 || */ (i_tmp & 0x8000) == 0) {
					/* must_print("AUXADC_CON33=0x%x at %d\n\n", iv_queue, hw_read_idx); */
					/* must_print("v_tmp=0x%x i_tmp= 0x%x, hw_read_idx %d, V_Addr 0x%x, I_Addr 0x%x\n\n", v_tmp, i_tmp, hw_read_idx, I_Buffer_Table[hw_read_idx], V_Buffer_Table[hw_read_idx]); */
				}
				if (dCurValue > dMax) {
					dMax = dCurValue;
				}
				hw_read_idx++;
			}
			if (hw_read_idx >= HW_BUFFER_LENGTH) {
				hw_read_idx = 0;
			}
			/* If warp to head, do second round */
			for (i = 0; i < cnt2; i++) {
				int v_tmp, i_tmp;

				i_tmp = Ana_Get_Reg(I_Buffer_Table[hw_read_idx]);
				v_tmp = Ana_Get_Reg(V_Buffer_Table[hw_read_idx]);
				/*
				   if( hw_read_idx == 19)
				   {
				   bufferBase[ring_write_idx++] = ((v_tmp >> 3)& 0xFFF); //LSB 15 bits
				   }
				   else
				   {
				   bufferBase[ring_write_idx++] = (v_tmp & 0xFFF); //LSB 12 bits
				   }
				 */
				if (hw_read_idx == 17 || hw_read_idx == 18 || hw_read_idx == 19) {
					dCurValue = ((i_tmp >> 3) & 0xFFF);	/* LSB 15 bits */
				} else {
					dCurValue = (i_tmp & 0xFFF);	/* LSB 12 bits */
				}
				if (/*(v_tmp & 0x8000) == 0 || */ (i_tmp & 0x8000) == 0) {
					/* must_print("AUXADC_CON33=0x%x at %d\n\n", iv_queue, hw_read_idx); */
					/* must_print("v_tmp=0x%x i_tmp= 0x%x, hw_read_idx %d, V_Addr 0x%x, I_Addr 0x%x\n\n", v_tmp, i_tmp, hw_read_idx, I_Buffer_Table[hw_read_idx], V_Buffer_Table[hw_read_idx]); */
				}
				if (dCurValue > dMax) {
					dMax = dCurValue;
				}
				hw_read_idx++;
			}
			if (hw_read_idx >= HW_BUFFER_LENGTH) {
				hw_read_idx = 0;
			}
			dRecReadIndex += cnt;
		}

	}
	while (dRecCount > dRecReadIndex);


	Ana_Set_Reg(MT6332_AUXADC_CON12, 0, 0x8000);	/* [15]Set Speaker mode */
	Ana_Set_Reg(MT6332_AUXADC_CON13, 0, 0x0200);	/* [9]: enable */

	ucontrol->value.integer.value[0] = dMax;
#endif
	pr_debug("%s dMax = 0x%lx\n", __func__, ucontrol->value.integer.value[0]);
	return 0;

}

static int Audio_AuxAdcData_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	dAuxAdcChannel = ucontrol->value.integer.value[0];
	pr_debug("%s dAuxAdcChannel = 0x%x\n", __func__, dAuxAdcChannel);
	return 0;
}


static const struct snd_kcontrol_new Audio_snd_auxadc_controls[] = {
	SOC_SINGLE_EXT("Audio AUXADC Data", SND_SOC_NOPM, 0, 0x80000, 0, Audio_AuxAdcData_Get,
		       Audio_AuxAdcData_Set),
};


static const char *amp_function[] = { "Off", "On" };

/* static const char *DAC_SampleRate_function[] = {"8000", "11025", "16000", "24000", "32000", "44100", "48000"}; */
static const char *DAC_DL_PGA_Headset_GAIN[] =
    { "8Db", "7Db", "6Db", "5Db", "4Db", "3Db", "2Db", "1Db", "0Db", "-1Db", "-2Db", "-3Db",
	"-4Db", "-5Db", "-6Db", "-7Db", "-8Db", "-9Db", "-10Db", "-40Db"
};

static const char *DAC_DL_PGA_Handset_GAIN[] =
    { "8Db", "7Db", "6Db", "5Db", "4Db", "3Db", "2Db", "1Db", "0Db", "-1Db", "-2Db", "-3Db",
	"-4Db", "-5Db", "-6Db", "-7Db", "-8Db", "-9Db", "-10Db", "-40Db"
};

static const char *DAC_DL_PGA_Speaker_GAIN[] =
    { "8Db", "7Db", "6Db", "5Db", "4Db", "3Db", "2Db", "1Db", "0Db", "-1Db", "-2Db", "-3Db",
	"-4Db", "-5Db", "-6Db", "-7Db", "-8Db", "-9Db", "-10Db", "-40Db"
};

/* static const char *Voice_Mux_function[] = {"Voice", "Speaker"}; */

static int Lineout_PGAL_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("Speaker_PGA_Get = %d\n",
		 mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_SPKL]);
	ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_SPKL];
	return 0;
}

static int Lineout_PGAL_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int index = 0;
	pr_debug("%s(), index = %d\n", __func__, ucontrol->value.enumerated.item[0]);

	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(DAC_DL_PGA_Speaker_GAIN)) {
		pr_debug("return -EINVAL\n");
		return -EINVAL;
	}
	index = ucontrol->value.integer.value[0];
	if (ucontrol->value.enumerated.item[0] == (ARRAY_SIZE(DAC_DL_PGA_Speaker_GAIN) - 1)) {
		index = 0x1f;
	}
	Ana_Set_Reg(ZCD_CON1, index, 0x001f);
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_SPKL] = ucontrol->value.integer.value[0];
	return 0;
}

static int Lineout_PGAR_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s  = %d\n", __func__, mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_SPKR]);
	ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_SPKR];
	return 0;
}

static int Lineout_PGAR_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	/* struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol); */
	int index = 0;
	pr_debug("%s(), index = %d\n", __func__, ucontrol->value.enumerated.item[0]);

	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(DAC_DL_PGA_Speaker_GAIN)) {
		pr_debug("return -EINVAL\n");
		return -EINVAL;
	}
	index = ucontrol->value.integer.value[0];
	if (ucontrol->value.enumerated.item[0] == (ARRAY_SIZE(DAC_DL_PGA_Speaker_GAIN) - 1)) {
		index = 0x1f;
	}
	Ana_Set_Reg(ZCD_CON1, index << 7, 0x0f10);
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_SPKR] = ucontrol->value.integer.value[0];
	return 0;
}

static int Handset_PGA_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("Handset_PGA_Get = %d\n",
		 mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HSOUTL]);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HSOUTL];
	return 0;
}

static int Handset_PGA_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	/* struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol); */
	int index = 0;

	pr_debug("%s(), index = %d\n", __func__, ucontrol->value.enumerated.item[0]);

	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(DAC_DL_PGA_Handset_GAIN)) {
		pr_debug("return -EINVAL\n");
		return -EINVAL;
	}
	index = ucontrol->value.integer.value[0];
	if (ucontrol->value.enumerated.item[0] == (ARRAY_SIZE(DAC_DL_PGA_Handset_GAIN) - 1)) {
		index = 0x1f;
	}
	Ana_Set_Reg(ZCD_CON3, index, 0x001f);
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HSOUTL] =
	    ucontrol->value.integer.value[0];
	return 0;
}


static void HeadsetLVolumeSet(void)
{
	int index = 0;
	pr_debug("%s\n", __func__);
	index = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTL];
	Ana_Set_Reg(ZCD_CON2, index, 0x001f);
}

static int Headset_PGAL_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("Headset_PGAL_Get = %d\n",
		 mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTL]);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTL];
	return 0;
}

static int Headset_PGAL_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	/* struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol); */
	int index = 0;

	pr_debug("%s(), index = %d arraysize = %d\n", __func__, ucontrol->value.enumerated.item[0],
		 ARRAY_SIZE(DAC_DL_PGA_Headset_GAIN));

	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(DAC_DL_PGA_Headset_GAIN)) {
		pr_debug("return -EINVAL\n");
		return -EINVAL;
	}
	index = ucontrol->value.integer.value[0];
	if (ucontrol->value.enumerated.item[0] == (ARRAY_SIZE(DAC_DL_PGA_Headset_GAIN) - 1)) {
		index = 0x1f;
	}
	Ana_Set_Reg(ZCD_CON2, index, 0x001f);
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTL] =
	    ucontrol->value.integer.value[0];
	return 0;
}

static void HeadsetRVolumeSet(void)
{
	int index = 0;
	pr_debug("%s\n", __func__);
	index = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTR];
	Ana_Set_Reg(ZCD_CON2, index << 7, 0x0f80);
}

static int Headset_PGAR_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("Headset_PGAR_Get = %d\n",
		 mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTR]);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTR];
	return 0;
}

static int Headset_PGAR_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	/* struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol); */
	int index = 0;

	pr_debug("%s(), index = %d\n", __func__, ucontrol->value.enumerated.item[0]);

	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(DAC_DL_PGA_Headset_GAIN)) {
		pr_debug("return -EINVAL\n");
		return -EINVAL;
	}
	index = ucontrol->value.integer.value[0];
	if (ucontrol->value.enumerated.item[0] == (ARRAY_SIZE(DAC_DL_PGA_Headset_GAIN) - 1)) {
		index = 0x1f;
	}
	Ana_Set_Reg(ZCD_CON2, index << 7, 0x0f80);
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTR] =
	    ucontrol->value.integer.value[0];
	return 0;
}


static int Voice_Mux_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("Voice_Mux_Get = %d\n", mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_VOICE]);
	ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_VOICE];
	return 0;
}

static int Voice_Mux_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{

	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	pr_debug("%s()\n", __func__);
	if (ucontrol->value.integer.value[0]) {
		pr_debug("%s()\n", __func__);
		snd_soc_dapm_disable_pin(&codec->dapm, "SPEAKER");
		snd_soc_dapm_disable_pin(&codec->dapm, "RX_BIAS");
		snd_soc_dapm_sync(&codec->dapm);
	} else {
		pr_debug("%s()\n", __func__);
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
	pr_debug("Audio_Hp_Impedance_Get = %d\n", mHp_Impedance);
	ucontrol->value.integer.value[0] = mHp_Impedance;
	return 0;

}

static int Audio_Hp_Impedance_Set(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	mHp_Impedance = ucontrol->value.integer.value[0];
	pr_debug("%s Audio_Hp_Impedance_Set = 0x%x\n", __func__, mHp_Impedance);
	return 0;
}

static const struct soc_enum Audio_DL_Enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(amp_function), amp_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(amp_function), amp_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(amp_function), amp_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(amp_function), amp_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(amp_function), amp_function),
	/* here comes pga gain setting */
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(DAC_DL_PGA_Headset_GAIN), DAC_DL_PGA_Headset_GAIN),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(DAC_DL_PGA_Headset_GAIN), DAC_DL_PGA_Headset_GAIN),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(DAC_DL_PGA_Handset_GAIN), DAC_DL_PGA_Handset_GAIN),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(DAC_DL_PGA_Speaker_GAIN), DAC_DL_PGA_Speaker_GAIN),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(DAC_DL_PGA_Speaker_GAIN), DAC_DL_PGA_Speaker_GAIN),
};

static const struct snd_kcontrol_new mt6331_snd_controls[] = {
	SOC_ENUM_EXT("Audio_Amp_R_Switch", Audio_DL_Enum[0], Audio_AmpR_Get, Audio_AmpR_Set),
	SOC_ENUM_EXT("Audio_Amp_L_Switch", Audio_DL_Enum[1], Audio_AmpL_Get, Audio_AmpL_Set),
	SOC_ENUM_EXT("Voice_Amp_Switch", Audio_DL_Enum[2], Voice_Amp_Get, Voice_Amp_Set),
	SOC_ENUM_EXT("Speaker_Amp_Switch", Audio_DL_Enum[3], Speaker_Amp_Get, Speaker_Amp_Set),
	SOC_ENUM_EXT("Headset_Speaker_Amp_Switch", Audio_DL_Enum[4], Headset_Speaker_Amp_Get,
		     Headset_Speaker_Amp_Set),
	SOC_ENUM_EXT("Headset_PGAL_GAIN", Audio_DL_Enum[5], Headset_PGAL_Get, Headset_PGAL_Set),
	SOC_ENUM_EXT("Headset_PGAR_GAIN", Audio_DL_Enum[6], Headset_PGAR_Get, Headset_PGAR_Set),
	SOC_ENUM_EXT("Handset_PGA_GAIN", Audio_DL_Enum[7], Handset_PGA_Get, Handset_PGA_Set),
	SOC_ENUM_EXT("Lineout_PGAR_GAIN", Audio_DL_Enum[8], Lineout_PGAR_Get, Lineout_PGAR_Set),
	SOC_ENUM_EXT("Lineout_PGAL_GAIN", Audio_DL_Enum[9], Lineout_PGAL_Get, Lineout_PGAL_Set),
	SOC_SINGLE_EXT("Audio HP Impedance", SND_SOC_NOPM, 0, 512, 0, Audio_Hp_Impedance_Get,
		       Audio_Hp_Impedance_Set),
};

static const struct snd_kcontrol_new mt6331_Voice_Switch[] = {
	SOC_DAPM_ENUM_EXT("Voice Mux", Audio_DL_Enum[10], Voice_Mux_Get, Voice_Mux_Set),
};

void SetMicPGAGain(void)
{
	int index = 0;
	index = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP1];
	Ana_Set_Reg(AUDPREAMPGAIN_CFG0, index, 0x0007);
	index = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP2];
	Ana_Set_Reg(AUDPREAMPGAIN_CFG0, index << 4, 0x0070);
	index = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP3];
	Ana_Set_Reg(AUDPREAMPGAIN_CFG0, index << 8, 0x0700);
	index = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP4];
	Ana_Set_Reg(AUDPREAMPGAIN_CFG0, index << 12, 0x7000);
}

static bool GetAdcStatus(void)
{
	int i = 0;
	for (i = AUDIO_ANALOG_DEVICE_IN_ADC1; i < AUDIO_ANALOG_DEVICE_MAX; i++) {
		if (mCodec_data->mAudio_Ana_DevicePower[i] == true) {
			return true;
		}
	}
	return false;
}


static bool TurnOnADcPowerACC(int ADCType, bool enable)
{
	pr_debug("%s ADCType = %d enable = %d\n", __func__, ADCType, enable);
	if (enable) {
		uint32 SampleRate_VUL1 = mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC];
		uint32 SampleRate_VUL2 = mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC_2];
		if (GetAdcStatus() == false) {
			NvregEnable(true);
			Ana_Set_Reg(TOP_CLKSQ, 0x0003, 0xffff);	/* Enable CLKSQ */
			ClsqAuxEnable(true);
			ClsqEnable(true);

			Ana_Set_Reg(AUDADC_CFG0, 0x0400, 0xffff);	/* Enable ADC CLK */
			/* Ana_Set_Reg(AUDMICBIAS_CFG0, 0x78F, 0xffff);   //Enable MICBIAS0,1 (2.7V) */
			OpenMicbias1();
			OpenMicbias0();
			Ana_Set_Reg(AUDMICBIAS_CFG1, 0x78F, 0xffff);	/* Enable MICBIAS2,3 (2.7V) */
			Ana_Set_Reg(AUDLDO_NVREG_CFG1, 0x0007, 0xffff);	/* Enable LCLDO18_ENC (1.8V), Remote-Sense */
			Ana_Set_Reg(AUDLDO_NVREG_CFG2, 0x7777, 0xffff);	/* Enable LCLDO19_ADCCH0_1, Remote-Sense */
			/* Ana_Set_Reg(AUDPREAMPGAIN_CFG0, 0x3333, 0xffff);   //Set PGA CH0_1 gain = 18dB */
			SetMicPGAGain();
			Ana_Set_Reg(AUDPREAMP_CFG0, 0x0051, 0xffff);	/* Enable PGA CH0_1 (CH0 in) */
			Ana_Set_Reg(AUDPREAMP_CFG1, 0x16d5, 0xffff);	/* Enable ADC CH0_1 (PGA in) */

			/* here to set digital part */
			Topck_Enable(true);
			AdcClockEnable(true);
			Ana_Set_Reg(AFE_ADDA2_UL_SRC_CON1_L, 0x0000, 0xffff);	/* power on ADC clk */

			Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0000, 0xffff);	/* power on clock */
			Ana_Set_Reg(AFE_ADDA2_UL_SRC_CON1_L, 0x0000, 0xffff);	/* power on ADC clk */
			Ana_Set_Reg(PMIC_AFE_TOP_CON0, 0x0000, 0xffff);	/* configure ADC setting */

			Ana_Set_Reg(AFE_MIC_ARRAY_CFG, 0x44e4, 0xffff);	/* AFE_MIC_ARRAY_CFG */
			Ana_Set_Reg(AFE_UL_DL_CON0, 0x0001, 0xffff);	/* turn on afe */

			Ana_Set_Reg(AFE_PMIC_NEWIF_CFG2, 0x302F | (GetULNewIFFrequency(mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC]) << 10), 0xffff);	/* config UL up8x_rxif adc voice mode */
			Ana_Set_Reg(AFE_UL_SRC0_CON0_H, (ULSampleRateTransform(SampleRate_VUL1) << 3 | ULSampleRateTransform(SampleRate_VUL1) << 1), 0x001f);	/* ULsampling rate */

			Ana_Set_Reg(AFE_ADDA2_UL_SRC_CON0_H, (ULSampleRateTransform(SampleRate_VUL2) << 3 | ULSampleRateTransform(SampleRate_VUL2) << 1), 0x001f);	/* ULsampling rate */
			Ana_Set_Reg(AFE_ADDA2_UL_SRC_CON0_L, 0x0041, 0xffff);

			Ana_Set_Reg(AFE_UL_SRC0_CON0_L, 0x0041, 0xffff);	/* power on uplink */

		}
		/* todo , open ADC indivisaully */
		if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC1) {
			Ana_Set_Reg(AUDLDO_NVREG_CFG2, 0x0007, 0x000f);	/* Enable LCLDO19_ADCCH0_1, Remote-Sense */
			Ana_Set_Reg(AUDPREAMP_CFG0, 0x0051, 0xffff);	/* Enable PGA CH0_1 */
			Ana_Set_Reg(AUDPREAMP_CFG1, 0x0055, 0x007f);	/* Enable ADC CH0_1 (PGA in) */
			AudioPreAmp1_Sel(mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_PREAMP_1]);
		} else if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC2) {
			Ana_Set_Reg(AUDLDO_NVREG_CFG2, 0x0070, 0x00f0);	/* Enable LCLDO19_ADCCH2, Remote-Sensen) */
			Ana_Set_Reg(AUDPREAMP_CFG0, 0x0040, 0x03c0);	/* Enable PGA CH2 */
			Ana_Set_Reg(AUDPREAMP_CFG1, 0x0055, 0x007f);	/* Enable ADC CH0_1 (PGA in) */
		} else if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC3) {
			Ana_Set_Reg(AUDLDO_NVREG_CFG2, 0x0700, 0x0f00);	/* Enable LCLDO19_ADCCH3, Remote-Sense */
			Ana_Set_Reg(AUDPREAMP_CFG1, 0x0280, 0x0380);	/* Enable ADC CH3 (PGA in) */
			Ana_Set_Reg(AUDPREAMP_CFG2, 0x0001, 0x000f);	/* Enable preamp CH3 */
			Ana_Set_Reg(AFE_ADDA2_UL_SRC_CON0_H, (ULSampleRateTransform(SampleRate_VUL2) << 3 | ULSampleRateTransform(SampleRate_VUL2) << 1), 0x001f);	/* ULsampling rate */
			Ana_Set_Reg(AFE_ADDA2_UL_SRC_CON0_L, 0x0041, 0xffff);
		} else if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC4) {
			Ana_Set_Reg(AUDLDO_NVREG_CFG2, 0x7000, 0xf000);	/* Enable LCLDO19_ADCCH4, Remote-Sense */
			Ana_Set_Reg(AUDPREAMP_CFG1, 0x1400, 0x1c00);	/* Enable ADC CH4 (PGA in) */
			Ana_Set_Reg(AUDPREAMP_CFG2, 0x0010, 0x00f0);	/* Enable preamp CH4 */
			Ana_Set_Reg(AFE_ADDA2_UL_SRC_CON0_H, (ULSampleRateTransform(SampleRate_VUL2) << 3 | ULSampleRateTransform(SampleRate_VUL2) << 1), 0x001f);	/* ULsampling rate */
			Ana_Set_Reg(AFE_ADDA2_UL_SRC_CON0_L, 0x0041, 0xffff);
		} else {
			pr_debug("\n");
		}
	} else {
		if (GetAdcStatus() == false) {
			Ana_Set_Reg(AFE_UL_SRC0_CON0_L, 0x0000, 0xffff);	/* power on uplink */
			Ana_Set_Reg(AFE_ADDA2_UL_SRC_CON0_L, 0x0000, 0xffff);	/* power on uplink */

			Ana_Set_Reg(AUDPREAMP_CFG0, 0x0000, 0xffff);	/* Disable ADC CH0_1 (PGA in) Disable ADC CH_2 (PGA in) */
			Ana_Set_Reg(AUDPREAMP_CFG1, 0x0000, 0xffff);	/* Disable PGA CH0_1 (CH0 in) Disable PGA CH_2 */
			Ana_Set_Reg(AUDPREAMPGAIN_CFG0, 0x0000, 0xffff);	/* Set PGA CH0_1 gain = 0dB  Set PGA CH_2 gain = 0dB */

			Ana_Set_Reg(AUDLDO_NVREG_CFG2, 0x2222, 0xffff);	/* disable LCLDO19_ADCCH0_1, Remote-Sense */
			Ana_Set_Reg(AUDLDO_NVREG_CFG1, 0x0002, 0xffff);	/* disable LCLDO18_ENC (1.8V), Remote-Sense */

			Ana_Set_Reg(AUDMICBIAS_CFG1, 0x2020, 0xffff);	/* power on clock */
			/* Ana_Set_Reg(AUDMICBIAS_CFG0, 0x0000, 0xffff);   //power on ADC clk */
			CloseMicbias1();
			CloseMicbias0();

			Ana_Set_Reg(AUDADC_CFG0, 0x0000, 0xffff);	/* configure ADC setting */
			ClsqAuxEnable(false);
			ClsqEnable(false);
			NvregEnable(false);
			Topck_Enable(false);
			if (GetDLStatus() == false) {
				/* check for if DL/UL will share same register */

			} else {

			}

		}
		/* todo , close analog */
		if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC1) {

		} else if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC2) {

		} else if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC3) {

		} else if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC4) {

		}
	}
	return true;
}


static bool TurnOnADcPowerDmic(int ADCType, bool enable)
{
	pr_debug("%s ADCType = %d enable = %d\n", __func__, ADCType, enable);
	if (enable) {
		uint32 ULIndex = GetULFrequency(mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC]);
		uint32 SampleRate_VUL1 = mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC];
		uint32 SampleRate_VUL2 = mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC_2];
		if (GetAdcStatus() == false) {
			NvregEnable(true);
			ClsqAuxEnable(true);
			ClsqEnable(true);
			Ana_Set_Reg(AUDDIGMI_CFG0, 0x0041, 0xffff);	/* Enable DMIC0 (BIAS=10) */
			Ana_Set_Reg(AUDDIGMI_CFG1, 0x0041, 0xffff);	/* Enable DMIC1 (BIAS=10) */

			Ana_Set_Reg(AUDADC_CFG0, 0x0400, 0xffff);	/* Enable ADC CLK */
			/* Ana_Set_Reg(AUDMICBIAS_CFG0, 0x78F, 0xffff);   //Enable MICBIAS0,1 (2.7V) */
			OpenMicbias1();
			OpenMicbias0();
			Ana_Set_Reg(AUDMICBIAS_CFG1, 0x780, 0xffff);	/* Enable MICBIAS2,3 (2.7V) */

			/* here to set digital part */
			Topck_Enable(true);
			AdcClockEnable(true);

			Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0000, 0xffff);	/* power on clock */
			Ana_Set_Reg(AFE_ADDA2_UL_SRC_CON1_L, 0x0000, 0xffff);	/* power on ADC clk */
			Ana_Set_Reg(PMIC_AFE_TOP_CON0, (ULIndex << 7) | (ULIndex << 6), 0xffff);	/* configure ADC setting */
			Ana_Set_Reg(AFE_UL_DL_CON0, 0x0001, 0xffff);	/* turn on afe */
			Ana_Set_Reg(AFE_UL_SRC0_CON0_H, (ULSampleRateTransform(SampleRate_VUL1) << 3 | ULSampleRateTransform(SampleRate_VUL1) << 1), 0x001f);	/* ULsampling rate */
			Ana_Set_Reg(AFE_UL_SRC0_CON0_H, (1 << 5) | (1 << 6), (1 << 5) | (1 << 6));	/* dmic open */

			Ana_Set_Reg(AFE_ADDA2_UL_SRC_CON0_H, (ULSampleRateTransform(SampleRate_VUL2) << 3 | ULSampleRateTransform(SampleRate_VUL2) << 1), 0x001f);	/* ULsampling rate */
			Ana_Set_Reg(AFE_UL_SRC0_CON0_H, (1 << 5) | (1 << 6), (1 << 5) | (1 << 6));	/* dmic open */
			Ana_Set_Reg(AFE_ADDA2_UL_SRC_CON0_L, 0x0043, 0xffff);

			Ana_Set_Reg(AFE_UL_SRC0_CON0_L, 0x0003, 0xffff);	/* power on uplink */

		}
		/* todo , open ADC indivisaully */
		if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC1) {
		} else if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC2) {
		} else if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC3) {

		} else if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC4) {

		} else {
			pr_debug("\n");
		}
	} else {
		if (GetAdcStatus() == false) {

			Ana_Set_Reg(AUDMICBIAS_CFG1, 0x2020, 0xffff);	/* Enable MICBIAS2,3 (2.7V) */
			CloseMicbias1();
			CloseMicbias0();

			Ana_Set_Reg(AUDDIGMI_CFG0, 0x0040, 0xffff);	/* Disable DMIC0 (BIAS=10) */
			Ana_Set_Reg(AUDDIGMI_CFG1, 0x0040, 0xffff);	/* Disable DMIC1 (BIAS=10) */

			Ana_Set_Reg(AFE_UL_SRC0_CON0_L, 0x0000, 0xffff);	/* power on uplink */
			Ana_Set_Reg(AFE_ADDA2_UL_SRC_CON0_L, 0x0000, 0xffff);	/* power on uplink */

			ClsqAuxEnable(false);
			ClsqEnable(false);
			NvregEnable(false);
			Topck_Enable(false);
			if (GetDLStatus() == false) {
				/* check for if DL/UL will share same register */

			} else {

			}

		}
		/* todo , close analog */
		if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC1) {

		} else if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC2) {

		} else if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC3) {

		} else if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC4) {

		}
	}
	return true;
}

static bool TurnOnADcPowerDCC(int ADCType, bool enable)
{
	pr_debug("%s ADCType = %d enable = %d\n", __func__, ADCType, enable);
	if (enable) {
		uint32 SampleRate_VUL1 = mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC];
		uint32 SampleRate_VUL2 = mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC_2];
		if (GetAdcStatus() == false) {
			NvregEnable(true);	/* Enable AUDGLB */
			ClsqAuxEnable(true);	/* Enable ADC CLK */
			ClsqEnable(true);

			/* here to set digital part */
			Topck_Enable(true);
			AdcClockEnable(true);

			Ana_Set_Reg(AUDADC_CFG0, 0x0400, 0xffff);	/* Enable ADC CLK */
			/* Ana_Set_Reg(AUDMICBIAS_CFG0, 0x78F, 0xffff);   //Enable MICBIAS0,1 (2.7V) */
			OpenMicbias1();
			OpenMicbias0();
			Ana_Set_Reg(AUDMICBIAS_CFG1, 0x78F, 0xffff);	/* Enable MICBIAS2,3 (2.7V) */
			Ana_Set_Reg(AUDLDO_NVREG_CFG1, 0x0007, 0xffff);	/* Enable LCLDO18_ENC (1.8V), Remote-Sense */
			Ana_Set_Reg(AUDLDO_NVREG_CFG2, 0x7777, 0xffff);	/* Enable LCLDO19_ADCCH0_1, Remote-Sense */

			Ana_Set_Reg(AFE_DCCLK_CFG0, 0x2061, 0xffff);	/* DC_26M_50K_EN */

			Ana_Set_Reg(AUDPREAMPGAIN_CFG0, 0x3333, 0xffff);	/* Set PGA CH0_1 gain = 18dB */
			SetMicPGAGain();

			Ana_Set_Reg(AUDPREAMP_CFG0, 0x01c7, 0xffff);	/* Enable PGA CH0_1 (CH0 in) */
			Ana_Set_Reg(AUDPREAMP_CFG1, 0x0055, 0xffff);	/* Enable ADC CH0_1 (PGA in) */
			Ana_Set_Reg(AUDPREAMP_CFG0, 0x00d3, 0xffff);	/* Enable PGA CH0_1 (CH0 in) */


			Ana_Set_Reg(AFE_ADDA2_UL_SRC_CON1_L, 0x0000, 0xffff);	/* power on ADC clk */

			Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0000, 0xffff);	/* power on clock */
			Ana_Set_Reg(AFE_ADDA2_UL_SRC_CON1_L, 0x0000, 0xffff);	/* power on ADC clk */
			Ana_Set_Reg(PMIC_AFE_TOP_CON0, 0x0000, 0xffff);	/* configure ADC setting */

			Ana_Set_Reg(AFE_MIC_ARRAY_CFG, 0x44e4, 0xffff);	/* AFE_MIC_ARRAY_CFG */
			Ana_Set_Reg(AFE_UL_DL_CON0, 0x0001, 0xffff);	/* turn on afe */

			Ana_Set_Reg(AFE_PMIC_NEWIF_CFG2, 0x302F | (GetULNewIFFrequency(mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC]) << 10), 0xffff);	/* config UL up8x_rxif adc voice mode */
			Ana_Set_Reg(AFE_UL_SRC0_CON0_H, (ULSampleRateTransform(SampleRate_VUL1) << 3 | ULSampleRateTransform(SampleRate_VUL1) << 1), 0x001f);	/* ULsampling rate */

			Ana_Set_Reg(AFE_ADDA2_UL_SRC_CON0_H, (ULSampleRateTransform(SampleRate_VUL2) << 3 | ULSampleRateTransform(SampleRate_VUL2) << 1), 0x001f);	/* ULsampling rate */
			Ana_Set_Reg(AFE_ADDA2_UL_SRC_CON0_L, 0x0041, 0xffff);

			Ana_Set_Reg(AFE_UL_SRC0_CON0_L, 0x0041, 0xffff);	/* power on uplink */

		}
		/* todo , open ADC indivisaully */
		if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC1) {

			Ana_Set_Reg(AUDLDO_NVREG_CFG2, 0x0007, 0x000f);	/* Enable LCLDO19_ADCCH0_1, Remote-Sense */

			Ana_Set_Reg(AUDPREAMP_CFG0, 0x0007, 0x003f);	/* Enable PGA CH0_1 */
			Ana_Set_Reg(AUDPREAMP_CFG1, 0x0055, 0x007f);	/* Enable ADC CH0_1 (PGA in) */
			Ana_Set_Reg(AUDPREAMP_CFG0, 0x0013, 0x003f);	/* Enable PGA CH0_1 */
			AudioPreAmp1_Sel(mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_PREAMP_1]);
		} else if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC2) {
			Ana_Set_Reg(AUDLDO_NVREG_CFG2, 0x0070, 0x00f0);	/* Enable LCLDO19_ADCCH2, Remote-Sensen) */

			Ana_Set_Reg(AUDPREAMP_CFG0, 0x01c0, 0x03c0);	/* Enable PGA CH2 */
			Ana_Set_Reg(AUDPREAMP_CFG1, 0x0055, 0x007f);	/* Enable ADC CH0_1 (PGA in) */
			Ana_Set_Reg(AUDPREAMP_CFG0, 0x00c0, 0x03c0);	/* Enable PGA CH2 */
		} else if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC3) {
			Ana_Set_Reg(AUDLDO_NVREG_CFG2, 0x0700, 0x0f00);	/* Enable LCLDO19_ADCCH3, Remote-Sense */

			Ana_Set_Reg(AUDPREAMP_CFG2, 0x0007, 0x000f);	/* Enable preamp CH3 */
			Ana_Set_Reg(AUDPREAMP_CFG1, 0x0280, 0x0380);	/* Enable ADC CH3 (PGA in) */
			Ana_Set_Reg(AUDPREAMP_CFG2, 0x0003, 0x000f);	/* Enable preamp CH3 */

			Ana_Set_Reg(AFE_ADDA2_UL_SRC_CON0_H, (ULSampleRateTransform(SampleRate_VUL2) << 3 | ULSampleRateTransform(SampleRate_VUL2) << 1), 0x001f);	/* ULsampling rate */
			Ana_Set_Reg(AFE_ADDA2_UL_SRC_CON0_L, 0x0041, 0xffff);
		} else if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC4) {
			Ana_Set_Reg(AUDLDO_NVREG_CFG2, 0x7000, 0xf000);	/* Enable LCLDO19_ADCCH4, Remote-Sense */

			Ana_Set_Reg(AUDPREAMP_CFG2, 0x0070, 0x00f0);	/* Enable preamp CH4 */
			Ana_Set_Reg(AUDPREAMP_CFG1, 0x1400, 0x1c00);	/* Enable ADC CH4 (PGA in) */
			Ana_Set_Reg(AUDPREAMP_CFG2, 0x0030, 0x00f0);	/* Enable preamp CH4 */
			Ana_Set_Reg(AFE_ADDA2_UL_SRC_CON0_H, (ULSampleRateTransform(SampleRate_VUL2) << 3 | ULSampleRateTransform(SampleRate_VUL2) << 1), 0x001f);	/* ULsampling rate */
			Ana_Set_Reg(AFE_ADDA2_UL_SRC_CON0_L, 0x0041, 0xffff);
		} else {
			pr_debug("\n");
		}
	} else {
		if (GetAdcStatus() == false) {
			Ana_Set_Reg(AFE_UL_SRC0_CON0_L, 0x0000, 0xffff);	/* power on uplink */
			Ana_Set_Reg(AFE_ADDA2_UL_SRC_CON0_L, 0x0000, 0xffff);	/* power on uplink */

			Ana_Set_Reg(AUDPREAMP_CFG0, 0x0000, 0xffff);	/* Disable ADC CH0_1 (PGA in) Disable ADC CH_2 (PGA in) */
			Ana_Set_Reg(AUDPREAMP_CFG1, 0x0000, 0xffff);	/* Disable PGA CH0_1 (CH0 in) Disable PGA CH_2 */
			Ana_Set_Reg(AUDPREAMPGAIN_CFG0, 0x0000, 0xffff);	/* Set PGA CH0_1 gain = 0dB  Set PGA CH_2 gain = 0dB */
			Ana_Set_Reg(AFE_DCCLK_CFG0, 0x0, 0xffff);	/* DC_26M_50K_ off */

			Ana_Set_Reg(AUDLDO_NVREG_CFG2, 0x2222, 0xffff);	/* disable LCLDO19_ADCCH0_1, Remote-Sense */
			Ana_Set_Reg(AUDLDO_NVREG_CFG1, 0x0002, 0xffff);	/* disable LCLDO18_ENC (1.8V), Remote-Sense */

			Ana_Set_Reg(AUDMICBIAS_CFG1, 0x2020, 0xffff);	/* power on clock */
			/* Ana_Set_Reg(AUDMICBIAS_CFG0, 0x0000, 0xffff);   //power on ADC clk */
			CloseMicbias1();
			CloseMicbias0();

			Ana_Set_Reg(AUDADC_CFG0, 0x0000, 0xffff);	/* configure ADC setting */

			ClsqAuxEnable(false);
			ClsqEnable(false);
			NvregEnable(false);
			Topck_Enable(false);
			if (GetDLStatus() == false) {
				Ana_Set_Reg(AFE_UL_DL_CON0, 0x0000, 0xffff);	/* turn off afe */
			} else {

			}

		}
		/* todo , close analog */
		if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC1) {

		} else if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC2) {

		} else if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC3) {

		} else if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC4) {

		}
	}
	return true;
}




/* here start uplink power function */
static const char *ADC_function[] = { "Off", "On" };
static const char *PreAmp_Mux_function[] = { "OPEN", "IN_ADC1", "IN_ADC2" };

/* static const char *ADC_SampleRate_function[] = {"8000", "16000", "32000", "48000"}; */
static const char *ADC_UL_PGA_GAIN[] = { "0Db", "6Db", "12Db", "18Db", "24Db", "30Db" };
static const char *Pmic_Digital_Mux[] = { "ADC1", "ADC2", "ADC3", "ADC4" };
static const char *Adc_Input_Sel[] = { "idle", "AIN", "Preamp" };
static const char *Audio_AnalogMic_Mode[] = { "ACCMODE", "DCCMODE", "DMIC" };


static const struct soc_enum Audio_UL_Enum[] = {
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
};

static int Audio_ADC1_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("Audio_ADC1_Get = %d\n",
		 mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC1]);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC1];
	return 0;
}

static int Audio_ADC1_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()\n", __func__);
	if (ucontrol->value.integer.value[0]) {
		if (mAudio_Analog_Mic1_mode == AUDIO_ANALOGUL_MODE_ACC) {
			TurnOnADcPowerACC(AUDIO_ANALOG_DEVICE_IN_ADC1, true);
		} else if (mAudio_Analog_Mic1_mode == AUDIO_ANALOGUL_MODE_DCC) {
			TurnOnADcPowerDCC(AUDIO_ANALOG_DEVICE_IN_ADC1, true);
		} else if (mAudio_Analog_Mic1_mode == AUDIO_ANALOGUL_MODE_DMIC) {
			TurnOnADcPowerDmic(AUDIO_ANALOG_DEVICE_IN_ADC1, true);
		}
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC1] =
		    ucontrol->value.integer.value[0];
	} else {
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC1] =
		    ucontrol->value.integer.value[0];
		if (mAudio_Analog_Mic1_mode == AUDIO_ANALOGUL_MODE_ACC) {
			TurnOnADcPowerACC(AUDIO_ANALOG_DEVICE_IN_ADC1, false);
		} else if (mAudio_Analog_Mic1_mode == AUDIO_ANALOGUL_MODE_DCC) {
			TurnOnADcPowerDCC(AUDIO_ANALOG_DEVICE_IN_ADC1, false);
		} else if (mAudio_Analog_Mic1_mode == AUDIO_ANALOGUL_MODE_DMIC) {
			TurnOnADcPowerDmic(AUDIO_ANALOG_DEVICE_IN_ADC1, false);
		}
	}
	return 0;
}

static int Audio_ADC2_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("Audio_ADC2_Get = %d\n",
		 mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC2]);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC2];
	return 0;
}

static int Audio_ADC2_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()\n", __func__);
	if (ucontrol->value.integer.value[0]) {
		if (mAudio_Analog_Mic2_mode == AUDIO_ANALOGUL_MODE_ACC) {
			TurnOnADcPowerACC(AUDIO_ANALOG_DEVICE_IN_ADC2, true);
		} else if (mAudio_Analog_Mic2_mode == AUDIO_ANALOGUL_MODE_DCC) {
			TurnOnADcPowerDCC(AUDIO_ANALOG_DEVICE_IN_ADC2, true);
		} else if (mAudio_Analog_Mic2_mode == AUDIO_ANALOGUL_MODE_DMIC) {
			TurnOnADcPowerDmic(AUDIO_ANALOG_DEVICE_IN_ADC2, true);
		}
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC2] =
		    ucontrol->value.integer.value[0];
	} else {
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC2] =
		    ucontrol->value.integer.value[0];
		if (mAudio_Analog_Mic2_mode == AUDIO_ANALOGUL_MODE_ACC) {
			TurnOnADcPowerACC(AUDIO_ANALOG_DEVICE_IN_ADC2, false);
		} else if (mAudio_Analog_Mic2_mode == AUDIO_ANALOGUL_MODE_DCC) {
			TurnOnADcPowerDCC(AUDIO_ANALOG_DEVICE_IN_ADC2, false);
		} else if (mAudio_Analog_Mic2_mode == AUDIO_ANALOGUL_MODE_DMIC) {
			TurnOnADcPowerDmic(AUDIO_ANALOG_DEVICE_IN_ADC2, false);
		}
	}
	return 0;
}

static int Audio_ADC3_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("Audio_ADC3_Get = %d\n",
		 mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC3]);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC3];
	return 0;
}

static int Audio_ADC3_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()\n", __func__);
	if (ucontrol->value.integer.value[0]) {
		if (mAudio_Analog_Mic3_mode == AUDIO_ANALOGUL_MODE_ACC) {
			TurnOnADcPowerACC(AUDIO_ANALOG_DEVICE_IN_ADC3, true);
		} else if (mAudio_Analog_Mic3_mode == AUDIO_ANALOGUL_MODE_DCC) {
			TurnOnADcPowerDCC(AUDIO_ANALOG_DEVICE_IN_ADC3, true);
		} else if (mAudio_Analog_Mic3_mode == AUDIO_ANALOGUL_MODE_DMIC) {
			TurnOnADcPowerDmic(AUDIO_ANALOG_DEVICE_IN_ADC3, true);
		}
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC3] =
		    ucontrol->value.integer.value[0];
	} else {
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC3] =
		    ucontrol->value.integer.value[0];

		if (mAudio_Analog_Mic3_mode == AUDIO_ANALOGUL_MODE_ACC) {
			TurnOnADcPowerACC(AUDIO_ANALOG_DEVICE_IN_ADC3, false);
		} else if (mAudio_Analog_Mic3_mode == AUDIO_ANALOGUL_MODE_DCC) {
			TurnOnADcPowerDCC(AUDIO_ANALOG_DEVICE_IN_ADC3, false);
		} else if (mAudio_Analog_Mic3_mode == AUDIO_ANALOGUL_MODE_DMIC) {
			TurnOnADcPowerDmic(AUDIO_ANALOG_DEVICE_IN_ADC3, false);
		}
	}
	return 0;
}

static int Audio_ADC4_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("Audio_ADC4_Get = %d\n",
		 mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC4]);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC4];
	return 0;
}

static int Audio_ADC4_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()\n", __func__);
	if (ucontrol->value.integer.value[0]) {
		if (mAudio_Analog_Mic4_mode == AUDIO_ANALOGUL_MODE_ACC) {
			TurnOnADcPowerACC(AUDIO_ANALOG_DEVICE_IN_ADC4, true);
		} else if (mAudio_Analog_Mic4_mode == AUDIO_ANALOGUL_MODE_DCC) {
			TurnOnADcPowerDCC(AUDIO_ANALOG_DEVICE_IN_ADC4, true);
		} else if (mAudio_Analog_Mic4_mode == AUDIO_ANALOGUL_MODE_DMIC) {
			TurnOnADcPowerDmic(AUDIO_ANALOG_DEVICE_IN_ADC4, true);
		}
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC4] =
		    ucontrol->value.integer.value[0];
	} else {
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC4] =
		    ucontrol->value.integer.value[0];
		if (mAudio_Analog_Mic4_mode == AUDIO_ANALOGUL_MODE_ACC) {
			TurnOnADcPowerACC(AUDIO_ANALOG_DEVICE_IN_ADC4, false);
		} else if (mAudio_Analog_Mic4_mode == AUDIO_ANALOGUL_MODE_DCC) {
			TurnOnADcPowerDCC(AUDIO_ANALOG_DEVICE_IN_ADC4, false);
		} else if (mAudio_Analog_Mic4_mode == AUDIO_ANALOGUL_MODE_DMIC) {
			TurnOnADcPowerDmic(AUDIO_ANALOG_DEVICE_IN_ADC4, false);
		}
	}
	return 0;
}

static int Audio_ADC1_Sel_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s() = %d\n", __func__, mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC1]);
	ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC1];
	return 0;
}

static int Audio_ADC1_Sel_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()\n", __func__);

	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Adc_Input_Sel)) {
		pr_debug("return -EINVAL\n");
		return -EINVAL;
	}
	if (ucontrol->value.integer.value[0] == 0) {
		Ana_Set_Reg(AUDPREAMP_CFG1, (0x0000 << 1), 0x0006);	/* pinumx sel */
	} else if (ucontrol->value.integer.value[0] == 1) {
		Ana_Set_Reg(AUDPREAMP_CFG1, (0x0001 << 1), 0x0006);
	}
	/* ADC2 */
	else if (ucontrol->value.integer.value[0] == 2) {
		Ana_Set_Reg(AUDPREAMP_CFG1, (0x0002 << 1), 0x0006);
	} else {
		pr_debug("%s() warning\n ", __func__);
	}
	pr_debug("%s() done\n", __func__);
	mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC1] = ucontrol->value.integer.value[0];
	return 0;
}

static int Audio_ADC2_Sel_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s() = %d\n", __func__, mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC2]);
	ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC2];
	return 0;
}

static int Audio_ADC2_Sel_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()\n", __func__);

	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Adc_Input_Sel)) {
		pr_debug("return -EINVAL\n");
		return -EINVAL;
	}
	if (ucontrol->value.integer.value[0] == 0) {
		Ana_Set_Reg(AUDPREAMP_CFG1, (0x0000 << 5), 0x0060);	/* pinumx sel */
	} else if (ucontrol->value.integer.value[0] == 1) {
		Ana_Set_Reg(AUDPREAMP_CFG1, (0x0001 << 5), 0x0060);
	}
	/* ADC2 */
	else if (ucontrol->value.integer.value[0] == 2) {
		Ana_Set_Reg(AUDPREAMP_CFG1, (0x0002 << 5), 0x0060);
	} else {
		pr_debug("%s() warning\n ", __func__);
	}
	pr_debug("%s() done\n", __func__);
	mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC2] = ucontrol->value.integer.value[0];
	return 0;
}

static int Audio_ADC3_Sel_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s() = %d\n", __func__, mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC3]);
	ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC3];
	return 0;
}

static int Audio_ADC3_Sel_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()\n", __func__);

	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Adc_Input_Sel)) {
		pr_debug("return -EINVAL\n");
		return -EINVAL;
	}

	if (ucontrol->value.integer.value[0] == 0) {
		Ana_Set_Reg(AUDPREAMP_CFG1, (0x0000 << 8), 0x0300);	/* pinumx sel */
	} else if (ucontrol->value.integer.value[0] == 1) {
		Ana_Set_Reg(AUDPREAMP_CFG1, (0x0001 << 8), 0x0300);
	} else if (ucontrol->value.integer.value[0] == 2) {
		Ana_Set_Reg(AUDPREAMP_CFG1, (0x0002 << 8), 0x0300);
	} else {
		pr_debug("%s() warning\n ", __func__);
	}
	pr_debug("%s() done\n", __func__);
	mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC3] = ucontrol->value.integer.value[0];
	return 0;
}


static int Audio_ADC4_Sel_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s() = %d\n", __func__, mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC4]);
	ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC4];
	return 0;
}

static int Audio_ADC4_Sel_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{

	pr_debug("%s()\n", __func__);

	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Adc_Input_Sel)) {
		pr_debug("return -EINVAL\n");
		return -EINVAL;
	}

	if (ucontrol->value.integer.value[0] == 0) {
		Ana_Set_Reg(AUDPREAMP_CFG1, (0x0000 << 12), 0x1800);	/* pinumx sel */
	} else if (ucontrol->value.integer.value[0] == 1) {
		Ana_Set_Reg(AUDPREAMP_CFG1, (0x0001 << 12), 0x1800);
	} else if (ucontrol->value.integer.value[0] == 2) {
		Ana_Set_Reg(AUDPREAMP_CFG1, (0x0002 << 12), 0x1800);
	} else {
		pr_debug("%s() warning\n ", __func__);
	}
	pr_debug("%s() done\n", __func__);
	mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC4] = ucontrol->value.integer.value[0];
	return 0;
}


static bool AudioPreAmp1_Sel(int Mul_Sel)
{
	pr_debug("%s Mul_Sel = %d ", __func__, Mul_Sel);
	if (Mul_Sel == 0) {
		Ana_Set_Reg(AUDPREAMP_CFG0, 0x0000, 0x0030);	/* pinumx open */
	} else if (Mul_Sel == 1) {
		Ana_Set_Reg(AUDPREAMP_CFG0, 0x0010, 0x0030);	/* ADC 0 */
	}
	/* ADC2 */
	else if (Mul_Sel == 2) {
		Ana_Set_Reg(AUDPREAMP_CFG0, 0x0020, 0x0030);	/* ADC 1 */
	} else {
		pr_debug("AnalogSetMux warning");
	}

	return true;
}


static int Audio_PreAmp1_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s() mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_PREAMP_1]; = %d\n", __func__,
		 mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_PREAMP_1]);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_PREAMP_1];
	return 0;
}

static int Audio_PreAmp1_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()\n", __func__);

	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(PreAmp_Mux_function)) {
		pr_debug("return -EINVAL\n");
		return -EINVAL;
	}
	mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_PREAMP_1] =
	    ucontrol->value.integer.value[0];
	AudioPreAmp1_Sel(mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_PREAMP_1]);
	pr_debug("%s() done\n", __func__);
	return 0;
}

static int Audio_PGA1_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("Audio_AmpR_Get = %d\n",
		 mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP1]);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP1];
	return 0;
}

static int Audio_PGA1_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int index = 0;
	pr_debug("%s()\n", __func__);
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(ADC_UL_PGA_GAIN)) {
		pr_debug("return -EINVAL\n");
		return -EINVAL;
	}
	index = ucontrol->value.integer.value[0];
	Ana_Set_Reg(AUDPREAMPGAIN_CFG0, index, 0x0007);
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP1] =
	    ucontrol->value.integer.value[0];
	return 0;
}


static int Audio_PGA2_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("Audio_PGA2_Get = %d\n",
		 mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP2]);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP2];
	return 0;
}

static int Audio_PGA2_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int index = 0;
	pr_debug("%s()\n", __func__);
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(ADC_UL_PGA_GAIN)) {
		pr_debug("return -EINVAL\n");
		return -EINVAL;
	}
	index = ucontrol->value.integer.value[0];
	Ana_Set_Reg(AUDPREAMPGAIN_CFG0, index << 4, 0x0070);
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP2] =
	    ucontrol->value.integer.value[0];
	return 0;
}


static int Audio_PGA3_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("Audio_AmpR_Get = %d\n",
		 mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP3]);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP3];
	return 0;
}

static int Audio_PGA3_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int index = 0;
	pr_debug("%s()\n", __func__);
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(ADC_UL_PGA_GAIN)) {
		pr_debug("return -EINVAL\n");
		return -EINVAL;
	}
	index = ucontrol->value.integer.value[0];
	Ana_Set_Reg(AUDPREAMPGAIN_CFG0, index << 8, 0x0700);
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP3] =
	    ucontrol->value.integer.value[0];
	return 0;
}

static int Audio_PGA4_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("Audio_AmpR_Get = %d\n",
		 mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP4]);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP4];
	return 0;
}

static int Audio_PGA4_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int index = 0;
	pr_debug("%s()\n", __func__);
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(ADC_UL_PGA_GAIN)) {
		pr_debug("return -EINVAL\n");
		return -EINVAL;
	}
	index = ucontrol->value.integer.value[0];
	Ana_Set_Reg(AUDPREAMPGAIN_CFG0, index << 12, 0x7000);
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP4] =
	    ucontrol->value.integer.value[0];
	return 0;
}

static int Audio_MicSource1_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("Audio_MicSource1_Get = %d\n",
		 mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1]);
	ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1];
	return 0;
}

static int Audio_MicSource1_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int index = 0;
	pr_debug("%s()\n", __func__);
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Pmic_Digital_Mux)) {
		pr_debug("return -EINVAL\n");
		return -EINVAL;
	}
	index = ucontrol->value.integer.value[0];
	pr_debug("%s() index = %d done\n", __func__, index);
	Ana_Set_Reg(AFE_MIC_ARRAY_CFG, index | index << 8, 0x0303);
	mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] = ucontrol->value.integer.value[0];
	return 0;
}

static int Audio_MicSource2_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s() = %d\n", __func__, mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_2]);
	ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_2];
	return 0;
}

static int Audio_MicSource2_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int index = 0;
	pr_debug("%s()\n", __func__);
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Pmic_Digital_Mux)) {
		pr_debug("return -EINVAL\n");
		return -EINVAL;
	}
	index = ucontrol->value.integer.value[0];
	pr_debug("%s() done\n", __func__);
	Ana_Set_Reg(AFE_MIC_ARRAY_CFG, index << 2 | index << 10, 0x0c0c);
	mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_2] = ucontrol->value.integer.value[0];
	return 0;
}

static int Audio_MicSource3_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s() = %d\n", __func__, mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_3]);
	ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_3];
	return 0;
}

static int Audio_MicSource3_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int index = 0;
	pr_debug("%s()\n", __func__);
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Pmic_Digital_Mux)) {
		pr_debug("return -EINVAL\n");
		return -EINVAL;
	}
	index = ucontrol->value.integer.value[0];
	pr_debug("%s() done\n", __func__);
	Ana_Set_Reg(AFE_MIC_ARRAY_CFG, index << 4 | index << 12, 0x3030);
	mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_3] = ucontrol->value.integer.value[0];
	return 0;
}


static int Audio_MicSource4_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s() = %d\n", __func__, mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_4]);
	ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_4];
	return 0;
}

static int Audio_MicSource4_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int index = 0;
	pr_debug("%s()\n", __func__);
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Pmic_Digital_Mux)) {
		pr_debug("return -EINVAL\n");
		return -EINVAL;
	}
	index = ucontrol->value.integer.value[0];
	pr_debug("%s() done\n", __func__);
	Ana_Set_Reg(AFE_MIC_ARRAY_CFG, index << 6 | index << 14, 0xc0c0);
	mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_4] = ucontrol->value.integer.value[0];
	return 0;
}

/* Mic ACC/DCC Mode Setting */
static int Audio_Mic1_Mode_Select_Get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s() mAudio_Analog_Mic1_mode = %d\n", __func__, mAudio_Analog_Mic1_mode);
	ucontrol->value.integer.value[0] = mAudio_Analog_Mic1_mode;
	return 0;
}

static int Audio_Mic1_Mode_Select_Set(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()\n", __func__);
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Audio_AnalogMic_Mode)) {
		pr_debug("return -EINVAL\n");
		return -EINVAL;
	}
	mAudio_Analog_Mic1_mode = ucontrol->value.integer.value[0];
	pr_debug("%s() mAudio_Analog_Mic1_mode = %d\n", __func__, mAudio_Analog_Mic1_mode);
	return 0;
}

static int Audio_Mic2_Mode_Select_Get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()  = %d\n", __func__, mAudio_Analog_Mic2_mode);
	ucontrol->value.integer.value[0] = mAudio_Analog_Mic2_mode;
	return 0;
}

static int Audio_Mic2_Mode_Select_Set(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()\n", __func__);
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Audio_AnalogMic_Mode)) {
		pr_debug("return -EINVAL\n");
		return -EINVAL;
	}
	mAudio_Analog_Mic2_mode = ucontrol->value.integer.value[0];
	pr_debug("%s() mAudio_Analog_Mic1_mode = %d\n", __func__, mAudio_Analog_Mic2_mode);
	return 0;
}


static int Audio_Mic3_Mode_Select_Get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()  = %d\n", __func__, mAudio_Analog_Mic3_mode);
	ucontrol->value.integer.value[0] = mAudio_Analog_Mic3_mode;
	return 0;
}

static int Audio_Mic3_Mode_Select_Set(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()\n", __func__);
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Audio_AnalogMic_Mode)) {
		pr_debug("return -EINVAL\n");
		return -EINVAL;
	}
	mAudio_Analog_Mic3_mode = ucontrol->value.integer.value[0];
	pr_debug("%s() mAudio_Analog_Mic1_mode = %d\n", __func__, mAudio_Analog_Mic3_mode);
	return 0;
}

static int Audio_Mic4_Mode_Select_Get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()  = %d\n", __func__, mAudio_Analog_Mic4_mode);
	ucontrol->value.integer.value[0] = mAudio_Analog_Mic4_mode;
	return 0;
}

static int Audio_Mic4_Mode_Select_Set(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()\n", __func__);
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Audio_AnalogMic_Mode)) {
		pr_debug("return -EINVAL\n");
		return -EINVAL;
	}
	mAudio_Analog_Mic4_mode = ucontrol->value.integer.value[0];
	pr_debug("%s() mAudio_Analog_Mic1_mode = %d\n", __func__, mAudio_Analog_Mic4_mode);
	return 0;
}

static bool SineTable_DAC_HP_flag;
static bool SineTable_UL2_flag;

static int SineTable_UL2_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	if (ucontrol->value.integer.value[0]) {
		Ana_Set_Reg(PMIC_AFE_TOP_CON0, 0x0002, 0x2);	/* set DL sine gen table */
		Ana_Set_Reg(AFE_SGEN_CFG0, 0x0080, 0xffffffff);
		Ana_Set_Reg(AFE_SGEN_CFG1, 0x0101, 0xffffffff);
	} else {
		Ana_Set_Reg(PMIC_AFE_TOP_CON0, 0x0002, 0x2);	/* set DL sine gen table */
		Ana_Set_Reg(AFE_SGEN_CFG0, 0x0000, 0xffffffff);
		Ana_Set_Reg(AFE_SGEN_CFG1, 0x0101, 0xffffffff);
	}
	return 0;
}

static int SineTable_UL2_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()\n", __func__);
	ucontrol->value.integer.value[0] = SineTable_UL2_flag;
	return 0;
}

static int SineTable_DAC_HP_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()\n", __func__);
	ucontrol->value.integer.value[0] = SineTable_DAC_HP_flag;
	return 0;
}

static int SineTable_DAC_HP_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()\n", __func__);
	if (ucontrol->value.integer.value[0]) {
		SineTable_DAC_HP_flag = ucontrol->value.integer.value[0];
		pr_debug("TurnOnDacPower\n");
		ClsqEnable(true);
		Topck_Enable(true);
		Ana_Set_Reg(AFUNC_AUD_CON2, 0x0006, 0xffffffff);
		Ana_Set_Reg(AFUNC_AUD_CON0, 0xc3a1, 0xffffffff);	/* sdm audio fifo clock power on */
		Ana_Set_Reg(AFUNC_AUD_CON2, 0x0003, 0xffffffff);	/* sdm power on */
		Ana_Set_Reg(AFUNC_AUD_CON2, 0x000b, 0xffffffff);	/* sdm fifo enable */
		Ana_Set_Reg(AFE_DL_SDM_CON1, 0x001e, 0xffffffff);	/* set attenuation gain */
		Ana_Set_Reg(AFE_UL_DL_CON0, 0x0001, 0xffffffff);	/* [0] afe enable */

		Ana_Set_Reg(AFE_PMIC_NEWIF_CFG0, 0x8330, 0xffffffff);
		Ana_Set_Reg(AFE_DL_SRC2_CON0_H, 0x8330, 0xffff000f);

		Ana_Set_Reg(AFE_DL_SRC2_CON0_L, 0x1801, 0xffffffff);	/* turn off mute function and turn on dl */
		Ana_Set_Reg(PMIC_AFE_TOP_CON0, 0x0001, 0xffffffff);	/* set DL  sine gen table */
		Ana_Set_Reg(AFE_SGEN_CFG0, 0x0080, 0xffffffff);
		Ana_Set_Reg(AFE_SGEN_CFG1, 0x0101, 0xffffffff);

		Ana_Set_Reg(0x0680, 0x0000, 0xffff);	/* Enable AUDGLB */
		OpenClassAB();
		Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0028, 0xffff);	/* Enable cap-less LDOs (1.6V) */
		Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0068, 0xffff);	/* Enable NV regulator (-1.6V) */
		Ana_Set_Reg(ZCD_CON0, 0x0700, 0xffff);	/* Disable AUD_ZCD */
		Ana_Set_Reg(AUDBUF_CFG0, 0xE008, 0xffff);	/* Disable headphone, voice and short-ckt protection. */
		Ana_Set_Reg(IBIASDIST_CFG0, 0x0092, 0xffff);	/* Enable IBIST */
		Ana_Set_Reg(ZCD_CON2, 0x0F9F, 0xffff);	/* Set HPR/HPL gain as minimum (~ -40dB) */
		Ana_Set_Reg(ZCD_CON3, 0x001F, 0xffff);	/* Set voice gain as minimum (~ -40dB) */
		Ana_Set_Reg(AUDBUF_CFG1, 0x0900, 0xffff);	/* De_OSC of HP and enable output STBENH */
		Ana_Set_Reg(AUDBUF_CFG2, 0x0022, 0xffff);	/* De_OSC of voice, enable output STBENH */
		Ana_Set_Reg(AUDBUF_CFG0, 0xE009, 0xffff);	/* Enable voice driver */
		Ana_Set_Reg(AUDBUF_CFG1, 0x0940, 0xffff);	/* Enable pre-charge buffer */
		msleep(1);
		Ana_Set_Reg(AUDCLKGEN_CFG0, 0x5501, 0xffff);	/* Enable AUD_CLK */
		Ana_Set_Reg(AUDDAC_CFG0, 0x000f, 0xffff);	/* Enable Audio DAC */
		SetDcCompenSation();

		Ana_Set_Reg(AUDBUF_CFG0, 0xE149, 0xffff);	/* Switch HP MUX to audio DAC */
		Ana_Set_Reg(AUDBUF_CFG0, 0xE14F, 0xffff);	/* Enable HPR/HPL */
		Ana_Set_Reg(AUDBUF_CFG1, 0x0900, 0xffff);	/* Disable pre-charge buffer */
		Ana_Set_Reg(AUDBUF_CFG2, 0x0020, 0xffff);	/* Disable De_OSC of voice */
		Ana_Set_Reg(AUDBUF_CFG0, 0xE14E, 0xffff);	/* Disable voice buffer */
		Ana_Set_Reg(ZCD_CON2, 0x0489, 0xffff);	/* Set HPR/HPL gain as 0dB, step by step */

	} else {
		SineTable_DAC_HP_flag = ucontrol->value.integer.value[0];
		if (GetDLStatus() == false) {
			Ana_Set_Reg(AUDBUF_CFG0, 0xE149, 0xffff);	/* Disable HPR/HPL */
			Ana_Set_Reg(AUDDAC_CFG0, 0x0000, 0xffff);	/* Disable Audio DAC */
			Ana_Set_Reg(AUDCLKGEN_CFG0, 0x5500, 0xffff);	/* Disable AUD_CLK */
			Ana_Set_Reg(IBIASDIST_CFG0, 0x0192, 0xffff);	/* Disable IBIST */
			Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0028, 0xffff);	/* Disable NV regulator (-1.6V) */
			Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0000, 0xffff);	/* Disable cap-less LDOs (1.6V) */
			Ana_Set_Reg(AFE_CLASSH_CFG0, 0xd518, 0xffff);	/* ClassH off */
			Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0518, 0xffff);	/* NCP offset */
			Ana_Set_Reg(PMIC_AFE_TOP_CON0, 0x0000, 0xffffffff);	/* set DL normal */
		}
	}
	return 0;
}

static void ADC_LOOP_DAC_Func(int command)
{
	if (command == AUDIO_ANALOG_DAC_LOOP_DAC_HS_ON
	    || command == AUDIO_ANALOG_DAC_LOOP_DAC_HP_ON) {
		ClsqEnable(true);
		Topck_Enable(true);
		NvregEnable(true);
		Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0000, 0xffff);	/* power on clock */
		Ana_Set_Reg(AUDADC_CFG0, 0x0400, 0xffff);	/* Enable ADC CLK */

		/* Ana_Set_Reg(AUDMICBIAS_CFG0, 0x78F, 0xffff);   //Enable MICBIAS0,1 (2.7V) */
		OpenMicbias1();
		OpenMicbias0();

		Ana_Set_Reg(AUDMICBIAS_CFG1, 0x78F, 0xffff);	/* Enable MICBIAS2,3 (2.7V) */
		Ana_Set_Reg(AUDLDO_NVREG_CFG1, 0x0007, 0xffff);	/* Enable LCLDO18_ENC (1.8V), Remote-Sense */
		Ana_Set_Reg(AUDLDO_NVREG_CFG2, 0x2277, 0xffff);	/* Enable LCLDO19_ADCCH0_1, Remote-Sense */
		Ana_Set_Reg(AUDPREAMPGAIN_CFG0, 0x0022, 0xffff);	/* Set PGA CH0_1 gain = 12dB */
		SetMicPGAGain();
		Ana_Set_Reg(AUDPREAMP_CFG0, 0x0051, 0xffff);	/* Enable PGA CH0_1 (CH0 in) */
		Ana_Set_Reg(AUDPREAMP_CFG1, 0x0055, 0xffff);	/* Enable ADC CH0_1 (PGA in) */

		Ana_Set_Reg(AFE_ADDA2_UL_SRC_CON1_L, 0x0000, 0xffffffff);	/* power on ADC clk */
		Ana_Set_Reg(AFE_TOP_CON0, 0x4000, 0xffffffff);	/* AFE[14] loopback test1 ( UL tx sdata to DL rx) */
		Ana_Set_Reg(AFUNC_AUD_CON2, 0x0006, 0xffffffff);
		Ana_Set_Reg(AFUNC_AUD_CON0, 0xc3a1, 0xffffffff);	/* sdm audio fifo clock power on */
		Ana_Set_Reg(AFUNC_AUD_CON2, 0x0003, 0xffffffff);	/* sdm power on */
		Ana_Set_Reg(AFUNC_AUD_CON2, 0x000b, 0xffffffff);	/* sdm fifo enable */
		Ana_Set_Reg(AFE_DL_SDM_CON1, 0x001e, 0xffffffff);	/* set attenuation gain */
		Ana_Set_Reg(AFE_UL_DL_CON0, 0x0001, 0xffffffff);	/* [0] afe enable */

		Ana_Set_Reg(AFE_UL_SRC0_CON0_H, 0x0000, 0x0010);	/* UL1 */

		Ana_Set_Reg(AFE_UL_SRC0_CON0_L, 0x0001, 0xffff);	/* power on uplink */
		Ana_Set_Reg(AFE_PMIC_NEWIF_CFG0, 0x0380, 0xffff);	/* MTKIF */
		Ana_Set_Reg(AFE_DL_SRC2_CON0_H, 0x0800, 0xffff);	/* DL */
		Ana_Set_Reg(AFE_DL_SRC2_CON0_L, 0x0001, 0xffff);	/* DL */

		/* here to start analog part */
		/* Ana_Set_Reg(AUDNVREGGLB_CFG0, 0x0000, 0xffff); //Enable AUDGLB */
		OpenClassAB();

		Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0028, 0xffff);	/* Enable cap-less LDOs (1.6V) */
		Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0068, 0xffff);	/* Enable NV regulator (-1.6V) */
		Ana_Set_Reg(ZCD_CON0, 0x0700, 0xffff);	/* Disable AUD_ZCD */
		Ana_Set_Reg(AUDBUF_CFG0, 0xE008, 0xffff);	/* Disable headphone, voice and short-ckt protection. */
		Ana_Set_Reg(IBIASDIST_CFG0, 0x0092, 0xffff);	/* Enable IBIST */
		if (command == AUDIO_ANALOG_DAC_LOOP_DAC_HS_ON) {
			Ana_Set_Reg(ZCD_CON3, 0x001f, 0xffff);	/* Set voice gain as minimum (~ -40dB) */
			Ana_Set_Reg(AUDBUF_CFG2, 0x0022, 0xffff);	/* De_OSC of voice, enable output STBENH */
			Ana_Set_Reg(AUDCLKGEN_CFG0, 0x5501, 0xffff);	/* Enable AUD_CLK */
			Ana_Set_Reg(AUDDAC_CFG0, 0x0009, 0xffff);	/* Enable Audio DAC */
			SetDcCompenSation();

			Ana_Set_Reg(AUDBUF_CFG0, 0xE010, 0xffff);	/* Switch HP MUX to audio DAC */
			Ana_Set_Reg(AUDBUF_CFG0, 0xE011, 0xffff);	/* Enable HPR/HPL */
			Ana_Set_Reg(ZCD_CON3, 0x0009, 0xffff);	/* Set HPR/HPL gain as 0dB, step by step */
		} else if (command == AUDIO_ANALOG_DAC_LOOP_DAC_HP_ON) {
			Ana_Set_Reg(ZCD_CON2, 0x0F9F, 0xffff);	/* Set HPR/HPL gain as minimum (~ -40dB) */
			Ana_Set_Reg(ZCD_CON3, 0x001f, 0xffff);	/* Set voice gain as minimum (~ -40dB) */
			Ana_Set_Reg(AUDBUF_CFG1, 0x0900, 0xffff);	/* De_OSC of HP and enable output STBENH */
			Ana_Set_Reg(AUDBUF_CFG2, 0x0022, 0xffff);	/* De_OSC of voice, enable output STBENH */
			Ana_Set_Reg(AUDBUF_CFG0, 0xE009, 0xffff);	/* Enable voice driver */
			Ana_Set_Reg(AUDBUF_CFG1, 0x0940, 0xffff);	/* De_OSC of HP and enable output STBENH */
			Ana_Set_Reg(AUDCLKGEN_CFG0, 0x5501, 0xffff);	/* Enable AUD_CLK */
			Ana_Set_Reg(AUDDAC_CFG0, 0x000F, 0xffff);	/* Enable Audio DAC */
			SetDcCompenSation();

			Ana_Set_Reg(AUDBUF_CFG0, 0xE149, 0xffff);	/* Switch HP MUX to audio DAC */
			Ana_Set_Reg(AUDBUF_CFG0, 0xE14F, 0xffff);	/* Enable HPR/HPL */
			Ana_Set_Reg(AUDBUF_CFG1, 0x0900, 0xffff);	/* Enable HPR/HPL */
			Ana_Set_Reg(AUDBUF_CFG2, 0x0020, 0xffff);	/* Enable HPR/HPL */
			Ana_Set_Reg(AUDBUF_CFG0, 0xE14E, 0xffff);	/* Enable HPR/HPL */
			Ana_Set_Reg(ZCD_CON2, 0x0489, 0xffff);	/* Set HPR/HPL gain as 0dB, step by step */
		}
	} else {
		if (command == AUDIO_ANALOG_DAC_LOOP_DAC_HS_ON) {
			Ana_Set_Reg(AUDBUF_CFG0, 0xe010, 0xffff);	/* Disable voice driver */
			Ana_Set_Reg(AUDDAC_CFG0, 0x0000, 0xffff);	/* Disable L-ch Audio DAC */
		} else if (command == AUDIO_ANALOG_DAC_LOOP_DAC_HP_ON) {
			Ana_Set_Reg(AUDBUF_CFG0, 0xE149, 0xffff);	/* Disable voice DRIVERMODE_CODEC_ONLY */
			Ana_Set_Reg(AUDDAC_CFG0, 0x0000, 0xffff);	/* Disable L-ch Audio DAC */
		}
		Ana_Set_Reg(AUDCLKGEN_CFG0, 0x5500, 0xffff);	/* Disable AUD_CLK */
		Ana_Set_Reg(IBIASDIST_CFG0, 0x0192, 0xffff);	/* Disable IBIST */
		Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0028, 0xffff);	/* Disable NV regulator (-1.6V) */
		Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0000, 0xffff);	/* Disable cap-less LDOs (1.6V) */
		Ana_Set_Reg(AFE_CLASSH_CFG0, 0xd518, 0xffff);	/* ClassH offset */
		Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0518, 0xffff);	/* NCP offset */
	}
}

static bool DAC_LOOP_DAC_HS_flag;
static int ADC_LOOP_DAC_HS_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()\n", __func__);
	ucontrol->value.integer.value[0] = DAC_LOOP_DAC_HS_flag;
	return 0;
}

static int ADC_LOOP_DAC_HS_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()\n", __func__);
	if (ucontrol->value.integer.value[0]) {
		DAC_LOOP_DAC_HS_flag = ucontrol->value.integer.value[0];
		ADC_LOOP_DAC_Func(AUDIO_ANALOG_DAC_LOOP_DAC_HS_ON);
	} else {
		DAC_LOOP_DAC_HS_flag = ucontrol->value.integer.value[0];
		ADC_LOOP_DAC_Func(AUDIO_ANALOG_DAC_LOOP_DAC_HS_OFF);
	}
	return 0;
}

static bool DAC_LOOP_DAC_HP_flag;
static int ADC_LOOP_DAC_HP_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()\n", __func__);
	ucontrol->value.integer.value[0] = DAC_LOOP_DAC_HP_flag;
	return 0;
}

static int ADC_LOOP_DAC_HP_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{

	pr_debug("%s()\n", __func__);
	if (ucontrol->value.integer.value[0]) {
		DAC_LOOP_DAC_HP_flag = ucontrol->value.integer.value[0];
		ADC_LOOP_DAC_Func(AUDIO_ANALOG_DAC_LOOP_DAC_HP_ON);
	} else {
		DAC_LOOP_DAC_HP_flag = ucontrol->value.integer.value[0];
		ADC_LOOP_DAC_Func(AUDIO_ANALOG_DAC_LOOP_DAC_HP_OFF);
	}
	return 0;
}

static bool Voice_Call_DAC_DAC_HS_flag;
static int Voice_Call_DAC_DAC_HS_Get(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()\n", __func__);
	ucontrol->value.integer.value[0] = Voice_Call_DAC_DAC_HS_flag;
	return 0;
}

static int Voice_Call_DAC_DAC_HS_Set(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()\n", __func__);
	if (ucontrol->value.integer.value[0]) {
		Voice_Call_DAC_DAC_HS_flag = ucontrol->value.integer.value[0];
		/* here to set voice call 16L setting... */
		Ana_Set_Reg(AUDNVREGGLB_CFG0, 0x0000, 0xffff);	/* RG_AUDGLB_PWRDN_VA32 = 1'b0 */
		Ana_Set_Reg(TOP_CLKSQ, 0x0001, 0xffff);	/* CKSQ Enable */
		Ana_Set_Reg(AUDADC_CFG0, 0x0400, 0xffff);	/* Enable ADC CLK26CALI */
		/* Ana_Set_Reg(AUDMICBIAS_CFG0,  0x78f, 0xffff); //  Enable MICBIAS0 (2.7V) */
		OpenMicbias1();
		OpenMicbias0();

		Ana_Set_Reg(AUDMICBIAS_CFG1, 0x78f, 0xffff);	/* Enable MICBIAS2 (2.7V) */
		Ana_Set_Reg(AUDLDO_NVREG_CFG1, 0x0007, 0xffff);	/* Enable LCLDO18_ENC (1.8V), Remote-Sense ; Set LCLDO19_ADC voltage 1.9V */
		Ana_Set_Reg(AUDLDO_NVREG_CFG2, 0x2277, 0xffff);	/* Enable LCLDO19_ADCCH0_1, Remote-Sense ; Enable LCLDO19_ADCCH_2, Remote-Sense */
		Ana_Set_Reg(AUDPREAMPGAIN_CFG0, 0x033, 0xffff);	/* Set PGA CH0_1 gain = 18dB ; Set PGA CH_2 gain = 18dB */
		SetMicPGAGain();
		Ana_Set_Reg(AUDPREAMP_CFG0, 0x051, 0xffff);	/* Enable PGA CH0_1 (CH0 in) ; Enable PGA CH_2 */
		Ana_Set_Reg(AUDPREAMP_CFG1, 0x055, 0xffff);	/* Enable ADC CH0_1 (PGA in) ; Enable ADC CH_2 (PGA in) */

		Ana_Set_Reg(TOP_CLKSQ_SET, 0x0003, 0xffff);	/* CKSQ Enable */
		Ana_Set_Reg(TOP_CKPDN_CON0_CLR, 0x3000, 0xffff);	/* AUD clock power down released */
		Ana_Set_Reg(TOP_CKSEL_CON_CLR, 0x0001, 0x0001);	/* use internal 26M */

		Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0000, 0xffff);	/* power on clock */

		Ana_Set_Reg(AFE_ADDA2_UL_SRC_CON1_L, 0x0000, 0xffff);	/* power on ADC clk */
		Ana_Set_Reg(AFUNC_AUD_CON2, 0x0006, 0xffff);	/* sdm audio fifo clock power on */
		Ana_Set_Reg(AFUNC_AUD_CON0, 0xc3a1, 0xffff);	/* scrambler clock on enable */
		Ana_Set_Reg(AFUNC_AUD_CON2, 0x0003, 0xffff);	/* sdm power on */
		Ana_Set_Reg(AFUNC_AUD_CON2, 0x000b, 0xffff);	/* sdm fifo enable */
		Ana_Set_Reg(AFE_DL_SDM_CON1, 0x001e, 0xffff);	/* set attenuation gain */
		Ana_Set_Reg(AFE_UL_DL_CON0, 0x0001, 0xffff);	/* afe enable */
		Ana_Set_Reg(AFE_PMIC_NEWIF_CFG0, 0x3330, 0xffff);	/* time slot1= 47, time slot2=24 @ 384K interval. */
		Ana_Set_Reg(AFE_DL_SRC2_CON0_H, 0x3330, 0xffff);	/* 16k samplerate */
		Ana_Set_Reg(AFE_DL_SRC2_CON0_L, 0x1801, 0xffff);	/* turn off mute function and turn on dl */
		Ana_Set_Reg(AFE_UL_SRC0_CON0_H, 0x000a, 0xffff);	/* UL1 */
		Ana_Set_Reg(AFE_UL_SRC0_CON0_L, 0x0001, 0xffff);	/* power on uplink */

		/* ============sine gen table============ */
		Ana_Set_Reg(PMIC_AFE_TOP_CON0, 0x0000, 0xffff);	/* no loopback */
		Ana_Set_Reg(AFE_SGEN_CFG0, 0x0080, 0xffff);	/* L/R-ch @ sample rate = 8*8K for tone = 0dB of 1K Hz example. */
		Ana_Set_Reg(AFE_SGEN_CFG1, 0x0101, 0xffff);	/* L/R-ch @ sample rate = 8*8K for tone = 0dB of 1K Hz example. */

		/* ======================here set analog part (audio HP playback)========================= */
		Ana_Set_Reg(AUDNVREGGLB_CFG0, 0x0000, 0xffff);	/* [0] RG_AUDGLB_PWRDN_VA32 = 1'b0 */

		Ana_Set_Reg(AFE_CLASSH_CFG7, 0x8909, 0xffff);	/* Classh CK fix 591KHz */
		Ana_Set_Reg(AFE_CLASSH_CFG8, 0x0d0d, 0xffff);	/* Classh CK fix 591KHz */
		Ana_Set_Reg(AFE_CLASSH_CFG9, 0x0d10, 0xffff);	/* Classh CK fix 591KHz */
		Ana_Set_Reg(AFE_CLASSH_CFG10, 0x1010, 0xffff);	/* Classh CK fix 591KHz */
		Ana_Set_Reg(AFE_CLASSH_CFG11, 0x1010, 0xffff);	/* Classh CK fix 591KHz */
		Ana_Set_Reg(AFE_CLASSH_CFG12, 0x0000, 0xffff);	/* Classh CK fix 591KHz */
		Ana_Set_Reg(AFE_CLASSH_CFG13, 0x0000, 0xffff);	/* Classh CK fix 591KHz */
		Ana_Set_Reg(AFE_CLASSH_CFG14, 0x009c, 0xffff);	/* Classh CK fix 591KHz */
		Ana_Set_Reg(AFE_CLASSH_CFG26, 0x8d0d, 0xffff);	/* Classh CK fix 591KHz */
		Ana_Set_Reg(AFE_CLASSH_CFG27, 0x0d0d, 0xffff);	/* Classh CK fix 591KHz */
		Ana_Set_Reg(AFE_CLASSH_CFG28, 0x0d10, 0xffff);	/* Classh CK fix 591KHz */
		Ana_Set_Reg(AFE_CLASSH_CFG29, 0x1010, 0xffff);	/* Classh CK fix 591KHz */
		Ana_Set_Reg(AFE_CLASSH_CFG30, 0x1010, 0xffff);	/* Classh CK fix 591KHz */
		Ana_Set_Reg(AFE_CLASSH_CFG1, 0x0024, 0xffff);	/* Classh CK fix 591KHz */
		Ana_Set_Reg(AFE_CLASSH_CFG2, 0x2f90, 0xffff);	/* Classh CK fix 591KHz */
		Ana_Set_Reg(AFE_CLASSH_CFG3, 0x1104, 0xffff);	/* Classh CK fix 591KHz */
		Ana_Set_Reg(AFE_CLASSH_CFG4, 0x2412, 0xffff);	/* Classh CK fix 591KHz */
		Ana_Set_Reg(AFE_CLASSH_CFG5, 0x0201, 0xffff);	/* Classh CK fix 591KHz */
		Ana_Set_Reg(AFE_CLASSH_CFG6, 0x2800, 0xffff);	/* Classh CK fix 591KHz */
		Ana_Set_Reg(AFE_CLASSH_CFG21, 0xa108, 0xffff);	/* Classh CK fix 591KHz */
		Ana_Set_Reg(AFE_CLASSH_CFG22, 0x06db, 0xffff);	/* Classh CK fix 591KHz */
		Ana_Set_Reg(AFE_CLASSH_CFG23, 0x0bd6, 0xffff);	/* Classh CK fix 591KHz */
		Ana_Set_Reg(AFE_CLASSH_CFG24, 0x1492, 0xffff);	/* Classh CK fix 591KHz */
		Ana_Set_Reg(AFE_CLASSH_CFG25, 0x1740, 0xffff);	/* Classh CK fix 591KHz */
		Ana_Set_Reg(AFE_CLASSH_CFG0, 0xd518, 0xffff);	/* Classh CK fix 591KHz */
		msleep(1);
		Ana_Set_Reg(AFE_CLASSH_CFG0, 0xd419, 0xffff);	/* Classh CK fix 591KHz */
		msleep(1);
		Ana_Set_Reg(AFE_CLASSH_CFG1, 0x0021, 0xffff);	/* Classh CK fix 591KHz */
		msleep(1);

		Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0028, 0xffff);	/* Enable cap-less LDOs (1.6V) */
		Ana_Set_Reg(AUDLDO_NVREG_CFG0, 0x0068, 0xffff);	/* Enable NV regulator (-1.6V) */
		Ana_Set_Reg(ZCD_CON0, 0x0700, 0xffff);	/* Disable AUD_ZCD */
		Ana_Set_Reg(AUDBUF_CFG0, 0xE008, 0xffff);	/* Disable headphone, voice and short-ckt protection. */
		Ana_Set_Reg(IBIASDIST_CFG0, 0x0092, 0xffff);	/* Enable IBIST */

		Ana_Set_Reg(ZCD_CON2, 0x0F9F, 0xffff);	/* Set HPR/HPL gain as minimum (~ -40dB) */
		Ana_Set_Reg(ZCD_CON3, 0x001f, 0xffff);	/* Set voice gain as minimum (~ -40dB) */
		Ana_Set_Reg(AUDBUF_CFG1, 0x0900, 0xffff);	/* De_OSC of HP and enable output STBENH */
		Ana_Set_Reg(AUDBUF_CFG2, 0x0022, 0xffff);	/* De_OSC of voice, enable output STBENH */
		Ana_Set_Reg(AUDBUF_CFG0, 0xE009, 0xffff);	/* Enable voice driver */
		Ana_Set_Reg(AUDBUF_CFG1, 0x0940, 0xffff);	/* De_OSC of HP and enable output STBENH */
		Ana_Set_Reg(AUDCLKGEN_CFG0, 0x5501, 0xffff);	/* Enable AUD_CLK */
		Ana_Set_Reg(AUDDAC_CFG0, 0x000F, 0xffff);	/* Enable Audio DAC */
		SetDcCompenSation();

		Ana_Set_Reg(AUDBUF_CFG0, 0xE010, 0xffff);	/* Switch HP MUX to audio DAC */
		Ana_Set_Reg(AUDBUF_CFG0, 0xE011, 0xffff);
		Ana_Set_Reg(AUDBUF_CFG1, 0x0900, 0xffff);
		Ana_Set_Reg(AUDBUF_CFG2, 0x0020, 0xffff);
		/* Ana_Set_Reg(AUDBUF_CFG0, 0xE146 , 0xffff); // Enable HPR/HPL */
		Ana_Set_Reg(ZCD_CON2, 0x0489, 0xffff);	/* Set HPR/HPL gain as 0dB, step by step */
		Ana_Set_Reg(ZCD_CON3, 0x0489, 0xffff);	/* Set HPR/HPL gain as 0dB, step by step */

		/* Phone_Call_16k_Vioce_mode_DL_UL */

	} else {
		Voice_Call_DAC_DAC_HS_flag = ucontrol->value.integer.value[0];
	}
	return 0;
}

/* here start uplink power function */
static const char *Pmic_Test_function[] = { "Off", "On" };

static const struct soc_enum Pmic_Test_Enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Pmic_Test_function), Pmic_Test_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Pmic_Test_function), Pmic_Test_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Pmic_Test_function), Pmic_Test_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Pmic_Test_function), Pmic_Test_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Pmic_Test_function), Pmic_Test_function),
};

static const struct snd_kcontrol_new mt6331_pmic_Test_controls[] = {
	SOC_ENUM_EXT("SineTable_DAC_HP", Pmic_Test_Enum[0], SineTable_DAC_HP_Get,
		     SineTable_DAC_HP_Set),
	SOC_ENUM_EXT("DAC_LOOP_DAC_HS", Pmic_Test_Enum[1], ADC_LOOP_DAC_HS_Get,
		     ADC_LOOP_DAC_HS_Set),
	SOC_ENUM_EXT("DAC_LOOP_DAC_HP", Pmic_Test_Enum[2], ADC_LOOP_DAC_HP_Get,
		     ADC_LOOP_DAC_HP_Set),
	SOC_ENUM_EXT("Voice_Call_DAC_DAC_HS", Pmic_Test_Enum[3], Voice_Call_DAC_DAC_HS_Get,
		     Voice_Call_DAC_DAC_HS_Set),
	SOC_ENUM_EXT("SineTable_UL2", Pmic_Test_Enum[4], SineTable_UL2_Get, SineTable_UL2_Set),
};

static const struct snd_kcontrol_new mt6331_UL_Codec_controls[] = {
	SOC_ENUM_EXT("Audio_ADC_1_Switch", Audio_UL_Enum[0], Audio_ADC1_Get, Audio_ADC1_Set),
	SOC_ENUM_EXT("Audio_ADC_2_Switch", Audio_UL_Enum[1], Audio_ADC2_Get, Audio_ADC2_Set),
	SOC_ENUM_EXT("Audio_ADC_3_Switch", Audio_UL_Enum[2], Audio_ADC3_Get, Audio_ADC3_Set),
	SOC_ENUM_EXT("Audio_ADC_4_Switch", Audio_UL_Enum[3], Audio_ADC4_Get, Audio_ADC4_Set),
	SOC_ENUM_EXT("Audio_Preamp1_Switch", Audio_UL_Enum[4], Audio_PreAmp1_Get,
		     Audio_PreAmp1_Set),
	SOC_ENUM_EXT("Audio_ADC_1_Sel", Audio_UL_Enum[5], Audio_ADC1_Sel_Get, Audio_ADC1_Sel_Set),
	SOC_ENUM_EXT("Audio_ADC_2_Sel", Audio_UL_Enum[6], Audio_ADC2_Sel_Get, Audio_ADC2_Sel_Set),
	SOC_ENUM_EXT("Audio_ADC_3_Sel", Audio_UL_Enum[7], Audio_ADC3_Sel_Get, Audio_ADC3_Sel_Set),
	SOC_ENUM_EXT("Audio_ADC_4_Sel", Audio_UL_Enum[8], Audio_ADC4_Sel_Get, Audio_ADC4_Sel_Set),
	SOC_ENUM_EXT("Audio_PGA1_Setting", Audio_UL_Enum[9], Audio_PGA1_Get, Audio_PGA1_Set),
	SOC_ENUM_EXT("Audio_PGA2_Setting", Audio_UL_Enum[10], Audio_PGA2_Get, Audio_PGA2_Set),
	SOC_ENUM_EXT("Audio_PGA3_Setting", Audio_UL_Enum[11], Audio_PGA3_Get, Audio_PGA3_Set),
	SOC_ENUM_EXT("Audio_PGA4_Setting", Audio_UL_Enum[12], Audio_PGA4_Get, Audio_PGA4_Set),
	SOC_ENUM_EXT("Audio_MicSource1_Setting", Audio_UL_Enum[13], Audio_MicSource1_Get,
		     Audio_MicSource1_Set),
	SOC_ENUM_EXT("Audio_MicSource2_Setting", Audio_UL_Enum[14], Audio_MicSource2_Get,
		     Audio_MicSource2_Set),
	SOC_ENUM_EXT("Audio_MicSource3_Setting", Audio_UL_Enum[15], Audio_MicSource3_Get,
		     Audio_MicSource3_Set),
	SOC_ENUM_EXT("Audio_MicSource4_Setting", Audio_UL_Enum[16], Audio_MicSource4_Get,
		     Audio_MicSource4_Set),
	SOC_ENUM_EXT("Audio_MIC1_Mode_Select", Audio_UL_Enum[17], Audio_Mic1_Mode_Select_Get,
		     Audio_Mic1_Mode_Select_Set),
	SOC_ENUM_EXT("Audio_MIC2_Mode_Select", Audio_UL_Enum[18], Audio_Mic2_Mode_Select_Get,
		     Audio_Mic2_Mode_Select_Set),
	SOC_ENUM_EXT("Audio_MIC3_Mode_Select", Audio_UL_Enum[19], Audio_Mic3_Mode_Select_Get,
		     Audio_Mic3_Mode_Select_Set),
	SOC_ENUM_EXT("Audio_MIC4_Mode_Select", Audio_UL_Enum[20], Audio_Mic4_Mode_Select_Get,
		     Audio_Mic4_Mode_Select_Set),
};

/*
static void speaker_event(struct snd_soc_dapm_widget *w,
			  struct snd_kcontrol *kcontrol, int event)
{
    pr_debug("speaker_event = %d\n", event);
    switch (event)
    {
	case SND_SOC_DAPM_PRE_PMU:
	    pr_debug("%s SND_SOC_DAPM_PRE_PMU", __func__);
	    break;
	case SND_SOC_DAPM_POST_PMU:
	    pr_debug("%s SND_SOC_DAPM_POST_PMU", __func__);
	    break;
	case SND_SOC_DAPM_PRE_PMD:
	    pr_debug("%s SND_SOC_DAPM_PRE_PMD", __func__);
	    break;
	case SND_SOC_DAPM_POST_PMD:
	    pr_debug("%s SND_SOC_DAPM_POST_PMD", __func__);
	case SND_SOC_DAPM_PRE_REG:
	    pr_debug("%s SND_SOC_DAPM_PRE_REG", __func__);
	case SND_SOC_DAPM_POST_REG:
	    pr_debug("%s SND_SOC_DAPM_POST_REG", __func__);
	    break;
    }
}*/


/*
static int codec_enable_rx_bias(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *kcontrol, int event)
{
    pr_debug("codec_enable_rx_bias = %d\n", event);
    switch (event)
    {
	case SND_SOC_DAPM_PRE_PMU:
	    pr_debug("%s SND_SOC_DAPM_PRE_PMU", __func__);
	    break;
	case SND_SOC_DAPM_POST_PMU:
	    pr_debug("%s SND_SOC_DAPM_POST_PMU", __func__);
	    break;
	case SND_SOC_DAPM_PRE_PMD:
	    pr_debug("%s SND_SOC_DAPM_PRE_PMD", __func__);
	    break;
	case SND_SOC_DAPM_POST_PMD:
	    pr_debug("%s SND_SOC_DAPM_POST_PMD", __func__);
	case SND_SOC_DAPM_PRE_REG:
	    pr_debug("%s SND_SOC_DAPM_PRE_REG", __func__);
	case SND_SOC_DAPM_POST_REG:
	    pr_debug("%s SND_SOC_DAPM_POST_REG", __func__);
	    break;
    }
    return 0;
}*/

static const struct snd_soc_dapm_widget mt6331_dapm_widgets[] = {
	/* Outputs */
	SND_SOC_DAPM_OUTPUT("EARPIECE"),
	SND_SOC_DAPM_OUTPUT("HEADSET"),
	SND_SOC_DAPM_OUTPUT("SPEAKER"),
	/*
	   SND_SOC_DAPM_MUX_E("VOICE_Mux_E", SND_SOC_NOPM, 0, 0  , &mt6331_Voice_Switch, codec_enable_rx_bias,
	   SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD | SND_SOC_DAPM_POST_PMD |
	   SND_SOC_DAPM_PRE_REG | SND_SOC_DAPM_POST_REG), */

};

static const struct snd_soc_dapm_route mtk_audio_map[] = {
	{"VOICE_Mux_E", "Voice Mux", "SPEAKER PGA"},
};

static void mt6331_codec_init_reg(struct snd_soc_codec *codec)
{
	pr_debug("mt6331_codec_init_reg\n");
	Ana_Set_Reg(TOP_CLKSQ, 0x0, 0xffff);
	Ana_Set_Reg(AUDNVREGGLB_CFG0, 0x0001, 0xffff);
	Ana_Set_Reg(TOP_CKPDN_CON0_SET, 0x3000, 0x3000);
	Ana_Set_Reg(AUDBUF_CFG0, 0xE000, 0xe000);	/* Disable voice DriverVer_type */
	/* set to lowe power mode */
	mt6331_upmu_set_rg_audmicbias1lowpen(true);	/* mic 1 low power mode */
	mt6331_upmu_set_rg_audmicbias0lowpen(true);	/* mic 1 low power mode */
	Ana_Set_Reg(AUDMICBIAS_CFG1, 0x2020, 0xffff);	/* power on clock */
}

void InitCodecDefault(void)
{
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP1] = 3;
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP2] = 3;
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP3] = 3;
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP4] = 3;
}

static int mt6331_codec_probe(struct snd_soc_codec *codec)
{
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	pr_debug("%s()\n", __func__);
	mt6331_codec_init_reg(codec);
	snd_soc_dapm_new_controls(dapm, mt6331_dapm_widgets, ARRAY_SIZE(mt6331_dapm_widgets));
	snd_soc_dapm_add_routes(dapm, mtk_audio_map, ARRAY_SIZE(mtk_audio_map));

	/* add codec controls */
	snd_soc_add_codec_controls(codec, mt6331_snd_controls, ARRAY_SIZE(mt6331_snd_controls));
	snd_soc_add_codec_controls(codec, mt6331_UL_Codec_controls,
				   ARRAY_SIZE(mt6331_UL_Codec_controls));
	snd_soc_add_codec_controls(codec, mt6331_Voice_Switch, ARRAY_SIZE(mt6331_Voice_Switch));
	snd_soc_add_codec_controls(codec, mt6331_pmic_Test_controls,
				   ARRAY_SIZE(mt6331_pmic_Test_controls));

#ifdef CONFIG_MTK_SPEAKER
	snd_soc_add_codec_controls(codec, mt6331_snd_Speaker_controls,
				   ARRAY_SIZE(mt6331_snd_Speaker_controls));
#endif

	snd_soc_add_codec_controls(codec, Audio_snd_auxadc_controls,
				   ARRAY_SIZE(Audio_snd_auxadc_controls));

	/* here to set  private data */
	mCodec_data = kzalloc(sizeof(mt6331_Codec_Data_Priv), GFP_KERNEL);
	if (!mCodec_data) {
		pr_debug("Failed to allocate private data\n");
		return -ENOMEM;
	}

	snd_soc_codec_set_drvdata(codec, mCodec_data);
	memset((void *)mCodec_data, 0, sizeof(mt6331_Codec_Data_Priv));

	InitCodecDefault();

	return 0;
}

static int mt6331_codec_remove(struct snd_soc_codec *codec)
{
	pr_debug("%s()\n", __func__);
	return 0;
}

static unsigned int mt6331_read(struct snd_soc_codec *codec, unsigned int reg)
{
	pr_debug("mt6331_read reg = 0x%x", reg);
	Ana_Get_Reg(reg);
	return 0;
}

static int mt6331_write(struct snd_soc_codec *codec, unsigned int reg, unsigned int value)
{
	pr_debug("mt6331_write reg = 0x%x  value= 0x%x\n", reg, value);
	Ana_Set_Reg(reg, value, 0xffffffff);
	return 0;
}

static int mt6331_Readable_registers(struct snd_soc_codec *codec, unsigned int reg)
{
	return 1;
}

static int mt6331_volatile_registers(struct snd_soc_codec *codec, unsigned int reg)
{
	return 1;
}

static struct snd_soc_codec_driver soc_mtk_codec = {
	.probe = mt6331_codec_probe,
	.remove = mt6331_codec_remove,

	.read = mt6331_read,
	.write = mt6331_write,

	.readable_register = mt6331_Readable_registers,
	.volatile_register = mt6331_volatile_registers,

	/* use add control to replace */
	/* .controls = mt6331_snd_controls, */
	/* .num_controls = ARRAY_SIZE(mt6331_snd_controls), */

	.dapm_widgets = mt6331_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(mt6331_dapm_widgets),
	.dapm_routes = mtk_audio_map,
	.num_dapm_routes = ARRAY_SIZE(mtk_audio_map),

};

static int mtk_mt6331_codec_dev_probe(struct platform_device *pdev)
{
	if (pdev->dev.of_node) {
		dev_set_name(&pdev->dev, "%s.%d", "msm-stub-codec", 1);
	}

	dev_err(&pdev->dev, "dev name %s\n", dev_name(&pdev->dev));

	return snd_soc_register_codec(&pdev->dev,
				      &soc_mtk_codec, mtk_6331_dai_codecs,
				      ARRAY_SIZE(mtk_6331_dai_codecs));
}

static int mtk_mt6331_codec_dev_remove(struct platform_device *pdev)
{
	pr_debug("%s:\n", __func__);

	snd_soc_unregister_codec(&pdev->dev);
	return 0;

}

static struct platform_driver mtk_codec_6331_driver = {
	.driver = {
		   .name = MT_SOC_CODEC_NAME,
		   .owner = THIS_MODULE,
		   },
	.probe = mtk_mt6331_codec_dev_probe,
	.remove = mtk_mt6331_codec_dev_remove,
};

static struct platform_device *soc_mtk_codec6331_dev;

static int __init mtk_mt6331_codec_init(void)
{
	int ret;
	pr_debug("%s:\n", __func__);
	soc_mtk_codec6331_dev = platform_device_alloc(MT_SOC_CODEC_NAME, -1);
	if (!soc_mtk_codec6331_dev) {
		return -ENOMEM;
	}

	ret = platform_device_add(soc_mtk_codec6331_dev);
	if (ret != 0) {
		platform_device_put(soc_mtk_codec6331_dev);
		return ret;
	}

	return platform_driver_register(&mtk_codec_6331_driver);
}
module_init(mtk_mt6331_codec_init);

static void __exit mtk_mt6331_codec_exit(void)
{
	pr_debug("%s:\n", __func__);

	platform_driver_unregister(&mtk_codec_6331_driver);
}
module_exit(mtk_mt6331_codec_exit);

/* Module information */
MODULE_DESCRIPTION("MTK  codec driver");
MODULE_LICENSE("GPL v2");
