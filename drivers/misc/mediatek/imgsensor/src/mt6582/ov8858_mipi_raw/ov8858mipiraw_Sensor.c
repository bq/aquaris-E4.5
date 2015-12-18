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

#include "ov8858mipiraw_Sensor.h"
#include "ov8858mipiraw_Camera_Sensor_para.h"
#include "ov8858mipiraw_CameraCustomized.h"
static DEFINE_SPINLOCK(ov8858mipiraw_drv_lock);

#define OV8858_DEBUG
#ifdef OV8858_DEBUG
	#define OV8858DB  printk
#else
	#define OV8858DB(fmt, arg...)
#endif


kal_uint32 OV8858_FeatureControl_PERIOD_PixelNum=OV8858_PV_PERIOD_PIXEL_NUMS;
kal_uint32 OV8858_FeatureControl_PERIOD_LineNum=OV8858_PV_PERIOD_LINE_NUMS;

UINT16 OV8858_VIDEO_MODE_TARGET_FPS = 30;

MSDK_SCENARIO_ID_ENUM OV8858CurrentScenarioId = MSDK_SCENARIO_ID_CAMERA_PREVIEW;
MSDK_SENSOR_CONFIG_STRUCT OV8858SensorConfigData;
static OV8858_PARA_STRUCT ov8858;
kal_uint32 OV8858_FAC_SENSOR_REG;


SENSOR_REG_STRUCT OV8858SensorCCT[]=CAMERA_SENSOR_CCT_DEFAULT_VALUE;
SENSOR_REG_STRUCT OV8858SensorReg[ENGINEER_END]=CAMERA_SENSOR_REG_DEFAULT_VALUE;


//TODO~
#define OV8858_TEST_PATTERN_CHECKSUM 0x47a75476
kal_bool OV8858_During_testpattern = KAL_FALSE;

extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);

#define OV8858_write_cmos_sensor(addr, para) iWriteReg((u16) addr , (u32) para , 1, ov8858.write_id) //yanggy modify for I2c read >>>Do not modify this part when code merging,or I2C will not available
//Gionee yanggy add for CAM Performance Optimization begin
#ifdef ORIGINAL_VERSION 
#else
static struct ov8858_MIPI_otp_struct s_otp_wb;
static struct ov8858_MIPI_otp_struct s_otp_lenc;
static bool isNeedReadOtp = true;
#endif
//Gionee yanggy add for CAM Performance Optimization end
kal_uint16 OV8858_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
	iReadReg((u16) addr ,(u8*)&get_byte,ov8858.write_id);   //yanggy modify for I2c read >>>Do not modify this part when code merging,or I2C will not available
    return get_byte;
}


//Gionee yanggy 2013-09-03 add for module compability begin
#ifdef GN_MTK_BSP_LENS_COMPABILITY
static unsigned int OV8858_MIPI_Module_id=0;
static unsigned int OV8858_MIPI_Module_Lens_id =0;
#endif

//OV8858 OTP ¡§?|¡§??code
#if GN_MTK_BSP_LENS_COMPABILITY //ov8858 otp start
struct ov8858_MIPI_otp_struct
{
  kal_uint16 customer_id;
	kal_uint16 module_integrator_id;
	kal_uint16 lens_id;
	kal_uint16 rg_ratio;
	kal_uint16 bg_ratio;
	kal_uint16 light_rg;
	kal_uint16 light_bg;
	kal_uint16 user_data[5];
	kal_uint16 lenc[110];//63
};
//R/G and B/G ratio of typical camera module is defined here
kal_uint16 RG_Ratio_typical = 0x146; //RG_TYPICAL
kal_uint16 BG_Ratio_typical = 0x12a;//BG_TYPICAL

struct ov8858_MIPI_otp_struct current_otp={0};

kal_uint16 ov8858_mipi_check_otp_info(kal_uint16 index)
{
	kal_uint16 flag;

   	OV8858_write_cmos_sensor(0x3d84,0xc0); 		
   	//partial mode otp write start address
   	OV8858_write_cmos_sensor(0x3d88,0x70); 	
   	OV8858_write_cmos_sensor(0x3d89,0x10); 	
   	//partial mode otp write end address
   	OV8858_write_cmos_sensor(0x3d8a,0x70); 	
   	OV8858_write_cmos_sensor(0x3d8b,0x10); 	
   	
   	//read otp into buffer
   	OV8858_write_cmos_sensor(0x3d81,0x01); 
   	mdelay(10);
	
	  //select group
		flag =OV8858_read_cmos_sensor(0x7010);
  /* 	
   	switch(index)
   	{
   		case 1:
   			flag =(flag>>6)&0x03;
   			break;
   		case 2:
   			flag =(flag>>4)&0x03;
   			break;
   		case 3:
   			flag =(flag>>2)&0x03;	
   			break;
   		default :
   				break;
   	}
   	*/

	if(index == 1)
		flag =(flag>>6)&0x03;
	else if(index == 2)
		flag =(flag>>4)&0x03;
	else if(index == 3)
		flag =(flag>>2)&0x03;	
	
   	//clear otp buffer
   	OV8858_write_cmos_sensor(0x7010,0x00); 
   	
		if(flag ==0x00)
			{
				return 0;
			}	
			else if(flag & 0x02)
				{
					return 1;
				}
				else
					{
						return 2;
					}
	
}

kal_uint16 ov8858_mipi_read_otp_info(kal_uint16 index, struct ov8858_MIPI_otp_struct *otp)
{
    kal_uint16 i=0,temp=0;
    kal_uint16 start_addr=0,end_addr=0;
	  
    if(index == 1)
    	{
    		start_addr=0x7011;
    		end_addr  =0x7015;
    	}
   else  if(index == 2)
   	{
    		start_addr=0x7016;
    		end_addr  =0x701a;
    	}
   else  if(index == 3)
   	{
    		start_addr=0x701b;
    		end_addr  =0x701f;
    }
    
    OV8858_write_cmos_sensor(0x3d84,0xc0);
   	//partial mode otp write start address
    OV8858_write_cmos_sensor(0x3d88,(start_addr>>8)&0xff);
    OV8858_write_cmos_sensor(0x3d89,start_addr & 0xff);
    //partial mode otp write end address
    OV8858_write_cmos_sensor(0x3d8a,(end_addr>>8)&0xff);
    OV8858_write_cmos_sensor(0x3d8b,end_addr & 0xff);
    
    //read otp into buffer
    OV8858_write_cmos_sensor(0x3d81,0x01);
    
    mdelay(10);	
    otp->module_integrator_id = OV8858_read_cmos_sensor(start_addr);
    otp->lens_id=OV8858_read_cmos_sensor(start_addr+1);


		//clear otp buffer
		for(i=start_addr;i<end_addr;i++)
		{
			OV8858_write_cmos_sensor(i,0x00);
		}
		
		return 0;
}


kal_uint16 ov8858_mipi_check_otp_wb(kal_uint16 index)
{
   	kal_uint16 flag;

   	OV8858_write_cmos_sensor(0x3d84,0xc0); 		
   	//partial mode otp write start address
   	OV8858_write_cmos_sensor(0x3d88,0x70); 	
   	OV8858_write_cmos_sensor(0x3d89,0x20); 	
   	//partial mode otp write end address
   	OV8858_write_cmos_sensor(0x3d8a,0x70); 	
   	OV8858_write_cmos_sensor(0x3d8b,0x20); 	
   	
   	//read otp into buffer
   	OV8858_write_cmos_sensor(0x3d81,0x01); 
   	mdelay(10);
	
	  //select group
		flag =OV8858_read_cmos_sensor(0x7020);
   	
   	switch(index)
   	{
   		case 1:
   			flag =(flag>>6)&0x03;
   			break;
   		case 2:
   			flag =(flag>>4)&0x03;
   			break;
   		case 3:
   			flag =(flag>>2)&0x03;	
   			break;
   		default :
   				break;
   	}
   	
   	//clear otp buffer
   	OV8858_write_cmos_sensor(0x7020,0x00); 
   	
		if(flag ==0x00)
			{
				return 0;
			}	
			else if(flag & 0x02)
				{
					return 1;
				}
				else
					{
						return 2;
					}
}

//index:index of otp group.(0,1,2)
//return:	0.group index is empty.
//		1.group index has invalid data
//		2.group index has valid data

kal_uint16 ov8858_mipi_check_otp_lenc(kal_uint16 index)
{
  	kal_uint16 flag;

   	OV8858_write_cmos_sensor(0x3d84,0xc0); 		
   	//partial mode otp write start address
   	OV8858_write_cmos_sensor(0x3d88,0x70); 	
   	OV8858_write_cmos_sensor(0x3d89,0x3a); 	
   	//partial mode otp write end address
   	OV8858_write_cmos_sensor(0x3d8a,0x70); 	
   	OV8858_write_cmos_sensor(0x3d8b,0x3a); 	
   	
   	//read otp into buffer
   	OV8858_write_cmos_sensor(0x3d81,0x01); 
   	mdelay(10);
	
	  //select group
		flag =OV8858_read_cmos_sensor(0x703a);
   	switch(index)
   	{
   		case 1:
   			flag =(flag>>6)&0x03;
   			break;
   		case 2:
   			flag =(flag>>4)&0x03;
   			break;
   		case 3:
   			flag =(flag>>2)&0x03;	
   			break;
   		default :
   			OV8858DB("[ov8858_mipi_check_otp_lenc] index error\n");
   				break;
   	}
   	
   	//clear otp buffer
   	OV8858_write_cmos_sensor(0x703a,0x00); 
   	
		if(flag ==0x00)
			{
				return 0;
			}	
			else if(flag & 0x02)
				{
					return 1;
				}
				else
					{
						return 2;
					}
}

//index:index of otp group.(0,1,2)
//return: 0
kal_uint16 ov8858_mipi_read_otp_wb(kal_uint16 index, struct ov8858_MIPI_otp_struct *otp)
{
    kal_uint16 i=0,temp=0;
    kal_uint16 start_addr=0,end_addr=0;
	  
    switch(index)
    {
    	case 1:
    		start_addr=0x7021;
    		end_addr  =0x7025;
    		break;
    	case 2:
    		start_addr=0x7026;
    		end_addr  =0x702a;
    		break;
    	case 3:
    		start_addr=0x702b;
    		end_addr  =0x702f;
    		break;
    	default:
    		OV8858DB("[ov8858_MIPI_read_otp_wb] index error\n");
    		break;
    }
    
    
    OV8858_write_cmos_sensor(0x3d84,0xc0);
   	//partial mode otp write start address
    OV8858_write_cmos_sensor(0x3d88,(start_addr>>8)&0xff);
    OV8858_write_cmos_sensor(0x3d89,start_addr & 0xff);
    //partial mode otp write end address
    OV8858_write_cmos_sensor(0x3d8a,(end_addr>>8)&0xff);
    OV8858_write_cmos_sensor(0x3d8b,end_addr & 0xff);
    
    //read otp into buffer
    OV8858_write_cmos_sensor(0x3d81,0x01);
    
    mdelay(10);	
    temp =OV8858_read_cmos_sensor(start_addr +4);
    
    otp->rg_ratio=(OV8858_read_cmos_sensor(start_addr)<<2)+((temp>>6)&0x03);
    otp->bg_ratio=(OV8858_read_cmos_sensor(start_addr+1)<<2)+((temp>>4)&0x03);
 #if 1
	OV8858DB("[wujh_0616][ov8858_MIPI_read_otp_wb] rg_ratio=%x\n",otp->rg_ratio);
       OV8858DB("[wujh_0616][ov8858_MIPI_read_otp_wb] bg_ratio=%x\n",otp->bg_ratio);
 #endif

    otp->light_rg=0;
    otp->light_bg=0;
    
		//clear otp buffer
		for(i=start_addr;i<end_addr;i++)
		{
			OV8858_write_cmos_sensor(i,0x00);
		}
		
		return 0;
}

kal_uint16 ov8858_mipi_read_otp_lenc(kal_uint16 index,struct ov8858_MIPI_otp_struct *otp)
{
	  kal_uint16 i=0,temp=0;
    kal_uint16 start_addr=0,end_addr=0;
	 
	 switch(index)
	 {
	 	case 1:
	 			start_addr =0x703b;
	 			end_addr=0x70a8;
	 			break;
	 	case 2:
	 		  start_addr =0x70a9;
	 			end_addr=0x7116;
	 			break;
	 	case 3:
	 			start_addr =0x7117;
	 			end_addr=0x7184;
	 			break;
	 	default :
	 			OV8858DB("[ov8858_mipi_read_otp_lenc error [wangc_test]]");
	 			break;
	  }
	  
	  OV8858_write_cmos_sensor(0x3d84,0xc0);
	  //partial mode otp write start address
    OV8858_write_cmos_sensor(0x3d88,(start_addr>>8)&0xff);
    OV8858_write_cmos_sensor(0x3d89,start_addr & 0xff);
    //partial mode otp write end address
    OV8858_write_cmos_sensor(0x3d8a,(end_addr>>8)&0xff);
    OV8858_write_cmos_sensor(0x3d8b,end_addr & 0xff);
    
    //read otp into buffer
    OV8858_write_cmos_sensor(0x3d81,0x01);
	  mdelay(10);	
	  
	  for(i=0;i<110;i++)
	  {
	  	otp->lenc[i] = OV8858_read_cmos_sensor(start_addr+i);
	  }
  	
  	//clear otp buffer
  	for(i=start_addr;i<=end_addr;i++)
  	{
  		OV8858_write_cmos_sensor(i,0x00);
  	}
	  
	return 1;
}

//R_gain: red gain of sensor AWB, 0x400 = 1
//G_gain: green gain of sensor AWB, 0x400 = 1
//B_gain: blue gain of sensor AWB, 0x400 = 1
//reutrn 0
void ov8858_mipi_update_wb_gain(kal_uint32 R_gain, kal_uint32 G_gain, kal_uint32 B_gain)
{   
	if(R_gain > 0x400)
		{
			OV8858_write_cmos_sensor(0x5032,R_gain >> 8);
			OV8858_write_cmos_sensor(0x5033,(R_gain&0x00ff));
		}
	if(G_gain > 0x400)
		{
			OV8858_write_cmos_sensor(0x5034,G_gain >> 8);
			OV8858_write_cmos_sensor(0x5035,(G_gain&0x00ff));
		}
	if(B_gain >0x400)
		{
			OV8858_write_cmos_sensor(0x5036,B_gain >> 8);
			OV8858_write_cmos_sensor(0x5037,(B_gain&0x00ff));
		}
	OV8858DB("[ov8858_MIPI_update_wb_gain]R_gain[%x]G_gain[%x]B_gain[%x]\n",R_gain,G_gain,B_gain);
	OV8858DB("[ov8858_MIPI_update_wb_gain_Finished[wangc_test]]");
}

void ov8858_mipi_update_lenc(struct ov8858_MIPI_otp_struct *otp)
{
    kal_uint16 i=0, temp=0;
    
    temp=OV8858_read_cmos_sensor(0x5000);
    temp =temp | 0x80;
  //  OV8858_write_cmos_sensor(0x5800,temp);
	OV8858_write_cmos_sensor(0x5000,temp);

    
    for(i = 0; i < 110; i++)
    {
       OV8858_write_cmos_sensor(0x5800+i,otp->lenc[i]);
       OV8858DB("[ov8858_MIPI_update_lenc]otp->lenc[%d][%x]\n",i,otp->lenc[i]);
     }
    OV8858DB("[ov8858_MIPI_update_lenc_Finished]\n");

}

//call this function after OV5650 initialization
//return value:	0 update success
//				1 no 	OTP
kal_uint16 ov8858_mipi_update_wb_register_from_otp(void)
{
	kal_uint16 temp=0, i=0, otp_index=0,rg=0,bg=0;
	kal_uint32 awb_rgb_gain[2]={0};
	kal_uint32 R_gain=0, B_gain=0, G_gain=0;
	OV8858DB("ov8858_MIPI_update_wb_register_from_otp_Start[wangc_test]\n");
	//check first wb OTP with valid OTP
//Gionee yanggy add for CAM Performance Optimization begin
#ifdef ORIGINAL_VERSION 
	for(i = 1; i <= 3; i++)
		{
			temp = ov8858_mipi_check_otp_wb(i);
			if(temp == 2)
				{
					otp_index = i;
					break;
				}
		}
	if( i >4)//wangc modified
		{
		 	OV8858DB("[ov8858_mipi_update_wb_register_from_otp] OTP AWB ERROR\r\n");
			return 0;//wangc_test_ov8858_mipi_update_wb_register_from_otp invalid
		}
	/*	
	if(!ov8858_mipi_read_otp_wb(otp_index,&current_otp))//wangc added for checking read_otp_wb 
		{
			return 0;//wangc_test_ov8858_mipi_update_wb_register_from_otp invalid
		}       
		*/

    ov8858_mipi_read_otp_wb(otp_index,&current_otp);
#else
    if( true == isNeedReadOtp)
    {
        for(i = 1; i <= 3; i++)
        {
            temp = ov8858_mipi_check_otp_wb(i);
            if(temp == 2)
            {
                otp_index = i;
                break;
            }
        }
        if( i >4)//wangc modified
        {
            OV8858DB("[ov8858_mipi_update_wb_register_from_otp] OTP AWB ERROR\r\n");
            return 0;//wangc_test_ov8858_mipi_update_wb_register_from_otp invalid
        }
        ov8858_mipi_read_otp_wb(otp_index,&current_otp);
        s_otp_wb = current_otp;
    }
    else 
    {
        current_otp = s_otp_wb;
    }
#endif
    //Gionee yanggy add for CAM Performance Optimization end	
	//calculate gain
	//0x400 = 1x gain
	if(current_otp.light_rg==0)
		{
		//if no light source information in OTP,light factor=1;
		rg=current_otp.rg_ratio;
	      }
	else
		{
		rg = current_otp.rg_ratio * (current_otp.light_rg +512) / 1024;   
		}

	if(current_otp.light_bg==0)
		{
		//if no light source information in OTP,light factor=1;
				bg=current_otp.bg_ratio;
	  }
	else
		{
        bg = current_otp.bg_ratio * (current_otp.light_bg +512) / 1024;  
		}
	//calculate G gain	
	awb_rgb_gain[0] =RG_Ratio_typical*1024/rg;
	awb_rgb_gain[1] =BG_Ratio_typical*1024/bg;
	
	if((awb_rgb_gain[0]<1024)||(awb_rgb_gain[1]<1024))
		{
			if(awb_rgb_gain[0]<awb_rgb_gain[1])
				{
					R_gain = 0x400;
					G_gain = R_gain * rg / RG_Ratio_typical;
					B_gain = G_gain * BG_Ratio_typical /bg;
				}
			else
				{
					B_gain = 0x400;
					G_gain = B_gain * bg / BG_Ratio_typical;
					R_gain = G_gain * RG_Ratio_typical /rg;
				}

		}
	else
		{
			G_gain = 0x400;
			R_gain = G_gain * RG_Ratio_typical / rg;
			B_gain = G_gain * BG_Ratio_typical /bg;
		}
	//write sensor wb gain to register
	OV8858DB("[R_gain=%x,G_gain=%x,B_Gain=%x",R_gain,G_gain,B_gain);
	ov8858_mipi_update_wb_gain(R_gain,G_gain,B_gain);
	OV8858DB("[ov8858_mipi_update_wb_register_from_otp_Finished[wangc_test]\n]");
    return 1;//wangc_test_ov8858_mipi_update_wb_register_from_otp invalid
}

//call this function after ov5650 initialization
//return value:	0 update success
//				1 no otp

kal_uint16 ov8858_mipi_update_lenc_register_from_otp(void)
{
	   kal_uint16 temp,i,otp_index;

    OV8858DB("ov8858_mipi_update_lenc_register_from_otp_Start[wangc_test]\n");
//Gionee yanggy add for CAM Performance Optimization begin
#ifdef ORIGINAL_VERSION
    for(i = 1; i<4;i++)
	{
		temp = ov8858_mipi_check_otp_lenc(i);
		if(2 == temp)
		{
			otp_index = i;
			break;
		}
	}
	if(i == 4) //wangc modified
		{
		 	OV8858DB("[ov8858_mipi_update_lenc_register_from_otp]no valid wb OTP data!\r\n");
			return 0;//wangc_test_ov8858_mipi_update_lenc_register_from_otp invalid
        }
    if(!ov8858_mipi_read_otp_lenc(otp_index,&current_otp))
    {
        return 0;//wangc_test_ov8858_mipi_update_lenc_register_from_otp invalid
    }
#else
    if(true == isNeedReadOtp)
    {
        for(i = 1; i<4;i++)
        {
            temp = ov8858_mipi_check_otp_lenc(i);
            if(2 == temp)
            {
                otp_index = i;
                break;
            }
        }
        if(i == 4) //wangc modified
        {
            OV8858DB("[ov8858_mipi_update_lenc_register_from_otp]no valid wb OTP data!\r\n");
            return 0;//wangc_test_ov8858_mipi_update_lenc_register_from_otp invalid
        }
        if(!ov8858_mipi_read_otp_lenc(otp_index,&current_otp))
        {
            return 0;//wangc_test_ov8858_mipi_update_lenc_register_from_otp invalid
        }
        s_otp_lenc =  current_otp;
        isNeedReadOtp = false;
    }
    else
    {
        current_otp = s_otp_lenc;
    }
#endif
    //Gionee yanggy add for CAM Performance Optimization end
	ov8858_mipi_update_lenc(&current_otp);
	OV8858DB("ov8858_mipi_update_lenc_register_from_otp_Finished[wangc_test]\n");
	return 1;// wangc_test_ov8858_mipi_update_lenc_register_from_otp success
}

#endif

/*****************************  OTP Feature  End**********************************/



//#endif //ov8858 otp end



//Gionee yanggy 2013-09-03 add for module compability end




void OV8858_Init_Para(void)
{

	spin_lock(&ov8858mipiraw_drv_lock);
	ov8858.sensorMode = SENSOR_MODE_INIT;
	ov8858.OV8858AutoFlickerMode = KAL_FALSE;
	ov8858.OV8858VideoMode = KAL_FALSE;
	ov8858.DummyLines= 0;
	ov8858.DummyPixels= 0;
	ov8858.pvPclk =  (7200); 
	ov8858.videoPclk = (14400);
	ov8858.capPclk = (14400);

	ov8858.shutter = 0x4C00;
	ov8858.ispBaseGain = BASEGAIN;		//64
	ov8858.sensorGlobalGain = 0x0200;  //512
	spin_unlock(&ov8858mipiraw_drv_lock);
}

kal_uint32 GetOv8858LineLength(void)
{
	kal_uint32 OV8858_line_length = 0;
	if ( SENSOR_MODE_PREVIEW == ov8858.sensorMode )  
	{
		OV8858_line_length = OV8858_PV_PERIOD_PIXEL_NUMS + ov8858.DummyPixels;
	}
	else if( SENSOR_MODE_VIDEO == ov8858.sensorMode ) 
	{
		OV8858_line_length = OV8858_VIDEO_PERIOD_PIXEL_NUMS + ov8858.DummyPixels;
	}
	else
	{
		OV8858_line_length = OV8858_FULL_PERIOD_PIXEL_NUMS + ov8858.DummyPixels;
	}
	
#ifdef OV8858_DEBUG
	OV8858DB("[GetOv8858LineLength]: ov8858.sensorMode = %d, OV8858_line_length =%d, ov8858.DummyPixels = %d\n", ov8858.sensorMode,OV8858_line_length,ov8858.DummyPixels);
#endif


    return OV8858_line_length;

}


kal_uint32 GetOv8858FrameLength(void)
{
	kal_uint32 OV8858_frame_length = 0;

	if ( SENSOR_MODE_PREVIEW == ov8858.sensorMode )  
	{
		OV8858_frame_length = OV8858_PV_PERIOD_LINE_NUMS + ov8858.DummyLines ;
	}
	else if( SENSOR_MODE_VIDEO == ov8858.sensorMode ) 
	{
		OV8858_frame_length = OV8858_VIDEO_PERIOD_LINE_NUMS + ov8858.DummyLines ;
	}
	else
	{
		OV8858_frame_length = OV8858_FULL_PERIOD_LINE_NUMS + ov8858.DummyLines ;
	}

#ifdef OV8858_DEBUG
		OV8858DB("[GetOv8858FrameLength]: ov8858.sensorMode = %d, OV8858_frame_length =%d, ov8858.DummyLines = %d\n", ov8858.sensorMode,OV8858_frame_length,ov8858.DummyLines);
#endif


	return OV8858_frame_length;
}


kal_uint32 OV8858_CalcExtra_For_ShutterMargin(kal_uint32 shutter_value,kal_uint32 shutterLimitation)
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

#ifdef OV8858_DEBUG
			OV8858DB("[OV8858_CalcExtra_For_ShutterMargin]: shutter_value = %d, shutterLimitation =%d, extra_lines = %d\n", shutter_value,shutterLimitation,extra_lines);
#endif

    return extra_lines;

}


//TODO~
kal_uint32 OV8858_CalcFrameLength_For_AutoFlicker(void)
{

    kal_uint32 AutoFlicker_min_framelength = 0;

	switch(OV8858CurrentScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			AutoFlicker_min_framelength = (ov8858.capPclk*10000) /(OV8858_FULL_PERIOD_LINE_NUMS + ov8858.DummyPixels)/OV8858_AUTOFLICKER_OFFSET_30*10 ;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			if(OV8858_VIDEO_MODE_TARGET_FPS==30)
			{
				AutoFlicker_min_framelength = (ov8858.videoPclk*10000) /(OV8858_VIDEO_PERIOD_PIXEL_NUMS + ov8858.DummyPixels)/OV8858_AUTOFLICKER_OFFSET_30*10 ;
			}
			else if(OV8858_VIDEO_MODE_TARGET_FPS==15)
			{
				AutoFlicker_min_framelength = (ov8858.videoPclk*10000) /(OV8858_VIDEO_PERIOD_PIXEL_NUMS + ov8858.DummyPixels)/OV8858_AUTOFLICKER_OFFSET_15*10 ;
			}
			else
			{
				AutoFlicker_min_framelength = OV8858_VIDEO_PERIOD_LINE_NUMS + ov8858.DummyLines;
			}
			break;
			
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			AutoFlicker_min_framelength = (ov8858.pvPclk*10000) /(OV8858_PV_PERIOD_PIXEL_NUMS + ov8858.DummyPixels)/OV8858_AUTOFLICKER_OFFSET_30*10 ;
			break;
	}

	#ifdef OV8858_DEBUG 
	OV8858DB("AutoFlicker_min_framelength =%d,OV8858CurrentScenarioId =%d\n", AutoFlicker_min_framelength,OV8858CurrentScenarioId);
	#endif

	return AutoFlicker_min_framelength;

}


void OV8858_write_shutter(kal_uint32 shutter)
{
	//kal_uint32 min_framelength = OV8858_PV_PERIOD_PIXEL_NUMS;
	//the init code write as up line;
	//modify it as follow
	kal_uint32 min_framelength = OV8858_PV_PERIOD_LINE_NUMS;
	kal_uint32 max_shutter=0;
	kal_uint32 line_length = 0;
	kal_uint32 frame_length = 0;
	unsigned long flags;
//Gionee <wujh><2014-06-05> modify for CR01284824 <fix frame rate in 25fps>  begin
#ifdef ORIGINAL_VERSION
#else 
	kal_uint16 realtime_fp = 0;
#endif
//Gionee <wujh><2014-06-05> modify for CR01284824 <fix frame rate in 25fps>  end
	//TODO~
	kal_uint32 read_shutter_1 = 0;
	kal_uint32 read_shutter_2 = 0;
	kal_uint32 read_shutter_3 = 0;

	//TODO~
    if(shutter > 0x90f7)//500ms for capture SaturationGain
    {
    	#ifdef OV8858_DEBUG
		OV8858DB("[OV8858_write_shutter] shutter > 0x90f7 [warn.] shutter=%x, \n", shutter);
		#endif
		shutter = 0x90f7;
    }
	
    line_length  = GetOv8858LineLength();
	frame_length = GetOv8858FrameLength();
	
	max_shutter  = frame_length-OV8858_SHUTTER_MARGIN;

    frame_length = frame_length + OV8858_CalcExtra_For_ShutterMargin(shutter,max_shutter);
	


	if(ov8858.OV8858AutoFlickerMode == KAL_TRUE)
	{
        min_framelength = OV8858_CalcFrameLength_For_AutoFlicker();

        if(frame_length < min_framelength)
			frame_length = min_framelength;
	}
	

	spin_lock_irqsave(&ov8858mipiraw_drv_lock,flags);
	OV8858_FeatureControl_PERIOD_PixelNum = line_length;
	OV8858_FeatureControl_PERIOD_LineNum = frame_length;
	spin_unlock_irqrestore(&ov8858mipiraw_drv_lock,flags);

//Gionee <wujh><2014-06-05> modify for CR01284824<fix frame rate at 25fps>  begin
#ifdef ORIGINAL_VERSION
#else 
	if (ov8858.sensorMode==SENSOR_MODE_CAPTURE)
		{
		OV8858DB("[wujh_test]Preview mode\n");
		  if(shutter< (OV8858_CAPTURE_PCLK/(OV8858_FULL_PERIOD_PIXEL_NUMS+50)/100))
            		{            
	
				OV8858DB("[wujh_test]shutter<0x16f\n");
				realtime_fp=250;
	      			spin_lock_irqsave(&ov8858mipiraw_drv_lock,flags);
             			frame_length = OV8858_CAPTURE_PCLK *10 / (OV8858_FULL_PERIOD_PIXEL_NUMS * realtime_fp);
	      			spin_unlock_irqrestore(&ov8858mipiraw_drv_lock,flags);
               		OV8858DB("[realtime_fp=250][height:%d]",frame_length);
             		}
		}
#endif
//Gionee <wujh><2014-06-05> modify for CR01284824 <fix frame rate at 25fps>  end
	//Set total frame length
	OV8858_write_cmos_sensor(0x380e, (frame_length >> 8) & 0xFF);
	OV8858_write_cmos_sensor(0x380f, frame_length & 0xFF);
	
	//Set shutter 
	OV8858_write_cmos_sensor(0x3500, (shutter>>12) & 0x0F);
	OV8858_write_cmos_sensor(0x3501, (shutter>>4) & 0xFF);
	OV8858_write_cmos_sensor(0x3502, (shutter<<4) & 0xF0);	

	#ifdef OV8858_DEBUG
	OV8858DB("[OV8858_write_shutter]ov8858 write shutter=%x, line_length=%x, frame_length=%x\n", shutter, line_length, frame_length);
	#endif

}


void OV8858_SetShutter(kal_uint32 iShutter)
{

   spin_lock(&ov8858mipiraw_drv_lock);
   ov8858.shutter= iShutter;
   spin_unlock(&ov8858mipiraw_drv_lock);

   OV8858_write_shutter(iShutter);
   return;
}


UINT32 OV8858_read_shutter(void)
{

	kal_uint16 temp_reg1, temp_reg2 ,temp_reg3;
	UINT32 shutter =0;
	temp_reg1 = OV8858_read_cmos_sensor(0x3500);    // AEC[b19~b16]
	temp_reg2 = OV8858_read_cmos_sensor(0x3501);    // AEC[b15~b8]
	temp_reg3 = OV8858_read_cmos_sensor(0x3502);    // AEC[b7~b0]
	
	shutter  = (temp_reg1 <<12)| (temp_reg2<<4)|(temp_reg3>>4);

	return shutter;
}

static kal_uint16 OV8858Reg2Gain(const kal_uint16 iReg)
{
    kal_uint16 iGain =0; 

	iGain = iReg*BASEGAIN/OV8858_GAIN_BASE;
	return iGain;
}

static kal_uint16 OV8858Gain2Reg(const kal_uint32 iGain)
{
    kal_uint32 iReg = 0x0000;

	iReg = iGain * 2; //(iGain/BASEGAIN)*OV8858_GAIN_BASE;

    return iReg;
}

void write_OV8858_gain(kal_uint16 gain)
{
	//kal_uint16 read_gain=0;

	OV8858_write_cmos_sensor(0x3508,(gain>>8));
	OV8858_write_cmos_sensor(0x3509,(gain&0xff));

	//read_gain=(((OV8858_read_cmos_sensor(0x3508)&0x1F) << 8) | OV8865_read_cmos_sensor(0x3509));
	//OV8858DB("[OV8858_SetGain]0x3508|0x3509=0x%x \n",read_gain);

	return;
}

void OV8858_SetGain(UINT16 iGain)
{
	unsigned long flags;

	
	OV8858DB("OV8858_SetGain iGain = %d :\n ",iGain);

	spin_lock_irqsave(&ov8858mipiraw_drv_lock,flags);
	ov8858.realGain = iGain;
	ov8858.sensorGlobalGain = OV8858Gain2Reg(iGain);
	spin_unlock_irqrestore(&ov8858mipiraw_drv_lock,flags);
	write_OV8858_gain(ov8858.sensorGlobalGain);
	#ifdef OV8858_DEBUG
	OV8858DB(" [OV8858_SetGain]ov8858.sensorGlobalGain=0x%x,ov8858.realGain =0x%x",ov8858.sensorGlobalGain,ov8858.realGain); 
	#endif
	//temperature test
	//OV8858_write_cmos_sensor(0x4d12,0x01);
	//OV8858DB("Temperature read_reg  0x4d13  =%x \n",OV8865_read_cmos_sensor(0x4d13));
}   

kal_uint16 read_OV8858_gain(void)
{
	kal_uint16 read_gain=0;

	read_gain=(((OV8858_read_cmos_sensor(0x3508)&0x1F) << 8) | OV8858_read_cmos_sensor(0x3509));

	spin_lock(&ov8858mipiraw_drv_lock);
	ov8858.sensorGlobalGain = read_gain;
	ov8858.realGain = OV8858Reg2Gain(ov8858.sensorGlobalGain);
	spin_unlock(&ov8858mipiraw_drv_lock);

	OV8858DB("ov8858.sensorGlobalGain=0x%x,ov8858.realGain=%d\n",ov8858.sensorGlobalGain,ov8858.realGain);

	return ov8858.sensorGlobalGain;
}  



static void OV8858_SetDummy( const kal_uint32 iPixels, const kal_uint32 iLines )
{
	kal_uint32 line_length = 0;
	kal_uint32 frame_length = 0;

	if ( SENSOR_MODE_PREVIEW == ov8858.sensorMode )
	{
		line_length = OV8858_PV_PERIOD_PIXEL_NUMS + iPixels;
		frame_length = OV8858_PV_PERIOD_LINE_NUMS + iLines;
	}
	else if( SENSOR_MODE_VIDEO== ov8858.sensorMode )
	{
		line_length = OV8858_VIDEO_PERIOD_PIXEL_NUMS + iPixels;
		frame_length = OV8858_VIDEO_PERIOD_LINE_NUMS + iLines;
	}
	else
	{
		line_length = OV8858_FULL_PERIOD_PIXEL_NUMS + iPixels;
		frame_length = OV8858_FULL_PERIOD_LINE_NUMS + iLines;
	}

	spin_lock(&ov8858mipiraw_drv_lock);
	OV8858_FeatureControl_PERIOD_PixelNum = line_length;
	OV8858_FeatureControl_PERIOD_LineNum = frame_length;
	spin_unlock(&ov8858mipiraw_drv_lock);

	//Set total frame length
	OV8858_write_cmos_sensor(0x380e, (frame_length >> 8) & 0xFF);
	OV8858_write_cmos_sensor(0x380f, frame_length & 0xFF);
	//Set total line length
	OV8858_write_cmos_sensor(0x380c, (line_length >> 8) & 0xFF);
	OV8858_write_cmos_sensor(0x380d, line_length & 0xFF);

	#ifdef OV8858_DEBUG
	OV8858DB(" [OV8858_SetDummy]ov8858.sensorMode = %d, line_length = %d,iPixels = %d, frame_length =%d, iLines = %d\n",ov8858.sensorMode, line_length,iPixels, frame_length, iLines); 
	#endif

}   


#if 1
void OV8858_camera_para_to_sensor(void)
{
    kal_uint32    i;
    for(i=0; 0xFFFFFFFF!=OV8858SensorReg[i].Addr; i++)
    {
        OV8858_write_cmos_sensor(OV8858SensorReg[i].Addr, OV8858SensorReg[i].Para);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=OV8858SensorReg[i].Addr; i++)
    {
        OV8858_write_cmos_sensor(OV8858SensorReg[i].Addr, OV8858SensorReg[i].Para);
    }
    for(i=FACTORY_START_ADDR; i<FACTORY_END_ADDR; i++)
    {
        OV8858_write_cmos_sensor(OV8858SensorCCT[i].Addr, OV8858SensorCCT[i].Para);
    }
}

void OV8858_sensor_to_camera_para(void)
{
    kal_uint32    i, temp_data;
    for(i=0; 0xFFFFFFFF!=OV8858SensorReg[i].Addr; i++)
    {
         temp_data = OV8858_read_cmos_sensor(OV8858SensorReg[i].Addr);
		 spin_lock(&ov8858mipiraw_drv_lock);
		 OV8858SensorReg[i].Para =temp_data;
		 spin_unlock(&ov8858mipiraw_drv_lock);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=OV8858SensorReg[i].Addr; i++)
    {
        temp_data = OV8858_read_cmos_sensor(OV8858SensorReg[i].Addr);
		spin_lock(&ov8858mipiraw_drv_lock);
		OV8858SensorReg[i].Para = temp_data;
		spin_unlock(&ov8858mipiraw_drv_lock);
    }
}

kal_int32  OV8858_get_sensor_group_count(void)
{
    return GROUP_TOTAL_NUMS;
}

void OV8858_get_sensor_group_info(kal_uint16 group_idx, kal_int8* group_name_ptr, kal_int32* item_count_ptr)
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

void OV8858_get_sensor_item_info(kal_uint16 group_idx,kal_uint16 item_idx, MSDK_SENSOR_ITEM_INFO_STRUCT* info_ptr)
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

            temp_para= OV8858SensorCCT[temp_addr].Para;
			//temp_gain= (temp_para/ov8865.sensorBaseGain) * 1000;

            info_ptr->ItemValue=temp_gain;
            info_ptr->IsTrueFalse=KAL_FALSE;
            info_ptr->IsReadOnly=KAL_FALSE;
            info_ptr->IsNeedRestart=KAL_FALSE;
            info_ptr->Min= OV8858_MIN_ANALOG_GAIN * 1000;
            info_ptr->Max= OV8858_MAX_ANALOG_GAIN * 1000;
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



kal_bool OV8858_set_sensor_item_info(kal_uint16 group_idx, kal_uint16 item_idx, kal_int32 ItemValue)
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
//             temp_para=(temp_gain * ov8865.sensorBaseGain + BASEGAIN/2)/BASEGAIN;
          }
          else
			  ASSERT(0);

		  spin_lock(&ov8858mipiraw_drv_lock);
          OV8858SensorCCT[temp_addr].Para = temp_para;
		  spin_unlock(&ov8858mipiraw_drv_lock);
          OV8858_write_cmos_sensor(OV8858SensorCCT[temp_addr].Addr,temp_para);

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
					spin_lock(&ov8858mipiraw_drv_lock);
                    OV8858_FAC_SENSOR_REG=ItemValue;
					spin_unlock(&ov8858mipiraw_drv_lock);
                    break;
                case 1:
                    OV8858_write_cmos_sensor(OV8858_FAC_SENSOR_REG,ItemValue);
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

void OV8858_1224pSetting(void)
{

	/*   Preview of ov8858 setting                                 */
	/*   @@5.1.2.2 Raw 10bit 1632x1224 30fps 2lane 720M bps/lane   */
	/*   ;Pclk 72MHz                                               */
	/*   ;pixels per line=1928(0x788)                              */
	/*   ;lines per frame=1244(0x4dc)                              */
	OV8858_write_cmos_sensor(0x0100, 0x00); //
	OV8858_write_cmos_sensor(0x030e, 0x00); // ; pll2_rdiv
	OV8858_write_cmos_sensor(0x030f, 0x09); // ; pll2_divsp
	OV8858_write_cmos_sensor(0x0312, 0x01); // ; pll2_pre_div0, pll2_r_divdac
	OV8858_write_cmos_sensor(0x3015, 0x01); //
	OV8858_write_cmos_sensor(0x3501, 0x4d); // ; exposure M
	OV8858_write_cmos_sensor(0x3502, 0x40); // ; exposure L
	OV8858_write_cmos_sensor(0x3508, 0x04); // ; gain H
	OV8858_write_cmos_sensor(0x3706, 0x35); //
	OV8858_write_cmos_sensor(0x370a, 0x00); //
	OV8858_write_cmos_sensor(0x370b, 0xb5); //
	OV8858_write_cmos_sensor(0x3778, 0x1b); //
	OV8858_write_cmos_sensor(0x3808, 0x06); // ; x output size H 1632 
	OV8858_write_cmos_sensor(0x3809, 0x60); // ; x output size L
	OV8858_write_cmos_sensor(0x380a, 0x04); // ; y output size H 1224
	OV8858_write_cmos_sensor(0x380b, 0xc8); // ; y output size L
	OV8858_write_cmos_sensor(0x380c, 0x07); // ; HTS H
	OV8858_write_cmos_sensor(0x380d, 0x88); // ; HTS L
	OV8858_write_cmos_sensor(0x380e, 0x04); // ; VTS H
	OV8858_write_cmos_sensor(0x380f, 0xdc); // ; VTS L
	OV8858_write_cmos_sensor(0x3814, 0x03); // ; x odd inc
	OV8858_write_cmos_sensor(0x3821, 0x61); // ; mirror on, bin on
	OV8858_write_cmos_sensor(0x382a, 0x03); // ; y odd inc
	OV8858_write_cmos_sensor(0x3830, 0x08); //
	OV8858_write_cmos_sensor(0x3836, 0x02); //
	OV8858_write_cmos_sensor(0x3f0a, 0x80); //
	OV8858_write_cmos_sensor(0x4001, 0x10); // ; total 128 black column
	OV8858_write_cmos_sensor(0x4022, 0x04); // ; Anchor left end H
	OV8858_write_cmos_sensor(0x4023, 0xb9); // ; Anchor left end L
	OV8858_write_cmos_sensor(0x4024, 0x05); // ; Anchor right start H
	OV8858_write_cmos_sensor(0x4025, 0x2a); // ; Anchor right start L
	OV8858_write_cmos_sensor(0x4026, 0x05); // ; Anchor right end H
	OV8858_write_cmos_sensor(0x4027, 0x2b); // ; Anchor right end L
	OV8858_write_cmos_sensor(0x402b, 0x04); // ; top black line number
	OV8858_write_cmos_sensor(0x402e, 0x08); // ; bottom black line start
	OV8858_write_cmos_sensor(0x4500, 0x38); //
	OV8858_write_cmos_sensor(0x4600, 0x00); //
	OV8858_write_cmos_sensor(0x4601, 0xcb); //
	OV8858_write_cmos_sensor(0x382d, 0x7f); //
	OV8858_write_cmos_sensor(0x0100, 0x01); //
}

void OV8858PreviewSetting(void)
{
	// ++++++++  @@5.1.1.2 Raw 10bit 1632x1224 30fps 4lane 672M bps/lane
	// ++++++++  ;;
	// ++++++++  ;; MIPI=672Mbps, SysClk=72Mhz,Dac Clock=360Mhz.
	// ++++++++  ;;
	// ++++++++  ;Pclk 72MHz
	// ++++++++  ;pixels per line=1928(0x788) 
	// ++++++++  ;lines per frame=1244(0x4dc)
		//OV8858_write_cmos_sensor( 0x0100, 0x00);  //
		//OV8858_write_cmos_sensor( 0x030f, 0x09);  // ; pll2_divsp
		//OV8858_write_cmos_sensor( 0x3501, 0x4d);  // ; exposure M
		//OV8858_write_cmos_sensor( 0x3502, 0x40);  // ; exposure L
		//OV8858_write_cmos_sensor( 0x3508, 0x04);  // ; gain H
		//OV8858_write_cmos_sensor( 0x3700, 0x18);  //
		//OV8858_write_cmos_sensor( 0x3701, 0x0c);  //
		//OV8858_write_cmos_sensor( 0x3702, 0x28);  //
		//OV8858_write_cmos_sensor( 0x3703, 0x19);  //
		//OV8858_write_cmos_sensor( 0x3704, 0x14);  //
		//OV8858_write_cmos_sensor( 0x3706, 0x35);  //
		//OV8858_write_cmos_sensor( 0x3707, 0x04);  //
		//OV8858_write_cmos_sensor( 0x3708, 0x24);  //
		//OV8858_write_cmos_sensor( 0x3709, 0x33);  //
		//OV8858_write_cmos_sensor( 0x370a, 0x00);  //
		//OV8858_write_cmos_sensor( 0x370b, 0xb5);  //
		//OV8858_write_cmos_sensor( 0x370c, 0x04);  //
		//OV8858_write_cmos_sensor( 0x3718, 0x12);  //
		//OV8858_write_cmos_sensor( 0x3712, 0x42);  //
		//OV8858_write_cmos_sensor( 0x371e, 0x19);  //
		//OV8858_write_cmos_sensor( 0x371f, 0x40);  //
		//OV8858_write_cmos_sensor( 0x3720, 0x05);  //
		//OV8858_write_cmos_sensor( 0x3721, 0x05);  //
		//OV8858_write_cmos_sensor( 0x3724, 0x06);  //
		//OV8858_write_cmos_sensor( 0x3725, 0x01);  //
		//OV8858_write_cmos_sensor( 0x3726, 0x06);  //
		//OV8858_write_cmos_sensor( 0x3728, 0x05);  //
		//OV8858_write_cmos_sensor( 0x3729, 0x02);  //
		//OV8858_write_cmos_sensor( 0x372a, 0x03);  //
		//OV8858_write_cmos_sensor( 0x372b, 0x53);  //
		//OV8858_write_cmos_sensor( 0x372c, 0xa3);  //
		//OV8858_write_cmos_sensor( 0x372d, 0x53);  //
		//OV8858_write_cmos_sensor( 0x372e, 0x06);  //
		//OV8858_write_cmos_sensor( 0x372f, 0x10);  //
		//OV8858_write_cmos_sensor( 0x3730, 0x01);  //
		//OV8858_write_cmos_sensor( 0x3731, 0x06);  //
		//OV8858_write_cmos_sensor( 0x3732, 0x14);  //
		//OV8858_write_cmos_sensor( 0x3736, 0x20);  //
		//OV8858_write_cmos_sensor( 0x373a, 0x05);  //
		//OV8858_write_cmos_sensor( 0x373b, 0x06);  //
		//OV8858_write_cmos_sensor( 0x373c, 0x0a);  //
		//OV8858_write_cmos_sensor( 0x373e, 0x03);  //
		//OV8858_write_cmos_sensor( 0x375a, 0x06);  //
		//OV8858_write_cmos_sensor( 0x375b, 0x13);  //
		//OV8858_write_cmos_sensor( 0x375d, 0x02);  //
		//OV8858_write_cmos_sensor( 0x375f, 0x14);  //
		//OV8858_write_cmos_sensor( 0x3772, 0x23);  //
		//OV8858_write_cmos_sensor( 0x3773, 0x02);  //
		//OV8858_write_cmos_sensor( 0x3774, 0x16);  //
		//OV8858_write_cmos_sensor( 0x3775, 0x12);  //
		//OV8858_write_cmos_sensor( 0x3776, 0x04);  //
		//OV8858_write_cmos_sensor( 0x3778, 0x1b);  //
		//OV8858_write_cmos_sensor( 0x37a0, 0x44);  //
		//OV8858_write_cmos_sensor( 0x37a1, 0x3d);  //
		//OV8858_write_cmos_sensor( 0x37a2, 0x3d);  //
		//OV8858_write_cmos_sensor( 0x37a7, 0x44);  //
		//OV8858_write_cmos_sensor( 0x37a8, 0x4c);  //
		//OV8858_write_cmos_sensor( 0x37a9, 0x4c);  //
		//OV8858_write_cmos_sensor( 0x37aa, 0x44);  //
		//OV8858_write_cmos_sensor( 0x37ab, 0x2e);  //
		//OV8858_write_cmos_sensor( 0x37ac, 0x2e);  //
		//OV8858_write_cmos_sensor( 0x37ad, 0x33);  //
		//OV8858_write_cmos_sensor( 0x37ae, 0x0d);  //
		//OV8858_write_cmos_sensor( 0x37af, 0x0d);  //
		//OV8858_write_cmos_sensor( 0x37b3, 0x42);  //
		//OV8858_write_cmos_sensor( 0x37b4, 0x42);  //
		//OV8858_write_cmos_sensor( 0x37b5, 0x33);  //
		//OV8858_write_cmos_sensor( 0x3808, 0x06);  // ; x output size H
		//OV8858_write_cmos_sensor( 0x3809, 0x60);  // ; x output size L
		//OV8858_write_cmos_sensor( 0x380a, 0x04);  // ; y output size H
		//OV8858_write_cmos_sensor( 0x380b, 0xc8);  // ; y output size L
		//OV8858_write_cmos_sensor( 0x380c, 0x07);  // ; HTS H
		//OV8858_write_cmos_sensor( 0x380d, 0x88);  // ; HTS L
		//OV8858_write_cmos_sensor( 0x380e, 0x04);  // ; VTS H
		//OV8858_write_cmos_sensor( 0x380f, 0xdc);  // ; VTS L
		//OV8858_write_cmos_sensor( 0x3814, 0x03);  // ; x odd inc
		//OV8858_write_cmos_sensor( 0x3821, 0x61);  // ; mirror on, bin on
		//OV8858_write_cmos_sensor( 0x382a, 0x03);  // ; y odd inc
		//OV8858_write_cmos_sensor( 0x3830, 0x08);  //
		//OV8858_write_cmos_sensor( 0x3836, 0x02);  //
		//OV8858_write_cmos_sensor( 0x3845, 0x02);
		//OV8858_write_cmos_sensor( 0x3f08, 0x08);  //
		//OV8858_write_cmos_sensor( 0x3f0a, 0x80);  //
		//OV8858_write_cmos_sensor( 0x4001, 0x10);  // ; total 128 black column
		//OV8858_write_cmos_sensor( 0x4022, 0x04);  // ; Anchor left end H
		//OV8858_write_cmos_sensor( 0x4023, 0xb9);  // ; Anchor left end L
		//OV8858_write_cmos_sensor( 0x4024, 0x05);  // ; Anchor right start H
		//OV8858_write_cmos_sensor( 0x4025, 0x2a);  // ; Anchor right start L
		//OV8858_write_cmos_sensor( 0x4026, 0x05);  // ; Anchor right end H
		//OV8858_write_cmos_sensor( 0x4027, 0x2b);  // ; Anchor right end L
		//OV8858_write_cmos_sensor( 0x402b, 0x04);  // ; top black line number
		//OV8858_write_cmos_sensor( 0x402e, 0x08);  // ; bottom black line start
		//OV8858_write_cmos_sensor( 0x4500, 0x38);  //
		//OV8858_write_cmos_sensor( 0x4600, 0x00);  //
		//OV8858_write_cmos_sensor( 0x4601, 0xcb);  //
		//OV8858_write_cmos_sensor( 0x382d, 0x7f);  //
		//OV8858_write_cmos_sensor( 0x0100, 0x01);  // ; 

		//@@1632x1224_30fps_4Lane_PCLK=74.4M

OV8858_write_cmos_sensor(0x0100, 0x00);
OV8858_write_cmos_sensor(0x030e, 0x02);
OV8858_write_cmos_sensor(0x0312, 0x03);
OV8858_write_cmos_sensor(0x3015, 0x00);
OV8858_write_cmos_sensor(0x3700, 0x18);
OV8858_write_cmos_sensor(0x3701, 0x0c);
OV8858_write_cmos_sensor(0x3702, 0x28);
OV8858_write_cmos_sensor(0x3703, 0x19);
OV8858_write_cmos_sensor(0x3704, 0x14);
OV8858_write_cmos_sensor(0x3707, 0x04);
OV8858_write_cmos_sensor(0x3708, 0x24);
OV8858_write_cmos_sensor(0x3709, 0x33);
OV8858_write_cmos_sensor(0x370c, 0x04);
OV8858_write_cmos_sensor(0x3718, 0x12);
OV8858_write_cmos_sensor(0x3712, 0x42);
OV8858_write_cmos_sensor(0x371e, 0x19);
OV8858_write_cmos_sensor(0x371f, 0x40);
OV8858_write_cmos_sensor(0x3720, 0x05);
OV8858_write_cmos_sensor(0x3721, 0x05);
OV8858_write_cmos_sensor(0x3724, 0x06);
OV8858_write_cmos_sensor(0x3725, 0x01);
OV8858_write_cmos_sensor(0x3726, 0x06);
OV8858_write_cmos_sensor(0x3728, 0x05);
OV8858_write_cmos_sensor(0x3729, 0x02);
OV8858_write_cmos_sensor(0x372a, 0x03);
OV8858_write_cmos_sensor(0x372b, 0x53);
OV8858_write_cmos_sensor(0x372c, 0xa3);
OV8858_write_cmos_sensor(0x372d, 0x53);
OV8858_write_cmos_sensor(0x372e, 0x06);
OV8858_write_cmos_sensor(0x372f, 0x10);
OV8858_write_cmos_sensor(0x3730, 0x01);
OV8858_write_cmos_sensor(0x3731, 0x06);
OV8858_write_cmos_sensor(0x3732, 0x14);
OV8858_write_cmos_sensor(0x3736, 0x20);
OV8858_write_cmos_sensor(0x373a, 0x05);
OV8858_write_cmos_sensor(0x373b, 0x06);
OV8858_write_cmos_sensor(0x373c, 0x0a);
OV8858_write_cmos_sensor(0x373e, 0x03);
OV8858_write_cmos_sensor(0x375a, 0x06);
OV8858_write_cmos_sensor(0x375b, 0x13);
OV8858_write_cmos_sensor(0x375d, 0x02);
OV8858_write_cmos_sensor(0x375f, 0x14);
OV8858_write_cmos_sensor(0x3772, 0x23);
OV8858_write_cmos_sensor(0x3773, 0x02);
OV8858_write_cmos_sensor(0x3774, 0x16);
OV8858_write_cmos_sensor(0x3775, 0x12);
OV8858_write_cmos_sensor(0x3776, 0x04);
OV8858_write_cmos_sensor(0x3778, 0x17);
OV8858_write_cmos_sensor(0x37a0, 0x44);
OV8858_write_cmos_sensor(0x37a1, 0x3d);
OV8858_write_cmos_sensor(0x37a2, 0x3d);
OV8858_write_cmos_sensor(0x37a7, 0x44);
OV8858_write_cmos_sensor(0x37a8, 0x4c);
OV8858_write_cmos_sensor(0x37a9, 0x4c);
OV8858_write_cmos_sensor(0x37aa, 0x44);
OV8858_write_cmos_sensor(0x37ab, 0x2e);
OV8858_write_cmos_sensor(0x37ac, 0x2e);
OV8858_write_cmos_sensor(0x37ad, 0x33);
OV8858_write_cmos_sensor(0x37ae, 0x0d);
OV8858_write_cmos_sensor(0x37af, 0x0d);
OV8858_write_cmos_sensor(0x37b3, 0x42);
OV8858_write_cmos_sensor(0x37b4, 0x42);
OV8858_write_cmos_sensor(0x37b5, 0x31);
OV8858_write_cmos_sensor(0x3768, 0x22);
OV8858_write_cmos_sensor(0x3769, 0x44);
OV8858_write_cmos_sensor(0x376a, 0x44);
OV8858_write_cmos_sensor(0x3808, 0x06);
OV8858_write_cmos_sensor(0x3809, 0x60);
OV8858_write_cmos_sensor(0x380a, 0x04);
OV8858_write_cmos_sensor(0x380b, 0xc8);
OV8858_write_cmos_sensor(0x380c, 0x07);
OV8858_write_cmos_sensor(0x380d, 0x88);
OV8858_write_cmos_sensor(0x380e, 0x05);
OV8858_write_cmos_sensor(0x380f, 0x04);
OV8858_write_cmos_sensor(0x3814, 0x03);
OV8858_write_cmos_sensor(0x3821, 0x67);
OV8858_write_cmos_sensor(0x382a, 0x03);
OV8858_write_cmos_sensor(0x382b, 0x01);
OV8858_write_cmos_sensor(0x3830, 0x08);
OV8858_write_cmos_sensor(0x3836, 0x02);
OV8858_write_cmos_sensor(0x3f08, 0x08);
OV8858_write_cmos_sensor(0x4001, 0x10);
OV8858_write_cmos_sensor(0x4020, 0x00);
OV8858_write_cmos_sensor(0x4021, 0x04);
OV8858_write_cmos_sensor(0x4022, 0x04);
OV8858_write_cmos_sensor(0x4023, 0xb9);
OV8858_write_cmos_sensor(0x4024, 0x05);
OV8858_write_cmos_sensor(0x4025, 0x2a);
OV8858_write_cmos_sensor(0x4026, 0x05);
OV8858_write_cmos_sensor(0x4027, 0x2b);
OV8858_write_cmos_sensor(0x402a, 0x04);
OV8858_write_cmos_sensor(0x402b, 0x04);
OV8858_write_cmos_sensor(0x402e, 0x08);
OV8858_write_cmos_sensor(0x402f, 0x02);
OV8858_write_cmos_sensor(0x4600, 0x00);
OV8858_write_cmos_sensor(0x4601, 0xcb);
OV8858_write_cmos_sensor(0x5901, 0x00);
OV8858_write_cmos_sensor(0x382d, 0x7f);
OV8858_write_cmos_sensor(0x0100, 0x01);

}

void OV8858CaptureSetting(void)
{
	// +++++++	@@5.1.1.3 Raw 10bit 3264*2448 30fps 4lane 672M bps/lane
	// +++++++	;Pclk 144MHz
	// +++++++	;pixels per line=1940(0x794) 
	// +++++++	;lines per frame=2474(0x9aa)
	
		//OV8858_write_cmos_sensor( 0x0100, 0x00);  //
		//OV8858_write_cmos_sensor( 0x030f, 0x04);  // ; pll2_divsp
		//OV8858_write_cmos_sensor( 0x3501, 0x9a);  // ; exposure M
		//OV8858_write_cmos_sensor( 0x3502, 0x20);  // ; exposure L
		//OV8858_write_cmos_sensor( 0x3508, 0x02);  // ; gain H
		//OV8858_write_cmos_sensor( 0x3700, 0x30);  //
		//OV8858_write_cmos_sensor( 0x3701, 0x18);  //
		//OV8858_write_cmos_sensor( 0x3702, 0x50);  //
		//OV8858_write_cmos_sensor( 0x3703, 0x32);  //
		//OV8858_write_cmos_sensor( 0x3704, 0x28);  //
		//OV8858_write_cmos_sensor( 0x3706, 0x6a);  //
		//OV8858_write_cmos_sensor( 0x3707, 0x08);  //
		//OV8858_write_cmos_sensor( 0x3708, 0x48);  //
		//OV8858_write_cmos_sensor( 0x3709, 0x66);  //
		//OV8858_write_cmos_sensor( 0x370a, 0x01);  //
		//OV8858_write_cmos_sensor( 0x370b, 0x6a);  //
		//OV8858_write_cmos_sensor( 0x370c, 0x07);  //
		//OV8858_write_cmos_sensor( 0x3718, 0x14);  //
		//OV8858_write_cmos_sensor( 0x3712, 0x44);  //
		//OV8858_write_cmos_sensor( 0x371e, 0x31);  //
		//OV8858_write_cmos_sensor( 0x371f, 0x7f);  //
		//OV8858_write_cmos_sensor( 0x3720, 0x0a);  //
		//OV8858_write_cmos_sensor( 0x3721, 0x0a);  //
		//OV8858_write_cmos_sensor( 0x3724, 0x0c);  //
		//OV8858_write_cmos_sensor( 0x3725, 0x02);  //
		//OV8858_write_cmos_sensor( 0x3726, 0x0c);  //
		//OV8858_write_cmos_sensor( 0x3728, 0x0a);  //
		//OV8858_write_cmos_sensor( 0x3729, 0x03);  //
		//OV8858_write_cmos_sensor( 0x372a, 0x06);  //
		//OV8858_write_cmos_sensor( 0x372b, 0xa6);  //
		//OV8858_write_cmos_sensor( 0x372c, 0xa6);  //
		//OV8858_write_cmos_sensor( 0x372d, 0xa6);  //
		//OV8858_write_cmos_sensor( 0x372e, 0x0c);  //
		//OV8858_write_cmos_sensor( 0x372f, 0x20);  //
		//OV8858_write_cmos_sensor( 0x3730, 0x02);  //
		//OV8858_write_cmos_sensor( 0x3731, 0x0c);  //
		//OV8858_write_cmos_sensor( 0x3732, 0x28);  //
		//OV8858_write_cmos_sensor( 0x3736, 0x30);  //
		//OV8858_write_cmos_sensor( 0x373a, 0x0a);  //
		//OV8858_write_cmos_sensor( 0x373b, 0x0b);  //
		//OV8858_write_cmos_sensor( 0x373c, 0x14);  //
		//OV8858_write_cmos_sensor( 0x373e, 0x06);  //
		//OV8858_write_cmos_sensor( 0x375a, 0x0c);  //
		//OV8858_write_cmos_sensor( 0x375b, 0x26);  //
		//OV8858_write_cmos_sensor( 0x375d, 0x04);  //
		//OV8858_write_cmos_sensor( 0x375f, 0x28);  //
		//OV8858_write_cmos_sensor( 0x3772, 0x46);  //
		//OV8858_write_cmos_sensor( 0x3773, 0x04);  //
		//OV8858_write_cmos_sensor( 0x3774, 0x2c);  //
		//OV8858_write_cmos_sensor( 0x3775, 0x13);  //
		//OV8858_write_cmos_sensor( 0x3776, 0x08);  //
		//OV8858_write_cmos_sensor( 0x3778, 0x16);  //
		//OV8858_write_cmos_sensor( 0x37a0, 0x88);  //
		//OV8858_write_cmos_sensor( 0x37a1, 0x7a);  //
		//OV8858_write_cmos_sensor( 0x37a2, 0x7a);  //
		//OV8858_write_cmos_sensor( 0x37a7, 0x88);  //
		//OV8858_write_cmos_sensor( 0x37a8, 0x98);  //
		//OV8858_write_cmos_sensor( 0x37a9, 0x98);  //
		//OV8858_write_cmos_sensor( 0x37aa, 0x88);  //
		//OV8858_write_cmos_sensor( 0x37ab, 0x5c);  //
		//OV8858_write_cmos_sensor( 0x37ac, 0x5c);  //
		//OV8858_write_cmos_sensor( 0x37ad, 0x55);  //
		//OV8858_write_cmos_sensor( 0x37ae, 0x19);  //
		//OV8858_write_cmos_sensor( 0x37af, 0x19);  //
		//OV8858_write_cmos_sensor( 0x37b3, 0x84);  //
		//OV8858_write_cmos_sensor( 0x37b4, 0x84);  //
		//OV8858_write_cmos_sensor( 0x37b5, 0x66);  //
		//OV8858_write_cmos_sensor( 0x3808, 0x0c);  // ; x output size H
		//OV8858_write_cmos_sensor( 0x3809, 0xc0);  // ; x output size L
		//OV8858_write_cmos_sensor( 0x380a, 0x09);  // ; y output size H
		//OV8858_write_cmos_sensor( 0x380b, 0x90);  // ; y output size L
		//OV8858_write_cmos_sensor( 0x380c, 0x07);  // ; HTS H
		//OV8858_write_cmos_sensor( 0x380d, 0x94);  // ; HTS L
		//OV8858_write_cmos_sensor( 0x380e, 0x09);  // ; VTS H
		//OV8858_write_cmos_sensor( 0x380f, 0xaa);  // ; VTS L
		//OV8858_write_cmos_sensor( 0x3814, 0x01);  // ; x odd inc
		//OV8858_write_cmos_sensor( 0x3821, 0x40);  // ; mirror on, bin off
		//OV8858_write_cmos_sensor( 0x382a, 0x01);  // ; y odd inc
		//OV8858_write_cmos_sensor( 0x3830, 0x06);  //
		//OV8858_write_cmos_sensor( 0x3836, 0x01);  //
		//OV8858_write_cmos_sensor( 0x3845, 0x00);
		//OV8858_write_cmos_sensor( 0x3f08, 0x08);  //
		//OV8858_write_cmos_sensor( 0x3f0a, 0x00);  //
		//OV8858_write_cmos_sensor( 0x4001, 0x00);  // ; total 256 black column
		//OV8858_write_cmos_sensor( 0x4022, 0x0b);  // ; Anchor left end H
		//OV8858_write_cmos_sensor( 0x4023, 0xc3);  // ; Anchor left end L
		//OV8858_write_cmos_sensor( 0x4024, 0x0c);  // ; Anchor right start H
		//OV8858_write_cmos_sensor( 0x4025, 0x36);  // ; Anchor right start L
		//OV8858_write_cmos_sensor( 0x4026, 0x0c);  // ; Anchor right end H
		//OV8858_write_cmos_sensor( 0x4027, 0x37);  // ; Anchor right end L
		//OV8858_write_cmos_sensor( 0x402b, 0x08);  // ; top black line number
		//OV8858_write_cmos_sensor( 0x402e, 0x0c);  // ; bottom black line start
		//OV8858_write_cmos_sensor( 0x4500, 0x58);  //
		//OV8858_write_cmos_sensor( 0x4600, 0x01);  //
		//OV8858_write_cmos_sensor( 0x4601, 0x97);  //
		//OV8858_write_cmos_sensor( 0x382d, 0xff);  //
		//OV8858_write_cmos_sensor( 0x0100, 0x01);  //
OV8858_write_cmos_sensor(0x0100, 0x00);
OV8858_write_cmos_sensor(0x030e, 0x00);
OV8858_write_cmos_sensor(0x0312, 0x01);
OV8858_write_cmos_sensor(0x3015, 0x01);
OV8858_write_cmos_sensor(0x3700, 0x30);
OV8858_write_cmos_sensor(0x3701, 0x18);
OV8858_write_cmos_sensor(0x3702, 0x50);
OV8858_write_cmos_sensor(0x3703, 0x32);
OV8858_write_cmos_sensor(0x3704, 0x28);
OV8858_write_cmos_sensor(0x3707, 0x08);
OV8858_write_cmos_sensor(0x3708, 0x48);
OV8858_write_cmos_sensor(0x3709, 0x66);
OV8858_write_cmos_sensor(0x370c, 0x07);
OV8858_write_cmos_sensor(0x3718, 0x14);
OV8858_write_cmos_sensor(0x3712, 0x44);
OV8858_write_cmos_sensor(0x371e, 0x31);
OV8858_write_cmos_sensor(0x371f, 0x7f);
OV8858_write_cmos_sensor(0x3720, 0x0a);
OV8858_write_cmos_sensor(0x3721, 0x0a);
OV8858_write_cmos_sensor(0x3724, 0x0c);
OV8858_write_cmos_sensor(0x3725, 0x02);
OV8858_write_cmos_sensor(0x3726, 0x0c);
OV8858_write_cmos_sensor(0x3728, 0x0a);
OV8858_write_cmos_sensor(0x3729, 0x03);
OV8858_write_cmos_sensor(0x372a, 0x06);
OV8858_write_cmos_sensor(0x372b, 0xa6);
OV8858_write_cmos_sensor(0x372c, 0xa6);
OV8858_write_cmos_sensor(0x372d, 0xa6);
OV8858_write_cmos_sensor(0x372e, 0x0c);
OV8858_write_cmos_sensor(0x372f, 0x20);
OV8858_write_cmos_sensor(0x3730, 0x02);
OV8858_write_cmos_sensor(0x3731, 0x0c);
OV8858_write_cmos_sensor(0x3732, 0x28);
OV8858_write_cmos_sensor(0x3736, 0x30);
OV8858_write_cmos_sensor(0x373a, 0x0a);
OV8858_write_cmos_sensor(0x373b, 0x0b);
OV8858_write_cmos_sensor(0x373c, 0x14);
OV8858_write_cmos_sensor(0x373e, 0x06);
OV8858_write_cmos_sensor(0x375a, 0x0c);
OV8858_write_cmos_sensor(0x375b, 0x26);
OV8858_write_cmos_sensor(0x375d, 0x04);
OV8858_write_cmos_sensor(0x375f, 0x28);
OV8858_write_cmos_sensor(0x3772, 0x46);
OV8858_write_cmos_sensor(0x3773, 0x04);
OV8858_write_cmos_sensor(0x3774, 0x2c);
OV8858_write_cmos_sensor(0x3775, 0x13);
OV8858_write_cmos_sensor(0x3776, 0x08);
OV8858_write_cmos_sensor(0x3778, 0x17);
OV8858_write_cmos_sensor(0x37a0, 0x88);
OV8858_write_cmos_sensor(0x37a1, 0x7a);
OV8858_write_cmos_sensor(0x37a2, 0x7a);
OV8858_write_cmos_sensor(0x37a7, 0x88);
OV8858_write_cmos_sensor(0x37a8, 0x98);
OV8858_write_cmos_sensor(0x37a9, 0x98);
OV8858_write_cmos_sensor(0x37aa, 0x88);
OV8858_write_cmos_sensor(0x37ab, 0x5c);
OV8858_write_cmos_sensor(0x37ac, 0x5c);
OV8858_write_cmos_sensor(0x37ad, 0x55);
OV8858_write_cmos_sensor(0x37ae, 0x19);
OV8858_write_cmos_sensor(0x37af, 0x19);
OV8858_write_cmos_sensor(0x37b3, 0x84);
OV8858_write_cmos_sensor(0x37b4, 0x84);
OV8858_write_cmos_sensor(0x37b5, 0x60);
OV8858_write_cmos_sensor(0x3768, 0x22);
OV8858_write_cmos_sensor(0x3769, 0x44);
OV8858_write_cmos_sensor(0x376a, 0x44);
OV8858_write_cmos_sensor(0x3808, 0x0c);
OV8858_write_cmos_sensor(0x3809, 0xc0);
OV8858_write_cmos_sensor(0x380a, 0x09);
OV8858_write_cmos_sensor(0x380b, 0x90);
OV8858_write_cmos_sensor(0x380c, 0x07);
OV8858_write_cmos_sensor(0x380d, 0x94);
OV8858_write_cmos_sensor(0x380e, 0x09);
OV8858_write_cmos_sensor(0x380f, 0xfc);
OV8858_write_cmos_sensor(0x3814, 0x01);
OV8858_write_cmos_sensor(0x3821, 0x46);
OV8858_write_cmos_sensor(0x382a, 0x01);
OV8858_write_cmos_sensor(0x382b, 0x01);
OV8858_write_cmos_sensor(0x3830, 0x06);
OV8858_write_cmos_sensor(0x3836, 0x01);
OV8858_write_cmos_sensor(0x3f08, 0x10);
OV8858_write_cmos_sensor(0x4001, 0x00);
OV8858_write_cmos_sensor(0x4020, 0x00);
OV8858_write_cmos_sensor(0x4021, 0x04);
OV8858_write_cmos_sensor(0x4022, 0x0b);
OV8858_write_cmos_sensor(0x4023, 0xc3);
OV8858_write_cmos_sensor(0x4024, 0x0c);
OV8858_write_cmos_sensor(0x4025, 0x36);
OV8858_write_cmos_sensor(0x4026, 0x0c);
OV8858_write_cmos_sensor(0x4027, 0x37);
OV8858_write_cmos_sensor(0x402a, 0x04);
OV8858_write_cmos_sensor(0x402b, 0x08);
OV8858_write_cmos_sensor(0x402e, 0x0c);
OV8858_write_cmos_sensor(0x402f, 0x02);
OV8858_write_cmos_sensor(0x4600, 0x01);
OV8858_write_cmos_sensor(0x4601, 0x97);
OV8858_write_cmos_sensor(0x5901, 0x00);
OV8858_write_cmos_sensor(0x382d, 0xff);
OV8858_write_cmos_sensor(0x0100, 0x01);
}

void OV8858VideoSetting(void)
{
	//	++++++++   @@5.1.1.4 Raw 10bit 3264*1836 30fps 4lane 672M bps/lane
	//	++++++++   ;Pclk 144MHz
	//	++++++++   ;pixels per line=2566(0xa06) 
	//	++++++++   ;lines per frame=1872(0x750)
		//OV8858_write_cmos_sensor( 0x0100, 0x00);  //  
		//OV8858_write_cmos_sensor( 0x030f, 0x04);  //   ; pll2_divsp
		//OV8858_write_cmos_sensor( 0x3501, 0x74);  //   ; exposure M
		//OV8858_write_cmos_sensor( 0x3502, 0x80);  //   ; exposure L
		//OV8858_write_cmos_sensor( 0x3508, 0x02);  //   ; gain H
		//OV8858_write_cmos_sensor( 0x3700, 0x30);  //  
		//OV8858_write_cmos_sensor( 0x3701, 0x18);  //  
		//OV8858_write_cmos_sensor( 0x3702, 0x50);  //  
		//OV8858_write_cmos_sensor( 0x3703, 0x32);  //  
		//OV8858_write_cmos_sensor( 0x3704, 0x28);  //  
		//OV8858_write_cmos_sensor( 0x3706, 0x6a);  //  
		//OV8858_write_cmos_sensor( 0x3707, 0x08);  //  
		//OV8858_write_cmos_sensor( 0x3708, 0x48);  //  
		//OV8858_write_cmos_sensor( 0x3709, 0x66);  //  
		//OV8858_write_cmos_sensor( 0x370a, 0x01);  //  
		//OV8858_write_cmos_sensor( 0x370b, 0x6a);  //  
		//OV8858_write_cmos_sensor( 0x370c, 0x07);  //  
		//OV8858_write_cmos_sensor( 0x3718, 0x14);  //  
		//OV8858_write_cmos_sensor( 0x3712, 0x44);  //  
		//OV8858_write_cmos_sensor( 0x371e, 0x31);  //  
		//OV8858_write_cmos_sensor( 0x371f, 0x7f);  //  
		//OV8858_write_cmos_sensor( 0x3720, 0x0a);  //  
		//OV8858_write_cmos_sensor( 0x3721, 0x0a);  //  
		//OV8858_write_cmos_sensor( 0x3724, 0x0c);  //  
		//OV8858_write_cmos_sensor( 0x3725, 0x02);  //  
		//OV8858_write_cmos_sensor( 0x3726, 0x0c);  //  
		//OV8858_write_cmos_sensor( 0x3728, 0x0a);  //  
		//OV8858_write_cmos_sensor( 0x3729, 0x03);  //  
		//OV8858_write_cmos_sensor( 0x372a, 0x06);  //  
		//OV8858_write_cmos_sensor( 0x372b, 0xa6);  //  
		//OV8858_write_cmos_sensor( 0x372c, 0xa6);  //  
		//OV8858_write_cmos_sensor( 0x372d, 0xa6);  //  
		//OV8858_write_cmos_sensor( 0x372e, 0x0c);  //  
		//OV8858_write_cmos_sensor( 0x372f, 0x20);  //  
		//OV8858_write_cmos_sensor( 0x3730, 0x02);  //  
		//OV8858_write_cmos_sensor( 0x3731, 0x0c);  //  
		//OV8858_write_cmos_sensor( 0x3732, 0x28);  //  
		//OV8858_write_cmos_sensor( 0x3736, 0x30);  //  
		//OV8858_write_cmos_sensor( 0x373a, 0x0a);  //  
		//OV8858_write_cmos_sensor( 0x373b, 0x0b);  //  
		//OV8858_write_cmos_sensor( 0x373c, 0x14);  //  
		//OV8858_write_cmos_sensor( 0x373e, 0x06);  //  
		//OV8858_write_cmos_sensor( 0x375a, 0x0c);  //  
		//OV8858_write_cmos_sensor( 0x375b, 0x26);  //  
		//OV8858_write_cmos_sensor( 0x375d, 0x04);  //  
		//OV8858_write_cmos_sensor( 0x375f, 0x28);  //  
		//OV8858_write_cmos_sensor( 0x3772, 0x46);  //  
		//OV8858_write_cmos_sensor( 0x3773, 0x04);  //  
		//OV8858_write_cmos_sensor( 0x3774, 0x2c);  //  
		//OV8858_write_cmos_sensor( 0x3775, 0x13);  //  
		//OV8858_write_cmos_sensor( 0x3776, 0x08);  //  
		//OV8858_write_cmos_sensor( 0x3778, 0x16);  //  
		//OV8858_write_cmos_sensor( 0x37a0, 0x88);  //  
		//OV8858_write_cmos_sensor( 0x37a1, 0x7a);  //  
		//OV8858_write_cmos_sensor( 0x37a2, 0x7a);  //  
		//OV8858_write_cmos_sensor( 0x37a7, 0x88);  //  
		//OV8858_write_cmos_sensor( 0x37a8, 0x98);  //  
		//OV8858_write_cmos_sensor( 0x37a9, 0x98);  //  
		//OV8858_write_cmos_sensor( 0x37aa, 0x88);  //  
		//OV8858_write_cmos_sensor( 0x37ab, 0x5c);  //  
		//OV8858_write_cmos_sensor( 0x37ac, 0x5c);  //  
		//OV8858_write_cmos_sensor( 0x37ad, 0x55);  //  
		//OV8858_write_cmos_sensor( 0x37ae, 0x19);  //  
		//OV8858_write_cmos_sensor( 0x37af, 0x19);  //  
		//OV8858_write_cmos_sensor( 0x37b3, 0x84);  //  
		//OV8858_write_cmos_sensor( 0x37b4, 0x84);  //  
		//OV8858_write_cmos_sensor( 0x37b5, 0x66);  //  
		//OV8858_write_cmos_sensor( 0x3808, 0x0c);  //   ; x output size H
		//OV8858_write_cmos_sensor( 0x3809, 0xc0);  //   ; x output size L
		//OV8858_write_cmos_sensor( 0x380a, 0x07);  //   ; y output size H
		//OV8858_write_cmos_sensor( 0x380b, 0x2c);  //   ; y output size L
		//OV8858_write_cmos_sensor( 0x380c, 0x0a);  //   ; HTS H
		//OV8858_write_cmos_sensor( 0x380d, 0x06);  //   ; HTS L
		//OV8858_write_cmos_sensor( 0x380e, 0x07);  //   ; VTS H
		//OV8858_write_cmos_sensor( 0x380f, 0x50);  //   ; VTS L
		//OV8858_write_cmos_sensor( 0x3814, 0x01);  //   ; x odd inc
		//OV8858_write_cmos_sensor( 0x3821, 0x40);  //   ; mirror on, bin off
		//OV8858_write_cmos_sensor( 0x382a, 0x01);  //   ; y odd inc
		//OV8858_write_cmos_sensor( 0x3830, 0x06);  //  
		//OV8858_write_cmos_sensor( 0x3836, 0x01);  //  
		//OV8858_write_cmos_sensor( 0x3845, 0x00);
		//OV8858_write_cmos_sensor( 0x3f08, 0x08);  //  
		//OV8858_write_cmos_sensor( 0x3f0a, 0x00);  //  
		//OV8858_write_cmos_sensor( 0x4001, 0x00);  //   ; total 256 black column
		//OV8858_write_cmos_sensor( 0x4022, 0x0b);  //   ; Anchor left end H
		//OV8858_write_cmos_sensor( 0x4023, 0xc3);  //   ; Anchor left end L
		//OV8858_write_cmos_sensor( 0x4024, 0x0c);  //   ; Anchor right start H
		//OV8858_write_cmos_sensor( 0x4025, 0x36);  //   ; Anchor right start L
		//OV8858_write_cmos_sensor( 0x4026, 0x0c);  //   ; Anchor right end H
		//OV8858_write_cmos_sensor( 0x4027, 0x37);  //   ; Anchor right end L
		//OV8858_write_cmos_sensor( 0x402b, 0x08);  //   ; top black line number
		//OV8858_write_cmos_sensor( 0x402e, 0x0c);  //   ; bottom black line start
		//OV8858_write_cmos_sensor( 0x4500, 0x58);  //  
		//OV8858_write_cmos_sensor( 0x4600, 0x01);  //  
		//OV8858_write_cmos_sensor( 0x4601, 0x97);  //  
		//OV8858_write_cmos_sensor( 0x382d, 0xff);  //  
		//OV8858_write_cmos_sensor( 0x0100, 0x01);  //	
OV8858_write_cmos_sensor(0x0100, 0x00);
OV8858_write_cmos_sensor(0x030e, 0x00);
OV8858_write_cmos_sensor(0x0312, 0x01);
OV8858_write_cmos_sensor(0x3015, 0x01);
OV8858_write_cmos_sensor(0x3700, 0x30);
OV8858_write_cmos_sensor(0x3701, 0x18);
OV8858_write_cmos_sensor(0x3702, 0x50);
OV8858_write_cmos_sensor(0x3703, 0x32);
OV8858_write_cmos_sensor(0x3704, 0x28);
OV8858_write_cmos_sensor(0x3707, 0x08);
OV8858_write_cmos_sensor(0x3708, 0x48);
OV8858_write_cmos_sensor(0x3709, 0x66);
OV8858_write_cmos_sensor(0x370c, 0x07);
OV8858_write_cmos_sensor(0x3718, 0x14);
OV8858_write_cmos_sensor(0x3712, 0x44);
OV8858_write_cmos_sensor(0x371e, 0x31);
OV8858_write_cmos_sensor(0x371f, 0x7f);
OV8858_write_cmos_sensor(0x3720, 0x0a);
OV8858_write_cmos_sensor(0x3721, 0x0a);
OV8858_write_cmos_sensor(0x3724, 0x0c);
OV8858_write_cmos_sensor(0x3725, 0x02);
OV8858_write_cmos_sensor(0x3726, 0x0c);
OV8858_write_cmos_sensor(0x3728, 0x0a);
OV8858_write_cmos_sensor(0x3729, 0x03);
OV8858_write_cmos_sensor(0x372a, 0x06);
OV8858_write_cmos_sensor(0x372b, 0xa6);
OV8858_write_cmos_sensor(0x372c, 0xa6);
OV8858_write_cmos_sensor(0x372d, 0xa6);
OV8858_write_cmos_sensor(0x372e, 0x0c);
OV8858_write_cmos_sensor(0x372f, 0x20);
OV8858_write_cmos_sensor(0x3730, 0x02);
OV8858_write_cmos_sensor(0x3731, 0x0c);
OV8858_write_cmos_sensor(0x3732, 0x28);
OV8858_write_cmos_sensor(0x3736, 0x30);
OV8858_write_cmos_sensor(0x373a, 0x0a);
OV8858_write_cmos_sensor(0x373b, 0x0b);
OV8858_write_cmos_sensor(0x373c, 0x14);
OV8858_write_cmos_sensor(0x373e, 0x06);
OV8858_write_cmos_sensor(0x375a, 0x0c);
OV8858_write_cmos_sensor(0x375b, 0x26);
OV8858_write_cmos_sensor(0x375d, 0x04);
OV8858_write_cmos_sensor(0x375f, 0x28);
OV8858_write_cmos_sensor(0x3772, 0x46);
OV8858_write_cmos_sensor(0x3773, 0x04);
OV8858_write_cmos_sensor(0x3774, 0x2c);
OV8858_write_cmos_sensor(0x3775, 0x13);
OV8858_write_cmos_sensor(0x3776, 0x08);
OV8858_write_cmos_sensor(0x3778, 0x17);
OV8858_write_cmos_sensor(0x37a0, 0x88);
OV8858_write_cmos_sensor(0x37a1, 0x7a);
OV8858_write_cmos_sensor(0x37a2, 0x7a);
OV8858_write_cmos_sensor(0x37a7, 0x88);
OV8858_write_cmos_sensor(0x37a8, 0x98);
OV8858_write_cmos_sensor(0x37a9, 0x98);
OV8858_write_cmos_sensor(0x37aa, 0x88);
OV8858_write_cmos_sensor(0x37ab, 0x5c);
OV8858_write_cmos_sensor(0x37ac, 0x5c);
OV8858_write_cmos_sensor(0x37ad, 0x55);
OV8858_write_cmos_sensor(0x37ae, 0x19);
OV8858_write_cmos_sensor(0x37af, 0x19);
OV8858_write_cmos_sensor(0x37b3, 0x84);
OV8858_write_cmos_sensor(0x37b4, 0x84);
OV8858_write_cmos_sensor(0x37b5, 0x60);
OV8858_write_cmos_sensor(0x3768, 0x22);
OV8858_write_cmos_sensor(0x3769, 0x44);
OV8858_write_cmos_sensor(0x376a, 0x44);
OV8858_write_cmos_sensor(0x3808, 0x0c);
OV8858_write_cmos_sensor(0x3809, 0xc0);
OV8858_write_cmos_sensor(0x380a, 0x07);
OV8858_write_cmos_sensor(0x380b, 0x2c);
OV8858_write_cmos_sensor(0x380c, 0x0a);
OV8858_write_cmos_sensor(0x380d, 0x06);
OV8858_write_cmos_sensor(0x380e, 0x07);
OV8858_write_cmos_sensor(0x380f, 0x8c);
OV8858_write_cmos_sensor(0x3814, 0x01);
OV8858_write_cmos_sensor(0x3821, 0x46);
OV8858_write_cmos_sensor(0x382a, 0x01);
OV8858_write_cmos_sensor(0x382b, 0x01);
OV8858_write_cmos_sensor(0x3830, 0x06);
OV8858_write_cmos_sensor(0x3836, 0x01);
OV8858_write_cmos_sensor(0x3f08, 0x10);
OV8858_write_cmos_sensor(0x4001, 0x00);
OV8858_write_cmos_sensor(0x4020, 0x00);
OV8858_write_cmos_sensor(0x4021, 0x04);
OV8858_write_cmos_sensor(0x4022, 0x0b);
OV8858_write_cmos_sensor(0x4023, 0xc3);
OV8858_write_cmos_sensor(0x4024, 0x0c);
OV8858_write_cmos_sensor(0x4025, 0x36);
OV8858_write_cmos_sensor(0x4026, 0x0c);
OV8858_write_cmos_sensor(0x4027, 0x37);
OV8858_write_cmos_sensor(0x402a, 0x04);
OV8858_write_cmos_sensor(0x402b, 0x08);
OV8858_write_cmos_sensor(0x402e, 0x0c);
OV8858_write_cmos_sensor(0x402f, 0x02);
OV8858_write_cmos_sensor(0x4600, 0x01);
OV8858_write_cmos_sensor(0x4601, 0x97);
OV8858_write_cmos_sensor(0x5901, 0x00);
OV8858_write_cmos_sensor(0x382d, 0xff);
OV8858_write_cmos_sensor(0x0100, 0x01);

}

static void OV8858_Sensor_Init(void)
{
	// ++++++++ @@5.1.1.1 Initialization (Global Setting)
	// ++++++++ ;;
	// ++++++++ ;; MIPI=672Mbps, SysClk=72Mhz,Dac Clock=360Mhz.
	// ++++++++ ;;
	// ++++++++ ;;
	// ++++++++ ;; v00_01_00 (12/19/2013) : initial setting
	// ++++++++ ;;
	//OV8858_write_cmos_sensor( 0x0103, 0x01);
	//OV8858_write_cmos_sensor( 0x0100, 0x00);
	//OV8858_write_cmos_sensor( 0x0100, 0x00);
	//OV8858_write_cmos_sensor( 0x0100, 0x00);
	//OV8858_write_cmos_sensor( 0x0100, 0x00);
	//OV8858_write_cmos_sensor( 0x0302, 0x1c);
	//OV8858_write_cmos_sensor( 0x0303, 0x00);
	//OV8858_write_cmos_sensor( 0x0304, 0x03);
	//OV8858_write_cmos_sensor( 0x030e, 0x00);
	//OV8858_write_cmos_sensor( 0x030f, 0x09);
	//OV8858_write_cmos_sensor( 0x0312, 0x01);
	//OV8858_write_cmos_sensor( 0x031e, 0x0c);
	//OV8858_write_cmos_sensor( 0x3600, 0x00);
	//OV8858_write_cmos_sensor( 0x3601, 0x00);
	//OV8858_write_cmos_sensor( 0x3602, 0x00);
	//OV8858_write_cmos_sensor( 0x3603, 0x00);
	//OV8858_write_cmos_sensor( 0x3604, 0x22);
	//OV8858_write_cmos_sensor( 0x3605, 0x30);
	//OV8858_write_cmos_sensor( 0x3606, 0x00);
	//OV8858_write_cmos_sensor( 0x3607, 0x20);
	//OV8858_write_cmos_sensor( 0x3608, 0x11);
	//OV8858_write_cmos_sensor( 0x3609, 0x28);
	//OV8858_write_cmos_sensor( 0x360a, 0x00);
	//OV8858_write_cmos_sensor( 0x360b, 0x06);
	//OV8858_write_cmos_sensor( 0x360c, 0xdc);
	//OV8858_write_cmos_sensor( 0x360d, 0x40);
	//OV8858_write_cmos_sensor( 0x360e, 0x0c);
	//OV8858_write_cmos_sensor( 0x360f, 0x20);
	//OV8858_write_cmos_sensor( 0x3610, 0x07);
	//OV8858_write_cmos_sensor( 0x3611, 0x20);
	//OV8858_write_cmos_sensor( 0x3612, 0x88);
	//OV8858_write_cmos_sensor( 0x3613, 0x80);
	//OV8858_write_cmos_sensor( 0x3614, 0x58);
	//OV8858_write_cmos_sensor( 0x3615, 0x00);
	//OV8858_write_cmos_sensor( 0x3616, 0x4a);
	//OV8858_write_cmos_sensor( 0x3617, 0x90);
	//OV8858_write_cmos_sensor( 0x3618, 0x56);
	//OV8858_write_cmos_sensor( 0x3619, 0x70);
	//OV8858_write_cmos_sensor( 0x361a, 0x99);
	//OV8858_write_cmos_sensor( 0x361b, 0x00);
	//OV8858_write_cmos_sensor( 0x361c, 0x07);
	//OV8858_write_cmos_sensor( 0x361d, 0x00);
	//OV8858_write_cmos_sensor( 0x361e, 0x00);
	//OV8858_write_cmos_sensor( 0x361f, 0x00);
	//OV8858_write_cmos_sensor( 0x3638, 0xff);
	//OV8858_write_cmos_sensor( 0x3633, 0x0c);
	//OV8858_write_cmos_sensor( 0x3634, 0x0c);
	//OV8858_write_cmos_sensor( 0x3635, 0x0c);
	//OV8858_write_cmos_sensor( 0x3636, 0x0c);
	//OV8858_write_cmos_sensor( 0x3645, 0x13);
	//OV8858_write_cmos_sensor( 0x3646, 0x83);
	//OV8858_write_cmos_sensor( 0x364a, 0x07);
	//OV8858_write_cmos_sensor( 0x3015, 0x01);
	//OV8858_write_cmos_sensor( 0x3018, 0x72);
	//OV8858_write_cmos_sensor( 0x3020, 0x93);
	//OV8858_write_cmos_sensor( 0x3022, 0x01);
	//OV8858_write_cmos_sensor( 0x3031, 0x0a);
	//OV8858_write_cmos_sensor( 0x3034, 0x00);
	//OV8858_write_cmos_sensor( 0x3106, 0x01);
	//OV8858_write_cmos_sensor( 0x3305, 0xf1);
	//OV8858_write_cmos_sensor( 0x3308, 0x00);
	//OV8858_write_cmos_sensor( 0x3309, 0x28);
	//OV8858_write_cmos_sensor( 0x330a, 0x00);
	//OV8858_write_cmos_sensor( 0x330b, 0x20);
	//OV8858_write_cmos_sensor( 0x330c, 0x00);
	//OV8858_write_cmos_sensor( 0x330d, 0x00);
	//OV8858_write_cmos_sensor( 0x330e, 0x00);
	//OV8858_write_cmos_sensor( 0x330f, 0x40);
	//OV8858_write_cmos_sensor( 0x3307, 0x04);
	//OV8858_write_cmos_sensor( 0x3500, 0x00);
	//OV8858_write_cmos_sensor( 0x3501, 0x4d);
	//OV8858_write_cmos_sensor( 0x3502, 0x40);
	//OV8858_write_cmos_sensor( 0x3503, 0x00);
	//OV8858_write_cmos_sensor( 0x3505, 0x80);
	//OV8858_write_cmos_sensor( 0x3508, 0x04);
	//OV8858_write_cmos_sensor( 0x3509, 0x00);
	//OV8858_write_cmos_sensor( 0x350c, 0x00);
	//OV8858_write_cmos_sensor( 0x350d, 0x80);
	//OV8858_write_cmos_sensor( 0x3510, 0x00);
	//OV8858_write_cmos_sensor( 0x3511, 0x02);
	//OV8858_write_cmos_sensor( 0x3512, 0x00);
	//OV8858_write_cmos_sensor( 0x3700, 0x18);
	//OV8858_write_cmos_sensor( 0x3701, 0x0c);
	//OV8858_write_cmos_sensor( 0x3702, 0x28);
	//OV8858_write_cmos_sensor( 0x3703, 0x19);
	//OV8858_write_cmos_sensor( 0x3704, 0x14);
	//OV8858_write_cmos_sensor( 0x3705, 0x00);
	//OV8858_write_cmos_sensor( 0x3706, 0x35);
	//OV8858_write_cmos_sensor( 0x3707, 0x04);
	//OV8858_write_cmos_sensor( 0x3708, 0x24);
	//OV8858_write_cmos_sensor( 0x3709, 0x33);
	//OV8858_write_cmos_sensor( 0x370a, 0x00);
	//OV8858_write_cmos_sensor( 0x370b, 0xb5);
	//OV8858_write_cmos_sensor( 0x370c, 0x04);
	//OV8858_write_cmos_sensor( 0x3718, 0x12);
	//OV8858_write_cmos_sensor( 0x3719, 0x31);
	//OV8858_write_cmos_sensor( 0x3712, 0x42);
	//OV8858_write_cmos_sensor( 0x3714, 0x24);
	//OV8858_write_cmos_sensor( 0x371e, 0x19);
	//OV8858_write_cmos_sensor( 0x371f, 0x40);
	//OV8858_write_cmos_sensor( 0x3720, 0x05);
	//OV8858_write_cmos_sensor( 0x3721, 0x05);
	//OV8858_write_cmos_sensor( 0x3724, 0x06);
	//OV8858_write_cmos_sensor( 0x3725, 0x01);
	//OV8858_write_cmos_sensor( 0x3726, 0x06);
	//OV8858_write_cmos_sensor( 0x3728, 0x05);
	//OV8858_write_cmos_sensor( 0x3729, 0x02);
	//OV8858_write_cmos_sensor( 0x372a, 0x03);
	//OV8858_write_cmos_sensor( 0x372b, 0x53);
	//OV8858_write_cmos_sensor( 0x372c, 0xa3);
	//OV8858_write_cmos_sensor( 0x372d, 0x53);
	//OV8858_write_cmos_sensor( 0x372e, 0x06);
	//OV8858_write_cmos_sensor( 0x372f, 0x10);
	//OV8858_write_cmos_sensor( 0x3730, 0x01);
	//OV8858_write_cmos_sensor( 0x3731, 0x06);
	//OV8858_write_cmos_sensor( 0x3732, 0x14);
	//OV8858_write_cmos_sensor( 0x3733, 0x10);
	//OV8858_write_cmos_sensor( 0x3734, 0x40);
	//OV8858_write_cmos_sensor( 0x3736, 0x20);
	//OV8858_write_cmos_sensor( 0x373a, 0x05);
	//OV8858_write_cmos_sensor( 0x373b, 0x06);
	//OV8858_write_cmos_sensor( 0x373c, 0x0a);
	//OV8858_write_cmos_sensor( 0x373e, 0x03);
	//OV8858_write_cmos_sensor( 0x3755, 0x10);
	//OV8858_write_cmos_sensor( 0x3758, 0x00);
	//OV8858_write_cmos_sensor( 0x3759, 0x4c);
	//OV8858_write_cmos_sensor( 0x375a, 0x06);
	//OV8858_write_cmos_sensor( 0x375b, 0x13);
	//OV8858_write_cmos_sensor( 0x375c, 0x20);
	//OV8858_write_cmos_sensor( 0x375d, 0x02);
	//OV8858_write_cmos_sensor( 0x375e, 0x00);
	//OV8858_write_cmos_sensor( 0x375f, 0x14);
	//OV8858_write_cmos_sensor( 0x3768, 0x22);
	//OV8858_write_cmos_sensor( 0x3769, 0x44);
	//OV8858_write_cmos_sensor( 0x376a, 0x44);
	//OV8858_write_cmos_sensor( 0x3761, 0x00);
	//OV8858_write_cmos_sensor( 0x3762, 0x00);
	//OV8858_write_cmos_sensor( 0x3763, 0x00);
	//OV8858_write_cmos_sensor( 0x3766, 0xff);
	//OV8858_write_cmos_sensor( 0x376b, 0x00);
	//OV8858_write_cmos_sensor( 0x3772, 0x23);
	//OV8858_write_cmos_sensor( 0x3773, 0x02);
	//OV8858_write_cmos_sensor( 0x3774, 0x16);
	//OV8858_write_cmos_sensor( 0x3775, 0x12);
	//OV8858_write_cmos_sensor( 0x3776, 0x04);
	//OV8858_write_cmos_sensor( 0x3777, 0x00);
	//OV8858_write_cmos_sensor( 0x3778, 0x1b);
	//OV8858_write_cmos_sensor( 0x37a0, 0x44);
	//OV8858_write_cmos_sensor( 0x37a1, 0x3d);
	//OV8858_write_cmos_sensor( 0x37a2, 0x3d);
	//OV8858_write_cmos_sensor( 0x37a3, 0x00);
	//OV8858_write_cmos_sensor( 0x37a4, 0x00);
	//OV8858_write_cmos_sensor( 0x37a5, 0x00);
	//OV8858_write_cmos_sensor( 0x37a6, 0x00);
	//OV8858_write_cmos_sensor( 0x37a7, 0x44);
	//OV8858_write_cmos_sensor( 0x37a8, 0x4c);
	//OV8858_write_cmos_sensor( 0x37a9, 0x4c);
	//OV8858_write_cmos_sensor( 0x3760, 0x00);
	//OV8858_write_cmos_sensor( 0x376f, 0x01);
	//OV8858_write_cmos_sensor( 0x37aa, 0x44);
	//OV8858_write_cmos_sensor( 0x37ab, 0x2e);
	//OV8858_write_cmos_sensor( 0x37ac, 0x2e);
	//OV8858_write_cmos_sensor( 0x37ad, 0x33);
	//OV8858_write_cmos_sensor( 0x37ae, 0x0d);
	//OV8858_write_cmos_sensor( 0x37af, 0x0d);
	//OV8858_write_cmos_sensor( 0x37b0, 0x00);
	//OV8858_write_cmos_sensor( 0x37b1, 0x00);
	//OV8858_write_cmos_sensor( 0x37b2, 0x00);
	//OV8858_write_cmos_sensor( 0x37b3, 0x42);
	//OV8858_write_cmos_sensor( 0x37b4, 0x42);
	//OV8858_write_cmos_sensor( 0x37b5, 0x33);
	//OV8858_write_cmos_sensor( 0x37b6, 0x00);
	//OV8858_write_cmos_sensor( 0x37b7, 0x00);
	//OV8858_write_cmos_sensor( 0x37b8, 0x00);
	//OV8858_write_cmos_sensor( 0x37b9, 0xff);
	//OV8858_write_cmos_sensor( 0x3800, 0x00);
	//OV8858_write_cmos_sensor( 0x3801, 0x0c);
	//OV8858_write_cmos_sensor( 0x3802, 0x00);
	//OV8858_write_cmos_sensor( 0x3803, 0x0c);
	//OV8858_write_cmos_sensor( 0x3804, 0x0c);
	//OV8858_write_cmos_sensor( 0x3805, 0xd3);
	//OV8858_write_cmos_sensor( 0x3806, 0x09);
	//OV8858_write_cmos_sensor( 0x3807, 0xa3);
	//OV8858_write_cmos_sensor( 0x3808, 0x06);
	//OV8858_write_cmos_sensor( 0x3809, 0x60);
	//OV8858_write_cmos_sensor( 0x380a, 0x04);
	//OV8858_write_cmos_sensor( 0x380b, 0xc8);
	//OV8858_write_cmos_sensor( 0x380c, 0x07);
	//OV8858_write_cmos_sensor( 0x380d, 0x88);
	//OV8858_write_cmos_sensor( 0x380e, 0x04);
	//OV8858_write_cmos_sensor( 0x380f, 0xdc);
	//OV8858_write_cmos_sensor( 0x3810, 0x00);
	//OV8858_write_cmos_sensor( 0x3811, 0x04);
	//OV8858_write_cmos_sensor( 0x3813, 0x02);
	//OV8858_write_cmos_sensor( 0x3814, 0x03);
	//OV8858_write_cmos_sensor( 0x3815, 0x01);
	//OV8858_write_cmos_sensor( 0x3820, 0x06);
	//OV8858_write_cmos_sensor( 0x3821, 0x61);
	//OV8858_write_cmos_sensor( 0x382a, 0x03);
	//OV8858_write_cmos_sensor( 0x382b, 0x01);
	//OV8858_write_cmos_sensor( 0x3830, 0x08);
	//OV8858_write_cmos_sensor( 0x3836, 0x02);
	//OV8858_write_cmos_sensor( 0x3837, 0x18);
	//OV8858_write_cmos_sensor( 0x3841, 0xff);
	//OV8858_write_cmos_sensor( 0x3846, 0x48);
	//OV8858_write_cmos_sensor( 0x3d85, 0x16); // 0x14
	//OV8858_write_cmos_sensor( 0x3d8c, 0x73);
	//OV8858_write_cmos_sensor( 0x3d8d, 0xde);
	//OV8858_write_cmos_sensor( 0x3f08, 0x08);
	//OV8858_write_cmos_sensor( 0x3f0a, 0x80);
	//OV8858_write_cmos_sensor( 0x4000, 0xf1);
	//OV8858_write_cmos_sensor( 0x4001, 0x10);
	//OV8858_write_cmos_sensor( 0x4005, 0x10);
	//OV8858_write_cmos_sensor( 0x4002, 0x27);
	//OV8858_write_cmos_sensor( 0x4006, 0x04);
	//OV8858_write_cmos_sensor( 0x4007, 0x04);
	//OV8858_write_cmos_sensor( 0x4009, 0x81);
	//OV8858_write_cmos_sensor( 0x400b, 0x0c);
	//OV8858_write_cmos_sensor( 0x401b, 0x00);
	//OV8858_write_cmos_sensor( 0x401d, 0x00);
	//OV8858_write_cmos_sensor( 0x4020, 0x00);
	//OV8858_write_cmos_sensor( 0x4021, 0x04);
	//OV8858_write_cmos_sensor( 0x4022, 0x04);
	//OV8858_write_cmos_sensor( 0x4023, 0xb9);
	//OV8858_write_cmos_sensor( 0x4024, 0x05);
	//OV8858_write_cmos_sensor( 0x4025, 0x2a);
	//OV8858_write_cmos_sensor( 0x4026, 0x05);
	//OV8858_write_cmos_sensor( 0x4027, 0x2b);
	//OV8858_write_cmos_sensor( 0x4028, 0x00);
	//OV8858_write_cmos_sensor( 0x4029, 0x02);
	//OV8858_write_cmos_sensor( 0x402a, 0x04);
	//OV8858_write_cmos_sensor( 0x402b, 0x04);
	//OV8858_write_cmos_sensor( 0x402c, 0x02);
	//OV8858_write_cmos_sensor( 0x402d, 0x02);
	//OV8858_write_cmos_sensor( 0x402e, 0x08);
	//OV8858_write_cmos_sensor( 0x402f, 0x02);
	//OV8858_write_cmos_sensor( 0x401f, 0x00);
	//OV8858_write_cmos_sensor( 0x4034, 0x3f);
	//OV8858_write_cmos_sensor( 0x403d, 0x04);
	//OV8858_write_cmos_sensor( 0x4300, 0xff);
	//OV8858_write_cmos_sensor( 0x4301, 0x00);
	//OV8858_write_cmos_sensor( 0x4302, 0x0f);
	//OV8858_write_cmos_sensor( 0x4316, 0x00);
	//OV8858_write_cmos_sensor( 0x4500, 0x38);
	//OV8858_write_cmos_sensor( 0x4503, 0x18);
	//OV8858_write_cmos_sensor( 0x4600, 0x00);
	//OV8858_write_cmos_sensor( 0x4601, 0xcb);
	//OV8858_write_cmos_sensor( 0x481f, 0x32);
	//OV8858_write_cmos_sensor( 0x4837, 0x17);
	//OV8858_write_cmos_sensor( 0x4850, 0x10);
	//OV8858_write_cmos_sensor( 0x4851, 0x32);
	//OV8858_write_cmos_sensor( 0x4b00, 0x2a);
	//OV8858_write_cmos_sensor( 0x4b0d, 0x00);
	//OV8858_write_cmos_sensor( 0x4d00, 0x04);
	//OV8858_write_cmos_sensor( 0x4d01, 0x18);
	//OV8858_write_cmos_sensor( 0x4d02, 0xc3);
	//OV8858_write_cmos_sensor( 0x4d03, 0xff);
	//OV8858_write_cmos_sensor( 0x4d04, 0xff);
	//OV8858_write_cmos_sensor( 0x4d05, 0xff);
	//OV8858_write_cmos_sensor( 0x5000, 0x7e);
	//OV8858_write_cmos_sensor( 0x5001, 0x01);
	//OV8858_write_cmos_sensor( 0x5002, 0x08);
	//OV8858_write_cmos_sensor( 0x5003, 0x20);
	//OV8858_write_cmos_sensor( 0x5046, 0x12);
	//OV8858_write_cmos_sensor( 0x5780, 0xfc);
	//OV8858_write_cmos_sensor( 0x5784, 0x0c);
	//OV8858_write_cmos_sensor( 0x5787, 0x40);
	//OV8858_write_cmos_sensor( 0x5788, 0x08);
	//OV8858_write_cmos_sensor( 0x578a, 0x02);
	//OV8858_write_cmos_sensor( 0x578b, 0x01);
	//OV8858_write_cmos_sensor( 0x578c, 0x01);
	//OV8858_write_cmos_sensor( 0x578e, 0x02);
	//OV8858_write_cmos_sensor( 0x578f, 0x01);
	//OV8858_write_cmos_sensor( 0x5790, 0x01);
//2014-3-31
//nick email add
//fuction
//--which will auto decrease sensor shading gain when sensor gain > 4x. 
//which can improve low light performance.
    //OV8858_write_cmos_sensor( 0x5871, 0x0d);
	//OV8858_write_cmos_sensor( 0x5870, 0x18);
	//OV8858_write_cmos_sensor( 0x586e, 0x10);
	//OV8858_write_cmos_sensor( 0x586f, 0x08);
//2014-3-31 end
	//OV8858_write_cmos_sensor( 0x5901, 0x00);
	//OV8858_write_cmos_sensor( 0x5b00, 0x02);
	//OV8858_write_cmos_sensor( 0x5b01, 0x10);
	//OV8858_write_cmos_sensor( 0x5b02, 0x03);
	//OV8858_write_cmos_sensor( 0x5b03, 0xcf);
	//OV8858_write_cmos_sensor( 0x5b05, 0x6c);
	//OV8858_write_cmos_sensor( 0x5e00, 0x00);
	//OV8858_write_cmos_sensor( 0x5e01, 0x41);
	//OV8858_write_cmos_sensor( 0x382d, 0x7f);
	//OV8858_write_cmos_sensor( 0x4825, 0x3a);
	//OV8858_write_cmos_sensor( 0x4826, 0x40);
	//OV8858_write_cmos_sensor( 0x4808, 0x25);
	//OV8858_write_cmos_sensor( 0x0100, 0x01);
	
	//nick moidify to R2A version 0901
	//@@ OV8858R2A_Initial_1632x1224_30FPS_MIPI_2LANE(Binning)_672Mbps/lane

OV8858_write_cmos_sensor(0x0100, 0x00);
OV8858_write_cmos_sensor(0x0302, 0x1c);
OV8858_write_cmos_sensor(0x0303, 0x00);
OV8858_write_cmos_sensor(0x0304, 0x03);
OV8858_write_cmos_sensor(0x030d, 0x1f);
OV8858_write_cmos_sensor(0x030e, 0x02);
OV8858_write_cmos_sensor(0x030f, 0x04);
OV8858_write_cmos_sensor(0x0312, 0x03);
OV8858_write_cmos_sensor(0x031e, 0x0c);
OV8858_write_cmos_sensor(0x3007, 0x80);
OV8858_write_cmos_sensor(0x3600, 0x00);
OV8858_write_cmos_sensor(0x3601, 0x00);
OV8858_write_cmos_sensor(0x3602, 0x00);
OV8858_write_cmos_sensor(0x3603, 0x00);
OV8858_write_cmos_sensor(0x3604, 0x22);
OV8858_write_cmos_sensor(0x3605, 0x20);
OV8858_write_cmos_sensor(0x3606, 0x00);
OV8858_write_cmos_sensor(0x3607, 0x20);
OV8858_write_cmos_sensor(0x3608, 0x11);
OV8858_write_cmos_sensor(0x3609, 0x28);
OV8858_write_cmos_sensor(0x360a, 0x00);
OV8858_write_cmos_sensor(0x360b, 0x05);
OV8858_write_cmos_sensor(0x360c, 0xd4);
OV8858_write_cmos_sensor(0x360d, 0x40);
OV8858_write_cmos_sensor(0x360e, 0x0c);
OV8858_write_cmos_sensor(0x360f, 0x20);
OV8858_write_cmos_sensor(0x3610, 0x07);
OV8858_write_cmos_sensor(0x3611, 0x20);
OV8858_write_cmos_sensor(0x3612, 0x88);
OV8858_write_cmos_sensor(0x3613, 0x80);
OV8858_write_cmos_sensor(0x3614, 0x58);
OV8858_write_cmos_sensor(0x3615, 0x00);
OV8858_write_cmos_sensor(0x3616, 0x4a);
OV8858_write_cmos_sensor(0x3617, 0x40);
OV8858_write_cmos_sensor(0x3618, 0x5a);
OV8858_write_cmos_sensor(0x3619, 0x70);
OV8858_write_cmos_sensor(0x361a, 0x99);
OV8858_write_cmos_sensor(0x361b, 0x0a);
OV8858_write_cmos_sensor(0x361c, 0x07);
OV8858_write_cmos_sensor(0x361d, 0x00);
OV8858_write_cmos_sensor(0x361e, 0x00);
OV8858_write_cmos_sensor(0x361f, 0x00);
OV8858_write_cmos_sensor(0x3638, 0xff);
OV8858_write_cmos_sensor(0x3633, 0x0f);
OV8858_write_cmos_sensor(0x3634, 0x0f);
OV8858_write_cmos_sensor(0x3635, 0x0f);
OV8858_write_cmos_sensor(0x3636, 0x12);
OV8858_write_cmos_sensor(0x3645, 0x13);
OV8858_write_cmos_sensor(0x3646, 0x83);
OV8858_write_cmos_sensor(0x364a, 0x07);
OV8858_write_cmos_sensor(0x3015, 0x00);
OV8858_write_cmos_sensor(0x3018, 0x72);
OV8858_write_cmos_sensor(0x3020, 0x93);
OV8858_write_cmos_sensor(0x3022, 0x01);
OV8858_write_cmos_sensor(0x3031, 0x0a);
OV8858_write_cmos_sensor(0x3034, 0x00);
OV8858_write_cmos_sensor(0x3106, 0x01);
OV8858_write_cmos_sensor(0x3305, 0xf1);
OV8858_write_cmos_sensor(0x3308, 0x00);
OV8858_write_cmos_sensor(0x3309, 0x28);
OV8858_write_cmos_sensor(0x330a, 0x00);
OV8858_write_cmos_sensor(0x330b, 0x20);
OV8858_write_cmos_sensor(0x330c, 0x00);
OV8858_write_cmos_sensor(0x330d, 0x00);
OV8858_write_cmos_sensor(0x330e, 0x00);
OV8858_write_cmos_sensor(0x330f, 0x40);
OV8858_write_cmos_sensor(0x3307, 0x04);
OV8858_write_cmos_sensor(0x3500, 0x00);
OV8858_write_cmos_sensor(0x3501, 0x4d);
OV8858_write_cmos_sensor(0x3502, 0x40);
OV8858_write_cmos_sensor(0x3503, 0x80);
OV8858_write_cmos_sensor(0x3505, 0x80);
OV8858_write_cmos_sensor(0x3508, 0x02);
OV8858_write_cmos_sensor(0x3509, 0x00);
OV8858_write_cmos_sensor(0x350c, 0x00);
OV8858_write_cmos_sensor(0x350d, 0x80);
OV8858_write_cmos_sensor(0x3510, 0x00);
OV8858_write_cmos_sensor(0x3511, 0x02);
OV8858_write_cmos_sensor(0x3512, 0x00);
OV8858_write_cmos_sensor(0x3700, 0x18);
OV8858_write_cmos_sensor(0x3701, 0x0c);
OV8858_write_cmos_sensor(0x3702, 0x28);
OV8858_write_cmos_sensor(0x3703, 0x19);
OV8858_write_cmos_sensor(0x3704, 0x14);
OV8858_write_cmos_sensor(0x3705, 0x00);
OV8858_write_cmos_sensor(0x3706, 0x82);
OV8858_write_cmos_sensor(0x3707, 0x04);
OV8858_write_cmos_sensor(0x3708, 0x24);
OV8858_write_cmos_sensor(0x3709, 0x33);
OV8858_write_cmos_sensor(0x370a, 0x01);
OV8858_write_cmos_sensor(0x370b, 0x82);
OV8858_write_cmos_sensor(0x370c, 0x04);
OV8858_write_cmos_sensor(0x3718, 0x12);
OV8858_write_cmos_sensor(0x3719, 0x31);
OV8858_write_cmos_sensor(0x3712, 0x42);
OV8858_write_cmos_sensor(0x3714, 0x24);
OV8858_write_cmos_sensor(0x371e, 0x19);
OV8858_write_cmos_sensor(0x371f, 0x40);
OV8858_write_cmos_sensor(0x3720, 0x05);
OV8858_write_cmos_sensor(0x3721, 0x05);
OV8858_write_cmos_sensor(0x3724, 0x06);
OV8858_write_cmos_sensor(0x3725, 0x01);
OV8858_write_cmos_sensor(0x3726, 0x06);
OV8858_write_cmos_sensor(0x3728, 0x05);
OV8858_write_cmos_sensor(0x3729, 0x02);
OV8858_write_cmos_sensor(0x372a, 0x03);
OV8858_write_cmos_sensor(0x372b, 0x53);
OV8858_write_cmos_sensor(0x372c, 0xa3);
OV8858_write_cmos_sensor(0x372d, 0x53);
OV8858_write_cmos_sensor(0x372e, 0x06);
OV8858_write_cmos_sensor(0x372f, 0x10);
OV8858_write_cmos_sensor(0x3730, 0x01);
OV8858_write_cmos_sensor(0x3731, 0x06);
OV8858_write_cmos_sensor(0x3732, 0x14);
OV8858_write_cmos_sensor(0x3733, 0x10);
OV8858_write_cmos_sensor(0x3734, 0x40);
OV8858_write_cmos_sensor(0x3736, 0x20);
OV8858_write_cmos_sensor(0x373a, 0x05);
OV8858_write_cmos_sensor(0x373b, 0x06);
OV8858_write_cmos_sensor(0x373c, 0x0a);
OV8858_write_cmos_sensor(0x373e, 0x03);
OV8858_write_cmos_sensor(0x3750, 0x0a);
OV8858_write_cmos_sensor(0x3751, 0x0e);
OV8858_write_cmos_sensor(0x3755, 0x10);
OV8858_write_cmos_sensor(0x3758, 0x00);
OV8858_write_cmos_sensor(0x3759, 0x4c);
OV8858_write_cmos_sensor(0x375a, 0x06);
OV8858_write_cmos_sensor(0x375b, 0x13);
OV8858_write_cmos_sensor(0x375c, 0x20);
OV8858_write_cmos_sensor(0x375d, 0x02);
OV8858_write_cmos_sensor(0x375e, 0x00);
OV8858_write_cmos_sensor(0x375f, 0x14);
OV8858_write_cmos_sensor(0x3768, 0xcc);
OV8858_write_cmos_sensor(0x3769, 0x44);
OV8858_write_cmos_sensor(0x376a, 0x44);
OV8858_write_cmos_sensor(0x3761, 0x00);
OV8858_write_cmos_sensor(0x3762, 0x00);
OV8858_write_cmos_sensor(0x3763, 0x18);
OV8858_write_cmos_sensor(0x3766, 0xff);
OV8858_write_cmos_sensor(0x376b, 0x00);
OV8858_write_cmos_sensor(0x3772, 0x23);
OV8858_write_cmos_sensor(0x3773, 0x02);
OV8858_write_cmos_sensor(0x3774, 0x16);
OV8858_write_cmos_sensor(0x3775, 0x12);
OV8858_write_cmos_sensor(0x3776, 0x04);
OV8858_write_cmos_sensor(0x3777, 0x00);
OV8858_write_cmos_sensor(0x3778, 0x17);
OV8858_write_cmos_sensor(0x37a0, 0x44);
OV8858_write_cmos_sensor(0x37a1, 0x3d);
OV8858_write_cmos_sensor(0x37a2, 0x3d);
OV8858_write_cmos_sensor(0x37a3, 0x00);
OV8858_write_cmos_sensor(0x37a4, 0x00);
OV8858_write_cmos_sensor(0x37a5, 0x00);
OV8858_write_cmos_sensor(0x37a6, 0x00);
OV8858_write_cmos_sensor(0x37a7, 0x44);
OV8858_write_cmos_sensor(0x37a8, 0x4c);
OV8858_write_cmos_sensor(0x37a9, 0x4c);
OV8858_write_cmos_sensor(0x3760, 0x00);
OV8858_write_cmos_sensor(0x376f, 0x01);
OV8858_write_cmos_sensor(0x37aa, 0x44);
OV8858_write_cmos_sensor(0x37ab, 0x2e);
OV8858_write_cmos_sensor(0x37ac, 0x2e);
OV8858_write_cmos_sensor(0x37ad, 0x33);
OV8858_write_cmos_sensor(0x37ae, 0x0d);
OV8858_write_cmos_sensor(0x37af, 0x0d);
OV8858_write_cmos_sensor(0x37b0, 0x00);
OV8858_write_cmos_sensor(0x37b1, 0x00);
OV8858_write_cmos_sensor(0x37b2, 0x00);
OV8858_write_cmos_sensor(0x37b3, 0x42);
OV8858_write_cmos_sensor(0x37b4, 0x42);
OV8858_write_cmos_sensor(0x37b5, 0x31);
OV8858_write_cmos_sensor(0x37b6, 0x00);
OV8858_write_cmos_sensor(0x37b7, 0x00);
OV8858_write_cmos_sensor(0x37b8, 0x00);
OV8858_write_cmos_sensor(0x37b9, 0xff);
OV8858_write_cmos_sensor(0x3800, 0x00);
OV8858_write_cmos_sensor(0x3801, 0x0c);
OV8858_write_cmos_sensor(0x3802, 0x00);
OV8858_write_cmos_sensor(0x3803, 0x0c);
OV8858_write_cmos_sensor(0x3804, 0x0c);
OV8858_write_cmos_sensor(0x3805, 0xd3);
OV8858_write_cmos_sensor(0x3806, 0x09);
OV8858_write_cmos_sensor(0x3807, 0xa3);
OV8858_write_cmos_sensor(0x3808, 0x06);
OV8858_write_cmos_sensor(0x3809, 0x60);
OV8858_write_cmos_sensor(0x380a, 0x04);
OV8858_write_cmos_sensor(0x380b, 0xc8);
OV8858_write_cmos_sensor(0x380c, 0x07);
OV8858_write_cmos_sensor(0x380d, 0x88);
OV8858_write_cmos_sensor(0x380e, 0x04);
OV8858_write_cmos_sensor(0x380f, 0xdc);
OV8858_write_cmos_sensor(0x3810, 0x00);
OV8858_write_cmos_sensor(0x3811, 0x04);
OV8858_write_cmos_sensor(0x3813, 0x02);
OV8858_write_cmos_sensor(0x3814, 0x03);
OV8858_write_cmos_sensor(0x3815, 0x01);
OV8858_write_cmos_sensor(0x3820, 0x00);
OV8858_write_cmos_sensor(0x3821, 0x67);
OV8858_write_cmos_sensor(0x382a, 0x03);
OV8858_write_cmos_sensor(0x382b, 0x01);
OV8858_write_cmos_sensor(0x3830, 0x08);
OV8858_write_cmos_sensor(0x3836, 0x02);
OV8858_write_cmos_sensor(0x3837, 0x18);
OV8858_write_cmos_sensor(0x3841, 0xff);
OV8858_write_cmos_sensor(0x3846, 0x48);
OV8858_write_cmos_sensor(0x3d85, 0x16);
OV8858_write_cmos_sensor(0x3d8c, 0x73);
OV8858_write_cmos_sensor(0x3d8d, 0xde);
OV8858_write_cmos_sensor(0x3f08, 0x08);
OV8858_write_cmos_sensor(0x3f0a, 0x00);
OV8858_write_cmos_sensor(0x4000, 0xf1);
OV8858_write_cmos_sensor(0x4001, 0x10);
OV8858_write_cmos_sensor(0x4005, 0x10);
OV8858_write_cmos_sensor(0x4002, 0x27);
OV8858_write_cmos_sensor(0x4009, 0x83);
OV8858_write_cmos_sensor(0x400a, 0x01);
OV8858_write_cmos_sensor(0x400b, 0x0c);
OV8858_write_cmos_sensor(0x400d, 0x10);
OV8858_write_cmos_sensor(0x4011, 0x20);
OV8858_write_cmos_sensor(0x401b, 0x00);
OV8858_write_cmos_sensor(0x401d, 0x00);
OV8858_write_cmos_sensor(0x4020, 0x00);
OV8858_write_cmos_sensor(0x4021, 0x04);
OV8858_write_cmos_sensor(0x4022, 0x04);
OV8858_write_cmos_sensor(0x4023, 0xb9);
OV8858_write_cmos_sensor(0x4024, 0x05);
OV8858_write_cmos_sensor(0x4025, 0x2a);
OV8858_write_cmos_sensor(0x4026, 0x05);
OV8858_write_cmos_sensor(0x4027, 0x2b);
OV8858_write_cmos_sensor(0x4028, 0x00);
OV8858_write_cmos_sensor(0x4029, 0x02);
OV8858_write_cmos_sensor(0x402a, 0x04);
OV8858_write_cmos_sensor(0x402b, 0x04);
OV8858_write_cmos_sensor(0x402c, 0x02);
OV8858_write_cmos_sensor(0x402d, 0x02);
OV8858_write_cmos_sensor(0x402e, 0x08);
OV8858_write_cmos_sensor(0x402f, 0x02);
OV8858_write_cmos_sensor(0x401f, 0x00);
OV8858_write_cmos_sensor(0x4034, 0x3f);
OV8858_write_cmos_sensor(0x403d, 0x04);
OV8858_write_cmos_sensor(0x403e, 0x08);
OV8858_write_cmos_sensor(0x4040, 0x07);
OV8858_write_cmos_sensor(0x4041, 0xc6);
OV8858_write_cmos_sensor(0x4300, 0xff);
OV8858_write_cmos_sensor(0x4202, 0x00);
OV8858_write_cmos_sensor(0x4301, 0x00);
OV8858_write_cmos_sensor(0x4302, 0x0f);
OV8858_write_cmos_sensor(0x4316, 0x00);
OV8858_write_cmos_sensor(0x4500, 0x58);
OV8858_write_cmos_sensor(0x4503, 0x18);
OV8858_write_cmos_sensor(0x4600, 0x00);
OV8858_write_cmos_sensor(0x4601, 0xcb);
OV8858_write_cmos_sensor(0x470b, 0x28);
OV8858_write_cmos_sensor(0x481f, 0x32);
OV8858_write_cmos_sensor(0x4837, 0x17);
OV8858_write_cmos_sensor(0x4850, 0x10);
OV8858_write_cmos_sensor(0x4851, 0x32);
OV8858_write_cmos_sensor(0x4b00, 0x2a);
OV8858_write_cmos_sensor(0x4b0d, 0x00);
OV8858_write_cmos_sensor(0x4d00, 0x04);
OV8858_write_cmos_sensor(0x4d01, 0x18);
OV8858_write_cmos_sensor(0x4d02, 0xc3);
OV8858_write_cmos_sensor(0x4d03, 0xff);
OV8858_write_cmos_sensor(0x4d04, 0xff);
OV8858_write_cmos_sensor(0x4d05, 0xff);
OV8858_write_cmos_sensor(0x5000, 0x7e);
OV8858_write_cmos_sensor(0x5001, 0x01);
OV8858_write_cmos_sensor(0x5002, 0x08);
OV8858_write_cmos_sensor(0x5003, 0x20);
OV8858_write_cmos_sensor(0x5046, 0x12);
OV8858_write_cmos_sensor(0x5780, 0x3e);
OV8858_write_cmos_sensor(0x5781, 0x0f);
OV8858_write_cmos_sensor(0x5782, 0x44);
OV8858_write_cmos_sensor(0x5783, 0x02);
OV8858_write_cmos_sensor(0x5784, 0x01);
OV8858_write_cmos_sensor(0x5785, 0x00);
OV8858_write_cmos_sensor(0x5786, 0x00);
OV8858_write_cmos_sensor(0x5787, 0x04);
OV8858_write_cmos_sensor(0x5788, 0x02);
OV8858_write_cmos_sensor(0x5789, 0x0f);
OV8858_write_cmos_sensor(0x578a, 0xfd);
OV8858_write_cmos_sensor(0x578b, 0xf5);
OV8858_write_cmos_sensor(0x578c, 0xf5);
OV8858_write_cmos_sensor(0x578d, 0x03);
OV8858_write_cmos_sensor(0x578e, 0x08);
OV8858_write_cmos_sensor(0x578f, 0x0c);
OV8858_write_cmos_sensor(0x5790, 0x08);
OV8858_write_cmos_sensor(0x5791, 0x04);
OV8858_write_cmos_sensor(0x5792, 0x00);
OV8858_write_cmos_sensor(0x5793, 0x52);
OV8858_write_cmos_sensor(0x5794, 0xa3);
OV8858_write_cmos_sensor(0x5871, 0x0d);
OV8858_write_cmos_sensor(0x5870, 0x18);
OV8858_write_cmos_sensor(0x586e, 0x10);
OV8858_write_cmos_sensor(0x586f, 0x08);
OV8858_write_cmos_sensor(0x58f8, 0x3d);
OV8858_write_cmos_sensor(0x5901, 0x00);
OV8858_write_cmos_sensor(0x5b00, 0x02);
OV8858_write_cmos_sensor(0x5b01, 0x10);
OV8858_write_cmos_sensor(0x5b02, 0x03);
OV8858_write_cmos_sensor(0x5b03, 0xcf);
OV8858_write_cmos_sensor(0x5b05, 0x6c);
OV8858_write_cmos_sensor(0x5e00, 0x00);
OV8858_write_cmos_sensor(0x5e01, 0x41);
OV8858_write_cmos_sensor(0x382d, 0x7f);
OV8858_write_cmos_sensor(0x4825, 0x3a);
OV8858_write_cmos_sensor(0x4826, 0x40);
OV8858_write_cmos_sensor(0x4808, 0x25);
OV8858_write_cmos_sensor(0x0100, 0x01);
	                                 
	
	//otp apply by o-film 2014-3-28
	#if GN_MTK_BSP_LENS_COMPABILITY 
	//#if 1  //OTP AWB Start
	OV8858_write_cmos_sensor( 0x5002, 0x00);
	mdelay(2);
			ov8858_mipi_update_wb_register_from_otp();
	//#endif //OTP AWB end
	//#if 1 //otp lens shading start
			ov8858_mipi_update_lenc_register_from_otp();
	OV8858_write_cmos_sensor( 0x5002, 0x08);
	mdelay(2);
	//#endif //otp lens shading end
	#endif


}


UINT32 OV8858Open(void)
{

	volatile signed int i;
	kal_uint16 sensor_id = 0;

	OV8858DB("OV8858 Open enter :\n ");
	OV8858_write_cmos_sensor(0x0103,0x01);// Reset sensor
    mdelay(2);

	for(i=0;i<2;i++)
	{
		sensor_id = (OV8858_read_cmos_sensor(0x300B)<<8)|OV8858_read_cmos_sensor(0x300C);
		OV8858DB("OV8858 READ ID :%x",sensor_id);
		if(sensor_id != OV8858_SENSOR_ID)
		{
			return ERROR_SENSOR_CONNECT_FAIL;
		}else
			break;
	}
	
	OV8858_Sensor_Init();
	mdelay(40);
    OV8858_Init_Para();
	
	#ifdef OV8858_DEBUG
		OV8858DB("[OV8858Open] enter and exit."); 
	#endif

    return ERROR_NONE;
}


//Gionee yanggy 2013-09-03 add for module compability begin
#ifdef GN_MTK_BSP_LENS_COMPABILITY
static int OV8858_MIPI_GetModuleInfo(unsigned int *module_id, unsigned int *lens_id)
{
    kal_uint16 temp, i, otp_index,bank;
    kal_uint32 address;
		OV8858_write_cmos_sensor( 0x5002, 0x00);
	mdelay(2);
    //check first wb OTP with valid OTP
    for(i = 1; i < 4; i++)
    {
        temp = ov8858_mipi_check_otp_info(i);
        if(temp == 2)
        {
            otp_index = i;
            break;
        }
    }
    if( i == 4)
    {
        OV8858DB("[ov8858_mipi_otp_info error]\r\n");
        return -1;
    }

		if(ov8858_mipi_read_otp_info(otp_index,&current_otp))
			{
				return 0;
			}

    *module_id = current_otp.module_integrator_id;
		*lens_id =current_otp.lens_id;

    printk("[OV8858_MIPI_GetModuleInfo] the module id is : 0x%02x , the lens id is :0x%02x ~~~\n",*module_id,*lens_id );
    return 0;
		OV8858_write_cmos_sensor( 0x5002, 0x08);
	mdelay(2);

}
#endif
//Gionee yanggy 2013-09-03 add for module compability end


UINT32 OV8858GetSensorID(UINT32 *sensorID)
{
    int  retry = 2;


//Gionee yanggy 2013-09-03 add for module compbility begin
#ifdef GN_MTK_BSP_LENS_COMPABILITY
    unsigned int module_id,lens_id;
#endif
//Gionee yanggy 2013-09-03 add for module compbility end

	OV8858DB("OV8858GetSensorID enter :\n ");

    int i;
    const kal_uint16 sccb_writeid[] = {OV8858MIPI_WRITE_ID,OV8858MIPI_WRITE_ID1,OV8858MIPI_WRITE_ID2};
    for (i=0;i<(sizeof(sccb_writeid)/sizeof(sccb_writeid[0]));i++){
    do {
        ov8858.write_id = sccb_writeid[i];
        ov8858.read_id = (sccb_writeid[i]|0x01);
        *sensorID = (OV8858_read_cmos_sensor(0x300B)<<8)|OV8858_read_cmos_sensor(0x300C);
        if (*sensorID == OV8858_SENSOR_ID)
        	{
        		OV8858DB("Sensor ID = 0x%04x\n", *sensorID);
            	break;
        	}
        OV8858DB("[OV8858GetSensorID] Read Sensor ID Fail = 0x%04x ,i2c addr is:0x%02x====\n ", *sensorID,ov8858.write_id);
        retry--;
    } while (retry > 0);
    }
    if (*sensorID != OV8858_SENSOR_ID) {
        *sensorID = 0xFFFFFFFF;
        return ERROR_SENSOR_CONNECT_FAIL;
    }

//Gionee yanggy 2013-09-03 add for module compbility begin
#ifdef GN_MTK_BSP_LENS_COMPABILITY
    OV8858_Sensor_Init();
    OV8858_MIPI_GetModuleInfo(&module_id, &lens_id);
    OV8858_MIPI_Module_id = module_id;
    OV8858_MIPI_Module_Lens_id = lens_id;
 
    if(0x01 != module_id)
    {
        *sensorID = 0xFFFFFFFF;
        OV8858DB("[OV8858_MIPIGetSensorID] the module_id :0x%02x is not correct !!!!\n",module_id);
        return ERROR_SENSOR_CONNECT_FAIL;
    }
    else
    {
        *sensorID = OV8858_SENSOR_ID;
        OV8858DB("[OV8858_MIPIGetSensorID] the module_id is :0x%02x ,the lens_id is :0x%02x ~~~\n",module_id ,lens_id );

    }
#endif
//Gionee yanggy 2013-09-03 add for module compbility end	

    return ERROR_NONE;
}

UINT32 OV8858Close(void)
{
	#ifdef OV8858_DEBUG
		OV8858DB("[OV8858Close]enter and exit.\n");
	#endif

    return ERROR_NONE;
}

#if 0
void OV8865SetFlipMirror(kal_int32 imgMirror)
{
	kal_int16 mirror=0,flip=0;
	mirror= OV8865_read_cmos_sensor(0x3820);
	flip  = OV8865_read_cmos_sensor(0x3821);

    switch (imgMirror)
    {
        case IMAGE_H_MIRROR://IMAGE_NORMAL:
            OV8865_write_cmos_sensor(0x3820, (mirror & (0xF9)));//Set normal
            OV8865_write_cmos_sensor(0x3821, (flip & (0xF9)));	//Set normal
            break;
        case IMAGE_NORMAL://IMAGE_V_MIRROR:
            OV8865_write_cmos_sensor(0x3820, (mirror & (0xF9)));//Set flip
            OV8865_write_cmos_sensor(0x3821, (flip | (0x06)));	//Set flip
            break;
        case IMAGE_HV_MIRROR://IMAGE_H_MIRROR:
            OV8865_write_cmos_sensor(0x3820, (mirror |(0x06)));	//Set mirror
            OV8865_write_cmos_sensor(0x3821, (flip & (0xF9)));	//Set mirror
            break;
        case IMAGE_V_MIRROR://IMAGE_HV_MIRROR:
            OV8865_write_cmos_sensor(0x3820, (mirror |(0x06)));	//Set mirror & flip
            OV8865_write_cmos_sensor(0x3821, (flip |(0x06)));	//Set mirror & flip
            break;
    }
}
#endif

kal_uint32 OV8858_SET_FrameLength_ByVideoMode(UINT16 Video_TargetFps)
{

    UINT32 frameRate = 0;
	kal_uint32 MIN_FrameLength=0;
	
	if(ov8858.OV8858AutoFlickerMode == KAL_TRUE)
	{
		if (Video_TargetFps==30)
			frameRate= OV8858_AUTOFLICKER_OFFSET_30;
		else if(Video_TargetFps==15)
			frameRate= OV8858_AUTOFLICKER_OFFSET_15;
		else
			frameRate=Video_TargetFps*10;
	
		MIN_FrameLength = (ov8858.videoPclk*10000)/(OV8858_VIDEO_PERIOD_PIXEL_NUMS + ov8858.DummyPixels)/frameRate*10;
	}
	else
		MIN_FrameLength = (ov8858.videoPclk*10000) /(OV8858_VIDEO_PERIOD_PIXEL_NUMS + ov8858.DummyPixels)/Video_TargetFps;

     return MIN_FrameLength;


}



UINT32 OV8858SetVideoMode(UINT16 u2FrameRate)
{
    kal_uint32 MIN_Frame_length =0,frameRate=0,extralines=0;
    OV8858DB("[OV8858SetVideoMode] frame rate = %d\n", u2FrameRate);

	spin_lock(&ov8858mipiraw_drv_lock);
	OV8858_VIDEO_MODE_TARGET_FPS=u2FrameRate;
	spin_unlock(&ov8858mipiraw_drv_lock);

	if(u2FrameRate==0)
	{
		OV8858DB("Disable Video Mode or dynimac fps\n");
		return KAL_TRUE;
	}
	if(u2FrameRate >30 || u2FrameRate <5)
	    OV8858DB("abmornal frame rate seting,pay attention~\n");

    if(ov8858.sensorMode == SENSOR_MODE_VIDEO)//video ScenarioId recording
    {

        MIN_Frame_length = OV8858_SET_FrameLength_ByVideoMode(u2FrameRate);

		if((MIN_Frame_length <=OV8858_VIDEO_PERIOD_LINE_NUMS))
		{
			MIN_Frame_length = OV8858_VIDEO_PERIOD_LINE_NUMS;
			OV8858DB("[OV8858SetVideoMode]current fps = %d\n", (ov8858.videoPclk*10000)  /(OV8858_VIDEO_PERIOD_PIXEL_NUMS)/OV8858_VIDEO_PERIOD_LINE_NUMS);
		}
		OV8858DB("[OV8858SetVideoMode]current fps (10 base)= %d\n", (ov8858.videoPclk*10000)*10/(OV8858_VIDEO_PERIOD_PIXEL_NUMS + ov8858.DummyPixels)/MIN_Frame_length);
		extralines = MIN_Frame_length - OV8858_VIDEO_PERIOD_LINE_NUMS;
		
		spin_lock(&ov8858mipiraw_drv_lock);
		ov8858.DummyPixels = 0;//define dummy pixels and lines
		ov8858.DummyLines = extralines ;
		spin_unlock(&ov8858mipiraw_drv_lock);
		
		OV8858_SetDummy(0, extralines);
    }
	
	OV8858DB("[OV8858SetVideoMode]MIN_Frame_length=%d,ov8858.DummyLines=%d\n",MIN_Frame_length,ov8858.DummyLines);

    return KAL_TRUE;
}


UINT32 OV8858SetAutoFlickerMode(kal_bool bEnable, UINT16 u2FrameRate)
{
	if(bEnable) {   
		spin_lock(&ov8858mipiraw_drv_lock);
		ov8858.OV8858AutoFlickerMode = KAL_TRUE;
		spin_unlock(&ov8858mipiraw_drv_lock);
        OV8858DB("OV8858 Enable Auto flicker\n");
    } else {
    	spin_lock(&ov8858mipiraw_drv_lock);
        ov8858.OV8858AutoFlickerMode = KAL_FALSE;
		spin_unlock(&ov8858mipiraw_drv_lock);
        OV8858DB("OV8858 Disable Auto flicker\n");
    }

    return ERROR_NONE;
}


UINT32 OV8858SetTestPatternMode(kal_bool bEnable)
{
    OV8858DB("[OV8858SetTestPatternMode] Test pattern enable:%d\n", bEnable);
    if(bEnable == KAL_TRUE)
    {
        OV8858_During_testpattern = KAL_TRUE;
		OV8858_write_cmos_sensor(0x5E00,0x80);
    }
	else
	{
        OV8858_During_testpattern = KAL_FALSE;
		OV8858_write_cmos_sensor(0x5E00,0x00);
	}

    return ERROR_NONE;
}


/*************************************************************************
*
* DESCRIPTION:
* INTERFACE FUNCTION, FOR USER TO SET MAX  FRAMERATE;
* 
*************************************************************************/
UINT32 OV8858MIPISetMaxFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 frameRate) 
{
	kal_uint32 pclk;
	kal_int16 dummyLine;
	kal_uint16 lineLength,frameHeight;
		
	OV8858DB("OV8858MIPISetMaxFramerateByScenario: scenarioId = %d, frame rate = %d\n",scenarioId,frameRate);
	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			pclk = OV8858_PREVIEW_PCLK;
			lineLength = OV8858_PV_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - OV8858_PV_PERIOD_LINE_NUMS;
			if(dummyLine<0)
				dummyLine = 0;
			spin_lock(&ov8858mipiraw_drv_lock);
			ov8858.sensorMode = SENSOR_MODE_PREVIEW;
			spin_unlock(&ov8858mipiraw_drv_lock);
			OV8858_SetDummy(0, dummyLine);			
			break;			
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			pclk = OV8858_VIDEO_PCLK;
			lineLength = OV8858_VIDEO_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - OV8858_VIDEO_PERIOD_LINE_NUMS;
			if(dummyLine<0)
				dummyLine = 0;
			spin_lock(&ov8858mipiraw_drv_lock);
			ov8858.sensorMode = SENSOR_MODE_VIDEO;
			spin_unlock(&ov8858mipiraw_drv_lock);
			OV8858_SetDummy(0, dummyLine);			
			break;			
			 break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:			
			pclk = OV8858_CAPTURE_PCLK;
			lineLength = OV8858_FULL_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - OV8858_FULL_PERIOD_LINE_NUMS;
			if(dummyLine<0)
				dummyLine = 0;
			spin_lock(&ov8858mipiraw_drv_lock);
			ov8858.sensorMode = SENSOR_MODE_CAPTURE;
			spin_unlock(&ov8858mipiraw_drv_lock);
			OV8858_SetDummy(0, dummyLine);			
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


UINT32 OV8858MIPIGetDefaultFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 *pframeRate) 
{

	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			 *pframeRate = OV8858_MAX_FPS_PREVIEW;
			 break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			 *pframeRate = OV8858_MAX_FPS_CAPTURE;
			break;		
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added   
			 *pframeRate = OV8858_MAX_FPS_CAPTURE;
			break;		
		default:
			break;
	}

	return ERROR_NONE;

}


void OV8858_NightMode(kal_bool bEnable)
{
	
	#ifdef OV8858_DEBUG
	OV8858DB("[OV8858_NightMode]enter and exit.\n");
	#endif
}



UINT32 OV8858Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

	OV8858DB("OV8858Preview enter:");

	OV8858PreviewSetting();

	spin_lock(&ov8858mipiraw_drv_lock);
	ov8858.sensorMode = SENSOR_MODE_PREVIEW; 
	ov8858.DummyPixels = 0;
	ov8858.DummyLines = 0 ;
	OV8858_FeatureControl_PERIOD_PixelNum=OV8858_PV_PERIOD_PIXEL_NUMS+ ov8858.DummyPixels;
	OV8858_FeatureControl_PERIOD_LineNum=OV8858_PV_PERIOD_LINE_NUMS+ov8858.DummyLines;
	//TODO~
	//ov8858.imgMirror = sensor_config_data->SensorImageMirror;
	spin_unlock(&ov8858mipiraw_drv_lock);
	
	//OV8858SetFlipMirror(sensor_config_data->SensorImageMirror);
	//TODO~
    mdelay(40);//THIS DELAY SHOULD BE NEED BY CTS OR MONKEY
	OV8858DB("OV8858Preview exit:\n");

	  
    return ERROR_NONE;
}


UINT32 OV8858Video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	OV8858DB("OV8858Video enter:");

	OV8858VideoSetting();

	spin_lock(&ov8858mipiraw_drv_lock);
	ov8858.sensorMode = SENSOR_MODE_VIDEO;
	OV8858_FeatureControl_PERIOD_PixelNum=OV8858_VIDEO_PERIOD_PIXEL_NUMS+ ov8858.DummyPixels;
	OV8858_FeatureControl_PERIOD_LineNum=OV8858_VIDEO_PERIOD_LINE_NUMS+ov8858.DummyLines;
	ov8858.imgMirror = sensor_config_data->SensorImageMirror;
	spin_unlock(&ov8858mipiraw_drv_lock);
	
	//OV8865SetFlipMirror(sensor_config_data->SensorImageMirror);

    mdelay(40);//THIS DELAY SHOULD BE NEED BY CTS OR MONKEY
	OV8858DB("OV8865Video exit:\n");
    return ERROR_NONE;
}


UINT32 OV8858Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                            MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

 	kal_uint32 shutter = ov8858.shutter;
	kal_uint32 temp_data;


	OV8858DB("OV8858Capture enter:\n");

	OV8858CaptureSetting();

	spin_lock(&ov8858mipiraw_drv_lock);
	ov8858.sensorMode = SENSOR_MODE_CAPTURE;
	//TODO~
	//ov8858.imgMirror = sensor_config_data->SensorImageMirror;
	ov8858.DummyPixels = 0;
	ov8858.DummyLines = 0 ;
	OV8858_FeatureControl_PERIOD_PixelNum = OV8858_FULL_PERIOD_PIXEL_NUMS + ov8858.DummyPixels;
	OV8858_FeatureControl_PERIOD_LineNum = OV8858_FULL_PERIOD_LINE_NUMS + ov8858.DummyLines;
	spin_unlock(&ov8858mipiraw_drv_lock);

	//OV8865SetFlipMirror(sensor_config_data->SensorImageMirror);
    mdelay(40);//THIS DELAY SHOULD BE NEED BY CTS OR MONKEY

	#if 0
	if(OV8858_During_testpattern == KAL_TRUE)
	{
		//TODO~
		//Test pattern
		OV8858_write_cmos_sensor(0x5E00,0x80);
	}
	#endif
	OV8858DB("OV8865Capture exit:\n");
    return ERROR_NONE;

}	

#if 0
#endif

UINT32 OV8858GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{

    OV8858DB("OV8858GetResolution!!\n");

	pSensorResolution->SensorPreviewWidth	= OV8858_IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorPreviewHeight	= OV8858_IMAGE_SENSOR_PV_HEIGHT;
	
    pSensorResolution->SensorFullWidth		= OV8858_IMAGE_SENSOR_FULL_WIDTH;
    pSensorResolution->SensorFullHeight		= OV8858_IMAGE_SENSOR_FULL_HEIGHT;
	
    pSensorResolution->SensorVideoWidth		= OV8858_IMAGE_SENSOR_VIDEO_WIDTH;
    pSensorResolution->SensorVideoHeight    = OV8858_IMAGE_SENSOR_VIDEO_HEIGHT;
    return ERROR_NONE;
}   

UINT32 OV8858GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
                                                MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    OV8858DB("OV8858GetInfo enter!!\n");
	spin_lock(&ov8858mipiraw_drv_lock);
	ov8858.imgMirror = pSensorConfigData->SensorImageMirror ;
	spin_unlock(&ov8858mipiraw_drv_lock);

    pSensorInfo->SensorOutputDataFormat= SENSOR_OUTPUT_FORMAT_RAW_B;
   
    pSensorInfo->SensorClockPolarity =SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;

    pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_MIPI;

//Gionee yanggy modify for CAM Performance Optimization begin
#ifdef ORIGINAL_VERSION
    pSensorInfo->CaptureDelayFrame = 3;
    pSensorInfo->PreviewDelayFrame = 3;
#else
    pSensorInfo->CaptureDelayFrame = 1;
    pSensorInfo->PreviewDelayFrame = 1;
#endif
//Gionee yanggy modify for CAM Performance Optimization end
    pSensorInfo->VideoDelayFrame = 3;

    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_8MA;
    pSensorInfo->AEShutDelayFrame = 0;	    
    pSensorInfo->AESensorGainDelayFrame = 0;
    pSensorInfo->AEISPGainDelayFrame = 2;
	pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_4_LANE;
	pSensorInfo->MIPIsensorType = MIPI_OPHY_CSI2;


    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockRisingCount= 0;

            pSensorInfo->SensorGrabStartX = OV8858_PV_X_START;
            pSensorInfo->SensorGrabStartY = OV8858_PV_Y_START;
			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	     	pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 30;
	    	pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockRisingCount= 0;

            pSensorInfo->SensorGrabStartX = OV8858_VIDEO_X_START;
            pSensorInfo->SensorGrabStartY = OV8858_VIDEO_Y_START;
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	     	pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 30;//0,4,14,32,40
	    	pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockRisingCount= 0;

            pSensorInfo->SensorGrabStartX = OV8858_FULL_X_START;	
            pSensorInfo->SensorGrabStartY = OV8858_FULL_Y_START;	

            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 30;
            pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
        default:
			pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockRisingCount= 0;

            pSensorInfo->SensorGrabStartX = OV8858_PV_X_START;
            pSensorInfo->SensorGrabStartY = OV8858_PV_Y_START;
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	     	pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 30;
	    	pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
    }
	
//Gionee yanggy 2013-09-03 add for module compability begin
#ifdef GN_MTK_BSP_LENS_COMPABILITY
    pSensorInfo->Sensor_Lens_id = OV8858_MIPI_Module_Lens_id;
    pSensorInfo->Sensor_Module_id = OV8858_MIPI_Module_id;
#endif
//Gionee yanggy 2013-09-03 add for module compability end

    memcpy(pSensorConfigData, &OV8858SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    OV8858DB("OV8858GetInfo exit!!\n");

    return ERROR_NONE;
}   /* OV8858GetInfo() */



UINT32 OV8858Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
		spin_lock(&ov8858mipiraw_drv_lock);
		OV8858CurrentScenarioId = ScenarioId;
		spin_unlock(&ov8858mipiraw_drv_lock);
		
		OV8858DB("[OV8858Control]OV8858CurrentScenarioId=%d\n",OV8858CurrentScenarioId);

	switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            OV8858Preview(pImageWindow, pSensorConfigData);
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			OV8858Video(pImageWindow, pSensorConfigData);
			break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
            OV8858Capture(pImageWindow, pSensorConfigData);
            break;

        default:
            return ERROR_INVALID_SCENARIO_ID;

    }
    return ERROR_NONE;
} /* OV8858Control() */


UINT32 OV8858FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
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
            *pFeatureReturnPara16++= OV8858_IMAGE_SENSOR_FULL_WIDTH;
            *pFeatureReturnPara16= OV8858_IMAGE_SENSOR_FULL_HEIGHT;
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_PERIOD:
				*pFeatureReturnPara16++= OV8858_FeatureControl_PERIOD_PixelNum;
				*pFeatureReturnPara16= OV8858_FeatureControl_PERIOD_LineNum;
				*pFeatureParaLen=4;
				break;
        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			switch(OV8858CurrentScenarioId)
			{
				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
					*pFeatureReturnPara32 = OV8858_PREVIEW_PCLK;
					*pFeatureParaLen=4;
					break;
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					*pFeatureReturnPara32 = OV8858_VIDEO_PCLK;
					*pFeatureParaLen=4;
					break;
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
				case MSDK_SCENARIO_ID_CAMERA_ZSD:
					*pFeatureReturnPara32 = OV8858_CAPTURE_PCLK;
					*pFeatureParaLen=4;
					break;
				default:
					*pFeatureReturnPara32 = OV8858_PREVIEW_PCLK;
					*pFeatureParaLen=4;
					break;
			}
		    break;

        case SENSOR_FEATURE_SET_ESHUTTER:
            OV8858_SetShutter(*pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_NIGHTMODE:
            OV8858_NightMode((BOOL) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_GAIN:  
           	OV8858_SetGain((UINT16) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_FLASHLIGHT:
            break;
        case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
            //OV8858_isp_master_clock=*pFeatureData32;
            break;
        case SENSOR_FEATURE_SET_REGISTER:
            OV8858_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
            break;
        case SENSOR_FEATURE_GET_REGISTER:
            pSensorRegData->RegData = OV8858_read_cmos_sensor(pSensorRegData->RegAddr);
            break;
        case SENSOR_FEATURE_SET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            for (i=0;i<SensorRegNumber;i++)
            {
            	spin_lock(&ov8858mipiraw_drv_lock);
                OV8858SensorCCT[i].Addr=*pFeatureData32++;
                OV8858SensorCCT[i].Para=*pFeatureData32++;
				spin_unlock(&ov8858mipiraw_drv_lock);
            }
            break;
        case SENSOR_FEATURE_GET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=OV8858SensorCCT[i].Addr;
                *pFeatureData32++=OV8858SensorCCT[i].Para;
            }
            break;
        case SENSOR_FEATURE_SET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            for (i=0;i<SensorRegNumber;i++)
            {
            	spin_lock(&ov8858mipiraw_drv_lock);
                OV8858SensorReg[i].Addr=*pFeatureData32++;
                OV8858SensorReg[i].Para=*pFeatureData32++;
				spin_unlock(&ov8858mipiraw_drv_lock);
            }
            break;
        case SENSOR_FEATURE_GET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=OV8858SensorReg[i].Addr;
                *pFeatureData32++=OV8858SensorReg[i].Para;
            }
            break;
        case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
            if (*pFeatureParaLen>=sizeof(NVRAM_SENSOR_DATA_STRUCT))
            {
                pSensorDefaultData->Version=NVRAM_CAMERA_SENSOR_FILE_VERSION;
                pSensorDefaultData->SensorId=OV8858_SENSOR_ID;
                memcpy(pSensorDefaultData->SensorEngReg, OV8858SensorReg, sizeof(SENSOR_REG_STRUCT)*ENGINEER_END);
                memcpy(pSensorDefaultData->SensorCCTReg, OV8858SensorCCT, sizeof(SENSOR_REG_STRUCT)*FACTORY_END_ADDR);
            }
            else
                return FALSE;
            *pFeatureParaLen=sizeof(NVRAM_SENSOR_DATA_STRUCT);
            break;
        case SENSOR_FEATURE_GET_CONFIG_PARA:
            memcpy(pSensorConfigData, &OV8858SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
            *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
            break;
        case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
            OV8858_camera_para_to_sensor();
            break;

        case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
            OV8858_sensor_to_camera_para();
            break;
        case SENSOR_FEATURE_GET_GROUP_COUNT:
            *pFeatureReturnPara32++=OV8858_get_sensor_group_count();
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_GROUP_INFO:
            OV8858_get_sensor_group_info(pSensorGroupInfo->GroupIdx, pSensorGroupInfo->GroupNamePtr, &pSensorGroupInfo->ItemCount);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_GROUP_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_ITEM_INFO:
            OV8858_get_sensor_item_info(pSensorItemInfo->GroupIdx,pSensorItemInfo->ItemIdx, pSensorItemInfo);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_SET_ITEM_INFO:
            OV8858_set_sensor_item_info(pSensorItemInfo->GroupIdx, pSensorItemInfo->ItemIdx, pSensorItemInfo->ItemValue);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_GET_ENG_INFO:
			//TODO~
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
			OV8858SetVideoMode(*pFeatureData16);
            break;
        case SENSOR_FEATURE_CHECK_SENSOR_ID:
            OV8858GetSensorID(pFeatureReturnPara32);
            break;
        case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
			//TODO~
			OV8858SetAutoFlickerMode((BOOL)*pFeatureData16, *(pFeatureData16+1));
	        break;
		case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
			OV8858MIPISetMaxFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, *(pFeatureData32+1));
			break;
		case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
			OV8858MIPIGetDefaultFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, (MUINT32 *)(*(pFeatureData32+1)));
			break;
		case SENSOR_FEATURE_SET_TEST_PATTERN:
			//TODO~
			OV8858SetTestPatternMode((BOOL)*pFeatureData16);
			break;
		case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE://for factory mode auto testing 			
			*pFeatureReturnPara32=OV8858_TEST_PATTERN_CHECKSUM; 		  
			*pFeatureParaLen=4; 							
		     break;
        default:
            break;
    }
    return ERROR_NONE;
}	


SENSOR_FUNCTION_STRUCT	SensorFuncOV8858=
{
    OV8858Open,
    OV8858GetInfo,
    OV8858GetResolution,
    OV8858FeatureControl,
    OV8858Control,
    OV8858Close
};

UINT32 OV8858_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    if (pfFunc!=NULL)
        *pfFunc=&SensorFuncOV8858;

    return ERROR_NONE;
}  


