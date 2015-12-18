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

#include <asm/uaccess.h>
#include <asm/system.h>

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/err.h>
#include <mach/mt_typedefs.h>

//************************************
// Definition
//************************************
unsigned long (*mtk_thermal_get_gpu_loading_fp)(void) = NULL;


/* Init */
static int __init mtk_thermal_platform_init(void)
{
    int err = 0;
    return err;
}

/* Exit */
static void __exit mtk_thermal_platform_exit(void)
{
   
}


//*********************************************
// Export Interface
//*********************************************

EXPORT_SYMBOL(mtk_thermal_get_gpu_loading_fp);
module_init(mtk_thermal_platform_init);
module_exit(mtk_thermal_platform_exit);


