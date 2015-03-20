LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := m4u_ut.cpp \

LOCAL_MODULE := m4u_ut

  
LOCAL_SHARED_LIBRARIES := \
  libcutils \
  liblog 

LOCAL_C_INCLUDES+= $(MTK_PATH_PLATFORM)hardware/m4u
LOCAL_SHARED_LIBRARIES += libm4u
  
  
#LOCAL_STATIC_LIBRARIES := 
LOCAL_MODULE_TAGS := eng
#LOCAL_PRELINK_MODULE:=false
include $(BUILD_EXECUTABLE) 


