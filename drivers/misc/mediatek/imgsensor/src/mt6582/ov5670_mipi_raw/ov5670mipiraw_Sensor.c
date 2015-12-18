/*******************************************************************************************/
     
 
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

#include "ov5670mipiraw_Sensor.h"
#include "ov5670mipiraw_Camera_Sensor_para.h"
#include "ov5670mipiraw_CameraCustomized.h"
static DEFINE_SPINLOCK(ov5670mipiraw_drv_lock);

#define OV5670_DEBUG
#ifdef OV5670_DEBUG
	#define OV5670DB(fmt, arg...) xlog_printk(ANDROID_LOG_DEBUG, "[OV5670Raw] ",  fmt, ##arg)
#else
	#define OV5670DB(fmt, arg...)
#endif


kal_uint32 OV5670_FeatureControl_PERIOD_PixelNum=OV5670_PV_PERIOD_PIXEL_NUMS;
kal_uint32 OV5670_FeatureControl_PERIOD_LineNum=OV5670_PV_PERIOD_LINE_NUMS;

UINT16 OV5670_VIDEO_MODE_TARGET_FPS = 30;

MSDK_SCENARIO_ID_ENUM OV5670CurrentScenarioId = MSDK_SCENARIO_ID_CAMERA_PREVIEW;
MSDK_SENSOR_CONFIG_STRUCT OV5670SensorConfigData;
static OV5670_PARA_STRUCT OV5670;
kal_uint32 OV5670_FAC_SENSOR_REG;


SENSOR_REG_STRUCT OV5670SensorCCT[]=CAMERA_SENSOR_CCT_DEFAULT_VALUE;
SENSOR_REG_STRUCT OV5670SensorReg[ENGINEER_END]=CAMERA_SENSOR_REG_DEFAULT_VALUE;


#define OV5670_TEST_PATTERN_CHECKSUM 0x75bef806 //0xa2230d9f    //0xf5e2f1ce
kal_bool OV5670_During_testpattern = KAL_FALSE;

extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);

#define OV5670_write_cmos_sensor(addr, para) iWriteReg((u16) addr , (u32) para , 1, OV5670MIPI_WRITE_ID)

kal_uint16 OV5670_read_cmos_sensor(kal_uint32 addr)
{
kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,OV5670MIPI_WRITE_ID);
    return get_byte;
}


void OV5670_Init_Para(void)
{

	spin_lock(&ov5670mipiraw_drv_lock);
	OV5670.sensorMode = SENSOR_MODE_INIT;
	OV5670.OV5670AutoFlickerMode = KAL_FALSE;
	OV5670.OV5670VideoMode = KAL_FALSE;
	OV5670.DummyLines= 0;
	OV5670.DummyPixels= 0;
	OV5670.pvPclk =  (10285); 
	OV5670.videoPclk = (10285);
	OV5670.capPclk = (10285);

	OV5670.shutter = 0x4C00;
	OV5670.ispBaseGain = BASEGAIN;
	OV5670.sensorGlobalGain = 0x0200;
	spin_unlock(&ov5670mipiraw_drv_lock);
}

kal_uint32 GetOV5670LineLength(void)
{
	kal_uint32 OV5670_line_length = 0;
	if ( SENSOR_MODE_PREVIEW == OV5670.sensorMode )  
	{
		OV5670_line_length = OV5670_PV_PERIOD_PIXEL_NUMS + OV5670.DummyPixels;
	}
	else if( SENSOR_MODE_VIDEO == OV5670.sensorMode ) 
	{
		OV5670_line_length = OV5670_VIDEO_PERIOD_PIXEL_NUMS + OV5670.DummyPixels;
	}
	else
	{
		OV5670_line_length = OV5670_FULL_PERIOD_PIXEL_NUMS + OV5670.DummyPixels;
	}

    return OV5670_line_length;

}


kal_uint32 GetOV5670FrameLength(void)
{
	kal_uint32 OV5670_frame_length = 0;

	if ( SENSOR_MODE_PREVIEW == OV5670.sensorMode )  
	{
		OV5670_frame_length = OV5670_PV_PERIOD_LINE_NUMS + OV5670.DummyLines ;
	}
	else if( SENSOR_MODE_VIDEO == OV5670.sensorMode ) 
	{
		OV5670_frame_length = OV5670_VIDEO_PERIOD_LINE_NUMS + OV5670.DummyLines ;
	}
	else
	{
		OV5670_frame_length = OV5670_FULL_PERIOD_LINE_NUMS + OV5670.DummyLines ;
	}

	return OV5670_frame_length;
}


kal_uint32 OV5670_CalcExtra_For_ShutterMargin(kal_uint32 shutter_value,kal_uint32 shutterLimitation)
{
    kal_uint32 extra_lines = 0;

	
	if (shutter_value <4 ){
		shutter_value = 4;
	}

	
	if (shutter_value > shutterLimitation)
	{
		extra_lines = shutter_value - shutterLimitation;
    }
	else
		extra_lines = 0;

    return extra_lines;

}


kal_uint32 OV5670_CalcFrameLength_For_AutoFlicker(void)
{

    kal_uint32 AutoFlicker_min_framelength = 0;

	switch(OV5670CurrentScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			AutoFlicker_min_framelength = (OV5670.capPclk*10000) /(OV5670_FULL_PERIOD_PIXEL_NUMS + OV5670.DummyPixels)/OV5670_AUTOFLICKER_OFFSET_30*10 ;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			if(OV5670_VIDEO_MODE_TARGET_FPS==30)
			{
				AutoFlicker_min_framelength = (OV5670.videoPclk*10000) /(OV5670_VIDEO_PERIOD_PIXEL_NUMS + OV5670.DummyPixels)/OV5670_AUTOFLICKER_OFFSET_30*10 ;
			}
			else if(OV5670_VIDEO_MODE_TARGET_FPS==15)
			{
				AutoFlicker_min_framelength = (OV5670.videoPclk*10000) /(OV5670_VIDEO_PERIOD_PIXEL_NUMS + OV5670.DummyPixels)/OV5670_AUTOFLICKER_OFFSET_15*10 ;
			}
			else
			{
				AutoFlicker_min_framelength = OV5670_VIDEO_PERIOD_LINE_NUMS + OV5670.DummyLines;
			}
			break;
			
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			AutoFlicker_min_framelength = (OV5670.pvPclk*10000) /(OV5670_PV_PERIOD_PIXEL_NUMS + OV5670.DummyPixels)/OV5670_AUTOFLICKER_OFFSET_30*10 ;
			break;
	}

	OV5670DB("AutoFlicker_min_framelength =%d,OV5670CurrentScenarioId =%d\n", AutoFlicker_min_framelength,OV5670CurrentScenarioId);

	return AutoFlicker_min_framelength;

}


void OV5670_write_shutter(kal_uint32 shutter)
{
	kal_uint32 min_framelength = OV5670_PV_PERIOD_PIXEL_NUMS, max_shutter=0;
	kal_uint32 line_length = 0;
	kal_uint32 frame_length = 0;
	unsigned long flags;

	//for test
	//shutter = 0x7fc;  //issue 
	
	
    line_length  = GetOV5670LineLength();
	frame_length = GetOV5670FrameLength();
	
	max_shutter  = frame_length-OV5670_SHUTTER_MARGIN;

    frame_length = frame_length + OV5670_CalcExtra_For_ShutterMargin(shutter,max_shutter);
	


	if(OV5670.OV5670AutoFlickerMode == KAL_TRUE)
	{
        min_framelength = OV5670_CalcFrameLength_For_AutoFlicker();

        if(frame_length < min_framelength)
			frame_length = min_framelength;
	}
	

	spin_lock_irqsave(&ov5670mipiraw_drv_lock,flags);
	OV5670_FeatureControl_PERIOD_PixelNum = line_length;
	OV5670_FeatureControl_PERIOD_LineNum = frame_length;
	spin_unlock_irqrestore(&ov5670mipiraw_drv_lock,flags);

	//Set total frame length  //VTS
	OV5670_write_cmos_sensor(0x380e, (frame_length >> 8) & 0xFF);
	OV5670_write_cmos_sensor(0x380f, frame_length & 0xFF);

	//Set shutter 
	OV5670_write_cmos_sensor(0x3500, (shutter>>12) & 0x0F);
	OV5670_write_cmos_sensor(0x3501, (shutter>>4) & 0xFF);
	OV5670_write_cmos_sensor(0x3502, (shutter<<4) & 0xF0);	  /* Don't use the fraction part. */

	OV5670DB("ov5670 write shutter=%x, line_length=%x, frame_length=%x\n", shutter, line_length, frame_length);

}


static kal_uint16 OV5670Reg2Gain(const kal_uint16 iReg)
{
    kal_uint16 iGain =0; 

	iGain = iReg*BASEGAIN/OV5670_GAIN_BASE;
	return iGain;
	
}

static kal_uint16 OV5670Gain2Reg(const kal_uint16 Gain)
{
    kal_uint32 iReg = 0x0000;
	kal_uint32 TempGain = BASEGAIN;


	TempGain = Gain;
	if(TempGain < BASEGAIN){
		TempGain = BASEGAIN;
		//OV5670DB("###ov5670 write gain underflow### Gain =%x\n", Gain);
	}
	if(TempGain > 8*BASEGAIN){
		TempGain = 8*BASEGAIN;
		//OV5670DB("###ov5670 write gain overflow### Gain =%x\n", Gain);
	}

	iReg = (TempGain*OV5670_GAIN_BASE)/BASEGAIN;

	//OV5670DB("###ov5670 write Reg ### iReg =%x\n", iReg);

    return iReg;

}

void write_OV5670_gain(kal_uint16 gain)
{
	kal_uint16 iGain =1;
	kal_uint8 ChangeFlag=0x01;

	kal_uint16 read_gain;
	
	iGain=(gain / OV5670_GAIN_BASE);

	if(iGain<2){
		ChangeFlag= 0x00;
	}
	else if(iGain<4){
		ChangeFlag= 0x01;
	}
	else if(iGain<8){
		ChangeFlag= 0x03;
	}
	else{
		ChangeFlag= 0x07;
	}

	//ChangeFlag= 0x07;
	
	OV5670_write_cmos_sensor(0x301d, 0xf0);
	OV5670_write_cmos_sensor(0x3209, 0x00);
	OV5670_write_cmos_sensor(0x320a, 0x01);
	
	//group write  hold
	//group 0:delay 0x366a for one frame,then active with gain
	OV5670_write_cmos_sensor(0x3208, 0x00);
	OV5670_write_cmos_sensor(0x366a, ChangeFlag);
	OV5670_write_cmos_sensor(0x3208, 0x10);

	//group 1:all other registers( gain)
	OV5670_write_cmos_sensor(0x3208, 0x01);
	OV5670_write_cmos_sensor(0x3508,(gain>>8));
    OV5670_write_cmos_sensor(0x3509,(gain&0xff));
	
	OV5670_write_cmos_sensor(0x3208, 0x11);

	//group lanch
	OV5670_write_cmos_sensor(0x320B, 0x15);
	OV5670_write_cmos_sensor(0x3208, 0xA1);

	//read_gain=(((OV5670_read_cmos_sensor(0x3508)&0x1F) << 8) | OV5670_read_cmos_sensor(0x3509));
	//OV5670DB("[OV5670_SetGain]0x3508|0x3509=0x%x \n",read_gain);
	//OV5670DB("[OV5670_SetGain]0x366a=%d \n",(OV5670_read_cmos_sensor(0x366a)));

	return;

}

void OV5670_SetGain(UINT16 iGain)
{
	unsigned long flags;
	spin_lock_irqsave(&ov5670mipiraw_drv_lock,flags);

	OV5670DB("OV5670_SetGain iGain = %d :\n ",iGain);
	
	OV5670.realGain = iGain;
	OV5670.sensorGlobalGain = OV5670Gain2Reg(iGain);
	spin_unlock_irqrestore(&ov5670mipiraw_drv_lock,flags);

	write_OV5670_gain(OV5670.sensorGlobalGain);
	OV5670DB(" [OV5670_SetGain]OV5670.sensorGlobalGain=0x%x,OV5670.realGain =%d",OV5670.sensorGlobalGain,
		OV5670.realGain); 

	//temperature test
	//OV5670_write_cmos_sensor(0x4d12,0x01);
	//OV5670DB("Temperature read_reg  0x4d13  =%x \n",OV5670_read_cmos_sensor(0x4d13));
}   

kal_uint16 read_OV5670_gain(void)
{
	kal_uint16 read_gain=0;

	read_gain=(((OV5670_read_cmos_sensor(0x3508)&0x1F) << 8) | OV5670_read_cmos_sensor(0x3509));

	spin_lock(&ov5670mipiraw_drv_lock);
	OV5670.sensorGlobalGain = read_gain;
	OV5670.realGain = OV5670Reg2Gain(OV5670.sensorGlobalGain);
	spin_unlock(&ov5670mipiraw_drv_lock);

	OV5670DB("OV5670.sensorGlobalGain=0x%x,OV5670.realGain=%d\n",OV5670.sensorGlobalGain,OV5670.realGain);

	return OV5670.sensorGlobalGain;
}  


#if 1
void OV5670_camera_para_to_sensor(void)
{
    kal_uint32    i;
    for(i=0; 0xFFFFFFFF!=OV5670SensorReg[i].Addr; i++)
    {
        OV5670_write_cmos_sensor(OV5670SensorReg[i].Addr, OV5670SensorReg[i].Para);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=OV5670SensorReg[i].Addr; i++)
    {
        OV5670_write_cmos_sensor(OV5670SensorReg[i].Addr, OV5670SensorReg[i].Para);
    }
    for(i=FACTORY_START_ADDR; i<FACTORY_END_ADDR; i++)
    {
        OV5670_write_cmos_sensor(OV5670SensorCCT[i].Addr, OV5670SensorCCT[i].Para);
    }
}

void OV5670_sensor_to_camera_para(void)
{
    kal_uint32    i, temp_data;
    for(i=0; 0xFFFFFFFF!=OV5670SensorReg[i].Addr; i++)
    {
         temp_data = OV5670_read_cmos_sensor(OV5670SensorReg[i].Addr);
		 spin_lock(&ov5670mipiraw_drv_lock);
		 OV5670SensorReg[i].Para =temp_data;
		 spin_unlock(&ov5670mipiraw_drv_lock);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=OV5670SensorReg[i].Addr; i++)
    {
        temp_data = OV5670_read_cmos_sensor(OV5670SensorReg[i].Addr);
		spin_lock(&ov5670mipiraw_drv_lock);
		OV5670SensorReg[i].Para = temp_data;
		spin_unlock(&ov5670mipiraw_drv_lock);
    }
}

kal_int32  OV5670_get_sensor_group_count(void)
{
    return GROUP_TOTAL_NUMS;
}

void OV5670_get_sensor_group_info(kal_uint16 group_idx, kal_int8* group_name_ptr, kal_int32* item_count_ptr)
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

void OV5670_get_sensor_item_info(kal_uint16 group_idx,kal_uint16 item_idx, MSDK_SENSOR_ITEM_INFO_STRUCT* info_ptr)
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

            temp_para= OV5670SensorCCT[temp_addr].Para;
			//temp_gain= (temp_para/OV5670.sensorBaseGain) * 1000;

            info_ptr->ItemValue=temp_gain;
            info_ptr->IsTrueFalse=KAL_FALSE;
            info_ptr->IsReadOnly=KAL_FALSE;
            info_ptr->IsNeedRestart=KAL_FALSE;
            info_ptr->Min= OV5670_MIN_ANALOG_GAIN * 1000;
            info_ptr->Max= OV5670_MAX_ANALOG_GAIN * 1000;
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
                    info_ptr->ItemValue=    111;  
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



kal_bool OV5670_set_sensor_item_info(kal_uint16 group_idx, kal_uint16 item_idx, kal_int32 ItemValue)
{
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
//             temp_para=(temp_gain * OV5670.sensorBaseGain + BASEGAIN/2)/BASEGAIN;
          }
          else
			  ASSERT(0);

		  spin_lock(&ov5670mipiraw_drv_lock);
          OV5670SensorCCT[temp_addr].Para = temp_para;
		  spin_unlock(&ov5670mipiraw_drv_lock);
          OV5670_write_cmos_sensor(OV5670SensorCCT[temp_addr].Addr,temp_para);

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
					spin_lock(&ov5670mipiraw_drv_lock);
                    OV5670_FAC_SENSOR_REG=ItemValue;
					spin_unlock(&ov5670mipiraw_drv_lock);
                    break;
                case 1:
                    OV5670_write_cmos_sensor(OV5670_FAC_SENSOR_REG,ItemValue);
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
#endif


static void OV5670_SetDummy( const kal_uint32 iPixels, const kal_uint32 iLines )
{
	kal_uint32 line_length = 0;
	kal_uint32 frame_length = 0;

	if ( SENSOR_MODE_PREVIEW == OV5670.sensorMode )
	{
		line_length = OV5670_PV_PERIOD_PIXEL_NUMS + iPixels;
		frame_length = OV5670_PV_PERIOD_LINE_NUMS + iLines;
	}
	else if( SENSOR_MODE_VIDEO== OV5670.sensorMode )
	{
		line_length = OV5670_VIDEO_PERIOD_PIXEL_NUMS + iPixels;
		frame_length = OV5670_VIDEO_PERIOD_LINE_NUMS + iLines;
	}
	else
	{
		line_length = OV5670_FULL_PERIOD_PIXEL_NUMS + iPixels;
		frame_length = OV5670_FULL_PERIOD_LINE_NUMS + iLines;
	}

	spin_lock(&ov5670mipiraw_drv_lock);
	OV5670_FeatureControl_PERIOD_PixelNum = line_length;
	OV5670_FeatureControl_PERIOD_LineNum = frame_length;
	spin_unlock(&ov5670mipiraw_drv_lock);

	//Set total frame length
	OV5670_write_cmos_sensor(0x380e, (frame_length >> 8) & 0xFF);
	OV5670_write_cmos_sensor(0x380f, frame_length & 0xFF);
	//Set total line length
	OV5670_write_cmos_sensor(0x380c, (line_length >> 8) & 0xFF);
	OV5670_write_cmos_sensor(0x380d, line_length & 0xFF);

}   


void OV5670PreviewSetting(void)
{
	OV5670DB(" OV5670PreviewSetting_2lane enter\n");

	/*  //
	
	//@@PV_Quarter_size_30fps_800Mbps/lane				  
	//99 1296 960 										  
	//;;102 3601	157c									  
	//;;PCLK=HTS*VTS*fps=0x68c*0x7fd*30=1676*2045*30=102.85M

		OV5670_write_cmos_sensor(0x0100, 0x00);  //
	
	OV5670_write_cmos_sensor(0x3501, 0x3d);  //
	OV5670_write_cmos_sensor(0x366e, 0x08);  //
	OV5670_write_cmos_sensor(0x370b, 0x1b);  //
	OV5670_write_cmos_sensor(0x3808, 0x05);  //
	OV5670_write_cmos_sensor(0x3809, 0x10);  //
	OV5670_write_cmos_sensor(0x380a, 0x03);  //
	OV5670_write_cmos_sensor(0x380b, 0xc0);  //
	OV5670_write_cmos_sensor(0x380c, 0x06);  //
	OV5670_write_cmos_sensor(0x380d, 0x8c);  //
	OV5670_write_cmos_sensor(0x380e, 0x07);  //;03
	OV5670_write_cmos_sensor(0x380f, 0xfd);  //;e0
	OV5670_write_cmos_sensor(0x3814, 0x03);  //
	OV5670_write_cmos_sensor(0x3820, 0x90);  //
	OV5670_write_cmos_sensor(0x3821, 0x47);  //
	OV5670_write_cmos_sensor(0x382a, 0x03);  //
	OV5670_write_cmos_sensor(0x4009, 0x05);  //
	OV5670_write_cmos_sensor(0x4502, 0x48);  //
	OV5670_write_cmos_sensor(0x4508, 0x55);  //
	OV5670_write_cmos_sensor(0x4509, 0x55);  //
	OV5670_write_cmos_sensor(0x4600, 0x00);  //
	OV5670_write_cmos_sensor(0x4601, 0x81);  //
	OV5670_write_cmos_sensor(0x4017, 0x10);  //; threshold = 4LSB for Binning sum format.
	OV5670_write_cmos_sensor(0x400a, 0x02);  //;
	OV5670_write_cmos_sensor(0x400b, 0x00);  //; 
	
	OV5670_write_cmos_sensor(0x0100, 0x01);  //
*/

	//@@PV_Quarter_size_30fps_800Mbps/lane_1296x972							  
	//99 1296 972 															  
	//;;102 3601	157c														  
	//;;PCLK=HTS*VTS*fps=0x68c*0x7fd*30=1676*2045*30=102.85M					  
																			  
	OV5670_write_cmos_sensor(0x0100, 0x00);  // 	
	
	OV5670_write_cmos_sensor(0x3501, 0x73);  // 							  
	OV5670_write_cmos_sensor(0x3502, 0x00);  // 							  
	OV5670_write_cmos_sensor(0x3508, 0x01);  // 							  
	OV5670_write_cmos_sensor(0x3509, 0x80);  // 							  
	OV5670_write_cmos_sensor(0x366e, 0x08);  // 							  
	OV5670_write_cmos_sensor(0x370b, 0x1b);  // 							  
	OV5670_write_cmos_sensor(0x3808, 0x05);  // 							  
	OV5670_write_cmos_sensor(0x3809, 0x10);  // 							  
	OV5670_write_cmos_sensor(0x380a, 0x03);  // 							  
	OV5670_write_cmos_sensor(0x380b, 0xcc);  //;c0							  
	OV5670_write_cmos_sensor(0x380c, 0x06);  // 							  
	OV5670_write_cmos_sensor(0x380d, 0x8c);  // 							  
	OV5670_write_cmos_sensor(0x380e, 0x07);  //;03							  
	OV5670_write_cmos_sensor(0x380f, 0xfd);  //;e0							  
	OV5670_write_cmos_sensor(0x3814, 0x03);  // 							  
	OV5670_write_cmos_sensor(0x3820, 0x90);  // 							  
	OV5670_write_cmos_sensor(0x3821, 0x47);  // 							  
	OV5670_write_cmos_sensor(0x382a, 0x03);  // 							  
	OV5670_write_cmos_sensor(0x3845, 0x02);  // 							  
	OV5670_write_cmos_sensor(0x4009, 0x05);  // 							  
	OV5670_write_cmos_sensor(0x4502, 0x48);  // 							  
	OV5670_write_cmos_sensor(0x4508, 0x55);  // 							  
	OV5670_write_cmos_sensor(0x4509, 0x55);  // 							  
	OV5670_write_cmos_sensor(0x4600, 0x00);  // 							  
	OV5670_write_cmos_sensor(0x4601, 0x81);  // 							  
	OV5670_write_cmos_sensor(0x4017, 0x10);  //; threshold = 4LSB for Binning 
	OV5670_write_cmos_sensor(0x400a, 0x02);  //;							  
	OV5670_write_cmos_sensor(0x400b, 0x00);  //;	
	
	OV5670_write_cmos_sensor(0x0100, 0x01);  // 	
}


void OV5670VideoSetting(void)
{
	OV5670DB(" OV5670videoSetting_2lane enter:video/preview sync\n");

	OV5670PreviewSetting();
}



void OV5670CaptureSetting(void)
{
	OV5670DB("OV5670CaptureSetting_2lane enter\n");

	OV5670_write_cmos_sensor(0x0100, 0x00); 
	
	OV5670_write_cmos_sensor(0x3501, 0x5f); //long exposure
	OV5670_write_cmos_sensor(0x3502, 0xd0);  //long exposure
	
	OV5670_write_cmos_sensor(0x3508, 0x03);  //gain
	OV5670_write_cmos_sensor(0x3509, 0x00);  //gain
	
	OV5670_write_cmos_sensor(0x366e, 0x10); 
	OV5670_write_cmos_sensor(0x370b, 0x1b); 
	OV5670_write_cmos_sensor(0x3808, 0x0a); 
	OV5670_write_cmos_sensor(0x3809, 0x20); 
	OV5670_write_cmos_sensor(0x380a, 0x07); 
	OV5670_write_cmos_sensor(0x380b, 0x98); 
	OV5670_write_cmos_sensor(0x380c, 0x07); //;06
	OV5670_write_cmos_sensor(0x380d, 0xdc); //;8c
	OV5670_write_cmos_sensor(0x380e, 0x07); 
	OV5670_write_cmos_sensor(0x380f, 0xfd); 
	OV5670_write_cmos_sensor(0x3814, 0x01); 
	OV5670_write_cmos_sensor(0x3820, 0x80); 
	OV5670_write_cmos_sensor(0x3821, 0x46); 
	OV5670_write_cmos_sensor(0x382a, 0x01);
	
	OV5670_write_cmos_sensor(0x3845, 0x00);  //v_offset for auto size mode
	
	OV5670_write_cmos_sensor(0x4009, 0x0d); 
	OV5670_write_cmos_sensor(0x4502, 0x40); 
	OV5670_write_cmos_sensor(0x4508, 0xaa); 
	OV5670_write_cmos_sensor(0x4509, 0xaa); 
	OV5670_write_cmos_sensor(0x4600, 0x01); 
	OV5670_write_cmos_sensor(0x4601, 0x03); 
	OV5670_write_cmos_sensor(0x4017, 0x08); //threshold= 2LSB for full size
	OV5670_write_cmos_sensor(0x400a, 0x02); //
	OV5670_write_cmos_sensor(0x400b, 0x00); //
	
	OV5670_write_cmos_sensor(0x0100, 0x01); 
	
}


static void OV5670_Sensor_Init(void)
{
	OV5670DB("OV5670_Sensor_Init_2lane enter\n");
	
	OV5670_write_cmos_sensor(0x0103,0x01);// ; software reset
	mdelay(10);
	OV5670_write_cmos_sensor(0x0100, 0x00);// ; software standby
	OV5670_write_cmos_sensor(0x0100, 0x00); 
	OV5670_write_cmos_sensor(0x0300, 0x04); 
	OV5670_write_cmos_sensor(0x0301, 0x00); 
	OV5670_write_cmos_sensor(0x0302, 0x64); //;78
	OV5670_write_cmos_sensor(0x0303, 0x00); 
	OV5670_write_cmos_sensor(0x0304, 0x03); 
	OV5670_write_cmos_sensor(0x0305, 0x01); 
	OV5670_write_cmos_sensor(0x0306, 0x01); 
	OV5670_write_cmos_sensor(0x030a, 0x00); 
	OV5670_write_cmos_sensor(0x030b, 0x00); 
	OV5670_write_cmos_sensor(0x030c, 0x00); 
	OV5670_write_cmos_sensor(0x030d, 0x1e); 
	OV5670_write_cmos_sensor(0x030e, 0x00); 
	OV5670_write_cmos_sensor(0x030f, 0x06); 
	OV5670_write_cmos_sensor(0x0312, 0x01); 
	OV5670_write_cmos_sensor(0x3000, 0x00); 
	OV5670_write_cmos_sensor(0x3002, 0x21); 
	OV5670_write_cmos_sensor(0x3005, 0xf0); 
	OV5670_write_cmos_sensor(0x3007, 0x00); 
	OV5670_write_cmos_sensor(0x3015, 0x0f); 
	OV5670_write_cmos_sensor(0x3018, 0x32); 
	OV5670_write_cmos_sensor(0x301a, 0xf0); 
	OV5670_write_cmos_sensor(0x301b, 0xf0); 
	OV5670_write_cmos_sensor(0x301c, 0xf0); 
	OV5670_write_cmos_sensor(0x301d, 0xf0); 
	OV5670_write_cmos_sensor(0x301e, 0xf0); 
	OV5670_write_cmos_sensor(0x3030, 0x00); 
	OV5670_write_cmos_sensor(0x3031, 0x0a); 
	OV5670_write_cmos_sensor(0x303c, 0xff); 
	OV5670_write_cmos_sensor(0x303e, 0xff); 
	OV5670_write_cmos_sensor(0x3040, 0xf0); 
	OV5670_write_cmos_sensor(0x3041, 0x00); 
	OV5670_write_cmos_sensor(0x3042, 0xf0); 
	OV5670_write_cmos_sensor(0x3106, 0x11); 
	OV5670_write_cmos_sensor(0x3500, 0x00); 
	OV5670_write_cmos_sensor(0x3501, 0x7b); 
	OV5670_write_cmos_sensor(0x3502, 0x00); 
	OV5670_write_cmos_sensor(0x3503, 0x04); 
	OV5670_write_cmos_sensor(0x3504, 0x03); 
	OV5670_write_cmos_sensor(0x3505, 0x83); 
	OV5670_write_cmos_sensor(0x3508, 0x07); 
	OV5670_write_cmos_sensor(0x3509, 0x80); 
	OV5670_write_cmos_sensor(0x350e, 0x04); 
	OV5670_write_cmos_sensor(0x350f, 0x00); 
	OV5670_write_cmos_sensor(0x3510, 0x00); 
	OV5670_write_cmos_sensor(0x3511, 0x02); 
	OV5670_write_cmos_sensor(0x3512, 0x00); 
	OV5670_write_cmos_sensor(0x3601, 0xc8); 
	OV5670_write_cmos_sensor(0x3610, 0x88); 
	OV5670_write_cmos_sensor(0x3612, 0x48); 
	OV5670_write_cmos_sensor(0x3614, 0x5b); 
	OV5670_write_cmos_sensor(0x3615, 0x96); 
	OV5670_write_cmos_sensor(0x3621, 0xd0); 
	OV5670_write_cmos_sensor(0x3622, 0x00); 
	OV5670_write_cmos_sensor(0x3623, 0x00); 
	OV5670_write_cmos_sensor(0x3633, 0x13); 
	OV5670_write_cmos_sensor(0x3634, 0x13); 
	OV5670_write_cmos_sensor(0x3635, 0x13); 
	OV5670_write_cmos_sensor(0x3636, 0x13); 
	OV5670_write_cmos_sensor(0x3645, 0x13); 
	OV5670_write_cmos_sensor(0x3646, 0x82); 
	OV5670_write_cmos_sensor(0x3650, 0x00); 
	OV5670_write_cmos_sensor(0x3652, 0xff); 
	OV5670_write_cmos_sensor(0x3656, 0xff); 
	OV5670_write_cmos_sensor(0x365a, 0xff); 
	OV5670_write_cmos_sensor(0x365e, 0xff); 
	OV5670_write_cmos_sensor(0x3668, 0x00); 
	OV5670_write_cmos_sensor(0x366a, 0x07); 
	OV5670_write_cmos_sensor(0x366e, 0x10); 
	OV5670_write_cmos_sensor(0x366d, 0x00); 
	OV5670_write_cmos_sensor(0x366f, 0x80); 
	OV5670_write_cmos_sensor(0x3700, 0x28); 
	OV5670_write_cmos_sensor(0x3701, 0x10); 
	OV5670_write_cmos_sensor(0x3702, 0x3a); 
	OV5670_write_cmos_sensor(0x3703, 0x19); 
	OV5670_write_cmos_sensor(0x3705, 0x00); 
	OV5670_write_cmos_sensor(0x3706, 0x66); 
	OV5670_write_cmos_sensor(0x3707, 0x08); 
	OV5670_write_cmos_sensor(0x3708, 0x34); 
	OV5670_write_cmos_sensor(0x3709, 0x40); 
	OV5670_write_cmos_sensor(0x370a, 0x01); 
	OV5670_write_cmos_sensor(0x370b, 0x1b); 
	OV5670_write_cmos_sensor(0x3714, 0x24); 
	OV5670_write_cmos_sensor(0x371a, 0x3e); 
	OV5670_write_cmos_sensor(0x3733, 0x00); 
	OV5670_write_cmos_sensor(0x3734, 0x00); 
	OV5670_write_cmos_sensor(0x373a, 0x05); 
	OV5670_write_cmos_sensor(0x373b, 0x06); 
	OV5670_write_cmos_sensor(0x373c, 0x0a); 
	OV5670_write_cmos_sensor(0x373f, 0xa0); 
	OV5670_write_cmos_sensor(0x3755, 0x00); 
	OV5670_write_cmos_sensor(0x3758, 0x00); 
	OV5670_write_cmos_sensor(0x3766, 0x5f); 
	OV5670_write_cmos_sensor(0x3768, 0x00); 
	OV5670_write_cmos_sensor(0x3769, 0x22); 
	OV5670_write_cmos_sensor(0x3773, 0x08); 
	OV5670_write_cmos_sensor(0x3774, 0x1f); 
	OV5670_write_cmos_sensor(0x3776, 0x06); 
	OV5670_write_cmos_sensor(0x37a0, 0x88); 
	OV5670_write_cmos_sensor(0x37a1, 0x5c); 
	OV5670_write_cmos_sensor(0x37a7, 0x88); 
	OV5670_write_cmos_sensor(0x37a8, 0x70); 
	OV5670_write_cmos_sensor(0x37aa, 0x88); 
	OV5670_write_cmos_sensor(0x37ab, 0x48); 
	OV5670_write_cmos_sensor(0x37b3, 0x66); 
	OV5670_write_cmos_sensor(0x37c2, 0x04); 
	OV5670_write_cmos_sensor(0x37c5, 0x00); 
	OV5670_write_cmos_sensor(0x37c8, 0x00); 
	OV5670_write_cmos_sensor(0x3800, 0x00); 
	OV5670_write_cmos_sensor(0x3801, 0x0c); 
	OV5670_write_cmos_sensor(0x3802, 0x00); 
	OV5670_write_cmos_sensor(0x3803, 0x04); 
	OV5670_write_cmos_sensor(0x3804, 0x0a); 
	OV5670_write_cmos_sensor(0x3805, 0x33); 
	OV5670_write_cmos_sensor(0x3806, 0x07); 
	OV5670_write_cmos_sensor(0x3807, 0xa3); 
	OV5670_write_cmos_sensor(0x3808, 0x0a); 
	OV5670_write_cmos_sensor(0x3809, 0x20); 
	OV5670_write_cmos_sensor(0x380a, 0x07); 
	OV5670_write_cmos_sensor(0x380b, 0x98); 
	OV5670_write_cmos_sensor(0x380c, 0x07); // ;06
	OV5670_write_cmos_sensor(0x380d, 0xdc); // ;8c
	OV5670_write_cmos_sensor(0x380e, 0x07); 
	OV5670_write_cmos_sensor(0x380f, 0xb8); 
	OV5670_write_cmos_sensor(0x3811, 0x04); 
	OV5670_write_cmos_sensor(0x3813, 0x02); 
	OV5670_write_cmos_sensor(0x3814, 0x01); 
	OV5670_write_cmos_sensor(0x3815, 0x01); 
	OV5670_write_cmos_sensor(0x3816, 0x00); 
	OV5670_write_cmos_sensor(0x3817, 0x00); 
	OV5670_write_cmos_sensor(0x3818, 0x00); 
	OV5670_write_cmos_sensor(0x3819, 0x00); 
	OV5670_write_cmos_sensor(0x3820, 0x80); 
	OV5670_write_cmos_sensor(0x3821, 0x46); 
	OV5670_write_cmos_sensor(0x3822, 0x48); 
	OV5670_write_cmos_sensor(0x3826, 0x00); 
	OV5670_write_cmos_sensor(0x3827, 0x08); 
	OV5670_write_cmos_sensor(0x382a, 0x01); 
	OV5670_write_cmos_sensor(0x382b, 0x01); 
	OV5670_write_cmos_sensor(0x3830, 0x08); 
	OV5670_write_cmos_sensor(0x3836, 0x02); 
	OV5670_write_cmos_sensor(0x3837, 0x00); 
	OV5670_write_cmos_sensor(0x3838, 0x10); 
	OV5670_write_cmos_sensor(0x3841, 0xff); 
	OV5670_write_cmos_sensor(0x3846, 0x48); 
	OV5670_write_cmos_sensor(0x3861, 0x00); 
	OV5670_write_cmos_sensor(0x3862, 0x00); 
	OV5670_write_cmos_sensor(0x3863, 0x18); 
	OV5670_write_cmos_sensor(0x3a11, 0x01); 
	OV5670_write_cmos_sensor(0x3a12, 0x78); 
	OV5670_write_cmos_sensor(0x3b00, 0x00); 
	OV5670_write_cmos_sensor(0x3b02, 0x00); 
	OV5670_write_cmos_sensor(0x3b03, 0x00); 
	OV5670_write_cmos_sensor(0x3b04, 0x00); 
	OV5670_write_cmos_sensor(0x3b05, 0x00); 
	OV5670_write_cmos_sensor(0x3c00, 0x89); 
	OV5670_write_cmos_sensor(0x3c01, 0xab); 
	OV5670_write_cmos_sensor(0x3c02, 0x01); 
	OV5670_write_cmos_sensor(0x3c03, 0x00); 
	OV5670_write_cmos_sensor(0x3c04, 0x00); 
	OV5670_write_cmos_sensor(0x3c05, 0x03); 
	OV5670_write_cmos_sensor(0x3c06, 0x00); 
	OV5670_write_cmos_sensor(0x3c07, 0x05); 
	OV5670_write_cmos_sensor(0x3c0c, 0x00); 
	OV5670_write_cmos_sensor(0x3c0d, 0x00); 
	OV5670_write_cmos_sensor(0x3c0e, 0x00); 
	OV5670_write_cmos_sensor(0x3c0f, 0x00); 
	OV5670_write_cmos_sensor(0x3c40, 0x00); 
	OV5670_write_cmos_sensor(0x3c41, 0xa3); 
	OV5670_write_cmos_sensor(0x3c43, 0x7d); 
	OV5670_write_cmos_sensor(0x3c45, 0xd7); 
	OV5670_write_cmos_sensor(0x3c47, 0xfc); 
	OV5670_write_cmos_sensor(0x3c50, 0x05); 
	OV5670_write_cmos_sensor(0x3c52, 0xaa); 
	OV5670_write_cmos_sensor(0x3c54, 0x71); 
	OV5670_write_cmos_sensor(0x3c56, 0x80); 
	OV5670_write_cmos_sensor(0x3f03, 0x00); 
	OV5670_write_cmos_sensor(0x3f0a, 0x00); 
	OV5670_write_cmos_sensor(0x3f0b, 0x00); 
	OV5670_write_cmos_sensor(0x4001, 0x60); 
	OV5670_write_cmos_sensor(0x4009, 0x0d); 
	OV5670_write_cmos_sensor(0x4020, 0x00); 
	OV5670_write_cmos_sensor(0x4021, 0x00); 
	OV5670_write_cmos_sensor(0x4022, 0x00); 
	OV5670_write_cmos_sensor(0x4023, 0x00); 
	OV5670_write_cmos_sensor(0x4024, 0x00); 
	OV5670_write_cmos_sensor(0x4025, 0x00); 
	OV5670_write_cmos_sensor(0x4026, 0x00); 
	OV5670_write_cmos_sensor(0x4027, 0x00); 
	OV5670_write_cmos_sensor(0x4028, 0x00); 
	OV5670_write_cmos_sensor(0x4029, 0x00); 
	OV5670_write_cmos_sensor(0x402a, 0x00); 
	OV5670_write_cmos_sensor(0x402b, 0x00); 
	OV5670_write_cmos_sensor(0x402c, 0x00); 
	OV5670_write_cmos_sensor(0x402d, 0x00); 
	OV5670_write_cmos_sensor(0x402e, 0x00); 
	OV5670_write_cmos_sensor(0x402f, 0x00); 
	OV5670_write_cmos_sensor(0x4040, 0x00); 
	OV5670_write_cmos_sensor(0x4041, 0x00); 
	OV5670_write_cmos_sensor(0x4042, 0x00); 
	OV5670_write_cmos_sensor(0x4043, 0x80); 
	OV5670_write_cmos_sensor(0x4044, 0x00); 
	OV5670_write_cmos_sensor(0x4045, 0x80); 
	OV5670_write_cmos_sensor(0x4046, 0x00); 
	OV5670_write_cmos_sensor(0x4047, 0x80); 
	OV5670_write_cmos_sensor(0x4048, 0x00); 
	OV5670_write_cmos_sensor(0x4049, 0x80); 
	OV5670_write_cmos_sensor(0x4303, 0x00); 
	OV5670_write_cmos_sensor(0x4307, 0x30); 
	OV5670_write_cmos_sensor(0x4500, 0x58); 
	OV5670_write_cmos_sensor(0x4501, 0x04); 
	OV5670_write_cmos_sensor(0x4502, 0x40); 
	OV5670_write_cmos_sensor(0x4503, 0x10); 
	OV5670_write_cmos_sensor(0x4508, 0xaa); 
	OV5670_write_cmos_sensor(0x4509, 0xaa); 
	OV5670_write_cmos_sensor(0x450a, 0x00); 
	OV5670_write_cmos_sensor(0x450b, 0x00); 
	OV5670_write_cmos_sensor(0x4600, 0x01); 
	OV5670_write_cmos_sensor(0x4601, 0x03); 
	OV5670_write_cmos_sensor(0x4700, 0xa4); 
	OV5670_write_cmos_sensor(0x4800, 0x4c); 
	OV5670_write_cmos_sensor(0x4816, 0x53); 
	OV5670_write_cmos_sensor(0x481f, 0x40); 
	OV5670_write_cmos_sensor(0x4837, 0x14); // ;11
	OV5670_write_cmos_sensor(0x5000, 0x16); 
	OV5670_write_cmos_sensor(0x5001, 0x01); 
	OV5670_write_cmos_sensor(0x5002, 0xa8); 
	OV5670_write_cmos_sensor(0x5004, 0x0c); 
	OV5670_write_cmos_sensor(0x5006, 0x0c); 
	OV5670_write_cmos_sensor(0x5007, 0xe0); 
	OV5670_write_cmos_sensor(0x5008, 0x01); 
	OV5670_write_cmos_sensor(0x5009, 0xb0); 
	OV5670_write_cmos_sensor(0x5901, 0x00); 
	OV5670_write_cmos_sensor(0x5a01, 0x00); 
	OV5670_write_cmos_sensor(0x5a03, 0x00); 
	OV5670_write_cmos_sensor(0x5a04, 0x0c); 
	OV5670_write_cmos_sensor(0x5a05, 0xe0); 
	OV5670_write_cmos_sensor(0x5a06, 0x09); 
	OV5670_write_cmos_sensor(0x5a07, 0xb0); 
	OV5670_write_cmos_sensor(0x5a08, 0x06); 
	OV5670_write_cmos_sensor(0x5e00, 0x00); 
	OV5670_write_cmos_sensor(0x3618, 0x2a); 
								   
	//;Ally031414					  
	OV5670_write_cmos_sensor(0x3734, 0x40); //	;; Improve HFPN
	OV5670_write_cmos_sensor(0x5b00, 0x01);  // ;; [2:0] otp start addr[10:8]
	OV5670_write_cmos_sensor(0x5b01, 0x10);  // ;; [7:0] otp start addr[7:0]
	OV5670_write_cmos_sensor(0x5b02, 0x01);  // ;; [2:0] otp end addr[10:8]
	OV5670_write_cmos_sensor(0x5b03, 0xDB);  // ;; [7:0] otp end addr[7:0]
	OV5670_write_cmos_sensor(0x3d8c, 0x71); //; Header address high byte
	OV5670_write_cmos_sensor(0x3d8d, 0xEA); //; Header address low byte
	OV5670_write_cmos_sensor(0x4017, 0x08); // ; threshold= 2LSB for full size
								  
	//;Strong DPC1.53				 
	OV5670_write_cmos_sensor(0x5780, 0x3e); 
	OV5670_write_cmos_sensor(0x5781, 0x0f); 
	OV5670_write_cmos_sensor(0x5782, 0x44); 
	OV5670_write_cmos_sensor(0x5783, 0x02); 
	OV5670_write_cmos_sensor(0x5784, 0x01); 
	OV5670_write_cmos_sensor(0x5785, 0x00); 
	OV5670_write_cmos_sensor(0x5786, 0x00); 
	OV5670_write_cmos_sensor(0x5787, 0x04); 
	OV5670_write_cmos_sensor(0x5788, 0x02); 
	OV5670_write_cmos_sensor(0x5789, 0x0f); 
	OV5670_write_cmos_sensor(0x578a, 0xfd); 
	OV5670_write_cmos_sensor(0x578b, 0xf5); 
	OV5670_write_cmos_sensor(0x578c, 0xf5); 
	OV5670_write_cmos_sensor(0x578d, 0x03); 
	OV5670_write_cmos_sensor(0x578e, 0x08); 
	OV5670_write_cmos_sensor(0x578f, 0x0c); 
	OV5670_write_cmos_sensor(0x5790, 0x08); 
	OV5670_write_cmos_sensor(0x5791, 0x04); 
	OV5670_write_cmos_sensor(0x5792, 0x00); 
	OV5670_write_cmos_sensor(0x5793, 0x52); 
	OV5670_write_cmos_sensor(0x5794, 0xa3); 
	//;Ping 					  
	OV5670_write_cmos_sensor(0x380e, 0x07); //; fps fine adjustment
	OV5670_write_cmos_sensor(0x380f, 0xfd); //; fps fine adjustment
	OV5670_write_cmos_sensor(0x3503, 0x00); //; real gain [2]   gain no delay, shutter no delay
	//;added					 
	OV5670_write_cmos_sensor(0x3d85, 0x17); 
	OV5670_write_cmos_sensor(0x3655, 0x20); 
								   
	//OV5670_write_cmos_sensor(0x0100, 0x00); //;01


}


UINT32 OV5670Open(void)
{

	volatile signed int i;
	kal_uint16 sensor_id = 0;

	OV5670DB("OV5670 Open enter :\n ");
	OV5670_write_cmos_sensor(0x0103,0x01);// Reset sensor
    mdelay(2);

	for(i=0;i<2;i++)
	{
		sensor_id = (OV5670_read_cmos_sensor(0x300B)<<8)|OV5670_read_cmos_sensor(0x300C);
		OV5670DB("OV5670 READ ID :%x",sensor_id);
		if(sensor_id != OV5670_SENSOR_ID)
		{
			return ERROR_SENSOR_CONNECT_FAIL;
		}else
			break;
	}
	
	OV5670_Sensor_Init();
    OV5670_Init_Para();
	
	OV5670DB("OV5670Open exit :\n ");

    return ERROR_NONE;
}


UINT32 OV5670GetSensorID(UINT32 *sensorID)
{
    int  retry = 2;

	OV5670DB("OV5670GetSensorID enter :\n ");
    mdelay(5);

    do {
        *sensorID = (OV5670_read_cmos_sensor(0x300B)<<8)|OV5670_read_cmos_sensor(0x300C);
        if (*sensorID == OV5670_SENSOR_ID)
        	{
        		OV5670DB("Sensor ID = 0x%04x\n", *sensorID);
            	break;
        	}
        OV5670DB("Read Sensor ID Fail = 0x%04x\n", *sensorID);
        retry--;
    } while (retry > 0);

    if (*sensorID != OV5670_SENSOR_ID) {
		OV5670DB("Read Sensor ID Fail = 0x%04x\n", *sensorID);
		
		/*
        *sensorID = OV5670_SENSOR_ID;
        return ERROR_SENSOR_CONNECT_FAIL;
	*/
	 *sensorID = 0xFFFFFFFF;
     return ERROR_SENSOR_CONNECT_FAIL;
    }
    return ERROR_NONE;
}


void OV5670_SetShutter(kal_uint32 iShutter)
{

   spin_lock(&ov5670mipiraw_drv_lock);
   OV5670.shutter= iShutter;
   spin_unlock(&ov5670mipiraw_drv_lock);

   OV5670_write_shutter(iShutter);
   return;
}



UINT32 OV5670_read_shutter(void)
{

	kal_uint16 temp_reg1, temp_reg2 ,temp_reg3;
	UINT32 shutter =0;
	temp_reg1 = OV5670_read_cmos_sensor(0x3500);    // AEC[b19~b16]
	temp_reg2 = OV5670_read_cmos_sensor(0x3501);    // AEC[b15~b8]
	temp_reg3 = OV5670_read_cmos_sensor(0x3502);    // AEC[b7~b0]
	
	shutter  = (temp_reg1 <<12)| (temp_reg2<<4)|(temp_reg3>>4);

	return shutter;
}

void OV5670_NightMode(kal_bool bEnable)
{

}

UINT32 OV5670Close(void)
{

    return ERROR_NONE;
}

#if 1
void OV5670SetFlipMirror(kal_int32 imgMirror)
{
	kal_int16 mirror=0,flip=0,temp=0;
	flip= OV5670_read_cmos_sensor(0x3820);
	mirror  = OV5670_read_cmos_sensor(0x3821);
	temp  = OV5670_read_cmos_sensor(0x450b);

	switch (imgMirror)
	{
		case IMAGE_NORMAL:
			OV5670_write_cmos_sensor(0x3820, (flip & (0xF9)));//Set  flip off
			OV5670_write_cmos_sensor(0x3821, (mirror & (0xF9)));	//Set mirror off
			OV5670_write_cmos_sensor(0x450b, (temp & (0xDF)));
			break;
		case IMAGE_H_MIRROR://default
			OV5670_write_cmos_sensor(0x3820, (flip & (0xF9)));//Set flip off
			OV5670_write_cmos_sensor(0x3821, (mirror | (0x06)));	//Set mirror on
			OV5670_write_cmos_sensor(0x450b, (temp & (0xDF)));
			break;
		case IMAGE_HV_MIRROR:
			OV5670_write_cmos_sensor(0x3820, (flip |(0x06)));	//Set flip on
			OV5670_write_cmos_sensor(0x3821, (mirror | (0x06)));	//Set mirror on
			OV5670_write_cmos_sensor(0x450b, (temp | (0x20)));
			break;
		case IMAGE_V_MIRROR:
			OV5670_write_cmos_sensor(0x3820, (flip |(0x06)));	//Set flip on
			OV5670_write_cmos_sensor(0x3821, (mirror & (0xF9)));	//Set mirror off
			OV5670_write_cmos_sensor(0x450b, (temp | (0x20)));
			break;
		default:
                        break;
	}
}
#endif


UINT32 OV5670Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

	OV5670DB("OV5670Preview enter:");

	OV5670PreviewSetting();

	spin_lock(&ov5670mipiraw_drv_lock);
	OV5670.sensorMode = SENSOR_MODE_PREVIEW; 
	OV5670.DummyPixels = 0;
	OV5670.DummyLines = 0 ;
	OV5670_FeatureControl_PERIOD_PixelNum=OV5670_PV_PERIOD_PIXEL_NUMS+ OV5670.DummyPixels;
	OV5670_FeatureControl_PERIOD_LineNum=OV5670_PV_PERIOD_LINE_NUMS+OV5670.DummyLines;
	OV5670.imgMirror = sensor_config_data->SensorImageMirror;
	spin_unlock(&ov5670mipiraw_drv_lock);
	
	OV5670SetFlipMirror(IMAGE_H_MIRROR);

    mdelay(40);//THIS DELAY SHOULD BE NEED BY CTS OR MONKEY
	OV5670DB("OV5670Preview exit:\n");

	  
    return ERROR_NONE;
}


UINT32 OV5670Video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

	OV5670DB("OV5670Video enter:");

	OV5670VideoSetting();

	spin_lock(&ov5670mipiraw_drv_lock);
	OV5670.sensorMode = SENSOR_MODE_VIDEO;
	OV5670_FeatureControl_PERIOD_PixelNum=OV5670_VIDEO_PERIOD_PIXEL_NUMS+ OV5670.DummyPixels;
	OV5670_FeatureControl_PERIOD_LineNum=OV5670_VIDEO_PERIOD_LINE_NUMS+OV5670.DummyLines;
	OV5670.imgMirror = sensor_config_data->SensorImageMirror;
	spin_unlock(&ov5670mipiraw_drv_lock);
	
	OV5670SetFlipMirror(IMAGE_H_MIRROR);

    mdelay(40);//THIS DELAY SHOULD BE NEED BY CTS OR MONKEY
	OV5670DB("OV5670Video exit:\n");
    return ERROR_NONE;
}


UINT32 OV5670Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

 	//kal_uint32 shutter = OV5670.shutter;

	if( SENSOR_MODE_CAPTURE== OV5670.sensorMode)
	{
		OV5670DB("OV5670Capture BusrtShot / ZSD!!!\n");
	}
	else
	{
		OV5670DB("OV5670Capture enter:\n");

		OV5670CaptureSetting();
	    mdelay(40);//THIS DELAY SHOULD BE NEED BY CTS OR MONKEY

		spin_lock(&ov5670mipiraw_drv_lock);
		OV5670.sensorMode = SENSOR_MODE_CAPTURE;
		OV5670.imgMirror = sensor_config_data->SensorImageMirror;
		OV5670.DummyPixels = 0;
		OV5670.DummyLines = 0 ;
		OV5670_FeatureControl_PERIOD_PixelNum = OV5670_FULL_PERIOD_PIXEL_NUMS + OV5670.DummyPixels;
		OV5670_FeatureControl_PERIOD_LineNum = OV5670_FULL_PERIOD_LINE_NUMS + OV5670.DummyLines;
		spin_unlock(&ov5670mipiraw_drv_lock);

		OV5670SetFlipMirror(IMAGE_H_MIRROR);

		OV5670DB("OV5670Capture exit:\n");
	}

	if(OV5670_During_testpattern == KAL_TRUE)
	{
		OV5670_write_cmos_sensor(0x5E00,0x80);
	}

    return ERROR_NONE;
}	



UINT32 OV5670GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{

    OV5670DB("OV5670GetResolution!!\n");

	pSensorResolution->SensorPreviewWidth	= OV5670_IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorPreviewHeight	= OV5670_IMAGE_SENSOR_PV_HEIGHT;
	
    pSensorResolution->SensorFullWidth		= OV5670_IMAGE_SENSOR_FULL_WIDTH;
    pSensorResolution->SensorFullHeight		= OV5670_IMAGE_SENSOR_FULL_HEIGHT;
	
    pSensorResolution->SensorVideoWidth		= OV5670_IMAGE_SENSOR_VIDEO_WIDTH;
    pSensorResolution->SensorVideoHeight    = OV5670_IMAGE_SENSOR_VIDEO_HEIGHT;
    return ERROR_NONE;
}   

UINT32 OV5670GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
                                                MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{

	spin_lock(&ov5670mipiraw_drv_lock);
	OV5670.imgMirror = pSensorConfigData->SensorImageMirror ;
	spin_unlock(&ov5670mipiraw_drv_lock);

    pSensorInfo->SensorOutputDataFormat= SENSOR_OUTPUT_FORMAT_RAW_B;
   
    pSensorInfo->SensorClockPolarity =SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;

    pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_MIPI;

    pSensorInfo->CaptureDelayFrame = 2;
    pSensorInfo->PreviewDelayFrame = 2;
    pSensorInfo->VideoDelayFrame = 2;

    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_8MA;
    pSensorInfo->AEShutDelayFrame = 0;	    
    pSensorInfo->AESensorGainDelayFrame = 0;
    pSensorInfo->AEISPGainDelayFrame = 2;

    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockRisingCount= 0;

            pSensorInfo->SensorGrabStartX = OV5670_PV_X_START;
            pSensorInfo->SensorGrabStartY = OV5670_PV_Y_START;

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;
			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	     	pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14;
	    	pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockRisingCount= 0;

            pSensorInfo->SensorGrabStartX = OV5670_VIDEO_X_START;
            pSensorInfo->SensorGrabStartY = OV5670_VIDEO_Y_START;

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;
			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	     	pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14;
	    	pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockRisingCount= 0;

            pSensorInfo->SensorGrabStartX = OV5670_FULL_X_START;	
            pSensorInfo->SensorGrabStartY = OV5670_FULL_Y_START;	

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;
			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14;
            pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
        default:
			pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockRisingCount= 0;

            pSensorInfo->SensorGrabStartX = OV5670_PV_X_START;
            pSensorInfo->SensorGrabStartY = OV5670_PV_Y_START;

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;
			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	     	pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14;
	    	pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
    }

    memcpy(pSensorConfigData, &OV5670SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

    return ERROR_NONE;
}   /* OV5670GetInfo() */



UINT32 OV5670Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
		spin_lock(&ov5670mipiraw_drv_lock);
		OV5670CurrentScenarioId = ScenarioId;
		spin_unlock(&ov5670mipiraw_drv_lock);
		
		OV5670DB("OV5670CurrentScenarioId=%d\n",OV5670CurrentScenarioId);

	switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            OV5670Preview(pImageWindow, pSensorConfigData);
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			OV5670DB("OV5670 video_preiew sync\n");
			OV5670Video(pImageWindow, pSensorConfigData);
			break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
            OV5670Capture(pImageWindow, pSensorConfigData);
            break;

        default:
            return ERROR_INVALID_SCENARIO_ID;

    }
    return ERROR_NONE;
} /* OV5670Control() */



kal_uint32 OV5670_SET_FrameLength_ByVideoMode(UINT16 Video_TargetFps)
{
    UINT32 frameRate = 0;
	kal_uint32 MIN_FrameLength=0;
	
	if(OV5670.OV5670AutoFlickerMode == KAL_TRUE)
	{
		if (Video_TargetFps==30)
			frameRate= OV5670_AUTOFLICKER_OFFSET_30;
		else if(Video_TargetFps==15)
			frameRate= OV5670_AUTOFLICKER_OFFSET_15;
		else
			frameRate=Video_TargetFps*10;
	
		MIN_FrameLength = (OV5670.videoPclk*10000)/(OV5670_VIDEO_PERIOD_PIXEL_NUMS + OV5670.DummyPixels)/frameRate*10;
	}
	else
		MIN_FrameLength = (OV5670.videoPclk*10000) /(OV5670_VIDEO_PERIOD_PIXEL_NUMS + OV5670.DummyPixels)/Video_TargetFps;

     return MIN_FrameLength;

}



UINT32 OV5670SetVideoMode(UINT16 u2FrameRate)
{

    kal_uint32 MIN_Frame_length =0,frameRate=0,extralines=0;
    OV5670DB("[OV5670SetVideoMode] frame rate = %d\n", u2FrameRate);

	spin_lock(&ov5670mipiraw_drv_lock);
	OV5670_VIDEO_MODE_TARGET_FPS=u2FrameRate;
	spin_unlock(&ov5670mipiraw_drv_lock);

	if(u2FrameRate==0)
	{
		OV5670DB("Disable Video Mode or dynimac fps\n");
		return KAL_TRUE;
	}
	if(u2FrameRate >30 || u2FrameRate <5)
	    OV5670DB("abmornal frame rate seting,pay attention~\n");

    if(OV5670.sensorMode == SENSOR_MODE_VIDEO)//video ScenarioId recording
    {

        MIN_Frame_length = OV5670_SET_FrameLength_ByVideoMode(u2FrameRate);

		if((MIN_Frame_length <=OV5670_VIDEO_PERIOD_LINE_NUMS))
		{
			MIN_Frame_length = OV5670_VIDEO_PERIOD_LINE_NUMS;
			OV5670DB("[OV5670SetVideoMode]current fps = %d\n", (OV5670.videoPclk*10000)  /(OV5670_VIDEO_PERIOD_PIXEL_NUMS)/OV5670_VIDEO_PERIOD_LINE_NUMS);
		}
		OV5670DB("[OV5670SetVideoMode]current fps (10 base)= %d\n", (OV5670.videoPclk*10000)*10/(OV5670_VIDEO_PERIOD_PIXEL_NUMS + OV5670.DummyPixels)/MIN_Frame_length);
		extralines = MIN_Frame_length - OV5670_VIDEO_PERIOD_LINE_NUMS;
		
		spin_lock(&ov5670mipiraw_drv_lock);
		OV5670.DummyPixels = 0;//define dummy pixels and lines
		OV5670.DummyLines = extralines ;
		spin_unlock(&ov5670mipiraw_drv_lock);
		
		OV5670_SetDummy(OV5670.DummyPixels,extralines);
    }
	
	OV5670DB("[OV5670SetVideoMode]MIN_Frame_length=%d,OV5670.DummyLines=%d\n",MIN_Frame_length,OV5670.DummyLines);

    return KAL_TRUE;
}


UINT32 OV5670SetAutoFlickerMode(kal_bool bEnable, UINT16 u2FrameRate)
{

	if(bEnable) {   
		spin_lock(&ov5670mipiraw_drv_lock);
		OV5670.OV5670AutoFlickerMode = KAL_TRUE;
		spin_unlock(&ov5670mipiraw_drv_lock);
        OV5670DB("OV5670 Enable Auto flicker\n");
    } else {
    	spin_lock(&ov5670mipiraw_drv_lock);
        OV5670.OV5670AutoFlickerMode = KAL_FALSE;
		spin_unlock(&ov5670mipiraw_drv_lock);
        OV5670DB("OV5670 Disable Auto flicker\n");
    }

    return ERROR_NONE;
}


UINT32 OV5670SetTestPatternMode(kal_bool bEnable)
{
    OV5670DB("[OV5670SetTestPatternMode] Test pattern enable:%d\n", bEnable);
    if(bEnable == KAL_TRUE)
    {
        OV5670_During_testpattern = KAL_TRUE;

		OV5670_write_cmos_sensor(0x5000,0x16);// ; LENC off, MWB on, BPC on, WPC on
		
		OV5670_write_cmos_sensor(0x5E00,0x80);
    }
	else
	{
        OV5670_During_testpattern = KAL_FALSE;
		OV5670_write_cmos_sensor(0x5000,0x96);// ; LENC on, MWB on, BPC on, WPC on
		OV5670_write_cmos_sensor(0x5E00,0x00);
	}

    return ERROR_NONE;
}


/*************************************************************************
*
* DESCRIPTION:
* INTERFACE FUNCTION, FOR USER TO SET MAX  FRAMERATE;
* 
*************************************************************************/
UINT32 OV5670MIPISetMaxFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 frameRate) {
	kal_uint32 pclk;
	kal_int16 dummyLine;
	kal_uint16 lineLength,frameHeight;
		
	OV5670DB("OV5670MIPISetMaxFramerateByScenario: scenarioId = %d, frame rate = %d\n",scenarioId,frameRate);
	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			pclk = OV5670_PREVIEW_PCLK;
			lineLength = OV5670_PV_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - OV5670_PV_PERIOD_LINE_NUMS;
			if(dummyLine<0)
				dummyLine = 0;
			spin_lock(&ov5670mipiraw_drv_lock);
			OV5670.sensorMode = SENSOR_MODE_PREVIEW;
			spin_unlock(&ov5670mipiraw_drv_lock);
			OV5670_SetDummy(0, dummyLine);			
			break;			
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			pclk = OV5670_VIDEO_PCLK;
			lineLength = OV5670_VIDEO_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - OV5670_VIDEO_PERIOD_LINE_NUMS;
			if(dummyLine<0)
				dummyLine = 0;
			spin_lock(&ov5670mipiraw_drv_lock);
			OV5670.sensorMode = SENSOR_MODE_VIDEO;
			spin_unlock(&ov5670mipiraw_drv_lock);
			OV5670_SetDummy(0, dummyLine);			
			break;			
			 break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:			
			pclk = OV5670_CAPTURE_PCLK;
			lineLength = OV5670_FULL_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - OV5670_FULL_PERIOD_LINE_NUMS;
			if(dummyLine<0)
				dummyLine = 0;
			spin_lock(&ov5670mipiraw_drv_lock);
			OV5670.sensorMode = SENSOR_MODE_CAPTURE;
			spin_unlock(&ov5670mipiraw_drv_lock);
			OV5670_SetDummy(0, dummyLine);			
			break;		
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW:
            break;
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
			break;
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE:   
			break;		
		default:
			break;
	}	
	return ERROR_NONE;
}


UINT32 OV5670MIPIGetDefaultFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 *pframeRate) 
{

	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			 *pframeRate = OV5670_MAX_FPS_PREVIEW;
			 break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			 *pframeRate = OV5670_MAX_FPS_CAPTURE;
			break;		
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added   
			 *pframeRate = OV5670_MAX_FPS_CAPTURE;
			break;		
		default:
			break;
	}

	return ERROR_NONE;
}


UINT32 OV5670FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
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
            *pFeatureReturnPara16++= OV5670_IMAGE_SENSOR_FULL_WIDTH;
            *pFeatureReturnPara16= OV5670_IMAGE_SENSOR_FULL_HEIGHT;
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_PERIOD:
				*pFeatureReturnPara16++= OV5670_FeatureControl_PERIOD_PixelNum;
				*pFeatureReturnPara16= OV5670_FeatureControl_PERIOD_LineNum;
				*pFeatureParaLen=4;
				break;
        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			switch(OV5670CurrentScenarioId)
			{
				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
					*pFeatureReturnPara32 = OV5670_PREVIEW_PCLK;
					*pFeatureParaLen=4;
					break;
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					*pFeatureReturnPara32 = OV5670_VIDEO_PCLK;
					*pFeatureParaLen=4;
					break;
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
				case MSDK_SCENARIO_ID_CAMERA_ZSD:
					*pFeatureReturnPara32 = OV5670_CAPTURE_PCLK;
					*pFeatureParaLen=4;
					break;
				default:
					*pFeatureReturnPara32 = OV5670_PREVIEW_PCLK;
					*pFeatureParaLen=4;
					break;
			}
		    break;

        case SENSOR_FEATURE_SET_ESHUTTER:
            OV5670_SetShutter(*pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_NIGHTMODE:
            OV5670_NightMode((BOOL) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_GAIN:  
           OV5670_SetGain((UINT16) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_FLASHLIGHT:
            break;
        case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
            //OV5670_isp_master_clock=*pFeatureData32;
            break;
        case SENSOR_FEATURE_SET_REGISTER:
            OV5670_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
            break;
        case SENSOR_FEATURE_GET_REGISTER:
            pSensorRegData->RegData = OV5670_read_cmos_sensor(pSensorRegData->RegAddr);
            break;
        case SENSOR_FEATURE_SET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            for (i=0;i<SensorRegNumber;i++)
            {
            	spin_lock(&ov5670mipiraw_drv_lock);
                OV5670SensorCCT[i].Addr=*pFeatureData32++;
                OV5670SensorCCT[i].Para=*pFeatureData32++;
				spin_unlock(&ov5670mipiraw_drv_lock);
            }
            break;
        case SENSOR_FEATURE_GET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=OV5670SensorCCT[i].Addr;
                *pFeatureData32++=OV5670SensorCCT[i].Para;
            }
            break;
        case SENSOR_FEATURE_SET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            for (i=0;i<SensorRegNumber;i++)
            {
            	spin_lock(&ov5670mipiraw_drv_lock);
                OV5670SensorReg[i].Addr=*pFeatureData32++;
                OV5670SensorReg[i].Para=*pFeatureData32++;
				spin_unlock(&ov5670mipiraw_drv_lock);
            }
            break;
        case SENSOR_FEATURE_GET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=OV5670SensorReg[i].Addr;
                *pFeatureData32++=OV5670SensorReg[i].Para;
            }
            break;
        case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
            if (*pFeatureParaLen>=sizeof(NVRAM_SENSOR_DATA_STRUCT))
            {
                pSensorDefaultData->Version=NVRAM_CAMERA_SENSOR_FILE_VERSION;
                pSensorDefaultData->SensorId=OV5670_SENSOR_ID;
                memcpy(pSensorDefaultData->SensorEngReg, OV5670SensorReg, sizeof(SENSOR_REG_STRUCT)*ENGINEER_END);
                memcpy(pSensorDefaultData->SensorCCTReg, OV5670SensorCCT, sizeof(SENSOR_REG_STRUCT)*FACTORY_END_ADDR);
            }
            else
                return FALSE;
            *pFeatureParaLen=sizeof(NVRAM_SENSOR_DATA_STRUCT);
            break;
        case SENSOR_FEATURE_GET_CONFIG_PARA:
            memcpy(pSensorConfigData, &OV5670SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
            *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
            break;
        case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
            OV5670_camera_para_to_sensor();
            break;

        case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
            OV5670_sensor_to_camera_para();
            break;
        case SENSOR_FEATURE_GET_GROUP_COUNT:
            *pFeatureReturnPara32++=OV5670_get_sensor_group_count();
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_GROUP_INFO:
            OV5670_get_sensor_group_info(pSensorGroupInfo->GroupIdx, pSensorGroupInfo->GroupNamePtr, &pSensorGroupInfo->ItemCount);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_GROUP_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_ITEM_INFO:
            OV5670_get_sensor_item_info(pSensorItemInfo->GroupIdx,pSensorItemInfo->ItemIdx, pSensorItemInfo);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_SET_ITEM_INFO:
            OV5670_set_sensor_item_info(pSensorItemInfo->GroupIdx, pSensorItemInfo->ItemIdx, pSensorItemInfo->ItemValue);
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
            OV5670SetVideoMode(*pFeatureData16);
            break;
        case SENSOR_FEATURE_CHECK_SENSOR_ID:
            OV5670GetSensorID(pFeatureReturnPara32);
            break;
        case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
            OV5670SetAutoFlickerMode((BOOL)*pFeatureData16, *(pFeatureData16+1));
	        break;
		case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
			OV5670MIPISetMaxFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, *(pFeatureData32+1));
			break;
		case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
			OV5670MIPIGetDefaultFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, (MUINT32 *)(*(pFeatureData32+1)));
			break;
		case SENSOR_FEATURE_SET_TEST_PATTERN:
			OV5670SetTestPatternMode((BOOL)*pFeatureData16);
			break;
		case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE://for factory mode auto testing 			
			*pFeatureReturnPara32=OV5670_TEST_PATTERN_CHECKSUM; 		  
			*pFeatureParaLen=4; 							
		     break;
        default:
            break;
    }
    return ERROR_NONE;
}	


SENSOR_FUNCTION_STRUCT	SensorFuncOV5670=
{
    OV5670Open,
    OV5670GetInfo,
    OV5670GetResolution,
    OV5670FeatureControl,
    OV5670Control,
    OV5670Close
};

UINT32 OV5670_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    if (pfFunc!=NULL)
        *pfFunc=&SensorFuncOV5670;

    return ERROR_NONE;
}  

