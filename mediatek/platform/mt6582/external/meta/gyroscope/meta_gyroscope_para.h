

#ifndef __META_GYROSCOPE_PARA_H__
#define __META_GYROSCOPE_PARA_H__

#include "FT_Public.h"

typedef enum {
    GYRO_OP_CALI,
    GYRO_OP_READ_RAW,
    GYRO_OP_WRITE_NVRAM,
    GYRO_OP_READ_NVRAM,
    GYRO_OP_END
} GYRO_OP;

typedef struct {
    int num;
    int delay;
    int tolerance;
} GYRO_CMD_CALI;

typedef struct {  /*16bits -> 1g*/
    int x;
    int y;
    int z; 
} GYRO_ACK_CALI;

typedef struct {
    int dummy;
} GYRO_CMD_READ_RAW;

typedef struct {  /*16bits -> 1g*/
    int x;
    int y;
    int z;
} GYRO_ACK_READ_RAW;

typedef struct {
    int x;
    int y;
    int z;    
} GYRO_CMD_WRITE_NVRAM;

typedef struct {
    int   dummy;
} GYRO_ACK_WRITE_NVRAM;

typedef struct {
    int   dummy;
} GYRO_CMD_READ_NVRAM;

typedef struct {
    int x;
    int y;
    int z;    
} GYRO_ACK_READ_NVRAM;


typedef union {
    GYRO_ACK_CALI        cali;
    GYRO_ACK_READ_RAW    readraw;
    GYRO_ACK_WRITE_NVRAM writenv;
    GYRO_ACK_READ_NVRAM  readnv; 
} GYRO_ACK;

typedef union {
    GYRO_CMD_CALI        cali;
    GYRO_CMD_READ_RAW    readraw;    
    GYRO_CMD_WRITE_NVRAM writenv;
    GYRO_CMD_READ_NVRAM  readnv;
} GYRO_CMD;

typedef struct {
    FT_H header;
    GYRO_OP op;
    GYRO_CMD cmd; 		
} GYRO_REQ;

typedef struct {
    FT_H header;
    GYRO_OP op;
    GYRO_ACK ack;	
    int gyro_err;     /*gs->FT*/
    int status;     /*the status of the operation*/
} GYRO_CNF;

bool Meta_Gyroscope_Open(void);
void Meta_Gyroscope_OP(GYRO_REQ *req, char *peer_buff, unsigned short peer_len);
bool Meta_Gyroscope_Close(void);


#endif




