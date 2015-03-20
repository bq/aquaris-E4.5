LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    m4u_it.cpp \
    
LOCAL_SHARED_LIBRARIES:= \
    libui \
    libutils \
    libcutils \
    libmhalpipe \
    libmhaldrv \
    libmhalscenario \
	  libhardware \
	  libmhal \
 
    
LOCAL_C_INCLUDES:= \
  $(TOP)/$(MTK_PATH_SOURCE)/external/mhal/src/core/drv/6575/mdp \
  $(TOP)/$(MTK_PATH_SOURCE)/external/mhal/src/core/drv/6575/jpeg \
  $(TOP)/$(MTK_PATH_SOURCE)/external/mhal/src/core/scenario/imagetransform \
  $(TOP)/$(MTK_PATH_SOURCE)/external/mhal/src/core/scenario/camera \
  $(TOP)/$(MTK_PATH_SOURCE)/external/mhal/inc \
  $(TOP)/system/core/include/cutils \
  $(TOP)/$(MTK_PATH_PLATFORM)/kernel/drivers/jpeg \
  $(TOP)/$(MTK_PATH_PLATFORM)/kernel/core/include/mach \
  $(TOP)/$(MTK_PATH_SOURCE)/external/mhal/src/core/drv/6575/inc \
  $(TOP)/$(MTK_PATH_SOURCE)/external/mhal/src/core/drv/6575/inc \
    
    
LOCAL_MODULE:= m4u_it

LOCAL_PRELINK_MODULE:=false
include $(BUILD_EXECUTABLE) 


