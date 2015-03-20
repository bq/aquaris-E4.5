
#ifndef SEC_H
#define SEC_H

#include "sec_cust.h"
#include "sec_error.h"
#include "sec_platform.h"

/******************************************************************************
 * FPGA PORTING
 ******************************************************************************/
//#define SEC_FPGA_EARLY_PORTING

/******************************************************************************
 * SECURE CFG READ SIZE
 ******************************************************************************/
#define SEC_CFG_READ_SIZE               SEC_WORKING_BUFFER_LENGTH

/******************************************************************************
 * SECURE CFG PARTITION INFO
 ******************************************************************************/
typedef struct _SECURE_CFG_INFO
{   
    U32                                 addr;
    U32                                 len;
    
} SECURE_CFG_INFO;


/**************************************************************************
 * EXPORTED FUNCTIONS
 **************************************************************************/

/* configuration for PL download DA feature */
extern BOOL seclib_sbc_enabled(void);
extern BOOL seclib_daa_enabled(void);
extern BOOL seclib_sla_enabled(void);

/* secure boot */
extern void sec_boot_check (void);
extern BOOL seclib_sec_boot_enabled (BOOL bMsg);
extern U32 seclib_image_check (U8* image_name, BOOL bMsg);

 /* sec_cfg related */
extern U8* sec_util_image_read (U32 offset, U32 size);
extern U8* sec_util_get_img_buf (void);
extern U8* sec_util_get_working_buf (void);
extern void seclib_image_hash_compute (U8 *buf, U32 size);
extern void seclib_cfg_print_status();


/* library initialization */
extern U32 seclib_init ( CUSTOM_SEC_CFG *cust_cfg, U8* sec_cfg_cipher_data, U32 sec_cfg_read_size, BOOL bMsg, BOOL bAC);

/* region check */
extern U32 seclib_region_check (U32 addr, U32 len);

#endif /* SEC_H */



