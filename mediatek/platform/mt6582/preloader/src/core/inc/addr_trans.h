
#ifndef ADDR_TRANS_H
#define ADDR_TRANS_H

#include "typedefs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct addr_trans_info {
    u32 id;
    u64 len;
} addr_trans_info_t;

typedef struct addr_trans_tbl {
    u32 num;
    addr_trans_info_t *info;
} addr_trans_tbl_t;

typedef struct virt_addr {
    u64 addr;
} virt_addr_t;

typedef struct phys_addr {
    u32 id;
    u64 addr;
} phys_addr_t;

extern int virt_to_phys_addr(addr_trans_tbl_t *tbl, virt_addr_t *virt, phys_addr_t *phys);
extern int phys_to_virt_addr(addr_trans_tbl_t *tbl, phys_addr_t *phys, virt_addr_t *virt);

#ifdef __cplusplus
}
#endif

#endif /* ADDR_TRANS_H */



