 
#ifndef _MSDC_CFG_H_
#define _MSDC_CFG_H_

/*--------------------------------------------------------------------------*/
/* Common Definition                                                        */
/*--------------------------------------------------------------------------*/

#ifdef MACH_FPGA
#define FPGA_PLATFORM
#endif

#define MMC_DEBUG               (0)
#define MSDC_DEBUG              (0)

#define MSDC_USE_REG_OPS_DUMP   (0)
#ifdef FPGA_PLATFORM
#define MSDC_USE_DMA_MODE       (1)
#else
#define MSDC_USE_DMA_MODE       (1)
#endif

#if (1 == MSDC_USE_DMA_MODE)
//#define MSDC_DMA_BOUNDARY_LIMITAION
#endif

//#define FEATURE_MMC_UHS1
//#define FEATURE_MMC_BOOT_MODE
#define FEATURE_MMC_WR_TUNING
#define FEATURE_MMC_RD_TUNING
#define FEATURE_MMC_CM_TUNING
#define MSDC_TUNE_LOG	(1)
#if MSDC_DEBUG
#define MSG_DEBUG
#endif

#endif /* end of _MSDC_CFG_H_ */



