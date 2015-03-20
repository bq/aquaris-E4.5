

#ifndef __META_GSENSOR_PARA_H__
#define __META_GSENSOR_PARA_H__

#include "FT_Public.h"

typedef enum {
    GS_OP_CALI,
    GS_OP_READ_RAW,
    GS_OP_WRITE_NVRAM,
    GS_OP_READ_NVRAM,
    GS_OP_END
} GS_OP;

typedef struct {
    int num;
    int delay;
    int tolerance;
} GS_CMD_CALI;

typedef struct {  /*16bits -> 1g*/
    int x;
    int y;
    int z; 
} GS_ACK_CALI;

typedef struct {
    int dummy;
} GS_CMD_READ_RAW;

typedef struct {  /*16bits -> 1g*/
    int x;
    int y;
    int z;
} GS_ACK_READ_RAW;

typedef struct {
    int x;
    int y;
    int z;    
} GS_CMD_WRITE_NVRAM;

typedef struct {
    int   dummy;
} GS_ACK_WRITE_NVRAM;

typedef struct {
    int   dummy;
} GS_CMD_READ_NVRAM;

typedef struct {
    int x;
    int y;
    int z;    
} GS_ACK_READ_NVRAM;


typedef union {
    GS_ACK_CALI        cali;
    GS_ACK_READ_RAW    readraw;
    GS_ACK_WRITE_NVRAM writenv;
    GS_ACK_READ_NVRAM  readnv; 
} GS_ACK;

typedef union {
    GS_CMD_CALI        cali;
    GS_CMD_READ_RAW    readraw;    
    GS_CMD_WRITE_NVRAM writenv;
    GS_CMD_READ_NVRAM  readnv;
} GS_CMD;

typedef struct {
    FT_H header;
    GS_OP op;
    GS_CMD cmd; 		
} GS_REQ;

typedef struct {
    FT_H header;
    GS_OP op;
    GS_ACK ack;	
    int gs_err;     /*gs->FT*/
    int status;     /*the status of the operation*/
} GS_CNF;

bool Meta_GSensor_Open(void);
void Meta_GSensor_OP(GS_REQ *req, char *peer_buff, unsigned short peer_len);
bool Meta_GSensor_Close(void);


#endif




