ifeq ($(MTK_EMMC_SUPPORT), yes)
##### EMMC META library #####
BUILD_CRYPTFS := true

ifeq ($(BUILD_CRYPTFS), true)
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := meta_cryptfs.c
LOCAL_C_INCLUDES := $(MTK_PATH_SOURCE)/external/meta/common/inc
LOCAL_SHARED_LIBRARIES := libcutils libc libft
LOCAL_MODULE := libmeta_cryptfs
LOCAL_PRELINK_MODULE := false
include $(BUILD_STATIC_LIBRARY)
endif
endif


