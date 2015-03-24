/*
 * (c) ARM Limited 2008-2011, 2013
 */



#ifndef __MALI_USER_SETTINGS_DB_H__
#define __MALI_USER_SETTINGS_DB_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "mali_uk_types.h"

/** @brief Set Mali user setting in DB
 *
 * Update the DB with a new value for \a setting. If the value is different from theprevious set value running sessions will be notified of the change.
 *
 * @param setting the setting to be changed
 * @param value the new value to set
 */
void mali_set_user_setting(_mali_uk_user_setting_t setting, u32 value);

/** @brief Get current Mali user setting value from DB
 *
 * @param setting the setting to extract
 * @return the value of the selected setting
 */
u32 mali_get_user_setting(_mali_uk_user_setting_t setting);

#ifdef __cplusplus
}
#endif
#endif  /* __MALI_KERNEL_USER_SETTING__ */




