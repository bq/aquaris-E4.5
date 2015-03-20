
/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef _MTK_MAV_ERRCODE_H
#define _MTK_MAV_ERRCODE_H

///////////////////////////////////////////////////////////////////////////////                     
//!  Error code formmat is:
//!
//!  Bit 31~24 is global, each module must follow it, bit 23~0 is defined by module
//!  | 31(1 bit) |30-24(7 bits) |         23-0   (24 bits)      |
//!  | Indicator | Module ID    |   Module-defined error Code   |
//!  
//!  Example 1:
//!  | 31(1 bit) |30-24(7 bits) |   23-16(8 bits)   | 15-0(16 bits) |
//!  | Indicator | Module ID    | group or sub-mod  |    Err Code   |
//! 
//!  Example 2:
//!  | 31(1 bit) |30-24(7 bits) | 23-12(12 bits)| 11-8(8 bits) | 7-0(16 bits)  |
//!  | Indicator | Module ID    |   line number |    group     |    Err Code   |
//! 
//!  Indicator  : 0 - success, 1 - error
//!  Module ID  : module ID, defined below
//!  Extended   : module dependent, but providee macro to add partial line info
//!  Err code   : defined in each module's public include file,
//!               IF module ID is MODULE_COMMON, the errocode is
//!               defined here
//
///////////////////////////////////////////////////////////////////////////////

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

#define MODULE_MTK_MAV (0) // Temp value

#define MTKMAV_OKCODE(errid)         OKCODE(MODULE_MTK_MAV, errid)
#define MTKMAV_ERRCODE(errid)        ERRCODE(MODULE_MTK_MAV, errid)


// Detection error code
#define S_MAV_OK                  MTKMAV_OKCODE(0x0000)

#define E_MAV_NEED_OVER_WRITE     MTKMAV_ERRCODE(0x0001)
#define E_MAV_NULL_OBJECT         MTKMAV_ERRCODE(0x0002)
#define E_MAV_WRONG_STATE         MTKMAV_ERRCODE(0x0003)
#define E_MAV_WRONG_CMD_ID        MTKMAV_ERRCODE(0x0004)
#define E_MAV_WRONG_CMD_PARAM     MTKMAV_ERRCODE(0x0005)
#define E_MAV_INSUFF_WORK_BUF     MTKMAV_ERRCODE(0x0006)
#define E_MAV_ILLEGAL_CORE_NUM    MTKMAV_ERRCODE(0x0007)
#define E_MAV_INSUFF_MATCH_PTS    MTKMAV_ERRCODE(0x0008)
#define E_MAV_LM_DIVERGENCE       MTKMAV_ERRCODE(0x0009)
#define E_MAV_INSUFF_ALIGN_NUM    MTKMAV_ERRCODE(0x000A)
#define E_MAV_SMALL_CLIP_REGION   MTKMAV_ERRCODE(0x000B)

#define E_MAV_ERR                 MTKMAV_ERRCODE(0x0100)

#endif



