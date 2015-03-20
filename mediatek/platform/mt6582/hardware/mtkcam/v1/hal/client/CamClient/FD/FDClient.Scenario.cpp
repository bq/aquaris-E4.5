
#define LOG_TAG "MtkCam/CamClient/FDClient"
//
#include "FDClient.h"
//

using namespace NSCamClient;
using namespace NSFDClient;
//

/******************************************************************************
*
*******************************************************************************/
#define ENABLE_LOG_PER_FRAME        (1)
#define GET_FD_RESULT (1)
#define GET_SD_RESULT (1)

static unsigned char *src_buf;
static unsigned char *dst_rgb;
static unsigned char numFace =0;

unsigned char g_BufferGroup = 0;

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
 ******************************************************************************/
bool
FDClient::
doFD(ImgBufQueNode const& rQueNode, bool &rIsDetected_FD, bool &rIsDetected_SD, bool rDoSD)
{
    bool ret = true;
    MINT32 SD_Result;
   
    MY_LOGD_IF(ENABLE_LOG_PER_FRAME, "+");
    //MY_LOGD("Rotation_Info1:%d", Rotation_Info);
    
//********************************************************************************************//

    dst_rgb = (unsigned char *)DDPBuffer;
    src_buf = (unsigned char *)rQueNode.getImgBuf()->getVirAddr();
    
    g_BufferGroup = doBufferAnalysis(rQueNode);
    //g_BufferGroup = 1;

    mpFDHalObj->halFDBufferCreate(dst_rgb, src_buf, g_BufferGroup);
    mpFDHalObj->halFDDo(0, (MINT32)dst_rgb,  (MINT32)rQueNode.getImgBuf()->getVirAddr(),  rDoSD, Rotation_Info);
    
//********************************************************************************************//
    
#if (GET_FD_RESULT)    
    if( NULL != mpDetectedFaces )
         numFace = mpFDHalObj->halFDGetFaceResult(mpDetectedFaces);
    MY_LOGD("Scenario FD Num: %d\n",numFace );
    if(numFace>0)
        rIsDetected_FD = 1;
   else
        rIsDetected_FD = 0;
#endif

#if (GET_SD_RESULT)
    SD_Result = mpFDHalObj->halSDGetSmileResult();
    MY_LOGD("Scenario SD Result1: %d\n",SD_Result );
    if(SD_Result>0)
        rIsDetected_SD = 1;
   else
        rIsDetected_SD = 0;
#endif
    
    MY_LOGD_IF(ENABLE_LOG_PER_FRAME, "-");    
    return ret;
}
/******************************************************************************
 *
 ******************************************************************************/

int 
FDClient::
doBufferAnalysis(ImgBufQueNode const& a_rQueNode)
{
    int plane =0;
    
    int BufWidth = a_rQueNode.getImgBuf()->getImgWidth();
    int BufHeight  = a_rQueNode.getImgBuf()->getImgHeight();
    //plane = a_rQueNode.getPlaneCount();
    plane = FmtUtils::queryPlaneCount(a_rQueNode.getImgBuf()->getImgFormat().string());
    //MY_LOGD("getImgWidth:%d, getImgHeight:%d", a_rQueNode.getImgBuf()->getImgWidth(), a_rQueNode.getImgBuf()->getImgHeight());
    
    //MY_LOGD("plane: %d\n",plane );
    
    //Need PlaneCount Information
    if( (BufWidth==640) && (BufHeight == 480) && (plane==2))
        return 0;
    else if ( (BufWidth*3 == BufHeight*4) && (plane==2) )
        return 1;
    else if ( (BufWidth*3 == BufHeight*4) && (plane==3) )
        return 2;
    else if ( ( BufWidth*9 == BufHeight*16) && (plane==2) )
        return 3;
    else if ( ( BufWidth*9 == BufHeight*16) && (plane==3) )
        return 4;
    else if ( ( BufWidth*3 == BufHeight*5) && (plane==2) )
        return 5;
    else if ( ( BufWidth*3 == BufHeight*5) && (plane==3) )
        return 6;
    else if ( (plane==2) )
        return 3;
    else if ( (plane==3) )
        return 4;        
    else
        return 9;
    
    return plane;
    
}

 

