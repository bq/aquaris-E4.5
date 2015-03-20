
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

#ifndef _APP_MAV_H
#define _APP_MAV_H

#include "MTKMav.h"

/*****************************************************************************
	Class Define
******************************************************************************/
class AppMav : public MTKMav {
public:    
    static MTKMav* getInstance(DrvMavObject_e eobject);
    virtual void destroyInstance();
    
    AppMav();
    virtual ~AppMav();   
    // Process Control
    MRESULT MavInit(void* InitInData, void* InitOutData);
    MRESULT MavMain(void);					// START
    MRESULT MavReset(void);					// RESET
    MRESULT MavMerge(MUINT32 *MavResult);	// MERGE        
    static void* MavThreadFunc(void *ParaIn);

	// Feature Control        
	MRESULT MavFeatureCtrl(MUINT32 FeatureID, void* pParaIn, void* pParaOut);	
private:
   

};

#endif



