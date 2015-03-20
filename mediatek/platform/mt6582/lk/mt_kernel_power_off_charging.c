
/*
 * (C) Copyright 2008
 * MediaTek <www.mediatek.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <printf.h>
#include <platform/mt_typedefs.h>
#include <platform/mt_rtc.h>
#include <platform/boot_mode.h>
#include <platform/mtk_wdt.h>
#include <platform/mt_pmic.h>

extern BOOL meta_mode_check(void);
extern int mtk_wdt_boot_check(void);
extern bool rtc_2sec_boot_check(void);

BOOL is_force_boot(void)
{
	if (rtc_boot_check(true))
	{
		printf("[%s] Bypass Kernel Power off charging mode and enter Alarm Boot\n", __func__);
		return TRUE;
	}
	else if (meta_mode_check())
	{
		printf("[%s] Bypass Kernel Power off charging mode and enter Meta Boot\n", __func__);
		return TRUE;	
	}
	else if (pmic_detect_powerkey() || mtk_wdt_boot_check()==WDT_BY_PASS_PWK_REBOOT || rtc_2sec_boot_check())			
	{
		printf("[%s] Bypass Kernel Power off charging mode and enter Normal Boot\n", __func__);
		g_boot_mode = NORMAL_BOOT;
		return TRUE;
	}
	
	return FALSE;

}


BOOL kernel_power_off_charging_detection(void)
{
    /* */
    if(is_force_boot()) {
        upmu_set_rg_chrind_on(0);
		printf("[%s] Turn off HW Led\n", __func__);
        return FALSE;
    }

    if((upmu_is_chr_det() == KAL_TRUE)) {
        g_boot_mode = KERNEL_POWER_OFF_CHARGING_BOOT;
		return TRUE;
    }
    else {
        /* power off */
        #ifndef NO_POWER_OFF
        printf("[kernel_power_off_charging_detection] power off\n");
        mt6575_power_off();        
        #endif
		return FALSE;	
    }
    /* */
}







