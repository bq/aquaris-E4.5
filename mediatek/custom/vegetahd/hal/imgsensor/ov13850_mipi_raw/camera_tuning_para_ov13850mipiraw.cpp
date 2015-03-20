#include <utils/Log.h>
#include <fcntl.h>
#include <math.h>

#include "camera_custom_nvram.h"
#include "camera_custom_sensor.h"
#include "image_sensor.h"
#include "kd_imgsensor_define.h"
#include "camera_AE_PLineTable_ov13850mipiraw.h"
#include "camera_info_ov13850mipiraw.h"
#include "camera_custom_AEPlinetable.h"
#include "camera_custom_tsf_tbl.h"
const NVRAM_CAMERA_ISP_PARAM_STRUCT CAMERA_ISP_DEFAULT_VALUE =
{{
    //Version
    Version: NVRAM_CAMERA_PARA_FILE_VERSION,
    //SensorId
    SensorId: SENSOR_ID,
    ISPComm:{
        {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        }
    },
    ISPPca:{
        #include INCLUDE_FILENAME_ISP_PCA_PARAM
    },
    ISPRegs:{
        #include INCLUDE_FILENAME_ISP_REGS_PARAM
        },
    ISPMfbMixer:{{
        {//00: MFB mixer for ISO 100
            0x00000000, 0x00000000
        },
        {//01: MFB mixer for ISO 200
            0x00000000, 0x00000000
        },
        {//02: MFB mixer for ISO 400
            0x00000000, 0x00000000
        },
        {//03: MFB mixer for ISO 800
            0x00000000, 0x00000000
        },
        {//04: MFB mixer for ISO 1600
            0x00000000, 0x00000000
        },
        {//05: MFB mixer for ISO 2400
            0x00000000, 0x00000000
        },
        {//06: MFB mixer for ISO 3200
            0x00000000, 0x00000000
        }
    }},
    ISPCcmPoly22:{
        69500,    // i4R_AVG
        18591,    // i4R_STD
        104080,    // i4B_AVG
        21123,    // i4B_STD
        {  // i4P00[9]
            4700000, -1986000, -154000, -1120000, 4274000, -596000, -712000, -2690000, 5964000
        },
        {  // i4P10[9]
            3630869, -3341307, -289562, -407245, 421369, -4006, 438882, 1105331, -1542697
        },
        {  // i4P01[9]
            2360967, -2017366, -343601, -658628, 353015, 314120, -234342, 418408, -179455
        },
        {  // i4P20[9]
            0, 0, 0, 0, 0, 0, 0, 0, 0
        },
        {  // i4P11[9]
            0, 0, 0, 0, 0, 0, 0, 0, 0
        },
        {  // i4P02[9]
            0, 0, 0, 0, 0, 0, 0, 0, 0
        }
    }
}};

const NVRAM_CAMERA_3A_STRUCT CAMERA_3A_NVRAM_DEFAULT_VALUE =
{
    NVRAM_CAMERA_3A_FILE_VERSION, // u4Version
    SENSOR_ID, // SensorId

    // AE NVRAM
    {
        // rDevicesInfo
        {
            1144,    // u4MinGain, 1024 base = 1x
            7680,    // u4MaxGain, 16x
            70,    // u4MiniISOGain, ISOxx  
            128,    // u4GainStepUnit, 1x/8 
            25,    // u4PreExpUnit 
            24,    // u4PreMaxFrameRate
            25,    // u4VideoExpUnit  
            24,    // u4VideoMaxFrameRate 
            1024,    // u4Video2PreRatio, 1024 base = 1x 
            25,    // u4CapExpUnit 
            12,    // u4CapMaxFrameRate
            1024,    // u4Cap2PreRatio, 1024 base = 1x
            20,    // u4LensFno, Fno = 2.8
            0    // u4FocusLength_100x
        },
        // rHistConfig
        {
            4,    // u4HistHighThres
            40,    // u4HistLowThres
            2,    // u4MostBrightRatio
            1,    // u4MostDarkRatio
            160,    // u4CentralHighBound
            20,    // u4CentralLowBound
            {240, 230, 220, 210, 200},    // u4OverExpThres[AE_CCT_STRENGTH_NUM] 
            {82, 108, 128, 148, 170},    // u4HistStretchThres[AE_CCT_STRENGTH_NUM] 
            {18, 22, 26, 30, 34}    // u4BlackLightThres[AE_CCT_STRENGTH_NUM] 
        },
        // rCCTConfig
        {
            TRUE,    // bEnableBlackLight
            TRUE,    // bEnableHistStretch
            FALSE,    // bEnableAntiOverExposure
            TRUE,    // bEnableTimeLPF
            FALSE,    // bEnableCaptureThres
            FALSE,    // bEnableVideoThres
            FALSE,    // bEnableStrobeThres
            54,    // u4AETarget
            47,    // u4StrobeAETarget
            50,    // u4InitIndex
            4,    // u4BackLightWeight
            32,    // u4HistStretchWeight
            4,    // u4AntiOverExpWeight
            2,    // u4BlackLightStrengthIndex
            2,    // u4HistStretchStrengthIndex
            2,    // u4AntiOverExpStrengthIndex
            2,    // u4TimeLPFStrengthIndex
            {1, 3, 5, 7, 8},    // u4LPFConvergeTable[AE_CCT_STRENGTH_NUM] 
            90,    // u4InDoorEV = 9.0, 10 base 
            -4,    // i4BVOffset delta BV = value/10 
            64,    // u4PreviewFlareOffset
            64,    // u4CaptureFlareOffset
            3,    // u4CaptureFlareThres
            64,    // u4VideoFlareOffset
            3,    // u4VideoFlareThres
            32,    // u4StrobeFlareOffset
            3,    // u4StrobeFlareThres
            160,    // u4PrvMaxFlareThres
            0,    // u4PrvMinFlareThres
            160,    // u4VideoMaxFlareThres
            0,    // u4VideoMinFlareThres
            18,    // u4FlatnessThres    // 10 base for flatness condition.
            75    // u4FlatnessStrength
        }
    },
    // AWB NVRAM
    {
        // AWB calibration data
        {
            // rUnitGain (unit gain: 1.0 = 512)
            {
                0,    // i4R
                0,    // i4G
                0    // i4B
            },
            // rGoldenGain (golden sample gain: 1.0 = 512)
            {
                0,    // i4R
                0,    // i4G
                0    // i4B
            },
            // rTuningUnitGain (Tuning sample unit gain: 1.0 = 512)
            {
                0,    // i4R
                0,    // i4G
                0    // i4B
            },
            // rD65Gain (D65 WB gain: 1.0 = 512)
            {
                990,    // i4R
                512,    // i4G
                701    // i4B
            }
        },
        // Original XY coordinate of AWB light source
        {
           // Strobe
            {
                79,    // i4X
                -328    // i4Y
            },
            // Horizon
            {
                -359,    // i4X
                -315    // i4Y
            },
            // A
            {
                -249,    // i4X
                -343    // i4Y
            },
            // TL84
            {
                -94,    // i4X
                -384    // i4Y
            },
            // CWF
            {
                -81,    // i4X
                -425    // i4Y
            },
            // DNP
            {
                -7,    // i4X
                -392    // i4Y
            },
            // D65
            {
                127,    // i4X
                -360    // i4Y
            },
            // DF
            {
                30,    // i4X
                -400    // i4Y
            }
        },
        // Rotated XY coordinate of AWB light source
        {
            // Strobe
            {
                79,    // i4X
                -328    // i4Y
            },
            // Horizon
            {
                -359,    // i4X
                -315    // i4Y
            },
            // A
            {
                -249,    // i4X
                -343    // i4Y
            },
            // TL84
            {
                -94,    // i4X
                -384    // i4Y
            },
            // CWF
            {
                -81,    // i4X
                -425    // i4Y
            },
            // DNP
            {
                -7,    // i4X
                -392    // i4Y
            },
            // D65
            {
                127,    // i4X
                -360    // i4Y
            },
            // DF
            {
                30,    // i4X
                -400    // i4Y
            }
        },
        // AWB gain of AWB light source
        {
            // Strobe 
            {
                888,    // i4R
                512,    // i4G
                717    // i4B
            },
            // Horizon 
            {
                512,    // i4R
                543,    // i4G
                1355    // i4B
            },
            // A 
            {
                582,    // i4R
                512,    // i4G
                1141    // i4B
            },
            // TL84 
            {
                758,    // i4R
                512,    // i4G
                979    // i4B
            },
            // CWF 
            {
                815,    // i4R
                512,    // i4G
                1016    // i4B
            },
            // DNP 
            {
                862,    // i4R
                512,    // i4G
                879    // i4B
            },
            // D65 
            {
                990,    // i4R
                512,    // i4G
                701    // i4B
            },
            // DF 
            {
                916,    // i4R
                512,    // i4G
                845    // i4B
            }
        },
        // Rotation matrix parameter
        {
            0,    // i4RotationAngle
            256,    // i4Cos
            0    // i4Sin
        },
        // Daylight locus parameter
        {
            -115,    // i4SlopeNumerator
            128    // i4SlopeDenominator
        },
        // AWB light area
        {
            // Strobe:FIXME
            {
            0,    // i4RightBound
            0,    // i4LeftBound
            0,    // i4UpperBound
            0    // i4LowerBound
            },
            // Tungsten
            {
            -184,    // i4RightBound
            -894,    // i4LeftBound
            -279,    // i4UpperBound
            -379    // i4LowerBound
            },
            // Warm fluorescent
            {
            -184,    // i4RightBound
            -894,    // i4LeftBound
            -379,    // i4UpperBound
            -499    // i4LowerBound
            },
            // Fluorescent
            {
            -57,    // i4RightBound
            -184,    // i4LeftBound
            -279,    // i4UpperBound
            -384    // i4LowerBound
            },
            // CWF
            {
            -57,    // i4RightBound
            -184,    // i4LeftBound
            -384,    // i4UpperBound
            -475    // i4LowerBound
            },
            // Daylight
            {
            152,    // i4RightBound
            -57,    // i4LeftBound
            -280,    // i4UpperBound
            -450    // i4LowerBound
            },
            // Shade
            {
            512,    // i4RightBound
            152,    // i4LeftBound
            -280,    // i4UpperBound
            -450    // i4LowerBound
            },
            // Daylight Fluorescent
            {
            182,    // i4RightBound
            -57,    // i4LeftBound
            -450,    // i4UpperBound
            -535    // i4LowerBound
            }
        },
        // PWB light area
        {
            // Reference area
            {
            512,    // i4RightBound
            -894,    // i4LeftBound
            0,    // i4UpperBound
            -535    // i4LowerBound
            },
            // Daylight
            {
            177,    // i4RightBound
            -57,    // i4LeftBound
            -280,    // i4UpperBound
            -450    // i4LowerBound
            },
            // Cloudy daylight
            {
            277,    // i4RightBound
            102,    // i4LeftBound
            -280,    // i4UpperBound
            -450    // i4LowerBound
            },
            // Shade
            {
            377,    // i4RightBound
            102,    // i4LeftBound
            -280,    // i4UpperBound
            -450    // i4LowerBound
            },
            // Twilight
            {
            -57,    // i4RightBound
            -217,    // i4LeftBound
            -280,    // i4UpperBound
            -450    // i4LowerBound
            },
            // Fluorescent
            {
            177,    // i4RightBound
            -194,    // i4LeftBound
            -310,    // i4UpperBound
            -475    // i4LowerBound
            },
            // Warm fluorescent
            {
            -149,    // i4RightBound
            -349,    // i4LeftBound
            -310,    // i4UpperBound
            -475    // i4LowerBound
            },
            // Incandescent
            {
            -149,    // i4RightBound
            -349,    // i4LeftBound
            -280,    // i4UpperBound
            -450    // i4LowerBound
            },
            // Gray World
            {
            5000,    // i4RightBound
            -5000,    // i4LeftBound
            5000,    // i4UpperBound
            -5000    // i4LowerBound
            }
        },
        // PWB default gain	
        {
            // Daylight
            {
            910,    // i4R
            512,    // i4G
            774    // i4B
            },
            // Cloudy daylight
            {
            1085,    // i4R
            512,    // i4G
            649    // i4B
            },
            // Shade
            {
            1161,    // i4R
            512,    // i4G
            607    // i4B
            },
            // Twilight
            {
            697,    // i4R
            512,    // i4G
            1010    // i4B
            },
            // Fluorescent
            {
            861,    // i4R
            512,    // i4G
            881    // i4B
            },
            // Warm fluorescent
            {
            622,    // i4R
            512,    // i4G
            1220    // i4B
            },
            // Incandescent
            {
            599,    // i4R
            512,    // i4G
            1176    // i4B
            },
            // Gray World
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            }
        },
        // AWB preference color	
        {
            // Tungsten
            {
            0,    // i4SliderValue
            5721    // i4OffsetThr
            },
            // Warm fluorescent	
            {
            0,    // i4SliderValue
            5243    // i4OffsetThr
            },
            // Shade
            {
            0,    // i4SliderValue
            1341    // i4OffsetThr
            },
            // Daylight WB gain
            {
            826,    // i4R
            512,    // i4G
            841    // i4B
            },
            // Preference gain: strobe
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: tungsten
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: warm fluorescent
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: fluorescent
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: CWF
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: daylight
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: shade
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: daylight fluorescent
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            }
        },
        {// CCT estimation
            {// CCT
                2300,    // i4CCT[0]
                2850,    // i4CCT[1]
                4100,    // i4CCT[2]
                5100,    // i4CCT[3]
                6500    // i4CCT[4]
            },
            {// Rotated X coordinate
                -486,    // i4RotatedXCoordinate[0]
                -376,    // i4RotatedXCoordinate[1]
                -221,    // i4RotatedXCoordinate[2]
                -134,    // i4RotatedXCoordinate[3]
                0    // i4RotatedXCoordinate[4]
            }
        }
    },
    {0}
};

#include INCLUDE_FILENAME_ISP_LSC_PARAM
//};  //  namespace

const CAMERA_TSF_TBL_STRUCT CAMERA_TSF_DEFAULT_VALUE =
{
    #include INCLUDE_FILENAME_TSF_PARA
    #include INCLUDE_FILENAME_TSF_DATA
};


typedef NSFeature::RAWSensorInfo<SENSOR_ID> SensorInfoSingleton_T;


namespace NSFeature {
template <>
UINT32
SensorInfoSingleton_T::
impGetDefaultData(CAMERA_DATA_TYPE_ENUM const CameraDataType, VOID*const pDataBuf, UINT32 const size) const
{
    UINT32 dataSize[CAMERA_DATA_TYPE_NUM] = {sizeof(NVRAM_CAMERA_ISP_PARAM_STRUCT),
                                             sizeof(NVRAM_CAMERA_3A_STRUCT),
                                             sizeof(NVRAM_CAMERA_SHADING_STRUCT),
                                             sizeof(NVRAM_LENS_PARA_STRUCT),
                                             sizeof(AE_PLINETABLE_T),
                                             0,
                                             sizeof(CAMERA_TSF_TBL_STRUCT)};

    if (CameraDataType > CAMERA_DATA_TSF_TABLE || NULL == pDataBuf || (size < dataSize[CameraDataType]))
    {
        return 1;
    }

    switch(CameraDataType)
    {
        case CAMERA_NVRAM_DATA_ISP:
            memcpy(pDataBuf,&CAMERA_ISP_DEFAULT_VALUE,sizeof(NVRAM_CAMERA_ISP_PARAM_STRUCT));
            break;
        case CAMERA_NVRAM_DATA_3A:
            memcpy(pDataBuf,&CAMERA_3A_NVRAM_DEFAULT_VALUE,sizeof(NVRAM_CAMERA_3A_STRUCT));
            break;
        case CAMERA_NVRAM_DATA_SHADING:
            memcpy(pDataBuf,&CAMERA_SHADING_DEFAULT_VALUE,sizeof(NVRAM_CAMERA_SHADING_STRUCT));
            break;
        case CAMERA_DATA_AE_PLINETABLE:
            memcpy(pDataBuf,&g_PlineTableMapping,sizeof(AE_PLINETABLE_T));
            break;
        case CAMERA_DATA_TSF_TABLE:
            memcpy(pDataBuf,&CAMERA_TSF_DEFAULT_VALUE,sizeof(CAMERA_TSF_TBL_STRUCT));
            break;
        default:
            break;
    }
    return 0;
}}; // NSFeature


