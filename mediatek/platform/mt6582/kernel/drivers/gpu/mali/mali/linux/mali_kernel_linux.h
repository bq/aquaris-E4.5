/*
 * (c) ARM Limited 2008-2011, 2013
 */



#ifndef __MALI_KERNEL_LINUX_H__
#define __MALI_KERNEL_LINUX_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/cdev.h>     /* character device definitions */
#include "mali_kernel_license.h"
#include "mali_osk_types.h"

extern struct platform_device *mali_platform_device;

#if MALI_LICENSE_IS_GPL
/* Defined in mali_osk_irq.h */
extern struct workqueue_struct * mali_wq_normal;
#endif

#ifdef __cplusplus
}
#endif

#endif /* __MALI_KERNEL_LINUX_H__ */




