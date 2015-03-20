
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
#ifndef _NVRAM_DRV_MGR_H_
#define _NVRAM_DRV_MGR_H_


namespace NS3A
{


/*******************************************************************************
* NVRAM Driver Manager
*******************************************************************************/
class NvramDrvMgr
{
protected:
    virtual ~NvramDrvMgr() {}

public:     ////
    static NvramDrvMgr&   getInstance();

public:     ////    Interfaces.

    virtual MRESULT     init(
        MINT32 const i4SensorDev
    ) = 0;

    virtual MRESULT     uninit() = 0;

public:     ////    Interfaces.

    virtual MVOID   getRefBuf(NVRAM_CAMERA_ISP_PARAM_STRUCT*& rpBuf) const = 0;
    virtual MVOID   getRefBuf(NVRAM_CAMERA_SHADING_STRUCT*& rpBuf) const = 0;
    virtual MVOID   getRefBuf(NVRAM_CAMERA_3A_STRUCT*& rpBuf) const = 0;
    virtual MVOID   getRefBuf(AE_PLINETABLE_T*& rpBuf) const = 0;
    virtual MVOID   getRefBuf(NVRAM_LENS_PARA_STRUCT*& rpBuf) const = 0;
    virtual MVOID   getRefBuf(NVRAM_CAMERA_STROBE_STRUCT*& rpBuf) const = 0;
    virtual MVOID   getRefBuf(CAMERA_TSF_TBL_STRUCT*& rpBuf) const = 0;

};


/*******************************************************************************
*
*******************************************************************************/


};  //  NS3A
#endif // _NVRAM_DRV_MGR_H_



