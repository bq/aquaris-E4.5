
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>  /* for mdely */
#include <linux/irqflags.h>  /* for mdely */
//#include <linux/ioport.h> /* for */
//#include <linux/types.h>
//#include <linux/kernel.h> /* for __raw_readl ... */
//#include <mach/board.h>
#include <asm/io.h>        /* __raw_readl */

#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <mach/mt_pm_ldo.h>        /* hwPowerOn */
#include <mach/mt_reg_base.h>
#include "mt_sd.h"
#include "mt_dump.h"
#include <mach/board.h> 
#include <linux/mmc/sd_misc.h>

#ifdef MTK_EMMC_SUPPORT
#include "partition_define.h"
#endif

#ifndef FPGA_PLATFORM
#include <mach/mt_clkmgr.h>
#endif


MODULE_LICENSE("GPL");
/*--------------------------------------------------------------------------*/
/* head file define                                                         */
/*--------------------------------------------------------------------------*/
/* some marco will be reuse with mmc subsystem */

#ifdef MTK_EMMC_SUPPORT
#include "partition_define.h"
#endif
char test_kdump[]={6,5,8,2,'k','d','u','m','p','t','e','s','t'};
//==============
#define TEST_SIZE       (128*1024)
unsigned char g_tst_buf_w[TEST_SIZE] = {0};
unsigned char g_tst_buf_r[TEST_SIZE] = {0};
//==============

//==============
//#define printk(         printk(KERN_EMERG
//maybe use pr_debug
//==============

#define SIMP_SUCCESS          (1)
#define SIMP_FAILED           (0)

/* the base address of sd card slot */
#define BOOT_STORAGE_ID        (0)       
#define EXTEND_STORAGE_ID     (1)       
#define MSDC_CLKSRC      (MSDC_CLKSRC_200M)
static  unsigned int clks[] = {200000000}; 

#define BLK_LEN            (512)
#define MAX_SCLK           (52000000)
#define NORMAL_SCLK        (25000000)
#define MIN_SCLK           (260000)

#if (5 == HOST_MAX_NUM)
static u32 u_msdc_base[HOST_MAX_NUM] = {MSDC_0_BASE, MSDC_1_BASE, MSDC_2_BASE, MSDC_3_BASE, MSDC_4_BASE};
static struct msdc_hw *p_msdc_hw[HOST_MAX_NUM] = {NULL, NULL, NULL, NULL, NULL};
#elif (4 == HOST_MAX_NUM)
static u32 u_msdc_base[HOST_MAX_NUM] = {MSDC_0_BASE, MSDC_1_BASE, MSDC_2_BASE, MSDC_3_BASE};
static struct msdc_hw *p_msdc_hw[HOST_MAX_NUM] = {NULL, NULL, NULL, NULL};
#elif (3 == HOST_MAX_NUM)
static u32 u_msdc_base[HOST_MAX_NUM] = {MSDC_0_BASE, MSDC_1_BASE, MSDC_2_BASE};
static struct msdc_hw *p_msdc_hw[HOST_MAX_NUM] = {NULL, NULL, NULL};
#endif

static struct simp_msdc_host g_msdc_host[2];
static struct simp_msdc_card g_msdc_card[2];
static struct simp_msdc_host *pmsdc_boot_host = &g_msdc_host[BOOT_STORAGE_ID];
static struct simp_msdc_host *pmsdc_extend_host = &g_msdc_host[EXTEND_STORAGE_ID];
static struct simp_mmc_host g_mmc_host[2];
static struct simp_mmc_card g_mmc_card[2];
static struct simp_mmc_host *pmmc_boot_host = &g_mmc_host[BOOT_STORAGE_ID];
static struct simp_mmc_host *pmmc_extend_host = &g_mmc_host[EXTEND_STORAGE_ID];

static void simp_msdc_dump_register(struct simp_msdc_host *host)
{
    unsigned int base = host->base; 
    int i = 0; 
    unsigned int dbg_val1, dbg_val2, dbg_val3, dbg_val4; 
    printk(KERN_EMERG "R[00]=0x%x  R[04]=0x%x  R[08]=0x%x  R[0C]=0x%x  R[10]=0x%x  R[14]=0x%x\n", 
    				sdr_read32(base + 0x00), sdr_read32(base + 0x04), sdr_read32(base + 0x08), sdr_read32(base + 0x0C), sdr_read32(base + 0x10), sdr_read32(base + 0x14));
    printk(KERN_EMERG "R[30]=0x%x  R[34]=0x%x  R[38]=0x%x  R[3C]=0x%x  R[40]=0x%x  R[44]=0x%x\n", 
    				sdr_read32(base + 0x30), sdr_read32(base + 0x34), sdr_read32(base + 0x38), sdr_read32(base + 0x3C), sdr_read32(base + 0x40), sdr_read32(base + 0x44));
    printk(KERN_EMERG "R[48]=0x%x  R[4C]=0x%x  R[50]=0x%x  R[58]=0x%x  R[5C]=0x%x  R[60]=0x%x\n", 
    				sdr_read32(base + 0x48), sdr_read32(base + 0x4C), sdr_read32(base + 0x50), sdr_read32(base + 0x58), sdr_read32(base + 0x5C), sdr_read32(base + 0x60));
    printk(KERN_EMERG "R[70]=0x%x  R[74]=0x%x  R[78]=0x%x  R[7C]=0x%x  R[80]=0x%x  R[84]=0x%x\n", 
    				sdr_read32(base + 0x70), sdr_read32(base + 0x74), sdr_read32(base + 0x78), sdr_read32(base + 0x7C), sdr_read32(base + 0x80), sdr_read32(base + 0x84));      
    printk(KERN_EMERG "R[88]=0x%x  R[90]=0x%x  R[94]=0x%x  R[98]=0x%x  R[9C]=0x%x  R[A0]=0x%x\n", 
    				sdr_read32(base + 0x88), sdr_read32(base + 0x90), sdr_read32(base + 0x94), sdr_read32(base + 0x98), sdr_read32(base + 0x9C), sdr_read32(base + 0xA0));
    printk(KERN_EMERG "R[A4]=0x%x  R[A8]=0x%x  R[B0]=0x%x  R[B4]=0x%x  R[E0]=0x%x  R[E4]=0x%x\n", 
    				sdr_read32(base + 0xA4), sdr_read32(base + 0xA8), sdr_read32(base + 0xB0), sdr_read32(base + 0xB4), sdr_read32(base + 0xE0), sdr_read32(base + 0xE4));
    printk(KERN_EMERG "R[E8]=0x%x  R[EC]=0x%x  R[F0]=0x%x  R[F4]=0x%x  R[F8]=0x%x  R[100]=0x%x  R[104]=0x%x\n", 
    				sdr_read32(base + 0xE8), sdr_read32(base + 0xEC), sdr_read32(base + 0xF0), sdr_read32(base + 0xF4), sdr_read32(base + 0xF8), sdr_read32(base + 0x100), sdr_read32(base + 0x104));  
    i = 0; 
    while(i <= 25){
        *(u32*)(base + 0xa0) = i;
        dbg_val1 = *(u32*)(base + 0xa4); 
        *(u32*)(base + 0xa0) = i + 1;
        dbg_val2 = *(u32*)(base + 0xa4); 
        *(u32*)(base + 0xa0) = i + 2;
        dbg_val3 = *(u32*)(base + 0xa4); 
        *(u32*)(base + 0xa0) = i + 3;
        dbg_val4 = *(u32*)(base + 0xa4); 
        printk(KERN_EMERG "R[a0]=0x%x  R[a4]=0x%x  R[a0]=0x%x  R[a4]=0x%x  R[a0]=0x%x  R[a4]=0x%x  R[a0]=0x%x  R[a4]=0x%x\n", 
         			i, dbg_val1, (i+1), dbg_val2, (i+2), dbg_val3, (i+3), dbg_val4);
        i += 4; 
    }         
} 
static void simp_msdc_dump_info(unsigned int id)
{
	if(id == pmsdc_boot_host->id)
		simp_msdc_dump_register(pmsdc_boot_host);
	if(id == pmsdc_extend_host->id)
		simp_msdc_dump_register(pmsdc_extend_host);
}

//#define PERI_MSDC_SRCSEL   (0xF100000c)
//#define PDN_REG            (0xF1000010)     
static void simp_msdc_config_clksrc(struct simp_msdc_host *host, CLK_SOURCE_T clksrc)
{
    host->clksrc = clksrc;
    host->clk  = clks[clksrc];
}

static void simp_msdc_config_clock(struct simp_msdc_host *host, unsigned int hz)  /* no ddr */
{
    // struct msdc_hw *hw = host->priv;
    u32 base = host->base;
    u32 mode;  /* use divisor or not */
    u32 div = 0;           
    u32 sclk;
    u32 hclk = host->clk;
    u32 orig_clksrc = host->clksrc;
  
    if (hz >= hclk) {
        mode = 0x1; /* no divisor */
        sclk = hclk; 
    } else {
        mode = 0x0; /* use divisor */
        if (hz >= (hclk >> 1)) {
            div  = 0;         /* mean div = 1/2 */
            sclk = hclk >> 1; /* sclk = clk / 2 */
        } else {
            div  = (hclk + ((hz << 2) - 1)) / (hz << 2);
            sclk = (hclk >> 2) / div;
        }
    }
    host->sclk  = sclk;
    //printk(KERN_EMERG "clock<%d>\n",sclk);

    /* set clock mode and divisor */
    //simp_msdc_config_clksrc(host, MSDC_CLKSRC_NONE);

    /* designer said: best way is wait clk stable while modify clk config bit */
	sdr_set_field(MSDC_CFG, MSDC_CFG_CKMOD|MSDC_CFG_CKDIV,(mode << 8)|(div % 0xff));

    simp_msdc_config_clksrc(host, orig_clksrc);
 
    /* wait clock stable */
    while (!(sdr_read32(MSDC_CFG) & MSDC_CFG_CKSTB));
}

static void msdc_set_timeout(struct simp_msdc_host *host, u32 ns, u32 clks)
{
    u32 base = host->base;
    u32 timeout, clk_ns;

    clk_ns  = 1000000000UL / host->sclk;
    timeout = ns / clk_ns + clks;
    timeout = timeout >> 20; /* in 2^20 sclk cycle unit */
    timeout = timeout > 1 ? timeout - 1 : 0;
    timeout = timeout > 255 ? 255 : timeout;

    sdr_set_field(SDC_CFG, SDC_CFG_DTOC, timeout);
}

static unsigned int simp_msdc_ldo_power(unsigned int on, MT65XX_POWER powerId, MT65XX_POWER_VOLTAGE powerVolt)
{
    /* Fixme: must realize access register directly */
	if(on)
		hwPowerOn(powerId, powerVolt, "msdc");
	else
    	hwPowerDown(powerId, "msdc");    
    return SIMP_SUCCESS;          
}

static unsigned int simp_mmc_power_up(struct simp_mmc_host *host,bool on)
{
    switch(host->mtk_host->id){
        case 0:
            /* for emmc, host0 and host4 are mutually exclusive for emmc card */
			simp_msdc_ldo_power(on, MT6323_POWER_LDO_VEMC_3V3, VOL_3300);
            break;
        case 1:
            /* for sd, makesure msdc host volt is the same as mt6589 IP internal LDO volt */
			simp_msdc_ldo_power(on, MT6323_POWER_LDO_VMC, VOL_3300);
			simp_msdc_ldo_power(on, MT6323_POWER_LDO_VMCH, VOL_3300);
            break;
        default:
            break;
        }

    return SIMP_SUCCESS;
}

/* do not change to 1.8v, so cmd11 not used */
static unsigned int simp_mmc_set_signal_voltage(struct simp_mmc_host *host, int volt, bool cmd11)
{
    /* set mmc card voltage */

    return SIMP_SUCCESS;
}

#define clk_readl(addr) \
    DRV_Reg32(addr)

#define clk_setl(addr, val) \
        mt65xx_reg_sync_writel(clk_readl(addr) | (val), addr)

#define clk_clrl(addr, val) \
        mt65xx_reg_sync_writel(clk_readl(addr) & ~(val), addr)

static unsigned int simp_mmc_enable_clk(struct simp_mmc_host *host)
{
    /* step1: open pll */
    clk_setl(0xF020924C, 0x1);
    mdelay(1);

    clk_clrl(0xF020924C, 0x2);
    clk_setl(0xF0209240, 0x1);
    mdelay(1);
    printk(KERN_EMERG "[pll]0xF020924C = 0x%x\n", sdr_read32(0xF020924C)); 
    printk(KERN_EMERG "[pll]0xF0209240 = 0x%x\n", sdr_read32(0xF0209240)); 

    /* step2: enable mux */
    clk_clrl(0xF0000060, 0x80000000); 	
    clk_clrl(0xF0000070, 0x80); 	
    printk(KERN_EMERG "[mux] 0xF0000060 = 0x%x\n", sdr_read32(0xF0000060)); 
    printk(KERN_EMERG "[mux] 0xF0000070 = 0x%x\n", sdr_read32(0xF0000070)); 
    
    /* step3: enable clock */
    clk_setl(0xF0003010, 0x3000); 	
    printk(KERN_EMERG "[clk] 0xF0003010 = 0x%x\n", sdr_read32(0xF0003010)); 
    printk(KERN_EMERG "[clk] 0xF0003018 = 0x%x\n", sdr_read32(0xF0003018)); 

    return SIMP_SUCCESS;
}


static unsigned int simp_mmc_hw_reset_for_init(struct simp_mmc_host *host)
{
    unsigned int  base;
    
    base = host->mtk_host->base;
    if (0 == host->mtk_host->id){
        /* check emmc card support HW Rst_n yes or not is the good way. 
         * but if the card not support it , here just failed. 
         *     if the card support it, Rst_n function enable under DA driver, 
         *     pls see SDMMC_Download_BL_PostProcess_Internal() */
        /* 1ms pluse to trigger emmc enter pre-idle state */
        sdr_set_bits(EMMC_IOCON, EMMC_IOCON_BOOTRST);
        mdelay(1);
        sdr_clr_bits(EMMC_IOCON, EMMC_IOCON_BOOTRST);

        /* clock is need after Rst_n pull high, and the card need 
         * clock to calculate time for tRSCA, tRSTH */
        sdr_set_bits(MSDC_CFG, MSDC_CFG_CKPDN);
        mdelay(1);
        
        /* not to close, enable clock free run under mt_dump */
        //sdr_clr_bits(MSDC_CFG, MSDC_CFG_CKPDNT);
    }

    return SIMP_SUCCESS;
}

#if 0
enum {
    RESP_NONE = 0,
    RESP_R1   = 1,
    RESP_R2   = 2,
    RESP_R3   = 3,
    RESP_R4   = 4,
    RESP_R5   = 5,
    RESP_R6   = 6,
    RESP_R7   = 7,
    RESP_R1B  = 8
};
#endif

static int msdc_rsp[] = {
    0,  /* RESP_NONE */
    1,  /* RESP_R1 */
    2,  /* RESP_R2 */
    3,  /* RESP_R3 */
    4,  /* RESP_R4 */
    1,  /* RESP_R5 */
    1,  /* RESP_R6 */
    1,  /* RESP_R7 */
    7,  /* RESP_R1b */
};

#define msdc_retry(expr, retry, cnt,id) \
    do { \
        int backup = cnt; \
        while (retry) { \
            if (!(expr)) break; \
            if (cnt-- == 0) { \
                retry--; mdelay(1); cnt = backup; \
            } \
        } \
        if (retry == 0) { \
            simp_msdc_dump_info(id); \
        } \
        WARN_ON(retry == 0); \
    } while(0)

#define msdc_reset(id) \
    do { \
        int retry = 3, cnt = 1000; \
        sdr_set_bits(MSDC_CFG, MSDC_CFG_RST); \
        mb(); \
        msdc_retry(sdr_read32(MSDC_CFG) & MSDC_CFG_RST, retry, cnt, id); \
    } while(0)

#define msdc_clr_int() \
    do { \
        volatile u32 val = sdr_read32(MSDC_INT); \
        sdr_write32(MSDC_INT, val); \
    } while(0)

#define msdc_clr_fifo(id) \
    do { \
        int retry = 3, cnt = 1000; \
        sdr_set_bits(MSDC_FIFOCS, MSDC_FIFOCS_CLR); \
        msdc_retry(sdr_read32(MSDC_FIFOCS) & MSDC_FIFOCS_CLR, retry, cnt, id); \
    } while(0)

#define msdc_reset_hw(id) \
        msdc_reset(id); \
        msdc_clr_fifo(id); \
        msdc_clr_int(); 

static unsigned int simp_msdc_init(struct simp_mmc_host *mmc_host)
{
    unsigned int ret = 0;
    unsigned int  base;
    struct simp_msdc_host *host = mmc_host->mtk_host;
    
    //struct msdc_hw *hw;        
    /* Todo1: struct msdc_hw in board.c */
    
    base = host->base;
    
    /* set to SD/MMC mode, the first step while operation msdc */
    sdr_set_field(MSDC_CFG, MSDC_CFG_MODE, 1); // MSDC_SDMMC

    /* reset controller */
    msdc_reset(host->id); 

    /* clear FIFO */
    msdc_clr_fifo(host->id);     

    /* Disable & Clear all interrupts */    
    msdc_clr_int();  
    sdr_write32(MSDC_INTEN, 0);

    /* reset tuning parameter */
    //sdr_write32(MSDC_PAD_CTL0,   0x0090000);
    //sdr_write32(MSDC_PAD_CTL1,   0x00A0000);
    //sdr_write32(MSDC_PAD_CTL2,   0x00A0000);
    sdr_write32(MSDC_PAD_TUNE,   0x00000000);
    sdr_write32(MSDC_DAT_RDDLY0, 0x00000000);
    sdr_write32(MSDC_DAT_RDDLY1, 0x00000000);
    sdr_write32(MSDC_IOCON,      0x00000000);
    sdr_write32(MSDC_PATCH_BIT,  0x003C000F);
    
    /* PIO mode */    
    sdr_set_bits(MSDC_CFG, MSDC_CFG_PIO);

    /* sdio + inswkup*/
    sdr_set_bits(SDC_CFG, SDC_CFG_SDIO);
    sdr_set_bits(SDC_CFG, SDC_CFG_INSWKUP);

    switch(host->id){
        case 0: //pull up 10K
            sdr_set_field(MSDC0_GPIO_CMD_BASE, GPIO_R0_MASK, 1);
            sdr_set_field(MSDC0_GPIO_CMD_BASE, GPIO_R1_MASK, 0);
            sdr_set_field(MSDC0_GPIO_CMD_BASE, GPIO_PUPD_MASK, 0);
                    
            sdr_set_field(MSDC0_GPIO_DAT_BASE, GPIO_R0_MASK, 1);
            sdr_set_field(MSDC0_GPIO_DAT_BASE, GPIO_R1_MASK, 0);
            sdr_set_field(MSDC0_GPIO_DAT_BASE, GPIO_PUPD_MASK, 0);
                   
            /* clock pull down with 50k during card init */
            sdr_set_field(MSDC0_GPIO_CLK_BASE, GPIO_R0_MASK, 0);
            sdr_set_field(MSDC0_GPIO_CLK_BASE, GPIO_R1_MASK, 1);
            sdr_set_field(MSDC0_GPIO_CLK_BASE, GPIO_PUPD_MASK, 1);
            break;
        case 1: //pull up 50K
            sdr_set_field(MSDC1_GPIO_CMD_BASE, GPIO_PU_MASK, 1);
            sdr_set_field(MSDC1_GPIO_CMD_BASE, GPIO_PD_MASK, 0);
                
            sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT0_PU_MASK, 1);
            sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT0_PD_MASK, 0);

            sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT1_PU_MASK, 1);
            sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT1_PD_MASK, 0);
                
            sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT2_PU_MASK, 1);
            sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT2_PD_MASK, 0);
                
            sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT3_PU_MASK, 1);
            sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT3_PD_MASK, 0);
                
            /* clock pull down during card init */
            sdr_set_field(MSDC1_GPIO_CLK_BASE, GPIO_PU_MASK, 0);
            sdr_set_field(MSDC1_GPIO_CLK_BASE, GPIO_PD_MASK, 1);
            break;

        default: 
            break; 
    }

	switch (host->id) {
		case 0:
            sdr_set_field(MSDC0_GPIO_CLK_BASE, GPIO_SMT_MASK, 1);
            sdr_set_field(MSDC0_GPIO_CMD_BASE, GPIO_SMT_MASK, 1);
            sdr_set_field(MSDC0_GPIO_DAT_BASE, GPIO_SMT_MASK, 1); 
			break;
		case 1:
            sdr_set_field(MSDC1_GPIO_CLK_BASE, GPIO_SMT_MASK, 1);
            sdr_set_field(MSDC1_GPIO_CMD_BASE, GPIO_SMT_MASK, 1);
            sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_SMT_MASK, 1); 
			break;
		default:
			break;
	}
	
    switch (host->id) {
		case 0:
			sdr_set_field(MSDC0_GPIO_CLK_BASE, GPIO_MSDC0_DRVN, 5); /* feifei.wang -- not use hardcode here */
			sdr_set_field(MSDC0_GPIO_CMD_BASE, GPIO_MSDC0_DRVN, 5);
			sdr_set_field(MSDC0_GPIO_DAT_BASE, GPIO_MSDC0_DRVN, 5);
			break;
        case 1:
            sdr_set_field(MSDC1_GPIO_CLK_BASE, GPIO_MSDC1_MSDC2_DRVN, 5); /* feifei.wang -- not use hardcode here */
            sdr_set_field(MSDC1_GPIO_CMD_BASE, GPIO_MSDC1_MSDC2_DRVN, 5);
            sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_MSDC1_MSDC2_DRVN, 5);
			break;
		default:
			break;
		}

    /* set sampling edge */
    sdr_set_field(MSDC_IOCON, MSDC_IOCON_RSPL, 0); // rising: 0
    sdr_set_field(MSDC_IOCON, MSDC_IOCON_DSPL, 0);

    /* write crc timeout detection */
    sdr_set_field(MSDC_PATCH_BIT, 1 << 30, 1);

    /* Clock source select*/
    simp_msdc_config_clksrc(host, host->clksrc);
    
    /* Bus width to 1 bit*/
    sdr_set_field(SDC_CFG, SDC_CFG_BUSWIDTH, 0);

    /* make sure the clock is 260K */
    simp_msdc_config_clock(host, MIN_SCLK);
        
    /* Set Timeout 100ms*/ 
    msdc_set_timeout(host, 100000000, 0); 
        
    sdr_clr_bits(MSDC_PS, MSDC_PS_CDEN); 
    
    /* detect card */
    /* need to check card is insert [Fix me] */
    
    /* check write protection [Fix me] */

#if 0
    /* simple test for clk output */
    sdr_write32(MSDC_PATCH_BIT, 0xF3F);
    sdr_write32(MSDC_CFG, 0x10001013);
    sdr_write32(SDC_CFG, 0x0);
    sdr_write32(SDC_CMD, 0x0);
    sdr_write32(SDC_ARG, 0x0);
    
    /* dump register for debug */
    simp_msdc_dump_register(host);
#endif

    
    return ret;                                          
}


static void simp_mmc_set_bus_mode(struct simp_mmc_host *host, unsigned int mode)
{
    /* mtk: msdc not support to modify bus mode */
    
}

//=======================something for msdc cmd/data
#define CMD_WAIT_RETRY  (0x8FFFFFFF)
#define sdc_is_busy()          (sdr_read32(SDC_STS) & SDC_STS_SDCBUSY)
#define sdc_is_cmd_busy()      (sdr_read32(SDC_STS) & SDC_STS_CMDBUSY)

#define sdc_send_cmd(cmd,arg) \
    do { \
        sdr_write32(SDC_ARG, (arg)); \
        sdr_write32(SDC_CMD, (cmd)); \
    } while(0)

int simp_offset = 0;
u8 simp_ext_csd[512];
static int simp_msdc_cmd(struct simp_msdc_host *host, unsigned int cmd, unsigned int raw, 
                                         unsigned int arg, int rsptyp, unsigned int *resp)
{
    int retry = 5000; //CMD_WAIT_RETRY; 
    unsigned int base   = host->base;      
    unsigned int error = 0 ;     
    unsigned int intsts = 0; 
    unsigned int cmdsts = MSDC_INT_CMDRDY | MSDC_INT_CMDTMO | MSDC_INT_RSPCRCERR;
        
    /* wait before send command */ 
    if (cmd == CMD13) {
        while (retry--) {
            if (!sdc_is_cmd_busy()) 
                break;     
            mdelay(1);    
        }
        if (retry == 0) {
            error = 1;            
            goto end;
        }
    } else {
        while (retry--) {
            if (!sdc_is_busy())
                break;      
            mdelay(1);         
        }    
        if (retry == 0) {
            error = 2;            
            goto end;
        }        
    }  

    if ((CMD17 == cmd || CMD18 == cmd || 
        CMD24 == cmd || CMD25 == cmd) && (host->card->type == MMC_TYPE_MMC))
        arg += simp_offset;

    sdc_send_cmd(raw, arg);

    /* polling to check the interrupt */
    retry = 5000; //CMD_WAIT_RETRY; 
    while( (intsts & cmdsts) == 0) {     
        intsts = sdr_read32(MSDC_INT); 
        retry--;     
        if (retry == 0) {
            error = 3; 
            goto end;     
        }
        mdelay(1);  
    }      

    intsts &= cmdsts ; 
    sdr_write32(MSDC_INT, intsts); /* clear interrupts */    

    if (intsts & MSDC_INT_CMDRDY) {
		#if MTK_MMC_DUMP_DBG
        printk(KERN_EMERG "msdc cmd<%d> arg<0x%x> Ready \r\n", cmd,arg);
		#endif
        /* get the response */
        switch (rsptyp) {
        case RESP_NONE:
            break;                      
        case RESP_R2: 
            *resp++ = sdr_read32(SDC_RESP3); 
            *resp++ = sdr_read32(SDC_RESP2); 
            *resp++ = sdr_read32(SDC_RESP1); 
            *resp   = sdr_read32(SDC_RESP0); 
            break; 
        default: /* Response types 1, 3, 4, 5, 6, 7(1b) */
            *resp = sdr_read32(SDC_RESP0);           
        }           
    } else {
        printk(KERN_EMERG "msdc%d cmd<%d> raw<0x%x> arg<0x%x> error(0x%x)\n", host->id, cmd, raw, arg,intsts);
        error = 4;  
        goto end;            
    } 

    if (rsptyp == RESP_R1B) {
        retry = 9999;  
        while ((sdr_read32(MSDC_PS) & MSDC_PS_DAT0) != MSDC_PS_DAT0){
            retry--;   
            if(retry %5000 == 0){
                printk("int cmd=0x%x, arg=0x%x, retry=0x%x, intsts=0x%x\n", raw, arg, retry, intsts); 
                simp_msdc_dump_info(host->id); 
            }
            if (retry == 0) {
                error = 5; 
                goto end;     
            }
            mdelay(1); 
        }
		#if MTK_MMC_DUMP_DBG
		printk(KERN_EMERG "msdc cmd<%d> done \r\n", cmd);
		#endif
    }
    
end:    
    if(error){
       printk(KERN_EMERG "cmd:%d,arg:0x%x,error=%d,intsts=0x%x\n", cmd, arg, error, intsts);
       simp_msdc_dump_info(host->id); 
    }
    return error;                   
}

//=======================

static int simp_mmc_go_idle(struct simp_mmc_host *host)
{
    int err = 0;
    unsigned int resp = 0; 
    struct simp_msdc_host *phost = host->mtk_host;

    printk(KERN_EMERG "send cmd0========\n");
    err = simp_msdc_cmd(phost, CMD0, CMD0_RAW, CMD0_ARG, RESP_NONE, &resp);
    if (err){
        printk(KERN_EMERG "cmd0: error(0x%d)\n", err);
    }

    return err;
}

static int simp_mmc_send_op_cond(struct simp_mmc_host *host, unsigned int ocr, unsigned int *rocr)
{
    int err = 0, i;
    unsigned int resp = 0; 
    struct simp_msdc_host *phost = host->mtk_host;

    for (i = 500; i; i--) {
        err = simp_msdc_cmd(phost, CMD1, CMD1_RAW, ocr, RESP_R3, &resp);
        if (err){
            printk(KERN_EMERG "cmd1: error(0x%d)\n", err);
            break;
        }

        /* if we're just probing, do a single pass */
        if (ocr == 0)
            break;

        /* otherwise wait until reset completes */
        if (resp & MMC_CARD_BUSY)
            break;

        err = -ETIMEDOUT;

        mdelay(10);
    }
    
    if (rocr)
        *rocr = resp;

    printk(KERN_EMERG "cmd1: resp(0x%x), i=%d\n", resp, i);

    return err;
}

static int simp_mmc_all_send_cid(struct simp_mmc_host *host, unsigned int *cid)
{
    int err = 0;
    unsigned int resp[4] = {0}; 
    struct simp_msdc_host *phost = host->mtk_host;
    
    err = simp_msdc_cmd(phost, CMD2, CMD2_RAW, 0, RESP_R2, resp);
    if (err){
        printk(KERN_EMERG "cmd2: error(0x%d)\n", err);
    }

    printk(KERN_EMERG "resp: 0x%x 0x%x 0x%x 0x%x\n", resp[0], resp[1], resp[2], resp[3]);
    
    memcpy(cid, resp, sizeof(u32) * 4);

    return 0;
}

static int simp_mmc_set_relative_addr(struct simp_mmc_card *card)
{
    int err;
    unsigned int resp; 
    struct simp_msdc_host *phost = card->host->mtk_host;

    err = simp_msdc_cmd(phost, CMD3, CMD3_RAW, card->rca << 16, RESP_R1, &resp);
    if (err){
        printk(KERN_EMERG "cmd3: error(0x%d)\n", err);
    }

    return err;
}

static int simp_mmc_send_csd(struct simp_mmc_card *card, unsigned int *csd)
{
    int err;
    unsigned int resp[4] = {0}; 
    struct simp_msdc_host *phost = card->host->mtk_host;

    err = simp_msdc_cmd(phost, CMD9, CMD9_RAW, card->rca << 16, RESP_R2, resp);
    if (err){
        printk(KERN_EMERG "cmd9: error(0x%d)\n", err);
    }

    memcpy(csd, resp, sizeof(u32) * 4);

    return err;
}
static const unsigned int tran_exp[] = {
	10000,		100000,		1000000,	10000000,
	0,		0,		0,		0
};

static const unsigned char tran_mant[] = {
	0,	10,	12,	13,	15,	20,	25,	30,
	35,	40,	45,	50,	55,	60,	70,	80,
};

static const unsigned int tacc_exp[] = {
		1,	10, 100,	1000,	10000,	100000, 1000000, 10000000,
	};
		
static const unsigned int tacc_mant[] = {
			0,	10, 12, 13, 15, 20, 25, 30,
			35, 40, 45, 50, 55, 60, 70, 80,
	};

#define UNSTUFF_BITS(resp,start,size)					\
	({								\
		const int __size = size;				\
		const u32 __mask = (__size < 32 ? 1 << __size : 0) - 1;	\
		const int __off = 3 - ((start) / 32);			\
		const int __shft = (start) & 31;			\
		u32 __res;						\
									\
		__res = resp[__off] >> __shft;				\
		if (__size + __shft > 32)				\
			__res |= resp[__off-1] << ((32 - __shft) % 32);	\
		__res & __mask;						\
	})
	

static int simp_mmc_decode_csd(struct simp_mmc_card *card)
{
	struct mmc_csd *csd = &card->csd;
	unsigned int e, m, a, b;
	u32 *resp = card->raw_csd;

	/*
	 * We only understand CSD structure v1.1 and v1.2.
	 * v1.2 has extra information in bits 15, 11 and 10.
	 * We also support eMMC v4.4 & v4.41.
	 */
	csd->structure = UNSTUFF_BITS(resp, 126, 2);
	if (csd->structure == 0) {
		printk(KERN_EMERG "unrecognised CSD structure version %d\n",
			csd->structure);
		return -EINVAL;
	}

	csd->mmca_vsn	 = UNSTUFF_BITS(resp, 122, 4);
	m = UNSTUFF_BITS(resp, 115, 4);
	e = UNSTUFF_BITS(resp, 112, 3);
	csd->tacc_ns	 = (tacc_exp[e] * tacc_mant[m] + 9) / 10;
	csd->tacc_clks	 = UNSTUFF_BITS(resp, 104, 8) * 100;

	m = UNSTUFF_BITS(resp, 99, 4);
	e = UNSTUFF_BITS(resp, 96, 3);
	csd->max_dtr	  = tran_exp[e] * tran_mant[m];
	csd->cmdclass	  = UNSTUFF_BITS(resp, 84, 12);

	e = UNSTUFF_BITS(resp, 47, 3);
	m = UNSTUFF_BITS(resp, 62, 12);
	csd->capacity	  = (1 + m) << (e + 2);

	csd->read_blkbits = UNSTUFF_BITS(resp, 80, 4);
	csd->read_partial = UNSTUFF_BITS(resp, 79, 1);
	csd->write_misalign = UNSTUFF_BITS(resp, 78, 1);
	csd->read_misalign = UNSTUFF_BITS(resp, 77, 1);
	csd->r2w_factor = UNSTUFF_BITS(resp, 26, 3);
	csd->write_blkbits = UNSTUFF_BITS(resp, 22, 4);
	csd->write_partial = UNSTUFF_BITS(resp, 21, 1);

	if (csd->write_blkbits >= 9) {
		a = UNSTUFF_BITS(resp, 42, 5);
		b = UNSTUFF_BITS(resp, 37, 5);
		csd->erase_size = (a + 1) * (b + 1);
		csd->erase_size <<= csd->write_blkbits - 9;
	}

	return 0;
}

static int simp_mmc_select_card(struct simp_mmc_host *host, struct simp_mmc_card *card)
{
    int err;
    unsigned int resp; 
    struct simp_msdc_host *phost = host->mtk_host;

    err = simp_msdc_cmd(phost, CMD7, CMD7_RAW, card->rca << 16, RESP_R1, &resp);
    if (err){
        printk(KERN_EMERG "cmd7: select card error(0x%d)\n", err);
    }

    return 0;
}

/*
 * Mask off any voltages we don't support and select
 * the lowest voltage
 */
static unsigned int simp_mmc_select_voltage(struct simp_mmc_host *host, unsigned int ocr)
{
#if 0

    int bit;


    ocr &= host->ocr_avail;

    bit = ffs(ocr);
    if (bit) {
        bit -= 1;

        ocr &= 3 << bit;

        mmc_host_clk_hold(host);
        host->ios.vdd = bit;
        mmc_set_ios(host);
        mmc_host_clk_release(host);
    } else {
        pr_warning("%s: host doesn't support card's voltages\n",
                mmc_hostname(host));
        ocr = 0;
    }
#endif

    return ocr;
}
#define EXT_CSD_BOOT_SIZE_MULT          226 /* R */
#define EXT_CSD_RPMB_SIZE_MULT          168 /* R */
#define EXT_CSD_GP1_SIZE_MULT           143 /* R/W 3 bytes */
#define EXT_CSD_GP2_SIZE_MULT           146 /* R/W 3 bytes */
#define EXT_CSD_GP3_SIZE_MULT           149 /* R/W 3 bytes */
#define EXT_CSD_GP4_SIZE_MULT           152 /* R/W 3 bytes */
#define EXT_CSD_PART_CFG                179 /* R/W/E & R/W/E_P */
#define EXT_CSD_SEC_CNT					212
#define CAPACITY_2G						(2 * 1024 * 1024 * 1024ULL)
#ifdef MTK_EMMC_SUPPORT
static u64 simp_msdc_get_user_capacity(struct simp_mmc_card *card)
{
	u64 device_capacity = 0;
    u32 device_legacy_capacity = 0;
		if(card->csd.read_blkbits)
				device_legacy_capacity = card->csd.capacity * (2 << (card->csd.read_blkbits - 1));
		else{
				device_legacy_capacity = card->csd.capacity;
				printk(KERN_EMERG "XXX read_blkbits = 0 XXX\n");
			}
		device_capacity = (u64)(card->ext_csd.sectors)* 512 > device_legacy_capacity ? (u64)(card->ext_csd.sectors)* 512 : device_legacy_capacity;
	
	
	return device_capacity;
}
#endif

static void simp_emmc_cal_offset(struct simp_mmc_card *card)
{
#ifdef MTK_EMMC_SUPPORT
	u64 device_capacity = 0;
	simp_offset = MBR_START_ADDRESS_BYTE - (simp_ext_csd[EXT_CSD_BOOT_SIZE_MULT]* 128 * 1024
                + simp_ext_csd[EXT_CSD_BOOT_SIZE_MULT] * 128 * 1024
                + simp_ext_csd[EXT_CSD_RPMB_SIZE_MULT] * 128 * 1024
                + simp_ext_csd[EXT_CSD_GP1_SIZE_MULT + 2] * 256 * 256 
                + simp_ext_csd[EXT_CSD_GP1_SIZE_MULT + 1] * 256 
                + simp_ext_csd[EXT_CSD_GP1_SIZE_MULT + 0]
                + simp_ext_csd[EXT_CSD_GP2_SIZE_MULT + 2] * 256 * 256 
                + simp_ext_csd[EXT_CSD_GP2_SIZE_MULT + 1] * 256 
                + simp_ext_csd[EXT_CSD_GP2_SIZE_MULT + 0]
                + simp_ext_csd[EXT_CSD_GP3_SIZE_MULT + 2] * 256 * 256 
                + simp_ext_csd[EXT_CSD_GP3_SIZE_MULT + 1] * 256 
                + simp_ext_csd[EXT_CSD_GP3_SIZE_MULT + 0]
                + simp_ext_csd[EXT_CSD_GP4_SIZE_MULT + 2] * 256 * 256 
                + simp_ext_csd[EXT_CSD_GP4_SIZE_MULT + 1] * 256 
                + simp_ext_csd[EXT_CSD_GP4_SIZE_MULT + 0]);
	if(simp_offset < 0){
		printk(KERN_EMERG "cal offset error(0x%d)\n", simp_offset);
	}
	device_capacity = simp_msdc_get_user_capacity(card);
	if(device_capacity > CAPACITY_2G)
			simp_offset /= 512;
	printk(KERN_EMERG "emmc offset (0x%x)\n", simp_offset);

#endif /* end of MTK_EMMC_SUPPORT */
}
static int simp_msdc_pio_read(struct simp_msdc_host *host, unsigned int *ptr, unsigned int size);
static void simp_msdc_set_blknum(struct simp_msdc_host *host, unsigned int blknum);

static int simp_mmc_read_ext_csd(struct simp_mmc_host *host, struct simp_mmc_card *card)
{
    int err = 0;
	unsigned int resp; 
	struct simp_msdc_host *phost = host->mtk_host;
	u32 base = phost->base;
	memset(simp_ext_csd, 0, 512);
    if (card->csd.mmca_vsn < CSD_SPEC_VER_4) {
        printk(KERN_EMERG "MSDC MMCA_VSN: %d. Skip EXT_CSD\n", card->csd.mmca_vsn);
        return 0;
    }    	
	msdc_clr_fifo(host->mtk_host->id);
	simp_msdc_set_blknum(phost, 1);
    msdc_set_timeout(phost, 100000000, 0);
	
    err = simp_msdc_cmd(phost, CMD8, CMD8_RAW_EMMC, 0, RESP_R1, &resp);
    if (err){
        printk(KERN_EMERG "cmd8: send cmd to read ext csd error(0x%d)\n", err);
		goto out;
		}
	
    err = simp_msdc_pio_read(phost, (unsigned int *)(simp_ext_csd), 512);  
    if (err){
        printk(KERN_EMERG "pio read ext csd error(0x%d)\n", err);
		goto out;
    }
	
out:
    return err;
}
static void simp_mmc_decode_ext_csd(struct simp_mmc_card *card)
{
    card->ext_csd.sectors =
       	simp_ext_csd[EXT_CSD_SEC_CNT + 0] << 0 |
    	simp_ext_csd[EXT_CSD_SEC_CNT + 1] << 8 |
    	simp_ext_csd[EXT_CSD_SEC_CNT + 2] << 16 |
    	simp_ext_csd[EXT_CSD_SEC_CNT + 3] << 24;
    return;
}

static int simp_emmc_switch_bus(struct simp_mmc_host *host,struct simp_mmc_card *card)
{
	struct simp_msdc_host *phost = host->mtk_host;
	unsigned int resp;
	return simp_msdc_cmd(phost, ACMD6, ACMD6_RAW_EMMC, ACMD6_ARG_EMMC, RESP_R1B, &resp);
}
static int simp_mmc_init_card(struct simp_mmc_host *host, unsigned int ocr,
    struct simp_mmc_card *oldcard)
{
    int err = 0;
    unsigned int rocr;
    unsigned int cid[4];
	u32 base;
    struct simp_mmc_card* card = host->card;
	base = host->mtk_host->base;

    /* Set correct bus mode for MMC before attempting init */
    simp_mmc_set_bus_mode(host, MMC_BUSMODE_OPENDRAIN);  // NULL func now

    /* Initialization should be done at 3.3 V I/O voltage. */
    simp_mmc_set_signal_voltage(host, MMC_SIGNAL_VOLTAGE_330, 0); // NULL func now

    /*
     * Since we're changing the OCR value, we seem to
     * need to tell some cards to go back to the idle
     * state.  We wait 1ms to give cards time to
     * respond.
     * mmc_go_idle is needed for eMMC that are asleep
     */
    simp_mmc_go_idle(host);

    /* The extra bit indicates that we support high capacity */
    err = simp_mmc_send_op_cond(host, ocr | (1 << 30), &rocr);
    if (err)
        goto err;

    err = simp_mmc_all_send_cid(host, cid);
    if (err)
        goto err;

    card->type = MMC_TYPE_MMC;
    card->rca = 1;
	host->mtk_host->card->rca = 1;
    memcpy(card->raw_cid, cid, sizeof(card->raw_cid));

    /*
     * For native busses:  set card RCA and quit open drain mode.
     */
    err = simp_mmc_set_relative_addr(card);
    if (err)
        goto err;

    simp_mmc_set_bus_mode(host, MMC_BUSMODE_PUSHPULL);

    /*
     * Fetch CSD from card.
     */
    err = simp_mmc_send_csd(card, card->raw_csd);
    if (err)
        goto err;

	err = simp_mmc_decode_csd(card);
    if (err)
        goto err;

#if 0
    err = mmc_decode_csd(card);
    if (err)
        goto err;
    err = mmc_decode_cid(card);
    if (err)
        goto err;
#endif

    err = simp_mmc_select_card(host, card);
    if (err)
        goto err;
	err = simp_mmc_read_ext_csd(host,card);
	if (err)
			goto err;
	simp_mmc_decode_ext_csd(card);
	simp_emmc_cal_offset(card);
	if(simp_offset < 0)
		goto err;
	err = simp_emmc_switch_bus(host,card);
	sdr_set_field(SDC_CFG, SDC_CFG_BUSWIDTH, 1);  /* 1: 4 bits mode */  
	simp_msdc_config_clock(host->mtk_host, NORMAL_SCLK);
    return SIMP_SUCCESS;

err:
    return SIMP_FAILED;
}

#define ACMD41_RETRY   (20)
static int simp_mmc_sd_init(struct simp_mmc_host *host)
{
    struct simp_msdc_host *phost = host->mtk_host;
    u32 ACMD41_ARG = 0;     
    u8  retry; 
    u32 base;
    unsigned int resp = 0;
    int bRet = 0;
        
    base = phost->base;
    if (simp_msdc_cmd(phost, CMD0, CMD0_RAW, CMD0_ARG, RESP_NONE, &resp)) goto EXIT;

    if (simp_msdc_cmd(phost, CMD8, CMD8_RAW, CMD8_ARG, RESP_R7,   &resp)){
		// SD v1.0 will not repsonse to CMD8, then clr HCS bit
        printk("SD v1.0, clr HCS bit\n");
    	ACMD41_ARG = ACMD41_ARG_10;             
    } else if (resp == CMD8_ARG) {
    	printk("SD v2.0, set HCS bit\n");
        ACMD41_ARG = ACMD41_ARG_20;       
    }
    

    retry = ACMD41_RETRY; 
    while (retry--) {
        if (simp_msdc_cmd(phost,  CMD55,  CMD55_RAW,  CMD55_ARG << 16,  RESP_R1, &resp)) goto EXIT;                
        if (simp_msdc_cmd(phost, ACMD41, ACMD41_RAW, ACMD41_ARG,  RESP_R3, &resp)) goto EXIT;
        if (resp & R3_OCR_POWER_UP_BIT) {
            phost->card->card_cap = ((resp & R3_OCR_CARD_CAPACITY_BIT) ? high_capacity : standard_capacity);
            if(phost->card->card_cap == standard_capacity){ 
                printk("just standard_capacity card!!\r\n");    
            }
            break;                              
        }
        mdelay(1000 / ACMD41_RETRY);        
    }

    if (simp_msdc_cmd(phost,  CMD2,    CMD2_RAW,   CMD2_ARG, RESP_R2, &resp)) goto EXIT;    

    if (simp_msdc_cmd(phost,  CMD3,    CMD3_RAW,   CMD3_ARG, RESP_R6, &resp)) goto EXIT;
    
    /* save the rca */    
    phost->card->rca = (resp & 0xffff0000) >> 16;     /* RCA[31:16]*/

    if (simp_msdc_cmd(phost,  CMD9,    CMD9_RAW,   CMD9_ARG << 16, RESP_R2, &resp)) goto EXIT;    
            
    if (simp_msdc_cmd(phost,  CMD13,  CMD13_RAW,  CMD13_ARG << 16, RESP_R1, &resp)) goto EXIT;            

    //printk(KERN_EMERG "cmd7 raw: 0x%x, cmd 7 arg: 0x%x, reps type: 0x%x\n", CMD7_RAW,   CMD7_ARG, RESP_R1);
    if (simp_msdc_cmd(phost,  CMD7,    CMD7_RAW,   CMD7_ARG << 16, RESP_R1, &resp)) goto EXIT;            
    
    /* dump register for debug */
    //simp_msdc_dump_register(phost);
    
    mdelay(10);     

    if (simp_msdc_cmd(phost,  CMD55,  CMD55_RAW,  CMD55_ARG << 16, RESP_R1, &resp)) goto EXIT;    

    if (simp_msdc_cmd(phost, ACMD42, ACMD42_RAW, ACMD42_ARG, RESP_R1, &resp)) goto EXIT;    
    
    if (simp_msdc_cmd(phost,  CMD55,  CMD55_RAW,  CMD55_ARG << 16, RESP_R1, &resp)) goto EXIT;    
    
    if (simp_msdc_cmd(phost, ACMD6,   ACMD6_RAW,  ACMD6_ARG, RESP_R1, &resp)) goto EXIT;
    
    /* set host bus width to 4 */        
    sdr_set_field(SDC_CFG, SDC_CFG_BUSWIDTH, 1);  /* 1: 4 bits mode */     
	simp_msdc_config_clock(phost, NORMAL_SCLK);

    printk(KERN_EMERG "sd card inited\n");
        
    bRet = 1;

EXIT:
    return bRet;        
}


static unsigned int simple_mmc_attach_sd(struct simp_mmc_host *host)
{
    //int err = SIMP_FAILED;

    /* power up host */
	simp_mmc_power_up(host,0);
	mdelay(20);
    simp_mmc_power_up(host,1);

    /* enable clock */
    simp_mmc_enable_clk(host);

    /*
     * Some eMMCs (with VCCQ always on) may not be reset after power up, so
     * do a hardware reset if possible.
     */
    simp_mmc_hw_reset_for_init(host);

    /* power up card: Initialization should be done at 3.3 V I/O voltage. */
    simp_mmc_set_signal_voltage(host, MMC_SIGNAL_VOLTAGE_330, 0);
    
    /* init msdc host */
    simp_msdc_init(host);

    simp_mmc_sd_init(host);

    return SIMP_SUCCESS;
}

/* make clk & power always on */
static unsigned int simple_mmc_attach_mmc(struct simp_mmc_host *host)
{
    int err = 0;
    unsigned int ocr;

    /* power up host */
//	simp_mmc_power_up(host,0);
//	mdelay(20);
    simp_mmc_power_up(host,1);
	mdelay(10);

    /*
     * Some eMMCs (with VCCQ always on) may not be reset after power up, so
     * do a hardware reset if possible.
     */
    simp_mmc_hw_reset_for_init(host);

    /* power up card: Initialization should be done at 3.3 V I/O voltage. */
    simp_mmc_set_signal_voltage(host, MMC_SIGNAL_VOLTAGE_330, 0);

    /* enable clock */
    simp_mmc_enable_clk(host);

    /* init msdc host */
    simp_msdc_init(host);

    /*=================== begin to init emmc card =======================*/
    
    /* Set correct bus mode for MMC before attempting attach */
    simp_mmc_set_bus_mode(host, MMC_BUSMODE_OPENDRAIN);
    
    simp_mmc_go_idle(host);

    err = simp_mmc_send_op_cond(host, 0, &ocr);

    //=========ok

    /*
     * Sanity check the voltages that the card claims to
     * support.
     */
    if (ocr & 0x7F) {
        printk(KERN_EMERG "msdc0: card claims to support voltages "
               "below the defined range. These will be ignored.\n");
        ocr &= ~0x7F;
    }

    host->ocr = simp_mmc_select_voltage(host, ocr);

    /*
     * Can we support the voltage of the card?
     */
    if (!host->ocr) {
        printk(KERN_EMERG "msdc0: card voltage not support\n");
        err = -EINVAL;
        goto err;
    }

    /*
     * Detect and init the card.
     */
    err = simp_mmc_init_card(host, host->ocr, NULL);
    if (err == SIMP_FAILED){
        printk(KERN_EMERG "init eMMC failed\n");
        goto err;
    }
	printk(KERN_EMERG "init eMMC success\n");

    /*=================== end mmc card init =============================*/
    return SIMP_SUCCESS;
err:
    return SIMP_FAILED;
}

/* not use freq para */
static unsigned int simp_mmc_rescan_try_freq(struct simp_mmc_host *host, unsigned freq)
{
    int err = SIMP_FAILED; 

    /* sd/emmc will support */
    if (host->mtk_host->card->type == MMC_TYPE_MMC){
		printk(KERN_EMERG "init emmc for ipanic dump\n");
        err = simple_mmc_attach_mmc(host);
    } else if(host->mtk_host->card->type == MMC_TYPE_SD){
        printk(KERN_EMERG "init sd card\n");
        err = simple_mmc_attach_sd(host);
    }

    return err;
}

#define MSDC_FIFO_SZ            (128)
#define MSDC_FIFO_THD           (64)  // (128)
#define msdc_txfifocnt()   ((sdr_read32(MSDC_FIFOCS) & MSDC_FIFOCS_TXCNT) >> 16)
#define msdc_rxfifocnt()   ((sdr_read32(MSDC_FIFOCS) & MSDC_FIFOCS_RXCNT) >> 0)
#define msdc_fifo_write32(v)   sdr_write32(MSDC_TXDATA, (v))
#define msdc_fifo_write8(v)    sdr_write8(MSDC_TXDATA, (v))
#define msdc_fifo_read32()     sdr_read32(MSDC_RXDATA)
#define msdc_fifo_read8()    sdr_read8(MSDC_RXDATA)  
static int simp_msdc_pio_write(struct simp_msdc_host *host, unsigned int *ptr, unsigned int size)
{
    unsigned int  base = host->base;    
    unsigned int  left = size; 
	unsigned int  status = 0;
    unsigned char *u8ptr;
	int l_count = 0;
	int err = 0;
	
	while(1){
		status = sdr_read32(MSDC_INT);
       	sdr_write32(MSDC_INT,status);
        if (status & MSDC_INT_DATCRCERR) {
            printk(KERN_ERR "[MSDC%d] DAT CRC error (0x%x), Left DAT: %d bytes\n", 
                host->id, status, left);
            err = -5;
            break;
        } else if (status & MSDC_INT_DATTMO) {
            printk("[MSDC%d] DAT TMO error (0x%x), Left DAT: %d bytes\n", 
                host->id, status, left);
            err = -110;
            break;
			}
		else if(status & MSDC_INT_XFER_COMPL){
			break;
		}
	  if(left == 0)
	  	continue;
      if ((left >= MSDC_FIFO_SZ) && (msdc_txfifocnt() == 0)) {
            int count = MSDC_FIFO_SZ >> 2;
            do {
                msdc_fifo_write32(*ptr); ptr++;
            } while (--count);
            left -= MSDC_FIFO_SZ;          
        } else if (left < MSDC_FIFO_SZ && msdc_txfifocnt() == 0) {
            while (left > 3) {
                msdc_fifo_write32(*ptr); ptr++;
                left -= 4;
            } 

            u8ptr = (u8*)ptr; 
            while(left){
                msdc_fifo_write8(*u8ptr);    u8ptr++;
                left--;
            }
        } else {
                status = sdr_read32(MSDC_INT);

                if ((status & MSDC_INT_DATCRCERR) || (status & MSDC_INT_DATTMO)) {

                    if (status & MSDC_INT_DATCRCERR){
                        printk(KERN_ERR "[MSDC%d] DAT CRC error (0x%x), Left DAT: %d bytes\n", 
                                host->id, status, left);
                        err = -5;
                    }
                    if (status & MSDC_INT_DATTMO){
                        printk(KERN_ERR "[MSDC%d] DAT TMO error (0x%x), Left DAT: %d bytes\n", 
                                host->id, status, left);
                        err = -110;
                    }

                    simp_msdc_dump_register(host);

                    sdr_write32(MSDC_INT, status);
                    msdc_reset_hw(host->id);
                    return err;
                }
        }

      l_count++;
      if (l_count > 500) {
          l_count=0;
          printk(KERN_EMERG "size= %d, left= %d.\r\n", size, left);
          simp_msdc_dump_register(host);
      }
    }

    return err;         
}

static int simp_msdc_pio_read(struct simp_msdc_host *host, unsigned int *ptr, unsigned int size)
{
    unsigned int  base = host->base;    
    unsigned int  left = size;    
    unsigned int  status = 0;
    unsigned char *u8ptr;
    int l_count = 0;
    int err = 0;
	while(1){
  		status = sdr_read32(MSDC_INT);
		sdr_write32(MSDC_INT,status);
        if (status & MSDC_INT_DATCRCERR) {
            printk(KERN_ERR "[MSDC%d] DAT CRC error (0x%x), Left DAT: %d bytes\n", 
                host->id, status, left);
            err = -5;
            break;
        } else if (status & MSDC_INT_DATTMO) {
            printk(KERN_ERR "[MSDC%d] DAT TMO error (0x%x), Left DAT: %d bytes\n", 
                host->id, status, left);
            err = -110;
            break;
			}
		else if(status & MSDC_INT_XFER_COMPL){
			break;
		}
        
        if(left == 0){
            printk(KERN_EMERG "read fifo done, wait xfer_compl interrupt\n");
            continue;
        }
        
        while (left) {
            //printk(KERN_EMERG "left(%d)/FIFO(%d)\n", left,msdc_rxfifocnt());
            if ((left >=  MSDC_FIFO_THD) && (msdc_rxfifocnt() >= MSDC_FIFO_THD)) {    
                int count = MSDC_FIFO_THD >> 2; 
                do {
                    *ptr++ = msdc_fifo_read32();
                } while (--count);
                left -= MSDC_FIFO_THD;                
            } else if ((left < MSDC_FIFO_THD) && msdc_rxfifocnt() >= left) {
                while (left > 3) {
                    *ptr++ = msdc_fifo_read32();
                    left -= 4;
                }

                u8ptr = (u8 *)ptr; 
                while(left) {
                    *u8ptr++ = msdc_fifo_read8();
                    left--;       
                }
            } else {
                status = sdr_read32(MSDC_INT);

                if ((status & MSDC_INT_DATCRCERR) || (status & MSDC_INT_DATTMO)) {

                    if (status & MSDC_INT_DATCRCERR){
                        printk(KERN_ERR "[MSDC%d] DAT CRC error (0x%x), Left DAT: %d bytes\n", 
                                host->id, status, left);
                        err = -5;
                    }
                    if (status & MSDC_INT_DATTMO){
                        printk(KERN_ERR "[MSDC%d] DAT TMO error (0x%x), Left DAT: %d bytes\n", 
                                host->id, status, left);
                        err = -110;
                    }

                    simp_msdc_dump_register(host);

                    sdr_write32(MSDC_INT, status);
                    msdc_reset_hw(host->id);
                    return err;
                }
            }

            // timeout monitor
            l_count++;
            if (l_count > 50000) {
                l_count = 0;
                printk(KERN_EMERG "size= %d, left= %d.\r\n", size, left);
                simp_msdc_dump_register(host);
            }
        }    
    }
    return err;                 
}

static void simp_msdc_set_blknum(struct simp_msdc_host *host, unsigned int blknum)
{
    unsigned int base = host->base;
    sdr_write32(SDC_BLK_NUM, blknum);
}
static unsigned int simp_mmc_get_status(struct simp_mmc_host *host, unsigned int* status)
{
    unsigned int resp  = 0; 
    unsigned int err  = 0; 
    struct simp_msdc_host *phost = host->mtk_host;
    
    err = simp_msdc_cmd(phost, CMD13, CMD13_RAW, phost->card->rca << 16, RESP_R1,  &resp);
    if (err){
        printk(KERN_EMERG "cmd13: error(0x%d)\n", err);
    }

    *status = resp;

    return err;
}

static  int simp_mmc_single_write(struct simp_mmc_host *host, unsigned int addr, void* buf, unsigned int size)
{
    unsigned int resp  = 0; 
    unsigned int err = 0;
    //unsigned int intsts = 0;
    struct simp_msdc_host *phost = host->mtk_host;
    unsigned int base = phost->base;
    
    if (size != 512){
        printk(KERN_EMERG "emmc: write para error!\n");
        return -1;
    }
    
    simp_msdc_set_blknum(phost, 1);

    /* send command */
    err = simp_msdc_cmd(phost, CMD24, CMD24_RAW, addr, RESP_R1,  &resp);
    if (err){
        printk(KERN_EMERG "cmd24: error(%d)\n", err);
    }

    /* write the data to FIFO */
    err = simp_msdc_pio_write(phost, (unsigned int *)buf, 512); 
	if (err){
			printk(KERN_EMERG "write data: error(%d)\n", err);
	}

    /* make sure contents in fifo flushed to device */
    BUG_ON(msdc_txfifocnt());

    /* check and clear interrupt */
    //while( (intsts & MSDC_INT_XFER_COMPL) == 0 ){
      //  intsts = sdr_read32(MSDC_INT);                    
    //}
    //sdr_set_bits(MSDC_INT, MSDC_INT_XFER_COMPL);    

    return err;
}

static  int simp_mmc_single_read(struct simp_mmc_host *host, unsigned int addr, void* buf, unsigned int size)
{
    unsigned int resp  = 0; 
    unsigned int err  = 0; 
    //unsigned int intsts = 0;
    struct simp_msdc_host *phost = host->mtk_host;
    //unsigned int base = phost->base;
    
    if (size != 512){
        printk(KERN_EMERG "emmc: read para error!\n");
        return -1;
    }

    simp_msdc_set_blknum(phost, 1);

    /* send command */
    err = simp_msdc_cmd(phost, CMD17, CMD17_RAW, addr, RESP_R1,  &resp);
    if (err){
        printk(KERN_EMERG "cmd17: error(0x%d)\n", err);
    }

    /* read the data out*/
    err = simp_msdc_pio_read(phost, (unsigned int *)buf, 512);     
    if (err){
			printk(KERN_EMERG "read data: error(%d)\n", err);
	}
    /* check and clear interrupt */
   // while( (intsts & MSDC_INT_XFER_COMPL) == 0 ){
     //   intsts = sdr_read32(MSDC_INT);                    
   // }
    //sdr_set_bits(MSDC_INT, MSDC_INT_XFER_COMPL);    
    
    return err;
}
static unsigned int simp_mmc_send_stop(struct simp_mmc_host *host)
{ 
    unsigned int resp  = 0; 
    unsigned int err  = 0; 
    struct simp_msdc_host *phost = host->mtk_host;
    
    /* send command */
    err = simp_msdc_cmd(phost, CMD12, CMD12_RAW, 0, RESP_R1B,  &resp);
    if (err){
        printk(KERN_EMERG "cmd12: error(0x%d)\n", err);
    }
    
    return err;
}

static  int simp_mmc_multi_write(struct simp_mmc_host *host, unsigned int addr, void* buf, unsigned int nblk)
{
    unsigned int resp  = 0; 
    unsigned int err = 0;
    //unsigned int intsts = 0;
    struct simp_msdc_host *phost = host->mtk_host;
    unsigned int base = phost->base;
    
    simp_msdc_set_blknum(phost, nblk);

    /* send command */
    err = simp_msdc_cmd(phost, CMD25, CMD25_RAW, addr, RESP_R1,  &resp);
    if (err){
        printk(KERN_EMERG "cmd25: error(0x%d)\n", err);
    }

    /* write the data to FIFO */
    err = simp_msdc_pio_write(phost, (unsigned int *)buf, 512*nblk); 
	if (err){
				printk(KERN_EMERG "write data: error(%d)\n", err);
	}

    /* make sure contents in fifo flushed to device */
   BUG_ON(msdc_txfifocnt());

    /* check and clear interrupt */
     
	simp_mmc_send_stop(host);

    return err;
}
static  int simp_mmc_multi_read(struct simp_mmc_host *host, unsigned int addr, void* buf, unsigned int nblk)
{
    unsigned int resp  = 0; 
    unsigned int err  = 0; 
    //unsigned int intsts = 0;
    struct simp_msdc_host *phost = host->mtk_host;
    //unsigned int base = phost->base;
    
    
    simp_msdc_set_blknum(phost, nblk);

    /* send command */
    err = simp_msdc_cmd(phost, CMD18, CMD18_RAW, addr, RESP_R1,  &resp);
    if (err){
        printk(KERN_EMERG "cmd18: error(0x%d)\n", err);
    }

    /* read the data out*/
    err = simp_msdc_pio_read(phost, (unsigned int *)buf, 512*nblk);     
    if (err){
				printk(KERN_EMERG "read data: error(%d)\n", err);
	}

    simp_mmc_send_stop(host);
    return err;
}


/* card_type tell to use which host, will support PANIC dump info to emmc card
 * and KDUMP info to sd card */
int msdc_init_panic(int dev)
{
	return 1;
}
static int simp_mmc_get_host(int card_type,bool boot)
{
	int index = 0;
	for(;index < HOST_MAX_NUM;++index){
		if(p_msdc_hw[index]){
			if((card_type == p_msdc_hw[index]->host_function) && (boot == p_msdc_hw[index]->boot))
				return index;
		}
	}
	return HOST_MAX_NUM;
		
}
static int simp_mmc_init(int card_type,bool boot)
{
    struct simp_mmc_host *host;
    if(boot){
            /* init some struct */
            pmmc_boot_host->mtk_host = pmsdc_boot_host;
            pmmc_boot_host->card = &g_mmc_card[BOOT_STORAGE_ID]; 
            pmmc_boot_host->card->host = pmmc_boot_host; 
			
            host = pmmc_boot_host;

            memset(pmmc_boot_host->mtk_host, 0, sizeof(struct simp_msdc_host));  
            pmmc_boot_host->mtk_host->id       = simp_mmc_get_host(card_type,boot);
			if(pmmc_boot_host->mtk_host->id >= HOST_MAX_NUM)
				return -1;
            pmmc_boot_host->mtk_host->base     = u_msdc_base[pmmc_boot_host->mtk_host->id]; 
            pmmc_boot_host->mtk_host->clksrc   = MSDC_CLKSRC; 
            pmmc_boot_host->mtk_host->clk      = clks[MSDC_CLKSRC]; 
            pmmc_boot_host->mtk_host->card     = &g_msdc_card[BOOT_STORAGE_ID];

            /* not use now, may be delete */
            memset(&g_msdc_card[BOOT_STORAGE_ID], 0, sizeof(struct simp_msdc_card));  
            g_msdc_card[BOOT_STORAGE_ID].type        = MMC_TYPE_MMC;  
            g_msdc_card[BOOT_STORAGE_ID].file_system = _RAW_; 

            /* init host & card */
			
    	}
    else   {
            pmmc_extend_host->mtk_host = pmsdc_extend_host;
            pmmc_extend_host->card = &g_mmc_card[EXTEND_STORAGE_ID]; 
            pmmc_extend_host->card->host = pmmc_extend_host; 
			
            host = pmmc_extend_host;

            memset(pmmc_extend_host->mtk_host, 0, sizeof(struct simp_msdc_host));  
            pmmc_extend_host->mtk_host->id       = simp_mmc_get_host(card_type,boot);
			if(pmmc_extend_host->mtk_host->id >= HOST_MAX_NUM)
				return -1;
            pmmc_extend_host->mtk_host->base     = u_msdc_base[pmmc_extend_host->mtk_host->id]; 
            pmmc_extend_host->mtk_host->clksrc   = MSDC_CLKSRC; 
            pmmc_extend_host->mtk_host->clk      = clks[MSDC_CLKSRC]; 
            pmmc_extend_host->mtk_host->card     = &g_msdc_card[EXTEND_STORAGE_ID];

            /* not use now, may be delete */
            memset(&g_msdc_card[EXTEND_STORAGE_ID], 0, sizeof(struct simp_msdc_card));  
            g_msdc_card[EXTEND_STORAGE_ID].type        = MMC_TYPE_SD;  
            g_msdc_card[EXTEND_STORAGE_ID].file_system = FAT32; 
    	
            //printk(KERN_EMERG "g_msdc_card[SD_MSDC_ID] addr is 0x%x\n", &g_msdc_card[EXTEND_STORAGE_ID]);
            //printk(KERN_EMERG "g_msdc_card +1 addr is 0x%x\n", g_msdc_card + 1);
            //printk(KERN_EMERG "pmsdc_sd_host->card addr is 0x%x\n", pmsdc_extend_host->card);
            pmsdc_extend_host->card->card_cap = 1;
            }
	return 0;
}


static const unsigned g_freqs[] = {400000, 300000, 200000, 100000};
#define HOST_MIN_MCLK                    (260000)

#define MAX_POLLING_STATUS (50000)
/*--------------------------------------------------------------------------*/
/* porting for panic dump interface                                         */
/*--------------------------------------------------------------------------*/
#ifdef MTK_EMMC_SUPPORT
static int simp_emmc_dump_write(unsigned char* buf, unsigned int len, unsigned int offset,unsigned int dev)
{

    /* maybe delete in furture */
    unsigned int i;
	unsigned int status = 0;
	int polling = MAX_POLLING_STATUS;
    unsigned int l_user_begin_num = 0;
    unsigned int l_dest_num = 0;
    unsigned long long l_start_offset;
    unsigned int l_addr;
    unsigned char *l_buf;
    unsigned int ret = 1;
    static int emmc_init = 0;
	int err = 0;

    if (0 != len % 512) {
        /* emmc always in slot0 */
        printk("debug: parameter error!\n");
        return ret;
    }
	
#if 0
    printk("write data:");
    for (i = 0; i < 32; i++) {
        printk("0x%x", buf[i]);
        if (0 == (i+1)%32)
            printk("\n");
    }
#endif

    /* find the offset in emmc */
    for (i = 0; i < PART_NUM; i++) {
        if ('m' == *(PartInfo[i].name) && 'b' == *(PartInfo[i].name + 1) &&
            'r' == *(PartInfo[i].name + 2)){
            l_user_begin_num = i;
        }

        if ('e' == *(PartInfo[i].name) && 'x' == *(PartInfo[i].name + 1) &&
            'p' == *(PartInfo[i].name + 2) && 'd' == *(PartInfo[i].name + 3) &&
            'b' == *(PartInfo[i].name + 4)){
            l_dest_num = i;
        }
    }

    if (l_user_begin_num >= PART_NUM && l_dest_num >= PART_NUM) {
        printk("not find in scatter file error!\n");
        return ret;
    }

    if (PartInfo[l_dest_num].size < (len + offset)) {
        printk("write operation oversize!\n");
        return ret;
    }

#if MTK_MMC_DUMP_DBG
    printk(KERN_EMERG "write start address=%llu\n", 
                        PartInfo[l_dest_num].start_address - PartInfo[l_user_begin_num].start_address);
#endif 

    l_start_offset = (u64)offset + PartInfo[l_dest_num].start_address - PartInfo[l_user_begin_num].start_address;
   
    if (emmc_init == 0) {
        for (i = 0; i < ARRAY_SIZE(g_freqs); i++) {
            if (SIMP_SUCCESS == simp_mmc_rescan_try_freq(pmmc_boot_host, (unsigned)max(g_freqs[i], HOST_MIN_MCLK))) {
                break;
            }

            if (g_freqs[i] <= HOST_MIN_MCLK)
                break;
        }
        emmc_init = 1;
    }

    printk("-");
    for (i = 0; i < (len/512); i++) {
        /* code */
        l_addr = (l_start_offset >> 9) + i; //blk address
        l_buf  = (buf + i * 512);

#if MTK_MMC_DUMP_DBG
        printk("l_start_offset = 0x%x\n", l_addr);
#endif  
		
        err = simp_mmc_single_write(pmmc_boot_host, l_addr, l_buf, 512);
		do{
				simp_mmc_get_status(pmmc_boot_host, &status);
		}while(R1_CURRENT_STATE(status) == 7 && polling--);
	}
	if(err == 0){
		printk("=");
		return 0;}
	else
		return ret;
}
#endif /* end of MTK_EMMC_SUPPORT */

static int simp_sd_dump_write(unsigned char* buf, unsigned int len, unsigned int offset,unsigned int dev)
{
	//unsigned int i;
	unsigned int l_addr;
    unsigned char *l_buf;
	int polling = MAX_POLLING_STATUS;
	unsigned int status = 0;
	//unsigned int l_start_offset;
    unsigned int ret = SIMP_FAILED;
	int err = 0;
	
	if (0 != len % 512) {
			/* emmc always in slot0 */
			printk("debug: parameter error!\n");
			return ret;
		}
#if 0
		printk("write data:");
		for (i = 0; i < 32; i++) {
			printk("0x%x", buf[i]);
			if (0 == (i+1)%32)
				printk("\n");
		}
#endif
 //l_start_offset = offset;
 l_buf  = buf;
		if(pmsdc_extend_host->card->card_cap == standard_capacity) {
				 l_addr = offset << 9; 	
			} else {
				 l_addr = offset; 
			}	

#if MTK_MMC_DUMP_DBG
        printk("l_start_offset = 0x%x len = %d buf<0x%x>\n", l_addr,len,l_buf);
#endif  
		
		if(len == 512)      
        	err = simp_mmc_single_write(pmmc_extend_host, l_addr, l_buf, 512);
		else
    		err = simp_mmc_multi_write(pmmc_extend_host, l_addr, l_buf, len/512);
		do{
			simp_mmc_get_status(pmmc_extend_host, &status);
		}while(R1_CURRENT_STATE(status) == 7 && polling--);
		if(err == 0)
			ret = SIMP_SUCCESS;
		return ret;
}

static int sd_dump_read(unsigned char* buf, unsigned int len, unsigned int offset)
{
	static int sd_init = 0;
//	unsigned int i;
	unsigned int l_addr;
    unsigned char *l_buf;
	//unsigned int l_start_offset;
    unsigned int ret = SIMP_FAILED;
	int err = 0;
#if MTK_MMC_DUMP_DBG
        printk("1 l_start_offset = 0x%x len =0x \n", l_addr,len);
#endif  	
	if (0 != len % 512) {
			printk("debug: parameter error!\n");
			return ret;
		}

if (sd_init == 0) {
        sd_init = 1; 
	    simp_mmc_rescan_try_freq(pmmc_extend_host, 0);
    }
	 //l_start_offset = offset;
     l_buf  = buf;

#if MTK_MMC_DUMP_DBG
        printk("l_start_offset = 0x%x len = %d\n", offset,len);
#endif  
		if(pmsdc_extend_host->card->card_cap == standard_capacity) {
						 l_addr = offset << 9;	
					} else {
						 l_addr = offset; 
					}
		if(len == 512)      
        	err = simp_mmc_single_read(pmmc_extend_host, l_addr, l_buf, 512);
		else
    		err = simp_mmc_multi_read(pmmc_extend_host, l_addr, l_buf, len/512);
#if 0
		printk("read data:");
		for (i = 0; i < 32; i++) {
			printk("0x%x", buf[i]);
			if (0 == (i+1)%32)
				printk("\n");
		}
#endif
	if(err == 0)
		ret = SIMP_SUCCESS;
	return ret;
}

int card_dump_func_write(unsigned char* buf, unsigned int len, unsigned long long offset, int dev)
{
    int ret = SIMP_FAILED;

    //local_irq_disable();
    //preempt_disable();
    unsigned int sec_offset = 0;
	#if MTK_MMC_DUMP_DBG
	printk(KERN_EMERG "card_dump_func_write len<%d> addr<%lld> type<%d>\n",len,offset,dev);
	#endif
    if(offset % 512){
			 printk("Address isn't 512 alignment!\n");
			return SIMP_FAILED;
    	}
	sec_offset = offset/512;
    switch (dev){
        case DUMP_INTO_BOOT_CARD_IPANIC:
            #ifdef MTK_EMMC_SUPPORT
            ret = simp_emmc_dump_write(buf, len, (unsigned int)offset, dev);
			#endif
            break;
        case DUMP_INTO_BOOT_CARD_KDUMP:
            break;
        case DUMP_INTO_EXTERN_CARD:
			ret = simp_sd_dump_write(buf, len, sec_offset, dev);
            break;
        default:
            printk("unknown card type, error!\n");
            break;
    }

    return ret;
}
EXPORT_SYMBOL(card_dump_func_write);

extern int simple_sd_ioctl_rw(struct msdc_ioctl* msdc_ctl);
#ifdef MTK_EMMC_SUPPORT

#define SD_FALSE             -1
#define SD_TRUE              0
#define DEBUG_MMC_IOCTL      0
static int emmc_dump_read(unsigned char* buf, unsigned int len, unsigned int offset,unsigned int slot)
{
    /* maybe delete in furture */
    struct msdc_ioctl     msdc_ctl;
    unsigned int i;
    unsigned int l_user_begin_num = 0;
    unsigned int l_dest_num = 0;
    unsigned long long l_start_offset = 0;
    unsigned int ret = SD_FALSE;

    if ((0 != slot) || (0 != offset % 512) || (0 != len % 512)) {
        /* emmc always in slot0 */
        printk("debug: slot is not use for emmc!\n");
        return ret;
    }

    /* find the offset in emmc */
    for (i = 0; i < PART_NUM; i++) {
    //for (i = 0; i < 1; i++) {
        if ('m' == *(PartInfo[i].name) && 'b' == *(PartInfo[i].name + 1) &&
                'r' == *(PartInfo[i].name + 2)){
            l_user_begin_num = i;
        }

        if ('e' == *(PartInfo[i].name) && 'x' == *(PartInfo[i].name + 1) &&
                'p' == *(PartInfo[i].name + 2) && 'd' == *(PartInfo[i].name + 3) &&
                'b' == *(PartInfo[i].name + 4)){
            l_dest_num = i;
        }
    }

#if DEBUG_MMC_IOCTL
    printk("l_user_begin_num = %d l_dest_num = %d\n",l_user_begin_num,l_dest_num);
#endif

    if (l_user_begin_num >= PART_NUM && l_dest_num >= PART_NUM) {
        printk("not find in scatter file error!\n");
        return ret;
    }

    if (PartInfo[l_dest_num].size < (len + offset)) {
        printk("read operation oversize!\n");
        return ret;
    }

#if DEBUG_MMC_IOCTL
    printk("read start address=0x%llx\n", PartInfo[l_dest_num].start_address - PartInfo[l_user_begin_num].start_address);
#endif 
    l_start_offset = offset + PartInfo[l_dest_num].start_address - PartInfo[l_user_begin_num].start_address;

    msdc_ctl.partition = 0;
    msdc_ctl.iswrite = 0;
    msdc_ctl.host_num = slot;
    msdc_ctl.opcode = MSDC_CARD_DUNM_FUNC;
    msdc_ctl.total_size = 512;
    msdc_ctl.trans_type = 0;
    for (i = 0; i < (len/512); i++) {
        /* code */
        msdc_ctl.address = (l_start_offset >> 9) + i; //blk address
        msdc_ctl.buffer =(u32*)(buf + i * 512);

#if DEBUG_MMC_IOCTL
        printk("l_start_offset = 0x%x\n", msdc_ctl.address);
#endif        
        msdc_ctl.result = simple_sd_ioctl_rw(&msdc_ctl);
    }
    
#if DEBUG_MMC_IOCTL
    printk("read data:");
    for (i = 0; i < 32; i++) {
        printk("0x%x", buf[i]);
        if (0 == (i+1)%32)
            printk("\n");
    }
#endif
    return SD_TRUE;
}
#endif

int card_dump_func_read(unsigned char* buf, unsigned int len, unsigned long long offset, int dev)
{

//    unsigned int l_slot;
    unsigned int ret = SIMP_FAILED;
	unsigned int sec_offset = 0;
	#if MTK_MMC_DUMP_DBG
	printk(KERN_EMERG "card_dump_func_read len<%d> addr<%lld> type<%d>\n",len,offset,dev);
	#endif
	if(offset % 512){
			 printk("Address isn't 512 alignment!\n");
			return SIMP_FAILED;
    	}
	sec_offset = offset/512;
    switch (dev){
        case DUMP_INTO_BOOT_CARD_IPANIC:
            #ifdef MTK_EMMC_SUPPORT
            ret = emmc_dump_read(buf, len, (unsigned int)offset, dev);
			#endif
            break;
        case DUMP_INTO_BOOT_CARD_KDUMP:
            break;
        case DUMP_INTO_EXTERN_CARD:
			ret = sd_dump_read(buf, len, sec_offset);
            break;
        default:
            printk("unknown card type, error!\n");
            break;
    }
    return ret;

}
EXPORT_SYMBOL(card_dump_func_read);


/*--------------------------------------------------------------------------*/
/* porting for kdump interface                                              */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* module init/exit                                                         */
/*--------------------------------------------------------------------------*/

static void simp_msdc_hw_init(void)
{
#ifdef CFG_DEV_MSDC0
		p_msdc_hw[0] = &msdc0_hw;
#endif
#ifdef CFG_DEV_MSDC1
		p_msdc_hw[1] = &msdc1_hw;
#endif
#ifdef CFG_DEV_MSDC2
		p_msdc_hw[2] = &msdc2_hw;
#endif
#ifdef CFG_DEV_MSDC3
		p_msdc_hw[3] = &msdc3_hw;
#endif
#ifdef CFG_DEV_MSDC4
		p_msdc_hw[4] = &msdc4_hw;
#endif
}
static int __init emmc_dump_init(void)
{
   	simp_msdc_hw_init();
	#ifdef MTK_EMMC_SUPPORT
	simp_mmc_init(MSDC_EMMC,1);
	#endif
    simp_mmc_init(MSDC_SD,0);
	printk(KERN_EMERG "EMMC/SD dump init\n");
    return 0;
}

static void __exit emmc_dump_exit(void)
{
}
module_init(emmc_dump_init);
module_exit(emmc_dump_exit);
