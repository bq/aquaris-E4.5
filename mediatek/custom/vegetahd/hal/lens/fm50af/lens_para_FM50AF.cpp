/********************************************************************************************
 *     LEGAL DISCLAIMER
 *
 *     (Header of MediaTek Software/Firmware Release or Documentation)
 *
 *     BY OPENING OR USING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 *     THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE") RECEIVED
 *     FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON AN "AS-IS" BASIS
 *     ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED,
 *     INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR
 *     A PARTICULAR PURPOSE OR NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY
 *     WHATSOEVER WITH RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 *     INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK
 *     ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
 *     NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S SPECIFICATION
 *     OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
 *
 *     BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE LIABILITY WITH
 *     RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION,
 *     TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE
 *     FEES OR SERVICE CHARGE PAID BY BUYER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 *     THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE WITH THE LAWS
 *     OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF LAWS PRINCIPLES.
 ************************************************************************************************/
#include <utils/Log.h>
#include <fcntl.h>
#include <math.h>

#include "camera_custom_nvram.h"
#include "camera_custom_lens.h"


const NVRAM_LENS_PARA_STRUCT FM50AF_LENS_PARA_DEFAULT_VALUE =
{
    //Version
    NVRAM_CAMERA_LENS_FILE_VERSION,

    // Focus Range NVRAM
    {0, 1023},

    // AF NVRAM
    {
        // -------- AF ------------
        {150,  // i4Offset
         21,  // i4NormalNum
         21,  // i4MacroNum
         0,  // i4InfIdxOffset
         0,  // i4MacroIdxOffset
        {
                       0,  30,  60,  90, 120, 150, 180, 210, 258, 306,
                     354, 402, 450, 498, 546, 594, 642, 690, 738, 786,
                     845,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        },
        15,  // i4THRES_MAIN
        10,  // i4THRES_SUB
        5,  // i4INIT_WAIT
        {500, 500, 500, 500, 500},  // i4FRAME_WAIT
        0,  // i4DONE_WAIT
        -1,  // i4FAIL_POS
        33,  // i4FRAME_TIME
        3,  // i4FIRST_FV_WAIT
        40,  // i4FV_CHANGE_THRES
        10000,  // i4FV_CHANGE_OFFSET
        12,  // i4FV_CHANGE_CNT
        0,  // i4GS_CHANGE_THRES
        15,  // i4GS_CHANGE_OFFSET
        12,  // i4GS_CHANGE_CNT
        10,  // i4FV_STABLE_THRES
        10000,  // i4FV_STABLE_OFFSET
        4,  // i4FV_STABLE_NUM
        4,  // i4FV_STABLE_CNT
        12,  // i4FV_1ST_STABLE_THRES
        10000,  // i4FV_1ST_STABLE_OFFSET
        6,  // i4FV_1ST_STABLE_NUM
        6  // i4FV_1ST_STABLE_CNT
        },

        //-------- ZSD AF ------------
        {150,  // i4Offset
         21,  // i4NormalNum
         21,  // i4MacroNum
         0,  // i4InfIdxOffset
         0,  // i4MacroIdxOffset
        {
                       0,  30,  60,  90, 120, 150, 180, 210, 258, 306,
                     354, 402, 450, 498, 546, 594, 642, 690, 738, 786,
                     845,   0,   0,   0,   0,   0,   0,   0,   0,   0
        },
        15,  // i4THRES_MAIN
        10,  // i4THRES_SUB
        5,  // i4INIT_WAIT
        {500, 500, 500, 500, 500}, // i4FRAME_WAIT
        0,  // i4DONE_WAIT
        -1,  // i4FAIL_POS
        33,  // i4FRAME_TIME
        3,  // i4FIRST_FV_WAIT
        45,  // i4FV_CHANGE_THRES
        10000,  // i4FV_CHANGE_OFFSET
        12,  // i4FV_CHANGE_CNT
        0,  // i4GS_CHANGE_THRES
        15,  // i4GS_CHANGE_OFFSET
        12,  // i4GS_CHANGE_CNT
        10,  // i4FV_STABLE_THRES
        10000,  // i4FV_STABLE_OFFSET
        4,  // i4FV_STABLE_NUM
        4,  // i4FV_STABLE_CNT
        12,  // i4FV_1ST_STABLE_THRES
        10000,  // i4FV_1ST_STABLE_OFFSET
        6,  // i4FV_1ST_STABLE_NUM
        6  // i4FV_1ST_STABLE_CNT
        },

        //-------- VAFC ------------
        {160,  // i4Offset
         11,  // i4NormalNum
         11,  // i4MacroNum
         0,  // i4InfIdxOffset
         0,  // i4MacroIdxOffset
        {
                       0,  40,  80, 120, 160, 200, 240, 280, 320, 360,
                     400,   0,   0,   0,   0,   0,   0,   0,   0,   0,
                       0,   0,   0,   0,   0,   0,   0,   0,   0,   0
        },
        15,  // i4THRES_MAIN
        10,  // i4THRES_SUB
        5,  // i4INIT_WAIT
        {500, 500, 500, 500, 500},  // i4FRAME_WAIT
        0,  // i4DONE_WAIT
        -1,  // i4FAIL_POS
        33,  // i4FRAME_TIME
        3,  // i4FIRST_FV_WAIT
        40,  // i4FV_CHANGE_THRES
        10000,  // i4FV_CHANGE_OFFSET
        12,  // i4FV_CHANGE_CNT
        0,  // i4GS_CHANGE_THRES
        15,  // i4GS_CHANGE_OFFSET
        12,  // i4GS_CHANGE_CNT
        10,  // i4FV_STABLE_THRES
        10000,  // i4FV_STABLE_OFFSET
        4,  // i4FV_STABLE_NUM
        4,  // i4FV_STABLE_CNT
        12,  // i4FV_1ST_STABLE_THRES
        10000,  // i4FV_1ST_STABLE_OFFSET
        6,  // i4FV_1ST_STABLE_NUM
        6  // i4FV_1ST_STABLE_CNT
        },

        //-------- sAF_TH ------------
        {
        8,  // i4ISONum;
        {100,150,200,300,400,600,800,1600},    // i4ISO[ISO_MAX_NUM];
        6,  // i4GMeanNum;
        {20,55,105,150,180,205},    // i4GMean[GMEAN_MAX_NUM];
        {
                31,31,31,31,31,89,89,89,
                63,63,63,63,63,127,127,127,
                127,127,127,127,127,180,180,180
            }, // i4GMR[3][ISO_MAX_NUM];
        {
                0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0
            }, // i4FV_DC[GMEAN_MAX_NUM][ISO_MAX_NUM];
            {
                120000,120000,120000,120000,120000,100000,80000,80000,
                120000,120000,120000,120000,120000,100000,80000,80000,
                120000,120000,120000,120000,120000,100000,80000,80000,
                120000,120000,120000,120000,120000,100000,80000,80000,
                120000,120000,120000,120000,120000,100000,80000,80000,
            120000,120000,120000,120000,120000,100000,80000,80000
            }, // i4MIN_TH[GMEAN_MAX_NUM][ISO_MAX_NUM];  
            {
            5,5,5,5,5,5,0,0,
            5,5,5,5,5,5,0,0,
            5,5,5,5,5,5,0,0,
            5,5,5,5,5,5,0,0,
            5,5,5,5,5,5,0,0,
            5,5,5,5,5,5,0,0
            },// i4HW_TH[GMEAN_MAX_NUM][ISO_MAX_NUM]; 
            {
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0
            }, // i4FV_DC2[GMEAN_MAX_NUM][ISO_MAX_NUM];
            {
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0
            }, // i4MIN_TH2[GMEAN_MAX_NUM][ISO_MAX_NUM];
            {
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0
            }
        }, // i4HW_TH2[GMEAN_MAX_NUM][ISO_MAX_NUM]; 
        //-------- sZSDAF_TH ------------
        {
            8,  // i4ISONum;
            {100,150,200,300,400,600,800,1600},    // i4ISO[ISO_MAX_NUM];
            6,  // i4GMeanNum;
            {20,55,105,150,180,205},    // i4GMean[GMEAN_MAX_NUM];
            {
            31,31,31,31,31,31,31,31,
            63,63,63,63,63,63,63,63,
            127,127,127,127,127,127,127,127
            }, // i4GMR[3][ISO_MAX_NUM];
            {
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0
            }, // i4FV_DC[GMEAN_MAX_NUM][ISO_MAX_NUM];
            {
            50000,50000,50000,50000,50000,50000,50000,50000,
            50000,50000,50000,50000,50000,50000,50000,50000,
            50000,50000,50000,50000,50000,50000,50000,50000,
            50000,50000,50000,50000,50000,50000,50000,50000,
            50000,50000,50000,50000,50000,50000,50000,50000,
            50000,50000,50000,50000,50000,50000,50000,50000
            }, // i4MIN_TH[GMEAN_MAX_NUM][ISO_MAX_NUM];  
            {
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0
            }, // i4HW_TH[GMEAN_MAX_NUM][ISO_MAX_NUM]; 
            {
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0
            }, // i4FV_DC2[GMEAN_MAX_NUM][ISO_MAX_NUM];
            {
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0
            }, // i4MIN_TH2[GMEAN_MAX_NUM][ISO_MAX_NUM];
            {
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0
            }
        }, // i4HW_TH2[GMEAN_MAX_NUM][ISO_MAX_NUM]; 
        1,  // i4VAFC_FAIL_CNT
        0,  // i4CHANGE_CNT_DELTA
        0,  // i4LV_THRES
        18,  // i4WIN_PERCENT_W
        24,  // i4WIN_PERCENT_H
        200,  // i4InfPos
        20,  // i4AFC_STEP_SIZE
        {
            {50,100,150,200,250},  // back to bestpos step
            {0,0,0,0,0},  // hysteresis compensate step
        },
        {0,50,150,250,350},    // back jump
        400,  //i4BackJumpPos;
        80,  //i4FDWinPercent;
        40,  //i4FDSizeDiff;
        3,  //i4StatGain;
        {
            0,0,0,0,0,0,0,0,0,0,        
            0,0,0,0,0,0,0,0,0,0}      
        }, // i4Coef[20];
    {0}
};


UINT32 FM50AF_getDefaultData(VOID *pDataBuf, UINT32 size)
{
    UINT32 dataSize = sizeof(NVRAM_LENS_PARA_STRUCT);

    if ((pDataBuf == NULL) || (size < dataSize))
    {
        return 1;
    }

    // copy from Buff to global struct
    memcpy(pDataBuf, &FM50AF_LENS_PARA_DEFAULT_VALUE, dataSize);

    return 0;
}

PFUNC_GETLENSDEFAULT pFM50AF_getDefaultData = FM50AF_getDefaultData;



