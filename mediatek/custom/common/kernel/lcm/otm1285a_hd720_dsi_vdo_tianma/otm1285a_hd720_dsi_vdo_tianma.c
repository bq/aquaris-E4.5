#include "lcm_drv.h"
#include <cust_gpio_usage.h>
#ifdef BUILD_LK
	#include <platform/mt_gpio.h>
	#include <string.h>
#else
	#include <mach/mt_gpio.h>
	#include <linux/string.h>
#endif
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(720)
#define FRAME_HEIGHT 										(1280)

#define REGFLAG_DELAY             							0XFE
#define REGFLAG_END_OF_TABLE      							0xFF   // END OF REGISTERS MARKER

#define OTM1285A_LCM_ID										(0x1285)

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

#ifndef BUILD_LK
static unsigned int lcm_esd_test = FALSE;      ///only for ESD test
#endif
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))


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
extern void DSI_clk_HS_mode(unsigned char enter);
static void DSI_Enter_ULPM(bool enter)
{
	DSI_clk_ULP_mode(enter);  //enter ULPM
	DSI_lane0_ULP_mode(enter);
}


static LCM_setting_table_V3 lcm_initialization_setting[] = {
    {0x15,0x00,1,{0x00}},
	{0x39,0xFF,3,{0x12,0x85,0x01}},
	{0x15,0x00,1,{0x80}},
	{0x39,0xFF,2,{0x12,0x85}},
	{0x15,0x51,1,{0xFF}},
	{0x15,0x53,1,{0x2C}},
	{0x15,0x55,1,{0x00}},
    {0x05,0x11,1,{0x00}}, //sleep out
    {REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 200, {}},
    	
    {0x05,0x29,1,{0x00}},//DISPLAY ON
    {REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 50, {}},
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

    #if (LCM_DSI_CMD_MODE)
	params->dsi.mode   = CMD_MODE;
    #else
	params->dsi.mode   = BURST_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE; 
    #endif

	// DSI
	/* Command mode setting */
	params->dsi.LANE_NUM				= LCM_FOUR_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

	// Highly depends on LCD driver capability.
	// Not support in MT6573
	params->dsi.packet_size=256;

	// Video mode setting		
	params->dsi.intermediat_buffer_num = 2;

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
	MDELAY(105); 
		
	dsi_set_cmdq_V3(lcm_initialization_setting,sizeof(lcm_initialization_setting)/sizeof(lcm_initialization_setting[0]),1);

	#ifdef BUILD_LK
	printf("[LK]---cmd---otm1285a_hd720_dsi_vdo_tianma----%s------\n",__func__);
    #else
	printk("[KERNEL]---cmd---otm1285a_hd720_dsi_vdo_tianma----%s------\n",__func__);
    #endif	
}



static void lcm_suspend(void)
{
	unsigned int data_array[16];

	data_array[0]=0x00280500; // Display Off
	dsi_set_cmdq(data_array, 1, 1);
	
	data_array[0] = 0x00100500; // Sleep In
	dsi_set_cmdq(data_array, 1, 1);

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
		lcm_init();
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

static unsigned int lcm_compare_id(void)
{
	unsigned int id=0;
	unsigned char buffer[4] = {0,0,0,0};
	unsigned int array[16];  

	dct_pmic_VGP2_enable(1);

	mt_set_gpio_mode(GPIO112,GPIO_MODE_00);
	mt_set_gpio_dir(GPIO112,GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO112,GPIO_OUT_ONE);
	MDELAY(5); 

	mt_set_gpio_out(GPIO112,GPIO_OUT_ZERO);
	MDELAY(50);

	mt_set_gpio_out(GPIO112,GPIO_OUT_ONE);
	MDELAY(105); 

	array[0]=0x00001500;
	dsi_set_cmdq(&array, 1, 1);

	array[0]=0x00043902;
	array[1]=0x018512FF;// page enable
	dsi_set_cmdq(&array, 2, 1);
	MDELAY(10);

	array[0]=0x80001500;
	dsi_set_cmdq(&array, 1, 1);

	array[0]=0x00033902;
	array[1]=0x008512FF;// page enable
	dsi_set_cmdq(&array, 2, 1);
	MDELAY(10);

	array[0]=0x00043700;// return byte number
	dsi_set_cmdq(&array, 1, 1);
	MDELAY(10);

	array[0]=0x00001500;
	dsi_set_cmdq(&array, 1, 1);

	read_reg_v2(0xA1, buffer, 4);
	id = (buffer[2]<<8)|buffer[3]; 

	#ifdef BUILD_LK
	//printf("[LK]---cmd---otm1285a_hd720_dsi_vdo_tianma----%s------[%x,%x,%x]\n",__func__,buffer[2],buffer[3],id);
    #else
	//printk("[KERNEL]---cmd---otm1285a_hd720_dsi_vdo_tianma----%s------[%x,%x,%x]\n",__func__,buffer[2],buffer[3],id);
    #endif	
	return (id == OTM1285A_LCM_ID)?1:0;

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
	DSI_clk_HS_mode(1);

	read_reg_v2(0x0A, buffer, 1);
	//printk("[%s]buffer[0]=0x%x",__func__,buffer[0]);
	DSI_clk_HS_mode(0);

	if(buffer[0]==0x9C)
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

    return TRUE;
}

// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER otm1285a_hd720_dsi_vdo_tianma_lcm_drv = 
{
    .name			= "otm1285a_hd720_dsi_vdo_tianma",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id     = lcm_compare_id,
	.esd_check 		= lcm_esd_check,
	.esd_recover 	= lcm_esd_recover,
#if (LCM_DSI_CMD_MODE)
    .update         = lcm_update,
#endif
};
