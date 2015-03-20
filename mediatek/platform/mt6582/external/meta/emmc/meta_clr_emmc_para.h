#ifndef __META_CLR_EMMC_H_
#define __META_CLR_EMMC_H_


#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
     FT_EMMC_OP_CLEAR = 0,
     FT_EMMC_OP_FORMAT_TCARD,
     FT_EMMC_OP_CLEAR_WITHOUT_TCARD,
     FT_EMMC_END
}FT_EMMC_OP;

typedef struct {       
    unsigned char   status;   // The operation whether success , 1 means success
} EMMC_CLEAR_CNF;

typedef struct {
    unsigned char status;
}EMMC_CLEAR_WITHOUT_TCARD_CNF;

typedef struct
{       
  unsigned char   status;   // The operation whether success , 1 means success
} EMMC_FORMAT_TCARD_CNF;

typedef union {
  EMMC_CLEAR_CNF     clear_cnf;
  EMMC_FORMAT_TCARD_CNF   form_tcard_cnf;
  EMMC_CLEAR_WITHOUT_TCARD_CNF clear_without_tcard_cnf;
} FT_EMMC_RESULT;

typedef struct {       
    unsigned char   sign;   // No means
} EMMC_CLEAR_REQ;

typedef struct
{
  unsigned char   sign;   // No means
} EMMC_FORMAT_TCARD_REQ;

typedef struct {
    unsigned char   sign;
} EMMC_CLEAR_WITHOUT_TCARD_REQ;

typedef union {
  EMMC_CLEAR_REQ     clear_req;
  EMMC_FORMAT_TCARD_REQ   format_tcard_req;
  EMMC_CLEAR_WITHOUT_TCARD_REQ clear_without_tcard_req;
} FT_EMMC_CMD;

typedef struct {
    FT_H	       header;  
    FT_EMMC_OP	       op;
    FT_EMMC_CMD  cmd;
} FT_EMMC_REQ;

typedef struct {
     FT_H	    header;
     FT_EMMC_OP	    op;
     unsigned char  m_status;   // The data frame state, 0 means normal
     FT_EMMC_RESULT  result;
} FT_EMMC_CNF;

bool META_CLR_EMMC_init();
void META_CLR_EMMC_deinit();
void META_CLR_EMMC_OP(FT_EMMC_REQ *req) ;

#ifdef __cplusplus
};
#endif

#endif



