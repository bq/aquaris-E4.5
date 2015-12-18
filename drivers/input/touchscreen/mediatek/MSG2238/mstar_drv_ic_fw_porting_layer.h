////////////////////////////////////////////////////////////////////////////////
// (c) MStar Semiconductor, Inc. 2006-2014
////////////////////////////////////////////////////////////////////////////////

/**
 *
 * @file    mstar_drv_ic_fw_porting_layer.h
 *
 * @brief   This file defines the interface of touch screen
 *
 * @version v2.3.0.0
 *
 */

#ifndef __MSTAR_DRV_IC_FW_PORTING_LAYER_H__
#define __MSTAR_DRV_IC_FW_PORTING_LAYER_H__

/*--------------------------------------------------------------------------*/
/* INCLUDE FILE                                                             */
/*--------------------------------------------------------------------------*/

#include "mstar_drv_common.h"
#if defined(CONFIG_ENABLE_CHIP_MSG26XXM)
#include "mstar_drv_mutual_fw_control.h"
#ifdef CONFIG_ENABLE_ITO_MP_TEST
#include "mstar_drv_mutual_mp_test.h"
#endif //CONFIG_ENABLE_ITO_MP_TEST
#elif defined(CONFIG_ENABLE_CHIP_MSG21XXA) || defined(CONFIG_ENABLE_CHIP_MSG22XX)
#include "mstar_drv_self_fw_control.h"
#ifdef CONFIG_ENABLE_ITO_MP_TEST
#include "mstar_drv_self_mp_test.h"
#endif //CONFIG_ENABLE_ITO_MP_TEST
#endif

/*--------------------------------------------------------------------------*/
/* PREPROCESSOR CONSTANT DEFINITION                                         */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* GLOBAL FUNCTION DECLARATION                                              */
/*--------------------------------------------------------------------------*/

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
extern void DrvIcFwLyrOpenGestureWakeup(u16 nWakeupMode);
extern void DrvIcFwLyrCloseGestureWakeup(void);
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#ifdef CONFIG_ENABLE_FIRMWARE_DATA_LOG
extern u16 DrvIcFwLyrChangeFirmwareMode(u16 nMode);
extern void DrvIcFwLyrGetFirmwareInfo(FirmwareInfo_t *pInfo);
#if defined(CONFIG_ENABLE_CHIP_MSG26XXM)
extern u16 DrvIcFwLyrGetFirmwareMode(void);
#endif //CONFIG_ENABLE_CHIP_MSG26XXM
extern void DrvIcFwLyrRestoreFirmwareModeToLogDataMode(void);
#endif //CONFIG_ENABLE_FIRMWARE_DATA_LOG

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
extern void DrvIcFwLyrCheckFirmwareUpdateBySwId(void);
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

extern u8 DrvIcFwLyrGetChipType(void);
extern void DrvIcFwLyrGetCustomerFirmwareVersion(u16 *pMajor, u16 *pMinor, u8 **ppVersion);
extern void DrvIcFwLyrGetPlatformFirmwareVersion(u8 **ppVersion);
extern void DrvIcFwLyrHandleFingerTouch(u8 *pPacket, u16 nLength);
extern u32 DrvIcFwLyrIsRegisterFingerTouchInterruptHandler(void);
extern s32 DrvIcFwLyrUpdateFirmware(u8 szFwData[][1024], EmemType_e eEmemType);

#ifdef CONFIG_ENABLE_ITO_MP_TEST
extern void DrvIcFwLyrCreateMpTestWorkQueue(void);
extern void DrvIcFwLyrScheduleMpTestWork(ItoTestMode_e eItoTestMode);
extern void DrvIcFwLyrGetMpTestDataLog(ItoTestMode_e eItoTestMode, u8 *pDataLog, u32 *pLength);
extern void DrvIcFwLyrGetMpTestFailChannel(ItoTestMode_e eItoTestMode, u8 *pFailChannel, u32 *pFailChannelCount);
extern s32 DrvIcFwLyrGetMpTestResult(void);
#endif //CONFIG_ENABLE_ITO_MP_TEST
        
#endif  /* __MSTAR_DRV_IC_FW_PORTING_LAYER_H__ */
