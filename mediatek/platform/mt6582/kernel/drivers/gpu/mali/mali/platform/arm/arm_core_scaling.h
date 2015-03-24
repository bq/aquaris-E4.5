/*
 * (c) ARM Limited 2008-2011, 2013
 */



#ifndef __ARM_CORE_SCALING_H__
#define __ARM_CORE_SCALING_H__

struct mali_gpu_utilization_data;

/**
 * Initialize core scaling policy.
 *
 * @note The core scaling policy will assume that all PP cores are on initially.
 *
 * @param num_pp_cores Total number of PP cores.
 */
void mali_core_scaling_init(int num_pp_cores);

/**
 * Terminate core scaling policy.
 */
void mali_core_scaling_term(void);

/**
 * Update core scaling policy with new utilization data.
 *
 * @param data Utilization data.
 */
void mali_core_scaling_update(struct mali_gpu_utilization_data *data);

void mali_core_scaling_sync(int num_cores);

#endif /* __ARM_CORE_SCALING_H__ */




