#include <cust_leds.h>
#include <cust_leds_def.h>

/****************************************************************************
 * LED Variable Settings
 ***************************************************************************/
#define NLED_OFF 0
#define NLED_ON 1
#define NLED_BLINK 2
#define MIN_FRE_OLD_PWM 32 // the min frequence when use old mode pwm by kHz
#define PWM_DIV_NUM 8
#define ERROR_BL_LEVEL 0xFFFFFFFF

#ifdef RESPIRATION_LAMP
#include <linux/ioctl.h>
#define RGB_BLINK 'R'
#define RGB_BLINK_SET_LEVEL			_IOW(RGB_BLINK, 0x01, u32)
#define RGB_BLINK_SET_DELAY_ON		_IOW(RGB_BLINK, 0x02, int)
#define RGB_BLINK_SET_DELAY_OFF		_IOW(RGB_BLINK, 0x03, int)
#define RGB_BLINK_START_BLINKING		_IO(RGB_BLINK, 0x04)
#endif

struct nled_setting
{
	u8 nled_mode; //0, off; 1, on; 2, blink;
	u32 blink_on_time ;
	u32 blink_off_time;
	#ifdef RESPIRATION_LAMP
	u32 level;
	#endif
};
 
typedef enum{  
    PMIC_PWM_0 = 0,  
    PMIC_PWM_1 = 1,  
    PMIC_PWM_2 = 2
} MT65XX_PMIC_PWM_NUMBER;

typedef enum{      
	ISINK_PWM_MODE = 0,      
	ISINK_BREATH_MODE = 1,      
	ISINK_REGISTER_MODE = 2
} MT65XX_PMIC_ISINK_MODE;

/*****************PWM *************************************************/
//extern int time_array[PWM_DIV_NUM];
//extern u8 div_array[PWM_DIV_NUM]; //defined in leds_sw.h
//extern unsigned int backlight_PWM_div;// this para come from cust_leds.

/****************************************************************************
 * structures
 ***************************************************************************/
struct mt65xx_led_data {
	struct led_classdev cdev;
	struct cust_mt65xx_led cust;
	struct work_struct work;
	int level;
	int delay_on;
	int delay_off;
};

