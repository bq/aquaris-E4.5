/*******************************************************************************
 *
 * Filename:
 * ---------
 * aud_custom_exp.h
 *
 * Project:
 * --------
 *   DUMA
 *
 * Description:
 * ------------
 * This file is the header of audio customization related function or definition.
 *
 * Author:
 * -------
 * JY Huang
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Revision:$
 * $Modtime:$
 * $Log:$
 *
 * 05 26 2010 chipeng.chang
 * [ALPS00002287][Need Patch] [Volunteer Patch] ALPS.10X.W10.11 Volunteer patch for audio paramter
 * modify audio parameter.
 *
 * 05 26 2010 chipeng.chang
 * [ALPS00002287][Need Patch] [Volunteer Patch] ALPS.10X.W10.11 Volunteer patch for audio paramter
 * modify for Audio parameter
 *
 *    mtk80306
 * [DUMA00132370] waveform driver file re-structure.
 * waveform driver file re-structure.
 *
 * Jul 28 2009 mtk01352
 * [DUMA00009909] Check in TWO_IN_ONE_SPEAKER and rearrange
 *
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#ifndef AUDIO_CUSTOM_H
#define AUDIO_CUSTOM_H

/* define Gain For Normal */
/* Normal volume: TON, SPK, MIC, FMR, SPH, SID, MED */

#define GAIN_NOR_TON_VOL      8
#define GAIN_NOR_KEY_VOL      43
#define GAIN_NOR_MIC_VOL      26
#define GAIN_NOR_FMR_VOL      0
#define GAIN_NOR_SPH_VOL      20
#define GAIN_NOR_SID_VOL      100
#define GAIN_NOR_MED_VOL      25


/* define Gain For Headset */
/* Headset volume: TON, SPK, MIC, FMR, SPH, SID, MED */

#define GAIN_HED_TON_VOL      8
#define GAIN_HED_KEY_VOL      24
#define GAIN_HED_MIC_VOL      20
#define GAIN_HED_FMR_VOL      24
#define GAIN_HED_SPH_VOL      12
#define GAIN_HED_SID_VOL      100
#define GAIN_HED_MED_VOL      12

/* define Gain For Handfree */
/* Handfree volume: TON, SPK, MIC, FMR, SPH, SID, MED */

#define GAIN_HND_TON_VOL      15
#define GAIN_HND_KEY_VOL      24
#define GAIN_HND_MIC_VOL      20
#define GAIN_HND_FMR_VOL      24
#define GAIN_HND_SPH_VOL      6
#define GAIN_HND_SID_VOL      100
#define GAIN_HND_MED_VOL      12


/* 0: Input FIR coefficients for 2G/3G Normal mode */
/* 1: Input FIR coefficients for 2G/3G/VoIP Headset mode */
/* 2: Input FIR coefficients for 2G/3G Handfree mode */
/* 3: Input FIR coefficients for 2G/3G/VoIP BT mode */
/* 4: Input FIR coefficients for VoIP Normal mode */
/* 5: Input FIR coefficients for VoIP Handfree mode */
#define SPEECH_INPUT_FIR_COEFF \
       19,   260,  -613,   559,  -307,\
      130, -1188,   612, -1706,  1313,\
    -1621,   250, -2905,  3940, -5493,\
     3900, -3786,  1890, -5582,  5289,\
   -14286, 20675, 20675,-14286,  5289,\
    -5582,  1890, -3786,  3900, -5493,\
     3940, -2905,   250, -1621,  1313,\
    -1706,   612, -1188,   130,  -307,\
      559,  -613,   260,    19,     0,\
                                      \
    32767,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
                                      \
      244,   -90, -1236,  1015,  -832,\
      220, -1164,   175, -1256,  -410,\
     -270, -1084,  -543,  1678, -1342,\
      646, -3140,   317, -5259,  6623,\
   -12416, 20675, 20675,-12416,  6623,\
    -5259,   317, -3140,   646, -1342,\
     1678,  -543, -1084,  -270,  -410,\
    -1256,   175, -1164,   220,  -832,\
     1015, -1236,   -90,   244,     0,\
                                      \
    32767,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
                                      \
    32767,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
                                      \
    32767,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0
    /* 0: Output FIR coefficients for 2G/3G Normal mode */
    /* 1: Output FIR coefficients for 2G/3G/VoIP Headset mode */
    /* 2: Output FIR coefficients for 2G/3G Handfree mode */
    /* 3: Output FIR coefficients for 2G/3G/VoIP BT mode */
    /* 4: Output FIR coefficients for VoIP Normal mode */
    /* 5: Output FIR coefficients for VoIP Handfree mode */
#define SPEECH_OUTPUT_FIR_COEFF \
     -352,   182,  -393,   262,  -442,\
      317,  -780,  1346,  -763,  1024,\
     -500,   665, -2522,  2969, -2808,\
     2645,  -426,  1732, -6910,  8171,\
   -10638, 32767, 32767,-10638,  8171,\
    -6910,  1732,  -426,  2645, -2808,\
     2969, -2522,   665,  -500,  1024,\
     -763,  1346,  -780,   317,  -442,\
      262,  -393,   182,  -352,     0,\
                                       \
    32767,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
                                       \
    32767,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
                                       \
    32767,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
                                       \
    32767,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
                                       \
    32767,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0
#define   DG_DL_Speech    0xe3d
#define   DG_Microphone   0x1400
#define   FM_Record_Vol   6     /* 0 is smallest. each step increase 1dB.
                                    Be careful of distortion when increase too much.
                                    Generally, it's not suggested to tune this parameter */

/* 0: Input FIR coefficients for 2G/3G Normal mode */
/* 1: Input FIR coefficients for 2G/3G/VoIP Headset mode */
/* 2: Input FIR coefficients for 2G/3G Handfree mode */
/* 3: Input FIR coefficients for 2G/3G/VoIP BT mode */
/* 4: Input FIR coefficients for VoIP Normal mode */
/* 5: Input FIR coefficients for VoIP Handfree mode */
#define WB_Speech_Input_FIR_Coeff \
      -76,   -63,   -49,  -124,   -71,  -104,  -135,   -69,  -118,  -101,\
      -79,  -162,  -228,  -104,  -197,  -138,  -243,  -257,  -262,  -102,\
     -207,  -187,  -421,  -140,  -199,  -133,    37,  -320,    67,  -414,\
     -551,  -239,   -37,  -887,   931, -1668,  -395, -1251,   363,  -850,\
     2566, -4816,  4122, -4588, 16422, 16422, -4588,  4122, -4816,  2566,\
     -850,   363, -1251,  -395, -1668,   931,  -887,   -37,  -239,  -551,\
     -414,    67,  -320,    37,  -133,  -199,  -140,  -421,  -187,  -207,\
     -102,  -262,  -257,  -243,  -138,  -197,  -104,  -228,  -162,   -79,\
     -101,  -118,   -69,  -135,  -104,   -71,  -124,   -49,   -63,   -76,\
\
    32767,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,  \
\
    32767,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,  \
\
    32767,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,  \
\
    32767,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,  \
\
    32767,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0

/* 0: Output FIR coefficients for 2G/3G Normal mode */
/* 1: Output FIR coefficients for 2G/3G/VoIP Headset mode */
/* 2: Output FIR coefficients for 2G/3G Handfree mode */
/* 3: Output FIR coefficients for 2G/3G/VoIP BT mode */
/* 4: Output FIR coefficients for VoIP Normal mode */
/* 5: Output FIR coefficients for VoIP Handfree mode */

#define WB_Speech_Output_FIR_Coeff \
      -65,  -115,   -36,  -121,   -43,  -123,  -156,  -142,  -181,  -120,\
     -223,   -57,  -233,   -23,  -341,   -28,  -262,   -19,  -279,     8,\
     -196,    45,  -102,   295,   328,    86,   380,  -206,   974,    45,\
      908,  -487,   984,  -734,  2634,  -321,   994, -3254,   729,  -773,\
     4158, -6244,  5622, -5302, 20675, 20675, -5302,  5622, -6244,  4158,\
     -773,   729, -3254,   994,  -321,  2634,  -734,   984,  -487,   908,\
       45,   974,  -206,   380,    86,   328,   295,  -102,    45,  -196,\
        8,  -279,   -19,  -262,   -28,  -341,   -23,  -233,   -57,  -223,\
     -120,  -181,  -142,  -156,  -123,   -43,  -121,   -36,  -115,   -65,\
\
    32767,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
\
    32767,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
\
    32767,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
\
    32767,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
\
    32767,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0, \
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0

/*
 * The Bluetooth DAI Hardware COnfiguration Parameter
 */
#define DEFAULT_BLUETOOTH_SYNC_TYPE               0
#define DEFAULT_BLUETOOTH_SYNC_LENGTH             1

#endif
