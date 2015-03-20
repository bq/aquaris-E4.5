################################################################################

LOCAL_PATH := $(call my-dir)

################################################################################
#
################################################################################
include $(CLEAR_VARS)

#-----------------------------------------------------------
#$(shell cd $(LOCAL_PATH); touch -m `find . -maxdepth 2`)
MTKCAM_HAVE_CAMFEATURE          := '1'      # built-in if '1' ; otherwise not built-in
MTKCAM_HAVE_SENSOR_HAL          := '1'      # built-in if '1' ; otherwise not built-in
MTKCAM_HAVE_3A_HAL              := '1'      # built-in if '1' ; otherwise not built-in

#-----------------------------------------------------------
LOCAL_SRC_FILES += 

#-----------------------------------------------------------
LOCAL_C_INCLUDES += 
#

#-----------------------------------------------------------
LOCAL_WHOLE_STATIC_LIBRARIES += libcam.paramsmgr.common
LOCAL_WHOLE_STATIC_LIBRARIES += libcam.paramsmgr.platform
#
ifeq "'1'" "$(strip $(MTKCAM_HAVE_CAMFEATURE))"
LOCAL_WHOLE_STATIC_LIBRARIES += libcam.paramsmgr.feature
endif
#
LOCAL_STATIC_LIBRARIES += 

#-----------------------------------------------------------
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += libdl
LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libcutils
#
LOCAL_SHARED_LIBRARIES += libcamera_client
#
LOCAL_SHARED_LIBRARIES += libcam.utils
#
ifeq "'1'" "$(strip $(MTKCAM_HAVE_SENSOR_HAL))"
LOCAL_SHARED_LIBRARIES += libcamdrv
endif
#
ifeq "'1'" "$(strip $(MTKCAM_HAVE_3A_HAL))"
LOCAL_SHARED_LIBRARIES += libfeatureio
endif
#

#-----------------------------------------------------------
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libcam.paramsmgr

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
include $(BUILD_SHARED_LIBRARY)



################################################################################
#
################################################################################
include $(CLEAR_VARS)
include $(call all-makefiles-under,$(LOCAL_PATH))



