
#ifndef _MTK_CAMERA_CAMADAPTER_SCENARIO_SHOT_TESTSHOT_H_
#define _MTK_CAMERA_CAMADAPTER_SCENARIO_SHOT_TESTSHOT_H_


namespace android {
namespace NSShot {
namespace NSTestShot {
/******************************************************************************
 *
 ******************************************************************************/


/******************************************************************************
 *
 ******************************************************************************/
class MyShot : public ImpShot
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////                    Instantiation.
    virtual                         ~MyShot();
                                    MyShot(
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

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Implementations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////                    Operations.
    virtual bool                    onCmd_reset();
    virtual bool                    onCmd_capture();
    virtual void                    onCmd_cancel();

protected:  ////                    Data Members.
    int32_t volatile                mi4IsCancel;

};


/******************************************************************************
 *
 ******************************************************************************/
}; // namespace NSTestShot
}; // namespace NSShot
}; // namespace android
#endif  //  _MTK_CAMERA_CAMADAPTER_SCENARIO_SHOT_TESTSHOT_H_



