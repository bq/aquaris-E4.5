
#ifndef _MTK_CAMERA_INC_CAMSHOT_ISAMPLE_SHOT_H_
#define _MTK_CAMERA_INC_CAMSHOT_ISAMPLE_SHOT_H_

/*******************************************************************************
*
********************************************************************************/
namespace NSCamShot {

/******************************************************************************
 * 
 ******************************************************************************/

/**  
 * @class ISampleShot
 * @brief Interface of Sampleshot 
 */
class ISampleShot : public ICamShot
{
public:

public:     ////    Instantiation.
    /**
     * @brief create the ISampleShot instance  
     *
     * @details 
     * 
     * @note 
     * 
     * @param[in] eShotMode: the shot mode of the caller 
     * @param[in] pszCamShotName: the shot name of the caller           
     *
     * @return
     * - The ISampleShot instance 
     */ 
    static ISampleShot* createInstance(EShotMode const eShotMode, char const* const pszCamShotName);

}; 

}; //namespace NSCamShot
#endif  //  _MTK_CAMERA_INC_CAMSHOT_ISAMPLE_SHOT_H_


