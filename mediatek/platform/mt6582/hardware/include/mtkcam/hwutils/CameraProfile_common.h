
#ifndef _MTK_PLATFORM_HARDWARE_INCLUDE_MTKCAM_HWUTILS_CAMERAPROFILE_COMMON_H_
#define _MTK_PLATFORM_HARDWARE_INCLUDE_MTKCAM_HWUTILS_CAMERAPROFILE_COMMON_H_


/******************************************************************************
 *  Camera Profiling Tool
 ******************************************************************************/
namespace CPTool
{


/**
 * @brief Camera profile event of the camera platform
 */
enum
{
    // Define the root event of camera
    Event_Camera_Platform_Root = EVENT_CAMERA_PLATFORM_COMMON,

        // Define the event used in Core
        Event_Core,

            // Define the event used in Core::CamShot
            Event_Core_CamShot,
                // --Define the event used in multi shot
                Event_MShot,
                    Event_MShot_init,
                    Event_MShot_uninit,
                    Event_MShot_start,
                    Event_MShot_onCreateImage,
                    Event_MShot_stop,
                    Event_MShot_onCreateYuvImage,
                    Event_MShot_onCreateThumbImage,
                    Event_MShot_onCreateJpegImage,
                    Event_MShot_createSensorRawImg,
                    Event_MShot_createYuvRawImg,
                    Event_MShot_createJpegImg,
                    Event_MShot_createJpegImgSW,
                    Event_MShot_convertImage,
                    Event_MShot_YV12ToJpeg,
                // --Define the event used in single shot
                Event_SShot,
                    Event_SShot_init,
                    Event_SShot_uninit,
                    Event_SShot_startOneSensor,
                    Event_SShot_startOneMem,
                    Event_SShot_createSensorRawImg,
                    Event_SShot_createYuvRawImg,
                    Event_SShot_createJpegImg,
                // --Define the event used in burst shot
                Event_BShot,
                    Event_BShot_init,
                    Event_BShot_uninit,
                    Event_BShot_startOneSensor,
                    Event_BShot_startOneMem,
                    Event_BShot_createSensorRawImg,
                    Event_BShot_createYuvRawImg,
                    Event_BShot_createJpegImg,

            // Define the event used in Core::drv
            Event_Core_Drv,

                // --Define the event used in sensor driver
                Event_Sensor,
                    Event_Sensor_search,
                    Event_Sensor_open,
                    Event_Sensor_close,
                    Event_Sensor_setScenario,

                // --Define the event used in tpipe driver
                Event_TpipeDrv,

            // Define the event used in Core::FeatureIO
            Event_Core_FeatureIO,

                // --Define the event used in aaa pipe layer
                Event_Pipe_3A,
                    Event_Pipe_3A_AE,
                    Event_Pipe_3A_Single_AF,
                    Event_Pipe_3A_Continue_AF,
                    Event_Pipe_3A_AWB,
                    Event_Pipe_3A_Strobe,
                    Event_Pipe_3A_Flare,
                    Event_Pipe_3A_Flicker,
                    Event_Pipe_3A_ISP,
                    Event_Pipe_3A_ISP_DRVMGR_INIT,
                    Event_Pipe_3A_ISP_TDRIMGR_INIT,
                    Event_Pipe_3A_ISP_LSCMGR_INIT,
                    Event_Pipe_3A_ISP_VALIDATE_FRAMELESS,
                    Event_Pipe_3A_ISP_VALIDATE_PERFRAME,
                    Event_Pipe_3A_ISP_VALIDATE_PERFRAME_DYNAMIC_TUNING,
                    Event_Pipe_3A_ISP_VALIDATE_PERFRAME_PREPARE,
                    Event_Pipe_3A_ISP_VALIDATE_PERFRAME_APPLY,

                // --Define the event used in tdri Mgr
                Event_TdriMgr,

};


/******************************************************************************
 *  BUILD_PLATFORM_CPTEVENTINFO_COMMON
 ******************************************************************************/
#if defined(BUILD_PLATFORM_CPTEVENTINFO_COMMON)
static
CPT_Event_Info
BUILD_PLATFORM_CPTEVENTINFO_COMMON [] =
{
    {Event_Camera_Platform_Root, EVENT_CAMERA_ROOT, "CameraPlatform"}

        // Define the event used in Core
        ,{Event_Core, Event_Camera_Platform_Root, "Core"}
            // Define the event used in Core::CamShot
            ,{Event_Core_CamShot, Event_Core, "CamShot"}
                // --Define the event used in multi shot
                ,{Event_MShot, Event_Core_CamShot, "MultiShot"}
                    ,{Event_MShot_init, Event_MShot, "init"}
                    ,{Event_MShot_uninit, Event_MShot, "uninit"}
                    ,{Event_MShot_start, Event_MShot, "start"}
                    ,{Event_MShot_onCreateImage, Event_MShot, "onCreateImage"}
                    ,{Event_MShot_stop, Event_MShot, "stop"}
                    ,{Event_MShot_onCreateYuvImage, Event_MShot, "onCreateYuvImage"}
                    ,{Event_MShot_onCreateThumbImage, Event_MShot, "onCreateThumbImage"}
                    ,{Event_MShot_onCreateJpegImage, Event_MShot, "onCreateJpegImage"}
                    ,{Event_MShot_createSensorRawImg, Event_MShot, "createSensorRawImg"}
                    ,{Event_MShot_createYuvRawImg, Event_MShot, "createYuvRawImg"}
                    ,{Event_MShot_createJpegImg, Event_MShot, "createJpegImg"}
                    ,{Event_MShot_createJpegImgSW, Event_MShot, "createJpegImgSW"}
                    ,{Event_MShot_convertImage, Event_MShot, "convertImage"}
                    ,{Event_MShot_YV12ToJpeg, Event_MShot, "YV12ToJpeg"}

                // --Define the event used in single shot
                ,{Event_SShot, Event_Core_CamShot, "SingleShot"}
                    ,{Event_SShot_init, Event_SShot, "init"}
                    ,{Event_SShot_uninit, Event_SShot, "uninit"}
                    ,{Event_SShot_startOneSensor, Event_SShot, "startOneSensor"}
                    ,{Event_SShot_startOneMem, Event_SShot, "startOneMem"}
                    ,{Event_SShot_createSensorRawImg, Event_SShot, "createSensorRawImg"}
                    ,{Event_SShot_createYuvRawImg, Event_SShot, "createYuvRawImg"}
                    ,{Event_SShot_createJpegImg, Event_SShot, "createJpegImg"}

                // --Define the event used in burst shot
                ,{Event_BShot, Event_Core_CamShot, "BurstShot"}
                    ,{Event_BShot_init, Event_BShot, "init"}
                    ,{Event_BShot_uninit, Event_BShot, "uninit"}
                    ,{Event_BShot_startOneSensor, Event_BShot, "startOneSensor"}
                    ,{Event_BShot_startOneMem, Event_BShot, "startOneMem"}
                    ,{Event_BShot_createSensorRawImg, Event_BShot, "createSensorRawImg"}
                    ,{Event_BShot_createYuvRawImg, Event_BShot, "createYuvRawImg"}
                    ,{Event_BShot_createJpegImg, Event_BShot, "createJpegImg"}

            // Define the event used in Core::drv
            ,{Event_Core_Drv, Event_Core, "Drv"}
                // --Define the event used in sensor driver
                ,{Event_Sensor, Event_Core_Drv, "Sensor"}
                    ,{Event_Sensor_search, Event_Sensor, "searchSensor"}
                    ,{Event_Sensor_open, Event_Sensor, "open"}
                    ,{Event_Sensor_close, Event_Sensor, "close"}
                    ,{Event_Sensor_setScenario, Event_Sensor, "setScenario"}
                // --Define the event used in tpipe driver
                ,{Event_TpipeDrv, Event_Core_Drv, "TpipeDrv"}

            // Define the event used in Core::FeatureIO
            ,{Event_Core_FeatureIO, Event_Core, "FeatureIO"}
                // --Define the event used in 3A pipe
                ,{Event_Pipe_3A, Event_Core_FeatureIO, "Pipe3A"}
                    ,{Event_Pipe_3A_AE, Event_Pipe_3A, "AE"}
                    ,{Event_Pipe_3A_Single_AF, Event_Pipe_3A, "SingleAF"}
                    ,{Event_Pipe_3A_Continue_AF, Event_Pipe_3A, "ContinueAF"}
                    ,{Event_Pipe_3A_AWB, Event_Pipe_3A, "AWB"}
                    ,{Event_Pipe_3A_Strobe, Event_Pipe_3A, "Strobe"}
                    ,{Event_Pipe_3A_Flare, Event_Pipe_3A, "Flare"}
                    ,{Event_Pipe_3A_Flicker, Event_Pipe_3A, "Flicker"}
                    ,{Event_Pipe_3A_ISP, Event_Pipe_3A, "ISP"}
                    ,{Event_Pipe_3A_ISP_DRVMGR_INIT, Event_Pipe_3A, "ISPDrvMgrInit"}
                    ,{Event_Pipe_3A_ISP_TDRIMGR_INIT, Event_Pipe_3A, "ISPTdriMgrInit"}
                    ,{Event_Pipe_3A_ISP_LSCMGR_INIT, Event_Pipe_3A, "ISPLscMgrInit"}
                    ,{Event_Pipe_3A_ISP_VALIDATE_FRAMELESS, Event_Pipe_3A, "ISPValidateFrameless"}
                    ,{Event_Pipe_3A_ISP_VALIDATE_PERFRAME, Event_Pipe_3A, "ISPValidatePerframe"}
                    ,{Event_Pipe_3A_ISP_VALIDATE_PERFRAME_DYNAMIC_TUNING, Event_Pipe_3A, "ISPValidatePerframeDynamicTuning"}
                    ,{Event_Pipe_3A_ISP_VALIDATE_PERFRAME_PREPARE, Event_Pipe_3A, "ISPValidatePerframePrepare"}
                    ,{Event_Pipe_3A_ISP_VALIDATE_PERFRAME_APPLY, Event_Pipe_3A, "ISPValidatePerframeApply"}
                // --Define the event used in tdriMgr
                ,{Event_TdriMgr, Event_Core_FeatureIO, "TdriMgr"}

};
#endif//BUILD_PLATFORM_CPTEVENTINFO_COMMON


/******************************************************************************
 *
 ******************************************************************************/
};  // namespace CPTool
#endif



