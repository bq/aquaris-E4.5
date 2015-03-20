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

#include "ov8865mipiraw_Sensor.h"
#include "ov8865mipiraw_Camera_Sensor_para.h"
#include "ov8865mipiraw_CameraCustomized.h"
static DEFINE_SPINLOCK(ov8865mipiraw_drv_lock);

#define OV8865_DEBUG
#ifdef OV8865_DEBUG
	#define OV8865DB(fmt, arg...) xlog_printk(ANDROID_LOG_DEBUG, "[OV8865Raw] ",  fmt, ##arg)
#else
	#define OV8865DB(fmt, arg...)
#endif


kal_uint32 OV8865_FeatureControl_PERIOD_PixelNum=OV8865_PV_PERIOD_PIXEL_NUMS;
kal_uint32 OV8865_FeatureControl_PERIOD_LineNum=OV8865_PV_PERIOD_LINE_NUMS;

UINT16 OV8865_VIDEO_MODE_TARGET_FPS = 30;

MSDK_SCENARIO_ID_ENUM OV8865CurrentScenarioId = MSDK_SCENARIO_ID_CAMERA_PREVIEW;
MSDK_SENSOR_CONFIG_STRUCT OV8865SensorConfigData;
static OV8865_PARA_STRUCT ov8865;
kal_uint32 OV8865_FAC_SENSOR_REG;


SENSOR_REG_STRUCT OV8865SensorCCT[]=CAMERA_SENSOR_CCT_DEFAULT_VALUE;
SENSOR_REG_STRUCT OV8865SensorReg[ENGINEER_END]=CAMERA_SENSOR_REG_DEFAULT_VALUE;


#define OV8865_TEST_PATTERN_CHECKSUM 0xf5e2f1ce
kal_bool OV8865_During_testpattern = KAL_FALSE;

extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);

#define OV8865_write_cmos_sensor(addr, para) iWriteReg((u16) addr , (u32) para , 1, OV8865MIPI_WRITE_ID)

kal_uint16 OV8865_read_cmos_sensor(kal_uint32 addr)
{
kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,OV8865MIPI_WRITE_ID);
    return get_byte;
}




#define OV8865_OTP

#if defined(OV8865_OTP)

#define LENSC_SETTINGS_NUM 62

//Register and Addrs definition
#define OTP_BUF_BASE_ADDR         0x7000 //
#define OTP_BUF_USABLE_OFFSET 	 0x10 //usable buffer 0x7010~0x720f  512Bytes
#define OTP_BUF_ADDR 			 OTP_BUF_BASE_ADDR + OTP_BUF_USABLE_OFFSET //0x7010

#define OTP_BUF_INFO_OFFSET 0
#define OTP_INFO_BASE_ADDR OTP_BUF_ADDR+OTP_BUF_INFO_OFFSET //0x7010

#define OTP_BUF_AWB_OFFSET 	0x10
#define OTP_AWB_BASE_ADDR OTP_BUF_ADDR+OTP_BUF_AWB_OFFSET //0x7020

#define OTP_BUF_VCM_OFFSET 	0x20
#define OTP_VCM_BASE_ADDR OTP_BUF_ADDR+OTP_BUF_VCM_OFFSET//0x7030

#define OTP_BUF_LCS_OFFSET 	0x2A
#define OTP_LCS_BASE_ADDR OTP_BUF_ADDR+OTP_BUF_LCS_OFFSET //0x703A

typedef enum {
	MI_ID=0,
	LENS_ID,
	YEAR,
	MONTH,
	DAY,
	INFO_GROUP=5,
}OTP_INFO_OFFSET;

typedef enum {
	AWB_RG_MSB=0,
	AWB_BG_MSB,
	LIGHT_RG_MSB,
	LIGHT_BG_MSB,
	AWB_LIGHT_LSB,
	AWB_GROUP=5,
} OTP_AWB_OFFSET;

typedef enum {
	VCMST_MSB=0,
	VCMED_MSB,
	VCM_DIR_LSB,
	VCM_GROUP=3,
} OTP_VCM_OFFSET;

typedef enum {
	LCS=0,
	LCS_GROUP=LENSC_SETTINGS_NUM,
} OTP_LCS_OFFSET;

#define OTP_LOAD_DUMP         0x3D81
#define OTP_MODE_SELECT         0x3D84

#define OTP_DUMP_STADD_H  0x3D88
#define OTP_DUMP_STADD_L   0x3D89

#define OTP_DUMP_EDADD_H  0x3D8A
#define OTP_DUMP_EDADD_L   0x3D8B

#define LCS_BUF_SIZE         LENSC_SETTINGS_NUM

#define GAIN_RH_ADDR          0x5018
#define GAIN_RL_ADDR          0x5019
#define GAIN_GH_ADDR          0x501a
#define GAIN_GL_ADDR          0x501b
#define GAIN_BH_ADDR          0x501c
#define GAIN_BL_ADDR          0x501d

#define GAIN_DEFAULT_VALUE    0x0400 // 1x gain

#define OTP_MID               0x06 //Qtech

//For HW define
struct otp_struct {
	int product_year;
	int product_month;
	int product_day;
	int module_integrator_id;
	int lens_id;
	int rg_ratio;
	int bg_ratio;
	int light_rg;
	int light_bg;
	int VCM_start;
	int VCM_end;
	int VCM_dir;
	int lenc[LENSC_SETTINGS_NUM];

};

// R/G and B/G of typical camera module is defined here

int RG_Ratio_Typical = 0x126;//0x230;
int BG_Ratio_Typical = 0x122;// 0x2B1;

#define OTPLOG(fmt, arg...) xlog_printk(ANDROID_LOG_DEBUG, "OTP",  fmt, ##arg)

static struct otp_struct current_otp;

static void clear_otp_buffer(int start_addr,int end_addr)
{
	int i;
	// clear otp buffer
	for (i=start_addr;i<end_addr;i++) {
		OV8865_write_cmos_sensor(i, 0x00);
	}
}

static void dump_otp_buffer(int start_addr,int end_addr)
{

	OTPLOG("dump_otp_buffer from %#x to %#x\n",start_addr,end_addr);
	// select otp mode
	OV8865_write_cmos_sensor(OTP_MODE_SELECT, 0xC0);
	//set dump addr
	OV8865_write_cmos_sensor(OTP_DUMP_STADD_H,(start_addr>>8));
	OV8865_write_cmos_sensor(OTP_DUMP_STADD_L,(start_addr&0xff));

	OV8865_write_cmos_sensor(OTP_DUMP_EDADD_H,(end_addr>>8));
	OV8865_write_cmos_sensor(OTP_DUMP_EDADD_L,(end_addr&0xff));
		
	// read otp into buffer
	OV8865_write_cmos_sensor(OTP_LOAD_DUMP, 0x01);

	// disable otp read
	//OV8865_write_cmos_sensor(0x3d81, 0x00);

}

//For HW
// index: index of otp group. (1, 2, 3)
// return: 	index 1, 2, 3, if empty, return 0,invalid return -1;
static int check_otp_info()
{
	int flag;
	int index;
	int reg, start_addr,end_addr;

	for(index = 1;index<=3;index++)
	{
		//set dump addr
		start_addr=OTP_INFO_BASE_ADDR;//dupm wb flag
		end_addr=OTP_INFO_BASE_ADDR;
		
		dump_otp_buffer(start_addr,end_addr);
		mdelay(5);
		
		// read info flag
		flag = OV8865_read_cmos_sensor(OTP_INFO_BASE_ADDR);
		OTPLOG("check_otp_info, info_flag = %x \n",flag);

		//clear buffer
		clear_otp_buffer(start_addr,end_addr);
		//OV8865_write_cmos_sensor(OTP_AWB_BASE_ADDR, 0x00)

		//check WB group 
		flag = (flag>>(8-2*index))&0x3;		

		if(flag==0x1)
			{
			return index;
		}
		else if(flag==0x0)
			{
		      //return 0;
		}
		else
			{
			//return 2;
		}

	}
	return 0;
}


// For HW
// index: index of otp group. (1, 2, 3)
// otp_ptr: pointer of otp_struct
// return: 	0, 
static int read_otp_info(int index, struct otp_struct * otp_ptr)
{
	int awb_light_lsb;
	int group_addr,start_addr,end_addr;
	  
	//set dump addr
	group_addr = OTP_INFO_BASE_ADDR+(INFO_GROUP*(index-1))+1;//select group
	start_addr=group_addr;//dupm one group one time
	end_addr=start_addr+INFO_GROUP-1;
	OTPLOG("read_otp_info, dump addr start= %x end=%x ,group=%d \n",start_addr,end_addr,index);	
	
	dump_otp_buffer(start_addr,end_addr);
	mdelay(5);
	
	(*otp_ptr).product_year = OV8865_read_cmos_sensor(group_addr + YEAR);
	(*otp_ptr).product_month = OV8865_read_cmos_sensor(group_addr + MONTH);
	(*otp_ptr).product_day = OV8865_read_cmos_sensor(group_addr + DAY );
	(*otp_ptr).module_integrator_id =  OV8865_read_cmos_sensor(group_addr + MI_ID );
	(*otp_ptr).lens_id =  OV8865_read_cmos_sensor(group_addr + LENS_ID );

	OTPLOG("read_otp_info,Date:%d\\%d\\%d  ModuleID=%d   LensID=%d \n",(*otp_ptr).product_year,(*otp_ptr).product_month,(*otp_ptr).product_day,(*otp_ptr).module_integrator_id,(*otp_ptr).lens_id);
	// disable otp read
	//OV8865_write_cmos_sensor(0x3d81, 0x00);

	clear_otp_buffer(start_addr,end_addr);


	return 0;	
}

//For HW
// index: index of otp group. (1, 2, 3)
// return: 	index 1, 2, 3, if empty, return 0,invalid return -1;
static int check_otp_wb()
{
	int flag,i;
	int index;
	int reg, start_addr,end_addr;

	for(index = 1;index<=3;index++)
	{
		//set dump addr
		start_addr=OTP_AWB_BASE_ADDR;//dupm wb flag
		end_addr=OTP_AWB_BASE_ADDR;
		
		dump_otp_buffer(start_addr,end_addr);
		mdelay(5);

		// read WB flag
		flag = OV8865_read_cmos_sensor(OTP_AWB_BASE_ADDR);
		OTPLOG("check_otp_wb, wb_flag = %x \n",flag);

		//clear buffer
		clear_otp_buffer(start_addr,end_addr);
		//OV8865_write_cmos_sensor(OTP_AWB_BASE_ADDR, 0x00)

		//check WB group 
		flag = (flag>>(8-2*index))&0x3;		

		if(flag==0x1)
			{
			return index;
		}
		else if(flag==0x0)
			{
		      //return 0;
		}
		else
			{
			//return -1;
		}

	}
	return 0;
}




// For HW
// index: index of otp group. (1, 2, 3)
// otp_ptr: pointer of otp_struct
// return: 	0, 
static int read_otp_wb(int index, struct otp_struct * otp_ptr)
{
	int awb_light_lsb;
	int group_addr,start_addr,end_addr;
	  
	//set dump addr
	group_addr = OTP_AWB_BASE_ADDR+(AWB_GROUP*(index-1))+1;//select group
	start_addr=group_addr;//dupm one group one time
	end_addr=start_addr+AWB_GROUP-1;
	OTPLOG("read_otp_wb, dump addr start= %x end=%x ,group=%d \n",start_addr,end_addr,index);	
	
	dump_otp_buffer(start_addr,end_addr);
	mdelay(5);
	
	awb_light_lsb=OV8865_read_cmos_sensor(group_addr+AWB_LIGHT_LSB);
	(*otp_ptr).rg_ratio = ((OV8865_read_cmos_sensor(group_addr + AWB_RG_MSB))<<2)+((awb_light_lsb>>6)&0x3);
	(*otp_ptr).bg_ratio = ((OV8865_read_cmos_sensor(group_addr + AWB_BG_MSB))<<2)+((awb_light_lsb>>4)&0x3);
	(*otp_ptr).light_rg = ((OV8865_read_cmos_sensor(group_addr + LIGHT_RG_MSB ))<<2)+((awb_light_lsb>>2)&0x3);
	(*otp_ptr).light_bg =  ((OV8865_read_cmos_sensor(group_addr + LIGHT_BG_MSB ))<<2)+((awb_light_lsb)&0x3);

	OTPLOG("read_otp_wb, rg_ratio= %d  bg_ratio=%d ,light_rg=%d light_bg=%d\n",(*otp_ptr).rg_ratio,(*otp_ptr).bg_ratio,(*otp_ptr).light_rg,(*otp_ptr).light_bg);
	// disable otp read
	//OV8865_write_cmos_sensor(0x3d81, 0x00);

	clear_otp_buffer(start_addr,end_addr);


	return 0;	
}

//For HW
// index: index of otp group. (1, 2, 3)
// return: 	index 1, 2, 3, if empty, return 0,invalid return -1;
static int check_otp_lenc()
{
	int flag,i;
	int index;
	int reg, start_addr,end_addr;

	for(index = 1;index<=3;index++)
	{
		//set dump addr
		start_addr=OTP_LCS_BASE_ADDR;//dupm wb flag
		end_addr=OTP_LCS_BASE_ADDR;
		
		dump_otp_buffer(start_addr,end_addr);
		mdelay(5);

		// read LCS flag
		flag = OV8865_read_cmos_sensor(OTP_LCS_BASE_ADDR);
		OTPLOG("check_otp_lenc, lenc_flag = %x \n",flag);

		//clear buffer
		clear_otp_buffer(start_addr,end_addr);
		//OV8865_write_cmos_sensor(OTP_AWB_BASE_ADDR, 0x00)

		//check LCS group 
		flag = (flag>>(8-2*index))&0x3;		

		if(flag==0x1)
			{
			return index;
		}
		else if(flag==0x0)
			{
		      //return 0;
		}
		else
			{
			//return -1;
		}

	}
	return 0;
}

// For HW
// index: index of otp group. (1, 2, 3)
// otp_ptr: pointer of otp_struct
// return: 	0, 
static int read_otp_lenc(int index, struct otp_struct * otp_ptr)
{
	int bank, i;
	int group_addr,start_addr,end_addr;
	
	//set dump addr
	group_addr = OTP_LCS_BASE_ADDR+(LCS_GROUP*(index-1))+1;//select group
	start_addr=group_addr;//dupm one group one time
	end_addr=start_addr+LCS_GROUP-1;
	OTPLOG("read_otp_lenc, dump addr start= %x end=%x ,group=%d \n",start_addr,end_addr,index);	
	
	dump_otp_buffer(start_addr,end_addr);
	mdelay(10);

	for(i=0;i<LCS_BUF_SIZE;i++) {
		(* otp_ptr).lenc[i]=OV8865_read_cmos_sensor(group_addr+i);
	}

	// disable otp read
	//OV8865_write_cmos_sensor(0x3d81, 0x00);

	clear_otp_buffer(start_addr,end_addr);

	return 0;	
}


// R_gain, sensor red gain of AWB, 0x400 =1
// G_gain, sensor green gain of AWB, 0x400 =1
// B_gain, sensor blue gain of AWB, 0x400 =1
// return 0;
static int update_awb_gain(int R_gain, int G_gain, int B_gain)
{
	if (R_gain>=GAIN_DEFAULT_VALUE) {
		OV8865_write_cmos_sensor(GAIN_RH_ADDR, R_gain>>6);
		OV8865_write_cmos_sensor(GAIN_RL_ADDR, R_gain & 0x003f);
	}

	if (G_gain>=GAIN_DEFAULT_VALUE) {
		OV8865_write_cmos_sensor(GAIN_GH_ADDR, G_gain>>6);
		OV8865_write_cmos_sensor(GAIN_GL_ADDR, G_gain & 0x003f);
	}

	if (B_gain>=GAIN_DEFAULT_VALUE) {
		OV8865_write_cmos_sensor(GAIN_BH_ADDR, B_gain>>6);
		OV8865_write_cmos_sensor(GAIN_BL_ADDR, B_gain & 0x003f);
	}

    OTPLOG("update_awb_gain, R=%x, G=%x, B=%x\n",R_gain, G_gain, B_gain);
    OTPLOG("update_awb_gain, %x = %x\n",GAIN_RH_ADDR, OV8865_read_cmos_sensor(GAIN_RH_ADDR));
    OTPLOG("update_awb_gain, %x = %x\n",GAIN_RL_ADDR, OV8865_read_cmos_sensor(GAIN_RL_ADDR));
    OTPLOG("update_awb_gain, %x = %x\n",GAIN_GH_ADDR, OV8865_read_cmos_sensor(GAIN_GH_ADDR));
    OTPLOG("update_awb_gain, %x = %x\n",GAIN_GL_ADDR, OV8865_read_cmos_sensor(GAIN_GL_ADDR));
    OTPLOG("update_awb_gain, %x = %x\n",GAIN_BH_ADDR, OV8865_read_cmos_sensor(GAIN_BH_ADDR));
    OTPLOG("update_awb_gain, %x = %x\n",GAIN_BL_ADDR, OV8865_read_cmos_sensor(GAIN_BL_ADDR));
	
	return 0;
}

// call this function after OV8858 initialization
// otp_ptr: pointer of otp_struct
static int update_lenc(struct otp_struct * otp_ptr)
{
	int i, temp;
	temp = 0x80|OV8865_read_cmos_sensor(0x5000);
	OV8865_write_cmos_sensor(0x5000, temp);

	for(i=0;i<LCS_BUF_SIZE;i++) {
		OV8865_write_cmos_sensor(0x5800 + i, (*otp_ptr).lenc[i]);
	}

	for(i=0;i<LCS_BUF_SIZE;i++){
		OTPLOG("update_lenc, 0x5800 + %d = %x (%x)\n", i,OV8865_read_cmos_sensor((0x5800)+i),(*otp_ptr).lenc[i]);
	}

	return 0;
}

// call this function after OV8858 initialization
// return value: 0 update success
//		1, no OTP


static int update_otp_info(){


	int otp_index;

	OTPLOG("update_otp_info\n");
	otp_index = check_otp_info();

	if(otp_index<=0)
	{	
		// no valid wb OTP data
		return 1;
	}	

	read_otp_info(otp_index, &current_otp);

	return 0;

}


static int update_otp_wb()
{
	int otp_index;
	int R_gain, G_gain, B_gain, G_gain_R, G_gain_B;
	int rg,bg;
	OTPLOG("update_otp_wb\n");


	// R/G and B/G of current camera module is read out from sensor OTP
	// check first OTP with valid data
	
	otp_index = check_otp_wb();
	if(otp_index<=0)
	{	
		// no valid wb OTP data
		return 1;
	}	

	read_otp_wb(otp_index, &current_otp);



	if(current_otp.light_rg==0) {
		// no light source information in OTP, light factor = 1
		rg = current_otp.rg_ratio;
	}
	else {
		rg = current_otp.rg_ratio * (current_otp.light_rg +512) /1024;
	}
	
	if(current_otp.light_bg==0) {
		// not light source information in OTP, light factor = 1
		bg = current_otp.bg_ratio;
	}
	else {
		bg = current_otp.bg_ratio * (current_otp.light_bg +512) /1024;
	}

    OTPLOG("OV8858_Upate_Otp_WB, r/g:0x%x, b/g:0x%x golden_rg:0x%x, golden_bg:0x%x\n", rg, bg,RG_Ratio_Typical,BG_Ratio_Typical);

	//calculate G gain
	//0x400 = 1x gain
	if(bg < BG_Ratio_Typical) {
		if (rg< RG_Ratio_Typical) {
			// current_otp.bg_ratio < BG_Ratio_typical &&  
			// current_otp.rg_ratio < RG_Ratio_typical
   			G_gain = GAIN_DEFAULT_VALUE;
			B_gain = GAIN_DEFAULT_VALUE * BG_Ratio_Typical / bg;
    		R_gain = GAIN_DEFAULT_VALUE * RG_Ratio_Typical / rg; 
		}
		else {
			// current_otp.bg_ratio < BG_Ratio_typical &&  
			// current_otp.rg_ratio >= RG_Ratio_typical
    		R_gain = GAIN_DEFAULT_VALUE;
   	 		G_gain = GAIN_DEFAULT_VALUE * rg / RG_Ratio_Typical;
    		B_gain = G_gain * BG_Ratio_Typical /bg;
		}
	}
	else {
		if (rg < RG_Ratio_Typical) {
			// current_otp.bg_ratio >= BG_Ratio_typical &&  
			// current_otp.rg_ratio < RG_Ratio_typical
    		B_gain = GAIN_DEFAULT_VALUE;
    		G_gain = GAIN_DEFAULT_VALUE * bg / BG_Ratio_Typical;
    		R_gain = G_gain * RG_Ratio_Typical / rg;
		}
		else {
			// current_otp.bg_ratio >= BG_Ratio_typical &&  
			// current_otp.rg_ratio >= RG_Ratio_typical
    		G_gain_B = GAIN_DEFAULT_VALUE * bg / BG_Ratio_Typical;
   	 		G_gain_R = GAIN_DEFAULT_VALUE * rg / RG_Ratio_Typical;

    		if(G_gain_B > G_gain_R ) {
        				B_gain = GAIN_DEFAULT_VALUE;
        				G_gain = G_gain_B;
 	     			R_gain = G_gain * RG_Ratio_Typical /rg;
  			}
    		else {
        			R_gain = GAIN_DEFAULT_VALUE;
       				G_gain = G_gain_R;
        			B_gain = G_gain * BG_Ratio_Typical / bg;
			}
    	}    
	}

	update_awb_gain(R_gain, G_gain, B_gain);

	return 0;

}


static int update_otp_lenc()
{
	int otp_index;

	// R/G and B/G of current camera module is read out from sensor OTP
	// check first OTP with valid data
	OTPLOG("update_lenc_wb\n");
	otp_index = check_otp_lenc();
	if(otp_index<=0)
	{	
		// no valid wb OTP data
		return 1;
	}	
	read_otp_lenc(otp_index, &current_otp);

	update_lenc(&current_otp);
	return 0;
}


#endif



void OV8865_Init_Para(void)
{

	spin_lock(&ov8865mipiraw_drv_lock);
	ov8865.sensorMode = SENSOR_MODE_INIT;
	ov8865.OV8865AutoFlickerMode = KAL_FALSE;
	ov8865.OV8865VideoMode = KAL_FALSE;
	ov8865.DummyLines= 0;
	ov8865.DummyPixels= 0;
	ov8865.pvPclk =  (7440); 
	ov8865.videoPclk = (14880);
	ov8865.capPclk = (14880);

	ov8865.shutter = 0x4C00;
	ov8865.ispBaseGain = BASEGAIN;
	ov8865.sensorGlobalGain = 0x0200;
	spin_unlock(&ov8865mipiraw_drv_lock);
}

kal_uint32 GetOv8865LineLength(void)
{
	kal_uint32 OV8865_line_length = 0;
	if ( SENSOR_MODE_PREVIEW == ov8865.sensorMode )  
	{
		OV8865_line_length = OV8865_PV_PERIOD_PIXEL_NUMS + ov8865.DummyPixels;
	}
	else if( SENSOR_MODE_VIDEO == ov8865.sensorMode ) 
	{
		OV8865_line_length = OV8865_VIDEO_PERIOD_PIXEL_NUMS + ov8865.DummyPixels;
	}
	else
	{
		OV8865_line_length = OV8865_FULL_PERIOD_PIXEL_NUMS + ov8865.DummyPixels;
	}

    return OV8865_line_length;

}


kal_uint32 GetOv8865FrameLength(void)
{
	kal_uint32 OV8865_frame_length = 0;

	if ( SENSOR_MODE_PREVIEW == ov8865.sensorMode )  
	{
		OV8865_frame_length = OV8865_PV_PERIOD_LINE_NUMS + ov8865.DummyLines ;
	}
	else if( SENSOR_MODE_VIDEO == ov8865.sensorMode ) 
	{
		OV8865_frame_length = OV8865_VIDEO_PERIOD_LINE_NUMS + ov8865.DummyLines ;
	}
	else
	{
		OV8865_frame_length = OV8865_FULL_PERIOD_LINE_NUMS + ov8865.DummyLines ;
	}

	return OV8865_frame_length;
}


kal_uint32 OV8865_CalcExtra_For_ShutterMargin(kal_uint32 shutter_value,kal_uint32 shutterLimitation)
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


kal_uint32 OV8865_CalcFrameLength_For_AutoFlicker(void)
{

    kal_uint32 AutoFlicker_min_framelength = 0;

	switch(OV8865CurrentScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			AutoFlicker_min_framelength = (ov8865.capPclk*10000) /(OV8865_FULL_PERIOD_LINE_NUMS + ov8865.DummyPixels)/OV8865_AUTOFLICKER_OFFSET_30*10 ;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			if(OV8865_VIDEO_MODE_TARGET_FPS==30)
			{
				AutoFlicker_min_framelength = (ov8865.videoPclk*10000) /(OV8865_VIDEO_PERIOD_PIXEL_NUMS + ov8865.DummyPixels)/OV8865_AUTOFLICKER_OFFSET_30*10 ;
			}
			else if(OV8865_VIDEO_MODE_TARGET_FPS==15)
			{
				AutoFlicker_min_framelength = (ov8865.videoPclk*10000) /(OV8865_VIDEO_PERIOD_PIXEL_NUMS + ov8865.DummyPixels)/OV8865_AUTOFLICKER_OFFSET_15*10 ;
			}
			else
			{
				AutoFlicker_min_framelength = OV8865_VIDEO_PERIOD_LINE_NUMS + ov8865.DummyLines;
			}
			break;
			
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			AutoFlicker_min_framelength = (ov8865.pvPclk*10000) /(OV8865_PV_PERIOD_PIXEL_NUMS + ov8865.DummyPixels)/OV8865_AUTOFLICKER_OFFSET_30*10 ;
			break;
	}

	//OV8865DB("AutoFlicker_min_framelength =%d,OV8865CurrentScenarioId =%d\n", AutoFlicker_min_framelength,OV8865CurrentScenarioId);

	return AutoFlicker_min_framelength;

}


void OV8865_write_shutter(kal_uint32 shutter)
{
	kal_uint32 min_framelength = OV8865_PV_PERIOD_PIXEL_NUMS, max_shutter=0;
	kal_uint32 line_length = 0;
	kal_uint32 frame_length = 0;
	unsigned long flags;

	kal_uint32 read_shutter_1 = 0;
	kal_uint32 read_shutter_2 = 0;
	kal_uint32 read_shutter_3 = 0;

    if(shutter > 0x73c8)//400ms for capture SaturationGain
		shutter = 0x73c8;
	
    line_length  = GetOv8865LineLength();
	frame_length = GetOv8865FrameLength();
	
	max_shutter  = frame_length-OV8865_SHUTTER_MARGIN;

    frame_length = frame_length + OV8865_CalcExtra_For_ShutterMargin(shutter,max_shutter);
	


	if(ov8865.OV8865AutoFlickerMode == KAL_TRUE)
	{
        min_framelength = OV8865_CalcFrameLength_For_AutoFlicker();

        if(frame_length < min_framelength)
			frame_length = min_framelength;
	}
	

	spin_lock_irqsave(&ov8865mipiraw_drv_lock,flags);
	OV8865_FeatureControl_PERIOD_PixelNum = line_length;
	OV8865_FeatureControl_PERIOD_LineNum = frame_length;
	spin_unlock_irqrestore(&ov8865mipiraw_drv_lock,flags);

	//Set total frame length
	OV8865_write_cmos_sensor(0x380e, (frame_length >> 8) & 0xFF);
	OV8865_write_cmos_sensor(0x380f, frame_length & 0xFF);
	
	//Set shutter 
	OV8865_write_cmos_sensor(0x3500, (shutter>>12) & 0x0F);
	OV8865_write_cmos_sensor(0x3501, (shutter>>4) & 0xFF);
	OV8865_write_cmos_sensor(0x3502, (shutter<<4) & 0xF0);	

	OV8865DB("ov8825 write shutter=%x, line_length=%x, frame_length=%x\n", shutter, line_length, frame_length);

}


static kal_uint16 OV8865Reg2Gain(const kal_uint16 iReg)
{
    kal_uint16 iGain =0; 

	iGain = iReg*BASEGAIN/OV8865_GAIN_BASE;
	return iGain;
	
}

static kal_uint16 OV8865Gain2Reg(const kal_uint32 Gain)
{
    kal_uint32 iReg = 0x0000;

	iReg = Gain*OV8865_GAIN_BASE/BASEGAIN;

    return iReg;

}

void write_OV8865_gain(kal_uint16 gain)
{
	//kal_uint16 read_gain=0;

	OV8865_write_cmos_sensor(0x3508,(gain>>8));
	OV8865_write_cmos_sensor(0x3509,(gain&0xff));

	//read_gain=(((OV8865_read_cmos_sensor(0x3508)&0x1F) << 8) | OV8865_read_cmos_sensor(0x3509));
	//OV8865DB("[OV8865_SetGain]0x3508|0x3509=0x%x \n",read_gain);

	return;
}

void OV8865_SetGain(UINT16 iGain)
{
	unsigned long flags;
	spin_lock_irqsave(&ov8865mipiraw_drv_lock,flags);

	OV8865DB("OV8865_SetGain iGain = %d :\n ",iGain);

	ov8865.realGain = iGain;
	ov8865.sensorGlobalGain = OV8865Gain2Reg(iGain);
	spin_unlock_irqrestore(&ov8865mipiraw_drv_lock,flags);

	write_OV8865_gain(ov8865.sensorGlobalGain);
	OV8865DB(" [OV8865_SetGain]ov8865.sensorGlobalGain=0x%x,ov8865.realGain =%x",ov8865.sensorGlobalGain,
		ov8865.realGain); 

	//temperature test
	//OV8865_write_cmos_sensor(0x4d12,0x01);
	//OV8865DB("Temperature read_reg  0x4d13  =%x \n",OV8865_read_cmos_sensor(0x4d13));
}   

kal_uint16 read_OV8865_gain(void)
{
	kal_uint16 read_gain=0;

	read_gain=(((OV8865_read_cmos_sensor(0x3508)&0x1F) << 8) | OV8865_read_cmos_sensor(0x3509));

	spin_lock(&ov8865mipiraw_drv_lock);
	ov8865.sensorGlobalGain = read_gain;
	ov8865.realGain = OV8865Reg2Gain(ov8865.sensorGlobalGain);
	spin_unlock(&ov8865mipiraw_drv_lock);

	OV8865DB("ov8865.sensorGlobalGain=0x%x,ov8865.realGain=%d\n",ov8865.sensorGlobalGain,ov8865.realGain);

	return ov8865.sensorGlobalGain;
}  


#if 1
void OV8865_camera_para_to_sensor(void)
{
    kal_uint32    i;
    for(i=0; 0xFFFFFFFF!=OV8865SensorReg[i].Addr; i++)
    {
        OV8865_write_cmos_sensor(OV8865SensorReg[i].Addr, OV8865SensorReg[i].Para);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=OV8865SensorReg[i].Addr; i++)
    {
        OV8865_write_cmos_sensor(OV8865SensorReg[i].Addr, OV8865SensorReg[i].Para);
    }
    for(i=FACTORY_START_ADDR; i<FACTORY_END_ADDR; i++)
    {
        OV8865_write_cmos_sensor(OV8865SensorCCT[i].Addr, OV8865SensorCCT[i].Para);
    }
}

void OV8865_sensor_to_camera_para(void)
{
    kal_uint32    i, temp_data;
    for(i=0; 0xFFFFFFFF!=OV8865SensorReg[i].Addr; i++)
    {
         temp_data = OV8865_read_cmos_sensor(OV8865SensorReg[i].Addr);
		 spin_lock(&ov8865mipiraw_drv_lock);
		 OV8865SensorReg[i].Para =temp_data;
		 spin_unlock(&ov8865mipiraw_drv_lock);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=OV8865SensorReg[i].Addr; i++)
    {
        temp_data = OV8865_read_cmos_sensor(OV8865SensorReg[i].Addr);
		spin_lock(&ov8865mipiraw_drv_lock);
		OV8865SensorReg[i].Para = temp_data;
		spin_unlock(&ov8865mipiraw_drv_lock);
    }
}

kal_int32  OV8865_get_sensor_group_count(void)
{
    return GROUP_TOTAL_NUMS;
}

void OV8865_get_sensor_group_info(kal_uint16 group_idx, kal_int8* group_name_ptr, kal_int32* item_count_ptr)
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

void OV8865_get_sensor_item_info(kal_uint16 group_idx,kal_uint16 item_idx, MSDK_SENSOR_ITEM_INFO_STRUCT* info_ptr)
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

            temp_para= OV8865SensorCCT[temp_addr].Para;
			//temp_gain= (temp_para/ov8865.sensorBaseGain) * 1000;

            info_ptr->ItemValue=temp_gain;
            info_ptr->IsTrueFalse=KAL_FALSE;
            info_ptr->IsReadOnly=KAL_FALSE;
            info_ptr->IsNeedRestart=KAL_FALSE;
            info_ptr->Min= OV8865_MIN_ANALOG_GAIN * 1000;
            info_ptr->Max= OV8865_MAX_ANALOG_GAIN * 1000;
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



kal_bool OV8865_set_sensor_item_info(kal_uint16 group_idx, kal_uint16 item_idx, kal_int32 ItemValue)
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

		  spin_lock(&ov8865mipiraw_drv_lock);
          OV8865SensorCCT[temp_addr].Para = temp_para;
		  spin_unlock(&ov8865mipiraw_drv_lock);
          OV8865_write_cmos_sensor(OV8865SensorCCT[temp_addr].Addr,temp_para);

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
					spin_lock(&ov8865mipiraw_drv_lock);
                    OV8865_FAC_SENSOR_REG=ItemValue;
					spin_unlock(&ov8865mipiraw_drv_lock);
                    break;
                case 1:
                    OV8865_write_cmos_sensor(OV8865_FAC_SENSOR_REG,ItemValue);
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


static void OV8865_SetDummy( const kal_uint32 iPixels, const kal_uint32 iLines )
{
	kal_uint32 line_length = 0;
	kal_uint32 frame_length = 0;

	if ( SENSOR_MODE_PREVIEW == ov8865.sensorMode )
	{
		line_length = OV8865_PV_PERIOD_PIXEL_NUMS + iPixels;
		frame_length = OV8865_PV_PERIOD_LINE_NUMS + iLines;
	}
	else if( SENSOR_MODE_VIDEO== ov8865.sensorMode )
	{
		line_length = OV8865_VIDEO_PERIOD_PIXEL_NUMS + iPixels;
		frame_length = OV8865_VIDEO_PERIOD_LINE_NUMS + iLines;
	}
	else
	{
		line_length = OV8865_FULL_PERIOD_PIXEL_NUMS + iPixels;
		frame_length = OV8865_FULL_PERIOD_LINE_NUMS + iLines;
	}

	spin_lock(&ov8865mipiraw_drv_lock);
	OV8865_FeatureControl_PERIOD_PixelNum = line_length;
	OV8865_FeatureControl_PERIOD_LineNum = frame_length;
	spin_unlock(&ov8865mipiraw_drv_lock);

	//Set total frame length
	OV8865_write_cmos_sensor(0x380e, (frame_length >> 8) & 0xFF);
	OV8865_write_cmos_sensor(0x380f, frame_length & 0xFF);
	//Set total line length
	OV8865_write_cmos_sensor(0x380c, (line_length >> 8) & 0xFF);
	OV8865_write_cmos_sensor(0x380d, line_length & 0xFF);

}   


void OV8865PreviewSetting(void)
{
	OV8865DB(" OV8865PreviewSetting_4lane enter\n");

	OV8865_write_cmos_sensor(0x0100,0x00);//; software standby

	OV8865_write_cmos_sensor(0x030d,0x1f);// ; PLL	  
	OV8865_write_cmos_sensor(0x030f,0x09);//; PLL
	OV8865_write_cmos_sensor(0x3018,0x72);//
	OV8865_write_cmos_sensor(0x3106,0x01);//
	
	//OV8865_write_cmos_sensor(0x3501,0x28);//; expouere H
	//OV8865_write_cmos_sensor(0x3502,0x90);//; exposure L
	
	OV8865_write_cmos_sensor(0x3700,0x18);//; sensor control
	OV8865_write_cmos_sensor(0x3701,0x0c);//
	OV8865_write_cmos_sensor(0x3702,0x28);//
	OV8865_write_cmos_sensor(0x3703,0x19);//
	OV8865_write_cmos_sensor(0x3704,0x14);//
	OV8865_write_cmos_sensor(0x3706,0x28);//
	OV8865_write_cmos_sensor(0x3707,0x04);//
	OV8865_write_cmos_sensor(0x3708,0x24);//
	OV8865_write_cmos_sensor(0x3709,0x40);//
	OV8865_write_cmos_sensor(0x370a,0x00);//
	OV8865_write_cmos_sensor(0x370b,0xa8);//
	OV8865_write_cmos_sensor(0x370c,0x04);//
	OV8865_write_cmos_sensor(0x3718,0x12);//
	OV8865_write_cmos_sensor(0x3712,0x42);//
	OV8865_write_cmos_sensor(0x371e,0x19);//
	OV8865_write_cmos_sensor(0x371f,0x40);//
	OV8865_write_cmos_sensor(0x3720,0x05);//
	OV8865_write_cmos_sensor(0x3721,0x05);//
	OV8865_write_cmos_sensor(0x3724,0x02);//
	OV8865_write_cmos_sensor(0x3725,0x02);//
	OV8865_write_cmos_sensor(0x3726,0x06);//
	OV8865_write_cmos_sensor(0x3728,0x05);//
	OV8865_write_cmos_sensor(0x3729,0x02);//
	OV8865_write_cmos_sensor(0x372a,0x03);//
	OV8865_write_cmos_sensor(0x372b,0x53);//
	OV8865_write_cmos_sensor(0x372c,0xa3);//
	OV8865_write_cmos_sensor(0x372d,0x53);//
	OV8865_write_cmos_sensor(0x372e,0x06);//
	OV8865_write_cmos_sensor(0x372f,0x10);//
	OV8865_write_cmos_sensor(0x3730,0x01);//
	OV8865_write_cmos_sensor(0x3731,0x06);//
	OV8865_write_cmos_sensor(0x3732,0x14);//
	OV8865_write_cmos_sensor(0x3736,0x20);//
	OV8865_write_cmos_sensor(0x373a,0x02);//
	OV8865_write_cmos_sensor(0x373b,0x0c);//
	OV8865_write_cmos_sensor(0x373c,0x0a);//
	OV8865_write_cmos_sensor(0x373e,0x03);//
	OV8865_write_cmos_sensor(0x375a,0x06);//
	OV8865_write_cmos_sensor(0x375b,0x13);//
	OV8865_write_cmos_sensor(0x375d,0x02);//
	OV8865_write_cmos_sensor(0x375f,0x14);//
	OV8865_write_cmos_sensor(0x3767,0x04);//
	OV8865_write_cmos_sensor(0x3769,0x20);//
	OV8865_write_cmos_sensor(0x3772,0x23);//
	OV8865_write_cmos_sensor(0x3773,0x02);//
	OV8865_write_cmos_sensor(0x3774,0x16);//
	OV8865_write_cmos_sensor(0x3775,0x12);//
	OV8865_write_cmos_sensor(0x3776,0x08);//
	OV8865_write_cmos_sensor(0x37a0,0x44);//
	OV8865_write_cmos_sensor(0x37a1,0x3d);//
	OV8865_write_cmos_sensor(0x37a2,0x3d);//
	OV8865_write_cmos_sensor(0x37a3,0x01);//
	OV8865_write_cmos_sensor(0x37a5,0x08);//
	OV8865_write_cmos_sensor(0x37a7,0x44);//
	OV8865_write_cmos_sensor(0x37a8,0x4c);//
	OV8865_write_cmos_sensor(0x37a9,0x4c);//
	OV8865_write_cmos_sensor(0x37aa,0x44);//
	OV8865_write_cmos_sensor(0x37ab,0x2e);//
	OV8865_write_cmos_sensor(0x37ac,0x2e);//
	OV8865_write_cmos_sensor(0x37ad,0x33);//
	OV8865_write_cmos_sensor(0x37ae,0x0d);//
	OV8865_write_cmos_sensor(0x37af,0x0d);//
	OV8865_write_cmos_sensor(0x37b3,0x42);//
	OV8865_write_cmos_sensor(0x37b4,0x42);//
	OV8865_write_cmos_sensor(0x37b5,0x33);//

	OV8865_write_cmos_sensor(0x3808,0x06);//; X output size H   active 
	OV8865_write_cmos_sensor(0x3809,0x68);//; X output size L  (0x3809,0x60)
	OV8865_write_cmos_sensor(0x380a,0x04);//; Y output size H   active 
	OV8865_write_cmos_sensor(0x380b,0xc8);//; Y output size L (0x380b,0xc8)

	OV8865_write_cmos_sensor(0x380c,0x07);//; HTS H
	OV8865_write_cmos_sensor(0x380d,0x83);//; HTS L
	OV8865_write_cmos_sensor(0x380e,0x04);//; VTS H
	OV8865_write_cmos_sensor(0x380f,0xe0);//; VTS L
	
	OV8865_write_cmos_sensor(0x3813,0x04);//; ISP Y win L
	OV8865_write_cmos_sensor(0x3814,0x03);//; X inc odd
	OV8865_write_cmos_sensor(0x3821,0x67);//; hsync_en_o, fst_vbin, mirror on
	OV8865_write_cmos_sensor(0x382a,0x03);//; Y inc odd
	OV8865_write_cmos_sensor(0x382b,0x01);//; Y inc even
	OV8865_write_cmos_sensor(0x3830,0x08);//; ablc_use_num[5:1]
	OV8865_write_cmos_sensor(0x3836,0x02);//; zline_use_num[5:1]
	OV8865_write_cmos_sensor(0x3846,0x88);//; Y/X boundary pixel numbber for auto size mode
	OV8865_write_cmos_sensor(0x3f08,0x0b);//
	
	OV8865_write_cmos_sensor(0x4000,0xf1);//; our range trig en, format chg en, gan chg en, exp chg en, median en
	
	OV8865_write_cmos_sensor(0x4001,0x14);//; left 32 column, final BLC offset limitation enable
	OV8865_write_cmos_sensor(0x4020,0x01);//; anchor left start H
	OV8865_write_cmos_sensor(0x4021,0x20);//; anchor left start L
	OV8865_write_cmos_sensor(0x4022,0x01);//; anchor left end H
	OV8865_write_cmos_sensor(0x4023,0x9f);//; anchor left end L
	OV8865_write_cmos_sensor(0x4024,0x03);//; anchor right start H
	OV8865_write_cmos_sensor(0x4025,0xe0);//; anchor right start L
	OV8865_write_cmos_sensor(0x4026,0x04);//; anchor right end H
	OV8865_write_cmos_sensor(0x4027,0x5f);//; anchor right end L
	OV8865_write_cmos_sensor(0x402a,0x04);//; top black line start
	OV8865_write_cmos_sensor(0x402c,0x02);//; bottom zero line start
	OV8865_write_cmos_sensor(0x402d,0x02);//; bottom zero line number
	OV8865_write_cmos_sensor(0x402e,0x08);//; bottom black line start
	OV8865_write_cmos_sensor(0x4500,0x40);//; ADC sync control
	OV8865_write_cmos_sensor(0x4601,0x74);//; V FIFO control
	OV8865_write_cmos_sensor(0x5002,0x08);//; vario pixel off
	OV8865_write_cmos_sensor(0x5901,0x00);//
	
	OV8865_write_cmos_sensor(0x0100,0x01);//; wake up, streaming
}


void OV8865VideoSetting(void)
{
	OV8865DB(" OV8865VideoSetting_4lane enter\n");

	OV8865_write_cmos_sensor(0x0100,0x00);
	
	OV8865_write_cmos_sensor(0x030d,0x1f);// ; PLL	  ;1e
	OV8865_write_cmos_sensor(0x030f,0x04);// ; PLL
	OV8865_write_cmos_sensor(0x3018,0x72);//
	OV8865_write_cmos_sensor(0x3106,0x01);//
	
	//OV8865_write_cmos_sensor(0x3501,0x8f);//; expouere H
	//OV8865_write_cmos_sensor(0x3502,0xa0);// ; exposure L
	
	OV8865_write_cmos_sensor(0x3700,0x30);// ; sensor control
	OV8865_write_cmos_sensor(0x3701,0x18);//
	OV8865_write_cmos_sensor(0x3702,0x50);//
	OV8865_write_cmos_sensor(0x3703,0x32);//
	OV8865_write_cmos_sensor(0x3704,0x28);//
	OV8865_write_cmos_sensor(0x3706,0x50);//
	OV8865_write_cmos_sensor(0x3707,0x08);//
	OV8865_write_cmos_sensor(0x3708,0x48);//
	OV8865_write_cmos_sensor(0x3709,0x80);//
	OV8865_write_cmos_sensor(0x370a,0x01);//
	OV8865_write_cmos_sensor(0x370b,0x50);//
	OV8865_write_cmos_sensor(0x370c,0x07);//
	OV8865_write_cmos_sensor(0x3718,0x14);//
	OV8865_write_cmos_sensor(0x3712,0x44);//
	OV8865_write_cmos_sensor(0x371e,0x31);//
	OV8865_write_cmos_sensor(0x371f,0x7f);//
	OV8865_write_cmos_sensor(0x3720,0x0a);//
	OV8865_write_cmos_sensor(0x3721,0x0a);//
	OV8865_write_cmos_sensor(0x3724,0x04);//
	OV8865_write_cmos_sensor(0x3725,0x04);//
	OV8865_write_cmos_sensor(0x3726,0x0c);//
	OV8865_write_cmos_sensor(0x3728,0x0a);//
	OV8865_write_cmos_sensor(0x3729,0x03);//
	OV8865_write_cmos_sensor(0x372a,0x06);//
	OV8865_write_cmos_sensor(0x372b,0xa6);//
	OV8865_write_cmos_sensor(0x372c,0xa6);//
	OV8865_write_cmos_sensor(0x372d,0xa6);//
	OV8865_write_cmos_sensor(0x372e,0x0c);//
	OV8865_write_cmos_sensor(0x372f,0x20);//
	OV8865_write_cmos_sensor(0x3730,0x02);//
	OV8865_write_cmos_sensor(0x3731,0x0c);//
	OV8865_write_cmos_sensor(0x3732,0x28);//
	OV8865_write_cmos_sensor(0x3736,0x30);//
	OV8865_write_cmos_sensor(0x373a,0x04);//
	OV8865_write_cmos_sensor(0x373b,0x18);//
	OV8865_write_cmos_sensor(0x373c,0x14);//
	OV8865_write_cmos_sensor(0x373e,0x06);//
	OV8865_write_cmos_sensor(0x375a,0x0c);//
	OV8865_write_cmos_sensor(0x375b,0x26);//
	OV8865_write_cmos_sensor(0x375d,0x04);//
	OV8865_write_cmos_sensor(0x375f,0x28);//
	OV8865_write_cmos_sensor(0x3767,0x04);//
	OV8865_write_cmos_sensor(0x3769,0x20);//
	OV8865_write_cmos_sensor(0x3772,0x46);//
	OV8865_write_cmos_sensor(0x3773,0x04);//
	OV8865_write_cmos_sensor(0x3774,0x2c);//
	OV8865_write_cmos_sensor(0x3775,0x13);//
	OV8865_write_cmos_sensor(0x3776,0x10);//
	OV8865_write_cmos_sensor(0x37a0,0x88);//
	OV8865_write_cmos_sensor(0x37a1,0x7a);//
	OV8865_write_cmos_sensor(0x37a2,0x7a);//
	OV8865_write_cmos_sensor(0x37a3,0x02);//
	OV8865_write_cmos_sensor(0x37a5,0x09);//
	OV8865_write_cmos_sensor(0x37a7,0x88);//
	OV8865_write_cmos_sensor(0x37a8,0x98);//
	OV8865_write_cmos_sensor(0x37a9,0x98);//
	OV8865_write_cmos_sensor(0x37aa,0x88);//
	OV8865_write_cmos_sensor(0x37ab,0x5c);//
	OV8865_write_cmos_sensor(0x37ac,0x5c);//
	OV8865_write_cmos_sensor(0x37ad,0x55);//
	OV8865_write_cmos_sensor(0x37ae,0x19);//
	OV8865_write_cmos_sensor(0x37af,0x19);//
	OV8865_write_cmos_sensor(0x37b3,0x84);//
	OV8865_write_cmos_sensor(0x37b4,0x84);//
	OV8865_write_cmos_sensor(0x37b5,0x66);//
	
	OV8865_write_cmos_sensor(0x3808,0x0c);// ; X output size H
	OV8865_write_cmos_sensor(0x3809,0xc8);// ; X output size L  (0x3809,0xc0)
	OV8865_write_cmos_sensor(0x380a,0x07);// ; Y output size H
	OV8865_write_cmos_sensor(0x380b,0x2c);// ; Y output size L  (0x380b,0x2c)
	
	OV8865_write_cmos_sensor(0x380c,0x0a);// ; HTS H
	OV8865_write_cmos_sensor(0x380d,0x16);// ; HTS L
	OV8865_write_cmos_sensor(0x380e,0x07);// ; VTS H
	OV8865_write_cmos_sensor(0x380f,0x42);// ; VTS L
	
	OV8865_write_cmos_sensor(0x3813,0x02);// ; ISP Y win L
	OV8865_write_cmos_sensor(0x3814,0x01);// ; X inc odd
	OV8865_write_cmos_sensor(0x3821,0x46);// ; hsync_en_o, fst_vbin, mirror on
	OV8865_write_cmos_sensor(0x382a,0x01);// ; Y inc odd
	OV8865_write_cmos_sensor(0x382b,0x01);// ; Y inc even
	OV8865_write_cmos_sensor(0x3830,0x04);// ; ablc_use_num[5:1]
	OV8865_write_cmos_sensor(0x3836,0x01);// ; zline_use_num[5:1]
	OV8865_write_cmos_sensor(0x3846,0x48);// ; Y/X boundary pixel numbber for auto size mode
	OV8865_write_cmos_sensor(0x3f08,0x16);//
	
	OV8865_write_cmos_sensor(0x4000,0xf1);//; our range trig en, format chg en, gan chg en, exp chg en, median en
	
	OV8865_write_cmos_sensor(0x4001,0x04);// ; left 32 column, final BLC offset limitation enable
	OV8865_write_cmos_sensor(0x4020,0x02);// ; anchor left start H
	OV8865_write_cmos_sensor(0x4021,0x40);// ; anchor left start L
	OV8865_write_cmos_sensor(0x4022,0x03);// ; anchor left end H
	OV8865_write_cmos_sensor(0x4023,0x3f);// ; anchor left end L
	OV8865_write_cmos_sensor(0x4024,0x07);// ; anchor right start H
	OV8865_write_cmos_sensor(0x4025,0xc0);// ; anchor right start L
	OV8865_write_cmos_sensor(0x4026,0x08);// ; anchor right end H
	OV8865_write_cmos_sensor(0x4027,0xbf);// ; anchor right end L
	OV8865_write_cmos_sensor(0x402a,0x04);// ; top black line start
	OV8865_write_cmos_sensor(0x402c,0x02);// ; bottom zero line start
	OV8865_write_cmos_sensor(0x402d,0x02);// ; bottom zero line number
	OV8865_write_cmos_sensor(0x402e,0x08);// ; bottom black line start
	OV8865_write_cmos_sensor(0x4500,0x68);// ; ADC sync control
	OV8865_write_cmos_sensor(0x4601,0x10);// ; V FIFO control
	OV8865_write_cmos_sensor(0x5002,0x08);// ; vario pixel off
	OV8865_write_cmos_sensor(0x5901,0x00);//
	
	OV8865_write_cmos_sensor(0x0100,0x01);//; wake up, streaming

}


void OV8865CaptureSetting(void)
{
	OV8865DB("OV8865CaptureSetting_4lane enter\n");

	OV8865_write_cmos_sensor(0x0100,0x00);

	OV8865_write_cmos_sensor(0x030d,0x1f);// ; PLL	  ;1e
	OV8865_write_cmos_sensor(0x030f,0x04);// ; PLL
	OV8865_write_cmos_sensor(0x3018,0x72);//
	OV8865_write_cmos_sensor(0x3106,0x01);//
	
	//OV8865_write_cmos_sensor(0x3501,0x8f);// ; expouere H
	//OV8865_write_cmos_sensor(0x3502,0xa0);// ; exposure L
	
	OV8865_write_cmos_sensor(0x3700,0x30);// ; sensor control
	OV8865_write_cmos_sensor(0x3701,0x18);//
	OV8865_write_cmos_sensor(0x3702,0x50);//
	OV8865_write_cmos_sensor(0x3703,0x32);//
	OV8865_write_cmos_sensor(0x3704,0x28);//
	OV8865_write_cmos_sensor(0x3706,0x50);//
	OV8865_write_cmos_sensor(0x3707,0x08);//
	OV8865_write_cmos_sensor(0x3708,0x48);//
	OV8865_write_cmos_sensor(0x3709,0x80);//
	OV8865_write_cmos_sensor(0x370a,0x01);//
	OV8865_write_cmos_sensor(0x370b,0x50);//
	OV8865_write_cmos_sensor(0x370c,0x07);//
	OV8865_write_cmos_sensor(0x3718,0x14);//
	OV8865_write_cmos_sensor(0x3712,0x44);//
	OV8865_write_cmos_sensor(0x371e,0x31);//
	OV8865_write_cmos_sensor(0x371f,0x7f);//
	OV8865_write_cmos_sensor(0x3720,0x0a);//
	OV8865_write_cmos_sensor(0x3721,0x0a);//
	OV8865_write_cmos_sensor(0x3724,0x04);//
	OV8865_write_cmos_sensor(0x3725,0x04);//
	OV8865_write_cmos_sensor(0x3726,0x0c);//
	OV8865_write_cmos_sensor(0x3728,0x0a);//
	OV8865_write_cmos_sensor(0x3729,0x03);//
	OV8865_write_cmos_sensor(0x372a,0x06);//
	OV8865_write_cmos_sensor(0x372b,0xa6);//
	OV8865_write_cmos_sensor(0x372c,0xa6);//
	OV8865_write_cmos_sensor(0x372d,0xa6);//
	OV8865_write_cmos_sensor(0x372e,0x0c);//
	OV8865_write_cmos_sensor(0x372f,0x20);//
	OV8865_write_cmos_sensor(0x3730,0x02);//
	OV8865_write_cmos_sensor(0x3731,0x0c);//
	OV8865_write_cmos_sensor(0x3732,0x28);//
	OV8865_write_cmos_sensor(0x3736,0x30);//
	OV8865_write_cmos_sensor(0x373a,0x04);//
	OV8865_write_cmos_sensor(0x373b,0x18);//
	OV8865_write_cmos_sensor(0x373c,0x14);//
	OV8865_write_cmos_sensor(0x373e,0x06);//
	OV8865_write_cmos_sensor(0x375a,0x0c);//
	OV8865_write_cmos_sensor(0x375b,0x26);//
	OV8865_write_cmos_sensor(0x375d,0x04);//
	OV8865_write_cmos_sensor(0x375f,0x28);//
	OV8865_write_cmos_sensor(0x3767,0x04);//
	OV8865_write_cmos_sensor(0x3769,0x20);//
	OV8865_write_cmos_sensor(0x3772,0x46);//
	OV8865_write_cmos_sensor(0x3773,0x04);//
	OV8865_write_cmos_sensor(0x3774,0x2c);//
	OV8865_write_cmos_sensor(0x3775,0x13);//
	OV8865_write_cmos_sensor(0x3776,0x10);//
	OV8865_write_cmos_sensor(0x37a0,0x88);//
	OV8865_write_cmos_sensor(0x37a1,0x7a);//
	OV8865_write_cmos_sensor(0x37a2,0x7a);//
	OV8865_write_cmos_sensor(0x37a3,0x02);//
	OV8865_write_cmos_sensor(0x37a5,0x09);//
	OV8865_write_cmos_sensor(0x37a7,0x88);//
	OV8865_write_cmos_sensor(0x37a8,0x98);//
	OV8865_write_cmos_sensor(0x37a9,0x98);//
	OV8865_write_cmos_sensor(0x37aa,0x88);//
	OV8865_write_cmos_sensor(0x37ab,0x5c);//
	OV8865_write_cmos_sensor(0x37ac,0x5c);//
	OV8865_write_cmos_sensor(0x37ad,0x55);//
	OV8865_write_cmos_sensor(0x37ae,0x19);//
	OV8865_write_cmos_sensor(0x37af,0x19);//
	OV8865_write_cmos_sensor(0x37b3,0x84);//
	OV8865_write_cmos_sensor(0x37b4,0x84);//
	OV8865_write_cmos_sensor(0x37b5,0x66);//

	OV8865_write_cmos_sensor(0x3808,0x0c);// ; X output size H
	OV8865_write_cmos_sensor(0x3809,0xc8);// ; X output size L (0x3809,0xc0)
	OV8865_write_cmos_sensor(0x380a,0x09);// ; Y output size H
	OV8865_write_cmos_sensor(0x380b,0x90);// ; Y output size L (0x380b,0x90)
	
	OV8865_write_cmos_sensor(0x380c,0x07);// ; HTS H //30fps
	OV8865_write_cmos_sensor(0x380d,0xd8);// ; HTS L 

	OV8865_write_cmos_sensor(0x380e,0x09);//; VTS H
	OV8865_write_cmos_sensor(0x380f,0xd8);//; VTS L
	
	OV8865_write_cmos_sensor(0x3813,0x02);// ; ISP Y win L
	OV8865_write_cmos_sensor(0x3814,0x01);// ; X inc odd
	OV8865_write_cmos_sensor(0x3821,0x46);// ; hsync_en_o, fst_vbin, mirror on
	OV8865_write_cmos_sensor(0x382a,0x01);// ; Y inc odd
	OV8865_write_cmos_sensor(0x382b,0x01);// ; Y inc even
	OV8865_write_cmos_sensor(0x3830,0x04);// ; ablc_use_num[5:1]
	OV8865_write_cmos_sensor(0x3836,0x01);// ; zline_use_num[5:1]
	OV8865_write_cmos_sensor(0x3846,0x48);// ; Y/X boundary pixel numbber for auto size mode
	OV8865_write_cmos_sensor(0x3f08,0x16);//
	
	OV8865_write_cmos_sensor(0x4000,0xf1);//; our range trig en, format chg en, gan chg en, exp chg en, median en
	
	OV8865_write_cmos_sensor(0x4001,0x04);// ; left 32 column, final BLC offset limitation enable
	OV8865_write_cmos_sensor(0x4020,0x02);// ; anchor left start H
	OV8865_write_cmos_sensor(0x4021,0x40);// ; anchor left start L
	OV8865_write_cmos_sensor(0x4022,0x03);// ; anchor left end H
	OV8865_write_cmos_sensor(0x4023,0x3f);// ; anchor left end L
	OV8865_write_cmos_sensor(0x4024,0x07);// ; anchor right start H
	OV8865_write_cmos_sensor(0x4025,0xc0);// ; anchor right start L
	OV8865_write_cmos_sensor(0x4026,0x08);// ; anchor right end H
	OV8865_write_cmos_sensor(0x4027,0xbf);// ; anchor right end L
	OV8865_write_cmos_sensor(0x402a,0x04);// ; top black line start
	OV8865_write_cmos_sensor(0x402c,0x02);// ; bottom zero line start  
	OV8865_write_cmos_sensor(0x402d,0x02);// ; bottom zero line number
	OV8865_write_cmos_sensor(0x402e,0x08);// ; bottom black line start
	OV8865_write_cmos_sensor(0x4500,0x68);// ; ADC sync control
	OV8865_write_cmos_sensor(0x4601,0x10);// ; V FIFO control
	OV8865_write_cmos_sensor(0x5002,0x08);// ; vario pixel off
	OV8865_write_cmos_sensor(0x5901,0x00);//
	
	OV8865_write_cmos_sensor(0x0100,0x01);//; wake up, streaming
	
}


static void OV8865_Sensor_Init(void)
{
	OV8865DB("OV8865_Sensor_Init_4lane enter\n");

	OV8865_write_cmos_sensor(0x0103,0x01);// ; software reset
	mdelay(10);
	OV8865_write_cmos_sensor(0x0100,0x00);// ; software standby
	OV8865_write_cmos_sensor(0x0100,0x00);//
	OV8865_write_cmos_sensor(0x0100,0x00);//
	OV8865_write_cmos_sensor(0x0100,0x00);//
	OV8865_write_cmos_sensor(0x3638,0xff);// ; analog control

	OV8865_write_cmos_sensor(0x0302,0x1f);// ; PLL
	
	OV8865_write_cmos_sensor(0x0303,0x00);// ; PLL
	OV8865_write_cmos_sensor(0x0304,0x03);// ; PLL
	OV8865_write_cmos_sensor(0x030d,0x1f);// ; PLL	  ;1e
	OV8865_write_cmos_sensor(0x030e,0x00);// ; PLL
	OV8865_write_cmos_sensor(0x030f,0x09);// ; PLL
	OV8865_write_cmos_sensor(0x0312,0x01);// ; PLL
	OV8865_write_cmos_sensor(0x031e,0x0c);// ; PLL
	OV8865_write_cmos_sensor(0x3015,0x01);// ; clock Div
	OV8865_write_cmos_sensor(0x3018,0x72);// ; MIPI 4 lane
	OV8865_write_cmos_sensor(0x3020,0x93);// ; clock normal, pclk/1
	OV8865_write_cmos_sensor(0x3022,0x01);// ; pd_mini enable when rst_sync
	OV8865_write_cmos_sensor(0x3031,0x0a);// ; 10-bit
	OV8865_write_cmos_sensor(0x3106,0x01);// ; PLL
	OV8865_write_cmos_sensor(0x3305,0xf1);//
	OV8865_write_cmos_sensor(0x3308,0x00);//
	OV8865_write_cmos_sensor(0x3309,0x28);//
	OV8865_write_cmos_sensor(0x330a,0x00);//
	OV8865_write_cmos_sensor(0x330b,0x20);//
	OV8865_write_cmos_sensor(0x330c,0x00);//
	OV8865_write_cmos_sensor(0x330d,0x00);//
	OV8865_write_cmos_sensor(0x330e,0x00);//
	OV8865_write_cmos_sensor(0x330f,0x40);//
	OV8865_write_cmos_sensor(0x3307,0x04);//
	OV8865_write_cmos_sensor(0x3604,0x04);// ; analog control
	OV8865_write_cmos_sensor(0x3602,0x30);//
	OV8865_write_cmos_sensor(0x3605,0x00);//
	OV8865_write_cmos_sensor(0x3607,0x20);//
	OV8865_write_cmos_sensor(0x3608,0x11);//
	OV8865_write_cmos_sensor(0x3609,0x68);//
	OV8865_write_cmos_sensor(0x360a,0x40);//
	OV8865_write_cmos_sensor(0x360c,0xdd);//
	OV8865_write_cmos_sensor(0x360e,0x0c);//
	OV8865_write_cmos_sensor(0x3610,0x07);//
	OV8865_write_cmos_sensor(0x3612,0x89);//
	OV8865_write_cmos_sensor(0x3614,0x28);//
	OV8865_write_cmos_sensor(0x3617,0x40);//
	OV8865_write_cmos_sensor(0x3618,0x5a);//
	OV8865_write_cmos_sensor(0x3619,0x9b);//
	OV8865_write_cmos_sensor(0x361c,0x00);//
	OV8865_write_cmos_sensor(0x361d,0x60);//
	OV8865_write_cmos_sensor(0x3631,0x60);//
	OV8865_write_cmos_sensor(0x3633,0x10);//
	OV8865_write_cmos_sensor(0x3634,0x10);//
	OV8865_write_cmos_sensor(0x3635,0x10);//
	OV8865_write_cmos_sensor(0x3636,0x0a);//
	OV8865_write_cmos_sensor(0x3641,0x55);// ; MIPI settings
	OV8865_write_cmos_sensor(0x3646,0x86);// ; MIPI settings
	OV8865_write_cmos_sensor(0x3647,0x27);// ; MIPI settings
	OV8865_write_cmos_sensor(0x364a,0x1b);// ; MIPI settings
	
	OV8865_write_cmos_sensor(0x3500,0x00);// ; exposurre HH
	OV8865_write_cmos_sensor(0x3501,0x4c);// ; expouere H
	OV8865_write_cmos_sensor(0x3502,0x00);// ; exposure L
	
	OV8865_write_cmos_sensor(0x3503,0x00);// ; gain no delay, exposure no delay
	
	OV8865_write_cmos_sensor(0x3508,0x02);// ; gain H
	OV8865_write_cmos_sensor(0x3509,0x00);// ; gain L
	
	OV8865_write_cmos_sensor(0x3700,0x18);// ; sensor control
	OV8865_write_cmos_sensor(0x3701,0x0c);//
	OV8865_write_cmos_sensor(0x3702,0x28);//
	OV8865_write_cmos_sensor(0x3703,0x19);//
	OV8865_write_cmos_sensor(0x3704,0x14);//
	OV8865_write_cmos_sensor(0x3705,0x00);//
	OV8865_write_cmos_sensor(0x3706,0x28);//
	OV8865_write_cmos_sensor(0x3707,0x04);//
	OV8865_write_cmos_sensor(0x3708,0x24);//
	OV8865_write_cmos_sensor(0x3709,0x40);//
	OV8865_write_cmos_sensor(0x370a,0x00);//
	OV8865_write_cmos_sensor(0x370b,0xa8);//
	OV8865_write_cmos_sensor(0x370c,0x04);//
	OV8865_write_cmos_sensor(0x3718,0x12);//
	OV8865_write_cmos_sensor(0x3719,0x31);//
	OV8865_write_cmos_sensor(0x3712,0x42);//
	OV8865_write_cmos_sensor(0x3714,0x12);//
	OV8865_write_cmos_sensor(0x371e,0x19);//
	OV8865_write_cmos_sensor(0x371f,0x40);//
	OV8865_write_cmos_sensor(0x3720,0x05);//
	OV8865_write_cmos_sensor(0x3721,0x05);//
	OV8865_write_cmos_sensor(0x3724,0x02);//
	OV8865_write_cmos_sensor(0x3725,0x02);//
	OV8865_write_cmos_sensor(0x3726,0x06);//
	OV8865_write_cmos_sensor(0x3728,0x05);//
	OV8865_write_cmos_sensor(0x3729,0x02);//
	OV8865_write_cmos_sensor(0x372a,0x03);//
	OV8865_write_cmos_sensor(0x372b,0x53);//
	OV8865_write_cmos_sensor(0x372c,0xa3);//
	OV8865_write_cmos_sensor(0x372d,0x53);//
	OV8865_write_cmos_sensor(0x372e,0x06);//
	OV8865_write_cmos_sensor(0x372f,0x10);//
	OV8865_write_cmos_sensor(0x3730,0x01);//
	OV8865_write_cmos_sensor(0x3731,0x06);//
	OV8865_write_cmos_sensor(0x3732,0x14);//
	OV8865_write_cmos_sensor(0x3733,0x10);//
	OV8865_write_cmos_sensor(0x3734,0x40);//
	OV8865_write_cmos_sensor(0x3736,0x20);//
	OV8865_write_cmos_sensor(0x373a,0x02);//
	OV8865_write_cmos_sensor(0x373b,0x0c);//
	OV8865_write_cmos_sensor(0x373c,0x0a);//
	OV8865_write_cmos_sensor(0x373e,0x03);//
	OV8865_write_cmos_sensor(0x3755,0x40);//
	OV8865_write_cmos_sensor(0x3758,0x00);//
	OV8865_write_cmos_sensor(0x3759,0x4c);//
	OV8865_write_cmos_sensor(0x375a,0x06);//
	OV8865_write_cmos_sensor(0x375b,0x13);//
	OV8865_write_cmos_sensor(0x375c,0x20);//
	OV8865_write_cmos_sensor(0x375d,0x02);//
	OV8865_write_cmos_sensor(0x375e,0x00);//
	OV8865_write_cmos_sensor(0x375f,0x14);//
	OV8865_write_cmos_sensor(0x3767,0x04);//
	OV8865_write_cmos_sensor(0x3768,0x04);//
	OV8865_write_cmos_sensor(0x3769,0x20);//
	OV8865_write_cmos_sensor(0x376c,0x00);//
	OV8865_write_cmos_sensor(0x376d,0x00);//
	OV8865_write_cmos_sensor(0x376a,0x08);//
	OV8865_write_cmos_sensor(0x3761,0x00);//
	OV8865_write_cmos_sensor(0x3762,0x00);//
	OV8865_write_cmos_sensor(0x3763,0x00);//
	OV8865_write_cmos_sensor(0x3766,0xff);//
	OV8865_write_cmos_sensor(0x376b,0x42);//
	OV8865_write_cmos_sensor(0x3772,0x23);//
	OV8865_write_cmos_sensor(0x3773,0x02);//
	OV8865_write_cmos_sensor(0x3774,0x16);//
	OV8865_write_cmos_sensor(0x3775,0x12);//
	OV8865_write_cmos_sensor(0x3776,0x08);//
	OV8865_write_cmos_sensor(0x37a0,0x44);//
	OV8865_write_cmos_sensor(0x37a1,0x3d);//
	OV8865_write_cmos_sensor(0x37a2,0x3d);//
	OV8865_write_cmos_sensor(0x37a3,0x01);//
	OV8865_write_cmos_sensor(0x37a4,0x00);//
	OV8865_write_cmos_sensor(0x37a5,0x08);//
	OV8865_write_cmos_sensor(0x37a6,0x00);//
	OV8865_write_cmos_sensor(0x37a7,0x44);//
	OV8865_write_cmos_sensor(0x37a8,0x4c);//
	OV8865_write_cmos_sensor(0x37a9,0x4c);//
	OV8865_write_cmos_sensor(0x3760,0x00);//
	OV8865_write_cmos_sensor(0x376f,0x01);//
	OV8865_write_cmos_sensor(0x37aa,0x44);//
	OV8865_write_cmos_sensor(0x37ab,0x2e);//
	OV8865_write_cmos_sensor(0x37ac,0x2e);//
	OV8865_write_cmos_sensor(0x37ad,0x33);//
	OV8865_write_cmos_sensor(0x37ae,0x0d);//
	OV8865_write_cmos_sensor(0x37af,0x0d);//
	OV8865_write_cmos_sensor(0x37b0,0x00);//
	OV8865_write_cmos_sensor(0x37b1,0x00);//
	OV8865_write_cmos_sensor(0x37b2,0x00);//
	OV8865_write_cmos_sensor(0x37b3,0x42);//
	OV8865_write_cmos_sensor(0x37b4,0x42);//
	OV8865_write_cmos_sensor(0x37b5,0x33);//
	OV8865_write_cmos_sensor(0x37b6,0x00);//
	OV8865_write_cmos_sensor(0x37b7,0x00);//
	OV8865_write_cmos_sensor(0x37b8,0x00);//
	OV8865_write_cmos_sensor(0x37b9,0xff);// ; sensor control
	OV8865_write_cmos_sensor(0x3800,0x00);// ; X start H
	OV8865_write_cmos_sensor(0x3801,0x0c);// ; X start L
	OV8865_write_cmos_sensor(0x3802,0x00);// ; Y start H
	OV8865_write_cmos_sensor(0x3803,0x0c);// ; Y start L
	OV8865_write_cmos_sensor(0x3804,0x0c);// ; X end H
	OV8865_write_cmos_sensor(0x3805,0xd3);// ; X end L
	OV8865_write_cmos_sensor(0x3806,0x09);// ; Y end H
	OV8865_write_cmos_sensor(0x3807,0xa3);// ; Y end L
	OV8865_write_cmos_sensor(0x3808,0x06);// ; X output size H
	OV8865_write_cmos_sensor(0x3809,0x60);// ; X output size L
	OV8865_write_cmos_sensor(0x380a,0x04);// ; Y output size H
	OV8865_write_cmos_sensor(0x380b,0xc8);// ; Y output size L
	OV8865_write_cmos_sensor(0x380c,0x07);// ; HTS H
	OV8865_write_cmos_sensor(0x380d,0x83);// ; HTS L
	OV8865_write_cmos_sensor(0x380e,0x04);// ; VTS H
	OV8865_write_cmos_sensor(0x380f,0xe0);// ; VTS L
	OV8865_write_cmos_sensor(0x3810,0x00);// ; ISP X win H
	OV8865_write_cmos_sensor(0x3811,0x04);// ; ISP X win L
	OV8865_write_cmos_sensor(0x3813,0x04);// ; ISP Y win L
	OV8865_write_cmos_sensor(0x3814,0x03);// ; X inc odd
	OV8865_write_cmos_sensor(0x3815,0x01);// ; X inc even
	OV8865_write_cmos_sensor(0x3820,0x00);// ; flip off
	OV8865_write_cmos_sensor(0x3821,0x67);// ; hsync_en_o, fst_vbin, mirror on
	OV8865_write_cmos_sensor(0x382a,0x03);// ; Y inc odd
	OV8865_write_cmos_sensor(0x382b,0x01);// ; Y inc even
	OV8865_write_cmos_sensor(0x3830,0x08);// ; ablc_use_num[5:1]
	OV8865_write_cmos_sensor(0x3836,0x02);// ; zline_use_num[5:1]
	OV8865_write_cmos_sensor(0x3837,0x18);// ; vts_add_dis, cexp_gt_vts_offs=8
	OV8865_write_cmos_sensor(0x3841,0xff);// ; auto size
	OV8865_write_cmos_sensor(0x3846,0x88);// ; Y/X boundary pixel numbber for auto size mode
	OV8865_write_cmos_sensor(0x3f08,0x0b);//
	
	OV8865_write_cmos_sensor(0x4000,0xf1);// ; our range trig en, format chg en, gan chg en, exp chg en, median en
	OV8865_write_cmos_sensor(0x4001,0x14);// ; left 32 column, final BLC offset limitation enable
	OV8865_write_cmos_sensor(0x4005,0x10);// ; BLC target

	OV8865_write_cmos_sensor(0x4006,0x04);// ;revise for ZSD ON/OFF unstable,MTK
	OV8865_write_cmos_sensor(0x4007,0x04);// ; 
	
	OV8865_write_cmos_sensor(0x400b,0x0c);// ; start line =0, offset limitation en, cut range function en
	OV8865_write_cmos_sensor(0x400d,0x10);// ; offset trigger threshold
	OV8865_write_cmos_sensor(0x401b,0x00);//
	OV8865_write_cmos_sensor(0x401d,0x00);//
	OV8865_write_cmos_sensor(0x4020,0x01);// ; anchor left start H
	OV8865_write_cmos_sensor(0x4021,0x20);// ; anchor left start L
	OV8865_write_cmos_sensor(0x4022,0x01);// ; anchor left end H
	OV8865_write_cmos_sensor(0x4023,0x9f);// ; anchor left end L
	OV8865_write_cmos_sensor(0x4024,0x03);// ; anchor right start H
	OV8865_write_cmos_sensor(0x4025,0xe0);// ; anchor right start L
	OV8865_write_cmos_sensor(0x4026,0x04);// ; anchor right end H
	OV8865_write_cmos_sensor(0x4027,0x5f);// ; anchor right end L
	OV8865_write_cmos_sensor(0x4028,0x00);// ; top zero line start
	OV8865_write_cmos_sensor(0x4029,0x02);// ; top zero line number
	OV8865_write_cmos_sensor(0x402a,0x04);// ; top black line start
	OV8865_write_cmos_sensor(0x402b,0x04);// ; top black line number
	OV8865_write_cmos_sensor(0x402c,0x02);// ; bottom zero line start
	OV8865_write_cmos_sensor(0x402d,0x02);// ; bottom zero line number
	OV8865_write_cmos_sensor(0x402e,0x08);// ; bottom black line start
	OV8865_write_cmos_sensor(0x402f,0x02);// ; bottom black line number
	OV8865_write_cmos_sensor(0x401f,0x00);// ; anchor one disable
	OV8865_write_cmos_sensor(0x4034,0x3f);// ; limitation BLC offset
	OV8865_write_cmos_sensor(0x4300,0xff);// ; clip max H
	OV8865_write_cmos_sensor(0x4301,0x00);// ; clip min H
	OV8865_write_cmos_sensor(0x4302,0x0f);// ; clip min L/clip max L
	OV8865_write_cmos_sensor(0x4500,0x40);// ; ADC sync control
	OV8865_write_cmos_sensor(0x4503,0x10);//
	OV8865_write_cmos_sensor(0x4601,0x74);// ; V FIFO control
	OV8865_write_cmos_sensor(0x481f,0x32);// ; clk_prepare_min
	
	OV8865_write_cmos_sensor(0x4837,0x16);// ; clock period
	
	
	OV8865_write_cmos_sensor(0x4850,0x10);// ; lane select
	OV8865_write_cmos_sensor(0x4851,0x32);// ; lane select
	OV8865_write_cmos_sensor(0x4b00,0x2a);// ; LVDS settings
	OV8865_write_cmos_sensor(0x4b0d,0x00);// ; LVDS settings
	OV8865_write_cmos_sensor(0x4d00,0x04);// ; temperature sensor
	OV8865_write_cmos_sensor(0x4d01,0x18);// ; temperature sensor
	OV8865_write_cmos_sensor(0x4d02,0xc3);// ; temperature sensor
	OV8865_write_cmos_sensor(0x4d03,0xff);// ; temperature sensor
	OV8865_write_cmos_sensor(0x4d04,0xff);// ; temperature sensor
	OV8865_write_cmos_sensor(0x4d05,0xff);// ; temperature sensor
	
	OV8865_write_cmos_sensor(0x5000,0x96);// ; LENC on, MWB on, BPC on, WPC on
	OV8865_write_cmos_sensor(0x5001,0x01);// ; BLC on
	
	OV8865_write_cmos_sensor(0x5002,0x08);// ; vario pixel off
	OV8865_write_cmos_sensor(0x5901,0x00);//
	OV8865_write_cmos_sensor(0x5e00,0x00);// ; test pattern off
	OV8865_write_cmos_sensor(0x5e01,0x41);// ; window cut enable
	OV8865_write_cmos_sensor(0x0100,0x01);// ; wake up, streaming
	OV8865_write_cmos_sensor(0x5800,0x1d);// ; lens correction
	OV8865_write_cmos_sensor(0x5801,0x0e);//
	OV8865_write_cmos_sensor(0x5802,0x0c);//
	OV8865_write_cmos_sensor(0x5803,0x0c);//
	OV8865_write_cmos_sensor(0x5804,0x0f);//
	OV8865_write_cmos_sensor(0x5805,0x22);//
	OV8865_write_cmos_sensor(0x5806,0x0a);//
	OV8865_write_cmos_sensor(0x5807,0x06);//
	OV8865_write_cmos_sensor(0x5808,0x05);//
	OV8865_write_cmos_sensor(0x5809,0x05);//
	OV8865_write_cmos_sensor(0x580a,0x07);//
	OV8865_write_cmos_sensor(0x580b,0x0a);//
	OV8865_write_cmos_sensor(0x580c,0x06);//
	OV8865_write_cmos_sensor(0x580d,0x02);//
	OV8865_write_cmos_sensor(0x580e,0x00);//
	OV8865_write_cmos_sensor(0x580f,0x00);//
	OV8865_write_cmos_sensor(0x5810,0x03);//
	OV8865_write_cmos_sensor(0x5811,0x07);//
	OV8865_write_cmos_sensor(0x5812,0x06);//
	OV8865_write_cmos_sensor(0x5813,0x02);//
	OV8865_write_cmos_sensor(0x5814,0x00);//
	OV8865_write_cmos_sensor(0x5815,0x00);//
	OV8865_write_cmos_sensor(0x5816,0x03);//
	OV8865_write_cmos_sensor(0x5817,0x07);//
	OV8865_write_cmos_sensor(0x5818,0x09);//
	OV8865_write_cmos_sensor(0x5819,0x06);//
	OV8865_write_cmos_sensor(0x581a,0x04);//
	OV8865_write_cmos_sensor(0x581b,0x04);//
	OV8865_write_cmos_sensor(0x581c,0x06);//
	OV8865_write_cmos_sensor(0x581d,0x0a);//
	OV8865_write_cmos_sensor(0x581e,0x19);//
	OV8865_write_cmos_sensor(0x581f,0x0d);//
	OV8865_write_cmos_sensor(0x5820,0x0b);//
	OV8865_write_cmos_sensor(0x5821,0x0b);//
	OV8865_write_cmos_sensor(0x5822,0x0e);//
	OV8865_write_cmos_sensor(0x5823,0x22);//
	OV8865_write_cmos_sensor(0x5824,0x23);//
	OV8865_write_cmos_sensor(0x5825,0x28);//
	OV8865_write_cmos_sensor(0x5826,0x29);//
	OV8865_write_cmos_sensor(0x5827,0x27);//
	OV8865_write_cmos_sensor(0x5828,0x13);//
	OV8865_write_cmos_sensor(0x5829,0x26);//
	OV8865_write_cmos_sensor(0x582a,0x33);//
	OV8865_write_cmos_sensor(0x582b,0x32);//
	OV8865_write_cmos_sensor(0x582c,0x33);//
	OV8865_write_cmos_sensor(0x582d,0x16);//
	OV8865_write_cmos_sensor(0x582e,0x14);//
	OV8865_write_cmos_sensor(0x582f,0x30);//
	OV8865_write_cmos_sensor(0x5830,0x31);//
	OV8865_write_cmos_sensor(0x5831,0x30);//
	OV8865_write_cmos_sensor(0x5832,0x15);//
	OV8865_write_cmos_sensor(0x5833,0x26);//
	OV8865_write_cmos_sensor(0x5834,0x23);//
	OV8865_write_cmos_sensor(0x5835,0x21);//
	OV8865_write_cmos_sensor(0x5836,0x23);//
	OV8865_write_cmos_sensor(0x5837,0x05);//
	OV8865_write_cmos_sensor(0x5838,0x36);//
	OV8865_write_cmos_sensor(0x5839,0x27);//
	OV8865_write_cmos_sensor(0x583a,0x28);//
	OV8865_write_cmos_sensor(0x583b,0x26);//
	OV8865_write_cmos_sensor(0x583c,0x24);//
	OV8865_write_cmos_sensor(0x583d,0xdf);// ; lens correction

	//OV8865_write_cmos_sensor(0x4800,0x5c);// ; mipi gate:lens start/end

}


UINT32 OV8865Open(void)
{

	volatile signed int i;
	kal_uint16 sensor_id = 0;

	OV8865DB("OV8865 Open enter :\n ");
	OV8865_write_cmos_sensor(0x0103,0x01);// Reset sensor
    mdelay(2);

	for(i=0;i<2;i++)
	{
		sensor_id = (OV8865_read_cmos_sensor(0x300B)<<8)|OV8865_read_cmos_sensor(0x300C);
		OV8865DB("OV8865 READ ID :%x",sensor_id);
		if(sensor_id != OV8865_SENSOR_ID)
		{
			return ERROR_SENSOR_CONNECT_FAIL;
		}else
			break;
	}
	
	OV8865_Sensor_Init();
    OV8865_Init_Para();

	#if defined(OV8865_OTP)
#if 0
	dump_otp_buffer(0x7000,0x720f);
	mdelay(100);
	for(i=0;i<0x210;i++){
		OTPLOG("dump_otp_buffer, %#x = %x \n",0x7000+i,OV8865_read_cmos_sensor(0x7000+i));

	}
#endif	
	update_otp_info();
	update_otp_wb();
	update_otp_lenc();
	#endif

	
	OV8865DB("OV8865Open exit :\n ");

    return ERROR_NONE;
}


UINT32 OV8865GetSensorID(UINT32 *sensorID)
{
    int  retry = 2;

	OV8865DB("OV8865GetSensorID enter :\n ");
    mdelay(5);

    do {
        *sensorID = (OV8865_read_cmos_sensor(0x300B)<<8)|OV8865_read_cmos_sensor(0x300C);
        if (*sensorID == OV8865_SENSOR_ID)
        	{
        		OV8865DB("Sensor ID = 0x%04x\n", *sensorID);
            	break;
        	}
        OV8865DB("Read Sensor ID Fail = 0x%04x\n", *sensorID);
        retry--;
    } while (retry > 0);

    if (*sensorID != OV8865_SENSOR_ID) {
        *sensorID = 0xFFFFFFFF;
        return ERROR_SENSOR_CONNECT_FAIL;
    }
    return ERROR_NONE;
}


void OV8865_SetShutter(kal_uint32 iShutter)
{

   spin_lock(&ov8865mipiraw_drv_lock);
   ov8865.shutter= iShutter;
   spin_unlock(&ov8865mipiraw_drv_lock);

   OV8865_write_shutter(iShutter);
   return;
}



UINT32 OV8865_read_shutter(void)
{

	kal_uint16 temp_reg1, temp_reg2 ,temp_reg3;
	UINT32 shutter =0;
	temp_reg1 = OV8865_read_cmos_sensor(0x3500);    // AEC[b19~b16]
	temp_reg2 = OV8865_read_cmos_sensor(0x3501);    // AEC[b15~b8]
	temp_reg3 = OV8865_read_cmos_sensor(0x3502);    // AEC[b7~b0]
	
	shutter  = (temp_reg1 <<12)| (temp_reg2<<4)|(temp_reg3>>4);

	return shutter;
}

void OV8865_NightMode(kal_bool bEnable)
{

}

UINT32 OV8865Close(void)
{

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


UINT32 OV8865Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

	OV8865DB("OV8865Preview enter:");

	OV8865PreviewSetting();

	spin_lock(&ov8865mipiraw_drv_lock);
	ov8865.sensorMode = SENSOR_MODE_PREVIEW; 
	ov8865.DummyPixels = 0;
	ov8865.DummyLines = 0 ;
	OV8865_FeatureControl_PERIOD_PixelNum=OV8865_PV_PERIOD_PIXEL_NUMS+ ov8865.DummyPixels;
	OV8865_FeatureControl_PERIOD_LineNum=OV8865_PV_PERIOD_LINE_NUMS+ov8865.DummyLines;
	ov8865.imgMirror = sensor_config_data->SensorImageMirror;
	spin_unlock(&ov8865mipiraw_drv_lock);
	
	//OV8865SetFlipMirror(sensor_config_data->SensorImageMirror);

    mdelay(40);//THIS DELAY SHOULD BE NEED BY CTS OR MONKEY
	OV8865DB("OV8865Preview exit:\n");

	  
    return ERROR_NONE;
}


UINT32 OV8865Video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

	OV8865DB("OV8865Video enter:");

	OV8865VideoSetting();

	spin_lock(&ov8865mipiraw_drv_lock);
	ov8865.sensorMode = SENSOR_MODE_VIDEO;
	OV8865_FeatureControl_PERIOD_PixelNum=OV8865_VIDEO_PERIOD_PIXEL_NUMS+ ov8865.DummyPixels;
	OV8865_FeatureControl_PERIOD_LineNum=OV8865_VIDEO_PERIOD_LINE_NUMS+ov8865.DummyLines;
	ov8865.imgMirror = sensor_config_data->SensorImageMirror;
	spin_unlock(&ov8865mipiraw_drv_lock);
	
	//OV8865SetFlipMirror(sensor_config_data->SensorImageMirror);

    mdelay(40);//THIS DELAY SHOULD BE NEED BY CTS OR MONKEY
	OV8865DB("OV8865Video exit:\n");
    return ERROR_NONE;
}


UINT32 OV8865Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

 	kal_uint32 shutter = ov8865.shutter;
	kal_uint32 temp_data;

	if( SENSOR_MODE_CAPTURE== ov8865.sensorMode)
	{
		OV8865DB("OV8865Capture BusrtShot / ZSD!!!\n");
	}
	else
	{
		OV8865DB("OV8865Capture enter:\n");

		OV8865CaptureSetting();
	    mdelay(40);//THIS DELAY SHOULD BE NEED BY CTS OR MONKEY

		spin_lock(&ov8865mipiraw_drv_lock);
		ov8865.sensorMode = SENSOR_MODE_CAPTURE;
		ov8865.imgMirror = sensor_config_data->SensorImageMirror;
		ov8865.DummyPixels = 0;
		ov8865.DummyLines = 0 ;
		OV8865_FeatureControl_PERIOD_PixelNum = OV8865_FULL_PERIOD_PIXEL_NUMS + ov8865.DummyPixels;
		OV8865_FeatureControl_PERIOD_LineNum = OV8865_FULL_PERIOD_LINE_NUMS + ov8865.DummyLines;
		spin_unlock(&ov8865mipiraw_drv_lock);

		//OV8865SetFlipMirror(sensor_config_data->SensorImageMirror);

		OV8865DB("OV8865Capture exit:\n");
	}

	if(OV8865_During_testpattern == KAL_TRUE)
	{
		OV8865_write_cmos_sensor(0x5E00,0x80);
	}

    return ERROR_NONE;
}	



UINT32 OV8865GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{

    OV8865DB("OV8865GetResolution!!\n");

	pSensorResolution->SensorPreviewWidth	= OV8865_IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorPreviewHeight	= OV8865_IMAGE_SENSOR_PV_HEIGHT;
	
    pSensorResolution->SensorFullWidth		= OV8865_IMAGE_SENSOR_FULL_WIDTH;
    pSensorResolution->SensorFullHeight		= OV8865_IMAGE_SENSOR_FULL_HEIGHT;
	
    pSensorResolution->SensorVideoWidth		= OV8865_IMAGE_SENSOR_VIDEO_WIDTH;
    pSensorResolution->SensorVideoHeight    = OV8865_IMAGE_SENSOR_VIDEO_HEIGHT;
    return ERROR_NONE;
}   

UINT32 OV8865GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
                                                MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{

	spin_lock(&ov8865mipiraw_drv_lock);
	ov8865.imgMirror = pSensorConfigData->SensorImageMirror ;
	spin_unlock(&ov8865mipiraw_drv_lock);

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

            pSensorInfo->SensorGrabStartX = OV8865_PV_X_START;
            pSensorInfo->SensorGrabStartY = OV8865_PV_Y_START;

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_4_LANE;
			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	     	pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14;
	    	pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockRisingCount= 0;

            pSensorInfo->SensorGrabStartX = OV8865_VIDEO_X_START;
            pSensorInfo->SensorGrabStartY = OV8865_VIDEO_Y_START;

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_4_LANE;
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	     	pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14;
	    	pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockRisingCount= 0;

            pSensorInfo->SensorGrabStartX = OV8865_FULL_X_START;	
            pSensorInfo->SensorGrabStartY = OV8865_FULL_Y_START;	

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_4_LANE;
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14;
            pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
        default:
			pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockRisingCount= 0;

            pSensorInfo->SensorGrabStartX = OV8865_PV_X_START;
            pSensorInfo->SensorGrabStartY = OV8865_PV_Y_START;

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_4_LANE;
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	     	pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14;
	    	pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
    }

    memcpy(pSensorConfigData, &OV8865SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

    return ERROR_NONE;
}   /* OV8865GetInfo() */



UINT32 OV8865Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
		spin_lock(&ov8865mipiraw_drv_lock);
		OV8865CurrentScenarioId = ScenarioId;
		spin_unlock(&ov8865mipiraw_drv_lock);
		
		OV8865DB("hkm_0815 OV8865CurrentScenarioId=%d\n",OV8865CurrentScenarioId);

	switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            OV8865Preview(pImageWindow, pSensorConfigData);
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			OV8865Video(pImageWindow, pSensorConfigData);
			break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
            OV8865Capture(pImageWindow, pSensorConfigData);
            break;

        default:
            return ERROR_INVALID_SCENARIO_ID;

    }
    return ERROR_NONE;
} /* OV8865Control() */



kal_uint32 OV8865_SET_FrameLength_ByVideoMode(UINT16 Video_TargetFps)
{
    UINT32 frameRate = 0;
	kal_uint32 MIN_FrameLength=0;
	
	if(ov8865.OV8865AutoFlickerMode == KAL_TRUE)
	{
		if (Video_TargetFps==30)
			frameRate= OV8865_AUTOFLICKER_OFFSET_30;
		else if(Video_TargetFps==15)
			frameRate= OV8865_AUTOFLICKER_OFFSET_15;
		else
			frameRate=Video_TargetFps*10;
	
		MIN_FrameLength = (ov8865.videoPclk*10000)/(OV8865_VIDEO_PERIOD_PIXEL_NUMS + ov8865.DummyPixels)/frameRate*10;
	}
	else
		MIN_FrameLength = (ov8865.videoPclk*10000) /(OV8865_VIDEO_PERIOD_PIXEL_NUMS + ov8865.DummyPixels)/Video_TargetFps;

     return MIN_FrameLength;

}



UINT32 OV8865SetVideoMode(UINT16 u2FrameRate)
{

    kal_uint32 MIN_Frame_length =0,frameRate=0,extralines=0;
    OV8865DB("[OV8865SetVideoMode] frame rate = %d\n", u2FrameRate);

	spin_lock(&ov8865mipiraw_drv_lock);
	OV8865_VIDEO_MODE_TARGET_FPS=u2FrameRate;
	spin_unlock(&ov8865mipiraw_drv_lock);

	if(u2FrameRate==0)
	{
		OV8865DB("Disable Video Mode or dynimac fps\n");
		return KAL_TRUE;
	}
	if(u2FrameRate >30 || u2FrameRate <5)
	    OV8865DB("abmornal frame rate seting,pay attention~\n");

    if(ov8865.sensorMode == SENSOR_MODE_VIDEO)//video ScenarioId recording
    {

        MIN_Frame_length = OV8865_SET_FrameLength_ByVideoMode(u2FrameRate);

		if((MIN_Frame_length <=OV8865_VIDEO_PERIOD_LINE_NUMS))
		{
			MIN_Frame_length = OV8865_VIDEO_PERIOD_LINE_NUMS;
			OV8865DB("[OV8865SetVideoMode]current fps = %d\n", (ov8865.videoPclk*10000)  /(OV8865_VIDEO_PERIOD_PIXEL_NUMS)/OV8865_VIDEO_PERIOD_LINE_NUMS);
		}
		OV8865DB("[OV8865SetVideoMode]current fps (10 base)= %d\n", (ov8865.videoPclk*10000)*10/(OV8865_VIDEO_PERIOD_PIXEL_NUMS + ov8865.DummyPixels)/MIN_Frame_length);
		extralines = MIN_Frame_length - OV8865_VIDEO_PERIOD_LINE_NUMS;
		
		spin_lock(&ov8865mipiraw_drv_lock);
		ov8865.DummyPixels = 0;//define dummy pixels and lines
		ov8865.DummyLines = extralines ;
		spin_unlock(&ov8865mipiraw_drv_lock);
		
		OV8865_SetDummy(ov8865.DummyPixels,extralines);
    }
	
	OV8865DB("[OV8865SetVideoMode]MIN_Frame_length=%d,ov8865.DummyLines=%d\n",MIN_Frame_length,ov8865.DummyLines);

    return KAL_TRUE;
}


UINT32 OV8865SetAutoFlickerMode(kal_bool bEnable, UINT16 u2FrameRate)
{

	if(bEnable) {   
		spin_lock(&ov8865mipiraw_drv_lock);
		ov8865.OV8865AutoFlickerMode = KAL_TRUE;
		spin_unlock(&ov8865mipiraw_drv_lock);
        OV8865DB("OV8865 Enable Auto flicker\n");
    } else {
    	spin_lock(&ov8865mipiraw_drv_lock);
        ov8865.OV8865AutoFlickerMode = KAL_FALSE;
		spin_unlock(&ov8865mipiraw_drv_lock);
        OV8865DB("OV8865 Disable Auto flicker\n");
    }

    return ERROR_NONE;
}


UINT32 OV8865SetTestPatternMode(kal_bool bEnable)
{
    OV8865DB("[OV8865SetTestPatternMode] Test pattern enable:%d\n", bEnable);
    if(bEnable == KAL_TRUE)
    {
        OV8865_During_testpattern = KAL_TRUE;
		OV8865_write_cmos_sensor(0x5E00,0x80);
    }
	else
	{
        OV8865_During_testpattern = KAL_FALSE;
		OV8865_write_cmos_sensor(0x5E00,0x00);
	}

    return ERROR_NONE;
}


/*************************************************************************
*
* DESCRIPTION:
* INTERFACE FUNCTION, FOR USER TO SET MAX  FRAMERATE;
* 
*************************************************************************/
UINT32 OV8865MIPISetMaxFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 frameRate) {
	kal_uint32 pclk;
	kal_int16 dummyLine;
	kal_uint16 lineLength,frameHeight;
		
	OV8865DB("OV8865MIPISetMaxFramerateByScenario: scenarioId = %d, frame rate = %d\n",scenarioId,frameRate);
	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			pclk = OV8865_PREVIEW_PCLK;
			lineLength = OV8865_PV_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - OV8865_PV_PERIOD_LINE_NUMS;
			if(dummyLine<0)
				dummyLine = 0;
			spin_lock(&ov8865mipiraw_drv_lock);
			ov8865.sensorMode = SENSOR_MODE_PREVIEW;
			spin_unlock(&ov8865mipiraw_drv_lock);
			OV8865_SetDummy(0, dummyLine);			
			break;			
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			pclk = OV8865_VIDEO_PCLK;
			lineLength = OV8865_VIDEO_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - OV8865_VIDEO_PERIOD_LINE_NUMS;
			if(dummyLine<0)
				dummyLine = 0;
			spin_lock(&ov8865mipiraw_drv_lock);
			ov8865.sensorMode = SENSOR_MODE_VIDEO;
			spin_unlock(&ov8865mipiraw_drv_lock);
			OV8865_SetDummy(0, dummyLine);			
			break;			
			 break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:			
			pclk = OV8865_CAPTURE_PCLK;
			lineLength = OV8865_FULL_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - OV8865_FULL_PERIOD_LINE_NUMS;
			if(dummyLine<0)
				dummyLine = 0;
			spin_lock(&ov8865mipiraw_drv_lock);
			ov8865.sensorMode = SENSOR_MODE_CAPTURE;
			spin_unlock(&ov8865mipiraw_drv_lock);
			OV8865_SetDummy(0, dummyLine);			
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


UINT32 OV8865MIPIGetDefaultFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 *pframeRate) 
{

	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			 *pframeRate = OV8865_MAX_FPS_PREVIEW;
			 break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			 *pframeRate = OV8865_MAX_FPS_CAPTURE;
			break;		
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added   
			 *pframeRate = OV8865_MAX_FPS_CAPTURE;
			break;		
		default:
			break;
	}

	return ERROR_NONE;
}


UINT32 OV8865FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
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
            *pFeatureReturnPara16++= OV8865_IMAGE_SENSOR_FULL_WIDTH;
            *pFeatureReturnPara16= OV8865_IMAGE_SENSOR_FULL_HEIGHT;
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_PERIOD:
				*pFeatureReturnPara16++= OV8865_FeatureControl_PERIOD_PixelNum;
				*pFeatureReturnPara16= OV8865_FeatureControl_PERIOD_LineNum;
				*pFeatureParaLen=4;
				break;
        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			switch(OV8865CurrentScenarioId)
			{
				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
					*pFeatureReturnPara32 = OV8865_PREVIEW_PCLK;
					*pFeatureParaLen=4;
					break;
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					*pFeatureReturnPara32 = OV8865_VIDEO_PCLK;
					*pFeatureParaLen=4;
					break;
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
				case MSDK_SCENARIO_ID_CAMERA_ZSD:
					*pFeatureReturnPara32 = OV8865_CAPTURE_PCLK;
					*pFeatureParaLen=4;
					break;
				default:
					*pFeatureReturnPara32 = OV8865_PREVIEW_PCLK;
					*pFeatureParaLen=4;
					break;
			}
		    break;

        case SENSOR_FEATURE_SET_ESHUTTER:
            OV8865_SetShutter(*pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_NIGHTMODE:
            OV8865_NightMode((BOOL) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_GAIN:  
           OV8865_SetGain((UINT16) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_FLASHLIGHT:
            break;
        case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
            //OV8865_isp_master_clock=*pFeatureData32;
            break;
        case SENSOR_FEATURE_SET_REGISTER:
            OV8865_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
            break;
        case SENSOR_FEATURE_GET_REGISTER:
            pSensorRegData->RegData = OV8865_read_cmos_sensor(pSensorRegData->RegAddr);
            break;
        case SENSOR_FEATURE_SET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            for (i=0;i<SensorRegNumber;i++)
            {
            	spin_lock(&ov8865mipiraw_drv_lock);
                OV8865SensorCCT[i].Addr=*pFeatureData32++;
                OV8865SensorCCT[i].Para=*pFeatureData32++;
				spin_unlock(&ov8865mipiraw_drv_lock);
            }
            break;
        case SENSOR_FEATURE_GET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=OV8865SensorCCT[i].Addr;
                *pFeatureData32++=OV8865SensorCCT[i].Para;
            }
            break;
        case SENSOR_FEATURE_SET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            for (i=0;i<SensorRegNumber;i++)
            {
            	spin_lock(&ov8865mipiraw_drv_lock);
                OV8865SensorReg[i].Addr=*pFeatureData32++;
                OV8865SensorReg[i].Para=*pFeatureData32++;
				spin_unlock(&ov8865mipiraw_drv_lock);
            }
            break;
        case SENSOR_FEATURE_GET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=OV8865SensorReg[i].Addr;
                *pFeatureData32++=OV8865SensorReg[i].Para;
            }
            break;
        case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
            if (*pFeatureParaLen>=sizeof(NVRAM_SENSOR_DATA_STRUCT))
            {
                pSensorDefaultData->Version=NVRAM_CAMERA_SENSOR_FILE_VERSION;
                pSensorDefaultData->SensorId=OV8865_SENSOR_ID;
                memcpy(pSensorDefaultData->SensorEngReg, OV8865SensorReg, sizeof(SENSOR_REG_STRUCT)*ENGINEER_END);
                memcpy(pSensorDefaultData->SensorCCTReg, OV8865SensorCCT, sizeof(SENSOR_REG_STRUCT)*FACTORY_END_ADDR);
            }
            else
                return FALSE;
            *pFeatureParaLen=sizeof(NVRAM_SENSOR_DATA_STRUCT);
            break;
        case SENSOR_FEATURE_GET_CONFIG_PARA:
            memcpy(pSensorConfigData, &OV8865SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
            *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
            break;
        case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
            OV8865_camera_para_to_sensor();
            break;

        case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
            OV8865_sensor_to_camera_para();
            break;
        case SENSOR_FEATURE_GET_GROUP_COUNT:
            *pFeatureReturnPara32++=OV8865_get_sensor_group_count();
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_GROUP_INFO:
            OV8865_get_sensor_group_info(pSensorGroupInfo->GroupIdx, pSensorGroupInfo->GroupNamePtr, &pSensorGroupInfo->ItemCount);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_GROUP_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_ITEM_INFO:
            OV8865_get_sensor_item_info(pSensorItemInfo->GroupIdx,pSensorItemInfo->ItemIdx, pSensorItemInfo);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_SET_ITEM_INFO:
            OV8865_set_sensor_item_info(pSensorItemInfo->GroupIdx, pSensorItemInfo->ItemIdx, pSensorItemInfo->ItemValue);
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
            OV8865SetVideoMode(*pFeatureData16);
            break;
        case SENSOR_FEATURE_CHECK_SENSOR_ID:
            OV8865GetSensorID(pFeatureReturnPara32);
            break;
        case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
            OV8865SetAutoFlickerMode((BOOL)*pFeatureData16, *(pFeatureData16+1));
	        break;
		case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
			OV8865MIPISetMaxFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, *(pFeatureData32+1));
			break;
		case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
			OV8865MIPIGetDefaultFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, (MUINT32 *)(*(pFeatureData32+1)));
			break;
		case SENSOR_FEATURE_SET_TEST_PATTERN:
			OV8865SetTestPatternMode((BOOL)*pFeatureData16);
			break;
		case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE://for factory mode auto testing 			
			*pFeatureReturnPara32=OV8865_TEST_PATTERN_CHECKSUM; 		  
			*pFeatureParaLen=4; 							
		     break;
        default:
            break;
    }
    return ERROR_NONE;
}	


SENSOR_FUNCTION_STRUCT	SensorFuncOV8865=
{
    OV8865Open,
    OV8865GetInfo,
    OV8865GetResolution,
    OV8865FeatureControl,
    OV8865Control,
    OV8865Close
};

UINT32 OV8865_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    if (pfFunc!=NULL)
        *pfFunc=&SensorFuncOV8865;

    return ERROR_NONE;
}  

