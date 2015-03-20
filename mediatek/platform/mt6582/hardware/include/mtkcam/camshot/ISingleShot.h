
#ifndef _MTK_CAMERA_INC_CAMSHOT_ISINGLESHOT_H_
#define _MTK_CAMERA_INC_CAMPIPE_ISINGLESHOT_H_

#include <mtkcam/common.h>

/*******************************************************************************
*
********************************************************************************/
namespace NSCamShot {

/******************************************************************************
 * 
 ******************************************************************************/

/**  
 * @class ISingleShot
 * @brief Interface of SingleShot
 */
class ISingleShot : public ICamShot
{
public:

public:     ////    Instantiation.
    /**
     * @brief create the ISingleShot instance  
     *
     * @details 
     * 
     * @note 
     * 
     * @param[in] eShotMode: the shot mode of the caller 
     * @param[in] pszCamShotName: the shot name of the caller           
     *
     * @return
     * - The ISingleShot instance 
     */ 
    static ISingleShot* createInstance(EShotMode const eShotMode, char const* const pszCamShotName);

}; 

}; //namespace NSCamShot
#endif  //  _MTK_CAMERA_INC_CAMSHOT_ISINGLESHOT_H_


