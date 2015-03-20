
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "meta_battery.h"
#include "WM2Linux.h"

BOOL Meta_Battery_Init()
{
	printf("Meta_Battery_Init ! \n");
	META_LOG("Meta_Battery_Init ! \n");
	return true;
}

BOOL Meta_Battery_Deinit(){
	printf("Meta_Battery_Deinit ! \n");
	META_LOG("Meta_Battery_Deinit ! \n");
	return true;
}

void Meta_Battery_OP(FT_BATT_REQ* req, BYTE* peer_buf,unsigned short peer_len)
{
	/* dummy code */
	
	FT_BATT_CNF batcnf;
	
	memset(&batcnf, 0, sizeof(batcnf));

	META_LOG("Meta_Battery_OP");
	
	batcnf.header.id=req->header.id+1;
	batcnf.header.token=req->header.token;
	batcnf.status=META_SUCCESS;
	batcnf.type=req->type;
	batcnf.result.m_rWmCmdWriteResult.DL_Status=BAT_FILE_Success;

	if (peer_buf==NULL) {
		printf("peer_buf is NULL \n");
		META_LOG("peer_buf is NULL \n");
		return;
	}

	switch (req->cmd.m_rWmCmdWriteReq.nReqWriteFileStatus)
	{
		case BAT_FILE_ONCE:
			batcnf.result.m_rWmCmdWriteResult.nCnfWriteFileStatus=BAT_FILE_ONCE;
			break;
		case BAT_FILE_START:
			batcnf.result.m_rWmCmdWriteResult.nCnfWriteFileStatus=BAT_FILE_START;
			break;	
		case BAT_FILE_ONGOING:
			batcnf.result.m_rWmCmdWriteResult.nCnfWriteFileStatus=BAT_FILE_ONGOING;
			break;	
		case BAT_FILE_CLOSE:
			batcnf.result.m_rWmCmdWriteResult.nCnfWriteFileStatus=BAT_FILE_CLOSE;
			break;	
	}

	WriteDataToPC(&batcnf, sizeof(batcnf), NULL, 0);	
}

void Meta_Battery_Read_FW(FT_BATT_READ_INFO_REQ *req)
{
	/* dummy code */

	FT_BATT_READ_INFO_CNF batreadcnf;

	memset(&batreadcnf,0, sizeof(batreadcnf));

	META_LOG("Meta_Battery_Read_FW");

	batreadcnf.header.id=req->header.id+1;
	batreadcnf.header.token=req->header.token;
	batreadcnf.status=META_SUCCESS;
	batreadcnf.type=req->type;
	
	batreadcnf.result.m_rWmFwReadResult.Drv_Status=BAT_READ_INFO_SUCCESS;
	batreadcnf.result.m_rWmSocReadResult.Drv_Status=BAT_READ_INFO_SUCCESS;
	batreadcnf.result.m_rWmSocWriteResult.Drv_Status=BAT_READ_INFO_SUCCESS;

	WriteDataToPC(&batreadcnf, sizeof(batreadcnf), NULL, 0);
}

void Meta_Battery_UPdate_FW(FT_BATT_REQ* req,  BYTE* bDataAddress,unsigned short data_number)
{
	/* dummy code */

	FT_BATT_CNF batcnf;

	memset(&batcnf, 0, sizeof(batcnf));

	META_LOG("Meta_Battery_UPdate_FW");

	batcnf.header.id=req->header.id+1;
	batcnf.header.token=req->header.token;
	batcnf.status=META_SUCCESS;
	batcnf.type=req->type;
	
	batcnf.result.m_rWmCmdUpdateResult.nCnfUpdateStatus=BAT_FILE_Success;

	switch (req->cmd.m_rWmCmdUpdateReq.nReqStartStatus)
	{
		case BAT_FILE_ONCE:
			batcnf.result.m_rWmCmdUpdateResult.nCnfStartStatus=BAT_FILE_ONCE;
			break;	
		case BAT_FILE_START:
			batcnf.result.m_rWmCmdUpdateResult.nCnfStartStatus=BAT_FILE_START;
			break;	
		case BAT_FILE_ONGOING:
			batcnf.result.m_rWmCmdUpdateResult.nCnfStartStatus=BAT_FILE_ONGOING;
			break;	
		case BAT_FILE_CLOSE:
			batcnf.result.m_rWmCmdUpdateResult.nCnfStartStatus=BAT_FILE_CLOSE;
			break;	
	}

	WriteDataToPC(&batcnf, sizeof(batcnf), NULL, 0);	
}



