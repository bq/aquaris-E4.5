#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#include <utils/Log.h>
#include "AudioMTKHeadsetMessager.h"

/*****************************************************************************
*                          C O N S T A N T S
******************************************************************************
*/
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "AudioHeadSetMessager"

#ifdef ENABLE_LOG_AUDIOHEADSETMESSAGER
#define LOG_AudioHeadSetMessager ALOGD
#else
#define LOG_AudioHeadSetMessager ALOGV
#endif

namespace android
{

/*****************************************************************************
*                   G L O B A L      V A R I A B L E
******************************************************************************
*/
static int HeadsetFd = 0;
static int headstatusFd = 0;
#define BUF_LEN 1
static char rbuf[BUF_LEN] = {'\0'};
static char wbuf[BUF_LEN] = {'1'};
static char wbuf1[BUF_LEN] = {'2'};

AudioMTKHeadSetMessager *AudioMTKHeadSetMessager::UniqueHeadsetInstance = 0;

/*****************************************************************************
*                        F U N C T I O N   D E F I N I T I O N
******************************************************************************
*/
AudioMTKHeadSetMessager *AudioMTKHeadSetMessager::getInstance()
{
    if (UniqueHeadsetInstance == 0)
    {
        ALOGD("+UniqueDigitalInstance\n");
        UniqueHeadsetInstance = new AudioMTKHeadSetMessager();
        ALOGD("-UniqueDigitalInstance\n");
    }
    return UniqueHeadsetInstance;
}

bool AudioMTKHeadSetMessager::SetHeadInit()
{
    LOG_AudioHeadSetMessager("SetHeadInit");
    int ret = 0;
    if (HeadsetFd <= 0)
    {
        // open headset device
        HeadsetFd = open(HEADSET_PATH, O_RDONLY);
        if (HeadsetFd < 0)
        {
            ALOGE("open %s error fd = %d", HEADSET_PATH, HeadsetFd);
            return false;
        }
    }
    ret = ::ioctl(HeadsetFd, ACCDET_INIT, 0);
    return true;
}

AudioMTKHeadSetMessager::AudioMTKHeadSetMessager()
{
    LOG_AudioHeadSetMessager("AudioHeadSetMessager Contructor");
    int ret = 0;
}

void AudioMTKHeadSetMessager::SetHeadSetState(int state)
{
    LOG_AudioHeadSetMessager("SetHeadSetState state = %d");
    int ret = 0;
    if (HeadsetFd <= 0)
    {
        // open headset device
        HeadsetFd = open(HEADSET_PATH, O_RDONLY);
        if (HeadsetFd < 0)
        {
            ALOGE("open %s error fd = %d", HEADSET_PATH, HeadsetFd);
            return;
        }
    }
    ret = ::ioctl(HeadsetFd, SET_CALL_STATE, state);
}

bool AudioMTKHeadSetMessager::Get_headset_info(void)
{
    headstatusFd = -1;
    headstatusFd = open(YUSUHEADSET_STAUTS_PATH, O_RDONLY, 0);

    if (headstatusFd < 0)
    {
        ALOGE("open %s error fd = %d", YUSUHEADSET_STAUTS_PATH, headstatusFd);
        return false;
    }

    if (read(headstatusFd, rbuf, BUF_LEN) == -1)
    {
        ALOGD("Get_headset_info Can't read headset");
        close(headstatusFd);
        return false;
    }

    if (!strncmp(wbuf, rbuf, BUF_LEN))
    {
        ALOGD("Get_headset_info Get_headset_info state  == 1");
        close(headstatusFd);
        return  true;
    }

    if (!strncmp(wbuf, rbuf, BUF_LEN))
    {
        ALOGD("Get_headset_info state  == 2");
        close(headstatusFd);
        return true;
    }
    else
    {
        ALOGD("Get_headset_info return  false");
        close(headstatusFd);
        return  false;
    }
}

}






