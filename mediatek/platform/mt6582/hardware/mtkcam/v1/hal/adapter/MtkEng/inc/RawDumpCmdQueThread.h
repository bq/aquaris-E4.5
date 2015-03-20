
#ifndef RAW_DUMP_CMDQ_THREAD_H
#define RAW_DUMP_CMDQ_THREAD_H
//
#include <utils/threads.h>
#include <utils/RefBase.h>
#include <semaphore.h>
#include <inc/IPreviewBufMgr.h>
//


namespace android {
namespace NSMtkEngCamAdapter {

#define BUFCNT      (75)

/******************************************************************************
*
*******************************************************************************/
class RawDumpCmdCookie : public virtual RefBase
{
public:
    RawDumpCmdCookie(MUINT32 frame_count, MUINT32 slot_index, MUINT32 buf_size)
        : mFrameCnt (frame_count)
        , mSlotIndex(slot_index)
        , mBufSize(buf_size)        
    {
    }

    MUINT32 getSlotIndex() const { return mSlotIndex; }
    MUINT32 getFrameCnt() const { return mFrameCnt; }
    MUINT32 getBufSize() const { return mBufSize; }
private:
    MUINT32 mFrameCnt;
    MUINT32 mSlotIndex;
    MUINT32 mBufSize;
};


/******************************************************************************
*
*******************************************************************************/
class IRawDumpCmdQueThread : public Thread 
{
public:
    static IRawDumpCmdQueThread*    createInstance(MUINT32 mem_out_width, MUINT32 mem_out_height, MUINT32 bitOrder, MUINT32 bitDepth, sp<IParamsManager> pParamsMgr);
public:
    virtual int  getTid() const  = 0;
    virtual bool postCommand(MUINT32 buf_addr, MUINT32 buf_size)= 0;
    virtual void setCallbacks(sp<CamMsgCbInfo> const& rpCamMsgCbInfo) = 0;
};

}; // namespace NSMtkEngCamAdapter
}; // end of namespace
#endif



