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
	.file	"s_sin.c"
	.section	.text.hot.sin,"ax",%progbits
	.align	2
	.global	sin
	.type	sin, %function
sin:
	.fnstart
	@ args = 0, pretend = 0, frame = 40
	@ frame_needed = 0, uses_anonymous_args = 0
	fmdrr	d16, r0, r1
	stmfd	sp!, {r4, r5, r6, r7, lr}
	.save {r4, r5, r6, r7, lr}
	movw	r1, #8699
	movt	r1, 16361
	.pad #52
	sub	sp, sp, #52
	fmrrd	r2, r3, d16
	movw	r2, #65535
	bic	r3, r3, #-2147483648
	cmp	r3, r1
	ble	.L56
	movt	r2, 32751
	cmp	r3, r2
	bgt	.L57
	fmrrd	r4, r5, d16
	movw	r0, #27258
	movt	r0, 16399
	bic	r3, r5, #-2147483648
	cmp	r3, r0
	ble	.L58
	movw	ip, #17979
	movt	ip, 16412
	cmp	r3, ip
	ble	.L59
	movw	r1, #8698
	movt	r1, 16697
	cmp	r3, r1
	bgt	.L19
	fldd	d21, .L62
	fldd	d7, .L62+8
	fcpyd	d23, d21
	fcpyd	d25, d16
	fldd	d22, .L62+16
	fmacd	d23, d16, d7
	fldd	d24, .L62+24
	mov	r1, r3, asr #20
	fsubd	d17, d23, d21
	fnmacd	d25, d17, d22
	ftosizd	s3, d17
	fmuld	d26, d17, d24
	fmrs	r0, s3	@ int
	fsubd	d16, d25, d26
	fcpyd	d6, d25
	fmrrd	r2, r3, d16
	ubfx	r2, r3, #20, #11
	rsb	r3, r2, r1
	cmp	r3, #16
	bgt	.L20
	fsubd	d27, d25, d16
	and	r0, r0, #3
	cmp	r0, #1
	fsubd	d19, d27, d26
	beq	.L23
.L54:
	cmp	r0, #2
	beq	.L24
	cmp	r0, #0
	bne	.L21
.L22:
	fmuld	d7, d16, d16
	fldd	d20, .L62+32
	fldd	d21, .L62+40
	fldd	d25, .L62+48
	fmscd	d21, d7, d20
	fldd	d23, .L62+56
	fldd	d26, .L62+64
	fmuld	d24, d7, d7
	fldd	d22, .L62+72
	fmscd	d26, d7, d23
	fconstd	d27, #96
	fmacd	d25, d21, d7
	fmuld	d28, d7, d24
	fmuld	d29, d7, d16
	fmacd	d25, d28, d26
	fmuld	d30, d29, d25
	fmscd	d30, d19, d27
	fmscd	d19, d30, d7
	fmacd	d19, d29, d22
	fsubd	d16, d16, d19
.L1:
	fmrrd	r0, r1, d16
	add	sp, sp, #52
	ldmfd	sp!, {r4, r5, r6, r7, pc}
.L8:
	cmp	r5, #0
	fldd	d19, .L62+80
	fldd	d6, .L62+88
	ble	.L12
	fsubd	d21, d16, d19
	fsubd	d16, d21, d6
	fsubd	d22, d21, d16
	fsubd	d19, d22, d6
.L24:
	fmuld	d31, d16, d16
	fldd	d0, .L62+32
	fldd	d17, .L62+40
	fldd	d18, .L62+48
	fmscd	d17, d31, d0
	fldd	d1, .L62+56
	fldd	d2, .L62+64
	fmuld	d3, d31, d31
	fldd	d4, .L62+72
	fmscd	d2, d31, d1
	fconstd	d5, #96
	fmacd	d18, d17, d31
	fmuld	d6, d31, d3
	fmuld	d7, d31, d16
	fmacd	d18, d6, d2
	fmuld	d20, d7, d18
	fmscd	d20, d19, d5
	fmscd	d19, d20, d31
	fmacd	d19, d7, d4
	fsubd	d19, d16, d19
	fnegd	d16, d19
	b	.L1
.L58:
	movw	r2, #8699
	movt	r2, 9
	ubfx	r1, r5, #0, #20
	cmp	r1, r2
	beq	.L7
	movw	ip, #55676
	movt	ip, 16386
	cmp	r3, ip
	bgt	.L8
	cmp	r5, #0
	fldd	d23, .L62+16
	fldd	d24, .L62+24
	ble	.L9
	fsubd	d27, d16, d23
	fsubd	d16, d27, d24
	fsubd	d28, d27, d16
	fsubd	d19, d28, d24
.L23:
	fmuld	d25, d16, d16
	fldd	d21, .L62+96
	fldd	d23, .L62+104
	fldd	d26, .L62+112
	fmacd	d23, d25, d21
	fldd	d22, .L62+120
	fmscd	d22, d25, d26
	fldd	d27, .L62+128
	fmuld	d28, d25, d25
	fldd	d29, .L62+136
	fmscd	d27, d23, d25
	fconstd	d24, #96
	fconstd	d30, #112
	fmuld	d31, d28, d28
	fmacd	d29, d22, d25
	fmuld	d0, d31, d27
	fmuld	d17, d25, d24
	fmacd	d0, d29, d25
	fmuld	d18, d19, d16
	fsubd	d1, d30, d17
	fsubd	d16, d30, d1
	fmscd	d18, d25, d0
	fsubd	d2, d16, d17
	faddd	d3, d2, d18
	faddd	d16, d3, d1
	b	.L1
.L59:
	movw	r1, #64956
	movt	r1, 16405
	cmp	r3, r1
	bgt	.L15
	movw	r0, #55676
	movt	r0, 16402
	cmp	r3, r0
	beq	.L7
	cmp	r5, #0
	fldd	d0, .L62+144
	fldd	d1, .L62+152
	ble	.L16
	fsubd	d4, d16, d0
	fsubd	d16, d4, d1
	fsubd	d5, d4, d16
	fsubd	d19, d5, d1
.L21:
	fmuld	d25, d16, d16
	fldd	d24, .L62+96
	fldd	d26, .L62+104
	fldd	d27, .L62+112
	fmacd	d26, d25, d24
	fldd	d28, .L62+120
	fmscd	d28, d25, d27
	fldd	d29, .L62+128
	fmuld	d30, d25, d25
	fldd	d31, .L62+136
	fmscd	d29, d26, d25
	fconstd	d0, #96
	fconstd	d17, #112
	fmuld	d18, d30, d30
	fmacd	d31, d28, d25
	fmuld	d1, d18, d29
	fmuld	d2, d25, d0
	fmacd	d1, d31, d25
	fmuld	d3, d19, d16
	fsubd	d4, d17, d2
	fsubd	d16, d17, d4
	fmscd	d3, d25, d1
	fsubd	d5, d16, d2
	faddd	d19, d5, d3
	faddd	d6, d19, d4
	fnegd	d16, d6
	b	.L1
.L7:
	fldd	d30, .L62
	fldd	d29, .L62+8
	fcpyd	d18, d30
	fldd	d31, .L62+16
	fldd	d0, .L62+24
	fmacd	d18, d16, d29
	mov	r1, r3, asr #20
	fsubd	d17, d18, d30
	fnmacd	d16, d17, d31
	ftosizd	s5, d17
	fmuld	d19, d17, d0
	fmrs	r0, s5	@ int
	fcpyd	d6, d16
	fsubd	d16, d16, d19
	fmrrd	r2, r3, d16
	ubfx	r3, r3, #20, #11
	rsb	r2, r3, r1
	cmp	r2, #16
	ble	.L25
.L20:
	fldd	d1, .L62+160
	fldd	d2, .L62+168
	fmuld	d3, d17, d1
	fsubd	d4, d6, d3
	fsubd	d5, d6, d4
	fsubd	d19, d5, d3
	fmscd	d19, d17, d2
	fsubd	d16, d4, d19
	fmrrd	r2, r3, d16
	ubfx	ip, r3, #20, #11
	rsb	r1, ip, r1
	cmp	r1, #49
	ble	.L30
	fldd	d20, .L62+176
	fldd	d7, .L62+184
	fmuld	d21, d17, d20
	fsubd	d6, d4, d21
	fsubd	d22, d4, d6
	fsubd	d19, d22, d21
	fmscd	d19, d17, d7
	fsubd	d16, d6, d19
.L25:
	fsubd	d23, d6, d16
	fsubd	d19, d23, d19
.L26:
	and	r0, r0, #3
	cmp	r0, #1
	bne	.L54
	b	.L23
.L15:
	movw	ip, #8699
	movt	ip, 16409
	cmp	r3, ip
	beq	.L7
	cmp	r5, #0
	fldd	d28, .L62+192
	fldd	d29, .L62+200
	ble	.L17
	fsubd	d18, d16, d28
	fsubd	d16, d18, d29
	fsubd	d17, d18, d16
	fsubd	d19, d17, d29
	b	.L22
.L63:
	.align	3
.L62:
	.word	0
	.word	1127743488
	.word	1841940611
	.word	1071931184
	.word	1413480448
	.word	1073291771
	.word	442655537
	.word	1037087841
	.word	1471282813
	.word	1053236707
	.word	432103893
	.word	1059717536
	.word	286324902
	.word	1065423121
	.word	1523570044
	.word	1038473530
	.word	-1976853269
	.word	1046144486
	.word	1431655753
	.word	1069897045
	.word	1413480448
	.word	1074340347
	.word	442655537
	.word	1038136417
	.word	-1098368812
	.word	-1112999191
	.word	-1112231484
	.word	1042411166
	.word	432739728
	.word	1056571808
	.word	381768055
	.word	1062650220
	.word	-2137238867
	.word	1049787983
	.word	1431655756
	.word	1067799893
	.word	2133852160
	.word	1074977148
	.word	-1483500342
	.word	1038683793
	.word	442499072
	.word	1037087841
	.word	771977331
	.word	1000544650
	.word	771751936
	.word	1000544650
	.word	622873025
	.word	964395930
	.word	1413480448
	.word	1075388923
	.word	442655537
	.word	1039184993
	.word	1471282813
	.word	1053236707
	.word	286324902
	.word	1065423121
	.word	1523570044
	.word	1038473530
	.word	-1976853269
	.word	1046144486
	.word	1431655753
	.word	1069897045
	.word	432103893
	.word	1059717536
	.word	0
	.word	1097859072
.L56:
	movt	r2, 15951
	cmp	r3, r2
	bgt	.L3
	ftosizd	s7, d16
	fmrs	r3, s7	@ int
	cmp	r3, #0
	beq	.L1
.L3:
	fmuld	d4, d16, d16
	fldd	d5, .L62+208
	fldd	d6, .L62+248
	fldd	d7, .L62+216
	fmscd	d6, d4, d5
	fldd	d25, .L62+224
	fldd	d20, .L62+232
	fmuld	d21, d4, d4
	fldd	d19, .L62+240
	fmscd	d20, d4, d25
	fmacd	d7, d6, d4
	fmuld	d23, d4, d21
	fmuld	d26, d4, d16
	fmacd	d7, d23, d20
	fmscd	d19, d4, d7
	fmacd	d16, d19, d26
	b	.L1
.L30:
	fcpyd	d6, d4
	b	.L25
.L16:
	faddd	d2, d16, d0
	faddd	d16, d2, d1
	fsubd	d3, d2, d16
	faddd	d19, d3, d1
	b	.L23
.L12:
	faddd	d20, d16, d19
	faddd	d16, d20, d6
	fsubd	d7, d20, d16
	faddd	d19, d7, d6
	b	.L24
.L9:
	faddd	d25, d16, d23
	faddd	d16, d25, d24
	fsubd	d26, d25, d16
	faddd	d19, d26, d24
	b	.L21
.L17:
	faddd	d30, d16, d28
	faddd	d16, d30, d29
	fsubd	d31, d30, d16
	faddd	d19, d31, d29
	b	.L22
.L19:
	cmp	r3, r2
	fsubdgt	d16, d16, d16
	fcpydgt	d19, d16
	bgt	.L22
	mov	r6, r3, asr #20
	sub	r7, r6, #1040
	sub	r2, r7, #6
	fmrrd	r6, r7, d16
	mov	lr, r2, asl #20
	rsb	r1, lr, r3
	mov	r0, r6
	fldd	d16, .L62+256
	fmdrr	d18, r0, r1
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
.L28:
	sub	r1, r3, #1
	add	ip, sp, #24
	add	r0, ip, r1, asl #3
	fldd	d5, [r0, #0]
	fcmpzd	d5
	fmstat
	bne	.L60
	mov	r3, r1
	b	.L28
.L57:
	fsubd	d16, d16, d16
	b	.L1
.L60:
	mov	lr, #1
	mov	r0, ip
	str	lr, [sp, #0]
	add	r1, sp, #8
	bl	__kernel_rem_pio2(PLT)
	cmp	r5, #0
	blt	.L61
	fldd	d16, [sp, #8]
	fldd	d19, [sp, #16]
	b	.L26
.L61:
	fldd	d6, [sp, #8]
	fldd	d20, [sp, #16]
	rsb	r0, r0, #0
	fnegd	d16, d6
	fnegd	d19, d20
	b	.L26
	.fnend
	.size	sin, .-sin
#if (LDBL_MANT_DIG == 53)
    .weak   sinl
    .equ    sinl, sin
#endif
	.ident	"GCC: (crosstool-NG linaro-1.13.1-4.7-2012.10-20121022 - Linaro GCC 2012.10) 4.7.3 20121001 (prerelease)"
	.section	.note.GNU-stack,"",%progbits
