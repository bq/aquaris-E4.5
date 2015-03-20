
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

#elif defined(MTK_MT6616) || defined(MTK_MT6620) || defined(MTK_MT6622) || defined(MTK_MT6626) || defined(MTK_MT6628)

#define BT_COMP_FILTER (1 << 15)
#define BT_SYNC_DELAY  86

#else // MTK_MT6620

#define BT_COMP_FILTER (0 << 15)
#define BT_SYNC_DELAY  86

#endif

#ifdef MTK_DUAL_MIC_SUPPORT

  #ifndef MTK_INTERNAL
  #define SPEECH_MODE_PARA13 (371)
  #define SPEECH_MODE_PARA14 (23)
  #define SPEECH_MODE_PARA03 (29)
  #define SPEECH_MODE_PARA08 (400)
  #else
  #define SPEECH_MODE_PARA13 (0)
  #define SPEECH_MODE_PARA14 (0)
  #define SPEECH_MODE_PARA03 (31)
  #define SPEECH_MODE_PARA08 (80)
  #endif

#else
#define SPEECH_MODE_PARA13 (0)
#define SPEECH_MODE_PARA14 (0)
#define SPEECH_MODE_PARA03 (31)
#define SPEECH_MODE_PARA08 (80)


#endif

#ifdef NXP_SMARTPA_SUPPORT
	#define MANUAL_CLIPPING (1 << 15)
	#define NXP_DELAY_REF   (1 << 6)
#else
	#define MANUAL_CLIPPING (0 << 15)
	#define NXP_DELAY_REF   (0 << 6) 
#endif



#define DEFAULT_SPEECH_NORMAL_MODE_PARA \
    96,   253, 16388,    29, 57351,    31,   400,    64,\
   400,  4325,   611,     0, 20488,   371,    23,  8192

#define DEFAULT_SPEECH_EARPHONE_MODE_PARA \
     0,   189, 10756,    31, 57351,    31,   400,   134,\
    80,  4325,    611,     0, 20488,     0,     0,     0  

#define DEFAULT_SPEECH_BT_EARPHONE_MODE_PARA \
     0,   253, 10756,    31, 53255,    31,   400,    99,\
    80,  4325,   611,     0, 53256|BT_COMP_FILTER,     0,     0,BT_SYNC_DELAY

#define DEFAULT_SPEECH_LOUDSPK_MODE_PARA \
    96,   224,  5256,    31, 57351, 24607,   400,     6,\
    84,  4325,   611,     0, 20488,     0,     0,     0

#define DEFAULT_SPEECH_CARKIT_MODE_PARA \
    96,   224,  5256,    31, 57351, 24607,   400,   132, \
    84,  4325,    611,     0, 20488,        0,     0,     0

#define DEFAULT_SPEECH_BT_CORDLESS_MODE_PARA \
    0,      0,      0,      0,      0,      0,      0,      0, \
    0,      0,      0,      0,      0,      0,      0,      0

#define DEFAULT_SPEECH_AUX1_MODE_PARA \
    0,      0,      0,      0,      0,      0,      0,      0, \
    0,      0,      0,      0,      0,      0,      0,      0

#define DEFAULT_SPEECH_AUX2_MODE_PARA \
    0,      0,      0,      0,      0,      0,      0,      0, \
    0,      0,      0,      0,      0,      0,      0,      0

#define DEFAULT_SPEECH_COMMON_PARA \
    0,  55997,  31000,    10752,      32769,      0,      0,      0, \
    0,      0,      0,      0

#define DEFAULT_SPEECH_VOL_PARA \
    0,      0,      0,      0

#define DEFAULT_AUDIO_DEBUG_INFO \
    0,      0,      0,      0,      0,      0,      0,      0, \
    0,      0,      0,      0,      0,      0,      0,      0

#define DEFAULT_VM_SUPPORT  FALSE

#define DEFAULT_AUTO_VM     FALSE

#define MICBAIS     1900

#define DEFAULT_WB_SPEECH_NORMAL_MODE_PARA \
    96,   253, 16388,    29, 57607,    31,   400,    64,\
   400,  4325,   611,     0, 16392,   371,    23,  8192

#define DEFAULT_WB_SPEECH_EARPHONE_MODE_PARA \
     0,   189, 10756,    31, 57607,     31,  400,     64, \
    80,  4325,   611,     0,  16392,     0,     0,     0  

#define DEFAULT_WB_SPEECH_BT_EARPHONE_MODE_PARA \
     0,   253, 10756,    31, 53511,  31,   400,     0, \
    80,  4325,   611,     0, 49160|BT_COMP_FILTER,     0,     0,BT_SYNC_DELAY

#define DEFAULT_WB_SPEECH_LOUDSPK_MODE_PARA \
    96,   224,  5256,    31, 57607, 24607,   400,   4,\
    84,  4325,   611,     0, 16392,     0,     0,     0

#define DEFAULT_WB_SPEECH_CARKIT_MODE_PARA \
 65293, 65279, 65274, 65434, 65329, 65349, 65115, 65396,\
 65337, 65403,    37, 65216,    67, 65122, 64985, 65297

#define DEFAULT_WB_SPEECH_BT_CORDLESS_MODE_PARA \
 65460, 65473, 65487, 65412, 65465, 65432, 65401, 65467,\
 65418, 65435, 65457, 65374, 65308, 65432, 65339, 65398

#define DEFAULT_WB_SPEECH_AUX1_MODE_PARA \
 65499, 64649,   931, 63868, 65141, 64285,   363, 64686,\
  2566, 60720,  4122, 60948, 16422, 16422, 60948,  4122

#define DEFAULT_WB_SPEECH_AUX2_MODE_PARA \
 60720,  2566, 64686,   363, 64285, 65141, 63868,   931,\
 64649, 65499, 65297, 64985, 65122,    67, 65216,    37


/* The Bluetooth PCM digital volume */
/* default_bt_pcm_in_vol : uplink, only for enlarge volume,
                           0x100 : 0dB  gain
                           0x200 : 6dB  gain
                           0x300 : 9dB  gain
                           0x400 : 12dB gain
                           0x800 : 18dB gain
                           0xF00 : 24dB gain             */
#define DEFAULT_BT_PCM_IN_VOL        0x100
/* default_bt_pcm_out_vol : downlink gain,
                           0x1000 : 0dB; maximum 0x7FFF  */
#define DEFAULT_BT_PCM_OUT_VOL       0x1000


#endif



