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
#define ADC0_LOW_TRULY (0)
#define ADC0_HIGH_TRULY (40)

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

static void init_lcm_registers(void)
{

	/*
		Note :
	
		Data ID will depends on the following rule.
		
			count of parameters > 1 => Data ID = 0x39
			count of parameters = 1 => Data ID = 0x15
			count of parameters = 0 => Data ID = 0x05
	
		Structure Format :
	
		{DCS command, count of parameters, {parameter list}}
		{REGFLAG_DELAY, milliseconds of time, {}},
	
		...
	
		Setting ending by predefined flag
		
		{REGFLAG_END_OF_TABLE, 0x00, {}}
		*/

    
	unsigned int data_array[16];
	
	//data_array[0] = 0x00110500; // Sleep Out
	//dsi_set_cmdq(data_array, 1, 1);
	//MDELAY(200);

	//PacketHeader[39 04 00 xx] // Set EXTC 
	//Payload[B9 FF 83 94] 
    data_array[0] = 0x00043902;                          
    data_array[1] = 0x8983ffb9;                 
    dsi_set_cmdq(data_array, 2, 1);	

      //PacketHeader[39 11 00 xx] // Set MIPI 
      //Payload[BA 13 82 00 16 C5 00 10 FF 0F 24 03 21 24 25 20 08] 
      
    data_array[0] = 0x00043902;                          
    data_array[1] = 0x105805de;       	  
    dsi_set_cmdq(data_array, 2, 1);	
      
    data_array[0] = 0x00033902;                          
    data_array[1] = 0x009201ba;       	  
    dsi_set_cmdq(data_array, 2, 1);	

   
    data_array[0] = 0x00143902;                          
    data_array[1] = 0x040000b1; 
    data_array[2] = 0x111096e3; 
    data_array[3] = 0x3028ef6f; 
    data_array[4] = 0x00422323; 
    data_array[5] = 0x0020f258;                 	 
    dsi_set_cmdq(data_array, 6, 1);	

	
	data_array[0] = 0x00083902;                          
    data_array[1] = 0x780000b2; 
    data_array[2] = 0x80000308;                	 
    dsi_set_cmdq(data_array, 3, 1);	
   
   	data_array[0] = 0x00183902;                          
    data_array[1] = 0x000880b4; 
    data_array[2] = 0x00001032; 
    data_array[3] = 0x00000000;	
    data_array[4] = 0x400a3700;		
    data_array[5] = 0x400a3704;	
    data_array[6] = 0x0a555014;	
    dsi_set_cmdq(data_array, 7, 1);	

   	data_array[0] = 0x001e3902;                          
    data_array[1] = 0x4c0000d5; 
    data_array[2] = 0x00000100; 
    data_array[3] = 0x99006000;	
    data_array[4] = 0x88889988;		
    data_array[5] = 0x88108832;	
    data_array[6] = 0x10548876;	
    data_array[7] = 0x88888832;	
    data_array[8] = 0x00008888;	    
    dsi_set_cmdq(data_array, 9, 1);	
    
   	data_array[0] = 0x00233902;                          
    data_array[1] = 0x181405e0; 
    data_array[2] = 0x203f342d; 
    data_array[3] = 0x0e0e083c;	
    data_array[4] = 0x12101311;		
    data_array[5] = 0x14051c1a;	
    data_array[6] = 0x3f342d18;	
    data_array[7] = 0x0e083c20;	
    data_array[8] = 0x1013110e;	   
    data_array[9] = 0x001c1a12; 
    dsi_set_cmdq(data_array, 10, 1);	   
   
   /////////////////////////////DGC
    data_array[0] = 0x00213902;  
    data_array[1] = 0x080201C1 ;
    data_array[2] = 0x2D272017 ;
    data_array[3] = 0x48403832 ;
    data_array[4] = 0x675F574F ;
    data_array[5] = 0x89817970 ;
    data_array[6] = 0xA8A19991 ;
    data_array[7] = 0xC9C2B9B0 ;
    data_array[8] = 0xE9E2D9D0 ;
    data_array[9] = 0x000000F3 ;
    dsi_set_cmdq(data_array, 10, 1);
    
    data_array[0] = 0x00212902;  
    data_array[1] = 0xD0FFFAC1 ;
    data_array[2] = 0xA9A9DFAF ;
    data_array[3] = 0xC092793D ;
    data_array[4] = 0x20170802 ;
    data_array[5] = 0x38322D27 ;
    data_array[6] = 0x574F4840 ;
    data_array[7] = 0x7970675F ;
    data_array[8] = 0x99918981 ;
    data_array[9] = 0x000000A1 ;
    dsi_set_cmdq(data_array, 10, 1);
    
    data_array[0] = 0x00212902;  
    data_array[1] = 0xB9B0A8C1 ;
    data_array[2] = 0xD9D0C9C2 ;
    data_array[3] = 0xFAF3E9E2 ;
    data_array[4] = 0xDFAFD0FF ;
    data_array[5] = 0x793DA9A9 ;
    data_array[6] = 0x0802C092 ;
    data_array[7] = 0x2D272017 ;
    data_array[8] = 0x48403832 ;
    data_array[9] = 0x0000004F ;
    dsi_set_cmdq(data_array, 10, 1);
    
     data_array[0] = 0x00202902;  
    data_array[1] = 0x675F57C1 ;
    data_array[2] = 0x89817970 ;
    data_array[3] = 0xA8A19991 ;
    data_array[4] = 0xC9C2B9B0 ;
    data_array[5] = 0xE9E2D9D0 ;
    data_array[6] = 0xD0FFFAF3 ;
    data_array[7] = 0xA9A9DFAF ;
    data_array[8] = 0xC092793D ;
    dsi_set_cmdq(data_array, 9, 1);
    //////////////////// DGC  
   
    data_array[0] = 0x00023902;                          
    data_array[1] = 0x000002cc; 
    dsi_set_cmdq(data_array, 2, 1);
    
	data_array[0] = 0x00053902;                          
    data_array[1] = 0x00af00b6;
    data_array[2] = 0x000000af;
    dsi_set_cmdq(data_array, 3, 1);
    
    data_array[0] = 0x00033902;                          
    data_array[1] = 0x000707cb; 
    dsi_set_cmdq(data_array, 2, 1);
    
    data_array[0] = 0x00053902;                          
    data_array[1] = 0xff0000bb;
    data_array[2] = 0x00000080;
    dsi_set_cmdq(data_array, 3, 1);
       
	data_array[0] = 0x00110500; // Sleep Out
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(200);
       //PacketHeader[05 29 00 xx] // Display On 
       //Delay 10ms 
    data_array[0] = 0x00290500; // Sleep Out
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(10);
	  
}

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

#if 	1	
		//sen.luo
		params->dsi.LANE_NUM = LCM_TWO_LANE;
		//The following defined the fomat for data coming from LCD engine.
		params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
		params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
		params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;
		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
		params->dsi.vertical_sync_active = 3;//2;
		params->dsi.vertical_backporch = 9;//9; 
		params->dsi.vertical_frontporch = 9;//15;
		params->dsi.vertical_active_line = FRAME_HEIGHT; 
		params->dsi.horizontal_sync_active = 8;//20;
		params->dsi.horizontal_backporch = 20; //20//46;
		params->dsi.horizontal_frontporch = 22;
		params->dsi.horizontal_active_pixel = FRAME_WIDTH;

      	//end
#else
		// DSI
		/* Command mode setting */
		//1 Three lane or Four lane
		params->dsi.LANE_NUM				= LCM_TWO_LANE;
		//The following defined the fomat for data coming from LCD engine.
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

		// Video mode setting		
		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
		
		params->dsi.vertical_sync_active				= 0x05;// 3    2
		params->dsi.vertical_backporch					= 0x0d;// 20   1
		params->dsi.vertical_frontporch					= 0x08; // 1  12
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 

		params->dsi.horizontal_sync_active				= 0x12;// 50  2
		params->dsi.horizontal_backporch				= 0x5f;
		params->dsi.horizontal_frontporch				= 0x5f;
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;
#endif
	     // params->dsi.LPX=8; 

		// Bit rate calculation
		params->dsi.PLL_CLOCK = 250;
		//1 Every lane speed
	//	params->dsi.pll_div1=0;		// div1=0,1,2,3;div1_real=1,2,4,4 ----0: 546Mbps  1:273Mbps
	//	params->dsi.pll_div2=0;		// div2=0,1,2,3;div1_real=1,2,4,4	
		//params->dsi.fbk_div =9;    // fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)	

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
		MDELAY(20); 
		
		//dsi_set_cmdq_V3(lcm_initialization_setting,sizeof(lcm_initialization_setting)/sizeof(lcm_initialization_setting[0]),1);
              init_lcm_registers();
	#ifdef BUILD_LK
	  printf("[LK]---cmd---hx8389_qhd_dsi_vdo_truly----%s------\n",__func__);
    #else
	  printk("[KERNEL]---cmd---hx8389_qhd_dsi_vdo_truly----%s------\n",__func__);
    #endif	
}



static void lcm_suspend(void)
{
	unsigned int data_array[16];

	//data_array[0]=0x00280500; // Display Off
	//dsi_set_cmdq(data_array, 1, 1);
	
	data_array[0] = 0x00100500; // Sleep In
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);
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
		//lcm_init();
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
		
		//dsi_set_cmdq_V3(lcm_initialization_setting,sizeof(lcm_initialization_setting)/sizeof(lcm_initialization_setting[0]),1);
              init_lcm_registers();
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
    array[1] = 0x009201ba;       	  
    dsi_set_cmdq(array, 2, 1);
	MDELAY(10);

	array[0] = 0x00013700;// return byte number
	dsi_set_cmdq(array, 1, 1);
	MDELAY(10);

	read_reg_v2(0xF4, buffer, 1);
	id  =  buffer[0];

#ifdef BUILD_LK
	printf("[LK]hx8389_qhd_dsi_vdo_truly----%s------[%02x]\n",__func__,buffer[0]);
#else
	printk("[KERNEL]hx8389_qhd_dsi_vdo_truly----%s------[%02x]\n",__func__,buffer[0]);
#endif

	if(LCM_ID_HX8389B == id)
	{
		IMM_GetOneChannelValue(0,data,NULL);
		auxadc0_vol = data[0]*1000 + data[1]*10;
	#ifdef BUILD_LK
		printf("[LK]---cmd---hx8389_qhd_dsi_vdo_truly--adc0 vol %d\n",auxadc0_vol);
	#else
		printk("[KERNEL]---cmd---hx8389_qhd_dsi_vdo_truly--adc0 vol %d\n",auxadc0_vol);
	#endif	
		return ((ADC0_LOW_TRULY <= auxadc0_vol)&&(ADC0_HIGH_TRULY >= auxadc0_vol))?1:0;
	}
	return 0;
}


static unsigned int lcm_esd_check(void)
{
	unsigned int ret=FALSE;
#ifndef BUILD_LK
	char  buffer[6];
	int   array[4];
	char esd;

	array[0] = 0x00023700;
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x0A, buffer, 2);
	esd=buffer[0];
    printk("lcm_esd_check=0x%x",esd);
	if(esd==0x1C)
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
	//lcm_init();
	//lcm_resume();
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
	
	//dsi_set_cmdq_V3(lcm_initialization_setting,sizeof(lcm_initialization_setting)/sizeof(lcm_initialization_setting[0]),1);
    init_lcm_registers();
	return TRUE;
}



LCM_DRIVER hx8389_qhd_dsi_vdo_truly_lcm_drv = 
{
    .name			= "hx8389_qhd_dsi_vdo_truly",
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
