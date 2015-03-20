
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
/*
** $Log: autorama_hal.h $
 *
*/

#ifndef _AUTORAMA_HAL_H_
#define _AUTORAMA_HAL_H_

#include <mtkcam/featureio/autorama_hal_base.h>
#include <mtkcam/algorithm/libmotion/MTKMotion.h>

class MTKAutorama;
class MTKMotion;
/*******************************************************************************
*
********************************************************************************/
class halAUTORAMA: public halAUTORAMABase 
{
public:
    //
    static halAUTORAMABase* getInstance();
    virtual void destroyInstance();
    //
    /////////////////////////////////////////////////////////////////////////
    //
    // halFDBase () -
    //! \brief FD Hal constructor
    //
    /////////////////////////////////////////////////////////////////////////                       
    halAUTORAMA(); 

    /////////////////////////////////////////////////////////////////////////
    //
    // ~mhalCamBase () -
    //! \brief mhal cam base descontrustor 
    //
    /////////////////////////////////////////////////////////////////////////    
    virtual ~halAUTORAMA();

        /////////////////////////////////////////////////////////////////////////
    //
    // mHalAUTORAMAInit () -
    //! \brief init autorama
    //
    /////////////////////////////////////////////////////////////////////////    
    virtual MINT32 mHalAutoramaInit(MTKPipeAutoramaEnvInfo AutoramaInitInData, MTKPipeMotionEnvInfo MotionInitInfo);

    /////////////////////////////////////////////////////////////////////////
    //
    // mHalAutoramaUninit () -
    //! \brief autorama uninit 
    //
    /////////////////////////////////////////////////////////////////////////    
    virtual MINT32 mHalAutoramaUninit();
       
    /////////////////////////////////////////////////////////////////////////
    //
    // mHalAutoramaGetParam () -
    //! \brief draw fd face detection result rectangle 
    //
    /////////////////////////////////////////////////////////////////////////    
    virtual MINT32 mHalAutoramaGetResult(MTKPipeAutoramaResultInfo* ResultInfo );

    /////////////////////////////////////////////////////////////////////////
    //
    // mHalAutoramaCalcStitch () -
    //! \brief get face detection result 
    //
    /////////////////////////////////////////////////////////////////////////    
    virtual MINT32 mHalAutoramaCalcStitch(void* SrcImg,MINT32 gEv,MTKPIPEAUTORAMA_DIRECTION_ENUM DIRECTION);
    
    /////////////////////////////////////////////////////////////////////////
    //
    // mHalAutoramaDoStitch () -
    //! \brief get face detection result 
    //
    /////////////////////////////////////////////////////////////////////////    
    virtual MINT32 mHalAutoramaDoStitch(); 

    /////////////////////////////////////////////////////////////////////////
    //
    // mHalAutoramaDoMotion () -
    //! \brief check motion and capture 
    //
    ///////////////////////////////////////////////////////////////////////// 
    virtual MINT32 mHalAutoramaDoMotion(MUINT32* ImgSrc,MUINT32* MotionResult);

    /////////////////////////////////////////////////////////////////////////
    //
    // mHalAutoramaGetWokSize () -
    //! \brief get working buffer size 
    //
    ///////////////////////////////////////////////////////////////////////// 
    virtual MINT32 mHalAutoramaGetWokSize(int SrcWidth, int SrcHeight, int ShotNum, int &WorkingSize);
    
protected:


private:
    MTKAutorama* m_pMTKAutoramaObj;
    MTKMotion* m_pMTKMotionObj;
};

#endif



