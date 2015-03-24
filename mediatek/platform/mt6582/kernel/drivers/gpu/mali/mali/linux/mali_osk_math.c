/*
 * (c) ARM Limited 2008-2011, 2013
 */



#include "mali_osk.h"
#include <linux/bitops.h>

u32 _mali_osk_clz( u32 input )
{
	return 32-fls(input);
}

u32 _mali_osk_fls( u32 input )
{
	return fls(input);
}




