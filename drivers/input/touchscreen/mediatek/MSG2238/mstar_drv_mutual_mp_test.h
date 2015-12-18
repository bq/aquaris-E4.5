////////////////////////////////////////////////////////////////////////////////
// (c) MStar Semiconductor, Inc. 2006-2014
////////////////////////////////////////////////////////////////////////////////

/**
 *
 * @file    mstar_drv_mutual_mp_test.h
 *
 * @brief   This file defines the interface of touch screen
 *
 * @version v2.3.0.0
 *
 */

#ifndef __MSTAR_DRV_MUTUAL_MP_TEST_H__
#define __MSTAR_DRV_MUTUAL_MP_TEST_H__

/*--------------------------------------------------------------------------*/
/* INCLUDE FILE                                                             */
/*--------------------------------------------------------------------------*/

#include "mstar_drv_common.h"

#if defined(CONFIG_ENABLE_CHIP_MSG26XXM)
#ifdef CONFIG_ENABLE_ITO_MP_TEST

/*--------------------------------------------------------------------------*/
/* PREPROCESSOR CONSTANT DEFINITION                                         */
/*--------------------------------------------------------------------------*/

#define MAX_CHANNEL_NUM  38
#define MAX_CHANNEL_DRV  28
#define MAX_CHANNEL_SEN  14
#define MAX_MUTUAL_NUM  (MAX_CHANNEL_DRV * MAX_CHANNEL_SEN)
#define ANA3_MUTUAL_CSUB_NUMBER (192) //192 = 14 * 13 + 10
#define ANA4_MUTUAL_CSUB_NUMBER (MAX_MUTUAL_NUM - ANA3_MUTUAL_CSUB_NUMBER) //200 = 392 - 192
#define FILTER1_MUTUAL_DELTA_C_NUMBER (190) //190 = (6 * 14 + 11) * 2
#define FILTER2_MUTUAL_DELTA_C_NUMBER (594) //594 = (MAX_MUTUAL_NUM - (6 * 14 + 11)) * 2

#define FIR_THRESHOLD    6553
#define FIR_RATIO    50 //25

#define CTP_MP_TEST_RETRY_COUNT (3)

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
#endif //CONFIG_ENABLE_CHIP_MSG26XXM

#endif  /* __MSTAR_DRV_MUTUAL_MP_TEST_H__ */