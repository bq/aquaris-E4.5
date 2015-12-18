#ifndef __PARTITION_DEFINE_H__
#define __PARTITION_DEFINE_H__

#include <linux/types.h>

#define PART_MAX_COUNT	    40
#define WRITE_SIZE_Byte		512

extern int PART_NUM;
extern u64 MBR_START_ADDRESS_BYTE;

typedef enum {
	EMMC = 1,
	NAND = 2,
} dev_type;

typedef enum {
	USER = 0,
	BOOT_1,
	BOOT_2,
	RPMB,
	GP_1,
	GP_2,
	GP_3,
	GP_4,
} Region;

struct excel_info {
	char *name;
	unsigned long long size;
	unsigned long long start_address;
	dev_type type;
	unsigned int partition_idx;
	Region region;
};

extern struct excel_info PartInfo[PART_MAX_COUNT];

#endif
