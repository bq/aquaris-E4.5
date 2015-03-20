
#ifndef _MTK_HAL_CAMCLIENT_RECORD_RECORDCLIENT_H_
#define _MTK_HAL_CAMCLIENT_RECORD_RECORDCLIENT_H_
//
#include <CamUtils.h>
#include <mtkcam/v1/ExtImgProc/IExtImgProc.h>
#include <mtkcam/v1/ExtImgProc/ExtImgProc.h>
using namespace android;
using namespace MtkCamUtils;
#include <mtkcam/v1/IParamsManager.h>
#include <mtkcam/v1/client/IRecordClient.h>


namespace android {
namespace NSCamClient {
namespace NSRecordClient {
/******************************************************************************
 *   
 ******************************************************************************/
class RecBufManager;


/******************************************************************************
 *  Record Client Handler.
 ******************************************************************************/
class RecordClient : public IRecordClient
                    , public Thread
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Instantiation.
    //
                                    RecordClient(sp<IParamsManager> pParamsMgr);
    virtual                         ~RecordClient();

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////

    virtual bool                    init();
    virtual bool                    uninit();

    virtual bool                    setImgBufProviderClient(
                                        sp<IImgBufProviderClient>const& rpClient
                                    );
    //
    //
    virtual void                    setCallbacks(sp<CamMsgCbInfo> const& rpCamMsgCbInfo);
    //
    virtual void                    enableMsgType(int32_t msgType);
    virtual void                    disableMsgType(int32_t msgType);
    //
    virtual bool                    startRecording();
    virtual bool                    stopRecording();
    virtual void                    releaseRecordingFrame(const void *opaque);

    //
    virtual status_t                dump(int fd, Vector<String8>& args);

    //
    virtual status_t                sendCommand(
                                        int32_t cmd,
                                        int32_t arg1,
                                        int32_t arg2);

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Operations in base class Thread
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////
    // Ask this object's thread to exit. This function is asynchronous, when the
    // function returns the thread might still be running. Of course, this
    // function can be called from a different thread.
    virtual void                    requestExit();

    // Good place to do one-time initializations
    virtual status_t                readyToRun();

private:
    // Derived class must implement threadLoop(). The thread starts its life
    // here. There are two ways of using the Thread object:
    // 1) loop: if threadLoop() returns true, it will be called again if
    //          requestExit() wasn't called.
    // 2) once: if threadLoop() returns false, the thread will exit upon return.
    virtual bool                    threadLoop();

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Command Queue.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////                    Definitions.
                                    struct Command
                                    {
                                        //  Command ID.
                                        enum EID
                                        {
                                            eID_EXIT,
                                            eID_WAKEUP
                                        };
                                        //
                                        //  Operations.
                                        Command(EID const _eId = eID_WAKEUP)
                                            : eId(_eId)
                                        {}
                                        //
                                        static  char const* getName(EID const _eId);
                                        inline  char const* name() const    { return getName(eId); }
                                        //
                                        //  Data Members.
                                        EID     eId;
                                    };

protected:  ////                    Operations.

    virtual void                    postCommand(Command const& rCmd);
    virtual bool                    getCommand(Command& rCmd);
    //
    virtual void                    onClientThreadLoop(Command const& rCmd);
    //
    inline  int32_t                 getThreadId() const    { return mi4ThreadId; }

protected:  ////                    Data Members.
    List<Command>                   mCmdQue;
    Mutex                           mCmdQueMtx;
    Condition                       mCmdQueCond;    //  Condition to wait: [ ! exitPending() && mCmdQue.empty() ]
    int32_t                         mi4ThreadId;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////                    

    virtual bool                    isEnabledState() const;
    bool                            isMsgEnabled();
    bool                            onStateChanged();

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
protected:  ////                    Common Info.
    mutable Mutex                   mModuleMtx;
    sp<CamMsgCbInfo>                mpCamMsgCbInfo;         //  Pointer to Camera Message-Callback Info.
    sp<IParamsManager>              mpParamsMgr;            //  Pointer to Parameters Manager.
    volatile int32_t                mIsMsgEnabled;          //  Message Enabled ?
    volatile int32_t                mIsRecStarted;          //  Record Started ?
    int32_t                         mi4RecWidth;            //  Record Width.
    int32_t                         mi4RecHeight;           //  Record Height.

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
protected:  ////                    Callback.
    //
    int32_t                         mi4CallbackRefCount;    //  Record callback reference count.
    int64_t                         mi8CallbackTimeInMs;    //  The timestamp in millisecond of last preview callback.
    //

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Image Buffer
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////                    Definitions.
                                    struct ImgBufNode
                                    {
                                        sp<ICameraImgBuf>   mpCameraImgBuf;
                                        //
                                        ImgBufNode(sp<ICameraImgBuf>const& pCameraImgBuf = NULL)
                                            : mpCameraImgBuf(pCameraImgBuf)
                                        {}
                                        //
                                        sp<ICameraImgBuf>const& getImgBuf() const   { return mpCameraImgBuf; }
                                        sp<ICameraImgBuf>&      getImgBuf()         { return mpCameraImgBuf; }
                                    };
                                    //
    typedef List<ImgBufNode>        ImgBufList_t;
    enum                            { eMAX_RECORD_BUFFER_NUM = 10 };
    //
protected:  ////                    Data Members.
    uint_t                          muImgBufIdx;            //  index for ring buffer of mpImgBufMgr.
    sp<RecBufManager>               mpImgBufMgr;
    sp<IImgBufQueue>                mpImgBufQueue;
    //  Pointer to the client of Image Buffer Provider (i.e. a client is a provider-IF user of mpImgBufQueue).
    sp<IImgBufProviderClient>       mpImgBufPvdrClient;

protected:  ////                    Operations.
    //
    bool                            initBuffers();
    void                            uninitBuffers();
    //
    bool                            prepareAllTodoBuffers(sp<IImgBufQueue>const& rpBufQueue, sp<RecBufManager>const& rpBufMgr);
    bool                            cancelAllUnreturnBuffers();
    bool                            waitAndHandleReturnBuffers(sp<IImgBufQueue>const& rpBufQueue);
    bool                            handleReturnBuffers(Vector<ImgBufQueNode>const& rvQueNode);
    bool                            performRecordCallback(int32_t bufIdx, sp<ICameraImgBuf>const& pCameraImgBuf, int32_t const msgType);

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  dump
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////                
    mutable Mutex                   mDumpMtx;
    mutable Mutex                   mBufferMtx;
    Condition                       mBufferCond;
    volatile int32_t                mbForceReleaseBuf;
    //
    typedef enum
    {
        REC_BUF_STA_EMPTY,
        REC_BUF_STA_ENQUE,
        REC_BUF_STA_FILL,
        REC_BUF_STA_CB,
        REC_BUF_STA_AMOUNT
    }REC_BUF_STA_ENUM;
    //
    typedef struct
    {
        //REC_BUF_STA_ENUM    Sta;
        uint32_t            Sta;
        void*               VirAddr;
    }REC_BUF_INFO_STRUCT;
    //
    int32_t                         mi4DumpImgBufCount;
    String8                         ms8DumpImgBufPath;
    int32_t                         mi4DumpImgBufIndex;
    Vector<REC_BUF_INFO_STRUCT>     mvRecBufInfo;
    volatile nsecs_t                mTimeStart;
    volatile nsecs_t                mTimeEnd;
    volatile uint32_t               mFrameCount;
    nsecs_t                         mLastTimeStamp;
    //
    int32_t                         mi4BufSecu;
    int32_t                         mi4BufCohe;
    //
    volatile bool                   mbThreadExit;
    //
    ExtImgProc*                     mpExtImgProc;
};


}; // namespace NSRecordClient
}; // namespace NSCamClient
}; // namespace android
#endif  //_MTK_HAL_CAMCLIENT_RECORD_RECORDCLIENT_H_



