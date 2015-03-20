/* l3g4200d.h
 *
 * (C) Copyright 2008 
 * MediaTek <www.mediatek.com>
 *
 * mpu300 head file for MT65xx
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
#ifndef L3GD20_H
#define L3GD20_H
	 
#include <linux/ioctl.h>
	 
#define L3GD20_I2C_SLAVE_ADDR		0xD6     //SD0 high--->D6    SD0 low ----->D4
#define L3GD20_FIXED_DEVID			0xD4


/* L3GD20 Register Map  (Please refer to L3GD20 Specifications) */
#define L3GD20_REG_DEVID			         0x0F

/*
#define L3GD20_REG_DEVID			         0x00
#define L3GD20_REG_OFS_XH			0x0C
#define L3GD20_REG_OFS_XL		         0x0D
#define L3GD20_REG_OFS_YH			0x0E
#define L3GD20_REG_OFS_YL			0x0F
#define L3GD20_REG_OFS_ZH			0x10
#define L3GD20_REG_OFS_ZL			0x11
#define L3GD20_REG_FIFO_EN			0x12
#define L3GD20_REG_AUX_VDD			0x13
#define L3GD20_REG_AUX_ID		         0x14
#define L3GD20_REG_SAMRT_DIV		0x15
#define L3GD20_REG_DATA_FMT			0x16   //set external sync, full-scale range and sample rate, low pass filter bandwidth
#define L3GD20_REG_INT_EN			0x17
#define L3GD20_REG_AUX_ADDR   		0x18
#define L3GD20_REG_INT_STA			0x1A

#define L3GD20_REG_TEMP_OUTH		0x1B
#define L3GD20_REG_TEMP_OUTL	         0x1C
*/
#define L3GD20_REG_GYRO_XH			0x29
#define L3GD20_REG_GYRO_XL			0x28
//#define L3GD20_REG_GYRO_YH			0x1F
//#define L3GD20_REG_GYRO_YL			0x20
//#define L3GD20_REG_GYRO_ZH			0x21
//#define L3GD20_REG_GYRO_ZL			0x22
/*
#define L3GD20_REG_AUX_XH			0x23
#define L3GD20_REG_AUX_XL			0x24
#define L3GD20_REG_AUX_YH			0x25
#define L3GD20_REG_AUX_YL			0x26
#define L3GD20_REG_AUX_ZH			0x27
#define L3GD20_REG_AUX_ZL			0x28
#define L3GD20_REG_FIFO_CNTH		0x3A
#define L3GD20_REG_FIFO_CNTL		0x3B
#define L3GD20_REG_FIFO_DATA		0x3C
#define L3GD20_REG_FIFO_CTL			0x3D
*/
#define L3GD20_CTL_REG1			0x20 
#define L3GD20_FIFO_CTL			0x2E 

#define LSM330_CTL_REG2_G		0x21 
#define LSM330_CTL_REG5_G  	       0x24
#define L3GD20_CTL_REG4			0x23 
#define L3GD20_FIFO_SRC_REG		0x2F 
#define L3GD20_STATUS_REG		    0x27 

#define LSM330_HPF_CUTOFF_045              0X24
#define LSM330_HPF_CUTOFF_018              0X25
#define LSM330_HPF_CUTOFF_009              0X26
#define LSM330_HPF_CUTOFF_0018              0X28
	 

/*L3GD20 Register Bit definitions*/ 

#define L3GD20_FS_250_LSB			131	// LSB/(o/s)
#define L3GD20_FS_500_LSB			66 
#define L3GD20_FS_2000_LSB		14  //(14286/1000)    //16
#define L3GD20_OUT_MAGNIFY		131
#define L3GD20_RANGE_250		    0x00
#define L3GD20_RANGE_500		    0x10
#define L3GD20_RANGE_2000		    0x30

#define L3GD20_FIFO_MODE_BYPASS   0x00
#define L3GD20_FIFO_MODE_FIFO     0x20
#define L3GD20_FIFO_MODE_STREAM   0x40

#define AUTO_INCREMENT 0x80










#define L3GD20_SAM_RATE_MASK		    0x07	//set sample rate and low padd filter configuration
#define L3GD20_RATE_8K_LPFB_256HZ 	0x00
#define L3GD20_RATE_1K_LPFB_188HZ	0x01
#define L3GD20_RATE_1K_LPFB_98HZ 	0x02
#define L3GD20_RATE_1K_LPFB_42HZ 	0x03
#define L3GD20_RATE_1K_LPFB_20HZ 	0x04
#define L3GD20_RATE_1K_LPFB_10HZ 	0x05
#define L3GD20_RATE_1K_LPFB_5HZ 	0x06


#define L3GD20_POWER_ON			0x08	
#define L3GD20_100HZ 0x00 
#define L3GD20_200HZ 0x40 
#define L3GD20_400HZ 0x80
#define L3GD20_800HZ 0xC0 


	 
#define L3GD20_SUCCESS		       0
#define L3GD20_ERR_I2C		      -1
#define L3GD20_ERR_STATUS			  -3
#define L3GD20_ERR_SETUP_FAILURE	  -4
#define L3GD20_ERR_GETGSENSORDATA  -5
#define L3GD20_ERR_IDENTIFICATION	  -6

#define L3GD20_BUFSIZE 60

// 1 rad = 180/PI degree, L3GD20_OUT_MAGNIFY = 131,
// 180*131/PI = 7506
#define DEGREE_TO_RAD	7506

#define FAC_CALI_GYRO


#define  GYROSCOPE_IOC_GET_OFLAG  _IOR(GYROSCOPE, 0x0a, int)
#define  GYROSCOPE_IOC_GET_GRFLAG  _IOR(GYROSCOPE, 0x0b, int)
#define  GYROSCOPE_IOC_GET_RVFLAG  _IOR(GYROSCOPE, 0x0c, int)
#define  GYROSCOPE_IOC_GET_GYFLAG  _IOR(GYROSCOPE, 0x0d, int)
#define  GYROSCOPE_IOC_GET_LAFLAG  _IOR(GYROSCOPE, 0x0e, int)
#define  GYROSCOPE_IOC_GET_OPEN_STATUS  _IOR(GYROSCOPE, 0x0f, int)
#define  GYROSCOPE_IOC_GET_SUSPEND_STATUS  _IOR(GYROSCOPE, 0x018, int)

#define GYROSCOPE_IOCTL_SET_ORIENTATION        	      _IOW(GYROSCOPE, 0x10, int)
#define GYROSCOPE_IOCTL_READ_ORIENTATION_DATA		  _IOR(GYROSCOPE, 0x11, int)
#define GYROSCOPE_IOCTL_SET_ROTATION_VECTOR  	      _IOW(GYROSCOPE, 0x12, int)
#define GYROSCOPE_IOCTL_READ_ROTATION_VECTOR_DATA	  _IOR(GYROSCOPE, 0x13, int)
#define GYROSCOPE_IOCTL_SET_GRAVITY  	              _IOW(GYROSCOPE, 0x14, int)
#define GYROSCOPE_IOCTL_READ_GRAVITY_DATA	          _IOR(GYROSCOPE, 0x15, int)
#define GYROSCOPE_IOCTL_SET_LINEAR_ACC  	          _IOW(GYROSCOPE, 0x16, int)
#define GYROSCOPE_IOCTL_READ_LINEAR_ACC_DATA	      _IOR(GYROSCOPE, 0x17,int)


#define GYROSCOPE_IOCTL_CLEAR_DYNAMIC_CALI	      _IOR(GYROSCOPE, 0x30,int)
#define GYROSCOPE_IOCTL_SET_DYNAMIC_CALI	      _IOR(GYROSCOPE, 0x31,int)

#endif //L3GD20_H

