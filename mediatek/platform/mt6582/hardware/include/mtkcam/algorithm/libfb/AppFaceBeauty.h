
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

#ifndef _APP_FACEBEAUTY_H
#define _APP_FACEBEAUTY_H

#include "MTKFaceBeautyType.h"
#include "MTKFaceBeauty.h"


typedef enum
{
	MTKFACEBEAUTY_STATE_STANDBY=0,	                           // After create Obj or Reset
    MTKFACEBEAUTY_STATE_READY_TO_SET_NR_PROC_INFO,
    MTKFACEBEAUTY_STATE_READY_TO_NR,
    MTKFACEBEAUTY_STATE_READY_TO_SET_ALIGNMENT_PROC_INFO,      // After call FaceBeautyInit()
	MTKFACEBEAUTY_STATE_READY_TO_FACE_ALIGN,			       // After set proc info with MTKFACEBEAUTY_CTRL_ALIGNMENT
    MTKFACEBEAUTY_STATE_READY_TO_SET_ALPHA_MAP_PROC_INFO,      // After get the result with MTKFACEBEAUTY_CTRL_ALIGNMENT
	MTKFACEBEAUTY_STATE_READY_TO_ALPHA_MAP,			           // After set proc info with MTKFACEBEAUTY_CTRL_ALPHA_MAP
	MTKFACEBEAUTY_STATE_READY_TO_SET_TEXTURE_BLEND_PROC_INFO,  // After get the result with MTKFACEBEAUTY_CTRL_ALPHA_MAP
    MTKFACEBEAUTY_STATE_READY_TO_TEXTURE_BLEND,			       // After set proc info with MTKFACEBEAUTY_CTRL_BLEND_TEXTURE_IMG
	MTKFACEBEAUTY_STATE_READY_TO_SET_COLOR_ADJUST_PROC_INFO,    // After get the result with MTKFACEBEAUTY_CTRL_BLEND_TEXTURE_IMG
    MTKFACEBEAUTY_STATE_READY_TO_COLOR_ADJUST,	               // After set proc info with MTKFACEBEAUTY_CTRL_ADJUST_COLOR_IMG
	MTKFACEBEAUTY_STATE_READY_TO_SET_WARPING_PROC_INFO,        // After get the result with MTKFACEBEAUTY_CTRL_ADJUST_COLOR_IMG
    MTKFACEBEAUTY_STATE_READY_TO_WARPING,                      // After set proc result with MTKFACEBEAUTY_CTRL_WARPING
    MTKFACEBEAUTY_STATE_READY_TO_GETRESULT,		               // After call FaceBeautyMain()
} MTKFACEBEAUTY_STATE_ENUM;

/*****************************************************************************
	Class Define
******************************************************************************/
class AppFaceBeauty : public MTKFaceBeauty {
public:    
    static MTKFaceBeauty* getInstance();
    virtual void destroyInstance();
    
    AppFaceBeauty();
    virtual ~AppFaceBeauty();   
    // Process Control
    MRESULT FaceBeautyInit(void *InitInData, void *InitOutData);
    MRESULT FaceBeautyMain(void);					// START
    MRESULT FaceBeautyExit(void);					// EXIT

	// Feature Control        
	MRESULT FaceBeautyFeatureCtrl(MUINT32 FeatureID, void* pParaIn, void* pParaOut);	
private:
    MTKFACEBEAUTY_STATE_ENUM m_FaceBeautyState;
    MTKFACEBEAUTY_CTRL_ENUM m_CtrlEnum;
    MTKFaceBeautyEnvInfo m_FaceBeautyEnvInfo;
};

#endif



