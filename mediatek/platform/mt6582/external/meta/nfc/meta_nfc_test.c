

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "WM2Linux.h"


#include "meta_nfc_para.h"


static void nfc_info_callback(NFC_CNF *cnf, void *buf, unsigned int size)
{
    META_LOG("Enter nfc_info_callback\n");

}

int main(int argc, const char** argv)
{
    NFC_REQ req;

    memset(&req, 0, sizeof(NFC_REQ));

    META_NFC_Register(nfc_info_callback);

    if (META_NFC_init() != 0) {
        META_LOG("META_NFC_init Failure\n");
        return -1;
    }
    sleep(1);

    nfc_cnf.header.id = FT_NFC_CNF_ID;
    nfc_cnf.header.token = req->header.token;
    nfc_cnf.op = req->op;

    
    req.cmd.m_setting_req.debug_enable = 1;
    req.cmd.m_setting_req.get_capabilities = 1;
    req.cmd.m_setting_req.nfc_enable = 1;
    req.cmd.m_setting_req.sw_protocol = 1;
    
    req.header.id = 1;
    req.header.token = 1;
    req.op = NFC_OP_SETTING;
    META_NFC_OP(&req, NULL, 0);

    sleep(10);

    META_NFC_OP(&req, NULL, 0);
    
    sleep(1);
    
    META_NFC_deinit();
    META_NFC_Register(NULL);
    
    return 0;
}



