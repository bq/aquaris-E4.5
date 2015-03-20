

#ifndef __META_LCDBK_PARA_H__
#define __META_LCDBK_PARA_H__

 

typedef struct  {
	//unsigned char		lcd_light_level;
	int		lcd_light_level;
}LCDLevel_REQ;

typedef struct  {
	BOOL 				status;
}LCDLevel_CNF;


/*implement this function in META_LCDBK.LIB  */
BOOL			Meta_LCDBK_Init();
LCDLevel_CNF	Meta_LCDBK_OP(LCDLevel_REQ dwBrightness);
BOOL			Meta_LCDBK_Deinit();




#endif



