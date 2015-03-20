
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
** $Log: mcu_drv.h $
 *
*/

#ifndef _MCU_DRV_H_
#define _MCU_DRV_H_

#include "MediaTypes.h"
#include "camera_custom_lens.h"              //in custom folder 

/*******************************************************************************
*
********************************************************************************/
//Structures
typedef struct {
    //current position
    unsigned long u4CurrentPosition;
    //macro position
    unsigned long u4MacroPosition;
    //Infiniti position
    unsigned long u4InfPosition;
    //Motor Status
    bool          bIsMotorMoving;
    //Motor Open?
    bool          bIsMotorOpen;
    //Slew Rate?
    bool          bIsSupportSR;
    
} mcuMotorInfo;


/*******************************************************************************
*
********************************************************************************/
class MCUDrv {
public:
    /////////////////////////////////////////////////////////////////////////
    //
    /////////////////////////////////////////////////////////////////////////                           
    typedef enum
    {
        MCU_NO_ERROR = 0,                  ///< The function work successfully
        MCU_UNKNOWN_ERROR = 0x80000000,    ///< Unknown error    
        MCU_INVALID_DRIVER   = 0x80000001,
    } MCU_ERROR_ENUM;

protected:
    /////////////////////////////////////////////////////////////////////////
    //
    // ~MCUDrv () -
    //! \brief mhal mcu base descontrustor
    //
    /////////////////////////////////////////////////////////////////////////    
    virtual ~MCUDrv() = 0;

public:     //// Interfaces
    /////////////////////////////////////////////////////////////////////////
    //
    /////////////////////////////////////////////////////////////////////////                           
    static MCUDrv* createInstance(unsigned int a_u4CurrLensId);

    /////////////////////////////////////////////////////////////////////////
    //
    /////////////////////////////////////////////////////////////////////////                           
    virtual void destroyInstance() = 0;

    /////////////////////////////////////////////////////////////////////////
    //
    // init () -
    //! \brief 
    //
    /////////////////////////////////////////////////////////////////////////    
    virtual int init() = 0; 

    /////////////////////////////////////////////////////////////////////////
    //
    // uninit () -
    //! \brief 
    //
    /////////////////////////////////////////////////////////////////////////    
    virtual int uninit() = 0;

    /////////////////////////////////////////////////////////////////////////
    //
    // moveMCU () -
    //! \brief 
    //
    /////////////////////////////////////////////////////////////////////////    
    virtual int moveMCU(int a_i4FocusPos) = 0;

    /////////////////////////////////////////////////////////////////////////
    //
    // getMCUInfo () -
    //! \brief 
    //
    /////////////////////////////////////////////////////////////////////////    
    virtual int  getMCUInfo(mcuMotorInfo *a_pMotorInfo) = 0;

    /////////////////////////////////////////////////////////////////////////
    //
    // setMCUInfPos () -
    //! \brief 
    //
    /////////////////////////////////////////////////////////////////////////    
    virtual int setMCUInfPos(int a_i4FocusPos) = 0;

    /////////////////////////////////////////////////////////////////////////
    //
    // setMCUMacroPos () -
    //! \brief 
    //
    /////////////////////////////////////////////////////////////////////////    
    virtual int setMCUMacroPos(int a_i4FocusPos) = 0;

    /////////////////////////////////////////////////////////////////////////
    //
    // lensSearch () -
    //! \brief 
    //
    /////////////////////////////////////////////////////////////////////////   
    static int lensSearch(unsigned int  a_u4CurrSensorDev, unsigned int  a_u4CurrSensorId);

    /////////////////////////////////////////////////////////////////////////
    //
    // getCurrLensID () -
    //! \brief 
    //
    /////////////////////////////////////////////////////////////////////////   
    static unsigned int getCurrLensID();

    //FIXME, is this need here 
    static MSDK_LENS_INIT_FUNCTION_STRUCT m_LensInitFunc[MAX_NUM_OF_SUPPORT_LENS];
    static unsigned int  m_u4CurrLensIdx;

};

#endif



