#include <linux/types.h>
#include <mach/mt_pm_ldo.h>
#include <cust_alsps.h>
//#include <mach/mt6575_pm_ldo.h>

static struct alsps_hw cust_alsps_hw = {
	.i2c_num    = 0,
	.polling_mode_ps =0,
	.polling_mode_als =1,
#if defined(CKT_LOW_POWER_SUPPORT)
    .power_id   = MT65XX_POWER_LDO_VIO28, //MT65XX_POWER_LDO_VIO28,//MT65XX_POWER_NONE,    /*LDO is not used*/
    .power_vol  = VOL_2800,//VOL_DEFAULT,          /*LDO is not used*/
#else
    .power_id   = MT65XX_POWER_NONE,    /*LDO is not used*/
    .power_vol  = VOL_DEFAULT,          /*LDO is not used*/
#endif
    .i2c_addr   = {0x72, 0x48, 0x78, 0x00},
    /*Lenovo-sw chenlj2 add 2011-06-03,modify parameter below two lines*/
   .als_level  = { 1,  2,  3,  4,  6,  8,  10,   13,  16,   20,   25,  30,   36,   42,  48,  54},
  .als_value  = { 15, 100, 140, 180, 240, 280, 1600,  1600,  2800,  2800,  4000,  4000,   9000, 9000,  9999, 10240},
    //.als_level  = { 4, 40,  80,   120,   160, 250,  400, 800, 1200,  1600, 2000, 3000, 5000, 10000, 65535},
    //.als_value  = {10, 20,20,  120, 120, 280,  280,  280, 1600,  1600,  1600,  6000,  6000, 9000,  10240, 10240},
    .ps_threshold_high = 400,
    .ps_threshold_low = 200,
    .ps_threshold = 400,
};
struct alsps_hw *get_cust_alsps_hw(void) {
    return &cust_alsps_hw;
}

int ZOOM_TIME = 60; // 4;


