#ifndef __MT_DEVINFO_H
#define __MT_DEVFINO_H

#include <linux/types.h>

/*device information data*/
#define ATAG_DEVINFO_DATA 0x41000804
#define ATAG_DEVINFO_DATA_SIZE 22

struct tag_devinfo_data {
    u32 devinfo_data[ATAG_DEVINFO_DATA_SIZE]; 	/* device information */
    u32 devinfo_data_size;                      /* device information size */
};

#endif
