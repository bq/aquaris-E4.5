
/********************************************************************************************
 *     LEGAL DISCLAIMER
 *
 *     (Header of MediaTek Software/Firmware Release or Documentation)
 *
 *     BY OPENING OR USING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 *     THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE") RECEIVED
 *     FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON AN "AS-IS" BASIS
 *     ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED,
 *     INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR
 *     A PARTICULAR PURPOSE OR NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY
 *     WHATSOEVER WITH RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 *     INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK
 *     ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
 *     NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S SPECIFICATION
 *     OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
 *
 *     BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE LIABILITY WITH
 *     RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION,
TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE
 *     FEES OR SERVICE CHARGE PAID BY BUYER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 *     THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE WITH THE LAWS
 *     OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF LAWS PRINCIPLES.
 ************************************************************************************************/
#ifndef _MTK_CAMERA_CORE_CAMSHOT_INC_CAMSHOT_IMP_H_
#define _MTK_CAMERA_CORE_CAMSHOT_INC_CAMSHOT_IMP_H_

//
#include <vector>
//
using namespace std;

//
#include <cutils/atomic.h>
//

//

using namespace NSCamPipe; 

/*******************************************************************************
*
********************************************************************************/
namespace NSCamShot {
////////////////////////////////////////////////////////////////////////////////


/*******************************************************************************
*
********************************************************************************/
class CamShotImp
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Attributes.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////    Data Members.
    EShotMode           meShotMode; 
    char const*const    mszCamShotName;
    MINT32              mi4ErrorCode;

public:     ////    Operations.
    virtual EShotMode     getShotMode() const          {return meShotMode; }
    virtual char const* getCamShotName() const         { return mszCamShotName; }
    virtual MINT32      getLastErrorCode() const    { return mi4ErrorCode; }

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Instantiation.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////    Constructor/Destructor.
                    CamShotImp(
                        EShotMode const eShotMode,
                        char const*const szCamShotName
                    );
    virtual         ~CamShotImp()  {}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Callbacks.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////    Data Members.
    //
    MVOID*          mpCbUser;           //  Callback user.
    //
    //  notify callback
    volatile MINT32         mi4NotifyMsgSet;//  bitset of notify message types.
    CamShotNotifyCallback_t    mNotifyCb;      //  notify callback function pointer.
    //
    //  data callback
    volatile MINT32         mi4DataMsgSet;  //  bitset of data message types.
    CamShotDataCallback_t      mDataCb;        //  data callback function pointer.

protected:  ////    Helpers.
    virtual MBOOL   onNotifyCallback(CamShotNotifyInfo const& msg) const;
    virtual MBOOL   onDataCallback(CamShotDataInfo const& msg) const;

public:     ////    Operations.
    //
    virtual MVOID   setCallbacks(CamShotNotifyCallback_t notify_cb, CamShotDataCallback_t data_cb, MVOID* user);
    //
    //  notify callback
    inline MBOOL    isNotifyMsgEnabled(MINT32 const i4MsgTypes) const   { return (i4MsgTypes & mi4NotifyMsgSet); }
    inline MVOID    enableNotifyMsg(MINT32 const i4MsgTypes)            { ::android_atomic_or(i4MsgTypes, &mi4NotifyMsgSet); }
    inline MVOID    disableNotifyMsg(MINT32 const i4MsgTypes)           { ::android_atomic_and(~i4MsgTypes, &mi4NotifyMsgSet); }
    //
    //  data callback
    inline MBOOL    isDataMsgEnabled(MINT32 const i4MsgTypes) const     { return (i4MsgTypes & mi4DataMsgSet); }
    inline MVOID    enableDataMsg(MINT32 const i4MsgTypes)              { ::android_atomic_or(i4MsgTypes, &mi4DataMsgSet); }
    inline MVOID    disableDataMsg(MINT32 const i4MsgTypes)             { ::android_atomic_and(~i4MsgTypes, &mi4DataMsgSet); }

protected:  ////    Helper function 
    MVOID      dumpPipeProperty(vector<PortProperty> const &vInPorts, vector<PortProperty> const &vOutPorts); 
    MVOID      dumpSensorParam(SensorParam const & rParam);     
    MVOID      dumpShotParam(ShotParam const & rParam); 
    MVOID      dumpJpegParam(JpegParam const & rParam); 
    //

protected:
    MBOOL      handleNotifyCallback(MINT32 const i4Msg, MUINT32 const ext1, MUINT32 const ext2);  
    MBOOL      handleDataCallback(MINT32 const i4Msg, MUINT32 const ext1, MUINT32 const ext2, MUINT8* puData, MUINT32 const u4Size); 
    
protected:

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Scenario.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////    Data Members.
    //

public:     ////    Operations.




};


////////////////////////////////////////////////////////////////////////////////
};  //namespace NSCamShot
#endif  //  _MTK_CAMERA_CORE_CAMSHOT_INC_CAMSHOT_IMP_H_



