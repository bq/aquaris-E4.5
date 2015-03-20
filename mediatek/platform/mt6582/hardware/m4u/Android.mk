ifeq ($(MTK_PLATFORM),$(filter $(MTK_PLATFORM),MT6582))

LOCAL_PATH:= $(call my-dir)


include $(CLEAR_VARS)

LOCAL_SRC_FILES:= m4u_lib.cpp

LOCAL_C_INCLUDES:= \
  $(TOP)/$(MTK_PATH_PLATFORM)/kernel/core/include/mach \
  $(TOP)/$(MTK_PATH_PLATFORM)/hardware/m4u \


LOCAL_SHARED_LIBRARIES := \
     libcutils \
     liblog \
 
 
LOCAL_MODULE := libm4u

LOCAL_MODULE_TAGS := eng

#LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
#include $(call all-makefiles-under,$(LOCAL_PATH))


endif




