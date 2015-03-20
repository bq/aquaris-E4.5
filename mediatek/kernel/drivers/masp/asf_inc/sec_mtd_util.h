#ifndef MTD_UTILS_H
#define MTD_UTILS_H

/**************************************************************************
*  PARTITION RECORD
**************************************************************************/
typedef struct _MtdPart 
{
    char name[16];
    unsigned int sz;    
    unsigned int off;

} MtdPart;

/**************************************************************************
*  MTD CONFIGURATION
**************************************************************************/
/* partition table read from /proc/mtd */
#define MAX_MTD_PARTITIONS              (25)

/* search region and off */
//#ifdef EMMC_PROJECT
/* work for nand and emmc */
#define ROM_INFO_SEARCH_START           (0x0)
//#else
//#define ROM_INFO_SEARCH_START         (0x20000)
//#endif

/**************************************************************************
 *  EXPORT VARIABLES
 **************************************************************************/
extern MtdPart                          mtd_part_map[];

/**************************************************************************
 *  UTILITY
 **************************************************************************/
char* mtd2pl (char* part_name);
char* pl2mtd (char* part_name);

#endif  // MTD_UTILS_H
