/* alps/ALPS_SW/TRUNK/MAIN/alps/kernel/arch/arm/mach-mt6516/include/mach/fm.h
 *
 * (C) Copyright 2009 
 * MediaTek <www.MediaTek.com>
 * William Chung <William.Chung@MediaTek.com>
 *
 * MT6516 AR10x0 FM Radio Driver
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

#ifndef __MTGPIO_H__
#define __MTGPIO_H__

#include <linux/ioctl.h>

#define GPIO_IOC_MAGIC 0x90

#define GPIO_IOCQMODE           _IOR(GPIO_IOC_MAGIC, 0x01, uint32_t)
#define GPIO_IOCTMODE0          _IOW(GPIO_IOC_MAGIC, 0x02, uint32_t)
#define GPIO_IOCTMODE1          _IOW(GPIO_IOC_MAGIC, 0x03, uint32_t)
#define GPIO_IOCTMODE2          _IOW(GPIO_IOC_MAGIC, 0x04, uint32_t)
#define GPIO_IOCTMODE3          _IOW(GPIO_IOC_MAGIC, 0x05, uint32_t)
#define GPIO_IOCQDIR            _IOR(GPIO_IOC_MAGIC, 0x06, uint32_t)
#define GPIO_IOCSDIRIN          _IOW(GPIO_IOC_MAGIC, 0x07, uint32_t)
#define GPIO_IOCSDIROUT         _IOW(GPIO_IOC_MAGIC, 0x08, uint32_t)
#define GPIO_IOCQPULLEN         _IOR(GPIO_IOC_MAGIC, 0x09, uint32_t)
#define GPIO_IOCSPULLENABLE     _IOW(GPIO_IOC_MAGIC, 0x0A, uint32_t)
#define GPIO_IOCSPULLDISABLE    _IOW(GPIO_IOC_MAGIC, 0x0B, uint32_t)
#define GPIO_IOCQPULL           _IOR(GPIO_IOC_MAGIC, 0x0C, uint32_t)
#define GPIO_IOCSPULLDOWN       _IOW(GPIO_IOC_MAGIC, 0x0D, uint32_t)
#define GPIO_IOCSPULLUP         _IOW(GPIO_IOC_MAGIC, 0x0E, uint32_t)
#define GPIO_IOCQINV            _IOR(GPIO_IOC_MAGIC, 0x0F, uint32_t)
#define GPIO_IOCSINVENABLE      _IOW(GPIO_IOC_MAGIC, 0x10, uint32_t)
#define GPIO_IOCSINVDISABLE     _IOW(GPIO_IOC_MAGIC, 0x11, uint32_t)
#define GPIO_IOCQDATAIN         _IOR(GPIO_IOC_MAGIC, 0x12, uint32_t)
#define GPIO_IOCQDATAOUT        _IOR(GPIO_IOC_MAGIC, 0x13, uint32_t)
#define GPIO_IOCSDATALOW        _IOW(GPIO_IOC_MAGIC, 0x14, uint32_t)
#define GPIO_IOCSDATAHIGH       _IOW(GPIO_IOC_MAGIC, 0x15, uint32_t)


#endif // __MTGPIO_H__
