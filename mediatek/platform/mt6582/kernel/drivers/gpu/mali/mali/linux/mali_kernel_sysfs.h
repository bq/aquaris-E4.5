/*
 * (c) ARM Limited 2008-2011, 2013
 */



#ifndef __MALI_KERNEL_SYSFS_H__
#define __MALI_KERNEL_SYSFS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/device.h>

#define MALI_PROC_DIR "driver/mali"

int mali_sysfs_register(const char *mali_dev_name);
int mali_sysfs_unregister(void);

#ifdef __cplusplus
}
#endif

#endif /* __MALI_KERNEL_LINUX_H__ */




