
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
** $Log: cam_cal_drv_imp.h $
 *
 *
*/

#ifndef _CAM_CAL_DRV_IMP_H_
#define _CAM_CAL_DRV_IMP_H_

//#include <utils/threads.h>

/*******************************************************************************
*
********************************************************************************/

class CamCalDrv : public CamCalDrvBase
{
public:
    /////////////////////////////////////////////////////////////////////////
    //
    // getInstance () -
    //! \brief get instance
    //
    /////////////////////////////////////////////////////////////////////////
    static CamCalDrvBase* getInstance();

    /////////////////////////////////////////////////////////////////////////
    //
    // destroyInstance () -
    //! \brief destroy instance
    //
    /////////////////////////////////////////////////////////////////////////
    virtual void destroyInstance();
  
    virtual int GetCamCalCalData(unsigned long i4SensorDevId,
                          /*unsigned long u4SensorID,*/
                          CAMERA_CAM_CAL_TYPE_ENUM a_eCamCalDataType,
	                      void *a_pCamCalData);	                       

private:
    /////////////////////////////////////////////////////////////////////////
    //
    // CamCalDrv () -
    //! \brief constructor
    //
    /////////////////////////////////////////////////////////////////////////
    CamCalDrv();

    /////////////////////////////////////////////////////////////////////////
    //
    // ~CamCalDrv () -
    //! \brief descontrustor
    //
    /////////////////////////////////////////////////////////////////////////
    virtual ~CamCalDrv();

    int Init(unsigned long u4SensorID,void *a_pCamCalData)  ;

    
private: 
    static CAM_CAL_DATA_STRUCT StCamCalCaldata;
    static MINT32 m32CamCalDataValidation;
};

#endif  //  _NVRAM_DRV_IMP_H_



