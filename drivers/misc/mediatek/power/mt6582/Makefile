include $(srctree)/drivers/misc/mediatek/Makefile.custom

ccflags-y += -I$(srctree)/drivers/power/mediatek/$(CONFIG_MTK_PLATFORM)/$(ARCH_MTK_PROJECT)

obj-$(CONFIG_MTK_PMIC)          += pmic_mt6323.o upmu_common.o

ifeq ($(CONFIG_MTK_MT6333_SUPPORT),y)
    obj-$(CONFIG_MTK_PMIC) += mt6333.o
endif

obj-$(CONFIG_MTK_SMART_BATTERY) += battery_meter_hal.o 

#ifeq ($(CONFIG_MTK_BQ27541_SUPPORT),y)
#    obj-$(CONFIG_MTK_SMART_BATTERY) += bq27541.o
#endif
ifeq ($(CONFIG_MTK_BQ24196_SUPPORT),y) 
    obj-$(CONFIG_MTK_SMART_BATTERY) += charging_hw_bq24196.o bq24196.o tbl_helper_func.o
else
ifeq ($(CONFIG_MTK_NCP1851_SUPPORT),y) 
    obj-$(CONFIG_MTK_SMART_BATTERY) += ncp1851.o charging_hw_ncp1851.o tbl_helper_func.o
else
ifeq ($(MTK_NCP1854_SUPPORT),y) 
    obj-$(CONFIG_MTK_SMART_BATTERY) += ncp1854.o charging_hw_ncp1854.o tbl_helper_func.o
else
ifeq ($(CONFIG_MTK_FAN5405_SUPPORT),y)
    obj-$(CONFIG_MTK_SMART_BATTERY) += fan5405.o charging_hw_fan5405.o
else
ifeq ($(CONFIG_MTK_BQ24158_SUPPORT),y)
    obj-$(CONFIG_MTK_SMART_BATTERY) += bq24158.o charging_hw_bq24158.o
else
ifeq ($(MTK_BQ24296_SUPPORT),y) 
    obj-$(CONFIG_MTK_SMART_BATTERY) += bq24296.o charging_hw_bq24296.o tbl_helper_func.o
else
ifeq ($(CONFIG_MTK_SWCHR_SUPPORT),y)
    obj-$(CONFIG_MTK_SMART_BATTERY) += charging_hw_6333.o                
else
ifeq ($(CONFIG_MTK_SNB1058_SUPPORT),y)
    obj-$(CONFIG_MTK_SMART_BATTERY) += snb1058/                
else
ifeq ($(CONFIG_MTK_MAX8971_SUPPORT),y)
    obj-$(CONFIG_MTK_SMART_BATTERY) += max8971/                
else
ifeq ($(CONFIG_MTK_RT9536_SUPPORT),y)
    obj-$(CONFIG_MTK_SMART_BATTERY) += rt9536/                
else
    obj-$(CONFIG_MTK_SMART_BATTERY) += charging_hw_pmic.o 
endif 
endif 
endif 
endif 
endif 
endif 
endif 
endif
endif
endif
