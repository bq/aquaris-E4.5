
#ifndef _MTK_HAL_MTKATVCAMADAPTER_DISPLAYDELAYTHREAD_H_
#define _MTK_HAL_MTKATVCAMADAPTER_DISPLAYDELAYTHREAD_H_
//
#include <utils/threads.h>
#include <utils/StrongPointer.h>

//
//
namespace android {
namespace NSMtkAtvCamAdapter {
/*******************************************************************************
*	Command
*******************************************************************************/
struct Command
{
	//	Command ID.
	enum EID
	{
		eID_EXIT, 
		eID_DISPLAY_FRAME, 
	};
	//
	//	Operations.
	Command(EID const _eId = eID_DISPLAY_FRAME)
		: eId(_eId)
	{}
	//
	static	char const* getCmdName(EID const _eId);
	inline	char const* name() const	{ return getCmdName(eId); }
	//
	//	Data Members.
	EID 	eId;
};
	
//
/*******************************************************************************
*   IDisplayThreadHandler
*******************************************************************************/
class IDisplayDelayThreadHandler : public virtual RefBase
{
public:     ////        Instantiation.
    virtual             ~IDisplayDelayThreadHandler() {}

public:     ////        Interfaces
    virtual bool        delayDisplay()           = 0;

};

/*******************************************************************************
*   IDisplayDelayThread
*******************************************************************************/
class IDisplayDelayThread : public Thread
{
public:     ////        Instantiation.
    static  IDisplayDelayThread* createInstance(IDisplayDelayThreadHandler*const pHandler);
    //
public:     ////        Attributes.
    virtual int32_t     getTid() const                              = 0;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Commands.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////        Interfaces.
    /*
     */
    virtual void        postCommand(Command const& rCmd)            = 0;

};

};  // namespace NSMtkAtvCamAdapter
};  // namespace android

#endif  //_MTK_HAL_MTKATVCAMADAPTER_DISPLAYDELAYTHREAD_H_

