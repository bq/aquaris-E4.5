 /* 
  *
  *
  * Copyright (C) 2008,2009 MediaTek <www.mediatek.com>
  * Authors: Infinity Chen <infinity.chen@mediatek.com>  
  *
  * This program is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation; either version 2 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program; if not, write to the Free Software
  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  */

#ifndef __MT_BOOT_COMMON_H__
#define __MT_BOOT_COMMON_H__

/* boot type definitions */
typedef enum 
{
    NORMAL_BOOT = 0,
    META_BOOT = 1,
    RECOVERY_BOOT = 2,    
    SW_REBOOT = 3,
    FACTORY_BOOT = 4,
    ADVMETA_BOOT = 5,
    ATE_FACTORY_BOOT = 6,
    ALARM_BOOT = 7,
#if defined (MTK_KERNEL_POWER_OFF_CHARGING)
    KERNEL_POWER_OFF_CHARGING_BOOT = 8,
    LOW_POWER_OFF_CHARGING_BOOT = 9,
#endif
    UNKNOWN_BOOT
} BOOTMODE;

typedef enum {
    BR_POWER_KEY = 0,
    BR_USB,
    BR_RTC,
    BR_WDT,
    BR_WDT_BY_PASS_PWK,
    BR_TOOL_BY_PASS_PWK,
    BR_2SEC_REBOOT,
    BR_UNKNOWN
} boot_reason_t;

extern BOOTMODE g_boot_mode;
extern BOOTMODE get_boot_mode(void);
extern bool is_meta_mode(void);
extern bool is_advanced_meta_mode(void);
extern void set_boot_mode(BOOTMODE bm);

extern boot_reason_t g_boot_reason;
extern boot_reason_t get_boot_reason(void);
extern void set_boot_reason(boot_reason_t br);


#endif 

