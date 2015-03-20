#ifndef __MTKFB_H
#define __MTKFB_H

#include "mtkfb_info.h"


#define MTK_FB_NO_ION_FD                 ((int)(~0U>>1))
#define FBCAPS_GENERIC_MASK              (0x00000fff)
#define FBCAPS_LCDC_MASK                 (0x00fff000)
#define FBCAPS_PANEL_MASK                (0xff000000)
#define FBCAPS_MANUAL_UPDATE             (0x00001000)
#define FBCAPS_SET_BACKLIGHT             (0x01000000)
#define MTKFB_ERROR_IS_EARLY_SUSPEND     (0x12000000)
#define HW_OVERLAY_COUNT                 (4)
#define RESERVED_LAYER_COUNT             (2)
#define VIDEO_LAYER_COUNT                (HW_OVERLAY_COUNT - RESERVED_LAYER_COUNT)
//---------------------------------------------------------------------------
/* IOCTL commands. */
#define MTK_IOW(num, dtype)     _IOW('O', num, dtype)
#define MTK_IOR(num, dtype)     _IOR('O', num, dtype)
#define MTK_IOWR(num, dtype)    _IOWR('O', num, dtype)
#define MTK_IO(num)             _IO('O', num)

// --------------------------------------------------------------------------
#define MTKFB_SET_OVERLAY_LAYER                MTK_IOW(45, struct fb_overlay_layer)
#define MTKFB_TRIG_OVERLAY_OUT                 MTK_IO(46)
#define MTKFB_SET_VIDEO_LAYERS                 MTK_IOW(51, struct fb_overlay_layer)
#define MTKFB_CAPTURE_FRAMEBUFFER              MTK_IOW(52, unsigned long)
#define MTKFB_CONFIG_IMMEDIATE_UPDATE          MTK_IOW(53, unsigned long)
#define MTKFB_SET_MULTIPLE_LAYERS              MTK_IOW(54, struct fb_overlay_layer)
#define MTKFB_REGISTER_OVERLAYBUFFER           MTK_IOW(55, struct fb_overlay_buffer_info)
#define MTKFB_UNREGISTER_OVERLAYBUFFER         MTK_IOW(56, unsigned int)
#define MTKFB_SET_ORIENTATION                  MTK_IOW(57, unsigned long)
#define MTKFB_FBLAYER_ENABLE                   MTK_IOW(58,unsigned int)
#define MTKFB_LOCK_FRONT_BUFFER                MTK_IO(59)
#define MTKFB_UNLOCK_FRONT_BUFFER              MTK_IO(60)
#define MTKFB_POWERON				           MTK_IO(61)
#define MTKFB_POWEROFF				           MTK_IO(62)

// Fence/Ion, OVL decoupling
#define MTKFB_PREPARE_OVERLAY_BUFFER           MTK_IOW(66, struct fb_overlay_buffer)

// S3D control
#define MTKFB_SET_COMPOSING3D                  MTK_IOW(70, unsigned long)
#define MTKFB_SET_S3D_FTM		               MTK_IOW(71, unsigned long)

//FM De-sense for EM and Normal mode
#define MTKFB_GET_DEFAULT_UPDATESPEED          MTK_IOR(72, unsigned long)
#define MTKFB_GET_CURR_UPDATESPEED             MTK_IOR(73, unsigned long)
#define MTKFB_CHANGE_UPDATESPEED               MTK_IOW(74, unsigned long)   //for EM, not called change writecycle because DPI change pll ckl
#define MTKFB_GET_INTERFACE_TYPE               MTK_IOR(77, unsigned long)  ///0 DBI, 1 DPI, 2 MIPI
#define MTKFB_GET_POWERSTATE		           MTK_IOR(78, unsigned long)  ///0: power off  1: power on
#define MTKFB_GET_DISPLAY_IF_INFORMATION       MTK_IOR(90, mtk_dispif_info_t)
#define MTKFB_AEE_LAYER_EXIST                  MTK_IOR(92, unsigned long)	//called before SET_OVERLAY each time, if true, hwc will not use FB_LAYER again
#define MTKFB_GET_OVERLAY_LAYER_INFO           MTK_IOR(93, struct fb_overlay_layer_info)
#define MTKFB_FACTORY_AUTO_TEST                MTK_IOR(94, unsigned long)
#define MTKFB_GET_FRAMEBUFFER_MVA              MTK_IOR(95, unsigned int)
#define MTKFB_SLT_AUTO_CAPTURE                 MTK_IOWR(96, struct fb_slt_catpure)

//error handling
#define MTKFB_META_RESTORE_SCREEN              MTK_IOW(101, unsigned long)
#define MTKFB_ERROR_INDEX_UPDATE_TIMEOUT       MTK_IO(103)
#define MTKFB_ERROR_INDEX_UPDATE_TIMEOUT_AEE   MTK_IO(104)

//----------------------------------------------------------------------

// --------------------------------------------------------------------------

typedef enum
{
    MTK_FB_ORIENTATION_0   = 0,
    MTK_FB_ORIENTATION_90  = 1,
    MTK_FB_ORIENTATION_180 = 2,
    MTK_FB_ORIENTATION_270 = 3,
} MTK_FB_ORIENTATION;


typedef enum
{
    MTK_FB_TV_SYSTEM_NTSC = 0,
    MTK_FB_TV_SYSTEM_PAL  = 1,
} MTK_FB_TV_SYSTEM;


typedef enum
{
    MTK_FB_TV_FMT_RGB565     = 0,
    MTK_FB_TV_FMT_YUV420_SEQ = 1,
    MTK_FB_TV_FMT_UYUV422 =    2,
    MTK_FB_TV_FMT_YUV420_BLK = 3,
} MTK_FB_TV_SRC_FORMAT;

typedef struct _disp_dfo_item
{
	char name[32];
	int  value;
}disp_dfo_item_t;

// --------------------------------------------------------------------------
struct fb_slt_catpure
{
    MTK_FB_FORMAT format;
	volatile char* outputBuffer;
	unsigned int wdma_width;
	unsigned int wdma_height;
};

struct fb_scale {
    unsigned int xscale, yscale;
};

struct fb_frame_offset {
    unsigned int idx;
    unsigned long offset;
};

struct fb_update_window {
    unsigned int x, y;
    unsigned int width, height;
};

typedef enum
{
	LAYER_2D 			= 0,
	LAYER_3D_SBS_0 		= 0x1,
	LAYER_3D_SBS_90 	= 0x2,
	LAYER_3D_SBS_180 	= 0x3,
	LAYER_3D_SBS_270 	= 0x4,
	LAYER_3D_TAB_0 		= 0x10,
	LAYER_3D_TAB_90 	= 0x20,
	LAYER_3D_TAB_180 	= 0x30,
	LAYER_3D_TAB_270 	= 0x40,
} MTK_FB_LAYER_TYPE;

typedef enum {
	DISP_DIRECT_LINK_MODE,
	DISP_DECOUPLE_MODE
}MTK_DISP_MODE;
struct fb_overlay_mode {
	MTK_DISP_MODE mode;
};

typedef enum {              // map sessions to scenairos in kernel driver
	DISP_SESSION_LCM        = 1<<0,   // DSI0
	DISP_SESSION_MEM        = 1<<1,		// OVL0->WDMA0
  DISP_SESSION_WFD          = 1<<2,   // Extention mode, Dst buf is provided by user, for Wifi Display or other purpose
	DISP_SESSION_MHL        = 1<<3,   // DPI
	DISP_SESSION_LCM1       = 1<<4,   // DSI1
  DISP_SESSION_MEM1         = 1<<5,   // OVL1->WDMA1
	//TODO:can be extended with other Session Id
	SESSION_MASK	        = 0xff&~(1<<6)
} MTK_DISP_SESSION;

struct fb_overlay_session {
	unsigned int  session; // one or more @MTK_DISP_SESSION combined
};

struct fb_overlay_decouple {
	MTK_DISP_MODE mode;
	unsigned int session;
};
struct fb_overlay_buffer{
	//	Input
	int layer_id;
	unsigned int layer_en;
	int ion_fd;
	unsigned int cache_sync;
    // Output
	unsigned int index;
    int fence_fd;
};

struct fb_overlay_layer {
    unsigned int layer_id;
    unsigned int layer_enable;

    void* src_base_addr;
    void* src_phy_addr;
    unsigned int  src_direct_link;
    MTK_FB_FORMAT src_fmt;
    unsigned int  src_use_color_key;
    unsigned int  src_color_key;
    unsigned int  src_pitch;
    unsigned int  src_offset_x, src_offset_y;
    unsigned int  src_width, src_height;

    unsigned int  tgt_offset_x, tgt_offset_y;
    unsigned int  tgt_width, tgt_height;
    MTK_FB_ORIENTATION layer_rotation;
    MTK_FB_LAYER_TYPE	layer_type;
    MTK_FB_ORIENTATION video_rotation;
    
    unsigned int isTdshp;  // set to 1, will go through tdshp first, then layer blending, then to color

    int next_buff_idx;
    int identity;
    int connected_type;
    unsigned int security;
    unsigned int alpha_enable;
    unsigned int alpha;
};

struct fb_overlay_buffer_info{
    unsigned int src_vir_addr;
    unsigned int size;
};

struct fb_overlay_layer_info {
    unsigned int layer_id;
    unsigned int layer_enabled;  // TO BE DEL
    unsigned int curr_en;
    unsigned int next_en;
    unsigned int hw_en;
    int curr_idx;
    int next_idx;
    int hw_idx;
    int curr_identity;
    int next_identity;
    int hw_identity;
    int curr_conn_type;
    int next_conn_type;
    int hw_conn_type;
};
// --------------------------------------------------------------------------

struct fb_post_video_buffer {
    void*                phy_addr;
    void*                vir_addr;
    MTK_FB_TV_SRC_FORMAT format;
    unsigned int         width, height;
};

#ifdef __KERNEL__

#include <linux/completion.h>
#include <linux/interrupt.h>

#define MTKFB_DRIVER "mtkfb"

enum mtkfb_state {
    MTKFB_DISABLED  = 0,
    MTKFB_SUSPENDED = 99,
    MTKFB_ACTIVE    = 100
};

typedef enum {
    MTKFB_LAYER_ENABLE_DIRTY = (1 << 0),
    MTKFB_LAYER_FORMAT_DIRTY = (1 << 1),
    MTKFB_LAYER_SET_DIRTY    = (1 << 2),
} MTKFB_LAYER_CONFIG_DIRTY;

struct mtkfb_device {
    int             state;
    void           *fb_va_base;             /* MPU virtual address */
    dma_addr_t      fb_pa_base;             /* Bus physical address */
    unsigned long   fb_size_in_byte;

    unsigned long   layer_enable;
    MTK_FB_FORMAT   layer_format[HW_OVERLAY_COUNT];
    unsigned int    layer_config_dirty;

    int             xscale, yscale, mirror; /* transformations.
                                               rotate is stored in fb_info->var */
    u32             pseudo_palette[17];

    struct fb_info *fb_info;                /* Linux fbdev framework data */
    struct device  *dev;
};

#endif /* __KERNEL__ */

#endif /* __MTKFB_H */
