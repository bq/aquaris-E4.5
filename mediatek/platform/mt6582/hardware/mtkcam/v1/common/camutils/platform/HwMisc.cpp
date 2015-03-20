
#define LOG_TAG "MtkCam/MtkCamUtils"
//
#include <sys/types.h>
#include <mtkcam/common/camutils/HwMisc.h>


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


#define ROUND_TO_2X(x) ((x) & (~0x1))
/******************************************************************************
*
*******************************************************************************/
namespace android {
namespace MtkCamUtils {


/******************************************************************************
* Calc the crop information 
*******************************************************************************/
#warning [TODO] should not in this file ??   
NSCamHW::Rect calCrop(NSCamHW::Rect const &rSrc, NSCamHW::Rect const &rDst, uint32_t ratio)
{
    NSCamHW::Rect rCrop;

    // srcW/srcH < dstW/dstH 
    if (rSrc.w * rDst.h < rDst.w * rSrc.h) {
        rCrop.w = rSrc.w; 
        rCrop.h = rSrc.w * rDst.h / rDst.w;     
    }
    //srcW/srcH > dstW/dstH
    else if (rSrc.w * rDst.h > rDst.w * rSrc.h) { 
        rCrop.w = rSrc.h * rDst.w / rDst.h; 
        rCrop.h = rSrc.h; 
    }
    else {
        rCrop.w = rSrc.w; 
        rCrop.h = rSrc.h; 
    }
    // 
    rCrop.w =  ROUND_TO_2X(rCrop.w * 100 / ratio);   
    rCrop.h =  ROUND_TO_2X(rCrop.h * 100 / ratio);
    //
    rCrop.x = (rSrc.w - rCrop.w) / 2;
    rCrop.y = (rSrc.h - rCrop.h) / 2;
 
    return rCrop;
}


};  // namespace MtkCamUtils
};  // namespace android



