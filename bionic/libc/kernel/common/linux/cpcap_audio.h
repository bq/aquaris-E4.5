/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ***   To edit the content of this header, modify the corresponding
 ***   source file (e.g. under external/kernel-headers/original/) then
 ***   run bionic/libc/kernel/tools/update_all.py
 ***
 ***   Any manual change here will be lost the next time this script will
 ***   be run. You've been warned!
 ***
 ****************************************************************************
 ****************************************************************************/
#ifndef _CPCAP_AUDIO_H
#define _CPCAP_AUDIO_H
#include <linux/ioctl.h>
#define CPCAP_AUDIO_MAGIC 'c'
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define CPCAP_AUDIO_OUT_SPEAKER 0
#define CPCAP_AUDIO_OUT_HEADSET 1
#define CPCAP_AUDIO_OUT_HEADSET_AND_SPEAKER 2
#define CPCAP_AUDIO_OUT_STANDBY 3
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define CPCAP_AUDIO_OUT_ANLG_DOCK_HEADSET 4
#define CPCAP_AUDIO_OUT_MAX 4
struct cpcap_audio_stream {
 unsigned id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 int on;
};
#define CPCAP_AUDIO_OUT_SET_OUTPUT _IOW(CPCAP_AUDIO_MAGIC, 0,   const struct cpcap_audio_stream *)
#define CPCAP_AUDIO_OUT_VOL_MIN 0
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define CPCAP_AUDIO_OUT_VOL_MAX 15
#define CPCAP_AUDIO_OUT_SET_VOLUME _IOW(CPCAP_AUDIO_MAGIC, 1, unsigned int)
#define CPCAP_AUDIO_OUT_GET_OUTPUT   _IOR(CPCAP_AUDIO_MAGIC, 2, struct cpcap_audio_stream *)
#define CPCAP_AUDIO_OUT_GET_VOLUME   _IOR(CPCAP_AUDIO_MAGIC, 3, unsigned int *)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define CPCAP_AUDIO_IN_MIC1 0
#define CPCAP_AUDIO_IN_MIC2 1
#define CPCAP_AUDIO_IN_STANDBY 2
#define CPCAP_AUDIO_IN_MAX 2
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define CPCAP_AUDIO_IN_SET_INPUT _IOW(CPCAP_AUDIO_MAGIC, 4,   const struct cpcap_audio_stream *)
#define CPCAP_AUDIO_IN_GET_INPUT _IOR(CPCAP_AUDIO_MAGIC, 5,   struct cpcap_audio_stream *)
#define CPCAP_AUDIO_IN_VOL_MIN 0
#define CPCAP_AUDIO_IN_VOL_MAX 31
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define CPCAP_AUDIO_IN_SET_VOLUME _IOW(CPCAP_AUDIO_MAGIC, 6, unsigned int)
#define CPCAP_AUDIO_IN_GET_VOLUME _IOR(CPCAP_AUDIO_MAGIC, 7, unsigned int *)
#define CPCAP_AUDIO_OUT_GET_RATE _IOR(CPCAP_AUDIO_MAGIC, 8, unsigned int *)
#define CPCAP_AUDIO_OUT_SET_RATE _IOW(CPCAP_AUDIO_MAGIC, 9, unsigned int)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define CPCAP_AUDIO_IN_GET_RATE _IOR(CPCAP_AUDIO_MAGIC, 10, unsigned int *)
#define CPCAP_AUDIO_IN_SET_RATE _IOW(CPCAP_AUDIO_MAGIC, 11, unsigned int)
#define CPCAP_AUDIO_SET_BLUETOOTH_BYPASS _IOW(CPCAP_AUDIO_MAGIC, 12, unsigned int)
#endif
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
