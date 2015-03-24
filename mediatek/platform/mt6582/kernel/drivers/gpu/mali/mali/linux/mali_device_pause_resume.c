/*
 * (c) ARM Limited 2008-2011, 2013
 */



#include <linux/module.h>
#include <linux/mali/mali_utgard.h>
#include "mali_gp_scheduler.h"
#include "mali_pp_scheduler.h"

void mali_dev_pause(void)
{
	mali_gp_scheduler_suspend();
	mali_pp_scheduler_suspend();
	mali_group_power_off(MALI_FALSE);
	mali_l2_cache_pause_all(MALI_TRUE);
}

EXPORT_SYMBOL(mali_dev_pause);

void mali_dev_resume(void)
{
	mali_l2_cache_pause_all(MALI_FALSE);
	mali_gp_scheduler_resume();
	mali_pp_scheduler_resume();
}

EXPORT_SYMBOL(mali_dev_resume);




