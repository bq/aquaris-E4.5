/*
 * (c) ARM Limited 2008-2011, 2013
 */



#ifndef __UMP_KERNEL_TYPES_H__
#define __UMP_KERNEL_TYPES_H__

#include "ump_kernel_interface.h"
#include "mali_osk.h"


typedef enum {
	UMP_USED_BY_CPU = 0,
	UMP_USED_BY_MALI = 1,
	UMP_USED_BY_UNKNOWN_DEVICE= 100,
} ump_hw_usage;

typedef enum {
	UMP_NOT_LOCKED = 0,
	UMP_READ = 1,
	UMP_READ_WRITE = 3,
} ump_lock_usage;


/*
 * This struct is what is "behind" a ump_dd_handle
 */
typedef struct ump_dd_mem {
	ump_secure_id secure_id;
	_mali_osk_atomic_t ref_count;
	unsigned long size_bytes;
	unsigned long nr_blocks;
	ump_dd_physical_block * block_array;
	void (*release_func)(void * ctx, struct ump_dd_mem * descriptor);
	void * ctx;
	void * backend_info;
	int is_cached;
	ump_hw_usage hw_device;
	ump_lock_usage lock_usage;
} ump_dd_mem;



#endif /* __UMP_KERNEL_TYPES_H__ */




