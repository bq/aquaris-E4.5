 
#ifndef _MSDC_CFG_H_
#define _MSDC_CFG_H_

#include "platform.h"

/*--------------------------------------------------------------------------*/
/* Common Definition                                                        */
/*--------------------------------------------------------------------------*/
#if CFG_FPGA_PLATFORM
#define FPGA_PLATFORM
#endif

#define MMC_DEBUG               (0)
#define MSDC_DEBUG              (0)

#define MSDC_USE_REG_OPS_DUMP   (0)
#ifdef SLT      //#ifdef FPGA_PLATFORM       
#define MSDC_USE_DMA_MODE       (1)
#else
#define MSDC_USE_DMA_MODE       (0)
#endif

//#define MSDC0_BASE              (0xC1220000)
//#define MSDC1_BASE              (0xC1230000)
//#define MSDC2_BASE              (0xC1250000)
//#define MSDC3_BASE              (0xC1240000)

//#define FEATURE_MMC_UHS1
//#define FEATURE_MMC_BOOT_MODE
//#define FEATURE_MMC_WR_TUNING
//#define FEATURE_MMC_CARD_DETECT
#define FEATURE_MMC_STRIPPED
#define FEATURE_MMC_RD_TUNING
#define FEATURE_MMC_CM_TUNING
#define MSDC_TUNE_LOG	(1)
//#define FEATURE_MSDC_ENH_DMA_MODE

#if MSDC_DEBUG
#define MSG_DEBUG
#endif

#endif /* end of _MSDC_CFG_H_ */



