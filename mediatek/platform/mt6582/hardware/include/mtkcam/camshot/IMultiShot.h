
#ifndef _MTK_CAMERA_INC_CAMSHOT_IMULTISHOT_H_
#define _MTK_CAMERA_INC_CAMPIPE_IMULTISHOT_H_

#include <mtkcam/common.h>

/*******************************************************************************
*
********************************************************************************/
namespace NSCamShot {

/******************************************************************************
 * 
 ******************************************************************************/
typedef enum
{
	SHOT_NCC = 0,
	SHOT_CC,
}SHOT_TYPE;


/******************************************************************************
 * 
 ******************************************************************************/

/**  
 * @class IMultiShot
 * @brief Interface of MultiShot
 */
class IMultiShot : public ICamShot
{
public:

public:     ////    Instantiation.
    /**
     * @brief create the IMultiShot instance  
     *
     * @details 
     * 
     * @note 
     * 
     * @param[in] eShotMode: the shot mode of the caller 
     * @param[in] pszCamShotName: the shot name of the caller           
     *
     * @return
     * - The IMultiShot instance 
     */ 
    static IMultiShot* createInstance(EShotMode const eShotMode, char const* const pszCamShotName, SHOT_TYPE eShotType);

}; 

}; //namespace NSCamShot
#endif  //  _MTK_CAMERA_INC_CAMSHOT_IMULTISHOT_H_


