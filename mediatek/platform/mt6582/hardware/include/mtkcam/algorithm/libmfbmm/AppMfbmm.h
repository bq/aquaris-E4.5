
#ifndef _APP_MFBMM_H
#define _APP_MFBMM_H

#include "mtkmfbmm.h"

/*****************************************************************************
    Class Define
******************************************************************************/
class AppMfbmm : public MTKMfbmm {
public:    
    static MTKMfbmm* getInstance();
    virtual void destroyInstance();
    
    AppMfbmm();
    virtual ~AppMfbmm();   
    // Process Control
    MRESULT MfbmmInit(void* pParaIn, void* pParaOut);
    MRESULT MfbmmReset(void);
    MRESULT MfbmmMain(MFBMM_PROC_ENUM ProcId, void* pParaIn, void* pParaOut);
            
    // Feature Control        
    MRESULT MfbmmFeatureCtrl(MFBMM_FTCTRL_ENUM FcId, void* pParaIn, void* pParaOut);
    
private:
};


#endif



