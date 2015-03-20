
#ifndef BLKDEV_H
#define BLKDEV_H

#include "typedefs.h"

typedef struct blkdev blkdev_t;

struct blkdev {
    u32 type;       /* block device type */
    u32 blksz;      /* block size. (read/write unit) */
    u32 erasesz;    /* erase size */
    u32 blks;       /* number of blocks in the device */
    u32 offset;     /* user area offset in blksz unit */
    u8 *blkbuf;     /* block size buffer */
    void *priv;     /* device private data */    
    blkdev_t *next; /* next block device */
    int (*bread)(blkdev_t *bdev, u32 blknr, u32 blks, u8 *buf);
    int (*bwrite)(blkdev_t *bdev, u32 blknr, u32 blks, u8 *buf);
};

extern int blkdev_register(blkdev_t *bdev);
extern int blkdev_read(blkdev_t *bdev, u64 src, u32 size, u8 *dst);
extern int blkdev_write(blkdev_t *bdev, u64 dst, u32 size, u8 *src);
extern int blkdev_bread(blkdev_t *bdev, u32 blknr, u32 blks, u8 *buf);
extern int blkdev_bwrite(blkdev_t *bdev, u32 blknr, u32 blks, u8 *buf);
extern blkdev_t *blkdev_get(u32 type);

#endif /* BLKDEV_H */


