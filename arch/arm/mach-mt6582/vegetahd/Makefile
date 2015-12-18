
include $(srctree)/drivers/misc/mediatek/Makefile.custom

# Module project related part

ifeq ($(CONFIG_MTK_SENSOR_SUPPORT),y)
ifeq ($(CONFIG_MTK_CM36283),y)
obj-y	+=  alsps/cm36283/
endif

ifeq ($(CONFIG_MTK_TMD2772_AUTO),y)
obj-y	+=  alsps/tmd2772_auto/
endif


ifeq ($(CONFIG_MTK_BMA050_NEW),y)
obj-y   +=  accelerometer/bma050-new/
endif

ifeq ($(CONFIG_MTK_LSM330),y)
obj-y   +=  accelerometer/lsm330/
endif

ifeq ($(CONFIG_MTK_L3GD20),y)
obj-y   +=  gyroscope/l3gd20/
endif

ifeq ($(CONFIG_MTK_MPU3000),y)
obj-y   +=  gyroscope/mpu3000/
endif

ifeq ($(CONFIG_MTK_MPU3000_NEW),y)
obj-y   +=  gyroscope/mpu3000-new/
endif
ifeq ($(CONFIG_MTK_BMM050_NEW),y)
obj-y	+=  magnetometer/bmm050-new/
endif

ifeq ($(CONFIG_MC3XXX_AUTO),y)
obj-y   +=  accelerometer/mc3xxx_auto/
endif

ifeq ($(CONFIG_KXTJ2_1009_AUTO),y)
obj-y   +=  accelerometer/kxtj2_1009_auto/
endif

ifeq ($(CONFIG_EPL2182_AUTO),y)
obj-y	+=  alsps/EPL2182_auto/
endif

ifeq ($(CONFIG_CKTPS),y)
obj-y	+=  alsps/cktps/
endif
ifeq ($(CONFIG_MTK_ST480_AUTO),y)
obj-y	+=  magnetometer/st480_auto/
endif

ifeq ($(CONFIG_MMC3524X_AUTO),y)
obj-y   +=  magnetometer/mmc3524x_auto/
endif
ifeq ($(CONFIG_MTK_MMC3516X_AUTO),y)
obj-y	+=  magnetometer/mmc3516x_auto/
endif
endif
ifneq ($(CONFIG_ARCH_MTK_PROJECT),)
ARCH_MTK_PROJECT := $(shell echo $(CONFIG_ARCH_MTK_PROJECT))
obj-y	+= core/
obj-y	+= dct/dct/
endif

#obj-y	+=  hdmi/
obj-y	+=  camera/camera/
obj-y	+=  vibrator/
obj-y	+=  leds/
obj-y	+=  keypad/
obj-y	+=  sound/
obj-y	+=  accdet/
obj-y	+=  partition/

