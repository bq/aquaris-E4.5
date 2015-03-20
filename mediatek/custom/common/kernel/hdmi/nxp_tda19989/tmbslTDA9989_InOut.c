
/*============================================================================*/
/*                       INCLUDE FILES                                        */
/*============================================================================*/
#ifndef TMFL_TDA19989 
#define TMFL_TDA19989 
#endif

#ifndef TMFL_NO_RTOS 
#define TMFL_NO_RTOS 
#endif

#ifndef TMFL_LINUX_OS_KERNEL_DRIVER
#define TMFL_LINUX_OS_KERNEL_DRIVER
#endif



#ifdef TMFL_LINUX_OS_KERNEL_DRIVER
#include <linux/kernel.h>
#include <linux/module.h>
#endif

#include "tmbslHdmiTx_types.h"
#include "tmbslTDA9989_Functions.h"
#include "tmbslTDA9989_local.h"
#include "tmbslTDA9989_HDCP_l.h"
#include "tmbslTDA9989_State_l.h"
#include "tmbslTDA9989_Misc_l.h"
#include "tmbslTDA9989_InOut_l.h"

/*============================================================================*/
/*                     TYPES DECLARATIONS                                     */
/*============================================================================*/

#define SSD_UNUSED_VALUE 0xF0

#ifdef FORMAT_PC
#define DEPTH_COLOR_PC 1  /* PC_FORMAT only 8 bits available */
#endif /* FORMAT_PC */

#define REG_VAL_SEL_AIP_SPDIF   0
#define REG_VAL_SEL_AIP_I2S     1
#define REG_VAL_SEL_AIP_OBA     2
#define REG_VAL_SEL_AIP_DST     3
#define REG_VAL_SEL_AIP_HBR     5

struct vic2reg {
   unsigned char vic;
   unsigned char reg;
};

struct sync_desc {
   UInt16 Vs2;
   UInt8 pix_rep;
   UInt8 v_toggle;
   UInt8 h_toggle;
   UInt16 hfp;    /* Output values for Vs/Hs input sync */
   UInt16 vfp;
   UInt16 href; /* Output values for all other input sync sources */
   UInt16 vref;
};

/*============================================================================*/
/*                       CONSTANTS DECLARATIONS EXPORTED                      */
/*============================================================================*/

extern tmHdmiTxRegMaskVal_t kVoutHdcpOff[];
extern tmHdmiTxRegMaskVal_t kVoutHdcpOn[];

/**
 * Lookup table of input port control registers and their swap and mirror masks
 */
tmbslTDA9989RegVip
 kRegVip[HDMITX_VIN_PORT_MAP_TABLE_LEN] =
{
    {E_REG_P00_VIP_CNTRL_0_W,
        E_MASKREG_P00_VIP_CNTRL_0_swap_a,
        E_MASKREG_P00_VIP_CNTRL_0_mirr_a
    }, /* Port group 0 */
    {E_REG_P00_VIP_CNTRL_0_W,
        E_MASKREG_P00_VIP_CNTRL_0_swap_b,
        E_MASKREG_P00_VIP_CNTRL_0_mirr_b
    }, /* Port group 1 */
    {E_REG_P00_VIP_CNTRL_1_W,
        E_MASKREG_P00_VIP_CNTRL_1_swap_c,
        E_MASKREG_P00_VIP_CNTRL_1_mirr_c
    }, /* Port group 2 */
    {E_REG_P00_VIP_CNTRL_1_W,
        E_MASKREG_P00_VIP_CNTRL_1_swap_d,
        E_MASKREG_P00_VIP_CNTRL_1_mirr_d
    }, /* Port group 3 */
    {E_REG_P00_VIP_CNTRL_2_W,
        E_MASKREG_P00_VIP_CNTRL_2_swap_e,
        E_MASKREG_P00_VIP_CNTRL_2_mirr_e
    }, /* Port group 4 */
    {E_REG_P00_VIP_CNTRL_2_W,
        E_MASKREG_P00_VIP_CNTRL_2_swap_f,
        E_MASKREG_P00_VIP_CNTRL_2_mirr_f
    }  /* Port group 5 */
};

/**
 * Table of PLL settings registers to configure for all video input format (vinFmt)
 */
tmHdmiTxRegMaskVal_t kCommonPllCfg[] =
{
    {E_REG_P02_PLL_SERIAL_1_RW, E_MASKREG_ALL,  0x00},
    {E_REG_P02_PLL_SERIAL_2_RW, E_MASKREG_ALL,  0x01},
    {E_REG_P02_PLL_SERIAL_3_RW, E_MASKREG_ALL,  0x00},
    {E_REG_P02_SERIALIZER_RW,   E_MASKREG_ALL,  0x00},
    {E_REG_P02_BUFFER_OUT_RW,   E_MASKREG_ALL,  0x00},
    {E_REG_P02_PLL_SCG1_RW,     E_MASKREG_ALL,  0x00},
    {E_REG_P02_AUDIO_DIV_RW,    E_MASKREG_ALL,  0x03},
    /*{E_REG_P02_TEST2_RW,        E_MASKREG_ALL,  0x00},*/
    {E_REG_P02_SEL_CLK_RW,      E_MASKREG_ALL,  0x09},
    {0,0,0}
};

/**
 * Table of PLL settings registers to configure double mode pixel rate,
 * vinFmt other than 480i or 576i
 */
tmHdmiTxRegMaskVal_t kDoublePrateVfmtOtherPllCfg[] =
{
    {E_REG_P02_PLL_SCG2_RW,     E_MASKREG_ALL,  0x00},
    {0,0,0}
};

/**
 * Table of PLL settings registers to configure for single mode pixel rate,
 * vinFmt 480i or 576i only
 */
tmHdmiTxRegMaskVal_t kSinglePrateVfmt480i576iPllCfg[] =
{
    {E_REG_P02_PLL_SCG2_RW,     E_MASKREG_ALL,  0x11},
    {0,0,0}
};

/**
 * Table of PLL settings registers to configure single mode pixel rate,
 * vinFmt other than 480i or 576i
 */
tmHdmiTxRegMaskVal_t kSinglePrateVfmtOtherPllCfg[] =
{
    {E_REG_P02_PLL_SCG2_RW,     E_MASKREG_ALL,  0x10},
    {0,0,0}
};

/**
 * Table of PLL settings registers to configure for single repeated mode pixel rate,
 * vinFmt 480i or 576i only
 */
tmHdmiTxRegMaskVal_t kSrepeatedPrateVfmt480i576iPllCfg[] =
{
    {E_REG_P02_PLL_SCG2_RW,     E_MASKREG_ALL,  0x01},
    {0,0,0}
};

/**
 * Table of PLL settings registers to configure for 480i and 576i vinFmt
 */
tmHdmiTxRegMaskVal_t kVfmt480i576iPllCfg[] =
{
    {E_REG_P02_PLL_SCGN1_RW,    E_MASKREG_ALL,  0x14},
    {E_REG_P02_PLL_SCGN2_RW,    E_MASKREG_ALL,  0x00},
    {E_REG_P02_PLL_SCGR1_RW,    E_MASKREG_ALL,  0x0A},
    {E_REG_P02_PLL_SCGR2_RW,    E_MASKREG_ALL,  0x00},
    {0,0,0}
};

/**
 * Table of PLL settings registers to configure for other vinFmt than 480i and 576i
 */
tmHdmiTxRegMaskVal_t kVfmtOtherPllCfg[] =
{
    {E_REG_P02_PLL_SCGN1_RW,    E_MASKREG_ALL,  0xFA},
    {E_REG_P02_PLL_SCGN2_RW,    E_MASKREG_ALL,  0x00},
    {E_REG_P02_PLL_SCGR1_RW,    E_MASKREG_ALL,  0x5B},
    {E_REG_P02_PLL_SCGR2_RW,    E_MASKREG_ALL,  0x00},
    {0,0,0}
};

/**
 * Lookup table to convert from EIA/CEA TV video formats used in the EDID and
 * in API parameters to pixel clock frequencies, according to SCS Table
 * "HDMI Pixel Clock Frequencies per EIA/CEA-861B Video Output Format".
 * The other index is the veritical frame frequency.
 */
 
UInt8 kVfmtToPixClk_TV[HDMITX_VFMT_TV_MAX][HDMITX_VFREQ_NUM] =
{
  /* HDMITX_VFREQ_24Hz HDMITX_VFREQ_25Hz HDMITX_VFREQ_30Hz  HDMITX_VFREQ_50Hz HDMITX_VFREQ_59Hz HDMITX_VFREQ_60Hz */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_25175,   E_PIXCLK_25200},     /* HDMITX_VFMT_01_640x480p_60Hz   */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_27000,   E_PIXCLK_27027},     /* HDMITX_VFMT_02_720x480p_60Hz   */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_27000,   E_PIXCLK_27027},     /* HDMITX_VFMT_03_720x480p_60Hz   */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_74175,   E_PIXCLK_74250},     /* HDMITX_VFMT_04_1280x720p_60Hz  */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_74175,   E_PIXCLK_74250},     /* HDMITX_VFMT_05_1920x1080i_60Hz */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_27000,   E_PIXCLK_27027},     /* HDMITX_VFMT_06_720x480i_60Hz   */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_27000,   E_PIXCLK_27027},     /* HDMITX_VFMT_07_720x480i_60Hz   */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_27000,   E_PIXCLK_27027},     /* HDMITX_VFMT_08_720x240p_60Hz   */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_27000,   E_PIXCLK_27027},     /* HDMITX_VFMT_09_720x240p_60Hz   */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_54000,   E_PIXCLK_54054},     /* HDMITX_VFMT_10_720x480i_60Hz   */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_54000,   E_PIXCLK_54054},     /* HDMITX_VFMT_11_720x480i_60Hz   */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_54000,   E_PIXCLK_54054},     /* HDMITX_VFMT_12_720x240p_60Hz   */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_54000,   E_PIXCLK_54054},     /* HDMITX_VFMT_13_720x240p_60Hz   */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_54000,   E_PIXCLK_54054},     /* HDMITX_VFMT_14_1440x480p_60Hz  */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_54000,   E_PIXCLK_54054},     /* HDMITX_VFMT_15_1440x480p_60Hz  */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_148350,  E_PIXCLK_148500},    /* HDMITX_VFMT_16_1920x1080p_60Hz */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_27000,   E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_17_720x576p_50Hz   */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_27000,   E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_18_720x576p_50Hz   */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_74250,   E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_19_1280x720p_50Hz  */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_74250,   E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_20_1920x1080i_50Hz */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_27000,   E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_21_720x576i_50Hz   */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_27000,   E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_22_720x576i_50Hz   */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_27000,   E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_23_720x288p_50Hz   */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_27000,   E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_24_720x288p_50Hz   */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_54000,   E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_25_720x576i_50Hz   */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_54000,   E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_26_720x576i_50Hz   */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_54000,   E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_27_720x288p_50Hz   */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_54000,   E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_28_720x288p_50Hz   */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_54000,   E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_29_1440x576p_50Hz  */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_54000,   E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_30_1440x576p_50Hz  */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_148500,  E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_31_1920x1080p_50Hz */
    {E_PIXCLK_74250,   E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_32_1920x1080p_24Hz */
    {E_PIXCLK_INVALID, E_PIXCLK_74250,   E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_33_1920x1080p_25Hz */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_74250,   E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID},    /* HDMITX_VFMT_34_1920x1080p_30Hz */

    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_108000, E_PIXCLK_108108},     /* HDMITX_VFMT_35_2880x480p_60Hz  */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_108000, E_PIXCLK_108108},     /* HDMITX_VFMT_36_2880x480p_60Hz  */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_108000,  E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_37_2880x576p_50Hz  */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_108000,  E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_38_2880x576p_50Hz  */

    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_39_  */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_40_  */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_41_  */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_42_  */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_43_  */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_44_  */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_45_  */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_46_  */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_47_  */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_48_  */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_49_  */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_50_  */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_51_  */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_52_  */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_53_  */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_54_  */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_55_  */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_56_  */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_57_  */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_58_  */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_59_  */

    {E_PIXCLK_59400,   E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_60_1280x720p_24Hz */
    {E_PIXCLK_INVALID, E_PIXCLK_74250,   E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID},   /* HDMITX_VFMT_61_1280x720p_25Hz */
    {E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_74250,   E_PIXCLK_INVALID, E_PIXCLK_INVALID, E_PIXCLK_INVALID}    /* HDMITX_VFMT_62_1280x720p_30Hz */
};

/*============================================================================*/
/*                       CONSTANTS DECLARATIONS                               */
/*============================================================================*/


/**
 * Lookup table to convert PC formats used in API parameters to pixel clock 
 * frequencies.
 * The other index is the veritical frame frequency.
 */
#ifdef FORMAT_PC
UInt8 kVfmtToPixClk_PC[HDMITX_VFMT_PC_NUM] =
{
  /* HDMITX_VFREQ_60Hz HDMITX_VFREQ_70Hz HDMITX_VFREQ_72Hz HDMITX_VFREQ_75Hz HDMITX_VFREQ_85Hz HDMITX_VFREQ_87Hz*/
    E_PIXCLK_25175  ,   /* HDMITX_VFMT_PC_640x480p_60Hz   */
    E_PIXCLK_40000  ,   /* HDMITX_VFMT_PC_800x600p_60Hz   */
    E_PIXCLK_INVALID,   /* HDMITX_VFMT_PC_1152x960p_60Hz  */
    E_PIXCLK_65000  ,   /* HDMITX_VFMT_PC_1024x768p_60Hz */
    E_PIXCLK_79500  ,   /* HDMITX_VFMT_PC_1280x768p_60Hz */
    E_PIXCLK_108000 ,   /* HDMITX_VFMT_PC_1280x1024p_60Hz */
    E_PIXCLK_85500  ,   /* HDMITX_VFMT_PC_1360x768p_60Hz  */
    E_PIXCLK_121750 ,   /* HDMITX_VFMT_PC_1400x1050p_60Hz */
    E_PIXCLK_162000 ,   /* HDMITX_VFMT_PC_1600x1200p_60Hz */
    E_PIXCLK_75000  ,   /* HDMITX_VFMT_PC_1024x768p_70Hz  */
    E_PIXCLK_31500  ,   /* HDMITX_VFMT_PC_640x480p_72Hz   */
    E_PIXCLK_50000  ,   /* HDMITX_VFMT_PC_800x600p_72Hz   */
    E_PIXCLK_31500  ,   /* HDMITX_VFMT_PC_640x480p_75Hz   */
    E_PIXCLK_78750  ,   /* HDMITX_VFMT_PC_1024x768p_75Hz  */
    E_PIXCLK_49500  ,   /* HDMITX_VFMT_PC_800x600p_75Hz   */
    E_PIXCLK_INVALID,   /* HDMITX_VFMT_PC_1024x864p_75Hz  */
    E_PIXCLK_INVALID,   /* HDMITX_VFMT_PC_1280x1024p_75Hz */
    E_PIXCLK_INVALID,   /* HDMITX_VFMT_PC_640x350p_85Hz   */
    E_PIXCLK_INVALID,   /* HDMITX_VFMT_PC_640x400p_85Hz   */
    E_PIXCLK_INVALID,   /* HDMITX_VFMT_PC_720x400p_85Hz   */
    E_PIXCLK_36000  ,   /* HDMITX_VFMT_PC_640x480p_85Hz   */
    E_PIXCLK_56250  ,   /* HDMITX_VFMT_PC_800x600p_85Hz   */
    E_PIXCLK_INVALID,   /* HDMITX_VFMT_PC_1024x768p_85Hz  */
    E_PIXCLK_INVALID,   /* HDMITX_VFMT_PC_1152x864p_85Hz  */
    E_PIXCLK_INVALID,   /* HDMITX_VFMT_PC_1280x960p_85Hz  */
    E_PIXCLK_INVALID,    /* HDMITX_VFMT_PC_1280x1024p_85Hz */ /* PR1570 FIXED */
    E_PIXCLK_INVALID    /* HDMITX_VFMT_PC_1024x768i_87Hz  */
};
#endif

/**
 * Lookup table to convert from EIA/CEA TV video formats used in the EDID and in
 * API parameters to the format used in the E_REG_P00_VIDFORMAT_W register
 */

#ifdef TMFL_RGB_DDR_12BITS
static struct vic2reg vic2reg_TV[] = {
   {HDMITX_VFMT_01_640x480p_60Hz,  E_REGVFMT_640x480p_60Hz},
   {HDMITX_VFMT_02_720x480p_60Hz,  E_REGVFMT_720x480p_60Hz},
   {HDMITX_VFMT_03_720x480p_60Hz,  E_REGVFMT_720x480p_60Hz},
   {HDMITX_VFMT_04_1280x720p_60Hz, E_REGVFMT_1280x720p_60Hz},
   {HDMITX_VFMT_05_1920x1080i_60Hz,E_REGVFMT_1920x1080i_60Hz},
   {HDMITX_VFMT_06_720x480i_60Hz,  E_REGVFMT_720x480i_60Hz},
   {HDMITX_VFMT_07_720x480i_60Hz,  E_REGVFMT_720x480i_60Hz},
   {HDMITX_VFMT_08_720x240p_60Hz,  E_REGVFMT_720x240p_60Hz},
   {HDMITX_VFMT_09_720x240p_60Hz,  E_REGVFMT_720x240p_60Hz},
   {HDMITX_VFMT_10_720x480i_60Hz,  E_REGVFMT_2880x480i_60Hz_PR4},
   {HDMITX_VFMT_11_720x480i_60Hz,  E_REGVFMT_2880x480i_60Hz_PR4},
   {HDMITX_VFMT_14_1440x480p_60Hz, E_REGVFMT_1440x480p_60Hz},
   {HDMITX_VFMT_15_1440x480p_60Hz, E_REGVFMT_1440x480p_60Hz},
   {HDMITX_VFMT_16_1920x1080p_60Hz,E_REGVFMT_1920x1080p_60Hz},
   {HDMITX_VFMT_17_720x576p_50Hz,  E_REGVFMT_720x576p_50Hz},
   {HDMITX_VFMT_18_720x576p_50Hz,  E_REGVFMT_720x576p_50Hz},
   {HDMITX_VFMT_19_1280x720p_50Hz, E_REGVFMT_1280x720p_50Hz},
   {HDMITX_VFMT_20_1920x1080i_50Hz,E_REGVFMT_1920x1080i_50Hz},
   {HDMITX_VFMT_21_720x576i_50Hz,  E_REGVFMT_720x576i_50Hz},
   {HDMITX_VFMT_22_720x576i_50Hz,  E_REGVFMT_720x576i_50Hz},
   {HDMITX_VFMT_23_720x288p_50Hz,  E_REGVFMT_720x288p_50Hz},
   {HDMITX_VFMT_24_720x288p_50Hz,  E_REGVFMT_720x288p_50Hz},
   {HDMITX_VFMT_25_720x576i_50Hz,  E_REGVFMT_2880x576i_50Hz}, /* FIXME PR 2 */
   {HDMITX_VFMT_26_720x576i_50Hz,  E_REGVFMT_2880x576i_50Hz}, /* FIXME PR 2 */
   {HDMITX_VFMT_29_1440x576p_50Hz, E_REGVFMT_1440x576p_50Hz},
   {HDMITX_VFMT_30_1440x576p_50Hz, E_REGVFMT_1440x576p_50Hz},
   {HDMITX_VFMT_31_1920x1080p_50Hz,E_REGVFMT_1920x1080p_50Hz},
   {HDMITX_VFMT_32_1920x1080p_24Hz,E_REGVFMT_1920x1080p_24Hz},
   {HDMITX_VFMT_33_1920x1080p_25Hz,E_REGVFMT_1920x1080p_25Hz},
   {HDMITX_VFMT_34_1920x1080p_30Hz,E_REGVFMT_1920x1080p_30Hz},
   {HDMITX_VFMT_35_2880x480p_60Hz, E_REGVFMT_2880x480p_60Hz},
   {HDMITX_VFMT_36_2880x480p_60Hz, E_REGVFMT_2880x480p_60Hz},
   {HDMITX_VFMT_37_2880x576p_50Hz, E_REGVFMT_720x576p_50Hz},
   {HDMITX_VFMT_38_2880x576p_50Hz, E_REGVFMT_720x576p_50Hz},
   {HDMITX_VFMT_60_1280x720p_24Hz, E_REGVFMT_1280x720p_24Hz},
   {HDMITX_VFMT_61_1280x720p_25Hz, E_REGVFMT_1280x720p_25Hz},
   {HDMITX_VFMT_62_1280x720p_30Hz, E_REGVFMT_1280x720p_30Hz}
};
static struct vic2reg vic2reg_TV_FP[] = {
   {HDMITX_VFMT_01_640x480p_60Hz,  E_REGVFMT_720x480p_60Hz_FP},
   {HDMITX_VFMT_02_720x480p_60Hz,  E_REGVFMT_720x480p_60Hz_FP},
   {HDMITX_VFMT_03_720x480p_60Hz,  E_REGVFMT_720x480p_60Hz_FP},
   {HDMITX_VFMT_04_1280x720p_60Hz, E_REGVFMT_1280x720p_60Hz_FP},
   {HDMITX_VFMT_05_1920x1080i_60Hz,E_REGVFMT_1920x1080i_60Hz_FP},
   {HDMITX_VFMT_17_720x576p_50Hz,  E_REGVFMT_720x576p_50Hz_FP},
   {HDMITX_VFMT_18_720x576p_50Hz,  E_REGVFMT_720x576p_50Hz_FP},
   {HDMITX_VFMT_19_1280x720p_50Hz, E_REGVFMT_1280x720p_50Hz_FP},
   {HDMITX_VFMT_20_1920x1080i_50Hz,E_REGVFMT_1920x1080i_50Hz_FP},
   {HDMITX_VFMT_32_1920x1080p_24Hz,E_REGVFMT_1920x1080p_24Hz_FP},
   {HDMITX_VFMT_33_1920x1080p_25Hz,E_REGVFMT_1920x1080p_25Hz_FP},
   {HDMITX_VFMT_34_1920x1080p_30Hz,E_REGVFMT_1920x1080p_30Hz_FP},
   {HDMITX_VFMT_60_1280x720p_24Hz, E_REGVFMT_1280x720p_24Hz_FP},
   {HDMITX_VFMT_61_1280x720p_25Hz, E_REGVFMT_1280x720p_25Hz_FP},
   {HDMITX_VFMT_62_1280x720p_30Hz, E_REGVFMT_1280x720p_30Hz_FP}
};
#else
static struct vic2reg vic2reg_TV[] = {
   {HDMITX_VFMT_01_640x480p_60Hz,  E_REGVFMT_640x480p_60Hz},
   {HDMITX_VFMT_02_720x480p_60Hz,  E_REGVFMT_720x480p_60Hz},
   {HDMITX_VFMT_03_720x480p_60Hz,  E_REGVFMT_720x480p_60Hz},
   {HDMITX_VFMT_04_1280x720p_60Hz, E_REGVFMT_1280x720p_60Hz},
   {HDMITX_VFMT_05_1920x1080i_60Hz,E_REGVFMT_1920x1080i_60Hz},
   {HDMITX_VFMT_06_720x480i_60Hz,  E_REGVFMT_720x480i_60Hz},
   {HDMITX_VFMT_07_720x480i_60Hz,  E_REGVFMT_720x480i_60Hz},
   {HDMITX_VFMT_08_720x240p_60Hz,  E_REGVFMT_720x240p_60Hz},
   {HDMITX_VFMT_09_720x240p_60Hz,  E_REGVFMT_720x240p_60Hz},
   {HDMITX_VFMT_16_1920x1080p_60Hz,E_REGVFMT_1920x1080p_60Hz},
   {HDMITX_VFMT_17_720x576p_50Hz,  E_REGVFMT_720x576p_50Hz},
   {HDMITX_VFMT_18_720x576p_50Hz,  E_REGVFMT_720x576p_50Hz},
   {HDMITX_VFMT_19_1280x720p_50Hz, E_REGVFMT_1280x720p_50Hz},
   {HDMITX_VFMT_20_1920x1080i_50Hz,E_REGVFMT_1920x1080i_50Hz},
   {HDMITX_VFMT_21_720x576i_50Hz,  E_REGVFMT_720x576i_50Hz},
   {HDMITX_VFMT_22_720x576i_50Hz,  E_REGVFMT_720x576i_50Hz},
   {HDMITX_VFMT_23_720x288p_50Hz,  E_REGVFMT_720x288p_50Hz},
   {HDMITX_VFMT_24_720x288p_50Hz,  E_REGVFMT_720x288p_50Hz},
   {HDMITX_VFMT_31_1920x1080p_50Hz,E_REGVFMT_1920x1080p_50Hz},
   {HDMITX_VFMT_32_1920x1080p_24Hz,E_REGVFMT_1920x1080p_24Hz},
   {HDMITX_VFMT_33_1920x1080p_25Hz,E_REGVFMT_1920x1080p_25Hz},
   {HDMITX_VFMT_34_1920x1080p_30Hz,E_REGVFMT_1920x1080p_30Hz},
   {HDMITX_VFMT_35_2880x480p_60Hz, E_REGVFMT_720x480p_60Hz},
   {HDMITX_VFMT_36_2880x480p_60Hz, E_REGVFMT_720x480p_60Hz},
   {HDMITX_VFMT_37_2880x576p_50Hz, E_REGVFMT_720x576p_50Hz},
   {HDMITX_VFMT_38_2880x576p_50Hz, E_REGVFMT_720x576p_50Hz},
   {HDMITX_VFMT_60_1280x720p_24Hz, E_REGVFMT_1280x720p_24Hz},
   {HDMITX_VFMT_61_1280x720p_25Hz, E_REGVFMT_1280x720p_25Hz},
   {HDMITX_VFMT_62_1280x720p_30Hz, E_REGVFMT_1280x720p_30Hz}
};
static struct vic2reg vic2reg_TV_FP[] = {
   {HDMITX_VFMT_04_1280x720p_60Hz, E_REGVFMT_1280x720p_60Hz_FP},
   {HDMITX_VFMT_05_1920x1080i_60Hz,E_REGVFMT_1920x1080i_60Hz_FP},
   {HDMITX_VFMT_19_1280x720p_50Hz, E_REGVFMT_1280x720p_50Hz_FP},
   {HDMITX_VFMT_20_1920x1080i_50Hz,E_REGVFMT_1920x1080i_50Hz_FP},
   {HDMITX_VFMT_32_1920x1080p_24Hz,E_REGVFMT_1920x1080p_24Hz_FP},
   {HDMITX_VFMT_33_1920x1080p_25Hz,E_REGVFMT_1920x1080p_25Hz_FP},
   {HDMITX_VFMT_34_1920x1080p_30Hz,E_REGVFMT_1920x1080p_30Hz_FP},
   {HDMITX_VFMT_60_1280x720p_24Hz, E_REGVFMT_1280x720p_24Hz_FP},
   {HDMITX_VFMT_61_1280x720p_25Hz, E_REGVFMT_1280x720p_25Hz_FP},
   {HDMITX_VFMT_62_1280x720p_30Hz, E_REGVFMT_1280x720p_30Hz_FP}
};
#endif

#ifdef FORMAT_PC
static struct vic2reg vic2reg_PC[HDMITX_VFMT_PC_NUM] = {
   {HDMITX_VFMT_PC_640x480p_60Hz,  E_REGVFMT_640x480p_60Hz},
   {HDMITX_VFMT_PC_800x600p_60Hz,  E_REGVFMT_800x600p_60Hz},
   {HDMITX_VFMT_PC_1024x768p_60Hz, E_REGVFMT_1024x768p_60Hz},
   {HDMITX_VFMT_PC_1280x768p_60Hz, E_REGVFMT_1280x768p_60Hz},
   {HDMITX_VFMT_PC_1280x1024p_60Hz,E_REGVFMT_1280x1024p_60Hz},
   {HDMITX_VFMT_PC_1360x768p_60Hz, E_REGVFMT_1360x768p_60Hz},
   {HDMITX_VFMT_PC_1400x1050p_60Hz,E_REGVFMT_1400x1050p_60Hz},
   {HDMITX_VFMT_PC_1600x1200p_60Hz,E_REGVFMT_1600x1200p_60Hz},
   {HDMITX_VFMT_PC_1024x768p_70Hz, E_REGVFMT_1024x768p_70Hz},
   {HDMITX_VFMT_PC_640x480p_72Hz,  E_REGVFMT_640x480p_72Hz},
   {HDMITX_VFMT_PC_800x600p_72Hz,  E_REGVFMT_800x600p_72Hz},
   {HDMITX_VFMT_PC_640x480p_75Hz,  E_REGVFMT_640x480p_75Hz},
   {HDMITX_VFMT_PC_1024x768p_75Hz, E_REGVFMT_1024x768p_75Hz},
   {HDMITX_VFMT_PC_800x600p_75Hz,  E_REGVFMT_800x600p_75Hz},
   {HDMITX_VFMT_PC_640x480p_85Hz,  E_REGVFMT_640x480p_85Hz},
   {HDMITX_VFMT_PC_800x600p_85Hz,  E_REGVFMT_800x600p_85Hz},
   {HDMITX_VFMT_PC_1280x1024p_85Hz,E_REGVFMT_1280x1024p_85Hz}
};
#endif /* FORMAT PC */ 
    

/**
 * Lookup table to convert from video format codes used in the 
 * E_REG_P00_VIDFORMAT_W register to corresponding VS_PIX_STRT_2
 * register values, to correct the output window for interlaced
 * output formats, with or without the scaler.
 *
 * The correction is VS_PIX_STRT_2=VS_PIX_STRT_2+VS_PIX_STRT_1.
 * The same value is also applied to VS_PIX_END_2.
 */

/**
 * Lookup table to convert from video format codes used in the 
 * E_REG_P00_VIDFORMAT_W register to corresponding 
 * pixel repetition values in the PLL_SERIAL_2 register.
 * 0=no repetition (pixel sent once)
 * 1=one repetition (pixel sent twice) etc
 */

/**
 * Lookup table to convert from video format codes used in the 
 * E_REG_P00_VIDFORMAT_W register to corresponding 
 * trios of 2-bit values in the srl_nosc, scg_nosc and de_nosc
 * PLL control registers
 * 
 * Rational for dummies by André ;)
 * ----------------------------- 
 *    the TMDS serializer multiply x10 the pixclk (this is a PLL;)
 *    <format>   -->   <pixclk>   -->   <PLL>   -->   <div by>
 *    576i or 480i     13.5 Mhz (*2)    270 Mhz       4
 *    576p             27 Mhz           270 Mhz       4
 *    720p or 1080i    74.25 Mhz        742 Mhz       2
 *    1080p            148.5 Mhz        1485 Mhz      1
 * 
 */

static UInt8 pll[] = {
   /* prefetch */
   2,  /* E_REGVFMT_640x480p_60Hz   */
   2,  /* E_REGVFMT_720x480p_60Hz   */
   1,  /* E_REGVFMT_1280x720p_60Hz  */
   1,  /* E_REGVFMT_1920x1080i_60Hz */
   3,  /* E_REGVFMT_720x480i_60Hz   */
   0,  /* E_REGVFMT_720x240p_60Hz   */ /** \todo Need nosc PLL value */
   0,  /* E_REGVFMT_1920x1080p_60Hz */
   2,  /* E_REGVFMT_720x576p_50Hz   */
   1,  /* E_REGVFMT_1280x720p_50Hz  */
   1,  /* E_REGVFMT_1920x1080i_50Hz */
   3,  /* E_REGVFMT_720x576i_50Hz   */
   0,  /* E_REGVFMT_720x288p_50Hz   */ /** \todo Need nosc PLL value */
   0,  /* E_REGVFMT_1920x1080p_50Hz */
#ifdef TMFL_RGB_DDR_12BITS
   0,  /* E_REGVFMT_1920x1080p_24Hz   */
   1,  /* E_REGVFMT_1440x576p_50Hz    */
   1,  /* E_REGVFMT_1440x480p_50Hz    */
   0,  /* E_REGVFMT_2880x480p_50Hz    */
   0,  /* E_REGVFMT_2880x576p_50Hz    */
   1,  /* E_REGVFMT_2880x480i_60Hz    */
   2,  /* E_REGVFMT_2880x480i_60Hz_PR2*/
   2,  /* E_REGVFMT_2880x480i_60Hz_PR4*/
   1,  /* E_REGVFMT_2880x576i_50Hz    */
   2,  /* E_REGVFMT_2880x576i_50Hz_PR2*/
   1,  /* E_REGVFMT_720x480p_60Hz_FP  */
   0,  /* E_REGVFMT_1280x720p_60Hz_FP */
   1,  /* E_REGVFMT_720x576p_50Hz_FP  */
   0,  /* E_REGVFMT_1280x720p_50Hz_FP */
   0,  /* E_REGVFMT_1920x1080p_23Hz_FP*/
   0,  /* E_REGVFMT_1920x1080p_25Hz_FP*/
   0,  /* E_REGVFMT_1920x1080p_29Hz_FP*/
   0,  /* E_REGVFMT_1920x1080i_60Hz_FP*/
   0,  /* E_REGVFMT_1920x1080i_50Hz_FP*/
#endif
        /* extra list */
#ifndef TMFL_RGB_DDR_12BITS
   1,  /* E_REGVFMT_1920x1080p_24Hz */
#endif
   1,  /* E_REGVFMT_1920x1080p_25Hz */
   1,  /* E_REGVFMT_1920x1080p_30Hz */
   1,  /* E_REGVFMT_1280x720p_24Hz  */
   1,  /* E_REGVFMT_1280x720p_25Hz  */
   1,  /* E_REGVFMT_1280x720p_30Hz  */
#ifndef TMFL_RGB_DDR_12BITS
   0,  /* E_REGVFMT_1280x720p_60Hz_FP */
   0,  /* E_REGVFMT_1920x1080i_60Hz_FP */
   0,  /* E_REGVFMT_1280x720p_50Hz_FP */
   0,  /* E_REGVFMT_1920x1080i_50Hz_FP */
   0,  /* E_REGVFMT_1920x1080p_24Hz_FP */
   0,  /* E_REGVFMT_1920x1080p_25Hz_FP */
   0,  /* E_REGVFMT_1920x1080p_30Hz_FP */
#endif
   0,  /* E_REGVFMT_1280x720p_24Hz_FP */
   0,  /* E_REGVFMT_1280x720p_25Hz_FP */
   0,  /* E_REGVFMT_1280x720p_30Hz_FP */
#ifdef FORMAT_PC
   2,   /* E_REGVFMT_640x480p_72Hz   */
   2,   /* E_REGVFMT_640x480p_75Hz   */
   2,   /* E_REGVFMT_640x480p_85Hz   */
   1,   /* E_REGVFMT_800x600p_60Hz   */
   1,   /* E_REGVFMT_800x600p_72Hz   */
   1,   /* E_REGVFMT_800x600p_75Hz   */
   1,   /* E_REGVFMT_800x600p_85Hz   */
   1,   /* E_REGVFMT_1024x768p_60Hz  */
   1,   /* E_REGVFMT_1024x768p_70Hz  */
   1,   /* E_REGVFMT_1024x768p_75Hz  */
   0,   /* E_REGVFMT_1280x768p_60Hz  */
   0,   /* E_REGVFMT_1280x1024p_60Hz */
   0,   /* E_REGVFMT_1360x768p_60Hz  */
   0,   /* E_REGVFMT_1400x1050p_60Hz */
   0,   /* E_REGVFMT_1600x1200p_60Hz */
   1    /* E_REGVFMT_1280x1024p_85Hz */
#endif /* FORMAT_PC */
};



/**
 * Lokup table to convert from video format codes used in the 
 * E_REG_P00_VIDFORMAT_W register to RefPix and RefLine values
 * according to sync source
 */
/* prefetch list */
static struct sync_desc ref_sync[] =
{
   /*
     designer world <==> CEA-861 reader world
     ----------------------------------------
     t_hs_s : hfp+1
     t_vsl_s1 : vfp+1
     t_de_s : href+1
     t_vw_s1 : vref+1

     For the story, designer have defined VsPixRef and VsLineRef concept
     that are the position of VSync in pixel and line starting from the top
     of the frame.
     So we have in fact : VSync that is hfp + vfp*total_h_active away from top

   */
   /* Vs2    PR  Vtg Htg HFP VFP HREF VREF */
   {0,       0,  1,  1,  17,   2, 161, 36}, /* E_REGVFMT_640x480p_60Hz   */
   {0,       0,  1,  1,  17,   8, 139, 43}, /* E_REGVFMT_720x480p_60Hz   */
   {0,       0,  0,  0,  111,  2, 371, 26}, /* E_REGVFMT_1280x720p_60Hz  */
   {1100+88, 0,  0,  0,  89,   2, 281, 21}, /* E_REGVFMT_1920x1080i_60Hz */
   {429+19,  1,  1,  1,  20,   5, 139, 22}, /* E_REGVFMT_720x480i_60Hz   */
   {0,       1,  1,  1,  20,   5, 139, 22}, /* E_REGVFMT_720x240p_60Hz   */
   {0,       0,  0,  0,  89,   2, 281, 42}, /* E_REGVFMT_1920x1080p_60Hz */
   {0,       0,  1,  1,  13,   2, 145, 45}, /* E_REGVFMT_720x576p_50Hz   */
   {0,       0,  0,  0,  441,  2, 701, 26}, /* E_REGVFMT_1280x720p_50Hz  */
   {1320+528,0,  0,  0,  529,  2, 721, 21}, /* E_REGVFMT_1920x1080i_50Hz */
   {432+12,  1,  1,  1,  13,   2, 145, 23}, /* E_REGVFMT_720x576i_50Hz   */
   {0,       1,  1,  1,  13,   2, 145, 23}, /* E_REGVFMT_720x288p_50Hz   */
   {0,       0,  0,  0,  529,  2, 721, 42}, /* E_REGVFMT_1920x1080p_50Hz */
#ifdef TMFL_RGB_DDR_12BITS
   {0,       0,  0,  0,  639,  2, 831, 42}, /* E_REGVFMT_1920x1080p_24Hz  */
   {0,       0,  1,  1,  25,   2, 289, 45}, /* E_REGVFMT_1440x576p_50Hz   */
   {0,       0,  1,  1,  33,   8, 277, 43}, /* E_REGVFMT_1440x480p_50Hz   */
   {0,       0,  1,  1,  65,   8, 553, 43}, /* E_REGVFMT_2880x480p_50Hz   */
   {0,       0,  1,  1,  49,   2, 577, 45}, /* E_REGVFMT_2880x576p_50Hz   */
   {1716+76, 0,  1,  1,  77,   5, 553, 22}, /* E_REGVFMT_2880x480i_60Hz   */
   {858+38,  1,  1,  1,  39,   5, 277, 22}, /* E_REGVFMT_2880x480i_60Hz_PR2*/
   {429+19,  2,  1,  1,  20,   5, 139, 22}, /* E_REGVFMT_2880x480i_60Hz_PR4*/
   {1728+48, 0,  1,  1,  49,   2, 577, 23}, /* E_REGVFMT_2880x576i_50Hz   */
   {864+24,  1,  1,  1,  25,   2, 289, 23}  /* E_REGVFMT_2880x576i_50Hz_PR*/
#endif
};

/* extra list */
static struct sync_desc ref_sync_extra[] =
{
   /* Vs2    PR  Vtg Htg HFP VFP HREF VREF */
#ifndef TMFL_RGB_DDR_12BITS
   {0,       0,  0,  0,  639,  2, 831, 42}, /* E_REGVFMT_1920x1080p_24Hz */
#endif
   {0,       0,  0,  0,  529,  2, 721, 42}, /* E_REGVFMT_1920x1080p_25Hz */
   {0,       0,  0,  0,  89,   2, 281, 42}, /* E_REGVFMT_1920x1080p_30Hz */    
   {0,       0,  0,  0,  1761, 2, 2021,26}, /* E_REGVFMT_1280x720p_24Hz  */
   {0,       0,  0,  0,  2421, 2, 2681,26}, /* E_REGVFMT_1280x720p_25Hz  */
   {0,       0,  0,  0,  1761, 2, 2021,26}  /* E_REGVFMT_1280x720p_30Hz  */
};

#ifdef FORMAT_PC    
   /* PC list */
static struct sync_desc ref_sync_PC[] =
{
   /* Vs2    PR  Vtg Htg HFP VFP HREF VREF */
   {0,       0,  1,  1,  25,   2, 195, 32}, /* E_REGVFMT_640x480p_72Hz   */
   {0,       0,  1,  1,  17,   2, 203, 20}, /* E_REGVFMT_640x480p_75Hz   */
   {0,       0,  1,  1,  57,   2, 195, 29}, /* E_REGVFMT_640x480p_85Hz   */
   {0,       0,  0,  0,  41,   2, 259, 28}, /* E_REGVFMT_800x600p_60Hz   */
   {0,       0,  0,  0,  57,   2, 243, 30}, /* E_REGVFMT_800x600p_72Hz   */
   {0,       0,  0,  0,  17,   2, 259, 25}, /* E_REGVFMT_800x600p_75Hz   */
   {0,       0,  0,  0,  33,   2, 251, 31}, /* E_REGVFMT_800x600p_85Hz   */
   {0,       0,  1,  1,  25,   2, 323, 36}, /* E_REGVFMT_1024x768p_60Hz  */
   {0,       0,  1,  1,  25,   2, 307, 36}, /* E_REGVFMT_1024x768p_70Hz  */
   {0,       0,  0,  0,  17,   2, 291, 32}, /* E_REGVFMT_1024x768p_75Hz  */
   {0,       0,  0,  1,  65,   2, 387, 28}, /* E_REGVFMT_1280x768p_60Hz  */
   {0,       0,  0,  0,  49,   2, 411, 42}, /* E_REGVFMT_1280x1024p_60Hz */
   {0,       0,  0,  0,  65,   2, 435, 25}, /* E_REGVFMT_1360x768p_60Hz  */
   {0,       0,  0,  1,  89,   2, 467, 37}, /* E_REGVFMT_1400x1050p_60Hz */
   {0,       0,  0,  0,  65,   2, 563, 50}, /* E_REGVFMT_1600x1200p_60Hz */
   {0,       0,  0,  0,  65,   2, 451, 48}  /* E_REGVFMT_1280x1024p_85Hz */
};
#endif/* FORMAT_PC */

static tmHdmiTxVidReg_t format_param_extra[] = {
   /*  NPIX    NLINE  VsLineStart  VsPixStart  VsLineEnd   VsPixEnd    HsStart     HsEnd   ActiveVideoStart   ActiveVideoEnd DeStart DeEnd */
   /*  npix    nline  vsl_s1       vsp_s1      vsl_e1      vsp_e1      hs_e        hs_e    vw_s1              vw_e1          de_s    de_e */
#ifndef TMFL_RGB_DDR_12BITS
   {2750,  1125,     1,          638,         6,           638,        638,    682,    41,         1121,   830,   2750, 0, 0},/* E_REGVFMT_1920x1080p_24Hz */
#endif
   {2640,  1125,     1,          528,         6,           528,        528,    572,    41,         1121,   720,   2640, 0, 0},/* E_REGVFMT_1920x1080p_25Hz */
   {2200,  1125,     1,          88,          6,           88,         88,     132,    41,         1121,   280,   2200, 0, 0},/* E_REGVFMT_1920x1080p_30Hz */
   {3300,   750,     1,         1760,         6,          1760,       1760,   1800,    25,          745,  2020,   3300, 0, 0},/* E_REGVFMT_1280x720p_24Hz  */
   {3960,   750,     1,         2420,         6,          2420,       2420,   2460,    25,          745,  2680,   3960, 0, 0},/* E_REGVFMT_1280x720p_25Hz  */
   {3300,   750,     1,         1760,         6,          1760,       1760,   1800,    25,          745,  2020,   3300, 0, 0},/* E_REGVFMT_1280x720p_30Hz  */
#ifndef TMFL_RGB_DDR_12BITS
   {1650,   1500,    1,          110,         6,           110,        110,    150,    25,         1495,   370,   1650, 746, 776},/* E_REGVFMT_1280x720p_60Hz_FP  */
   {2200,   2250,    1,          88,          6,           88,         88,     132,    20,         2248,   280,   2200, 0, 0},/* E_REGVFMT_1920x1080i_60Hz_FP */
   {1980,   1500,    1,          440,         6,           440,        440,    480,    25,         1495,   700,   1980, 746, 776},/* E_REGVFMT_1280x720p_50Hz_FP  */
   {2640,   2250,    1,          528,         6,           528,        528,    572,    20,         2248,   720,   2640, 0, 0},/* E_REGVFMT_1920x1080i_50Hz_FP */
   {2750,   2250,    1,          638,         6,           638,        638,    682,    41,         2246,   830,   2750, 1122, 1167},/* E_REGVFMT_1920x1080p_24Hz_FP */
   {2640,   2250,    1,          528,         6,           528,        528,    572,    41,         2246,   720,   2640, 1122, 1167},/* E_REGVFMT_1920x1080p_25Hz_FP */
   {2200,   2250,    1,          88,          6,           88,         88,     132,    41,         2246,   280,   2200, 1122, 1167},/* E_REGVFMT_1920x1080p_30Hz_FP */
#endif
   {3300,   1500,    1,         1760,         6,          1760,       1760,   1800,    25,         1495,  2020,   3300, 0, 0},/* E_REGVFMT_1280x720p_24Hz_FP  */
   {3960,   1500,    1,         2420,         6,          2420,       2420,   2460,    25,         1495,  2680,   3960, 0, 0},/* E_REGVFMT_1280x720p_25Hz_FP  */
   {3300,   1500,    1,         1760,         6,          1760,       1760,   1800,    25,         1495,  2020,   3300, 0, 0},/* E_REGVFMT_1280x720p_30Hz_FP  */
};

#ifdef FORMAT_PC
static tmHdmiTxVidReg_t format_param_PC[HDMITX_VFMT_PC_NUM] =
{
   /*  NPIX    NLINE  VsLineStart  VsPixStart  VsLineEnd   VsPixEnd    HsStart     HsEnd   ActiveVideoStart   ActiveVideoEnd DeStart DeEnd */
   /*  npix    nline  vsl_s1       vsp_s1      vsl_e1      vsp_e1      hs_e        hs_e    vw_s1              vw_e1          de_s    de_e */
    {832,   520,      1,          24,          4,           24,         24,     64,     31,         511,    192,    832, 0, 0},/* E_REGVFMT_640x480p_72Hz   */
    {840,   500,      1,          16,          4,           16,         16,     80,     19,         499,    200,    840, 0, 0},/* E_REGVFMT_640x480p_75Hz   */
    {832,   509,      1,          56,          4,           56,         56,     112,    28,         508,    192,    832, 0, 0},/* E_REGVFMT_640x480p_85Hz   */
    {1056,  628,      1,          40,          5,           40,         40,     168,    27,         627,    256,   1056, 0, 0},/* E_REGVFMT_800x600p_60Hz   */
    {1040,  666,      1,          56,          7,           56,         56,     176,    29,         619,    240,   1040, 0, 0},/* E_REGVFMT_800x600p_72Hz   */
    {1056,  625,      1,          16,          4,           16,         16,     96,     24,         624,    256,   1056, 0, 0},/* E_REGVFMT_800x600p_75Hz   */
    {1048,  631,      1,          32,          4,           32,         32,     96,     30,         630,    248,   1048, 0, 0},/* E_REGVFMT_800x600p_85Hz   */
    {1344,  806,      1,          24,          7,           24,         24,     160,    35,         803,    320,   1344, 0, 0},/* E_REGVFMT_1024x768p_60Hz  */
    {1328,  806,      1,          24,          7,           24,         24,     160,    35,         803,    304,   1328, 0, 0},/* E_REGVFMT_1024x768p_70Hz  */
    {1312,  800,      1,          16,          4,           16,         16,     112,    31,         799,    288,   1312, 0, 0},/* E_REGVFMT_1024x768p_75Hz  */
    {1664,  798,      1,          64,          8,           64,         64,     192,    27,         795,    384,   1664, 0, 0},/* E_REGVFMT_1280x768p_60Hz  */
    {1688,  1066,     1,          48,          4,           48,         48,     160,    41,         1065,   408,   1688, 0, 0},/* E_REGVFMT_1280x1024p_60Hz */
    {1792,  795,      1,          64,          7,           64,         64,     176,    24,         792,    432,   1792, 0, 0},/* E_REGVFMT_1360x768p_60Hz  */
    {1864,  1089,     1,          88,          5,           88,         88,     232,    36,         1086,   464,   1864, 0, 0},/* E_REGVFMT_1400x1050p_60Hz */
    {2160,  1250,     1,          64,          4,           64,         64,     256,    49,         1249,   560,   2160, 0, 0},/* E_REGVFMT_1600x1200p_60Hz */
    {1728,  1072,     1,          64,          4,           64,         64,     224,    47,         1071,   448,   1728, 0, 0} /* E_REGVFMT_1280x1024p_85Hz */
};
#endif/* FORMAT_PC */

 /**
 *  Lookup table for each pixel clock frequency's CTS value in kHz
 *  according to SCS table "Audio Clock Recovery CTS Values"
 */
static UInt32 kPixClkToAcrCts[E_PIXCLK_NUM][HDMITX_AFS_NUM] =
{
 /* HDMITX_AFS_32k  _AFS_48K       _AFS_96K        _AFS_192K */
 /*         _AFS_44_1k      _AFS_88_2K      _AFS_176_4K       */
   { 28125,  31250,  28125,  31250,  28125,  31250,  28125}, /* E_PIXCLK_25175 */
   { 25200,  28000,  25200,  28000,  25200,  28000,  25200}, /* E_PIXCLK_25200 */
   { 27000,  30000,  27000,  30000,  27000,  30000,  27000}, /* E_PIXCLK_27000 */
   { 27027,  30030,  27027,  30030,  27027,  30030,  27027}, /* E_PIXCLK_27027 */
   { 54000,  60000,  54000,  60000,  54000,  60000,  54000}, /* E_PIXCLK_54000 */
   { 54054,  60060,  54054,  60060,  54054,  60060,  54054}, /* E_PIXCLK_54054 */
   { 59400,  65996,  59400,  65996,  59400,  65996,  59400}, /* E_PIXCLK_59400 */
   {210937, 234375, 140625, 234375, 140625, 234375, 140625}, /* E_PIXCLK_74175 */
   { 74250,  82500,  74250,  82500,  74250,  82500,  74250}, /* E_PIXCLK_74250 */
   {421875, 234375, 140625, 234375, 140625, 234375, 140625}, /* E_PIXCLK_148350*/
   {148500, 165000, 148500, 165000, 148500, 165000, 148500}  /* E_PIXCLK_148500*/
#ifdef FORMAT_PC
  ,{ 31500,  35000,  31500,  35000,  31500,  35000,  31500}, /* E_PIXCLK_31500 */
   { 36000,  40000,  36000,  40000,  36000,  40000,  36000}, /* E_PIXCLK_36000 */
   { 40000,  44444,  40000,  44444,  40000,  44444,  40000}, /* E_PIXCLK_40000 */
   { 49500,  55000,  49500,  55000,  49500,  55000,  49500}, /* E_PIXCLK_49500 */
   { 50000,  55556,  50000,  55556,  50000,  55556,  50000}, /* E_PIXCLK_50000 */
   { 56250,  62500,  56250,  62500,  56250,  62500,  56250}, /* E_PIXCLK_56250 */
   { 65000,  72222,  65000,  72222,  65000,  72222,  65000}, /* E_PIXCLK_65000 */
   { 75000,  83333,  75000,  83333,  75000,  83333,  75000}, /* E_PIXCLK_75000 */
   { 78750,  87500,  78750,  87500,  78750,  87500,  78750}, /* E_PIXCLK_78750 */
   {162000, 180000, 162000, 180000, 162000, 180000, 162000}, /* E_PIXCLK_162000*/
   {157500, 175000, 157500, 175000, 157500, 175000, 157500}  /* E_PIXCLK_157500 */
#endif /* FORMAT_PC */
};

/**
 *  Lookup table for each pixel clock frequency's Audio Clock Regeneration N,
 *  according to SCS Table "Audio Clock Recovery N Values"
 */
static UInt32 kPixClkToAcrN[E_PIXCLK_NUM][HDMITX_AFS_NUM] =
{
 /* HDMITX_AFS_32k  _AFS_48K       _AFS_96K        _AFS_192K */
 /*         _AFS_44_1k      _AFS_88_2K      _AFS_176_4K       */
   { 4576,   7007,   6864,  14014,  13728,  28028,  27456}, /* E_PIXCLK_25175 */
   { 4096,   6272,   6144,  12544,  12288,  25088,  24576}, /* E_PIXCLK_25200 */
   { 4096,   6272,   6144,  12544,  12288,  25088,  24576}, /* E_PIXCLK_27000 */
   { 4096,   6272,   6144,  12544,  12288,  25088,  24576}, /* E_PIXCLK_27027 */
   { 4096,   6272,   6144,  12544,  12288,  25088,  24576}, /* E_PIXCLK_54000 */
   { 4096,   6272,   6144,  12544,  12288,  25088,  24576}, /* E_PIXCLK_54054 */
   { 4096,   6272,   6144,  12544,  12288,  25088,  24576}, /* E_PIXCLK_59400 */
   {11648,  17836,  11648,  35672,  23296,  71344,  46592}, /* E_PIXCLK_74175 */
   { 4096,   6272,   6144,  12544,  12288,  25088,  24576}, /* E_PIXCLK_74250 */
   {11648,   8918,   5824,  17836,  11648,  35672,  23296}, /* E_PIXCLK_148350*/
   { 4096,   6272,   6144,  12544,  12288,  25088,  24576}  /* E_PIXCLK_148500*/
#ifdef FORMAT_PC
  ,{ 4096,  6272,  6144,  12544,  12288,  25088,  24576}, /* E_PIXCLK_31500 */
   { 4096,  6272,  6144,  12544,  12288,  25088,  24576}, /* E_PIXCLK_36000 */
   { 4096,  6272,  6144,  12544,  12288,  25088,  24576}, /* E_PIXCLK_40000 */
   { 4096,  6272,  6144,  12544,  12288,  25088,  24576}, /* E_PIXCLK_49500 */
   { 4096,  6272,  6144,  12544,  12288,  25088,  24576}, /* E_PIXCLK_50000 */
   { 4096,  6272,  6144,  12544,  12288,  25088,  24576}, /* E_PIXCLK_56250 */
   { 4096,  6272,  6144,  12544,  12288,  25088,  24576}, /* E_PIXCLK_65000 */
   { 4096,  6272,  6144,  12544,  12288,  25088,  24576}, /* E_PIXCLK_75000 */
   { 4096,  6272,  6144,  12544,  12288,  25088,  24576}, /* E_PIXCLK_78750 */
   { 4096,  6272,  6144,  12544,  12288,  25088,  24576},  /* E_PIXCLK_162000*/
   { 4096,  6272,  6144,  12544,  12288,  25088,  24576}  /* E_PIXCLK_157500*/
#endif /* FORMAT_PC */
};

/**
 *  Lookup table for each pixel clock frequency's Audio Divider, according to
 *  SCS Table "Audio Clock Recovery Divider Values"
 */
static UInt8 kPixClkToAdiv[E_PIXCLK_NUM][HDMITX_AFS_NUM] =
{
 /* HDMITX_AFS_32k  _AFS_48K       _AFS_96K        _AFS_192K */
 /*         _AFS_44_1k      _AFS_88_2K      _AFS_176_4K       */
   {2,      2,      2,      1,      1,      0,      0},     /* E_PIXCLK_25175 */
   {2,      2,      2,      1,      1,      0,      0},     /* E_PIXCLK_25200 */
   {2,      2,      2,      1,      1,      0,      0},     /* E_PIXCLK_27000 */
   {2,      2,      2,      1,      1,      0,      0},     /* E_PIXCLK_27027 */
   {3,      3,      3,      2,      2,      1,      1},     /* E_PIXCLK_54000 */
   {3,      3,      3,      2,      2,      1,      1},     /* E_PIXCLK_54054 */
   {3,      3,      3,      2,      2,      1,      1},     /* E_PIXCLK_59400 */
   {4,      3,      3,      2,      2,      1,      1},     /* E_PIXCLK_74175 */
   {4,      3,      3,      2,      2,      1,      1},     /* E_PIXCLK_74250 */
   {5,      4,      4,      3,      3,      2,      2},     /* E_PIXCLK_148350 */
   {5,      4,      4,      3,      3,      2,      2}      /* E_PIXCLK_148500 */
#ifdef FORMAT_PC
  ,{2,      2,      2,      1,      1,      0,      0}, /* E_PIXCLK_31500  */
   {3,      2,      2,      1,      1,      0,      0}, /* E_PIXCLK_36000  */
   {3,      2,      2,      1,      1,      0,      0}, /* E_PIXCLK_40000  */
   {3,      3,      3,      2,      2,      1,      1}, /* E_PIXCLK_49500  */
   {3,      3,      3,      2,      2,      1,      1}, /* E_PIXCLK_50000  */
   {3,      3,      3,      2,      2,      1,      1}, /* E_PIXCLK_56250  */
   {4,      3,      3,      2,      2,      1,      1}, /* E_PIXCLK_65000  */
   {4,      3,      3,      2,      2,      1,      1}, /* E_PIXCLK_75000  */
   {4,      3,      3,      2,      2,      1,      1}, /* E_PIXCLK_78750  */
   {5,      4,      4,      3,      3,      2,      2}, /* E_PIXCLK_162000 */
   {5,      4,      4,      3,      3,      2,      2}  /* E_PIXCLK_157500 */
#endif /* FORMAT_PC */

};

/**
 *  Lookup table for converting a sampling frequency into the values
 *  required in channel status byte 3 according to IEC60958-3
 */
static UInt8 kAfsToCSbyte3[HDMITX_AFS_NUM+1] =
{
    3,      /* HDMITX_AFS_32k */
    0,      /* HDMITX_AFS_44_1k */
    2,      /* HDMITX_AFS_48k */
    8,      /* HDMITX_AFS_88_2k */
    10,     /* HDMITX_AFS_96k */
    12,     /* HDMITX_AFS_176_4k */
    14,     /* HDMITX_AFS_192k */
    9,      /* HDMITX_AFS_768k */
    1,      /* HDMITX_AFS_NOT_INDICATED */
};



/**
 *  Lookup table for each CTS X factor's k and m register values
 */
static UInt8 kCtsXToMK[HDMITX_CTSX_NUM][2] =
{
/*   Register values    Actual values */
/*   m  k               m, k */ 
    {3, 0},          /* 8, 1 */
    {3, 1},          /* 8, 2 */
    {3, 2},          /* 8, 3 */
    {3, 3},          /* 8, 4 */
    {0, 0}           /* 1, 1 */
};

/**
 * Table of registers to reset and release the CTS generator
 */
static tmHdmiTxRegMaskVal_t kResetCtsGenerator[] =
{
    {E_REG_P11_AIP_CNTRL_0_RW,  E_MASKREG_P11_AIP_CNTRL_0_rst_cts,  1},
    {E_REG_P11_AIP_CNTRL_0_RW,  E_MASKREG_P11_AIP_CNTRL_0_rst_cts,  0},
    {0,0,0}
};

/**
 * Table of registers to bypass colour processing (up/down sampler & matrix)
 */
static tmHdmiTxRegMaskVal_t kBypassColourProc[] =
{
    /* Bypass upsampler for RGB colourbars */
    {E_REG_P00_HVF_CNTRL_0_W,   E_MASKREG_P00_HVF_CNTRL_0_intpol,   0},
    /* Bypass matrix for RGB colourbars */
    {E_REG_P00_MAT_CONTRL_W,    E_MASKREG_P00_MAT_CONTRL_mat_bp,    1},
    /* Bypass downsampler for RGB colourbars */
    {E_REG_P00_HVF_CNTRL_1_W,   E_MASKREG_P00_HVF_CNTRL_1_for,      0},
    {0,0,0}
};

/**
 * Table of registers to configure video input mode CCIR656*/
static tmHdmiTxRegMaskVal_t kVinModeCCIR656[] =
{
    {E_REG_P00_VIP_CNTRL_4_W,   E_MASKREG_P00_VIP_CNTRL_4_ccir656,      1},
    {E_REG_P00_HVF_CNTRL_1_W,   E_MASKREG_P00_HVF_CNTRL_1_semi_planar,  1},
    /*{E_REG_P02_SEL_CLK_RW,      E_MASKREG_P02_SEL_CLK_sel_clk1,         1},*/
    {E_REG_P02_PLL_SERIAL_3_RW, E_MASKREG_P02_PLL_SERIAL_3_srl_ccir,    1},
    {E_REG_P02_SEL_CLK_RW,      E_MASKREG_P02_SEL_CLK_sel_vrf_clk,      1},
    {0,0,0}
};

 /* Table of registers to configure video input mode for CCIR656 DDR with 1280*720p and 1920*1080i formats*/
static tmHdmiTxRegMaskVal_t kVinModeCCIR656_DDR_above720p[] =
{
    {E_REG_P00_VIP_CNTRL_4_W,   E_MASKREG_P00_VIP_CNTRL_4_ccir656,      1},
    {E_REG_P00_HVF_CNTRL_1_W,   E_MASKREG_P00_HVF_CNTRL_1_semi_planar,  1},/*To be defined*/
    /*{E_REG_P02_SEL_CLK_RW,      E_MASKREG_P02_SEL_CLK_sel_clk1,         0},To be defined*/
    {E_REG_P02_PLL_SERIAL_3_RW, E_MASKREG_P02_PLL_SERIAL_3_srl_ccir,    0},
    {E_REG_P02_SEL_CLK_RW,      E_MASKREG_P02_SEL_CLK_sel_vrf_clk,      0},
    {0,0,0}
};
/**
 * Table of registers to configure video input mode RGB444 or YUV444
 */
static tmHdmiTxRegMaskVal_t kVinMode444[] =
{
    {E_REG_P00_VIP_CNTRL_4_W,   E_MASKREG_P00_VIP_CNTRL_4_ccir656,      0},
    {E_REG_P00_HVF_CNTRL_1_W,   E_MASKREG_P00_HVF_CNTRL_1_semi_planar,  0},
   /* {E_REG_P02_SEL_CLK_RW,      E_MASKREG_P02_SEL_CLK_sel_clk1,         0},*/
    {E_REG_P02_PLL_SERIAL_3_RW, E_MASKREG_P02_PLL_SERIAL_3_srl_ccir,    0},
    {E_REG_P02_SEL_CLK_RW,      E_MASKREG_P02_SEL_CLK_sel_vrf_clk,      0},
    {0,0,0}
};

/**
 * Table of registers to configure video input mode YUV422
 */
static tmHdmiTxRegMaskVal_t kVinModeYUV422[] =
{
    {E_REG_P00_VIP_CNTRL_4_W,   E_MASKREG_P00_VIP_CNTRL_4_ccir656,      0},
    {E_REG_P00_HVF_CNTRL_1_W,   E_MASKREG_P00_HVF_CNTRL_1_semi_planar,  1},
    /*{E_REG_P02_SEL_CLK_RW,      E_MASKREG_P02_SEL_CLK_sel_clk1,         0},*/
    {E_REG_P02_PLL_SERIAL_3_RW, E_MASKREG_P02_PLL_SERIAL_3_srl_ccir,    0},
    {E_REG_P02_SEL_CLK_RW,      E_MASKREG_P02_SEL_CLK_sel_vrf_clk,      0},
    {0,0,0}
};


/**
 *  Lookup table for colour space conversion matrix register sets.
 *  Each array consists of 31 register values from MAT_CONTROL through
 *  to MAT_OO3_LSB
 */
static UInt8 kMatrixPreset[MATRIX_PRESET_QTY][MATRIX_PRESET_SIZE] =
{
    {0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3, 0x6F, 0x0, 0x0, 0x0,
     0x0, 0x0, 0x0, 0x3, 0x6F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3,
     0x6F, 0x0, 0x40, 0x0, 0x40, 0x0, 0x40
    },  /* RGB Full to RGB Limited */

    {0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x2, 0x4, 0x1, 0x7, 0x0,
     0x64, 0x6, 0x88, 0x1, 0xC2, 0x7, 0xB7, 0x6, 0xD6, 0x7, 0x68, 0x1,
     0xC2, 0x0, 0x40, 0x2, 0x0, 0x2, 0x0
    },  /* RGB Full to BT601 */

    {0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x2, 0x75, 0x0, 0xBB, 0x0,
     0x3F, 0x6, 0x68, 0x1, 0xC2, 0x7, 0xD7, 0x6, 0xA6, 0x7, 0x99, 0x1,
     0xC2, 0x0, 0x40, 0x2, 0x0, 0x2, 0x0
    },  /* RGB Full to BT709 */

    {0x1, 0x7, 0xC0, 0x7, 0xC0, 0x7, 0xC0, 0x2, 0x54, 0x0, 0x0, 0x0,
     0x0, 0x0, 0x0, 0x2, 0x54, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x2,
     0x54, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
    },  /* RGB Limited to RGB Full */

    {0x2, 0x7, 0xC0, 0x7, 0xC0, 0x7, 0xC0, 0x2, 0x59, 0x1, 0x32, 0x0,
     0x75, 0x6, 0x4A, 0x2, 0x0C, 0x7, 0xAB, 0x6, 0xA5, 0x7, 0x4F, 0x2,
     0x0C, 0x0, 0x40, 0x2, 0x0, 0x2, 0x0
    },  /* RGB Limited to BT601 */

    {0x2, 0x7, 0xC0, 0x7, 0xC0, 0x7, 0xC0, 0x2, 0xDC, 0x0, 0xDA, 0x0,
     0x4A, 0x6, 0x24, 0x2, 0x0C, 0x7, 0xD0, 0x6, 0x6C, 0x7, 0x88, 0x2,
     0x0C, 0x0, 0x40, 0x2, 0x0, 0x2, 0x0
    },  /* RGB Limited to BT709 */

    {0x0, 0x7, 0xC0, 0x6, 0x0, 0x6, 0x0, 0x1, 0x2A, 0x7, 0x30, 0x7,
     0x9C, 0x1, 0x2A, 0x1, 0x99, 0x0, 0x0, 0x1, 0x2A, 0x0, 0x0, 0x2,
     0x5, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
    },  /* BT601 to RGB Full */

    {0x1, 0x7, 0xC0, 0x6, 0x0, 0x6, 0x0, 0x2, 0x0, 0x6, 0x9A, 0x7,
     0x54, 0x2, 0x0, 0x2, 0xBE, 0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0x3,
     0x77, 0x0, 0x40, 0x0, 0x40, 0x0, 0x40
    },  /* BT601 to RGB Limited */

    {0x1, 0x7, 0xC0, 0x6, 0x0, 0x6, 0x0, 0x2, 0x0, 0x7, 0x96, 0x7,
     0xC5, 0x0, 0x0, 0x2, 0x0D, 0x0, 0x26, 0x0, 0x0, 0x0, 0x3B, 0x2,
     0x0A, 0x0, 0x40, 0x2, 0x0, 0x2, 0x0
    },  /* BT601 to BT709 */

    {0x0, 0x7, 0xC0, 0x6, 0x0, 0x6, 0x0, 0x1, 0x2A, 0x7, 0x77, 0x7,
     0xC9, 0x1, 0x2A, 0x1, 0xCB, 0x0, 0x0, 0x1, 0x2A, 0x0, 0x0, 0x2,
     0x1D, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
    },  /* BT709 to RGB Full */

    {0x1, 0x7, 0xC0, 0x6, 0x0, 0x6, 0x0, 0x2, 0x0, 0x7, 0x16, 0x7,
     0xA2, 0x2, 0x0, 0x3, 0x14, 0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0x3,
     0xA1, 0x0, 0x40, 0x0, 0x40, 0x0, 0x40
    },  /* BT709 to RGB Limited */

    {0x1, 0x7, 0xC0, 0x6, 0x0, 0x6, 0x0, 0x2, 0x0, 0x0, 0x62, 0x0,
     0x33, 0x0, 0x0, 0x1, 0xF7, 0x7, 0xDB, 0x0, 0x0, 0x7, 0xC7, 0x1,
     0xFB, 0x0, 0x40, 0x2, 0x0, 0x2, 0x0
    }  /* BT709 to BT601 */
}; 

/**
 *  This table gives us the index into the kMatrixPreset array, based
 *  on the input and output colourspaces.
 *  The co-ordinates into this array are tmbslTDA9989Colourspace_t enums.
 *  The value of -1 is returned for matching input/output colourspaces.
 */
static Int kMatrixIndex[HDMITX_CS_NUM][HDMITX_CS_NUM] =
{
    {-1, E_MATRIX_RGBF_2_RGBL, E_MATRIX_RGBF_2_BT601, E_MATRIX_RGBF_2_BT709},
    {E_MATRIX_RGBL_2_RGBF, -1, E_MATRIX_RGBL_2_BT601, E_MATRIX_RGBL_2_BT709},
    {E_MATRIX_BT601_2_RGBF, E_MATRIX_BT601_2_RGBL, -1, E_MATRIX_BT601_2_BT709},
    {E_MATRIX_BT709_2_RGBF, E_MATRIX_BT709_2_RGBL, E_MATRIX_BT709_2_BT601, -1}
};

/**
 *  Blue filter Lookup table for colour space conversion.
 *  Each array consists of 31 register values from MAT_CONTROL through
 *  to MAT_OO3_LSB
 */
static UInt8 MatrixCoeffBlueScreen[][MATRIX_PRESET_SIZE] =
{
    {0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
     0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
     0x0, 0x0, 0x0, 0x0, 0x0, 0x2, 0x0
    },  /* blue screen for RGB output color space */

    {0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
     0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
     0x0, 0x0, 0x0, 0x1, 0x0, 0x3, 0x0
    },  /* blue screen for YCbCr422 output color space */

    {0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
     0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
     0x0, 0x0, 0x0, 0x1, 0x0, 0x3, 0x0
    },  /* blue screen for YCbCr444 output color space */
};

/**
 *  Black filter Lookup table for colour space conversion.
 *  Each array consists of 31 register values from MAT_CONTROL through
 *  to MAT_OO3_LSB
 */
static UInt8 MatrixCoeffBlackScreen[][MATRIX_PRESET_SIZE] =
{
    {0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
     0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
     0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
    },  /* black screen for RGB output color space */

    {0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
     0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
     0x0, 0x0, 0x0, 0x2, 0x0, 0x2, 0x0
    },  /* black screen for YCbCr422 output color space */

    {0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
     0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
     0x0, 0x0, 0x0, 0x2, 0x0, 0x2, 0x0
    },  /* black screen for YCbCr444 output color space */
};


/*============================================================================*/
/*                       DEFINES DECLARATIONS                               */
/*============================================================================*/
#define HDMITX_LAT_SCO_MAX_VAL 40
#define HDMITX_LAT_SCO_MIN_VAL 34

/*============================================================================*/
/*                       VARIABLES DECLARATIONS                               */
/*============================================================================*/

/* Register values per device to restore colour processing after test pattern */
static UInt8   gMatContrl[HDMITX_UNITS_MAX];
static UInt8   gHvfCntrl0[HDMITX_UNITS_MAX];
static UInt8   gHvfCntrl1[HDMITX_UNITS_MAX];

/*============================================================================*/
/*                       FUNCTION PROTOTYPES                                  */
/*============================================================================*/

static tmErrorCode_t setDeVs(tmHdmiTxobject_t *pDis,
                             tmbslHdmiTxVidFmt_t voutFmt,
                             tmbslHdmiTx3DStructure_t structure3D);
static tmErrorCode_t setPixelRepeat(tmHdmiTxobject_t *pDis, 
                                    tmbslHdmiTxVidFmt_t voutFmt,
                                    UInt8 uPixelRepeat,
                                    tmbslHdmiTx3DStructure_t structure3D);
static tmErrorCode_t setSampling(tmHdmiTxobject_t *pDis);
static UInt8 calculateChecksum(UInt8 *pData, Int numBytes);
static UInt8 reg_vid_fmt(tmbslHdmiTxVidFmt_t fmt,              \
                         tmbslHdmiTx3DStructure_t structure3D, \
                         UInt8 *idx,                           \
                         UInt8 *idx3d,                         \
                         struct sync_desc **sync);
UInt8 pix_clk(tmbslHdmiTxVidFmt_t fmt, tmbslHdmiTxVfreq_t freq, UInt8 *pclk);
static tmErrorCode_t InputConfig(tmHdmiTxobject_t *pDis,
                                 tmbslHdmiTxVinMode_t vinMode,
                                 tmbslHdmiTxPixEdge_t sampleEdge,
                                 tmbslHdmiTxPixRate_t pixRate,
                                 tmbslHdmiTxUpsampleMode_t upsampleMode, 
                                 UInt8 uPixelRepeat,
                                 tmbslHdmiTxVidFmt_t voutFmt,
                                 tmbslHdmiTx3DStructure_t structure3D);
/*============================================================================*/
/* tmbslTDA9989AudioInResetCts                                                 */
/*============================================================================*/
tmErrorCode_t
tmbslTDA9989AudioInResetCts
(
    tmUnitSelect_t              txUnit
)
{
    tmHdmiTxobject_t *pDis;     /* Pointer to Device Instance Structure */
    tmErrorCode_t     err;      /* Error code */

    /* Check unit parameter and point to its object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Return if sink is not an HDMI device */
    RETIF(pDis->sinkType != HDMITX_SINK_HDMI, 
          TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED)

    /* Reset and release the CTS generator */
    err = setHwRegisterFieldTable(pDis, &kResetCtsGenerator[0]);
    return err;
}


/*============================================================================*/
/* tmbslTDA9989AudioInSetConfig                                                */
/*============================================================================*/
tmErrorCode_t
tmbslTDA9989AudioInSetConfig
(
    tmUnitSelect_t           txUnit,
    tmbslHdmiTxaFmt_t        aFmt,
    tmbslHdmiTxI2sFor_t      i2sFormat,
    UInt8                    chanI2s,
    UInt8                    chanDsd,
    tmbslHdmiTxClkPolDsd_t   clkPolDsd,
    tmbslHdmiTxSwapDsd_t     swapDsd,
    UInt8                    layout,
    UInt16                   uLatency_rd,
    tmbslHdmiTxDstRate_t     dstRate
)
{
    tmHdmiTxobject_t *pDis;     /* Pointer to Device Instance Structure */
    tmErrorCode_t     err;      /* Error code */
    UInt8             regVal;   /* Value to write in register */

    DUMMY_ACCESS(dstRate);

    /* Check unit parameter and point to its object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Return if sink is not an HDMI device */
    RETIF(pDis->sinkType != HDMITX_SINK_HDMI, 
          TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED)

    /* Check remaining parameters */
#ifdef TMFL_HBR_SUPPORT
    RETIF_BADPARAM((aFmt != HDMITX_AFMT_SPDIF) &&
                   (aFmt != HDMITX_AFMT_I2S)   &&
                   (aFmt != HDMITX_AFMT_OBA)   &&
                   (aFmt != HDMITX_AFMT_HBR))

#else /* TMFL_HBR_SUPPORT */
    RETIF_BADPARAM((aFmt != HDMITX_AFMT_SPDIF) &&
                   (aFmt != HDMITX_AFMT_I2S)   &&
                   (aFmt != HDMITX_AFMT_OBA))


#endif /* TMFL_HBR_SUPPORT */

    RETIF_BADPARAM(chanI2s     >= HDMITX_CHAN_INVALID)
    RETIF_BADPARAM(chanDsd     >= HDMITX_CHAN_INVALID)
    RETIF_BADPARAM(clkPolDsd   >= HDMITX_CLKPOLDSD_INVALID)
    RETIF_BADPARAM(swapDsd     >= HDMITX_SWAPDSD_INVALID)
    RETIF_BADPARAM(layout      >= HDMITX_LAYOUT_INVALID)
    RETIF_BADPARAM(uLatency_rd >= HDMITX_LATENCY_INVALID)

    if ((aFmt == HDMITX_AFMT_I2S) 
#ifdef TMFL_HBR_SUPPORT 
        || (aFmt == HDMITX_AFMT_HBR)
#endif /* TMFL_HBR_SUPPORT */
        )
    {
        RETIF_BADPARAM((i2sFormat != HDMITX_I2SFOR_PHILIPS_L) &&
                       (i2sFormat != HDMITX_I2SFOR_OTH_L)     &&
                       (i2sFormat != HDMITX_I2SFOR_OTH_R)
                       )
    }

#ifdef TMFL_HDCP_OPTIMIZED_POWER
    /* 
       power management :
       freeze/wakeup SPDIF clock
    */
    err = setHwRegisterField(pDis, E_REG_FEAT_POWER_DOWN,               \
                             E_MASKREG_FEAT_POWER_DOWN_spdif,           \
                             (aFmt != HDMITX_AFMT_SPDIF));
    RETIF_REG_FAIL(err);
#endif

    switch (aFmt)
    {
        case HDMITX_AFMT_SPDIF:
            regVal = (UInt8)REG_VAL_SEL_AIP_SPDIF;
            /* configure MUX_AP */
            err = setHwRegister(pDis, E_REG_P00_MUX_AP_RW, TDA19989_MUX_AP_SELECT_SPDIF);
            RETIF_REG_FAIL(err)
            break;

        case HDMITX_AFMT_I2S:
            regVal = (UInt8)REG_VAL_SEL_AIP_I2S;
            /* configure MUX_AP */
            err = setHwRegister(pDis, E_REG_P00_MUX_AP_RW, TDA19989_MUX_AP_SELECT_I2S);
            RETIF_REG_FAIL(err)
            break;

        case HDMITX_AFMT_OBA:
            regVal = (UInt8)REG_VAL_SEL_AIP_OBA;
            break;

        case HDMITX_AFMT_HBR:
            regVal = (UInt8)REG_VAL_SEL_AIP_HBR;
            break;
            
        default:
            return TMBSL_ERR_HDMI_BAD_PARAMETER;
    }


    /* Set the audio input processor format to aFmt. */
    err = setHwRegisterField(pDis,
                             E_REG_P00_AIP_CLKSEL_W,
                             E_MASKREG_P00_AIP_CLKSEL_sel_aip,
                             regVal);
    RETIF_REG_FAIL(err)

    /* Channel status on 1 channel  */
    err = setHwRegisterField(pDis,
                             E_REG_P11_CA_I2S_RW,
                             E_MASKREG_P11_CA_I2S_hbr_chstat_4,
                             0);
    RETIF_REG_FAIL(err)

    /* Select the audio format */
    if ((aFmt == HDMITX_AFMT_I2S) 
#ifdef TMFL_HBR_SUPPORT
        || (aFmt == HDMITX_AFMT_HBR)
#endif /* TMFL_HBR_SUPPORT */
       )
    {
        if (chanI2s != HDMITX_CHAN_NO_CHANGE)
        {
            err = setHwRegisterField(pDis,
                  E_REG_P11_CA_I2S_RW,
                  E_MASKREG_P11_CA_I2S_ca_i2s,
                  (UInt8)chanI2s);
        }

        /* Select the I2S format */
        err = setHwRegisterField(pDis,
                                 E_REG_P00_I2S_FORMAT_RW,
                                 E_MASKREG_P00_I2S_FORMAT_i2s_format,
                                 (UInt8)i2sFormat);
        RETIF_REG_FAIL(err)

//#ifdef TMFL_HBR_SUPPORT 
      //if (aFmt == HDMITX_AFMT_HBR)
      // {
            /* Channel status on 1 channel  */
      //      err = setHwRegisterField(pDis,
      //                       E_REG_P11_CA_I2S_RW,
      //                       E_MASKREG_P11_CA_I2S_hbr_chstat_4,
      //                       1);
      //      RETIF_REG_FAIL(err)
      //  }
//#endif /* TMFL_HBR_SUPPORT */
    }
    else if (aFmt == HDMITX_AFMT_OBA)
    {
        if (chanDsd != HDMITX_CHAN_NO_CHANGE)
        {
            err = setHwRegister(pDis, E_REG_P11_CA_DSD_RW, chanDsd);
            RETIF_REG_FAIL(err)
        }
        if (clkPolDsd != HDMITX_CLKPOLDSD_NO_CHANGE)
        {
            err = setHwRegisterField(pDis,
                                     E_REG_P00_AIP_CLKSEL_W,
                                     E_MASKREG_P00_AIP_CLKSEL_sel_pol_clk,
                                     (UInt8)clkPolDsd);
            RETIF_REG_FAIL(err)
        }
        if (swapDsd != HDMITX_SWAPDSD_NO_CHANGE)
        {
            err = setHwRegisterField(pDis,
                                     E_REG_P11_AIP_CNTRL_0_RW,
                                     E_MASKREG_P11_AIP_CNTRL_0_swap,
                                     (UInt8)swapDsd);
            RETIF_REG_FAIL(err)
        }
    }

    /* Set layout and latency */
    if (layout != HDMITX_LAYOUT_NO_CHANGE)
    {
        err = setHwRegisterField(pDis,
                                 E_REG_P11_AIP_CNTRL_0_RW,
                                 E_MASKREG_P11_AIP_CNTRL_0_layout,
                                 layout);
        RETIF_REG_FAIL(err)
    }
    if (uLatency_rd != HDMITX_LATENCY_NO_CHANGE)
    {
        err = setHwRegister(pDis, E_REG_P11_LATENCY_RD_RW, (UInt8)uLatency_rd);
        RETIF_REG_FAIL(err)
    }
    return TM_OK;
}

/*============================================================================*/
/* tmbslTDA9989AudioInSetCts                                                   */
/*============================================================================*/
tmErrorCode_t
tmbslTDA9989AudioInSetCts
(
    tmUnitSelect_t       txUnit,
    tmbslHdmiTxctsRef_t  ctsRef,
    tmbslHdmiTxafs_t     afs, 
    tmbslHdmiTxVidFmt_t  voutFmt, 
    tmbslHdmiTxVfreq_t   voutFreq, 
    UInt32               uCts, 
    UInt16               uCtsX,
    tmbslHdmiTxctsK_t    ctsK,
    tmbslHdmiTxctsM_t    ctsM,
    tmbslHdmiTxDstRate_t dstRate
)
{
    tmHdmiTxobject_t *pDis;     /* Pointer to Device Instance Structure */
    tmErrorCode_t   err;        /* Error code */
    UInt8           regVal;     /* Register value */
    UInt8           pixClk;     /* Pixel clock index */
    UInt32          acrN;       /* Audio clock recovery N */

    DUMMY_ACCESS(dstRate);

    /* Check unit parameter and point to its object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Return if sink is not an HDMI device */
    RETIF(pDis->sinkType != HDMITX_SINK_HDMI, 
          TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED)

    /* Check remaining parameters */
    RETIF_BADPARAM(ctsRef   >= HDMITX_CTSREF_INVALID)
    RETIF_BADPARAM(afs      >= HDMITX_AFS_INVALID)
    RETIF_BADPARAM(!IS_VALID_FMT(voutFmt))
    RETIF_BADPARAM(voutFreq >= HDMITX_VFREQ_INVALID)
    RETIF_BADPARAM(uCtsX    >= HDMITX_CTSX_INVALID)
    RETIF_BADPARAM(ctsK     >= HDMITX_CTSK_INVALID)
    RETIF_BADPARAM(ctsM     >= HDMITX_CTSMTS_INVALID)
   
    
    if (IS_TV(voutFmt))
    {
       if (voutFreq == HDMITX_VFREQ_50Hz)
          {
             RETIF(((voutFmt < HDMITX_VFMT_17_720x576p_50Hz)        \
                    || (voutFmt > HDMITX_VFMT_31_1920x1080p_50Hz)),
                   TMBSL_ERR_HDMI_INCONSISTENT_PARAMS);
          }
       else if (voutFreq == HDMITX_VFREQ_24Hz)
          {
             RETIF((voutFmt != HDMITX_VFMT_32_1920x1080p_24Hz)      \
                   && (voutFmt != HDMITX_VFMT_60_1280x720p_24Hz),
                   TMBSL_ERR_HDMI_INCONSISTENT_PARAMS);
          }
       else if (voutFreq == HDMITX_VFREQ_25Hz)
          {
             RETIF((voutFmt != HDMITX_VFMT_33_1920x1080p_25Hz)      \
                   && (voutFmt != HDMITX_VFMT_20_1920x1080i_50Hz)   \
                   && (voutFmt != HDMITX_VFMT_61_1280x720p_25Hz),
                   TMBSL_ERR_HDMI_INCONSISTENT_PARAMS);
          }
       else if (voutFreq == HDMITX_VFREQ_30Hz)
          {
             RETIF((voutFmt != HDMITX_VFMT_34_1920x1080p_30Hz)      \
                   && (voutFmt != HDMITX_VFMT_05_1920x1080i_60Hz)   \
                   && (voutFmt != HDMITX_VFMT_62_1280x720p_30Hz),
                   TMBSL_ERR_HDMI_INCONSISTENT_PARAMS);
          }
       else
          {
             RETIF(voutFmt >= HDMITX_VFMT_17_720x576p_50Hz,
                   TMBSL_ERR_HDMI_INCONSISTENT_PARAMS);
          }
    }

#ifdef FORMAT_PC
    if (IS_PC(voutFmt))
    {
        if (voutFreq == HDMITX_VFREQ_60Hz)
        {
            RETIF(voutFmt > HDMITX_VFMT_PC_1600x1200p_60Hz,
                                       TMBSL_ERR_HDMI_INCONSISTENT_PARAMS)
        }
        else if (voutFreq == HDMITX_VFREQ_70Hz)
        {
            RETIF(voutFmt != HDMITX_VFMT_PC_1024x768p_70Hz,
                                       TMBSL_ERR_HDMI_INCONSISTENT_PARAMS)
        }
        else if (voutFreq == HDMITX_VFREQ_72Hz)
        {
            RETIF( ((voutFmt < HDMITX_VFMT_PC_640x480p_72Hz) ||
                    (voutFmt > HDMITX_VFMT_PC_800x600p_72Hz)),
                                       TMBSL_ERR_HDMI_INCONSISTENT_PARAMS)
        }
        else if (voutFreq == HDMITX_VFREQ_75Hz)
        {
            RETIF( ((voutFmt < HDMITX_VFMT_PC_640x480p_75Hz) ||
                    (voutFmt > HDMITX_VFMT_PC_1280x1024p_75Hz)),
                                       TMBSL_ERR_HDMI_INCONSISTENT_PARAMS)
        }
        else if (voutFreq == HDMITX_VFREQ_85Hz)
        {
            RETIF( ((voutFmt < HDMITX_VFMT_PC_640x350p_85Hz) ||
                    (voutFmt > HDMITX_VFMT_PC_1280x1024p_85Hz)),
                                       TMBSL_ERR_HDMI_INCONSISTENT_PARAMS)
        }
        else
        {
            RETIF(voutFmt != HDMITX_VFMT_PC_1024x768i_87Hz,
                                       TMBSL_ERR_HDMI_INCONSISTENT_PARAMS)
        }
    }
#endif /* FORMAT_PC */

    /* Check for auto or manual CTS */
    if (uCts == HDMITX_CTS_AUTO)
    {
        /* Auto */
        err = setHwRegisterField(pDis,
                                 E_REG_P11_AIP_CNTRL_0_RW,
                                 E_MASKREG_P11_AIP_CNTRL_0_acr_man,
                                 0);
        RETIF_REG_FAIL(err)
    }
    else
    {
        /* Manual */
        err = setHwRegisterField(pDis,
                                 E_REG_P11_AIP_CNTRL_0_RW,
                                 E_MASKREG_P11_AIP_CNTRL_0_acr_man,
                                 1);
        RETIF_REG_FAIL(err)
    }

    /* Derive M and K from X? */
    if ((ctsM == HDMITX_CTSMTS_USE_CTSX) || (ctsK == HDMITX_CTSK_USE_CTSX))
    {
        RETIF_BADPARAM(uCtsX == HDMITX_CTSX_UNUSED)
        ctsM = (tmbslHdmiTxctsM_t) kCtsXToMK[uCtsX][0];
        ctsK = (tmbslHdmiTxctsK_t)kCtsXToMK[uCtsX][1];
    }

    /* Set the Post-divider measured timestamp factor */
    regVal = (UInt8)ctsM;
    err = setHwRegisterField(pDis,
                             E_REG_P11_CTS_N_RW,
                             E_MASKREG_P11_CTS_N_m_sel,
                             regVal);
    RETIF_REG_FAIL(err)

    /* Set the Pre-divider scale */
    regVal = (UInt8)ctsK;
    err = setHwRegisterField(pDis,
                             E_REG_P11_CTS_N_RW,
                             E_MASKREG_P11_CTS_N_k_sel,
                             regVal);
    RETIF_REG_FAIL(err);

    /* Use voutFmt and voutFreq to index into a lookup table to get
     * the current pixel clock value. */
    pix_clk(voutFmt, voutFreq, &pixClk);

    if (pixClk != E_PIXCLK_INVALID)
    {
        /* Set the Audio Clock Recovery N multiplier based on the audio sample
         * frequency afs and current pixel clock. */
        acrN = kPixClkToAcrN[pixClk][afs];

        /* Set ACR N multiplier [19 to 16] */
        regVal = (UInt8)(acrN >> 16);
        err = setHwRegister(pDis, E_REG_P11_ACR_N_2_RW, regVal);
        RETIF_REG_FAIL(err)
        /* Set ACR N multiplier [15 to 8] */
        regVal = (UInt8)(acrN >> 8);
        err = setHwRegister(pDis, E_REG_P11_ACR_N_1_RW, regVal);
        RETIF_REG_FAIL(err)
        /* Set ACR N multiplier [7 to 0] */
        regVal = (UInt8)acrN;
        err = setHwRegister(pDis, E_REG_P11_ACR_N_0_RW, regVal);
        RETIF_REG_FAIL(err)

        /* Set the CDC Audio Divider register based on the audio sample frequency
         * afs and current pixel clock. */
        regVal = kPixClkToAdiv[pixClk][afs];
        err = setHwRegister(pDis, E_REG_P02_AUDIO_DIV_RW, regVal);
        RETIF_REG_FAIL(err)

        /* If auto CTS, get CTS value based on the audio sample
         * frequency afs and current pixel clock. */
        if (uCts == HDMITX_CTS_AUTO)
        {
            uCts = kPixClkToAcrCts[pixClk][afs];
        }
    }

    /* Set manual or pixel clock CTS */
    if (uCts != HDMITX_CTS_AUTO)
    {
        /* Set manual ACR CTS [19 to 16 */
        regVal = (UInt8)(uCts >> 16);
        err = setHwRegister(pDis, E_REG_P11_ACR_CTS_2_RW, regVal);
        RETIF_REG_FAIL(err)
        /* Set manual ACR CTS [15 to 8] */
        regVal = (UInt8)(uCts >> 8);
        err = setHwRegister(pDis, E_REG_P11_ACR_CTS_1_RW, regVal);
        RETIF_REG_FAIL(err)
        /* Set manual ACR CTS [7 to 0] */
        regVal = (UInt8)uCts;
        err = setHwRegister(pDis, E_REG_P11_ACR_CTS_0_RW, regVal);
        RETIF_REG_FAIL(err)
    }

    /* Set the CTS clock reference register according to ctsRef */
    regVal = (UInt8)ctsRef;
    err = setHwRegisterField(pDis,
                             E_REG_P00_AIP_CLKSEL_W,
                             E_MASKREG_P00_AIP_CLKSEL_sel_fs,
                             regVal);
    RETIF_REG_FAIL(err)

    /* Reset and release the CTS generator */
    err = setHwRegisterFieldTable(pDis, &kResetCtsGenerator[0]);
    return err;
}

/*============================================================================*/
/* tmbslTDA9989AudioOutSetChanStatus                                           */
/*============================================================================*/
tmErrorCode_t
tmbslTDA9989AudioOutSetChanStatus
(
    tmUnitSelect_t               txUnit,
    tmbslHdmiTxAudioData_t       pcmIdentification,
    tmbslHdmiTxCSformatInfo_t    formatInfo,
    tmbslHdmiTxCScopyright_t     copyright,
    UInt8                        categoryCode,
    tmbslHdmiTxafs_t             sampleFreq,
    tmbslHdmiTxCSclkAcc_t        clockAccuracy,
    tmbslHdmiTxCSmaxWordLength_t maxWordLength,
    tmbslHdmiTxCSwordLength_t    wordLength,
    tmbslHdmiTxCSorigAfs_t       origSampleFreq
)
{
    tmHdmiTxobject_t *pDis;     /* Pointer to Device Instance Structure */
    tmErrorCode_t     err;      /* Error code */
    UInt8             buf[4];   /* Buffer to hold channel status data */

    /* Check unit parameter and point to its object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Return if sink is not an HDMI device */
    RETIF(pDis->sinkType != HDMITX_SINK_HDMI, 
          TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED)

    /* Check remaining parameters */
    RETIF_BADPARAM(formatInfo     >= HDMITX_CSFI_INVALID)
    RETIF_BADPARAM(copyright      >= HDMITX_CSCOPYRIGHT_INVALID)
    RETIF_BADPARAM(sampleFreq     >  HDMITX_AFS_NOT_INDICATED)
    RETIF_BADPARAM(clockAccuracy  >= HDMITX_CSCLK_INVALID)
    RETIF_BADPARAM(maxWordLength  >= HDMITX_CSMAX_INVALID)
    RETIF_BADPARAM(wordLength     >= HDMITX_CSWORD_INVALID)
    RETIF_BADPARAM(wordLength     == HDMITX_CSWORD_RESVD)
    RETIF_BADPARAM(origSampleFreq >= HDMITX_CSAFS_INVALID)
    RETIF_BADPARAM(pcmIdentification >=HDMITX_AUDIO_DATA_INVALID)

    /* Prepare Byte 0 */
    buf[0] = ((UInt8)formatInfo << 3) | ((UInt8)copyright << 2) | ((UInt8)pcmIdentification<< 1);

    /* Prepare Byte 1 */
    buf[1] = categoryCode;

    /* Prepare Byte 3  - note Byte 2 not in sequence in TDA9983 register map */
    buf[2] = ((UInt8)clockAccuracy << 4) | kAfsToCSbyte3[sampleFreq];

    /* Prepare Byte 4 */
    buf[3] = ((UInt8)origSampleFreq << 4) | ((UInt8)wordLength << 1) |
              (UInt8)maxWordLength;

    /* Write 4 Channel Status bytes */
    err = setHwRegisters(pDis, E_REG_P11_CH_STAT_B_0_RW, &buf[0], 4);
    return err;
}

/*============================================================================*/
/* tmbslTDA9989AudioOutSetChanStatusMapping                                    */
/*============================================================================*/

tmErrorCode_t
tmbslTDA9989AudioOutSetChanStatusMapping
(
    tmUnitSelect_t  txUnit,
    UInt8           sourceLeft[4],
    UInt8           channelLeft[4],
    UInt8           sourceRight[4],
    UInt8           channelRight[4]
)
{
    tmHdmiTxobject_t *pDis;     /* Pointer to Device Instance Structure */
    tmErrorCode_t     err;      /* Error code */
    UInt8             buf[2];   /* Buffer to hold channel status data */

    /* Check unit parameter and point to its object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Return if sink is not an HDMI device */
    RETIF(pDis->sinkType != HDMITX_SINK_HDMI, 
          TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED)

    /* Check remaining parameters */
    RETIF_BADPARAM(sourceLeft[0]   > HDMITX_CS_SOURCES_MAX)
    RETIF_BADPARAM(channelLeft[0]  > HDMITX_CS_CHANNELS_MAX)
    RETIF_BADPARAM(sourceRight[0]  > HDMITX_CS_SOURCES_MAX)
    RETIF_BADPARAM(channelRight[0] > HDMITX_CS_CHANNELS_MAX)

    /* Prepare Left byte */
    buf[0] = ((UInt8)channelLeft[0] << 4) | (UInt8)sourceLeft[0];

    /* Prepare Right byte */
    buf[1] = ((UInt8)channelRight[0] << 4) | (UInt8)sourceRight[0];

    /* Write 2 Channel Status bytes */
    err = setHwRegisters(pDis, E_REG_P11_CH_STAT_B_2_ap0_l_RW, &buf[0], 2);
    return err;
}


/*============================================================================*/
/* tmbslTDA9989AudioOutSetMute                                                 */
/*============================================================================*/
tmErrorCode_t
tmbslTDA9989AudioOutSetMute
(
    tmUnitSelect_t      txUnit,
    tmbslHdmiTxaMute_t  aMute
)
{
    tmHdmiTxobject_t *pDis;             /* Pointer to Device Instance Structure */
    tmErrorCode_t     err;              /* Error code */

    /* Check unit parameter and point to its object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Return if sink is not an HDMI device */
    RETIF(pDis->sinkType != HDMITX_SINK_HDMI, 
          TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED)

    /* Check remaining parameters */
    RETIF_BADPARAM(aMute >= HDMITX_AMUTE_INVALID)

    /* audio mute workaround, un-map audio input before muting */
    if (aMute == HDMITX_AMUTE_ON)
    {
        err = setHwRegisterField(pDis,
                                E_REG_P00_SR_REG_W,
                                E_MASKREG_P00_SR_REG_sr_audio,
                                (UInt8)aMute);
        RETIF(err != TM_OK, err)


        err = setHwRegisterField(pDis,
                                E_REG_P00_SR_REG_W,
                                E_MASKREG_P00_SR_REG_sr_audio,
                                (UInt8) !aMute);
        RETIF(err != TM_OK, err)

    }

    /* Reset the audio FIFO to mute audio */
    err = setHwRegisterField(pDis,
                             E_REG_P11_AIP_CNTRL_0_RW,
                             E_MASKREG_P11_AIP_CNTRL_0_rst_fifo,
                             (UInt8)aMute);
    RETIF(err != TM_OK, err)


    return TM_OK;

}

/*============================================================================*/
/* tmbslTDA9989ScalerGet                                                       */
/*============================================================================*/

tmErrorCode_t
tmbslTDA9989ScalerGet
(
    tmUnitSelect_t            txUnit,
    tmbslHdmiTxScalerDiag_t  *pScalerDiag
)
{
    DUMMY_ACCESS(txUnit); /* else not referenced */
    DUMMY_ACCESS(pScalerDiag); /* else not referenced */
    return TMBSL_ERR_HDMI_NOT_SUPPORTED;
}


/*============================================================================*/
/* tmbslTDA9989ScalerGetMode                                                   */
/*============================================================================*/
tmErrorCode_t
tmbslTDA9989ScalerGetMode
(
    tmUnitSelect_t            txUnit,
    tmbslHdmiTxScaMode_t      *pScalerMode
)
{
    DUMMY_ACCESS(txUnit); /* else not referenced */
    DUMMY_ACCESS(pScalerMode); /* else is declared but not used */
    return TMBSL_ERR_HDMI_NOT_SUPPORTED;
}

/*============================================================================*/
/* tmbslTDA9989ScalerInDisable                                                 */
/*============================================================================*/
tmErrorCode_t
tmbslTDA9989ScalerInDisable
(
    tmUnitSelect_t  txUnit,
    Bool            bDisable
)
{
    DUMMY_ACCESS(txUnit); /* else not referenced */
    DUMMY_ACCESS(bDisable);
    return TMBSL_ERR_HDMI_NOT_SUPPORTED;

}

/*============================================================================*/
/* tmbslTDA9989ScalerSetCoeffs                                                 */
/*============================================================================*/

tmErrorCode_t
tmbslTDA9989ScalerSetCoeffs
(
    tmUnitSelect_t        txUnit,
    tmbslHdmiTxScaLut_t   lutSel,
    UInt8                *pVsLut
)
{
    DUMMY_ACCESS(txUnit); /* else not referenced */
    DUMMY_ACCESS(lutSel); /* else is declared but not used */
    DUMMY_ACCESS(pVsLut); /* else is declared but not used */
    return TMBSL_ERR_HDMI_NOT_SUPPORTED;

}


/*============================================================================*/
/* tmbslTDA9989ScalerSetFieldOrder                                             */
/*============================================================================*/
tmErrorCode_t   
tmbslTDA9989ScalerSetFieldOrder 
(
    tmUnitSelect_t      txUnit,
    tmbslHdmiTxIntExt_t topExt,
    tmbslHdmiTxIntExt_t deExt,
    tmbslHdmiTxTopSel_t topSel,
    tmbslHdmiTxTopTgl_t topTgl 
)
{
    DUMMY_ACCESS(txUnit); /* else not referenced */
    DUMMY_ACCESS(deExt); /* else is declared but not used */
    DUMMY_ACCESS(topExt); /* else is declared but not used */
    DUMMY_ACCESS(topSel); /* else is declared but not used */
    DUMMY_ACCESS(topTgl); /* else is declared but not used */
    return TMBSL_ERR_HDMI_NOT_SUPPORTED;

}

/*============================================================================*/
/* tmbslTDA9989ScalerSetPhase                                                 */
/*============================================================================*/
tmErrorCode_t   
tmbslTDA9989ScalerSetPhase
(
    tmUnitSelect_t        txUnit,
    tmbslHdmiTxHPhases_t  horizontalPhases   
)
{
    DUMMY_ACCESS(txUnit); /* else not referenced */
    DUMMY_ACCESS(horizontalPhases); /* else is declared but not used */
    return TMBSL_ERR_HDMI_NOT_SUPPORTED;

}

/*============================================================================*/
/* tmbslTDA9989ScalerSetLatency                                                 */
/*============================================================================*/
tmErrorCode_t   
tmbslTDA9989ScalerSetLatency
(
    tmUnitSelect_t        txUnit,
    UInt8                 scaler_latency
)
{
    DUMMY_ACCESS(txUnit); /* else not referenced */
    DUMMY_ACCESS(scaler_latency); /* else is declared but not used */
    return TMBSL_ERR_HDMI_NOT_SUPPORTED;

}

/*============================================================================*/
/* tmbslTDA9989ScalerSetFine                                                   */
/*============================================================================*/
tmErrorCode_t   
tmbslTDA9989ScalerSetFine
(
    tmUnitSelect_t  txUnit,
    UInt16          uRefPix,    
    UInt16          uRefLine   
)
{
    DUMMY_ACCESS(txUnit); /* else not referenced */
    DUMMY_ACCESS(uRefPix); /* else is declared but not used */
    DUMMY_ACCESS(uRefLine); /* else is declared but not used */
    return TMBSL_ERR_HDMI_NOT_SUPPORTED;

}

/*============================================================================*/
/* tmbslTDA9989ScalerSetSync                                                   */
/*============================================================================*/
tmErrorCode_t   
tmbslTDA9989ScalerSetSync
(
    tmUnitSelect_t      txUnit,
    tmbslHdmiTxVsMeth_t method,
    tmbslHdmiTxVsOnce_t once
)
{
    DUMMY_ACCESS(txUnit); /* else not referenced */
    DUMMY_ACCESS(method); /* else is declared but not used */
    DUMMY_ACCESS(once); /* else is declared but not used */
    return TMBSL_ERR_HDMI_NOT_SUPPORTED;

}

/*============================================================================*/
/* tmbslTDA9989TmdsSetOutputs                                                  */
/*============================================================================*/
tmErrorCode_t   
tmbslTDA9989TmdsSetOutputs 
(
    tmUnitSelect_t          txUnit,
    tmbslHdmiTxTmdsOut_t    tmdsOut
)
{
    tmHdmiTxobject_t *pDis;       /* Pointer to Device Instance Structure */
    tmErrorCode_t     err;        /* Error code */

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Check parameters */
    RETIF_BADPARAM(tmdsOut >= HDMITX_TMDSOUT_INVALID)

    /* Set the TMDS output mode */
    err = setHwRegisterField(pDis,
                             E_REG_P02_BUFFER_OUT_RW,
                             E_MASKREG_P02_BUFFER_OUT_srl_force,
                             (UInt8)tmdsOut);
    return err;
}

/*============================================================================*/
/* tmbslTDA9989TmdsSetSerializer                                               */
/*============================================================================*/
tmErrorCode_t   
tmbslTDA9989TmdsSetSerializer 
(
    tmUnitSelect_t  txUnit,
    UInt8           uPhase2,
    UInt8           uPhase3
)
{

    DUMMY_ACCESS(txUnit);
    DUMMY_ACCESS(uPhase2);
    DUMMY_ACCESS(uPhase3);
    return TMBSL_ERR_HDMI_NOT_SUPPORTED;
}

/*============================================================================*/
/* tmbslTDA9989TestSetPattern                                                  */
/*============================================================================*/
tmErrorCode_t   
tmbslTDA9989TestSetPattern
(
    tmUnitSelect_t            txUnit,
    tmbslHdmiTxTestPattern_t pattern 
)
{
    tmHdmiTxobject_t *pDis;                        /* Pointer to Device Instance Structure */
    tmErrorCode_t     err;                        /* Error code */
    UInt8             serviceMode;                /* Register value */
    UInt8             bars8;                    /* Register value */
    UInt8              buf[MATRIX_PRESET_SIZE];    /* Temp buffer */
    UInt8              i;                        /* Loop index */
    UInt8              *MatrixCoeff=Null;    

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Check pattern parameters */
    switch (pattern)
    {
    case HDMITX_PATTERN_CBAR4:
        serviceMode = 1;
        bars8 = 0;
        break;
    case HDMITX_PATTERN_BLUE:
        MatrixCoeff = (UInt8*)&MatrixCoeffBlueScreen[pDis->voutMode][0]; //point to the blue matrix 
        serviceMode = 1;
        bars8 = 1;
        break;
    case HDMITX_PATTERN_BLACK:
        MatrixCoeff = (UInt8*)&MatrixCoeffBlackScreen[pDis->voutMode][0]; //point to the black matrix 
    case HDMITX_PATTERN_CBAR8:
        serviceMode = 1;
        bars8 = 1;
        break;
    case HDMITX_PATTERN_OFF:
        serviceMode = 0;
        bars8 = 0;
        break;
    default:
        return TMBSL_ERR_HDMI_BAD_PARAMETER;
    }

    if (serviceMode)
    {
        if (!pDis->prevPattern) /* if a pattern is on, registers are already saved */
        {
            /* The kBypassColourProc registers are saved in tmbslTDA9989VideoSetInOut API */
            /* Bypass up/down sampler and matrix for RGB colourbars */
            setHwRegisterFieldTable(pDis, &kBypassColourProc[0]);
        }
        if (( pattern == HDMITX_PATTERN_BLUE )||( pattern == HDMITX_PATTERN_BLACK )) /* blue or black screen pattern */
        {

            /* To create blue or black screen, we use the internal color bar 8 on which we apply a matrix to change it to blue or black */
            /* Set the first block byte separately, as it is shadowed and can't
            * be set by setHwRegisters */

            /* Set the first block byte separately, as it is shadowed and can't
            * be set by setHwRegisters */
            err = setHwRegister(pDis,                    
                                E_REG_P00_MAT_CONTRL_W,
                                MatrixCoeff[0]);
            RETIF_REG_FAIL(err)

            for (i = 0; i < MATRIX_PRESET_SIZE; i++)
            {
                buf[i] = MatrixCoeff[i];
            }
            
            /* Set the rest of the block */
            err = setHwRegisters(pDis,
                                E_REG_P00_MAT_OI1_MSB_W,
                                &buf[1],
                                MATRIX_PRESET_SIZE - 1);
            RETIF_REG_FAIL(err)
            pDis->prevFilterPattern = True;
        }
        else /* colour bars patterns */
        {
            /* Set number of colour bars */
            err = setHwRegisterField(pDis, 
                                    E_REG_P00_HVF_CNTRL_0_W,
                                    E_MASKREG_P00_HVF_CNTRL_0_rwb,
                                    bars8);
            RETIF_REG_FAIL(err)
            
            /* Bypass up/down sampler and matrix for RGB colourbars */
            setHwRegisterFieldTable(pDis, &kBypassColourProc[0]);
        }
        pDis->prevPattern = True;
    }
    else /* serviceMode == 0 */
    {
        if (pDis->prevFilterPattern)
        {
            /* Restore the previous Matrix when pattern goes off */
            err = tmbslTDA9989MatrixSetConversion ( txUnit, pDis->vinFmt, pDis->vinMode, pDis->voutFmt, pDis->voutMode,pDis->dviVqr);
            RETIF_REG_FAIL(err)

            pDis->prevFilterPattern = False;
        }
        /* Restore kBypassColourProc registers when pattern goes off */
        setHwRegister(pDis, E_REG_P00_MAT_CONTRL_W,  gMatContrl[txUnit]);
        setHwRegister(pDis, E_REG_P00_HVF_CNTRL_0_W, gHvfCntrl0[txUnit]);
        setHwRegister(pDis, E_REG_P00_HVF_CNTRL_1_W, gHvfCntrl1[txUnit]);
        pDis->prevPattern = False;
    }

    /* Set Service Mode on or off */
    err = setHwRegisterField(pDis, 
                             E_REG_P00_HVF_CNTRL_0_W,
                             E_MASKREG_P00_HVF_CNTRL_0_sm,
                             serviceMode);
#ifdef TMFL_HDCP_SUPPORT
   pDis->HDCPIgnoreEncrypt = True;  /* Skip the next encrypt IT */
#endif /* TMFL_HDCP_SUPPORT */

    return err;
}

/*============================================================================*/
/* tmbslTDA9989VideoInSetBlanking                                              */
/*============================================================================*/
tmErrorCode_t
tmbslTDA9989VideoInSetBlanking
(
    tmUnitSelect_t         txUnit,
    tmbslHdmiTxBlnkSrc_t   blankitSource,
    tmbslHdmiTxBlnkCode_t  blankingCodes
)
{
    tmHdmiTxobject_t *pDis;       /* Pointer to Device Instance Structure */
    tmErrorCode_t     err;        /* Error code */

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Check parameters */
    RETIF_BADPARAM(blankitSource >= HDMITX_BLNKSRC_INVALID)
    RETIF_BADPARAM(blankingCodes >= HDMITX_BLNKCODE_INVALID)

    /* For each parameter that is not No Change, set its register */
    if (blankitSource != HDMITX_BLNKSRC_NO_CHANGE)
    {
        err = setHwRegisterField(pDis,
                                 E_REG_P00_VIP_CNTRL_4_W,
                                 E_MASKREG_P00_VIP_CNTRL_4_blankit,
                                 (UInt8)blankitSource);
        RETIF_REG_FAIL(err)
    }
    if (blankingCodes != HDMITX_BLNKCODE_NO_CHANGE)
    {
        err = setHwRegisterField(pDis,
                                 E_REG_P00_VIP_CNTRL_4_W,
                                 E_MASKREG_P00_VIP_CNTRL_4_blc,
                                 (UInt8)blankingCodes);
        RETIF_REG_FAIL(err)
    }

    return TM_OK;
}

/*============================================================================*/
/* tmbslTDA9989VideoInSetConfig                                                */
/*============================================================================*/
tmErrorCode_t
tmbslTDA9989VideoInSetConfig
(
    tmUnitSelect_t             txUnit,
    tmbslHdmiTxVinMode_t       vinMode,
    tmbslHdmiTxVidFmt_t        voutFmt,
    tmbslHdmiTx3DStructure_t   structure3D,
    tmbslHdmiTxPixEdge_t       sampleEdge,
    tmbslHdmiTxPixRate_t       pixRate,
    tmbslHdmiTxUpsampleMode_t  upsampleMode 
)
{
    tmHdmiTxobject_t *pDis;       /* Pointer to Device Instance Structure */
    tmErrorCode_t     err;        /* Error code */

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Check parameters */
    RETIF_BADPARAM(vinMode      >= HDMITX_VINMODE_INVALID)
    RETIF_BADPARAM(sampleEdge   >= HDMITX_PIXEDGE_INVALID)
    RETIF_BADPARAM(pixRate      >= HDMITX_PIXRATE_INVALID)
    RETIF_BADPARAM(upsampleMode >= HDMITX_UPSAMPLE_INVALID)

    err = InputConfig(pDis,
                      vinMode,
                      sampleEdge,
                      pixRate,
                      upsampleMode, 
                      HDMITX_PIXREP_NO_CHANGE,
                      voutFmt,
                      structure3D);
    RETIF_REG_FAIL(err)

    return TM_OK;
}
/*============================================================================*/
/* tmbslTDA9989VideoInSetFine                                                  */
/*============================================================================*/
tmErrorCode_t
tmbslTDA9989VideoInSetFine
(
    tmUnitSelect_t            txUnit,
    tmbslHdmiTxPixSubpkt_t    subpacketCount,
    tmbslHdmiTxPixTogl_t      toggleClk1 
)
{
    tmHdmiTxobject_t *pDis;       /* Pointer to Device Instance Structure */
    tmErrorCode_t     err;        /* Error code */

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Check parameters */
    RETIF_BADPARAM(subpacketCount >= HDMITX_PIXSUBPKT_INVALID)
    RETIF_BADPARAM(toggleClk1     >= HDMITX_PIXTOGL_INVALID)

    /* IF subpacketCount is Fix at 0/1/2/3 THEN set subpacket count register
     * to 0/1/2/3 and set subpacket sync register to 3
     */
    if (subpacketCount <= HDMITX_PIXSUBPKT_FIX_3)
    {
        err = setHwRegisterField(pDis,
                                 E_REG_P00_VIP_CNTRL_5_W,
                                 E_MASKREG_P00_VIP_CNTRL_5_sp_cnt,
                                 (UInt8)subpacketCount);
        RETIF_REG_FAIL(err)
        err = setHwRegisterField(pDis,
                                 E_REG_P00_VIP_CNTRL_3_W,
                                 E_MASKREG_P00_VIP_CNTRL_3_sp_sync,
                                 HDMITX_PIXSUBPKT_SYNC_FIXED);
        RETIF_REG_FAIL(err)
    }
    /* ELSE IF subpacketCount is Sync by Hemb/ Sync by Rising Edge DE/ 
     * Sync by Rising Edge HS THEN set the unused subpacket count to zero and
     * set subpacket sync register to 0/1/2
     */
    else if (subpacketCount != HDMITX_PIXSUBPKT_NO_CHANGE)
    {
        err = setHwRegisterField(pDis,
                                 E_REG_P00_VIP_CNTRL_5_W,
                                 E_MASKREG_P00_VIP_CNTRL_5_sp_cnt,
                                 HDMITX_PIXSUBPKT_FIX_0);
        RETIF_REG_FAIL(err)
       
        err = setHwRegisterField(pDis,
                                 E_REG_P00_VIP_CNTRL_3_W,
                                 E_MASKREG_P00_VIP_CNTRL_3_sp_sync,
                         (UInt8)(subpacketCount - HDMITX_PIXSUBPKT_SYNC_FIRST));
        RETIF_REG_FAIL(err)
    }

    /* IF toggleClk1 is not No Change THEN set ckcase bitfield */
    if (toggleClk1 != HDMITX_PIXTOGL_NO_CHANGE)
    {
        err = setHwRegisterField(pDis,
                                 E_REG_P00_VIP_CNTRL_5_W,
                                 E_MASKREG_P00_VIP_CNTRL_5_ckcase,
                                 (UInt8)toggleClk1);
        RETIF_REG_FAIL(err)
    }
    return TM_OK;
}

/*============================================================================*/
/* tmbslTDA9989VideoInSetMapping                                               */
/*============================================================================*/
tmErrorCode_t
tmbslTDA9989VideoInSetMapping
#ifdef TMFL_RGB_DDR_12BITS
(
    tmUnitSelect_t  txUnit,
    UInt8           *pSwapTable,
    UInt8           *pMirrorTable,
    UInt8           *pMux
)
#else
(
    tmUnitSelect_t  txUnit,
    UInt8           *pSwapTable,
    UInt8           *pMirrorTable
)
#endif
{
    tmHdmiTxobject_t *pDis;       /* Pointer to Device Instance Structure */
    tmErrorCode_t     err;        /* Error code */
    Int               i;          /* Loop counter */

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Check parameters */
    RETIF_BADPARAM(pSwapTable == Null)
    RETIF_BADPARAM(pMirrorTable == Null)
    for (i = 0; i < HDMITX_VIN_PORT_MAP_TABLE_LEN; i++)
    {
        RETIF_BADPARAM(pSwapTable[i] >= HDMITX_VIN_PORT_SWAP_INVALID)
        RETIF_BADPARAM(pMirrorTable[i] >= HDMITX_VIN_PORT_MIRROR_INVALID)
    }

    /* IF pswapTable[n] is not No Change THEN set the port swap registers from
     * pswapTable[n]
     */
    for (i = 0; i < HDMITX_VIN_PORT_MAP_TABLE_LEN; i++)
    {
        if (pSwapTable[0] < HDMITX_VIN_PORT_SWAP_NO_CHANGE)
        {
            err = setHwRegisterField(pDis,
                                     kRegVip[i].Register,
                                     kRegVip[i].MaskSwap,
                                     pSwapTable[i]);
            RETIF_REG_FAIL(err)
        }
    }

    /* IF pMirrorTable[n] is not No Change THEN set the port mirror registers
     * from pMirrorTable[n]
     */
    for (i = 0; i < HDMITX_VIN_PORT_MAP_TABLE_LEN; i++)
    {
        if (pMirrorTable[0] < HDMITX_VIN_PORT_MIRROR_NO_CHANGE)
        {
            err = setHwRegisterField(pDis,
                                     kRegVip[i].Register,
                                     kRegVip[i].MaskMirror,
                                     pMirrorTable[i]);
            RETIF_REG_FAIL(err)
        }
    }

#ifdef TMFL_RGB_DDR_12BITS
    /*
      mux for RGB_DDR_12bits
    */
    err = setHwRegister(pDis,E_REG_P00_MUX_VP_VIP_OUT_RW, *pMux);
    RETIF_REG_FAIL(err);
#endif
    
    return TM_OK;
}

/*============================================================================*/
/* tmbslTDA9989SetVideoPortConfig                                              */
/*============================================================================*/
tmErrorCode_t
tmbslTDA9989SetVideoPortConfig
(
    tmUnitSelect_t  txUnit,
    UInt8           *pEnaVideoPortTable,
    UInt8           *pGndVideoPortTable
)
{
    tmHdmiTxobject_t *pDis;       /* Pointer to Device Instance Structure */
    tmErrorCode_t     err;        /* Error code */

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Check parameters */
    RETIF_BADPARAM(pEnaVideoPortTable == Null)
    RETIF_BADPARAM(pGndVideoPortTable == Null)

    err = setHwRegister(pDis,
                        E_REG_P00_ENA_VP_0_RW,
                        pEnaVideoPortTable[0]);
    RETIF_REG_FAIL(err)

    err = setHwRegister(pDis,
                        E_REG_P00_ENA_VP_1_RW,
                        pEnaVideoPortTable[1]);
    RETIF_REG_FAIL(err)

    err = setHwRegister(pDis,
                        E_REG_P00_ENA_VP_2_RW,
                        pEnaVideoPortTable[2]);
    RETIF_REG_FAIL(err)

 /*   err = setHwRegister(pDis,
                        E_REG_P00_GND_VP_0_RW,
                        pGndVideoPortTable[0]);
    RETIF_REG_FAIL(err)*/

 /*  err = setHwRegister(pDis,
                        E_REG_P00_GND_VP_1_RW,
                        pGndVideoPortTable[1]);
    RETIF_REG_FAIL(err)*/

 /* err = setHwRegister(pDis,
                        E_REG_P00_GND_VP_2_RW,
                        pGndVideoPortTable[2]);
    RETIF_REG_FAIL(err)*/

    return TM_OK;
}

/*============================================================================*/
/* tmbslTDA9989SetAudioPortConfig                                              */
/*============================================================================*/
tmErrorCode_t
tmbslTDA9989SetAudioPortConfig
(
    tmUnitSelect_t  txUnit,
    UInt8           *pEnaAudioPortTable,
    UInt8           *pGndAudioPortTable
)
{
    tmHdmiTxobject_t *pDis;       /* Pointer to Device Instance Structure */
    tmErrorCode_t     err;        /* Error code */

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Check parameters */
    RETIF_BADPARAM(pEnaAudioPortTable == Null)
    RETIF_BADPARAM(pGndAudioPortTable == Null)

    err = setHwRegister(pDis,
                        E_REG_P00_ENA_AP_RW,
                        pEnaAudioPortTable[0]);
    RETIF_REG_FAIL(err)

 /*   err = setHwRegister(pDis,
                        E_REG_P00_GND_AP_RW,
                        pGndAudioPortTable[0]);
    RETIF_REG_FAIL(err)*/

    return TM_OK;
}

/*============================================================================*/
/* tmbslTDA9989SetAudioClockPortConfig                                              */
/*============================================================================*/
tmErrorCode_t
tmbslTDA9989SetAudioClockPortConfig
(
    tmUnitSelect_t  txUnit,
    UInt8           *pEnaAudioClockPortTable,
    UInt8           *pGndAudioClockPortTable
)
{
    tmHdmiTxobject_t *pDis;       /* Pointer to Device Instance Structure */
    tmErrorCode_t     err;        /* Error code */

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Check parameters */
    RETIF_BADPARAM(pEnaAudioClockPortTable == Null)
    RETIF_BADPARAM(pGndAudioClockPortTable == Null)

    err = setHwRegister(pDis,
                        E_REG_P00_ENA_ACLK_RW,
                        pEnaAudioClockPortTable[0]);
    RETIF_REG_FAIL(err)

    /*err = setHwRegister(pDis,
                        E_REG_P00_GND_ACLK_RW,
                        pGndAudioClockPortTable[0]);
    RETIF_REG_FAIL(err)*/

    return TM_OK;
}

/*============================================================================*/
/* set_video replace E_REG_P00_VIDFORMAT_W register                           */
/* use it for new video format                                                */
/*============================================================================*/
tmErrorCode_t set_video(tmHdmiTxobject_t *pDis,tmbslHdmiTxVidFmt_t reg_idx,tmHdmiTxVidReg_t *format_param)
{
   tmErrorCode_t err;
   UInt8 regVal;

   regVal = 0x00;/* PR1570 FIXED */
   err = setHwRegister(pDis, E_REG_P00_VIDFORMAT_W, regVal);
   RETIF_REG_FAIL(err);

   regVal = (UInt8)format_param[reg_idx].nPix;
   err = setHwRegister(pDis, E_REG_P00_NPIX_LSB_W, regVal);
   RETIF_REG_FAIL(err);
   
   regVal = (UInt8)(format_param[reg_idx].nPix>>8);
   err = setHwRegister(pDis, E_REG_P00_NPIX_MSB_W, regVal);
   RETIF_REG_FAIL(err);
   
   regVal = (UInt8)format_param[reg_idx].nLine;
   err = setHwRegister(pDis, E_REG_P00_NLINE_LSB_W, regVal);
   RETIF_REG_FAIL(err);
   
   regVal = (UInt8)(format_param[reg_idx].nLine>>8);
   err = setHwRegister(pDis, E_REG_P00_NLINE_MSB_W, regVal);
   RETIF_REG_FAIL(err);
   
   regVal = (UInt8)format_param[reg_idx].VsLineStart;
   err = setHwRegister(pDis, E_REG_P00_VS_LINE_STRT_1_LSB_W, regVal);
   RETIF_REG_FAIL(err);
   
   regVal = (UInt8)format_param[reg_idx].VsPixStart;
   err = setHwRegister(pDis, E_REG_P00_VS_PIX_STRT_1_LSB_W, regVal);
   RETIF_REG_FAIL(err);
   
   regVal = (UInt8)(format_param[reg_idx].VsPixStart>>8);
   err = setHwRegister(pDis, E_REG_P00_VS_PIX_STRT_1_MSB_W, regVal);
   RETIF_REG_FAIL(err);
   
   regVal = (UInt8)format_param[reg_idx].VsLineEnd;
   err = setHwRegister(pDis, E_REG_P00_VS_LINE_END_1_LSB_W, regVal);
   RETIF_REG_FAIL(err);
   
   regVal = (UInt8)format_param[reg_idx].VsPixEnd;
   err = setHwRegister(pDis, E_REG_P00_VS_PIX_END_1_LSB_W, regVal);
   RETIF_REG_FAIL(err);
   
   regVal = (UInt8)(format_param[reg_idx].VsPixEnd>>8);
   err = setHwRegister(pDis, E_REG_P00_VS_PIX_END_1_MSB_W, regVal);
   RETIF_REG_FAIL(err);
   
   regVal = (UInt8)format_param[reg_idx].HsStart;
   err = setHwRegister(pDis, E_REG_P00_HS_PIX_START_LSB_W, regVal);
   RETIF_REG_FAIL(err);
   
   regVal = (UInt8)(format_param[reg_idx].HsStart>>8);
   err = setHwRegister(pDis, E_REG_P00_HS_PIX_START_MSB_W, regVal);
   RETIF_REG_FAIL(err);
   
   regVal = (UInt8)format_param[reg_idx].HsEnd;
   err = setHwRegister(pDis, E_REG_P00_HS_PIX_STOP_LSB_W, regVal);
   RETIF_REG_FAIL(err);
   
   regVal = (UInt8)(format_param[reg_idx].HsEnd>>8);
   err = setHwRegister(pDis, E_REG_P00_HS_PIX_STOP_MSB_W, regVal);
   RETIF_REG_FAIL(err);
   
   regVal = (UInt8)format_param[reg_idx].ActiveVideoStart;
   err = setHwRegister(pDis, E_REG_P00_VWIN_START_1_LSB_W, regVal);
   RETIF_REG_FAIL(err);
   err = setHwRegister(pDis, E_REG_P00_VWIN_START_1_MSB_W, 0);
   RETIF_REG_FAIL(err);
   
   regVal = (UInt8)format_param[reg_idx].ActiveVideoEnd;
   err = setHwRegister(pDis, E_REG_P00_VWIN_END_1_LSB_W, regVal);
   RETIF_REG_FAIL(err);
   
   regVal = (UInt8)(format_param[reg_idx].ActiveVideoEnd>>8);
   err = setHwRegister(pDis, E_REG_P00_VWIN_END_1_MSB_W, regVal);
   RETIF_REG_FAIL(err);
   
   regVal = (UInt8)format_param[reg_idx].DeStart;
   err = setHwRegister(pDis, E_REG_P00_DE_START_LSB_W, regVal);
   RETIF_REG_FAIL(err);
   
   regVal = (UInt8)(format_param[reg_idx].DeStart>>8);
   err = setHwRegister(pDis, E_REG_P00_DE_START_MSB_W, regVal);
   RETIF_REG_FAIL(err);
   
   regVal = (UInt8)format_param[reg_idx].DeEnd;
   err = setHwRegister(pDis, E_REG_P00_DE_STOP_LSB_W, regVal);
   RETIF_REG_FAIL(err);
   
   regVal = (UInt8)(format_param[reg_idx].DeEnd>>8);
   err = setHwRegister(pDis, E_REG_P00_DE_STOP_MSB_W, regVal);
   RETIF_REG_FAIL(err);

#ifdef TMFL_RGB_DDR_12BITS
   if (format_param[reg_idx].ActiveSpaceStart) {
      /* enable active space */
      err = setHwRegisterField(pDis, E_REG_P00_ENABLE_SPACE_W, 0x01, 0x01);
      RETIF_REG_FAIL(err);
      
      /* set active space to black */
      err = setHwRegister(pDis, E_REG_P00_VSPACE_Y_DATA_W, 0x00);
      RETIF_REG_FAIL(err);
      
      err = setHwRegister(pDis, E_REG_P00_VSPACE_U_DATA_W, 0x80);
      RETIF_REG_FAIL(err);
      
      err = setHwRegister(pDis, E_REG_P00_VSPACE_V_DATA_W, 0x80);
      RETIF_REG_FAIL(err);
      
      /* active space definition */
      regVal = (UInt8)format_param[reg_idx].ActiveSpaceStart;
      err = setHwRegister(pDis, E_REG_P00_VSPACE_START_LSB_W, regVal);
      RETIF_REG_FAIL(err);
      
      regVal = (UInt8)((format_param[reg_idx].ActiveSpaceStart>>8) & 0x0F);
      err = setHwRegister(pDis, E_REG_P00_VSPACE_START_MSB_W, regVal);
      RETIF_REG_FAIL(err);
      
      regVal = (UInt8)format_param[reg_idx].ActiveSpaceEnd;
      err = setHwRegister(pDis, E_REG_P00_VSPACE_END_LSB_W, regVal);
      RETIF_REG_FAIL(err);
      
      regVal = (UInt8)((format_param[reg_idx].ActiveSpaceEnd>>8) & 0x0F);
      err = setHwRegister(pDis, E_REG_P00_VSPACE_END_MSB_W, regVal);
      RETIF_REG_FAIL(err);
   }
   else {
      /* let incoming pixels feel the active space (if any) */
      err = setHwRegisterField(pDis, E_REG_P00_ENABLE_SPACE_W, 0x01, 0x00);
      RETIF_REG_FAIL(err);
   }
#endif

   return TM_OK;
}


/*============================================================================*/
/* tmbslTDA9989VideoInSetSyncAuto                                              */
/*============================================================================*/
tmErrorCode_t
tmbslTDA9989VideoInSetSyncAuto
(
    tmUnitSelect_t            txUnit,
    tmbslHdmiTxSyncSource_t   syncSource,
    tmbslHdmiTxVidFmt_t       vinFmt,
    tmbslHdmiTxVinMode_t      vinMode,
    tmbslHdmiTx3DStructure_t  structure3D
)
{
    tmHdmiTxobject_t *pDis;       /* Pointer to Device Instance Structure */
    tmErrorCode_t     err;        /* Error code */
    UInt8             reg_idx,reg_idx3D;  /* Video i/p fmt value used for comparison */
    UInt8             embedded;   /* Register value */
    UInt8             syncMethod; /* Sync method */
    UInt8             toggleV;    /* V toggle */
    UInt8             toggleH;    /* H toggle */
    UInt8             toggleX;    /* X toggle */
    UInt16            uRefPix;    /* Output refpix */
    UInt16            uRefLine;   /* Output refline */
#ifdef FORMAT_PC
    UInt8             regVal;/* PR1570 FIXED */
#endif /* FORMAT_PC */
    struct sync_desc *sync;

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err);
    
    /* Check parameters - syncSource must be specified */
    RETIF_BADPARAM(syncSource >= HDMITX_SYNCSRC_NO_CHANGE);
    RETIF_BADPARAM(!IS_VALID_FMT(vinFmt));
    
    /* Look up the VIDFORMAT register format from the register format table */
    /* Quit if the input format does not map to the register format */
    RETIF_BADPARAM(reg_vid_fmt(vinFmt,structure3D,&reg_idx,&reg_idx3D,&sync));

    /* Select values according to sync source */
    embedded = 0;
    switch (syncSource)
    {
    case HDMITX_SYNCSRC_EXT_VS:
        syncMethod = 0;
        toggleV    = sync[BASE(reg_idx)].v_toggle;
        toggleH    = sync[BASE(reg_idx)].h_toggle;
        toggleX    = 0;
        uRefPix    = sync[BASE(reg_idx)].hfp;
        uRefLine   = sync[BASE(reg_idx)].vfp;
        break;
    case HDMITX_SYNCSRC_EMBEDDED:
        embedded++;
        /* fall thru */
    case HDMITX_SYNCSRC_EXT_VREF:
    default:
        syncMethod = 1;
        toggleV    = 1;
        toggleH    = 1;
        toggleX    = 1;
        uRefPix    = sync[BASE(reg_idx)].href;
        uRefLine   = sync[BASE(reg_idx)].vref;
        break;
    }
    /* Table has +1 added to refpix values which are not needed in 
       RGB444, YUV444 and YUV422 modes, but +2 is required in those cases */
    if (vinMode != HDMITX_VINMODE_CCIR656) 
    {
        uRefPix = uRefPix + 2;
    }

    /* ---------------------------------------------------------- */
    /* Synchronicity software workaround issue number 106         */
    /* ---------------------------------------------------------- */
    if (vinMode == HDMITX_VINMODE_CCIR656) {
        if (syncSource == HDMITX_SYNCSRC_EXT_VS) {
            if (pDis->pixRate == HDMITX_PIXRATE_DOUBLE) {

                switch (reg_idx) {
                case E_REGVFMT_720x480p_60Hz:
                case E_REGVFMT_720x480i_60Hz:
                case E_REGVFMT_720x576p_50Hz:
                case E_REGVFMT_720x576i_50Hz:
                    uRefPix = uRefPix + 1;
                break;
                default:
                /* do nothing... well I would say : FIXME */
                break;
                }

            }
        }
    }


    /* Set embedded sync */
    err = setHwRegisterField(pDis,
                             E_REG_P00_VIP_CNTRL_3_W,
                             E_MASKREG_P00_VIP_CNTRL_3_emb,
                             embedded);
    RETIF_REG_FAIL(err)

    /* Set sync method */
    err = setHwRegisterField(pDis,
                             E_REG_P00_TBG_CNTRL_0_W,
                             E_MASKREG_P00_TBG_CNTRL_0_sync_mthd,
                             syncMethod);
    RETIF_REG_FAIL(err)

/*     printk("DBG auto toggle X:%d V:%d H:%d\n",toggleX,toggleV,toggleH); */
    /* Set VH toggle */
    err = setHwRegisterField(pDis,
                             E_REG_P00_VIP_CNTRL_3_W,
                             E_MASKREG_P00_VIP_CNTRL_3_v_tgl,
                             toggleV);
    RETIF_REG_FAIL(err)
    err = setHwRegisterField(pDis,
                             E_REG_P00_VIP_CNTRL_3_W,
                             E_MASKREG_P00_VIP_CNTRL_3_h_tgl,
                             toggleH);
    RETIF_REG_FAIL(err)

    /* Set X toggle */
    err = setHwRegisterField(pDis,
                             E_REG_P00_VIP_CNTRL_3_W,
                             E_MASKREG_P00_VIP_CNTRL_3_x_tgl,
                             toggleX);
    RETIF_REG_FAIL(err);
    
#ifdef TMFL_RGB_DDR_12BITS
    if (syncSource == HDMITX_SYNCSRC_EXT_VREF) {
       if (structure3D == HDMITX_3D_FRAME_PACKING) {
          /* 
             stereo sync signaling : 
             -----------------------
             
             When there is a positive sync at the input pins, therefore a negative sync
             at input of the TBG, then 3d_neg_vs signal has to be set at 1 (OR-function)
             to create the correct VS to preset the line and pixel counters
             
             case Vs > 0:
             -----------
             Vs     : __/¨¨\__
             Vs(TBG): ¨¨\__/¨¨ where Vs(TBG)=NOT(Vs)
             3D     : ¨¨\_____ 
             Stereo : ¨¨\__/¨¨ where Stereo = Vs(TBG) OR 3D because 3d_neg_vs = 1
             
             case Vs < 0:
             -----------
             Vs     : ¨¨\__/¨¨
             Vs(TBG): __/¨¨\__ where Vs(TBG)=NOT(Vs)
             3D     : ¨¨\_____ 
             Stereo : __/¨¨\__ where Stereo = Vs(TBG) AND NOT(3D) because 3d_neg_vs = 0
             
             
             It is possible to invert the incoming VS, HS and DE. In case of 3D format
             the DE input will be the 3D signal. This signal will only be used to remove
             1 of the VS depending on the polarity of the 3D signal. When there is a need
             to switch the Left or Right it is possible to invert the 3D signal with an
             already existing register.
             
          */
          
          err = setHwRegisterField(pDis,
                                   E_REG_P00_VIDFORMAT_W,
                                   E_MASKREG_P00_VIDFORMAT_3d_neg_vs,
                                   toggleV);
          RETIF_REG_FAIL(err);
       }
       err = setHwRegisterField(pDis,
                                E_REG_P00_VIDFORMAT_W,
                                E_MASKREG_P00_VIDFORMAT_3d,
                                (structure3D == HDMITX_3D_FRAME_PACKING));
       RETIF_REG_FAIL(err);
    }
#endif


    if (EXTRA(reg_idx) && (structure3D != HDMITX_3D_FRAME_PACKING)) {
       /* 2d extra video format */
       RETIF_REG_FAIL(set_video(pDis,BASE(reg_idx),(tmHdmiTxVidReg_t *)format_param_extra));
    }
    else if (EXTRA(reg_idx3D) && (structure3D == HDMITX_3D_FRAME_PACKING)) {
       /* 3d extra frame packing */
       RETIF_REG_FAIL(set_video(pDis,BASE(reg_idx3D),(tmHdmiTxVidReg_t *)format_param_extra));
    }
    else {
       /* see video set up using E_REG_P00_VIDFORMAT_W */ 
    }
                   
                   
#ifdef FORMAT_PC
                   
    if (IS_PC(vinFmt))
    {
       RETIF_REG_FAIL(set_video(pDis,reg_idx,(tmHdmiTxVidReg_t *)format_param_PC));

       regVal = DEPTH_COLOR_PC;
       err = setHwRegisterField(pDis,
                                E_REG_P00_HVF_CNTRL_1_W,
                                E_MASKREG_P00_HVF_CNTRL_1_pad,
                                regVal);
       RETIF_REG_FAIL(err);
    }
#endif /* FORMAT_PC */

    /* Set refpix, refline */
    err = setHwRegisterMsbLsb(pDis, E_REG_P00_REFPIX_MSB_W, uRefPix);
    RETIF_REG_FAIL(err)
    err = setHwRegisterMsbLsb(pDis, E_REG_P00_REFLINE_MSB_W, uRefLine);
    return err;
}

/*============================================================================*/
/* tmbslTDA9989VideoInSetSyncManual                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA9989VideoInSetSyncManual
(
    tmUnitSelect_t            txUnit,
    tmbslHdmiTxSyncSource_t   syncSource,
    tmbslHdmiTxVsMeth_t       syncMethod,
    tmbslHdmiTxPixTogl_t      toggleV,
    tmbslHdmiTxPixTogl_t      toggleH,
    tmbslHdmiTxPixTogl_t      toggleX,
    UInt16                    uRefPix,    
    UInt16                    uRefLine   
)
{
    tmHdmiTxobject_t *pDis;       /* Pointer to Device Instance Structure */
    tmErrorCode_t     err;        /* Error code */
    UInt8             embedded;   /* Register value */

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Check parameters */
    RETIF_BADPARAM(syncSource   >= HDMITX_SYNCSRC_INVALID)
    RETIF_BADPARAM(syncMethod   >= HDMITX_VSMETH_INVALID)
    RETIF_BADPARAM(toggleV      >= HDMITX_PIXTOGL_INVALID)
    RETIF_BADPARAM(toggleH      >= HDMITX_PIXTOGL_INVALID)
    RETIF_BADPARAM(toggleX      >= HDMITX_PIXTOGL_INVALID)
    RETIF_BADPARAM(uRefPix  >= HDMITX_VOUT_FINE_PIXEL_INVALID)
    RETIF_BADPARAM(uRefLine >= HDMITX_VOUT_FINE_LINE_INVALID)

    if (syncSource != HDMITX_SYNCSRC_NO_CHANGE)
    {
        if (syncSource == HDMITX_SYNCSRC_EMBEDDED)
        {
            embedded = 1;
        }
        else
        {
            embedded = 0;
        }
        err = setHwRegisterField(pDis,
                                 E_REG_P00_VIP_CNTRL_3_W,
                                 E_MASKREG_P00_VIP_CNTRL_3_emb,
                                 embedded);
        RETIF_REG_FAIL(err)
    }
    if (syncMethod != HDMITX_VSMETH_NO_CHANGE)
    {
        err = setHwRegisterField(pDis,
                                 E_REG_P00_TBG_CNTRL_0_W,
                                 E_MASKREG_P00_TBG_CNTRL_0_sync_mthd,
                                 (UInt8)syncMethod);
        RETIF_REG_FAIL(err)
    }
/*     printk("DBG manual toggle X:%d V:%d H:%d\n",toggleX,toggleV,toggleH); */
    if (toggleV != HDMITX_PIXTOGL_NO_CHANGE)
    {
        err = setHwRegisterField(pDis,
                                 E_REG_P00_VIP_CNTRL_3_W,
                                 E_MASKREG_P00_VIP_CNTRL_3_v_tgl,
                                 (UInt8)toggleV);
        RETIF_REG_FAIL(err)
    }
    if (toggleH != HDMITX_PIXTOGL_NO_CHANGE)
    {
        err = setHwRegisterField(pDis,
                                 E_REG_P00_VIP_CNTRL_3_W,
                                 E_MASKREG_P00_VIP_CNTRL_3_h_tgl,
                                 (UInt8)toggleH);
        RETIF_REG_FAIL(err)
    }
    if (toggleX != HDMITX_PIXTOGL_NO_CHANGE)
    {
        err = setHwRegisterField(pDis,
                                 E_REG_P00_VIP_CNTRL_3_W,
                                 E_MASKREG_P00_VIP_CNTRL_3_x_tgl,
                                 (UInt8)toggleX);
        RETIF_REG_FAIL(err)
    }
    if (uRefPix < HDMITX_VOUT_FINE_PIXEL_NO_CHANGE)
    {
        err = setHwRegisterMsbLsb(pDis, E_REG_P00_REFPIX_MSB_W, uRefPix);
        RETIF_REG_FAIL(err)
    }
    if (uRefLine < HDMITX_VOUT_FINE_LINE_NO_CHANGE)
    {
        err = setHwRegisterMsbLsb(pDis, E_REG_P00_REFLINE_MSB_W, uRefLine);
        RETIF_REG_FAIL(err)
    }

    return TM_OK;
}

/*============================================================================*/
/* tmbslTDA9989VideoOutDisable                                                 */
/*============================================================================*/

tmErrorCode_t
tmbslTDA9989VideoOutDisable
(
    tmUnitSelect_t  txUnit,
    Bool            bDisable
)
{
    tmHdmiTxobject_t *pDis;       /* Pointer to Device Instance Structure */
    tmErrorCode_t     err;        /* Error code */

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Check parameters */
    RETIF_BADPARAM(bDisable > True)

    /* Set or clear frame_dis in the scaler Timebase Control 0 register 
     * according to bDisable
     */
    err = setHwRegisterField(pDis,
                             E_REG_P00_TBG_CNTRL_0_W,
                             E_MASKREG_P00_TBG_CNTRL_0_frame_dis,
                             (UInt8)bDisable);
    if (bDisable)
    {
        setState(pDis, EV_OUTDISABLE);
    }
    return err;
}


/*============================================================================*/
/* tmbslTDA9989VideoOutSetConfig                                               */
/*============================================================================*/
tmErrorCode_t
tmbslTDA9989VideoOutSetConfig
(
    tmUnitSelect_t            txUnit,
    tmbslHdmiTxSinkType_t     sinkType,
    tmbslHdmiTxVoutMode_t     voutMode,
    tmbslHdmiTxVoutPrefil_t   preFilter,
    tmbslHdmiTxVoutYuvBlnk_t  yuvBlank,
    tmbslHdmiTxVoutQrange_t   quantization 
)
{
    tmHdmiTxobject_t *pDis;       /* Pointer to Device Instance Structure */
    tmErrorCode_t     err;        /* Error code */
    UInt8             regVal;     /* Register value */

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Check parameters */
    RETIF_BADPARAM(sinkType     >= HDMITX_SINK_INVALID)
    RETIF_BADPARAM(voutMode     >= HDMITX_VOUTMODE_INVALID)
    RETIF_BADPARAM(preFilter    >= HDMITX_VOUT_PREFIL_INVALID)
    RETIF_BADPARAM(yuvBlank     >= HDMITX_VOUT_YUV_BLNK_INVALID)
    RETIF_BADPARAM(quantization >= HDMITX_VOUT_QRANGE_INVALID)

    if (sinkType == HDMITX_SINK_EDID)
    {
        if (pDis->EdidStatus == HDMITX_EDID_NOT_READ)
        {
            /* EDID has not been read so assume simplest sink */
            pDis->sinkType = HDMITX_SINK_DVI;
        }
        else
        {
            /* EDID has been read so set sink to the type that was read */
            pDis->sinkType = pDis->EdidSinkType;
        }
    }
    else
    {
        /* Set demanded sink type */
        pDis->sinkType = sinkType;
    }

    /* Is DVI sink required? */
    if (pDis->sinkType == HDMITX_SINK_DVI)
    {
        /* Mute the audio FIFO */
        err = setHwRegisterField(pDis,
                                 E_REG_P11_AIP_CNTRL_0_RW,
                                 E_MASKREG_P11_AIP_CNTRL_0_rst_fifo,
                                 1);
        RETIF_REG_FAIL(err)

        /* Force RGB mode for DVI sink */
        voutMode = HDMITX_VOUTMODE_RGB444;

        /* Set HDMI HDCP mode off for DVI */
        err = setHwRegisterFieldTable(pDis, &kVoutHdcpOff[0]);
        RETIF_REG_FAIL(err);

        HDCP_F1;

        err = setHwRegisterField(pDis,
                                 E_REG_P11_ENC_CNTRL_RW,
                                 E_MASKREG_P11_ENC_CNTRL_ctl_code,
                                 regVal);
        RETIF_REG_FAIL(err)
    }
    else
    {
        /* Unmute the audio FIFO */
        err = setHwRegisterField(pDis,
                                 E_REG_P11_AIP_CNTRL_0_RW,
                                 E_MASKREG_P11_AIP_CNTRL_0_rst_fifo,
                                 0);
        RETIF_REG_FAIL(err)

        /* Set HDMI HDCP mode on for HDMI */
        /* Also sets E_MASKREG_P11_ENC_CNTRL_ctl_code */
        err = setHwRegisterFieldTable(pDis, &kVoutHdcpOn[0]);
        RETIF_REG_FAIL(err)
    }

    /* For each parameter that is not No Change, set its register */
    if (voutMode != HDMITX_VOUTMODE_NO_CHANGE)
    {
        /* Save the output mode for later use by the matrix & downsampler */
        pDis->voutMode = voutMode;
    }
    if (preFilter < HDMITX_VOUT_PREFIL_NO_CHANGE)
    {
#ifdef TMFL_HDCP_OPTIMIZED_POWER
       /* 
          power management :
          freeze/wakeup SPDIF clock
       */
       err = setHwRegisterField(pDis, E_REG_FEAT_POWER_DOWN,            \
                                E_MASKREG_FEAT_POWER_DOWN_prefilt,      \
                                (preFilter == HDMITX_VOUT_PREFIL_OFF));
       RETIF_REG_FAIL(err);
#endif
        err = setHwRegisterField(pDis, 
                                 E_REG_P00_HVF_CNTRL_0_W, 
                                 E_MASKREG_P00_HVF_CNTRL_0_prefil,
                                 (UInt8)preFilter);
        RETIF_REG_FAIL(err)
    }
    if (yuvBlank < HDMITX_VOUT_YUV_BLNK_NO_CHANGE)
    {
        err = setHwRegisterField(pDis, 
                                 E_REG_P00_HVF_CNTRL_1_W, 
                                 E_MASKREG_P00_HVF_CNTRL_1_yuvblk,
                                 (UInt8)yuvBlank);
        RETIF_REG_FAIL(err)
    }
    return TM_OK;
}

/*============================================================================*/
/* tmbslTDA9989VideoOutSetSync                                                 */
/*============================================================================*/
tmErrorCode_t
tmbslTDA9989VideoOutSetSync
(
    tmUnitSelect_t          txUnit,
    tmbslHdmiTxVsSrc_t      srcH,
    tmbslHdmiTxVsSrc_t      srcV,
    tmbslHdmiTxVsSrc_t      srcX,
    tmbslHdmiTxVsTgl_t      toggle,
    tmbslHdmiTxVsOnce_t     once
)
{
    tmHdmiTxobject_t *pDis;       /* Pointer to Device Instance Structure */
    tmErrorCode_t     err;        /* Error code */
    UInt8 reg_idx;
    struct sync_desc *sync;

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Check parameters */
    RETIF_BADPARAM(srcH   >= HDMITX_VSSRC_INVALID)
    RETIF_BADPARAM(srcV   >= HDMITX_VSSRC_INVALID)
    RETIF_BADPARAM(srcX   >= HDMITX_VSSRC_INVALID)
    RETIF_BADPARAM(toggle >= HDMITX_VSTGL_INVALID)
    RETIF_BADPARAM(once   >= HDMITX_VSONCE_INVALID)

    /* For each parameter that is not No Change, set its register */
    if (srcH != HDMITX_VSSRC_NO_CHANGE)
    {
        err = setHwRegisterField(pDis,
                                 E_REG_P00_TBG_CNTRL_1_W,
                                 E_MASKREG_P00_TBG_CNTRL_1_vhx_ext_hs,
                                 (UInt8)srcH);
        RETIF_REG_FAIL(err)
    }
    if (srcV != HDMITX_VSSRC_NO_CHANGE)
    {
        err = setHwRegisterField(pDis,
                                 E_REG_P00_TBG_CNTRL_1_W,
                                 E_MASKREG_P00_TBG_CNTRL_1_vhx_ext_vs,
                                 (UInt8)srcV);
        RETIF_REG_FAIL(err)
    }
    if (srcX != HDMITX_VSSRC_NO_CHANGE)
    {
        err = setHwRegisterField(pDis,
                                 E_REG_P00_TBG_CNTRL_1_W,
                                 E_MASKREG_P00_TBG_CNTRL_1_vhx_ext_de,
                                 (UInt8)srcX);
        RETIF_REG_FAIL(err)
    }
    {
       /* Hs Vs polarity fix */
       /* set polarity back when VIDFORMAT_TABLE (E_REG_P00_VIDFORMAT_W) is not used */
       RETIF_BADPARAM(reg_vid_fmt(pDis->vinFmt,0,&reg_idx,0,&sync));
       if (EXTRA(reg_idx)) {
          toggle= E_MASKREG_P00_TBG_CNTRL_1_vh_tgl &                    \
             (0x04 | sync[BASE(reg_idx)].v_toggle | sync[BASE(reg_idx)].h_toggle);
       }
    }


    if (toggle != HDMITX_VSTGL_NO_CHANGE)
    {
        err = setHwRegisterField(pDis,
                                 E_REG_P00_TBG_CNTRL_1_W,
                                 E_MASKREG_P00_TBG_CNTRL_1_vh_tgl,
                                 (UInt8)toggle);
/*         printk("DBG toogl CNTRL1:%d\n",toggle); */
        RETIF_REG_FAIL(err)
    }
    if (once != HDMITX_VSONCE_NO_CHANGE)
    {
        /* Must be last register set */
        err = setHwRegisterField(pDis,
                                 E_REG_P00_TBG_CNTRL_0_W,
                                 E_MASKREG_P00_TBG_CNTRL_0_sync_once,
                                 (UInt8)once);
        RETIF_REG_FAIL(err)
    }

    /* Toggle TMDS serialiser force flags - stability fix */
    err = setHwRegisterField(pDis,
                             E_REG_P02_BUFFER_OUT_RW,
                             E_MASKREG_P02_BUFFER_OUT_srl_force,
                             (UInt8)HDMITX_TMDSOUT_FORCED0);
    RETIF_REG_FAIL(err)
    err = setHwRegisterField(pDis,
                             E_REG_P02_BUFFER_OUT_RW,
                             E_MASKREG_P02_BUFFER_OUT_srl_force,
                             (UInt8)HDMITX_TMDSOUT_NORMAL);
    RETIF_REG_FAIL(err)


    if (once == HDMITX_VSONCE_ONCE)
    {
        /* Toggle output Sync Once flag for settings to take effect */
        err = setHwRegisterField(pDis,
                                 E_REG_P00_TBG_CNTRL_0_W,
                                 E_MASKREG_P00_TBG_CNTRL_0_sync_once,
                                 (UInt8)HDMITX_VSONCE_EACH_FRAME);
        RETIF_REG_FAIL(err)
        err = setHwRegisterField(pDis,
                                 E_REG_P00_TBG_CNTRL_0_W,
                                 E_MASKREG_P00_TBG_CNTRL_0_sync_once,
                                 (UInt8)HDMITX_VSONCE_ONCE);
        RETIF_REG_FAIL(err)
    }
    return TM_OK;
}

/*============================================================================*/
/* tmbslTDA9989VideoSetInOut                                                   */
/*============================================================================*/
tmErrorCode_t
tmbslTDA9989VideoSetInOut
(
    tmUnitSelect_t          txUnit,
    tmbslHdmiTxVidFmt_t     vinFmt,
    tmbslHdmiTx3DStructure_t structure3D,
    tmbslHdmiTxScaMode_t    scaModeRequest,
    tmbslHdmiTxVidFmt_t     voutFmt,
    UInt8                   uPixelRepeat,
    tmbslHdmiTxMatMode_t    matMode,
    tmbslHdmiTxVoutDbits_t  datapathBits,
    tmbslHdmiTxVQR_t        dviVqr
)
{
    tmHdmiTxobject_t        *pDis;       /* Pointer to Device Instance Structure */
    tmErrorCode_t           err;         /* Error code */
    tmbslHdmiTxScaMode_t    scaMode;     /* Scaler mode */
    UInt8                   reg_idx,reg_idx3D;  /* Video o/p format value used for register */
    UInt8 regVal;
    
    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Check parameters */
    RETIF_BADPARAM(!IS_VALID_FMT(vinFmt))
    RETIF_BADPARAM(!IS_VALID_FMT(voutFmt))

    RETIF_BADPARAM(scaModeRequest >= HDMITX_SCAMODE_INVALID)
    RETIF_BADPARAM(uPixelRepeat   >= HDMITX_PIXREP_INVALID)
    RETIF_BADPARAM(matMode        >= HDMITX_MATMODE_INVALID)
    RETIF_BADPARAM(datapathBits   >= HDMITX_VOUT_DBITS_INVALID)

    scaMode = HDMITX_SCAMODE_OFF;
    pDis->scaMode = HDMITX_SCAMODE_OFF;

    /* Get current input format if it must not change */
    if (vinFmt == HDMITX_VFMT_NO_CHANGE)
    {
        RETIF(pDis->vinFmt == HDMITX_VFMT_NULL,
              TMBSL_ERR_HDMI_INCONSISTENT_PARAMS)
        vinFmt = pDis->vinFmt;
    }
    else
    {
        pDis->vinFmt = vinFmt;
        pDis->h3dFpOn = (structure3D == HDMITX_3D_FRAME_PACKING);

#ifdef TMFL_TDA9989_PIXEL_CLOCK_ON_DDC

        if (IS_TV(pDis->vinFmt)) {
           
           err = setHwRegister(pDis, E_REG_P00_TIMER_H_W, 0);
           RETIF(err != TM_OK, err);
           
           err = setHwRegister(pDis, E_REG_P00_NDIV_IM_W, kndiv_im[vinFmt]);
           RETIF(err != TM_OK, err);
           
           err = setHwRegister(pDis, E_REG_P12_TX3_RW, kclk_div[vinFmt]);
           RETIF(err != TM_OK, err);
           
        }
        else {
           
           err = setHwRegister(pDis, E_REG_P00_TIMER_H_W, E_MASKREG_P00_TIMER_H_im_clksel);
           RETIF(err != TM_OK, err);
           err = setHwRegister(pDis, E_REG_P12_TX3_RW, 17);
           RETIF(err != TM_OK, err);
        }
#endif /* TMFL_TDA9989_PIXEL_CLOCK_ON_DDC */


    }

    /* Get current output format if it must not change */
    if (voutFmt == HDMITX_VFMT_NO_CHANGE)
    {
        RETIF(pDis->voutFmt == HDMITX_VFMT_NULL,
              TMBSL_ERR_HDMI_INCONSISTENT_PARAMS)
        voutFmt = pDis->voutFmt;
    }
    else
    {
        pDis->voutFmt = voutFmt;
    }
    if (pDis->voutMode == HDMITX_VOUTMODE_RGB444)
    {
       if ((pDis->voutFmt >= HDMITX_VFMT_02_720x480p_60Hz) && (IS_TV(pDis->voutFmt)))
        {
            err = setHwRegisterField(pDis, 
                                    E_REG_P00_HVF_CNTRL_1_W, 
                                    E_MASKREG_P00_HVF_CNTRL_1_vqr,
                                    (UInt8) HDMITX_VOUT_QRANGE_RGB_YUV);
            RETIF_REG_FAIL(err)
        }
        else /*Format PC or VGA*/
        {
            err = setHwRegisterField(pDis, 
                                    E_REG_P00_HVF_CNTRL_1_W, 
                                    E_MASKREG_P00_HVF_CNTRL_1_vqr,
                                    (UInt8) HDMITX_VOUT_QRANGE_FS);
            RETIF_REG_FAIL(err)
        }
    }
    else
    {
        err = setHwRegisterField(pDis, 
                                 E_REG_P00_HVF_CNTRL_1_W, 
                                 E_MASKREG_P00_HVF_CNTRL_1_vqr,
                                 (UInt8) HDMITX_VOUT_QRANGE_YUV);
        RETIF_REG_FAIL(err);
    }

    /* Set pixel repetition - sets pixelRepeatCount, used by setScalerFormat */
    err = setPixelRepeat(pDis, voutFmt, uPixelRepeat, structure3D);
    RETIF(err != TM_OK, err);
    
    /* If scaler mode is auto then set mode based on input and output format */
    if (scaMode != HDMITX_SCAMODE_NO_CHANGE)
       {
          /* Set scaler clock */
          regVal = 0;
          if ((pDis->pixelRepeatCount > HDMITX_PIXREP_MIN) && 
              (pDis->pixelRepeatCount <= HDMITX_PIXREP_MAX))
             {
                regVal = 2;
             }
          else if (pDis->vinMode == HDMITX_VINMODE_CCIR656)
             {
                regVal = (UInt8)((pDis->scaMode == HDMITX_SCAMODE_ON) ? 0 : 1);
                
                if (pDis->pixRate == HDMITX_PIXRATE_DOUBLE)
                   {
                      regVal = 0;
                   }
             }
          
          err = setHwRegisterField(pDis,
                                   E_REG_P02_SEL_CLK_RW, 
                                   E_MASKREG_P02_SEL_CLK_sel_vrf_clk,
                                   regVal);
          RETIF_REG_FAIL(err);
          
          /* Look up the VIDFORMAT register format from the register format table */
          RETIF_BADPARAM(reg_vid_fmt(vinFmt,structure3D,&reg_idx,&reg_idx3D,0));

          /* Set format register for the selected output format voutFmt */
          if (PREFETCH(reg_idx3D) && (structure3D == HDMITX_3D_FRAME_PACKING)) {
             /* embedded 3D video format */
             err = setHwRegister(pDis, E_REG_P00_VIDFORMAT_W,reg_idx3D);
          }
          else if (PREFETCH(reg_idx) && (structure3D != HDMITX_3D_FRAME_PACKING)) {
             /* embedded 2D video format */
/*              printk("DBG %s E_REG_P00_VIDFORMAT_W used\n",__func__); */
             err = setHwRegister(pDis, E_REG_P00_VIDFORMAT_W,reg_idx);
          }
          else {
             /* see video set up using set_video() */
          }
          RETIF_REG_FAIL(err);
          
       }
    
    /* Set VS and optional DE */
    err = setDeVs(pDis, voutFmt, structure3D);
    RETIF(err != TM_OK, err);
       
    /* If matrix mode is auto then set mode based on input and output format */
    if (matMode != HDMITX_MATMODE_NO_CHANGE)
    {
        if (matMode == HDMITX_MATMODE_AUTO)
        {
            err = tmbslTDA9989MatrixSetConversion(txUnit, vinFmt,
          pDis->vinMode, voutFmt, pDis->voutMode,pDis->dviVqr);
        }
        else
        {
            err = tmbslTDA9989MatrixSetMode(txUnit, HDMITX_MCNTRL_OFF, 
                  HDMITX_MSCALE_NO_CHANGE);
        }
        RETIF(err != TM_OK, err)
    }

    /* Set upsampler and downsampler */
    err = setSampling(pDis);
    RETIF(err != TM_OK, err)

    /* Set colour component bit depth */
    if (datapathBits != HDMITX_VOUT_DBITS_NO_CHANGE)
    {
        err = setHwRegisterField(pDis, 
                                 E_REG_P00_HVF_CNTRL_1_W, 
                                 E_MASKREG_P00_HVF_CNTRL_1_pad,
                                 (UInt8)datapathBits);
        RETIF_REG_FAIL(err)
    }

    /* Save kBypassColourProc registers before pattern goes on */
    getHwRegister(pDis, E_REG_P00_MAT_CONTRL_W,  &gMatContrl[txUnit]);
    getHwRegister(pDis, E_REG_P00_HVF_CNTRL_0_W, &gHvfCntrl0[txUnit]);
    getHwRegister(pDis, E_REG_P00_HVF_CNTRL_1_W, &gHvfCntrl1[txUnit]);

    setState(pDis, EV_SETINOUT);
    return TM_OK;
}

/*============================================================================*/
/* tmbslTDA9989MatrixSetCoeffs                                                 */
/*============================================================================*/

tmErrorCode_t
tmbslTDA9989MatrixSetCoeffs
(
    tmUnitSelect_t         txUnit,
    tmbslHdmiTxMatCoeff_t *pMatCoeff
)
{
    tmHdmiTxobject_t    *pDis;  /* Pointer to Device Instance Structure */
    tmErrorCode_t       err;    /* Error code */
    Int                 i;      /* Loop index */
    UInt8               buf[HDMITX_MAT_COEFF_NUM * 2];     /* Temp buffer */

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Check remaining parameter(s) */
    RETIF_BADPARAM(pMatCoeff == (tmbslHdmiTxMatCoeff_t *)0)
    for (i = 0; i < HDMITX_MAT_COEFF_NUM; i++)
    {
        RETIF_BADPARAM((pMatCoeff->Coeff[i] < HDMITX_MAT_OFFSET_MIN) || 
              (pMatCoeff->Coeff[i] > HDMITX_MAT_OFFSET_MAX))
    }

    /* Convert signed 11 bit values from Coeff array to pairs of MSB-LSB
     * register values, and write to register pairs
     */
    for (i = 0; i < HDMITX_MAT_COEFF_NUM; i++)
    {
        /* Mask & copy MSB */
        buf[i*2] = (UInt8)(((UInt16)pMatCoeff->Coeff[i] & 0x0700) >> 8);
        /* Copy LSB */
        buf[(i*2)+1] = (UInt8)((UInt16)pMatCoeff->Coeff[i] & 0x00FF);
    }
    err = setHwRegisters(pDis,
                         E_REG_P00_MAT_P11_MSB_W,
                         &buf[0],
                         HDMITX_MAT_COEFF_NUM * 2);
    return err;
}


/*============================================================================*/
/* tmbslTDA9989MatrixSetConversion                                             */
/*============================================================================*/
tmErrorCode_t
tmbslTDA9989MatrixSetConversion
(
    tmUnitSelect_t          txUnit,
    tmbslHdmiTxVidFmt_t     vinFmt,
    tmbslHdmiTxVinMode_t    vinMode,
    tmbslHdmiTxVidFmt_t     voutFmt,
    tmbslHdmiTxVoutMode_t   voutMode,
    tmbslHdmiTxVQR_t        dviVqr
)
{
    tmHdmiTxobject_t            *pDis;  /* Ptr to Device Instance Structure */
    tmErrorCode_t               err;        /* Error code */
    tmbslTDA9989Colourspace_t    cspace_in;  /* Input colourspaces */
    tmbslTDA9989Colourspace_t    cspace_out; /* Output colourspaces */
    Int                         matrixIndex;/* Index into matrix preset array */
    UInt8                       buf[MATRIX_PRESET_SIZE]; /* Temp buffer */
    UInt8                       i; /* Loop index */

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Check remaining parameter(s) */
    RETIF_BADPARAM(!IS_VALID_FMT(vinFmt))
    RETIF_BADPARAM(!IS_VALID_FMT(voutFmt))
    /* NB: NO_CHANGE is not valid for this function, so limit to actual values*/
    RETIF_BADPARAM(vinMode >= HDMITX_VINMODE_NO_CHANGE)
    /* NB: NO_CHANGE is not valid for this function, so limit to actual values*/
    RETIF_BADPARAM(voutMode >= HDMITX_VOUTMODE_NO_CHANGE)

    /* Since vinMode and voutMode are different types, we don't use a local
       function to do this and use inline code twice */
     

    /* Calculate input colour space */
        switch (vinFmt)
        {       /* Catch the HD modes */
        case HDMITX_VFMT_04_1280x720p_60Hz:
        case HDMITX_VFMT_05_1920x1080i_60Hz:
        case HDMITX_VFMT_16_1920x1080p_60Hz:
        case HDMITX_VFMT_19_1280x720p_50Hz:
        case HDMITX_VFMT_20_1920x1080i_50Hz:
        case HDMITX_VFMT_31_1920x1080p_50Hz:
        case HDMITX_VFMT_32_1920x1080p_24Hz:
        case HDMITX_VFMT_33_1920x1080p_25Hz:
        case HDMITX_VFMT_34_1920x1080p_30Hz:
        case HDMITX_VFMT_60_1280x720p_24Hz:
        case HDMITX_VFMT_61_1280x720p_25Hz:
        case HDMITX_VFMT_62_1280x720p_30Hz:
    
            if(vinMode == HDMITX_VINMODE_RGB444)    /* RGB */
            {
                cspace_in = HDMITX_CS_RGB_LIMITED;
            }
            else                                    /* CCIR656, YUV444, YU422 */
            {
                cspace_in = HDMITX_CS_YUV_ITU_BT709;
            }
            break;
        default:    /* Now all the SD modes */
            if(vinMode == HDMITX_VINMODE_RGB444)    /* we're RGB */
            {
                cspace_in = HDMITX_CS_RGB_LIMITED;
            }
            else                                    /* CCIR656, YUV444, YU422 */
            {
                cspace_in = HDMITX_CS_YUV_ITU_BT601;
            }
            break;
        }

/*     } */

    /* Calculate output colour space */
#ifdef FORMAT_PC
    if(IS_PC(voutFmt))
    {
        /* Catch the PC formats */
    	cspace_in = HDMITX_CS_RGB_FULL; /* PR1570 FIXED */
        cspace_out = HDMITX_CS_RGB_FULL;
    }
    else
    {
#endif
        switch (voutFmt)
        {       /* Catch the HD modes */
        case HDMITX_VFMT_04_1280x720p_60Hz:
        case HDMITX_VFMT_05_1920x1080i_60Hz:
        case HDMITX_VFMT_16_1920x1080p_60Hz:
        case HDMITX_VFMT_19_1280x720p_50Hz:
        case HDMITX_VFMT_20_1920x1080i_50Hz:
        case HDMITX_VFMT_31_1920x1080p_50Hz:
        case HDMITX_VFMT_32_1920x1080p_24Hz:
        case HDMITX_VFMT_33_1920x1080p_25Hz:
        case HDMITX_VFMT_34_1920x1080p_30Hz:
        case HDMITX_VFMT_60_1280x720p_24Hz:
        case HDMITX_VFMT_61_1280x720p_25Hz:
        case HDMITX_VFMT_62_1280x720p_30Hz:
            
            if(voutMode == HDMITX_VOUTMODE_RGB444)  /* RGB */
            {
                cspace_out = HDMITX_CS_RGB_LIMITED;
            }
            else                                    /* YUV444 or YUV422 */
            {
                cspace_out = HDMITX_CS_YUV_ITU_BT709;
            }
            break;
        default:    /* Now all the SD modes */
            if(voutMode == HDMITX_VOUTMODE_RGB444)  /* RGB */
            {
                cspace_out = HDMITX_CS_RGB_LIMITED;
            }
            else                                    /* YUV444 or YUV422 */
            {
                cspace_out = HDMITX_CS_YUV_ITU_BT601;
            }
            break;
        }
#ifdef FORMAT_PC
    }
#endif

#ifdef TMFL_HDCP_OPTIMIZED_POWER
    /* 
       power management :
       freeze/wakeup color space conversion clock
    */
    err = setHwRegisterField(pDis, E_REG_FEAT_POWER_DOWN,             \
                             E_MASKREG_FEAT_POWER_DOWN_csc,           \
                             (cspace_in == cspace_out));
    RETIF_REG_FAIL(err);
#endif

    if (cspace_in == cspace_out)
    {
        /* Switch off colour matrix by setting bypass flag */
        err = setHwRegisterField(pDis, 
                                 E_REG_P00_MAT_CONTRL_W,
                                 E_MASKREG_P00_MAT_CONTRL_mat_bp,
                                 1);
    }
    else
    {
        /* Load appropriate values into matrix  - we have preset blocks of
         * 31 register vales in a table, just need to work out which set to use
         */
        matrixIndex = kMatrixIndex[cspace_in][cspace_out];

        /* Set the first block byte separately, as it is shadowed and can't
         * be set by setHwRegisters */
        err = setHwRegister(pDis,
                             E_REG_P00_MAT_CONTRL_W,
                             kMatrixPreset[matrixIndex][0]);
        RETIF_REG_FAIL(err)
        
        for (i = 0; i < MATRIX_PRESET_SIZE; i++)
        {
            buf[i] = kMatrixPreset[matrixIndex][i];
        }
        
        /* Set the rest of the block */
        err = setHwRegisters(pDis,
                             E_REG_P00_MAT_OI1_MSB_W,
                             &buf[1],
                             MATRIX_PRESET_SIZE - 1);
    }
    return err;
}

/*============================================================================*/
/* tmbslTDA9989MatrixSetInputOffset                                            */
/*============================================================================*/

tmErrorCode_t
tmbslTDA9989MatrixSetInputOffset
(
    tmUnitSelect_t          txUnit,
    tmbslHdmiTxMatOffset_t  *pMatOffset
)
{
    tmHdmiTxobject_t    *pDis;  /* Pointer to Device Instance Structure */
    tmErrorCode_t       err;    /* Error code */
    Int                 i;      /* Loop index */
    UInt8               buf[HDMITX_MAT_OFFSET_NUM * 2];    /* Temp buffer */

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Check remaining parameter(s) */
    RETIF_BADPARAM(pMatOffset == (tmbslHdmiTxMatOffset_t *)0)
    for (i = 0; i < HDMITX_MAT_OFFSET_NUM; i++)
    {
        RETIF_BADPARAM((pMatOffset->Offset[i] < HDMITX_MAT_OFFSET_MIN) || 
              (pMatOffset->Offset[i] > HDMITX_MAT_OFFSET_MAX))
    }

    /* Convert signed 11 bit values from Offset array to pairs of MSB-LSB
     * register values, and write to register pairs
     */
    for (i = 0; i < HDMITX_MAT_OFFSET_NUM; i++)
    {
        /* Mask & copy MSB */
        buf[i*2] = (UInt8)(((UInt16)pMatOffset->Offset[i] & 0x0700) >> 8);
        /* Copy LSB */
        buf[(i*2)+1] = (UInt8)((UInt16)pMatOffset->Offset[i] & 0x00FF);
    }
    err = setHwRegisters(pDis,
                         E_REG_P00_MAT_OI1_MSB_W,
                         &buf[0],
                         HDMITX_MAT_OFFSET_NUM * 2);
    return err;
}


/*============================================================================*/
/* tmbslTDA9989MatrixSetMode                                                   */
/*============================================================================*/
tmErrorCode_t
tmbslTDA9989MatrixSetMode
(
    tmUnitSelect_t       txUnit,
    tmbslHdmiTxmCntrl_t mControl,
    tmbslHdmiTxmScale_t mScale
)
{
    tmHdmiTxobject_t    *pDis;  /* Pointer to Device Instance Structure */
    tmErrorCode_t       err;    /* Error code */

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Check remaining parameter(s) */
    RETIF_BADPARAM((mControl > HDMITX_MCNTRL_MAX) ||
          (mScale > HDMITX_MSCALE_MAX))

    /* For each value that is not NoChange, update the appropriate register */
    if (mControl != HDMITX_MCNTRL_NO_CHANGE)
    {
        err = setHwRegisterField(pDis, 
                                 E_REG_P00_MAT_CONTRL_W,
                                 E_MASKREG_P00_MAT_CONTRL_mat_bp,
                                 (UInt8)mControl);
        RETIF_REG_FAIL(err)
    }

    if (mScale != HDMITX_MSCALE_NO_CHANGE)
    {
        err = setHwRegisterField(pDis, 
                                 E_REG_P00_MAT_CONTRL_W,
                                 E_MASKREG_P00_MAT_CONTRL_mat_sc,
                                 (UInt8)mScale);
        RETIF_REG_FAIL(err)
    }

    return TM_OK;
}

/*============================================================================*/
/* tmbslTDA9989MatrixSetOutputOffset                                           */
/*============================================================================*/

tmErrorCode_t
tmbslTDA9989MatrixSetOutputOffset
(
    tmUnitSelect_t          txUnit,
    tmbslHdmiTxMatOffset_t  *pMatOffset
)
{
    tmHdmiTxobject_t    *pDis;  /* Pointer to Device Instance Structure */
    tmErrorCode_t       err;    /* Error code */
    Int                 i;      /* Loop index */
    UInt8               buf[HDMITX_MAT_OFFSET_NUM * 2];   /* Temp buffer */

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Check remaining parameter(s) */
    RETIF_BADPARAM(pMatOffset == (tmbslHdmiTxMatOffset_t *)0)
    for (i = 0; i < HDMITX_MAT_OFFSET_NUM; i++)
    {
        RETIF_BADPARAM((pMatOffset->Offset[i] < HDMITX_MAT_OFFSET_MIN) || 
              (pMatOffset->Offset[i] > HDMITX_MAT_OFFSET_MAX))
    }

    /* Convert signed 11 bit values from Offset array to pairs of MSB-LSB
     * register values, and write to register pairs
     */
    for (i = 0; i < HDMITX_MAT_OFFSET_NUM; i++)
    {
        /* Mask & copy MSB */
        buf[i*2] = (UInt8)(((UInt16)pMatOffset->Offset[i] & 0x0700) >> 8);
        /* Copy LSB */
        buf[(i*2)+1] = (UInt8)((UInt16)pMatOffset->Offset[i] & 0x00FF);
    }
    err = setHwRegisters(pDis,
                         E_REG_P00_MAT_OO1_MSB_W,
                         &buf[0],
                         HDMITX_MAT_OFFSET_NUM * 2);
    return err;
}


/*============================================================================*/
/* tmbslTDA9989PktSetAclkRecovery                                              */
/*============================================================================*/

tmErrorCode_t
tmbslTDA9989PktSetAclkRecovery
(
    tmUnitSelect_t  txUnit,
    Bool            bEnable
)
{
    tmHdmiTxobject_t    *pDis;  /* Pointer to Device Instance Structure */
    tmErrorCode_t       err;    /* Error code */

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Return TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED error if the 
     * sinkType is not HDMI
     */ 
    RETIF(pDis->sinkType != HDMITX_SINK_HDMI,
          TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED)

    /* Check remaining parameter(s) */
    RETIF_BADPARAM((bEnable != True) && (bEnable != False))
    
    /* Write the ACR packet insertion flag */
    err = setHwRegisterField(pDis, 
                             E_REG_P11_DIP_FLAGS_RW,
                             E_MASKREG_P11_DIP_FLAGS_acr,
                             (UInt8)bEnable);
    return err;
}

/*============================================================================*/
/* tmbslTDA9989PktSetAcp                                                       */
/*============================================================================*/
tmErrorCode_t
tmbslTDA9989PktSetAcp
(
    tmUnitSelect_t      txUnit,
    tmbslHdmiTxPkt_t    *pPkt,
    UInt                byteCnt,
    UInt8               uAcpType,
    Bool                bEnable
)
{
    tmHdmiTxobject_t    *pDis;  /* Pointer to Device Instance Structure */
    tmErrorCode_t       err;    /* Error code */
    UInt8               buf[3]; /* Temp buffer to hold header bytes */

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Return TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED error if the 
     * sinkType is not HDMI
     */ 
    RETIF(pDis->sinkType != HDMITX_SINK_HDMI,
          TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED)

    /* Only supported for device N4 or later */ 

    /* Check remaining parameter(s) - NULL pointer allowed */
    RETIF_BADPARAM((bEnable != True) && (bEnable != False))

    if(pPkt != Null)
    {
        /* Pointer to structure provided so check parameters */
        RETIF_BADPARAM(byteCnt > HDMITX_PKT_DATA_BYTE_CNT)
        RETIF(byteCnt == 0, TMBSL_ERR_HDMI_INCONSISTENT_PARAMS)

        /* Data to change, start by clearing ACP packet insertion flag */
        err = setHwRegisterField(pDis, 
                                 E_REG_P11_DIP_FLAGS_RW,
                                 E_MASKREG_P11_DIP_FLAGS_acp,
                                 0x00);
        RETIF_REG_FAIL(err)

        /* Prepare ACP header */
        buf[0] = 0x04;      /* ACP packet */
        buf[1] = uAcpType;
        buf[2] = 0;         /* Reserved [HDMI 1.2] */


        /* Write 3 header bytes to registers */
        err = setHwRegisters(pDis,
                             E_REG_P11_ACP_HB0_RW,
                             &buf[0],
                             3);
        RETIF_REG_FAIL(err)

        /* Write "byteCnt" bytes of data to registers */
        err = setHwRegisters(pDis,
                             E_REG_P11_ACP_PB0_RW,
                             &pPkt->dataByte[0],
                             (UInt16)byteCnt);
        RETIF_REG_FAIL(err)
    }

    /* Write the ACP packet insertion flag */
    err = setHwRegisterField(pDis, 
                             E_REG_P11_DIP_FLAGS_RW,
                             E_MASKREG_P11_DIP_FLAGS_acp,
                             (UInt8)bEnable);
    return err;
}


/*============================================================================*/
/* tmbslTDA9989PktSetAudioInfoframe                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA9989PktSetAudioInfoframe
(
    tmUnitSelect_t      txUnit,
    tmbslHdmiTxPktAif_t *pPkt,
    Bool                bEnable
)
{
    tmHdmiTxobject_t    *pDis;  /* Pointer to Device Instance Structure */
    tmErrorCode_t       err;    /* Error code */
    UInt8               buf[9]; /* Temp buffer to hold header/packet bytes */
    UInt16              bufReg; /* Base register used for writing InfoFrame*/
    UInt16              flagReg;/* Flag register to be used */
    UInt8               flagMask;/* Mask used for writing flag register */

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Return TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED error if the 
     * sinkType is not HDMI
     */ 
    RETIF(pDis->sinkType != HDMITX_SINK_HDMI,
          TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED)

    /* Check remaining parameter(s) - NULL pointer allowed */
    RETIF_BADPARAM((bEnable != True) && (bEnable != False))
    if(pPkt != Null)
    {
        /* Pointer to structure provided so check parameters */
        RETIF_BADPARAM(pPkt->CodingType   > 0x0F)
        RETIF_BADPARAM(pPkt->ChannelCount > 0x07)
        RETIF_BADPARAM(pPkt->SampleFreq   > 0x07)
        RETIF_BADPARAM(pPkt->SampleSize   > 0x03)
        /* No need to check ChannelAlloc - all values are allowed */
        RETIF_BADPARAM((pPkt->DownMixInhibit != True) &&
              (pPkt->DownMixInhibit != False))
        RETIF_BADPARAM(pPkt->LevelShift   > 0x0F)
    }

    /* Only supported for device N4 or later */ 

    /* We're using n4 or later, use IF4 buffer for Audio InfoFrame */
    bufReg = E_REG_P10_IF4_HB0_RW; 
    flagReg = E_REG_P11_DIP_IF_FLAGS_RW;
    flagMask = E_MASKREG_P11_DIP_IF_FLAGS_if4;

    if(pPkt != Null)
    {
        /* Data to change, start by clearing AIF packet insertion flag */
        err = setHwRegisterField(pDis, 
                                 flagReg,
                                 flagMask,
                                 0x00);
        RETIF_REG_FAIL(err)

        /* Prepare AIF header */
        buf[0] = 0x84;     /* Audio InfoFrame */
        buf[1] = 0x01;     /* Version 1 [HDMI 1.2] */
        buf[2] = 0x0A;     /* Length [HDMI 1.2] */

        /* Prepare AIF packet (byte numbers offset by 3) */
        buf[0+3] = 0;     /* Preset checksum to zero so calculation works! */
        buf[1+3] = ((pPkt->CodingType & 0x0F) << 4) |
                     (pPkt->ChannelCount & 0x07);        /* CT3-0, CC2-0 */
        buf[2+3] = ((pPkt->SampleFreq & 0x07) << 2) |
                     (pPkt->SampleSize & 0x03);          /* SF2-0, SS1-0 */
        buf[3+3] = 0;                                      /* [HDMI 1.2] */
        buf[4+3] = pPkt->ChannelAlloc;                  /* CA7-0 */
        buf[5+3] = ((pPkt->LevelShift & 0x0F) << 3);    /* LS3-0 */
        if(pPkt->DownMixInhibit == True)
        {
            buf[5+3] += 0x80;                              /* DMI bit */
        }

        /* Calculate checksum - this is worked out on "Length" bytes of the
         * packet, the checksum (which we've preset to zero), and the three
         * header bytes.  We exclude bytes PB6 to PB10 (which we
         * are not writing) since they are zero.
         */
        buf[0+3] = calculateChecksum(&buf[0], 0x0A+1+3-5);

        /* Write header and packet bytes in one operation */
        err = setHwRegisters(pDis,
                             bufReg,
                             &buf[0],
                             9);
        RETIF_REG_FAIL(err)
    }

    /* Write AIF packet insertion flag */
    err = setHwRegisterField(pDis, 
                             flagReg,
                             flagMask,
                             (UInt8)bEnable);
    return err;
}

/*============================================================================*/
/* tmbslTDA9989PktSetGeneralCntrl                                              */
/*============================================================================*/

tmErrorCode_t
tmbslTDA9989PktSetGeneralCntrl
(
    tmUnitSelect_t      txUnit,
    tmbslHdmiTxaMute_t  *paMute,
    Bool                bEnable
)
{
    tmHdmiTxobject_t    *pDis;  /* Pointer to Device Instance Structure */
    tmErrorCode_t       err;    /* Error code */

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Return TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED error if the 
     * sinkType is not HDMI
     */ 
    RETIF(pDis->sinkType != HDMITX_SINK_HDMI,
          TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED)

    /* Check remaining parameter(s) - NULL pointer allowed */
    RETIF_BADPARAM((bEnable != True) && (bEnable != False))

    if(paMute != Null)
    {
        RETIF_BADPARAM((*paMute != HDMITX_AMUTE_OFF) && (*paMute != HDMITX_AMUTE_ON))

        if (*paMute == HDMITX_AMUTE_ON)
        {
            err = setHwRegister(pDis, E_REG_P11_GC_AVMUTE_RW, 0x02);
            RETIF_REG_FAIL(err)
        }
        else
        {
            err = setHwRegister(pDis, E_REG_P11_GC_AVMUTE_RW, 0x01);
            RETIF_REG_FAIL(err)
        }
    }

    /* Set or clear GC packet insertion flag */
    err = setHwRegisterField(pDis, 
                             E_REG_P11_DIP_FLAGS_RW,
                             E_MASKREG_P11_DIP_FLAGS_gc,
                             (UInt8)bEnable);
    return err;
}

/*============================================================================*/
/* tmbslTDA9989PktSetIsrc1                                                     */
/*============================================================================*/
tmErrorCode_t
tmbslTDA9989PktSetIsrc1
(
    tmUnitSelect_t      txUnit,
    tmbslHdmiTxPkt_t    *pPkt,
    UInt                byteCnt,
    Bool                bIsrcCont,
    Bool                bIsrcValid,
    UInt8               uIsrcStatus,
    Bool                bEnable
)
{
    tmHdmiTxobject_t    *pDis;  /* Pointer to Device Instance Structure */
    tmErrorCode_t       err;    /* Error code */
    UInt8               buf[3]; /* Temp buffer to hold header bytes */

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Return TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED error if the 
     * sinkType is not HDMI
     */ 
    RETIF(pDis->sinkType != HDMITX_SINK_HDMI,
          TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED)

    /* Only supported for device N4 or later */ 

    /* Check remaining parameter(s) - NULL pointer allowed */
    RETIF_BADPARAM((bEnable != True) && (bEnable != False))

    if(pPkt != Null)
    {
        /* Pointer to structure provided so check parameters */
        RETIF_BADPARAM((bIsrcCont != True) && (bIsrcCont != False))
        RETIF_BADPARAM((bIsrcValid != True) && (bIsrcValid != False))
        RETIF_BADPARAM(uIsrcStatus > 7)    /* 3 bits */
        RETIF_BADPARAM(byteCnt > HDMITX_PKT_DATA_BYTE_CNT)
        RETIF(byteCnt == 0, TMBSL_ERR_HDMI_INCONSISTENT_PARAMS)

        /* Data to change, start by clearing ISRC1 packet insertion flag */
        err = setHwRegisterField(pDis, 
                                 E_REG_P11_DIP_FLAGS_RW,
                                 E_MASKREG_P11_DIP_FLAGS_isrc1,
                                 0x00);
        RETIF_REG_FAIL(err)

        /* Prepare ISRC1 header */
        buf[0] = 0x05;      /* ISRC1 packet */
        buf[1] = (uIsrcStatus & 0x07);
        if(bIsrcValid == True)
        {
            buf[1] += 0x40;
        }
        if(bIsrcCont == True)
        {
            buf[1] += 0x80;
        }
        buf[2] = 0;         /* Reserved [HDMI 1.2] */

        /* Write 3 header bytes to registers */
        err = setHwRegisters(pDis,
                             E_REG_P11_ISRC1_HB0_RW,
                             &buf[0],
                             3);
        RETIF_REG_FAIL(err)

        /* Write "byteCnt" bytes of data to registers */
        err = setHwRegisters(pDis,
                             E_REG_P11_ISRC1_PB0_RW,
                             &pPkt->dataByte[0],
                             (UInt16)byteCnt);
        RETIF_REG_FAIL(err)
    }

    /* Write the ISRC1 packet insertion flag */
    err = setHwRegisterField(pDis, 
                             E_REG_P11_DIP_FLAGS_RW,
                             E_MASKREG_P11_DIP_FLAGS_isrc1,
                             (UInt8)bEnable);
    return err;
}

/*============================================================================*/
/* tmbslTDA9989PktSetIsrc2                                                     */
/*============================================================================*/
tmErrorCode_t
tmbslTDA9989PktSetIsrc2
(
    tmUnitSelect_t      txUnit,
    tmbslHdmiTxPkt_t    *pPkt,
    UInt                byteCnt,
    Bool                bEnable
)
{
    tmHdmiTxobject_t    *pDis;  /* Pointer to Device Instance Structure */
    tmErrorCode_t       err;    /* Error code */
    UInt8               buf[3]; /* Temp buffer to hold header bytes */

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Return TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED error if the 
     * sinkType is not HDMI
     */ 
    RETIF(pDis->sinkType != HDMITX_SINK_HDMI,
          TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED)

    /* Only supported for device N4 or later */ 

    /* Check remaining parameter(s) - NULL pointer allowed */
    RETIF_BADPARAM((bEnable != True) && (bEnable != False))

    if(pPkt != Null)
    {
        /* Pointer to structure provided so check parameters */
        RETIF_BADPARAM(byteCnt > HDMITX_PKT_DATA_BYTE_CNT)
        RETIF(byteCnt == 0, TMBSL_ERR_HDMI_INCONSISTENT_PARAMS)

        /* Data to change, start by clearing ISRC2 packet insertion flag */
        err = setHwRegisterField(pDis, 
                                 E_REG_P11_DIP_FLAGS_RW,
                                 E_MASKREG_P11_DIP_FLAGS_isrc2,
                                 0x00);
        RETIF_REG_FAIL(err)

        /* Prepare ISRC2 header */
        buf[0] = 0x06;      /* ISRC2 packet */
        buf[1] = 0;         /* Reserved [HDMI 1.2] */
        buf[2] = 0;         /* Reserved [HDMI 1.2] */

        /* Write 3 header bytes to registers */
        err = setHwRegisters(pDis,
                             E_REG_P11_ISRC2_HB0_RW,
                             &buf[0],
                             3);
        RETIF_REG_FAIL(err)

        /* Write "byteCnt" bytes of data to registers */
        err = setHwRegisters(pDis,
                             E_REG_P11_ISRC2_PB0_RW,
                             &pPkt->dataByte[0],
                             (UInt16)byteCnt);
        RETIF_REG_FAIL(err)
    }

    /* Write the ISRC2 packet insertion flag */
    err = setHwRegisterField(pDis, 
                             E_REG_P11_DIP_FLAGS_RW,
                             E_MASKREG_P11_DIP_FLAGS_isrc2,
                             (UInt8)bEnable);
    return err;
}

/*============================================================================*/
/* tmbslTDA9989PktSetMpegInfoframe                                             */
/*============================================================================*/
tmErrorCode_t
tmbslTDA9989PktSetMpegInfoframe
(
    tmUnitSelect_t          txUnit,
    tmbslHdmiTxPktMpeg_t    *pPkt,
    Bool                    bEnable
)
{
    tmHdmiTxobject_t    *pDis;  /* Pointer to Device Instance Structure */
    tmErrorCode_t       err;    /* Error code */
    UInt8               buf[9]; /* Temp buffer to hold packet */

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Return TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED error if the 
     * sinkType is not HDMI
     */ 
    RETIF(pDis->sinkType != HDMITX_SINK_HDMI,
          TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED)

    /* Only supported for device N4 or later */ 

    /* Check remaining parameter(s) */
    RETIF_BADPARAM((bEnable != True) && (bEnable != False))

    if(pPkt != Null)
    {
        /* Pointer to structure provided so check parameters */
        RETIF_BADPARAM((pPkt->bFieldRepeat != True) && (pPkt->bFieldRepeat != False))
        RETIF_BADPARAM(pPkt->frameType >= HDMITX_MPEG_FRAME_INVALID)

        /* Data to change, start by clearing MPEG packet insertion flag */
        err = setHwRegisterField(pDis, 
                                 E_REG_P11_DIP_IF_FLAGS_RW,
                                 E_MASKREG_P11_DIP_IF_FLAGS_if5,
                                 0x00);
        RETIF_REG_FAIL(err)

        /* Prepare MPEG header */
        buf[0] = 0x85;      /* MPEG Source InfoFrame */
        buf[1] = 0x01;      /* Version 1 [HDMI 1.2] */
        buf[2] = 0x0A;      /* Length [HDMI 1.2] */

        /* Prepare MPEG packet (byte numbers offset by 3) */
        buf[0+3] = 0;     /* Preset checksum to zero so calculation works! */
        buf[1+3] = (UInt8)(pPkt->bitRate & 0x000000FF);
        buf[2+3] = (UInt8)((pPkt->bitRate & 0x0000FF00) >> 8);
        buf[3+3] = (UInt8)((pPkt->bitRate & 0x00FF0000) >> 16);
        buf[4+3] = (UInt8)((pPkt->bitRate & 0xFF000000) >> 24);
        buf[5+3] = pPkt->frameType;                         /* MF1-0 */
        if(pPkt->bFieldRepeat == True)
        {
            buf[5+3] += 0x10;                               /* FR0 bit */
        }

        /* Calculate checksum - this is worked out on "Length" bytes of the
         * packet, the checksum (which we've preset to zero), and the three
         * header bytes.  We exclude bytes PB6 to PB10 (which we
         * are not writing) since they are zero.
         */
        buf[0+3] = calculateChecksum(&buf[0], 0x0A+1+3-5);

        /* Write header and packet bytes in one operation */
        err = setHwRegisters(pDis,
                             E_REG_P10_IF5_HB0_RW,
                             &buf[0],
                             9);
        RETIF_REG_FAIL(err)
    }

    /* Write the MPEG packet insertion flag */
    err = setHwRegisterField(pDis, 
                             E_REG_P11_DIP_IF_FLAGS_RW,
                             E_MASKREG_P11_DIP_IF_FLAGS_if5,
                             (UInt8)bEnable);
    return err;
}
/*============================================================================*/
/* tmbslTDA9989PktSetNullInsert                                                */
/*============================================================================*/
tmErrorCode_t
tmbslTDA9989PktSetNullInsert
(
    tmUnitSelect_t  txUnit,
    Bool            bEnable
)
{
    tmHdmiTxobject_t    *pDis;  /* Pointer to Device Instance Structure */
    tmErrorCode_t       err;    /* Error code */

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Return TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED error if the 
     * sinkType is not HDMI
     */ 
    RETIF(pDis->sinkType != HDMITX_SINK_HDMI,
          TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED)

    /* Check remaining parameter(s) */
    RETIF_BADPARAM((bEnable != True) && (bEnable != False))

    /* Set or clear FORCE_NULL packet insertion flag */
    err = setHwRegisterField(pDis, 
                             E_REG_P11_DIP_FLAGS_RW,
                             E_MASKREG_P11_DIP_FLAGS_force_null,
                             (UInt8)bEnable);
    return err;
}

/*============================================================================*/
/* tmbslTDA9989PktSetNullSingle                                                */
/*============================================================================*/
tmErrorCode_t
tmbslTDA9989PktSetNullSingle
(
    tmUnitSelect_t  txUnit
)
{
    tmHdmiTxobject_t    *pDis;  /* Pointer to Device Instance Structure */
    tmErrorCode_t       err;    /* Error code */

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Return TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED error if the 
     * sinkType is not HDMI
     */ 
    RETIF(pDis->sinkType != HDMITX_SINK_HDMI,
          TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED)

    /* Set NULL packet insertion flag */
    err = setHwRegisterField(pDis, 
                             E_REG_P11_DIP_FLAGS_RW,
                             E_MASKREG_P11_DIP_FLAGS_null,
                             0x01);
    return err;
}

/*============================================================================*/
/* tmbslTDA9989PktSetSpdInfoframe                                              */
/*============================================================================*/
tmErrorCode_t
tmbslTDA9989PktSetSpdInfoframe
(
    tmUnitSelect_t      txUnit,
    tmbslHdmiTxPktSpd_t *pPkt,
    Bool                bEnable
)
{
    tmHdmiTxobject_t    *pDis;  /* Pointer to Device Instance Structure */
    tmErrorCode_t       err;    /* Error code */
    UInt8               buf[29];/* Temp buffer to hold packet */

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Return TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED error if the 
     * sinkType is not HDMI
     */ 
    RETIF(pDis->sinkType != HDMITX_SINK_HDMI,
          TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED)

    /* Only supported for device N4 or later */ 

    /* Check remaining parameter(s) */
    RETIF_BADPARAM((bEnable != True) && (bEnable != False))

    if(pPkt != Null)
    {
        /* Pointer to structure provided so check parameters */
        RETIF_BADPARAM(pPkt->SourceDevInfo >= HDMITX_SPD_INFO_INVALID)

        /* Data to change, start by clearing SPD packet insertion flag */
        err = setHwRegisterField(pDis, 
                                 E_REG_P11_DIP_IF_FLAGS_RW,
                                 E_MASKREG_P11_DIP_IF_FLAGS_if3,
                                 0x00);
        RETIF_REG_FAIL(err)

        /* Prepare SPD header */
        buf[0] = 0x83;      /* Source. Product Descriptor InfoFrame */
        buf[1] = 0x01;      /* Version 1 [CEA 861B] */
        buf[2] = 0x19;      /* Length [HDMI 1.2] */

        /* Prepare SPD packet (byte numbers offset by 3) */
        buf[0+3] = 0;     /* Preset checksum to zero so calculation works! */
        lmemcpy(&buf[1+3], &pPkt->VendorName[0], HDMI_TX_SPD_VENDOR_SIZE);
        lmemcpy(&buf[1+3+HDMI_TX_SPD_VENDOR_SIZE], &pPkt->ProdDescr[0],
               HDMI_TX_SPD_DESCR_SIZE);


        buf[HDMI_TX_SPD_LENGTH+3] = pPkt->SourceDevInfo;

        /* Calculate checksum - this is worked out on "Length" bytes of the
         * packet, the checksum (which we've preset to zero), and the three
         * header bytes.  
         */
        buf[0+3] = calculateChecksum(&buf[0], HDMI_TX_SPD_LENGTH+1+3);

        /* Write header and packet bytes in one operation */
        err = setHwRegisters(pDis,
                             E_REG_P10_IF3_HB0_RW,
                             &buf[0],
                             29);
        RETIF_REG_FAIL(err)
    }

    /* Write the SPD packet insertion flag */
    err = setHwRegisterField(pDis, 
                             E_REG_P11_DIP_IF_FLAGS_RW,
                             E_MASKREG_P11_DIP_IF_FLAGS_if3,
                             (UInt8)bEnable);
    return err;
}


/*============================================================================*/
/* tmbslTDA9989PktSetVideoInfoframe                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA9989PktSetVideoInfoframe
(
    tmUnitSelect_t      txUnit,
    tmbslHdmiTxPktVif_t *pPkt,
    Bool                bEnable
)
{
    tmHdmiTxobject_t    *pDis;  /* Pointer to Device Instance Structure */
    tmErrorCode_t       err;    /* Error code */
    UInt8               buf[17];/* Temp buffer to hold header/packet bytes */
    UInt16              bufReg; /* Base register used for writing InfoFrame*/
    UInt16              flagReg;/* Flag register to be used */
    UInt8               flagMask;/* Mask used for writing flag register */

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Return TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED error if the 
     * sinkType is not HDMI
     */ 
    RETIF(pDis->sinkType != HDMITX_SINK_HDMI,
          TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED)

    /* Check remaining parameter(s) - NULL pointer allowed */
    RETIF_BADPARAM((bEnable != True) && (bEnable != False))
    if(pPkt != Null)
    {
        /* Pointer to structure provided so check parameters */
        RETIF_BADPARAM(pPkt->Colour             > 0x03)
        RETIF_BADPARAM((pPkt->ActiveInfo != True) && (pPkt->ActiveInfo != False))
        RETIF_BADPARAM(pPkt->BarInfo            > 0x03)
        RETIF_BADPARAM(pPkt->ScanInfo           > 0x03)
        RETIF_BADPARAM(pPkt->Colorimetry        > 0x03)
        RETIF_BADPARAM(pPkt->PictureAspectRatio > 0x03)
        RETIF_BADPARAM(pPkt->ActiveFormatRatio  > 0x0F)
        RETIF_BADPARAM(pPkt->Scaling            > 0x03)
        RETIF_BADPARAM(pPkt->VidFormat          > 0x7F)
        RETIF_BADPARAM(pPkt->PixelRepeat        > 0x0F)
    }

    /* Only supported for device N4 or later */ 

    /* We're using n4 or later, use IF2 buffer for Video InfoFrame */
    bufReg = E_REG_P10_IF2_HB0_RW; 
    flagReg = E_REG_P11_DIP_IF_FLAGS_RW;
    flagMask = E_MASKREG_P11_DIP_IF_FLAGS_if2;

    if(pPkt != Null)
    {
        /* Data to change, start by clearing VIF packet insertion flag */
        err = setHwRegisterField(pDis, 
                                 flagReg,
                                 flagMask,
                                 0x00);
        RETIF_REG_FAIL(err)

        /* Prepare VIF header */
        buf[0] = 0x82;     /* Video InfoFrame */
        buf[1] = 0x02;     /* Version 2 [HDMI 1.2] */
        buf[2] = 0x0D;     /* Length [HDMI 1.2] */

        /* Prepare VIF packet (byte numbers offset by 3) */
        buf[0+3] = 0;     /* Preset checksum to zero so calculation works! */
        buf[1+3] = ((pPkt->Colour & 0x03) << 5) |       /* Y1-0, B1-0,S1-0 */
                    ((pPkt->BarInfo & 0x03) << 2) |
                     (pPkt->ScanInfo & 0x03);
        if(pPkt->ActiveInfo == True)
        {
            buf[1+3] += 0x10;                              /* AI bit */
        }
        buf[2+3] = ((pPkt->Colorimetry & 0x03) << 6) |  /* C1-0, M1-0, R3-0 */
                    ((pPkt->PictureAspectRatio & 0x03) << 4) |
                     (pPkt->ActiveFormatRatio & 0x0F);
        buf[3+3] = (pPkt->Scaling & 0x03);              /* SC1-0 */                                    /* [HDMI 1.2] */
        buf[4+3] = (pPkt->VidFormat & 0x7F);            /* VIC6-0 */
        buf[5+3] = (pPkt->PixelRepeat & 0x0F);          /* PR3-0 */
        buf[6+3] = (UInt8)(pPkt->EndTopBarLine & 0x00FF);
        buf[7+3] = (UInt8)((pPkt->EndTopBarLine & 0xFF00) >> 8);
        buf[8+3] = (UInt8)(pPkt->StartBottomBarLine & 0x00FF);
        buf[9+3] = (UInt8)((pPkt->StartBottomBarLine & 0xFF00) >> 8);
        buf[10+3] = (UInt8)(pPkt->EndLeftBarPixel & 0x00FF);
        buf[11+3] = (UInt8)((pPkt->EndLeftBarPixel & 0xFF00) >> 8);
        buf[12+3] = (UInt8)(pPkt->StartRightBarPixel & 0x00FF);
        buf[13+3] = (UInt8)((pPkt->StartRightBarPixel & 0xFF00) >> 8);

        /* Calculate checksum - this is worked out on "Length" bytes of the
         * packet, the checksum (which we've preset to zero), and the three
         * header bytes.
         */
        buf[0+3] = calculateChecksum(&buf[0], 0x0D+1+3);

        /* Write header and packet bytes in one operation */
        err = setHwRegisters(pDis,
                             bufReg,
                             &buf[0],
                             17);
        RETIF_REG_FAIL(err)
    }

    /* Write VIF packet insertion flag */
    err = setHwRegisterField(pDis, 
                             flagReg,
                             flagMask,
                             (UInt8)bEnable);
    return err;
}

/*============================================================================*/
/* tmbslTDA9989PktSetRawVideoInfoframe                                       */
/*============================================================================*/
tmErrorCode_t tmbslTDA9989PktSetRawVideoInfoframe
(
    tmUnitSelect_t          txUnit,
    tmbslHdmiTxPktRawAvi_t *pPkt,
    Bool                    bEnable
)
{


    tmHdmiTxobject_t    *pDis;  /* Pointer to Device Instance Structure */
    tmErrorCode_t       err;    /* Error code */

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Return TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED error if the 
     * sinkType is not HDMI
     */ 
    RETIF(pDis->sinkType != HDMITX_SINK_HDMI,
          TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED)

    /* Check remaining parameter(s) - NULL pointer allowed */
    RETIF_BADPARAM((bEnable != True) && (bEnable != False))
    
    /* use IF2 buffer */
    if(pPkt != Null)
    {
        /* Data to change, start by clearing VIF packet insertion flag */
        err = setHwRegisterField(pDis, 
                                 E_REG_P11_DIP_IF_FLAGS_RW,
                                 E_MASKREG_P11_DIP_IF_FLAGS_if2,
                                 0x00);
        RETIF_REG_FAIL(err)


        /* Write VIF raw header bytes 0-2 */
        err = setHwRegisters(pDis,
                             E_REG_P10_IF2_HB0_RW,
                             pPkt->HB,
                             3);
        RETIF_REG_FAIL(err)

        /* Write VIF raw payload bytes 0-27 */
        err = setHwRegisters(pDis,
                             E_REG_P10_IF2_PB0_RW,
                             pPkt->PB,
                             28);

        RETIF_REG_FAIL(err)

    }

    /* Write VIF packet insertion flag */
    err = setHwRegisterField(pDis, 
                             E_REG_P11_DIP_IF_FLAGS_RW,
                             E_MASKREG_P11_DIP_IF_FLAGS_if2,
                             (UInt8)bEnable);
    
    return err;
}

/*============================================================================*/
/* tmbslTDA9989PktSetVsInfoframe                                               */
/*============================================================================*/

tmErrorCode_t
tmbslTDA9989PktSetVsInfoframe
(
    tmUnitSelect_t      txUnit,
    tmbslHdmiTxPkt_t    *pPkt,
    UInt                byteCnt,
    UInt8               uVersion,
    Bool                bEnable
)
{
    tmHdmiTxobject_t    *pDis;  /* Pointer to Device Instance Structure */
    tmErrorCode_t       err;    /* Error code */
    UInt8               buf[31];/* Temp buffer to hold packet */

    /* Check unit parameter and point to TX unit object */
    err = checkUnitSetDis(txUnit, &pDis);
    RETIF(err != TM_OK, err)

    /* Return TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED error if the 
     * sinkType is not HDMI
     */ 
    RETIF(pDis->sinkType != HDMITX_SINK_HDMI,
          TMBSL_ERR_HDMI_OPERATION_NOT_PERMITTED)

    /* Only supported for device N4 or later */ 

    /* Check remaining parameter(s) - NULL pointer allowed */
    RETIF_BADPARAM((bEnable != True) && (bEnable != False))

    if(pPkt != Null)
    {
        /* Pointer to structure provided so check parameters */
        /* InfoFrame needs a checksum, so 1 usable byte less than full pkt */
        RETIF_BADPARAM(byteCnt > (HDMITX_PKT_DATA_BYTE_CNT-1))
        RETIF(byteCnt == 0, TMBSL_ERR_HDMI_INCONSISTENT_PARAMS)

        /* Data to change, start by clearing VS_IF packet insertion flag */
        err = setHwRegisterField(pDis, 
                                 E_REG_P11_DIP_IF_FLAGS_RW,
                                 E_MASKREG_P11_DIP_IF_FLAGS_if1,
                                 0x00);
        RETIF_REG_FAIL(err)

        /* Prepare VS_IF header */
        lmemset(&buf[0], 0, 31); /* Clear buffer as user may vary length used */
        buf[0] = 0x81;          /* Vendor Specific InfoFrame */
        buf[1] = uVersion;      /* Vendor defined version */
        buf[2] = (UInt8)byteCnt;       /* Length [HDMI 1.2] */

        /* Prepare VS_IF packet (byte numbers offset by 3) */
        buf[0+3] = 0;     /* Preset checksum to zero so calculation works! */
        lmemcpy(&buf[1+3], &pPkt->dataByte[0], byteCnt);

        /* Calculate checksum - this is worked out on "Length" bytes of the
         * packet, the checksum (which we've preset to zero), and the three
         * header bytes.  
         */
        buf[0+3] = calculateChecksum(&buf[0], byteCnt+1+3);

        /* Write header and packet bytes in one operation  - write entire 
         * buffer even though we may not be using it all so that zeros
         * are placed in the unused registers. */
        err = setHwRegisters(pDis,
                             E_REG_P10_IF1_HB0_RW,
                             &buf[0],
                             31);
        RETIF_REG_FAIL(err)
    }

    /* Write the VS_IF packet insertion flag */
    err = setHwRegisterField(pDis, 
                             E_REG_P11_DIP_IF_FLAGS_RW,
                             E_MASKREG_P11_DIP_IF_FLAGS_if1,
                             (UInt8)bEnable);
    return err;
}

/*============================================================================*/
/*                              STATIC FUNCTIONS                              */
/*============================================================================*/

/*===============================================================================*/
/* reg_vid_fmt(): get register index for normal and 3D, plus sync table          */
/*===============================================================================*/
static UInt8 reg_vid_fmt(tmbslHdmiTxVidFmt_t fmt,              \
                         tmbslHdmiTx3DStructure_t structure3D, \
                         UInt8 *idx,                           \
                         UInt8 *idx3d,                         \
                         struct sync_desc **sync)
{
   struct vic2reg *hash;
   int i;

   (*idx)=REGVFMT_INVALID;
   if (idx3d) (*idx3d)=REGVFMT_INVALID;
   if (IS_TV(fmt)) {
      VIC2REG_LOOP(vic2reg_TV,idx);
      if (idx3d) {
         if (structure3D == HDMITX_3D_FRAME_PACKING) {
            /* any 3D FP prefetch ? */
            VIC2REG_LOOP(vic2reg_TV_FP,idx3d);
         }
      }
   }
#ifdef FORMAT_PC
   else {
      VIC2REG_LOOP(vic2reg_PC,idx);
   }
#endif
   /* PR1570 FIXED */
   if (sync) {
	   if PREFETCH(*idx) {
		   *sync = (struct sync_desc *)ref_sync;
	   }
#ifdef FORMAT_PC 	   
	   else if PCFORMAT(*idx) {
		   *sync = (struct sync_desc *)ref_sync_PC;
		   *idx = *idx - E_REGVFMT_MAX_EXTRA;
	   }
#endif //FORMAT_PC
	   else {
		   *sync = (struct sync_desc *)ref_sync_extra;
	   }
   }
   return ((*idx)==REGVFMT_INVALID);
}

/*===============================================================================*/
/* pix_clk(): get pixel clock                                                    */
/*===============================================================================*/
UInt8 pix_clk(tmbslHdmiTxVidFmt_t fmt, tmbslHdmiTxVfreq_t freq, UInt8 *pclk)
{

   (*pclk)=REGVFMT_INVALID;
#ifdef FORMAT_PC
   if (IS_PC(fmt)) {
      (*pclk)=kVfmtToPixClk_PC[fmt - HDMITX_VFMT_PC_MIN];
   }
#endif
   if (IS_TV(fmt)) {
      (*pclk)=kVfmtToPixClk_TV[fmt - HDMITX_VFMT_TV_MIN][freq];
   }
   return ((*pclk)==REGVFMT_INVALID);
}
#ifdef TMFL_LINUX_OS_KERNEL_DRIVER
EXPORT_SYMBOL(pix_clk);
#endif

/*===============================================================================*/
/* calculateVidFmtIndex(): Calculate table index according to video format value */
/*===============================================================================*/
tmbslHdmiTxVidFmt_t calculateVidFmtIndex(tmbslHdmiTxVidFmt_t vidFmt)
{
    tmbslHdmiTxVidFmt_t vidFmtIndex = vidFmt;
    
    /* Hanlde VIC or table index discontinuity */
    if((vidFmt >= HDMITX_VFMT_60_1280x720p_24Hz) && (vidFmt <= HDMITX_VFMT_62_1280x720p_30Hz))
    {
        vidFmtIndex = (tmbslHdmiTxVidFmt_t)(HDMITX_VFMT_INDEX_60_1280x720p_24Hz + (vidFmt - HDMITX_VFMT_60_1280x720p_24Hz));
    }
#ifdef FORMAT_PC
    else if (IS_PC(vidFmt))
    {
        vidFmtIndex = (tmbslHdmiTxVidFmt_t)(HDMITX_VFMT_TV_NUM + (vidFmt - HDMITX_VFMT_PC_MIN));
    }
#endif /* FORMAT_PC */
    return(vidFmtIndex);
}

/*============================================================================*/
/* setDeVs                                                                    */
/*============================================================================*/
static tmErrorCode_t
setDeVs
(
    tmHdmiTxobject_t    *pDis,
    tmbslHdmiTxVidFmt_t  voutFmt,
    tmbslHdmiTx3DStructure_t structure3D
)
{
    tmErrorCode_t   err;        /* Error code */
    UInt16          vsPixStrt2; /* VS pixel number for start pulse in field 2 */
    UInt8           reg_idx;    /* Video format value used for register */
    struct sync_desc *sync;

    /* IF voutFmt = No Change THEN return TM_OK */
    RETIF(voutFmt == HDMITX_VFMT_NO_CHANGE, TM_OK);

    /* Quit if the output format does not map to the register format */
    RETIF_BADPARAM(reg_vid_fmt(voutFmt,structure3D,&reg_idx,0,&sync));

    /* DE_START & DE_STOP no longer set because N2 device no longer supported */

    /* Adjust VS_PIX_STRT_2 and VS_PIX_END_2 for interlaced output formats */
    vsPixStrt2 = sync[BASE(reg_idx)].Vs2;
    err = setHwRegisterMsbLsb(pDis, E_REG_P00_VS_PIX_STRT_2_MSB_W, vsPixStrt2);
    RETIF_REG_FAIL(err)
    err = setHwRegisterMsbLsb(pDis, E_REG_P00_VS_PIX_END_2_MSB_W, vsPixStrt2);
/*     printk("DBG %s vs2:%d\n",__func__,vsPixStrt2); */

    return err;
}

/*============================================================================*/
/* setPixelRepeat                                                             */
/*============================================================================*/
static tmErrorCode_t
setPixelRepeat
(
    tmHdmiTxobject_t    *pDis,
    tmbslHdmiTxVidFmt_t voutFmt,
    UInt8               uPixelRepeat,
    tmbslHdmiTx3DStructure_t structure3D
)
{
    tmErrorCode_t err = TM_OK;  /* Error code */

    RETIF(voutFmt == HDMITX_VFMT_NO_CHANGE, TM_OK)

    err = InputConfig(pDis,
                      HDMITX_VINMODE_NO_CHANGE,
                      HDMITX_PIXEDGE_NO_CHANGE,
                      HDMITX_PIXRATE_NO_CHANGE,
                      HDMITX_UPSAMPLE_NO_CHANGE, 
                      uPixelRepeat,
                      voutFmt,
                      structure3D);

   return err;
}
/*============================================================================*/
/* setSampling                                                                */
/*============================================================================*/
static tmErrorCode_t
setSampling
(
 tmHdmiTxobject_t   *pDis
)
{
    tmErrorCode_t err;          /* Error code */
    UInt8         upSample;     /* 1 if upsampler must be enabled */
    UInt8         downSample;   /* 1 if downsampler must be enabled */
    UInt8         matrixBypass; /*>0 if matrix has been bypassed */

    if ((pDis->vinMode == HDMITX_VINMODE_YUV422)
    ||  (pDis->vinMode == HDMITX_VINMODE_CCIR656))
    {
        if (pDis->voutMode == HDMITX_VOUTMODE_YUV422)
        {
            /* Input 422/656, output 422 */
            err = getHwRegister(pDis, E_REG_P00_MAT_CONTRL_W, &matrixBypass);
            RETIF_REG_FAIL(err)
            matrixBypass &= E_MASKREG_P00_MAT_CONTRL_mat_bp;
            /* Has matrix been bypassed? */
            if (matrixBypass > 0)
            {
                upSample = 0;
                downSample = 0;
            }
            else
            {
                upSample = 1;
                downSample = 1;
            }
        }
        else
        {
            /* Input 422/656, output not 422 */
            upSample = 1;
            downSample = 0;
        }
    }
    else
    {
        if (pDis->voutMode == HDMITX_VOUTMODE_YUV422)
        {
            /* Input not 422/656, output 422 */
            upSample = 0;
            downSample = 1;
        }
        else
        {
            /* Input not 422/656, output not 422 */
            upSample = 0;
            downSample = 0;
        }
    }

    /* Check upsample mode saved by tmbslTDA9989VideoInSetConfig */
    if (pDis->upsampleMode != HDMITX_UPSAMPLE_AUTO)
    {
        /* Saved upsample mode overrides local one */
        upSample = pDis->upsampleMode;
    }

    /* Set upsampler */
    err = setHwRegisterField(pDis, 
                             E_REG_P00_HVF_CNTRL_0_W, 
                             E_MASKREG_P00_HVF_CNTRL_0_intpol,
                             upSample);
    RETIF_REG_FAIL(err)

    /* Set downsampler */
    err = setHwRegisterField(pDis, 
                             E_REG_P00_HVF_CNTRL_1_W, 
                             E_MASKREG_P00_HVF_CNTRL_1_for,
                             downSample);
    return err;
}


/*============================================================================*/
/* calculateChecksum - returns the byte needed to yield a checksum of zero    */
/*============================================================================*/
static UInt8
calculateChecksum
(   
    UInt8       *pData,     /* Pointer to checksum data */
    Int         numBytes    /* Number of bytes over which to calculate */
    )
{
    UInt8       checksum = 0;   /* Working checksum calculation */
    UInt8       result = 0;     /* Value to be returned */
    Int         i;

    if((pData != Null) && (numBytes > 0))
    {
        for (i = 0; i < numBytes; i++)
        {
            checksum = checksum + (*(pData + i));
        }
        result = (255 - checksum) + 1;
    }
    return result;          /* returns 0 in the case of null ptr or 0 bytes */
}

/*============================================================================*/
/* InputConfig                                                                */
/*============================================================================*/
static tmErrorCode_t
InputConfig
(
    tmHdmiTxobject_t           *pDis,
    tmbslHdmiTxVinMode_t       vinMode,
    tmbslHdmiTxPixEdge_t       sampleEdge,
    tmbslHdmiTxPixRate_t       pixRate,
    tmbslHdmiTxUpsampleMode_t  upsampleMode, 
    UInt8                      uPixelRepeat,
    tmbslHdmiTxVidFmt_t        voutFmt,
    tmbslHdmiTx3DStructure_t structure3D
)
{
    tmErrorCode_t err = TM_OK;  /* Error code */
    UInt8         reg_idx,reg_idx3D;      /* Video format value used for register */
    UInt8         ssd=0;          /* Packed srl, scg and de */
    struct sync_desc *sync;

    /****************Check Parameters********************/
    /* Check parameters */
    RETIF_BADPARAM(vinMode      >= HDMITX_VINMODE_INVALID);
    RETIF_BADPARAM(sampleEdge   >= HDMITX_PIXEDGE_INVALID);
    RETIF_BADPARAM(pixRate      >= HDMITX_PIXRATE_INVALID);
    RETIF_BADPARAM(upsampleMode >= HDMITX_UPSAMPLE_INVALID);

    RETIF(voutFmt == HDMITX_VFMT_NO_CHANGE, TM_OK);
    RETIF_BADPARAM(!IS_VALID_FMT(voutFmt));

    /* Quit if the output format does not map to the register format */
    RETIF_BADPARAM(reg_vid_fmt(voutFmt,structure3D,&reg_idx,&reg_idx3D,&sync));

/****************Set the VinMode************************
- P00_VIP_CNTRL_4_ccir656
- P00_HVF_CNTRL_1_semi_planar
- P02_PLL_SERIAL_3_srl_ccir
- P02_SEL_CLK_sel_vrf_clk
*/
    if (vinMode != HDMITX_VINMODE_NO_CHANGE)
    {
        pDis->vinMode = vinMode;
    }
/****************Set the sampleEdge***********************
-P00_VIP_CNTRL_3_edge*/

    if (sampleEdge != HDMITX_PIXEDGE_NO_CHANGE)
    {
        err = setHwRegisterField(pDis,
                                 E_REG_P00_VIP_CNTRL_3_W,
                                 E_MASKREG_P00_VIP_CNTRL_3_edge,
                                 (UInt8)sampleEdge);
        RETIF_REG_FAIL(err)
    }

/****************Set the Pixel Rate***********************
-P02_CCIR_DIV_refdiv2
-P02_P02_PLL_SCG2_selpllclkin
-P02_P02_PLL_DE_bypass_pllde
-P00_VIP_CNTRL_4_656_alt */

    if (pixRate != HDMITX_PIXRATE_NO_CHANGE)
       {
          pDis->pixRate = pixRate;
       }
    
    if ((pixRate != HDMITX_PIXRATE_NO_CHANGE)||(vinMode != HDMITX_VINMODE_NO_CHANGE))
       {
          switch (pDis->vinMode)
             {
             case HDMITX_VINMODE_RGB444:
             case HDMITX_VINMODE_YUV444:    
                
                if (pDis->pixRate == HDMITX_PIXRATE_SINGLE)
                   {
                      err = setHwRegisterFieldTable(pDis, &kVinMode444[0]);
                      
                      RETIF_REG_FAIL(err)
                         
                         
                         
                         err = setHwRegisterField(pDis, 
                                                  E_REG_P00_VIP_CNTRL_4_W,
                                                  E_MASKREG_P00_VIP_CNTRL_4_656_alt,
                                                  0);
                      RETIF_REG_FAIL(err)
                         
                         }
                else if (pDis->pixRate == HDMITX_PIXRATE_SINGLE_REPEATED)
                   {
                      err = setHwRegisterFieldTable(pDis, &kVinMode444[0]);
                      
                      RETIF_REG_FAIL(err)
                         
                         
                         
                         err = setHwRegisterField(pDis, 
                                                  E_REG_P00_VIP_CNTRL_4_W,
                                                  E_MASKREG_P00_VIP_CNTRL_4_656_alt,
                                                  0);
                      RETIF_REG_FAIL(err)
                         
                         }
                else 
                   {
                      /* Not supported*/
                   }
                break;
             case HDMITX_VINMODE_YUV422: 
                if (pDis->pixRate == HDMITX_PIXRATE_SINGLE)
                   {
                      err = setHwRegisterFieldTable(pDis, &kVinModeYUV422[0]);
                      
                      RETIF_REG_FAIL(err)
                         
                         
                         
                         err = setHwRegisterField(pDis, 
                                                  E_REG_P00_VIP_CNTRL_4_W,
                                                  E_MASKREG_P00_VIP_CNTRL_4_656_alt,
                                                  0);
                      RETIF_REG_FAIL(err)
                         
                         }
                else if (pDis->pixRate == HDMITX_PIXRATE_SINGLE_REPEATED)
                   {
                      err = setHwRegisterFieldTable(pDis, &kVinModeYUV422[0]);
                      
                      RETIF_REG_FAIL(err)
                         
                         
                         
                         err = setHwRegisterField(pDis, 
                                                  E_REG_P00_VIP_CNTRL_4_W,
                                                  E_MASKREG_P00_VIP_CNTRL_4_656_alt,
                                                  0);
                      RETIF_REG_FAIL(err)
                         
                         }
                else 
                   {
                      /* Not supported*/
                      return TMBSL_ERR_HDMI_BAD_PARAMETER;
                   }
                break;
             case HDMITX_VINMODE_CCIR656:
                if(pDis->pixRate == HDMITX_PIXRATE_SINGLE)
                   {
                      
                      err = setHwRegisterFieldTable(pDis, &kVinModeCCIR656[0]);
                      
                      RETIF_REG_FAIL(err)
                         
                         
                         
                         err = setHwRegisterField(pDis, 
                                                  E_REG_P00_VIP_CNTRL_4_W,
                                                  E_MASKREG_P00_VIP_CNTRL_4_656_alt,
                                                  0);
                      RETIF_REG_FAIL(err)
                         
                         }
                else if (pDis->pixRate == HDMITX_PIXRATE_SINGLE_REPEATED)
                   {
                      err = setHwRegisterFieldTable(pDis, &kVinModeCCIR656[0]);
                      
                      RETIF_REG_FAIL(err)
                         
                         
                         
                         err = setHwRegisterField(pDis, 
                                                  E_REG_P00_VIP_CNTRL_4_W,
                                                  E_MASKREG_P00_VIP_CNTRL_4_656_alt,
                                                  0);
                      RETIF_REG_FAIL(err)
                         
                         }
                else if (pDis->pixRate == HDMITX_PIXRATE_DOUBLE)
                   {
                      err = setHwRegisterFieldTable(pDis, &kVinModeCCIR656_DDR_above720p[0]);
                      
                      RETIF_REG_FAIL(err)
                         
                         
                         
                         
                         err = setHwRegisterField(pDis, 
                                                  E_REG_P00_VIP_CNTRL_4_W,
                                                  E_MASKREG_P00_VIP_CNTRL_4_656_alt,
                                                  1);
                      RETIF_REG_FAIL(err)
                         
                         }
                break;
             default:
                err = setHwRegisterFieldTable(pDis, &kVinMode444[0]);
                
                RETIF_REG_FAIL(err)
                   break;
             }
          
       }
    /****************Update the Sample Mode***********************/
    
    if (upsampleMode != HDMITX_UPSAMPLE_NO_CHANGE) {
       pDis->upsampleMode = upsampleMode;
    }

/****************Set the Pixel repeat PLL Value ***********************
- P02_PLL_SERIAL_2_srl_nosc
- P02_PLL_DE_pllde_nosc */

    if ((structure3D == HDMITX_3D_FRAME_PACKING) && \
        (reg_idx3D != REGVFMT_INVALID)) {
       /* embedded 3D video format */
       ssd = pll[reg_idx3D];
    }
    else {
       /* embedded 2D video format */
       ssd = pll[reg_idx];
    }

    if ( ssd < SSD_UNUSED_VALUE) {
        err = setHwRegisterField(pDis, E_REG_P02_PLL_SERIAL_2_RW,
                             E_MASKREG_P02_PLL_SERIAL_2_srl_nosc,
                             ssd);
/*         printk("DBG nosc:%d\n",ssd); */
    }

/*****************Set the Pixel Repetition***********************
- P02_PLL_SERIAL_2_srl_pr*/

    /* Set pixel repetition */
    if (uPixelRepeat != HDMITX_PIXREP_NO_CHANGE)
    {
        if (uPixelRepeat == HDMITX_PIXREP_DEFAULT)
        {
            /* Look up default pixel repeat value for this output format */
            uPixelRepeat = sync[BASE(reg_idx)].pix_rep;
        }

        /* Update current pixel repetition count */
        pDis->pixelRepeatCount = uPixelRepeat;

        err = setHwRegisterField(pDis, 
                                 E_REG_P02_PLL_SERIAL_2_RW, 
                                 E_MASKREG_P02_PLL_SERIAL_2_srl_pr,
                                 uPixelRepeat);
        RETIF_REG_FAIL(err)
        /* Set pixel repetition count for Repetitor module */
        err = setHwRegister(pDis, E_REG_P00_RPT_CNTRL_W, uPixelRepeat);
    }

/*******************Fixe other settings*********************
- P02_PLL_SERIAL_1_srl_man_iz     = 0
- P02_PLL_SERIAL_3_srl_de         = 0
- Pol Clk Sel = P02_SERIALIZER_RW = 0
- P02_BUFFER_OUT_srl_force        = 0 
- P02_BUFFER_OUT_srl_clk          = 0
- P02_PLL_DE_pllde_iz             = 0
*/

    err = setHwRegisterField(pDis, 
                            E_REG_P02_PLL_SERIAL_1_RW, 
                            E_MASKREG_P02_PLL_SERIAL_1_srl_man_iz,
                            0);


RETIF_REG_FAIL(err)

   err = setHwRegisterField(pDis, 
                            E_REG_P02_PLL_SERIAL_3_RW, 
                            E_MASKREG_P02_PLL_SERIAL_3_srl_de,
                            0);
RETIF_REG_FAIL(err)

err = setHwRegister(pDis, E_REG_P02_SERIALIZER_RW, 0);
RETIF_REG_FAIL(err)

return err;
}

/*============================================================================*/
/*                            END OF FILE                                     */
/*============================================================================*/


