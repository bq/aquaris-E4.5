
#define LOG_TAG "MtkCam/CamAdapter"
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
#include <inc/MtkVTCamAdapter.h>
using namespace NSMtkVTCamAdapter;
//
#include <cutils/properties.h>
//
/******************************************************************************
*
*******************************************************************************/
#define MY_LOGV(fmt, arg...)        CAM_LOGV("(%d)(%s)[%s] "fmt, ::gettid(), getName(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_LOGD("(%d)(%s)[%s] "fmt, ::gettid(), getName(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_LOGI("(%d)(%s)[%s] "fmt, ::gettid(), getName(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_LOGW("(%d)(%s)[%s] "fmt, ::gettid(), getName(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_LOGE("(%d)(%s)[%s] "fmt, ::gettid(), getName(), __FUNCTION__, ##arg)
//
#define MY_LOGV_IF(cond, arg...)    if (cond) { MY_LOGV(arg); }
#define MY_LOGD_IF(cond, arg...)    if (cond) { MY_LOGD(arg); }
#define MY_LOGI_IF(cond, arg...)    if (cond) { MY_LOGI(arg); }
#define MY_LOGW_IF(cond, arg...)    if (cond) { MY_LOGW(arg); }
#define MY_LOGE_IF(cond, arg...)    if (cond) { MY_LOGE(arg); }

/******************************************************************************
*
*******************************************************************************/
bool
CamAdapter::
previewEnabled() const
{
    return (    mpStateManager->isState(IState::eState_Preview)||
                recordingEnabled());
}


/******************************************************************************
*
*******************************************************************************/
status_t
CamAdapter::
startPreview()
{
    return  mpStateManager->getCurrentState()->onStartPreview(this);
}


/******************************************************************************
*
*******************************************************************************/
void
CamAdapter::
stopPreview()
{
    mpStateManager->getCurrentState()->onStopPreview(this);
}


/******************************************************************************
*   CamAdapter::startPreview() -> IState::onStartPreview() -> 
*   IStateHandler::onHandleStartPreview() -> CamAdapter::onHandleStartPreview()
*******************************************************************************/
status_t
CamAdapter::
onHandleStartPreview()
{
    MY_LOGD("+");

    //
    int32_t eResourceType = ResourceLock::eMTK_VT;
    //
    if ( ! mpResourceLock->SetMode((ResourceLock::ECamAdapter)eResourceType) )
    {
        CAM_LOGE("Resource SetMode fail");
        return INVALID_OPERATION;
    }

    //
    if ( ! mpResourceLock->Lock((ResourceLock::ECamAdapter)eResourceType) )
    {
        CAM_LOGE("Resource Lock fail");
        return INVALID_OPERATION;
    }
    
    //
    if ( ! mpPreviewCmdQueThread->postCommand(PrvCmdCookie::eStart, PrvCmdCookie::eSemAfter) )
    {
        MY_LOGE("StartPreview stage 1 (start): fail");
        goto lbExit;
    }
    
    //
    if ( ! mpPreviewCmdQueThread->postCommand(PrvCmdCookie::eDelay, PrvCmdCookie::eSemAfter) )
    {
         MY_LOGE("StartPreview stage 2 (delay): fail");
         goto lbExit;
    }
      
    //
    if ( ! mpPreviewCmdQueThread->postCommand(PrvCmdCookie::eUpdate, PrvCmdCookie::eSemBefore) )
    {
         MY_LOGE("StartPreview stage 3 (udpate): fail");
         goto lbExit;
    }
    //
    return OK;
   //
lbExit:
    MY_LOGD("-");
    return INVALID_OPERATION;
}


/******************************************************************************
*   CamAdapter::stopPreview() -> IState::onStopPreview() -> 
*   IStateHandler::onHandleStopPreview() -> CamAdapter::onHandleStopPreview()
*******************************************************************************/
status_t
CamAdapter::
onHandleStopPreview()
{
    MY_LOGD("+");

    //
    int32_t eResourceType = ResourceLock::eMTK_VT;
    //
    if ( ! mpPreviewCmdQueThread->postCommand(PrvCmdCookie::eStop, PrvCmdCookie::eSemAfter) )
    {
        MY_LOGE("StopPreview fail");
        goto lbExit;
    }
    //
    if ( ! mpResourceLock->Unlock((ResourceLock::ECamAdapter)eResourceType) )
    {
        CAM_LOGE("Resource Lock fail");
        return INVALID_OPERATION;
    }
    //
    if ( ! mpResourceLock->SetMode(ResourceLock::eMTKCAM_IDLE) )
    {
        CAM_LOGE("Resource SetMode fail");
        return INVALID_OPERATION;
    }

    MY_LOGD("-");

    return OK;
    
lbExit:
    return INVALID_OPERATION;
}



