	.cpu cortex-a7
	.eabi_attribute 27, 3
	.fpu neon-vfpv4
	.eabi_attribute 20, 1
	.eabi_attribute 21, 1
	.eabi_attribute 23, 3
	.eabi_attribute 24, 1
	.eabi_attribute 25, 1
	.eabi_attribute 26, 2
	.eabi_attribute 30, 2
	.eabi_attribute 34, 1
	.eabi_attribute 18, 4
	.file	"e_log.c"
	.section	.text.hot.log,"ax",%progbits
	.align	2
	.global	log
	.type	log, %function
log:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	movw	ip, #65535
	str	r4, [sp, #-4]!
	.save {r4}
	movt	ip, 32751
	cmp	r1, ip
	mov	r3, r1
	bgt	.L16
	cmp	r1, #1048576
	blt	.L4
	movw	r1, #24420
	ubfx	ip, r3, #0, #20
	movt	r1, 9
	add	r4, ip, r1
	and	r4, r4, #1048576
	eor	r2, r4, #1069547520
	eor	r1, r2, #3145728
	orr	r1, r1, ip
	fconstd	d16, #112
	fmdrr	d18, r0, r1
	fldd	d26, .L20
	fldd	d24, .L20+8
	fsubd	d17, d18, d16
	faddd	d0, d18, d16
	fldd	d25, .L20+16
	fldd	d23, .L20+24
	fdivd	d6, d17, d0
	fldd	d22, .L20+32
	fldd	d21, .L20+40
	fldd	d20, .L20+48
	movw	r0, #60294
	movw	r1, #47185
	mov	r3, r3, asr #20
	movt	r0, 65529
	movt	r1, 6
	sub	r3, r3, #1020
	add	r2, ip, r0
	rsb	ip, ip, r1
	sub	r0, r3, #3
.L14:
	fmuld	d19, d6, d6
	add	r3, r0, r4, lsr #20
	orr	r1, ip, r2
	fmsr	s15, r3	@ int
	cmp	r1, #0
	fsitod	d27, s15
	fmuld	d7, d19, d19
	fmacd	d24, d7, d26
	fmacd	d23, d7, d25
	fmacd	d22, d24, d7
	fmacd	d21, d23, d7
	fmacd	d20, d22, d7
	fmuld	d28, d21, d7
	fmacd	d28, d20, d19
	bgt	.L17
	cmp	r3, #0
	bne	.L6
	fsubd	d18, d17, d28
	fnmacd	d17, d18, d6
.L1:
	fmrrd	r0, r1, d17
	ldmfd	sp!, {r4}
	bx	lr
.L16:
	fmdrr	d19, r0, r1
	fmdrr	d7, r0, r1
	faddd	d17, d19, d7
	b	.L1
.L6:
	fldd	d29, .L20+56
	fsubd	d30, d17, d28
	fmuld	d31, d27, d29
	fldd	d16, .L20+64
	fmscd	d31, d30, d6
	fsubd	d17, d31, d17
	fmscd	d17, d27, d16
	b	.L1
.L17:
	fconstd	d0, #96
	cmp	r3, #0
	fmuld	d1, d17, d0
	fmuld	d2, d1, d17
	bne	.L10
	faddd	d4, d2, d28
	fnmacd	d2, d4, d6
	fsubd	d17, d17, d2
	b	.L1
.L4:
	bic	ip, r1, #-2147483648
	orrs	ip, ip, r0
	beq	.L18
	cmp	r1, #0
	blt	.L19
	fmdrr	d1, r0, r1
	fldd	d17, .L20+72
	movw	r4, #24420
	fmuld	d2, d1, d17
	movt	r4, 9
	fconstd	d3, #112
	fldd	d26, .L20
	fldd	d24, .L20+8
	fmrrd	r2, r3, d2
	fmrrd	r0, r1, d2
	ubfx	ip, r3, #0, #20
	add	r2, ip, r4
	and	r4, r2, #1048576
	eor	r1, r4, #1069547520
	eor	r2, r1, #3145728
	orr	r1, r2, ip
	mov	r3, r3, asr #20
	fmdrr	d4, r0, r1
	movw	r0, #60294
	movw	r1, #47185
	movt	r0, 65529
	fsubd	d17, d4, d3
	faddd	d5, d4, d3
	movt	r1, 6
	sub	r3, r3, #1072
	add	r2, ip, r0
	fldd	d25, .L20+16
	fldd	d23, .L20+24
	fldd	d22, .L20+32
	fldd	d21, .L20+40
	fldd	d20, .L20+48
	fdivd	d6, d17, d5
	rsb	ip, ip, r1
	sub	r0, r3, #5
	b	.L14
.L10:
	fldd	d3, .L20+56
	faddd	d26, d2, d28
	fmuld	d24, d27, d3
	fldd	d25, .L20+64
	fmacd	d24, d26, d6
	fsubd	d23, d2, d24
	fsubd	d17, d23, d17
	fmscd	d17, d27, d25
	b	.L1
.L19:
	fmdrr	d22, r0, r1
	fmdrr	d21, r0, r1
	fldd	d20, .L20+80
	fsubd	d5, d22, d21
	fdivd	d17, d5, d20
	b	.L1
.L18:
	fldd	d6, .L20+88
	fldd	d27, .L20+80
	fdivd	d17, d6, d27
	b	.L1
.L21:
	.align	3
.L20:
	.word	-549563836
	.word	1069740306
	.word	-1765080098
	.word	1070024292
	.word	-797391201
	.word	1069783561
	.word	495876271
	.word	1070363077
	.word	-1809673383
	.word	1070745892
	.word	-1718093308
	.word	1071225241
	.word	1431655827
	.word	1071994197
	.word	897137782
	.word	1038760431
	.word	-18874368
	.word	1072049730
	.word	0
	.word	1129316352
	.word	0
	.word	0
	.word	0
	.word	-1018167296
	.fnend
	.size	log, .-log
	.ident	"GCC: (crosstool-NG linaro-1.13.1-4.7-2012.10-20121022 - Linaro GCC 2012.10) 4.7.3 20121001 (prerelease)"
	.section	.note.GNU-stack,"",%progbits
