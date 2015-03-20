LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(MTK_PLATFORM),MT6589)

LOCAL_MODULE_TAGS := eng

LOCAL_SRC_FILES:= \
	m4u_it.cpp \

LOCAL_C_INCLUDES:= \
	$(TOP)/$(MTK_PATH_SOURCE)/external/mhal/src/core/drv/dispsys/src/engine \
	$(TOP)/$(MTK_PATH_SOURCE)/external/mhal/src/core/drv/dispsys/src/engine/6589 \
	$(TOP)/$(MTK_PATH_SOURCE)/external/mhal/src/core/drv/inc \
	$(MTK_PATH_PLATFORM)hardware/m4u \
	#$(TOP)/$(MTK_PATH_SOURCE)/kernel/drivers/video \

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdispsys \
	libmhaldrv\

LOCAL_STATIC_LIBRARIES := \

LOCAL_WHOLE_STATIC_LIBRARIES := \


LOCAL_MODULE := m4u_it

LOCAL_CFLAGS += -O0

include $(BUILD_EXECUTABLE)

include $(call all-makefiles-under,$(LOCAL_PATH))

endif


