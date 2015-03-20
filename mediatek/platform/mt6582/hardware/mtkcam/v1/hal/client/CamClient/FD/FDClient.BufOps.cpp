
#define LOG_TAG "MtkCam/CamClient/FDClient"
//
#include "FDClient.h"
#include <camera/MtkCameraParameters.h>
//
using namespace NSCamClient;
using namespace NSFDClient;
/******************************************************************************
*
*******************************************************************************/
#define ENABLE_LOG_PER_FRAME        (1)
#define SCALE_NUM (11)

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
FDClient::initBuffers(sp<IImgBufQueue>const &rpBufQueue)
{   
     return createDetectedBuffers() && createWorkingBuffers(rpBufQueue) && createDDPWorkBuffers() && createFDWorkBuffers();
}


/******************************************************************************
*
*******************************************************************************/
void
FDClient::uninitBuffers()
{
    destroyDetectedBuffers();
    destroyWorkingBuffers();
    destroyDDPWorkBuffers();
    destroyFDWorkBuffers();
}

/******************************************************************************
*
*******************************************************************************/
bool
FDClient::
createDetectedBuffers()
{

    bool ret = false;

    mpDetectedFaces = new MtkCameraFaceMetadata;
    if ( NULL != mpDetectedFaces )
    {
        MtkCameraFace *faces = new MtkCameraFace[FDClient::mDetectedFaceNum];
        MtkFaceInfo *posInfo = new MtkFaceInfo[FDClient::mDetectedFaceNum];

        if ( NULL != faces &&  NULL != posInfo)
        {
            mpDetectedFaces->faces = faces;

            mpDetectedFaces->posInfo = posInfo;
            mpDetectedFaces->number_of_faces = 0;    
            ret = true;
        }
    }

    return ret;
}


    
/******************************************************************************
*
*******************************************************************************/
bool
FDClient::
createWorkingBuffers(sp<IImgBufQueue>const &rpBufQueue)
{
    bool ret = true;
    //
    // [Seed FD buffer]
    // use AP setting: format/width/height
    // 
    int bufWidth = 0, bufHeight = 0;
    mpParamsMgr->getPreviewSize(&bufWidth, &bufHeight);
        
    // String8 const format = mpParamsMgr->getPreviewFormat();
    String8 const format = String8(MtkCameraParameters::PIXEL_FORMAT_YUV420P);

    // if rot equals to 90 or 270, width and height should be switched. 
    uint32_t rot = 0;
    if (bufWidth < bufHeight) 
    {
        rot = 90;
        int tmp = bufWidth;
        bufWidth = bufHeight;
        bufHeight = tmp;
    }
    //
    MY_LOGD("[seed FD buffer] w: %d, h: %d, format: %s, rot: %d", bufWidth, bufHeight, format.string(), rot);
    //
    for (int i = 0; i < FDClient::mBufCnt; i++) 
    {
        sp<FDBuffer> one = new FDBuffer(bufWidth, bufHeight, 
                                        FmtUtils::queryBitsPerPixel(format.string()),
                                        FmtUtils::queryImgBufferSize(format.string(), bufWidth, bufHeight),
                                        format, "FDBuffer");
            
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
bool
FDClient::
createDDPWorkBuffers()
{
    bool ret = true;   
    int buff_size = 0;
 
    for(int i =0;i<SCALE_NUM;i++)
    {
        buff_size += bufScaleX[i] * bufScaleY[i] * 2 ; //415880 bytes
    }
    
    //DDPBuffer = new FDBuffer(0, 0, 0, buff_size);
    DDPBuffer = new unsigned char[buff_size];
    
    return ret;
}


/******************************************************************************
*
*******************************************************************************/
bool
FDClient::
createFDWorkBuffers()
{
    bool ret = true;   
    //FDWorkingBufferSize = 1572864; //1.5M: 1024*1024*1.5
    FDWorkingBufferSize = 15728640; //15M: 1024*1024*15
    FDWorkingBuffer = new unsigned char[FDWorkingBufferSize];
    
    return ret;
}

/******************************************************************************
*
*******************************************************************************/
void
FDClient::
destroyDetectedBuffers()
{

    if ( mpDetectedFaces != NULL )
    {
        if ( mpDetectedFaces->faces != NULL )
        {
            delete [] mpDetectedFaces->faces;
            mpDetectedFaces->faces = NULL;
        }

        if ( mpDetectedFaces->posInfo != NULL)
        {
            delete [] mpDetectedFaces->posInfo;   
            mpDetectedFaces->posInfo = NULL;
        }

        delete mpDetectedFaces;
        mpDetectedFaces = NULL;
    }
}


/******************************************************************************
*
*******************************************************************************/
void
FDClient::
destroyWorkingBuffers()
{
    // suppose destroy buffer by stopProcessor
}


/******************************************************************************
*
*******************************************************************************/
void
FDClient::
destroyDDPWorkBuffers()
{
    // suppose destroy buffer by stopProcessor
//#warning "[MUST RESOLVE][To Bin] DDP has nothing to do with Processor. \
//          So the way FD buffer do destroy doesn't work for DDP."

    delete [] DDPBuffer;
}

void
FDClient::
destroyFDWorkBuffers()
{
    // suppose destroy buffer by stopProcessor

    delete [] FDWorkingBuffer;
}


/******************************************************************************
*
*******************************************************************************/
bool 
FDClient::
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
FDClient::
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



