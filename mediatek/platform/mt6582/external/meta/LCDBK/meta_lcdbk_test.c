
#include <stdio.h>
#include "meta_lcdbk.h"

// ---------------------------------------------------------------------------

int main(int argc, const char** args)
{
    LCDLevel_REQ req_brightness;
	LCDLevel_CNF config_brightness;
	int tmp;
	
	printf("Meta Test lcdbk AP test : START\n");

	if (false == Meta_LCDBK_Init())
    {
        printf("Meta_LCDBK_Init() fail\n");
        return -1;
    }

	/* dark -> bright --------------------------*/	
	for(tmp=9;tmp>=0;tmp--)
	{
		//printf("Test AP : set lcd_light_level = %d\n", tmp);
		req_brightness.lcd_light_level=tmp;
		config_brightness=Meta_LCDBK_OP(req_brightness);
		if (!config_brightness.status)
	    {
	        printf("Meta_LCDBK_OP() fail\n");
	        return -1;
	    }
	}

	for(tmp=0;tmp<10;tmp++)
	{
		//printf("Test AP : set lcd_light_level = %d\n", tmp);
		req_brightness.lcd_light_level=tmp;
		config_brightness=Meta_LCDBK_OP(req_brightness);
		if (!config_brightness.status)
	    {
	        printf("Meta_LCDBK_OP() fail\n");
	        return -1;
	    }
	}
	/* dark -> bright --------------------------*/

	if (false == Meta_LCDBK_Deinit())
    {
        printf("Meta_LCDBK_Deinit() fail\n");
        return -1;
    }

	printf("Meta Test lcdbk AP test : END\n");
	
    return 0;
}


