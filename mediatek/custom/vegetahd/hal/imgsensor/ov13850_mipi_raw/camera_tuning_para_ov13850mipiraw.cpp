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
        67400,    // i4R_AVG
        18247,    // i4R_STD
        108480,    // i4B_AVG
        19703,    // i4B_STD
        {  // i4P00[9]
            4578000, -2268000, 248000, -1024000, 3994000, -404000, -612000, -2906000, 6080000
        },
        {  // i4P10[9]
            3449722, -2937362, -521253, -197639, 107623, 114633, 844544, 893577, -1742668
        },
        {  // i4P01[9]
            2213495, -2041533, -178465, -390716, -31588, 440181, 351036, 171751, -531083
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
            24,    // u4LensFno, Fno = 2.8
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
            5,    // u4CaptureFlareThres
            64,    // u4VideoFlareOffset
            5,    // u4VideoFlareThres
            64,    // u4StrobeFlareOffset
            5,    // u4StrobeFlareThres
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
                997,    // i4R
                512,    // i4G
                750    // i4B
            }
        },
        // Original XY coordinate of AWB light source
        {
           // Strobe
            {
                16,    // i4X
                -350    // i4Y
            },
            // Horizon
            {
                -382,    // i4X
                -288    // i4Y
            },
            // A
            {
                -269,    // i4X
                -364    // i4Y
            },
            // TL84
            {
                -104,    // i4X
                -432    // i4Y
            },
            // CWF
            {
                -103,    // i4X
                -429    // i4Y
            },
            // DNP
            {
                -48,    // i4X
                -391    // i4Y
            },
            // D65
            {
                105,    // i4X
                -387    // i4Y
            },
            // DF
            {
                7,    // i4X
                -392    // i4Y
            }
        },
        // Rotated XY coordinate of AWB light source
        {
            // Strobe
            {
                16,    // i4X
                -350    // i4Y
            },
            // Horizon
            {
                -382,    // i4X
                -288    // i4Y
            },
            // A
            {
                -269,    // i4X
                -364    // i4Y
            },
            // TL84
            {
                -104,    // i4X
                -432    // i4Y
            },
            // CWF
            {
                -103,    // i4X
                -429    // i4Y
            },
            // DNP
            {
                -48,    // i4X
                -391    // i4Y
            },
            // D65
            {
                105,    // i4X
                -387    // i4Y
            },
            // DF
            {
                7,    // i4X
                -392    // i4Y
            }
        },
        // AWB gain of AWB light source
        {
            // Strobe 
            {
                840,    // i4R
                512,    // i4G
                805    // i4B
            },
            // Horizon 
            {
                512,    // i4R
                581,    // i4G
                1440    // i4B
            },
            // A 
            {
                582,    // i4R
                512,    // i4G
                1207    // i4B
            },
            // TL84 
            {
                798,    // i4R
                512,    // i4G
                1057    // i4B
            },
            // CWF 
            {
                796,    // i4R
                512,    // i4G
                1053    // i4B
            },
            // DNP 
            {
                815,    // i4R
                512,    // i4G
                927    // i4B
            },
            // D65 
            {
                997,    // i4R
                512,    // i4G
                750    // i4B
            },
            // DF 
            {
                878,    // i4R
                512,    // i4G
                863    // i4B
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
            -99,    // i4SlopeNumerator
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
            -174,    // i4RightBound
            -954,    // i4LeftBound
            -243,    // i4UpperBound
            -359    // i4LowerBound
            },
            // Warm fluorescent
            {
            -174,    // i4RightBound
            -954,    // i4LeftBound
            -359,    // i4UpperBound
            -529    // i4LowerBound
            },
            // Fluorescent
            {
            -58,    // i4RightBound
            -174,    // i4LeftBound
            -285,    // i4UpperBound
            -400    // i4LowerBound
            },
            // CWF
            {
            -58,    // i4RightBound
            -174,    // i4LeftBound
            -400,    // i4UpperBound
            -509    // i4LowerBound
            },
            // Daylight
            {
            130,    // i4RightBound
            -58,    // i4LeftBound
            -285,    // i4UpperBound
            -437    // i4LowerBound
            },
            // Shade
            {
            490,    // i4RightBound
            130,    // i4LeftBound
            -285,    // i4UpperBound
            -437    // i4LowerBound
            },
            // Daylight Fluorescent
            {
            160,    // i4RightBound
            -58,    // i4LeftBound
            -437,    // i4UpperBound
            -509    // i4LowerBound
            }
        },
        // PWB light area
        {
            // Reference area
            {
            490,    // i4RightBound
            -954,    // i4LeftBound
            0,    // i4UpperBound
            -529    // i4LowerBound
            },
            // Daylight
            {
            155,    // i4RightBound
            -58,    // i4LeftBound
            -285,    // i4UpperBound
            -437    // i4LowerBound
            },
            // Cloudy daylight
            {
            255,    // i4RightBound
            80,    // i4LeftBound
            -285,    // i4UpperBound
            -437    // i4LowerBound
            },
            // Shade
            {
            355,    // i4RightBound
            80,    // i4LeftBound
            -285,    // i4UpperBound
            -437    // i4LowerBound
            },
            // Twilight
            {
            -58,    // i4RightBound
            -218,    // i4LeftBound
            -285,    // i4UpperBound
            -437    // i4LowerBound
            },
            // Fluorescent
            {
            155,    // i4RightBound
            -204,    // i4LeftBound
            -337,    // i4UpperBound
            -479    // i4LowerBound
            },
            // Warm fluorescent
            {
            -169,    // i4RightBound
            -369,    // i4LeftBound
            -337,    // i4UpperBound
            -479    // i4LowerBound
            },
            // Incandescent
            {
            -169,    // i4RightBound
            -369,    // i4LeftBound
            -285,    // i4UpperBound
            -437    // i4LowerBound
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
            891,    // i4R
            512,    // i4G
            782    // i4B
            },
            // Cloudy daylight
            {
            1047,    // i4R
            512,    // i4G
            665    // i4B
            },
            // Shade
            {
            1120,    // i4R
            512,    // i4G
            622    // i4B
            },
            // Twilight
            {
            692,    // i4R
            512,    // i4G
            1006    // i4B
            },
            // Fluorescent
            {
            860,    // i4R
            512,    // i4G
            920    // i4B
            },
            // Warm fluorescent
            {
            618,    // i4R
            512,    // i4G
            1280    // i4B
            },
            // Incandescent
            {
            580,    // i4R
            512,    // i4G
            1201    // i4B
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
            5996    // i4OffsetThr
            },
            // Warm fluorescent	
            {
            50,    // i4SliderValue
            3825    // i4OffsetThr
            },
            // Shade
            {
            50,    // i4SliderValue
            343    // i4OffsetThr
            },
            // Daylight WB gain
            {
            810,    // i4R
            512,    // i4G
            923    // i4B
            },
            // Preference gain: strobe
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: tungsten
            {
            480,    // i4R
            512,    // i4G
            530    // i4B
            },
            // Preference gain: warm fluorescent
            {
            480,    // i4R
            512,    // i4G
            530    // i4B
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
            520,    // i4R
            512,    // i4G
            505    // i4B
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
                -487,    // i4RotatedXCoordinate[0]
                -374,    // i4RotatedXCoordinate[1]
                -209,    // i4RotatedXCoordinate[2]
                -153,    // i4RotatedXCoordinate[3]
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


