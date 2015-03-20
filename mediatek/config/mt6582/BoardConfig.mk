# config.mk
#
# Product-specific compile-time definitions.
#
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_CPU_VARIANT := cortex-a7

# eMMC support
ifeq ($(MTK_EMMC_SUPPORT),yes)
TARGET_USERIMAGES_USE_EXT4:=true
TARGET_USERIMAGES_SPARSE_EXT_DISABLED := false
endif

TARGET_CPU_SMP := true
USE_CAMERA_STUB := true

TARGET_NO_FACTORYIMAGE := true

# for migrate build system
# temporarily open this two options
HAVE_HTC_AUDIO_DRIVER := true
#BOARD_USES_GENERIC_AUDIO := true
 
BOARD_USES_MTK_AUDIO := true

BOARD_EGL_CFG := $(BOARD_CONFIG_DIR)/egl.cfg

BOARD_MTK_LIBSENSORS_NAME :=
BOARD_MTK_LIB_SENSOR :=

# MTK, Baochu Wang, 20101130, Add A-GPS {
ifeq ($(MTK_AGPS_APP), yes)
   BOARD_AGPS_SUPL_LIBRARIES := true
else
   BOARD_AGPS_SUPL_LIBRARIES := false
endif
# MTK, Baochu Wang, 20101130, Add A-GPS }

ifeq ($(MTK_GPS_SUPPORT), yes)
  BOARD_GPS_LIBRARIES := true
else
  BOARD_GPS_LIBRARIES := false
endif

# MTK, Infinity, 20090720, Add WiFi {
ifeq ($(MTK_WLAN_SUPPORT), yes)
BOARD_CONNECTIVITY_VENDOR := MediaTek
BOARD_CONNECTIVITY_MODULE := conn_soc

WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_HOSTAPD_DRIVER := NL80211
BOARD_HOSTAPD_PRIVATE_LIB := lib_driver_cmd_mt66xx
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_mt66xx
WIFI_DRIVER_FW_PATH_PARAM:="/dev/wmtWifi"
WIFI_DRIVER_FW_PATH_STA:=STA
WIFI_DRIVER_FW_PATH_AP:=AP
WIFI_DRIVER_FW_PATH_P2P:=P2P
#HAVE_CUSTOM_WIFI_DRIVER_2 := true
#HAVE_INTERNAL_WPA_SUPPLICANT_CONF := true
#HAVE_CUSTOM_WIFI_HAL := mediatek
endif
# MTK, Infinity, 20090720, Add WiFi }

TARGET_KMODULES := true

TARGET_ARCH_VARIANT := armv7-a-neon

ifeq ($(strip $(MTK_NAND_PAGE_SIZE)), 4K)
  BOARD_NAND_PAGE_SIZE := 4096 -s 128
else
  BOARD_NAND_PAGE_SIZE := 2048 -s 64   # default 2K
endif

WITH_DEXPREOPT := false

# include all config files
include $(BOARD_CONFIG_DIR)/configs/*.mk

