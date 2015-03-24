/*
 * (c) ARM Limited 2008-2011, 2013
 */



#ifndef __MALI_PM_H__
#define __MALI_PM_H__

#include "mali_osk.h"

_mali_osk_errcode_t mali_pm_initialize(void);
void mali_pm_terminate(void);

/* Callback functions registered for the runtime PMM system */
void mali_pm_os_suspend(void);
void mali_pm_os_resume(void);
void mali_pm_runtime_suspend(void);
void mali_pm_runtime_resume(void);

void mali_pm_set_power_is_on(void);
mali_bool mali_pm_is_power_on(void);

#endif /* __MALI_PM_H__ */




