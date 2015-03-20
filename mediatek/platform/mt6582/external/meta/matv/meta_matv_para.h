#ifndef __META_MATV_PARA_H_
#define __META_MATV_PARA_H_

#include "FT_Public.h"
#include "meta_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __FT_PRIVATE_H__
typedef struct 
{
	kal_uint32	freq; //khz
	kal_uint8	sndsys;	/* reference sv_const.h, TV_AUD_SYS_T ...*/
	kal_uint8	colsys;	/* reference sv_const.h, SV_CS_PAL_N, SV_CS_PAL,SV_CS_NTSC358...*/
	kal_uint8	flag;
} matv_ch_entry;
#endif

/* MATV Operation enumeration */
typedef enum 
{ 
	FT_MATV_OP_POWER_ON = 0 
	,FT_MATV_OP_POWER_OFF 
	,FT_MATV_OP_SET_REGION 
	,FT_MATV_OP_SCAN 
	,FT_MATV_OP_STOP_SCAN
    ,FT_MATV_OP_GET_CHANNEL_LIST
    ,FT_MATV_OP_CHANGE_CHANNEL
    ,FT_MATV_OP_SET_CHANNEL_PROPERTY
    ,FT_MATV_OP_GET_CHANNEL_QUALITY
    ,FT_MATV_OP_GET_CHANNEL_QUALITY_ALL
    ,FT_MATV_OP_GET_CHIPNAME
    ,FT_MATV_OP_END 
} FT_MATV_CMD_TYPE;

typedef struct 
{ 	
	kal_uint8        		m_ucChannel;
    matv_ch_entry	m_rmatv_ch_entry; 
} FT_MATV_SET_CHANNEL_PROPERTY_REQ_T;

typedef union 
{ 
	kal_uint8	m_ucRegion;
    kal_uint8	m_ucScanMode;
    kal_uint8	m_ucChannel;
    kal_uint8	m_ucItem;
    FT_MATV_SET_CHANNEL_PROPERTY_REQ_T m_rSetChannelProperty; 
} FT_MATV_CMD_U;

typedef struct 
{ 
	FT_H         		header;
    FT_MATV_CMD_TYPE	type;
    FT_MATV_CMD_U		cmd; 
} FT_MATV_REQ;

typedef struct 
{ 
	kal_uint8        		m_ucChannels;
    matv_ch_entry	m_rmatv_ch_entry[70]; 
} FT_MATV_GET_CHANNEL_LIST_CNF_T;

typedef struct 
{ 
	kal_int32	m_i4QualityIndex[128]; 
} FT_MATV_GET_CHANNEL_QUALITY_ALL_CNF_T;

typedef union 
{ 
	kal_int32 m_i4QualityIndex; 
	kal_uint8 m_ucProgress; 
	char chipname[20];
} FT_MATV_CNF_U;

typedef struct
{ 
	FT_H           		header;
    FT_MATV_CMD_TYPE	type;
    kal_uint32         		status;
    FT_MATV_CNF_U  		result; 
} FT_MATV_CNF;

void META_MATV_OP(FT_MATV_REQ *req) ;

#ifdef __cplusplus
}
#endif

#endif


