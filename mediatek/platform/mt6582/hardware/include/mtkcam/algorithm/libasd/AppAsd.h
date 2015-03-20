
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

#ifndef _APP_ASD_H
#define _APP_ASD_H

#include "MTKAsd.h"

/*****************************************************************************
	Class Define
******************************************************************************/
class AppAsd : public MTKAsd {
public:    
    static MTKAsd* getInstance();
    virtual void destroyInstance();
    
    AppAsd();
    virtual ~AppAsd();   
    // Process Control
    MRESULT AsdInit(void* InitInData, void* InitOutData);	// Env/Cb setting
    MRESULT AsdMain(ASD_PROC_ENUM ProcId, void* pParaIn);	// START
            
	// Feature Control        
	MRESULT AsdFeatureCtrl(MUINT32 FeatureID, void* pParaIn, void* pParaOut);

private:
   

};

#endif



