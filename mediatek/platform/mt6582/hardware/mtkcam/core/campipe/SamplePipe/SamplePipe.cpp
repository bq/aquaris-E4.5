
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
#define LOG_TAG "campipe/sample"
//
#include <mtkcam/Log.h>
#define MY_LOGV(fmt, arg...)    CAM_LOGV("[%s] "fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)    CAM_LOGD("[%s] "fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)    CAM_LOGI("[%s] "fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)    CAM_LOGW("[%s] "fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)    CAM_LOGE("[%s] "fmt, __FUNCTION__, ##arg)
#define FUNCTION_LOG_START      MY_LOGD("+");
#define FUNCTION_LOG_END        MY_LOGD("-");

//
#include <mtkcam/common.h>
#include <mtkcam/common/hw/hwstddef.h>
//
#include "../inc/PipeImp.h"
#include "../inc/SamplePipe.h"
//


/*******************************************************************************
*
********************************************************************************/
namespace NSCamPipe {
////////////////////////////////////////////////////////////////////////////////


/*******************************************************************************
* 
********************************************************************************/
SamplePipe::
SamplePipe(
    char const*const szPipeName, 
    EPipeID const ePipeID, 
    ESWScenarioID const eSWScenarioID, 
    EScenarioFmt const eScenarioFmt
)
    : PipeImp(szPipeName, ePipeID, eSWScenarioID, eScenarioFmt)
{
}


/*******************************************************************************
* 
********************************************************************************/
MBOOL
SamplePipe::
init()
{
    FUNCTION_LOG_START;


        
    FUNCTION_LOG_END;
    //
    return MTRUE;
}


/*******************************************************************************
* 
********************************************************************************/
MBOOL
SamplePipe::
uninit()
{
     FUNCTION_LOG_START;



     FUNCTION_LOG_END;
     //
     return MTRUE;
}


/*******************************************************************************
* 
********************************************************************************/
MBOOL
SamplePipe::
start()
{
    FUNCTION_LOG_START

    FUNCTION_LOG_END

    return  MTRUE;
}


/*******************************************************************************
* 
********************************************************************************/
MBOOL
SamplePipe::
stop()
{
    FUNCTION_LOG_START;

    FUNCTION_LOG_END;
    //
    return MTRUE;
}


/*******************************************************************************
* 
********************************************************************************/
MBOOL
SamplePipe::
enqueBuf(PortID const ePortID, QBufInfo const& rQBufInfo)
{
    MY_LOGD("+ tid(%d) PortID:(type, index, inout)=(%d, %d, %d)",  gettid(), ePortID.type, ePortID.index, ePortID.inout);
    MY_LOGD("QBufInfo:(user, reserved, num)=(%x, %d, %d)", rQBufInfo.u4User, rQBufInfo.u4Reserved, rQBufInfo.vBufInfo.size());
    return  MTRUE;
}


/*******************************************************************************
* 
********************************************************************************/
MBOOL
SamplePipe::
dequeBuf(PortID const ePortID, QTimeStampBufInfo& rQBufInfo, MUINT32 const u4TimeoutMs /*= 0xFFFFFFFF*/)
{
    MY_LOGD("+ tid(%d) PortID:(type, index, inout, timeout)=(%d, %d, %d, %d)", gettid(), ePortID.type, ePortID.index, ePortID.inout, u4TimeoutMs);
    return  MTRUE;
}


/*******************************************************************************
* 
********************************************************************************/
MBOOL
SamplePipe::
configPipe(vector<PortInfo const*>const& vInPorts, vector<PortInfo const*>const& vOutPorts)
{
    MY_LOGD("+ %d in / %d out", vInPorts.size(), vOutPorts.size());
    return  MTRUE;
}

/*******************************************************************************
* 
********************************************************************************/
MBOOL
SamplePipe::
queryPipeProperty(vector<PortProperty> &vInPorts, vector<PortProperty> &vOutPorts)
{
    MY_LOGD("+ %d in / %d out", vInPorts.size(), vOutPorts.size());
    return  MTRUE;
}



/*******************************************************************************
* 
********************************************************************************/
MBOOL
SamplePipe::
sendCommand(MINT32 cmd, MINT32 arg1, MINT32 arg2, MINT32 arg3)
{
   return MTRUE; 
}

////////////////////////////////////////////////////////////////////////////////
};  //namespace NSCamPipe



