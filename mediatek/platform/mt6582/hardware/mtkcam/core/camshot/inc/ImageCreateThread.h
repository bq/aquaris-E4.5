
#ifndef _MTK_HAL_MTKATVCAMADAPTER_IMAGECREATETHREAD_H_
#define _MTK_HAL_MTKATVCAMADAPTER_IMAGECREATETHREAD_H_
//
#include <utils/threads.h>
#include <utils/RefBase.h>
#include <mtkcam/common.h>
using namespace android;

//
//
namespace NSCamShot {

/*******************************************************************************
*	Command
*******************************************************************************/
struct Command
{
	//	Command ID.
	enum EID
	{
		eID_EXIT, 
		eID_WAKEUP, 
		eID_YUV_BUF,
		eID_POSTVIEW_BUF, 
	};
	//
	//	Operations.
	Command(EID const _eId = eID_EXIT)
		: eId(_eId)
	{}
	//
	static	char const* getCmdName(EID const _eId)
	{
	
#define CMD_NAME(x) case x: return #x
		switch	(_eId)
		{
		CMD_NAME(eID_EXIT);
		CMD_NAME(eID_WAKEUP);
		CMD_NAME(eID_YUV_BUF);
		CMD_NAME(eID_POSTVIEW_BUF);
		default:
			break;
		}
#undef  CMD_NAME
		return	"";
	}
	inline	char const* name() const	{ return getCmdName(eId); }
	//
	//	Data Members.
	EID 	eId;
};

/*******************************************************************************
* ImageType
*******************************************************************************/
typedef enum
{
	IMAGE_CREATE,
	YUV_IMAGE_CREATE,
	THUMBNAIL_IMAGE_CREATE,
	JPEG_IMAGE_CREATE,
}IMAGE_TYPE;
	
//
/*******************************************************************************
*   IImageCreateThreadHandler
*******************************************************************************/
class IImageCreateThreadHandler : public virtual RefBase
{
public:     ////        Instantiation.
    virtual             ~IImageCreateThreadHandler() {}

public:     ////        Interfaces
    virtual MBOOL        onThreadLoop(IMAGE_TYPE imgType)
    {
    	MBOOL ret = MFALSE;
		switch	(imgType)
		{
		case IMAGE_CREATE: 
			ret = onCreateImage();
			break;
		case YUV_IMAGE_CREATE: 
			ret = onCreateYuvImage();
			break;
		case THUMBNAIL_IMAGE_CREATE:
			ret = onCreateThumbnailImage();
			break;
		case JPEG_IMAGE_CREATE:
			ret = onCreateJpegImage();
			break;
		default:
			break;
		}
		return	ret;
	}
    virtual MBOOL        onCreateImage()  					= 	0;
    virtual MBOOL        onCreateYuvImage()           		= 	0;
    virtual MBOOL        onCreateThumbnailImage()    	= 	0;
	virtual MBOOL		 onCreateJpegImage()		   	= 	0;

};

/*******************************************************************************
*   IImageCreateThread
*******************************************************************************/
class IImageCreateThread : public Thread
{
public:     ////        Instantiation.
    static  IImageCreateThread* createInstance(IMAGE_TYPE imgType, IImageCreateThreadHandler*const pHandler);
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

};  // namespace NSCamShot

#endif  //_MTK_HAL_MTKATVCAMADAPTER_IMAGECREATETHREAD_H_


