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
 *   Header file of Sensor driver
 *
 *
 * Author:
 * -------
 *   Leo Lee
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Revision:$
 * $Modtime:$
 * $Log:$
 *
 * [GC2035YUV V1.0.0]
 * 8.17.2012 Lanking zhou
 * .First Release
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by GalaxyCoreinc. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
/* SENSOR FULL SIZE */
#ifndef __SENSOR_H
#define __SENSOR_H


typedef enum _GC2035_OP_TYPE_ {
        GC2035_MODE_NONE,
        GC2035_MODE_PREVIEW,
        GC2035_MODE_CAPTURE,
        GC2035_MODE_QCIF_VIDEO,
        GC2035_MODE_CIF_VIDEO,
        GC2035_MODE_QVGA_VIDEO
    } GC2035_OP_TYPE;

extern GC2035_OP_TYPE GC2035_g_iGC2035_Mode;

/* START GRAB PIXEL OFFSET */
#define IMAGE_SENSOR_START_GRAB_X		        2 //2
#define IMAGE_SENSOR_START_GRAB_Y		        2 //2

/* SENSOR PIXEL/LINE NUMBERS IN ONE PERIOD */
#define GC2035_FULL_PERIOD_PIXEL_NUMS  (1940)  // default pixel#(w/o dummy pixels) in UXGA mode
#define GC2035_FULL_PERIOD_LINE_NUMS   (1238)  // default line#(w/o dummy lines) in UXGA mode
#define GC2035_PV_PERIOD_PIXEL_NUMS    (970)  // default pixel#(w/o dummy pixels) in SVGA mode
#define GC2035_PV_PERIOD_LINE_NUMS     (670)   // default line#(w/o dummy lines) in SVGA mode

/* SENSOR EXPOSURE LINE LIMITATION */
#define GC2035_FULL_EXPOSURE_LIMITATION    (1236)
#define GC2035_PV_EXPOSURE_LIMITATION      (671)

/* SENSOR FULL SIZE */
#define GC2035_IMAGE_SENSOR_FULL_WIDTH	   (1600)
#define GC2035_IMAGE_SENSOR_FULL_HEIGHT	  (1200)

/* SENSOR PV SIZE */
#define GC2035_IMAGE_SENSOR_PV_WIDTH   (800) //800
#define GC2035_IMAGE_SENSOR_PV_HEIGHT  (600) //

#define GC2035_VIDEO_QCIF_WIDTH   (176)
#define GC2035_VIDEO_QCIF_HEIGHT  (144)

/* SENSOR READ/WRITE ID */
#define GC2035_WRITE_ID	0x78
#define GC2035_READ_ID		0x79


/* SENSOR CHIP VERSION */
//#define GC2035_SENSOR_ID							0x2035


//s_add for porting
//s_add for porting
//s_add for porting

//export functions
UINT32 GC2035Open(void);
UINT32 GC2035GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 GC2035GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 GC2035Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 GC2035FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
UINT32 GC2035Close(void);


//e_add for porting
//e_add for porting
//e_add for porting


#endif /* __SENSOR_H */
