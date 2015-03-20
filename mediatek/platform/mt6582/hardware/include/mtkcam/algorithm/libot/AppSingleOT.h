
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

#ifndef _APP_SINGLE_OT_H
#define _APP_SINGLE_OT_H

#include <mtkcam/algorithm/libot/MTKOT.h>


typedef enum
{
	MTKOT_STATE_STANDBY=0,	                          
    MTKOT_STATE_READY_TO_SET_PROC_INFO,		    
    MTKOT_STATE_READY_TO_MAIN,		    
    MTKOT_STATE_READY_TO_GETRESULT,		              
} MTKOT_STATE_ENUM;




class AppSingleOT  : public MTKOT {
public:
    static MTKOT* getInstance();
    virtual void  destroyInstance();
       
    AppSingleOT();
    virtual ~AppSingleOT();
    // Process Control
    virtual MRESULT OTInit(void *InitInData, void *InitOutData);	
    virtual MRESULT OTMain(void);					
    virtual MRESULT OTExit(void);					

	// Feature Control        
	virtual MRESULT OTFeatureCtrl(MUINT32 FeatureID, void* pParaIn, void* pParaOut);
private:
    
    MTKOT_STATE_ENUM m_SingleOTState;
    MTKOTEnvInfo m_SingleOTEnvInfo;
};

#endif


