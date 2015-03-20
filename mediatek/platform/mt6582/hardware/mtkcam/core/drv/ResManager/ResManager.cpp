
#define LOG_TAG "MtkCam/ResManager"
//
#include <cutils/atomic.h>
#include <utils/threads.h>
#include <mtkcam/Log.h>
#include <mtkcam/common.h>
#include <mtkcam/hal/IResManager.h>
//
#include <mtkcam/drv/res_mgr_drv.h>
#include <mtkcam/campipe/pipe_mgr_drv.h>
//
using namespace android;
/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...)        CAM_LOGV("[%s] "fmt, __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_LOGD("[%s] "fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_LOGI("[%s] "fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_LOGW("[%s] "fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_LOGE("[%s] "fmt, __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_LOGA("[%s] "fmt, __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_LOGF("[%s] "fmt, __FUNCTION__, ##arg)
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
 ******************************************************************************/
class ResManager : public IResManager
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////        Operations.

    virtual MBOOL       open(char const* szCallerName);
    virtual MBOOL       close(char const* szCallerName);

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Implementations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////        Instantiation.

    virtual             ~ResManager() {}
                        ResManager();

protected:  ////        Operations.
    virtual MBOOL       init(void);
    virtual MBOOL       uninit(void);

protected:  ////        Data Members.

    PipeMgrDrv*         mpPipeMgr;
    ResMgrDrv*          mpResMgr;
    mutable Mutex       mLock;
    volatile MINT32     mUser;

};


/******************************************************************************
 *
 ******************************************************************************/
static ResManager gResManager;
IResManager*
IResManager::
getInstance()
{
    return  &gResManager;
}


/******************************************************************************
 *
 ******************************************************************************/
ResManager::
ResManager()
    : mpPipeMgr(NULL)
    , mpResMgr(NULL)
    , mUser(0)
{
}


/******************************************************************************
 *
 ******************************************************************************/
MBOOL
ResManager::
open(char const* szCallerName)
{
    MBOOL ret = MTRUE;
    //
    MY_LOGD("%s",szCallerName);
    //
    if(init())
    {
        RES_MGR_DRV_MODE_STRUCT Mode;
        Mode.Dev = RES_MGR_DRV_DEV_CAM;
        Mode.ScenSw = RES_MGR_DRV_SCEN_SW_CAM_IDLE;
        Mode.ScenHw = RES_MGR_DRV_SCEN_HW_NONE;
        if(!(mpResMgr->SetMode(&Mode)))
        {
            MY_LOGE("Set mode fail");
            ret = MFALSE;
        }
    }
    else
    {
        ret = MFALSE;
    }
    return  ret;
}


/******************************************************************************
 *
 ******************************************************************************/
MBOOL
ResManager::
close(char const* szCallerName)
{
    MBOOL ret = MTRUE;
    //
    MY_LOGD("%s",szCallerName);
    //
    if(mUser <= 0)
    {
        MY_LOGW("No user");
        ret = MTRUE;
        goto EXIT;
    }
    //
    RES_MGR_DRV_MODE_STRUCT Mode;
    Mode.Dev = RES_MGR_DRV_DEV_CAM;
    Mode.ScenSw = RES_MGR_DRV_SCEN_SW_NONE;
    Mode.ScenHw = RES_MGR_DRV_SCEN_HW_NONE;
    if(!(mpResMgr->SetMode(&Mode)))
    {
        MY_LOGE("Set mode fail");
        ret = MFALSE;
        goto EXIT;
    }
    //
    if(!uninit())
    {
        MY_LOGE("uninit fail");
        ret = MFALSE;
    }
    //
    EXIT:
    return  ret;
}


/******************************************************************************
 *
 ******************************************************************************/
MBOOL
ResManager::
init(void)
{
    MBOOL Result = MTRUE;
    //
    Mutex::Autolock lock(mLock);
    //
    if(mUser == 0)
    {
        MY_LOGD("First user(%d)",mUser);
    }
    else
    {
        MY_LOGD("More user(%d)",mUser);
        android_atomic_inc(&mUser);
        goto EXIT;
    }
    //
    if(mpResMgr == NULL)
    {
        mpResMgr = ResMgrDrv::CreateInstance();
        if(mpResMgr != NULL)
        {
           mpResMgr->Init();
        }
        else
        {
            MY_LOGE("mpResMgr is NULL");
            Result = MFALSE;
            goto EXIT;
        }
    }
    else
    {
        MY_LOGD("ResMgr is created already");
    }
    //
    android_atomic_inc(&mUser);
    //
    EXIT:
    return Result;
}



/******************************************************************************
 *
 ******************************************************************************/
MBOOL
ResManager::
uninit(void)
{
    MBOOL Result = MTRUE;
    //
    Mutex::Autolock lock(mLock);
    //
    if(mUser <= 0)
    {
        MY_LOGW("No user(%d)",mUser);
        goto EXIT;
    }
    //
    android_atomic_dec(&mUser);
    //
    if(mUser == 0)
    {
        MY_LOGD("Last user(%d)",mUser);
    }
    else
    {
        MY_LOGD("More user(%d)",mUser);
        goto EXIT;
    }
    //
    if(mpResMgr != NULL)
    {
        mpResMgr->Uninit();
        mpResMgr->DestroyInstance();
        mpResMgr = NULL;
    }
    //
    EXIT:
    return Result;
}





