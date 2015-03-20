MODEM_SPEC = MTK_MODEM_3G_WCDMA
  # Description:
  #   MTK modem spec type
  # Option Values:
  #   MTK_MODEM_3G_WCDMA: MTK 3G WCDMA modem
  #   MTK_MODEM_3G_TDD: MTK 3G TD modem
  #   MTK_MODEM_2G_EDGE: MTK 2G EDGE modem
  #   MTK_MODEM_4G_LTE_SM: MTK 4G LTE single mode modem
  #   MTK_MODEM_4G_LTE_MM_WCDMA: MTK 4G LTE-WCDMA-EDGE multi-mode modem
  #   MTK_MODEM_4G_LTE_MM_TDD: MTK 4G LTE-TD-EDGE multi-mode modem
  # Switch-ability:
  #   Non-switchable
include make/modem_spec/$(strip $(MODEM_SPEC)).mak

# ------------------------------ Configurable Features
AFC_VCXO_TYPE = VCXO
  # Description:
  #   the oscillator type used in this project
  # Option Values:
  #   VCTCXO: VCTCXO clk
  #   VCXO: VCXO clk
  # Switch-ability:
  #   [Any] -> [Any]

OTP_SUPPORT = FALSE
  # Description:
  #   Support One-Time-Programming area for the flash device
  #   
  #   Notify:
  #   If  OTP_SUPPORT = TRUE
  #   
  #   Please also modify AP part : 
  #   <EMMC>
  #   Please set /mediatek/config/[project]/ProjectConfig.mk
  #   MTK_EMMC_SUPPORT_OTP = YES
  #   
  #   <NAND>
  #   Please set /mediatek/config/[project]/ProjectConfig.mk
  #   NAND_OTP_SUPPORT = YES
  #   
  #   Relative doc could be download from
  #   http://dms.mediatek.inc:80/webtop/drl/objectId/0900006f8056690f
  # Option Values:
  #   TRUE: Enable One-Time Program Support
  #   FALSE: Disable One-Time Program Support
  # Switch-ability:
  #   FALSE -> TRUE
  #   TRUE -> FALSE

BAND_SUPPORT = QUAD
  # Description:
  #   Described the support band of RF
  # Option Values:
  #   DUAL900: Support 900/1800
  #   EGSM900: Support 900
  #   RGSM900: Support 900
  #   DUAL850: Support 850/1900
  #   QUAD: Support 850/900/1800/1900
  #   PGSM900: Support 900
  #   DCS1800: Support 1800
  #   PCS1900: Support 1900
  #   GSM850: Support 850
  #   GSM450: Support 450
  #   TRIPLE: Support 900/1800/1900
  #   TRIPLE850: Support 850/1800/1900
  # Switch-ability:
  #   DUAL850 -> GSM850
  #   DUAL850 -> PCS1900
  #   DUAL900 -> DCS1800
  #   DUAL900 -> EGSM900
  #   QUAD -> [Any]
  #   TRIPLE -> DCS1800
  #   TRIPLE -> DUAL900
  #   TRIPLE -> EGSM900
  #   TRIPLE -> PCS1900

RF_MODULE = MT6166_2G_SKY_CUSTOM
  # Description:
  #   describe the RF module used in this project
  # Option Values:
  #   NA: the RF module used in this project
  # Switch-ability:
  #   [Any] -> [Any]

UMTS_RF_MODULE = CUSTOM_MT6166_SKY_18
  # Description:
  #   This option is used specify which RF module is used in UL1D RF driver
  #   "UL1D" means UMTS Layer 1 Driver.
  # Option Values:
  #   MT6162: the RF module used in this project
  # Switch-ability:
  #   Non-switchable

# ------------------------------ Verno information
VERNO = MOLY.WR8.W1315.MD.WG.MP.V37.P5
BUILD = BUILD_NO
BRANCH = WR8.W1315.MD.WG.MP
# ------------------------------ System configurations
PLATFORM = MT6582
  # Description:
  #   Name of BB-chip.
  # Option Values:
  #   NA: 
  # Switch-ability:
  #   Non-switchable

CHIP_VER = S00
  # Description:
  #   Chip version, changed according to ECO.
  # Option Values:
  #   S01: .
  #   S00: .
  #   S02: .
  # Switch-ability:
  #   Non-switchable

BOARD_VER = CKT82_WE_KK_BB
  # Description:
  #   Name of the PCB or EVB.
  # Option Values:
  #   N/A
  # Switch-ability:
  #   Non-switchable


#==============
  
CUSTOM_OPTION += 
# if you want to ture off L1_EPSK_TX please add following custom option
CUSTOM_OPTION += __EPSK_TX_SW_SWITCH_OFF__

CUSTOM_OPTION += __SMS_FDN_CHECK_DA_ONLY__

# internal configuration
PROJECT_MAKEFILE_EXT = CKT82_WE_KK_HSPA_EXT
include make/custom_config/$(strip $(PROJECT_MAKEFILE_EXT)).mak

