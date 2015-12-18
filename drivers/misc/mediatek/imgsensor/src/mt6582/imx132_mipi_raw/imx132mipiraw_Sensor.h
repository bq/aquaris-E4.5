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
#ifndef _IMX132MIPI_SENSOR_H
#define _IMX132MIPI_SENSOR_H

#define IMX132MIPI_DEBUG
#define IMX132MIPI_DRIVER_TRACE
//#define IMX132MIPI_TEST_PATTEM
#ifdef IMX132MIPI_DEBUG
//#define SENSORDB printk
#else
//#define SENSORDB(x,...)
#endif

#define IMX132MIPI_FACTORY_START_ADDR 0
#define IMX132MIPI_ENGINEER_START_ADDR 10

//#define MIPI_INTERFACE

 
typedef enum IMX132MIPI_group_enum
{
  IMX132MIPI_PRE_GAIN = 0,
  IMX132MIPI_CMMCLK_CURRENT,
  IMX132MIPI_FRAME_RATE_LIMITATION,
  IMX132MIPI_REGISTER_EDITOR,
  IMX132MIPI_GROUP_TOTAL_NUMS
} IMX132MIPI_FACTORY_GROUP_ENUM;

typedef enum IMX132MIPI_register_index
{
  IMX132MIPI_SENSOR_BASEGAIN = IMX132MIPI_FACTORY_START_ADDR,
  IMX132MIPI_PRE_GAIN_R_INDEX,
  IMX132MIPI_PRE_GAIN_Gr_INDEX,
  IMX132MIPI_PRE_GAIN_Gb_INDEX,
  IMX132MIPI_PRE_GAIN_B_INDEX,
  IMX132MIPI_FACTORY_END_ADDR
} IMX132MIPI_FACTORY_REGISTER_INDEX;

typedef enum IMX132MIPI_engineer_index
{
  IMX132MIPI_CMMCLK_CURRENT_INDEX = IMX132MIPI_ENGINEER_START_ADDR,
  IMX132MIPI_ENGINEER_END
} IMX132MIPI_FACTORY_ENGINEER_INDEX;

typedef struct _sensor_data_struct
{
  SENSOR_REG_STRUCT reg[IMX132MIPI_ENGINEER_END];
  SENSOR_REG_STRUCT cct[IMX132MIPI_FACTORY_END_ADDR];
} sensor_data_struct;
typedef enum {
    IMX132MIPI_SENSOR_MODE_INIT = 0,
    IMX132MIPI_SENSOR_MODE_PREVIEW,
    IMX132MIPI_SENSOR_MODE_VIDEO,
    IMX132MIPI_SENSOR_MODE_CAPTURE
} IMX132MIPI_SENSOR_MODE;


/* SENSOR PREVIEW/CAPTURE VT CLOCK */
//#define IMX132MIPI_PREVIEW_CLK                     69333333  //48100000
//#define IMX132MIPI_CAPTURE_CLK                     69333333  //48100000

#define IMX132MIPI_COLOR_FORMAT                    SENSOR_OUTPUT_FORMAT_RAW_R

#define IMX132MIPI_MIN_ANALOG_GAIN				1	/* 1x */
#define IMX132MIPI_MAX_ANALOG_GAIN				8	/* 32x */


/* FRAME RATE UNIT */
#define IMX132MIPI_FPS(x)                          (10 * (x))

/* SENSOR PIXEL/LINE NUMBERS IN ONE PERIOD */
//#define IMX132MIPI_FULL_PERIOD_PIXEL_NUMS          2700 /* 9 fps */

//#define IMX132MIPI_DEBUG_SETTING
//#define IMX132_BINNING_SUM//binning: enable  for sum, disable for vertical averag	
//#define IMX132MIPI_4LANE

#if 1
#define IMX132MIPI_FULL_PERIOD_PIXEL_NUMS          2248  //3055 /* 8 fps */
#define IMX132MIPI_FULL_PERIOD_LINE_NUMS           1192//30fps
//#define IMX132MIPI_FULL_PERIOD_LINE_NUMS           1560//30.8fps
#define IMX132MIPI_PV_PERIOD_PIXEL_NUMS            2248 //1630 /* 30 fps */
#define IMX132MIPI_PV_PERIOD_LINE_NUMS             1192//1260 //984
#define IMX132MIPI_VIDEO_PERIOD_PIXEL_NUMS         2248 //1630 /* 30 fps */
#define IMX132MIPI_VIDEO_PERIOD_LINE_NUMS          1192//1260 //984
#else


#define IMX132MIPI_FULL_PERIOD_PIXEL_NUMS          2248  //3055 /* 8 fps */
#define IMX132MIPI_FULL_PERIOD_LINE_NUMS           1124//2642//2586 //2484   //1968
#define IMX132MIPI_PV_PERIOD_PIXEL_NUMS            2248 //1630 /* 30 fps */
#define IMX132MIPI_PV_PERIOD_LINE_NUMS             1124//1260 //984
#define IMX132MIPI_VIDEO_PERIOD_PIXEL_NUMS         2248 //1630 /* 30 fps */
#define IMX132MIPI_VIDEO_PERIOD_LINE_NUMS          1124//1260 //984
#endif


/* SENSOR START/END POSITION */
#if 0
#define IMX132MIPI_FULL_X_START                    2
#define IMX132MIPI_FULL_Y_START                    2
#define IMX132MIPI_IMAGE_SENSOR_FULL_WIDTH         (1936 - 16) /* 2560 */
#define IMX132MIPI_IMAGE_SENSOR_FULL_HEIGHT        (1090 - 10) /* 1920 */
#define IMX132MIPI_PV_X_START                      2
#define IMX132MIPI_PV_Y_START                      2
#define IMX132MIPI_IMAGE_SENSOR_PV_WIDTH           (1936 - 16)  //    (1280 - 16) /* 1264 */
#define IMX132MIPI_IMAGE_SENSOR_PV_HEIGHT          (1090 - 10)  //(960 - 12) /* 948 */

#define IMX132MIPI_VIDEO_X_START                   2
#define IMX132MIPI_VIDEO_Y_START                   2
#define IMX132MIPI_IMAGE_SENSOR_VIDEO_WIDTH        (1936 - 16)//(3264 - 64) /* 2560 */
#define IMX132MIPI_IMAGE_SENSOR_VIDEO_HEIGHT       (1090 - 10)//(2448 - 48) /* 1920 */
#else

#define IMX132MIPI_FULL_X_START                    2
#define IMX132MIPI_FULL_Y_START                    2
#define IMX132MIPI_IMAGE_SENSOR_FULL_WIDTH         (1948 - 28) /* 2560 */
#define IMX132MIPI_IMAGE_SENSOR_FULL_HEIGHT        (1108 - 12) /* 1920 */
#define IMX132MIPI_PV_X_START                      2
#define IMX132MIPI_PV_Y_START                      2
#define IMX132MIPI_IMAGE_SENSOR_PV_WIDTH           (1948 - 28)  //    (1280 - 16) /* 1264 */
#define IMX132MIPI_IMAGE_SENSOR_PV_HEIGHT          (1108 - 12)  //(960 - 12) /* 948 */

#define IMX132MIPI_VIDEO_X_START                   2
#define IMX132MIPI_VIDEO_Y_START                   2
#define IMX132MIPI_IMAGE_SENSOR_VIDEO_WIDTH        (1948 - 28)//(3264 - 64) /* 2560 */
#define IMX132MIPI_IMAGE_SENSOR_VIDEO_HEIGHT       (1108 - 12)//(2448 - 48) /* 1920 */

#endif

/* SENSOR READ/WRITE ID */
#define IMX132MIPI_SLAVE_WRITE_ID_1   (0x6c)
#define IMX132MIPI_SLAVE_WRITE_ID_2   (0x20)

#define IMX132MIPI_WRITE_ID   (0x20)  //(0x6c)
#define IMX132MIPI_READ_ID    (0x21)  //(0x6d)

/* SENSOR ID */
//#define IMX132MIPI_SENSOR_ID						(0x5647)


//added by mandrave
//#define IMX132MIPI_USE_OTP

#if defined(IMX132MIPI_USE_OTP)

struct imx132mipi_otp_struct
{
    kal_uint16 customer_id;
	kal_uint16 module_integrator_id;
	kal_uint16 lens_id;
	kal_uint16 rg_ratio;
	kal_uint16 bg_ratio;
	kal_uint16 user_data[5];
	kal_uint16 lenc[63];

};

#define RG_TYPICAL 0x51
#define BG_TYPICAL 0x57


#endif




/* SENSOR PRIVATE STRUCT */
typedef struct IMX132MIPI_sensor_STRUCT
{
  MSDK_SENSOR_CONFIG_STRUCT cfg_data;
  sensor_data_struct eng; /* engineer mode */
  MSDK_SENSOR_ENG_INFO_STRUCT eng_info;
  kal_uint8 mirror;
  kal_bool pv_mode;
  kal_bool video_mode;
  kal_bool is_zsd;
  kal_bool is_zsd_cap;
  kal_bool is_autofliker;
  kal_uint16 normal_fps; /* video normal mode max fps */
  kal_uint16 night_fps; /* video night mode max fps */
  kal_uint16 shutter;
  kal_uint16 gain;
  kal_uint32 pv_pclk;
  kal_uint32 cap_pclk;
  kal_uint32 video_pclk;
  kal_uint32 pclk;
  kal_uint16 frame_length;
  kal_uint16 line_length;  
  kal_uint16 write_id;
  kal_uint16 read_id;
  kal_uint16 dummy_pixels;
  kal_uint16 dummy_lines;
  kal_uint32 max_exposure_lines;
  
  IMX132MIPI_SENSOR_MODE sensorMode;
} IMX132MIPI_sensor_struct;

//export functions
UINT32 IMX132MIPIOpen(void);
UINT32 IMX132MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 IMX132MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
UINT32 IMX132MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 IMX132MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 IMX132MIPIClose(void);

#define Sleep(ms) mdelay(ms)

#endif 
