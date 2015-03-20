
#ifndef _MTK_HAL_CAMCLIENT_ASDCLIENT_H_
#define _MTK_HAL_CAMCLIENT_ASDCLIENT_H_
//
#include <CamUtils.h>
using namespace android;
using namespace MtkCamUtils;
//
#include <CamUtils.h>
#include <mtkcam/v1/IParamsManager.h>
#include <IAsdClient.h>
#include <mtkcam/common/faces.h>
#include <system/camera.h>


#include "mtkcam/hal/aaa_hal_base.h"
using namespace NS3A;
#include <mtkcam/featureio/fd_hal_base.h>
#include <mtkcam/featureio/asd_hal_base.h>
#include <mtkcam/hal/sensor_hal.h>

//

#define MHAL_ASD_WORKING_BUF_SIZE       (160*120*2*11+200*1024)

namespace android {
namespace NSCamClient {
namespace NSAsdClient {

/******************************************************************************
 *
 ******************************************************************************/
class AsdClient : public IAsdClient
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////                    Instantiation.
    //
                                    AsdClient(sp<IParamsManager> pParamsMgr);
    virtual                         ~AsdClient();

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////

    virtual bool                    init();
    virtual bool                    uninit();

    virtual void                    setCallbacks(sp<CamMsgCbInfo> const& rpCamMsgCbInfo);

    virtual void                    enable(bool fgEnable);
    virtual bool                    isEnabled() const;

    virtual void                    update(MUINT8 * OT_Buffer, MINT32 a_Buffer_width, MINT32 a_Buffer_height);
    //virtual void                    update(ImgBufQueNode const& rQueNode);

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////                    Common Info.

    mutable Mutex                   mModuleMtx;
    sp<CamMsgCbInfo>                mpCamMsgCbInfo;         //  Pointer to Camera Message-Callback Info.
    sp<IParamsManager>              mpParamsMgr;            //  Pointer to Parameters Manager.
    volatile int32_t                mIsAsdEnabled;
    MtkCameraFaceMetadata*          mpFaceInfo; 
    static const int                mDetectedFaceNum = 15;

    halFDBase*                      mpHalFD;
    MUINT8*                         mpWorkingBuf;
    halSensorType_e                 eSensorType;            // sensor hal defined
    Hal3ABase*                      mpHal3A;
    halASDBase*                     mpHalASDObj;
    mhal_ASD_DECIDER_UI_SCENE_TYPE_ENUM             mSceneCur;
    volatile mhal_ASD_DECIDER_UI_SCENE_TYPE_ENUM    mScenePre;    
};


}; // namespace NSAsdClient
}; // namespace NSCamClient
}; // namespace android
#endif  //_MTK_HAL_CAMCLIENT_ASDCLIENT_H_



