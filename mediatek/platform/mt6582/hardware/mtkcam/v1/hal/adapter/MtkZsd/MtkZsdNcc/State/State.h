
#ifndef _MTK_HAL_CAMADAPTER_MTKZSDNCC_STATE_STATE_H_
#define _MTK_HAL_CAMADAPTER_MTKZSDNCC_STATE_STATE_H_
//
#include <utils/threads.h>


namespace android {
namespace NSMtkZsdNccCamAdapter {
/*******************************************************************************
*
*******************************************************************************/
class IState;
class IStateHandler;


/*******************************************************************************
*
*******************************************************************************/
class StateBase : public IState
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////        Interfaces
    virtual status_t    onStartPreview(IStateHandler* pHandler) { return op_UnSupport(__FUNCTION__); }
    virtual status_t    onStopPreview(IStateHandler* pHandler)  { return op_UnSupport(__FUNCTION__); }
    //
    virtual status_t    onPreCapture(IStateHandler* pHandler)   { return op_UnSupport(__FUNCTION__); }
    virtual status_t    onCapture(IStateHandler* pHandler)      { return op_UnSupport(__FUNCTION__); }
    virtual status_t    onCaptureDone(IStateHandler* pHandler)  { return op_UnSupport(__FUNCTION__); }
    virtual status_t    onCancelCapture(IStateHandler* pHandler){ return op_UnSupport(__FUNCTION__); }

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Implementation.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////        Operations.
                        StateBase(char const*const pcszName, ENState const eState);
    virtual char const* getName() const { return m_pcszName; }
    virtual ENState     getEnum() const { return m_eState; }
    IStateManager*      getStateManager() const { return m_pStateManager; }

protected:  ////        Data Members.
    char const*const    m_pcszName;
    ENState const       m_eState;
    IStateManager*const m_pStateManager;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
protected:  ////        Operations.
    status_t            waitState(ENState const eState, nsecs_t const timeout = -1);
    status_t            op_UnSupport(char const*const pcszDbgText = "");

};


/*******************************************************************************
*
*******************************************************************************/
struct StateIdle : public StateBase
{
                        StateIdle(ENState const eState);
    //
    virtual status_t    onStartPreview(IStateHandler* pHandler);
    virtual status_t    onCapture(IStateHandler* pHandler);
};


/*******************************************************************************
*
*******************************************************************************/
struct StatePreview : public StateBase
{
                        StatePreview(ENState const eState);
    //
    virtual status_t    onStopPreview(IStateHandler* pHandler);
    virtual status_t    onPreCapture(IStateHandler* pHandler);
};


/*******************************************************************************
*
*******************************************************************************/
struct StatePreCapture : public StateBase
{
                        StatePreCapture(ENState const eState);
    //
    virtual status_t    onStopPreview(IStateHandler* pHandler);
};


/*******************************************************************************
*
*******************************************************************************/
struct StateCapture : public StateBase
{
                        StateCapture(ENState const eState);
    //
    virtual status_t    onCaptureDone(IStateHandler* pHandler);
    virtual status_t    onCancelCapture(IStateHandler* pHandler);
};



/*******************************************************************************
*
*******************************************************************************/
}; // namespace NSMtkZsdNccCamAdapter
}; // namespace android
#endif // _MTK_HAL_CAMADAPTER_MTKZSDNCC_STATE_STATE_H_



