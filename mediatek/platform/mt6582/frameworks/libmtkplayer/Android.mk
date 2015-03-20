$(warning "lw1")

LOCAL_PATH:= $(call my-dir)
LOCAL_COMMON_PATH:=../../../common

#
# libmtkplayer(depend on libaudiosetting and audio hal)
#

#ifneq ($(strip $(BOARD_USES_GENERIC_AUDIO)),true)

#ifneq ($(strip $(HAVE_MATV_FEATURE))_$(strip $(MTK_FM_SUPPORT)), no_no)

include $(CLEAR_VARS)

$(warning "lw1")
LOCAL_SRC_FILES+= \
    $(LOCAL_COMMON_PATH)/frameworks/libmtkplayer/mATVAudioPlayer.cpp
ifneq ($(strip $(MTK_FM_SUPPORT)), no)
    LOCAL_SRC_FILES+= \
    $(LOCAL_COMMON_PATH)/frameworks/libmtkplayer/FMAudioPlayer.cpp
endif
ifeq ($(strip $(BOARD_USES_MTK_AUDIO)),true)
    LOCAL_SRC_FILES+= \
    $(LOCAL_COMMON_PATH)/frameworks/libmtkplayer/VibSpkAudioPlayer.cpp
endif

ifeq ($(TARGET_OS)-$(TARGET_SIMULATOR),linux-true)
LOCAL_LDLIBS += -ldl -lpthread
endif

LOCAL_SHARED_LIBRARIES :=     \
	libcutils             \
	libutils              \
	libbinder             \
	libmedia              \
	libaudiosetting       

ifneq ($(TARGET_SIMULATOR),true)
LOCAL_SHARED_LIBRARIES += libdl
endif

LOCAL_C_INCLUDES :=  \
	$(JNI_H_INCLUDE)                                              \
	$(TOP)/frameworks/av/services/audioflinger                    \
	$(TOP)/frameworks/av/include/media                            \
  $(TOP)/frameworks/av/include                                  \
  $(TOP)/$(MTK_PATH_SOURCE)/frameworks-ext/av                   \
	$(TOP)/$(MTK_PATH_SOURCE)/frameworks-ext/av/include           \
	$(TOP)/$(MTK_PATH_SOURCE)/frameworks-ext/av/include/media     \
	$(TOP)/$(MTK_PATH_PLATFORM)/frameworks/av/media/libmtkplayer	\
	$(TOP)/$(MTK_PATH_SOURCE)/external/matvctrl                   \
	
ifeq ($(strip $(BOARD_USES_MTK_AUDIO)),true)
LOCAL_C_INCLUDES+= \
	 $(TOP)/$(MTK_PATH_SOURCE)/platform/common/hardware/audio/aud_drv \
   $(TOP)/$(MTK_PATH_SOURCE)/platform/common/hardware/audio/include \
   $(TOP)/$(MTK_PATH_SOURCE)/platform/common/hardware/audio/speech_driver \
   $(TOP)/$(MTK_PATH_SOURCE)/platform/common/hardware/audio/ \
   $(TOP)/$(MTK_PATH_PLATFORM)/hardware/audio/aud_drv \
   $(TOP)/$(MTK_PATH_PLATFORM)/hardware/audio/include \
   $(TOP)/$(MTK_PATH_PLATFORM)/hardware/audio/speech_driver \
   $(TOP)/$(MTK_PATH_PLATFORM)/hardware/audio \
   $(TOP)/$(MTK_PATH_SOURCE)/external/AudioCompensationFilter \
   $(TOP)/$(MTK_PATH_SOURCE)/external/cvsd_plc_codec \
   $(TOP)/$(MTK_PATH_SOURCE)/external/msbc_codec \
   $(call include-path-for, audio-utils) \
   $(call include-path-for, audio-effects) \
   $(TOP)/$(MTK_PATH_SOURCE)/external/audiodcremovefl \
   $(MTK_PATH_SOURCE)/external/AudioSpeechEnhancement/inc \
   $(MTK_PATH_SOURCE)/external/audiodcremoveflt \
   $(MTK_PATH_SOURCE)/external/audiocustparam \
   $(MTK_PATH_SOURCE)/external/AudioComponentEngine
   
LOCAL_SHARED_LIBRARIES += libaudio.primary.default

ifeq ($(HAVE_AEE_FEATURE),yes)
    LOCAL_SHARED_LIBRARIES += libaed
    LOCAL_C_INCLUDES += \
    $(MTK_PATH_SOURCE)/external/aee/binary/inc
    LOCAL_CFLAGS += -DHAVE_AEE_FEATURE
endif
else
LOCAL_CFLAGS += -DFAKE_FM
LOCAL_CFLAGS += -DFAKE_MATV
LOCAL_CFLAGS += -DFAKE_VIBSPK
endif

ifeq ($(MTK_VIBSPK_SUPPORT),yes)
  LOCAL_CFLAGS += -DMTK_VIBSPK_SUPPORT
endif

$(warning "lw2")
LOCAL_MODULE:= libmtkplayer

LOCAL_PRELINK_MODULE := no

include $(BUILD_SHARED_LIBRARY)

#endif



