/*
 * (c) MediaTek Inc. 2010
 */


#include <linux/types.h>
#include <cust_acc.h>
#include <mach/mt_pm_ldo.h>

/*---------------------------------------------------------------------------*/
static struct acc_hw mc3xxx_cust_acc =
{
    .i2c_num   = 2,
    .direction = 4,// 6,
    .power_id  = MT65XX_POWER_NONE,    /* !< LDO is not used               */
    .power_vol = VOL_DEFAULT,          /* !< LDO is not used               */
    .firlen    = 0,                    /* !< don't enable low pass fileter */
};

/*---------------------------------------------------------------------------*/
struct acc_hw* mc3xxx_get_cust_acc_hw(void) 
{
    return (&mc3xxx_cust_acc);
}




