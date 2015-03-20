/*****************************************************************************

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
#include <asm/system.h>
#include <linux/slab.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "hi542rawmipi_Sensor.h"
#include "hi542rawmipi_Camera_Sensor_para.h"
#include "hi542rawmipi_CameraCustomized.h"

kal_bool  HI542MIPI_MPEG4_encode_mode = KAL_FALSE;
kal_bool  HI542MIPI_Auto_Flicker_mode = KAL_FALSE;

kal_uint16  HI542MIPI_sensor_gain_base=0x0;
/* MAX/MIN Explosure Lines Used By AE Algorithm */
kal_uint16 HI542MIPI_MAX_EXPOSURE_LINES = 0x191739;//HI542MIPI_PV_PERIOD_LINE_NUMS;//980;
kal_uint8  HI542MIPI_MIN_EXPOSURE_LINES = 4;//2;
kal_uint32 HI542MIPI_isp_master_clock;
kal_uint16 HI542MIPI_CURRENT_FRAME_LINES = HI542MIPI_PV_PERIOD_LINE_NUMS;//980;





static MSDK_SCENARIO_ID_ENUM CurrentScenarioId=MSDK_SCENARIO_ID_CAMERA_PREVIEW;

static kal_uint16 HI542MIPI_dummy_pixels=0, HI542MIPI_dummy_lines=0;
kal_uint16 HI542MIPI_PV_dummy_pixels=0,HI542MIPI_PV_dummy_lines=0;

kal_uint8 HI542MIPI_sensor_write_I2C_address = HI542MIPI_WRITE_ID;
kal_uint8 HI542MIPI_sensor_read_I2C_address = HI542MIPI_READ_ID;

#define LOG_TAG "[HI542Raw]"
#define SENSORDB(fmt, arg...) printk( LOG_TAG  fmt, ##arg)
#define RETAILMSG(x,...)
#define TEXT

kal_uint16 HI542MIPI_g_iDummyLines = 28; 

UINT8 HI542MIPIPixelClockDivider=0;
kal_uint32 HI542MIPI_sensor_pclk=84000000;//19500000;
kal_uint32 HI542MIPI_PV_pclk = 8400; 
kal_uint32 HI542MIPI_CAP_pclk = 8400;

kal_uint16 HI542MIPI_pv_exposure_lines=0x100,HI542MIPI_g_iBackupExtraExp = 0,HI542MIPI_extra_exposure_lines = 0;

kal_uint16 HI542MIPI_sensor_id=0;
kal_uint16 old_dummy_line=0;

kal_uint8 Fix_framerate=0;

MSDK_SENSOR_CONFIG_STRUCT HI542MIPISensorConfigData;

kal_uint32 HI542MIPI_FAC_SENSOR_REG;
kal_uint16 HI542MIPI_sensor_flip_value;


/* FIXME: old factors and DIDNOT use now. s*/
SENSOR_REG_STRUCT HI542MIPISensorCCT[]=CAMERA_SENSOR_CCT_DEFAULT_VALUE;
SENSOR_REG_STRUCT HI542MIPISensorReg[ENGINEER_END]=CAMERA_SENSOR_REG_DEFAULT_VALUE;
/* FIXME: old factors and DIDNOT use now. e*/

typedef enum
{
  HI542MIPI_720P,       //1M 1280x960
  HI542MIPI_5M,     //5M 2592x1944
} HI542MIPI_RES_TYPE;
HI542MIPI_RES_TYPE HI542MIPI_g_RES = HI542MIPI_720P;

typedef enum
{
  HI542MIPI_Video_Auto,      
  HI542MIPI_Video_Night,    
} HI542MIPI_VIDEO_MODE;
HI542MIPI_VIDEO_MODE HI542MIPI_V_mode = HI542MIPI_Video_Auto;


typedef enum
{
  HI542MIPI_MODE_PREVIEW,  //1M  	1280x960
  HI542MIPI_MODE_CAPTURE   //5M    2592x1944
} HI542MIPI_MODE;
HI542MIPI_MODE g_iHI542MIPI_Mode=HI542MIPI_MODE_PREVIEW;


extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);
extern int iBurstWriteReg(u8 *pData, u32 bytes, u16 i2cId); 
#define HI542MIPI_write_cmos_sensor(addr, para) iWriteReg((u16) addr , (u32) para , 1, HI542MIPI_WRITE_ID)
#define HI542MIPI_burst_write_cmos_sensor(pData, bytes)  iBurstWriteReg(pData, bytes, HI542MIPI_WRITE_ID)

static DEFINE_SPINLOCK(hi542mipi_drv_lock);

UINT32 HI542MIPISetMaxFrameRate(UINT16 u2FrameRate);


/*******************************************************************************
* 
********************************************************************************/
kal_uint16 HI542MIPI_read_cmos_sensor(kal_uint32 addr)
{
kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,HI542MIPI_WRITE_ID);
    return get_byte;
}

/*******************************************************************************
* 
********************************************************************************/
void HI542MIPI_write_shutter(kal_uint16 shutter)
{
    kal_uint32 iExp = shutter;
	kal_uint16 CurrentFps;
 
  	kal_uint16 PixelsOneline = HI542MIPI_FULL_PERIOD_PIXEL_NUMS + HI542MIPI_dummy_pixels + 130 +HI542MIPI_PV_PERIOD_EXTRA_PIXEL_NUMS;
	kal_uint16 LinesOneframe = HI542MIPI_PV_PERIOD_LINE_NUMS;
	unsigned long flags;
	#if 0
	 kal_uint16 FixedEnableCheck_11;
	 kal_uint16 FixedEnableCheck_13;
	 kal_uint32 FixedFpsSet;
	#endif
	

	if(HI542MIPI_Auto_Flicker_mode)
	{
	//preview size:   PixelsOneline��capture��һ����,  LinesOneframe�����õ�preview��size��
	  if(CurrentScenarioId==MSDK_SCENARIO_ID_CAMERA_ZSD)
	  {
	  
	  PixelsOneline = HI542MIPI_FULL_PERIOD_PIXEL_NUMS + HI542MIPI_dummy_pixels + 130 +HI542MIPI_FULL_PERIOD_EXTRA_PIXEL_NUMS;
	  //2790
	  LinesOneframe = HI542MIPI_FULL_PERIOD_LINE_NUMS + HI542MIPI_dummy_lines + HI542MIPI_FULL_PERIOD_EXTRA_LINE_NUMS;
	  }
	  else
	  {
		PixelsOneline = HI542MIPI_FULL_PERIOD_PIXEL_NUMS + HI542MIPI_dummy_pixels + 130 +HI542MIPI_PV_PERIOD_EXTRA_PIXEL_NUMS;
		//2790
		LinesOneframe = HI542MIPI_PV_PERIOD_LINE_NUMS + HI542MIPI_dummy_lines + HI542MIPI_PV_PERIOD_EXTRA_LINE_NUMS;
		//980
	  }
		if(shutter < LinesOneframe)
			CurrentFps = (10 * 84000000) /LinesOneframe / PixelsOneline;
		else
			CurrentFps = (10 * 84000000) / shutter / PixelsOneline;
		printk("[HI542raw]:currentFps :%d\n", CurrentFps);

		//Change frame 29.5fps ~ 29.8fps to do auto flick
		if(300 == CurrentFps)
		HI542MIPISetMaxFrameRate(296);
		else if(150 == CurrentFps)
		HI542MIPISetMaxFrameRate(146);

	}

    if(HI542MIPI_720P == HI542MIPI_g_RES)
    {
        PixelsOneline = (HI542MIPI_FULL_PERIOD_PIXEL_NUMS + HI542MIPI_dummy_pixels + 130 + HI542MIPI_PV_PERIOD_EXTRA_PIXEL_NUMS);     //2790
    }
    else if (HI542MIPI_5M == HI542MIPI_g_RES)
    {
        PixelsOneline = HI542MIPI_FULL_PERIOD_PIXEL_NUMS + HI542MIPI_dummy_pixels + 130 + HI542MIPI_FULL_PERIOD_EXTRA_PIXEL_NUMS;      //2790
    }
    
    iExp = shutter*PixelsOneline;

    /*  shutter calc
            EXPTIME[28:0]={EXPTIMEH[4:0],EXPTIMEM1[7:0],EXPTIMEM2[7:0],EXPTIMEL[7:0]}
            Exposure time = EXPTIME/OPCLK       in pixels
      */
     #if 0
	 FixedFpsSet=iExp+2*PixelsOneline;
	 FixedEnableCheck_11=HI542MIPI_read_cmos_sensor(0x0011);
	 FixedEnableCheck_13=HI542MIPI_read_cmos_sensor(0x0013);
	 HI542MIPI_write_cmos_sensor(0x0011,0x04 | FixedEnableCheck_11);
	 HI542MIPI_write_cmos_sensor(0x0013,0x40 | FixedEnableCheck_13);
	 /*
	 if(iExp<0x2a4c20)
	 	{
	 	HI542MIPI_write_cmos_sensor(0x0120, 0x00);
    	HI542MIPI_write_cmos_sensor(0x0121, 0x2a);
    	HI542MIPI_write_cmos_sensor(0x0122, 0x4c);
    	HI542MIPI_write_cmos_sensor(0x0123, 0x20);
	 	}
	 else
	 */
	 	
	 	HI542MIPI_write_cmos_sensor(0x0120, (FixedFpsSet >> 24) & 0xFF);
    	HI542MIPI_write_cmos_sensor(0x0121, (FixedFpsSet >> 16) & 0xFF);
    	HI542MIPI_write_cmos_sensor(0x0122, (FixedFpsSet >> 8 ) & 0xFF);
    	HI542MIPI_write_cmos_sensor(0x0123, (FixedFpsSet) & 0xFF);
	 

	

	    HI542MIPI_write_cmos_sensor(0x011c, (iExp >> 24) & 0xFF);
    	HI542MIPI_write_cmos_sensor(0x011d, (iExp >> 16) & 0xFF);
    	HI542MIPI_write_cmos_sensor(0x011e, (iExp >> 8 ) & 0xFF);
    	HI542MIPI_write_cmos_sensor(0x011f, (iExp) & 0xFF);
 #endif	
 if(Fix_framerate==0)
 	{
		 HI542MIPI_write_cmos_sensor(0x0120, ((iExp+2*PixelsOneline) >> 24) & 0xFF);
		 HI542MIPI_write_cmos_sensor(0x0121, ((iExp+2*PixelsOneline) >> 16) & 0xFF);
		 HI542MIPI_write_cmos_sensor(0x0122, ((iExp+2*PixelsOneline) >> 8 ) & 0xFF);
		 HI542MIPI_write_cmos_sensor(0x0123, ((iExp+2*PixelsOneline)) & 0xFF);
	  
 		HI542MIPI_write_cmos_sensor(0x011C, (iExp >> 24) & 0xFF);
		HI542MIPI_write_cmos_sensor(0x011D, (iExp >> 16) & 0xFF);
		HI542MIPI_write_cmos_sensor(0x011E, (iExp >> 8 ) & 0xFF);
		HI542MIPI_write_cmos_sensor(0x011F, (iExp) & 0xFF);
  	}
 
		HI542MIPI_write_cmos_sensor(0x0115, (iExp >> 24) & 0xFF);
    	HI542MIPI_write_cmos_sensor(0x0116, (iExp >> 16) & 0xFF);
    	HI542MIPI_write_cmos_sensor(0x0117, (iExp >> 8 ) & 0xFF);
    	HI542MIPI_write_cmos_sensor(0x0118, (iExp) & 0xFF);

		

/*
	printk("[HI542raw]:HI542Open Read Sensor REG 0x0115 :0x%x\n", HI542MIPI_read_cmos_sensor(0x0115));
	printk("[HI542raw]:HI542Open Read Sensor REG 0x0116 :0x%x\n", HI542MIPI_read_cmos_sensor(0x0116));
	printk("[HI542raw]:HI542Open Read Sensor REG 0x0117 :0x%x\n", HI542MIPI_read_cmos_sensor(0x0117));
	printk("[HI542raw]:HI542Open Read Sensor REG 0x0118 :0x%x\n", HI542MIPI_read_cmos_sensor(0x0118));
    
	printk("[HI542raw]:HI542Open Read Sensor REG 0x0129 :0x%x\n", HI542MIPI_read_cmos_sensor(0x0129));

	printk("[HI542raw]:HI542Open Read Sensor REG 0x0040 :0x%x\n", HI542MIPI_read_cmos_sensor(0x0040));
	printk("[HI542raw]:HI542Open Read Sensor REG 0x0041 :0x%x\n", HI542MIPI_read_cmos_sensor(0x0041));
	printk("[HI542raw]:HI542Open Read Sensor REG 0x0042 :0x%x\n", HI542MIPI_read_cmos_sensor(0x0042));
	printk("[HI542raw]:HI542Open Read Sensor REG 0x0043 :0x%x\n", HI542MIPI_read_cmos_sensor(0x0043));
*/
 
	spin_lock_irqsave(&hi542mipi_drv_lock,flags);
    HI542MIPI_g_iBackupExtraExp = shutter;    
	spin_unlock_irqrestore(&hi542mipi_drv_lock,flags);

}   /* write_HI542_shutter */

/*******************************************************************************
* 
********************************************************************************/
static kal_uint16 HI542MIPIReg2Gain(const kal_uint8 iReg)
{
    kal_uint16 iGain = 64;    // 1x-gain base

    //! For HI542 sensor, AG is common gain for R,G and B channel 
    //!AG = 256/(B[7:0] + 32)  of 0x0129 

	iGain = iGain*256/(iReg + 32);

    return iGain;
}

static kal_uint16 HI542MIPIGain2Reg(const kal_uint8 iGain)
{
    kal_uint8 iReg;
	kal_uint8 iBaseGain = 64;

	printk("[HI542raw]:HI542MIPIGain2Reg iGain :0x%x\n", iGain); 

    //! For HI542 sensor, AG is common gain for R,G and B channel 
    //!AG = 256/(B[7:0] + 32)  of 0x0129 

	iReg = 256*iBaseGain/iGain - 32;

    return iReg;
}


/*************************************************************************
* FUNCTION
*    HI542_SetGain
*
* DESCRIPTION
*    This function is to set global gain to sensor.
*
* PARAMETERS
*    gain : sensor global gain(base: 0x40)
*
* RETURNS
*    the actually gain set to sensor.
*
* GLOBALS AFFECTED
*
*************************************************************************/
 void HI542MIPI_SetGain(UINT16 iGain)
{
    kal_uint8 iReg;
	kal_uint8 iBaseGain = 64;
 
	mdelay(5);
	printk("[HI542raw FUNCTION TEST]:HI542MIPI_SetGain iGain :0x%x\n", iGain); 

    //! For HI542 sensor, AG is common gain for R,G and B channel 
    //!AG = 256/(B[7:0] + 32)  of 0x0129 

	iReg = 256*iBaseGain/iGain - 32;
 
		  HI542MIPI_write_cmos_sensor(0x0129, iReg);  
 
}   /*  HI542_SetGain  */

/*************************************************************************
* FUNCTION
*    read_HI542_gain
*
* DESCRIPTION
*    This function is to set global gain to sensor.
*
* PARAMETERS
*    None
*
* RETURNS
*    gain : sensor global gain(base: 0x40)
*
* GLOBALS AFFECTED
*
*************************************************************************/
kal_uint16 read_HI542MIPI_gain(void)
{
    return (kal_uint16)HI542MIPI_read_cmos_sensor(0x0129);
}  /* read_HI542_gain */

/*******************************************************************************
* 
********************************************************************************/
void write_HI542MIPI_gain(kal_uint16 gain)
{
    HI542MIPI_SetGain(gain);
}

/*******************************************************************************
* 
********************************************************************************/
void HI542MIPI_camera_para_to_sensor(void)
{
    kal_uint32    i;
    for(i=0; 0xFFFFFFFF!=HI542MIPISensorReg[i].Addr; i++)
    {
        HI542MIPI_write_cmos_sensor(HI542MIPISensorReg[i].Addr, HI542MIPISensorReg[i].Para);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=HI542MIPISensorReg[i].Addr; i++)
    {
        HI542MIPI_write_cmos_sensor(HI542MIPISensorReg[i].Addr, HI542MIPISensorReg[i].Para);
    }
    for(i=FACTORY_START_ADDR; i<FACTORY_END_ADDR; i++)
    {
        HI542MIPI_write_cmos_sensor(HI542MIPISensorCCT[i].Addr, HI542MIPISensorCCT[i].Para);
    }
}


/*************************************************************************
* FUNCTION
*    HI542_sensor_to_camera_para
*
* DESCRIPTION
*    // update camera_para from sensor register
*
* PARAMETERS
*    None
*
* RETURNS
*    gain : sensor global gain(base: 0x40)
*
* GLOBALS AFFECTED
*
*************************************************************************/
void HI542MIPI_sensor_to_camera_para(void)
{
    kal_uint32    i, temp_data;
    for(i=0; 0xFFFFFFFF!=HI542MIPISensorReg[i].Addr; i++)
    {
        temp_data = HI542MIPI_read_cmos_sensor(HI542MIPISensorReg[i].Addr);
		spin_lock(&hi542mipi_drv_lock);
        HI542MIPISensorReg[i].Para = temp_data;
		spin_unlock(&hi542mipi_drv_lock);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=HI542MIPISensorReg[i].Addr; i++)
    {
        temp_data  = HI542MIPI_read_cmos_sensor(HI542MIPISensorReg[i].Addr);
		spin_lock(&hi542mipi_drv_lock);
        HI542MIPISensorReg[i].Para = temp_data;
		spin_unlock(&hi542mipi_drv_lock);
    }
}


/*************************************************************************
* FUNCTION
*    HI542_get_sensor_group_count
*
* DESCRIPTION
*    //
*
* PARAMETERS
*    None
*
* RETURNS
*    gain : sensor global gain(base: 0x40)
*
* GLOBALS AFFECTED
*
*************************************************************************/
kal_int32  HI542MIPI_get_sensor_group_count(void)
{
    return GROUP_TOTAL_NUMS;
}

void HI542MIPI_get_sensor_group_info(kal_uint16 group_idx, kal_int8* group_name_ptr, kal_int32* item_count_ptr)
{
   switch (group_idx)
   {
        case PRE_GAIN:
            sprintf((char *)group_name_ptr, "CCT");
            *item_count_ptr = 2;
            break;
        case CMMCLK_CURRENT:
            sprintf((char *)group_name_ptr, "CMMCLK Current");
            *item_count_ptr = 1;
            break;
        case FRAME_RATE_LIMITATION:
            sprintf((char *)group_name_ptr, "Frame Rate Limitation");
            *item_count_ptr = 2;
            break;
        case REGISTER_EDITOR:
            sprintf((char *)group_name_ptr, "Register Editor");
            *item_count_ptr = 2;
            break;
        default:
            ASSERT(0);
}
}

void HI542MIPI_get_sensor_item_info(kal_uint16 group_idx,kal_uint16 item_idx, MSDK_SENSOR_ITEM_INFO_STRUCT* info_ptr)
{
    kal_int16 temp_reg=0;
    kal_uint16 temp_gain=0, temp_addr=0, temp_para=0;

	printk("[HI542raw FUNCTION TEST]:HI542MIPI_get_sensor_item_info group_idx: Ox%x\n .........................",group_idx);
	
    switch (group_idx)
    {
        case PRE_GAIN:
            switch (item_idx)
            {
                case 0:
                    sprintf((char *)info_ptr->ItemNamePtr,"Pregain-Global");
                    temp_addr = PRE_GAIN_INDEX;
                    break;
                case 1:
                    sprintf((char *)info_ptr->ItemNamePtr,"GLOBAL_GAIN");
                    temp_addr = GLOBAL_GAIN_INDEX;
                    break;
                default:
                    ASSERT(0);
            }

            temp_para=HI542MIPISensorCCT[temp_addr].Para;

            temp_gain = HI542MIPIReg2Gain(temp_para);

            temp_gain=(temp_gain*1000)/BASEGAIN;

            info_ptr->ItemValue=temp_gain;
            info_ptr->IsTrueFalse=KAL_FALSE;
            info_ptr->IsReadOnly=KAL_FALSE;
            info_ptr->IsNeedRestart=KAL_FALSE;
            info_ptr->Min=1000;
            info_ptr->Max=15875;
            break;
        case CMMCLK_CURRENT:
            switch (item_idx)
            {
                case 0:
                    sprintf((char *)info_ptr->ItemNamePtr,"Drv Cur[2,4,6,8]mA");
                
                    //temp_reg=HI542SensorReg[CMMCLK_CURRENT_INDEX].Para;
                    temp_reg = ISP_DRIVING_2MA;
                    if(temp_reg==ISP_DRIVING_2MA)
                    {
                        info_ptr->ItemValue=2;
                    }
                    else if(temp_reg==ISP_DRIVING_4MA)
                    {
                        info_ptr->ItemValue=4;
                    }
                    else if(temp_reg==ISP_DRIVING_6MA)
                    {
                        info_ptr->ItemValue=6;
                    }
                    else if(temp_reg==ISP_DRIVING_8MA)
                    {
                        info_ptr->ItemValue=8;
                    }
                
                    info_ptr->IsTrueFalse=KAL_FALSE;
                    info_ptr->IsReadOnly=KAL_FALSE;
                    info_ptr->IsNeedRestart=KAL_TRUE;
                    info_ptr->Min=2;
                    info_ptr->Max=8;
                    break;
                default:
                    ASSERT(0);
            }
            break;
        case FRAME_RATE_LIMITATION:
            switch (item_idx)
            {
                case 0:
                    sprintf((char *)info_ptr->ItemNamePtr,"Max Exposure Lines");
                    info_ptr->ItemValue=HI542MIPI_MAX_EXPOSURE_LINES;
                    info_ptr->IsTrueFalse=KAL_FALSE;
                    info_ptr->IsReadOnly=KAL_TRUE;
                    info_ptr->IsNeedRestart=KAL_FALSE;
                    info_ptr->Min=0;
                    info_ptr->Max=0;
                    break;
                case 1:
                    sprintf((char *)info_ptr->ItemNamePtr,"Min Frame Rate");
                    info_ptr->ItemValue=12;
                    info_ptr->IsTrueFalse=KAL_FALSE;
                    info_ptr->IsReadOnly=KAL_TRUE;
                    info_ptr->IsNeedRestart=KAL_FALSE;
                    info_ptr->Min=0;
                    info_ptr->Max=0;
                    break;
                default:
                    ASSERT(0);
            }
            break;
        case REGISTER_EDITOR:
            switch (item_idx)
            {
                case 0:
                    sprintf((char *)info_ptr->ItemNamePtr,"REG Addr.");
                    info_ptr->ItemValue=0;
                    info_ptr->IsTrueFalse=KAL_FALSE;
                    info_ptr->IsReadOnly=KAL_FALSE;
                    info_ptr->IsNeedRestart=KAL_FALSE;
                    info_ptr->Min=0;
                    info_ptr->Max=0xFFFF;
                    break;
                case 1:
                    sprintf((char *)info_ptr->ItemNamePtr,"REG Value");
                    info_ptr->ItemValue=0;
                    info_ptr->IsTrueFalse=KAL_FALSE;
                    info_ptr->IsReadOnly=KAL_FALSE;
                    info_ptr->IsNeedRestart=KAL_FALSE;
                    info_ptr->Min=0;
                    info_ptr->Max=0xFFFF;
                    break;
                default:
                ASSERT(0);
            }
            break;
        default:
            ASSERT(0);
    }
}

//void HI542_set_isp_driving_current(kal_uint8 current)
//{
//}

kal_bool HI542MIPI_set_sensor_item_info(kal_uint16 group_idx, kal_uint16 item_idx, kal_int32 ItemValue)
{
//   kal_int16 temp_reg;
   kal_uint16 temp_addr=0, temp_para=0;
   
   printk("[HI542raw FUNCTION TEST]:HI542MIPI_set_sensor_item_info group_idx: Ox%x\n .........................",group_idx);

   switch (group_idx)
    {
        case PRE_GAIN:
            switch (item_idx)
            {
                case 0:
                    temp_addr = PRE_GAIN_INDEX;
                    break;
                case 1:
                    temp_addr = GLOBAL_GAIN_INDEX;
                    break;
                default:
                    ASSERT(0);
            }

            temp_para = HI542MIPIGain2Reg(ItemValue);

			spin_lock(&hi542mipi_drv_lock);
            HI542MIPISensorCCT[temp_addr].Para = temp_para;
			spin_unlock(&hi542mipi_drv_lock);
			
            HI542MIPI_write_cmos_sensor(HI542MIPISensorCCT[temp_addr].Addr,temp_para);

			temp_para = read_HI542MIPI_gain();
			
			spin_lock(&hi542mipi_drv_lock);
            HI542MIPI_sensor_gain_base=temp_para;
			spin_unlock(&hi542mipi_drv_lock);

            break;
        case CMMCLK_CURRENT:
            switch (item_idx)
            {
                case 0:
					spin_lock(&hi542mipi_drv_lock);
                    if(ItemValue==2)
                    {
                        HI542MIPISensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_2MA;
                        //HI542_set_isp_driving_current(ISP_DRIVING_2MA);
                    }
                    else if(ItemValue==3 || ItemValue==4)
                    {
                        HI542MIPISensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_4MA;
                        //HI542_set_isp_driving_current(ISP_DRIVING_4MA);
                    }
                    else if(ItemValue==5 || ItemValue==6)
                    {
                        HI542MIPISensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_6MA;
                        //HI542_set_isp_driving_current(ISP_DRIVING_6MA);
                    }
                    else
                    {
                        HI542MIPISensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_8MA;
                        //HI542_set_isp_driving_current(ISP_DRIVING_8MA);
                    }
					spin_unlock(&hi542mipi_drv_lock);
                    break;
                default:
                    ASSERT(0);
            }
            break;
        case FRAME_RATE_LIMITATION:
            ASSERT(0);
            break;
        case REGISTER_EDITOR:
            switch (item_idx)
            {
                case 0:
					spin_lock(&hi542mipi_drv_lock);
                    HI542MIPI_FAC_SENSOR_REG=ItemValue;
					spin_unlock(&hi542mipi_drv_lock);
                    break;
                case 1:
                    HI542MIPI_write_cmos_sensor(HI542MIPI_FAC_SENSOR_REG,ItemValue);
                    break;
                default:
                    ASSERT(0);
            }
            break;
        default:
            ASSERT(0);
    }
    return KAL_TRUE;
}

static void HI542MIPI_SetDummy(const kal_uint16 iPixels, const kal_uint16 iLines)
{
    kal_uint16 ExtraPixelsOneline = HI542MIPI_FULL_PERIOD_PIXEL_NUMS;
	kal_uint16 ExtraLinesOneframe = HI542MIPI_FULL_PERIOD_LINE_NUMS;
    if(HI542MIPI_720P == HI542MIPI_g_RES)
    {
        ExtraPixelsOneline = iPixels + HI542MIPI_PV_PERIOD_EXTRA_PIXEL_NUMS;
		ExtraLinesOneframe = iLines + HI542MIPI_PV_PERIOD_EXTRA_LINE_NUMS;
		
        if(HI542MIPI_MPEG4_encode_mode == KAL_FALSE)//for Fix video framerate
        {
            spin_lock(&hi542mipi_drv_lock);
            HI542MIPI_CURRENT_FRAME_LINES = iLines + HI542MIPI_PV_PERIOD_LINE_NUMS + HI542MIPI_PV_PERIOD_EXTRA_LINE_NUMS ;
			spin_unlock(&hi542mipi_drv_lock);
        }
    }
    else if (HI542MIPI_5M == HI542MIPI_g_RES)
    {
        ExtraPixelsOneline = iPixels + HI542MIPI_FULL_PERIOD_EXTRA_PIXEL_NUMS;
		ExtraLinesOneframe = iLines + HI542MIPI_FULL_PERIOD_EXTRA_LINE_NUMS;
		spin_lock(&hi542mipi_drv_lock);
        HI542MIPI_CURRENT_FRAME_LINES = iLines + HI542MIPI_FULL_PERIOD_LINE_NUMS + HI542MIPI_FULL_PERIOD_EXTRA_LINE_NUMS;
		spin_unlock(&hi542mipi_drv_lock);
    }
    HI542MIPI_write_cmos_sensor(0x0040, (ExtraPixelsOneline >> 8) & 0xFF);
    HI542MIPI_write_cmos_sensor(0x0041, ExtraPixelsOneline & 0xFF);

	HI542MIPI_write_cmos_sensor(0x0042, (ExtraLinesOneframe >> 8) & 0xFF);
    HI542MIPI_write_cmos_sensor(0x0043, ExtraLinesOneframe & 0xFF);

    printk("[HI542raw FUNCTION TEST]:HI542MIPI_SetDummy PixelsOneline :0x%x,LinesOneframe :0x%x\n", ExtraPixelsOneline,ExtraLinesOneframe);

}   /*  HI542_SetDummy */


/*Avoid Folat, frame rate =10 * u2FrameRate */
UINT32 HI542MIPISetMaxFrameRate(UINT16 u2FrameRate)
{
	kal_int16 dummy_line;
	kal_uint16 LinesOneframe;
	kal_uint16 PixelsOneline;
	kal_uint16 CurrentFps;
	kal_uint16 SetFps = u2FrameRate;
	if(CurrentScenarioId==MSDK_SCENARIO_ID_CAMERA_ZSD)
	{
		PixelsOneline = HI542MIPI_FULL_PERIOD_PIXEL_NUMS + HI542MIPI_dummy_pixels + 130 + HI542MIPI_FULL_PERIOD_EXTRA_PIXEL_NUMS;
		LinesOneframe = HI542MIPI_FULL_PERIOD_LINE_NUMS + HI542MIPI_dummy_lines + HI542MIPI_FULL_PERIOD_EXTRA_LINE_NUMS;
	
	}
	else
	{
		PixelsOneline = HI542MIPI_FULL_PERIOD_PIXEL_NUMS + HI542MIPI_dummy_pixels + 130 + HI542MIPI_FULL_PERIOD_EXTRA_PIXEL_NUMS;
		LinesOneframe = HI542MIPI_PV_PERIOD_LINE_NUMS + HI542MIPI_dummy_lines + HI542MIPI_PV_PERIOD_EXTRA_LINE_NUMS;
	}
	printk("[HI542SetMaxFrameRate]u2FrameRate=%d,PixelsOneline=%d,LinesOneframe=%d",u2FrameRate,PixelsOneline,LinesOneframe);

	if(HI542MIPI_MPEG4_encode_mode == KAL_TRUE)
	{
		CurrentFps = (10 * 84000000)/LinesOneframe/PixelsOneline;
		if(300 == CurrentFps)
			SetFps = 296;
		else if(150 == CurrentFps)
			SetFps = 146;
	}
	
	LinesOneframe = (10 * 84000000) / SetFps / PixelsOneline;
	if(CurrentScenarioId==MSDK_SCENARIO_ID_CAMERA_ZSD)
	{
		dummy_line = LinesOneframe - (HI542MIPI_FULL_PERIOD_LINE_NUMS + HI542MIPI_FULL_PERIOD_EXTRA_LINE_NUMS);
	}
	else
	{
		dummy_line = LinesOneframe - (HI542MIPI_PV_PERIOD_LINE_NUMS + HI542MIPI_PV_PERIOD_EXTRA_LINE_NUMS);
	    /* to fix VSYNC, to fix frame rate */
	}
	if(dummy_line<0)
		dummy_line=0;
	if(old_dummy_line!=dummy_line)
		{
		  HI542MIPI_SetDummy(0, dummy_line); /* modify dummy_pixel must gen AE table again */	
		  old_dummy_line=dummy_line;
		}
}

/*******************************************************************************
*
********************************************************************************/
static void HI542MIPI_Sensor_Init(void)
{
	//[SENSOR_INITIALIZATION]
	//DISP_DATE = "2010-02-09 13:27:00"
	//DISP_WIDTH = 1288
	//DISP_HEIGHT = 968
	//DISP_FORMAT = BAYER10
	//DISP_DATABUS = 16
	//DISP_DATAORDER = BG
	//MCLK = 24.00
	//PLL = 2.00
	
	//BEGIN
	//I2C_ID = 0x14
	//I2C_BYTE	= 0x21
	//
	////// Mode Controls
#if 0
	HI542MIPI_write_cmos_sensor(0x0014,0x42);	// B[7] Justification control (Data Justification on output Data)
	HI542MIPI_write_cmos_sensor(0x0015,0x00);	// B[7] Tris,PCLK Inverting
	HI542MIPI_write_cmos_sensor(0x0036,0x00);	// MIPI Spec --> 0.9
	HI542MIPI_write_cmos_sensor(0x0017,0x2B);	// YUV422:0x18 RAW10:0x2B RAW8:0x2A RAW12:0x2C
	HI542MIPI_write_cmos_sensor(0x0019,0x04);	// Select Data_ID by REGISTER(0x0017)
	HI542MIPI_write_cmos_sensor(0x0002,0x15);	// B[7:2] ui_x4_clk_lane (Unit interval time step 0.25ns) 3ns  0.25 X 12
	HI542MIPI_write_cmos_sensor(0x0003,0x00);	// B[4] hs_invert_clk_lane (Invert HS signals)
	HI542MIPI_write_cmos_sensor(0x0004,0x02);	// B[4] hs_rx_term_e_subLVDS_clk_lane (Enables the hs-termination for subLVDs mode)
	HI542MIPI_write_cmos_sensor(0x0005,0x03);	// B[5] force_rx_mod_data_lane (Force lane module into receive mode wait for stop state)
	HI542MIPI_write_cmos_sensor(0x0006,0x01);	// B[5] Cd_off Contention_detector on/off
	HI542MIPI_write_cmos_sensor(0x0009,0x01);	// B[5] force_rx_mod_data_lane (Force lane module into receive mode wait for stop state)
	HI542MIPI_write_cmos_sensor(0x000A,0x01);	// B[5] Cd_off Contention_detector on/off
#endif
	//I2C_ID = 0x40
	//I2C_BYTE	= 0x21
	///////////////////////////////////////////////////
	///////////////////////////////////////////////////

	
	HI542MIPI_write_cmos_sensor(0x0001,0x02);//  sw reset			 
	HI542MIPI_write_cmos_sensor(0x0001,0x01);  //sleep
	
	HI542MIPI_write_cmos_sensor(0x03d4,0x18);     //  internal   dvdd   
	mDELAY(10);
	///////////////////////////////////////////////////
	///////////////////////////////////////////////////
	
	//HI542MIPI_write_cmos_sensor(0x0010,0x00);  
	HI542MIPI_write_cmos_sensor(0x0011,0x93);  
	//HI542MIPI_write_cmos_sensor(0x0012,0x08);  
	//HI542MIPI_write_cmos_sensor(0x0013,0x00);  
	HI542MIPI_write_cmos_sensor(0x0020,0x00);  
	HI542MIPI_write_cmos_sensor(0x0021,0x00);  
	HI542MIPI_write_cmos_sensor(0x0022,0x00);  
	HI542MIPI_write_cmos_sensor(0x0023,0x00);  
	
	//HI542MIPI_write_cmos_sensor(0x0024,0x07);  
	//HI542MIPI_write_cmos_sensor(0x0025,0xA8);  
	//HI542MIPI_write_cmos_sensor(0x0026,0x0A);  
	//HI542MIPI_write_cmos_sensor(0x0027,0xB0);  
	
	HI542MIPI_write_cmos_sensor(0x0038,0x02);  
	HI542MIPI_write_cmos_sensor(0x0039,0x2C); 
	
	//HI542MIPI_write_cmos_sensor(0x003A,0x02);  
	//HI542MIPI_write_cmos_sensor(0x003B,0x2C);  
	
	HI542MIPI_write_cmos_sensor(0x003C,0x00);  
	HI542MIPI_write_cmos_sensor(0x003D,0x0C);  
	HI542MIPI_write_cmos_sensor(0x003E,0x00);  
	HI542MIPI_write_cmos_sensor(0x003F,0x0C);  
	HI542MIPI_write_cmos_sensor(0x0040,0x00);  //Hblank H
	HI542MIPI_write_cmos_sensor(0x0041,0x35);  //2E} Hblank L
//changed by zhiqiang original 35

	HI542MIPI_write_cmos_sensor(0x0042,0x00);  
	HI542MIPI_write_cmos_sensor(0x0043,0x14);  
	HI542MIPI_write_cmos_sensor(0x0045,0x07);  
	HI542MIPI_write_cmos_sensor(0x0046,0x01);  
	HI542MIPI_write_cmos_sensor(0x0047,0xD0);  
	
	//HI542MIPI_write_cmos_sensor(0x004A,0x02);   
	//HI542MIPI_write_cmos_sensor(0x004B,0xD8);    
	//HI542MIPI_write_cmos_sensor(0x004C,0x05);  
	//HI542MIPI_write_cmos_sensor(0x004D,0x08);  

	HI542MIPI_write_cmos_sensor(0x0050,0x00);  
	HI542MIPI_write_cmos_sensor(0x0052,0x10);  
	HI542MIPI_write_cmos_sensor(0x0053,0x10);  
	HI542MIPI_write_cmos_sensor(0x0054,0x10);  
	HI542MIPI_write_cmos_sensor(0x0055,0x08);  
	HI542MIPI_write_cmos_sensor(0x0056,0x80);  
	HI542MIPI_write_cmos_sensor(0x0057,0x08);  
	HI542MIPI_write_cmos_sensor(0x0058,0x08);  
	HI542MIPI_write_cmos_sensor(0x0059,0x08);  
	HI542MIPI_write_cmos_sensor(0x005A,0x08);  
	HI542MIPI_write_cmos_sensor(0x005B,0x02);  
	HI542MIPI_write_cmos_sensor(0x0070,0x03);	//EMI OFF
	
	//HI542MIPI_write_cmos_sensor(0x0080,0xC0);  
	HI542MIPI_write_cmos_sensor(0x0081,0x01);  //09},//0B},BLC scheme
	HI542MIPI_write_cmos_sensor(0x0082,0x23);  
	HI542MIPI_write_cmos_sensor(0x0083,0x00);  

	//HI542MIPI_write_cmos_sensor(0x0084,0x30);  
	HI542MIPI_write_cmos_sensor(0x0085,0x00);  
	HI542MIPI_write_cmos_sensor(0x0086,0x00);  
	HI542MIPI_write_cmos_sensor(0x008C,0x02);  
	//HI542MIPI_write_cmos_sensor(0x008D,0xFA);  
	//HI542MIPI_write_cmos_sensor(0x0090,0x0b);  
	HI542MIPI_write_cmos_sensor(0x00A0,0x0f);  //0C},//0B},RAMP DC OFFSET
	HI542MIPI_write_cmos_sensor(0x00A1,0x00);  
	HI542MIPI_write_cmos_sensor(0x00A2,0x00);  
	HI542MIPI_write_cmos_sensor(0x00A3,0x00);  
	HI542MIPI_write_cmos_sensor(0x00A4,0xFF);  
	HI542MIPI_write_cmos_sensor(0x00A5,0x00);  
	HI542MIPI_write_cmos_sensor(0x00A6,0x00);  
	//HI542MIPI_write_cmos_sensor(0x00A7,0xa6);  
	HI542MIPI_write_cmos_sensor(0x00A8,0x7F);  
	HI542MIPI_write_cmos_sensor(0x00A9,0x7F);  
	HI542MIPI_write_cmos_sensor(0x00AA,0x7F);  
	HI542MIPI_write_cmos_sensor(0x00B4,0x00);  //08},BLC offset
	HI542MIPI_write_cmos_sensor(0x00B5,0x00);  //08},
	HI542MIPI_write_cmos_sensor(0x00B6,0x02);  //07},
	HI542MIPI_write_cmos_sensor(0x00B7,0x01);  //07},
	HI542MIPI_write_cmos_sensor(0x00D4,0x00);  
	HI542MIPI_write_cmos_sensor(0x00D5,0xaa);  //a9},RAMP T1
	HI542MIPI_write_cmos_sensor(0x00D6,0x01);  
	HI542MIPI_write_cmos_sensor(0x00D7,0xc9);  
	HI542MIPI_write_cmos_sensor(0x00D8,0x05);  
	HI542MIPI_write_cmos_sensor(0x00D9,0x59);  
	HI542MIPI_write_cmos_sensor(0x00DA,0x00);  
	HI542MIPI_write_cmos_sensor(0x00DB,0xb0);  
	HI542MIPI_write_cmos_sensor(0x00DC,0x01);  
	HI542MIPI_write_cmos_sensor(0x00DD,0xc9);  //c5},
	//HI542MIPI_write_cmos_sensor(0x0119,0x00);  
	//HI542MIPI_write_cmos_sensor(0x011A,0x00);  
	//HI542MIPI_write_cmos_sensor(0x011B,0x00);  
	HI542MIPI_write_cmos_sensor(0x011C,0x1F);  
	HI542MIPI_write_cmos_sensor(0x011D,0xFF);  
	HI542MIPI_write_cmos_sensor(0x011E,0xFF);  
	HI542MIPI_write_cmos_sensor(0x011F,0xFF);  
	
	//HI542MIPI_write_cmos_sensor(0x0115,0x00);  
	//HI542MIPI_write_cmos_sensor(0x0116,0x16);  
	//HI542MIPI_write_cmos_sensor(0x0117,0x27);  
	//HI542MIPI_write_cmos_sensor(0x0118,0xE0);  
	HI542MIPI_write_cmos_sensor(0x012A,0xFF);  
	HI542MIPI_write_cmos_sensor(0x012B,0x00);  
	HI542MIPI_write_cmos_sensor(0x0129,0x40);  
	HI542MIPI_write_cmos_sensor(0x0210,0x00);  
	HI542MIPI_write_cmos_sensor(0x0212,0x00);  
	HI542MIPI_write_cmos_sensor(0x0213,0x00);  
	HI542MIPI_write_cmos_sensor(0x0216,0x00);  
	//HI542MIPI_write_cmos_sensor(0x0217,0x40);  
	//HI542MIPI_write_cmos_sensor(0x0218,0x00);  
	HI542MIPI_write_cmos_sensor(0x0219,0x33);  //66},Pixel bias
	//HI542MIPI_write_cmos_sensor(0x021A,0x15);  //15},
	HI542MIPI_write_cmos_sensor(0x021B,0x55);  
	HI542MIPI_write_cmos_sensor(0x021C,0x85);  
	HI542MIPI_write_cmos_sensor(0x021D,0xFF);  
	HI542MIPI_write_cmos_sensor(0x021E,0x01);  
	HI542MIPI_write_cmos_sensor(0x021F,0x00);  
	HI542MIPI_write_cmos_sensor(0x0220,0x02);  
	HI542MIPI_write_cmos_sensor(0x0221,0x00);  
	HI542MIPI_write_cmos_sensor(0x0222,0xA0);  
	
	//HI542MIPI_write_cmos_sensor(0x0223,0x2D);  
	//HI542MIPI_write_cmos_sensor(0x0224,0x24);  
	//HI542MIPI_write_cmos_sensor(0x0225,0x00);  
	//HI542MIPI_write_cmos_sensor(0x0226,0x3F);  
	HI542MIPI_write_cmos_sensor(0x0227,0x0A);  
	HI542MIPI_write_cmos_sensor(0x0228,0x5C);  
	HI542MIPI_write_cmos_sensor(0x0229,0x2d);  //41},//00},//2C},RAMP swing range
	HI542MIPI_write_cmos_sensor(0x022A,0x04);  
	
	//HI542MIPI_write_cmos_sensor(0x022B,0x9f);  
	HI542MIPI_write_cmos_sensor(0x022C,0x01);  
	HI542MIPI_write_cmos_sensor(0x022D,0x23);  
	//HI542MIPI_write_cmos_sensor(0x0232,0x10);  
	HI542MIPI_write_cmos_sensor(0x0237,0x00);  
	HI542MIPI_write_cmos_sensor(0x0238,0x00);  
	HI542MIPI_write_cmos_sensor(0x0239,0xA5);  
	HI542MIPI_write_cmos_sensor(0x023A,0x20);  
	HI542MIPI_write_cmos_sensor(0x023B,0x00);  
	HI542MIPI_write_cmos_sensor(0x023C,0x22);  
	
	//HI542MIPI_write_cmos_sensor(0x023E,0x00);  
	HI542MIPI_write_cmos_sensor(0x023F,0x80);  
	HI542MIPI_write_cmos_sensor(0x0240,0x04);  
	HI542MIPI_write_cmos_sensor(0x0241,0x07);  
	HI542MIPI_write_cmos_sensor(0x0242,0x00);  
	HI542MIPI_write_cmos_sensor(0x0243,0x01);  
	HI542MIPI_write_cmos_sensor(0x0244,0x80);  
	HI542MIPI_write_cmos_sensor(0x0245,0xE0);  
	HI542MIPI_write_cmos_sensor(0x0246,0x00);  
	HI542MIPI_write_cmos_sensor(0x0247,0x00);  
	HI542MIPI_write_cmos_sensor(0x024A,0x00);  
	HI542MIPI_write_cmos_sensor(0x024B,0x14);  
	HI542MIPI_write_cmos_sensor(0x024D,0x00);  
	HI542MIPI_write_cmos_sensor(0x024E,0x03);  
	HI542MIPI_write_cmos_sensor(0x024F,0x00);  
	HI542MIPI_write_cmos_sensor(0x0250,0x53);  
	HI542MIPI_write_cmos_sensor(0x0251,0x00);  
	HI542MIPI_write_cmos_sensor(0x0252,0x07);  
	HI542MIPI_write_cmos_sensor(0x0253,0x00);  
	HI542MIPI_write_cmos_sensor(0x0254,0x4F);  
	HI542MIPI_write_cmos_sensor(0x0255,0x00);  
	HI542MIPI_write_cmos_sensor(0x0256,0x07);  
	HI542MIPI_write_cmos_sensor(0x0257,0x00);  
	HI542MIPI_write_cmos_sensor(0x0258,0x4F);  
	HI542MIPI_write_cmos_sensor(0x0259,0x0C);  
	HI542MIPI_write_cmos_sensor(0x025A,0x0C);  
	HI542MIPI_write_cmos_sensor(0x025B,0x0C);  
	HI542MIPI_write_cmos_sensor(0x026C,0x00);  
	HI542MIPI_write_cmos_sensor(0x026D,0x09);  
	HI542MIPI_write_cmos_sensor(0x026E,0x00);  
	HI542MIPI_write_cmos_sensor(0x026F,0x4B);  
	HI542MIPI_write_cmos_sensor(0x0270,0x00);  
	HI542MIPI_write_cmos_sensor(0x0271,0x09);  
	HI542MIPI_write_cmos_sensor(0x0272,0x00);  
	HI542MIPI_write_cmos_sensor(0x0273,0x4B);  
	HI542MIPI_write_cmos_sensor(0x0274,0x00);  
	HI542MIPI_write_cmos_sensor(0x0275,0x09);  
	HI542MIPI_write_cmos_sensor(0x0276,0x00);  
	HI542MIPI_write_cmos_sensor(0x0277,0x4B);  
	HI542MIPI_write_cmos_sensor(0x0278,0x00);  
	HI542MIPI_write_cmos_sensor(0x0279,0x01);  
	HI542MIPI_write_cmos_sensor(0x027A,0x00);  
	HI542MIPI_write_cmos_sensor(0x027B,0x55);  
	HI542MIPI_write_cmos_sensor(0x027C,0x00);  
	HI542MIPI_write_cmos_sensor(0x027D,0x00);  
	HI542MIPI_write_cmos_sensor(0x027E,0x05);  
	HI542MIPI_write_cmos_sensor(0x027F,0x5E);  
	HI542MIPI_write_cmos_sensor(0x0280,0x00);  
	HI542MIPI_write_cmos_sensor(0x0281,0x03);  
	HI542MIPI_write_cmos_sensor(0x0282,0x00);  
	HI542MIPI_write_cmos_sensor(0x0283,0x45);  
	HI542MIPI_write_cmos_sensor(0x0284,0x00);  
	HI542MIPI_write_cmos_sensor(0x0285,0x03);  
	HI542MIPI_write_cmos_sensor(0x0286,0x00);  
	HI542MIPI_write_cmos_sensor(0x0287,0x45);  
	HI542MIPI_write_cmos_sensor(0x0288,0x05);  
	HI542MIPI_write_cmos_sensor(0x0289,0x5c);  
	HI542MIPI_write_cmos_sensor(0x028A,0x05);  
	HI542MIPI_write_cmos_sensor(0x028B,0x60);  
	HI542MIPI_write_cmos_sensor(0x02A0,0x01);  
	HI542MIPI_write_cmos_sensor(0x02A1,0xe0);  
	HI542MIPI_write_cmos_sensor(0x02A2,0x02);  
	HI542MIPI_write_cmos_sensor(0x02A3,0x22);  
	HI542MIPI_write_cmos_sensor(0x02A4,0x05);  
	HI542MIPI_write_cmos_sensor(0x02A5,0x5C);  
	HI542MIPI_write_cmos_sensor(0x02A6,0x05);  
	HI542MIPI_write_cmos_sensor(0x02A7,0x60);  
	HI542MIPI_write_cmos_sensor(0x02A8,0x05);  
	HI542MIPI_write_cmos_sensor(0x02A9,0x5C);  
	HI542MIPI_write_cmos_sensor(0x02AA,0x05);  
	HI542MIPI_write_cmos_sensor(0x02AB,0x60);  
	HI542MIPI_write_cmos_sensor(0x02D2,0x0F);  
	HI542MIPI_write_cmos_sensor(0x02DB,0x00);  
	HI542MIPI_write_cmos_sensor(0x02DC,0x00);  
	HI542MIPI_write_cmos_sensor(0x02DD,0x00);  
	HI542MIPI_write_cmos_sensor(0x02DE,0x0C);  
	HI542MIPI_write_cmos_sensor(0x02DF,0x00);  
	HI542MIPI_write_cmos_sensor(0x02E0,0x04);  
	HI542MIPI_write_cmos_sensor(0x02E1,0x00);  
	HI542MIPI_write_cmos_sensor(0x02E2,0x00);  
	HI542MIPI_write_cmos_sensor(0x02E3,0x00);  
	HI542MIPI_write_cmos_sensor(0x02E4,0x0F);  
	HI542MIPI_write_cmos_sensor(0x02F0,0x05);  
	HI542MIPI_write_cmos_sensor(0x02F1,0x05);  
	HI542MIPI_write_cmos_sensor(0x0310,0x00);  
	HI542MIPI_write_cmos_sensor(0x0311,0x01);  
	HI542MIPI_write_cmos_sensor(0x0312,0x05);  
	HI542MIPI_write_cmos_sensor(0x0313,0x5A);  
	HI542MIPI_write_cmos_sensor(0x0314,0x00);  
	HI542MIPI_write_cmos_sensor(0x0315,0x01);  
	HI542MIPI_write_cmos_sensor(0x0316,0x05);  
	HI542MIPI_write_cmos_sensor(0x0317,0x5A);  
	HI542MIPI_write_cmos_sensor(0x0318,0x00);  
	HI542MIPI_write_cmos_sensor(0x0319,0x05);  
	HI542MIPI_write_cmos_sensor(0x031A,0x00);  
	HI542MIPI_write_cmos_sensor(0x031B,0x2F);  
	HI542MIPI_write_cmos_sensor(0x031C,0x00);  
	HI542MIPI_write_cmos_sensor(0x031D,0x05);  
	HI542MIPI_write_cmos_sensor(0x031E,0x00);  
	HI542MIPI_write_cmos_sensor(0x031F,0x2F);  
	HI542MIPI_write_cmos_sensor(0x0320,0x00);  
	HI542MIPI_write_cmos_sensor(0x0321,0xAB);  
	HI542MIPI_write_cmos_sensor(0x0322,0x02);  
	HI542MIPI_write_cmos_sensor(0x0323,0x55);  
	HI542MIPI_write_cmos_sensor(0x0324,0x00);  
	HI542MIPI_write_cmos_sensor(0x0325,0xAB);  
	HI542MIPI_write_cmos_sensor(0x0326,0x02);  
	HI542MIPI_write_cmos_sensor(0x0327,0x55);  
	HI542MIPI_write_cmos_sensor(0x0328,0x00);  
	HI542MIPI_write_cmos_sensor(0x0329,0x01);  
	HI542MIPI_write_cmos_sensor(0x032A,0x00);  
	HI542MIPI_write_cmos_sensor(0x032B,0x10);  
	HI542MIPI_write_cmos_sensor(0x032C,0x00);  
	HI542MIPI_write_cmos_sensor(0x032D,0x01);  
	HI542MIPI_write_cmos_sensor(0x032E,0x00);  
	HI542MIPI_write_cmos_sensor(0x032F,0x10);  
	HI542MIPI_write_cmos_sensor(0x0330,0x00);  
	HI542MIPI_write_cmos_sensor(0x0331,0x02);  
	HI542MIPI_write_cmos_sensor(0x0332,0x00);  
	HI542MIPI_write_cmos_sensor(0x0333,0x2e);  
	HI542MIPI_write_cmos_sensor(0x0334,0x00);  
	HI542MIPI_write_cmos_sensor(0x0335,0x02);  
	HI542MIPI_write_cmos_sensor(0x0336,0x00);  
	HI542MIPI_write_cmos_sensor(0x0337,0x2e);  
	HI542MIPI_write_cmos_sensor(0x0358,0x00);  
	HI542MIPI_write_cmos_sensor(0x0359,0x46);  
	HI542MIPI_write_cmos_sensor(0x035A,0x05);  
	HI542MIPI_write_cmos_sensor(0x035B,0x59);  
	HI542MIPI_write_cmos_sensor(0x035C,0x00);  
	HI542MIPI_write_cmos_sensor(0x035D,0x46);  
	HI542MIPI_write_cmos_sensor(0x035E,0x05);  
	HI542MIPI_write_cmos_sensor(0x035F,0x59);  
	HI542MIPI_write_cmos_sensor(0x0360,0x00);  
	HI542MIPI_write_cmos_sensor(0x0361,0x46);  
	HI542MIPI_write_cmos_sensor(0x0362,0x00);  
	HI542MIPI_write_cmos_sensor(0x0363,0xa4);  //a2},Black sun
	HI542MIPI_write_cmos_sensor(0x0364,0x00);  
	HI542MIPI_write_cmos_sensor(0x0365,0x46);  
	HI542MIPI_write_cmos_sensor(0x0366,0x00);  
	HI542MIPI_write_cmos_sensor(0x0367,0xa4);  //a2},Black sun
	HI542MIPI_write_cmos_sensor(0x0368,0x00);  
	HI542MIPI_write_cmos_sensor(0x0369,0x46);  
	HI542MIPI_write_cmos_sensor(0x036A,0x00);  
	HI542MIPI_write_cmos_sensor(0x036B,0xa6);  //a9},S2 off
	HI542MIPI_write_cmos_sensor(0x036C,0x00);  
	HI542MIPI_write_cmos_sensor(0x036D,0x46);  
	HI542MIPI_write_cmos_sensor(0x036E,0x00);  
	HI542MIPI_write_cmos_sensor(0x036F,0xa6);  //a9},S2 off
	HI542MIPI_write_cmos_sensor(0x0370,0x00);  
	HI542MIPI_write_cmos_sensor(0x0371,0xb0);  
	HI542MIPI_write_cmos_sensor(0x0372,0x05);  
	HI542MIPI_write_cmos_sensor(0x0373,0x59);  
	HI542MIPI_write_cmos_sensor(0x0374,0x00);  
	HI542MIPI_write_cmos_sensor(0x0375,0xb0);  
	HI542MIPI_write_cmos_sensor(0x0376,0x05);  
	HI542MIPI_write_cmos_sensor(0x0377,0x59);  
	HI542MIPI_write_cmos_sensor(0x0378,0x00);  
	HI542MIPI_write_cmos_sensor(0x0379,0x45);  
	HI542MIPI_write_cmos_sensor(0x037A,0x00);  
	HI542MIPI_write_cmos_sensor(0x037B,0xAA);  
	HI542MIPI_write_cmos_sensor(0x037C,0x00);  
	HI542MIPI_write_cmos_sensor(0x037D,0x99);  
	HI542MIPI_write_cmos_sensor(0x037E,0x01);  
	HI542MIPI_write_cmos_sensor(0x037F,0xAE);  
	HI542MIPI_write_cmos_sensor(0x0380,0x01);  
	HI542MIPI_write_cmos_sensor(0x0381,0xB1);  
	HI542MIPI_write_cmos_sensor(0x0382,0x02);  
	HI542MIPI_write_cmos_sensor(0x0383,0x56);  
	HI542MIPI_write_cmos_sensor(0x0384,0x05);  
	HI542MIPI_write_cmos_sensor(0x0385,0x6D);  
	HI542MIPI_write_cmos_sensor(0x0386,0x00);  
	HI542MIPI_write_cmos_sensor(0x0387,0xDC);  
	HI542MIPI_write_cmos_sensor(0x03A0,0x05);  
	HI542MIPI_write_cmos_sensor(0x03A1,0x5E);  
	HI542MIPI_write_cmos_sensor(0x03A2,0x05);  
	HI542MIPI_write_cmos_sensor(0x03A3,0x62);  
	HI542MIPI_write_cmos_sensor(0x03A4,0x01);  
	HI542MIPI_write_cmos_sensor(0x03A5,0xc9);  
	HI542MIPI_write_cmos_sensor(0x03A6,0x01);  
	HI542MIPI_write_cmos_sensor(0x03A7,0x27);  
	HI542MIPI_write_cmos_sensor(0x03A8,0x05);  
	HI542MIPI_write_cmos_sensor(0x03A9,0x59);  
	HI542MIPI_write_cmos_sensor(0x03AA,0x02);  
	HI542MIPI_write_cmos_sensor(0x03AB,0x55);  
	HI542MIPI_write_cmos_sensor(0x03AC,0x01);  
	HI542MIPI_write_cmos_sensor(0x03AD,0xc5);  
	HI542MIPI_write_cmos_sensor(0x03AE,0x01);  
	HI542MIPI_write_cmos_sensor(0x03AF,0x27);  
	HI542MIPI_write_cmos_sensor(0x03B0,0x05);  
	HI542MIPI_write_cmos_sensor(0x03B1,0x55);  
	HI542MIPI_write_cmos_sensor(0x03B2,0x02);  
	HI542MIPI_write_cmos_sensor(0x03B3,0x55);  
	HI542MIPI_write_cmos_sensor(0x03B4,0x00);  
	HI542MIPI_write_cmos_sensor(0x03B5,0x0A);  
	
	//HI542MIPI_write_cmos_sensor(0x03D0,0xee);  
	//HI542MIPI_write_cmos_sensor(0x03D1,0x15);  
	//HI542MIPI_write_cmos_sensor(0x03D2,0xb0);  
	HI542MIPI_write_cmos_sensor(0x03D3,0x08);  
	//HI542MIPI_write_cmos_sensor(0x03D4,0x18);  //08},LDO OUTPUT 
	HI542MIPI_write_cmos_sensor(0x03D5,0x44);  
	HI542MIPI_write_cmos_sensor(0x03D6,0x51);  
	HI542MIPI_write_cmos_sensor(0x03D7,0x56);  
	HI542MIPI_write_cmos_sensor(0x03D8,0x44);  
	HI542MIPI_write_cmos_sensor(0x03D9,0x06);  

	//HI542MIPI_write_cmos_sensor(0x0500,0x18);  
	HI542MIPI_write_cmos_sensor(0x0580,0x01);  
	HI542MIPI_write_cmos_sensor(0x0581,0x00);  
	HI542MIPI_write_cmos_sensor(0x0582,0x80);  
	HI542MIPI_write_cmos_sensor(0x0583,0x00);  
	HI542MIPI_write_cmos_sensor(0x0584,0x80);  
	HI542MIPI_write_cmos_sensor(0x0585,0x00);  
	HI542MIPI_write_cmos_sensor(0x0586,0x80);  
	HI542MIPI_write_cmos_sensor(0x0587,0x00);  
	HI542MIPI_write_cmos_sensor(0x0588,0x80);  
	HI542MIPI_write_cmos_sensor(0x0589,0x00);  
	HI542MIPI_write_cmos_sensor(0x058A,0x80);  
	
	//HI542MIPI_write_cmos_sensor(0x05A0,0x01);  
	//HI542MIPI_write_cmos_sensor(0x05B0,0x01);  
	HI542MIPI_write_cmos_sensor(0x05C2,0x00);  
	HI542MIPI_write_cmos_sensor(0x05C3,0x00);  
	HI542MIPI_write_cmos_sensor(0x0080,0xC7);  
	HI542MIPI_write_cmos_sensor(0x0119,0x00);  
	HI542MIPI_write_cmos_sensor(0x011A,0x15);  
	HI542MIPI_write_cmos_sensor(0x011B,0xC0);  
/*
	HI542MIPI_write_cmos_sensor(0x0115,0x00);  
	HI542MIPI_write_cmos_sensor(0x0116,0x2A);  
	HI542MIPI_write_cmos_sensor(0x0117,0x4C);  
	HI542MIPI_write_cmos_sensor(0x0118,0x20);  
*/
	HI542MIPI_write_cmos_sensor(0x0223,0xED);  
	HI542MIPI_write_cmos_sensor(0x0224,0xE4);  
	HI542MIPI_write_cmos_sensor(0x0225,0x09);  
	HI542MIPI_write_cmos_sensor(0x0226,0x36);  
	HI542MIPI_write_cmos_sensor(0x023E,0x80);  
	HI542MIPI_write_cmos_sensor(0x05B0,0x00);  
	//HI542MIPI_write_cmos_sensor(0x03D0,0xe9);  
	//HI542MIPI_write_cmos_sensor(0x03D1,0x75);  
	HI542MIPI_write_cmos_sensor(0x03D2,0xAD);

	HI542MIPI_write_cmos_sensor(0x0616,0x00);
	HI542MIPI_write_cmos_sensor(0x0616,0x01);
	HI542MIPI_write_cmos_sensor(0x03D2,0xAC);
	HI542MIPI_write_cmos_sensor(0x03D0,0xe9);  
	HI542MIPI_write_cmos_sensor(0x03D1,0x75);  


	
	HI542MIPI_write_cmos_sensor(0x0800,0x07);  //07},//0F},EMI disable
	HI542MIPI_write_cmos_sensor(0x0801,0x08);  
	HI542MIPI_write_cmos_sensor(0x0802,0x02);  //04},//00},apb clock speed down
	//HI542MIPI_write_cmos_sensor(0x0010,0x05);  
	HI542MIPI_write_cmos_sensor(0x0012,0x00);  
	HI542MIPI_write_cmos_sensor(0x0013,0x00);  
	HI542MIPI_write_cmos_sensor(0x0024,0x07);  
	HI542MIPI_write_cmos_sensor(0x0025,0xA8);  
	HI542MIPI_write_cmos_sensor(0x0026,0x0A);  
	HI542MIPI_write_cmos_sensor(0x0027,0x30);  
	HI542MIPI_write_cmos_sensor(0x0030,0x00);  
	HI542MIPI_write_cmos_sensor(0x0031,0x03);  
	HI542MIPI_write_cmos_sensor(0x0032,0x07);  
	HI542MIPI_write_cmos_sensor(0x0033,0xAC);  
	HI542MIPI_write_cmos_sensor(0x0034,0x03);  
	HI542MIPI_write_cmos_sensor(0x0035,0xD4);  
	HI542MIPI_write_cmos_sensor(0x003A,0x00);  
	HI542MIPI_write_cmos_sensor(0x003B,0x2E);  
	HI542MIPI_write_cmos_sensor(0x004A,0x03);  
	HI542MIPI_write_cmos_sensor(0x004B,0xD4);  
	HI542MIPI_write_cmos_sensor(0x004C,0x05);  
	HI542MIPI_write_cmos_sensor(0x004D,0x18);  
	HI542MIPI_write_cmos_sensor(0x0C98,0x05);  
	HI542MIPI_write_cmos_sensor(0x0C99,0x5E);  
	HI542MIPI_write_cmos_sensor(0x0C9A,0x05);  
	HI542MIPI_write_cmos_sensor(0x0C9B,0x62);
	
	//HI542MIPI_write_cmos_sensor(0x0500,0x18);  
	HI542MIPI_write_cmos_sensor(0x05A0,0x01);  
	HI542MIPI_write_cmos_sensor(0x0084,0x30);  //10},BLC control
	HI542MIPI_write_cmos_sensor(0x008D,0xFF);  
	HI542MIPI_write_cmos_sensor(0x0090,0x02);  //0b},BLC defect pixel th
	HI542MIPI_write_cmos_sensor(0x00A7,0x80);  //FF},
	HI542MIPI_write_cmos_sensor(0x021A,0x15);  
	HI542MIPI_write_cmos_sensor(0x022B,0xb0);  //f0},RAMP filter
	HI542MIPI_write_cmos_sensor(0x0232,0x37);  //17},black sun enable
	HI542MIPI_write_cmos_sensor(0x0010,0x41);  //01},
	HI542MIPI_write_cmos_sensor(0x0740,0x1A);  
	HI542MIPI_write_cmos_sensor(0x0742,0x1A);  
	HI542MIPI_write_cmos_sensor(0x0743,0x1A);  
	HI542MIPI_write_cmos_sensor(0x0744,0x1A);  
	HI542MIPI_write_cmos_sensor(0x0745,0x04);  
	HI542MIPI_write_cmos_sensor(0x0746,0x32);  
	HI542MIPI_write_cmos_sensor(0x0747,0x05);  
	HI542MIPI_write_cmos_sensor(0x0748,0x01);  
	HI542MIPI_write_cmos_sensor(0x0749,0x90);  
	HI542MIPI_write_cmos_sensor(0x074A,0x1A);  
	HI542MIPI_write_cmos_sensor(0x074B,0xB1);							 
	HI542MIPI_write_cmos_sensor(0x0500,0x19);  //0x19},//1b},LSC disable
	HI542MIPI_write_cmos_sensor(0x0510,0x10);  
									   
	HI542MIPI_write_cmos_sensor(0x0217,0x44);  //adaptive NCP on
	HI542MIPI_write_cmos_sensor(0x0218,0x00);  //scn_sel
									   
	HI542MIPI_write_cmos_sensor(0x02ac,0x00);  //outdoor on
	HI542MIPI_write_cmos_sensor(0x02ad,0x00);  
	HI542MIPI_write_cmos_sensor(0x02ae,0x00);  //outdoor off
	HI542MIPI_write_cmos_sensor(0x02af,0x00);  
	HI542MIPI_write_cmos_sensor(0x02b0,0x00);  //indoor on
	HI542MIPI_write_cmos_sensor(0x02b1,0x00);  
	HI542MIPI_write_cmos_sensor(0x02b2,0x00);  //indoor off
	HI542MIPI_write_cmos_sensor(0x02b3,0x00);  
	HI542MIPI_write_cmos_sensor(0x02b4,0x60);  //dark1 on
	HI542MIPI_write_cmos_sensor(0x02b5,0x21);  
	HI542MIPI_write_cmos_sensor(0x02b6,0x66);  //dark1 off
	HI542MIPI_write_cmos_sensor(0x02b7,0x8a);  
									   
	HI542MIPI_write_cmos_sensor(0x02c0,0x36);  //outdoor NCP en
	HI542MIPI_write_cmos_sensor(0x02c1,0x36);  //indoor NCP en
	HI542MIPI_write_cmos_sensor(0x02c2,0x36);  //dark1 NCP en
	HI542MIPI_write_cmos_sensor(0x02c3,0x36);  //3f},//dark2 NCP disable
	HI542MIPI_write_cmos_sensor(0x02c4,0xE4);  //outdoor NCP voltage
	HI542MIPI_write_cmos_sensor(0x02c5,0xE4);  //indoor NCP voltage
	HI542MIPI_write_cmos_sensor(0x02c6,0xE4);  //dark1 NCP voltage
	HI542MIPI_write_cmos_sensor(0x02c7,0xdb);  //24},//dark2 NCP voltage

	//#if define MIPI_INTERFACE
	HI542MIPI_write_cmos_sensor(0x061A,0x01);  
	HI542MIPI_write_cmos_sensor(0x061B,0x03);  
	HI542MIPI_write_cmos_sensor(0x061C,0x00);  
	HI542MIPI_write_cmos_sensor(0x061D,0x00);  
	HI542MIPI_write_cmos_sensor(0x061E,0x00);  
	HI542MIPI_write_cmos_sensor(0x061F,0x03);  
	HI542MIPI_write_cmos_sensor(0x0613,0x01);  
	HI542MIPI_write_cmos_sensor(0x0615,0x01);  
	HI542MIPI_write_cmos_sensor(0x0616,0x01);  
	HI542MIPI_write_cmos_sensor(0x0617,0x00);  
	HI542MIPI_write_cmos_sensor(0x0619,0x00); 

	HI542MIPI_write_cmos_sensor(0x0661,0x03);  
	HI542MIPI_write_cmos_sensor(0x0650,0x03);  
	HI542MIPI_write_cmos_sensor(0x0651,0x02);  
	HI542MIPI_write_cmos_sensor(0x0652,0x10);  
	HI542MIPI_write_cmos_sensor(0x0655,0x04);  
	HI542MIPI_write_cmos_sensor(0x0654,0x08); 

	
	HI542MIPI_write_cmos_sensor(0x0008,0x0F);  
	HI542MIPI_write_cmos_sensor(0x0630,0x05);  
	HI542MIPI_write_cmos_sensor(0x0631,0x18);  
	HI542MIPI_write_cmos_sensor(0x0632,0x03);  
	HI542MIPI_write_cmos_sensor(0x0633,0xD4);  
	HI542MIPI_write_cmos_sensor(0x0663,0x05);  //0a},trail time 
	HI542MIPI_write_cmos_sensor(0x0660,0x03);  	
	
	
	HI542MIPI_write_cmos_sensor(0x0001,0x00);	//PWRCTLB  
	
	//END
	//[END]
	HI542MIPI_Auto_Flicker_mode = KAL_FALSE; 

    printk("[HI542Raw] Init Success \n");
}   /*  HI542_Sensor_Init  */

void HI542MIPI_set_preview_setting(void)
{
    //int temp;
    spin_lock(&hi542mipi_drv_lock);
    HI542MIPI_g_RES = HI542MIPI_720P;
	spin_unlock(&hi542mipi_drv_lock);
	//DISP_HEIGHT = 968
	//DISP_FORMAT = BAYER10
	//DISP_DATABUS = 16
	//DISP_DATAORDER = BG
	//MCLK = 24.00
	//PLL = 2.00
	
	//BEGIN
	//I2C_ID = 0x14
	//I2C_BYTE	= 0x21
	//
	////// Mode Controls
	//HI542MIPI_write_cmos_sensor(0x0014,0x42); 	// B[7] Justification control (Data Justification on output Data)
	//HI542MIPI_write_cmos_sensor(0x0015,0x00); 	// B[7] Tris,PCLK Inverting
	//HI542MIPI_write_cmos_sensor(0x0036,0x00); 	// MIPI Spec --> 0.9
	//HI542MIPI_write_cmos_sensor(0x0017,0x2B); // YUV422:0x18 RAW10:0x2B RAW8:0x2A RAW12:0x2C
	//HI542MIPI_write_cmos_sensor(0x0019,0x04); 	// Select Data_ID by REGISTER(0x0017)
	//HI542MIPI_write_cmos_sensor(0x0002,0x15); // B[7:2] ui_x4_clk_lane (Unit interval time step 0.25ns) 3ns  0.25 X 12
	//HI542MIPI_write_cmos_sensor(0x0003,0x00); 	// B[4] hs_invert_clk_lane (Invert HS signals)
	//HI542MIPI_write_cmos_sensor(0x0004,0x02);  // B[4] hs_rx_term_e_subLVDS_clk_lane (Enables the hs-termination for subLVDs mode)
	//HI542MIPI_write_cmos_sensor(0x0005,0x03); 	// B[5] force_rx_mod_data_lane (Force lane module into receive mode wait for stop state)
	//HI542MIPI_write_cmos_sensor(0x0006,0x01); 	// B[5] Cd_off Contention_detector on/off
	//HI542MIPI_write_cmos_sensor(0x0009,0x01); 	// B[5] force_rx_mod_data_lane (Force lane module into receive mode wait for stop state)
	//HI542MIPI_write_cmos_sensor(0x000A,0x01); 	// B[5] Cd_off Contention_detector on/off
	
	//I2C_ID = 0x40
	//I2C_BYTE	= 0x21

	//HI542MIPI_write_cmos_sensor(0x0632, 0x00);
	//HI542MIPI_write_cmos_sensor(0x0633, 0x00);



//                   fixed 15fps
	HI542MIPI_write_cmos_sensor(0x0001, 0x01);	  
	HI542MIPI_write_cmos_sensor(0x0617, 0x01);
	HI542MIPI_write_cmos_sensor(0x0001, 0x00);
	HI542MIPI_write_cmos_sensor(0x0001, 0x01);
	HI542MIPI_write_cmos_sensor(0x0617, 0x00);
	
	HI542MIPI_write_cmos_sensor(0x0010, 0x41);
	HI542MIPI_write_cmos_sensor(0x0011, 0x03);
	HI542MIPI_write_cmos_sensor(0x0034, 0x03);
	HI542MIPI_write_cmos_sensor(0x0035, 0xD4);
	HI542MIPI_write_cmos_sensor(0x0040, 0x00);
	HI542MIPI_write_cmos_sensor(0x0041, 0x35);
//changed by zhiqiang

	HI542MIPI_write_cmos_sensor(0x0042, 0x00);
	HI542MIPI_write_cmos_sensor(0x0043, 0x14);
	HI542MIPI_write_cmos_sensor(0x0500, 0x19);

	HI542MIPI_write_cmos_sensor(0x0630, 0x05);
	HI542MIPI_write_cmos_sensor(0x0631, 0x18);//08},
	HI542MIPI_write_cmos_sensor(0x0632, 0x03);
	HI542MIPI_write_cmos_sensor(0x0633, 0xD4);//C8},
#if 0
	HI542MIPI_write_cmos_sensor(0x0011, 0x03); //fixed frame setting turn off
	HI542MIPI_write_cmos_sensor(0x0013, 0x00); //fixed frame setting turn off
#else
HI542MIPI_write_cmos_sensor(0x0011, 0x07); //fixed frame setting turn off
HI542MIPI_write_cmos_sensor(0x0013, 0x40); //fixed frame setting turn off

#endif


	//HI542MIPI_write_cmos_sensor(0x0010, 0x01);
	//HI542MIPI_write_cmos_sensor(0x0011, 0x00);
	//HI542MIPI_write_cmos_sensor(0x0120, 0x00); //Fixed exposure time 15
	//HI542MIPI_write_cmos_sensor(0x0121, 0x55); //Fixed exposure time 15
	//HI542MIPI_write_cmos_sensor(0x0122, 0x79); //Fixed exposure time 15
	//HI542MIPI_write_cmos_sensor(0x0123, 0x01); //Fixed exposure time 15
	
	HI542MIPI_write_cmos_sensor(0x011C,0x1F); // Max exposure time 
	HI542MIPI_write_cmos_sensor(0x011D,0xFF); // Max exposure time 
	HI542MIPI_write_cmos_sensor(0x011E,0xFF); // Max exposure time/
	HI542MIPI_write_cmos_sensor(0x011F,0xFF); // Max exposure time 
/*
	HI542MIPI_write_cmos_sensor(0x0115, 0x00); //// manual exposure time
	HI542MIPI_write_cmos_sensor(0x0116, 0x2A); //// manual exposure time
	HI542MIPI_write_cmos_sensor(0x0117, 0x4C); //// manual exposure time
	HI542MIPI_write_cmos_sensor(0x0118, 0x20); //// manual exposure time
*/
	HI542MIPI_write_cmos_sensor(0x0001, 0x00);	 
	
	//END
	//[END]
    spin_lock(&hi542mipi_drv_lock);
    HI542MIPI_PV_pclk = 8400; 
    HI542MIPI_sensor_pclk=84000000;//19500000;
    spin_unlock(&hi542mipi_drv_lock);
	
    SENSORDB("Set 720P End\n"); 
}

/*******************************************************************************
*
capture setting
********************************************************************************/
void HI542MIPI_set_5M15fps(void)
{
	
		spin_lock(&hi542mipi_drv_lock);
		HI542MIPI_g_RES = HI542MIPI_5M;
		spin_unlock(&hi542mipi_drv_lock);
	
	
	
		//HI542MIPI_write_cmos_sensor(0x0632, 0x00);
		//HI542MIPI_write_cmos_sensor(0x0633, 0x00);
		HI542MIPI_write_cmos_sensor(0x0001, 0x01);	  
		HI542MIPI_write_cmos_sensor(0x0617, 0x01);
		HI542MIPI_write_cmos_sensor(0x0001, 0x00);
		HI542MIPI_write_cmos_sensor(0x0001, 0x01);
		HI542MIPI_write_cmos_sensor(0x0617, 0x00);
		
	//	HI542MIPI_write_cmos_sensor(0x0020, 0x00);
	//	HI542MIPI_write_cmos_sensor(0x0021, 0x00);
	//	HI542MIPI_write_cmos_sensor(0x0022, 0x00);
	//	HI542MIPI_write_cmos_sensor(0x0023, 0x00);
	
	
		HI542MIPI_write_cmos_sensor(0x0010, 0x40);
		HI542MIPI_write_cmos_sensor(0x0011, 0x07);
		HI542MIPI_write_cmos_sensor(0x0013, 0x40);
	
		HI542MIPI_write_cmos_sensor(0x0034, 0x07);
		HI542MIPI_write_cmos_sensor(0x0035, 0xA8);
	
		HI542MIPI_write_cmos_sensor(0x0500, 0x11);
		
		HI542MIPI_write_cmos_sensor(0x0630, 0x0A);
		HI542MIPI_write_cmos_sensor(0x0631, 0x30);
		HI542MIPI_write_cmos_sensor(0x0632, 0x07);
		HI542MIPI_write_cmos_sensor(0x0633, 0xA8);
	
		HI542MIPI_write_cmos_sensor(0x0001, 0x00);	
	
	
	
	
		spin_lock(&hi542mipi_drv_lock);
		HI542MIPI_CAP_pclk = 8400; 
		HI542MIPI_sensor_pclk = 84000000;//19500000;
		spin_unlock(&hi542mipi_drv_lock);
		
		SENSORDB("Set 5M End\n"); 
}






void HI542MIPI_set_video15fps(void)
{

	HI542MIPI_write_cmos_sensor(0x0001, 0x01);	  
	HI542MIPI_write_cmos_sensor(0x0617, 0x01);
	HI542MIPI_write_cmos_sensor(0x0001, 0x00);
	HI542MIPI_write_cmos_sensor(0x0001, 0x01);
	HI542MIPI_write_cmos_sensor(0x0617, 0x00);
	
	HI542MIPI_write_cmos_sensor(0x0010, 0x41);
	HI542MIPI_write_cmos_sensor(0x0011, 0x03);
	
	HI542MIPI_write_cmos_sensor(0x0034, 0x03);
	HI542MIPI_write_cmos_sensor(0x0035, 0xD4);

	HI542MIPI_write_cmos_sensor(0x0040, 0x00);
	HI542MIPI_write_cmos_sensor(0x0041, 0x35);
//changed by zhiqiang  original 35
	
	HI542MIPI_write_cmos_sensor(0x0042, 0x03);
	HI542MIPI_write_cmos_sensor(0x0043, 0x28);

	HI542MIPI_write_cmos_sensor(0x0500, 0x19);
	
	HI542MIPI_write_cmos_sensor(0x0630, 0x05);
	HI542MIPI_write_cmos_sensor(0x0631, 0x18);
	HI542MIPI_write_cmos_sensor(0x0632, 0x03);
	HI542MIPI_write_cmos_sensor(0x0633, 0xD4);

	HI542MIPI_write_cmos_sensor(0x0011, 0x07);	
	HI542MIPI_write_cmos_sensor(0x0013, 0x40);
	
	HI542MIPI_write_cmos_sensor(0x0120, 0x00);
	HI542MIPI_write_cmos_sensor(0x0121, 0x55);
	HI542MIPI_write_cmos_sensor(0x0122, 0x79);
	HI542MIPI_write_cmos_sensor(0x0123, 0x01);
	
	HI542MIPI_write_cmos_sensor(0x011C, 0x00);	
	HI542MIPI_write_cmos_sensor(0x011D, 0x55);
	HI542MIPI_write_cmos_sensor(0x011E, 0x63);
	HI542MIPI_write_cmos_sensor(0x011F, 0x33);
/*
	HI542MIPI_write_cmos_sensor(0x0115, 0x00);
	HI542MIPI_write_cmos_sensor(0x0116, 0x55);
	HI542MIPI_write_cmos_sensor(0x0117, 0x79);
	HI542MIPI_write_cmos_sensor(0x0118, 0x01);
*/
	HI542MIPI_write_cmos_sensor(0x0001, 0x00);
	spin_lock(&hi542mipi_drv_lock);
	Fix_framerate=1;
	spin_unlock(&hi542mipi_drv_lock);

    SENSORDB("Set fixed15fps End\n"); 
}



void HI542MIPI_set_video30fps(void)
{

	HI542MIPI_write_cmos_sensor(0x0001, 0x01);	  
	HI542MIPI_write_cmos_sensor(0x0617, 0x01);
	HI542MIPI_write_cmos_sensor(0x0001, 0x00);
	HI542MIPI_write_cmos_sensor(0x0001, 0x01);
	HI542MIPI_write_cmos_sensor(0x0617, 0x00);
	
	HI542MIPI_write_cmos_sensor(0x0010, 0x41);
	HI542MIPI_write_cmos_sensor(0x0011, 0x03);
	
	HI542MIPI_write_cmos_sensor(0x0034, 0x03);
	HI542MIPI_write_cmos_sensor(0x0035, 0xD4);

	HI542MIPI_write_cmos_sensor(0x0040, 0x00);
	HI542MIPI_write_cmos_sensor(0x0041, 0x35);
//changed by zhiqiang   original 35
	
	HI542MIPI_write_cmos_sensor(0x0042, 0x00);
	HI542MIPI_write_cmos_sensor(0x0043, 0x10);//20

	HI542MIPI_write_cmos_sensor(0x0500, 0x19);
	
	HI542MIPI_write_cmos_sensor(0x0630, 0x05);
	HI542MIPI_write_cmos_sensor(0x0631, 0x18);
	HI542MIPI_write_cmos_sensor(0x0632, 0x03);
	HI542MIPI_write_cmos_sensor(0x0633, 0xD4);

	HI542MIPI_write_cmos_sensor(0x0011, 0x07);	
	HI542MIPI_write_cmos_sensor(0x0013, 0x40);

	HI542MIPI_write_cmos_sensor(0x0120, 0x00);
	HI542MIPI_write_cmos_sensor(0x0121, 0x29);//2A
	HI542MIPI_write_cmos_sensor(0x0122, 0x5a);//c1
	HI542MIPI_write_cmos_sensor(0x0123, 0x2d);//f4

	HI542MIPI_write_cmos_sensor(0x011C, 0x00);//1f	
	HI542MIPI_write_cmos_sensor(0x011D, 0x29);//ff
	HI542MIPI_write_cmos_sensor(0x011E, 0x44);//ff
	HI542MIPI_write_cmos_sensor(0x011F, 0x5a);//ff
/*
	HI542MIPI_write_cmos_sensor(0x0115, 0x00);
	HI542MIPI_write_cmos_sensor(0x0116, 0x26);
	HI542MIPI_write_cmos_sensor(0x0117, 0xb6);
	HI542MIPI_write_cmos_sensor(0x0118, 0x3b);
*/
	HI542MIPI_write_cmos_sensor(0x0001, 0x00);	
	spin_lock(&hi542mipi_drv_lock);
	Fix_framerate=1;
	spin_unlock(&hi542mipi_drv_lock);
    SENSORDB("Set fixed30fps End\n"); 
}


void HI542MIPI_set_video_1080P()
{
	SENSORDB("Set fixed 1080P video\n");
	HI542MIPI_write_cmos_sensor(0x0011,0x07);
	HI542MIPI_write_cmos_sensor(0x0013,0x40);
	HI542MIPI_write_cmos_sensor(0x0120,0x00);
	HI542MIPI_write_cmos_sensor(0x0121,0x54);
	HI542MIPI_write_cmos_sensor(0x0122,0x9E);
	HI542MIPI_write_cmos_sensor(0x0123,0xF5);
	HI542MIPI_write_cmos_sensor(0x011C,0x00);
	HI542MIPI_write_cmos_sensor(0x011D,0x54);
	HI542MIPI_write_cmos_sensor(0x011E,0x89);
	HI542MIPI_write_cmos_sensor(0x011F,0x27);
/*	
	HI542MIPI_write_cmos_sensor(0x0115,0x00);
	HI542MIPI_write_cmos_sensor(0x0116,0x54);
	HI542MIPI_write_cmos_sensor(0x0117,0x89);
	HI542MIPI_write_cmos_sensor(0x0118,0x27);
*/

}



/*******************************************************************************
*
********************************************************************************/
void HI542MIPI_set_5M(void)
{
    spin_lock(&hi542mipi_drv_lock);
    HI542MIPI_g_RES = HI542MIPI_5M;
	spin_unlock(&hi542mipi_drv_lock);

    HI542MIPI_write_cmos_sensor(0x3503,0x7 );

    HI542MIPI_write_cmos_sensor(0x3001,0x40);
    HI542MIPI_write_cmos_sensor(0x3002,0x1c);
    HI542MIPI_write_cmos_sensor(0x300d,0x21);
    //HI542MIPI_write_cmos_sensor(0x3011,0x8 );//pll
    //HI542MIPI_write_cmos_sensor(0x3010,0x30);//pll
    //HI542MIPI_write_cmos_sensor(0x3012,0x00);//pll

    //HI542MIPI_write_cmos_sensor(0x350c,0x07);
    //HI542MIPI_write_cmos_sensor(0x350d,0xd0);

    HI542MIPI_write_cmos_sensor(0x3622,0x60);
    HI542MIPI_write_cmos_sensor(0x3621,0x09);//29
    HI542MIPI_write_cmos_sensor(0x3709,0x0 );
    HI542MIPI_write_cmos_sensor(0x3600,0x54);
    HI542MIPI_write_cmos_sensor(0x3602,0xe4);
    HI542MIPI_write_cmos_sensor(0x3606,0x1b);
    HI542MIPI_write_cmos_sensor(0x3612,0xac);
    HI542MIPI_write_cmos_sensor(0x3613,0x44);
    HI542MIPI_write_cmos_sensor(0x3623,0x22);
    HI542MIPI_write_cmos_sensor(0x3705,0xda);
    HI542MIPI_write_cmos_sensor(0x370a,0x80);

    HI542MIPI_write_cmos_sensor(0x3801,0xb4);
    HI542MIPI_write_cmos_sensor(0x3804,0x0a);
    HI542MIPI_write_cmos_sensor(0x3805,0x20);
    HI542MIPI_write_cmos_sensor(0x3806,0x07);
    HI542MIPI_write_cmos_sensor(0x3807,0x98);
    HI542MIPI_write_cmos_sensor(0x3808,0x0a);
    HI542MIPI_write_cmos_sensor(0x3809,0x20);
    HI542MIPI_write_cmos_sensor(0x380a,0x07);
    HI542MIPI_write_cmos_sensor(0x380b,0x98);
    HI542MIPI_write_cmos_sensor(0x380c,0x0c);
    HI542MIPI_write_cmos_sensor(0x380d,0x80);
    HI542MIPI_write_cmos_sensor(0x380e,0x07);
    HI542MIPI_write_cmos_sensor(0x380f,0xd0);
    
    HI542MIPI_write_cmos_sensor(0x3810,0xc2);
    HI542MIPI_write_cmos_sensor(0x3818,0xc0);//80
    HI542MIPI_write_cmos_sensor(0x3824,0x11);
    HI542MIPI_write_cmos_sensor(0x3825,0xac);
    HI542MIPI_write_cmos_sensor(0x3827,0xc );

    HI542MIPI_write_cmos_sensor(0x3a1a,0x4 );
    HI542MIPI_write_cmos_sensor(0x3a08,0x9 );
    HI542MIPI_write_cmos_sensor(0x3a09,0x60);
    HI542MIPI_write_cmos_sensor(0x3a0a,0x7 );
    HI542MIPI_write_cmos_sensor(0x3a0b,0xd0);
    HI542MIPI_write_cmos_sensor(0x3a0d,0x10);
    HI542MIPI_write_cmos_sensor(0x3a0e,0xd );

    HI542MIPI_write_cmos_sensor(0x401c,0x6 );
    HI542MIPI_write_cmos_sensor(0x5000,0x6 );
    HI542MIPI_write_cmos_sensor(0x5001,0x1 );//awb
    HI542MIPI_write_cmos_sensor(0x5005,0x0 );
    HI542MIPI_write_cmos_sensor(0x3011,0x13 );    //increase PLL, fps = 9.64

    // PLL Setting, set PCLK to 61.75, and fps = 9.6fps 
    spin_lock(&hi542mipi_drv_lock);
    HI542MIPI_CAP_pclk = 8400; 
    HI542MIPI_sensor_pclk=84000000;//19500000;
	spin_unlock(&hi542mipi_drv_lock);
	
    SENSORDB("Set 5M End\n"); 
}


/*****************************************************************************/
/* Windows Mobile Sensor Interface */
/*****************************************************************************/
/*************************************************************************
* FUNCTION
*   HI542Open
*
* DESCRIPTION
*   This function initialize the registers of CMOS sensor
*
* PARAMETERS
*   None
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/

UINT32 HI542MIPIOpen(void)
{
    int  retry = 0; 
	kal_uint16 temp_data;

	temp_data= HI542MIPI_read_cmos_sensor(0x0004);
	
	spin_lock(&hi542mipi_drv_lock);
    HI542MIPI_sensor_id = temp_data;
	spin_unlock(&hi542mipi_drv_lock);
    // check if sensor ID correct
    retry = 3; 
    do {
       
     	temp_data= HI542MIPI_read_cmos_sensor(0x0004);
		
        spin_lock(&hi542mipi_drv_lock);
        HI542MIPI_sensor_id = temp_data;
	    spin_unlock(&hi542mipi_drv_lock);  
        if (HI542MIPI_sensor_id == HI542MIPI_SENSOR_ID)
            break; 
        SENSORDB("Read Sensor ID Fail = 0x%04x\n", HI542MIPI_sensor_id); 
        retry--; 
    } while (retry > 0);

    if (HI542MIPI_sensor_id != HI542MIPI_SENSOR_ID)
        return ERROR_SENSOR_CONNECT_FAIL;
	
	printk("[HI542raw]:HI542Open Read Sensor ID :0x%x\n", HI542MIPI_sensor_id);  

    HI542MIPI_Sensor_Init();

	temp_data = read_HI542MIPI_gain();
	
	spin_lock(&hi542mipi_drv_lock);
    HI542MIPI_sensor_gain_base = temp_data;
	spin_unlock(&hi542mipi_drv_lock);

    printk("[HI542raw]:HI542Open Read Sensor BaseGain :0x%x\n", HI542MIPI_sensor_gain_base); 
	printk("[HI542raw]:HI542Open Read Sensor REG 0x0024 :0x%x\n", HI542MIPI_read_cmos_sensor(0x0024));
	printk("[HI542raw]:HI542Open Read Sensor REG 0x0025 :0x%x\n", HI542MIPI_read_cmos_sensor(0x0025));
	printk("[HI542raw]:HI542Open Read Sensor REG 0x0026 :0x%x\n", HI542MIPI_read_cmos_sensor(0x0026));
	printk("[HI542raw]:HI542Open Read Sensor REG 0x0027 :0x%x\n", HI542MIPI_read_cmos_sensor(0x0027));
	printk("[HI542raw]:HI542Open Read Sensor REG 0x0500 :0x%x\n", HI542MIPI_read_cmos_sensor(0x0500));

	spin_lock(&hi542mipi_drv_lock);
    HI542MIPI_g_iBackupExtraExp = 0;
	Fix_framerate=0;
    spin_unlock(&hi542mipi_drv_lock);	
	

    return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
*   HI542GetSensorID
*
* DESCRIPTION
*   This function get the sensor ID 
*
* PARAMETERS
*   *sensorID : return the sensor ID 
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 HI542MIPIGetSensorID(UINT32 *sensorID) 
{
    int  retry = 3; 
	kal_uint16 temp_data;

	temp_data= HI542MIPI_read_cmos_sensor(0x0004);
	
	spin_lock(&hi542mipi_drv_lock);
    HI542MIPI_sensor_id = temp_data;
	spin_unlock(&hi542mipi_drv_lock);
    // check if sensor ID correct
    retry = 3; 
    do {
     	temp_data= HI542MIPI_read_cmos_sensor(0x0004);
		
        spin_lock(&hi542mipi_drv_lock);
        HI542MIPI_sensor_id = temp_data;
	    spin_unlock(&hi542mipi_drv_lock);  
        if (HI542MIPI_sensor_id == HI542MIPI_SENSOR_ID)
            break; 
        SENSORDB("Read Sensor ID Fail = 0x%04x\n", HI542MIPI_sensor_id); 
        retry--; 
    } while (retry > 0);


	printk("[HI542raw]:HI542OMIPIpen Read Sensor ID :0x%x\n", HI542MIPI_sensor_id);  
	
    *sensorID = HI542MIPI_sensor_id;

    if (*sensorID != HI542MIPI_SENSOR_ID) {
        *sensorID = 0xFFFFFFFF; 
        return ERROR_SENSOR_CONNECT_FAIL;
    }
    return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
*   HI542_SetShutter
*
* DESCRIPTION
*   This function set e-shutter of HI542 to change exposure time.
*
* PARAMETERS
*   shutter : exposured lines
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void HI542MIPI_SetShutter(kal_uint16 iShutter)
{
	unsigned long flags;
	kal_uint32 realshutter = iShutter;
    if (realshutter < 4 )
        realshutter = 4;

    printk("[HI542raw FUNCTION TEST]:HI542_SetShutter iShutter :0x%x\n", realshutter);

    spin_lock_irqsave(&hi542mipi_drv_lock,flags);
    HI542MIPI_pv_exposure_lines = realshutter;
	spin_unlock_irqrestore(&hi542mipi_drv_lock,flags);
    /*  shutter calc
            EXPTIME[28:0]={EXPTIMEH[4:0],EXPTIMEM1[7:0],EXPTIMEM2[7:0],EXPTIMEL[7:0]}
            Exposure time = EXPTIME/OPCLK       in pixels
      */

/*
	if(HI542MIPI_MPEG4_encode_mode == KAL_TRUE)
	{
		if(HI542MIPI_V_mode == HI542MIPI_Video_Night)
		{
			realshutter = 0x07D7;
		}
	}
  */ 
    HI542MIPI_write_shutter(realshutter);

}   /*  HI542_SetShutter   */



/*************************************************************************
* FUNCTION
*   HI542_read_shutter
*
* DESCRIPTION
*   This function to  Get exposure time.
*
* PARAMETERS
*   None
*
* RETURNS
*   shutter : exposured lines
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT16 HI542MIPI_read_shutter(void)
{
    kal_uint32 iExp;
	kal_uint16 PixelsOneline = HI542MIPI_FULL_PERIOD_PIXEL_NUMS;
    if(HI542MIPI_720P == HI542MIPI_g_RES)
    {
        PixelsOneline = (HI542MIPI_PV_PERIOD_PIXEL_NUMS + HI542MIPI_dummy_pixels + HI542MIPI_PV_PERIOD_EXTRA_PIXEL_NUMS);
    }
    else if (HI542MIPI_5M == HI542MIPI_g_RES)
    {
        PixelsOneline = HI542MIPI_FULL_PERIOD_PIXEL_NUMS + HI542MIPI_dummy_pixels + HI542MIPI_FULL_PERIOD_EXTRA_PIXEL_NUMS;
    }
   
    /*  shutter calc
            EXPTIME[28:0]={EXPTIMEH[4:0],EXPTIMEM1[7:0],EXPTIMEM2[7:0],EXPTIMEL[7:0]}
            Exposure time = EXPTIME/OPCLK       in pixels
      */

    iExp = (HI542MIPI_read_cmos_sensor(0x0115)<<24) |(HI542MIPI_read_cmos_sensor(0x0116)<<16) |(HI542MIPI_read_cmos_sensor(0x0117)<<8) |(HI542MIPI_read_cmos_sensor(0x0118));
    iExp = iExp/PixelsOneline;

    if (iExp < 4 )
        iExp = 4;

	printk("[HI542raw]:HI542MIPI_read_shutter iExp :0x%x\n",iExp);

    return iExp;
}

/*************************************************************************
* FUNCTION
*   HI542_night_mode
*
* DESCRIPTION
*   This function night mode of HI542.
*
* PARAMETERS
*   none
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void HI542MIPI_NightMode(kal_bool bEnable)
{

printk("[HI542raw FUNCTION TEST]:HI542_NightMode .........................");

#if 0
    /************************************************************************/
    /*                      Auto Mode: 30fps                                                                                          */
    /*                      Night Mode:15fps                                                                                          */
    /************************************************************************/
    if(bEnable)
    {
        if(HI542MIPI_MPEG4_encode_mode==KAL_TRUE)
        {
            HI542MIPI_MAX_EXPOSURE_LINES = (kal_uint16)((HI542MIPI_sensor_pclk/15)/(HI542MIPI_PV_PERIOD_PIXEL_NUMS+HI542MIPI_PV_dummy_pixels));
            HI542MIPI_write_cmos_sensor(0x0042, (HI542MIPI_MAX_EXPOSURE_LINES >> 8) & 0xFF);
            HI542MIPI_write_cmos_sensor(0x0043, HI542MIPI_MAX_EXPOSURE_LINES & 0xFF);
            HI542MIPI_CURRENT_FRAME_LINES = HI542MIPI_MAX_EXPOSURE_LINES;
            HI542MIPI_MAX_EXPOSURE_LINES = HI542MIPI_CURRENT_FRAME_LINES - HI542MIPI_SHUTTER_LINES_GAP;
        }
    }
    else// Fix video framerate 30 fps
    {
        if(HI542MIPI_MPEG4_encode_mode==KAL_TRUE)
        {
            HI542MIPI_MAX_EXPOSURE_LINES = (kal_uint16)((HI542MIPI_sensor_pclk/30)/(HI542MIPI_PV_PERIOD_PIXEL_NUMS+HI542MIPI_PV_dummy_pixels));
            if(HI542MIPI_pv_exposure_lines < (HI542MIPI_MAX_EXPOSURE_LINES - HI542MIPI_SHUTTER_LINES_GAP)) // for avoid the shutter > frame_lines,move the frame lines setting to shutter function
            {
                HI542MIPI_write_cmos_sensor(0x0042, (HI542MIPI_MAX_EXPOSURE_LINES >> 8) & 0xFF);
                HI542MIPI_write_cmos_sensor(0x0043, HI542MIPI_MAX_EXPOSURE_LINES & 0xFF);
                HI542MIPI_CURRENT_FRAME_LINES = HI542MIPI_MAX_EXPOSURE_LINES;
            }
            HI542MIPI_MAX_EXPOSURE_LINES = HI542MIPI_MAX_EXPOSURE_LINES - HI542MIPI_SHUTTER_LINES_GAP;
        }
    }
#endif
}/*	HI542_NightMode */



/*************************************************************************
* FUNCTION
*   HI542Close
*
* DESCRIPTION
*   This function is to turn off sensor module power.
*
* PARAMETERS
*   None
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 HI542MIPIClose(void)
{
    //  CISModulePowerOn(FALSE);

    //s_porting
    //  DRV_I2CClose(HI542hDrvI2C);
    //e_porting
    return ERROR_NONE;
}	/* HI542Close() */

UINT32 HI542MIPIGetDefaultFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId,MUINT32 *pframeRate)
{
	switch(scenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*pframeRate=300;
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			*pframeRate=150;
			break;
		case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW:
		case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
		case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE:
			*pframeRate=300;
			break;
		default:
			break;
		
	}
	return ERROR_NONE;
}


UINT32 HI542MIPISetMaxFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 frameRate)
{
	kal_int16 dummyLine=0;
	SENSORDB("S5K3H2YXMIPISetMaxFramerateByScenario scenarioId=%d,frameRate=%d",scenarioId,frameRate);
	switch(scenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			dummyLine=10*HI542MIPI_PV_pclk/frameRate/(HI542MIPI_PV_PERIOD_EXTRA_PIXEL_NUMS + HI542MIPI_PV_PERIOD_PIXEL_NUMS + 130)
				-(HI542MIPI_PV_PERIOD_LINE_NUMS+HI542MIPI_PV_PERIOD_EXTRA_LINE_NUMS);
			if(dummyLine<=0)
				dummyLine=0;
			HI542MIPI_SetDummy(0,dummyLine);
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			dummyLine=10*HI542MIPI_CAP_pclk/frameRate/(HI542MIPI_FULL_PERIOD_EXTRA_PIXEL_NUMS + HI542MIPI_FULL_PERIOD_PIXEL_NUMS + 130)
				-(HI542MIPI_FULL_PERIOD_LINE_NUMS+HI542MIPI_FULL_PERIOD_EXTRA_LINE_NUMS);
			if(dummyLine<=0)
				dummyLine=0;
			HI542MIPI_SetDummy(0,dummyLine);
			
			break;			
		case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW:
		case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
		case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE:
			break;
	}
	return ERROR_NONE;
}



void HI542MIPI_Set_Mirror_Flip(kal_uint8 image_mirror)
{
	kal_uint8 iTemp = 0;

	//MOD1[0x03],
	//Sensor mirror image control, bit[1]
	//Sensor flip image control, bit[0];
	iTemp = HI542MIPI_read_cmos_sensor(0x0011) & 0xFC;	//Clear the mirror and flip bits.
    switch (image_mirror)
	{
	    case IMAGE_NORMAL:
	        HI542MIPI_write_cmos_sensor(0x0011, iTemp | 0x00);	//Set normal
			
                //SET_FIRST_GRAB_COLOR(BAYER_Gr);
	        break;

		case IMAGE_V_MIRROR:
			HI542MIPI_write_cmos_sensor(0x0011, iTemp | 0x02);	//Set flip
			
                //SET_FIRST_GRAB_COLOR(BAYER_B);
			break;
			
		case IMAGE_H_MIRROR:
			HI542MIPI_write_cmos_sensor(0x0011, iTemp | 0x01);	//Set mirror
                //SET_FIRST_GRAB_COLOR(BAYER_Gr);
			break;
				
	    case IMAGE_HV_MIRROR:
	        HI542MIPI_write_cmos_sensor(0x0011, iTemp | 0x03);	//Set mirror and flip
                //SET_FIRST_GRAB_COLOR(BAYER_B);
	        break;
    }

}


/*************************************************************************
* FUNCTION
*   HI542Preview
*
* DESCRIPTION
*   This function start the sensor preview.
*
* PARAMETERS
*   *image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 HI542MIPIPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    kal_uint16 iStartX = 0, iStartY = 0;

	

    spin_lock(&hi542mipi_drv_lock);
    g_iHI542MIPI_Mode = HI542MIPI_MODE_PREVIEW;
	spin_unlock(&hi542mipi_drv_lock);

    {
        HI542MIPI_set_preview_setting();
    }
	
	spin_lock(&hi542mipi_drv_lock);
    if(sensor_config_data->SensorOperationMode==MSDK_SENSOR_OPERATION_MODE_VIDEO)		// MPEG4 Encode Mode
    {
    
        HI542MIPI_MPEG4_encode_mode = KAL_TRUE;
		Fix_framerate=1;
		printk("[HI542raw FUNCTION TEST]:HI542 video mode...................\n"); 
    }
    else
    {
        HI542MIPI_MPEG4_encode_mode = KAL_FALSE;
		Fix_framerate=0;
		printk("[HI542raw FUNCTION TEST]:HI542 Preview mode..................\n"); 
    }
	spin_unlock(&hi542mipi_drv_lock);

    iStartX += HI542MIPI_IMAGE_SENSOR_PV_STARTX;
    iStartY += HI542MIPI_IMAGE_SENSOR_PV_STARTY;

    //sensor_config_data->SensorImageMirror = IMAGE_HV_MIRROR; 
    
    //HI542_Set_Mirror_Flip(sensor_config_data->SensorImageMirror);
    
	spin_lock(&hi542mipi_drv_lock);
    HI542MIPI_dummy_pixels = 0;
    HI542MIPI_dummy_lines = 0;
    HI542MIPI_PV_dummy_pixels = HI542MIPI_dummy_pixels;
    HI542MIPI_PV_dummy_lines = HI542MIPI_dummy_lines;
	spin_unlock(&hi542mipi_drv_lock);

    //HI542MIPI_SetDummy(HI542MIPI_dummy_pixels, HI542MIPI_dummy_lines);
    //HI542_SetShutter(HI542_pv_exposure_lines);

    memcpy(&HI542MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

    image_window->GrabStartX= iStartX;
    image_window->GrabStartY= iStartY;
    image_window->ExposureWindowWidth= HI542MIPI_IMAGE_SENSOR_PV_WIDTH - 2*iStartX;
    image_window->ExposureWindowHeight= HI542MIPI_IMAGE_SENSOR_PV_HEIGHT - 2*iStartY;
    return ERROR_NONE;
}	/* HI542Preview() */



/*******************************************************************************
*
********************************************************************************/
UINT32 HI542MIPICapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    kal_uint32 shutter=HI542MIPI_pv_exposure_lines;
    kal_uint16 iStartX = 0, iStartY = 0;

    printk("[HI542raw FUNCTION TEST]:HI542Capture before calc...................shutter= 0x%x\n",shutter);  
	
	spin_lock(&hi542mipi_drv_lock);
    g_iHI542MIPI_Mode = HI542MIPI_MODE_CAPTURE;
	HI542MIPI_MPEG4_encode_mode = KAL_FALSE; 
	HI542MIPI_Auto_Flicker_mode = KAL_FALSE; 
	spin_unlock(&hi542mipi_drv_lock);

    if(sensor_config_data->EnableShutterTansfer==KAL_TRUE)
        shutter=sensor_config_data->CaptureShutter;

    

	    spin_lock(&hi542mipi_drv_lock);
        HI542MIPI_dummy_pixels= 0;
        HI542MIPI_dummy_lines = 0;   
		spin_unlock(&hi542mipi_drv_lock);

        HI542MIPI_set_5M15fps();
		
        
   
        //SVGA Internal CLK = 1/4 UXGA Internal CLK
        //shutter = 4* shutter;

        iStartX = 2* HI542MIPI_IMAGE_SENSOR_PV_STARTX;
        iStartY = 2* HI542MIPI_IMAGE_SENSOR_PV_STARTY;
		
        printk("[HI542raw FUNCTION TEST]:HI542Capture full size mode...................shutter= 0x%x\n",shutter);
		

    // config flashlight preview setting
    
    //sensor_config_data->SensorImageMirror = IMAGE_HV_MIRROR; 
     
    //HI542SetFlipMirror(sensor_config_data->SensorImageMirror); 
   


 
    HI542MIPI_SetDummy(HI542MIPI_dummy_pixels, HI542MIPI_dummy_lines);
    HI542MIPI_SetShutter(shutter);
    
    memcpy(&HI542MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

   mdelay(300);


   printk("[HI542raw FUNCTION TEST]:HI542Capture full size mode...................shutter= 0x%x\n",shutter);
   
   image_window->GrabStartX=iStartX;
   image_window->GrabStartY=iStartY;
   image_window->ExposureWindowWidth=HI542MIPI_IMAGE_SENSOR_FULL_WIDTH -2*iStartX;
   image_window->ExposureWindowHeight=HI542MIPI_IMAGE_SENSOR_FULL_HEIGHT-2*iStartY;

 /*  
   printk("[HI542raw]:HI542Open Read Sensor REG 0x0115 :0x%x\n", HI542MIPI_read_cmos_sensor(0x0115));
   printk("[HI542raw]:HI542Open Read Sensor REG 0x0116 :0x%x\n", HI542MIPI_read_cmos_sensor(0x0116));
   printk("[HI542raw]:HI542Open Read Sensor REG 0x0117 :0x%x\n", HI542MIPI_read_cmos_sensor(0x0117));
   printk("[HI542raw]:HI542Open Read Sensor REG 0x0118 :0x%x\n", HI542MIPI_read_cmos_sensor(0x0118));
   printk("[HI542raw]:HI542Open Read Sensor REG 0x0129 :0x%x\n", HI542MIPI_read_cmos_sensor(0x0129));
   */

    return ERROR_NONE;
}	/* HI542Capture() */

UINT32 HI542MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
    pSensorResolution->SensorFullWidth=IMAGE_SENSOR_FULL_WIDTH ;//- 36*HI542MIPI_IMAGE_SENSOR_PV_STARTX;
    pSensorResolution->SensorFullHeight=IMAGE_SENSOR_FULL_HEIGHT ;//- 26*HI542MIPI_IMAGE_SENSOR_PV_STARTY;
    pSensorResolution->SensorPreviewWidth=IMAGE_SENSOR_PV_WIDTH ;//- 2*HI542MIPI_IMAGE_SENSOR_PV_STARTX;
    pSensorResolution->SensorPreviewHeight=IMAGE_SENSOR_PV_HEIGHT;// - 2*HI542MIPI_IMAGE_SENSOR_PV_STARTY;
	pSensorResolution->SensorVideoWidth=IMAGE_SENSOR_PV_WIDTH ;//- 2*HI542MIPI_IMAGE_SENSOR_PV_STARTX;
	pSensorResolution->SensorVideoHeight=IMAGE_SENSOR_PV_HEIGHT;// - 2*HI542MIPI_IMAGE_SENSOR_PV_STARTY;
	pSensorResolution->Sensor3DFullWidth=IMAGE_SENSOR_PV_WIDTH ;//- 2*HI542MIPI_IMAGE_SENSOR_PV_STARTX;
	pSensorResolution->Sensor3DFullHeight=IMAGE_SENSOR_PV_HEIGHT;// - 2*HI542MIPI_IMAGE_SENSOR_PV_STARTY;	
	pSensorResolution->Sensor3DPreviewWidth=IMAGE_SENSOR_PV_WIDTH;// - 2*HI542MIPI_IMAGE_SENSOR_PV_STARTX;
	pSensorResolution->Sensor3DPreviewHeight=IMAGE_SENSOR_PV_HEIGHT;// - 2*HI542MIPI_IMAGE_SENSOR_PV_STARTY;		
	pSensorResolution->Sensor3DVideoWidth=IMAGE_SENSOR_PV_WIDTH;// - 2*HI542MIPI_IMAGE_SENSOR_PV_STARTX;
	pSensorResolution->Sensor3DVideoHeight=IMAGE_SENSOR_PV_HEIGHT;// - 2*HI542MIPI_IMAGE_SENSOR_PV_STARTY;

    return ERROR_NONE;
}   /* HI542GetResolution() */

UINT32 HI542MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
                                                MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    switch(ScenarioId)
    {
    	case MSDK_SCENARIO_ID_CAMERA_ZSD:
			 pSensorInfo->SensorPreviewResolutionX=IMAGE_SENSOR_FULL_WIDTH- 36*HI542MIPI_IMAGE_SENSOR_PV_STARTX;
			 pSensorInfo->SensorPreviewResolutionY=IMAGE_SENSOR_FULL_HEIGHT - 26*HI542MIPI_IMAGE_SENSOR_PV_STARTY;
			 pSensorInfo->SensorCameraPreviewFrameRate=15;
			 break;

		default:
  			 pSensorInfo->SensorPreviewResolutionX=IMAGE_SENSOR_PV_WIDTH - 2*HI542MIPI_IMAGE_SENSOR_PV_STARTX;
   			 pSensorInfo->SensorPreviewResolutionY=IMAGE_SENSOR_PV_HEIGHT - 2*HI542MIPI_IMAGE_SENSOR_PV_STARTY;
			 pSensorInfo->SensorCameraPreviewFrameRate=30;
    }
	
    pSensorInfo->SensorFullResolutionX=IMAGE_SENSOR_FULL_WIDTH - 36*HI542MIPI_IMAGE_SENSOR_PV_STARTX;
    pSensorInfo->SensorFullResolutionY=IMAGE_SENSOR_FULL_HEIGHT - 26*HI542MIPI_IMAGE_SENSOR_PV_STARTY;

    pSensorInfo->SensorVideoFrameRate=30;
    pSensorInfo->SensorStillCaptureFrameRate=10;
    pSensorInfo->SensorWebCamCaptureFrameRate=15;
    pSensorInfo->SensorResetActiveHigh=FALSE;
    pSensorInfo->SensorResetDelayCount=5;
    pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_RAW_B;
    pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW; /*??? */
    pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorInterruptDelayLines = 1;

    pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_MIPI;	
    //pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_PARALLEL;               CHENGXUE_ORIGINAL

      pSensorInfo->CaptureDelayFrame = 2; 
    pSensorInfo->PreviewDelayFrame = 5; 
    pSensorInfo->VideoDelayFrame = 5; 
    pSensorInfo->SensorMasterClockSwitch = 0; 
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_6MA;      
    pSensorInfo->AEShutDelayFrame = 0;		    /* The frame of setting shutter default 0 for TG int */
    pSensorInfo->AESensorGainDelayFrame = 1;   /* The frame of setting sensor gain */
    pSensorInfo->AEISPGainDelayFrame = 2;

    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockDividCount=	3;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = 1; 
            pSensorInfo->SensorGrabStartY = 1;        
	        pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;			   //add from here by zhiqiang
	        pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
		    pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
		    pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
	        pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
	        pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
	        pSensorInfo->SensorPacketECCOrder = 1;

            break;
			
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockDividCount=	3;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = 1; 
            pSensorInfo->SensorGrabStartY = 1;       
	        pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;			
	        pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	        pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
	        pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0; 
	        pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
	        pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x
            pSensorInfo->SensorPacketECCOrder = 1;

            break;
        default:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockDividCount=	3;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = 1; 
            pSensorInfo->SensorGrabStartY = 1;             
            break;
    }

    HI542MIPIPixelClockDivider=pSensorInfo->SensorPixelClockCount;
    memcpy(pSensorConfigData, &HI542MIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

    return ERROR_NONE;
}   /* HI542GetInfo() */


UINT32 HI542MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
 CurrentScenarioId=ScenarioId;
    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
            HI542MIPIPreview(pImageWindow, pSensorConfigData);
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
            HI542MIPICapture(pImageWindow, pSensorConfigData);
            break;
            //s_porting add
            //s_porting add
            //s_porting add
        default:
            return ERROR_INVALID_SCENARIO_ID;
            //e_porting add
            //e_porting add
            //e_porting add
    }
    return ERROR_NONE;
} /* HI542Control() */


UINT32 HI542MIPISetVideoMode(UINT16 u2FrameRate)
{
//	fixed mode setting

	
	spin_lock(&hi542mipi_drv_lock);
    HI542MIPI_MPEG4_encode_mode = KAL_TRUE; 
	spin_unlock(&hi542mipi_drv_lock);
	printk("[HI542raw FUNCTION TEST]:HI542SetVideoMode :u2FrameRate= Ox%x\n...................\n",u2FrameRate); 

	if(u2FrameRate >30 || u2FrameRate <5)
	    SENSORDB("Error frame rate seting");

    if (u2FrameRate == 30)
    { 
       if(CurrentScenarioId==MSDK_SCENARIO_ID_CAMERA_ZSD)
       	{
         	HI542MIPI_set_video_1080P();
            //HI542_MAX_EXPOSURE_LINES = (kal_uint16)((HI542_sensor_pclk/30)/(HI542_PV_PERIOD_PIXEL_NUMS + HI542_FULL_PERIOD_EXTRA_PIXEL_NUMS + HI542_PV_dummy_pixels));
			HI542MIPI_V_mode = HI542MIPI_Video_Auto;
			spin_lock(&hi542mipi_drv_lock);
			HI542MIPI_MAX_EXPOSURE_LINES =  HI542MIPI_FULL_PERIOD_EXTRA_LINE_NUMS; 
			spin_unlock(&hi542mipi_drv_lock);
			 
            if(HI542MIPI_pv_exposure_lines < (HI542MIPI_FULL_PERIOD_LINE_NUMS + HI542MIPI_FULL_PERIOD_EXTRA_LINE_NUMS - HI542MIPI_SHUTTER_LINES_GAP)) // for avoid the shutter > frame_lines,move the frame lines setting to shutter function
            {
                HI542MIPI_write_cmos_sensor(0x0042, (HI542MIPI_MAX_EXPOSURE_LINES >> 8) & 0xFF);
                HI542MIPI_write_cmos_sensor(0x0043, HI542MIPI_MAX_EXPOSURE_LINES & 0xFF);

				spin_lock(&hi542mipi_drv_lock);
				HI542MIPI_CURRENT_FRAME_LINES = HI542MIPI_MAX_EXPOSURE_LINES;
				spin_unlock(&hi542mipi_drv_lock);
            }
       	}
	   else
	   	{
    		HI542MIPI_set_video30fps();
            //HI542_MAX_EXPOSURE_LINES = (kal_uint16)((HI542_sensor_pclk/30)/(HI542_PV_PERIOD_PIXEL_NUMS + HI542_FULL_PERIOD_EXTRA_PIXEL_NUMS + HI542_PV_dummy_pixels));
			HI542MIPI_V_mode = HI542MIPI_Video_Auto;
			spin_lock(&hi542mipi_drv_lock);
			HI542MIPI_MAX_EXPOSURE_LINES =  HI542MIPI_PV_PERIOD_EXTRA_LINE_NUMS; 
			spin_unlock(&hi542mipi_drv_lock);
			 
            if(HI542MIPI_pv_exposure_lines < (HI542MIPI_PV_PERIOD_LINE_NUMS + HI542MIPI_PV_PERIOD_EXTRA_LINE_NUMS - HI542MIPI_SHUTTER_LINES_GAP)) // for avoid the shutter > frame_lines,move the frame lines setting to shutter function
            {
                HI542MIPI_write_cmos_sensor(0x0042, (HI542MIPI_MAX_EXPOSURE_LINES >> 8) & 0xFF);
                HI542MIPI_write_cmos_sensor(0x0043, HI542MIPI_MAX_EXPOSURE_LINES & 0xFF);

				spin_lock(&hi542mipi_drv_lock);
				HI542MIPI_CURRENT_FRAME_LINES = HI542MIPI_MAX_EXPOSURE_LINES;
				spin_unlock(&hi542mipi_drv_lock);
            }
            //HI542_MAX_EXPOSURE_LINES = HI542_MAX_EXPOSURE_LINES - HI542_SHUTTER_LINES_GAP;
	   	}
    }
    else if (u2FrameRate == 15)       
    {
         if(CurrentScenarioId==MSDK_SCENARIO_ID_CAMERA_ZSD)
         {
         	HI542MIPI_set_video_1080P();
            //HI542_MAX_EXPOSURE_LINES = (kal_uint16)((HI542_sensor_pclk/30)/(HI542_PV_PERIOD_PIXEL_NUMS + HI542_FULL_PERIOD_EXTRA_PIXEL_NUMS + HI542_PV_dummy_pixels));
			HI542MIPI_V_mode = HI542MIPI_Video_Auto;
			spin_lock(&hi542mipi_drv_lock);
			HI542MIPI_MAX_EXPOSURE_LINES =  HI542MIPI_FULL_PERIOD_EXTRA_LINE_NUMS; 
			spin_unlock(&hi542mipi_drv_lock);
			 
            if(HI542MIPI_pv_exposure_lines < (HI542MIPI_FULL_PERIOD_LINE_NUMS + HI542MIPI_FULL_PERIOD_EXTRA_LINE_NUMS - HI542MIPI_SHUTTER_LINES_GAP)) // for avoid the shutter > frame_lines,move the frame lines setting to shutter function
            {
                HI542MIPI_write_cmos_sensor(0x0042, (HI542MIPI_MAX_EXPOSURE_LINES >> 8) & 0xFF);
                HI542MIPI_write_cmos_sensor(0x0043, HI542MIPI_MAX_EXPOSURE_LINES & 0xFF);

				spin_lock(&hi542mipi_drv_lock);
				HI542MIPI_CURRENT_FRAME_LINES = HI542MIPI_MAX_EXPOSURE_LINES;
				spin_unlock(&hi542mipi_drv_lock);
            }
         }
		 else
		 {
    		HI542MIPI_set_video15fps();

			spin_lock(&hi542mipi_drv_lock);
     		HI542MIPI_V_mode = HI542MIPI_Video_Night;
			HI542MIPI_MAX_EXPOSURE_LINES = (HI542MIPI_PV_PERIOD_LINE_NUMS + HI542MIPI_PV_PERIOD_EXTRA_LINE_NUMS) * 2 - HI542MIPI_PV_PERIOD_LINE_NUMS;  

			spin_unlock(&hi542mipi_drv_lock);

			HI542MIPI_write_cmos_sensor(0x0042, (HI542MIPI_MAX_EXPOSURE_LINES >> 8) & 0xFF);
            HI542MIPI_write_cmos_sensor(0x0043, HI542MIPI_MAX_EXPOSURE_LINES & 0xFF);

			spin_lock(&hi542mipi_drv_lock);
			HI542MIPI_CURRENT_FRAME_LINES = HI542MIPI_MAX_EXPOSURE_LINES;
            HI542MIPI_MAX_EXPOSURE_LINES = HI542MIPI_CURRENT_FRAME_LINES - HI542MIPI_SHUTTER_LINES_GAP;
            spin_unlock(&hi542mipi_drv_lock);				
		 }
    }
    else if(u2FrameRate == 0)
	{

		    spin_lock(&hi542mipi_drv_lock);
		    HI542MIPI_MPEG4_encode_mode = KAL_FALSE; 
		    spin_unlock(&hi542mipi_drv_lock);
			printk("disable video mode %d\n",u2FrameRate);

    }else
    {
        printk("Wrong frame rate setting \n");
    }
    return TRUE;
}


UINT32 HI542MIPISetAutoFlickerMode(kal_bool bEnable, UINT16 u2FrameRate)
{

    printk("[HI542raw FUNCTION TEST]:HI542SetAutoFlickerMode :bEnable = %x,u2FrameRate= Ox%x\n...................\n",bEnable,u2FrameRate); 
 
    if(bEnable)
    {
     spin_lock(&hi542mipi_drv_lock);
   	 HI542MIPI_Auto_Flicker_mode = KAL_TRUE; 
	 spin_unlock(&hi542mipi_drv_lock);

   	 //Change frame rate 29.5fps to 29.8fps to do Auto flick
  	 if(HI542MIPI_MPEG4_encode_mode == KAL_TRUE)
  	 {
	    switch(HI542MIPI_V_mode)
			{
				case HI542MIPI_Video_Auto:
	 				HI542MIPISetMaxFrameRate(296);
					break;
				case HI542MIPI_Video_Night:
	 				HI542MIPISetMaxFrameRate(146);		
					break;
				default:
					break;
			}
  	 }	
    }
	else
    {//Cancel Auto flick
     spin_lock(&hi542mipi_drv_lock);
   	 HI542MIPI_Auto_Flicker_mode = KAL_FALSE;
	 spin_unlock(&hi542mipi_drv_lock);
   	 if(HI542MIPI_MPEG4_encode_mode == KAL_TRUE)
   	 	{
		 switch(HI542MIPI_V_mode)
			 {
				 case HI542MIPI_Video_Auto:
					 HI542MIPISetMaxFrameRate(300);
					 break;
				 case HI542MIPI_Video_Night:
					 HI542MIPISetMaxFrameRate(150); 
					break;
				default:
					break;					 
			 }	 
   	 	}

	  printk("Disable Auto flicker\n"); 
    }

    return TRUE;
}



UINT32 HI542MIPISetTestPatternMode(kal_bool bEnable)
{
    SENSORDB("[HI542SetTestPatternMode] Test pattern enable:%d\n", bEnable);
    
    if(bEnable) {   // enable color bar   
	 /* 0x80,0x00: color bar 0x85,0x12: color square 0x85,0x1A: B/W square */
	 HI542MIPI_write_cmos_sensor(0x503D, 0x80);//0050
	 HI542MIPI_write_cmos_sensor(0x503E, 0x00);    
    } else {
        HI542MIPI_write_cmos_sensor(0x503D, 0x00);  // disable color bar test pattern
    }
    return TRUE;
}

UINT32 HI542MIPISetSoftwarePWDNMode(kal_bool bEnable)
{
    printk("[HI542raw FUNCTION TEST]:HI542SetSoftwarePWDNMode :bEnable = %x\n...................\n",bEnable); 
    
    if(bEnable) {   // enable software power down mode   
	 HI542MIPI_write_cmos_sensor(0x3008, 0x40);
    } else {
        HI542MIPI_write_cmos_sensor(0x3008, 0x00);  
    }
    return TRUE;
}

UINT32 HI542MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
                                                                UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{    
    UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
    UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
    UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
    UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
    UINT32 SensorRegNumber;
    UINT32 i;
    PNVRAM_SENSOR_DATA_STRUCT pSensorDefaultData=(PNVRAM_SENSOR_DATA_STRUCT) pFeaturePara;
    MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
    MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;
    MSDK_SENSOR_GROUP_INFO_STRUCT *pSensorGroupInfo=(MSDK_SENSOR_GROUP_INFO_STRUCT *) pFeaturePara;
    MSDK_SENSOR_ITEM_INFO_STRUCT *pSensorItemInfo=(MSDK_SENSOR_ITEM_INFO_STRUCT *) pFeaturePara;
    MSDK_SENSOR_ENG_INFO_STRUCT	*pSensorEngInfo=(MSDK_SENSOR_ENG_INFO_STRUCT *) pFeaturePara;

    switch (FeatureId)
    {
        case SENSOR_FEATURE_GET_RESOLUTION:
            *pFeatureReturnPara16++=IMAGE_SENSOR_FULL_WIDTH;
            *pFeatureReturnPara16=IMAGE_SENSOR_FULL_HEIGHT;
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_PERIOD:

			switch(CurrentScenarioId)
			 {
			 	case MSDK_SCENARIO_ID_CAMERA_ZSD:
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
					*pFeatureReturnPara16++=HI542MIPI_FULL_PERIOD_EXTRA_PIXEL_NUMS + HI542MIPI_FULL_PERIOD_PIXEL_NUMS + 130;//HI542_PV_PERIOD_PIXEL_NUMS+HI542_dummy_pixels;
					*pFeatureReturnPara16=HI542MIPI_FULL_PERIOD_LINE_NUMS+HI542MIPI_FULL_PERIOD_EXTRA_LINE_NUMS;
					*pFeatureParaLen=4;
					break;
				default:
					*pFeatureReturnPara16++=HI542MIPI_PV_PERIOD_EXTRA_PIXEL_NUMS + HI542MIPI_PV_PERIOD_PIXEL_NUMS + 130;//HI542_PV_PERIOD_PIXEL_NUMS+HI542_dummy_pixels;
					*pFeatureReturnPara16=HI542MIPI_PV_PERIOD_LINE_NUMS+HI542MIPI_PV_PERIOD_EXTRA_LINE_NUMS;
					*pFeatureParaLen=4;
					break;
					
			 }
			
            break;
        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
            *pFeatureReturnPara32 = 84000000; //19500000;
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_SET_ESHUTTER:
            HI542MIPI_SetShutter(*pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_NIGHTMODE:
            HI542MIPI_NightMode((BOOL) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_GAIN:
            HI542MIPI_SetGain((UINT16) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_FLASHLIGHT:
            break;
        case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
			
			spin_lock(&hi542mipi_drv_lock);
            HI542MIPI_isp_master_clock=*pFeatureData32;
			spin_unlock(&hi542mipi_drv_lock);
            break;
        case SENSOR_FEATURE_SET_REGISTER:
            HI542MIPI_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
            break;
        case SENSOR_FEATURE_GET_REGISTER:
            pSensorRegData->RegData = HI542MIPI_read_cmos_sensor(pSensorRegData->RegAddr);
            break;
        case SENSOR_FEATURE_SET_CCT_REGISTER:
			printk("[HI542raw FUNCTION TEST]:HI542FeatureControl : SENSOR_FEATURE_SET_CCT_REGISTER .........................");
            SensorRegNumber=FACTORY_END_ADDR;
            for (i=0;i<SensorRegNumber;i++)
            {
                spin_lock(&hi542mipi_drv_lock);
                HI542MIPISensorCCT[i].Addr=*pFeatureData32++;
                HI542MIPISensorCCT[i].Para=*pFeatureData32++;
			    spin_unlock(&hi542mipi_drv_lock); 
            }
            break;
        case SENSOR_FEATURE_GET_CCT_REGISTER:
			printk("[HI542raw FUNCTION TEST]:HI542FeatureControl : SENSOR_FEATURE_GET_CCT_REGISTER .........................");
            SensorRegNumber=FACTORY_END_ADDR;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=HI542MIPISensorCCT[i].Addr;
                *pFeatureData32++=HI542MIPISensorCCT[i].Para;
            }
            break;
        case SENSOR_FEATURE_SET_ENG_REGISTER:
			
			printk("[HI542raw FUNCTION TEST]:HI542FeatureControl : SENSOR_FEATURE_SET_ENG_REGISTER .........................");
            SensorRegNumber=ENGINEER_END;
            for (i=0;i<SensorRegNumber;i++)
            {
                spin_lock(&hi542mipi_drv_lock);
                HI542MIPISensorReg[i].Addr=*pFeatureData32++;
                HI542MIPISensorReg[i].Para=*pFeatureData32++;
				spin_unlock(&hi542mipi_drv_lock);
            }
            break;
        case SENSOR_FEATURE_GET_ENG_REGISTER:
			
			printk("[HI542raw FUNCTION TEST]:HI542FeatureControl : SENSOR_FEATURE_GET_ENG_REGISTER .........................");
            SensorRegNumber=ENGINEER_END;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=HI542MIPISensorReg[i].Addr;
                *pFeatureData32++=HI542MIPISensorReg[i].Para;
            }
            break;
        case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
            if (*pFeatureParaLen>=sizeof(NVRAM_SENSOR_DATA_STRUCT))
            {
                pSensorDefaultData->Version=NVRAM_CAMERA_SENSOR_FILE_VERSION;
                pSensorDefaultData->SensorId=HI542MIPI_SENSOR_ID;
                memcpy(pSensorDefaultData->SensorEngReg, HI542MIPISensorReg, sizeof(SENSOR_REG_STRUCT)*ENGINEER_END);
                memcpy(pSensorDefaultData->SensorCCTReg, HI542MIPISensorCCT, sizeof(SENSOR_REG_STRUCT)*FACTORY_END_ADDR);
            }
            else
                return FALSE;
            *pFeatureParaLen=sizeof(NVRAM_SENSOR_DATA_STRUCT);
            break;
        case SENSOR_FEATURE_GET_CONFIG_PARA:
            memcpy(pSensorConfigData, &HI542MIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
            *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
            break;
        case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
            HI542MIPI_camera_para_to_sensor();
            break;

        case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
            HI542MIPI_sensor_to_camera_para();
            break;
        case SENSOR_FEATURE_GET_GROUP_COUNT:
            *pFeatureReturnPara32++=HI542MIPI_get_sensor_group_count();
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_GROUP_INFO:
            HI542MIPI_get_sensor_group_info(pSensorGroupInfo->GroupIdx, pSensorGroupInfo->GroupNamePtr, &pSensorGroupInfo->ItemCount);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_GROUP_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_ITEM_INFO:
            HI542MIPI_get_sensor_item_info(pSensorItemInfo->GroupIdx,pSensorItemInfo->ItemIdx, pSensorItemInfo);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_SET_ITEM_INFO:
            HI542MIPI_set_sensor_item_info(pSensorItemInfo->GroupIdx, pSensorItemInfo->ItemIdx, pSensorItemInfo->ItemValue);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_GET_ENG_INFO:
            pSensorEngInfo->SensorId = 135;
            pSensorEngInfo->SensorType = CMOS_SENSOR;
            pSensorEngInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_RAW_B;
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ENG_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
            // get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
            // if EEPROM does not exist in camera module.
            *pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
            *pFeatureParaLen=4;
            break;

        case SENSOR_FEATURE_INITIALIZE_AF:
        case SENSOR_FEATURE_CONSTANT_AF:
        case SENSOR_FEATURE_MOVE_FOCUS_LENS:
        case SENSOR_FEATURE_GET_AF_STATUS:
        case SENSOR_FEATURE_GET_AF_INF:
        case SENSOR_FEATURE_GET_AF_MACRO: 
            break;                
        case SENSOR_FEATURE_SET_VIDEO_MODE:
            HI542MIPISetVideoMode(*pFeatureData16);
            break;
        case SENSOR_FEATURE_CHECK_SENSOR_ID:
            HI542MIPIGetSensorID(pFeatureReturnPara32); 
            break; 
		
		case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
            HI542MIPISetAutoFlickerMode((BOOL)*pFeatureData16, *(pFeatureData16+1));            
	        break;
		
		case SENSOR_FEATURE_SET_TEST_PATTERN:
            HI542MIPISetTestPatternMode((BOOL)*pFeatureData16);        	
            break;
        case SENSOR_FEATURE_SET_SOFTWARE_PWDN:
            HI542MIPISetSoftwarePWDNMode((BOOL)*pFeatureData16);        	        	
            break;
		case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
			HI542MIPISetMaxFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32,*(pFeatureData32+1));
			break;
		case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
			HI542MIPIGetDefaultFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32,(MUINT32 *)(*(pFeatureData32+1)));
			break;
        default:
            break;
    }
    return ERROR_NONE;
}	/* HI542FeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncHI542MIPI=
{
    HI542MIPIOpen,
    HI542MIPIGetInfo,
    HI542MIPIGetResolution,
    HI542MIPIFeatureControl,
    HI542MIPIControl,
    HI542MIPIClose
};

UINT32 HI542_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&SensorFuncHI542MIPI;

    return ERROR_NONE;
}   /* SensorInit() */

