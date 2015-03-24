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

#include <machine/asm.h>
#include "libc_events.h"

/*
 * This code assumes it is running on a processor that supports all arm v7
 * instructions, that supports neon instructions, and that has a 32 byte
 * cache line.
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


        .syntax unified
        .fpu    neon
        .thumb
        .thumb_func

ENTRY(__memcpy_chk)
        .cfi_startproc
        cmp         r2, r3
        bhi         __memcpy_chk_fail

        // Fall through to memcpy...
        .cfi_endproc
END(__memcpy_chk)

ENTRY(memcpy)
        .cfi_startproc

        pld     [r1, #0]
        stmfd   sp!, {r0, lr}
        .save   {r0, lr}
        .cfi_def_cfa_offset 8
        .cfi_rel_offset r0, 0
        .cfi_rel_offset lr, 4
        pld     [r1, #64]

        .cfi_endproc
END(memcpy)

#define MEMCPY_BASE         __memcpy_base
#define MEMCPY_BASE_ALIGNED __memcpy_base_aligned
#include "memcpy_base.S"

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
        .word   error_string-(1b+4)
        .cfi_endproc
END(__memcpy_chk_fail)

        .data
error_string:
        .string     "memcpy buffer overflow"
