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
	.file	"k_rem_pio2.c"
	.global	__aeabi_idiv
	.section	.text.unlikely.__kernel_rem_pio2,"ax",%progbits
	.align	2
	.global	__kernel_rem_pio2
	.type	__kernel_rem_pio2, %function
__kernel_rem_pio2:
	.fnstart
	@ args = 4, pretend = 0, frame = 584
	@ frame_needed = 0, uses_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, lr}
	.save {r4, r5, r6, r7, r8, r9, sl, fp, lr}
	mov	r7, r2
	fstmfdd	sp!, {d8, d9, d10, d11}
	.vsave {d8, d9, d10, d11}
	ldr	r2, .L91+24
	.pad #588
	sub	sp, sp, #588
.LPIC16:
	add	r4, pc, r2
	ldr	ip, [sp, #656]
	str	r0, [sp, #16]
	mov	r5, r1
	sub	r0, r7, #3
	mov	r1, #24
	ldr	r6, [r4, ip, asl #2]
	str	r3, [sp, #12]
	bl	__aeabi_idiv(PLT)
	ldr	r3, [sp, #12]
	ldr	sl, .L91+28
	bic	r0, r0, r0, asr #31
	sub	fp, r3, #1
	mov	r1, #24
	mvn	r8, r0
	rsb	r2, fp, r0
.LPIC17:
	add	ip, pc, sl
	str	r0, [sp, #8]
	mla	r4, r1, r8, r7
	add	r0, ip, r2, asl #2
	add	r1, fp, r6
	mov	sl, #0
	b	.L2
.L4:
	cmn	sl, r2
	add	r8, sp, #104
	ldrpl	ip, [r0, sl, asl #2]
	flddmi	d16, .L91
	fmsrpl	s15, ip	@ int
	add	ip, r8, sl, asl #3
	add	sl, sl, #1
	fsitodpl	d16, s15
	fstd	d16, [ip, #0]
.L2:
	cmp	sl, r1
	ble	.L4
	mov	r3, #0
	b	.L5
.L6:
	fldmiad	r8!, {d10}
	fldd	d11, [lr, #0]
	add	r1, r1, #1
	fmacd	d9, d10, d11
.L8:
	cmp	r1, fp
	sub	sl, sl, #8
	add	lr, r2, sl
	ble	.L6
	add	ip, sp, #424
	add	sl, ip, r3, asl #3
	add	r3, r3, #1
	fstd	d9, [sl, #0]
.L5:
	cmp	r3, r6
	bgt	.L7
	ldr	r8, [sp, #12]
	mov	sl, #0
	add	ip, r3, r8
	add	r0, sp, #104
	add	r2, r0, ip, asl #3
	mov	r1, sl
	ldr	r8, [sp, #16]
	fldd	d9, .L91
	b	.L8
.L7:
	fldd	d11, .L91+8
	fldd	d10, .L91+16
	add	lr, sp, #24
	fconstd	d9, #64
	add	r2, lr, r6, asl #2
	mov	r8, r6
	str	r2, [sp, #20]
	str	r5, [sp, #4]
.L9:
	mov	r5, r8, asl #3
	add	r3, sp, #584
	add	r1, r3, r5
	mov	ip, #0
	add	r0, sp, #424
	fldd	d16, [r1, #-160]
	add	r5, r0, r5
	mov	r3, ip
	b	.L10
.L11:
	fmuld	d28, d16, d11
	fcpyd	d31, d16
	add	sl, r5, ip
	fldd	d29, [sl, #0]
	add	lr, sp, #24
	ftosizd	s13, d28
	fsitod	d30, s13
	fnmacd	d31, d30, d10
	faddd	d16, d30, d29
	ftosizd	s17, d31
	fmrs	r2, s17	@ int
	str	r2, [lr, r3, asl #2]
	add	r3, r3, #1
.L10:
	rsb	sl, r3, r8
	cmp	sl, #0
	sub	ip, ip, #8
	bgt	.L11
	fmrrd	r0, r1, d16
	mov	r2, r4
	bl	scalbn(PLT)
	fmdrr	d8, r0, r1
	fmuld	d16, d8, d9
	fmrrd	r0, r1, d16
	bl	floor(PLT)
	fconstd	d0, #32
	cmp	r4, #0
	fmdrr	d17, r0, r1
	fnmacd	d8, d17, d0
	ftosizd	s15, d8
	fsitod	d1, s15
	fmrs	sl, s15	@ int
	fsubd	d8, d8, d1
	ble	.L12
	sub	r3, r8, #1
	add	r1, sp, #584
	add	lr, r1, r3, asl #2
	rsb	ip, r4, #24
	ldr	r9, [lr, #-560]
	rsb	r0, r4, #23
	mov	r2, r9, asr ip
	add	sl, sl, r2
	sub	r3, r9, r2, asl ip
	str	r3, [lr, #-560]
	mov	r9, r3, asr r0
	b	.L13
.L12:
	bne	.L14
	add	r9, sp, #584
	add	lr, r9, r8, asl #2
	ldr	r2, [lr, #-564]
	mov	r9, r2, asr #23
	b	.L13
.L14:
	fconstd	d2, #96
	fcmped	d8, d2
	fmstat
	movlt	r9, #0
	blt	.L16
	b	.L69
.L13:
	cmp	r9, #0
	bgt	.L15
	b	.L16
.L69:
	mov	r9, #2
.L15:
	mov	r1, #0
	add	sl, sl, #1
	mov	r3, r1
	add	r2, sp, #24
	b	.L17
.L20:
	cmp	r3, #0
	ldr	ip, [r2], #4
	bne	.L18
	cmp	ip, #0
	rsb	r0, ip, #16777216
	movne	r3, #1
	strne	r0, [r2, #-4]
	b	.L19
.L18:
	movt	lr, 255
	rsb	lr, ip, lr
	mov	r3, #1
	str	lr, [r2, #-4]
.L19:
	add	r1, r1, #1
.L17:
	cmp	r1, r8
	movw	lr, #65535
	blt	.L20
	cmp	r4, #0
	ble	.L21
	cmp	r4, #1
	beq	.L22
	cmp	r4, #2
	bne	.L21
	b	.L23
.L22:
	sub	r1, r8, #1
	add	ip, sp, #584
	add	r0, ip, r1, asl #2
	ldr	r2, [r0, #-560]
	ubfx	lr, r2, #0, #23
	str	lr, [r0, #-560]
	b	.L21
.L23:
	sub	ip, r8, #1
	add	r1, sp, #584
	add	r0, r1, ip, asl #2
	ldr	r2, [r0, #-560]
	ubfx	lr, r2, #0, #22
	str	lr, [r0, #-560]
.L21:
	cmp	r9, #2
	bne	.L16
	fconstd	d3, #112
	cmp	r3, #0
	fsubd	d8, d3, d8
	beq	.L16
	fmrrd	r0, r1, d3
	mov	r2, r4
	bl	scalbn(PLT)
	fmdrr	d4, r0, r1
	fsubd	d8, d8, d4
.L16:
	fcmpzd	d8
	fmstat
	bne	.L24
	sub	r3, r8, #1
	mov	lr, r8
	mov	r2, #0
	b	.L25
.L26:
	add	r0, sp, #24
	ldr	r1, [r0, lr, asl #2]
	orr	r2, r2, r1
.L25:
	sub	lr, lr, #1
	cmp	lr, r6
	bge	.L26
	cmp	r2, #0
	beq	.L70
	ldr	r5, [sp, #4]
	sub	r4, r4, #24
	mov	r8, r3
	b	.L67
.L29:
	add	r7, r7, #1
	b	.L27
.L70:
	mov	r7, #1
	mvn	sl, #3
.L27:
	mul	lr, sl, r7
	ldr	ip, [sp, #20]
	ldr	r1, [ip, lr]
	cmp	r1, #0
	beq	.L29
	ldr	r0, [sp, #8]
	ldr	sl, [sp, #12]
	ldr	r2, .L91+32
	add	lr, r0, r8
	add	ip, r8, sl
.LPIC18:
	add	r3, pc, r2
	add	r1, r8, r7
	add	r7, sp, #104
	add	r0, r3, lr, asl #2
	add	sl, r7, ip, asl #3
	add	r8, r8, #1
	mov	r3, #0
	mvn	r7, #7
	b	.L30
.L33:
	ldr	r9, [r0, #4]!
	add	ip, sl, r3
	fmsr	s11, r9	@ int
	ldr	lr, [sp, #16]
	mov	r2, #0
	fldd	d25, .L91
	fsitod	d8, s11
	fstd	d8, [ip, #0]
	b	.L31
.L32:
	fldmiad	lr!, {d26}
	fldd	d27, [r9, #0]
	fmacd	d25, d26, d27
.L31:
	cmp	r2, fp
	mla	r9, r7, r2, ip
	add	r2, r2, #1
	ble	.L32
	add	lr, r5, r3
	add	r8, r8, #1
	add	r3, r3, #8
	fstd	d25, [lr, #8]
.L30:
	cmp	r1, r8
	bge	.L33
	mov	r8, r1
	b	.L9
.L34:
	sub	r8, r8, #1
	sub	r4, r4, #24
.L67:
	add	r3, sp, #24
	ldr	r1, [r3, r8, asl #2]
	cmp	r1, #0
	beq	.L34
	b	.L35
.L65:
	fldd	d18, .L91+8
	add	ip, sp, #584
	fmuld	d7, d5, d18
	add	r0, ip, r8, asl #2
	add	r8, r8, #1
	add	r2, ip, r8, asl #2
	add	r4, r4, #24
	ftosizd	s3, d7
	fsitod	d19, s3
	fnmacd	d5, d19, d6
	ftosizd	s5, d19
	ftosizd	s7, d5
	fsts	s7, [r0, #-560]	@ int
	fsts	s5, [r2, #-560]	@ int
	b	.L35
.L92:
	.align	3
.L91:
	.word	0
	.word	0
	.word	0
	.word	1047527424
	.word	0
	.word	1097859072
	.word	.LANCHOR0-(.LPIC16+8)
	.word	.LANCHOR1-(.LPIC17+8)
	.word	.LANCHOR1-(.LPIC18+8)
.L89:
	ftosizd	s1, d5
	add	r3, sp, #584
	add	r1, r3, r8, asl #2
	fsts	s1, [r1, #-560]	@ int
.L35:
	mov	r1, #0
	mov	r2, r4
	mov	r0, #0
	movt	r1, 16368
	bl	scalbn(PLT)
	mov	r4, r8, asl #3
	add	ip, sp, #24
	mov	r3, #0
	fmdrr	d24, r0, r1
	add	r0, sp, #424
	add	r1, ip, r8, asl #2
	add	r2, r0, r4
	fldd	d21, .L93
	b	.L36
.L37:
	ldr	r0, [r1, r3, asl #2]
	sub	r3, r3, #1
	fmsr	s9, r0	@ int
	fsitod	d22, s9
	fmuld	d23, d22, d24
	fmuld	d24, d24, d21
	fstd	d23, [lr, #0]
.L36:
	cmn	r3, r8
	add	lr, r2, r3, asl #3
	bpl	.L37
	ldr	r1, .L93+24
	add	r2, sp, #424
	add	r4, r2, r4
	mov	r3, #0
.LPIC19:
	add	ip, pc, r1
	b	.L38
.L40:
	fldd	d19, [fp, #0]
	fldd	d20, [r0, #0]
	fmacd	d7, d19, d20
.L43:
	cmp	r1, r6
	mov	r2, r1, asl #3
	add	fp, ip, r2
	add	r0, r7, r2
	ble	.L39
.L41:
	add	r1, sp, #264
	add	lr, r1, lr
	add	r3, r3, #1
	fstd	d7, [lr, #0]
	b	.L38
.L39:
	cmp	r3, r1
	add	r1, r1, #1
	bge	.L40
	b	.L41
.L38:
	rsb	r0, r3, r8
	cmp	r0, #0
	blt	.L42
	mov	lr, r3, asl #3
	rsb	r7, lr, r4
	fldd	d7, .L93+8
	mov	r1, #0
	b	.L43
.L42:
	ldr	r3, [sp, #656]
	cmp	r3, #3
	addls	pc, pc, r3, asl #2
	b	.L44
.L48:
	b	.L45
	b	.L46
	b	.L46
	b	.L47
.L46:
	mov	r0, r8
	fldd	d5, .L93+8
	b	.L49
.L45:
	fldd	d18, .L93+8
	b	.L50
.L47:
	add	ip, sp, #584
	add	lr, ip, r8, asl #3
	sub	ip, lr, #312
	mov	lr, ip
	mov	r1, r8
	b	.L51
.L52:
	fldd	d6, [r0, #0]
	faddd	d18, d18, d6
.L50:
	cmp	r8, #0
	add	r2, sp, #264
	add	r0, r2, r8, asl #3
	sub	r8, r8, #1
	bge	.L52
	cmp	r9, #0
	fnegdne	d18, d18
	fstd	d18, [r5, #0]
	b	.L44
.L54:
	fldd	d4, [r2, #0]
	faddd	d5, d5, d4
.L49:
	cmp	r0, #0
	add	r1, sp, #264
	add	r2, r1, r0, asl #3
	sub	r0, r0, #1
	bge	.L54
	cmp	r9, #0
	add	r1, sp, #264
	fcpyd	d17, d5
	fnegdne	d5, d5
	fldd	d1, [r1, #0]
	mov	lr, #1
	fstd	d5, [r5, #0]
	fsubd	d3, d1, d17
	b	.L56
.L57:
	fldd	d2, [ip, #0]
	faddd	d3, d3, d2
.L56:
	cmp	lr, r8
	add	ip, r1, lr, asl #3
	add	lr, lr, #1
	ble	.L57
	cmp	r9, #0
	fnegdne	d3, d3
	fstd	d3, [r5, #8]
	b	.L44
.L59:
	fldd	d30, [lr, #-8]
	fldd	d29, [lr, #-16]
	faddd	d31, d29, d30
	fsubd	d16, d29, d31
	faddd	d0, d30, d16
	fstmdbd	lr!, {d0}
	fstd	d31, [lr, #-8]
.L51:
	cmp	r1, #0
	sub	r1, r1, #1
	bgt	.L59
	mov	r2, r8
	b	.L60
.L61:
	fldd	d25, [ip, #-8]
	fldd	d24, [ip, #-16]
	faddd	d26, d24, d25
	fsubd	d27, d24, d26
	faddd	d28, d25, d27
	fstmdbd	ip!, {d28}
	fstd	d26, [ip, #-8]
.L60:
	cmp	r2, #1
	sub	r2, r2, #1
	bgt	.L61
	fldd	d23, .L93+8
	b	.L62
.L63:
	fldd	d22, [r3, #0]
	faddd	d23, d23, d22
.L62:
	cmp	r8, #1
	add	r0, sp, #264
	add	r3, r0, r8, asl #3
	sub	r8, r8, #1
	bgt	.L63
	fldd	d20, [sp, #264]
	fldd	d21, [sp, #272]
	cmp	r9, #0
	fnegdne	d20, d20
	fnegdne	d21, d21
	fnegdne	d23, d23
	fstd	d20, [r5, #0]
	fstd	d21, [r5, #8]
	fstd	d23, [r5, #16]
.L44:
	and	r0, sl, #7
	b	.L88
.L24:
	fmrrd	r0, r1, d8
	rsb	r2, r4, #0
	ldr	r5, [sp, #4]
	bl	scalbn(PLT)
	fldd	d6, .L93+16
	fmdrr	d5, r0, r1
	fcmped	d5, d6
	fmstat
	bge	.L65
	b	.L89
.L88:
	add	sp, sp, #588
	fldmfdd	sp!, {d8, d9, d10, d11}
	ldmfd	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, pc}
.L94:
	.align	3
.L93:
	.word	0
	.word	1047527424
	.word	0
	.word	0
	.word	0
	.word	1097859072
	.word	.LANCHOR2-(.LPIC19+8)
	.fnend
	.size	__kernel_rem_pio2, .-__kernel_rem_pio2
	.section	.rodata.init_jk,"a",%progbits
	.align	3
.LANCHOR0 = . + 0
	.type	init_jk, %object
	.size	init_jk, 16
init_jk:
	.word	3
	.word	4
	.word	4
	.word	6
	.section	.rodata.ipio2,"a",%progbits
	.align	3
.LANCHOR1 = . + 0
	.type	ipio2, %object
	.size	ipio2, 264
ipio2:
	.word	10680707
	.word	7228996
	.word	1387004
	.word	2578385
	.word	16069853
	.word	12639074
	.word	9804092
	.word	4427841
	.word	16666979
	.word	11263675
	.word	12935607
	.word	2387514
	.word	4345298
	.word	14681673
	.word	3074569
	.word	13734428
	.word	16653803
	.word	1880361
	.word	10960616
	.word	8533493
	.word	3062596
	.word	8710556
	.word	7349940
	.word	6258241
	.word	3772886
	.word	3769171
	.word	3798172
	.word	8675211
	.word	12450088
	.word	3874808
	.word	9961438
	.word	366607
	.word	15675153
	.word	9132554
	.word	7151469
	.word	3571407
	.word	2607881
	.word	12013382
	.word	4155038
	.word	6285869
	.word	7677882
	.word	13102053
	.word	15825725
	.word	473591
	.word	9065106
	.word	15363067
	.word	6271263
	.word	9264392
	.word	5636912
	.word	4652155
	.word	7056368
	.word	13614112
	.word	10155062
	.word	1944035
	.word	9527646
	.word	15080200
	.word	6658437
	.word	6231200
	.word	6832269
	.word	16767104
	.word	5075751
	.word	3212806
	.word	1398474
	.word	7579849
	.word	6349435
	.word	12618859
	.section	.rodata.PIo2,"a",%progbits
	.align	3
.LANCHOR2 = . + 0
	.type	PIo2, %object
	.size	PIo2, 64
PIo2:
	.word	1073741824
	.word	1073291771
	.word	0
	.word	1047807021
	.word	-2147483648
	.word	1022903960
	.word	1610612736
	.word	997772369
	.word	-2147483648
	.word	972036995
	.word	1073741824
	.word	947528992
	.word	-2147483648
	.word	920879650
	.word	0
	.word	896135965
	.ident	"GCC: (crosstool-NG linaro-1.13.1-4.7-2012.10-20121022 - Linaro GCC 2012.10) 4.7.3 20121001 (prerelease)"
	.section	.note.GNU-stack,"",%progbits
