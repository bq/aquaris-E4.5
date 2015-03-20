
/********************************************************************************************
 *     LEGAL DISCLAIMER
 *
 *     (Header of MediaTek Software/Firmware Release or Documentation)
 *
 *     BY OPENING OR USING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 *     THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE") RECEIVED
 *     FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON AN "AS-IS" BASIS
 *     ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED,
 *     INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR
 *     A PARTICULAR PURPOSE OR NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY
 *     WHATSOEVER WITH RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 *     INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK
 *     ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
 *     NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S SPECIFICATION
 *     OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
 *
 *     BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE LIABILITY WITH
 *     RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION,
TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE
 *     FEES OR SERVICE CHARGE PAID BY BUYER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 *     THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE WITH THE LAWS
 *     OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF LAWS PRINCIPLES.
 ************************************************************************************************/
#ifndef _MTK_EIS_H
#define _MTK_EIS_H

#include "MTKEisType.h"
#include "MTKEisErrCode.h"

//#define EIS_DEBUG
//#define PC_SIM

#define EIS_WIN_NUM 32

#define EIS_MAX_LOG_FRAME 500
#define EIS_LOG_BUFFER_SIZE (1500*EIS_MAX_LOG_FRAME) //record 500 frames info. (Each frame needs 1K bytes)

typedef enum
{
    EIS_STATE_STANDBY,
    EIS_STATE_INIT,
    EIS_STATE_PROC,
    EIS_STATE_FINISH,
    EIS_STATE_IDLE
}EIS_STATE_ENUM;


typedef enum
{
	EIS_PATH_RAW_DOMAIN,
	EIS_PATH_YUV_DOMAIN
}EIS_INPUT_PATH_ENUM;


typedef enum
{
	EIS_SENSI_LEVEL_HIGH = 0,
	EIS_SENSI_LEVEL_NORMAL = 1,
	EIS_SENSI_LEVEL_ADVTUNE = 2
} EIS_SENSITIVITY_ENUM;


typedef enum
{
    ABSOLUTE_HIST_METHOD,
    SMOOTH_HIST_METHOD    
} EIS_VOTE_METHOD_ENUM;


//IMPORTANTAT! - Do not modify the adv. tuning parameters at will
typedef struct
{
    MUINT32 new_tru_th; // 0~100
    MUINT32 vot_th;      // 1~16
    MUINT32 votb_enlarge_size;  // 0~1280
    MUINT32 min_s_th; // 10~100
    MUINT32 vec_th;   // 0~11   should be even
    MUINT32 spr_offset; //0 ~ MarginX/2
    MUINT32 spr_gain1; // 0~127
    MUINT32 spr_gain2; // 0~127
    MUINT32 gmv_pan_array[4];           //0~5
    MUINT32 gmv_sm_array[4];            //0~5
    MUINT32 cmv_pan_array[4];           //0~5
    MUINT32 cmv_sm_array[4];            //0~5
    
    EIS_VOTE_METHOD_ENUM vot_his_method; //0 or 1
    MUINT32 smooth_his_step; // 2~6
    MUINT32 eis_debug;
}EIS_ADV_TUNING_PARA_STRUCT, *P_EIS_ADV_TUNING_PARA_STRUCT;


typedef struct
{
    EIS_SENSITIVITY_ENUM sensitivity;          // 0 or 1 or 2
    MUINT32 filter_small_motion;      // 0 or 1

    EIS_ADV_TUNING_PARA_STRUCT advtuning_data;

}EIS_TUNING_PARA_STRUCT, *P_EIS_TUNING_PARA_STRUCT;


typedef struct
{
	EIS_TUNING_PARA_STRUCT eis_tuning_data;
	EIS_INPUT_PATH_ENUM Eis_Input_Path;    
} EIS_SET_ENV_INFO_STRUCT, *P_EIS_SET_ENV_INFO_STRUCT;

typedef struct
{
	MINT32 CMV_X;
	MINT32 CMV_Y;                         
}EIS_RESULT_INFO_STRUCT, *P_EIS_RESULT_INFO_STRUCT;


typedef struct
{
    MINT32 i4LMV_X[EIS_WIN_NUM];
    MINT32 i4LMV_Y[EIS_WIN_NUM];

    MINT32 i4LMV_X2[EIS_WIN_NUM];
    MINT32 i4LMV_Y2[EIS_WIN_NUM];

    MUINT32 NewTrust_X[EIS_WIN_NUM];
    MUINT32 NewTrust_Y[EIS_WIN_NUM];

    MUINT32 SAD[EIS_WIN_NUM];
    MUINT32 SAD2[EIS_WIN_NUM];    
    MUINT32 AVG_SAD[EIS_WIN_NUM];
} EIS_STATISTIC_STRUCT;


typedef struct
{
	MINT32 EIS_GMVx;
	MINT32 EIS_GMVy;
}EIS_GMV_INFO_STRUCT, *P_EIS_GMV_INFO_STRUCT;

/*
typedef struct
{
	MINT32 Div_H;
	MINT32 Div_W;
	MUINT32 RpWinNum;
}EIS_CONFIG_INFO_STRUCT;
*/

typedef struct
{
	MUINT32 InputWidth;
	MUINT32 InputHeight;
	MUINT32 TargetWidth;
	MUINT32 TargetHeight;
}EIS_CONFIG_IMAGE_INFO_STRUCT, *P_EIS_CONFIG_IMAGE_INFO_STRUCT;


typedef struct
{
	EIS_STATISTIC_STRUCT eis_state;
	//EIS_CONFIG_INFO_STRUCT eis_config;
	EIS_CONFIG_IMAGE_INFO_STRUCT eis_image_size_config;
}EIS_SET_PROC_INFO_STRUCT, *P_EIS_SET_PROC_INFO_STRUCT;


typedef enum
{
	EIS_FEATURE_BEGIN = 0,

	EIS_FEATURE_SET_PROC_INFO,
	EIS_FEATURE_GET_PROC_INFO,
	EIS_FEATURE_SET_DEBUG_INFO,
	EIS_FEATURE_GET_EIS_STATE,
	EIS_FEATURE_SAVE_LOG,
	EIS_FEATURE_GET_ORI_GMV,

	EIS_FEATURE_MAX

}	EIS_FEATURE_ENUM;


typedef struct
{
    MBOOL  pathCDRZ;    // 0 : before, 1 : after
    MINT32 IIR_DS;  // 1 or 2 or 4
    MINT32 RPNum_H; // 1~16
    MINT32 RPNum_V; // 1~8
    MINT32 MBNum_H; // 1~4
    MINT32 MBNum_V; // 1~8
    MINT32 AD_Knee;
    MINT32 AD_Clip;
    MINT32 Gain_H;  // horizaontal gain control, 0~3
    MINT32 IIR_Gain_H;  // 3 or 4
    MINT32 IIR_Gain_V;  // 3 or 4
    MINT32 FIR_Gain_H;  // 16 or 32
    MINT32 LMV_TH_X_Cent;
    MINT32 LMV_TH_X_Surrd;
    MINT32 LMV_TH_Y_Cent;
    MINT32 LMV_TH_Y_Surrd;
    MINT32 FL_Offset_H; // -47~48
    MINT32 FL_Offset_V; // -64~65
    MINT32 MB_Offset_H;
    MINT32 MB_Offset_V;
    MINT32 MB_Intv_H;
    MINT32 MB_Intv_V;
}EIS_GET_PROC_INFO_STRUCT, *P_EIS_GET_PROC_INFO_STRUCT;


typedef struct  
{
    MUINT32 FrameNum;
    EIS_STATISTIC_STRUCT sEIS_Ori_Stat;   // 900 bytes

    MINT32 u4GMVx;
    MINT32 u4GMVy;
    MINT32 u4CMVx;
    MINT32 u4CMVy;

    MINT32 u4SmoothGMVx;
    MINT32 u4SmoothGMVy;

    MINT32 u4SmoothCMVx;
    MINT32 u4SmoothCMVy;

    MUINT32 u4WeightX[EIS_WIN_NUM];
    MUINT32 u4WeightY[EIS_WIN_NUM];

    MUINT16 u4VoteIndeX;
    MUINT16 u4VoteIndeY;
    
    
} EIS_DEBUG_TAG_STRUCT, *P_EIS_DEBUG_TAG_STRUCT;


typedef struct
{
    MUINT32 Eis_Log_Buf_Addr;
    MUINT32 Eis_Log_Buf_Size;
}EIS_SET_LOG_BUFFER_STRUCT, *P_EIS_SET_LOG_BUFFER_STRUCT;


class MTKEis
{
public:
    static MTKEis* createInstance();
    virtual void   destroyInstance() = 0;
       
    virtual ~MTKEis(){};
    // Process Control
	virtual MRESULT EisInit(void* InitInData);
    virtual MRESULT EisMain(EIS_RESULT_INFO_STRUCT *EisResult);	// START
    virtual MRESULT EisReset();   //Reset
            
	// Feature Control        
	virtual MRESULT EisFeatureCtrl(MUINT32 FeatureID, void* pParaIn, void* pParaOut);
private:
    
};


#endif


