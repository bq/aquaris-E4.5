CKT_AUTO_ADD_GLOBAL_DEFINE_BY_NAME = CKT_INTERPOLATION CKT_SUPPORT_AUTOTEST_MODE CKT_LOW_POWER_SUPPORT CKT_MAKE_ADB_DEVICES_NAME_TO_RANDOM RESPIRATION_LAMP TEMPERATURE_CONTROL_CHARGING
CKT_AUTO_ADD_GLOBAL_DEFINE_BY_NAME_VALUE =PROJ_NAME CUST_NAME SOFTCODE USB_MANUFACTURER_STRING USB_PRODUCT_STRING USB_STRING_SERIAL_IDX CUSTOM_EXIF_STRING_MAKE CUSTOM_EXIF_STRING_MODEL CUSTOM_EXIF_STRING_SOFTWARE CUSTOM_BTMTK_ANDROID_DEFAULT_REMOTE_NAME CUSTOM_BTMTK_ANDROID_DEFAULT_LOCAL_NAME 
CKT_AUTO_ADD_GLOBAL_DEFINE_BY_VALUE = PROJ_NAME
#############################
#############################
#############################

#项目的相关定义
PROJ_NAME = VEGETAHD
CUST_NAME = BQ
SOFTCODE = S0B
BASEVERNO = 202
#############################
#会用他设置ro.product.model
CKT_PRODUCT_MODEL=CKT_$(strip $(PROJ_NAME) )
#会用他设置缺省时区persist.sys.timezone
TIMEZONE=Europe/Amsterdam


############usb相关#################
USB_MANUFACTURER_STRING=$(strip $(CUST_NAME) )
USB_PRODUCT_STRING=$(strip $(CKT_PRODUCT_MODEL) )
USB_STRING_SERIAL_IDX=$(strip $(USB_PRODUCT_STRING) )

############exif相关#################
CUSTOM_EXIF_STRING_MAKE=$(strip $(CUST_NAME) )
CUSTOM_EXIF_STRING_MODEL=$(strip $(PROJ_NAME) )
CUSTOM_EXIF_STRING_SOFTWARE=""

############bt相关#################
CUSTOM_BTMTK_ANDROID_DEFAULT_LOCAL_NAME =$(strip $(PROJ_NAME) )_BT
CUSTOM_BTMTK_ANDROID_DEFAULT_REMOTE_NAME=$(strip $(PROJ_NAME) )_DEVICE

#############################
#功能的开关,会导入到mediatek/source/frameworks/featureoption/java/com/mediatek/featureoption/FeatureOption.java
#修改的时候注意,在 mediatek/build/tools/javaoption.pm中添加相关模块
#另外注意如果enable只可以用yes,不可以用其他
TESTA = yes
TESTB = no
TESTC = testc_none

#############################

#如果要固定版本号,请在这设置,否则注释调它,而不是留空!!!
#CKT_BUILD_VERNO = PANDORA-S0A_CKT_L2EN_111_111111
CKT_BUILD_INTERNAL_VERNO =VEGETA01A-S0B_BQ_L40EN_202_140331145500

#############################
#摄像头软件插值
CKT_INTERPOLATION = yes
#工厂模式自动测试开关
CKT_SUPPORT_AUTOTEST_MODE = no
#MTK的low power 测试
CKT_LOW_POWER_SUPPORT = no
#LED三色灯亮度渐变,
RESPIRATION_LAMP = yes
#后续包括在研的项目4.35V电池的充电限制请严格执行以下限制条件
# 1. 0℃以下，不允许充电
# 2. 0℃-10℃，充电电流0.1C且≤400mA，充电限制电压4.2V
# 3. 10℃-45℃，充电电流＜0.5C，充电限制电压4.35V
# 4. 45℃以上，不允许充电
TEMPERATURE_CONTROL_CHARGING = no


#打开自动版本号切换功能
CKT_VERSION_AUTO_SWITCH=yes
export CKT_VERSION_AUTO_SWITCH


#ckt helin 20131202 make adb devices name to be random
CKT_MAKE_ADB_DEVICES_NAME_TO_RANDOM=yes
export CKT_MAKE_ADB_DEVICES_NAME_TO_RANDOM

#ckt helin 20131210 add FlashLight apk to System
CKT_APP_FLASHLIGHT = no
export CKT_APP_FLASHLIGHT








































###########以下为产生的东西,一般不需要理会
_CKT_BUILD_VERNO  = $(strip $(PROJ_NAME) )-$(strip $(SOFTCODE) )_$(strip $(CUST_NAME) )_L$(words $(subst hdpi, ,$(strip $(MTK_PRODUCT_LOCALES))))$(word 1,$(subst _, ,$(subst zh_TW,TR,$(subst zh_CN,SM,$(strip $(MTK_PRODUCT_LOCALES))))))_$(strip $(BASEVERNO))

DATA_FOR_VERO=$(shell date +%y%m%d)
DATA_FOR_INTERNAL_VERO=$(shell date +%y%m%d%H%M%S)

CKT_BUILD_VERNO  ?= $(call uc, $(_CKT_BUILD_VERNO)_$(strip $(DATA_FOR_VERO)))
CKT_BUILD_INTERNAL_VERNO  ?= $(call uc, $(_CKT_BUILD_VERNO)_$(strip $(DATA_FOR_INTERNAL_VERO)))
#############################

