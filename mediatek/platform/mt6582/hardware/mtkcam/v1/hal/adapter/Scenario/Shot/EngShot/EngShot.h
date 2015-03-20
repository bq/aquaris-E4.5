
#ifndef _MTK_CAMERA_CAMADAPTER_SCENARIO_SHOT_ENGSHOT_H_
#define _MTK_CAMERA_CAMADAPTER_SCENARIO_SHOT_ENGSHOT_H_


namespace android {
namespace NSShot {
/******************************************************************************
 *
 ******************************************************************************/

struct EngParam;

/******************************************************************************
 *
 ******************************************************************************/
class EngShot : public ImpShot
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////                    Instantiation.
    virtual                         ~EngShot();
                                    EngShot(
                                        char const*const pszShotName, 
                                        uint32_t const u4ShotMode, 
                                        int32_t const i4OpenId
                                    );

public:     ////                    Operations.

    //  This function is invoked when this object is firstly created.
    //  All resources can be allocated here.
    virtual bool                    onCreate();

    //  This function is invoked when this object is ready to destryoed in the
    //  destructor. All resources must be released before this returns.
    virtual void                    onDestroy();

    virtual bool                    sendCommand(
                                        uint32_t const  cmd, 
                                        uint32_t const  arg1, 
                                        uint32_t const  arg2
                                    );

    virtual bool                    setEngParam(void const* pParam, size_t const size);
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Implementations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////                    Operations.
    virtual bool                    onCmd_reset();
    virtual bool                    onCmd_capture();
    virtual void                    onCmd_cancel();


protected:  ////                    callbacks 
    static MBOOL fgCamShotNotifyCb(MVOID* user, NSCamShot::CamShotNotifyInfo const msg);
    static MBOOL fgCamShotDataCb(MVOID* user, NSCamShot::CamShotDataInfo const msg); 

    EngParam                        *mEngParam;         //  Engineer Mode parameters.

protected:
    MBOOL           handlePostViewData(MUINT8* const puBuf, MUINT32 const u4Size);
    MBOOL           handleJpegData(MUINT8* const puJpegBuf, MUINT32 const u4JpegSize, MUINT8* const puThumbBuf, MUINT32 const u4ThumbSize);    

protected: 

};


/******************************************************************************
 *
 ******************************************************************************/
}; // namespace NSShot
}; // namespace android
#endif  //  _MTK_CAMERA_CAMADAPTER_SCENARIO_SHOT_ENGSHOT_H_



