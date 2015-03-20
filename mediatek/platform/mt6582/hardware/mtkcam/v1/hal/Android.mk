################################################################################
#
################################################################################

LOCAL_PATH := $(call my-dir)

################################################################################
#
################################################################################
include $(CLEAR_VARS)

#-----------------------------------------------------------
#$(shell cd $(LOCAL_PATH); touch *)
MTKCAM_HAVE_DISPLAY_CLIENT      := '1'  # built-in if '1' ; otherwise not built-in
MTKCAM_HAVE_PREVIEW_CLIENT      := '1'  # built-in if '1' ; otherwise not built-in
MTKCAM_HAVE_RECORD_CLIENT       := '1'  # built-in if '1' ; otherwise not built-in
MTKCAM_HAVE_FD_CLIENT           := '1'  # built-in if '1' ; otherwise not built-in
#
MTKCAM_HAVE_SENSOR_HAL          := '1'  # built-in if '1' ; otherwise not built-in
MTKCAM_HAVE_3A_HAL              := '1'  # built-in if '1' ; otherwise not built-in

################################################################################
#
################################################################################
include $(CLEAR_VARS)
include $(call all-makefiles-under,$(LOCAL_PATH))



