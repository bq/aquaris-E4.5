#include "android/log.h"

#include "FT_Public.h"
#include "meta_common.h"

#include "meta_hdcp_para.h"


#include "Dx_Hdcp_Provisioning.h"

//#include <stdlib.h>
#include <unistd.h> 
#include <errno.h>
#include <private/android_filesystem_config.h>

#undef   LOG_TAG

#define  LOG_TAG  "HDCP_META"
#define  HDCPLOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)


static HDCP_CNF hdcp_cnf;


bool META_HDCP_init()
{
    HDCPLOGI("META_HDCP_init()");
    /* Do nothing */ 
    return true;
}

void META_HDCP_deinit()
{
    HDCPLOGI("META_HDCP_deinit()");
	/* Do nothing */ 
}


void changeKeyFileOwner(void)
{
    const char* dataFile = "/persist/data/dxhdcp2/dxhdcp2/dxhdcp2.sfs";
    int ret = chown(dataFile, AID_MEDIA, AID_MEDIA); 

    if (ret == 0){
        HDCPLOGI("chown() succeed");
    } else {         
        HDCPLOGI("chown() fail. error:[%d] %s", errno, strerror(errno));    
    }     
}

bool startKeyProvisioning(unsigned char *data, int dataLen, unsigned char *cekData, int cekLen)
{
    if (dataLen != 572 || cekLen != 16) {
        HDCPLOGI("keyProvisioning() len is wrong! dataLen:%d, cekLen:%d", dataLen, cekLen);
        return false;
    }    

    DxStatus result = DX_SUCCESS;
    result = DxHdcp_Provisioning_Init();
    if (result != 0)
    {
        HDCPLOGI("DxHdcp_Provisioning_Init failed");
        goto end;
    }
    HDCPLOGI("DxHdcp_Provisioning_Init succeed");

    result = DxHDCP_ProvisionWithCEK(data, dataLen, cekData);   
    if (result != 0)
    {
        HDCPLOGI("DxHdcp_ProvisionWithCEK failed");
        goto end;
    }
    HDCPLOGI("DxHdcp_ProvisionWithCEK succeed");     

    /*
    result = DxHDCP_StoreCEK(cekData);  
    if (result != 0)
    {
        ALOGI("DxHDCP_StoreCEK failed\n");
        goto end;
    }
    ALOGI("DxHDCP_StoreCEK succeed");
    */
    
    result = DxHDCP_ProvisionValidate(data, dataLen, cekData);
    if (result != 0)
    {
        HDCPLOGI("DxHdcp_ProvisionValidate failed");
        goto end;
    }
    HDCPLOGI("DxHdcp_ProvisionValidate succeed");


end:
    DxHdcp_Provisioining_Terminate();

    HDCPLOGI("provisioning test finished with return code: 0x%08x\n",(unsigned int)result);

    /* The file permission is 660 by default.
     * So change file owner and group as media for mediaserver process to R/W
     */
    changeKeyFileOwner();      
     
    return (result == 0) ? true : false;
}


void META_HDCP_OP(HDCP_REQ *req, char *peer_buff, unsigned short peer_len) 
{	
    bool ret;
    
	memset(&hdcp_cnf, 0, sizeof(HDCP_CNF));	
	hdcp_cnf.header.id = FT_HDCP_CNF_ID;
	hdcp_cnf.header.token = req->header.token;
	hdcp_cnf.op = req->op;

    HDCPLOGI("META_HDCP_OP(), op:%d", req->op);
        
    switch(req->op){
        case HDCP_OP_INSTALL:

            ret = startKeyProvisioning(
                req->cmd.hdcp_install_req.data,
                req->cmd.hdcp_install_req.data_len,
                req->cmd.hdcp_install_req.cek_data,
                req->cmd.hdcp_install_req.cek_len);

            // Operation request is accepted successfully
            hdcp_cnf.status = META_SUCCESS;

            // Operation result
            if (ret == true) {
                hdcp_cnf.result.hdcp_install_cnf.install_result = HDCP_CNF_OK;
            } else {
                hdcp_cnf.result.hdcp_install_cnf.install_result = HDCP_CNF_FAIL;
            }             
            
            WriteDataToPC(&hdcp_cnf, sizeof(HDCP_CNF), NULL, 0);
			break;       
            
		
	    default:
			HDCPLOGI("Unsupported OP");
			hdcp_cnf.status = META_FAILED;
			WriteDataToPC(&hdcp_cnf, sizeof(HDCP_CNF), NULL, 0);
			break;
	}  
}



