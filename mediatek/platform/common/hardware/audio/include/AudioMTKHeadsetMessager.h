
#ifndef _AUDIO_HEADSET_MESSAGE_H_
#define _AUDIO_HEADSET_MESSAGE_H_

/*****************************************************************************
*                E X T E R N A L   R E F E R E N C E S
******************************************************************************
*/
#include "AudioDef.h"
#include <linux/ioctl.h>

/*****************************************************************************
*                          C O N S T A N T S
******************************************************************************
*/

#define ACCDET_IOC_MAGIC 'A'
#define ACCDET_INIT       _IO(ACCDET_IOC_MAGIC,0)  // call wehn first time
#define SET_CALL_STATE    _IO(ACCDET_IOC_MAGIC,1)  // when state is changing , tell headset driver.
#define GET_BUTTON_STATUS _IO(ACCDET_IOC_MAGIC,2)  // ioctl to get hook button state.

static const char *HEADSET_PATH = "/dev/accdet";
static const char *YUSUHEADSET_STAUTS_PATH = "/sys/class/switch/h2w/state";

namespace android
{

/*****************************************************************************
*                        F U N C T I O N   D E F I N I T I O N
******************************************************************************
*/
class AudioMTKHeadSetMessager
{
    public:
        static AudioMTKHeadSetMessager *getInstance();
        static  bool Get_headset_info(void);

        bool SetHeadInit(void);
        void SetHeadSetState(int state);

    private:
        static AudioMTKHeadSetMessager *UniqueHeadsetInstance;
        // private constructor
        AudioMTKHeadSetMessager();
        AudioMTKHeadSetMessager(const AudioMTKHeadSetMessager &);             // intentionally undefined
        AudioMTKHeadSetMessager &operator=(const AudioMTKHeadSetMessager &);  // intentionally undefined

        pthread_t hHeadsetReadThread;
};

}; // namespace android

#endif





