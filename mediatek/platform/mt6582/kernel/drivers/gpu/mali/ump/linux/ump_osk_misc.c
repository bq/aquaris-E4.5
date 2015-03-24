/*
 * (c) ARM Limited 2008-2011, 2013
 */




#include "ump_osk.h"

#include <linux/kernel.h>
#include "ump_kernel_linux.h"

/* is called from ump_kernel_constructor in common code */
_mali_osk_errcode_t _ump_osk_init( void )
{
	if (0 != ump_kernel_device_initialize()) {
		return _MALI_OSK_ERR_FAULT;
	}

	return _MALI_OSK_ERR_OK;
}

_mali_osk_errcode_t _ump_osk_term( void )
{
	ump_kernel_device_terminate();
	return _MALI_OSK_ERR_OK;
}




