
#ifndef MT8193_H
#define MT8193_H

#include "typedefs.h"
#include "platform.h"


#define CKGEN_BASE                       0x1000



#define REG_RW_BUS_CKCFG                 0x000 
#define CLK_BUS_SEL_XTAL                 0
#define CLK_BUS_SEL_NFIPLL_D2                 1
#define CLK_BUS_SEL_NFIPLL_D3                 2
#define CLK_BUS_SEL_XTAL_D2                 3
#define CLK_BUS_SEL_32K                4
#define CLK_BUS_SEL_PAD_DPI0                 5
#define CLK_BUS_SEL_PAD_DPI1                 6
#define CLK_BUS_SEL_ROSC                 7


#define REG_RW_NFI_CKCFG                 0x008 
#define CLK_NFI_SEL_NFIPLL               0
#define CLK_NFI_SEL_NFIPLL_D2            1
#define CLK_NFI_SEL_NFIPLL_D3            2
#define CLK_NFI_SEL_XTAL_D1              3
#define CLK_PDN_NFI                      (1U<<7) 


#define REG_RW_HDMI_PLL_CKCFG             0x00c
#define CLK_HDMI_PLL_SEL_HDMIPLL               0
#define CLK_HDMI_PLL_SEL_32K                  1
#define CLK_HDMI_PLL_SEL_XTAL_D1            2
#define CLK_HDMI_PLL_SEL_NFIPLL             3
#define CLK_PDN_HDMI_PLL                    (1U<<7) 




#define REG_RW_HDMI_DISP_CKCFG           0x010
#define CLK_HDMI_DISP_SEL_DISP               0
#define CLK_HDMI_DISP_SEL_32K                  1
#define CLK_HDMI_DISP_SEL_XTAL_D1            2
#define CLK_HDMI_DISP_SEL_NFIPLL             3
#define CLK_PDN_HDMI_DISP                      (1U<<7) 


#define REG_RW_LVDS_DISP_CKCFG           0x014
#define CLK_LVDS_DISP_SEL_AD_VPLL_DPIX         0
#define CLK_LVDS_DISP_SEL_32K                  1
#define CLK_LVDS_DISP_SEL_XTAL_D1            2
#define CLK_LVDS_DISP_SEL_NFIPLL             3
#define CLK_PDN_LVDS_DISP                      (1U<<7) 



#define REG_RW_LVDS_CTS_CKCFG            0x018
#define CLK_LVDS_CTS_SEL_AD_VPLL_DPIX         0
#define CLK_LVDS_CTS_SEL_32K                  1
#define CLK_LVDS_CTS_SEL_XTAL_D1            2
#define CLK_LVDS_CTS_SEL_NFIPLL             3
#define CLK_PDN_LVDS_CTS                      (1U<<7)

#define REG_RW_CKMISC_CTRL            0x050
#define CKGEN_CKMISC_CTRL_DCXO_MODE_EN         1

#define REG_RW_GPIO_EN                       0x144
#define GPIO_EN_PAD_INT_OUT_EN        (1U<<0)

#define REG_RW_GPIO_OUT_2                    0x120
#define GPIO_OUT_2_PAD_INT_OUT_HIGH        (1U<<0)





#define REG_RW_PMUX0                 0x200
#define REG_RW_PMUX1                 0x204
#define REG_RW_PMUX2                 0x208
#define REG_RW_PMUX3                 0x20c
#define REG_RW_PMUX4                 0x210
#define REG_RW_PMUX5                 0x214
#define REG_RW_PMUX6                 0x218

#define REG_RW_PMUX7                 0x21c
#define PMUX7_PAD_INT_SHIFT   (15)

#define REG_RW_PMUX8                 0x220
#define REG_RW_PMUX9                 0x224


#define REG_RW_LVDSWRAP_CTRL1                0x254
#define CKGEN_LVDSWRAP_CTRL1_NFIPLL_MON_EN   (1U<<7)
#define CKGEN_LVDSWRAP_CTRL1_DCXO_POR_MON_EN   (1U<<8)

#define REG_RW_PLL_GPANACFG0              0x34c

#define REG_RW_PLL_GPANACFG2              0x354

#define REG_RW_PLL_GPANACFG3              0x358

#define REG_RW_LVDS_ANACFG2              0x318

#define REG_RW_LVDS_ANACFG3              0x31c

#define REG_RW_LVDS_ANACFG4              0x320

/* DCXO */

#define REG_RW_DCXO_ANACFG2              0x308
#define DCXO_ANACFG2_LDO4_EN             (1U<<2)
#define DCXO_ANACFG2_LDO4_MAN_EN         (1U<<3)
#define DCXO_ANACFG2_LDO3_EN             (1U<<4)
#define DCXO_ANACFG2_LDO3_MAN_EN         (1U<<5)
#define DCXO_ANACFG2_LDO2_EN             (1U<<6)
#define DCXO_ANACFG2_LDO2_MAN_EN         (1U<<7)
#define DCXO_ANACFG2_LDO1_EN             (1U<<8)
#define DCXO_ANACFG2_LDO1_MAN_EN         (1U<<9)
#define DCXO_ANACFG2_PO_MAN              (1U<<29)




#define REG_RW_DCXO_ANACFG4              0x370
#define DCXO_ANACFG4_BT_MAN             (1U<<18)
#define DCXO_ANACFG4_EXT2_MAN           (1U<<19)
#define DCXO_ANACFG4_EXT1_MAN           (1U<<20)















#if 0
int mt8193_i2c_write(u16 addr, u32 data)
{
    
    return 0;
}

u32 mt8193_i2c_read(u16 addr)
{
    return 0;
}

#endif



#define IO_READ8(base, offset)                          mt8193_i2c_read8((base) + (offset))
#define IO_READ16(base, offset)                         mt8193_i2c_read16((base) + (offset))
#define IO_READ32(base, offset)                         mt8193_i2c_read32((base) + (offset))

//============================================================================
// Macros for register write
//============================================================================
#define IO_WRITE8(base, offset, value)                  mt8193_i2c_write8((base) + (offset), (value))
#define IO_WRITE16(base, offset, value)                 mt8193_i2c_write16((base) + (offset), (value))
#define IO_WRITE32(base, offset, value)                 mt8193_i2c_write32((base) + (offset), (value))



#define CKGEN_READ8(offset)            IO_READ8(CKGEN_BASE, (offset))
#define CKGEN_READ16(offset)           IO_READ16(CKGEN_BASE, (offset))
#define CKGEN_READ32(offset)           IO_READ32(CKGEN_BASE, (offset))

#define CKGEN_WRITE8(offset, value)    IO_WRITE8(CKGEN_BASE, (offset), (value))
#define CKGEN_WRITE16(offset, value)   IO_WRITE16(CKGEN_BASE, (offset), (value))
#define CKGEN_WRITE32(offset, value)   IO_WRITE32(CKGEN_BASE, (offset), (value))



/*=======================================================================*/
/* Constant Definitions                                                  */
/*=======================================================================*/

typedef enum
{
    SRC_CK_APLL,
    SRC_CK_ARMPLL,
    SRC_CK_VDPLL,
    SRC_CK_DMPLL,
    SRC_CK_SYSPLL1,
    SRC_CK_SYSPLL2,
    SRC_CK_USBCK,
    SRC_CK_MEMPLL,
    SRC_CK_MCK
} SRC_CK_T;

typedef enum
{
    e_CLK_NFI,             //0   0x70.3
    e_CLK_HDMIPLL,            //1   0x70.7
    e_CLK_HDMIDISP,
    e_CLK_LVDSDISP,
    e_CLK_LVDSCTS,
    e_CLK_MAX              //2
} e_CLK_T;

typedef enum
{
    e_CKEN_NFI,            //0   0x300.31
    e_CKEN_HDMI,          //1   0x300.29
    e_CKEN_MAX               //2
} e_CKEN_T;

#endif // MT8193_H


