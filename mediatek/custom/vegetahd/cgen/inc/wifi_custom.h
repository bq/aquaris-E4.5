/*******************************************************************************
 *
 * Filename:
 * ---------
 *   wifi_custom.h
 *
 * Project:
 * --------
 *   YuSu
 *
 * Description:
 * ------------
 *    This file is the header of wifi customization related function or definition.
 *
 * Author:
 * -------
 *  Renbang Jiang (MTK80150)
 *
 *------------------------------------------------------------------------------
 * $Revision:$
 * $Modtime:$
 * $Log:$
 *
 * 04 19 2011 cp.wu
 * [ALPS00041285] [Need Patch] [Volunteer Patch][MT6620 Wi-Fi] Merge MT6620 Wi-Fi into mt6575_evb project
 * 1. update init.rc for normal boot/meta/factory for MT6620 Wi-Fi related part.
 * 2. update NVRAM structure definition and default value for MT6620 Wi-Fi
 *
 * 07 10 2010 renbang.jiang
 * [ALPS00121785][Need Patch] [Volunteer Patch] use NVRAM to save Wi-Fi custom data 
 * .
 *
 * 07 10 2010 renbang.jiang
 * [ALPS00121785][Need Patch] [Volunteer Patch] use NVRAM to save Wi-Fi custom data 
 * .
 *
 * 07 10 2010 renbang.jiang
 * [ALPS00121785][Need Patch] [Volunteer Patch] use NVRAM to save Wi-Fi custom data 
 * .
 *
 *
 *******************************************************************************/
#ifndef __WIFI_CUSTOM_H
#define __WIFI_CUSTOM_H

#define WIFI_CUSTOM_SD_BLOCK_SIZE 512
#define WIFI_CUSTOM_SD_BUS_WIDTH  4 
#define WIFI_CUSTOM_SD_CLOCK_RATE 0

#define WIFI_CUSTOM_BT_COEXIST_WINDOW_T 0
#define WIFI_CUSTOM_ENABLE_TX_AUTO_FRAGMENT_FOR_BT 0
#define WIFI_CUSTOM_BTCR0 0
#define WIFI_CUSTOM_BTCR1 0
#define WIFI_CUSTOM_BTCR2 0
#define WIFI_CUSTOM_BTCR3 0

#if defined (MTK_MT6611)

#define WIFI_CUSTOM_SINGLE_ACL_BTCR0 0x82048041
#define WIFI_CUSTOM_SINGLE_ACL_BTCR1 0x19040F00

#define WIFI_CUSTOM_SINGLE_MIX_BTCR0 0x82040061
#define WIFI_CUSTOM_SINGLE_MIX_BTCR1 0x19040E00

#define WIFI_CUSTOM_DUAL_ACL_BTCR0   0x82000061
#define WIFI_CUSTOM_DUAL_ACL_BTCR1   0x09040F00

#define WIFI_CUSTOM_DUAL_MIX_BTCR0   0x82000061
#define WIFI_CUSTOM_DUAL_MIX_BTCR1   0x09040F00

#elif defined (MTK_MT6612)

#define WIFI_CUSTOM_SINGLE_ACL_BTCR0 0x7E048041
#define WIFI_CUSTOM_SINGLE_ACL_BTCR1 0x18840F00

#define WIFI_CUSTOM_SINGLE_MIX_BTCR0 0x82040061
#define WIFI_CUSTOM_SINGLE_MIX_BTCR1 0x18840E00

#define WIFI_CUSTOM_DUAL_ACL_BTCR0   0x7E000061
#define WIFI_CUSTOM_DUAL_ACL_BTCR1   0x08840F00

#define WIFI_CUSTOM_DUAL_MIX_BTCR0   0x7E000061
#define WIFI_CUSTOM_DUAL_MIX_BTCR1   0x08840F00

#elif defined (MTK_MT6616)

#define WIFI_CUSTOM_SINGLE_ACL_BTCR0 0x79048041
#define WIFI_CUSTOM_SINGLE_ACL_BTCR1 0x18040F00

#define WIFI_CUSTOM_SINGLE_MIX_BTCR0 0x82040061
#define WIFI_CUSTOM_SINGLE_MIX_BTCR1 0x18040E00

#define WIFI_CUSTOM_DUAL_ACL_BTCR0   0x79000061
#define WIFI_CUSTOM_DUAL_ACL_BTCR1   0x08040F00

#define WIFI_CUSTOM_DUAL_MIX_BTCR0   0x79000061
#define WIFI_CUSTOM_DUAL_MIX_BTCR1   0x08040F00

#else

#define WIFI_CUSTOM_SINGLE_ACL_BTCR0 0x82048041
#define WIFI_CUSTOM_SINGLE_ACL_BTCR1 0x19040F00

#define WIFI_CUSTOM_SINGLE_MIX_BTCR0 0x82040061
#define WIFI_CUSTOM_SINGLE_MIX_BTCR1 0x19040E00

#define WIFI_CUSTOM_DUAL_ACL_BTCR0   0x82000061
#define WIFI_CUSTOM_DUAL_ACL_BTCR1   0x09040F00

#define WIFI_CUSTOM_DUAL_MIX_BTCR0   0x82000061
#define WIFI_CUSTOM_DUAL_MIX_BTCR1   0x09040F00

#endif

#define WIFI_CUSTOM_SINGLE_ACL_BTCR2 0x0F0001D0
#define WIFI_CUSTOM_SINGLE_ACL_BTCR3 0x0200000A

#define WIFI_CUSTOM_SINGLE_MIX_BTCR2 0x00000000
#define WIFI_CUSTOM_SINGLE_MIX_BTCR3 0x00000000

#define WIFI_CUSTOM_DUAL_ACL_BTCR2   0x0A0001D0
#define WIFI_CUSTOM_DUAL_ACL_BTCR3   0x02000000
    
#define WIFI_CUSTOM_DUAL_MIX_BTCR2   0x00000000
#define WIFI_CUSTOM_DUAL_MIX_BTCR3   0x00000000
        
#define WIFI_CUSTOM_BT_SETTING 1
#define WIFI_CUSTOM_SINGLE_ANT 0
#define WIFI_CUSTOM_BT_PROFILE 0
#define WIFI_CUSTOM_PTA_ENABLED 0
    
#define WIFI_CUSTOM_MAC_ADDRESS {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define WIFI_CUSTOM_COUNTRY_CODE {0x0000, 0x0000, 0x0000, 0x0000}
#define WIFI_CUSTOM_UAPSD_AC 0xFF
#define WIFI_CUSTOM_POWER_MODE 2
#define WIFI_CUSTOM_ATIM_WINDOW 0
#define WIFI_CUSTOM_VOIP_INTERVAL 0
#define WIFI_CUSTOM_POLL_INTERVAL 0
#define WIFI_CUSTOM_POLL_INTERVAL_B 500
#define WIFI_CUSTOM_L3_PKT_FILTER_EN 0
#define WIFI_CUSTOM_ADHOC_MODE 1
#define WIFI_CUSTOM_ROAMING_EN 1
    
#define WIFI_CUSTOM_MULTI_DOMAIN_CAP 0
    
#define WIFI_CUSTOM_GPIO2_MODE 0
    
#define WIFI_CUSTOM_VI_AIFSN_BIAS 0
#define WIFI_CUSTOM_VI_MAX_TXOP_LIMIT 0xFFFF
    
#define WIFI_CUSTOM_INIT_DELAY_IND 100
    
#define WIFI_CUSTOM_USE_WAPI 0
    
#define WIFI_CUSTOM_DAISY_CHAIN_EN 0
    
#define WIFI_CUSTOM_LED_BLINK_MODE 3
#define WIFI_CUSTOM_LED_BLINK_ON_TIME 80
#define WIFI_CUSTOM_LED_BLINK_OFF_TIME 24
    
#define WIFI_CUSTOM_WMM_PS_EN 0
#define WIFI_CUSTOM_MULTI_DTIM_WAKE   300
    
#define WIFI_CUSTOM_RESERVED { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}

#endif 
