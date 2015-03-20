

#ifndef __META_VIBRATOR_PARA_H__
#define __META_VIBRATOR_PARA_H__

#include <meta_common.h>

enum LEDNUM {
	LEDNUM_VIBRATOR = 0, // 0
	LEDNUM_RED,          // 1
	LEDNUM_GREEN,        // 2
	LEDNUM_BLUE,         // 3
	LEDNUM_KEYPAD        // 4
};
 
typedef struct{
	unsigned int LedNum; // 0 - Vibrator, 1,2,3 - LED, 4 - keypad led 
	int onoff;           // 0 == off, 1 == on, 2 == blink
}NLED_REQ;

typedef struct{
	BOOL		status;
}NLED_CNF;


/* please implement this function in META_KEYPADBK.LIB  */
BOOL 			Meta_Vibrator_Init(void);
NLED_CNF	 	Meta_Vibrator_OP(NLED_REQ req);
BOOL 			Meta_Vibrator_Deinit(void);


#endif


