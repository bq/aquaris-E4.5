
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
*
*******************************************************************************/
sp<ICamAdapter>
createMtkDefaultCamAdapter(
    String8 const&      rName, 
    int32_t const       i4OpenId, 
    sp<IParamsManager>  pParamsMgr
)
{
    return new CamAdapter(
        rName, 
        i4OpenId, 
        pParamsMgr
    );
}


/******************************************************************************
*
*******************************************************************************/
CamAdapter::
CamAdapter(
    String8 const&      rName, 
    int32_t const       i4OpenId, 
    sp<IParamsManager>  pParamsMgr
)
    : BaseCamAdapter(rName, i4OpenId, pParamsMgr)
    //
    , mpStateManager(IStateManager::inst())
    //
    , mpPreviewCmdQueThread(0)
    , mpPreviewBufMgr(NULL)
    //
    , mpCaptureCmdQueThread(0)
    , mpShot(0)
    , mpResourceLock(NULL)
    , mpVideoSnapshotScenario(NULL)
    //
{
    MY_LOGD(
        "sizeof=%d, this=%p, mpStateManager=%p, mpPreviewCmdQueThread=%p, mpPreviewBufMgr=%p, mpCaptureCmdQueThread=%p, mpShot=%p, mpResourceLock=%p", 
        sizeof(CamAdapter), this, &mpStateManager, &mpPreviewCmdQueThread, &mpPreviewBufMgr, &mpCaptureCmdQueThread, &mpShot, &mpResourceLock
    );
}


/******************************************************************************
*
*******************************************************************************/
CamAdapter::
~CamAdapter()
{
    MY_LOGD("tid(%d), OpenId(%d)", ::gettid(), getOpenId());
}


/******************************************************************************
*
*******************************************************************************/
bool
CamAdapter::
init()
{
    status_t status = NO_ERROR;
    //
    if  ( ! BaseCamAdapter::init() ) {
        goto lbExit;
    }
    //
    //   
    mpPreviewBufMgr = IPreviewBufMgr::createInstance(mpImgBufProvidersMgr); 
    mpPreviewCmdQueThread = IPreviewCmdQueThread::createInstance(mpPreviewBufMgr, getOpenId(), mpParamsMgr);
    if  ( mpPreviewCmdQueThread == 0 || OK != (status = mpPreviewCmdQueThread->run()) )
    {
        MY_LOGE(
            "Fail to run PreviewCmdQueThread - mpPreviewCmdQueThread.get(%p), status[%s(%d)]", 
            mpPreviewCmdQueThread.get(), ::strerror(-status), -status
        );
        goto lbExit;
    }
    //
    //
    mpCaptureCmdQueThread = ICaptureCmdQueThread::createInstance(this);
    if  ( mpCaptureCmdQueThread == 0 || OK != (status = mpCaptureCmdQueThread->run() ) )
    {
        MY_LOGE(
            "Fail to run CaptureCmdQueThread - mpCaptureCmdQueThread.get(%p), status[%s(%d)]", 
            mpCaptureCmdQueThread.get(), ::strerror(-status), -status
        );
        goto lbExit;
    }
    //
    //
    if ( OK != init3A() )
    {
        MY_LOGE("Fail to init 3A");
        goto lbExit;
    }
    //
    //
    mpVideoSnapshotScenario = IVideoSnapshotScenario::createInstance();
    if(mpVideoSnapshotScenario != NULL)
    {
        if(!(mpVideoSnapshotScenario->setCallback(this)))
        {
            MY_LOGE("mpVideoSnapshotScenario->setCallback fail");
            goto lbExit;
        }
    }
    else
    {
        MY_LOGE("mpVideoSnapshotScenario is NULL");
        goto lbExit;
    }
    //
    //
    mpResourceLock = ResourceLock::CreateInstance();
    if(mpResourceLock != NULL)
    {
        if(!(mpResourceLock->Init()))
        {
            MY_LOGE("mpResourceLock->Init fail");
            goto lbExit;
        }
    }
    //
    return  true;
lbExit:
    MY_LOGE("init() fail; now call uninit()");
    uninit();
    return  false;
}


/******************************************************************************
*
*******************************************************************************/
bool
CamAdapter::
uninit()
{
    MY_LOGD("+");
    //
    //  Close Command Queue Thread of Capture.
    sp<ICaptureCmdQueThread> pCaptureCmdQueThread = mpCaptureCmdQueThread;
    mpCaptureCmdQueThread = 0;
    if  ( pCaptureCmdQueThread != 0 ) {
        pCaptureCmdQueThread->requestExit();
        pCaptureCmdQueThread = 0;
    }
    //
    //
    sp<IPreviewCmdQueThread> pPreviewCmdQueThread = mpPreviewCmdQueThread; 
    mpPreviewCmdQueThread = 0;       
    if ( pPreviewCmdQueThread != 0 ) {
        MY_LOGD(
            "PreviewCmdQ Thread: (tid, getStrongCount)=(%d, %d)", 
            pPreviewCmdQueThread->getTid(), pPreviewCmdQueThread->getStrongCount()
        );
        pPreviewCmdQueThread->requestExit();
        pPreviewCmdQueThread = 0;
    }
    //
    sp<IPreviewBufMgr> pPreviewBufMgr = mpPreviewBufMgr;
    mpPreviewBufMgr = 0;
    if ( pPreviewBufMgr != 0 )
    {
        pPreviewBufMgr->destroyInstance();
        pPreviewBufMgr = 0;
    }
    //
    //
    uninit3A();
    uninitSmoothZoom();
    //
    if(mpVideoSnapshotScenario != NULL)
    {
        mpVideoSnapshotScenario->destroyInstance();
        mpVideoSnapshotScenario = NULL;
    }
    //
    //
    if(mpResourceLock != NULL)
    {
        if(!(mpResourceLock->Uninit()))
        {
            MY_LOGE("mpResourceLock->Uninit fail");
        }
        mpResourceLock->DestroyInstance();
        mpResourceLock = NULL;
    }
    //
    //
    BaseCamAdapter::uninit();
    //
    MY_LOGD("-");
    return  true;
}


/******************************************************************************
*
*******************************************************************************/
status_t
CamAdapter::
sendCommand(int32_t cmd, int32_t arg1, int32_t arg2)
{
    MY_LOGD("cmd(0x%08X),arg1(0x%08X),arg2(0x%08X)",cmd,arg1,arg2);
    //
    switch  (cmd)
    {
    case CAMERA_CMD_START_SMOOTH_ZOOM:

        return startSmoothZoom(arg1);

    case CAMERA_CMD_STOP_SMOOTH_ZOOM:
        
        return stopSmoothZoom();

    case CAMERA_CMD_ENABLE_FOCUS_MOVE_MSG:
        CAM_LOGD("[sendCommand] CAMERA_CMD_ENABLE_FOCUS_MOVE_MSG (%d)\n", arg1); 
        enableAFMove(arg1);
        return OK;
        break; 

        
    default:
        break;
    }
    return  BaseCamAdapter::sendCommand(cmd, arg1, arg2);
}



