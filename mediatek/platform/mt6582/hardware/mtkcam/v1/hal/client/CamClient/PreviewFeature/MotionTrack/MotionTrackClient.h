
#ifndef _MTK_HAL_CAMCLIENT_MotionTrackCLIENT_H_
#define _MTK_HAL_CAMCLIENT_MotionTrackCLIENT_H_
//
#include <CamUtils.h>
#include <system/camera.h>
#include <mtkcam/drv/imem_drv.h>
#include <pthread.h>
#include <semaphore.h>
#include <cutils/properties.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include "mtkcam/common.h"
#include <mtkcam/featureio/motiontrack_hal_base.h>
#include "inc/IFeatureClient.h"
#include "mtkcam/hal/aaa_hal_base.h"
#include <mtkcam/hal/sensor_hal.h>
#include <mtkcam/featureio/eis_hal_base.h>
using namespace android;
using namespace MtkCamUtils;
using namespace NS3A;
//
namespace android {
namespace NSCamClient {

/******************************************************************************
 *  Preview Client Handler.
 ******************************************************************************/
class MotionTrackClient : public IFeatureClient
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////                    Instantiation.
    //
    MotionTrackClient(int ShotNum);
    MotionTrackClient(int ShotNum, int32_t previewFrameRate);
    virtual    ~MotionTrackClient();

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////
    virtual bool    init(int bufwidth,int bufheight);
    virtual bool    uninit();
    virtual MINT32  mHalCamFeatureProc(MVOID * bufadr, int32_t& mvX, int32_t& mvY, int32_t& dir, MBOOL& isShot);
    virtual bool    stopFeature(int cancel);
    virtual MVOID   setImgCallback(ImgDataCallback_t data_cb);
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  MotionTrackClient.Scenario function
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:
    virtual MINT32  CreateThumbImage(MVOID * srcbufadr, int ImgWidth, int ImgHeight, MVOID * dstbufadr);
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  MotionTrackClinet function
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:
    virtual MINT32    mHalCamFeatureCompress();
    virtual MINT32    mHalCamFeatureBlend();
    virtual MBOOL     allocMem(IMEM_BUF_INFO &memBuf);
    virtual MBOOL     deallocMem(IMEM_BUF_INFO &memBuf);
    virtual MVOID     updateEISMethod(MUINT16 method);

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Thread
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:     
    static MVOID*     MotionTrackthreadFunc(void *arg); 
    pthread_t  MotionTrackFuncThread;
    sem_t      MotionTrackAddImgDone;
    sem_t      MotionTrackBlendDone;
    MBOOL      mCancel;
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Image Buffer
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected: 
    IMemDrv*          mpIMemDrv;  
    IMEM_BUF_INFO     mpFrameBuffer;
    IMEM_BUF_INFO     mpThumbBuffer;
    IMEM_BUF_INFO     mpMotionTrackWorkingBuf;      
    IMEM_BUF_INFO     mpBlendBuffer;      
	MUINT32           mNumBlendImages;
    int               mMotionTrackFrameWidth;
    int               mMotionTrackFrameHeight;
    int               mMotionTrackFrameSize;
    int               mMotionTrackThumbSize;
    MUINT16           mSensorRawWidth;
    MUINT16           mSensorRawHeight;
    MUINT16           mMotionTrackOutputWidth;
    MUINT16           mMotionTrackOutputHeight;
    ImgDataCallback_t mDataCb;
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Parameter
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++    
private:	           
    int32_t 	        MotionTrackNum;
    halMOTIONTRACKBase* mpMotionTrackObj;
    int32_t 	      	mMotionTrackFrameIdx;
    int32_t			      mMotionTrackaddImgIdx; 
    int32_t 		      mJPGFrameAddr;
    uint8_t  	      	SaveFileName[64];
    int32_t             mPreviewFrameCount;
    SensorHal*          mpSensor;
    EisHalBase*         mpEisHal;
    mutable Mutex     mLock;
	  mutable Mutex 	  mLockUninit;
    MUINT32             mTimestamp;
    MUINT32             mTimelapse;
    MINT32              mMvX;
    MINT32              mMvY;
    MUINT16             mPreviewCropWidth;
    MUINT16             mPreviewCropHeight;
	  
	  
};
}; // namespace NSCamClient
}; // namespace android
#endif  //_MTK_HAL_CAMCLIENT_PREVIEW_PREVIEWCLIENT_H_



