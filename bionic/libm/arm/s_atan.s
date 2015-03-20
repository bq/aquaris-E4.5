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
	.file	"s_atan.c"
	.section	.text.hot.atan,"ax",%progbits
	.align	2
	.global	atan
	.type	atan, %function
atan:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	fmdrr	d16, r0, r1
	movw	r0, #65535
	movt	r0, 16370
	fmrrd	r2, r3, d16
	fabsd	d18, d16
	bic	r1, r3, #-2147483648
	cmp	r1, r0
	ble	.L2
	fconstd	d0, #120
	fconstd	d5, #112
	movw	r0, #32767
	fsubd	d6, d18, d0
	fmacd	d5, d18, d0
	movt	r0, 16387
	cmp	r1, r0
	fdivd	d0, d6, d5
	bgt	.L3
	fmuld	d17, d0, d0
	fldd	d22, .L23
	fldd	d29, .L23+8
	fldd	d19, .L23+16
	fmuld	d28, d17, d17
	fldd	d18, .L23+24
	fldd	d30, .L23+32
	fldd	d16, .L23+40
	fmacd	d29, d28, d22
	fldd	d31, .L23+48
	fmscd	d18, d28, d19
	fldd	d1, .L23+56
	fldd	d19, .L23+64
	fldd	d2, .L23+72
	fmacd	d30, d29, d28
	fldd	d3, .L23+80
	fmscd	d16, d18, d28
	fldd	d4, .L23+88
	fldd	d18, .L23+96
.L18:
	fmacd	d31, d30, d28
	cmp	r3, #0
	fmscd	d19, d16, d28
	fmacd	d1, d31, d28
	fmscd	d2, d19, d28
	fmacd	d3, d1, d28
	fmuld	d28, d2, d28
	fmacd	d28, d3, d17
	fmscd	d18, d28, d0
	fsubd	d30, d18, d0
	fsubd	d16, d4, d30
	blt	.L11
	fmrrd	r0, r1, d16
	bx	lr
.L2:
	fconstd	d19, #112
	movw	r2, #65535
	movt	r2, 16357
	fsubd	d20, d18, d19
	faddd	d17, d18, d19
	cmp	r1, r2
	fdivd	d0, d20, d17
	ble	.L10
	fmuld	d17, d0, d0
	fldd	d7, .L23
	fldd	d20, .L23+8
	fldd	d27, .L23+16
	fmuld	d28, d17, d17
	fldd	d29, .L23+24
	fldd	d30, .L23+32
	fldd	d16, .L23+40
	fmacd	d20, d28, d7
	fldd	d31, .L23+48
	fmscd	d29, d28, d27
	fldd	d19, .L23+64
	fldd	d1, .L23+56
	fldd	d2, .L23+72
	fmacd	d30, d20, d28
	fldd	d3, .L23+80
	fmscd	d16, d29, d28
	fldd	d18, .L23+104
	fldd	d4, .L23+112
	b	.L18
.L10:
	movw	ip, #65535
	movt	ip, 16347
	cmp	r1, ip
	ble	.L12
	faddd	d30, d18, d18
	fconstd	d16, #0
	fldd	d31, .L23
	fsubd	d19, d30, d19
	faddd	d18, d18, d16
	fldd	d0, .L23+8
	fldd	d1, .L23+16
	fdivd	d2, d19, d18
	fldd	d3, .L23+24
	fldd	d4, .L23+32
	fldd	d5, .L23+40
	fldd	d6, .L23+48
	fldd	d26, .L23+64
	fldd	d25, .L23+56
	fldd	d24, .L23+72
	fldd	d21, .L23+80
	fldd	d23, .L23+120
	fldd	d22, .L23+128
.L20:
	fmuld	d7, d2, d2
	cmp	r3, #0
	fmuld	d17, d7, d7
	fmacd	d0, d17, d31
	fmscd	d3, d17, d1
	fmacd	d4, d0, d17
	fmscd	d5, d3, d17
	fmacd	d6, d4, d17
	fmscd	d26, d5, d17
	fmacd	d25, d6, d17
	fmscd	d24, d26, d17
	fmacd	d21, d25, d17
	fmuld	d20, d24, d17
	fmacd	d20, d21, d7
	fmscd	d23, d20, d2
	fsubd	d27, d23, d2
	fsubd	d16, d22, d27
	fnegdlt	d16, d16
.L5:
	fmrrd	r0, r1, d16
	bx	lr
.L11:
	fnegd	d16, d16
	b	.L5
.L3:
	movw	r2, #65535
	movt	r2, 17423
	cmp	r1, r2
	bgt	.L6
	fconstd	d23, #240
	fldd	d31, .L23
	fdivd	d2, d23, d18
	fldd	d0, .L23+8
	fldd	d1, .L23+16
	fldd	d3, .L23+24
	fldd	d4, .L23+32
	fldd	d5, .L23+40
	fldd	d6, .L23+48
	fldd	d26, .L23+64
	fldd	d25, .L23+56
	fldd	d24, .L23+72
	fldd	d21, .L23+80
	fldd	d23, .L23+136
	fldd	d22, .L23+144
	b	.L20
.L12:
	cmp	r1, #1044381696
	bge	.L13
	fldd	d1, .L23+152
	faddd	d2, d16, d1
	fcmped	d2, d19
	fmstat
	bgt	.L5
.L13:
	fmuld	d3, d16, d16
	fldd	d21, .L23
	fldd	d4, .L23+8
	fldd	d5, .L23+16
	fmuld	d6, d3, d3
	fldd	d26, .L23+24
	fldd	d25, .L23+32
	fldd	d24, .L23+40
	fmacd	d4, d6, d21
	fldd	d23, .L23+48
	fmscd	d26, d6, d5
	fldd	d22, .L23+64
	fldd	d7, .L23+56
	fldd	d27, .L23+72
	fmacd	d25, d4, d6
	fldd	d28, .L23+80
	fmscd	d24, d26, d6
	fmacd	d23, d25, d6
	fmscd	d22, d24, d6
	fmacd	d7, d23, d6
	fmscd	d27, d22, d6
	fmacd	d28, d7, d6
	fmuld	d29, d27, d6
	fmacd	d29, d28, d3
	fnmacd	d16, d29, d16
	b	.L5
.L6:
	mov	ip, #0
	movt	ip, 32752
	cmp	r1, ip
	fmrrd	r0, r1, d16
	bgt	.L7
	beq	.L22
.L8:
	cmp	r3, #0
	ble	.L9
	ldr	r0, .L23+168
	fldd	d24, .L23+144
.LPIC0:
	add	r2, pc, r0
	fldd	d21, [r2, #0]
	faddd	d16, d21, d24
	b	.L5
.L22:
	cmp	r0, #0
	beq	.L8
.L7:
	faddd	d16, d16, d16
	b	.L5
.L9:
	ldr	r3, .L23+172
	fldd	d26, .L23+160
.LPIC1:
	add	r1, pc, r3
	fldd	d25, [r1, #0]
	fsubd	d16, d26, d25
	b	.L5
.L24:
	.align	3
.L23:
	.word	-484255215
	.word	1066446138
	.word	611716587
	.word	1068071755
	.word	745172015
	.word	-1079856060
	.word	1390345626
	.word	1068359213
	.word	-1596965551
	.word	1068567910
	.word	-1351312787
	.word	1068740850
	.word	-984866706
	.word	1068975565
	.word	-1845459969
	.word	1069697316
	.word	-31254927
	.word	1069314502
	.word	-1718031420
	.word	1070176665
	.word	1431655693
	.word	1070945621
	.word	-763234661
	.word	1072657163
	.word	2062601149
	.word	1013974920
	.word	856972295
	.word	1015129638
	.word	1413754136
	.word	1072243195
	.word	573531618
	.word	1014639487
	.word	90291023
	.word	1071492199
	.word	856972295
	.word	1016178214
	.word	1413754136
	.word	1073291771
	.word	-2013235812
	.word	2117592124
	.word	1413754136
	.word	-1074191877
	.word	.LANCHOR0-(.LPIC0+8)
	.word	.LANCHOR0-(.LPIC1+8)
	.fnend
	.size	atan, .-atan
	.section	.rodata.atanlo_3,"a",%progbits
	.align	3
.LANCHOR0 = . + 0
	.type	atanlo_3, %object
	.size	atanlo_3, 8
atanlo_3:
	.word	856972295
	.word	1016178214
#if (LDBL_MANT_DIG == 53)
    .weak   atanl
    .equ    atanl, atan
#endif
	.ident	"GCC: (crosstool-NG linaro-1.13.1-4.7-2012.10-20121022 - Linaro GCC 2012.10) 4.7.3 20121001 (prerelease)"
	.section	.note.GNU-stack,"",%progbits
