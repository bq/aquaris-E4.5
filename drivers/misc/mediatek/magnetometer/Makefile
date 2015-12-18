include $(srctree)/drivers/misc/mediatek/Makefile.custom

# In case the platform does NOT support this type of sensors
obj-y   +=  mag.o
obj-y   +=  mag_factory.o

ifeq ($(CONFIG_MTK_ST480_AUTO),y)
obj-y	+=  st480_auto/
endif

ifeq ($(CONFIG_MTK_MMC3516X_AUTO),y)
obj-y	+=  mmc3516x_auto/
endif

ifeq ($(CONFIG_MTK_AKM8963_NEW),y)
obj-y	+=  akm8963-new/
endif

ifeq ($(CONFIG_MTK_BMM050_NEW),y)
obj-y	+=  bmm050-new/
endif

ifeq ($(CONFIG_MTK_BMM056_NEW),y)
obj-y	+=  bmm056-new/
endif
ifeq ($(CONFIG_MTK_AKM8963),y)
obj-y	+=  akm8963/
endif

ifeq ($(CONFIG_MTK_YAS532),y)
obj-y   +=  yamaha532/
endif

ifeq ($(CONFIG_ST480_AUTO),y)
obj-y   +=  st480_auto/
endif

ifeq ($(CONFIG_MMC3524X_AUTO),y)
obj-y   +=  mmc3524x_auto/
endif

ifeq ($(CONFIG_MMC3516X_AUTO),y)
obj-y   +=  mmc3516x_auto/
endif
