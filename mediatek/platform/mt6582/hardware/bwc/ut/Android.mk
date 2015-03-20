LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := bwc_test.cpp \


LOCAL_MODULE := bwc_test

LOCAL_C_INCLUDES:= \
	$(MTK_PATH_SOURCE)/hardware/bwc/inc
  
LOCAL_SHARED_LIBRARIES := \
  libcutils \
  liblog \
  libbwc \
  
#LOCAL_STATIC_LIBRARIES := 
LOCAL_MODULE_TAGS := tests
LOCAL_PRELINK_MODULE:=false
include $(BUILD_EXECUTABLE) 


