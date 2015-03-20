
#define LOG_TAG "CameraProfile"

#define BUILD_PLATFORM_CPTEVENTINFO_COMMON  gCPTEventInfo_Common
#define BUILD_PLATFORM_CPTEVENTINFO_V1      gCPTEventInfo_V1
//#define BUILD_PLATFORM_CPTEVENTINFO_V3      gCPTEventInfo_V3
#include <mtkcam/hwutils/CameraProfile.h>
#include <mtkcam/mmp/Profile.h>


/******************************************************************************
 *
 ******************************************************************************/
#define ARRAY_OF(a)     (sizeof(a)/sizeof(a[0]))


/******************************************************************************
 *  Camera Profiling Tool
 ******************************************************************************/
namespace CPTool
{

static bool gbInit = false; 

bool initPlatformProfile()
{
    if  ( ! initCommonProfile() )
    {
        return false;
    }
    //
    if  ( ! gbInit )
    {
        bool ret = false;

#if defined(BUILD_PLATFORM_CPTEVENTINFO_COMMON)
        ret = CPTRegisterEvents(BUILD_PLATFORM_CPTEVENTINFO_COMMON, ARRAY_OF(BUILD_PLATFORM_CPTEVENTINFO_COMMON));
        if  ( ! ret ) return false;
#endif
        //
#if defined(BUILD_PLATFORM_CPTEVENTINFO_V1)
        ret = CPTRegisterEvents(BUILD_PLATFORM_CPTEVENTINFO_V1, ARRAY_OF(BUILD_PLATFORM_CPTEVENTINFO_V1));
        if  ( ! ret ) return false;
#endif
        //
#if defined(BUILD_PLATFORM_CPTEVENTINFO_V3)
        ret = CPTRegisterEvents(BUILD_PLATFORM_CPTEVENTINFO_V3, ARRAY_OF(BUILD_PLATFORM_CPTEVENTINFO_V3));
        if  ( ! ret ) return false;
#endif
        //
        gbInit = true;
    }
    return  true;
}


/******************************************************************************
 *
 ******************************************************************************/
};  // namespace CPTool



