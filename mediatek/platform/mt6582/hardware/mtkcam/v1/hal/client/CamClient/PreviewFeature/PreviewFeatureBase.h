
#ifndef _MTK_HAL_CAMCLIENT_PREVIEWBASE_H_
#define _MTK_HAL_CAMCLIENT_PREVIEWBASE_H_
//
#include <CamUtils.h>
#include <utils/String8.h>
using namespace android;
using namespace MtkCamUtils;
#include <mtkcam/v1/IParamsManager.h>
#include <mtkcam/v1/client/IPreviewFeatureClient.h>
//
#include <system/camera.h>
#include <mtkcam/drv/imem_drv.h>
#include "inc/IFeatureClient.h"
#include "MpoEncoder.h"

using namespace NSCamHW;

//
namespace android {
namespace NSCamClient {
namespace NSPREFEATUREABSE {
/******************************************************************************
 *
 ******************************************************************************/
class ImgBufManager;

/******************************************************************************
 *  Preview Client Handler.
 ******************************************************************************/
class PREFEATUREABSE : public IPREFEATUREClient
                     , public Thread
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////                    Instantiation.
    //
                     PREFEATUREABSE(sp<IParamsManager> pParamsMgr);
    virtual          ~PREFEATUREABSE();

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////

    virtual bool     init();
    virtual bool     uninit();

    virtual bool     setImgBufProviderClient(
                         sp<IImgBufProviderClient>const& rpClient
                     );
    //
    //
    virtual void     setCallbacks(sp<CamMsgCbInfo> const& rpCamMsgCbInfo);
    //
    //
    virtual status_t sendCommand(int32_t cmd, int32_t arg1, int32_t arg2);
    virtual bool     startMAV(int32_t ShotNum);
    virtual bool     stopFeature(int32_t Cancel);
    virtual bool     startPanorama(int32_t ShotNum);
#ifdef MTK_MOTION_TRACK_SUPPORTED
    virtual bool     startMotionTrack(int32_t ShotNum);
#endif
    virtual bool     stopPreview();

    //
    //
    virtual void     enableMsgType(int32_t msgType);
    virtual void     disableMsgType(int32_t msgType);
    static  MBOOL    handlePanoImgCallBack(MVOID* const puJpegBuf, int u4SrcWidth, int u4SrcHeight);
    static  MBOOL    handleMAVImgCallBack(MVOID* const puJpegBuf, int u4SrcWidth, int u4SrcHeight);
#ifdef MTK_MOTION_TRACK_SUPPORTED
    static  MBOOL    handleMotionTrackImgCallBack(MVOID* const puJpegBuf, int u4SrcWidth, int u4SrcHeight);
#endif
    virtual MBOOL    createMPO(MPImageInfo * pMPImageInfo, MUINT32 num, char* file, MUINT32 MPOType);
    virtual MBOOL    createMPOInMemory(MPImageInfo * pMPImageInfo, MUINT32 num, MUINT32 MPOType, MUINT8* mpoBuffer);
    virtual MBOOL    queryMpoSize(MPImageInfo * pMPImageInfo, MUINT32 num, MUINT32 MPOType, MUINT32 &mpoSize);

    virtual MBOOL    createPanoJpegImg(IMEM_BUF_INFO Srcbufinfo, int u4SrcWidth, int u4SrcHeight, IMEM_BUF_INFO jpgBuff, MUINT32 &u4JpegSize);
    virtual MBOOL    createJpegImgWithThumbnail(NSCamHW::ImgBufInfo const &rYuvImgBufInfo, IMEM_BUF_INFO jpgBuff, MUINT32 &u4JpegSize);
    virtual MBOOL    createJpegImg(NSCamHW::ImgBufInfo const & rSrcBufInfo
                     , MUINT32 quality
                     , bool fIsAddSOI
                     , NSCamHW::ImgBufInfo const & rDstBufInfo
                     , MUINT32 & u4EncSize);
    virtual MBOOL    makeExifHeader(MUINT32 const u4CamMode,
    			           MUINT8* const puThumbBuf,
				             MUINT32 const u4ThumbSize,
				             MUINT8* puExifBuf,
				             MUINT32 &u4FinalExifSize,
				             MUINT32 const Width,
				             MUINT32 const Height,
				             MUINT32 u4ImgIndex = 0,
				             MUINT32 u4GroupId = 0);
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Operations in base class Thread
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////
    // Ask this object's thread to exit. This function is asynchronous, when the
    // function returns the thread might still be running. Of course, this
    // function can be called from a different thread.
    virtual void     requestStop();
    virtual void     requestExit();
    // Good place to do one-time initializations
    virtual status_t readyToRun();

private:
    // Derived class must implement threadLoop(). The thread starts its life
    // here. There are two ways of using the Thread object:
    // 1) loop: if threadLoop() returns true, it will be called again if
    //          requestExit() wasn't called.
    // 2) once: if threadLoop() returns false, the thread will exit upon return.
    virtual bool     threadLoop();

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Capture image info.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:
    int                mShotNum;
    int                bufCnt;
    int                bufWidth;
    int                bufHeight;
    PreFeatureObject_e mobject;
    int32_t            isDoMerge;
    String8            filename;
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Command Queue.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////     Definitions.
    struct Command
    {
        //  Command ID.
        enum EID
        {
            eID_UNKNOWN,
            eID_STOP,
            eID_EXIT,
            eID_WAKEUP,
        };
    };

protected:  ////                    Operations.

    virtual void                    postCommand(Command::EID cmd);
    virtual bool                    getCommand(Command::EID &cmd);
    //
    virtual void                    onClientThreadLoop();
    //
    inline  int32_t                 getThreadId() const    { return mi4ThreadId; }

protected:  ////                    Data Members.
    List<Command::EID>              mCmdQue;
    Mutex                           mCmdQueMtx;
    Condition                       mCmdQueCond;    //  Condition to wait: [ ! exitPending() && mCmdQue.empty() ]
    int32_t                         mi4ThreadId;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////

    bool                            isEnabledState();
    bool                            isMsgEnabled();
    bool                            onStateChanged();
    bool                            performCallback(int32_t mvX, int32_t mvY, int32_t mStitchDir, MBOOL isShot, MBOOL isSound);
    bool                            captureDoneCallback(int32_t message, int32_t id, int32_t bufferAddr, int32_t bufferSize);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
protected:  ////                    Common Info.

    mutable Mutex                   mModuleMtx;
    sp<IParamsManager>              mpParamsMgr;            //  Reference to Parameters Manager.
    sp<CamMsgCbInfo>                mpCamMsgCbInfo;
    MtkCameraParameters             mrParameters;           //  Get cam parameters
    volatile int32_t                mIsMsgEnabled;          //  Message Enabled ?
    volatile int32_t                mIsFeatureStarted;      //  Frame      captured ?

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
protected:  ////                    Callback.
    //
    int32_t                         mi4CallbackRefCount;    //  Preview callback reference count.
    int64_t                         mi8CallbackTimeInMs;    //  The timestamp in millisecond of last preview callback.
    //
    void*                           mClientCbCookie;
    camera_notify_callback          mNotifyCb;
    camera_data_callback            mDataCb;
    camera_data_timestamp_callback  mDataCbTimestamp;
    camera_request_memory           mfpRequestMemory;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Image Buffer
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:
    //
    bool                            initBuffers(sp<IImgBufQueue>const &rpBufQueue);
    void                            uninitBuffers();
    //
    bool                            createBuffers();
    bool                            createWorkingBuffers(sp<IImgBufQueue>const & rpBufQueue);
    void                            destroyBuffers();
    void                            destroyWorkingBuffers();
    //
    bool                            waitAndHandleReturnBuffers(sp<IImgBufQueue>const& rpBufQueue, ImgBufQueNode &rQueNode);
    bool                            handleReturnBuffers(sp<IImgBufQueue>const& rpBufQueue, ImgBufQueNode const &rQueNode);
    //
protected:
    //
    sp<IImgBufQueue>                mpImgBufQueue;
    sp<IImgBufProviderClient>       mpImgBufPvdrClient;

private:
    //
    sp<IFeatureClient>  FeatureClient;

};
}; // namespace NSPREFEATUREABSE
}; // namespace NSCamClient
}; // namespace android
#endif  //_MTK_HAL_CAMCLIENT_PREVIEW_PREVIEWCLIENT_H_



