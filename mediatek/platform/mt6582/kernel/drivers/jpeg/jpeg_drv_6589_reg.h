#ifndef __JPEG_DRV_6589_REG_H__
#define __JPEG_DRV_6589_REG_H__


#include <mach/mt_reg_base.h>

#include <mach/sync_write.h>





#if 1

#define JPEG_EARLY_MM_BASE    0xF5000000

#define EARLY_JPEG_DEC_BASE    0xF5009000           
#define EARLY_JPEG_ENC_BASE    0xF500A000  
//#define JPEG_DEC_BASE   //EARLY_JPEG_DEC_BASE
//#define JPEG_ENC_BASE   //EARLY_JPEG_ENC_BASE
//#define JPEG_DEC_BASE   (JPG_CODEC_BASE)    //EARLY_JPEG_DEC_BASE
//#define JPEG_ENC_BASE   (JPG_CODEC_BASE + 0x1000)   //EARLY_JPEG_ENC_BASE
#define JPEG_DEC_BASE   EARLY_JPEG_DEC_BASE        
#define JPEG_ENC_BASE   EARLY_JPEG_ENC_BASE

#else
//0xF5003000

#define FPGA_JPEG_ENC_BASE    0xF500A000           //0xF5002000
#define FPGA_JPEG_DEC_BASE    0xF5009000           //0xF5003000
#define JPEG_ENC_BASE         FPGA_JPEG_ENC_BASE   //(JPG_CODEC_BASE)
#define JPEG_DEC_BASE         FPGA_JPEG_DEC_BASE   //(JPG_CODEC_BASE + 0x1000)

#endif


#define REG_JPEG_MM_REG_MASK                           *(volatile kal_uint32 *)(JPEG_EARLY_MM_BASE + 0x000)


#define IMG_REG_WRITE(v,a) mt65xx_reg_sync_writel(v,a)

#define IMG_REG_READ(v,a) { (v) = *(volatile kal_uint32 *)(a); }

//#define IMG_REG_READ(v,a) mt65xx_reg_sync_writew(v,a)


/********************************************************************/
/* The following registers are for JPEG Encoder Registers on MT6589 */
/********************************************************************/



#define REG_JPEG_ENC_RSTB                           *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x100)
#define REG_JPEG_ENC_CTRL                           *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x104)
#define REG_JPEG_ENC_QUALITY                        *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x108)
#define REG_JPEG_ENC_BLK_NUM                        *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x10C)
#define REG_JPEG_ENC_BLK_CNT                        *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x110)
#define REG_JPEG_ENC_INTERRUPT_STATUS               *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x11C)

#define REG_JPEG_ENC_DST_ADDR0                      *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x120)
#define REG_JPEG_ENC_DMA_ADDR0                      *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x124)
#define REG_JPEG_ENC_STALL_ADDR0                    *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x128)

#define REG_JPEG_ENC_OFFSET_ADDR                    *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x138)
#define REG_JPEG_ENC_CURR_DMA_ADDR                  *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x13C)

#define REG_JPEG_ENC_RST_MCU_NUM                    *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x150)
#define REG_JPEG_ENC_IMG_SIZE                       *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x154)
#define REG_JPEG_ENC_GULTRA_TRESH                   *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x158)


#define REG_JPEG_ENC_DEBUG_INFO0                    *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x160)
#define REG_JPEG_ENC_DEBUG_INFO1                    *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x164)
#define REG_JPEG_ENC_TOTAL_CYCLE                    *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x168)
#define REG_JPEG_ENC_BYTE_OFFSET_MASK               *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x16C)


#define REG_JPEG_ENC_SRC_LUMA_ADDR                  *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x170)
#define REG_JPEG_ENC_SRC_CHROMA_ADDR                *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x174)
#define REG_JPEG_ENC_STRIDE                         *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x178)
#define REG_JPEG_ENC_IMG_STRIDE                     *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x17C)
#define REG_JPEG_ENC_MEM_CYCLE                      *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x208)


#define REG_JPEG_ENC_SMI_DEBUG0                     *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x304)
#define REG_JPEG_ENC_SMI_DEBUG1                     *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x308)
#define REG_JPEG_ENC_SMI_DEBUG2                     *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x30C)
#define REG_JPEG_ENC_SMI_DEBUG3                     *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x310)
#define REG_JPEG_ENC_CODEC_SEL                      *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x314)


#define JPEG_ENC_REG_COUNT                          0x314





/********************************************************************/
/* define JPEG Encoder Registers register field*/
/********************************************************************/



/* #define REG_JPEG_ENC_RSTB                           *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x100)  */
/* #define REG_JPEG_ENC_CTRL                           *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x104)  */
/* #define REG_JPEG_ENC_QUALITY                        *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x108)  */
/* #define REG_JPEG_ENC_BLK_NUM                        *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x10C)  */
/* #define REG_JPEG_ENC_BLK_CNT                        *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x110)  */
/* #define REG_JPEG_ENC_INTERRUPT_STATUS               *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x11C)  */
#define JPEG_DRV_ENC_INT_STATUS_DONE   0x01
#define JPEG_DRV_ENC_INT_STATUS_STALL  0x02
#define JPEG_DRV_ENC_INT_STATUS_VCODEC_IRQ  0x10
#define JPEG_DRV_ENC_INT_STATUS_MASK_ALLIRQ 0x13
#define JPEG_DRV_DEC_INT_STATUS_DEC_ERR   0x04



/* #define REG_JPEG_ENC_DST_ADDR0                      *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x120)  */
/* #define REG_JPEG_ENC_DMA_ADDR0                      *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x124)  */
/* #define REG_JPEG_ENC_STALL_ADDR0                    *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x128)  */
/*                                                                                                      */
/* #define REG_JPEG_ENC_OFFSET_ADDR                    *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x138)  */
/* #define REG_JPEG_ENC_CURR_DMA_ADDR                  *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x13C)  */
/*                                                                                                      */
/* #define REG_JPEG_ENC_RST_MCU_NUM                    *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x150)  */
/* #define REG_JPEG_ENC_IMG_SIZE                       *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x154)  */
/* #define REG_JPEG_ENC_GULTRA_TRESH                   *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x158)  */
/*                                                                                                      */
/*                                                                                                      */
/* #define REG_JPEG_ENC_DEBUG_INFO0                    *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x160)  */
/* #define REG_JPEG_ENC_DEBUG_INFO1                    *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x164)  */
/* #define REG_JPEG_ENC_TOTAL_CYCLE                    *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x168)  */
/* #define REG_JPEG_ENC_BYTE_OFFSET_MASK               *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x16C)  */
/*                                                                                                      */
/*                                                                                                      */
/* #define REG_JPEG_ENC_SRC_LUMA_ADDR                  *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x170)  */
/* #define REG_JPEG_ENC_SRC_CHROMA_ADDR                *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x174)  */
/* #define REG_JPEG_ENC_STRIDE                         *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x178)  */
/* #define REG_JPEG_ENC_IMG_STRIDE                     *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x17C)  */
/* #define REG_JPEG_ENC_MEM_CYCLE                      *(volatile kal_uint32 *)(JPEG_ENC_BASE + 0x208)  */





/********************************************************************/
/* The following registers are for JPEG Encoder Registers on MT6589 */
/********************************************************************/



#define REG_ADDR_JPEG_ENC_RSTB                           (JPEG_ENC_BASE + 0x100)
#define REG_ADDR_JPEG_ENC_CTRL                           (JPEG_ENC_BASE + 0x104)
#define REG_ADDR_JPEG_ENC_QUALITY                        (JPEG_ENC_BASE + 0x108)
#define REG_ADDR_JPEG_ENC_BLK_NUM                        (JPEG_ENC_BASE + 0x10C)
#define REG_ADDR_JPEG_ENC_BLK_CNT                        (JPEG_ENC_BASE + 0x110)
#define REG_ADDR_JPEG_ENC_INTERRUPT_STATUS               (JPEG_ENC_BASE + 0x11C)
             
#define REG_ADDR_JPEG_ENC_DST_ADDR0                      (JPEG_ENC_BASE + 0x120)
#define REG_ADDR_JPEG_ENC_DMA_ADDR0                      (JPEG_ENC_BASE + 0x124)
#define REG_ADDR_JPEG_ENC_STALL_ADDR0                    (JPEG_ENC_BASE + 0x128)
             
#define REG_ADDR_JPEG_ENC_OFFSET_ADDR                    (JPEG_ENC_BASE + 0x138)
#define REG_ADDR_JPEG_ENC_CURR_DMA_ADDR                  (JPEG_ENC_BASE + 0x13C)
             
#define REG_ADDR_JPEG_ENC_RST_MCU_NUM                    (JPEG_ENC_BASE + 0x150)
#define REG_ADDR_JPEG_ENC_IMG_SIZE                       (JPEG_ENC_BASE + 0x154)
#define REG_ADDR_JPEG_ENC_GULTRA_TRESH                   (JPEG_ENC_BASE + 0x158)
             
             
#define REG_ADDR_JPEG_ENC_DEBUG_INFO0                    (JPEG_ENC_BASE + 0x160)
#define REG_ADDR_JPEG_ENC_DEBUG_INFO1                    (JPEG_ENC_BASE + 0x164)
#define REG_ADDR_JPEG_ENC_TOTAL_CYCLE                    (JPEG_ENC_BASE + 0x168)
#define REG_ADDR_JPEG_ENC_BYTE_OFFSET_MASK               (JPEG_ENC_BASE + 0x16C)
             
             
#define REG_ADDR_JPEG_ENC_SRC_LUMA_ADDR                  (JPEG_ENC_BASE + 0x170)
#define REG_ADDR_JPEG_ENC_SRC_CHROMA_ADDR                (JPEG_ENC_BASE + 0x174)
#define REG_ADDR_JPEG_ENC_STRIDE                         (JPEG_ENC_BASE + 0x178)
#define REG_ADDR_JPEG_ENC_IMG_STRIDE                     (JPEG_ENC_BASE + 0x17C)
#define REG_ADDR_JPEG_ENC_MEM_CYCLE                      (JPEG_ENC_BASE + 0x208)


#define REG_ADDR_JPEG_ENC_SMI_DEBUG0                     (JPEG_ENC_BASE + 0x304)
#define REG_ADDR_JPEG_ENC_SMI_DEBUG1                     (JPEG_ENC_BASE + 0x308)
#define REG_ADDR_JPEG_ENC_SMI_DEBUG2                     (JPEG_ENC_BASE + 0x30C)
#define REG_ADDR_JPEG_ENC_SMI_DEBUG3                     (JPEG_ENC_BASE + 0x310)
#define REG_ADDR_JPEG_ENC_CODEC_SEL                      (JPEG_ENC_BASE + 0x314)






/********************************************************************/
/* The following registers are for JPEG Decoder Registers on MT6589 */
/********************************************************************/




#define REG_JPGDEC_RESET                        *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x0090 )
#define REG_JPGDEC_BRZ_FACTOR                   *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x00F8 )
#define REG_JPGDEC_DU_SAMPLE                       *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x00FC )
#define REG_JPGDEC_DEST_ADDR0_Y                 *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x0140 )
#define REG_JPGDEC_DEST_ADDR0_U                 *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x0144 )
#define REG_JPGDEC_DEST_ADDR0_V                 *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x0148 )
#define REG_JPGDEC_DEST_ADDR1_Y                 *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x014C )
#define REG_JPGDEC_DEST_ADDR1_U                 *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x0150 )
#define REG_JPGDEC_DEST_ADDR1_V                 *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x0154 )
#define REG_JPGDEC_STRIDE_Y                     *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x0158 )
#define REG_JPGDEC_STRIDE_UV                    *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x015C )
#define REG_JPGDEC_IMG_STRIDE_Y                 *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x0160 )
#define REG_JPGDEC_IMG_STRIDE_UV                *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x0164 )
//#define REG_JPGDEC_TOTAL_MCU_NUM                *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x0168 )
#define REG_JPGDEC_WDMA_CTRL                    *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x016C )
#define REG_JPGDEC_PAUSE_MCU_NUM                *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x0170 )
#define REG_JPGDEC_OPERATION_MODE               *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x017C )
#define REG_JPGDEC_DEBUG0                       *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x0180 )
#define REG_JPGDEC_FILE_ADDR                    *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x0200 )
#define REG_JPGDEC_COMP_ID                      *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x020C )
#define REG_JPGDEC_TOTAL_MCU_NUM                *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x0210 )
#define REG_JPGDEC_COMP0_DATA_UNIT_NUM          *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x0224 )
#define REG_JPGDEC_DU_CTRL                      *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x023C )
#define REG_JPGDEC_TRIG                         *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x0240 )
#define REG_JPGDEC_FILE_BRP                     *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x0248 )
#define REG_JPGDEC_FILE_TOTAL_SIZE              *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x024C )
#define REG_JPGDEC_QT_ID                        *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x0270 )
#define REG_JPGDEC_INTERRUPT_STATUS             *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x0274 )
#define REG_JPGDEC_STATUS                       *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x0278 )
#define REG_JPGDEC_MCU_CNT                      *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x0294 )
        



#define REG_JPGDEC_DEBUG_MODE                        *(volatile kal_uint32 *) ( JPEG_DEC_BASE + 0x00C8 )





/********************************************************************/
/* define JPEG Decoder Registers register field*/
/********************************************************************/



/* REG_JPGDEC_RESET                ( JPEG_DEC_BASE + 0x0090 ) */
#define BIT_SOFT_RST_SHIFT     0
#define BIT_HARD_RST_SHIFT     4

/* REG_JPGDEC_BRZ_FACTOR           ( JPEG_DEC_BASE + 0x00F8 ) */
#define BIT_BRZ_YH_SHIFT     0
#define BIT_BRZ_YV_SHIFT     4
#define BIT_BRZ_CH_SHIFT     8
#define BIT_BRZ_CV_SHIFT     12
/* REG_JPGDEC_DU_SAMPLE               ( JPEG_DEC_BASE + 0x00FC ) */
/* REG_JPGDEC_DEST_ADDR0_Y         ( JPEG_DEC_BASE + 0x0140 ) */
/* REG_JPGDEC_DEST_ADDR0_U         ( JPEG_DEC_BASE + 0x0144 ) */
/* REG_JPGDEC_DEST_ADDR0_V         ( JPEG_DEC_BASE + 0x0148 ) */
/* REG_JPGDEC_DEST_ADDR1_Y         ( JPEG_DEC_BASE + 0x014C ) */
/* REG_JPGDEC_DEST_ADDR1_U         ( JPEG_DEC_BASE + 0x0150 ) */
/* REG_JPGDEC_DEST_ADDR1_V         ( JPEG_DEC_BASE + 0x0154 ) */
/* REG_JPGDEC_STRIDE_Y             ( JPEG_DEC_BASE + 0x0158 ) */
/* REG_JPGDEC_STRIDE_UV            ( JPEG_DEC_BASE + 0x015C ) */
/* REG_JPGDEC_IMG_STRIDE_Y         ( JPEG_DEC_BASE + 0x0160 ) */
/* REG_JPGDEC_IMG_STRIDE_UV        ( JPEG_DEC_BASE + 0x0164 ) */
/* REG_JPGDEC_WDMA_CTRL            ( JPEG_DEC_BASE + 0x016C ) */
/* REG_JPGDEC_PAUSE_MCU_NUM        ( JPEG_DEC_BASE + 0x0170 ) */
/* REG_JPGDEC_OPERATION_MODE       ( JPEG_DEC_BASE + 0x017C ) */
/* REG_JPGDEC_DEBUG0               ( JPEG_DEC_BASE + 0x0180 ) */
/* REG_JPGDEC_FILE_ADDR            ( JPEG_DEC_BASE + 0x0200 ) */
/* REG_JPGDEC_COMP_ID              ( JPEG_DEC_BASE + 0x020C ) */
/* REG_JPGDEC_TOTAL_MCU_NUM        ( JPEG_DEC_BASE + 0x0210 ) */
/* REG_JPGDEC_COMP0_DATA_UNIT_NUM  ( JPEG_DEC_BASE + 0x0224 ) */
/* REG_JPGDEC_DU_CTRL              ( JPEG_DEC_BASE + 0x023C ) */
#define BIT_DU_CTRL_COMP_Y     4
#define BIT_DU_CTRL_COMP_U     5
#define BIT_DU_CTRL_COMP_V     6
#define BIT_DU_CTRL_NOUSE      7

/* REG_JPGDEC_TRIG                 ( JPEG_DEC_BASE + 0x0240 ) */
/* REG_JPGDEC_FILE_BRP             ( JPEG_DEC_BASE + 0x0248 ) */
/* REG_JPGDEC_FILE_TOTAL_SIZE      ( JPEG_DEC_BASE + 0x024C ) */
/* REG_JPGDEC_QT_ID                ( JPEG_DEC_BASE + 0x0270 ) */
/* REG_JPGDEC_INTERRUPT_STATUS     ( JPEG_DEC_BASE + 0x0274 ) */
#define BIT_INQST_MASK_TYPE        0x80000000
#define BIT_INQST_MASK_ERROR_BS    0x20
#define BIT_INQST_MASK_PAUSE       0x10
#define BIT_INQST_MASK_OVERFLOW    0x04
#define BIT_INQST_MASK_UNDERFLOW   0x02
#define BIT_INQST_MASK_EOF         0x01
#define BIT_INQST_MASK_END         0x27
#define BIT_INQST_MASK_ALLIRQ      0x37


/* REG_JPGDEC_STATUS               ( JPEG_DEC_BASE + 0x0278 ) */
#define BIT_DEC_ST_STATE_MASK   0x07000000
#define BIT_DEC_ST_STATE_IDLE   0x00000000
#define BIT_DEC_ST_STATE_DMA    0x01000000
#define BIT_DEC_ST_STATE_HEADER 0x02000000
#define BIT_DEC_ST_STATE_VLD    0x03000000
#define BIT_DEC_ST_STATE_RST    0x04000000
#define BIT_DEC_ST_STATE_PROG   0x05000000
#define BIT_DEC_ST_STATE_IDCT   0x06000000








//REG_JPGDEC_MCU_CNT              ( JPEG_DEC_BASE + 0x0294 )



#define REG_ADDR_JPGDEC_RESET                        ( JPEG_DEC_BASE + 0x0090 )
#define REG_ADDR_JPGDEC_BRZ_FACTOR                   ( JPEG_DEC_BASE + 0x00F8 )
#define REG_ADDR_JPGDEC_DU_SAMPLE                    ( JPEG_DEC_BASE + 0x00FC )
#define REG_ADDR_JPGDEC_DEST_ADDR0_Y                 ( JPEG_DEC_BASE + 0x0140 )
#define REG_ADDR_JPGDEC_DEST_ADDR0_U                 ( JPEG_DEC_BASE + 0x0144 )
#define REG_ADDR_JPGDEC_DEST_ADDR0_V                 ( JPEG_DEC_BASE + 0x0148 )
#define REG_ADDR_JPGDEC_DEST_ADDR1_Y                 ( JPEG_DEC_BASE + 0x014C )
#define REG_ADDR_JPGDEC_DEST_ADDR1_U                 ( JPEG_DEC_BASE + 0x0150 )
#define REG_ADDR_JPGDEC_DEST_ADDR1_V                 ( JPEG_DEC_BASE + 0x0154 )
#define REG_ADDR_JPGDEC_STRIDE_Y                     ( JPEG_DEC_BASE + 0x0158 )
#define REG_ADDR_JPGDEC_STRIDE_UV                    ( JPEG_DEC_BASE + 0x015C )
#define REG_ADDR_JPGDEC_IMG_STRIDE_Y                 ( JPEG_DEC_BASE + 0x0160 )
#define REG_ADDR_JPGDEC_IMG_STRIDE_UV                ( JPEG_DEC_BASE + 0x0164 )
#define REG_ADDR_JPGDEC_WDMA_CTRL                    ( JPEG_DEC_BASE + 0x016C )
#define REG_ADDR_JPGDEC_PAUSE_MCU_NUM                ( JPEG_DEC_BASE + 0x0170 )
#define REG_ADDR_JPGDEC_OPERATION_MODE               ( JPEG_DEC_BASE + 0x017C )
#define REG_ADDR_JPGDEC_DEBUG0                       ( JPEG_DEC_BASE + 0x0180 )
#define REG_ADDR_JPGDEC_FILE_ADDR                    ( JPEG_DEC_BASE + 0x0200 )
#define REG_ADDR_JPGDEC_COMP_ID                      ( JPEG_DEC_BASE + 0x020C )
#define REG_ADDR_JPGDEC_TOTAL_MCU_NUM                ( JPEG_DEC_BASE + 0x0210 )
#define REG_ADDR_JPGDEC_COMP0_DATA_UNIT_NUM          ( JPEG_DEC_BASE + 0x0224 )
#define REG_ADDR_JPGDEC_DU_CTRL                      ( JPEG_DEC_BASE + 0x023C )
#define REG_ADDR_JPGDEC_TRIG                         ( JPEG_DEC_BASE + 0x0240 )
#define REG_ADDR_JPGDEC_FILE_BRP                     ( JPEG_DEC_BASE + 0x0248 )
#define REG_ADDR_JPGDEC_FILE_TOTAL_SIZE              ( JPEG_DEC_BASE + 0x024C )
#define REG_ADDR_JPGDEC_QT_ID                        ( JPEG_DEC_BASE + 0x0270 )
#define REG_ADDR_JPGDEC_INTERRUPT_STATUS             ( JPEG_DEC_BASE + 0x0274 )
#define REG_ADDR_JPGDEC_STATUS                       ( JPEG_DEC_BASE + 0x0278 )
#define REG_ADDR_JPGDEC_MCU_CNT                      ( JPEG_DEC_BASE + 0x0294 )





#define REG_ADDR_JPGDEC_DEBUG_MODE                   ( JPEG_DEC_BASE + 0x00C8 )



/********************************************************************/
/* END JPEG Decoder Registers on MT6589 */
/********************************************************************/






























// MT6575
//
///* Definition for control bit in REG_JPEG_ENC_CTRL */
//#define JPEG_ENC_CTRL_ENABLE_BIT                0x01
//#define JPEG_ENC_CTRL_GRAY_BIT                  0x02
//#define JPEG_ENC_CTRL_IN_EN_BIT                 0x04
//#define JPEG_ENC_CTRL_YUV_BIT                   0x18
//#define JPEG_ENC_CTRL_FILE_FORMAT_BIT           0x20
//#define JPEG_ENC_CTRL_CONT_SHOOT_BIT            0x40  /// not used
//#define JPEG_ENC_CTRL_CONT_SHOOT_ADDR_SW_BIT    0x80  /// not used
//#define JPEG_ENC_CTRL_INIT_EN_BIT               0x10000
//
///* Definition for file format which will be set into JPEG_ENC_CTRL_FILE_FORMAT_BIT */
//#define JPEG_DEV_ENC_FILE_FORMAT_JPEG           0x00
//#define JPEG_DEV_ENC_FILE_FORMAT_JFIF_EXIF      (0x01 << 5)
//
///* Definition for YUV format which will be set into JPEG_ENC_CTRL_YUV_BIT */
//#define JPEG_DRV_ENC_YUV400                     (0x02 << 3)
//#define JPEG_DRV_ENC_YUV422                     0x00
//#define JPEG_DRV_ENC_YUV420                     (0x01 << 3)
//#define JPEG_DRV_ENC_YUV411                     (0x03 << 3)
//
///* Definition for JPEG interrupt type */
//#define JPEG_DRV_ISR_TYPE_NONE                  0x00
//#define JPEG_DRV_ISR_TYPE_DEC                   0x01
//#define JPEG_DRV_ISR_TYPE_ENC                   0x02
//
///* Definition for JPEG interrupt status */
//#define JPEG_DRV_DEC_INT_STATUS_EOF             0x01
//#define JPEG_DRV_DEC_INT_STATUS_BREAKPOINT      0x02
//#define JPEG_DRV_DEC_INT_STATUS_FILE_OVERFLOW   0x04
//#define JPEG_DRV_DEC_INT_STATUS_RANGE_DECODE    0x08
//
//#define JPEG_DRV_DEC_INT_STATUS_IDP_COMPLETE    0x10  /// this bit is not defined in JPEG HW spec.
//                                                      /// it is used to indicate that IDP has completed.
//
///* Definition for encode quality */
//#define JPEG_DRV_ENC_QUALITY_EXCELLENT          0x03
//#define JPEG_DRV_ENC_QUALITY_GOOD               0x05
//#define JPEG_DRV_ENC_QUALITY_FAIR               0x07
//#define JPEG_DRV_ENC_QUALITY_LOW                0x0A
//#define JPEG_DRV_ENC_QUALITY_POOR               0x0E
//
//
//#define JPEG_DRV_DEC_TABLE_ALIGNED_SIZE         4096
//#define JPEG_DRV_DEC_TABLE_SIZE                 2048
//#define JPEG_DRV_DEC_TABLE_ALIGNED_ADDR         0xFFFFF800
//
//#define JPEG_DRV_DEC_PROG_COEF_BUFFER_MAX       (2 * 1024 * 1024)
//#define JPEG_DRV_ENC_FILE_BUFFER_SIZE           (2 * 1024 * 1024)
//
//
//#define JPEG_ISR_TYPE_NONE 0
//#define JPEG_ISR_TYPE_DEC  1
//#define JPEG_ISR_TYPE_ENC  2
//
//#define JPEG_DRV_ENC_INT_STATUS_DONE   0x01
//#define JPEG_DRV_ENC_INT_STATUS_STALL  0x02
//
//#define JPEG_HW_DRV_NONE         0x00
//#define JPEG_HW_DRV_ENCODER_BIT  0x01
//#define JPEG_HW_DRV_DECODER_BIT  0x02

#endif /// __MT6589_JPEG_REG_H__
