
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

#ifndef _MTK_CORE_ERR_CODE_H_
#define _MTK_CORE_ERR_CODE_H_

/* error message data */
#define CORE_ERR_MESSAGE_DATA(n, CMD) \
    CMD(n, CORE_OK)\
    CMD(n, CORE_COMMON_ERR_INVALID_PARAMETER)\
    CMD(n, CORE_COMMON_ERR_NULL_BUFFER_POINTER)\
    CMD(n, CORE_COMMON_ERR_OUT_OF_MEMORY)\
    CMD(n, CORE_MOTION_ERR_LOW_TRUST_VALUE)\
    CMD(n, CORE_MOTION_ERR_LARGE_MOTION_VECTOR)\
    CMD(n, CORE_MOTION_ERR_VERTICAL_SHAKE)\
    CMD(n, CORE_WARP_ERR_INCORRECT_IMAGE_FORMAT)\
    CMD(n, CORE_CYLIND_PROJ_ERR_INCORRECT_IMAGE_FORMAT)\
    CMD(n, CORE_ERR_MAX)

/* macro to declare and get string */
#define CORE_ERRCODE_ENUM_DECLARE(n, a) a,
#define CORE_ERRCODE_ENUM_STRING(n, a) (a == n) ? #a :
#define CORE_GET_ERRCODE_NAME(n) \
    (0 == n) ? "CORE_ERR_UNKNOWN" : CORE_ERR_MESSAGE_DATA(n, CORE_ERRCODE_ENUM_STRING)""\

/* error code enum */
typedef enum CORE_ERRCODE_ENUM
{
    CORE_ERR_UNKNOWN = 0,
    CORE_ERR_MESSAGE_DATA(, CORE_ERRCODE_ENUM_DECLARE)
} CORE_ERRCODE_NUM;

#endif /* _MTK_CORE_ERR_CODE_H_ */



