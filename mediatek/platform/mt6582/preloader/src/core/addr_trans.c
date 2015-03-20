
#include "addr_trans.h"

int virt_to_phys_addr(addr_trans_tbl_t *tbl, virt_addr_t *virt, phys_addr_t *phys)
{
    u32 i;
    u32 num = tbl->num;
    u64 base_addr = 0, boundary_addr = 0;
    addr_trans_info_t *info = tbl->info;

    if (!info) {
        phys->id   = -1;
        phys->addr = virt->addr;
        return 0;
    }

    for (i = 0; i < num; i++, info++) {
        if (info->len) {
            boundary_addr += info->len;
            if (boundary_addr < base_addr)
                break; /* overflow */
            if ((base_addr <= virt->addr) && (virt->addr < boundary_addr)) {
                phys->id   = info->id;
                phys->addr = virt->addr - base_addr;
                return 0;
            }
            base_addr += info->len;
        }
    }

    return -1;
}

int phys_to_virt_addr(addr_trans_tbl_t *tbl, phys_addr_t *phys, virt_addr_t *virt)
{
    u32 i;
    u32 num = tbl->num;
    u64 base_addr = 0;
    addr_trans_info_t *info = tbl->info;

    if (!info) {
        virt->addr = phys->addr;
        return 0;
    }

    for (i = 0; i < num; i++, info++) {
        if (info->id == phys->id && info->len && phys->addr <= info->len) {
            virt->addr = phys->addr + base_addr;
            return 0;            
        }
        base_addr += info->len;
    }

    return -1;
}




