
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

//! \file  AcdkBase.cpp

#define LOG_TAG "AcdkBase"

#include <utils/threads.h>
using namespace android;

#include "mtkcam/acdk/AcdkTypes.h"
#include "AcdkLog.h"
#include "mtkcam/acdk/AcdkCommon.h"
#include "AcdkSurfaceView.h"
#include "AcdkCallback.h"
#include "AcdkBase.h"
#include "AcdkMhalBase.h"
#include "AcdkUtility.h"

using namespace NSAcdkMhal;
using namespace NSACDK;

#include <mtkcam/common/hw/hwstddef.h>
#include <mtkcam/camshot/ICamShot.h>
#include <mtkcam/camshot/ISingleShot.h>

using namespace NSCamHW;
using namespace NSCamShot;

#include "kd_imgsensor_define.h"
#include <mtkcam/drv/imem_drv.h>

#include "AcdkMain.h"

/*******************************************************************************
*
********************************************************************************/
AcdkBase* AcdkBase::createInstance()
{
    ACDK_LOGD("createInstance");
    return new AcdkMain;
}



