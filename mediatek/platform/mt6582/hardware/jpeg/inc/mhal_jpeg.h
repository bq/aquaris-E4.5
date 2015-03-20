

#ifndef _MHAL_JPEG_H_
#define _MHAL_JPEG_H_

/*****************************************************************************
* Include Header File
*****************************************************************************/

#include "MediaHal.h"

#define MHAL_IMAGE_TYPE_JPEG 0x0000
#define MHAL_IMAGE_TYPE_WEBP 0x0001

/*****************************************************************************
* MHAL Jpeg Decoder API
*****************************************************************************/

int mHalJpgDecParser(unsigned char* srcAddr, unsigned int srcSize, int srcFD, unsigned int img_type);

int mHalJpgDecGetInfo(MHAL_JPEG_DEC_INFO_OUT *decOutInfo);

int mHalJpgDecStart(MHAL_JPEG_DEC_START_IN *decInParams, void* procHandler = NULL);
                     

int mi_mHalJpgDecParser(MHAL_JPEG_DEC_SRC_IN *srcInParams,unsigned char* srcAddr, unsigned int srcSize, int srcFD, unsigned int img_type);
int mi_mHalJpgDecGetInfo(MHAL_JPEG_DEC_INFO_OUT *decOutInfo);
int mi_mHalJpgDecStart(MHAL_JPEG_DEC_START_IN *decInParams, void* procHandler = NULL);                     
/*****************************************************************************
* MHAL Jpeg Encoder API
*****************************************************************************/

int mHalJpgEncStart(MHAL_JPEG_ENC_START_IN *encInParams); 

int mHalScaler_BitBlt( mHalBltParam_t* bltParam );

#endif



