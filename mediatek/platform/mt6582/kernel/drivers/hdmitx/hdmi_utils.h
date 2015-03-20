#ifndef HDMI_UTILS_H
#define HDMI_UTILS_H
#include <asm/ioctl.h>

#include "hdmitx.h"

#define HDMI_LOG(fmt, arg...) \
    do { \
        if (hdmi_log_on) {printk("[hdmi]%s,#%d ", __func__, __LINE__); printk(fmt, ##arg);} \
    }while (0)

#define HDMI_FUNC()    \
    do { \
        if(hdmi_log_on) printk("[hdmi] %s\n", __func__); \
    }while (0)

#define HDMI_LINE()    \
    do { \
        if (hdmi_log_on) {printk("[hdmi]%s,%d ", __func__, __LINE__); printk(fmt, ##arg);} \
    }while (0)


static char *_hdmi_ioctl_spy(unsigned int cmd)
{
    switch (cmd)
    {
        case MTK_HDMI_AUDIO_VIDEO_ENABLE:
            return "MTK_HDMI_AUDIO_VIDEO_ENABLE";

        case MTK_HDMI_AUDIO_ENABLE:
            return "MTK_HDMI_AUDIO_ENABLE";

        case MTK_HDMI_VIDEO_ENABLE:
            return "MTK_HDMI_VIDEO_ENABLE";

        case MTK_HDMI_GET_CAPABILITY:
            return "MTK_HDMI_GET_CAPABILITY";

        case MTK_HDMI_GET_DEVICE_STATUS:
            return "MTK_HDMI_GET_DEVICE_STATUS";

        case MTK_HDMI_VIDEO_CONFIG:
            return "MTK_HDMI_VIDEO_CONFIG";
        case MTK_HDMI_AUDIO_CONFIG:
            return "MTK_HDMI_AUDIO_CONFIG";
        case MTK_HDMI_FORCE_FULLSCREEN_ON:
            return "MTK_HDMI_FORCE_FULLSCREEN_ON";
        case MTK_HDMI_FORCE_FULLSCREEN_OFF:
            return "MTK_HDMI_FORCE_FULLSCREEN_OFF";
        case MTK_HDMI_IPO_POWEROFF:
            return "MTK_HDMI_IPO_POWEROFF";
        case MTK_HDMI_IPO_POWERON:
            return "MTK_HDMI_IPO_POWERON";
        case MTK_HDMI_POWER_ENABLE:
            return "MTK_HDMI_POWER_ENABLE";
        case MTK_HDMI_PORTRAIT_ENABLE:
            return "MTK_HDMI_PORTRAIT_ENABLE";
        case MTK_HDMI_FORCE_OPEN:
            return "MTK_HDMI_FORCE_OPEN";
        case MTK_HDMI_FORCE_CLOSE:
            return "MTK_HDMI_FORCE_CLOSE";
        case MTK_HDMI_IS_FORCE_AWAKE:
            return "MTK_HDMI_IS_FORCE_AWAKE";
        case MTK_HDMI_ENTER_VIDEO_MODE:
            return "MTK_HDMI_ENTER_VIDEO_MODE";
        case MTK_HDMI_LEAVE_VIDEO_MODE:
            return "MTK_HDMI_LEAVE_VIDEO_MODE";
        case MTK_HDMI_REGISTER_VIDEO_BUFFER:
            return "MTK_HDMI_REGISTER_VIDEO_BUFFER";
        case MTK_HDMI_POST_VIDEO_BUFFER:
            return "MTK_HDMI_POST_VIDEO_BUFFER";
        case MTK_HDMI_FACTORY_MODE_ENABLE:
            return "MTK_HDMI_FACTORY_MODE_ENABLE";
#ifdef MTK_MT8193_HDMI_SUPPORT

        case MTK_HDMI_WRITE_DEV:
            return "MTK_HDMI_WRITE_DEV";

        case MTK_HDMI_READ_DEV:
            return "MTK_HDMI_READ_DEV";

        case MTK_HDMI_ENABLE_LOG:
            return "MTK_HDMI_ENABLE_LOG";

        case MTK_HDMI_CHECK_EDID:
            return "MTK_HDMI_CHECK_EDID";

        case MTK_HDMI_INFOFRAME_SETTING:
            return "MTK_HDMI_INFOFRAME_SETTING";

        case MTK_HDMI_ENABLE_HDCP:
            return "MTK_HDMI_ENABLE_HDCP";

        case MTK_HDMI_STATUS:
            return "MTK_HDMI_STATUS";

        case MTK_HDMI_HDCP_KEY:
            return "MTK_HDMI_HDCP_KEY";

        case MTK_HDMI_GET_EDID:
            return "MTK_HDMI_GET_EDID";

        case MTK_HDMI_SETLA:
            return "MTK_HDMI_SETLA";

        case MTK_HDMI_GET_CECCMD:
            return "MTK_HDMI_GET_CECCMD";

        case MTK_HDMI_SET_CECCMD:
            return "MTK_HDMI_SET_CECCMD";

        case MTK_HDMI_CEC_ENABLE:
            return "MTK_HDMI_CEC_ENABLE";

        case MTK_HDMI_GET_CECADDR:
            return "MTK_HDMI_GET_CECADDR";

        case MTK_HDMI_CECRX_MODE:
            return "MTK_HDMI_CECRX_MODE";

        case MTK_HDMI_SENDSLTDATA:
            return "MTK_HDMI_SENDSLTDATA";

        case MTK_HDMI_GET_SLTDATA:
            return "MTK_HDMI_GET_SLTDATA";

        case MTK_HDMI_COLOR_DEEP:
            return "MTK_HDMI_COLOR_DEEP";
#endif

        case MTK_HDMI_GET_DEV_INFO:
            return "MTK_HDMI_GET_DEV_INFO";

        case MTK_HDMI_PREPARE_BUFFER:
            return "MTK_HDMI_PREPARE_BUFFER";

        case MTK_HDMI_FACTORY_GET_STATUS:
            return "MTK_HDMI_FACTORY_GET_STATUS";

        case MTK_HDMI_FACTORY_DPI_TEST:
            return "MTK_HDMI_FACTORY_DPI_TEST";

        case MTK_HDMI_SCREEN_CAPTURE:
            return "MTK_HDMI_SCREEN_CAPTURE";

        default:
            return "unknown ioctl command";
    }
}


#endif
