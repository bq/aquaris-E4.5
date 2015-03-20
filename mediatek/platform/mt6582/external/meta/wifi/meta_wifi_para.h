
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//

/*****************************************************************************
 *
 * Filename:
 * ---------
 *   meta_wifi_para.h
 *
 * Project:
 * --------
 *   DUMA
 *
 * Description:
 * ------------
 *   the defination of Wi-Fi wrapper interface for META FT task.
 *
 * Author:
 * -------
 *  Renbang Jiang (MTK80150)
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Revision:$
 * $Modtime:$
 * $Log:$
 *
 * Mar 6 2009 mtk80150
 * [DUMA00110922] [Wi-Fi] Wi-Fi driver for META initial timeout
 * Add Timeout for driver initializing
 *
 * Mar 6 2009 mtk80150
 * [DUMA00110922] [Wi-Fi] Wi-Fi driver for META initial timeout
 * Add timeout for Wi-Fi driver initialize
 *
 * Feb 22 2009 mtk80150
 * [DUMA00109732] [Wi-Fi] Driver version update to 1.13
 * 
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/

#ifndef _META_WIFI_PARA_H_
#define _META_WIFI_PARA_H_

#include <sys/types.h>
#include "FT_Public.h"

#define DRIVER_INIT_TIMEOUT 1000

#define ZONE_ERROR 1
#define ZONE_FUNC 0

#define FUNCTION_CODE_QUERY_OID_VALUE           0x201
#define FUNCTION_CODE_SET_OID_VALUE             0x205
#define FUNCTION_CODE_POSTINIT_VALUE            0x209

#define _META_CTL_CODE(_Function, _Method, _Access)                \
            CTL_CODE(FILE_DEVICE_NETWORK, _Function, _Method, _Access)

#define IOCTL_META_SET_OID_VALUE                                   \
            _META_CTL_CODE(FUNCTION_CODE_SET_OID_VALUE,            \
                           METHOD_BUFFERED,                        \
                           FILE_READ_ACCESS | FILE_WRITE_ACCESS)   

#define IOCTL_META_QUERY_OID_VALUE                                 \
            _META_CTL_CODE(FUNCTION_CODE_QUERY_OID_VALUE,          \
                           METHOD_BUFFERED,                        \
                           FILE_READ_ACCESS | FILE_WRITE_ACCESS)   
                            
   
#define IOCTL_META_WIFI_POSTINIT                                        \
            _META_CTL_CODE(FUNCTION_CODE_POSTINIT_VALUE,           \
                           METHOD_BUFFERED,                        \
                           FILE_READ_ACCESS | FILE_WRITE_ACCESS)  
                                       
#define WIFI_DEV_NAME (L"NDL1:")
#define WIFI_READY_EVENT_NAME (L"OEM/WiFiDriverReady")


typedef enum {
    WIFI_CMD_SET_OID = 0,
    WIFI_CMD_QUERY_OID,
    WIFI_CMD_NVRAM_WRITE_ACCESS,
    WIFI_CMD_NVRAM_READ_ACCESS,
    WIFI_CMD_INIT,
    WIFI_CMD_DEINIT
} WIFI_CMD_TYPE;

typedef struct
{
    FT_H            header;	
    WIFI_CMD_TYPE   type; 	
    int             dummy;	
} FT_WM_WIFI_REQ;

typedef struct
{
    FT_H            header;	
    WIFI_CMD_TYPE   type; 		
    long            drv_status; 	
    unsigned char   status;	
}FT_WM_WIFI_CNF;


typedef struct _SET_OID_STRUC
{
    unsigned int  oid;
    unsigned int  dataLen;
    unsigned char data[1];

} SET_OID_STRUC, *PSET_OID_STRUC;


typedef struct _QUERY_OID_STRUC
{
    unsigned int  oid;
    unsigned int  dataLen;
    unsigned char data[1];

} QUERY_OID_STRUC, *PQUERY_OID_STRUC;

typedef struct _NVRAM_ACCESS_STRUCT {
    unsigned int  dataLen;
    unsigned int  dataOffset;
    unsigned char data[1];
} NVRAM_ACCESS_STRUCT, *PNVRAM_ACCESS_STRUCT;

typedef union 
{
    SET_OID_STRUC   SetOidPara;
    QUERY_OID_STRUC QueryOidPara;
} OID_STRUC, *POID_STRUC;


int META_WIFI_init(void);
void META_WIFI_deinit(void);
void META_WIFI_OP(FT_WM_WIFI_REQ *req, char *peer_buf, unsigned short peer_len);

#endif


