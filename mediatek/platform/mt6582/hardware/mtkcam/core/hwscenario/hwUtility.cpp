

#define LOG_TAG "MtkCam/hwUtil"
/*******************************************************************************
*
********************************************************************************/
#include <mtkcam/common.h>
#include <mtkcam/imageio/ispio_pipe_scenario.h>
#include <mtkcam/imageio/ispio_pipe_ports.h>
#include <mtkcam/imageio/ispio_pipe_buffer.h>
#include <mtkcam/imageio/ispio_stddef.h>
#include <mtkcam/hal/sensor_hal.h>
using namespace NSImageio;
using namespace NSIspio;
//
#include <mtkcam/v1/hwscenario/IhwScenarioType.h>
using namespace NSHwScenario;
#include <mtkcam/v1/hwscenario/IhwScenario.h>
#include "hwUtility.h"
#include <mtkcam/common/camutils/CamFormat.h>
//
/*******************************************************************************
*
********************************************************************************/
#include <mtkcam/Log.h>
#define MY_LOGV(fmt, arg...)    CAM_LOGV("[%s] "fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)    CAM_LOGD("[%s] "fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)    CAM_LOGI("[%s] "fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)    CAM_LOGW("[%s] "fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)    CAM_LOGE("[%s] "fmt, __FUNCTION__, ##arg)

#define MY_LOGV_IF(cond, arg...)    if (cond) { MY_LOGV(arg); }
#define MY_LOGD_IF(cond, arg...)    if (cond) { MY_LOGD(arg); }
#define MY_LOGI_IF(cond, arg...)    if (cond) { MY_LOGI(arg); }
#define MY_LOGW_IF(cond, arg...)    if (cond) { MY_LOGW(arg); }
#define MY_LOGE_IF(cond, arg...)    if (cond) { MY_LOGE(arg); }
/*******************************************************************************
*
*******************************************************************************/
EScenarioFmt
mapSensorType(halSensorType_e const & type)
{
    EScenarioFmt mapSensorType;

    switch (type){
        case SENSOR_TYPE_RAW:
            mapSensorType =  eScenarioFmt_RAW;
            break;
        case SENSOR_TYPE_YUV:
            mapSensorType =  eScenarioFmt_YUV;
            break;
        default:
            mapSensorType = eScenarioFmt_UNKNOWN;
            MY_LOGE("Unknown sensor type!!");
            break;
    }
    
    return mapSensorType;
}


/*******************************************************************************
*
********************************************************************************/
MVOID 
mapPortCfg(EHwBufIdx const src, PortID &dst)
{
    switch (src){
       case eID_Pass1Out:
            dst.index = EPortIndex_IMGO;
           break;
        case eID_Pass2In:
            dst.index = EPortIndex_IMGI;
            break;
        case eID_Pass2DISPO:
            dst.index = EPortIndex_DISPO;  
            break;
        case eID_Pass2VIDO:
            dst.index = EPortIndex_VIDO;  
            break;
        // zsd added
        case eID_Pass1RawOut:
            dst.index = EPortIndex_IMGO;
            break;
        case eID_Pass1DispOut:
            dst.index = EPortIndex_IMG2O;
            break;
        default:
            MY_LOGE("Unknown port type!!");
            break;
    }
}


/*******************************************************************************
*
********************************************************************************/
MVOID
mapBufCfg(IhwScenario::PortBufInfo const &src, QBufInfo &dst)
{
    BufInfo buf(src.bufSize,
                src.virtAddr,
                src.phyAddr,
                src.memID,
                src.bufSecu,
                src.bufCohe); 
    //
    MY_LOGD_IF(0, "A(0x%08X/0x%08X),S(%d),Id(%d),S/C(%d/%d)",  
            buf.u4BufVA,
            buf.u4BufPA,
            buf.u4BufSize,
            buf.memID,
            buf.bufSecu,
            buf.bufCohe);
    //
    dst.vBufInfo.push_back(buf);
}


/*******************************************************************************
*
********************************************************************************/
MVOID 
mapConfig(IhwScenario::PortBufInfo const &rsrc, PortID &rPortID, QBufInfo &rQbufInfo)
{
    mapPortCfg(rsrc.ePortIndex, rPortID);
    mapBufCfg(rsrc, rQbufInfo);
}


/*******************************************************************************
*
********************************************************************************/
MVOID 
mapFormat(const char * const src, EImageFormat &dst)
{
    MUINT32 value = 0;
    
    value = android::MtkCamUtils::FmtUtils::queryImageioFormat(src);
    dst = (EImageFormat)value;
    
    //MY_LOGD_IF(value > 0, "F(%d)", value);
    MY_LOGD_IF(value == 0, "F(%s) has not done yet", src); 
}



