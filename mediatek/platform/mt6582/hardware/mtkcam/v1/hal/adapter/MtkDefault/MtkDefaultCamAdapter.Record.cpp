
#define LOG_TAG "MtkCam/CamAdapter"
//
#include <camera/MtkCamera.h>
//
#include <inc/CamUtils.h>
using namespace android;
using namespace MtkCamUtils;
//
#include <inc/ImgBufProvidersManager.h>
//
#include <mtkcam/v1/IParamsManager.h>
#include <mtkcam/v1/ICamAdapter.h>
#include <inc/BaseCamAdapter.h>
#include "inc/MtkDefaultCamAdapter.h"
using namespace NSMtkDefaultCamAdapter;
//


/******************************************************************************
*
*******************************************************************************/
#define MY_LOGV(fmt, arg...)        CAM_LOGV("(%d)(%s)[%s] "fmt, ::gettid(), getName(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_LOGD("(%d)(%s)[%s] "fmt, ::gettid(), getName(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_LOGI("(%d)(%s)[%s] "fmt, ::gettid(), getName(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_LOGW("(%d)(%s)[%s] "fmt, ::gettid(), getName(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_LOGE("(%d)(%s)[%s] "fmt, ::gettid(), getName(), __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_LOGA("(%d)(%s)[%s] "fmt, ::gettid(), getName(), __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_LOGF("(%d)(%s)[%s] "fmt, ::gettid(), getName(), __FUNCTION__, ##arg)
//
#define MY_LOGV_IF(cond, ...)       do { if ( (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( (cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( (cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)


/******************************************************************************
*   Function Prototype.
*******************************************************************************/
status_t
CamAdapter::
startRecording()
{
    status_t status = OK;
    //
    if(recordingEnabled())
    {
        MY_LOGW("Recording has been started");
    }
    else
    {
        status = mpStateManager->getCurrentState()->onStartRecording(this);
        if  ( OK != status ) {
            goto lbExit;
        }
    }
    //
lbExit:
    return status;
}


/******************************************************************************
*
*******************************************************************************/
void
CamAdapter::
stopRecording()
{
    if(recordingEnabled())
    {
        mpStateManager->getCurrentState()->onStopRecording(this);
    }
    else
    {
        MY_LOGW("Recording has been stopped");
    }
}


/******************************************************************************
*
*
*******************************************************************************/
status_t
CamAdapter::
onHandleStartRecording()
{
    MY_LOGD("+");
    //
    status_t ret = OK;
    NS3A::Param_T cam3aParam;
    Hal3ABase* p3AHal = Hal3ABase::createInstance(DevMetaInfo::queryHalSensorDev(getOpenId()));
    if ( ! p3AHal )
    {
        MY_LOGE("p3AHal == NULL");
        return INVALID_OPERATION;
    }
    //
    if ( ! p3AHal->getParams(cam3aParam) )
    {
        MY_LOGE("getParams fail");
        ret = INVALID_OPERATION;
        goto lbExit;
    }
    //
    if ( ! mpResourceLock->SetMode(ResourceLock::eMTKVIDEO_REC) )
    {
        CAM_LOGE("Resource SetMode fail");
        return INVALID_OPERATION;
    }
    //
    if(mpStateManager->isState(IState::eState_VideoSnapshot))
    {
        MY_LOGE("VSS to Record state");
        mpStateManager->transitState(IState::eState_Recording);
        return  OK;
    }
    //
    if ( ! mpResourceLock->Lock(ResourceLock::eMTKVIDEO_REC) )
    {
        CAM_LOGE("Resource Lock fail");
        return INVALID_OPERATION;
    }
    //
    mpStateManager->transitState(IState::eState_Recording);
    //
    cam3aParam.i4MinFps = mpParamsMgr->getInt(CameraParameters::KEY_PREVIEW_FRAME_RATE)*1000;
    cam3aParam.i4MaxFps = mpParamsMgr->getInt(CameraParameters::KEY_PREVIEW_FRAME_RATE)*1000;
    MY_LOGD("FPS:RH(%d),Min(%d),Max(%d)",mpParamsMgr->getRecordingHint(),cam3aParam.i4MinFps,cam3aParam.i4MaxFps);
    //
    if ( ! p3AHal->setParams(cam3aParam) )
    {
        MY_LOGE("setParams fail");
        ret = INVALID_OPERATION;
        goto lbExit;
    }
    //
    mpPreviewCmdQueThread->startRecording();
    //
    MY_LOGD("Start recording");
    //
lbExit:
    p3AHal->destroyInstance();
    MY_LOGD("-");
    //
    return  ret;
}


/******************************************************************************
*
*
*******************************************************************************/
status_t
CamAdapter::
onHandleStopRecording()
{
    MY_LOGD("+");
    //
    status_t ret = OK;
    NS3A::Param_T cam3aParam;
    Hal3ABase* p3AHal = Hal3ABase::createInstance(DevMetaInfo::queryHalSensorDev(getOpenId()));
    if ( ! p3AHal )
    {
        MY_LOGE("p3AHal == NULL");
        return INVALID_OPERATION;
    }
    //
    if ( ! p3AHal->getParams(cam3aParam) )
    {
        MY_LOGE("getParams fail");
        ret = INVALID_OPERATION;
        goto lbExit;
    }
    //
    if(mpStateManager->isState(IState::eState_VideoSnapshot))
    {
        if(mpVideoSnapshotScenario != NULL)
        {
            if( mpVideoSnapshotScenario->getStatus() == IVideoSnapshotScenario::Status_WaitImage)
            {
                MY_LOGW("Disable VSS due to stop record");
                mpVideoSnapshotScenario->enable(MFALSE);
            }
            else
            {
                IStateManager::StateObserver stateWaiter(mpStateManager);
                mpStateManager->registerOneShotObserver(&stateWaiter);
                if  ( OK != stateWaiter.waitState(IState::eState_Recording) )
                {
                    MY_LOGE("Wait VSS to record state fail");
                    ret = TIMED_OUT;
                    goto lbExit;
                }
                MY_LOGD("VSS done");
            }
        }
        else
        {
            MY_LOGE("mpVideoSnapshotScenario is NULL");
        }
    }
    //
    mpPreviewCmdQueThread->stopRecording();
    //
    if(mpParamsMgr->getRecordingHint())
    {
        cam3aParam.i4MinFps = mpParamsMgr->getInt(CameraParameters::KEY_PREVIEW_FRAME_RATE)*1000;
        cam3aParam.i4MaxFps = mpParamsMgr->getInt(CameraParameters::KEY_PREVIEW_FRAME_RATE)*1000;
    }
    else
    {
        cam3aParam.i4MinFps   = 5000;
        cam3aParam.i4MaxFps   = 60000;
    }
    MY_LOGD("FPS:RH(%d),Min(%d),Max(%d)",mpParamsMgr->getRecordingHint(),cam3aParam.i4MinFps,cam3aParam.i4MaxFps);
    //
    if ( ! p3AHal->setParams(cam3aParam) )
    {
        MY_LOGE("setParams fail");
        ret = INVALID_OPERATION;
        goto lbExit;
    }
    //
    mpStateManager->transitState(IState::eState_Preview);
    //
    if ( ! mpResourceLock->Unlock(ResourceLock::eMTKVIDEO_REC) )
    {
        CAM_LOGE("Resource Lock fail");
        return INVALID_OPERATION;
    }
    //
    if ( ! mpResourceLock->SetMode(ResourceLock::eMTKVIDEO_PRV) )
    {
        CAM_LOGE("Resource SetMode fail");
        return INVALID_OPERATION;
    }
    //
    MY_LOGD("Stop recording");
    //
lbExit:
    p3AHal->destroyInstance();
    MY_LOGD("-");
    //
    return  ret;
}


/******************************************************************************
*
*
*******************************************************************************/
bool
CamAdapter::
recordingEnabled() const
{

    return (    mpStateManager->isState(IState::eState_Recording)||
                mpStateManager->isState(IState::eState_VideoSnapshot));
}


/******************************************************************************
*
*
*******************************************************************************/
status_t
CamAdapter::
onHandleVideoSnapshot()
{
    status_t status = DEAD_OBJECT;
    //
    if(!(mpStateManager->isState(IState::eState_Recording)))
    {
        MY_LOGE("It is not in recording state");
        status = INVALID_OPERATION;
        goto lbExit;
    }
    //
    if(mpVideoSnapshotScenario != NULL)
    {
        if ( ! mpResourceLock->SetMode(ResourceLock::eMTKVIDEO_VSS) )
        {
            CAM_LOGE("Resource SetMode fail");
            return INVALID_OPERATION;
        }
        //
        if ( ! mpResourceLock->Lock(ResourceLock::eMTKVIDEO_VSS) )
        {
            CAM_LOGE("Resource Lock fail");
            return INVALID_OPERATION;
        }
        //
        if(mpVideoSnapshotScenario->enable(MTRUE))
        {
            MY_LOGD("mpVideoSnapshotScenario->enable OK");
            mpStateManager->transitState(IState::eState_VideoSnapshot);
            status = OK;
        }
        else
        {
            MY_LOGE("mpVideoSnapshotScenario->enable Fail");
        }
    }
    else
    {
        MY_LOGE("mpVideoSnapshotScenario is NULL");
    }
    //
lbExit:
    return  status;
}



