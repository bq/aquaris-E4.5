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

#include "hi707yuv_Sensor.h"
#include "hi707yuv_Camera_Sensor_para.h"
#include "hi707yuv_CameraCustomized.h"

#define HI707YUV_DEBUG
#ifdef HI707YUV_DEBUG
#define SENSORDB printk
#else
 
#define SENSORDB(x,...)
#endif
#define HI707_TEST_PATTERN_CHECKSUM (0x786a9657)//(0x7ba87eae)

#if 0
extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);
static int sensor_id_fail = 0; 
#define HI707_write_cmos_sensor(addr, para) iWriteReg((u16) addr , (u32) para ,1,HI707_WRITE_ID)
#define HI707_write_cmos_sensor_2(addr, para, bytes) iWriteReg((u16) addr , (u32) para ,bytes,HI707_WRITE_ID)
kal_uint16 HI707_read_cmos_sensor(kal_uint32 addr)
{
kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,HI707_WRITE_ID);
    return get_byte;
}

#endif
static DEFINE_SPINLOCK(hi707_yuv_drv_lock);

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
extern int iBurstWriteReg(u8 *pData, u32 bytes, u16 i2cId);


UINT8 HI707SetBrightness_value = 0x50;
UINT8 HI707_set_param_exposure_value = 0x48;


//for burst mode

#define USE_I2C_BURST_WRITE
#ifdef USE_I2C_BURST_WRITE
#define I2C_BUFFER_LEN 254 //MAX data to send by MT6572 i2c dma mode is 255 bytes
#define BLOCK_I2C_DATA_WRITE iBurstWriteReg
#else
#define I2C_BUFFER_LEN 8   // MT6572 i2s bus master fifo length is 8 bytes
#define BLOCK_I2C_DATA_WRITE iWriteRegI2C
#endif

// {addr, data} pair in para
// len is the total length of addr+data
// Using I2C multiple/burst write if the addr increase by 1
static kal_uint16 HI707_table_write_cmos_sensor(kal_uint8* para, kal_uint32 len)
{
   char puSendCmd[I2C_BUFFER_LEN]; //at most 2 bytes address and 6 bytes data for multiple write. MTK i2c master has only 8bytes fifo.
   kal_uint32 tosend, IDX;
   kal_uint8 addr, addr_last, data;

   tosend = 0;
   IDX = 0;
   while(IDX < len)
   {
       addr = para[IDX];

       if (tosend == 0) // new (addr, data) to send
       {
           puSendCmd[tosend++] = (char)addr;
           data = para[IDX+1];
           puSendCmd[tosend++] = (char)data;
           IDX += 2;
           addr_last = addr;
       }
       else if (addr == addr_last+1) // to multiple write the data to the incremental address
       {
           data = para[IDX+1];
           puSendCmd[tosend++] = (char)data;
           IDX += 2;
           addr_last ++;
       }
       // to send out the data if the sen buffer is full or last data or to program to the address not incremental.
       if (tosend == I2C_BUFFER_LEN || IDX == len || addr != addr_last)
       {
           BLOCK_I2C_DATA_WRITE(puSendCmd , tosend, HI707_SENSOR_ID);
           tosend = 0;
       }
   }
   return 0;
}
//


kal_uint16 HI707_write_cmos_sensor(kal_uint8 addr, kal_uint8 para)
{
    char puSendCmd[2] = {(char)(addr & 0xFF) , (char)(para & 0xFF)};
    iWriteRegI2C(puSendCmd , 2,HI707_WRITE_ID);
    return 0;
}
kal_uint16 HI707_read_cmos_sensor(kal_uint8 addr)
{
    kal_uint16 get_byte=0;
    char puSendCmd = { (char)(addr & 0xFF) };
    iReadRegI2C(&puSendCmd , 1, (u8*)&get_byte,1,HI707_WRITE_ID);
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
MSDK_SENSOR_CONFIG_STRUCT HI707SensorConfigData;

static struct HI707_sensor_STRUCT HI707_sensor;
static kal_uint32 HI707_zoom_factor = 0; 
static int sensor_id_fail = 0;	
#if 0
const HI707_SENSOR_INIT_INFO HI707_Initial_Setting_Info[] =
{
  
    //PAGE 0
    //Image Size/Windowing/HSYNC/VSYNC[Type1]
    {0x03, 0x00},   //PAGEMODE(0x03)
    {0x01, 0xf1},
    {0x01, 0xf3},   //PWRCTL(0x01[P0])Bit[1]:Software Reset.
    {0x01, 0xf1},

    {0x11, 0x90},   //For No Fixed Framerate Bit[2]
    {0x12, 0x04},  //PCLK INV
        
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
    {0x43, 0x13},   //0x04 -> 0x40: For Max Framerate = 30fps
            
    //BLC
    {0x80, 0x2e},
    {0x81, 0x7e},
    {0x82, 0x90},
    {0x83, 0x30},
    {0x84, 0x2c},
    {0x85, 0x4b},
    {0x89, 0x48},
        
    {0x90, 0x0a},
    {0x91, 0x0a},    
    {0x92, 0x48},
    {0x93, 0x40},
    {0x98, 0x38},
    {0x99, 0x40},
    {0xa0, 0x00},
    {0xa8, 0x40},
    
    //PAGE 2
    //Analog Circuit
    {0x03, 0x02},      
    {0x13, 0x40},
    {0x14, 0x04},
    {0x1a, 0x00},
    {0x1b, 0x08},
        
    {0x20, 0x33},
    {0x21, 0xaa},
    {0x22, 0xa7},
    {0x23, 0xb1},       //For Sun Pot
        
    {0x3b, 0x48},
        
    {0x50, 0x21},
    {0x52, 0xa2},
    {0x53, 0x0a},
    {0x54, 0x30},
    {0x55, 0x10},
    {0x56, 0x0c},
    {0x59, 0x0F},
        
    {0x60, 0x54},
    {0x61, 0x5d},
    {0x62, 0x56},
    {0x63, 0x5c},
    {0x64, 0x56},
    {0x65, 0x5c},
    {0x72, 0x57},
    {0x73, 0x5b},
    {0x74, 0x57},
    {0x75, 0x5b},
    {0x80, 0x02},
    {0x81, 0x46},
    {0x82, 0x07},
    {0x83, 0x10},
    {0x84, 0x07},
    {0x85, 0x10},
    {0x92, 0x24},
    {0x93, 0x30},
    {0x94, 0x24},
    {0x95, 0x30},
    {0xa0, 0x03},
    {0xa1, 0x45},
    {0xa4, 0x45},
    {0xa5, 0x03},
    {0xa8, 0x12},
    {0xa9, 0x20},
    {0xaa, 0x34},
    {0xab, 0x40},
    {0xb8, 0x55},
    {0xb9, 0x59},
    {0xbc, 0x05},
    {0xbd, 0x09},
    {0xc0, 0x5f},
    {0xc1, 0x67},
    {0xc2, 0x5f},
    {0xc3, 0x67},
    {0xc4, 0x60},
    {0xc5, 0x66},
    {0xc6, 0x60},
    {0xc7, 0x66},
    {0xc8, 0x61},
    {0xc9, 0x65},
    {0xca, 0x61},
    {0xcb, 0x65},
    {0xcc, 0x62},
    {0xcd, 0x64},
    {0xce, 0x62},
    {0xcf, 0x64},
    {0xd0, 0x53},
    {0xd1, 0x68},
     
    //PAGE 10
    //Image Format, Image Effect
    {0x03, 0x10},
    {0x10, 0x03},
    {0x11, 0x43},
    {0x12, 0x30},
        
    {0x40, 0x80},
    {0x41, 0x02},
    {0x48, 0x98},
        
    {0x50, 0x48},
           
    {0x60, 0x7f},
    {0x61, 0x00},
    {0x62, 0xb0},
    {0x63, 0xa8},
    {0x64, 0x48},
    {0x66, 0x90},
    {0x67, 0x42},
    
    //PAGE 11
    //Z-LPF
    {0x03, 0x11},
    {0x10, 0x25},   
    {0x11, 0x1f},   
        
    {0x20, 0x00},   
    {0x21, 0x38},   
    {0x23, 0x0a},
        
    {0x60, 0x10},   
    {0x61, 0x82},
    {0x62, 0x00},   
    {0x63, 0x83},   
    {0x64, 0x83},      
    {0x67, 0xF0},   
    {0x68, 0x30},   
    {0x69, 0x10},   
    
    //PAGE 12
    //2D
    {0x03, 0x12},
        
    {0x40, 0xe9},
    {0x41, 0x09},
        
    {0x50, 0x18},
    {0x51, 0x24},
        
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
        
    {0x20, 0x06},
    {0x21, 0x03},
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
        
    {0x90, 0x04},
    {0x91, 0x02},
    {0x92, 0x00},
    {0x93, 0x20},
    {0x94, 0x42},
    {0x95, 0x60},
    
    //PAGE 14
    //Lens Shading Correction
    {0x03, 0x14},
    {0x10, 0x01},
        
    {0x20, 0x80},   //For Y decay
    {0x21, 0x80},   //For Y decay
    {0x22, 0x78},
    {0x23, 0x4d},
    {0x24, 0x46},
    
    //PAGE 15 
    //Color Correction
    {0x03, 0x15}, 
    {0x10, 0x03},         
    {0x14, 0x3c},
    {0x16, 0x2c},
    {0x17, 0x2f},
          
    {0x30, 0xc4},
    {0x31, 0x5b},
    {0x32, 0x1f},
    {0x33, 0x2a},
    {0x34, 0xce},
    {0x35, 0x24},
    {0x36, 0x0b},
    {0x37, 0x3f},
    {0x38, 0x8a},
           
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
        
    {0x30,  0x00},
    {0x31,  0x1c},
    {0x32,  0x2d},
    {0x33,  0x4e},
    {0x34,  0x6d},
    {0x35,  0x8b},
    {0x36,  0xa2},
    {0x37,  0xb5},
    {0x38,  0xc4},
    {0x39,  0xd0},
    {0x3a,  0xda},
    {0x3b,  0xea},
    {0x3c,  0xf4},
    {0x3d,  0xfb},
    {0x3e,  0xff},
    
    //PAGE 17 
    //Auto Flicker Cancellation 
    {0x03, 0x17},
        
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
    {0x2c, 0x2b},
           
    {0x30, 0xf8},
    {0x39, 0x22},
    {0x3a, 0xde},
    {0x3b, 0x22},
    {0x3c, 0xde},
    
    {0x60, 0x95},
    {0x68, 0x3c},
    {0x69, 0x64},
    {0x6A, 0x28},
    {0x6B, 0xc8},
    
    {0x70, 0x42},   //For Y decay   
    {0x76, 0x22},
    {0x77, 0x02},   
    {0x78, 0x12},
    {0x79, 0x27},
    {0x7a, 0x23},  
    {0x7c, 0x1d},
    {0x7d, 0x22},
    
    {0x83, 0x00},//expTime:0x83,0x84,0x85
    {0x84, 0xbe},
    {0x85, 0x6e}, 
        
    {0x86, 0x00},//expMin is minimum time of expTime,
    {0x87, 0xfa},
        
    {0x88, 0x02},
    {0x89, 0x7a},
    {0x8a, 0xc4},    
        
    {0x8b, 0x3f},
    {0x8c, 0x7a},  
        
    {0x8d, 0x34},
    {0x8e, 0xbc},
    
    {0x91, 0x02},
    {0x92, 0xdc},
    {0x93, 0x6c},   
    {0x94, 0x01},
    {0x95, 0xb7},
    {0x96, 0x74},   
    {0x98, 0x8C},
    {0x99, 0x23},  
        
    {0x9c, 0x0b},   //For Y decay: Exposure Time
    {0x9d, 0xb8},   //For Y decay: Exposure Time
    {0x9e, 0x00},
    {0x9f, 0xfa},
    
    {0xb1, 0x14},
    {0xb2, 0x50},
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
    
    {0xc0, 0x16},   //0x1a->0x16
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
        
    {0x40, 0xf0},
    {0x41, 0x33},
    {0x42, 0x33},
    {0x43, 0xf3},
    {0x44, 0x55},
    {0x45, 0x44},
    {0x46, 0x02},
           
    {0x80, 0x45},
    {0x81, 0x20},
    {0x82, 0x48},
    {0x83, 0x52},
    {0x84, 0x1b},
    {0x85, 0x50},
    {0x86, 0x25},
    {0x87, 0x4d},
    {0x88, 0x38},
    {0x89, 0x3e},
    {0x8a, 0x29},
    {0x8b, 0x02},
    {0x8d, 0x22},
    {0x8e, 0x71},  
    {0x8f, 0x63},
        
    {0x90, 0x60},
    {0x91, 0x5c},
    {0x92, 0x56},
    {0x93, 0x52},
    {0x94, 0x4c},
    {0x95, 0x36},
    {0x96, 0x31},
    {0x97, 0x2e},
    {0x98, 0x2a},
    {0x99, 0x29},
    {0x9a, 0x26},
    {0x9b, 0x09},

    //PAGE 22
    {0x03, 0x22},
    {0x10, 0xfb},

    //PAGE 20
    {0x03, 0x20},
    {0x10, 0x9c},
    
    {0x01, 0xc0},

    //PAGE 0
    {0x03, 0x00},
    {0x01, 0xc0},   //0xf1 ->0x41 : For Preview Green/Red Line.

	{0xff, 0xff}    //End of Initial Setting

};

static void HI707_Set_VGA_mode(void)
{
    HI707_write_cmos_sensor(0x01, HI707_read_cmos_sensor(0x01)|0x01);   //Sleep: For Write Reg

    HI707_write_cmos_sensor(0x03, 0x00);
    HI707_write_cmos_sensor(0x10, 0x00);        //VGA Size

    HI707_write_cmos_sensor(0x20, 0x00);
    HI707_write_cmos_sensor(0x21, 0x04);

    HI707_write_cmos_sensor(0x40, 0x01);        //HBLANK: 0x70 = 112
    HI707_write_cmos_sensor(0x41, 0x58);
    HI707_write_cmos_sensor(0x42, 0x00);        //VBLANK: 0x04 = 4
    HI707_write_cmos_sensor(0x43, 0x13);

    HI707_write_cmos_sensor(0x03, 0x11);
    HI707_write_cmos_sensor(0x10, 0x25);  

    HI707_write_cmos_sensor(0x03, 0x20);

    HI707_write_cmos_sensor(0x10, HI707_read_cmos_sensor(0x10)&0x7f);   //Close AE
    HI707_write_cmos_sensor(0x18, HI707_read_cmos_sensor(0x18)|0x08);   //Reset AE
	
    HI707_write_cmos_sensor(0x83, 0x00);
    HI707_write_cmos_sensor(0x84, 0xbe);
    HI707_write_cmos_sensor(0x85, 0x6e);
    HI707_write_cmos_sensor(0x86, 0x00);
    HI707_write_cmos_sensor(0x87, 0xfa);

    HI707_write_cmos_sensor(0x8b, 0x3f);
    HI707_write_cmos_sensor(0x8c, 0x7a);
    HI707_write_cmos_sensor(0x8d, 0x34);
    HI707_write_cmos_sensor(0x8e, 0xbc);

    HI707_write_cmos_sensor(0x9c, 0x0b);
    HI707_write_cmos_sensor(0x9d, 0xb8);
    HI707_write_cmos_sensor(0x9e, 0x00);
    HI707_write_cmos_sensor(0x9f, 0xfa);

    HI707_write_cmos_sensor(0x01, HI707_read_cmos_sensor(0x01)&0xfe);   //Exit Sleep: For Write Reg

    HI707_write_cmos_sensor(0x03, 0x20);
    HI707_write_cmos_sensor(0x10, HI707_read_cmos_sensor(0x10)|0x80);   //Open AE
    HI707_write_cmos_sensor(0x18, HI707_read_cmos_sensor(0x18)&0xf7);   //Reset AE

}
#endif
static void HI707_Initial_Setting(void)
{
	kal_uint32 iEcount;
#if 0
	for(iEcount=0;(!((0xff==(HI707_Initial_Setting_Info[iEcount].address))&&(0xff==(HI707_Initial_Setting_Info[iEcount].data))));iEcount++)
	{	
		HI707_write_cmos_sensor(HI707_Initial_Setting_Info[iEcount].address, HI707_Initial_Setting_Info[iEcount].data);
	}
	
	HI707_Set_VGA_mode();

	#endif
	
#if 1
				{
			//		static const kal_uint8 addr_data_pair[] =
					{
			
					
					////////////////////////////////////////////
					/////////////////////////////// Hi-707 Setting
					////////////////////////////////////////////
					
					/////////////////////////////////I2C_ID = 0x60
					////////////////////////////I2C_BYTE  = 0x11
			//HI707_write_cmos_sensor		
			HI707_write_cmos_sensor(0x01, 0x71);//, // reset op.
			HI707_write_cmos_sensor(0x01, 0x73);//,
			HI707_write_cmos_sensor(0x01, 0x71);//,
			
			/////////////////////////////////// PLL Setting 20120307 final	
			HI707_write_cmos_sensor(0x03, 0x00);//,
			
			HI707_write_cmos_sensor(0x08, 0x0f);//, //Parallel NO Output_PAD Out
			HI707_write_cmos_sensor(0x10, 0x00);//,
			HI707_write_cmos_sensor(0x11, 0x90);//,
			HI707_write_cmos_sensor(0x12, 0x00);//,
			HI707_write_cmos_sensor(0x14, 0x88);//,
			
			HI707_write_cmos_sensor(0x0b, 0xaa);//,
			HI707_write_cmos_sensor(0x0c, 0xaa);//,
			HI707_write_cmos_sensor(0x0d, 0xaa);//,
			
			HI707_write_cmos_sensor(0xc0, 0x95);//,
			HI707_write_cmos_sensor(0xc1, 0x18);//,
			HI707_write_cmos_sensor(0xc2, 0x91);//,
			HI707_write_cmos_sensor(0xc3, 0x00);//,
			HI707_write_cmos_sensor(0xc4, 0x01);//,
			
			HI707_write_cmos_sensor(0x03, 0x20);//, //page 20
			HI707_write_cmos_sensor(0x10, 0x1c);//, //ae off
			HI707_write_cmos_sensor(0x03, 0x22);//, //page 22
			HI707_write_cmos_sensor(0x10, 0x7b);//, //awb off
			
			HI707_write_cmos_sensor(0x03, 0x00);//,
			HI707_write_cmos_sensor(0x12, 0x00);//,
			HI707_write_cmos_sensor(0x20, 0x00);//,
			HI707_write_cmos_sensor(0x21, 0x04);//,
			HI707_write_cmos_sensor(0x22, 0x00);//,
			HI707_write_cmos_sensor(0x23, 0x04);//,
			
			HI707_write_cmos_sensor(0x40, 0x00);//, //Hblank 144
			HI707_write_cmos_sensor(0x41, 0x90);//, 
		//	HI707_write_cmos_sensor(0x42, 0x00);//, //Vblank 104
		//	HI707_write_cmos_sensor(0x43, 0x68);//,
			
			HI707_write_cmos_sensor(0x42, 0x00);//, //Vblank 4 
			HI707_write_cmos_sensor(0x43, 0x04);//,
			
			//////////////////////////////////BLC
			HI707_write_cmos_sensor(0x80, 0x2e);//, //don't touch
			HI707_write_cmos_sensor(0x81, 0x7e);//, //don't touch
			HI707_write_cmos_sensor(0x82, 0x90);//, //don't touch
			HI707_write_cmos_sensor(0x83, 0x30);//, //don't touch
			HI707_write_cmos_sensor(0x84, 0x2c);//, //don't touch
			HI707_write_cmos_sensor(0x85, 0x4b);//, //don't touch
			HI707_write_cmos_sensor(0x86, 0x01);//, //don't touch
			HI707_write_cmos_sensor(0x88, 0x47);//, //don't touch
			
			HI707_write_cmos_sensor(0x90, 0x0b);//, //BLC_TIME_TH_ON
			HI707_write_cmos_sensor(0x91, 0x0b);//, //BLC_TIME_TH_OFF 
			HI707_write_cmos_sensor(0x92, 0x48);//, //BLC_AG_TH_ON
			HI707_write_cmos_sensor(0x93, 0x48);//, //BLC_AG_TH_OFF
			
			HI707_write_cmos_sensor(0x98, 0x38);//, //don't touch
			HI707_write_cmos_sensor(0x99, 0x40);//, //Out BLC
			HI707_write_cmos_sensor(0xa0, 0x40);//, //Dark BLC
			HI707_write_cmos_sensor(0xa8, 0x44);//, //Normal BLC 44-->42
			
			/////////////////////////////////////Page 2  Last Update 10_03_12
			HI707_write_cmos_sensor(0x03, 0x02);//,
			HI707_write_cmos_sensor(0x10, 0x00);//,
			HI707_write_cmos_sensor(0x11, 0x00);//,
			HI707_write_cmos_sensor(0x13, 0x40);//,
			HI707_write_cmos_sensor(0x14, 0x04);//,
			
			HI707_write_cmos_sensor(0x18, 0x1c);//,
			HI707_write_cmos_sensor(0x19, 0x00);//, //01
			HI707_write_cmos_sensor(0x1a, 0x00);//,
			HI707_write_cmos_sensor(0x1b, 0x08);//,
			
			HI707_write_cmos_sensor(0x1c, 0x9c);//,
			HI707_write_cmos_sensor(0x1d, 0x03);//,
			
			HI707_write_cmos_sensor(0x20, 0x33);//,
			HI707_write_cmos_sensor(0x21, 0x77);//,
			HI707_write_cmos_sensor(0x22, 0xa7);//,
			HI707_write_cmos_sensor(0x23, 0x32);//,
			HI707_write_cmos_sensor(0x24, 0x33);//,
			HI707_write_cmos_sensor(0x2b, 0x40);//,
			HI707_write_cmos_sensor(0x2d, 0x32);//,
			HI707_write_cmos_sensor(0x31, 0x99);//,
			HI707_write_cmos_sensor(0x32, 0x00);//,
			HI707_write_cmos_sensor(0x33, 0x00);//,
			HI707_write_cmos_sensor(0x34, 0x3c);//,
			HI707_write_cmos_sensor(0x35, 0x0d);//,
			HI707_write_cmos_sensor(0x3b, 0x60);//, //80
			
			/////////////////////////////////////timing control 1 // //don't touch
			HI707_write_cmos_sensor(0x50, 0x21);//,
			HI707_write_cmos_sensor(0x51, 0x1c);//,
			HI707_write_cmos_sensor(0x52, 0xaa);//,
			HI707_write_cmos_sensor(0x53, 0x5a);//,
			HI707_write_cmos_sensor(0x54, 0x30);//,
			HI707_write_cmos_sensor(0x55, 0x10);//,
			HI707_write_cmos_sensor(0x56, 0x0c);//,
			HI707_write_cmos_sensor(0x58, 0x00);//,
			HI707_write_cmos_sensor(0x59, 0x0f);//,
			
			/////////////////////////////////////////timing control 2 // //don't touch
			HI707_write_cmos_sensor(0x60, 0x34);//,
			HI707_write_cmos_sensor(0x61, 0x3a);//,
			HI707_write_cmos_sensor(0x62, 0x34);//,
			HI707_write_cmos_sensor(0x63, 0x39);//,
			HI707_write_cmos_sensor(0x64, 0x34);//,
			HI707_write_cmos_sensor(0x65, 0x39);//,
			HI707_write_cmos_sensor(0x72, 0x35);//,
			HI707_write_cmos_sensor(0x73, 0x38);//,
			HI707_write_cmos_sensor(0x74, 0x35);//,
			HI707_write_cmos_sensor(0x75, 0x38);//,
			HI707_write_cmos_sensor(0x80, 0x02);//,
			HI707_write_cmos_sensor(0x81, 0x2e);//,
			HI707_write_cmos_sensor(0x82, 0x0d);//,
			HI707_write_cmos_sensor(0x83, 0x10);//,
			HI707_write_cmos_sensor(0x84, 0x0d);//,
			HI707_write_cmos_sensor(0x85, 0x10);//,
			HI707_write_cmos_sensor(0x92, 0x1d);//,
			HI707_write_cmos_sensor(0x93, 0x20);//,
			HI707_write_cmos_sensor(0x94, 0x1d);//,
			HI707_write_cmos_sensor(0x95, 0x20);//,
			HI707_write_cmos_sensor(0xa0, 0x03);//,
			HI707_write_cmos_sensor(0xa1, 0x2d);//,
			HI707_write_cmos_sensor(0xa4, 0x2d);//,
			HI707_write_cmos_sensor(0xa5, 0x03);//,
			HI707_write_cmos_sensor(0xa8, 0x12);//,
			HI707_write_cmos_sensor(0xa9, 0x1b);//,
			HI707_write_cmos_sensor(0xaa, 0x22);//,
			HI707_write_cmos_sensor(0xab, 0x2b);//,
			HI707_write_cmos_sensor(0xac, 0x10);//,
			HI707_write_cmos_sensor(0xad, 0x0e);//,
			HI707_write_cmos_sensor(0xb8, 0x33);//,
			HI707_write_cmos_sensor(0xb9, 0x35);//,
			HI707_write_cmos_sensor(0xbc, 0x0c);//,
			HI707_write_cmos_sensor(0xbd, 0x0e);//,
			HI707_write_cmos_sensor(0xc0, 0x3a);//,
			HI707_write_cmos_sensor(0xc1, 0x3f);//,
			HI707_write_cmos_sensor(0xc2, 0x3a);//,
			HI707_write_cmos_sensor(0xc3, 0x3f);//,
			HI707_write_cmos_sensor(0xc4, 0x3a);//,
			HI707_write_cmos_sensor(0xc5, 0x3e);//,
			HI707_write_cmos_sensor(0xc6, 0x3a);//,
			HI707_write_cmos_sensor(0xc7, 0x3e);//,
			HI707_write_cmos_sensor(0xc8, 0x3a);//,
			HI707_write_cmos_sensor(0xc9, 0x3e);//,
			HI707_write_cmos_sensor(0xca, 0x3a);//,
			HI707_write_cmos_sensor(0xcb, 0x3e);//,
			HI707_write_cmos_sensor(0xcc, 0x3b);//,
			HI707_write_cmos_sensor(0xcd, 0x3d);//,
			HI707_write_cmos_sensor(0xce, 0x3b);//,
			HI707_write_cmos_sensor(0xcf, 0x3d);//,
			HI707_write_cmos_sensor(0xd0, 0x33);//,
			HI707_write_cmos_sensor(0xd1, 0x3f);//,
			
			/////////////////////////////////////Page 10
			HI707_write_cmos_sensor(0x03, 0x10);//,
			HI707_write_cmos_sensor(0x10, 0x03);//, //03, //ISPCTL1, YUV ORDER(FIX)//color format
			HI707_write_cmos_sensor(0x11, 0x43);//,
			HI707_write_cmos_sensor(0x12, 0x30);//, //Y offet, dy offseet enable
			HI707_write_cmos_sensor(0x40, 0x80);//,
			HI707_write_cmos_sensor(0x41, 0x02);//, //00 DYOFS 00->10	_100318
			HI707_write_cmos_sensor(0x48, 0x85);//, //Contrast 88->84	_100318
			HI707_write_cmos_sensor(0x50, 0x48);//, //AGBRT
			
			HI707_write_cmos_sensor(0x60, 0x01);//, //7f //7c
			HI707_write_cmos_sensor(0x61, 0x00);//, //Use default
			HI707_write_cmos_sensor(0x62, 0x80);//, //SATB (1.4x)
			HI707_write_cmos_sensor(0x63, 0x80);//, //SATR (1.2x)
			HI707_write_cmos_sensor(0x64, 0x48);//, //AGSAT
			HI707_write_cmos_sensor(0x66, 0x90);//, //wht_th2
			HI707_write_cmos_sensor(0x67, 0x36);//, //wht_gain Dark (0.4x), Normal (0.75x)
			
			HI707_write_cmos_sensor(0x80, 0x00);//,
			////////////////////////////////////////Page 11
			///////////////////////////////////LPF
			HI707_write_cmos_sensor(0x03, 0x11);//,
			HI707_write_cmos_sensor(0x10, 0x25);//,
			HI707_write_cmos_sensor(0x11, 0x07);//,
			HI707_write_cmos_sensor(0x20, 0x00);//,
			HI707_write_cmos_sensor(0x21, 0x60);//,
			HI707_write_cmos_sensor(0x23, 0x0a);//,
			HI707_write_cmos_sensor(0x60, 0x13);//,
			HI707_write_cmos_sensor(0x61, 0x85);//,
			HI707_write_cmos_sensor(0x62, 0x00);//,
			HI707_write_cmos_sensor(0x63, 0x00);//,
			HI707_write_cmos_sensor(0x64, 0x00);//,
			
			HI707_write_cmos_sensor(0x67, 0x00);//,
			HI707_write_cmos_sensor(0x68, 0x24);//,
			HI707_write_cmos_sensor(0x69, 0x04);//,
			
			///////////////////////////////////////Page 12
			/////////////////////////////////2D
			HI707_write_cmos_sensor(0x03, 0x12);//,
			HI707_write_cmos_sensor(0x40, 0xd3);//,
			HI707_write_cmos_sensor(0x41, 0x09);//,
			HI707_write_cmos_sensor(0x50, 0x18);//,
			HI707_write_cmos_sensor(0x51, 0x24);//,
			HI707_write_cmos_sensor(0x70, 0x1f);//,
			HI707_write_cmos_sensor(0x71, 0x00);//,
			HI707_write_cmos_sensor(0x72, 0x00);//,
			HI707_write_cmos_sensor(0x73, 0x00);//,
			HI707_write_cmos_sensor(0x74, 0x12);//,
			HI707_write_cmos_sensor(0x75, 0x12);//,
			HI707_write_cmos_sensor(0x76, 0x20);//,
			HI707_write_cmos_sensor(0x77, 0x80);//,
			HI707_write_cmos_sensor(0x78, 0x88);//,
			HI707_write_cmos_sensor(0x79, 0x18);//,
			
			////////////////////////////////////
			HI707_write_cmos_sensor(0x90, 0x3d);//,
			HI707_write_cmos_sensor(0x91, 0x34);//,
			HI707_write_cmos_sensor(0x99, 0x28);//,
			HI707_write_cmos_sensor(0x9c, 0x05);//, //14 For defect
			HI707_write_cmos_sensor(0x9d, 0x08);//, //15 For defect
			HI707_write_cmos_sensor(0x9e, 0x28);//,
			HI707_write_cmos_sensor(0x9f, 0x28);//,
			
			HI707_write_cmos_sensor(0xb0, 0x7d);//, //75 White Defect
			HI707_write_cmos_sensor(0xb5, 0x44);//,
			HI707_write_cmos_sensor(0xb6, 0x82);//,
			HI707_write_cmos_sensor(0xb7, 0x52);//,
			HI707_write_cmos_sensor(0xb8, 0x44);//,
			HI707_write_cmos_sensor(0xb9, 0x15);//,
			////////////////////////////////////
			
			/////////////////////////////////////////////Edge
			HI707_write_cmos_sensor(0x03, 0x13);//,
			HI707_write_cmos_sensor(0x10, 0x01);//, 
			HI707_write_cmos_sensor(0x11, 0x89);//, 
			HI707_write_cmos_sensor(0x12, 0x14);//, 
			HI707_write_cmos_sensor(0x13, 0x19);//, 
			HI707_write_cmos_sensor(0x14, 0x08);//,
			HI707_write_cmos_sensor(0x20, 0x03);//,
			HI707_write_cmos_sensor(0x21, 0x04);//,
			HI707_write_cmos_sensor(0x23, 0x25);//,
			HI707_write_cmos_sensor(0x24, 0x21);//,
			HI707_write_cmos_sensor(0x25, 0x08);//,
			HI707_write_cmos_sensor(0x26, 0x40);//,
			HI707_write_cmos_sensor(0x27, 0x00);//,
			HI707_write_cmos_sensor(0x28, 0x08);//,
			HI707_write_cmos_sensor(0x29, 0x50);//,
			HI707_write_cmos_sensor(0x2a, 0xe0);//,
			HI707_write_cmos_sensor(0x2b, 0x10);//,
			HI707_write_cmos_sensor(0x2c, 0x28);//,
			HI707_write_cmos_sensor(0x2d, 0x40);//,
			HI707_write_cmos_sensor(0x2e, 0x00);//,
			HI707_write_cmos_sensor(0x2f, 0x00);//,
			HI707_write_cmos_sensor(0x30, 0x11);//,
			HI707_write_cmos_sensor(0x80, 0x05);//,
			HI707_write_cmos_sensor(0x81, 0x07);//,
			HI707_write_cmos_sensor(0x90, 0x04);//,
			HI707_write_cmos_sensor(0x91, 0x05);//,
			HI707_write_cmos_sensor(0x92, 0x00);//,
			HI707_write_cmos_sensor(0x93, 0x30);//,
			HI707_write_cmos_sensor(0x94, 0x30);//,
			HI707_write_cmos_sensor(0x95, 0x10);//,
			
			//////////////////////////////////////////////////////////////LSC_Codina
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			/////////////////////////////////////////////////////////////////////14page LSC 20120214 ??¡§¡é?¨¨//////////
			HI707_write_cmos_sensor(0x03, 0x14);//,
			HI707_write_cmos_sensor(0x10, 0x01);//,
			
			HI707_write_cmos_sensor(0x22, 0x50);//, //69,76, 34, 2b, 50
			HI707_write_cmos_sensor(0x23, 0x41);//, //50,4b, 15, 0d, 41
			HI707_write_cmos_sensor(0x24, 0x41);//, //44,3b, 10, 0b, 41
			
			HI707_write_cmos_sensor(0x27, 0x90);//,  //RXCEN  
			HI707_write_cmos_sensor(0x28, 0x78);//,  //RYCEN
			HI707_write_cmos_sensor(0x29, 0x90);//,  //GXCEN
			HI707_write_cmos_sensor(0x2a, 0x78);//,  //GYCEN
			HI707_write_cmos_sensor(0x2b, 0x90);//,  //BXCEN
			HI707_write_cmos_sensor(0x2c, 0x78);//,   //BYCEN
			
			
			//////////////////////////15page/////////////////////////
			HI707_write_cmos_sensor(0x03, 0x15);//, 
			HI707_write_cmos_sensor(0x10, 0x03);//,
			
			HI707_write_cmos_sensor(0x14, 0x52);//, //CMCOFSGM 
			HI707_write_cmos_sensor(0x16, 0x3a);//, //CMCOFSGL
			HI707_write_cmos_sensor(0x17, 0x2f);//, //CMC SIGN
			
			////////////////////////////////////////CMC
			HI707_write_cmos_sensor(0x30, 0xf1);//,
			HI707_write_cmos_sensor(0x31, 0x71);//,
			HI707_write_cmos_sensor(0x32, 0x00);//,
			HI707_write_cmos_sensor(0x33, 0x1f);//,
			HI707_write_cmos_sensor(0x34, 0xe1);//,
			HI707_write_cmos_sensor(0x35, 0x42);//,
			HI707_write_cmos_sensor(0x36, 0x01);//,
			HI707_write_cmos_sensor(0x37, 0x31);//,
			HI707_write_cmos_sensor(0x38, 0x72);//,
			///////////////////////////////////////////////CMC OFS
			HI707_write_cmos_sensor(0x40, 0x90);//,
			HI707_write_cmos_sensor(0x41, 0x82);//,
			HI707_write_cmos_sensor(0x42, 0x12);//,
			HI707_write_cmos_sensor(0x43, 0x86);//,
			HI707_write_cmos_sensor(0x44, 0x92);//,
			HI707_write_cmos_sensor(0x45, 0x18);//,
			HI707_write_cmos_sensor(0x46, 0x84);//,
			HI707_write_cmos_sensor(0x47, 0x02);//,
			HI707_write_cmos_sensor(0x48, 0x02);//,
			
			
			HI707_write_cmos_sensor(0x03, 0x16);//,
			HI707_write_cmos_sensor(0x10, 0x01);//,
			HI707_write_cmos_sensor(0x30, 0x00);//,
			HI707_write_cmos_sensor(0x31, 0x0f);//,
			HI707_write_cmos_sensor(0x32, 0x20);//,
			HI707_write_cmos_sensor(0x33, 0x35);//,
			HI707_write_cmos_sensor(0x34, 0x58);//,
			HI707_write_cmos_sensor(0x35, 0x75);//,
			HI707_write_cmos_sensor(0x36, 0x8e);//,
			HI707_write_cmos_sensor(0x37, 0xa3);//,
			HI707_write_cmos_sensor(0x38, 0xb4);//,
			HI707_write_cmos_sensor(0x39, 0xc3);//,
			HI707_write_cmos_sensor(0x3a, 0xcf);//,
			HI707_write_cmos_sensor(0x3b, 0xe2);//,
			HI707_write_cmos_sensor(0x3c, 0xf0);//,
			HI707_write_cmos_sensor(0x3d, 0xf9);//,
			HI707_write_cmos_sensor(0x3e, 0xff);//,
			
			
			///////////////////////////////////////////////////Page 17 A 
			HI707_write_cmos_sensor(0x03, 0x17);//,
			HI707_write_cmos_sensor(0xc4, 0x3c);//,
			HI707_write_cmos_sensor(0xc5, 0x32);//,
			
			//////////////////////////////////////////////////////Page 20 A 
			HI707_write_cmos_sensor(0x03, 0x20);//,
			HI707_write_cmos_sensor(0x10, 0x1c);//,
			HI707_write_cmos_sensor(0x11, 0x04);//,
			
			HI707_write_cmos_sensor(0x20, 0x01);//,
			HI707_write_cmos_sensor(0x28, 0x27);//,
			HI707_write_cmos_sensor(0x29, 0xa1);//,
			
			HI707_write_cmos_sensor(0x2a, 0xf0);//,
			HI707_write_cmos_sensor(0x2b, 0xf4);//,
			HI707_write_cmos_sensor(0x2c, 0x2b);//, 
			
			HI707_write_cmos_sensor(0x30, 0xf8);//,
			
			HI707_write_cmos_sensor(0x3b, 0x22);//,
			HI707_write_cmos_sensor(0x3c, 0xde);//,
			
			HI707_write_cmos_sensor(0x39, 0x22);//,
			HI707_write_cmos_sensor(0x3a, 0xde);//,
			HI707_write_cmos_sensor(0x3b, 0x22);//, //23->22 _10_04_06 hhzin
			HI707_write_cmos_sensor(0x3c, 0xde);//,
			
			HI707_write_cmos_sensor(0x60, 0x71);//, //70
			HI707_write_cmos_sensor(0x61, 0x00);//, //11 //22
			
			HI707_write_cmos_sensor(0x62, 0x71);//,
			HI707_write_cmos_sensor(0x63, 0x00);//, //11 //22
			
			HI707_write_cmos_sensor(0x68, 0x30);//,// x no Flip 34
			HI707_write_cmos_sensor(0x69, 0x6a);//,// x no Flip 66
			HI707_write_cmos_sensor(0x6A, 0x27);//,
			HI707_write_cmos_sensor(0x6B, 0xbb);//,
			
			HI707_write_cmos_sensor(0x70, 0x34);//,//Y Targe 32
			
			HI707_write_cmos_sensor(0x76, 0x11);//, //Unlock bnd1
			HI707_write_cmos_sensor(0x77, 0x72);//, //Unlock bnd2 02->a2 _10_04_06 hhzin
			
			HI707_write_cmos_sensor(0x78, 0x12);//, //Yth 1
			HI707_write_cmos_sensor(0x79, 0x26);//, //Yth 2 26->27 _10_04_06 hhzin
			HI707_write_cmos_sensor(0x7a, 0x23);//, //Yth 3
	
			HI707_write_cmos_sensor(0x7c, 0x17);//, //1c->1d _10_04_06 hhzin
			HI707_write_cmos_sensor(0x7d, 0x22);//,
	
			//		
		#if 0
			HI707_write_cmos_sensor(0x83, 0x00);//, //EXP Normal 33.33 fps 
			HI707_write_cmos_sensor(0x84, 0xaf);//, 
			HI707_write_cmos_sensor(0x85, 0xc8);//, 
			
			HI707_write_cmos_sensor(0x86, 0x00);//, //EXPMin 7500.00 fps
			HI707_write_cmos_sensor(0x87, 0xc8);//, 
			
			HI707_write_cmos_sensor(0xa0, 0x02);//, //EXP Max 8.33 fps 
			HI707_write_cmos_sensor(0xa1, 0xbf);//, 
			HI707_write_cmos_sensor(0xa2, 0x20);//,
			
			HI707_write_cmos_sensor(0x8B, 0x3a);//, //EXP100 
			HI707_write_cmos_sensor(0x8C, 0x98);//, 
			HI707_write_cmos_sensor(0x8D, 0x30);//, //EXP120 
			HI707_write_cmos_sensor(0x8E, 0xd4);//, 
			
			HI707_write_cmos_sensor(0x98, 0x8C);//,
			HI707_write_cmos_sensor(0x99, 0x23);//,
			
			HI707_write_cmos_sensor(0x9c, 0x05);//, //EXP Limit 1071.43 fps 
			HI707_write_cmos_sensor(0x9d, 0x78);//,
			HI707_write_cmos_sensor(0x9e, 0x00);//, //4shared Unit //200
			HI707_write_cmos_sensor(0x9f, 0xc8);//,
		#endif
			//
		#if 1
			HI707_write_cmos_sensor(0x83, 0x00);// //EXP Normal 33.33 fps //0x00afc8
			HI707_write_cmos_sensor(0x84, 0xbd);// 
			HI707_write_cmos_sensor(0x85, 0xd8);// 
	
			HI707_write_cmos_sensor(0x86, 0x00);// //EXPMin 7500.00 fps
			HI707_write_cmos_sensor(0x87, 0xc8);// 
	
			HI707_write_cmos_sensor(0x88, 0x02);// //EXP Max(120Hz) 10.00 fps //0x0249f0
			HI707_write_cmos_sensor(0x89, 0x78);// 
			HI707_write_cmos_sensor(0x8a, 0xd0);// 
	
			HI707_write_cmos_sensor(0xa0, 0x02);// //EXP Max(100Hz) 10.00 fps 
			HI707_write_cmos_sensor(0xa1, 0x78);// 
			HI707_write_cmos_sensor(0xa2, 0xd0);// 
	
			HI707_write_cmos_sensor(0x8B, 0x3f);// //EXP100 
			HI707_write_cmos_sensor(0x8C, 0x48);// 
			HI707_write_cmos_sensor(0x8D, 0x34);// //EXP120 
			HI707_write_cmos_sensor(0x8E, 0xbc);// 
	
			HI707_write_cmos_sensor(0x98, 0x8C);//
			HI707_write_cmos_sensor(0x99, 0x23);//
	
			HI707_write_cmos_sensor(0x9c, 0x02);// //EXP Limit 2142.86 fps 
			HI707_write_cmos_sensor(0x9d, 0xbc);// 
			HI707_write_cmos_sensor(0x9e, 0x00);// //EXP Unit 
			HI707_write_cmos_sensor(0x9f, 0xc8);// 
		#endif
			
			HI707_write_cmos_sensor(0xb0, 0x1d);//,
			HI707_write_cmos_sensor(0xb1, 0x14);//, //14
			HI707_write_cmos_sensor(0xb2, 0xa0);//, //80
			HI707_write_cmos_sensor(0xb3, 0x17);//, //AGLVL //17
			HI707_write_cmos_sensor(0xb4, 0x17);//,
			HI707_write_cmos_sensor(0xb5, 0x3e);//,
			HI707_write_cmos_sensor(0xb6, 0x2b);//,
			HI707_write_cmos_sensor(0xb7, 0x24);//,
			HI707_write_cmos_sensor(0xb8, 0x21);//,
			HI707_write_cmos_sensor(0xb9, 0x1f);//,
			HI707_write_cmos_sensor(0xba, 0x1e);//,
			HI707_write_cmos_sensor(0xbb, 0x1d);//,
			HI707_write_cmos_sensor(0xbc, 0x1c);//,
			HI707_write_cmos_sensor(0xbd, 0x1b);//,
			
			HI707_write_cmos_sensor(0xc0, 0x1a);//,
			HI707_write_cmos_sensor(0xc3, 0x48);//,
			HI707_write_cmos_sensor(0xc4, 0x48);//,
			
			/////////////////////////////////////////////////////////////////Page 22 AWB
			HI707_write_cmos_sensor(0x03, 0x22);//,
			HI707_write_cmos_sensor(0x10, 0xe2);//,
			HI707_write_cmos_sensor(0x11, 0x2e);//, //2e
			HI707_write_cmos_sensor(0x20, 0x41);//, //01 //69
			HI707_write_cmos_sensor(0x21, 0x40);//,
			HI707_write_cmos_sensor(0x24, 0xfe);//,
			
			HI707_write_cmos_sensor(0x30, 0x80);//, //Cb
			HI707_write_cmos_sensor(0x31, 0x80);//, //Cr
			HI707_write_cmos_sensor(0x38, 0x12);//, //Lock Boundary //13
			HI707_write_cmos_sensor(0x39, 0x33);//,
			HI707_write_cmos_sensor(0x40, 0xf3);//,
			HI707_write_cmos_sensor(0x41, 0x54);//,
			HI707_write_cmos_sensor(0x42, 0x33);//,
			HI707_write_cmos_sensor(0x43, 0xf3);//,
			HI707_write_cmos_sensor(0x44, 0x44);//,
			HI707_write_cmos_sensor(0x45, 0x66);//,
			HI707_write_cmos_sensor(0x46, 0x08);//,
			
			HI707_write_cmos_sensor(0x80, 0x3d);//,
			HI707_write_cmos_sensor(0x81, 0x20);//,
			HI707_write_cmos_sensor(0x82, 0x40);//,
			
			HI707_write_cmos_sensor(0x83, 0x5a);//, //RMAX 5a
			HI707_write_cmos_sensor(0x84, 0x20);//, //RMIN 23
			HI707_write_cmos_sensor(0x85, 0x53);//, //BMAX 5a
			HI707_write_cmos_sensor(0x86, 0x24);//, //BMIN 
			
			HI707_write_cmos_sensor(0x87, 0x4a);//, //42
			HI707_write_cmos_sensor(0x88, 0x3c);//,
			HI707_write_cmos_sensor(0x89, 0x3e);//,
			HI707_write_cmos_sensor(0x8a, 0x34);//,
			
			HI707_write_cmos_sensor(0x8b, 0x00);//, //00
			HI707_write_cmos_sensor(0x8d, 0x24);//,
			HI707_write_cmos_sensor(0x8e, 0x61);//,
			
			HI707_write_cmos_sensor(0x8f, 0x63);//, //
			HI707_write_cmos_sensor(0x90, 0x62);//, //
			HI707_write_cmos_sensor(0x91, 0x5e);//, //
			HI707_write_cmos_sensor(0x92, 0x5a);//, //56
			HI707_write_cmos_sensor(0x93, 0x50);//, //4c
			HI707_write_cmos_sensor(0x94, 0x42);//, //3e
			HI707_write_cmos_sensor(0x95, 0x3b);//, //37
			HI707_write_cmos_sensor(0x96, 0x34);//, //30
			HI707_write_cmos_sensor(0x97, 0x2d);//, //2c
			HI707_write_cmos_sensor(0x98, 0x2b);//, //2a
			HI707_write_cmos_sensor(0x99, 0x29);//, //28
			HI707_write_cmos_sensor(0x9a, 0x27);//, //26
			HI707_write_cmos_sensor(0x9b, 0x0b);//, //
			
			////////////////////////////////////////// Page 48 - MIPI////
			HI707_write_cmos_sensor(0x03, 0x48);//,
			
			HI707_write_cmos_sensor(0x10, 0x05);//,
			HI707_write_cmos_sensor(0x11, 0x00);//, //async_fifo off
			HI707_write_cmos_sensor(0x12, 0x00);//,
			
			HI707_write_cmos_sensor(0x16, 0xc4);//,
			HI707_write_cmos_sensor(0x17, 0x00);//,
			HI707_write_cmos_sensor(0x19, 0x00);//,
			HI707_write_cmos_sensor(0x1a, 0x00);//, //06 
			HI707_write_cmos_sensor(0x1c, 0x02);//,
			HI707_write_cmos_sensor(0x1d, 0x04);//,
			HI707_write_cmos_sensor(0x1e, 0x07);//,
			HI707_write_cmos_sensor(0x1f, 0x06);//, //04 
			HI707_write_cmos_sensor(0x20, 0x00);//,
			HI707_write_cmos_sensor(0x21, 0xb8);//,
			HI707_write_cmos_sensor(0x22, 0x00);//,
			HI707_write_cmos_sensor(0x23, 0x01);//,
			
			HI707_write_cmos_sensor(0x30, 0x05);//,
			HI707_write_cmos_sensor(0x31, 0x00);//,
			HI707_write_cmos_sensor(0x34, 0x02);//, //01 
			HI707_write_cmos_sensor(0x32, 0x06);//,
			HI707_write_cmos_sensor(0x35, 0x03);//, //02 
			HI707_write_cmos_sensor(0x36, 0x01);//,
			HI707_write_cmos_sensor(0x37, 0x03);//,
			HI707_write_cmos_sensor(0x38, 0x00);//,
			HI707_write_cmos_sensor(0x39, 0x4a);//, 
			HI707_write_cmos_sensor(0x3c, 0x00);//,
			HI707_write_cmos_sensor(0x3d, 0xfa);//,
			HI707_write_cmos_sensor(0x3f, 0x10);//,
			HI707_write_cmos_sensor(0x40, 0x00);//,
			HI707_write_cmos_sensor(0x41, 0x20);//,
			HI707_write_cmos_sensor(0x42, 0x00);//,
			////////////////////////////////////////////////////////
			
			///////////////////////////////////////////////  {0x50, 0x81},
			
			HI707_write_cmos_sensor(0x03, 0x22);//,
			HI707_write_cmos_sensor(0x10, 0xfb);//,
			
			HI707_write_cmos_sensor(0x03, 0x20);//,
			HI707_write_cmos_sensor(0x10, 0x9c);//,
			
			HI707_write_cmos_sensor(0x01, 0x70);//,
			
					///////////////////////////I2C_ID = 0x60
					/////////////////////////////I2C_BYTE  = 0x11
					
					/////////////////////////////END
					//////////////////////////////[END]
			
				
					};
			//		HI707_table_write_cmos_sensor(addr_data_pair, sizeof(addr_data_pair)/sizeof(kal_uint8));
				}
			
#endif

#if 0
			
#if 1
			{
				static const kal_uint8 addr_data_pair[] =
				{
		
				
				////////////////////////////////////////////
				/////////////////////////////// Hi-707 Setting
				////////////////////////////////////////////
				
				/////////////////////////////////I2C_ID = 0x60
				////////////////////////////I2C_BYTE  = 0x11
		//HI707_write_cmos_sensor		
		0x01, 0x71,////, // reset op.
		0x01, 0x73,////,
		0x01, 0x71,////,
		
		/////////////////////////////////// PLL Setting 20120307 final	
		0x03, 0x00,////,
		
		0x08, 0x0f,////, //Parallel NO Output_PAD Out
		0x10, 0x00,////,
		0x11, 0x90,////,
		0x12, 0x00,////,
		0x14, 0x88,////,
		
		0x0b, 0xaa,////,
		0x0c, 0xaa,////,
		0x0d, 0xaa,////,
		
		0xc0, 0x95,////,
		0xc1, 0x18,////,
		0xc2, 0x91,////,
		0xc3, 0x00,////,
		0xc4, 0x01,////,
		
		0x03, 0x20,////, //page 20
		0x10, 0x1c,////, //ae off
		0x03, 0x22,////, //page 22
		0x10, 0x7b,////, //awb off
		
		0x03, 0x00,////,
		0x12, 0x00,////,
		0x20, 0x00,////,
		0x21, 0x04,////,
		0x22, 0x00,////,
		0x23, 0x04,////,
		
		0x40, 0x00,////, //Hblank 144
		0x41, 0x90,////, 
	//	0x42, 0x00,////, //Vblank 104
	//	0x43, 0x68,////,
		
		0x42, 0x00,////, //Vblank 4 
		0x43, 0x04,////,
		
		//////////////////////////////////BLC
		0x80, 0x2e,////, //don't touch
		0x81, 0x7e,////, //don't touch
		0x82, 0x90,////, //don't touch
		0x83, 0x30,////, //don't touch
		0x84, 0x2c,////, //don't touch
		0x85, 0x4b,////, //don't touch
		0x86, 0x01,////, //don't touch
		0x88, 0x47,////, //don't touch
		
		0x90, 0x0b,////, //BLC_TIME_TH_ON
		0x91, 0x0b,////, //BLC_TIME_TH_OFF 
		0x92, 0x48,////, //BLC_AG_TH_ON
		0x93, 0x48,////, //BLC_AG_TH_OFF
		
		0x98, 0x38,////, //don't touch
		0x99, 0x40,////, //Out BLC
		0xa0, 0x40,////, //Dark BLC
		0xa8, 0x44,////, //Normal BLC 44-->42
		
		/////////////////////////////////////Page 2  Last Update 10_03_12
		0x03, 0x02,////,
		0x10, 0x00,////,
		0x11, 0x00,////,
		0x13, 0x40,////,
		0x14, 0x04,////,
		
		0x18, 0x1c,////,
		0x19, 0x00,////, //01
		0x1a, 0x00,////,
		0x1b, 0x08,////,
		
		0x1c, 0x9c,////,
		0x1d, 0x03,////,
		
		0x20, 0x33,////,
		0x21, 0x77,////,
		0x22, 0xa7,////,
		0x23, 0x32,////,
		0x24, 0x33,////,
		0x2b, 0x40,////,
		0x2d, 0x32,////,
		0x31, 0x99,////,
		0x32, 0x00,////,
		0x33, 0x00,////,
		0x34, 0x3c,////,
		0x35, 0x0d,////,
		0x3b, 0x60,////, //80
		
		/////////////////////////////////////timing control 1 // //don't touch
		0x50, 0x21,////,
		0x51, 0x1c,////,
		0x52, 0xaa,////,
		0x53, 0x5a,////,
		0x54, 0x30,////,
		0x55, 0x10,////,
		0x56, 0x0c,////,
		0x58, 0x00,////,
		0x59, 0x0f,////,
		
		/////////////////////////////////////////timing control 2 // //don't touch
		0x60, 0x34,////,
		0x61, 0x3a,////,
		0x62, 0x34,////,
		0x63, 0x39,////,
		0x64, 0x34,////,
		0x65, 0x39,////,
		0x72, 0x35,////,
		0x73, 0x38,////,
		0x74, 0x35,////,
		0x75, 0x38,////,
		0x80, 0x02,////,
		0x81, 0x2e,////,
		0x82, 0x0d,////,
		0x83, 0x10,////,
		0x84, 0x0d,////,
		0x85, 0x10,////,
		0x92, 0x1d,////,
		0x93, 0x20,////,
		0x94, 0x1d,////,
		0x95, 0x20,////,
		0xa0, 0x03,////,
		0xa1, 0x2d,////,
		0xa4, 0x2d,////,
		0xa5, 0x03,////,
		0xa8, 0x12,////,
		0xa9, 0x1b,////,
		0xaa, 0x22,////,
		0xab, 0x2b,////,
		0xac, 0x10,////,
		0xad, 0x0e,////,
		0xb8, 0x33,////,
		0xb9, 0x35,////,
		0xbc, 0x0c,////,
		0xbd, 0x0e,////,
		0xc0, 0x3a,////,
		0xc1, 0x3f,////,
		0xc2, 0x3a,////,
		0xc3, 0x3f,////,
		0xc4, 0x3a,////,
		0xc5, 0x3e,////,
		0xc6, 0x3a,////,
		0xc7, 0x3e,////,
		0xc8, 0x3a,////,
		0xc9, 0x3e,////,
		0xca, 0x3a,////,
		0xcb, 0x3e,////,
		0xcc, 0x3b,////,
		0xcd, 0x3d,////,
		0xce, 0x3b,////,
		0xcf, 0x3d,////,
		0xd0, 0x33,////,
		0xd1, 0x3f,////,
		
		/////////////////////////////////////Page 10
		0x03, 0x10,////,
		0x10, 0x03,////, //03, //ISPCTL1, YUV ORDER(FIX)//color format
		0x11, 0x43,////,
		0x12, 0x30,////, //Y offet, dy offseet enable
		0x40, 0x80,////,
		0x41, 0x02,////, //00 DYOFS 00->10	_100318
		0x48, 0x85,////, //Contrast 88->84	_100318
		0x50, 0x48,////, //AGBRT
		
		0x60, 0x01,////, //7f //7c
		0x61, 0x00,////, //Use default
		0x62, 0x80,////, //SATB (1.4x)
		0x63, 0x80,////, //SATR (1.2x)
		0x64, 0x48,////, //AGSAT
		0x66, 0x90,////, //wht_th2
		0x67, 0x36,////, //wht_gain Dark (0.4x), Normal (0.75x)
		
		0x80, 0x00,////,
		////////////////////////////////////////Page 11
		///////////////////////////////////LPF
		0x03, 0x11,////,
		0x10, 0x25,////,
		0x11, 0x07,////,
		0x20, 0x00,////,
		0x21, 0x60,////,
		0x23, 0x0a,////,
		0x60, 0x13,////,
		0x61, 0x85,////,
		0x62, 0x00,////,
		0x63, 0x00,////,
		0x64, 0x00,////,
		
		0x67, 0x00,////,
		0x68, 0x24,////,
		0x69, 0x04,////,
		
		///////////////////////////////////////Page 12
		/////////////////////////////////2D
		0x03, 0x12,////,
		0x40, 0xd3,////,
		0x41, 0x09,////,
		0x50, 0x18,////,
		0x51, 0x24,////,
		0x70, 0x1f,////,
		0x71, 0x00,////,
		0x72, 0x00,////,
		0x73, 0x00,////,
		0x74, 0x12,////,
		0x75, 0x12,////,
		0x76, 0x20,////,
		0x77, 0x80,////,
		0x78, 0x88,////,
		0x79, 0x18,////,
		
		////////////////////////////////////
		0x90, 0x3d,////,
		0x91, 0x34,////,
		0x99, 0x28,////,
		0x9c, 0x05,////, //14 For defect
		0x9d, 0x08,////, //15 For defect
		0x9e, 0x28,////,
		0x9f, 0x28,////,
		
		0xb0, 0x7d,////, //75 White Defect
		0xb5, 0x44,////,
		0xb6, 0x82,////,
		0xb7, 0x52,////,
		0xb8, 0x44,////,
		0xb9, 0x15,////,
		////////////////////////////////////
		
		/////////////////////////////////////////////Edge
		0x03, 0x13,////,
		0x10, 0x01,////, 
		0x11, 0x89,////, 
		0x12, 0x14,////, 
		0x13, 0x19,////, 
		0x14, 0x08,////,
		0x20, 0x03,////,
		0x21, 0x04,////,
		0x23, 0x25,////,
		0x24, 0x21,////,
		0x25, 0x08,////,
		0x26, 0x40,////,
		0x27, 0x00,////,
		0x28, 0x08,////,
		0x29, 0x50,////,
		0x2a, 0xe0,////,
		0x2b, 0x10,////,
		0x2c, 0x28,////,
		0x2d, 0x40,////,
		0x2e, 0x00,////,
		0x2f, 0x00,////,
		0x30, 0x11,////,
		0x80, 0x05,////,
		0x81, 0x07,////,
		0x90, 0x04,////,
		0x91, 0x05,////,
		0x92, 0x00,////,
		0x93, 0x30,////,
		0x94, 0x30,////,
		0x95, 0x10,////,
		
		//////////////////////////////////////////////////////////////LSC_Codina
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		/////////////////////////////////////////////////////////////////////14page LSC 20120214 ???¡ì?¨¦?¡§¡§//////////
		0x03, 0x14,////,
		0x10, 0x01,////,
		
		0x22, 0x50,////, //69,76, 34, 2b, 50
		0x23, 0x41,////, //50,4b, 15, 0d, 41
		0x24, 0x41,////, //44,3b, 10, 0b, 41
		
		0x27, 0x90,////,  //RXCEN  
		0x28, 0x78,////,  //RYCEN
		0x29, 0x90,////,  //GXCEN
		0x2a, 0x78,////,  //GYCEN
		0x2b, 0x90,////,  //BXCEN
		0x2c, 0x78,////,   //BYCEN
		
		
		//////////////////////////15page/////////////////////////
		0x03, 0x15,////, 
		0x10, 0x03,////,
		
		0x14, 0x52,////, //CMCOFSGM 
		0x16, 0x3a,////, //CMCOFSGL
		0x17, 0x2f,////, //CMC SIGN
		
		////////////////////////////////////////CMC
		0x30, 0xf1,////,
		0x31, 0x71,////,
		0x32, 0x00,////,
		0x33, 0x1f,////,
		0x34, 0xe1,////,
		0x35, 0x42,////,
		0x36, 0x01,////,
		0x37, 0x31,////,
		0x38, 0x72,////,
		///////////////////////////////////////////////CMC OFS
		0x40, 0x90,////,
		0x41, 0x82,////,
		0x42, 0x12,////,
		0x43, 0x86,////,
		0x44, 0x92,////,
		0x45, 0x18,////,
		0x46, 0x84,////,
		0x47, 0x02,////,
		0x48, 0x02,////,
		
		
		0x03, 0x16,////,
		0x10, 0x01,////,
		0x30, 0x00,////,
		0x31, 0x0f,////,
		0x32, 0x20,////,
		0x33, 0x35,////,
		0x34, 0x58,////,
		0x35, 0x75,////,
		0x36, 0x8e,////,
		0x37, 0xa3,////,
		0x38, 0xb4,////,
		0x39, 0xc3,////,
		0x3a, 0xcf,////,
		0x3b, 0xe2,////,
		0x3c, 0xf0,////,
		0x3d, 0xf9,////,
		0x3e, 0xff,////,
		
		
		///////////////////////////////////////////////////Page 17 A 
		0x03, 0x17,////,
		0xc4, 0x3c,////,
		0xc5, 0x32,////,
		
		//////////////////////////////////////////////////////Page 20 A 
		0x03, 0x20,////,
		0x10, 0x1c,////,
		0x11, 0x04,////,
		
		0x20, 0x01,////,
		0x28, 0x27,////,
		0x29, 0xa1,////,
		
		0x2a, 0xf0,////,
		0x2b, 0xf4,////,
		0x2c, 0x2b,////, 
		
		0x30, 0xf8,////,
		
		0x3b, 0x22,////,
		0x3c, 0xde,////,
		
		0x39, 0x22,////,
		0x3a, 0xde,////,
		0x3b, 0x22,////, //23->22 _10_04_06 hhzin
		0x3c, 0xde,////,
		
		0x60, 0x71,////, //70
		0x61, 0x00,////, //11 //22
		
		0x62, 0x71,////,
		0x63, 0x00,////, //11 //22
		
		0x68, 0x30,////,// x no Flip 34
		0x69, 0x6a,////,// x no Flip 66
		0x6A, 0x27,////,
		0x6B, 0xbb,////,
		
		0x70, 0x34,////,//Y Targe 32
		
		0x76, 0x11,////, //Unlock bnd1
		0x77, 0x72,////, //Unlock bnd2 02->a2 _10_04_06 hhzin
		
		0x78, 0x12,////, //Yth 1
		0x79, 0x26,////, //Yth 2 26->27 _10_04_06 hhzin
		0x7a, 0x23,////, //Yth 3

		0x7c, 0x17,////, //1c->1d _10_04_06 hhzin
		0x7d, 0x22,////,

		//		
		#if 0
		0x83, 0x00,////, //EXP Normal 33.33 fps 
		0x84, 0xaf,////, 
		0x85, 0xc8,////, 
		
		0x86, 0x00,////, //EXPMin 7500.00 fps
		0x87, 0xc8,////, 
		
		0xa0, 0x02,////, //EXP Max 8.33 fps 
		0xa1, 0xbf,////, 
		0xa2, 0x20,////,
		
		0x8B, 0x3a,////, //EXP100 
		0x8C, 0x98,////, 
		0x8D, 0x30,////, //EXP120 
		0x8E, 0xd4,////, 
		
		0x98, 0x8C,////,
		0x99, 0x23,////,
		
		0x9c, 0x05,////, //EXP Limit 1071.43 fps 
		0x9d, 0x78,////,
		0x9e, 0x00,////, //4shared Unit //200
		0x9f, 0xc8,////,
		#endif
		//
		#if 1
		0x83, 0x00,//// //EXP Normal 33.33 fps 
		0x84, 0xaf,//// 
		0x85, 0xc8,//// 

		0x86, 0x00,//// //EXPMin 7500.00 fps
		0x87, 0xc8,//// 

		0x88, 0x02,//// //EXP Max(120Hz) 10.00 fps 
		0x89, 0x49,//// 
		0x8a, 0xf0,//// 

		0xa0, 0x02,//// //EXP Max(100Hz) 10.00 fps 
		0xa1, 0x49,//// 
		0xa2, 0xf0,//// 

		0x8B, 0x3a,//// //EXP100 
		0x8C, 0x98,//// 
		0x8D, 0x30,//// //EXP120 
		0x8E, 0xd4,//// 

		0x98, 0x8C,////
		0x99, 0x23,////

		0x9c, 0x02,//// //EXP Limit 2142.86 fps 
		0x9d, 0xbc,//// 
		0x9e, 0x00,//// //EXP Unit 
		0x9f, 0xc8,//// 
		#endif
		
		0xb0, 0x1d,////,
		0xb1, 0x14,////, //14
		0xb2, 0xa0,////, //80
		0xb3, 0x17,////, //AGLVL //17
		0xb4, 0x17,////,
		0xb5, 0x3e,////,
		0xb6, 0x2b,////,
		0xb7, 0x24,////,
		0xb8, 0x21,////,
		0xb9, 0x1f,////,
		0xba, 0x1e,////,
		0xbb, 0x1d,////,
		0xbc, 0x1c,////,
		0xbd, 0x1b,////,
		
		0xc0, 0x1a,////,
		0xc3, 0x48,////,
		0xc4, 0x48,////,
		
		/////////////////////////////////////////////////////////////////Page 22 AWB
		0x03, 0x22,////,
		0x10, 0xe2,////,
		0x11, 0x2e,////, //2e
		0x20, 0x41,////, //01 //69
		0x21, 0x40,////,
		0x24, 0xfe,////,
		
		0x30, 0x80,////, //Cb
		0x31, 0x80,////, //Cr
		0x38, 0x12,////, //Lock Boundary //13
		0x39, 0x33,////,
		0x40, 0xf3,////,
		0x41, 0x54,////,
		0x42, 0x33,////,
		0x43, 0xf3,////,
		0x44, 0x44,////,
		0x45, 0x66,////,
		0x46, 0x08,////,
		
		0x80, 0x3d,////,
		0x81, 0x20,////,
		0x82, 0x40,////,
		
		0x83, 0x5a,////, //RMAX 5a
		0x84, 0x20,////, //RMIN 23
		0x85, 0x53,////, //BMAX 5a
		0x86, 0x24,////, //BMIN 
		
		0x87, 0x4a,////, //42
		0x88, 0x3c,////,
		0x89, 0x3e,////,
		0x8a, 0x34,////,
		
		0x8b, 0x00,////, //00
		0x8d, 0x24,////,
		0x8e, 0x61,////,
		
		0x8f, 0x63,////, //
		0x90, 0x62,////, //
		0x91, 0x5e,////, //
		0x92, 0x5a,////, //56
		0x93, 0x50,////, //4c
		0x94, 0x42,////, //3e
		0x95, 0x3b,////, //37
		0x96, 0x34,////, //30
		0x97, 0x2d,////, //2c
		0x98, 0x2b,////, //2a
		0x99, 0x29,////, //28
		0x9a, 0x27,////, //26
		0x9b, 0x0b,////, //
		
		////////////////////////////////////////// Page 48 - MIPI////
		0x03, 0x48,////,
		
		0x10, 0x05,////,
		0x11, 0x00,////, //async_fifo off
		0x12, 0x00,////,
		
		0x16, 0xc4,////,
		0x17, 0x00,////,
		0x19, 0x00,////,
		0x1a, 0x00,////, //06 
		0x1c, 0x02,////,
		0x1d, 0x04,////,
		0x1e, 0x07,////,
		0x1f, 0x06,////, //04 
		0x20, 0x00,////,
		0x21, 0xb8,////,
		0x22, 0x00,////,
		0x23, 0x01,////,
		
		0x30, 0x05,////,
		0x31, 0x00,////,
		0x34, 0x02,////, //01 
		0x32, 0x06,////,
		0x35, 0x03,////, //02 
		0x36, 0x01,////,
		0x37, 0x03,////,
		0x38, 0x00,////,
		0x39, 0x4a,////, 
		0x3c, 0x00,////,
		0x3d, 0xfa,////,
		0x3f, 0x10,////,
		0x40, 0x00,////,
		0x41, 0x20,////,
		0x42, 0x00,////,
		////////////////////////////////////////////////////////
		
		///////////////////////////////////////////////  {0x50, 0x81},
		
		0x03, 0x22,////,
		0x10, 0xfb,////,
		
		0x03, 0x20,////,
		0x10, 0x9c,////,
		
		0x01, 0x70,////,
		
				///////////////////////////I2C_ID = 0x60
				/////////////////////////////I2C_BYTE  = 0x11
				
				/////////////////////////////END
				//////////////////////////////[END]
		
			
				};
				HI707_table_write_cmos_sensor(addr_data_pair, sizeof(addr_data_pair)/sizeof(kal_uint8));//
			}
		
#endif
		
#endif
	
}

static void HI707_Init_Parameter(void)
{
    spin_lock(&hi707_yuv_drv_lock);
    HI707_sensor.first_init = KAL_TRUE;
    HI707_sensor.Sensor_mode= SENSOR_MODE_PREVIEW;
    HI707_sensor.night_mode = KAL_FALSE;
    HI707_sensor.MPEG4_Video_mode = KAL_FALSE;

    HI707_sensor.cp_pclk = HI707_sensor.pv_pclk;

    HI707_sensor.pv_dummy_pixels = 0;
    HI707_sensor.pv_dummy_lines = 0;
    HI707_sensor.cp_dummy_pixels = 0;
    HI707_sensor.cp_dummy_lines = 0;

    HI707_sensor.wb = 0;
    HI707_sensor.exposure = 0;
    HI707_sensor.effect = 0;
    HI707_sensor.banding = AE_FLICKER_MODE_50HZ;

    HI707_sensor.pv_line_length = 640;
    HI707_sensor.pv_frame_height = 480;
    HI707_sensor.cp_line_length = 640;
    HI707_sensor.cp_frame_height = 480;
    spin_unlock(&hi707_yuv_drv_lock);    
}

static kal_uint8 HI707_power_on(void)
{
    kal_uint8 HI707_sensor_id = 0;
    spin_lock(&hi707_yuv_drv_lock);
    HI707_sensor.pv_pclk = 13000000;
    spin_unlock(&hi707_yuv_drv_lock);
    //Software Reset
    HI707_write_cmos_sensor(0x01,0xf1);
    HI707_write_cmos_sensor(0x01,0xf3);
    HI707_write_cmos_sensor(0x01,0xf1);

    /* Read Sensor ID  */
    HI707_sensor_id = HI707_read_cmos_sensor(0x04);
    SENSORDB("[HI707YUV]:read Sensor ID:%x\n",HI707_sensor_id);	
	//HI707_sensor_id = HI707_SENSOR_ID;
    return HI707_sensor_id;
}


/*************************************************************************
* FUNCTION
*	HI707Open
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
UINT32 HI707Open(void)
{
    spin_lock(&hi707_yuv_drv_lock);
    sensor_id_fail = 0; 
    spin_unlock(&hi707_yuv_drv_lock);
    SENSORDB("[Enter]:HI707 Open func:");

    if (HI707_power_on() != HI707_SENSOR_ID) 
    {
        SENSORDB("[HI707]Error:read sensor ID fail\n");
        spin_lock(&hi707_yuv_drv_lock);
        sensor_id_fail = 1;
        spin_unlock(&hi707_yuv_drv_lock);
        return ERROR_SENSOR_CONNECT_FAIL;
    }

    /* Apply sensor initail setting*/
    HI707_Initial_Setting();
    HI707_Init_Parameter(); 

    SENSORDB("[Exit]:HI707 Open func\n");     
    return ERROR_NONE;
}	/* HI707Open() */

/*************************************************************************
* FUNCTION
*	HI707_GetSensorID
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
static kal_uint32 HI707_GetSensorID(kal_uint32 *sensorID)
{
    SENSORDB("[Enter]:HI707 Open func ");
    *sensorID = HI707_power_on() ;

    if (*sensorID != HI707_SENSOR_ID) 
    {
        SENSORDB("[HI707]Error:read sensor ID fail\n");
        spin_lock(&hi707_yuv_drv_lock);
        sensor_id_fail = 1;
        spin_unlock(&hi707_yuv_drv_lock);
        *sensorID = 0xFFFFFFFF; 
        return ERROR_SENSOR_CONNECT_FAIL;
    }	   

    return ERROR_NONE;    
}   /* HI707Open  */


/*************************************************************************
* FUNCTION
*	HI707Close
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
UINT32 HI707Close(void)
{

	return ERROR_NONE;
}	/* HI707Close() */



static void HI707_Set_Mirror_Flip(kal_uint8 image_mirror)
{
    /********************************************************
    * Page Mode 0: Reg 0x0011 bit[1:0] = [Y Flip : X Flip]
    * 0: Off; 1: On.
    *********************************************************/ 
    kal_uint8 temp_data;   
    SENSORDB("[Enter]:HI707 set Mirror_flip func:image_mirror=%d\n",image_mirror);	
    HI707_write_cmos_sensor(0x03,0x00);     //Page 0	
    temp_data = (HI707_read_cmos_sensor(0x11) & 0xfc);
    //HI707_sensor.mirror = (HI707_read_cmos_sensor(0x11) & 0xfc); 
    switch (image_mirror) 
    {
    case IMAGE_NORMAL:
        //HI707_sensor.mirror |= 0x00;
        temp_data |= 0x00;
        break;
    case IMAGE_H_MIRROR:
        //HI707_sensor.mirror |= 0x01;
        temp_data |= 0x01;
        break;
    case IMAGE_V_MIRROR:
        //HI707_sensor.mirror |= 0x02;
        temp_data |= 0x02;
        break;
    case IMAGE_HV_MIRROR:
        //HI707_sensor.mirror |= 0x03;
        temp_data |= 0x03;
        break;
    default:
        //HI707_sensor.mirror |= 0x00;
        temp_data |= 0x00;
    }
    spin_lock(&hi707_yuv_drv_lock);
    HI707_sensor.mirror = temp_data;
    spin_unlock(&hi707_yuv_drv_lock);
    HI707_write_cmos_sensor(0x11, HI707_sensor.mirror);
    SENSORDB("[Exit]:HI707 set Mirror_flip func\n");
}

#if 0
static void HI707_set_dummy(kal_uint16 dummy_pixels,kal_uint16 dummy_lines)
{	
    HI707_write_cmos_sensor(0x03, 0x00);                        //Page 0
    HI707_write_cmos_sensor(0x40,((dummy_pixels & 0x0F00))>>8);       //HBLANK
    HI707_write_cmos_sensor(0x41,(dummy_pixels & 0xFF));
    HI707_write_cmos_sensor(0x42,((dummy_lines & 0xFF00)>>8));       //VBLANK ( Vsync Type 1)
    HI707_write_cmos_sensor(0x43,(dummy_lines & 0xFF));
}  
#endif

// 640 * 480


static void HI707_Cal_Min_Frame_Rate(kal_uint16 min_framerate)
{
    kal_uint32 HI707_expmax = 0;
    kal_uint32 HI707_expbanding = 0;
    kal_uint32 temp_data;
	kal_uint32 BLC_TIME_TH_ONOFF;
	
      
    SENSORDB("[HI707] HI707_Cal_Min_Frame_Rate:min_fps=%d\n",min_framerate);

    //No Fixed Framerate
    HI707_write_cmos_sensor(0x01, HI707_read_cmos_sensor(0x01)|0x01);   //Sleep: For Write Reg
    HI707_write_cmos_sensor(0x03, 0x00);
    HI707_write_cmos_sensor(0x11, HI707_read_cmos_sensor(0x11)&0xfb);

    HI707_write_cmos_sensor(0x03, 0x20);
    HI707_write_cmos_sensor(0x10, HI707_read_cmos_sensor(0x10)&0x7f);   //Close AE

    HI707_write_cmos_sensor(0x11, 0x04);
    HI707_write_cmos_sensor(0x18, HI707_read_cmos_sensor(0x18)|0x08);   //Reset AE
    HI707_write_cmos_sensor(0x2a, 0xf0);
    HI707_write_cmos_sensor(0x2b, 0x34);

    HI707_write_cmos_sensor(0x03, 0x00);
    temp_data = ((HI707_read_cmos_sensor(0x40)<<8)|HI707_read_cmos_sensor(0x41));
    spin_lock(&hi707_yuv_drv_lock);
    HI707_sensor.pv_dummy_pixels = temp_data;
    HI707_sensor.pv_line_length = HI707_VGA_DEFAULT_PIXEL_NUMS+ HI707_sensor.pv_dummy_pixels ;
    spin_unlock(&hi707_yuv_drv_lock);

    if(HI707_sensor.banding == AE_FLICKER_MODE_50HZ)
    {
        HI707_expbanding = (HI707_sensor.pv_pclk/HI707_sensor.pv_line_length/100)*HI707_sensor.pv_line_length/8;
        HI707_expmax = HI707_expbanding*(100*10/min_framerate) ;

		SENSORDB("preview 50Hz HI707_expmax=%x\n",HI707_expmax);

		if(min_framerate==HI707_MIN_FRAMERATE_10)
			HI707_expmax=0x0278d0;
		else
			HI707_expmax=0x04f1a0;

	    HI707_write_cmos_sensor(0x03, 0x20);
	    HI707_write_cmos_sensor(0xa0, (HI707_expmax>>16)&0xff);
	    HI707_write_cmos_sensor(0xa1, (HI707_expmax>>8)&0xff);
	    HI707_write_cmos_sensor(0xa2, (HI707_expmax>>0)&0xff);


		
		BLC_TIME_TH_ONOFF = (100*10/min_framerate);

	    HI707_write_cmos_sensor(0x03, 0x0);

		HI707_write_cmos_sensor(0x90, (BLC_TIME_TH_ONOFF>>8)&0xff);//, //BLC_TIME_TH_ON
		HI707_write_cmos_sensor(0x91, BLC_TIME_TH_ONOFF&0xff);//, //BLC_TIME_TH_OFF 
    }
    else if(HI707_sensor.banding == AE_FLICKER_MODE_60HZ)
    {
        HI707_expbanding = (HI707_sensor.pv_pclk/HI707_sensor.pv_line_length/120)*HI707_sensor.pv_line_length/8;
        HI707_expmax = HI707_expbanding*(120*10/min_framerate) ;

		SENSORDB("preview 60Hz HI707_expmax=%x\n",HI707_expmax);

		if(min_framerate==HI707_MIN_FRAMERATE_10)
			HI707_expmax=0x0278d0;
		else
			HI707_expmax=0x04f1a0;
		
	    HI707_write_cmos_sensor(0x03, 0x20);
	    HI707_write_cmos_sensor(0x88, (HI707_expmax>>16)&0xff);
	    HI707_write_cmos_sensor(0x89, (HI707_expmax>>8)&0xff);
	    HI707_write_cmos_sensor(0x8a, (HI707_expmax>>0)&0xff);

			
		BLC_TIME_TH_ONOFF = (120*10/min_framerate);

	    HI707_write_cmos_sensor(0x03, 0x0);

		HI707_write_cmos_sensor(0x90, (BLC_TIME_TH_ONOFF>>8)&0xff);//, //BLC_TIME_TH_ON
		HI707_write_cmos_sensor(0x91, BLC_TIME_TH_ONOFF&0xff);//, //BLC_TIME_TH_OFF 
    }
    else//default 5oHZ
    {
        //SENSORDB("[HI707][Error] Wrong Banding Setting!!!...");
        HI707_expbanding = (HI707_sensor.pv_pclk/HI707_sensor.pv_line_length/100)*HI707_sensor.pv_line_length/8;
        HI707_expmax = HI707_expbanding*(100*10/min_framerate) ;

		SENSORDB("preview default 50Hz HI707_expmax=%x\n",HI707_expmax);

		if(min_framerate==HI707_MIN_FRAMERATE_10)
			HI707_expmax=0x0278d0;
		else
			HI707_expmax=0x04f1a0;

	    HI707_write_cmos_sensor(0x03, 0x20);
	    HI707_write_cmos_sensor(0xa0, (HI707_expmax>>16)&0xff);
	    HI707_write_cmos_sensor(0xa1, (HI707_expmax>>8)&0xff);
	    HI707_write_cmos_sensor(0xa2, (HI707_expmax>>0)&0xff);


		BLC_TIME_TH_ONOFF = (100*10/min_framerate);

	    HI707_write_cmos_sensor(0x03, 0x0);

		HI707_write_cmos_sensor(0x90, (BLC_TIME_TH_ONOFF>>8)&0xff);//, //BLC_TIME_TH_ON
		HI707_write_cmos_sensor(0x91, BLC_TIME_TH_ONOFF&0xff);//, //BLC_TIME_TH_OFF 

	}
        

    HI707_write_cmos_sensor(0x01, HI707_read_cmos_sensor(0x01)&0xfe);   //Exit Sleep: For Write Reg

    HI707_write_cmos_sensor(0x03, 0x20);
    HI707_write_cmos_sensor(0x10, HI707_read_cmos_sensor(0x10)|0x80);   //Open AE
    HI707_write_cmos_sensor(0x18, HI707_read_cmos_sensor(0x18)&0xf7);   //Reset AE





	
}


static void HI707_Fix_Video_Frame_Rate(kal_uint16 fix_framerate)
{
    kal_uint32 HI707_expfix;
    kal_uint32 HI707_expfix_temp;
    kal_uint32 HI707_expmax=0,HI707_expmax50= 0,HI707_expmax60;
    kal_uint32 HI707_expbanding = 0;
    kal_uint32 temp_data1,temp_data2;
	kal_uint32 BLC_TIME_TH_ONOFF;
      
    SENSORDB("[Enter]HI707 Fix_video_frame_rate func: fix_fps=%d\n",fix_framerate);

    spin_lock(&hi707_yuv_drv_lock);
    HI707_sensor.video_current_frame_rate = fix_framerate;
    spin_unlock(&hi707_yuv_drv_lock);
    // Fixed Framerate
    HI707_write_cmos_sensor(0x01, HI707_read_cmos_sensor(0x01)|0x01);   //Sleep: For Write Reg

    HI707_write_cmos_sensor(0x03, 0x00);
    HI707_write_cmos_sensor(0x11, HI707_read_cmos_sensor(0x11)|0x04);

    HI707_write_cmos_sensor(0x03, 0x20);
    HI707_write_cmos_sensor(0x10, HI707_read_cmos_sensor(0x10)&0x7f);   //Close AE

    HI707_write_cmos_sensor(0x11, 0x00);
    HI707_write_cmos_sensor(0x18, HI707_read_cmos_sensor(0x18)|0x08);   //Reset AE
    HI707_write_cmos_sensor(0x2a, 0x00);
    HI707_write_cmos_sensor(0x2b, 0x35);

    HI707_write_cmos_sensor(0x03, 0x00);
    temp_data1 = ((HI707_read_cmos_sensor(0x40)<<8)|HI707_read_cmos_sensor(0x41));
    temp_data2 = ((HI707_read_cmos_sensor(0x42)<<8)|HI707_read_cmos_sensor(0x43));
    spin_lock(&hi707_yuv_drv_lock);
    HI707_sensor.pv_dummy_pixels = temp_data1; 
    HI707_sensor.pv_line_length = HI707_VGA_DEFAULT_PIXEL_NUMS + HI707_sensor.pv_dummy_pixels ;   
    HI707_sensor.pv_dummy_lines = temp_data2;
    spin_unlock(&hi707_yuv_drv_lock);
        
    //HI707_expfix_temp = ((HI707_sensor.pv_pclk*10/fix_framerate)-(HI707_sensor.pv_line_length*HI707_sensor.pv_dummy_lines))/8;   
    HI707_expfix_temp = ((HI707_sensor.pv_pclk*10/fix_framerate) )/8;  
    HI707_expfix = ((HI707_expfix_temp*8/HI707_sensor.pv_line_length)*HI707_sensor.pv_line_length)/8;


    SENSORDB("HI707_expfix=%x\n",HI707_expfix);

	
	if(fix_framerate==300)
		HI707_expfix=0x00d354;
	else
		{
		fix_framerate = 15;
		HI707_expfix=0x1a70c;
		}
        
    HI707_write_cmos_sensor(0x03, 0x20);    
    //HI707_write_cmos_sensor(0x83, (HI707_expfix>>16)&0xff);
    //HI707_write_cmos_sensor(0x84, (HI707_expfix>>8)&0xff);
    //HI707_write_cmos_sensor(0x85, (HI707_expfix>>0)&0xff);    
    HI707_write_cmos_sensor(0x91, (HI707_expfix>>16)&0xff);
    HI707_write_cmos_sensor(0x92, (HI707_expfix>>8)&0xff);
    HI707_write_cmos_sensor(0x93, (HI707_expfix>>0)&0xff);

    if(HI707_sensor.banding == AE_FLICKER_MODE_50HZ)
    {
        HI707_expbanding = ((HI707_read_cmos_sensor(0x8b)<<8)|HI707_read_cmos_sensor(0x8c));
		
		//HI707_expmax = ((HI707_expfix_temp-HI707_expbanding)/HI707_expbanding)*HI707_expbanding;	
		 HI707_expbanding = (HI707_sensor.pv_pclk/HI707_sensor.pv_line_length/100)*HI707_sensor.pv_line_length/8;
              HI707_expmax = HI707_expbanding*(100*10/fix_framerate) ;

		
		SENSORDB("video 50Hz HI707_expmax=%x\n",HI707_expmax);

		if(fix_framerate==300)
			HI707_expmax=0x00bdd8;
		else
			HI707_expmax=0x017bb0;

		
	    HI707_write_cmos_sensor(0x03, 0x20);
	    HI707_write_cmos_sensor(0xa0, (HI707_expmax>>16)&0xff);
	    HI707_write_cmos_sensor(0xa1, (HI707_expmax>>8)&0xff);
	    HI707_write_cmos_sensor(0xa2, (HI707_expmax>>0)&0xff);

		
		BLC_TIME_TH_ONOFF = (100*10/fix_framerate);
		
		HI707_write_cmos_sensor(0x03, 0x0);
		
		HI707_write_cmos_sensor(0x90, (BLC_TIME_TH_ONOFF>>8)&0xff);//, //BLC_TIME_TH_ON
		HI707_write_cmos_sensor(0x91, BLC_TIME_TH_ONOFF&0xff);//, //BLC_TIME_TH_OFF 

		
		
		HI707_write_cmos_sensor(0x01, HI707_read_cmos_sensor(0x01)&0xfe);	//Exit Sleep: For Write Reg
		
		HI707_write_cmos_sensor(0x03, 0x20);
		HI707_write_cmos_sensor(0x10, HI707_read_cmos_sensor(0x10)|0x80);	//Open AE
		HI707_write_cmos_sensor(0x18, HI707_read_cmos_sensor(0x18)&0xf7);	//Reset AE
    }
    else if(HI707_sensor.banding == AE_FLICKER_MODE_60HZ)
    {
        HI707_expbanding = ((HI707_read_cmos_sensor(0x8d)<<8)|HI707_read_cmos_sensor(0x8e));
		
		//HI707_expmax = ((HI707_expfix_temp-HI707_expbanding)/HI707_expbanding)*HI707_expbanding;	
			 HI707_expbanding = (HI707_sensor.pv_pclk/HI707_sensor.pv_line_length/120)*HI707_sensor.pv_line_length/8;
             // HI707_expmax = HI707_expbanding*120*10/fix_framerate ;
              HI707_expmax = HI707_expbanding*(120*10/fix_framerate) ;


		SENSORDB("video 60Hz HI707_expmax=%x\n",HI707_expmax);


		if(fix_framerate==300)
		{	HI707_expmax50=0x00bdd8;
			HI707_expmax60=0x00d2f0;
			}
			
		else
			HI707_expmax=0x01a5e0;


		HI707_write_cmos_sensor(0x03, 0x20);
		HI707_write_cmos_sensor(0x83, (HI707_expmax60>>16)&0xff);
		HI707_write_cmos_sensor(0x84, (HI707_expmax60>>8)&0xff);
		HI707_write_cmos_sensor(0x85, (HI707_expmax60>>0)&0xff);
		HI707_write_cmos_sensor(0x88, (HI707_expmax60>>16)&0xff);
		HI707_write_cmos_sensor(0x89, (HI707_expmax60>>8)&0xff);
		HI707_write_cmos_sensor(0x8a, (HI707_expmax60>>0)&0xff);
		HI707_write_cmos_sensor(0x03, 0x20);
	    HI707_write_cmos_sensor(0xa0, (HI707_expmax50>>16)&0xff);
	    HI707_write_cmos_sensor(0xa1, (HI707_expmax50>>8)&0xff);
	    HI707_write_cmos_sensor(0xa2, (HI707_expmax50>>0)&0xff);

		
		BLC_TIME_TH_ONOFF = (120*10/fix_framerate);
		
		HI707_write_cmos_sensor(0x03, 0x0);
		
		HI707_write_cmos_sensor(0x90, (BLC_TIME_TH_ONOFF>>8)&0xff);//, //BLC_TIME_TH_ON
		HI707_write_cmos_sensor(0x91, BLC_TIME_TH_ONOFF&0xff);//, //BLC_TIME_TH_OFF 
		
		HI707_write_cmos_sensor(0x01, HI707_read_cmos_sensor(0x01)&0xfe);	//Exit Sleep: For Write Reg
		
		HI707_write_cmos_sensor(0x03, 0x20);
		HI707_write_cmos_sensor(0x10, HI707_read_cmos_sensor(0x10)|0x80);	//Open AE
		HI707_write_cmos_sensor(0x18, HI707_read_cmos_sensor(0x18)&0xf7);	//Reset AE
    }
    else//default 50HZ
    {
        HI707_expbanding = ((HI707_read_cmos_sensor(0x8b)<<8)|HI707_read_cmos_sensor(0x8c));
		
		//HI707_expmax = ((HI707_expfix_temp-HI707_expbanding)/HI707_expbanding)*HI707_expbanding;	
			 HI707_expbanding = (HI707_sensor.pv_pclk/HI707_sensor.pv_line_length/100)*HI707_sensor.pv_line_length/8;
              HI707_expmax = HI707_expbanding*(100*10/fix_framerate) ;

		SENSORDB("video default 50Hz HI707_expmax=%x\n",HI707_expmax);

		if(fix_framerate==300)
			HI707_expmax=0x00bdd8;
		else
			HI707_expmax=0x017bb0;


	    HI707_write_cmos_sensor(0x03, 0x20);
	    HI707_write_cmos_sensor(0xa0, (HI707_expmax>>16)&0xff);
	    HI707_write_cmos_sensor(0xa1, (HI707_expmax>>8)&0xff);
	    HI707_write_cmos_sensor(0xa2, (HI707_expmax>>0)&0xff);


		
		BLC_TIME_TH_ONOFF = (100*10/fix_framerate);
		
		HI707_write_cmos_sensor(0x03, 0x0);
		
		HI707_write_cmos_sensor(0x90, (BLC_TIME_TH_ONOFF>>8)&0xff);//, //BLC_TIME_TH_ON
		HI707_write_cmos_sensor(0x91, BLC_TIME_TH_ONOFF&0xff);//, //BLC_TIME_TH_OFF 

		

		HI707_write_cmos_sensor(0x01, HI707_read_cmos_sensor(0x01)&0xfe);	//Exit Sleep: For Write Reg
		
		HI707_write_cmos_sensor(0x03, 0x20);
		HI707_write_cmos_sensor(0x10, HI707_read_cmos_sensor(0x10)|0x80);	//Open AE
		HI707_write_cmos_sensor(0x18, HI707_read_cmos_sensor(0x18)&0xf7);	//Reset AE
    }


	
}

#if 0
// 320 * 240
static void HI707_Set_QVGA_mode(void)
{
    HI707_write_cmos_sensor(0x01, HI707_read_cmos_sensor(0x01)|0x01);   //Sleep: For Write Reg
    
    HI707_write_cmos_sensor(0x03, 0x00);
    HI707_write_cmos_sensor(0x10, 0x01);        //QVGA Size: 0x10 -> 0x01

    HI707_write_cmos_sensor(0x20, 0x00);
    HI707_write_cmos_sensor(0x21, 0x02);

    HI707_write_cmos_sensor(0x40, 0x01);        //HBLANK:  0x0158 = 344
    HI707_write_cmos_sensor(0x41, 0x58);
    HI707_write_cmos_sensor(0x42, 0x00);        //VBLANK:  0x14 = 20
    HI707_write_cmos_sensor(0x43, 0x14);

    HI707_write_cmos_sensor(0x03, 0x11);        //QVGA Fixframerate
    HI707_write_cmos_sensor(0x10, 0x21);  

    HI707_write_cmos_sensor(0x03, 0x20);
    HI707_write_cmos_sensor(0x10, HI707_read_cmos_sensor(0x10)&0x7f);   //Close AE
    HI707_write_cmos_sensor(0x18, HI707_read_cmos_sensor(0x18)|0x08);   //Reset AE

    HI707_write_cmos_sensor(0x83, 0x00);
    HI707_write_cmos_sensor(0x84, 0xaf);
    HI707_write_cmos_sensor(0x85, 0xc8);
    HI707_write_cmos_sensor(0x86, 0x00);
    HI707_write_cmos_sensor(0x87, 0xfa);

    HI707_write_cmos_sensor(0x8b, 0x3a);
    HI707_write_cmos_sensor(0x8c, 0x98);
    HI707_write_cmos_sensor(0x8d, 0x30);
    HI707_write_cmos_sensor(0x8e, 0xd4);

    HI707_write_cmos_sensor(0x9c, 0x0b);
    HI707_write_cmos_sensor(0x9d, 0x3b);
    HI707_write_cmos_sensor(0x9e, 0x00);
    HI707_write_cmos_sensor(0x9f, 0xfa);

    HI707_write_cmos_sensor(0x01, HI707_read_cmos_sensor(0x01)&0xfe);   //Exit Sleep: For Write Reg

    HI707_write_cmos_sensor(0x03, 0x20);
    HI707_write_cmos_sensor(0x10, HI707_read_cmos_sensor(0x10)|0x80);   //Open AE
    HI707_write_cmos_sensor(0x18, HI707_read_cmos_sensor(0x18)&0xf7);   //Reset AE

}
#endif
void HI707_night_mode(kal_bool enable)
{
    SENSORDB("HHL[Enter]HI707 night mode func:enable = %d\n",enable);
    SENSORDB("HI707_sensor.video_mode = %d\n",HI707_sensor.MPEG4_Video_mode); 
    SENSORDB("HI707_sensor.night_mode = %d\n",HI707_sensor.night_mode);
    spin_lock(&hi707_yuv_drv_lock);
    HI707_sensor.night_mode = enable;
    spin_unlock(&hi707_yuv_drv_lock);

  //  if(HI707_sensor.MPEG4_Video_mode == KAL_TRUE)
   //     return;

    if(enable)
    {
        HI707_Cal_Min_Frame_Rate(HI707_MIN_FRAMERATE_5);                            
    }
    else
    {
        HI707_Cal_Min_Frame_Rate(HI707_MIN_FRAMERATE_10);
    }
}

/*************************************************************************
* FUNCTION
*	HI707Preview
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
static UINT32 HI707Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    spin_lock(&hi707_yuv_drv_lock);
    sensor_config_data->SensorImageMirror = IMAGE_HV_MIRROR; 
    if(HI707_sensor.first_init == KAL_TRUE)
    {
        HI707_sensor.MPEG4_Video_mode = HI707_sensor.MPEG4_Video_mode;
    }
    else
    {
        HI707_sensor.MPEG4_Video_mode = !HI707_sensor.MPEG4_Video_mode;
    }
    spin_unlock(&hi707_yuv_drv_lock);
    
    SENSORDB("HHL[Enter]:HI707 preview func:");		
    SENSORDB("HI707_sensor.video_mode = %d\n",HI707_sensor.MPEG4_Video_mode); 
    
    spin_lock(&hi707_yuv_drv_lock);
    HI707_sensor.first_init = KAL_FALSE;	
    spin_unlock(&hi707_yuv_drv_lock);
    if(sensor_config_data->SensorOperationMode== ACDK_SENSOR_OPERATION_MODE_VIDEO)	
    {
        spin_lock(&hi707_yuv_drv_lock);
        HI707_sensor.Sensor_mode =SENSOR_MODE_VIDEO;
        spin_unlock(&hi707_yuv_drv_lock);
    }
    else
    {
        spin_lock(&hi707_yuv_drv_lock);
        HI707_sensor.Sensor_mode =SENSOR_MODE_PREVIEW;
        spin_unlock(&hi707_yuv_drv_lock);
    }
    //spin_unlock(&hi707_yuv_drv_lock);
    SENSORDB("[Exit]:HI707 preview func\n");
    
    return TRUE; 
}	/* HI707_Preview */


UINT32 HI707Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    SENSORDB("HHL[HI707][Enter]HI707_capture_func\n");
    spin_lock(&hi707_yuv_drv_lock);
    HI707_sensor.Sensor_mode =SENSOR_MODE_CAPTURE;
    spin_unlock(&hi707_yuv_drv_lock);
    return ERROR_NONE;
}	/* HM3451Capture() */


UINT32 HI707GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
    SENSORDB("[Enter]:HI707 get Resolution func\n");

    pSensorResolution->SensorFullWidth=HI707_IMAGE_SENSOR_FULL_WIDTH ;  
    pSensorResolution->SensorFullHeight=HI707_IMAGE_SENSOR_FULL_HEIGHT ;
    pSensorResolution->SensorPreviewWidth=HI707_IMAGE_SENSOR_PV_WIDTH ;
    pSensorResolution->SensorPreviewHeight=HI707_IMAGE_SENSOR_PV_HEIGHT ;
    pSensorResolution->SensorVideoWidth=HI707_IMAGE_SENSOR_PV_WIDTH ;
    pSensorResolution->SensorVideoHeight=HI707_IMAGE_SENSOR_PV_HEIGHT ;
    pSensorResolution->Sensor3DFullWidth=HI707_IMAGE_SENSOR_FULL_WIDTH ;  
    pSensorResolution->Sensor3DFullHeight=HI707_IMAGE_SENSOR_FULL_HEIGHT ;
    pSensorResolution->Sensor3DPreviewWidth=HI707_IMAGE_SENSOR_PV_WIDTH ;
    pSensorResolution->Sensor3DPreviewHeight=HI707_IMAGE_SENSOR_PV_HEIGHT ;
    pSensorResolution->Sensor3DVideoWidth=HI707_IMAGE_SENSOR_PV_WIDTH ;
    pSensorResolution->Sensor3DVideoHeight=HI707_IMAGE_SENSOR_PV_HEIGHT ;

    SENSORDB("[Exit]:HI707 get Resolution func\n");	
    return ERROR_NONE;
}	/* HI707GetResolution() */

UINT32 HI707GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
					  MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{

	switch(ScenarioId)
		{
		 
			case MSDK_SCENARIO_ID_CAMERA_ZSD:
				 pSensorInfo->SensorPreviewResolutionX=HI707_IMAGE_SENSOR_PV_WIDTH;
				 pSensorInfo->SensorPreviewResolutionY=HI707_IMAGE_SENSOR_PV_HEIGHT;
				 pSensorInfo->SensorFullResolutionX=HI707_IMAGE_SENSOR_FULL_WIDTH;
				 pSensorInfo->SensorFullResolutionY=HI707_IMAGE_SENSOR_FULL_HEIGHT;			 
				 pSensorInfo->SensorCameraPreviewFrameRate=15;
				 break;
	
			case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
				 pSensorInfo->SensorPreviewResolutionX=HI707_IMAGE_SENSOR_PV_WIDTH;
				 pSensorInfo->SensorPreviewResolutionY=HI707_IMAGE_SENSOR_PV_HEIGHT;
				 pSensorInfo->SensorFullResolutionX=HI707_IMAGE_SENSOR_FULL_WIDTH;
				 pSensorInfo->SensorFullResolutionY=HI707_IMAGE_SENSOR_FULL_HEIGHT;				 
				 pSensorInfo->SensorCameraPreviewFrameRate=30;
				 break;
			case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
				 pSensorInfo->SensorPreviewResolutionX=HI707_IMAGE_SENSOR_PV_WIDTH;
				 pSensorInfo->SensorPreviewResolutionY=HI707_IMAGE_SENSOR_PV_HEIGHT;
				 pSensorInfo->SensorFullResolutionX=HI707_IMAGE_SENSOR_FULL_WIDTH;
				 pSensorInfo->SensorFullResolutionY=HI707_IMAGE_SENSOR_FULL_HEIGHT;				 
				 pSensorInfo->SensorCameraPreviewFrameRate=30;			
				break;
			case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
			case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
			case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added   
				 pSensorInfo->SensorPreviewResolutionX=HI707_IMAGE_SENSOR_PV_WIDTH;
				 pSensorInfo->SensorPreviewResolutionY=HI707_IMAGE_SENSOR_PV_HEIGHT;
				 pSensorInfo->SensorFullResolutionX=HI707_IMAGE_SENSOR_FULL_WIDTH;
				 pSensorInfo->SensorFullResolutionY=HI707_IMAGE_SENSOR_FULL_HEIGHT; 			 
				 pSensorInfo->SensorCameraPreviewFrameRate=30;			
				break;
			default:
	
				 pSensorInfo->SensorPreviewResolutionX=HI707_IMAGE_SENSOR_PV_WIDTH;
				 pSensorInfo->SensorPreviewResolutionY=HI707_IMAGE_SENSOR_PV_HEIGHT;
				 pSensorInfo->SensorFullResolutionX=HI707_IMAGE_SENSOR_FULL_WIDTH;
				 pSensorInfo->SensorFullResolutionY=HI707_IMAGE_SENSOR_FULL_HEIGHT;				 
				 pSensorInfo->SensorCameraPreviewFrameRate=30;
				 break;
				 
			}
	


    SENSORDB("[Enter]:HI707 getInfo func:ScenarioId = %d\n",ScenarioId);

  //  pSensorInfo->SensorPreviewResolutionX=HI707_IMAGE_SENSOR_PV_WIDTH;
  //  pSensorInfo->SensorPreviewResolutionY=HI707_IMAGE_SENSOR_PV_HEIGHT;
 //   pSensorInfo->SensorFullResolutionX=HI707_IMAGE_SENSOR_FULL_WIDTH;
 //   pSensorInfo->SensorFullResolutionY=HI707_IMAGE_SENSOR_FULL_HEIGHT;

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
    pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_MIPI;


    pSensorInfo->CaptureDelayFrame = 4; 
    pSensorInfo->PreviewDelayFrame = 4;//10; 
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
    //	HI707_PixelClockDivider=pSensorInfo->SensorPixelClockCount;

	
	pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE; 		
	pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 4; 
	pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;

	
    memcpy(pSensorConfigData, &HI707SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

    SENSORDB("[Exit]:HI707 getInfo func\n");	
    return ERROR_NONE;
}	/* HI707GetInfo() */


UINT32 HI707Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    SENSORDB("HHL [Enter]:HI707 Control func:ScenarioId = %d\n",ScenarioId);

    switch (ScenarioId)
    {
    case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
    //case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW:
    //case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:		
        HI707Preview(pImageWindow, pSensorConfigData); 
        break;
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
    //case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE:
        HI707Capture(pImageWindow, pSensorConfigData); 
        break;
     //   case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
     //   case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
    //    case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added   
    //    HI707Preview(pImageWindow, pSensorConfigData); 
     //   break;
	//	case MSDK_SCENARIO_ID_CAMERA_ZSD:
	//	HI707Capture(pImageWindow, pSensorConfigData); 
	//	break;		
    default:
		 HI707Preview(pImageWindow, pSensorConfigData); 
        break; 
    }

    SENSORDB("[Exit]:HI707 Control func\n");	
    return TRUE;
}	/* HI707Control() */

void HI707_set_scene_mode(UINT16 para)
{


 	SENSORDB("[Hi704_Debug]enter HI707_set_scene_mode function:\n ");	
	SENSORDB("[Hi704_Debug] HI707_set_scene_mode=%d",para);	
    spin_lock(&hi707_yuv_drv_lock);
    HI707_sensor.SceneMode = para;
    spin_unlock(&hi707_yuv_drv_lock);
  		
		switch (para)
		{
		
		case SCENE_MODE_NIGHTSCENE:
			
			HI707_night_mode(KAL_TRUE);
			break;

		case SCENE_MODE_HDR:
			SENSORDB("[Hi704_Debug]enter HI707_set_scene_mode function HDR:\n ");
			break;
		default :
			HI707_night_mode(KAL_FALSE);
			break;
		}
  	return;
}


/*************************************************************************
* FUNCTION
*	HI707_set_param_wb
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
BOOL HI707_set_param_wb(UINT16 para)
{
    //This sensor need more time to balance AWB, 
    //we suggest higher fps or drop some frame to avoid garbage color when preview initial
    SENSORDB("[Enter]HI707 set_param_wb func:para = %d\n",para);

    if(HI707_sensor.wb == para) return KAL_TRUE;	

    spin_lock(&hi707_yuv_drv_lock);
    HI707_sensor.wb = para;
    spin_unlock(&hi707_yuv_drv_lock);
    
    switch (para)
    {            
    case AWB_MODE_AUTO:
        {
        HI707_write_cmos_sensor(0x03, 0x22);			
        HI707_write_cmos_sensor(0x11, 0x2e);				
        HI707_write_cmos_sensor(0x80, 0x38);
        HI707_write_cmos_sensor(0x82, 0x38);				
        HI707_write_cmos_sensor(0x83, 0x5e);
        HI707_write_cmos_sensor(0x84, 0x24);
        HI707_write_cmos_sensor(0x85, 0x59);
        HI707_write_cmos_sensor(0x86, 0x24);				
        HI707_write_cmos_sensor(0x10, 0xfb);				
        }                
        break;
    case AWB_MODE_CLOUDY_DAYLIGHT:
    {
        HI707_write_cmos_sensor(0x03, 0x22);
        HI707_write_cmos_sensor(0x11, 0x28);
        HI707_write_cmos_sensor(0x80, 0x71);
        HI707_write_cmos_sensor(0x82, 0x2b);
        HI707_write_cmos_sensor(0x83, 0x72);
        HI707_write_cmos_sensor(0x84, 0x70);
        HI707_write_cmos_sensor(0x85, 0x2b);
        HI707_write_cmos_sensor(0x86, 0x28);
        HI707_write_cmos_sensor(0x10, 0xfb);
        }			   
        break;
    case AWB_MODE_DAYLIGHT:
        {
        HI707_write_cmos_sensor(0x03, 0x22);
        HI707_write_cmos_sensor(0x11, 0x28);          
        HI707_write_cmos_sensor(0x80, 0x59);
        HI707_write_cmos_sensor(0x82, 0x29);
        HI707_write_cmos_sensor(0x83, 0x60);
        HI707_write_cmos_sensor(0x84, 0x50);
        HI707_write_cmos_sensor(0x85, 0x2f);
        HI707_write_cmos_sensor(0x86, 0x23);
        HI707_write_cmos_sensor(0x10, 0xfb);
        }      
        break;
    case AWB_MODE_INCANDESCENT:	
        {
        HI707_write_cmos_sensor(0x03, 0x22);
        HI707_write_cmos_sensor(0x11, 0x28);          
        HI707_write_cmos_sensor(0x80, 0x29);
        HI707_write_cmos_sensor(0x82, 0x54);
        HI707_write_cmos_sensor(0x83, 0x2e);
        HI707_write_cmos_sensor(0x84, 0x23);
        HI707_write_cmos_sensor(0x85, 0x58);
        HI707_write_cmos_sensor(0x86, 0x4f);
        HI707_write_cmos_sensor(0x10, 0xfb);
        }		
        break;  
    case AWB_MODE_FLUORESCENT:
        {
        HI707_write_cmos_sensor(0x03, 0x22);
        HI707_write_cmos_sensor(0x11, 0x28);
        HI707_write_cmos_sensor(0x80, 0x41);
        HI707_write_cmos_sensor(0x82, 0x42);
        HI707_write_cmos_sensor(0x83, 0x44);
        HI707_write_cmos_sensor(0x84, 0x34);
        HI707_write_cmos_sensor(0x85, 0x46);
        HI707_write_cmos_sensor(0x86, 0x3a);
        HI707_write_cmos_sensor(0x10, 0xfb);
        }	
        break;  
    case AWB_MODE_TUNGSTEN:
        {
        HI707_write_cmos_sensor(0x03, 0x22);
        HI707_write_cmos_sensor(0x80, 0x24);
        HI707_write_cmos_sensor(0x81, 0x20);
        HI707_write_cmos_sensor(0x82, 0x58);
        HI707_write_cmos_sensor(0x83, 0x27);
        HI707_write_cmos_sensor(0x84, 0x22);
        HI707_write_cmos_sensor(0x85, 0x58);
        HI707_write_cmos_sensor(0x86, 0x52);
        HI707_write_cmos_sensor(0x10, 0xfb);
        }
        break;
    case AWB_MODE_OFF:
        {
        SENSORDB("HI707 AWB OFF");
        HI707_write_cmos_sensor(0x03, 0x22);
        HI707_write_cmos_sensor(0x10, 0xe2);
        }
        break;
    default:
        return FALSE;
    }

    return TRUE;	
} /* HI707_set_param_wb */

/*************************************************************************
* FUNCTION
*	HI707_set_param_effect
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
BOOL HI707_set_param_effect(UINT16 para)
{
   SENSORDB("[Enter]HI707 set_param_effect func:para = %d\n",para);
   
    if(HI707_sensor.effect == para) return KAL_TRUE;

    spin_lock(&hi707_yuv_drv_lock);
    HI707_sensor.effect = para;
    spin_unlock(&hi707_yuv_drv_lock);
    
    switch (para)
    {
    case MEFFECT_OFF:
        {
        HI707_write_cmos_sensor(0x03, 0x10);
        HI707_write_cmos_sensor(0x11, 0x03);
        HI707_write_cmos_sensor(0x12, 0x30);
        HI707_write_cmos_sensor(0x13, 0x00);
        HI707_write_cmos_sensor(0x44, 0x80);
        HI707_write_cmos_sensor(0x45, 0x80);

        HI707_write_cmos_sensor(0x47, 0x7f);
        HI707_write_cmos_sensor(0x03, 0x13);
        HI707_write_cmos_sensor(0x20, 0x07);
        HI707_write_cmos_sensor(0x21, 0x07);
        }
        break;
    case MEFFECT_SEPIA:
        {
        HI707_write_cmos_sensor(0x03, 0x10);
        HI707_write_cmos_sensor(0x11, 0x03);
        HI707_write_cmos_sensor(0x12, 0x23);
        HI707_write_cmos_sensor(0x13, 0x00);
        HI707_write_cmos_sensor(0x44, 0x70);
        HI707_write_cmos_sensor(0x45, 0x98);

        HI707_write_cmos_sensor(0x47, 0x7f);
        HI707_write_cmos_sensor(0x03, 0x13);
        HI707_write_cmos_sensor(0x20, 0x07);
        HI707_write_cmos_sensor(0x21, 0x07);
        }	
        break;  
    case MEFFECT_NEGATIVE:
        {
        HI707_write_cmos_sensor(0x03, 0x10);
        HI707_write_cmos_sensor(0x11, 0x03);
        HI707_write_cmos_sensor(0x12, 0x08);
        HI707_write_cmos_sensor(0x13, 0x00);
        HI707_write_cmos_sensor(0x14, 0x00);
        }
        break; 
    case MEFFECT_SEPIAGREEN:		
        {
        HI707_write_cmos_sensor(0x03, 0x10);
        HI707_write_cmos_sensor(0x11, 0x03);
        HI707_write_cmos_sensor(0x12, 0x03);
        //HI707_write_cmos_sensor(0x40, 0x00);
        HI707_write_cmos_sensor(0x13, 0x00);
        HI707_write_cmos_sensor(0x44, 0x30);
        HI707_write_cmos_sensor(0x45, 0x50);
        }	
        break;
    case MEFFECT_SEPIABLUE:
        {
        HI707_write_cmos_sensor(0x03, 0x10);
        HI707_write_cmos_sensor(0x11, 0x03);
        HI707_write_cmos_sensor(0x12, 0x03);
        //HI707_write_cmos_sensor(0x40, 0x00);
        HI707_write_cmos_sensor(0x13, 0x00);
        HI707_write_cmos_sensor(0x44, 0xb0);
        HI707_write_cmos_sensor(0x45, 0x40);
        }     
        break;        
    case MEFFECT_MONO:			
        {
        HI707_write_cmos_sensor(0x03, 0x10);
        HI707_write_cmos_sensor(0x11, 0x03);
        HI707_write_cmos_sensor(0x12, 0x03);
        //HI707_write_cmos_sensor(0x40, 0x00);
        HI707_write_cmos_sensor(0x44, 0x80);
        HI707_write_cmos_sensor(0x45, 0x80);
        }
        break;
    default:
        return KAL_FALSE;
    }

    return KAL_TRUE;
} /* HI707_set_param_effect */

/*************************************************************************
* FUNCTION
*	HI707_set_param_banding
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
BOOL HI707_set_param_banding(UINT16 para)
{
    SENSORDB("[Enter]HI707 set_param_banding func:para = %d\n",para);

    if(HI707_sensor.banding == para) return KAL_TRUE;

    spin_lock(&hi707_yuv_drv_lock);
    HI707_sensor.banding = para;
    spin_unlock(&hi707_yuv_drv_lock);

    switch (para)
    {
    case AE_FLICKER_MODE_50HZ:
        {
        HI707_write_cmos_sensor(0x03,0x20);
        HI707_write_cmos_sensor(0x10,0x9c);
        }
        break;
    case AE_FLICKER_MODE_60HZ:
        {
        HI707_write_cmos_sensor(0x03,0x20);
        HI707_write_cmos_sensor(0x10,0x8c);
        }
        break;
    default:
        return KAL_FALSE;
    }
    
    return KAL_TRUE;
} /* HI707_set_param_banding */




/*************************************************************************
* FUNCTION
*	HI707_set_param_exposure
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
BOOL HI707_set_param_exposure(UINT16 para)
{
    SENSORDB("[Enter]HI707 set_param_exposure func:para = %d\n",para);

    if(HI707_sensor.exposure == para) 
	return KAL_TRUE;
    spin_lock(&hi707_yuv_drv_lock);
    HI707_sensor.exposure = para;
    spin_unlock(&hi707_yuv_drv_lock);
	if (SCENE_MODE_HDR == HI707_sensor.SceneMode && SENSOR_MODE_CAPTURE == HI707_sensor.Sensor_mode)
	{
			  
			   switch (para)
					{
					  case AE_EV_COMP_n10:	
					  case AE_EV_COMP_n20:	
						  /* EV -2 */
						  SENSORDB("[Hi704_Debug]HDR AE_EV_COMP_n20 Para:%d;\n",para);	 
						  HI707_write_cmos_sensor(0x03,0x10);
						  HI707_write_cmos_sensor(0x12,HI707_read_cmos_sensor(0x12)|0x10);
						  HI707_write_cmos_sensor(0x40,0xe0);
					HI707_write_cmos_sensor(0x03,0x20);
					HI707_write_cmos_sensor(0x70,0x20);
					
						  break;
					  case AE_EV_COMP_10:
					  case AE_EV_COMP_20:			   /* EV +2 */
						  SENSORDB("[Hi704_Debug]HDR AE_EV_COMP_20 Para:%d;\n",para);
						  HI707_write_cmos_sensor(0x03,0x10);
						  HI707_write_cmos_sensor(0x12,HI707_read_cmos_sensor(0x12)|0x10);
						  HI707_write_cmos_sensor(0x40,0x50);		
					HI707_write_cmos_sensor(0x03,0x20);
					HI707_write_cmos_sensor(0x70,0x70);			  
						  break;
					  case AE_EV_COMP_00: 			 /* EV +2 */
							SENSORDB("[Hi704_Debug]HDR AE_EV_COMP_00 Para:%d;\n",para);
							HI707_write_cmos_sensor(0x03,0x10);
							HI707_write_cmos_sensor(0x12,HI707_read_cmos_sensor(0x12)|0x10);
							//HI707_write_cmos_sensor(0x40,0x80);
							HI707_write_cmos_sensor(0x40,HI707SetBrightness_value);	
							
					HI707_write_cmos_sensor(0x03,0x20);
					HI707_write_cmos_sensor(0x70,HI707_set_param_exposure_value);
						  break;
					  default:
						  return KAL_FALSE;
					}
			   return TRUE;
	}
 
	else{
			    switch (para)
			    {
			    case AE_EV_COMP_20: //+2 EV
					HI707_write_cmos_sensor(0x03,0x20);
					HI707_write_cmos_sensor(0x70,0x70);
					HI707_set_param_exposure_value = 0x70;
			        break;  
			    case AE_EV_COMP_10:  //+1 EV
					HI707_write_cmos_sensor(0x03,0x20);
					HI707_write_cmos_sensor(0x70,0x60);
					HI707_set_param_exposure_value = 0x60;
			        break;    
			    case AE_EV_COMP_00:  //+2 EV
					HI707_write_cmos_sensor(0x03,0x20);
					HI707_write_cmos_sensor(0x70,0x48);
					HI707_set_param_exposure_value = 0x48;
			        break;    
			     case AE_EV_COMP_n10: // -1 EV
					 HI707_write_cmos_sensor(0x03,0x20);
					 HI707_write_cmos_sensor(0x70,0x30);
					 HI707_set_param_exposure_value = 0x30;
			        break;    
			    case AE_EV_COMP_n20:// -2 EV		
					HI707_write_cmos_sensor(0x03,0x20);
					HI707_write_cmos_sensor(0x70,0x20);
					HI707_set_param_exposure_value = 0x20;
			        break;    
			     default:
			        return FALSE;
			    }
			}
    return TRUE;	
} /* HI707_set_param_exposure */

void HI707_set_AE_mode(UINT32 iPara)
{
    UINT8 temp_AE_reg = 0;
    SENSORDB("HI707_set_AE_mode = %d E \n",iPara);
    HI707_write_cmos_sensor(0x03,0x20);
    temp_AE_reg = HI707_read_cmos_sensor(0x10);

    if (AE_MODE_OFF == iPara)
    {
        // turn off AEC/AGC
        HI707_write_cmos_sensor(0x10,temp_AE_reg &~ 0x80);
    }	
    else
    {
        HI707_write_cmos_sensor(0x10,temp_AE_reg | 0x80);
    }
}
UINT32 HI707SetTestPatternMode(kal_bool bEnable)
{
	//	HI257MIPISENSORDB("[OV5645MIPI_OV5645SetTestPatternMode]test pattern bEnable:=%d\n",bEnable);
	if(bEnable)
	{
		
		HI707_write_cmos_sensor(0x03,0x00);
		HI707_write_cmos_sensor(0x50,0x05);
		//run_test_potten=1;
	}
	else
	{      
		HI707_write_cmos_sensor(0x03,0x00);
		HI707_write_cmos_sensor(0x50,0x00);
		//run_test_potten=0;
	}
	return ERROR_NONE;
}


void HI707SetBrightness(UINT16 para)
{
 
 HI707_write_cmos_sensor(0x03,0x10);
 HI707_write_cmos_sensor(0x12,HI707_read_cmos_sensor(0x12)|0x10);
    switch (para)
    {
        case ISP_BRIGHT_LOW: 
			HI707_write_cmos_sensor(0x40,0xd0);
			HI707SetBrightness_value = 0xd0;
             break; 
        case ISP_BRIGHT_HIGH:
			HI707_write_cmos_sensor(0x40,0x50);
			HI707SetBrightness_value = 0x50;
             break; 
        case ISP_BRIGHT_MIDDLE:
			HI707_write_cmos_sensor(0x40,0x80);
			HI707SetBrightness_value = 0x80;
			break; 
        default:
             break; 
    }
    return;
}



void HI707SetContrast(UINT16 para)
{
	HI707_write_cmos_sensor(0x03,0x10);
	HI707_write_cmos_sensor(0x11,HI707_read_cmos_sensor(0x11)|0x40);
 
    switch (para)
    {
        case ISP_CONTRAST_LOW:
			HI707_write_cmos_sensor(0x48,0x60);
             break; 
        case ISP_CONTRAST_HIGH:
			HI707_write_cmos_sensor(0x48,0xa0);
             break; 
        case ISP_CONTRAST_MIDDLE:
			HI707_write_cmos_sensor(0x48,0x80);
        default:
             break; 
    }
    //spin_unlock(&s5k4ecgx_mipi_rw_lock);	
    return;
}



void HI707SetSetIso(UINT16 para)
{
    //spin_lock(&s5k4ecgx_mipi_rw_lock);
    switch (para)
    {
        case AE_ISO_100:
             //ISO 100
            
             break; 
        case AE_ISO_200:
             //ISO 200
           
             break; 
        case AE_ISO_400:
             //ISO 400
         
             break; 
        default:
        case AE_ISO_AUTO:
             //ISO Auto
            
             break; 
    }	
    //spin_unlock(&s5k4ecgx_mipi_rw_lock);	
}


void HI707SetSaturation(UINT16 para)
{
    switch (para)
    {
        case ISP_SAT_HIGH:
			HI707_write_cmos_sensor(0x03,0x10);
			HI707_write_cmos_sensor(0x62,0xa0);
			HI707_write_cmos_sensor(0x63,0xa0);
             break; 
        case ISP_SAT_LOW:
			HI707_write_cmos_sensor(0x03,0x10);
			HI707_write_cmos_sensor(0x62,0x60);
			HI707_write_cmos_sensor(0x63,0x60);
             break; 
        case ISP_SAT_MIDDLE:
			HI707_write_cmos_sensor(0x03,0x10);
			HI707_write_cmos_sensor(0x62,0x80);
			HI707_write_cmos_sensor(0x63,0x80);
        default:
             break; 
    }	
}

UINT32 HI707YUVSensorSetting(FEATURE_ID iCmd, UINT32 iPara)
{
    SENSORDB("[Enter]HI707YUVSensorSetting func:cmd = %d\n",iCmd);

    switch (iCmd) 
    {
    case FID_SCENE_MODE:	    //auto mode or night mode
        HI707_set_scene_mode(iPara);
         break; 	    
    case FID_AWB_MODE:
        HI707_set_param_wb(iPara);
        break;
    case FID_COLOR_EFFECT:
        HI707_set_param_effect(iPara);
        break;
    case FID_AE_EV:	    	    
        HI707_set_param_exposure(iPara);
        break;
    case FID_AE_FLICKER:	    	    	    
        HI707_set_param_banding(iPara);
        break;
    case FID_ZOOM_FACTOR:
        spin_lock(&hi707_yuv_drv_lock);
        HI707_zoom_factor = iPara; 
        spin_unlock(&hi707_yuv_drv_lock);
        break; 
    case FID_AE_SCENE_MODE: 
        HI707_set_AE_mode(iPara);
        break; 
		case FID_ISP_CONTRAST:
			SENSORDB("S5K4ECGX_MIPISensorSetting func:FID_ISP_CONTRAST:%d\n",iPara);
			HI707SetContrast(iPara);
			break;
		case FID_ISP_BRIGHT:
			SENSORDB("S5K4ECGX_MIPISensorSetting func:FID_ISP_BRIGHT:%d\n",iPara);
			HI707SetBrightness(iPara);
			break;
		case FID_ISP_SAT:
			SENSORDB("S5K4ECGX_MIPISensorSetting func:FID_ISP_SAT:%d\n",iPara);
			HI707SetSaturation(iPara);
			break;
		case FID_AE_ISO:
			SENSORDB("S5K4ECGX_MIPISensorSetting func:FID_AE_ISO:%d\n",iPara);
			HI707SetSetIso(iPara);
			break;
    default:
        break;
    }
    return TRUE;
}   /* HI707YUVSensorSetting */

UINT32 HI707YUVSetVideoMode(UINT16 u2FrameRate)
{
    spin_lock(&hi707_yuv_drv_lock);
    HI707_sensor.MPEG4_Video_mode = KAL_TRUE;
    spin_unlock(&hi707_yuv_drv_lock);
    SENSORDB("[Enter]HI707 Set Video Mode:FrameRate= %d\n",u2FrameRate);
    SENSORDB("HI707_sensor.video_mode = %d\n",HI707_sensor.MPEG4_Video_mode);

    if(u2FrameRate >= 30)
		u2FrameRate = 30;
	else if(u2FrameRate>0)
		u2FrameRate = 15;
	else 
    {
        SENSORDB("Wrong Frame Rate"); 
		
		return FALSE;
    }
		
   
    spin_lock(&hi707_yuv_drv_lock);
    HI707_sensor.fix_framerate = u2FrameRate * 10;
    spin_unlock(&hi707_yuv_drv_lock);
   
        HI707_Fix_Video_Frame_Rate(HI707_sensor.fix_framerate); 
 
    
        
    return TRUE;
}

void HI707GetAFMaxNumFocusAreas(UINT32 *pFeatureReturnPara32)
{	
    *pFeatureReturnPara32 = 0;    
    SENSORDB("HI707GetAFMaxNumFocusAreas *pFeatureReturnPara32 = %d\n",  *pFeatureReturnPara32);
}

void HI707GetAEMaxNumMeteringAreas(UINT32 *pFeatureReturnPara32)
{     
    *pFeatureReturnPara32 = 0;    
    SENSORDB("HI707GetAEMaxNumMeteringAreas *pFeatureReturnPara32 = %d\n",  *pFeatureReturnPara32);	
}
void HI707_3ACtrl(ACDK_SENSOR_3A_LOCK_ENUM action)
{

	SENSORDB(" HI707_3ACtrl is %d\n",action);

	switch (action)
	{
	case SENSOR_3A_AE_LOCK:
		HI707_set_AE_mode(KAL_FALSE);
		break;
	case SENSOR_3A_AE_UNLOCK:
		HI707_set_AE_mode(KAL_TRUE);
		break;

	case SENSOR_3A_AWB_LOCK:
		HI707_set_param_wb(AWB_MODE_OFF);
		break;

	case SENSOR_3A_AWB_UNLOCK:
		HI707_set_param_wb(HI707_sensor.wb);
		break;
	default:
		break;
	}
	return;
}

void HI707GetExifInfo(UINT32 exifAddr)
{
    SENSOR_EXIF_INFO_STRUCT* pExifInfo = (SENSOR_EXIF_INFO_STRUCT*)exifAddr;
    pExifInfo->FNumber = 28;
    pExifInfo->AEISOSpeed = AE_ISO_100;
    pExifInfo->AWBMode = HI707_sensor.wb;
    pExifInfo->CapExposureTime = 0;
    pExifInfo->FlashLightTimeus = 0;
    pExifInfo->RealISOValue = AE_ISO_100;
}
void HI707GetDelayInfo(UINT32 delayAddr)
{
	SENSOR_DELAY_INFO_STRUCT* pDelayInfo = (SENSOR_DELAY_INFO_STRUCT*)delayAddr;
	pDelayInfo->InitDelay = 3;
	pDelayInfo->EffectDelay = 5;
	pDelayInfo->AwbDelay = 5;
}
void HI707GetAEAWBLock(UINT32 *pAElockRet32,UINT32 *pAWBlockRet32)
{
	*pAElockRet32 = 1;
	*pAWBlockRet32 = 1;
	SENSORDB("HI707GetAEAWBLock,AE=%d ,AWB=%d\n,",*pAElockRet32,*pAWBlockRet32);
}

UINT32 HI707FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
							 UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
    //UINT16 u2Temp = 0; 
    UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
    UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
    UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
    UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
    MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
    MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;
    SENSORDB("HHL [Enter]:HI707 Feature Control func:FeatureId = %d\n",FeatureId);

    switch (FeatureId)
    {
    case SENSOR_FEATURE_GET_RESOLUTION:
        *pFeatureReturnPara16++=HI707_IMAGE_SENSOR_FULL_WIDTH;
        *pFeatureReturnPara16=HI707_IMAGE_SENSOR_FULL_HEIGHT;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_GET_PERIOD:
        *pFeatureReturnPara16++=HI707_IMAGE_SENSOR_PV_WIDTH;//+HI707_sensor.pv_dummy_pixels;
        *pFeatureReturnPara16=HI707_IMAGE_SENSOR_PV_HEIGHT;//+HI707_sensor.pv_dummy_lines;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
        //*pFeatureReturnPara32 = HI707_sensor_pclk/10;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_SET_ESHUTTER:

        break;
    case SENSOR_FEATURE_SET_NIGHTMODE:
        HI707_night_mode((BOOL) *pFeatureData16);
        break;
    case SENSOR_FEATURE_SET_GAIN:
        break; 
    case SENSOR_FEATURE_SET_FLASHLIGHT:
        break;
    case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
        break;
    case SENSOR_FEATURE_SET_REGISTER:
        HI707_write_cmos_sensor(0x3, pSensorRegData->RegAddr>>8);
        HI707_write_cmos_sensor(pSensorRegData->RegAddr&0xff, pSensorRegData->RegData);
        break;
    case SENSOR_FEATURE_GET_REGISTER:
        HI707_write_cmos_sensor(0x3, pSensorRegData->RegAddr>>8);
        pSensorRegData->RegData = HI707_read_cmos_sensor(pSensorRegData->RegAddr&0xff);
        break;
    case SENSOR_FEATURE_GET_CONFIG_PARA:
        memcpy(pSensorConfigData, &HI707SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
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
        HI707YUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));
        break;	
    case SENSOR_FEATURE_SET_VIDEO_MODE:
        HI707YUVSetVideoMode(*pFeatureData16);
        break; 
    case SENSOR_FEATURE_CHECK_SENSOR_ID:
        HI707_GetSensorID(pFeatureData32); 
        break; 
    case SENSOR_FEATURE_GET_AF_MAX_NUM_FOCUS_AREAS:
        HI707GetAFMaxNumFocusAreas(pFeatureReturnPara32);            
        *pFeatureParaLen=4;
        break;        
    case SENSOR_FEATURE_GET_AE_MAX_NUM_METERING_AREAS:
        HI707GetAEMaxNumMeteringAreas(pFeatureReturnPara32);            
        *pFeatureParaLen=4;
        break;   
    case SENSOR_FEATURE_GET_EXIF_INFO:
        SENSORDB("SENSOR_FEATURE_GET_EXIF_INFO\n");
        SENSORDB("EXIF addr = 0x%x\n",*pFeatureData32);          
        HI707GetExifInfo(*pFeatureData32);
        break;        
	case SENSOR_FEATURE_GET_AE_AWB_LOCK_INFO:
		HI707GetAEAWBLock((*pFeatureData32),*(pFeatureData32+1));
		break;
	case SENSOR_FEATURE_GET_DELAY_INFO:
		SENSORDB("SENSOR_FEATURE_GET_DELAY_INFO\n");
		HI707GetDelayInfo(*pFeatureData32);
		break;
	case SENSOR_FEATURE_SET_YUV_3A_CMD:
		HI707_3ACtrl((ACDK_SENSOR_3A_LOCK_ENUM)*pFeatureData32);
		break;
	case SENSOR_FEATURE_SET_TEST_PATTERN:		   
		HI707SetTestPatternMode((BOOL)*pFeatureData16); 		  
		break;
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
		*pFeatureReturnPara32=HI707_TEST_PATTERN_CHECKSUM;
		*pFeatureParaLen=4;
		break;
		
    default:
        break;			
    }
    return ERROR_NONE;
}	/* HI707FeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncHI707=
{
    HI707Open,
    HI707GetInfo,
    HI707GetResolution,
    HI707FeatureControl,
    HI707Control,
    HI707Close
};

UINT32 HI707_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&SensorFuncHI707;

    return ERROR_NONE;
}	/* SensorInit() */


