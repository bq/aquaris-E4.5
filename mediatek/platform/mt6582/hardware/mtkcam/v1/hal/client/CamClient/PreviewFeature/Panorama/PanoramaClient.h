
#ifndef _MTK_HAL_CAMCLIENT_PanoramaCLIENT_H_
#define _MTK_HAL_CAMCLIENT_PanoramaCLIENT_H_
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
#include <mtkcam/featureio/autorama_hal_base.h>
#include "inc/IFeatureClient.h"
#include "mtkcam/hal/aaa_hal_base.h"
#include "mtkcam/hal/sensor_hal.h"
#include <math.h>
using namespace android;
using namespace MtkCamUtils;
using namespace NS3A;
//
namespace android {
namespace NSCamClient {

/******************************************************************************
 *  Preview Client Handler.
 ******************************************************************************/
class PanoramaClient : public IFeatureClient
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////                    Instantiation.
    //
    PanoramaClient(int ShotNum);
    virtual    ~PanoramaClient();

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
//  PanoramaClient.Scenario function
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:
    virtual MINT32  ISShot(MVOID * bufadr, MVOID *arg1, MBOOL &shot);
    virtual MINT32  CreateMotionSrc(MVOID * srcbufadr, int ImgWidth, int ImgHeight, MVOID * dstbufadr);   
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  PanoramaClinet function
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:
    virtual MINT32    mHalCamFeatureCompress();
    virtual MINT32    mHalCamFeatureMerge();
    virtual MINT32    mHalCamFeatureAddImg();    
    virtual MBOOL     allocMem(IMEM_BUF_INFO &memBuf);
    virtual MBOOL     deallocMem(IMEM_BUF_INFO &memBuf);

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Thread
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:     
    static MVOID*     PanoramathreadFunc(void *arg); 
    pthread_t  PanoramaFuncThread;
    sem_t      PanoramaSemThread;
    sem_t      PanoramamergeDone;
    MBOOL      mCancel;
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Image Buffer
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected: 
    IMemDrv*          mpIMemDrv;  
    IMEM_BUF_INFO     mpframeBuffer;
    IMEM_BUF_INFO     mpMotionBuffer;
    IMEM_BUF_INFO     mpPanoramaWorkingBuf;      
    int               mPanoramaFrameWidth;
    int               mPanoramaFrameHeight;
    int               mPanoramaFrameSize;
    ImgDataCallback_t mDataCb;
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Parameter
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++    
private:	           
    int32_t 	        PanoramaNum;
    halAUTORAMABase* 	mpPanoramaObj;
    Hal3ABase*          mpHal3A;
    int32_t 	      	mPanoramaFrameIdx;
    int32_t			      mPanoramaaddImgIdx; 
    MTKPIPEAUTORAMA_DIRECTION_ENUM mStitchDir;   
    int32_t 		      mJPGFrameAddr;
    MTKPipeAutoramaResultInfo mpPanoramaResult;
    uint8_t  	      	SaveFileName[64];
    mutable Mutex     mLock;
	  mutable Mutex 	  mLockUninit;
	  
	  
};
}; // namespace NSCamClient
}; // namespace android
#endif  //_MTK_HAL_CAMCLIENT_PREVIEW_PREVIEWCLIENT_H_



