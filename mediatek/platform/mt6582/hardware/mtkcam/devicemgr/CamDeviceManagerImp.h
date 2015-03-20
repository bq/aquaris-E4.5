
#ifndef _MTK_PLATFORM_HARDWARE_MTKCAM_DEVICEMGR_CAMDEVICEMANAGERIMP_H_
#define _MTK_PLATFORM_HARDWARE_MTKCAM_DEVICEMGR_CAMDEVICEMANAGERIMP_H_
//
#include <mtkcam/device/CamDeviceManagerBase.h>


/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {


/******************************************************************************
 *
 ******************************************************************************/
class CamDeviceManagerImp : public CamDeviceManagerBase
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Implementations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////                        Instantiation.
                                        CamDeviceManagerImp();

protected:  ////                        Operations.
    virtual android::status_t           validateOpenLocked(int32_t i4OpenId) const;

    virtual int32_t                     enumDeviceLocked();

};


/******************************************************************************
 *
 ******************************************************************************/
};  //namespace NSCam
#endif  //_MTK_PLATFORM_HARDWARE_MTKCAM_DEVICEMGR_CAMDEVICEMANAGERIMP_H_



