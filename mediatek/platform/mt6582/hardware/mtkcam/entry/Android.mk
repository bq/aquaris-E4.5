################################################################################
#
################################################################################

LOCAL_PATH := $(call my-dir)

################################################################################
#
################################################################################
include $(CLEAR_VARS)

#-----------------------------------------------------------
sinclude $(TOP)/$(MTK_PATH_PLATFORM)/hardware/mtkcam/mtkcam.mk
sinclude $(LOCAL_PATH)/moduleOption.mk

#-----------------------------------------------------------
LOCAL_SRC_FILES += $(call all-c-cpp-files-under, .)

#-----------------------------------------------------------
LOCAL_C_INCLUDES += $(MTKCAM_C_INCLUDES)
LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_SOURCE)/hardware/include
LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_PLATFORM)/hardware/include
#

#-----------------------------------------------------------
LOCAL_CFLAGS += $(MTKCAM_CFLAGS)
#
LOCAL_CFLAGS += $(MY_MODULE_CFLAGS)
#

#-----------------------------------------------------------
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libcutils
#
ifeq "'1'" "$(strip $(MY_SUPPORT_HALMEMORY))"
LOCAL_WHOLE_STATIC_LIBRARIES += libcam_platform.halmemoryadapter
LOCAL_SHARED_LIBRARIES += libcamdrv
endif
#
ifeq "'1'" "$(strip $(MY_SUPPORT_CAM1DEVICE))"
LOCAL_WHOLE_STATIC_LIBRARIES += libcam_platform.cam1device
LOCAL_SHARED_LIBRARIES += libcam.device1
endif
#
ifeq "'1'" "$(strip $(MY_SUPPORT_CAM3DEVICE))"
LOCAL_WHOLE_STATIC_LIBRARIES += libcam_platform.cam3device
LOCAL_SHARED_LIBRARIES += libcam.device3
endif
#
#
LOCAL_STATIC_LIBRARIES := 

#-----------------------------------------------------------
#LOCAL_MODULE_TAGS := user
LOCAL_MODULE := libcam_platform

#-----------------------------------------------------------
include $(BUILD_SHARED_LIBRARY)


################################################################################
#
################################################################################
include $(CLEAR_VARS)
include $(call all-makefiles-under,$(LOCAL_PATH))



