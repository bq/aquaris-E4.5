
#ifndef _MTK_HAL_CAMADAPTER_MTKENG_INC_CAPTURECMDQUETHREAD_H_
#define _MTK_HAL_CAMADAPTER_MTKENG_INC_CAPTURECMDQUETHREAD_H_
//
#include <utils/threads.h>
#include <utils/StrongPointer.h>
//


namespace android {
namespace NSMtkEngCamAdapter {


/*******************************************************************************
*   ICaptureCmdQueThreadHandler
*******************************************************************************/
class ICaptureCmdQueThreadHandler : public virtual RefBase
{
public:     ////        Instantiation.
    virtual             ~ICaptureCmdQueThreadHandler() {}

public:     ////        Interfaces

    // Derived class must implement the below function. The thread starts its
    // life here. There are two ways of using the Thread object:
    // 1) loop: if this function returns true, it will be called again if 
    //          requestExit() wasn't called.
    // 2) once: if this function returns false, the thread will exit upon return.
    virtual bool        onCaptureThreadLoop()               = 0;

};


/*******************************************************************************
*   ICaptureCmdQueThread
*******************************************************************************/
class ICaptureCmdQueThread : public Thread
{
public:     ////        Instantiation.
    static  ICaptureCmdQueThread*   createInstance(ICaptureCmdQueThreadHandler*const pHandler);
    //
public:     ////        Attributes.
    virtual int32_t     getTid() const                      = 0;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Commands.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////        Interfaces.

    virtual status_t    onCapture()                         = 0;

};


}; // namespace NSMtkEngCamAdapter
}; // namespace android
#endif  //_MTK_HAL_CAMADAPTER_MTKENG_INC_CAPTURECMDQUETHREAD_H_



