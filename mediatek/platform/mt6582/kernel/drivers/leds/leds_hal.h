#include "leds_sw.h"
#include <cust_leds.h>
#include <cust_leds_def.h>

#define AAAAAAAAAAAAAAAAAAAA 
/****************************************************************************
 * LED HAL functions
 ***************************************************************************/
extern void mt_leds_wake_lock_init(void);
extern unsigned int mt_get_bl_brightness(void);
extern unsigned int mt_get_bl_duty(void);
extern unsigned int mt_get_bl_div(void);
extern unsigned int mt_get_bl_frequency(void);
extern unsigned int *mt_get_div_array(void);
extern void mt_set_bl_duty(unsigned int level);
extern void mt_set_bl_div(unsigned int div);
extern void mt_set_bl_frequency(unsigned int freq);
extern void mt_led_pwm_disable(int pwm_num);
extern int mt_brightness_set_pmic_duty_store(u32 level, u32 div);
extern void mt_backlight_set_pwm_duty(int pwm_num, u32 level, u32 div, struct PWM_config *config_data);
extern void mt_backlight_set_pwm_div(int pwm_num, u32 level, u32 div, struct PWM_config *config_data);
extern void mt_backlight_get_pwm_fsel(unsigned int bl_div, unsigned int *bl_frequency);
extern void mt_store_pwm_register(unsigned int addr, unsigned int value);
extern unsigned int mt_show_pwm_register(unsigned int addr);
extern int mt_led_set_pwm(int pwm_num, struct nled_setting* led);
extern int mt_led_blink_pmic(enum mt65xx_led_pmic pmic_type, struct nled_setting* led);
extern int mt_backlight_set_pwm(int pwm_num, u32 level, u32 div, struct PWM_config *config_data);
extern int mt_brightness_set_pmic(enum mt65xx_led_pmic pmic_type, u32 level, u32 div);
extern int mt_mt65xx_led_set_cust(struct cust_mt65xx_led *cust, int level);
extern void mt_mt65xx_led_work(struct work_struct *work);
extern void mt_mt65xx_led_set(struct led_classdev *led_cdev, enum led_brightness level);
extern int  mt_mt65xx_blink_set(struct led_classdev *led_cdev,
			     unsigned long *delay_on,
			     unsigned long *delay_off);

struct cust_mt65xx_led* mt_get_cust_led_list(void);




