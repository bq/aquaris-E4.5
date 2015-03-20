
#ifndef PART_H
#define PART_H

#include "typedefs.h"
#include "blkdev.h"
#include "cust_part.h"

#define PART_HEADER_MEMADDR        (0xFFFFFFFF)

extern int part_init(void);
extern part_t *part_get(char *name);
extern int part_load(blkdev_t *bdev, part_t *part, u32 *addr, u32 offset, u32 size);
extern void part_dump(void);

#endif /* PART_H */


