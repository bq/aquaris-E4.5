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
#define LCM_ID_HX8389B (0x89)
#define ADC0_LOW_TIANMA (1360)
#define ADC0_HIGH_TIANMA (1440)

#define FRAME_WIDTH  (540)
#define FRAME_HEIGHT (960)

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
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
	pmic_config_interface(DIGLDO_CON29, 0x5, PMIC_RG_VGP2_VOSEL_MASK, PMIC_RG_VGP2_VOSEL_SHIFT); // 2.8v ?? ¡§?? 2013?¡§o10??31¡§¡§? 17:55:43
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
static LCM_setting_table_V3 lcm_initialization_setting[] = {
	
	/*
	Note :

	Data ID will depends on the following rule.
	
		count of parameters > 1	=> Data ID = 0x39
		count of parameters = 1	=> Data ID = 0x15
		count of parameters = 0	=> Data ID = 0x05

	Structure Format :

	{DCS command, count of parameters, {parameter list}}
	{REGFLAG_DELAY, milliseconds of time, {}},

	...

	Setting ending by predefined flag
	
	{REGFLAG_END_OF_TABLE, 0x00, {}}
	*/
	{0x39,0xB9, 3 ,{0xFF,0x83,0x89}},
	{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3,  1, {}},
	
	//{0x39,0xBA, 7 ,{0x41,0x83,0x00,0x16,0xA4,0x00,0x18}}, // couldn't readout register value correctly.
	{0x39,0xBA, 7 ,{0x01,0x92,0x00,0x16,0xA4,0x00,0x18}},
	{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3,  1, {}},
	
	{0x15,0xC6, 1 ,{0x08}},
	{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3,  1, {}},
	
	{0x39,0xDE, 2 ,{0x05,0x58}},
	{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3,  1, {}},
	
	{0x39,0xB1, 19 ,{0x00,0x00,0x04,0xD9,0x8F,0x10,0x11,0xEC,0xEC,0x1D,0x25,0x1D,0x1D,0x42,0x01,0x5A,0xF7,0x20,0x80}},
	{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3,  10, {}},
	
	{0x39,0xB2, 7 ,{0x00,0x00,0x78,0x01,0x02,0x3F,0x80}},
	{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3,  1, {}},
	
	{0x39,0xB4, 31 ,{0x80,0x04,0x00,0x32,0x10,0x00,0x32,0x10,0x00,0x00,0x00,0x00,0x17,0x0A,0x40,0x01,0x13,0x0A,0x40,0x14,0xFF,0x50,0x0A,0x0A,0x3C,0x0A,0x3C,0x14,0x46,0x50,0x0A}},
	{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3,  10, {}},
	
	{0x39,0xD5, 48 ,{0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x20,0x00,0x99,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x01,0x88,0x23,0x01,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x99,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x32,0x88,0x10,0x10,0x88,0x88,0x88,0x88,0x88,0x88}},
	{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3,  10, {}},
	

	{0x39,0xE0, 34 ,{0x05,0x11,0x14,0x37,0x3F,0x3F,0x20,0x43,0x05,0x0E,0x0D,0x12,0x14,0x12,0x14,0x10,0x1C,0x05,0x11,0x14,0x37,0x3F,0x3F,0x20,0x43,0x05,0x0E,0x0D,0x12,0x14,0x12,0x14,0x10,0x1C}},
	{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3,  5, {}},
	
	{0x39,0xB6, 4 ,{0x00,0x87,0x00,0x87}},  //0x8A
	{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3,  1, {}},
	
	{0x15,0xCC, 1 ,{0x02}},
	//{0x15,0xCC, 1 ,{0x06}},
	
	{0x05,0x11, 0 ,{}},
	{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3,  120, {}},
	{0x05,0x29, 0 ,{}},
	{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3,  10, {}}


};



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

    params->dbi.te_mode 				= LCM_DBI_TE_MODE_VSYNC_ONLY;
    params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;
    #if (LCM_DSI_CMD_MODE)
	params->dsi.mode   = CMD_MODE;
    #else
	params->dsi.mode   = SYNC_PULSE_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE; 
    #endif

	params->dsi.LANE_NUM = LCM_TWO_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.intermediat_buffer_num = 2;

	params->dsi.vertical_sync_active = 2;
	params->dsi.vertical_backporch = 3; //10;
	params->dsi.vertical_frontporch = 3;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 0x16;//20;
	params->dsi.horizontal_backporch = 0x38; //46;
	params->dsi.horizontal_frontporch = 0x18;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	
    //params->dsi.LPX=8; 

	// Bit rate calculation
	//1 Every lane speed
	//params->dsi.pll_select=1;
	//params->dsi.PLL_CLOCK  = LCM_DSI_6589_PLL_CLOCK_377;
	params->dsi.PLL_CLOCK=250;
	//params->dsi.pll_div1=0;		// div1=0,1,2,3;div1_real=1,2,4,4 ----0: 546Mbps  1:273Mbps
	//params->dsi.pll_div2=0;		// div2=0,1,2,3;div1_real=1,2,4,4	
#if (LCM_DSI_CMD_MODE)
	//params->dsi.fbk_div =9;
#else
	//params->dsi.fbk_div =9;    // fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)	
#endif
	//params->dsi.compatibility_for_nvk = 1;		// this parameter would be set to 1 if DriverIC is NTK's and when force match DSI clock for NTK's
}


static void lcm_init(void)
{
	lcm_is_init = true;
	dct_pmic_VGP2_enable(1);
	MDELAY(10);
	mt_set_gpio_mode(GPIO112,GPIO_MODE_00);
	mt_set_gpio_dir(GPIO112,GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO112,GPIO_OUT_ONE);
	MDELAY(5); 
    
	//SET_RESET_PIN(0);
	mt_set_gpio_out(GPIO112,GPIO_OUT_ZERO);
	MDELAY(50);
	
	//SET_RESET_PIN(1);
	mt_set_gpio_out(GPIO112,GPIO_OUT_ONE);
	MDELAY(20); 
	
	dsi_set_cmdq_V3(lcm_initialization_setting,sizeof(lcm_initialization_setting)/sizeof(lcm_initialization_setting[0]),1);
}



static void lcm_suspend(void)
{
	unsigned int data_array[16];

	//data_array[0]=0x00280500; // Display Off
	//dsi_set_cmdq(data_array, 1, 1);
	
	data_array[0] = 0x00100500; // Sleep In
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);

	DSI_Enter_ULPM(1); /* Enter low power mode  */
	MDELAY(60);


	//SET_RESET_PIN(0);
	mt_set_gpio_out(GPIO112,GPIO_OUT_ZERO);
	MDELAY(150);

	dct_pmic_VGP2_enable(0); /* [Ted 5-28] Disable VCI power to prevent lcd polarization */
	lcm_is_init = false;
}

static void lcm_resume(void)
{
	if(!lcm_is_init)
		dct_pmic_VGP2_enable(1);
	MDELAY(10);
	mt_set_gpio_mode(GPIO112,GPIO_MODE_00);
	mt_set_gpio_dir(GPIO112,GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO112,GPIO_OUT_ONE);
	MDELAY(5);
    
	//SET_RESET_PIN(0);
	mt_set_gpio_out(GPIO112,GPIO_OUT_ZERO);
	MDELAY(50);
	
	//SET_RESET_PIN(1);
	mt_set_gpio_out(GPIO112,GPIO_OUT_ONE);
	MDELAY(120); 
		
	dsi_set_cmdq_V3(lcm_initialization_setting,sizeof(lcm_initialization_setting)/sizeof(lcm_initialization_setting[0]),1);
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

extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);

static unsigned int lcm_compare_id(void)
{

	unsigned int id=0;
	unsigned char buffer[2];
	unsigned int array[16];  
	int data[4];
	int auxadc0_vol = 0;

	dct_pmic_VGP2_enable(1);
	MDELAY(10);
	mt_set_gpio_mode(GPIO112,GPIO_MODE_00);
	mt_set_gpio_dir(GPIO112,GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO112,GPIO_OUT_ONE);
	MDELAY(5); 
		    
	//SET_RESET_PIN(0);
	mt_set_gpio_out(GPIO112,GPIO_OUT_ZERO);
	MDELAY(50);
			
	//SET_RESET_PIN(1);
	mt_set_gpio_out(GPIO112,GPIO_OUT_ONE);
	MDELAY(100);

	array[0]=0x00043902;
	array[1]=0x8983FFB9;// page enable
	dsi_set_cmdq(array, 2, 1);
	MDELAY(10);

	array[0] = 0x00033902;                          
	array[1] = 0x009201BA;       	  
	dsi_set_cmdq(array, 2, 1);
	MDELAY(10);

	array[0] = 0x00013700;// return byte number
	dsi_set_cmdq(array, 1, 1);
	MDELAY(10);

	read_reg_v2(0xF4, buffer, 1);
	id  =  buffer[0];

#ifdef BUILD_LK
	printf("%s, id = 0x%02x \n", __func__, id);
#else
	printk("%s, id = 0x%02x \n", __func__, id);
#endif

	if(LCM_ID_HX8389B == id)
	{
		IMM_GetOneChannelValue(0,data,NULL);
		auxadc0_vol = data[0]*1000 + data[1]*10;
	#ifdef BUILD_LK
		printf("[LK]hx8389_qhd_dsi_vdo_tianma--adc0 vol %d\n",auxadc0_vol);
	#else
		printk("[KERNEL]hx8389_qhd_dsi_vdo_tianma--adc0 vol %d\n",auxadc0_vol);
	#endif	
		return ((ADC0_LOW_TIANMA <= auxadc0_vol)&&(ADC0_HIGH_TIANMA >= auxadc0_vol))?1:0;
	}
	return 0;
}



static unsigned int lcm_esd_check(void)
{
	unsigned int ret=FALSE;
#ifndef BUILD_LK
	char  buffer[6];
	int   array[4];

	array[0] = 0x00023700;
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x0A, buffer, 2);
    printk("tianma lcm_esd_check=0x%x",buffer[0]);
	if(buffer[0] == 0x1C)
	{
		ret=FALSE;
	}
	else
	{			 
		ret=TRUE;
	}
#endif
	return ret;
}

static unsigned int lcm_esd_recover(void)
{
	#ifndef BUILD_LK
	dct_pmic_VGP2_enable(1);
	MDELAY(10);
	mt_set_gpio_mode(GPIO112,GPIO_MODE_00);
	mt_set_gpio_dir(GPIO112,GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO112,GPIO_OUT_ONE);
	MDELAY(5); 

	//SET_RESET_PIN(0);
	mt_set_gpio_out(GPIO112,GPIO_OUT_ZERO);
	MDELAY(50);

	//SET_RESET_PIN(1);
	mt_set_gpio_out(GPIO112,GPIO_OUT_ONE);
	MDELAY(120); 

	dsi_set_cmdq_V3(lcm_initialization_setting,sizeof(lcm_initialization_setting)/sizeof(lcm_initialization_setting[0]),1);
	
	printk("lcm_esd_recover  hx8389b_video_tianma \n");
	#endif
	return TRUE;
}




LCM_DRIVER hx8389b_qhd_dsi_vdo_tianma_lcm_drv = 
{
    .name			= "hx8389b_qhd_dsi_vdo_tianma",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id     = lcm_compare_id,
	.esd_check		= lcm_esd_check,
	.esd_recover 	= lcm_esd_recover,
#if (LCM_DSI_CMD_MODE)
    .update         = lcm_update,
#endif
};
