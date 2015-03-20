
#ifndef VSS_SCENARIO_H
#define VSS_SCENARIO_H

using namespace NSImageio;
using namespace NSIspio;

namespace NSImageio{
namespace NSIspio{
    class ICamIOPipe;
    class IPostProcPipe;
};
};

#include <utils/threads.h>
using namespace android;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  setConfig --> start --> Loop {enque, deque} --> stop
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

class VSSScenario : public IhwScenario{

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  IhwScenario Interface.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:
    /////
    static VSSScenario*     createInstance(EScenarioFmt rSensorType, halSensorDev_e const &dev, ERawPxlID const &bitorder);
    virtual MVOID           destroyInstance();
    virtual                 ~VSSScenario();
    
protected:
                            VSSScenario(EScenarioFmt rSensorType, halSensorDev_e const &dev, ERawPxlID const &bitorder);     

public: 
    virtual MBOOL           init();
    virtual MBOOL           uninit();

    virtual MBOOL           start();
    virtual MBOOL           stop();

    virtual MVOID           wait(EWaitType rType);
    
    virtual MBOOL           deque(EHwBufIdx port, vector<PortQTBufInfo> *pBufIn);
    virtual MBOOL           enque(vector<PortBufInfo> *pBufIn = NULL, vector<PortBufInfo> *pBufOut = NULL);
    virtual MBOOL           enque(vector<IhwScenario::PortQTBufInfo> const &in);
    virtual MBOOL           replaceQue(vector<PortBufInfo> *pBufOld, vector<PortBufInfo> *pBufNew);

    virtual MBOOL           setConfig(vector<PortImgInfo> *pImgIn); 

    virtual MVOID           getHwValidSize(MUINT32 &width, MUINT32 &height);
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Private Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
private:
            MVOID           defaultSetting();
            //
            MBOOL           enquePass1(vector<PortBufInfo> *pBufOut = NULL);
            MBOOL           enquePass2(
                                vector<PortBufInfo> *pBufIn = NULL,
                                vector<PortBufInfo> *pBufOut = NULL);
            MBOOL           enquePass2TwoRun(
                                vector<PortBufInfo> *pBufIn = NULL,
                                vector<PortBufInfo> *pBufOut = NULL);
            //
            MBOOL           dequePass1(
                                EHwBufIdx port,
                                vector<PortQTBufInfo> *pBufIn);
            MBOOL           dequePass2(
                                EHwBufIdx port,
                                vector<PortQTBufInfo> *pBufIn);
            MBOOL           dequePass2TwoRun(
                                EHwBufIdx port,
                                vector<PortQTBufInfo> *pBufIn);
            //
            MVOID           handleRotate(vector<PortQTBufInfo> *pBufOut);
            MVOID           calCrop(
                                MUINT32     srcW,
                                MUINT32     SrcH,
                                MUINT32&    rCropW,
                                MUINT32&    rCropH, 
                                MUINT32&    rCropX,
                                MUINT32&    rCropY);
            EImageRotation  calRotation(EImageRotation rot = eImgRot_0);
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Data Members.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++    
private:
    ICamIOPipe*             mpCamIOPipe;
    IPostProcPipe*          mpPostProcPipe;
    EScenarioFmt            mSensorType;
    halSensorDev_e          mSensorDev;
    ERawPxlID               mSensorBitOrder;
    Mutex                   mModuleMtx;
    MINT32                  mSensorId;
    MBOOL                   mbTwoRunRot;
    //
    struct sDefaultSetting_Ports{
        PortInfo tgi;
        PortInfo imgo;
        PortInfo imgi;
        PortInfo vido;
        PortInfo dispo;
        //
        MVOID dump();
        //
    };
    sDefaultSetting_Ports   mSettingPorts;
    //
    struct TwoRunRotInfo{
        QBufInfo outBuf;
        QBufInfo inBuf;
        PortInfo outPort;
    };
    TwoRunRotInfo           mTwoRunRotInfo;
};


#endif


