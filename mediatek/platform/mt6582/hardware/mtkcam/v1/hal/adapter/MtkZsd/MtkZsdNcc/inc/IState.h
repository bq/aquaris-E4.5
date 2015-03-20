
#ifndef _MTK_HAL_CAMADAPTER_MTKZSDNCC_INC_ISTATE_H_
#define _MTK_HAL_CAMADAPTER_MTKZSDNCC_INC_ISTATE_H_
//


/*******************************************************************************
*
*******************************************************************************/
namespace android {
namespace NSMtkZsdNccCamAdapter {


/*******************************************************************************
*   IStateHandler
*******************************************************************************/
class IStateHandler
{
public:     ////            Operations.
    virtual                 ~IStateHandler() {}

public:     ////            Interfaces
    virtual status_t        onHandleStartPreview()                          = 0;
    virtual status_t        onHandleStopPreview()                           = 0;
    //
    virtual status_t        onHandlePreCapture()                            = 0;
    virtual status_t        onHandleCapture()                               = 0;
    virtual status_t        onHandleCaptureDone()                           = 0;
    virtual status_t        onHandleCancelCapture()                         = 0;
};


/*******************************************************************************
*   IState
*******************************************************************************/
class IState
{
public:     ////            State Enum.
                            enum ENState
                            {
                                eState_Idle, 
                                eState_Preview, 
                                eState_PreCapture, 
                                eState_Capture, 
                            };

public:     ////            Operations.
    virtual                 ~IState() {}
    virtual char const*     getName() const                                 = 0;
    virtual ENState         getEnum() const                                 = 0;
    //
public:     ////            Interfaces
    virtual status_t        onStartPreview(IStateHandler* pHandler)         = 0;
    virtual status_t        onStopPreview(IStateHandler* pHandler)          = 0;
    //
    virtual status_t        onPreCapture(IStateHandler* pHandler)           = 0;
    virtual status_t        onCapture(IStateHandler* pHandler)              = 0;
    virtual status_t        onCaptureDone(IStateHandler* pHandler)          = 0;
    virtual status_t        onCancelCapture(IStateHandler* pHandler)        = 0;

};


/*******************************************************************************
*   IStateManager
*******************************************************************************/
class IStateManager
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Definitions.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////            
    typedef IState::ENState ENState;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Observer
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////            
                            struct IObserver
                            {
                                virtual             ~IObserver() {}
                                virtual void        notify(ENState eNewState)   = 0;
                            };

                            class StateObserver : public IObserver
                            {
                            public:
                                virtual void        notify(ENState eNewState);
                                virtual status_t    waitState(ENState eState, nsecs_t const timeout = -1);
                                                    StateObserver(IStateManager*);
                                                    ~StateObserver();
                            protected:
                                IStateManager*const mpStateManager;
                                Mutex               mLock;
                                Condition           mCond;
                                ENState volatile    meCurrState;
                            };

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////            Attributes.
    virtual IState*         getCurrentState() const                         = 0;
    virtual bool            isState(ENState const eState)                   = 0;

public:     ////            Operations.
#if 0
    //
    //  eState:
    //      [in] the state to wait.
    //
    //  timeout:
    //      [in] the timeout to wait in nanoseconds. -1 indicates no timeout.
    //
    virtual status_t        waitState(
                                ENState const eState, 
                                nsecs_t const timeout = -1
                            )                                               = 0;
#endif
    virtual status_t        transitState(ENState const eNewState)           = 0;

    virtual bool            registerOneShotObserver(IObserver* pObserver)   = 0;
    virtual void            unregisterObserver(IObserver* pObserver)        = 0;

public:     ////            Instantiation.
    virtual                 ~IStateManager() {}

    static  IStateManager*  inst();

};


/*******************************************************************************
*
*******************************************************************************/
}; // namespace NSMtkZsdNccCamAdapter
}; // namespace android
#endif // _MTK_HAL_CAMADAPTER_MTKZSDNCC_INC_ISTATE_H_



