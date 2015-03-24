/*
 * (c) ARM Limited 2008-2011, 2013
 */



#ifndef __UMP_UKK_WRAPPERS_H__
#define __UMP_UKK_WRAPPERS_H__

#include <linux/kernel.h>
#include "ump_kernel_common.h"

#ifdef __cplusplus
extern "C" {
#endif



int ump_get_api_version_wrapper(u32 __user * argument, struct ump_session_data * session_data);
int ump_release_wrapper(u32 __user * argument, struct ump_session_data  * session_data);
int ump_size_get_wrapper(u32 __user * argument, struct ump_session_data  * session_data);
int ump_msync_wrapper(u32 __user * argument, struct ump_session_data  * session_data);
int ump_cache_operations_control_wrapper(u32 __user * argument, struct ump_session_data  * session_data);
int ump_switch_hw_usage_wrapper(u32 __user * argument, struct ump_session_data  * session_data);
int ump_lock_wrapper(u32 __user * argument, struct ump_session_data  * session_data);
int ump_unlock_wrapper(u32 __user * argument, struct ump_session_data  * session_data);




#ifdef __cplusplus
}
#endif



#endif /* __UMP_UKK_WRAPPERS_H__ */




