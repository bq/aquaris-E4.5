
#ifndef _MTK_PLATFORM_HARDWARE_MTKCAM_V1_DEVICE_MTKATVCAM1DEVICE_H_
#define _MTK_PLATFORM_HARDWARE_MTKCAM_V1_DEVICE_MTKATVCAM1DEVICE_H_

#include <mtkcam/device/Cam1DeviceBase.h>
//
#include <mtkcam/hal/IResManager.h>
//
#if '1'==MTKCAM_HAVE_SENSOR_HAL
    #include <mtkcam/hal/sensor_hal.h>
#endif
//
#if '1'==MTKCAM_HAVE_3A_HAL
    #include <mtkcam/hal/aaa_hal_base.h>
#endif


/******************************************************************************
 *
 ******************************************************************************/
namespace android {


/******************************************************************************
 *
 ******************************************************************************/
class MtkAtvCam1Device : public Cam1DeviceBase
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Data Members.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////
    //
#if '1'==MTKCAM_HAVE_SENSOR_HAL
    SensorHal*                      mpSensorHal;
#endif
    //
#if '1'==MTKCAM_HAVE_3A_HAL
    NS3A::Hal3ABase*                mp3AHal;
#endif
    //
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////                    Instantiation.

    virtual                         ~MtkAtvCam1Device();
                                    MtkAtvCam1Device(
                                        String8 const&          rDevName, 
                                        int32_t const           i4OpenId
                                    );

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
protected:  ////                    Operations.

    virtual int32_t                 queryDisplayBufCount() const    { return 12; }

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  [Template method] Operations.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
protected:  ////                    Operations.

    /**
     * [Template method] Called by initialize().
     * Initialize the device resources owned by this object.
     */
    virtual bool                    onInit();

    /**
     * [Template method] Called by uninitialize().
     * Uninitialize the device resources owned by this object. Note that this is
     * *not* done in the destructor.
     */
    virtual bool                    onUninit();

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Cam1Device Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////

    virtual status_t                setParameters(const char* params);

};


/******************************************************************************
 *
 ******************************************************************************/
};  //namespace android
#endif  //_MTK_PLATFORM_HARDWARE_MTKCAM_V1_DEVICE_MTKATVCAM1DEVICE_H_



