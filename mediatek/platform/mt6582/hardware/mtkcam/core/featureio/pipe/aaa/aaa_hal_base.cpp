
/********************************************************************************************
 *     LEGAL DISCLAIMER
 *
 *     (Header of MediaTek Software/Firmware Release or Documentation)
 *
 *     BY OPENING OR USING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 *     THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE") RECEIVED
 *     FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON AN "AS-IS" BASIS
 *     ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED,
 *     INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR
 *     A PARTICULAR PURPOSE OR NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY
 *     WHATSOEVER WITH RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 *     INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK
 *     ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
 *     NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S SPECIFICATION
 *     OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
 *
 *     BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE LIABILITY WITH
 *     RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION,
TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE
 *     FEES OR SERVICE CHARGE PAID BY BUYER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 *     THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE WITH THE LAWS
 *     OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF LAWS PRINCIPLES.
 ************************************************************************************************/
#define LOG_TAG "aaa_hal_base"

#include <stdlib.h>
#include <stdio.h>
#include <cutils/log.h>
#include <aaa_types.h>
#include <aaa_error_code.h>
#include "aaa_hal.h"
#include "aaa_hal_yuv.h"
//
#include <mtkcam/hal/sensor_hal.h>

using namespace NS3A;

/*******************************************************************************
*
********************************************************************************/
Hal3ABase*
Hal3ABase::createInstance(MINT32 const i4SensorDevId)
{
    halSensorType_e eSensorType;    // sensor hal defined

    SensorHal* pSensorHal = SensorHal::createInstance();
    if ( !pSensorHal )
    {
        return NULL;
    }
    //
    pSensorHal->sendCommand(static_cast<halSensorDev_e>(i4SensorDevId), 
                            SENSOR_CMD_SET_SENSOR_DEV,
                            0,
                            0,
                            0
                           );
    //
    pSensorHal->init();
    //
    pSensorHal->sendCommand(static_cast<halSensorDev_e>(i4SensorDevId), 
                             SENSOR_CMD_GET_SENSOR_TYPE, 
                             reinterpret_cast<int>(&eSensorType), 
                             0, 
                             0
                            );
    //
    if  ( pSensorHal )
    {
        pSensorHal->uninit();
        pSensorHal->destroyInstance();
        pSensorHal = NULL;
    }
    
    if (eSensorType == SENSOR_TYPE_RAW) {
#ifdef USING_MTK_LDVT
    static Hal3ABase singleton;
    return &singleton;
#else
        return Hal3A::createInstance(i4SensorDevId);
#endif
    }
    else if (eSensorType == SENSOR_TYPE_YUV)
    {
#ifdef USING_MTK_LDVT
         static Hal3ABase singleton;
         return &singleton;
#else
        return Hal3AYuv::createInstance(i4SensorDevId);
#endif        
    }
    else {
#if 1        
    static Hal3ABase singleton;
    return &singleton;        
#else    
    return NULL;//Hal3AYuv::getInstance();
#endif
    }
}



