
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
// AcdkCLITest.cpp  $Revision$
////////////////////////////////////////////////////////////////////////////////

//! \file  AcdkCLITest.cpp
//! \brief
 
#define LOG_TAG "CamPipeTest"

#include <stdio.h>
//
#include <errno.h>
#include <fcntl.h>

#include <stdlib.h>
extern int main_camio(int argc, char** argv); 
extern int main_xdp(int argc, char **argv); 
extern int main_postproc(int argc, char **argv); 

/*******************************************************************************
*  Main Function 
********************************************************************************/
int main(int argc, char** argv)
{
    int ret = 0; 
   
    if( argc < 2 )
    {
        printf("please select one of the pipe: campipetest <pipe> \n \tcamio(0), postproc(1), xdp(2)\n");
        return -1;
    }

    int testcase = atoi(argv[1]);
    int num = argc - 1;
    char** arg = argv + 1;
    switch( testcase )
    {
        case 0:
            main_camio(num, arg);
            break;
        case 1:
            main_postproc(num, arg);
            break;
        case 2:
            main_xdp(num, arg);
            break;
        default:
            printf("wrong pipe selection (%i)\n", testcase );
            break;
    }
    //
    // main_camio(argc, argv); 
    char* camio[] = {"test", "1"}; 

    //main_camio(2, camio); 
    //
    //main_xdp(argc, argv); 

    //
    //char* postproc[] = {"post", "/data/raw1274x948_00.raw", "1274", "948", "1"}; 
    // main_postproc(argc, argv); 
     //main_postproc(5, postproc); 

    return ret; 
}


