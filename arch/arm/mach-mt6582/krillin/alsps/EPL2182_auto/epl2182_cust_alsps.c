#include <linux/types.h>
#include <mach/mt_pm_ldo.h>
#include <cust_alsps.h>

static struct alsps_hw cust_alsps_hw = {
    .i2c_num    = 0,
	.polling_mode_ps =0,
	.polling_mode_als =1,
    .power_id   = MT65XX_POWER_NONE,    /*LDO is not used*/
    .power_vol  = VOL_DEFAULT,          /*LDO is not used*/
	.i2c_addr   = {0x0C, 0x48, 0x78, 0x00},
    //.als_level  =  {0,  3,    9,    22,   40,    70,    140,   280,   560,  650,  1900,  3000,  10000,  20000},
    //.als_value  = {0,  18,  60,  100,  220,  230,  250,   300,   500,  560,  1200,  2600,  10240,   10240},
    //.als_level  =  {0,   9,    22,    30,   40,    70,    140,   280,   560,  650,  1900,  3000,  10000,  20000},
    //.als_value  = {0,  18,    60,  100,  220,  230,  250,   300,   500,  560,  1200,  2600,  10240,   10240},
    .als_level	= {20, 45, 70, 90, 150, 300, 500, 700, 1150, 2250, 4500, 8000, 15000, 30000, 50000},
	.als_value	= {10, 30, 60, 80, 100, 200, 400, 600, 800, 1500, 3000, 6000, 10000, 20000, 40000, 60000},
	.ps_threshold_high = 100,
    .ps_threshold_low = 500,
};
struct alsps_hw *EPL2182_get_cust_alsps_hw(void) {
    return &cust_alsps_hw;
}

