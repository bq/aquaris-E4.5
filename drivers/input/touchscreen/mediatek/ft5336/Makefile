include $(srctree)/drivers/misc/mediatek/Makefile.custom

# Linux driver folder
ccflags-y += -I$(srctree)/arch/arm/mach-$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/touchpanel/ft5336/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/ft5336/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/

obj-y	+=  ft5336_driver.o
#obj-y	+=  focaltech_ctl.o
#obj-y	+=  ft5x06_ts.o

