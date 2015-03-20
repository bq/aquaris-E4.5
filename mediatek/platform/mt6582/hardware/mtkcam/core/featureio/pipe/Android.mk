#===============================================================================


LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \

  
LOCAL_C_INCLUDES:= \

LOCAL_STATIC_LIBRARIES := \
#     libfeatureiopipe_fdft

LOCAL_WHOLE_STATIC_LIBRARIES := \
    libfeatureiopipe_aaa \

ifneq ($(BUILD_MTK_LDVT),true)
    LOCAL_WHOLE_STATIC_LIBRARIES += libfeatureiopipe_eis
    LOCAL_WHOLE_STATIC_LIBRARIES += libfeatureiopipe_fdft
    LOCAL_WHOLE_STATIC_LIBRARIES += libfeatureiopipe_flk
    LOCAL_WHOLE_STATIC_LIBRARIES += libfeatureiopipe_hdr
    LOCAL_WHOLE_STATIC_LIBRARIES += libfeatureiopipe_asd
    LOCAL_WHOLE_STATIC_LIBRARIES += libcam.featureio.pipe.facebeauty
    LOCAL_WHOLE_STATIC_LIBRARIES += libcam.featureio.pipe.panorama
    LOCAL_WHOLE_STATIC_LIBRARIES += libcam.featureio.pipe.3DFeature
	ifeq (yes,$(MTK_MOTION_TRACK_SUPPORT))
    LOCAL_WHOLE_STATIC_LIBRARIES += libcam.featureio.pipe.motiontrack
	endif
endif
    
LOCAL_MODULE := libfeatureiopipe


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



include $(call all-makefiles-under,$(LOCAL_PATH))


