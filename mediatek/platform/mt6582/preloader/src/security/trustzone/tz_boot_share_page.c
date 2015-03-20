#include "typedefs.h"
#include "tz_boot_share_page.h"
#include "mt6582.h"

#define MOD "[BT_SD_PG]"

void tz_boot_share_page_init(void)
{
    /* assume D-cache is diabled */
    /* 1. clear boot share page */
    memset((void*)BOOT_SHARE_BASE, 0, BOOT_SHARE_SIZE);

    /* 2. Fill magic number */
    *(unsigned int*)(BOOT_SHARE_BASE+BOOT_SHARE_MAGIC1_OFST) = BOOT_SHARE_MAGIC;
    *(unsigned int*)(BOOT_SHARE_BASE+BOOT_SHARE_MAGIC2_OFST) = BOOT_SHARE_MAGIC;
    
    /* 3. Fill device information */
    *(unsigned int*)(BOOT_SHARE_BASE+BOOT_SHARE_DEV_INFO_OFST+0x00000000) = DRV_Reg32(APHW_CODE);
    *(unsigned int*)(BOOT_SHARE_BASE+BOOT_SHARE_DEV_INFO_OFST+0x00000004) = DRV_Reg32(APHW_SUBCODE);
    *(unsigned int*)(BOOT_SHARE_BASE+BOOT_SHARE_DEV_INFO_OFST+0x00000008) = DRV_Reg32(APHW_VER);
    *(unsigned int*)(BOOT_SHARE_BASE+BOOT_SHARE_DEV_INFO_OFST+0x0000000c) = DRV_Reg32(APSW_VER);
#if 0
    print("%s device info 0x%x 0x%x 0x%x 0x%x\n", MOD, DRV_Reg32(APHW_CODE), DRV_Reg32(APHW_SUBCODE), DRV_Reg32(APHW_VER), DRV_Reg32(APSW_VER));    
    print("%s device info 0x%x 0x%x 0x%x 0x%x\n", MOD, *(unsigned int*)(BOOT_SHARE_BASE+BOOT_SHARE_DEV_INFO_OFST+0x00000000), 
            *(unsigned int*)(BOOT_SHARE_BASE+BOOT_SHARE_DEV_INFO_OFST+0x00000004), 
            *(unsigned int*)(BOOT_SHARE_BASE+BOOT_SHARE_DEV_INFO_OFST+0x00000008), 
            *(unsigned int*)(BOOT_SHARE_BASE+BOOT_SHARE_DEV_INFO_OFST+0x0000000c));
#endif
}


