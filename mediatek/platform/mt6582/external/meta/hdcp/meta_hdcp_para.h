
#ifndef __META_HDCP_H_
#define __META_HDCP_H_

#include "FT_Public.h"


#define HDCP_CNF_OK     0
#define HDCP_CNF_FAIL   1

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * OP 
 */
typedef enum {
    HDCP_OP_INSTALL = 0
   ,HDCP_END
} HDCP_OP;


/*
 * Install REQ & CNF structure 
 */
typedef struct {
    unsigned char data[572];
    unsigned char cek_data[16];
    unsigned int data_len;
    unsigned int cek_len;
} HDCP_INSTALL_REQ_T;

typedef struct {
    unsigned int install_result;
} HDCP_INSTALL_CNF_T;


/* 
 * HDCP REQ & CNF union 
 */
typedef union {
    HDCP_INSTALL_REQ_T hdcp_install_req;
} META_HDCP_CMD_U;

typedef union {
    HDCP_INSTALL_CNF_T hdcp_install_cnf;
} META_HDCP_CNF_U;


/* 
 * HDCP REQ & CNF 
 */
typedef struct {
    FT_H            header;  //module do not need care it
    HDCP_OP         op;
    META_HDCP_CMD_U cmd;
} HDCP_REQ;

typedef struct {
    FT_H            header;  //module do not need care it
    HDCP_OP         op;
    META_HDCP_CNF_U result;   
    unsigned int    status;
} HDCP_CNF;



bool META_HDCP_init();
void META_HDCP_deinit();
void META_HDCP_OP(HDCP_REQ *req, char *peer_buff, unsigned short peer_len) ;

#ifdef __cplusplus
};
#endif

#endif


