
#include <platform/mt_typedefs.h>
#include <platform/mt_i2c.h>
#include <platform/mt8193.h>

//#ifndef bool
//typedef unsigned int bool;
//#endif



bool mt8193_CKGEN_AgtOnClk(e_CLK_T eAgt)
{
    u32 u4Tmp;

    printf("mt8193_CKGEN_AgtOnClk() %d\n", eAgt);

        
    switch (eAgt)
    {
       case e_CLK_NFI:
            u4Tmp = CKGEN_READ32(REG_RW_NFI_CKCFG);
            CKGEN_WRITE32(REG_RW_NFI_CKCFG, u4Tmp & (~CLK_PDN_NFI));
            break;
        case e_CLK_HDMIPLL:
            u4Tmp = CKGEN_READ32(REG_RW_HDMI_PLL_CKCFG);
            CKGEN_WRITE32(REG_RW_HDMI_PLL_CKCFG, u4Tmp & (~CLK_PDN_HDMI_PLL));
            break;
          case e_CLK_HDMIDISP:
            u4Tmp = CKGEN_READ32(REG_RW_HDMI_DISP_CKCFG);
            CKGEN_WRITE32(REG_RW_HDMI_DISP_CKCFG, u4Tmp & (~CLK_PDN_HDMI_DISP));
            break;
          case e_CLK_LVDSDISP:
            u4Tmp = CKGEN_READ32(REG_RW_LVDS_DISP_CKCFG);
            CKGEN_WRITE32(REG_RW_LVDS_DISP_CKCFG, u4Tmp & (~CLK_PDN_LVDS_DISP));
            break;
          case e_CLK_LVDSCTS:
            u4Tmp = CKGEN_READ32(REG_RW_LVDS_CTS_CKCFG);
            CKGEN_WRITE32(REG_RW_LVDS_DISP_CKCFG, u4Tmp & (~CLK_PDN_LVDS_CTS));
            break;
        default:
            return FALSE;
    }
        
       
    return TRUE;
}


bool mt8193_CKGEN_AgtOffClk(e_CLK_T eAgt)
{
      u32 u4Tmp;

      printf("mt8193_CKGEN_AgtOffClk() %d\n", eAgt);

    switch (eAgt)
    {
        case e_CLK_NFI:
            u4Tmp = CKGEN_READ32(REG_RW_NFI_CKCFG);
            CKGEN_WRITE32(REG_RW_NFI_CKCFG, u4Tmp | CLK_PDN_NFI);
            break;
        case e_CLK_HDMIPLL:
            u4Tmp = CKGEN_READ32(REG_RW_HDMI_PLL_CKCFG);
            CKGEN_WRITE32(REG_RW_HDMI_PLL_CKCFG, u4Tmp | CLK_PDN_HDMI_PLL);
            break;
        case e_CLK_HDMIDISP:
            u4Tmp = CKGEN_READ32(REG_RW_HDMI_DISP_CKCFG);
            CKGEN_WRITE32(REG_RW_HDMI_DISP_CKCFG, u4Tmp | CLK_PDN_HDMI_DISP);
            break;
        case e_CLK_LVDSDISP:
            u4Tmp = CKGEN_READ32(REG_RW_LVDS_DISP_CKCFG);
            CKGEN_WRITE32(REG_RW_LVDS_DISP_CKCFG, u4Tmp | CLK_PDN_LVDS_DISP);
            break;
        case e_CLK_LVDSCTS:
            u4Tmp = CKGEN_READ32(REG_RW_LVDS_CTS_CKCFG);
            CKGEN_WRITE32(REG_RW_LVDS_DISP_CKCFG, u4Tmp | CLK_PDN_LVDS_CTS);
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

bool mt8193_CKGEN_AgtSelClk(e_CLK_T eAgt, u32 u4Sel)
{
      u32 u4Tmp;

      printf("mt8193_CKGEN_AgtSelClk() %d\n", eAgt);

      switch (eAgt)
      {
          case e_CLK_NFI:
              u4Tmp = CKGEN_READ32(REG_RW_NFI_CKCFG);
                CKGEN_WRITE32(REG_RW_NFI_CKCFG, u4Tmp | u4Sel);
              break;
          case e_CLK_HDMIPLL:
              u4Tmp = CKGEN_READ32(REG_RW_HDMI_PLL_CKCFG);
                CKGEN_WRITE32(REG_RW_HDMI_PLL_CKCFG, u4Tmp | u4Sel);
              break;
            case e_CLK_HDMIDISP:
              u4Tmp = CKGEN_READ32(REG_RW_HDMI_DISP_CKCFG);
                CKGEN_WRITE32(REG_RW_HDMI_DISP_CKCFG, u4Tmp | u4Sel);
              break;
            case e_CLK_LVDSDISP:
                u4Tmp = CKGEN_READ32(REG_RW_LVDS_DISP_CKCFG);
                CKGEN_WRITE32(REG_RW_LVDS_DISP_CKCFG, u4Tmp | u4Sel);
              break;
            case e_CLK_LVDSCTS:
              u4Tmp = CKGEN_READ32(REG_RW_LVDS_CTS_CKCFG);
                CKGEN_WRITE32(REG_RW_LVDS_DISP_CKCFG, u4Tmp | u4Sel);
              break;
          default:
              return FALSE;
      }
       
    return TRUE;
}

u32 mt8193_CKGEN_AgtGetClk(e_CLK_T eAgt)
{
    printf("mt8193_CKGEN_AgtGetClk() %d\n", eAgt);
    return 0;
}




