
#ifndef GFH_H
#define GFH_H

#include "typedefs.h"

#define GFH_HDR_MAGIC           (0x004D4D4D)
#define GFH_HDR_MAGIC_END_MARK  (0x45454545)
#define GFH_GET_MAGIC(magic)    ((magic) & 0x00FFFFFF)
#define GFH_GET_VER(magic)      ((magic) >> 24)

enum {
    GFH_FILE_INFO = 0x0000
};

typedef struct gfh_header {
    u32 m_magic_ver;
    u16 m_size;
    u16 m_type;
} gfh_header_t;

typedef struct gfh_file_info {
    gfh_header_t    m_gfh_hdr;
    char            m_id[12]; /* include '\0' */
    u32             m_file_ver;
    u16             m_file_type;
    u8              m_flash_dev;
    u8              m_sig_type;
    u32             m_load_addr;
    u32             m_file_len;
    u32             m_max_size;
    u32             m_content_offset;
    u32             m_sig_len;
    u32             m_jump_offset;
    u32             m_attr;
} gfh_file_info_t;

#endif /* GFH_H */


