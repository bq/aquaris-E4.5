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

#include "hi191mipiraw_Sensor.h"
#include "hi191mipiraw_Camera_Sensor_para.h"
#include "hi191mipiraw_CameraCustomized.h"

#if 1
	#define SENSORDB printk
#else
	#define SENSORDB(x,...)
#endif
//#define ACDK
extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);

static HI191MIPI_sensor_struct HI191MIPI_sensor =
{
  .video_mode=KAL_FALSE,
  .shutter = 0x20,  
  .gain = 0x20,
  .pclk = HI191MIPI_PREVIEW_CLK,
  .frame_height   = HI191MIPI_PV_PERIOD_LINE_NUMS  + HI191MIPI_DEFAULT_DUMMY_LINES,
  .line_length    = HI191MIPI_PV_PERIOD_PIXEL_NUMS + HI191MIPI_DEFAULT_DUMMY_PIXELS,
  .default_height = HI191MIPI_PV_PERIOD_LINE_NUMS  + HI191MIPI_DEFAULT_DUMMY_LINES,
};

kal_uint16 HI191MIPI_sensor_id=0;
kal_uint16 HI191MIPI_state=1;
static DEFINE_SPINLOCK(hi191mipiraw_drv_lock);
kal_bool HI191MIPI_AutoFlicker_Mode=KAL_FALSE;
kal_uint8  HI191test_pattern_flag=0;

#define HI191_TEST_PATTERN_CHECKSUM (0x296ff865)

static MSDK_SCENARIO_ID_ENUM CurrentScenarioId=MSDK_SCENARIO_ID_CAMERA_PREVIEW;

kal_uint16 HI191MIPI_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
    char puSendCmd[3] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para & 0xFF)};
	
	iWriteRegI2C(puSendCmd , 3,HI191MIPI_WRITE_ID);

}
kal_uint16 HI191MIPI_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
    char puSendCmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,1,HI191MIPI_WRITE_ID);
		
    return get_byte;
}

static void HI191MIPI_Write_Shutter(kal_uint16 iShutter)
{
	kal_uint16 shutter_line = 0,current_fps=0,debugnum=0;
	unsigned long flags;
	SENSORDB("[HI191MIPI]enter HI191MIPI_Write_Shutter function iShutter=%d\n",iShutter);	    
	if (iShutter < 4) iShutter = 4; /* avoid 0 */	
	if(iShutter > HI191MIPI_sensor.default_height-4) 
	{
	    shutter_line=iShutter+4;
	}
	else
	{
	   shutter_line=HI191MIPI_sensor.default_height;
	}
	current_fps=HI191MIPI_sensor.pclk/HI191MIPI_sensor.line_length/shutter_line;
	SENSORDB("CURRENT FPS:%d,HI191MIPI_sensor.default_height=%d",
		current_fps,HI191MIPI_sensor.default_height);		 
	if(current_fps==30 || current_fps==15)
	{
		if(HI191MIPI_AutoFlicker_Mode==TRUE)
		  shutter_line=shutter_line+(shutter_line>>7);
	}
	spin_lock_irqsave(&hi191mipiraw_drv_lock,flags);
	HI191MIPI_sensor.frame_height=shutter_line;
	spin_unlock_irqrestore(&hi191mipiraw_drv_lock,flags);
	SENSORDB("shutter_line=%d",shutter_line);	
	HI191MIPI_write_cmos_sensor(0x0104,0x01);
	HI191MIPI_write_cmos_sensor(0x0340, shutter_line >> 8);   
    HI191MIPI_write_cmos_sensor(0x0341, shutter_line & 0xFF);
	HI191MIPI_write_cmos_sensor(0x0202, iShutter >> 8);
	HI191MIPI_write_cmos_sensor(0x0203, iShutter & 0xFF);
	HI191MIPI_write_cmos_sensor(0x0104,0x00);
	#if 0
	debugnum=HI191MIPI_read_cmos_sensor(0x0340);
	SENSORDB("debugnum=%d",debugnum);
	debugnum=HI191MIPI_read_cmos_sensor(0x0341);
	SENSORDB("debugnum=%d",debugnum);
	debugnum=HI191MIPI_read_cmos_sensor(0x0202);
	SENSORDB("debugnum=%d",debugnum);
	debugnum=HI191MIPI_read_cmos_sensor(0x0203);
	SENSORDB("debugnum=%d",debugnum);
	#endif
	SENSORDB("[HI191MIPI]exit HI191MIPI_Write_Shutter function 2722MIPI_frame_length=%x\n",HI191MIPI_sensor.frame_height);
}   /*  HI191MIPI_Write_Shutter    */

static void HI191MIPI_Set_Dummy(const kal_uint16 iPixels, const kal_uint16 iLines)
{
	kal_uint16 hactive, vactive, line_length, frame_height;
	SENSORDB("[HI191MIPI]enter HI191MIPI_Set_Dummy function iPixels:%x; iLines:%x \n",iPixels,iLines);
    line_length = HI191MIPI_FULL_PERIOD_PIXEL_NUMS + iPixels+HI191MIPI_DEFAULT_DUMMY_PIXELS;
	frame_height = HI191MIPI_FULL_PERIOD_LINE_NUMS + iLines+HI191MIPI_DEFAULT_DUMMY_LINES;	
	SENSORDB("[ylf]line_length:%x; frame_height:%x \n",line_length,frame_height);
	if ((line_length >= 0x1FFF)||(frame_height >= 0xFFF))
	{
		SENSORDB("Warnning: line length or frame height is overflow!!!!!!!!  \n");
		return ;
	}
#if 1  //add by chenqiang 
    spin_lock(&hi191mipiraw_drv_lock);
	HI191MIPI_sensor.line_length = line_length;
	HI191MIPI_sensor.frame_height = frame_height;
	HI191MIPI_sensor.default_height=frame_height;
	spin_unlock(&hi191mipiraw_drv_lock);
	SENSORDB("[ylf]line_length:%x; frame_height:%x \n",line_length,frame_height);
    HI191MIPI_write_cmos_sensor(0x0342, line_length >> 8);
    HI191MIPI_write_cmos_sensor(0x0343, line_length & 0xFF);
    HI191MIPI_write_cmos_sensor(0x0340, frame_height >> 8);
    HI191MIPI_write_cmos_sensor(0x0341, frame_height & 0xFF);
#endif
	SENSORDB("[HI191MIPI]exit HI191MIPI_Set_Dummy function \n");
}   /*  HI191MIPI_Set_Dummy    */
/*************************************************************************
* FUNCTION
*	HI191MIPI_SetShutter
*
* DESCRIPTION
*	This function set e-shutter of HI191MIPI to change exposure time.
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
void set_HI191MIPI_shutter(kal_uint16 iShutter)
{
#if 1
	unsigned long flags;
	SENSORDB("[HI191MIPI]enter set_HI191MIPI_shutter function iShutter=%x\n",iShutter);
    spin_lock_irqsave(&hi191mipiraw_drv_lock,flags);
	HI191MIPI_sensor.shutter = iShutter;
	spin_unlock_irqrestore(&hi191mipiraw_drv_lock,flags);
    HI191MIPI_Write_Shutter(iShutter);
	SENSORDB("[HI191MIPI]exit set_HI191MIPI_shutter function \n");
#endif
}   /*  Set_HI191MIPI_Shutter */
static kal_uint8 HI191MIPIGain2Reg(const kal_uint16 iGain)
{
	/* Total gain =256/ (reg [0]*1+ reg[1]*2+ reg [2]*4+ reg [3]*8+ reg [4]*16+ reg[5]*32+ reg[6]*64+ reg [7]*128+32) */
	#if 1
	SENSORDB("[HI191MIPI]enter HI191MIPIGain2Reg function \n");
	kal_uint16 iReg = 0;
	kal_uint16 iBaseGain = 64,max_gain=0;
	max_gain=iGain;
	if(max_gain>512)
		max_gain=512;
	iReg = 256 * iBaseGain/max_gain-32;
	iReg = iReg & 0xFF;
	return (kal_uint8)iReg;
	SENSORDB("[HI191MIPI]exit HI191MIPIGain2Reg function \n");
	#endif
}





/*************************************************************************
* FUNCTION
*	HI191MIPI_SetGain
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
void HI191MIPI_SetGain(UINT16 iGain)
{
	#if 1
	kal_uint8 iReg;
	//HI191sensor_gain  M= iGain/64;
	/* Total gain =256/ (M [0]*1+ M [1]*2+ M [2]*4+ M [3]*8+ M [4]*16+ M [5]*32+ M [6]*64+ M [7]*128+32) */
	SENSORDB("[HI191MIPI]enter HI191MIPIGain2Reg function iGain=%x\n",iGain);
	iReg = HI191MIPIGain2Reg(iGain);
	SENSORDB("[ylf]HI191MIPI_SetGain,iReg:%x",iReg);
	//if (iReg < 0x10) iReg = 0x10;
	HI191MIPI_write_cmos_sensor(0x0104,0x01);
	HI191MIPI_write_cmos_sensor(0x0205, iReg);
	HI191MIPI_write_cmos_sensor(0x0104,0x00);
	SENSORDB("[HI191MIPI]exit HI191MIPI_SetGain function \n");
	return iGain;		
	#endif
}	/*	HI191MIPI_SetGain	*/



/*************************************************************************
* FUNCTION
*	HI191MIPI_NightMode
*
* DESCRIPTION
*	This function night mode of HI191MIPI.
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
void HI191MIPI_night_mode(kal_bool enable)
{
	const kal_uint16 dummy_pixel = HI191MIPI_sensor.line_length - HI191MIPI_PV_PERIOD_PIXEL_NUMS;
	const kal_uint16 pv_min_fps =  enable ? HI191MIPI_sensor.night_fps : HI191MIPI_sensor.normal_fps;
	kal_uint16 dummy_line = HI191MIPI_sensor.frame_height - HI191MIPI_PV_PERIOD_LINE_NUMS;
	kal_uint16 max_exposure_lines;
	SENSORDB("[HI191MIPI]enter HI191MIPI_night_mode function enable=%x\n",enable);

	if (!HI191MIPI_sensor.video_mode) 
		return;
	max_exposure_lines = HI191MIPI_sensor.pclk * HI191MIPI_FPS(1) / (pv_min_fps * HI191MIPI_sensor.line_length);
	if (max_exposure_lines > HI191MIPI_sensor.frame_height) /* fix max frame rate, AE table will fix min frame rate */
	{
		dummy_line = max_exposure_lines - HI191MIPI_PV_PERIOD_LINE_NUMS;
		SENSORDB("[ylf]dummy_line:%x;\n",dummy_line);
		HI191MIPI_Set_Dummy(dummy_pixel, dummy_line);
	SENSORDB("[HI191MIPI]exit HI191MIPI_night_mode function \n");/*  HI191MIPI_NightMode    */
	}
}
void HI191MIPI_init()
{
    kal_uint16 debugnum=0;
	SENSORDB("[HI191MIPI]enter HI191MIPI_init function \n");
	HI191MIPI_write_cmos_sensor(0x5020, 0x30);            // pad_drv_cnt0
	HI191MIPI_write_cmos_sensor(0x5021, 0x00);            // pad_drv_cnt1
	HI191MIPI_write_cmos_sensor(0xB000, 0x00);            // mipi_tx_op_en
	HI191MIPI_write_cmos_sensor(0x0200, 0x00);            // fine integration
	HI191MIPI_write_cmos_sensor(0x0201, 0x50);            // fine integration
	HI191MIPI_write_cmos_sensor(0x0202, 0x02);            //  4 corse integration
	HI191MIPI_write_cmos_sensor(0x0203, 0x04);            // FF corse integration 
	//HI191MIPI_write_cmos_sensor(0x0205, 0xE0);                       // analog gain control
	HI191MIPI_write_cmos_sensor(0x0205, 0x00);            // analog gain control
	HI191MIPI_write_cmos_sensor(0x0340, 0x03);            // frame_length_lines
	HI191MIPI_write_cmos_sensor(0x0341, 0x04);            // frame_length_lines    
	HI191MIPI_write_cmos_sensor(0x0342, 0x06);            // line_length_pck
	HI191MIPI_write_cmos_sensor(0x0343, 0x14);            // line_length_pck    
	HI191MIPI_write_cmos_sensor(0x0344, 0x00);            // x_addr_start
	HI191MIPI_write_cmos_sensor(0x0345, 0x14);            // x_addr_start
	HI191MIPI_write_cmos_sensor(0x0346, 0x00);            // y_addr_start
	HI191MIPI_write_cmos_sensor(0x0347, 0x00);            // y_addr_start    
	HI191MIPI_write_cmos_sensor(0x0348, 0x05);            // x_addr_end
	HI191MIPI_write_cmos_sensor(0x0349, 0x37);            // x_addr_end    
	HI191MIPI_write_cmos_sensor(0x034A, 0x02);            // y_addr_end
	HI191MIPI_write_cmos_sensor(0x034B, 0xEF);            // y_addr_end    
	HI191MIPI_write_cmos_sensor(0x034C, 0x05);            // x_output_size
	HI191MIPI_write_cmos_sensor(0x034D, 0x20);            // x_output_size    
	HI191MIPI_write_cmos_sensor(0x034E, 0x02);            // y_output_size
	HI191MIPI_write_cmos_sensor(0x034F, 0xF0);            // y_output_size    
	HI191MIPI_write_cmos_sensor(0x0381, 0x01);            // x_even_inc
	HI191MIPI_write_cmos_sensor(0x0383, 0x01);            // x_odd_inc
	HI191MIPI_write_cmos_sensor(0x0385, 0x01);            // x_even_inc
	HI191MIPI_write_cmos_sensor(0x0387, 0x01);            // x_odd_inc
	HI191MIPI_write_cmos_sensor(0x0900, 0x00);            // binning_mode
	HI191MIPI_write_cmos_sensor(0x0901, 0x22);            // binning_type
	HI191MIPI_write_cmos_sensor(0x0B04, 0x00);            // black_level_correction_enable
	HI191MIPI_write_cmos_sensor(0x8000, 0x0C);            // tg_ctl1
	HI191MIPI_write_cmos_sensor(0x8001, 0x02);            // tg_ctl2
	HI191MIPI_write_cmos_sensor(0x8002, 0x60);            // tg_ctl3  
	HI191MIPI_write_cmos_sensor(0x8003, 0x04);            // tg_ctl4
	HI191MIPI_write_cmos_sensor(0x8004, 0x00);            // tg_ctl5
	HI191MIPI_write_cmos_sensor(0x8007, 0x00);            // data_c_time_opt
	HI191MIPI_write_cmos_sensor(0x800A, 0x00);            // drk_y_addr_s 
	HI191MIPI_write_cmos_sensor(0x800B, 0x07);            // drk_y_addr_e
	HI191MIPI_write_cmos_sensor(0x800C, 0x43);            // pd_flush_ctl1
	HI191MIPI_write_cmos_sensor(0x800D, 0x00);
	HI191MIPI_write_cmos_sensor(0x800E, 0x37);            // pd_flush_ctl3
	HI191MIPI_write_cmos_sensor(0x8033, 0x10);            // analog_func_ctl1 30-->10  08
	HI191MIPI_write_cmos_sensor(0x8034, 0x01);            // analog_func_ctl2
	HI191MIPI_write_cmos_sensor(0x8035, 0x20);            // analog_func_ctl3
	HI191MIPI_write_cmos_sensor(0x8036, 0x80);            // analog_func_ctl4
	HI191MIPI_write_cmos_sensor(0x8039, 0x0A);            // a_sig_fix_ctl3
	HI191MIPI_write_cmos_sensor(0x803A, 0x80);            // a_sig_fix_ctl4  
	HI191MIPI_write_cmos_sensor(0x803D, 0x02);            // cds_bn_sw_n_ctl  
	HI191MIPI_write_cmos_sensor(0x803F, 0xC0);            // cds_sig_clp  
	HI191MIPI_write_cmos_sensor(0x8040, 0x04);            // pxl_pwr_ctl_ctl
	HI191MIPI_write_cmos_sensor(0x8041, 0x04);            // ramp_gen_ctl1 B-->4  09
	HI191MIPI_write_cmos_sensor(0x8042, 0x79);            // ramp_gen_ctl2 79 --> 19  09
	HI191MIPI_write_cmos_sensor(0x8043, 0x7b);            // ramp_gen_ctl3  
	HI191MIPI_write_cmos_sensor(0x8079, 0x21);            // ramp_gen_ctl4  
	HI191MIPI_write_cmos_sensor(0x807A, 0x3F);            // ramp_gen_ctl5  
	HI191MIPI_write_cmos_sensor(0x8044, 0xFF);
	HI191MIPI_write_cmos_sensor(0x8045, 0x7F);
	HI191MIPI_write_cmos_sensor(0x8046, 0x00);
	HI191MIPI_write_cmos_sensor(0x8048, 0x00);            
	HI191MIPI_write_cmos_sensor(0x8049, 0xC8);
	HI191MIPI_write_cmos_sensor(0x804A, 0xF1);            // cds_ctl2
	HI191MIPI_write_cmos_sensor(0x807F, 0x27);            // cds_ctl3
	HI191MIPI_write_cmos_sensor(0x8080, 0x77);            // cds_ctl4
	HI191MIPI_write_cmos_sensor(0x8081, 0x03);            // cds_ctl5 
	HI191MIPI_write_cmos_sensor(0x8082, 0x00);            // cds_ctl6 
	HI191MIPI_write_cmos_sensor(0x8083, 0x00);            // cds_ctl7
	HI191MIPI_write_cmos_sensor(0x804B, 0x03);            // bias_ctl1 
	HI191MIPI_write_cmos_sensor(0x804E, 0x1f);            // pxl_bias_ctl1 98 -->17  09 
	HI191MIPI_write_cmos_sensor(0x804F, 0x1f);            // pxl_bias_ctl2 98 -->17  09
	HI191MIPI_write_cmos_sensor(0x8050, 0x1f);            // pxl_bias_ctl3 98 -->17  09
	HI191MIPI_write_cmos_sensor(0x8086, 0x1f);            // pxl_bias_ctl4
	HI191MIPI_write_cmos_sensor(0x8087, 0x1f);            // pxl_bias_ctl5
	HI191MIPI_write_cmos_sensor(0x8088, 0x1f);            // pxl_bias_ctl6
	HI191MIPI_write_cmos_sensor(0x8051, 0x3f);            // cds_bias_ctl1
	HI191MIPI_write_cmos_sensor(0x8052, 0x3f);            // cds_bias_ctl2
	HI191MIPI_write_cmos_sensor(0x8053, 0x3f);            // cds_bias_ctl3
	HI191MIPI_write_cmos_sensor(0x8089, 0x3f);            // cds_bias_ctl3
	HI191MIPI_write_cmos_sensor(0x808A, 0x08);            // cds_bias_ctl5
	HI191MIPI_write_cmos_sensor(0x8054, 0x55);            // clamp_ctl
	HI191MIPI_write_cmos_sensor(0x8058, 0x07);            // dcdc_ctl1 4-->7   09
	HI191MIPI_write_cmos_sensor(0x8059, 0x00);            // dcdc_ctl2
	HI191MIPI_write_cmos_sensor(0x805A, 0x17);            // dcdc_ctl3 04-->14  08
	HI191MIPI_write_cmos_sensor(0x805B, 0x1c);            // dcdc_ctl4 06-->16   08
	HI191MIPI_write_cmos_sensor(0x805C, 0x17);            // dcdc_ctl5 04-->14  08
	HI191MIPI_write_cmos_sensor(0x805D, 0x1c);            // dcdc_ctl6 06-->16  08
	HI191MIPI_write_cmos_sensor(0x808B, 0x00);            // dcdc_ctl7
	HI191MIPI_write_cmos_sensor(0x808C, 0x3F);            // dcdc_ctl8 36-->3f  08
	HI191MIPI_write_cmos_sensor(0x808D, 0x00);            // dcdc_ctl9
	HI191MIPI_write_cmos_sensor(0x805E, 0x55);            // dcdc_bias_ctl
	HI191MIPI_write_cmos_sensor(0x805F, 0x24);            // p_dcdc_ctl_exp_lum1
	HI191MIPI_write_cmos_sensor(0x8063, 0x26);            // n_dcdc_ctl_exp_lum1
	HI191MIPI_write_cmos_sensor(0x808E, 0x24);            // p_dcdc_ctl_read_lum1
	HI191MIPI_write_cmos_sensor(0x8092, 0x26);            // n_dcdc_ctl_read_lum1    
	HI191MIPI_write_cmos_sensor(0x8072, 0x01);            // int_bin_half_p
	HI191MIPI_write_cmos_sensor(0x8073, 0x72);            // int_bin_half_p    
	HI191MIPI_write_cmos_sensor(0x8074, 0x01);            // scn_bin_halp_p
	HI191MIPI_write_cmos_sensor(0x8075, 0x72);            // scn_bin_halp_p    
	HI191MIPI_write_cmos_sensor(0x8098, 0x40);            // cdrk_width
	HI191MIPI_write_cmos_sensor(0x8099, 0x00);
	HI191MIPI_write_cmos_sensor(0x809A, 0x08);
	HI191MIPI_write_cmos_sensor(0x809B, 0x08);
	HI191MIPI_write_cmos_sensor(0x80A0, 0x05);            // blc_ctl1
	HI191MIPI_write_cmos_sensor(0x80A1, 0x00);            // blc_ctl2 d --> 5  08
	HI191MIPI_write_cmos_sensor(0x80A2, 0x80);            // blc_ctl3
	HI191MIPI_write_cmos_sensor(0x80A3, 0x00);            // blc_ctl4
	HI191MIPI_write_cmos_sensor(0x80A4, 0x06);            // blc_ctl5
	HI191MIPI_write_cmos_sensor(0x80A5, 0x00);            // blc_ctl6   10-->0   13
	HI191MIPI_write_cmos_sensor(0x80A6, 0x40);            // blc_ctl7   40-->42   13  
	HI191MIPI_write_cmos_sensor(0x80A7, 0x20);            // blc_ctl8                
	HI191MIPI_write_cmos_sensor(0x80AC, 0x02);            // dig_blc_offset
	HI191MIPI_write_cmos_sensor(0x80AD, 0xFF);            // dig_blc_offset    
	HI191MIPI_write_cmos_sensor(0x80AE, 0x0a);            // col_blc_dead_pxl_th 10 --> a 09
	HI191MIPI_write_cmos_sensor(0x80B4, 0x40);            // dig_gb_off_in
	HI191MIPI_write_cmos_sensor(0x80B5, 0x40);            // dig_gr_off_in
	HI191MIPI_write_cmos_sensor(0x80B6, 0x40);            // dig_blu_off_in
	HI191MIPI_write_cmos_sensor(0x80B7, 0x40);            // dig_red_off_in
	HI191MIPI_write_cmos_sensor(0x80C0, 0x1f);            // row_blc_dead_pxl_th 10 --> a  09  
	HI191MIPI_write_cmos_sensor(0x80C1, 0x66);            // curr_frm_obp_avg_wgt (x0.4)  
	HI191MIPI_write_cmos_sensor(0x80C2, 0x10);            // frm_obp_avg_pga_wgt (x0.5) 
	HI191MIPI_write_cmos_sensor(0x8047, 0x00);            // ramp_sig_pofs_nonor
	HI191MIPI_write_cmos_sensor(0x807B, 0x0F);            // ramp_pofs_msb    
	/*****************analog dynamic switch ****************/
	HI191MIPI_write_cmos_sensor(0x8100, 0x01);            // ssamp_flu_pos
	HI191MIPI_write_cmos_sensor(0x8101, 0x5A);            // ssamp_flu_pos    
	HI191MIPI_write_cmos_sensor(0x8102, 0x00);            // ssamp_flu_wth
	HI191MIPI_write_cmos_sensor(0x8103, 0x38);            // ssamp_flu_wth    
	HI191MIPI_write_cmos_sensor(0x8104, 0x00);            // ramp_preset1_ful_pos
	HI191MIPI_write_cmos_sensor(0x8105, 0xDA);            // ramp_preset1_ful_pos    
	HI191MIPI_write_cmos_sensor(0x8106, 0x00);            // ramp_preset1_ful_neg
	HI191MIPI_write_cmos_sensor(0x8107, 0x98);            // ramp_preset1_ful_neg    
	HI191MIPI_write_cmos_sensor(0x8108, 0x02);            // ramp_preset2_ful_pos
	HI191MIPI_write_cmos_sensor(0x8109, 0xDA);            // ramp_preset2_ful_pos    
	HI191MIPI_write_cmos_sensor(0x810A, 0x01);            // ramp_preset2_ful_neg
	HI191MIPI_write_cmos_sensor(0x810B, 0x58);            // ramp_preset2_ful_neg    
	HI191MIPI_write_cmos_sensor(0x810C, 0x00);            // ramp_clk_msk1_ful_pos
	HI191MIPI_write_cmos_sensor(0x810D, 0xD8);            // ramp_clk_msk1_ful_pos    
	HI191MIPI_write_cmos_sensor(0x810E, 0x00);            // ramp_clk_msk1_ful_neg
	HI191MIPI_write_cmos_sensor(0x810F, 0x98);            // ramp_clk_msk1_ful_neg    
	HI191MIPI_write_cmos_sensor(0x8110, 0x02);            // ramp_clk_msk2_ful_pos
	HI191MIPI_write_cmos_sensor(0x8111, 0xD8);            // ramp_clk_msk2_ful_pos    
	HI191MIPI_write_cmos_sensor(0x8112, 0x01);            // ramp_clk_msk2_ful_neg
	HI191MIPI_write_cmos_sensor(0x8113, 0x58);            // ramp_clk_msk2_ful_neg    
	HI191MIPI_write_cmos_sensor(0x8130, 0x00);            // ramp_rst_ofs_ful_pos
	HI191MIPI_write_cmos_sensor(0x8131, 0x86);            // ramp_rst_ofs_ful_pos    
	HI191MIPI_write_cmos_sensor(0x8132, 0x00);            // ramp_rst_ofs_ful_neg
	HI191MIPI_write_cmos_sensor(0x8133, 0xDA);            // ramp_rst_ofs_ful_neg    
	HI191MIPI_write_cmos_sensor(0x8134, 0x02);            // ramp_sig_ofs_ful_neg
	HI191MIPI_write_cmos_sensor(0x8135, 0xDA);            // ramp_sig_ofs_ful_neg    
	HI191MIPI_write_cmos_sensor(0x8136, 0x00);            // ramp_rst_flg_ful_pos
	HI191MIPI_write_cmos_sensor(0x8137, 0x86);            // ramp_rst_flg_ful_pos    
	HI191MIPI_write_cmos_sensor(0x8138, 0x00);            // ramp_rst_flg_ful_neg
	HI191MIPI_write_cmos_sensor(0x8139, 0xDA);            // ramp_rst_flg_ful_neg    
	HI191MIPI_write_cmos_sensor(0x813A, 0x07);            // int_pxl_pwr_ful_pos
	HI191MIPI_write_cmos_sensor(0x813B, 0x14);            // int_pxl_pwr_ful_neg
	HI191MIPI_write_cmos_sensor(0x813E, 0x04);            // int_addr_ful_pos    
	HI191MIPI_write_cmos_sensor(0x8140, 0x00);            // int_addr_ful_neg
	HI191MIPI_write_cmos_sensor(0x8141, 0x46);            // int_addr_ful_neg    
	HI191MIPI_write_cmos_sensor(0x8142, 0x06);            // int_en_ful_pos    
	HI191MIPI_write_cmos_sensor(0x8144, 0x00);            // int_en_ful_neg
	HI191MIPI_write_cmos_sensor(0x8145, 0x44);            // int_en_ful_neg    
	HI191MIPI_write_cmos_sensor(0x8146, 0x08);            // int_sx_ful_pos    
	HI191MIPI_write_cmos_sensor(0x8148, 0x00);            // int_sx_ful_neg
	HI191MIPI_write_cmos_sensor(0x8149, 0x08);            // int_sx_ful_neg    
	HI191MIPI_write_cmos_sensor(0x814A, 0x0E);            // int_rx1_ful_pos    
	HI191MIPI_write_cmos_sensor(0x814C, 0x00);            // int_rx1_ful_neg
	HI191MIPI_write_cmos_sensor(0x814D, 0x40);            // int_rx1_ful_neg    
	HI191MIPI_write_cmos_sensor(0x814E, 0x00);            // int_rx2_ful_pos
	HI191MIPI_write_cmos_sensor(0x814F, 0x0E);            // int_rx2_ful_pos    
	HI191MIPI_write_cmos_sensor(0x8150, 0x00);            // int_rx2_ful_neg
	HI191MIPI_write_cmos_sensor(0x8151, 0x0E);            // int_rx2_ful_neg    
	HI191MIPI_write_cmos_sensor(0x8152, 0x00);            // int_rx3_ful_pos
	HI191MIPI_write_cmos_sensor(0x8153, 0x0E);            // int_rx3_ful_pos    
	HI191MIPI_write_cmos_sensor(0x8154, 0x00);            // int_rx3_ful_neg
	HI191MIPI_write_cmos_sensor(0x8155, 0x0E);            // int_rx3_ful_neg    
	HI191MIPI_write_cmos_sensor(0x8156, 0x12);            // int_tx1_ful_pos    
	HI191MIPI_write_cmos_sensor(0x8158, 0x00);            // int_tx1_ful_neg
	HI191MIPI_write_cmos_sensor(0x8159, 0x36);            // int_tx1_ful_neg    
	HI191MIPI_write_cmos_sensor(0x815A, 0x00);            // int_tx2_ful_pos
	HI191MIPI_write_cmos_sensor(0x815B, 0x12);            // int_tx2_ful_pos    
	HI191MIPI_write_cmos_sensor(0x815C, 0x00);            // int_tx2_ful_neg
	HI191MIPI_write_cmos_sensor(0x815D, 0x12);            // int_tx2_ful_neg    
	HI191MIPI_write_cmos_sensor(0x815E, 0x01);            // scn_addr_ful_pos    
	HI191MIPI_write_cmos_sensor(0x8160, 0x02);            // scn_addr_ful_neg
	HI191MIPI_write_cmos_sensor(0x8161, 0xE2);            // scn_addr_ful_neg    
	HI191MIPI_write_cmos_sensor(0x8162, 0x06);            // scn_pxl_pwr_ful_pos
	HI191MIPI_write_cmos_sensor(0x8163, 0x30);            // scn_pxl_pwr_ful_neg
	HI191MIPI_write_cmos_sensor(0x8166, 0x02);            // scn_en_ful_pos    
	HI191MIPI_write_cmos_sensor(0x8168, 0x02);            // scn_en_ful_neg
	HI191MIPI_write_cmos_sensor(0x8169, 0xDE);            // scn_en_ful_neg    
	HI191MIPI_write_cmos_sensor(0x816A, 0x04);            // scn_sx_ful_pos    
	HI191MIPI_write_cmos_sensor(0x816C, 0x02);            // scn_sx_ful_neg
	HI191MIPI_write_cmos_sensor(0x816D, 0xDA);            // scn_sx_ful_neg    
	HI191MIPI_write_cmos_sensor(0x816E, 0x00);            // scn_tx1_ful_pos
	HI191MIPI_write_cmos_sensor(0x816F, 0xDA);            // scn_tx1_ful_pos    
	HI191MIPI_write_cmos_sensor(0x8170, 0x01);            // scn_tx1_ful_neg
	HI191MIPI_write_cmos_sensor(0x8171, 0x0C);            // scn_tx1_ful_neg    
	HI191MIPI_write_cmos_sensor(0x8172, 0x01);            // scn_tx2_ful_pos
	HI191MIPI_write_cmos_sensor(0x8173, 0xA2);            // scn_tx2_ful_pos    
	HI191MIPI_write_cmos_sensor(0x8174, 0x01);            // scn_tx2_ful_neg
	HI191MIPI_write_cmos_sensor(0x8175, 0xA2);            // scn_tx2_ful_neg    
	HI191MIPI_write_cmos_sensor(0x8176, 0x02);            // scn_tx3_ful_pos
	HI191MIPI_write_cmos_sensor(0x8177, 0xDC);            // scn_tx3_ful_pos    
	HI191MIPI_write_cmos_sensor(0x8178, 0x02);            // scn_tx3_ful_neg
	HI191MIPI_write_cmos_sensor(0x8179, 0xE0);            // scn_tx3_ful_neg    
	HI191MIPI_write_cmos_sensor(0x817A, 0x08);            // scn_rx1_ful_pos    
	HI191MIPI_write_cmos_sensor(0x817C, 0x00);            // sc_rx1_ful_neg
	HI191MIPI_write_cmos_sensor(0x817D, 0x3a);            // sc_rx1_ful_neg    
	HI191MIPI_write_cmos_sensor(0x817E, 0x00);            // sc_rx2_ful_pos
	HI191MIPI_write_cmos_sensor(0x817F, 0x44);            // sc_rx2_ful_pos    
	HI191MIPI_write_cmos_sensor(0x8180, 0x00);            // sc_rx2_ful_neg
	HI191MIPI_write_cmos_sensor(0x8181, 0x44);            // sc_rx2_ful_neg    
	HI191MIPI_write_cmos_sensor(0x8182, 0x02);            // sc_rx3_ful_pos
	HI191MIPI_write_cmos_sensor(0x8183, 0xDC);            // sc_rx3_ful_pos    
	HI191MIPI_write_cmos_sensor(0x8184, 0x02);            // sc_rx3_ful_neg
	HI191MIPI_write_cmos_sensor(0x8185, 0xE0);            // sc_rx3_ful_neg    
	HI191MIPI_write_cmos_sensor(0x8186, 0x00);            // cds_rst_clp_ful_pos
	HI191MIPI_write_cmos_sensor(0x8187, 0x3c);            // cds_rst_clp_ful_pos    
	HI191MIPI_write_cmos_sensor(0x8188, 0x00);            // cds_rst_clp_ful_neg
	HI191MIPI_write_cmos_sensor(0x8189, 0x80);            // cds_rst_clp_ful_neg    
	HI191MIPI_write_cmos_sensor(0x818A, 0x00);            // cds_rst_ful_pos
	HI191MIPI_write_cmos_sensor(0x818B, 0x08);            // cds_rst_ful_pos    
	HI191MIPI_write_cmos_sensor(0x818C, 0x00);            // cds_rst_ful_neg
	HI191MIPI_write_cmos_sensor(0x818D, 0xDA);            // cds_rst_ful_neg    
	HI191MIPI_write_cmos_sensor(0x818E, 0x00);            // cds_sig_ful_pos
	HI191MIPI_write_cmos_sensor(0x818F, 0xDC);            // cds_sig_ful_pos    
	HI191MIPI_write_cmos_sensor(0x8190, 0x02);            // cds_sig_ful_neg
	HI191MIPI_write_cmos_sensor(0x8191, 0xDA);            // cds_sig_ful_neg    
	HI191MIPI_write_cmos_sensor(0x8192, 0x00);            // cds_s1_1_ful_pos
	HI191MIPI_write_cmos_sensor(0x8193, 0x3C);            // cds_s1_1_ful_pos    
	HI191MIPI_write_cmos_sensor(0x8194, 0x00);            // cds_s1_1_ful_neg
	HI191MIPI_write_cmos_sensor(0x8195, 0x84);            // cds_s1_1_ful_neg    
	HI191MIPI_write_cmos_sensor(0x8196, 0x00);            // cds_s2_ful_pos
	HI191MIPI_write_cmos_sensor(0x8197, 0x4a);            // cds_s2_ful_pos    
	HI191MIPI_write_cmos_sensor(0x8198, 0x00);            // cds_s2_ful_neg
	HI191MIPI_write_cmos_sensor(0x8199, 0x7b);            // cds_s2_ful_neg    
	HI191MIPI_write_cmos_sensor(0x819A, 0x00);            // cds_s3_ful_pos
	HI191MIPI_write_cmos_sensor(0x819B, 0x4a);            // cds_s3_ful_pos    
	HI191MIPI_write_cmos_sensor(0x819C, 0x00);            // cds_s3_ful_neg
	HI191MIPI_write_cmos_sensor(0x819D, 0x7b);            // cds_s3_ful_neg    
	HI191MIPI_write_cmos_sensor(0x819E, 0x00);            // cds_s4_ful_pos
	HI191MIPI_write_cmos_sensor(0x819F, 0x90);            // cds_s4_ful_pos    
	HI191MIPI_write_cmos_sensor(0x81A0, 0x02);            // cds_s4_ful_neg
	HI191MIPI_write_cmos_sensor(0x81A1, 0xDA);            // cds_s4_ful_neg    
	HI191MIPI_write_cmos_sensor(0x81A2, 0x00);            // cds_vbias_sample1_ful_pos
	HI191MIPI_write_cmos_sensor(0x81A3, 0x01);            // cds_vbias_sample1_ful_pos    
	HI191MIPI_write_cmos_sensor(0x81A4, 0x00);            // cds_vbias_sample1_ful_neg
	HI191MIPI_write_cmos_sensor(0x81A5, 0x3A);            // cds_vbias_sample1_ful_neg    
	HI191MIPI_write_cmos_sensor(0x81A6, 0x00);            // cds_vbias_sample2_ful_pos
	HI191MIPI_write_cmos_sensor(0x81A7, 0x01);            // cds_vbias_sample2_ful_pos    
	HI191MIPI_write_cmos_sensor(0x81A8, 0x00);            // cds_vbias_sample2_ful_neg
	HI191MIPI_write_cmos_sensor(0x81A9, 0x3A);            // cds_vbias_sample2_ful_neg    
	HI191MIPI_write_cmos_sensor(0x81AA, 0x01);            // cds_s1_2_ful_pos
	HI191MIPI_write_cmos_sensor(0x81AB, 0x0E);            // cds_s1_2_ful_pos    
	HI191MIPI_write_cmos_sensor(0x81AC, 0x01);            // cds_s1_2_ful_neg
	HI191MIPI_write_cmos_sensor(0x81AD, 0x56);            // cds_s1_2_ful_neg    
	HI191MIPI_write_cmos_sensor(0x8196, 0x00);            // cds_s2_ful_pos
	HI191MIPI_write_cmos_sensor(0x8197, 0x4a);            // cds_s2_ful_pos    
	HI191MIPI_write_cmos_sensor(0x8198, 0x00);            // cds_s2_ful_neg
	HI191MIPI_write_cmos_sensor(0x8199, 0x7b);            // cds_s2_ful_neg    
	HI191MIPI_write_cmos_sensor(0x819A, 0x00);            // cds_s3_ful_pos
	HI191MIPI_write_cmos_sensor(0x819B, 0x4a);            // cds_s3_ful_pos    
	HI191MIPI_write_cmos_sensor(0x819C, 0x00);            // cds_s3_ful_neg
	HI191MIPI_write_cmos_sensor(0x819D, 0x7b);            // cds_s3_ful_neg    
	HI191MIPI_write_cmos_sensor(0x819E, 0x00);            // cds_s4_ful_pos
	HI191MIPI_write_cmos_sensor(0x819F, 0x90);            // cds_s4_ful_pos    
	HI191MIPI_write_cmos_sensor(0x81A0, 0x02);            // cds_s4_ful_neg
	HI191MIPI_write_cmos_sensor(0x81A1, 0xDA);            // cds_s4_ful_neg    
	HI191MIPI_write_cmos_sensor(0x81A2, 0x00);            // cds_vbias_sample1_ful_pos
	HI191MIPI_write_cmos_sensor(0x81A3, 0x01);            // cds_vbias_sample1_ful_pos    
	HI191MIPI_write_cmos_sensor(0x81A4, 0x00);            // cds_vbias_sample1_ful_neg
	HI191MIPI_write_cmos_sensor(0x81A5, 0x3A);            // cds_vbias_sample1_ful_neg    
	HI191MIPI_write_cmos_sensor(0x81A6, 0x00);            // cds_vbias_sample2_ful_pos
	HI191MIPI_write_cmos_sensor(0x81A7, 0x01);            // cds_vbias_sample2_ful_pos    
	HI191MIPI_write_cmos_sensor(0x81A8, 0x00);            // cds_vbias_sample2_ful_neg
	HI191MIPI_write_cmos_sensor(0x81A9, 0x3A);            // cds_vbias_sample2_ful_neg  
	HI191MIPI_write_cmos_sensor(0x81B0, 0x00);            // col_init_ful_pos
	HI191MIPI_write_cmos_sensor(0x81B1, 0x0A);            // col_init_ful_pos    
	HI191MIPI_write_cmos_sensor(0x81B2, 0x00);            // col_init_ful_neg
	HI191MIPI_write_cmos_sensor(0x81B3, 0x86);            // col_init_ful_neg    
	HI191MIPI_write_cmos_sensor(0x81B4, 0x02);            // col_load_ful_pos
	HI191MIPI_write_cmos_sensor(0x81B5, 0xDC);            // col_load_ful_pos    
	HI191MIPI_write_cmos_sensor(0x81B6, 0x02);            // col_load_ful_neg
	HI191MIPI_write_cmos_sensor(0x81B7, 0xE2);            // col_load_ful_neg    
	HI191MIPI_write_cmos_sensor(0x81B8, 0x02);            // sxb_ful_pos
	HI191MIPI_write_cmos_sensor(0x81B9, 0xDC);            // sxb_ful_pos    
	HI191MIPI_write_cmos_sensor(0x81BA, 0x00);            // sxb_ful_neg
	HI191MIPI_write_cmos_sensor(0x81BB, 0x02);            // sxb_ful_neg    
	HI191MIPI_write_cmos_sensor(0x81BC, 0x00);            // en_col_addr_ful_pos
	HI191MIPI_write_cmos_sensor(0x81BD, 0x0B);            // en_col_addr_ful_pos    
	HI191MIPI_write_cmos_sensor(0x81BE, 0x02);            // en_col_addr_ful_neg
	HI191MIPI_write_cmos_sensor(0x81BF, 0xBD);            // en_col_addr_ful_neg    
	HI191MIPI_write_cmos_sensor(0x81C0, 0x00);            // cds_pxbias_sample_ful_pos
	HI191MIPI_write_cmos_sensor(0x81C1, 0x01);            // cds_pxbias_sample_ful_pos    
	HI191MIPI_write_cmos_sensor(0x81C2, 0x00);            // cds_pxbias_sample_ful_neg
	HI191MIPI_write_cmos_sensor(0x81C3, 0x3A);            // cds_pxbias_sample_ful_neg    
	HI191MIPI_write_cmos_sensor(0x81C4, 0x00);            // ramp_dn1_ful_pos
	HI191MIPI_write_cmos_sensor(0x81C5, 0xB8);            // ramp_dn1_ful_pos    
	HI191MIPI_write_cmos_sensor(0x81C6, 0x00);            // ramp_dn1_ful_neg
	HI191MIPI_write_cmos_sensor(0x81C7, 0xA8);            // ramp_dn1_ful_neg    
	HI191MIPI_write_cmos_sensor(0x81C8, 0x00);            // ramp_dn2_ful_pos
	HI191MIPI_write_cmos_sensor(0x81C9, 0xDA);            // ramp_dn2_ful_pos    
	HI191MIPI_write_cmos_sensor(0x81CA, 0x00);            // ramp_dn2_ful_neg
	HI191MIPI_write_cmos_sensor(0x81CB, 0xC8);            // ramp_dn2_ful_neg    
	HI191MIPI_write_cmos_sensor(0x81CC, 0x02);            // ramp_dn3_ful_pos
	HI191MIPI_write_cmos_sensor(0x81CD, 0x18);            // ramp_dn3_ful_pos    
	HI191MIPI_write_cmos_sensor(0x81CE, 0x01);            // ramp_dn3_ful_neg
	HI191MIPI_write_cmos_sensor(0x81CF, 0xB8);            // ramp_dn3_ful_neg    
	HI191MIPI_write_cmos_sensor(0x81D0, 0x02);            // ramp_dn4_ful_pos
	HI191MIPI_write_cmos_sensor(0x81D1, 0xDA);            // ramp_dn4_ful_pos    
	HI191MIPI_write_cmos_sensor(0x81D2, 0x02);            // ramp_dn4_ful_neg
	HI191MIPI_write_cmos_sensor(0x81D3, 0x78);            // ramp_dn4_ful_neg    
	HI191MIPI_write_cmos_sensor(0x81D4, 0x00);            // ramp_lsb1c1_ful_pos
	HI191MIPI_write_cmos_sensor(0x81D5, 0xA8);            // ramp_lsb1c1_ful_pos    
	HI191MIPI_write_cmos_sensor(0x81D6, 0x00);            // ramp_lsb1c1_ful_neg
	HI191MIPI_write_cmos_sensor(0x81D7, 0xDA);            // ramp_lsb1c1_ful_neg    
	HI191MIPI_write_cmos_sensor(0x81D8, 0x01);            // ramp_lsb1c2_ful_pos
	HI191MIPI_write_cmos_sensor(0x81D9, 0xB8);            // ramp_lsb1c2_ful_pos    
	HI191MIPI_write_cmos_sensor(0x81DA, 0x02);            // ramp_lsb1c2_ful_neg
	HI191MIPI_write_cmos_sensor(0x81DB, 0xDA);            // ramp_lsb1c2_ful_neg    
	HI191MIPI_write_cmos_sensor(0x81DC, 0x00);            // ramp_lsb2c1_ful_pos
	HI191MIPI_write_cmos_sensor(0x81DD, 0xB8);            // ramp_lsb2c1_ful_pos    
	HI191MIPI_write_cmos_sensor(0x81DE, 0x00);            // ramp_lsb2c1_ful_neg
	HI191MIPI_write_cmos_sensor(0x81DF, 0xDA);            // ramp_lsb2c1_ful_neg    
	HI191MIPI_write_cmos_sensor(0x81E0, 0x02);            // ramp_lsb2c2_ful_pos
	HI191MIPI_write_cmos_sensor(0x81E1, 0x18);            // ramp_lsb2c2_ful_pos    
	HI191MIPI_write_cmos_sensor(0x81E2, 0x02);            // ramp_lsb2c2_ful_neg
	HI191MIPI_write_cmos_sensor(0x81E3, 0xDA);            // ramp_lsb2c2_ful_neg    
	HI191MIPI_write_cmos_sensor(0x81E4, 0x00);            // ramp_lsb3c1_ful_pos
	HI191MIPI_write_cmos_sensor(0x81E5, 0xC8);            // ramp_lsb3c1_ful_pos    
	HI191MIPI_write_cmos_sensor(0x81E6, 0x00);            // ramp_lsb3c1_ful_neg
	HI191MIPI_write_cmos_sensor(0x81E7, 0xDA);            // ramp_lsb3c1_ful_neg    
	HI191MIPI_write_cmos_sensor(0x81E8, 0x02);            // ramp_lsb3c2_ful_pos
	HI191MIPI_write_cmos_sensor(0x81E9, 0x78);            // ramp_lsb3c2_ful_pos    
	HI191MIPI_write_cmos_sensor(0x81EA, 0x02);            // ramp_lsb3c2_ful_neg
	HI191MIPI_write_cmos_sensor(0x81EB, 0xDA);            // ramp_lsb3c2_ful_neg    
	HI191MIPI_write_cmos_sensor(0x81EC, 0x00);            // cds_bn_sw_n_ctl_pos
	HI191MIPI_write_cmos_sensor(0x81ED, 0x3C);            // cds_bn_sw_n_ctl_pos    
	HI191MIPI_write_cmos_sensor(0x81EE, 0x00);            // cds_bn_sw_n_ctl_neg
	HI191MIPI_write_cmos_sensor(0x81EF, 0x84);            // cds_bn_sw_n_ctl_neg    
	HI191MIPI_write_cmos_sensor(0x81F0, 0x01);            // cds_sig_clp_ful_pos
	HI191MIPI_write_cmos_sensor(0x81F1, 0x0C);            // cds_sig_clp_ful_pos    
	HI191MIPI_write_cmos_sensor(0x81F2, 0x01);            // cds_sig_clp_ful_neg
	HI191MIPI_write_cmos_sensor(0x81F3, 0x5A);            // cds_sig_clp_ful_neg    
	HI191MIPI_write_cmos_sensor(0x81F4, 0x00);            // cds_col_bin_pos
	HI191MIPI_write_cmos_sensor(0x81F5, 0xC8);            // cds_col_bin_pos    
	HI191MIPI_write_cmos_sensor(0x81F6, 0x00);            // cds_col_bin_neg
	HI191MIPI_write_cmos_sensor(0x81F7, 0xC8);            // cds_col_bin_neg    
	HI191MIPI_write_cmos_sensor(0x8200, 0x12);            // int_tx1_odd_ln_ful_pos    
	HI191MIPI_write_cmos_sensor(0x8202, 0x00);            // int_tx1_odd_ln_ful_neg
	HI191MIPI_write_cmos_sensor(0x8203, 0x36);            // int_tx1_odd_ln_ful_neg    
	HI191MIPI_write_cmos_sensor(0x8204, 0x0E);            // int_rx1_odd_ln_ful_pos    
	HI191MIPI_write_cmos_sensor(0x8206, 0x00);            // int_rx1_odd_ln_ful_neg
	HI191MIPI_write_cmos_sensor(0x8207, 0x40);            // int_rx1_odd_ln_ful_neg    
	HI191MIPI_write_cmos_sensor(0x8208, 0x00);            // scn_tx1_odd_ln_ful_pos
	HI191MIPI_write_cmos_sensor(0x8209, 0xDA);            // scn_tx1_odd_ln_ful_pos    
	HI191MIPI_write_cmos_sensor(0x820A, 0x01);            // scn_tx1_odd_ln_ful_neg
	HI191MIPI_write_cmos_sensor(0x820B, 0x0C);            // scn_tx1_odd_ln_ful_neg    
	HI191MIPI_write_cmos_sensor(0x820C, 0x08);            // scn_rx1_odd_ln_ful_pos    
	HI191MIPI_write_cmos_sensor(0x820E, 0x00);            // scn_tx1_odd_ln_ful_neg
	HI191MIPI_write_cmos_sensor(0x820F, 0x3A);            // scn_tx1_odd_ln_ful_neg    
	HI191MIPI_write_cmos_sensor(0x8210, 0x06);            // scn_pxl_pwr_odd_ln_ful_pos
	HI191MIPI_write_cmos_sensor(0x8211, 0x14);            // scn_pxl_pwr_odd_ln_ful_neg
	/***********pll on **************/                                   
	HI191MIPI_write_cmos_sensor(0x5000, 0x1B);            // clkcon
	HI191MIPI_write_cmos_sensor(0x5003, 0x55);            // mode_pll1
	HI191MIPI_write_cmos_sensor(0x5004, 0x2D);            // mode_pll2  0x5004
	//HI191MIPI_write_cmos_sensor(0x5004, 0x0A);            // mode_pll2
	HI191MIPI_write_cmos_sensor(0x5005, 0x35);            // mode_pll3 
	HI191MIPI_write_cmos_sensor(0x5006, 0xD3);            // mode_pll4
	HI191MIPI_write_cmos_sensor(0x5007, 0x66);            // mode_pll5
	HI191MIPI_write_cmos_sensor(0x5008, 0x0F);            // mode_pll6
	HI191MIPI_write_cmos_sensor(0x5010, 0x80);            // mode_io
	HI191MIPI_write_cmos_sensor(0x5014, 0x0F);            // frame_end_delay
	/*********** ADPC set file**************/   
	HI191MIPI_write_cmos_sensor(0xA000, 0x02);            // adpc enable
	HI191MIPI_write_cmos_sensor(0xA200, 0x18);            // adpc_ctl1
	HI191MIPI_write_cmos_sensor(0xA21E, 0x01);            // adpc_start_x
	HI191MIPI_write_cmos_sensor(0xA21F, 0x1E);            // adpc_start_x
	HI191MIPI_write_cmos_sensor(0xA401, 0x01);            // fmt_ctl2
	HI191MIPI_write_cmos_sensor(0x804e, 0x20);
	HI191MIPI_write_cmos_sensor(0x8051, 0x88);
	HI191MIPI_write_cmos_sensor(0x808a, 0x05);
	HI191MIPI_write_cmos_sensor(0x817d, 0x48);
	HI191MIPI_write_cmos_sensor(0x8197, 0x44);
	HI191MIPI_write_cmos_sensor(0x8199, 0x7a);
	HI191MIPI_write_cmos_sensor(0x819b, 0x50);
	HI191MIPI_write_cmos_sensor(0x819d, 0x5a);
	HI191MIPI_write_cmos_sensor(0x8187, 0x45);
	HI191MIPI_write_cmos_sensor(0x8189, 0x7f);
	HI191MIPI_write_cmos_sensor(0x8131, 0xa0);
	HI191MIPI_write_cmos_sensor(0x8137, 0xa0);
	HI191MIPI_write_cmos_sensor(0x816f, 0xda);
	HI191MIPI_write_cmos_sensor(0x8170, 0x01);
	HI191MIPI_write_cmos_sensor(0x8171, 0x00);
	HI191MIPI_write_cmos_sensor(0x803C, 0x0C);            // softreset only
	HI191MIPI_write_cmos_sensor(0x807c, 0x7f);
	HI191MIPI_write_cmos_sensor(0x807d, 0x7f);
	HI191MIPI_write_cmos_sensor(0x807e, 0x7f);
	HI191MIPI_write_cmos_sensor(0x8081, 0xC3);            // cds_ctl5 
	HI191MIPI_write_cmos_sensor(0x8043, 0x73);
	HI191MIPI_write_cmos_sensor(0x80a1, 0x4f);            // line OBP DPC on
	HI191MIPI_write_cmos_sensor(0x80ae, 0x01);            // line OBP DPC threshold
	HI191MIPI_write_cmos_sensor(0x80c0, 0x1f);            // frame OBP DPC threshold
	HI191MIPI_write_cmos_sensor(0x805a, 0x07);           
	HI191MIPI_write_cmos_sensor(0x80a6, 0x46);
	HI191MIPI_write_cmos_sensor(0x0b04, 0x01);
	HI191MIPI_write_cmos_sensor(0x800d, 0x80);
	HI191MIPI_write_cmos_sensor(0x816e, 0x01);            // tx
	HI191MIPI_write_cmos_sensor(0x816f, 0x23);            
	HI191MIPI_write_cmos_sensor(0x8170, 0x01);            
	HI191MIPI_write_cmos_sensor(0x8171, 0x43);            
	HI191MIPI_write_cmos_sensor(0x8130, 0x00);            // r offset 
	HI191MIPI_write_cmos_sensor(0x8131, 0x90);            
	HI191MIPI_write_cmos_sensor(0x8132, 0x01);            // r offset 
	HI191MIPI_write_cmos_sensor(0x8133, 0x1e);            
	HI191MIPI_write_cmos_sensor(0x810e, 0x00);            
	HI191MIPI_write_cmos_sensor(0x810f, 0xdc);            
	HI191MIPI_write_cmos_sensor(0x810c, 0x00);            
	HI191MIPI_write_cmos_sensor(0x810d, 0xdc);            
	HI191MIPI_write_cmos_sensor(0x8132, 0x01);            
	HI191MIPI_write_cmos_sensor(0x8133, 0x1e);            
	HI191MIPI_write_cmos_sensor(0x8136, 0x00);            // flag
	HI191MIPI_write_cmos_sensor(0x8137, 0xa0);            
	HI191MIPI_write_cmos_sensor(0x8138, 0x01);            // flag
	HI191MIPI_write_cmos_sensor(0x8139, 0x1e);
	HI191MIPI_write_cmos_sensor(0x810c, 0x01);
	HI191MIPI_write_cmos_sensor(0x810d, 0x1c);
	HI191MIPI_write_cmos_sensor(0x8104, 0x01);
	HI191MIPI_write_cmos_sensor(0x8105, 0x1c);
	HI191MIPI_write_cmos_sensor(0x80ad, 0xfe);
	/******************bias sample***************/
	HI191MIPI_write_cmos_sensor(0x8039, 0x00);            // bias1,2 samp
	HI191MIPI_write_cmos_sensor(0x81a2, 0x00);            // vbias1_samp
	HI191MIPI_write_cmos_sensor(0x81a3, 0x01);
	HI191MIPI_write_cmos_sensor(0x81a4, 0x00);
	HI191MIPI_write_cmos_sensor(0x81a5, 0x44);
	HI191MIPI_write_cmos_sensor(0x81a6, 0x00);            // vbias2_sam
	HI191MIPI_write_cmos_sensor(0x81a7, 0x01);
	HI191MIPI_write_cmos_sensor(0x81a8, 0x00);
	HI191MIPI_write_cmos_sensor(0x81a9, 0x44);
	/****************NEW setting***********************/
	HI191MIPI_write_cmos_sensor(0x8041, 0x02);  //4-->2
	HI191MIPI_write_cmos_sensor(0x805a, 0x1c);  //7-->1c 
	HI191MIPI_write_cmos_sensor(0x805B, 0x1c);            
	HI191MIPI_write_cmos_sensor(0x805C, 0x1c);  //7-->1c          
	HI191MIPI_write_cmos_sensor(0x805D, 0x1c);            
	/***************Gain linearity*******************/
	HI191MIPI_write_cmos_sensor(0x8043, 0x50);
	HI191MIPI_write_cmos_sensor(0x807c, 0x74);
	HI191MIPI_write_cmos_sensor(0x807d, 0x74); 
	HI191MIPI_write_cmos_sensor(0x807e, 0x74);
	/*********************DD option OFF***************/
	HI191MIPI_write_cmos_sensor(0x807a, 0x3F);   
	HI191MIPI_write_cmos_sensor(0x8041, 0x08);  
	/***********************M2***********************/
	HI191MIPI_write_cmos_sensor(0x8136, 0x00);         // rst_flag
	HI191MIPI_write_cmos_sensor(0x8137, 0xa0);        
	HI191MIPI_write_cmos_sensor(0x8138, 0x01);       
	HI191MIPI_write_cmos_sensor(0x8139, 0x1e);
	HI191MIPI_write_cmos_sensor(0x810e, 0x00);         // clk_msk  rst
	HI191MIPI_write_cmos_sensor(0x810f, 0xc8);            
	HI191MIPI_write_cmos_sensor(0x810c, 0x01);            
	HI191MIPI_write_cmos_sensor(0x810d, 0x1c);            
	HI191MIPI_write_cmos_sensor(0x8132, 0x01);		// Mid point of pofs            
	HI191MIPI_write_cmos_sensor(0x8133, 0x1e);            
	HI191MIPI_write_cmos_sensor(0x8106, 0x00);		// preset rst           
	HI191MIPI_write_cmos_sensor(0x8107, 0xc6);            
	HI191MIPI_write_cmos_sensor(0x8104, 0x01);            
	HI191MIPI_write_cmos_sensor(0x8105, 0x1e);            
	HI191MIPI_write_cmos_sensor(0x8051, 0x84);		// CDS BIAS1
	HI191MIPI_write_cmos_sensor(0x8041, 0x88);		// RAMP CTRL 08->88
	HI191MIPI_write_cmos_sensor(0x8078, 0x0F);		// DN CTRL BF->3F->0F
	HI191MIPI_write_cmos_sensor(0x81C4, 0x00);		// DN 
	HI191MIPI_write_cmos_sensor(0x81C5, 0xc6);
	HI191MIPI_write_cmos_sensor(0x81C6, 0x00);
	HI191MIPI_write_cmos_sensor(0x81C7, 0xF2);
	HI191MIPI_write_cmos_sensor(0x81C8, 0x01);
	HI191MIPI_write_cmos_sensor(0x81C9, 0x56);
	HI191MIPI_write_cmos_sensor(0x81CA, 0x02);
	HI191MIPI_write_cmos_sensor(0x81CB, 0x18);
	HI191MIPI_write_cmos_sensor(0x81CC, 0x05);
	HI191MIPI_write_cmos_sensor(0x81CD, 0xB6);
	HI191MIPI_write_cmos_sensor(0x81CE, 0x05);
	HI191MIPI_write_cmos_sensor(0x81CF, 0xB6);
	HI191MIPI_write_cmos_sensor(0x81D0, 0x05);
	HI191MIPI_write_cmos_sensor(0x81D1, 0xB6);
	HI191MIPI_write_cmos_sensor(0x81D2, 0x05);
	HI191MIPI_write_cmos_sensor(0x81D3, 0xB6);
	HI191MIPI_write_cmos_sensor(0x81D4, 0x00);		// LSB1
	HI191MIPI_write_cmos_sensor(0x81D5, 0xF3);
	HI191MIPI_write_cmos_sensor(0x81D6, 0x01);
	HI191MIPI_write_cmos_sensor(0x81D7, 0x1c);
	HI191MIPI_write_cmos_sensor(0x81D8, 0x02);
	HI191MIPI_write_cmos_sensor(0x81D9, 0x19);
	HI191MIPI_write_cmos_sensor(0x81DA, 0x05);
	HI191MIPI_write_cmos_sensor(0x81DB, 0xB6);
	HI191MIPI_write_cmos_sensor(0x81DC, 0x05);
	HI191MIPI_write_cmos_sensor(0x81DD, 0xB6);
	HI191MIPI_write_cmos_sensor(0x81DE, 0x05);
	HI191MIPI_write_cmos_sensor(0x81DF, 0xB6);
	HI191MIPI_write_cmos_sensor(0x80AC, 0x03);            // dig_blc_offset
	HI191MIPI_write_cmos_sensor(0x80AD, 0x4d);            // dig_blc_offset   
	/***********************expose start*************/
	HI191MIPI_write_cmos_sensor(0x0200, 0x00);            // fine integration
	HI191MIPI_write_cmos_sensor(0x0201, 0x50);            // fine integration
	HI191MIPI_write_cmos_sensor(0x813E, 0x02);            // int_addr_ful_pos    
	HI191MIPI_write_cmos_sensor(0x8140, 0x00);            // int_addr_ful_neg
	HI191MIPI_write_cmos_sensor(0x8141, 0x28);            // int_addr_ful_neg    
	HI191MIPI_write_cmos_sensor(0x8142, 0x03);            // int_en_ful_pos    
	HI191MIPI_write_cmos_sensor(0x8144, 0x00);            // int_en_ful_neg
	HI191MIPI_write_cmos_sensor(0x8145, 0x26);            // int_en_ful_neg    
	HI191MIPI_write_cmos_sensor(0x8146, 0x08);            // int_sx_ful_pos    
	HI191MIPI_write_cmos_sensor(0x8148, 0x00);            // int_sx_ful_neg
	HI191MIPI_write_cmos_sensor(0x8149, 0x08);            // int_sx_ful_neg    
	HI191MIPI_write_cmos_sensor(0x814A, 0x05);            // int_rx1_ful_pos    
	HI191MIPI_write_cmos_sensor(0x814C, 0x00);            // int_rx1_ful_neg
	HI191MIPI_write_cmos_sensor(0x814D, 0x25);            // int_rx1_ful_neg    
	HI191MIPI_write_cmos_sensor(0x814E, 0x00);            // int_rx2_ful_pos
	HI191MIPI_write_cmos_sensor(0x814F, 0x04);            // int_rx2_ful_pos    
	HI191MIPI_write_cmos_sensor(0x8150, 0x00);            // int_rx2_ful_neg
	HI191MIPI_write_cmos_sensor(0x8151, 0x04);            // int_rx2_ful_neg    
	HI191MIPI_write_cmos_sensor(0x8152, 0x00);            // int_rx3_ful_pos
	HI191MIPI_write_cmos_sensor(0x8153, 0x04);            // int_rx3_ful_pos    
	HI191MIPI_write_cmos_sensor(0x8154, 0x00);            // int_rx3_ful_neg
	HI191MIPI_write_cmos_sensor(0x8155, 0x04);            // int_rx3_ful_neg    
	HI191MIPI_write_cmos_sensor(0x8156, 0x06);            // int_tx1_ful_pos    
	HI191MIPI_write_cmos_sensor(0x8158, 0x00);            // int_tx1_ful_neg
	HI191MIPI_write_cmos_sensor(0x8159, 0x22);            // int_tx1_ful_neg    
	HI191MIPI_write_cmos_sensor(0x815A, 0x00);            // int_tx2_ful_pos
	HI191MIPI_write_cmos_sensor(0x815B, 0x06);            // int_tx2_ful_pos    
	HI191MIPI_write_cmos_sensor(0x815C, 0x00);            // int_tx2_ful_neg
	HI191MIPI_write_cmos_sensor(0x815D, 0x06);            // int_tx2_ful_neg    
	/**************************digtal_gain*****************************/
	HI191MIPI_write_cmos_sensor(0xA000, 0x03);            // en_dgain 
	HI191MIPI_write_cmos_sensor(0xA160, 0x08);            // dgain_gr_h 
	HI191MIPI_write_cmos_sensor(0xA161, 0x00);            // dgain_gr_l 
	HI191MIPI_write_cmos_sensor(0xA162, 0x08);            // dgain_r_h 
	HI191MIPI_write_cmos_sensor(0xA163, 0x00);            // dgain_r_l 
	HI191MIPI_write_cmos_sensor(0xA164, 0x08);            // dgain_b_h 
	HI191MIPI_write_cmos_sensor(0xA165, 0x00);            // dgain_b_l 
	HI191MIPI_write_cmos_sensor(0xA166, 0x08);            // dgain_gb_h 
	HI191MIPI_write_cmos_sensor(0xA167, 0x00);            // dgain_gb_l 
	HI191MIPI_write_cmos_sensor(0x800A, 0x00);            // drk_y_addr_s 
	HI191MIPI_write_cmos_sensor(0x800B, 0x05);            // drk_y_addr_e
	HI191MIPI_write_cmos_sensor(0x809A, 0x08);            // r_rdrk_height
	                                     
	HI191MIPI_write_cmos_sensor(0xA000, 0x02);            // dgain_diable
	HI191MIPI_write_cmos_sensor(0x80A0, 0x01);            // blc_ctl1 = blc offset disable
	HI191MIPI_write_cmos_sensor(0x8002, 0x00);            // tg_ctl3 = x,y auto size disable 
	HI191MIPI_write_cmos_sensor(0x0340, 0x02);            // frame_length_lines
	HI191MIPI_write_cmos_sensor(0x0341, 0xFA);            // frame_length_lines = 30.36fps   
	HI191MIPI_write_cmos_sensor(0x0344, 0x00);            // x_addr_start
	HI191MIPI_write_cmos_sensor(0x0345, 0x22);            // x_addr_start = 34
	HI191MIPI_write_cmos_sensor(0x0346, 0x00);            // y_addr_start
	HI191MIPI_write_cmos_sensor(0x0347, 0x0E);            // y_addr_start = 14
	HI191MIPI_write_cmos_sensor(0x0348, 0x05);            // x_addr_end
	HI191MIPI_write_cmos_sensor(0x0349, 0x29);            // x_addr_end = 1321   
	HI191MIPI_write_cmos_sensor(0x034A, 0x02);            // y_addr_end
	HI191MIPI_write_cmos_sensor(0x034B, 0xDF);            // y_addr_end = 735
	HI191MIPI_write_cmos_sensor(0x034C, 0x05);            // x_output_size
	HI191MIPI_write_cmos_sensor(0x034D, 0x04);            // x_output_size = 1284
	HI191MIPI_write_cmos_sensor(0x034E, 0x02);            // y_output_size
	HI191MIPI_write_cmos_sensor(0x034F, 0xD2);            // y_output_size = 722
	HI191MIPI_write_cmos_sensor(0xB000, 0x03);            // mipi_tx_op_en
	HI191MIPI_write_cmos_sensor(0xB004, 0x70);            // mipi_tx_test_mode 
	HI191MIPI_write_cmos_sensor(0xB006, 0x01);            // mipi_fifo_ctrl

	HI191MIPI_write_cmos_sensor(0x0800, 0x57);  // tclk_post
	HI191MIPI_write_cmos_sensor(0x0801, 0x17);  // ths_prepare   80ns 
	HI191MIPI_write_cmos_sensor(0x0802, 0x37);  // ths_zero
	HI191MIPI_write_cmos_sensor(0x0803, 0x1F);  // ths_trail    94ns
	HI191MIPI_write_cmos_sensor(0x0804, 0x1F);  // tclk_trail 87ns
	HI191MIPI_write_cmos_sensor(0x0805, 0x17);  // tclk_prepare  82ns  
	HI191MIPI_write_cmos_sensor(0x0806, 0x67);  // tclk_zero
	HI191MIPI_write_cmos_sensor(0x0807, 0x1F);  // tlpx

	HI191MIPI_write_cmos_sensor(0x0800, 0x57); // tclk_post
	HI191MIPI_write_cmos_sensor(0x0801, 0x0f);//1F},  // ths_prepare   80ns 
	HI191MIPI_write_cmos_sensor(0x0802, 0x37);  // ths_zero
	HI191MIPI_write_cmos_sensor(0x0803, 0x28);//1F},  // ths_trail    94ns
	HI191MIPI_write_cmos_sensor(0x0804, 0x38);//1F},  // tclk_trail 87ns
	HI191MIPI_write_cmos_sensor(0x0805, 0x0f);  //17},  // tclk_prepare  82ns  
	HI191MIPI_write_cmos_sensor(0x0806, 0x67);  // tclk_zero
	HI191MIPI_write_cmos_sensor(0x0807, 0x1F);  // tlpx

	HI191MIPI_write_cmos_sensor(0xB008, 0x05);  // mipi_value_clk_pre 46ns
	HI191MIPI_write_cmos_sensor(0xB00A, 0x05);  // mipi_value_clk_exit
	HI191MIPI_write_cmos_sensor(0xB00B, 0x05);  // mipi_value_data_exit
	HI191MIPI_write_cmos_sensor(0x0101, 0x01);
	HI191MIPI_write_cmos_sensor(0x0100, 0x01);            // mode_select
	SENSORDB("[HI191MIPI]exit HI191MIPI_init function \n");
}

void HI191MIPI_HVMirror(kal_uint8 image_mirror)
{
#if 0
    switch (imgMirror)
    {
        case IMAGE_H_MIRROR://IMAGE_NORMAL:
            HI191MIPI_write_cmos_sensor(0x0101,0x00);//Set normal
            break;
        case IMAGE_NORMAL://IMAGE_V_MIRROR:
             HI191MIPI_write_cmos_sensor(0x0101,0x01);//Set V_MIRROR
            break;
        case IMAGE_HV_MIRROR://IMAGE_H_MIRROR:
             HI191MIPI_write_cmos_sensor(0x0101,0x02);//Set H_MIRROR
            break;
        case IMAGE_V_MIRROR://IMAGE_HV_MIRROR:
             HI191MIPI_write_cmos_sensor(0x0101,0x03);//Set HV_MIRROR
            break;
    }

#endif
}



/*****************************************************************************/
/* Windows Mobile Sensor Interface */
/*****************************************************************************/
/*************************************************************************
* FUNCTION
*	HI191MIPIOpen
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

UINT32 HI191MIPIOpen(void)
{
	kal_uint32 id=0;
	SENSORDB("[HI191MIPI]enter HI191MIPIOpen function \n");
	if(HI191MIPI_SENSOR_ID!=HI191MIPI_sensor_id)
	{    	 
		id= ((HI191MIPI_read_cmos_sensor(0x0000) << 8) | HI191MIPI_read_cmos_sensor(0x0001));
		spin_lock(&hi191mipiraw_drv_lock);
	    HI191MIPI_sensor_id=id;
		spin_unlock(&hi191mipiraw_drv_lock);
		if (HI191MIPI_sensor_id != HI191MIPI_SENSOR_ID)
		{
			return ERROR_SENSOR_CONNECT_FAIL;
	    }
	}
	SENSORDB("[ylf]HI191MIPI Sensor Read ID:0x%04x OK\n",HI191MIPI_sensor_id);
	HI191MIPI_init();
	spin_lock(&hi191mipiraw_drv_lock);
	HI191MIPI_state=1;
	HI191MIPI_sensor.pv_mode=KAL_TRUE;
	HI191MIPI_sensor.shutter=0x100;
	spin_unlock(&hi191mipiraw_drv_lock);
	SENSORDB("[HI191MIPI]exit HI191MIPIOpen function \n");
	return ERROR_NONE;
}


UINT32 HI191MIPIGetSensorID(UINT32 *sensorID)
{
	int  retry = 3; 
	SENSORDB("[HI191MIPI]enter HI191MIPIGetSensorID function \n");
    // check if sensor ID correct
    do 
	{
        *sensorID = ((HI191MIPI_read_cmos_sensor(0x0000) << 8) | HI191MIPI_read_cmos_sensor(0x0001));
		spin_lock(&hi191mipiraw_drv_lock);
		HI191MIPI_sensor_id=*sensorID;
		spin_unlock(&hi191mipiraw_drv_lock);
        if (*sensorID == HI191MIPI_SENSOR_ID)
            break; 
        SENSORDB("[ylf]0v2722 Read Sensor ID Fail = 0x%04x\n", *sensorID); 
        retry--; 
    } while (retry > 0);

    if (*sensorID != HI191MIPI_SENSOR_ID) 
	{
        *sensorID = 0xFFFFFFFF; 
        return ERROR_SENSOR_CONNECT_FAIL;
    }
	SENSORDB("[HI191MIPI]exit HI191MIPIGetSensorID function \n");
    return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
*	HI191MIPIClose
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
UINT32 HI191MIPIClose(void)
{
   SENSORDB("[HI191MIPI]enter HI191MIPIClose function \n");
  //CISModulePowerOn(FALSE);
//	DRV_I2CClose(HI191MIPIhDrvI2C);
	return ERROR_NONE;
}   /* HI191MIPIClose */
UINT32 HI191MIPISetTestPatternMode(kal_bool bEnable)
{
    SENSORDB("[HI191MIPI]enter HI191MIPISetTestPatternMode function bEnable=%d \n",bEnable);   
    if(bEnable) 
	{   // enable color bar
		HI191test_pattern_flag=TRUE; 
		HI191MIPI_write_cmos_sensor(0x0601,0x02);
    } 
	else 
	{   
		HI191test_pattern_flag=FALSE;
		HI191MIPI_write_cmos_sensor(0x0601,0x00);
    }
	SENSORDB("[HI191MIPI]exit HI191MIPISetTestPatternMode function \n");  
    return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
* HI191MIPIPreview
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
UINT32 HI191MIPIPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	kal_uint16 startx=0,starty=0;
	kal_uint16 dummy_line = 0;
	SENSORDB("[HI191MIPI]enter HI191MIPIPreview function \n");
	if(HI191MIPI_state==1)
	{
		spin_lock(&hi191mipiraw_drv_lock);
		HI191MIPI_state=0;
		spin_unlock(&hi191mipiraw_drv_lock);
	}
	spin_lock(&hi191mipiraw_drv_lock);
	HI191MIPI_sensor.pv_mode = KAL_TRUE;
	spin_unlock(&hi191mipiraw_drv_lock);
	switch (sensor_config_data->SensorOperationMode)
	{
		case MSDK_SENSOR_OPERATION_MODE_VIDEO: 
			spin_lock(&hi191mipiraw_drv_lock);
			HI191MIPI_sensor.video_mode = KAL_TRUE;
			spin_unlock(&hi191mipiraw_drv_lock);		
		    SENSORDB("[ylf]Video mode \n");
		break;
		default: /* ISP_PREVIEW_MODE */
			spin_lock(&hi191mipiraw_drv_lock);
			HI191MIPI_sensor.video_mode = KAL_FALSE;
			spin_unlock(&hi191mipiraw_drv_lock);
			SENSORDB("[ylf]Camera preview mode \n");
		break;
	}
	//HI191MIPI_HVMirror(sensor_config_data->SensorImageMirror);
	HI191MIPI_Set_Dummy(0, dummy_line); /* modify dummy_pixel must gen AE table again */
	SENSORDB("[HI191MIPI]exit HI191MIPIPreview function \n");
	return ERROR_NONE;	
}   /*  HI191MIPIPreview   */

/*************************************************************************
* FUNCTION
*	HI191MIPICapture
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
UINT32 HI191MIPICapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
						  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	const kal_uint16 pv_line_length = HI191MIPI_sensor.line_length;
	//const kal_uint32 pv_pclk = HI191MIPI_sensor.pclk;
	kal_uint16 shutter = HI191MIPI_sensor.shutter;
	kal_uint16 dummy_pixel=0,dummy_line=0, cap_fps;
	kal_uint16 startx=0,starty=0;
	SENSORDB("HI191MIPICapture \n");
	spin_lock(&hi191mipiraw_drv_lock);
	HI191MIPI_state=1;
	spin_unlock(&hi191mipiraw_drv_lock);

	spin_lock(&hi191mipiraw_drv_lock);
    HI191MIPI_sensor.video_mode=KAL_FALSE;
	spin_unlock(&hi191mipiraw_drv_lock);	
	HI191MIPI_HVMirror(sensor_config_data->SensorImageMirror);
	spin_lock(&hi191mipiraw_drv_lock);
	HI191MIPI_sensor.pv_mode = KAL_FALSE;
	spin_unlock(&hi191mipiraw_drv_lock);
	startx+=HI191MIPI_FULL_X_START;
	starty+=HI191MIPI_FULL_Y_START;
	image_window->GrabStartX= startx;
    image_window->GrabStartY= starty;
    image_window->ExposureWindowWidth= HI191MIPI_IMAGE_SENSOR_PV_WIDTH - 2*startx;
    image_window->ExposureWindowHeight= HI191MIPI_IMAGE_SENSOR_PV_HEIGHT - 2*starty;
	HI191MIPI_Set_Dummy(0, dummy_line); /* modify dummy_pixel must gen AE table again */
	return ERROR_NONE;
}   /* HI191MIPI_Capture() */  

UINT32 HI191MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
    //SENSORDB("[HI191MIPI]enter HI191MIPIGetResolution function pSensorResolution=%x\n",*pSensorResolution);
    pSensorResolution->SensorPreviewWidth=HI191MIPI_IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorPreviewHeight=HI191MIPI_IMAGE_SENSOR_PV_HEIGHT;
	pSensorResolution->SensorFullWidth=HI191MIPI_IMAGE_SENSOR_FULL_WIDTH;
	pSensorResolution->SensorFullHeight=HI191MIPI_IMAGE_SENSOR_FULL_HEIGHT;
	//Gionee yanggy 2012-12-10 add for rear camera begin
    pSensorResolution->SensorVideoWidth= HI191MIPI_IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorVideoHeight=HI191MIPI_IMAGE_SENSOR_PV_HEIGHT;
    pSensorResolution->Sensor3DPreviewWidth=HI191MIPI_IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->Sensor3DPreviewHeight=HI191MIPI_IMAGE_SENSOR_PV_HEIGHT;
    pSensorResolution->Sensor3DFullWidth=HI191MIPI_IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->Sensor3DFullHeight=HI191MIPI_IMAGE_SENSOR_PV_HEIGHT;
    pSensorResolution->Sensor3DVideoWidth=HI191MIPI_IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->Sensor3DVideoHeight=HI191MIPI_IMAGE_SENSOR_PV_HEIGHT;
	//Gionee yanggy 2012-12-10 add for rear camera end
	SENSORDB("[HI191MIPI]exit HI191MIPIGetResolution function SensorPreviewWidth=%d,SensorFullWidth=%d \n",pSensorResolution->SensorPreviewWidth,pSensorResolution->SensorFullWidth);
	return ERROR_NONE;
}	/* HI191MIPIGetResolution() */

UINT32 HI191MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
					  MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	SENSORDB("[HI191MIPI]enter HI191MIPIGetInfo function FeatureId:%d\n",ScenarioId);
    switch(ScenarioId)
    {
    	case MSDK_SCENARIO_ID_CAMERA_ZSD:
			pSensorInfo->SensorPreviewResolutionX=HI191MIPI_IMAGE_SENSOR_PV_WIDTH;
			pSensorInfo->SensorPreviewResolutionY=HI191MIPI_IMAGE_SENSOR_PV_HEIGHT;
			pSensorInfo->SensorCameraPreviewFrameRate=15;
			break;
		default:			
			pSensorInfo->SensorPreviewResolutionX=HI191MIPI_IMAGE_SENSOR_PV_WIDTH;
			pSensorInfo->SensorPreviewResolutionY=HI191MIPI_IMAGE_SENSOR_PV_HEIGHT;
			pSensorInfo->SensorCameraPreviewFrameRate=30;
		    break;
			
    }
	pSensorInfo->SensorFullResolutionX=HI191MIPI_IMAGE_SENSOR_FULL_WIDTH;
	pSensorInfo->SensorFullResolutionY=HI191MIPI_IMAGE_SENSOR_FULL_HEIGHT;
	pSensorInfo->SensorVideoFrameRate=30;
	pSensorInfo->SensorStillCaptureFrameRate=10;
	pSensorInfo->SensorWebCamCaptureFrameRate=15;
	pSensorInfo->SensorResetActiveHigh=TRUE; //low active
	pSensorInfo->SensorResetDelayCount=5; 

	pSensorInfo->SensorOutputDataFormat=HI191MIPI_COLOR_FORMAT;
	pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;	
	pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;

	pSensorInfo->SensorInterruptDelayLines = 4;
	pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_MIPI;
    pSensorInfo->MIPIsensorType=MIPI_OPHY_CSI2;
	
	pSensorInfo->CaptureDelayFrame = 2; 
	pSensorInfo->PreviewDelayFrame = 2; 
	pSensorInfo->VideoDelayFrame = 2; 	

	pSensorInfo->SensorMasterClockSwitch = 0; 
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_4MA;
    pSensorInfo->AEShutDelayFrame = 0;		   /* The frame of setting shutter default 0 for TG int */
	pSensorInfo->AESensorGainDelayFrame = 0;	   /* The frame of setting sensor gain */
	pSensorInfo->AEISPGainDelayFrame = 2; 
	//pSensorInfo->AEISPGainDelayFrame = 1;    
	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			pSensorInfo->SensorClockFreq=24;//26  hehe@0327 for flicker
			pSensorInfo->SensorClockDividCount= 3;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 3;
			pSensorInfo->SensorDataLatchCount= 2;
			pSensorInfo->SensorGrabStartX = HI191MIPI_FULL_X_START; 
			pSensorInfo->SensorGrabStartY = HI191MIPI_FULL_Y_START; 
			pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	        pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 0; 
	        pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
            pSensorInfo->SensorPacketECCOrder = 1;

		break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		 case MSDK_SCENARIO_ID_CAMERA_ZSD:

			pSensorInfo->SensorClockFreq=24;//26  hehe@0327 for flicker
			pSensorInfo->SensorClockDividCount= 3;
			pSensorInfo->SensorClockRisingCount=0;
			pSensorInfo->SensorClockFallingCount=2;
			pSensorInfo->SensorPixelClockCount=3;
			pSensorInfo->SensorDataLatchCount=2;
			pSensorInfo->SensorGrabStartX = HI191MIPI_FULL_X_START; 
			pSensorInfo->SensorGrabStartY = HI191MIPI_FULL_Y_START; 
			pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	        pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 0; 
	        pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
            pSensorInfo->SensorPacketECCOrder = 1;
		break;
		default:
			pSensorInfo->SensorClockFreq=24;//26 hehe@0327 for flicker
			pSensorInfo->SensorClockDividCount=3;
			pSensorInfo->SensorClockRisingCount=0;
			pSensorInfo->SensorClockFallingCount=2;		
			pSensorInfo->SensorPixelClockCount=3;
			pSensorInfo->SensorDataLatchCount=2;
			pSensorInfo->SensorGrabStartX = HI191MIPI_FULL_Y_START; 
			pSensorInfo->SensorGrabStartY = HI191MIPI_FULL_Y_START; 
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
	HI191MIPIPixelClockDivider=pSensorInfo->SensorPixelClockCount;
	memcpy(pSensorConfigData, &HI191MIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
#endif	
   SENSORDB("[HI191MIPI]exit HI191MIPIGetInfo function \n");	
 	return ERROR_NONE;
}	/* HI191MIPIGetInfo() */
UINT32 HI191MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	SENSORDB("[HI191MIPI]enter HI191MIPIControl function FeatureId:%d\n",ScenarioId);
	CurrentScenarioId=ScenarioId;		
	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			HI191MIPIPreview(pImageWindow, pSensorConfigData);
		break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:;
			HI191MIPICapture(pImageWindow, pSensorConfigData);
		break;		
        default:
            return ERROR_INVALID_SCENARIO_ID;
	}
	SENSORDB("[HI191MIPI]exit HI191MIPIControl function\n");
	return TRUE;
}	/* HI191MIPIControl() */

UINT32 HI191MIPISetVideoMode(UINT16 u2FrameRate)
{
	kal_int16 dummy_line;
    /* to fix VSYNC, to fix frame rate */
	SENSORDB("[HI191MIPI]enter HI191MIPISetVideoMode functionu 2FrameRate=%d\n",u2FrameRate);
	if(u2FrameRate==0)
		return TRUE;
	dummy_line = HI191MIPI_sensor.pclk / u2FrameRate / (HI191MIPI_PV_PERIOD_PIXEL_NUMS+HI191MIPI_DEFAULT_DUMMY_PIXELS) - (HI191MIPI_PV_PERIOD_LINE_NUMS+HI191MIPI_DEFAULT_DUMMY_LINES);
	if (dummy_line < 0) dummy_line = 0;
	SENSORDB("[ylf]dummy_line %d\n", dummy_line);
	HI191MIPI_Set_Dummy(0, dummy_line); /* modify dummy_pixel must gen AE table again */
	spin_lock(&hi191mipiraw_drv_lock);
	HI191MIPI_sensor.video_mode = KAL_TRUE;
	spin_unlock(&hi191mipiraw_drv_lock);
	SENSORDB("[HI191MIPI]exit HI191MIPISetVideoMode functionu \n");
    return TRUE;
}

UINT32 HI191MIPISetAutoFlickerMode(kal_bool bEnable,UINT16 u2FrameRate)
{
	SENSORDB("[HI191MIPI]enter HI191MIPISetAutoFlickerMode functionu bEnable=%d,u2FrameRate=%d\n",bEnable,u2FrameRate);
	if(bEnable)
	{
		spin_lock(&hi191mipiraw_drv_lock);
	   HI191MIPI_AutoFlicker_Mode=KAL_TRUE;
	   spin_unlock(&hi191mipiraw_drv_lock);
	}
	else
	{
		spin_lock(&hi191mipiraw_drv_lock);
	   HI191MIPI_AutoFlicker_Mode=KAL_FALSE;
	   spin_unlock(&hi191mipiraw_drv_lock);

	}
	SENSORDB("[HI191MIPI]exit HI191MIPISetAutoFlickerMode functionu \n");
}
UINT32 HI191MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
							 UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
	UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
	UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
	UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
	UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
	UINT32 HI191MIPISensorRegNumber;
	UINT32 i;
	PNVRAM_SENSOR_DATA_STRUCT pSensorDefaultData=(PNVRAM_SENSOR_DATA_STRUCT) pFeaturePara;
	MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
	MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;
	MSDK_SENSOR_GROUP_INFO_STRUCT *pSensorGroupInfo=(MSDK_SENSOR_GROUP_INFO_STRUCT *) pFeaturePara;
	MSDK_SENSOR_ITEM_INFO_STRUCT *pSensorItemInfo=(MSDK_SENSOR_ITEM_INFO_STRUCT *) pFeaturePara;
	MSDK_SENSOR_ENG_INFO_STRUCT	*pSensorEngInfo=(MSDK_SENSOR_ENG_INFO_STRUCT *) pFeaturePara;
	SENSORDB("[HI191MIPI]enter HI191MIPIFeatureControl functionu FeatureId:%d\n",FeatureId);
	switch (FeatureId)
	{
		case SENSOR_FEATURE_GET_PERIOD:	/* 3 */
			switch(CurrentScenarioId)
				{
					case MSDK_SCENARIO_ID_CAMERA_ZSD:
						*pFeatureReturnPara16++=HI191MIPI_sensor.line_length;
						*pFeatureReturnPara16=HI191MIPI_sensor.frame_height;
						*pFeatureParaLen=4;
					break;
					default:
						*pFeatureReturnPara16++=HI191MIPI_sensor.line_length;
						*pFeatureReturnPara16=HI191MIPI_sensor.frame_height;
						*pFeatureParaLen=4;
					break;
				}
			break;
		
		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:  /* 3 */
			switch(CurrentScenarioId)
			 {
			 	case MSDK_SCENARIO_ID_CAMERA_ZSD:
					*pFeatureReturnPara32 = HI191MIPI_sensor.pclk;
					*pFeatureParaLen=4;
				break;
				default:
					*pFeatureReturnPara32 = HI191MIPI_sensor.pclk;
					*pFeatureParaLen=4;
				break;					
			 }
		break;
			
		case SENSOR_FEATURE_SET_ESHUTTER:	/* 4 */
			set_HI191MIPI_shutter(*pFeatureData16);
		break;
		
		case SENSOR_FEATURE_SET_NIGHTMODE:
			HI191MIPI_night_mode((BOOL) *pFeatureData16);
		break;
		
		case SENSOR_FEATURE_SET_GAIN:	/* 6 */
			HI191MIPI_SetGain((UINT16) *pFeatureData16);
		break;
		
		case SENSOR_FEATURE_SET_FLASHLIGHT:
		break;
		
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
		break;	
		case SENSOR_FEATURE_SET_REGISTER:
			HI191MIPI_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
		break;		
		case SENSOR_FEATURE_GET_REGISTER:
			pSensorRegData->RegData = HI191MIPI_read_cmos_sensor(pSensorRegData->RegAddr);
		break;		
		case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
			// get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
			// if EEPROM does not exist in camera module.
			*pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_SET_VIDEO_MODE:
		       HI191MIPISetVideoMode(*pFeatureData16);
        break; 
		case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
			HI191MIPISetAutoFlickerMode((BOOL)*pFeatureData16,*(pFeatureData16+1));
			break;
        case SENSOR_FEATURE_CHECK_SENSOR_ID:
            HI191MIPIGetSensorID(pFeatureReturnPara32); 
            break; 			
		case SENSOR_FEATURE_SET_TEST_PATTERN:
             HI191MIPISetTestPatternMode((BOOL)*pFeatureData16);        	
            break;
		case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
			 *pFeatureReturnPara32 =  HI191_TEST_PATTERN_CHECKSUM;
			 *pFeatureParaLen=4;
			break;
		default:
			break;
	}
	SENSORDB("[HI191MIPI]exit HI191MIPIFeatureControl functionu FeatureId:\n");
	return ERROR_NONE;
}	/* HI191MIPIFeatureControl() */
SENSOR_FUNCTION_STRUCT    SensorFuncHI191MIPI=
{
  HI191MIPIOpen,
  HI191MIPIGetInfo,
  HI191MIPIGetResolution,
  HI191MIPIFeatureControl,
  HI191MIPIControl,
  HI191MIPIClose
};

UINT32 HI191MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{

  /* To Do : Check Sensor status here */
  if (pfFunc!=NULL)
      *pfFunc=&SensorFuncHI191MIPI;

  return ERROR_NONE;
} /* SensorInit() */



