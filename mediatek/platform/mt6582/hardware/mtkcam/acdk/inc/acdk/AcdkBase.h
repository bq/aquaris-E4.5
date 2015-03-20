
///////////////////////////////////////////////////////////////////////////////
// No Warranty
// Except as may be otherwise agreed to in writing, no warranties of any
// kind, whether express or implied, are given by MTK with respect to any MTK
// Deliverables or any use thereof, and MTK Deliverables are provided on an
// "AS IS" basis.  MTK hereby expressly disclaims all such warranties,
// including any implied warranties of merchantability, non-infringement and
// fitness for a particular purpose and any warranties arising out of course
// of performance, course of dealing or usage of trade.  Parties further
// acknowledge that Company may, either presently and/or in the future,
// instruct MTK to assist it in the development and the implementation, in
// accordance with Company's designs, of certain softwares relating to
// Company's product(s) (the "Services").  Except as may be otherwise agreed
// to in writing, no warranties of any kind, whether express or implied, are
// given by MTK with respect to the Services provided, and the Services are
// provided on an "AS IS" basis.  Company further acknowledges that the
// Services may contain errors, that testing is important and Company is
// solely responsible for fully testing the Services and/or derivatives
// thereof before they are used, sublicensed or distributed.  Should there be
// any third party action brought against MTK, arising out of or relating to
// the Services, Company agree to fully indemnify and hold MTK harmless.
// If the parties mutually agree to enter into or continue a business
// relationship or other arrangement, the terms and conditions set forth
// hereunder shall remain effective and, unless explicitly stated otherwise,
// shall prevail in the event of a conflict in the terms in any agreements
// entered into between the parties.
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2008, MediaTek Inc.
// All rights reserved.
//
// Unauthorized use, practice, perform, copy, distribution, reproduction,
// or disclosure of this information in whole or in part is prohibited.
////////////////////////////////////////////////////////////////////////////////
// AcdkBase.h  $Revision$
////////////////////////////////////////////////////////////////////////////////

//! \file  AcdkBase.h

#ifndef _ACDKBASE_H_
#define _ACDKBASE_H_

namespace NSACDK
{
    /**
         *@class AcdkBase
         *@brief ACDK Class for AcdkIF
       */
    class AcdkBase
    {
    public:

        /**                       
                *@brief Create AcdkBase Object
                *@note Actually, it will create AcdkMain object
              */
        static AcdkBase* createInstance();
        
        /**                       
                *@brief Destory AcdkBase Object
              */
        virtual void destroyInstance() = 0;

        /**                       
                *@brief Initialize function
                *@note Must call this function right after createInstance and before other functions
                *@return
                *-0 indicates success, otherwise indicates fail
              */
        virtual MINT32 init () = 0;

        /**                       
                *@brief Uninitialize function
                *@note Must call this function before destroyInstance
                *@return
                *-0 indicates success, otherwise indicates fail
              */
        virtual MINT32 uninit() = 0;

        /**                       
                *@brief Start preview
                *
                *@param[in] prvCb : preview callback function
                *
                *@return
                *-0 indicates success, otherwise indicates fail
              */
        virtual MINT32 startPreview(Func_CB prvCb) = 0;

        /**                       
                *@brief Stop preview
                *@return
                *-0 indicates success, otherwise indicates fail
              */
        virtual MINT32 stopPreview() = 0;

        /**                       
                *@brief Capture
                *@note If parameter width and height are not spcified, ACDK will decide by itself based on capture mode
                *
                *@param[in] mode : capture mode
                *@param[in] imgType : capture image type
                *@param[in] capCb : capture callback function
                *@param[in] width : width of capture image
                *@param[in] height : height of capture image
                *@param[in] captureCnt : capture times. default is 1
                *@param[in] isSaveImg : save capture image or not. 0-no save, 1-save. default is 0
                *
                *@return
                *-0 indicates success, otherwise indicates fail
              */
        virtual MINT32 takePicture(
                    MUINT32 const mode,
                    MUINT32 const imgType,
                    Func_CB const capCb = 0,
                    MUINT32 const width = 0,
                    MUINT32 const height = 0,
                    MUINT32 const captureCnt = 1,
                    MINT32  const isSaveImg = 0) = 0;

        /**                       
                *@brief Get preview frame number
                *
                *@param[in,out] frameCnt : will set to preview frame number   
                *
                *@return
                *-0 indicates success, otherwise indicates fail
              */
        virtual MINT32 getFrameCnt(MUINT32 &frameCnt) = 0;

        /**                       
                *@brief Setting sensor device
                *@note Sensor device is default setted to main sensor
                *
                *@param[in] srcDev : 0x1-main sensor, 0x2-sub sensor, 0x8-main2 sensor
                *
                *@return
                *-0 indicates success, otherwise indicates fail
              */
        virtual MINT32 setSrcDev(MINT32 srcDev) = 0;

        /**                       
                *@brief Show quick-view image
                *@note It will be called after takePicture automatically
                *
                *@param[in] qvFormat : image format of QV
                *
                *@return
                *-0 indicates success, otherwise indicates fail
              */
        virtual MINT32 quickViewImg(MUINT32 qvFormat) = 0;

        /**                       
                *@brief Command interface
                *@note AcdkIF only use this function to communicate with ACDK
                *
                *@param[in] a_u4Ioctl : command
                *@param[in] puParaIn : input parameter
                *@param[in] u4ParaInLen : input parameter length
                *@param[in] puParaOut : output parameter
                *@param[in] u4ParaOutLen : output parameter length
                *@param[in] pu4RealParaOutLen
                *
                *@return
                *-0 indicates success, otherwise indicates fail
              */
        virtual MINT32 sendcommand(
                    MUINT32 const a_u4Ioctl,
                    MUINT8 *puParaIn,
                    MUINT32 const u4ParaInLen,
                    MUINT8 *puParaOut,
                    MUINT32 const u4ParaOutLen,
                    MUINT32 *pu4RealParaOutLen) = 0;

    protected:
        AcdkBase () {};
        virtual ~AcdkBase() {};
    };
};
#endif //end AcdkBase.h



