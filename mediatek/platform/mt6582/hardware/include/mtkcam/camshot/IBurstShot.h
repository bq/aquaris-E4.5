
#ifndef _MTK_CAMERA_INC_CAMSHOT_ISINGLESHOT_H_
#define _MTK_CAMERA_INC_CAMPIPE_ISINGLESHOT_H_

#include <mtkcam/common.h>

namespace NS3A {
    class CaptureParam_T;
};
/*******************************************************************************
*
********************************************************************************/
namespace NSCamShot {

/******************************************************************************
 *
 ******************************************************************************/

/**
 * @class IBurstShot
 * @brief Interface of BurstShot
 */
class IBurstShot : public ICamShot
{
public:

public:     ////    Instantiation.
    /**
     * @brief create the IBurstShot instance
     *
     * @details
     *
     * @note
     *
     * @param[in] eShotMode: the shot mode of the caller
     * @param[in] pszCamShotName: the shot name of the caller
     *
     * @return
     * - The IBurstShot instance
     */
    static IBurstShot* createInstance(EShotMode const eShotMode, char const* const pszCamShotName);
    virtual MBOOL   registerImgBufInfo(ECamShotImgBufType const eBufType, ImgBufInfo *pImgBuf, MUINT32 length);
    virtual MBOOL   registerCap3AParam(NS3A::CaptureParam_T *pCap3AParam, MUINT32 length);
};

}; //namespace NSCamShot
#endif  //  _MTK_CAMERA_INC_CAMSHOT_ISINGLESHOT_H_


