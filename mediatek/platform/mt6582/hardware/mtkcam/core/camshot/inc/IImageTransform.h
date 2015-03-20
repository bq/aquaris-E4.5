
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
#ifndef _MTK_CAMERA_CORE_CAMSHOT_INC_IIMAGE_TRANSFORM_H_
#define _MTK_CAMERA_CORE_CAMSHOT_INC_IIMAGE_TRANSFORM_H_
//
//

#include <mtkcam/common/hw/hwstddef.h>

using namespace NSCamHW; 
/*******************************************************************************
*
********************************************************************************/
namespace NSCamShot {
////////////////////////////////////////////////////////////////////////////////



/*******************************************************************************
* Interface of Image Transform 
********************************************************************************/
class IImageTransform
{
protected:  ////    Constructor/Destructor.
    virtual         ~IImageTransform() {}

public:     ////    Attributes 
    virtual MINT32      getLastErrorCode() const = 0;

public:     ////    Instantiation.
    static IImageTransform* createInstance();
    virtual MVOID   destroyInstance() = 0;

public:     ////    Operations.
    /*
     *  Image transform , the functionality is such as bitblt function 
     *
     *  Params:
     *      rSrcBufInfo
     *      [I] The image buffer info of the input image 
     *          
     *
     *      rDstBufInfo
     *      [I] The image buffer info of the output image 
     *          
     *      rROI 
     *      [I] The crop of region of the input image 
     *   
     *      u4Rotation
     *      [I] The rotation operation 
     *      
     *      u4Flip
     *      [I] The flip operation
     * 
     *      u4TimeOutInMs
     *      [I] The time out of the operation
     *
     *  Return:
     *      MTRUE indicates success; MFALSE indicates failure, and an error code
     *      can be retrived by getLastErrorCode().
     */
    virtual  MBOOL    execute(ImgBufInfo const rSrcBufInfo, ImgBufInfo const rDstBufInfo, Rect const rROI, MUINT32 const u4Rotation, MUINT32 const u4Flip, MUINT32 const u4TimeOutInMs) = 0; 

};


////////////////////////////////////////////////////////////////////////////////
};  //namespace NSCamShot
#endif  //  _MTK_CAMERA_CORE_CAMSHOT_INC_IIMAGE_TRANSFORM_H_



