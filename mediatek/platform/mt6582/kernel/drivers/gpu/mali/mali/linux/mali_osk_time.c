/*
 * (c) ARM Limited 2008-2011, 2013
 */



#include "mali_osk.h"
#include <linux/jiffies.h>
#include <linux/time.h>
#include <asm/delay.h>

int	_mali_osk_time_after( u32 ticka, u32 tickb )
{
	return time_after((unsigned long)ticka, (unsigned long)tickb);
}

u32	_mali_osk_time_mstoticks( u32 ms )
{
	return msecs_to_jiffies(ms);
}

u32	_mali_osk_time_tickstoms( u32 ticks )
{
	return jiffies_to_msecs(ticks);
}

u32	_mali_osk_time_tickcount( void )
{
	return jiffies;
}

void _mali_osk_time_ubusydelay( u32 usecs )
{
	udelay(usecs);
}

u64 _mali_osk_time_get_ns( void )
{
	struct timespec tsval;
	getnstimeofday(&tsval);
	return (u64)timespec_to_ns(&tsval);
}




