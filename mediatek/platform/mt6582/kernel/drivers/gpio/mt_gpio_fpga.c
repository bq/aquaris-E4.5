/******************************************************************************
 * mt_gpio_fpga.c - MTKLinux GPIO Device Driver
 * 
 * Copyright 2008-2009 MediaTek Co.,Ltd.
 * 
 * DESCRIPTION:
 *     This file provid the other drivers GPIO debug functions
 *
 ******************************************************************************/
 
#include <mach/mt_gpio.h>
#include <mach/mt_gpio_core.h>
#include <mach/mt_gpio_fpga.h>
//#define CONFIG_MT_GPIO_FPGA_ENABLE
/* FPGA support code */	
//#if (defined(CONFIG_MT6589_FPGA))
#define MT_GPIO_DIR_REG 0xF0001E88
#define MT_GPIO_OUT_REG 0xF0001E84
#define MT_GPIO_IN_REG  0xF0001E80

static void mt_gpio_set_bit(unsigned long nr, unsigned long reg)
{
	unsigned long value;
	value = readl(reg);
	value |= 1L << nr;
	writel(value, reg);
}

static void mt_gpio_clear_bit(unsigned long nr, unsigned long reg)
{
	unsigned long value;
	value = readl(reg);
	value &= ~(1L << nr);
	writel(value, reg);
}

static unsigned long mt_gpio_get_bit(unsigned long nr , unsigned long reg)
{
	unsigned long value;
	value = readl(reg);
	value &= (1L << nr);
	return value ? 1 : 0 ;
}

int mt_set_gpio_dir_base(unsigned long pin, unsigned long dir)
{
	int ret = RSUCCESS;
	unsigned long flags = 0;
	if (dir == GPIO_DIR_IN || dir == GPIO_DIR_DEFAULT)
		mt_gpio_clear_bit(pin, MT_GPIO_DIR_REG);
	else if (dir == GPIO_DIR_OUT)
		mt_gpio_set_bit(pin, MT_GPIO_DIR_REG);
	else
		ret = GPIO_DIR_UNSUPPORTED;
	return ret;
}
int mt_get_gpio_dir_base(unsigned long pin)
{
	int value;
	unsigned long flags = 0;
	value = mt_gpio_get_bit(pin, MT_GPIO_DIR_REG);
	return value;
}  
int mt_set_gpio_pull_enable_base(unsigned long pin, unsigned long enable)    {return RSUCCESS;}
int mt_get_gpio_pull_enable_base(unsigned long pin)                {return GPIO_PULL_EN_UNSUPPORTED;}
int mt_set_gpio_pull_select_base(unsigned long pin, unsigned long select)    {return RSUCCESS;}
int mt_get_gpio_pull_select_base(unsigned long pin)                {return GPIO_PULL_UNSUPPORTED;}
int mt_set_gpio_inversion_base(unsigned long pin, unsigned long enable)      {return RSUCCESS;}
int mt_get_gpio_inversion_base(unsigned long pin)                  {return GPIO_DATA_INV_UNSUPPORTED;}
int mt_set_gpio_ies_base(unsigned long pin, unsigned long enable)	 {return RSUCCESS;}
int mt_get_gpio_ies_base(unsigned long pin)                  {return GPIO_IES_UNSUPPORTED;}

int mt_set_gpio_out_base(unsigned long pin, unsigned long output)
{
	int ret = RSUCCESS;
	unsigned long flags = 0;
	if (output == GPIO_OUT_ZERO || output == GPIO_OUT_DEFAULT || output == GPIO_DATA_OUT_DEFAULT) {
		mt_gpio_clear_bit(pin, MT_GPIO_OUT_REG);
	} else if (output == GPIO_OUT_ONE) {
		mt_gpio_set_bit(pin, MT_GPIO_OUT_REG);
	} else {
		ret = GPIO_OUT_UNSUPPORTED;
	}
	return ret;
}
int mt_get_gpio_out_base(unsigned long pin)
{
	int value;
	unsigned long flags = 0;
	value = mt_gpio_get_bit(pin, MT_GPIO_OUT_REG);
	
	return value;
}
int mt_get_gpio_in_base(unsigned long pin) 
{
	int value;
	unsigned long flags = 0;
	value = mt_gpio_get_bit(pin, MT_GPIO_IN_REG);
	return value;
}
int mt_set_gpio_mode_base(unsigned long pin, unsigned long mode)             {return RSUCCESS;}
int mt_get_gpio_mode_base(unsigned long pin)                       {return GPIO_MODE_UNSUPPORTED;}
int mt_set_clock_output_base(unsigned long num, unsigned long src, unsigned long div)    {return RSUCCESS;}
int mt_get_clock_output_base(unsigned long num, unsigned long *src, unsigned long *div)  {return CLK_SRC_UNSUPPORTED;}


#if 0
static ssize_t mt_gpio_dump_regs(char *buf, ssize_t bufLen)
{
    int idx = 0, len = 0;
	
	GPIOMSG("PIN: [DIR] [DOUT] [DIN]\n");
    for (idx = 0; idx < 8; idx++) {        
        len += snprintf(buf+len, bufLen-len, "%d: %d %d %d\n",
               idx, mt_get_gpio_dir(idx), mt_get_gpio_out(idx),mt_get_gpio_in(idx)); 
    }
	GPIOMSG("PIN: [MODE] [PULL_SEL] [DIN] [DOUT] [PULL EN] [DIR] [INV]\n");
    for (idx = GPIO_EXTEND_START; idx < MAX_GPIO_PIN; idx++) {        
		len += snprintf(buf+len, bufLen-len, "%d: %d %d %d %d %d %d %d\n",
		   idx,mt_get_gpio_mode(idx), mt_get_gpio_pull_select(idx), mt_get_gpio_in(idx),mt_get_gpio_out(idx),
		   mt_get_gpio_pull_enable(idx),mt_get_gpio_dir(idx),mt_get_gpio_inversion(idx)); 
    }

    return len;
}
#endif
/*---------------------------------------------------------------------------*/

