
/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef _MTK_MAV_H
#define _MTK_MAV_H

#include "MTKMavType.h"
#include "MTKMavErrCode.h"

typedef enum DRVMavObject_s {
    DRV_MAV_OBJ_NONE = 0,
    DRV_MAV_OBJ_SW,
    DRV_MAV_OBJ_SW_NEON,    
    DRV_MAV_OBJ_PANO3D, 
//20111206
    DRV_MAV_OBJ_SWHDR,
    DRV_MAV_OBJ_UNKNOWN = 0xFF,
} DrvMavObject_e;

/*****************************************************************************
    MAV Define and State Machine 
******************************************************************************/
#define MAV_MAX_IMAGE_NUM   (25)    // maximum image number
#define MAV_BUFFER_UNSET    (0xFFFF)

/* perfomance parameter options in RectifyImage function */
#define RANK                    (3)

//#define SW_EIS_SUPPORT
#define DEBUG
//#define FEM_DUMP
#define MAV_MULTI_CORE_OPT
#ifndef SIM_MAIN
//#define TIME_PROF
#endif


typedef enum MAV_STATE_ENUM
{
    MAV_STATE_STANDBY=0,    // After Create Obj or Reset
    MAV_STATE_INIT,         // After Called MavInit
    MAV_STATE_PROC,         // After Called MavMain
    MAV_STATE_PROC_READY,   // After Finish MavMain  
    MAV_STATE_MERGE,        // After Called MavMerge    
    MAV_STATE_READY         // After Finish MavMerge
} MAV_STATE_ENUM;

//========================================================================================
/* Debug option */
#define DUMP        // define dump intermediate data, undefine dump only error and info
//========================================================================================

/*****************************************************************************
    Process Control 
******************************************************************************/
// MAVInit, 
// Input    : MavTuningInfo
// Output   : NONE
struct MavTuningInfo
{
    // Extraction parameters
    MINT32                  RCWinBound;         // RC Window Bound to select points

    // Matching parameters
    MINT32                  SearchRange;        // Search Range to find the matching pair
    MINT32                  MatchRate;          // Error rate between minimum error and second minimum error

    // Rectification parameters
    MINT32                  RectErrThre;        // Pixel error threshold for self image rectification
    MINT32                  IterNum;            // Iteration number of LM method
    MINT32                  MaxAngle;           // Maximum angle for self image recitification

    // Alignment parameters
    MINT32                  ClipRatio;          // Image clip ratio after alignment (real value*256)
    MFLOAT                  DispBound;          // bound for disparity check
    MFLOAT                  DispRatio;          // ratio for disparity check
    MINT32                  ActMatThre;         // threshold of active matching pairs

    // Disparity parameters
    MINT32                  SpecDispMin;        // disparity boundary of foreground
    MINT32                  SpecDispMax;        // disparity boundary of background
    MUINT32                 SpecPanelWidth;     // visible width of the panel
    MUINT32                 SpecPanelHeight;    // visible height of the panel

    // Multi-Core parameters
    MUINT32                 CoreNumber;         // given cpu core number
};

typedef enum MAV_ERR_ENUM
{
    MAV_ERR_LACK_MATCH_POINT=0,                 // error of lacking of matching points pair
    MAV_ERR_LEVMAR_DIVERGENCE,                  // error of divergence in levmar
    MAV_ERR_SMALL_CLIP_REGION,                  // error of small clip region
    MAV_ERR_ALIGN_INSUFF_NUM,                   // error of insufficient active matching pairs in alignement
    MAV_ERR_REVERSE_ORDER,                      // warning of reverse capture order
    MAV_ERR_VERGENCE_FAIL,                      // error of entering painful disparity
    MAV_ERR_VERGENCE_WARNING,                   // warning of entering discomfort disparity
    MAV_ERR_NO
} MAV_ERR_ENUM;


/*****************************************************************************
    Feature Control Enum and Structure
******************************************************************************/
typedef enum MAV_FEATURE_ENUM
{
    MAV_FEATURE_BEGIN,
    MAV_FEATURE_ADD_IMAGE,
    MAV_FEATURE_GET_RESULT,
    MAV_FEATURE_GET_LOG,
    MAV_FEATURE_GET_WORKBUF_SIZE,
    MAV_FEATURE_SET_WORKBUF_ADDR,
    MAV_FEATURE_GET_MATCH_PAIR_INFO,
    MAV_FEATURE_SET_PTHREAD_ATTR,
    MAV_FEATURE_MAX
}   MAV_FEATURE_ENUM;

struct MavInitInfo
{
    MUINT32                 WorkingBuffAddr;        // default size : MAV_WORKING_BUFFER_SIZE
    // Other Tuning Parameters
    MavTuningInfo           *pTuningInfo;           // tuning parameters
};

// MAV_FEATURE_ADD_IMAGE, 
// Input    : MavImageInfo
// Output   : NONE
struct MavImageInfo
{
    MUINT32     ImgAddr;
    MUINT16     Width;                  // input image width
    MUINT16     Height;                 // input image height
    MUINT32     AngleValueX;
    MUINT32     AngleValueY;    
    MFLOAT      AngleValueZ;            // AngleValue   
    MINT32      ClipX;                  // Image Global Offset X
    MINT32      ClipY;                  // Image Global Offset Y
    MINT32      MotionValue[2];

    // for 3D Panorama
    MINT16      GridX;                  // vertical offset in panorama space
    MINT16      MinX;                   // x_start in panorama space
    MINT16      MaxX;                   // x_end in panorama space
    MBOOL       ControlFlow;            // head align (0) or center align (1)
};

struct MavResultInfo
{
    MRESULT         RetCode;            // return warning
    MINT16          ClipWidth;          // Image Result Width
    MINT16          ClipHeight;         // Image Result Height
    MUINT16         ViewIdx;            // Image Start View Index
    MUINT8          ErrPattern;         // Returned error/Warning bit pattern
                                        // bit 0: lack of match points (set if error)
                                        // bit 1: small clip region (set if error)
                                        // bit 2: reverse order(set if warning) -> 0/1 = Left-To-Right/Right-To-Left
    MavImageInfo    ImageInfo[MAV_MAX_IMAGE_NUM];
    MFLOAT          ImageHmtx[MAV_MAX_IMAGE_NUM][RANK][RANK];   // 3x3 rectification matrix
};

typedef struct MAV_MULTI_CORE_STRUCT
{
    MINT32 CoreIdx;     // core index
    MINT32 CoreNum;     // core number
    MINT32 FuncType;    // 0:FE, 1:FM
    MINT32 ImageIdx;    // image index
    MUINT32 ImageAddr;  // for FE
    MINT32 MotionX;     // for FM
    MINT32 MotionY;     // for FM
} MAV_MULTI_CORE_STRUCT, *P_MAV_MULTI_CORE_STRUCT;

/*******************************************************************************
*
********************************************************************************/
class MTKMav {
public:
    static MTKMav* createInstance(DrvMavObject_e eobject);
    virtual void   destroyInstance() = 0;
       
    virtual ~MTKMav(){}
    // Process Control
    virtual MRESULT MavInit(void* InitInData, void* InitOutData);   // Env/Cb setting
    virtual MRESULT MavMain(void);                  // START
    virtual MRESULT MavReset(void);                 // RESET
    virtual MRESULT MavMerge(MUINT32 *MavResult);   // MERGE
            
    // Feature Control        
    virtual MRESULT MavFeatureCtrl(MUINT32 FeatureID, void* pParaIn, void* pParaOut);
private:
    
};

class AppMavTmp : public MTKMav {
public:
    //
    static MTKMav* getInstance();
    virtual void destroyInstance();
    //
    AppMavTmp() {}; 
    virtual ~AppMavTmp() {};
};

#endif



