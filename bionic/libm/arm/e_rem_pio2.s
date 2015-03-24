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
	.file	"e_rem_pio2.c"
	.section	.text.unlikely.__ieee754_rem_pio2,"ax",%progbits
	.align	2
	.global	__ieee754_rem_pio2
	.type	__ieee754_rem_pio2, %function
__ieee754_rem_pio2:
	.fnstart
	@ args = 0, pretend = 0, frame = 40
	@ frame_needed = 0, uses_anonymous_args = 0
	fmdrr	d22, r0, r1
	stmfd	sp!, {r4, r5, r6, r8, r9, lr}
	.save {r4, r5, r6, r8, r9, lr}
	movw	r1, #27258
	movt	r1, 16399
	.pad #48
	sub	sp, sp, #48
	fmrrd	r4, r5, d22
	mov	r6, r2
	bic	r3, r5, #-2147483648
	cmp	r3, r1
	bgt	.L2
	movw	r0, #8699
	movt	r0, 9
	ubfx	r2, r5, #0, #20
	cmp	r2, r0
	beq	.L3
	movw	ip, #55676
	movt	ip, 16386
	cmp	r3, ip
	bgt	.L4
	cmp	r5, #0
	fldd	d26, .L21
	fldd	d21, .L21+8
	ble	.L5
	fsubd	d23, d22, d26
	mov	r0, #1
	b	.L19
.L5:
	faddd	d22, d22, d26
	mvn	r0, #0
	b	.L20
.L4:
	cmp	r5, #0
	fldd	d25, .L21+16
	fldd	d21, .L21+24
	ble	.L7
	fsubd	d23, d22, d25
	mov	r0, #2
.L19:
	fsubd	d30, d23, d21
	fsubd	d31, d23, d30
	fstd	d30, [r6, #0]
	fsubd	d16, d31, d21
	fstd	d16, [r6, #8]
	b	.L6
.L7:
	faddd	d22, d22, d25
	mvn	r0, #1
.L20:
	faddd	d27, d22, d21
	fsubd	d28, d22, d27
	fstd	d27, [r6, #0]
	faddd	d29, d28, d21
	fstd	d29, [r6, #8]
	b	.L6
.L2:
	movw	r2, #17979
	movt	r2, 16412
	cmp	r3, r2
	bgt	.L8
	movw	r2, #64956
	movt	r2, 16405
	cmp	r3, r2
	bgt	.L9
	movw	r1, #55676
	movt	r1, 16402
	cmp	r3, r1
	beq	.L3
	cmp	r5, #0
	fldd	d24, .L21+32
	fldd	d21, .L21+40
	ble	.L10
	fsubd	d23, d22, d24
	mov	r0, #3
	b	.L19
.L10:
	faddd	d22, d22, d24
	mvn	r0, #2
	b	.L20
.L9:
	movw	ip, #8699
	movt	ip, 16409
	cmp	r3, ip
	beq	.L3
	cmp	r5, #0
	fldd	d20, .L21+48
	fldd	d21, .L21+56
	ble	.L11
	fsubd	d23, d22, d20
	mov	r0, #4
	b	.L19
.L11:
	faddd	d22, d22, d20
	mvn	r0, #3
	b	.L20
.L8:
	movw	r0, #8698
	movt	r0, 16697
	cmp	r3, r0
	bgt	.L12
.L3:
	fldd	d18, .L21+64
	fldd	d17, .L21+72
	fcpyd	d1, d18
	fldd	d0, .L21
	fldd	d2, .L21+8
	fmacd	d1, d22, d17
	mov	r3, r3, asr #20
	fsubd	d3, d1, d18
	fnmacd	d22, d3, d0
	ftosizd	s3, d3
	fmuld	d21, d3, d2
	fmrs	r0, s3	@ int
	fsubd	d4, d22, d21
	fmrrd	r4, r5, d4
	fstd	d4, [r6, #0]
	ubfx	r1, r5, #20, #11
	rsb	r2, r1, r3
	cmp	r2, #16
	ble	.L13
	fldd	d5, .L21+80
	fldd	d6, .L21+88
	fmuld	d7, d3, d5
	fsubd	d19, d22, d7
	fsubd	d20, d22, d19
	fsubd	d21, d20, d7
	fmscd	d21, d3, d6
	fsubd	d22, d19, d21
	fmrrd	r4, r5, d22
	fstd	d22, [r6, #0]
	ubfx	ip, r5, #20, #11
	rsb	r3, ip, r3
	cmp	r3, #49
	flddgt	d21, .L21+96
	flddgt	d6, .L21+104
	fmuldgt	d21, d3, d21
	fcpydle	d22, d19
	fsubdgt	d22, d19, d21
	fsubdgt	d19, d19, d22
	fsubdgt	d21, d19, d21
	fmscdgt	d21, d3, d6
	fsubdgt	d3, d22, d21
	fstdgt	d3, [r6, #0]
.L13:
	fldd	d23, [r6, #0]
	fsubd	d24, d22, d23
	fsubd	d25, d24, d21
	fstd	d25, [r6, #8]
	b	.L6
.L12:
	movw	ip, #65535
	movt	ip, 32751
	cmp	r3, ip
	ble	.L14
	fsubd	d19, d22, d22
	mov	r0, #0
	fstd	d19, [r6, #8]
	fstd	d19, [r6, #0]
	b	.L6
.L14:
	mov	r8, r3, asr #20
	sub	r9, r8, #1040
	sub	r2, r9, #6
	mov	r0, r4
	mov	lr, r2, asl #20
	rsb	r1, lr, r3
	fldd	d16, .L21+112
	fmdrr	d18, r0, r1
	fmrrd	r8, r9, d22
	add	r0, sp, #48
	mov	r3, #3
	ftosizd	s15, d18
	fsitod	d17, s15
	fsubd	d0, d18, d17
	fstd	d17, [sp, #24]
	fmuld	d1, d0, d16
	ftosizd	s1, d1
	fsitod	d2, s1
	fsubd	d3, d1, d2
	fstd	d2, [sp, #32]
	fmuld	d4, d3, d16
	fstd	d4, [sp, #40]
	b	.L15
.L16:
	sub	r3, r3, #1
.L15:
	fldmdbd	r0!, {d5}
	fcmpzd	d5
	fmstat
	beq	.L16
	mov	r1, #1
	add	r0, sp, #24
	str	r1, [sp, #0]
	add	r1, sp, #8
	bl	__kernel_rem_pio2(PLT)
	cmp	r5, #0
	fldd	d6, [sp, #8]
	fldd	d7, [sp, #16]
	fnegdlt	d6, d6
	fnegdlt	d7, d7
	rsblt	r0, r0, #0
	fstd	d6, [r6, #0]
	fstd	d7, [r6, #8]
.L6:
	add	sp, sp, #48
	ldmfd	sp!, {r4, r5, r6, r8, r9, pc}
.L22:
	.align	3
.L21:
	.word	1413480448
	.word	1073291771
	.word	442655537
	.word	1037087841
	.word	1413480448
	.word	1074340347
	.word	442655537
	.word	1038136417
	.word	2133852160
	.word	1074977148
	.word	-1483500342
	.word	1038683793
	.word	1413480448
	.word	1075388923
	.word	442655537
	.word	1039184993
	.word	0
	.word	1127743488
	.word	1841940611
	.word	1071931184
	.word	442499072
	.word	1037087841
	.word	771977331
	.word	1000544650
	.word	771751936
	.word	1000544650
	.word	622873025
	.word	964395930
	.word	0
	.word	1097859072
	.fnend
	.size	__ieee754_rem_pio2, .-__ieee754_rem_pio2
	.ident	"GCC: (crosstool-NG linaro-1.13.1-4.7-2012.10-20121022 - Linaro GCC 2012.10) 4.7.3 20121001 (prerelease)"
	.section	.note.GNU-stack,"",%progbits
