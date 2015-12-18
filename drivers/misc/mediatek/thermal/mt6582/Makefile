obj-  := dummy.o

ifeq ($(CONFIG_THERMAL), y)
else
    obj-y += mtk_thermal_dummy.o
endif

obj-$(CONFIG_THERMAL) += mtk_thermal_platform.o
obj-$(CONFIG_THERMAL) += mtk_ts_pa_thput.o
#obj-$(CONFIG_THERMAL) += mtk_ts_skin.o

ifneq ($(CONFIG_MTK_COMBO_WIFI),)
    obj-$(CONFIG_THERMAL) += mtk_ts_wmt.o
endif

# Only included in non-EVB projects
ifeq ($(CONFIG_POWER_EXT), y)
else
    ifeq ($(CONFIG_MTK_SMART_BATTERY), y)
        # Always align the Makefile in mediatek/platform/mt6582/kernel/drivers/power/
        ifeq ($(CONFIG_MTK_NCP1851_SUPPORT),y)
            obj-$(CONFIG_THERMAL) += mtk_cooler_bcct.o
        else
            ifeq ($(CONFIG_MTK_BQ24196_SUPPORT),y)
                obj-$(CONFIG_THERMAL) += mtk_cooler_bcct.o
            else
                # FAN5405 or PMIC6320 pulse charging or new charger IC
                obj-$(CONFIG_THERMAL) += mtk_cooler_bcct.o
            endif
        endif
    endif
endif

obj-y += mtk_ts_cpu.o

ifeq ($(CONFIG_THERMAL),y)
    obj-y += mtk_ts_pmic.o
    obj-y += mtk_ts_pa.o
    obj-y += mtk_ts_abb.o
    #obj-y += mtk_ts_xtal.o
    ifeq ($(CONFIG_MTK_ENABLE_MD2),y)
    	ifeq ($(CONFIG_MTK_MD2_SUPPORT),3)
	    	obj-y += mtk_ts_tdpa.o
	    endif
    endif

    ifeq ($(CONFIG_POWER_EXT), y)
    else
        ifeq ($(CONFIG_MTK_SMART_BATTERY), y)
            # Always align the Makefile in mediatek/platform/mt6589/kernel/drivers/power/
            ifeq ($(CONFIG_MTK_NCP1851_SUPPORT),y)
                obj-y += mtk_ts_battery.o
            else
                ifeq ($(CONFIG_MTK_BQ24196_SUPPORT),y)
                    obj-y += mtk_ts_battery.o
                else
                    # FAN5405 or PMIC pulse charging
                    obj-y += mtk_ts_battery.o
                endif
            endif
        endif
    endif
    ifeq ($(CONFIG_MTK_PCB_BATTERY_SENSOR), y)
        obj-y += mtk_ts_battery2.o
    endif
endif


ifeq ($(CONFIG_MTK_TMP103_SUPPORT),y)
#obj-$(CONFIG_THERMAL) += mtk_ts_pcb.o
endif
