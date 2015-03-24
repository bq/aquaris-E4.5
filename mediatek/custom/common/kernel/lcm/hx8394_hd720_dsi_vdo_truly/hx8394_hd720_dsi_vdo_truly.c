#ifndef BUILD_LK
#include <linux/string.h>
#endif
#include "lcm_drv.h"
#include <cust_gpio_usage.h>
#ifdef BUILD_LK
	#include <platform/mt_gpio.h>
	#include <string.h>
#elif defined(BUILD_UBOOT)
	#include <asm/arch/mt_gpio.h>
#else
	#include <mach/mt_gpio.h>
#endif
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (720)
#define FRAME_HEIGHT (1280)

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

#define HX8394_LCM_ID				(0x94)

#ifndef BUILD_LK
static unsigned int lcm_esd_test = FALSE;      ///only for ESD test
#endif
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V3(para_tbl,size,force_update)        lcm_util.dsi_set_cmdq_V3(para_tbl,size,force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	        lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)											lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)



#define   LCM_DSI_CMD_MODE							0

static bool lcm_is_init = false;


#ifdef BUILD_LK
#include  <platform/mt_pmic.h>
static void dct_pmic_VGP2_enable(bool dctEnable)
{
	pmic_config_interface(DIGLDO_CON29, 0x5, PMIC_RG_VGP2_VOSEL_MASK, PMIC_RG_VGP2_VOSEL_SHIFT); // 2.8v ËÕ ÓÂ 2013Äê10ÔÂ31ÈÕ 17:55:43
	pmic_config_interface( (U32)(DIGLDO_CON8),
                             (U32)(dctEnable),
                             (U32)(PMIC_RG_VGP2_EN_MASK),
                             (U32)(PMIC_RG_VGP2_EN_SHIFT)
	                         );
}
#else
void dct_pmic_VGP2_enable(bool dctEnable);
#endif

extern void DSI_clk_ULP_mode(bool enter);
extern void DSI_lane0_ULP_mode(bool enter);
static void DSI_Enter_ULPM(bool enter)
{
	DSI_clk_ULP_mode(enter);  //enter ULPM
	DSI_lane0_ULP_mode(enter);
}

#if 0
struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] = {
    {0xB9,3,{0xFF,0x83,0x94}},//SET PASSWORD
    {0xBA,1,{0x13}},//SET MIPI 4 LANE
    {0xB1,16,{0x01,0x00,0x07,0x8A,0x01,0x12,0x12,0x2F,0x37,0x3F,0x3F,0x47,0x12,0x01,0xE6,0xE2}},//SET POWER
    {0xB4,22,{0x80,0x06,0x32,0x10,0x03,0x32,0x15,0x08,0x32,0x10,0x08,0x33,0x04,0x43,0x05,0x37,0x04,0x3F,0x06,0x61,0x61,0x06}},//SET CYC
    //TC358768_Initial_Setting_over_8bytes(}},
    {0xB2,6,{0x00,0xC8,0x08,0x04,0x00,0x22}},//SET DISPLAY RELATED REGISTER
    {0xD5,32,{0x00,0x00,0x00,0x00,0x0A,0x00,0x01,0x00,0xCC,0x00,0x00,0x00,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x01,0x67,0x45,0x23,0x01,0x23,0x88,0x88,0x88,0x88}},//SET GIP
    //TC358768_Initial_Setting_over_8bytes(}},
    {0xC7,4,{0x00,0x10,0x00,0x10}},//SET TCON
    {0xBF,4,{0x06,0x00,0x10,0x04}},// Row line issue(reduce charge pumb ripple)
    {0xCC,1,{0x09}},// SET PANEL
    {0xE0,42,{0x00,0x05,0x08,0x2B,0x33,0x3F,0x15,0x36,0x07,0x0D,0x0E,0x11,0x12,0x11,0x12,0x10,0x17,0x00,0x05,0x08,0x2B,0x33,0x3F,0x15,0x36,0x07,0x0D,0x0E,0x11,0x12,0x11,0x12,0x10,0x17,0x0B,0x17,0x06,0x11,0x0B,0x17,0x06,0x11}},//SET GAMMA
    {0xB6,1,{0x09}},// VCOM
    {0xD4,1,{0x32}},
    {0x35,1,{0x00}},
    	
    {0x11,1,{0x00}}, //sleep out
    {REGFLAG_DELAY, 200, {}},
    	
    {0x29,1,{0x00}},//DISPLAY ON
    {REGFLAG_DELAY, 50, {}},
};
#else
static LCM_setting_table_V3 lcm_initialization_setting[] = {
    {0x39,0xB9,3,{0xFF,0x83,0x94}},//SET PASSWORD
    {0x15,0xBA,1,{0x13}},//SET MIPI 4 LANE
    {0x39,0xB1,16,{0x01,0x00,0x07,0x8A,0x01,0x12,0x12,0x2F,0x37,0x3F,0x3F,0x47,0x12,0x01,0xE6,0xE2}},//SET POWER
    {0x39,0xB4,22,{0x80,0x06,0x32,0x10,0x03,0x32,0x15,0x08,0x32,0x10,0x08,0x33,0x04,0x43,0x05,0x37,0x04,0x3F,0x06,0x61,0x61,0x06}},//SET CYC
    //TC358768_Initial_Setting_over_8bytes(}},
    {0x39,0xB2,6,{0x00,0xC8,0x08,0x04,0x00,0x22}},//SET DISPLAY RELATED REGISTER
    {0x39,0xD5,32,{0x00,0x00,0x00,0x00,0x0A,0x00,0x01,0x00,0xCC,0x00,0x00,0x00,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x01,0x67,0x45,0x23,0x01,0x23,0x88,0x88,0x88,0x88}},//SET GIP
    //TC358768_Initial_Setting_over_8bytes(}},
    {0x39,0xC7,4,{0x00,0x10,0x00,0x10}},//SET TCON
    {0x39,0xBF,4,{0x06,0x00,0x10,0x04}},// Row line issue(reduce charge pumb ripple)
    {0x15,0xCC,1,{0x09}},// SET PANEL
    {0x39,0xE0,42,{0x00,0x05,0x08,0x2B,0x33,0x3F,0x15,0x36,0x07,0x0D,0x0E,0x11,0x12,0x11,0x12,0x10,0x17,0x00,0x05,0x08,0x2B,0x33,0x3F,0x15,0x36,0x07,0x0D,0x0E,0x11,0x12,0x11,0x12,0x10,0x17,0x0B,0x17,0x06,0x11,0x0B,0x17,0x06,0x11}},//SET GAMMA
    {0x15,0xB6,1,{0x09}},// VCOM
    {0x15,0xD4,1,{0x32}},
    {0x15,0x35,1,{0x00}},
    	
    {0x05,0x11,1,{0x00}}, //sleep out
    {REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 200, {}},
    	
    {0x05,0x29,1,{0x00}},//DISPLAY ON
    {REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 50, {}},
};
#endif

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
		memset(params, 0, sizeof(LCM_PARAMS));
	
		params->type   = LCM_TYPE_DSI;

		params->width  = FRAME_WIDTH;
		params->height = FRAME_HEIGHT;

        #if (LCM_DSI_CMD_MODE)
		params->dsi.mode   = CMD_MODE;
        #else
		params->dsi.mode   = BURST_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE; 
        #endif
	
		// DSI
		/* Command mode setting */
		//1 Three lane or Four lane
		params->dsi.LANE_NUM				= LCM_FOUR_LANE;
		//The following defined the fomat for data coming from LCD engine.
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

		// Video mode setting		
		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
		
		params->dsi.vertical_sync_active				= 0x05;// 3    2
		params->dsi.vertical_backporch					= 0x5; //0x0d;// 20   1
		params->dsi.vertical_frontporch					= 0x0d;//0x08; // 1  12
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 

		params->dsi.horizontal_sync_active				= 0x12;// 50  2
		params->dsi.horizontal_backporch				= 0x5f;
		params->dsi.horizontal_frontporch				= 0x5f;
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

	    //params->dsi.LPX=8; 

		// Bit rate calculation
		params->dsi.PLL_CLOCK = 250;//240;
		//1 Every lane speed
		params->dsi.pll_div1=0;		// div1=0,1,2,3;div1_real=1,2,4,4 ----0: 546Mbps  1:273Mbps
		params->dsi.pll_div2=0;		// div2=0,1,2,3;div1_real=1,2,4,4	
		params->dsi.fbk_div =9;    // fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)	

}


static void lcm_init(void)
{
		//SET_RESET_PIN(0);
		//MDELAY(20); 
		//SET_RESET_PIN(1);
		//MDELAY(20); 

		lcm_is_init = true;
		dct_pmic_VGP2_enable(1);
		MDELAY(5);
		mt_set_gpio_mode(GPIO112,GPIO_MODE_00);
		mt_set_gpio_dir(GPIO112,GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO112,GPIO_OUT_ONE);
		MDELAY(5); 
	    
		//SET_RESET_PIN(0);
		mt_set_gpio_out(GPIO112,GPIO_OUT_ZERO);
		MDELAY(5);
		
		//SET_RESET_PIN(1);
		mt_set_gpio_out(GPIO112,GPIO_OUT_ONE);
		MDELAY(20); 
		
		dsi_set_cmdq_V3(lcm_initialization_setting,sizeof(lcm_initialization_setting)/sizeof(lcm_initialization_setting[0]),1);

	#ifdef BUILD_LK
	  printf("[LK]---cmd---hx8394_hd720_dsi_vdo_truly----%s------\n",__func__);
    #else
	  printk("[KERNEL]---cmd---hx8394_hd720_dsi_vdo_truly----%s------\n",__func__);
    #endif	
}



static void lcm_suspend(void)
{
	unsigned int data_array[16];

	data_array[0]=0x00280500; // Display Off
	dsi_set_cmdq(data_array, 1, 1);
	
	data_array[0] = 0x00100500; // Sleep In
	dsi_set_cmdq(data_array, 1, 1);

	#if 1
	DSI_Enter_ULPM(1); /* Enter low power mode  */

	MDELAY(60);
	//SET_RESET_PIN(0);
	mt_set_gpio_out(GPIO112,GPIO_OUT_ZERO);
	MDELAY(150);

	dct_pmic_VGP2_enable(0); /* [Ted 5-28] Disable VCI power to prevent lcd polarization */
	lcm_is_init = false;
	
	#else
	SET_RESET_PIN(1);	
	SET_RESET_PIN(0);
	MDELAY(1); // 1ms
	
	SET_RESET_PIN(1);
	MDELAY(120);     
	lcm_util.set_gpio_out(GPIO_LCD_ENN, GPIO_OUT_ZERO);
	lcm_util.set_gpio_out(GPIO_LCD_ENP, GPIO_OUT_ZERO); 
	#endif
}


static void lcm_resume(void)
{
	#if 1
	if(!lcm_is_init)
		lcm_init();
	#else
	lcm_util.set_gpio_out(GPIO_LCD_ENN, GPIO_OUT_ONE);
	lcm_util.set_gpio_out(GPIO_LCD_ENP, GPIO_OUT_ONE);
	lcm_init();
	#endif
}
         
#if (LCM_DSI_CMD_MODE)
static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
	
	data_array[0]= 0x00053902;
	data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[2]= (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]= 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);

}
#endif

extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int *rawdata);

static unsigned int lcm_compare_id(void)
{
#if 0
  	unsigned int ret = 0;

	ret = mt_get_gpio_in(GPIO92);
#if defined(BUILD_LK)
	printf("%s, [jx]hx8394a GPIO92 = %d \n", __func__, ret);
#endif	

	return (ret == 0)?1:0; 
#else
	unsigned int id=0;
	unsigned char buffer[2] = {0,0};
	unsigned int array[16];  

	dct_pmic_VGP2_enable(1);

	mt_set_gpio_mode(GPIO112,GPIO_MODE_00);
	mt_set_gpio_dir(GPIO112,GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO112,GPIO_OUT_ONE);
	MDELAY(5); 
		    
	//SET_RESET_PIN(0);
	mt_set_gpio_out(GPIO112,GPIO_OUT_ZERO);
	MDELAY(50);
			
	//SET_RESET_PIN(1);
	mt_set_gpio_out(GPIO112,GPIO_OUT_ONE);
	MDELAY(105); 

	array[0] = 0x00043902;
	array[1] = 0x9483FFB9;// page enable
	dsi_set_cmdq(&array, 2, 1);
	MDELAY(10);

	array[0] = 0x00023902;
	array[1] = 0x000013BA;       	  
	dsi_set_cmdq(array, 2, 1);
	MDELAY(10);

	array[0] = 0x00013700;// return byte number
	dsi_set_cmdq(&array, 1, 1);
	MDELAY(10);

	read_reg_v2(0xF4, buffer, 1);
	id = buffer[0]; 

	#ifdef BUILD_LK
	printf("[LK]---cmd---hx8394_hd720_dsi_vdo_truly----%s------[0x%x]\n",__func__,buffer[0]);
    #else
	printk("[KERNEL]---cmd---hx8394_hd720_dsi_vdo_truly----%s------[0x%x]\n",__func__,buffer[0]);
    #endif	
	if(id==HX8394_LCM_ID)
	{
		int adcdata[4];
		int lcmadc=0;
		IMM_GetOneChannelValue(0,adcdata,&lcmadc);
		lcmadc = lcmadc * 1500/4096; 
		#ifdef BUILD_LK
		printf("[LK]---cmd---hx8394_hd720_dsi_vdo_truly----%s------adc[%d]\n",__func__,lcmadc);
		#else
		printk("[KERNEL]---cmd---hx8394_hd720_dsi_vdo_truly----%s------adc[%d]\n",__func__,lcmadc);
		#endif	
		if(lcmadc < 200)
			return 1;
	}
	return 0;//(id == HX8394_LCM_ID)?1:0;
#endif
}


static unsigned int lcm_esd_check(void)
{
  #ifndef BUILD_LK
	char  buffer[3];
	int   array[4];

	if(lcm_esd_test)
	{
		lcm_esd_test = FALSE;
		return TRUE;
	}

	array[0] = 0x00013700;
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x0a, buffer, 1);
	printk("[%s]buffer[0]=0x%x",__func__,buffer[0]);
	if(buffer[0]==0x1c)
	{
		return FALSE;
	}
	else
	{			 
		return TRUE;
	}
#else
	return FALSE;
#endif

}

static unsigned int lcm_esd_recover(void)
{
	lcm_init();
	//lcm_resume();

	return TRUE;
}



LCM_DRIVER hx8394_hd720_dsi_vdo_truly_lcm_drv = 
{
    .name			= "hx8394_hd720_dsi_vdo_truly",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id     = lcm_compare_id,
	.esd_check = lcm_esd_check,
	.esd_recover = lcm_esd_recover,
#if (LCM_DSI_CMD_MODE)
    .update         = lcm_update,
#endif
    };
