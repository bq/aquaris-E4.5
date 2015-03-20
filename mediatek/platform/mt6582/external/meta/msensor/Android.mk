LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := meta_msensor.c
LOCAL_C_INCLUDES := $(MTK_PATH_SOURCE)/external/meta/common/inc $(TOP)/bionic/libc/kernel/include/common
LOCAL_SHARED_LIBRARIES := libft liblog libnvram libhwm
LOCAL_MODULE := libmeta_msensor
LOCAL_PRELINK_MODULE := false
include $(BUILD_STATIC_LIBRARY)

###############################################################################
# SELF TEST
###############################################################################
BUILD_SELF_TEST := false
ifeq ($(BUILD_SELF_TEST), true)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := meta_msensor_test.c
LOCAL_C_INCLUDES := $(MTK_PATH_SOURCE)/external/meta/common/inc $(TOP)/bionic/libc/kernel/include/common
LOCAL_MODULE := meta_ms_test
LOCAL_ALLOW_UNDEFINED_SYMBOLS := true
LOCAL_SHARED_LIBRARIES := libft libhwm
LOCAL_STATIC_LIBRARIES := libmeta_msensor
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_UNSTRIPPED_PATH := $(TARGET_ROOT_OUT_SBIN_UNSTRIPPED)
include $(BUILD_EXECUTABLE)
endif
