
#ifndef AC_REGION_H
#define AC_REGION_H

/**************************************************************************
 * [AC REGION ID]
 **************************************************************************/
#define ROM_SEC_AC_REGION_ID            "AND_AC_REGION"
#define ROM_SEC_AC_REGION_ID_LEN        (13)

#define RES_FOR_HEADER                  (0x400) // 1KB
#define ROM_SEC_AC_SEARCH_LEN           0x100000 // 1MB


/**************************************************************************
 * [AC-REGION HEADER FORNAT]
 **************************************************************************/
#define AC_H_MAGIC                      (0x48484848)
 
typedef struct {

    unsigned char                       m_identifier[16];
    unsigned int                        magic_number;

    unsigned int                        region_length;  /* include andro, sv5 and hash */
    unsigned int                        region_offset; 

    unsigned int                        hash_length;    
    unsigned int                        hash_offset;
    
    unsigned int                        andro_length;
    unsigned int                        andro_offset;   
    
    unsigned int                        sv5_length;
    unsigned int                        sv5_offset;   
    
    unsigned char                       reserve[12];
    
} AND_AC_HEADER_T;


/**************************************************************************
 * [Software Secure Boot Format]
 **************************************************************************/
#define SW_SEC_BOOT_MAGIC                  (0x57575757) //WWWW
    
typedef enum {
    SW_SUPPORT_DISABLE=0,
    SW_SUPPORT_ENABLE=1,
} SW_SEC_BOOT_FEATURE_SUPPORT;

typedef enum {
    SW_SEC_BOOT_LOCK=1,
    SW_SEC_BOOT_UNLOCK=2,
} SW_SEC_BOOT_LOCK_TYPE;

typedef enum {
    SW_SEC_BOOT_CERT_PER_PROJECT=1,
    SW_SEC_BOOT_CERT_PER_DEVICE=2,
} SW_SEC_BOOT_CERT_TYPE;

typedef enum {
    SW_SEC_BOOT_NOT_TRY=0,
    SW_SEC_BOOT_TRY_LOCK=1,
    SW_SEC_BOOT_TRY_UNLOCK=2
} SW_SEC_BOOT_TRY_TYPE;

typedef enum {
    SW_SEC_BOOT_NOT_DONE=0,
    SW_SEC_BOOT_DONE_LOCKED=1,
    SW_SEC_BOOT_DONE_UNLOCKED=2
} SW_SEC_BOOT_DONE_TYPE;

typedef enum {
    SW_SEC_BOOT_CHECK_IMAGE=1,
    SW_SEC_BOOT_NOT_CHECK_IMAGE=2,
} SW_SEC_BOOT_CHK_TYPE;
    
typedef struct {
    unsigned int magic_number;
    unsigned int flashtool_unlock_support;
    unsigned int lock_type;
    unsigned int dl_format_lock;
    unsigned int dl_1st_loader_lock;
    unsigned int dl_2nd_loader_lock;
    unsigned int dl_image_lock;
    unsigned int boot_chk_2nd_loader;
    unsigned int boot_chk_logo;
    unsigned int boot_chk_bootimg;
    unsigned int boot_chk_recovery;
    unsigned int boot_chk_system;
    unsigned int boot_chk_others;    
    unsigned int fastboot_unlock_support;
    unsigned int fastboot_unlock_unsigned;
    unsigned int clean_keybox;
    unsigned int cert_type;
    unsigned char cert_device_id[128];
    unsigned char reserve[60];
} AND_SW_SEC_BOOT_T;


/**************************************************************************
 * [AC-REGION ANDRO FORNAT]
 **************************************************************************/
#define AC_ANDRO_MAGIC                  (0x41414141)
#define AP_SECRO_MAX_LEN                (2672)

/* control */
#define FACTORY_EN_CODE                 (0x45)                      

typedef struct {

    unsigned int                        magic_number;       
    unsigned char                       sml_aes_key[32]; /* sml aes key */ 
    unsigned char                       factory_en;
    unsigned char                       reserve1[11];
    AND_SW_SEC_BOOT_T                   sw_sec_boot;
    unsigned char                       reserve2[AP_SECRO_MAX_LEN];
} AND_AC_ANDRO_T;

/**************************************************************************
 * [AC-REGION SV5 FORNAT]
 **************************************************************************/
#define AC_SV5_MAGIC                    (0x35353535)
#define SV5_SECRO_MAX_LEN               (8188)

typedef struct {

    unsigned int                        magic_number;
    unsigned char                       reserve[SV5_SECRO_MAX_LEN];
    
} AND_AC_SV5_T;

/**************************************************************************
 * [SECROIMG FORNAT]
 **************************************************************************/
#define AND_SECROIMG_SIZE              (0x2C00)
typedef struct {
    
    AND_AC_HEADER_T                     m_header;   /* 64 */
    AND_AC_ANDRO_T                      m_andro;    /* 0xBA0  : 2976 */
    AND_AC_SV5_T                        m_sv5;      /* 0x2000 : 8192 */     
    unsigned char                       hash[32];   /* it can be extended to SHA256 */
    
} AND_SECROIMG_T;

#endif /* AC_REGION_H */


