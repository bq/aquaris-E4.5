
#ifndef _MTK_CAMERA_CAMADAPTER_SCENARIO_SHOT_CONTINUOUSSHOT_H_
#define _MTK_CAMERA_CAMADAPTER_SCENARIO_SHOT_CONTINUOUSSHOT_H_

#include <utils/threads.h>
#include <semaphore.h>

namespace android {
namespace NSShot {
/******************************************************************************
 *
 ******************************************************************************/


/******************************************************************************
 *
 ******************************************************************************/
class ContinuousShot : public ImpShot
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////                    Instantiation.
    virtual                         ~ContinuousShot();
                                    ContinuousShot(
                                        char const*const pszShotName, 
                                        uint32_t const u4ShotMode, 
                                        int32_t const i4OpenId
                                    );

public:     ////                    Operations.

    //  This function is invoked when this object is firstly created.
    //  All resources can be allocated here.
    virtual bool                    onCreate();

    //  This function is invoked when this object is ready to destryoed in the
    //  destructor. All resources must be released before this returns.
    virtual void                    onDestroy();

    virtual bool                    sendCommand(
                                        uint32_t const  cmd, 
                                        uint32_t const  arg1, 
                                        uint32_t const  arg2
                                    );

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Implementations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////                    Operations.
    virtual bool                    onCmd_reset();
    virtual bool                    onCmd_capture();
    virtual bool                    onCmd_captureNcc();
    virtual bool                    onCmd_captureCc();
    virtual void                    onCmd_cancel();
	virtual bool					onCmd_setCShotSpeed(uint32_t u4CShotSpeed);
    virtual bool                    onCmd_setCaptureBufMgr(uint32_t const arg1, uint32_t const arg2);

protected:  ////                    callbacks 
    static MBOOL fgCamShotNotifyCb(MVOID* user, NSCamShot::CamShotNotifyInfo const msg);
    static MBOOL fgCamShotDataCb(MVOID* user, NSCamShot::CamShotDataInfo const msg); 

protected:
    MBOOL           handleNotifyCb(NSCamShot::CamShotNotifyInfo const msg);
    MBOOL           handlePostViewData(MUINT8* const puBuf, MUINT32 const u4Size);
    MBOOL           handleJpegData(MUINT8* const puJpegBuf, MUINT32 const u4JpegSize, MUINT8* const puThumbBuf, MUINT32 const u4ThumbSize);    

protected: 
	NSCamShot::IMultiShot* 			mpMultiShot;
	MUINT32 volatile				mu4ShotConut;
	bool volatile					mbLastImage;
	bool volatile		   			mbShotStoped;
	Mutex                  			mShotStopMtx;
    sem_t 							semMShotEnd;
	MUINT32 volatile				mu4GroupId;
    sp<ICaptureBufMgrHandler>    	mpCaptureBufMgr;
	
	MUINT32 volatile 				mu4FocusValH;
	MUINT32 volatile				mu4FocusValL;
	bool    volatile 				mbCbShutterMsg;
};


/******************************************************************************
 *
 ******************************************************************************/
}; // namespace NSShot
}; // namespace android
#endif  //  _MTK_CAMERA_CAMADAPTER_SCENARIO_SHOT_CONTINUOUSSHOT_H_



