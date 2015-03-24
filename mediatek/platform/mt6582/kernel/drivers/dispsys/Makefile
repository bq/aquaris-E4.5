# drivers/dispsys

ccflags-y += -I$(MTK_PATH_SOURCE)/drivers/video/ 							\
             -I$(MTK_PATH_PLATFORM)/../../common/kernel/drivers/video/ 		\
             -I$(MTK_PATH_PLATFORM)/drivers/video/

ifeq ($(strip $(MTK_SEC_VIDEO_PATH_SUPPORT)),yes)
ifeq ($(strip $(TRUSTONIC_TEE_SUPPORT)),yes)
ccflags-y += -I$(MTK_PATH_PLATFORM)/drivers/gud/MobiCoreKernelApi/include \
             -I$(MTK_PATH_PLATFORM)/drivers/gud/MobiCoreKernelApi/public \
             -I$(MTK_PATH_SOURCE)/../trustzone/vendor/trustonic/t-base/trustlets/cmdq/cmdq_sec_dr/public \
             -I$(MTK_PATH_SOURCE)/../trustzone/vendor/trustonic/t-base/trustlets/cmdq/cmdq_sec_tl/public \
             -I$(MTK_PATH_SOURCE)/../trustzone/vendor/trustonic/t-base/trustlets/svp/TlSvp/Locals/Code/public \
             -I$(MTK_PATH_SOURCE)/../trustzone/vendor/trustonic/t-base/trustlets/svp/DrSvp/Locals/Code/public
endif
endif

#TODO: the header file under "protect-bsp" folder does not exist for customer release codebase.
ifeq ($(strip $(MTK_SEC_VIDEO_PATH_SUPPORT)),yes)
ifeq ($(strip $(TRUSTONIC_TEE_SUPPORT)),yes)
ccflags-y += -I$(MTK_PATH_SOURCE)/../protect-bsp/platform/mt6582/external/trustzone/trustlets/sec/Tlsec/Locals/Code/public \
             -I$(MTK_PATH_SOURCE)/../protect-bsp/platform/mt6582/external/trustzone/trustlets/cmdq/cmdq_sec_dr/Locals/Code/public \
             -I$(MTK_PATH_SOURCE)/../protect-bsp/platform/mt6582/external/trustzone/trustlets/cmdq/cmdq_sec_tl/Locals/Code/public
endif
endif

obj-y += ddp_drv.o
obj-y += ddp_ovl.o
obj-y += ddp_bls.o
obj-y += ddp_color.o
# obj-y += ddp_tdshp.o
obj-y += ddp_rdma.o
obj-y += ddp_wdma.o
# obj-y += ddp_scl.o
# obj-y += ddp_rot.o
obj-y += ddp_path.o
obj-y += ddp_debug.o
obj-y += ddp_cmdq.o
obj-y += ddp_cmdq_debug.o
obj-y += ddp_cmdq_sec.o
#obj-y += ddp_cmdq_record.o
#obj-y += ddp_cmdq_test.o
# obj-y += ddp_dpfd.o
obj-y += ddp_aal.o
obj-y += data_rgb888_64x64.o
obj-y += data_rgb888_64x64_golden.o
ifeq ($(strip $(MTK_SEC_VIDEO_PATH_SUPPORT)),yes)
ifeq ($(strip $(TRUSTONIC_TEE_SUPPORT)),yes)
obj-y += ddp_svp.o
endif
endif
# EOF
