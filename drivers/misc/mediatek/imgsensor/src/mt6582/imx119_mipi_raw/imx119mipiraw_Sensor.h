/*****************************************************************************
 *
 * Filename:
 * ---------
 *   imx111mipiraw_sensor.h
 *
 * Project:
 * --------
 *   YUSU
 *
 * Description:
 * ------------
 *   Header file of Sensor driver
 *
 *
 * Author:

 *============================================================================
 ****************************************************************************/
/* SENSOR FULL SIZE */
#ifndef __SENSOR_H
#define __SENSOR_H

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


// Important Note:
//     1. Make sure horizontal PV sensor output is larger than IMX111MIPI_REAL_PV_WIDTH  + 2 * IMX111MIPI_IMAGE_SENSOR_PV_STARTX + 4.
//     2. Make sure vertical   PV sensor output is larger than IMX111MIPI_REAL_PV_HEIGHT + 2 * IMX111MIPI_IMAGE_SENSOR_PV_STARTY + 6.
//     3. Make sure horizontal CAP sensor output is larger than IMX111MIPI_REAL_CAP_WIDTH  + 2 * IMX111MIPI_IMAGE_SENSOR_CAP_STARTX + IMAGE_SENSOR_H_DECIMATION*4.
//     4. Make sure vertical   CAP sensor output is larger than IMX111MIPI_REAL_CAP_HEIGHT + 2 * IMX111MIPI_IMAGE_SENSOR_CAP_STARTY + IMAGE_SENSOR_V_DECIMATION*6.
// Note:
//     1. The reason why we choose REAL_PV_WIDTH/HEIGHT as tuning starting point is
//        that if we choose REAL_CAP_WIDTH/HEIGHT as starting point, then:
//            REAL_PV_WIDTH  = REAL_CAP_WIDTH  / IMAGE_SENSOR_H_DECIMATION
//            REAL_PV_HEIGHT = REAL_CAP_HEIGHT / IMAGE_SENSOR_V_DECIMATION
//        There might be some truncation error when dividing, which may cause a little view angle difference.
//Macro for Resolution
#define IMAGE_SENSOR_H_DECIMATION				2	// For current PV mode, take 1 line for every 2 lines in horizontal direction.
#define IMAGE_SENSOR_V_DECIMATION				2	// For current PV mode, take 1 line for every 2 lines in vertical direction.

/* Real PV Size, i.e. the size after all ISP processing (so already -4/-6), before MDP. */
#define IMX119MIPI_REAL_PV_WIDTH				1280
#define IMX119MIPI_REAL_PV_HEIGHT				960

/* Real CAP Size, i.e. the size after all ISP processing (so already -4/-6), before MDP. */
#define IMX119MIPI_REAL_CAP_WIDTH				1280
#define IMX119MIPI_REAL_CAP_HEIGHT				960
#define IMX119MIPI_REAL_VIDEO_WIDTH				1280
#define IMX119MIPI_REAL_VIDEO_HEIGHT				960

/* X/Y Starting point */
#define IMX119MIPI_IMAGE_SENSOR_PV_STARTX       0
#define IMX119MIPI_IMAGE_SENSOR_PV_STARTY       0	// The value must bigger or equal than 1.
#define IMX119MIPI_IMAGE_SENSOR_CAP_STARTX		0
#define IMX119MIPI_IMAGE_SENSOR_CAP_STARTY		0// The value must bigger or equal than 1.

#define MAX_FRAME_RATE	(30)
#define MIN_FRAME_RATE  (10)

#define IMX119MIPI_IMAGE_SENSOR_CCT_WIDTH		(1296)
#define IMX119MIPI_IMAGE_SENSOR_CCT_HEIGHT		(1040)

#define IMX119MIPI_PV_LINE_LENGTH_PIXELS 						(1392)
#define IMX119MIPI_PV_FRAME_LENGTH_LINES						(1560)	
#define IMX119MIPI_FULL_LINE_LENGTH_PIXELS 						(1392)
#define IMX119MIPI_FULL_FRAME_LENGTH_LINES			            (1560)
#define IMX119MIPI_VIDEO_LINE_LENGTH_PIXELS 					(1392)
#define IMX119MIPI_VIDEO_FRAME_LENGTH_LINES			            (1560)


#define IMX119MIPI_SHUTTER_LINES_GAP	  3

//#define IMX119MIPI_WRITE_ID (0x34)
//#define IMX119MIPI_READ_ID	(0x35)


#define IMX119MIPI_WRITE_ID (0x6E)
#define IMX119MIPI_READ_ID	(0x6F)


/* SENSOR PRIVATE STRUCT */
struct IMX119_SENSOR_STRUCT
{
	kal_uint8 i2c_write_id;
	kal_uint8 i2c_read_id;

};
struct IMX119MIPI_sensor_STRUCT
	{	 
		  kal_uint16 i2c_write_id;
		  kal_uint16 i2c_read_id;
		  kal_bool first_init;
		  kal_bool fix_video_fps;
		  kal_bool pv_mode; 				//True: Preview Mode; False: Capture Mode
		  kal_bool night_mode;				//True: Night Mode; False: Auto Mode
		  kal_uint8 mirror_flip;
		  kal_uint32 pv_pclk;				//Preview Pclk
		  kal_uint32 cp_pclk;				//Capture Pclk
		  kal_uint32 pv_shutter;		   
		  kal_uint32 cp_shutter;
		  kal_uint32 pv_gain;
		  kal_uint32 cp_gain;
		  kal_uint32 pv_line_length;
		  kal_uint32 pv_frame_length;
		  kal_uint32 cp_line_length;
		  kal_uint32 cp_frame_length;
		  kal_uint16 pv_dummy_pixels;		   //Dummy Pixels:must be 12s
		  kal_uint16 pv_dummy_lines;		   //Dummy Lines
		  kal_uint16 cp_dummy_pixels;		   //Dummy Pixels:must be 12s
		  kal_uint16 cp_dummy_lines;		   //Dummy Lines			
		  kal_uint16 video_current_frame_rate;
	};

#define IMX119MIPI_SENSOR_ID            IMX119_SENSOR_ID

#define IMX119MIPI_PAGE_SETTING_REG    (0xFF)

UINT32 IMX119MIPIOpen(void);
UINT32 IMX119MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 IMX119MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 IMX119MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 IMX119MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
UINT32 IMX119MIPIClose(void);
#endif /* __SENSOR_H */

