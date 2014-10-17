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

/*******************************************************************************
 *
 * Filename:
 * ---------
 * pmic_android_speaker.h
 *
 * Project:
 * --------
 *   sprout
 *
 * Description:
 * ------------
 *   speaker select
 *
 * Author:
 * -------
 *   ChiPeng Chang (mtk02308)
 *
 *
 *------------------------------------------------------------------------------
 * $Revision$
 * $Modtime:$
 * $Log:$
 *
 * 06 17 2012 weiguo.li
 * [ALPS00302429] [Need Patch] [Volunteer Patch]modify speaker driver
 * .
 *
 * 12 14 2011 weiguo.li
 * [ALPS00102848] [Need Patch] [Volunteer Patch] build waring in yusu_android_speaker.h
 * .
 *
 * 11 10 2011 weiguo.li
 * [ALPS00091610] [Need Patch] [Volunteer Patch]chang yusu_android_speaker.c function name and modules use it
 * .
 *
 * 09 28 2011 weiguo.li
 * [ALPS00076254] [Need Patch] [Volunteer Patch]LGE audio driver using Voicebuffer for incall
 * .
 *
 * 07 08 2011 weiguo.li
 * [ALPS00059378] poring lge code to alps(audio)
 * .
 *
 * 07 23 2010 chipeng.chang
 * [ALPS00122386][Music]The playing music is no sound after below steps.
 * when mode change , record deivce for volume setting.
 *
 * 07 03 2010 chipeng.chang
 * [ALPS00002838][Need Patch] [Volunteer Patch] for speech volume step
 * modify for headset customization.
 *
 *******************************************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <mach/mt_typedefs.h>
#include "yusu_android_speaker.h"

#ifndef _PMIC_ANDROID_SPEAKER_H_
#define _PMIC_ANDROID_SPEAKER_H_

bool PMIC_Speaker_Init(void);
bool PMIC_Speaker_DeInit(void);
bool PMIC_Speaker_Register(void);
int  PMIC_ExternalAmp(void);
void PMIC_Sound_Speaker_Turnon(int channel);
void PMIC_Sound_Speaker_Turnoff(int channel);
void PMIC_Sound_Speaker_SetVolLevel(int level);
void PMIC_Sound_Headset_Turnon(void);
void PMIC_Sound_Headset_Turnoff(void);
//now for  kernal use
void PMIC_AudioAMPDevice_Suspend(void);
void PMIC_AudioAMPDevice_Resume(void);
// used for AEE beep sound
void PMIC_AudioAMPDevice_SpeakerLouderOpen(void); //some times kernal need to force  speaker for notification
void PMIC_AudioAMPDevice_SpeakerLouderClose(void);
void PMIC_AudioAMPDevice_mute(void);
int PMIC_Audio_eamp_command(unsigned int type, unsigned long args, unsigned int count);
kal_int32 PMIC_Sound_ExtFunction(const char* name, void* param, int param_size);

#if 0
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
#else
extern struct loudspeaker_amp_operations  pmic_spk_amp;
#endif
#endif


