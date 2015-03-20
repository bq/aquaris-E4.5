
#ifndef _MTK_CAMERA_CAMADAPTER_SCENARIO_SHOT_ENGPARAM_H_
#define _MTK_CAMERA_CAMADAPTER_SCENARIO_SHOT_ENGPARAM_H_


namespace android {
namespace NSShot {
/******************************************************************************
 *
 ******************************************************************************/

struct EngParam: public ShotParam
{
    MUINT32                         u4SensorWidth;
    MUINT32                         u4SensorHeight;
    MUINT32                         u4Bitdepth;
    MUINT32                         u4RawPixelID;
    EImageFormat                    eImgFmt;
    int32_t                         mi4EngRawSaveEn;
    int32_t                         mi4EngSensorMode;
    int32_t                         mi4EngIspMode;    

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////                    Instantiation.
                                    EngParam(
                                        MUINT32 const                   SensorWidth     = 0,
                                        MUINT32 const                   SensorHeight    = 0,
                                        MUINT32 const                   Bitdepth        = 0,
                                        MUINT32 const                   RawPixelID      = 0,
                                        EImageFormat const              ImgFmt          = eImgFmt_UNKNOWN,
                                        int32_t const                   i4EngRawSaveEn  = 0, 
                                        int32_t const                   i4EngSensorMode = 0,
                                        int32_t const                   i4EngIspMode    = 0
                                    )
                                        : u4SensorWidth(SensorWidth)
                                        , u4SensorHeight(SensorHeight)
                                        , u4Bitdepth(Bitdepth)
                                        , u4RawPixelID(RawPixelID)
                                        , eImgFmt(ImgFmt)
                                        , mi4EngRawSaveEn(i4EngRawSaveEn)
                                        , mi4EngSensorMode(i4EngSensorMode)
                                        , mi4EngIspMode(i4EngIspMode)
                                    {
                                    }

                                    enum EngSensorMode { // Duplicate from sensor_hal.h (mediatek\platform\mt6589\hardware\camera\inc\drv\)
                                        ENG_SENSOR_MODE_PREVIEW,            // ACDK_SCENARIO_ID_CAMERA_PREVIEW=0,
                                        ENG_SENSOR_MODE_CAPTURE,            // ACDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG,
                                        ENG_SENSOR_MODE_VIDEO_PREVIEW,      // ACDK_SCENARIO_ID_VIDEO_PREVIEW,
                                        ENG_SENSOR_MODE_ENUM,
                                    };

                                    enum EngIspMode { // for Engineer Mode App (written in Java)
                                        ENG_ISP_MODE_PROCESSED_RAW = '0',
                                        ENG_ISP_MODE_PURE_RAW = '1',
                                        ENG_ISP_MODE_ENUM,
                                    };

                                    enum EngRawType { // for Cam IO
                                        ENG_RAW_TYPE_PURE_RAW = 0, // 0: pure raw
                                        ENG_RAW_TYPE_PROCESSED_RAW = 1, // 1: pre-process raw
                                        ENG_RAW_TYPE_ENUM,
                                    };
};

} // namespace android
} // namespace NSShot
#endif  //  _MTK_CAMERA_CAMADAPTER_SCENARIO_SHOT_ENGPARAM_H_



