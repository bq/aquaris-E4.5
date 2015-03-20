
#ifndef _MTK_HAL_INC_IFEATURECLINET_H_
#define _MTK_HAL_INC_IFEATURECLINET_H_
//

/******************************************************************************
*   Camera Client Callback Interface.
*******************************************************************************/
typedef enum PreFeatureObject_s  
{
    PRE_MAV_OBJ_NORMAL,
    PRE_PANO_OBJ_NORMAL, 
    PRE_PANO3D_OBJ_NORMAL, 
    PRE_MOTIONTRACK_OBJ_NORMAL, 
}PreFeatureObject_e;
    
namespace android {
namespace NSCamClient {

typedef MBOOL   (*ImgDataCallback_t)(MVOID* const puJpegBuf, int u4SrcWidth, int u4SrcHeight);

class IFeatureClient : public virtual RefBase
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////
    static sp<IFeatureClient>       createInstance(PreFeatureObject_e eobject,int ShotNum);                     
    virtual                         ~IFeatureClient()  {}
    
    virtual bool                    init(int bufwidth,int bufheight)    = 0;
    virtual bool                    uninit()                            = 0;

    /**
     * After get preview frame then do addimg
     */
    virtual MINT32                  mHalCamFeatureProc(MVOID * bufadr, int32_t& mvX, int32_t& mvY, int32_t& dir, MBOOL& isShot) = 0;

    /**
     * Stop feature the parameter wil design to save image or not.
     */
    virtual bool                    stopFeature(int cancel)                 = 0;
    virtual MVOID                   setImgCallback(ImgDataCallback_t data_cb) = 0;
    virtual MINT32                  mHalCamFeatureCompress() = 0;
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

};
};
};
#endif  //_MTK_HAL_INC_ICAMCLIENT_H_



