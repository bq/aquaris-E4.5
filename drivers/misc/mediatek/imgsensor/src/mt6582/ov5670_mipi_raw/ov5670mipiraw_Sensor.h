/*******************************************************************************************/
  
  
/*******************************************************************************************/

/* SENSOR FULL SIZE */
#ifndef __OV5670_SENSOR_H
#define __OV5670_SENSOR_H   

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
} OV5670_SENSOR_MODE;


typedef struct
{
	kal_uint32 DummyPixels;
	kal_uint32 DummyLines;
	
	kal_uint32 pvShutter;
	kal_uint32 pvGain;
	
	kal_uint32 pvPclk;  
	kal_uint32 videoPclk;
	kal_uint32 capPclk;
	
	kal_uint32 shutter;

	kal_uint16 sensorGlobalGain;
	kal_uint16 ispBaseGain;
	kal_uint16 realGain;

	kal_int16 imgMirror;

	OV5670_SENSOR_MODE sensorMode;

	kal_bool OV5670AutoFlickerMode;
	kal_bool OV5670VideoMode;
	
}OV5670_PARA_STRUCT,*POV5670_PARA_STRUCT;


    #define OV5670_SHUTTER_MARGIN 			(4)
	#define OV5670_GAIN_BASE				(128)
	#define OV5670_AUTOFLICKER_OFFSET_30 	(296)
	#define OV5670_AUTOFLICKER_OFFSET_25 	(250)
	#define OV5670_AUTOFLICKER_OFFSET_15 	(146)
	#define OV5670_PREVIEW_PCLK 			(102850000)
	#define OV5670_VIDEO_PCLK 				(OV5670_PREVIEW_PCLK)
	#define OV5670_CAPTURE_PCLK 			(102850000)
	
	#define OV5670_MAX_FPS_PREVIEW			(300)
	#define OV5670_MAX_FPS_VIDEO			(300)
	#define OV5670_MAX_FPS_CAPTURE			(250)
	//#define OV5670_MAX_FPS_N3D				(300)


	//grab window
	#define OV5670_IMAGE_SENSOR_PV_WIDTH					(1296)
	#define OV5670_IMAGE_SENSOR_PV_HEIGHT					(972) //(960)
	#define OV5670_IMAGE_SENSOR_VIDEO_WIDTH 				(OV5670_IMAGE_SENSOR_PV_WIDTH)
	#define OV5670_IMAGE_SENSOR_VIDEO_HEIGHT				(OV5670_IMAGE_SENSOR_PV_HEIGHT)
	#define OV5670_IMAGE_SENSOR_FULL_WIDTH					(2592)	
	#define OV5670_IMAGE_SENSOR_FULL_HEIGHT 				(1944)

	#define OV5670_FULL_X_START						    		(0)
	#define OV5670_FULL_Y_START						    		(0)
	#define OV5670_PV_X_START						    		(0)
	#define OV5670_PV_Y_START						    		(0)
	#define OV5670_VIDEO_X_START								(0)
	#define OV5670_VIDEO_Y_START								(0)

	#define OV5670_MAX_ANALOG_GAIN					(8)
	#define OV5670_MIN_ANALOG_GAIN					(1)


	/* SENSOR PIXEL/LINE NUMBERS IN ONE PERIOD */
	#define OV5670_PV_PERIOD_PIXEL_NUMS 				0x068C	//1676*2=>3352
	#define OV5670_PV_PERIOD_LINE_NUMS					0x07FD	//2045

	#define OV5670_VIDEO_PERIOD_PIXEL_NUMS				OV5670_PV_PERIOD_PIXEL_NUMS
	#define OV5670_VIDEO_PERIOD_LINE_NUMS				OV5670_PV_PERIOD_LINE_NUMS

	#define OV5670_FULL_PERIOD_PIXEL_NUMS					0x07DC	//2012*2   25fps
	#define OV5670_FULL_PERIOD_LINE_NUMS					0x07FD	//2045
	
	#define OV5670MIPI_WRITE_ID 	(0x6C)
	#define OV5670MIPI_READ_ID	(0x6D)

	#define OV5670MIPI_SENSOR_ID            OV5670_SENSOR_ID


	UINT32 OV5670MIPIOpen(void);
	UINT32 OV5670MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
	UINT32 OV5670MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
	UINT32 OV5670MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
	UINT32 OV5670MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
	UINT32 OV5670MIPIClose(void);

#endif 

