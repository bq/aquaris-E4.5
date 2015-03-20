
#define LOG_TAG "MtkCam/CamClient/PREFEATUREBASE"
//
#include <CamUtils.h>
using namespace android;
using namespace MtkCamUtils;
//
#include <camera/MtkCameraParameters.h>
//
#include "PreviewFeatureBufMgr.h"
#include "PreviewFeatureBase.h"
//
using namespace NSCamClient;
using namespace NSPREFEATUREABSE;
/******************************************************************************
*
*******************************************************************************/
#define ENABLE_LOG_PER_FRAME        (1)


/******************************************************************************
*
*******************************************************************************/
#define MY_LOGV(fmt, arg...)        CAM_LOGV("(%d)[%s] "fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_LOGD("(%d)[%s] "fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_LOGI("(%d)[%s] "fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_LOGW("(%d)[%s] "fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_LOGE("(%d)[%s] "fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_LOGA("(%d)[%s] "fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_LOGF("(%d)[%s] "fmt, ::gettid(), __FUNCTION__, ##arg)
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
bool
PREFEATUREABSE::initBuffers(sp<IImgBufQueue>const &rpBufQueue)
{   
    return createBuffers() && createWorkingBuffers(rpBufQueue);
}


/******************************************************************************
*
*******************************************************************************/
void
PREFEATUREABSE::uninitBuffers()
{
    destroyBuffers();
    destroyWorkingBuffers();
}


/******************************************************************************
*
*******************************************************************************/
bool
PREFEATUREABSE::
createBuffers()
{
    MY_LOGD("[createBuffers] + mShotNum = %d",mShotNum);
    bool ret = true;

    return ret;
}


    
/******************************************************************************
*
*******************************************************************************/
bool
PREFEATUREABSE::
createWorkingBuffers(sp<IImgBufQueue>const & rpBufQueue)
{
    bool ret = true;
    //   
    //String8 const format = mpParamsMgr->getPreviewFormat();
    String8 const format =  String8(MtkCameraParameters::PIXEL_FORMAT_YUV420SP);
    //
    MY_LOGD("[PreviewFeature buffer] w: %d, h: %d, format: %s", bufWidth, bufHeight, format.string());
    //
    bufCnt = 2;
    for (int i = 0; i < bufCnt; i++)
    {
        sp<PREVIEWFEATUREBuffer> one = new PREVIEWFEATUREBuffer(bufWidth, bufHeight, 
                       FmtUtils::queryBitsPerPixel(format.string()),
                       FmtUtils::queryImgBufferSize(format.string(), bufWidth, bufHeight),
                       format,"PREVIEWFEATUREBuffer");

        ret = rpBufQueue->enqueProcessor( 
            ImgBufQueNode(one, ImgBufQueNode::eSTATUS_TODO)
        );
        
        if ( ! ret )
        {
            MY_LOGW("enqueProcessor() fails");
        }
    }
    
    return ret;
}

/******************************************************************************
*
*******************************************************************************/
void
PREFEATUREABSE::
destroyBuffers()
{
    MY_LOGD("[destroyBuffers] +");
    //deallocMem(mpSourceBuf);
    MY_LOGD("[destroyBuffers] -");
}

/******************************************************************************
*
*******************************************************************************/
void
PREFEATUREABSE::
destroyWorkingBuffers()
{
    // suppose destroy buffer by stopProcessor
}

/******************************************************************************
*
*******************************************************************************/
bool 
PREFEATUREABSE::
handleReturnBuffers(sp<IImgBufQueue>const& rpBufQueue, ImgBufQueNode const &rQueNode)
{
    bool ret = true;
 
    ret = rpBufQueue->enqueProcessor(
            ImgBufQueNode(rQueNode.getImgBuf(), ImgBufQueNode::eSTATUS_TODO));
    
    if ( ! ret )
    {
        MY_LOGE("enqueProcessor() fails");
        ret = false;
    } 

    return ret;
}



/******************************************************************************
* buffer can be reached either by client enque back buffer
* or by previewclient. 
*******************************************************************************/
bool 
PREFEATUREABSE::
waitAndHandleReturnBuffers(sp<IImgBufQueue>const& rpBufQueue, ImgBufQueNode &rQueNode)
{
    bool ret = false;
    Vector<ImgBufQueNode> vQueNode;
    //
    MY_LOGD_IF(ENABLE_LOG_PER_FRAME, "+");
    //
    // (1) wait and deque from processor
    rpBufQueue->dequeProcessor(vQueNode);
    if ( vQueNode.empty() )
    {
        ret = false;
        MY_LOGD("Deque from processor is empty. Suppose stopProcessor has been called");
        goto lbExit;

    }
    // (2) check vQueNode: 
    //    - TODO or CANCEL
    //    - get and keep the latest one with TODO tag;
    //    - otherwise, return to processors
    for (size_t i = 0; i < vQueNode.size(); i++)
    {
        if (vQueNode[i].isDONE() && vQueNode[i].getImgBuf() != 0)
        {
             if (rQueNode.getImgBuf() != 0 ) // already got 
             {
                 handleReturnBuffers(rpBufQueue, rQueNode);
             }
             rQueNode = vQueNode[i];  // update a latest one
             ret = true;
        }
        else 
        {
             handleReturnBuffers(rpBufQueue, vQueNode[i]);
        }
    }
lbExit:
    //
    MY_LOGD_IF(ENABLE_LOG_PER_FRAME, "-");
    return ret;
}



