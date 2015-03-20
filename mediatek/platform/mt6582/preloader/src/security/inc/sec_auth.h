
#ifndef SEC_DA_VERIFY_H
#define SEC_DA_VERIFY_H

/**************************************************************************
 *  AUTH GENERAL FUNCTION
 **************************************************************************/
extern int sec_auth (unsigned char* data_buf, unsigned int data_len, unsigned char* sig_buf, unsigned int sig_len);

/**************************************************************************
 *  HASH GENERAL FUNCTION
 **************************************************************************/
extern int sec_hash (U8* data_buf, U32 data_len, U8* hash_buf, U32 hash_len);

/**************************************************************************
 *  DOWNLOAD AGENT
 **************************************************************************/
extern unsigned int da_auth_init (void);

/**************************************************************************
 *  IMAGE
 **************************************************************************/
extern unsigned int img_auth_init (void);

#endif /* SEC_DA_VERIFY_H */



