
#ifndef _MTK_PLATFORM_HARDWARE_INCLUDE_MTKCAM_HWUTILS_CAMERAPROFILE_H_
#define _MTK_PLATFORM_HARDWARE_INCLUDE_MTKCAM_HWUTILS_CAMERAPROFILE_H_

#include <mtkcam/mmp/mmp.h>

#define EVENT_CAMERA_PLATFORM_COMMON        (EVENT_CAMERA_PLATFORM | 0x00000000)
#define EVENT_CAMERA_PLATFORM_V1            (EVENT_CAMERA_PLATFORM | 0x01000000)
#define EVENT_CAMERA_PLATFORM_V3            (EVENT_CAMERA_PLATFORM | 0x03000000)

#include "CameraProfile_common.h"
#include "CameraProfile_v1.h"

/******************************************************************************
 *  Camera Profiling Tool
 ******************************************************************************/
namespace CPTool
{


bool initPlatformProfile();


/******************************************************************************
 *
 ******************************************************************************/
};  // namespace CPTool
#endif



