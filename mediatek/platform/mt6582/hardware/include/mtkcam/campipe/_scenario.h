
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
#ifndef _MTK_CAMERA_INC_CAMPIPE_SCENARIO_H_
#define _MTK_CAMERA_INC_CAMPIPE_SCENARIO_H_


/*******************************************************************************
*
********************************************************************************/
namespace NSCamPipe {
////////////////////////////////////////////////////////////////////////////////


/**  
 * @enum ESWScenarioID
 * @brief SW Scenario ID, this will map to sw feature or use case 
 */
enum ESWScenarioID
{
    //
    eSWScenarioID_MTK_PREVIEW,            /*!< MTK preview scenario */ 
    //
    eSWScenarioID_DEFAULT_PREVIEW,        /*!< Default/3rd party APK preview scenario */ 
    //
    eSWScenarioID_VIDEO,                  /*!< Video preview scenario */  
    //
    eSWScenarioID_VSS,                    /*!< Video snapshot scenario */  
    //    
    eSWScenarioID_CAPTURE_NORMAL,         /*!< Normal capture scenario */  
    //
    eSWScenarioID_CAPTURE_ZSD,            /*!< ZSD capture scenario */
    //
    eSWScenarioID_PLAYBACK,               /*!< Image playback scenario  */
    //
};


/**  
 * @enum EScenarioFmt
 * @brief Scenario Format
 */
enum EScenarioFmt
{
    //
    eScenarioFmt_RAW,       /*!< RAW sensor scenario format  */
    //
    eScenarioFmt_YUV,       /*!< YUV sensor scenario format  */
    //
    eScenarioFmt_RGB,       /*!< RGB sensor scenario format  */
    //
    eScenarioFmt_JPG,       /*!< Jpeg sensor scenario format  */
    //
    eScenarioFmt_UNKNOWN    = 0xFFFFFFFF,    /*!< Unknow sensor scenario format  */
};


////////////////////////////////////////////////////////////////////////////////
};  //namespace NSCamPipe
#endif  //  _MTK_CAMERA_INC_CAMPIPE_SCENARIO_H_



