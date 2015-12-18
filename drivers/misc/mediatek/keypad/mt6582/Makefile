include $(srctree)/drivers/misc/mediatek/Makefile.custom

ccflags-y += -I$(srctree)/drivers/input/keyboard/mediatek/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)

ccflags-y += -I$(MTK_PROJECT_PATH_ROOT)/keypad/

obj-$(CONFIG_MTK_KEYPAD) := hal_kpd.o
