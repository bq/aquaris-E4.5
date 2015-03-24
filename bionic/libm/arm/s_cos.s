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
	.file	"s_cos.c"
	.section	.text.hot.cos,"ax",%progbits
	.align	2
	.global	cos
	.type	cos, %function
cos:
	.fnstart
	@ args = 0, pretend = 0, frame = 40
	@ frame_needed = 0, uses_anonymous_args = 0
	fmdrr	d6, r0, r1
	stmfd	sp!, {r4, r5, r6, r7, lr}
	.save {r4, r5, r6, r7, lr}
	movw	r1, #8699
	movt	r1, 16361
	.pad #52
	sub	sp, sp, #52
	fmrrd	r2, r3, d6
	bic	r3, r3, #-2147483648
	cmp	r3, r1
	ble	.L54
	movw	r2, #65535
	movt	r2, 32751
	cmp	r3, r2
	bgt	.L55
	fmrrd	r4, r5, d6
	movw	r0, #27258
	movt	r0, 16399
	bic	r3, r5, #-2147483648
	cmp	r3, r0
	ble	.L56
	movw	ip, #17979
	movt	ip, 16412
	cmp	r3, ip
	ble	.L57
	movw	r1, #8698
	movt	r1, 16697
	cmp	r3, r1
	bgt	.L19
	fldd	d22, .L60
	fldd	d21, .L60+8
	fcpyd	d24, d22
	fcpyd	d26, d6
	fldd	d23, .L60+16
	fmacd	d24, d6, d21
	fldd	d25, .L60+24
	mov	r1, r3, asr #20
	fsubd	d17, d24, d22
	fnmacd	d26, d17, d23
	ftosizd	s3, d17
	fmuld	d27, d17, d25
	fmrs	r0, s3	@ int
	fsubd	d6, d26, d27
	fcpyd	d7, d26
	fmrrd	r2, r3, d6
	ubfx	r2, r3, #20, #11
	rsb	r3, r2, r1
	cmp	r3, #16
	bgt	.L20
	fsubd	d28, d26, d6
	and	r0, r0, #3
	cmp	r0, #1
	fsubd	d19, d28, d27
	beq	.L23
.L52:
	cmp	r0, #2
	beq	.L24
	cmp	r0, #0
	bne	.L21
.L22:
	fmuld	d2, d6, d6
	fldd	d3, .L60+32
	fldd	d4, .L60+40
	fldd	d5, .L60+48
	fmacd	d4, d2, d3
	fldd	d7, .L60+56
	fmscd	d7, d2, d5
	fldd	d21, .L60+64
	fmuld	d22, d2, d2
	fldd	d20, .L60+72
	fmscd	d21, d4, d2
	fconstd	d24, #96
	fconstd	d23, #112
	fmuld	d25, d22, d22
	fmacd	d20, d7, d2
	fmuld	d26, d25, d21
	fmuld	d27, d2, d24
	fmacd	d26, d20, d2
	fmuld	d6, d19, d6
	fsubd	d19, d23, d27
	fsubd	d28, d23, d19
	fmscd	d6, d2, d26
	fsubd	d29, d28, d27
	faddd	d30, d29, d6
	faddd	d1, d30, d19
.L1:
	fmrrd	r0, r1, d1
	add	sp, sp, #52
	ldmfd	sp!, {r4, r5, r6, r7, pc}
.L8:
	cmp	r5, #0
	fldd	d19, .L60+80
	fldd	d7, .L60+88
	ble	.L12
	fsubd	d22, d6, d19
	fsubd	d6, d22, d7
	fsubd	d23, d22, d6
	fsubd	d19, d23, d7
.L24:
	fmuld	d31, d6, d6
	fldd	d16, .L60+32
	fldd	d0, .L60+40
	fldd	d18, .L60+48
	fmacd	d0, d31, d16
	fldd	d17, .L60+56
	fmscd	d17, d31, d18
	fldd	d1, .L60+64
	fmuld	d2, d31, d31
	fldd	d3, .L60+72
	fmscd	d1, d0, d31
	fconstd	d4, #96
	fconstd	d5, #112
	fmuld	d7, d2, d2
	fmacd	d3, d17, d31
	fmuld	d22, d7, d1
	fmuld	d21, d31, d4
	fmacd	d22, d3, d31
	fmuld	d24, d19, d6
	fsubd	d20, d5, d21
	fsubd	d23, d5, d20
	fmscd	d24, d31, d22
	fsubd	d25, d23, d21
	faddd	d26, d25, d24
	faddd	d27, d26, d20
	fnegd	d1, d27
	b	.L1
.L56:
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
	fldd	d24, .L60+16
	fldd	d25, .L60+24
	ble	.L9
	fsubd	d28, d6, d24
	fsubd	d6, d28, d25
	fsubd	d29, d28, d6
	fsubd	d19, d29, d25
.L23:
	fmuld	d28, d6, d6
	fldd	d29, .L60+96
	fldd	d30, .L60+104
	fldd	d31, .L60+112
	fmscd	d30, d28, d29
	fldd	d16, .L60+120
	fldd	d0, .L60+128
	fmuld	d18, d28, d28
	fldd	d17, .L60+136
	fmscd	d0, d28, d16
	fconstd	d1, #96
	fmacd	d31, d30, d28
	fmuld	d2, d28, d18
	fmuld	d3, d28, d6
	fmacd	d31, d2, d0
	fmuld	d4, d3, d31
	fmscd	d4, d19, d1
	fmscd	d19, d4, d28
	fmacd	d19, d3, d17
	fsubd	d6, d6, d19
	fnegd	d1, d6
	b	.L1
.L57:
	movw	r1, #64956
	movt	r1, 16405
	cmp	r3, r1
	bgt	.L15
	movw	r0, #55676
	movt	r0, 16402
	cmp	r3, r0
	beq	.L7
	cmp	r5, #0
	fldd	d0, .L60+144
	fldd	d1, .L60+152
	ble	.L16
	fsubd	d4, d6, d0
	fsubd	d6, d4, d1
	fsubd	d5, d4, d6
	fsubd	d19, d5, d1
.L21:
	fmuld	d25, d6, d6
	fldd	d26, .L60+96
	fldd	d27, .L60+104
	fldd	d28, .L60+112
	fmscd	d27, d25, d26
	fldd	d29, .L60+120
	fldd	d30, .L60+128
	fmuld	d31, d25, d25
	fldd	d16, .L60+136
	fmscd	d30, d25, d29
	fconstd	d0, #96
	fmacd	d28, d27, d25
	fmuld	d18, d25, d31
	fmuld	d17, d25, d6
	fmacd	d28, d18, d30
	fmuld	d1, d17, d28
	fmscd	d1, d19, d0
	fmscd	d19, d1, d25
	fmacd	d19, d17, d16
	fsubd	d1, d6, d19
	b	.L1
.L7:
	fldd	d31, .L60
	fldd	d30, .L60+8
	fcpyd	d18, d31
	fldd	d16, .L60+16
	fldd	d0, .L60+24
	fmacd	d18, d6, d30
	mov	r1, r3, asr #20
	fsubd	d17, d18, d31
	fnmacd	d6, d17, d16
	ftosizd	s5, d17
	fmuld	d19, d17, d0
	fmrs	r0, s5	@ int
	fcpyd	d7, d6
	fsubd	d6, d6, d19
	fmrrd	r2, r3, d6
	ubfx	r3, r3, #20, #11
	rsb	r2, r3, r1
	cmp	r2, #16
	ble	.L25
.L20:
	fldd	d1, .L60+160
	fldd	d2, .L60+168
	fmuld	d3, d17, d1
	fsubd	d4, d7, d3
	fsubd	d5, d7, d4
	fsubd	d19, d5, d3
	fmscd	d19, d17, d2
	fsubd	d6, d4, d19
	fmrrd	r2, r3, d6
	ubfx	ip, r3, #20, #11
	rsb	r1, ip, r1
	cmp	r1, #49
	ble	.L31
	fldd	d20, .L60+176
	fldd	d21, .L60+184
	fmuld	d22, d17, d20
	fsubd	d7, d4, d22
	fsubd	d23, d4, d7
	fsubd	d19, d23, d22
	fmscd	d19, d17, d21
	fsubd	d6, d7, d19
.L25:
	fsubd	d24, d7, d6
	fsubd	d19, d24, d19
.L26:
	and	r0, r0, #3
	cmp	r0, #1
	bne	.L52
	b	.L23
.L15:
	movw	ip, #8699
	movt	ip, 16409
	cmp	r3, ip
	beq	.L7
	cmp	r5, #0
	fldd	d29, .L60+192
	fldd	d30, .L60+200
	ble	.L17
	fsubd	d18, d6, d29
	fsubd	d6, d18, d30
	fsubd	d17, d18, d6
	fsubd	d19, d17, d30
	b	.L22
.L54:
	movw	r2, #41117
	movt	r2, 15942
	cmp	r3, r2
	bgt	.L3
	ftosizd	s7, d6
	fmrs	r3, s7	@ int
	cmp	r3, #0
	bne	.L3
	fconstd	d1, #112
	b	.L1
.L31:
	fcpyd	d7, d4
	b	.L25
.L61:
	.align	3
.L60:
	.word	0
	.word	1127743488
	.word	1841940611
	.word	1071931184
	.word	1413480448
	.word	1073291771
	.word	442655537
	.word	1037087841
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
	.word	1413480448
	.word	1074340347
	.word	442655537
	.word	1038136417
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
	.word	-1098368812
	.word	-1112999191
	.word	432739728
	.word	1056571808
	.word	381768055
	.word	1062650220
	.word	-2137238867
	.word	1049787983
	.word	1431655756
	.word	1067799893
	.word	-1112231484
	.word	1042411166
	.word	0
	.word	0
	.word	0
	.word	1097859072
.L3:
	fmuld	d5, d6, d6
	fldd	d19, .L60+208
	fldd	d7, .L60+248
	fldd	d22, .L60+216
	fmacd	d7, d5, d19
	fldd	d21, .L60+224
	fmscd	d21, d5, d22
	fldd	d20, .L60+232
	fmuld	d23, d5, d5
	fldd	d25, .L60+240
	fmscd	d20, d7, d5
	fconstd	d24, #96
	fconstd	d26, #112
	fmuld	d27, d23, d23
	fldd	d28, .L60+256
	fmacd	d25, d21, d5
	fmuld	d29, d27, d20
	fmuld	d30, d5, d24
	fmacd	d29, d25, d5
	fmuld	d31, d6, d28
	fsubd	d0, d26, d30
	fsubd	d16, d26, d0
	fmscd	d31, d5, d29
	fsubd	d18, d16, d30
	faddd	d17, d18, d31
	faddd	d1, d17, d0
	b	.L1
.L16:
	faddd	d2, d6, d0
	faddd	d6, d2, d1
	fsubd	d3, d2, d6
	faddd	d19, d3, d1
	b	.L23
.L12:
	faddd	d20, d6, d19
	faddd	d6, d20, d7
	fsubd	d21, d20, d6
	faddd	d19, d21, d7
	b	.L24
.L9:
	faddd	d26, d6, d24
	faddd	d6, d26, d25
	fsubd	d27, d26, d6
	faddd	d19, d27, d25
	b	.L21
.L17:
	faddd	d31, d6, d29
	faddd	d6, d31, d30
	fsubd	d16, d31, d6
	faddd	d19, d16, d30
	b	.L22
.L19:
	cmp	r3, r2
	fsubdgt	d6, d6, d6
	fcpydgt	d19, d6
	bgt	.L22
	mov	r6, r3, asr #20
	sub	r7, r6, #1040
	sub	r2, r7, #6
	fmrs	r0, s12	@ int
	mov	lr, r2, asl #20
	rsb	r1, lr, r3
	fldd	d16, .L60+264
	fmdrr	d18, r0, r1
	fmrrd	r6, r7, d6
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
	bne	.L58
	mov	r3, r1
	b	.L28
.L55:
	fsubd	d1, d6, d6
	b	.L1
.L58:
	mov	lr, #1
	mov	r0, ip
	str	lr, [sp, #0]
	add	r1, sp, #8
	bl	__kernel_rem_pio2(PLT)
	cmp	r5, #0
	blt	.L59
	fldd	d6, [sp, #8]
	fldd	d19, [sp, #16]
	b	.L26
.L59:
	fldd	d7, [sp, #8]
	fldd	d20, [sp, #16]
	rsb	r0, r0, #0
	fnegd	d6, d7
	fnegd	d19, d20
	b	.L26
	.fnend
	.size	cos, .-cos
#if (LDBL_MANT_DIG == 53)
    .weak   cosl
    .equ    cosl, cos
#endif
	.ident	"GCC: (crosstool-NG linaro-1.13.1-4.7-2012.10-20121022 - Linaro GCC 2012.10) 4.7.3 20121001 (prerelease)"
	.section	.note.GNU-stack,"",%progbits
