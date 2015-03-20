
#ifndef _MTK_CAMERA_INC_CONFIG_PRIORITYDEFS_H_
#define _MTK_CAMERA_INC_CONFIG_PRIORITYDEFS_H_
/******************************************************************************
 *  Priority Definitions.
 ******************************************************************************/
#include <linux/rtpm_prio.h>


/******************************************************************************
 *  Real-Time Priority (SCHED_RR, SCHED_FIFO)
 ******************************************************************************/
enum
{
    //
    PRIO_RT_CAMERA_PREVIEW                  = (RTPM_PRIO_CAMERA_TOPBASE - 1),
    PRIO_RT_CAMERA_CAPTURE                  = (RTPM_PRIO_CAMERA_TOPBASE - 1),
    //
    PRIO_RT_CAMERA_SHUTTER_CB               = (RTPM_PRIO_CAMERA_TOPBASE - 1),
    PRIO_RT_CAMERA_ZIP_IMAGE_CB             = (RTPM_PRIO_CAMERA_TOPBASE - 1),
    //
    //  Clients
    PRIO_RT_CAMERA_RECORD_CLIENT            = (RTPM_PRIO_CAMERA_TOPBASE - 1),
    PRIO_RT_CAMERA_PREVIEW_CLIENT           = (RTPM_PRIO_CAMERA_TOPBASE - 1),
    PRIO_RT_CAMERA_DISPLAY_CLIENT           = (RTPM_PRIO_CAMERA_TOPBASE - 0),
    //
    //
    PRIO_RT_ISP_EVENT_THREAD                = (RTPM_PRIO_CAMERA_TOPBASE - 1),
    //
    PRIO_RT_AF_THREAD                       = (RTPM_PRIO_CAMERA_TOPBASE - 2),
    PRIO_RT_3A_THREAD                       = (RTPM_PRIO_CAMERA_TOPBASE - 2),
    PRIO_RT_F858_THREAD                     = (RTPM_PRIO_CAMERA_TOPBASE - 2),
    //
    PRIO_RT_VSS_THREAD                      = (RTPM_PRIO_CAMERA_TOPBASE - 2),
    //
    PRIO_RT_FD_THREAD                       = (RTPM_PRIO_CAMERA_TOPBASE - 3),
};


/******************************************************************************
 *
 ******************************************************************************/
#endif  //_MTK_CAMERA_INC_CONFIG_PRIORITYDEFS_H_



