////////////////////////////////////////////////////////////////////////////////
// (c) MStar Semiconductor, Inc. 2006-2014
////////////////////////////////////////////////////////////////////////////////

/**
 *
 * @file    mstar_drv_self_fw_control.c
 *
 * @brief   This file defines the interface of touch screen
 *
 * @version v2.3.0.0
 *
 */
 
/*=============================================================*/
// INCLUDE FILE
/*=============================================================*/

#include "mstar_drv_self_fw_control.h"
#include "mstar_drv_utility_adaption.h"
#include "mstar_drv_platform_porting_layer.h"

#if defined(CONFIG_ENABLE_CHIP_MSG21XXA) || defined(CONFIG_ENABLE_CHIP_MSG22XX)

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

static u8 _gTpVendorCode[3] = {0};

static u8 _gDwIicInfoData[1024];
static u8 _gOneDimenFwData[MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE*1024+MSG22XX_FIRMWARE_INFO_BLOCK_SIZE] = {0}; // used for MSG22XX

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
/*
 * Note.
 * Please modify the name of the below .h depends on the vendor TP that you are using.
 */
#include "msg2xxx_HRC_update_bin.h"
#include "msg2xxx_DJ_update_bin.h"

static u32 _gUpdateRetryCount = UPDATE_FIRMWARE_RETRY_COUNT;
static struct work_struct _gUpdateFirmwareBySwIdWork;
static struct workqueue_struct *_gUpdateFirmwareBySwIdWorkQueue = NULL;

static u32 _gIsUpdateInfoBlockFirst = 0;
static u8 _gIsUpdateFirmware = 0x00;

#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
static u16 _gGestureWakeupValue = 0;
extern int g_gesture_enable;
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
u8 *_gGestureWakeupPacket = NULL;

u16 g_GestureWakeupMode = 0x0000;
u8 g_GestureWakeupFlag = 0;
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

/*=============================================================*/
// LOCAL FUNCTION DECLARATION
/*=============================================================*/

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
static void _DrvFwCtrlUpdateFirmwareBySwIdDoWork(struct work_struct *pWork);
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

/*=============================================================*/
// LOCAL FUNCTION DEFINITION
/*=============================================================*/

static void _DrvFwCtrlEraseEmemC32(void)
{
    DBG("*** %s() ***\n", __func__);

    /////////////////////////
    //Erase  all
    /////////////////////////
    
    // Enter gpio mode
    RegSet16BitValue(0x161E, 0xBEAF);

    // Before gpio mode, set the control pin as the orginal status
    RegSet16BitValue(0x1608, 0x0000);
    RegSetLByteValue(0x160E, 0x10);
    mdelay(10); 

    // ptrim = 1, h'04[2]
    RegSetLByteValue(0x1608, 0x04);
    RegSetLByteValue(0x160E, 0x10);
    mdelay(10); 

    // ptm = 6, h'04[12:14] = b'110
    RegSetLByteValue(0x1609, 0x60);
    RegSetLByteValue(0x160E, 0x10);

    // pmasi = 1, h'04[6]
    RegSetLByteValue(0x1608, 0x44);
    // pce = 1, h'04[11]
    RegSetLByteValue(0x1609, 0x68);
    // perase = 1, h'04[7]
    RegSetLByteValue(0x1608, 0xC4);
    // pnvstr = 1, h'04[5]
    RegSetLByteValue(0x1608, 0xE4);
    // pwe = 1, h'04[9]
    RegSetLByteValue(0x1609, 0x6A);
    // trigger gpio load
    RegSetLByteValue(0x160E, 0x10);
}

static void _DrvFwCtrlEraseEmemC33(EmemType_e eEmemType)
{
    DBG("*** %s() ***\n", __func__);

    // Stop mcu
    RegSet16BitValue(0x0FE6, 0x0001);

    // Disable watchdog
    RegSetLByteValue(0x3C60, 0x55);
    RegSetLByteValue(0x3C61, 0xAA);

    // Set PROGRAM password
    RegSetLByteValue(0x161A, 0xBA);
    RegSetLByteValue(0x161B, 0xAB);

    // Clear pce
    RegSetLByteValue(0x1618, 0x80);

    if (eEmemType == EMEM_ALL)
    {
        RegSetLByteValue(0x1608, 0x10); //mark
    }

    RegSetLByteValue(0x1618, 0x40);
    mdelay(10);

    RegSetLByteValue(0x1618, 0x80);

    // erase trigger
    if (eEmemType == EMEM_MAIN)
    {
        RegSetLByteValue(0x160E, 0x04); //erase main
    }
    else
    {
        RegSetLByteValue(0x160E, 0x08); //erase all block
    }
}

static void _DrvFwCtrlMsg22xxGetTpVendorCode(u8 *pTpVendorCode)
{
    DBG("*** %s() ***\n", __func__);

    if (g_ChipType == CHIP_TYPE_MSG22XX)
    {
        u16 nRegData1, nRegData2;

        DrvPlatformLyrTouchDeviceResetHw();
    
        DbBusEnterSerialDebugMode();
        DbBusStopMCU();
        DbBusIICUseBus();
        DbBusIICReshape();
        mdelay(100);
        
        // Stop mcu
        RegSetLByteValue(0x0FE6, 0x01); 

        // Stop watchdog
        RegSet16BitValue(0x3C60, 0xAA55);

        // RIU password
        RegSet16BitValue(0x161A, 0xABBA); 

        // Clear pce
        RegSet16BitValue(0x1618, (RegGet16BitValue(0x1618) | 0x80));

        RegSet16BitValue(0x1600, 0xC1E9); // Set start address for tp vendor code on info block(Actually, start reading from 0xC1E8)
    
        // Enable burst mode
//        RegSet16BitValue(0x160C, (RegGet16BitValue(0x160C) | 0x01));

        // Set pce
        RegSet16BitValue(0x1618, (RegGet16BitValue(0x1618) | 0x40)); 
    
        RegSetLByteValue(0x160E, 0x01); 

        nRegData1 = RegGet16BitValue(0x1604);
        nRegData2 = RegGet16BitValue(0x1606);

        pTpVendorCode[0] = ((nRegData1 >> 8) & 0xFF);
        pTpVendorCode[1] = (nRegData2 & 0xFF);
        pTpVendorCode[2] = ((nRegData2 >> 8) & 0xFF);

        DBG("pTpVendorCode[0] = 0x%x , %c \n", pTpVendorCode[0], pTpVendorCode[0]); 
        DBG("pTpVendorCode[1] = 0x%x , %c \n", pTpVendorCode[1], pTpVendorCode[1]); 
        DBG("pTpVendorCode[2] = 0x%x , %c \n", pTpVendorCode[2], pTpVendorCode[2]); 
        
        // Clear burst mode
//        RegSet16BitValue(0x160C, RegGet16BitValue(0x160C) & (~0x01));      

        RegSet16BitValue(0x1600, 0x0000); 

        // Clear RIU password
        RegSet16BitValue(0x161A, 0x0000); 

        DbBusIICNotUseBus();
        DbBusNotStopMCU();
        DbBusExitSerialDebugMode();

        DrvPlatformLyrTouchDeviceResetHw();
        mdelay(100);
    }
}

static void _DrvFwCtrlMsg22xxEraseEmem(EmemType_e eEmemType)
{
    u32 i;
    u16 nRegData = 0;
    
    DBG("*** %s() eEmemType = %d ***\n", __func__, eEmemType);

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    DBG("Erase start\n");

    // Stop mcu
    RegSet16BitValue(0x0FE6, 0x0001);

    // Disable watchdog
    RegSetLByteValue(0x3C60, 0x55);
    RegSetLByteValue(0x3C61, 0xAA);

    // Set PROGRAM password
    RegSetLByteValue(0x161A, 0xBA);
    RegSetLByteValue(0x161B, 0xAB);

    if (eEmemType == EMEM_ALL) // 48KB + 512Byte
    {
        DBG("Erase all block\n");

        // Clear pce
        RegSetLByteValue(0x1618, 0x80);
        mdelay(100);

        // Chip erase
        RegSet16BitValue(0x160E, BIT3);

        DBG("Wait erase done flag\n");

        do // Wait erase done flag
        {
            nRegData = RegGet16BitValue(0x1610); // Memory status
            mdelay(50);
        } while((nRegData & BIT1) != BIT1);
    }
    else if (eEmemType == EMEM_MAIN) // 48KB (32+8+8)
    {
        DBG("Erase main block\n");

        for (i = 0; i < 3; i ++)
        {
            // Clear pce
            RegSetLByteValue(0x1618, 0x80);
            mdelay(10);

            if (i == 0)
            {
                RegSet16BitValue(0x1600, 0x0000);
            }
            else if (i == 1)
            {
                RegSet16BitValue(0x1600, 0x8000);
            }
            else if (i == 2)
            {
                RegSet16BitValue(0x1600, 0xA000);
            }

            // Sector erase
            RegSet16BitValue(0x160E, (RegGet16BitValue(0x160E) | BIT2));

            DBG("Wait erase done flag\n");

            do // Wait erase done flag
            {
                nRegData = RegGet16BitValue(0x1610); // Memory status
                mdelay(50);
            } while((nRegData & BIT1) != BIT1);
        }   
    }
    else if (eEmemType == EMEM_INFO) // 512Byte
    {
        DBG("Erase info block\n");

        // Clear pce
        RegSetLByteValue(0x1618, 0x80);
        mdelay(10);

        RegSet16BitValue(0x1600, 0xC000);
        
        // Sector erase
        RegSet16BitValue(0x160E, (RegGet16BitValue(0x160E) | BIT2));

        DBG("Wait erase done flag\n");

        do // Wait erase done flag
        {
            nRegData = RegGet16BitValue(0x1610); // Memory status
            mdelay(50);
        } while((nRegData & BIT1) != BIT1);
    }
    
    DBG("Erase end\n");
    
    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();
}

static void _DrvFwCtrlMsg22xxProgramEmem(EmemType_e eEmemType) // For MSG22XX
{
    u32 i, j; 
    u32 nRemainSize = 0, nBlockSize = 0, nSize = 0, index = 0;
    u16 nRegData;
    u8 szDbBusTxData[1024] = {0};
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    u32 nSizePerWrite = 125;
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
    u32 nSizePerWrite = 1021;
#endif

    DBG("*** %s() eEmemType = %d ***\n", __func__, eEmemType);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaAlloc();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    // Hold reset pin before program
    RegSetLByteValue(0x1E06, 0x00);

    DBG("Program start\n");

    RegSet16BitValue(0x161A, 0xABBA);
    RegSet16BitValue(0x1618, (RegGet16BitValue(0x1618) | 0x80));

    if (eEmemType == EMEM_MAIN)
    {
        DBG("Program main block\n");

        RegSet16BitValue(0x1600, 0x0000); // Set start address of main block
        nRemainSize = MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024; //48KB
        index = 0;
    }
    else if (eEmemType == EMEM_INFO)
    {
        DBG("Program info block\n");

        RegSet16BitValue(0x1600, 0xC000); // Set start address of info block
        nRemainSize = MSG22XX_FIRMWARE_INFO_BLOCK_SIZE; //512Byte
        index = MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024;
    }
    else
    {
        DBG("eEmemType = %d is not supported for program e-memory.\n", eEmemType);
        return;
    }

    RegSet16BitValue(0x160C, (RegGet16BitValue(0x160C) | 0x01)); // Enable burst mode

    // Program start
    szDbBusTxData[0] = 0x10;
    szDbBusTxData[1] = 0x16;
    szDbBusTxData[2] = 0x02;

    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3);    

    szDbBusTxData[0] = 0x20;

    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 1);    

    i = 0;
    
    while (nRemainSize > 0)
    {
        if (nRemainSize > nSizePerWrite)
        {
            nBlockSize = nSizePerWrite;
        }
        else
        {
            nBlockSize = nRemainSize;
        }

        szDbBusTxData[0] = 0x10;
        szDbBusTxData[1] = 0x16;
        szDbBusTxData[2] = 0x02;

        nSize = 3;

        for (j = 0; j < nBlockSize; j ++)
        {
            szDbBusTxData[3+j] = _gOneDimenFwData[index+(i*nSizePerWrite)+j];
            nSize ++; 
        }
        i ++;

        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], nSize);

        nRemainSize = nRemainSize - nBlockSize;
    }

    // Program end
    szDbBusTxData[0] = 0x21;

    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 1);    

    nRegData = RegGet16BitValue(0x160C); 
    RegSet16BitValue(0x160C, nRegData & (~0x01));      

    DBG("Wait write done flag\n");

    // Polling 0x1610 is 0x0002
    do
    {
        nRegData = RegGet16BitValue(0x1610);
        nRegData = nRegData & BIT1;
        mdelay(10);

    } while (nRegData != BIT1); // Wait write done flag

    DBG("Program end\n");

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaFree();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
}

static u32 _DrvFwCtrlMsg22xxGetFirmwareCrcByHardware(EmemType_e eEmemType) // For MSG22XX
{
    u16 nCrcDown = 0;
    u32 nRetVal = 0; 

    DBG("*** %s() eEmemType = %d ***\n", __func__, eEmemType);

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);

    // RIU password
    RegSet16BitValue(0x161A, 0xABBA);      

    // Set PCE high
    RegSetLByteValue(0x1618, 0x40);      

    if (eEmemType == EMEM_MAIN)
    {
        // Set start address and end address for main block
        RegSet16BitValue(0x1600, 0x0000);      
        RegSet16BitValue(0x1640, 0xBFF8);      
    }
    else if (eEmemType == EMEM_INFO)
    {
        // Set start address and end address for info block
        RegSet16BitValue(0x1600, 0xC000);      
        RegSet16BitValue(0x1640, 0xC1F8);      
    }

    // CRC reset
    RegSet16BitValue(0x164E, 0x0001);      

    RegSet16BitValue(0x164E, 0x0000);   
    
    // Trigger CRC check
    RegSetLByteValue(0x160E, 0x20);   
    mdelay(10);
       
    nCrcDown = RegGet16BitValue(0x164E);
    
    while (nCrcDown != 2)
    {
        DBG("Wait CRC down\n");
        mdelay(10);
        nCrcDown = RegGet16BitValue(0x164E);
    }

    nRetVal = RegGet16BitValue(0x1652);
    nRetVal = (nRetVal << 16) | RegGet16BitValue(0x1650);

    DBG("Hardware CRC = 0x%x\n", nRetVal);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    return nRetVal;	
}

static void _DrvFwCtrlMsg22xxConvertFwDataTwoDimenToOneDimen(u8 szTwoDimenFwData[][1024], u8* pOneDimenFwData)
{
    u32 i, j;

    DBG("*** %s() ***\n", __func__);

    for (i = 0; i < (MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE+1); i ++)
    {
        if (i < MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE) // i < 48
        {
            for (j = 0; j < 1024; j ++)
            {
                pOneDimenFwData[i*1024+j] = szTwoDimenFwData[i][j];
            }
        }
        else // i == 48
        {
            for (j = 0; j < 512; j ++)
            {
                pOneDimenFwData[i*1024+j] = szTwoDimenFwData[i][j];
            }
        }
    }
}

static s32 _DrvFwCtrlParsePacket(u8 *pPacket, u16 nLength, TouchInfo_t *pInfo)
{
    u8 nCheckSum = 0;
    u32 nDeltaX = 0, nDeltaY = 0;
    u32 nX = 0;
    u32 nY = 0;
#ifdef CONFIG_SWAP_X_Y
    u32 nTempX;
    u32 nTempY;
#endif
#ifdef CONFIG_ENABLE_FIRMWARE_DATA_LOG
    u8 nCheckSumIndex = 0;
#endif //CONFIG_ENABLE_FIRMWARE_DATA_LOG

    DBG("*** %s() ***\n", __func__);

#ifdef CONFIG_ENABLE_FIRMWARE_DATA_LOG
    if (g_FirmwareMode == FIRMWARE_MODE_DEMO_MODE)
    {
        nCheckSumIndex = 7;
    }
    else if (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE || g_FirmwareMode == FIRMWARE_MODE_RAW_DATA_MODE)
    {
        nCheckSumIndex = 31;
    }

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
    if (g_GestureWakeupFlag == 1)
    {
        nCheckSumIndex = nLength-1;
    }
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#endif //CONFIG_ENABLE_FIRMWARE_DATA_LOG
    
#ifdef CONFIG_ENABLE_FIRMWARE_DATA_LOG
    nCheckSum = DrvCommonCalculateCheckSum(&pPacket[0], nCheckSumIndex);
    DBG("check sum : [%x] == [%x]? \n", pPacket[nCheckSumIndex], nCheckSum);
#else
    nCheckSum = DrvCommonCalculateCheckSum(&pPacket[0], (nLength-1));
    DBG("check ksum : [%x] == [%x]? \n", pPacket[nLength-1], nCheckSum);
#endif //CONFIG_ENABLE_FIRMWARE_DATA_LOG


#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
    if (g_GestureWakeupFlag == 1)
    {
        u8 nWakeupMode = 0;
        u8 bIsCorrectFormat = 0;

        DBG("received raw data from touch panel as following:\n");
        DBG("pPacket[0]=%x \n pPacket[1]=%x pPacket[2]=%x pPacket[3]=%x pPacket[4]=%x pPacket[5]=%x \n", \
            pPacket[0], pPacket[1], pPacket[2], pPacket[3], pPacket[4], pPacket[5]);

        if (g_ChipType == CHIP_TYPE_MSG22XX && pPacket[0] == 0xA7 && pPacket[1] == 0x00 && pPacket[2] == 0x06 && pPacket[3] == 0x50)
        {
            nWakeupMode = pPacket[4];
            bIsCorrectFormat = 1;
        } 
        else if (g_ChipType == CHIP_TYPE_MSG21XXA && pPacket[0] == 0x52 && pPacket[1] == 0xFF && pPacket[2] == 0xFF && pPacket[3] == 0xFF && pPacket[4] == 0xFF && pPacket[6] == 0xFF)
        {
            nWakeupMode = pPacket[5];
            bIsCorrectFormat = 1;
        }
        
        if (bIsCorrectFormat) 
        {
            DBG("nWakeupMode = 0x%x\n", nWakeupMode);

            switch (nWakeupMode)
            {
                case 0x58:
                    //printk("jzw,%s:gesture_enable=%d\n",__func__,g_gesture_enable);
                    if (g_gesture_enable) {
                        _gGestureWakeupValue = GESTURE_WAKEUP_MODE_DOUBLE_CLICK_FLAG;

                        DBG("Light up screen by DOUBLE_CLICK gesture wakeup.\n");

                        input_report_key(g_InputDevice, KEY_POWER, 1);
                        input_sync(g_InputDevice);
                        input_report_key(g_InputDevice, KEY_POWER, 0);
                        input_sync(g_InputDevice);
                    }
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
    DBG("pPacket[0]=%x \n pPacket[1]=%x pPacket[2]=%x pPacket[3]=%x pPacket[4]=%x \n pPacket[5]=%x pPacket[6]=%x pPacket[7]=%x \n", \
                pPacket[0], pPacket[1], pPacket[2], pPacket[3], pPacket[4], pPacket[5], pPacket[6], pPacket[7]);

#ifdef CONFIG_ENABLE_FIRMWARE_DATA_LOG
    if ((pPacket[nCheckSumIndex] == nCheckSum) && (pPacket[0] == 0x52))   // check the checksum of packet
#else
    if ((pPacket[nLength-1] == nCheckSum) && (pPacket[0] == 0x52))   // check the checksum of packet
#endif //CONFIG_ENABLE_FIRMWARE_DATA_LOG
    {

      
  #ifdef __TP_PROXIMITY_SUPPORT__
        if(pPacket[5] == 0x80&&(pPacket[1]==0xFF) &&(pPacket[4]==0xFF) ) // 接近触摸屏  =----->  灭屏
        {
          if((g_bPsSensorOpen == 1) && (g_nPsSensorDate == 1))
          {
            mutex_lock(&msg2133_sensor_mutex);
            g_nPsSensorDate = 0;
            mutex_unlock(&msg2133_sensor_mutex);
          }
          return 0;
                    
        }
        else if(pPacket[5] == 0x40&&(pPacket[1]==0xFF) &&(pPacket[4]==0xFF) ) // 远离触摸屏---->开屏
        {     
          if(/*(g_bPsSensorOpen == 1) && */(g_nPsSensorDate == 0))
          {
            mutex_lock(&msg2133_sensor_mutex);
            g_nPsSensorDate = 1;
            mutex_unlock(&msg2133_sensor_mutex);
          }
          return 0;
        }
#endif

        nX = (((pPacket[1] & 0xF0) << 4) | pPacket[2]);         // parse the packet to coordinate
        nY = (((pPacket[1] & 0x0F) << 8) | pPacket[3]);

        nDeltaX = (((pPacket[4] & 0xF0) << 4) | pPacket[5]);
        nDeltaY = (((pPacket[4] & 0x0F) << 8) | pPacket[6]);

        DBG("[x,y]=[%d,%d]\n", nX, nY);
        DBG("[delta_x,delta_y]=[%d,%d]\n", nDeltaX, nDeltaY);

#ifdef CONFIG_SWAP_X_Y
        nTempY = nX;
        nTempX = nY;
        nX = nTempX;
        nY = nTempY;
        
        nTempY = nDeltaX;
        nTempX = nDeltaY;
        nDeltaX = nTempX;
        nDeltaY = nTempY;
#endif

#ifdef CONFIG_REVERSE_X
        nX = 2047 - nX;
        nDeltaX = 4095 - nDeltaX;
#endif

#ifdef CONFIG_REVERSE_Y
        nY = 2047 - nY;
        nDeltaY = 4095 - nDeltaY;
#endif

        /*
         * pPacket[0]:id, pPacket[1]~pPacket[3]:the first point abs, pPacket[4]~pPacket[6]:the relative distance between the first point abs and the second point abs
         * when pPacket[1]~pPacket[4], pPacket[6] is 0xFF, keyevent, pPacket[5] to judge which key press.
         * pPacket[1]~pPacket[6] all are 0xFF, release touch
        */
        if ((pPacket[1] == 0xFF) && (pPacket[2] == 0xFF) && (pPacket[3] == 0xFF) && (pPacket[4] == 0xFF) && (pPacket[6] == 0xFF))
        {
            pInfo->tPoint[0].nX = 0; // final X coordinate
            pInfo->tPoint[0].nY = 0; // final Y coordinate

            if ((pPacket[5] != 0x00) && (pPacket[5] != 0xFF)) /* pPacket[5] is key value */
            {   /* 0x00 is key up, 0xff is touch screen up */
                DBG("touch key down pPacket[5]=%d\n", pPacket[5]);
                pInfo->nFingerNum = 1;
                pInfo->nTouchKeyCode = pPacket[5];
                pInfo->nTouchKeyMode = 1;

#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
                pInfo->nFingerNum = 1;
                pInfo->nTouchKeyCode = 0;
                pInfo->nTouchKeyMode = 0;

                if (pPacket[5] == 4) // TOUCH_KEY_HOME
                {
                    pInfo->tPoint[0].nX = g_TpVirtualKeyDimLocal[1][0];
                    pInfo->tPoint[0].nY = g_TpVirtualKeyDimLocal[1][1];
                }
                else if (pPacket[5] == 1) // TOUCH_KEY_MENU
                {
                    pInfo->tPoint[0].nX = g_TpVirtualKeyDimLocal[0][0];
                    pInfo->tPoint[0].nY = g_TpVirtualKeyDimLocal[0][1];
                }           
                else if (pPacket[5] == 2) // TOUCH_KEY_BACK
                {
                    pInfo->tPoint[0].nX = g_TpVirtualKeyDimLocal[2][0];
                    pInfo->tPoint[0].nY = g_TpVirtualKeyDimLocal[2][1];
                }           
                else if (pPacket[5] == 8) // TOUCH_KEY_SEARCH 
                {	
                    pInfo->tPoint[0].nX = g_TpVirtualKeyDimLocal[3][0];
                    pInfo->tPoint[0].nY = g_TpVirtualKeyDimLocal[3][1];
                }
                else
                {
                    DBG("multi-key is pressed.\n");

                    return -1;
                }
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
            }
            else
            {   /* key up or touch up */
                DBG("touch end\n");
                pInfo->nFingerNum = 0; //touch end
                pInfo->nTouchKeyCode = 0;
                pInfo->nTouchKeyMode = 0;    
            }
        }
        else
        {
            pInfo->nTouchKeyMode = 0; //Touch on screen...

//            if ((nDeltaX == 0) && (nDeltaY == 0))
          if(
#ifdef CONFIG_REVERSE_X
                (nDeltaX == 4095)
#else
                (nDeltaX == 0)
#endif
                &&
#ifdef CONFIG_REVERSE_Y
                (nDeltaY == 4095)
#else
                (nDeltaY == 0)
#endif
            )
            {   /* one touch point */
                pInfo->nFingerNum = 1; // one touch
                pInfo->tPoint[0].nX = (nX * TOUCH_SCREEN_X_MAX) / TPD_WIDTH;
                pInfo->tPoint[0].nY = (nY * TOUCH_SCREEN_Y_MAX) / TPD_HEIGHT;
                DBG("[%s]: [x,y]=[%d,%d]\n", __func__, nX, nY);
                DBG("[%s]: point[x,y]=[%d,%d]\n", __func__, pInfo->tPoint[0].nX, pInfo->tPoint[0].nY);
            }
            else
            {   /* two touch points */
                u32 nX2, nY2;
                
                pInfo->nFingerNum = 2; // two touch
                /* Finger 1 */
                pInfo->tPoint[0].nX = (nX * TOUCH_SCREEN_X_MAX) / TPD_WIDTH;
                pInfo->tPoint[0].nY = (nY * TOUCH_SCREEN_Y_MAX) / TPD_HEIGHT;
                DBG("[%s]: point1[x,y]=[%d,%d]\n", __func__, pInfo->tPoint[0].nX, pInfo->tPoint[0].nY);
                /* Finger 2 */
                if (nDeltaX > 2048)     // transform the unsigned value to signed value
                {
                    nDeltaX -= 4096;
                }
                
                if (nDeltaY > 2048)
                {
                    nDeltaY -= 4096;
                }

                nX2 = (u32)(nX + nDeltaX);
                nY2 = (u32)(nY + nDeltaY);

                pInfo->tPoint[1].nX = (nX2 * TOUCH_SCREEN_X_MAX) / TPD_WIDTH; 
                pInfo->tPoint[1].nY = (nY2 * TOUCH_SCREEN_Y_MAX) / TPD_HEIGHT;
                DBG("[%s]: point2[x,y]=[%d,%d]\n", __func__, pInfo->tPoint[1].nX, pInfo->tPoint[1].nY);
            }
        }
    }
#ifdef CONFIG_ENABLE_FIRMWARE_DATA_LOG
    else if (pPacket[nCheckSumIndex] == nCheckSum && pPacket[0] == 0x62)
    {
        nX = ((pPacket[1] << 8) | pPacket[2]);  // Position_X
        nY = ((pPacket[3] << 8) | pPacket[4]);  // Position_Y

        nDeltaX = ((pPacket[13] << 8) | pPacket[14]); // Distance_X
        nDeltaY = ((pPacket[15] << 8) | pPacket[16]); // Distance_Y

        DBG("[x,y]=[%d,%d]\n", nX, nY);
        DBG("[delta_x,delta_y]=[%d,%d]\n", nDeltaX, nDeltaY);

#ifdef CONFIG_SWAP_X_Y
        nTempY = nX;
        nTempX = nY;
        nX = nTempX;
        nY = nTempY;
        
        nTempY = nDeltaX;
        nTempX = nDeltaY;
        nDeltaX = nTempX;
        nDeltaY = nTempY;
#endif

#ifdef CONFIG_REVERSE_X
        nX = 2047 - nX;
        nDeltaX = 4095 - nDeltaX;
#endif

#ifdef CONFIG_REVERSE_Y
        nY = 2047 - nY;
        nDeltaY = 4095 - nDeltaY;
#endif

        /*
         * pPacket[0]:id, pPacket[1]~pPacket[4]:the first point abs, pPacket[13]~pPacket[16]:the relative distance between the first point abs and the second point abs
         * when pPacket[1]~pPacket[7] is 0xFF, keyevent, pPacket[8] to judge which key press.
         * pPacket[1]~pPacket[8] all are 0xFF, release touch
         */
        if ((pPacket[1] == 0xFF) && (pPacket[2] == 0xFF) && (pPacket[3] == 0xFF) && (pPacket[4] == 0xFF) && (pPacket[5] == 0xFF) && (pPacket[6] == 0xFF) && (pPacket[7] == 0xFF))
        {
            pInfo->tPoint[0].nX = 0; // final X coordinate
            pInfo->tPoint[0].nY = 0; // final Y coordinate

            if ((pPacket[8] != 0x00) && (pPacket[8] != 0xFF)) /* pPacket[8] is key value */
            {   /* 0x00 is key up, 0xff is touch screen up */
                DBG("touch key down pPacket[8]=%d\n", pPacket[8]);
                pInfo->nFingerNum = 1;
                pInfo->nTouchKeyCode = pPacket[8];
                pInfo->nTouchKeyMode = 1;

#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
                pInfo->nFingerNum = 1;
                pInfo->nTouchKeyCode = 0;
                pInfo->nTouchKeyMode = 0;

                if (pPacket[8] == 4) // TOUCH_KEY_HOME
                {
                    pInfo->tPoint[0].nX = g_TpVirtualKeyDimLocal[1][0];
                    pInfo->tPoint[0].nY = g_TpVirtualKeyDimLocal[1][1];
                }
                else if (pPacket[8] == 1) // TOUCH_KEY_MENU
                {
                    pInfo->tPoint[0].nX = g_TpVirtualKeyDimLocal[0][0];
                    pInfo->tPoint[0].nY = g_TpVirtualKeyDimLocal[0][1];
                }           
                else if (pPacket[8] == 2) // TOUCH_KEY_BACK
                {
                    pInfo->tPoint[0].nX = g_TpVirtualKeyDimLocal[2][0];
                    pInfo->tPoint[0].nY = g_TpVirtualKeyDimLocal[2][1];
                }           
                else if (pPacket[8] == 8) // TOUCH_KEY_SEARCH 
                {	
                    pInfo->tPoint[0].nX = g_TpVirtualKeyDimLocal[3][0];
                    pInfo->tPoint[0].nY = g_TpVirtualKeyDimLocal[3][1];
                }
                else
                {
                    DBG("multi-key is pressed.\n");

                    return -1;
                }
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
            }
            else
            {   /* key up or touch up */
                DBG("touch end\n");
                pInfo->nFingerNum = 0; //touch end
                pInfo->nTouchKeyCode = 0;
                pInfo->nTouchKeyMode = 0;    
            }
        }
        else
        {
            pInfo->nTouchKeyMode = 0; //Touch on screen...

//            if ((nDeltaX == 0) && (nDeltaY == 0))
            if (
#ifdef CONFIG_REVERSE_X
                (nDeltaX == 4095)
#else
                (nDeltaX == 0)
#endif
                &&
#ifdef CONFIG_REVERSE_Y
                (nDeltaY == 4095)
#else
                (nDeltaY == 0)
#endif
                )
            {   /* one touch point */
                pInfo->nFingerNum = 1; // one touch
                pInfo->tPoint[0].nX = (nX * TOUCH_SCREEN_X_MAX) / TPD_WIDTH;
                pInfo->tPoint[0].nY = (nY * TOUCH_SCREEN_Y_MAX) / TPD_HEIGHT;
                DBG("[%s]: [x,y]=[%d,%d]\n", __func__, nX, nY);
                DBG("[%s]: point[x,y]=[%d,%d]\n", __func__, pInfo->tPoint[0].nX, pInfo->tPoint[0].nY);
            }
            else
            {   /* two touch points */
                u32 nX2, nY2;
                
                pInfo->nFingerNum = 2; // two touch
                /* Finger 1 */
                pInfo->tPoint[0].nX = (nX * TOUCH_SCREEN_X_MAX) / TPD_WIDTH;
                pInfo->tPoint[0].nY = (nY * TOUCH_SCREEN_Y_MAX) / TPD_HEIGHT;
                DBG("[%s]: point1[x,y]=[%d,%d]\n", __func__, pInfo->tPoint[0].nX, pInfo->tPoint[0].nY);
                /* Finger 2 */
                if (nDeltaX > 2048)     // transform the unsigned value to signed value
                {
                    nDeltaX -= 4096;
                }
                
                if (nDeltaY > 2048)
                {
                    nDeltaY -= 4096;
                }

                nX2 = (u32)(nX + nDeltaX);
                nY2 = (u32)(nY + nDeltaY);

                pInfo->tPoint[1].nX = (nX2 * TOUCH_SCREEN_X_MAX) / TPD_WIDTH; 
                pInfo->tPoint[1].nY = (nY2 * TOUCH_SCREEN_Y_MAX) / TPD_HEIGHT;
                DBG("[%s]: point2[x,y]=[%d,%d]\n", __func__, pInfo->tPoint[1].nX, pInfo->tPoint[1].nY);
            }

            // Notify android application to retrieve log data mode packet from device driver by sysfs.   
            if (g_TouchKObj != NULL)
            {
                char *pEnvp[2];
                s32 nRetVal = 0; 

                pEnvp[0] = "STATUS=GET_PACKET";  
                pEnvp[1] = NULL;  
    
                nRetVal = kobject_uevent_env(g_TouchKObj, KOBJ_CHANGE, pEnvp); 
                DBG("kobject_uevent_env() nRetVal = %d\n", nRetVal);
            }
        }
    }
    else
    {
        if (pPacket[nCheckSumIndex] != nCheckSum)
        {
            DBG("WRONG CHECKSUM\n");
            return -1;
        }

        if (g_FirmwareMode == FIRMWARE_MODE_DEMO_MODE && pPacket[0] != 0x52)
        {
            DBG("WRONG DEMO MODE HEADER\n");
            return -1;
        }
        else if (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE && pPacket[0] != 0x62)
        {
            DBG("WRONG DEBUG MODE HEADER\n");
            return -1;
        }
        else if (g_FirmwareMode == FIRMWARE_MODE_RAW_DATA_MODE && pPacket[0] != 0x62)
        {
            DBG("WRONG RAW DATA MODE HEADER\n");
            return -1;
        }
    }
#else    
    else
    {
        DBG("pPacket[0]=0x%x, pPacket[7]=0x%x, nCheckSum=0x%x\n", pPacket[0], pPacket[7], nCheckSum);

        if (pPacket[nLength-1] != nCheckSum)
        {
            DBG("WRONG CHECKSUM\n");
            return -1;
        }

        if (pPacket[0] != 0x52)
        {
            DBG("WRONG DEMO MODE HEADER\n");
            return -1;
        }
    }
#endif //CONFIG_ENABLE_FIRMWARE_DATA_LOG

    return 0;
}

//------------------------------------------------------------------------------//

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID

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

//-------------------------Start of SW ID for MSG22XX----------------------------//

static u32 _DrvFwCtrlMsg22xxRetrieveFirmwareCrcFromEFlash(EmemType_e eEmemType) // For MSG22XX
{
    u32 nRetVal = 0; 
    u16 nRegData1 = 0, nRegData2 = 0;

    DBG("*** %s() eEmemType = %d ***\n", __func__, eEmemType);

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);

    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01); 

    // Stop watchdog
    RegSet16BitValue(0x3C60, 0xAA55);

    // RIU password
    RegSet16BitValue(0x161A, 0xABBA); 

    // Clear pce
    RegSet16BitValue(0x1618, (RegGet16BitValue(0x1618) | 0x80));

    if (eEmemType == EMEM_MAIN) // Read main block CRC(48KB-4) from main block
    {
        RegSet16BitValue(0x1600, 0xBFFC); // Set start address for main block CRC
    }
    else if (eEmemType == EMEM_INFO) // Read info block CRC(512Byte-4) from info block
    {
        RegSet16BitValue(0x1600, 0xC1FC); // Set start address for info block CRC
    }
    
    // Enable burst mode
    RegSet16BitValue(0x160C, (RegGet16BitValue(0x160C) | 0x01));

    // Set pce
    RegSet16BitValue(0x1618, (RegGet16BitValue(0x1618) | 0x40)); 
    
    RegSetLByteValue(0x160E, 0x01); 

    nRegData1 = RegGet16BitValue(0x1604);
    nRegData2 = RegGet16BitValue(0x1606);

    nRetVal  = ((nRegData2 >> 8) & 0xFF) << 24;
    nRetVal |= (nRegData2 & 0xFF) << 16;
    nRetVal |= ((nRegData1 >> 8) & 0xFF) << 8;
    nRetVal |= (nRegData1 & 0xFF);
    
    DBG("CRC = 0x%x\n", nRetVal);

    // Clear burst mode
    RegSet16BitValue(0x160C, RegGet16BitValue(0x160C) & (~0x01));      

    RegSet16BitValue(0x1600, 0x0000); 

    // Clear RIU password
    RegSet16BitValue(0x161A, 0x0000); 

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    return nRetVal;	
}

static u32 _DrvFwCtrlMsg22xxRetrieveFrimwareCrcFromBinFile(u8 szTmpBuf[], EmemType_e eEmemType) // For MSG22XX
{
    u32 nRetVal = 0; 
    
    DBG("*** %s() eEmemType = %d ***\n", __func__, eEmemType);
    
    if (szTmpBuf != NULL)
    {
        if (eEmemType == EMEM_MAIN) // Read main block CRC(48KB-4) from bin file
        {
            nRetVal  = szTmpBuf[0xBFFF] << 24;
            nRetVal |= szTmpBuf[0xBFFE] << 16;
            nRetVal |= szTmpBuf[0xBFFD] << 8;
            nRetVal |= szTmpBuf[0xBFFC];
        }
        else if (eEmemType == EMEM_INFO) // Read info block CRC(512Byte-4) from bin file
        {
            nRetVal  = szTmpBuf[0xC1FF] << 24;
            nRetVal |= szTmpBuf[0xC1FE] << 16;
            nRetVal |= szTmpBuf[0xC1FD] << 8;
            nRetVal |= szTmpBuf[0xC1FC];
        }
    }

    return nRetVal;
}

static u16 _DrvFwCtrlMsg22xxGetSwId(EmemType_e eEmemType) // For MSG22XX
{
    u16 nRetVal = 0; 
    u16 nRegData1 = 0;

    DBG("*** %s() eEmemType = %d ***\n", __func__, eEmemType);

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);

    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01); 

    // Stop watchdog
    RegSet16BitValue(0x3C60, 0xAA55);

    // RIU password
    RegSet16BitValue(0x161A, 0xABBA); 

    // Clear pce
    RegSet16BitValue(0x1618, (RegGet16BitValue(0x1618) | 0x80));

    if (eEmemType == EMEM_MAIN) // Read SW ID from main block
    {
        RegSet16BitValue(0x1600, 0xBFF4); // Set start address for main block SW ID
    }
    else if (eEmemType == EMEM_INFO) // Read SW ID from info block
    {
        RegSet16BitValue(0x1600, 0xC1EC); // Set start address for info block SW ID
    }

    /*
      Ex. SW ID in Main Block :
          Major low byte at address 0xBFF4
          Major high byte at address 0xBFF5
          
          SW ID in Info Block :
          Major low byte at address 0xC1EC
          Major high byte at address 0xC1ED
    */
    
    // Enable burst mode
//    RegSet16BitValue(0x160C, (RegGet16BitValue(0x160C) | 0x01));

    // Set pce
    RegSet16BitValue(0x1618, (RegGet16BitValue(0x1618) | 0x40)); 
    
    RegSetLByteValue(0x160E, 0x01); 

    nRegData1 = RegGet16BitValue(0x1604);
//    nRegData2 = RegGet16BitValue(0x1606);

    nRetVal = ((nRegData1 >> 8) & 0xFF) << 8;
    nRetVal |= (nRegData1 & 0xFF);
    
    // Clear burst mode
//    RegSet16BitValue(0x160C, RegGet16BitValue(0x160C) & (~0x01));      

    RegSet16BitValue(0x1600, 0x0000); 

    // Clear RIU password
    RegSet16BitValue(0x161A, 0x0000); 
    
    DBG("SW ID = 0x%x\n", nRetVal);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    return nRetVal;		
}

static s32 _DrvFwCtrlMsg22xxUpdateFirmwareBySwId(void) // For MSG22XX
{
    s32 nRetVal = -1;
    u32 nCrcInfoA = 0, nCrcInfoB = 0, nCrcMainA = 0, nCrcMainB = 0;
    
    DBG("*** %s() ***\n", __func__);
    
    DBG("_gIsUpdateInfoBlockFirst = %d, _gIsUpdateFirmware = 0x%x\n", _gIsUpdateInfoBlockFirst, _gIsUpdateFirmware);

    _DrvFwCtrlMsg22xxConvertFwDataTwoDimenToOneDimen(g_FwData, _gOneDimenFwData);
    
    if (_gIsUpdateInfoBlockFirst == 1)
    {
        if ((_gIsUpdateFirmware & 0x10) == 0x10)
        {
            _DrvFwCtrlMsg22xxEraseEmem(EMEM_INFO);
            _DrvFwCtrlMsg22xxProgramEmem(EMEM_INFO);
 
            nCrcInfoA = _DrvFwCtrlMsg22xxRetrieveFrimwareCrcFromBinFile(_gOneDimenFwData, EMEM_INFO);
            nCrcInfoB = _DrvFwCtrlMsg22xxGetFirmwareCrcByHardware(EMEM_INFO);

            DBG("nCrcInfoA = 0x%x, nCrcInfoB = 0x%x\n", nCrcInfoA, nCrcInfoB);
        
            if (nCrcInfoA == nCrcInfoB)
            {
                _DrvFwCtrlMsg22xxEraseEmem(EMEM_MAIN);
                _DrvFwCtrlMsg22xxProgramEmem(EMEM_MAIN);

                nCrcMainA = _DrvFwCtrlMsg22xxRetrieveFrimwareCrcFromBinFile(_gOneDimenFwData, EMEM_MAIN);
                nCrcMainB = _DrvFwCtrlMsg22xxGetFirmwareCrcByHardware(EMEM_MAIN);

                DBG("nCrcMainA = 0x%x, nCrcMainB = 0x%x\n", nCrcMainA, nCrcMainB);
        		
                if (nCrcMainA == nCrcMainB)
                {
                    _gIsUpdateFirmware = 0x00;
                    nRetVal = 0;
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
            _DrvFwCtrlMsg22xxEraseEmem(EMEM_MAIN);
            _DrvFwCtrlMsg22xxProgramEmem(EMEM_MAIN);

            nCrcMainA = _DrvFwCtrlMsg22xxRetrieveFrimwareCrcFromBinFile(_gOneDimenFwData, EMEM_MAIN);
            nCrcMainB = _DrvFwCtrlMsg22xxGetFirmwareCrcByHardware(EMEM_MAIN);

            DBG("nCrcMainA=0x%x, nCrcMainB=0x%x\n", nCrcMainA, nCrcMainB);
    		
            if (nCrcMainA == nCrcMainB)
            {
                _gIsUpdateFirmware = 0x00;
                nRetVal = 0;
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
            _DrvFwCtrlMsg22xxEraseEmem(EMEM_MAIN);
            _DrvFwCtrlMsg22xxProgramEmem(EMEM_MAIN);

            nCrcMainA = _DrvFwCtrlMsg22xxRetrieveFrimwareCrcFromBinFile(_gOneDimenFwData, EMEM_MAIN);
            nCrcMainB = _DrvFwCtrlMsg22xxGetFirmwareCrcByHardware(EMEM_MAIN);

            DBG("nCrcMainA=0x%x, nCrcMainB=0x%x\n", nCrcMainA, nCrcMainB);

            if (nCrcMainA == nCrcMainB)
            {
                _DrvFwCtrlMsg22xxEraseEmem(EMEM_INFO);
                _DrvFwCtrlMsg22xxProgramEmem(EMEM_INFO);

                nCrcInfoA = _DrvFwCtrlMsg22xxRetrieveFrimwareCrcFromBinFile(_gOneDimenFwData, EMEM_INFO);
                nCrcInfoB = _DrvFwCtrlMsg22xxGetFirmwareCrcByHardware(EMEM_INFO);
                
                DBG("nCrcInfoA=0x%x, nCrcInfoB=0x%x\n", nCrcInfoA, nCrcInfoB);

                if (nCrcInfoA == nCrcInfoB)
                {
                    _gIsUpdateFirmware = 0x00;
                    nRetVal = 0;
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
            _DrvFwCtrlMsg22xxEraseEmem(EMEM_INFO);
            _DrvFwCtrlMsg22xxProgramEmem(EMEM_INFO);

            nCrcInfoA = _DrvFwCtrlMsg22xxRetrieveFrimwareCrcFromBinFile(_gOneDimenFwData, EMEM_INFO);
            nCrcInfoB = _DrvFwCtrlMsg22xxGetFirmwareCrcByHardware(EMEM_INFO);

            DBG("nCrcInfoA=0x%x, nCrcInfoB=0x%x\n", nCrcInfoA, nCrcInfoB);

            if (nCrcInfoA == nCrcInfoB)
            {
                _gIsUpdateFirmware = 0x00;
                nRetVal = 0;
            }
            else
            {
                _gIsUpdateFirmware = 0x01;
            }
        }    		
    }
    
    return nRetVal;	
}

void _DrvFwCtrlMsg22xxCheckFirmwareUpdateBySwId(void) // For MSG22XX
{
    u32 nCrcMainA, nCrcInfoA, nCrcMainB, nCrcInfoB;
    u32 i;
    u16 nUpdateBinMajor = 0, nUpdateBinMinor = 0;
    u16 nMajor = 0, nMinor = 0;
    u8 *pVersion = NULL;
    Msg22xxSwId_e eSwId = MSG22XX_SW_ID_UNDEFINED;
    
    DBG("*** %s() ***\n", __func__);

    DrvPlatformLyrDisableFingerTouchReport();

    nCrcMainA = _DrvFwCtrlMsg22xxGetFirmwareCrcByHardware(EMEM_MAIN);
    nCrcMainB = _DrvFwCtrlMsg22xxRetrieveFirmwareCrcFromEFlash(EMEM_MAIN);

    nCrcInfoA = _DrvFwCtrlMsg22xxGetFirmwareCrcByHardware(EMEM_INFO);
    nCrcInfoB = _DrvFwCtrlMsg22xxRetrieveFirmwareCrcFromEFlash(EMEM_INFO);
    
    _gUpdateFirmwareBySwIdWorkQueue = create_singlethread_workqueue("update_firmware_by_sw_id");
    INIT_WORK(&_gUpdateFirmwareBySwIdWork, _DrvFwCtrlUpdateFirmwareBySwIdDoWork);

    DBG("nCrcMainA=0x%x, nCrcInfoA=0x%x, nCrcMainB=0x%x, nCrcInfoB=0x%x\n", nCrcMainA, nCrcInfoA, nCrcMainB, nCrcInfoB);
               
    if (nCrcMainA == nCrcMainB && nCrcInfoA == nCrcInfoB) // Case 1. Main Block:OK, Info Block:OK
    {
        eSwId = _DrvFwCtrlMsg22xxGetSwId(EMEM_MAIN);
    		
        if (eSwId == MSG22XX_SW_ID_HRC)
        {
            nUpdateBinMajor = msg2xxx_HRC_update_bin[0xBFF5]<<8 | msg2xxx_HRC_update_bin[0xBFF4];
            nUpdateBinMinor = msg2xxx_HRC_update_bin[0xBFF7]<<8 | msg2xxx_HRC_update_bin[0xBFF6];
        }
		
        else if (eSwId == MSG22XX_SW_ID_DJ)
        {
            nUpdateBinMajor = msg2xxx_DJ_update_bin[0xBFF5]<<8 | msg2xxx_DJ_update_bin[0xBFF4];
            nUpdateBinMinor = msg2xxx_DJ_update_bin[0xBFF7]<<8 | msg2xxx_DJ_update_bin[0xBFF6];
        }
        
        else //eSwId == MSG22XX_SW_ID_UNDEFINED
        {
            DBG("eSwId = 0x%x is an undefined SW ID.\n", eSwId);

            eSwId = MSG22XX_SW_ID_UNDEFINED;
            nUpdateBinMajor = 0;
            nUpdateBinMinor = 0;    		        						
        }
    		
        DrvFwCtrlGetCustomerFirmwareVersion(&nMajor, &nMinor, &pVersion);

        DBG("eSwId=0x%x, nMajor=%d, nMinor=%d, nUpdateBinMajor=%d, nUpdateBinMinor=%d\n", eSwId, nMajor, nMinor, nUpdateBinMajor, nUpdateBinMinor);

        if (nUpdateBinMinor > nMinor)
        {
            if (eSwId < MSG22XX_SW_ID_UNDEFINED && eSwId != 0x0000 && eSwId != 0xFFFF)
            {
                if (eSwId == MSG22XX_SW_ID_HRC)
                {
                    for (i = 0; i < (MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE+1); i ++)
                    {
                        if (i < MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE) // i < 48
                        {
                            _DrvFwCtrlStoreFirmwareData(&(msg2xxx_HRC_update_bin[i*1024]), 1024);
                        }
                        else // i == 48
                        {
                            _DrvFwCtrlStoreFirmwareData(&(msg2xxx_HRC_update_bin[i*1024]), 512);
                        }
                    }
                }
				
                else if (eSwId == MSG22XX_SW_ID_DJ)
                {
                    for (i = 0; i < (MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE+1); i ++)
                    {
                        if (i < MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE) // i < 48
                        {
                            _DrvFwCtrlStoreFirmwareData(&(msg2xxx_DJ_update_bin[i*1024]), 1024);
                        }
                        else // i == 48
                        {
                            _DrvFwCtrlStoreFirmwareData(&(msg2xxx_DJ_update_bin[i*1024]), 512);
                        }
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
        eSwId = _DrvFwCtrlMsg22xxGetSwId(EMEM_MAIN);
    		
        DBG("eSwId=0x%x\n", eSwId);

        if (eSwId < MSG22XX_SW_ID_UNDEFINED && eSwId != 0x0000 && eSwId != 0xFFFF)
        {
            if (eSwId == MSG22XX_SW_ID_HRC)
            {
                for (i = 0; i < (MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE+1); i ++)
                {
                    if (i < MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE) // i < 48
                    {
                        _DrvFwCtrlStoreFirmwareData(&(msg2xxx_HRC_update_bin[i*1024]), 1024);
                    }
                    else // i == 48
                    {
                        _DrvFwCtrlStoreFirmwareData(&(msg2xxx_HRC_update_bin[i*1024]), 512);
                    }
                }
            }
			
            else if (eSwId == MSG22XX_SW_ID_DJ)
            {
                for (i = 0; i < (MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE+1); i ++)
                {
                    if (i < MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE) // i < 48
                    {
                        _DrvFwCtrlStoreFirmwareData(&(msg2xxx_DJ_update_bin[i*1024]), 1024);
                    }
                    else // i == 48
                    {
                        _DrvFwCtrlStoreFirmwareData(&(msg2xxx_DJ_update_bin[i*1024]), 512);
                    }
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
    else if (nCrcMainA != nCrcMainB && nCrcInfoA == nCrcInfoB) // Case 3. Main Block:FAIL, Info Block:OK
    {
        eSwId = _DrvFwCtrlMsg22xxGetSwId(EMEM_INFO);
		
        DBG("eSwId=0x%x\n", eSwId);

        if (eSwId < MSG22XX_SW_ID_UNDEFINED && eSwId != 0x0000 && eSwId != 0xFFFF)
        {
            if (eSwId == MSG22XX_SW_ID_HRC)
            {
                for (i = 0; i < (MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE+1); i ++)
                {
                    if (i < MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE) // i < 48
                    {
                        _DrvFwCtrlStoreFirmwareData(&(msg2xxx_HRC_update_bin[i*1024]), 1024);
                    }
                    else // i == 48
                    {
                        _DrvFwCtrlStoreFirmwareData(&(msg2xxx_HRC_update_bin[i*1024]), 512);
                    }
                }
            }
			
            else if (eSwId == MSG22XX_SW_ID_DJ)
            {
                for (i = 0; i < (MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE+1); i ++)
                {
                    if (i < MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE) // i < 48
                    {
                        _DrvFwCtrlStoreFirmwareData(&(msg2xxx_DJ_update_bin[i*1024]), 1024);
                    }
                    else // i == 48
                    {
                        _DrvFwCtrlStoreFirmwareData(&(msg2xxx_DJ_update_bin[i*1024]), 512);
                    }
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
    else // Case 4. Main Block:FAIL, Info Block:FAIL
    {
        DBG("Main block and Info block are broken.\n");
        DBG("Go to normal boot up process.\n");
    }

    DrvPlatformLyrTouchDeviceResetHw();

    DrvPlatformLyrEnableFingerTouchReport();
}

//-------------------------End of SW ID for MSG22XX----------------------------//

//-------------------------Start of SW ID for MSG21XXA----------------------------//

static u32 _DrvFwCtrlMsg21xxaCalculateMainCrcFromEFlash(void) // For MSG21XXA
{
    u32 nRetVal = 0; 
    u16 nRegData = 0;

    DBG("*** %s() ***\n", __func__);
    
    DrvPlatformLyrTouchDeviceResetHw();//showlo
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);

    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073

    // Stop Watchdog
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
//        DBG("*** reg(0x3C, 0xE4) = 0x%x ***\n", nRegData); // add for debug

    } while (nRegData != 0x9432);

    // Read calculated main block CRC from register
    nRetVal = RegGet16BitValue(0x3C80);
    nRetVal = (nRetVal << 16) | RegGet16BitValue(0x3C82);
        
    DBG("Main Block CRC = 0x%x\n", nRetVal);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    return nRetVal;	
}

static u32 _DrvFwCtrlMsg21xxaRetrieveMainCrcFromMainBlock(void) // For MSG21XXA
{
    u32 nRetVal = 0; 
    u16 nRegData = 0;
    u8 szDbBusTxData[5] = {0};
    u8 szDbBusRxData[4] = {0};

    DBG("*** %s() ***\n", __func__);
    DrvPlatformLyrTouchDeviceResetHw();//showlo
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

     // Read main block CRC from main block
    szDbBusTxData[0] = 0x72;
    szDbBusTxData[1] = 0x7F;
    szDbBusTxData[2] = 0xFC;
    szDbBusTxData[3] = 0x00;
    szDbBusTxData[4] = 0x04;
    
    IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 5);
    IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 4);

    nRetVal = szDbBusRxData[0];
    nRetVal = (nRetVal << 8) | szDbBusRxData[1];
    nRetVal = (nRetVal << 8) | szDbBusRxData[2];
    nRetVal = (nRetVal << 8) | szDbBusRxData[3];
   
    DBG("CRC = 0x%x\n", nRetVal);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    return nRetVal;	
}

static u16 _DrvFwCtrlMsg21xxaGetSwId(EmemType_e eEmemType) // For MSG21XXA
{
    u16 nRetVal = 0; 
    u16 nRegData = 0;
    u8 szDbBusTxData[5] = {0};
    u8 szDbBusRxData[4] = {0};

    DBG("*** %s() eEmemType = %d ***\n", __func__, eEmemType);
    DrvPlatformLyrTouchDeviceResetHw();//showlo
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
        szDbBusTxData[1] = 0x7F;
        szDbBusTxData[2] = 0x55;
    }
    else if (eEmemType == EMEM_INFO) // Read SW ID from info block
    {
        szDbBusTxData[1] = 0x83;
        szDbBusTxData[2] = 0x00;
    }
    szDbBusTxData[3] = 0x00;
    szDbBusTxData[4] = 0x04;

    IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 5);
    IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 4);

    DBG("szDbBusRxData[0,1,2,3] = 0x%x,0x%x,0x%x,0x%x\n", szDbBusRxData[0], szDbBusRxData[1], szDbBusRxData[2], szDbBusRxData[3]);

    if ((szDbBusRxData[0] >= 0x30 && szDbBusRxData[0] <= 0x39)
        &&(szDbBusRxData[1] >= 0x30 && szDbBusRxData[1] <= 0x39)
        &&(szDbBusRxData[2] >= 0x31 && szDbBusRxData[2] <= 0x39))  
    {
        nRetVal = (szDbBusRxData[0]-0x30)*100+(szDbBusRxData[1]-0x30)*10+(szDbBusRxData[2]-0x30);
    }
    
    DBG("SW ID = 0x%x\n", nRetVal);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    return nRetVal;		
}		

static s32 _DrvFwCtrlMsg21xxaUpdateFirmwareBySwId(u8 szFwData[][1024], EmemType_e eEmemType) // For MSG21XXA //showlo
{
    u32 i, j, nCalculateCrcSize;
    u32 nCrcMain = 0, nCrcMainTp = 0;
    u32 nCrcInfo = 0, nCrcInfoTp = 0;
    u32 nCrcTemp = 0;
    u16 nRegData = 0;

    DBG("*** %s() ***\n", __func__);

    nCrcMain = 0xffffffff;
    nCrcInfo = 0xffffffff;

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaAlloc();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
    // before erase setting-showlo
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(300);

    // erase main
    _DrvFwCtrlEraseEmemC33(EMEM_MAIN);
    
    mdelay(1000);
    DrvPlatformLyrTouchDeviceResetHw();
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(300);

    /////////////////////////
    // Program
    /////////////////////////

    // Polling 0x3CE4 is 0x1C70
    if ((eEmemType == EMEM_ALL) || (eEmemType == EMEM_MAIN))
    {
        do
        {
            nRegData = RegGet16BitValue(0x3CE4);
        } while (nRegData != 0x1C70);
    }

    switch (eEmemType)
    {
        case EMEM_ALL:
            RegSet16BitValue(0x3CE4, 0xE38F);  // for all blocks
            break;
        case EMEM_MAIN:
            RegSet16BitValue(0x3CE4, 0x7731);  // for main block
            break;
        case EMEM_INFO:
            RegSet16BitValue(0x3CE4, 0x7731);  // for info block

            RegSetLByteValue(0x0FE6, 0x01);

            RegSetLByteValue(0x3CE4, 0xC5); 
            RegSetLByteValue(0x3CE5, 0x78); 

            RegSetLByteValue(0x1E04, 0x9F);
            RegSetLByteValue(0x1E05, 0x82);

            RegSetLByteValue(0x0FE6, 0x00);
            mdelay(100);
            break;
    }

    // Polling 0x3CE4 is 0x2F43
    do
    {
        nRegData = RegGet16BitValue(0x3CE4);
    } while (nRegData != 0x2F43);

    // Calculate CRC 32
    DrvCommonCrcInitTable();

    if (eEmemType == EMEM_ALL)
    {
        nCalculateCrcSize = MSG21XXA_FIRMWARE_WHOLE_SIZE;
    }
    else if (eEmemType == EMEM_MAIN)
    {
        nCalculateCrcSize = MSG21XXA_FIRMWARE_MAIN_BLOCK_SIZE;
    }
    else if (eEmemType == EMEM_INFO)
    {
        nCalculateCrcSize = MSG21XXA_FIRMWARE_INFO_BLOCK_SIZE;
    }
    else
    {
        nCalculateCrcSize = 0;
    }
		
    for (i = 0; i < nCalculateCrcSize; i ++)
    {
        if (eEmemType == EMEM_INFO)
        {
            i = 32;
        }

        if (i < 32)   // emem_main
        {
            if (i == 31)
            {
                szFwData[i][1014] = 0x5A;
                szFwData[i][1015] = 0xA5;

                for (j = 0; j < 1016; j ++)
                {
                    nCrcMain = DrvCommonCrcGetValue(szFwData[i][j], nCrcMain);
                }

                nCrcTemp = nCrcMain;
                nCrcTemp = nCrcTemp ^ 0xffffffff;

                DBG("nCrcTemp=%x\n", nCrcTemp); // add for debug

                for (j = 0; j < 4; j ++)
                {
                    szFwData[i][1023-j] = ((nCrcTemp>>(8*j)) & 0xFF);

                    DBG("((nCrcTemp>>(8*%d)) & 0xFF)=%x\n", j, ((nCrcTemp>>(8*j)) & 0xFF)); // add for debug
                    DBG("Update main clock crc32 into bin buffer szFwData[%d][%d]=%x\n", i, (1020+j), szFwData[i][1020+j]);
                }
            }
            else
            {
                for (j = 0; j < 1024; j ++)
                {
                    nCrcMain = DrvCommonCrcGetValue(szFwData[i][j], nCrcMain);
                }
            }
        }
        else  // emem_info
        {
            for (j = 0; j < 1024; j ++)
            {
                nCrcInfo = DrvCommonCrcGetValue(szFwData[i][j], nCrcInfo);
            }
            
            if (eEmemType == EMEM_MAIN)
            {
                break;
            }
        }

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
        for (j = 0; j < 128; j ++)
        {
                IicWriteData(SLAVE_I2C_ID_DWI2C, &szFwData[i][j*8], 8);
        }
#else //showlo
        for (j = 0; j < 8; j ++)
        {
                IicWriteData(SLAVE_I2C_ID_DWI2C, &szFwData[i][j*128], 128);
        }
#endif
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
        IicWriteData(SLAVE_I2C_ID_DWI2C, szFwData[i], 1024);
#endif

        // Polling 0x3CE4 is 0xD0BC
        do
        {
            nRegData = RegGet16BitValue(0x3CE4);
        } while (nRegData != 0xD0BC);

        RegSet16BitValue(0x3CE4, 0x2F43);
    }

    if ((eEmemType == EMEM_ALL) || (eEmemType == EMEM_MAIN))
    {
        // write file done and check crc
        RegSet16BitValue(0x3CE4, 0x1380);
    }
    mdelay(10);

    if ((eEmemType == EMEM_ALL) || (eEmemType == EMEM_MAIN))
    {
        // Polling 0x3CE4 is 0x9432
        do
        {
            nRegData = RegGet16BitValue(0x3CE4);
        } while (nRegData != 0x9432);
    }

    nCrcMain = nCrcMain ^ 0xffffffff;
    nCrcInfo = nCrcInfo ^ 0xffffffff;

    if ((eEmemType == EMEM_ALL) || (eEmemType == EMEM_MAIN))
    {
        // CRC Main from TP
        nCrcMainTp = RegGet16BitValue(0x3C80);
        nCrcMainTp = (nCrcMainTp << 16) | RegGet16BitValue(0x3C82);
    }

    if (eEmemType == EMEM_ALL)
    {
        // CRC Info from TP
        nCrcInfoTp = RegGet16BitValue(0x3CA0);
        nCrcInfoTp = (nCrcInfoTp << 16) | RegGet16BitValue(0x3CA2);
    }

    DBG("nCrcMain=0x%x, nCrcInfo=0x%x, nCrcMainTp=0x%x, nCrcInfoTp=0x%x\n", nCrcMain, nCrcInfo, nCrcMainTp, nCrcInfoTp);

    g_FwDataCount = 0; // Reset g_FwDataCount to 0 after update firmware

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    DrvPlatformLyrTouchDeviceResetHw();
    
#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaFree();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    if ((eEmemType == EMEM_ALL) || (eEmemType == EMEM_MAIN))
    {
        if (nCrcMainTp != nCrcMain)
        {
            DBG("Update FAILED\n");

            return -1;
        }
    }

    if (eEmemType == EMEM_ALL)
    {
        if (nCrcInfoTp != nCrcInfo)
        {
            DBG("Update FAILED\n");

            return -1;
        }
    }

    DBG("Update SUCCESS\n");

    return 0;
} 

void _DrvFwCtrlMsg21xxaCheckFirmwareUpdateBySwId(void) // For MSG21XXA
{
    u32 nCrcMainA, nCrcMainB;
    u32 i;
    u16 nUpdateBinMajor = 0, nUpdateBinMinor = 0;
    u16 nMajor = 0, nMinor = 0;
    u8 nIsCompareVersion = 0;
    u8 *pVersion = NULL; 
    Msg21xxaSwId_e eMainSwId = MSG21XXA_SW_ID_UNDEFINED, eInfoSwId = MSG21XXA_SW_ID_UNDEFINED, eSwId = MSG21XXA_SW_ID_UNDEFINED;

    DBG("*** %s() ***\n", __func__);

    DrvPlatformLyrDisableFingerTouchReport();

    nCrcMainA = _DrvFwCtrlMsg21xxaCalculateMainCrcFromEFlash();
    nCrcMainB = _DrvFwCtrlMsg21xxaRetrieveMainCrcFromMainBlock();

    _gUpdateFirmwareBySwIdWorkQueue = create_singlethread_workqueue("update_firmware_by_sw_id");
    INIT_WORK(&_gUpdateFirmwareBySwIdWork, _DrvFwCtrlUpdateFirmwareBySwIdDoWork);

    DBG("nCrcMainA=0x%x, nCrcMainB=0x%x\n", nCrcMainA, nCrcMainB);
               
    if (nCrcMainA == nCrcMainB) 
    {
        eMainSwId = _DrvFwCtrlMsg21xxaGetSwId(EMEM_MAIN);
        eInfoSwId = _DrvFwCtrlMsg21xxaGetSwId(EMEM_INFO);
    		
        DBG("Check firmware integrity success\n");
        DBG("eMainSwId=0x%x, eInfoSwId=0x%x\n", eMainSwId, eInfoSwId);

        if (eMainSwId == eInfoSwId)
        {
        		eSwId = eMainSwId;
        		nIsCompareVersion = 1;
        }
        else
        {
        		eSwId = eInfoSwId;
        		nIsCompareVersion = 0;
        }
        
        if (eSwId == MSG21XXA_SW_ID_XXXX)//21xxA version addr (4 byte): major low byte 7F4E hight byte 7F4F  minor low byte7F50 7F51
        {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
            nUpdateBinMajor = msg2xxx_HRC_update_bin[31][0x34F]<<8 | msg2xxx_HRC_update_bin[31][0x34E];
            nUpdateBinMinor = msg2xxx_HRC_update_bin[31][0x351]<<8 | msg2xxx_HRC_update_bin[31][0x350];
#else // By one dimensional array
            nUpdateBinMajor = msg2xxx_HRC_update_bin[0x7F4F]<<8 | msg2xxx_HRC_update_bin[0x7F4E];
            nUpdateBinMinor = msg2xxx_HRC_update_bin[0x7F51]<<8 | msg2xxx_HRC_update_bin[0x7F50];
#endif
        }
        else if (eSwId == MSG21XXA_SW_ID_YYYY)
        {
      
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
            nUpdateBinMajor = msg2xxx_DJ_update_bin[31][0x34F]<<8 | msg2xxx_DJ_update_bin[31][0x34E];
            nUpdateBinMinor = msg2xxx_DJ_update_bin[31][0x351]<<8 | msg2xxx_DJ_update_bin[31][0x350];
#else // By one dimensional array
            nUpdateBinMajor = msg2xxx_DJ_update_bin[0x7F4F]<<8 | msg2xxx_DJ_update_bin[0x7F4E];
            nUpdateBinMinor = msg2xxx_DJ_update_bin[0x7F51]<<8 | msg2xxx_DJ_update_bin[0x7F50];
#endif

        }
        else //eSwId == MSG21XXA_SW_ID_UNDEFINED
        {
            DBG("eSwId = 0x%x is an undefined SW ID.\n", eSwId);

            eSwId = MSG21XXA_SW_ID_UNDEFINED;
            nUpdateBinMajor = 0;
            nUpdateBinMinor = 0;    		        						
        }

        DrvFwCtrlGetCustomerFirmwareVersion(&nMajor, &nMinor, &pVersion);
    		        
        DBG("eSwId=0x%x, nMajor=%d, nMinor=%d, nUpdateBinMajor=%d, nUpdateBinMinor=%d\n", eSwId, nMajor, nMinor, nUpdateBinMajor, nUpdateBinMinor);

        if ((nUpdateBinMinor > nMinor && nIsCompareVersion == 1) || (nIsCompareVersion == 0))
        {
            if (eSwId < MSG21XXA_SW_ID_UNDEFINED && eSwId != 0xFFFF)
            {
                if (eSwId == MSG21XXA_SW_ID_XXXX)
                {
                    for (i = 0; i < MSG21XXA_FIRMWARE_MAIN_BLOCK_SIZE; i ++)
                    {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
                        _DrvFwCtrlStoreFirmwareData(msg2xxx_HRC_update_bin[i], 1024);
#else // By one dimensional array
                        _DrvFwCtrlStoreFirmwareData(&(msg2xxx_HRC_update_bin[i*1024]), 1024);
#endif
                    }
                }
                else if (eSwId == MSG21XXA_SW_ID_YYYY)
                {
                    for (i = 0; i < MSG21XXA_FIRMWARE_MAIN_BLOCK_SIZE; i ++)
                    {
                   /*
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
                        _DrvFwCtrlStoreFirmwareData(msg2xxx_yyyy_update_bin[i], 1024);
#else // By one dimensional array
                        _DrvFwCtrlStoreFirmwareData(&(msg2xxx_yyyy_update_bin[i*1024]), 1024);
#endif
*/
                    }
                }

                g_FwDataCount = 0; // Reset g_FwDataCount to 0 after copying update firmware data to temp buffer

                _gUpdateRetryCount = UPDATE_FIRMWARE_RETRY_COUNT;
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
    else
    {
        eSwId = _DrvFwCtrlMsg21xxaGetSwId(EMEM_INFO);
    		
        DBG("Check firmware integrity failed\n");
        DBG("eSwId=0x%x\n", eSwId);

        if (eSwId < MSG21XXA_SW_ID_UNDEFINED && eSwId != 0xFFFF)
        {
            if (eSwId == MSG21XXA_SW_ID_XXXX)
            {
                for (i = 0; i < MSG21XXA_FIRMWARE_MAIN_BLOCK_SIZE; i ++)
                {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
                    _DrvFwCtrlStoreFirmwareData(msg2xxx_HRC_update_bin[i], 1024);
#else // By one dimensional array
                    _DrvFwCtrlStoreFirmwareData(&(msg2xxx_HRC_update_bin[i*1024]), 1024);
#endif
                }
            }
            else if (eSwId == MSG21XXA_SW_ID_YYYY)
            {
                for (i = 0; i < MSG21XXA_FIRMWARE_MAIN_BLOCK_SIZE; i ++)
                {
                /*
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
                    _DrvFwCtrlStoreFirmwareData(msg2xxx_yyyy_update_bin[i], 1024);
#else // By one dimensional array
                    _DrvFwCtrlStoreFirmwareData(&(msg2xxx_yyyy_update_bin[i*1024]), 1024);
#endif
*/
                }
            }

            g_FwDataCount = 0; // Reset g_FwDataCount to 0 after copying update firmware data to temp buffer

            _gUpdateRetryCount = UPDATE_FIRMWARE_RETRY_COUNT;
            queue_work(_gUpdateFirmwareBySwIdWorkQueue, &_gUpdateFirmwareBySwIdWork);
            return;
        }
        else
        {
            DBG("The sw id is invalid.\n");
            DBG("Go to normal boot up process.\n");
        }
    }

    DrvPlatformLyrTouchDeviceResetHw();
    
    DrvPlatformLyrEnableFingerTouchReport();
}

//-------------------------End of SW ID for MSG21XXA----------------------------//

static void _DrvFwCtrlUpdateFirmwareBySwIdDoWork(struct work_struct *pWork)
{
    s32 nRetVal = 0;
    
    DBG("*** %s() _gUpdateRetryCount = %d ***\n", __func__, _gUpdateRetryCount);
    
    DrvPlatformLyrTouchDeviceResetHw();//showlo

    if (g_ChipType == CHIP_TYPE_MSG21XXA)   
    {
        nRetVal = _DrvFwCtrlMsg21xxaUpdateFirmwareBySwId(g_FwData, EMEM_MAIN);
    }
    else if (g_ChipType == CHIP_TYPE_MSG22XX)    
    {
        nRetVal = _DrvFwCtrlMsg22xxUpdateFirmwareBySwId();
    }
    else
    {
        DBG("This chip type (%d) does not support update firmware by sw id\n", g_ChipType);

        DrvPlatformLyrTouchDeviceResetHw(); 

        DrvPlatformLyrEnableFingerTouchReport();

        nRetVal = -1;
        return;
    }
    
    DBG("*** update firmware by sw id result = %d ***\n", nRetVal);
    
    if (nRetVal == 0)
    {
        DBG("update firmware by sw id success\n");

        DrvPlatformLyrTouchDeviceResetHw();

        DrvPlatformLyrEnableFingerTouchReport();

        if (g_ChipType == CHIP_TYPE_MSG22XX)    
        {
            _gIsUpdateInfoBlockFirst = 0;
            _gIsUpdateFirmware = 0x00;
        }
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

            DrvPlatformLyrTouchDeviceResetHw();

            DrvPlatformLyrEnableFingerTouchReport();

            if (g_ChipType == CHIP_TYPE_MSG22XX)    
            {
                _gIsUpdateInfoBlockFirst = 0;
                _gIsUpdateFirmware = 0x00;
            }
        }
    }
}

#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

//------------------------------------------------------------------------------//

static void _DrvFwCtrlReadInfoC33(void)
{
    u8 szDbBusTxData[5] = {0};
    u16 nRegData = 0;
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    u32 i;
#endif 

    DBG("*** %s() ***\n", __func__);
    
    mdelay(300);

    // Stop Watchdog
    RegSetLByteValue(0x3C60, 0x55);
    RegSetLByteValue(0x3C61, 0xAA);

    RegSet16BitValue(0x3CE4, 0xA4AB);

    RegSet16BitValue(0x1E04, 0x7d60);

    // TP SW reset
    RegSet16BitValue(0x1E04, 0x829F);
    mdelay(1);
    
    szDbBusTxData[0] = 0x10;
    szDbBusTxData[1] = 0x0F;
    szDbBusTxData[2] = 0xE6;
    szDbBusTxData[3] = 0x00;
    IicWriteData(SLAVE_I2C_ID_DBBUS, szDbBusTxData, 4);    
    mdelay(100);

    do
    {
        nRegData = RegGet16BitValue(0x3CE4);
    } while (nRegData != 0x5B58);

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    szDbBusTxData[0] = 0x72;
    szDbBusTxData[3] = 0x00;
    szDbBusTxData[4] = 0x80; // read 128 bytes

    for (i = 0; i < 8; i ++)
    {
        szDbBusTxData[1] = 0x80 + (((i*128)&0xff00)>>8);
        szDbBusTxData[2] = (i*128)&0x00ff;

        IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 5);

        mdelay(50);

        // Receive info data
        IicReadData(SLAVE_I2C_ID_DWI2C, &_gDwIicInfoData[i*128], 128);
    }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
    szDbBusTxData[0] = 0x72;
    szDbBusTxData[1] = 0x80;
    szDbBusTxData[2] = 0x00;
    szDbBusTxData[3] = 0x04; // read 1024 bytes
    szDbBusTxData[4] = 0x00;
    
    IicWriteData(SLAVE_I2C_ID_DWI2C, szDbBusTxData, 5);

    mdelay(50);

    // Receive info data
    IicReadData(SLAVE_I2C_ID_DWI2C, &_gDwIicInfoData[0], 1024);
#endif
}

static s32 _DrvFwCtrlUpdateFirmwareC32(u8 szFwData[][1024], EmemType_e eEmemType)
{
    u32 i, j;
    u32 nCrcMain, nCrcMainTp;
    u32 nCrcInfo, nCrcInfoTp;
    u32 nCrcTemp;
    u16 nRegData = 0;

    DBG("*** %s() ***\n", __func__);

    nCrcMain = 0xffffffff;
    nCrcInfo = 0xffffffff;

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaAlloc();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    /////////////////////////
    // Erase  all
    /////////////////////////
    _DrvFwCtrlEraseEmemC32();
    mdelay(1000); 

    DrvPlatformLyrTouchDeviceResetHw();

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(300);

    // Reset watch dog
    RegSetLByteValue(0x3C60, 0x55);
    RegSetLByteValue(0x3C61, 0xAA);

    /////////////////////////
    // Program
    /////////////////////////

    // Polling 0x3CE4 is 0x1C70
    do
    {
        nRegData = RegGet16BitValue(0x3CE4);
    } while (nRegData != 0x1C70);

    RegSet16BitValue(0x3CE4, 0xE38F);  // for all-blocks

    // Polling 0x3CE4 is 0x2F43
    do
    {
        nRegData = RegGet16BitValue(0x3CE4);
    } while (nRegData != 0x2F43);

    // Calculate CRC 32
    DrvCommonCrcInitTable();

    for (i = 0; i < 33; i ++) // total  33 KB : 2 byte per R/W
    {
        if (i < 32)   // emem_main
        {
            if (i == 31)
            {
                szFwData[i][1014] = 0x5A;
                szFwData[i][1015] = 0xA5;

                for (j = 0; j < 1016; j ++)
                {
                    nCrcMain = DrvCommonCrcGetValue(szFwData[i][j], nCrcMain);
                }

                nCrcTemp = nCrcMain;
                nCrcTemp = nCrcTemp ^ 0xffffffff;

                DBG("nCrcTemp=%x\n", nCrcTemp); // add for debug

                for (j = 0; j < 4; j ++)
                {
                    szFwData[i][1023-j] = ((nCrcTemp>>(8*j)) & 0xFF);

                    DBG("((nCrcTemp>>(8*%d)) & 0xFF)=%x\n", j, ((nCrcTemp>>(8*j)) & 0xFF)); // add for debug
                    DBG("Update main clock crc32 into bin buffer szFwData[%d][%d]=%x\n", i, (1020+j), szFwData[i][1020+j]);
                }
            }
            else
            {
                for (j = 0; j < 1024; j ++)
                {
                    nCrcMain = DrvCommonCrcGetValue(szFwData[i][j], nCrcMain);
                }
            }
        }
        else  // emem_info
        {
            for (j = 0; j < 1024; j ++)
            {
                nCrcInfo = DrvCommonCrcGetValue(szFwData[i][j], nCrcInfo);
            }
        }

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
        for (j = 0; j < 8; j ++)
        {
            IicWriteData(SLAVE_I2C_ID_DWI2C, &szFwData[i][j*128], 128);
        }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
        IicWriteData(SLAVE_I2C_ID_DWI2C, szFwData[i], 1024);
#endif

        // Polling 0x3CE4 is 0xD0BC
        do
        {
            nRegData = RegGet16BitValue(0x3CE4);
        } while (nRegData != 0xD0BC);

        RegSet16BitValue(0x3CE4, 0x2F43);
    }

    // Write file done
    RegSet16BitValue(0x3CE4, 0x1380);

    mdelay(10); 
    // Polling 0x3CE4 is 0x9432
    do
    {
        nRegData = RegGet16BitValue(0x3CE4);
    } while (nRegData != 0x9432);

    nCrcMain = nCrcMain ^ 0xffffffff;
    nCrcInfo = nCrcInfo ^ 0xffffffff;

    // CRC Main from TP
    nCrcMainTp = RegGet16BitValue(0x3C80);
    nCrcMainTp = (nCrcMainTp << 16) | RegGet16BitValue(0x3C82);
 
    // CRC Info from TP
    nCrcInfoTp = RegGet16BitValue(0x3CA0);
    nCrcInfoTp = (nCrcInfoTp << 16) | RegGet16BitValue(0x3CA2);

    DBG("nCrcMain=0x%x, nCrcInfo=0x%x, nCrcMainTp=0x%x, nCrcInfoTp=0x%x\n",
               nCrcMain, nCrcInfo, nCrcMainTp, nCrcInfoTp);

    g_FwDataCount = 0; // Reset g_FwDataCount to 0 after update firmware

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    DrvPlatformLyrTouchDeviceResetHw();

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaFree();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    if ((nCrcMainTp != nCrcMain) || (nCrcInfoTp != nCrcInfo))
    {
        DBG("Update FAILED\n");

        return -1;
    }

    DBG("Update SUCCESS\n");

    return 0;
}

static s32 _DrvFwCtrlUpdateFirmwareC33(u8 szFwData[][1024], EmemType_e eEmemType)
{
    u8 szLifeCounter[2];
    u32 i, j;
    u32 nCrcMain, nCrcMainTp;
    u32 nCrcInfo, nCrcInfoTp;
    u32 nCrcTemp;
    u16 nRegData = 0;

    DBG("*** %s() ***\n", __func__);

    nCrcMain = 0xffffffff;
    nCrcInfo = 0xffffffff;

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaAlloc();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    _DrvFwCtrlReadInfoC33();

    if (_gDwIicInfoData[0] == 'M' && _gDwIicInfoData[1] == 'S' && _gDwIicInfoData[2] == 'T' && _gDwIicInfoData[3] == 'A' && _gDwIicInfoData[4] == 'R' && _gDwIicInfoData[5] == 'T' && _gDwIicInfoData[6] == 'P' && _gDwIicInfoData[7] == 'C')
    {
        _gDwIicInfoData[8] = szFwData[32][8];
        _gDwIicInfoData[9] = szFwData[32][9];
        _gDwIicInfoData[10] = szFwData[32][10];
        _gDwIicInfoData[11] = szFwData[32][11];
        // updata life counter
        szLifeCounter[1] = ((((_gDwIicInfoData[13] << 8) | _gDwIicInfoData[12]) + 1) >> 8) & 0xFF;
        szLifeCounter[0] = (((_gDwIicInfoData[13] << 8) | _gDwIicInfoData[12]) + 1) & 0xFF;
        _gDwIicInfoData[12] = szLifeCounter[0];
        _gDwIicInfoData[13] = szLifeCounter[1];
        
        RegSet16BitValue(0x3CE4, 0x78C5);
        RegSet16BitValue(0x1E04, 0x7d60);
        // TP SW reset
        RegSet16BitValue(0x1E04, 0x829F);

        mdelay(50);

        // Polling 0x3CE4 is 0x2F43
        do
        {
            nRegData = RegGet16BitValue(0x3CE4);
        } while (nRegData != 0x2F43);

        // Transmit lk info data
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
        for (j = 0; j < 8; j ++)
        {
            IicWriteData(SLAVE_I2C_ID_DWI2C, &_gDwIicInfoData[j*128], 128);
        }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
        IicWriteData(SLAVE_I2C_ID_DWI2C, &_gDwIicInfoData[0], 1024);
#endif

        // Polling 0x3CE4 is 0xD0BC
        do
        {
            nRegData = RegGet16BitValue(0x3CE4);
        } while (nRegData != 0xD0BC);
    }

    // erase main
    _DrvFwCtrlEraseEmemC33(EMEM_MAIN);
    mdelay(1000);

    DrvPlatformLyrTouchDeviceResetHw();

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(300);

    /////////////////////////
    // Program
    /////////////////////////

    // Polling 0x3CE4 is 0x1C70
    if ((eEmemType == EMEM_ALL) || (eEmemType == EMEM_MAIN))
    {
        do
        {
            nRegData = RegGet16BitValue(0x3CE4);
        } while (nRegData != 0x1C70);
    }

    switch (eEmemType)
    {
        case EMEM_ALL:
            RegSet16BitValue(0x3CE4, 0xE38F);  // for all blocks
            break;
        case EMEM_MAIN:
            RegSet16BitValue(0x3CE4, 0x7731);  // for main block
            break;
        case EMEM_INFO:
            RegSet16BitValue(0x3CE4, 0x7731);  // for info block

            RegSetLByteValue(0x0FE6, 0x01);

            RegSetLByteValue(0x3CE4, 0xC5); 
            RegSetLByteValue(0x3CE5, 0x78); 

            RegSetLByteValue(0x1E04, 0x9F);
            RegSetLByteValue(0x1E05, 0x82);

            RegSetLByteValue(0x0FE6, 0x00);
            mdelay(100);
            break;
    }

    // Polling 0x3CE4 is 0x2F43
    do
    {
        nRegData = RegGet16BitValue(0x3CE4);
    } while (nRegData != 0x2F43);

    // Calculate CRC 32
    DrvCommonCrcInitTable();

    for (i = 0; i < 33; i ++) // total 33 KB : 2 byte per R/W
    {
        if (eEmemType == EMEM_INFO)
        {
            i = 32;
        }

        if (i < 32)   // emem_main
        {
            if (i == 31)
            {
                szFwData[i][1014] = 0x5A;
                szFwData[i][1015] = 0xA5;

                for (j = 0; j < 1016; j ++)
                {
                    nCrcMain = DrvCommonCrcGetValue(szFwData[i][j], nCrcMain);
                }

                nCrcTemp = nCrcMain;
                nCrcTemp = nCrcTemp ^ 0xffffffff;

                DBG("nCrcTemp=%x\n", nCrcTemp); // add for debug

                for (j = 0; j < 4; j ++)
                {
                    szFwData[i][1023-j] = ((nCrcTemp>>(8*j)) & 0xFF);

                    DBG("((nCrcTemp>>(8*%d)) & 0xFF)=%x\n", j, ((nCrcTemp>>(8*j)) & 0xFF)); // add for debug
                    DBG("Update main clock crc32 into bin buffer szFwData[%d][%d]=%x\n", i, (1020+j), szFwData[i][1020+j]); // add for debug
                }
            }
            else
            {
                for (j = 0; j < 1024; j ++)
                {
                    nCrcMain = DrvCommonCrcGetValue(szFwData[i][j], nCrcMain);
                }
            }
        }
        else  // emem_info
        {
            for (j = 0; j < 1024; j ++)
            {
                nCrcInfo = DrvCommonCrcGetValue(_gDwIicInfoData[j], nCrcInfo);
            }
            
            if (eEmemType == EMEM_MAIN)
            {
                break;
            }
        }

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
        for (j = 0; j < 8; j ++)
        {
            IicWriteData(SLAVE_I2C_ID_DWI2C, &szFwData[i][j*128], 128);
        }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
        IicWriteData(SLAVE_I2C_ID_DWI2C, szFwData[i], 1024);
#endif

        // Polling 0x3CE4 is 0xD0BC
        do
        {
            nRegData = RegGet16BitValue(0x3CE4);
        } while (nRegData != 0xD0BC);

        RegSet16BitValue(0x3CE4, 0x2F43);
    }

    if ((eEmemType == EMEM_ALL) || (eEmemType == EMEM_MAIN))
    {
        // write file done and check crc
        RegSet16BitValue(0x3CE4, 0x1380);
    }
    mdelay(10);

    if ((eEmemType == EMEM_ALL) || (eEmemType == EMEM_MAIN))
    {
        // Polling 0x3CE4 is 0x9432
        do
        {
            nRegData = RegGet16BitValue(0x3CE4);
        } while (nRegData != 0x9432);
    }

    nCrcMain = nCrcMain ^ 0xffffffff;
    nCrcInfo = nCrcInfo ^ 0xffffffff;

    if ((eEmemType == EMEM_ALL) || (eEmemType == EMEM_MAIN))
    {
        // CRC Main from TP
        nCrcMainTp = RegGet16BitValue(0x3C80);
        nCrcMainTp = (nCrcMainTp << 16) | RegGet16BitValue(0x3C82);

        // CRC Info from TP
        nCrcInfoTp = RegGet16BitValue(0x3CA0);
        nCrcInfoTp = (nCrcInfoTp << 16) | RegGet16BitValue(0x3CA2);
    }
    DBG("nCrcMain=0x%x, nCrcInfo=0x%x, nCrcMainTp=0x%x, nCrcInfoTp=0x%x\n", nCrcMain, nCrcInfo, nCrcMainTp, nCrcInfoTp);

    g_FwDataCount = 0; // Reset g_FwDataCount to 0 after update firmware

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    DrvPlatformLyrTouchDeviceResetHw();

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaFree();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    if ((eEmemType == EMEM_ALL) || (eEmemType == EMEM_MAIN))
    {
        if ((nCrcMainTp != nCrcMain) || (nCrcInfoTp != nCrcInfo))
        {
            DBG("Update FAILED\n");

            return -1;
        }
    }
    
    DBG("Update SUCCESS\n");

    return 0;
}

static s32 _DrvFwCtrlMsg22xxUpdateFirmware(u8 szFwData[][1024], EmemType_e eEmemType)
{
    u32 i, index;
    u32 nCrcMain, nCrcMainTp;
    u32 nCrcInfo, nCrcInfoTp;
    u32 nRemainSize, nBlockSize, nSize;
    u16 nRegData = 0;
    u8 szDbBusTxData[1024] = {0};
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    u32 nSizePerWrite = 125;
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
    u32 nSizePerWrite = 1021;
#endif

    DBG("*** %s() ***\n", __func__);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaAlloc();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    _DrvFwCtrlMsg22xxConvertFwDataTwoDimenToOneDimen(szFwData, _gOneDimenFwData);

    DrvPlatformLyrTouchDeviceResetHw();

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    
    DBG("Erase start\n");

    // Stop mcu
    RegSet16BitValue(0x0FE6, 0x0001);

    // Disable watchdog
    RegSetLByteValue(0x3C60, 0x55);
    RegSetLByteValue(0x3C61, 0xAA);

    // Set PROGRAM password
    RegSetLByteValue(0x161A, 0xBA);
    RegSetLByteValue(0x161B, 0xAB);

    if (eEmemType == EMEM_ALL) // 48KB + 512Byte
    {
        DBG("Erase all block\n");

        // Clear pce
        RegSetLByteValue(0x1618, 0x80);
        mdelay(100);

        // Chip erase
        RegSet16BitValue(0x160E, BIT3);

        DBG("Wait erase done flag\n");

        do // Wait erase done flag
        {
            nRegData = RegGet16BitValue(0x1610); // Memory status
            mdelay(50);
        } while((nRegData & BIT1) != BIT1);
    }
    else if (eEmemType == EMEM_MAIN) // 48KB (32+8+8)
    {
        DBG("Erase main block\n");

        for (i = 0; i < 3; i ++)
        {
            // Clear pce
            RegSetLByteValue(0x1618, 0x80);
            mdelay(10);

            if (i == 0)
            {
                RegSet16BitValue(0x1600, 0x0000);
            }
            else if (i == 1)
            {
                RegSet16BitValue(0x1600, 0x8000);
            }
            else if (i == 2)
            {
                RegSet16BitValue(0x1600, 0xA000);
            }

            // Sector erase
            RegSet16BitValue(0x160E, (RegGet16BitValue(0x160E) | BIT2));

            DBG("Wait erase done flag\n");

            do // Wait erase done flag
            {
                nRegData = RegGet16BitValue(0x1610); // Memory status
                mdelay(50);
            } while((nRegData & BIT1) != BIT1);
        }   
    }
    else if (eEmemType == EMEM_INFO) // 512Byte
    {
        DBG("Erase info block\n");

        // Clear pce
        RegSetLByteValue(0x1618, 0x80);
        mdelay(10);

        RegSet16BitValue(0x1600, 0xC000);
        
        // Sector erase
        RegSet16BitValue(0x160E, (RegGet16BitValue(0x160E) | BIT2));

        DBG("Wait erase done flag\n");

        do // Wait erase done flag
        {
            nRegData = RegGet16BitValue(0x1610); // Memory status
            mdelay(50);
        } while((nRegData & BIT1) != BIT1);
    }
    
    DBG("Erase end\n");
    
    // Hold reset pin before program
    RegSetLByteValue(0x1E06, 0x00);

    /////////////////////////
    // Program
    /////////////////////////

    if (eEmemType == EMEM_ALL || eEmemType == EMEM_MAIN) // 48KB
    {
        DBG("Program main block start\n");
		
        // Program main block
        RegSet16BitValue(0x161A, 0xABBA);
        RegSet16BitValue(0x1618, (RegGet16BitValue(0x1618) | 0x80));
		
        RegSet16BitValue(0x1600, 0x0000); // Set start address of main block
        RegSet16BitValue(0x160C, (RegGet16BitValue(0x160C) | 0x01)); // Enable burst mode
		
        // Program start
        szDbBusTxData[0] = 0x10;
        szDbBusTxData[1] = 0x16;
        szDbBusTxData[2] = 0x02;
		
        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3);    
		
        szDbBusTxData[0] = 0x20;
		
        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 1);    
		
        nRemainSize = MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024; //48KB
        index = 0;
		    
        while (nRemainSize > 0)
        {
            if (nRemainSize > nSizePerWrite)
            {
                nBlockSize = nSizePerWrite;
            }
            else
            {
                nBlockSize = nRemainSize;
            }
		
            szDbBusTxData[0] = 0x10;
            szDbBusTxData[1] = 0x16;
            szDbBusTxData[2] = 0x02;
		
            nSize = 3;
		
            for (i = 0; i < nBlockSize; i ++)
            {
                szDbBusTxData[3+i] = _gOneDimenFwData[index*nSizePerWrite+i];
                nSize ++; 
            }
            index ++;
		
            IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], nSize);
		        
            nRemainSize = nRemainSize - nBlockSize;
        }
		
        // Program end
        szDbBusTxData[0] = 0x21;
		
        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 1);    
		
        nRegData = RegGet16BitValue(0x160C); 
        RegSet16BitValue(0x160C, nRegData & (~0x01));      
		
        DBG("Wait main block write done flag\n");
		
        // Polling 0x1610 is 0x0002
        do
        {
            nRegData = RegGet16BitValue(0x1610);
            nRegData = nRegData & BIT1;
            mdelay(10);
		
        } while (nRegData != BIT1); // Wait write done flag
		
        DBG("Program main block end\n");
    }
    
    if (eEmemType == EMEM_ALL || eEmemType == EMEM_INFO) // 512 Byte
    {
        DBG("Program info block start\n");

        // Program info block
        RegSet16BitValue(0x161A, 0xABBA);
        RegSet16BitValue(0x1618, (RegGet16BitValue(0x1618) | 0x80));

        RegSet16BitValue(0x1600, 0xC000); // Set start address of info block
        RegSet16BitValue(0x160C, (RegGet16BitValue(0x160C) | 0x01)); // Enable burst mode

        // Program start
        szDbBusTxData[0] = 0x10;
        szDbBusTxData[1] = 0x16;
        szDbBusTxData[2] = 0x02;

        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3);    

        szDbBusTxData[0] = 0x20;

        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 1);    

        nRemainSize = MSG22XX_FIRMWARE_INFO_BLOCK_SIZE; //512Byte
        index = 0;
    
        while (nRemainSize > 0)
        {
            if (nRemainSize > nSizePerWrite)
            {
                nBlockSize = nSizePerWrite;
            }
            else
            {
                nBlockSize = nRemainSize;
            }

            szDbBusTxData[0] = 0x10;
            szDbBusTxData[1] = 0x16;
            szDbBusTxData[2] = 0x02;

            nSize = 3;

            for (i = 0; i < nBlockSize; i ++)
            {
                szDbBusTxData[3+i] = _gOneDimenFwData[(MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE*1024)+(index*nSizePerWrite)+i];
                nSize ++; 
            }
            index ++;

            IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], nSize);
        
            nRemainSize = nRemainSize - nBlockSize;
        }

        // Program end
        szDbBusTxData[0] = 0x21;

        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 1);    

        nRegData = RegGet16BitValue(0x160C); 
        RegSet16BitValue(0x160C, nRegData & (~0x01));      

        DBG("Wait info block write done flag\n");

        // Polling 0x1610 is 0x0002
        do
        {
            nRegData = RegGet16BitValue(0x1610);
            nRegData = nRegData & BIT1;
            mdelay(10);

        } while (nRegData != BIT1); // Wait write done flag

        DBG("Program info block end\n");
    }

    if (eEmemType == EMEM_ALL || eEmemType == EMEM_MAIN)
    {
        // Get CRC 32 from updated firmware bin file
        nCrcMain  = _gOneDimenFwData[0xBFFF] << 24;
        nCrcMain |= _gOneDimenFwData[0xBFFE] << 16;
        nCrcMain |= _gOneDimenFwData[0xBFFD] << 8;
        nCrcMain |= _gOneDimenFwData[0xBFFC];

        // CRC Main from TP
        DBG("Get Main CRC from TP\n");

        nCrcMainTp = _DrvFwCtrlMsg22xxGetFirmwareCrcByHardware(EMEM_MAIN);
    
        DBG("nCrcMain=0x%x, nCrcMainTp=0x%x\n", nCrcMain, nCrcMainTp);
    }

    if (eEmemType == EMEM_ALL || eEmemType == EMEM_INFO)
    {
        nCrcInfo  = _gOneDimenFwData[0xC1FF] << 24;
        nCrcInfo |= _gOneDimenFwData[0xC1FE] << 16;
        nCrcInfo |= _gOneDimenFwData[0xC1FD] << 8;
        nCrcInfo |= _gOneDimenFwData[0xC1FC];

        // CRC Info from TP
        DBG("Get Info CRC from TP\n");

        nCrcInfoTp = _DrvFwCtrlMsg22xxGetFirmwareCrcByHardware(EMEM_INFO);

        DBG("nCrcInfo=0x%x, nCrcInfoTp=0x%x\n", nCrcInfo, nCrcInfoTp);
    }

    g_FwDataCount = 0; // Reset g_FwDataCount to 0 after update firmware

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    DrvPlatformLyrTouchDeviceResetHw();

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaFree();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    if (eEmemType == EMEM_ALL)
    {
        if ((nCrcMainTp != nCrcMain) || (nCrcInfoTp != nCrcInfo))
        {
            DBG("Update FAILED\n");
          
            return -1;
        } 
    }
    else if (eEmemType == EMEM_MAIN)
    {
        if (nCrcMainTp != nCrcMain)
        {
            DBG("Update FAILED\n");
          
            return -1;
        } 
    }
    else if (eEmemType == EMEM_INFO)
    {
        if (nCrcInfoTp != nCrcInfo)
        {
            DBG("Update FAILED\n");
          
            return -1;
        } 
    }
    
    DBG("Update SUCCESS\n");

    return 0;
}

static s32 _DrvFwCtrlUpdateFirmwareCash(u8 szFwData[][1024])
{
    DBG("*** %s() ***\n", __func__);

    DBG("chip type = 0x%x\n", g_ChipType);
    
    if (g_ChipType == CHIP_TYPE_MSG21XXA) // (0x02)
    {
//        u16 nChipType;
        u8 nChipVersion = 0;

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
    
        /////////////////////////
        // Difference between C2 and C3
        /////////////////////////
        // c2:MSG2133(1) c32:MSG2133A(2) c33:MSG2138A(2)
        // check ic type
//        nChipType = RegGet16BitValue(0x1ECC) & 0xFF;
            
        // check ic version
        nChipVersion = RegGet16BitValue(0x3CEA) & 0xFF;

        DBG("chip version = 0x%x\n", nChipVersion);
        
        if (nChipVersion == 3)
        {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
            return _DrvFwCtrlMsg21xxaUpdateFirmwareBySwId(szFwData, EMEM_MAIN);
#else
            return _DrvFwCtrlUpdateFirmwareC33(szFwData, EMEM_MAIN);
#endif        
        }
        else
        {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
            return _DrvFwCtrlMsg21xxaUpdateFirmwareBySwId(szFwData, EMEM_MAIN);
#else
            return _DrvFwCtrlUpdateFirmwareC32(szFwData, EMEM_ALL);
#endif        
        }
    }
    else if (g_ChipType == CHIP_TYPE_MSG22XX) // (0x7A)
    {
//        _DrvFwCtrlMsg22xxGetTpVendorCode(_gTpVendorCode);
        
//        if (_gTpVendorCode[0] == 'C' && _gTpVendorCode[1] == 'N' && _gTpVendorCode[2] == 'T') // for specific TP vendor which store some important information in info block, only update firmware for main block, do not update firmware for info block.
//        {
//            return _DrvFwCtrlMsg22xxUpdateFirmware(szFwData, EMEM_MAIN);
//        }
//        else
//        {
            return _DrvFwCtrlMsg22xxUpdateFirmware(szFwData, EMEM_ALL);
//        }
    }
    else // CHIP_TYPE_MSG21XX (0x01)
    {
        DBG("Can not update firmware. Catch-2 is no need to be maintained now.\n");
        g_FwDataCount = 0; // Reset g_FwDataCount to 0 after update firmware

        return -1;
    }
}

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
    
    /////////////////////////
    // Difference between C2 and C3
    /////////////////////////
    // c2:MSG2133(1) c32:MSG2133A(2) c33:MSG2138A(2)
    // check ic type
    nChipType = RegGet16BitValue(0x1ECC) & 0xFF;

    if (nChipType != CHIP_TYPE_MSG21XX &&   // (0x01) 
        nChipType != CHIP_TYPE_MSG21XXA &&  // (0x02) 
        nChipType != CHIP_TYPE_MSG26XXM &&  // (0x03) 
        nChipType != CHIP_TYPE_MSG22XX)     // (0x7A) 
    {
        nChipType = 0;
    }

    DBG("*** Chip Type = 0x%x ***\n", nChipType);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();
        
    DrvPlatformLyrTouchDeviceResetHw();
    
    return nChipType;
}

void DrvFwCtrlGetCustomerFirmwareVersion(u16 *pMajor, u16 *pMinor, u8 **ppVersion)
{
    DBG("*** %s() ***\n", __func__);
    
    if (g_ChipType == CHIP_TYPE_MSG21XXA || g_ChipType == CHIP_TYPE_MSG21XX)
    {
        u8 szDbBusTxData[3] = {0};
        u8 szDbBusRxData[4] = {0};

        szDbBusTxData[0] = 0x53;
        szDbBusTxData[1] = 0x00;

        if (g_ChipType == CHIP_TYPE_MSG21XXA)
        {    
            szDbBusTxData[2] = 0x2A;
        }
        else if (g_ChipType == CHIP_TYPE_MSG21XX)
        {
            szDbBusTxData[2] = 0x74;
        }
        else
        {
            szDbBusTxData[2] = 0x2A;
        }

        mutex_lock(&g_Mutex);

        DrvPlatformLyrTouchDeviceResetHw();
        mdelay(100);

        IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 3);
        IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 4);

        mutex_unlock(&g_Mutex);

        *pMajor = (szDbBusRxData[1]<<8) + szDbBusRxData[0];
        *pMinor = (szDbBusRxData[3]<<8) + szDbBusRxData[2];
    }
    else if (g_ChipType == CHIP_TYPE_MSG22XX)
    {
        u16 nRegData1, nRegData2;

        mutex_lock(&g_Mutex);

        DrvPlatformLyrTouchDeviceResetHw();
    
        DbBusEnterSerialDebugMode();
        DbBusStopMCU();
        DbBusIICUseBus();
        DbBusIICReshape();
        mdelay(100);
        
        // Stop mcu
        RegSetLByteValue(0x0FE6, 0x01); 

        // Stop watchdog
        RegSet16BitValue(0x3C60, 0xAA55);

        // RIU password
        RegSet16BitValue(0x161A, 0xABBA); 

        // Clear pce
        RegSet16BitValue(0x1618, (RegGet16BitValue(0x1618) | 0x80));

        RegSet16BitValue(0x1600, 0xBFF4); // Set start address for customer firmware version on main block
    
        // Enable burst mode
       //RegSet16BitValue(0x160C, (RegGet16BitValue(0x160C) | 0x01));

        // Set pce
        RegSet16BitValue(0x1618, (RegGet16BitValue(0x1618) | 0x40)); 
    
        RegSetLByteValue(0x160E, 0x01); 

        nRegData1 = RegGet16BitValue(0x1604);
        nRegData2 = RegGet16BitValue(0x1606);

        *pMajor = (((nRegData1 >> 8) & 0xFF) << 8) + (nRegData1 & 0xFF);
        *pMinor = (((nRegData2 >> 8) & 0xFF) << 8) + (nRegData2 & 0xFF);

        // Clear burst mode
//        RegSet16BitValue(0x160C, RegGet16BitValue(0x160C) & (~0x01));      

        RegSet16BitValue(0x1600, 0x0000); 

        // Clear RIU password
        RegSet16BitValue(0x161A, 0x0000); 

        DbBusIICNotUseBus();
        DbBusNotStopMCU();
        DbBusExitSerialDebugMode();

        DrvPlatformLyrTouchDeviceResetHw();
        mdelay(100);

        mutex_unlock(&g_Mutex);
    }

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
    u16 nRegData1, nRegData2;
    u8 szDbBusRxData[12] = {0};

    DBG("*** %s() ***\n", __func__);

    mutex_lock(&g_Mutex);

    DrvPlatformLyrTouchDeviceResetHw();
    
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);

    if (g_ChipType == CHIP_TYPE_MSG22XX) // Only MSG22XX support platform firmware version
    {
        // Stop mcu
        RegSetLByteValue(0x0FE6, 0x01); 

        // Stop watchdog
        RegSet16BitValue(0x3C60, 0xAA55);

        // RIU password
        RegSet16BitValue(0x161A, 0xABBA); 

        // Clear pce
        RegSet16BitValue(0x1618, (RegGet16BitValue(0x1618) | 0x80));

        RegSet16BitValue(0x1600, 0xC1F2); // Set start address for platform firmware version on info block(Actually, start reading from 0xC1F0)
    
        // Enable burst mode
        RegSet16BitValue(0x160C, (RegGet16BitValue(0x160C) | 0x01));

        // Set pce
        RegSet16BitValue(0x1618, (RegGet16BitValue(0x1618) | 0x40)); 
    
        for (i = 0; i < 3; i ++)
        {
            RegSetLByteValue(0x160E, 0x01); 

            nRegData1 = RegGet16BitValue(0x1604);
            nRegData2 = RegGet16BitValue(0x1606);

            szDbBusRxData[i*4+0] = (nRegData1 & 0xFF);
            szDbBusRxData[i*4+1] = ((nRegData1 >> 8 ) & 0xFF);
            
//            DBG("szDbBusRxData[%d] = 0x%x , %c \n", i*4+0, szDbBusRxData[i*4+0], szDbBusRxData[i*4+0]); // add for debug
//            DBG("szDbBusRxData[%d] = 0x%x , %c \n", i*4+1, szDbBusRxData[i*4+1], szDbBusRxData[i*4+1]); // add for debug
            
            szDbBusRxData[i*4+2] = (nRegData2 & 0xFF);
            szDbBusRxData[i*4+3] = ((nRegData2 >> 8 ) & 0xFF);

//            DBG("szDbBusRxData[%d] = 0x%x , %c \n", i*4+2, szDbBusRxData[i*4+2], szDbBusRxData[i*4+2]); // add for debug
//            DBG("szDbBusRxData[%d] = 0x%x , %c \n", i*4+3, szDbBusRxData[i*4+3], szDbBusRxData[i*4+3]); // add for debug
        }

        // Clear burst mode
        RegSet16BitValue(0x160C, RegGet16BitValue(0x160C) & (~0x01));      

        RegSet16BitValue(0x1600, 0x0000); 

        // Clear RIU password
        RegSet16BitValue(0x161A, 0x0000); 

        if (*ppVersion == NULL)
        {
            *ppVersion = kzalloc(sizeof(u8)*10, GFP_KERNEL);
        }
    
        sprintf(*ppVersion, "%c%c%c%c%c%c%c%c%c%c", szDbBusRxData[2], szDbBusRxData[3], szDbBusRxData[4],
            szDbBusRxData[5], szDbBusRxData[6], szDbBusRxData[7], szDbBusRxData[8], szDbBusRxData[9], szDbBusRxData[10], szDbBusRxData[11]);
    }
    else
    {
        if (*ppVersion == NULL)
        {
            *ppVersion = kzalloc(sizeof(u8)*10, GFP_KERNEL);
        }
    
        sprintf(*ppVersion, "%s", "N/A");
    }

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    DrvPlatformLyrTouchDeviceResetHw();
    mdelay(100);

    mutex_unlock(&g_Mutex);
    
    DBG("*** platform firmware version = %s ***\n", *ppVersion);
}

s32 DrvFwCtrlUpdateFirmware(u8 szFwData[][1024], EmemType_e eEmemType)
{
    DBG("*** %s() ***\n", __func__);

    return _DrvFwCtrlUpdateFirmwareCash(szFwData);
}	

void DrvFwCtrlHandleFingerTouch(void)
{
    TouchInfo_t tInfo;
    u32 i;
    u8 nTouchKeyCode = 0;
    static u32 nLastKeyCode = 0;
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

        if (g_FirmwareInfo.nLogModePacketHeader != 0x62)
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
    }
    else if (g_FirmwareMode == FIRMWARE_MODE_RAW_DATA_MODE)
    {
        DBG("FIRMWARE_MODE_RAW_DATA_MODE\n");

        if (g_FirmwareInfo.nLogModePacketHeader != 0x62)
        {
            DBG("WRONG RAW DATA MODE HEADER : 0x%x\n", g_FirmwareInfo.nLogModePacketHeader);
            return;
        }

        if (g_LogModePacket == NULL)
        {
            g_LogModePacket = kzalloc(sizeof(u8)*g_FirmwareInfo.nLogModePacketLength, GFP_KERNEL);
        }
        
        nReportPacketLength = g_FirmwareInfo.nLogModePacketLength;
        pPacket = g_LogModePacket;
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
        DBG("Set gesture wakeup packet length, g_ChipType=%d\n", g_ChipType);
        
        if (g_ChipType == CHIP_TYPE_MSG22XX)
        {
            if (_gGestureWakeupPacket == NULL)
            {
                _gGestureWakeupPacket = kzalloc(sizeof(u8)*GESTURE_WAKEUP_PACKET_LENGTH, GFP_KERNEL);
            }

            nReportPacketLength = GESTURE_WAKEUP_PACKET_LENGTH;
            pPacket = _gGestureWakeupPacket;
        } 
        else if (g_ChipType == CHIP_TYPE_MSG21XXA)
        {
            if (_gGestureWakeupPacket == NULL)
            {
                _gGestureWakeupPacket = kzalloc(sizeof(u8)*DEMO_MODE_PACKET_LENGTH, GFP_KERNEL);
            }

        		nReportPacketLength = DEMO_MODE_PACKET_LENGTH;
            pPacket = _gGestureWakeupPacket;
        }
        else
        {
            DBG("This chip type does not support gesture wakeup.\n");
            return;
        }
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
        //report...
        if ((tInfo.nFingerNum) == 0)   //touch end
        {
            if (nLastKeyCode != 0)
            {
                DBG("key touch released\n");

                input_report_key(g_InputDevice, BTN_TOUCH, 0);
                input_report_key(g_InputDevice, nLastKeyCode, 0);
                    
                nLastKeyCode = 0; //clear key status..
            }
            else
            {
                DrvPlatformLyrFingerTouchReleased(0, 0);
            }
        }
        else //touch on screen
        {
            if (tInfo.nTouchKeyCode != 0)
            {
#ifdef CONFIG_TP_HAVE_KEY
                if (tInfo.nTouchKeyCode == 4) // TOUCH_KEY_HOME
                {
                    nTouchKeyCode = g_TpVirtualKey[1];           
                }
                else if (tInfo.nTouchKeyCode == 1) // TOUCH_KEY_MENU
                {
                    nTouchKeyCode = g_TpVirtualKey[0];
                }           
                else if (tInfo.nTouchKeyCode == 2) // TOUCH_KEY_BACK
                {
                    nTouchKeyCode = g_TpVirtualKey[2];
                }           
                else if (tInfo.nTouchKeyCode == 8) // TOUCH_KEY_SEARCH 
                {	
                    nTouchKeyCode = g_TpVirtualKey[3];           
                }

                if (nLastKeyCode != nTouchKeyCode)
                {
                    DBG("key touch pressed\n");
                    DBG("nTouchKeyCode = %d, nLastKeyCode = %d\n", nTouchKeyCode, nLastKeyCode);
                    
                    nLastKeyCode = nTouchKeyCode;

                    input_report_key(g_InputDevice, BTN_TOUCH, 1);
                    input_report_key(g_InputDevice, nTouchKeyCode, 1);
                }
#endif //CONFIG_TP_HAVE_KEY
            }
            else
            {
                DBG("tInfo->nFingerNum = %d...............\n", tInfo.nFingerNum);
                
                for (i = 0; i < tInfo.nFingerNum; i ++) 
                {
                    DrvPlatformLyrFingerTouchPressed(tInfo.tPoint[i].nX, tInfo.tPoint[i].nY, 0, 0);
                }
            }
        }
        
        input_sync(g_InputDevice);
    }
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

#ifdef CONFIG_ENABLE_FIRMWARE_DATA_LOG

u16 DrvFwCtrlChangeFirmwareMode(u16 nMode)
{
    u8 szDbBusTxData[2] = {0};

    DBG("*** %s() *** nMode = 0x%x\n", __func__, nMode);

    szDbBusTxData[0] = 0x02;
    szDbBusTxData[1] = (u8)nMode;

    mdelay(20);
    
    mutex_lock(&g_Mutex);

    IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 2);

    mutex_unlock(&g_Mutex);

    return nMode;
}

void DrvFwCtrlGetFirmwareInfo(FirmwareInfo_t *pInfo)
{
    u8 szDbBusTxData[1] = {0};
    u8 szDbBusRxData[8] = {0};
    
    DBG("*** %s() ***\n", __func__);

    szDbBusTxData[0] = 0x01;

    mutex_lock(&g_Mutex);
    
    mdelay(300);
    IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 1);
    mdelay(20);
    IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 8);

    mutex_unlock(&g_Mutex);
    
    if ((szDbBusRxData[1] & 0x80) == 0x80)
    {
        pInfo->nIsCanChangeFirmwareMode = 0;	
    }
    else
    {
        pInfo->nIsCanChangeFirmwareMode = 1;	
    }
    
    pInfo->nFirmwareMode = szDbBusRxData[1] & 0x7F;
    pInfo->nLogModePacketHeader = szDbBusRxData[2];
    pInfo->nLogModePacketLength = (szDbBusRxData[3]<<8) + szDbBusRxData[4];

    DBG("pInfo->nFirmwareMode=0x%x, pInfo->nLogModePacketHeader=0x%x, pInfo->nLogModePacketLength=%d, pInfo->nIsCanChangeFirmwareMode=%d\n", pInfo->nFirmwareMode, pInfo->nLogModePacketHeader, pInfo->nLogModePacketLength, pInfo->nIsCanChangeFirmwareMode);
}

void DrvFwCtrlRestoreFirmwareModeToLogDataMode(void)
{
    FirmwareInfo_t tInfo;
    
    DBG("*** %s() ***\n", __func__);

    memset(&tInfo, 0x0, sizeof(FirmwareInfo_t));

    DrvFwCtrlGetFirmwareInfo(&tInfo);

    DBG("g_FirmwareMode = 0x%x, tInfo.nFirmwareMode = 0x%x\n", g_FirmwareMode, tInfo.nFirmwareMode);

    // Since reset_hw() will reset the firmware mode to demo mode, we must reset the firmware mode again after reset_hw().
    if (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE && FIRMWARE_MODE_DEBUG_MODE != tInfo.nFirmwareMode)
    {
        g_FirmwareMode = DrvFwCtrlChangeFirmwareMode(FIRMWARE_MODE_DEBUG_MODE);
    }
    else if (g_FirmwareMode == FIRMWARE_MODE_RAW_DATA_MODE && FIRMWARE_MODE_RAW_DATA_MODE != tInfo.nFirmwareMode)
    {
        g_FirmwareMode = DrvFwCtrlChangeFirmwareMode(FIRMWARE_MODE_RAW_DATA_MODE);
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
    if (g_ChipType == CHIP_TYPE_MSG21XXA)   
    {
        _DrvFwCtrlMsg21xxaCheckFirmwareUpdateBySwId();
    }
    else if (g_ChipType == CHIP_TYPE_MSG22XX)    
    {
        _DrvFwCtrlMsg22xxCheckFirmwareUpdateBySwId();
    }
    else
    {
        DBG("This chip type (%d) does not support update firmware by sw id\n", g_ChipType);
    }
}	

#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

//------------------------------------------------------------------------------//

#endif //CONFIG_ENABLE_CHIP_MSG21XXA || CONFIG_ENABLE_CHIP_MSG22XX


#ifdef __TP_PROXIMITY_SUPPORT__
static int msg2133_enable_ps(int enable)
{
    #if 1 //21xxA/22xx   0=26xx
	u8 ps_store_data[4];
    #else
    u8 ps_store_data[2];//if chip type is 26xxM
    #endif
	u8 i;
	s16 rc = 0;
	mutex_lock(&msg2133_sensor_mutex);

	if(enable == 1)
	{
		if(g_bPsSensorOpen == 0)
		{
            #if 1//21xxA/22xx  0=26xx
      		ps_store_data[0] = 0x52;
			ps_store_data[1] = 0x00;
      		ps_store_data[2] = 0x4A;
			ps_store_data[3] = 0xA0;
            #else
  			ps_store_data[0] = 0x30;       //if chip type is 26xxM
			ps_store_data[1] = 0xa0;		//if chip type is 26xxM 
            #endif
			while(i < 5)
            {
                #if 1 //21xxA/22xx 0=26xx
				rc = IicWriteData(0x26, &ps_store_data[0], 4);
                #else
                rc = IicWriteData(0x26, &ps_store_data[0], 2);//if chip type is 26xxM 
                #endif
				if(rc >0)
				{
					g_bPsSensorOpen = 1;
        				DBG("MSG2633 open ps success!!!\n");
					break;
            }
				else
				{
					DBG("MSG2633 open ps fail!!!\n");
					i++;
				}
			}

			if(i == 5 || rc < 0)
			{
				g_bPsSensorOpen = 0;
    				DBG("MSG2633 open ps fail!!!\n");
			}						// suspend()的操作会使TP下电
		}
	}
	else
	{	
		if(g_bPsSensorOpen == 1)
		{
            #if 1 //21xxA /22xx   0=26xx
      		ps_store_data[0] = 0x52;
			ps_store_data[1] = 0x00;
      		ps_store_data[2] = 0x4A;
			ps_store_data[3] = 0xA1;
            #else
			ps_store_data[0] = 0x30;  //if chip type is 26xxM
			ps_store_data[1] = 0xa1;  //if chip type is 26xxM
			#endif
			while(i < 5)
            {
                #if 1 //21xxA/22xx 0=26xx
				rc = IicWriteData(0x26, &ps_store_data[0], 4);
                #else
                rc = IicWriteData(0x26, &ps_store_data[0], 2);//if chip type is 26xxM
                #endif
				if(rc >0)
				{
			g_bPsSensorOpen = 0;			
        				DBG("MSG2633 close ps success!!!\n");
					break;
		}
				else
				{
					i++;
				}
			}
			if(i == 5 || rc < 0)
			{
				g_bPsSensorOpen = 1;
    				DBG("MSG2633 close ps fail!!!\n");
			}		
		}
	}
	mutex_unlock(&msg2133_sensor_mutex);
	return 0;
}

int msg2133_ps_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	hwm_sensor_data* sensor_data;

	//APS_FUN(f);
	switch (command)
	{
		case SENSOR_DELAY:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Set delay parameter error!\n");
				err = -EINVAL;
			}
			// Do nothing
			break;

		case SENSOR_ENABLE:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{				
				value = *(int *)buff_in;
				if(value)
				{
					wake_lock(&ps_lock);		//wujinyou
					if(err = msg2133_enable_ps(1))
					{
						APS_ERR("enable ps fail: %d\n", err); 
						return -1;
					}
					g_bPsSensorOpen = 1;
				}
				else
				{
					wake_unlock(&ps_lock);		//wujinyou
					if(err = msg2133_enable_ps(0))
					{
						APS_ERR("disable ps fail: %d\n", err); 
						return -1;
					}
					g_bPsSensorOpen = 0;
				}
			}
			break;

		case SENSOR_GET_DATA:
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				APS_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				sensor_data = (hwm_sensor_data *)buff_out;	
				if(g_call_state)
				sensor_data->values[0] = g_nPsSensorDate;
				else
				sensor_data->values[0] = 1;	
				sensor_data->value_divide = 1;
				sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;			
			}
			break;
		default:
			APS_ERR("proxmy sensor operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}
	
	return err;
}

void tpd_initialize_ps_sensor_function()
{
	struct hwmsen_object obj_ps = {0};
	int err = 0;
	
	g_nPsSensorDate = 1;

	obj_ps.self = NULL;	// no use
	obj_ps.polling = 1;
	obj_ps.sensor_operate = msg2133_ps_operate;

	wake_lock_init(&ps_lock,WAKE_LOCK_SUSPEND,"ps wakelock"); //shaohui add
		
	if(err = hwmsen_attach(ID_PROXIMITY, &obj_ps))
	{
		TPD_DEBUG("attach fail = %d\n", err);
		return;
	}
}
#endif

