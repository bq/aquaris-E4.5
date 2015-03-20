
#ifndef _MTK_Detection_COMMON_H
#define _MTK_Detection_COMMON_H

#include "kal_release.h"
#include <mtkcam/algorithm/libfdft/MTKDetectionType.h>
#include <mtkcam/algorithm/libfdft/MTKDetectionErrCode.h>

#define RGB565_R_MASK (0xF800)
#define RGB565_G_MASK (0x07E0)
#define RGB565_B_MASK (0x001F)

#define RGB565_TO_RGB888_R(VALUE)   ((((VALUE) & RGB565_R_MASK)>>8))
#define RGB565_TO_RGB888_G(VALUE)   ((((VALUE) & RGB565_G_MASK)>>3))
#define RGB565_TO_RGB888_B(VALUE)   ((((VALUE) & RGB565_B_MASK)<<3))

#define RGB888_TO_YUV_Y(R, G, B)    ((  306 * (R) + 601 * (G) + 117 * (B) + 512) >> 10)

#define abs(a) (((a) < 0) ? -(a) : (a)) 

#endif



