
#include <stdio.h>
#include "meta_pmic.h"

// ---------------------------------------------------------------------------

int main(int argc, const char** args)
{
    FT_PMIC_REG_READ req_read;
	FT_PMIC_REG_READ_CNF config_read;
	FT_PMIC_REG_WRITE req_write;
	FT_PMIC_REG_WRITE_CNF config_write;
	
	printf("Meta Test pmic AP test : START\n");

	if (false == Meta_Pmic_Init())
    {
        printf("Meta_Pmic_Init() fail\n");
        return -1;
    }

	req_read.m_rWmCmdReadReq.registernumber=0x0;
	config_read=META_PMICR_OP(&req_read);
	if (!config_read.m_rWmCmdReadResult.status)
    {
        printf("META_PMICR_OP() fail\n");
        return -1;
    }

	req_write.m_rWmCmdWriteReq.registernumber=0x5D;
	req_write.m_rWmCmdWriteReq.registervalue=0x0;
	config_write=META_PMICW_OP(&req_write);
	if (!config_write.m_rWmCmdWriteResult.status)
    {
        printf("META_PMICW_OP() fail\n");
        return -1;
    }

	req_write.m_rWmCmdWriteReq.registernumber=0x5D;
	req_write.m_rWmCmdWriteReq.registervalue=0x8;
	config_write=META_PMICW_OP(&req_write);
	if (!config_write.m_rWmCmdWriteResult.status)
    {
        printf("META_PMICW_OP() fail\n");
        return -1;
    }

	if (false == Meta_Pmic_Deinit())
    {
        printf("Meta_Pmic_Deinit() fail\n");
        return -1;
    }

	printf("Meta Test pmic AP test : END\n");
	
    return 0;
}


