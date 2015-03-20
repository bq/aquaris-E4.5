
#include <stdio.h>
#include "meta_lcd.h"

// ---------------------------------------------------------------------------

int main(int argc, const char** args)
{
    LCDFt_REQ req = {2000};
    LCDFt_CNF config;

    if (false == Meta_LCDFt_Init())
    {
        printf(TAG "Meta_LCDFt_Init() fail\n");
        return -1;
    }

    config = Meta_LCDFt_OP(req);

    if (!config.status)
    {
        printf(TAG "Meta_LCDFt_OP() fail\n");
        return -1;
    }

    if (false == Meta_LCDFt_Deinit())
    {
        printf(TAG "Meta_LCDFt_Deinit() fail\n");
        return -1;
    }

    return 0;
}


