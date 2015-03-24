/*
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Copyright (c) 2013 ARM Ltd
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ARM LTD ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ARM LTD BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Prototype: void *memcpy (void *dst, const void *src, size_t count).

#include <machine/asm.h>
#include "libc_events.h"

        .text
        .syntax unified
        .fpu    neon

/* Temporarily use memcpy */
        .global vfp_copy_forward_not_align
        .type vfp_copy_forward_not_align, %function
        .align 4
vfp_copy_forward_not_align:
        .fnstart
        stmfd   sp!, {r0-r3, lr}
        mov     r3, r0
        mov     r0, r1
        mov     r1, r3
        bl      memcpy
        ldmfd   sp!, {r0-r3, lr}
        bx      lr
        .fnend
 
        .global vfp_copy_backward_not_align
        .type vfp_copy_backward_not_align, %function
        .align 4
vfp_copy_backward_not_align:
        .fnstart  
        stmfd   sp!, {r0-r5, lr}

vfp_copy_backward_pld:
        subs    r2, r2, #0x40
        blt     finish_vfp_copy_backward
        mov     r5, r1
        sub     r5, r5, #0x100        
        pld     [r5, #0x00]  
        pld     [r5, #0x40] 
        pld     [r5, #0x80] 
        pld     [r5, #0xc0] 
        sub     r1, r1, #0x40
        sub     r0, r0, #0x40
vfp_copy_backward_start_copy:
				mov     r5, r0
        mov     r4, r1 
				vld1.8      {d0 - d3}, [r4]!
        vld1.8      {d4 - d7}, [r4]!
        vst1.8      {d0 - d3}, [r5, :128]!
        vst1.8      {d4 - d7}, [r5, :128]!
        //
        subs    r2, r2, #0x40
        blt     finish_vfp_copy_backward
        sub     r1, r1, #0x40
        sub     r0, r0, #0x40
        mov     r5, r0
        mov     r4, r1 
				vld1.8      {d0 - d3}, [r4]!
        vld1.8      {d4 - d7}, [r4]!
        vst1.8      {d0 - d3}, [r5, :128]!
        vst1.8      {d4 - d7}, [r5, :128]!
        //
        subs    r2, r2, #0x40
        blt     finish_vfp_copy_backward
        sub     r1, r1, #0x40
        sub     r0, r0, #0x40
        mov     r5, r0
        mov     r4, r1 
				vld1.8      {d0 - d3}, [r4]!
        vld1.8      {d4 - d7}, [r4]!
        vst1.8      {d0 - d3}, [r5, :128]!
        vst1.8      {d4 - d7}, [r5, :128]!
        //
        subs    r2, r2, #0x40
        blt     finish_vfp_copy_backward
        sub     r1, r1, #0x40
        sub     r0, r0, #0x40
        mov     r5, r0  
        mov     r4, r1 
				vld1.8      {d0 - d3}, [r4]!
        vld1.8      {d4 - d7}, [r4]!
        vst1.8      {d0 - d3}, [r5, :128]!
        vst1.8      {d4 - d7}, [r5, :128]! 
        b       vfp_copy_backward_pld

finish_vfp_copy_backward:
				cmp         r3, #0x00  
				beq finish_vfp_copy_backward_end
        sub     r1, r1, #0x20
        pld     [r1, #0x00]  
				sub     r0, r0, #0x20 
        vld1.8  {d0-d3}, [r1]!
        vst1.8  {d0-d3}, [r0, :128]! 
finish_vfp_copy_backward_end:				
        ldmfd   sp!, {r0-r5, lr} 
        bx      lr
        .fnend
        
        .global memcpy
        .type memcpy, %function
        .align 4

ENTRY(__memcpy_chk)
        .cfi_startproc
        cmp     r2, r3
        bhi     __memcpy_chk_fail

        // Fall through to memcpy...
        .cfi_endproc
END(__memcpy_chk)

#define CACHE_LINE_SIZE 64
#if 1
ENTRY(memcpy)
        .cfi_startproc
        pld         [r1, #(CACHE_LINE_SIZE * 0)]
        .word 0xf590f000  // pldw         [r0, #(CACHE_LINE_SIZE * 0)]

        push    {r0, lr} 
        pld         [r1, #(CACHE_LINE_SIZE * 1)]
        .word 0xf590f040  // pldw         [r0, #(CACHE_LINE_SIZE * 1)]
        .save   {r0, lr}
        .cfi_def_cfa_offset 8
        .cfi_rel_offset r0, 0
        .cfi_rel_offset lr, 4

        .cfi_endproc
END(memcpy)

#define MEMCPY_BASE         __memcpy_base
#define MEMCPY_BASE_ALIGNED __memcpy_base_aligned
#include "memcpy_base.S"
#else
ENTRY(memcpy)
        .cfi_startproc
        .save       {r0, lr}
        /* start preloading as early as possible */ 
        pld         [r1, #(CACHE_LINE_SIZE * 0)]
        .word 0xf590f000  // pldw         [r0, #(CACHE_LINE_SIZE * 0)]

        stmfd       sp!, {r0, lr}

        pld         [r1, #(CACHE_LINE_SIZE * 1)]
        .word 0xf590f040  // pldw         [r0, #(CACHE_LINE_SIZE * 1)]

/* If Neon supports unaligned access then remove the align code,  
 * unless a size limit has been specified.  
 */  
        /* do we have at least 16-bytes to copy (needed for alignment below) */
        cmp         r2, #16
        blo         5f

        /* check if buffers are aligned. If so, run arm-only version */
        eor         r3, r0, r1  
        ands        r3, r3, #0x3  
        beq         11f  
  
        /* align destination to cache-line for the write-buffer */  
        rsb         r3, r0, #0
        ands        r3, r3, #0xF
        beq         2f

        /* copy up to 15-bytes (count in r3) */
        sub         r2, r2, r3
        movs        ip, r3, lsl #31
        ldrbmi      lr, [r1], #1
        strbmi      lr, [r0], #1
        ldrbcs      ip, [r1], #1
        ldrbcs      lr, [r1], #1
        strbcs      ip, [r0], #1
        strbcs      lr, [r0], #1
        movs        ip, r3, lsl #29
        bge         1f
        // copies 4 bytes, destination 32-bits aligned
        vld4.8      {d0[0], d1[0], d2[0], d3[0]}, [r1]!
        vst4.8      {d0[0], d1[0], d2[0], d3[0]}, [r0, :32]!
1:      bcc         2f
        // copies 8 bytes, destination 64-bits aligned
        vld1.8      {d0}, [r1]!
        vst1.8      {d0}, [r0, :64]!
2:

        /* preload immediately the next cache line, which we may need */
        pld         [r1, #(CACHE_LINE_SIZE * 0)]
        pld         [r1, #(CACHE_LINE_SIZE * 1)]
	.word 0xf590f000  // pldw         [r0, #(CACHE_LINE_SIZE * 0)]
	.word 0xf590f040  // pldw         [r0, #(CACHE_LINE_SIZE * 1)]
        /* make sure we have at least 64 bytes to copy */
        subs        r2, r2, #64
        blo         2f

        /* preload all the cache lines we need. */
        pld         [r1, #(CACHE_LINE_SIZE * 2)]
        pld         [r1, #(CACHE_LINE_SIZE * 3)]
	.word 0xf590f080  // pldw         [r0, #(CACHE_LINE_SIZE * 2)]
	.word 0xf590f0c0  // pldw         [r0, #(CACHE_LINE_SIZE * 3)]
1:      /* The main loop copies 64 bytes at a time */
        vld1.8      {d0 - d3}, [r1]!
        vld1.8      {d4 - d7}, [r1]!
        pld         [r1, #(CACHE_LINE_SIZE * 2)]  
        pld         [r1, #(CACHE_LINE_SIZE * 3)]  
        .word 0xf590f080  // pldw         [r0, #(CACHE_LINE_SIZE * 2)]
        .word 0xf590f0c0  // pldw         [r0, #(CACHE_LINE_SIZE * 3)]

        subs        r2, r2, #64
        vst1.8      {d0 - d3}, [r0, :128]!
        vst1.8      {d4 - d7}, [r0, :128]!
        bhs         1b

2:      /* fix-up the remaining count and make sure we have >= 32 bytes left */
        add         r2, r2, #64
        subs        r2, r2, #32
        blo         4f

3:      /* 32 bytes at a time. These cache lines were already preloaded */
        vld1.8      {d0 - d3}, [r1]!
        subs        r2, r2, #32
        vst1.8      {d0 - d3}, [r0, :128]!
        bhs         3b

4:      /* less than 32 left */
        add         r2, r2, #32
        tst         r2, #0x10
        beq         5f
        // copies 16 bytes, 128-bits aligned
        vld1.8      {d0, d1}, [r1]!
        vst1.8      {d0, d1}, [r0, :128]!

5:      /* copy up to 15-bytes (count in r2) */
        movs        ip, r2, lsl #29
        bcc         1f
        vld1.8      {d0}, [r1]!
        vst1.8      {d0}, [r0]!
1:      bge         2f
        vld4.8      {d0[0], d1[0], d2[0], d3[0]}, [r1]!
        vst4.8      {d0[0], d1[0], d2[0], d3[0]}, [r0]!
2:      movs        ip, r2, lsl #31
        ldrbmi      r3, [r1], #1
        ldrbcs      ip, [r1], #1
        ldrbcs      lr, [r1], #1
        strbmi      r3, [r0], #1
        strbcs      ip, [r0], #1
        strbcs      lr, [r0], #1
        VMOV        s0, s0 @ NOP for ARM Errata 754319 754320

        ldmfd       sp!, {r0, lr}
        bx          lr
11:  
        /* Simple arm-only copy loop to handle aligned copy operations */  
        stmfd       sp!, {r4, r5, r6, r7, r8}  
        pld         [r1, #(CACHE_LINE_SIZE * 2)]  
        .word 0xf590f080  // pldw         [r0, #(CACHE_LINE_SIZE * 2)]
        /* Check alignment */  
        rsb         r3, r1, #0  
        ands        r3, #3  
        beq         2f  
  
        /* align source to 32 bits. We need to insert 2 instructions between  
         * a ldr[b|h] and str[b|h] because byte and half-word instructions  
         * stall 2 cycles.  
         */  
        movs        r12, r3, lsl #31  
        sub         r2, r2, r3      /* we know that r3 <= r2 because r2 >= 4 */  
        ldrbmi      r3, [r1], #1  
        ldrbcs      r4, [r1], #1  
        ldrbcs      r5, [r1], #1  
        strbmi      r3, [r0], #1  
        strbcs      r4, [r0], #1  
        strbcs      r5, [r0], #1  
2:  
        subs        r2, #32  
        blt         5f  
        pld         [r1, #(CACHE_LINE_SIZE * 3)]  
        .word 0xf590f0c0  // pldw         [r0, #(CACHE_LINE_SIZE * 3)]
3:      /* Main copy loop, copying 32 bytes at a time */  
        pld         [r1, #(CACHE_LINE_SIZE * 4)]  
        .word 0xf590f100  // pldw         [r0, #(CACHE_LINE_SIZE * 4)]
        ldmia       r1!, {r3, r4, r5, r6, r7, r8, r12, lr}  
        subs        r2, r2, #32  
        stmia       r0!, {r3, r4, r5, r6, r7, r8, r12, lr}  
        bge         3b  
5:      /* Handle any remaining bytes */  
        adds        r2, #32  
        beq         6f  
  
        movs        r12, r2, lsl #28  
        ldmiacs     r1!, {r3, r4, r5, r6}   /* 16 bytes */  
        ldmiami     r1!, {r7, r8}           /*  8 bytes */  
        stmiacs     r0!, {r3, r4, r5, r6}  
        stmiami     r0!, {r7, r8}  
        movs        r12, r2, lsl #30  
        ldrcs       r3, [r1], #4            /*  4 bytes */  
        ldrhmi      r4, [r1], #2            /*  2 bytes */  
        strcs       r3, [r0], #4  
        strhmi      r4, [r0], #2  
        tst         r2, #0x1  
        ldrbne      r3, [r1]                /*  last byte  */  
        strbne      r3, [r0]  
6:  
        ldmfd       sp!, {r4, r5, r6, r7, r8}  
        ldmfd       sp!, {r0, pc} 

        .cfi_endproc
END(memcpy)
#endif

ENTRY(__memcpy_chk_fail)
        .cfi_startproc
        // Preserve lr for backtrace.
        push    {lr}
        .save   {lr}
        .cfi_def_cfa_offset 4
        .cfi_rel_offset lr, 0

        ldr     r0, error_message
        ldr     r1, error_code
1:
        add     r0, pc
        bl      __fortify_chk_fail
error_code:
        .word   BIONIC_EVENT_MEMCPY_BUFFER_OVERFLOW
error_message:
        .word   error_string-(1b+8)
        .cfi_endproc
END(__memcpy_chk_fail)

        .data
error_string:
        .string "memcpy buffer overflow"
