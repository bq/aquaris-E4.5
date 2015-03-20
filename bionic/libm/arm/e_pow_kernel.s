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
	.file	"e_pow_kernel.c"
	.section	.text.hot.pow_kernel,"ax",%progbits
	.align	2
	.global	pow_kernel
	.type	pow_kernel, %function
pow_kernel:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, r7, r8, lr}
	.save {r4, r5, r6, r7, r8, lr}
	mov	r4, r2
	cmp	r4, #0
	mov	r5, r3
	fmdrr	d16, r2, r3
	fstmfdd	sp!, {d8}
	.vsave {d8}
	mov	r2, r0
	mov	r3, r1
	bic	r6, r1, #-2147483648
	bic	r7, r5, #-2147483648
	beq	.L2
	movw	ip, #65534
	movt	ip, 32751
	sub	r8, r1, #1
	cmp	r8, ip
	bhi	.L3
	mov	ip, #0
	movt	ip, 17392
	cmp	r7, ip
	bgt	.L4
	cmp	r6, #1048576
	blt	.L5
	movw	r4, #46713
	movt	r4, 11
	ubfx	r3, r6, #0, #20
	cmp	r3, r4
	mov	r2, r6, asr #20
	sub	ip, r2, #1020
	orr	r4, r3, #1069547520
	sub	ip, ip, #3
	orr	r4, r4, #3145728
	bgt	.L66
	movw	r1, #39054
	movt	r1, 3
	cmp	r3, r1
	mov	r1, r4
	fmdrr	d0, r0, r1
	ble	.L8
.L7:
	fconstd	d8, #120
	fconstd	d27, #112
	fldd	d19, .L107+16
	faddd	d2, d0, d8
	fsubd	d24, d0, d8
	fldd	d26, .L107+24
	fdivd	d6, d27, d2
	fmrrd	r2, r3, d2
	mov	r2, #0
	fmrrd	r0, r1, d16
	mov	r0, r2
	fldd	d25, .L107+32
	fmdrr	d29, r0, r1
	fmdrr	d3, r2, r3
	fldd	d23, .L107+40
	fldd	d22, .L107+80
	fsubd	d1, d3, d8
	fldd	d21, .L107+56
	fconstd	d31, #8
	fsubd	d7, d0, d1
	fldd	d5, .L107+72
	fldd	d18, .L107
	fmsr	s1, ip	@ int
	fldd	d1, .L107+64
	fldd	d2, .L107+48
	fsitod	d30, s1
	fldd	d28, .L107+8
	fmuld	d20, d24, d6
	faddd	d0, d30, d28
	fsubd	d8, d16, d29
	mov	r4, r2
	movt	r4, 16352
	fmuld	d17, d20, d20
	fmrrd	r0, r1, d20
	mov	r0, r2
	fmacd	d26, d17, d19
	fmdrr	d19, r0, r1
	fmuld	d4, d17, d17
	fnmacd	d24, d19, d3
	fmacd	d25, d26, d17
	fmuld	d26, d19, d19
	fnmacd	d24, d19, d7
	fmacd	d23, d25, d17
	faddd	d25, d26, d31
	fmuld	d24, d24, d6
	fmacd	d22, d23, d17
	fmuld	d6, d24, d20
	fmacd	d21, d22, d17
	fmacd	d6, d24, d19
	fmuld	d23, d4, d21
	faddd	d22, d6, d23
	faddd	d7, d25, d22
	fmsr	s14, r2	@ int
	fsubd	d3, d7, d31
	fmuld	d21, d24, d7
	fsubd	d31, d3, d26
	fmuld	d7, d19, d7
	fsubd	d17, d22, d31
	fmacd	d21, d20, d17
	faddd	d3, d7, d21
	fmsr	s6, r2	@ int
	fmacd	d18, d3, d5
	fsubd	d5, d3, d7
	fmuld	d2, d3, d2
	fsubd	d20, d21, d5
	fmacd	d18, d20, d1
	faddd	d1, d0, d2
	faddd	d0, d1, d18
	fmsr	s0, r2	@ int
	fsubd	d30, d0, d30
	fmuld	d29, d0, d29
	fsubd	d28, d30, d28
	fsubd	d19, d28, d2
	fsubd	d18, d18, d19
	fmuld	d16, d16, d18
	fmacd	d16, d0, d8
	faddd	d0, d16, d29
	fmrrd	r0, r1, d0
	bic	ip, r1, #-2147483648
	cmp	ip, r4
	bgt	.L24
	fmrrd	r0, r1, d0
	mov	r0, r2
	fldd	d0, .L107+88
	fmdrr	d24, r0, r1
	fldd	d4, .L107+96
	fldd	d26, .L107+144
	fsubd	d29, d24, d29
	fmuld	d23, d24, d0
	fldd	d25, .L107+120
	fsubd	d16, d16, d29
	fmuld	d3, d24, d26
	fldd	d6, .L107+136
	fmacd	d23, d16, d4
	fldd	d22, .L107+128
	fldd	d21, .L107+112
	fldd	d31, .L107+104
	fconstd	d7, #0
	faddd	d5, d3, d23
	fmuld	d17, d5, d5
	fcpyd	d1, d5
	fsubd	d2, d5, d3
	fsubd	d20, d23, d2
	fmscd	d6, d17, d25
	fmacd	d20, d5, d20
	fmacd	d22, d6, d17
	fmscd	d21, d22, d17
	fmacd	d31, d21, d17
	fnmacd	d1, d31, d17
	fmuld	d28, d5, d1
	fsubd	d19, d1, d7
	fdivd	d18, d28, d19
	fsubd	d0, d18, d20
	fsubd	d4, d0, d5
	fsubd	d2, d27, d4
	fmrrd	r4, r5, d2
	cmp	r5, #1048576
	blt	.L25
.L89:
	fmrrd	r0, r1, d2
	mov	r1, r5
.L1:
	fldmfdd	sp!, {d8}
	ldmfd	sp!, {r4, r5, r6, r7, r8, pc}
.L108:
	.align	3
.L107:
	.word	1137692678
	.word	1045233131
	.word	1073741824
	.word	1071822851
	.word	1246056175
	.word	1070235176
	.word	-1815487643
	.word	1070433866
	.word	-1457700607
	.word	1070691424
	.word	1368335949
	.word	1070945621
	.word	-536870912
	.word	1072613129
	.word	858993411
	.word	1071854387
	.word	-600177667
	.word	1072613129
	.word	341508597
	.word	-1103220768
	.word	-613438465
	.word	1071345078
	.word	212364345
	.word	-1105175455
	.word	-17155601
	.word	1072049730
	.word	1431655742
	.word	1069897045
	.word	381599123
	.word	1063698796
	.word	1925096656
	.word	1046886249
	.word	-1356472788
	.word	1058100842
	.word	-976065551
	.word	1052491073
	.word	0
	.word	1072049731
	.word	0
	.word	0
.L66:
	fmdrr	d17, r0, r1
.L6:
	fmrrd	r2, r3, d17
	sub	r3, r4, #1048576
	add	ip, ip, #1
	fmdrr	d0, r2, r3
.L8:
	fconstd	d20, #112
	fldd	d28, .L107+16
	faddd	d27, d0, d20
	fsubd	d5, d0, d20
	fldd	d26, .L107+24
	fdivd	d31, d20, d27
	fmrrd	r2, r3, d27
	mov	r2, #0
	fmrrd	r0, r1, d16
	fmdrr	d6, r2, r3
	mov	r0, r2
	fldd	d25, .L107+32
	fsubd	d29, d6, d20
	fldd	d24, .L107+40
	fldd	d23, .L107+80
	fsubd	d7, d0, d29
	fmdrr	d29, r0, r1
	fldd	d21, .L107+56
	fmsr	s9, ip	@ int
	fconstd	d27, #8
	fldd	d2, .L107+72
	fsitod	d30, s9
	fldd	d3, .L107+64
	fldd	d1, .L107+48
	fsubd	d0, d16, d29
	mov	r4, r2
	movt	r4, 16352
	fmuld	d22, d5, d31
	fmuld	d18, d22, d22
	fmrrd	r0, r1, d22
	mov	r0, r2
	fmdrr	d19, r0, r1
	fmacd	d26, d18, d28
	fnmacd	d5, d19, d6
	fmuld	d4, d18, d18
	fmacd	d25, d26, d18
	fnmacd	d5, d19, d7
	fmuld	d28, d19, d19
	fmacd	d24, d25, d18
	fmuld	d5, d5, d31
	faddd	d26, d28, d27
	fmacd	d23, d24, d18
	fmuld	d17, d5, d22
	fmacd	d21, d23, d18
	fmacd	d17, d5, d19
	fmuld	d31, d4, d21
	faddd	d25, d17, d31
	faddd	d7, d26, d25
	fmsr	s14, r2	@ int
	fsubd	d24, d7, d27
	fmuld	d23, d5, d7
	fsubd	d6, d24, d28
	fmuld	d21, d19, d7
	fsubd	d27, d25, d6
	fmacd	d23, d22, d27
	faddd	d6, d21, d23
	fmsr	s12, r2	@ int
	fcpyd	d22, d6
	fsubd	d7, d22, d21
	fmuld	d2, d22, d2
	fsubd	d18, d23, d7
	fmuld	d1, d22, d1
	fmacd	d2, d18, d3
	faddd	d3, d30, d1
	faddd	d7, d3, d2
	fmsr	s14, r2	@ int
	fcpyd	d19, d7
	fsubd	d30, d19, d30
	fmuld	d29, d19, d29
	fsubd	d4, d30, d1
	fsubd	d28, d2, d4
	fmuld	d16, d16, d28
	fmacd	d16, d19, d0
	faddd	d0, d16, d29
	fmrrd	r0, r1, d0
	bic	ip, r1, #-2147483648
	cmp	ip, r4
	bgt	.L24
	fmrrd	r0, r1, d0
	mov	r0, r2
	fldd	d5, .L107+88
	fmdrr	d17, r0, r1
	fldd	d31, .L107+96
	fldd	d25, .L107+144
	fsubd	d6, d17, d29
	fmuld	d27, d17, d5
	fldd	d26, .L107+120
	fsubd	d21, d16, d6
	fmuld	d7, d17, d25
	fldd	d24, .L107+136
	fmacd	d27, d21, d31
	fldd	d23, .L107+128
	fldd	d22, .L107+112
	fldd	d2, .L107+104
	fconstd	d1, #0
	faddd	d3, d7, d27
	fmuld	d30, d3, d3
	fcpyd	d19, d3
	fsubd	d18, d3, d7
	fsubd	d29, d27, d18
	fmscd	d24, d30, d26
	fmacd	d29, d3, d29
	fmacd	d23, d24, d30
	fmscd	d22, d23, d30
	fmacd	d2, d22, d30
	fnmacd	d19, d2, d30
	fmuld	d28, d3, d19
	fsubd	d16, d19, d1
	fdivd	d0, d28, d16
	fsubd	d5, d0, d29
	fsubd	d31, d5, d3
	fsubd	d2, d20, d31
	fmrrd	r4, r5, d2
	cmp	r5, #1048576
	bge	.L89
.L25:
	fmrrd	r0, r1, d2
	fldmfdd	sp!, {d8}
	ldmfd	sp!, {r4, r5, r6, r7, r8, lr}
	b	scalbn(PLT)
.L4:
	mov	ip, #0
	movt	ip, 32752
	cmp	r6, ip
	bgt	.L11
	beq	.L92
.L32:
	mov	ip, #0
	movt	ip, 32752
	cmp	r7, ip
	bgt	.L11
	beq	.L93
.L33:
	cmp	r3, #0
	blt	.L94
.L75:
	mov	r8, #0
.L34:
	cmp	r6, #0
	beq	.L95
	mov	r2, #0
	movt	r2, 32752
	cmp	r6, r2
	beq	.L96
.L37:
	cmp	r3, #0
	blt	.L97
	add	r3, r7, #-1140850688
	add	ip, r3, #1048576
	cmn	ip, #-1006632959
	bhi	.L15
	fmdrr	d4, r0, r1
	fconstd	d3, #112
	fcmpd	d4, d3
	fmstat
	beq	.L82
	movw	r0, #65535
	movt	r0, 16367
	cmp	r6, r0
	ble	.L98
	cmp	r5, #0
	bgt	.L65
.L15:
	fldd	d16, .L107+152
.L9:
	fmrrd	r0, r1, d16
	b	.L1
.L82:
	fmdrr	d16, r0, r1
	b	.L9
.L96:
	cmp	r5, #0
	fmdrrge	d0, r0, r1
	fabsdge	d16, d0
	bge	.L41
	fmdrr	d18, r0, r1
	fconstd	d2, #112
	fabsd	d1, d18
	fdivd	d16, d2, d1
.L41:
	cmp	r8, #1
	bne	.L9
.L86:
	fnegd	d16, d16
	b	.L9
.L95:
	cmp	r2, #0
	bne	.L37
	cmp	r5, #0
	fmdrrge	d17, r0, r1
	fabsdge	d16, d17
	bge	.L41
	fmdrr	d19, r0, r1
	fconstd	d4, #112
	fabsd	d3, d19
	fdivd	d16, d4, d3
	b	.L41
.L94:
	movw	ip, #65535
	movt	ip, 17215
	cmp	r7, ip
	movgt	r8, #2
	bgt	.L34
	movw	r8, #65535
	movt	r8, 16367
	cmp	r7, r8
	ble	.L75
	mov	ip, r7, asr #20
	movw	r8, #1043
	cmp	ip, r8
	ble	.L35
	rsb	r8, ip, #1072
	add	r8, r8, #3
	mov	ip, r4, lsr r8
	cmp	r4, ip, asl r8
	bne	.L75
.L85:
	and	ip, ip, #1
	rsb	r8, ip, #2
	b	.L34
.L93:
	cmp	r4, #0
	beq	.L33
.L11:
	fmdrr	d3, r0, r1
	fldd	d7, .L107+152
	faddd	d21, d16, d7
	faddd	d30, d3, d7
	faddd	d16, d30, d21
	b	.L9
.L92:
	cmp	r2, #0
	beq	.L32
	b	.L11
.L35:
	cmp	r4, #0
	bne	.L75
	rsb	ip, ip, #1040
	add	r8, ip, #3
	mov	ip, r7, asr r8
	cmp	r7, ip, asl r8
	movne	r8, r4
	bne	.L34
	b	.L85
.L97:
	cmp	r8, #0
	beq	.L99
	fconstd	d5, #240
	fconstd	d8, #112
	cmp	r8, #1
	mov	r2, #0
	movt	r2, 17392
	fcpydeq	d8, d5
	cmp	r7, r2
	bgt	.L45
	fmdrr	d6, r0, r1
	cmp	r6, #1048576
	fabsd	d7, d6
	bge	.L46
	fldd	d21, .L109
	fmuld	d7, d7, d21
	fmrrd	r2, r3, d7
	mov	r6, r3, asr #20
	sub	r2, r6, #1072
	sub	r1, r2, #4
	mov	r6, r3
.L47:
	movw	r3, #46713
	ubfx	r6, r6, #0, #20
	movt	r3, 11
	cmp	r6, r3
	orr	ip, r6, #1069547520
	orr	r4, ip, #3145728
	fmrrd	r2, r3, d7
	ble	.L48
	sub	r3, r4, #1048576
	add	r1, r1, #1
	fmdrr	d27, r2, r3
.L49:
	fconstd	d2, #112
	fldd	d0, .L109+80
	faddd	d18, d27, d2
	fsubd	d1, d27, d2
	fldd	d30, .L109+88
	fdivd	d21, d2, d18
	fmrrd	r2, r3, d18
	mov	r2, #0
	fcpyd	d4, d1
	fmdrr	d31, r2, r3
	fldd	d24, .L109+96
	fldd	d22, .L109+120
	fsubd	d5, d31, d2
	fmsr	s5, r1	@ int
	fldd	d23, .L109+160
	fsubd	d7, d27, d5
	fsitod	d25, s5
	fldd	d20, .L109+144
	fconstd	d29, #8
	fldd	d28, .L109+136
	fldd	d3, .L109+152
	fldd	d27, .L109+128
	fmuld	d19, d1, d21
	fmuld	d17, d19, d19
	fmrrd	r0, r1, d19
	mov	r0, r2
	fmdrr	d26, r0, r1
	fmacd	d30, d17, d0
	fnmacd	d4, d26, d31
	fmuld	d6, d17, d17
	fmacd	d24, d30, d17
	fcpyd	d2, d4
	fmuld	d0, d26, d26
	fnmacd	d2, d26, d7
	fmacd	d22, d24, d17
	faddd	d18, d0, d29
	fmuld	d1, d2, d21
	fmacd	d23, d22, d17
	fmuld	d30, d1, d19
	fmacd	d20, d23, d17
	fmacd	d30, d1, d26
	fmuld	d21, d6, d20
	faddd	d24, d30, d21
	faddd	d7, d18, d24
	fmsr	s14, r2	@ int
	fcpyd	d22, d7
	fsubd	d23, d22, d29
	fmuld	d31, d1, d22
	fsubd	d20, d23, d0
	fmuld	d29, d26, d22
	fsubd	d5, d24, d20
	fmacd	d31, d19, d5
	faddd	d7, d29, d31
	fmsr	s14, r2	@ int
	fsubd	d19, d7, d29
	fmuld	d28, d7, d28
	fsubd	d17, d31, d19
	fmuld	d27, d7, d27
	fmacd	d28, d17, d3
	faddd	d3, d25, d27
	faddd	d7, d3, d28
	fmsr	s14, r2	@ int
	fcpyd	d4, d7
	fsubd	d25, d4, d25
	fsubd	d26, d25, d27
	fsubd	d6, d28, d26
.L50:
	fmrrd	r0, r1, d16
	mov	r0, #0
	fmuld	d2, d16, d6
	fmdrr	d0, r0, r1
	mov	r4, #0
	movt	r4, 16352
	fsubd	d16, d16, d0
	fmuld	d18, d0, d4
	fmacd	d2, d16, d4
	faddd	d1, d2, d18
	fmrrd	r2, r3, d1
	bic	ip, r3, #-2147483648
	cmp	ip, r4
	ble	.L80
	movw	r1, #65535
	movt	r1, 16527
	cmp	ip, r1
	ble	.L52
	cmp	r3, r1
	ble	.L53
	add	r0, r3, #-1090519040
	add	r1, r0, #7340032
	orrs	r1, r1, r2
	bne	.L88
	fldd	d24, .L109+8
	fsubd	d22, d1, d18
	faddd	d23, d2, d24
	fcmped	d23, d22
	fmstat
	bgt	.L88
.L52:
	mov	r5, ip, asr #20
	sub	r2, r5, #1020
	sub	r4, r2, #2
	mov	ip, #1048576
	movw	r2, #65535
	add	r0, r3, ip, asr r4
	ubfx	r1, r0, #20, #11
	sub	r5, r1, #1020
	sub	r4, r5, #3
	movt	r2, 15
	rsb	ip, r1, #1040
	bic	r1, r0, r2, asr r4
	mov	r5, r1
	mov	r4, #0
	ubfx	r0, r0, #0, #20
	fmdrr	d31, r4, r5
	cmp	r3, #0
	orr	r2, r0, #1048576
	add	r3, ip, #3
	fsubd	d18, d18, d31
	mov	r2, r2, asr r3
	rsblt	r2, r2, #0
	faddd	d1, d2, d18
.L51:
	fmrrd	r4, r5, d1
	mov	r4, #0
	fldd	d29, .L109+16
	fmdrr	d7, r4, r5
	fldd	d20, .L109+24
	fldd	d5, .L109+32
	fsubd	d17, d7, d18
	fmuld	d19, d7, d29
	fldd	d28, .L109+40
	fsubd	d4, d2, d17
	fmuld	d26, d7, d5
	fldd	d27, .L109+48
	fmacd	d19, d4, d20
	fldd	d3, .L109+56
	fldd	d6, .L109+64
	fldd	d2, .L109+72
	fconstd	d25, #0
	fconstd	d0, #112
	faddd	d16, d26, d19
	fmuld	d1, d16, d16
	fcpyd	d21, d16
	fsubd	d18, d16, d26
	fsubd	d30, d19, d18
	fmscd	d27, d1, d28
	fmacd	d30, d16, d30
	fmacd	d3, d27, d1
	fmscd	d6, d3, d1
	fmacd	d2, d6, d1
	fnmacd	d21, d2, d1
	fmuld	d22, d16, d21
	fsubd	d23, d21, d25
	fdivd	d31, d22, d23
	fsubd	d29, d31, d30
	fsubd	d20, d29, d16
	fsubd	d21, d0, d20
	fmrrd	r4, r5, d21
	add	ip, r5, r2, asl #20
	cmp	ip, #1048576
	fmrrdge	r2, r3, d21
	movge	r3, ip
	fmdrrge	d21, r2, r3
	bge	.L87
	fmrrd	r0, r1, d21
	bl	scalbn(PLT)
	fmdrr	d21, r0, r1
.L87:
	fmuld	d16, d8, d21
	b	.L9
.L80:
	mov	r2, r0
	b	.L51
.L110:
	.align	3
.L109:
	.word	0
	.word	1128267776
	.word	1697350398
	.word	1016534343
	.word	212364345
	.word	-1105175455
	.word	-17155601
	.word	1072049730
	.word	0
	.word	1072049731
	.word	1925096656
	.word	1046886249
	.word	-976065551
	.word	1052491073
	.word	-1356472788
	.word	1058100842
	.word	381599123
	.word	1063698796
	.word	1431655742
	.word	1069897045
	.word	1246056175
	.word	1070235176
	.word	-1815487643
	.word	1070433866
	.word	-1457700607
	.word	1070691424
	.word	1137692678
	.word	1045233131
	.word	1073741824
	.word	1071822851
	.word	1368335949
	.word	1070945621
	.word	-536870912
	.word	1072613129
	.word	341508597
	.word	-1103220768
	.word	858993411
	.word	1071854387
	.word	-600177667
	.word	1072613129
	.word	-613438465
	.word	1071345078
	.word	-1023872167
	.word	27618847
	.word	-2013235812
	.word	2117592124
.L48:
	movw	r0, #39054
	movt	r0, 3
	mov	r3, r4
	cmp	r6, r0
	fmdrr	d27, r2, r3
	ble	.L49
	fconstd	d26, #120
	fconstd	d29, #112
	fldd	d0, .L109+80
	faddd	d22, d27, d26
	fsubd	d23, d27, d26
	fldd	d25, .L109+88
	fdivd	d1, d29, d22
	fmrrd	r2, r3, d22
	fldd	d24, .L109+96
	mov	r2, #0
	fmsr	s11, r1	@ int
	fmdrr	d4, r2, r3
	fldd	d29, .L109+120
	fldd	d21, .L109+160
	fsubd	d31, d4, d26
	fldd	d20, .L109+144
	fconstd	d28, #8
	fsubd	d7, d27, d31
	fsitod	d27, s11
	fldd	d30, .L109+152
	fldd	d3, .L109+136
	fldd	d6, .L109+128
	fldd	d26, .L109+112
	fldd	d2, .L109+104
	faddd	d31, d27, d26
	fmuld	d19, d23, d1
	fmuld	d17, d19, d19
	fmrrd	r0, r1, d19
	mov	r0, r2
	fmdrr	d18, r0, r1
	fmacd	d25, d17, d0
	fnmacd	d23, d18, d4
	fmuld	d5, d17, d17
	fmacd	d24, d25, d17
	fnmacd	d23, d18, d7
	fmuld	d0, d18, d18
	fmacd	d29, d24, d17
	fmuld	d23, d23, d1
	faddd	d25, d0, d28
	fmacd	d21, d29, d17
	fmuld	d22, d23, d19
	fmacd	d20, d21, d17
	fmacd	d22, d23, d18
	fmuld	d1, d5, d20
	faddd	d24, d22, d1
	faddd	d7, d25, d24
	fmsr	s14, r2	@ int
	fsubd	d29, d7, d28
	fmuld	d21, d23, d7
	fsubd	d4, d29, d0
	fmuld	d20, d18, d7
	fsubd	d28, d24, d4
	fmacd	d21, d19, d28
	faddd	d7, d20, d21
	fmsr	s14, r2	@ int
	fsubd	d19, d7, d20
	fmuld	d6, d7, d6
	fsubd	d17, d21, d19
	faddd	d31, d31, d6
	fmuld	d30, d17, d30
	fmacd	d30, d7, d3
	faddd	d3, d30, d2
	faddd	d7, d31, d3
	fmsr	s14, r2	@ int
	fcpyd	d4, d7
	fsubd	d7, d4, d27
	fsubd	d27, d7, d26
	fsubd	d26, d27, d6
	fsubd	d6, d3, d26
	b	.L50
.L46:
	mov	r4, r6, asr #20
	sub	r0, r4, #1020
	sub	r1, r0, #3
	b	.L47
.L45:
	add	r3, r7, #-1140850688
	add	ip, r3, #1048576
	cmn	ip, #-1006632959
	bhi	.L15
	fmdrr	d20, r0, r1
	fcmpd	d20, d5
	fmstat
	fcpydeq	d16, d8
	beq	.L9
	movw	r1, #65535
	movt	r1, 16367
	cmp	r6, r1
	bgt	.L61
	cmp	r5, #0
	blt	.L88
.L62:
	fldd	d21, .L109+168
	fmuld	d8, d8, d21
	b	.L87
.L99:
	fmdrr	d7, r0, r1
	fsubd	d27, d7, d7
	fdivd	d16, d27, d27
	b	.L9
.L98:
	cmp	r5, #0
	bge	.L15
.L65:
	fldd	d23, .L109+176
	fmuld	d16, d23, d23
	b	.L9
.L61:
	cmp	r5, #0
	ble	.L62
.L88:
	fldd	d5, .L109+176
	fmuld	d8, d8, d5
	fmuld	d16, d8, d5
	b	.L9
.L53:
	movw	r0, #52223
	movt	r0, 16528
	cmp	ip, r0
	ble	.L52
	add	r1, r3, #1061158912
	add	r0, r1, #3080192
	add	r1, r0, #13312
	orrs	r1, r1, r2
	bne	.L62
	fsubd	d30, d1, d18
	fcmped	d2, d30
	fmstat
	bhi	.L52
	b	.L62
.L2:
	cmp	r7, #0
	beq	.L69
	mov	ip, #0
	movt	ip, 16368
	cmp	r1, ip
	beq	.L100
.L10:
	mov	ip, #0
	movt	ip, 32752
	cmp	r6, ip
	bgt	.L11
	beq	.L101
.L12:
	mov	ip, #0
	movt	ip, 32752
	cmp	r7, ip
	beq	.L102
	mov	ip, #0
	movt	ip, 16368
	cmp	r5, ip
	beq	.L82
	cmp	r5, #1073741824
	beq	.L103
	mov	ip, #0
	movt	ip, 16336
	cmp	r5, ip
	beq	.L104
	mov	ip, #0
	movt	ip, 16352
	cmp	r5, ip
	beq	.L105
	mov	ip, #0
	movt	ip, 49136
	cmp	r5, ip
	beq	.L106
.L18:
	movw	ip, #65534
	movt	ip, 32751
	sub	r8, r3, #1
	cmp	r8, ip
	bls	.L20
.L3:
	cmp	r3, #0
	bne	.L4
	cmp	r2, #0
	beq	.L4
.L20:
	mov	ip, #0
	movt	ip, 17392
	cmp	r7, ip
	bgt	.L4
	cmp	r6, #1048576
	blt	.L5
	mov	r3, r6, asr #20
	fmdrr	d17, r0, r1
	sub	r4, r3, #1020
	sub	ip, r4, #3
.L22:
	movw	r0, #46713
	ubfx	r6, r6, #0, #20
	movt	r0, 11
	cmp	r6, r0
	orr	r2, r6, #1069547520
	orr	r4, r2, #3145728
	bgt	.L6
	fmrrd	r2, r3, d17
	movw	r3, #39054
	movt	r3, 3
	cmp	r6, r3
	mov	r3, r4
	fmdrr	d0, r2, r3
	bgt	.L7
	b	.L8
.L106:
	fmdrr	d19, r0, r1
	fconstd	d16, #112
	fdivd	d16, d16, d19
	b	.L9
.L105:
	cmp	r3, #0
	blt	.L18
	fldmfdd	sp!, {d8}
	ldmfd	sp!, {r4, r5, r6, r7, r8, lr}
	b	sqrt(PLT)
.L104:
	cmp	r3, #0
	blt	.L18
	bl	sqrt(PLT)
	fldmfdd	sp!, {d8}
	ldmfd	sp!, {r4, r5, r6, r7, r8, lr}
	b	sqrt(PLT)
.L103:
	fmdrr	d1, r0, r1
	fmuld	d16, d1, d1
	b	.L9
.L102:
	add	r3, r6, #-1073741824
	add	r0, r3, #1048576
	orrs	r3, r0, r2
	beq	.L69
	movw	r2, #65535
	movt	r2, 16367
	cmp	r6, r2
	ble	.L14
	cmp	r5, #0
	bge	.L9
	b	.L15
.L101:
	cmp	r2, #0
	beq	.L12
	b	.L11
.L14:
	cmp	r5, #0
	bge	.L15
	b	.L86
.L100:
	cmp	r0, #0
	bne	.L10
.L69:
	fconstd	d16, #112
	b	.L9
.L24:
	movw	r2, #65535
	movt	r2, 16527
	cmp	ip, r2
	ble	.L26
	cmp	r1, r2
	ble	.L27
	add	r2, r1, #-1090519040
	add	r3, r2, #7340032
	orrs	r3, r3, r0
	bne	.L65
	fldd	d25, .L111
	fsubd	d17, d0, d29
	faddd	d26, d16, d25
	fcmped	d26, d17
	fmstat
	bgt	.L65
.L26:
	mov	r5, ip, asr #20
	sub	r4, r5, #1020
	sub	r0, r4, #2
	mov	ip, #1048576
	movw	r4, #65535
	add	r0, r1, ip, asr r0
	ubfx	r2, r0, #20, #11
	sub	r3, r2, #1020
	sub	ip, r3, #3
	movt	r4, 15
	cmp	r1, #0
	bic	r3, r0, r4, asr ip
	ubfx	r0, r0, #0, #20
	mov	r1, r3
	orr	r4, r0, #1048576
	mov	r0, #0
	fldd	d27, .L111+8
	fmdrr	d24, r0, r1
	fldd	d2, .L111+16
	fldd	d23, .L111+24
	fsubd	d6, d29, d24
	fldd	d1, .L111+32
	fldd	d22, .L111+40
	faddd	d7, d16, d6
	flds	s14, .L111+80	@ int
	fldd	d21, .L111+48
	fsubd	d30, d7, d6
	fmuld	d18, d7, d27
	fldd	d3, .L111+56
	fsubd	d19, d16, d30
	fmuld	d4, d7, d23
	fldd	d29, .L111+64
	fmacd	d18, d19, d2
	fconstd	d28, #0
	fconstd	d0, #112
	rsb	r5, r2, #1040
	add	ip, r5, #3
	mov	r2, r4, asr ip
	rsblt	r2, r2, #0
	faddd	d16, d4, d18
	fmuld	d5, d16, d16
	fcpyd	d20, d16
	fsubd	d31, d16, d4
	fsubd	d25, d18, d31
	fmscd	d22, d5, d1
	fmacd	d25, d16, d25
	fmacd	d21, d22, d5
	fmscd	d3, d21, d5
	fmacd	d29, d3, d5
	fnmacd	d20, d29, d5
	fmuld	d26, d16, d20
	fsubd	d24, d20, d28
	fdivd	d6, d26, d24
	fsubd	d27, d6, d25
	fsubd	d16, d27, d16
	fsubd	d2, d0, d16
	fmrrd	r0, r1, d2
	add	r1, r1, r2, asl #20
	cmp	r1, #1048576
	fmrrdge	r2, r3, d2
	movge	r3, r1
	fmdrrge	d16, r2, r3
	bge	.L9
	b	.L25
.L5:
	fmdrr	d6, r0, r1
	fldd	d26, .L111+72
	fmuld	d17, d6, d26
	fmrrd	r2, r3, d17
	mov	r6, r3, asr #20
	sub	r1, r6, #1072
	sub	ip, r1, #4
	mov	r6, r3
	b	.L22
.L27:
	movw	r3, #52223
	movt	r3, 16528
	cmp	ip, r3
	ble	.L26
	mov	r2, #13312
	movt	r2, 16239
	add	r3, r1, r2
	orrs	r3, r3, r0
	bne	.L15
	fsubd	d20, d0, d29
	fcmped	d16, d20
	fmstat
	bhi	.L26
	b	.L15
.L112:
	.align	3
.L111:
	.word	1697350398
	.word	1016534343
	.word	212364345
	.word	-1105175455
	.word	-17155601
	.word	1072049730
	.word	0
	.word	1072049731
	.word	1925096656
	.word	1046886249
	.word	-976065551
	.word	1052491073
	.word	-1356472788
	.word	1058100842
	.word	381599123
	.word	1063698796
	.word	1431655742
	.word	1069897045
	.word	0
	.word	1128267776
	.word	0
	.fnend
	.size	pow_kernel, .-pow_kernel
	.ident	"GCC: (crosstool-NG linaro-1.13.1-4.7-2012.10-20121022 - Linaro GCC 2012.10) 4.7.3 20121001 (prerelease)"
	.section	.note.GNU-stack,"",%progbits
