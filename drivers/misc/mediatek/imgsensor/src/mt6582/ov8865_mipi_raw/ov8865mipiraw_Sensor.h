/*******************************************************************************************/
  
  
/*******************************************************************************************/

/* SENSOR FULL SIZE */
#ifndef __OV8865_SENSOR_H
#define __OV8865_SENSOR_H   

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
} OV8865_SENSOR_MODE;


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

	kal_uint16 sensorGlobalGain;//sensor gain read from 0x350a 0x350b;
	kal_uint16 ispBaseGain;//64
	kal_uint16 realGain;//ispBaseGain as 1x

	kal_int16 imgMirror;

	OV8865_SENSOR_MODE sensorMode;

	kal_bool OV8865AutoFlickerMode;
	kal_bool OV8865VideoMode;
	
}OV8865_PARA_STRUCT,*POV8865_PARA_STRUCT;


    #define OV8865_SHUTTER_MARGIN 			(4)
	#define OV8865_GAIN_BASE				(128)
	#define OV8865_AUTOFLICKER_OFFSET_30 	(296)
	#define OV8865_AUTOFLICKER_OFFSET_15 	(146)
	#define OV8865_PREVIEW_PCLK 			(74400000)
	#define OV8865_VIDEO_PCLK 				(148800000)
	#define OV8865_CAPTURE_PCLK 			(148800000)
	
	#define OV8865_MAX_FPS_PREVIEW			(300)
	#define OV8865_MAX_FPS_VIDEO			(300)
	#define OV8865_MAX_FPS_CAPTURE			(300)
	//#define OV8865_MAX_FPS_N3D				(300)


	//grab window
	#define OV8865_IMAGE_SENSOR_PV_WIDTH					(1632)
	#define OV8865_IMAGE_SENSOR_PV_HEIGHT					(1224)
	#define OV8865_IMAGE_SENSOR_VIDEO_WIDTH 				(3264)
	#define OV8865_IMAGE_SENSOR_VIDEO_HEIGHT				(1836)
	#define OV8865_IMAGE_SENSOR_FULL_WIDTH					(3264)	
	#define OV8865_IMAGE_SENSOR_FULL_HEIGHT 				(2448)

	#define OV8865_FULL_X_START						    		(2)
	#define OV8865_FULL_Y_START						    		(0)
	#define OV8865_PV_X_START						    		(2)
	#define OV8865_PV_Y_START						    		(0)
	#define OV8865_VIDEO_X_START								(2)
	#define OV8865_VIDEO_Y_START								(0)

	#define OV8865_MAX_ANALOG_GAIN					(8)
	#define OV8865_MIN_ANALOG_GAIN					(1)


	/* SENSOR PIXEL/LINE NUMBERS IN ONE PERIOD */
	#define OV8865_PV_PERIOD_PIXEL_NUMS 				0x0783	//1923*2=>3846
	#define OV8865_PV_PERIOD_LINE_NUMS					0x04E0	//1248

	#define OV8865_VIDEO_PERIOD_PIXEL_NUMS				0x0A16	//2582*2=>5164
	#define OV8865_VIDEO_PERIOD_LINE_NUMS				0x0742	//1858
	
	#define OV8865_FULL_PERIOD_PIXEL_NUMS					0x07d8	//2008*2   30fps
	#define OV8865_FULL_PERIOD_LINE_NUMS					0x09d8	//2470
	
	#define OV8865MIPI_WRITE_ID 	(0x20)
	#define OV8865MIPI_READ_ID	(0x21)

	#define OV8865MIPI_SENSOR_ID            OV8865_SENSOR_ID


	UINT32 OV8865MIPIOpen(void);
	UINT32 OV8865MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
	UINT32 OV8865MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
	UINT32 OV8865MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
	UINT32 OV8865MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
	UINT32 OV8865MIPIClose(void);

#endif 

