/*
 * (c) ARM Limited 2008-2011, 2013
 */



#include "ump_osk.h"
#include <asm/atomic.h>

int _ump_osk_atomic_dec_and_read( _mali_osk_atomic_t *atom )
{
	return atomic_dec_return((atomic_t *)&atom->u.val);
}

int _ump_osk_atomic_inc_and_read( _mali_osk_atomic_t *atom )
{
	return atomic_inc_return((atomic_t *)&atom->u.val);
}




