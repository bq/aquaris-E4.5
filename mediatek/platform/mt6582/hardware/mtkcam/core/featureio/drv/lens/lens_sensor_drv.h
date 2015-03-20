
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
** $Log: lens_sensor_drv.h $
 *
*/

#ifndef _LENS_SENSOR_DRV_H_
#define _LENS_SENSOR_DRV_H_

#include <utils/threads.h>

using namespace android;

extern "C" {
#include <pthread.h>
#include <semaphore.h>
}


/*******************************************************************************
*
********************************************************************************/
class LensSensorDrv : public MCUDrv
{
private:    //// Instanciation outside is disallowed.
    /////////////////////////////////////////////////////////////////////////
    //
    // LensSensorDrv () -
    //! \brief
    //
    /////////////////////////////////////////////////////////////////////////                       
    LensSensorDrv();

    /////////////////////////////////////////////////////////////////////////
    //
    // ~LensSensorDrv () -
    //! \brief
    //
    /////////////////////////////////////////////////////////////////////////    
    virtual ~LensSensorDrv();

public:     //// Interfaces
    /////////////////////////////////////////////////////////////////////////
    //
    /////////////////////////////////////////////////////////////////////////
    static MCUDrv* getInstance();

    /////////////////////////////////////////////////////////////////////////
    //
    /////////////////////////////////////////////////////////////////////////
    virtual void destroyInstance();

    /////////////////////////////////////////////////////////////////////////
    //
    // init () -
    //! \brief 
    //
    /////////////////////////////////////////////////////////////////////////    
    virtual int init();

    /////////////////////////////////////////////////////////////////////////
    //
    // uninit () -
    //! \brief 
    //
    /////////////////////////////////////////////////////////////////////////    
    virtual int uninit();

    /////////////////////////////////////////////////////////////////////////
    //
    // moveMCU () -
    //! \brief 
    //
    /////////////////////////////////////////////////////////////////////////    
    virtual int moveMCU(int a_i4FocusPos);

    /////////////////////////////////////////////////////////////////////////
    //
    // getMCUInfo () -
    //! \brief 
    //
    /////////////////////////////////////////////////////////////////////////    
    virtual int  getMCUInfo(mcuMotorInfo *a_pMotorInfo);

    /////////////////////////////////////////////////////////////////////////
    //
    // setMCUInfPos () -
    //! \brief 
    //
    /////////////////////////////////////////////////////////////////////////    
    virtual int setMCUInfPos(int a_i4FocusPos);

    /////////////////////////////////////////////////////////////////////////
    //
    // setMCUMacroPos () -
    //! \brief 
    //
    /////////////////////////////////////////////////////////////////////////    
    virtual int setMCUMacroPos(int a_i4FocusPos);

private:
    //
    int             m_fdMCU;
    int             m_userCnt; 
    mutable Mutex   mLock;    

    int             m_i4FocusPos;

};

#endif  //  _LENS_DRV_H_



