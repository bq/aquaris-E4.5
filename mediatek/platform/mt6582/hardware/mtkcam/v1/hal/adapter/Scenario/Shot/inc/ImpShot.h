
#ifndef _MTK_CAMERA_CAMADAPTER_SCENARIO_SHOT_INC_IMPSHOT_H_
#define _MTK_CAMERA_CAMADAPTER_SCENARIO_SHOT_INC_IMPSHOT_H_


namespace android {
namespace NSShot {
/******************************************************************************
 *
 ******************************************************************************/


/******************************************************************************
 *  Implement Shot Class
 ******************************************************************************/
class ImpShot : public RefBase
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Attributes.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////                    Attributes.

    String8 const                   ms8ShotName;        //  shot name
    uint32_t const                  mu4ShotMode;        //  shot mode (defined in EShotMode)
    int32_t const                   mi4OpenId;          //  open id: 0/1/2

    sp<IShotCallback>               mpShotCallback;     //  pointer to IShotCallback.

    ShotParam                       mShotParam;         //  Shot parameters.
    JpegParam                       mJpegParam;         //  Jpeg parameters.

public:     ////                    Attributes.
    inline char const*              getShotName() const     { return ms8ShotName.string(); }
    inline uint32_t                 getShotMode() const     { return mu4ShotMode; }
    inline int32_t                  getOpenId() const       { return mi4OpenId; }

    //  Set Shot parameters.
    virtual bool                    setShotParam(void const* pParam, size_t const size);

    //  Set Jpeg parameters.
    virtual bool                    setJpegParam(void const* pParam, size_t const size);

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////                    Instantiation.
    virtual                         ~ImpShot();
                                    ImpShot(
                                        char const*const pszShotName, 
                                        uint32_t const u4ShotMode, 
                                        int32_t const i4OpenId
                                    );

public:     ////                    Operations.

    //  This function is invoked when this object is ready to destryoed in the
    //  destructor. All resources must be released before this returns.
    virtual void                    onDestroy()                             = 0;

    //  Set callbacks.
    virtual bool                    setCallback(sp<IShotCallback>& rpShotCallback);

    virtual bool                    sendCommand(
                                        uint32_t const  cmd, 
                                        uint32_t const  arg1, 
                                        uint32_t const  arg2
                                    );

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Implementations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////                    

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Utilities
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////                    
    virtual bool      makeExifHeader(MUINT32 const u4CamMode, 
										MUINT8* const puThumbBuf, 
										MUINT32 const u4ThumbSize, 
										MUINT8* puExifBuf, 
										MUINT32 &u4FinalExifSize, 
										MUINT32 u4ImgIndex = 0, 
										MUINT32 u4GroupId = 0,
										MUINT32 u4FocusValH = 0,
										MUINT32 u4FocusValL = 0	
										); 
};


/******************************************************************************
 *
 ******************************************************************************/
}; // namespace NSShot
}; // namespace android
#endif  //  _MTK_CAMERA_CAMADAPTER_SCENARIO_SHOT_INC_IMPSHOT_H_



