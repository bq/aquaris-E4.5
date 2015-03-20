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
#ifndef __ASM_ARM_UNALIGNED_H
#define __ASM_ARM_UNALIGNED_H
#include <asm/types.h>
#define __get_unaligned_2_le(__p)   (__p[0] | __p[1] << 8)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define __get_unaligned_2_be(__p)   (__p[0] << 8 | __p[1])
#define __get_unaligned_4_le(__p)   (__p[0] | __p[1] << 8 | __p[2] << 16 | __p[3] << 24)
#define __get_unaligned_4_be(__p)   (__p[0] << 24 | __p[1] << 16 | __p[2] << 8 | __p[3])
#define __get_unaligned_le(ptr)   ({   __typeof__(*(ptr)) __v;   __u8 *__p = (__u8 *)(ptr);   switch (sizeof(*(ptr))) {   case 1: __v = *(ptr); break;   case 2: __v = __get_unaligned_2_le(__p); break;   case 4: __v = __get_unaligned_4_le(__p); break;   case 8: {   unsigned int __v1, __v2;   __v2 = __get_unaligned_4_le((__p+4));   __v1 = __get_unaligned_4_le(__p);   __v = ((unsigned long long)__v2 << 32 | __v1);   }   break;   default: __v = __bug_unaligned_x(__p); break;   }   __v;   })
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define __get_unaligned_be(ptr)   ({   __typeof__(*(ptr)) __v;   __u8 *__p = (__u8 *)(ptr);   switch (sizeof(*(ptr))) {   case 1: __v = *(ptr); break;   case 2: __v = __get_unaligned_2_be(__p); break;   case 4: __v = __get_unaligned_4_be(__p); break;   case 8: {   unsigned int __v1, __v2;   __v2 = __get_unaligned_4_be(__p);   __v1 = __get_unaligned_4_be((__p+4));   __v = ((unsigned long long)__v2 << 32 | __v1);   }   break;   default: __v = __bug_unaligned_x(__p); break;   }   __v;   })
#define __put_unaligned_le(val,ptr)   ({   switch (sizeof(*(ptr))) {   case 1:   *(ptr) = (val);   break;   case 2: __put_unaligned_2_le((val),(__u8 *)(ptr));   break;   case 4: __put_unaligned_4_le((val),(__u8 *)(ptr));   break;   case 8: __put_unaligned_8_le((val),(__u8 *)(ptr));   break;   default: __bug_unaligned_x(ptr);   break;   }   (void) 0;   })
#define __put_unaligned_be(val,ptr)   ({   switch (sizeof(*(ptr))) {   case 1:   *(ptr) = (val);   break;   case 2: __put_unaligned_2_be((val),(__u8 *)(ptr));   break;   case 4: __put_unaligned_4_be((val),(__u8 *)(ptr));   break;   case 8: __put_unaligned_8_be((val),(__u8 *)(ptr));   break;   default: __bug_unaligned_x(ptr);   break;   }   (void) 0;   })
#ifndef __ARMEB__
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define get_unaligned __get_unaligned_le
#define put_unaligned __put_unaligned_le
#else
#define get_unaligned __get_unaligned_be
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define put_unaligned __put_unaligned_be
#endif
#endif
