/*
 * (c) ARM Limited 2008-2011, 2013
 */



#include <linux/version.h>
#include <linux/sched.h>
#include <linux/module.h>
#include "mali_osk.h"
#include "mali_kernel_common.h"
#include "mali_pmu.h"
#include "mali_pp_scheduler.h"
#include "linux/mali/mali_utgard.h"

/* Mali PMU power up/down APIs */

int mali_pmu_powerup(void)
{
	struct mali_pmu_core *pmu = mali_pmu_get_global_pmu_core();

	MALI_DEBUG_PRINT(5, ("Mali PMU: Power up\n"));

	MALI_DEBUG_ASSERT_POINTER(pmu);
	if (NULL == pmu) {
		return -ENXIO;
	}

	if (_MALI_OSK_ERR_OK != mali_pmu_power_up_all(pmu)) {
		return -EFAULT;
	}

	return 0;
}

EXPORT_SYMBOL(mali_pmu_powerup);

int mali_pmu_powerdown(void)
{
	struct mali_pmu_core *pmu = mali_pmu_get_global_pmu_core();

	MALI_DEBUG_PRINT(5, ("Mali PMU: Power down\n"));

	MALI_DEBUG_ASSERT_POINTER(pmu);
	if (NULL == pmu) {
		return -ENXIO;
	}

	if (_MALI_OSK_ERR_OK != mali_pmu_power_down_all(pmu)) {
		return -EFAULT;
	}

	return 0;
}

EXPORT_SYMBOL(mali_pmu_powerdown);

int mali_perf_set_num_pp_cores(unsigned int num_cores)
{
	return mali_pp_scheduler_set_perf_level(num_cores, MALI_FALSE);
}

EXPORT_SYMBOL(mali_perf_set_num_pp_cores);




