#ifndef __JPEG_DRV_6589_COMMON_H__
#define __JPEG_DRV_6589_COMMON_H__

/*
//#include <linux/uaccess.h>
//#include <linux/module.h>
//#include <linux/fs.h>
//#include <linux/platform_device.h>
//#include <linux/cdev.h>
//#include <linux/interrupt.h>
//#include <linux/sched.h>
//#include <linux/wait.h>
//#include <linux/spinlock.h>
//#include <linux/delay.h>
//#include <linux/earlysuspend.h>
//#include <linux/mm.h>
//#include <linux/slab.h>
//
//#include <mach/mt6575_typedefs.h>
//
//#include <asm/tcm.h>
//#include <asm/io.h>
*/

#include <mach/mt_typedefs.h>
//#include <mach/typedefs.h>

#include "jpeg_drv.h"






/*
typedef unsigned int    kal_uint32;
typedef unsigned char   kal_uint8;
typedef int             kal_int32;
*/


/* Decoder Driver */




/* Encoder Driver */

typedef enum
{
    YUYV,
    YVYU,
    NV12,
    NV21
} JpegDrvEncYUVFormat;

typedef enum
{
    Q60 = 0x0,
    Q80 = 0x1, 
    Q90 = 0x2,   
    Q95 = 0x3,

    Q39 = 0x4,
    Q68 = 0x5,  
    Q84 = 0x6,
    Q92 = 0x7,
   
    Q48 = 0x9,   
    Q74 = 0xA,   
    Q87 = 0xB,

    Q34 = 0xD,
    Q64 = 0xE,       
    Q82 = 0xF,

    Q_ALL = 0x10
} JpegDrvEncQuality;


typedef struct 
{
    kal_uint32 width;
    kal_uint32 height;
    kal_uint32 yuv_format;
    kal_uint32 luma_addr;
    kal_uint32 chroma_addr;
} JpegDrvEncSrcCfg;

typedef struct
{
    kal_uint32 exif_en;// 0:JPG mode, 1:JFIF/EXIF mode
    kal_uint32 dst_addr;
    kal_uint32 dst_size;
    kal_uint32 offset_addr;
    kal_uint32 byte_offset_mask;
} JpegDrvEncDstCfg;

typedef struct
{
    kal_uint32 quality;
    kal_uint32 restart_interval;
    kal_uint32  gmc_disable;        //HW not support
} JpegDrvEncCtrlCfg;

#define JPEG_DRV_ENC_YUYV                     (0x00 << 3)
#define JPEG_DRV_ENC_YVYU                     (0x01 << 3)
#define JPEG_DRV_ENC_NV12                     (0x02 << 3)
#define JPEG_DRV_ENC_NV21                     (0x03 << 3)


///////// JPEG Driver Decoder ///////
//
//
int         jpeg_drv_dec_set_config_data(JPEG_DEC_DRV_IN* config);
void jpeg_drv_dec_set_dst_bank0(unsigned int addr_Y, unsigned int addr_U,unsigned int addr_V) ;
void        jpeg_drv_dec_reset(void);
void        jpeg_drv_dec_hard_reset(void);
void        jpeg_drv_dec_start(void);
int         jpeg_drv_dec_wait(JPEG_DEC_DRV_IN* config);
void        jpeg_drv_dec_dump_key_reg(void);
void        jpeg_drv_dec_dump_reg(void);
int         jpeg_drv_dec_break(void);

void jpeg_drv_dec_set_pause_mcu_idx(unsigned int McuIdx) ;
void jpeg_drv_dec_resume(unsigned int resume) ;

kal_uint32 jpeg_drv_dec_get_result(void) ;


/////// JPEG Driver Encoder ///////

kal_uint32  jpeg_drv_enc_src_cfg(JpegDrvEncSrcCfg srcCfg);
kal_uint32  jpeg_drv_enc_dst_buff(JpegDrvEncDstCfg dstCfg);
kal_uint32 jpeg_drv_enc_ctrl_cfg( kal_uint32 exif_en, kal_uint32 quality, kal_uint32 restart_interval);

void        jpeg_drv_enc_reset(void);
kal_uint32  jpeg_drv_enc_warm_reset(void);
void        jpeg_drv_enc_start(void);
kal_uint32  jpeg_drv_enc_set_quality(kal_uint32 quality);
kal_uint32  jpeg_drv_enc_set_img_size(kal_uint32 width, kal_uint32 height);
kal_uint32  jpeg_drv_enc_set_blk_num(kal_uint32 blk_num);
kal_uint32  jpeg_drv_enc_set_luma_addr(kal_uint32 src_luma_addr);
kal_uint32  jpeg_drv_enc_set_chroma_addr(kal_uint32 src_luma_addr);
kal_uint32  jpeg_drv_enc_set_memory_stride(kal_uint32 mem_stride);
kal_uint32  jpeg_drv_enc_set_image_stride(kal_uint32 img_stride);
void        jpeg_drv_enc_set_restart_interval(kal_uint32 restart_interval);
kal_uint32  jpeg_drv_enc_set_offset_addr(kal_uint32 offset);
void        jpeg_drv_enc_set_EncodeMode(kal_uint32 exif_en);// 0:JPG mode, 1:JFIF/EXIF mode
void        jpeg_drv_enc_set_burst_type(kal_uint32 burst_type);
kal_uint32 jpeg_drv_enc_set_dst_buff(kal_uint32 dst_addr, kal_uint32 stall_size, kal_uint32 init_offset, kal_uint32 offset_mask);
kal_uint32  jpeg_drv_enc_set_sample_format_related(kal_uint32 width, kal_uint32 height, kal_uint32 yuv_format);
kal_uint32  jpeg_drv_enc_get_file_size(void);
kal_uint32  jpeg_drv_enc_get_result(kal_uint32 *fileSize);
kal_uint32  jpeg_drv_enc_get_cycle_count(void);

void        jpeg_drv_enc_dump_reg(void);

kal_uint32  jpeg_drv_enc_rw_reg(void);


int jpeg_isr_enc_lisr(void);
int jpeg_isr_dec_lisr(void);


kal_uint32 jpeg_drv_enc_set_src_image(kal_uint32 width, kal_uint32 height, kal_uint32 yuv_format, kal_uint32 totalEncDU) ;
kal_uint32 jpeg_drv_enc_set_src_buf(kal_uint32 yuv_format, kal_uint32 img_stride, kal_uint32 mem_stride, kal_uint32 srcAddr, kal_uint32 srcAddr_C) ;
kal_uint32 jpeg_drv_enc_set_encFormat(kal_uint32 encFormat);


#endif
