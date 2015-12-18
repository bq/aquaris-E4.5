
/************************************************************************************
 ** File: - mmc3524x.h
 ** VENDOR_EDIT
 ** Copyright (C), 2008-2012, OPPO Mobile Comm Corp., Ltd
 ** 
 ** Description: 
 **      Sensor driver, we need to replace but compare because the difference is most.
 ** 
 ** Version: 0.1
 ** Date created: 10:28:00,10/18/2013
 ** Author: zhangqiang@Prd.BasicDrv
 ** 
 ** --------------------------- Revision History: --------------------------------
 ** 	<author>	                    <data>			<desc>
 ** 
 ************************************************************************************/
 

#ifndef __MMC3524X_H__
#define __MMC3524X_H__

#include <linux/ioctl.h>

#define MMC3524X_I2C_NAME		"mmc3524x"
#define CALIBRATION_DATA_SIZE	16

#define MMC3524X_DELAY_TM	10	/* ms */
#define MMC3524X_DELAY_SET	75	/* ms */
#define MMC3524X_DELAY_RESET     50     /* ms */
#define MMC3524X_DEFAULT_DELAY   100
#define MMC3524X_RETRY_COUNT	3
#define MMC3524X_SET_INTV	250
#define	MMC3524x_BUFSIZE  0x20

#define MMC3524X_OFFSET_X		32768
#define MMC3524X_OFFSET_Y		32768
#define MMC3524X_OFFSET_Z		32768
#define MMC3524X_SENSITIVITY_X		2048
#define MMC3524X_SENSITIVITY_Y		2048
#define MMC3524X_SENSITIVITY_Z		2048

#define MMC3524X_I2C_ADDR		0x30
#define MMC3524X_REG_CTRL		0x07 //control 0
#define MMC3524X_REG_BITS		0x08 //control 1
#define MMC3524X_REG_DATA		0x00//data 
#define MMC3524X_REG_DS			0x06//device status
#define MMC3524X_REG_PRODUCTID_0		0x10
#define MMC3524X_REG_PRODUCTID_1		0x20

#define MMC3524X_CTRL_TM			0x01//take measurment
#define MMC3524X_CTRL_CM			0x02//continue mode 
#define MMC3524X_CTRL_50HZ		0x00// set mode 00
#define MMC3524X_CTRL_25HZ		0x04// set mode 01
#define MMC3524X_CTRL_12HZ		0x08// set mode 10
#define MMC3524X_CTRL_NOBOOST            0x10//disable charge pump
#define MMC3524X_CTRL_SET  	        0x20
#define MMC3524X_CTRL_RESET              0x40
#define MMC3524X_CTRL_REFILL             0x80 //recharge pump,do before set/reset

#define MMC3524X_BITS_SLOW_16            0x00
#define MMC3524X_BITS_FAST_16            0x01
#define MMC3524X_BITS_14                 0x02

typedef struct mmc3524x_read_reg_str
{
	unsigned char start_register;
	unsigned char length;
	unsigned char reg_contents[10];
} read_reg_str ;


/* Use 'm' as magic number */
#define MMC3524X_IOM			'm'

/* IOCTLs for MMC3524X device */
#define MMC3524X_IOC_TM			_IO (MMC3524X_IOM, 0x00)
#define MMC3524X_IOC_SET			_IO (MMC3524X_IOM, 0x01)
#define MMC3524X_IOC_READ		_IOR(MMC3524X_IOM, 0x02, int[3])
#define MMC3524X_IOC_READXYZ		_IOR(MMC3524X_IOM, 0x03, int[3])
#define MMC3524X_IOC_RESET               _IO (MMC3524X_IOM, 0x04)
#define MMC3524X_IOC_NOBOOST             _IO (MMC3524X_IOM, 0x05)
#define MMC3524X_IOC_ID                  _IOR(MMC3524X_IOM, 0x06, short)
#define MMC3524X_IOC_DIAG                _IOR(MMC3524X_IOM, 0x14, int[1])

//
#define CONVERT_M			25
#define CONVERT_M_DIV				8192		
#define CONVERT_O			45
#define CONVERT_O_DIV		8192
#define MMC3524X_CTRL_RM    MMC3524X_CTRL_RESET
#define MMC3524X_RETRY_COUNT		3
#define MMC3524X_SET_INTV			250
#define MMC3524X_I2C_DELAY			2		// ms
#define MMC3524X_DELAY_TM			10		// ms
#define MMC3524X_DELAY_SET			75		// ms
#define MMC3524X_DELAY_RESET     	50  	// ms

#define CONVERT_GY		32
#define CONVERT_GY_DIV	32768

#define SENSOR_DATA_SIZE			8
#define CALIBRATION_DATA_SIZE		16
#define MMC3524X_BUFSIZE			0x20

#define POWER_ON					1
#define POWER_OFF					0
#define MMC3524X_SUCCESS			0
#define MMC3524X_I2C_ERR			-1


#endif /* __MMC3524X_H__ */

