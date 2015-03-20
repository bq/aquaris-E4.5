
/* Lite-On LTR-501ALS Linux Driver
 *
 * Copyright (C) 2011 Lite-On Technology Corp (Singapore)
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef _LTR559_H
#define _LTR559_H

#include <linux/ioctl.h>
/* LTR-559 Registers */
#define LTR559_ALS_CONTR	0x80
#define LTR559_PS_CONTR		0x81
#define LTR559_PS_LED		0x82
#define LTR559_PS_N_PULSES	0x83
#define LTR559_PS_MEAS_RATE	0x84
#define LTR559_ALS_MEAS_RATE	0x85
#define LTR559_MANUFACTURER_ID	0x87

#define LTR559_INTERRUPT	0x8F
#define LTR559_PS_THRES_UP_0	0x90
#define LTR559_PS_THRES_UP_1	0x91
#define LTR559_PS_THRES_LOW_0	0x92
#define LTR559_PS_THRES_LOW_1	0x93

#define LTR559_ALS_THRES_UP_0	0x97
#define LTR559_ALS_THRES_UP_1	0x98
#define LTR559_ALS_THRES_LOW_0	0x99
#define LTR559_ALS_THRES_LOW_1	0x9A

#define LTR559_INTERRUPT_PERSIST 0x9E

/* 559's Read Only Registers */
#define LTR559_ALS_DATA_CH1_0	0x88
#define LTR559_ALS_DATA_CH1_1	0x89
#define LTR559_ALS_DATA_CH0_0	0x8A
#define LTR559_ALS_DATA_CH0_1	0x8B
#define LTR559_ALS_PS_STATUS	0x8C
#define LTR559_PS_DATA_0	0x8D
#define LTR559_PS_DATA_1	0x8E


/* Basic Operating Modes */
#define MODE_ALS_ON_Range1	0x01  ///for als gain x1
#define MODE_ALS_ON_Range2	0x05  ///for als  gain x2
#define MODE_ALS_ON_Range3	0x09  ///for als  gain x4
#define MODE_ALS_ON_Range4	0x0D  ///for als gain x8
#define MODE_ALS_ON_Range5	0x19  ///for als gain x48
#define MODE_ALS_ON_Range6	0x1D  ///for als gain x96

#define MODE_ALS_StdBy		0x00

#define ALS_RANGE_64K	1
#define ALS_RANGE_32K 	2
#define ALS_RANGE_16K 	4
#define ALS_RANGE_8K 	8
#define ALS_RANGE_1300 48
#define ALS_RANGE_600 	96


#define MODE_PS_ON_Gain16	0x03
#define MODE_PS_ON_Gain32	0x0B
#define MODE_PS_ON_Gain64	0x0F

#define MODE_PS_StdBy		0x00

#define PS_RANGE16 	1
#define PS_RANGE32	4
#define PS_RANGE64 	8



/* 
 * Magic Number
 * ============
 * Refer to file ioctl-number.txt for allocation
 */
#define LTR559_IOCTL_MAGIC      'c'

/* IOCTLs for ltr559 device */
#define LTR559_IOCTL_PS_ENABLE		_IOW(LTR559_IOCTL_MAGIC, 0, char *)
#define LTR559_IOCTL_ALS_ENABLE		_IOW(LTR559_IOCTL_MAGIC, 1, char *)
#define LTR559_IOCTL_READ_PS_DATA	_IOR(LTR559_IOCTL_MAGIC, 2, char *)
#define LTR559_IOCTL_READ_PS_INT	_IOR(LTR559_IOCTL_MAGIC, 3, char *)
#define LTR559_IOCTL_READ_ALS_DATA	_IOR(LTR559_IOCTL_MAGIC, 4, char *)
#define LTR559_IOCTL_READ_ALS_INT	_IOR(LTR559_IOCTL_MAGIC, 5, char *)


/* Power On response time in ms */
#define PON_DELAY	600
#define WAKEUP_DELAY	10

#define ltr559_SUCCESS						0
#define ltr559_ERR_I2C						-1
#define ltr559_ERR_STATUS					-3
#define ltr559_ERR_SETUP_FAILURE				-4
#define ltr559_ERR_GETGSENSORDATA			-5
#define ltr559_ERR_IDENTIFICATION			-6




/* Interrupt vector number to use when probing IRQ number.
 * User changeable depending on sys interrupt.
 * For IRQ numbers used, see /proc/interrupts.
 */
#define GPIO_INT_NO	32

#endif
