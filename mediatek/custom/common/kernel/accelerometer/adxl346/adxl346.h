/* linux/drivers/hwmon/adxl346.c
 *
 * (C) Copyright 2008 
 * MediaTek <www.mediatek.com>
 *
 * ADXL346 driver for MT6516
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
#ifndef ADXL346_H
#define ADXL346_H
	 
#include <linux/ioctl.h>
	 
#define ADXL346_I2C_SLAVE_ADDR		0xA6
	 
	 /* ADXL346 Register Map  (Please refer to ADXL346 Specifications) */
#define ADXL346_REG_DEVID			0x00
#define ADXL346_REG_THRESH_TAP		0x1D
#define ADXL346_REG_OFSX			0x1E
#define ADXL346_REG_OFSY			0x1F
#define ADXL346_REG_OFSZ			0x20
#define ADXL346_REG_DUR				0x21
#define ADXL346_REG_THRESH_ACT		0x24
#define ADXL346_REG_THRESH_INACT	0x25
#define ADXL346_REG_TIME_INACT		0x26
#define ADXL346_REG_ACT_INACT_CTL	0x27
#define ADXL346_REG_THRESH_FF		0x28
#define ADXL346_REG_TIME_FF			0x29
#define ADXL346_REG_TAP_AXES		0x2A
#define ADXL346_REG_ACT_TAP_STATUS	0x2B
#define	ADXL346_REG_BW_RATE			0x2C
#define ADXL346_REG_POWER_CTL		0x2D
#define ADXL346_REG_INT_ENABLE		0x2E
#define ADXL346_REG_INT_MAP			0x2F
#define ADXL346_REG_INT_SOURCE		0x30
#define ADXL346_REG_DATA_FORMAT		0x31
#define ADXL346_REG_DATAX0			0x32
#define ADXL346_REG_FIFO_CTL		0x38
#define ADXL346_REG_FIFO_STATUS		0x39
	 
#define ADXL346_FIXED_DEVID			0xE6
	 
#define ADXL346_BW_200HZ			0x0C
#define ADXL346_BW_100HZ			0x0B
#define ADXL346_BW_50HZ				0x0A

#define	ADXL346_FULLRANG_LSB		0XFF
	 
#define ADXL346_MEASURE_MODE		0x08	
#define ADXL346_DATA_READY			0x80
	 
#define ADXL346_FULL_RES			0x08
#define ADXL346_RANGE_2G			0x00
#define ADXL346_RANGE_4G			0x01
#define ADXL346_RANGE_8G			0x02
#define ADXL346_RANGE_16G			0x03
#define ADXL346_SELF_TEST           0x80
	 
#define ADXL346_STREAM_MODE			0x80
#define ADXL346_SAMPLES_15			0x0F
	 
#define ADXL346_FS_8G_LSB_G			64
#define ADXL346_FS_4G_LSB_G			128
#define ADXL346_FS_2G_LSB_G			256
	 
#define ADXL346_LEFT_JUSTIFY		0x04
#define ADXL346_RIGHT_JUSTIFY		0x00
	 
	 
#define ADXL346_SUCCESS						0
#define ADXL346_ERR_I2C						-1
#define ADXL346_ERR_STATUS					-3
#define ADXL346_ERR_SETUP_FAILURE			-4
#define ADXL346_ERR_GETGSENSORDATA			-5
#define ADXL346_ERR_IDENTIFICATION			-6
	 
	 
	 
#define ADXL346_BUFSIZE				256
	 
#endif

