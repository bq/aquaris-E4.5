LOCAL_PATH := $(call my-dir)

ifeq ($(MTK_NFC_SUPPORT), yes)

MY_LOCAL_PATH := $(LOCAL_PATH)
BUILD_META_LIB := true
ifeq ($(BUILD_META_LIB), true)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := meta_nfc.c

LOCAL_CFLAGS += -O0 -g

LOCAL_C_INCLUDES += $(MTK_PATH_SOURCE)/external/meta/common/inc
LOCAL_C_INCLUDES += $(MTK_PATH_SOURCE)/external/mtknfc/inc
LOCAL_C_INCLUDES += $(MY_LOCAL_PATH)/
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libmeta_nfc
LOCAL_SHARED_LIBRARIES := libcutils libc libft
LOCAL_PRELINK_MODULE := false
include $(BUILD_STATIC_LIBRARY)
#include $(BUILD_SHARED_LIBRARY)
endif
###############################################################################
# SELF TEST
###############################################################################
BUILD_SELF_TEST := false
ifeq ($(BUILD_SELF_TEST), true)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := meta_nfc_test.c
LOCAL_CFLAGS += -O0 -g
LOCAL_C_INCLUDES += $(MTK_PATH_SOURCE)/external/meta/common/inc
LOCAL_C_INCLUDES += $(MTK_PATH_SOURCE)/external/mtknfc/inc
LOCAL_C_INCLUDES += $(MY_LOCAL_PATH)/
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := meta_nfc_test
LOCAL_ALLOW_UNDEFINED_SYMBOLS := true
LOCAL_SHARED_LIBRARIES := libmeta_nfc libcutils libc libft
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_UNSTRIPPED_PATH := $(TARGET_ROOT_OUT_SBIN_UNSTRIPPED)
include $(BUILD_EXECUTABLE)
endif

endif

