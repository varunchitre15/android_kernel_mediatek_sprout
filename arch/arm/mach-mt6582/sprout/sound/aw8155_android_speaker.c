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
#include "yusu_android_speaker.h"
#include "aw8155_android_speaker.h"
#include <mach/mt_gpio.h>
#include <mach/mt_typedefs.h>

/*****************************************************************************
*                C O M P I L E R      F L A G S
******************************************************************************
*/
//#define CONFIG_DEBUG_MSG
//#ifdef CONFIG_DEBUG_MSG
//#define PRINTK(format, args...) printk_( KERN_EMERG format,##args )
//#else
//#define PRINTK(format, args...)
//#endif

struct loudspeaker_amp_operations  aw8155_spk_amp = {

.Speaker_Init = AW8155_Speaker_Init,
.Speaker_DeInit = AW8155_Speaker_DeInit,
.Speaker_Register = AW8155_Speaker_Register,
.ExternalAmp = AW8155_ExternalAmp,
.Sound_Speaker_Turnon = AW8155_Sound_Speaker_Turnon,
.Sound_Speaker_Turnoff = AW8155_Sound_Speaker_Turnoff,
.Sound_Speaker_SetVolLevel = AW8155_Sound_Speaker_SetVolLevel,
.Sound_Headset_Turnon = AW8155_Sound_Headset_Turnon,
.Sound_Headset_Turnoff = AW8155_Sound_Headset_Turnoff,
//now for  kernal use
.AudioAMPDevice_Suspend = AW8155_AudioAMPDevice_Suspend,
.AudioAMPDevice_Resume = AW8155_AudioAMPDevice_Resume,
// used for AEE beep sound
.AudioAMPDevice_SpeakerLouderOpen = AW8155_AudioAMPDevice_SpeakerLouderOpen,
.AudioAMPDevice_SpeakerLouderClose = AW8155_AudioAMPDevice_SpeakerLouderClose,
.AudioAMPDevice_mute = AW8155_AudioAMPDevice_mute,
.Audio_eamp_command = AW8155_Audio_eamp_command,
.Sound_ExtFunction = AW8155_Sound_ExtFunction,

};

/*****************************************************************************
*                          C O N S T A N T S
******************************************************************************
*/

#define AW8155_SPK_WARM_UP_TIME        (10) //unit is ms
/*****************************************************************************
*                         D A T A      T Y P E S
******************************************************************************
*/
static int AW8155_Speaker_Volume=0;
static bool AW8155_gsk_on=false; // speaker is open?
static bool AW8155_gsk_resume=false;
static bool AW8155_gsk_forceon=false;
/*****************************************************************************
*                  F U N C T I O N        D E F I N I T I O N
******************************************************************************
*/
//extern void Yusu_Sound_AMP_Switch(BOOL enable);
#ifndef GPIO_SPEAKER_EN_PIN
#define GPIO_SPEAKER_EN_PIN (0x80000000|81)
#endif

bool AW8155_Speaker_Init(void)
{
   pr_notice("+AW8155_Speaker_Init Success");
   mt_set_gpio_mode(GPIO_SPEAKER_EN_PIN,GPIO_MODE_00);  // gpio mode
   mt_set_gpio_pull_enable(GPIO_SPEAKER_EN_PIN,GPIO_PULL_ENABLE);
   mt_set_gpio_dir(GPIO_SPEAKER_EN_PIN,GPIO_DIR_OUT); // output
   mt_set_gpio_out(GPIO_SPEAKER_EN_PIN,GPIO_OUT_ZERO); // high
   pr_notice("-AW8155_Speaker_Init Success");
   return true;
}

bool AW8155_Speaker_Register(void)
{
    return false;
}

int AW8155_ExternalAmp(void)
{
    return 0;
}

bool AW8155_Speaker_DeInit(void)
{
    return false;
}

void AW8155_Sound_SpeakerL_SetVolLevel(int level)
{
   pr_debug(" Sound_SpeakerL_SetVolLevel level=%d\n",level);
}

void AW8155_Sound_SpeakerR_SetVolLevel(int level)
{
   pr_debug(" Sound_SpeakerR_SetVolLevel level=%d\n",level);
}

void AW8155_Sound_Speaker_Turnon(int channel)
{
    pr_debug("Sound_Speaker_Turnon channel = %d\n",channel);
    if(AW8155_gsk_on)
        return;
    mt_set_gpio_dir(GPIO_SPEAKER_EN_PIN,GPIO_DIR_OUT); // output
    mt_set_gpio_out(GPIO_SPEAKER_EN_PIN,GPIO_OUT_ONE); // low
    udelay(2);
    mt_set_gpio_out(GPIO_SPEAKER_EN_PIN,GPIO_OUT_ZERO); // high
    udelay(2);
    mt_set_gpio_out(GPIO_SPEAKER_EN_PIN,GPIO_OUT_ONE); // high
    udelay(2);
    mt_set_gpio_out(GPIO_SPEAKER_EN_PIN,GPIO_OUT_ZERO); // high
    udelay(2);
    mt_set_gpio_out(GPIO_SPEAKER_EN_PIN,GPIO_OUT_ONE); // high
    msleep(AW8155_SPK_WARM_UP_TIME);
    AW8155_gsk_on = true;
}

void AW8155_Sound_Speaker_Turnoff(int channel)
{
    pr_debug("Sound_Speaker_Turnoff channel = %d\n",channel);
    if(!AW8155_gsk_on)
        return;
    mt_set_gpio_dir(GPIO_SPEAKER_EN_PIN,GPIO_DIR_OUT); // output
    mt_set_gpio_out(GPIO_SPEAKER_EN_PIN,GPIO_OUT_ZERO); // high
    AW8155_gsk_on = false;
}

void AW8155_Sound_Speaker_SetVolLevel(int level)
{
    AW8155_Speaker_Volume =level;
}


void AW8155_Sound_Headset_Turnon(void)
{

}

void AW8155_Sound_Headset_Turnoff(void)
{

}

//kernal use
void AW8155_AudioAMPDevice_Suspend(void)
{
    pr_debug("AudioDevice_Suspend\n");
    if(AW8155_gsk_on)
    {
        AW8155_Sound_Speaker_Turnoff(Channel_Stereo);
        AW8155_gsk_resume = true;
    }

}
void AW8155_AudioAMPDevice_Resume(void)
{
    pr_debug("AudioDevice_Resume\n");
    if(AW8155_gsk_resume)
        AW8155_Sound_Speaker_Turnon(Channel_Stereo);
    AW8155_gsk_resume = false;
}
void AW8155_AudioAMPDevice_SpeakerLouderOpen(void)
{
    pr_debug("AudioDevice_SpeakerLouderOpen\n");
        AW8155_gsk_forceon = false;
        if(AW8155_gsk_on)
        return;
    AW8155_Sound_Speaker_Turnon(Channel_Stereo);
        AW8155_gsk_forceon = true;
    return ;

}
void AW8155_AudioAMPDevice_SpeakerLouderClose(void)
{
    pr_debug("AudioDevice_SpeakerLouderClose\n");

    if(AW8155_gsk_forceon)
        AW8155_Sound_Speaker_Turnoff(Channel_Stereo);
    AW8155_gsk_forceon = false;

}
void AW8155_AudioAMPDevice_mute(void)
{
    pr_debug("AudioDevice_mute\n");
    if(AW8155_gsk_on)
        AW8155_Sound_Speaker_Turnoff(Channel_Stereo);
}

int AW8155_Audio_eamp_command(unsigned int type, unsigned long args, unsigned int count)
{
    return 0;
}
static char *AW8155_ExtFunArray[] =
{
    "InfoMATVAudioStart",
    "InfoMATVAudioStop",
    "End",
};

kal_int32 AW8155_Sound_ExtFunction(const char* name, void* param, int param_size)
{
    int i = 0;
    int funNum = -1;

    //Search the supported function defined in ExtFunArray
    while(strcmp("End",AW8155_ExtFunArray[i]) != 0 ) {        //while function not equal to "End"

        if (strcmp(name,AW8155_ExtFunArray[i]) == 0 ) {        //When function name equal to table, break
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


