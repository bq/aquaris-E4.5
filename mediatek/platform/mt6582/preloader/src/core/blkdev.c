
#include "typedefs.h"
#include "platform.h"
#include "blkdev.h"
#include "dram_buffer.h"

static blkdev_t *blkdev_list = NULL;

int blkdev_register(blkdev_t *bdev)
{
    blkdev_t *tail = blkdev_list;

    bdev->next = NULL;

    while (tail && tail->next) {
        tail = tail->next;
    }

    if (tail) {
        tail->next = bdev;
    } else {
        blkdev_list = bdev;
    }

    return 0;
}

blkdev_t *blkdev_get(u32 type)
{
    blkdev_t *bdev = blkdev_list;

    while (bdev) {
        if (bdev->type == type)
            break;
        bdev = bdev->next;
    }
    return bdev;
}

int blkdev_bread(blkdev_t *bdev, u32 blknr, u32 blks, u8 *buf)
{
    return bdev->bread(bdev, blknr, blks, buf);
}

int blkdev_bwrite(blkdev_t *bdev, u32 blknr, u32 blks, u8 *buf)
{
    return bdev->bwrite(bdev, blknr, blks, buf);
}

int blkdev_read(blkdev_t *bdev, u64 src, u32 size, u8 *dst)
{
    u8 *buf = (u8*)bdev->blkbuf ? bdev->blkbuf : g_dram_buf->storage_buffer;
    u32 blksz = bdev->blksz;
    u64 end, part_start, part_end, part_len, aligned_start, aligned_end;
    u32 blknr, blks;

    if (!bdev) {
        return -1;
    }

    if (size == 0) 
        return 0;

    end = src + size;

    part_start    = src &  (blksz - 1);
    part_end      = end &  (blksz - 1);
    aligned_start = src & ~(blksz - 1);
    aligned_end   = end & ~(blksz - 1);
 
	if (part_start) {
        blknr = aligned_start / blksz;	
        part_len = part_start + size > blksz ? blksz - part_start : size;
        if ((bdev->bread(bdev, blknr, 1, buf)) != 0)
            return -1;
        memcpy(dst, buf + part_start, part_len);
        dst  += part_len;
        src  += part_len;
        size -= part_len;
    }

    if (size >= blksz) {
        aligned_start = src & ~(blksz - 1);
        blknr  = aligned_start / blksz;
        blks = (aligned_end - aligned_start) / blksz;

        if (blks && 0 != bdev->bread(bdev, blknr, blks, dst))
            return -1;

        src  += (blks * blksz);
        dst  += (blks * blksz);
        size -= (blks * blksz);
    }
    if (size && part_end && src < end) {
        blknr = aligned_end / blksz;	
        if ((bdev->bread(bdev, blknr, 1, buf)) != 0)
            return -1;
        memcpy(dst, buf, part_end);
    }
    return 0;    
}

int blkdev_write(blkdev_t *bdev, u64 dst, u32 size, u8 *src)
{
    u8 *buf = (u8*)bdev->blkbuf ? bdev->blkbuf : g_dram_buf->storage_buffer;
    u32 blksz = bdev->blksz;
    u64 end, part_start, part_end, part_len, aligned_start, aligned_end;
    u32 blknr, blks;

    if (!bdev) {
        return -1;
    }

    if (size == 0) 
        return 0;

    end = dst + size;

    part_start    = dst &  (blksz - 1);
    part_end      = end &  (blksz - 1);
    aligned_start = dst & ~(blksz - 1);
    aligned_end   = end & ~(blksz - 1);
 
    if (part_start) {
        blknr = aligned_start / blksz;	
        part_len = part_start + size > blksz ? blksz - part_start : size;
        if ((bdev->bread(bdev, blknr, 1, buf)) != 0)
            return -1;
        memcpy(buf + part_start, src, part_len);
        if ((bdev->bwrite(bdev, blknr, 1, buf)) != 0)
            return -1;
        dst  += part_len;
        src  += part_len;
        size -= part_len;
    }

    if (size >= blksz) {
        aligned_start = dst & ~(blksz - 1);
        blknr  = aligned_start / blksz;
        blks = (aligned_end - aligned_start) / blksz;

        if (blks && 0 != bdev->bwrite(bdev, blknr, blks, src))
            return -1;

        src  += (blks * blksz);
        dst  += (blks * blksz);
        size -= (blks * blksz);
    }
    
    if (size && part_end && dst < end) {
        blknr = aligned_end / blksz;	
        if ((bdev->bread(bdev, blknr, 1, buf)) != 0)
            return -1;
        memcpy(buf, src, part_end);
        if ((bdev->bwrite(bdev, blknr, 1, buf)) != 0)
            return -1;		
    }
    return 0;
}



