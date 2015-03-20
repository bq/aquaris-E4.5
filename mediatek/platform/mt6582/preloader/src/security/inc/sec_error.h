
#ifndef SECLIB_ERROR_H
#define SECLIB_ERROR_H

#define SEC_OK                                  0x0000

/* CRYPTO */
#define ERR_CRYPTO_INIT_FAIL                    0x1000
#define ERR_CRYPTO_DEINIT_FAIL                  0x1001
#define ERR_CRYPTO_MODE_INVALID                 0x1002
#define ERR_CRYPTO_KEY_INVALID                  0x1003
#define ERR_CRYPTO_DATA_UNALIGNED               0x1004
#define ERR_CRYPTO_SEED_LEN_ERROR               0x1005

/* AUTH */
#define ERR_AUTH_IMAGE_VERIFY_FAIL              0x2000
#define ERR_DA_IMAGE_SIG_VERIFY_FAIL            0x2001
#define ERR_DA_IMAGE_NO_MEM_FAIL                0x2002
#define ERR_DA_INIT_KEY_FAIL                    0x2003
#define ERR_IMG_INIT_KEY_FAIL                   0x2004
#define ERR_HASH_IMAGE_FAIL                     0x2005
#define ERR_DA_RELOCATE_SIZE_NOT_ENOUGH         0x2006

/* LIB */
#define ERR_LIB_SEC_CFG_NOT_EXIST               0x3000
#define ERR_LIB_VER_INVALID                     0x3001
#define ERR_LIB_SEC_CFG_ERASE_FAIL              0x3002
#define ERR_LIB_SEC_CFG_CANNOT_WRITE            0x3003
#define ERR_LIB_SEC_CFG_END_PATTERN_NOT_EXIST   0x3004
#define ERR_LIB_SEC_CFG_STATUS_INVALID          0x3005
#define ERR_LIB_SEC_CFG_READ_SIZE_NOT_ENOUGH    0x3006
#define ERR_LIB_SEC_CFG_RSA_KEY_INIT_FAIL       0x3007

/* SECURE DOWNLOAD / IMAGE VERIFICATION */
#define ERR_IMG_VERIFY_THIS_IMG_INFO_NOT_EXIST  0x4000
#define ERR_IMG_VERIFY_HASH_COMPARE_FAIL        0x4001
#define ERR_IMG_VERIFY_NO_SPACE_ADD_IMG_INFO    0x4002
#define ERR_SEC_DL_TOKEN_NOT_FOUND_IN_IMG       0x4003
#define ERR_SEC_DL_FLOW_ERROR                   0x4004
#define ERR_IMG_VERIFY_INVALID_IMG_INFO_ATTR    0x4005
#define ERR_IMG_SECROIMG_NOT_FOUND              0x4006
#define ERR_IMG_READ_FAIL                       0x4007
#define ERR_IMG_VERIFY_SIGNATURE_FAIL           0x4008
#define ERR_IMG_SIGN_FORMAT_NOT_MATCH           0x4009
#define ERR_IMG_EXTENSION_HDR_NOT_FOUND         0x400A
#define ERR_IMG_EXTENSION_MAGIC_WRONG           0x400B
#define ERR_IMG_EXTENSION_TYPE_NOT_SUPPORT      0x400C
#define ERR_IMG_EXTENSION_HASH_CAL_FAIL         0x400D

/* IMAGE DOWNLOAD LOCK */
#define ERR_IMG_LOCK_TABLE_NOT_EXIST            0x5000
#define ERR_IMG_LOCK_ALL_LOCK                   0x5001
#define ERR_IMG_LOCK_NO_SPACE_ADD_LOCK_INFO     0x5002
#define ERR_IMG_LOCK_THIS_IMG_INFO_NOT_EXIST    0x5003
#define ERR_IMG_LOCK_MAGIC_ERROR                0x5004

/* SECURE KEY */
#define ERR_SBC_KEY_NOT_FOUND                   0x6000
#define ERR_BR_SEC_CFG_NOT_FOUND	        0x6001

/* SECURE REGION CHECK */
#define ERR_REGION_INVALID_INCLUDE              0x7000
#define ERR_REGION_INVALID_OVERLAP              0x7001
#define ERR_REGION_INVALID_OVERFLOW             0x7002
#define ERR_DA_INVALID_LOCATION                 0x7003
#define ERR_DA_INVALID_LENGTH                   0x7004

#endif /* SECLIB_ERROR_H */



