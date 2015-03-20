
#ifndef __META_TOUCH_PARA_H__
#define __META_TOUCH_PARA_H__

#include "FT_Public.h"
 
typedef struct{
	FT_H	    	header;  //module do not need care it
	unsigned char	tpd_type;	
}Touch_REQ;

typedef struct{
	FT_H	    header;  //module do not need care it
	unsigned char	tpd_type;
	bool			status;
}Touch_CNF;


/* please implement this function   */
BOOL 			Meta_Touch_Init(void);
void Meta_Touch_OP(Touch_REQ *req, char *peer_buff, unsigned short peer_len); 
BOOL 			Meta_Touch_Deinit(void);


#endif



