
#include <stdio.h>
#include "meta_adc.h"

// ---------------------------------------------------------------------------

int main(int argc, const char** args)
{
	AUXADC_CNF config_adc;
	AUXADC_REQ req_adc;
	char peer_buff[100]={0};
	unsigned short peer_len=0;

   	printf("Meta Test ADC AP test : START\n");

	if (false == Meta_AUXADC_Init())
    {
        printf("Meta_AUXADC_Init() fail\n");
        return -1;
    }

	req_adc.dwChannel=3;
	req_adc.dwCount=5;
	Meta_AUXADC_OP(&req_adc, peer_buff, peer_len);
	
	if (false == Meta_AUXADC_Deinit())
    {
        printf("Meta_AUXADC_Deinit() fail\n");
		
        return -1;
    }

	printf("Meta Test ADC AP test : END\n");
	
    return 0;
}


