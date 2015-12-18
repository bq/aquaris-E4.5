////////////////////////////////////////////////////////////////////////////////
// (c) MStar Semiconductor, Inc. 2006-2014
////////////////////////////////////////////////////////////////////////////////

/**
 *
 * @file    mstar_drv_mutual_fw_control.c
 *
 * @brief   This file defines the interface of touch screen
 *
 * @version v2.3.0.0
 *
 */

/*=============================================================*/
// INCLUDE FILE
/*=============================================================*/

#include "mstar_drv_mutual_fw_control.h"
#include "mstar_drv_utility_adaption.h"
#include "mstar_drv_platform_porting_layer.h"

#if defined(CONFIG_ENABLE_CHIP_MSG26XXM)

/*=============================================================*/
// EXTERN VARIABLE DECLARATION
/*=============================================================*/

#ifdef CONFIG_TP_HAVE_KEY
extern const int g_TpVirtualKey[];

#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
extern const int g_TpVirtualKeyDimLocal[][4];
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
#endif //CONFIG_TP_HAVE_KEY

extern struct input_dev *g_InputDevice;

extern u8 g_FwData[94][1024];
extern u32 g_FwDataCount;

extern struct mutex g_Mutex;

#ifdef CONFIG_ENABLE_FIRMWARE_DATA_LOG
extern struct kobject *g_TouchKObj;
#endif //CONFIG_ENABLE_FIRMWARE_DATA_LOG

/*=============================================================*/
// LOCAL VARIABLE DEFINITION
/*=============================================================*/

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
/*
 * Note.
 * Please modify the name of the below .h depends on the vendor TP that you are using.
 */
#include "msg2xxx_xxxx_update_bin.h"
#include "msg2xxx_yyyy_update_bin.h"

static u32 _gUpdateRetryCount = UPDATE_FIRMWARE_RETRY_COUNT;
static u32 _gIsUpdateInfoBlockFirst = 0;
static struct work_struct _gUpdateFirmwareBySwIdWork;
static struct workqueue_struct *_gUpdateFirmwareBySwIdWorkQueue = NULL;
static u8 _gIsUpdateFirmware = 0x00;
static u8 _gTempData[1024]; 
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
static u16 _gGestureWakeupValue = 0;
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

/*=============================================================*/
// GLOBAL VARIABLE DEFINITION
/*=============================================================*/

u8 g_ChipType = 0;
u8 g_DemoModePacket[DEMO_MODE_PACKET_LENGTH] = {0};

#ifdef CONFIG_ENABLE_FIRMWARE_DATA_LOG
FirmwareInfo_t g_FirmwareInfo;

u8 *g_LogModePacket = NULL;
u16 g_FirmwareMode = FIRMWARE_MODE_DEMO_MODE;
#endif //CONFIG_ENABLE_FIRMWARE_DATA_LOG

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
u8 _gGestureWakeupPacket[GESTURE_WAKEUP_PACKET_LENGTH] = {0};

u16 g_GestureWakeupMode = 0x0000;
u8 g_GestureWakeupFlag = 0;
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

/*=============================================================*/
// LOCAL FUNCTION DEFINITION
/*=============================================================*/

static s32 _DrvFwCtrlParsePacket(u8 *pPacket, u16 nLength, TouchInfo_t *pInfo)
{
    u32 i;
    u8 nCheckSum = 0;
    u32 nX = 0, nY = 0;

    DBG("*** %s() ***\n", __func__);

    nCheckSum = DrvCommonCalculateCheckSum(&pPacket[0], (nLength-1));
    DBG("checksum : [%x] == [%x]? \n", pPacket[nLength-1], nCheckSum);

    if (pPacket[nLength-1] != nCheckSum)
    {
        DBG("WRONG CHECKSUM\n");
        return -1;
    }

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
    if (g_GestureWakeupFlag == 1)
    {
        DBG("received raw data from touch panel as following:\n");
        DBG("pPacket[0]=%x \n pPacket[1]=%x pPacket[2]=%x pPacket[3]=%x pPacket[4]=%x pPacket[5]=%x \n", \
                pPacket[0], pPacket[1], pPacket[2], pPacket[3], pPacket[4], pPacket[5]);

        if (pPacket[0] == 0xA7 && pPacket[1] == 0x00 && pPacket[2] == 0x06 && pPacket[3] == PACKET_TYPE_GESTURE_WAKEUP) 
        {
            u8 nWakeupMode = pPacket[4];

            DBG("nWakeupMode = 0x%x\n", nWakeupMode);

            switch (nWakeupMode)
            {
                case 0x58:
                    _gGestureWakeupValue = GESTURE_WAKEUP_MODE_DOUBLE_CLICK_FLAG;

                    DBG("Light up screen by DOUBLE_CLICK gesture wakeup.\n");

                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x60:
                    _gGestureWakeupValue = GESTURE_WAKEUP_MODE_UP_DIRECT_FLAG;
                    
                    DBG("Light up screen by UP_DIRECT gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_UP, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_UP, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x61:
                    _gGestureWakeupValue = GESTURE_WAKEUP_MODE_DOWN_DIRECT_FLAG;

                    DBG("Light up screen by DOWN_DIRECT gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_DOWN, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_DOWN, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x62:
                    _gGestureWakeupValue = GESTURE_WAKEUP_MODE_LEFT_DIRECT_FLAG;

                    DBG("Light up screen by LEFT_DIRECT gesture wakeup.\n");

//                  input_report_key(g_InputDevice, KEY_LEFT, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_LEFT, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x63:
                    _gGestureWakeupValue = GESTURE_WAKEUP_MODE_RIGHT_DIRECT_FLAG;

                    DBG("Light up screen by RIGHT_DIRECT gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_RIGHT, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_RIGHT, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x64:
                    _gGestureWakeupValue = GESTURE_WAKEUP_MODE_m_CHARACTER_FLAG;

                    DBG("Light up screen by m_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_M, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_M, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x65:
                    _gGestureWakeupValue = GESTURE_WAKEUP_MODE_W_CHARACTER_FLAG;

                    DBG("Light up screen by W_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_W, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_W, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x66:
                    _gGestureWakeupValue = GESTURE_WAKEUP_MODE_C_CHARACTER_FLAG;

                    DBG("Light up screen by C_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_C, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_C, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x67:
                    _gGestureWakeupValue = GESTURE_WAKEUP_MODE_e_CHARACTER_FLAG;

                    DBG("Light up screen by e_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_E, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_E, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x68:
                    _gGestureWakeupValue = GESTURE_WAKEUP_MODE_V_CHARACTER_FLAG;

                    DBG("Light up screen by V_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_V, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_V, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x69:
                    _gGestureWakeupValue = GESTURE_WAKEUP_MODE_O_CHARACTER_FLAG;

                    DBG("Light up screen by O_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_O, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_O, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x6A:
                    _gGestureWakeupValue = GESTURE_WAKEUP_MODE_S_CHARACTER_FLAG;

                    DBG("Light up screen by S_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_S, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_S, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x6B:
                    _gGestureWakeupValue = GESTURE_WAKEUP_MODE_Z_CHARACTER_FLAG;

                    DBG("Light up screen by Z_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_Z, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_Z, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                default:
                    _gGestureWakeupValue = 0;

                    DBG("Un-supported gesture wakeup mode. Please check your device driver code.\n");
                    break;		
            }

            DBG("_gGestureWakeupValue = 0x%x\n", _gGestureWakeupValue);
        }
        else
        {
            DBG("gesture wakeup packet format is incorrect.\n");
        }
        
        return -1;
    }
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

    DBG("received raw data from touch panel as following:\n");
    DBG("pPacket[0]=%x \n pPacket[1]=%x pPacket[2]=%x pPacket[3]=%x pPacket[4]=%x \n pPacket[5]=%x pPacket[6]=%x pPacket[7]=%x pPacket[8]=%x \n", \
                pPacket[0], pPacket[1], pPacket[2], pPacket[3], pPacket[4], pPacket[5], pPacket[6], pPacket[7], pPacket[8]);

#ifdef CONFIG_ENABLE_FIRMWARE_DATA_LOG
    if (g_FirmwareMode == FIRMWARE_MODE_DEMO_MODE && pPacket[0] != 0x5A)
    {
        DBG("WRONG DEMO MODE HEADER\n");
        return -1;
    }
    else if (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE && pPacket[0] != 0xA5 && pPacket[0] != 0xAB && (pPacket[0] != 0xA7 && pPacket[3] != PACKET_TYPE_TOOTH_PATTERN))
    {
        DBG("WRONG DEBUG MODE HEADER\n");
        return -1;
    }
#else
    if (pPacket[0] != 0x5A)
    {
        DBG("WRONG DEMO MODE HEADER\n");
        return -1;
    }
#endif //CONFIG_ENABLE_FIRMWARE_DATA_LOG

    // Process raw data...
    if (pPacket[0] == 0x5A)
    {
        for (i = 0; i < MAX_TOUCH_NUM; i ++)
        {
            if ((pPacket[(4*i)+1] == 0xFF) && (pPacket[(4*i)+2] == 0xFF) && (pPacket[(4*i)+3] == 0xFF))
            {
                continue;
            }
		
            nX = (((pPacket[(4*i)+1] & 0xF0) << 4) | (pPacket[(4*i)+2]));
            nY = (((pPacket[(4*i)+1] & 0x0F) << 8) | (pPacket[(4*i)+3]));
		
            pInfo->tPoint[pInfo->nCount].nX = nX * TOUCH_SCREEN_X_MAX / TPD_WIDTH;
            pInfo->tPoint[pInfo->nCount].nY = nY * TOUCH_SCREEN_Y_MAX / TPD_HEIGHT;
            pInfo->tPoint[pInfo->nCount].nP = pPacket[4*(i+1)];
            pInfo->tPoint[pInfo->nCount].nId = i;
		
            DBG("[x,y]=[%d,%d]\n", nX, nY);
            DBG("point[%d] : (%d,%d) = %d\n", pInfo->tPoint[pInfo->nCount].nId, pInfo->tPoint[pInfo->nCount].nX, pInfo->tPoint[pInfo->nCount].nY, pInfo->tPoint[pInfo->nCount].nP);
		
            pInfo->nCount ++;
        }
    }
#ifdef CONFIG_ENABLE_FIRMWARE_DATA_LOG
    else if (pPacket[0] == 0xA5 || pPacket[0] == 0xAB)
    {
        for (i = 0; i < MAX_TOUCH_NUM; i ++)
        {
            if ((pPacket[(3*i)+4] == 0xFF) && (pPacket[(3*i)+5] == 0xFF) && (pPacket[(3*i)+6] == 0xFF))
            {
                continue;
            }
		
            nX = (((pPacket[(3*i)+4] & 0xF0) << 4) | (pPacket[(3*i)+5]));
            nY = (((pPacket[(3*i)+4] & 0x0F) << 8) | (pPacket[(3*i)+6]));

            pInfo->tPoint[pInfo->nCount].nX = nX * TOUCH_SCREEN_X_MAX / TPD_WIDTH;
            pInfo->tPoint[pInfo->nCount].nY = nY * TOUCH_SCREEN_Y_MAX / TPD_HEIGHT;
            pInfo->tPoint[pInfo->nCount].nP = 1;
            pInfo->tPoint[pInfo->nCount].nId = i;
		
            DBG("[x,y]=[%d,%d]\n", nX, nY);
            DBG("point[%d] : (%d,%d) = %d\n", pInfo->tPoint[pInfo->nCount].nId, pInfo->tPoint[pInfo->nCount].nX, pInfo->tPoint[pInfo->nCount].nY, pInfo->tPoint[pInfo->nCount].nP);
		
            pInfo->nCount ++;
        }

        // Notify android application to retrieve debug mode packet from device driver by sysfs.   
        if (g_TouchKObj != NULL)
        {
            char *pEnvp[2];
            s32 nRetVal = 0;  

            pEnvp[0] = "STATUS=GET_DEBUG_MODE_PACKET";  
            pEnvp[1] = NULL;  
            
            nRetVal = kobject_uevent_env(g_TouchKObj, KOBJ_CHANGE, pEnvp); 
            DBG("kobject_uevent_env() nRetVal = %d\n", nRetVal);
        }
    }
    else if (pPacket[0] == 0xA7 && pPacket[3] == PACKET_TYPE_TOOTH_PATTERN)
    {
        for (i = 0; i < MAX_TOUCH_NUM; i ++)
        {
            if ((pPacket[(3*i)+5] == 0xFF) && (pPacket[(3*i)+6] == 0xFF) && (pPacket[(3*i)+7] == 0xFF))
            {
                continue;
            }
		
            nX = (((pPacket[(3*i)+5] & 0xF0) << 4) | (pPacket[(3*i)+6]));
            nY = (((pPacket[(3*i)+5] & 0x0F) << 8) | (pPacket[(3*i)+7]));

            pInfo->tPoint[pInfo->nCount].nX = nX * TOUCH_SCREEN_X_MAX / TPD_WIDTH;
            pInfo->tPoint[pInfo->nCount].nY = nY * TOUCH_SCREEN_Y_MAX / TPD_HEIGHT;
            pInfo->tPoint[pInfo->nCount].nP = 1;
            pInfo->tPoint[pInfo->nCount].nId = i;
		
            DBG("[x,y]=[%d,%d]\n", nX, nY);
            DBG("point[%d] : (%d,%d) = %d\n", pInfo->tPoint[pInfo->nCount].nId, pInfo->tPoint[pInfo->nCount].nX, pInfo->tPoint[pInfo->nCount].nY, pInfo->tPoint[pInfo->nCount].nP);
		
            pInfo->nCount ++;
        }

        // Notify android application to retrieve debug mode packet from device driver by sysfs.   
        if (g_TouchKObj != NULL)
        {
            char *pEnvp[2];
            s32 nRetVal = 0;  

            pEnvp[0] = "STATUS=GET_DEBUG_MODE_PACKET";  
            pEnvp[1] = NULL;  
            
            nRetVal = kobject_uevent_env(g_TouchKObj, KOBJ_CHANGE, pEnvp); 
            DBG("kobject_uevent_env() nRetVal = %d\n", nRetVal);
        }
    }
#endif //CONFIG_ENABLE_FIRMWARE_DATA_LOG

#ifdef CONFIG_TP_HAVE_KEY
    if (pPacket[0] == 0x5A)
    {
        u8 nButton = pPacket[nLength-2]; //Since the key value is stored in 0th~3th bit of variable "button", we can only retrieve 0th~3th bit of it. 

//        if (nButton)
        if (nButton != 0xFF)
        {
            DBG("button = %x\n", nButton);

            for (i = 0; i < MAX_KEY_NUM; i ++)
            {
                if ((nButton & (1<<i)) == (1<<i))
                {
                    if (pInfo->nKeyCode == 0)
                    {
                        pInfo->nKeyCode = i;

                        DBG("key[%d]=%d ...\n", i, g_TpVirtualKey[i]);

#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
                        pInfo->nKeyCode = 0xFF;
                        pInfo->nCount = 1;
                        pInfo->tPoint[0].nX = g_TpVirtualKeyDimLocal[i][0];
                        pInfo->tPoint[0].nY = g_TpVirtualKeyDimLocal[i][1];
                        pInfo->tPoint[0].nP = 1;
                        pInfo->tPoint[0].nId = 0;
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
                    }
                    else
                    {
                        /// if pressing multi-key => no report
                        pInfo->nKeyCode = 0xFF;
                    }
                }
            }
        }
        else
        {
            pInfo->nKeyCode = 0xFF;
        }
    }
#ifdef CONFIG_ENABLE_FIRMWARE_DATA_LOG
    else if (pPacket[0] == 0xA5 || pPacket[0] == 0xAB || (pPacket[0] == 0xA7 && pPacket[3] == PACKET_TYPE_TOOTH_PATTERN))
    {
    		// TODO : waiting for firmware define the virtual key

        if (pPacket[0] == 0xA5)
        {
        	  // Do nothing	because of 0xA5 not define virtual key in the packet
        }
        else if (pPacket[0] == 0xAB || (pPacket[0] == 0xA7 && pPacket[3] == PACKET_TYPE_TOOTH_PATTERN))
        {
            u8 nButton = 0xFF;

            if (pPacket[0] == 0xAB)
            {
                nButton = pPacket[3]; // The pressed virtual key is stored in 4th byte for debug mode packet 0xAB.
            }
            else if (pPacket[0] == 0xA7 && pPacket[3] == PACKET_TYPE_TOOTH_PATTERN)
            {
                nButton = pPacket[4]; // The pressed virtual key is stored in 5th byte for debug mode packet 0xA7.
            }

            if (nButton != 0xFF)
            {
                DBG("button = %x\n", nButton);

                for (i = 0; i < MAX_KEY_NUM; i ++)
                {
                    if ((nButton & (1<<i)) == (1<<i))
                    {
                        if (pInfo->nKeyCode == 0)
                        {
                            pInfo->nKeyCode = i;

                            DBG("key[%d]=%d ...\n", i, g_TpVirtualKey[i]);

#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
                            pInfo->nKeyCode = 0xFF;
                            pInfo->nCount = 1;
                            pInfo->tPoint[0].nX = g_TpVirtualKeyDimLocal[i][0];
                            pInfo->tPoint[0].nY = g_TpVirtualKeyDimLocal[i][1];
                            pInfo->tPoint[0].nP = 1;
                            pInfo->tPoint[0].nId = 0;
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
                        }
                        else
                        {
                            /// if pressing multi-key => no report
                            pInfo->nKeyCode = 0xFF;
                        }
                    }
                }
            }
            else
            {
                pInfo->nKeyCode = 0xFF;
            }
        }
    }
#endif //CONFIG_ENABLE_FIRMWARE_DATA_LOG
#endif //CONFIG_TP_HAVE_KEY

    return 0;
}

static void _DrvFwCtrlStoreFirmwareData(u8 *pBuf, u32 nSize)
{
    u32 nCount = nSize / 1024;
    u32 i;

    DBG("*** %s() ***\n", __func__);

    if (nCount > 0) // nSize >= 1024
   	{
        for (i = 0; i < nCount; i ++)
        {
            memcpy(g_FwData[g_FwDataCount], pBuf+(i*1024), 1024);

            g_FwDataCount ++;
        }
    }
    else // nSize < 1024
    {
    		if (nSize > 0)
    		{
            memcpy(g_FwData[g_FwDataCount], pBuf, nSize);

            g_FwDataCount ++;
    		}
    }

    DBG("*** g_FwDataCount = %d ***\n", g_FwDataCount);

    if (pBuf != NULL)
    {
        DBG("*** buf[0] = %c ***\n", pBuf[0]);
    }
}

static s32 _DrvFwCtrlUpdateFirmwareCash(u8 szFwData[][1024], EmemType_e eEmemType)
{
    u32 i, j;
    u32 nCrcMain, nCrcMainTp;
    u32 nCrcInfo, nCrcInfoTp;
    u16 nRegData = 0;

    DBG("*** %s() eEmemType = %d ***\n", __func__, eEmemType);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaAlloc();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    nCrcMain = 0xffffffff;
    nCrcInfo = 0xffffffff;

    /////////////////////////
    // Erase
    /////////////////////////

    DBG("erase 0\n");

    DrvPlatformLyrTouchDeviceResetHw(); 
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(300);

    DBG("erase 1\n");

    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073

    // Stop watchdog
    RegSet16BitValue(0x3C60, 0xAA55); //bank:reg_PIU_MISC_0, addr:h0030

    // Set PROGRAM password
    RegSet16BitValue(0x161A, 0xABBA); //bank:emem, addr:h000D

    // Clear pce
    RegSetLByteValue(0x1618, 0x80); //bank:emem, addr:h000C

    DBG("erase 2\n");
    // Clear setting
    RegSetLByteValue(0x1618, 0x40); //bank:emem, addr:h000C
    
    mdelay(10);
    
    // Clear pce
    RegSetLByteValue(0x1618, 0x80); //bank:emem, addr:h000C

    DBG("erase 3\n");
    // Trigger erase
    if (eEmemType == EMEM_ALL)
    {
        RegSetLByteValue(0x160E, 0x08); //all chip //bank:emem, addr:h0007
    }
    else
    {
        RegSetLByteValue(0x160E, 0x04); //sector //bank:emem, addr:h0007
    }

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();
    
    mdelay(1000);
    DBG("erase OK\n");

    /////////////////////////
    // Program
    /////////////////////////

    DBG("program 0\n");

    DrvPlatformLyrTouchDeviceResetHw();
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(300);

    DBG("program 1\n");

    // Check_Loader_Ready: Polling 0x3CE4 is 0x1C70
    do
    {
        nRegData = RegGet16BitValue(0x3CE4); //bank:reg_PIU_MISC_0, addr:h0072
    } while (nRegData != 0x1C70);

    DBG("program 2\n");

    RegSet16BitValue(0x3CE4, 0xE38F);  //all chip
    mdelay(100);

    // Check_Loader_Ready2Program: Polling 0x3CE4 is 0x2F43
    do
    {
        nRegData = RegGet16BitValue(0x3CE4);
    } while (nRegData != 0x2F43);

    DBG("program 3\n");

    // prepare CRC & send data
    DrvCommonCrcInitTable();

    for (i = 0; i < MSG26XXM_FIRMWARE_WHOLE_SIZE; i ++) // Main : 32KB + Info : 8KB
    {
        if (i > 31)
        {
            for (j = 0; j < 1024; j ++)
            {
                nCrcInfo = DrvCommonCrcGetValue(szFwData[i][j], nCrcInfo);
            }
        }
        else if (i < 31)
        {
            for (j = 0; j < 1024; j ++)
            {
                nCrcMain = DrvCommonCrcGetValue(szFwData[i][j], nCrcMain);
            }
        }
        else ///if (i == 31)
        {
            szFwData[i][1014] = 0x5A;
            szFwData[i][1015] = 0xA5;

            for (j = 0; j < 1016; j ++)
            {
                nCrcMain = DrvCommonCrcGetValue(szFwData[i][j], nCrcMain);
            }
        }

        for (j = 0; j < 8; j ++)
        {
            IicWriteData(SLAVE_I2C_ID_DWI2C, &szFwData[i][j*128], 128);
        }
        mdelay(100);

        // Check_Program_Done: Polling 0x3CE4 is 0xD0BC
        do
        {
            nRegData = RegGet16BitValue(0x3CE4);
        } while (nRegData != 0xD0BC);

        // Continue_Program
        RegSet16BitValue(0x3CE4, 0x2F43);
    }

    DBG("program 4\n");

    // Notify_Write_Done
    RegSet16BitValue(0x3CE4, 0x1380);
    mdelay(100);

    DBG("program 5\n");

    // Check_CRC_Done: Polling 0x3CE4 is 0x9432
    do
    {
       nRegData = RegGet16BitValue(0x3CE4);
    } while (nRegData != 0x9432);

    DBG("program 6\n");

    // check CRC
    nCrcMain = nCrcMain ^ 0xffffffff;
    nCrcInfo = nCrcInfo ^ 0xffffffff;

    // read CRC from TP
    nCrcMainTp = RegGet16BitValue(0x3C80);
    nCrcMainTp = (nCrcMainTp << 16) | RegGet16BitValue(0x3C82);
    nCrcInfoTp = RegGet16BitValue(0x3CA0);
    nCrcInfoTp = (nCrcInfoTp << 16) | RegGet16BitValue(0x3CA2);

    DBG("nCrcMain=0x%x, nCrcInfo=0x%x, nCrcMainTp=0x%x, nCrcInfoTp=0x%x\n",
               nCrcMain, nCrcInfo, nCrcMainTp, nCrcInfoTp);

    g_FwDataCount = 0; // Reset g_FwDataCount to 0 after update firmware

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    DrvPlatformLyrTouchDeviceResetHw();
    mdelay(300);

    if ((nCrcMainTp != nCrcMain) || (nCrcInfoTp != nCrcInfo))
    {
        DBG("Update FAILED\n");

        return -1;
    }

    DBG("Update SUCCESS\n");

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaFree();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    return 0;
}

//------------------------------------------------------------------------------//

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP

void DrvFwCtrlOpenGestureWakeup(u16 nMode)
{
    u8 szDbBusTxData[3] = {0};
    u32 i = 0;
    s32 rc;

    DBG("*** %s() ***\n", __func__);

    DBG("wakeup mode = 0x%x\n", nMode);

    szDbBusTxData[0] = 0x58;
    szDbBusTxData[1] = (nMode >> 8) & 0xFF;
    szDbBusTxData[2] = nMode & 0xFF;

    while (i < 5)
    {
        rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 3);
        
        if (rc > 0)
        {
            DBG("Enable gesture wakeup success\n");
            break;
        }

        mdelay(10);
                
        i ++;
    }
    
    if (i == 5)
    {
        DBG("Enable gesture wakeup failed\n");		
    }
/*
    rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 3);
    if (rc < 0)
    {
        DBG("Enable gesture wakeup failed\n");
    }
    else
    {
        DBG("Enable gesture wakeup success\n");
    }
*/    
    g_GestureWakeupFlag = 1; // gesture wakeup is enabled
}

void DrvFwCtrlCloseGestureWakeup(void)
{
//    u8 szDbBusTxData[3] = {0};
//    s32 rc;

    DBG("*** %s() ***\n", __func__);
/*
    szDbBusTxData[0] = 0x58;
    szDbBusTxData[1] = 0x00;
    szDbBusTxData[2] = 0x00;

    rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 3);
    if (rc < 0)
    {
        DBG("Disable gesture wakeup failed\n");
    }
    else
    {
        DBG("Disable gesture wakeup success\n");
    }
*/
    g_GestureWakeupFlag = 0; // gesture wakeup is disabled
}

#endif //CONFIG_ENABLE_GESTURE_WAKEUP

//------------------------------------------------------------------------------//

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID

static u32 _DrvFwCtrlCalculateFirmwareCrcFromEFlash(EmemType_e eEmemType, u8 nIsNeedResetHW)
{
    u32 nRetVal = 0; 
    u16 nRegData = 0;

    DBG("*** %s() eEmemType = %d, nIsNeedResetHW = %d ***\n", __func__, eEmemType, nIsNeedResetHW);

    if (1 == nIsNeedResetHW)
    {
        DrvPlatformLyrTouchDeviceResetHw();
    }
    
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);

    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073

    // Stop watchdog
    RegSet16BitValue(0x3C60, 0xAA55); //bank:reg_PIU_MISC_0, addr:h0030

    // cmd
    RegSet16BitValue(0x3CE4, 0xDF4C); //bank:reg_PIU_MISC_0, addr:h0072

    // TP SW reset
    RegSet16BitValue(0x1E04, 0x7d60); //bank:chip, addr:h0002
    RegSet16BitValue(0x1E04, 0x829F);

    // Start mcu
    RegSetLByteValue(0x0FE6, 0x00); //bank:mheg5, addr:h0073
    
    mdelay(100);

    // Polling 0x3CE4 is 0x9432
    do
    {
        nRegData = RegGet16BitValue(0x3CE4); //bank:reg_PIU_MISC_0, addr:h0072
    } while (nRegData != 0x9432);

    if (eEmemType == EMEM_MAIN) // Read calculated main block CRC(32K-8) from register
    {
        nRetVal = RegGet16BitValue(0x3C80);
        nRetVal = (nRetVal << 16) | RegGet16BitValue(0x3C82);
        
        DBG("Main Block CRC = 0x%x\n", nRetVal);
    }
    else if (eEmemType == EMEM_INFO) // Read calculated info block CRC(8K) from register
    {
        nRetVal = RegGet16BitValue(0x3CA0);
        nRetVal = (nRetVal << 16) | RegGet16BitValue(0x3CA2);

        DBG("Info Block CRC = 0x%x\n", nRetVal);
    }

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    return nRetVal;	
}

static u32 _DrvFwCtrlRetrieveFirmwareCrcFromMainBlock(EmemType_e eEmemType)
{
    u32 nRetVal = 0; 
    u16 nRegData = 0;
    u8 szDbBusTxData[5] = {0};
    u8 szDbBusRxData[4] = {0};

    DBG("*** %s() eEmemType = %d ***\n", __func__, eEmemType);

    DrvPlatformLyrTouchDeviceResetHw();
    
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);

    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073

    // Stop watchdog
    RegSet16BitValue(0x3C60, 0xAA55); //bank:reg_PIU_MISC_0, addr:h0030

    // cmd
    RegSet16BitValue(0x3CE4, 0xA4AB); //bank:reg_PIU_MISC_0, addr:h0072

    // TP SW reset
    RegSet16BitValue(0x1E04, 0x7d60); //bank:chip, addr:h0002
    RegSet16BitValue(0x1E04, 0x829F);

    // Start mcu
    RegSetLByteValue(0x0FE6, 0x00); //bank:mheg5, addr:h0073
    
    mdelay(100);

    // Polling 0x3CE4 is 0x5B58
    do
    {
        nRegData = RegGet16BitValue(0x3CE4); //bank:reg_PIU_MISC_0, addr:h0072
    } while (nRegData != 0x5B58);

    szDbBusTxData[0] = 0x72;
    if (eEmemType == EMEM_MAIN) // Read main block CRC(32K-8) from main block
    {
        szDbBusTxData[1] = 0x7F;
        szDbBusTxData[2] = 0xF8;
    }
    else if (eEmemType == EMEM_INFO) // Read info block CRC(8K) from main block
    {
        szDbBusTxData[1] = 0x7F;
        szDbBusTxData[2] = 0xFC;
    }
    szDbBusTxData[3] = 0x00;
    szDbBusTxData[4] = 0x04;

    IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 5);
    IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 4);

    /*
      The order of 4 bytes [ 0 : 1 : 2 : 3 ]
      Ex. CRC32 = 0x12345678
          0x7FF8 = 0x78, 0x7FF9 = 0x56,
          0x7FFA = 0x34, 0x7FFB = 0x12
    */

    nRetVal = szDbBusRxData[3];
    nRetVal = (nRetVal << 8) | szDbBusRxData[2];
    nRetVal = (nRetVal << 8) | szDbBusRxData[1];
    nRetVal = (nRetVal << 8) | szDbBusRxData[0];
    
    DBG("CRC = 0x%x\n", nRetVal);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    return nRetVal;	
}

static u32 _DrvFwCtrlRetrieveInfoCrcFromInfoBlock(void)
{
    u32 nRetVal = 0; 
    u16 nRegData = 0;
    u8 szDbBusTxData[5] = {0};
    u8 szDbBusRxData[4] = {0};

    DBG("*** %s() ***\n", __func__);

    DrvPlatformLyrTouchDeviceResetHw();
    
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);

    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073

    // Stop watchdog
    RegSet16BitValue(0x3C60, 0xAA55); //bank:reg_PIU_MISC_0, addr:h0030

    // cmd
    RegSet16BitValue(0x3CE4, 0xA4AB); //bank:reg_PIU_MISC_0, addr:h0072

    // TP SW reset
    RegSet16BitValue(0x1E04, 0x7d60); //bank:chip, addr:h0002
    RegSet16BitValue(0x1E04, 0x829F);

    // Start mcu
    RegSetLByteValue(0x0FE6, 0x00); //bank:mheg5, addr:h0073
    
    mdelay(100);

    // Polling 0x3CE4 is 0x5B58
    do
    {
        nRegData = RegGet16BitValue(0x3CE4); //bank:reg_PIU_MISC_0, addr:h0072
    } while (nRegData != 0x5B58);


    // Read info CRC(8K-4) from info block
    szDbBusTxData[0] = 0x72;
    szDbBusTxData[1] = 0x80;
    szDbBusTxData[2] = 0x00;
    szDbBusTxData[3] = 0x00;
    szDbBusTxData[4] = 0x04;

    IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 5);
    IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 4);

    nRetVal = szDbBusRxData[3];
    nRetVal = (nRetVal << 8) | szDbBusRxData[2];
    nRetVal = (nRetVal << 8) | szDbBusRxData[1];
    nRetVal = (nRetVal << 8) | szDbBusRxData[0];
    
    DBG("CRC = 0x%x\n", nRetVal);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();
    
    return nRetVal;	
}

static u32 _DrvFwCtrlRetrieveFrimwareCrcFromBinFile(u8 szTmpBuf[][1024], EmemType_e eEmemType)
{
    u32 nRetVal = 0; 
    
    DBG("*** %s() eEmemType = %d ***\n", __func__, eEmemType);
    
    if (szTmpBuf != NULL)
    {
        if (eEmemType == EMEM_MAIN) // Read main block CRC(32K-8) from bin file
        {
            nRetVal = szTmpBuf[31][1019];
            nRetVal = (nRetVal << 8) | szTmpBuf[31][1018];
            nRetVal = (nRetVal << 8) | szTmpBuf[31][1017];
            nRetVal = (nRetVal << 8) | szTmpBuf[31][1016];
        }
        else if (eEmemType == EMEM_INFO) // Read info block CRC(8K) from bin file
        {
            nRetVal = szTmpBuf[31][1023];
            nRetVal = (nRetVal << 8) | szTmpBuf[31][1022];
            nRetVal = (nRetVal << 8) | szTmpBuf[31][1021];
            nRetVal = (nRetVal << 8) | szTmpBuf[31][1020];
        }
    }

    return nRetVal;
}

static u32 _DrvFwCtrlCalculateInfoCrcByDeviceDriver(void)
{
    u32 nRetVal = 0xffffffff; 
    u32 i, j;
    u16 nRegData = 0;
    u8 szDbBusTxData[5] = {0};

    DBG("*** %s() ***\n", __func__);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaAlloc();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    DrvPlatformLyrTouchDeviceResetHw();
    
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(300);

    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073

    // Stop watchdog
    RegSet16BitValue(0x3C60, 0xAA55); //bank:reg_PIU_MISC_0, addr:h0030

    // cmd
    RegSet16BitValue(0x3CE4, 0xA4AB); //bank:reg_PIU_MISC_0, addr:h0072

    // TP SW reset
    RegSet16BitValue(0x1E04, 0x7d60); //bank:chip, addr:h0002
    RegSet16BitValue(0x1E04, 0x829F);

    // Start mcu
    RegSetLByteValue(0x0FE6, 0x00); //bank:mheg5, addr:h0073
    
    mdelay(100);

    // Polling 0x3CE4 is 0x5B58
    do
    {
        nRegData = RegGet16BitValue(0x3CE4); //bank:reg_PIU_MISC_0, addr:h0072
    } while (nRegData != 0x5B58);

    DrvCommonCrcInitTable();

    // Read info data(8K) from info block
    szDbBusTxData[0] = 0x72;
    szDbBusTxData[3] = 0x00; // read 128 bytes
    szDbBusTxData[4] = 0x80;

    for (i = 0; i < 8; i ++)
    {
        for (j = 0; j < 8; j ++)
        {
            szDbBusTxData[1] = 0x80 + (i*0x04) + (((j*128)&0xff00)>>8);
            szDbBusTxData[2] = (j*128)&0x00ff;

            IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 5);

            // Receive info data
            IicReadData(SLAVE_I2C_ID_DWI2C, &_gTempData[j*128], 128); 
        }
     
        if (i == 0)
        {
            for (j = 4; j < 1024; j ++)
            {
                nRetVal = DrvCommonCrcGetValue(_gTempData[j], nRetVal);
            }
        }
        else
        {
            for (j = 0; j < 1024; j ++)
            {
                nRetVal = DrvCommonCrcGetValue(_gTempData[j], nRetVal);
            }
        }
    }

    nRetVal = nRetVal ^ 0xffffffff;

    DBG("Info(8K-4) CRC = 0x%x\n", nRetVal);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaFree();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    return nRetVal;	
}

static s32 _DrvFwCtrlCompare8BytesForCrc(u8 szTmpBuf[][1024])
{
    s32 nRetVal = -1;
    u16 nRegData = 0;
    u8 szDbBusTxData[5] = {0};
    u8 szDbBusRxData[8] = {0};
    u8 crc[8] = {0}; 
    
    DBG("*** %s() ***\n", __func__);
    
    // Read 8 bytes from bin file
    if (szTmpBuf != NULL)
    {
        crc[0] = szTmpBuf[31][1016];
        crc[1] = szTmpBuf[31][1017];
        crc[2] = szTmpBuf[31][1018];
        crc[3] = szTmpBuf[31][1019];
        crc[4] = szTmpBuf[31][1020];
        crc[5] = szTmpBuf[31][1021];
        crc[6] = szTmpBuf[31][1022];
        crc[7] = szTmpBuf[31][1023];
    }

    // Read 8 bytes from the firmware on e-flash
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);

    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073

    // Stop watchdog
    RegSet16BitValue(0x3C60, 0xAA55); //bank:reg_PIU_MISC_0, addr:h0030

    // cmd
    RegSet16BitValue(0x3CE4, 0xA4AB); //bank:reg_PIU_MISC_0, addr:h0072

    // TP SW reset
    RegSet16BitValue(0x1E04, 0x7d60); //bank:chip, addr:h0002
    RegSet16BitValue(0x1E04, 0x829F);

    // Start mcu
    RegSetLByteValue(0x0FE6, 0x00); //bank:mheg5, addr:h0073
    
    mdelay(100);

    // Polling 0x3CE4 is 0x5B58
    do
    {
        nRegData = RegGet16BitValue(0x3CE4); //bank:reg_PIU_MISC_0, addr:h0072
    } while (nRegData != 0x5B58);

    szDbBusTxData[0] = 0x72;
    szDbBusTxData[1] = 0x7F;
    szDbBusTxData[2] = 0xF8;
    szDbBusTxData[3] = 0x00;
    szDbBusTxData[4] = 0x08;

    IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 5);
    IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 8);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();
    
    if (crc[0] == szDbBusRxData[0]
        && crc[1] == szDbBusRxData[1]
        && crc[2] == szDbBusRxData[2]
        && crc[3] == szDbBusRxData[3]
        && crc[4] == szDbBusRxData[4]
        && crc[5] == szDbBusRxData[5]
        && crc[6] == szDbBusRxData[6]
        && crc[7] == szDbBusRxData[7])
    {
        nRetVal = 0;		
    }
    else
    {
        nRetVal = -1;		
    }
    
    DBG("compare 8bytes for CRC = %d\n", nRetVal);

    return nRetVal;
}

static u16 _DrvFwCtrlGetSwId(EmemType_e eEmemType)
{
    u16 nRetVal = 0; 
    u16 nRegData = 0;
    u8 szDbBusTxData[5] = {0};
    u8 szDbBusRxData[4] = {0};

    DBG("*** %s() eEmemType = %d ***\n", __func__, eEmemType);

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);

    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073

    // Stop watchdog
    RegSet16BitValue(0x3C60, 0xAA55); //bank:reg_PIU_MISC_0, addr:h0030

    // cmd
    RegSet16BitValue(0x3CE4, 0xA4AB); //bank:reg_PIU_MISC_0, addr:h0072

    // TP SW reset
    RegSet16BitValue(0x1E04, 0x7d60); //bank:chip, addr:h0002
    RegSet16BitValue(0x1E04, 0x829F);

    // Start mcu
    RegSetLByteValue(0x0FE6, 0x00); //bank:mheg5, addr:h0073
    
    mdelay(100);

    // Polling 0x3CE4 is 0x5B58
    do
    {
        nRegData = RegGet16BitValue(0x3CE4); //bank:reg_PIU_MISC_0, addr:h0072
    } while (nRegData != 0x5B58);

    szDbBusTxData[0] = 0x72;
    if (eEmemType == EMEM_MAIN) // Read SW ID from main block
    {
        szDbBusTxData[1] = 0x00;
        szDbBusTxData[2] = 0x2A;
    }
    else if (eEmemType == EMEM_INFO) // Read SW ID from info block
    {
        szDbBusTxData[1] = 0x80;
        szDbBusTxData[2] = 0x04;
    }
    szDbBusTxData[3] = 0x00;
    szDbBusTxData[4] = 0x04;

    IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 5);
    IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 4);

    /*
      Ex. SW ID in Main Block :
          Major low byte at address 0x002A
          Major high byte at address 0x002B
          
          SW ID in Info Block :
          Major low byte at address 0x8004
          Major high byte at address 0x8005
    */

    nRetVal = szDbBusRxData[1];
    nRetVal = (nRetVal << 8) | szDbBusRxData[0];
    
    DBG("SW ID = 0x%x\n", nRetVal);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    return nRetVal;		
}

static void _DrvFwCtrlEraseFirmwareOnEFlash(EmemType_e eEmemType)
{
    DBG("*** %s() eEmemType = %d ***\n", __func__, eEmemType);

    DBG("erase 0\n");

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);

    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073

    // Stop watchdog
    RegSet16BitValue(0x3C60, 0xAA55); //bank:reg_PIU_MISC_0, addr:h0030

    // cmd
    RegSet16BitValue(0x3CE4, 0x78C5); //bank:reg_PIU_MISC_0, addr:h0072

    // TP SW reset
    RegSet16BitValue(0x1E04, 0x7d60); //bank:chip, addr:h0002
    RegSet16BitValue(0x1E04, 0x829F);

    // Start mcu
    RegSetLByteValue(0x0FE6, 0x00); //bank:mheg5, addr:h0073

    mdelay(100);
        
    DBG("erase 1\n");

    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073

    // Stop watchdog
    RegSet16BitValue(0x3C60, 0xAA55); //bank:reg_PIU_MISC_0, addr:h0030

    // Set PROGRAM password
    RegSet16BitValue(0x161A, 0xABBA); //bank:emem, addr:h000D

    if (eEmemType == EMEM_INFO)
    {
        RegSet16BitValue(0x1600, 0x8000); //bank:emem, addr:h0000
    }

    // Clear pce
    RegSetLByteValue(0x1618, 0x80); //bank:emem, addr:h000C

    DBG("erase 2\n");

    // Clear setting
    RegSetLByteValue(0x1618, 0x40); //bank:emem, addr:h000C
    
    mdelay(10);
    
    // Clear pce
    RegSetLByteValue(0x1618, 0x80); //bank:emem, addr:h000C

    DBG("erase 3\n");

    // Trigger erase
    if (eEmemType == EMEM_ALL)
    {
        RegSetLByteValue(0x160E, 0x08); //all chip //bank:emem, addr:h0007
    }
    else if (eEmemType == EMEM_MAIN || eEmemType == EMEM_INFO)
    {
        RegSetLByteValue(0x160E, 0x04); //sector //bank:emem, addr:h0007
    }

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();
    
    mdelay(200);	
    
    DBG("erase OK\n");
}

static void _DrvFwCtrlProgramFirmwareOnEFlash(EmemType_e eEmemType)
{
    u32 nStart = 0, nEnd = 0; 
    u32 i, j; 
    u16 nRegData = 0;
//    u16 nRegData2 = 0, nRegData3 = 0; // add for debug

    DBG("*** %s() eEmemType = %d ***\n", __func__, eEmemType);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaAlloc();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    DBG("program 0\n");

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);

    if (eEmemType == EMEM_INFO || eEmemType == EMEM_MAIN)
    {
        // Stop mcu
        RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073

        // Stop watchdog
        RegSet16BitValue(0x3C60, 0xAA55); //bank:reg_PIU_MISC_0, addr:h0030

        // cmd
        RegSet16BitValue(0x3CE4, 0x78C5); //bank:reg_PIU_MISC_0, addr:h0072

        // TP SW reset
        RegSet16BitValue(0x1E04, 0x7d60); //bank:chip, addr:h0002
        RegSet16BitValue(0x1E04, 0x829F);
        
        nRegData = RegGet16BitValue(0x1618);
//        DBG("*** reg(0x16, 0x18)  = 0x%x ***\n", nRegData); // add for debug
        
        nRegData |= 0x40;
//        DBG("*** nRegData  = 0x%x ***\n", nRegData); // add for debug
        
        RegSetLByteValue(0x1618, nRegData);

        // Start mcu
        RegSetLByteValue(0x0FE6, 0x00); //bank:mheg5, addr:h0073
    
        mdelay(100);
    }

    DBG("program 1\n");

    RegSet16BitValue(0x0F52, 0xDB00); // add for analysis

    // Check_Loader_Ready: Polling 0x3CE4 is 0x1C70
    do
    {
        nRegData = RegGet16BitValue(0x3CE4); //bank:reg_PIU_MISC_0, addr:h0072
//        DBG("*** reg(0x3C, 0xE4) = 0x%x ***\n", nRegData); // add for debug

//        nRegData2 = RegGet16BitValue(0x0F00); // add for debug
//        DBG("*** reg(0x0F, 0x00) = 0x%x ***\n", nRegData2);

//        nRegData3 = RegGet16BitValue(0x1E04); // add for debug
//        DBG("*** reg(0x1E, 0x04) = 0x%x ***\n", nRegData3);

    } while (nRegData != 0x1C70);

    DBG("program 2\n");

    if (eEmemType == EMEM_ALL)
    {
        RegSet16BitValue(0x3CE4, 0xE38F);  //all chip

        nStart = 0;
        nEnd = MSG26XXM_FIRMWARE_WHOLE_SIZE; //32K + 8K
    }
    else if (eEmemType == EMEM_MAIN)
    {
        RegSet16BitValue(0x3CE4, 0x7731);  //main block

        nStart = 0;
        nEnd = MSG26XXM_FIRMWARE_MAIN_BLOCK_SIZE; //32K
    }
    else if (eEmemType == EMEM_INFO)
    {
        RegSet16BitValue(0x3CE4, 0xB9D6);  //info block

        nStart = MSG26XXM_FIRMWARE_MAIN_BLOCK_SIZE;
        nEnd = MSG26XXM_FIRMWARE_MAIN_BLOCK_SIZE + MSG26XXM_FIRMWARE_INFO_BLOCK_SIZE;
    }

    // Check_Loader_Ready2Program: Polling 0x3CE4 is 0x2F43
    do
    {
        nRegData = RegGet16BitValue(0x3CE4);
    } while (nRegData != 0x2F43);

    DBG("program 3\n");

    for (i = nStart; i < nEnd; i ++)
    {
        for (j = 0; j < 8; j ++)
        {
            IicWriteData(SLAVE_I2C_ID_DWI2C, &g_FwData[i][j*128], 128);
        }

        mdelay(100);

        // Check_Program_Done: Polling 0x3CE4 is 0xD0BC
        do
        {
            nRegData = RegGet16BitValue(0x3CE4);
        } while (nRegData != 0xD0BC);

        // Continue_Program
        RegSet16BitValue(0x3CE4, 0x2F43);
    }

    DBG("program 4\n");

    // Notify_Write_Done
    RegSet16BitValue(0x3CE4, 0x1380);
    mdelay(100);

    DBG("program 5\n");

    // Check_CRC_Done: Polling 0x3CE4 is 0x9432
    do
    {
       nRegData = RegGet16BitValue(0x3CE4);
    } while (nRegData != 0x9432);

    DBG("program 6\n");

    g_FwDataCount = 0; // Reset g_FwDataCount to 0 after update firmware

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    mdelay(300);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaFree();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    DBG("program OK\n");
}

static s32 _DrvFwCtrlUpdateFirmwareBySwId(void)
{
    s32 nRetVal = -1;
    u32 nCrcInfoA = 0, nCrcInfoB = 0, nCrcMainA = 0, nCrcMainB = 0;
    
    DBG("*** %s() ***\n", __func__);
    
    DBG("_gIsUpdateInfoBlockFirst = %d, _gIsUpdateFirmware = 0x%x\n", _gIsUpdateInfoBlockFirst, _gIsUpdateFirmware);
    
    if (_gIsUpdateInfoBlockFirst == 1)
    {
        if ((_gIsUpdateFirmware & 0x10) == 0x10)
        {
            _DrvFwCtrlEraseFirmwareOnEFlash(EMEM_INFO);
            _DrvFwCtrlProgramFirmwareOnEFlash(EMEM_INFO);
 
            nCrcInfoA = _DrvFwCtrlRetrieveFrimwareCrcFromBinFile(g_FwData, EMEM_INFO);
            nCrcInfoB = _DrvFwCtrlCalculateFirmwareCrcFromEFlash(EMEM_INFO, 0);

            DBG("nCrcInfoA = 0x%x, nCrcInfoB = 0x%x\n", nCrcInfoA, nCrcInfoB);
        
            if (nCrcInfoA == nCrcInfoB)
            {
                _DrvFwCtrlEraseFirmwareOnEFlash(EMEM_MAIN);
                _DrvFwCtrlProgramFirmwareOnEFlash(EMEM_MAIN);

                nCrcMainA = _DrvFwCtrlRetrieveFrimwareCrcFromBinFile(g_FwData, EMEM_MAIN);
                nCrcMainB = _DrvFwCtrlCalculateFirmwareCrcFromEFlash(EMEM_MAIN, 0);

                DBG("nCrcMainA = 0x%x, nCrcMainB = 0x%x\n", nCrcMainA, nCrcMainB);
        		
                if (nCrcMainA == nCrcMainB)
                {
                    nRetVal = _DrvFwCtrlCompare8BytesForCrc(g_FwData);
                    
                    if (nRetVal == 0)
                    {
                        _gIsUpdateFirmware = 0x00;
                    }
                    else
                    {
                        _gIsUpdateFirmware = 0x11;
                    }
                }
                else
                {
                    _gIsUpdateFirmware = 0x01;
                }
            }
            else
            {
                _gIsUpdateFirmware = 0x11;
            }
        }
        else if ((_gIsUpdateFirmware & 0x01) == 0x01)
        {
            _DrvFwCtrlEraseFirmwareOnEFlash(EMEM_MAIN);
            _DrvFwCtrlProgramFirmwareOnEFlash(EMEM_MAIN);

            nCrcMainA = _DrvFwCtrlRetrieveFrimwareCrcFromBinFile(g_FwData, EMEM_MAIN);
            nCrcMainB = _DrvFwCtrlCalculateFirmwareCrcFromEFlash(EMEM_MAIN, 0);

            DBG("nCrcMainA=0x%x, nCrcMainB=0x%x\n", nCrcMainA, nCrcMainB);
    		
            if (nCrcMainA == nCrcMainB)
            {
                nRetVal = _DrvFwCtrlCompare8BytesForCrc(g_FwData);

                if (nRetVal == 0)
                {
                    _gIsUpdateFirmware = 0x00;
                }
                else
                {
                    _gIsUpdateFirmware = 0x11;
                }
            }
            else
            {
                _gIsUpdateFirmware = 0x01;
            }
        }
    }
    else //_gIsUpdateInfoBlockFirst == 0
    {
        if ((_gIsUpdateFirmware & 0x10) == 0x10)
        {
            _DrvFwCtrlEraseFirmwareOnEFlash(EMEM_MAIN);
            _DrvFwCtrlProgramFirmwareOnEFlash(EMEM_MAIN);

            nCrcMainA = _DrvFwCtrlRetrieveFrimwareCrcFromBinFile(g_FwData, EMEM_MAIN);
            nCrcMainB = _DrvFwCtrlCalculateFirmwareCrcFromEFlash(EMEM_MAIN, 0);

            DBG("nCrcMainA=0x%x, nCrcMainB=0x%x\n", nCrcMainA, nCrcMainB);

            if (nCrcMainA == nCrcMainB)
            {
                _DrvFwCtrlEraseFirmwareOnEFlash(EMEM_INFO);
                _DrvFwCtrlProgramFirmwareOnEFlash(EMEM_INFO);

                nCrcInfoA = _DrvFwCtrlRetrieveFrimwareCrcFromBinFile(g_FwData, EMEM_INFO);
                nCrcInfoB = _DrvFwCtrlCalculateFirmwareCrcFromEFlash(EMEM_INFO, 0);
                
                DBG("nCrcInfoA=0x%x, nCrcInfoB=0x%x\n", nCrcInfoA, nCrcInfoB);

                if (nCrcInfoA == nCrcInfoB)
                {
                    nRetVal = _DrvFwCtrlCompare8BytesForCrc(g_FwData);
                    
                    if (nRetVal == 0)
                    {
                        _gIsUpdateFirmware = 0x00;
                    }
                    else
                    {
                        _gIsUpdateFirmware = 0x11;
                    }
                }
                else
                {
                    _gIsUpdateFirmware = 0x01;
                }
            }
            else
            {
                _gIsUpdateFirmware = 0x11;
            }
        }
        else if ((_gIsUpdateFirmware & 0x01) == 0x01)
        {
            _DrvFwCtrlEraseFirmwareOnEFlash(EMEM_INFO);
            _DrvFwCtrlProgramFirmwareOnEFlash(EMEM_INFO);

            nCrcInfoA = _DrvFwCtrlRetrieveFrimwareCrcFromBinFile(g_FwData, EMEM_INFO);
            nCrcInfoB = _DrvFwCtrlCalculateFirmwareCrcFromEFlash(EMEM_INFO, 0);

            DBG("nCrcInfoA=0x%x, nCrcInfoB=0x%x\n", nCrcInfoA, nCrcInfoB);

            if (nCrcInfoA == nCrcInfoB)
            {
                nRetVal = _DrvFwCtrlCompare8BytesForCrc(g_FwData);
                
                if (nRetVal == 0)
                {
                    _gIsUpdateFirmware = 0x00;
                }
                else
                {
                    _gIsUpdateFirmware = 0x11;
                }
            }
            else
            {
                _gIsUpdateFirmware = 0x01;
            }
        }    		
    }
    
    return nRetVal;	
}

static void _DrvFwCtrlUpdateFirmwareBySwIdDoWork(struct work_struct *pWork)
{
    s32 nRetVal = 0;
    
    DBG("*** %s() _gUpdateRetryCount = %d ***\n", __func__, _gUpdateRetryCount);

    nRetVal = _DrvFwCtrlUpdateFirmwareBySwId();
    
    DBG("*** update firmware by sw id result = %d ***\n", nRetVal);
    
    if (nRetVal == 0)
    {
        DBG("update firmware by sw id success\n");
        _gIsUpdateInfoBlockFirst = 0;
        _gIsUpdateFirmware = 0x00;

        DrvPlatformLyrTouchDeviceResetHw();

        DrvPlatformLyrEnableFingerTouchReport();
    }
    else //nRetVal == -1
    {
        _gUpdateRetryCount --;
        if (_gUpdateRetryCount > 0)
        {
            DBG("_gUpdateRetryCount = %d\n", _gUpdateRetryCount);
            queue_work(_gUpdateFirmwareBySwIdWorkQueue, &_gUpdateFirmwareBySwIdWork);
        }
        else
        {
            DBG("update firmware by sw id failed\n");
            _gIsUpdateInfoBlockFirst = 0;
            _gIsUpdateFirmware = 0x00;

            DrvPlatformLyrTouchDeviceResetHw();

            DrvPlatformLyrEnableFingerTouchReport();
        }
    }
}

#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

/*=============================================================*/
// GLOBAL FUNCTION DEFINITION
/*=============================================================*/

u8 DrvFwCtrlGetChipType(void)
{
    u8 nChipType = 0;

    DBG("*** %s() ***\n", __func__);

    DrvPlatformLyrTouchDeviceResetHw();

    // Erase TP Flash first
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(300);

    // Stop MCU
    RegSetLByteValue(0x0FE6, 0x01);

    // Disable watchdog
    RegSet16BitValue(0x3C60, 0xAA55);
    
    nChipType = RegGet16BitValue(0x1ECC) & 0xFF;

    if (nChipType != CHIP_TYPE_MSG21XX &&   // (0x01) 
        nChipType != CHIP_TYPE_MSG21XXA &&  // (0x02) 
        nChipType != CHIP_TYPE_MSG26XXM &&  // (0x03) 
        nChipType != CHIP_TYPE_MSG22XX)     // (0x7A)
    {
        nChipType = 0;
    }

    DBG("*** Chip Type = 0x%x ***\n", nChipType);

    DrvPlatformLyrTouchDeviceResetHw();
    
    return nChipType;
}	

void DrvFwCtrlGetCustomerFirmwareVersion(u16 *pMajor, u16 *pMinor, u8 **ppVersion)
{
    u8 szDbBusTxData[3] = {0};
    u8 szDbBusRxData[4] = {0};

    DBG("*** %s() ***\n", __func__);

    szDbBusTxData[0] = 0x53;
    szDbBusTxData[1] = 0x00;
    szDbBusTxData[2] = 0x2A;
    
    mutex_lock(&g_Mutex);

    DrvPlatformLyrTouchDeviceResetHw();
    mdelay(100);

    IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 3);
    IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 4);

    mutex_unlock(&g_Mutex);

    *pMajor = (szDbBusRxData[1]<<8) + szDbBusRxData[0];
    *pMinor = (szDbBusRxData[3]<<8) + szDbBusRxData[2];

    DBG("*** major = %d ***\n", *pMajor);
    DBG("*** minor = %d ***\n", *pMinor);

    if (*ppVersion == NULL)
    {
        *ppVersion = kzalloc(sizeof(u8)*6, GFP_KERNEL);
    }
    
    sprintf(*ppVersion, "%03d%03d", *pMajor, *pMinor);
}

void DrvFwCtrlGetPlatformFirmwareVersion(u8 **ppVersion)
{
    u32 i;
    u16 nRegData = 0;
    u8 szDbBusTxData[5] = {0};
    u8 szDbBusRxData[16] = {0};

    DBG("*** %s() ***\n", __func__);

    mutex_lock(&g_Mutex);

    DrvPlatformLyrTouchDeviceResetHw();
    
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);

    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073

    // Stop watchdog
    RegSet16BitValue(0x3C60, 0xAA55); //bank:reg_PIU_MISC_0, addr:h0030

    // cmd
    RegSet16BitValue(0x3CE4, 0xA4AB); //bank:reg_PIU_MISC_0, addr:h0072

    // TP SW reset
    RegSet16BitValue(0x1E04, 0x7d60); //bank:chip, addr:h0002
    RegSet16BitValue(0x1E04, 0x829F);

    // Start mcu
    RegSetLByteValue(0x0FE6, 0x00); //bank:mheg5, addr:h0073
    
    mdelay(100);

    // Polling 0x3CE4 is 0x5B58
    do
    {
        nRegData = RegGet16BitValue(0x3CE4); //bank:reg_PIU_MISC_0, addr:h0072
    } while (nRegData != 0x5B58);

    // Read platform firmware version from info block
    szDbBusTxData[0] = 0x72;
    szDbBusTxData[3] = 0x00;
    szDbBusTxData[4] = 0x08;

    for (i = 0; i < 2; i ++)
    {
        szDbBusTxData[1] = 0x80;
        szDbBusTxData[2] = 0x10 + ((i*8)&0x00ff);

        IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 5);

        mdelay(50);

        IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[i*8], 8);
    }

    if (*ppVersion == NULL)
    {
        *ppVersion = kzalloc(sizeof(u8)*16, GFP_KERNEL);
    }
    
    sprintf(*ppVersion, "%.16s", szDbBusRxData);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();
    
    DrvPlatformLyrTouchDeviceResetHw();
    mdelay(300);

    mutex_unlock(&g_Mutex);
    
    DBG("*** platform firmware version = %s ***\n", *ppVersion);
}

s32 DrvFwCtrlUpdateFirmware(u8 szFwData[][1024], EmemType_e eEmemType)
{
    DBG("*** %s() ***\n", __func__);

    return _DrvFwCtrlUpdateFirmwareCash(szFwData, eEmemType);
}	

//------------------------------------------------------------------------------//

#ifdef CONFIG_ENABLE_FIRMWARE_DATA_LOG

u16 DrvFwCtrlGetFirmwareMode(void)
{
    u16 nMode = 0;
    
    DBG("*** %s() ***\n", __func__);

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);

    nMode = RegGet16BitValue(0x3CF4); //bank:reg_PIU_MISC0, addr:h007a

    DBG("firmware mode = 0x%x\n", nMode);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();
    mdelay(100);

    return nMode;
}

u16 DrvFwCtrlChangeFirmwareMode(u16 nMode)
{
    DBG("*** %s() *** nMode = 0x%x\n", __func__, nMode);

    DrvPlatformLyrTouchDeviceResetHw(); 

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);

    RegSet16BitValue(0x3CF4, nMode); //bank:reg_PIU_MISC0, addr:h007a
    nMode = RegGet16BitValue(0x3CF4); 

    DBG("firmware mode = 0x%x\n", nMode);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();
    mdelay(100);

    return nMode;
}

void DrvFwCtrlGetFirmwareInfo(FirmwareInfo_t *pInfo)
{
    u8 szDbBusTxData[3] = {0};
    u8 szDbBusRxData[8] = {0};

    DBG("*** %s() ***\n", __func__);

    szDbBusTxData[0] = 0x53;
    szDbBusTxData[1] = 0x00;
    szDbBusTxData[2] = 0x48;

    mutex_lock(&g_Mutex);
    
    IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 3);
    mdelay(50);
    IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 8);

    mutex_unlock(&g_Mutex);

    if (szDbBusRxData[0] == 8 && szDbBusRxData[1] == 0 && szDbBusRxData[2] == 9 && szDbBusRxData[3] == 0)
    {
        DBG("*** Debug Mode Packet Header is 0xA5 ***\n");
/*
        DbBusEnterSerialDebugMode();
        DbBusStopMCU();
        DbBusIICUseBus();
        DbBusIICReshape();
        mdelay(100);
*/
        pInfo->nLogModePacketHeader = 0xA5;
        pInfo->nMy = 0;
        pInfo->nMx = 0;
//        *pDriveLineNumber = AnaGetMutualSubframeNum();
//        *pSenseLineNumber = AnaGetMutualChannelNum();

/*        
        DbBusIICNotUseBus();
        DbBusNotStopMCU();
        DbBusExitSerialDebugMode();
        mdelay(100);
*/
    }
    else if (szDbBusRxData[0] == 0xAB)
    {
        DBG("*** Debug Mode Packet Header is 0xAB ***\n");

        pInfo->nLogModePacketHeader = szDbBusRxData[0];
        pInfo->nMy = szDbBusRxData[1];
        pInfo->nMx = szDbBusRxData[2];
//        pInfo->nSd = szDbBusRxData[1];
//        pInfo->nSs = szDbBusRxData[2];
    }
    else if (szDbBusRxData[0] == 0xA7 && szDbBusRxData[3] == PACKET_TYPE_TOOTH_PATTERN) 
    {
        DBG("*** Debug Packet Header is 0xA7 ***\n");

        pInfo->nLogModePacketHeader = szDbBusRxData[0];
        pInfo->nType = szDbBusRxData[3];
        pInfo->nMy = szDbBusRxData[4];
        pInfo->nMx = szDbBusRxData[5];
        pInfo->nSd = szDbBusRxData[6];
        pInfo->nSs = szDbBusRxData[7];
    }

    if (pInfo->nLogModePacketHeader == 0xA5)
    {
        if (pInfo->nMy != 0 && pInfo->nMx != 0)
        {
            // for parsing debug mode packet 0xA5 
            pInfo->nLogModePacketLength = 1+1+1+1+10*3+pInfo->nMx*pInfo->nMy*2+1;
        }
        else
        {
            DBG("Failed to retrieve channel number or subframe number for debug mode packet 0xA5.\n");
        }
    }
    else if (pInfo->nLogModePacketHeader == 0xAB)
    {
        if (pInfo->nMy != 0 && pInfo->nMx != 0)
        {
            // for parsing debug mode packet 0xAB 
            pInfo->nLogModePacketLength = 1+1+1+1+10*3+pInfo->nMy*pInfo->nMx*2+pInfo->nMy*2+pInfo->nMx*2+2*2+8*2+1;
        }
        else
        {
            DBG("Failed to retrieve channel number or subframe number for debug mode packet 0xAB.\n");
        }
    }
    else if (pInfo->nLogModePacketHeader == 0xA7 && pInfo->nType == PACKET_TYPE_TOOTH_PATTERN)
    {
        if (pInfo->nMy != 0 && pInfo->nMx != 0 && pInfo->nSd != 0 && pInfo->nSs != 0)
        {
            // for parsing debug mode packet 0xA7  
            pInfo->nLogModePacketLength = 1+1+1+1+1+10*3+pInfo->nMy*pInfo->nMx*2+pInfo->nSd*2+pInfo->nSs*2+10*2+1;
        }
        else
        {
            DBG("Failed to retrieve channel number or subframe number for debug mode packet 0xA7.\n");
        }
    }
    else
    {
        DBG("Undefined debug mode packet header = 0x%x\n", pInfo->nLogModePacketHeader);
    }
    
    DBG("*** debug mode packet header = 0x%x ***\n", pInfo->nLogModePacketHeader);
    DBG("*** debug mode packet length = %d ***\n", pInfo->nLogModePacketLength);
    DBG("*** Type = 0x%x ***\n", pInfo->nType);
    DBG("*** My = %d ***\n", pInfo->nMy);
    DBG("*** Mx = %d ***\n", pInfo->nMx);
    DBG("*** Sd = %d ***\n", pInfo->nSd);
    DBG("*** Ss = %d ***\n", pInfo->nSs);
}

void DrvFwCtrlRestoreFirmwareModeToLogDataMode(void)
{
    DBG("*** %s() ***\n", __func__);

    // Since reset_hw() will reset the firmware mode to demo mode, we must reset the firmware mode again after reset_hw().
    if (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE && FIRMWARE_MODE_DEMO_MODE == DrvFwCtrlGetFirmwareMode())
    {
        g_FirmwareMode = DrvFwCtrlChangeFirmwareMode(FIRMWARE_MODE_DEBUG_MODE);
    }
    else
    {
        DBG("firmware mode is not restored\n");
    }
}	

#endif //CONFIG_ENABLE_FIRMWARE_DATA_LOG

//------------------------------------------------------------------------------//

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
void DrvFwCtrlCheckFirmwareUpdateBySwId(void)
{
    u32 nCrcMainA, nCrcInfoA, nCrcMainB, nCrcInfoB;
    u32 i;
    u16 nUpdateBinMajor = 0, nUpdateBinMinor = 0;
    u16 nMajor = 0, nMinor = 0;
    u8 *pVersion = NULL;
    Msg26xxmSwId_e eSwId = MSG26XXM_SW_ID_UNDEFINED;
    
    DBG("*** %s() ***\n", __func__);

    DrvPlatformLyrDisableFingerTouchReport();

    nCrcMainA = _DrvFwCtrlCalculateFirmwareCrcFromEFlash(EMEM_MAIN, 1);
    nCrcMainB = _DrvFwCtrlRetrieveFirmwareCrcFromMainBlock(EMEM_MAIN);

    nCrcInfoA = _DrvFwCtrlCalculateFirmwareCrcFromEFlash(EMEM_INFO, 1);
    nCrcInfoB = _DrvFwCtrlRetrieveFirmwareCrcFromMainBlock(EMEM_INFO);
    
    _gUpdateFirmwareBySwIdWorkQueue = create_singlethread_workqueue("update_firmware_by_sw_id");
    INIT_WORK(&_gUpdateFirmwareBySwIdWork, _DrvFwCtrlUpdateFirmwareBySwIdDoWork);

    DBG("nCrcMainA=0x%x, nCrcInfoA=0x%x, nCrcMainB=0x%x, nCrcInfoB=0x%x\n", nCrcMainA, nCrcInfoA, nCrcMainB, nCrcInfoB);
               
    if (nCrcMainA == nCrcMainB && nCrcInfoA == nCrcInfoB) // Case 1. Main Block:OK, Info Block:OK
    {
        eSwId = _DrvFwCtrlGetSwId(EMEM_MAIN);
    		
        if (eSwId == MSG26XXM_SW_ID_XXXX)
        {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
            nUpdateBinMajor = msg2xxx_xxxx_update_bin[0][0x2B]<<8 | msg2xxx_xxxx_update_bin[0][0x2A];
            nUpdateBinMinor = msg2xxx_xxxx_update_bin[0][0x2D]<<8 | msg2xxx_xxxx_update_bin[0][0x2C];
#else // By one dimensional array
            nUpdateBinMajor = msg2xxx_xxxx_update_bin[0x002B]<<8 | msg2xxx_xxxx_update_bin[0x002A];
            nUpdateBinMinor = msg2xxx_xxxx_update_bin[0x002D]<<8 | msg2xxx_xxxx_update_bin[0x002C];
#endif
        }
        else if (eSwId == MSG26XXM_SW_ID_YYYY)
        {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
            nUpdateBinMajor = msg2xxx_yyyy_update_bin[0][0x2B]<<8 | msg2xxx_yyyy_update_bin[0][0x2A];
            nUpdateBinMinor = msg2xxx_yyyy_update_bin[0][0x2D]<<8 | msg2xxx_yyyy_update_bin[0][0x2C];
#else // By one dimensional array
            nUpdateBinMajor = msg2xxx_yyyy_update_bin[0x002B]<<8 | msg2xxx_yyyy_update_bin[0x002A];
            nUpdateBinMinor = msg2xxx_yyyy_update_bin[0x002D]<<8 | msg2xxx_yyyy_update_bin[0x002C];
#endif
        }
        else //eSwId == MSG26XXM_SW_ID_UNDEFINED
        {
            DBG("eSwId = 0x%x is an undefined SW ID.\n", eSwId);

            eSwId = MSG26XXM_SW_ID_UNDEFINED;
            nUpdateBinMajor = 0;
            nUpdateBinMinor = 0;    		        						
        }
    		
        DrvFwCtrlGetCustomerFirmwareVersion(&nMajor, &nMinor, &pVersion);

        DBG("eSwId=0x%x, nMajor=%d, nMinor=%d, nUpdateBinMajor=%d, nUpdateBinMinor=%d\n", eSwId, nMajor, nMinor, nUpdateBinMajor, nUpdateBinMinor);

        if (nUpdateBinMinor > nMinor)
        {
            if (eSwId < MSG26XXM_SW_ID_UNDEFINED && eSwId != 0x0000 && eSwId != 0xFFFF)
            {
                if (eSwId == MSG26XXM_SW_ID_XXXX)
                {
                    for (i = 0; i < MSG26XXM_FIRMWARE_WHOLE_SIZE; i ++)
                    {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
                        _DrvFwCtrlStoreFirmwareData(msg2xxx_xxxx_update_bin[i], 1024);
#else // By one dimensional array
                        _DrvFwCtrlStoreFirmwareData(&(msg2xxx_xxxx_update_bin[i*1024]), 1024);
#endif
                    }
                }
                else if (eSwId == MSG26XXM_SW_ID_YYYY)
                {
                    for (i = 0; i < MSG26XXM_FIRMWARE_WHOLE_SIZE; i ++)
                    {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
                        _DrvFwCtrlStoreFirmwareData(msg2xxx_yyyy_update_bin[i], 1024);
#else // By one dimensional array
                        _DrvFwCtrlStoreFirmwareData(&(msg2xxx_yyyy_update_bin[i*1024]), 1024);
#endif
                    }
                }

                g_FwDataCount = 0; // Reset g_FwDataCount to 0 after copying update firmware data to temp buffer

                _gUpdateRetryCount = UPDATE_FIRMWARE_RETRY_COUNT;
                _gIsUpdateInfoBlockFirst = 1; // Set 1 for indicating main block is complete 
                _gIsUpdateFirmware = 0x11;
                queue_work(_gUpdateFirmwareBySwIdWorkQueue, &_gUpdateFirmwareBySwIdWork);
                return;
            }
            else
            {
                DBG("The sw id is invalid.\n");
                DBG("Go to normal boot up process.\n");
            }
        }
        else
        {
            DBG("The update bin version is older than or equal to the current firmware version on e-flash.\n");
            DBG("Go to normal boot up process.\n");
        }
    }
    else if (nCrcMainA == nCrcMainB && nCrcInfoA != nCrcInfoB) // Case 2. Main Block:OK, Info Block:FAIL
    {
        eSwId = _DrvFwCtrlGetSwId(EMEM_MAIN);
    		
        DBG("eSwId=0x%x\n", eSwId);

        if (eSwId < MSG26XXM_SW_ID_UNDEFINED && eSwId != 0x0000 && eSwId != 0xFFFF)
        {
            if (eSwId == MSG26XXM_SW_ID_XXXX)
            {
                for (i = 0; i < MSG26XXM_FIRMWARE_WHOLE_SIZE; i ++)
                {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
                    _DrvFwCtrlStoreFirmwareData(msg2xxx_xxxx_update_bin[i], 1024);
#else // By one dimensional array
                    _DrvFwCtrlStoreFirmwareData(&(msg2xxx_xxxx_update_bin[i*1024]), 1024);
#endif
                }
            }
            else if (eSwId == MSG26XXM_SW_ID_YYYY)
            {
                for (i = 0; i < MSG26XXM_FIRMWARE_WHOLE_SIZE; i ++)
                {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
                    _DrvFwCtrlStoreFirmwareData(msg2xxx_yyyy_update_bin[i], 1024);
#else // By one dimensional array
                    _DrvFwCtrlStoreFirmwareData(&(msg2xxx_yyyy_update_bin[i*1024]), 1024);
#endif
                }
            }

            g_FwDataCount = 0; // Reset g_FwDataCount to 0 after copying update firmware data to temp buffer

            _gUpdateRetryCount = UPDATE_FIRMWARE_RETRY_COUNT;
            _gIsUpdateInfoBlockFirst = 1; // Set 1 for indicating main block is complete 
            _gIsUpdateFirmware = 0x11;
            queue_work(_gUpdateFirmwareBySwIdWorkQueue, &_gUpdateFirmwareBySwIdWork);
            return;
        }
        else
        {
            DBG("The sw id is invalid.\n");
            DBG("Go to normal boot up process.\n");
        }
    }
    else // Case 3. Main Block:FAIL, Info Block:FAIL/OK
    {
        nCrcInfoA = _DrvFwCtrlRetrieveInfoCrcFromInfoBlock();
        nCrcInfoB = _DrvFwCtrlCalculateInfoCrcByDeviceDriver();
        
        DBG("8K-4 : nCrcInfoA=0x%x, nCrcInfoB=0x%x\n", nCrcInfoA, nCrcInfoB);

        if (nCrcInfoA == nCrcInfoB) // Check if info block is actually OK.
        {
            eSwId = _DrvFwCtrlGetSwId(EMEM_INFO);

            DBG("eSwId=0x%x\n", eSwId);

            if (eSwId < MSG26XXM_SW_ID_UNDEFINED && eSwId != 0x0000 && eSwId != 0xFFFF)
            {
                if (eSwId == MSG26XXM_SW_ID_XXXX)
                {
                    for (i = 0; i < MSG26XXM_FIRMWARE_WHOLE_SIZE; i ++)
                    {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
                        _DrvFwCtrlStoreFirmwareData(msg2xxx_xxxx_update_bin[i], 1024);
#else // By one dimensional array
                        _DrvFwCtrlStoreFirmwareData(&(msg2xxx_xxxx_update_bin[i*1024]), 1024);
#endif
                    }
                }
                else if (eSwId == MSG26XXM_SW_ID_YYYY)
                {
                    for (i = 0; i < MSG26XXM_FIRMWARE_WHOLE_SIZE; i ++)
                    {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
                        _DrvFwCtrlStoreFirmwareData(msg2xxx_yyyy_update_bin[i], 1024);
#else // By one dimensional array
                        _DrvFwCtrlStoreFirmwareData(&(msg2xxx_yyyy_update_bin[i*1024]), 1024);
#endif
                    }
                }

                g_FwDataCount = 0; // Reset g_FwDataCount to 0 after copying update firmware data to temp buffer

                _gUpdateRetryCount = UPDATE_FIRMWARE_RETRY_COUNT;
                _gIsUpdateInfoBlockFirst = 0; // Set 0 for indicating main block is broken 
                _gIsUpdateFirmware = 0x11;
                queue_work(_gUpdateFirmwareBySwIdWorkQueue, &_gUpdateFirmwareBySwIdWork);
                return;
            }
            else
            {
                DBG("The sw id is invalid.\n");
                DBG("Go to normal boot up process.\n");
            }
        }
        else
        {
            DBG("Info block is broken.\n");
        }
    }

    DrvPlatformLyrTouchDeviceResetHw(); 

    DrvPlatformLyrEnableFingerTouchReport();
}
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

void DrvFwCtrlHandleFingerTouch(void)
{
    TouchInfo_t tInfo;
    u32 i = 0;
    static u32 nLastKeyCode = 0xFF;
    static u32 nLastCount = 0;
    u8 *pPacket = NULL;
    u16 nReportPacketLength = 0;

//    DBG("*** %s() ***\n", __func__);
    
    memset(&tInfo, 0x0, sizeof(tInfo));

#ifdef CONFIG_ENABLE_FIRMWARE_DATA_LOG
    if (g_FirmwareMode == FIRMWARE_MODE_DEMO_MODE)
    {
        DBG("FIRMWARE_MODE_DEMO_MODE\n");

        nReportPacketLength = DEMO_MODE_PACKET_LENGTH;
        pPacket = g_DemoModePacket;
    }
    else if (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE)
    {
        DBG("FIRMWARE_MODE_DEBUG_MODE\n");

        if (g_FirmwareInfo.nLogModePacketHeader != 0xA5 && g_FirmwareInfo.nLogModePacketHeader != 0xAB && g_FirmwareInfo.nLogModePacketHeader != 0xA7)
        {
            DBG("WRONG DEBUG MODE HEADER : 0x%x\n", g_FirmwareInfo.nLogModePacketHeader);
            return;
        }

        if (g_LogModePacket == NULL)
        {
            g_LogModePacket = kzalloc(sizeof(u8)*g_FirmwareInfo.nLogModePacketLength, GFP_KERNEL);
        }
       
        nReportPacketLength = g_FirmwareInfo.nLogModePacketLength;
        pPacket = g_LogModePacket;
        mdelay(10);
    }
    else
    {
        DBG("WRONG FIRMWARE MODE : 0x%x\n", g_FirmwareMode);
        return;
    }
#else
    DBG("FIRMWARE_MODE_DEMO_MODE\n");

    nReportPacketLength = DEMO_MODE_PACKET_LENGTH;
    pPacket = g_DemoModePacket;
#endif //CONFIG_ENABLE_FIRMWARE_DATA_LOG

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
    if (g_GestureWakeupFlag == 1)
    {
        DBG("Set gesture wakeup packet length\n");

        nReportPacketLength = GESTURE_WAKEUP_PACKET_LENGTH;
        pPacket = _gGestureWakeupPacket;
    }
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
    if (g_GestureWakeupFlag == 1)
    {
        u32 i = 0, rc;
        
        while (i < 5)
        {
            mdelay(50);

            rc = IicReadData(SLAVE_I2C_ID_DWI2C, &pPacket[0], nReportPacketLength);
            
            if (rc > 0)
            {
                break;
            }
            
            i ++;
        }
    }
    else
    {
        IicReadData(SLAVE_I2C_ID_DWI2C, &pPacket[0], nReportPacketLength);
    }
#else
    IicReadData(SLAVE_I2C_ID_DWI2C, &pPacket[0], nReportPacketLength);
#endif //CONFIG_ENABLE_GESTURE_WAKEUP   
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
    IicReadData(SLAVE_I2C_ID_DWI2C, &pPacket[0], nReportPacketLength);
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaAlloc();
#endif //CONFIG_ENABLE_DMA_IIC
    IicReadData(SLAVE_I2C_ID_DWI2C, &pPacket[0], nReportPacketLength);
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaFree();
#endif //CONFIG_ENABLE_DMA_IIC
#endif

    if (0 == _DrvFwCtrlParsePacket(pPacket, nReportPacketLength, &tInfo))
    {
#ifdef CONFIG_TP_HAVE_KEY
        if (tInfo.nKeyCode != 0xFF)   //key touch pressed
        {
            DBG("tInfo.nKeyCode=%x, nLastKeyCode=%x, g_TpVirtualKey[%d]=%d\n", tInfo.nKeyCode, nLastKeyCode, tInfo.nKeyCode, g_TpVirtualKey[tInfo.nKeyCode]);
            
            if (tInfo.nKeyCode < MAX_KEY_NUM)
            {
                if (tInfo.nKeyCode != nLastKeyCode)
                {
                    DBG("key touch pressed\n");

                    input_report_key(g_InputDevice, BTN_TOUCH, 1);
                    input_report_key(g_InputDevice, g_TpVirtualKey[tInfo.nKeyCode], 1);

                    nLastKeyCode = tInfo.nKeyCode;
                }
                else
                {
                    /// pass duplicate key-pressing
                    DBG("REPEATED KEY\n");
                }
            }
            else
            {
                DBG("WRONG KEY\n");
            }
        }
        else                        //key touch released
        {
            if (nLastKeyCode != 0xFF)
            {
                DBG("key touch released\n");

                input_report_key(g_InputDevice, BTN_TOUCH, 0);
                input_report_key(g_InputDevice, g_TpVirtualKey[nLastKeyCode], 0);
                    
                nLastKeyCode = 0xFF;
            }
        }
#endif //CONFIG_TP_HAVE_KEY

        DBG("tInfo.nCount = %d, nLastCount = %d\n", tInfo.nCount, nLastCount);

        if (tInfo.nCount > 0)          //point touch pressed
        {
            for (i = 0; i < tInfo.nCount; i ++)
            {
                DrvPlatformLyrFingerTouchPressed(tInfo.tPoint[i].nX, tInfo.tPoint[i].nY, tInfo.tPoint[i].nP, tInfo.tPoint[i].nId);
            }

            nLastCount = tInfo.nCount;
        }
        else                        //point touch released
        {
            if (nLastCount > 0)
            {
                DrvPlatformLyrFingerTouchReleased(tInfo.tPoint[0].nX, tInfo.tPoint[0].nY);
                nLastCount = 0;
            }
        }

        input_sync(g_InputDevice);
    }
}

#endif //CONFIG_ENABLE_CHIP_MSG26XXM
