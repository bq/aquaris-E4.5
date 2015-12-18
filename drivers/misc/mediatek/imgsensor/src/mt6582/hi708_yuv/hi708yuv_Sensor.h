/*
 * (c) MediaTek Inc. 2010
 */

/* SENSOR FULL SIZE */
   #ifndef __SENSOR_H
 #define __SENSOR_H

	//follow is define by jun
	/* SENSOR READ/WRITE ID */

#define HI708_IMAGE_SENSOR_QVGA_WIDTH       (320)
#define HI708_IMAGE_SENSOR_QVGA_HEIGHT      (240)
#define HI708_IMAGE_SENSOR_VGA_WIDTH        (640)
#define HI708_IMAGE_SENSOR_VGA_HEIGHT       (480)
#define HI708_IMAGE_SENSOR_SXGA_WIDTH       (1280)
#define HI708_IMAGE_SENSOR_SXGA_HEIGHT      (1024)

#define HI708_IMAGE_SENSOR_FULL_WIDTH	   HI708_IMAGE_SENSOR_VGA_WIDTH-12 
#define HI708_IMAGE_SENSOR_FULL_HEIGHT	   HI708_IMAGE_SENSOR_VGA_HEIGHT-8    

#define HI708_IMAGE_SENSOR_PV_WIDTH   HI708_IMAGE_SENSOR_VGA_WIDTH-12
#define HI708_IMAGE_SENSOR_PV_HEIGHT  HI708_IMAGE_SENSOR_VGA_HEIGHT-8

//SENSOR PIXEL/LINE NUMBERS IN ONE PERIOD
#define HI708_VGA_DEFAULT_PIXEL_NUMS		   (656)	
#define HI708_VGA_DEFAULT_LINE_NUMS 		   (500)

#define HI708_QVGA_DEFAULT_PIXEL_NUMS		   (656)	 
#define HI708_QVGA_DEFAULT_LINE_NUMS		   (254)

/* MAX/MIN FRAME RATE (FRAMES PER SEC.) */
#define HI708_MIN_FRAMERATE_5					(50)
#define HI708_MIN_FRAMERATE_7_5 				(75)
#define HI708_MIN_FRAMERATE_10					(100)
#define HI708_MIN_FRAMERATE_15                  (150)

//Video Fixed Framerate
#define HI708_VIDEO_FIX_FRAMERATE_5 			(50)
#define HI708_VIDEO_FIX_FRAMERATE_7_5			(75)
#define HI708_VIDEO_FIX_FRAMERATE_10			(100)
#define HI708_VIDEO_FIX_FRAMERATE_15			(150)
#define HI708_VIDEO_FIX_FRAMERATE_20			(200)
#define HI708_VIDEO_FIX_FRAMERATE_25			(250)
#define HI708_VIDEO_FIX_FRAMERATE_30			(300)


#define HI708_WRITE_ID		0x60
#define HI708_READ_ID		0x61

	//#define HI708_SCCB_SLAVE_ADDR 0x60

typedef struct _SENSOR_INIT_INFO
{
	  kal_uint8 address;
	  kal_uint8 data;
}HI708_SENSOR_INIT_INFO;
typedef enum __VIDEO_MODE__
{
	  HI708_VIDEO_NORMAL = 0,
	  HI708_VIDEO_MPEG4,	  
	  HI708_VIDEO_MAX
} HI708_VIDEO_MODE;

struct HI708_sensor_STRUCT
{    
      kal_bool first_init;
	  kal_bool pv_mode;                 //True: Preview Mode; False: Capture Mode
	  kal_bool night_mode;              //True: Night Mode; False: Auto Mode
	  kal_bool MPEG4_Video_mode;      //Video Mode: MJPEG or MPEG4
	  kal_uint8 mirror;
	  kal_uint32 pv_pclk;               //Preview Pclk
	  kal_uint32 cp_pclk;               //Capture Pclk
	  kal_uint16 pv_dummy_pixels;          //Dummy Pixels
	  kal_uint16 pv_dummy_lines;           //Dummy Lines
	  kal_uint16 cp_dummy_pixels;          //Dummy Pixels
	  kal_uint16 cp_dummy_lines;           //Dummy Lines         
	  kal_uint16 fix_framerate;         //Fixed Framerate
	  kal_uint32 wb;
	  kal_uint32 exposure;
	  kal_uint32 effect;
	  kal_uint32 banding;
	  kal_uint16 pv_line_length;
	  kal_uint16 pv_frame_height;
	  kal_uint16 cp_line_length;
	  kal_uint16 cp_frame_height;
	  kal_uint16 video_current_frame_rate;
};


//export functions
UINT32 HI708Open(void);
UINT32 HI708GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 HI708GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 HI708Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 HI708FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
UINT32 HI708Close(void);


#endif /* __SENSOR_H */




