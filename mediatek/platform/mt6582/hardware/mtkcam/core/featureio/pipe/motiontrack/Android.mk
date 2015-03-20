#===============================================================================


LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(BUILD_MTK_LDVT),true)
    LOCAL_CFLAGS += -DUSING_MTK_LDVT
endif

LOCAL_SRC_FILES += \
    motiontrack_hal_base.cpp \
    motiontrack/motiontrack_hal.cpp \
  
LOCAL_C_INCLUDES:= \
	 $(TOP)/$(MTK_PATH_CUSTOM)/hal/inc \
	 $(TOP)/$(MTK_PATH_PLATFORM)/hardware/mtkcam/inc \
	 $(TOP)/$(MTK_PATH_PLATFORM)/hardware/mtkcam/core/featureio/pipe/motiontrack/motiontrack \
	 $(TOP)/$(MTK_PATH_PLATFORM)/hardware/include/mtkcam/algorithm/libmfbmm \
	 $(TOP)/$(MTK_PATH_PLATFORM)/hardware/mtkcam/inc/featureio \
	 $(TOP)/$(MTK_PATH_CUSTOM)/hal/inc

LOCAL_STATIC_LIBRARIES := \

LOCAL_WHOLE_STATIC_LIBRARIES := \

LOCAL_MODULE := libcam.featureio.pipe.motiontrack


#
# Start of common part ------------------------------------
sinclude $(TOP)/$(MTK_PATH_PLATFORM)/hardware/mtkcam/mtkcam.mk

#-----------------------------------------------------------
LOCAL_CFLAGS += $(MTKCAM_CFLAGS)

#-----------------------------------------------------------
LOCAL_C_INCLUDES += $(MTKCAM_C_INCLUDES)

#-----------------------------------------------------------
LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_SOURCE)/hardware/include
LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_PLATFORM)/hardware/include

# End of common part ---------------------------------------
#
include $(BUILD_STATIC_LIBRARY)


