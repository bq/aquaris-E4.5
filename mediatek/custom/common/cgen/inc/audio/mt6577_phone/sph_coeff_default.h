/******************************************************************************* 
* 
* Filename: 
* --------- 
* 
* 
* Project: 
* -------- 
*   ALPS 
* 
* Description: 
* ------------ 
* 
* Author: 
* ------- 
* ChiPeng 
* 
*------------------------------------------------------------------------------ 
* $Revision:$ 1.0.0 
* $Modtime:$ 
* $Log:$ 
* 
* 06 26 2010 chipeng.chang 
* [ALPS00002705][Need Patch] [Volunteer Patch] ALPS.10X.W10.11 Volunteer patch for speech parameter 
* modify speech parameters. 
* 
* Mar 15 2010 mtk02308 
* [ALPS] Init Custom parameter 
* 
* 
 
* 
* 
*******************************************************************************/ 
#ifndef SPEECH_COEFF_DEFAULT_H 
#define SPEECH_COEFF_DEFAULT_H 
#ifndef FALSE 
#define FALSE 0
#endif
//speech parameter depen on BT_CHIP cersion 
#if defined(MTK_MT6611) 
#define BT_COMP_FILTER (1 << 15) 
#define BT_SYNC_DELAY  86 
#elif defined(MTK_MT6612) 
#define BT_COMP_FILTER (1 << 15) 
#define BT_SYNC_DELAY  86 
#elif defined(MTK_MT6616) 
#define BT_COMP_FILTER (1 << 15) 
#define BT_SYNC_DELAY  86 
#else // MTK_MT6620 
#define BT_COMP_FILTER (0 << 15) 
#define BT_SYNC_DELAY  0 
#endif 
#ifdef MTK_DUAL_MIC_SUPPORT 
#define SPEECH_MODE_PARA13 (371) 
#define SPEECH_MODE_PARA14 (23) 
#else 
#define SPEECH_MODE_PARA13 (0) 
#define SPEECH_MODE_PARA14 (0) 
#endif 
#define DEFAULT_SPEECH_NORMAL_MODE_PARA \
    96,   253, 16388,    31, 57351,    31,   400,     0,\
    80,  4325,    99,     0, 20488,     0|SPEECH_MODE_PARA13,     0|SPEECH_MODE_PARA14,  8192 
#define DEFAULT_SPEECH_EARPHONE_MODE_PARA \
     0,   189, 10756,    31, 57351,    31,   400,   116,\
    80,  4325,    99,     0, 20488,     0,     0,     0 
#define DEFAULT_SPEECH_LOUDSPK_MODE_PARA \
    96,   224,  5256,    31, 57351,    24607,   400,   132,\
    90,  4325,    99,     0, 20488,     0,     0,     0 
#define DEFAULT_SPEECH_BT_EARPHONE_MODE_PARA \
     0,   253, 10756,    31, 53255, 32799,   400,     0,\
    80,  4325,    99,     0, 53256|BT_COMP_FILTER,     0,     0,BT_SYNC_DELAY 
#define DEFAULT_SPEECH_BT_CORDLESS_MODE_PARA \
     0,     0,     0,     0,     0,     0,     0,     0,\
     0,     0,     0,     0,     0,     0,     0,     0 
#define DEFAULT_SPEECH_CARKIT_MODE_PARA \
    96,   224,  5256,    31, 57351, 16407,   400,   132,\
    80,  4325,    99,     0, 20488,     0,     0,     0 
#define DEFAULT_SPEECH_AUX1_MODE_PARA \
     0,     0,     0,     0,     0,     0,     0,     0,\
     0,     0,     0,     0,     0,     0,     0,     0 
#define DEFAULT_SPEECH_AUX2_MODE_PARA \
     0,     0,     0,     0,     0,     0,     0,     0,\
     0,     0,     0,     0,     0,     0,     0,     0 
#define DEFAULT_SPEECH_COMMON_PARA \
     0, 55997, 31000, 20675, 32769,     0,     0,     0, \
     0,     0,     0,     0
#define DEFAULT_SPEECH_VOL_PARA \
     0,     0,     0,     0 
#define DEFAULT_AUDIO_DEBUG_INFO \
     0,     0,     0,     0,     0,     0,     0,     0, \
     0,     0,     0,     0,     0,     0,     0,     0
 
#define DEFAULT_VM_SUPPORT FALSE 
#define DEFAULT_AUTO_VM FALSE 
#define MICBAIS  1900
/* The Bluetooth PCM digital volume */
/* default_bt_pcm_in_vol : uplink, only for enlarge volume,
                       0x100 : 0dB  gain
                       0x200 : 6dB  gain
                       0x300 : 9dB  gain
                       0x400 : 12dB gain
                       0x800 : 18dB gain 
                       0xF00 : 24dB gain             */
#define DEFAULT_BT_PCM_IN_VOL  0x100
/* default_bt_pcm_out_vol : downlink gain,
                       0x1000 : 0dB; maximum 0x7FFF  */
#define DEFAULT_BT_PCM_OUT_VOL  0x1000
#define DEFAULT_WB_SPEECH_NORMAL_MODE_PARA \
    96,   253, 16388,    31, 57607,    31,   400,     0,\
    80,  4325,    99,     0, 16392,     0,     0,  8192 
#define DEFAULT_WB_SPEECH_EARPHONE_MODE_PARA \
     0,   189, 10756,    31, 57607,    31,   400,   116,\
    80,  4325,    99,     0, 16392,     0,     0,     0 
#define DEFAULT_WB_SPEECH_LOUDSPK_MODE_PARA \
    96,   224,  5256,    31, 57607, 16407,   400,   132,\
    80,  4325,    99,     0, 16392,     0,     0,     0 
#define DEFAULT_WB_SPEECH_BT_EARPHONE_MODE_PARA \
     0,   253, 10756,    31, 53511, 32799,   400,     0,\
    80,  4325,    99,     0, 49160 | BT_COMP_FILTER,     0,     0,BT_SYNC_DELAY 
#define DEFAULT_WB_SPEECH_BT_CORDLESS_MODE_PARA \
     0,     0,     0,     0,     0,     0,     0,     0,\
     0,     0,     0,     0,     0,     0,     0,     0 
#define DEFAULT_WB_SPEECH_CARKIT_MODE_PARA \
    96,   224,  5256,    31, 57607, 16407,   400,   132,\
    80,  4325,    99,     0, 16392,     0,     0,     0 
#define DEFAULT_WB_SPEECH_AUX1_MODE_PARA \
    96,   224,  5256,    31, 57607,     0,   400,     0,\
  4112,  4325,    11,     0,     0,     0,     0,     0 
#define DEFAULT_WB_SPEECH_AUX2_MODE_PARA \
     0,     0,     0,     0,     0,     0,     0,     0,\
     0,     0,     0,     0,     0,     0,     0,     0 

#endif 
