
#ifndef ZSD_SCENARIO_H
#define ZSD_SCENARIO_H

using namespace NSImageio;
using namespace NSIspio;

namespace NSImageio{
namespace NSIspio{
    class ICamIOPipe;
};
};

class DpIspStream;
#include <DpDataType.h>

class IMemDrv;

#include <utils/threads.h>
using namespace android;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  setConfig --> start --> Loop {enque, deque} --> stop
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

class ZSDScenario : public IhwScenario{

private:
class DpPortInfo;
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  IhwScenario Interface.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:
    /////
    static ZSDScenario*     createInstance(EScenarioFmt rSensorType, halSensorDev_e const &dev, ERawPxlID const &bitorder);
    virtual MVOID           destroyInstance();
    virtual                 ~ZSDScenario();
    
protected:
                            ZSDScenario(EScenarioFmt rSensorType, halSensorDev_e const &dev, ERawPxlID const &bitorder);     

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
            MVOID           dumpPass1EnqueSeq();
            MVOID           dumpPass1DequeSeq();
            MBOOL           mapDpPortInfo( const PortImgInfo& port, DpPortInfo* pDpPort);

            MBOOL           mapPhyAddr( MUINT32 size, MINT32 id, MUINT32 va, 
                                        MINT32 secu, MINT32 cohe, MUINT32& pa );
            MBOOL           unmapPhyAddr( MUINT32 size, MINT32 id, MUINT32 va, 
                                        MINT32 secu, MINT32 cohe, MUINT32& pa );
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Data Members.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++    
private:
    IMemDrv*                mIMemDrv;
    ICamIOPipe*             mpCamIOPipe;
    DpIspStream*            mpDpStream;
    EScenarioFmt            mSensorType;
    halSensorDev_e          mSensorDev;
    ERawPxlID               mSensorBitOrder;
    Mutex                   mModuleMtx;   
    QBufInfo                mQbufVido;
    QBufInfo                mQbufDispo;
#if 1
    Vector<MUINT32>         mvPass1EnqueSeq;
    Vector<MUINT32>         mvPass1DequeSeq;
#endif


    class DpPortInfo{
    public:
        class tuple_3{
        public:
            MUINT32 v[3];
            tuple_3()
            {
                v[0] = v[1] = v[2] = 0;
            }
            tuple_3( const tuple_3 & t )
            {
                v[0] = t.v[0];
                v[1] = t.v[1];
                v[2] = t.v[2];
            }
            tuple_3& operator=( const tuple_3 & t )
            {
                v[0] = t.v[0];
                v[1] = t.v[1];
                v[2] = t.v[2];
                return *this;
            }
            MUINT32  operator[]( unsigned int n ) const { return v[n]; }
            MUINT32& operator[]( unsigned int n ) { return v[n]; }
            operator MUINT32*() { return v; }
        };
        DpPortInfo()
        : width(0) , height(0) , planenum(0)
        , crop_x(0) , crop_y(0) , crop_fx(0) , crop_fy(0) , crop_w(0) , crop_h(0)
        , rot(0) , flip(0) { } 

        DpColorFormat fmt;
        MUINT32 portID;
        MUINT32 width;
        MUINT32 height;
        MUINT32 planenum;
        tuple_3 stride;
        tuple_3 planesize;
        MUINT32 crop_x;
        MUINT32 crop_y;
        MUINT32 crop_fx;
        MUINT32 crop_fy;
        MUINT32 crop_w;
        MUINT32 crop_h;
        MUINT32 rot;
        MUINT32 flip;

        inline MVOID dump();
    };

    struct sDefaultSetting_Ports{
        //pass1
        PortInfo tgi;
        PortInfo imgo;
        PortInfo img2o;
        //pass2
        DpPortInfo imgi;
        DpPortInfo vido; //support rot, flip
        DpPortInfo dispo;
        //
        MVOID dump();
        //
    };
    sDefaultSetting_Ports   mSettingPorts;
};


#endif


