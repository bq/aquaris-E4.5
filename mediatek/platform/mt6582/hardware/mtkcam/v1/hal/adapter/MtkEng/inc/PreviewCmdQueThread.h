
#ifndef PREVIEW_CMDQ_THREAD_H
#define PREVIEW_CMDQ_THREAD_H
//
#include <utils/threads.h>
#include <utils/RefBase.h>
#include <semaphore.h>
#include <inc/IPreviewBufMgr.h>
//
namespace android {
namespace NSMtkEngCamAdapter {
/******************************************************************************
*
*******************************************************************************/
class PrvCmdCookie : public virtual RefBase
{

public:
    enum ECmdType{
         eStart,               //prvStart()
         eDelay,               //prvDelay()
         eUpdate,              //prvUpdate()
         ePrecap,              //prvPrecap()
         eStop,                //prvStop()
         eExit,                
    };
    ///
    enum ESemWait{
        eSemNone    = 0,
        eSemBefore  = 0x01,
        eSemAfter   = 0x02,
    };
    ///
    
public:
    PrvCmdCookie(ECmdType _eType, ESemWait _eSem)
        : eType(_eType)
        , bsemBefore(false)
        , bsemAfter(false)
        , bvalid(true)
    {
        if (_eSem & eSemBefore)
        {
            bsemBefore = true;
            sem_init(&semBefore, 0, 0); 
        }
        if (_eSem & eSemAfter)
        {
            bsemAfter = true;
            sem_init(&semAfter, 0, 0);  
        } 
    }
    
    void waitSem(){
        if (isSemBefore()){
            sem_wait(&semBefore); 
        }
        if (isSemAfter()){
            sem_wait(&semAfter);
        }
    }
    
    void postSem(ESemWait _eSem){
        if ((_eSem & eSemBefore) && isSemBefore()) 
        {
            sem_post(&semBefore); 
        }
        if ((_eSem & eSemAfter) && isSemAfter())
        {
            sem_post(&semAfter); 
        }
    }

    ECmdType getCmd() const { return eType; }
    bool isValid() const { return bvalid;}
    void setValid(bool in) { bvalid = in;}
/////
private:
    bool isSemBefore() const {return bsemBefore;}
    bool isSemAfter()  const {return bsemAfter;}
    

private:
    ECmdType eType;
    sem_t semBefore;
    sem_t semAfter;
    bool  bsemBefore;
    bool  bsemAfter;
    bool  bvalid;
};



/******************************************************************************
*
*******************************************************************************/
class IPreviewCmdQueThread : public Thread 
{
public:
    static IPreviewCmdQueThread*    createInstance(
                                        sp<IPreviewBufMgrHandler> pHandler, 
                                        int const& rSensorid,
                                        sp<IParamsManager> pParamsMgr
                                    );
public:
    virtual int  getTid() const  = 0;
    virtual bool postCommand(PrvCmdCookie::ECmdType const rcmdType, 
                             PrvCmdCookie::ESemWait const rSemWait = PrvCmdCookie::eSemNone)= 0;
    virtual bool setParameters() = 0;
    virtual bool setZoom(uint32_t zoomValue) = 0;
    virtual void setCallbacks(sp<CamMsgCbInfo> const& rpCamMsgCbInfo) = 0;
};

}; // namespace NSMtkEngCamAdapter
}; // end of namespace
#endif



