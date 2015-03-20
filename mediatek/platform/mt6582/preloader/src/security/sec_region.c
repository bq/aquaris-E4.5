
#include "sec_platform.h"
#include "sec_region.h"
#include "sec.h"

/******************************************************************************
 * MODULE
 ******************************************************************************/
#define MOD                         "SEC_REGION"

/******************************************************************************
 * DEBUG
 ******************************************************************************/
#define SMSG                        dbg_print

/******************************************************************************
 *  EXTERNAL VARIABLES
 ******************************************************************************/

/******************************************************************************
 *  SECURITY REGION CHECK
 ******************************************************************************/
void sec_region_check (U32 addr, U32 len)
{
    U32 ret = SEC_OK;   
    U32 tmp = addr + len;

    /* check if it does access AHB/APB register */
    if ((IO_PHYS != (addr & REGION_MASK)) || (IO_PHYS != (tmp & REGION_MASK))) {
        SMSG("[%s] 0x%x Not AHB/APB Address\n", MOD, addr);   
        ASSERT(0);
    }

    if (len >= REGION_BANK) {
        SMSG("[%s] Overflow\n",MOD);
        ASSERT(0);
    }    

#ifdef MTK_SECURITY_SW_SUPPORT
    /* check platform security region */
    if (SEC_OK != (ret = seclib_region_check(addr,len))) {
        SMSG("[%s] ERR '0x%x' ADDR: 0x%x, LEN: %d\n", MOD, ret, addr, len);
        ASSERT(0);
    }
#endif
}

/******************************************************************************
 *  DA REGION CHECK
 ******************************************************************************/
U32 da_region_check (U32 addr, U32 len)
{
    U32 ret = SEC_OK;

    if(DA_DOWNLOAD_LOC != addr)
    {
        ret = ERR_DA_INVALID_LOCATION;
        goto _exit;
    }

    if(DA_DOWNLOAD_MAX_SZ < len)
    {
        ret = ERR_DA_INVALID_LENGTH;
        goto _exit;        
    }

_exit:

    return ret;
}


