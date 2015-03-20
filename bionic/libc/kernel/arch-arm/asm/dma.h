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
#ifndef __ASM_ARM_DMA_H
#define __ASM_ARM_DMA_H
typedef unsigned int dmach_t;
#include <linux/spinlock.h>
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#include <asm/system.h>
#include <asm/scatterlist.h>
#include <asm/arch/dma.h>
#ifndef MAX_DMA_ADDRESS
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define MAX_DMA_ADDRESS 0xffffffff
#endif
typedef unsigned int dmamode_t;
#define DMA_MODE_MASK 3
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DMA_MODE_READ 0
#define DMA_MODE_WRITE 1
#define DMA_MODE_CASCADE 2
#define DMA_AUTOINIT 4
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define clear_dma_ff(channel)
#define set_dma_addr(channel, addr)   __set_dma_addr(channel, bus_to_virt(addr))
#ifndef NO_DMA
#define NO_DMA 255
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#endif
#define isa_dma_bridge_buggy (0)
#endif
