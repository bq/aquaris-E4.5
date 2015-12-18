#ifndef TOUCHPANEL_H__
#define TOUCHPANEL_H__

/* Pre-defined definition */
#define TPD_TYPE_CAPACITIVE
#define TPD_TYPE_RESISTIVE
#define TPD_I2C_NUMBER           0
#define TPD_WAKEUP_TRIAL         60
#define TPD_WAKEUP_DELAY         100

#define VELOCITY_CUSTOM
#define TPD_VELOCITY_CUSTOM_X 15
#define TPD_VELOCITY_CUSTOM_Y 15

//#define TPD_CLOSE_POWER_IN_SLEEP
#define TPD_POWER_SOURCE_CUSTOM         MT6323_POWER_LDO_VGP1

#define TPD_DELAY                (2*HZ/100)
//#define TPD_RES_X                480
//#define TPD_RES_Y                800
#define TPD_CALIBRATION_MATRIX  {962,0,0,0,1600,0,0,0};
//#define TPD_CUSTOM_CALIBRATION
//#define TPD_HAVE_CALIBRATION
//#define TPD_HAVE_BUTTON
//#define TPD_HAVE_TREMBLE_ELIMINATION
#define TPD_HAVE_BUTTON
#define TPD_BUTTON_HEIGH        (100)
#define TPD_KEY_COUNT           3
#define TPD_KEYS                { KEY_APP_SWITCH, KEY_HOMEPAGE ,KEY_BACK}
#define TPD_KEYS_DIM            {{80,1020,100,100},{240,1020,100,100},{400,1020,100,100}}


//#define FTS_GESTRUE			//双击唤醒
#define FTS_PRESSURE		//压力和面积
#define LCM_NAME1 "hx8389_qhd_dsi_vdo_truly"
#define LCM_NAME2 "hx8389b_qhd_dsi_vdo_tianma"

static unsigned char CTPM_FW[]=
{
//#include "FT5336_HiKe_Vegeta_OGS_720X1280_Truly0x5a_Ver0x10_20140307_app.i"
//#include "FT5336_HiKe_Vegeta_OGS_720X1280_Truly0x5a_Ver0x11_20140324_app.i"
//#include "FT5336_HiKe_Vegeta_OGS_720X1280_Truly0x5a_Ver0x12_20140331_app.i"
//#include "FT5336_HiKe_Vegeta_OGS_720X1280_Truly0x5a_Ver0x13_20140403_app.i"
//#include "FT5336_HiKe_Vegeta_OGS_720X1280_Truly0x5a_Ver0x14_20140521_app.i"
//#include "FT5336_HiKe_Vegeta_OGS_720X1280_Truly0x5a_Ver0x15_20140528_app.i"
//#include "FT5336_HiKe_Vegeta_OGS_720X1280_Truly0x5a_Ver0x16_20140618_app.i"
#include "FT5336_HiKe_Krilin_OGS_540X960_Truly0x5a_Ver0x15_20140618_app.i"
};

static unsigned char CTPM_FW2[]=
{
//#include "FT5336_HiKe_Vegeta_OGS_720X1280_TianMa0x55_Ver0x12_20140528_app.i"
#include "FT5336_Hike_Krilin_540X960_LaiBao0x55_Ver0x12_20140729_app.i"
};
#endif /* TOUCHPANEL_H__ */
