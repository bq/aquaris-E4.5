/*******************************************************************************
 *
 * Filename:
 * ---------
 * audio_custom_exp.h
 *
 * Project:
 * --------
 *   ALPS
 *
 * Description:
 * ------------
 * This file is the header of audio customization related function or definition.
 *
 * Author:
 * -------
 * ChiPeng
 *
***********************************************************************************/

#ifndef AUDIO_CUSTOM_EXP_H
#define AUDIO_CUSTOM_EXP_H

#include "Audio_Customization_Common.h"

/*********************************************************************************
*Common definitations  are  defined  in below  file 
*alps\mediatek\custom\common\hal\audioflinger\Audio_Customization_Common.h.
*if  some of  common definitations are not need, the specific customer  can mark the definitation in
* Audio_Customization_Common.h or  can undefine  the definitations in this file,just like:
*#undef ENABLE_AUDIO_COMPENSATION_FILTER
***********************************************************************************/

/*****************************************************************
** refine volume
*****************************************************************/
#define DEVICE_MAX_VOLUME       (8)
#define DEVICE_VOICE_MAX_VOLUME (8)
#define DEVICE_AMP_MAX_VOLUME   (15)
#define DEVICE_MIN_VOLUME       (-4)
#define DEVICE_VOICE_MIN_VOLUME (-4)
#define DEVICE_AMP_MIN_VOLUME   (6)
#define DEVICE_VOLUME_RANGE     (64)
#define DEVICE_VOLUME_STEP      (256)

/******************************************************************
** define Vibration SPK Default Center Freq and RMS
******************************************************************/
#define VIBSPK_MV_RMS           (350) //280~560, 70 per step
#define VIBSPK_DEFAULT_FREQ     (156) //141~330 Hz

/******************************************************************
** define using which flag
******************************************************************/
#define USING_CLASSD_AMP                // define using which flag
#define USING_EXTAMP_HP                // define using which flag
#endif

