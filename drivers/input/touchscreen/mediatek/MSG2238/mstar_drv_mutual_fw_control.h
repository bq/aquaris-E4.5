////////////////////////////////////////////////////////////////////////////////
// (c) MStar Semiconductor, Inc. 2006-2014
////////////////////////////////////////////////////////////////////////////////

/**
 *
 * @file    mstar_drv_mutual_fw_control.h
 *
 * @brief   This file defines the interface of touch screen
 *
 * @version v2.3.0.0
 *
 */

#ifndef __MSTAR_DRV_MUTUAL_FW_CONTROL_H__
#define __MSTAR_DRV_MUTUAL_FW_CONTROL_H__

/*--------------------------------------------------------------------------*/
/* INCLUDE FILE                                                             */
/*--------------------------------------------------------------------------*/

#include "mstar_drv_common.h"

#if defined(CONFIG_ENABLE_CHIP_MSG26XXM)

/*--------------------------------------------------------------------------*/
/* PREPROCESSOR CONSTANT DEFINITION                                         */
/*--------------------------------------------------------------------------*/

#define DEMO_MODE_PACKET_LENGTH    (43)
#define MAX_TOUCH_NUM           (5)     //10

#define MSG26XXM_FIRMWARE_MAIN_BLOCK_SIZE (32) //32K
#define MSG26XXM_FIRMWARE_INFO_BLOCK_SIZE (8) //8K
#define MSG26XXM_FIRMWARE_WHOLE_SIZE (MSG26XXM_FIRMWARE_MAIN_BLOCK_SIZE+MSG26XXM_FIRMWARE_INFO_BLOCK_SIZE) //40K


#ifdef CONFIG_ENABLE_FIRMWARE_DATA_LOG
#define FIRMWARE_MODE_DEMO_MODE    (0x0005)
#define FIRMWARE_MODE_DEBUG_MODE   (0x0105)
#endif //CONFIG_ENABLE_FIRMWARE_DATA_LOG

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
#define UPDATE_FIRMWARE_RETRY_COUNT (2)
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

/*--------------------------------------------------------------------------*/
/* DATA TYPE DEFINITION                                                     */
/*--------------------------------------------------------------------------*/

typedef struct
{
    u16 nId;
    u16 nX;
    u16 nY;
    u16 nP;
} TouchPoint_t;

/// max 80+1+1 = 82 bytes
typedef struct
{
    u8 nCount;
    u8 nKeyCode;
    TouchPoint_t tPoint[MAX_TOUCH_NUM];
} TouchInfo_t;

#ifdef CONFIG_ENABLE_FIRMWARE_DATA_LOG

typedef struct
{
    u8 nType;
    u8 nLogModePacketHeader;
    u8 nMy;
    u8 nMx;
    u8 nSd;
    u8 nSs;
    u16 nLogModePacketLength;
} FirmwareInfo_t;

#endif //CONFIG_ENABLE_FIRMWARE_DATA_LOG


#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
/*
 * Note.
 * 0x0000 and 0xFFFF are not allowed to be defined as SW ID.
 * SW_ID_UNDEFINED is a reserved enum value, do not delete it or modify it.
 * Please modify the SW ID of the below enum value depends on the TP vendor that you are using.
 */
typedef enum {
    MSG26XXM_SW_ID_XXXX = 0x0001,  
    MSG26XXM_SW_ID_YYYY = 0x0002,  
    MSG26XXM_SW_ID_UNDEFINED
} Msg26xxmSwId_e;
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

/*--------------------------------------------------------------------------*/
/* GLOBAL FUNCTION DECLARATION                                              */
/*--------------------------------------------------------------------------*/

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
extern void DrvFwCtrlOpenGestureWakeup(u16 nMode);
extern void DrvFwCtrlCloseGestureWakeup(void);
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
extern void DrvFwCtrlCheckFirmwareUpdateBySwId(void);
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

#ifdef CONFIG_ENABLE_FIRMWARE_DATA_LOG
extern u16 DrvFwCtrlChangeFirmwareMode(u16 nMode);
extern void DrvFwCtrlGetFirmwareInfo(FirmwareInfo_t *pInfo);
extern u16 DrvFwCtrlGetFirmwareMode(void);
extern void DrvFwCtrlRestoreFirmwareModeToLogDataMode(void);
#endif //CONFIG_ENABLE_FIRMWARE_DATA_LOG

extern u8 DrvFwCtrlGetChipType(void);
extern void DrvFwCtrlGetCustomerFirmwareVersion(u16 *pMajor, u16 *pMinor, u8 **ppVersion);
extern void DrvFwCtrlGetPlatformFirmwareVersion(u8 **ppVersion);
extern void DrvFwCtrlHandleFingerTouch(void);
extern s32 DrvFwCtrlUpdateFirmware(u8 szFwData[][1024], EmemType_e eEmemType);

#endif //CONFIG_ENABLE_CHIP_MSG26XXM
        
#endif  /* __MSTAR_DRV_MUTUAL_FW_CONTROL_H__ */
