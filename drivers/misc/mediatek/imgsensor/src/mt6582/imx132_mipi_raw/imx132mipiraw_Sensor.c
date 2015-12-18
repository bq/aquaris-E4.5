/*****************************************************************************
 *
 * Filename:
 * ---------
 *   Sensor.c
 *
 * Project:
 * --------
 *   DUMA
 *
 * Description:
 * ------------
 *   Image sensor driver function
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
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

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "imx132mipiraw_Sensor.h"
#include "imx132mipiraw_Camera_Sensor_para.h"
#include "imx132mipiraw_CameraCustomized.h"

#ifdef IMX132MIPI_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif
//#define ACDK
extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);

//#define IMX132MIPI_PREVIEW_CLK 138666667  //65000000
//#define IMX132MIPI_CAPTURE_CLK 134333333  //117000000  //69333333
//#define IMX132MIPI_DEBUG_SETTING
#if 0
#define IMX132MIPI_PREVIEW_CLK 108000000  //65000000
#define IMX132MIPI_CAPTURE_CLK 108000000  //117000000  //69333333
#define IMX132MIPI_ZSD_PRE_CLK 108000000 //117000000
#define IMX132MIPI_VIDEO_CLK   108000000 //117000000
#else
#define IMX132MIPI_PREVIEW_CLK 81600000  //65000000
#define IMX132MIPI_CAPTURE_CLK 81600000  //117000000  //69333333
#define IMX132MIPI_ZSD_PRE_CLK 81600000 //117000000
#define IMX132MIPI_VIDEO_CLK   81600000 //117000000
#endif
//#define IMX132MIPI_ZSD_PRE_CLK 134333333 //117000000

MSDK_SCENARIO_ID_ENUM IMX132MIPIMIPIRAWCurrentScenarioId = MSDK_SCENARIO_ID_CAMERA_PREVIEW;//ACDK_SCENARIO_ID_CAMERA_PREVIEW;

kal_uint32 IMX132MIPI_FeatureControl_PERIOD_PixelNum=IMX132MIPI_PV_PERIOD_PIXEL_NUMS;
kal_uint32 IMX132MIPI_FeatureControl_PERIOD_LineNum=IMX132MIPI_PV_PERIOD_LINE_NUMS;

static IMX132MIPI_sensor_struct IMX132MIPI_sensor =
{
  .eng =
  {
    .reg = CAMERA_SENSOR_REG_DEFAULT_VALUE,
    .cct = CAMERA_SENSOR_CCT_DEFAULT_VALUE,
  },
  .eng_info =
  {
    .SensorId = 0x132,
    .SensorType = CMOS_SENSOR,
    .SensorOutputDataFormat = IMX132MIPI_COLOR_FORMAT,
  },
  .shutter = 0x20,  
  .gain = 0x20,
  .pv_pclk = IMX132MIPI_PREVIEW_CLK,
  .cap_pclk = IMX132MIPI_CAPTURE_CLK,
  .video_pclk = IMX132MIPI_VIDEO_CLK,
  .frame_length = IMX132MIPI_PV_PERIOD_LINE_NUMS,
  .line_length = IMX132MIPI_PV_PERIOD_PIXEL_NUMS,
  .is_zsd = KAL_FALSE, //for zsd
  .dummy_pixels = 0,
  .dummy_lines = 0,  //for zsd
  .is_autofliker = KAL_FALSE,
  .sensorMode = IMX132MIPI_SENSOR_MODE_INIT,
};

static DEFINE_SPINLOCK(imx132mipi_drv_lock);
#if 0
kal_uint16 IMX132MIPI_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
    char puSendCmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,1,IMX132MIPI_sensor.write_id);
#ifdef IMX132MIPI_DRIVER_TRACE
	//SENSORDB("IMX132MIPI_read_cmos_sensor, addr:%x;get_byte:%x \n",addr,get_byte);
#endif		
    return get_byte;
}


kal_uint16 IMX132MIPI_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
    //kal_uint16 reg_tmp;
	
    char puSendCmd[3] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para & 0xFF)};
	
	iWriteRegI2C(puSendCmd , 3,IMX132MIPI_sensor.write_id);

	//SENSORDB("IMX132MIPI_write_cmos_sensor, addr:%x;get_byte:%x \n",addr,para);

	//for(i=0;i<0x100;i++)
	//	{
			
	//	}
	
	//reg_tmp = IMX132MIPI_read_cmos_sensor(addr);

	//SENSORDB("IMX132MIPI_read_cmos_sensor, addr:%x;get_byte:%x \n",addr,reg_tmp);
	return 0;
}
#endif
#if 1
#define IMX132MIPI_MaxGainIndex (74+3)																				 // Gain Index
kal_uint16 IMX132MIPI_sensorGainMapping[IMX132MIPI_MaxGainIndex][2] = {
    { 64,  0}, { 66, 8}, { 68, 15}, { 69, 20}, { 70, 24}, {73, 32}, { 76, 41}, { 78, 47}, { 80, 52}, { 83, 59},
    {87, 68}, { 89, 72}, {92,77}, {96,85}, {100,92}, {105,100}, {110,107}, {115,114}, {119,118}, {123,123},
    {126,126}, {128,128}, {130,130}, {132,132}, {136,136}, {140,144}, {144,142}, {148,145}, {150,147}, {156,151},
    {162,155}, {169,159}, {176,163}, {180,165}, {184,167}, {188,169}, {191,170}, {195,172}, {197,173}, {202,175}, 
    {205,176}, {210,178}, {215,180}, {221,182}, {227,184}, {231,185}, {237,187}, {241,189}, {252,191}, {260,193},    
	{270,195}, {282,198}, {292,200}, {309,203}, {315,204}, {328,206}, {334,207}, {348,209}, {364,211}, 


	//{381,213}, 
	//{399,215}, {420,217}, {443,219}, {468,221}, {482,222}, 
		//64*6=384
		//{381,213},	{399,215}, {420,217}, //64*6  
		{(384-1),213},	{(400-1),215}, {(416-1),217}, {(432-1),218},//64*6 
		
		//64*7=448
		//{443,219},	{468,221}, {482,222}, //64*7
		{(448-1),219},	{(464-1),221}, {(480-1),222}, {(496-1),223},//64*7



	//{(512-1),224}, {546,225}, {585,228}, {630,230}, {683,232},
	{(512-1),224}, {(528-1),225},{(544-1),226}, {585,228}, {630,230}, {683,232},
	{745,234}, {819,236}, {910,238}, {1024,240}};


#else

#define IMX132MIPI_MaxGainIndex 74																				 // Gain Index
kal_uint16 IMX132MIPI_sensorGainMapping[IMX132MIPI_MaxGainIndex][2] = {
	//64*1=64
    { 64,  0}, { 66, 8}, { 68, 15}, { 69, 20}, { 70, 24}, {73, 32}, { 76, 41}, { 78, 47}, 
    { 80, 52}, { 83, 59},{87, 68}, { 89, 72}, {92,77}, {96,85}, {100,92}, {105,100}, 
    {110,107}, {115,114}, {119,118}, {123,123},    {126,126}, 

	//64*2=128
	{128,128}, {130,130}, {132,132}, {136,136}, {140,144}, {144,142}, {148,145}, {150,147}, {156,151},
    {162,155}, {169,159}, {176,163}, {180,165}, {184,167}, {188,169}, 

	//64*3=192
	{191,170}, {195,172}, {197,173}, {202,175}, {205,176}, {210,178}, {215,180}, {221,182}, {227,184}, {231,185}, {237,187}, {241,189}, 
		
	
	//64*4=256
	{252,191}, {260,193}, {270,195}, {282,198}, {292,200}, {309,203}, {315,204}, 
		
		
	//64*5=320
	{328,206}, {334,207}, {348,209}, {364,211},
	
	//64*6=384
	//{381,213}, 	{399,215}, {420,217}, //64*6  
	{384,213}, 	{400,215}, {416,217}, {432,218},//64*6 
	
	//64*7=448
	//{443,219}, 	{468,221}, {482,222}, //64*7
	{448,219},	{464,221}, {480,222}, {496,223},//64*7

	//64*8=512
	{512,224}, 	{546,226}, 

	
	//64*9=576
	{585,228}, 
	//64*10=640
	{630,230},
	//64*11=640
	{683,232},//64*5
	{745,234}, {819,236}, {910,238}, {1024,240}};//64*5
#endif

kal_uint8 IMX132MIPI_read_cmos_sensor(kal_uint16 addr)
{
	kal_uint8 get_byte=0;
    char puSendCmd[2] = {(char)((addr&0xFF00) >> 8) , (char)(addr & 0xFF) };
	iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,1,IMX132MIPI_sensor.write_id);
    return get_byte;
}

void IMX132MIPI_write_cmos_sensor(kal_uint16 addr, kal_uint8 para)
{

	char puSendCmd[3] = {(char)((addr&0xFF00) >> 8) , (char)(addr & 0xFF) ,(char)(para & 0xFF)};
	
	iWriteRegI2C(puSendCmd , 3,IMX132MIPI_sensor.write_id);
}



#ifdef IMX132MIPI_USE_OTP

//index:index of otp group.(0,1,2)
//return:	0:group index is empty.
//		1.group index has invalid data
//		2.group index has valid data

kal_uint16 imx132mipi_check_otp_wb(kal_uint16 index)
{
	kal_uint16 temp,flag;
	kal_uint32 address;

    if(index < 2)
    	{
    		//select bank 0
    		IMX132MIPI_write_cmos_sensor(0x3d84,0xc0);
			//load otp to buffer
			IMX132MIPI_write_cmos_sensor(0x3d81,0x01);
			msleep(10);

			//disable otp read
			IMX132MIPI_write_cmos_sensor(0x3d81,0x00);

			//read flag
			address = 0x3d05+index*9;
			flag = IMX132MIPI_read_cmos_sensor(address);
    	}
	else
		{
			//select bank 1
    		IMX132MIPI_write_cmos_sensor(0x3d84,0xc1);
			//load otp to buffer
			IMX132MIPI_write_cmos_sensor(0x3d81,0x01);
			msleep(10);

			//disable otp read
			IMX132MIPI_write_cmos_sensor(0x3d81,0x00);

			//read flag
        address = 0x3d07;
			flag = IMX132MIPI_read_cmos_sensor(address);
		}
	//disable otp read
	//IMX132MIPI_write_cmos_sensor(0x3d81,0x00);

	if(NULL == flag)
		{
			
			SENSORDB("[imx132mipi_check_otp_wb]index[%x]read flag[%x][0]\n",index,flag);
			return 0;
			
		}
	else if(!(flag&0x80) && (flag&0x7f))
		{
			SENSORDB("[imx132mipi_check_otp_wb]index[%x]read flag[%x][2]\n",index,flag);
			return 2;
		}
	else
		{
			SENSORDB("[imx132mipi_check_otp_wb]index[%x]read flag[%x][1]\n",index,flag);
		    return 1;
		}
	
}

//index:index of otp group.(0,1,2)
//return:	0.group index is empty.
//		1.group index has invalid data
//		2.group index has valid data

kal_uint16 imx132mipi_check_otp_lenc(kal_uint16 index)
{
   kal_uint16 temp,flag,i;
   kal_uint32 address;

   //select bank :index*4+2
   IMX132MIPI_write_cmos_sensor(0x3d84,0xc2+index*4);

   //read otp
   IMX132MIPI_write_cmos_sensor(0x3d81,0x01);
   msleep(10);

   //disable otp read
   IMX132MIPI_write_cmos_sensor(0x3d81,0x00);

   //read flag
   address = 0x3d00; 
   flag = IMX132MIPI_read_cmos_sensor(address);
   flag = flag & 0xc0;

   //disable otp read
   IMX132MIPI_write_cmos_sensor(0x3d81,0x00);

   //clear otp buffer
   address = 0x3d00;
   for(i = 0;i<16; i++)
   	{
   		IMX132MIPI_write_cmos_sensor(address+i,0x00);
   	}

   if(NULL == flag)
   	{
   		SENSORDB("[imx132mipi_check_otp_lenc]index[%x]read flag[%x][0]\n",index,flag);
   	    return 0;
   	}
   else if(0x40 == flag)
   	{
   		SENSORDB("[imx132mipi_check_otp_lenc]index[%x]read flag[%x][2]\n",index,flag);
   	    return 2;
   	}
   else
   	{
   		SENSORDB("[imx132mipi_check_otp_lenc]index[%x]read flag[%x][1]\n",index,flag);
		return 1;
   	}
}

kal_uint16 imx132mipi_check_otp_blc(kal_uint16 index)
{
	kal_uint16 temp,flag;
	kal_uint32 address;
	
    	{
			
			//select bank 31: 0x1f
    		IMX132MIPI_write_cmos_sensor(0x3d84,0xc0+0x1f);
			//load otp to buffer
			IMX132MIPI_write_cmos_sensor(0x3d81,0x01);
			msleep(10);

			//disable otp read
			IMX132MIPI_write_cmos_sensor(0x3d81,0x00);

			//read flag
			address = 0x3d01;
			flag = IMX132MIPI_read_cmos_sensor(address);
    	}

	//disable otp read
	//IMX132MIPI_write_cmos_sensor(0x3d81,0x00);

	if(NULL == flag)
		{
			
			SENSORDB("[imx132mipi_check_otp_blc] read flag[%x]\n",index,flag);
			return 0;			
		}
	else
		{
			SENSORDB("[imx132mipi_check_otp_blc] read flag[%x]\n",index,flag);
			return 1;
		}	
}

//for otp_af
struct imx132mipi_otp_af_struct 
{
	kal_uint16 group1_data;
	kal_uint16 group1_far_h8;
	kal_uint16 group1_far_l8;
	kal_uint16 group1_near_h8;
	kal_uint16 group1_near_l8;

	kal_uint16 group2_data;
	kal_uint16 group2_far_h8;
	kal_uint16 group2_far_l8;
	kal_uint16 group2_near_h8;
	kal_uint16 group2_near_l8;

	kal_uint16 group3_data;
	kal_uint16 group3_far_h8;
	kal_uint16 group3_far_l8;
	kal_uint16 group3_near_h8;
	kal_uint16 group3_near_l8;
};

struct imx132mipi_otp_af_struct imx132mipi_read_otp_af(void)
{
	struct imx132mipi_otp_af_struct otp;
	kal_uint16 i;
	kal_uint32 address;

	//select bank 15
	IMX132MIPI_write_cmos_sensor(0x3d84,0xcf);
	//load otp to buffer
	IMX132MIPI_write_cmos_sensor(0x3d81,0x01);
	msleep(10);
				
	//disable otp read
	IMX132MIPI_write_cmos_sensor(0x3d81,0x00);

	otp.group1_data = IMX132MIPI_read_cmos_sensor(0x3d00);
	otp.group1_far_h8 = IMX132MIPI_read_cmos_sensor(0x3d01);
	otp.group1_far_l8 = IMX132MIPI_read_cmos_sensor(0x3d02);
	otp.group1_near_h8 = IMX132MIPI_read_cmos_sensor(0x3d03);
	otp.group1_near_l8 = IMX132MIPI_read_cmos_sensor(0x3d04);

	otp.group2_data = IMX132MIPI_read_cmos_sensor(0x3d05);
	otp.group2_far_h8 = IMX132MIPI_read_cmos_sensor(0x3d06);
	otp.group2_far_l8 = IMX132MIPI_read_cmos_sensor(0x3d07);
	otp.group2_near_h8 = IMX132MIPI_read_cmos_sensor(0x3d08);
	otp.group2_near_l8 = IMX132MIPI_read_cmos_sensor(0x3d09);

	otp.group3_data = IMX132MIPI_read_cmos_sensor(0x3d0a);
	otp.group3_far_h8 = IMX132MIPI_read_cmos_sensor(0x3d0b);
	otp.group3_far_l8 = IMX132MIPI_read_cmos_sensor(0x3d0c);
	otp.group3_near_h8 = IMX132MIPI_read_cmos_sensor(0x3d0d);
	otp.group3_near_l8 = IMX132MIPI_read_cmos_sensor(0x3d0e);


	SENSORDB("[imx132mipi_read_otp_af]address[%x]group1_data[%x]\n",0x3d00,otp.group1_data);
	SENSORDB("[imx132mipi_read_otp_af]address[%x]group1_far_h8[%x]\n",0x3d01,otp.group1_far_h8);
	SENSORDB("[imx132mipi_read_otp_af]address[%x]group1_far_l8[%x]\n",0x3d02,otp.group1_far_l8);
	SENSORDB("[imx132mipi_read_otp_af]address[%x]group1_near_h8[%x]\n",0x3d03,otp.group1_near_h8);
	SENSORDB("[imx132mipi_read_otp_af]address[%x]group1_near_l8[%x]\n",0x3d04,otp.group1_near_l8);
	SENSORDB("[imx132mipi_read_otp_af]address[%x]group2_data[%x]\n",0x3d05,otp.group2_data);
	SENSORDB("[imx132mipi_read_otp_af]address[%x]group2_far_h8[%x]\n",0x3d06,otp.group2_far_h8);	
	SENSORDB("[imx132mipi_read_otp_af]address[%x]group2_far_l8[%x]\n",0x3d07,otp.group2_far_l8);
	SENSORDB("[imx132mipi_read_otp_af]address[%x]group2_near_h8[%x]\n",0x3d08,otp.group2_near_h8);
	SENSORDB("[imx132mipi_read_otp_af]address[%x]group2_near_h8[%x]\n",0x3d09,otp.group2_near_l8);
	SENSORDB("[imx132mipi_read_otp_af]address[%x]group2_near_h8[%x]\n",0x3d0a,otp.group3_data);
	SENSORDB("[imx132mipi_read_otp_af]address[%x]group2_near_h8[%x]\n",0x3d0b,otp.group3_far_h8);
	SENSORDB("[imx132mipi_read_otp_af]address[%x]group2_near_h8[%x]\n",0x3d0c,otp.group3_far_l8);
	SENSORDB("[imx132mipi_read_otp_af]address[%x]group2_near_h8[%x]\n",0x3d0d,otp.group3_near_h8);
	SENSORDB("[imx132mipi_read_otp_af]address[%x]group2_near_h8[%x]\n",0x3d0e,otp.group3_near_l8);
	//disable otp read
	//IMX132MIPI_write_cmos_sensor(0x3d81,0x00);
	
	//clear otp buffer
	address = 0x3d00;
	for(i =0; i<32; i++)
		{
			IMX132MIPI_write_cmos_sensor(0x3d00,0x00);
		}

	return otp;
}
//end otp_af

//index:index of otp group.(0,1,2)
//return: 0
kal_uint16 imx132mipi_read_otp_wb(kal_uint16 index, struct imx132mipi_otp_struct *otp)
{
	kal_uint16 temp,i;
	kal_uint32 address;

	switch(index)
		{
			case 0:
				
				//select bank 0
				IMX132MIPI_write_cmos_sensor(0x3d84,0xc0);
				//load otp to buffer
				IMX132MIPI_write_cmos_sensor(0x3d81,0x01);
				msleep(10);
				
				//disable otp read
				IMX132MIPI_write_cmos_sensor(0x3d81,0x00);

                address = 0x3d05;
                otp->module_integrator_id = (IMX132MIPI_read_cmos_sensor(address)&0x7f);
				otp->lens_id = IMX132MIPI_read_cmos_sensor(address+1);
				otp->rg_ratio = IMX132MIPI_read_cmos_sensor(address+2);
				otp->bg_ratio = IMX132MIPI_read_cmos_sensor(address+3);
				otp->user_data[0] = IMX132MIPI_read_cmos_sensor(address+4);
				otp->user_data[1] = IMX132MIPI_read_cmos_sensor(address+5);
				otp->user_data[2] = IMX132MIPI_read_cmos_sensor(address+6);
				otp->user_data[3] = IMX132MIPI_read_cmos_sensor(address+7);
				otp->user_data[4] = IMX132MIPI_read_cmos_sensor(address+8);
				break;
			case 1:
				//select bank 0
				IMX132MIPI_write_cmos_sensor(0x3d84,0xc0);
				//load otp to buffer
				IMX132MIPI_write_cmos_sensor(0x3d81,0x01);
				msleep(10);
				
				//disable otp read
                IMX132MIPI_write_cmos_sensor(0x3d81,0x00);

                address = 0x3d0e;
                otp->module_integrator_id = (IMX132MIPI_read_cmos_sensor(address)&0x7f);
                otp->lens_id = IMX132MIPI_read_cmos_sensor(address+1);

				//select bank 1
				IMX132MIPI_write_cmos_sensor(0x3d84,0xc1);
				//load otp to buffer
				IMX132MIPI_write_cmos_sensor(0x3d81,0x01);
				msleep(10);
				
				//disable otp read
				IMX132MIPI_write_cmos_sensor(0x3d81,0x00);

				address = 0x3d00;
				otp->rg_ratio = IMX132MIPI_read_cmos_sensor(address);
				otp->bg_ratio = IMX132MIPI_read_cmos_sensor(address+1);
				otp->user_data[0] = IMX132MIPI_read_cmos_sensor(address+2);
				otp->user_data[1] = IMX132MIPI_read_cmos_sensor(address+3);
				otp->user_data[2] = IMX132MIPI_read_cmos_sensor(address+4);
				otp->user_data[3] = IMX132MIPI_read_cmos_sensor(address+5);
				otp->user_data[4] = IMX132MIPI_read_cmos_sensor(address+6);
				break;
			case 2:
				//select bank 1
				IMX132MIPI_write_cmos_sensor(0x3d84,0xc1);
				//load otp to buffer
				IMX132MIPI_write_cmos_sensor(0x3d81,0x01);
				msleep(10);
				
                //disable otp read
                IMX132MIPI_write_cmos_sensor(0x3d81,0x00);

                address = 0x3d07;
                otp->module_integrator_id = (IMX132MIPI_read_cmos_sensor(address)&0x7f);
                otp->lens_id = IMX132MIPI_read_cmos_sensor(address+1);
                otp->rg_ratio = IMX132MIPI_read_cmos_sensor(address+2);
				otp->bg_ratio = IMX132MIPI_read_cmos_sensor(address+3);
				otp->user_data[0] = IMX132MIPI_read_cmos_sensor(address+4);
				otp->user_data[1] = IMX132MIPI_read_cmos_sensor(address+5);
				otp->user_data[2] = IMX132MIPI_read_cmos_sensor(address+6);
				otp->user_data[3] = IMX132MIPI_read_cmos_sensor(address+7);
				otp->user_data[4] = IMX132MIPI_read_cmos_sensor(address+8);
				break;
			default:
				break;
				
		}

	SENSORDB("[imx132mipi_read_otp_wb]address[%x]module_integrator_id[%x]\n",address,otp->module_integrator_id);
	SENSORDB("[imx132mipi_read_otp_wb]address[%x]lens_id[%x]\n",address,otp->lens_id);
	SENSORDB("[imx132mipi_read_otp_wb]address[%x]rg_ratio[%x]\n",address,otp->rg_ratio);
	SENSORDB("[imx132mipi_read_otp_wb]address[%x]bg_ratio[%x]\n",address,otp->bg_ratio);
	SENSORDB("[imx132mipi_read_otp_wb]address[%x]user_data[0][%x]\n",address,otp->user_data[0]);
	SENSORDB("[imx132mipi_read_otp_wb]address[%x]user_data[1][%x]\n",address,otp->user_data[1]);
	SENSORDB("[imx132mipi_read_otp_wb]address[%x]user_data[2][%x]\n",address,otp->user_data[2]);	
	SENSORDB("[imx132mipi_read_otp_wb]address[%x]user_data[3][%x]\n",address,otp->user_data[3]);
	SENSORDB("[imx132mipi_read_otp_wb]address[%x]user_data[4][%x]\n",address,otp->user_data[4]);

	//disable otp read
	//IMX132MIPI_write_cmos_sensor(0x3d81,0x00);

        //clear otp buffer
        address = 0x3d00;
        for(i =0; i<16; i++)
        {
            IMX132MIPI_write_cmos_sensor(address+i, 0x00);
        }

        return 0;
	
}

kal_uint16 imx132mipi_read_otp_lenc(kal_uint16 index,struct imx132mipi_otp_struct *otp)
{
	kal_uint16 bank,temp,i;
	kal_uint32 address;

    //select bank: index*4 +2;
    bank = index*4+2;
	temp = 0xc0|bank;
	IMX132MIPI_write_cmos_sensor(0x3d84,temp);

	//read otp
	IMX132MIPI_write_cmos_sensor(0x3d81,0x01);
	msleep(10);

	//disabe otp read
	IMX132MIPI_write_cmos_sensor(0x3d81,0x00);

	
	address = 0x3d01;
	for(i = 0; i < 15; i++)
		{
			otp->lenc[i] = IMX132MIPI_read_cmos_sensor(address);
			address++;
		}

	//select bank+1
    bank++;
	temp = 0xc0|bank;
	IMX132MIPI_write_cmos_sensor(0x3d84,temp);

	//read otp
	IMX132MIPI_write_cmos_sensor(0x3d81,0x01);
	msleep(10);

	//disabe otp read
	IMX132MIPI_write_cmos_sensor(0x3d81,0x00);


        address = 0x3d00;
        for(i = 15; i < 31; i++)
        {
                otp->lenc[i] = IMX132MIPI_read_cmos_sensor(address);
                address++;
        }

	//select bank+2
    bank++;
	temp = 0xc0|bank;
	IMX132MIPI_write_cmos_sensor(0x3d84,temp);

	//read otp
	IMX132MIPI_write_cmos_sensor(0x3d81,0x01);
	msleep(10);

	//disabe otp read
        IMX132MIPI_write_cmos_sensor(0x3d81,0x00);


        address = 0x3d00;
        for(i = 31; i < 47; i++)
        {
                otp->lenc[i] = IMX132MIPI_read_cmos_sensor(address);
                address++;
        }

	//select bank+3
    bank++;
	temp = 0xc0|bank;
	IMX132MIPI_write_cmos_sensor(0x3d84,temp);

	//read otp
	IMX132MIPI_write_cmos_sensor(0x3d81,0x01);
	msleep(10);

	//disabe otp read
	IMX132MIPI_write_cmos_sensor(0x3d81,0x00);


        address = 0x3d00;
        for(i = 47; i < 62; i++)
        {
            otp->lenc[i] = IMX132MIPI_read_cmos_sensor(address);
            address++;
        }

	//disable otp read
	//IMX132MIPI_write_cmos_sensor(0x3d81,0x00);

	
	//clear otp buffer
		address = 0x3d00;
		for(i =0; i<32; i++)
			{
				IMX132MIPI_write_cmos_sensor(0x3d00,0x00);
			}
	return 0;
}

kal_uint16 imx132mipi_read_otp_blc(kal_uint16 index, struct imx132mipi_otp_struct *otp)
{
	kal_uint16 temp,i;
	kal_uint32 address;


		{

				
				//select bank 0
				IMX132MIPI_write_cmos_sensor(0x3d84,0xc0);
				//load otp to buffer
				IMX132MIPI_write_cmos_sensor(0x3d81,0x01);
				msleep(10);
				
				//disable otp read
				IMX132MIPI_write_cmos_sensor(0x3d81,0x00);

                address = 0x3d0a;
             //   otp->module_integrator_id = (IMX132MIPI_read_cmos_sensor(address)&0x7f);
			//	otp->lens_id = IMX132MIPI_read_cmos_sensor(address+1);
			//	otp->rg_ratio = IMX132MIPI_read_cmos_sensor(address+2);
			//	otp->bg_ratio = IMX132MIPI_read_cmos_sensor(address+3);
				otp->user_data[0] = IMX132MIPI_read_cmos_sensor(address);
			//	otp->user_data[1] = IMX132MIPI_read_cmos_sensor(address+5);
			//	otp->user_data[2] = IMX132MIPI_read_cmos_sensor(address+6);
			//	otp->user_data[3] = IMX132MIPI_read_cmos_sensor(address+7);
			//	otp->user_data[4] = IMX132MIPI_read_cmos_sensor(address+8);
				
		}

	/*SENSORDB("[imx132mipi_read_otp_wb]address[%x]module_integrator_id[%x]\n",address,otp->module_integrator_id);
	SENSORDB("[imx132mipi_read_otp_wb]address[%x]lens_id[%x]\n",address,otp->lens_id);
	SENSORDB("[imx132mipi_read_otp_wb]address[%x]rg_ratio[%x]\n",address,otp->rg_ratio);
	SENSORDB("[imx132mipi_read_otp_wb]address[%x]bg_ratio[%x]\n",address,otp->bg_ratio);*/
	SENSORDB("[imx132mipi_read_otp_wb]address[%x]user_data[0][%x]\n",address,otp->user_data[0]);
	/*SENSORDB("[imx132mipi_read_otp_wb]address[%x]user_data[1][%x]\n",address,otp->user_data[1]);
	SENSORDB("[imx132mipi_read_otp_wb]address[%x]user_data[2][%x]\n",address,otp->user_data[2]);	
	SENSORDB("[imx132mipi_read_otp_wb]address[%x]user_data[3][%x]\n",address,otp->user_data[3]);
	SENSORDB("[imx132mipi_read_otp_wb]address[%x]user_data[4][%x]\n",address,otp->user_data[4]);*/

	//disable otp read
	//IMX132MIPI_write_cmos_sensor(0x3d81,0x00);

        return 0;
	
}


//R_gain: red gain of sensor AWB, 0x400 = 1
//G_gain: green gain of sensor AWB, 0x400 = 1
//B_gain: blue gain of sensor AWB, 0x400 = 1
//reutrn 0
kal_uint16 imx132mipi_update_wb_gain(kal_uint32 R_gain, kal_uint32 G_gain, kal_uint32 B_gain)
{

    SENSORDB("[imx132mipi_update_wb_gain]R_gain[%x]G_gain[%x]B_gain[%x]\n",R_gain,G_gain,B_gain);

	if(R_gain > 0x400)
		{
			IMX132MIPI_write_cmos_sensor(0x5186,R_gain >> 8);
			IMX132MIPI_write_cmos_sensor(0x5187,(R_gain&0x00ff));
		}
	if(G_gain > 0x400)
		{
			IMX132MIPI_write_cmos_sensor(0x5188,G_gain >> 8);
			IMX132MIPI_write_cmos_sensor(0x5189,(G_gain&0x00ff));
		}
	if(B_gain >0x400)
		{
			IMX132MIPI_write_cmos_sensor(0x518a,B_gain >> 8);
			IMX132MIPI_write_cmos_sensor(0x518b,(B_gain&0x00ff));
		}
	return 0;
}


//return 0
kal_uint16 imx132mipi_update_lenc(struct imx132mipi_otp_struct *otp)
{
        kal_uint16 i, temp;
        //lenc g
        for(i = 0; i < 62; i++)
        {
                IMX132MIPI_write_cmos_sensor(0x5800+i,otp->lenc[i]);

                SENSORDB("[imx132mipi_update_lenc]otp->lenc[%d][%x]\n",i,otp->lenc[i]);
        }

        return 0;
}

kal_uint16 imx132mipi_update_blc(struct imx132mipi_otp_struct *otp)
{
        kal_uint16 i, temp;
        IMX132MIPI_write_cmos_sensor(0x4006,otp->user_data[0]);

        SENSORDB("[imx132mipi_update_Blc]otp->blc[%d][%x]\n",i,otp->user_data[0]);
        return 0;
}


//R/G and B/G ratio of typical camera module is defined here

kal_uint32 RG_Ratio_typical = RG_TYPICAL;
kal_uint32 BG_Ratio_typical = BG_TYPICAL;

//call this function after OV5650 initialization
//return value:	0 update success
//				1 no 	OTP

kal_uint16 imx132mipi_update_wb_register_from_otp(void)
{
	kal_uint16 temp, i, otp_index;
	struct imx132mipi_otp_struct current_otp;
	kal_uint32 R_gain, B_gain, G_gain, G_gain_R,G_gain_B;

	SENSORDB("imx132mipi_update_wb_register_from_otp\n");


	//check first wb OTP with valid OTP
	for(i = 0; i < 3; i++)
		{
			temp = imx132mipi_check_otp_wb(i);
			if(temp == 2)
				{
					otp_index = i;
					break;
				}
		}
	if( 3 == i)
		{
		 	SENSORDB("[imx132mipi_update_wb_register_from_otp]no valid wb OTP data!\r\n");
			return 1;
		}
	imx132mipi_read_otp_wb(otp_index,&current_otp);

	//calculate gain
	//0x400 = 1x gain
	if(current_otp.bg_ratio < BG_Ratio_typical)
		{
			if(current_otp.rg_ratio < RG_Ratio_typical)
				{
					//current_opt.bg_ratio < BG_Ratio_typical &&
					//cuttent_otp.rg < RG_Ratio_typical

					G_gain = 0x400;
					B_gain = 0x400 * BG_Ratio_typical / current_otp.bg_ratio;
					R_gain = 0x400 * RG_Ratio_typical / current_otp.rg_ratio;
				}
			else
				{
					//current_otp.bg_ratio < BG_Ratio_typical &&
			        //current_otp.rg_ratio >= RG_Ratio_typical
			        R_gain = 0x400;
					G_gain = 0x400 * current_otp.rg_ratio / RG_Ratio_typical;
					B_gain = G_gain * BG_Ratio_typical / current_otp.bg_ratio;
					
			        
				}
		}
	else
		{
			if(current_otp.rg_ratio < RG_Ratio_typical)
				{
					//current_otp.bg_ratio >= BG_Ratio_typical &&
			        //current_otp.rg_ratio < RG_Ratio_typical
			        B_gain = 0x400;
					G_gain = 0x400 * current_otp.bg_ratio / BG_Ratio_typical;
					R_gain = G_gain * RG_Ratio_typical / current_otp.rg_ratio;
					
				}
			else
				{
					//current_otp.bg_ratio >= BG_Ratio_typical &&
			        //current_otp.rg_ratio >= RG_Ratio_typical
			        G_gain_B = 0x400*current_otp.bg_ratio / BG_Ratio_typical;
				    G_gain_R = 0x400*current_otp.rg_ratio / RG_Ratio_typical;
					
					if(G_gain_B > G_gain_R)
						{
							B_gain = 0x400;
							G_gain = G_gain_B;
							R_gain = G_gain * RG_Ratio_typical / current_otp.rg_ratio;
						}
					else

						{
							R_gain = 0x400;
							G_gain = G_gain_R;
							B_gain = G_gain * BG_Ratio_typical / current_otp.bg_ratio;
						}
			        
				}
			
		}
	//write sensor wb gain to register
	imx132mipi_update_wb_gain(R_gain,G_gain,B_gain);

	//success
	return 0;
}

//call this function after ov5650 initialization
//return value:	0 update success
//				1 no otp

kal_uint16 imx132mipi_update_lenc_register_from_otp(void)
{
	kal_uint16 temp,i,otp_index;
    struct imx132mipi_otp_struct current_otp;

	for(i = 0; i < 3; i++)
		{
			temp = imx132mipi_check_otp_lenc(i);
			if(2 == temp)
				{
					otp_index = i;
					break;
				}
		}
	if(3 == i)
		{
		 	SENSORDB("[imx132mipi_update_lenc_register_from_otp]no valid wb OTP data!\r\n");
			return 1;
		}
	imx132mipi_read_otp_lenc(otp_index,&current_otp);

	imx132mipi_update_lenc(&current_otp);

	//success
	return 0;
}

kal_uint16 imx132mipi_update_blc_register_from_otp(void)
{
	kal_uint16 temp,i,otp_index;
    struct imx132mipi_otp_struct current_otp;

	temp = imx132mipi_check_otp_blc(i);

	if(temp == 0)//not update
	{
	 	SENSORDB("[imx132mipi_update_blc_register_from_otp]no valid blc OTP data!\r\n");
		return 1;
	}
	else//update
	{
		imx132mipi_read_otp_blc(otp_index,&current_otp);
		imx132mipi_update_blc(&current_otp);
	 	SENSORDB("[imx132mipi_update_blc_register_from_otp] valid blc OTP data!\r\n");
		return 0;
	}
}

#endif

static void IMX132MIPI_Set_Dummy(const kal_uint16 iPixels, const kal_uint16 iLines)
{
	kal_uint16 line_length, frame_length;
	unsigned long flags;
//	return;
#ifdef IMX132MIPI_DRIVER_TRACE
	SENSORDB("IMX132MIPI_Set_Dummy:iPixels:%x; iLines:%x \n",iPixels,iLines);
#endif
//return;
	if ( IMX132MIPI_SENSOR_MODE_PREVIEW == IMX132MIPI_sensor.sensorMode )	//SXGA size output
	{
		line_length = IMX132MIPI_PV_PERIOD_PIXEL_NUMS + iPixels;
		frame_length = IMX132MIPI_PV_PERIOD_LINE_NUMS + iLines;
	}
	else if( IMX132MIPI_SENSOR_MODE_VIDEO == IMX132MIPI_sensor.sensorMode )
	{
		line_length = IMX132MIPI_VIDEO_PERIOD_PIXEL_NUMS + iPixels;
		frame_length = IMX132MIPI_VIDEO_PERIOD_LINE_NUMS + iLines;
	}
	else if( IMX132MIPI_SENSOR_MODE_CAPTURE == IMX132MIPI_sensor.sensorMode )
	{
		line_length = IMX132MIPI_FULL_PERIOD_PIXEL_NUMS + iPixels;
		frame_length = IMX132MIPI_FULL_PERIOD_LINE_NUMS + iLines;
	}
	else
	{
		line_length = IMX132MIPI_PV_PERIOD_PIXEL_NUMS + iPixels;
		frame_length = IMX132MIPI_PV_PERIOD_LINE_NUMS + iLines;
	}

	if ((line_length >= 0x1FFF)||(frame_length >= 0xFFF)) {
		SENSORDB("[soso][IMX132MIPI_Set_Dummy] Error: line_length=%d, frame_length = %d \n",line_length, frame_length);
	}

	if(frame_length < (IMX132MIPI_sensor.shutter+5))
	{
		frame_length = IMX132MIPI_sensor.shutter +5;
	}
	
#ifdef IMX132MIPI_DRIVER_TRACE
	SENSORDB("line_length:%x; frame_length:%x \n",line_length,frame_length);
#endif

//	if ((line_length >= 0x1FFF)||(frame_length >= 0xFFF))
//	{
//#ifdef IMX132MIPI_DRIVER_TRACE
//		SENSORDB("Warnning: line length or frame height is overflow!!!!!!!!  \n");
//#endif
//		return ;
//	}


//	if((line_length == IMX132MIPI_sensor.line_length)&&(frame_length == IMX132MIPI_sensor.frame_length))
//		return ;
	spin_lock_irqsave(&imx132mipi_drv_lock,flags);

	IMX132MIPI_sensor.line_length = line_length;
	IMX132MIPI_sensor.frame_length = frame_length;
	//IMX132MIPI_sensor.dummy_pixels = iPixels;
	//IMX132MIPI_sensor.dummy_lines = iLines;
	spin_unlock_irqrestore(&imx132mipi_drv_lock,flags);

	SENSORDB("line_length:%x; frame_length:%x \n",line_length,frame_length);
	
    /*  Add dummy pixels: */
    /* 0x0342 [0:4], 0x0343 defines the PCLKs in one line of IMX132MIPI  */  
    /* Add dummy lines:*/
    /* 0x0340 [0:1], 0x0341 defines total lines in one frame of IMX132MIPI */
    IMX132MIPI_write_cmos_sensor(0x0342, line_length >> 8);
    IMX132MIPI_write_cmos_sensor(0x0343, line_length & 0xFF);
    IMX132MIPI_write_cmos_sensor(0x0340, frame_length >> 8);
    IMX132MIPI_write_cmos_sensor(0x0341, frame_length & 0xFF);
	
}   /*  IMX132MIPI_Set_Dummy    */


static UINT32 IMX132MIPISetMaxFrameRate(UINT16 u2FrameRate)
{
	kal_int16 dummy_line=0;
	kal_uint16 frame_length = IMX132MIPI_sensor.frame_length;
	unsigned long flags;
		
	SENSORDB("[soso][IMX132MIPISetMaxFrameRate]u2FrameRate=%d \n",u2FrameRate);

	frame_length= (10*IMX132MIPI_sensor.pclk) / u2FrameRate / IMX132MIPI_sensor.line_length;
	if(KAL_FALSE == IMX132MIPI_sensor.pv_mode){
		if(frame_length < IMX132MIPI_FULL_PERIOD_LINE_NUMS)
			frame_length = IMX132MIPI_FULL_PERIOD_LINE_NUMS;		
	}

		spin_lock_irqsave(&imx132mipi_drv_lock,flags);
		IMX132MIPI_sensor.frame_length = frame_length;
		spin_unlock_irqrestore(&imx132mipi_drv_lock,flags);
	
		if((IMX132MIPIMIPIRAWCurrentScenarioId == MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG)||(IMX132MIPIMIPIRAWCurrentScenarioId == MSDK_SCENARIO_ID_CAMERA_ZSD)){
			dummy_line = frame_length - IMX132MIPI_FULL_PERIOD_LINE_NUMS;
		}
		else if(IMX132MIPIMIPIRAWCurrentScenarioId==MSDK_SCENARIO_ID_CAMERA_PREVIEW){
			dummy_line = frame_length - IMX132MIPI_PV_PERIOD_LINE_NUMS;
		}
		else if(IMX132MIPIMIPIRAWCurrentScenarioId==MSDK_SCENARIO_ID_VIDEO_PREVIEW) {
			dummy_line = frame_length - IMX132MIPI_VIDEO_PERIOD_LINE_NUMS;
		}
		else
			dummy_line = frame_length - IMX132MIPI_PV_PERIOD_LINE_NUMS;
		SENSORDB("[soso][IMX132MIPISetMaxFrameRate]frame_length = %d, dummy_line=%d \n",IMX132MIPI_sensor.frame_length,dummy_line);
		if(dummy_line<0) {
			dummy_line = 0;
		}
	    /* to fix VSYNC, to fix frame rate */
		IMX132MIPI_Set_Dummy(0, dummy_line); /* modify dummy_pixel must gen AE table again */
	//}
	return (UINT32)u2FrameRate;
}


void IMX132MIPI_Write_Shutter(kal_uint16 ishutter)
{

kal_uint16 extra_shutter = 0;
kal_uint16 frame_length = 0;
kal_uint16 realtime_fp = 0;
kal_uint32 pclk = 0;
unsigned long flags;

//return;
#ifdef IMX132MIPI_DRIVER_TRACE
	SENSORDB("IMX132MIPI_write_shutter:%x \n",ishutter);
#endif
   


			SENSORDB("IMX132MIPI_sensor.is_autofliker:%x \n",IMX132MIPI_sensor.is_autofliker);
#if 0
	if(IMX132MIPI_sensor.is_autofliker == KAL_TRUE)
		{
		  		if(IMX132MIPI_sensor.is_zsd == KAL_TRUE)
		  			{
		  				pclk = IMX132MIPI_ZSD_PRE_CLK;
		  			}
				 else
				 	{
				 		pclk = IMX132MIPI_PREVIEW_CLK;
				 	}
					
				realtime_fp = pclk *10 / (IMX132MIPI_sensor.line_length * IMX132MIPI_sensor.frame_length);
			    SENSORDB("[IMX132MIPI_Write_Shutter]pv_clk:%d\n",pclk);
				SENSORDB("[IMX132MIPI_Write_Shutter]line_length:%d\n",IMX132MIPI_sensor.line_length);
				SENSORDB("[IMX132MIPI_Write_Shutter]frame_length:%d\n",IMX132MIPI_sensor.frame_length);
			    SENSORDB("[IMX132MIPI_Write_Shutter]framerate(10base):%d\n",realtime_fp);

				if((realtime_fp >= 297)&&(realtime_fp <= 303))
					{
						realtime_fp = 296;
						spin_lock_irqsave(&imx132mipi_drv_lock,flags);
						IMX132MIPI_sensor.frame_length = pclk *10 / (IMX132MIPI_sensor.line_length * realtime_fp);
						spin_unlock_irqrestore(&imx132mipi_drv_lock,flags);

						SENSORDB("[autofliker realtime_fp=30,extern heights slowdown to 29.6fps][height:%d]",IMX132MIPI_sensor.frame_length);
					}
				else if((realtime_fp >= 147)&&(realtime_fp <= 153))
					{
						realtime_fp = 146;
						spin_lock_irqsave(&imx132mipi_drv_lock,flags);
						IMX132MIPI_sensor.frame_length = pclk *10 / (IMX132MIPI_sensor.line_length * realtime_fp);
						spin_unlock_irqrestore(&imx132mipi_drv_lock,flags);
						
						SENSORDB("[autofliker realtime_fp=15,extern heights slowdown to 14.6fps][height:%d]",IMX132MIPI_sensor.frame_length);
					}
				//IMX132MIPI_sensor.frame_length = IMX132MIPI_sensor.frame_length +(IMX132MIPI_sensor.frame_length>>7);

		}
#endif
#if 1
	if(IMX132MIPI_sensor.is_autofliker == KAL_TRUE)
	{
		if(IMX132MIPI_sensor.video_mode == KAL_FALSE)
			{
				realtime_fp = IMX132MIPI_sensor.pclk *10 / (IMX132MIPI_sensor.line_length * IMX132MIPI_sensor.frame_length);
				SENSORDB("[IMX132MIPI_Write_Shutter]pv_clk:%d\n",pclk);
				SENSORDB("[IMX132MIPI_Write_Shutter]line_length:%d\n",IMX132MIPI_sensor.line_length);
				SENSORDB("[IMX132MIPI_Write_Shutter]frame_length:%d\n",IMX132MIPI_sensor.frame_length);
				SENSORDB("[IMX132MIPI_Write_Shutter]framerate(10base):%d\n",realtime_fp);
				
				if((realtime_fp >= 297)&&(realtime_fp <= 303))
				{
					realtime_fp = 305;
					IMX132MIPISetMaxFrameRate((UINT16)realtime_fp);
				}
				else if((realtime_fp >= 147)&&(realtime_fp <= 153))
				{
					realtime_fp = 146;
					IMX132MIPISetMaxFrameRate((UINT16)realtime_fp);
				}		
			}
	}
#endif
   	if (!ishutter) ishutter = 1; /* avoid 0 */

	spin_lock_irqsave(&imx132mipi_drv_lock,flags);
	frame_length = IMX132MIPI_sensor.max_exposure_lines;
	spin_unlock_irqrestore(&imx132mipi_drv_lock,flags);

	if(ishutter > (frame_length -5))
	{
		extra_shutter = ishutter - frame_length + 5;
		SENSORDB("[shutter > frame_length] extra_shutter:%x \n", extra_shutter);
	}
	else
	{
		extra_shutter = 0;
	}
	spin_lock_irqsave(&imx132mipi_drv_lock,flags);
	IMX132MIPI_sensor.frame_length = frame_length+extra_shutter;
	spin_unlock_irqrestore(&imx132mipi_drv_lock,flags);
	
	SENSORDB("IMX132MIPI_sensor.frame_length:%x\n",IMX132MIPI_sensor.frame_length);

    IMX132MIPI_write_cmos_sensor(0x0340, (IMX132MIPI_sensor.frame_length>>8)&0xFF);
	IMX132MIPI_write_cmos_sensor(0x0341, (IMX132MIPI_sensor.frame_length)&0xFF);

	IMX132MIPI_write_cmos_sensor(0x0104, 1);  
    IMX132MIPI_write_cmos_sensor(0x202, (ishutter >> 8) & 0xFF);
	IMX132MIPI_write_cmos_sensor(0x203, (ishutter >> 0) & 0xFF);	
	IMX132MIPI_write_cmos_sensor(0x0104, 0);  

}




/*************************************************************************
* FUNCTION
*	IMX132MIPI_SetShutter
*
* DESCRIPTION
*	This function set e-shutter of IMX132MIPI to change exposure time.
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


void set_IMX132MIPI_shutter(kal_uint16 iShutter)
{

	unsigned long flags;
	
#ifdef IMX132MIPI_DRIVER_TRACE
	SENSORDB("set_IMX132MIPI_shutter:%x \n",iShutter);
#endif

    if((IMX132MIPI_sensor.pv_mode == KAL_FALSE)&&(IMX132MIPI_sensor.is_zsd == KAL_FALSE))
    	{
    	   SENSORDB("[set_IMX132MIPI_shutter]now is in 1/4size cap mode\n");
    	   //return;
    	}
	else if((IMX132MIPI_sensor.is_zsd == KAL_TRUE)&&(IMX132MIPI_sensor.is_zsd_cap == KAL_TRUE))
		{
			SENSORDB("[set_IMX132MIPI_shutter]now is in zsd cap mode\n");

			//SENSORDB("[set_IMX132MIPI_shutter]0x3500:%x\n",IMX132MIPI_read_cmos_sensor(0x3500));
			//SENSORDB("[set_IMX132MIPI_shutter]0x3500:%x\n",IMX132MIPI_read_cmos_sensor(0x3501));
			//SENSORDB("[set_IMX132MIPI_shutter]0x3500:%x\n",IMX132MIPI_read_cmos_sensor(0x3502));
			//return;
		}

/*	if(IMX132MIPI_sensor.shutter == iShutter)
		{
			SENSORDB("[set_IMX132MIPI_shutter]shutter is the same with previous, skip\n");
			return;
		}*/

	spin_lock_irqsave(&imx132mipi_drv_lock,flags);
	IMX132MIPI_sensor.shutter = iShutter;
	spin_unlock_irqrestore(&imx132mipi_drv_lock,flags);
	
    IMX132MIPI_Write_Shutter(iShutter);

}   /*  Set_IMX132MIPI_Shutter */

static kal_uint8 IMX132MIPIGain2Reg(const kal_uint16 iGain)
{
//  256/(256-temp_reg) = igain/64  =>   temp_reg = 256*(iGain-64)/iGain
   #if 0
   kal_uint8 iI;
	
	
    for (iI = 0; iI < (IMX132MIPI_MaxGainIndex-1); iI++) {
        if(iGain <= IMX132MIPI_sensorGainMapping[iI][0]){
            break;
        }
    }
   
    if(iGain != IMX132MIPI_sensorGainMapping[iI][0])
    {
         printk("[IMX132MIPIGain2Reg] Gain mapping don't correctly:%d %d \n", iGain, IMX132MIPI_sensorGainMapping[iI][0]);
    }
	
    return IMX132MIPI_sensorGainMapping[iI][1];
   #else
	kal_uint32 temp_reg;
	temp_reg = ((2*256*((kal_uint32)iGain-64))/((kal_uint32)iGain)+1)/2;
#ifdef IMX132MIPI_DRIVER_TRACE
		   SENSORDB("IMX132MIPIGain2Reg:%x;\n",(temp_reg) );
#endif

	return (temp_reg&0xff) ; 
	#endif
}


kal_uint16 IMX132MIPI_SetGain(kal_uint16 iGain)
{
   kal_uint16 iReg;
   unsigned long flags;
  // 	  return 0;
#ifdef IMX132MIPI_DRIVER_TRACE
   SENSORDB("IMX132MIPI_SetGain:%x;\n",iGain);
#endif
/*   if(IMX132MIPI_sensor.gain == iGain)
   	{
   		SENSORDB("[IMX132MIPI_SetGain]:gain is the same with previous,skip\n");
	 	return 0;
   	}*/

	spin_lock_irqsave(&imx132mipi_drv_lock,flags);
	IMX132MIPI_sensor.gain = iGain;
	spin_unlock_irqrestore(&imx132mipi_drv_lock,flags);

	iReg = IMX132MIPIGain2Reg(iGain);

	//imx123 max gain = 8X  
	IMX132MIPI_write_cmos_sensor(0x0104, 1);  
	IMX132MIPI_write_cmos_sensor(0x205, iReg&0xFF);//only use 0x350b for gain control
	IMX132MIPI_write_cmos_sensor(0x0104, 0);  

	  return iGain;
}



/*************************************************************************
* FUNCTION
*	IMX132MIPI_SetGain
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

#if 0
void IMX132MIPI_set_isp_driving_current(kal_uint16 current)
{
#ifdef IMX132MIPI_DRIVER_TRACE
   SENSORDB("IMX132MIPI_set_isp_driving_current:current:%x;\n",current);
#endif
  //iowrite32((0x2 << 12)|(0<<28)|(0x8880888), 0xF0001500);
}
#endif

/*************************************************************************
* FUNCTION
*	IMX132MIPI_NightMode
*
* DESCRIPTION
*	This function night mode of IMX132MIPI.
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
void IMX132MIPI_night_mode(kal_bool enable)
{
}   /*  IMX132MIPI_NightMode    */


/* write camera_para to sensor register */
static void IMX132MIPI_camera_para_to_sensor(void)
{
  kal_uint32 i;
#ifdef IMX132MIPI_DRIVER_TRACE
	 SENSORDB("IMX132MIPI_camera_para_to_sensor\n");
#endif
  for (i = 0; 0xFFFFFFFF != IMX132MIPI_sensor.eng.reg[i].Addr; i++)
  {
    IMX132MIPI_write_cmos_sensor(IMX132MIPI_sensor.eng.reg[i].Addr, IMX132MIPI_sensor.eng.reg[i].Para);
  }
  for (i = IMX132MIPI_FACTORY_START_ADDR; 0xFFFFFFFF != IMX132MIPI_sensor.eng.reg[i].Addr; i++)
  {
    IMX132MIPI_write_cmos_sensor(IMX132MIPI_sensor.eng.reg[i].Addr, IMX132MIPI_sensor.eng.reg[i].Para);
  }
  IMX132MIPI_SetGain(IMX132MIPI_sensor.gain); /* update gain */
}

/* update camera_para from sensor register */
static void IMX132MIPI_sensor_to_camera_para(void)
{
  kal_uint32 i;
  kal_uint32 temp_data;
  
#ifdef IMX132MIPI_DRIVER_TRACE
   SENSORDB("IMX132MIPI_sensor_to_camera_para\n");
#endif
  for (i = 0; 0xFFFFFFFF != IMX132MIPI_sensor.eng.reg[i].Addr; i++)
  {
  	temp_data = IMX132MIPI_read_cmos_sensor(IMX132MIPI_sensor.eng.reg[i].Addr);

	spin_lock(&imx132mipi_drv_lock);
    IMX132MIPI_sensor.eng.reg[i].Para = temp_data;
	spin_unlock(&imx132mipi_drv_lock);
	
  }
  for (i = IMX132MIPI_FACTORY_START_ADDR; 0xFFFFFFFF != IMX132MIPI_sensor.eng.reg[i].Addr; i++)
  {
  	temp_data = IMX132MIPI_read_cmos_sensor(IMX132MIPI_sensor.eng.reg[i].Addr);
	
	spin_lock(&imx132mipi_drv_lock);
    IMX132MIPI_sensor.eng.reg[i].Para = temp_data;
	spin_unlock(&imx132mipi_drv_lock);
  }
}

/* ------------------------ Engineer mode ------------------------ */
inline static void IMX132MIPI_get_sensor_group_count(kal_int32 *sensor_count_ptr)
{
#ifdef IMX132MIPI_DRIVER_TRACE
   SENSORDB("IMX132MIPI_get_sensor_group_count\n");
#endif
  *sensor_count_ptr = IMX132MIPI_GROUP_TOTAL_NUMS;
}

inline static void IMX132MIPI_get_sensor_group_info(MSDK_SENSOR_GROUP_INFO_STRUCT *para)
{
#ifdef IMX132MIPI_DRIVER_TRACE
   SENSORDB("IMX132MIPI_get_sensor_group_info\n");
#endif
  switch (para->GroupIdx)
  {
  case IMX132MIPI_PRE_GAIN:
    sprintf(para->GroupNamePtr, "CCT");
    para->ItemCount = 5;
    break;
  case IMX132MIPI_CMMCLK_CURRENT:
    sprintf(para->GroupNamePtr, "CMMCLK Current");
    para->ItemCount = 1;
    break;
  case IMX132MIPI_FRAME_RATE_LIMITATION:
    sprintf(para->GroupNamePtr, "Frame Rate Limitation");
    para->ItemCount = 2;
    break;
  case IMX132MIPI_REGISTER_EDITOR:
    sprintf(para->GroupNamePtr, "Register Editor");
    para->ItemCount = 2;
    break;
  default:
    ASSERT(0);
  }
}

inline static void IMX132MIPI_get_sensor_item_info(MSDK_SENSOR_ITEM_INFO_STRUCT *para)
{

  const static kal_char *cct_item_name[] = {"SENSOR_BASEGAIN", "Pregain-R", "Pregain-Gr", "Pregain-Gb", "Pregain-B"};
  const static kal_char *editer_item_name[] = {"REG addr", "REG value"};
  
#ifdef IMX132MIPI_DRIVER_TRACE
	 SENSORDB("IMX132MIPI_get_sensor_item_info\n");
#endif
  switch (para->GroupIdx)
  {
  case IMX132MIPI_PRE_GAIN:
    switch (para->ItemIdx)
    {
    case IMX132MIPI_SENSOR_BASEGAIN:
    case IMX132MIPI_PRE_GAIN_R_INDEX:
    case IMX132MIPI_PRE_GAIN_Gr_INDEX:
    case IMX132MIPI_PRE_GAIN_Gb_INDEX:
    case IMX132MIPI_PRE_GAIN_B_INDEX:
      break;
    default:
      ASSERT(0);
    }
    sprintf(para->ItemNamePtr, cct_item_name[para->ItemIdx - IMX132MIPI_SENSOR_BASEGAIN]);
    para->ItemValue = IMX132MIPI_sensor.eng.cct[para->ItemIdx].Para * 1000 / BASEGAIN;
    para->IsTrueFalse = para->IsReadOnly = para->IsNeedRestart = KAL_FALSE;
    para->Min = IMX132MIPI_MIN_ANALOG_GAIN * 1000;
    para->Max = IMX132MIPI_MAX_ANALOG_GAIN * 1000;
    break;
  case IMX132MIPI_CMMCLK_CURRENT:
    switch (para->ItemIdx)
    {
    case 0:
      sprintf(para->ItemNamePtr, "Drv Cur[2,4,6,8]mA");
      switch (IMX132MIPI_sensor.eng.reg[IMX132MIPI_CMMCLK_CURRENT_INDEX].Para)
      {
      case ISP_DRIVING_2MA:
        para->ItemValue = 2;
        break;
      case ISP_DRIVING_4MA:
        para->ItemValue = 4;
        break;
      case ISP_DRIVING_6MA:
        para->ItemValue = 6;
        break;
      case ISP_DRIVING_8MA:
        para->ItemValue = 8;
        break;
      default:
        ASSERT(0);
      }
      para->IsTrueFalse = para->IsReadOnly = KAL_FALSE;
      para->IsNeedRestart = KAL_TRUE;
      para->Min = 2;
      para->Max = 8;
      break;
    default:
      ASSERT(0);
    }
    break;
  case IMX132MIPI_FRAME_RATE_LIMITATION:
    switch (para->ItemIdx)
    {
    case 0:
      sprintf(para->ItemNamePtr, "Max Exposure Lines");
      para->ItemValue = 5998;
      break;
    case 1:
      sprintf(para->ItemNamePtr, "Min Frame Rate");
      para->ItemValue = 5;
      break;
    default:
      ASSERT(0);
    }
    para->IsTrueFalse = para->IsNeedRestart = KAL_FALSE;
    para->IsReadOnly = KAL_TRUE;
    para->Min = para->Max = 0;
    break;
  case IMX132MIPI_REGISTER_EDITOR:
    switch (para->ItemIdx)
    {
    case 0:
    case 1:
      sprintf(para->ItemNamePtr, editer_item_name[para->ItemIdx]);
      para->ItemValue = 0;
      para->IsTrueFalse = para->IsReadOnly = para->IsNeedRestart = KAL_FALSE;
      para->Min = 0;
      para->Max = (para->ItemIdx == 0 ? 0xFFFF : 0xFF);
      break;
    default:
      ASSERT(0);
    }
    break;
  default:
    ASSERT(0);
  }
}

inline static kal_bool IMX132MIPI_set_sensor_item_info(MSDK_SENSOR_ITEM_INFO_STRUCT *para)
{
  kal_uint16 temp_para;
#ifdef IMX132MIPI_DRIVER_TRACE
   SENSORDB("IMX132MIPI_set_sensor_item_info\n");
#endif
  switch (para->GroupIdx)
  {
  case IMX132MIPI_PRE_GAIN:
    switch (para->ItemIdx)
    {
    case IMX132MIPI_SENSOR_BASEGAIN:
    case IMX132MIPI_PRE_GAIN_R_INDEX:
    case IMX132MIPI_PRE_GAIN_Gr_INDEX:
    case IMX132MIPI_PRE_GAIN_Gb_INDEX:
    case IMX132MIPI_PRE_GAIN_B_INDEX:
	  spin_lock(&imx132mipi_drv_lock);
      IMX132MIPI_sensor.eng.cct[para->ItemIdx].Para = para->ItemValue * BASEGAIN / 1000;
	  spin_unlock(&imx132mipi_drv_lock);
	  
      IMX132MIPI_SetGain(IMX132MIPI_sensor.gain); /* update gain */
      break;
    default:
      ASSERT(0);
    }
    break;
  case IMX132MIPI_CMMCLK_CURRENT:
    switch (para->ItemIdx)
    {
    case 0:
      switch (para->ItemValue)
      {
      case 2:
        temp_para = ISP_DRIVING_2MA;
        break;
      case 3:
      case 4:
        temp_para = ISP_DRIVING_4MA;
        break;
      case 5:
      case 6:
        temp_para = ISP_DRIVING_6MA;
        break;
      default:
        temp_para = ISP_DRIVING_8MA;
        break;
      }
      //IMX132MIPI_set_isp_driving_current((kal_uint16)temp_para);
	  spin_lock(&imx132mipi_drv_lock);
      IMX132MIPI_sensor.eng.reg[IMX132MIPI_CMMCLK_CURRENT_INDEX].Para = temp_para;
	  spin_unlock(&imx132mipi_drv_lock);
      break;
    default:
      ASSERT(0);
    }
    break;
  case IMX132MIPI_FRAME_RATE_LIMITATION:
    ASSERT(0);
    break;
  case IMX132MIPI_REGISTER_EDITOR:
    switch (para->ItemIdx)
    {
      static kal_uint32 fac_sensor_reg;
    case 0:
      if (para->ItemValue < 0 || para->ItemValue > 0xFFFF) return KAL_FALSE;
      fac_sensor_reg = para->ItemValue;
      break;
    case 1:
      if (para->ItemValue < 0 || para->ItemValue > 0xFF) return KAL_FALSE;
      IMX132MIPI_write_cmos_sensor(fac_sensor_reg, para->ItemValue);
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




void IMX132MIPI_globle_setting(void)
{
	//IMX132MIPI_Global_setting
	//Base_on_IMX132MIPI_APP_R1.11
	//2012_2_1
	// 
	//;;;;;;;;;;;;;Any modify please inform to OV FAE;;;;;;;;;;;;;;;

//#ifdef IMX132MIPI_DEBUG_SETTING
	//@@IMX132_init
	
	SENSORDB("IMX132MIPI_globle_setting  start \n");
	//@@IMX132_init_15fps
	//IMX132MIPI_write_cmos_sensor
	#if 0
	
#else
	//IMX132MIPI_write_cmos_sensor


	////////////////Address	value
	IMX132MIPI_write_cmos_sensor(0x3087, 0x53);
	IMX132MIPI_write_cmos_sensor(0x308B, 0x5A);
	IMX132MIPI_write_cmos_sensor(0x3094, 0x11);
	IMX132MIPI_write_cmos_sensor(0x309D, 0xA4);
	IMX132MIPI_write_cmos_sensor(0x30AA, 0x01);
	IMX132MIPI_write_cmos_sensor(0x30C6, 0x00);
	IMX132MIPI_write_cmos_sensor(0x30C7, 0x00);
	IMX132MIPI_write_cmos_sensor(0x3118, 0x2F);
	IMX132MIPI_write_cmos_sensor(0x312A, 0x00);
	IMX132MIPI_write_cmos_sensor(0x312B, 0x0B);
	IMX132MIPI_write_cmos_sensor(0x312C, 0x0B);
	IMX132MIPI_write_cmos_sensor(0x312D, 0x13);
	//IMX132MIPI_write_cmos_sensor(      ,     );
	//IMX132MIPI_write_cmos_sensor(      ,     );
	//IMX132MIPI_write_cmos_sensor(	     ,     );
	//IMX132MIPI_write_cmos_sensor(//////, ////);////////////Address	value
	IMX132MIPI_write_cmos_sensor(0x0305, 0x02);
	//IMX132MIPI_write_cmos_sensor(0x0307, 0x2D);//108M 30.03fps 
	//IMX132MIPI_write_cmos_sensor(0x0307, 0x2E);//110.4M 30.69fps 
	IMX132MIPI_write_cmos_sensor(0x0307, 0x44);//110.4M 30.69fps 
	IMX132MIPI_write_cmos_sensor(0x30A4, 0x02);
	IMX132MIPI_write_cmos_sensor(0x303C, 0x4B);
	//IMX132MIPI_write_cmos_sensor(	     ,     );
	//IMX132MIPI_write_cmos_sensor(//////, ////);////////////////////Address	value
	IMX132MIPI_write_cmos_sensor(0x0340, 0x04);
	IMX132MIPI_write_cmos_sensor(0x0341, 0xA8);//frame length 1600
	//IMX132MIPI_write_cmos_sensor(0x0341, 0x18);//frame length 1560 30.8fps
	IMX132MIPI_write_cmos_sensor(0x0342, 0x08);
	IMX132MIPI_write_cmos_sensor(0x0343, 0xC8);
	#if 0//1920X1080
	IMX132MIPI_write_cmos_sensor(0x0344, 0x00);//startx
	IMX132MIPI_write_cmos_sensor(0x0345, 0x1C);//startx
	IMX132MIPI_write_cmos_sensor(0x0346, 0x00);//starty
	IMX132MIPI_write_cmos_sensor(0x0347, 0x3C);//starty
	IMX132MIPI_write_cmos_sensor(0x0348, 0x07);//endx
	IMX132MIPI_write_cmos_sensor(0x0349, 0x9B);//endx
	IMX132MIPI_write_cmos_sensor(0x034A, 0x04);//endy
	IMX132MIPI_write_cmos_sensor(0x034B, 0x73);//endy
	IMX132MIPI_write_cmos_sensor(0x034C, 0x07);//outx
	IMX132MIPI_write_cmos_sensor(0x034D, 0x80);//outx
	IMX132MIPI_write_cmos_sensor(0x034E, 0x04);//outy
	IMX132MIPI_write_cmos_sensor(0x034F, 0x38);//outy
	#else
	IMX132MIPI_write_cmos_sensor(0x0344, 0x00);//startx
	IMX132MIPI_write_cmos_sensor(0x0345, 0x1c-0xc);//startx 1c -> 0x10
	IMX132MIPI_write_cmos_sensor(0x0346, 0x00);//starty
	IMX132MIPI_write_cmos_sensor(0x0347, 0x3c-0x0c);//starty 3c -> 30
	IMX132MIPI_write_cmos_sensor(0x0348, 0x07);//endx //0x79b -> 0x7ab
	IMX132MIPI_write_cmos_sensor(0x0349, 0x9B+0x10);//endx
	IMX132MIPI_write_cmos_sensor(0x034A, 0x04);//endy 473-> 483
	IMX132MIPI_write_cmos_sensor(0x034B, 0x73+0x10);//endy
	IMX132MIPI_write_cmos_sensor(0x034C, 0x07);//outx 0x780 -> 0x79c
	IMX132MIPI_write_cmos_sensor(0x034D, 0x80+0x1c);//outx
	IMX132MIPI_write_cmos_sensor(0x034E, 0x04);//outy 438 -> 454 
	IMX132MIPI_write_cmos_sensor(0x034F, 0x38+0x1c);//outy

	#endif
	IMX132MIPI_write_cmos_sensor(0x0381, 0x01);
	IMX132MIPI_write_cmos_sensor(0x0383, 0x01);
	IMX132MIPI_write_cmos_sensor(0x0385, 0x01);
	IMX132MIPI_write_cmos_sensor(0x0387, 0x01);
	IMX132MIPI_write_cmos_sensor(0x303D, 0x10);
	IMX132MIPI_write_cmos_sensor(0x303E, 0x4A);
	IMX132MIPI_write_cmos_sensor(0x3040, 0x08);
	IMX132MIPI_write_cmos_sensor(0x3041, 0x97);
	IMX132MIPI_write_cmos_sensor(0x3048, 0x00);
	IMX132MIPI_write_cmos_sensor(0x304C, 0x2F);
	IMX132MIPI_write_cmos_sensor(0x304D, 0x02);
	IMX132MIPI_write_cmos_sensor(0x3064, 0x92);
	IMX132MIPI_write_cmos_sensor(0x306A, 0x10);
	IMX132MIPI_write_cmos_sensor(0x309B, 0x00);
	IMX132MIPI_write_cmos_sensor(0x309E, 0x41);
	IMX132MIPI_write_cmos_sensor(0x30A0, 0x10);
	IMX132MIPI_write_cmos_sensor(0x30A1, 0x0B);
	IMX132MIPI_write_cmos_sensor(0x30B2, 0x00);
	IMX132MIPI_write_cmos_sensor(0x30D5, 0x00);
	IMX132MIPI_write_cmos_sensor(0x30D6, 0x00);
	IMX132MIPI_write_cmos_sensor(0x30D7, 0x00);
	IMX132MIPI_write_cmos_sensor(0x30D8, 0x00);
	IMX132MIPI_write_cmos_sensor(0x30D9, 0x00);
	IMX132MIPI_write_cmos_sensor(0x30DA, 0x00);
	IMX132MIPI_write_cmos_sensor(0x30DB, 0x00);
	IMX132MIPI_write_cmos_sensor(0x30DC, 0x00);
	IMX132MIPI_write_cmos_sensor(0x30DD, 0x00);
	IMX132MIPI_write_cmos_sensor(0x30DE, 0x00);
	IMX132MIPI_write_cmos_sensor(0x3102, 0x0C);
	IMX132MIPI_write_cmos_sensor(0x3103, 0x33);
	IMX132MIPI_write_cmos_sensor(0x3104, 0x18);
	IMX132MIPI_write_cmos_sensor(0x3105, 0x00);
	IMX132MIPI_write_cmos_sensor(0x3106, 0x65);
	IMX132MIPI_write_cmos_sensor(0x3107, 0x00);
	IMX132MIPI_write_cmos_sensor(0x3108, 0x06);
	IMX132MIPI_write_cmos_sensor(0x3109, 0x04);
	IMX132MIPI_write_cmos_sensor(0x310A, 0x04);
	IMX132MIPI_write_cmos_sensor(0x315C, 0x3D);
	IMX132MIPI_write_cmos_sensor(0x315D, 0x3C);
	IMX132MIPI_write_cmos_sensor(0x316E, 0x3E);
	IMX132MIPI_write_cmos_sensor(0x316F, 0x3D);
	IMX132MIPI_write_cmos_sensor(0x3301, 0x01);
	IMX132MIPI_write_cmos_sensor(0x3304, 0x07);
	IMX132MIPI_write_cmos_sensor(0x3305, 0x06);
	IMX132MIPI_write_cmos_sensor(0x3306, 0x19);
	IMX132MIPI_write_cmos_sensor(0x3307, 0x03);
	IMX132MIPI_write_cmos_sensor(0x3308, 0x0F);
	IMX132MIPI_write_cmos_sensor(0x3309, 0x07);
	IMX132MIPI_write_cmos_sensor(0x330A, 0x0C);
	IMX132MIPI_write_cmos_sensor(0x330B, 0x06);
	IMX132MIPI_write_cmos_sensor(0x330C, 0x0B);
	IMX132MIPI_write_cmos_sensor(0x330D, 0x07);
	IMX132MIPI_write_cmos_sensor(0x330E, 0x03);
	IMX132MIPI_write_cmos_sensor(0x3318, 0x64);
	IMX132MIPI_write_cmos_sensor(0x3322, 0x09);
	IMX132MIPI_write_cmos_sensor(0x3342, 0x00);
	IMX132MIPI_write_cmos_sensor(0x3348, 0xE0);
	////IMX132MIPI_write_cmos_sensor(      ,     );
	IMX132MIPI_write_cmos_sensor(0x0100, 0x01);
	IMX132MIPI_write_cmos_sensor(0x0101, 0x03); //image_orientation

#endif

	SENSORDB("IMX132MIPI_globle_setting  end \n");
                                                                   		
}

//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
IMX132MIPI_1632_1224_2Lane_30fps_Mclk26M_setting(void)
{
return;


	SENSORDB("IMX132MIPI_1632_1224_2Lane_30fps_Mclk26M_setting end \n");

}

IMX132MIPI_3264_2448_2Lane_15fps_Mclk26M_setting(void)
{
	
return;
//	;//IMX132MIPI_3264*2448_setting_2lanes_690Mbps/lane_15fps									   
//	;//Base_on_IMX132MIPI_APP_R1.11 															   
//	;//2012_2_1 																			   
//	;//Tony Li																				   
//	;;;;;;;;;;;;;Any modify please inform to OV FAE;;;;;;;;;;;;;;;	


	//@@Ov8835_3264x2448_2lane_15fps_143MSclk_693Mbps

	SENSORDB("IMX132MIPI_3264_2448_2Lane_15fps_Mclk26M_setting start \n");


	SENSORDB("IMX132MIPI_3264_2448_2Lane_15fps_Mclk26M_setting end \n");

	//SENSORDB("IMX132MIPI_3264_2448_2Lane_15fps_Mclk26M_setting end \n");
	
}



UINT32 IMX132MIPIOpen(void)
{
	kal_uint16 sensor_id=0; 

	//added by mandrave
	int i;
	const kal_uint16 sccb_writeid[] = {IMX132MIPI_SLAVE_WRITE_ID_1,IMX132MIPI_SLAVE_WRITE_ID_2};

	spin_lock(&imx132mipi_drv_lock);
	IMX132MIPI_sensor.is_zsd = KAL_FALSE;  //for zsd full size preview
	IMX132MIPI_sensor.is_zsd_cap = KAL_FALSE;
	IMX132MIPI_sensor.is_autofliker = KAL_FALSE; //for autofliker.
	IMX132MIPI_sensor.pv_mode = KAL_TRUE;
	IMX132MIPI_sensor.sensorMode = IMX132MIPI_SENSOR_MODE_INIT;
	IMX132MIPI_sensor.max_exposure_lines = IMX132MIPI_sensor.frame_length;
	spin_unlock(&imx132mipi_drv_lock);
   
   //Soft Reset
   IMX132MIPI_write_cmos_sensor(0x0103, 0x01);
   mdelay(10); 
   
  	for(i = 0; i <(sizeof(sccb_writeid)/sizeof(sccb_writeid[0])); i++)
  	{
  		spin_lock(&imx132mipi_drv_lock);
  	   IMX132MIPI_sensor.write_id = sccb_writeid[i];
	   IMX132MIPI_sensor.read_id = (sccb_writeid[i]|0x01);
	   spin_unlock(&imx132mipi_drv_lock);

	   
	   sensor_id = ((IMX132MIPI_read_cmos_sensor(0x0000) << 8) | IMX132MIPI_read_cmos_sensor(0x0001));
	   
#ifdef IMX132MIPI_DRIVER_TRACE
		SENSORDB("IMX132MIPIOpen, sensor_id:%x \n",sensor_id);
#endif
		if(IMX132MIPI_SENSOR_ID == sensor_id)
			{
				SENSORDB("IMX132MIPI slave write id:%x \n",IMX132MIPI_sensor.write_id);
				break;
			}
  	}
  
	// check if sensor ID correct		
	if (sensor_id != IMX132MIPI_SENSOR_ID) 
	{	
		//sensor_id = 0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}
#ifndef IMX132MIPI_4LANE
	IMX132MIPI_globle_setting();
#else
	IMX132MIPI_4LANE_globle_setting();
#endif
	
	spin_lock(&imx132mipi_drv_lock);
	IMX132MIPI_sensor.shutter = 0x200;//init shutter
	IMX132MIPI_sensor.gain = 0x20;//init gain
	spin_unlock(&imx132mipi_drv_lock);

	SENSORDB("test for bootimage \n");
 	
	return ERROR_NONE;
}   /* IMX132MIPIOpen  */

/*************************************************************************
* FUNCTION
*   OV5642GetSensorID
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
UINT32 IMX132MIPIGetSensorID(UINT32 *sensorID) 
{
  //added by mandrave
   int i;
   const kal_uint16 sccb_writeid[] = {IMX132MIPI_SLAVE_WRITE_ID_1,IMX132MIPI_SLAVE_WRITE_ID_2};
 
	//Soft Reset
	IMX132MIPI_write_cmos_sensor(0x0103, 0x01);
	mdelay(10); 

  for(i = 0; i <(sizeof(sccb_writeid)/sizeof(sccb_writeid[0])); i++)
  	{
  		spin_lock(&imx132mipi_drv_lock);
  	   IMX132MIPI_sensor.write_id = sccb_writeid[i];
	   IMX132MIPI_sensor.read_id = (sccb_writeid[i]|0x01);
	   spin_unlock(&imx132mipi_drv_lock);
	
	   	*sensorID = ((IMX132MIPI_read_cmos_sensor(0x0000) << 8) | IMX132MIPI_read_cmos_sensor(0x0001));
#ifdef IMX132MIPI_DRIVER_TRACE
		SENSORDB("IMX132MIPIOpen, sensor_id:%x \n",*sensorID);
#endif
		if(IMX132MIPI_SENSOR_ID == *sensorID)
			{
				SENSORDB("IMX132MIPI slave write id:%x \n",IMX132MIPI_sensor.write_id);
				break;
			}
  	}
  
	// check if sensor ID correct		
	if (*sensorID != IMX132MIPI_SENSOR_ID) 
		{	
			*sensorID = 0xFFFFFFFF;
			return ERROR_SENSOR_CONNECT_FAIL;
		}
	
   return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*	IMX132MIPIClose
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
UINT32 IMX132MIPIClose(void)
{
#ifdef IMX132MIPI_DRIVER_TRACE
   SENSORDB("IMX132MIPIClose\n");
#endif
  //CISModulePowerOn(FALSE);
//	DRV_I2CClose(IMX132MIPIhDrvI2C);
	return ERROR_NONE;
}   /* IMX132MIPIClose */

/*************************************************************************
* FUNCTION
* IMX132MIPIPreview
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
UINT32 IMX132MIPIPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	//kal_uint16 dummy_line;
	//kal_uint16 ret;
#ifdef IMX132MIPI_DRIVER_TRACE
	SENSORDB("IMX132MIPIPreview setting\n");
#endif
	//IMX132MIPI_Sensor_1M();
	//IMX132MIPI_globle_setting();

    spin_lock(&imx132mipi_drv_lock);
	IMX132MIPI_sensor.pv_mode = KAL_TRUE;
	IMX132MIPI_sensor.sensorMode = IMX132MIPI_SENSOR_MODE_PREVIEW;
	IMX132MIPI_sensor.pclk = IMX132MIPI_PREVIEW_CLK;
	spin_unlock(&imx132mipi_drv_lock);
	#ifndef IMX132MIPI_4LANE
	IMX132MIPI_1632_1224_2Lane_30fps_Mclk26M_setting();
	#else
	//IMX132MIPI_1632_1224_4LANE_30fps_Mclk26M_setting();
	IMX132MIPI_1632_1224_4LANE_30fps_Mclk26M_setting();
	#endif
	
    //#ifdef IMX132MIPI_USE_OTP
	#if 0		   
		ret = imx132mipi_update_wb_register_from_otp();
				if(1 == ret)
					{
				         SENSORDB("imx132mipi_update_wb_register_from_otp invalid\n");
					}
				else if(0 == ret)
					{
						 SENSORDB("imx132mipi_update_wb_register_from_otp success\n");
					}
			   
		ret = imx132mipi_update_lenc_register_from_otp();
				 if(1 == ret)
					{
						 SENSORDB("imx132mipi_update_lenc_register_from_otp invalid\n");
					}
				 else if(0 == ret)
					{
						 SENSORDB("imx132mipi_update_lenc_register_from_otp success\n");
					}
		ret = imx132mipi_update_blc_register_from_otp();
				 if(1 == ret)
					{
						 SENSORDB("imx132mipi_update_blc_register_from_otp invalid\n");
					}
				 else if(0 == ret)
					{
						 SENSORDB("imx132mipi_update_blc_register_from_otp success\n");
					}
	#endif
    //msleep(30);
	
	//IMX132MIPI_set_mirror(sensor_config_data->SensorImageMirror);
	switch (sensor_config_data->SensorOperationMode)
	{
	  case MSDK_SENSOR_OPERATION_MODE_VIDEO: 
	  	spin_lock(&imx132mipi_drv_lock);
		IMX132MIPI_sensor.video_mode = KAL_TRUE;		
		IMX132MIPI_sensor.normal_fps = IMX132MIPI_FPS(30);
		IMX132MIPI_sensor.night_fps = IMX132MIPI_FPS(15);
		spin_unlock(&imx132mipi_drv_lock);
		//dummy_line = 0;
#ifdef IMX132MIPI_DRIVER_TRACE
		SENSORDB("Video mode \n");
#endif
	   break;
	  default: /* ISP_PREVIEW_MODE */
	  	spin_lock(&imx132mipi_drv_lock);
		IMX132MIPI_sensor.video_mode = KAL_FALSE;
		spin_unlock(&imx132mipi_drv_lock);
		//dummy_line = 0;
#ifdef IMX132MIPI_DRIVER_TRACE
		SENSORDB("Camera preview mode \n");
#endif
	  break;
	}

	spin_lock(&imx132mipi_drv_lock);
	IMX132MIPI_sensor.dummy_pixels = 0;
	IMX132MIPI_sensor.dummy_lines = 0;
	IMX132MIPI_sensor.line_length = IMX132MIPI_PV_PERIOD_PIXEL_NUMS+IMX132MIPI_sensor.dummy_pixels;
	IMX132MIPI_sensor.frame_length = IMX132MIPI_PV_PERIOD_LINE_NUMS+IMX132MIPI_sensor.dummy_lines;
	IMX132MIPI_sensor.max_exposure_lines = IMX132MIPI_sensor.frame_length;
	spin_unlock(&imx132mipi_drv_lock);
	
    IMX132MIPI_Write_Shutter(IMX132MIPI_sensor.shutter);
	IMX132MIPI_Set_Dummy(IMX132MIPI_sensor.dummy_pixels, IMX132MIPI_sensor.dummy_lines); /* modify dummy_pixel must gen AE table again */

	mdelay(40);
	return ERROR_NONE;
	
}   /*  IMX132MIPIPreview   */

/*************************************************************************
* FUNCTION
*	IMX132MIPICapture
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
UINT32 IMX132MIPICapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
						  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	const kal_uint16 pv_line_length = IMX132MIPI_sensor.line_length;
	const kal_uint32 pv_pclk = IMX132MIPI_sensor.pv_pclk;
	const kal_uint32 cap_pclk = IMX132MIPI_sensor.cap_pclk;
	kal_uint32 shutter = IMX132MIPI_sensor.shutter;
	kal_uint16 dummy_pixel;
	//kal_uint32 temp;
	//kal_uint16 ret;
#ifdef IMX132MIPI_DRIVER_TRACE
	SENSORDB("IMX132MIPICapture setting start \n");
#endif
	//if((IMX132MIPI_sensor.pv_mode == KAL_TRUE)||(IMX132MIPI_sensor.is_zsd == KAL_TRUE))
	//if(IMX132MIPI_sensor.pv_mode == KAL_TRUE)
	
	if(IMX132MIPI_sensor.sensorMode == IMX132MIPI_SENSOR_MODE_CAPTURE)
		return ERROR_NONE;
	{
		// 
		spin_lock(&imx132mipi_drv_lock);
		IMX132MIPI_sensor.video_mode = KAL_FALSE;
		IMX132MIPI_sensor.is_autofliker = KAL_FALSE;		
		IMX132MIPI_sensor.sensorMode = IMX132MIPI_SENSOR_MODE_CAPTURE;
		IMX132MIPI_sensor.pclk = IMX132MIPI_CAPTURE_CLK;
		spin_unlock(&imx132mipi_drv_lock);
		
		if(IMX132MIPI_sensor.is_zsd == KAL_TRUE)
			{
				spin_lock(&imx132mipi_drv_lock);
				IMX132MIPI_sensor.pv_mode = KAL_FALSE;
				spin_unlock(&imx132mipi_drv_lock);

				//IMX132MIPI_3264_2448_2Lane_13fps_Mclk26M_setting();
				#ifndef IMX132MIPI_4LANE
				IMX132MIPI_3264_2448_2Lane_15fps_Mclk26M_setting();
				SENSORDB("IMX132MIPI_FPS 15 \n");
				#else
				IMX132MIPI_3264_2448_4LANE_30fps_Mclk26M_setting();
				SENSORDB("IMX132MIPI_FPS 30 \n");
				
				#endif

				spin_lock(&imx132mipi_drv_lock);
				IMX132MIPI_sensor.dummy_pixels = 0;
				IMX132MIPI_sensor.dummy_lines = 0;
				IMX132MIPI_sensor.line_length = IMX132MIPI_FULL_PERIOD_PIXEL_NUMS +IMX132MIPI_sensor.dummy_pixels;
				IMX132MIPI_sensor.frame_length = IMX132MIPI_FULL_PERIOD_LINE_NUMS+IMX132MIPI_sensor.dummy_lines;
				IMX132MIPI_sensor.max_exposure_lines = IMX132MIPI_sensor.frame_length;
				spin_unlock(&imx132mipi_drv_lock);

				//#ifdef IMX132MIPI_USE_OTP			   
				#if 0
					ret = imx132mipi_update_wb_register_from_otp();
				   if(1 == ret)
					   {
						   SENSORDB("imx132mipi_update_wb_register_from_otp invalid\n");
					   }
				   else if(0 == ret)
					   {
						   SENSORDB("imx132mipi_update_wb_register_from_otp success\n");
					   }
			   
				   ret = imx132mipi_update_lenc_register_from_otp();
				   if(1 == ret)
					   {
						   SENSORDB("imx132mipi_update_lenc_register_from_otp invalid\n");
					   }
				   else if(0 == ret)
					   {
						   SENSORDB("imx132mipi_update_lenc_register_from_otp success\n");
					   }
				#endif
				
				IMX132MIPI_Set_Dummy(IMX132MIPI_sensor.dummy_pixels, IMX132MIPI_sensor.dummy_lines);
			   
			}
		else
			{
				spin_lock(&imx132mipi_drv_lock);
				IMX132MIPI_sensor.pv_mode = KAL_FALSE;
				spin_unlock(&imx132mipi_drv_lock);
				
				//if(IMX132MIPI_sensor.pv_mode == KAL_TRUE)
				#ifndef IMX132MIPI_4LANE
				IMX132MIPI_3264_2448_2Lane_15fps_Mclk26M_setting();
				SENSORDB("IMX132MIPI_FPS 15 \n");
				#else
				IMX132MIPI_3264_2448_4LANE_30fps_Mclk26M_setting();
				SENSORDB("IMX132MIPI_FPS 30 \n");
				#endif
				
				spin_lock(&imx132mipi_drv_lock);
				IMX132MIPI_sensor.dummy_pixels = 0;
				IMX132MIPI_sensor.dummy_lines = 0;
				IMX132MIPI_sensor.line_length = IMX132MIPI_FULL_PERIOD_PIXEL_NUMS +IMX132MIPI_sensor.dummy_pixels;
				IMX132MIPI_sensor.frame_length = IMX132MIPI_FULL_PERIOD_LINE_NUMS+IMX132MIPI_sensor.dummy_lines;
				IMX132MIPI_sensor.max_exposure_lines = IMX132MIPI_sensor.frame_length;
				spin_unlock(&imx132mipi_drv_lock);

				//cap_fps = IMX132MIPI_FPS(8);

			    //dummy_pixel=0;
				IMX132MIPI_Set_Dummy(IMX132MIPI_sensor.dummy_pixels, IMX132MIPI_sensor.dummy_lines);

				
				#if 0
						//dummy_pixel = IMX132MIPI_sensor.cap_pclk * IMX132MIPI_FPS(1) / (IMX132MIPI_FULL_PERIOD_LINE_NUMS * cap_fps);
						//dummy_pixel = dummy_pixel < IMX132MIPI_FULL_PERIOD_PIXEL_NUMS ? 0 : dummy_pixel - IMX132MIPI_FULL_PERIOD_PIXEL_NUMS;

						//IMX132MIPI_Set_Dummy(dummy_pixel, 0);
						
				/* shutter translation */
				//shutter = shutter * pv_line_length / IMX132MIPI_sensor.line_length;
				
				SENSORDB("pv shutter %d\n",shutter);
				SENSORDB("cap pclk %d\n",cap_pclk);
				SENSORDB("pv pclk %d\n",pv_pclk);
				SENSORDB("pv line length %d\n",pv_line_length);
				SENSORDB("cap line length %d\n",IMX132MIPI_sensor.line_length);

				//shutter = shutter * (cap_pclk / pv_pclk);
				//SENSORDB("pv shutter %d\n",shutter);
				//shutter = shutter * pv_line_length / IMX132MIPI_sensor.line_length;
				//SENSORDB("pv shutter %d\n",shutter);
				shutter = ((cap_pclk / 1000) * shutter) / (pv_pclk / 1000);
				SENSORDB("pv shutter %d\n",shutter);
				#ifdef IMX132_BINNING_SUM
				shutter = 2*(shutter * pv_line_length) /IMX132MIPI_sensor.line_length*94/100;//*101/107;
				#else
				shutter = (shutter * pv_line_length) /IMX132MIPI_sensor.line_length;
				#endif
				SENSORDB("cp shutter %d\n",shutter);
				
				//shutter *= 2;
				//shutter = ( shutter * cap_pclk * pv_line_length) / (pv_pclk * IMX132MIPI_sensor.line_length);
				IMX132MIPI_Write_Shutter(shutter);
				//set_IMX132MIPI_shutter(shutter);
				#endif
				
			}

		//IMX132MIPI_Set_Dummy(IMX132MIPI_sensor.dummy_pixels, IMX132MIPI_sensor.dummy_lines);
			
		//mdelay(80);
	}
	mdelay(50);

#ifdef IMX132MIPI_DRIVER_TRACE
		SENSORDB("IMX132MIPICapture end\n");
#endif

	return ERROR_NONE;
}   /* IMX132MIPI_Capture() */
/*************************************************************************
* FUNCTION
*	IMX132MIPIVideo
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
UINT32 IMX132MIPIVideo(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
						  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	const kal_uint16 pv_line_length = IMX132MIPI_sensor.line_length;
	const kal_uint32 pv_pclk = IMX132MIPI_sensor.pv_pclk;
	const kal_uint32 cap_pclk = IMX132MIPI_sensor.cap_pclk;
	kal_uint32 shutter = IMX132MIPI_sensor.shutter;
	kal_uint16 dummy_pixel;
	//kal_uint32 temp;
	//kal_uint16 ret;
#ifdef IMX132MIPI_DRIVER_TRACE
	SENSORDB("IMX132MIPIVideo setting start \n");
#endif
	//if((IMX132MIPI_sensor.pv_mode == KAL_TRUE)||(IMX132MIPI_sensor.is_zsd == KAL_TRUE))
	//if(IMX132MIPI_sensor.pv_mode == KAL_TRUE)
	{
		// 
		spin_lock(&imx132mipi_drv_lock);
		IMX132MIPI_sensor.video_mode = KAL_FALSE;
		IMX132MIPI_sensor.is_autofliker = KAL_FALSE;
		IMX132MIPI_sensor.sensorMode = IMX132MIPI_SENSOR_MODE_VIDEO;
		IMX132MIPI_sensor.pclk = IMX132MIPI_VIDEO_CLK;
		spin_unlock(&imx132mipi_drv_lock);
			{
				spin_lock(&imx132mipi_drv_lock);
				IMX132MIPI_sensor.pv_mode = KAL_FALSE;
				spin_unlock(&imx132mipi_drv_lock);
				
				#ifndef IMX132MIPI_4LANE
				IMX132MIPI_1632_1224_2Lane_30fps_Mclk26M_setting();
				SENSORDB("IMX132MIPI_FPS 15 \n");
				#else
				IMX132MIPI_3264_1836_4LANE_30fps_Mclk26M_setting();
				SENSORDB("IMX132MIPI_FPS 30 \n");
				#endif
				
				spin_lock(&imx132mipi_drv_lock);
				IMX132MIPI_sensor.dummy_pixels = 0;
				IMX132MIPI_sensor.dummy_lines = 0;
				IMX132MIPI_sensor.line_length = IMX132MIPI_VIDEO_PERIOD_PIXEL_NUMS + IMX132MIPI_sensor.dummy_pixels;
				IMX132MIPI_sensor.frame_length = IMX132MIPI_VIDEO_PERIOD_LINE_NUMS + IMX132MIPI_sensor.dummy_lines;
				IMX132MIPI_sensor.max_exposure_lines = IMX132MIPI_sensor.frame_length;
				spin_unlock(&imx132mipi_drv_lock);

				//cap_fps = IMX132MIPI_FPS(8);
				//SENSORDB("IMX132MIPI_FPS 15 \n");

			    //dummy_pixel=0;
				IMX132MIPI_Set_Dummy(IMX132MIPI_sensor.dummy_pixels, IMX132MIPI_sensor.dummy_lines);
			}
	}
	mdelay(50);

#ifdef IMX132MIPI_DRIVER_TRACE
	SENSORDB("IMX132MIPICapture end\n");
#endif

	return ERROR_NONE;
}   /* IMX132MIPI_Capture() */

UINT32 IMX132MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
#ifdef IMX132MIPI_DRIVER_TRACE
		SENSORDB("IMX132MIPIGetResolution \n");
#endif		
//#ifdef ACDK
//	pSensorResolution->SensorFullWidth=IMX132MIPI_IMAGE_SENSOR_PV_WIDTH;
//	pSensorResolution->SensorFullHeight=IMX132MIPI_IMAGE_SENSOR_PV_HEIGHT;
//#else
	pSensorResolution->SensorFullWidth=IMX132MIPI_IMAGE_SENSOR_FULL_WIDTH;
	pSensorResolution->SensorFullHeight=IMX132MIPI_IMAGE_SENSOR_FULL_HEIGHT;
//#endif

	pSensorResolution->SensorPreviewWidth=IMX132MIPI_IMAGE_SENSOR_PV_WIDTH;
	pSensorResolution->SensorPreviewHeight=IMX132MIPI_IMAGE_SENSOR_PV_HEIGHT;
	
    pSensorResolution->SensorVideoWidth		= IMX132MIPI_IMAGE_SENSOR_VIDEO_WIDTH;
    pSensorResolution->SensorVideoHeight    = IMX132MIPI_IMAGE_SENSOR_VIDEO_HEIGHT;
	return ERROR_NONE;
}	/* IMX132MIPIGetResolution() */

UINT32 IMX132MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
					  MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
#ifdef IMX132MIPI_DRIVER_TRACE
	SENSORDB("IMX132MIPIGetInfo£¬FeatureId:%d\n",ScenarioId);
#endif
#if 1
	switch(ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			pSensorInfo->SensorPreviewResolutionX=IMX132MIPI_IMAGE_SENSOR_FULL_WIDTH;
			pSensorInfo->SensorPreviewResolutionY=IMX132MIPI_IMAGE_SENSOR_FULL_HEIGHT;
			pSensorInfo->SensorCameraPreviewFrameRate = 15;
		break;
		default:
  	        pSensorInfo->SensorPreviewResolutionX=IMX132MIPI_IMAGE_SENSOR_PV_WIDTH;
			pSensorInfo->SensorPreviewResolutionY=IMX132MIPI_IMAGE_SENSOR_PV_HEIGHT;
			pSensorInfo->SensorCameraPreviewFrameRate = 30;
		break;
	}

	//pSensorInfo->SensorPreviewResolutionX=IMX132MIPI_IMAGE_SENSOR_PV_WIDTH;
	//pSensorInfo->SensorPreviewResolutionY=IMX132MIPI_IMAGE_SENSOR_PV_HEIGHT;
	pSensorInfo->SensorFullResolutionX=IMX132MIPI_IMAGE_SENSOR_FULL_WIDTH;
	pSensorInfo->SensorFullResolutionY=IMX132MIPI_IMAGE_SENSOR_FULL_HEIGHT;

	//pSensorInfo->SensorCameraPreviewFrameRate=30;
	pSensorInfo->SensorVideoFrameRate=30;
	pSensorInfo->SensorStillCaptureFrameRate=30;
	pSensorInfo->SensorWebCamCaptureFrameRate=15;
	pSensorInfo->SensorResetActiveHigh=FALSE; //low active
	pSensorInfo->SensorResetDelayCount=5; 
#endif
	pSensorInfo->SensorOutputDataFormat=IMX132MIPI_COLOR_FORMAT;
	pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;	
	pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
#if 1
	pSensorInfo->SensorInterruptDelayLines = 4;
	
	//#ifdef MIPI_INTERFACE
   		pSensorInfo->SensroInterfaceType        = SENSOR_INTERFACE_TYPE_MIPI;
   	//#else
   	//	pSensorInfo->SensroInterfaceType		= SENSOR_INTERFACE_TYPE_PARALLEL;
   	//#endif

/*    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].MaxWidth=CAM_SIZE_5M_WIDTH;
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
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].BinningEnable=TRUE;

	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxWidth=CAM_SIZE_1M_WIDTH;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxHeight=CAM_SIZE_1M_HEIGHT;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].ISOSupported=TRUE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].BinningEnable=TRUE;*/
#endif
	pSensorInfo->CaptureDelayFrame = 2; 
	pSensorInfo->PreviewDelayFrame = 3; 
	pSensorInfo->VideoDelayFrame = 1; 	

	pSensorInfo->SensorMasterClockSwitch = 0; 
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_6MA;
    pSensorInfo->AEShutDelayFrame = 0;		   /* The frame of setting shutter default 0 for TG int */
	pSensorInfo->AESensorGainDelayFrame = 0;	   /* The frame of setting sensor gain */
	pSensorInfo->AEISPGainDelayFrame = 2; 
	
	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		//case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
			pSensorInfo->SensorClockFreq=24;
			pSensorInfo->SensorClockDividCount=	3;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 3;
			pSensorInfo->SensorDataLatchCount= 2;
			pSensorInfo->SensorGrabStartX = IMX132MIPI_PV_X_START; 
			pSensorInfo->SensorGrabStartY = IMX132MIPI_PV_Y_START; 
			
            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;		

            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	        pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
	        pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
            pSensorInfo->SensorPacketECCOrder = 1;
		break;
		
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		//case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
			pSensorInfo->SensorClockFreq=24;
			pSensorInfo->SensorClockDividCount=	3;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 3;
			pSensorInfo->SensorDataLatchCount= 2;
			pSensorInfo->SensorGrabStartX = IMX132MIPI_PV_X_START; 
			pSensorInfo->SensorGrabStartY = IMX132MIPI_PV_Y_START; 
			
            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;		
			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	        pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
	        pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
            pSensorInfo->SensorPacketECCOrder = 1;
		break;
		
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		//case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			pSensorInfo->SensorClockFreq=24;
			pSensorInfo->SensorClockDividCount= 3;
			pSensorInfo->SensorClockRisingCount=0;
			pSensorInfo->SensorClockFallingCount=2;
			pSensorInfo->SensorPixelClockCount=3;
			pSensorInfo->SensorDataLatchCount=2;
			pSensorInfo->SensorGrabStartX = IMX132MIPI_FULL_X_START; 
			pSensorInfo->SensorGrabStartY = IMX132MIPI_FULL_Y_START; 

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;		
	
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	        pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
	        pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
            pSensorInfo->SensorPacketECCOrder = 1;
		break;
		default:
			pSensorInfo->SensorClockFreq=24;
			pSensorInfo->SensorClockDividCount=3;
			pSensorInfo->SensorClockRisingCount=0;
			pSensorInfo->SensorClockFallingCount=2;		
			pSensorInfo->SensorPixelClockCount=3;
			pSensorInfo->SensorDataLatchCount=2;
			pSensorInfo->SensorGrabStartX = IMX132MIPI_PV_X_START; 
			pSensorInfo->SensorGrabStartY = IMX132MIPI_PV_Y_START; 

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;		
	
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	        pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
	        pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
            pSensorInfo->SensorPacketECCOrder = 1;
		break;
	}
#if 0
	//IMX132MIPIPixelClockDivider=pSensorInfo->SensorPixelClockCount;
	memcpy(pSensorConfigData, &IMX132MIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
#endif		
			pSensorInfo->SensorGrabStartX = pSensorInfo->SensorGrabStartX+1; 
			pSensorInfo->SensorGrabStartY = pSensorInfo->SensorGrabStartY+1; 
  return ERROR_NONE;
}	/* IMX132MIPIGetInfo() */


UINT32 IMX132MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
#ifdef IMX132MIPI_DRIVER_TRACE
	SENSORDB("IMX132MIPIControl£¬FeatureId:%d\n",ScenarioId);
#endif	

	spin_lock(&imx132mipi_drv_lock);
	IMX132MIPIMIPIRAWCurrentScenarioId = ScenarioId;
	spin_unlock(&imx132mipi_drv_lock);

	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			IMX132MIPIPreview(pImageWindow, pSensorConfigData);
			break;
			
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		//case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
			IMX132MIPIVideo(pImageWindow, pSensorConfigData);
		break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		//case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
			if(IMX132MIPI_sensor.is_zsd == KAL_TRUE)
				{
					spin_lock(&imx132mipi_drv_lock);
					IMX132MIPI_sensor.is_zsd_cap = KAL_TRUE;
					spin_unlock(&imx132mipi_drv_lock);
					SENSORDB("IMX132MIPIControl£¬is_zsd_cap is true!\n");
				}
			else
				{
					spin_lock(&imx132mipi_drv_lock);
					IMX132MIPI_sensor.is_zsd_cap = KAL_FALSE;
					spin_unlock(&imx132mipi_drv_lock);
					SENSORDB("IMX132MIPIControl£¬is_zsd_cap is false!\n");
				}
			
			IMX132MIPICapture(pImageWindow, pSensorConfigData);
			break;
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			spin_lock(&imx132mipi_drv_lock);
			IMX132MIPI_sensor.is_zsd = KAL_TRUE;  //for zsd full size preview
			IMX132MIPI_sensor.is_zsd_cap = KAL_FALSE;
			spin_unlock(&imx132mipi_drv_lock);
			IMX132MIPICapture(pImageWindow, pSensorConfigData);
		break;		
        default:
            return ERROR_INVALID_SCENARIO_ID;
	}
	return ERROR_NONE;
}	/* IMX132MIPIControl() */

UINT32 IMX132MIPISetAutoFlickerMode(kal_bool bEnable, UINT16 u2FrameRate)
{
	
	//kal_uint32 pv_max_frame_rate_lines = IMX132MIPI_sensor.dummy_lines;
	
	SENSORDB("[IMX132MIPISetAutoFlickerMode] frame rate(10base) = %d %d\n", bEnable, u2FrameRate);

	if(bEnable)
		{
		    
			spin_lock(&imx132mipi_drv_lock);
			IMX132MIPI_sensor.is_autofliker = KAL_TRUE;
			spin_unlock(&imx132mipi_drv_lock);

			//if(IMX132MIPI_sensor.video_mode = KAL_TRUE)
			//	{
			//		pv_max_frame_rate_lines = IMX132MIPI_sensor.frame_length + (IMX132MIPI_sensor.frame_length>>7);
			//		IMX132MIPI_write_cmos_sensor(0x380e, (pv_max_frame_rate_lines>>8)&0xFF);
			//		IMX132MIPI_write_cmos_sensor(0x380f, (pv_max_frame_rate_lines)&0xFF);
			//	}	
		}
	else
		{
			spin_lock(&imx132mipi_drv_lock);
			IMX132MIPI_sensor.is_autofliker = KAL_FALSE;
			spin_unlock(&imx132mipi_drv_lock);
			
			//if(IMX132MIPI_sensor.video_mode = KAL_TRUE)
			//	{
			//		pv_max_frame_rate_lines = IMX132MIPI_sensor.frame_length;
			//		IMX132MIPI_write_cmos_sensor(0x380e, (pv_max_frame_rate_lines>>8)&0xFF);
			//		IMX132MIPI_write_cmos_sensor(0x380f, (pv_max_frame_rate_lines)&0xFF);
			//	}	
		}
	SENSORDB("[IMX132MIPISetAutoFlickerMode]bEnable:%x \n",bEnable);
	return 0;
}


UINT32 IMX132MIPISetVideoMode(UINT16 u2FrameRate)
{
	kal_int16 dummy_line;
	UINT16 frameRate;
    /* to fix VSYNC, to fix frame rate */
#ifdef IMX132MIPI_DRIVER_TRACE
	SENSORDB("IMX132MIPISetVideoMode, u2FrameRate:%d\n",u2FrameRate);
#endif	
	if(u2FrameRate==0)
	{
		SENSORDB("Disable Video Mode or dynimac fps\n");
		spin_lock(&imx132mipi_drv_lock);
		IMX132MIPI_sensor.video_mode = KAL_FALSE;
		spin_unlock(&imx132mipi_drv_lock);
		return KAL_TRUE;
	}

	if(IMX132MIPI_sensor.is_autofliker == KAL_TRUE)
	{
		if (u2FrameRate==30)
			frameRate= 306;
		else if(u2FrameRate==15)
			frameRate= 148;//148;
		else
			frameRate=u2FrameRate*10;
	}
	else
		frameRate=u2FrameRate*10;

	IMX132MIPISetMaxFrameRate(frameRate);
	
	spin_lock(&imx132mipi_drv_lock);
	IMX132MIPI_sensor.max_exposure_lines = IMX132MIPI_sensor.frame_length;
	IMX132MIPI_sensor.video_mode = KAL_TRUE;
	spin_unlock(&imx132mipi_drv_lock);


#if 0
    if((30 == u2FrameRate)||(15 == u2FrameRate))
    	{
			if( IMX132MIPIMIPIRAWCurrentScenarioId == MSDK_SCENARIO_ID_CAMERA_ZSD)
				dummy_line = IMX132MIPI_sensor.cap_pclk / u2FrameRate / IMX132MIPI_FULL_PERIOD_PIXEL_NUMS - IMX132MIPI_FULL_PERIOD_LINE_NUMS;
			else
				dummy_line = IMX132MIPI_sensor.pv_pclk / u2FrameRate / IMX132MIPI_PV_PERIOD_PIXEL_NUMS - IMX132MIPI_PV_PERIOD_LINE_NUMS;
				
			if (dummy_line < 0) dummy_line = 0;
         #ifdef IMX132MIPI_DRIVER_TRACE
			SENSORDB("dummy_line %d\n", dummy_line);
         #endif
		 
			IMX132MIPI_Set_Dummy(0, dummy_line); /* modify dummy_pixel must gen AE table again */
		 
		 	spin_lock(&imx132mipi_drv_lock);
			IMX132MIPI_sensor.video_mode = KAL_TRUE;
			spin_unlock(&imx132mipi_drv_lock);
			
    	}
#endif
    return KAL_TRUE;
}
UINT32 IMX132MIPISetMaxFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 frameRate) {
	kal_uint16 pclk, lineLength;
	kal_int16 dummyLine;
	kal_uint16 frame_length;
	IMX132MIPIMIPIRAWCurrentScenarioId = scenarioId;

	SENSORDB("IMX132MIPISetMaxFramerateByScenario: scenarioId = %d, frame rate = %d\n",scenarioId,frameRate);
	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			pclk = IMX132MIPI_PREVIEW_CLK;
			lineLength = IMX132MIPI_PV_PERIOD_PIXEL_NUMS;
			frame_length = (10 * pclk)/frameRate/lineLength;
			dummyLine = frame_length - IMX132MIPI_PV_PERIOD_LINE_NUMS;
			//IMX132MIPI_sensor.sensorMode = SENSOR_MODE_PREVIEW;
			IMX132MIPI_Set_Dummy(0, dummyLine);			
			spin_lock(&imx132mipi_drv_lock);
			IMX132MIPI_sensor.max_exposure_lines = IMX132MIPI_sensor.frame_length;
			spin_unlock(&imx132mipi_drv_lock);	
			break;			
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			pclk = IMX132MIPI_VIDEO_CLK;
			lineLength = IMX132MIPI_VIDEO_PERIOD_PIXEL_NUMS;
			frame_length = (10 * pclk)/frameRate/lineLength;
			dummyLine = frame_length - IMX132MIPI_VIDEO_PERIOD_LINE_NUMS;
			//IMX132MIPI_sensor.sensorMode = SENSOR_MODE_VIDEO;
			IMX132MIPI_Set_Dummy(0, dummyLine);			
			spin_lock(&imx132mipi_drv_lock);
			IMX132MIPI_sensor.max_exposure_lines = IMX132MIPI_sensor.frame_length;
			spin_unlock(&imx132mipi_drv_lock);	
			break;			
			 break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:			
			pclk = IMX132MIPI_CAPTURE_CLK;
			lineLength = IMX132MIPI_FULL_PERIOD_PIXEL_NUMS;
			frame_length = (10 * pclk)/frameRate/lineLength;
			dummyLine = frame_length - IMX132MIPI_FULL_PERIOD_LINE_NUMS;
			//IMX132MIPI_sensor.sensorMode = SENSOR_MODE_CAPTURE;
			IMX132MIPI_Set_Dummy(0, dummyLine);			
			spin_lock(&imx132mipi_drv_lock);
			IMX132MIPI_sensor.max_exposure_lines = IMX132MIPI_sensor.frame_length;
			spin_unlock(&imx132mipi_drv_lock);		
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


UINT32 IMX132MIPIGetDefaultFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 *pframeRate) 
{

	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			 *pframeRate = 300;
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			#ifndef IMX132MIPI_4LANE
			*pframeRate = 300;
			#else
			*pframeRate = 300;
			#endif
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


UINT32 IMX132MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
							 UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
	UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
	UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
	UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
	UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
	UINT32 IMX132MIPISensorRegNumber;
	UINT32 i;
	//PNVRAM_SENSOR_DATA_STRUCT pSensorDefaultData=(PNVRAM_SENSOR_DATA_STRUCT) pFeaturePara;
	//MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
	MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;
	//MSDK_SENSOR_GROUP_INFO_STRUCT *pSensorGroupInfo=(MSDK_SENSOR_GROUP_INFO_STRUCT *) pFeaturePara;
	//MSDK_SENSOR_ITEM_INFO_STRUCT *pSensorItemInfo=(MSDK_SENSOR_ITEM_INFO_STRUCT *) pFeaturePara;
	//MSDK_SENSOR_ENG_INFO_STRUCT	*pSensorEngInfo=(MSDK_SENSOR_ENG_INFO_STRUCT *) pFeaturePara;

#ifdef IMX132MIPI_DRIVER_TRACE
	//SENSORDB("IMX132MIPIFeatureControl£¬FeatureId:%d\n",FeatureId); 
#endif		
	switch (FeatureId)
	{
	
		case SENSOR_FEATURE_GET_RESOLUTION:
			*pFeatureReturnPara16++=IMX132MIPI_IMAGE_SENSOR_FULL_WIDTH;
			*pFeatureReturnPara16=IMX132MIPI_IMAGE_SENSOR_FULL_HEIGHT;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_GET_PERIOD:	/* 3 */
			//switch(IMX132MIPIMIPIRAWCurrentScenarioId)
				{
					/*case MSDK_SCENARIO_ID_CAMERA_ZSD:
						*pFeatureReturnPara16++= IMX132MIPI_FeatureControl_PERIOD_PixelNum;
						*pFeatureReturnPara16= IMX132MIPI_FeatureControl_PERIOD_LineNum;
						*pFeatureParaLen=4;
			            #ifdef IMX132MIPI_DRIVER_TRACE
				          SENSORDB("SENSOR_FEATURE_GET_PERIOD£¬IMX132MIPI cap line length:%d\n",IMX132MIPI_FULL_PERIOD_PIXEL_NUMS + IMX132MIPI_sensor.dummy_pixels); 
			            #endif	
						break;

						
					default:*/
						*pFeatureReturnPara16++= IMX132MIPI_sensor.line_length;
						*pFeatureReturnPara16= IMX132MIPI_sensor.frame_length;
						*pFeatureParaLen=4;
			            #ifdef IMX132MIPI_DRIVER_TRACE
				          SENSORDB("SENSOR_FEATURE_GET_PERIOD£¬IMX132MIPI pv line length:%d\n",IMX132MIPI_FeatureControl_PERIOD_PixelNum); 
			            #endif	
						break;
				}		
		break;
		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:  /* 3 */
			switch(IMX132MIPIMIPIRAWCurrentScenarioId)
				{
				  /* case MSDK_SCENARIO_ID_CAMERA_ZSD:
						*pFeatureReturnPara32 = IMX132MIPI_ZSD_PRE_CLK; //IMX132MIPI_sensor.cap_pclk;
						*pFeatureParaLen=4;
						#ifdef IMX132MIPI_DRIVER_TRACE
				          SENSORDB("SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ£¬IMX132MIPI_ZSD_PRE_CLK:%d\n",IMX132MIPI_ZSD_PRE_CLK); 
			            #endif
						break;*/

						
						case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
							*pFeatureReturnPara32 = IMX132MIPI_PREVIEW_CLK;
							*pFeatureParaLen=4;
							#ifdef IMX132MIPI_DRIVER_TRACE
				          	SENSORDB("SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ£¬IMX132MIPI_PREVIEW_CLK:%d\n",IMX132MIPI_PREVIEW_CLK); 
			            	#endif
							break;
						case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
							*pFeatureReturnPara32 = IMX132MIPI_VIDEO_CLK;
							*pFeatureParaLen=4;
							#ifdef IMX132MIPI_DRIVER_TRACE
				          	SENSORDB("SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ£¬MSDK_SCENARIO_ID_VIDEO_PREVIEW:%d\n",IMX132MIPI_VIDEO_CLK); 
			            	#endif
							break;
						case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
						case MSDK_SCENARIO_ID_CAMERA_ZSD:
							*pFeatureReturnPara32 = IMX132MIPI_CAPTURE_CLK;
							*pFeatureParaLen=4;
							#ifdef IMX132MIPI_DRIVER_TRACE
				          	SENSORDB("SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ£¬IMX132MIPI_CAPTURE_CLK/ZSD:%d\n",IMX132MIPI_CAPTURE_CLK); 
			            	#endif
							break;
							
						default:
							*pFeatureReturnPara32 = IMX132MIPI_sensor.pv_pclk;
							*pFeatureParaLen=4;
							#ifdef IMX132MIPI_DRIVER_TRACE
							SENSORDB("SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ£¬IMX132MIPI_sensor.pv_pclk:%d\n",IMX132MIPI_sensor.pv_pclk); 
							#endif
							break;
				}
		break;
		case SENSOR_FEATURE_SET_ESHUTTER:	/* 4 */
			set_IMX132MIPI_shutter(*pFeatureData16);
		break;
		case SENSOR_FEATURE_SET_NIGHTMODE:
			//IMX132MIPI_night_mode((BOOL) *pFeatureData16);
		break;
		case SENSOR_FEATURE_SET_GAIN:	/* 6 */
			IMX132MIPI_SetGain((UINT16) *pFeatureData16);
		break;
		case SENSOR_FEATURE_SET_FLASHLIGHT:
		break;
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
		break;
		case SENSOR_FEATURE_SET_REGISTER:
		IMX132MIPI_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
		break;
		case SENSOR_FEATURE_GET_REGISTER:
			pSensorRegData->RegData = IMX132MIPI_read_cmos_sensor(pSensorRegData->RegAddr);
		break;
		case SENSOR_FEATURE_SET_CCT_REGISTER:
			//memcpy(&IMX132MIPI_sensor.eng.cct, pFeaturePara, sizeof(IMX132MIPI_sensor.eng.cct));
			IMX132MIPISensorRegNumber = IMX132MIPI_FACTORY_END_ADDR;
			for (i=0;i<IMX132MIPISensorRegNumber;i++)
            {
                spin_lock(&imx132mipi_drv_lock);
                IMX132MIPI_sensor.eng.cct[i].Addr=*pFeatureData32++;
                IMX132MIPI_sensor.eng.cct[i].Para=*pFeatureData32++;
			    spin_unlock(&imx132mipi_drv_lock);
            }
			
		break;
		case SENSOR_FEATURE_GET_CCT_REGISTER:	/* 12 */
			if (*pFeatureParaLen >= sizeof(IMX132MIPI_sensor.eng.cct) + sizeof(kal_uint32))
			{
			  *((kal_uint32 *)pFeaturePara++) = sizeof(IMX132MIPI_sensor.eng.cct);
			  memcpy(pFeaturePara, &IMX132MIPI_sensor.eng.cct, sizeof(IMX132MIPI_sensor.eng.cct));
			}
			break;
		case SENSOR_FEATURE_SET_ENG_REGISTER:
			//memcpy(&IMX132MIPI_sensor.eng.reg, pFeaturePara, sizeof(IMX132MIPI_sensor.eng.reg));
			IMX132MIPISensorRegNumber = IMX132MIPI_ENGINEER_END;
			for (i=0;i<IMX132MIPISensorRegNumber;i++)
            {
                spin_lock(&imx132mipi_drv_lock);
                IMX132MIPI_sensor.eng.reg[i].Addr=*pFeatureData32++;
                IMX132MIPI_sensor.eng.reg[i].Para=*pFeatureData32++;
			    spin_unlock(&imx132mipi_drv_lock);
            }
			break;
		case SENSOR_FEATURE_GET_ENG_REGISTER:	/* 14 */
			if (*pFeatureParaLen >= sizeof(IMX132MIPI_sensor.eng.reg) + sizeof(kal_uint32))
			{
			  *((kal_uint32 *)pFeaturePara++) = sizeof(IMX132MIPI_sensor.eng.reg);
			  memcpy(pFeaturePara, &IMX132MIPI_sensor.eng.reg, sizeof(IMX132MIPI_sensor.eng.reg));
			}
		case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
			((PNVRAM_SENSOR_DATA_STRUCT)pFeaturePara)->Version = NVRAM_CAMERA_SENSOR_FILE_VERSION;
			((PNVRAM_SENSOR_DATA_STRUCT)pFeaturePara)->SensorId = IMX132MIPI_SENSOR_ID;
			memcpy(((PNVRAM_SENSOR_DATA_STRUCT)pFeaturePara)->SensorEngReg, &IMX132MIPI_sensor.eng.reg, sizeof(IMX132MIPI_sensor.eng.reg));
			memcpy(((PNVRAM_SENSOR_DATA_STRUCT)pFeaturePara)->SensorCCTReg, &IMX132MIPI_sensor.eng.cct, sizeof(IMX132MIPI_sensor.eng.cct));
			*pFeatureParaLen = sizeof(NVRAM_SENSOR_DATA_STRUCT);
			break;
		case SENSOR_FEATURE_GET_CONFIG_PARA:
			memcpy(pFeaturePara, &IMX132MIPI_sensor.cfg_data, sizeof(IMX132MIPI_sensor.cfg_data));
			*pFeatureParaLen = sizeof(IMX132MIPI_sensor.cfg_data);
			break;
		case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
		     IMX132MIPI_camera_para_to_sensor();
		break;
		case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
			IMX132MIPI_sensor_to_camera_para();
		break;							
		case SENSOR_FEATURE_GET_GROUP_COUNT:
			IMX132MIPI_get_sensor_group_count((kal_uint32 *)pFeaturePara);
			*pFeatureParaLen = 4;
		break;										
		  IMX132MIPI_get_sensor_group_info((MSDK_SENSOR_GROUP_INFO_STRUCT *)pFeaturePara);
		  *pFeatureParaLen = sizeof(MSDK_SENSOR_GROUP_INFO_STRUCT);
		  break;
		case SENSOR_FEATURE_GET_ITEM_INFO:
		  IMX132MIPI_get_sensor_item_info((MSDK_SENSOR_ITEM_INFO_STRUCT *)pFeaturePara);
		  *pFeatureParaLen = sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
		  break;
		case SENSOR_FEATURE_SET_ITEM_INFO:
		  IMX132MIPI_set_sensor_item_info((MSDK_SENSOR_ITEM_INFO_STRUCT *)pFeaturePara);
		  *pFeatureParaLen = sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
		  break;
		case SENSOR_FEATURE_GET_ENG_INFO:
     		memcpy(pFeaturePara, &IMX132MIPI_sensor.eng_info, sizeof(IMX132MIPI_sensor.eng_info));
     		*pFeatureParaLen = sizeof(IMX132MIPI_sensor.eng_info);
     		break;
		case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
			// get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
			// if EEPROM does not exist in camera module.
			*pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_SET_VIDEO_MODE:
			IMX132MIPISetVideoMode(*pFeatureData16);
        break; 
        case SENSOR_FEATURE_CHECK_SENSOR_ID:
            IMX132MIPIGetSensorID(pFeatureReturnPara32); 
            break; 
		case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
			IMX132MIPISetAutoFlickerMode((BOOL)*pFeatureData16,*(pFeatureData16+1));
			break;
		case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
			IMX132MIPISetMaxFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, *(pFeatureData32+1));
			break;
		case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
			IMX132MIPIGetDefaultFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, (MUINT32 *)(*(pFeatureData32+1)));
			break;
		default:
			break;
	}
	return ERROR_NONE;
}	/* IMX132MIPIFeatureControl() */
SENSOR_FUNCTION_STRUCT	SensorFuncIMX132MIPI=
{
	IMX132MIPIOpen,
	IMX132MIPIGetInfo,
	IMX132MIPIGetResolution,
	IMX132MIPIFeatureControl,
	IMX132MIPIControl,
	IMX132MIPIClose
};

UINT32 IMX132_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&SensorFuncIMX132MIPI;

	return ERROR_NONE;
}	/* SensorInit() */



