////////////////////////////////////////////////////////////////////////////////
// (c) MStar Semiconductor, Inc. 2006-2014
////////////////////////////////////////////////////////////////////////////////

/**
 *
 * @file    mstar_drv_self_fw_control.h
 *
 * @brief   This file defines the interface of touch screen
 *
 * @version v2.3.0.0
 *
 */

#ifndef __MSTAR_DRV_SELF_FW_CONTROL_H__
#define __MSTAR_DRV_SELF_FW_CONTROL_H__

/*--------------------------------------------------------------------------*/
/* INCLUDE FILE                                                             */
/*--------------------------------------------------------------------------*/

#include "mstar_drv_common.h"

#if defined(CONFIG_ENABLE_CHIP_MSG21XXA) || defined(CONFIG_ENABLE_CHIP_MSG22XX)

/*--------------------------------------------------------------------------*/
/* COMPILE OPTION DEFINITION                                                */
/*--------------------------------------------------------------------------*/

//#define CONFIG_SWAP_X_Y

//#define CONFIG_REVERSE_X
//#define CONFIG_REVERSE_Y

/*--------------------------------------------------------------------------*/
/* PREPROCESSOR CONSTANT DEFINITION                                         */
/*--------------------------------------------------------------------------*/

#define DEMO_MODE_PACKET_LENGTH    (8)
#define MAX_TOUCH_NUM           (2)     

#define MSG21XXA_FIRMWARE_MAIN_BLOCK_SIZE (32) //32K
#define MSG21XXA_FIRMWARE_INFO_BLOCK_SIZE (1)  //1K
#define MSG21XXA_FIRMWARE_WHOLE_SIZE (MSG21XXA_FIRMWARE_MAIN_BLOCK_SIZE+MSG21XXA_FIRMWARE_INFO_BLOCK_SIZE) //33K

#define MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE (48)  //48K
#define MSG22XX_FIRMWARE_INFO_BLOCK_SIZE (512) //512Byte


#ifdef CONFIG_ENABLE_FIRMWARE_DATA_LOG
#define FIRMWARE_MODE_DEMO_MODE      (0x00)
#define FIRMWARE_MODE_DEBUG_MODE     (0x01)
#define FIRMWARE_MODE_RAW_DATA_MODE  (0x02)
#endif //CONFIG_ENABLE_FIRMWARE_DATA_LOG

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
#define UPDATE_FIRMWARE_RETRY_COUNT (2)
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

/*--------------------------------------------------------------------------*/
/* DATA TYPE DEFINITION                                                     */
/*--------------------------------------------------------------------------*/

typedef struct
{
    u16 nX;
    u16 nY;
} TouchPoint_t;

typedef struct
{
    u8 nTouchKeyMode;
    u8 nTouchKeyCode;
    u8 nFingerNum;
    TouchPoint_t tPoint[MAX_TOUCH_NUM];
} TouchInfo_t;

#ifdef CONFIG_ENABLE_FIRMWARE_DATA_LOG

typedef struct
{
    u8 nFirmwareMode;
    u8 nLogModePacketHeader;
    u16 nLogModePacketLength;
    u8 nIsCanChangeFirmwareMode;
} FirmwareInfo_t;

#endif //CONFIG_ENABLE_FIRMWARE_DATA_LOG

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
/*
 * Note.
 * The following is sw id enum definition for MSG22XX.
 * 0x0000 and 0xFFFF are not allowed to be defined as SW ID.
 * SW_ID_UNDEFINED is a reserved enum value, do not delete it or modify it.
 * Please modify the SW ID of the below enum value depends on the TP vendor that you are using.
 */
typedef enum {
    MSG22XX_SW_ID_DJ= 0x0002,
    MSG22XX_SW_ID_HRC= 0x0009,
    MSG22XX_SW_ID_UNDEFINED
} Msg22xxSwId_e;


/*
 * Note.
 * The following is sw id enum definition for MSG21XXA.
 * SW_ID_UNDEFINED is a reserved enum value, do not delete it or modify it.
 * Please modify the SW ID of the below enum value depends on the TP vendor that you are using.
 */
typedef enum {
    MSG21XXA_SW_ID_XXXX = 0,  
    MSG21XXA_SW_ID_YYYY,
    MSG21XXA_SW_ID_UNDEFINED
} Msg21xxaSwId_e;
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

/*--------------------------------------------------------------------------*/
/* GLOBAL FUNCTION DECLARATION                                              */
/*--------------------------------------------------------------------------*/

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
extern void DrvFwCtrlOpenGestureWakeup(u16 nMode);
extern void DrvFwCtrlCloseGestureWakeup(void);
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#ifdef CONFIG_ENABLE_FIRMWARE_DATA_LOG
extern u16 DrvFwCtrlChangeFirmwareMode(u16 nMode);        
extern void DrvFwCtrlGetFirmwareInfo(FirmwareInfo_t *pInfo);
extern void DrvFwCtrlRestoreFirmwareModeToLogDataMode(void);
#endif //CONFIG_ENABLE_FIRMWARE_DATA_LOG

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
extern void DrvFwCtrlCheckFirmwareUpdateBySwId(void);
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

extern u8 DrvFwCtrlGetChipType(void);
extern void DrvFwCtrlGetCustomerFirmwareVersion(u16 *pMajor, u16 *pMinor, u8 **ppVersion);
extern void DrvFwCtrlGetPlatformFirmwareVersion(u8 **ppVersion);
extern void DrvFwCtrlHandleFingerTouch(void);
extern s32 DrvFwCtrlUpdateFirmware(u8 szFwData[][1024], EmemType_e eEmemType);

#endif //CONFIG_ENABLE_CHIP_MSG21XXA || CONFIG_ENABLE_CHIP_MSG22XX
        
#endif  /* __MSTAR_DRV_SELF_FW_CONTROL_H__ */
