/*
 * Charging IC driver (rt9536)
 *
 * Copyright (C) 2010 LGE, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef __LINUX_RT9536_CHARGER_H
#define __LINUX_RT9536_CHARGER_H

#define CHG_EN_SET_N                    GPIO_SWCHARGER_EN_PIN
#define CHG_EN_MODE                     GPIO_SWCHARGER_EN_PIN_M_GPIO
#define CHG_EN_DIR                      GPIO41_DIR
#define CHG_EN_DATA_OUT                 GPIO41_DATAOUT
 //#define CHG_EN_PULL_ENABLE        GPIO35_PULLEN
 //#define CHG_EN_PULL_SELECT        GPIO35_PULL


 #define CHG_EOC_N                      GPIO_EINT_CHG_STAT_PIN
 #define CHG_EOC_MODE                   GPIO_EINT_CHG_STAT_PIN_M_GPIO
 #define CHG_EOC_DIR                    GPIO0_DIR
 #define CHG_EOC_PULL_ENABLE            GPIO0_PULLEN
 #define CHG_EOC_PULL_SELECT            GPIO0_PULL

/* Function Prototype */
//enum power_supply_type get_charging_ic_status(void);

extern void charging_ic_active_default(void);
//void charging_ic_set_ta_mode(void);
//void charging_ic_set_usb_mode(void);
extern void charging_ic_deactive(void);
//void charging_ic_set_factory_mode(void);
extern void rt9536_charging_enable(unsigned int set_current, unsigned int enable);
extern unsigned char rt9536_check_eoc_status(void);

#endif /* __LINUX_MAX8971_CHARGER_H */
