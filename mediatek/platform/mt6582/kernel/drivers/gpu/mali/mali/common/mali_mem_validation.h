/*
 * (c) ARM Limited 2008-2011, 2013
 */



#ifndef __MALI_MEM_VALIDATION_H__
#define __MALI_MEM_VALIDATION_H__

#include "mali_osk.h"

_mali_osk_errcode_t mali_mem_validation_add_range(u32 start, u32 size);
_mali_osk_errcode_t mali_mem_validation_check(u32 phys_addr, u32 size);

#endif /* __MALI_MEM_VALIDATION_H__ */




