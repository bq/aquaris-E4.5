/*****************************************************************************
 *
 * Filename:
 * ---------
 *   sensor.h
 *
 * Project:
 * --------
 *   DUMA
 *
 * Description:
 * ------------
 *   CMOS sensor header file
 *
 ****************************************************************************/
#ifndef _HI191MIPIRAW_SENSOR_H
#define _HI191MIPIRAW_SENSOR_H
	 
	 //#define HI191MIPI_DEBUG
	 //#define HI191MIPI_DRIVER_TRACE
	 //#define HI191MIPI_TEST_PATTEM
#ifdef HI191MIPI_DEBUG
	 //#define SENSORDB printk
#else
	 //#define SENSORDB(x,...)
#endif
	  
/* SENSOR PREVIEW/CAPTURE VT CLOCK */
#define HI191MIPI_PREVIEW_CLK                     36000000   //42000000
#define HI191MIPI_CAPTURE_CLK                     36000000   //42000000
	 
#define HI191MIPI_COLOR_FORMAT                    SENSOR_OUTPUT_FORMAT_RAW_Gr
	  	 
	 /* FRAME RATE UNIT */
#define HI191MIPI_FPS(x)                          (10 * (x))
	 
	 /* SENSOR PIXEL/LINE NUMBERS IN ONE PERIOD */
	 //#define HI191MIPI_FULL_PERIOD_PIXEL_NUMS		  2700 /* 9 fps */
#define HI191MIPI_FULL_PERIOD_PIXEL_NUMS          1280 /* 8 fps */
#define HI191MIPI_FULL_PERIOD_LINE_NUMS           720
#define HI191MIPI_PV_PERIOD_PIXEL_NUMS            1280 /* 30 fps */
#define HI191MIPI_PV_PERIOD_LINE_NUMS             720
	 
	 /* SENSOR START/END POSITION */
#define HI191MIPI_FULL_X_START                    1
#define HI191MIPI_FULL_Y_START                    1
#define HI191MIPI_IMAGE_SENSOR_FULL_WIDTH         (1280)
#define HI191MIPI_IMAGE_SENSOR_FULL_HEIGHT        (720)
#define HI191MIPI_PV_X_START                      1
#define HI191MIPI_PV_Y_START                      1
#define HI191MIPI_IMAGE_SENSOR_PV_WIDTH           (1280)
#define HI191MIPI_IMAGE_SENSOR_PV_HEIGHT          (720)
	 
#define HI191MIPI_DEFAULT_DUMMY_PIXELS			(276)
#define HI191MIPI_DEFAULT_DUMMY_LINES			(42)
	 
	 /* SENSOR READ/WRITE ID */
#define HI191MIPI_WRITE_ID (0x42)
#define HI191MIPI_READ_ID  (0x43)

	 
	 
	 /* SENSOR ID */
	 //#define HI191MIPI_SENSOR_ID 					 (0x2720)
	 
	 /* SENSOR PRIVATE STRUCT */
	 typedef struct HI191MIPI_sensor_STRUCT
	 {
	   kal_uint8 mirror;
	   kal_bool video_mode;
	   kal_bool pv_mode;
	   kal_uint16 normal_fps; /* video normal mode max fps */
	   kal_uint16 night_fps; /* video night mode max fps */
	   kal_uint16 shutter;
	   kal_uint16 gain;
	   kal_uint32 pclk;
	   kal_uint16 frame_height;
	   kal_uint16 default_height;
	   kal_uint16 line_length;	
	 } HI191MIPI_sensor_struct;
	 
	 //export functions
	 UINT32 HI191MIPIOpen(void);
	 UINT32 HI191MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
	 UINT32 HI191MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
	 UINT32 HI191MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
	 UINT32 HI191MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
	 UINT32 HI191MIPIClose(void);
	 
#define Sleep(ms) mdelay(ms)
	 
#endif 

