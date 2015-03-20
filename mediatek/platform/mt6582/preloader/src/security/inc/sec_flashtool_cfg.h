
#ifndef SEC_FLASHTOOL_CFG_H
#define SEC_FLASHTOOL_CFG_H

/**************************************************************************
 * [FLASH TOOL ID]
 **************************************************************************/
#define FLASHTOOL_CFG_MAGIC             (0x544F4F4C)
#define FLASHTOOL_CFG_MAGIC_64          (0x544F4F5C)


/**************************************************************************
 * [FLASH TOOL SECURE CONFIG]
 **************************************************************************/
typedef struct {
    unsigned char                       m_img_name [16];
    unsigned int                        m_img_offset;    
    unsigned int                        m_img_length;
} BYPASS_CHECK_IMAGE_T;

typedef struct {
    unsigned char                       m_img_name [16];
    unsigned int                        m_img_offset_high;    
    unsigned int                        m_img_offset_low;
} BYPASS_CHECK_IMAGE_T_64;

/**************************************************************************
 * [FLASH TOOL SECURE CONFIG]
 **************************************************************************/
#define FLASHTOOL_CFG_SIZE              (76)
typedef struct {
    unsigned int                        m_magic_num;
#if defined(FLASHTOOL_SEC_CFG_64)        
    BYPASS_CHECK_IMAGE_T_64             m_bypass_check_img [3];
#else
    BYPASS_CHECK_IMAGE_T                m_bypass_check_img [3];
#endif
} FLASHTOOL_SECCFG_T;


/**************************************************************************
 * [FLASH TOOL FORBID DOWNLOAD ID]
 **************************************************************************/
#define FLASHTOOL_NON_SLA_FORBID_MAGIC             (0x544F4F4D)
#define FLASHTOOL_NON_SLA_FORBID_MAGIC_64          (0x544F4F5D)


/**************************************************************************
 * [FLASH TOOL FORBID DOWNLOAD CONFIG]
 **************************************************************************/
typedef struct {
    unsigned char                       m_img_name [16];
    unsigned int                        m_img_offset;    
    unsigned int                        m_img_length;
} FORBID_DOWNLOAD_IMAGE_T;

typedef struct {
    unsigned char                       m_img_name [16];
    unsigned int                        m_img_offset_high;    
    unsigned int                        m_img_offset_low;
} FORBID_DOWNLOAD_IMAGE_T_64;

/**************************************************************************
 * [FLASH TOOL FORBID DOWNLOAD CONFIG]
 **************************************************************************/
#define FLASHTOOL_NON_SLA_FORBID_CFG_SIZE              (52)
typedef struct {
    unsigned int                        m_forbid_magic_num;

#if defined(FLASHTOOL_FORBID_DL_NSLA_CFG_64)
    FORBID_DOWNLOAD_IMAGE_T_64          m_forbid_dl_nsla_img [2];
#else
    FORBID_DOWNLOAD_IMAGE_T             m_forbid_dl_nsla_img [2];
#endif
} FLASHTOOL_FORBID_DOWNLOAD_NSLA_T;

#endif /* SEC_FLASHTOOL_CFG_H */



