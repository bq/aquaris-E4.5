
#define LOG_TAG "MtkATV/State"
//
#include <inc/CamUtils.h>
using namespace android;
using namespace MtkCamUtils;
//
#include <inc/IState.h>
#include "State.h"
using namespace NSMtkAtvCamAdapter;
//


/******************************************************************************
*
*******************************************************************************/
#define MY_LOGV(fmt, arg...)        CAM_LOGV("(%s)[%s] "fmt,  getName(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_LOGD("(%s)[%s] "fmt,  getName(), __FUNCTION__, ##arg)
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


/******************************************************************************
*
*******************************************************************************/
#define ENABLE_OP_LOG               (1)


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  StateBase
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


/*******************************************************************************
 *  
 ******************************************************************************/
StateBase::
StateBase(char const*const pcszName, ENState const eState)
    : IState()
    , m_pcszName(pcszName)
    , m_eState(eState)
    , m_pStateManager(IStateManager::inst())
{
}


/*******************************************************************************
 *  
 ******************************************************************************/
status_t
StateBase::
op_UnSupport(char const*const pcszDbgText)
{
    MY_LOGW("%s", pcszDbgText);
    //
    return  INVALID_OPERATION;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  StateIdle
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
StateIdle::
StateIdle(ENState const eState)
    : StateBase(__FUNCTION__, eState)
{
}


status_t
StateIdle::
onStartPreview(IStateHandler* pHandler)
{
    
    IStateManager::StateObserver stateWaiter(getStateManager());
    getStateManager()->registerOneShotObserver(&stateWaiter);
    //
    //
    MY_LOGD_IF(ENABLE_OP_LOG, "+");
    //
    status_t status = OK;
    //
    status = pHandler->onHandleStartPreview();
    if  ( OK != status ) {
        goto lbExit;
    }
    //
    status = stateWaiter.waitState(eState_Preview);
    if  ( OK != status ) {
        goto lbExit;
    }
    //
lbExit:
    MY_LOGD_IF(ENABLE_OP_LOG, "- status(%d)", status);
    return  status;
}


//[ATV]+
/*
status_t
StateIdle::
onCapture(IStateHandler* pHandler)
{
    IStateManager::StateObserver stateWaiter(getStateManager());
    getStateManager()->registerOneShotObserver(&stateWaiter);
    //
    //
    MY_LOGD_IF(ENABLE_OP_LOG, "+");
    //
    status_t status = OK;
    //
    status = pHandler->onHandleCapture();
    if  ( OK != status ) {
        goto lbExit;
    }
    //
#if 1
    status = stateWaiter.waitState(eState_Capture);
    if  ( OK != status ) {
        goto lbExit;
    }
#endif
    //
lbExit:
    MY_LOGD_IF(ENABLE_OP_LOG, "- status(%d)", status);
    return  status;
}
*/
//[ATV]-



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  StatePreview
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
StatePreview::
StatePreview(ENState const eState)
    : StateBase(__FUNCTION__, eState)
{
}


status_t
StatePreview::
onStopPreview(IStateHandler* pHandler)
{
    IStateManager::StateObserver stateWaiter(getStateManager());
    getStateManager()->registerOneShotObserver(&stateWaiter);
    //
    //
    MY_LOGD_IF(ENABLE_OP_LOG, "+");
    //
    status_t status = OK;
    //
    status = pHandler->onHandleStopPreview();
    if  ( OK != status ) {
        goto lbExit;
    }
    //
    status = stateWaiter.waitState(eState_Idle);
    if  ( OK != status ) {
        goto lbExit;
    }
    //
lbExit:
    MY_LOGD_IF(ENABLE_OP_LOG, "- status(%d)", status);
    return  status;
}


status_t
StatePreview::
onPreCapture(IStateHandler* pHandler)
{
    MY_LOGD("+");
    IStateManager::StateObserver stateWaiter(getStateManager());
    getStateManager()->registerOneShotObserver(&stateWaiter);
    //
    //
    MY_LOGD_IF(ENABLE_OP_LOG, "++");
    //
    status_t status = OK;
    //
    status = pHandler->onHandlePreCapture();

    //[ATV]+
    
    if  ( OK != status ) {
        goto lbExit;
    }
    //
    status = stateWaiter.waitState(eState_PreCapture);
    if  ( OK != status ) {
        goto lbExit;
    }

    //[ATV]-
    //
lbExit:
    MY_LOGD_IF(ENABLE_OP_LOG, "- status(%d)", status);
    return  status;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  StatePreCapture
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
StatePreCapture::
StatePreCapture(ENState const eState)
    : StateBase(__FUNCTION__, eState)
{
    MY_LOGD("+");
}

//[ATV]+
/*
status_t
StatePreCapture::
onStopPreview(IStateHandler* pHandler)
{
    IStateManager::StateObserver stateWaiter(getStateManager());
    getStateManager()->registerOneShotObserver(&stateWaiter);
    //
    //
    MY_LOGD_IF(ENABLE_OP_LOG, "+");
    //
    status_t status = OK;
    //
    status = pHandler->onHandleStopPreview();
    if  ( OK != status ) {
        goto lbExit;
    }
    //
    status = stateWaiter.waitState(eState_Idle);
    if  ( OK != status ) {
        goto lbExit;
    }
    //
lbExit:
    MY_LOGD_IF(ENABLE_OP_LOG, "- status(%d)", status);
    return  status;
}
*/
status_t
StatePreCapture:: //[ATV]
onCapture(IStateHandler* pHandler)
{
    MY_LOGD("+");
    IStateManager::StateObserver stateWaiter(getStateManager());
    getStateManager()->registerOneShotObserver(&stateWaiter);
    //
    //
    MY_LOGD_IF(ENABLE_OP_LOG, "+");
    //
    status_t status = OK;
    //
    status = pHandler->onHandleCapture();
    if  ( OK != status ) {
        goto lbExit;
    }
    //
#if 1
    status = stateWaiter.waitState(eState_CapturePreview); //[ATV] eState_Capture
    if  ( OK != status ) {
        goto lbExit;
    }
#endif
    //
lbExit:
    MY_LOGD_IF(ENABLE_OP_LOG, "- status(%d)", status);
    return  status;
}

//[ATV]-

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  StateCapture
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// [ATV]+
/*
StateCapture::
StateCapture(ENState const eState)
    : StateBase(__FUNCTION__, eState)
{
}


status_t
StateCapture::
onCaptureDone(IStateHandler* pHandler)
*/
StateCapturePreview::
StateCapturePreview(ENState const eState)
    : StateBase(__FUNCTION__, eState)
{
    MY_LOGD("");
}


status_t
StateCapturePreview::
// [ATV]-
onCaptureDone(IStateHandler* pHandler)
{
    MY_LOGD("+");
    IStateManager::StateObserver stateWaiter(getStateManager());
    getStateManager()->registerOneShotObserver(&stateWaiter);
    //
    //
    MY_LOGD_IF(ENABLE_OP_LOG, "+");
    //
    status_t status = OK;
    //
    status = pHandler->onHandleCaptureDone();
    if  ( OK != status ) {
        goto lbExit;
    }
    //
    
    // [ATV]+
    /*
    status = stateWaiter.waitState(eState_Idle);
    */
    status = stateWaiter.waitState(eState_Preview);
    // [ATV]-
    
    if  ( OK != status ) {
        goto lbExit;
    }
    //
lbExit:
    MY_LOGD_IF(ENABLE_OP_LOG, "- status(%d)", status);
    return  status;
}


status_t
StateCapturePreview::  // [ATV]
onCancelCapture(IStateHandler* pHandler)
{
    IStateManager::StateObserver stateWaiter(getStateManager());
    getStateManager()->registerOneShotObserver(&stateWaiter);
    //
    //
    MY_LOGD_IF(ENABLE_OP_LOG, "+");
    //
    status_t status = OK;
    //
    status = pHandler->onHandleCancelCapture();
    if  ( OK != status ) {
        goto lbExit;
    }
    //
#if 1
    // [ATV]+
    /*
    status = stateWaiter.waitState(eState_Idle);
    */
    status = stateWaiter.waitState(eState_Preview);
    // [ATV]-
    if  ( OK != status ) {
        goto lbExit;
    }
#endif
    //
lbExit:
    MY_LOGD_IF(ENABLE_OP_LOG, "- status(%d)", status);
    return  status;
}



