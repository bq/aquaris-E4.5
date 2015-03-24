/*
 * Copyright (C) 2010 MediaTek, Inc.
 *
 * Author: Terry Chang <terry.chang@mediatek.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


/*kpd.h file path: ALPS/mediatek/kernel/include/linux */
#include <linux/kpd.h>

#define KPD_NAME	"mtk-kpd"
#define MTK_KP_WAKESOURCE//this is for auto set wake up source
#include "cust_kpd.h"
struct input_dev *kpd_input_dev;
static bool kpd_suspend = false;
static int kpd_show_hw_keycode = 1;
static int kpd_show_register = 1;
static volatile int call_status = 0;
static int incall = 0;//this is for whether phone in call state judgement when resume

/*for kpd_memory_setting() function*/
static u16 kpd_keymap[KPD_NUM_KEYS];
static u16 kpd_keymap_state[KPD_NUM_MEMS];
/***********************************/

/* for slide QWERTY */
#if KPD_HAS_SLIDE_QWERTY
static void kpd_slide_handler(unsigned long data);
static DECLARE_TASKLET(kpd_slide_tasklet, kpd_slide_handler, 0);
static u8 kpd_slide_state = CUST_EINT_MHALL_TYPE;
#endif

#define WLC_CHR_USE_EINT 0
#if WLC_CHR_USE_EINT
static void wlc_chr_handler(unsigned long data);
static DECLARE_TASKLET(wlc_chr_tasklet, wlc_chr_handler, 0);
static u8 wlc_chr_state = !CUST_EINT_CHR_STAT_TYPE;
#endif
/* for Power key using EINT */
#if KPD_PWRKEY_USE_EINT
static void kpd_pwrkey_handler(unsigned long data);
static DECLARE_TASKLET(kpd_pwrkey_tasklet, kpd_pwrkey_handler, 0);
#endif
#define CKT_HALL_SWITCH_SUPPORT 1
#if CKT_HALL_SWITCH_SUPPORT
#include <linux/kthread.h>
void wake_up_hall_switch(void);
void hall_switch_eint_irq(void);
static int hall_switch_thread_kthread(void *x);
 void hall_switch_handler(unsigned long data);
static DECLARE_TASKLET(hall_switch_tasklet, hall_switch_handler, 0);
extern unsigned int mt_eint_get_polarity(unsigned int eint_num);
extern unsigned int mt_eint_get_sens(unsigned int eint_num);
extern unsigned int mt_eint_get_soft(unsigned int eint_num);
unsigned int mt_eint_get_status(unsigned int eint_num);
#define EINT_PIN_CLOSE        (1)
#define EINT_PIN_FARAWAY       (0)
int cur_hall_eint_state = EINT_PIN_FARAWAY;
int cur_hall_light_level = 0;
extern long tmd2772_enable_ps(struct i2c_client *client, int enable);
extern long tmd2772_read_ps(struct i2c_client *client, u16 *data);
int g_hall_switch_value =0;
extern struct i2c_client *tmd2772_i2c_client;
static DEFINE_MUTEX(hall_switch_mutex);
static DECLARE_WAIT_QUEUE_HEAD(hall_switch_thread_wq);
int hall_switch_thread_timeout=0;
int g_is_calling=0;
int hall_switch_if_in_front(void);
#endif
/* for keymap handling */
static void kpd_keymap_handler(unsigned long data);
static DECLARE_TASKLET(kpd_keymap_tasklet, kpd_keymap_handler, 0);

/*********************************************************************/
static void kpd_memory_setting(void);

/*********************************************************************/
static int kpd_pdrv_probe(struct platform_device *pdev);
static int kpd_pdrv_remove(struct platform_device *pdev);
#ifndef USE_EARLY_SUSPEND	
static int kpd_pdrv_suspend(struct platform_device *pdev, pm_message_t state);
static int kpd_pdrv_resume(struct platform_device *pdev);
#endif


static struct platform_driver kpd_pdrv = {
	.probe		= kpd_pdrv_probe,
	.remove		= kpd_pdrv_remove,
#ifndef USE_EARLY_SUSPEND	
	.suspend	= kpd_pdrv_suspend,
	.resume		= kpd_pdrv_resume,
#endif	
	.driver		= {
		.name	= KPD_NAME,
		.owner	= THIS_MODULE,
	},
};
/********************************************************************/
static void kpd_memory_setting(void)
{
	kpd_init_keymap(kpd_keymap);
	kpd_init_keymap_state(kpd_keymap_state);
	return;
}


/*****************for kpd auto set wake up source*************************/

static ssize_t kpd_store_call_state(struct device_driver *ddri, const char *buf, size_t count)
{
	if (sscanf(buf, "%u", &call_status) != 1) {
			kpd_print("kpd call state: Invalid values\n");
			return -EINVAL;
		}

	switch(call_status)
    	{
        	case 1 :
			kpd_print("kpd call state: Idle state!\n");
     		break;
		case 2 :
			kpd_print("kpd call state: ringing state!\n");
		break;
		case 3 :
			kpd_print("kpd call state: active or hold state!\n");	
		break;
            
		default:
   			kpd_print("kpd call state: Invalid values\n");
        	break;
  	}
	return count;
}

static ssize_t kpd_show_call_state(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	res = snprintf(buf, PAGE_SIZE, "%d\n", call_status);     
	return res;   
}
static DRIVER_ATTR(kpd_call_state,	S_IWUSR | S_IRUGO,	kpd_show_call_state,	kpd_store_call_state);

static struct driver_attribute *kpd_attr_list[] = {
	&driver_attr_kpd_call_state,
};

/*----------------------------------------------------------------------------*/
static int kpd_create_attr(struct device_driver *driver) 
{
	int idx, err = 0;
	int num = (int)(sizeof(kpd_attr_list)/sizeof(kpd_attr_list[0]));
	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if((err = driver_create_file(driver, kpd_attr_list[idx])))
		{            
			kpd_print("driver_create_file (%s) = %d\n", kpd_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}
/*----------------------------------------------------------------------------*/
static int kpd_delete_attr(struct device_driver *driver)
	{
	int idx ,err = 0;
	int num = (int)(sizeof(kpd_attr_list)/sizeof(kpd_attr_list[0]));

	if (!driver)
	return -EINVAL;

	for (idx = 0; idx < num; idx++) 
	{
		driver_remove_file(driver, kpd_attr_list[idx]);
	}
	
	return err;
}
/*----------------------------------------------------------------------------*/
/********************************************************************************************/
/************************************************************************************************************************************************/
/* for autotest */
#if KPD_AUTOTEST
static const u16 kpd_auto_keymap[] = {
	KEY_MENU,
	KEY_HOME, KEY_BACK,
	KEY_CALL, KEY_ENDCALL,
	KEY_VOLUMEUP, KEY_VOLUMEDOWN,
	KEY_FOCUS, KEY_CAMERA,
};
#endif
/* for AEE manual dump */
#define AEE_VOLUMEUP_BIT	0
#define AEE_VOLUMEDOWN_BIT	1
#define AEE_DELAY_TIME		15
/* enable volup + voldown was pressed 5~15 s Trigger aee manual dump */
#define AEE_ENABLE_5_15		1
static struct hrtimer aee_timer;
static unsigned long  aee_pressed_keys;
static bool aee_timer_started;

#if AEE_ENABLE_5_15
#define AEE_DELAY_TIME_5S	5
static struct hrtimer aee_timer_5s;
static bool aee_timer_5s_started;
static bool flags_5s;
#endif

static inline void kpd_update_aee_state(void) {
	if(aee_pressed_keys == ((1<<AEE_VOLUMEUP_BIT) | (1<<AEE_VOLUMEDOWN_BIT))) {
		/* if volumeup and volumedown was pressed the same time then start the time of ten seconds */
		aee_timer_started = true;
		
#if AEE_ENABLE_5_15
		aee_timer_5s_started = true;
		hrtimer_start(&aee_timer_5s, 
				ktime_set(AEE_DELAY_TIME_5S, 0),
				HRTIMER_MODE_REL);
#endif
		hrtimer_start(&aee_timer, 
				ktime_set(AEE_DELAY_TIME, 0),
				HRTIMER_MODE_REL);
		kpd_print("aee_timer started\n");
	} else {
		if(aee_timer_started) {
/*
  * hrtimer_cancel - cancel a timer and wait for the handler to finish.
  * Returns:
  *	0 when the timer was not active. 
  *	1 when the timer was active.
 */
			if(hrtimer_cancel(&aee_timer))
			{
				kpd_print("try to cancel hrtimer \n");
#if AEE_ENABLE_5_15
				if(flags_5s)
				{
					printk("Pressed Volup + Voldown5s~15s then trigger aee manual dump.\n");
					aee_kernel_reminding("manual dump", "Trigger Vol Up +Vol Down 5s");
				}
#endif
					
			}
#if AEE_ENABLE_5_15
			flags_5s = false;
#endif
			aee_timer_started = false;
			kpd_print("aee_timer canceled\n");
		}

#if AEE_ENABLE_5_15
		if(aee_timer_5s_started) {
/*
  * hrtimer_cancel - cancel a timer and wait for the handler to finish.
  * Returns:
  *	0 when the timer was not active. 
  *	1 when the timer was active.
 */
			if(hrtimer_cancel(&aee_timer_5s))
			{
				kpd_print("try to cancel hrtimer (5s) \n");
			}
			aee_timer_5s_started = false;
			kpd_print("aee_timer canceled (5s)\n");
		}

#endif
	}
}

static void kpd_aee_handler(u32 keycode, u16 pressed) {
	if(pressed) {
		if(keycode == KEY_VOLUMEUP) {
			__set_bit(AEE_VOLUMEUP_BIT, &aee_pressed_keys);
		} else if(keycode == KEY_VOLUMEDOWN) {
			__set_bit(AEE_VOLUMEDOWN_BIT, &aee_pressed_keys);
		} else {
			return;
		}
		kpd_update_aee_state();
	} else {
		if(keycode == KEY_VOLUMEUP) {
			__clear_bit(AEE_VOLUMEUP_BIT, &aee_pressed_keys);
		} else if(keycode == KEY_VOLUMEDOWN) {
			__clear_bit(AEE_VOLUMEDOWN_BIT, &aee_pressed_keys);
		} else {
			return;
		}
		kpd_update_aee_state();
	}
}

static enum hrtimer_restart aee_timer_func(struct hrtimer *timer) {
	//printk("kpd: vol up+vol down AEE manual dump!\n");
	//aee_kernel_reminding("manual dump ", "Triggered by press KEY_VOLUMEUP+KEY_VOLUMEDOWN");
	aee_trigger_kdb();
	return HRTIMER_NORESTART;
}

#if AEE_ENABLE_5_15
static enum hrtimer_restart aee_timer_5s_func(struct hrtimer *timer) {
	
	//printk("kpd: vol up+vol down AEE manual dump timer 5s !\n");
	flags_5s = true;
	return HRTIMER_NORESTART;
}
#endif

/************************************************************************************************************************************************/

#if KPD_HAS_SLIDE_QWERTY
extern int get_suspend_state();
static int system_suspend_state=false;
static void kpd_slide_handler(unsigned long data)
{
	bool slid;
	u8 old_state = kpd_slide_state;

	kpd_slide_state = !kpd_slide_state;
	slid = (kpd_slide_state == !!CUST_EINT_MHALL_TYPE);
	/* for SW_LID, 1: lid open => slid, 0: lid shut => closed */
	//input_report_switch(kpd_input_dev, SW_LID, slid);
	printk("hall report QWERTY = %s\n", slid ? "far away" : "closed");
	printk("hall cur_hall_light_level = %d\n", cur_hall_light_level);
if((0==slid)&&(0 != cur_hall_light_level)&&(0==g_is_calling))
{
				input_report_key(kpd_input_dev, KPD_PWRKEY_MAP, 1);
	input_sync(kpd_input_dev);

				udelay(10);
				input_report_key(kpd_input_dev, KPD_PWRKEY_MAP, 0);
				input_sync(kpd_input_dev);
				cur_hall_eint_state =  EINT_PIN_CLOSE;
		}
		else if((1==slid)&&(0 == cur_hall_light_level)&&(0==g_is_calling))//if((cur_hall_eint_state == EINT_PIN_FARAWAY )&&(0 != cur_hall_light_level))
			{
			input_report_key(kpd_input_dev, KPD_PWRKEY_MAP, 1);
				input_sync(kpd_input_dev);
				udelay(10);
				input_report_key(kpd_input_dev, KPD_PWRKEY_MAP, 0);
				input_sync(kpd_input_dev);
				cur_hall_eint_state =  EINT_PIN_FARAWAY;
	}
	/* for detecting the return to old_state */
	mt_eint_set_polarity(CUST_EINT_MHALL_NUM, old_state);
	mt_eint_unmask(CUST_EINT_MHALL_NUM);
}

static void kpd_slide_eint_handler(void)
{
	tasklet_schedule(&kpd_slide_tasklet);
}
#endif

#if WLC_CHR_USE_EINT
//extern void open_blue_led(int level);
//extern void open_red_led(int level);
int ckt_g_wlc_chr_state;
void ckt_wlc_chr_start(void)
	{
	return;
	}
void ckt_wlc_chr_end(void)
	{
	return;
	}
static void wlc_chr_handler(unsigned long data)
{
	bool slid;
	u8 old_state = wlc_chr_state;
	wlc_chr_state = !wlc_chr_state;
	slid = (wlc_chr_state == !!CUST_EINT_CHR_STAT_TYPE);
if(0==slid)
           {
           ckt_g_wlc_chr_state = 1;
		   ckt_wlc_chr_start();
         // open_blue_led(255);
		}
		else if(1==slid)//if((cur_hall_eint_state == EINT_PIN_FARAWAY )&&(0 != cur_hall_light_level))
			{			
			//open_blue_led(0);
			ckt_wlc_chr_end();
			ckt_g_wlc_chr_state = 0;
	}
	mt_eint_set_polarity(CUST_EINT_CHR_STAT_NUM, old_state);
	mt_eint_unmask(CUST_EINT_CHR_STAT_NUM);
}
static void wlc_chr_eint_handler(void)
{
	tasklet_schedule(&wlc_chr_tasklet);
}
#endif
#if KPD_PWRKEY_USE_EINT
static void kpd_pwrkey_handler(unsigned long data)
{
	kpd_pwrkey_handler_hal(data);
}

static void kpd_pwrkey_eint_handler(void)
{
	tasklet_schedule(&kpd_pwrkey_tasklet);
}
#endif
#if  CKT_HALL_SWITCH_SUPPORT
void wake_up_hall_switch(void)
{
    hall_switch_thread_timeout = 1;
    wake_up(&hall_switch_thread_wq);
}
EXPORT_SYMBOL(wake_up_hall_switch);
void hall_switch_eint_irq(void)
{
  printk( "hall_switch_eint_irq enter\n");
if(cur_hall_eint_state ==  EINT_PIN_CLOSE ) 
	{
		printk( "hall_switch_eint_irq old cur_hall_eint_state ==  EINT_PIN_CLOSE\n");
		if (CUST_EINT_MHALL_TYPE){
					mt_eint_set_polarity(CUST_EINT_MHALL_NUM, (1));
		}else{
					mt_eint_set_polarity(CUST_EINT_MHALL_NUM, (0));
		}
        mt_eint_set_hw_debounce(CUST_EINT_MHALL_NUM, CUST_EINT_MHALL_DEBOUNCE_CN);
		cur_hall_eint_state = EINT_PIN_FARAWAY;
	} 
	else 
	{
		printk( "hall_switch_eint_irq old cur_hall_eint_state ==  EINT_PIN_FARAWAY\n");	
		if (CUST_EINT_MHALL_TYPE){
				mt_eint_set_polarity(CUST_EINT_MHALL_NUM, !(1));
		}else{
				mt_eint_set_polarity(CUST_EINT_MHALL_NUM, !(0));
		}
        mt_eint_set_hw_debounce(CUST_EINT_MHALL_NUM, CUST_EINT_MHALL_DEBOUNCE_CN);
		cur_hall_eint_state = EINT_PIN_CLOSE;
	}
if (cur_hall_eint_state == EINT_PIN_CLOSE)	
  printk( "hall_switch_eint_irq new cur_hall_eint_state = =EINT_PIN_CLOSE\n");
else if (cur_hall_eint_state == EINT_PIN_FARAWAY)
  printk( "hall_switch_eint_irq new cur_hall_eint_state = =EINT_PIN_FARAWAY\n"); 	
    wake_up_hall_switch();
    return ;
}
static int hall_switch_thread_kthread(void *x)
{
    kal_uint32 ret=0;
    printk("[hall_switch_thread_kthread] enter\n");
    while (1) {
   mutex_lock(&hall_switch_mutex);
   printk("[hall_switch_thread_kthread] running\n");
if ((cur_hall_eint_state ==  EINT_PIN_CLOSE )&&(cur_hall_light_level != 0))
		{
	kpd_pwrkey_pmic_handler(1);
	msleep(50);
	kpd_pwrkey_pmic_handler(0);
	printk("hall_switch close level =%d\n",cur_hall_light_level);
		}
	else if((cur_hall_eint_state ==  EINT_PIN_FARAWAY)&&(cur_hall_light_level == 0))
		{
	kpd_pwrkey_pmic_handler(1);
	msleep(50);
	kpd_pwrkey_pmic_handler(0); 
	printk("hall_switch faraway level =%d\n",cur_hall_light_level);
		}
	mt_eint_unmask(CUST_EINT_MHALL_NUM);
        mutex_unlock(&hall_switch_mutex);
        wait_event(hall_switch_thread_wq, hall_switch_thread_timeout);
        hall_switch_thread_timeout=0;
    }
    return 0;
}
#if 1
static void kpd_slide_handler_00(unsigned long data)
{
      unsigned int ret,ret1,ret2,ret3;
	if(cur_hall_eint_state ==  EINT_PIN_CLOSE ) 
	{
		if (CUST_EINT_MHALL_TYPE){
					mt_eint_set_polarity(CUST_EINT_MHALL_NUM, (1));
		}else{
					mt_eint_set_polarity(CUST_EINT_MHALL_NUM, (0));
		}
        mt_eint_set_hw_debounce(CUST_EINT_MHALL_NUM, CUST_EINT_MHALL_DEBOUNCE_CN);
		cur_hall_eint_state = EINT_PIN_FARAWAY;
	} 
	else 
	{
		if (CUST_EINT_MHALL_TYPE){
				mt_eint_set_polarity(CUST_EINT_MHALL_NUM, !(1));
		}else{
				mt_eint_set_polarity(CUST_EINT_MHALL_NUM, !(0));
		}
        mt_eint_set_hw_debounce(CUST_EINT_MHALL_NUM, 256);
		cur_hall_eint_state = EINT_PIN_CLOSE;
	}
    ret=mt_eint_get_polarity(CUST_EINT_MHALL_NUM);
	if ((cur_hall_eint_state ==  EINT_PIN_CLOSE )&&(cur_hall_light_level != 0))
		{
	kpd_pwrkey_pmic_handler(1);
	kpd_pwrkey_pmic_handler(0);
	printk("hall_switch close level =%d\n",cur_hall_light_level);
		}
	else if((cur_hall_eint_state ==  EINT_PIN_FARAWAY)&&(cur_hall_light_level == 0))
		{
	kpd_pwrkey_pmic_handler(1);
	kpd_pwrkey_pmic_handler(0); 
	printk("hall_switch faraway level =%d\n",cur_hall_light_level);
		}
	mt_eint_unmask(CUST_EINT_MHALL_NUM);
}
static void hall_switch_eint_handler(void)
{
	tasklet_schedule(&hall_switch_tasklet);
}
#endif
#endif
/*********************************************************************/

/*********************************************************************/
#if KPD_PWRKEY_USE_PMIC
void kpd_pwrkey_pmic_handler(unsigned long pressed)
{
	printk(KPD_SAY "Power Key generate, pressed=%ld\n", pressed);
	if(!kpd_input_dev) {
		printk("KPD input device not ready\n");
		return;
	}
	kpd_pmic_pwrkey_hal(pressed);
}
#endif


void kpd_pmic_rstkey_handler(unsigned long pressed)
{
	printk(KPD_SAY "PMIC reset Key generate, pressed=%ld\n", pressed);
	if(!kpd_input_dev) {
		printk("KPD input device not ready\n");
		return;
	}
	kpd_pmic_rstkey_hal(pressed);
#ifdef KPD_PMIC_RSTKEY_MAP
	kpd_aee_handler(KPD_PMIC_RSTKEY_MAP, pressed);
#endif
}
/*********************************************************************/

/*********************************************************************/
static void kpd_keymap_handler(unsigned long data)
{
	int i, j;
	bool pressed;
	u16 new_state[KPD_NUM_MEMS], change, mask;
	u16 hw_keycode, linux_keycode;
	kpd_get_keymap_state(new_state);

	for (i = 0; i < KPD_NUM_MEMS; i++) {
		change = new_state[i] ^ kpd_keymap_state[i];
		if (!change)
			continue;

		for (j = 0; j < 16; j++) {
			mask = 1U << j;
			if (!(change & mask))
				continue;

			hw_keycode = (i << 4) + j;
			/* bit is 1: not pressed, 0: pressed */
			pressed = !(new_state[i] & mask);
			if (kpd_show_hw_keycode) {
				printk(KPD_SAY "(%s) HW keycode = %u\n",
				       pressed ? "pressed" : "released",
				       hw_keycode);
			}
			BUG_ON(hw_keycode >= KPD_NUM_KEYS);
			linux_keycode = kpd_keymap[hw_keycode];			
			if (unlikely(linux_keycode == 0)) {
				kpd_print("Linux keycode = 0\n");
				continue;
			}		
			kpd_aee_handler(linux_keycode, pressed);
			
			kpd_backlight_handler(pressed, linux_keycode);
			input_report_key(kpd_input_dev, linux_keycode, pressed);
			input_sync(kpd_input_dev);
			kpd_print("report Linux keycode = %u\n", linux_keycode);
		}
	}
	
	memcpy(kpd_keymap_state, new_state, sizeof(new_state));
	kpd_print("save new keymap state\n");
	enable_irq(MT_KP_IRQ_ID);
}

static irqreturn_t kpd_irq_handler(int irq, void *dev_id)
{
	/* use _nosync to avoid deadlock */
	disable_irq_nosync(MT_KP_IRQ_ID);
	tasklet_schedule(&kpd_keymap_tasklet);
	return IRQ_HANDLED;
}
/*********************************************************************/

/*****************************************************************************************/
long kpd_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	//void __user *uarg = (void __user *)arg;

	switch (cmd) {
#if KPD_AUTOTEST
	case PRESS_OK_KEY://KPD_AUTOTEST disable auto test setting to resolve CR ALPS00464496
		if(test_bit(KEY_OK, kpd_input_dev->keybit)){
			printk("[AUTOTEST] PRESS OK KEY!!\n");
			input_report_key(kpd_input_dev, KEY_OK, 1);
			input_sync(kpd_input_dev);
		}else{
			printk("[AUTOTEST] Not Support OK KEY!!\n");
		}
		break;
	case RELEASE_OK_KEY:
		if(test_bit(KEY_OK, kpd_input_dev->keybit)){
			printk("[AUTOTEST] RELEASE OK KEY!!\n");
			input_report_key(kpd_input_dev, KEY_OK, 0);
			input_sync(kpd_input_dev);
		}else{
			printk("[AUTOTEST] Not Support OK KEY!!\n");
		}
		break;
	case PRESS_MENU_KEY:
		if(test_bit(KEY_MENU, kpd_input_dev->keybit)){
			printk("[AUTOTEST] PRESS MENU KEY!!\n");
			input_report_key(kpd_input_dev, KEY_MENU, 1);
			input_sync(kpd_input_dev);
		}else{
			printk("[AUTOTEST] Not Support MENU KEY!!\n");
		}
		break;
	case RELEASE_MENU_KEY:
		if(test_bit(KEY_MENU, kpd_input_dev->keybit)){
			printk("[AUTOTEST] RELEASE MENU KEY!!\n");
			input_report_key(kpd_input_dev, KEY_MENU, 0);
			input_sync(kpd_input_dev);
		}else{
			printk("[AUTOTEST] Not Support MENU KEY!!\n");
		}

		break;
	case PRESS_UP_KEY:
		if(test_bit(KEY_UP, kpd_input_dev->keybit)){
			printk("[AUTOTEST] PRESS UP KEY!!\n");
			input_report_key(kpd_input_dev, KEY_UP, 1);
			input_sync(kpd_input_dev);
		}else{
			printk("[AUTOTEST] Not Support UP KEY!!\n");
		}
		break;
	case RELEASE_UP_KEY:
		if(test_bit(KEY_UP, kpd_input_dev->keybit)){
			printk("[AUTOTEST] RELEASE UP KEY!!\n");
			input_report_key(kpd_input_dev, KEY_UP, 0);
			input_sync(kpd_input_dev);
		}else{
			printk("[AUTOTEST] Not Support UP KEY!!\n");
		}
		break;
	case PRESS_DOWN_KEY:
		if(test_bit(KEY_DOWN, kpd_input_dev->keybit)){
			printk("[AUTOTEST] PRESS DOWN KEY!!\n");
			input_report_key(kpd_input_dev, KEY_DOWN, 1);
			input_sync(kpd_input_dev);
		}else{
			printk("[AUTOTEST] Not Support DOWN KEY!!\n");
		}
		break;
	case RELEASE_DOWN_KEY:
		if(test_bit(KEY_DOWN, kpd_input_dev->keybit)){
			printk("[AUTOTEST] RELEASE DOWN KEY!!\n");
			input_report_key(kpd_input_dev, KEY_DOWN, 0);
			input_sync(kpd_input_dev);
		}else{
			printk("[AUTOTEST] Not Support DOWN KEY!!\n");
		}
		break;
	case PRESS_LEFT_KEY:
		if(test_bit(KEY_LEFT, kpd_input_dev->keybit)){
			printk("[AUTOTEST] PRESS LEFT KEY!!\n");
			input_report_key(kpd_input_dev, KEY_LEFT, 1);
			input_sync(kpd_input_dev);
		}else{
			printk("[AUTOTEST] Not Support LEFT KEY!!\n");
		}
		break;		
	case RELEASE_LEFT_KEY:
		if(test_bit(KEY_LEFT, kpd_input_dev->keybit)){
			printk("[AUTOTEST] RELEASE LEFT KEY!!\n");
			input_report_key(kpd_input_dev, KEY_LEFT, 0);
			input_sync(kpd_input_dev);
		}else{
			printk("[AUTOTEST] Not Support LEFT KEY!!\n");
		}
		break;

	case PRESS_RIGHT_KEY:
		if(test_bit(KEY_RIGHT, kpd_input_dev->keybit)){
			printk("[AUTOTEST] PRESS RIGHT KEY!!\n");
			input_report_key(kpd_input_dev, KEY_RIGHT, 1);
			input_sync(kpd_input_dev);
		}else{
			printk("[AUTOTEST] Not Support RIGHT KEY!!\n");
		}
		break;
	case RELEASE_RIGHT_KEY:
		if(test_bit(KEY_RIGHT, kpd_input_dev->keybit)){
			printk("[AUTOTEST] RELEASE RIGHT KEY!!\n");
			input_report_key(kpd_input_dev, KEY_RIGHT, 0);
			input_sync(kpd_input_dev);
		}else{
			printk("[AUTOTEST] Not Support RIGHT KEY!!\n");
		}
		break;
	case PRESS_HOME_KEY:
		if(test_bit(KEY_HOME, kpd_input_dev->keybit)){
			printk("[AUTOTEST] PRESS HOME KEY!!\n");
			input_report_key(kpd_input_dev, KEY_HOME, 1);
			input_sync(kpd_input_dev);
		}else{
			printk("[AUTOTEST] Not Support HOME KEY!!\n");
		}
		break;
	case RELEASE_HOME_KEY:
		if(test_bit(KEY_HOME, kpd_input_dev->keybit)){
			printk("[AUTOTEST] RELEASE HOME KEY!!\n");
			input_report_key(kpd_input_dev, KEY_HOME, 0);
			input_sync(kpd_input_dev);
		}else{
			printk("[AUTOTEST] Not Support HOME KEY!!\n");
		}
		break;
	case PRESS_BACK_KEY:
		if(test_bit(KEY_BACK, kpd_input_dev->keybit)){
			printk("[AUTOTEST] PRESS BACK KEY!!\n");
			input_report_key(kpd_input_dev, KEY_BACK, 1);
			input_sync(kpd_input_dev);
		}else{
			printk("[AUTOTEST] Not Support BACK KEY!!\n");
		}
		break;
	case RELEASE_BACK_KEY:
		if(test_bit(KEY_BACK, kpd_input_dev->keybit)){
			printk("[AUTOTEST] RELEASE BACK KEY!!\n");
			input_report_key(kpd_input_dev, KEY_BACK, 0);
			input_sync(kpd_input_dev);
		}else{
			printk("[AUTOTEST] Not Support BACK KEY!!\n");
		}
		break;
	case PRESS_CALL_KEY:
		if(test_bit(KEY_CALL, kpd_input_dev->keybit)){
			printk("[AUTOTEST] PRESS CALL KEY!!\n");
			input_report_key(kpd_input_dev, KEY_CALL, 1);
			input_sync(kpd_input_dev);
		}else{
			printk("[AUTOTEST] Not Support CALL KEY!!\n");
		}
		break;
	case RELEASE_CALL_KEY:
		if(test_bit(KEY_CALL, kpd_input_dev->keybit)){
			printk("[AUTOTEST] RELEASE CALL KEY!!\n");
			input_report_key(kpd_input_dev, KEY_CALL, 0);
			input_sync(kpd_input_dev);
		}else{
			printk("[AUTOTEST] Not Support CALL KEY!!\n");
		}
		break;

	case PRESS_ENDCALL_KEY:
		if(test_bit(KEY_ENDCALL, kpd_input_dev->keybit)){
			printk("[AUTOTEST] PRESS ENDCALL KEY!!\n");
			input_report_key(kpd_input_dev, KEY_ENDCALL, 1);
			input_sync(kpd_input_dev);
		}else{
			printk("[AUTOTEST] Not Support ENDCALL KEY!!\n");
		}
		break;
	case RELEASE_ENDCALL_KEY:
		if(test_bit(KEY_ENDCALL, kpd_input_dev->keybit)){
			printk("[AUTOTEST] RELEASE ENDCALL KEY!!\n");
			input_report_key(kpd_input_dev, KEY_ENDCALL, 0);
			input_sync(kpd_input_dev);
		}else{
			printk("[AUTOTEST] Not Support ENDCALL KEY!!\n");
		}
		break;
	case PRESS_VLUP_KEY:
		if(test_bit(KEY_VOLUMEUP, kpd_input_dev->keybit)){
			printk("[AUTOTEST] PRESS VOLUMEUP KEY!!\n");
			input_report_key(kpd_input_dev, KEY_VOLUMEUP, 1);
			input_sync(kpd_input_dev);
		}else{
			printk("[AUTOTEST] Not Support VOLUMEUP KEY!!\n");
		}
		break;
	case RELEASE_VLUP_KEY:
		if(test_bit(KEY_VOLUMEUP, kpd_input_dev->keybit)){
			printk("[AUTOTEST] RELEASE VOLUMEUP KEY!!\n");
			input_report_key(kpd_input_dev, KEY_VOLUMEUP, 0);
			input_sync(kpd_input_dev);
		}else{
			printk("[AUTOTEST] Not Support VOLUMEUP KEY!!\n");
		}
		break;
	case PRESS_VLDOWN_KEY:
		if(test_bit(KEY_VOLUMEDOWN, kpd_input_dev->keybit)){
			printk("[AUTOTEST] PRESS VOLUMEDOWN KEY!!\n");
			input_report_key(kpd_input_dev, KEY_VOLUMEDOWN, 1);
			input_sync(kpd_input_dev);
		}else{
			printk("[AUTOTEST] Not Support VOLUMEDOWN KEY!!\n");
		}
		break;
	case RELEASE_VLDOWN_KEY:
		if(test_bit(KEY_VOLUMEDOWN, kpd_input_dev->keybit)){
			printk("[AUTOTEST] RELEASE VOLUMEDOWN KEY!!\n");
			input_report_key(kpd_input_dev, KEY_VOLUMEDOWN, 0);
			input_sync(kpd_input_dev);
		}else{
			printk("[AUTOTEST] Not Support VOLUMEDOWN KEY!!\n");
		}
		break;
	case PRESS_FOCUS_KEY:
		if(test_bit(KEY_FOCUS, kpd_input_dev->keybit)){
			printk("[AUTOTEST] PRESS FOCUS KEY!!\n");
			input_report_key(kpd_input_dev, KEY_FOCUS, 1);
			input_sync(kpd_input_dev);
		}else{
			printk("[AUTOTEST] Not Support FOCUS KEY!!\n");
		}
		break;
	case RELEASE_FOCUS_KEY:
		if(test_bit(KEY_FOCUS, kpd_input_dev->keybit)){
			printk("[AUTOTEST] RELEASE FOCUS KEY!!\n");
			input_report_key(kpd_input_dev, KEY_FOCUS, 0);
			input_sync(kpd_input_dev);
		}else{
			printk("[AUTOTEST] Not Support RELEASE KEY!!\n");
		}
		break;
	case PRESS_CAMERA_KEY:
		if(test_bit(KEY_CAMERA, kpd_input_dev->keybit)){
			printk("[AUTOTEST] PRESS CAMERA KEY!!\n");
			input_report_key(kpd_input_dev, KEY_CAMERA, 1);
			input_sync(kpd_input_dev);
		}else{
			printk("[AUTOTEST] Not Support CAMERA KEY!!\n");
		}
		break;
	case RELEASE_CAMERA_KEY:
		if(test_bit(KEY_CAMERA, kpd_input_dev->keybit)){
			printk("[AUTOTEST] RELEASE CAMERA KEY!!\n");
			input_report_key(kpd_input_dev, KEY_CAMERA, 0);
			input_sync(kpd_input_dev);
		}else{
			printk("[AUTOTEST] Not Support CAMERA KEY!!\n");
		}
		break;
	case PRESS_POWER_KEY:
		if(test_bit(KEY_POWER, kpd_input_dev->keybit)){
			printk("[AUTOTEST] PRESS POWER KEY!!\n");
			input_report_key(kpd_input_dev, KEY_POWER, 1);
			input_sync(kpd_input_dev);
		}else{
			printk("[AUTOTEST] Not Support POWER KEY!!\n");
		}
		break;
	case RELEASE_POWER_KEY:
		if(test_bit(KEY_POWER, kpd_input_dev->keybit)){
			printk("[AUTOTEST] RELEASE POWER KEY!!\n");
			input_report_key(kpd_input_dev, KEY_POWER, 0);
			input_sync(kpd_input_dev);
		}else{
			printk("[AUTOTEST] Not Support POWER KEY!!\n");
		}
		break;
#endif
		
	case SET_KPD_KCOL:
		kpd_auto_test_for_factorymode();//API 3 for kpd factory mode auto-test
		printk("[kpd_auto_test_for_factorymode] test performed!!\n");
		break;		
	default:
		return -EINVAL;
	}

	return 0;
}


int kpd_dev_open(struct inode *inode, struct file *file)
{
	return 0;
}

static struct file_operations kpd_dev_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= kpd_dev_ioctl,
	.open		= kpd_dev_open,
};
/*********************************************************************/
static struct miscdevice kpd_dev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= KPD_NAME,
	.fops	= &kpd_dev_fops,
};

static int kpd_open(struct input_dev *dev)
{
#if KPD_HAS_SLIDE_QWERTY
	bool evdev_flag=false;
	bool power_op=false;
	struct input_handler *handler;
	struct input_handle *handle;
	handle = rcu_dereference(dev->grab);
	if (handle)
	{
		handler = handle->handler;
		if(strcmp(handler->name, "evdev")==0) 
		{
			return -1;
		}	
	}
	else 
	{
		list_for_each_entry_rcu(handle, &dev->h_list, d_node) {
			handler = handle->handler;
			if(strcmp(handler->name, "evdev")==0) 
			{
				evdev_flag=true;
				break;
			}
		}
		if(evdev_flag==false)
		{
			return -1;	
		}	
	}
	if(!power_op) {
		printk(KPD_SAY "Qwerty slide pin interface power on fail\n");
	} else {
		kpd_print("Qwerty slide pin interface power on success\n");
	}
#if 0
	mt_eint_set_sens(KPD_SLIDE_EINT, KPD_SLIDE_SENSITIVE);
	mt_eint_set_hw_debounce(KPD_SLIDE_EINT, KPD_SLIDE_DEBOUNCE);
	mt_eint_registration(KPD_SLIDE_EINT, true, KPD_SLIDE_POLARITY,
	                         kpd_slide_eint_handler, false);
#else
	mt_set_gpio_mode(GPIO_MHALL_EINT_PIN, GPIO_MHALL_EINT_PIN_M_EINT);
    mt_set_gpio_dir(GPIO_MHALL_EINT_PIN, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_MHALL_EINT_PIN, GPIO_PULL_DISABLE); //To disable GPIO PULL.
	mt_eint_set_hw_debounce(CUST_EINT_MHALL_NUM, CUST_EINT_MHALL_DEBOUNCE_CN);
	mt_eint_registration(CUST_EINT_MHALL_NUM, CUST_EINT_MHALL_TYPE,kpd_slide_eint_handler, false);
	mt_eint_unmask(CUST_EINT_MHALL_NUM); 
#endif	                         
#if WLC_CHR_USE_EINT
	mt_set_gpio_mode(GPIO_CHR_CE_PIN, GPIO_CHR_CE_PIN_M_EINT);
    mt_set_gpio_dir(GPIO_CHR_CE_PIN, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_CHR_CE_PIN, GPIO_PULL_DISABLE); //To disable GPIO PULL.
	mt_eint_set_hw_debounce(CUST_EINT_CHR_STAT_NUM, CUST_EINT_CHR_STAT_DEBOUNCE_EN);
	mt_eint_registration(CUST_EINT_CHR_STAT_NUM, CUST_EINT_CHR_STAT_TYPE,wlc_chr_eint_handler, false);
	mt_eint_unmask(CUST_EINT_CHR_STAT_NUM); 
#endif
	if(!power_op) {
		printk(KPD_SAY "Qwerty slide pin interface power off fail\n");
	} else {
		kpd_print("Qwerty slide pin interface power off success\n");
	}
#if 0
	mt_set_gpio_mode(214, 2);
	mt_set_gpio_dir(214, 0);
	mt_set_gpio_pull_enable(214, 1);
	mt_set_gpio_pull_select(214, 0);
#endif
#endif	
	return 0;
}

#if KPD_HAS_SLIDE_QWERTY

static ssize_t show_hall_status(struct device_driver *ddri, char *buf)
{
	int size = 0;
	if(kpd_slide_state)
		{
			size = sprintf(buf, "slid\n");
		}
	else
		{
			 size = sprintf(buf, "close\n");
		}
	return size;
}
static DRIVER_ATTR(hallstatus, S_IRUGO, show_hall_status, NULL);
static struct driver_attribute *hall_attr_list[] = {
	&driver_attr_hallstatus,
};
static int hall_create_attr(struct device_driver *driver) 
{
	int idx, err = 0;
	int num = (int)(sizeof(hall_attr_list)/sizeof(hall_attr_list[0]));
	if (driver == NULL)
	{
		return -EINVAL;
	}
	for(idx = 0; idx < num; idx++)
	{
		if(err = driver_create_file(driver, hall_attr_list[idx]))
		{            
			kpd_print("driver_create_file (%s) = %d\n", hall_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}
static int hall_delete_attr(struct device_driver *driver)
{
	int idx ,err = 0;
	int num = (int)(sizeof(hall_attr_list)/sizeof(hall_attr_list[0]));
	if(driver == NULL)
	{
		return -EINVAL;
	}
	for(idx = 0; idx < num; idx++)
	{
		driver_remove_file(driver, hall_attr_list[idx]);
	}
	return err;
}
#endif
static int kpd_pdrv_probe(struct platform_device *pdev)
{
	
	int i, r;
	int err = 0;
	
	kpd_ldvt_test_init();//API 2 for kpd LFVT test enviroment settings

	/* initialize and register input device (/dev/input/eventX) */
	kpd_input_dev = input_allocate_device();
	if (!kpd_input_dev)
		return -ENOMEM;

	kpd_input_dev->name = KPD_NAME;
	kpd_input_dev->id.bustype = BUS_HOST;
	kpd_input_dev->id.vendor = 0x2454;
	kpd_input_dev->id.product = 0x6500;
	kpd_input_dev->id.version = 0x0010;
	kpd_input_dev->open = kpd_open;

	//fulfill custom settings	
	kpd_memory_setting();
	
	__set_bit(EV_KEY, kpd_input_dev->evbit);

#if (KPD_PWRKEY_USE_EINT||KPD_PWRKEY_USE_PMIC)
	__set_bit(KPD_PWRKEY_MAP, kpd_input_dev->keybit);
	kpd_keymap[8] = 0;
#endif
	for (i = 17; i < KPD_NUM_KEYS; i += 9)	/* only [8] works for Power key */
		kpd_keymap[i] = 0;

	for (i = 0; i < KPD_NUM_KEYS; i++) {
		if (kpd_keymap[i] != 0)
			__set_bit(kpd_keymap[i], kpd_input_dev->keybit);
	}

#if KPD_AUTOTEST
	for (i = 0; i < ARRAY_SIZE(kpd_auto_keymap); i++)
		__set_bit(kpd_auto_keymap[i], kpd_input_dev->keybit);
#endif

#if KPD_HAS_SLIDE_QWERTY
	__set_bit(EV_SW, kpd_input_dev->evbit);
	__set_bit(SW_LID, kpd_input_dev->swbit);
#endif

#ifdef KPD_PMIC_RSTKEY_MAP
	__set_bit(KPD_PMIC_RSTKEY_MAP, kpd_input_dev->keybit);
#endif

	kpd_input_dev->dev.parent = &pdev->dev;
	r = input_register_device(kpd_input_dev);
	if (r) {
		printk(KPD_SAY "register input device failed (%d)\n", r);
		input_free_device(kpd_input_dev);
		return r;
	}

	/* register device (/dev/mt6575-kpd) */
	kpd_dev.parent = &pdev->dev;
	r = misc_register(&kpd_dev);
	if (r) {
		printk(KPD_SAY "register device failed (%d)\n", r);
		input_unregister_device(kpd_input_dev);
		return r;
	}

	/* register IRQ and EINT */
	kpd_set_debounce(KPD_KEY_DEBOUNCE);
	r = request_irq(MT_KP_IRQ_ID, kpd_irq_handler, IRQF_TRIGGER_FALLING, KPD_NAME, NULL);
	if (r) {
		printk(KPD_SAY "register IRQ failed (%d)\n", r);
		misc_deregister(&kpd_dev);
		input_unregister_device(kpd_input_dev);
		return r;
	}

#if KPD_PWRKEY_USE_EINT
	mt_eint_register();
#endif

#ifndef KPD_EARLY_PORTING /*add for avoid early porting build err the macro is defined in custom file*/
	long_press_reboot_function_setting();///API 4 for kpd long press reboot function setting
#endif	
	hrtimer_init(&aee_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	aee_timer.function = aee_timer_func;

#if AEE_ENABLE_5_15
    hrtimer_init(&aee_timer_5s, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    aee_timer_5s.function = aee_timer_5s_func;
#endif

	if((err = kpd_create_attr(&kpd_pdrv.driver)))
	{
		kpd_print("create attr file fail\n");
		kpd_delete_attr(&kpd_pdrv.driver);
		return err;
	}
	#if KPD_HAS_SLIDE_QWERTY
		hall_create_attr(&kpd_pdrv.driver);
	#endif

	return 0;
}

/* should never be called */
static int kpd_pdrv_remove(struct platform_device *pdev)
{
	#if KPD_HAS_SLIDE_QWERTY
		hall_delete_attr(&kpd_pdrv.driver);
	#endif
	return 0;
}

#ifndef USE_EARLY_SUSPEND
static int kpd_pdrv_suspend(struct platform_device *pdev, pm_message_t state)
{
	kpd_suspend = true;
#ifdef MTK_KP_WAKESOURCE
	if(call_status == 2){
		kpd_print("kpd_early_suspend wake up source enable!! (%d)\n", kpd_suspend);
	}else{
		kpd_wakeup_src_setting(0);
		kpd_print("kpd_early_suspend wake up source disable!! (%d)\n", kpd_suspend);
	}
#endif		
	kpd_disable_backlight();
	kpd_print("suspend!! (%d)\n", kpd_suspend);
	return 0;
}

static int kpd_pdrv_resume(struct platform_device *pdev)
{
	kpd_suspend = false;	
#ifdef MTK_KP_WAKESOURCE
	if(call_status == 2){
		kpd_print("kpd_early_suspend wake up source enable!! (%d)\n", kpd_suspend);
	}else{
		kpd_print("kpd_early_suspend wake up source resume!! (%d)\n", kpd_suspend);
		kpd_wakeup_src_setting(1);
	}
#endif	
	kpd_print("resume!! (%d)\n", kpd_suspend);
	return 0;
}
#else
#define kpd_pdrv_suspend	NULL
#define kpd_pdrv_resume		NULL
#endif


#ifdef USE_EARLY_SUSPEND
static void kpd_early_suspend(struct early_suspend *h)
{
	kpd_suspend = true;
#ifdef MTK_KP_WAKESOURCE
	if(call_status == 2){
		kpd_print("kpd_early_suspend wake up source enable!! (%d)\n", kpd_suspend);
	}else{
		//kpd_wakeup_src_setting(0);
		kpd_print("kpd_early_suspend wake up source disable!! (%d)\n", kpd_suspend);
	}
#endif	
	kpd_disable_backlight();
	kpd_print("early suspend!! (%d)\n", kpd_suspend);
}

static void kpd_early_resume(struct early_suspend *h)
{
	kpd_suspend = false;
#ifdef MTK_KP_WAKESOURCE
	if(call_status == 2){
		kpd_print("kpd_early_resume wake up source resume!! (%d)\n", kpd_suspend);
	}else{
		kpd_print("kpd_early_resume wake up source enable!! (%d)\n", kpd_suspend);
		//kpd_wakeup_src_setting(1);
	}
#endif	
	kpd_print("early resume!! (%d)\n", kpd_suspend);
}

static struct early_suspend kpd_early_suspend_desc = {
	.level		= EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1,
	.suspend	= kpd_early_suspend,
	.resume		= kpd_early_resume,
};
#endif

#ifdef MTK_SMARTBOOK_SUPPORT
#ifdef CONFIG_HAS_SBSUSPEND
static struct sb_handler kpd_sb_handler_desc = {
	.level		= SB_LEVEL_DISABLE_KEYPAD,
	.plug_in	= sb_kpd_enable,
	.plug_out	= sb_kpd_disable,
};
#endif
#endif

static int __init kpd_mod_init(void)
{
	int r;

	r = platform_driver_register(&kpd_pdrv);
	if (r) {
		printk(KPD_SAY "register driver failed (%d)\n", r);
		return r;
	}

#ifdef USE_EARLY_SUSPEND
	register_early_suspend(&kpd_early_suspend_desc);
#endif

#ifdef MTK_SMARTBOOK_SUPPORT
#ifdef CONFIG_HAS_SBSUSPEND
	register_sb_handler(&kpd_sb_handler_desc);
#endif
#endif

	return 0;
}

/* should never be called */
static void __exit kpd_mod_exit(void)
{
}

module_init(kpd_mod_init);
module_exit(kpd_mod_exit);

module_param(kpd_show_hw_keycode, int, 0644);
module_param(kpd_show_register, int, 0644);

MODULE_AUTHOR("yucong.xiong <yucong.xiong@mediatek.com>");
MODULE_DESCRIPTION("MTK Keypad (KPD) Driver v0.4");
MODULE_LICENSE("GPL");
