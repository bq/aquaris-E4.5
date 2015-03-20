
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
TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE
 *     FEES OR SERVICE CHARGE PAID BY BUYER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 *     THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE WITH THE LAWS
 *     OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF LAWS PRINCIPLES.
 ************************************************************************************************/
 
#ifndef _MTK_TO3D_ERRCODE_H
#define _MTK_TO3D_ERRCODE_H


///////////////////////////////////////////////////////////////////////////
//! Error code type definition
///////////////////////////////////////////////////////////////////////////
typedef MINT32 MRESULT;

///////////////////////////////////////////////////////////////////////////
//! Helper macros to define error code
///////////////////////////////////////////////////////////////////////////
#define ERRCODE(modid, errid)           \
  ((MINT32)                              \
    ((MUINT32)(0x80000000) |             \
     (MUINT32)((modid & 0x7f) << 24) |   \
     (MUINT32)(errid & 0xffff))          \
  )

#define OKCODE(modid, okid)             \
  ((MINT32)                              \
    ((MUINT32)(0x00000000) |             \
     (MUINT32)((modid & 0x7f) << 24) |   \
     (MUINT32)(okid & 0xffff))           \
  )

///////////////////////////////////////////////////////////////////////////
//! Helper macros to check error code
///////////////////////////////////////////////////////////////////////////
#define SUCCEEDED(Status)   ((MRESULT)(Status) >= 0)
#define FAILED(Status)      ((MRESULT)(Status) < 0)

#define MODULE_MTK_TO3D (0) // Temp value

#define MTKTO3D_OKCODE(errid)         OKCODE(MODULE_MTK_TO3D, errid)
#define MTKTO3D_ERRCODE(errid)        ERRCODE(MODULE_MTK_TO3D, errid)


//Error code
#define S_TO3D_OK                  MTKTO3D_OKCODE(0x0000)

#define E_TO3D_NEED_OVER_WRITE     MTKTO3D_ERRCODE(0x0001)
#define E_TO3D_NULL_OBJECT         MTKTO3D_ERRCODE(0x0002)
#define E_TO3D_WRONG_STATE         MTKTO3D_ERRCODE(0x0003)
#define E_TO3D_WRONG_CMD_ID        MTKTO3D_ERRCODE(0x0004)
#define E_TO3D_WRONG_CMD_PARAM     MTKTO3D_ERRCODE(0x0005)
#define E_TO3D_NOT_ENOUGH_MEM      MTKTO3D_ERRCODE(0x0006)
#define E_TO3D_INPUT_IMAGE_TOO_SMALL MTKTO3D_ERRCODE(0x0007)
#define E_TO3D_INVALID_EGL         MTKTO3D_ERRCODE(0x0008)
#define E_TO3D_ERR                 MTKTO3D_ERRCODE(0x0100)

#endif



