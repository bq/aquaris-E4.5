
#include "mali_ukk.h"
#include "mali_osk.h"
#include "mali_kernel_common.h"
#include "mali_session.h"
#include "mali_ukk_wrappers.h"

int pp_start_job_wrapper(struct mali_session_data *session_data, _mali_uk_pp_start_job_s __user *uargs)
{
	_mali_osk_errcode_t err;

	/* If the job was started successfully, 0 is returned.  If there was an error, but the job
	 * was started, we return -ENOENT.  For anything else returned, the job was not started. */

	MALI_CHECK_NON_NULL(uargs, -EINVAL);
	MALI_CHECK_NON_NULL(session_data, -EINVAL);

	err = _mali_ukk_pp_start_job(session_data, uargs);
	if (_MALI_OSK_ERR_OK != err) return map_errcode(err);

	return 0;
}

int pp_and_gp_start_job_wrapper(struct mali_session_data *session_data, _mali_uk_pp_and_gp_start_job_s __user *uargs)
{
	_mali_osk_errcode_t err;

	/* If the jobs were started successfully, 0 is returned.  If there was an error, but the
	 * jobs were started, we return -ENOENT.  For anything else returned, the jobs were not
	 * started. */

	MALI_CHECK_NON_NULL(uargs, -EINVAL);
	MALI_CHECK_NON_NULL(session_data, -EINVAL);

	err = _mali_ukk_pp_and_gp_start_job(session_data, uargs);
	if (_MALI_OSK_ERR_OK != err) return map_errcode(err);

	return 0;
}

int pp_get_number_of_cores_wrapper(struct mali_session_data *session_data, _mali_uk_get_pp_number_of_cores_s __user *uargs)
{
	_mali_uk_get_pp_number_of_cores_s kargs;
	_mali_osk_errcode_t err;

	MALI_CHECK_NON_NULL(uargs, -EINVAL);
	MALI_CHECK_NON_NULL(session_data, -EINVAL);

	kargs.ctx = session_data;

	err = _mali_ukk_get_pp_number_of_cores(&kargs);
	if (_MALI_OSK_ERR_OK != err) {
		return map_errcode(err);
	}

	kargs.ctx = NULL; /* prevent kernel address to be returned to user space */
	if (0 != copy_to_user(uargs, &kargs, sizeof(_mali_uk_get_pp_number_of_cores_s))) {
		return -EFAULT;
	}

	return 0;
}

int pp_get_core_version_wrapper(struct mali_session_data *session_data, _mali_uk_get_pp_core_version_s __user *uargs)
{
	_mali_uk_get_pp_core_version_s kargs;
	_mali_osk_errcode_t err;

	MALI_CHECK_NON_NULL(uargs, -EINVAL);
	MALI_CHECK_NON_NULL(session_data, -EINVAL);

	kargs.ctx = session_data;
	err = _mali_ukk_get_pp_core_version(&kargs);
	if (_MALI_OSK_ERR_OK != err) return map_errcode(err);

	if (0 != put_user(kargs.version, &uargs->version)) return -EFAULT;

	return 0;
}

int pp_disable_wb_wrapper(struct mali_session_data *session_data, _mali_uk_pp_disable_wb_s __user *uargs)
{
	_mali_uk_pp_disable_wb_s kargs;

	MALI_CHECK_NON_NULL(uargs, -EINVAL);
	MALI_CHECK_NON_NULL(session_data, -EINVAL);

	if (0 != copy_from_user(&kargs, uargs, sizeof(_mali_uk_pp_disable_wb_s))) return -EFAULT;

	kargs.ctx = session_data;
	_mali_ukk_pp_job_disable_wb(&kargs);

	return 0;
}


