/*******************************************************************************************/
// schedule
//   getsensorid ok
//   open ok
//   setting(pv,cap,video) ok

/*******************************************************************************************/

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/xlog.h>
#include <asm/system.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "hi544mipiraw_Sensor.h"
#include "hi544mipiraw_Camera_Sensor_para.h"
#include "hi544mipiraw_CameraCustomized.h"
static DEFINE_SPINLOCK(HI544mipiraw_drv_lock);

#define HI544_TEST_PATTERN_CHECKSUM (0xfc8d8b88)//do rotate will change this value

#define HI544_DEBUG
//#define HI544_DEBUG_SOFIA

#ifdef HI544_DEBUG
	#define HI544DB(fmt, arg...) xlog_printk(ANDROID_LOG_DEBUG, "[HI544Raw] ",  fmt, ##arg)
#else
	#define HI544DB(fmt, arg...)
#endif

#ifdef HI544_DEBUG_SOFIA
	#define HI544DBSOFIA(fmt, arg...) xlog_printk(ANDROID_LOG_DEBUG, "[HI544Raw] ",  fmt, ##arg)
#else
	#define HI544DBSOFIA(fmt, arg...)
#endif

#define mDELAY(ms)  mdelay(ms)

kal_uint32 HI544_FeatureControl_PERIOD_PixelNum=HI544_PV_PERIOD_PIXEL_NUMS;
kal_uint32 HI544_FeatureControl_PERIOD_LineNum=HI544_PV_PERIOD_LINE_NUMS;

UINT16 VIDEO_MODE_TARGET_FPS = 30;
static BOOL ReEnteyCamera = KAL_FALSE;


MSDK_SENSOR_CONFIG_STRUCT HI544SensorConfigData;

kal_uint32 HI544_FAC_SENSOR_REG;

MSDK_SCENARIO_ID_ENUM HI544CurrentScenarioId = MSDK_SCENARIO_ID_CAMERA_PREVIEW;

/* FIXME: old factors and DIDNOT use now. s*/
SENSOR_REG_STRUCT HI544SensorCCT[]=CAMERA_SENSOR_CCT_DEFAULT_VALUE;
SENSOR_REG_STRUCT HI544SensorReg[ENGINEER_END]=CAMERA_SENSOR_REG_DEFAULT_VALUE;
/* FIXME: old factors and DIDNOT use now. e*/

static HI544_PARA_STRUCT HI544;

extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);

//#define HI544_write_cmos_sensor(addr, para) iWriteReg((u16) addr , (u32) para , 1, HI544MIPI_WRITE_ID)
// modify by yfx
//extern int iMultiWriteReg(u8 *pData, u16 lens);
#if 1
#define HI544_write_cmos_sensor(addr, para) iWriteReg((u16) addr , (u32) para , 2, HI544MIPI_WRITE_ID)
#else
void HI544_write_cmos_sensor(u16 addr, u16 para) 
{
	u8 senddata[4];
	senddata[0] = (addr >> 8) & 0xff;
	senddata[1] = addr & 0xff;
	senddata[2] = (para >> 8) & 0xff;
	senddata[3] = para & 0xff;
	iMultiWriteReg(senddata, 4)
}
#endif
// end

kal_uint16 HI544_read_cmos_sensor(kal_uint32 addr)
{
kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,HI544MIPI_WRITE_ID);
    return get_byte;
}

#define Sleep(ms) mdelay(ms)

void HI544_write_shutter(kal_uint32 shutter)
{
	kal_uint32 min_framelength = HI544_PV_PERIOD_PIXEL_NUMS, max_shutter=0;
	kal_uint32 extra_lines = 0;
	kal_uint32 line_length = 0;
	kal_uint32 frame_length = 0;
	unsigned long flags;

	HI544DBSOFIA("!!shutter=%d!!!!!\n", shutter);

	if(HI544.HI544AutoFlickerMode == KAL_TRUE)
	{

		if ( SENSOR_MODE_PREVIEW == HI544.sensorMode )  //(g_iHI544_Mode == HI544_MODE_PREVIEW)	//SXGA size output
		{
			line_length = HI544_PV_PERIOD_PIXEL_NUMS + HI544.DummyPixels;
			max_shutter = HI544_PV_PERIOD_LINE_NUMS + HI544.DummyLines ;
		}
		else if( SENSOR_MODE_VIDEO == HI544.sensorMode ) //add for video_6M setting
		{
			line_length = HI544_VIDEO_PERIOD_PIXEL_NUMS + HI544.DummyPixels;
			max_shutter = HI544_VIDEO_PERIOD_LINE_NUMS + HI544.DummyLines ;
		}
		else
		{
			line_length = HI544_FULL_PERIOD_PIXEL_NUMS + HI544.DummyPixels;
			max_shutter = HI544_FULL_PERIOD_LINE_NUMS + HI544.DummyLines ;
		}

		switch(HI544CurrentScenarioId)
		{
        	case MSDK_SCENARIO_ID_CAMERA_ZSD:
			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
				HI544DBSOFIA("AutoFlickerMode!!! MSDK_SCENARIO_ID_CAMERA_ZSD  0!!\n");
				min_framelength = max_shutter;// capture max_fps 24,no need calculate
				break;
			case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
				if(VIDEO_MODE_TARGET_FPS==30)
				{
					min_framelength = (HI544.videoPclk*10000) /(HI544_VIDEO_PERIOD_PIXEL_NUMS + HI544.DummyPixels)/304*10 ;
				}
				else if(VIDEO_MODE_TARGET_FPS==15)
				{
					min_framelength = (HI544.videoPclk*10000) /(HI544_VIDEO_PERIOD_PIXEL_NUMS + HI544.DummyPixels)/148*10 ;
				}
				else
				{
					min_framelength = max_shutter;
				}
				break;
			default:
				min_framelength = (HI544.pvPclk*10000) /(HI544_PV_PERIOD_PIXEL_NUMS + HI544.DummyPixels)/296*10 ;
    			break;
		}

		HI544DBSOFIA("AutoFlickerMode!!! min_framelength for AutoFlickerMode = %d (0x%x)\n",min_framelength,min_framelength);
		HI544DBSOFIA("max framerate(10 base) autofilker = %d\n",(HI544.pvPclk*10000)*10 /line_length/min_framelength);

		if (shutter < 4)
			shutter = 4;

		if (shutter > (max_shutter-4) )
			extra_lines = shutter - max_shutter + 4;
		else
			extra_lines = 0;

		if ( SENSOR_MODE_PREVIEW == HI544.sensorMode )	//SXGA size output
		{
			frame_length = HI544_PV_PERIOD_LINE_NUMS+ HI544.DummyLines + extra_lines ;
		}
		else if(SENSOR_MODE_VIDEO == HI544.sensorMode)
		{
			frame_length = HI544_VIDEO_PERIOD_LINE_NUMS+ HI544.DummyLines + extra_lines ;
		}
		else				//QSXGA size output
		{
			frame_length = HI544_FULL_PERIOD_LINE_NUMS + HI544.DummyLines + extra_lines ;
		}
		HI544DBSOFIA("frame_length 0= %d\n",frame_length);

		if (frame_length < min_framelength)
		{
			//shutter = min_framelength - 4;

			switch(HI544CurrentScenarioId)
			{
        	case MSDK_SCENARIO_ID_CAMERA_ZSD:
			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
				extra_lines = min_framelength- (HI544_FULL_PERIOD_LINE_NUMS+ HI544.DummyLines);
				break;
			case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
				extra_lines = min_framelength- (HI544_VIDEO_PERIOD_LINE_NUMS+ HI544.DummyLines);
				break;
			default:
				extra_lines = min_framelength- (HI544_PV_PERIOD_LINE_NUMS+ HI544.DummyLines);
    			break;
			}
			frame_length = min_framelength;
		}

		HI544DBSOFIA("frame_length 1= %d\n",frame_length);

		ASSERT(line_length < HI544_MAX_LINE_LENGTH);		//0xCCCC
		ASSERT(frame_length < HI544_MAX_FRAME_LENGTH); 	//0xFFFF
		
		spin_lock_irqsave(&HI544mipiraw_drv_lock,flags);
		HI544.maxExposureLines = frame_length - 4;
		HI544_FeatureControl_PERIOD_PixelNum = line_length;
		HI544_FeatureControl_PERIOD_LineNum = frame_length;
		spin_unlock_irqrestore(&HI544mipiraw_drv_lock,flags);


		HI544_write_cmos_sensor(0x0046, 0x0100);
		//Set total frame length
		HI544_write_cmos_sensor(0x0006, frame_length);
//		HI544_write_cmos_sensor(0x0007, frame_length & 0xFF);

		//Set shutter (Coarse integration time, uint: lines.)
		HI544_write_cmos_sensor(0x0004, shutter);
		//HI544_write_cmos_sensor(0x0005, (shutter) & 0xFF);
		
		HI544_write_cmos_sensor(0x0046, 0x0000);

		HI544DBSOFIA("frame_length 2= %d\n",frame_length);
		HI544DB("framerate(10 base) = %d\n",(HI544.pvPclk*10000)*10 /line_length/frame_length);

		HI544DB("shutter=%d, extra_lines=%d, line_length=%d, frame_length=%d\n", shutter, extra_lines, line_length, frame_length);

	}
	else
	{
		if ( SENSOR_MODE_PREVIEW == HI544.sensorMode )  //(g_iHI544_Mode == HI544_MODE_PREVIEW)	//SXGA size output
		{
			max_shutter = HI544_PV_PERIOD_LINE_NUMS + HI544.DummyLines ;
		}
		else if( SENSOR_MODE_VIDEO == HI544.sensorMode ) //add for video_6M setting
		{
			max_shutter = HI544_VIDEO_PERIOD_LINE_NUMS + HI544.DummyLines ;
		}
		else
		{
			max_shutter = HI544_FULL_PERIOD_LINE_NUMS + HI544.DummyLines ;
		}

		if (shutter < 4)
			shutter = 4;

		if (shutter > (max_shutter-4) )
			extra_lines = shutter - max_shutter + 4;
		else
			extra_lines = 0;

		if ( SENSOR_MODE_PREVIEW == HI544.sensorMode )	//SXGA size output
		{
			line_length = HI544_PV_PERIOD_PIXEL_NUMS + HI544.DummyPixels;
			frame_length = HI544_PV_PERIOD_LINE_NUMS+ HI544.DummyLines + extra_lines ;
		}
		else if( SENSOR_MODE_VIDEO == HI544.sensorMode )
		{
			line_length = HI544_VIDEO_PERIOD_PIXEL_NUMS + HI544.DummyPixels;
			frame_length = HI544_VIDEO_PERIOD_LINE_NUMS + HI544.DummyLines + extra_lines ;
		}
		else				//QSXGA size output
		{
			line_length = HI544_FULL_PERIOD_PIXEL_NUMS + HI544.DummyPixels;
			frame_length = HI544_FULL_PERIOD_LINE_NUMS + HI544.DummyLines + extra_lines ;
		}

		ASSERT(line_length < HI544_MAX_LINE_LENGTH);		//0xCCCC
		ASSERT(frame_length < HI544_MAX_FRAME_LENGTH); 	//0xFFFF

		//Set total frame length
		HI544_write_cmos_sensor(0x0046, 0x0100);		
		HI544_write_cmos_sensor(0x0006, frame_length);
		//HI544_write_cmos_sensor(0x0007, frame_length & 0xFF);
		HI544_write_cmos_sensor(0x0046, 0x0000);		

		spin_lock_irqsave(&HI544mipiraw_drv_lock,flags);
		HI544.maxExposureLines = frame_length -4;
		HI544_FeatureControl_PERIOD_PixelNum = line_length;
		HI544_FeatureControl_PERIOD_LineNum = frame_length;
		spin_unlock_irqrestore(&HI544mipiraw_drv_lock,flags);


		//Set shutter (Coarse integration time, uint: lines.)
		HI544_write_cmos_sensor(0x0046, 0x0100);		
		HI544_write_cmos_sensor(0x0004, shutter);
		//HI544_write_cmos_sensor(0x0005, (shutter) & 0xFF);
		HI544_write_cmos_sensor(0x0046, 0x0000);		
		
		//HI544DB("framerate(10 base) = %d\n",(HI544.pvPclk*10000)*10 /line_length/frame_length);
		HI544DB("shutter=%d, extra_lines=%d, line_length=%d, frame_length=%d\n", shutter, extra_lines, line_length, frame_length);
	}

}   /* write_HI544_shutter */

/*******************************************************************************
*
********************************************************************************/
static kal_uint16 HI544Reg2Gain(const kal_uint8 iReg)
{
	kal_uint16 iGain = 64;

	iGain = 4 * iReg + 64;

	return iGain;
}

/*******************************************************************************
*
********************************************************************************/
static kal_uint16 HI544Gain2Reg(const kal_uint16 Gain)
{
	kal_uint16 iReg;
	kal_uint8 iBaseGain = 64;
	
	iReg = Gain / 4 - 16;
    return iReg;//HI544. sensorGlobalGain

}


void write_HI544_gain(kal_uint8 gain)
{
#if 1
	HI544_write_cmos_sensor(0x003a, (gain << 8));
#endif
	return;
}

/*************************************************************************
* FUNCTION
*    HI544_SetGain
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
void HI544_SetGain(UINT16 iGain)
{
	unsigned long flags;
    kal_uint16 HI544GlobalGain=0;
	kal_uint16 DigitalGain = 0;

    
	// AG = (regvalue / 16) + 1
	if(iGain > 1024)  
	{
		iGain = 1024;
	}
	if(iGain < 64)  // gain的reg最大值是255
	{
		iGain = 64;
	}
		
	HI544GlobalGain = HI544Gain2Reg(iGain); 
	spin_lock(&HI544mipiraw_drv_lock);
	HI544.realGain = iGain;
	HI544.sensorGlobalGain =HI544GlobalGain;
	spin_unlock(&HI544mipiraw_drv_lock);

	HI544DB("[HI544_SetGain]HI544.sensorGlobalGain=0x%x,HI544.realGain=%d\n",HI544.sensorGlobalGain,HI544.realGain);

	HI544_write_cmos_sensor(0x0046, 0x0100);
	write_HI544_gain(HI544.sensorGlobalGain);	
	HI544_write_cmos_sensor(0x0046, 0x0000);

	
	return;	
}   /*  HI544_SetGain_SetGain  */


/*************************************************************************
* FUNCTION
*    read_HI544_gain
*
* DESCRIPTION
*    This function is to set global gain to sensor.
*
* PARAMETERS
*    None
*
* RETURNS
*    gain : sensor global gain
*
* GLOBALS AFFECTED
*
*************************************************************************/
kal_uint16 read_HI544_gain(void)
{
	kal_uint16 read_gain_anolog=0;
	kal_uint16 HI544RealGain_anolog =0;
	kal_uint16 HI544RealGain =0;

	read_gain_anolog=HI544_read_cmos_sensor(0x003a);
	
	HI544RealGain_anolog = HI544Reg2Gain(read_gain_anolog);
	

	spin_lock(&HI544mipiraw_drv_lock);
	HI544.sensorGlobalGain = read_gain_anolog;
	HI544.realGain = HI544RealGain;
	spin_unlock(&HI544mipiraw_drv_lock);
	HI544DB("[read_HI544_gain]HI544RealGain_anolog=0x%x\n",HI544RealGain_anolog);

	return HI544.sensorGlobalGain;
}  /* read_HI544_gain */


void HI544_camera_para_to_sensor(void)
{
    kal_uint32    i;
    for(i=0; 0xFFFFFFFF!=HI544SensorReg[i].Addr; i++)
    {
        HI544_write_cmos_sensor(HI544SensorReg[i].Addr, HI544SensorReg[i].Para);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=HI544SensorReg[i].Addr; i++)
    {
        HI544_write_cmos_sensor(HI544SensorReg[i].Addr, HI544SensorReg[i].Para);
    }
    for(i=FACTORY_START_ADDR; i<FACTORY_END_ADDR; i++)
    {
        HI544_write_cmos_sensor(HI544SensorCCT[i].Addr, HI544SensorCCT[i].Para);
    }
}


/*************************************************************************
* FUNCTION
*    HI544_sensor_to_camera_para
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
void HI544_sensor_to_camera_para(void)
{
    kal_uint32    i, temp_data;
    for(i=0; 0xFFFFFFFF!=HI544SensorReg[i].Addr; i++)
    {
         temp_data = HI544_read_cmos_sensor(HI544SensorReg[i].Addr);
		 spin_lock(&HI544mipiraw_drv_lock);
		 HI544SensorReg[i].Para =temp_data;
		 spin_unlock(&HI544mipiraw_drv_lock);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=HI544SensorReg[i].Addr; i++)
    {
        temp_data = HI544_read_cmos_sensor(HI544SensorReg[i].Addr);
		spin_lock(&HI544mipiraw_drv_lock);
		HI544SensorReg[i].Para = temp_data;
		spin_unlock(&HI544mipiraw_drv_lock);
    }
}

/*************************************************************************
* FUNCTION
*    HI544_get_sensor_group_count
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
kal_int32  HI544_get_sensor_group_count(void)
{
    return GROUP_TOTAL_NUMS;
}

void HI544_get_sensor_group_info(kal_uint16 group_idx, kal_int8* group_name_ptr, kal_int32* item_count_ptr)
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

void HI544_get_sensor_item_info(kal_uint16 group_idx,kal_uint16 item_idx, MSDK_SENSOR_ITEM_INFO_STRUCT* info_ptr)
{
    kal_int16 temp_reg=0;
    kal_uint16 temp_gain=0, temp_addr=0, temp_para=0;

    switch (group_idx)
    {
        case PRE_GAIN:
           switch (item_idx)
          {
              case 0:
                sprintf((char *)info_ptr->ItemNamePtr,"Pregain-R");
                  temp_addr = PRE_GAIN_R_INDEX;
              break;
              case 1:
                sprintf((char *)info_ptr->ItemNamePtr,"Pregain-Gr");
                  temp_addr = PRE_GAIN_Gr_INDEX;
              break;
              case 2:
                sprintf((char *)info_ptr->ItemNamePtr,"Pregain-Gb");
                  temp_addr = PRE_GAIN_Gb_INDEX;
              break;
              case 3:
                sprintf((char *)info_ptr->ItemNamePtr,"Pregain-B");
                  temp_addr = PRE_GAIN_B_INDEX;
              break;
              case 4:
                 sprintf((char *)info_ptr->ItemNamePtr,"SENSOR_BASEGAIN");
                 temp_addr = SENSOR_BASEGAIN;
              break;
              default:
                 ASSERT(0);
          }

            temp_para= HI544SensorCCT[temp_addr].Para;
			//temp_gain= (temp_para/HI544.sensorBaseGain) * 1000;

            info_ptr->ItemValue=temp_gain;
            info_ptr->IsTrueFalse=KAL_FALSE;
            info_ptr->IsReadOnly=KAL_FALSE;
            info_ptr->IsNeedRestart=KAL_FALSE;
            info_ptr->Min= HI544_MIN_ANALOG_GAIN * 1000;
            info_ptr->Max= HI544_MAX_ANALOG_GAIN * 1000;
            break;
        case CMMCLK_CURRENT:
            switch (item_idx)
            {
                case 0:
                    sprintf((char *)info_ptr->ItemNamePtr,"Drv Cur[2,4,6,8]mA");

                    //temp_reg=MT9P017SensorReg[CMMCLK_CURRENT_INDEX].Para;
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
                    info_ptr->ItemValue=    111;  //MT9P017_MAX_EXPOSURE_LINES;
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



kal_bool HI544_set_sensor_item_info(kal_uint16 group_idx, kal_uint16 item_idx, kal_int32 ItemValue)
{
//   kal_int16 temp_reg;
   kal_uint16  temp_gain=0,temp_addr=0, temp_para=0;

   switch (group_idx)
    {
        case PRE_GAIN:
            switch (item_idx)
            {
              case 0:
                temp_addr = PRE_GAIN_R_INDEX;
              break;
              case 1:
                temp_addr = PRE_GAIN_Gr_INDEX;
              break;
              case 2:
                temp_addr = PRE_GAIN_Gb_INDEX;
              break;
              case 3:
                temp_addr = PRE_GAIN_B_INDEX;
              break;
              case 4:
                temp_addr = SENSOR_BASEGAIN;
              break;
              default:
                 ASSERT(0);
          }

		 temp_gain=((ItemValue*BASEGAIN+500)/1000);			//+500:get closed integer value

		  if(temp_gain>=1*BASEGAIN && temp_gain<=16*BASEGAIN)
          {
//             temp_para=(temp_gain * HI544.sensorBaseGain + BASEGAIN/2)/BASEGAIN;
          }
          else
			  ASSERT(0);

			 HI544DBSOFIA("HI544????????????????????? :\n ");
		  spin_lock(&HI544mipiraw_drv_lock);
          HI544SensorCCT[temp_addr].Para = temp_para;
		  spin_unlock(&HI544mipiraw_drv_lock);
          HI544_write_cmos_sensor(HI544SensorCCT[temp_addr].Addr,temp_para);

            break;
        case CMMCLK_CURRENT:
            switch (item_idx)
            {
                case 0:
                    //no need to apply this item for driving current
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
					spin_lock(&HI544mipiraw_drv_lock);
                    HI544_FAC_SENSOR_REG=ItemValue;
					spin_unlock(&HI544mipiraw_drv_lock);
                    break;
                case 1:
                    HI544_write_cmos_sensor(HI544_FAC_SENSOR_REG,ItemValue);
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

static void HI544_SetDummy( const kal_uint32 iPixels, const kal_uint32 iLines )
{
	kal_uint32 line_length = 0;
	kal_uint32 frame_length = 0;

	if ( SENSOR_MODE_PREVIEW == HI544.sensorMode )	//SXGA size output
	{
		line_length = HI544_PV_PERIOD_PIXEL_NUMS + iPixels;
		frame_length = HI544_PV_PERIOD_LINE_NUMS + iLines;
	}
	else if( SENSOR_MODE_VIDEO== HI544.sensorMode )
	{
		line_length = HI544_VIDEO_PERIOD_PIXEL_NUMS + iPixels;
		frame_length = HI544_VIDEO_PERIOD_LINE_NUMS + iLines;
	}
	else//QSXGA size output
	{
		line_length = HI544_FULL_PERIOD_PIXEL_NUMS + iPixels;
		frame_length = HI544_FULL_PERIOD_LINE_NUMS + iLines;
	}

	//if(HI544.maxExposureLines > frame_length -4 )
	//	return;

	//ASSERT(line_length < HI544_MAX_LINE_LENGTH);		//0xCCCC
	//ASSERT(frame_length < HI544_MAX_FRAME_LENGTH);	//0xFFFF

	//Set total frame length
	HI544_write_cmos_sensor(0x0006, frame_length);
	//HI544_write_cmos_sensor(0x0007, frame_length & 0xFF);

	spin_lock(&HI544mipiraw_drv_lock);
	HI544.maxExposureLines = frame_length -4;
	HI544_FeatureControl_PERIOD_PixelNum = line_length;
	HI544_FeatureControl_PERIOD_LineNum = frame_length;
	spin_unlock(&HI544mipiraw_drv_lock);

	//Set total line length
	HI544_write_cmos_sensor(0x0008, line_length);
	//HI544_write_cmos_sensor(0x0009, line_length & 0xFF);

}   /*  HI544_SetDummy */

void HI544PreviewSetting(void)
{	
	HI544DB("HI544PreviewSetting_2lane_30fps:\n ");

	//////////////////////////////////////////////////////////////////////////
	//			Sensor        	 : Hi-545
	//      Set Name         : Preview
	//      Mode             : D-Binning ( = Scaler 1/2)
	//      Size             : 1296 * 972
	//	set file	 : v0.45
	//	Date		 : 20140724
	//////////////////////////////////////////////////////////////////////////


	HI544_write_cmos_sensor(0x0118, 0x0000); //sleep On

	//--- SREG ---//
	HI544_write_cmos_sensor(0x0B02, 0x0014); //140718
	HI544_write_cmos_sensor(0x0B16, 0x4A8B); //140718 sys_clk 1/2
	HI544_write_cmos_sensor(0x0B18, 0x0000); //140718
	HI544_write_cmos_sensor(0x0B1A, 0x1044); //140718
	HI544_write_cmos_sensor(0x004C, 0x0100); 
	HI544_write_cmos_sensor(0x000C, 0x0000); 

	//---< mipi time >---//
	HI544_write_cmos_sensor(0x0902, 0x4101); //mipi_value_clk_trail.  
	HI544_write_cmos_sensor(0x090A, 0x01E4); //mipi_vblank_delay_h.
	HI544_write_cmos_sensor(0x090C, 0x0005); //mipi_hblank_short_delay_h.
	HI544_write_cmos_sensor(0x090E, 0x0100); //mipi_hblank_long_delay_h.
	HI544_write_cmos_sensor(0x0910, 0x5D04); //05 mipi_LPX
	HI544_write_cmos_sensor(0x0912, 0x030f); //05 mipi_CLK_prepare
	HI544_write_cmos_sensor(0x0914, 0x0204); //02 mipi_clk_pre
	HI544_write_cmos_sensor(0x0916, 0x0707); //09 mipi_data_zero
	HI544_write_cmos_sensor(0x0918, 0x0f04); //0c mipi_clk_post

	//---< Pixel Array Address >------//
	HI544_write_cmos_sensor(0x0012, 0x00AA); //x_addr_start_hact_h. 170
	HI544_write_cmos_sensor(0x0018, 0x0ACB); //x_addr_end_hact_h. 2763 . 2763-170+1=2594
	HI544_write_cmos_sensor(0x0026, 0x0018); //y_addr_start_vact_h. 24
	HI544_write_cmos_sensor(0x002C, 0x07AF); //y_addr_end_vact_h.   1967. 1967-24+1=1944

	//---< Crop size : 2592x1944 >----//
	HI544_write_cmos_sensor(0x0128, 0x0002); // digital_crop_x_offset_l  
	HI544_write_cmos_sensor(0x012A, 0x0000); // digital_crop_y_offset_l  
	HI544_write_cmos_sensor(0x012C, 0x0A20); // digital_crop_image_width 
	HI544_write_cmos_sensor(0x012E, 0x0798); // digital_crop_image_height

	//---< FMT Size : 1296x972 >---------//
	HI544_write_cmos_sensor(0x0110, 0x0510); //X_output_size_h 
	HI544_write_cmos_sensor(0x0112, 0x03CC); //Y_output_size_h 

	//---< Frame/Line Length >-----//
	HI544_write_cmos_sensor(0x0006, 0x07e1); //frame_length_h 2017
	HI544_write_cmos_sensor(0x0008, 0x0B40); //line_length_h 2880
	HI544_write_cmos_sensor(0x000A, 0x0DB0); 

	//---< ETC set >---------------//
	HI544_write_cmos_sensor(0x0000, 0x0100); //orientation. [0]:x-flip, [1]:y-flip.
	HI544_write_cmos_sensor(0x0700, 0xA0A8); //140718 scaler 1/2
	HI544_write_cmos_sensor(0x001E, 0x0101); //140718
	HI544_write_cmos_sensor(0x0032, 0x0101); //140718
	HI544_write_cmos_sensor(0x0A02, 0x0100); //140718 Fast sleep Enable
	HI544_write_cmos_sensor(0x0A04, 0x0133); //TEST PATTERN

	HI544_write_cmos_sensor(0x0118, 0x0100); //sleep Off 

	mDELAY(50);

	ReEnteyCamera = KAL_FALSE;

    HI544DB("HI544PreviewSetting_2lane exit :\n ");	
}


void HI544VideoSetting(void)
{

	HI544DB("HI544VideoSetting:\n ");
	//////////////////////////////////////////////////////////////////////////
	//			Sensor			 : Hi-545
	//		Set Name		 : FHD
	//		Mode			 : D-Binning ( = Scaler 3/4)
	//		Size			 : 1920 * 1080
	//	set file	 : v0.45
	//	Date		 : 20140724
	//////////////////////////////////////////////////////////////////////////
	
	
	HI544_write_cmos_sensor(0x0118, 0x0000); //sleep On
	
	//--- SREG ---//
	HI544_write_cmos_sensor(0x0B02, 0x0014); //140718
	HI544_write_cmos_sensor(0x0B16, 0x4A0B); 
	HI544_write_cmos_sensor(0x0B18, 0x0000); //140718
	HI544_write_cmos_sensor(0x0B1A, 0x1044); //140718
	HI544_write_cmos_sensor(0x004C, 0x0100); 
	HI544_write_cmos_sensor(0x000C, 0x0000);
	
	//---< mipi time >---//
	HI544_write_cmos_sensor(0x0902, 0x4101); 
	HI544_write_cmos_sensor(0x090A, 0x03E4); 
	HI544_write_cmos_sensor(0x090C, 0x0020); 
	HI544_write_cmos_sensor(0x090E, 0x0020); 
	HI544_write_cmos_sensor(0x0910, 0x5D07); 
	HI544_write_cmos_sensor(0x0912, 0x061e); 
	HI544_write_cmos_sensor(0x0914, 0x0407); 
	HI544_write_cmos_sensor(0x0916, 0x0b0a); 
	HI544_write_cmos_sensor(0x0918, 0x0e09); 
	
	//---< Pixel Array Address >------//
	HI544_write_cmos_sensor(0x0012, 0x00BA); //186
	HI544_write_cmos_sensor(0x0018, 0x0ABB); //2747. 2747-186+1=2562
	HI544_write_cmos_sensor(0x0026, 0x0114); //276
	HI544_write_cmos_sensor(0x002C, 0x06b3); //1715. 1715-276=1440
	
	//---< Crop size : 2560x1440 >----//
	HI544_write_cmos_sensor(0x0128, 0x0002); // digital_crop_x_offset_l
	HI544_write_cmos_sensor(0x012A, 0x0000); // digital_crop_y_offset_l
	HI544_write_cmos_sensor(0x012C, 0x0A00); // digital_crop_image_width
	HI544_write_cmos_sensor(0x012E, 0x05A0); // digital_crop_image_height
	
	//---< FMT Size : 1920x1080 >---------//
	HI544_write_cmos_sensor(0x0110, 0x0780); //X_output_size_h	   
	HI544_write_cmos_sensor(0x0112, 0x0438); //Y_output_size_h	
	
	//---< Frame/Line Length >-----//
	HI544_write_cmos_sensor(0x0006, 0x07C0); //frame_length_h 1984 @2560x1440@30.7fps
	HI544_write_cmos_sensor(0x0008, 0x0B40); //line_length_h 2880
	HI544_write_cmos_sensor(0x000A, 0x0DB0); //line_length for binning 3504
	//---------------------------------------//
	
	//---< ETC set >---------------//
	HI544_write_cmos_sensor(0x0700, 0x5090); //140718
	HI544_write_cmos_sensor(0x001E, 0x0101); //140718
	HI544_write_cmos_sensor(0x0032, 0x0101); //140718
	HI544_write_cmos_sensor(0x0A02, 0x0100); //140718 Fast sleep Enable
	HI544_write_cmos_sensor(0x0A04, 0x0133); //isp_en. [9]s-gamma,[8]MIPI_en,[6]compresion10to8,[5]Scaler,[4]window,[3]DG,[2]LSC,[1]adpc,[0]tpg //TEST PATTERN
	
	HI544_write_cmos_sensor(0x0118, 0x0100); //sleep Off 

	
	mDELAY(50);

	ReEnteyCamera = KAL_FALSE;

	HI544DB("HI544VideoSetting_4:3 exit :\n ");
}


void HI544CaptureSetting(void)
{

    if(ReEnteyCamera == KAL_TRUE)
    {
		HI544DB("HI544CaptureSetting_2lane_SleepIn :\n ");
    }
	else
	{
		HI544DB("HI544CaptureSetting_2lane_streamOff :\n ");
	}

	HI544DB("HI544CaptureSetting_2lane_OB:\n ");
	

	//////////////////////////////////////////////////////////////////////////
	//			Sensor			 : Hi-545
	//		Set Name		 : Capture
	//		Mode			 : Normal
	//		Size			 : 2592 * 1944
	//	set file	 : v0.45
	//	Date		 : 20140724
	//////////////////////////////////////////////////////////////////////////


	HI544_write_cmos_sensor(0x0118, 0x0000); //sleep On

	//--- SREG ---//
	HI544_write_cmos_sensor(0x0B02, 0x0014); //140718
	HI544_write_cmos_sensor(0x0B16, 0x4A0B); 
	HI544_write_cmos_sensor(0x0B18, 0x0000); //140718
	HI544_write_cmos_sensor(0x0B1A, 0x1044); //140718
	HI544_write_cmos_sensor(0x004C, 0x0100); 
	HI544_write_cmos_sensor(0x000C, 0x0000); 

	//---< mipi time >---//
	HI544_write_cmos_sensor(0x0902, 0x4101); 
	HI544_write_cmos_sensor(0x090A, 0x03E4); 
	HI544_write_cmos_sensor(0x090C, 0x0020); 
	HI544_write_cmos_sensor(0x090E, 0x0020); 
	HI544_write_cmos_sensor(0x0910, 0x5D07); 
	HI544_write_cmos_sensor(0x0912, 0x061e); 
	HI544_write_cmos_sensor(0x0914, 0x0407); 
	HI544_write_cmos_sensor(0x0916, 0x0b0a); 
	HI544_write_cmos_sensor(0x0918, 0x0e09); 

	//---< Pixel Array Address >------//
	HI544_write_cmos_sensor(0x0012, 0x00AA); //x_addr_start_hact_h. 170
	HI544_write_cmos_sensor(0x0018, 0x0ACB); //x_addr_end_hact_h. 2763 . 2763-170+1=2594
	HI544_write_cmos_sensor(0x0026, 0x0018); //y_addr_start_vact_h. 24
	HI544_write_cmos_sensor(0x002C, 0x07AF); //y_addr_end_vact_h.	1967. 1967-24+1=1944

	//---< Crop size : 2592x1944 >----//
	HI544_write_cmos_sensor(0x0128, 0x0002); //digital_crop_x_offset_l	
	HI544_write_cmos_sensor(0x012A, 0x0000); //digital_crop_y_offset_l	
	HI544_write_cmos_sensor(0x012C, 0x0A20); //2592 digital_crop_image_width 
	HI544_write_cmos_sensor(0x012E, 0x0798); //1944 digital_crop_image_height 

	//---< FMT Size : 2592x1944 >---------//
	HI544_write_cmos_sensor(0x0110, 0x0A20); //X_output_size_h 
	HI544_write_cmos_sensor(0x0112, 0x0798); //Y_output_size_h 

	//---< Frame/Line Length >-----//
	HI544_write_cmos_sensor(0x0006, 0x07e1); //frame_length_h 2017
	HI544_write_cmos_sensor(0x0008, 0x0B40); //line_length_h 2880
	HI544_write_cmos_sensor(0x000A, 0x0DB0); 

	//---< ETC set >---------------//
	HI544_write_cmos_sensor(0x0000, 0x0100); //orientation. [0]:x-flip, [1]:y-flip.
	HI544_write_cmos_sensor(0x0700, 0x0590); //140718
	HI544_write_cmos_sensor(0x001E, 0x0101); //140718
	HI544_write_cmos_sensor(0x0032, 0x0101); //140718
	HI544_write_cmos_sensor(0x0A02, 0x0100); //140718 Fast sleep Enable
	HI544_write_cmos_sensor(0x0A04, 0x011B); //TEST PATTERN enable

	HI544_write_cmos_sensor(0x0118, 0x0100); //sleep Off 

	mDELAY(50);

	ReEnteyCamera = KAL_FALSE;
	
	HI544DB("HI544CaptureSetting_2lane exit :\n ");
}

static void HI544_Sensor_Init(void)
{
	//////////////////////////////////////////////////////////////////////////
	//<<<<<<<<<  Sensor Information  >>>>>>>>>>///////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	//
	//	Sensor        	: Hi-544
	//
	//	Initial Ver.  	: v0.47 for LGE
	//	Initial Date		: 2014-10-15
	//
	//	Customs          : MTK for LGE
	//	AP or B/E        : MT6582
	//	
	//	Image size       : 2604x1956
	//	mclk/pclk        : 24mhz / 88Mhz
	//	MIPI speed(Mbps) : 880Mbps (each lane)
	//	MIPI						 : Non-continuous 
	//	Frame Length     : 1984
	//	V-Blank          : 546us
	//	Line Length	     : 2880
	//	H-Blank          : 386ns / 387ns	
	//	Max Fps          : 30fps (= Exp.time : 33ms )	
	//	Pixel order      : Blue 1st (=BGGR)
	//	X/Y-flip         : No-X/Y flip
	//	I2C Address      : 0x40(Write), 0x41(Read)
	//	AG               : x1
	//	DG               : x1
	//	BLC offset       : 64 code
	//
	//////////////////////////////////////////////////////////////////////////
	//<<<<<<<<<  Notice >>>>>>>>>/////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	//
	//	Notice
	//	1) I2C Address & Data Type is 2Byte. 
	//	2) I2C Data construction that high byte is low addres, and low byte is high address. 
	//		 Initial register address must have used the even number. 
	//		 ex){Address, Data} = {Address(Even, 2byte), Data(2byte)} <==== Used Type
	//												= {Address(Even, 2byte(low address)), Data(1byte)} + {Address(Odd, 2byte(high address)), Data(1byte)} <== Not Used Type
	//												= HI544_write_cmos_sensor(0x0000, 0x0F03} => HI544_write_cmos_sensor(0x0000, 0x0F} + HI544_write_cmos_sensor(0x0001, 0x03}
	//	3) The Continuous Mode of MIPI 2 Lane set is HI544_write_cmos_sensor(0x0902, 0x4301}. And, 1lane set is HI544_write_cmos_sensor(0x0902, 0x0301}.
	//		 The Non-continuous Mode of MIPI 2 Lane set is HI544_write_cmos_sensor(0x0902, 0x4101}. And, 1lane set is HI544_write_cmos_sensor(0x0902, 0x0101}.	
	//	4) Analog Gain address is 0x003a. 
	//			ex)	0x0000 = x1, 0x7000 = x8, 0xf000 = x16
	//
	/////////////////////////////////////////////////////////////////////////



	////////////////////////////////////////////////
	////////////// Hi-544 Initial //////////////////
	////////////////////////////////////////////////
	//-- Start --//

	HI544_write_cmos_sensor(0x0118, 0x0000); //sleep On

	//--- SRAM timing control---//
	HI544_write_cmos_sensor(0x0E00, 0x0101);
	HI544_write_cmos_sensor(0x0E02, 0x0101);
	HI544_write_cmos_sensor(0x0E04, 0x0101);
	HI544_write_cmos_sensor(0x0E06, 0x0101);
	HI544_write_cmos_sensor(0x0E08, 0x0101);
	HI544_write_cmos_sensor(0x0E0A, 0x0101);
	HI544_write_cmos_sensor(0x0E0C, 0x0101);
	HI544_write_cmos_sensor(0x0E0E, 0x0101);

	//Firmware 2Lane v0.39, LB OTP Burst On + double luma under dark. FW 20140804
	HI544_write_cmos_sensor(0x2000, 0x4031);
	HI544_write_cmos_sensor(0x2002, 0x83F8);
	HI544_write_cmos_sensor(0x2004, 0x4104);
	HI544_write_cmos_sensor(0x2006, 0x4307);
	HI544_write_cmos_sensor(0x2008, 0x4382);
	HI544_write_cmos_sensor(0x200a, 0x80D0);
	HI544_write_cmos_sensor(0x200c, 0x4382);
	HI544_write_cmos_sensor(0x200e, 0x8070);
	HI544_write_cmos_sensor(0x2010, 0x43A2);
	HI544_write_cmos_sensor(0x2012, 0x0B80);
	HI544_write_cmos_sensor(0x2014, 0x0C0A);
	HI544_write_cmos_sensor(0x2016, 0x4382);
	HI544_write_cmos_sensor(0x2018, 0x0B90);
	HI544_write_cmos_sensor(0x201a, 0x0C0A);
	HI544_write_cmos_sensor(0x201c, 0x4382);
	HI544_write_cmos_sensor(0x201e, 0x0B9C);
	HI544_write_cmos_sensor(0x2020, 0x0C0A);
	HI544_write_cmos_sensor(0x2022, 0x93D2);
	HI544_write_cmos_sensor(0x2024, 0x003D);
	HI544_write_cmos_sensor(0x2026, 0x2002);
	HI544_write_cmos_sensor(0x2028, 0x4030);
	HI544_write_cmos_sensor(0x202a, 0xF6C0);
	HI544_write_cmos_sensor(0x202c, 0x43C2);
	HI544_write_cmos_sensor(0x202e, 0x0F82);
	HI544_write_cmos_sensor(0x2030, 0x425F);
	HI544_write_cmos_sensor(0x2032, 0x0118);
	HI544_write_cmos_sensor(0x2034, 0xF37F);
	HI544_write_cmos_sensor(0x2036, 0x930F);
	HI544_write_cmos_sensor(0x2038, 0x2002);
	HI544_write_cmos_sensor(0x203a, 0x0CC8);
	HI544_write_cmos_sensor(0x203c, 0x3FF9);
	HI544_write_cmos_sensor(0x203e, 0x4F82);
	HI544_write_cmos_sensor(0x2040, 0x809A);
	HI544_write_cmos_sensor(0x2042, 0x43D2);
	HI544_write_cmos_sensor(0x2044, 0x0A80);
	HI544_write_cmos_sensor(0x2046, 0x43D2);
	HI544_write_cmos_sensor(0x2048, 0x0180);
	HI544_write_cmos_sensor(0x204a, 0x43D2);
	HI544_write_cmos_sensor(0x204c, 0x019A);
	HI544_write_cmos_sensor(0x204e, 0x40F2);
	HI544_write_cmos_sensor(0x2050, 0x0009);
	HI544_write_cmos_sensor(0x2052, 0x019B);
	HI544_write_cmos_sensor(0x2054, 0x12B0);
	HI544_write_cmos_sensor(0x2056, 0xFE26);
	HI544_write_cmos_sensor(0x2058, 0x4382);
	HI544_write_cmos_sensor(0x205a, 0x8094);
	HI544_write_cmos_sensor(0x205c, 0x93D2);
	HI544_write_cmos_sensor(0x205e, 0x003E);
	HI544_write_cmos_sensor(0x2060, 0x2002);
	HI544_write_cmos_sensor(0x2062, 0x4030);
	HI544_write_cmos_sensor(0x2064, 0xF5A0);
	HI544_write_cmos_sensor(0x2066, 0x4308);
	HI544_write_cmos_sensor(0x2068, 0x5038);
	HI544_write_cmos_sensor(0x206a, 0x0030);
	HI544_write_cmos_sensor(0x206c, 0x480F);
	HI544_write_cmos_sensor(0x206e, 0x12B0);
	HI544_write_cmos_sensor(0x2070, 0xFE38);
	HI544_write_cmos_sensor(0x2072, 0x403B);
	HI544_write_cmos_sensor(0x2074, 0x7606);
	HI544_write_cmos_sensor(0x2076, 0x4B29);
	HI544_write_cmos_sensor(0x2078, 0x5318);
	HI544_write_cmos_sensor(0x207a, 0x480F);
	HI544_write_cmos_sensor(0x207c, 0x12B0);
	HI544_write_cmos_sensor(0x207e, 0xFE38);
	HI544_write_cmos_sensor(0x2080, 0x4B2A);
	HI544_write_cmos_sensor(0x2082, 0x5318);
	HI544_write_cmos_sensor(0x2084, 0x480F);
	HI544_write_cmos_sensor(0x2086, 0x12B0);
	HI544_write_cmos_sensor(0x2088, 0xFE38);
	HI544_write_cmos_sensor(0x208a, 0x4A0D);
	HI544_write_cmos_sensor(0x208c, 0xF03D);
	HI544_write_cmos_sensor(0x208e, 0x000F);
	HI544_write_cmos_sensor(0x2090, 0x108D);
	HI544_write_cmos_sensor(0x2092, 0x4B2E);
	HI544_write_cmos_sensor(0x2094, 0x5E0E);
	HI544_write_cmos_sensor(0x2096, 0x5E0E);
	HI544_write_cmos_sensor(0x2098, 0x5E0E);
	HI544_write_cmos_sensor(0x209a, 0x5E0E);
	HI544_write_cmos_sensor(0x209c, 0x4A0F);
	HI544_write_cmos_sensor(0x209e, 0xC312);
	HI544_write_cmos_sensor(0x20a0, 0x100F);
	HI544_write_cmos_sensor(0x20a2, 0x110F);
	HI544_write_cmos_sensor(0x20a4, 0x110F);
	HI544_write_cmos_sensor(0x20a6, 0x110F);
	HI544_write_cmos_sensor(0x20a8, 0x590D);
	HI544_write_cmos_sensor(0x20aa, 0x4D87);
	HI544_write_cmos_sensor(0x20ac, 0x5000);
	HI544_write_cmos_sensor(0x20ae, 0x5F0E);
	HI544_write_cmos_sensor(0x20b0, 0x4E87);
	HI544_write_cmos_sensor(0x20b2, 0x6000);
	HI544_write_cmos_sensor(0x20b4, 0x5327);
	HI544_write_cmos_sensor(0x20b6, 0x5038);
	HI544_write_cmos_sensor(0x20b8, 0xFFD1);
	HI544_write_cmos_sensor(0x20ba, 0x9038);
	HI544_write_cmos_sensor(0x20bc, 0x0300);
	HI544_write_cmos_sensor(0x20be, 0x2BD4);
	HI544_write_cmos_sensor(0x20c0, 0x0261);
	HI544_write_cmos_sensor(0x20c2, 0x0000);
	HI544_write_cmos_sensor(0x20c4, 0x43A2);
	HI544_write_cmos_sensor(0x20c6, 0x0384);
	HI544_write_cmos_sensor(0x20c8, 0x42B2);
	HI544_write_cmos_sensor(0x20ca, 0x0386);
	HI544_write_cmos_sensor(0x20cc, 0x43C2);
	HI544_write_cmos_sensor(0x20ce, 0x0180);
	HI544_write_cmos_sensor(0x20d0, 0x43D2);
	HI544_write_cmos_sensor(0x20d2, 0x003D);
	HI544_write_cmos_sensor(0x20d4, 0x40B2);
	HI544_write_cmos_sensor(0x20d6, 0x808B);
	HI544_write_cmos_sensor(0x20d8, 0x0B88);
	HI544_write_cmos_sensor(0x20da, 0x0C0A);
	HI544_write_cmos_sensor(0x20dc, 0x40B2);
	HI544_write_cmos_sensor(0x20de, 0x1009);
	HI544_write_cmos_sensor(0x20e0, 0x0B8A);
	HI544_write_cmos_sensor(0x20e2, 0x0C0A);
	HI544_write_cmos_sensor(0x20e4, 0x40B2);
	HI544_write_cmos_sensor(0x20e6, 0xC40C);
	HI544_write_cmos_sensor(0x20e8, 0x0B8C);
	HI544_write_cmos_sensor(0x20ea, 0x0C0A);
	HI544_write_cmos_sensor(0x20ec, 0x40B2);
	HI544_write_cmos_sensor(0x20ee, 0xC9E1);
	HI544_write_cmos_sensor(0x20f0, 0x0B8E);
	HI544_write_cmos_sensor(0x20f2, 0x0C0A);
	HI544_write_cmos_sensor(0x20f4, 0x40B2);
	HI544_write_cmos_sensor(0x20f6, 0x0C1E);
	HI544_write_cmos_sensor(0x20f8, 0x0B92);
	HI544_write_cmos_sensor(0x20fa, 0x0C0A);
	HI544_write_cmos_sensor(0x20fc, 0x43D2);
	HI544_write_cmos_sensor(0x20fe, 0x0F82);
	HI544_write_cmos_sensor(0x2100, 0x0C3C);
	HI544_write_cmos_sensor(0x2102, 0x0C3C);
	HI544_write_cmos_sensor(0x2104, 0x0C3C);
	HI544_write_cmos_sensor(0x2106, 0x0C3C);
	HI544_write_cmos_sensor(0x2108, 0x421F);
	HI544_write_cmos_sensor(0x210a, 0x00A6);
	HI544_write_cmos_sensor(0x210c, 0x503F);
	HI544_write_cmos_sensor(0x210e, 0x07D0);
	HI544_write_cmos_sensor(0x2110, 0x3811);
	HI544_write_cmos_sensor(0x2112, 0x4F82);
	HI544_write_cmos_sensor(0x2114, 0x7100);
	HI544_write_cmos_sensor(0x2116, 0x0004);
	HI544_write_cmos_sensor(0x2118, 0x0C0D);
	HI544_write_cmos_sensor(0x211a, 0x0005);
	HI544_write_cmos_sensor(0x211c, 0x0C04);
	HI544_write_cmos_sensor(0x211e, 0x000D);
	HI544_write_cmos_sensor(0x2120, 0x0C09);
	HI544_write_cmos_sensor(0x2122, 0x003D);
	HI544_write_cmos_sensor(0x2124, 0x0C1D);
	HI544_write_cmos_sensor(0x2126, 0x003C);
	HI544_write_cmos_sensor(0x2128, 0x0C13);
	HI544_write_cmos_sensor(0x212a, 0x0004);
	HI544_write_cmos_sensor(0x212c, 0x0C09);
	HI544_write_cmos_sensor(0x212e, 0x0004);
	HI544_write_cmos_sensor(0x2130, 0x533F);
	HI544_write_cmos_sensor(0x2132, 0x37EF);
	HI544_write_cmos_sensor(0x2134, 0x4392);
	HI544_write_cmos_sensor(0x2136, 0x8096);
	HI544_write_cmos_sensor(0x2138, 0x4382);
	HI544_write_cmos_sensor(0x213a, 0x809E);
	HI544_write_cmos_sensor(0x213c, 0x4382);
	HI544_write_cmos_sensor(0x213e, 0x80B6);
	HI544_write_cmos_sensor(0x2140, 0x4382);
	HI544_write_cmos_sensor(0x2142, 0x80BE);
	HI544_write_cmos_sensor(0x2144, 0x4382);
	HI544_write_cmos_sensor(0x2146, 0x80A2);
	HI544_write_cmos_sensor(0x2148, 0x40B2);
	HI544_write_cmos_sensor(0x214a, 0x0028);
	HI544_write_cmos_sensor(0x214c, 0x7000);
	HI544_write_cmos_sensor(0x214e, 0x43A2);
	HI544_write_cmos_sensor(0x2150, 0x80A0);
	HI544_write_cmos_sensor(0x2152, 0xB3E2);
	HI544_write_cmos_sensor(0x2154, 0x00B4);
	HI544_write_cmos_sensor(0x2156, 0x2402);
	HI544_write_cmos_sensor(0x2158, 0x4392);
	HI544_write_cmos_sensor(0x215a, 0x80A0);
	HI544_write_cmos_sensor(0x215c, 0x4326);
	HI544_write_cmos_sensor(0x215e, 0xB3D2);
	HI544_write_cmos_sensor(0x2160, 0x00B4);
	HI544_write_cmos_sensor(0x2162, 0x2002);
	HI544_write_cmos_sensor(0x2164, 0x4030);
	HI544_write_cmos_sensor(0x2166, 0xF590);
	HI544_write_cmos_sensor(0x2168, 0x4306);
	HI544_write_cmos_sensor(0x216a, 0x4384);
	HI544_write_cmos_sensor(0x216c, 0x0002);
	HI544_write_cmos_sensor(0x216e, 0x4384);
	HI544_write_cmos_sensor(0x2170, 0x0006);
	HI544_write_cmos_sensor(0x2172, 0x4382);
	HI544_write_cmos_sensor(0x2174, 0x809C);
	HI544_write_cmos_sensor(0x2176, 0x4382);
	HI544_write_cmos_sensor(0x2178, 0x8098);
	HI544_write_cmos_sensor(0x217a, 0x40B2);
	HI544_write_cmos_sensor(0x217c, 0x0005);
	HI544_write_cmos_sensor(0x217e, 0x7320);
	HI544_write_cmos_sensor(0x2180, 0x4392);
	HI544_write_cmos_sensor(0x2182, 0x7326);
	HI544_write_cmos_sensor(0x2184, 0x12B0);
	HI544_write_cmos_sensor(0x2186, 0xF952);
	HI544_write_cmos_sensor(0x2188, 0x4392);
	HI544_write_cmos_sensor(0x218a, 0x731C);
	HI544_write_cmos_sensor(0x218c, 0x9382);
	HI544_write_cmos_sensor(0x218e, 0x8096);
	HI544_write_cmos_sensor(0x2190, 0x200A);
	HI544_write_cmos_sensor(0x2192, 0x0B00);
	HI544_write_cmos_sensor(0x2194, 0x7302);
	HI544_write_cmos_sensor(0x2196, 0x02BC);
	HI544_write_cmos_sensor(0x2198, 0x4382);
	HI544_write_cmos_sensor(0x219a, 0x7004);
	HI544_write_cmos_sensor(0x219c, 0x430F);
	HI544_write_cmos_sensor(0x219e, 0x12B0);
	HI544_write_cmos_sensor(0x21a0, 0xF752);
	HI544_write_cmos_sensor(0x21a2, 0x12B0);
	HI544_write_cmos_sensor(0x21a4, 0xF952);
	HI544_write_cmos_sensor(0x21a6, 0x4392);
	HI544_write_cmos_sensor(0x21a8, 0x80BC);
	HI544_write_cmos_sensor(0x21aa, 0x4382);
	HI544_write_cmos_sensor(0x21ac, 0x740E);
	HI544_write_cmos_sensor(0x21ae, 0xB3E2);
	HI544_write_cmos_sensor(0x21b0, 0x0080);
	HI544_write_cmos_sensor(0x21b2, 0x2402);
	HI544_write_cmos_sensor(0x21b4, 0x4392);
	HI544_write_cmos_sensor(0x21b6, 0x740E);
	HI544_write_cmos_sensor(0x21b8, 0x431F);
	HI544_write_cmos_sensor(0x21ba, 0x12B0);
	HI544_write_cmos_sensor(0x21bc, 0xF752);
	HI544_write_cmos_sensor(0x21be, 0x4392);
	HI544_write_cmos_sensor(0x21c0, 0x7004);
	HI544_write_cmos_sensor(0x21c2, 0x4682);
	HI544_write_cmos_sensor(0x21c4, 0x7110);
	HI544_write_cmos_sensor(0x21c6, 0x9382);
	HI544_write_cmos_sensor(0x21c8, 0x8092);
	HI544_write_cmos_sensor(0x21ca, 0x2005);
	HI544_write_cmos_sensor(0x21cc, 0x9392);
	HI544_write_cmos_sensor(0x21ce, 0x7110);
	HI544_write_cmos_sensor(0x21d0, 0x2402);
	HI544_write_cmos_sensor(0x21d2, 0x4030);
	HI544_write_cmos_sensor(0x21d4, 0xF494);
	HI544_write_cmos_sensor(0x21d6, 0x9392);
	HI544_write_cmos_sensor(0x21d8, 0x7110);
	HI544_write_cmos_sensor(0x21da, 0x20A4);
	HI544_write_cmos_sensor(0x21dc, 0x0B00);
	HI544_write_cmos_sensor(0x21de, 0x7302);
	HI544_write_cmos_sensor(0x21e0, 0x0032);
	HI544_write_cmos_sensor(0x21e2, 0x4382);
	HI544_write_cmos_sensor(0x21e4, 0x7004);
	HI544_write_cmos_sensor(0x21e6, 0x0B00);
	HI544_write_cmos_sensor(0x21e8, 0x7302);
	HI544_write_cmos_sensor(0x21ea, 0x03E8);
	HI544_write_cmos_sensor(0x21ec, 0x0800);
	HI544_write_cmos_sensor(0x21ee, 0x7114);
	HI544_write_cmos_sensor(0x21f0, 0x425F);
	HI544_write_cmos_sensor(0x21f2, 0x0C9C);
	HI544_write_cmos_sensor(0x21f4, 0x4F4E);
	HI544_write_cmos_sensor(0x21f6, 0x430F);
	HI544_write_cmos_sensor(0x21f8, 0x4E0D);
	HI544_write_cmos_sensor(0x21fa, 0x430C);
	HI544_write_cmos_sensor(0x21fc, 0x421F);
	HI544_write_cmos_sensor(0x21fe, 0x0C9A);
	HI544_write_cmos_sensor(0x2200, 0xDF0C);
	HI544_write_cmos_sensor(0x2202, 0x1204);
	HI544_write_cmos_sensor(0x2204, 0x440F);
	HI544_write_cmos_sensor(0x2206, 0x532F);
	HI544_write_cmos_sensor(0x2208, 0x120F);
	HI544_write_cmos_sensor(0x220a, 0x1212);
	HI544_write_cmos_sensor(0x220c, 0x0CA2);
	HI544_write_cmos_sensor(0x220e, 0x403E);
	HI544_write_cmos_sensor(0x2210, 0x80C0);
	HI544_write_cmos_sensor(0x2212, 0x403F);
	HI544_write_cmos_sensor(0x2214, 0x8072);
	HI544_write_cmos_sensor(0x2216, 0x12B0);
	HI544_write_cmos_sensor(0x2218, 0xF7BA);
	HI544_write_cmos_sensor(0x221a, 0x4F07);
	HI544_write_cmos_sensor(0x221c, 0x425F);
	HI544_write_cmos_sensor(0x221e, 0x0CA0);
	HI544_write_cmos_sensor(0x2220, 0x4F4E);
	HI544_write_cmos_sensor(0x2222, 0x430F);
	HI544_write_cmos_sensor(0x2224, 0x4E0D);
	HI544_write_cmos_sensor(0x2226, 0x430C);
	HI544_write_cmos_sensor(0x2228, 0x421F);
	HI544_write_cmos_sensor(0x222a, 0x0C9E);
	HI544_write_cmos_sensor(0x222c, 0xDF0C);
	HI544_write_cmos_sensor(0x222e, 0x440F);
	HI544_write_cmos_sensor(0x2230, 0x522F);
	HI544_write_cmos_sensor(0x2232, 0x120F);
	HI544_write_cmos_sensor(0x2234, 0x532F);
	HI544_write_cmos_sensor(0x2236, 0x120F);
	HI544_write_cmos_sensor(0x2238, 0x1212);
	HI544_write_cmos_sensor(0x223a, 0x0CA4);
	HI544_write_cmos_sensor(0x223c, 0x403E);
	HI544_write_cmos_sensor(0x223e, 0x80A4);
	HI544_write_cmos_sensor(0x2240, 0x403F);
	HI544_write_cmos_sensor(0x2242, 0x8050);
	HI544_write_cmos_sensor(0x2244, 0x12B0);
	HI544_write_cmos_sensor(0x2246, 0xF7BA);
	HI544_write_cmos_sensor(0x2248, 0x4F08);
	HI544_write_cmos_sensor(0x224a, 0x430D);
	HI544_write_cmos_sensor(0x224c, 0x441E);
	HI544_write_cmos_sensor(0x224e, 0x0004);
	HI544_write_cmos_sensor(0x2250, 0x442F);
	HI544_write_cmos_sensor(0x2252, 0x5031);
	HI544_write_cmos_sensor(0x2254, 0x000C);
	HI544_write_cmos_sensor(0x2256, 0x9E0F);
	HI544_write_cmos_sensor(0x2258, 0x2C01);
	HI544_write_cmos_sensor(0x225a, 0x431D);
	HI544_write_cmos_sensor(0x225c, 0x8E0F);
	HI544_write_cmos_sensor(0x225e, 0x930F);
	HI544_write_cmos_sensor(0x2260, 0x3402);
	HI544_write_cmos_sensor(0x2262, 0xE33F);
	HI544_write_cmos_sensor(0x2264, 0x531F);
	HI544_write_cmos_sensor(0x2266, 0x421E);
	HI544_write_cmos_sensor(0x2268, 0x0CA2);
	HI544_write_cmos_sensor(0x226a, 0xC312);
	HI544_write_cmos_sensor(0x226c, 0x100E);
	HI544_write_cmos_sensor(0x226e, 0x9E0F);
	HI544_write_cmos_sensor(0x2270, 0x2804);
	HI544_write_cmos_sensor(0x2272, 0x930D);
	HI544_write_cmos_sensor(0x2274, 0x2001);
	HI544_write_cmos_sensor(0x2276, 0x5317);
	HI544_write_cmos_sensor(0x2278, 0x5D08);
	HI544_write_cmos_sensor(0x227a, 0x403B);
	HI544_write_cmos_sensor(0x227c, 0x0196);
	HI544_write_cmos_sensor(0x227e, 0x403D);
	HI544_write_cmos_sensor(0x2280, 0x0040);
	HI544_write_cmos_sensor(0x2282, 0x4D0F);
	HI544_write_cmos_sensor(0x2284, 0x8B2F);
	HI544_write_cmos_sensor(0x2286, 0x470A);
	HI544_write_cmos_sensor(0x2288, 0x4F0C);
	HI544_write_cmos_sensor(0x228a, 0x12B0);
	HI544_write_cmos_sensor(0x228c, 0xFE58);
	HI544_write_cmos_sensor(0x228e, 0x4E09);
	HI544_write_cmos_sensor(0x2290, 0xC312);
	HI544_write_cmos_sensor(0x2292, 0x1009);
	HI544_write_cmos_sensor(0x2294, 0x1109);
	HI544_write_cmos_sensor(0x2296, 0x1109);
	HI544_write_cmos_sensor(0x2298, 0x1109);
	HI544_write_cmos_sensor(0x229a, 0x1109);
	HI544_write_cmos_sensor(0x229c, 0x1109);
	HI544_write_cmos_sensor(0x229e, 0x4D0F);
	HI544_write_cmos_sensor(0x22a0, 0x8B2F);
	HI544_write_cmos_sensor(0x22a2, 0x480A);
	HI544_write_cmos_sensor(0x22a4, 0x4F0C);
	HI544_write_cmos_sensor(0x22a6, 0x12B0);
	HI544_write_cmos_sensor(0x22a8, 0xFE58);
	HI544_write_cmos_sensor(0x22aa, 0x4E0F);
	HI544_write_cmos_sensor(0x22ac, 0xC312);
	HI544_write_cmos_sensor(0x22ae, 0x100F);
	HI544_write_cmos_sensor(0x22b0, 0x110F);
	HI544_write_cmos_sensor(0x22b2, 0x110F);
	HI544_write_cmos_sensor(0x22b4, 0x110F);
	HI544_write_cmos_sensor(0x22b6, 0x110F);
	HI544_write_cmos_sensor(0x22b8, 0x110F);
	HI544_write_cmos_sensor(0x22ba, 0x5F09);
	HI544_write_cmos_sensor(0x22bc, 0xC312);
	HI544_write_cmos_sensor(0x22be, 0x1009);
	HI544_write_cmos_sensor(0x22c0, 0x92B2);
	HI544_write_cmos_sensor(0x22c2, 0x80BE);
	HI544_write_cmos_sensor(0x22c4, 0x280C);
	HI544_write_cmos_sensor(0x22c6, 0x90B2);
	HI544_write_cmos_sensor(0x22c8, 0x0096);
	HI544_write_cmos_sensor(0x22ca, 0x80B6);
	HI544_write_cmos_sensor(0x22cc, 0x2408);
	HI544_write_cmos_sensor(0x22ce, 0x0900);
	HI544_write_cmos_sensor(0x22d0, 0x710E);
	HI544_write_cmos_sensor(0x22d2, 0x0B00);
	HI544_write_cmos_sensor(0x22d4, 0x7302);
	HI544_write_cmos_sensor(0x22d6, 0x0320);
	HI544_write_cmos_sensor(0x22d8, 0x12B0);
	HI544_write_cmos_sensor(0x22da, 0xF6F2);
	HI544_write_cmos_sensor(0x22dc, 0x3F74);
	HI544_write_cmos_sensor(0x22de, 0x470A);
	HI544_write_cmos_sensor(0x22e0, 0x580A);
	HI544_write_cmos_sensor(0x22e2, 0xC312);
	HI544_write_cmos_sensor(0x22e4, 0x100A);
	HI544_write_cmos_sensor(0x22e6, 0x890A);
	HI544_write_cmos_sensor(0x22e8, 0xB3E2);
	HI544_write_cmos_sensor(0x22ea, 0x0C81);
	HI544_write_cmos_sensor(0x22ec, 0x2418);
	HI544_write_cmos_sensor(0x22ee, 0x425F);
	HI544_write_cmos_sensor(0x22f0, 0x0C92);
	HI544_write_cmos_sensor(0x22f2, 0xF37F);
	HI544_write_cmos_sensor(0x22f4, 0x9F0A);
	HI544_write_cmos_sensor(0x22f6, 0x280E);
	HI544_write_cmos_sensor(0x22f8, 0x425F);
	HI544_write_cmos_sensor(0x22fa, 0x0C92);
	HI544_write_cmos_sensor(0x22fc, 0xF37F);
	HI544_write_cmos_sensor(0x22fe, 0x4A0E);
	HI544_write_cmos_sensor(0x2300, 0x8F0E);
	HI544_write_cmos_sensor(0x2302, 0x4E82);
	HI544_write_cmos_sensor(0x2304, 0x0CAC);
	HI544_write_cmos_sensor(0x2306, 0x425F);
	HI544_write_cmos_sensor(0x2308, 0x0C92);
	HI544_write_cmos_sensor(0x230a, 0xF37F);
	HI544_write_cmos_sensor(0x230c, 0x8F0A);
	HI544_write_cmos_sensor(0x230e, 0x4A82);
	HI544_write_cmos_sensor(0x2310, 0x0CAE);
	HI544_write_cmos_sensor(0x2312, 0x3FDD);
	HI544_write_cmos_sensor(0x2314, 0x4382);
	HI544_write_cmos_sensor(0x2316, 0x0CAC);
	HI544_write_cmos_sensor(0x2318, 0x4382);
	HI544_write_cmos_sensor(0x231a, 0x0CAE);
	HI544_write_cmos_sensor(0x231c, 0x3FD8);
	HI544_write_cmos_sensor(0x231e, 0x4A82);
	HI544_write_cmos_sensor(0x2320, 0x0CAC);
	HI544_write_cmos_sensor(0x2322, 0x3FF5);
	HI544_write_cmos_sensor(0x2324, 0x0B00);
	HI544_write_cmos_sensor(0x2326, 0x7302);
	HI544_write_cmos_sensor(0x2328, 0x0002);
	HI544_write_cmos_sensor(0x232a, 0x069A);
	HI544_write_cmos_sensor(0x232c, 0x0C1F);
	HI544_write_cmos_sensor(0x232e, 0x0403);
	HI544_write_cmos_sensor(0x2330, 0x0C05);
	HI544_write_cmos_sensor(0x2332, 0x0001);
	HI544_write_cmos_sensor(0x2334, 0x0C01);
	HI544_write_cmos_sensor(0x2336, 0x0003);
	HI544_write_cmos_sensor(0x2338, 0x0C03);
	HI544_write_cmos_sensor(0x233a, 0x000B);
	HI544_write_cmos_sensor(0x233c, 0x0C33);
	HI544_write_cmos_sensor(0x233e, 0x0003);
	HI544_write_cmos_sensor(0x2340, 0x0C03);
	HI544_write_cmos_sensor(0x2342, 0x0653);
	HI544_write_cmos_sensor(0x2344, 0x0C03);
	HI544_write_cmos_sensor(0x2346, 0x065B);
	HI544_write_cmos_sensor(0x2348, 0x0C13);
	HI544_write_cmos_sensor(0x234a, 0x065F);
	HI544_write_cmos_sensor(0x234c, 0x0C43);
	HI544_write_cmos_sensor(0x234e, 0x0657);
	HI544_write_cmos_sensor(0x2350, 0x0C03);
	HI544_write_cmos_sensor(0x2352, 0x0653);
	HI544_write_cmos_sensor(0x2354, 0x0C03);
	HI544_write_cmos_sensor(0x2356, 0x0643);
	HI544_write_cmos_sensor(0x2358, 0x0C0F);
	HI544_write_cmos_sensor(0x235a, 0x067D);
	HI544_write_cmos_sensor(0x235c, 0x0C01);
	HI544_write_cmos_sensor(0x235e, 0x077F);
	HI544_write_cmos_sensor(0x2360, 0x0C01);
	HI544_write_cmos_sensor(0x2362, 0x0677);
	HI544_write_cmos_sensor(0x2364, 0x0C01);
	HI544_write_cmos_sensor(0x2366, 0x0673);
	HI544_write_cmos_sensor(0x2368, 0x0C67);
	HI544_write_cmos_sensor(0x236a, 0x0677);
	HI544_write_cmos_sensor(0x236c, 0x0C03);
	HI544_write_cmos_sensor(0x236e, 0x077D);
	HI544_write_cmos_sensor(0x2370, 0x0C19);
	HI544_write_cmos_sensor(0x2372, 0x0013);
	HI544_write_cmos_sensor(0x2374, 0x0C27);
	HI544_write_cmos_sensor(0x2376, 0x0003);
	HI544_write_cmos_sensor(0x2378, 0x0C45);
	HI544_write_cmos_sensor(0x237a, 0x0675);
	HI544_write_cmos_sensor(0x237c, 0x0C01);
	HI544_write_cmos_sensor(0x237e, 0x0671);
	HI544_write_cmos_sensor(0x2380, 0x4392);
	HI544_write_cmos_sensor(0x2382, 0x7004);
	HI544_write_cmos_sensor(0x2384, 0x430F);
	HI544_write_cmos_sensor(0x2386, 0x9382);
	HI544_write_cmos_sensor(0x2388, 0x80BC);
	HI544_write_cmos_sensor(0x238a, 0x2001);
	HI544_write_cmos_sensor(0x238c, 0x431F);
	HI544_write_cmos_sensor(0x238e, 0x4F82);
	HI544_write_cmos_sensor(0x2390, 0x80BC);
	HI544_write_cmos_sensor(0x2392, 0x930F);
	HI544_write_cmos_sensor(0x2394, 0x2473);
	HI544_write_cmos_sensor(0x2396, 0x0B00);
	HI544_write_cmos_sensor(0x2398, 0x7302);
	HI544_write_cmos_sensor(0x239a, 0x033A);
	HI544_write_cmos_sensor(0x239c, 0x0675);
	HI544_write_cmos_sensor(0x239e, 0x0C02);
	HI544_write_cmos_sensor(0x23a0, 0x0339);
	HI544_write_cmos_sensor(0x23a2, 0xAE0C);
	HI544_write_cmos_sensor(0x23a4, 0x0C01);
	HI544_write_cmos_sensor(0x23a6, 0x003C);
	HI544_write_cmos_sensor(0x23a8, 0x0C01);
	HI544_write_cmos_sensor(0x23aa, 0x0004);
	HI544_write_cmos_sensor(0x23ac, 0x0C01);
	HI544_write_cmos_sensor(0x23ae, 0x0642);
	HI544_write_cmos_sensor(0x23b0, 0x0B00);
	HI544_write_cmos_sensor(0x23b2, 0x7302);
	HI544_write_cmos_sensor(0x23b4, 0x0386);
	HI544_write_cmos_sensor(0x23b6, 0x0643);
	HI544_write_cmos_sensor(0x23b8, 0x0C05);
	HI544_write_cmos_sensor(0x23ba, 0x0001);
	HI544_write_cmos_sensor(0x23bc, 0x0C01);
	HI544_write_cmos_sensor(0x23be, 0x0003);
	HI544_write_cmos_sensor(0x23c0, 0x0C03);
	HI544_write_cmos_sensor(0x23c2, 0x000B);
	HI544_write_cmos_sensor(0x23c4, 0x0C33);
	HI544_write_cmos_sensor(0x23c6, 0x0003);
	HI544_write_cmos_sensor(0x23c8, 0x0C03);
	HI544_write_cmos_sensor(0x23ca, 0x0653);
	HI544_write_cmos_sensor(0x23cc, 0x0C03);
	HI544_write_cmos_sensor(0x23ce, 0x065B);
	HI544_write_cmos_sensor(0x23d0, 0x0C13);
	HI544_write_cmos_sensor(0x23d2, 0x065F);
	HI544_write_cmos_sensor(0x23d4, 0x0C43);
	HI544_write_cmos_sensor(0x23d6, 0x0657);
	HI544_write_cmos_sensor(0x23d8, 0x0C03);
	HI544_write_cmos_sensor(0x23da, 0x0653);
	HI544_write_cmos_sensor(0x23dc, 0x0C03);
	HI544_write_cmos_sensor(0x23de, 0x0643);
	HI544_write_cmos_sensor(0x23e0, 0x0C0F);
	HI544_write_cmos_sensor(0x23e2, 0x067D);
	HI544_write_cmos_sensor(0x23e4, 0x0C01);
	HI544_write_cmos_sensor(0x23e6, 0x077F);
	HI544_write_cmos_sensor(0x23e8, 0x0C01);
	HI544_write_cmos_sensor(0x23ea, 0x0677);
	HI544_write_cmos_sensor(0x23ec, 0x0C01);
	HI544_write_cmos_sensor(0x23ee, 0x0673);
	HI544_write_cmos_sensor(0x23f0, 0x0C67);
	HI544_write_cmos_sensor(0x23f2, 0x0677);
	HI544_write_cmos_sensor(0x23f4, 0x0C03);
	HI544_write_cmos_sensor(0x23f6, 0x077D);
	HI544_write_cmos_sensor(0x23f8, 0x0C19);
	HI544_write_cmos_sensor(0x23fa, 0x0013);
	HI544_write_cmos_sensor(0x23fc, 0x0C27);
	HI544_write_cmos_sensor(0x23fe, 0x0003);
	HI544_write_cmos_sensor(0x2400, 0x0C45);
	HI544_write_cmos_sensor(0x2402, 0x0675);
	HI544_write_cmos_sensor(0x2404, 0x0C01);
	HI544_write_cmos_sensor(0x2406, 0x0671);
	HI544_write_cmos_sensor(0x2408, 0x12B0);
	HI544_write_cmos_sensor(0x240a, 0xF6F2);
	HI544_write_cmos_sensor(0x240c, 0x930F);
	HI544_write_cmos_sensor(0x240e, 0x2405);
	HI544_write_cmos_sensor(0x2410, 0x4292);
	HI544_write_cmos_sensor(0x2412, 0x8096);
	HI544_write_cmos_sensor(0x2414, 0x809E);
	HI544_write_cmos_sensor(0x2416, 0x4382);
	HI544_write_cmos_sensor(0x2418, 0x8096);
	HI544_write_cmos_sensor(0x241a, 0x9382);
	HI544_write_cmos_sensor(0x241c, 0x80BC);
	HI544_write_cmos_sensor(0x241e, 0x241E);
	HI544_write_cmos_sensor(0x2420, 0x0B00);
	HI544_write_cmos_sensor(0x2422, 0x7302);
	HI544_write_cmos_sensor(0x2424, 0x069E);
	HI544_write_cmos_sensor(0x2426, 0x0675);
	HI544_write_cmos_sensor(0x2428, 0x0C02);
	HI544_write_cmos_sensor(0x242a, 0x0339);
	HI544_write_cmos_sensor(0x242c, 0xAE0C);
	HI544_write_cmos_sensor(0x242e, 0x0C01);
	HI544_write_cmos_sensor(0x2430, 0x003C);
	HI544_write_cmos_sensor(0x2432, 0x0C01);
	HI544_write_cmos_sensor(0x2434, 0x0004);
	HI544_write_cmos_sensor(0x2436, 0x0C01);
	HI544_write_cmos_sensor(0x2438, 0x0642);
	HI544_write_cmos_sensor(0x243a, 0x0C01);
	HI544_write_cmos_sensor(0x243c, 0x06A1);
	HI544_write_cmos_sensor(0x243e, 0x0C03);
	HI544_write_cmos_sensor(0x2440, 0x06A0);
	HI544_write_cmos_sensor(0x2442, 0x9382);
	HI544_write_cmos_sensor(0x2444, 0x80D0);
	HI544_write_cmos_sensor(0x2446, 0x2004);
	HI544_write_cmos_sensor(0x2448, 0x930F);
	HI544_write_cmos_sensor(0x244a, 0x26BD);
	HI544_write_cmos_sensor(0x244c, 0x4030);
	HI544_write_cmos_sensor(0x244e, 0xF18C);
	HI544_write_cmos_sensor(0x2450, 0x43C2);
	HI544_write_cmos_sensor(0x2452, 0x0A80);
	HI544_write_cmos_sensor(0x2454, 0x0B00);
	HI544_write_cmos_sensor(0x2456, 0x7302);
	HI544_write_cmos_sensor(0x2458, 0xFFF0);
	HI544_write_cmos_sensor(0x245a, 0x3EB5);
	HI544_write_cmos_sensor(0x245c, 0x0B00);
	HI544_write_cmos_sensor(0x245e, 0x7302);
	HI544_write_cmos_sensor(0x2460, 0x069E);
	HI544_write_cmos_sensor(0x2462, 0x0675);
	HI544_write_cmos_sensor(0x2464, 0x0C02);
	HI544_write_cmos_sensor(0x2466, 0x0301);
	HI544_write_cmos_sensor(0x2468, 0xAE0C);
	HI544_write_cmos_sensor(0x246a, 0x0C01);
	HI544_write_cmos_sensor(0x246c, 0x0004);
	HI544_write_cmos_sensor(0x246e, 0x0C03);
	HI544_write_cmos_sensor(0x2470, 0x0642);
	HI544_write_cmos_sensor(0x2472, 0x0C01);
	HI544_write_cmos_sensor(0x2474, 0x06A1);
	HI544_write_cmos_sensor(0x2476, 0x0C03);
	HI544_write_cmos_sensor(0x2478, 0x06A0);
	HI544_write_cmos_sensor(0x247a, 0x3FE3);
	HI544_write_cmos_sensor(0x247c, 0x0B00);
	HI544_write_cmos_sensor(0x247e, 0x7302);
	HI544_write_cmos_sensor(0x2480, 0x033A);
	HI544_write_cmos_sensor(0x2482, 0x0675);
	HI544_write_cmos_sensor(0x2484, 0x0C02);
	HI544_write_cmos_sensor(0x2486, 0x0301);
	HI544_write_cmos_sensor(0x2488, 0xAE0C);
	HI544_write_cmos_sensor(0x248a, 0x0C01);
	HI544_write_cmos_sensor(0x248c, 0x0004);
	HI544_write_cmos_sensor(0x248e, 0x0C03);
	HI544_write_cmos_sensor(0x2490, 0x0642);
	HI544_write_cmos_sensor(0x2492, 0x3F8E);
	HI544_write_cmos_sensor(0x2494, 0x0B00);
	HI544_write_cmos_sensor(0x2496, 0x7302);
	HI544_write_cmos_sensor(0x2498, 0x0002);
	HI544_write_cmos_sensor(0x249a, 0x069A);
	HI544_write_cmos_sensor(0x249c, 0x0C1F);
	HI544_write_cmos_sensor(0x249e, 0x0402);
	HI544_write_cmos_sensor(0x24a0, 0x0C05);
	HI544_write_cmos_sensor(0x24a2, 0x0001);
	HI544_write_cmos_sensor(0x24a4, 0x0C01);
	HI544_write_cmos_sensor(0x24a6, 0x0003);
	HI544_write_cmos_sensor(0x24a8, 0x0C03);
	HI544_write_cmos_sensor(0x24aa, 0x000B);
	HI544_write_cmos_sensor(0x24ac, 0x0C33);
	HI544_write_cmos_sensor(0x24ae, 0x0003);
	HI544_write_cmos_sensor(0x24b0, 0x0C03);
	HI544_write_cmos_sensor(0x24b2, 0x0653);
	HI544_write_cmos_sensor(0x24b4, 0x0C03);
	HI544_write_cmos_sensor(0x24b6, 0x065B);
	HI544_write_cmos_sensor(0x24b8, 0x0C13);
	HI544_write_cmos_sensor(0x24ba, 0x065F);
	HI544_write_cmos_sensor(0x24bc, 0x0C43);
	HI544_write_cmos_sensor(0x24be, 0x0657);
	HI544_write_cmos_sensor(0x24c0, 0x0C03);
	HI544_write_cmos_sensor(0x24c2, 0x0653);
	HI544_write_cmos_sensor(0x24c4, 0x0C03);
	HI544_write_cmos_sensor(0x24c6, 0x0643);
	HI544_write_cmos_sensor(0x24c8, 0x0C0F);
	HI544_write_cmos_sensor(0x24ca, 0x077D);
	HI544_write_cmos_sensor(0x24cc, 0x0C01);
	HI544_write_cmos_sensor(0x24ce, 0x067F);
	HI544_write_cmos_sensor(0x24d0, 0x0C01);
	HI544_write_cmos_sensor(0x24d2, 0x0677);
	HI544_write_cmos_sensor(0x24d4, 0x0C01);
	HI544_write_cmos_sensor(0x24d6, 0x0673);
	HI544_write_cmos_sensor(0x24d8, 0x0C5F);
	HI544_write_cmos_sensor(0x24da, 0x0663);
	HI544_write_cmos_sensor(0x24dc, 0x0C6F);
	HI544_write_cmos_sensor(0x24de, 0x0667);
	HI544_write_cmos_sensor(0x24e0, 0x0C01);
	HI544_write_cmos_sensor(0x24e2, 0x0677);
	HI544_write_cmos_sensor(0x24e4, 0x0C01);
	HI544_write_cmos_sensor(0x24e6, 0x077D);
	HI544_write_cmos_sensor(0x24e8, 0x0C33);
	HI544_write_cmos_sensor(0x24ea, 0x0013);
	HI544_write_cmos_sensor(0x24ec, 0x0C27);
	HI544_write_cmos_sensor(0x24ee, 0x0003);
	HI544_write_cmos_sensor(0x24f0, 0x0C4F);
	HI544_write_cmos_sensor(0x24f2, 0x0675);
	HI544_write_cmos_sensor(0x24f4, 0x0C01);
	HI544_write_cmos_sensor(0x24f6, 0x0671);
	HI544_write_cmos_sensor(0x24f8, 0x0CFF);
	HI544_write_cmos_sensor(0x24fa, 0x0C78);
	HI544_write_cmos_sensor(0x24fc, 0x0661);
	HI544_write_cmos_sensor(0x24fe, 0x4392);
	HI544_write_cmos_sensor(0x2500, 0x7004);
	HI544_write_cmos_sensor(0x2502, 0x430F);
	HI544_write_cmos_sensor(0x2504, 0x9382);
	HI544_write_cmos_sensor(0x2506, 0x80BC);
	HI544_write_cmos_sensor(0x2508, 0x2001);
	HI544_write_cmos_sensor(0x250a, 0x431F);
	HI544_write_cmos_sensor(0x250c, 0x4F82);
	HI544_write_cmos_sensor(0x250e, 0x80BC);
	HI544_write_cmos_sensor(0x2510, 0x12B0);
	HI544_write_cmos_sensor(0x2512, 0xF6F2);
	HI544_write_cmos_sensor(0x2514, 0x930F);
	HI544_write_cmos_sensor(0x2516, 0x2405);
	HI544_write_cmos_sensor(0x2518, 0x4292);
	HI544_write_cmos_sensor(0x251a, 0x8096);
	HI544_write_cmos_sensor(0x251c, 0x809E);
	HI544_write_cmos_sensor(0x251e, 0x4382);
	HI544_write_cmos_sensor(0x2520, 0x8096);
	HI544_write_cmos_sensor(0x2522, 0x9382);
	HI544_write_cmos_sensor(0x2524, 0x80BC);
	HI544_write_cmos_sensor(0x2526, 0x2019);
	HI544_write_cmos_sensor(0x2528, 0x0B00);
	HI544_write_cmos_sensor(0x252a, 0x7302);
	HI544_write_cmos_sensor(0x252c, 0x0562);
	HI544_write_cmos_sensor(0x252e, 0x0665);
	HI544_write_cmos_sensor(0x2530, 0x0C02);
	HI544_write_cmos_sensor(0x2532, 0x0301);
	HI544_write_cmos_sensor(0x2534, 0xA60C);
	HI544_write_cmos_sensor(0x2536, 0x0204);
	HI544_write_cmos_sensor(0x2538, 0xAE0C);
	HI544_write_cmos_sensor(0x253a, 0x0C03);
	HI544_write_cmos_sensor(0x253c, 0x0642);
	HI544_write_cmos_sensor(0x253e, 0x0C13);
	HI544_write_cmos_sensor(0x2540, 0x06A1);
	HI544_write_cmos_sensor(0x2542, 0x0C03);
	HI544_write_cmos_sensor(0x2544, 0x06A0);
	HI544_write_cmos_sensor(0x2546, 0x9382);
	HI544_write_cmos_sensor(0x2548, 0x80D0);
	HI544_write_cmos_sensor(0x254a, 0x277E);
	HI544_write_cmos_sensor(0x254c, 0x43C2);
	HI544_write_cmos_sensor(0x254e, 0x0A80);
	HI544_write_cmos_sensor(0x2550, 0x0B00);
	HI544_write_cmos_sensor(0x2552, 0x7302);
	HI544_write_cmos_sensor(0x2554, 0xFFF0);
	HI544_write_cmos_sensor(0x2556, 0x4030);
	HI544_write_cmos_sensor(0x2558, 0xF1C6);
	HI544_write_cmos_sensor(0x255a, 0x0B00);
	HI544_write_cmos_sensor(0x255c, 0x7302);
	HI544_write_cmos_sensor(0x255e, 0x0562);
	HI544_write_cmos_sensor(0x2560, 0x0665);
	HI544_write_cmos_sensor(0x2562, 0x0C02);
	HI544_write_cmos_sensor(0x2564, 0x0339);
	HI544_write_cmos_sensor(0x2566, 0xA60C);
	HI544_write_cmos_sensor(0x2568, 0x023C);
	HI544_write_cmos_sensor(0x256a, 0xAE0C);
	HI544_write_cmos_sensor(0x256c, 0x0C01);
	HI544_write_cmos_sensor(0x256e, 0x0004);
	HI544_write_cmos_sensor(0x2570, 0x0C01);
	HI544_write_cmos_sensor(0x2572, 0x0642);
	HI544_write_cmos_sensor(0x2574, 0x0C13);
	HI544_write_cmos_sensor(0x2576, 0x06A1);
	HI544_write_cmos_sensor(0x2578, 0x0C03);
	HI544_write_cmos_sensor(0x257a, 0x06A0);
	HI544_write_cmos_sensor(0x257c, 0x9382);
	HI544_write_cmos_sensor(0x257e, 0x80D0);
	HI544_write_cmos_sensor(0x2580, 0x2763);
	HI544_write_cmos_sensor(0x2582, 0x43C2);
	HI544_write_cmos_sensor(0x2584, 0x0A80);
	HI544_write_cmos_sensor(0x2586, 0x0B00);
	HI544_write_cmos_sensor(0x2588, 0x7302);
	HI544_write_cmos_sensor(0x258a, 0xFFF0);
	HI544_write_cmos_sensor(0x258c, 0x4030);
	HI544_write_cmos_sensor(0x258e, 0xF1C6);
	HI544_write_cmos_sensor(0x2590, 0xB3E2);
	HI544_write_cmos_sensor(0x2592, 0x00B4);
	HI544_write_cmos_sensor(0x2594, 0x2002);
	HI544_write_cmos_sensor(0x2596, 0x4030);
	HI544_write_cmos_sensor(0x2598, 0xF16A);
	HI544_write_cmos_sensor(0x259a, 0x4316);
	HI544_write_cmos_sensor(0x259c, 0x4030);
	HI544_write_cmos_sensor(0x259e, 0xF16A);
	HI544_write_cmos_sensor(0x25a0, 0x4392);
	HI544_write_cmos_sensor(0x25a2, 0x760E);
	HI544_write_cmos_sensor(0x25a4, 0x425F);
	HI544_write_cmos_sensor(0x25a6, 0x0118);
	HI544_write_cmos_sensor(0x25a8, 0xF37F);
	HI544_write_cmos_sensor(0x25aa, 0x930F);
	HI544_write_cmos_sensor(0x25ac, 0x2005);
	HI544_write_cmos_sensor(0x25ae, 0x43C2);
	HI544_write_cmos_sensor(0x25b0, 0x0A80);
	HI544_write_cmos_sensor(0x25b2, 0x0B00);
	HI544_write_cmos_sensor(0x25b4, 0x7302);
	HI544_write_cmos_sensor(0x25b6, 0xFFF0);
	HI544_write_cmos_sensor(0x25b8, 0x9382);
	HI544_write_cmos_sensor(0x25ba, 0x760C);
	HI544_write_cmos_sensor(0x25bc, 0x2002);
	HI544_write_cmos_sensor(0x25be, 0x0C64);
	HI544_write_cmos_sensor(0x25c0, 0x3FF1);
	HI544_write_cmos_sensor(0x25c2, 0x4F82);
	HI544_write_cmos_sensor(0x25c4, 0x809A);
	HI544_write_cmos_sensor(0x25c6, 0x421F);
	HI544_write_cmos_sensor(0x25c8, 0x760A);
	HI544_write_cmos_sensor(0x25ca, 0x932F);
	HI544_write_cmos_sensor(0x25cc, 0x2013);
	HI544_write_cmos_sensor(0x25ce, 0x4292);
	HI544_write_cmos_sensor(0x25d0, 0x018A);
	HI544_write_cmos_sensor(0x25d2, 0x80B8);
	HI544_write_cmos_sensor(0x25d4, 0x12B0);
	HI544_write_cmos_sensor(0x25d6, 0xFE26);
	HI544_write_cmos_sensor(0x25d8, 0x40B2);
	HI544_write_cmos_sensor(0x25da, 0x0005);
	HI544_write_cmos_sensor(0x25dc, 0x7600);
	HI544_write_cmos_sensor(0x25de, 0x4382);
	HI544_write_cmos_sensor(0x25e0, 0x7602);
	HI544_write_cmos_sensor(0x25e2, 0x0262);
	HI544_write_cmos_sensor(0x25e4, 0x0000);
	HI544_write_cmos_sensor(0x25e6, 0x0222);
	HI544_write_cmos_sensor(0x25e8, 0x0000);
	HI544_write_cmos_sensor(0x25ea, 0x0262);
	HI544_write_cmos_sensor(0x25ec, 0x0000);
	HI544_write_cmos_sensor(0x25ee, 0x0260);
	HI544_write_cmos_sensor(0x25f0, 0x0000);
	HI544_write_cmos_sensor(0x25f2, 0x3FD6);
	HI544_write_cmos_sensor(0x25f4, 0x903F);
	HI544_write_cmos_sensor(0x25f6, 0x0003);
	HI544_write_cmos_sensor(0x25f8, 0x285B);
	HI544_write_cmos_sensor(0x25fa, 0x903F);
	HI544_write_cmos_sensor(0x25fc, 0x0102);
	HI544_write_cmos_sensor(0x25fe, 0x204E);
	HI544_write_cmos_sensor(0x2600, 0x425F);
	HI544_write_cmos_sensor(0x2602, 0x0186);
	HI544_write_cmos_sensor(0x2604, 0x4F4C);
	HI544_write_cmos_sensor(0x2606, 0x93D2);
	HI544_write_cmos_sensor(0x2608, 0x018F);
	HI544_write_cmos_sensor(0x260a, 0x2446);
	HI544_write_cmos_sensor(0x260c, 0x425F);
	HI544_write_cmos_sensor(0x260e, 0x018F);
	HI544_write_cmos_sensor(0x2610, 0x4F4D);
	HI544_write_cmos_sensor(0x2612, 0x4308);
	HI544_write_cmos_sensor(0x2614, 0x421B);
	HI544_write_cmos_sensor(0x2616, 0x80B8);
	HI544_write_cmos_sensor(0x2618, 0x431F);
	HI544_write_cmos_sensor(0x261a, 0x480E);
	HI544_write_cmos_sensor(0x261c, 0x930E);
	HI544_write_cmos_sensor(0x261e, 0x2403);
	HI544_write_cmos_sensor(0x2620, 0x5F0F);
	HI544_write_cmos_sensor(0x2622, 0x831E);
	HI544_write_cmos_sensor(0x2624, 0x23FD);
	HI544_write_cmos_sensor(0x2626, 0xFC0F);
	HI544_write_cmos_sensor(0x2628, 0x242F);
	HI544_write_cmos_sensor(0x262a, 0x430F);
	HI544_write_cmos_sensor(0x262c, 0x9D0F);
	HI544_write_cmos_sensor(0x262e, 0x2C2C);
	HI544_write_cmos_sensor(0x2630, 0x4B0E);
	HI544_write_cmos_sensor(0x2632, 0x4E82);
	HI544_write_cmos_sensor(0x2634, 0x7600);
	HI544_write_cmos_sensor(0x2636, 0x4882);
	HI544_write_cmos_sensor(0x2638, 0x7602);
	HI544_write_cmos_sensor(0x263a, 0x4C82);
	HI544_write_cmos_sensor(0x263c, 0x7604);
	HI544_write_cmos_sensor(0x263e, 0x0264);
	HI544_write_cmos_sensor(0x2640, 0x0000);
	HI544_write_cmos_sensor(0x2642, 0x0224);
	HI544_write_cmos_sensor(0x2644, 0x0000);
	HI544_write_cmos_sensor(0x2646, 0x0264);
	HI544_write_cmos_sensor(0x2648, 0x0000);
	HI544_write_cmos_sensor(0x264a, 0x0260);
	HI544_write_cmos_sensor(0x264c, 0x0000);
	HI544_write_cmos_sensor(0x264e, 0x0268);
	HI544_write_cmos_sensor(0x2650, 0x0000);
	HI544_write_cmos_sensor(0x2652, 0x0C18);
	HI544_write_cmos_sensor(0x2654, 0x02E8);
	HI544_write_cmos_sensor(0x2656, 0x0000);
	HI544_write_cmos_sensor(0x2658, 0x0C30);
	HI544_write_cmos_sensor(0x265a, 0x02A8);
	HI544_write_cmos_sensor(0x265c, 0x0000);
	HI544_write_cmos_sensor(0x265e, 0x0C30);
	HI544_write_cmos_sensor(0x2660, 0x0C30);
	HI544_write_cmos_sensor(0x2662, 0x0C30);
	HI544_write_cmos_sensor(0x2664, 0x0C30);
	HI544_write_cmos_sensor(0x2666, 0x0C30);
	HI544_write_cmos_sensor(0x2668, 0x0C30);
	HI544_write_cmos_sensor(0x266a, 0x0C30);
	HI544_write_cmos_sensor(0x266c, 0x0C30);
	HI544_write_cmos_sensor(0x266e, 0x0C00);
	HI544_write_cmos_sensor(0x2670, 0x02E8);
	HI544_write_cmos_sensor(0x2672, 0x0000);
	HI544_write_cmos_sensor(0x2674, 0x0C30);
	HI544_write_cmos_sensor(0x2676, 0x0268);
	HI544_write_cmos_sensor(0x2678, 0x0000);
	HI544_write_cmos_sensor(0x267a, 0x0C18);
	HI544_write_cmos_sensor(0x267c, 0x0260);
	HI544_write_cmos_sensor(0x267e, 0x0000);
	HI544_write_cmos_sensor(0x2680, 0x0C18);
	HI544_write_cmos_sensor(0x2682, 0x531F);
	HI544_write_cmos_sensor(0x2684, 0x9D0F);
	HI544_write_cmos_sensor(0x2686, 0x2BD5);
	HI544_write_cmos_sensor(0x2688, 0x5318);
	HI544_write_cmos_sensor(0x268a, 0x9238);
	HI544_write_cmos_sensor(0x268c, 0x2BC5);
	HI544_write_cmos_sensor(0x268e, 0x0260);
	HI544_write_cmos_sensor(0x2690, 0x0000);
	HI544_write_cmos_sensor(0x2692, 0x5392);
	HI544_write_cmos_sensor(0x2694, 0x80B8);
	HI544_write_cmos_sensor(0x2696, 0x3F84);
	HI544_write_cmos_sensor(0x2698, 0x432D);
	HI544_write_cmos_sensor(0x269a, 0x3FBB);
	HI544_write_cmos_sensor(0x269c, 0x903F);
	HI544_write_cmos_sensor(0x269e, 0x0201);
	HI544_write_cmos_sensor(0x26a0, 0x237F);
	HI544_write_cmos_sensor(0x26a2, 0x5392);
	HI544_write_cmos_sensor(0x26a4, 0x80B8);
	HI544_write_cmos_sensor(0x26a6, 0x421F);
	HI544_write_cmos_sensor(0x26a8, 0x80B8);
	HI544_write_cmos_sensor(0x26aa, 0x12B0);
	HI544_write_cmos_sensor(0x26ac, 0xFE38);
	HI544_write_cmos_sensor(0x26ae, 0x3F78);
	HI544_write_cmos_sensor(0x26b0, 0x931F);
	HI544_write_cmos_sensor(0x26b2, 0x2376);
	HI544_write_cmos_sensor(0x26b4, 0x12B0);
	HI544_write_cmos_sensor(0x26b6, 0xFE26);
	HI544_write_cmos_sensor(0x26b8, 0x4292);
	HI544_write_cmos_sensor(0x26ba, 0x018A);
	HI544_write_cmos_sensor(0x26bc, 0x80B8);
	HI544_write_cmos_sensor(0x26be, 0x3FF3);
	HI544_write_cmos_sensor(0x26c0, 0x4382);
	HI544_write_cmos_sensor(0x26c2, 0x0B88);
	HI544_write_cmos_sensor(0x26c4, 0x0C0A);
	HI544_write_cmos_sensor(0x26c6, 0x4382);
	HI544_write_cmos_sensor(0x26c8, 0x0B8A);
	HI544_write_cmos_sensor(0x26ca, 0x0C0A);
	HI544_write_cmos_sensor(0x26cc, 0x40B2);
	HI544_write_cmos_sensor(0x26ce, 0x000C);
	HI544_write_cmos_sensor(0x26d0, 0x0B8C);
	HI544_write_cmos_sensor(0x26d2, 0x0C0A);
	HI544_write_cmos_sensor(0x26d4, 0x40B2);
	HI544_write_cmos_sensor(0x26d6, 0xB5E1);
	HI544_write_cmos_sensor(0x26d8, 0x0B8E);
	HI544_write_cmos_sensor(0x26da, 0x0C0A);
	HI544_write_cmos_sensor(0x26dc, 0x40B2);
	HI544_write_cmos_sensor(0x26de, 0x641C);
	HI544_write_cmos_sensor(0x26e0, 0x0B92);
	HI544_write_cmos_sensor(0x26e2, 0x0C0A);
	HI544_write_cmos_sensor(0x26e4, 0x43C2);
	HI544_write_cmos_sensor(0x26e6, 0x003D);
	HI544_write_cmos_sensor(0x26e8, 0x4030);
	HI544_write_cmos_sensor(0x26ea, 0xF02C);
	HI544_write_cmos_sensor(0x26ec, 0x5231);
	HI544_write_cmos_sensor(0x26ee, 0x4030);
	HI544_write_cmos_sensor(0x26f0, 0xFE54);
	HI544_write_cmos_sensor(0x26f2, 0xE3B2);
	HI544_write_cmos_sensor(0x26f4, 0x740E);
	HI544_write_cmos_sensor(0x26f6, 0x425F);
	HI544_write_cmos_sensor(0x26f8, 0x0118);
	HI544_write_cmos_sensor(0x26fa, 0xF37F);
	HI544_write_cmos_sensor(0x26fc, 0x4F82);
	HI544_write_cmos_sensor(0x26fe, 0x809A);
	HI544_write_cmos_sensor(0x2700, 0x930F);
	HI544_write_cmos_sensor(0x2702, 0x2005);
	HI544_write_cmos_sensor(0x2704, 0x93C2);
	HI544_write_cmos_sensor(0x2706, 0x0A82);
	HI544_write_cmos_sensor(0x2708, 0x2402);
	HI544_write_cmos_sensor(0x270a, 0x4392);
	HI544_write_cmos_sensor(0x270c, 0x80D0);
	HI544_write_cmos_sensor(0x270e, 0x9382);
	HI544_write_cmos_sensor(0x2710, 0x809A);
	HI544_write_cmos_sensor(0x2712, 0x2002);
	HI544_write_cmos_sensor(0x2714, 0x4392);
	HI544_write_cmos_sensor(0x2716, 0x8070);
	HI544_write_cmos_sensor(0x2718, 0x421F);
	HI544_write_cmos_sensor(0x271a, 0x710E);
	HI544_write_cmos_sensor(0x271c, 0x93A2);
	HI544_write_cmos_sensor(0x271e, 0x7110);
	HI544_write_cmos_sensor(0x2720, 0x2411);
	HI544_write_cmos_sensor(0x2722, 0x9382);
	HI544_write_cmos_sensor(0x2724, 0x710E);
	HI544_write_cmos_sensor(0x2726, 0x240C);
	HI544_write_cmos_sensor(0x2728, 0x5292);
	HI544_write_cmos_sensor(0x272a, 0x80A0);
	HI544_write_cmos_sensor(0x272c, 0x7110);
	HI544_write_cmos_sensor(0x272e, 0x4382);
	HI544_write_cmos_sensor(0x2730, 0x740E);
	HI544_write_cmos_sensor(0x2732, 0x9382);
	HI544_write_cmos_sensor(0x2734, 0x80BA);
	HI544_write_cmos_sensor(0x2736, 0x2402);
	HI544_write_cmos_sensor(0x2738, 0x4392);
	HI544_write_cmos_sensor(0x273a, 0x740E);
	HI544_write_cmos_sensor(0x273c, 0x4392);
	HI544_write_cmos_sensor(0x273e, 0x80BC);
	HI544_write_cmos_sensor(0x2740, 0x430F);
	HI544_write_cmos_sensor(0x2742, 0x4130);
	HI544_write_cmos_sensor(0x2744, 0xF31F);
	HI544_write_cmos_sensor(0x2746, 0x27ED);
	HI544_write_cmos_sensor(0x2748, 0x40B2);
	HI544_write_cmos_sensor(0x274a, 0x0003);
	HI544_write_cmos_sensor(0x274c, 0x7110);
	HI544_write_cmos_sensor(0x274e, 0x431F);
	HI544_write_cmos_sensor(0x2750, 0x4130);
	HI544_write_cmos_sensor(0x2752, 0x4F0E);
	HI544_write_cmos_sensor(0x2754, 0x421D);
	HI544_write_cmos_sensor(0x2756, 0x8070);
	HI544_write_cmos_sensor(0x2758, 0x425F);
	HI544_write_cmos_sensor(0x275a, 0x0118);
	HI544_write_cmos_sensor(0x275c, 0xF37F);
	HI544_write_cmos_sensor(0x275e, 0x903E);
	HI544_write_cmos_sensor(0x2760, 0x0003);
	HI544_write_cmos_sensor(0x2762, 0x2405);
	HI544_write_cmos_sensor(0x2764, 0x931E);
	HI544_write_cmos_sensor(0x2766, 0x2403);
	HI544_write_cmos_sensor(0x2768, 0x0B00);
	HI544_write_cmos_sensor(0x276a, 0x7302);
	HI544_write_cmos_sensor(0x276c, 0x0384);
	HI544_write_cmos_sensor(0x276e, 0x930F);
	HI544_write_cmos_sensor(0x2770, 0x241A);
	HI544_write_cmos_sensor(0x2772, 0x930D);
	HI544_write_cmos_sensor(0x2774, 0x2018);
	HI544_write_cmos_sensor(0x2776, 0x9382);
	HI544_write_cmos_sensor(0x2778, 0x7308);
	HI544_write_cmos_sensor(0x277a, 0x2402);
	HI544_write_cmos_sensor(0x277c, 0x930E);
	HI544_write_cmos_sensor(0x277e, 0x2419);
	HI544_write_cmos_sensor(0x2780, 0x9382);
	HI544_write_cmos_sensor(0x2782, 0x7328);
	HI544_write_cmos_sensor(0x2784, 0x2402);
	HI544_write_cmos_sensor(0x2786, 0x931E);
	HI544_write_cmos_sensor(0x2788, 0x2414);
	HI544_write_cmos_sensor(0x278a, 0x9382);
	HI544_write_cmos_sensor(0x278c, 0x710E);
	HI544_write_cmos_sensor(0x278e, 0x2402);
	HI544_write_cmos_sensor(0x2790, 0x932E);
	HI544_write_cmos_sensor(0x2792, 0x240F);
	HI544_write_cmos_sensor(0x2794, 0x9382);
	HI544_write_cmos_sensor(0x2796, 0x7114);
	HI544_write_cmos_sensor(0x2798, 0x2402);
	HI544_write_cmos_sensor(0x279a, 0x922E);
	HI544_write_cmos_sensor(0x279c, 0x240A);
	HI544_write_cmos_sensor(0x279e, 0x903E);
	HI544_write_cmos_sensor(0x27a0, 0x0003);
	HI544_write_cmos_sensor(0x27a2, 0x23DA);
	HI544_write_cmos_sensor(0x27a4, 0x3C06);
	HI544_write_cmos_sensor(0x27a6, 0x43C2);
	HI544_write_cmos_sensor(0x27a8, 0x0A80);
	HI544_write_cmos_sensor(0x27aa, 0x0B00);
	HI544_write_cmos_sensor(0x27ac, 0x7302);
	HI544_write_cmos_sensor(0x27ae, 0xFFF0);
	HI544_write_cmos_sensor(0x27b0, 0x3FD3);
	HI544_write_cmos_sensor(0x27b2, 0x4F82);
	HI544_write_cmos_sensor(0x27b4, 0x809A);
	HI544_write_cmos_sensor(0x27b6, 0x431F);
	HI544_write_cmos_sensor(0x27b8, 0x4130);
	HI544_write_cmos_sensor(0x27ba, 0x120B);
	HI544_write_cmos_sensor(0x27bc, 0x120A);
	HI544_write_cmos_sensor(0x27be, 0x1209);
	HI544_write_cmos_sensor(0x27c0, 0x1208);
	HI544_write_cmos_sensor(0x27c2, 0x1207);
	HI544_write_cmos_sensor(0x27c4, 0x1206);
	HI544_write_cmos_sensor(0x27c6, 0x1205);
	HI544_write_cmos_sensor(0x27c8, 0x1204);
	HI544_write_cmos_sensor(0x27ca, 0x8221);
	HI544_write_cmos_sensor(0x27cc, 0x403B);
	HI544_write_cmos_sensor(0x27ce, 0x0016);
	HI544_write_cmos_sensor(0x27d0, 0x510B);
	HI544_write_cmos_sensor(0x27d2, 0x4F08);
	HI544_write_cmos_sensor(0x27d4, 0x4E09);
	HI544_write_cmos_sensor(0x27d6, 0x4BA1);
	HI544_write_cmos_sensor(0x27d8, 0x0000);
	HI544_write_cmos_sensor(0x27da, 0x4B1A);
	HI544_write_cmos_sensor(0x27dc, 0x0002);
	HI544_write_cmos_sensor(0x27de, 0x4B91);
	HI544_write_cmos_sensor(0x27e0, 0x0004);
	HI544_write_cmos_sensor(0x27e2, 0x0002);
	HI544_write_cmos_sensor(0x27e4, 0x4304);
	HI544_write_cmos_sensor(0x27e6, 0x4305);
	HI544_write_cmos_sensor(0x27e8, 0x4306);
	HI544_write_cmos_sensor(0x27ea, 0x4307);
	HI544_write_cmos_sensor(0x27ec, 0x9382);
	HI544_write_cmos_sensor(0x27ee, 0x80B4);
	HI544_write_cmos_sensor(0x27f0, 0x2425);
	HI544_write_cmos_sensor(0x27f2, 0x438A);
	HI544_write_cmos_sensor(0x27f4, 0x0000);
	HI544_write_cmos_sensor(0x27f6, 0x430B);
	HI544_write_cmos_sensor(0x27f8, 0x4B0F);
	HI544_write_cmos_sensor(0x27fa, 0x5F0F);
	HI544_write_cmos_sensor(0x27fc, 0x5F0F);
	HI544_write_cmos_sensor(0x27fe, 0x580F);
	HI544_write_cmos_sensor(0x2800, 0x4C8F);
	HI544_write_cmos_sensor(0x2802, 0x0000);
	HI544_write_cmos_sensor(0x2804, 0x4D8F);
	HI544_write_cmos_sensor(0x2806, 0x0002);
	HI544_write_cmos_sensor(0x2808, 0x4B0F);
	HI544_write_cmos_sensor(0x280a, 0x5F0F);
	HI544_write_cmos_sensor(0x280c, 0x590F);
	HI544_write_cmos_sensor(0x280e, 0x41AF);
	HI544_write_cmos_sensor(0x2810, 0x0000);
	HI544_write_cmos_sensor(0x2812, 0x531B);
	HI544_write_cmos_sensor(0x2814, 0x923B);
	HI544_write_cmos_sensor(0x2816, 0x2BF0);
	HI544_write_cmos_sensor(0x2818, 0x430B);
	HI544_write_cmos_sensor(0x281a, 0x4B0F);
	HI544_write_cmos_sensor(0x281c, 0x5F0F);
	HI544_write_cmos_sensor(0x281e, 0x5F0F);
	HI544_write_cmos_sensor(0x2820, 0x580F);
	HI544_write_cmos_sensor(0x2822, 0x5F34);
	HI544_write_cmos_sensor(0x2824, 0x6F35);
	HI544_write_cmos_sensor(0x2826, 0x4B0F);
	HI544_write_cmos_sensor(0x2828, 0x5F0F);
	HI544_write_cmos_sensor(0x282a, 0x590F);
	HI544_write_cmos_sensor(0x282c, 0x4F2E);
	HI544_write_cmos_sensor(0x282e, 0x430F);
	HI544_write_cmos_sensor(0x2830, 0x5E06);
	HI544_write_cmos_sensor(0x2832, 0x6F07);
	HI544_write_cmos_sensor(0x2834, 0x531B);
	HI544_write_cmos_sensor(0x2836, 0x923B);
	HI544_write_cmos_sensor(0x2838, 0x2BF0);
	HI544_write_cmos_sensor(0x283a, 0x3C18);
	HI544_write_cmos_sensor(0x283c, 0x4A2E);
	HI544_write_cmos_sensor(0x283e, 0x4E0F);
	HI544_write_cmos_sensor(0x2840, 0x5F0F);
	HI544_write_cmos_sensor(0x2842, 0x5F0F);
	HI544_write_cmos_sensor(0x2844, 0x580F);
	HI544_write_cmos_sensor(0x2846, 0x4C8F);
	HI544_write_cmos_sensor(0x2848, 0x0000);
	HI544_write_cmos_sensor(0x284a, 0x4D8F);
	HI544_write_cmos_sensor(0x284c, 0x0002);
	HI544_write_cmos_sensor(0x284e, 0x5E0E);
	HI544_write_cmos_sensor(0x2850, 0x590E);
	HI544_write_cmos_sensor(0x2852, 0x41AE);
	HI544_write_cmos_sensor(0x2854, 0x0000);
	HI544_write_cmos_sensor(0x2856, 0x4A2F);
	HI544_write_cmos_sensor(0x2858, 0x903F);
	HI544_write_cmos_sensor(0x285a, 0x0007);
	HI544_write_cmos_sensor(0x285c, 0x2404);
	HI544_write_cmos_sensor(0x285e, 0x531F);
	HI544_write_cmos_sensor(0x2860, 0x4F8A);
	HI544_write_cmos_sensor(0x2862, 0x0000);
	HI544_write_cmos_sensor(0x2864, 0x3FD9);
	HI544_write_cmos_sensor(0x2866, 0x438A);
	HI544_write_cmos_sensor(0x2868, 0x0000);
	HI544_write_cmos_sensor(0x286a, 0x3FD6);
	HI544_write_cmos_sensor(0x286c, 0x440C);
	HI544_write_cmos_sensor(0x286e, 0x450D);
	HI544_write_cmos_sensor(0x2870, 0x460A);
	HI544_write_cmos_sensor(0x2872, 0x470B);
	HI544_write_cmos_sensor(0x2874, 0x12B0);
	HI544_write_cmos_sensor(0x2876, 0xFEAA);
	HI544_write_cmos_sensor(0x2878, 0x4C08);
	HI544_write_cmos_sensor(0x287a, 0x4D09);
	HI544_write_cmos_sensor(0x287c, 0x4C0E);
	HI544_write_cmos_sensor(0x287e, 0x430F);
	HI544_write_cmos_sensor(0x2880, 0x4E0A);
	HI544_write_cmos_sensor(0x2882, 0x4F0B);
	HI544_write_cmos_sensor(0x2884, 0x460C);
	HI544_write_cmos_sensor(0x2886, 0x470D);
	HI544_write_cmos_sensor(0x2888, 0x12B0);
	HI544_write_cmos_sensor(0x288a, 0xFE6E);
	HI544_write_cmos_sensor(0x288c, 0x8E04);
	HI544_write_cmos_sensor(0x288e, 0x7F05);
	HI544_write_cmos_sensor(0x2890, 0x440E);
	HI544_write_cmos_sensor(0x2892, 0x450F);
	HI544_write_cmos_sensor(0x2894, 0xC312);
	HI544_write_cmos_sensor(0x2896, 0x100F);
	HI544_write_cmos_sensor(0x2898, 0x100E);
	HI544_write_cmos_sensor(0x289a, 0x110F);
	HI544_write_cmos_sensor(0x289c, 0x100E);
	HI544_write_cmos_sensor(0x289e, 0x110F);
	HI544_write_cmos_sensor(0x28a0, 0x100E);
	HI544_write_cmos_sensor(0x28a2, 0x411D);
	HI544_write_cmos_sensor(0x28a4, 0x0002);
	HI544_write_cmos_sensor(0x28a6, 0x4E8D);
	HI544_write_cmos_sensor(0x28a8, 0x0000);
	HI544_write_cmos_sensor(0x28aa, 0x480F);
	HI544_write_cmos_sensor(0x28ac, 0x5221);
	HI544_write_cmos_sensor(0x28ae, 0x4134);
	HI544_write_cmos_sensor(0x28b0, 0x4135);
	HI544_write_cmos_sensor(0x28b2, 0x4136);
	HI544_write_cmos_sensor(0x28b4, 0x4137);
	HI544_write_cmos_sensor(0x28b6, 0x4138);
	HI544_write_cmos_sensor(0x28b8, 0x4139);
	HI544_write_cmos_sensor(0x28ba, 0x413A);
	HI544_write_cmos_sensor(0x28bc, 0x413B);
	HI544_write_cmos_sensor(0x28be, 0x4130);
	HI544_write_cmos_sensor(0x28c0, 0x120A);
	HI544_write_cmos_sensor(0x28c2, 0x4F0D);
	HI544_write_cmos_sensor(0x28c4, 0x4E0C);
	HI544_write_cmos_sensor(0x28c6, 0x425F);
	HI544_write_cmos_sensor(0x28c8, 0x00BA);
	HI544_write_cmos_sensor(0x28ca, 0x4F4A);
	HI544_write_cmos_sensor(0x28cc, 0x503A);
	HI544_write_cmos_sensor(0x28ce, 0x0010);
	HI544_write_cmos_sensor(0x28d0, 0x931D);
	HI544_write_cmos_sensor(0x28d2, 0x242B);
	HI544_write_cmos_sensor(0x28d4, 0x932D);
	HI544_write_cmos_sensor(0x28d6, 0x2421);
	HI544_write_cmos_sensor(0x28d8, 0x903D);
	HI544_write_cmos_sensor(0x28da, 0x0003);
	HI544_write_cmos_sensor(0x28dc, 0x2418);
	HI544_write_cmos_sensor(0x28de, 0x922D);
	HI544_write_cmos_sensor(0x28e0, 0x2413);
	HI544_write_cmos_sensor(0x28e2, 0x903D);
	HI544_write_cmos_sensor(0x28e4, 0x0005);
	HI544_write_cmos_sensor(0x28e6, 0x2407);
	HI544_write_cmos_sensor(0x28e8, 0x903D);
	HI544_write_cmos_sensor(0x28ea, 0x0006);
	HI544_write_cmos_sensor(0x28ec, 0x2028);
	HI544_write_cmos_sensor(0x28ee, 0xC312);
	HI544_write_cmos_sensor(0x28f0, 0x100A);
	HI544_write_cmos_sensor(0x28f2, 0x110A);
	HI544_write_cmos_sensor(0x28f4, 0x3C24);
	HI544_write_cmos_sensor(0x28f6, 0x4A0E);
	HI544_write_cmos_sensor(0x28f8, 0xC312);
	HI544_write_cmos_sensor(0x28fa, 0x100E);
	HI544_write_cmos_sensor(0x28fc, 0x110E);
	HI544_write_cmos_sensor(0x28fe, 0x4E0F);
	HI544_write_cmos_sensor(0x2900, 0x110F);
	HI544_write_cmos_sensor(0x2902, 0x4E0A);
	HI544_write_cmos_sensor(0x2904, 0x5F0A);
	HI544_write_cmos_sensor(0x2906, 0x3C1B);
	HI544_write_cmos_sensor(0x2908, 0xC312);
	HI544_write_cmos_sensor(0x290a, 0x100A);
	HI544_write_cmos_sensor(0x290c, 0x3C18);
	HI544_write_cmos_sensor(0x290e, 0x4A0E);
	HI544_write_cmos_sensor(0x2910, 0xC312);
	HI544_write_cmos_sensor(0x2912, 0x100E);
	HI544_write_cmos_sensor(0x2914, 0x4E0F);
	HI544_write_cmos_sensor(0x2916, 0x110F);
	HI544_write_cmos_sensor(0x2918, 0x3FF3);
	HI544_write_cmos_sensor(0x291a, 0x4A0F);
	HI544_write_cmos_sensor(0x291c, 0xC312);
	HI544_write_cmos_sensor(0x291e, 0x100F);
	HI544_write_cmos_sensor(0x2920, 0x4F0E);
	HI544_write_cmos_sensor(0x2922, 0x110E);
	HI544_write_cmos_sensor(0x2924, 0x4F0A);
	HI544_write_cmos_sensor(0x2926, 0x5E0A);
	HI544_write_cmos_sensor(0x2928, 0x3C0A);
	HI544_write_cmos_sensor(0x292a, 0x4A0F);
	HI544_write_cmos_sensor(0x292c, 0xC312);
	HI544_write_cmos_sensor(0x292e, 0x100F);
	HI544_write_cmos_sensor(0x2930, 0x4F0E);
	HI544_write_cmos_sensor(0x2932, 0x110E);
	HI544_write_cmos_sensor(0x2934, 0x4F0A);
	HI544_write_cmos_sensor(0x2936, 0x5E0A);
	HI544_write_cmos_sensor(0x2938, 0x4E0F);
	HI544_write_cmos_sensor(0x293a, 0x110F);
	HI544_write_cmos_sensor(0x293c, 0x3FE3);
	HI544_write_cmos_sensor(0x293e, 0x12B0);
	HI544_write_cmos_sensor(0x2940, 0xFE58);
	HI544_write_cmos_sensor(0x2942, 0x4E0F);
	HI544_write_cmos_sensor(0x2944, 0xC312);
	HI544_write_cmos_sensor(0x2946, 0x100F);
	HI544_write_cmos_sensor(0x2948, 0x110F);
	HI544_write_cmos_sensor(0x294a, 0x110F);
	HI544_write_cmos_sensor(0x294c, 0x110F);
	HI544_write_cmos_sensor(0x294e, 0x413A);
	HI544_write_cmos_sensor(0x2950, 0x4130);
	HI544_write_cmos_sensor(0x2952, 0x120B);
	HI544_write_cmos_sensor(0x2954, 0x120A);
	HI544_write_cmos_sensor(0x2956, 0x1209);
	HI544_write_cmos_sensor(0x2958, 0x1208);
	HI544_write_cmos_sensor(0x295a, 0x425F);
	HI544_write_cmos_sensor(0x295c, 0x0080);
	HI544_write_cmos_sensor(0x295e, 0xF36F);
	HI544_write_cmos_sensor(0x2960, 0x4F0E);
	HI544_write_cmos_sensor(0x2962, 0xF32E);
	HI544_write_cmos_sensor(0x2964, 0x4E82);
	HI544_write_cmos_sensor(0x2966, 0x80BA);
	HI544_write_cmos_sensor(0x2968, 0xB3D2);
	HI544_write_cmos_sensor(0x296a, 0x0786);
	HI544_write_cmos_sensor(0x296c, 0x2402);
	HI544_write_cmos_sensor(0x296e, 0x4392);
	HI544_write_cmos_sensor(0x2970, 0x7002);
	HI544_write_cmos_sensor(0x2972, 0x4392);
	HI544_write_cmos_sensor(0x2974, 0x80BC);
	HI544_write_cmos_sensor(0x2976, 0x4382);
	HI544_write_cmos_sensor(0x2978, 0x740E);
	HI544_write_cmos_sensor(0x297a, 0x9382);
	HI544_write_cmos_sensor(0x297c, 0x80BA);
	HI544_write_cmos_sensor(0x297e, 0x2402);
	HI544_write_cmos_sensor(0x2980, 0x4392);
	HI544_write_cmos_sensor(0x2982, 0x740E);
	HI544_write_cmos_sensor(0x2984, 0x93C2);
	HI544_write_cmos_sensor(0x2986, 0x00C6);
	HI544_write_cmos_sensor(0x2988, 0x2406);
	HI544_write_cmos_sensor(0x298a, 0xB392);
	HI544_write_cmos_sensor(0x298c, 0x732A);
	HI544_write_cmos_sensor(0x298e, 0x2403);
	HI544_write_cmos_sensor(0x2990, 0xB3D2);
	HI544_write_cmos_sensor(0x2992, 0x00C7);
	HI544_write_cmos_sensor(0x2994, 0x2412);
	HI544_write_cmos_sensor(0x2996, 0x4292);
	HI544_write_cmos_sensor(0x2998, 0x01A8);
	HI544_write_cmos_sensor(0x299a, 0x0688);
	HI544_write_cmos_sensor(0x299c, 0x4292);
	HI544_write_cmos_sensor(0x299e, 0x01AA);
	HI544_write_cmos_sensor(0x29a0, 0x068A);
	HI544_write_cmos_sensor(0x29a2, 0x4292);
	HI544_write_cmos_sensor(0x29a4, 0x01AC);
	HI544_write_cmos_sensor(0x29a6, 0x068C);
	HI544_write_cmos_sensor(0x29a8, 0x4292);
	HI544_write_cmos_sensor(0x29aa, 0x01AE);
	HI544_write_cmos_sensor(0x29ac, 0x068E);
	HI544_write_cmos_sensor(0x29ae, 0x4292);
	HI544_write_cmos_sensor(0x29b0, 0x0190);
	HI544_write_cmos_sensor(0x29b2, 0x0A92);
	HI544_write_cmos_sensor(0x29b4, 0x4292);
	HI544_write_cmos_sensor(0x29b6, 0x0192);
	HI544_write_cmos_sensor(0x29b8, 0x0A94);
	HI544_write_cmos_sensor(0x29ba, 0x430E);
	HI544_write_cmos_sensor(0x29bc, 0x425F);
	HI544_write_cmos_sensor(0x29be, 0x00C7);
	HI544_write_cmos_sensor(0x29c0, 0xF35F);
	HI544_write_cmos_sensor(0x29c2, 0xF37F);
	HI544_write_cmos_sensor(0x29c4, 0xF21F);
	HI544_write_cmos_sensor(0x29c6, 0x732A);
	HI544_write_cmos_sensor(0x29c8, 0x200C);
	HI544_write_cmos_sensor(0x29ca, 0xB3D2);
	HI544_write_cmos_sensor(0x29cc, 0x00C7);
	HI544_write_cmos_sensor(0x29ce, 0x2003);
	HI544_write_cmos_sensor(0x29d0, 0xB392);
	HI544_write_cmos_sensor(0x29d2, 0x80A2);
	HI544_write_cmos_sensor(0x29d4, 0x2006);
	HI544_write_cmos_sensor(0x29d6, 0xB3A2);
	HI544_write_cmos_sensor(0x29d8, 0x732A);
	HI544_write_cmos_sensor(0x29da, 0x2003);
	HI544_write_cmos_sensor(0x29dc, 0x9382);
	HI544_write_cmos_sensor(0x29de, 0x8096);
	HI544_write_cmos_sensor(0x29e0, 0x2401);
	HI544_write_cmos_sensor(0x29e2, 0x431E);
	HI544_write_cmos_sensor(0x29e4, 0x4E82);
	HI544_write_cmos_sensor(0x29e6, 0x80B4);
	HI544_write_cmos_sensor(0x29e8, 0x930E);
	HI544_write_cmos_sensor(0x29ea, 0x2002);
	HI544_write_cmos_sensor(0x29ec, 0x4030);
	HI544_write_cmos_sensor(0x29ee, 0xFDE4);
	HI544_write_cmos_sensor(0x29f0, 0x4382);
	HI544_write_cmos_sensor(0x29f2, 0x80BE);
	HI544_write_cmos_sensor(0x29f4, 0x421F);
	HI544_write_cmos_sensor(0x29f6, 0x732A);
	HI544_write_cmos_sensor(0x29f8, 0xF31F);
	HI544_write_cmos_sensor(0x29fa, 0x4F82);
	HI544_write_cmos_sensor(0x29fc, 0x80A2);
	HI544_write_cmos_sensor(0x29fe, 0x425F);
	HI544_write_cmos_sensor(0x2a00, 0x008C);
	HI544_write_cmos_sensor(0x2a02, 0x4FC2);
	HI544_write_cmos_sensor(0x2a04, 0x8092);
	HI544_write_cmos_sensor(0x2a06, 0x43C2);
	HI544_write_cmos_sensor(0x2a08, 0x8093);
	HI544_write_cmos_sensor(0x2a0a, 0x425F);
	HI544_write_cmos_sensor(0x2a0c, 0x009E);
	HI544_write_cmos_sensor(0x2a0e, 0x4F48);
	HI544_write_cmos_sensor(0x2a10, 0x425F);
	HI544_write_cmos_sensor(0x2a12, 0x009F);
	HI544_write_cmos_sensor(0x2a14, 0xF37F);
	HI544_write_cmos_sensor(0x2a16, 0x5F08);
	HI544_write_cmos_sensor(0x2a18, 0x1108);
	HI544_write_cmos_sensor(0x2a1a, 0x1108);
	HI544_write_cmos_sensor(0x2a1c, 0x425F);
	HI544_write_cmos_sensor(0x2a1e, 0x00B2);
	HI544_write_cmos_sensor(0x2a20, 0x4F49);
	HI544_write_cmos_sensor(0x2a22, 0x425F);
	HI544_write_cmos_sensor(0x2a24, 0x00B3);
	HI544_write_cmos_sensor(0x2a26, 0xF37F);
	HI544_write_cmos_sensor(0x2a28, 0x5F09);
	HI544_write_cmos_sensor(0x2a2a, 0x1109);
	HI544_write_cmos_sensor(0x2a2c, 0x1109);
	HI544_write_cmos_sensor(0x2a2e, 0x425F);
	HI544_write_cmos_sensor(0x2a30, 0x00BA);
	HI544_write_cmos_sensor(0x2a32, 0x4F4C);
	HI544_write_cmos_sensor(0x2a34, 0x407A);
	HI544_write_cmos_sensor(0x2a36, 0x001C);
	HI544_write_cmos_sensor(0x2a38, 0x12B0);
	HI544_write_cmos_sensor(0x2a3a, 0xFE8E);
	HI544_write_cmos_sensor(0x2a3c, 0x934C);
	HI544_write_cmos_sensor(0x2a3e, 0x25BE);
	HI544_write_cmos_sensor(0x2a40, 0x403E);
	HI544_write_cmos_sensor(0x2a42, 0x0080);
	HI544_write_cmos_sensor(0x2a44, 0x4E0F);
	HI544_write_cmos_sensor(0x2a46, 0xF37F);
	HI544_write_cmos_sensor(0x2a48, 0x108F);
	HI544_write_cmos_sensor(0x2a4a, 0xD03F);
	HI544_write_cmos_sensor(0x2a4c, 0x008B);
	HI544_write_cmos_sensor(0x2a4e, 0x4F82);
	HI544_write_cmos_sensor(0x2a50, 0x0B88);
	HI544_write_cmos_sensor(0x2a52, 0x0C0A);
	HI544_write_cmos_sensor(0x2a54, 0x403C);
	HI544_write_cmos_sensor(0x2a56, 0x0813);
	HI544_write_cmos_sensor(0x2a58, 0x403D);
	HI544_write_cmos_sensor(0x2a5a, 0x007F);
	HI544_write_cmos_sensor(0x2a5c, 0x403F);
	HI544_write_cmos_sensor(0x2a5e, 0x00BA);
	HI544_write_cmos_sensor(0x2a60, 0x4F6E);
	HI544_write_cmos_sensor(0x2a62, 0x90FF);
	HI544_write_cmos_sensor(0x2a64, 0x0010);
	HI544_write_cmos_sensor(0x2a66, 0x0000);
	HI544_write_cmos_sensor(0x2a68, 0x2D13);
	HI544_write_cmos_sensor(0x2a6a, 0x403C);
	HI544_write_cmos_sensor(0x2a6c, 0x1005);
	HI544_write_cmos_sensor(0x2a6e, 0x430D);
	HI544_write_cmos_sensor(0x2a70, 0x425E);
	HI544_write_cmos_sensor(0x2a72, 0x00BA);
	HI544_write_cmos_sensor(0x2a74, 0x4E4F);
	HI544_write_cmos_sensor(0x2a76, 0x108F);
	HI544_write_cmos_sensor(0x2a78, 0xDD0F);
	HI544_write_cmos_sensor(0x2a7a, 0x4F82);
	HI544_write_cmos_sensor(0x2a7c, 0x0B90);
	HI544_write_cmos_sensor(0x2a7e, 0x0C0A);
	HI544_write_cmos_sensor(0x2a80, 0x4C82);
	HI544_write_cmos_sensor(0x2a82, 0x0B8A);
	HI544_write_cmos_sensor(0x2a84, 0x0C0A);
	HI544_write_cmos_sensor(0x2a86, 0x425F);
	HI544_write_cmos_sensor(0x2a88, 0x0C87);
	HI544_write_cmos_sensor(0x2a8a, 0x4F4E);
	HI544_write_cmos_sensor(0x2a8c, 0x425F);
	HI544_write_cmos_sensor(0x2a8e, 0x0C88);
	HI544_write_cmos_sensor(0x2a90, 0xF37F);
	HI544_write_cmos_sensor(0x2a92, 0x12B0);
	HI544_write_cmos_sensor(0x2a94, 0xF8C0);
	HI544_write_cmos_sensor(0x2a96, 0x4F82);
	HI544_write_cmos_sensor(0x2a98, 0x0C8C);
	HI544_write_cmos_sensor(0x2a9a, 0x425F);
	HI544_write_cmos_sensor(0x2a9c, 0x0C85);
	HI544_write_cmos_sensor(0x2a9e, 0x4F4E);
	HI544_write_cmos_sensor(0x2aa0, 0x425F);
	HI544_write_cmos_sensor(0x2aa2, 0x0C89);
	HI544_write_cmos_sensor(0x2aa4, 0xF37F);
	HI544_write_cmos_sensor(0x2aa6, 0x12B0);
	HI544_write_cmos_sensor(0x2aa8, 0xF8C0);
	HI544_write_cmos_sensor(0x2aaa, 0x4F82);
	HI544_write_cmos_sensor(0x2aac, 0x0C8A);
	HI544_write_cmos_sensor(0x2aae, 0x425E);
	HI544_write_cmos_sensor(0x2ab0, 0x00B7);
	HI544_write_cmos_sensor(0x2ab2, 0x5E4E);
	HI544_write_cmos_sensor(0x2ab4, 0x4EC2);
	HI544_write_cmos_sensor(0x2ab6, 0x0CB0);
	HI544_write_cmos_sensor(0x2ab8, 0x425F);
	HI544_write_cmos_sensor(0x2aba, 0x00B8);
	HI544_write_cmos_sensor(0x2abc, 0x5F4F);
	HI544_write_cmos_sensor(0x2abe, 0x4FC2);
	HI544_write_cmos_sensor(0x2ac0, 0x0CB1);
	HI544_write_cmos_sensor(0x2ac2, 0x480E);
	HI544_write_cmos_sensor(0x2ac4, 0x5E0E);
	HI544_write_cmos_sensor(0x2ac6, 0x5E0E);
	HI544_write_cmos_sensor(0x2ac8, 0x5E0E);
	HI544_write_cmos_sensor(0x2aca, 0x5E0E);
	HI544_write_cmos_sensor(0x2acc, 0x490F);
	HI544_write_cmos_sensor(0x2ace, 0x5F0F);
	HI544_write_cmos_sensor(0x2ad0, 0x5F0F);
	HI544_write_cmos_sensor(0x2ad2, 0x5F0F);
	HI544_write_cmos_sensor(0x2ad4, 0x5F0F);
	HI544_write_cmos_sensor(0x2ad6, 0x5F0F);
	HI544_write_cmos_sensor(0x2ad8, 0x5F0F);
	HI544_write_cmos_sensor(0x2ada, 0x5F0F);
	HI544_write_cmos_sensor(0x2adc, 0xDF0E);
	HI544_write_cmos_sensor(0x2ade, 0x4E82);
	HI544_write_cmos_sensor(0x2ae0, 0x0A8E);
	HI544_write_cmos_sensor(0x2ae2, 0xB229);
	HI544_write_cmos_sensor(0x2ae4, 0x2401);
	HI544_write_cmos_sensor(0x2ae6, 0x5339);
	HI544_write_cmos_sensor(0x2ae8, 0xB3E2);
	HI544_write_cmos_sensor(0x2aea, 0x0080);
	HI544_write_cmos_sensor(0x2aec, 0x2403);
	HI544_write_cmos_sensor(0x2aee, 0x40F2);
	HI544_write_cmos_sensor(0x2af0, 0x0003);
	HI544_write_cmos_sensor(0x2af2, 0x00B5);
	HI544_write_cmos_sensor(0x2af4, 0x40B2);
	HI544_write_cmos_sensor(0x2af6, 0x1000);
	HI544_write_cmos_sensor(0x2af8, 0x7500);
	HI544_write_cmos_sensor(0x2afa, 0x40B2);
	HI544_write_cmos_sensor(0x2afc, 0x1001);
	HI544_write_cmos_sensor(0x2afe, 0x7502);
	HI544_write_cmos_sensor(0x2b00, 0x40B2);
	HI544_write_cmos_sensor(0x2b02, 0x0803);
	HI544_write_cmos_sensor(0x2b04, 0x7504);
	HI544_write_cmos_sensor(0x2b06, 0x40B2);
	HI544_write_cmos_sensor(0x2b08, 0x080F);
	HI544_write_cmos_sensor(0x2b0a, 0x7506);
	HI544_write_cmos_sensor(0x2b0c, 0x40B2);
	HI544_write_cmos_sensor(0x2b0e, 0x6003);
	HI544_write_cmos_sensor(0x2b10, 0x7508);
	HI544_write_cmos_sensor(0x2b12, 0x40B2);
	HI544_write_cmos_sensor(0x2b14, 0x0801);
	HI544_write_cmos_sensor(0x2b16, 0x750A);
	HI544_write_cmos_sensor(0x2b18, 0x40B2);
	HI544_write_cmos_sensor(0x2b1a, 0x0800);
	HI544_write_cmos_sensor(0x2b1c, 0x750C);
	HI544_write_cmos_sensor(0x2b1e, 0x40B2);
	HI544_write_cmos_sensor(0x2b20, 0x1400);
	HI544_write_cmos_sensor(0x2b22, 0x750E);
	HI544_write_cmos_sensor(0x2b24, 0x403F);
	HI544_write_cmos_sensor(0x2b26, 0x0003);
	HI544_write_cmos_sensor(0x2b28, 0x12B0);
	HI544_write_cmos_sensor(0x2b2a, 0xF752);
	HI544_write_cmos_sensor(0x2b2c, 0x421F);
	HI544_write_cmos_sensor(0x2b2e, 0x0098);
	HI544_write_cmos_sensor(0x2b30, 0x821F);
	HI544_write_cmos_sensor(0x2b32, 0x0092);
	HI544_write_cmos_sensor(0x2b34, 0x531F);
	HI544_write_cmos_sensor(0x2b36, 0xC312);
	HI544_write_cmos_sensor(0x2b38, 0x100F);
	HI544_write_cmos_sensor(0x2b3a, 0x4F82);
	HI544_write_cmos_sensor(0x2b3c, 0x0A86);
	HI544_write_cmos_sensor(0x2b3e, 0x421F);
	HI544_write_cmos_sensor(0x2b40, 0x00AC);
	HI544_write_cmos_sensor(0x2b42, 0x821F);
	HI544_write_cmos_sensor(0x2b44, 0x00A6);
	HI544_write_cmos_sensor(0x2b46, 0x531F);
	HI544_write_cmos_sensor(0x2b48, 0x4F82);
	HI544_write_cmos_sensor(0x2b4a, 0x0A88);
	HI544_write_cmos_sensor(0x2b4c, 0xB0B2);
	HI544_write_cmos_sensor(0x2b4e, 0x0010);
	HI544_write_cmos_sensor(0x2b50, 0x0A84);
	HI544_write_cmos_sensor(0x2b52, 0x248F);
	HI544_write_cmos_sensor(0x2b54, 0x421E);
	HI544_write_cmos_sensor(0x2b56, 0x068C);
	HI544_write_cmos_sensor(0x2b58, 0xC312);
	HI544_write_cmos_sensor(0x2b5a, 0x100E);
	HI544_write_cmos_sensor(0x2b5c, 0x4E82);
	HI544_write_cmos_sensor(0x2b5e, 0x0782);
	HI544_write_cmos_sensor(0x2b60, 0x4292);
	HI544_write_cmos_sensor(0x2b62, 0x068E);
	HI544_write_cmos_sensor(0x2b64, 0x0784);
	HI544_write_cmos_sensor(0x2b66, 0xB3D2);
	HI544_write_cmos_sensor(0x2b68, 0x0CB6);
	HI544_write_cmos_sensor(0x2b6a, 0x2418);
	HI544_write_cmos_sensor(0x2b6c, 0x421A);
	HI544_write_cmos_sensor(0x2b6e, 0x0CB8);
	HI544_write_cmos_sensor(0x2b70, 0x430B);
	HI544_write_cmos_sensor(0x2b72, 0x425F);
	HI544_write_cmos_sensor(0x2b74, 0x0CBA);
	HI544_write_cmos_sensor(0x2b76, 0x4F4E);
	HI544_write_cmos_sensor(0x2b78, 0x430F);
	HI544_write_cmos_sensor(0x2b7a, 0x4E0F);
	HI544_write_cmos_sensor(0x2b7c, 0x430E);
	HI544_write_cmos_sensor(0x2b7e, 0xDE0A);
	HI544_write_cmos_sensor(0x2b80, 0xDF0B);
	HI544_write_cmos_sensor(0x2b82, 0x421F);
	HI544_write_cmos_sensor(0x2b84, 0x0CBC);
	HI544_write_cmos_sensor(0x2b86, 0x4F0C);
	HI544_write_cmos_sensor(0x2b88, 0x430D);
	HI544_write_cmos_sensor(0x2b8a, 0x421F);
	HI544_write_cmos_sensor(0x2b8c, 0x0CBE);
	HI544_write_cmos_sensor(0x2b8e, 0x430E);
	HI544_write_cmos_sensor(0x2b90, 0xDE0C);
	HI544_write_cmos_sensor(0x2b92, 0xDF0D);
	HI544_write_cmos_sensor(0x2b94, 0x12B0);
	HI544_write_cmos_sensor(0x2b96, 0xFEAA);
	HI544_write_cmos_sensor(0x2b98, 0x4C82);
	HI544_write_cmos_sensor(0x2b9a, 0x0194);
	HI544_write_cmos_sensor(0x2b9c, 0xB2A2);
	HI544_write_cmos_sensor(0x2b9e, 0x0A84);
	HI544_write_cmos_sensor(0x2ba0, 0x2412);
	HI544_write_cmos_sensor(0x2ba2, 0x421E);
	HI544_write_cmos_sensor(0x2ba4, 0x0A96);
	HI544_write_cmos_sensor(0x2ba6, 0xC312);
	HI544_write_cmos_sensor(0x2ba8, 0x100E);
	HI544_write_cmos_sensor(0x2baa, 0x110E);
	HI544_write_cmos_sensor(0x2bac, 0x110E);
	HI544_write_cmos_sensor(0x2bae, 0x43C2);
	HI544_write_cmos_sensor(0x2bb0, 0x0A98);
	HI544_write_cmos_sensor(0x2bb2, 0x431D);
	HI544_write_cmos_sensor(0x2bb4, 0x4E0F);
	HI544_write_cmos_sensor(0x2bb6, 0x9F82);
	HI544_write_cmos_sensor(0x2bb8, 0x0194);
	HI544_write_cmos_sensor(0x2bba, 0x2850);
	HI544_write_cmos_sensor(0x2bbc, 0x5E0F);
	HI544_write_cmos_sensor(0x2bbe, 0x531D);
	HI544_write_cmos_sensor(0x2bc0, 0x903D);
	HI544_write_cmos_sensor(0x2bc2, 0x0009);
	HI544_write_cmos_sensor(0x2bc4, 0x2BF8);
	HI544_write_cmos_sensor(0x2bc6, 0x4292);
	HI544_write_cmos_sensor(0x2bc8, 0x0084);
	HI544_write_cmos_sensor(0x2bca, 0x7524);
	HI544_write_cmos_sensor(0x2bcc, 0x4292);
	HI544_write_cmos_sensor(0x2bce, 0x0088);
	HI544_write_cmos_sensor(0x2bd0, 0x7316);
	HI544_write_cmos_sensor(0x2bd2, 0x9382);
	HI544_write_cmos_sensor(0x2bd4, 0x8092);
	HI544_write_cmos_sensor(0x2bd6, 0x2403);
	HI544_write_cmos_sensor(0x2bd8, 0x4292);
	HI544_write_cmos_sensor(0x2bda, 0x008A);
	HI544_write_cmos_sensor(0x2bdc, 0x7316);
	HI544_write_cmos_sensor(0x2bde, 0x430E);
	HI544_write_cmos_sensor(0x2be0, 0x421F);
	HI544_write_cmos_sensor(0x2be2, 0x0086);
	HI544_write_cmos_sensor(0x2be4, 0x822F);
	HI544_write_cmos_sensor(0x2be6, 0x9F82);
	HI544_write_cmos_sensor(0x2be8, 0x0084);
	HI544_write_cmos_sensor(0x2bea, 0x2801);
	HI544_write_cmos_sensor(0x2bec, 0x431E);
	HI544_write_cmos_sensor(0x2bee, 0x4292);
	HI544_write_cmos_sensor(0x2bf0, 0x0086);
	HI544_write_cmos_sensor(0x2bf2, 0x7314);
	HI544_write_cmos_sensor(0x2bf4, 0x93C2);
	HI544_write_cmos_sensor(0x2bf6, 0x00BC);
	HI544_write_cmos_sensor(0x2bf8, 0x2007);
	HI544_write_cmos_sensor(0x2bfa, 0xB31E);
	HI544_write_cmos_sensor(0x2bfc, 0x2405);
	HI544_write_cmos_sensor(0x2bfe, 0x421F);
	HI544_write_cmos_sensor(0x2c00, 0x0084);
	HI544_write_cmos_sensor(0x2c02, 0x522F);
	HI544_write_cmos_sensor(0x2c04, 0x4F82);
	HI544_write_cmos_sensor(0x2c06, 0x7314);
	HI544_write_cmos_sensor(0x2c08, 0x425F);
	HI544_write_cmos_sensor(0x2c0a, 0x00BC);
	HI544_write_cmos_sensor(0x2c0c, 0xF37F);
	HI544_write_cmos_sensor(0x2c0e, 0xFE0F);
	HI544_write_cmos_sensor(0x2c10, 0x2406);
	HI544_write_cmos_sensor(0x2c12, 0x421E);
	HI544_write_cmos_sensor(0x2c14, 0x0086);
	HI544_write_cmos_sensor(0x2c16, 0x503E);
	HI544_write_cmos_sensor(0x2c18, 0xFFFB);
	HI544_write_cmos_sensor(0x2c1a, 0x4E82);
	HI544_write_cmos_sensor(0x2c1c, 0x7524);
	HI544_write_cmos_sensor(0x2c1e, 0x430E);
	HI544_write_cmos_sensor(0x2c20, 0x421F);
	HI544_write_cmos_sensor(0x2c22, 0x7524);
	HI544_write_cmos_sensor(0x2c24, 0x9F82);
	HI544_write_cmos_sensor(0x2c26, 0x809C);
	HI544_write_cmos_sensor(0x2c28, 0x2C07);
	HI544_write_cmos_sensor(0x2c2a, 0x9382);
	HI544_write_cmos_sensor(0x2c2c, 0x8096);
	HI544_write_cmos_sensor(0x2c2e, 0x2004);
	HI544_write_cmos_sensor(0x2c30, 0x9382);
	HI544_write_cmos_sensor(0x2c32, 0x8098);
	HI544_write_cmos_sensor(0x2c34, 0x2001);
	HI544_write_cmos_sensor(0x2c36, 0x431E);
	HI544_write_cmos_sensor(0x2c38, 0x40B2);
	HI544_write_cmos_sensor(0x2c3a, 0x0032);
	HI544_write_cmos_sensor(0x2c3c, 0x7522);
	HI544_write_cmos_sensor(0x2c3e, 0x4292);
	HI544_write_cmos_sensor(0x2c40, 0x7524);
	HI544_write_cmos_sensor(0x2c42, 0x809C);
	HI544_write_cmos_sensor(0x2c44, 0x930E);
	HI544_write_cmos_sensor(0x2c46, 0x24E6);
	HI544_write_cmos_sensor(0x2c48, 0x421F);
	HI544_write_cmos_sensor(0x2c4a, 0x7316);
	HI544_write_cmos_sensor(0x2c4c, 0xC312);
	HI544_write_cmos_sensor(0x2c4e, 0x100F);
	HI544_write_cmos_sensor(0x2c50, 0x832F);
	HI544_write_cmos_sensor(0x2c52, 0x4F82);
	HI544_write_cmos_sensor(0x2c54, 0x7522);
	HI544_write_cmos_sensor(0x2c56, 0x53B2);
	HI544_write_cmos_sensor(0x2c58, 0x7524);
	HI544_write_cmos_sensor(0x2c5a, 0x3CDC);
	HI544_write_cmos_sensor(0x2c5c, 0x431F);
	HI544_write_cmos_sensor(0x2c5e, 0x4D0E);
	HI544_write_cmos_sensor(0x2c60, 0x533E);
	HI544_write_cmos_sensor(0x2c62, 0x930E);
	HI544_write_cmos_sensor(0x2c64, 0x2403);
	HI544_write_cmos_sensor(0x2c66, 0x5F0F);
	HI544_write_cmos_sensor(0x2c68, 0x831E);
	HI544_write_cmos_sensor(0x2c6a, 0x23FD);
	HI544_write_cmos_sensor(0x2c6c, 0x4FC2);
	HI544_write_cmos_sensor(0x2c6e, 0x0A98);
	HI544_write_cmos_sensor(0x2c70, 0x3FAA);
	HI544_write_cmos_sensor(0x2c72, 0x4292);
	HI544_write_cmos_sensor(0x2c74, 0x0A86);
	HI544_write_cmos_sensor(0x2c76, 0x0782);
	HI544_write_cmos_sensor(0x2c78, 0x421F);
	HI544_write_cmos_sensor(0x2c7a, 0x0A88);
	HI544_write_cmos_sensor(0x2c7c, 0x490E);
	HI544_write_cmos_sensor(0x2c7e, 0x930E);
	HI544_write_cmos_sensor(0x2c80, 0x2404);
	HI544_write_cmos_sensor(0x2c82, 0xC312);
	HI544_write_cmos_sensor(0x2c84, 0x100F);
	HI544_write_cmos_sensor(0x2c86, 0x831E);
	HI544_write_cmos_sensor(0x2c88, 0x23FC);
	HI544_write_cmos_sensor(0x2c8a, 0x4F82);
	HI544_write_cmos_sensor(0x2c8c, 0x0784);
	HI544_write_cmos_sensor(0x2c8e, 0x3F6B);
	HI544_write_cmos_sensor(0x2c90, 0x90F2);
	HI544_write_cmos_sensor(0x2c92, 0x0010);
	HI544_write_cmos_sensor(0x2c94, 0x00BA);
	HI544_write_cmos_sensor(0x2c96, 0x2809);
	HI544_write_cmos_sensor(0x2c98, 0x90F2);
	HI544_write_cmos_sensor(0x2c9a, 0x0031);
	HI544_write_cmos_sensor(0x2c9c, 0x00BA);
	HI544_write_cmos_sensor(0x2c9e, 0x2C05);
	HI544_write_cmos_sensor(0x2ca0, 0x403C);
	HI544_write_cmos_sensor(0x2ca2, 0x0E05);
	HI544_write_cmos_sensor(0x2ca4, 0x403D);
	HI544_write_cmos_sensor(0x2ca6, 0x003C);
	HI544_write_cmos_sensor(0x2ca8, 0x3EE3);
	HI544_write_cmos_sensor(0x2caa, 0x90F2);
	HI544_write_cmos_sensor(0x2cac, 0x0031);
	HI544_write_cmos_sensor(0x2cae, 0x00BA);
	HI544_write_cmos_sensor(0x2cb0, 0x280D);
	HI544_write_cmos_sensor(0x2cb2, 0x90F2);
	HI544_write_cmos_sensor(0x2cb4, 0x0039);
	HI544_write_cmos_sensor(0x2cb6, 0x00BA);
	HI544_write_cmos_sensor(0x2cb8, 0x2C09);
	HI544_write_cmos_sensor(0x2cba, 0x403C);
	HI544_write_cmos_sensor(0x2cbc, 0x0E09);
	HI544_write_cmos_sensor(0x2cbe, 0x403D);
	HI544_write_cmos_sensor(0x2cc0, 0x003C);
	HI544_write_cmos_sensor(0x2cc2, 0x425E);
	HI544_write_cmos_sensor(0x2cc4, 0x00BA);
	HI544_write_cmos_sensor(0x2cc6, 0x507E);
	HI544_write_cmos_sensor(0x2cc8, 0x0003);
	HI544_write_cmos_sensor(0x2cca, 0x3ED4);
	HI544_write_cmos_sensor(0x2ccc, 0x90F2);
	HI544_write_cmos_sensor(0x2cce, 0x0039);
	HI544_write_cmos_sensor(0x2cd0, 0x00BA);
	HI544_write_cmos_sensor(0x2cd2, 0x280D);
	HI544_write_cmos_sensor(0x2cd4, 0x90F2);
	HI544_write_cmos_sensor(0x2cd6, 0x0041);
	HI544_write_cmos_sensor(0x2cd8, 0x00BA);
	HI544_write_cmos_sensor(0x2cda, 0x2C09);
	HI544_write_cmos_sensor(0x2cdc, 0x403C);
	HI544_write_cmos_sensor(0x2cde, 0x0C0B);
	HI544_write_cmos_sensor(0x2ce0, 0x403D);
	HI544_write_cmos_sensor(0x2ce2, 0x003C);
	HI544_write_cmos_sensor(0x2ce4, 0x425E);
	HI544_write_cmos_sensor(0x2ce6, 0x00BA);
	HI544_write_cmos_sensor(0x2ce8, 0x507E);
	HI544_write_cmos_sensor(0x2cea, 0x0005);
	HI544_write_cmos_sensor(0x2cec, 0x3EC3);
	HI544_write_cmos_sensor(0x2cee, 0x90F2);
	HI544_write_cmos_sensor(0x2cf0, 0x0041);
	HI544_write_cmos_sensor(0x2cf2, 0x00BA);
	HI544_write_cmos_sensor(0x2cf4, 0x280D);
	HI544_write_cmos_sensor(0x2cf6, 0x90F2);
	HI544_write_cmos_sensor(0x2cf8, 0x0051);
	HI544_write_cmos_sensor(0x2cfa, 0x00BA);
	HI544_write_cmos_sensor(0x2cfc, 0x2C09);
	HI544_write_cmos_sensor(0x2cfe, 0x403C);
	HI544_write_cmos_sensor(0x2d00, 0x0A0F);
	HI544_write_cmos_sensor(0x2d02, 0x403D);
	HI544_write_cmos_sensor(0x2d04, 0x003C);
	HI544_write_cmos_sensor(0x2d06, 0x425E);
	HI544_write_cmos_sensor(0x2d08, 0x00BA);
	HI544_write_cmos_sensor(0x2d0a, 0x507E);
	HI544_write_cmos_sensor(0x2d0c, 0x0009);
	HI544_write_cmos_sensor(0x2d0e, 0x3EB2);
	HI544_write_cmos_sensor(0x2d10, 0x90F2);
	HI544_write_cmos_sensor(0x2d12, 0x0051);
	HI544_write_cmos_sensor(0x2d14, 0x00BA);
	HI544_write_cmos_sensor(0x2d16, 0x280D);
	HI544_write_cmos_sensor(0x2d18, 0x90F2);
	HI544_write_cmos_sensor(0x2d1a, 0x0061);
	HI544_write_cmos_sensor(0x2d1c, 0x00BA);
	HI544_write_cmos_sensor(0x2d1e, 0x2C09);
	HI544_write_cmos_sensor(0x2d20, 0x403C);
	HI544_write_cmos_sensor(0x2d22, 0x0A11);
	HI544_write_cmos_sensor(0x2d24, 0x403D);
	HI544_write_cmos_sensor(0x2d26, 0x003C);
	HI544_write_cmos_sensor(0x2d28, 0x425E);
	HI544_write_cmos_sensor(0x2d2a, 0x00BA);
	HI544_write_cmos_sensor(0x2d2c, 0x507E);
	HI544_write_cmos_sensor(0x2d2e, 0x000B);
	HI544_write_cmos_sensor(0x2d30, 0x3EA1);
	HI544_write_cmos_sensor(0x2d32, 0x90F2);
	HI544_write_cmos_sensor(0x2d34, 0x0061);
	HI544_write_cmos_sensor(0x2d36, 0x00BA);
	HI544_write_cmos_sensor(0x2d38, 0x2807);
	HI544_write_cmos_sensor(0x2d3a, 0x90F2);
	HI544_write_cmos_sensor(0x2d3c, 0x0075);
	HI544_write_cmos_sensor(0x2d3e, 0x00BA);
	HI544_write_cmos_sensor(0x2d40, 0x2C03);
	HI544_write_cmos_sensor(0x2d42, 0x403C);
	HI544_write_cmos_sensor(0x2d44, 0x0813);
	HI544_write_cmos_sensor(0x2d46, 0x3FF0);
	HI544_write_cmos_sensor(0x2d48, 0x90F2);
	HI544_write_cmos_sensor(0x2d4a, 0x0075);
	HI544_write_cmos_sensor(0x2d4c, 0x00BA);
	HI544_write_cmos_sensor(0x2d4e, 0x280B);
	HI544_write_cmos_sensor(0x2d50, 0x90F2);
	HI544_write_cmos_sensor(0x2d52, 0xFF91);
	HI544_write_cmos_sensor(0x2d54, 0x00BA);
	HI544_write_cmos_sensor(0x2d56, 0x2C07);
	HI544_write_cmos_sensor(0x2d58, 0x403C);
	HI544_write_cmos_sensor(0x2d5a, 0x0813);
	HI544_write_cmos_sensor(0x2d5c, 0x425E);
	HI544_write_cmos_sensor(0x2d5e, 0x00BA);
	HI544_write_cmos_sensor(0x2d60, 0x507E);
	HI544_write_cmos_sensor(0x2d62, 0x000C);
	HI544_write_cmos_sensor(0x2d64, 0x3E87);
	HI544_write_cmos_sensor(0x2d66, 0x90F2);
	HI544_write_cmos_sensor(0x2d68, 0xFF91);
	HI544_write_cmos_sensor(0x2d6a, 0x00BA);
	HI544_write_cmos_sensor(0x2d6c, 0x280B);
	HI544_write_cmos_sensor(0x2d6e, 0x90F2);
	HI544_write_cmos_sensor(0x2d70, 0xFFB1);
	HI544_write_cmos_sensor(0x2d72, 0x00BA);
	HI544_write_cmos_sensor(0x2d74, 0x2C07);
	HI544_write_cmos_sensor(0x2d76, 0x403D);
	HI544_write_cmos_sensor(0x2d78, 0x0060);
	HI544_write_cmos_sensor(0x2d7a, 0x425E);
	HI544_write_cmos_sensor(0x2d7c, 0x00BA);
	HI544_write_cmos_sensor(0x2d7e, 0x507E);
	HI544_write_cmos_sensor(0x2d80, 0x000D);
	HI544_write_cmos_sensor(0x2d82, 0x3E78);
	HI544_write_cmos_sensor(0x2d84, 0x90F2);
	HI544_write_cmos_sensor(0x2d86, 0xFFB1);
	HI544_write_cmos_sensor(0x2d88, 0x00BA);
	HI544_write_cmos_sensor(0x2d8a, 0x280B);
	HI544_write_cmos_sensor(0x2d8c, 0x90F2);
	HI544_write_cmos_sensor(0x2d8e, 0xFFD1);
	HI544_write_cmos_sensor(0x2d90, 0x00BA);
	HI544_write_cmos_sensor(0x2d92, 0x2C07);
	HI544_write_cmos_sensor(0x2d94, 0x403D);
	HI544_write_cmos_sensor(0x2d96, 0x0050);
	HI544_write_cmos_sensor(0x2d98, 0x425E);
	HI544_write_cmos_sensor(0x2d9a, 0x00BA);
	HI544_write_cmos_sensor(0x2d9c, 0x507E);
	HI544_write_cmos_sensor(0x2d9e, 0x000E);
	HI544_write_cmos_sensor(0x2da0, 0x3E69);
	HI544_write_cmos_sensor(0x2da2, 0x403D);
	HI544_write_cmos_sensor(0x2da4, 0x003C);
	HI544_write_cmos_sensor(0x2da6, 0x403F);
	HI544_write_cmos_sensor(0x2da8, 0x00BA);
	HI544_write_cmos_sensor(0x2daa, 0x4F6E);
	HI544_write_cmos_sensor(0x2dac, 0x507E);
	HI544_write_cmos_sensor(0x2dae, 0x000F);
	HI544_write_cmos_sensor(0x2db0, 0x90FF);
	HI544_write_cmos_sensor(0x2db2, 0xFFF1);
	HI544_write_cmos_sensor(0x2db4, 0x0000);
	HI544_write_cmos_sensor(0x2db6, 0x2A5E);
	HI544_write_cmos_sensor(0x2db8, 0x437E);
	HI544_write_cmos_sensor(0x2dba, 0x3E5C);
	HI544_write_cmos_sensor(0x2dbc, 0x425F);
	HI544_write_cmos_sensor(0x2dbe, 0x00BA);
	HI544_write_cmos_sensor(0x2dc0, 0x4F4C);
	HI544_write_cmos_sensor(0x2dc2, 0x407A);
	HI544_write_cmos_sensor(0x2dc4, 0x001C);
	HI544_write_cmos_sensor(0x2dc6, 0x12B0);
	HI544_write_cmos_sensor(0x2dc8, 0xFE8E);
	HI544_write_cmos_sensor(0x2dca, 0x4E4F);
	HI544_write_cmos_sensor(0x2dcc, 0xC312);
	HI544_write_cmos_sensor(0x2dce, 0x104F);
	HI544_write_cmos_sensor(0x2dd0, 0x114F);
	HI544_write_cmos_sensor(0x2dd2, 0xF37F);
	HI544_write_cmos_sensor(0x2dd4, 0x5F0F);
	HI544_write_cmos_sensor(0x2dd6, 0x5F0F);
	HI544_write_cmos_sensor(0x2dd8, 0x5F0F);
	HI544_write_cmos_sensor(0x2dda, 0x5F0F);
	HI544_write_cmos_sensor(0x2ddc, 0x403E);
	HI544_write_cmos_sensor(0x2dde, 0x00F0);
	HI544_write_cmos_sensor(0x2de0, 0x8F0E);
	HI544_write_cmos_sensor(0x2de2, 0x3E30);
	HI544_write_cmos_sensor(0x2de4, 0x421F);
	HI544_write_cmos_sensor(0x2de6, 0x80BE);
	HI544_write_cmos_sensor(0x2de8, 0x903F);
	HI544_write_cmos_sensor(0x2dea, 0x0009);
	HI544_write_cmos_sensor(0x2dec, 0x2C05);
	HI544_write_cmos_sensor(0x2dee, 0x531F);
	HI544_write_cmos_sensor(0x2df0, 0x4F82);
	HI544_write_cmos_sensor(0x2df2, 0x80BE);
	HI544_write_cmos_sensor(0x2df4, 0x4030);
	HI544_write_cmos_sensor(0x2df6, 0xF9F4);
	HI544_write_cmos_sensor(0x2df8, 0x421F);
	HI544_write_cmos_sensor(0x2dfa, 0x80B6);
	HI544_write_cmos_sensor(0x2dfc, 0x903F);
	HI544_write_cmos_sensor(0x2dfe, 0x0098);
	HI544_write_cmos_sensor(0x2e00, 0x2C05);
	HI544_write_cmos_sensor(0x2e02, 0x531F);
	HI544_write_cmos_sensor(0x2e04, 0x4F82);
	HI544_write_cmos_sensor(0x2e06, 0x80B6);
	HI544_write_cmos_sensor(0x2e08, 0x4030);
	HI544_write_cmos_sensor(0x2e0a, 0xF9F4);
	HI544_write_cmos_sensor(0x2e0c, 0x4382);
	HI544_write_cmos_sensor(0x2e0e, 0x80B6);
	HI544_write_cmos_sensor(0x2e10, 0x4030);
	HI544_write_cmos_sensor(0x2e12, 0xF9F4);
	HI544_write_cmos_sensor(0x2e14, 0x4E82);
	HI544_write_cmos_sensor(0x2e16, 0x8098);
	HI544_write_cmos_sensor(0x2e18, 0xD392);
	HI544_write_cmos_sensor(0x2e1a, 0x7102);
	HI544_write_cmos_sensor(0x2e1c, 0x4138);
	HI544_write_cmos_sensor(0x2e1e, 0x4139);
	HI544_write_cmos_sensor(0x2e20, 0x413A);
	HI544_write_cmos_sensor(0x2e22, 0x413B);
	HI544_write_cmos_sensor(0x2e24, 0x4130);
	HI544_write_cmos_sensor(0x2e26, 0x0260);
	HI544_write_cmos_sensor(0x2e28, 0x0000);
	HI544_write_cmos_sensor(0x2e2a, 0x0C18);
	HI544_write_cmos_sensor(0x2e2c, 0x0240);
	HI544_write_cmos_sensor(0x2e2e, 0x0000);
	HI544_write_cmos_sensor(0x2e30, 0x0260);
	HI544_write_cmos_sensor(0x2e32, 0x0000);
	HI544_write_cmos_sensor(0x2e34, 0x0C05);
	HI544_write_cmos_sensor(0x2e36, 0x4130);
	HI544_write_cmos_sensor(0x2e38, 0x4382);
	HI544_write_cmos_sensor(0x2e3a, 0x7602);
	HI544_write_cmos_sensor(0x2e3c, 0x4F82);
	HI544_write_cmos_sensor(0x2e3e, 0x7600);
	HI544_write_cmos_sensor(0x2e40, 0x0270);
	HI544_write_cmos_sensor(0x2e42, 0x0000);
	HI544_write_cmos_sensor(0x2e44, 0x0C07);
	HI544_write_cmos_sensor(0x2e46, 0x0270);
	HI544_write_cmos_sensor(0x2e48, 0x0001);
	HI544_write_cmos_sensor(0x2e4a, 0x421F);
	HI544_write_cmos_sensor(0x2e4c, 0x7606);
	HI544_write_cmos_sensor(0x2e4e, 0x4FC2);
	HI544_write_cmos_sensor(0x2e50, 0x0188);
	HI544_write_cmos_sensor(0x2e52, 0x4130);
	HI544_write_cmos_sensor(0x2e54, 0xDF02);
	HI544_write_cmos_sensor(0x2e56, 0x3FFE);
	HI544_write_cmos_sensor(0x2e58, 0x430E);
	HI544_write_cmos_sensor(0x2e5a, 0x930A);
	HI544_write_cmos_sensor(0x2e5c, 0x2407);
	HI544_write_cmos_sensor(0x2e5e, 0xC312);
	HI544_write_cmos_sensor(0x2e60, 0x100C);
	HI544_write_cmos_sensor(0x2e62, 0x2801);
	HI544_write_cmos_sensor(0x2e64, 0x5A0E);
	HI544_write_cmos_sensor(0x2e66, 0x5A0A);
	HI544_write_cmos_sensor(0x2e68, 0x930C);
	HI544_write_cmos_sensor(0x2e6a, 0x23F7);
	HI544_write_cmos_sensor(0x2e6c, 0x4130);
	HI544_write_cmos_sensor(0x2e6e, 0x430E);
	HI544_write_cmos_sensor(0x2e70, 0x430F);
	HI544_write_cmos_sensor(0x2e72, 0x3C08);
	HI544_write_cmos_sensor(0x2e74, 0xC312);
	HI544_write_cmos_sensor(0x2e76, 0x100D);
	HI544_write_cmos_sensor(0x2e78, 0x100C);
	HI544_write_cmos_sensor(0x2e7a, 0x2802);
	HI544_write_cmos_sensor(0x2e7c, 0x5A0E);
	HI544_write_cmos_sensor(0x2e7e, 0x6B0F);
	HI544_write_cmos_sensor(0x2e80, 0x5A0A);
	HI544_write_cmos_sensor(0x2e82, 0x6B0B);
	HI544_write_cmos_sensor(0x2e84, 0x930C);
	HI544_write_cmos_sensor(0x2e86, 0x23F6);
	HI544_write_cmos_sensor(0x2e88, 0x930D);
	HI544_write_cmos_sensor(0x2e8a, 0x23F4);
	HI544_write_cmos_sensor(0x2e8c, 0x4130);
	HI544_write_cmos_sensor(0x2e8e, 0xEE4E);
	HI544_write_cmos_sensor(0x2e90, 0x407B);
	HI544_write_cmos_sensor(0x2e92, 0x0009);
	HI544_write_cmos_sensor(0x2e94, 0x3C05);
	HI544_write_cmos_sensor(0x2e96, 0x100D);
	HI544_write_cmos_sensor(0x2e98, 0x6E4E);
	HI544_write_cmos_sensor(0x2e9a, 0x9A4E);
	HI544_write_cmos_sensor(0x2e9c, 0x2801);
	HI544_write_cmos_sensor(0x2e9e, 0x8A4E);
	HI544_write_cmos_sensor(0x2ea0, 0x6C4C);
	HI544_write_cmos_sensor(0x2ea2, 0x6D0D);
	HI544_write_cmos_sensor(0x2ea4, 0x835B);
	HI544_write_cmos_sensor(0x2ea6, 0x23F7);
	HI544_write_cmos_sensor(0x2ea8, 0x4130);
	HI544_write_cmos_sensor(0x2eaa, 0xEF0F);
	HI544_write_cmos_sensor(0x2eac, 0xEE0E);
	HI544_write_cmos_sensor(0x2eae, 0x4039);
	HI544_write_cmos_sensor(0x2eb0, 0x0021);
	HI544_write_cmos_sensor(0x2eb2, 0x3C0A);
	HI544_write_cmos_sensor(0x2eb4, 0x1008);
	HI544_write_cmos_sensor(0x2eb6, 0x6E0E);
	HI544_write_cmos_sensor(0x2eb8, 0x6F0F);
	HI544_write_cmos_sensor(0x2eba, 0x9B0F);
	HI544_write_cmos_sensor(0x2ebc, 0x2805);
	HI544_write_cmos_sensor(0x2ebe, 0x2002);
	HI544_write_cmos_sensor(0x2ec0, 0x9A0E);
	HI544_write_cmos_sensor(0x2ec2, 0x2802);
	HI544_write_cmos_sensor(0x2ec4, 0x8A0E);
	HI544_write_cmos_sensor(0x2ec6, 0x7B0F);
	HI544_write_cmos_sensor(0x2ec8, 0x6C0C);
	HI544_write_cmos_sensor(0x2eca, 0x6D0D);
	HI544_write_cmos_sensor(0x2ecc, 0x6808);
	HI544_write_cmos_sensor(0x2ece, 0x8319);
	HI544_write_cmos_sensor(0x2ed0, 0x23F1);
	HI544_write_cmos_sensor(0x2ed2, 0x4130);
	HI544_write_cmos_sensor(0x2ed4, 0x0000);
	HI544_write_cmos_sensor(0x2ffe, 0xf000);
	HI544_write_cmos_sensor(0x3000, 0x00AE);
	HI544_write_cmos_sensor(0x3002, 0x00AE);
	HI544_write_cmos_sensor(0x3004, 0x00AE);
	HI544_write_cmos_sensor(0x3006, 0x00AE);
	HI544_write_cmos_sensor(0x3008, 0x00AE);
	HI544_write_cmos_sensor(0x4000, 0x0400);
	HI544_write_cmos_sensor(0x4002, 0x0400);
	HI544_write_cmos_sensor(0x4004, 0x0C04);
	HI544_write_cmos_sensor(0x4006, 0x0C04);
	HI544_write_cmos_sensor(0x4008, 0x0C04);

	//--- FW End ---//


	//--- Initial Set file ---//
	HI544_write_cmos_sensor(0x0B02, 0x0014);
	HI544_write_cmos_sensor(0x0B04, 0x07C8);
	HI544_write_cmos_sensor(0x0B06, 0x5ED7);
	HI544_write_cmos_sensor(0x0B14, 0x370B); //PLL Main Div[15:8]. 0x1b = Pclk(86.4mhz), 0x37 = Pclk(176mhz)
	HI544_write_cmos_sensor(0x0B16, 0x4A0B);
	HI544_write_cmos_sensor(0x0B18, 0x0000);
	HI544_write_cmos_sensor(0x0B1A, 0x1044);

	HI544_write_cmos_sensor(0x004C, 0x0100); 
	HI544_write_cmos_sensor(0x000C, 0x0000);
	HI544_write_cmos_sensor(0x0036, 0x0048); 
	HI544_write_cmos_sensor(0x0038, 0x4800); 
	HI544_write_cmos_sensor(0x0138, 0x0104); 
	HI544_write_cmos_sensor(0x013A, 0x0100); 
	HI544_write_cmos_sensor(0x0C00, 0x3BC7); 
	HI544_write_cmos_sensor(0x0C0E, 0x2500); 
	HI544_write_cmos_sensor(0x0C10, 0x0510); 
	HI544_write_cmos_sensor(0x0C16, 0x4040); 
	HI544_write_cmos_sensor(0x0C18, 0x4040); 
	HI544_write_cmos_sensor(0x0C36, 0x0100); 

	//--- MIPI blank time --------------//
	HI544_write_cmos_sensor(0x0902, 0x4101); //mipi_value_clk_trail. MIPI CLK mode [1]'1'cont(0x43),'0'non-cont(0x41) [6]'1'2lane(0x41), '0'1lane(0x01) 
	HI544_write_cmos_sensor(0x090A, 0x03E4); //mipi_vblank_delay_h.
	HI544_write_cmos_sensor(0x090C, 0x0020); //mipi_hblank_short_delay_h.
	HI544_write_cmos_sensor(0x090E, 0x0020); //mipi_hblank_long_delay_h.
	HI544_write_cmos_sensor(0x0910, 0x5D07); //05 mipi_LPX
	HI544_write_cmos_sensor(0x0912, 0x061e); //05 mipi_CLK_prepare
	HI544_write_cmos_sensor(0x0914, 0x0407); //02 mipi_clk_pre
	HI544_write_cmos_sensor(0x0916, 0x0b0a); //09 mipi_data_zero
	HI544_write_cmos_sensor(0x0918, 0x0e09); //0c mipi_clk_post
	//----------------------------------//

	//--- Pixel Array Addressing ------//
	HI544_write_cmos_sensor(0x000E, 0x0000); //x_addr_start_lobp_h.
	HI544_write_cmos_sensor(0x0014, 0x003F); //x_addr_end_lobp_h.
	HI544_write_cmos_sensor(0x0010, 0x0050); //x_addr_start_robp_h.
	HI544_write_cmos_sensor(0x0016, 0x008F); //x_addr_end_robp_h.
	HI544_write_cmos_sensor(0x0012, 0x00A4); //x_addr_start_hact_h. 170
	HI544_write_cmos_sensor(0x0018, 0x0AD1); //x_addr_end_hact_h. 2769. 2769-170+1=2606
	HI544_write_cmos_sensor(0x0020, 0x0700); //x_regin_sel
	HI544_write_cmos_sensor(0x0022, 0x0004); //y_addr_start_fobp_h.
	HI544_write_cmos_sensor(0x0028, 0x000B); //y_addr_end_fobp_h.  
	HI544_write_cmos_sensor(0x0024, 0xFFFA); //y_addr_start_dummy_h.
	HI544_write_cmos_sensor(0x002A, 0xFFFF); //y_addr_end_dummy_h.  
	HI544_write_cmos_sensor(0x0026, 0x0012); //y_addr_start_vact_h. 18
	HI544_write_cmos_sensor(0x002C, 0x07B5); //y_addr_end_vact_h.  1973. 1973-18+1=1956
	HI544_write_cmos_sensor(0x0034, 0x0700); //Y_region_sel
	//------------------------------//

	//--Crop size 2604x1956 ----///
	HI544_write_cmos_sensor(0x0128, 0x0002); // digital_crop_x_offset_l
	HI544_write_cmos_sensor(0x012A, 0x0000); // digital_crop_y_offset_l
	HI544_write_cmos_sensor(0x012C, 0x0A2C); // digital_crop_image_width
	HI544_write_cmos_sensor(0x012E, 0x07A4); // digital_crop_image_height
	//------------------------------//

	//----< Image FMT Size >--------------------//
	//Image size 2604x1956
	HI544_write_cmos_sensor(0x0110, 0x0A2C); //X_output_size_h     
	HI544_write_cmos_sensor(0x0112, 0x07A4); //Y_output_size_h     
	//------------------------------------------//

	//----< Frame / Line Length >--------------//
	HI544_write_cmos_sensor(0x0006, 0x07c0); //frame_length_h 1984
	HI544_write_cmos_sensor(0x0008, 0x0B40); //line_length_h 2880
	HI544_write_cmos_sensor(0x000A, 0x0DB0); //line_length for binning 3504
	//---------------------------------------//

	//--- ETC set ----//
	HI544_write_cmos_sensor(0x003C, 0x0000); //fixed frame off. b[0] '1'enable, '0'disable
	HI544_write_cmos_sensor(0x0000, 0x0000); //orientation. [0]:x-flip, [1]:y-flip.
	HI544_write_cmos_sensor(0x0500, 0x0000); //DGA_ctl.  b[1]'0'OTP_color_ratio_disable, '1' OTP_color_ratio_enable, b[2]'0'data_pedestal_en, '1'data_pedestal_dis.
	HI544_write_cmos_sensor(0x0700, 0x0590); //Scaler Normal 
	HI544_write_cmos_sensor(0x001E, 0x0101); 
	HI544_write_cmos_sensor(0x0032, 0x0101); 
	HI544_write_cmos_sensor(0x0A02, 0x0100); // Fast sleep Enable
	HI544_write_cmos_sensor(0x0C12, 0x0100); // BLC Offset
	HI544_write_cmos_sensor(0x0116, 0x0024); // FBLC Ratio
	//----------------//

	//--- AG / DG control ----------------//
	//AG
	HI544_write_cmos_sensor(0x003A, 0x0000); //Analog Gain.  0x00=x1, 0x70=x8, 0xf0=x16.

	//DG
	HI544_write_cmos_sensor(0x0508, 0x0100); //DG_Gr_h.  0x01=x1, 0x07=x8.
	HI544_write_cmos_sensor(0x050a, 0x0100); //DG_Gb_h.  0x01=x1, 0x07=x8.
	HI544_write_cmos_sensor(0x050c, 0x0100); //DG_R_h.  0x01=x1, 0x07=x8.
	HI544_write_cmos_sensor(0x050e, 0x0100); //DG_B_h.  0x01=x1, 0x07=x8.
	//----------------------------------//


	//-----< Exp.Time >------------------------//
	// Pclk_88Mhz @ Line_length_pclk : 2880 @Exp.Time 33.33ms
	HI544_write_cmos_sensor(0x0002, 0x04b0);	//Fine_int : 33.33ms@Pclk88mhz@Line_length2880 
	HI544_write_cmos_sensor(0x0004, 0x07F4); //coarse_int : 33.33ms@Pclk88mhz@Line_length2880

	//--- ISP enable Selection ---------------//
	HI544_write_cmos_sensor(0x0A04, 0x011A); //isp_en. [9]s-gamma,[8]MIPI_en,[6]compresion10to8,[5]Scaler,[4]window,[3]DG,[2]LSC,[1]adpc,[0]tpg
	//----------------------------------------//

	HI544_write_cmos_sensor(0x0118, 0x0100); //sleep Off


	//-- END --//	
}


/*************************************************************************
* FUNCTION
*   HI544Open
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

UINT32 HI544Open(void)
{

	volatile signed int i;
	kal_uint16 sensor_id = 0;

	HI544DB("HI544Open enter :\n ");

	for(i=0;i<3;i++)
	{
		sensor_id = (HI544_read_cmos_sensor(0x0F17)<<8)|HI544_read_cmos_sensor(0x0F16);
		HI544DB("OHI544 READ ID :%x",sensor_id);
		if(sensor_id != HI544MIPI_SENSOR_ID)
		{
			return ERROR_SENSOR_CONNECT_FAIL;
		}
		else
		{
			break;
		}
	}
	
	spin_lock(&HI544mipiraw_drv_lock);
	HI544.sensorMode = SENSOR_MODE_INIT;
	HI544.HI544AutoFlickerMode = KAL_FALSE;
	HI544.HI544VideoMode = KAL_FALSE;
	spin_unlock(&HI544mipiraw_drv_lock);
	HI544_Sensor_Init();

	spin_lock(&HI544mipiraw_drv_lock);
	HI544.DummyLines= 0;
	HI544.DummyPixels= 0;
	HI544.pvPclk =  ( HI544_PV_CLK / 10000); 
	HI544.videoPclk = ( HI544_VIDEO_CLK / 10000);
	HI544.capPclk = (HI544_CAP_CLK / 10000);

	HI544.shutter = 0x4EA;
	HI544.pvShutter = 0x4EA;
	HI544.maxExposureLines =HI544_PV_PERIOD_LINE_NUMS -4;

	HI544.ispBaseGain = BASEGAIN;//0x40
	HI544.sensorGlobalGain = 0x1f;//sensor gain read from 0x350a 0x350b; 0x1f as 3.875x
	HI544.pvGain = 0x1f;
	HI544.realGain = 0x1f;//ispBaseGain as 1x
	spin_unlock(&HI544mipiraw_drv_lock);


	HI544DB("HI544Open exit :\n ");

    return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*   HI544GetSensorID
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
UINT32 HI544GetSensorID(UINT32 *sensorID)
{
    int  retry = 3;

	HI544DB("HI544GetSensorID enter :\n ");

    // check if sensor ID correct
    do {
        *sensorID = (HI544_read_cmos_sensor(0x0F17)<<8)|HI544_read_cmos_sensor(0x0F16);
        if (*sensorID == HI544MIPI_SENSOR_ID)
        	{
        		HI544DB("Sensor ID = 0x%04x\n", *sensorID);
            	break;
        	}
        HI544DB("Read Sensor ID Fail = 0x%04x\n", *sensorID);
//      mDELAY(1000);
        retry--;
    } while (retry > 0);
//    } while (1);

    if (*sensorID != HI544MIPI_SENSOR_ID) {
        *sensorID = 0xFFFFFFFF;
        return ERROR_SENSOR_CONNECT_FAIL;
    }
    return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
*   HI544_SetShutter
*
* DESCRIPTION
*   This function set e-shutter of HI544 to change exposure time.
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
void HI544_SetShutter(kal_uint32 iShutter)
{

//   if(HI544.shutter == iShutter)
//   		return;

   spin_lock(&HI544mipiraw_drv_lock);
   HI544.shutter= iShutter;
   spin_unlock(&HI544mipiraw_drv_lock);

   HI544_write_shutter(iShutter);
   return;
 
}   /*  HI544_SetShutter   */



/*************************************************************************
* FUNCTION
*   HI544_read_shutter
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
UINT32 HI544_read_shutter(void)
{

	kal_uint16 temp_reg1, temp_reg2;
	UINT32 shutter =0;
	temp_reg1 = HI544_read_cmos_sensor(0x0004);  
	//temp_reg2 = HI544_read_cmos_sensor(0x0005);    
	//read out register value and divide 16;
	shutter  = temp_reg1;

	return shutter;
}

/*************************************************************************
* FUNCTION
*   HI544_night_mode
*
* DESCRIPTION
*   This function night mode of HI544.
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
void HI544_NightMode(kal_bool bEnable)
{
}/*	HI544_NightMode */



/*************************************************************************
* FUNCTION
*   HI544Close
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
UINT32 HI544Close(void)
{
    //  CISModulePowerOn(FALSE);
    //s_porting
    //  DRV_I2CClose(HI544hDrvI2C);
    //e_porting
    ReEnteyCamera = KAL_FALSE;
    return ERROR_NONE;
}	/* HI544Close() */

void HI544SetFlipMirror(kal_int32 imgMirror)
{
#if 1
	
    switch (imgMirror)
    {
        case IMAGE_NORMAL://IMAGE_NOMAL:
            HI544_write_cmos_sensor(0x0000, 0x0000);//Set normal
            break;    
        case IMAGE_V_MIRROR://IMAGE_V_MIRROR:
            HI544_write_cmos_sensor(0x0000, 0x0200);	//Set flip
            break;
        case IMAGE_H_MIRROR://IMAGE_H_MIRROR:
            HI544_write_cmos_sensor(0x0000, 0x0100);//Set mirror
            break;
        case IMAGE_HV_MIRROR://IMAGE_H_MIRROR:
            HI544_write_cmos_sensor(0x0000, 0x0300);	//Set mirror & flip
            break;            
    }
#endif	
}


/*************************************************************************
* FUNCTION
*   HI544Preview
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
UINT32 HI544Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

	HI544DB("HI544Preview enter:");

	// preview size
	if(HI544.sensorMode == SENSOR_MODE_PREVIEW)
	{
		// do nothing
		// FOR CCT PREVIEW
	}
	else
	{
		//HI544DB("HI544Preview setting!!\n");
		HI544PreviewSetting();
	}
	
	spin_lock(&HI544mipiraw_drv_lock);
	HI544.sensorMode = SENSOR_MODE_PREVIEW; // Need set preview setting after capture mode
	HI544.DummyPixels = 0;//define dummy pixels and lines
	HI544.DummyLines = 0 ;
	HI544_FeatureControl_PERIOD_PixelNum=HI544_PV_PERIOD_PIXEL_NUMS+ HI544.DummyPixels;
	HI544_FeatureControl_PERIOD_LineNum=HI544_PV_PERIOD_LINE_NUMS+HI544.DummyLines;
	spin_unlock(&HI544mipiraw_drv_lock);

	spin_lock(&HI544mipiraw_drv_lock);
	HI544.imgMirror = sensor_config_data->SensorImageMirror;
	//HI544.imgMirror =IMAGE_NORMAL; //by module layout
	spin_unlock(&HI544mipiraw_drv_lock);
	
	//HI544SetFlipMirror(sensor_config_data->SensorImageMirror);
	HI544SetFlipMirror(IMAGE_NORMAL);
	

	HI544DBSOFIA("[HI544Preview]frame_len=%x\n", ((HI544_read_cmos_sensor(0x380e)<<8)+HI544_read_cmos_sensor(0x380f)));

 //   mDELAY(40);
	HI544DB("HI544Preview exit:\n");
    return ERROR_NONE;
}	/* HI544Preview() */



UINT32 HI544Video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

	HI544DB("HI544Video enter:");

	if(HI544.sensorMode == SENSOR_MODE_VIDEO)
	{
		// do nothing
	}
	else
	{
		HI544VideoSetting();

	}
	spin_lock(&HI544mipiraw_drv_lock);
	HI544.sensorMode = SENSOR_MODE_VIDEO;
	HI544_FeatureControl_PERIOD_PixelNum=HI544_VIDEO_PERIOD_PIXEL_NUMS+ HI544.DummyPixels;
	HI544_FeatureControl_PERIOD_LineNum=HI544_VIDEO_PERIOD_LINE_NUMS+HI544.DummyLines;
	spin_unlock(&HI544mipiraw_drv_lock);


	spin_lock(&HI544mipiraw_drv_lock);
	HI544.imgMirror = sensor_config_data->SensorImageMirror;
	//HI544.imgMirror =IMAGE_NORMAL; //by module layout
	spin_unlock(&HI544mipiraw_drv_lock);
	//HI544SetFlipMirror(sensor_config_data->SensorImageMirror);
	HI544SetFlipMirror(IMAGE_NORMAL);

//    mDELAY(40);
	HI544DB("HI544Video exit:\n");
    return ERROR_NONE;
}


UINT32 HI544Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

 	kal_uint32 shutter = HI544.shutter;
	kal_uint32 temp_data;
	//kal_uint32 pv_line_length , cap_line_length,

	if( SENSOR_MODE_CAPTURE== HI544.sensorMode)
	{
		HI544DB("HI544Capture BusrtShot!!!\n");
	}
	else
	{
		HI544DB("HI544Capture enter:\n");
#if 0
		//Record Preview shutter & gain
		shutter=HI544_read_shutter();
		temp_data =  read_HI544_gain();
		spin_lock(&HI544mipiraw_drv_lock);
		HI544.pvShutter =shutter;
		HI544.sensorGlobalGain = temp_data;
		HI544.pvGain =HI544.sensorGlobalGain;
		spin_unlock(&HI544mipiraw_drv_lock);

		HI544DB("[HI544Capture]HI544.shutter=%d, read_pv_shutter=%d, read_pv_gain = 0x%x\n",HI544.shutter, shutter,HI544.sensorGlobalGain);
#endif
		// Full size setting
		HI544CaptureSetting();
	//    mDELAY(20);

		spin_lock(&HI544mipiraw_drv_lock);
		HI544.sensorMode = SENSOR_MODE_CAPTURE;
		HI544.imgMirror = sensor_config_data->SensorImageMirror;
		//HI544.imgMirror =IMAGE_NORMAL; //by module layout
		HI544.DummyPixels = 0;//define dummy pixels and lines
		HI544.DummyLines = 0 ;
		HI544_FeatureControl_PERIOD_PixelNum = HI544_FULL_PERIOD_PIXEL_NUMS + HI544.DummyPixels;
		HI544_FeatureControl_PERIOD_LineNum = HI544_FULL_PERIOD_LINE_NUMS + HI544.DummyLines;
		spin_unlock(&HI544mipiraw_drv_lock);

		//HI544SetFlipMirror(sensor_config_data->SensorImageMirror);

		HI544SetFlipMirror(IMAGE_NORMAL);

		HI544DB("HI544Capture exit:\n");
	}

    return ERROR_NONE;
}	/* HI544Capture() */

UINT32 HI544GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{

    HI544DB("HI544GetResolution!!\n");

	pSensorResolution->SensorPreviewWidth	= HI544_IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorPreviewHeight	= HI544_IMAGE_SENSOR_PV_HEIGHT;
    pSensorResolution->SensorFullWidth		= HI544_IMAGE_SENSOR_FULL_WIDTH;
    pSensorResolution->SensorFullHeight		= HI544_IMAGE_SENSOR_FULL_HEIGHT;
    pSensorResolution->SensorVideoWidth		= HI544_IMAGE_SENSOR_VIDEO_WIDTH;
    pSensorResolution->SensorVideoHeight    = HI544_IMAGE_SENSOR_VIDEO_HEIGHT;
//    HI544DB("SensorPreviewWidth:  %d.\n", pSensorResolution->SensorPreviewWidth);
//    HI544DB("SensorPreviewHeight: %d.\n", pSensorResolution->SensorPreviewHeight);
//    HI544DB("SensorFullWidth:  %d.\n", pSensorResolution->SensorFullWidth);
//    HI544DB("SensorFullHeight: %d.\n", pSensorResolution->SensorFullHeight);
    return ERROR_NONE;
}   /* HI544GetResolution() */

UINT32 HI544GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
                                                MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{

	pSensorInfo->SensorPreviewResolutionX= HI544_IMAGE_SENSOR_PV_WIDTH;
	pSensorInfo->SensorPreviewResolutionY= HI544_IMAGE_SENSOR_PV_HEIGHT;

	pSensorInfo->SensorFullResolutionX= HI544_IMAGE_SENSOR_FULL_WIDTH;
    pSensorInfo->SensorFullResolutionY= HI544_IMAGE_SENSOR_FULL_HEIGHT;

	spin_lock(&HI544mipiraw_drv_lock);
	HI544.imgMirror = pSensorConfigData->SensorImageMirror ;
	spin_unlock(&HI544mipiraw_drv_lock);

   	pSensorInfo->SensorOutputDataFormat= SENSOR_OUTPUT_FORMAT_RAW_B;
    pSensorInfo->SensorClockPolarity =SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;

    pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_MIPI;

    pSensorInfo->CaptureDelayFrame = 4;  // from 2 to 3 for test shutter linearity
    pSensorInfo->PreviewDelayFrame = 4;  // from 442 to 666
    pSensorInfo->VideoDelayFrame = 2;

    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_8MA;
    pSensorInfo->AEShutDelayFrame = 0;//0;		    /* The frame of setting shutter default 0 for TG int */
    pSensorInfo->AESensorGainDelayFrame = 0;//0;     /* The frame of setting sensor gain */
    pSensorInfo->AEISPGainDelayFrame = 2;

    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockRisingCount= 0;

            pSensorInfo->SensorGrabStartX = HI544_PV_X_START;
            pSensorInfo->SensorGrabStartY = HI544_PV_Y_START;

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	     	pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = MIPI_DELAY_COUNT;
	    	pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockRisingCount= 0;

            pSensorInfo->SensorGrabStartX = HI544_VIDEO_X_START;
            pSensorInfo->SensorGrabStartY = HI544_VIDEO_Y_START;

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	     	pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = MIPI_DELAY_COUNT;
	    	pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockRisingCount= 0;

            pSensorInfo->SensorGrabStartX = HI544_FULL_X_START;	//2*HI544_IMAGE_SENSOR_PV_STARTX;
            pSensorInfo->SensorGrabStartY = HI544_FULL_Y_START;	//2*HI544_IMAGE_SENSOR_PV_STARTY;

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = MIPI_DELAY_COUNT;
            pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
        default:
			pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockRisingCount= 0;

            pSensorInfo->SensorGrabStartX = HI544_PV_X_START;
            pSensorInfo->SensorGrabStartY = HI544_PV_Y_START;

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	     	pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = MIPI_DELAY_COUNT;
	    	pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
    }

    memcpy(pSensorConfigData, &HI544SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

    return ERROR_NONE;
}   /* HI544GetInfo() */


UINT32 HI544Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
		spin_lock(&HI544mipiraw_drv_lock);
		HI544CurrentScenarioId = ScenarioId;
		spin_unlock(&HI544mipiraw_drv_lock);
		//HI544DB("ScenarioId=%d\n",ScenarioId);
		HI544DB("HI544CurrentScenarioId=%d\n",HI544CurrentScenarioId);

	switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            HI544Preview(pImageWindow, pSensorConfigData);
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			HI544Video(pImageWindow, pSensorConfigData);
			break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
            HI544Capture(pImageWindow, pSensorConfigData);
            break;

        default:
            return ERROR_INVALID_SCENARIO_ID;

    }
    return ERROR_NONE;
} /* HI544Control() */


UINT32 HI544SetVideoMode(UINT16 u2FrameRate)
{

    kal_uint32 MIN_Frame_length =0,frameRate=0,extralines=0;
    HI544DB("[HI544SetVideoMode] frame rate = %d\n", u2FrameRate);

	spin_lock(&HI544mipiraw_drv_lock);
	VIDEO_MODE_TARGET_FPS=u2FrameRate;
	spin_unlock(&HI544mipiraw_drv_lock);

	if(u2FrameRate==0)
	{
		HI544DB("Disable Video Mode or dynimac fps\n");
		return KAL_TRUE;
	}
	if(u2FrameRate >30 || u2FrameRate <5)
	    HI544DB("error frame rate seting\n");

    if(HI544.sensorMode == SENSOR_MODE_VIDEO)//video ScenarioId recording
    {
    	if(HI544.HI544AutoFlickerMode == KAL_TRUE)
    	{
    		if (u2FrameRate==30)
				frameRate= 304;
			else if(u2FrameRate==15)
				frameRate= 148;//148;
			else
				frameRate=u2FrameRate*10;

			MIN_Frame_length = (HI544.videoPclk*10000)/(HI544_VIDEO_PERIOD_PIXEL_NUMS + HI544.DummyPixels)/frameRate*10;
    	}
		else
		{
    		if (u2FrameRate==30)
				MIN_Frame_length= HI544_VIDEO_PERIOD_LINE_NUMS;
			else	
				MIN_Frame_length = (HI544.videoPclk*10000) /(HI544_VIDEO_PERIOD_PIXEL_NUMS + HI544.DummyPixels)/u2FrameRate;
		}

		if((MIN_Frame_length <=HI544_VIDEO_PERIOD_LINE_NUMS))
		{
			MIN_Frame_length = HI544_VIDEO_PERIOD_LINE_NUMS;
			HI544DB("[HI544SetVideoMode]current fps = %d\n", (HI544.videoPclk*10000)  /(HI544_VIDEO_PERIOD_PIXEL_NUMS)/HI544_VIDEO_PERIOD_LINE_NUMS);
		}
		HI544DB("[HI544SetVideoMode]current fps (10 base)= %d\n", (HI544.videoPclk*10000)*10/(HI544_VIDEO_PERIOD_PIXEL_NUMS + HI544.DummyPixels)/MIN_Frame_length);

		if(HI544.shutter + 4 > MIN_Frame_length)
				MIN_Frame_length = HI544.shutter + 4;

		extralines = MIN_Frame_length - HI544_VIDEO_PERIOD_LINE_NUMS;
		
		spin_lock(&HI544mipiraw_drv_lock);
		HI544.DummyPixels = 0;//define dummy pixels and lines
		HI544.DummyLines = extralines ;
		spin_unlock(&HI544mipiraw_drv_lock);
		
		HI544_SetDummy(HI544.DummyPixels,extralines);
    }
	else if(HI544.sensorMode == SENSOR_MODE_CAPTURE)
	{
		HI544DB("-------[HI544SetVideoMode]ZSD???---------\n");
		if(HI544.HI544AutoFlickerMode == KAL_TRUE)
    	{
    		if (u2FrameRate==15)
			    frameRate= 148;
			else
				frameRate=u2FrameRate*10;

			MIN_Frame_length = (HI544.capPclk*10000) /(HI544_FULL_PERIOD_PIXEL_NUMS + HI544.DummyPixels)/frameRate*10;
    	}
		else
			MIN_Frame_length = (HI544.capPclk*10000) /(HI544_FULL_PERIOD_PIXEL_NUMS + HI544.DummyPixels)/u2FrameRate;

		if((MIN_Frame_length <=HI544_FULL_PERIOD_LINE_NUMS))
		{
			MIN_Frame_length = HI544_FULL_PERIOD_LINE_NUMS;
			HI544DB("[HI544SetVideoMode]current fps = %d\n", (HI544.capPclk*10000) /(HI544_FULL_PERIOD_PIXEL_NUMS)/HI544_FULL_PERIOD_LINE_NUMS);

		}
		HI544DB("[HI544SetVideoMode]current fps (10 base)= %d\n", (HI544.capPclk*10000)*10/(HI544_FULL_PERIOD_PIXEL_NUMS + HI544.DummyPixels)/MIN_Frame_length);

		if(HI544.shutter + 4 > MIN_Frame_length)
				MIN_Frame_length = HI544.shutter + 4;


		extralines = MIN_Frame_length - HI544_FULL_PERIOD_LINE_NUMS;

		spin_lock(&HI544mipiraw_drv_lock);
		HI544.DummyPixels = 0;//define dummy pixels and lines
		HI544.DummyLines = extralines ;
		spin_unlock(&HI544mipiraw_drv_lock);

		HI544_SetDummy(HI544.DummyPixels,extralines);
	}
	HI544DB("[HI544SetVideoMode]MIN_Frame_length=%d,HI544.DummyLines=%d\n",MIN_Frame_length,HI544.DummyLines);

    return KAL_TRUE;
}

UINT32 HI544SetAutoFlickerMode(kal_bool bEnable, UINT16 u2FrameRate)
{
	//return ERROR_NONE;

    //HI544DB("[HI544SetAutoFlickerMode] frame rate(10base) = %d %d\n", bEnable, u2FrameRate);
	if(bEnable) {   // enable auto flicker
		spin_lock(&HI544mipiraw_drv_lock);
		HI544.HI544AutoFlickerMode = KAL_TRUE;
		spin_unlock(&HI544mipiraw_drv_lock);
    } else {
    	spin_lock(&HI544mipiraw_drv_lock);
        HI544.HI544AutoFlickerMode = KAL_FALSE;
		spin_unlock(&HI544mipiraw_drv_lock);
        HI544DB("Disable Auto flicker\n");
    }

    return ERROR_NONE;
}

UINT32 HI544SetTestPatternMode(kal_bool bEnable)
{
    HI544DB("[HI544SetTestPatternMode] Test pattern enable:%d\n", bEnable);
#if 1
    if(bEnable) 
    {
        HI544_write_cmos_sensor(0x020a,0x0200);        
    }
    else
    {
        HI544_write_cmos_sensor(0x020a,0x0000);  

    }
#endif
    return ERROR_NONE;
}

UINT32 HI544MIPISetMaxFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 frameRate) {
	kal_uint32 pclk;
	kal_int16 dummyLine;
	kal_uint16 lineLength,frameHeight;
		
	HI544DB("HI544MIPISetMaxFramerateByScenario: scenarioId = %d, frame rate = %d\n",scenarioId,frameRate);
	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			pclk =  HI544_PV_CLK;
			lineLength = HI544_PV_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - HI544_PV_PERIOD_LINE_NUMS;
			if(dummyLine<0)
				dummyLine = 0;
			spin_lock(&HI544mipiraw_drv_lock);
			HI544.sensorMode = SENSOR_MODE_PREVIEW;
			spin_unlock(&HI544mipiraw_drv_lock);
			HI544_SetDummy(0, dummyLine);			
			break;			
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			pclk =  HI544_VIDEO_CLK;
			lineLength = HI544_VIDEO_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - HI544_VIDEO_PERIOD_LINE_NUMS;
			if(dummyLine<0)
				dummyLine = 0;
			spin_lock(&HI544mipiraw_drv_lock);
			HI544.sensorMode = SENSOR_MODE_VIDEO;
			spin_unlock(&HI544mipiraw_drv_lock);
			HI544_SetDummy(0, dummyLine);			
			break;			
			 break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:			
			pclk = HI544_CAP_CLK;
			lineLength = HI544_FULL_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - HI544_FULL_PERIOD_LINE_NUMS;
			if(dummyLine<0)
				dummyLine = 0;
			spin_lock(&HI544mipiraw_drv_lock);
			HI544.sensorMode = SENSOR_MODE_CAPTURE;
			spin_unlock(&HI544mipiraw_drv_lock);
			HI544_SetDummy(0, dummyLine);			
			break;		
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
            break;
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
			break;
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added   
			break;		
		default:
			break;
	}	
	return ERROR_NONE;
}


UINT32 HI544MIPIGetDefaultFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 *pframeRate) 
{

	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			 *pframeRate = 300;
			 break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			 *pframeRate = 240;  // modify by yfx for zsd cc
			break;		
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added   
			 *pframeRate = 300;
			break;		
		default:
			break;
	}

	return ERROR_NONE;
}



UINT32 HI544FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
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
            *pFeatureReturnPara16++= HI544_IMAGE_SENSOR_FULL_WIDTH;
            *pFeatureReturnPara16= HI544_IMAGE_SENSOR_FULL_HEIGHT;
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_PERIOD:
				*pFeatureReturnPara16++= HI544_FeatureControl_PERIOD_PixelNum;
				*pFeatureReturnPara16= HI544_FeatureControl_PERIOD_LineNum;
				*pFeatureParaLen=4;
				break;
        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			switch(HI544CurrentScenarioId)
			{
				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
					*pFeatureReturnPara32 =  HI544_PV_CLK;
					*pFeatureParaLen=4;
					break;
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					*pFeatureReturnPara32 =  HI544_VIDEO_CLK;
					*pFeatureParaLen=4;
					break;
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
				case MSDK_SCENARIO_ID_CAMERA_ZSD:
					*pFeatureReturnPara32 = HI544_CAP_CLK;
					*pFeatureParaLen=4;
					break;
				default:
					*pFeatureReturnPara32 =  HI544_CAP_CLK;
					*pFeatureParaLen=4;
					break;
			}
		    break;

        case SENSOR_FEATURE_SET_ESHUTTER:
            HI544_SetShutter(*pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_NIGHTMODE:
            HI544_NightMode((BOOL) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_GAIN:
            HI544_SetGain((UINT16) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_FLASHLIGHT:
            break;
        case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
            //HI544_isp_master_clock=*pFeatureData32;
            break;
        case SENSOR_FEATURE_SET_REGISTER:
            HI544_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
            break;
        case SENSOR_FEATURE_GET_REGISTER:
            pSensorRegData->RegData = HI544_read_cmos_sensor(pSensorRegData->RegAddr);
            break;
        case SENSOR_FEATURE_SET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            for (i=0;i<SensorRegNumber;i++)
            {
            	spin_lock(&HI544mipiraw_drv_lock);
                HI544SensorCCT[i].Addr=*pFeatureData32++;
                HI544SensorCCT[i].Para=*pFeatureData32++;
				spin_unlock(&HI544mipiraw_drv_lock);
            }
            break;
        case SENSOR_FEATURE_GET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=HI544SensorCCT[i].Addr;
                *pFeatureData32++=HI544SensorCCT[i].Para;
            }
            break;
        case SENSOR_FEATURE_SET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            for (i=0;i<SensorRegNumber;i++)
            {
            	spin_lock(&HI544mipiraw_drv_lock);
                HI544SensorReg[i].Addr=*pFeatureData32++;
                HI544SensorReg[i].Para=*pFeatureData32++;
				spin_unlock(&HI544mipiraw_drv_lock);
            }
            break;
        case SENSOR_FEATURE_GET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=HI544SensorReg[i].Addr;
                *pFeatureData32++=HI544SensorReg[i].Para;
            }
            break;
        case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
            if (*pFeatureParaLen>=sizeof(NVRAM_SENSOR_DATA_STRUCT))
            {
                pSensorDefaultData->Version=NVRAM_CAMERA_SENSOR_FILE_VERSION;
                pSensorDefaultData->SensorId=HI544MIPI_SENSOR_ID;
                memcpy(pSensorDefaultData->SensorEngReg, HI544SensorReg, sizeof(SENSOR_REG_STRUCT)*ENGINEER_END);
                memcpy(pSensorDefaultData->SensorCCTReg, HI544SensorCCT, sizeof(SENSOR_REG_STRUCT)*FACTORY_END_ADDR);
            }
            else
                return FALSE;
            *pFeatureParaLen=sizeof(NVRAM_SENSOR_DATA_STRUCT);
            break;
        case SENSOR_FEATURE_GET_CONFIG_PARA:
            memcpy(pSensorConfigData, &HI544SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
            *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
            break;
        case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
            HI544_camera_para_to_sensor();
            break;

        case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
            HI544_sensor_to_camera_para();
            break;
        case SENSOR_FEATURE_GET_GROUP_COUNT:
            *pFeatureReturnPara32++=HI544_get_sensor_group_count();
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_GROUP_INFO:
            HI544_get_sensor_group_info(pSensorGroupInfo->GroupIdx, pSensorGroupInfo->GroupNamePtr, &pSensorGroupInfo->ItemCount);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_GROUP_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_ITEM_INFO:
            HI544_get_sensor_item_info(pSensorItemInfo->GroupIdx,pSensorItemInfo->ItemIdx, pSensorItemInfo);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_SET_ITEM_INFO:
            HI544_set_sensor_item_info(pSensorItemInfo->GroupIdx, pSensorItemInfo->ItemIdx, pSensorItemInfo->ItemValue);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_GET_ENG_INFO:
            pSensorEngInfo->SensorId = 129;
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
            break;
        case SENSOR_FEATURE_CONSTANT_AF:
            break;
        case SENSOR_FEATURE_MOVE_FOCUS_LENS:
            break;
        case SENSOR_FEATURE_SET_VIDEO_MODE:
            HI544SetVideoMode(*pFeatureData16);
            break;
        case SENSOR_FEATURE_CHECK_SENSOR_ID:
            HI544GetSensorID(pFeatureReturnPara32);
            break;
        case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
            HI544SetAutoFlickerMode((BOOL)*pFeatureData16, *(pFeatureData16+1));
	        break;
        case SENSOR_FEATURE_SET_TEST_PATTERN:
            HI544SetTestPatternMode((BOOL)*pFeatureData16);
            break;
		case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
			HI544MIPISetMaxFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, *(pFeatureData32+1));
			break;
		case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
			HI544MIPIGetDefaultFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, (MUINT32 *)(*(pFeatureData32+1)));
            break;       
        case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE://for factory mode auto testing             
            *pFeatureReturnPara32=HI544_TEST_PATTERN_CHECKSUM;           
            *pFeatureParaLen=4;                             
        break;     
        default:
            break;
    }
    return ERROR_NONE;
}	/* HI544FeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncHI544=
{
    HI544Open,
    HI544GetInfo,
    HI544GetResolution,
    HI544FeatureControl,
    HI544Control,
    HI544Close
};

UINT32 HI544_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&SensorFuncHI544;

    return ERROR_NONE;
}   /* SensorInit() */

