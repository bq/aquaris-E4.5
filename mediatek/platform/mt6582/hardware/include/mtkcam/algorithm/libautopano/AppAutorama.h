
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

#ifndef _APP_AUTORAMA_H
#define _APP_AUTORAMA_H

#include "MTKAutoramaType.h"
#include "MTKAutorama.h"

/*****************************************************************************
	Class Define
******************************************************************************/
class AppAutorama : public MTKAutorama {
public:    
    static MTKAutorama* getInstance();
    virtual void destroyInstance();
    
    AppAutorama();
    virtual ~AppAutorama();   
    // Process Control
    MRESULT AutoramaInit(void *InitInData, void *InitOutData);
    MRESULT AutoramaMain(void);					// START
    MRESULT AutoramaExit(void);					// EXIT

	// Feature Control        
	MRESULT AutoramaFeatureCtrl(MUINT32 FeatureID, void* pParaIn, void* pParaOut);	
private:
    MTKAUTORAMA_STATE_ENUM m_AutoramaState;
    MTKAUTORAMA_CTRL_ENUM m_CtrlEnum;
    MTKAutoramaEnvInfo m_AutoramaEnvInfo;

    MUINT32 m_AutoramaCurrProcNum;
    MUINT32 m_AutoramaCurrImageNum;
    MUINT32 m_ProcessCount;


};

#endif



