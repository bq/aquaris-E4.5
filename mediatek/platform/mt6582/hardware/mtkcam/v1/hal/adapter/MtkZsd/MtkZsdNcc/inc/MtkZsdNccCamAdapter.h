
#ifndef _MTK_HAL_CAMADAPTER_MTKZSDNCC_INC_MTKZSDNCCCAMADAPTER_H_
#define _MTK_HAL_CAMADAPTER_MTKZSDNCC_INC_MTKZSDNCCCAMADAPTER_H_
//
/*******************************************************************************
*
*******************************************************************************/
#include "inc/IState.h"
//
//
#include <inc/ResourceLock.h>
//
#include <mtkcam/hal/aaa_hal_base.h>
using namespace NS3A;
//
#include <mtkcam/v1/hwscenario/IhwScenarioType.h>
using namespace NSHwScenario;
#include "inc/PreviewCmdQueThread.h"
#include <inc/IPreviewBufMgr.h>
//
#include <inc/ICaptureBufMgr.h>
#include "inc/CaptureCmdQueThread.h"
#include <Scenario/Shot/IShot.h>
using namespace NSShot;
//
#include <vector>
using namespace std;
//
namespace android {
namespace NSMtkZsdNccCamAdapter {


class CamAdapter : public BaseCamAdapter
                 , public IStateHandler
                 , public ICaptureCmdQueThreadHandler
                 , public IShotCallback
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  ICamAdapter Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////

    /**
     * Initialize the device resources owned by this object.
     */
    virtual bool                    init();

    /**
     * Uninitialize the device resources owned by this object. Note that this is
     * *not* done in the destructor.
     */
    virtual bool                    uninit();

    /**
     * Start preview mode.
     */
    virtual status_t                startPreview();

    /**
     * Stop a previously started preview.
     */
    virtual void                    stopPreview();

    /**
     * Returns true if preview is enabled.
     */
    virtual bool                    previewEnabled() const;

    /**
     * Start record mode. When a record image is available a CAMERA_MSG_VIDEO_FRAME
     * message is sent with the corresponding frame. Every record frame must be released
     * by a cameral hal client via releaseRecordingFrame() before the client calls
     * disableMsgType(CAMERA_MSG_VIDEO_FRAME). After the client calls
     * disableMsgType(CAMERA_MSG_VIDEO_FRAME), it is camera hal's responsibility
     * to manage the life-cycle of the video recording frames, and the client must
     * not modify/access any video recording frames.
     */
    virtual status_t                startRecording()            { return INVALID_OPERATION; }

    /**
     * Stop a previously started recording.
     */
    virtual void                    stopRecording()             {}

    /**
     * Returns true if recording is enabled.
     */
    virtual bool                    recordingEnabled() const    { return false; }

    /**
     * Start auto focus, the notification callback routine is called
     * with CAMERA_MSG_FOCUS once when focusing is complete. autoFocus()
     * will be called again if another auto focus is needed.
     */
    virtual status_t                autoFocus(); 

    /**
     * Cancels auto-focus function. If the auto-focus is still in progress,
     * this function will cancel it. Whether the auto-focus is in progress
     * or not, this function will return the focus position to the default.
     * If the camera does not support auto-focus, this is a no-op.
     */
    virtual status_t                cancelAutoFocus();        

    /**
     * Returns true if capture is on-going.
     */
    virtual bool                    isTakingPicture() const;

    /**
     * Take a picture.
     */
    virtual status_t                takePicture();

    /**
     * Cancel a picture that was started with takePicture.  Calling this
     * method when no picture is being taken is a no-op.
     */
    virtual status_t                cancelPicture();
	
    /**
     * set continuous shot speed
     */
    virtual status_t                setCShotSpeed(int32_t i4CShotSpeeed);

    /**
     * Set the camera parameters. This returns BAD_VALUE if any parameter is
     * invalid or not supported.
     */
    virtual status_t                setParameters();

    /**
     * Send command to camera driver.
     */
    virtual status_t                sendCommand(int32_t cmd, int32_t arg1, int32_t arg2);

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  IStateHandler Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////        
    virtual status_t                onHandleStartPreview();
    virtual status_t                onHandleStopPreview();
    //
    virtual status_t                onHandlePreCapture();
    virtual status_t                onHandleCapture();
    virtual status_t                onHandleCaptureDone();
    virtual status_t                onHandleCancelCapture();

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  ICaptureCmdQueThreadHandler Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////
    virtual bool                    onCaptureThreadLoop();

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  IShotCallback Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////
    //  Directly include this inline file to reduce file dependencies since the 
    //  interfaces in this file may often vary.
    #include "inc/ImpShotCallback.inl"

private:

    status_t                        init3A();
    void                            uninit3A();
    void                            enableAFMove(bool flag);
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////                    Instantiation.

    virtual                         ~CamAdapter();
                                    CamAdapter(
                                        String8 const&      rName, 
                                        int32_t const       i4OpenId, 
                                        sp<IParamsManager>  pParamsMgr
                                    );

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Implementation.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////

    bool                            updateShotInstance();

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Implementation.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////                    Data Members.
    //
    IStateManager*                  mpStateManager;
    //
    sp<IPreviewCmdQueThread>        mpPreviewCmdQueThread; 
    sp<IPreviewBufMgr>              mpPreviewBufMgr;
    //
    sp<ICaptureBufMgr>              mpCaptureBufMgr;
    //
    sp<ICaptureCmdQueThread>        mpCaptureCmdQueThread;
    sp<IShot>                       mpShot;
    //
    ResourceLock*                   mpResourceLock;
    // zsd added:
    uint32_t                        mu4ShotMode;
};


};  // namespace NSMtkPhotoCamAdapter
};  // namespace android
#endif  //_MTK_HAL_CAMADAPTER_MTKZSDNCC_INC_MTKZSDNCCCAMADAPTER_H_



