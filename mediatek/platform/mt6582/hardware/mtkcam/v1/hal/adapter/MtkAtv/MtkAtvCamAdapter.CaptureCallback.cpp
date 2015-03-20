
#define LOG_TAG "MtkATV/CaptureCb"
//
#include <camera/MtkCamera.h>
//
#include <mtkcam/v1/config/PriorityDefs.h>
#include <inc/CamUtils.h>
using namespace android;
using namespace MtkCamUtils;
//
#include <inc/ImgBufProvidersManager.h>

// [ATV]+
#include <mtkcam/camshot/ISImager.h>
#include <core/camshot/inc/ImageUtils.h>

using namespace NSCamShot;
// [ATV]-

//
#include <mtkcam/v1/IParamsManager.h>

//
#include <mtkcam/v1/ICamAdapter.h>
#include <inc/BaseCamAdapter.h>
#include "inc/MtkAtvCamAdapter.h"
using namespace NSMtkAtvCamAdapter;
//
#include <mtkcam/drv/hwutils.h>
//
#include <sys/prctl.h>
//


/******************************************************************************
*
*******************************************************************************/
#define MY_LOGV(fmt, arg...)        CAM_LOGV("(%s)[%s] "fmt,  getName(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_LOGD("[CaptureCallback::%s] " fmt,  __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_LOGI("(%s)[%s] "fmt,  getName(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_LOGW("(%s)[%s] "fmt,  getName(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_LOGE("(%s)[%s] "fmt,  getName(), __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_LOGA("(%s)[%s] "fmt,  getName(), __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_LOGF("(%s)[%s] "fmt,  getName(), __FUNCTION__, ##arg)
//
#define MY_LOGV_IF(cond, ...)       do { if ( (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( (cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( (cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)

// [ATV] +
#define CHECK_OBJECT(x)  { if (x == NULL) { MY_LOGE("Null %s Object", #x); return MFALSE;}}
// [ATV] -

/******************************************************************************
 *   
 ******************************************************************************/
//
//  Callback of Error (CAMERA_MSG_ERROR)
//
//  Arguments:
//      ext1
//          [I] extend argument 1.
//
//      ext2
//          [I] extend argument 2.
//
bool
CamAdapter::
onCB_Error(
    int32_t ext1,
    int32_t ext2
)
{
    MY_LOGW("CAMERA_MSG_ERROR %d %d", ext1, ext2);
    mpCamMsgCbInfo->mNotifyCb(CAMERA_MSG_ERROR, ext1, ext2, mpCamMsgCbInfo->mCbCookie);
    return  true;
}


/******************************************************************************
 *   
 ******************************************************************************/
namespace {
struct ShutterThread : public Thread
{
protected:  ////                Data Members.
    sp<CamMsgCbInfo>            mpCamMsgCbInfo;
    int32_t                     mi4PlayShutterSound;

public:
    ShutterThread(
        sp<CamMsgCbInfo> pCamMsgCbInfo, 
        int32_t i4PlayShutterSound
    )
        : Thread()
        , mpCamMsgCbInfo(pCamMsgCbInfo)
        , mi4PlayShutterSound(i4PlayShutterSound)
    {}

    // Good place to do one-time initializations
    status_t
    readyToRun()
    {
        ::prctl(PR_SET_NAME, "ShutterThread", 0, 0, 0);
        //
#if MTKCAM_HAVE_RR_PRIORITY
        int const expect_policy     = SCHED_RR;
        int const expect_priority   = PRIO_RT_CAMERA_SHUTTER_CB;
        int policy = 0, priority = 0;
        setThreadPriority(expect_policy, expect_priority);
        getThreadPriority(policy, priority);
        //
        MY_LOGD(
            "[ShutterThread] policy:(expect, result)=(%d, %d), priority:(expect, result)=(0x%x, 0x%x)"
            , expect_policy, policy, expect_priority, priority
        );
#endif 
        return OK;
    }

private:
    bool
    threadLoop()
    {
        MY_LOGD("(%d)[ShutterThread] +", ::gettid());
#if 1   //defined(MTK_CAMERA_BSP_SUPPORT)
        mpCamMsgCbInfo->mNotifyCb(MTK_CAMERA_MSG_EXT_NOTIFY, MTK_CAMERA_MSG_EXT_NOTIFY_SHUTTER, mi4PlayShutterSound, mpCamMsgCbInfo->mCbCookie);
#else
        mpCamMsgCbInfo->mNotifyCb(CAMERA_MSG_SHUTTER, 0, 0, mpCamMsgCbInfo->mCbCookie);
#endif
        MY_LOGD("(%d)[ShutterThread] -", ::gettid());
        return  false;  // returns false, the thread will exit upon return.
    }
};
}; // namespace


/******************************************************************************
 *   
 ******************************************************************************/
//
//  Callback of Shutter (CAMERA_MSG_SHUTTER)
//
//      Invoking this callback may play a shutter sound.
//
//  Arguments:
//      bPlayShutterSound
//          [I] Play a shutter sound if ture; otherwise play no sound.
//
//      u4CallbackIndex
//          [I] Callback index. 0 by default.
//              If more than one shutter callback must be invoked during
//              captures, for example burst shot & ev shot, this value is
//              the callback index; and 0 indicates the first one callback.
//
bool
CamAdapter::
onCB_Shutter(
    bool const bPlayShutterSound, 
    uint32_t const u4CallbackIndex
)
{
    if  ( msgTypeEnabled(CAMERA_MSG_SHUTTER) )
    {
        sp<Thread> pThread = new ShutterThread(mpCamMsgCbInfo, bPlayShutterSound);
        if  ( pThread == 0 || pThread->run() != OK )
        {
            MY_LOGW("Fail to run ShutterThread (%p)", pThread.get());
            return  false;
        }
    }
    return  true;
}


/******************************************************************************
 *   
 ******************************************************************************/
//
//  Callback of Postview for Display
//
//  Arguments:
//      i8Timestamp
//          [I] Postview timestamp
//
//      u4PostviewSize
//          [I] Postview buffer size in bytes.
//
//      puPostviewBuf
//          [I] Postview buffer with its size = u4PostviewSize
//
bool
CamAdapter::
onCB_PostviewDisplay(
    int64_t const   i8Timestamp, 
    uint32_t const  u4PostviewSize, 
    uint8_t const*  puPostviewBuf
)
{
    MY_LOGD("timestamp(%lld), size/buf=%d/%p", i8Timestamp, u4PostviewSize, puPostviewBuf);
#if 1
    //
    if  ( ! u4PostviewSize || ! puPostviewBuf )
    {
        MY_LOGW("Bad callback: size/buf=%d/%p", i8Timestamp, u4PostviewSize, puPostviewBuf);
        return  false;
    }
    //
    sp<IImgBufProvider> pImgBufPvdr = mpImgBufProvidersMgr->getProvider(IImgBufProvider::eID_DISPLAY);
    if  ( pImgBufPvdr == 0 )
    {
        MY_LOGW("Bad IImgBufProvider");
        return  false;
    }
    //
    ImgBufQueNode node;
    if  ( ! pImgBufPvdr->dequeProvider(node) )
    {
        MY_LOGW("dequeProvider fail");
        return  false;
    }
    //
    sp<IImgBuf> pImgBuf = node.getImgBuf();
    if  ( u4PostviewSize != pImgBuf->getBufSize() )
    {
        MY_LOGW(
            "callback size(%d) != display:[%d %s %dx%d]", 
            u4PostviewSize, pImgBuf->getBufSize(), pImgBuf->getImgFormat().string(), 
            pImgBuf->getImgWidth(), pImgBuf->getImgHeight()
        );
        node.setStatus(ImgBufQueNode::eSTATUS_CANCEL);
    }
    else
    {
        ::memcpy(pImgBuf->getVirAddr(), puPostviewBuf, u4PostviewSize);
        globalcacheFlushAll();
        MY_LOGD_IF(1, "- globalcacheFlushAll()");
        //
        pImgBuf->setTimestamp(i8Timestamp);
        node.setStatus(ImgBufQueNode::eSTATUS_DONE);
    }
    //
    if  ( ! pImgBufPvdr->enqueProvider(node) )
    {
        MY_LOGW("enqueProvider fail");
        return  false;
    }
    //
#endif
    return  true;
}


/******************************************************************************
 *   
 ******************************************************************************/
//
//  Callback of Postview for Client (CAMERA_MSG_POSTVIEW_FRAME)
//
//  Arguments:
//      i8Timestamp
//          [I] Postview timestamp
//
//      u4PostviewSize
//          [I] Postview buffer size in bytes.
//
//      puPostviewBuf
//          [I] Postview buffer with its size = u4PostviewSize
//
bool
CamAdapter::
onCB_PostviewClient(
    int64_t const   i8Timestamp, 
    uint32_t const  u4PostviewSize, 
    uint8_t const*  puPostviewBuf
)
{
    MY_LOGD("timestamp(%lld), size/buf=%d/%p", i8Timestamp, u4PostviewSize, puPostviewBuf);

    MY_LOGW("Not implement yet");

    return  true;
}


/******************************************************************************
 *   
 ******************************************************************************/
//
//  Callback of Raw Image (CAMERA_MSG_RAW_IMAGE/CAMERA_MSG_RAW_IMAGE_NOTIFY)
//
//  Arguments:
//      i8Timestamp
//          [I] Raw image timestamp
//
//      u4RawImgSize
//          [I] Raw image buffer size in bytes.
//
//      puRawImgBuf
//          [I] Raw image buffer with its size = u4RawImgSize
//
bool
CamAdapter::
onCB_RawImage(
    int64_t const   i8Timestamp, 
    uint32_t const  u4RawImgSize, 
    uint8_t const*  puRawImgBuf
)
{
    MY_LOGD("timestamp(%lld), size/buf=%d/%p", i8Timestamp, u4RawImgSize, puRawImgBuf);
    //
    if  ( msgTypeEnabled(CAMERA_MSG_RAW_IMAGE_NOTIFY) )
    {
        MY_LOGD("CAMERA_MSG_RAW_IMAGE_NOTIFY");
        mpCamMsgCbInfo->mNotifyCb(CAMERA_MSG_RAW_IMAGE_NOTIFY, 0, 0, mpCamMsgCbInfo->mCbCookie);
        return  true;
    }
    //
    if  ( msgTypeEnabled(CAMERA_MSG_RAW_IMAGE) )
    {
        MY_LOGD("CAMERA_MSG_RAW_IMAGE");
        if  ( ! u4RawImgSize || ! puRawImgBuf )
        {
            MY_LOGD("dummy callback");
            camera_memory* pmem = mpCamMsgCbInfo->mRequestMemory(-1, 1, 1, NULL);
            if  ( pmem )
            {
                mpCamMsgCbInfo->mDataCb(CAMERA_MSG_RAW_IMAGE, pmem, 0, NULL, mpCamMsgCbInfo->mCbCookie);
                pmem->release(pmem);
            }
        }
        else
        {
            camera_memory* pmem = mpCamMsgCbInfo->mRequestMemory(-1, u4RawImgSize, 1, NULL);
            {
                ::memcpy(pmem->data, puRawImgBuf, u4RawImgSize);
                mpCamMsgCbInfo->mDataCb(CAMERA_MSG_RAW_IMAGE, pmem, 0, NULL, mpCamMsgCbInfo->mCbCookie);
                pmem->release(pmem);
            }
        }
    }
    //
    return  true;
}


/******************************************************************************
 *  ZIP (Compressed) Image Callback Thread
 ******************************************************************************/
namespace {
struct ZipImageCallbackThread : public Thread
{
protected:  ////                Data Members.
    char const*const            mpszThreadName;
    sp<CamMsgCbInfo>            mpCamMsgCbInfo;
    camera_memory*              mpImage;
    uint32_t const              mu4CallbackIndex;
    bool                        mfgIsFinalImage;
    uint32_t                    mu4ShotMode;

public:
    ZipImageCallbackThread(
        sp<CamMsgCbInfo> pCamMsgCbInfo, 
        camera_memory* image, 
        uint32_t const u4CallbackIndex, 
        bool const fgIsFinalImage, 
        uint32_t const u4ShotMode
    )
        : Thread()
        , mpszThreadName("ZipImageCallbackThread")
        , mpCamMsgCbInfo(pCamMsgCbInfo)
        , mpImage(image)
        , mu4CallbackIndex(u4CallbackIndex)
        , mfgIsFinalImage(fgIsFinalImage)
        , mu4ShotMode(u4ShotMode)
    {

    }

    // Good place to do one-time initializations
    status_t
    readyToRun()
    {
        ::prctl(PR_SET_NAME, mpszThreadName, 0, 0, 0);
        //
#if MTKCAM_HAVE_RR_PRIORITY
        int const expect_policy     = SCHED_RR;
        int const expect_priority   = PRIO_RT_CAMERA_ZIP_IMAGE_CB;
        int policy = 0, priority = 0;
        setThreadPriority(expect_policy, expect_priority);
        getThreadPriority(policy, priority);
        //
        MY_LOGD(
            "[%s] policy:(expect, result)=(%d, %d), priority:(expect, result)=(0x%x, 0x%x)"
            , mpszThreadName, expect_policy, policy, expect_priority, priority
        );
#endif
        return OK;
    }

private:
    bool
    threadLoop()
    {
/*    
        if  ( mfgIsFinalImage )
        {
            MY_LOGD("(%d)[%s] the final image: wait done before callback", ::gettid(), mpszThreadName);
            IStateManager* pStateManager = IStateManager::inst();
            IStateManager::StateObserver stateWaiter(pStateManager);
            pStateManager->registerOneShotObserver(&stateWaiter);
            stateWaiter.waitState(IState::eState_Preview);  /// [ATV] eState_Idle
        }
*/        

#if 1   //defined(MTK_CAMERA_BSP_SUPPORT)
        MY_LOGD("(%d)[%s] MTK_CAMERA_MSG_EXT_DATA_COMPRESSED_IMAGE - index(%d)", ::gettid(), mpszThreadName, mu4CallbackIndex);
        mpCamMsgCbInfo->mDataCb(MTK_CAMERA_MSG_EXT_DATA, mpImage, 0, NULL, mpCamMsgCbInfo->mCbCookie);
        mpImage->release(mpImage);
#else
        MY_LOGD("(%d)[%s] CAMERA_MSG_COMPRESSED_IMAGE - index(%d)", ::gettid(), mpszThreadName, mu4CallbackIndex);
        mpCamMsgCbInfo->mDataCb(CAMERA_MSG_COMPRESSED_IMAGE, mpImage, 0, NULL, mpCamMsgCbInfo->mCbCookie);
        mpImage->release(mpImage);
#endif
        // Fix me this, make only continuous shot mode CB end msg
        if  ( mfgIsFinalImage && eShotMode_ContinuousShot == mu4ShotMode )
        {
#if 1   //defined(MTK_CAMERA_BSP_SUPPORT)
            MY_LOGD("Continuous shot end msg callback, total shot number is %d", mu4CallbackIndex);
            mpCamMsgCbInfo->mNotifyCb(MTK_CAMERA_MSG_EXT_NOTIFY, MTK_CAMERA_MSG_EXT_NOTIFY_CONTINUOUS_END, mu4CallbackIndex, mpCamMsgCbInfo->mCbCookie);
#endif
        }

        MY_LOGD("(%d)[%s] -", ::gettid(), mpszThreadName);
        return  false;  // returns false, the thread will exit upon return.
    }

};
}; // namespace


/******************************************************************************
 *   
 ******************************************************************************/
//
//  Callback of Compressed Image (CAMERA_MSG_COMPRESSED_IMAGE)
//
//      [Compressed Image] = [Header] + [Bitstream], 
//      where 
//          Header may be jpeg exif (including thumbnail)
//
//  Arguments:
//      i8Timestamp
//          [I] Compressed image timestamp
//
//      u4BitstreamSize
//          [I] Bitstream buffer size in bytes.
//
//      puBitstreamBuf
//          [I] Bitstream buffer with its size = u4BitstreamSize
//
//      u4HeaderSize
//          [I] Header size in bytes; header may be jpeg exif.
//
//      puHeaderBuf
//          [I] Header buffer with its size = u4HeaderSize
//
//      u4CallbackIndex
//          [I] Callback index. 0 by default.
//              If more than one compressed callback must be invoked during
//              captures, for example burst shot & ev shot, this value is
//              the callback index; and 0 indicates the first one callback.
//
//      fgIsFinalImage
//          [I] booliean value to indicate whether it is the final image.
//              true if this is the final image callback; otherwise false.
//              For single captures, this value must be true.
//
bool
CamAdapter::
onCB_CompressedImage(
    int64_t const   i8Timestamp,
    uint32_t const  u4BitstreamSize,
    uint8_t const*  puBitstreamBuf,
    uint32_t const  u4HeaderSize,
    uint8_t const*  puHeaderBuf,
    uint32_t const  u4CallbackIndex,
    bool            fgIsFinalImage,
    uint32_t const  msgType
)
{
    MY_LOGD(
        "timestamp(%lld), bitstream:size/buf=%d/%p, header:size/buf=%d/%p, index(%d), IsFinalImage(%d)", 
        i8Timestamp, u4BitstreamSize, puBitstreamBuf, u4HeaderSize, puHeaderBuf, u4CallbackIndex, fgIsFinalImage
    );
    //
    if  ( ! msgTypeEnabled(CAMERA_MSG_COMPRESSED_IMAGE) )
    {
        MY_LOGW("msgTypeEnabled=%#x", msgTypeEnabled(0xFFFFFFFF));
        return  false;
    }
    //
    camera_memory* image = NULL;
    uint8_t* pImage = NULL;
    //
#if 1   //defined(MTK_CAMERA_BSP_SUPPORT)
    uint32_t const u4DataSize = u4HeaderSize + u4BitstreamSize + sizeof(uint32_t)*(1+1);
    image = mpCamMsgCbInfo->mRequestMemory(-1, u4DataSize, 1, NULL);
    if  ( image )
    {
        uint32_t*const pCBData = reinterpret_cast<uint32_t*>(image->data);
        pCBData[0] = msgType;
        pCBData[1] = u4CallbackIndex;
        pImage = reinterpret_cast<uint8_t*>(&pCBData[2]);
    }
#else
    image = mpCamMsgCbInfo->mRequestMemory(-1, u4HeaderSize + u4BitstreamSize, 1, NULL);
    if  ( image )
    {
        pImage = reinterpret_cast<uint8_t*>(image->data);
    }
#endif
    if  ( ! image )
    {
        MY_LOGW("mRequestMemory fail");
        return  false;
    }
    //
    if  ( image )
    {
        if  ( 0 != u4HeaderSize && 0 != puHeaderBuf )
        {
            ::memcpy(pImage, puHeaderBuf, u4HeaderSize);
            pImage += u4HeaderSize;
        }
        if  ( 0 != u4BitstreamSize && 0 != puBitstreamBuf )
        {
            ::memcpy(pImage, puBitstreamBuf, u4BitstreamSize);
        }
        //
        sp<Thread> pThread = new ZipImageCallbackThread(mpCamMsgCbInfo, image, u4CallbackIndex, fgIsFinalImage, getParamsManager()->getShotMode());
        if  ( pThread == 0 || pThread->run() != OK )
        {
            MY_LOGW("Fail to run ZipImageCallbackThread (%p)", pThread.get());
            return  false;
        }
    }
    return  true;
}



