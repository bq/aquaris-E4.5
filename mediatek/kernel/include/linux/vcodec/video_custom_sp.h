/**
 * @file
 *   video_custom_sp.h
 *
 * @par Project:
 *   Video
 *
 * @par Description:
 *   Video Codec Driver & Codec Liabrary Interface
 *
 * @par Author:
 *   Jackal Chen (mtk02532)
 *
 * @par $Revision: #1 $
 * @par $Modtime:$
 * @par $Log:$
 *
 */

#ifndef VIDEO_CUSTOM_SP_H
#define VIDEO_CUSTOM_SP_H

//#include "video_codec_if_sp.h"
#include "vcodec_if.h"

#define ASSERT(expr)                                            \
    do {                                                        \
        if (!(expr))                                            \
            AssertionFailed(__FUNCTION__,__FILE__, __LINE__);   \
    } while (0)     ///< ASSERT definition

/******************************************************************************
 *
 *
 * decode
 *
 *
******************************************************************************/


/**
 * @par Enumeration
 *   VIDEO_DECODER_T
 * @par Description
 *   This is the item for video decoder format
 */
typedef enum _VIDEO_DECODER_T
{
    CODEC_DEC_NONE      = 0,            ///< NONE
    CODEC_DEC_H263      = (0x1<<0),     ///< H263
    CODEC_DEC_MPEG4     = (0x1<<1),     ///< MPEG4
    CODEC_DEC_H264      = (0x1<<2),     ///< H264
    CODEC_DEC_RV        = (0x1<<3),     ///< RV
    CODEC_DEC_VC1       = (0x1<<4),     ///< VC1
    CODEC_DEC_VP8       = (0x1<<5),     ///< VP8
    CODEC_DEC_VP9       = (0x1<<6),  	///< VP9
    CODEC_DEC_HEVC      = (0x1<<7),     ///< HEVC
    CODEC_DEC_MAX       = (0x1<<8)      ///< MAX
} VIDEO_DECODER_T;

/**
 * @par Function       
 *   GetDecoderAPI
 * @par Description
 *   The function used to get decoder API for codec library  
 * @param              
 *   eDecType           [IN] decoder type
 * @param              
 *   hWrapper           [IN] wrapper
 * @param              
 *   ppDrvModule        [IN/OUT] driver module
 * @param              
 *   bUseMultiCoreCodec [IN] multi core codec flag
 * @par Returns        
 *   VIDEO_DEC_API_T,   the decoder API
 */
VIDEO_DEC_API_T  *GetDecoderAPI(VIDEO_DECODER_T eDecType, HANDLE hWrapper, void **ppDrvModule, unsigned int bUseMultiCoreCodec);
VIDEO_DEC_API_T  *GetDecoderAPI_SingleCore(VIDEO_DECODER_T, HANDLE); // HANDLE : wrapper's handle
//VCODEC_DEC_API_T *GetMPEG4DecoderAPI(void);
//VCODEC_DEC_API_T *GetH264DecoderAPI(void);
//VCODEC_DEC_API_T *GetRVDecoderAPI(void);
//VCODEC_DEC_API_T *GetVP8DecoderAPI(void);
//VCODEC_DEC_API_T *GetVC1DecoderAPI(void);


/******************************************************************************
*
*
* encode
*
*
******************************************************************************/


/**
 * @par Enumeration
 *   VIDEO_ENCODER_T
 * @par Description
 *   This is the item for video decoder format
 */
typedef enum _VIDEO_ENCODER_T
{
    CODEC_ENC_NONE      = 0,            ///< NONE
    CODEC_ENC_H263      = (0x1<<0),     ///< H263
    CODEC_ENC_MPEG4     = (0x1<<1),     ///< MPEG4
    CODEC_ENC_H264      = (0x1<<2),     ///< H264
    CODEC_ENC_HEVC      = (0x1<<3),     ///< HEVC
    CODEC_ENC_VP8       = (0x1<<5),     ///< VP8
    CODEC_ENC_VP9       = (0x1<<6),     ///< VP9
    CODEC_ENC_MAX       = (0x1<<7)      ///< MAX
} VIDEO_ENCODER_T;

/**
 * @par Function       
 *   GetEncoderAPI
 * @par Description
 *   The function used to get encoder API for codec library  
 * @param              
 *   eEncType           [IN] encoder type
 * @param              
 *   hWrapper           [IN] wrapper
 * @param              
 *   ppDrvModule        [IN/OUT] driver module
 * @param              
 *   bUseMultiCoreCodec [IN] multi core codec flag
 * @par Returns        
 *   VIDEO_DEC_API_T,   the encoder API
 */
VIDEO_ENC_API_T *GetEncoderAPI(VIDEO_ENCODER_T eEncType, HANDLE hWrapper, void **ppDrvModule, unsigned int bUseMultiCoreCodec);
VIDEO_ENC_API_T *GetEncoderAPI_SingleCore(VIDEO_ENCODER_T, HANDLE); // HANDLE : wrapper's handle
//VCODEC_ENC_API_T *GetMPEG4EncoderAPI(void);
//VCODEC_ENC_API_T* GetH264EncoderAPI(void);
//VIDEO_ENCODER_API_T *GetVP8EncoderAPI(void);


#endif /* VIDEO_CUSTOM_SP_H */
