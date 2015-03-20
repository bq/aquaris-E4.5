
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

/*******************************************************************************
 * AcdkMhalBase.cpp
 * brief : 
 *******************************************************************************/
#define LOG_TAG "AcdkMhalBase"

#include <utils/threads.h>
#include <cutils/properties.h>
using namespace android;

#include "AcdkLog.h"
#include "AcdkMhalBase.h"

#include <mtkcam/imageio/IPipe.h>
#include <mtkcam/imageio/ICamIOPipe.h>
#include <mtkcam/imageio/ICdpPipe.h>
#include <mtkcam/imageio/IPostProcPipe.h>
using namespace NSImageio;
using namespace NSIspio;

#include "AcdkMhalPure.h"
#include "AcdkMhalEng.h"

using namespace NSAcdkMhal;

/*******************************************************************************
*
********************************************************************************/
AcdkMhalBase *AcdkMhalBase::createInstance()
{
    char value[PROPERTY_VALUE_MAX] = {'\0'};
    property_get("camera.acdkmhal.pure", value, "0");
    MUINT32 swutchFlag = atoi(value);

    if(swutchFlag == 0)
    {
        ACDK_LOGD("createInstance : AcdkMhalEng\n");
        return new AcdkMhalEng;
    }
    else
    {
        ACDK_LOGD("createInstance : AcdkMhalPure\n");
        return new AcdkMhalPure;
    }
}



