
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

#ifndef _APP_MAV_MOTION_H
#define _APP_MAV_MOTION_H

#include "MTKMotionType.h"
#include "MTKMotion.h"

/*****************************************************************************
	Class Define
******************************************************************************/
class AppMavMotion : public MTKMotion {
public:    
    static MTKMotion* getInstance();
    virtual void destroyInstance();
    
    AppMavMotion();
    virtual ~AppMavMotion();   
    // Process Control
    MRESULT MotionInit(void *InitInData, void *InitOutData);
    MRESULT MotionMain(void);					// START
    MRESULT MotionExit(void);					// EXIT

	// Feature Control        
	MRESULT MotionFeatureCtrl(MUINT32 FeatureID, void* pParaIn, void* pParaOut);	
private:
   

};

#endif



