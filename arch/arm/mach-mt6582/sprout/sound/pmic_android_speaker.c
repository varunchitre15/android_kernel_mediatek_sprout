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

/*****************************************************************************
*                E X T E R N A L      R E F E R E N C E S
******************************************************************************
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include "pmic_android_speaker.h"

/*****************************************************************************
*                C O M P I L E R      F L A G S
******************************************************************************
*/
//#define CONFIG_DEBUG_MSG
//#ifdef CONFIG_DEBUG_MSG
//#define PRINTK(format, args...) printk( KERN_EMERG format,##args )
//#else
//#define PRINTK(format, args...)
//#endif

#define AMP_CLASS_AB
//#define AMP_CLASS_D
//#define ENABLE_2_IN_1_SPK

#if !defined(AMP_CLASS_AB) && !defined(AMP_CLASS_D)
#error "MT6323 SPK AMP TYPE does not be defined!!!"
#endif
struct loudspeaker_amp_operations  pmic_spk_amp = {
.Speaker_Init = PMIC_Speaker_Init,
.Speaker_DeInit = PMIC_Speaker_DeInit,
.Speaker_Register = PMIC_Speaker_Register,
.ExternalAmp = PMIC_ExternalAmp,
.Sound_Speaker_Turnon = PMIC_Sound_Speaker_Turnon,
.Sound_Speaker_Turnoff = PMIC_Sound_Speaker_Turnoff,
.Sound_Speaker_SetVolLevel = PMIC_Sound_Speaker_SetVolLevel,
.Sound_Headset_Turnon = PMIC_Sound_Headset_Turnon,
.Sound_Headset_Turnoff = PMIC_Sound_Headset_Turnoff,
//now for  kernal use
.AudioAMPDevice_Suspend = PMIC_AudioAMPDevice_Suspend,
.AudioAMPDevice_Resume = PMIC_AudioAMPDevice_Resume,
// used for AEE beep sound
.AudioAMPDevice_SpeakerLouderOpen = PMIC_AudioAMPDevice_SpeakerLouderOpen,
.AudioAMPDevice_SpeakerLouderClose = PMIC_AudioAMPDevice_SpeakerLouderClose,
.AudioAMPDevice_mute = PMIC_AudioAMPDevice_mute,
.Audio_eamp_command = PMIC_Audio_eamp_command,
.Sound_ExtFunction = PMIC_Sound_ExtFunction,

};

/*****************************************************************************
*                          C O N S T A N T S
******************************************************************************
*/

#define PMIC_SPK_WARM_UP_TIME        (55) //unit is ms
#define PMIC_SPK_AMP_GAIN            (4)  //4:15dB
#define PMIC_RCV_AMP_GAIN            (1)  //1:-3dB
#define PMIC_SPK_R_ENABLE            (1)
#define PMIC_SPK_L_ENABLE            (1)
/*****************************************************************************
*                         D A T A      T Y P E S
******************************************************************************
*/
static int PMIC_Speaker_Volume=0;
static bool PMIC_gsk_on=false; // speaker is open?
static bool PMIC_gsk_resume=false;
static bool PMIC_gsk_forceon=false;
/*****************************************************************************
*                  F U N C T I O N        D E F I N I T I O N
******************************************************************************
*/
//extern void Yusu_Sound_AMP_Switch(BOOL enable);

bool PMIC_Speaker_Init(void)
{
   pr_notice("+PMIC_Speaker_Init Success");
#if defined(AMP_CLASS_AB)

#elif defined(AMP_CLASS_D)

#endif

   pr_notice("-PMIC_Speaker_Init Success");
   return true;
}

bool PMIC_Speaker_Register(void)
{
    return false;
}

int PMIC_ExternalAmp(void)
{
    return 0;
}

bool PMIC_Speaker_DeInit(void)
{
    return false;
}

void PMIC_Sound_SpeakerL_SetVolLevel(int level)
{
   pr_debug(" Sound_SpeakerL_SetVolLevel level=%d\n",level);
}

void PMIC_Sound_SpeakerR_SetVolLevel(int level)
{
   pr_debug(" Sound_SpeakerR_SetVolLevel level=%d\n",level);
}

void PMIC_Sound_Speaker_Turnon(int channel)
{
    pr_debug("Sound_Speaker_Turnon channel = %d\n",channel);
    if(PMIC_gsk_on)
        return;
#if defined(ENABLE_2_IN_1_SPK)
#if defined(AMP_CLASS_D)

#endif
#endif
#if defined(AMP_CLASS_AB)

#elif defined(AMP_CLASS_D)

#endif
    //msleep(SPK_WARM_UP_TIME);
    PMIC_gsk_on = true;
}

void PMIC_Sound_Speaker_Turnoff(int channel)
{
    pr_debug("Sound_Speaker_Turnoff channel = %d\n",channel);
    if(!PMIC_gsk_on)
        return;
#if defined(AMP_CLASS_AB)

#elif defined(AMP_CLASS_D)

#endif
    PMIC_gsk_on = false;
}

void PMIC_Sound_Speaker_SetVolLevel(int level)
{
    PMIC_Speaker_Volume =level;
}

void PMIC_Sound_Headset_Turnon(void)
{
}

void PMIC_Sound_Headset_Turnoff(void)
{
}

void PMIC_Sound_Earpiece_Turnon(void)
{
#if defined(ENABLE_2_IN_1_SPK)

#if defined(AMP_CLASS_D)

#endif

#endif
}

void PMIC_Sound_Earpiece_Turnoff(void)
{
#if defined(ENABLE_2_IN_1_SPK)

#if defined(AMP_CLASS_D)

#endif

#endif
}

//kernal use
void PMIC_AudioAMPDevice_Suspend(void)
{
    pr_debug("AudioDevice_Suspend\n");
    if(PMIC_gsk_on)
    {
        PMIC_Sound_Speaker_Turnoff(Channel_Stereo);
        PMIC_gsk_resume = true;
    }

}
void PMIC_AudioAMPDevice_Resume(void)
{
    pr_debug("AudioDevice_Resume\n");
    if(PMIC_gsk_resume)
        PMIC_Sound_Speaker_Turnon(Channel_Stereo);
    PMIC_gsk_resume = false;
}
void PMIC_AudioAMPDevice_SpeakerLouderOpen(void)
{
    pr_debug("AudioDevice_SpeakerLouderOpen\n");
    PMIC_gsk_forceon = false;
    if(PMIC_gsk_on)
        return;
    PMIC_Sound_Speaker_Turnon(Channel_Stereo);
    PMIC_gsk_forceon = true;
    return ;

}
void PMIC_AudioAMPDevice_SpeakerLouderClose(void)
{
    pr_debug("AudioDevice_SpeakerLouderClose\n");

    if(PMIC_gsk_forceon)
        PMIC_Sound_Speaker_Turnoff(Channel_Stereo);
    PMIC_gsk_forceon = false;

}
void PMIC_AudioAMPDevice_mute(void)
{
    pr_debug("AudioDevice_mute\n");
    if(PMIC_gsk_on)
        PMIC_Sound_Speaker_Turnoff(Channel_Stereo);
}

int PMIC_Audio_eamp_command(unsigned int type, unsigned long args, unsigned int count)
{
    return 0;
}
static char *PMIC_ExtFunArray[] =
{
    "InfoMATVAudioStart",
    "InfoMATVAudioStop",
    "End",
};

kal_int32 PMIC_Sound_ExtFunction(const char* name, void* param, int param_size)
{
    int i = 0;
    int funNum = -1;

    //Search the supported function defined in ExtFunArray
    while(strcmp("End",PMIC_ExtFunArray[i]) != 0 ) {        //while function not equal to "End"

        if (strcmp(name,PMIC_ExtFunArray[i]) == 0 ) {        //When function name equal to table, break
            funNum = i;
            break;
        }
        i++;
    }

    switch (funNum) {
        case 0:            //InfoMATVAudioStart
            pr_debug("RunExtFunction InfoMATVAudioStart \n");
            break;

        case 1:            //InfoMATVAudioStop
            pr_debug("RunExtFunction InfoMATVAudioStop \n");
            break;

        default:
             break;
    }

    return 1;
}


