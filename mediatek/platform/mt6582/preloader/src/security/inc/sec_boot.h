
#ifndef SEC_BOOT_H
#define SEC_BOOT_H

#include "cust_sec_ctrl.h"


/**************************************************************************
 * [SECURE PARTITION NAME] 
 **************************************************************************/
/* Note : these definitions all sync with download tool. do not modify it */
#define SBOOT_PART_PL                   "PRELOADER"
#define SBOOT_PART_UBOOT                "UBOOT"
#define SBOOT_PART_LOGO                 "LOGO"
#define SBOOT_PART_BOOTIMG              "BOOTIMG"
#define SBOOT_PART_ANDSYSIMG            "ANDROID"
#define SBOOT_PART_USERDATA             "USRDATA"
#define SBOOT_PART_CACHE                "CACHE"
#define SBOOT_PART_RECOVERY             "RECOVERY"
#define SBOOT_PART_SECSTATIC            "SEC_RO"
#define SBOOT_PART_SECURE               "SECCNFG"

/**************************************************************************
 * [SECURE BOOT CHECK] 
 **************************************************************************/
/* Note : this structure record all the partitions
          which should be verified by secure boot check */
#define AND_SEC_BOOT_CHECK_PART_SIZE    (90)
typedef struct {
    unsigned char                       name[9][10];
} AND_SECBOOT_CHECK_PART_T;

#endif /* SEC_BOOT_H */



