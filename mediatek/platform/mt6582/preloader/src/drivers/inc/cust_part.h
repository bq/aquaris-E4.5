
#ifndef CUST_PART_H
#define CUST_PART_H

#include "partition_define.h"

#define LOAD_ADDR_MODE_NORMAL    0xffffffff
#define LOAD_ADDR_MODE_BACKWARD  0x00000000

typedef union
{
    struct
    {
        unsigned int magic;     /* partition magic */
        unsigned int dsize;     /* partition data size */
        char name[32];          /* partition name */    
        unsigned int maddr;     /* partition memory address */
	unsigned int mode;      /* memory addressing mode */
    } info;
    unsigned char data[512];
} part_hdr_t;

#ifdef MTK_EMMC_SUPPORT
typedef struct
{
    unsigned char *name;        /* partition name */
    unsigned long startblk;     /* partition start blk */
    unsigned long long size;         /* partition size */
    unsigned long blks;         /* partition blks */
    unsigned long flags;        /* partition flags */
} part_t;
#else
typedef struct
{
    unsigned char *name;        /* partition name */
    unsigned long startblk;     /* partition start blk */
    unsigned long size;         /* partition size */
    unsigned long blks;         /* partition blks */
    unsigned long flags;        /* partition flags */
} part_t;
#endif

extern void cust_part_init(void);
extern part_t *cust_part_tbl(void);

#endif /* CUST_PART_H */


