#include <platform/ddp_reg.h>
#include <platform/ddp_path.h>
#include <debug.h>
#include <string.h>
#include <cust_leds.h>

#ifndef CLK_CFG_1
#define CLK_CFG_1 0x10000050
#endif


//#define CKT_SGM3727_SUPPORT
//luosen 增加用GPIO 控制SGM3727 背光芯片
#if defined(CKT_SGM3727_SUPPORT)

#ifdef BUILD_LK
	#include <platform/mt_gpio.h>
#elif defined(BUILD_UBOOT)
	#include <asm/arch/mt_gpio.h>
#else
	#include <mach/mt_gpio.h>
#endif

static int pre_state = 0; // 0 ------ off;    1 ------ on
static int pre_level = 0;
static int cur_level = 0;
#endif

#define POLLING_TIME_OUT 10000

#define PWM_DEFAULT_DIV_VALUE 0x0

#if !defined(MTK_AAL_SUPPORT) 
static int gBLSMutexID = 3;
#endif

static int gPWMDiv = PWM_DEFAULT_DIV_VALUE;

static unsigned int brightness_mapping(unsigned int level)
{
    unsigned int mapped_level;

    // PWM duty input =  PWM_DUTY_IN / 1024
    mapped_level = level * 1023 / 255;

    if (mapped_level > 0x3FF)
        mapped_level = 0x3FF;

	return mapped_level;
}


int disp_poll_for_reg(unsigned int addr, unsigned int value, unsigned int mask, unsigned int timeout)
{
    unsigned int cnt = 0;
    
    while ((DISP_REG_GET(addr) & mask) != value)
    {
        cnt++;
        if (cnt > timeout)
        {
            return -1;
        }
    }

    return 0;
}

static int disp_bls_get_mutex()
{
#if !defined(MTK_AAL_SUPPORT)    
    if (gBLSMutexID < 0)
        return -1;

    DISP_REG_SET(DISP_REG_CONFIG_MUTEX(gBLSMutexID), 1);
    if(disp_poll_for_reg(DISP_REG_CONFIG_MUTEX(gBLSMutexID), 0x2, 0x2, POLLING_TIME_OUT))
    {
        printf("[DDP] error! disp_bls_get_mutex(), get mutex timeout! \n");
        disp_dump_reg(DISP_MODULE_CONFIG);        
        return -1;
    }
#endif    
    return 0;
}

static int disp_bls_release_mutex()
{
#if !defined(MTK_AAL_SUPPORT)    
    if (gBLSMutexID < 0)
        return -1;
    
    DISP_REG_SET(DISP_REG_CONFIG_MUTEX(gBLSMutexID), 0);
    if(disp_poll_for_reg(DISP_REG_CONFIG_MUTEX(gBLSMutexID), 0, 0x2, POLLING_TIME_OUT))
    {
        printf("[DDP] error! disp_bls_release_mutex(), release mutex timeout! \n");
        disp_dump_reg(DISP_MODULE_CONFIG);
        return -1;
    }
#endif    
    return 0;
}

void disp_bls_init(unsigned int srcWidth, unsigned int srcHeight)
{
    struct cust_mt65xx_led *cust_led_list = get_cust_led_list();
    struct cust_mt65xx_led *cust = NULL;
    struct PWM_config *config_data = NULL;

    if(cust_led_list)
    {
        cust = &cust_led_list[MT65XX_LED_TYPE_LCD];
        if((strcmp(cust->name,"lcd-backlight") == 0) && (cust->mode == MT65XX_LED_MODE_CUST_BLS_PWM))
        {
            config_data = &cust->config_data;
            if (config_data->clock_source >= 0 && config_data->clock_source <= 3)
            {
            	unsigned int regVal = DISP_REG_GET(CLK_CFG_1);
                DISP_REG_SET(CLK_CFG_1, (regVal & ~0x3) | config_data->clock_source);
                printf("disp_bls_init : CLK_CFG_1 0x%x => 0x%x\n", regVal, DISP_REG_GET(CLK_CFG_1));
            }
            gPWMDiv = (config_data->div == 0) ? PWM_DEFAULT_DIV_VALUE : config_data->div;
            gPWMDiv &= 0x3FF;
            printf("disp_bls_init : PWM config data (%d,%d)\n", config_data->clock_source, config_data->div);
        }
    }
    
    printf("[DDP] disp_bls_init : srcWidth = %d, srcHeight = %d\n", srcWidth, srcHeight);
    printf("[DDP] disp_bls_init : CG = 0x%x, BLS_EN = 0x%x, PWM_DUTY = %d\n", 
        DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0), 
        DISP_REG_GET(DISP_REG_BLS_EN),
        DISP_REG_GET(DISP_REG_BLS_PWM_DUTY));
    
    DISP_REG_SET(DISP_REG_BLS_SRC_SIZE, (srcHeight << 16) | srcWidth);
    DISP_REG_SET(DISP_REG_BLS_PWM_DUTY, 0);
    DISP_REG_SET(DISP_REG_BLS_PWM_CON, 0x0 | (gPWMDiv << 16));
    DISP_REG_SET(DISP_REG_BLS_EN, 0x00010000);
}

int disp_bls_config(void)
{
#if !defined(MTK_AAL_SUPPORT) 
    struct cust_mt65xx_led *cust_led_list = get_cust_led_list();
    struct cust_mt65xx_led *cust = NULL;
    struct PWM_config *config_data = NULL;

    if(cust_led_list)
    {
        cust = &cust_led_list[MT65XX_LED_TYPE_LCD];
        if((strcmp(cust->name,"lcd-backlight") == 0) && (cust->mode == MT65XX_LED_MODE_CUST_BLS_PWM))
        {
            config_data = &cust->config_data;
            if (config_data->clock_source >= 0 && config_data->clock_source <= 3)
            {
		unsigned int regVal = DISP_REG_GET(CLK_CFG_1);
                DISP_REG_SET(CLK_CFG_1, (regVal & ~0x3) | config_data->clock_source);
                printf("disp_bls_config : CLK_CFG_1 0x%x => 0x%x\n", regVal, DISP_REG_GET(CLK_CFG_1));
            }
            gPWMDiv = (config_data->div == 0) ? PWM_DEFAULT_DIV_VALUE : config_data->div;
            gPWMDiv &= 0x3FF;
            printf("disp_bls_config : PWM config data (%d,%d)\n", config_data->clock_source, config_data->div);
        }
    }
    
    printf("[DDP] disp_bls_config : CG = 0x%x, BLS_EN = 0x%x, PWM_DUTY = %d\n", 
        DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0), 
        DISP_REG_GET(DISP_REG_BLS_EN),
        DISP_REG_GET(DISP_REG_BLS_PWM_DUTY));
#ifdef USE_DISP_BLS_MUTEX
    printf("[DDP] disp_bls_config : gBLSMutexID = %d\n", gBLSMutexID);
    DISP_REG_SET(DISP_REG_CONFIG_MUTEX_RST(gBLSMutexID), 1);
    DISP_REG_SET(DISP_REG_CONFIG_MUTEX_RST(gBLSMutexID), 0);
    DISP_REG_SET(DISP_REG_CONFIG_MUTEX_MOD(gBLSMutexID), 0x200);    // BLS
    DISP_REG_SET(DISP_REG_CONFIG_MUTEX_SOF(gBLSMutexID), 0);        // single mode
    DISP_REG_SET(DISP_REG_CONFIG_MUTEX_EN(gBLSMutexID), 1);

    if (disp_bls_get_mutex() == 0)
    {
#else
        DISP_REG_SET(DISP_REG_BLS_DEBUG, 0x3);
#endif

        DISP_REG_SET(DISP_REG_BLS_PWM_DUTY, 0);
        DISP_REG_SET(DISP_REG_BLS_PWM_CON, 0x0 | (gPWMDiv << 16));
        DISP_REG_SET(DISP_REG_BLS_EN, 0x00010000);

#ifdef USE_DISP_BLS_MUTEX
        if (disp_bls_release_mutex() == 0)
            return 0;
    }
    return -1;
#else
    DISP_REG_SET(DISP_REG_BLS_DEBUG, 0x0);
#endif

#endif
    return 0;
}


#if defined(CKT_SGM3727_SUPPORT)
int disp_bls_set_backlight(unsigned int level)
{
	unsigned long value,i;
	static unsigned long temp_value =0;
	u32 gpio_num;
	gpio_num = (GPIO90|0x80000000);
	printf("[LED_luosen]GPIO#%d:%d\n",gpio_num,level);
	mt_set_gpio_mode(gpio_num,GPIO_MODE_GPIO);
	mt_set_gpio_dir(gpio_num,GPIO_DIR_OUT);

	if (level == 0)
	{
	mt_set_gpio_out(gpio_num,GPIO_OUT_ZERO);
		pre_state = 0;
		pre_level = 0;
		cur_level = 0;
	//printf("luosen_backlight off\n");
       }
	else
     {
	int count = 0, i = 0;
	volatile int j =0;
	//yan test 2~31
	#if 1
	level = (29 * level - 580/*(20 * 29)*/) / 235;
	#else
	level = (31 * (level - 20)) / 235;
	#endif
	level = 31 - level;
printf("[ysm]backlight on >>> pre_state: %d\n", pre_state);
	 if(pre_state == 0)
	 {
		if(level < 32 && level >= 0)
		{
			int i = 0;
			mt_set_gpio_out(gpio_num, GPIO_OUT_ZERO);
		//Delayms(5);
			mdelay(5);
			mt_set_gpio_out(gpio_num, GPIO_OUT_ONE);
		//udelay(50);
		//gpio_delay( 40 );
			for( j=0; j< 1000; j++); //3000 个大概25us			
             //local_irq_disable();  //防止中断导致时间延长
			for(i = 0; i < level; i++)
			{
			mt_set_gpio_out(gpio_num, GPIO_OUT_ZERO);
			//	udelay(2);
	             //	gpio_delay( 10 );
			for( j=0; j< 5; j++);
			mt_set_gpio_out(gpio_num, GPIO_OUT_ONE);
		        //udelay(4);
		       //gpio_delay( 10 );
			for( j=0; j< 5; j++);
			}
                    //local_irq_enable();
			pre_state = 1;
			pre_level = level;
			cur_level = level;
		}
	}
	else
	{
		pre_state = 1;
		cur_level = level;
//printk("[ysm]backlight on >>> pre_level: %d >>> cur_level: %d\n", pre_level, cur_level);
		
		if(pre_level != cur_level)
		{
			if(pre_level > cur_level)
			{
				//count = pre_level - cur_level;
				count = 32 + cur_level - pre_level;
			}
			else
			{
				count = cur_level - pre_level;
				//count = 32 + pre_level - cur_level;
			}
               //printk("[ysm]backlight on >>> count: %d\n", count);

              //local_irq_disable();
			for(i = 0; i < count; i++)
			{
				mt_set_gpio_out(gpio_num, GPIO_OUT_ZERO);
		//	udelay(2);
		//	gpio_delay( 10 );
			for( j=0; j< 5; j++);
			mt_set_gpio_out(gpio_num, GPIO_OUT_ONE);
	       //	udelay(4);
		//	gpio_delay( 10 );
			for( j=0; j< 5; j++);

			}			
             //local_irq_enable();
			
			pre_level = cur_level;
		}
	}
}
}

#else
int disp_bls_set_backlight(unsigned int level)
{
    printf("[DDP] disp_bls_set_backlight: %d, CG = 0x%x, BLS_EN = 0x%x, PWM_DUTY = %d\n", 
        level,
        DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0), 
        DISP_REG_GET(DISP_REG_BLS_EN),
        DISP_REG_GET(DISP_REG_BLS_PWM_DUTY));
    
    if (level && (!(DISP_REG_GET(DISP_REG_BLS_EN) & 0x10000)))
    {
        disp_bls_config();
    }

#ifdef USE_DISP_BLS_MUTEX
    disp_bls_get_mutex();
#else
    DISP_REG_SET(DISP_REG_BLS_DEBUG, 0x3);
#endif

    DISP_REG_SET(DISP_REG_BLS_PWM_DUTY, brightness_mapping(level));
    printf("[DDP] PWM_DUTY: %x\n", DISP_REG_GET(DISP_REG_BLS_PWM_DUTY));

#ifdef USE_DISP_BLS_MUTEX
    disp_bls_release_mutex();
#else
    DISP_REG_SET(DISP_REG_BLS_DEBUG, 0x0);
#endif

    return 0;    
}
#endif

