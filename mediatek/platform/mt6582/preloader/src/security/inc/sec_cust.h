
#ifndef SEC_CUST_H
#define SEC_CUST_H

/**************************************************************************
 *  CUSTOMER INTERFACE
 **************************************************************************/
/* this data structure will be sent to 
   SecLib for security feature initialization */
#define CUSTOM_SEC_CFG_SIZE     (352)
typedef struct _CUSTOM_SEC_CFG
{
    /* feature configuration */
    unsigned int                sec_usb_dl;    
    unsigned int                sec_boot;
    unsigned int                reserve[8];

    /* RSA key for S-USBDL */
    unsigned char               img_auth_rsa_n[256];
    unsigned char               img_auth_rsa_e[5];

    /* AES key */
    unsigned char               sml_aes_key[32];

    /* crypto seed */
    unsigned char               crypto_seed[16];
    
} CUSTOM_SEC_CFG;

#endif /* SEC_CUST_H */



