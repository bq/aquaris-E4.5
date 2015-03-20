
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
#ifndef _MTK_CAMERA_INC_CAMSHOT_BUFFERS_H_
#define _MTK_CAMERA_INC_CAMSHOT_BUFFERS_H_

using namespace NSCamHW; 

/*******************************************************************************
*
********************************************************************************/
namespace NSCamShot {
////////////////////////////////////////////////////////////////////////////////

/**  
 * @struct BayerImgBufInfo
 * @brief This structure is bayer image buffer info
 *
 */
struct BayerImgBufInfo: public ImgBufInfo
{
public:    //// fields.
    /**
      * @var u4PixelOrder 
      * The pixel oreder, 0:BGbGrR 1:GbBRGr 2:GrRBGb 3:RGrGbB 
      */    
    MUINT32 u4PixelOrder; 
    /**
      * @var u4BitDepth 
      * The bitdepth of the pixel, 8, 10, 12
      */        
    MUINT32 u4BitDepth; 
    /**
      * @var fgIsPacked 
      * Is the pixel packed 
      */        
    MBOOL   fgIsPacked; 
    
public:    //// constructors.
    BayerImgBufInfo(
    )
        : ImgBufInfo()
        , u4PixelOrder(0)
        , u4BitDepth(0)
        , fgIsPacked(0) 
    {   
    }
    //
    BayerImgBufInfo(
        ImgBufInfo const _ImgBufInfo, 
        MUINT32    const _u4PixelOrder, 
        MUINT32    const _u4BitDepth, 
        MBOOL      const _fgIsPacked
    )
        : ImgBufInfo(_ImgBufInfo)
        , u4PixelOrder(_u4PixelOrder)
        , u4BitDepth(_u4BitDepth)
        , fgIsPacked(_fgIsPacked)
    {
    }
};  

////////////////////////////////////////////////////////////////////////////////
};  //namespace NSCamShot
#endif  //  _MTK_CAMERA_INC_CAMSHOT_BUFFERS_H_



