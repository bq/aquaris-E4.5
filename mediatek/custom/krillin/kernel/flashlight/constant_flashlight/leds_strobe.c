#include <linux/kernel.h> //constant xx
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/time.h>
#include "kd_flashlight.h"
#include <asm/io.h>
#include <asm/uaccess.h>
#include "kd_camera_hw.h"
#include <cust_gpio_usage.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/xlog.h>
#include <linux/version.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
#include <linux/mutex.h>
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
#include <linux/semaphore.h>
#else
#include <asm/semaphore.h>
#endif
#endif



/******************************************************************************
 * Debug configuration
******************************************************************************/
// availible parameter
// ANDROID_LOG_ASSERT
// ANDROID_LOG_ERROR
// ANDROID_LOG_WARNING
// ANDROID_LOG_INFO
// ANDROID_LOG_DEBUG
// ANDROID_LOG_VERBOSE
#define TAG_NAME "leds_strobe.c"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    xlog_printk(ANDROID_LOG_DEBUG  , TAG_NAME, KERN_INFO  "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_WARN(fmt, arg...)        xlog_printk(ANDROID_LOG_WARNING, TAG_NAME, KERN_WARNING  "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_NOTICE(fmt, arg...)      xlog_printk(ANDROID_LOG_DEBUG  , TAG_NAME, KERN_NOTICE  "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_INFO(fmt, arg...)        xlog_printk(ANDROID_LOG_INFO   , TAG_NAME, KERN_INFO  "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_TRC_FUNC(f)              xlog_printk(ANDROID_LOG_DEBUG  , TAG_NAME,  "<%s>\n", __FUNCTION__);
#define PK_TRC_VERBOSE(fmt, arg...) xlog_printk(ANDROID_LOG_VERBOSE, TAG_NAME,  fmt, ##arg)
#define PK_ERROR(fmt, arg...)       xlog_printk(ANDROID_LOG_ERROR  , TAG_NAME, KERN_ERR "%s: " fmt, __FUNCTION__ ,##arg)


#define DEBUG_LEDS_STROBE
#ifdef  DEBUG_LEDS_STROBE
	#define PK_DBG PK_DBG_FUNC
	#define PK_VER PK_TRC_VERBOSE
	#define PK_ERR PK_ERROR
#else
	#define PK_DBG(a,...)
	#define PK_VER(a,...)
	#define PK_ERR(a,...)
#endif

//control by BB gpio
#define GPIO_CAMERA_FLASH_MODE GPIO_CAMERA_FLASH_MODE_PIN //GPIO12
#define GPIO_CAMERA_FLASH_MODE_M_GPIO  GPIO_MODE_00
        //CAMERA-FLASH-T/F
            //H:flash mode
            //L:torch mode
#define GPIO_CAMERA_FLASH_EN GPIO_CAMERA_FLASH_EN_PIN//GPIO13
#define GPIO_CAMERA_FLASH_EN_M_GPIO  GPIO_MODE_00
#define GPIO_FLASH_LEVEL GPIO_CAMERA_FLASH_LEVEL_PIN//114
#define GPIO_FLASH_LEVEL_M_GPIO  GPIO_CAMERA_FLASH_LEVEL_PIN_M_GPIO

#if 0
#define GPIO_CAMERA_FLASH_FRONT_EN GPIO_CAMERA_FLASH_EXT2_PIN//119 for front flashliht
#define GPIO_CAMERA_FLASH_FRONT_EN_M_GPIO GPIO_CAMERA_FLASH_EXT2_PIN_M_GPIO

#define GPIO_CAMERA_FLASH_BACK_EN GPIO_CAMERA_FLASH_EXT1_PIN//118 for back flashlight
#define GPIO_CAMERA_FLASH_BACK_EN_M_GPIO GPIO_CAMERA_FLASH_EXT1_PIN_M_GPIO
#else
#undef GPIO_CAMERA_FLASH_FRONT_EN
#undef GPIO_CAMERA_FLASH_BACK_EN
#endif
        //CAMERA-FLASH-EN
#define TORCH_LIGHT_LEVEL 3 //just 1st level modified by tyrael
#define PRE_LIGHT_LEVEL 6

//add by tyrael for back/front flashlight
#include "kd_camera_feature.h"
extern CAMERA_DUAL_CAMERA_SENSOR_ENUM get_current_invokeSensorIdx(void);
//add end

/******************************************************************************
 * local variables
******************************************************************************/

static DEFINE_SPINLOCK(g_strobeSMPLock); /* cotta-- SMP proection */


static u32 strobe_Res = 0;
static u32 strobe_Timeus = 0;
static BOOL g_strobe_On = 0;

static int g_duty=-1;
static int g_timeOutTimeMs=0;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
static DEFINE_MUTEX(g_strobeSem);
#else
static DECLARE_MUTEX(g_strobeSem);
#endif


#define STROBE_DEVICE_ID 0x60


static struct work_struct workTimeOut;

/*****************************************************************************
Functions
*****************************************************************************/
#define GPIO_ENF GPIO_CAMERA_FLASH_EN_PIN
#define GPIO_ENT GPIO_CAMERA_FLASH_MODE_PIN


    /*CAMERA-FLASH-EN */


extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
static void work_timeOutFunc(struct work_struct *data);














int FL_Enable(void)
{
#if 1
/*Enable*/
    if(mt_set_gpio_mode(GPIO_CAMERA_FLASH_EN,GPIO_CAMERA_FLASH_MODE_M_GPIO)){PK_DBG("[constant_flashlight] set gpio mode failed!! \n");}
    if(mt_set_gpio_dir(GPIO_CAMERA_FLASH_EN,GPIO_DIR_OUT)){PK_DBG("[constant_flashlight] set gpio dir failed!! \n");}
    if(mt_set_gpio_out(GPIO_CAMERA_FLASH_EN,GPIO_OUT_ONE)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}
	PK_DBG("[constant_flashlight] set gpio %d %s \n",GPIO_CAMERA_FLASH_EN,GPIO_OUT_ONE?"HIGH":"LOW");

     #ifdef GPIO_CAMERA_FLASH_BACK_EN
     if(mt_set_gpio_mode(GPIO_CAMERA_FLASH_BACK_EN, GPIO_CAMERA_FLASH_BACK_EN_M_GPIO)){PK_DBG("[constant_flashlight] set gpio mode failed!! \n");}
     if(mt_set_gpio_dir(GPIO_CAMERA_FLASH_BACK_EN,GPIO_DIR_OUT)){PK_DBG("[constant_flashlight] set gpio dir failed!! \n");}
     if(mt_set_gpio_out(GPIO_CAMERA_FLASH_BACK_EN,GPIO_OUT_ONE)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}	
     #endif

#else
	if(g_duty==0)
	{
		mt_set_gpio_out(GPIO_ENT,GPIO_OUT_ONE);
		mt_set_gpio_out(GPIO_ENF,GPIO_OUT_ZERO);
		PK_DBG(" FL_Enable line=%d\n",__LINE__);
	}
	else
	{
		mt_set_gpio_out(GPIO_ENT,GPIO_OUT_ZERO);
		mt_set_gpio_out(GPIO_ENF,GPIO_OUT_ONE);
		PK_DBG(" FL_Enable line=%d\n",__LINE__);
	}
#endif
    return 0;
}

int FL_Disable(void)
{
#if 1
	/*Disable*/	   
    if(mt_set_gpio_mode(GPIO_CAMERA_FLASH_EN,GPIO_CAMERA_FLASH_MODE_M_GPIO)){PK_DBG("[constant_flashlight] set gpio mode failed!! \n");}
    if(mt_set_gpio_dir(GPIO_CAMERA_FLASH_EN,GPIO_DIR_OUT)){PK_DBG("[constant_flashlight] set gpio dir failed!! \n");}
    if(mt_set_gpio_out(GPIO_CAMERA_FLASH_EN,GPIO_OUT_ZERO)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}
	PK_DBG("[constant_flashlight] set gpio %d %s \n",GPIO_CAMERA_FLASH_EN,GPIO_OUT_ZERO?"HIGH":"LOW");
	 //disable back flashlight en pin
	 #ifdef GPIO_CAMERA_FLASH_BACK_EN
     if(mt_set_gpio_mode(GPIO_CAMERA_FLASH_BACK_EN, GPIO_CAMERA_FLASH_BACK_EN_M_GPIO)){PK_DBG("[constant_flashlight] set gpio mode failed!! \n");}
     if(mt_set_gpio_dir(GPIO_CAMERA_FLASH_BACK_EN,GPIO_DIR_OUT)){PK_DBG("[constant_flashlight] set gpio dir failed!! \n");}
     if(mt_set_gpio_out(GPIO_CAMERA_FLASH_BACK_EN,GPIO_OUT_ZERO)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}
	 #endif
#else
	mt_set_gpio_out(GPIO_ENT,GPIO_OUT_ZERO);
	mt_set_gpio_out(GPIO_ENF,GPIO_OUT_ZERO);
	PK_DBG(" FL_Disable line=%d\n",__LINE__);
#endif
    return 0;
}

int FL_dim_duty(kal_uint32 duty)
{
    int i;
   switch (duty){
   	case 0://in torch mode
   	            PK_DBG("set torch mode\n");

			if(mt_set_gpio_mode(GPIO_FLASH_LEVEL, GPIO_FLASH_LEVEL_M_GPIO)){PK_DBG("[constant_flashlight] set gpio mode failed!! \n");}
                   if(mt_set_gpio_dir(GPIO_FLASH_LEVEL,GPIO_DIR_OUT)){PK_DBG("[constant_flashlight] set gpio dir failed!! \n");}
                   if(mt_set_gpio_out(GPIO_FLASH_LEVEL,GPIO_OUT_ZERO)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}

		    /*set torch mode*/
		    if(mt_set_gpio_mode(GPIO_CAMERA_FLASH_MODE,GPIO_CAMERA_FLASH_MODE_M_GPIO)){PK_DBG("[constant_flashlight] set gpio mode failed!! \n");}
		    if(mt_set_gpio_dir(GPIO_CAMERA_FLASH_MODE,GPIO_DIR_OUT)){PK_DBG("[constant_flashlight] set gpio dir failed!! \n");}
		    if(mt_set_gpio_out(GPIO_CAMERA_FLASH_MODE,GPIO_OUT_ZERO)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}
		
		break;
	case 1://in pre flash & af flash mode
	             PK_DBG("set pre flash & af flash mdoe\n");
                    for(i=0;i<PRE_LIGHT_LEVEL;i++)
                    {
                    if(mt_set_gpio_mode(GPIO_CAMERA_FLASH_EN,GPIO_CAMERA_FLASH_EN_M_GPIO)){PK_DBG("[constant_flashlight] set gpio mode failed!! \n");}
                    if(mt_set_gpio_dir(GPIO_CAMERA_FLASH_EN,GPIO_DIR_OUT)){PK_DBG("[constant_flashlight] set gpio dir failed!! \n");}
                    if(mt_set_gpio_out(GPIO_CAMERA_FLASH_EN,GPIO_OUT_ZERO)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}
                    if(mt_set_gpio_out(GPIO_CAMERA_FLASH_EN,GPIO_OUT_ONE)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}
                    }
                    PK_DBG("[constant_flashlight] set gpio %d %s \n",GPIO_CAMERA_FLASH_EN,GPIO_OUT_ONE?"HIGH":"LOW");
	
				 
	             if(mt_set_gpio_mode(GPIO_FLASH_LEVEL, GPIO_FLASH_LEVEL_M_GPIO)){PK_DBG("[constant_flashlight] set gpio mode failed!! \n");}
                   if(mt_set_gpio_dir(GPIO_FLASH_LEVEL,GPIO_DIR_OUT)){PK_DBG("[constant_flashlight] set gpio dir failed!! \n");}
                   if(mt_set_gpio_out(GPIO_FLASH_LEVEL,GPIO_OUT_ZERO)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}

					 
                   if(mt_set_gpio_mode(GPIO_CAMERA_FLASH_MODE,GPIO_CAMERA_FLASH_MODE_M_GPIO)){PK_DBG("[constant_flashlight] set gpio mode failed!! \n");}
                   if(mt_set_gpio_dir(GPIO_CAMERA_FLASH_MODE,GPIO_DIR_OUT)){PK_DBG("[constant_flashlight] set gpio dir failed!! \n");}
                   if(mt_set_gpio_out(GPIO_CAMERA_FLASH_MODE,GPIO_OUT_ZERO)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}
		break;
	case 2://in main flash mode
	            PK_DBG("set main flash mode\n");
	            if(mt_set_gpio_mode(GPIO_FLASH_LEVEL, GPIO_FLASH_LEVEL_M_GPIO)){PK_DBG("[constant_flashlight] set gpio mode failed!! \n");}
                   if(mt_set_gpio_dir(GPIO_FLASH_LEVEL,GPIO_DIR_OUT)){PK_DBG("[constant_flashlight] set gpio dir failed!! \n");}
                   if(mt_set_gpio_out(GPIO_FLASH_LEVEL,GPIO_OUT_ZERO)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}

					 
                   if(mt_set_gpio_mode(GPIO_CAMERA_FLASH_MODE,GPIO_CAMERA_FLASH_MODE_M_GPIO)){PK_DBG("[constant_flashlight] set gpio mode failed!! \n");}
                   if(mt_set_gpio_dir(GPIO_CAMERA_FLASH_MODE,GPIO_DIR_OUT)){PK_DBG("[constant_flashlight] set gpio dir failed!! \n");}
                   if(mt_set_gpio_out(GPIO_CAMERA_FLASH_MODE,GPIO_OUT_ONE)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}
		break;
	default:
		PK_DBG("error duty=%d value !!!%d\n",duty);
		break;
   	}
    return 0;
}


int FL_step(kal_uint32 step)
{
//	upmu_set_flash_sel(step);
    return 0;
}

int FL_Init(void)
{
#if 1

	PK_DBG("start\n");
    /*set torch mode*/
    if(mt_set_gpio_mode(GPIO_CAMERA_FLASH_MODE,GPIO_CAMERA_FLASH_MODE_M_GPIO)){PK_DBG("[constant_flashlight] set gpio mode failed!! \n");}
    if(mt_set_gpio_dir(GPIO_CAMERA_FLASH_MODE,GPIO_DIR_OUT)){PK_DBG("[constant_flashlight] set gpio dir failed!! \n");}
    if(mt_set_gpio_out(GPIO_CAMERA_FLASH_MODE,GPIO_OUT_ZERO)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}
    /*Init. to disable*/
    if(mt_set_gpio_mode(GPIO_CAMERA_FLASH_EN,GPIO_CAMERA_FLASH_EN_M_GPIO)){PK_DBG("[constant_flashlight] set gpio mode failed!! \n");}
    if(mt_set_gpio_dir(GPIO_CAMERA_FLASH_EN,GPIO_DIR_OUT)){PK_DBG("[constant_flashlight] set gpio dir failed!! \n");}
    if(mt_set_gpio_out(GPIO_CAMERA_FLASH_EN,GPIO_OUT_ZERO)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}
  	//disable front flashlight en pin
  	#ifdef GPIO_CAMERA_FLASH_FRONT_EN
     if(mt_set_gpio_mode(GPIO_CAMERA_FLASH_FRONT_EN, GPIO_CAMERA_FLASH_FRONT_EN_M_GPIO)){PK_DBG("[constant_flashlight] set gpio mode failed!! \n");}
     if(mt_set_gpio_dir(GPIO_CAMERA_FLASH_FRONT_EN,GPIO_DIR_OUT)){PK_DBG("[constant_flashlight] set gpio dir failed!! \n");}
     if(mt_set_gpio_out(GPIO_CAMERA_FLASH_FRONT_EN,GPIO_OUT_ZERO)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}	
	 #endif
	INIT_WORK(&workTimeOut, work_timeOutFunc);
PK_DBG("done\n");

#else

	if(mt_set_gpio_mode(GPIO_ENF,GPIO_MODE_00)){PK_DBG("[constant_flashlight] set gpio mode failed!! \n");}
    if(mt_set_gpio_dir(GPIO_ENF,GPIO_DIR_OUT)){PK_DBG("[constant_flashlight] set gpio dir failed!! \n");}
    if(mt_set_gpio_out(GPIO_ENF,GPIO_OUT_ZERO)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}
    /*Init. to disable*/
    if(mt_set_gpio_mode(GPIO_ENT,GPIO_MODE_00)){PK_DBG("[constant_flashlight] set gpio mode failed!! \n");}
    if(mt_set_gpio_dir(GPIO_ENT,GPIO_DIR_OUT)){PK_DBG("[constant_flashlight] set gpio dir failed!! \n");}
    if(mt_set_gpio_out(GPIO_ENT,GPIO_OUT_ZERO)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}

    INIT_WORK(&workTimeOut, work_timeOutFunc);
    PK_DBG(" FL_Init line=%d\n",__LINE__);
#endif
    return 0;
}


int FL_Uninit(void)
{
	FL_Disable();
    return 0;
}

/*****************************************************************************
User interface
*****************************************************************************/

static void work_timeOutFunc(struct work_struct *data)
{
    FL_Disable();
    PK_DBG("ledTimeOut_callback\n");
    //printk(KERN_ALERT "work handler function./n");
}



enum hrtimer_restart ledTimeOutCallback(struct hrtimer *timer)
{
    schedule_work(&workTimeOut);
    return HRTIMER_NORESTART;
}
static struct hrtimer g_timeOutTimer;
void timerInit(void)
{
	g_timeOutTimeMs=1000; //1s
	hrtimer_init( &g_timeOutTimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
	g_timeOutTimer.function=ledTimeOutCallback;

}


static int set_flashlight_state(unsigned long state)
{

    if(state==1){
    /*Enable*/
     PK_DBG("in flash light test mode so open so enable back and front flash light at same time \n");
	#ifdef GPIO_CAMERA_FLASH_BACK_EN
	if(mt_set_gpio_mode(GPIO_CAMERA_FLASH_BACK_EN, GPIO_CAMERA_FLASH_BACK_EN_M_GPIO)){PK_DBG("[constant_flashlight] set gpio mode failed!! \n");}
	if(mt_set_gpio_dir(GPIO_CAMERA_FLASH_BACK_EN,GPIO_DIR_OUT)){PK_DBG("[constant_flashlight] set gpio dir failed!! \n");}
	if(mt_set_gpio_out(GPIO_CAMERA_FLASH_BACK_EN,GPIO_OUT_ONE)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}
	#endif

	#ifdef GPIO_CAMERA_FLASH_FRONT_EN
	if(mt_set_gpio_mode(GPIO_CAMERA_FLASH_FRONT_EN, GPIO_CAMERA_FLASH_FRONT_EN_M_GPIO)){PK_DBG("[constant_flashlight] set gpio mode failed!! \n");}
	if(mt_set_gpio_dir(GPIO_CAMERA_FLASH_FRONT_EN,GPIO_DIR_OUT)){PK_DBG("[constant_flashlight] set gpio dir failed!! \n");}
	if(mt_set_gpio_out(GPIO_CAMERA_FLASH_FRONT_EN,GPIO_OUT_ONE)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}
	#endif
#if 0
    switch(state){
	case 1:
		if(mt_set_gpio_mode(GPIO_FLASH_LEVEL, GPIO_FLASH_LEVEL_M_GPIO)){PK_DBG("[constant_flashlight] set gpio mode failed!! \n");}
             if(mt_set_gpio_dir(GPIO_FLASH_LEVEL,GPIO_DIR_OUT)){PK_DBG("[constant_flashlight] set gpio dir failed!! \n");}
             if(mt_set_gpio_out(GPIO_FLASH_LEVEL,GPIO_OUT_ONE)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}

		    /*set torch mode*/
		if(mt_set_gpio_mode(GPIO_CAMERA_FLASH_MODE,GPIO_CAMERA_FLASH_MODE_M_GPIO)){PK_DBG("[constant_flashlight] set gpio mode failed!! \n");}
		if(mt_set_gpio_dir(GPIO_CAMERA_FLASH_MODE,GPIO_DIR_OUT)){PK_DBG("[constant_flashlight] set gpio dir failed!! \n");}
		if(mt_set_gpio_out(GPIO_CAMERA_FLASH_MODE,GPIO_OUT_ZERO)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}
		break;
	case 2:
			if(mt_set_gpio_mode(GPIO_FLASH_LEVEL, GPIO_FLASH_LEVEL_M_GPIO)){PK_DBG("[constant_flashlight] set gpio mode failed!! \n");}
                   if(mt_set_gpio_dir(GPIO_FLASH_LEVEL,GPIO_DIR_OUT)){PK_DBG("[constant_flashlight] set gpio dir failed!! \n");}
                   if(mt_set_gpio_out(GPIO_FLASH_LEVEL,GPIO_OUT_ZERO)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}
		 
                   if(mt_set_gpio_mode(GPIO_CAMERA_FLASH_MODE,GPIO_CAMERA_FLASH_MODE_M_GPIO)){PK_DBG("[constant_flashlight] set gpio mode failed!! \n");}
                   if(mt_set_gpio_dir(GPIO_CAMERA_FLASH_MODE,GPIO_DIR_OUT)){PK_DBG("[constant_flashlight] set gpio dir failed!! \n");}
                   if(mt_set_gpio_out(GPIO_CAMERA_FLASH_MODE,GPIO_OUT_ZERO)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}
		break;
	case 3:
		 if(mt_set_gpio_mode(GPIO_FLASH_LEVEL, GPIO_FLASH_LEVEL_M_GPIO)){PK_DBG("[constant_flashlight] set gpio mode failed!! \n");}
              if(mt_set_gpio_dir(GPIO_FLASH_LEVEL,GPIO_DIR_OUT)){PK_DBG("[constant_flashlight] set gpio dir failed!! \n");}
              if(mt_set_gpio_out(GPIO_FLASH_LEVEL,GPIO_OUT_ZERO)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}
			 
              if(mt_set_gpio_mode(GPIO_CAMERA_FLASH_MODE,GPIO_CAMERA_FLASH_MODE_M_GPIO)){PK_DBG("[constant_flashlight] set gpio mode failed!! \n");}
              if(mt_set_gpio_dir(GPIO_CAMERA_FLASH_MODE,GPIO_DIR_OUT)){PK_DBG("[constant_flashlight] set gpio dir failed!! \n");}
              if(mt_set_gpio_out(GPIO_CAMERA_FLASH_MODE,GPIO_OUT_ONE)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}
		break;
	default :
    		PK_DBG(" No such command \n");	
	}
#endif
    		return 0;
	}
	else{
		PK_DBG("There must be something wrong !!!\n");
	}
}

static int constant_flashlight_ioctl(MUINT32 cmd, MUINT32 arg)
{
	int i4RetValue = 0;
	int ior_shift;
	int iow_shift;
	int iowr_shift;
	ior_shift = cmd - (_IOR(FLASHLIGHT_MAGIC,0, int));
	iow_shift = cmd - (_IOW(FLASHLIGHT_MAGIC,0, int));
	iowr_shift = cmd - (_IOWR(FLASHLIGHT_MAGIC,0, int));
	PK_DBG("constant_flashlight_ioctl() line=%d ior_shift=%d, iow_shift=%d iowr_shift=%d arg=%d\n",__LINE__, ior_shift, iow_shift, iowr_shift, arg);
    switch(cmd)
    {

		case FLASHLIGHTIOC_T_STATE:
			PK_DBG("FLASHLIGHTIOC_T_STATE: %d\n",arg);
			set_flashlight_state(arg);
		break;
		
		case FLASH_IOC_SET_TIME_OUT_TIME_MS:
			PK_DBG("FLASH_IOC_SET_TIME_OUT_TIME_MS: %d\n",arg);
			g_timeOutTimeMs=arg;
		break;


    	case FLASH_IOC_SET_DUTY :
    		PK_DBG("FLASHLIGHT_DUTY: %d\n",arg);
		g_duty=arg;
    		FL_dim_duty(arg);
    		break;


    	case FLASH_IOC_SET_STEP:
    		PK_DBG("FLASH_IOC_SET_STEP: %d\n",arg);
    		FL_step(arg);
    		break;

    	case FLASH_IOC_SET_ONOFF :
    		PK_DBG("FLASHLIGHT_ONOFF: %d\n",arg);
    		if(arg==1)
    		{
				if(g_timeOutTimeMs!=0)
	            {
	            	ktime_t ktime;
					ktime = ktime_set( 0, g_timeOutTimeMs*1000000 );
					hrtimer_start( &g_timeOutTimer, ktime, HRTIMER_MODE_REL );
	            }
    			FL_Enable();
			g_strobe_On=1;
    		}
    		else
    		{
    			FL_Disable();
				hrtimer_cancel( &g_timeOutTimer );
				g_strobe_On=0;
    		}
    		break;
		default :
    		PK_DBG(" No such command \n");
    		i4RetValue = -EPERM;
    		break;
    }
    return i4RetValue;
}




static int constant_flashlight_open(void *pArg)
{
    int i4RetValue = 0;
    PK_DBG("constant_flashlight_open line=%d\n", __LINE__);

	if (0 == strobe_Res)
	{
	    FL_Init();
		timerInit();
	}
	PK_DBG("constant_flashlight_open line=%d\n", __LINE__);
	spin_lock_irq(&g_strobeSMPLock);


    if(strobe_Res)
    {
        PK_ERR(" busy!\n");
        i4RetValue = -EBUSY;
    }
    else
    {
        strobe_Res += 1;
    }


    spin_unlock_irq(&g_strobeSMPLock);
    PK_DBG("constant_flashlight_open line=%d\n", __LINE__);

    return i4RetValue;

}


static int constant_flashlight_release(void *pArg)
{
    PK_DBG(" constant_flashlight_release\n");

    if (strobe_Res)
    {
        spin_lock_irq(&g_strobeSMPLock);

        strobe_Res = 0;
        strobe_Timeus = 0;

        /* LED On Status */
        g_strobe_On = FALSE;

        spin_unlock_irq(&g_strobeSMPLock);

    	FL_Uninit();
    }

    PK_DBG(" Done\n");

    return 0;

}


FLASHLIGHT_FUNCTION_STRUCT	constantFlashlightFunc=
{
	constant_flashlight_open,
	constant_flashlight_release,
	constant_flashlight_ioctl
};


MUINT32 constantFlashlightInit(PFLASHLIGHT_FUNCTION_STRUCT *pfFunc)
{
    if (pfFunc != NULL)
    {
        *pfFunc = &constantFlashlightFunc;
    }
    return 0;
}



/* LED flash control for high current capture mode*/
ssize_t strobe_VDIrq(void)
{

    return 0;
}

EXPORT_SYMBOL(strobe_VDIrq);


