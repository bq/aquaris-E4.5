
#include "sec_platform.h"
#include "sec_auth.h"
#include "KEY_IMAGE_AUTH.h"
#include "sec_error.h"

/**************************************************************************
 *  EXTERNAL FUNCTIONS
 **************************************************************************/
extern int seclib_rsa_verify (U8* data_buf,  U32 data_len, U8* sig_buf, U32 sig_len);
extern int seclib_image_hash_compute (data_buf, data_len, hash_buf, hash_len);
extern U32 seclib_rsa_init (const U8 *nKey, U32 nKey_len, const U8 *eKey, U32 eKey_len);

/**************************************************************************
 *  RSA (only support RSA1024)
 **************************************************************************/
int sec_auth (U8* data_buf, U32 data_len, U8* sig_buf, U32 sig_len)
{
    u8 *rel_addr;
    u8 *sig_addr;
    u32 verify_len;
    /* define for static use only */
    #define SEC_IMG_HDR_SZ (64)
    #define SEC_IMG_SIG_SZ (128)

    /* check if this is used to verify DA_PL or verify image */
    if( *(U32*)data_buf != (0x53535353) )
    {
        /* relocate DA address (header+data) */
        rel_addr = (u8*)DA_RAM_RELOCATE_ADDR;
        memcpy((u8*)(DA_RAM_RELOCATE_ADDR), (u8*)sig_buf, SEC_IMG_HDR_SZ);
        memcpy((u8*)(DA_RAM_RELOCATE_ADDR + SEC_IMG_HDR_SZ), (u8*)data_buf, data_len);
        verify_len = SEC_IMG_HDR_SZ + data_len;

        /* relocate signature address */
        sig_addr = (u8*)sig_buf + SEC_IMG_HDR_SZ;
        
        return seclib_rsa_verify(rel_addr, verify_len, sig_addr, SEC_IMG_SIG_SZ);
    }
    else
    {
        return seclib_rsa_verify(data_buf, data_len, sig_buf, sig_len);
    }
}

/**************************************************************************
 *  HASH (only support SHA1)
 **************************************************************************/
int sec_hash (U8* data_buf, U32 data_len, U8* hash_buf, U32 hash_len)
{
    return seclib_image_hash_compute (data_buf, data_len, hash_buf, hash_len);
}

/**************************************************************************
 *  DA AUTH INIT
 **************************************************************************/
U32 da_auth_init (void)
{       

    U32 ret = SEC_OK;
    U32 nKey_len = seclib_mtk_rsa_n_key_len();
    U32 eKey_len = seclib_mtk_rsa_e_key_len();

    if(SEC_OK != (ret = seclib_rsa_init (IMG_CUSTOM_RSA_N, nKey_len, IMG_CUSTOM_RSA_E, eKey_len)))
    {
        goto _err;
    }

_err:    

    return ret;    
}

/**************************************************************************
 *  IMAGE AUTH INIT
 **************************************************************************/
U32 img_auth_init (void)
{       

    U32 ret = SEC_OK;
    U32 nKey_len = seclib_mtk_rsa_n_key_len();
    U32 eKey_len = seclib_mtk_rsa_e_key_len();

    if(SEC_OK != (ret = seclib_rsa_init (IMG_CUSTOM_RSA_N, nKey_len, IMG_CUSTOM_RSA_E, eKey_len)))
    {
        goto _err;
    }

_err:    

    return ret;    
}


