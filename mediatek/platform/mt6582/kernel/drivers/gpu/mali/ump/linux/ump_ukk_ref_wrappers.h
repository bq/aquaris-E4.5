/*
 * (c) ARM Limited 2008-2011, 2013
 */



#ifndef __UMP_UKK_REF_WRAPPERS_H__
#define __UMP_UKK_REF_WRAPPERS_H__

#include <linux/kernel.h>
#include "ump_kernel_common.h"

#ifdef __cplusplus
extern "C" {
#endif


int ump_allocate_wrapper(u32 __user * argument, struct ump_session_data  * session_data);


#ifdef __cplusplus
}
#endif

#endif /* __UMP_UKK_REF_WRAPPERS_H__ */




