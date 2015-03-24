/*******************************************************************************************/


/*******************************************************************************************/

/* SENSOR FULL SIZE */
#ifndef __SENSOR_H
#define __SENSOR_H

//#define ZSD15FPS
//#define OV13850_CAP_24FPS

typedef enum group_enum {
    PRE_GAIN=0,
    CMMCLK_CURRENT,
    FRAME_RATE_LIMITATION,
    REGISTER_EDITOR,
    GROUP_TOTAL_NUMS
} FACTORY_GROUP_ENUM;


#define ENGINEER_START_ADDR 10
#define FACTORY_START_ADDR 0

typedef enum engineer_index
{
    CMMCLK_CURRENT_INDEX=ENGINEER_START_ADDR,
    ENGINEER_END
} FACTORY_ENGINEER_INDEX;

typedef enum register_index
{
	SENSOR_BASEGAIN=FACTORY_START_ADDR,
	PRE_GAIN_R_INDEX,
	PRE_GAIN_Gr_INDEX,
	PRE_GAIN_Gb_INDEX,
	PRE_GAIN_B_INDEX,
	FACTORY_END_ADDR
} FACTORY_REGISTER_INDEX;

typedef struct
{
    SENSOR_REG_STRUCT	Reg[ENGINEER_END];
    SENSOR_REG_STRUCT	CCT[FACTORY_END_ADDR];
} SENSOR_DATA_STRUCT, *PSENSOR_DATA_STRUCT;

typedef enum {
    SENSOR_MODE_INIT = 0,
    SENSOR_MODE_PREVIEW,
    SENSOR_MODE_VIDEO,
    SENSOR_MODE_CAPTURE
} OV13850_SENSOR_MODE;


typedef struct
{
	kal_uint32 DummyPixels;
	kal_uint32 DummyLines;
	
	kal_uint32 pvShutter;
	kal_uint32 pvGain;
	
	kal_uint32 pvPclk;  // x10 480 for 48MHZ
	kal_uint32 videoPclk;
	kal_uint32 capPclk; // x10
	
	kal_uint32 shutter;
	kal_uint32 maxExposureLines;

	kal_uint16 sensorGlobalGain;//sensor gain read from 0x350a 0x350b;
	kal_uint16 ispBaseGain;//64
	kal_uint16 realGain;//ispBaseGain as 1x

	kal_int16 imgMirror;

	OV13850_SENSOR_MODE sensorMode;

	kal_bool OV13850AutoFlickerMode;
	kal_bool OV13850VideoMode;
	
}OV13850_PARA_STRUCT,*POV13850_PARA_STRUCT;

//2112 1568
//	@@ RES_4208x3120 15fps

	#define OV13850_IMAGE_SENSOR_FULL_WIDTH					4224-16 //(4200-8)	
	#define OV13850_IMAGE_SENSOR_FULL_HEIGHT				3136-16 //(3100-6)

	/* SENSOR PV SIZE */
	#define OV13850_IMAGE_SENSOR_PV_WIDTH					2104-8//(2100-4)
	#define OV13850_IMAGE_SENSOR_PV_HEIGHT					1560-8//(1560-4)

	#define OV13850_IMAGE_SENSOR_VIDEO_WIDTH				2104-8//	(2100-4) //(2088-16) //(2100-4)
	#define OV13850_IMAGE_SENSOR_VIDEO_HEIGHT				1560-8//(1560-4) //(1568-12) //(1580-4) //(1560-4)
	

	/* SENSOR SCALER FACTOR */
	#define OV13850_PV_SCALER_FACTOR					    	3
	#define OV13850_FULL_SCALER_FACTOR					    1
	                                        	
	/* SENSOR START/EDE POSITION */         	
	#define OV13850_FULL_X_START						    		(8)
	#define OV13850_FULL_Y_START						    		(8)

	#define OV13850_FULL_X_END						        	(4224)
	#define OV13850_FULL_Y_END						        	(3136)
	#define OV13850_PV_X_START						    		(8)//(7)//(6)
	#define OV13850_PV_Y_START						    		(8)//(7)//(6)
	#define OV13850_PV_X_END						    			(2000) 
	#define OV13850_PV_Y_END						    			(1500) 
	
	#define OV13850_VIDEO_X_START								(8)//(2)
	#define OV13850_VIDEO_Y_START								(8)//(2)
	#define OV13850_VIDEO_X_END 									(2000)
	#define OV13850_VIDEO_Y_END 									(1500)

	#define OV13850_MAX_ANALOG_GAIN					(16)
	#define OV13850_MIN_ANALOG_GAIN					(1)
	#define OV13850_ANALOG_GAIN_1X						(0x0020)

	//OV13850_CAP_18FPS
	#define OV13850MIPI_CAPTURE_CLK 	(240000000)

	#define OV13850MIPI_PREVIEW_CLK 	(480000000)
	#define OV13850MIPI_VIDEO_CLK		(480000000)


	/* SENSOR PIXEL/LINE NUMBERS IN ONE PERIOD */
	
	#define OV13850_FULL_PERIOD_PIXEL_NUMS					(4800)
	#define OV13850_FULL_PERIOD_LINE_NUMS					(3328)


	#define OV13850_PV_PERIOD_PIXEL_NUMS					(4800)
	#define OV13850_PV_PERIOD_LINE_NUMS						3328//(2664) //(1664):pclk 240M; (2664) for pclk 384;(3328) for pclk 480	 

	#define OV13850_VIDEO_PERIOD_PIXEL_NUMS 				(4800)  
	#define OV13850_VIDEO_PERIOD_LINE_NUMS					3328//(2664) //(1664):pclk 240M; (2664) for pclk 384;(3328) for pclk 480	 
	

	#define OV13850_MIN_LINE_LENGTH						0x1100     
	#define OV13850_MIN_FRAME_LENGTH						0xc00	  
	
	#define OV13850_MAX_LINE_LENGTH						0xCCCC
	#define OV13850_MAX_FRAME_LENGTH						0xFFFF

	/* DUMMY NEEDS TO BE INSERTED */
	/* SETUP TIME NEED TO BE INSERTED */
	#define OV13850_IMAGE_SENSOR_PV_INSERTED_PIXELS			2
	#define OV13850_IMAGE_SENSOR_PV_INSERTED_LINES			2

	#define OV13850_IMAGE_SENSOR_FULL_INSERTED_PIXELS		4
	#define OV13850_IMAGE_SENSOR_FULL_INSERTED_LINES		4

 //H/W SID is low.
	#define OV13850MIPI_WRITE_ID 	(0x20)
	#define OV13850MIPI_READ_ID	  (0x21)
	
 //H/W SID is high.
	#define OV13850MIPI_WRITE_ID_1 	(0x6C)
	#define OV13850MIPI_READ_ID_1	(0x6D)

// SENSOR CHIP VERSION

#define OV13850MIPI_SENSOR_ID            OV13850_SENSOR_ID

#define OV13850MIPI_PAGE_SETTING_REG    (0xFF)

//s_add for porting
//s_add for porting
//s_add for porting

//export functions
UINT32 OV13850MIPIOpen(void);
UINT32 OV13850MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 OV13850MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 OV13850MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 OV13850MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
UINT32 OV13850MIPIClose(void);

//#define Sleep(ms) mdelay(ms)
//#define RETAILMSG(x,...)
//#define TEXT

//e_add for porting
//e_add for porting
//e_add for porting

#endif /* __SENSOR_H */

