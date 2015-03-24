/*
 * (c) mCube, Inc. 2013
 */


#ifndef _MC3XXX_H_
    #define _MC3XXX_H_ 
	 
#include <linux/ioctl.h>

/***********************************************
 *** REGISTER MAP
 ***********************************************/
#define MC3XXX_REG_XOUT                    0x00
#define MC3XXX_REG_YOUT                    0x01
#define MC3XXX_REG_ZOUT                    0x02
#define MC3XXX_REG_TILT_STATUS             0x03
#define MC3XXX_REG_SAMPLE_RATE_STATUS      0x04
#define MC3XXX_REG_SLEEP_COUNT             0x05
#define MC3XXX_REG_INTERRUPT_ENABLE        0x06
#define MC3XXX_REG_MODE_FEATURE            0x07
#define MC3XXX_REG_SAMPLE_RATE             0x08
#define MC3XXX_REG_TAP_DETECTION_ENABLE    0x09
#define MC3XXX_REG_TAP_DWELL_REJECT        0x0A
#define MC3XXX_REG_DROP_CONTROL            0x0B
#define MC3XXX_REG_SHAKE_DEBOUNCE          0x0C
#define MC3XXX_REG_XOUT_EX_L               0x0D
#define MC3XXX_REG_XOUT_EX_H               0x0E
#define MC3XXX_REG_YOUT_EX_L               0x0F
#define MC3XXX_REG_YOUT_EX_H               0x10
#define MC3XXX_REG_ZOUT_EX_L               0x11
#define MC3XXX_REG_ZOUT_EX_H               0x12
#define MC3XXX_REG_RANGE_CONTROL           0x20
#define MC3XXX_REG_SHAKE_THRESHOLD         0x2B
#define MC3XXX_REG_UD_Z_TH                 0x2C
#define MC3XXX_REG_UD_X_TH                 0x2D
#define MC3XXX_REG_RL_Z_TH                 0x2E
#define MC3XXX_REG_RL_Y_TH                 0x2F
#define MC3XXX_REG_FB_Z_TH                 0x30
#define MC3XXX_REG_DROP_THRESHOLD          0x31
#define MC3XXX_REG_TAP_THRESHOLD           0x32
#define MC3XXX_REG_PRODUCT_CODE            0x3B

/***********************************************
 *** RETURN CODE
 ***********************************************/
#define MC3XXX_RETCODE_SUCCESS                 (0)
#define MC3XXX_RETCODE_ERROR_I2C               (-1)
#define MC3XXX_RETCODE_ERROR_NULL_POINTER      (-2)
#define MC3XXX_RETCODE_ERROR_STATUS            (-3)
#define MC3XXX_RETCODE_ERROR_SETUP             (-4)
#define MC3XXX_RETCODE_ERROR_GET_DATA          (-5)
#define MC3XXX_RETCODE_ERROR_IDENTIFICATION    (-6)

/***********************************************
 *** CONFIGURATION
 ***********************************************/
#define MC3XXX_BUF_SIZE    256

/***********************************************
 *** PRODUCT ID
 ***********************************************/
#define MC3XXX_PCODE_3210     0x90
#define MC3XXX_PCODE_3230     0x19
#define MC3XXX_PCODE_3250     0x88
#define MC3XXX_PCODE_3410     0xA8
#define MC3XXX_PCODE_3410N    0xB8
#define MC3XXX_PCODE_3430     0x29
#define MC3XXX_PCODE_3430N    0x39
#define MC3XXX_PCODE_3510B    0x40
#define MC3XXX_PCODE_3530B    0x30
#define MC3XXX_PCODE_3510C    0x10
#define MC3XXX_PCODE_3530C    0x60

#endif    // END OF _MC3XXX_H_



