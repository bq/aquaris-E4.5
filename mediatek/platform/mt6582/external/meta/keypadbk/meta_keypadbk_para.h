

#ifndef __META_KEYPADBK_PARA_H__
#define __META_KEYPADBK_PARA_H__

#include <meta_common.h>

 
typedef struct{
	BOOL			onoff;
	BYTE			DIV;
	BYTE			DUTY;
}KeypadBK_REQ;

typedef struct{
	BOOL			status;
}KeypadBK_CNF;


/* please implement this function in META_KEYPADBK.LIB  */
BOOL 			Meta_KeypadBK_Init(void);
KeypadBK_CNF 	Meta_KeypadBK_OP(KeypadBK_REQ req);
BOOL 			Meta_KeypadBK_Deinit(void);


#endif


