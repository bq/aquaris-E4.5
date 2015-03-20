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
	.file	"e_exp.c"
	.section	.text.hot.exp,"ax",%progbits
	.align	2
	.global	exp
	.type	exp, %function
exp:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	fmdrr	d24, r0, r1
	mov	r3, r1
	movw	r0, #11842
	bic	r1, r3, #-2147483648
	movt	r0, 16342
	cmp	r1, r0
	bls	.L2
	movw	r2, #41649
	movt	r2, 16368
	cmp	r1, r2
	mov	ip, r3, lsr #31
	bls	.L23
	movw	r0, #11841
	movt	r0, 16518
	cmp	r1, r0
	bhi	.L6
	ldr	r1, .L25+112
	fldd	d2, .L25
.LPIC0:
	add	r3, pc, r1
	add	r2, r3, ip, asl #3
	fldd	d3, [r2, #0]
	fmacd	d3, d24, d2
	ftosizd	s15, d3
	fldd	d4, .L25+8
	fsitod	d5, s15
	fldd	d6, .L25+16
	fmrs	ip, s15	@ int
	fnmacd	d24, d5, d4
	fmuld	d7, d5, d6
	fsubd	d22, d24, d7
	fldd	d23, .L25+24
	fmuld	d16, d22, d22
	fldd	d25, .L25+32
	fldd	d26, .L25+40
	fmscd	d25, d16, d23
	fldd	d27, .L25+48
	fmacd	d26, d25, d16
	fldd	d28, .L25+56
	fmscd	d27, d26, d16
	fcpyd	d30, d22
	fmacd	d28, d27, d16
	fconstd	d29, #0
	fnmacd	d30, d28, d16
	fmuld	d17, d22, d30
	fsubd	d0, d29, d30
	fdivd	d18, d17, d0
	fsubd	d1, d7, d18
	fconstd	d20, #112
	fsubd	d24, d1, d24
	mvn	r0, #1020
	cmp	ip, r0
	fsubd	d19, d20, d24
	blt	.L7
.L21:
	fmrrd	r0, r1, d19
	fmrrd	r2, r3, d19
	add	r3, r1, ip, asl #20
	fmdrr	d24, r2, r3
.L1:
	fmrrd	r0, r1, d24
	bx	lr
.L2:
	movw	r3, #65535
	movt	r3, 15919
	cmp	r1, r3
	bls	.L13
	fmuld	d25, d24, d24
	fldd	d26, .L25+24
	fldd	d27, .L25+32
	fldd	d28, .L25+40
	fmscd	d27, d25, d26
	fldd	d29, .L25+48
	fmacd	d28, d27, d25
	fldd	d30, .L25+56
	fmscd	d29, d28, d25
	fcpyd	d17, d24
	fmacd	d30, d29, d25
	fconstd	d31, #0
	fnmacd	d17, d30, d25
	fmuld	d18, d24, d17
	fsubd	d1, d17, d31
	fdivd	d20, d18, d1
	fsubd	d19, d20, d24
	fconstd	d21, #112
	fsubd	d24, d21, d19
	fmrrd	r0, r1, d24
	bx	lr
.L23:
	cmp	ip, #0
	fldd	d4, .L25+8
	beq	.L4
	faddd	d2, d24, d4
	fldd	d19, .L25+16
	faddd	d3, d2, d19
	fldd	d4, .L25+24
	fmuld	d5, d3, d3
	fldd	d6, .L25+32
	fldd	d21, .L25+40
	fmscd	d6, d5, d4
	fldd	d7, .L25+48
	fmacd	d21, d6, d5
	fldd	d23, .L25+56
	fmscd	d7, d21, d5
	fcpyd	d25, d3
	fmacd	d23, d7, d5
	fconstd	d16, #0
	fnmacd	d25, d23, d5
	fmuld	d22, d3, d25
	fsubd	d27, d16, d25
	fldd	d28, .L25+64
	fdivd	d29, d22, d27
	fsubd	d30, d28, d29
	fconstd	d31, #112
	fsubd	d17, d30, d2
	fsubd	d0, d31, d17
	fmrrd	r0, r1, d0
	fmrrd	r2, r3, d0
	sub	r3, r1, #1048576
	fmdrr	d24, r2, r3
	b	.L1
.L4:
	fsubd	d6, d24, d4
	fldd	d5, .L25+16
	fsubd	d7, d6, d5
	fldd	d23, .L25+24
	fmuld	d16, d7, d7
	fldd	d25, .L25+32
	fldd	d26, .L25+40
	fmscd	d25, d16, d23
	fldd	d22, .L25+48
	fmacd	d26, d25, d16
	fldd	d27, .L25+56
	fmscd	d22, d26, d16
	fcpyd	d29, d7
	fmacd	d27, d22, d16
	fconstd	d28, #0
	fnmacd	d29, d27, d16
	fmuld	d31, d7, d29
	fsubd	d17, d28, d29
	fdivd	d0, d31, d17
	fsubd	d18, d5, d0
	fconstd	d1, #112
	fsubd	d20, d18, d6
	fsubd	d24, d1, d20
	fmrrd	r0, r1, d24
	fmrrd	r2, r3, d24
	add	r3, r1, #1048576
	fmdrr	d24, r2, r3
	b	.L1
.L8:
	fldd	d19, .L25+72
	fcmped	d24, d19
	fmstat
	bgt	.L24
	fldd	d21, .L25+80
	fcmped	d24, d21
	fmstat
	bmi	.L12
	ldr	r1, .L25+116
	fldd	d2, .L25
.LPIC1:
	add	r3, pc, r1
	add	ip, r3, ip, asl #3
	fldd	d3, [ip, #0]
	fmacd	d3, d24, d2
	ftosizd	s1, d3
	fldd	d4, .L25+8
	fsitod	d5, s1
	fldd	d6, .L25+16
	fnmacd	d24, d5, d4
	fmuld	d7, d5, d6
	fsubd	d23, d24, d7
	fldd	d16, .L25+24
	fmuld	d25, d23, d23
	fldd	d26, .L25+32
	fldd	d22, .L25+40
	fmscd	d26, d25, d16
	fldd	d27, .L25+48
	fmacd	d22, d26, d25
	fldd	d28, .L25+56
	fmscd	d27, d22, d25
	fcpyd	d30, d23
	fmacd	d28, d27, d25
	fconstd	d29, #0
	fnmacd	d30, d28, d25
	fmrs	ip, s1	@ int
	fmuld	d17, d23, d30
	fsubd	d0, d29, d30
	fdivd	d18, d17, d0
	fsubd	d1, d7, d18
	fconstd	d20, #112
	fsubd	d24, d1, d24
	mvn	r0, #1020
	cmp	ip, r0
	fsubd	d19, d20, d24
	bge	.L21
.L7:
	fmrrd	r0, r1, d19
	fmrrd	r2, r3, d19
	add	r3, ip, #1000
	add	r3, r1, r3, asl #20
	fmdrr	d21, r2, r3
	fldd	d2, .L25+88
	fmuld	d24, d21, d2
	b	.L1
.L6:
	movw	r2, #65535
	movt	r2, 32751
	cmp	r1, r2
	bls	.L8
	fmrrd	r0, r1, d24
	ubfx	r2, r3, #0, #20
	orrs	r3, r2, r0
	fadddne	d24, d24, d24
	bne	.L1
	cmp	ip, #0
	beq	.L1
.L12:
	fldd	d24, .L25+96
	b	.L1
.L13:
	fldd	d17, .L25+104
	fconstd	d18, #112
	faddd	d0, d24, d17
	fcmped	d0, d18
	fmstat
	fadddgt	d24, d24, d18
	bgt	.L1
.L20:
	fmuld	d1, d24, d24
	fldd	d20, .L25+24
	fldd	d19, .L25+32
	fldd	d21, .L25+40
	fmscd	d19, d1, d20
	fldd	d2, .L25+48
	fmacd	d21, d19, d1
	fldd	d3, .L25+56
	fmscd	d2, d21, d1
	fcpyd	d5, d24
	fmacd	d3, d2, d1
	fconstd	d4, #0
	fnmacd	d5, d3, d1
	fmuld	d7, d24, d5
	fsubd	d22, d5, d4
	fdivd	d23, d7, d22
	fsubd	d16, d23, d24
	fsubd	d24, d18, d16
	b	.L1
.L24:
	fldd	d3, .L25+104
	fmuld	d24, d3, d3
	b	.L1
.L26:
	.align	3
.L25:
	.word	1697350398
	.word	1073157447
	.word	-18874368
	.word	1072049730
	.word	897137782
	.word	1038760431
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
	.word	897137782
	.word	-1108723217
	.word	-17155601
	.word	1082535490
	.word	-718458799
	.word	-1064875760
	.word	0
	.word	24117248
	.word	0
	.word	0
	.word	-2013235812
	.word	2117592124
	.word	.LANCHOR0-(.LPIC0+8)
	.word	.LANCHOR0-(.LPIC1+8)
	.fnend
	.size	exp, .-exp
	.section	.rodata.halF,"a",%progbits
	.align	3
.LANCHOR0 = . + 0
	.type	halF, %object
	.size	halF, 16
halF:
	.word	0
	.word	1071644672
	.word	0
	.word	-1075838976
#if (LDBL_MANT_DIG == 53)
    .weak   expl
    .equ    expl, exp
#endif
	.ident	"GCC: (crosstool-NG linaro-1.13.1-4.7-2012.10-20121022 - Linaro GCC 2012.10) 4.7.3 20121001 (prerelease)"
	.section	.note.GNU-stack,"",%progbits
