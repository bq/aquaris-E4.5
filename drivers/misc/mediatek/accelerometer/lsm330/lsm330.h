/* linux/drivers/hwmon/LSM330.c
 *
 * (C) Copyright 2008 
 * MediaTek <www.mediatek.com>
 *
 * LSM330 driver for MT6516
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef LSM330_H
#define LSM330_H
	 
#include <linux/ioctl.h>

#define LSM330_I2C_SLAVE_ADDR		0x3A//0x3A//0x1E<-> SD0=GND;0x1D<-> SD0=High//jy
	 
	 /* LSM330 Register Map  (Please refer to LSM330 Specifications) */
// ËÕ ÓÂ 2014Äê04ÔÂ02ÈÕ 15:12:38#define L3GD20_CTL_REG1			0x20 
#define L3GD20_POWER_ON			0x08	

#define LSM330_REG_CTL_REG5		0x20
#define LSM330_REG_CTL_REG6		0x24
#define LSM330_REG_CTL_REG4		0x23

//#define LSM330_REG_CTL_REG2		0x21
//#define LSM330_REG_CTL_REG3		0x22

#define LSM330_REG_DATAX0		    0x28
#define LSM330_REG_OUT_X		    0x28
#define LSM330_REG_OUT_Y		    0x2A
#define LSM330_REG_OUT_Z		    0x2C

#define LSM330_REG_DEVID			0x0F
#define WHO_AM_I 					0x40

/*
#define LSM330_REG_THRESH_TAP		0x1D
#define LSM330_REG_OFSX			0x1E
#define LSM330_REG_OFSY			0x1F
#define LSM330_REG_OFSZ			0x20
#define LSM330_REG_DUR				0x21
#define LSM330_REG_THRESH_ACT		0x24
#define LSM330_REG_THRESH_INACT	0x25
#define LSM330_REG_TIME_INACT		0x26
#define LSM330_REG_ACT_INACT_CTL	0x27
#define LSM330_REG_THRESH_FF		0x28
#define LSM330_REG_TIME_FF			0x29
#define LSM330_REG_TAP_AXES		0x2A
#define LSM330_REG_ACT_TAP_STATUS	0x2B
#define	LSM330_REG_BW_RATE			0x2C
#define LSM330_REG_POWER_CTL		0x2D
#define LSM330_REG_INT_ENABLE		0x2E
#define LSM330_REG_INT_MAP			0x2F
#define LSM330_REG_INT_SOURCE		0x30
#define LSM330_REG_DATA_FORMAT		0x31
#define LSM330_REG_DATAX0			0x32
#define LSM330_REG_FIFO_CTL		0x38
#define LSM330_REG_FIFO_STATUS		0x39
*/

//#define LSM330_FIXED_DEVID			0xE5
	 
#define LSM330_BW_400HZ			0x70
#define LSM330_BW_100HZ			0x60 //400 or 100 on other choise //changed
#define LSM330_BW_50HZ				0x50

#define	LSM330_FULLRANG_LSB		0XFF
	 
#define LSM330_MEASURE_MODE		0x08	//changed 
#define LSM330_DATA_READY			0x07    //changed
	 
//#define LSM330_FULL_RES			0x08
#define LSM330_RANGE_2G			0x00
#define LSM330_RANGE_4G			0x08
#define LSM330_RANGE_6G			0x10

#define LSM330_RANGE_8G			0x18 //8g or 2g no ohter choise//changed
#define LSM330_RANGE_16G			0x20 //8g or 2g no ohter choise//changed

#define LSM330_SELF_TEST           0x10 //changed
	 
#define LSM330_STREAM_MODE			0x80
#define LSM330_SAMPLES_15			0x0F
/*
#define LSM330_FS_16G_LSB_G			0x20	 
#define LSM330_FS_8G_LSB_G			0x18
#define LSM330_FS_6G_LSB_G			0x10

#define LSM330_FS_4G_LSB_G			0x08
#define LSM330_FS_2G_LSB_G			0x00
*/	 
#define LSM330_LEFT_JUSTIFY		0x04
#define LSM330_RIGHT_JUSTIFY		0x00
	 
	 
#define LSM330_SUCCESS						0
#define LSM330_ERR_I2C						-1
#define LSM330_ERR_STATUS					-3
#define LSM330_ERR_SETUP_FAILURE			-4
#define LSM330_ERR_GETGSENSORDATA			-5
#define LSM330_ERR_IDENTIFICATION			-6
	 
	 
	 
#define LSM330_BUFSIZE				256
	 
#endif

