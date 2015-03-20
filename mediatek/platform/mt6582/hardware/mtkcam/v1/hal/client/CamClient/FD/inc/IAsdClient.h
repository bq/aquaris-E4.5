
#ifndef _MTK_HAL_CAMCLIENT_INC_IASDCLIENT_H_
#define _MTK_HAL_CAMCLIENT_INC_IASDCLIENT_H_
//


namespace android {
namespace NSCamClient {


/******************************************************************************
 *
 ******************************************************************************/
class IAsdClient : public virtual RefBase
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////                    Instantiation.

    static sp<IAsdClient>           createInstance(sp<IParamsManager> pParamsMgr);

    virtual                         ~IAsdClient() {}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////

    virtual bool                    init()                                  = 0;
    virtual bool                    uninit()                                = 0;

    virtual void                    setCallbacks(
                                        sp<CamMsgCbInfo> const& rpCamMsgCbInfo
                                    )                                       = 0;

    virtual void                    enable(bool fgEnable)                   = 0;
    virtual bool                    isEnabled() const                       = 0;

    virtual void                    update(MUINT8 * OT_Buffer, MINT32 a_Buffer_width, MINT32 a_Buffer_height) = 0;

};


}; // namespace NSCamClient
}; // namespace android
#endif  //_MTK_HAL_CAMCLIENT_INC_IASDCLIENT_H_



