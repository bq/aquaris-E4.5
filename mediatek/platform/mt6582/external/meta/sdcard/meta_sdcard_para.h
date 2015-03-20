

#ifndef __META_SDCARD_PARA_H__
#define __META_SDCARD_PARA_H__

#include "FT_Public.h"
 
typedef struct{
	FT_H	     header;  //module do not need care it
	unsigned int dwSDHCIndex;	// MSDC1/2/3. if dwSDHCIndex no equal 1/2/3 auto test.
}SDCARD_REQ;

typedef struct{
	FT_H	            header;  //module do not need care it
	unsigned char       DatBusWidth;
	bool                IsSecuredMode;
	unsigned short		CardTpye;
	unsigned int        SizeOfProtectErea;
	unsigned char       SpeedClass;
	unsigned char       Performance_move;
	unsigned char       AUSize;
	unsigned char		status;	
}SDCARD_CNF;

typedef enum
{
    Card_Standard=0,
    Card_HighCap
} CardCapType;

/* please implement this function   */
bool Meta_SDcard_Init(SDCARD_REQ *req);
void Meta_SDcard_OP(SDCARD_REQ *req, char *peer_buff, unsigned short peer_len); 
bool Meta_SDcard_Deinit(void);

#endif


