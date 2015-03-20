#include "accdet.h"

//#include <mach/mt_boot.h>
#include <cust_eint.h>
#include <cust_gpio_usage.h>
#include <mach/mt_gpio.h>
#include <mach/eint.h>


#define SW_WORK_AROUND_ACCDET_REMOTE_BUTTON_ISSUE
#define DEBUG_THREAD 1

/*----------------------------------------------------------------------
static variable defination
----------------------------------------------------------------------*/

#define REGISTER_VALUE(x)   (x - 1)


static int button_press_debounce = 0x400;

static int debug_enable = 1;

struct headset_mode_settings *cust_headset_settings = NULL;

#define ACCDET_DEBUG(format, args...) do{ \
	if(debug_enable) \
	{\
		printk(KERN_DEBUG format,##args);\
	}\
}while(0)

static struct switch_dev accdet_data;
static struct input_dev *kpd_accdet_dev;
static struct cdev *accdet_cdev;
static struct class *accdet_class = NULL;
static struct device *accdet_nor_device = NULL;

static dev_t accdet_devno;

static int pre_status = 0;
static int pre_state_swctrl = 0;
static int accdet_status = PLUG_OUT;
static int cable_type = 0;
#if defined ACCDET_EINT && defined ACCDET_PIN_RECOGNIZATION
//add for new feature PIN recognition
static int cable_pin_recognition = 0;
static int show_icon_delay = 0;
#endif

//ALPS443614: EINT trigger during accdet irq flow running, need add sync method 
//between both
static int eint_accdet_sync_flag = 0;

static s64 long_press_time_ns = 0 ;

static int g_accdet_first = 1;
static bool IRQ_CLR_FLAG = FALSE;
static volatile int call_status =0;
static volatile int button_status = 0;


struct wake_lock accdet_suspend_lock; 
struct wake_lock accdet_irq_lock;
struct wake_lock accdet_key_lock;
struct wake_lock accdet_timer_lock;


static struct work_struct accdet_work;
static struct workqueue_struct * accdet_workqueue = NULL;

static int long_press_time;

static DEFINE_MUTEX(accdet_eint_irq_sync_mutex);

static inline void clear_accdet_interrupt(void);

#ifdef ACCDET_EINT

#ifndef ACCDET_MULTI_KEY_FEATURE
static int g_accdet_working_in_suspend =0;
#endif

static struct work_struct accdet_eint_work;
static struct workqueue_struct * accdet_eint_workqueue = NULL;

static inline void accdet_init(void);


#ifdef ACCDET_LOW_POWER

#include <linux/timer.h>
#define MICBIAS_DISABLE_TIMER   (6 *HZ)         //6 seconds
struct timer_list micbias_timer;
static void disable_micbias(unsigned long a);
/* Used to let accdet know if the pin has been fully plugged-in */
#define EINT_PIN_PLUG_IN        (1)
#define EINT_PIN_PLUG_OUT       (0)
int cur_eint_state = EINT_PIN_PLUG_OUT;
static struct work_struct accdet_disable_work;
static struct workqueue_struct * accdet_disable_workqueue = NULL;

#endif

#endif//end ACCDET_EINT

extern S32 pwrap_read( U32  adr, U32 *rdata );
extern S32 pwrap_write( U32  adr, U32  wdata );
extern struct headset_mode_settings* get_cust_headset_settings(void);
extern struct headset_key_custom* get_headset_key_custom_setting(void);
extern int PMIC_IMM_GetOneChannelValue(int dwChannel, int deCount, int trimd);
extern struct file_operations *accdet_get_fops(void);//from accdet_drv.c
extern struct platform_driver accdet_driver_func(void);//from accdet_drv.c

#ifdef DEBUG_THREAD
extern  void accdet_create_attr_func(void); //from accdet_drv.c
#endif
static U32 pmic_pwrap_read(U32 addr);
static void pmic_pwrap_write(U32 addr, unsigned int wdata);


/****************************************************************/
/***        export function                                                                        **/
/****************************************************************/

void accdet_detect(void)
{
	int ret = 0 ;
    
	ACCDET_DEBUG("[Accdet]accdet_detect\n");
    
	accdet_status = PLUG_OUT;
    ret = queue_work(accdet_workqueue, &accdet_work);	
    if(!ret)
    {
  		ACCDET_DEBUG("[Accdet]accdet_detect:accdet_work return:%d!\n", ret);  		
    }

	return;
}
EXPORT_SYMBOL(accdet_detect);

void accdet_state_reset(void)
{
    
	ACCDET_DEBUG("[Accdet]accdet_state_reset\n");
    
	accdet_status = PLUG_OUT;
    cable_type = NO_DEVICE;
        
	return;
}
EXPORT_SYMBOL(accdet_state_reset);

int accdet_get_cable_type(void)
{
	return cable_type;
}
void accdet_auxadc_switch(int enable)
{
   if (enable) {
	#ifndef ACCDET_28V_MODE
	 pmic_pwrap_write(ACCDET_RSV, ACCDET_1V9_MODE_ON);
     ACCDET_DEBUG("ACCDET enable switch in 1.9v mode \n");
	#else
	 pmic_pwrap_write(ACCDET_RSV, ACCDET_2V8_MODE_ON);
	 ACCDET_DEBUG("ACCDET enable switch in 2.8v mode \n");
	#endif
   }else {
   	#ifndef ACCDET_28V_MODE
	 pmic_pwrap_write(ACCDET_RSV, ACCDET_1V9_MODE_OFF);
     ACCDET_DEBUG("ACCDET diable switch in 1.9v mode \n");
	#else
	 pmic_pwrap_write(ACCDET_RSV, ACCDET_2V8_MODE_OFF);
	 ACCDET_DEBUG("ACCDET diable switch in 2.8v mode \n");
	#endif
   }
   	
}

/****************************************************************/
/*******static function defination                             **/
/****************************************************************/
// pmic wrap read and write func
static U32 pmic_pwrap_read(U32 addr)
{

	U32 val =0;
	pwrap_read(addr, &val);
	//ACCDET_DEBUG("[Accdet]wrap write func addr=0x%x, val=0x%x\n", addr, val);
	return val;
	
}

static void pmic_pwrap_write(unsigned int addr, unsigned int wdata)

{
    pwrap_write(addr, wdata);
	//ACCDET_DEBUG("[Accdet]wrap write func addr=0x%x, wdate=0x%x\n", addr, wdata);
}
#ifndef ACCDET_MULTI_KEY_FEATURE
//detect if remote button is short pressed or long pressed
static bool is_long_press(void)
{
	int current_status = 0;
	int index = 0;
	int count = long_press_time / 100;
	while(index++ < count)
	{ 
		current_status = ((pmic_pwrap_read(ACCDET_STATE_RG) & 0xc0)>>6);
		if(current_status != 0)
		{
			return false;
		}
			
		msleep(100);
	}
	
	return true;
}
#endif
#ifdef ACCDET_PIN_RECOGNIZATION
static void headset_standard_judge_message(void)
{
	ACCDET_DEBUG("[Accdet]Dear user: You plug in a headset which this phone doesn't support!!\n");

}
#endif

#ifdef ACCDET_PIN_SWAP

static void accdet_FSA8049_enable(void)
{
	mt_set_gpio_mode(GPIO_FSA8049_PIN, GPIO_FSA8049_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_FSA8049_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_FSA8049_PIN, GPIO_OUT_ONE);
}

static void accdet_FSA8049_disable(void)
{
	mt_set_gpio_mode(GPIO_FSA8049_PIN, GPIO_FSA8049_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_FSA8049_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_FSA8049_PIN, GPIO_OUT_ZERO);
}


#endif
static void inline headset_plug_out(void) 
{
        accdet_status = PLUG_OUT;
        cable_type = NO_DEVICE;
        //update the cable_type
        switch_set_state((struct switch_dev *)&accdet_data, cable_type);
        ACCDET_DEBUG( " [accdet] set state in cable_type = NO_DEVICE\n");
        
}

//Accdet only need this func
static void inline enable_accdet(u32 state_swctrl)
{
   // enable ACCDET unit
   ACCDET_DEBUG("accdet: enable_accdet\n");
   //enable clock
   pmic_pwrap_write(TOP_CKPDN_CLR, RG_ACCDET_CLK_CLR); 
   
   pmic_pwrap_write(ACCDET_STATE_SWCTRL, pmic_pwrap_read(ACCDET_STATE_SWCTRL)|state_swctrl);
   pmic_pwrap_write(ACCDET_CTRL, ACCDET_ENABLE);
  

}

#ifdef ACCDET_EINT
static void inline disable_accdet(void)
{
	int irq_temp = 0;
	//sync with accdet_irq_handler set clear accdet irq bit to avoid  set clear accdet irq bit after disable accdet
	
	//disable accdet irq
	pmic_pwrap_write(INT_CON_ACCDET_CLR, RG_ACCDET_IRQ_CLR);
	clear_accdet_interrupt();
	udelay(200);
	mutex_lock(&accdet_eint_irq_sync_mutex);
	while(pmic_pwrap_read(ACCDET_IRQ_STS) & IRQ_STATUS_BIT)
	{
		ACCDET_DEBUG("[Accdet]check_cable_type: Clear interrupt on-going....\n");
		msleep(5);
	}
	irq_temp = pmic_pwrap_read(ACCDET_IRQ_STS);
	irq_temp = irq_temp & (~IRQ_CLR_BIT);
	pmic_pwrap_write(ACCDET_IRQ_STS, irq_temp);
	ACCDET_DEBUG("[Accdet]disable_accdet:Clear interrupt:Done[0x%x]!\n", pmic_pwrap_read(ACCDET_IRQ_STS));	
	mutex_unlock(&accdet_eint_irq_sync_mutex);
   // disable ACCDET unit
   ACCDET_DEBUG("accdet: disable_accdet\n");
   pre_state_swctrl = pmic_pwrap_read(ACCDET_STATE_SWCTRL);
   
   pmic_pwrap_write(ACCDET_CTRL, ACCDET_DISABLE);
   pmic_pwrap_write(ACCDET_STATE_SWCTRL, 0);
//disable clock
   pmic_pwrap_write(TOP_CKPDN_SET, RG_ACCDET_CLK_SET);  
}
static void disable_micbias(unsigned long a)
{
	  int ret = 0;
      ret = queue_work(accdet_disable_workqueue, &accdet_disable_work);	
      if(!ret)
      {
  	    ACCDET_DEBUG("[Accdet]disable_micbias:accdet_work return:%d!\n", ret);  		
      }
}
static void disable_micbias_callback(struct work_struct *work)
{
	
        if(cable_type == HEADSET_NO_MIC) {
			#ifdef ACCDET_PIN_RECOGNIZATION
			   show_icon_delay = 0;
			   cable_pin_recognition = 0;
			   ACCDET_DEBUG("[Accdet] cable_pin_recognition = %d\n", cable_pin_recognition);
				pmic_pwrap_write(ACCDET_PWM_WIDTH, cust_headset_settings->pwm_width);
    			pmic_pwrap_write(ACCDET_PWM_THRESH, cust_headset_settings->pwm_thresh);
			#endif
                // setting pwm idle;
            #ifdef ACCDET_MULTI_KEY_FEATURE  
   				pmic_pwrap_write(ACCDET_STATE_SWCTRL, pmic_pwrap_read(ACCDET_STATE_SWCTRL)&~ACCDET_SWCTRL_IDLE_EN);
		    #endif 
			#ifdef ACCDET_PIN_SWAP
		    	//accdet_FSA8049_disable();  //disable GPIO209 for PIN swap 
		    	//ACCDET_DEBUG("[Accdet] FSA8049 disable!\n");
			#endif
                disable_accdet();
                ACCDET_DEBUG("[Accdet] more than 5s MICBIAS : Disabled\n");
        }
	#ifdef ACCDET_PIN_RECOGNIZATION
		else if(cable_type == HEADSET_MIC) {       
			pmic_pwrap_write(ACCDET_PWM_WIDTH, cust_headset_settings->pwm_width);
    		pmic_pwrap_write(ACCDET_PWM_THRESH, cust_headset_settings->pwm_thresh);
			ACCDET_DEBUG("[Accdet]pin recog after 5s recover micbias polling!\n");
		}
	#endif
}

static void accdet_eint_work_callback(struct work_struct *work)
{
   //KE under fastly plug in and plug out
    mt_eint_mask(CUST_EINT_ACCDET_NUM);
   
    if (cur_eint_state == EINT_PIN_PLUG_IN) {
		ACCDET_DEBUG("[Accdet]EINT func :plug-in\n");
		mutex_lock(&accdet_eint_irq_sync_mutex);
		eint_accdet_sync_flag = 1;
		mutex_unlock(&accdet_eint_irq_sync_mutex);
		#ifdef ACCDET_LOW_POWER
		wake_lock_timeout(&accdet_timer_lock, 7*HZ);
		#endif
		#ifdef ACCDET_28V_MODE
		pmic_pwrap_write(ACCDET_RSV, ACCDET_2V8_MODE_OFF);
		ACCDET_DEBUG("ACCDET use in 2.8V mode!! \n");
  		#endif
		#ifdef ACCDET_PIN_SWAP
			pmic_pwrap_write(0x0400, pmic_pwrap_read(0x0400)|(1<<14)); 
			msleep(800);
		    accdet_FSA8049_enable();  //enable GPIO209 for PIN swap 
		    ACCDET_DEBUG("[Accdet] FSA8049 enable!\n");
			msleep(250); //PIN swap need ms 
		#endif
		
			accdet_init();// do set pwm_idle on in accdet_init
		
		#ifdef ACCDET_PIN_RECOGNIZATION
		  show_icon_delay = 1;
		//micbias always on during detected PIN recognition
		  pmic_pwrap_write(ACCDET_PWM_WIDTH, cust_headset_settings->pwm_width);
    	  pmic_pwrap_write(ACCDET_PWM_THRESH, cust_headset_settings->pwm_width);
		  ACCDET_DEBUG("[Accdet]pin recog start!  micbias always on!\n");
		#endif
		//set PWM IDLE  on
		#ifdef ACCDET_MULTI_KEY_FEATURE
			pmic_pwrap_write(ACCDET_STATE_SWCTRL, (pmic_pwrap_read(ACCDET_STATE_SWCTRL)|ACCDET_SWCTRL_IDLE_EN));
		#endif  
		//enable ACCDET unit
			enable_accdet(ACCDET_SWCTRL_EN); 
    } else {
//EINT_PIN_PLUG_OUT
//Disable ACCDET
		ACCDET_DEBUG("[Accdet]EINT func :plug-out\n");
		mutex_lock(&accdet_eint_irq_sync_mutex);
		eint_accdet_sync_flag = 0;
		mutex_unlock(&accdet_eint_irq_sync_mutex);

		#ifdef ACCDET_LOW_POWER
			del_timer_sync(&micbias_timer);
		#endif
		#ifdef ACCDET_PIN_RECOGNIZATION
		  show_icon_delay = 0;
		  cable_pin_recognition = 0;
		#endif
		#ifdef ACCDET_PIN_SWAP
			pmic_pwrap_write(0x0400, pmic_pwrap_read(0x0400)&~(1<<14)); 
		    accdet_FSA8049_disable();  //disable GPIO209 for PIN swap 
		    ACCDET_DEBUG("[Accdet] FSA8049 disable!\n");
		#endif
			accdet_auxadc_switch(0);
			disable_accdet();			   
			headset_plug_out();
			#ifdef ACCDET_28V_MODE
			pmic_pwrap_write(ACCDET_RSV, ACCDET_1V9_MODE_OFF);
			ACCDET_DEBUG("ACCDET use in 1.9V mode!! \n");
  			#endif
		  
    }
    //unmask EINT
    //msleep(500);
    mt_eint_unmask(CUST_EINT_ACCDET_NUM);
    ACCDET_DEBUG("[Accdet]eint unmask  !!!!!!\n");
    
}


static void accdet_eint_func(void)
{
	int ret=0;
	if(cur_eint_state ==  EINT_PIN_PLUG_IN ) 
	{
	/*
	To trigger EINT when the headset was plugged in
	We set the polarity back as we initialed.
	*/
		if (CUST_EINT_ACCDET_TYPE == CUST_EINTF_TRIGGER_HIGH){
					mt_eint_set_polarity(CUST_EINT_ACCDET_NUM, (1));
		}else{
					mt_eint_set_polarity(CUST_EINT_ACCDET_NUM, (0));
		}

#ifdef ACCDET_SHORT_PLUGOUT_DEBOUNCE
        mt_eint_set_hw_debounce(CUST_EINT_ACCDET_NUM, CUST_EINT_ACCDET_DEBOUNCE_CN);
#endif

		/* update the eint status */
		cur_eint_state = EINT_PIN_PLUG_OUT;
//#ifdef ACCDET_LOW_POWER
//		del_timer_sync(&micbias_timer);
//#endif
	} 
	else 
	{
	/* 
	To trigger EINT when the headset was plugged out 
	We set the opposite polarity to what we initialed. 
	*/
		if (CUST_EINT_ACCDET_TYPE == CUST_EINTF_TRIGGER_HIGH){
				mt_eint_set_polarity(CUST_EINT_ACCDET_NUM, !(1));
		}else{
				mt_eint_set_polarity(CUST_EINT_ACCDET_NUM, !(0));
		}
	/* update the eint status */
#ifdef ACCDET_SHORT_PLUGOUT_DEBOUNCE
        mt_eint_set_hw_debounce(CUST_EINT_ACCDET_NUM, ACCDET_SHORT_PLUGOUT_DEBOUNCE_CN);
#endif        
		cur_eint_state = EINT_PIN_PLUG_IN;
	
#ifdef ACCDET_LOW_POWER
		//INIT the timer to disable micbias.
					
		init_timer(&micbias_timer);
		micbias_timer.expires = jiffies + MICBIAS_DISABLE_TIMER;
		micbias_timer.function = &disable_micbias;
		micbias_timer.data = ((unsigned long) 0 );
		add_timer(&micbias_timer);
					
#endif
	}

	ret = queue_work(accdet_eint_workqueue, &accdet_eint_work);	
      if(!ret)
      {
  	    //ACCDET_DEBUG("[Accdet]accdet_eint_func:accdet_work return:%d!\n", ret);  		
      }
}


static inline int accdet_setup_eint(void)
{
	
	/*configure to GPIO function, external interrupt*/
    ACCDET_DEBUG("[Accdet]accdet_setup_eint\n");
	
	mt_set_gpio_mode(GPIO_ACCDET_EINT_PIN, GPIO_ACCDET_EINT_PIN_M_EINT);
    mt_set_gpio_dir(GPIO_ACCDET_EINT_PIN, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_ACCDET_EINT_PIN, GPIO_PULL_DISABLE); //To disable GPIO PULL.

	mt_eint_set_hw_debounce(CUST_EINT_ACCDET_NUM, CUST_EINT_ACCDET_DEBOUNCE_CN);
	mt_eint_registration(CUST_EINT_ACCDET_NUM, CUST_EINT_ACCDET_TYPE, accdet_eint_func, 0);
	ACCDET_DEBUG("[Accdet]accdet set EINT finished, accdet_eint_num=%d, accdet_eint_debounce_en=%d, accdet_eint_polarity=%d\n", CUST_EINT_ACCDET_NUM, CUST_EINT_ACCDET_DEBOUNCE_EN, CUST_EINT_ACCDET_TYPE);
	
	mt_eint_unmask(CUST_EINT_ACCDET_NUM);  
	return 0;
}

#endif//endif ACCDET_EINT

#ifdef ACCDET_MULTI_KEY_FEATURE
extern int PMIC_IMM_GetOneChannelValue(int dwChannel, int deCount, int trimd);

#define KEY_SAMPLE_PERIOD        (60)            //ms
#define MULTIKEY_ADC_CHANNEL	 (8)

#define NO_KEY			 (0x0)
#define UP_KEY			 (0x01)
#define MD_KEY		  	 (0x02)
#define DW_KEY			 (0x04)

#define SHORT_PRESS		 (0x0)
#define LONG_PRESS		 (0x10)
#define SHORT_UP                 ((UP_KEY) | SHORT_PRESS)
#define SHORT_MD             	 ((MD_KEY) | SHORT_PRESS)
#define SHORT_DW                 ((DW_KEY) | SHORT_PRESS)
#define LONG_UP                  ((UP_KEY) | LONG_PRESS)
#define LONG_MD                  ((MD_KEY) | LONG_PRESS)
#define LONG_DW                  ((DW_KEY) | LONG_PRESS)

#define KEYDOWN_FLAG 1
#define KEYUP_FLAG 0
//static int g_adcMic_channel_num =0;


static DEFINE_MUTEX(accdet_multikey_mutex);



/*

        MD              UP                DW
|---------|-----------|----------|
0V<=MD< 0.09V<= UP<0.24V<=DW <0.5V

*/

#define DW_KEY_HIGH_THR	 (500) //0.50v=500000uv
#define DW_KEY_THR		 (240) //0.24v=240000uv
#define UP_KEY_THR       (90) //0.09v=90000uv
#define MD_KEY_THR		 (0)

static int key_check(int b)
{
	//ACCDET_DEBUG("adc_data: %d v\n",b);
	
	/* 0.24V ~ */
	ACCDET_DEBUG("accdet: come in key_check!!\n");
	if((b<DW_KEY_HIGH_THR)&&(b >= DW_KEY_THR)) 
	{
		ACCDET_DEBUG("adc_data: %d mv\n",b);
		return DW_KEY;
	} 
	else if ((b < DW_KEY_THR)&& (b >= UP_KEY_THR))
	{
		ACCDET_DEBUG("adc_data: %d mv\n",b);
		return UP_KEY;
	}
	else if ((b < UP_KEY_THR) && (b >= MD_KEY_THR))
	{
		ACCDET_DEBUG("adc_data: %d mv\n",b);
		return MD_KEY;
	}
	ACCDET_DEBUG("accdet: leave key_check!!\n");
	return NO_KEY;
}

static void send_key_event(int keycode,int flag)
{
    if(call_status == 0)
    {
                switch (keycode)
                {
                case DW_KEY:
					input_report_key(kpd_accdet_dev, KEY_NEXTSONG, flag);
					input_sync(kpd_accdet_dev);
					ACCDET_DEBUG("KEY_NEXTSONG %d\n",flag);
					break;
				case UP_KEY:
		   	        input_report_key(kpd_accdet_dev, KEY_PREVIOUSSONG, flag);
                    input_sync(kpd_accdet_dev);
					ACCDET_DEBUG("KEY_PREVIOUSSONG %d\n",flag);
		   	        break;
                }
     }
	else
	{
	          switch (keycode)
              {
                case DW_KEY:
					input_report_key(kpd_accdet_dev, KEY_VOLUMEDOWN, flag);
					input_sync(kpd_accdet_dev);
					ACCDET_DEBUG("KEY_VOLUMEDOWN %d\n",flag);
					break;
				case UP_KEY:
		   	        input_report_key(kpd_accdet_dev, KEY_VOLUMEUP, flag);
                    input_sync(kpd_accdet_dev);
					ACCDET_DEBUG("KEY_VOLUMEUP %d\n",flag);
		   	        break;
	          }
	}
}
static int multi_key_detection(void)
{
    int current_status = 0;
	int index = 0;
	int count = long_press_time / (KEY_SAMPLE_PERIOD + 40 ); //ADC delay
	int m_key = 0;
	int cur_key = 0;
	int cali_voltage=0;
	
	cali_voltage = PMIC_IMM_GetOneChannelValue(MULTIKEY_ADC_CHANNEL,1,1);
	ACCDET_DEBUG("[Accdet]adc cali_voltage1 = %d mv\n", cali_voltage);
	m_key = cur_key = key_check(cali_voltage);

	send_key_event(m_key, KEYDOWN_FLAG);

	while(index++ < count)
	{

		/* Check if the current state has been changed */
		current_status = ((pmic_pwrap_read(ACCDET_STATE_RG) & 0xc0)>>6);
		ACCDET_DEBUG("[Accdet]accdet current_status = %d\n", current_status);
		if(current_status != 0)
		{
		      send_key_event(m_key, KEYUP_FLAG);
			return (m_key | SHORT_PRESS);
		}

		/* Check if the voltage has been changed (press one key and another) */
		//IMM_GetOneChannelValue(g_adcMic_channel_num, adc_data, &adc_raw);
		cali_voltage = PMIC_IMM_GetOneChannelValue(MULTIKEY_ADC_CHANNEL,1,1);
		ACCDET_DEBUG("[Accdet]adc in while loop [%d]= %d mv\n", index, cali_voltage);
		cur_key = key_check(cali_voltage);
		if(m_key != cur_key)
		{
		       send_key_event(m_key, KEYUP_FLAG);
			ACCDET_DEBUG("[Accdet]accdet press one key and another happen!!\n");   
			return (m_key | SHORT_PRESS);
		}
		else
		{
			m_key = cur_key;
		}
		
		msleep(KEY_SAMPLE_PERIOD);
	}
	
	return (m_key | LONG_PRESS);
}

#endif
static void accdet_workqueue_func(void)
{
	int ret;
	ret = queue_work(accdet_workqueue, &accdet_work);	
    if(!ret)
    {
  		ACCDET_DEBUG("[Accdet]accdet_work return:%d!\n", ret);  		
    }
	
}

int accdet_irq_handler(void)
{
	int i = 0;
	if((pmic_pwrap_read(ACCDET_IRQ_STS) & IRQ_STATUS_BIT)) {
		clear_accdet_interrupt();
	}
	#ifdef ACCDET_MULTI_KEY_FEATURE
    if (accdet_status == MIC_BIAS){
		accdet_auxadc_switch(1);
    	pmic_pwrap_write(ACCDET_PWM_WIDTH, REGISTER_VALUE(cust_headset_settings->pwm_width));
	 	pmic_pwrap_write(ACCDET_PWM_THRESH, REGISTER_VALUE(cust_headset_settings->pwm_width));
    }
	#endif
    accdet_workqueue_func();  
	while(((pmic_pwrap_read(ACCDET_IRQ_STS) & IRQ_STATUS_BIT) && i<10)) {
		i++;
		udelay(200);
	}
    return 1;
}

//clear ACCDET IRQ in accdet register
static inline void clear_accdet_interrupt(void)
{

	//it is safe by using polling to adjust when to clear IRQ_CLR_BIT
	pmic_pwrap_write(ACCDET_IRQ_STS, (IRQ_CLR_BIT));
	ACCDET_DEBUG("[Accdet]clear_accdet_interrupt: ACCDET_IRQ_STS = 0x%x\n", pmic_pwrap_read(ACCDET_IRQ_STS));
}

#ifdef SW_WORK_AROUND_ACCDET_REMOTE_BUTTON_ISSUE


#define    ACC_ANSWER_CALL      1
#define    ACC_END_CALL         2
#define    ACC_MEDIA_PLAYPAUSE  3

#ifdef ACCDET_MULTI_KEY_FEATURE
#define    ACC_MEDIA_STOP       4
#define    ACC_MEDIA_NEXT       5
#define    ACC_MEDIA_PREVIOUS   6
#define    ACC_VOLUMEUP   7
#define    ACC_VOLUMEDOWN   8
#endif

static atomic_t send_event_flag = ATOMIC_INIT(0);

static DECLARE_WAIT_QUEUE_HEAD(send_event_wq);


static int accdet_key_event=0;

static int sendKeyEvent(void *unuse)
{
    while(1)
    {
        ACCDET_DEBUG( " accdet:sendKeyEvent wait\n");
        //wait for signal
        wait_event_interruptible(send_event_wq, (atomic_read(&send_event_flag) != 0));

        wake_lock_timeout(&accdet_key_lock, 2*HZ);    //set the wake lock.
        ACCDET_DEBUG( " accdet:going to send event %d\n", accdet_key_event);
		
        if(PLUG_OUT !=accdet_status)
        {
            //send key event
            if(ACC_END_CALL == accdet_key_event)
            {
                ACCDET_DEBUG("[Accdet] end call!\n");
                input_report_key(kpd_accdet_dev, KEY_ENDCALL, 1);
                input_report_key(kpd_accdet_dev, KEY_ENDCALL, 0);
                input_sync(kpd_accdet_dev);
            }
#ifdef ACCDET_MULTI_KEY_FEATURE
            if(ACC_MEDIA_PLAYPAUSE == accdet_key_event)
            {
                                ACCDET_DEBUG("[Accdet] PLAY_PAUSE !\n");
                                input_report_key(kpd_accdet_dev, KEY_PLAYPAUSE, 1);
                                input_report_key(kpd_accdet_dev, KEY_PLAYPAUSE, 0);
                                input_sync(kpd_accdet_dev);
            }
// next, previous, volumeup, volumedown send key in multi_key_detection()
            if(ACC_MEDIA_NEXT == accdet_key_event)
            {
                                ACCDET_DEBUG("[Accdet] NEXT ..\n");
                      
            }
            if(ACC_MEDIA_PREVIOUS == accdet_key_event)
            {
                                ACCDET_DEBUG("[Accdet] PREVIOUS..\n");
                           
            }
	      if(ACC_VOLUMEUP== accdet_key_event)
            {
                                ACCDET_DEBUG("[Accdet] VOLUMUP ..\n");
                           
            }
	       if(ACC_VOLUMEDOWN== accdet_key_event)
            {
                                ACCDET_DEBUG("[Accdet] VOLUMDOWN..\n");
                     
            }

#else
			if(ACC_MEDIA_PLAYPAUSE == accdet_key_event)
            {
                                ACCDET_DEBUG("[Accdet] PLAY_PAUSE !\n");
                                input_report_key(kpd_accdet_dev, KEY_PLAYPAUSE, 1);
                                input_report_key(kpd_accdet_dev, KEY_PLAYPAUSE, 0);
                                input_sync(kpd_accdet_dev);
            }
#endif
        }
        atomic_set(&send_event_flag, 0);

      //  wake_unlock(&accdet_key_lock); //unlock wake lock
    }
    return 0;
}

static ssize_t notify_sendKeyEvent(int event)
{
	 
    accdet_key_event = event;
    atomic_set(&send_event_flag, 1);
    wake_up(&send_event_wq);
    ACCDET_DEBUG( " accdet:notify_sendKeyEvent !\n");
    return 0;
}


#endif

static inline void check_cable_type(void)
{
    int current_status = 0;
	int irq_temp = 0; //for clear IRQ_bit
	int wait_clear_irq_times = 0;
#ifdef ACCDET_PIN_RECOGNIZATION
    int pin_adc_value = 0;
#define PIN_ADC_CHANNEL 5
#endif
    
    current_status = ((pmic_pwrap_read(ACCDET_STATE_RG) & 0xc0)>>6); //A=bit1; B=bit0
    ACCDET_DEBUG("[Accdet]accdet interrupt happen:[%s]current AB = %d\n", 
		accdet_status_string[accdet_status], current_status);
	    	
    button_status = 0;
    pre_status = accdet_status;

    ACCDET_DEBUG("[Accdet]check_cable_type: ACCDET_IRQ_STS = 0x%x\n", pmic_pwrap_read(ACCDET_IRQ_STS));
    IRQ_CLR_FLAG = FALSE;
	switch(accdet_status)
    {
        case PLUG_OUT:
			  #ifdef ACCDET_PIN_RECOGNIZATION
			    pmic_pwrap_write(ACCDET_DEBOUNCE1, cust_headset_settings->debounce1);
			  #endif
            if(current_status == 0)
            {
				#ifdef ACCDET_PIN_RECOGNIZATION
				//micbias always on during detected PIN recognition
				pmic_pwrap_write(ACCDET_PWM_WIDTH, cust_headset_settings->pwm_width);
    	  		pmic_pwrap_write(ACCDET_PWM_THRESH, cust_headset_settings->pwm_width);
		  		ACCDET_DEBUG("[Accdet]PIN recognition micbias always on!\n");
				 ACCDET_DEBUG("[Accdet]before adc read, pin_adc_value = %d mv!\n", pin_adc_value);
				 msleep(1000);
				 current_status = ((pmic_pwrap_read(ACCDET_STATE_RG) & 0xc0)>>6); //A=bit1; B=bit0
				 if (current_status == 0 && show_icon_delay != 0)
				 {
					accdet_auxadc_switch(1);//switch on when need to use auxadc read voltage
					pin_adc_value = PMIC_IMM_GetOneChannelValue(8,10,1);
					ACCDET_DEBUG("[Accdet]pin_adc_value = %d mv!\n", pin_adc_value);
					accdet_auxadc_switch(0);			
					if (200 > pin_adc_value && pin_adc_value> 100) //100mv   ilegal headset
					{
						headset_standard_judge_message();
						mutex_lock(&accdet_eint_irq_sync_mutex);
						if(1 == eint_accdet_sync_flag) {
						cable_type = HEADSET_NO_MIC;
						accdet_status = HOOK_SWITCH;
						cable_pin_recognition = 1;
						ACCDET_DEBUG("[Accdet] cable_pin_recognition = %d\n", cable_pin_recognition);
						}else {
							ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
						}		
						mutex_unlock(&accdet_eint_irq_sync_mutex);
					}
					else
					{
						mutex_lock(&accdet_eint_irq_sync_mutex);
						if(1 == eint_accdet_sync_flag) {
							cable_type = HEADSET_NO_MIC;
							accdet_status = HOOK_SWITCH;
						}else {
							ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
						}	
						mutex_unlock(&accdet_eint_irq_sync_mutex);
					}
				 }
				 #else
				  mutex_lock(&accdet_eint_irq_sync_mutex);
				  if(1 == eint_accdet_sync_flag) {
					cable_type = HEADSET_NO_MIC;
					accdet_status = HOOK_SWITCH;
				  }else {
					ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
				  }
				  mutex_unlock(&accdet_eint_irq_sync_mutex);
           		 #endif
            }
			else if(current_status == 1)
            {
				mutex_lock(&accdet_eint_irq_sync_mutex);
				if(1 == eint_accdet_sync_flag) {
					accdet_status = MIC_BIAS;		
	         		cable_type = HEADSET_MIC;
				}else {
					ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
				}
				mutex_unlock(&accdet_eint_irq_sync_mutex);
				//ALPS00038030:reduce the time of remote button pressed during incoming call
                //solution: reduce hook switch debounce time to 0x400
                pmic_pwrap_write(ACCDET_DEBOUNCE0, button_press_debounce);
			   //recover polling set AB 00-01
			   #ifdef ACCDET_PIN_RECOGNIZATION
				pmic_pwrap_write(ACCDET_PWM_WIDTH, REGISTER_VALUE(cust_headset_settings->pwm_width));
                pmic_pwrap_write(ACCDET_PWM_THRESH, REGISTER_VALUE(cust_headset_settings->pwm_thresh));
			   #endif
				//#ifdef ACCDET_LOW_POWER
				//wake_unlock(&accdet_timer_lock);//add for suspend disable accdet more than 5S
				//#endif
            }
            else if(current_status == 3)
            {
                ACCDET_DEBUG("[Accdet]PLUG_OUT state not change!\n");
		    	#ifdef ACCDET_MULTI_KEY_FEATURE
		    		ACCDET_DEBUG("[Accdet] do not send plug out event in plug out\n");
		    	#else
				mutex_lock(&accdet_eint_irq_sync_mutex);
				if(1 == eint_accdet_sync_flag) {
		    		accdet_status = PLUG_OUT;		
	           		cable_type = NO_DEVICE;
				}else {
					ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
				}
				mutex_unlock(&accdet_eint_irq_sync_mutex);
		    	#ifdef ACCDET_EINT
		     		disable_accdet();
		    	#endif
		        #endif
            }
            else
            {
                ACCDET_DEBUG("[Accdet]PLUG_OUT can't change to this state!\n"); 
            }
            break;

	    case MIC_BIAS:
	    //ALPS00038030:reduce the time of remote button pressed during incoming call
            //solution: resume hook switch debounce time
            pmic_pwrap_write(ACCDET_DEBOUNCE0, cust_headset_settings->debounce0);
			
            if(current_status == 0)
            {
            
			mutex_lock(&accdet_eint_irq_sync_mutex);
			if(1 == eint_accdet_sync_flag) {
				while((pmic_pwrap_read(ACCDET_IRQ_STS) & IRQ_STATUS_BIT) && (wait_clear_irq_times<3))
	        	{
		          ACCDET_DEBUG("[Accdet]check_cable_type: MIC BIAS clear IRQ on-going1....\n");	
				  wait_clear_irq_times++;
				  msleep(5);
	        	}
				irq_temp = pmic_pwrap_read(ACCDET_IRQ_STS);
				irq_temp = irq_temp & (~IRQ_CLR_BIT);
				pmic_pwrap_write(ACCDET_IRQ_STS, irq_temp);
            	IRQ_CLR_FLAG = TRUE;
		    	accdet_status = HOOK_SWITCH;
			}else {
					ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			}
			mutex_unlock(&accdet_eint_irq_sync_mutex);
		    button_status = 1;
			if(button_status)
		    {	
			#ifdef ACCDET_MULTI_KEY_FEATURE		
				int multi_key = NO_KEY;		
			  	//mdelay(10);
			   	//if plug out don't send key
			    mutex_lock(&accdet_eint_irq_sync_mutex);
				if(1 == eint_accdet_sync_flag) {   
					multi_key = multi_key_detection();
				}else {
					ACCDET_DEBUG("[Accdet] multi_key_detection: Headset has plugged out\n");
				}
				mutex_unlock(&accdet_eint_irq_sync_mutex);
				accdet_auxadc_switch(0);
			//recover  pwm frequency and duty
                pmic_pwrap_write(ACCDET_PWM_WIDTH, REGISTER_VALUE(cust_headset_settings->pwm_width));
                pmic_pwrap_write(ACCDET_PWM_THRESH, REGISTER_VALUE(cust_headset_settings->pwm_thresh));
			switch (multi_key) 
			{
			case SHORT_UP:
				ACCDET_DEBUG("[Accdet] Short press up (0x%x)\n", multi_key);
                           if(call_status == 0)
                           {
                                 notify_sendKeyEvent(ACC_MEDIA_PREVIOUS);
                           }
					       else
						   {
							     notify_sendKeyEvent(ACC_VOLUMEUP);
						    }
				break;
			case SHORT_MD:
				ACCDET_DEBUG("[Accdet] Short press middle (0x%x)\n", multi_key);
                                 notify_sendKeyEvent(ACC_MEDIA_PLAYPAUSE);
				break;
			case SHORT_DW:
				ACCDET_DEBUG("[Accdet] Short press down (0x%x)\n", multi_key);
                           if(call_status == 0)
                            {
                                 notify_sendKeyEvent(ACC_MEDIA_NEXT);
                            }
							else
							{
							     notify_sendKeyEvent(ACC_VOLUMEDOWN);
							}
				break;
			case LONG_UP:
				ACCDET_DEBUG("[Accdet] Long press up (0x%x)\n", multi_key);
                                 send_key_event(UP_KEY, KEYUP_FLAG);
                            
				break;
			case LONG_MD:
				ACCDET_DEBUG("[Accdet] Long press middle (0x%x)\n", multi_key);
                                 notify_sendKeyEvent(ACC_END_CALL);
				break;
			case LONG_DW:
				ACCDET_DEBUG("[Accdet] Long press down (0x%x)\n", multi_key);			
                                 send_key_event(DW_KEY, KEYUP_FLAG);
							
				break;
			default:
				ACCDET_DEBUG("[Accdet] unkown key (0x%x)\n", multi_key);
				break;
			}
			#else
                if(call_status != 0) 
	            {
	                   if(is_long_press())
	                   {
                             ACCDET_DEBUG("[Accdet]send long press remote button event %d \n",ACC_END_CALL);
                             notify_sendKeyEvent(ACC_END_CALL);
                       } else {
                             ACCDET_DEBUG("[Accdet]send short press remote button event %d\n",ACC_ANSWER_CALL);
                             notify_sendKeyEvent(ACC_MEDIA_PLAYPAUSE);
                       }
                 }
			#endif////end  ifdef ACCDET_MULTI_KEY_FEATURE else
	     }
}
          else if(current_status == 1)
          {
          	 mutex_lock(&accdet_eint_irq_sync_mutex);
			 if(1 == eint_accdet_sync_flag) {
                accdet_status = MIC_BIAS;		
	            cable_type = HEADSET_MIC;
                ACCDET_DEBUG("[Accdet]MIC_BIAS state not change!\n");
			 }else {
					ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			 }
			 mutex_unlock(&accdet_eint_irq_sync_mutex);
          }
          else if(current_status == 3)
          {
           #ifdef ACCDET_MULTI_KEY_FEATURE
		   		ACCDET_DEBUG("[Accdet]do not send plug ou in micbiast\n");
		        mutex_lock(&accdet_eint_irq_sync_mutex);
		        if(1 == eint_accdet_sync_flag) {
		   			accdet_status = PLUG_OUT;
			 	}else {
					ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			 	}
			 	mutex_unlock(&accdet_eint_irq_sync_mutex);
		   #else
		   		mutex_lock(&accdet_eint_irq_sync_mutex);
		        if(1 == eint_accdet_sync_flag) {
		   			accdet_status = PLUG_OUT;		
	          		cable_type = NO_DEVICE;
				}else {
					ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			 	}
			 	mutex_unlock(&accdet_eint_irq_sync_mutex);
		   #ifdef ACCDET_EINT
		   		disable_accdet();
		   #endif
		   #endif
          }
          else
           {
               ACCDET_DEBUG("[Accdet]MIC_BIAS can't change to this state!\n"); 
           }
          break;

	case HOOK_SWITCH:
            if(current_status == 0)
            {
				mutex_lock(&accdet_eint_irq_sync_mutex);
		        if(1 == eint_accdet_sync_flag) {
					//for avoid 01->00 framework of Headset will report press key info for Audio
					//cable_type = HEADSET_NO_MIC;
		        	//accdet_status = HOOK_SWITCH;
                	ACCDET_DEBUG("[Accdet]HOOK_SWITCH state not change!\n");
				}else {
					ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			 	}
			 	mutex_unlock(&accdet_eint_irq_sync_mutex);
	    	}
            else if(current_status == 1)
            {
				mutex_lock(&accdet_eint_irq_sync_mutex);
		        if(1 == eint_accdet_sync_flag) {
					accdet_status = MIC_BIAS;		
	        		cable_type = HEADSET_MIC;
				}else {
					ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			 	}
			 	mutex_unlock(&accdet_eint_irq_sync_mutex);
				#ifdef ACCDET_PIN_RECOGNIZATION
				cable_pin_recognition = 0;
				ACCDET_DEBUG("[Accdet] cable_pin_recognition = %d\n", cable_pin_recognition);
				pmic_pwrap_write(ACCDET_PWM_WIDTH, REGISTER_VALUE(cust_headset_settings->pwm_width));
                pmic_pwrap_write(ACCDET_PWM_THRESH, REGISTER_VALUE(cust_headset_settings->pwm_thresh));
				#endif
		//ALPS00038030:reduce the time of remote button pressed during incoming call
         //solution: reduce hook switch debounce time to 0x400
                pmic_pwrap_write(ACCDET_DEBOUNCE0, button_press_debounce);
				//#ifdef ACCDET_LOW_POWER
				//wake_unlock(&accdet_timer_lock);//add for suspend disable accdet more than 5S
				//#endif
            }
            else if(current_status == 3)
            {
            	
             #ifdef ACCDET_PIN_RECOGNIZATION
			 	cable_pin_recognition = 0;
			 	ACCDET_DEBUG("[Accdet] cable_pin_recognition = %d\n", cable_pin_recognition);
			    mutex_lock(&accdet_eint_irq_sync_mutex);
		        if(1 == eint_accdet_sync_flag) {
			 	accdet_status = PLUG_OUT;
				}else {
					ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			 	}
			 	mutex_unlock(&accdet_eint_irq_sync_mutex);
			 #endif
             #ifdef ACCDET_MULTI_KEY_FEATURE
			 	ACCDET_DEBUG("[Accdet] do not send plug out event in hook switch\n"); 
			 	mutex_lock(&accdet_eint_irq_sync_mutex);
		        if(1 == eint_accdet_sync_flag) {
			 		accdet_status = PLUG_OUT;
				}else {
					ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			 	}
			 	mutex_unlock(&accdet_eint_irq_sync_mutex);
			 #else
			 	mutex_lock(&accdet_eint_irq_sync_mutex);
		        if(1 == eint_accdet_sync_flag) {
		      		accdet_status = PLUG_OUT;		
		      		cable_type = NO_DEVICE;
				}else {
					ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			 	}
			 	mutex_unlock(&accdet_eint_irq_sync_mutex);
			 #ifdef ACCDET_EINT
		       	disable_accdet();
		     #endif
		     #endif
            }
            else
            {
                ACCDET_DEBUG("[Accdet]HOOK_SWITCH can't change to this state!\n"); 
            }
            break;
			
	case STAND_BY:
			if(current_status == 3)
			{
                 #ifdef ACCDET_MULTI_KEY_FEATURE
						ACCDET_DEBUG("[Accdet]accdet do not send plug out event in stand by!\n");
		    	 #else
				 		mutex_lock(&accdet_eint_irq_sync_mutex);
		       			 if(1 == eint_accdet_sync_flag) {
							accdet_status = PLUG_OUT;		
							cable_type = NO_DEVICE;
						 }else {
							ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			 			 }
			 			mutex_unlock(&accdet_eint_irq_sync_mutex);
				#ifdef ACCDET_EINT
						disable_accdet();
			    #endif
			    #endif
			 }
			 else
			{
					ACCDET_DEBUG("[Accdet]STAND_BY can't change to this state!\n"); 
			}
			break;
			
			default:
				ACCDET_DEBUG("[Accdet]check_cable_type: accdet current status error!\n");
			break;
						
}
			
		if(!IRQ_CLR_FLAG)
		{
			mutex_lock(&accdet_eint_irq_sync_mutex);
			if(1 == eint_accdet_sync_flag) {
				while((pmic_pwrap_read(ACCDET_IRQ_STS) & IRQ_STATUS_BIT) && (wait_clear_irq_times<3))
				{
				  ACCDET_DEBUG("[Accdet]check_cable_type: Clear interrupt on-going2....\n");
				  wait_clear_irq_times++;
				  msleep(5);
				}
			}
			irq_temp = pmic_pwrap_read(ACCDET_IRQ_STS);
			irq_temp = irq_temp & (~IRQ_CLR_BIT);
			pmic_pwrap_write(ACCDET_IRQ_STS, irq_temp);
			mutex_unlock(&accdet_eint_irq_sync_mutex);
			IRQ_CLR_FLAG = TRUE;
			ACCDET_DEBUG("[Accdet]check_cable_type:Clear interrupt:Done[0x%x]!\n", pmic_pwrap_read(ACCDET_IRQ_STS));	
			
		}
		else
		{
			IRQ_CLR_FLAG = FALSE;
		}

		ACCDET_DEBUG("[Accdet]cable type:[%s], status switch:[%s]->[%s]\n",
        accdet_report_string[cable_type], accdet_status_string[pre_status], 
        accdet_status_string[accdet_status]);
} 
static void accdet_work_callback(struct work_struct *work)
{

    wake_lock(&accdet_irq_lock);
    check_cable_type();
	
	mutex_lock(&accdet_eint_irq_sync_mutex);
    if(1 == eint_accdet_sync_flag) {
		switch_set_state((struct switch_dev *)&accdet_data, cable_type);
    }else {
		ACCDET_DEBUG("[Accdet] Headset has plugged out don't set accdet state\n");
	}
	mutex_unlock(&accdet_eint_irq_sync_mutex);
	ACCDET_DEBUG( " [accdet] set state in cable_type  status\n");

    wake_unlock(&accdet_irq_lock);
}

//ACCDET hardware initial
static inline void accdet_init(void)
{ 
	ACCDET_DEBUG("[Accdet]accdet hardware init\n");
    
    pmic_pwrap_write(TOP_CKPDN_CLR, RG_ACCDET_CLK_CLR);  
	ACCDET_DEBUG("[Accdet]accdet TOP_CKPDN=0x%x!\n", pmic_pwrap_read(TOP_CKPDN));	
    //reset the accdet unit

	ACCDET_DEBUG("ACCDET reset : reset start!! \n\r");
	pmic_pwrap_write(TOP_RST_ACCDET_SET, ACCDET_RESET_SET);

	ACCDET_DEBUG("ACCDET reset function test: reset finished!! \n\r");
	pmic_pwrap_write(TOP_RST_ACCDET_CLR, ACCDET_RESET_CLR);
		
	//init  pwm frequency and duty
    pmic_pwrap_write(ACCDET_PWM_WIDTH, REGISTER_VALUE(cust_headset_settings->pwm_width));
    pmic_pwrap_write(ACCDET_PWM_THRESH, REGISTER_VALUE(cust_headset_settings->pwm_thresh));

	
	pwrap_write(ACCDET_STATE_SWCTRL, 0x07);
   			

	pmic_pwrap_write(ACCDET_EN_DELAY_NUM,
		(cust_headset_settings->fall_delay << 15 | cust_headset_settings->rise_delay));

    // init the debounce time
   #ifdef ACCDET_PIN_RECOGNIZATION
    pmic_pwrap_write(ACCDET_DEBOUNCE0, cust_headset_settings->debounce0);
    pmic_pwrap_write(ACCDET_DEBOUNCE1, 0xFFFF);
    pmic_pwrap_write(ACCDET_DEBOUNCE3, cust_headset_settings->debounce3);	
   #else
    pmic_pwrap_write(ACCDET_DEBOUNCE0, cust_headset_settings->debounce0);
    pmic_pwrap_write(ACCDET_DEBOUNCE1, cust_headset_settings->debounce1);
    pmic_pwrap_write(ACCDET_DEBOUNCE3, cust_headset_settings->debounce3);	
   #endif
    pmic_pwrap_write(ACCDET_IRQ_STS, pmic_pwrap_read(ACCDET_IRQ_STS)&(~IRQ_CLR_BIT));
	pmic_pwrap_write(INT_CON_ACCDET_SET, RG_ACCDET_IRQ_SET);
    #ifdef ACCDET_EINT
    // disable ACCDET unit
	pre_state_swctrl = pmic_pwrap_read(ACCDET_STATE_SWCTRL);
    pmic_pwrap_write(ACCDET_CTRL, ACCDET_DISABLE);
    pmic_pwrap_write(ACCDET_STATE_SWCTRL, 0x0);
	pmic_pwrap_write(TOP_CKPDN_SET, RG_ACCDET_CLK_SET);
	#else
	
    // enable ACCDET unit
   // pmic_pwrap_write(ACCDET_STATE_SWCTRL, ACCDET_SWCTRL_EN);
    pmic_pwrap_write(ACCDET_CTRL, ACCDET_ENABLE); 
	#endif

#ifdef GPIO_FSA8049_PIN
    //mt_set_gpio_out(GPIO_FSA8049_PIN, GPIO_OUT_ONE);
#endif
#ifdef FSA8049_V_POWER
    hwPowerOn(FSA8049_V_POWER, VOL_2800, "ACCDET");
#endif

}
/*-----------------------------------sysfs-----------------------------------------*/
#if DEBUG_THREAD
static int dump_register(void)
{
   int i=0;
   for (i=0x077A; i<= 0x079A; i+=2)
   {
     ACCDET_DEBUG(" ACCDET_BASE + %x=%x\n",i,pmic_pwrap_read(ACCDET_BASE + i));
   }

   ACCDET_DEBUG(" TOP_RST_ACCDET =%x\n",pmic_pwrap_read(TOP_RST_ACCDET));// reset register in 6320
   ACCDET_DEBUG(" INT_CON_ACCDET =%x\n",pmic_pwrap_read(INT_CON_ACCDET));//INT register in 6320
   ACCDET_DEBUG(" TOP_CKPDN =%x\n",pmic_pwrap_read(TOP_CKPDN));// clock register in 6320
  #ifdef ACCDET_PIN_SWAP
   //ACCDET_DEBUG(" 0x00004000 =%x\n",pmic_pwrap_read(0x00004000));//VRF28 power for PIN swap feature
  #endif
  return 0;
}

static ssize_t accdet_store_call_state(struct device_driver *ddri, const char *buf, size_t count)
{
	if (sscanf(buf, "%u", &call_status) != 1) {
			ACCDET_DEBUG("accdet: Invalid values\n");
			return -EINVAL;
	}

	switch(call_status)
    {
        case CALL_IDLE :
			ACCDET_DEBUG("[Accdet]accdet call: Idle state!\n");
     		break;
            
		case CALL_RINGING :
			
			ACCDET_DEBUG("[Accdet]accdet call: ringing state!\n");
			break;

		case CALL_ACTIVE :
			ACCDET_DEBUG("[Accdet]accdet call: active or hold state!\n");	
			ACCDET_DEBUG("[Accdet]accdet_ioctl : Button_Status=%d (state:%d)\n", button_status, accdet_data.state);	
			//return button_status;
			break;
            
		default:
   		    ACCDET_DEBUG("[Accdet]accdet call : Invalid values\n");
            break;
     }
	return count;
}

//#ifdef ACCDET_PIN_RECOGNIZATION

static ssize_t show_pin_recognition_state(struct device_driver *ddri, char *buf)
{
   #ifdef ACCDET_PIN_RECOGNIZATION
	ACCDET_DEBUG("ACCDET show_pin_recognition_state = %d\n", cable_pin_recognition);
	return sprintf(buf, "%u\n", cable_pin_recognition);
   #else
    return sprintf(buf, "%u\n", 0);
   #endif
}

static DRIVER_ATTR(accdet_pin_recognition,      0664, show_pin_recognition_state,  NULL);
static DRIVER_ATTR(accdet_call_state,      0664, NULL,         accdet_store_call_state);

static int g_start_debug_thread =0;
static struct task_struct *thread = NULL;
static int g_dump_register=0;
static int dbug_thread(void *unused) 
{
   while(g_start_debug_thread)
   	{
      if(g_dump_register)
	  {
	    dump_register();
		//dump_pmic_register(); 
      }

	  msleep(500);

   	}
   return 0;
}
//static ssize_t store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
static ssize_t store_accdet_start_debug_thread(struct device_driver *ddri, const char *buf, size_t count)
{
	
	unsigned int start_flag;
	int error;

	if (sscanf(buf, "%u", &start_flag) != 1) {
		ACCDET_DEBUG("accdet: Invalid values\n");
		return -EINVAL;
	}

	ACCDET_DEBUG("[Accdet] start flag =%d \n",start_flag);

	g_start_debug_thread = start_flag;

    if(1 == start_flag)
    {
	   thread = kthread_run(dbug_thread, 0, "ACCDET");
       if (IS_ERR(thread)) 
	   { 
          error = PTR_ERR(thread);
          ACCDET_DEBUG( " failed to create kernel thread: %d\n", error);
       }
    }

	return count;
}
static ssize_t store_accdet_set_headset_mode(struct device_driver *ddri, const char *buf, size_t count)
{

    unsigned int value;
	//int error;

	if (sscanf(buf, "%u", &value) != 1) {
		ACCDET_DEBUG("accdet: Invalid values\n");
		return -EINVAL;
	}

	ACCDET_DEBUG("[Accdet]store_accdet_set_headset_mode value =%d \n",value);

	return count;
}

static ssize_t store_accdet_dump_register(struct device_driver *ddri, const char *buf, size_t count)
{
    unsigned int value;
//	int error;

	if (sscanf(buf, "%u", &value) != 1) 
	{
		ACCDET_DEBUG("accdet: Invalid values\n");
		return -EINVAL;
	}

	g_dump_register = value;

	ACCDET_DEBUG("[Accdet]store_accdet_dump_register value =%d \n",value);

	return count;
}

/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(dump_register,      S_IWUSR | S_IRUGO, NULL,         store_accdet_dump_register);

static DRIVER_ATTR(set_headset_mode,      S_IWUSR | S_IRUGO, NULL,         store_accdet_set_headset_mode);

static DRIVER_ATTR(start_debug,      S_IWUSR | S_IRUGO, NULL,         store_accdet_start_debug_thread);

/*----------------------------------------------------------------------------*/
static struct driver_attribute *accdet_attr_list[] = {
	&driver_attr_start_debug,        
	&driver_attr_set_headset_mode,
	&driver_attr_dump_register,
	&driver_attr_accdet_call_state,
	//#ifdef ACCDET_PIN_RECOGNIZATION
	&driver_attr_accdet_pin_recognition,
	//#endif
};

static int accdet_create_attr(struct device_driver *driver) 
{
	int idx, err = 0;
	int num = (int)(sizeof(accdet_attr_list)/sizeof(accdet_attr_list[0]));
	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if((err = driver_create_file(driver, accdet_attr_list[idx])))
		{            
			ACCDET_DEBUG("driver_create_file (%s) = %d\n", accdet_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}

#endif

int mt_accdet_probe(void)	
{
	int ret = 0;
#ifdef SW_WORK_AROUND_ACCDET_REMOTE_BUTTON_ISSUE
     struct task_struct *keyEvent_thread = NULL;
	 int error=0;
#endif
#if DEBUG_THREAD
		 struct platform_driver accdet_driver_hal = accdet_driver_func();
#endif

	struct headset_key_custom* press_key_time = get_headset_key_custom_setting();

	ACCDET_DEBUG("[Accdet]accdet_probe begin!\n");

	
	//------------------------------------------------------------------
	// 							below register accdet as switch class
	//------------------------------------------------------------------	
	accdet_data.name = "h2w";
	accdet_data.index = 0;
	accdet_data.state = NO_DEVICE;

	cust_headset_settings = get_cust_headset_settings();
	
	ret = switch_dev_register(&accdet_data);
	if(ret)
	{
		ACCDET_DEBUG("[Accdet]switch_dev_register returned:%d!\n", ret);
		return 1;
	}
		
	//------------------------------------------------------------------
	// 							Create normal device for auido use
	//------------------------------------------------------------------
	ret = alloc_chrdev_region(&accdet_devno, 0, 1, ACCDET_DEVNAME);
	if (ret)
	{
		ACCDET_DEBUG("[Accdet]alloc_chrdev_region: Get Major number error!\n");			
	} 
		
	accdet_cdev = cdev_alloc();
    accdet_cdev->owner = THIS_MODULE;
    accdet_cdev->ops = accdet_get_fops();
    ret = cdev_add(accdet_cdev, accdet_devno, 1);
	if(ret)
	{
		ACCDET_DEBUG("[Accdet]accdet error: cdev_add\n");
	}
	
	accdet_class = class_create(THIS_MODULE, ACCDET_DEVNAME);

    // if we want auto creat device node, we must call this
	accdet_nor_device = device_create(accdet_class, NULL, accdet_devno, NULL, ACCDET_DEVNAME);  
	
	//------------------------------------------------------------------
	// 							Create input device 
	//------------------------------------------------------------------
	kpd_accdet_dev = input_allocate_device();
	if (!kpd_accdet_dev) 
	{
		ACCDET_DEBUG("[Accdet]kpd_accdet_dev : fail!\n");
		return -ENOMEM;
	}

	//define multi-key keycode
	__set_bit(EV_KEY, kpd_accdet_dev->evbit);
	__set_bit(KEY_CALL, kpd_accdet_dev->keybit);
	__set_bit(KEY_ENDCALL, kpd_accdet_dev->keybit);
    __set_bit(KEY_NEXTSONG, kpd_accdet_dev->keybit);
    __set_bit(KEY_PREVIOUSSONG, kpd_accdet_dev->keybit);
    __set_bit(KEY_PLAYPAUSE, kpd_accdet_dev->keybit);
    __set_bit(KEY_STOPCD, kpd_accdet_dev->keybit);
	__set_bit(KEY_VOLUMEDOWN, kpd_accdet_dev->keybit);
    __set_bit(KEY_VOLUMEUP, kpd_accdet_dev->keybit);
	
	kpd_accdet_dev->id.bustype = BUS_HOST;
	kpd_accdet_dev->name = "ACCDET";
	if(input_register_device(kpd_accdet_dev))
	{
		ACCDET_DEBUG("[Accdet]kpd_accdet_dev register : fail!\n");
	}else
	{
		ACCDET_DEBUG("[Accdet]kpd_accdet_dev register : success!!\n");
	} 
	//------------------------------------------------------------------
	// 							Create workqueue 
	//------------------------------------------------------------------	
	accdet_workqueue = create_singlethread_workqueue("accdet");
	INIT_WORK(&accdet_work, accdet_work_callback);

		
    //------------------------------------------------------------------
	//							wake lock
	//------------------------------------------------------------------
	wake_lock_init(&accdet_suspend_lock, WAKE_LOCK_SUSPEND, "accdet wakelock");
    wake_lock_init(&accdet_irq_lock, WAKE_LOCK_SUSPEND, "accdet irq wakelock");
    wake_lock_init(&accdet_key_lock, WAKE_LOCK_SUSPEND, "accdet key wakelock");
	wake_lock_init(&accdet_timer_lock, WAKE_LOCK_SUSPEND, "accdet timer wakelock");
#ifdef SW_WORK_AROUND_ACCDET_REMOTE_BUTTON_ISSUE
     init_waitqueue_head(&send_event_wq);
     //start send key event thread
	 keyEvent_thread = kthread_run(sendKeyEvent, 0, "keyEvent_send");
     if (IS_ERR(keyEvent_thread)) 
	 { 
        error = PTR_ERR(keyEvent_thread);
        ACCDET_DEBUG( " failed to create kernel thread: %d\n", error);
     }
#endif
	
#if DEBUG_THREAD
 	 if((ret = accdet_create_attr(&accdet_driver_hal.driver))!=0)
	 {
		ACCDET_DEBUG("create attribute err = %d\n", ret);
	
	 }
#endif
 
	 long_press_time = press_key_time->headset_long_press_time;

	ACCDET_DEBUG("[Accdet]accdet_probe : ACCDET_INIT\n");  
	if (g_accdet_first == 1) 
	{	
		long_press_time_ns = (s64)long_press_time * NSEC_PER_MSEC;
		
		eint_accdet_sync_flag = 1;
		
		//Accdet Hardware Init
		pmic_pwrap_write(ACCDET_RSV, ACCDET_1V9_MODE_OFF);
		ACCDET_DEBUG("ACCDET use in 1.9V mode!! \n");
		accdet_init();   
		queue_work(accdet_workqueue, &accdet_work); //schedule a work for the first detection					
		#ifdef ACCDET_EINT

          accdet_eint_workqueue = create_singlethread_workqueue("accdet_eint");
	      INIT_WORK(&accdet_eint_work, accdet_eint_work_callback);
	      accdet_setup_eint();
		  accdet_disable_workqueue = create_singlethread_workqueue("accdet_disable");
	      INIT_WORK(&accdet_disable_work, disable_micbias_callback);
	 	
       #endif
		g_accdet_first = 0;
	}
	
        ACCDET_DEBUG("[Accdet]accdet_probe done!\n");
//#ifdef ACCDET_PIN_SWAP
	//pmic_pwrap_write(0x0400, 0x1000); 
	//ACCDET_DEBUG("[Accdet]accdet enable VRF28 power!\n");
//#endif
		
	    return 0;
}

void mt_accdet_remove(void)	
{
	ACCDET_DEBUG("[Accdet]accdet_remove begin!\n");
	
	//cancel_delayed_work(&accdet_work);
	#ifdef ACCDET_EINT
	destroy_workqueue(accdet_eint_workqueue);
	#endif
	destroy_workqueue(accdet_workqueue);
	switch_dev_unregister(&accdet_data);
	device_del(accdet_nor_device);
	class_destroy(accdet_class);
	cdev_del(accdet_cdev);
	unregister_chrdev_region(accdet_devno,1);	
	input_unregister_device(kpd_accdet_dev);
	ACCDET_DEBUG("[Accdet]accdet_remove Done!\n");

}

void mt_accdet_suspend(void)  // only one suspend mode
{
	
//#ifdef ACCDET_PIN_SWAP
//		pmic_pwrap_write(0x0400, 0x0); 
//		accdet_FSA8049_disable(); 
//#endif

#ifdef ACCDET_MULTI_KEY_FEATURE
	ACCDET_DEBUG("[Accdet] in suspend1: ACCDET_IRQ_STS = 0x%x\n", pmic_pwrap_read(ACCDET_IRQ_STS));
#else

#if 0
#ifdef ACCDET_EINT
    if(accdet_get_enable_RG()&& call_status == 0)
    {
	   //record accdet status
	   //ACCDET_DEBUG("[Accdet]accdet_working_in_suspend\n");
	   printk(KERN_DEBUG "[Accdet]accdet_working_in_suspend\n");
	   g_accdet_working_in_suspend = 1;
       pre_state_swctrl = accdet_get_swctrl();
	   // disable ACCDET unit
       accdet_disable_hal();
	   //disable_clock
	   accdet_disable_clk();
    }
#else
    // disable ACCDET unit
    if(call_status == 0)
    {
       pre_state_swctrl = accdet_get_swctrl();
       accdet_disable_hal();
       //disable_clock
       accdet_disable_clk(); 
    }
#endif	
#endif 
	printk(KERN_DEBUG "[Accdet]accdet_suspend: ACCDET_CTRL=[0x%x], STATE=[0x%x]->[0x%x]\n", pmic_pwrap_read(ACCDET_CTRL), pre_state_swctrl, pmic_pwrap_read(ACCDET_STATE_SWCTRL));
#endif
}

void mt_accdet_resume(void) // wake up
{
//#ifdef ACCDET_PIN_SWAP
//		pmic_pwrap_write(0x0400, 0x1000); 
//		accdet_FSA8049_enable(); 
//#endif

#ifdef ACCDET_MULTI_KEY_FEATURE
	ACCDET_DEBUG("[Accdet] in resume1: ACCDET_IRQ_STS = 0x%x\n", pmic_pwrap_read(ACCDET_IRQ_STS));
#else
#if 0
#ifdef ACCDET_EINT

	if(1==g_accdet_working_in_suspend &&  0== call_status)
	{
	
	   //enable_clock
	   accdet_enable_hal(pre_state_swctrl); 
       //clear g_accdet_working_in_suspend
	   g_accdet_working_in_suspend =0;
	   ACCDET_DEBUG("[Accdet]accdet_resume : recovery accdet register\n");
	   
	}
#else
	if(call_status == 0)
	{
       accdet_enable_hal(pre_state_swctrl);
	}
#endif
#endif
	printk(KERN_DEBUG "[Accdet]accdet_resume: ACCDET_CTRL=[0x%x], STATE_SWCTRL=[0x%x]\n", pmic_pwrap_read(ACCDET_CTRL), pmic_pwrap_read(ACCDET_STATE_SWCTRL));

#endif

}
/**********************************************************************
//add for IPO-H need update headset state when resume

***********************************************************************/
#ifdef ACCDET_PIN_RECOGNIZATION	
struct timer_list accdet_disable_ipoh_timer;
static void mt_accdet_pm_disable(unsigned long a)
{
	if (cable_type == NO_DEVICE && eint_accdet_sync_flag ==0) {
		//disable accdet
		pre_state_swctrl = pmic_pwrap_read(ACCDET_STATE_SWCTRL);	
    	pmic_pwrap_write(ACCDET_CTRL, ACCDET_DISABLE);
   		pmic_pwrap_write(ACCDET_STATE_SWCTRL, 0);
		//disable clock
    	pmic_pwrap_write(TOP_CKPDN_SET, RG_ACCDET_CLK_SET); 
		printk("[Accdet]daccdet_pm_disable: disable!\n");
	}
	else
	{
		printk("[Accdet]daccdet_pm_disable: enable!\n");
	}
}
#endif
void mt_accdet_pm_restore_noirq(void)
{
	int current_status_restore = 0;
    printk("[Accdet]accdet_pm_restore_noirq start!\n");
	//enable accdet
	pmic_pwrap_write(ACCDET_STATE_SWCTRL, (pmic_pwrap_read(ACCDET_STATE_SWCTRL)|ACCDET_SWCTRL_IDLE_EN));
	// enable ACCDET unit
    ACCDET_DEBUG("accdet: enable_accdet\n");
    //enable clock
    pmic_pwrap_write(TOP_CKPDN_CLR, RG_ACCDET_CLK_CLR); 
    enable_accdet(ACCDET_SWCTRL_EN);
	eint_accdet_sync_flag = 1;
	current_status_restore = ((pmic_pwrap_read(ACCDET_STATE_RG) & 0xc0)>>6); //AB
		
	switch (current_status_restore) {
		case 0:     //AB=0
			cable_type = HEADSET_NO_MIC;
			accdet_status = HOOK_SWITCH;
			break;
		case 1:     //AB=1
			cable_type = HEADSET_MIC;
			accdet_status = MIC_BIAS;
			break;
		case 3:     //AB=3
			cable_type = NO_DEVICE;
			accdet_status = PLUG_OUT;
			break;
		default:
			printk("[Accdet]accdet_pm_restore_noirq: accdet current status error!\n");
			break;
	}
	switch_set_state((struct switch_dev *)&accdet_data, cable_type);
	if (cable_type == NO_DEVICE) {
	#ifdef ACCDET_PIN_RECOGNIZATION	
		init_timer(&accdet_disable_ipoh_timer);
		accdet_disable_ipoh_timer.expires = jiffies + 3*HZ;
		accdet_disable_ipoh_timer.function = &mt_accdet_pm_disable;
		accdet_disable_ipoh_timer.data = ((unsigned long) 0 );
		add_timer(&accdet_disable_ipoh_timer);
		printk("[Accdet]enable! pm timer\n");	

    #else
		//eint_accdet_sync_flag = 0;
		//disable accdet
		pre_state_swctrl = pmic_pwrap_read(ACCDET_STATE_SWCTRL);	
    	pmic_pwrap_write(ACCDET_CTRL, ACCDET_DISABLE);
   		pmic_pwrap_write(ACCDET_STATE_SWCTRL, 0);
		//disable clock
    	pmic_pwrap_write(TOP_CKPDN_SET, RG_ACCDET_CLK_SET);
	#endif
	}

}
////////////////////////////////////IPO_H end/////////////////////////////////////////////
long mt_accdet_unlocked_ioctl(unsigned int cmd, unsigned long arg)
{
//	bool ret = true;
		
    switch(cmd)
    {
        case ACCDET_INIT :
     		break;
            
		case SET_CALL_STATE :
			call_status = (int)arg;
			ACCDET_DEBUG("[Accdet]accdet_ioctl : CALL_STATE=%d \n", call_status);
			break;

		case GET_BUTTON_STATUS :
			ACCDET_DEBUG("[Accdet]accdet_ioctl : Button_Status=%d (state:%d)\n", button_status, accdet_data.state);	
			return button_status;
            
		default:
   		    ACCDET_DEBUG("[Accdet]accdet_ioctl : default\n");
            break;
  }
  return 0;
}

