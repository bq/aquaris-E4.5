
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
#include "inc/MtkEngCamAdapter.h"
#include <mtkcam/common.h>
using namespace NSCam;
#include "EngParam.h"
using namespace NSMtkEngCamAdapter;
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
bool
createShotInstance(
    sp<IShot>&          rpShot, 
    uint32_t const      u4ShotMode, 
    int32_t const       i4OpenId, 
    sp<IParamsManager>  pParamsMgr
);


/******************************************************************************
*
*******************************************************************************/
bool
CamAdapter::
updateShotInstance()
{
    String8 const s8ShotMode = getParamsManager()->getShotModeStr();
    uint32_t const u4ShotMode = getParamsManager()->getShotMode();
    MY_LOGI("<shot mode> %#x(%s)", u4ShotMode, s8ShotMode.string());
    //
    MY_LOGD("Engineer mode use EngShot only");
    return  createShotInstance(mpShot, eShotMode_EngShot, getOpenId(), getParamsManager()); // In engineer mode, force using EngShot
}


/******************************************************************************
*
*******************************************************************************/
bool
CamAdapter::
isTakingPicture() const
{
    bool ret =  mpStateManager->isState(IState::eState_Capture)
            ||  mpStateManager->isState(IState::eState_PreCapture)
                ;
    if  ( ret )
    {
        MY_LOGD("isTakingPicture(1):%s", mpStateManager->getCurrentState()->getName());
    }
    //
    return  ret;
}


/******************************************************************************
*
*******************************************************************************/
status_t
CamAdapter::
takePicture()
{
    status_t status = OK;
    //
    status = mpStateManager->getCurrentState()->onPreCapture(this);
    if  ( OK != status ) {
        goto lbExit;
    }
    status = mpStateManager->getCurrentState()->onStopPreview(this);
    if  ( OK != status ) {
        goto lbExit;
    }
    status = mpStateManager->getCurrentState()->onCapture(this);
    if  ( OK != status ) {
        goto lbExit;
    }
    //
lbExit:
    return status;
}


/******************************************************************************
*
*******************************************************************************/
status_t
CamAdapter::
cancelPicture()
{
    return  mpStateManager->getCurrentState()->onCancelCapture(this);
}


/******************************************************************************
*   CamAdapter::takePicture() -> IState::onCapture() -> 
*   IStateHandler::onHandleCapture() -> CamAdapter::onHandleCapture()
*******************************************************************************/
status_t
CamAdapter::
onHandleCapture()
{
    status_t status = DEAD_OBJECT;
    //
    sp<ICaptureCmdQueThread> pCaptureCmdQueThread = mpCaptureCmdQueThread;
    if  ( pCaptureCmdQueThread != 0 ) {
        status = pCaptureCmdQueThread->onCapture();
    }
    //
    return  status;
}


/******************************************************************************
*   
*******************************************************************************/
status_t
CamAdapter::
onHandleCaptureDone()
{
#if 0
    //  Message may disable before shutter/image callback if: DONE --> Image CB
    mNotifyCb(MTK_CAMERA_MSG_EXT_NOTIFY, MTK_CAMERA_MSG_EXT_NOTIFY_CAPTURE_DONE, 0, mCallbackCookie);
#endif
    mpStateManager->transitState(IState::eState_Idle);
    return  OK;
}


/******************************************************************************
*   CamAdapter::cancelPicture() -> IState::onCancelCapture() -> 
*   IStateHandler::onHandleCancelCapture() -> CamAdapter::onHandleCancelCapture()
*******************************************************************************/
status_t
CamAdapter::
onHandleCancelCapture()
{
    sp<IShot> pShot = mpShot;
    if  ( pShot != 0 )
    {
        pShot->sendCommand(eCmd_cancel);
    }
    //
    return  OK;
}


/******************************************************************************
*   
*******************************************************************************/
bool
CamAdapter::
onCaptureThreadLoop()
{
    bool ret = false;
    //
    //  [1] transit to "Capture" state.
    mpStateManager->transitState(IState::eState_Capture);
    //
    sp<IParamsManager> pParamsMgr = getParamsManager();

    String8 ms8IspMode = pParamsMgr->getStr(MtkCameraParameters::KEY_ISP_MODE);
    const char *strIspMode = ms8IspMode.string();
    engParam.mi4EngIspMode = EngParam::ENG_RAW_TYPE_PURE_RAW; // Default value
    switch (strIspMode[0])
    {
        case EngParam::ENG_ISP_MODE_PROCESSED_RAW:
            engParam.mi4EngIspMode = EngParam::ENG_RAW_TYPE_PROCESSED_RAW;
            break;
        case EngParam::ENG_ISP_MODE_PURE_RAW:
            engParam.mi4EngIspMode = EngParam::ENG_RAW_TYPE_PURE_RAW;
            break;
    }
    MY_LOGD("engParam.mi4EngIspMode = %d", engParam.mi4EngIspMode);

    String8 ms8SaveMode = pParamsMgr->getStr(MtkCameraParameters::KEY_RAW_SAVE_MODE);
    const char *strSaveMode = ms8SaveMode.string();
#warning "[TODO] Magic number: need to enumerize"
    switch (strSaveMode[0])
    {
        case '1': // 1: "Preview Mode", 
            engParam.mi4EngSensorMode = EngParam::ENG_SENSOR_MODE_PREVIEW;
            engParam.mi4EngRawSaveEn = 1;
            break;
        case '2': // 2: "Capture Mode",
            engParam.mi4EngSensorMode = EngParam::ENG_SENSOR_MODE_CAPTURE;
            engParam.mi4EngRawSaveEn = 1;        
            break;
        case '4': // 4: "Video Preview Mode"
            engParam.mi4EngSensorMode = EngParam::ENG_SENSOR_MODE_VIDEO_PREVIEW;
            engParam.mi4EngRawSaveEn = 1;        
            break;
        case '0': // 0: do not save
        case '3': // 3: "JPEG Only"
        default:
            engParam.mi4EngRawSaveEn = 0;
            break;
    }
    MY_LOGD("engParam.mi4EngRawSaveEn = %d", engParam.mi4EngRawSaveEn);

    //  [2.1] update mpShot instance.
    ret = updateShotInstance();
    sp<IShot> pShot = mpShot;
    //
    //  [2.2] return if no shot instance.
    if  ( ! ret || pShot == 0 )
    {
#warning "[TODO] perform a dummy compressed-image callback or CAMERA_MSG_ERROR to inform app of end of capture?"
        MY_LOGE("updateShotInstance(%d), pShot.get(%p)", ret, pShot.get());
        goto lbExit;
    }
    else
    {
        //  [3.1] prepare parameters
        sp<IParamsManager> pParamsMgr = getParamsManager();
        int iPictureWidth = 0, iPictureHeight = 0;
        pParamsMgr->getPictureSize(&iPictureWidth, &iPictureHeight);
        int iPreviewWidth = 0, iPreviewHeight = 0;
        pParamsMgr->getPreviewSize(&iPreviewWidth, &iPreviewHeight);
        String8 s8DisplayFormat = mpImgBufProvidersMgr->queryFormat(IImgBufProvider::eID_DISPLAY);
        if  ( String8::empty() == s8DisplayFormat ) {
            MY_LOGW("Display Format is empty");
        }
        //
        //  [3.2] prepare parameters: ShotParam
        // ShotParam shotParam;
        engParam.ms8PictureFormat          = pParamsMgr->getStr(CameraParameters::KEY_PICTURE_FORMAT);
        engParam.mi4PictureWidth           = iPictureWidth;
        engParam.mi4PictureHeight          = iPictureHeight;
        engParam.ms8PostviewDisplayFormat  = s8DisplayFormat;
        engParam.ms8PostviewClientFormat   = pParamsMgr->getStr(CameraParameters::KEY_PREVIEW_FORMAT);
        engParam.mi4PostviewWidth          = iPreviewWidth;
        engParam.mi4PostviewHeight         = iPreviewHeight;
        engParam.ms8ShotFileName           = pParamsMgr->getStr(MtkCameraParameters::KEY_CAPTURE_PATH);
        engParam.mu4ZoomRatio              = pParamsMgr->getZoomRatio();
        engParam.mu4ShotCount              = pParamsMgr->getInt(MtkCameraParameters::KEY_BURST_SHOT_NUM);
        engParam.mi4Rotation               = pParamsMgr->getInt(CameraParameters::KEY_ROTATION);
        //
        //  [3.3] prepare parameters: JpegParam
        JpegParam jpegParam;
        jpegParam.mu4JpegQuality            = pParamsMgr->getInt(CameraParameters::KEY_JPEG_QUALITY);
        jpegParam.mu4JpegThumbQuality       = pParamsMgr->getInt(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY);
        jpegParam.mi4JpegThumbWidth         = pParamsMgr->getInt(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH);
        jpegParam.mi4JpegThumbHeight        = pParamsMgr->getInt(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT);
        jpegParam.ms8GpsLatitude            = pParamsMgr->getStr(CameraParameters::KEY_GPS_LATITUDE);
        jpegParam.ms8GpsLongitude           = pParamsMgr->getStr(CameraParameters::KEY_GPS_LONGITUDE);
        jpegParam.ms8GpsAltitude            = pParamsMgr->getStr(CameraParameters::KEY_GPS_ALTITUDE);
        jpegParam.ms8GpsTimestamp           = pParamsMgr->getStr(CameraParameters::KEY_GPS_TIMESTAMP);
        jpegParam.ms8GpsMethod              = pParamsMgr->getStr(CameraParameters::KEY_GPS_PROCESSING_METHOD);
        //
        //  [4.1] perform Shot operations.
        ret =   
                pShot->sendCommand(eCmd_reset)
            &&  pShot->setCallback(this)
            &&  pShot->sendCommand(eCmd_setShotParam, (uint32_t)&engParam, sizeof(EngParam))
            &&  pShot->sendCommand(eCmd_setJpegParam, (uint32_t)&jpegParam, sizeof(JpegParam))
            &&  pShot->sendCommand(eCmd_capture)
                ;
        if  ( ! ret )
        {
            MY_LOGE("fail to perform shot operations");
        }
    }
    //
    //
lbExit:
    //
    //  [5.1] uninit shot instance.
    MY_LOGD("free shot instance: (mpShot/pShot)=(%p/%p)", mpShot.get(), pShot.get());
    mpShot = NULL;
    pShot  = NULL;
    //  [5.11] update focus steps.
    //
    pParamsMgr->updateBestFocusStep();
    //
    //  [5.2] notify capture done.
    mpStateManager->getCurrentState()->onCaptureDone(this);
    //
    //
    return  true;
}



