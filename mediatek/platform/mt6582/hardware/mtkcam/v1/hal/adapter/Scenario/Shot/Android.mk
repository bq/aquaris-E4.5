################################################################################
#
################################################################################

LOCAL_PATH := $(call my-dir)

################################################################################
#
################################################################################
include $(CLEAR_VARS)

#-----------------------------------------------------------
LOCAL_SRC_FILES += $(call all-c-cpp-files-under, .)

#-----------------------------------------------------------
LOCAL_C_INCLUDES += $(TOP)/$(MTK_ROOT)/frameworks-ext/av/include
LOCAL_C_INCLUDES += $(TOP)/$(MTK_ROOT)/hardware/mtkcam/inc
LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_PLATFORM)/hardware/mtkcam/inc
LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_PLATFORM)/hardware/mtkcam/v1/hal/adapter/inc/Scenario

#-----------------------------------------------------------
LOCAL_WHOLE_STATIC_LIBRARIES += libcam.camadapter.scenario.shot.testshot
LOCAL_WHOLE_STATIC_LIBRARIES += libcam.camadapter.scenario.shot.normalshot
LOCAL_WHOLE_STATIC_LIBRARIES += libcam.camadapter.scenario.shot.continuousshot
LOCAL_WHOLE_STATIC_LIBRARIES += libcam.camadapter.scenario.shot.facebeautyshot
LOCAL_WHOLE_STATIC_LIBRARIES += libcam.camadapter.scenario.shot.evshot
LOCAL_WHOLE_STATIC_LIBRARIES += libcam.camadapter.scenario.shot.bestshot
LOCAL_WHOLE_STATIC_LIBRARIES += libcam.camadapter.scenario.shot.hdrshot
LOCAL_WHOLE_STATIC_LIBRARIES += libcam.camadapter.scenario.shot.engshot
LOCAL_WHOLE_STATIC_LIBRARIES += libcam.camadapter.scenario.shot.zsdshot
#
LOCAL_STATIC_LIBRARIES += 

#-----------------------------------------------------------
LOCAL_SHARED_LIBRARIES += 

#-----------------------------------------------------------
LOCAL_MODULE := libcam.camadapter.scenario.shot

#-----------------------------------------------------------

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




################################################################################
#
################################################################################
include $(CLEAR_VARS)
include $(call all-makefiles-under,$(LOCAL_PATH))



