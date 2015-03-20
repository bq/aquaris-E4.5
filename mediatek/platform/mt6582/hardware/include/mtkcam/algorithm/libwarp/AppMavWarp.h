
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

#ifndef _APP_MAV_WARP_H
#define _APP_MAV_WARP_H

#include "MTKWarp.h"

#ifndef SIM_MAIN
#include "EGLUtils.h"
#include <EGL/egl.h>
#endif
    
/*****************************************************************************
	Class Define
******************************************************************************/
class AppMavWarp : public MTKWarp {
public:    
    static MTKWarp* getInstance();
    virtual void destroyInstance();
    
    AppMavWarp();
    virtual ~AppMavWarp();   
    // Process Control
    MRESULT WarpInit(MUINT32 *InitInData, MUINT32 *InitOutData);
    MRESULT WarpMain(void);					// START
    MRESULT WarpReset(void);					// RESET

	// Feature Control        
	MRESULT WarpFeatureCtrl(MUINT32 FeatureID, void* pParaIn, void* pParaOut);	
private:
   
#ifndef SIM_MAIN
    EGLDisplay eglDisplay;
    EGLConfig  eglConfig;
    EGLSurface eglSurface;
    EGLContext eglContext;
    EGLint     majorVersion;
    EGLint     minorVersion;
#endif
};

#endif



