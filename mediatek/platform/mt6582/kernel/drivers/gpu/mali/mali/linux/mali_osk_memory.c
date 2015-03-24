/*
 * (c) ARM Limited 2008-2011, 2013
 */



#include "mali_osk.h"
#include <linux/slab.h>
#include <linux/vmalloc.h>

void inline *_mali_osk_calloc( u32 n, u32 size )
{
	return kcalloc(n, size, GFP_KERNEL);
}

void inline *_mali_osk_malloc( u32 size )
{
	return kmalloc(size, GFP_KERNEL);
}

void inline _mali_osk_free( void *ptr )
{
	kfree(ptr);
}

void inline *_mali_osk_valloc( u32 size )
{
	return vmalloc(size);
}

void inline _mali_osk_vfree( void *ptr )
{
	vfree(ptr);
}

void inline *_mali_osk_memcpy( void *dst, const void *src, u32	len )
{
	return memcpy(dst, src, len);
}

void inline *_mali_osk_memset( void *s, u32 c, u32 n )
{
	return memset(s, c, n);
}

mali_bool _mali_osk_mem_check_allocated( u32 max_allocated )
{
	/* No need to prevent an out-of-memory dialogue appearing on Linux,
	 * so we always return MALI_TRUE.
	 */
	return MALI_TRUE;
}




