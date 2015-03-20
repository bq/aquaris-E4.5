
#ifndef _MTK_HAL_CAMADAPTER_INC_BASECAMADAPTER_H_
#define _MTK_HAL_CAMADAPTER_INC_BASECAMADAPTER_H_
//
#include <utils/Errors.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/threads.h>
//
#include <hardware/camera.h>
#include <system/camera.h>
//


namespace android {


class BaseCamAdapter : public ICamAdapter
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  IImgBufProviderClient Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////
    //
    /**
     * Notify when IImgBufProvider is created.
     */
    virtual bool                    onImgBufProviderCreated(sp<IImgBufProvider>const& rpProvider);
    /**
     * Notify when IImgBufProvider is destroyed.
     */
    virtual void                    onImgBufProviderDestroyed(int32_t const i4ProviderId);

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  ICamAdapter Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////
    /**
     * Initialize the device resources owned by this object.
     */
    virtual bool                    init()                  { return true; }

    /**
     * Uninitialize the device resources owned by this object. Note that this is
     * *not* done in the destructor.
     */
    virtual bool                    uninit()                { return true; }

    virtual char const*             getName()   const       { return mName.string(); }
    virtual int32_t                 getOpenId() const       { return mi4OpenId; }
    virtual sp<IParamsManager>const getParamsManager() const;
    virtual sp<CamMsgCbInfo>const   getCamMsgCbInfo() const;
    virtual void                    setCallbacks(sp<CamMsgCbInfo> const& rpCamMsgCbInfo);
    virtual void                    enableMsgType(int32_t msgType);
    virtual void                    disableMsgType(int32_t msgType);
    virtual bool                    msgTypeEnabled(int32_t msgType);
    virtual status_t                sendCommand(int32_t cmd, int32_t arg1, int32_t arg2);
    virtual status_t                dump(int fd, Vector<String8>const& args) { return OK; }

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////                    Instantiation.
    //
                                    BaseCamAdapter(
                                        String8 const&      rName, 
                                        int32_t const       i4OpenId, 
                                        sp<IParamsManager>  pParamsMgr
                                    );
    virtual                         ~BaseCamAdapter();

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Common Attributes.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////                    Adapter Info.
    String8 const                   mName;
    int32_t const                   mi4OpenId;
    sp<CamMsgCbInfo>                mpCamMsgCbInfo;

//------------------------------------------------------------------------------
protected:  ////    
    //
    //  Reference to Parameters Manager.
    sp<IParamsManager>              mpParamsMgr;

    //  Image Buffer Providers Manager.
    sp<ImgBufProvidersManager>      mpImgBufProvidersMgr;

};


}; // namespace android
#endif  //_MTK_HAL_CAMADAPTER_INC_BASECAMADAPTER_H_



