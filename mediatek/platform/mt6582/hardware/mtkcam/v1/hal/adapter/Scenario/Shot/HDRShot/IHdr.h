
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
#ifndef _IHDR_H_
#define _IHDR_H_

/*******************************************************************************
*
*******************************************************************************/
class IShot;
class ShotBase;


/*******************************************************************************
*
*******************************************************************************/
class IHdr : public IShot
{
private:
    IHdr(ShotBase*const pShot);
    virtual ~IHdr() {}

public:     ////    Interfaces.
    //
    static IHdr*    createInstance(ESensorType_t const eSensorType, EDeviceId_t const eDeviceId);

    virtual MVOID   destroyInstance();

    virtual MBOOL   init(ShotParam const& rShotParam, Hal3ABase*const pHal3A);

    virtual MBOOL   uninit()
                    { return mrShotBase.uninit(); }

    virtual MBOOL   capture()
                    { return mrShotBase.capture(); }

    virtual MBOOL   setParam(ShotParam const& rShotParam)
                    { return mrShotBase.setParam(rShotParam); }

public:     ////    Attributes.
    //
    virtual char const*     getShotName() const
                    { return mrShotBase.getShotName(); }

    virtual ESensorType_t   getSensorType() const
                    { return mrShotBase.getSensorType(); }

    virtual EDeviceId_t     getDeviceId() const
                    { return mrShotBase.getDeviceId(); }

protected:
    ShotBase&       mrShotBase;
};


#endif  //  _IHDR_H_



