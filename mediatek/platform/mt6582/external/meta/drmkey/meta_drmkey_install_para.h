
#ifndef __META_DRMKEY_INSTALL_H_
#define __META_DRMKEY_INSTALL_H_

#include "FT_Public.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FT_DRMKEY_INSTALL_SET = 0,
    FT_DRMKEY_INSTALL_QUERY,
    FT_DRMKEY_INSTALL_END
} FT_DRMKEY_INSTALL_OP;

#define KEY_BLK_CREATE	0x01
#define KEY_BLK_WRITE	0x02
#define KEY_BLK_EOF		0x04

// REQ
typedef struct
{
    unsigned int    file_size;
    unsigned char   stage;
} FT_DRMKEY_INSTALL_SET_REQ;

typedef struct
{
    unsigned int req;  // unused
} FT_DRMKEY_INSTALL_QUERY_REQ;

typedef union
{
    FT_DRMKEY_INSTALL_SET_REQ   set_req;
    FT_DRMKEY_INSTALL_QUERY_REQ query_req;
} FT_DRMKEY_INSTALL_CMD_U;

typedef struct {
    FT_H	                 header;  
    FT_DRMKEY_INSTALL_OP     op;
    FT_DRMKEY_INSTALL_CMD_U  cmd;
} FT_DRMKEY_INSTALL_REQ;


// Confirm
typedef struct {
    unsigned int result;
} DRMKEY_INSTALL_SET_CNF;

typedef struct{
	unsigned int  keycount;
	unsigned int  keytype[512];
} DRMKEY_INSTALL_QUERY_CNF;

typedef union {
    DRMKEY_INSTALL_SET_CNF        set_cnf;
	DRMKEY_INSTALL_QUERY_CNF      keyresult;
} FT_DRMKEY_INSTALL_CNF_U;

typedef enum {
    DRMKEY_INSTALL_OK = 0,
    DRMKEY_INSTALL_FAIL = -1,
    DRMKEY_INSTALL_VERIFY_FAIL = -2
} FT_DRMKEY_INSTALL_RESULT;

typedef struct {
    FT_H	                  header;
    FT_DRMKEY_INSTALL_OP      op;
    FT_DRMKEY_INSTALL_RESULT  status;
    FT_DRMKEY_INSTALL_CNF_U   result;
} FT_DRMKEY_INSTALL_CNF;

void META_DRMKEY_INSTALL_OP(FT_DRMKEY_INSTALL_REQ *req, char *peer_buff, unsigned short peer_len);

#ifdef __cplusplus
};
#endif

#endif



