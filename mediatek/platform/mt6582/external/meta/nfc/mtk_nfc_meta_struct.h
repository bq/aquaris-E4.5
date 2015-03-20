


#ifndef MTK_NFC_META_STRUCT_H
#define MTK_NFC_META_STRUCT_H

#include "./../../../external/mtknfc/inc/mtk_nfc_sys_type_ext.h"


//--------------------------
// STRUCT "META_NFC_CMD_U"
//--------------------------
typedef struct mtk_nfc_meta_als_readerm_req {
    UINT32 action;          /* Action, please refer ENUM of EM_ACTION*/
    UINT32 supporttype;     /* supporttype, please refer BITMAP of EM_ALS_READER_M_TYPE*/
    UINT32 typeA_datarate;  /* TypeA,datarate, please refer BITMAP of EM_ALS_READER_M_SPDRATE*/
    UINT32 typeB_datarate;  /* TypeB,datarate, please refer BITMAP of EM_ALS_READER_M_SPDRATE*/
    UINT32 typeV_datarate;  /* TypeV,datarate, please refer BITMAP of EM_ALS_READER_M_SPDRATE*/
    UINT32 typeF_datarate;  /* TypeF,datarate, please refer BITMAP of EM_ALS_READER_M_SPDRATE*/    
    UINT32 typeV_subcarrier;/* 0: subcarrier, 1 :dual subcarrier*/
} s_mtk_nfc_meta_als_readerm_req;

typedef struct mtk_nfc_meta_als_cardm_req {
    UINT32 action;          /* Action, please refer ENUM of EM_ACTION*/
    UINT32 SWNum;           /* SWNum, please refer BITMAP of EM_ALS_CARD_M_SW_NUM*/
    UINT32 supporttype;     /* supporttype, please refer BITMAP of EM_ALS_READER_M_TYPE*/
    UINT32 fgvirtualcard;   /* 1:enable virtual card, 0:disable virtual card(default)   */
} s_mtk_nfc_meta_als_cardm_req;

typedef struct mtk_nfc_meta_als_p2p_req {
    UINT32 action;          /* Action, please refer ENUM of EM_ACTION*/
    UINT32 supporttype;     /* supporttype, please refer BITMAP of EM_ALS_READER_M_TYPE*/
    UINT32 typeA_datarate;  /* TypeA,datarate, please refer BITMAP of EM_ALS_READER_M_SPDRATE*/
    UINT32 typeF_datarate;  /* TypeV,datarate, please refer BITMAP of EM_ALS_READER_M_SPDRATE*/
    UINT32 mode;            /* BITMAPS bit0: Passive mode, bit1: Active mode, please refer BITMAP of EM_P2P_MODE*/
    UINT32 role;            /* BITMAPS bit0: Initator, bit1: Target, please refer BITMAP of EM_P2P_ROLE*/
    UINT32 isDisableCardM;  /* 0: , 1: disable card mode*/
} s_mtk_nfc_meta_als_p2p_req;

typedef struct mtk_nfc_meta_polling_req {
    UINT32 action;          /* Action, please refer ENUM of EM_ACTION*/
    UINT32 phase;           /* 0:Listen phase, 1:Pause phase*/
    UINT32 Period;
    UINT32 enablefunc;      /* enablefunc, please refer BITMAP of EM_ENABLE_FUNC*/
    s_mtk_nfc_meta_als_p2p_req     p2pM;
    s_mtk_nfc_meta_als_cardm_req   cardM;
    s_mtk_nfc_meta_als_readerm_req readerM;
} s_mtk_nfc_meta_polling_req;

typedef struct mtk_nfc_meta_tx_carr_als_on_req {
    UINT32 action;          /* Action, please refer ENUM of EM_ACTION*/
} s_mtk_nfc_meta_tx_carr_als_on_req;

typedef struct mtk_nfc_meta_virtual_card_req {
    UINT32 action;          /* Action, please refer ENUM of EM_ACTION*/
    UINT32 supporttype;     /* supporttype, please refer BITMAP of EM_ALS_READER_M_TYPE*/
    //UINT32 typeA_datarate;  /* TypeA,datarate, please refer BITMAP of EM_ALS_READER_M_SPDRATE*/
    //UINT32 typeB_datarate;  /* TypeB,datarate, please refer BITMAP of EM_ALS_READER_M_SPDRATE*/
    UINT32 typeF_datarate;  /* TypeF,datarate, please refer BITMAP of EM_ALS_READER_M_SPDRATE*/ 
} s_mtk_nfc_meta_virtual_card_req;

#if 0
typedef struct mtk_nfc_meta_pnfc_req {
    UINT32 action;          /* Action, please refer ENUM of EM_ACTION*/
    UINT32 datalen;
    UINT8  data[256];
} s_mtk_nfc_meta_pnfc_req;
#endif

typedef struct mtk_nfc_meta_pnfc_req {
    //UINT32 u4ReqMsg;        /* MTK_NFC_MESSAGE_TYPE*/
    //UINT32 u4action;
    s_mtk_nfc_em_pnfc_req rEmPnfcReq;
} s_mtk_nfc_meta_pnfc_req;
  


typedef struct s_mtk_nfc_meta_test_mode_Setting_req
{
    UINT16   forceDownLoad; /*1:enable, 0:disable*/
    UINT16   TagAutoPresenceChk; /*1:enable, 0:disable*/
}s_mtk_nfc_meta_test_mode_Setting_req_t;

typedef struct s_mtk_nfc_meta_loopback_test_req
{
    CHAR   action; /*0:Start, 1:stop*/
}s_mtk_nfc_meta_loopback_test_req_t;


typedef struct mtk_nfc_meta_swp_test_req { 
    INT32  action;
    INT32  SEmap; // bit 0 - SIM1 , bit 1 - SIM2, bit2 - uSD
} s_mtk_nfc_meta_swp_test_req;

typedef struct Text_meta {
    UINT16 DataLength;
    UINT8  data[TAG_WRITE_MAXDATA];
} Text_meta_t;

typedef struct URL_meta {
    UINT16 URLLength;
    UINT8  URLData[64];
} URL_meta_t;


typedef union mtk_nfc_meta_tag_write_ndef_data {
    //SmartPoster_t  SP_Data;
    //Vcard_t        VC_Data;
    Text_meta_t         TX_Data;
    URL_meta_t          URL_Data;
    //EXTTag_t       EXT_Data;
} s_mtk_nfc_meta_tag_write_ndef_data;


typedef struct mtk_nfc_meta_tag_write_ndef {
    UINT32                          length;
    e_mtk_nfc_ndef_type             ndef_type;
    e_mtk_nfc_ndef_lang_type        language;
    s_mtk_nfc_meta_tag_write_ndef_data   ndef_data;
} s_mtk_nfc_meta_tag_write_ndef;

typedef struct mtk_nfc_meta_als_readerm_opt_req {
    INT32  action;          /* Action, please refer ENUM of EM_OPT_ACTION*/   
    s_mtk_nfc_meta_tag_write_ndef ndef_write;
} s_mtk_nfc_meta_als_readerm_opt_req;
//
//

//--------------------------
// STRUCT "META_NFC_CNF_U"
//--------------------------
typedef struct mtk_nfc_meta_als_readerm_rsp {
    INT32  result;          /* 0:Success, 1: Fail,*/
} s_mtk_nfc_meta_als_readerm_rsp;


typedef struct mtk_nfc_meta_tag_read_ndef {
   UINT32 length;    
   e_mtk_nfc_ndef_type ndef_type; 
   UINT8  recordId[32]; 
   UINT8  lang[3];
   UINT8  recordFlags;
   UINT8  recordTnf;
   UINT8  data[NDEF_DATA_LEN];
} s_mtk_nfc_meta_tag_read_ndef;


typedef struct mtk_nfc_meta_als_readerm_opt_rsp {
    INT32  result;          /* 0:Success,1:Fail*/
    s_mtk_nfc_meta_tag_read_ndef ndef_read;
} s_mtk_nfc_meta_als_readerm_opt_rsp;

typedef struct mtk_nfc_meta_als_cardm_rsp {
    INT32  result;          /*0:Success,1:Fail,0xE1:No SIM*/
} s_mtk_nfc_meta_als_cardm_rsp;

typedef struct mtk_nfc_meta_als_p2p_rsp {
    INT32  result;          /* 0:Success,1:Fail*/
} s_mtk_nfc_meta_als_p2p_rsp;

typedef struct mtk_nfc_meta_polling_rsp {
    INT32  result;          /* 0:Success,1:Fail*/
} s_mtk_nfc_meta_polling_rsp;


typedef struct mtk_nfc_meta_tx_carr_als_on_rsp {
    UINT32 result;          /* 0:Success,1:Fail*/
}s_mtk_nfc_meta_tx_carr_als_on_rsp;


typedef struct mtk_nfc_meta_virtual_card_rsp {
    UINT32 result;          /* 0:Success,1:Fail*/
} s_mtk_nfc_meta_virtual_card_rsp;


typedef struct mtk_nfc_meta_pnfc_new_rsp {
    UINT32 result;          /* 0:Success,1:Fail*/
    UINT32 datalen;
    UINT8  data[256];
} s_mtk_nfc_meta_pnfc_new_rsp;


typedef struct s_mtk_nfc_meta_test_mode_Setting_rsp
{
    INT32   result; /*0:success, 1:fail*/
}s_mtk_nfc_meta_test_mode_Setting_rsp_t;


typedef struct s_mtk_nfc_meta_loopback_test_rsp
{
    INT32   result; /*0:success, 1:fail*/
}s_mtk_nfc_meta_loopback_test_rsp_t;

typedef struct mtk_nfc_meta_swp_test_rsp {
    INT32  result;
} s_mtk_nfc_meta_swp_test_rsp;


typedef struct s_mtk_nfc_meta_sw_Version_rsp
{
    UINT16 fw_ver;
    UINT16 hw_ver;
    CHAR   mw_ver[20];
}s_mtk_nfc_meta_sw_Version_rsp_t;



typedef struct mtk_nfc_meta_tool_se_info {
    UINT32            seid;
    SE_STATUS         status;        /* 1: enable, 0 : disable*/
    SE_TYPE           type; 
    SE_CONNECT_TYPE   connecttype;
    SE_STATUS         lowpowermode;  /* 1: enable low power mode, 0 : disable*/
    SE_STATUS         pbf;           /* Each SE current status of Power by field*/     
} s_mtk_nfc_meta_tool_se_info ;


typedef struct mtk_nfc_meta_se_get_list_rsp{
    UINT32   status;          
    s_mtk_nfc_tool_se_info    SeInfor[MTK_NFC_MAX_SE_NUM];
    UINT8                     SeCount; 
}s_mtk_nfc_meta_se_get_list_rsp_t;


typedef struct mtk_nfc_meta_als_readerm_ntf {
    INT32  result;          /* 0:Success,Tag connected, 1: Fail, 2:Tag disconnected*/
    UINT32 isNDEF;          /* 1:NDEF, 0: Non-NDEF*/
    UINT32 UidLen;
    UINT8  Uid[10];
} s_mtk_nfc_meta_als_readerm_ntf;

typedef struct mtk_nfc_meta_als_p2p_ntf {
    INT32  link_status;     /* 1:llcp link is up,0:llcp link is down*/
    //UINT32  datalen;
    //UINT8 data[256];
} s_mtk_nfc_meta_als_p2p_ntf;


typedef union {
    s_mtk_nfc_meta_als_p2p_ntf     p2p;
    s_mtk_nfc_meta_als_cardm_rsp   card;
    s_mtk_nfc_meta_als_readerm_ntf reader;
} s_mtk_nfc_meta_polling_func_ntf;



typedef struct mtk_nfc_meta_polling_ntf {
    INT32 detecttype;       /* enablefunc, please refer ENUM of EM_ENABLE_FUNC*/
    s_mtk_nfc_meta_polling_func_ntf ntf;
} s_mtk_nfc_meta_polling_ntf;




#endif /*MTK_NFC_META_STRUCT_H*/







