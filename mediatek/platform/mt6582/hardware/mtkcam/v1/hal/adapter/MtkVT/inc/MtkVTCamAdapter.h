
#ifndef _MTK_HAL_CAMADAPTER_MTKVT_INC_MTKVTCAMADAPTER_H_
#define _MTK_HAL_CAMADAPTER_MTKVT_INC_MTKVTCAMADAPTER_H_
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
#include <vector>
using namespace std;
//
namespace android {
namespace NSMtkVTCamAdapter {


class CamAdapter : public BaseCamAdapter
                 , public IStateHandler
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
    virtual status_t                startRecording()        { return INVALID_OPERATION; }

    /**
     * Stop a previously started recording.
     */
    virtual void                    stopRecording()         {}

    /**
     * Returns true if recording is enabled.
     */
    virtual bool                    recordingEnabled() const{ return false; }

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
    virtual bool                    isTakingPicture() const { return false; }

    /**
     * Take a picture.
     */
    virtual status_t                takePicture()           { return INVALID_OPERATION; }

    /**
     * Cancel a picture that was started with takePicture.  Calling this
     * method when no picture is being taken is a no-op.
     */
    virtual status_t                cancelPicture()         { return OK; }

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

private:    ////                    3A
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
protected:  ////                    Data Members.
    //
    IStateManager*                  mpStateManager;
    //
    sp<IPreviewCmdQueThread>        mpPreviewCmdQueThread; 
    sp<IPreviewBufMgr>              mpPreviewBufMgr;
    //
    ResourceLock*                   mpResourceLock;
    //
};


};  // namespace NSMtkVTCamAdapter
};  // namespace android
#endif  //_MTK_HAL_CAMADAPTER_MTKVT_INC_MTKVTCAMADAPTER_H_



