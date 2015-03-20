
#define LOG_TAG "MtkCam/CamAdapter"
//
#include <inc/CamUtils.h>
using namespace android;
using namespace MtkCamUtils;
//
#include <camera/MtkCamera.h>
//
#include <inc/ImgBufProvidersManager.h>
//
#include <mtkcam/hal/sensor_hal.h>
//
#include <mtkcam/v1/ICamAdapter.h>
#include <inc/BaseCamAdapter.h>
#include <inc/MtkPhotoCamAdapter.h>
using namespace NSMtkPhotoCamAdapter;
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
namespace {

class Cam3ACallBack : public I3ACallBack
{

private:
    sp<CamMsgCbInfo>    mspCamMsgCbInfo; 
    mutable Mutex       mLock;
    bool                mAFMoveCb;

//Inherited from I3ACallBack
public:

    //  Notify Callback of 3A
    //  Arguments:
    //
    //      _msgType: 
    //          Defined at aaa_hal_base.h
    //
    //      _ext1:
    //          For eID_NOTIFY_AF_FOCUSED, it is Focused or not
    //          For eID_NOTIFY_AF_MOVING, it is Moving or not
    //
    //      _ext2 and _ext3: 
    //          Undefined.
    //

    virtual void        doNotifyCb (
                            int32_t _msgType, 
                            int32_t _ext1, 
                            int32_t _ext2,
                            int32_t _ext3
                        );

    //  Data Callback of 3A
    //  Arguments:
    //
    //      _msgType: 
    //          Defined at aaa_hal_base.h
    //  
    //      _data:
    //          pointer to data
    // 
    //      _size:
    //          size of data    
    virtual void        doDataCb (
                            int32_t _msgType,
                            void*   _data, 
                            uint32_t _size
                        );
    
// class member
public:
    
    virtual             ~Cam3ACallBack(){}
                        Cam3ACallBack()
                            : mspCamMsgCbInfo(0)
                            , mLock()
                        {}
    
    virtual void        setUser(sp<CamMsgCbInfo> const &rpCamMsgCbInfo) 
                        { 
                            Mutex::Autolock lock(mLock);
                            mspCamMsgCbInfo = rpCamMsgCbInfo; 
                        }       
    
    virtual void        destroyUser()        
                        {
                            Mutex::Autolock lock(mLock);
                            if ( mspCamMsgCbInfo != 0 )
                            {
                                mspCamMsgCbInfo = 0;
                            }
                        }
    virtual void        enableAFMove(bool flag)
                        {
                            Mutex::Autolock lock(mLock);
                            mAFMoveCb = flag;
                        }
};

/******************************************************************************
* 
*******************************************************************************/
void
Cam3ACallBack::
doNotifyCb(
    int32_t _msgType, 
    int32_t _ext1, 
    int32_t _ext2,
    int32_t _ext3
)
{
    CAM_LOGD("msgType(%d): +", _msgType);

    Mutex::Autolock lock(mLock);

    if( mspCamMsgCbInfo == 0)
    {
        CAM_LOGW("mspCamMsgCbInfo == 0");
        return;
    }

    if ( _msgType == I3ACallBack::eID_NOTIFY_AF_FOCUSED && 
         (mspCamMsgCbInfo->mMsgEnabled & (CAMERA_MSG_FOCUS))
       )
    {
        mspCamMsgCbInfo->mNotifyCb(CAMERA_MSG_FOCUS, _ext1, _ext2, mspCamMsgCbInfo->mCbCookie);
    }

    if ( _msgType == I3ACallBack::eID_NOTIFY_AF_MOVING && 
         (mspCamMsgCbInfo->mMsgEnabled & CAMERA_MSG_FOCUS_MOVE) &&
         mAFMoveCb
       )
    {
        mspCamMsgCbInfo->mNotifyCb(CAMERA_MSG_FOCUS_MOVE, _ext1, _ext2, mspCamMsgCbInfo->mCbCookie);        
    } 

    CAM_LOGD("-");
}


/******************************************************************************
* 
*******************************************************************************/
void
Cam3ACallBack::
doDataCb(
    int32_t  _msgType,
    void*    _data, 
    uint32_t  _size
)
{
    CAM_LOGD("msgType(%d): +", _msgType);
    
    Mutex::Autolock lock(mLock);

    if( mspCamMsgCbInfo == 0)
    {
        CAM_LOGW("mspCamMsgCbInfo == 0");
        return;
    }
    
    if ( _msgType == I3ACallBack::eID_DATA_AF_FOCUSED)
    {
        camera_memory* mem = mspCamMsgCbInfo->mRequestMemory(-1, _size + sizeof(int32_t), 1, NULL);
        if  ( mem )
        {
            if (mem->data && mem->size >= _size)
            {
                int32_t* pSrc = reinterpret_cast<int32_t*>(_data);
                int32_t* pDst = reinterpret_cast<int32_t*>(mem->data);
    
                pDst[0] = MTK_CAMERA_MSG_EXT_DATA_AF;
                for (uint32_t i = 0; i < _size/4; i++)
                {
                    pDst[i+1] = pSrc[i];
                }

                mspCamMsgCbInfo->mDataCb(MTK_CAMERA_MSG_EXT_DATA, mem, 0, NULL, mspCamMsgCbInfo->mCbCookie);
                mem->release(mem);
            }
        }
    }
    else 
    {
        CAM_LOGW("undefined");
    }
    
    CAM_LOGD("-");
}

static Cam3ACallBack g3ACallback;

}; // end of namespace


/******************************************************************************
* 
*******************************************************************************/
status_t
CamAdapter::
init3A()
{
    MY_LOGD("+");
    //
    g3ACallback.setUser(mpCamMsgCbInfo);
    //
    status_t ret = OK;
    Hal3ABase* p3AHal = Hal3ABase::createInstance(DevMetaInfo::queryHalSensorDev(getOpenId())); 

    if ( ! p3AHal )
    {
        MY_LOGE("p3AHal == NULL");
        return INVALID_OPERATION;
    }
    
    if ( ! p3AHal->setCallbacks(&g3ACallback) )
    {
        MY_LOGE("setCallbacks fail");
        ret = INVALID_OPERATION;
        goto lbExit;
    }

lbExit:
    
    p3AHal->destroyInstance();
    MY_LOGD("-");
    return ret;
}


/******************************************************************************
* 
*******************************************************************************/
void
CamAdapter::
uninit3A()
{
    MY_LOGD("");
    g3ACallback.destroyUser();
}


/******************************************************************************
*  This method is only valid when preview is active
*******************************************************************************/
status_t
CamAdapter::
autoFocus()          
{
    MY_LOGD("+");
    //
    status_t ret = OK;
    Hal3ABase* p3AHal = Hal3ABase::createInstance(DevMetaInfo::queryHalSensorDev(getOpenId())); 

    if ( ! p3AHal )
    {
        MY_LOGE("p3AHal == NULL");
        return INVALID_OPERATION;
    }

    if ( ! p3AHal->autoFocus() )
    {
        MY_LOGE("autoFocus fail");
        ret = INVALID_OPERATION;
        goto lbExit;
    }

lbExit:
    
    p3AHal->destroyInstance();
    MY_LOGD("-");
    return ret;
}


/******************************************************************************
* Cancels any auto-focus function in progress.
* Whether or not auto-focus is currently in progress
*******************************************************************************/
status_t
CamAdapter::
cancelAutoFocus()
{
    MY_LOGD("+");
    status_t ret = OK;    
    //
    Hal3ABase* p3AHal = Hal3ABase::createInstance(DevMetaInfo::queryHalSensorDev(getOpenId())); 

    if ( ! p3AHal )
    {
        MY_LOGE("p3AHal == NULL");
        return INVALID_OPERATION;
    }

    if ( ! p3AHal->cancelAutoFocus() )
    {
        MY_LOGE("cancelAutoFocus fail");
        // do not return error since 3a may not have received autofocus
        //ret = INVALID_OPERATION;
        //goto lbExit;
    }

lbExit:
    
    p3AHal->destroyInstance();
    MY_LOGD("-");
    return ret;

}


/******************************************************************************
*
*
*******************************************************************************/
void
CamAdapter::enableAFMove(bool flag)
{
    g3ACallback.enableAFMove(flag);
}





