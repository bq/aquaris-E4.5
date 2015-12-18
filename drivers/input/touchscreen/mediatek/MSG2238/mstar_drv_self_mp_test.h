////////////////////////////////////////////////////////////////////////////////
// (c) MStar Semiconductor, Inc. 2006-2014
////////////////////////////////////////////////////////////////////////////////

/**
 *
 * @file    mstar_drv_self_mp_test.h
 *
 * @brief   This file defines the interface of touch screen
 *
 * @version v2.3.0.0
 *
 */

#ifndef __MSTAR_DRV_SELF_MP_TEST_H__
#define __MSTAR_DRV_SELF_MP_TEST_H__

/*--------------------------------------------------------------------------*/
/* INCLUDE FILE                                                             */
/*--------------------------------------------------------------------------*/

#include "mstar_drv_common.h"

#if defined(CONFIG_ENABLE_CHIP_MSG21XXA) || defined(CONFIG_ENABLE_CHIP_MSG22XX)
#ifdef CONFIG_ENABLE_ITO_MP_TEST

/*--------------------------------------------------------------------------*/
/* PREPROCESSOR CONSTANT DEFINITION                                         */
/*--------------------------------------------------------------------------*/

#define CTP_MP_TEST_RETRY_COUNT (3)


#define OPEN_TEST_NON_BORDER_AREA_THRESHOLD (35) // range : 25~60
#define OPEN_TEST_BORDER_AREA_THRESHOLD     (40) // range : 25~60

#define	SHORT_TEST_THRESHOLD                (3500)

#define	MP_TEST_MODE_OPEN_TEST              (0x01)
#define	MP_TEST_MODE_SHORT_TEST             (0x02)

#define MAX_CHANNEL_NUM   (48)

#if defined(CONFIG_ENABLE_CHIP_MSG21XXA)
#define PIN_GUARD_RING    (46) 
#define GPO_SETTING_SIZE  (3)  
#elif defined(CONFIG_ENABLE_CHIP_MSG22XX)
#define RIU_BASE_ADDR       (0)   
#define RIU_WRITE_LENGTH    (144)  
#define CSUB_REF            (0) //(18)   
#define CSUB_REF_MAX        (0x3F) 

#define MAX_SUBFRAME_NUM    (24)
#define MAX_AFE_NUM         (4)
#endif

#define REG_INTR_FIQ_MASK           (0x04)          
#define FIQ_E_FRAME_READY_MASK      (1 << 8)


/*--------------------------------------------------------------------------*/
/* PREPROCESSOR MACRO DEFINITION                                            */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* DATA TYPE DEFINITION                                                     */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* GLOBAL VARIABLE DEFINITION                                               */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* GLOBAL FUNCTION DECLARATION                                              */
/*--------------------------------------------------------------------------*/

extern void DrvMpTestCreateMpTestWorkQueue(void);
extern void DrvMpTestGetTestDataLog(ItoTestMode_e eItoTestMode, u8 *pDataLog, u32 *pLength);
extern void DrvMpTestGetTestFailChannel(ItoTestMode_e eItoTestMode, u8 *pFailChannel, u32 *pFailChannelCount);
extern s32 DrvMpTestGetTestResult(void);
extern void DrvMpTestScheduleMpTestWork(ItoTestMode_e eItoTestMode);

#endif //CONFIG_ENABLE_ITO_MP_TEST
#endif //CONFIG_ENABLE_CHIP_MSG21XXA || CONFIG_ENABLE_CHIP_MSG22XX

#endif  /* __MSTAR_DRV_SELF_MP_TEST_H__ */