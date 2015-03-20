LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := mtdutils.c mounts.c

LOCAL_C_INCLUDES += $(MTK_PATH_SOURCE)/external/meta/common/inc
 
LOCAL_MODULE := libstorageutil

LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)


