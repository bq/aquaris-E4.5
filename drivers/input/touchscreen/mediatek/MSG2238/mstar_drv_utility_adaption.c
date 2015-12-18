////////////////////////////////////////////////////////////////////////////////
// (c) MStar Semiconductor, Inc. 2006-2014
////////////////////////////////////////////////////////////////////////////////

/**
 *
 * @file    mstar_drv_utility_adaption.c
 *
 * @brief   This file defines the interface of touch screen
 *
 * @version v2.3.0.0
 *
 */

////////////////////////////////////////////////////////////
/// Included Files
////////////////////////////////////////////////////////////
#include "mstar_drv_utility_adaption.h"

////////////////////////////////////////////////////////////
/// Data Types
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
/// Constant
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
/// Variables
////////////////////////////////////////////////////////////
extern struct i2c_client *g_I2cClient;

////////////////////////////////////////////////////////////
/// Macro
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
/// Function Prototypes
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
/// Function Implementation
////////////////////////////////////////////////////////////


#ifdef CONFIG_ENABLE_DMA_IIC
#include <linux/dma-mapping.h>
#include <linux/mm_types.h>
#include <linux/mm.h>
#include <asm/uaccess.h>
#include <asm/page.h>
#include <linux/vmalloc.h>

static unsigned char *I2CDMABuf_va = NULL;
static volatile unsigned int I2CDMABuf_pa = NULL;

void DmaAlloc(void)
{
    if (NULL == I2CDMABuf_va)
    {
        I2CDMABuf_va = (u8 *)dma_alloc_coherent(NULL, 4096, &I2CDMABuf_pa, GFP_KERNEL);
    }
    
    if (NULL == I2CDMABuf_va)
    {
        DBG("DrvCommonDmaAlloc FAILED!");
    }
    else
    {
        DBG("DrvCommonDmaAlloc SUCCESS!");
    }
}

void DmaFree(void)
{
    if (NULL != I2CDMABuf_va)
    {
        dma_free_coherent(NULL, 4096, I2CDMABuf_va, I2CDMABuf_pa);
	      I2CDMABuf_va = NULL;
	      I2CDMABuf_pa = 0;
    }
}
#endif //CONFIG_ENABLE_DMA_IIC

//------------------------------------------------------------------------------//
#define CHANGE_I2C_TIMING_50 //showlo

u16 RegGet16BitValue(u16 nAddr)
{
    u8 tx_data[3] = {0x10, (nAddr >> 8) & 0xFF, nAddr & 0xFF};
    u8 rx_data[2] = {0};
#if  defined CHANGE_I2C_TIMING_50 && defined CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM
    u16 i2c_timing = g_I2cClient ->timing;
    g_I2cClient->timing = 50;
    IicWriteData(SLAVE_I2C_ID_DBBUS, &tx_data[0], 3);
    IicReadData(SLAVE_I2C_ID_DBBUS, &rx_data[0], 2);
    g_I2cClient->timing = i2c_timing;
#elif defined CHANGE_I2C_TIMING_50 &&  defined CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM
    sprd_i2C_ctl_chg_clk(g_I2cClient->adapter->nr,50000);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &tx_data[0], 3);
    IicReadData(SLAVE_I2C_ID_DBBUS, &rx_data[0], 2);
    sprd_i2C_ctl_chg_clk(g_I2cClient->adapter->nr,100000);
#else
    IicWriteData(SLAVE_I2C_ID_DBBUS, &tx_data[0], 3);
    IicReadData(SLAVE_I2C_ID_DBBUS, &rx_data[0], 2);
#endif

    return (rx_data[1] << 8 | rx_data[0]);
}

u8 RegGetLByteValue(u16 nAddr)
{
    u8 tx_data[3] = {0x10, (nAddr >> 8) & 0xFF, nAddr & 0xFF};
    u8 rx_data = {0};
#if  defined CHANGE_I2C_TIMING_50 && defined CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM
    u16 i2c_timing = g_I2cClient ->timing;
    g_I2cClient->timing = 50;
    IicWriteData(SLAVE_I2C_ID_DBBUS, &tx_data[0], 3);
    IicReadData(SLAVE_I2C_ID_DBBUS, &rx_data, 1);
    g_I2cClient->timing = i2c_timing;
#elif defined CHANGE_I2C_TIMING_50 &&  defined CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM
    sprd_i2C_ctl_chg_clk(g_I2cClient->adapter->nr,50000);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &tx_data[0], 3);
    IicReadData(SLAVE_I2C_ID_DBBUS, &rx_data, 1);
    sprd_i2C_ctl_chg_clk(g_I2cClient->adapter->nr,100000);
#else
    IicWriteData(SLAVE_I2C_ID_DBBUS, &tx_data[0], 3);
    IicReadData(SLAVE_I2C_ID_DBBUS, &rx_data, 1);
#endif
    return (rx_data);
}

u8 RegGetHByteValue(u16 nAddr)
{
    u8 tx_data[3] = {0x10, (nAddr >> 8) & 0xFF, (nAddr & 0xFF) + 1};
    u8 rx_data = {0};
#if  defined CHANGE_I2C_TIMING_50 && defined CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM
    u16 i2c_timing = g_I2cClient ->timing;
    g_I2cClient->timing = 50;
    IicWriteData(SLAVE_I2C_ID_DBBUS, &tx_data[0], 3);
    IicReadData(SLAVE_I2C_ID_DBBUS, &rx_data, 1);
    g_I2cClient->timing = i2c_timing;
#elif defined CHANGE_I2C_TIMING_50 &&  defined CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM
    sprd_i2C_ctl_chg_clk(g_I2cClient->adapter->nr,50000);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &tx_data[0], 3);
    IicReadData(SLAVE_I2C_ID_DBBUS, &rx_data, 1);
    sprd_i2C_ctl_chg_clk(g_I2cClient->adapter->nr,100000);
#else
    IicWriteData(SLAVE_I2C_ID_DBBUS, &tx_data[0], 3);
    IicReadData(SLAVE_I2C_ID_DBBUS, &rx_data, 1);
#endif
    return (rx_data);
}

void RegSet16BitValue(u16 nAddr, u16 nData)
{
    u8 tx_data[5] = {0x10, (nAddr >> 8) & 0xFF, nAddr & 0xFF, nData & 0xFF, nData >> 8};
#if  defined CHANGE_I2C_TIMING_50 && defined CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM
    u16 i2c_timing = g_I2cClient ->timing;
    g_I2cClient->timing = 50;
    IicWriteData(SLAVE_I2C_ID_DBBUS, &tx_data[0], 5);
    g_I2cClient->timing = i2c_timing;
#elif defined CHANGE_I2C_TIMING_50 &&  defined CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM
    sprd_i2C_ctl_chg_clk(g_I2cClient->adapter->nr,50000);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &tx_data[0], 5);
    sprd_i2C_ctl_chg_clk(g_I2cClient->adapter->nr,100000);
#else
    IicWriteData(SLAVE_I2C_ID_DBBUS, &tx_data[0], 5);
#endif
}

void RegSetLByteValue(u16 nAddr, u8 nData)
{
    u8 tx_data[4] = {0x10, (nAddr >> 8) & 0xFF, nAddr & 0xFF, nData}; 
#if  defined CHANGE_I2C_TIMING_50 && defined CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM
    u16 i2c_timing = g_I2cClient ->timing;
    g_I2cClient->timing = 50;
    IicWriteData(SLAVE_I2C_ID_DBBUS, &tx_data[0], 4);
    g_I2cClient->timing = i2c_timing;    
#elif defined CHANGE_I2C_TIMING_50 &&  defined CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM
    sprd_i2C_ctl_chg_clk(g_I2cClient->adapter->nr,50000);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &tx_data[0], 4);
    sprd_i2C_ctl_chg_clk(g_I2cClient->adapter->nr,100000);
#else    
    IicWriteData(SLAVE_I2C_ID_DBBUS, &tx_data[0], 4);
#endif
}

void RegSetHByteValue(u16 nAddr, u8 nData)
{
    u8 tx_data[4] = {0x10, (nAddr >> 8) & 0xFF, (nAddr & 0xFF) + 1, nData};
#if  defined CHANGE_I2C_TIMING_50 && defined CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM
    u16 i2c_timing = g_I2cClient ->timing;
    g_I2cClient->timing = 50;
    IicWriteData(SLAVE_I2C_ID_DBBUS, &tx_data[0], 4);
    g_I2cClient->timing = i2c_timing;
#elif defined CHANGE_I2C_TIMING_50 &&  defined CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM
    sprd_i2C_ctl_chg_clk(g_I2cClient->adapter->nr,50000);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &tx_data[0], 4);
    sprd_i2C_ctl_chg_clk(g_I2cClient->adapter->nr,100000);
#else
    IicWriteData(SLAVE_I2C_ID_DBBUS, &tx_data[0], 4);
#endif
}

void RegSet16BitValueOn(u16 nAddr, u16 nData) //Set bit on nData from 0 to 1
{
    u16 rData = RegGet16BitValue(nAddr);
    rData |= nData;
    RegSet16BitValue(nAddr, rData);
}

void RegSet16BitValueOff(u16 nAddr, u16 nData) //Set bit on nData from 1 to 0
{
    u16 rData = RegGet16BitValue(nAddr);
    rData &= (~nData);
    RegSet16BitValue(nAddr, rData);
}

u16 RegGet16BitValueByAddressMode(u16 nAddr, AddressMode_e eAddressMode)
{
    u16 nData = 0;
    
    if (eAddressMode == ADDRESS_MODE_16BIT)
    {
        nAddr = nAddr - (nAddr & 0xFF) + ((nAddr & 0xFF) << 1);
    }
    
    nData = RegGet16BitValue(nAddr);
    
    return nData;
}
	
void RegSet16BitValueByAddressMode(u16 nAddr, u16 nData, AddressMode_e eAddressMode)
{
    if (eAddressMode == ADDRESS_MODE_16BIT)
    {
        nAddr = nAddr - (nAddr & 0xFF) + ((nAddr & 0xFF) << 1);
    }
    
    RegSet16BitValue(nAddr, nData);
}

void RegMask16BitValue(u16 nAddr, u16 nMask, u16 nData, AddressMode_e eAddressMode) 
{
    u16 nTmpData = 0;
    
    if (nData > nMask)
    {
        return;
    }

    nTmpData = RegGet16BitValueByAddressMode(nAddr, eAddressMode);
    nTmpData = (nTmpData & (~nMask));
    nTmpData = (nTmpData | nData);
    RegSet16BitValueByAddressMode(nAddr, nTmpData, eAddressMode);
}

void DbBusEnterSerialDebugMode(void)
{
    u8 data[5];

    // Enter the Serial Debug Mode
    data[0] = 0x53;
    data[1] = 0x45;
    data[2] = 0x52;
    data[3] = 0x44;
    data[4] = 0x42;

    IicWriteData(SLAVE_I2C_ID_DBBUS, data, 5);
}

void DbBusExitSerialDebugMode(void)
{
    u8 data[1];

    // Exit the Serial Debug Mode
    data[0] = 0x45;

    IicWriteData(SLAVE_I2C_ID_DBBUS, data, 1);
    
    // Delay some interval to guard the next transaction
//    udelay(200);        // delay about 0.2ms
}

void DbBusIICUseBus(void)
{
    u8 data[1];

    // IIC Use Bus
    data[0] = 0x35;

    IicWriteData(SLAVE_I2C_ID_DBBUS, data, 1);
}

void DbBusIICNotUseBus(void)
{
    u8 data[1];

    // IIC Not Use Bus
    data[0] = 0x34;

    IicWriteData(SLAVE_I2C_ID_DBBUS, data, 1);
}

void DbBusIICReshape(void)
{
    u8 data[1];

    // IIC Re-shape
    data[0] = 0x71;

    IicWriteData(SLAVE_I2C_ID_DBBUS, data, 1);
}

void DbBusStopMCU(void)
{
    u8 data[1];

    // Stop the MCU
    data[0] = 0x37;

    IicWriteData(SLAVE_I2C_ID_DBBUS, data, 1);
}

void DbBusNotStopMCU(void)
{
    u8 data[1];

    // Not Stop the MCU
    data[0] = 0x36;

    IicWriteData(SLAVE_I2C_ID_DBBUS, data, 1);
}

s32 IicWriteData(u8 nSlaveId, u8* pBuf, u16 nSize)
{
    s32 rc = 0;

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
    struct i2c_msg msgs[] =
    {
        {
            .addr = nSlaveId,
            .flags = 0, // if read flag is undefined, then it means write flag.
            .len = nSize,
            .buf = pBuf,
        },
    };

    /* If everything went ok (i.e. 1 msg transmitted), return #bytes
       transmitted, else error code. */
    if (g_I2cClient != NULL)
    {
        rc = i2c_transfer(g_I2cClient->adapter, msgs, 1);
        if (rc < 0)
        {
            PRINTF_ERR("IicWriteData() error %d\n", rc);
        }
    }
    else
    {
        PRINTF_ERR("i2c client is NULL\n");
    }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    if (g_I2cClient != NULL)
    {
        u8 nAddrBefore = g_I2cClient->addr;
        g_I2cClient->addr = nSlaveId;
//        g_I2cClient->addr = (g_I2cClient->addr & I2C_MASK_FLAG ) | (I2C_ENEXT_FLAG);

#ifdef CONFIG_ENABLE_DMA_IIC
        if (nSize > 8 && NULL != I2CDMABuf_va)
        {
            s32 i = 0;
	          
            for (i = 0; i < nSize; i ++)
            {
                I2CDMABuf_va[i] = pBuf[i];
            }
            g_I2cClient->ext_flag = g_I2cClient->ext_flag | I2C_DMA_FLAG;
            rc = i2c_master_send(g_I2cClient, (unsigned char *)I2CDMABuf_pa, nSize);
        }
        else
        {
            g_I2cClient->ext_flag = g_I2cClient->ext_flag & (~I2C_DMA_FLAG);	
            rc = i2c_master_send(g_I2cClient, pBuf, nSize);
        }
#else
        rc = i2c_master_send(g_I2cClient, pBuf, nSize);
#endif //CONFIG_ENABLE_DMA_IIC
        g_I2cClient->addr = nAddrBefore;

        if (rc < 0)
        {
            PRINTF_ERR("IicWriteData() error %d, nSlaveId=%d, nSize=%d\n", rc, nSlaveId, nSize);
        }
    }
    else
    {
        PRINTF_ERR("i2c client is NULL\n");
    }
#endif
    
    return rc;
}

s32 IicReadData(u8 nSlaveId, u8* pBuf, u16 nSize)
{
    s32 rc = 0;

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
    struct i2c_msg msgs[] =
    {
        {
            .addr = nSlaveId,
            .flags = I2C_M_RD, // read flag
            .len = nSize,
            .buf = pBuf,
        },
    };

    /* If everything went ok (i.e. 1 msg transmitted), return #bytes
       transmitted, else error code. */
    if (g_I2cClient != NULL)
    {
        rc = i2c_transfer(g_I2cClient->adapter, msgs, 1);
        if (rc < 0)
        {
            PRINTF_ERR("IicReadData() error %d\n", rc);
        }
    }
    else
    {
        PRINTF_ERR("i2c client is NULL\n");
    }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    if (g_I2cClient != NULL)
    {
        u8 nAddrBefore = g_I2cClient->addr;
        g_I2cClient->addr = nSlaveId;
//        g_I2cClient->addr = (g_I2cClient->addr & I2C_MASK_FLAG) | (I2C_ENEXT_FLAG);

#ifdef CONFIG_ENABLE_DMA_IIC
        if (nSize > 8 && NULL != I2CDMABuf_va)
        {
            s32 i = 0;
        
            g_I2cClient->ext_flag = g_I2cClient->ext_flag | I2C_DMA_FLAG;
            rc = i2c_master_recv(g_I2cClient, (unsigned char *)I2CDMABuf_pa, nSize);
        
            for (i = 0; i < nSize; i ++)
            {
                pBuf[i] = I2CDMABuf_va[i];
            }
        }
        else
        {
            g_I2cClient->ext_flag = g_I2cClient->ext_flag & (~I2C_DMA_FLAG);	
            rc = i2c_master_recv(g_I2cClient, pBuf, nSize);
        }
#else
        rc = i2c_master_recv(g_I2cClient, pBuf, nSize);
#endif //CONFIG_ENABLE_DMA_IIC
        g_I2cClient->addr = nAddrBefore;

        if (rc < 0)
        {
            PRINTF_ERR("IicReadData() error %d, nSlaveId=%d, nSize=%d\n", rc, nSlaveId, nSize);
        }
    }
    else
    {
        PRINTF_ERR("i2c client is NULL\n");
    }
#endif
    
    return rc;
}

void mstpMemSet(void *pDst, s8 nVal, u32 nSize)
{
    memset(pDst, nVal, nSize);
}

void mstpMemCopy(void *pDst, void *pSource, u32 nSize)
{
    memcpy(pDst, pSource, nSize);
}

void mstpDelay(u32 nTime)
{
    mdelay(nTime);
}

//------------------------------------------------------------------------------//
