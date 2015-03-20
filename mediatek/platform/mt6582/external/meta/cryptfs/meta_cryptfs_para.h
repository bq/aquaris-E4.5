#ifndef __META_CRYPTFS_H_
#define __META_CRYPTFS_H_


#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
     CRYPTFS_OP_QUERY_STATUS = 0,
     CRYPTFS_OP_VERIFY,
     CRYPTFS_END
}CRYPTFS_OP;

typedef struct
{       
    unsigned char  sign;   // No means
} CRYPTFS_QUERY_STATUS_REQ;

typedef struct
{       
  unsigned char   status;    // 1 means encrypted; 0 means decrypted or not supported
} CRYPTFS_QUERY_STATUS_CNF;

typedef struct
{
   unsigned char pwd[32];
   int  length;
} CRYPTFS_VERIFY_REQ;

typedef struct
{       
   unsigned char   decrypt_result;   // 1 means decrypt successfully, 0 means decrypt fail
} CRYPTFS_VERIFY_CNF;


typedef union 
{
   CRYPTFS_QUERY_STATUS_CNF     query_status_cnf;
   CRYPTFS_VERIFY_CNF   verify_cnf;
} FT_CRYPTFS_RESULT;

typedef union 
{
    CRYPTFS_QUERY_STATUS_REQ     query_statust_req;
    CRYPTFS_VERIFY_REQ   verify_req;
} FT_CRYPTFS_CMD;

typedef struct {
     FT_H	    header;
     CRYPTFS_OP     op;
     unsigned char  m_status;   // The data frame state, 0 means normal
     FT_CRYPTFS_RESULT  result;
} FT_CRYPTFS_CNF;

typedef struct {
    FT_H	       header;  
    CRYPTFS_OP    op;
    FT_CRYPTFS_CMD  cmd;
} FT_CRYPTFS_REQ;

bool META_CRYPTFS_init();
void META_CRYPTFS_deinit();
void META_CRYPTFS_OP(FT_CRYPTFS_REQ *req);

#ifdef __cplusplus
};
#endif

#endif





