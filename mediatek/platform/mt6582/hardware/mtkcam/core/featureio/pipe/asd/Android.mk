#===============================================================================


LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(BUILD_MTK_LDVT),true)
    LOCAL_CFLAGS += -DUSING_MTK_LDVT
endif

LOCAL_SRC_FILES += \
    asd_hal_base.cpp \
    asd/asd_hal.cpp \
  
LOCAL_C_INCLUDES:= \
	 $(TOP)/$(MTK_PATH_CUSTOM)/hal/inc \
	 $(TOP)/$(MTK_PATH_PLATFORM)/hardware/mtkcam/core/featureio/pipe/asd/asd \
	 $(TOP)/$(MTK_PATH_PLATFORM)/hardware/mtkcam/inc \
	 $(TOP)/$(MTK_PATH_PLATFORM)/hardware/mtkcam/inc/common \
	 $(TOP)/$(MTK_PATH_PLATFORM)/hardware/include/mtkcam/algorithm/libasd \
	 $(TOP)/$(MTK_PATH_PLATFORM)/hardware/mtkcam/inc/featureio \

LOCAL_STATIC_LIBRARIES := \

LOCAL_WHOLE_STATIC_LIBRARIES := \

LOCAL_MODULE := libfeatureiopipe_asd


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




