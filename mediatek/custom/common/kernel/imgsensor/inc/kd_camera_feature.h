#ifndef _KD_CAMERA_FEATURE_H_
#define _KD_CAMERA_FEATURE_H_


#ifndef FTYPE_ENUM
    #define FTYPE_ENUM(_enums...)  _enums
#endif//FTYPE_ENUM

#ifndef FID_TO_TYPE_ENUM
    #define FID_TO_TYPE_ENUM(_fid, _enums) \
        typedef enum { _enums/*, OVER_NUM_OF_##_fid*/ } 
#endif//FID_TO_TYPE_ENUM

#include "kd_camera_feature_id.h"
#include "kd_camera_feature_enum.h"


typedef enum {
    ORIENTATION_ANGLE_0 = 0,
    ORIENTATION_ANGLE_90 = 90,
    ORIENTATION_ANGLE_180 = 180,
    ORIENTATION_ANGLE_270 = 270
} ORIENTATION_ANGLE;


typedef enum
{
    DUAL_CAMERA_NONE_SENSOR=0,
    DUAL_CAMERA_MAIN_SENSOR     = 1,
    DUAL_CAMERA_SUB_SENSOR      = 2,
    DUAL_CAMERA_MAIN_2_SENSOR   = 8,
    //for backward compatible
    DUAL_CAMERA_MAIN_SECOND_SENSOR = 8,
    //DUAL_CAMERA_SUB_2_SENSOR   = 16,
    DUAL_CAMERA_SENSOR_MAX
} CAMERA_DUAL_CAMERA_SENSOR_ENUM;


#endif //_KD_IMGSENSOR_DATA_H

