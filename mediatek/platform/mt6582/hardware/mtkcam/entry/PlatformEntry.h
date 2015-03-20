
#ifndef _MTK_PLATFORM_HARDWARE_MTKCAM_ENTRY_PLATFORMENTRY_H_
#define _MTK_PLATFORM_HARDWARE_MTKCAM_ENTRY_PLATFORMENTRY_H_


/******************************************************************************
 *
 ******************************************************************************/
#include <mtkcam/IPlatform.h>


/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {


/******************************************************************************
 *
 ******************************************************************************/
class PlatformEntry : public IPlatform
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  IPlatform Interfaces.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////

    virtual IHalMemory*             createHalMemory(char const* szCallerName)
                                    #if ('1'==MY_SUPPORT_HALMEMORY)
                                        ;
                                    #else
                                        {return NULL;}
                                    #endif

    virtual ICamDevice*             createCam1Device(
                                        char const*         szClientAppMode, 
                                        int32_t const       i4OpenId
                                    )
                                    #if ('1'==MY_SUPPORT_CAM1DEVICE)
                                        ;
                                    #else
                                        {return NULL;}
                                    #endif

    virtual ICamDevice*             createCam3Device(
                                        char const*         szClientAppMode, 
                                        int32_t const       i4OpenId
                                    )
                                    #if ('1'==MY_SUPPORT_CAM3DEVICE)
                                        ;
                                    #else
                                        {return NULL;}
                                    #endif

};


/*******************************************s***********************************
 *
 ******************************************************************************/
};  //namespace NSCam
#endif  //_MTK_PLATFORM_HARDWARE_MTKCAM_ENTRY_PLATFORMENTRY_H_



