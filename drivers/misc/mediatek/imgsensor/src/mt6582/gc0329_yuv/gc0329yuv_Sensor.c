/*****************************************************************************
 *
 * Filename:
 * ---------
 *   gc0329yuv_Sensor.c
 *
 * Project:
 * --------
 *   MAUI
 *
 * Description:
 * ------------
 *   Image sensor driver function
 *   V1.2.3
 *
 * Author:
 * -------
 *   Leo
 *
 *=============================================================
 *             HISTORY
 * Below this line, this part is controlled by GCoreinc. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Log$
 * 2012.02.29  kill bugs
 *   
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by GCoreinc. DO NOT MODIFY!!
 *=============================================================
 ******************************************************************************/
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"

#include "gc0329yuv_Sensor.h"
#include "gc0329yuv_Camera_Sensor_para.h"
#include "gc0329yuv_CameraCustomized.h"

//#define GC0329YUV_DEBUG
#ifdef GC0329YUV_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif

typedef struct
{
  UINT16  iSensorVersion;
  UINT16  iNightMode;
  UINT16  iWB;
  UINT16  iEffect;
  UINT16  iEV;
  UINT16  iBanding;
  UINT16  iMirror;
  UINT16  iFrameRate;
  //0    :No-Fix FrameRate 
  //other:Fixed FrameRate
} Gc0329Status;
Gc0329Status Gc0329CurrentStatus;
static DEFINE_SPINLOCK(Gc0329_drv_lock);
extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);

kal_uint16 GC0329_write_cmos_sensor(kal_uint8 addr, kal_uint8 para)
{
    char puSendCmd[2] = {(char)(addr & 0xFF) , (char)(para & 0xFF)};
	
	iWriteRegI2C(puSendCmd , 2, GC0329_WRITE_ID);

}
kal_uint16 GC0329_read_cmos_sensor(kal_uint8 addr)
{
	kal_uint16 get_byte=0;
    char puSendCmd = { (char)(addr & 0xFF) };
	iReadRegI2C(&puSendCmd , 1, (u8*)&get_byte, 1, GC0329_READ_ID);
	
    return get_byte;
}


/*******************************************************************************
 * // Adapter for Winmo typedef
 ********************************************************************************/
#define WINMO_USE 0

#define Sleep(ms) mdelay(ms)
#define RETAILMSG(x,...)
#define TEXT

kal_bool   GC0329_MPEG4_encode_mode = KAL_FALSE;
kal_uint16 GC0329_dummy_pixels = 0, GC0329_dummy_lines = 0;
kal_bool   GC0329_MODE_CAPTURE = KAL_FALSE;
kal_bool   GC0329_NIGHT_MODE = KAL_FALSE;

kal_uint32 GC0329_isp_master_clock;
static kal_uint32 GC0329_g_fPV_PCLK = 26;

kal_uint8 GC0329_sensor_write_I2C_address = GC0329_WRITE_ID;
kal_uint8 GC0329_sensor_read_I2C_address = GC0329_READ_ID;

UINT8 GC0329PixelClockDivider=0;

MSDK_SENSOR_CONFIG_STRUCT GC0329SensorConfigData;

#define GC0329_SET_PAGE0 	GC0329_write_cmos_sensor(0xfe, 0x00)
#define GC0329_SET_PAGE1 	GC0329_write_cmos_sensor(0xfe, 0x01)


/*************************************************************************
 * FUNCTION
 *	GC0329_SetShutter
 *
 * DESCRIPTION
 *	This function set e-shutter of GC0329 to change exposure time.
 *
 * PARAMETERS
 *   iShutter : exposured lines
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
void GC0329_Set_Shutter(kal_uint16 iShutter)
{
} /* Set_GC0329_Shutter */


/*************************************************************************
 * FUNCTION
 *	GC0329_read_Shutter
 *
 * DESCRIPTION
 *	This function read e-shutter of GC0329 .
 *
 * PARAMETERS
 *  None
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
kal_uint16 GC0329_Read_Shutter(void)
{
    	kal_uint8 temp_reg1, temp_reg2;
	kal_uint16 shutter;

	temp_reg1 = GC0329_read_cmos_sensor(0x04);
	temp_reg2 = GC0329_read_cmos_sensor(0x03);

	shutter = (temp_reg1 & 0xFF) | (temp_reg2 << 8);

	return shutter;
} /* GC0329_read_shutter */


/*************************************************************************
 * FUNCTION
 *	GC0329_write_reg
 *
 * DESCRIPTION
 *	This function set the register of GC0329.
 *
 * PARAMETERS
 *	addr : the register index of GC0329
 *  para : setting parameter of the specified register of GC0329
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
void GC0329_write_reg(kal_uint32 addr, kal_uint32 para)
{
	GC0329_write_cmos_sensor(addr, para);
} /* GC0329_write_reg() */


/*************************************************************************
 * FUNCTION
 *	GC0329_read_cmos_sensor
 *
 * DESCRIPTION
 *	This function read parameter of specified register from GC0329.
 *
 * PARAMETERS
 *	addr : the register index of GC0329
 *
 * RETURNS
 *	the data that read from GC0329
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
kal_uint32 GC0329_read_reg(kal_uint32 addr)
{
	return GC0329_read_cmos_sensor(addr);
} /* OV7670_read_reg() */


/*************************************************************************
* FUNCTION
*	GC0329_awb_enable
*
* DESCRIPTION
*	This function enable or disable the awb (Auto White Balance).
*
* PARAMETERS
*	1. kal_bool : KAL_TRUE - enable awb, KAL_FALSE - disable awb.
*
* RETURNS
*	kal_bool : It means set awb right or not.
*
*************************************************************************/
static void GC0329_awb_enable(kal_bool enalbe)
{	 
	kal_uint16 temp_AWB_reg = 0;

	temp_AWB_reg = GC0329_read_cmos_sensor(0x42);
	
	if (enalbe)
	{
		GC0329_write_cmos_sensor(0x42, (temp_AWB_reg |0x02));
	}
	else
	{
		GC0329_write_cmos_sensor(0x42, (temp_AWB_reg & (~0x02)));
	}

}


static void Gc0329InitialPara(void)
{
  /*Initial status setting 
  Can be better by sync with MT9V114InitialSetting*/
  spin_lock(&Gc0329_drv_lock);
  Gc0329CurrentStatus.iNightMode = 0xFFFF;
  Gc0329CurrentStatus.iWB = AWB_MODE_AUTO;
  Gc0329CurrentStatus.iEffect = MEFFECT_OFF;
  Gc0329CurrentStatus.iBanding = AE_FLICKER_MODE_50HZ;
  Gc0329CurrentStatus.iEV = AE_EV_COMP_n03;
  Gc0329CurrentStatus.iMirror = IMAGE_NORMAL;
  Gc0329CurrentStatus.iFrameRate = 0;//No Fix FrameRate
  spin_unlock(&Gc0329_drv_lock);
}


void Gc0329GetExifInfo(UINT32 exifAddr)
{
    SENSOR_EXIF_INFO_STRUCT* pExifInfo = (SENSOR_EXIF_INFO_STRUCT*)exifAddr;
    pExifInfo->FNumber = 28;
    pExifInfo->AEISOSpeed = AE_ISO_100;
    pExifInfo->AWBMode = Gc0329CurrentStatus.iWB;
    pExifInfo->CapExposureTime = 0;
    pExifInfo->FlashLightTimeus = 0;
    pExifInfo->RealISOValue = AE_ISO_100;
}
/*************************************************************************
 * FUNCTION
 *	GC0329_config_window
 *
 * DESCRIPTION
 *	This function config the hardware window of GC0329 for getting specified
 *  data of that window.
 *
 * PARAMETERS
 *	start_x : start column of the interested window
 *  start_y : start row of the interested window
 *  width  : column widht of the itnerested window
 *  height : row depth of the itnerested window
 *
 * RETURNS
 *	the data that read from GC0329
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
void GC0329_config_window(kal_uint16 startx, kal_uint16 starty, kal_uint16 width, kal_uint16 height)
{
} /* GC0329_config_window */


/*************************************************************************
 * FUNCTION
 *	GC0329_SetGain
 *
 * DESCRIPTION
 *	This function is to set global gain to sensor.
 *
 * PARAMETERS
 *   iGain : sensor global gain(base: 0x40)
 *
 * RETURNS
 *	the actually gain set to sensor.
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
kal_uint16 GC0329_SetGain(kal_uint16 iGain)
{
	return iGain;
}


/*************************************************************************
* FUNCTION
*	GC0329_GAMMA_Select
*
* DESCRIPTION
*	This function is served for FAE to select the appropriate GAMMA curve.
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


void GC0329GammaSelect(kal_uint32 GammaLvl)
{
	switch(GammaLvl)
	{
		case GC0329_RGB_Gamma_m1:						//smallest gamma curve
			GC0329_write_cmos_sensor(0xfe, 0x00);
			GC0329_write_cmos_sensor(0xbf, 0x06);
			GC0329_write_cmos_sensor(0xc0, 0x12);
			GC0329_write_cmos_sensor(0xc1, 0x22);
			GC0329_write_cmos_sensor(0xc2, 0x35);
			GC0329_write_cmos_sensor(0xc3, 0x4b);
			GC0329_write_cmos_sensor(0xc4, 0x5f);
			GC0329_write_cmos_sensor(0xc5, 0x72);
			GC0329_write_cmos_sensor(0xc6, 0x8d);
			GC0329_write_cmos_sensor(0xc7, 0xa4);
			GC0329_write_cmos_sensor(0xc8, 0xb8);
			GC0329_write_cmos_sensor(0xc9, 0xc8);
			GC0329_write_cmos_sensor(0xca, 0xd4);
			GC0329_write_cmos_sensor(0xcb, 0xde);
			GC0329_write_cmos_sensor(0xcc, 0xe6);
			GC0329_write_cmos_sensor(0xcd, 0xf1);
			GC0329_write_cmos_sensor(0xce, 0xf8);
			GC0329_write_cmos_sensor(0xcf, 0xfd);
			break;
		case GC0329_RGB_Gamma_m2:
			GC0329_write_cmos_sensor(0xBF, 0x08);
			GC0329_write_cmos_sensor(0xc0, 0x0F);
			GC0329_write_cmos_sensor(0xc1, 0x21);
			GC0329_write_cmos_sensor(0xc2, 0x32);
			GC0329_write_cmos_sensor(0xc3, 0x43);
			GC0329_write_cmos_sensor(0xc4, 0x50);
			GC0329_write_cmos_sensor(0xc5, 0x5E);
			GC0329_write_cmos_sensor(0xc6, 0x78);
			GC0329_write_cmos_sensor(0xc7, 0x90);
			GC0329_write_cmos_sensor(0xc8, 0xA6);
			GC0329_write_cmos_sensor(0xc9, 0xB9);
			GC0329_write_cmos_sensor(0xcA, 0xC9);
			GC0329_write_cmos_sensor(0xcB, 0xD6);
			GC0329_write_cmos_sensor(0xcC, 0xE0);
			GC0329_write_cmos_sensor(0xcD, 0xEE);
			GC0329_write_cmos_sensor(0xcE, 0xF8);
			GC0329_write_cmos_sensor(0xcF, 0xFF);
			break;
			
		case GC0329_RGB_Gamma_m3:			
			GC0329_write_cmos_sensor(0xBF, 0x0B);
			GC0329_write_cmos_sensor(0xc0, 0x16);
			GC0329_write_cmos_sensor(0xc1, 0x29);
			GC0329_write_cmos_sensor(0xc2, 0x3C);
			GC0329_write_cmos_sensor(0xc3, 0x4F);
			GC0329_write_cmos_sensor(0xc4, 0x5F);
			GC0329_write_cmos_sensor(0xc5, 0x6F);
			GC0329_write_cmos_sensor(0xc6, 0x8A);
			GC0329_write_cmos_sensor(0xc7, 0x9F);
			GC0329_write_cmos_sensor(0xc8, 0xB4);
			GC0329_write_cmos_sensor(0xc9, 0xC6);
			GC0329_write_cmos_sensor(0xcA, 0xD3);
			GC0329_write_cmos_sensor(0xcB, 0xDD);
			GC0329_write_cmos_sensor(0xcC, 0xE5);
			GC0329_write_cmos_sensor(0xcD, 0xF1);
			GC0329_write_cmos_sensor(0xcE, 0xFA);
			GC0329_write_cmos_sensor(0xcF, 0xFF);
			break;
			
		case GC0329_RGB_Gamma_m4:
			GC0329_write_cmos_sensor(0xBF, 0x0E);
			GC0329_write_cmos_sensor(0xc0, 0x1C);
			GC0329_write_cmos_sensor(0xc1, 0x34);
			GC0329_write_cmos_sensor(0xc2, 0x48);
			GC0329_write_cmos_sensor(0xc3, 0x5A);
			GC0329_write_cmos_sensor(0xc4, 0x6B);
			GC0329_write_cmos_sensor(0xc5, 0x7B);
			GC0329_write_cmos_sensor(0xc6, 0x95);
			GC0329_write_cmos_sensor(0xc7, 0xAB);
			GC0329_write_cmos_sensor(0xc8, 0xBF);
			GC0329_write_cmos_sensor(0xc9, 0xCE);
			GC0329_write_cmos_sensor(0xcA, 0xD9);
			GC0329_write_cmos_sensor(0xcB, 0xE4);
			GC0329_write_cmos_sensor(0xcC, 0xEC);
			GC0329_write_cmos_sensor(0xcD, 0xF7);
			GC0329_write_cmos_sensor(0xcE, 0xFD);
			GC0329_write_cmos_sensor(0xcF, 0xFF);
			break;
			
		case GC0329_RGB_Gamma_m5:
			GC0329_write_cmos_sensor(0xBF, 0x10);
			GC0329_write_cmos_sensor(0xc0, 0x20);
			GC0329_write_cmos_sensor(0xc1, 0x38);
			GC0329_write_cmos_sensor(0xc2, 0x4E);
			GC0329_write_cmos_sensor(0xc3, 0x63);
			GC0329_write_cmos_sensor(0xc4, 0x76);
			GC0329_write_cmos_sensor(0xc5, 0x87);
			GC0329_write_cmos_sensor(0xc6, 0xA2);
			GC0329_write_cmos_sensor(0xc7, 0xB8);
			GC0329_write_cmos_sensor(0xc8, 0xCA);
			GC0329_write_cmos_sensor(0xc9, 0xD8);
			GC0329_write_cmos_sensor(0xcA, 0xE3);
			GC0329_write_cmos_sensor(0xcB, 0xEB);
			GC0329_write_cmos_sensor(0xcC, 0xF0);
			GC0329_write_cmos_sensor(0xcD, 0xF8);
			GC0329_write_cmos_sensor(0xcE, 0xFD);
			GC0329_write_cmos_sensor(0xcF, 0xFF);
			break;
			
		case GC0329_RGB_Gamma_m6:										// largest gamma curve
			GC0329_write_cmos_sensor(0xBF, 0x14);
			GC0329_write_cmos_sensor(0xc0, 0x28);
			GC0329_write_cmos_sensor(0xc1, 0x44);
			GC0329_write_cmos_sensor(0xc2, 0x5D);
			GC0329_write_cmos_sensor(0xc3, 0x72);
			GC0329_write_cmos_sensor(0xc4, 0x86);
			GC0329_write_cmos_sensor(0xc5, 0x95);
			GC0329_write_cmos_sensor(0xc6, 0xB1);
			GC0329_write_cmos_sensor(0xc7, 0xC6);
			GC0329_write_cmos_sensor(0xc8, 0xD5);
			GC0329_write_cmos_sensor(0xc9, 0xE1);
			GC0329_write_cmos_sensor(0xcA, 0xEA);
			GC0329_write_cmos_sensor(0xcB, 0xF1);
			GC0329_write_cmos_sensor(0xcC, 0xF5);
			GC0329_write_cmos_sensor(0xcD, 0xFB);
			GC0329_write_cmos_sensor(0xcE, 0xFE);
			GC0329_write_cmos_sensor(0xcF, 0xFF);
			break;
		case GC0329_RGB_Gamma_night:									//Gamma for night mode
			GC0329_write_cmos_sensor(0xBF, 0x0B);
			GC0329_write_cmos_sensor(0xc0, 0x16);
			GC0329_write_cmos_sensor(0xc1, 0x29);
			GC0329_write_cmos_sensor(0xc2, 0x3C);
			GC0329_write_cmos_sensor(0xc3, 0x4F);
			GC0329_write_cmos_sensor(0xc4, 0x5F);
			GC0329_write_cmos_sensor(0xc5, 0x6F);
			GC0329_write_cmos_sensor(0xc6, 0x8A);
			GC0329_write_cmos_sensor(0xc7, 0x9F);
			GC0329_write_cmos_sensor(0xc8, 0xB4);
			GC0329_write_cmos_sensor(0xc9, 0xC6);
			GC0329_write_cmos_sensor(0xcA, 0xD3);
			GC0329_write_cmos_sensor(0xcB, 0xDD);
			GC0329_write_cmos_sensor(0xcC, 0xE5);
			GC0329_write_cmos_sensor(0xcD, 0xF1);
			GC0329_write_cmos_sensor(0xcE, 0xFA);
			GC0329_write_cmos_sensor(0xcF, 0xFF);
			break;
		default:
			//GC0329_RGB_Gamma_m1
			GC0329_write_cmos_sensor(0xfe, 0x00);
			GC0329_write_cmos_sensor(0xbf, 0x06);
			GC0329_write_cmos_sensor(0xc0, 0x12);
			GC0329_write_cmos_sensor(0xc1, 0x22);
			GC0329_write_cmos_sensor(0xc2, 0x35);
			GC0329_write_cmos_sensor(0xc3, 0x4b);
			GC0329_write_cmos_sensor(0xc4, 0x5f);
			GC0329_write_cmos_sensor(0xc5, 0x72);
			GC0329_write_cmos_sensor(0xc6, 0x8d);
			GC0329_write_cmos_sensor(0xc7, 0xa4);
			GC0329_write_cmos_sensor(0xc8, 0xb8);
			GC0329_write_cmos_sensor(0xc9, 0xc8);
			GC0329_write_cmos_sensor(0xca, 0xd4);
			GC0329_write_cmos_sensor(0xcb, 0xde);
			GC0329_write_cmos_sensor(0xcc, 0xe6);
			GC0329_write_cmos_sensor(0xcd, 0xf1);
			GC0329_write_cmos_sensor(0xce, 0xf8);
			GC0329_write_cmos_sensor(0xcf, 0xfd);
			break;
	}
}


/*************************************************************************
 * FUNCTION
 *	GC0329_NightMode
 *
 * DESCRIPTION
 *	This function night mode of GC0329.
 *
 * PARAMETERS
 *	bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
void GC0329NightMode(kal_bool bEnable)
{
	if (bEnable)
	{	
		GC0329_write_cmos_sensor(0xfe, 0x01);
		if(GC0329_MPEG4_encode_mode == KAL_TRUE)
			GC0329_write_cmos_sensor(0x33, 0x00);
		else
			GC0329_write_cmos_sensor(0x33, 0x30);
		GC0329_write_cmos_sensor(0xfe, 0x00);
		GC0329GammaSelect(GC0329_RGB_Gamma_night);
		GC0329_NIGHT_MODE = KAL_TRUE;
	}
	else 
	{
		GC0329_write_cmos_sensor(0xfe, 0x01);
		if(GC0329_MPEG4_encode_mode == KAL_TRUE)
			GC0329_write_cmos_sensor(0x33, 0x00);
		else
			GC0329_write_cmos_sensor(0x33, 0x20);
		GC0329_write_cmos_sensor(0xfe, 0x00);
		GC0329GammaSelect(GC0329_RGB_Gamma_m6);
		GC0329_NIGHT_MODE = KAL_FALSE;
	}
} /* GC0329_NightMode */

/*************************************************************************
* FUNCTION
*	GC0329_Sensor_Init
*
* DESCRIPTION
*	This function apply all of the initial setting to sensor.
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
*************************************************************************/
void GC0329_Sensor_Init(void)
{
	GC0329_write_cmos_sensor(0xfe, 0x80);
	GC0329_write_cmos_sensor(0xfe, 0x80);
	GC0329_write_cmos_sensor(0xfc, 0x16);
	GC0329_write_cmos_sensor(0xfc, 0x16);
	GC0329_write_cmos_sensor(0xfc, 0x16);
	GC0329_write_cmos_sensor(0xfc, 0x16);
	GC0329_SET_PAGE0;
	GC0329_write_cmos_sensor(0xf0, 0x07);
	GC0329_write_cmos_sensor(0xf1, 0x01);

	GC0329_write_cmos_sensor(0x73, 0x90);
	GC0329_write_cmos_sensor(0x74, 0x80);
	GC0329_write_cmos_sensor(0x75, 0x80);
	GC0329_write_cmos_sensor(0x76, 0x94);

	GC0329_write_cmos_sensor(0x42, 0x00);
	GC0329_write_cmos_sensor(0x77, 0x57);
	GC0329_write_cmos_sensor(0x78, 0x4d);
	GC0329_write_cmos_sensor(0x79, 0x45);
	//GC0329_write_cmos_sensor(0x42, 0xfc);

	////////////////////analog////////////////////
	GC0329_write_cmos_sensor(0x0a, 0x02);
	GC0329_write_cmos_sensor(0x0c, 0x02);
	GC0329_write_cmos_sensor(0x17, 0x14); // 16   lanking 20120423  mirror setting
	GC0329_write_cmos_sensor(0x19, 0x05);
	GC0329_write_cmos_sensor(0x1b, 0x24);
	GC0329_write_cmos_sensor(0x1c, 0x04);
	GC0329_write_cmos_sensor(0x1e, 0x08);
	GC0329_write_cmos_sensor(0x1f, 0x08); // c0  20120720//c8
	GC0329_write_cmos_sensor(0x20, 0x00);
	GC0329_write_cmos_sensor(0x21, 0x48);
	GC0329_write_cmos_sensor(0x22, 0xba); //  ba  20120720
	GC0329_write_cmos_sensor(0x23, 0x22);
	GC0329_write_cmos_sensor(0x24, 0x16);

	////////////////////blk////////////////////
	GC0329_write_cmos_sensor(0x26, 0xf7); 
	GC0329_write_cmos_sensor(0x28, 0x7f); 
	GC0329_write_cmos_sensor(0x29, 0x00); 
	GC0329_write_cmos_sensor(0x32, 0x04);
	GC0329_write_cmos_sensor(0x33, 0x20);
	GC0329_write_cmos_sensor(0x34, 0x20);
	GC0329_write_cmos_sensor(0x35, 0x20);
	GC0329_write_cmos_sensor(0x36, 0x20);

	////////////////////ISP BLOCK ENABL////////////////////
	GC0329_write_cmos_sensor(0x40, 0xff);
	GC0329_write_cmos_sensor(0x41, 0x44);
	GC0329_write_cmos_sensor(0x42, 0x7e);
	GC0329_write_cmos_sensor(0x44, 0xa0);
	GC0329_write_cmos_sensor(0x46, 0x02);
	GC0329_write_cmos_sensor(0x4b, 0xca);
	GC0329_write_cmos_sensor(0x4d, 0x01);
	GC0329_write_cmos_sensor(0x4f, 0x01);
	GC0329_write_cmos_sensor(0x70, 0x48);

	//GC0329_write_cmos_sensor(0xb0, 0x00);
	//GC0329_write_cmos_sensor(0xbc, 0x00);
	//GC0329_write_cmos_sensor(0xbd, 0x00);
	//GC0329_write_cmos_sensor(0xbe, 0x00);
	////////////////////DNDD////////////////////
	GC0329_write_cmos_sensor(0x80, 0xe7); 
	GC0329_write_cmos_sensor(0x82, 0x5f); //55
	GC0329_write_cmos_sensor(0x83, 0x02);  // add by lanking 20120423
	GC0329_write_cmos_sensor(0x87, 0x4a);

	////////////////////INTPEE////////////////////
	GC0329_write_cmos_sensor(0x95, 0x43);//54
	GC0329_write_cmos_sensor(0xd3, 0x38);
	GC0329_write_cmos_sensor(0x3b, 0x04);
	GC0329_write_cmos_sensor(0x3c, 0x04);
	GC0329_write_cmos_sensor(0x3d, 0x04);
	GC0329_write_cmos_sensor(0x3e, 0x04);
	

	////////////////////ASDE////////////////////
	//GC0329_SET_PAGE1;
	//GC0329_write_cmos_sensor(0x18, 0x22);
	//GC0329_SET_PAGE0;
	//GC0329_write_cmos_sensor(0x9c, 0x0a);
	//GC0329_write_cmos_sensor(0xa0, 0xaf);
	//GC0329_write_cmos_sensor(0xa2, 0xff);
	//GC0329_write_cmos_sensor(0xa4, 0x50);
	//GC0329_write_cmos_sensor(0xa5, 0x21);
	//GC0329_write_cmos_sensor(0xa7, 0x35);

	////////////////////RGB gamma////////////////////
	//RGB gamma m4'
			GC0329_write_cmos_sensor(0xBF, 0x14);
			GC0329_write_cmos_sensor(0xc0, 0x28);
			GC0329_write_cmos_sensor(0xc1, 0x44);
			GC0329_write_cmos_sensor(0xc2, 0x5D);
			GC0329_write_cmos_sensor(0xc3, 0x72);
			GC0329_write_cmos_sensor(0xc4, 0x86);
			GC0329_write_cmos_sensor(0xc5, 0x95);
			GC0329_write_cmos_sensor(0xc6, 0xB1);
			GC0329_write_cmos_sensor(0xc7, 0xC6);
			GC0329_write_cmos_sensor(0xc8, 0xD5);
			GC0329_write_cmos_sensor(0xc9, 0xE1);
			GC0329_write_cmos_sensor(0xcA, 0xEA);
			GC0329_write_cmos_sensor(0xcB, 0xF1);
			GC0329_write_cmos_sensor(0xcC, 0xF5);
			GC0329_write_cmos_sensor(0xcD, 0xFB);
			GC0329_write_cmos_sensor(0xcE, 0xFE);
	GC0329_write_cmos_sensor(0xcF, 0xFF);

	//////////////////CC///////////////////
	GC0329_SET_PAGE0;

	GC0329_write_cmos_sensor(0xb3, 0x44);
	GC0329_write_cmos_sensor(0xb4, 0xfd);
	GC0329_write_cmos_sensor(0xb5, 0x02);
	GC0329_write_cmos_sensor(0xb6, 0xfa);
	GC0329_write_cmos_sensor(0xb7, 0x48);
	GC0329_write_cmos_sensor(0xb8, 0xf0);

	// crop 						   
	GC0329_write_cmos_sensor(0x50, 0x01);

	////////////////////YCP////////////////////
	GC0329_SET_PAGE0;
	GC0329_write_cmos_sensor(0xd1, 0x30);
	GC0329_write_cmos_sensor(0xd2, 0x30);
	GC0329_write_cmos_sensor(0xdd, 0x54);

	////////////////////AEC////////////////////
	GC0329_SET_PAGE1;
	GC0329_write_cmos_sensor(0x10, 0x40);
	GC0329_write_cmos_sensor(0x11, 0x21);
	GC0329_write_cmos_sensor(0x12, 0x07);
	GC0329_write_cmos_sensor(0x13, 0x50);
	GC0329_write_cmos_sensor(0x17, 0x88);
	GC0329_write_cmos_sensor(0x21, 0xb0);
	GC0329_write_cmos_sensor(0x22, 0x48);
	GC0329_write_cmos_sensor(0x3c, 0x95);
	GC0329_write_cmos_sensor(0x3d, 0x50);
	GC0329_write_cmos_sensor(0x3e, 0x48); 


			GC0329_SET_PAGE0;
			GC0329_write_cmos_sensor(0x05, 0x01); 	
			GC0329_write_cmos_sensor(0x06, 0x32); 
			GC0329_write_cmos_sensor(0x07, 0x00);
			GC0329_write_cmos_sensor(0x08, 0x70);
			
			GC0329_SET_PAGE1;
			GC0329_write_cmos_sensor(0x29, 0x00);   //anti-flicker step [11:8]
			GC0329_write_cmos_sensor(0x2a, 0x3c);   //anti-flicker step [7:0]
			
			GC0329_write_cmos_sensor(0x2b, 0x02);   //exp level 0  14.28fps
			GC0329_write_cmos_sensor(0x2c, 0x58); 
			GC0329_write_cmos_sensor(0x2d, 0x02);   //exp level 1  12.50fps
			GC0329_write_cmos_sensor(0x2e, 0xd0); 
			GC0329_write_cmos_sensor(0x2f, 0x03);   //exp level 2  10.00fps
			GC0329_write_cmos_sensor(0x30, 0xc0); 
			GC0329_write_cmos_sensor(0x31, 0x04);   //exp level 3  7.14fps
			GC0329_write_cmos_sensor(0x32, 0xb0); 
			GC0329_SET_PAGE0;

	////////////////////AWB////////////////////
	GC0329_SET_PAGE1;
	GC0329_write_cmos_sensor(0x06, 0x16);
	GC0329_write_cmos_sensor(0x07, 0x06);
	GC0329_write_cmos_sensor(0x08, 0x98);
	GC0329_write_cmos_sensor(0x09, 0xee);
	GC0329_write_cmos_sensor(0x50, 0xfc);
	GC0329_write_cmos_sensor(0x51, 0x18);//20
	GC0329_write_cmos_sensor(0x52, 0x0b);
	GC0329_write_cmos_sensor(0x53, 0x1d);//20
	GC0329_write_cmos_sensor(0x54, 0x35);//40
	GC0329_write_cmos_sensor(0x55, 0x10);
	GC0329_write_cmos_sensor(0x56, 0x20);
	//GC0329_write_cmos_sensor(0x57, 0x40);
	GC0329_write_cmos_sensor(0x58, 0xa0);
	GC0329_write_cmos_sensor(0x59, 0x28);
	GC0329_write_cmos_sensor(0x5a, 0x02);
	GC0329_write_cmos_sensor(0x5b, 0x63);
	GC0329_write_cmos_sensor(0x5c, 0x34);
	GC0329_write_cmos_sensor(0x5d, 0x73);
	GC0329_write_cmos_sensor(0x5e, 0x11);
	GC0329_write_cmos_sensor(0x5f, 0x40);
	GC0329_write_cmos_sensor(0x60, 0x40);
	GC0329_write_cmos_sensor(0x61, 0xc8);
	GC0329_write_cmos_sensor(0x62, 0xa0);
	GC0329_write_cmos_sensor(0x63, 0x40);
	GC0329_write_cmos_sensor(0x64, 0x50);
	GC0329_write_cmos_sensor(0x65, 0x98);
	GC0329_write_cmos_sensor(0x66, 0xfa);
	GC0329_write_cmos_sensor(0x67, 0x70);
	GC0329_write_cmos_sensor(0x68, 0x58);
	GC0329_write_cmos_sensor(0x69, 0x85);
	GC0329_write_cmos_sensor(0x6a, 0x40);
	GC0329_write_cmos_sensor(0x6b, 0x39);
	GC0329_write_cmos_sensor(0x6c, 0x18);
	GC0329_write_cmos_sensor(0x6d, 0x28);
	GC0329_write_cmos_sensor(0x6e, 0x41);
	GC0329_write_cmos_sensor(0x70, 0x02);
	GC0329_write_cmos_sensor(0x71, 0x00);
	GC0329_write_cmos_sensor(0x72, 0x10);
	GC0329_write_cmos_sensor(0x73, 0x40);

	//GC0329_write_cmos_sensor(0x74, 0x32);
	//GC0329_write_cmos_sensor(0x75, 0x40);
	//GC0329_write_cmos_sensor(0x76, 0x30);
	//GC0329_write_cmos_sensor(0x77, 0x48);
	//GC0329_write_cmos_sensor(0x7a, 0x50);
	//GC0329_write_cmos_sensor(0x7b, 0x20); 

	GC0329_write_cmos_sensor(0x80, 0x60);
	GC0329_write_cmos_sensor(0x81, 0x50);
	GC0329_write_cmos_sensor(0x82, 0x42);
	GC0329_write_cmos_sensor(0x83, 0x40);
	GC0329_write_cmos_sensor(0x84, 0x40);
	GC0329_write_cmos_sensor(0x85, 0x40);

	GC0329_write_cmos_sensor(0x74, 0x40);
	GC0329_write_cmos_sensor(0x75, 0x58);
	GC0329_write_cmos_sensor(0x76, 0x24);
	GC0329_write_cmos_sensor(0x77, 0x40);
	GC0329_write_cmos_sensor(0x78, 0x20);
	GC0329_write_cmos_sensor(0x79, 0x60);
	GC0329_write_cmos_sensor(0x7a, 0x58);
	GC0329_write_cmos_sensor(0x7b, 0x20);
	GC0329_write_cmos_sensor(0x7c, 0x30);
	GC0329_write_cmos_sensor(0x7d, 0x35);
	GC0329_write_cmos_sensor(0x7e, 0x10);
	GC0329_write_cmos_sensor(0x7f, 0x08);

	////////////////////ABS////////////////////
	GC0329_write_cmos_sensor(0x9c, 0x02); 
	GC0329_write_cmos_sensor(0x9d, 0x20);
	//GC0329_write_cmos_sensor(0x9f, 0x40);	

	////////////////////CC-AWB////////////////////
	GC0329_write_cmos_sensor(0xd0, 0x00);
	GC0329_write_cmos_sensor(0xd2, 0x2c);
	GC0329_write_cmos_sensor(0xd3, 0x80); 

	////////////////////LSC///////////////////
	GC0329_SET_PAGE1;
	GC0329_write_cmos_sensor(0xa0, 0x00);
	GC0329_write_cmos_sensor(0xa1, 0x3c);
	GC0329_write_cmos_sensor(0xa2, 0x50);
	GC0329_write_cmos_sensor(0xa3, 0x00);
	GC0329_write_cmos_sensor(0xa8, 0x0f);
	GC0329_write_cmos_sensor(0xa9, 0x08);
	GC0329_write_cmos_sensor(0xaa, 0x00);
	GC0329_write_cmos_sensor(0xab, 0x04);
	GC0329_write_cmos_sensor(0xac, 0x00);
	GC0329_write_cmos_sensor(0xad, 0x07);
	GC0329_write_cmos_sensor(0xae, 0x0e);
	GC0329_write_cmos_sensor(0xaf, 0x00);
	GC0329_write_cmos_sensor(0xb0, 0x00);
	GC0329_write_cmos_sensor(0xb1, 0x09);
	GC0329_write_cmos_sensor(0xb2, 0x00);
	GC0329_write_cmos_sensor(0xb3, 0x00);
	GC0329_write_cmos_sensor(0xb4, 0x31);
	GC0329_write_cmos_sensor(0xb5, 0x19);
	GC0329_write_cmos_sensor(0xb6, 0x24);
	GC0329_write_cmos_sensor(0xba, 0x3a);
	GC0329_write_cmos_sensor(0xbb, 0x24);
	GC0329_write_cmos_sensor(0xbc, 0x2a);
	GC0329_write_cmos_sensor(0xc0, 0x17);
	GC0329_write_cmos_sensor(0xc1, 0x13);
	GC0329_write_cmos_sensor(0xc2, 0x17);
	GC0329_write_cmos_sensor(0xc6, 0x21);
	GC0329_write_cmos_sensor(0xc7, 0x1c);
	GC0329_write_cmos_sensor(0xc8, 0x1c);
	GC0329_write_cmos_sensor(0xb7, 0x00);
	GC0329_write_cmos_sensor(0xb8, 0x00);
	GC0329_write_cmos_sensor(0xb9, 0x00);
	GC0329_write_cmos_sensor(0xbd, 0x00);
	GC0329_write_cmos_sensor(0xbe, 0x00);
	GC0329_write_cmos_sensor(0xbf, 0x00);
	GC0329_write_cmos_sensor(0xc3, 0x00);
	GC0329_write_cmos_sensor(0xc4, 0x00);
	GC0329_write_cmos_sensor(0xc5, 0x00);
	GC0329_write_cmos_sensor(0xc9, 0x00);
	GC0329_write_cmos_sensor(0xca, 0x00);
	GC0329_write_cmos_sensor(0xcb, 0x00);
	GC0329_write_cmos_sensor(0xa4, 0x00);
	GC0329_write_cmos_sensor(0xa5, 0x00);
	GC0329_write_cmos_sensor(0xa6, 0x00);
	GC0329_write_cmos_sensor(0xa7, 0x00);
	GC0329_SET_PAGE0;	

	////////////////////asde ///////////////////
	//GC0329_write_cmos_sensor(0xa0, 0xaf);
	//GC0329_write_cmos_sensor(0xa2, 0xff);

	GC0329_write_cmos_sensor(0x44, 0xa2); //a2

}


/*************************************************************************
* FUNCTION
*	GC329_Lens_Select
*
* DESCRIPTION
*	This function is served for FAE to select the appropriate lens parameter.
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
void GC0329_Lens_Select(kal_uint8 Lens_Tag)
{
	switch(Lens_Tag)
	{
		case CHT_806C_2:
			GC0329_write_cmos_sensor(0xfe, 0x01);
			GC0329_write_cmos_sensor(0xa0, 0x00);
			GC0329_write_cmos_sensor(0xa1, 0x3c);
			GC0329_write_cmos_sensor(0xa2, 0x50);
			GC0329_write_cmos_sensor(0xa3, 0x00);
			GC0329_write_cmos_sensor(0xa4, 0x00);
			GC0329_write_cmos_sensor(0xa5, 0x00);
			GC0329_write_cmos_sensor(0xa6, 0x00);
			GC0329_write_cmos_sensor(0xa7, 0x04);
			
			GC0329_write_cmos_sensor(0xa8, 0x0f);
			GC0329_write_cmos_sensor(0xa9, 0x08);
			GC0329_write_cmos_sensor(0xaa, 0x00);
			GC0329_write_cmos_sensor(0xab, 0x04);
			GC0329_write_cmos_sensor(0xac, 0x00);
			GC0329_write_cmos_sensor(0xad, 0x07);
			GC0329_write_cmos_sensor(0xae, 0x0e);
			GC0329_write_cmos_sensor(0xaf, 0x00);
			GC0329_write_cmos_sensor(0xb0, 0x00);
			GC0329_write_cmos_sensor(0xb1, 0x09);
			GC0329_write_cmos_sensor(0xb2, 0x00);
			GC0329_write_cmos_sensor(0xb3, 0x00);

			GC0329_write_cmos_sensor(0xb4, 0x30);
			GC0329_write_cmos_sensor(0xb5, 0x19);
			GC0329_write_cmos_sensor(0xb6, 0x21);
			GC0329_write_cmos_sensor(0xba, 0x3e);
			GC0329_write_cmos_sensor(0xbb, 0x26);
			GC0329_write_cmos_sensor(0xbc, 0x2f);
			GC0329_write_cmos_sensor(0xc0, 0x15);
			GC0329_write_cmos_sensor(0xc1, 0x11);
			GC0329_write_cmos_sensor(0xc2, 0x15);
			GC0329_write_cmos_sensor(0xc6, 0x1f);
			GC0329_write_cmos_sensor(0xc7, 0x16);
			GC0329_write_cmos_sensor(0xc8, 0x16);

			GC0329_write_cmos_sensor(0xb7, 0x00);
			GC0329_write_cmos_sensor(0xb8, 0x00);
			GC0329_write_cmos_sensor(0xb9, 0x00);
			GC0329_write_cmos_sensor(0xbd, 0x00);
			GC0329_write_cmos_sensor(0xbe, 0x00);
			GC0329_write_cmos_sensor(0xbf, 0x00);
			GC0329_write_cmos_sensor(0xc3, 0x00);
			GC0329_write_cmos_sensor(0xc4, 0x00);
			GC0329_write_cmos_sensor(0xc5, 0x00);
			GC0329_write_cmos_sensor(0xc9, 0x0d);
			GC0329_write_cmos_sensor(0xca, 0x00);
			GC0329_write_cmos_sensor(0xcb, 0x00);
			
			GC0329_write_cmos_sensor(0xfe, 0x00);
			break;

		case CHT_808C_2:
			GC0329_write_cmos_sensor(0xfe, 0x01);
			GC0329_write_cmos_sensor(0xa0, 0x00);
			GC0329_write_cmos_sensor(0xa1, 0x3c);
			GC0329_write_cmos_sensor(0xa2, 0x50);
			GC0329_write_cmos_sensor(0xa3, 0x00);
			GC0329_write_cmos_sensor(0xa4, 0x00);
			GC0329_write_cmos_sensor(0xa5, 0x02);
			GC0329_write_cmos_sensor(0xa6, 0x00);
			GC0329_write_cmos_sensor(0xa7, 0x00);

			GC0329_write_cmos_sensor(0xa8, 0x0c);
			GC0329_write_cmos_sensor(0xa9, 0x03);
			GC0329_write_cmos_sensor(0xaa, 0x00);
			GC0329_write_cmos_sensor(0xab, 0x05);
			GC0329_write_cmos_sensor(0xac, 0x01);
			GC0329_write_cmos_sensor(0xad, 0x07);
			GC0329_write_cmos_sensor(0xae, 0x0e);
			GC0329_write_cmos_sensor(0xaf, 0x00);
			GC0329_write_cmos_sensor(0xb0, 0x00);
			GC0329_write_cmos_sensor(0xb1, 0x08);
			GC0329_write_cmos_sensor(0xb2, 0x02);
			GC0329_write_cmos_sensor(0xb3, 0x00);

			GC0329_write_cmos_sensor(0xb4, 0x30);
			GC0329_write_cmos_sensor(0xb5, 0x0f);
			GC0329_write_cmos_sensor(0xb6, 0x16);
			GC0329_write_cmos_sensor(0xba, 0x44);
			GC0329_write_cmos_sensor(0xbb, 0x24);
			GC0329_write_cmos_sensor(0xbc, 0x2a);
			GC0329_write_cmos_sensor(0xc0, 0x13);
			GC0329_write_cmos_sensor(0xc1, 0x0e);
			GC0329_write_cmos_sensor(0xc2, 0x11);
			GC0329_write_cmos_sensor(0xc6, 0x28);
			GC0329_write_cmos_sensor(0xc7, 0x21);
			GC0329_write_cmos_sensor(0xc8, 0x20);

			GC0329_write_cmos_sensor(0xb7, 0x00);
			GC0329_write_cmos_sensor(0xb8, 0x00);
			GC0329_write_cmos_sensor(0xb9, 0x01);
			GC0329_write_cmos_sensor(0xbd, 0x00);
			GC0329_write_cmos_sensor(0xbe, 0x00);
			GC0329_write_cmos_sensor(0xbf, 0x00);
			GC0329_write_cmos_sensor(0xc3, 0x00);
			GC0329_write_cmos_sensor(0xc4, 0x00);
			GC0329_write_cmos_sensor(0xc5, 0x00);
			GC0329_write_cmos_sensor(0xc9, 0x00);
			GC0329_write_cmos_sensor(0xca, 0x00);
			GC0329_write_cmos_sensor(0xcb, 0x00);

			GC0329_write_cmos_sensor(0xfe, 0x00);
			break;
			
		case LY_982A_H114:
			GC0329_write_cmos_sensor(0xfe, 0x01);
			GC0329_write_cmos_sensor(0xa0, 0x00);
			GC0329_write_cmos_sensor(0xa1, 0x3c);
			GC0329_write_cmos_sensor(0xa2, 0x50);
			GC0329_write_cmos_sensor(0xa3, 0x00);
			GC0329_write_cmos_sensor(0xa4, 0x00);
			GC0329_write_cmos_sensor(0xa5, 0x00);
			GC0329_write_cmos_sensor(0xa6, 0x00);
			GC0329_write_cmos_sensor(0xa7, 0x00);

			GC0329_write_cmos_sensor(0xa8, 0x0c);
			GC0329_write_cmos_sensor(0xa9, 0x06);
			GC0329_write_cmos_sensor(0xaa, 0x02);
			GC0329_write_cmos_sensor(0xab, 0x13);
			GC0329_write_cmos_sensor(0xac, 0x06);
			GC0329_write_cmos_sensor(0xad, 0x05);
			GC0329_write_cmos_sensor(0xae, 0x0b);
			GC0329_write_cmos_sensor(0xaf, 0x03);
			GC0329_write_cmos_sensor(0xb0, 0x00);
			GC0329_write_cmos_sensor(0xb1, 0x08);
			GC0329_write_cmos_sensor(0xb2, 0x01);
			GC0329_write_cmos_sensor(0xb3, 0x00);

			GC0329_write_cmos_sensor(0xb4, 0x34);
			GC0329_write_cmos_sensor(0xb5, 0x29);
			GC0329_write_cmos_sensor(0xb6, 0x2e);
			GC0329_write_cmos_sensor(0xba, 0x30);
			GC0329_write_cmos_sensor(0xbb, 0x24);
			GC0329_write_cmos_sensor(0xbc, 0x28);
			GC0329_write_cmos_sensor(0xc0, 0x1c);
			GC0329_write_cmos_sensor(0xc1, 0x19);
			GC0329_write_cmos_sensor(0xc2, 0x19);
			GC0329_write_cmos_sensor(0xc6, 0x1a);
			GC0329_write_cmos_sensor(0xc7, 0x19);
			GC0329_write_cmos_sensor(0xc8, 0x1b);

			GC0329_write_cmos_sensor(0xb7, 0x01);
			GC0329_write_cmos_sensor(0xb8, 0x01);
			GC0329_write_cmos_sensor(0xb9, 0x00);
			GC0329_write_cmos_sensor(0xbd, 0x00);
			GC0329_write_cmos_sensor(0xbe, 0x00);
			GC0329_write_cmos_sensor(0xbf, 0x00);
			GC0329_write_cmos_sensor(0xc3, 0x00);
			GC0329_write_cmos_sensor(0xc4, 0x00);
			GC0329_write_cmos_sensor(0xc5, 0x03);
			GC0329_write_cmos_sensor(0xc9, 0x00);
			GC0329_write_cmos_sensor(0xca, 0x00);
			GC0329_write_cmos_sensor(0xcb, 0x00);

			GC0329_write_cmos_sensor(0xfe, 0x00);
			break;

		case XY_046A:
			GC0329_write_cmos_sensor(0xfe, 0x01);
			GC0329_write_cmos_sensor(0xa0, 0x00);
			GC0329_write_cmos_sensor(0xa1, 0x3c);
			GC0329_write_cmos_sensor(0xa2, 0x50);
			GC0329_write_cmos_sensor(0xa3, 0x00);
			GC0329_write_cmos_sensor(0xa4, 0x00);
			GC0329_write_cmos_sensor(0xa5, 0x00);
			GC0329_write_cmos_sensor(0xa6, 0x10);
			GC0329_write_cmos_sensor(0xa7, 0x00);

			GC0329_write_cmos_sensor(0xa8, 0x11);
			GC0329_write_cmos_sensor(0xa9, 0x0a);
			GC0329_write_cmos_sensor(0xaa, 0x05);
			GC0329_write_cmos_sensor(0xab, 0x04);
			GC0329_write_cmos_sensor(0xac, 0x03);
			GC0329_write_cmos_sensor(0xad, 0x00);
			GC0329_write_cmos_sensor(0xae, 0x08);
			GC0329_write_cmos_sensor(0xaf, 0x01);
			GC0329_write_cmos_sensor(0xb0, 0x00);
			GC0329_write_cmos_sensor(0xb1, 0x09);
			GC0329_write_cmos_sensor(0xb2, 0x02);
			GC0329_write_cmos_sensor(0xb3, 0x03);

			GC0329_write_cmos_sensor(0xb4, 0x2e);
			GC0329_write_cmos_sensor(0xb5, 0x16);
			GC0329_write_cmos_sensor(0xb6, 0x24);
			GC0329_write_cmos_sensor(0xba, 0x3a);
			GC0329_write_cmos_sensor(0xbb, 0x1e);
			GC0329_write_cmos_sensor(0xbc, 0x24);
			GC0329_write_cmos_sensor(0xc0, 0x09);
			GC0329_write_cmos_sensor(0xc1, 0x02);
			GC0329_write_cmos_sensor(0xc2, 0x06);
			GC0329_write_cmos_sensor(0xc6, 0x25);
			GC0329_write_cmos_sensor(0xc7, 0x21);
			GC0329_write_cmos_sensor(0xc8, 0x23);

			GC0329_write_cmos_sensor(0xb7, 0x00);
			GC0329_write_cmos_sensor(0xb8, 0x00);
			GC0329_write_cmos_sensor(0xb9, 0x0f);
			GC0329_write_cmos_sensor(0xbd, 0x00);
			GC0329_write_cmos_sensor(0xbe, 0x00);
			GC0329_write_cmos_sensor(0xbf, 0x00);
			GC0329_write_cmos_sensor(0xc3, 0x00);
			GC0329_write_cmos_sensor(0xc4, 0x00);
			GC0329_write_cmos_sensor(0xc5, 0x00);
			GC0329_write_cmos_sensor(0xc9, 0x00);
			GC0329_write_cmos_sensor(0xca, 0x00);
			GC0329_write_cmos_sensor(0xcb, 0x00);

			GC0329_write_cmos_sensor(0xfe, 0x00);
			break;

		case XY_0620:
			GC0329_write_cmos_sensor(0xfe, 0x01);
			GC0329_write_cmos_sensor(0xa0, 0x00);
			GC0329_write_cmos_sensor(0xa1, 0x3c);
			GC0329_write_cmos_sensor(0xa2, 0x50);
			GC0329_write_cmos_sensor(0xa3, 0x00);
			GC0329_write_cmos_sensor(0xa4, 0x00);
			GC0329_write_cmos_sensor(0xa5, 0x00);
			GC0329_write_cmos_sensor(0xa6, 0x00);
			GC0329_write_cmos_sensor(0xa7, 0x00);

			GC0329_write_cmos_sensor(0xa8, 0x0f);
			GC0329_write_cmos_sensor(0xa9, 0x06);
			GC0329_write_cmos_sensor(0xaa, 0x00);
			GC0329_write_cmos_sensor(0xab, 0x07);
			GC0329_write_cmos_sensor(0xac, 0x05);
			GC0329_write_cmos_sensor(0xad, 0x08);
			GC0329_write_cmos_sensor(0xae, 0x13);
			GC0329_write_cmos_sensor(0xaf, 0x06);
			GC0329_write_cmos_sensor(0xb0, 0x00);
			GC0329_write_cmos_sensor(0xb1, 0x06);
			GC0329_write_cmos_sensor(0xb2, 0x01);
			GC0329_write_cmos_sensor(0xb3, 0x04);

			GC0329_write_cmos_sensor(0xb4, 0x2d);
			GC0329_write_cmos_sensor(0xb5, 0x18);
			GC0329_write_cmos_sensor(0xb6, 0x22);
			GC0329_write_cmos_sensor(0xba, 0x45);
			GC0329_write_cmos_sensor(0xbb, 0x2d);
			GC0329_write_cmos_sensor(0xbc, 0x34);
			GC0329_write_cmos_sensor(0xc0, 0x16);
			GC0329_write_cmos_sensor(0xc1, 0x13);
			GC0329_write_cmos_sensor(0xc2, 0x19);
			GC0329_write_cmos_sensor(0xc6, 0x21);
			GC0329_write_cmos_sensor(0xc7, 0x1c);
			GC0329_write_cmos_sensor(0xc8, 0x18);

			GC0329_write_cmos_sensor(0xb7, 0x00);
			GC0329_write_cmos_sensor(0xb8, 0x00);
			GC0329_write_cmos_sensor(0xb9, 0x00);
			GC0329_write_cmos_sensor(0xbd, 0x00);
			GC0329_write_cmos_sensor(0xbe, 0x00);
			GC0329_write_cmos_sensor(0xbf, 0x08);
			GC0329_write_cmos_sensor(0xc3, 0x00);
			GC0329_write_cmos_sensor(0xc4, 0x00);
			GC0329_write_cmos_sensor(0xc5, 0x01);
			GC0329_write_cmos_sensor(0xc9, 0x00);
			GC0329_write_cmos_sensor(0xca, 0x00);
			GC0329_write_cmos_sensor(0xcb, 0x10);

			GC0329_write_cmos_sensor(0xfe, 0x00);
			break;

		case XY_078V: 
			GC0329_write_cmos_sensor(0xfe, 0x01);
			GC0329_write_cmos_sensor(0xa0, 0x00);
			GC0329_write_cmos_sensor(0xa1, 0x3c);
			GC0329_write_cmos_sensor(0xa2, 0x50);
			GC0329_write_cmos_sensor(0xa3, 0x00);
			GC0329_write_cmos_sensor(0xa4, 0x00);
			GC0329_write_cmos_sensor(0xa5, 0x00);
			GC0329_write_cmos_sensor(0xa6, 0x00);
			GC0329_write_cmos_sensor(0xa7, 0x00);

			GC0329_write_cmos_sensor(0xa8, 0x14);
			GC0329_write_cmos_sensor(0xa9, 0x08);
			GC0329_write_cmos_sensor(0xaa, 0x0a);
			GC0329_write_cmos_sensor(0xab, 0x11);
			GC0329_write_cmos_sensor(0xac, 0x05);
			GC0329_write_cmos_sensor(0xad, 0x07);
			GC0329_write_cmos_sensor(0xae, 0x0b);
			GC0329_write_cmos_sensor(0xaf, 0x03);
			GC0329_write_cmos_sensor(0xb0, 0x00);
			GC0329_write_cmos_sensor(0xb1, 0x09);
			GC0329_write_cmos_sensor(0xb2, 0x04);
			GC0329_write_cmos_sensor(0xb3, 0x01);

			GC0329_write_cmos_sensor(0xb4, 0x2f);
			GC0329_write_cmos_sensor(0xb5, 0x2a);
			GC0329_write_cmos_sensor(0xb6, 0x2c);
			GC0329_write_cmos_sensor(0xba, 0x3a);
			GC0329_write_cmos_sensor(0xbb, 0x2b);
			GC0329_write_cmos_sensor(0xbc, 0x32);
			GC0329_write_cmos_sensor(0xc0, 0x1b);
			GC0329_write_cmos_sensor(0xc1, 0x18);
			GC0329_write_cmos_sensor(0xc2, 0x1a);
			GC0329_write_cmos_sensor(0xc6, 0x12);
			GC0329_write_cmos_sensor(0xc7, 0x10);
			GC0329_write_cmos_sensor(0xc8, 0x12);

			GC0329_write_cmos_sensor(0xb7, 0x0a);
			GC0329_write_cmos_sensor(0xb8, 0x00);
			GC0329_write_cmos_sensor(0xb9, 0x00);
			GC0329_write_cmos_sensor(0xbd, 0x00);
			GC0329_write_cmos_sensor(0xbe, 0x00);
			GC0329_write_cmos_sensor(0xbf, 0x00);
			GC0329_write_cmos_sensor(0xc3, 0x00);
			GC0329_write_cmos_sensor(0xc4, 0x00);
			GC0329_write_cmos_sensor(0xc5, 0x00);
			GC0329_write_cmos_sensor(0xc9, 0x0d);
			GC0329_write_cmos_sensor(0xca, 0x00);
			GC0329_write_cmos_sensor(0xcb, 0x00);

			GC0329_write_cmos_sensor(0xfe, 0x00);
			break;

		case YG1001A_F:
			GC0329_write_cmos_sensor(0xfe, 0x01);
			GC0329_write_cmos_sensor(0xa0, 0x00);
			GC0329_write_cmos_sensor(0xa1, 0x3c);
			GC0329_write_cmos_sensor(0xa2, 0x50);
			GC0329_write_cmos_sensor(0xa3, 0x00);
			GC0329_write_cmos_sensor(0xa4, 0x00);
			GC0329_write_cmos_sensor(0xa5, 0x00);
			GC0329_write_cmos_sensor(0xa6, 0x00);
			GC0329_write_cmos_sensor(0xa7, 0x00);

			GC0329_write_cmos_sensor(0xa8, 0x0e);
			GC0329_write_cmos_sensor(0xa9, 0x05);
			GC0329_write_cmos_sensor(0xaa, 0x01);
			GC0329_write_cmos_sensor(0xab, 0x07);
			GC0329_write_cmos_sensor(0xac, 0x00);
			GC0329_write_cmos_sensor(0xad, 0x07);
			GC0329_write_cmos_sensor(0xae, 0x0e);
			GC0329_write_cmos_sensor(0xaf, 0x02);
			GC0329_write_cmos_sensor(0xb0, 0x00);
			GC0329_write_cmos_sensor(0xb1, 0x0d);
			GC0329_write_cmos_sensor(0xb2, 0x00);
			GC0329_write_cmos_sensor(0xb3, 0x00);

			GC0329_write_cmos_sensor(0xb4, 0x2a);
			GC0329_write_cmos_sensor(0xb5, 0x0f);
			GC0329_write_cmos_sensor(0xb6, 0x14);
			GC0329_write_cmos_sensor(0xba, 0x40);
			GC0329_write_cmos_sensor(0xbb, 0x26);
			GC0329_write_cmos_sensor(0xbc, 0x2a);
			GC0329_write_cmos_sensor(0xc0, 0x0e);
			GC0329_write_cmos_sensor(0xc1, 0x0a);
			GC0329_write_cmos_sensor(0xc2, 0x0d);
			GC0329_write_cmos_sensor(0xc6, 0x27);
			GC0329_write_cmos_sensor(0xc7, 0x20);
			GC0329_write_cmos_sensor(0xc8, 0x1f);

			GC0329_write_cmos_sensor(0xb7, 0x00);
			GC0329_write_cmos_sensor(0xb8, 0x00);
			GC0329_write_cmos_sensor(0xb9, 0x00);
			GC0329_write_cmos_sensor(0xbd, 0x00);
			GC0329_write_cmos_sensor(0xbe, 0x00);
			GC0329_write_cmos_sensor(0xbf, 0x00);
			GC0329_write_cmos_sensor(0xc3, 0x00);
			GC0329_write_cmos_sensor(0xc4, 0x00);
			GC0329_write_cmos_sensor(0xc5, 0x00);
			GC0329_write_cmos_sensor(0xc9, 0x00);
			GC0329_write_cmos_sensor(0xca, 0x00);
			GC0329_write_cmos_sensor(0xcb, 0x00);

			GC0329_write_cmos_sensor(0xfe, 0x00);
			break;

		default:
			break;
	}
}



UINT32 GC0329GetSensorID(UINT32 *sensorID)
{
    kal_uint16 sensor_id=0;
    int i;

    GC0329_write_cmos_sensor(0xfc, 0x16);
    Sleep(20);

    do
    {
        	// check if sensor ID correct
        	for(i = 0; i < 3; i++)
		{
	            	sensor_id = GC0329_read_cmos_sensor(0x00);
	            	printk("GC0329 Sensor id = %x\n", sensor_id);
	            	if (sensor_id == GC0329_SENSOR_ID)
			{
	               	break;
	            	}
        	}
        	mdelay(50);
    }while(0);

    if(sensor_id != GC0329_SENSOR_ID)
    {
        SENSORDB("GC0329 Sensor id read failed, ID = %x\n", sensor_id);
        return ERROR_SENSOR_CONNECT_FAIL;
    }

    *sensorID = sensor_id;

    RETAILMSG(1, (TEXT("Sensor Read ID OK \r\n")));
	
    return ERROR_NONE;
}




/*************************************************************************
* FUNCTION
*	GC0329_Write_More_Registers
*
* DESCRIPTION
*	This function is served for FAE to modify the necessary Init Regs. Do not modify the regs
*     in init_GC0329() directly.
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
void GC0329_Write_More_Registers(void)
{
	////////////20120427/////////////////////////
	GC0329_write_cmos_sensor(0xfe,0x01); 																																
	GC0329_write_cmos_sensor(0x18,0x22); 																																
	GC0329_write_cmos_sensor(0x21,0xc0); 																																
	GC0329_write_cmos_sensor(0x06,0x12);																																																																									
	GC0329_write_cmos_sensor(0x08,0x9c);																																																																									
	GC0329_write_cmos_sensor(0x51,0x28);																																																																									
	GC0329_write_cmos_sensor(0x52,0x10);																																																																									
	GC0329_write_cmos_sensor(0x53,0x20);																																																																									
	GC0329_write_cmos_sensor(0x54,0x40);																																																																									
	GC0329_write_cmos_sensor(0x55,0x16);																																																																									
	GC0329_write_cmos_sensor(0x56,0x30);																																																																									
	GC0329_write_cmos_sensor(0x58,0x60);																																																																									
	GC0329_write_cmos_sensor(0x59,0x08);																																																																									
	GC0329_write_cmos_sensor(0x5c,0x35);																																																																									
	GC0329_write_cmos_sensor(0x5d,0x72);																																																																									
	GC0329_write_cmos_sensor(0x67,0x80);																																																																									
	GC0329_write_cmos_sensor(0x68,0x60);																																																																									
	GC0329_write_cmos_sensor(0x69,0x90);																																																																									
	GC0329_write_cmos_sensor(0x6c,0x30);																																																																									
	GC0329_write_cmos_sensor(0x6d,0x60);																																																																									
	GC0329_write_cmos_sensor(0x70,0x10);	
																																																																									
	GC0329_write_cmos_sensor(0xfe,0x00);																																																																									
	GC0329_write_cmos_sensor(0x9c,0x0a);																																																																									
	GC0329_write_cmos_sensor(0xa0,0xaf);																																																																									
	GC0329_write_cmos_sensor(0xa2,0xff);																																																																									
	GC0329_write_cmos_sensor(0xa4,0x60);																																																																									
	GC0329_write_cmos_sensor(0xa5,0x31);																																																																									
	GC0329_write_cmos_sensor(0xa7,0x35);																																																																									
	GC0329_write_cmos_sensor(0x42,0xfe);																																																																									
	GC0329_write_cmos_sensor(0xd1,0x2e);																																																																									
	GC0329_write_cmos_sensor(0xd2,0x2e);
///////////////////////////////////////////////
	GC0329_write_cmos_sensor(0x1f,0x08);																																																																									
	GC0329_write_cmos_sensor(0x22,0xba);	
	GC0329_write_cmos_sensor(0xfe,0x00);	
	///////////////////////////////////////////////
    	GC0329GammaSelect(GC0329_RGB_Gamma_m6);//0:use default
    	GC0329_Lens_Select(0);//0:use default
}


/*************************************************************************
 * FUNCTION
 *	GC0329Open
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
UINT32 GC0329Open(void)
{
    kal_uint16 sensor_id=0;
    int i;

    GC0329_write_cmos_sensor(0xfc, 0x16);
    Sleep(20);

    do
    {
        	// check if sensor ID correct
        	for(i = 0; i < 3; i++)
		{
	            	sensor_id = GC0329_read_cmos_sensor(0x00);
	            	printk("GC0329 Sensor id = %x\n", sensor_id);
	            	if (sensor_id == GC0329_SENSOR_ID)
			{
	               	break;
	            	}
        	}
        	mdelay(50);
    }while(0);

    if(sensor_id != GC0329_SENSOR_ID)
    {
        SENSORDB("GC0329 Sensor id read failed, ID = %x\n", sensor_id);
        return ERROR_SENSOR_CONNECT_FAIL;
    }

    RETAILMSG(1, (TEXT("Sensor Read ID OK \r\n")));
    // initail sequence write in
    GC0329_Sensor_Init();
    GC0329_Write_More_Registers();
	
    return ERROR_NONE;
} /* GC0329Open */


/*************************************************************************
 * FUNCTION
 *	GC0329Close
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
UINT32 GC0329Close(void)
{
    return ERROR_NONE;
} /* GC0329Close */


/*************************************************************************
 * FUNCTION
 * GC0329Preview
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
UINT32 GC0329Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
        MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)

{
    //kal_uint32 iTemp;
    //kal_uint16 iStartX = 0, iStartY = 1;
	printk("sym GC0329Preview\n");
	#if 0
	if(mt_set_gpio_mode(138,0)){printk(" set flashlight gpio mode failed!! \n");}
	if(mt_set_gpio_dir(138,1)){printk("[CAMERA LENS] set gpio dir failed!! \n");}
			 if(mt_set_gpio_out(138,1)){printk("[CAMERA LENS] set gpio failed!! \n");}  //sym modify
			 msleep(3);
#endif
    if(sensor_config_data->SensorOperationMode == MSDK_SENSOR_OPERATION_MODE_VIDEO)		// MPEG4 Encode Mode
    {
        RETAILMSG(1, (TEXT("Camera Video preview\r\n")));
       // GC0329_MPEG4_encode_mode = KAL_TRUE;
       
    }
    else
    {
        RETAILMSG(1, (TEXT("Camera preview\r\n")));
       // GC0329_MPEG4_encode_mode = KAL_FALSE;
    }

    image_window->GrabStartX= IMAGE_SENSOR_VGA_GRAB_PIXELS;
    image_window->GrabStartY= IMAGE_SENSOR_VGA_GRAB_LINES;
    image_window->ExposureWindowWidth = IMAGE_SENSOR_PV_WIDTH;
    image_window->ExposureWindowHeight =IMAGE_SENSOR_PV_HEIGHT;

    // copy sensor_config_data
    memcpy(&GC0329SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
} /* GC0329Preview */


/*************************************************************************
 * FUNCTION
 *	GC0329Capture
 *
 * DESCRIPTION
 *	This function setup the CMOS sensor in capture MY_OUTPUT mode
 *
 * PARAMETERS
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
UINT32 GC0329Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
        MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)

{
    GC0329_MODE_CAPTURE=KAL_TRUE;
printk("sym GC0329Capture\n");
#if 0
	if(mt_set_gpio_mode(138,0)){printk(" set flashlight gpio mode failed!! \n");}
	if(mt_set_gpio_dir(138,1)){printk("[CAMERA LENS] set gpio dir failed!! \n");}
			 if(mt_set_gpio_out(138,0)){printk("[CAMERA LENS] set gpio failed!! \n");}  //sym modify
			 msleep(3);
			 #endif
    image_window->GrabStartX = IMAGE_SENSOR_VGA_GRAB_PIXELS;
    image_window->GrabStartY = IMAGE_SENSOR_VGA_GRAB_LINES;
    image_window->ExposureWindowWidth= IMAGE_SENSOR_FULL_WIDTH;
    image_window->ExposureWindowHeight = IMAGE_SENSOR_FULL_HEIGHT;

    // copy sensor_config_data
    memcpy(&GC0329SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
} /* GC0329_Capture() */



UINT32 GC0329GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
    pSensorResolution->SensorFullWidth=IMAGE_SENSOR_FULL_WIDTH;
    pSensorResolution->SensorFullHeight=IMAGE_SENSOR_FULL_HEIGHT;
    pSensorResolution->SensorPreviewWidth=IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorPreviewHeight=IMAGE_SENSOR_PV_HEIGHT;
    pSensorResolution->SensorVideoWidth=IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorVideoHeight=IMAGE_SENSOR_PV_HEIGHT;
    return ERROR_NONE;
} /* GC0329GetResolution() */


UINT32 GC0329GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
        MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
        MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    pSensorInfo->SensorPreviewResolutionX=IMAGE_SENSOR_PV_WIDTH;
    pSensorInfo->SensorPreviewResolutionY=IMAGE_SENSOR_PV_HEIGHT;
    pSensorInfo->SensorFullResolutionX=IMAGE_SENSOR_FULL_WIDTH;
    pSensorInfo->SensorFullResolutionY=IMAGE_SENSOR_FULL_WIDTH;

    pSensorInfo->SensorCameraPreviewFrameRate=30;
    pSensorInfo->SensorVideoFrameRate=30;
    pSensorInfo->SensorStillCaptureFrameRate=10;
    pSensorInfo->SensorWebCamCaptureFrameRate=15;
    pSensorInfo->SensorResetActiveHigh=FALSE;
    pSensorInfo->SensorResetDelayCount=1;
    pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_YUYV;
    pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorInterruptDelayLines = 1;
    pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_PARALLEL;
#if 0
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].MaxWidth=CAM_SIZE_5M_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].MaxHeight=CAM_SIZE_5M_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].BinningEnable=FALSE;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].MaxWidth=CAM_SIZE_5M_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].MaxHeight=CAM_SIZE_5M_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].BinningEnable=FALSE;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].MaxWidth=CAM_SIZE_5M_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].MaxHeight=CAM_SIZE_5M_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].BinningEnable=FALSE;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].MaxWidth=CAM_SIZE_1M_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].MaxHeight=CAM_SIZE_1M_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].BinningEnable=FALSE;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxWidth=CAM_SIZE_1M_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxHeight=CAM_SIZE_1M_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].BinningEnable=FALSE;
    pSensorInfo->CaptureDelayFrame = 1;
    pSensorInfo->PreviewDelayFrame = 0;
    pSensorInfo->VideoDelayFrame = 4;
    pSensorInfo->SensorMasterClockSwitch = 0;
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_2MA;
#endif
    switch (ScenarioId)
    {
    case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
   // case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
        pSensorInfo->SensorClockFreq=12;
        pSensorInfo->SensorClockDividCount=	3;
        pSensorInfo->SensorClockRisingCount= 0;
        pSensorInfo->SensorClockFallingCount= 2;
        pSensorInfo->SensorPixelClockCount= 3;
        pSensorInfo->SensorDataLatchCount= 2;
        pSensorInfo->SensorGrabStartX = IMAGE_SENSOR_VGA_GRAB_PIXELS;
        pSensorInfo->SensorGrabStartY = IMAGE_SENSOR_VGA_GRAB_LINES;

        break;
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
  //  case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
        pSensorInfo->SensorClockFreq=12;
        pSensorInfo->SensorClockDividCount= 3;
        pSensorInfo->SensorClockRisingCount=0;
        pSensorInfo->SensorClockFallingCount=2;
        pSensorInfo->SensorPixelClockCount=3;
        pSensorInfo->SensorDataLatchCount=2;
        pSensorInfo->SensorGrabStartX = IMAGE_SENSOR_VGA_GRAB_PIXELS;
        pSensorInfo->SensorGrabStartY = IMAGE_SENSOR_VGA_GRAB_LINES;
        break;
    default:
        pSensorInfo->SensorClockFreq=12;
        pSensorInfo->SensorClockDividCount= 3;
        pSensorInfo->SensorClockRisingCount=0;
        pSensorInfo->SensorClockFallingCount=2;
        pSensorInfo->SensorPixelClockCount=3;
        pSensorInfo->SensorDataLatchCount=2;
        pSensorInfo->SensorGrabStartX = IMAGE_SENSOR_VGA_GRAB_PIXELS;
        pSensorInfo->SensorGrabStartY = IMAGE_SENSOR_VGA_GRAB_LINES;
        break;
    }
    GC0329PixelClockDivider=pSensorInfo->SensorPixelClockCount;
    memcpy(pSensorConfigData, &GC0329SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
} /* GC0329GetInfo() */


UINT32 GC0329Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
        MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    switch (ScenarioId)
    {
    case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
   // case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
        GC0329Preview(pImageWindow, pSensorConfigData);
        break;
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
 //   case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
        GC0329Capture(pImageWindow, pSensorConfigData);
        break;
	default:
		break;
    }


    return TRUE;
}	/* GC0329Control() */

BOOL GC0329_set_param_wb(UINT16 para)
{

	switch (para)
	{
		case AWB_MODE_OFF:

		break;
		
		case AWB_MODE_AUTO:
			GC0329_write_cmos_sensor(0x77, 0x57);
			GC0329_write_cmos_sensor(0x78, 0x4d);
			GC0329_write_cmos_sensor(0x79, 0x45);
			GC0329_awb_enable(KAL_TRUE);
		break;
		
		case AWB_MODE_CLOUDY_DAYLIGHT: //cloudy
			GC0329_awb_enable(KAL_FALSE);
			GC0329_write_cmos_sensor(0x77, 0x8c); //WB_manual_gain 
			GC0329_write_cmos_sensor(0x78, 0x50);
			GC0329_write_cmos_sensor(0x79, 0x40);
		break;
		
		case AWB_MODE_DAYLIGHT: //sunny
			GC0329_awb_enable(KAL_FALSE);
			GC0329_write_cmos_sensor(0x77, 0x74); 
			GC0329_write_cmos_sensor(0x78, 0x52);
			GC0329_write_cmos_sensor(0x79, 0x40);			
		break;
		
		case AWB_MODE_INCANDESCENT: //office
			GC0329_awb_enable(KAL_FALSE);
			GC0329_write_cmos_sensor(0x77, 0x48);
			GC0329_write_cmos_sensor(0x78, 0x40);
			GC0329_write_cmos_sensor(0x79, 0x5c);
		break;
		
		case AWB_MODE_TUNGSTEN: //home
			GC0329_awb_enable(KAL_FALSE);
			GC0329_write_cmos_sensor(0x77, 0x40);
			GC0329_write_cmos_sensor(0x78, 0x54);
			GC0329_write_cmos_sensor(0x79, 0x70);
		break;
		
		case AWB_MODE_FLUORESCENT:
			GC0329_awb_enable(KAL_FALSE);
			GC0329_write_cmos_sensor(0x77, 0x40);
			GC0329_write_cmos_sensor(0x78, 0x42);
			GC0329_write_cmos_sensor(0x79, 0x50);
		break;
		
		default:
		return FALSE;
	}
	spin_lock(&Gc0329_drv_lock);
    Gc0329CurrentStatus.iWB = para;
	spin_unlock(&Gc0329_drv_lock);
	return TRUE;
} /* GC0329_set_param_wb */


BOOL GC0329_set_param_effect(UINT16 para)
{
	kal_uint32  ret = KAL_TRUE;

	switch (para)
	{
		case MEFFECT_OFF:
			GC0329_write_cmos_sensor(0x43 , 0x00);
		break;
		
		case MEFFECT_SEPIA:
			GC0329_write_cmos_sensor(0x43 , 0x02);
			GC0329_write_cmos_sensor(0xda , 0xd0);
			GC0329_write_cmos_sensor(0xdb , 0x28);
		break;
		
		case MEFFECT_NEGATIVE:
			GC0329_write_cmos_sensor(0x43 , 0x01);
		break;
		
		case MEFFECT_SEPIAGREEN:
			GC0329_write_cmos_sensor(0x43 , 0x02);
			GC0329_write_cmos_sensor(0xda , 0xc0);
			GC0329_write_cmos_sensor(0xdb , 0xc0);
		break;
		
		case MEFFECT_SEPIABLUE:
			GC0329_write_cmos_sensor(0x43 , 0x02);
			GC0329_write_cmos_sensor(0xda , 0x50);
			GC0329_write_cmos_sensor(0xdb , 0xe0);
		break;

		case MEFFECT_MONO:
			GC0329_write_cmos_sensor(0x43 , 0x02);
			GC0329_write_cmos_sensor(0xda , 0x00);
			GC0329_write_cmos_sensor(0xdb , 0x00);
		break;
		default:
			ret = FALSE;
	}

	return ret;

} /* GC0329_set_param_effect */


BOOL GC0329_set_param_banding(UINT16 para)
{
	switch (para)
	{
		case AE_FLICKER_MODE_50HZ:
			GC0329_write_cmos_sensor(0x05, 0x01); 	
			GC0329_write_cmos_sensor(0x06, 0x32); 
			GC0329_write_cmos_sensor(0x07, 0x00);
			GC0329_write_cmos_sensor(0x08, 0x70);
			
			GC0329_SET_PAGE1;
			GC0329_write_cmos_sensor(0x29, 0x00);   //anti-flicker step [11:8]
			GC0329_write_cmos_sensor(0x2a, 0x3c);   //anti-flicker step [7:0]
			
			GC0329_write_cmos_sensor(0x2b, 0x02);   //exp level 0  14.28fps
			GC0329_write_cmos_sensor(0x2c, 0x58); 
			GC0329_write_cmos_sensor(0x2d, 0x02);   //exp level 1  12.50fps
			GC0329_write_cmos_sensor(0x2e, 0xd0); 
			GC0329_write_cmos_sensor(0x2f, 0x03);   //exp level 2  10.00fps
			GC0329_write_cmos_sensor(0x30, 0xc0); 
			GC0329_write_cmos_sensor(0x31, 0x04);   //exp level 3  7.14fps
			GC0329_write_cmos_sensor(0x32, 0xb0); 
			GC0329_SET_PAGE0;
			break;

		case AE_FLICKER_MODE_60HZ:
			GC0329_write_cmos_sensor(0x05, 0x01); 	
			GC0329_write_cmos_sensor(0x06, 0x32); 
			GC0329_write_cmos_sensor(0x07, 0x00);
			GC0329_write_cmos_sensor(0x08, 0x70);
			
			GC0329_SET_PAGE1;
			GC0329_write_cmos_sensor(0x29, 0x00);   //anti-flicker step [11:8]
			GC0329_write_cmos_sensor(0x2a, 0x32);   //anti-flicker step [7:0]
			
			GC0329_write_cmos_sensor(0x2b, 0x02);   //exp level 0  15.00fps
			GC0329_write_cmos_sensor(0x2c, 0x58); 
			GC0329_write_cmos_sensor(0x2d, 0x02);   //exp level 0  12.00fps
			GC0329_write_cmos_sensor(0x2e, 0xbc); 
			GC0329_write_cmos_sensor(0x2f, 0x03);   //exp level 0  10.00fps
			GC0329_write_cmos_sensor(0x30, 0x52); 
			GC0329_write_cmos_sensor(0x31, 0x04);   //exp level 0  7.05fps
			GC0329_write_cmos_sensor(0x32, 0xb0); 
			GC0329_SET_PAGE0;
		break;
		default:
		return FALSE;
	}

	return TRUE;
} /* GC0329_set_param_banding */


BOOL GC0329_set_param_exposure(UINT16 para)
{
	kal_uint8 value_luma, value_Y;
	value_luma = (GC0329_NIGHT_MODE?0x2b:0x00);
	value_Y = (GC0329_NIGHT_MODE?0x68:0x50);

	switch (para)
	{
		case AE_EV_COMP_n13:
			GC0329_write_cmos_sensor(0xd5, value_luma - 0x48);
			GC0329_write_cmos_sensor(0xfe, 0x01);
			GC0329_write_cmos_sensor(0x13, value_Y - 0x28);
			GC0329_write_cmos_sensor(0xfe, 0x00);
		break;
		
		case AE_EV_COMP_n10:
			GC0329_write_cmos_sensor(0xd5, value_luma - 0x30);
			GC0329_write_cmos_sensor(0xfe, 0x01);
			GC0329_write_cmos_sensor(0x13, value_Y - 0x18);
			GC0329_write_cmos_sensor(0xfe, 0x00);
		break;
		
		case AE_EV_COMP_n07:
			GC0329_write_cmos_sensor(0xd5, value_luma - 0x20);
			GC0329_write_cmos_sensor(0xfe, 0x01);
			GC0329_write_cmos_sensor(0x13, value_Y - 0x10);
			GC0329_write_cmos_sensor(0xfe, 0x00);
		break;
		
		case AE_EV_COMP_n03:
			GC0329_write_cmos_sensor(0xd5, value_luma - 0x10);
			GC0329_write_cmos_sensor(0xfe, 0x01);
			GC0329_write_cmos_sensor(0x13, value_Y - 0x08);
			GC0329_write_cmos_sensor(0xfe, 0x00);
		break;				
		
		case AE_EV_COMP_00:
			GC0329_write_cmos_sensor(0xd5, value_luma+0x06);
			GC0329_write_cmos_sensor(0xfe, 0x01);
			GC0329_write_cmos_sensor(0x13, value_Y);
			GC0329_write_cmos_sensor(0xfe, 0x00);
		break;

		case AE_EV_COMP_03:
			GC0329_write_cmos_sensor(0xd5, value_luma + 0x10);
			GC0329_write_cmos_sensor(0xfe, 0x01);
			GC0329_write_cmos_sensor(0x13, value_Y + 0x10);
			GC0329_write_cmos_sensor(0xfe, 0x00);
		break;
		
		case AE_EV_COMP_07:
			GC0329_write_cmos_sensor(0xd5, value_luma + 0x20);
			GC0329_write_cmos_sensor(0xfe, 0x01);
			GC0329_write_cmos_sensor(0x13, value_Y + 0x20);
			GC0329_write_cmos_sensor(0xfe, 0x00);
		break;
		
		case AE_EV_COMP_10:
			GC0329_write_cmos_sensor(0xd5, value_luma + 0x30);
			GC0329_write_cmos_sensor(0xfe, 0x01);
			GC0329_write_cmos_sensor(0x13, value_Y + 0x30);
			GC0329_write_cmos_sensor(0xfe, 0x00);
		break;
		
		case AE_EV_COMP_13:
			GC0329_write_cmos_sensor(0xd5, value_luma + 0x48);
			GC0329_write_cmos_sensor(0xfe, 0x01);
			GC0329_write_cmos_sensor(0x13, value_Y + 0x48);
			GC0329_write_cmos_sensor(0xfe, 0x00);
		break;
		default:
		return FALSE;
	}

	return TRUE;
} /* GC0329_set_param_exposure */


UINT32 GC0329YUVSensorSetting(FEATURE_ID iCmd, UINT16 iPara)
{
    switch (iCmd) {
    case FID_AWB_MODE:
        GC0329_set_param_wb(iPara);
        break;
    case FID_COLOR_EFFECT:
        GC0329_set_param_effect(iPara);
        break;
    case FID_AE_EV:
        GC0329_set_param_exposure(iPara);
        break;
    case FID_AE_FLICKER:
        GC0329_set_param_banding(iPara);
		break;
	case FID_SCENE_MODE:
		GC0329NightMode(iPara);
        break;
    default:
        break;
    }
    return TRUE;
} /* GC0329YUVSensorSetting */


UINT32 GC0329FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
        UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
    UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
//    UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
    UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
    UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
//    UINT32 GC0329SensorRegNumber;
//    UINT32 i;
    MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
    MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;

    RETAILMSG(1, (_T("gaiyang GC0329FeatureControl FeatureId=%d\r\n"), FeatureId));

    switch (FeatureId)
    {
    case SENSOR_FEATURE_GET_RESOLUTION:
        *pFeatureReturnPara16++=IMAGE_SENSOR_FULL_WIDTH;
        *pFeatureReturnPara16=IMAGE_SENSOR_FULL_HEIGHT;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_GET_PERIOD:
        *pFeatureReturnPara16++=(VGA_PERIOD_PIXEL_NUMS)+GC0329_dummy_pixels;
        *pFeatureReturnPara16=(VGA_PERIOD_LINE_NUMS)+GC0329_dummy_lines;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
        *pFeatureReturnPara32 = GC0329_g_fPV_PCLK;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_SET_ESHUTTER:
        break;
    case SENSOR_FEATURE_SET_NIGHTMODE:
        //GC0329NightMode((BOOL) *pFeatureData16);
        break;
    case SENSOR_FEATURE_SET_GAIN:
    case SENSOR_FEATURE_SET_FLASHLIGHT:
        break;
    case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
        GC0329_isp_master_clock=*pFeatureData32;
        break;
    case SENSOR_FEATURE_SET_REGISTER:
        GC0329_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
        break;
    case SENSOR_FEATURE_GET_REGISTER:
        pSensorRegData->RegData = GC0329_read_cmos_sensor(pSensorRegData->RegAddr);
        break;
    case SENSOR_FEATURE_GET_CONFIG_PARA:
        memcpy(pSensorConfigData, &GC0329SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
        *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
        break;
    case SENSOR_FEATURE_SET_CCT_REGISTER:
    case SENSOR_FEATURE_GET_CCT_REGISTER:
    case SENSOR_FEATURE_SET_ENG_REGISTER:
    case SENSOR_FEATURE_GET_ENG_REGISTER:
    case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
    case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
    case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
    case SENSOR_FEATURE_GET_GROUP_COUNT:
    case SENSOR_FEATURE_GET_GROUP_INFO:
    case SENSOR_FEATURE_GET_ITEM_INFO:
    case SENSOR_FEATURE_SET_ITEM_INFO:
    case SENSOR_FEATURE_GET_ENG_INFO:
        break;
    case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
        // get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
        // if EEPROM does not exist in camera module.
        *pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_SET_YUV_CMD:
        GC0329YUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));
        break;
    case SENSOR_FEATURE_CHECK_SENSOR_ID:
	GC0329GetSensorID(pFeatureData32);
	break;
    case SENSOR_FEATURE_GET_EXIF_INFO:
			SENSORDB("SENSOR_FEATURE_GET_EXIF_INFO\n");
			SENSORDB("EXIF addr = 0x%x\n",*pFeatureData32); 
			Gc0329GetExifInfo(*pFeatureData32);
			break;
    default:
        break;
	}
return ERROR_NONE;
}	/* GC0329FeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncGC0329YUV=
{
	GC0329Open,
	GC0329GetInfo,
	GC0329GetResolution,
	GC0329FeatureControl,
	GC0329Control,
	GC0329Close
};


UINT32 GC0329_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&SensorFuncGC0329YUV;
	return ERROR_NONE;
} /* SensorInit() */
