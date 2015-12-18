include $(srctree)/drivers/misc/mediatek/Makefile.custom

obj-$(CONFIG_MTK_SMART_BATTERY) += battery_meter.o battery_common.o

ifeq ($(CONFIG_MTK_FAN5402_SUPPORT),y)
    obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
else ifeq ($(CONFIG_MTK_FAN5405_SUPPORT),y)
        obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o 
else
    ifeq ($(CONFIG_MTK_SWCHR_SUPPORT),y)
        obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
    else
        ifeq ($(CONFIG_MTK_BQ24158_SUPPORT),y)
            obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o 
        else
            ifeq ($(CONFIG_MTK_BQ24196_SUPPORT),y)
                obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
            else
                ifeq ($(CONFIG_MTK_BQ24296_SUPPORT),y)
                    obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
                else
                    ifeq ($(CONFIG_MTK_NCP1851_SUPPORT),y)
                        obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
                    else
                        ifeq ($(CONFIG_MTK_NCP1854_SUPPORT),y)
                            obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
                        else
                            ifeq ($(CONFIG_MTK_BQ24160_SUPPORT),y)
                                obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
                            else
                                ifeq ($(CONFIG_MTK_BQ24157_SUPPORT),y)
                                    obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
                                else
                                    ifeq ($(CONFIG_MTK_BQ24250_SUPPORT),y)
                                        obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o 
                                    else
                                        ifeq ($(CONFIG_MTK_BQ24261_SUPPORT),y)
                                            obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
                                        else
                                            ifeq ($(CONFIG_MTK_RT9536_SUPPORT),y)
                                                obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
                                            else
                                                obj-$(CONFIG_MTK_SMART_BATTERY) += linear_charging.o
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
    endif
endif  
