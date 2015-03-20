#ifndef __META_GPS_PARA_H_
#define __META_GPS_PARA_H_

#include "FT_Public.h"
#include "meta_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The TestCase Enum define of GPS_module */
typedef enum {
	 GPS_OP_OPEN = 0
	,GPS_OP_CLOSE
	,GPS_OP_SEND_CMD
	,GPS_OP_END
} GPS_OP;

/* The PMTK command struct */
typedef struct {
	unsigned int    len;      	
    unsigned char   buff[1024]; 
} GPS_CMD;

/* The PMTK command ACK struct */
typedef struct {
	unsigned int   len;      	
    unsigned char   buff[1024]; 
} GPS_ACK_BUF;

typedef struct {
    FT_H	    header;  //module do not need care it
	GPS_OP		op;
	GPS_CMD		cmd;
} GPS_REQ;

typedef struct {
    FT_H		    header;  //module do not need care it
	GPS_OP			op;
	unsigned int    gps_status;  //gps->FT
	GPS_ACK_BUF     gps_ack;	
	unsigned char	status;
} GPS_CNF;


void META_GPS_OP(GPS_REQ *req, char *peer_buff, unsigned short peer_len) ;
void *GPS_MetaThread(void *arg);

#ifdef __cplusplus
}
#endif

#endif


