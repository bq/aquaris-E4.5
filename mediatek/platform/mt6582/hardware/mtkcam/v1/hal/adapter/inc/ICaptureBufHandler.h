
#ifndef _MTK_HAL_CAMADAPTER_INC_ICAPTUREBUFHANDLER_H_
#define _MTK_HAL_CAMADAPTER_INC_ICAPTUREBUFHANDLER_H_
//

namespace android {

typedef struct {
    ImgBufQueNode mainImgNode;
    ImgBufQueNode subImgNode;
	
	uint32_t 	u4FocusValH;
	uint32_t  	u4FocusValL;
} CapBufQueNode;
/******************************************************************************
*
*******************************************************************************/
class ICaptureBufMgrHandler : public virtual RefBase
{
public:
    virtual                ~ICaptureBufMgrHandler() {}

public:
    virtual bool            dequeProvider(CapBufQueNode& rNode) = 0;
    virtual bool            dequeProvider(list<CapBufQueNode>* pvNode) = 0;
    virtual bool            enqueProvider(CapBufQueNode& rNode, bool bIsFilled) = 0;
    virtual bool            enqueProvider(unsigned int va, bool bIsFilled) = 0;

    virtual bool            dequeProcessor(CapBufQueNode& rNode, int index) = 0;
    virtual bool            enqueProcessor(CapBufQueNode& rNode) = 0;

    virtual void            allocBuffer(int w, int h, const char* format, int rotation,
                                        int _w, int _h, const char* _format, int cnt) = 0;

    virtual void            reallocBuffer(int w, int h, const char* format,
                                           int _w, int _h, const char* _format, int cnt) = 0;

    virtual void            freeBuffer() = 0;

    virtual int32_t         getStoredBufferCnt() = 0;
    virtual void            setStoredBufferCnt(int32_t const cnt) = 0;


};
};
#endif



