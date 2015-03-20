#ifndef __MTKFB_INFO_H__
#define __MTKFB_INFO_H__

#ifdef __cplusplus
extern "C" {
#endif


typedef enum
{
    DISPIF_TYPE_DBI = 0,
    DISPIF_TYPE_DPI,
    DISPIF_TYPE_DSI,
    DISPIF_TYPE_DPI0,
    DISPIF_TYPE_DPI1,
    DISPIF_TYPE_DSI0,
    DISPIF_TYPE_DSI1,
    HDMI,
    HDMI_SMARTBOOK, 
    MHL
} MTKFB_DISPIF_TYPE;

typedef enum
{
    MTKFB_DISPIF_PRIMARY_LCD = 0,
    MTKFB_DISPIF_HDMI,
    MTKFB_MAX_DISPLAY_COUNT
} MTKFB_DISPIF_DEVICE_TYPE;

typedef enum
{
    DISPIF_FORMAT_RGB565 = 0,
    DISPIF_FORMAT_RGB666,
    DISPIF_FORMAT_RGB888 
} MTKFB_DISPIF_FORMAT;


typedef enum
{
    DISPIF_MODE_VIDEO = 0,
    DISPIF_MODE_COMMAND
} MTKFB_DISPIF_MODE;

typedef struct mtk_dispif_info {
	unsigned int display_id;
	unsigned int isHwVsyncAvailable;
	MTKFB_DISPIF_TYPE displayType;
	unsigned int displayWidth;
	unsigned int displayHeight;
	unsigned int displayFormat;
	MTKFB_DISPIF_MODE displayMode;
	unsigned int vsyncFPS;
	unsigned int physicalWidth;
	unsigned int physicalHeight;
	unsigned int isConnected;
	unsigned int lcmOriginalWidth;	// this value is for DFO Multi-Resolution feature, which stores the original LCM Wdith
	unsigned int lcmOriginalHeight;	// this value is for DFO Multi-Resolution feature, which stores the original LCM Height
} mtk_dispif_info_t;

#define MAKE_MTK_FB_FORMAT_ID(id, bpp)  (((id) << 8) | (bpp))

typedef enum
{
    MTK_FB_FORMAT_UNKNOWN = 0,
        
    MTK_FB_FORMAT_RGB565   = MAKE_MTK_FB_FORMAT_ID(1, 2),
    MTK_FB_FORMAT_RGB888   = MAKE_MTK_FB_FORMAT_ID(2, 3),
    MTK_FB_FORMAT_BGR888   = MAKE_MTK_FB_FORMAT_ID(3, 3),
    MTK_FB_FORMAT_ARGB8888 = MAKE_MTK_FB_FORMAT_ID(4, 4),
    MTK_FB_FORMAT_ABGR8888 = MAKE_MTK_FB_FORMAT_ID(5, 4),
    MTK_FB_FORMAT_YUV422   = MAKE_MTK_FB_FORMAT_ID(6, 2),
    MTK_FB_FORMAT_XRGB8888 = MAKE_MTK_FB_FORMAT_ID(7, 4),
    MTK_FB_FORMAT_XBGR8888 = MAKE_MTK_FB_FORMAT_ID(8, 4),
    MTK_FB_FORMAT_UYVY     = MAKE_MTK_FB_FORMAT_ID(9, 2),
    MTK_FB_FORMAT_YUV420_P = MAKE_MTK_FB_FORMAT_ID(10, 2),
    MTK_FB_FORMAT_BPP_MASK = 0xFF,
} MTK_FB_FORMAT;

#define GET_MTK_FB_FORMAT_BPP(f)    ((f) & MTK_FB_FORMAT_BPP_MASK)


#ifdef __cplusplus
}
#endif

#endif // __DISP_DRV_H__

