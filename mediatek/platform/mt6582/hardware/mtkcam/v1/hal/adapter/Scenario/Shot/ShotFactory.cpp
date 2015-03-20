
#define LOG_TAG "MtkCam/Shot"
//
#include <dlfcn.h>
//
#include <mtkcam/Log.h>
#include <mtkcam/common.h>
using namespace NSCam;
#include <mtkcam/v1/IParamsManager.h>
//
#include <Shot/IShot.h>
//
using namespace android;
using namespace NSShot;


/******************************************************************************
 *
 ******************************************************************************/
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


////////////////////////////////////////////////////////////////////////////////
//  Function Prototype.
////////////////////////////////////////////////////////////////////////////////


/******************************************************************************
 *  Normal Shot
 ******************************************************************************/
extern "C"
sp<IShot>
createInstance_NormalShot(
    char const*const    pszShotName,
    uint32_t const      u4ShotMode,
    int32_t const       i4OpenId
);

/******************************************************************************
 *  Continuous Shot
 ******************************************************************************/
extern "C"
sp<IShot>
createInstance_ContinuousShot(
    char const*const    pszShotName, 
    uint32_t const      u4ShotMode, 
    int32_t const       i4OpenId
);

/******************************************************************************
 *  Bss Shot
 ******************************************************************************/
extern "C"
sp<IShot>
createInstance_BestShot(
    char const*const    pszShotName,
    uint32_t const      u4ShotMode,
    int32_t const       i4OpenId
);

/******************************************************************************
 *  Ev Shot
 ******************************************************************************/
extern "C"
sp<IShot>
createInstance_EvShot(
    char const*const    pszShotName,
    uint32_t const      u4ShotMode,
    int32_t const       i4OpenId
);

/******************************************************************************
 *  HDR Shot
 ******************************************************************************/
extern "C"
sp<IShot>
createInstance_HdrShot(
    char const*const    pszShotName,
    uint32_t const      u4ShotMode,
    int32_t const       i4OpenId
);

/******************************************************************************
 *  FB Shot
 ******************************************************************************/
extern "C"
sp<IShot>
createInstance_FaceBeautyShot(
    char const*const    pszShotName,
    uint32_t const      u4ShotMode,
    int32_t const       i4OpenId, 
    int32_t const       iSmoothLevel, 
    int32_t const       iSkinColor, 
    int32_t const       iSharp
);

/******************************************************************************
 *  Engineer Shot
 ******************************************************************************/
extern "C"
sp<IShot>
createInstance_EngShot(
    char const*const    pszShotName, 
    uint32_t const      u4ShotMode, 
    int32_t const       i4OpenId
);

/******************************************************************************
 *  Zsd Shot
 ******************************************************************************/
extern "C"
sp<IShot>
createInstance_ZsdShot(
    char const*const    pszShotName,
    uint32_t const      u4ShotMode,
    int32_t const       i4OpenId
);

////////////////////////////////////////////////////////////////////////////////
//  Shot Factory
////////////////////////////////////////////////////////////////////////////////
/*****************************************************************************
 * @brief Create a shot instance.
 *
 * @details 
 *
 * @note
 *
 * @param[out] rpShot: Reference to a pointer to a newly-created IShot instance.
 *
 * @param[in] u4ShotMode: Shot Mode (defined in EShotMode)
 *
 * @param[in] i4OpenId: Open id: 0, 1, 2, and so on.
 *
 * @param[in] pParamsMgr: Pointer to IParamsManager instance.
 *
 * @return
 *      true indicates success; false indicates failure.
 *
 ******************************************************************************/
bool
createShotInstance(
    sp<IShot>&          rpShot,
    uint32_t const      u4ShotMode,
    int32_t const       i4OpenId, 
    sp<IParamsManager>  pParamsMgr
)
{
    if  ( rpShot != 0 )
    {
        if  ( rpShot->getShotMode() == u4ShotMode )
        {
            MY_LOGI("No change ShotMode: <%#x>", u4ShotMode);
            return  true;
        }
        else
        {
            MY_LOGI("Change ShotMode: <%#x> -> <%#x>", rpShot->getShotMode(), u4ShotMode);
            rpShot = NULL;
        }
    }
    //
    // 
    switch  ( u4ShotMode )
    {
    //  Normal Shot
    case eShotMode_NormalShot:
        rpShot = createInstance_NormalShot("NormalShot", u4ShotMode, i4OpenId);
        break;
    //
    //  Smile-detection Shot
    case eShotMode_SmileShot:
        rpShot = createInstance_NormalShot("SmileShot", u4ShotMode, i4OpenId);
        break;
    //
    //  Auto-scene-detection Shot
    case eShotMode_AsdShot:
        rpShot = createInstance_NormalShot("AsdShot", u4ShotMode, i4OpenId);
        break;
    //
    //  Engineer mode Shot
    case eShotMode_EngShot:
        rpShot = createInstance_EngShot("EngShot", u4ShotMode, i4OpenId);
        break;
    //
    //  Continuous Shot Ncc
    case eShotMode_ContinuousShot:
        rpShot = createInstance_ContinuousShot("ContinuousShotNcc", u4ShotMode, i4OpenId);
        break;
        
    //  Continuous Shot Cc
    case eShotMode_ContinuousShotCc:
        rpShot = createInstance_ContinuousShot("ContinuousShotCc", u4ShotMode, i4OpenId);
        break;
    //
    //  Best Shot
    case eShotMode_BestShot:
        rpShot = createInstance_BestShot("BestShot", u4ShotMode, i4OpenId);
        break;
    //
    //  Ev-bracketshot Shot
    case eShotMode_EvShot:
        rpShot = createInstance_EvShot("EvShot", u4ShotMode, i4OpenId);
        break;
    //
    //  High-dynamic-range Shot
    case eShotMode_HdrShot:
		printf("ShotFactory::createShotInstance hdr");
        rpShot = createInstance_HdrShot("hdr", u4ShotMode, i4OpenId);
        break;
    //
    //  Zero-shutter-delay Shot
    case eShotMode_ZsdShot:
        rpShot = createInstance_ZsdShot("ZSD", u4ShotMode, i4OpenId);
        break;
    //
    //  Face-beautifier Shot
    case eShotMode_FaceBeautyShot: {
        int32_t iSmoothLevel = pParamsMgr->getInt(MtkCameraParameters::KEY_FB_SMOOTH_LEVEL);
        int32_t iSkinColor = pParamsMgr->getInt(MtkCameraParameters::KEY_FB_SKIN_COLOR);
        int32_t iSharp = pParamsMgr->getInt(MtkCameraParameters::KEY_FB_SHARP);
        rpShot = createInstance_FaceBeautyShot("FBShot", u4ShotMode, i4OpenId, iSmoothLevel, iSkinColor, iSharp);
        }break;
    //
    //  Multi-motion Shot
//    case eShotMode_MultiMotionShot:
//        break;
    //
    default: {
            MY_LOGW("Unsupported ShotMode<%#x>", u4ShotMode);
            char const szShotName[] = "TestShot";
            String8 const s8ShotInstFactory = String8::format("createInstance_%s", szShotName);
            //
            void* pCreateInstance = ::dlsym(RTLD_DEFAULT, s8ShotInstFactory.string());
            if  ( ! pCreateInstance ) {
                MY_LOGE("Not exist: %s", s8ShotInstFactory.string());
            }
            else {
                rpShot =reinterpret_cast<sp<IShot> (*)(char const*const, uint32_t const, int32_t const)>
                            (pCreateInstance)(szShotName, u4ShotMode, i4OpenId)
                            ;
            }
        }break;
    }
    //
    return  (rpShot != 0);
}



