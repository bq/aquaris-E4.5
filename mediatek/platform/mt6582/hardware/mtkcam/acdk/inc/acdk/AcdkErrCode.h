
///////////////////////////////////////////////////////////////////////////////
// No Warranty
// Except as may be otherwise agreed to in writing, no warranties of any
// kind, whether express or implied, are given by MTK with respect to any MTK
// Deliverables or any use thereof, and MTK Deliverables are provided on an
// "AS IS" basis.  MTK hereby expressly disclaims all such warranties,
// including any implied warranties of merchantability, non-infringement and
// fitness for a particular purpose and any warranties arising out of course
// of performance, course of dealing or usage of trade.  Parties further
// acknowledge that Company may, either presently and/or in the future,
// instruct MTK to assist it in the development and the implementation, in
// accordance with Company's designs, of certain softwares relating to
// Company's product(s) (the "Services").  Except as may be otherwise agreed
// to in writing, no warranties of any kind, whether express or implied, are
// given by MTK with respect to the Services provided, and the Services are
// provided on an "AS IS" basis.  Company further acknowledges that the
// Services may contain errors, that testing is important and Company is
// solely responsible for fully testing the Services and/or derivatives
// thereof before they are used, sublicensed or distributed.  Should there be
// any third party action brought against MTK, arising out of or relating to
// the Services, Company agree to fully indemnify and hold MTK harmless.
// If the parties mutually agree to enter into or continue a business
// relationship or other arrangement, the terms and conditions set forth
// hereunder shall remain effective and, unless explicitly stated otherwise,
// shall prevail in the event of a conflict in the terms in any agreements
// entered into between the parties.
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2008, MediaTek Inc.
// All rights reserved.
//
// Unauthorized use, practice, perform, copy, distribution, reproduction,
// or disclosure of this information in whole or in part is prohibited.
////////////////////////////////////////////////////////////////////////////////


//! \file  AcdkErrCode.h

#ifndef _ACDKERRCODE_H_
#define _ACDKERRCODE_H_

#include "mtkcam/acdk/AcdkTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/**  
*@enum ACDK_ERROR_ENUM_S
*@brief Return value of ACDK
*/
typedef enum ACDK_ERROR_ENUM_S
{
    ACDK_RETURN_NO_ERROR       = 0, //! no error
    ACDK_RETURN_INVALID_DRIVER,     //! invalid driver object
    ACDK_RETURN_API_FAIL,           //! api fail
    ACDK_RETURN_INVALID_PARA,       //! invalid parameter
    ACDK_RETURN_NULL_OBJ,           //! null object
    ACDK_RETURN_INVALID_SENSOR,     //! invalid sensor object
    ACDK_RETURN_MEMORY_ERROR,       //! memory error
    ACDK_RETURN_ERROR_STATE,        //! error state
    ACDK_RETURN_UNKNOWN_ERROR       //! unknown error
} ACDK_ERROR_ENUM_T;

#ifdef __cplusplus
}
#endif

#endif //end _ACDKERRCODE_H_



