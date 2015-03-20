/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ***   To edit the content of this header, modify the corresponding
 ***   source file (e.g. under external/kernel-headers/original/) then
 ***   run bionic/libc/kernel/tools/update_all.py
 ***
 ***   Any manual change here will be lost the next time this script will
 ***   be run. You've been warned!
 ***
 ****************************************************************************
 ****************************************************************************/
#ifndef __ASM_ARCH_OMAP_MCBSP_H
#define __ASM_ARCH_OMAP_MCBSP_H
#include <asm/hardware.h>
#define OMAP730_MCBSP1_BASE 0xfffb1000
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define OMAP730_MCBSP2_BASE 0xfffb1800
#define OMAP1510_MCBSP1_BASE 0xe1011800
#define OMAP1510_MCBSP2_BASE 0xfffb1000
#define OMAP1510_MCBSP3_BASE 0xe1017000
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define OMAP1610_MCBSP1_BASE 0xe1011800
#define OMAP1610_MCBSP2_BASE 0xfffb1000
#define OMAP1610_MCBSP3_BASE 0xe1017000
#define OMAP24XX_MCBSP1_BASE 0x48074000
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define OMAP24XX_MCBSP2_BASE 0x48076000
#define OMAP_MCBSP_READ(base, reg) __raw_readw((base) + OMAP_MCBSP_REG_##reg)
#define OMAP_MCBSP_WRITE(base, reg, val) __raw_writew((val), (base) + OMAP_MCBSP_REG_##reg)
#define RRST 0x0001
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define RRDY 0x0002
#define RFULL 0x0004
#define RSYNC_ERR 0x0008
#define RINTM(value) ((value)<<4)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define ABIS 0x0040
#define DXENA 0x0080
#define CLKSTP(value) ((value)<<11)
#define RJUST(value) ((value)<<13)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DLB 0x8000
#define XRST 0x0001
#define XRDY 0x0002
#define XEMPTY 0x0004
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define XSYNC_ERR 0x0008
#define XINTM(value) ((value)<<4)
#define GRST 0x0040
#define FRST 0x0080
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define SOFT 0x0100
#define FREE 0x0200
#define CLKRP 0x0001
#define CLKXP 0x0002
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define FSRP 0x0004
#define FSXP 0x0008
#define DR_STAT 0x0010
#define DX_STAT 0x0020
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define CLKS_STAT 0x0040
#define SCLKME 0x0080
#define CLKRM 0x0100
#define CLKXM 0x0200
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define FSRM 0x0400
#define FSXM 0x0800
#define RIOEN 0x1000
#define XIOEN 0x2000
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define IDLE_EN 0x4000
#define RWDLEN1(value) ((value)<<5)
#define RFRLEN1(value) ((value)<<8)
#define XWDLEN1(value) ((value)<<5)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define XFRLEN1(value) ((value)<<8)
#define RDATDLY(value) (value)
#define RFIG 0x0004
#define RCOMPAND(value) ((value)<<3)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define RWDLEN2(value) ((value)<<5)
#define RFRLEN2(value) ((value)<<8)
#define RPHASE 0x8000
#define XDATDLY(value) (value)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define XFIG 0x0004
#define XCOMPAND(value) ((value)<<3)
#define XWDLEN2(value) ((value)<<5)
#define XFRLEN2(value) ((value)<<8)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define XPHASE 0x8000
#define CLKGDV(value) (value)
#define FWID(value) ((value)<<8)
#define FPER(value) (value)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define FSGM 0x1000
#define CLKSM 0x2000
#define CLKSP 0x4000
#define GSYNC 0x8000
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define RMCM 0x0001
#define RCBLK(value) ((value)<<2)
#define RPABLK(value) ((value)<<5)
#define RPBBLK(value) ((value)<<7)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define XMCM(value) (value)
#define XCBLK(value) ((value)<<2)
#define XPABLK(value) ((value)<<5)
#define XPBBLK(value) ((value)<<7)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct omap_mcbsp_reg_cfg {
 u16 spcr2;
 u16 spcr1;
 u16 rcr2;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 u16 rcr1;
 u16 xcr2;
 u16 xcr1;
 u16 srgr2;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 u16 srgr1;
 u16 mcr2;
 u16 mcr1;
 u16 pcr0;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 u16 rcerc;
 u16 rcerd;
 u16 xcerc;
 u16 xcerd;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 u16 rcere;
 u16 rcerf;
 u16 xcere;
 u16 xcerf;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 u16 rcerg;
 u16 rcerh;
 u16 xcerg;
 u16 xcerh;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
typedef enum {
 OMAP_MCBSP1 = 0,
 OMAP_MCBSP2,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 OMAP_MCBSP3,
} omap_mcbsp_id;
typedef int __bitwise omap_mcbsp_io_type_t;
#define OMAP_MCBSP_IRQ_IO ((__force omap_mcbsp_io_type_t) 1)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define OMAP_MCBSP_POLL_IO ((__force omap_mcbsp_io_type_t) 2)
typedef enum {
 OMAP_MCBSP_WORD_8 = 0,
 OMAP_MCBSP_WORD_12,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 OMAP_MCBSP_WORD_16,
 OMAP_MCBSP_WORD_20,
 OMAP_MCBSP_WORD_24,
 OMAP_MCBSP_WORD_32,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
} omap_mcbsp_word_length;
typedef enum {
 OMAP_MCBSP_CLK_RISING = 0,
 OMAP_MCBSP_CLK_FALLING,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
} omap_mcbsp_clk_polarity;
typedef enum {
 OMAP_MCBSP_FS_ACTIVE_HIGH = 0,
 OMAP_MCBSP_FS_ACTIVE_LOW,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
} omap_mcbsp_fs_polarity;
typedef enum {
 OMAP_MCBSP_CLK_STP_MODE_NO_DELAY = 0,
 OMAP_MCBSP_CLK_STP_MODE_DELAY,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
} omap_mcbsp_clk_stp_mode;
typedef enum {
 OMAP_MCBSP_SPI_MASTER = 0,
 OMAP_MCBSP_SPI_SLAVE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
} omap_mcbsp_spi_mode;
struct omap_mcbsp_spi_cfg {
 omap_mcbsp_spi_mode spi_mode;
 omap_mcbsp_clk_polarity rx_clock_polarity;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 omap_mcbsp_clk_polarity tx_clock_polarity;
 omap_mcbsp_fs_polarity fsx_polarity;
 u8 clk_div;
 omap_mcbsp_clk_stp_mode clk_stp_mode;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 omap_mcbsp_word_length word_length;
};
#endif
