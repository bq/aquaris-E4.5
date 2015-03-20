
#define LOG_TAG "MtkCam/CamClient/RecordClient"
//
#include "RecordClient.h"
using namespace NSCamClient;
using namespace NSRecordClient;
//


/******************************************************************************
*
*******************************************************************************/
#define MY_LOGV(fmt, arg...)        CAM_LOGV("(%d)[%s] "fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_LOGD("(%d)[%s] "fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_LOGI("(%d)[%s] "fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_LOGW("(%d)[%s] "fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_LOGE("(%d)[%s] "fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_LOGA("(%d)[%s] "fmt, ::gettid(), __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_LOGF("(%d)[%s] "fmt, ::gettid(), __FUNCTION__, ##arg)
//
#define MY_LOGV_IF(cond, ...)       do { if ( (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( (cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( (cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)


/******************************************************************************
 *
 ******************************************************************************/
class TestThread : public Thread
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Operations in base class Thread
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
private:
    // Derived class must implement threadLoop(). The thread starts its life
    // here. There are two ways of using the Thread object:
    // 1) loop: if threadLoop() returns true, it will be called again if
    //          requestExit() wasn't called.
    // 2) once: if threadLoop() returns false, the thread will exit upon return.
    virtual bool        threadLoop()
                        {
                            ImgBufQueNode   QueNode;
                            //
                            for (int i = 0; i < miLoopCount; i++)
                            {
                                if  ( mpImgBufQueue->dequeProvider(QueNode) )
                                {
                                    renderImgBuf(QueNode.getImgBuf());
                                    //
                                    mpImgBufQueue->enqueProvider(ImgBufQueNode(QueNode.getImgBuf(), ImgBufQueNode::eSTATUS_DONE));
                                }
                                //
                                ::usleep(miSleepInUs);
                            }
                            return  false;
                        }
                        //
    void                renderImgBuf(sp<IImgBuf>const& pImgBuf)
                        {
                            char const aColor[] = {0, 128, 255};
                            static int idxColor = 0;
                            MY_LOGD("[TestThread::renderImgBuf]");

                            ::memset(pImgBuf->getVirAddr(), aColor[idxColor], pImgBuf->getBufSize());
                            idxColor = (idxColor+1) % (sizeof(aColor)/sizeof(aColor[0]));
                        }

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Implementation.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////        Instantiation.
                        TestThread(sp<IImgBufQueue> pImgBufQueue, int const iLoopCount = 30, int const iSleepInUs = 33333)
                            : mpImgBufQueue(pImgBufQueue)
                            , miLoopCount(iLoopCount)
                            , miSleepInUs(iSleepInUs)
                        {
                        }

protected:  ////
    sp<IImgBufQueue>    mpImgBufQueue;
    int                 miLoopCount;
    int                 miSleepInUs;
};


/******************************************************************************
 *
 ******************************************************************************/
status_t
RecordClient::
dump(int fd, Vector<String8>& args)
{
    if  ( args.empty() ) {
        MY_LOGW("empty arguments");
        return  OK;
    }
    //
    MY_LOGD("args(%d)=%s", args.size(), (*args.begin()).string());
    //
    //  Command: "testImgBufProcessor <loop count=30> <sleep in us=33333>"
    if  ( *args.begin() == "testImgBufProcessor" )
    {
        int iLoopCount = 30;
        int iSleepInUs = 33333;
        if  ( args.size() >= 2 ) {
            iLoopCount = ::atoi(args[1].string());
        }
        if  ( args.size() >= 3 ) {
            iSleepInUs = ::atoi(args[2].string());
        }
        //
        sp<Thread> pTestThread = new TestThread(mpImgBufQueue, iLoopCount, iSleepInUs);
        if  ( pTestThread != 0 )
        {
            pTestThread->run();
            pTestThread = 0;
        }
        return  OK;
    }
    //
    //  Command: "dumpImgBuf <count=1> <path=sdcard/DCIM/disp>"
    if  ( *args.begin() == "dumpImgBuf" )
    {
        int iCount = 1;    // always.
        String8 s8DumpPath("sdcard/DCIM/disp");
        //
        switch  ( args.size() )
        {
        case 3: //  "[command] <count=-1> <path=sdcard/DCIM/>"
            s8DumpPath.setPathName(args[2]);
        case 2: //  "[command] <count=-1>"
            iCount = ::atoi(args[1]);
        case 1: //  "[command]"
        default: {
            Mutex::Autolock _lock(mDumpMtx);
            ::android_atomic_write(iCount, &mi4DumpImgBufCount);
            ms8DumpImgBufPath = s8DumpPath;
            } break;
        }
        return  OK;
    }
    //
    //
    return  OK;
}



