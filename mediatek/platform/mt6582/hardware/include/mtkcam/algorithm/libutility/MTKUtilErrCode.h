
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

#ifndef _MTK_UTIL_ERR_CODE_H_
#define _MTK_UTIL_ERR_CODE_H_

/// error message data
#define UTIL_ERR_MESSAGE_DATA(n, CMD) \
    CMD(n, UTIL_OK)\
    CMD(n, UTIL_COMMON_ERR_INVALID_PARAMETER)\
    CMD(n, UTIL_COMMON_ERR_NULL_BUFFER_POINTER)\
    CMD(n, UTIL_COMMON_ERR_OUT_OF_MEMORY)\
    CMD(n, UTIL_COMMON_ERR_UNSUPPORTED_IMAGE_FORMAT)\
    CMD(n, UTIL_IMA_GPU_ERR_CREATING_BUFFER_OBJECT)\
    CMD(n, UTIL_IMA_GPU_ERR_READING_SHADER_BINARY)\
    CMD(n, UTIL_IMA_GPU_ERR_READING_SHADER_STRING)\
    CMD(n, UTIL_IMA_GPU_ERR_CREATING_PROGRAM_OBJECT)\
    CMD(n, UTIL_IMA_GPU_ERR_LINKING_PROGRAM_OBJECT)\
    CMD(n, UTIL_IMA_GPU_ERR_NULL_SHADER_HANDLE)\
    CMD(n, UTIL_IMA_GPU_ERR_NULL_PROGRAM_HANDLE)\
    CMD(n, UTIL_IMA_GPU_ERR_NULL_TEXTURE_HANDLE)\
    CMD(n, UTIL_IMA_GPU_ERR_NULL_VERTEX_HANDLE)\
    CMD(n, UTIL_ERR_MAX)

/// macro command to declare error code
#define UTIL_ERRCODE_ENUM_DECLARE(n, a) a,

/// macro command to get error code string
#define UTIL_ERRCODE_ENUM_STRING(n, a) (a == n) ? #a :

/// macro to get error code name by UTIL_ERRCODE_ENUM_STRING(n, a)
#define UTIL_GET_ERRCODE_NAME(n) \
    (0 == n) ? "UTIL_ERR_UNKNOWN" : UTIL_ERR_MESSAGE_DATA(n, UTIL_ERRCODE_ENUM_STRING)""\

/// error code enum
typedef enum UTIL_ERRCODE_ENUM
{
    UTIL_ERR_UNKNOWN = 0,
    UTIL_ERR_MESSAGE_DATA(, UTIL_ERRCODE_ENUM_DECLARE)
} UTIL_ERRCODE_NUM;

#endif /* _MTK_UTIL_ERR_CODE_H_ */



