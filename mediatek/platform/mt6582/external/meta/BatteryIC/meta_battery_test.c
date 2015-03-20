
#include <stdio.h>
#include "meta_battery.h"

// ---------------------------------------------------------------------------

int main(int argc, const char** args)
{    
	printf("Meta Test Smart Battery IC AP test : START\n");

	if (false == Meta_Battery_Init())
    {
        printf("Meta_Battery_Init() fail\n");
        return -1;
    }
	
	if (false == Meta_Battery_Deinit())
    {
        printf("Meta_Battery_Deinit() fail\n");
        return -1;
    }

	printf("Meta Test Smart Battery IC AP test : END\n");
	
    return 0;
}


