
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

#ifndef _DYNAMIC_CCM_H
#define _DYNAMIC_CCM_H

// CCM
typedef struct
{
    MINT32 M11;
    MINT32 M12;
    MINT32 M13;
    MINT32 M21;
    MINT32 M22;
    MINT32 M23;
    MINT32 M31;
    MINT32 M32;
    MINT32 M33;
} ISP_CCM_T;

MVOID MultiCCM(AWB_GAIN_T& rD65,
               AWB_GAIN_T& rTL84,
               AWB_GAIN_T& rCWF,
               AWB_GAIN_T& rA,
               AWB_GAIN_T const& rCurrent, 
               ISP_NVRAM_CCM_POLY22_STRUCT& rCCMPoly22,
               ISP_NVRAM_CCM_T (&rCCMInput)[eIDX_CCM_NUM], 
               ISP_NVRAM_CCM_T& rCCMOutput);


#endif

