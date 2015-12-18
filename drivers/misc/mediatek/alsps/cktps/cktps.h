/* 
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
/*
 * Definitions for tmd2771 als/ps sensor chip.
 */
#ifndef __CKTPS_H__
#define __CKTPS_H__

#include <linux/ioctl.h>

extern int CKTPS_CMM_PPCOUNT_VALUE;
extern int ZOOM_TIME;
extern int CKTPS_CMM_CONTROL_VALUE;

#define CKTPS_CMM_ENABLE 		0X80
#define CKTPS_CMM_ATIME 		0X81
#define CKTPS_CMM_PTIME 		0X82
#define CKTPS_CMM_WTIME 		0X83
/*for interrup work mode support -- by liaoxl.lenovo 12.08.2011*/
#define CKTPS_CMM_INT_LOW_THD_LOW   0X88
#define CKTPS_CMM_INT_LOW_THD_HIGH  0X89
#define CKTPS_CMM_INT_HIGH_THD_LOW  0X8A
#define CKTPS_CMM_INT_HIGH_THD_HIGH 0X8B
#define CKTPS_CMM_Persistence       0X8C
#define CKTPS_CMM_STATUS            0X93
#define TAOS_TRITON_CMD_REG           0X80
#define TAOS_TRITON_CMD_SPL_FN        0x60

#define CKTPS_CMM_CONFIG 		0X8D
#define CKTPS_CMM_PPCOUNT 		0X8E
#define CKTPS_CMM_CONTROL 		0X8F

#define CKTPS_CMM_PDATA_L 		0X98
#define CKTPS_CMM_PDATA_H 		0X99
#define CKTPS_CMM_C0DATA_L 	0X94
#define CKTPS_CMM_C0DATA_H 	0X95
#define CKTPS_CMM_C1DATA_L 	0X96
#define CKTPS_CMM_C1DATA_H 	0X97


#define CKTPS_SUCCESS						0
#define CKTPS_ERR_I2C						-1
#define CKTPS_ERR_STATUS					-3
#define CKTPS_ERR_SETUP_FAILURE				-4
#define CKTPS_ERR_GETGSENSORDATA			-5
#define CKTPS_ERR_IDENTIFICATION			-6


#define CKTPS_AUTO_DETECT
#define TP_PROXIMITY_SENSOR_NEW

#endif
