/*
* Copyright (C) 2011-2014 MediaTek Inc.
* 
* This program is free software: you can redistribute it and/or modify it under the terms of the 
* GNU General Public License version 2 as published by the Free Software Foundation.
* 
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _MT_CPUFREQ_H
#define _MT_CPUFREQ_H

#include <linux/module.h>

enum mt_cpu_dvfs_id {
    MT_CPU_DVFS_LITTLE,
    MT_CPU_DVFS_BIG,

    NR_MT_CPU_DVFS,
};

/*********************
* Clock Mux Register
**********************/
#define TOP_CKMUXSEL    (0xF0001000)
#define TOP_CKDIV1_CPU  (0xF0001008)

/****************************
* PMIC Wrapper DVFS Register
*****************************/
#define PWRAP_BASE              (0xF000D000)
#define PMIC_WRAP_DVFS_ADR0     (PWRAP_BASE + 0xE4)
#define PMIC_WRAP_DVFS_WDATA0   (PWRAP_BASE + 0xE8)
#define PMIC_WRAP_DVFS_ADR1     (PWRAP_BASE + 0xEC)
#define PMIC_WRAP_DVFS_WDATA1   (PWRAP_BASE + 0xF0)
#define PMIC_WRAP_DVFS_ADR2     (PWRAP_BASE + 0xF4)
#define PMIC_WRAP_DVFS_WDATA2   (PWRAP_BASE + 0xF8)
#define PMIC_WRAP_DVFS_ADR3     (PWRAP_BASE + 0xFC)
#define PMIC_WRAP_DVFS_WDATA3   (PWRAP_BASE + 0x100)
#define PMIC_WRAP_DVFS_ADR4     (PWRAP_BASE + 0x104)
#define PMIC_WRAP_DVFS_WDATA4   (PWRAP_BASE + 0x108)
#define PMIC_WRAP_DVFS_ADR5     (PWRAP_BASE + 0x10C)
#define PMIC_WRAP_DVFS_WDATA5   (PWRAP_BASE + 0x110)
#define PMIC_WRAP_DVFS_ADR6     (PWRAP_BASE + 0x114)
#define PMIC_WRAP_DVFS_WDATA6   (PWRAP_BASE + 0x118)
#define PMIC_WRAP_DVFS_ADR7     (PWRAP_BASE + 0x11C)
#define PMIC_WRAP_DVFS_WDATA7   (PWRAP_BASE + 0x120)

/*****************
* extern function 
******************/
extern int mt_cpufreq_state_set(int enabled);
extern void mt_cpufreq_thermal_protect(unsigned int limited_power);
void mt_cpufreq_enable_by_ptpod(void);
unsigned int mt_cpufreq_disable_by_ptpod(void);
extern unsigned int mt_cpufreq_max_frequency_by_DVS(unsigned int num);
void mt_cpufreq_return_default_DVS_by_ptpod(void);
extern bool mt_cpufreq_earlysuspend_status_get(void);
#endif