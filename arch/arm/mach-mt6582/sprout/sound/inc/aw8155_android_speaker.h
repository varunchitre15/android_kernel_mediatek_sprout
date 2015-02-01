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
 * aw8155_android_speaker.h
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

#ifndef _AW8155_ANDROID_SPEAKER_H_
#define _AW8155_ANDROID_SPEAKER_H_

bool AW8155_Speaker_Init(void);
bool AW8155_Speaker_DeInit(void);
bool AW8155_Speaker_Register(void);
int  AW8155_ExternalAmp(void);
void AW8155_Sound_Speaker_Turnon(int channel);
void AW8155_Sound_Speaker_Turnoff(int channel);
void AW8155_Sound_Speaker_SetVolLevel(int level);
void AW8155_Sound_Headset_Turnon(void);
void AW8155_Sound_Headset_Turnoff(void);
//now for  kernal use
void AW8155_AudioAMPDevice_Suspend(void);
void AW8155_AudioAMPDevice_Resume(void);
// used for AEE beep sound
void AW8155_AudioAMPDevice_SpeakerLouderOpen(void); //some times kernal need to force  speaker for notification
void AW8155_AudioAMPDevice_SpeakerLouderClose(void);
void AW8155_AudioAMPDevice_mute(void);
int AW8155_Audio_eamp_command(unsigned int type, unsigned long args, unsigned int count);
kal_int32 AW8155_Sound_ExtFunction(const char* name, void* param, int param_size);
#if 0
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
#else
extern struct loudspeaker_amp_operations  aw8155_spk_amp;
#endif
#endif


