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
#ifndef _LINUX_TIMERFD_H
#define _LINUX_TIMERFD_H
#include <linux/fcntl.h>
#define TFD_TIMER_ABSTIME (1 << 0)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TFD_TIMER_CANCEL_ON_SET (1 << 1)
#define TFD_CLOEXEC O_CLOEXEC
#define TFD_NONBLOCK O_NONBLOCK
#define TFD_SHARED_FCNTL_FLAGS (TFD_CLOEXEC | TFD_NONBLOCK)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TFD_CREATE_FLAGS TFD_SHARED_FCNTL_FLAGS
#define TFD_SETTIME_FLAGS (TFD_TIMER_ABSTIME | TFD_TIMER_CANCEL_ON_SET)
#endif
