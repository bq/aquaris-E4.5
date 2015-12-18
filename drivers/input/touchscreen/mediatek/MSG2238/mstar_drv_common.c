////////////////////////////////////////////////////////////////////////////////
// (c) MStar Semiconductor, Inc. 2006-2014
////////////////////////////////////////////////////////////////////////////////

/**
 *
 * @file    mstar_drv_common.c
 *
 * @brief   This file defines the interface of touch screen
 *
 * @version v2.3.0.0
 *
 */

/*=============================================================*/
// INCLUDE FILE
/*=============================================================*/

#include "mstar_drv_common.h"

/*=============================================================*/
// MACRO DEFINITION
/*=============================================================*/

/*=============================================================*/
// CONSTANT VALUE DEFINITION
/*=============================================================*/


/*=============================================================*/
// VARIABLE DEFINITION
/*=============================================================*/

static u32 _gCrc32Table[256]; 

/*=============================================================*/
// DATA TYPE DEFINITION
/*=============================================================*/

/*=============================================================*/
// GLOBAL FUNCTION DEFINITION
/*=============================================================*/

/// CRC
u32 DrvCommonCrcDoReflect(u32 nRef, s8 nCh)
{
    u32 nValue = 0;
    u32 i = 0;

    for (i = 1; i < (nCh + 1); i ++)
    {
        if (nRef & 1)
        {
            nValue |= 1 << (nCh - i);
        }
        nRef >>= 1;
    }

    return nValue;
}

u32 DrvCommonCrcGetValue(u32 nText, u32 nPrevCRC)
{
    u32 nCRC = nPrevCRC;

    nCRC = (nCRC >> 8) ^ _gCrc32Table[(nCRC & 0xFF) ^ nText];

    return nCRC;
}

void DrvCommonCrcInitTable(void)
{
    u32 nMagicNumber = 0x04c11db7;
    u32 i, j;

    for (i = 0; i <= 0xFF; i ++)
    {
        _gCrc32Table[i] = DrvCommonCrcDoReflect(i, 8) << 24;
        for (j = 0; j < 8; j ++)
        {
            _gCrc32Table[i] = (_gCrc32Table[i] << 1) ^ (_gCrc32Table[i] & (0x80000000L) ? nMagicNumber : 0);
        }
        _gCrc32Table[i] = DrvCommonCrcDoReflect(_gCrc32Table[i], 32);
    }
}

u8 DrvCommonCalculateCheckSum(u8 *pMsg, u32 nLength)
{
    s32 nCheckSum = 0;
    u32 i;

    for (i = 0; i < nLength; i ++)
    {
        nCheckSum += pMsg[i];
    }

    return (u8)((-nCheckSum) & 0xFF);
}

u32 DrvCommonConvertCharToHexDigit(char *pCh, u32 nLength)
{
    u32 nRetVal = 0;
    u32 i;
    
    DBG("nLength = %d\n", nLength);

    for (i = 0; i < nLength; i ++)
    {
        char ch = *pCh++;
        u32 n = 0;
        
        if ((i == 0 && ch == '0') || (i == 1 && ch == 'x'))
        {
            continue;		
        }
        
        if ('0' <= ch && ch <= '9')
        {
            n = ch-'0';
        }
        else if ('a' <= ch && ch <= 'f')
        {
            n = 10 + ch-'a';
        }
        else if ('A' <= ch && ch <= 'F')
        {
            n = 10 + ch-'A';
        }
        
        if (i < 6)
        {
            nRetVal = n + nRetVal*16;
        }
    }
    
    return nRetVal;
}

//------------------------------------------------------------------------------//