
#ifndef _MTK_CAMERA_INC_COMMON_CAMUTILS_HWMISC_H_
#define _MTK_CAMERA_INC_COMMON_CAMUTILS_HWMISC_H_
//

#include <mtkcam/common.h>
#include <mtkcam/common/hw/hwstddef.h>

/******************************************************************************
*
*******************************************************************************/


namespace android {
namespace MtkCamUtils {
/******************************************************************************
*
*******************************************************************************/


/**
  * @brief Calc the crop region of the zoom ratio 
  *
  * @details 
  * 
  * @note 
  * 
  * @param[in] rSrc: The input rectangle of the source 
  * @param[in] rDst: The input rectangle of the destination 
  *
  * @return
  * - The final crop rectangle. 
  */ 
NSCamHW::Rect
calCrop(
    NSCamHW::Rect const &rSrc, 
    NSCamHW::Rect const &rDst, 
    uint32_t ratio
); 


/******************************************************************************
*
*******************************************************************************/
};  // namespace MtkCamUtils
};  // namespace android
#endif  //_MTK_CAMERA_INC_COMMON_CAMUTILS_HWMISC_H_



