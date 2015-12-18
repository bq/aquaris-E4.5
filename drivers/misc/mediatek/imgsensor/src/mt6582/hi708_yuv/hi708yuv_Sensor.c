/*
 * (c) MediaTek Inc. 2010
 */

 *
 * Filename:
 * ---------
 *   sensor.c
 *
 * Project:
 * --------
 *   DUMA
 *
 * Description:
 * ------------
 *   Source code of Sensor driver
 *
 *
 * Author:
 * -------
 *   PC Huang (MTK02204)
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Revision:$
 * $Modtime:$
 * $Log:$
 *
 * 07 11 2011 jun.pei
 * [ALPS00059464] hi708 sensor check in
 * .
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
//#include <mach/mt6516_pll.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"

#include "hi708yuv_Sensor.h"
#include "hi708yuv_Camera_Sensor_para.h"
#include "hi708yuv_CameraCustomized.h"

#define HI708YUV_DEBUG
#ifdef HI708YUV_DEBUG
#define SENSORDB printk
#else
 
#define SENSORDB(x,...)
#endif

#if 0
extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);
static int sensor_id_fail = 0; 
#define HI708_write_cmos_sensor(addr, para) iWriteReg((u16) addr , (u32) para ,1,HI708_WRITE_ID)
#define HI708_write_cmos_sensor_2(addr, para, bytes) iWriteReg((u16) addr , (u32) para ,bytes,HI708_WRITE_ID)
kal_uint16 HI708_read_cmos_sensor(kal_uint32 addr)
{
kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,HI708_WRITE_ID);
    return get_byte;
}

#endif

#if defined(DTV_NMI5625) || defined(ATV_NMI168H)
extern bool g_bIsAtvStart;
#endif

static DEFINE_SPINLOCK(hi708_yuv_drv_lock);

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
kal_uint16 HI708_write_cmos_sensor(kal_uint8 addr, kal_uint8 para)
{
    char puSendCmd[2] = {(char)(addr & 0xFF) , (char)(para & 0xFF)};
    iWriteRegI2C(puSendCmd , 2,HI708_WRITE_ID);
    return 0;
}
kal_uint16 HI708_read_cmos_sensor(kal_uint8 addr)
{
    kal_uint16 get_byte=0;
    char puSendCmd = { (char)(addr & 0xFF) };
    iReadRegI2C(&puSendCmd , 1, (u8*)&get_byte,1,HI708_WRITE_ID);
    return get_byte;
}


/*******************************************************************************
* // Adapter for Winmo typedef 
********************************************************************************/
#define WINMO_USE 0

#define Sleep(ms) mdelay(ms)
#define RETAILMSG(x,...)
#define TEXT


/*******************************************************************************
* follow is define by jun
********************************************************************************/
MSDK_SENSOR_CONFIG_STRUCT HI708SensorConfigData;

static struct HI708_sensor_STRUCT HI708_sensor;
static kal_uint32 HI708_zoom_factor = 0; 
static kal_bool HI708_night_flag = 0;
static int sensor_id_fail = 0;	
const HI708_SENSOR_INIT_INFO HI708_Initial_Setting_Info[] =
{
  
    //PAGE 0
    //Image Size/Windowing/HSYNC/VSYNC[Type1]
	 {0x03, 0x00},   //PAGEMODE(0x03)
	{0x01, 0xf1},
	{0x01, 0xf3},   //PWRCTL(0x01[P0])Bit[1]:Software Reset.
	{0x01, 0xf1},

	{0x03, 0x00},//fuzr1 2012-07-09 require by vendor add
	{0x10, 0x00},//fuzr1 2012-07-09 require by vendor add
	{0x11, 0x90},   //For No Fixed Framerate Bit[2]
	{0x12, 0x05},  //PCLK INV

	{0x20, 0x00},
	{0x21, 0x04},
	{0x22, 0x00},
	{0x23, 0x04},

	{0x24, 0x01},
	{0x25, 0xe0},
	{0x26, 0x02},
	{0x27, 0x80},

	{0x40, 0x01},   //HBLANK: 0x70 = 112
	{0x41, 0x58},
	{0x42, 0x00},   //VBLANK: 0x40 = 64
	{0x43, 0x14},   //0x04 -> 0x40: For Max Framerate = 30fps

	//BLC
	{0x80, 0x2e},
	{0x81, 0x7e},
	{0x82, 0x90},
	{0x83, 0x30},
	{0x84, 0x2c},
	{0x85, 0x4b},
	{0x89, 0x48},//BLC hold//48

	{0x90, 0x0e}, //BLC_TIME_TH_ON
	{0x91, 0x0e}, //BLC_TIME_TH_OFF 
	{0x92, 0x48}, //BLC_AG_TH_ON
	{0x93, 0x38}, //BLC_AG_TH_OFF

	{0x98, 0x38},//0x20 --> 0x38	_100323
	{0x99, 0x40}, //Out BLC
	{0xa0, 0x00}, //Dark BLC
	{0xa8, 0x40}, //Normal BLC

	//Page 2  Last Update 10_03_02
	{0x03, 0x02},
	{0x20, 0x33},
	{0x21, 0x77},
	{0x22, 0xa7},
	{0x23, 0x30},
	{0x52, 0xa2},
	{0x55, 0x18},
	{0x56, 0x0c},
	{0x60, 0x11},
	{0x61, 0x1b},
	{0x62, 0x11},
	{0x63, 0x1a},
	{0x64, 0x11},
	{0x65, 0x1a},
	{0x72, 0x12},
	{0x73, 0x19},
	{0x74, 0x12},
	{0x75, 0x19},
	{0x80, 0x1d},
	{0x81, 0x6f},
	{0x82, 0x1e},
	{0x83, 0x2b},
	{0x84, 0x1e},
	{0x85, 0x2b},
	{0x92, 0x45},
	{0x93, 0x52},
	{0x94, 0x45},
	{0x95, 0x52},
	{0xa0, 0x1d},
	{0xa1, 0x6b},
	{0xa4, 0x6b},
	{0xa5, 0x1d},
	{0xa8, 0x2e},
	{0xa9, 0x42},
	{0xaa, 0x55},
	{0xab, 0x69},
	{0xb8, 0x10},
	{0xb9, 0x13},
	{0xbc, 0x1d},
	{0xbd, 0x1f},
	{0xc0, 0x04},
	{0xc1, 0x0d},
	{0xc4, 0x05},
	{0xc5, 0x0c},
	{0xc8, 0x06},
	{0xc9, 0x0b},
	{0xcc, 0x06},
	{0xcd, 0x0a},
	{0xc2, 0x04},
	{0xc3, 0x0d},
	{0xc6, 0x05},
	{0xc7, 0x0c},
	{0xca, 0x06},
	{0xcb, 0x0b},
	{0xce, 0x06},
	{0xcf, 0x0a},
	{0xd0, 0x03},
	{0xd1, 0x1c},
	{0xd6, 0x46},
	{0xd7, 0x48},

	//PAGE 10
	//Image Format, Image Effect
	{0x03, 0x10},
	{0x10, 0x03},
	{0x11, 0x43},
	{0x12, 0x30}, 
{0x40, 0x84},
{0x41, 0x00}, //00 DYOFS
{0x48, 0x8a}, //Contrast
	{0x50, 0x48}, //AGBRT

	{0x60, 0x67},
	{0x61, 0x00}, //
	{0x62, 0x80}, //SATB //a0
	{0x63, 0x90}, //SATR //70
	{0x64, 0x48},
	{0x66, 0x90},
	{0x67, 0x66}, //wht_gain 34  

	//PAGE 11
	//Z-LPF
	{0x03, 0x11},
	{0x10, 0x25},   
	{0x11, 0x1f},   
	{0x20, 0x00},	//LPF_AUTO_CTL
	{0x21, 0x38},	//LPF_PGA_TH
	{0x23, 0x10},	//Test Setting
	{0x60, 0x14},	//ZARA_SIGMA_TH //40->10
	{0x61, 0xa2}, //82
	{0x62, 0x00},   
	{0x63, 0x83},   
	{0x64, 0x83},      
	{0x67, 0xa0},   
	{0x68, 0x40},   
	{0x69, 0x10},   

	//PAGE 12
	//2D
	{0x03, 0x12},

	{0x40, 0xeb},
	{0x41, 0x09},

	{0x50, 0x18},
	{0x51, 0x44},

	{0x70, 0x1f},
	{0x71, 0x00},
	{0x72, 0x00},
	{0x73, 0x00},
	{0x74, 0x10},
	{0x75, 0x10},
	{0x76, 0x20},
	{0x77, 0x80},
	{0x78, 0x88},
	{0x79, 0x18},

	{0xb0, 0x7d},

	//PAGE 13
	//Edge Enhancement
	{0x03, 0x13},
	{0x10, 0x01},   
	{0x11, 0x89},   
	{0x12, 0x14},   
	{0x13, 0x19},   
	{0x14, 0x08},

	{0x20, 0x07}, //06
	{0x21, 0x05}, //03
	{0x23, 0x30},
	{0x24, 0x33},
	{0x25, 0x08},
	{0x26, 0x18},
	{0x27, 0x00},
	{0x28, 0x08},
	{0x29, 0x50},
	{0x2a, 0xe0},
	{0x2b, 0x10},
	{0x2c, 0x28},
	{0x2d, 0x40},
	{0x2e, 0x00},
	{0x2f, 0x00},

	//PAGE 11
	{0x30, 0x11},

	{0x80, 0x03},
	{0x81, 0x07},

	{0x90, 0x07},
	{0x91, 0x05},
	{0x92, 0x00},
	{0x93, 0x20},
	{0x94, 0x42},
	{0x95, 0x60},

	//PAGE 14
	//Lens Shading Correction
	{0x03, 0x14},
	{0x10, 0x01},

{0x20, 0x60}, //XCEN
{0x21, 0x80}, //YCEN
{0x22, 0x76}, //76, 34, 2b
{0x23, 0x50}, //4b, 15, 0d
{0x24, 0x44}, //3b, 10, 0b

  //PAGE 15
	//CMC
	{0x03, 0x15}, 
	{0x10, 0x03},         
	{0x14, 0x3c},
	{0x16, 0x2c},
	{0x17, 0x2f},

	{0x30, 0xcb},
	{0x31, 0x61},
	{0x32, 0x16},
	{0x33, 0x23},
	{0x34, 0xce},
	{0x35, 0x2b},
	{0x36, 0x01},
	{0x37, 0x34},
	{0x38, 0x75},

	{0x40, 0x87},
	{0x41, 0x18},
	{0x42, 0x91},
	{0x43, 0x94},
	{0x44, 0x9f},
	{0x45, 0x33},
	{0x46, 0x00},
	{0x47, 0x94},
	{0x48, 0x14},

	//PAGE 16
	//Gamma Correction
	{0x03,  0x16},

	{0x30, 0x00},
	{0x31,	0x0a},
	{0x32,	0x1b},
	{0x33,	0x2e},
	{0x34, 0x5c},
	{0x35, 0x79},
	{0x36, 0x95},
	{0x37, 0xa4},
	{0x38, 0xb1},
	{0x39, 0xbd},
	{0x3a, 0xc8},
	{0x3b, 0xd9},
	{0x3c, 0xe8},
	{0x3d, 0xf5},
	{0x3e, 0xff},

	//PAGE 17 
	//Auto Flicker Cancellation 
	{0x03, 0x17},
	{0xc0, 0x03},
	{0xc4, 0x3c},
	{0xc5, 0x32},

	//PAGE 20 
	//AE 
	{0x03, 0x20},

	{0x10, 0x0c},
	{0x11, 0x04},

	{0x20, 0x01},
	{0x28, 0x27},
	{0x29, 0xa1},   
	{0x2a, 0xf0},
	{0x2b, 0x34},

	{0x30, 0x78},
	{0x39, 0x22},
	{0x3a, 0xde},
	{0x3b, 0x22},
	{0x3c, 0xde},

	{0x60, 0x95}, //d5, 99
	{0x68, 0x3c},
	{0x69, 0x64},
	{0x6A, 0x28},
	{0x6B, 0xc8},

	{0x70, 0x48},//Y Target 42
	{0x76, 0x22},
	{0x77, 0x02},   
	{0x78, 0x12},
	{0x79, 0x26}, //Yth 2
	{0x7a, 0x23},  
	{0x7c, 0x1c},
	{0x7d, 0x22},

	{0x83, 0x01}, //EXP Normal 33.33 fps 
	{0x84, 0xbc}, 
	{0x85, 0x56}, 
	{0x86, 0x00}, //EXPMin 3250.00 fps
	{0x87, 0xfa}, 
	{0x88, 0x01}, //EXP Max 7.14 fps 
	{0x89, 0xbc}, 
	{0x8a, 0x56}, 
	{0x8B, 0x1f}, //EXP100 
	{0x8C, 0xbd}, 
	{0x8D, 0x1a}, //EXP120 
	{0x8E, 0x5e}, 
	{0x9c, 0x03}, //EXP Limit 812.50 fps 
	{0x9d, 0xe8}, 
	{0x9e, 0x00}, //EXP Unit 
	{0x9f, 0xfa}, 
//	{0xa4, 0x18}, 


	{0x94, 0x01},
	{0x95, 0xb7},
	{0x96, 0x74},   
	{0x98, 0x8C},
	{0x99, 0x23},  

	{0xb1, 0x14},
	{0xb2, 0x48},
	{0xb4, 0x14},
	{0xb5, 0x38},
	{0xb6, 0x26},
	{0xb7, 0x20},
	{0xb8, 0x1d},
	{0xb9, 0x1b},
	{0xba, 0x1a},
	{0xbb, 0x19},
	{0xbc, 0x19},
	{0xbd, 0x18},

	{0xc0, 0x1a},   //0x1a->0x16
	{0xc3, 0x48},
	{0xc4, 0x48}, 

	//PAGE 22 
	//AWB
	{0x03, 0x22},
	{0x10, 0xe2},
	{0x11, 0x26},
	{0x21, 0x40},
	{0x30, 0x80},
	{0x31, 0x80},
	{0x38, 0x12},
	{0x39, 0x33},
	{0x3a, 0x88},
	{0x3b, 0xc4},
	{0x40, 0xf0},
	{0x41, 0x33},
	{0x42, 0x33},
	{0x43, 0xf3},
	{0x44, 0x55},
	{0x45, 0x44},
	{0x46, 0x02},
	{0x60, 0x00},
	{0x61, 0x00},

	{0x80, 0x35},
	{0x81, 0x20},
	{0x82, 0x35},

	{0x83, 0x52}, //RMAX Default : 50 -> 48 -> 52 
	{0x84, 0x1b}, //RMIN Default : 20
	{0x85, 0x55}, //BMAX Default : 50, 5a -> 58 -> 55
	{0x86, 0x2e}, //BMIN Default : 20
	{0x87, 0x40}, //RMAXB Default : 50, 4d
	{0x88, 0x2a}, //RMINB Default : 3e, 45 --> 42
	{0x89, 0x40}, //BMAXB Default : 2e, 2d --> 30//3e
	{0x8a, 0x30}, //BMINB Default : 20, 22 --> 26 --> 29
	{0x8b, 0x07}, //OUT TH//02
	{0x8d, 0x22},
	{0x8e, 0x71},  
	{0x8f, 0x63},

	{0x90, 0x60},
	{0x91, 0x5c},
	{0x92, 0x56},
	{0x93, 0x4c},//52
	{0x94, 0x3c},
	{0x95, 0x34},
	{0x96, 0x2f},
	{0x97, 0x28},
	{0x98, 0x24},
	{0x99, 0x21},
	{0x9a, 0x20},
	{0x9b, 0x09},
	//PAGE 22
    {0x03, 0x22},
	{0x10, 0xfb},


	//PAGE 20
	{0x03, 0x20},
	{0x10, 0x9c},

	{0x01, 0xf0},
	{0x03, 0x00},  
	{0x01, 0xc0},

	{0xff, 0xff}    //End of Initial Setting

};

static void HI708_Set_VGA_mode(void)
{
    HI708_write_cmos_sensor(0x01, HI708_read_cmos_sensor(0x01)|0x01);   //Sleep: For Write Reg

    HI708_write_cmos_sensor(0x03, 0x00);
    HI708_write_cmos_sensor(0x10, 0x00);        //VGA Size

    HI708_write_cmos_sensor(0x20, 0x00);
    HI708_write_cmos_sensor(0x21, 0x04);

    HI708_write_cmos_sensor(0x40, 0x01);        //HBLANK: 0x70 = 112
    HI708_write_cmos_sensor(0x41, 0x58);
    HI708_write_cmos_sensor(0x42, 0x00);        //VBLANK: 0x04 = 4
    HI708_write_cmos_sensor(0x43, 0x14);

   // HI708_write_cmos_sensor(0x03, 0x11);
   // HI708_write_cmos_sensor(0x10, 0x25);  

    HI708_write_cmos_sensor(0x03, 0x20);

    HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)&0x7f);   //Close AE
    HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)|0x08);   //Reset AE
	
    //HI708_write_cmos_sensor(0x83, 0x00);
    // HI708_write_cmos_sensor(0x84, 0xbe);
    //HI708_write_cmos_sensor(0x85, 0x6e);
    HI708_write_cmos_sensor(0x86, 0x00);
    HI708_write_cmos_sensor(0x87, 0xfa);

    HI708_write_cmos_sensor(0x8b, 0x3f);
    HI708_write_cmos_sensor(0x8c, 0x7a);
    HI708_write_cmos_sensor(0x8d, 0x34);
    HI708_write_cmos_sensor(0x8e, 0xbc);

    HI708_write_cmos_sensor(0x9c, 0x07);//0b
    HI708_write_cmos_sensor(0x9d, 0xd0);//b8
    HI708_write_cmos_sensor(0x9e, 0x00);
    HI708_write_cmos_sensor(0x9f, 0xfa);

    HI708_write_cmos_sensor(0x01, HI708_read_cmos_sensor(0x01)&0xfe);   //Exit Sleep: For Write Reg

    HI708_write_cmos_sensor(0x03, 0x20);
    HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)|0x80);   //Open AE
    HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)&0xf7);   //Reset AE

}

static void HI708_Initial_Setting(void)
{
	kal_uint32 iEcount;
	for(iEcount=0;(!((0xff==(HI708_Initial_Setting_Info[iEcount].address))&&(0xff==(HI708_Initial_Setting_Info[iEcount].data))));iEcount++)
	{	
		HI708_write_cmos_sensor(HI708_Initial_Setting_Info[iEcount].address, HI708_Initial_Setting_Info[iEcount].data);
	}
	
	HI708_Set_VGA_mode();
}

static void HI708_Init_Parameter(void)
{
    spin_lock(&hi708_yuv_drv_lock);
    HI708_sensor.first_init = KAL_TRUE;
    HI708_sensor.pv_mode = KAL_TRUE;
    HI708_sensor.night_mode = KAL_FALSE;
    HI708_sensor.MPEG4_Video_mode = KAL_FALSE;

    HI708_sensor.cp_pclk = HI708_sensor.pv_pclk;

    HI708_sensor.pv_dummy_pixels = 0;
    HI708_sensor.pv_dummy_lines = 0;
    HI708_sensor.cp_dummy_pixels = 0;
    HI708_sensor.cp_dummy_lines = 0;

    HI708_sensor.wb = 0;
    HI708_sensor.exposure = 0;
    HI708_sensor.effect = 0;
    HI708_sensor.banding = AE_FLICKER_MODE_50HZ;

    HI708_sensor.pv_line_length = 640;
    HI708_sensor.pv_frame_height = 480;
    HI708_sensor.cp_line_length = 640;
    HI708_sensor.cp_frame_height = 480;
    spin_unlock(&hi708_yuv_drv_lock);    
}

static kal_uint8 HI708_power_on(void)
{
    kal_uint8 HI708_sensor_id = 0;
    spin_lock(&hi708_yuv_drv_lock);
    HI708_sensor.pv_pclk = 13000000;
    spin_unlock(&hi708_yuv_drv_lock);
    //Software Reset
    HI708_write_cmos_sensor(0x01,0xf1);
    HI708_write_cmos_sensor(0x01,0xf3);
    HI708_write_cmos_sensor(0x01,0xf1);

    /* Read Sensor ID  */
    HI708_sensor_id = HI708_read_cmos_sensor(0x04);
    SENSORDB("[HI708YUV]:read Sensor ID:%x\n",HI708_sensor_id);	
    return HI708_sensor_id;
}


/*************************************************************************
* FUNCTION
*	HI708Open
*
* DESCRIPTION
*	This function initialize the registers of CMOS sensor
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 HI708Open(void)
{
    spin_lock(&hi708_yuv_drv_lock);
    sensor_id_fail = 0; 
    spin_unlock(&hi708_yuv_drv_lock);
    SENSORDB("[Enter]:HI708 Open func:");


    mt_set_gpio_mode(GPIO123,GPIO_MODE_01);
    mt_set_gpio_dir(GPIO123,GPIO_DIR_IN);
    mt_set_gpio_mode(GPIO141,GPIO_MODE_02);
    mt_set_gpio_dir(GPIO141,GPIO_DIR_IN);
    mt_set_gpio_mode(GPIO142,GPIO_MODE_02);
    mt_set_gpio_dir(GPIO142,GPIO_DIR_IN);
    mt_set_gpio_mode(GPIO143,GPIO_MODE_02);
    mt_set_gpio_dir(GPIO143,GPIO_DIR_IN);
    mt_set_gpio_mode(GPIO144,GPIO_MODE_02);
    mt_set_gpio_dir(GPIO144,GPIO_DIR_IN);
    mt_set_gpio_mode(GPIO145,GPIO_MODE_02);
    mt_set_gpio_dir(GPIO145,GPIO_DIR_IN);
    mt_set_gpio_mode(GPIO146,GPIO_MODE_02);
    mt_set_gpio_dir(GPIO146,GPIO_DIR_IN);
    mt_set_gpio_mode(GPIO153,GPIO_MODE_02);
    mt_set_gpio_dir(GPIO153,GPIO_DIR_IN);
    mt_set_gpio_mode(GPIO154,GPIO_MODE_02);
    mt_set_gpio_dir(GPIO154,GPIO_DIR_IN);
    mt_set_gpio_mode(GPIO155,GPIO_MODE_02);
    mt_set_gpio_dir(GPIO155,GPIO_DIR_IN);
    mt_set_gpio_mode(GPIO156,GPIO_MODE_02);
    mt_set_gpio_dir(GPIO156,GPIO_DIR_IN);
    if (HI708_power_on() != HI708_SENSOR_ID) 
    {
        SENSORDB("[HI708]Error:read sensor ID fail\n");
        spin_lock(&hi708_yuv_drv_lock);
        sensor_id_fail = 1;
        spin_unlock(&hi708_yuv_drv_lock);
        return ERROR_SENSOR_CONNECT_FAIL;
    }

    /* Apply sensor initail setting*/
    HI708_Initial_Setting();
    HI708_Init_Parameter(); 

    SENSORDB("[Exit]:HI708 Open func\n");     
    return ERROR_NONE;
}	/* HI708Open() */

UINT32 HI708Init(void)
{
    UINT32 open_ret = HI708Open();
    
    if (ERROR_SENSOR_CONNECT_FAIL != open_ret)
    {    
		HI708_write_cmos_sensor(0x03, 0x00);  
		HI708_write_cmos_sensor(0x08, 0x0f);  
        HI708_write_cmos_sensor(0x01, 0xf1); //sensor sleep mode
        mDELAY(20);
    }    

    return open_ret;
}   /* HI708Init() */

/*************************************************************************
* FUNCTION
*	HI708_GetSensorID
*
* DESCRIPTION
*	This function get the sensor ID
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 HI708_GetSensorID(kal_uint32 *sensorID)
{
    SENSORDB("[Enter]:HI708 Open func ");
    *sensorID = HI708_power_on() ;

    if (*sensorID != HI708_SENSOR_ID) 
    {
        SENSORDB("[HI708]Error:read sensor ID fail\n");
        spin_lock(&hi708_yuv_drv_lock);
        sensor_id_fail = 1;
        spin_unlock(&hi708_yuv_drv_lock);
        *sensorID = 0xFFFFFFFF; 
        return ERROR_SENSOR_CONNECT_FAIL;
    }	   

    return ERROR_NONE;    
}   /* HI708Open  */


/*************************************************************************
* FUNCTION
*	HI708Close
*
* DESCRIPTION
*	This function is to turn off sensor module power.
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 HI708Close(void)
{

	return ERROR_NONE;
}	/* HI708Close() */


static void HI708_Set_Mirror_Flip(kal_uint8 image_mirror)
{
    /********************************************************
    * Page Mode 0: Reg 0x0011 bit[1:0] = [Y Flip : X Flip]
    * 0: Off; 1: On.
    *********************************************************/ 
    kal_uint8 temp_data;   
    SENSORDB("[Enter]:HI708 set Mirror_flip func:image_mirror=%d\n",image_mirror);	
    HI708_write_cmos_sensor(0x03,0x00);     //Page 0	
    temp_data = (HI708_read_cmos_sensor(0x11) & 0xfc);
    spin_lock(&hi708_yuv_drv_lock);
    //HI708_sensor.mirror = (HI708_read_cmos_sensor(0x11) & 0xfc); 
    switch (image_mirror) 
    {
    case IMAGE_NORMAL:
        //HI708_sensor.mirror |= 0x00;
        temp_data |= 0x00;
        break;
    case IMAGE_H_MIRROR:
        //HI708_sensor.mirror |= 0x01;
        temp_data |= 0x01;
        break;
    case IMAGE_V_MIRROR:
        //HI708_sensor.mirror |= 0x02;
        temp_data |= 0x02;
        break;
    case IMAGE_HV_MIRROR:
        //HI708_sensor.mirror |= 0x03;
        temp_data |= 0x03;
        break;
    default:
        //HI708_sensor.mirror |= 0x00;
        temp_data |= 0x00;
    }
    HI708_sensor.mirror = temp_data;
    spin_unlock(&hi708_yuv_drv_lock);
    HI708_write_cmos_sensor(0x11, HI708_sensor.mirror);
    SENSORDB("[Exit]:HI708 set Mirror_flip func\n");
}

#if 0
static void HI708_set_dummy(kal_uint16 dummy_pixels,kal_uint16 dummy_lines)
{	
    HI708_write_cmos_sensor(0x03, 0x00);                        //Page 0
    HI708_write_cmos_sensor(0x40,((dummy_pixels & 0x0F00))>>8);       //HBLANK
    HI708_write_cmos_sensor(0x41,(dummy_pixels & 0xFF));
    HI708_write_cmos_sensor(0x42,((dummy_lines & 0xFF00)>>8));       //VBLANK ( Vsync Type 1)
    HI708_write_cmos_sensor(0x43,(dummy_lines & 0xFF));
}  
#endif

// 640 * 480


static void HI708_Cal_Min_Frame_Rate(kal_uint16 min_framerate)
{
    kal_uint32 HI708_expmax = 0;
    kal_uint32 HI708_expbanding = 0;
    kal_uint32 temp_data;
      
    SENSORDB("[HI708] HI708_Cal_Min_Frame_Rate:min_fps=%d\n",min_framerate);

    //No Fixed Framerate
    HI708_write_cmos_sensor(0x01, HI708_read_cmos_sensor(0x01)|0x01);   //Sleep: For Write Reg
    HI708_write_cmos_sensor(0x03, 0x00);
    HI708_write_cmos_sensor(0x11, HI708_read_cmos_sensor(0x11)&0xfb);

    HI708_write_cmos_sensor(0x03, 0x20);
    HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)&0x7f);   //Close AE
    HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)|0x08);   //Reset AE
    //HI708_write_cmos_sensor(0x11, 0x04);
    //HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)|0x08);   //Reset AE
    //HI708_write_cmos_sensor(0x2a, 0xf0);
    //HI708_write_cmos_sensor(0x2b, 0x34);

    HI708_write_cmos_sensor(0x03, 0x00);
    temp_data = ((HI708_read_cmos_sensor(0x40)<<8)|HI708_read_cmos_sensor(0x41));
    spin_lock(&hi708_yuv_drv_lock);
    HI708_sensor.pv_dummy_pixels = temp_data;
    HI708_sensor.pv_line_length = HI708_VGA_DEFAULT_PIXEL_NUMS+ HI708_sensor.pv_dummy_pixels ;
    spin_unlock(&hi708_yuv_drv_lock);

    if(HI708_sensor.banding == AE_FLICKER_MODE_50HZ)
    {
        HI708_expbanding = (HI708_sensor.pv_pclk/HI708_sensor.pv_line_length/100)*HI708_sensor.pv_line_length/8;
        HI708_expmax = HI708_expbanding*10*(100/min_framerate) ;
    }
    else if(HI708_sensor.banding == AE_FLICKER_MODE_60HZ)
    {
        HI708_expbanding = (HI708_sensor.pv_pclk/HI708_sensor.pv_line_length/120)*HI708_sensor.pv_line_length/8;
        HI708_expmax = HI708_expbanding*10*(120/min_framerate) ;
    }
    else//default 5oHZ
    {
        SENSORDB("[HI708][Error] Wrong Banding Setting!!!...");
    }

    HI708_write_cmos_sensor(0x03, 0x00);
    HI708_write_cmos_sensor(0x12, 0x05);
	
    HI708_write_cmos_sensor(0x03, 0x20);

    HI708_write_cmos_sensor(0x8b, 0x3f);
    HI708_write_cmos_sensor(0x8c, 0x7a);
    HI708_write_cmos_sensor(0x8d, 0x34);
    HI708_write_cmos_sensor(0x8e, 0xbc);

    HI708_write_cmos_sensor(0x9c, 0x07);//0b
    HI708_write_cmos_sensor(0x9d, 0xd0);//b8
    HI708_write_cmos_sensor(0x9e, 0x00);
    HI708_write_cmos_sensor(0x9f, 0xfa);

	if(HI708_sensor.night_mode)
		{       
		        HI708_write_cmos_sensor(0x03, 0x20);
			    HI708_write_cmos_sensor(0x83, 0x02);//(HI708_expmax>>16)&0xff);//7fps
		    	HI708_write_cmos_sensor(0x84, 0x49);//(HI708_expmax>>8)&0xff);
		    	HI708_write_cmos_sensor(0x85, 0xf0);//(HI708_expmax>>0)&0xff);
			    HI708_write_cmos_sensor(0x88, 0x02);//(HI708_expmax>>16)&0xff);//7fps
		    	HI708_write_cmos_sensor(0x89, 0x49);//(HI708_expmax>>8)&0xff);
		    	HI708_write_cmos_sensor(0x8a, 0xf0);//(HI708_expmax>>0)&0xff);
		    	HI708_night_flag = KAL_TRUE;
		}else
		{
			if(HI708_night_flag)
			{
		    	HI708_write_cmos_sensor(0x83, 0x02);//(HI708_expmax>>16)&0xff);//01   10fps
		    	HI708_write_cmos_sensor(0x84, 0xf9);//(HI708_expmax>>8)&0xff);//bc
		    	HI708_write_cmos_sensor(0x85, 0xb8);//(HI708_expmax>>0)&0xff);//56
		    	HI708_night_flag = KAL_FALSE;
			}
		    HI708_write_cmos_sensor(0x88, 0x02);//(HI708_expmax>>16)&0xff);//01   10fps
		    HI708_write_cmos_sensor(0x89, 0xf9);//(HI708_expmax>>8)&0xff);//bc
		    HI708_write_cmos_sensor(0x8a, 0xb8);//(HI708_expmax>>0)&0xff);//56
		}

    HI708_write_cmos_sensor(0x03, 0x20);
    HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)|0x80);   //Open AE
    HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)&0xf7);   //Reset AE
    HI708_write_cmos_sensor(0x03, 0x00);
    HI708_write_cmos_sensor(0x01, HI708_read_cmos_sensor(0x01)&0xfe);   //Exit Sleep: For Write Reg
	
}


static void HI708_Fix_Video_Frame_Rate(kal_uint16 fix_framerate)
{
    kal_uint32 HI708_expfix;
    kal_uint32 HI708_expfix_temp;
    kal_uint32 HI708_expmax = 0;
    kal_uint32 HI708_expbanding = 0;
    kal_uint32 temp_data1,temp_data2;
      
    SENSORDB("[Enter]HI708 Fix_video_frame_rate func: fix_fps=%d\n",fix_framerate);

    spin_lock(&hi708_yuv_drv_lock);
    HI708_sensor.video_current_frame_rate = fix_framerate;
    spin_unlock(&hi708_yuv_drv_lock);
    // Fixed Framerate
    HI708_write_cmos_sensor(0x01, HI708_read_cmos_sensor(0x01)|0x01);   //Sleep: For Write Reg

    HI708_write_cmos_sensor(0x03, 0x00);
    //HI708_write_cmos_sensor(0x11, HI708_read_cmos_sensor(0x11)|0x04);

    //HI708_write_cmos_sensor(0x12,HI708_read_cmos_sensor(0x12)&0xfe);
	
    HI708_write_cmos_sensor(0x03, 0x20);
    HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)&0x7f);   //Close AE

    HI708_write_cmos_sensor(0x11, 0x00);
    HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)|0x08);   //Reset AE
    //HI708_write_cmos_sensor(0x2a, 0x00);
    //HI708_write_cmos_sensor(0x2b, 0x35);

    HI708_write_cmos_sensor(0x03, 0x00);
    temp_data1 = ((HI708_read_cmos_sensor(0x40)<<8)|HI708_read_cmos_sensor(0x41));
    temp_data2 = ((HI708_read_cmos_sensor(0x42)<<8)|HI708_read_cmos_sensor(0x43));
    spin_lock(&hi708_yuv_drv_lock);
    HI708_sensor.pv_dummy_pixels = temp_data1; 
    HI708_sensor.pv_line_length = HI708_VGA_DEFAULT_PIXEL_NUMS + HI708_sensor.pv_dummy_pixels ;   
    HI708_sensor.pv_dummy_lines = temp_data2;
    spin_unlock(&hi708_yuv_drv_lock);
        
    HI708_expfix_temp = ((HI708_sensor.pv_pclk*10/fix_framerate)-(HI708_sensor.pv_line_length*HI708_sensor.pv_dummy_lines))/8;    
    HI708_expfix = ((HI708_expfix_temp*8/HI708_sensor.pv_line_length)*HI708_sensor.pv_line_length)/8;
        
    HI708_write_cmos_sensor(0x03, 0x20);    
    //HI708_write_cmos_sensor(0x83, (HI708_expfix>>16)&0xff);
    //HI708_write_cmos_sensor(0x84, (HI708_expfix>>8)&0xff);
    //HI708_write_cmos_sensor(0x85, (HI708_expfix>>0)&0xff);    
    HI708_write_cmos_sensor(0x8b, 0x1D);
    HI708_write_cmos_sensor(0x8c, 0x4C);
    HI708_write_cmos_sensor(0x8d, 0x18);
    HI708_write_cmos_sensor(0x8e, 0x6A);
/*
	if(HI708_sensor.night_mode)
		{
    HI708_write_cmos_sensor(0x83, 0x02);//(HI708_expfix>>16)&0xff);//10fps
    HI708_write_cmos_sensor(0x84, 0x7a);//(HI708_expfix>>8)&0xff);
    HI708_write_cmos_sensor(0x85, 0xc4);//(HI708_expfix>>0)&0xff);			
    HI708_write_cmos_sensor(0x88, 0x02);//(HI708_expfix>>16)&0xff);//10fps
    HI708_write_cmos_sensor(0x89, 0x7a);//(HI708_expfix>>8)&0xff);
    HI708_write_cmos_sensor(0x8a, 0xc4);//(HI708_expfix>>0)&0xff);		
    HI708_write_cmos_sensor(0x91, 0x02);//(HI708_expfix>>16)&0xff);//10fps
    HI708_write_cmos_sensor(0x92, 0x71);//(HI708_expfix>>8)&0xff);
    HI708_write_cmos_sensor(0x93, 0x00);//(HI708_expfix>>0)&0xff);
}else*/
//{
    HI708_write_cmos_sensor(0x83, 0x01);//(HI708_expfix>>16)&0xff);//11fps
    HI708_write_cmos_sensor(0x84, 0x07);//(HI708_expfix>>8)&0xff);
    HI708_write_cmos_sensor(0x85, 0xac);//(HI708_expfix>>0)&0xff);	
    HI708_write_cmos_sensor(0x88, 0x01);//(HI708_expfix>>16)&0xff);//11fps
    HI708_write_cmos_sensor(0x89, 0x07);//(HI708_expfix>>8)&0xff);
    HI708_write_cmos_sensor(0x8a, 0xac);//(HI708_expfix>>0)&0xff);	
    HI708_write_cmos_sensor(0x91, 0x01);//(HI708_expfix>>16)&0xff);//10fps
    HI708_write_cmos_sensor(0x92, 0x1b);//(HI708_expfix>>8)&0xff);
    HI708_write_cmos_sensor(0x93, 0x34);//(HI708_expfix>>0)&0xff);	
//}
    if(HI708_sensor.banding == AE_FLICKER_MODE_50HZ)
    {
        HI708_expbanding = ((HI708_read_cmos_sensor(0x8b)<<8)|HI708_read_cmos_sensor(0x8c));
    }
    else if(HI708_sensor.banding == AE_FLICKER_MODE_60HZ)
    {
        HI708_expbanding = ((HI708_read_cmos_sensor(0x8d)<<8)|HI708_read_cmos_sensor(0x8e));
    }
    else
    {
        SENSORDB("[HI708]Wrong Banding Setting!!!...");
    }
    HI708_expmax = ((HI708_expfix_temp-HI708_expbanding)/HI708_expbanding)*HI708_expbanding;    

    HI708_write_cmos_sensor(0x03, 0x20);
    //HI708_write_cmos_sensor(0x88, (HI708_expmax>>16)&0xff);
    //HI708_write_cmos_sensor(0x89, (HI708_expmax>>8)&0xff);
    //HI708_write_cmos_sensor(0x8a, (HI708_expmax>>0)&0xff);

    HI708_write_cmos_sensor(0x03, 0x20);
    HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)|0x80);   //Open AE
    HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)&0xf7);   //Reset AE

    HI708_write_cmos_sensor(0x01, HI708_read_cmos_sensor(0x01)&0xfe);   //Exit Sleep: For Write Reg
}

#if 0
// 320 * 240
static void HI708_Set_QVGA_mode(void)
{
    HI708_write_cmos_sensor(0x01, HI708_read_cmos_sensor(0x01)|0x01);   //Sleep: For Write Reg
    
    HI708_write_cmos_sensor(0x03, 0x00);
    HI708_write_cmos_sensor(0x10, 0x01);        //QVGA Size: 0x10 -> 0x01

    HI708_write_cmos_sensor(0x20, 0x00);
    HI708_write_cmos_sensor(0x21, 0x02);

    HI708_write_cmos_sensor(0x40, 0x01);        //HBLANK:  0x0158 = 344
    HI708_write_cmos_sensor(0x41, 0x58);
    HI708_write_cmos_sensor(0x42, 0x00);        //VBLANK:  0x14 = 20
    HI708_write_cmos_sensor(0x43, 0x14);

    HI708_write_cmos_sensor(0x03, 0x11);        //QVGA Fixframerate
    HI708_write_cmos_sensor(0x10, 0x21);  

    HI708_write_cmos_sensor(0x03, 0x20);
    HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)&0x7f);   //Close AE
    HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)|0x08);   //Reset AE

    HI708_write_cmos_sensor(0x83, 0x00);
    HI708_write_cmos_sensor(0x84, 0xaf);
    HI708_write_cmos_sensor(0x85, 0xc8);
    HI708_write_cmos_sensor(0x86, 0x00);
    HI708_write_cmos_sensor(0x87, 0xfa);

    HI708_write_cmos_sensor(0x8b, 0x3a);
    HI708_write_cmos_sensor(0x8c, 0x98);
    HI708_write_cmos_sensor(0x8d, 0x30);
    HI708_write_cmos_sensor(0x8e, 0xd4);

    HI708_write_cmos_sensor(0x9c, 0x0b);
    HI708_write_cmos_sensor(0x9d, 0x3b);
    HI708_write_cmos_sensor(0x9e, 0x00);
    HI708_write_cmos_sensor(0x9f, 0xfa);

    HI708_write_cmos_sensor(0x01, HI708_read_cmos_sensor(0x01)&0xfe);   //Exit Sleep: For Write Reg

    HI708_write_cmos_sensor(0x03, 0x20);
    HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)|0x80);   //Open AE
    HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)&0xf7);   //Reset AE

}
#endif
void HI708_night_mode(kal_bool enable)
{
    SENSORDB("HHL[Enter]HI708 night mode func:enable = %d\n",enable);
    SENSORDB("HI708_sensor.video_mode = %d\n",HI708_sensor.MPEG4_Video_mode); 
    SENSORDB("HI708_sensor.night_mode = %d\n",HI708_sensor.night_mode);
    spin_lock(&hi708_yuv_drv_lock);
    HI708_sensor.night_mode = enable;
    spin_unlock(&hi708_yuv_drv_lock);

    if(HI708_sensor.MPEG4_Video_mode == KAL_TRUE)
        return;

   if(enable)

{
  if (HI708_sensor.banding == AE_FLICKER_MODE_50HZ) 
  {

	SENSORDB("[HI708]HI708NightMode Disable AE_FLICKER_MODE_50HZ\n");

  HI708_write_cmos_sensor(0x03, 0x20);
  
  HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)&0x7f);   //Close AE
  HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)|0x08);   //Reset AE

  //BEGIN <> <20130922> <improve the camera experience that before to capture the next photo, the screen lighten instantly> panzaoyan
  HI708_write_cmos_sensor(0x83, 0x02);
  HI708_write_cmos_sensor(0x84, 0x7a);
  HI708_write_cmos_sensor(0x85, 0xc4);//15fps
  HI708_write_cmos_sensor(0x86, 0x00);
  HI708_write_cmos_sensor(0x87, 0xfa);
   //END <> <20130922> <improve the camera experience that before to capture the next photo, the screen lighten instantly> panzaoyan 
  HI708_write_cmos_sensor(0x88, 0x02);
  HI708_write_cmos_sensor(0x89, 0x7a);
  HI708_write_cmos_sensor(0x8A, 0xc4);//7
  
  HI708_write_cmos_sensor(0x8b, 0x1f);
  HI708_write_cmos_sensor(0x8c, 0xbd);
  HI708_write_cmos_sensor(0x8d, 0x1a);
  HI708_write_cmos_sensor(0x8e, 0x5e);
  
  HI708_write_cmos_sensor(0x9c, 0x03);
  HI708_write_cmos_sensor(0x9d, 0xe8);
  HI708_write_cmos_sensor(0x9e, 0x00);
  HI708_write_cmos_sensor(0x9f, 0xfa);
  
  
  HI708_write_cmos_sensor(0x01, HI708_read_cmos_sensor(0x01)&0xfe);   //Exit Sleep: For Write Reg
  
  HI708_write_cmos_sensor(0x03, 0x20);
  HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)|0x80);   //Open AE
  HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)&0xf7);   //Reset AE



}
else
{
SENSORDB("[HI708]HI708NightMode Disable AE_FLICKER_MODE_60HZ\n");
HI708_write_cmos_sensor(0x03, 0x20);

HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)&0x7f);	//Close AE
HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)|0x08);	//Reset AE

//BEGIN <> <20130922> <improve the camera experience that before to capture the next photo, the screen lighten instantly> panzaoyan

    HI708_write_cmos_sensor(0x83, 0x02);
    HI708_write_cmos_sensor(0x84, 0x78);
    HI708_write_cmos_sensor(0x85, 0xd0);//15fps
HI708_write_cmos_sensor(0x86, 0x00);
HI708_write_cmos_sensor(0x87, 0xfa);
//END <> <20130922> <improve the camera experience that before to capture the next photo, the screen lighten instantly> panzaoyan		
HI708_write_cmos_sensor(0x88, 0x02);
HI708_write_cmos_sensor(0x89, 0x78);
HI708_write_cmos_sensor(0x8A, 0xd0);//7

HI708_write_cmos_sensor(0x8b, 0x1f);
HI708_write_cmos_sensor(0x8c, 0xbd);
HI708_write_cmos_sensor(0x8d, 0x1a);
HI708_write_cmos_sensor(0x8e, 0x5e);

HI708_write_cmos_sensor(0x9c, 0x03);
HI708_write_cmos_sensor(0x9d, 0xe8);
HI708_write_cmos_sensor(0x9e, 0x00);
HI708_write_cmos_sensor(0x9f, 0xfa);


HI708_write_cmos_sensor(0x01, HI708_read_cmos_sensor(0x01)&0xfe);	//Exit Sleep: For Write Reg

HI708_write_cmos_sensor(0x03, 0x20);
HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)|0x80);	//Open AE
HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)&0xf7);	//Reset AE


  }
	  

}
else
{
  if (HI708_sensor.banding == AE_FLICKER_MODE_50HZ) 
  { 	SENSORDB("[HI708]HI708NightMode Enable AE_FLICKER_MODE_50HZ\n");
  HI708_write_cmos_sensor(0x03, 0x20);
  
  HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)&0x7f);   //Close AE
  HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)|0x08);   //Reset AE
   //BEGIN <> <20130922> <improve the camera experience that before to capture the next photo, the screen lighten instantly> panzaoyan 
  HI708_write_cmos_sensor(0x83, 0x01);
  HI708_write_cmos_sensor(0x84, 0xbc);
  HI708_write_cmos_sensor(0x85, 0x56);//15fps
  HI708_write_cmos_sensor(0x86, 0x00);
  HI708_write_cmos_sensor(0x87, 0xfa);
    //END <> <20130922> <improve the camera experience that before to capture the next photo, the screen lighten instantly> panzaoyan
  HI708_write_cmos_sensor(0x88, 0x01);
  HI708_write_cmos_sensor(0x89, 0xbc);
  HI708_write_cmos_sensor(0x8A, 0x56);//7
  
  HI708_write_cmos_sensor(0x8b, 0x1f);
  HI708_write_cmos_sensor(0x8c, 0xbd);
  HI708_write_cmos_sensor(0x8d, 0x1a);
  HI708_write_cmos_sensor(0x8e, 0x5e);
  
  HI708_write_cmos_sensor(0x9c, 0x03);
  HI708_write_cmos_sensor(0x9d, 0xe8);
  HI708_write_cmos_sensor(0x9e, 0x00);
  HI708_write_cmos_sensor(0x9f, 0xfa);
  
  
  HI708_write_cmos_sensor(0x01, HI708_read_cmos_sensor(0x01)&0xfe);   //Exit Sleep: For Write Reg
  
  HI708_write_cmos_sensor(0x03, 0x20);
  HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)|0x80);   //Open AE
  HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)&0xf7);   //Reset AE

  }
  else
  { 	SENSORDB("[HI708]HI708NightMode Enable AE_FLICKER_MODE_60HZ\n");

  HI708_write_cmos_sensor(0x03, 0x20);
  
  HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)&0x7f);   //Close AE
  HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)|0x08);   //Reset AE
    //BEGIN <> <20130922> <improve the camera experience that before to capture the next photo, the screen lighten instantly> panzaoyan
  HI708_write_cmos_sensor(0x83, 0x01);
  HI708_write_cmos_sensor(0x84, 0xc0);
  HI708_write_cmos_sensor(0x85, 0x3e);//15fps
  HI708_write_cmos_sensor(0x86, 0x00);
  HI708_write_cmos_sensor(0x87, 0xfa);
    //END<> <20130922> <improve the camera experience that before to capture the next photo, the screen lighten instantly> panzaoyan
  HI708_write_cmos_sensor(0x88, 0x01);
  HI708_write_cmos_sensor(0x89, 0xc0);
  HI708_write_cmos_sensor(0x8A, 0x3e);//7
  
  HI708_write_cmos_sensor(0x8b, 0x1f);
  HI708_write_cmos_sensor(0x8c, 0xbd);
  HI708_write_cmos_sensor(0x8d, 0x1a);
  HI708_write_cmos_sensor(0x8e, 0x5e);
  
  HI708_write_cmos_sensor(0x9c, 0x03);
  HI708_write_cmos_sensor(0x9d, 0xe8);
  HI708_write_cmos_sensor(0x9e, 0x00);
  HI708_write_cmos_sensor(0x9f, 0xfa);
  
  
  HI708_write_cmos_sensor(0x01, HI708_read_cmos_sensor(0x01)&0xfe);   //Exit Sleep: For Write Reg
  
  HI708_write_cmos_sensor(0x03, 0x20);
  HI708_write_cmos_sensor(0x10, HI708_read_cmos_sensor(0x10)|0x80);   //Open AE
  HI708_write_cmos_sensor(0x18, HI708_read_cmos_sensor(0x18)&0xf7);   //Reset AE

  }


}
}

/*************************************************************************
* FUNCTION
*	HI708Preview
*
* DESCRIPTION
*	This function start the sensor preview.
*
* PARAMETERS
*	*image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static UINT32 HI708Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

    HI708_write_cmos_sensor(0x03, 0x22);
    HI708_write_cmos_sensor(0x10, 0xfb);
    spin_lock(&hi708_yuv_drv_lock);
    sensor_config_data->SensorImageMirror = IMAGE_HV_MIRROR; 
    if(HI708_sensor.first_init == KAL_TRUE)
    {
        HI708_sensor.MPEG4_Video_mode = HI708_sensor.MPEG4_Video_mode;
    }
    else
    {
        HI708_sensor.MPEG4_Video_mode = KAL_FALSE;//!HI708_sensor.MPEG4_Video_mode;
    }
    spin_unlock(&hi708_yuv_drv_lock);

    SENSORDB("HHL[Enter]:HI708 preview func:");		
    SENSORDB("HI708_sensor.video_mode = %d\n",HI708_sensor.MPEG4_Video_mode); 

    spin_lock(&hi708_yuv_drv_lock);
    HI708_sensor.first_init = KAL_FALSE;	
    HI708_sensor.pv_mode = KAL_TRUE;		
    spin_unlock(&hi708_yuv_drv_lock);

   // {   
      //  SENSORDB("[HI708]preview set_VGA_mode\n");
	//
  //  }
  
    //HI708_write_cmos_sensor(0x03, 0x10);
    //HI708_write_cmos_sensor(0x40, 0x03);
    //HI708_write_cmos_sensor(0x62, 0x83);
    //HI708_write_cmos_sensor(0x63, 0x9a);    

   // HI708_Set_Mirror_Flip(sensor_config_data->SensorImageMirror);
   // HI708_Set_Mirror_Flip(IMAGE_V_MIRROR);

    SENSORDB("[Exit]:HI708 preview func\n");
   
   HI708_night_mode(HI708_sensor.night_mode);
    return TRUE; 
}	/* HI708_Preview */


UINT32 HI708Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
kal_uint32 CapShutter; 
    SENSORDB("HHL[HI708][Enter]HI708_capture_func\n");
    spin_lock(&hi708_yuv_drv_lock);
    HI708_sensor.pv_mode = KAL_FALSE;	
    spin_unlock(&hi708_yuv_drv_lock);
#if 0
    HI708_write_cmos_sensor(0x03, 0x20);
	HI708_sensor.PvShutter = (HI708_read_cmos_sensor(0x80) << 16)|(HI708_read_cmos_sensor(0x81) << 8)|HI708_read_cmos_sensor(0x82);
	CapShutter = HI708_sensor.PvShutter;
       HI708_sensor.CapExposure=HI708_sensor.PvShutter*8*76923

  HI708_write_cmos_sensor(0x03, 0x20);
  HI708_write_cmos_sensor(0x83, (CapShutter >> 16) & 0xFF);
  HI708_write_cmos_sensor(0x84, (CapShutter >> 8) & 0xFF);
  HI708_write_cmos_sensor(0x85, CapShutter & 0xFF);  
  #endif
    return ERROR_NONE;
}	/* HM3451Capture() */


UINT32 HI708GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
    SENSORDB("[Enter]:HI708 get Resolution func\n");

    pSensorResolution->SensorFullWidth=HI708_IMAGE_SENSOR_FULL_WIDTH ;  
    pSensorResolution->SensorFullHeight=HI708_IMAGE_SENSOR_FULL_HEIGHT ;
    pSensorResolution->SensorPreviewWidth=HI708_IMAGE_SENSOR_PV_WIDTH ;
    pSensorResolution->SensorPreviewHeight=HI708_IMAGE_SENSOR_PV_HEIGHT ;
    pSensorResolution->SensorVideoWidth=HI708_IMAGE_SENSOR_PV_WIDTH ;
    pSensorResolution->SensorVideoHeight=HI708_IMAGE_SENSOR_PV_HEIGHT ;
    pSensorResolution->Sensor3DFullWidth=HI708_IMAGE_SENSOR_FULL_WIDTH ;  
    pSensorResolution->Sensor3DFullHeight=HI708_IMAGE_SENSOR_FULL_HEIGHT ;
    pSensorResolution->Sensor3DPreviewWidth=HI708_IMAGE_SENSOR_PV_WIDTH ;
    pSensorResolution->Sensor3DPreviewHeight=HI708_IMAGE_SENSOR_PV_HEIGHT ;
    pSensorResolution->Sensor3DVideoWidth=HI708_IMAGE_SENSOR_PV_WIDTH ;
    pSensorResolution->Sensor3DVideoHeight=HI708_IMAGE_SENSOR_PV_HEIGHT ;

    SENSORDB("[Exit]:HI708 get Resolution func\n");	
    return ERROR_NONE;
}	/* HI708GetResolution() */

UINT32 HI708GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
					  MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{

	switch(ScenarioId)
		{
		 
			case MSDK_SCENARIO_ID_CAMERA_ZSD:
				 pSensorInfo->SensorPreviewResolutionX=HI708_IMAGE_SENSOR_PV_WIDTH;
				 pSensorInfo->SensorPreviewResolutionY=HI708_IMAGE_SENSOR_PV_HEIGHT;
				 pSensorInfo->SensorFullResolutionX=HI708_IMAGE_SENSOR_FULL_WIDTH;
				 pSensorInfo->SensorFullResolutionY=HI708_IMAGE_SENSOR_FULL_HEIGHT;			 
				 pSensorInfo->SensorCameraPreviewFrameRate=15;
				 break;
	
			case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
				 pSensorInfo->SensorPreviewResolutionX=HI708_IMAGE_SENSOR_PV_WIDTH;
				 pSensorInfo->SensorPreviewResolutionY=HI708_IMAGE_SENSOR_PV_HEIGHT;
				 pSensorInfo->SensorFullResolutionX=HI708_IMAGE_SENSOR_FULL_WIDTH;
				 pSensorInfo->SensorFullResolutionY=HI708_IMAGE_SENSOR_FULL_HEIGHT;				 
				 pSensorInfo->SensorCameraPreviewFrameRate=30;
				 break;
			case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
				 pSensorInfo->SensorPreviewResolutionX=HI708_IMAGE_SENSOR_PV_WIDTH;
				 pSensorInfo->SensorPreviewResolutionY=HI708_IMAGE_SENSOR_PV_HEIGHT;
				 pSensorInfo->SensorFullResolutionX=HI708_IMAGE_SENSOR_FULL_WIDTH;
				 pSensorInfo->SensorFullResolutionY=HI708_IMAGE_SENSOR_FULL_HEIGHT;				 
				 pSensorInfo->SensorCameraPreviewFrameRate=30;			
				break;
			case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
			case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
			case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added   
				 pSensorInfo->SensorPreviewResolutionX=HI708_IMAGE_SENSOR_PV_WIDTH;
				 pSensorInfo->SensorPreviewResolutionY=HI708_IMAGE_SENSOR_PV_HEIGHT;
				 pSensorInfo->SensorFullResolutionX=HI708_IMAGE_SENSOR_FULL_WIDTH;
				 pSensorInfo->SensorFullResolutionY=HI708_IMAGE_SENSOR_FULL_HEIGHT; 			 
				 pSensorInfo->SensorCameraPreviewFrameRate=30;			
				break;
			default:
	
				 pSensorInfo->SensorPreviewResolutionX=HI708_IMAGE_SENSOR_PV_WIDTH;
				 pSensorInfo->SensorPreviewResolutionY=HI708_IMAGE_SENSOR_PV_HEIGHT;
				 pSensorInfo->SensorFullResolutionX=HI708_IMAGE_SENSOR_FULL_WIDTH;
				 pSensorInfo->SensorFullResolutionY=HI708_IMAGE_SENSOR_FULL_HEIGHT;				 
				 pSensorInfo->SensorCameraPreviewFrameRate=30;
				 break;
				 
			}
	


    SENSORDB("[Enter]:HI708 getInfo func:ScenarioId = %d\n",ScenarioId);

  //  pSensorInfo->SensorPreviewResolutionX=HI708_IMAGE_SENSOR_PV_WIDTH;
  //  pSensorInfo->SensorPreviewResolutionY=HI708_IMAGE_SENSOR_PV_HEIGHT;
 //   pSensorInfo->SensorFullResolutionX=HI708_IMAGE_SENSOR_FULL_WIDTH;
 //   pSensorInfo->SensorFullResolutionY=HI708_IMAGE_SENSOR_FULL_HEIGHT;

    pSensorInfo->SensorCameraPreviewFrameRate=30;
    pSensorInfo->SensorVideoFrameRate=30;
    pSensorInfo->SensorStillCaptureFrameRate=30;
    pSensorInfo->SensorWebCamCaptureFrameRate=15;
    pSensorInfo->SensorResetActiveHigh=FALSE;//low is to reset 
    pSensorInfo->SensorResetDelayCount=4;  //4ms 
    pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_YUYV; //SENSOR_OUTPUT_FORMAT_YVYU;
    pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;	
    pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorInterruptDelayLines = 1; 
    pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_PARALLEL;


    pSensorInfo->CaptureDelayFrame = 2; 
    pSensorInfo->PreviewDelayFrame = 2;//10; 
    pSensorInfo->VideoDelayFrame = 0; 
    pSensorInfo->SensorMasterClockSwitch = 0; 
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_8MA;   	

    switch (ScenarioId)
    {
    case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
    case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW:
    case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:		
        pSensorInfo->SensorClockFreq=26;
        pSensorInfo->SensorClockDividCount=	3;
        pSensorInfo->SensorClockRisingCount= 0;
        pSensorInfo->SensorClockFallingCount= 2;
        pSensorInfo->SensorPixelClockCount= 3;
        pSensorInfo->SensorDataLatchCount= 2;
        pSensorInfo->SensorGrabStartX = 4; 
        pSensorInfo->SensorGrabStartY = 2;  	
        break;
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
    case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE:
        pSensorInfo->SensorClockFreq=26;
        pSensorInfo->SensorClockDividCount=	3;
        pSensorInfo->SensorClockRisingCount= 0;
        pSensorInfo->SensorClockFallingCount= 2;
        pSensorInfo->SensorPixelClockCount= 3;
        pSensorInfo->SensorDataLatchCount= 2;
        pSensorInfo->SensorGrabStartX = 4; 
        pSensorInfo->SensorGrabStartY = 2;//4;     			
        break;
    default:
        pSensorInfo->SensorClockFreq=26;
        pSensorInfo->SensorClockDividCount=3;
        pSensorInfo->SensorClockRisingCount=0;
        pSensorInfo->SensorClockFallingCount=2;
        pSensorInfo->SensorPixelClockCount=3;
        pSensorInfo->SensorDataLatchCount=2;
        pSensorInfo->SensorGrabStartX = 4; 
        pSensorInfo->SensorGrabStartY = 2;//4;     			
        break;
    }
    //	HI708_PixelClockDivider=pSensorInfo->SensorPixelClockCount;
    memcpy(pSensorConfigData, &HI708SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

    SENSORDB("[Exit]:HI708 getInfo func\n");	
    return ERROR_NONE;
}	/* HI708GetInfo() */


UINT32 HI708Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    SENSORDB("HHL [Enter]:HI708 Control func:ScenarioId = %d\n",ScenarioId);

    switch (ScenarioId)
    {
    case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
    //case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW:
    //case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:		
        HI708Preview(pImageWindow, pSensorConfigData); 
        break;
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
    //case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE:
        HI708Capture(pImageWindow, pSensorConfigData); 
        break;
     //   case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
     //   case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
    //    case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added   
    //    HI708Preview(pImageWindow, pSensorConfigData); 
     //   break;
	//	case MSDK_SCENARIO_ID_CAMERA_ZSD:
	//	HI708Capture(pImageWindow, pSensorConfigData); 
	//	break;		
    default:
        break; 
    }

    SENSORDB("[Exit]:HI708 Control func\n");	
    return TRUE;
}	/* HI708Control() */


/*************************************************************************
* FUNCTION
*	HI708_set_param_wb
*
* DESCRIPTION
*	wb setting.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
BOOL HI708_set_param_wb(UINT16 para)
{
    //This sensor need more time to balance AWB, 
    //we suggest higher fps or drop some frame to avoid garbage color when preview initial
    SENSORDB("[Enter]HI708 set_param_wb func:para = %d\n",para);

    if(HI708_sensor.wb == para) return KAL_TRUE;	

    spin_lock(&hi708_yuv_drv_lock);
    HI708_sensor.wb = para;
    spin_unlock(&hi708_yuv_drv_lock);
    
    switch (para)
    {            
    case AWB_MODE_AUTO:
        {
        HI708_write_cmos_sensor(0x03, 0x22);			
        HI708_write_cmos_sensor(0x11, 0x2e);				
        //HI708_write_cmos_sensor(0x80, 0x38);
        //HI708_write_cmos_sensor(0x82, 0x38);				
        HI708_write_cmos_sensor(0x83, 0x66);//56
        HI708_write_cmos_sensor(0x84, 0x26);
        HI708_write_cmos_sensor(0x85, 0x57);
        HI708_write_cmos_sensor(0x86, 0x20);				
        }                
        break;
    case AWB_MODE_CLOUDY_DAYLIGHT:
    {
        HI708_write_cmos_sensor(0x03, 0x22);
        HI708_write_cmos_sensor(0x11, 0x28);
        HI708_write_cmos_sensor(0x80, 0x62);
        HI708_write_cmos_sensor(0x82, 0x2e);
        HI708_write_cmos_sensor(0x83, 0x6a);
        HI708_write_cmos_sensor(0x84, 0x65);
        HI708_write_cmos_sensor(0x85, 0x1d);
        HI708_write_cmos_sensor(0x86, 0x1a);
        }			   
        break;
    case AWB_MODE_DAYLIGHT:
        {
        HI708_write_cmos_sensor(0x03, 0x22);
        HI708_write_cmos_sensor(0x11, 0x28);          
        HI708_write_cmos_sensor(0x80, 0x50);
        HI708_write_cmos_sensor(0x82, 0x2d);
        HI708_write_cmos_sensor(0x83, 0x52);
        HI708_write_cmos_sensor(0x84, 0x40);
        HI708_write_cmos_sensor(0x85, 0x30);
        HI708_write_cmos_sensor(0x86, 0x1c);
        }      
        break;
    case AWB_MODE_INCANDESCENT:	
        {
        HI708_write_cmos_sensor(0x03, 0x22);
        HI708_write_cmos_sensor(0x11, 0x28);          
        HI708_write_cmos_sensor(0x80, 0x29);
        HI708_write_cmos_sensor(0x82, 0x54);
        HI708_write_cmos_sensor(0x83, 0x2e);
        HI708_write_cmos_sensor(0x84, 0x23);
        HI708_write_cmos_sensor(0x85, 0x58);
        HI708_write_cmos_sensor(0x86, 0x4f);
        }		
        break;  
    case AWB_MODE_FLUORESCENT:
        {
        HI708_write_cmos_sensor(0x03, 0x22);
        HI708_write_cmos_sensor(0x11, 0x28);
        HI708_write_cmos_sensor(0x80, 0x41);
        HI708_write_cmos_sensor(0x82, 0x42);
        HI708_write_cmos_sensor(0x83, 0x44);
        HI708_write_cmos_sensor(0x84, 0x34);
        HI708_write_cmos_sensor(0x85, 0x46);
        HI708_write_cmos_sensor(0x86, 0x3a);
        }	
        break;  
    case AWB_MODE_TUNGSTEN:
        {
        HI708_write_cmos_sensor(0x03, 0x22);
        HI708_write_cmos_sensor(0x80, 0x24);
        HI708_write_cmos_sensor(0x81, 0x20);
        HI708_write_cmos_sensor(0x82, 0x58);
        HI708_write_cmos_sensor(0x83, 0x27);
        HI708_write_cmos_sensor(0x84, 0x22);
        HI708_write_cmos_sensor(0x85, 0x58);
        HI708_write_cmos_sensor(0x86, 0x52);
        }
        break;
    case AWB_MODE_OFF:
        {
        SENSORDB("HI708 AWB OFF");
        HI708_write_cmos_sensor(0x03, 0x22);
        HI708_write_cmos_sensor(0x10, 0xe2);
        }
        break;
    default:
        return FALSE;
    }

    return TRUE;	
} /* HI708_set_param_wb */

/*************************************************************************
* FUNCTION
*	HI708_set_param_effect
*
* DESCRIPTION
*	effect setting.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
BOOL HI708_set_param_effect(UINT16 para)
{
   SENSORDB("[Enter]HI708 set_param_effect func:para = %d\n",para);
   
    if(HI708_sensor.effect == para) return KAL_TRUE;

    spin_lock(&hi708_yuv_drv_lock);
    HI708_sensor.effect = para;
    spin_unlock(&hi708_yuv_drv_lock);
    
    switch (para)
    {
    case MEFFECT_OFF:
        {
        HI708_write_cmos_sensor(0x03, 0x10);
        HI708_write_cmos_sensor(0x11, 0x43);
        HI708_write_cmos_sensor(0x12, 0x30);
        HI708_write_cmos_sensor(0x13, 0x00);
        HI708_write_cmos_sensor(0x44, 0x80);
        HI708_write_cmos_sensor(0x45, 0x80);

        HI708_write_cmos_sensor(0x47, 0x7f);
        //HI708_write_cmos_sensor(0x03, 0x13);
       // HI708_write_cmos_sensor(0x20, 0x07);
        //HI708_write_cmos_sensor(0x21, 0x07);


        }
        break;
    case MEFFECT_SEPIA:
        {
        HI708_write_cmos_sensor(0x03, 0x10);
        HI708_write_cmos_sensor(0x11, 0x03);
        HI708_write_cmos_sensor(0x12, 0x23);
        HI708_write_cmos_sensor(0x13, 0x00);
        HI708_write_cmos_sensor(0x44, 0x6c);
        HI708_write_cmos_sensor(0x45, 0x9a);

        HI708_write_cmos_sensor(0x47, 0x7f);
       // HI708_write_cmos_sensor(0x03, 0x13);
       // HI708_write_cmos_sensor(0x20, 0x07);
        //HI708_write_cmos_sensor(0x21, 0x07);
 

        }	
        break;  
    case MEFFECT_NEGATIVE:
        {
        HI708_write_cmos_sensor(0x03, 0x10);
        HI708_write_cmos_sensor(0x11, 0x03);
        HI708_write_cmos_sensor(0x12, 0x08);
        HI708_write_cmos_sensor(0x13, 0x00);
        HI708_write_cmos_sensor(0x14, 0x00);
        }
        break; 
    case MEFFECT_SEPIAGREEN:		
        {
        HI708_write_cmos_sensor(0x03, 0x10);
        HI708_write_cmos_sensor(0x11, 0x03);
        HI708_write_cmos_sensor(0x12, 0x03);
        //HI708_write_cmos_sensor(0x40, 0x00);
        HI708_write_cmos_sensor(0x13, 0x00);
        HI708_write_cmos_sensor(0x44, 0x30);
        HI708_write_cmos_sensor(0x45, 0x50);
        }	
        break;
    case MEFFECT_SEPIABLUE:
        {
        HI708_write_cmos_sensor(0x03, 0x10);
        HI708_write_cmos_sensor(0x11, 0x03);
        HI708_write_cmos_sensor(0x12, 0x03);
        //HI708_write_cmos_sensor(0x40, 0x00);
        HI708_write_cmos_sensor(0x13, 0x00);
        HI708_write_cmos_sensor(0x44, 0xb0);
        HI708_write_cmos_sensor(0x45, 0x40);
        }     
        break;        
    case MEFFECT_MONO:			
        {
        HI708_write_cmos_sensor(0x03, 0x10);
        HI708_write_cmos_sensor(0x11, 0x03);
        HI708_write_cmos_sensor(0x12, 0x03);
        //HI708_write_cmos_sensor(0x40, 0x00);
        HI708_write_cmos_sensor(0x44, 0x80);
        HI708_write_cmos_sensor(0x45, 0x80);
        }
        break;
    default:
        return KAL_FALSE;
    }

    return KAL_TRUE;
} /* HI708_set_param_effect */

/*************************************************************************
* FUNCTION
*	HI708_set_param_banding
*
* DESCRIPTION
*	banding setting.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
BOOL HI708_set_param_banding(UINT16 para)
{
    SENSORDB("[Enter]HI708 set_param_banding func:para = %d\n",para);

    if(HI708_sensor.banding == para) return KAL_TRUE;

    spin_lock(&hi708_yuv_drv_lock);
    HI708_sensor.banding = para;
    spin_unlock(&hi708_yuv_drv_lock);

    switch (para)
    {
    case AE_FLICKER_MODE_50HZ:
        {
              HI708_write_cmos_sensor(0x03,0x20);
		HI708_write_cmos_sensor(0x10,0x1c);
		HI708_write_cmos_sensor(0x18,0x38);
	  //BEGIN <> <20130922> <improve the camera experience that before to capture the next photo, the screen lighten instantly> panzaoyan	
              HI708_write_cmos_sensor(0x83, 0x01);
              HI708_write_cmos_sensor(0x84, 0xbc);
              HI708_write_cmos_sensor(0x85, 0x56);//15fps
	  //END <> <20130922> <improve the camera experience that before to capture the next photo, the screen lighten instantly> panzaoyan	
		HI708_write_cmos_sensor(0x88, 0x01);
		HI708_write_cmos_sensor(0x89, 0xbc);
		HI708_write_cmos_sensor(0x8A, 0x56);//50HZ

		HI708_write_cmos_sensor(0x18, 0x30);
              HI708_write_cmos_sensor(0x10, 0x9c);
        }
        break;
    case AE_FLICKER_MODE_60HZ:
        {
		HI708_write_cmos_sensor(0x03,0x20);
		HI708_write_cmos_sensor(0x10,0x0c);
		HI708_write_cmos_sensor(0x18,0x38);
		//BEGIN <> <20130922> <improve the camera experience that before to capture the next photo, the screen lighten instantly> panzaoyan
		HI708_write_cmos_sensor(0x83, 0x01);
		HI708_write_cmos_sensor(0x84, 0xc0);
		HI708_write_cmos_sensor(0x85, 0x3e);//15fps
		HI708_write_cmos_sensor(0x86, 0x00);
		HI708_write_cmos_sensor(0x87, 0xfa);
		//END <> <20130922> <improve the camera experience that before to capture the next photo, the screen lighten instantly> panzaoyan
		HI708_write_cmos_sensor(0x88, 0x01);
		HI708_write_cmos_sensor(0x89, 0xc0);
		HI708_write_cmos_sensor(0x8A, 0x3e);//60HZ

		HI708_write_cmos_sensor(0x18, 0x30);
		HI708_write_cmos_sensor(0x10, 0x8c);
        }
        break;
    default:
        return KAL_FALSE;
    }
    
    return KAL_TRUE;
} /* HI708_set_param_banding */




/*************************************************************************
* FUNCTION
*	HI708_set_param_exposure
*
* DESCRIPTION
*	exposure setting.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
BOOL HI708_set_param_exposure(UINT16 para)
{
    SENSORDB("[Enter]HI708 set_param_exposure func:para = %d\n",para);

    if(HI708_sensor.exposure == para) return KAL_TRUE;

    spin_lock(&hi708_yuv_drv_lock);
    HI708_sensor.exposure = para;
    spin_unlock(&hi708_yuv_drv_lock);

    HI708_write_cmos_sensor(0x03,0x10);
    HI708_write_cmos_sensor(0x12,HI708_read_cmos_sensor(0x12)|0x10);
    switch (para)
    {
		/*GIONEE: malp 20130625 add for CR00828155  start*/
	case AE_EV_COMP_20:  //+2 EV
		HI708_write_cmos_sensor(0x40,0x30);
		break; 
		/*GIONEE: malp 20130625 add for CR00828155	end*/
    case AE_EV_COMP_13:  //+1.3 EV
        HI708_write_cmos_sensor(0x40,0x20);
        break;  
    case AE_EV_COMP_10:  //+1 EV
        HI708_write_cmos_sensor(0x40,0x15);
        break;    
    case AE_EV_COMP_07:  //+0.7 EV
        HI708_write_cmos_sensor(0x40,0x10);
        break;    
    case AE_EV_COMP_03:	 //	+0.3 EV	
        HI708_write_cmos_sensor(0x40,0x05);	
        break;    
    case AE_EV_COMP_00:  // +0 EV
        HI708_write_cmos_sensor(0x40,0x88);//90//96 wangc modified for reducing previewer brightness at 2012-10-24

        break;    
    case AE_EV_COMP_n03:  // -0.3 EV
       HI708_write_cmos_sensor(0x40,0x95);//98//a3
  
        break;    
    case AE_EV_COMP_n07:	// -0.7 EV		
        HI708_write_cmos_sensor(0x40,0xa0);	
        break;    
    case AE_EV_COMP_n10:   //-1 EV
        HI708_write_cmos_sensor(0x40,0xa5);
        break;
    case AE_EV_COMP_n13:  // -1.3 EV
        HI708_write_cmos_sensor(0x40,0xb0);
        break;
		/*GIONEE: malp 20130625 add for CR00828155  start*/
	case AE_EV_COMP_n20:   //-2 EV
		HI708_write_cmos_sensor(0x40,0xc0);
		break;
		/*GIONEE: malp 20130625 add for CR00828155  end*/
    default:
        return FALSE;
    }

    return TRUE;	
} /* HI708_set_param_exposure */

void HI708_set_AE_mode(UINT32 iPara)
{
    UINT8 temp_AE_reg = 0;
    SENSORDB("HI708_set_AE_mode = %d E \n",iPara);
    HI708_write_cmos_sensor(0x03,0x20);
    temp_AE_reg = HI708_read_cmos_sensor(0x10);

    if (AE_MODE_OFF == iPara)
    {
        // turn off AEC/AGC
        HI708_write_cmos_sensor(0x10,temp_AE_reg &~ 0x10);
    }	
    else
    {
        HI708_write_cmos_sensor(0x10,temp_AE_reg | 0x10);
    }
}
UINT32 HI708YUVSensorSetting(FEATURE_ID iCmd, UINT32 iPara)
{
    SENSORDB("[Enter]HI708YUVSensorSetting func:cmd = %d\n",iCmd);

    switch (iCmd) 
    {
    case FID_SCENE_MODE:	    //auto mode or night mode
        if (iPara == SCENE_MODE_OFF)//auto mode
        {
            HI708_night_mode(FALSE); 
        }
        else if (iPara == SCENE_MODE_NIGHTSCENE)//night mode
        {
            HI708_night_mode(TRUE); 
        }	
        break; 	    
    case FID_AWB_MODE:
        HI708_set_param_wb(iPara);
        break;
    case FID_COLOR_EFFECT:
        HI708_set_param_effect(iPara);
        break;
    case FID_AE_EV:	    	    
        HI708_set_param_exposure(iPara);
        break;
    case FID_AE_FLICKER:	    	    	    
        HI708_set_param_banding(iPara);
        break;
    case FID_ZOOM_FACTOR:
        spin_lock(&hi708_yuv_drv_lock);
        HI708_zoom_factor = iPara; 
        spin_unlock(&hi708_yuv_drv_lock);
        break; 
    case FID_AE_SCENE_MODE: 
        HI708_set_AE_mode(iPara);
        break; 
    default:
        break;
    }
    return TRUE;
}   /* HI708YUVSensorSetting */

UINT32 HI708YUVSetVideoMode(UINT16 u2FrameRate)
{
/*    spin_lock(&hi708_yuv_drv_lock);
    HI708_sensor.MPEG4_Video_mode = KAL_TRUE;
    spin_unlock(&hi708_yuv_drv_lock);
    SENSORDB("[Enter]HI708 Set Video Mode:FrameRate= %d\n",u2FrameRate);
    SENSORDB("HI708_sensor.video_mode = %d\n",HI708_sensor.MPEG4_Video_mode);

   // if(u2FrameRate == 30) u2FrameRate = 20;
    //u2FrameRate = 12;
   
    spin_lock(&hi708_yuv_drv_lock);
    HI708_sensor.fix_framerate = u2FrameRate * 10;
    spin_unlock(&hi708_yuv_drv_lock);
    
    if(HI708_sensor.fix_framerate <= 300 )
    {
        HI708_Fix_Video_Frame_Rate(HI708_sensor.fix_framerate); 
    }
    else 
    {
        SENSORDB("Wrong Frame Rate"); 
    }
        
    return TRUE;
*/
}

void HI708GetAFMaxNumFocusAreas(UINT32 *pFeatureReturnPara32)
{	
    *pFeatureReturnPara32 = 0;    
    SENSORDB("HI708GetAFMaxNumFocusAreas *pFeatureReturnPara32 = %d\n",  *pFeatureReturnPara32);
}

void HI708GetAEMaxNumMeteringAreas(UINT32 *pFeatureReturnPara32)
{     
    *pFeatureReturnPara32 = 0;    
    SENSORDB("HI708GetAEMaxNumMeteringAreas *pFeatureReturnPara32 = %d\n",  *pFeatureReturnPara32);	
}

void HI708GetExifInfo(UINT32 exifAddr)
{
    SENSOR_EXIF_INFO_STRUCT* pExifInfo = (SENSOR_EXIF_INFO_STRUCT*)exifAddr;
    pExifInfo->FNumber = 28;
    pExifInfo->AEISOSpeed = AE_ISO_100;
    pExifInfo->AWBMode = HI708_sensor.wb;
    pExifInfo->CapExposureTime = 0;
    pExifInfo->FlashLightTimeus = 0;
    pExifInfo->RealISOValue = AE_ISO_100;
}

UINT32 HI708FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
							 UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
    //UINT16 u2Temp = 0; 
    UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
    UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
    UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
    UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
    MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
    MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;
    SENSORDB("HHL [Enter]:HI708 Feature Control func:FeatureId = %d\n",FeatureId);

    switch (FeatureId)
    {
    case SENSOR_FEATURE_GET_RESOLUTION:
        *pFeatureReturnPara16++=HI708_IMAGE_SENSOR_FULL_WIDTH;
        *pFeatureReturnPara16=HI708_IMAGE_SENSOR_FULL_HEIGHT;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_GET_PERIOD:
        *pFeatureReturnPara16++=HI708_IMAGE_SENSOR_PV_WIDTH;//+HI708_sensor.pv_dummy_pixels;
        *pFeatureReturnPara16=HI708_IMAGE_SENSOR_PV_HEIGHT;//+HI708_sensor.pv_dummy_lines;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
        //*pFeatureReturnPara32 = HI708_sensor_pclk/10;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_SET_ESHUTTER:

        break;
    case SENSOR_FEATURE_SET_NIGHTMODE:
        HI708_night_mode((BOOL) *pFeatureData16);
        break;
    case SENSOR_FEATURE_SET_GAIN:
        break; 
    case SENSOR_FEATURE_SET_FLASHLIGHT:
        break;
    case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
        break;
    case SENSOR_FEATURE_SET_REGISTER:
        HI708_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
        break;
    case SENSOR_FEATURE_GET_REGISTER:
        pSensorRegData->RegData = HI708_read_cmos_sensor(pSensorRegData->RegAddr);
        break;
    case SENSOR_FEATURE_GET_CONFIG_PARA:
        memcpy(pSensorConfigData, &HI708SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
        *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
        break;
    case SENSOR_FEATURE_SET_CCT_REGISTER:
    case SENSOR_FEATURE_GET_CCT_REGISTER:
    case SENSOR_FEATURE_SET_ENG_REGISTER:
    case SENSOR_FEATURE_GET_ENG_REGISTER:
    case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
    case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
    case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
    case SENSOR_FEATURE_GET_GROUP_INFO:
    case SENSOR_FEATURE_GET_ITEM_INFO:
    case SENSOR_FEATURE_SET_ITEM_INFO:
    case SENSOR_FEATURE_GET_ENG_INFO:
        break;
    case SENSOR_FEATURE_GET_GROUP_COUNT:
        // *pFeatureReturnPara32++=0;
        //*pFeatureParaLen=4;
        break; 

    case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
        // get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
        // if EEPROM does not exist in camera module.
        *pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_SET_YUV_CMD:
        HI708YUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));
        break;	
    case SENSOR_FEATURE_SET_VIDEO_MODE:
        HI708YUVSetVideoMode(*pFeatureData16);
        break; 
    case SENSOR_FEATURE_CHECK_SENSOR_ID:
        HI708_GetSensorID(pFeatureData32); 
        break; 
    case SENSOR_FEATURE_GET_AF_MAX_NUM_FOCUS_AREAS:
        HI708GetAFMaxNumFocusAreas(pFeatureReturnPara32);            
        *pFeatureParaLen=4;
        break;        
    case SENSOR_FEATURE_GET_AE_MAX_NUM_METERING_AREAS:
        HI708GetAEMaxNumMeteringAreas(pFeatureReturnPara32);            
        *pFeatureParaLen=4;
        break;   
    case SENSOR_FEATURE_GET_EXIF_INFO:
        SENSORDB("SENSOR_FEATURE_GET_EXIF_INFO\n");
        SENSORDB("EXIF addr = 0x%x\n",*pFeatureData32);          
        HI708GetExifInfo(*pFeatureData32);
        break;        
    default:
        break;			
    }
    return ERROR_NONE;
}	/* HI708FeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncHI708=
{
    HI708Open,
    HI708GetInfo,
    HI708GetResolution,
    HI708FeatureControl,
    HI708Control,
    HI708Close
};

UINT32 HI708_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{

    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&SensorFuncHI708;

    return ERROR_NONE;
}	/* SensorInit() */






