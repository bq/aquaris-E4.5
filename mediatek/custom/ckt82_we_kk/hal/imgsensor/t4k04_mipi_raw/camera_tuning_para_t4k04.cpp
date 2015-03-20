#include <utils/Log.h>
#include <fcntl.h>
#include <math.h>

#include "camera_custom_nvram.h"
#include "camera_custom_sensor.h"
#include "image_sensor.h"
#include "kd_imgsensor_define.h"
#include "camera_AE_PLineTable_t4k04.h"
#include "camera_info_t4k04.h"
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
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
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
        65860,    // i4R_AVG
        16037,    // i4R_STD
        82760,    // i4B_AVG
        17651,    // i4B_STD
        {  // i4P00[9]
            5308000, -1720000, -1030000, -930000, 3986000, -494000, 142000, -4102000, 6520000
        },
        {  // i4P10[9]
            1598966, -2549504, 953576, -2855, -636953, 649577, 214456, 246354, -460810
        },
        {  // i4P01[9]
            221979, -337896, 117184, -210532, -288982, 508438, 41062, -698903, 657841
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
            1136,    // u4MinGain, 1024 base = 1x
            10240,    // u4MaxGain, 16x
            130,    // u4MiniISOGain, ISOxx  
            128,    // u4GainStepUnit, 1x/8 
            15,    // u4PreExpUnit 
            30,    // u4PreMaxFrameRate
            14,    // u4VideoExpUnit  
            30,    // u4VideoMaxFrameRate 
            1024,    // u4Video2PreRatio, 1024 base = 1x 
            15,    // u4CapExpUnit 
            27,    // u4CapMaxFrameRate
            1024,    // u4Cap2PreRatio, 1024 base = 1x
            24,    // u4LensFno, Fno = 2.8
            350    // u4FocusLength_100x
        },
        // rHistConfig
        {
            2,    // u4HistHighThres
            40,    // u4HistLowThres
            2,    // u4MostBrightRatio
            1,    // u4MostDarkRatio
            160,    // u4CentralHighBound
            20,    // u4CentralLowBound
            {240, 230, 220, 210, 200},    // u4OverExpThres[AE_CCT_STRENGTH_NUM] 
            {86, 108, 128, 148, 170},    // u4HistStretchThres[AE_CCT_STRENGTH_NUM] 
            {18, 22, 26, 30, 34}    // u4BlackLightThres[AE_CCT_STRENGTH_NUM] 
        },
        // rCCTConfig
        {
            TRUE,    // bEnableBlackLight
            TRUE,    // bEnableHistStretch
            TRUE,    // bEnableAntiOverExposure
            TRUE,    // bEnableTimeLPF
            TRUE,    // bEnableCaptureThres
            TRUE,    // bEnableVideoThres
            TRUE,    // bEnableStrobeThres
            60,    // u4AETarget
            0,    // u4StrobeAETarget
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
            -10,    // i4BVOffset delta BV = value/10 
            4,    // u4PreviewFlareOffset
            4,    // u4CaptureFlareOffset
            5,    // u4CaptureFlareThres
            4,    // u4VideoFlareOffset
            5,    // u4VideoFlareThres
            2,    // u4StrobeFlareOffset
            2,    // u4StrobeFlareThres
            8,    // u4PrvMaxFlareThres
            0,    // u4PrvMinFlareThres
            8,    // u4VideoMaxFlareThres
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
                880,    // i4R
                512,    // i4G
                536    // i4B
            }
        },
        // Original XY coordinate of AWB light source
        {
           // Strobe
            {
                198,    // i4X
                -221    // i4Y
            },
            // Horizon
            {
                -300,    // i4X
                -223    // i4Y
            },
            // A
            {
                -195,    // i4X
                -235    // i4Y
            },
            // TL84
            {
                -13,    // i4X
                -256    // i4Y
            },
            // CWF
            {
                2,    // i4X
                -334    // i4Y
            },
            // DNP
            {
                38,    // i4X
                -232    // i4Y
            },
            // D65
            {
                183,    // i4X
                -217    // i4Y
            },
            // DF
            {
                129,    // i4X
                -296    // i4Y
            }
        },
        // Rotated XY coordinate of AWB light source
        {
            // Strobe
            {
                190,    // i4X
                -228    // i4Y
            },
            // Horizon
            {
                -308,    // i4X
                -212    // i4Y
            },
            // A
            {
                -203,    // i4X
                -228    // i4Y
            },
            // TL84
            {
                -22,    // i4X
                -256    // i4Y
            },
            // CWF
            {
                -10,    // i4X
                -334    // i4Y
            },
            // DNP
            {
                30,    // i4X
                -233    // i4Y
            },
            // D65
            {
                175,    // i4X
                -223    // i4Y
            },
            // DF
            {
                119,    // i4X
                -301    // i4Y
            }
        },
        // AWB gain of AWB light source
        {
            // Strobe 
            {
                903,    // i4R
                512,    // i4G
                528    // i4B
            },
            // Horizon 
            {
                512,    // i4R
                568,    // i4G
                1152    // i4B
            },
            // A 
            {
                540,    // i4R
                512,    // i4G
                916    // i4B
            },
            // TL84 
            {
                711,    // i4R
                512,    // i4G
                737    // i4B
            },
            // CWF 
            {
                806,    // i4R
                512,    // i4G
                802    // i4B
            },
            // DNP 
            {
                738,    // i4R
                512,    // i4G
                666    // i4B
            },
            // D65 
            {
                880,    // i4R
                512,    // i4G
                536    // i4B
            },
            // DF 
            {
                911,    // i4R
                512,    // i4G
                641    // i4B
            }
        },
        // Rotation matrix parameter
        {
            2,    // i4RotationAngle
            256,    // i4Cos
            9    // i4Sin
        },
        // Daylight locus parameter
        {
            -138,    // i4SlopeNumerator
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
            -100,    // i4RightBound
            -700,    // i4LeftBound
            -150,    // i4UpperBound
            -250    // i4LowerBound
            },
            // Warm fluorescent
            {
            -100,    // i4RightBound
            -700,    // i4LeftBound
            -250,    // i4UpperBound
            -400    // i4LowerBound
            },
            // Fluorescent
            {
            0,    // i4RightBound
            -100,    // i4LeftBound
            -150,    // i4UpperBound
            -300    // i4LowerBound
            },
            // CWF
            {
            0,    // i4RightBound
            -100,    // i4LeftBound
            -300,    // i4UpperBound
            -400    // i4LowerBound
            },
            // Daylight
            {
            200,    // i4RightBound
            0,    // i4LeftBound
            -150,    // i4UpperBound
            -300    // i4LowerBound
            },
            // Shade
            {
            550,    // i4RightBound
            200,    // i4LeftBound
            -150,    // i4UpperBound
            -300    // i4LowerBound
            },
            // Daylight Fluorescent
            {
            200,    // i4RightBound
            0,    // i4LeftBound
            -300,    // i4UpperBound
            -400    // i4LowerBound
            }
        },
        // PWB light area
        {
            // Reference area
            {
            550,    // i4RightBound
            -700,    // i4LeftBound
            0,    // i4UpperBound
            -400    // i4LowerBound
            },
            // Daylight
            {
            225,    // i4RightBound
            0,    // i4LeftBound
            -150,    // i4UpperBound
            -300    // i4LowerBound
            },
            // Cloudy daylight
            {
            325,    // i4RightBound
            150,    // i4LeftBound
            -150,    // i4UpperBound
            -300    // i4LowerBound
            },
            // Shade
            {
            425,    // i4RightBound
            150,    // i4LeftBound
            -150,    // i4UpperBound
            -300    // i4LowerBound
            },
            // Twilight
            {
            0,    // i4RightBound
            -160,    // i4LeftBound
            -150,    // i4UpperBound
            -300    // i4LowerBound
            },
            // Fluorescent
            {
            225,    // i4RightBound
            -122,    // i4LeftBound
            -173,    // i4UpperBound
            -384    // i4LowerBound
            },
            // Warm fluorescent
            {
            -103,    // i4RightBound
            -303,    // i4LeftBound
            -173,    // i4UpperBound
            -384    // i4LowerBound
            },
            // Incandescent
            {
            -103,    // i4RightBound
            -303,    // i4LeftBound
            -150,    // i4UpperBound
            -300    // i4LowerBound
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
            812,    // i4R
            512,    // i4G
            587    // i4B
            },
            // Cloudy daylight
            {
            956,    // i4R
            512,    // i4G
            492    // i4B
            },
            // Shade
            {
            1021,    // i4R
            512,    // i4G
            459    // i4B
            },
            // Twilight
            {
            632,    // i4R
            512,    // i4G
            768    // i4B
            },
            // Fluorescent
            {
            809,    // i4R
            512,    // i4G
            685    // i4B
            },
            // Warm fluorescent
            {
            580,    // i4R
            512,    // i4G
            978    // i4B
            },
            // Incandescent
            {
            538,    // i4R
            512,    // i4G
            912    // i4B
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
            6090    // i4OffsetThr
            },
            // Warm fluorescent	
            {
            0,    // i4SliderValue
            4750    // i4OffsetThr
            },
            // Shade
            {
            0,    // i4SliderValue
            1341    // i4OffsetThr
            },
            // Daylight WB gain
            {
            727,    // i4R
            512,    // i4G
            657    // i4B
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
                -483,    // i4RotatedXCoordinate[0]
                -378,    // i4RotatedXCoordinate[1]
                -197,    // i4RotatedXCoordinate[2]
                -145,    // i4RotatedXCoordinate[3]
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


