
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

#ifndef _APP_PANO3D_H
#define _APP_PANO3D_H

#include "MTKPano3DType.h"
#include "MTKPano3D.h"

/*****************************************************************************
    Class Define
******************************************************************************/
class AppPano3D : public MTKPano3D {
public:    
    static MTKPano3D* getInstance();
    virtual void destroyInstance();
    
    AppPano3D();
    virtual ~AppPano3D();   
    // Process Control
    MRESULT Pano3DInit(void *InitInData, void *InitOutData);
    MRESULT Pano3DMain(void);                   // START
    MRESULT Pano3DReset(void);                  // RESET

    // Feature Control        
    MRESULT Pano3DFeatureCtrl(MUINT32 FeatureID, void* pParaIn, void* pParaOut);    
private:
   

};

#endif



