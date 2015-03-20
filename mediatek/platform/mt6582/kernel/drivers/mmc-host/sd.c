#include <generated/autoconf.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/blkdev.h>
#include <linux/slab.h>
#include <linux/wakelock.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/core.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdio.h>
#include <linux/dma-mapping.h>
#include <linux/irq.h>
#include <linux/kthread.h>
#include <linux/mm_types.h>
#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/mman.h>
#include <linux/pagemap.h>
#include <linux/highmem.h>
#include <asm/page.h>

#include <mach/dma.h>
#include <mach/board.h>
#include <mach/devs.h>
#include <mach/mt_typedefs.h>
#include <mach/irqs.h>
#include <mach/mt_gpio.h>

#include "mt_sd.h"
#include "dbg.h"

#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
#include <linux/xlog.h>
#include <mach/mtk_thermal_monitor.h>
#endif // MTK_SDIO30_ONLINE_TUNING_SUPPORT

#include <linux/proc_fs.h>
#include "drivers/mmc/card/queue.h"
#include "partition_define.h"
#include <mach/emi_mpu.h>
#include <mach/memory.h>
#ifdef CONFIG_MTK_AEE_FEATURE
#include <linux/aee.h>
#endif  

#ifdef CONFIG_MTK_HIBERNATION
#include "mach/mtk_hibernate_dpm.h"
#endif

#ifndef FPGA_PLATFORM
#include <mach/mt_pm_ldo.h>
#include <mach/mt_clkmgr.h>
//#include "mach/mt6575_clkmgr_internal.h"
#include <mach/eint.h>
#include <cust_eint.h>
//#include <cust_power.h>
//#define MSDC_POWER_MC1 MSDC_VMC //Don't define this in local code!!!!!!
//#define MSDC_POWER_MC2 MSDC_VGP6 //Don't define this in local code!!!!!!
#endif
//static struct workqueue_struct *workqueue;

#include <mach/mt_storage_logger.h>
#include "partition_define.h"

#define EXT_CSD_BOOT_SIZE_MULT           226 /* R */
#define EXT_CSD_RPMB_SIZE_MULT           168 /* R */
#define EXT_CSD_GP1_SIZE_MULT            143 /* R/W 3 bytes */
#define EXT_CSD_GP2_SIZE_MULT            146 /* R/W 3 bytes */
#define EXT_CSD_GP3_SIZE_MULT            149 /* R/W 3 bytes */
#define EXT_CSD_GP4_SIZE_MULT            152 /* R/W 3 bytes */
#define EXT_CSD_PART_CFG                 179 /* R/W/E & R/W/E_P */
#define CAPACITY_2G                      (2 * 1024 * 1024 * 1024ULL)

#ifdef MTK_EMMC_SUPPORT
u32 g_emmc_mode_switch = 0;
#define MTK_EMMC_ETT_TO_DRIVER  /* for eMMC off-line apply to driver */
#endif

unsigned long long msdc_print_start_time;
unsigned long long msdc_print_end_time;
unsigned int print_nums;

#define UNSTUFF_BITS(resp,start,size)          \
({                \
    const int __size = size;        \
    const u32 __mask = (__size < 32 ? 1 << __size : 0) - 1;  \
    const int __off = 3 - ((start) / 32);      \
    const int __shft = (start) & 31;      \
    u32 __res;            \
                  \
    __res = resp[__off] >> __shft;        \
    if (__size + __shft > 32)        \
        __res |= resp[__off-1] << ((32 - __shft) % 32);  \
    __res & __mask;            \
}) 
  
#ifdef MTK_EMMC_ETT_TO_DRIVER
#include "emmc_device_list.h"
static  u8   m_id = 0;           // Manufacturer ID
static char pro_name[8] = {0};  // Product name
#endif 

#if (MSDC_DATA1_INT == 1)
static u16 u_sdio_irq_counter=0;
static u16 u_msdc_irq_counter=0;
static int int_sdio_irq_enable=0;
#endif

static DEFINE_SPINLOCK(SDIO_IRQ_lock);
struct msdc_host *ghost=NULL;
int src_clk_control=0;
struct mmc_blk_data {
    spinlock_t lock;
    struct gendisk *disk;
    struct mmc_queue queue;

    unsigned int usage;
    unsigned int read_only;
};

#ifdef MTK_SDIOAUTOK_SUPPORT
unsigned int osc_value = 0;
#endif


#define DRV_NAME                         "mtk-msdc"

#define MSDC_COOKIE_PIO		(1<<0)
#define MSDC_COOKIE_ASYNC	(1<<1)

#define msdc_use_async_way(x)	(x & MSDC_COOKIE_ASYNC)
#define msdc_async_use_dma(x)   ((x & MSDC_COOKIE_ASYNC) && (!(x & MSDC_COOKIE_PIO)))
#define msdc_async_use_pio(x)   ((x & MSDC_COOKIE_ASYNC) && ((x & MSDC_COOKIE_PIO))) //not used
#ifdef FPGA_PLATFORM
#define HOST_MAX_MCLK                    (12000000)
#else
#define HOST_MAX_MCLK                    (200000000) //(104000000)
#endif
#define HOST_MIN_MCLK                    (260000)

#define HOST_MAX_BLKSZ                   (2048)

#define MSDC_OCR_AVAIL                   (MMC_VDD_28_29 | MMC_VDD_29_30 | MMC_VDD_30_31 | MMC_VDD_31_32 | MMC_VDD_32_33)
//#define MSDC_OCR_AVAIL                 (MMC_VDD_32_33 | MMC_VDD_33_34)

#define GPIO_PULL_DOWN                   (0)
#define GPIO_PULL_UP                     (1)

#define MSDC1_IRQ_SEL                    (1 << 9)
#define PDN_REG                          (0xF1000010) 

#define DEFAULT_DEBOUNCE                 (8)       /* 8 cycles */
#define DEFAULT_DTOC                     (3)      /* data timeout counter. 65536x40(75/77) /1048576 * 3(83/85) sclk. */

#define CMD_TIMEOUT                      (HZ/10 * 5)   /* 100ms x5 */
#define DAT_TIMEOUT                      (HZ    * 5)   /* 1000ms x5 */
#define CLK_TIMEOUT                      (HZ    * 5)  /* 5s    */ 
#define POLLING_BUSY                     (HZ     * 3)
#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
//#define OT_PERIOD           60000          /* 60s    */ 
#define OT_PERIOD           (HZ    * 60)   /* 1000ms x60 */
#define OT_RX_TIMEOUT       (HZ/10)        /* 100ms */
#define OT_TIMEOUT          (HZ/50)           /* 20ms */
#define OT_TEMPDIFF_BOUNDRY 10000          /* 10 degree */ /* trigger online tuning if temperature difference exceed this value */
#define OT_BLK_SIZE         0x100
#define OT_START_TUNEWND    3
#endif // MTK_SDIO30_ONLINE_TUNING_SUPPORT
#define MAX_DMA_CNT                      (64 * 1024 - 512)   /* a single transaction for WIFI may be 50K*/
#define MAX_DMA_CNT_SDIO    (0xFFFFFFFF - 255)   /* a single transaction for LTE may be 128K*/ /* Basic DMA use 32 bits to store transfer size */

#define MAX_HW_SGMTS                     (MAX_BD_NUM)
#define MAX_PHY_SGMTS                    (MAX_BD_NUM)
#define MAX_SGMT_SZ                      (MAX_DMA_CNT)
#define MAX_SGMT_SZ_SDIO    (MAX_DMA_CNT_SDIO)
#define MAX_REQ_SZ                       (512*1024)  

#define CMD_TUNE_UHS_MAX_TIME            (2*32*8*8)
#define CMD_TUNE_HS_MAX_TIME             (2*32)

#define READ_TUNE_UHS_CLKMOD1_MAX_TIME   (2*32*32*8)
#define READ_TUNE_UHS_MAX_TIME           (2*32*32)
#define READ_TUNE_HS_MAX_TIME            (2*32)

#define WRITE_TUNE_HS_MAX_TIME           (2*32)
#define WRITE_TUNE_UHS_MAX_TIME          (2*32*8)

#ifdef MT_SD_DEBUG
static struct msdc_regs *msdc_reg[HOST_MAX_NUM];
#endif 

#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
static unsigned int msdc_online_tuning(struct msdc_host   *host, unsigned fn, unsigned addr);
#endif // MTK_SDIO30_ONLINE_TUNING_SUPPORT

//=================================

#define MSDC_LOWER_FREQ 
#define MSDC_MAX_FREQ_DIV                (2)  /* 200 / (4 * 2) */
#define MSDC_MAX_TIMEOUT_RETRY           (1)
#define MSDC_MAX_TIMEOUT_RETRY_EMMC      (2)
#define MSDC_MAX_W_TIMEOUT_TUNE          (5)
#define MSDC_MAX_W_TIMEOUT_TUNE_EMMC     (64)
#define MSDC_MAX_R_TIMEOUT_TUNE          (3)
#define  MSDC_MAX_POWER_CYCLE            (3)
#ifdef FPGA_PLATFORM
#define PWR_GPIO                         (0xf0001E84)
#define PWR_GPIO_EO                      (0xf0001E88)
#define PWR_MASK_VOL_33                  (1 << 10)
#define PWR_MASK_VOL_18                  (1 << 9)
#define PWR_MASK_EN                      (1 << 8)
#define PWR_GPIO_L4_DIR                  (1 << 11)
#define PWR_MASK_VOL_33_MASK             (~(1 << 10))
#define PWR_MASK_EN_MASK                 (~(1 << 8))
#define PWR_GPIO_L4_DIR_MASK             (~(1 << 11))

/*
#define MBR_START_ADDRESS_BYTE (0x660000)

#define PART_NUM  20

struct excel_info PartInfo[PART_NUM]={
    {"preloader",262144,0x0,0},
    {"dsp_bl",1966080,0x40000,0},
    {"mbr",16384,0x220000,0},
    {"ebr1",376832,0x224000,1},
    {"pmt",4194304,0x280000,0},
    {"nvram",3145728,0x680000,0},
    {"seccfg",131072,0x980000,0},
    {"uboot",393216,0x9a0000,0},
    {"bootimg",6291456,0xa00000,0},
    {"recovery",6291456,0x1000000,0},
    {"sec_ro",6291456,0x1600000,5},
    {"misc",393216,0x1c00000,0},
    {"logo",3145728,0x1c60000,0},
    {"expdb",655360,0x1f60000,0},
    {"ebr2",16384,0x2000000,0},
    {"android",537919488,0x2004000,6},
    {"cache",537919488,0x22104000,2},
    {"usrdata",537919488,0x42204000,3},
    {"fat",0,0x62304000,4},
    {"bmtpool",10485760,0xFFFF0050,0},
 };
*/
bool hwPowerOn_fpga(void){
    volatile u16 l_val;
    
    l_val = sdr_read16(PWR_GPIO); 
    sdr_write16(PWR_GPIO, (l_val | PWR_MASK_VOL_33 | PWR_MASK_EN | PWR_GPIO_L4_DIR));

    l_val = sdr_read16(PWR_GPIO); 
    printk("[%s]: pwr gpio = 0x%x\n", __func__, l_val);
    return true; 
}

bool hwPowerSwitch_fpga(void){
    volatile u16 l_val;
    
    l_val = sdr_read16(PWR_GPIO);
    sdr_write16(PWR_GPIO, (l_val & PWR_MASK_VOL_33_MASK));
    l_val = sdr_read16(PWR_GPIO);
    sdr_write16(PWR_GPIO, (l_val | PWR_MASK_VOL_18));

    l_val = sdr_read16(PWR_GPIO); 
    printk("[%s]: pwr gpio = 0x%x\n", __func__, l_val);
    return true; 
}

bool hwPowerDown_fpga(void){
    volatile u16 l_val;
    
    l_val = sdr_read16(PWR_GPIO); 
    sdr_write8(PWR_GPIO, (l_val & PWR_MASK_VOL_33_MASK & PWR_MASK_EN_MASK));

    l_val = sdr_read16(PWR_GPIO); 
    printk("[%s]: pwr gpio = 0x%x\n", __func__, l_val);
    return true; 
}
#endif
 
//static int g_intsts[] = {0, 0, 0, 0};
#if   (5 == HOST_MAX_NUM)
struct msdc_host *mtk_msdc_host[]    = {NULL, NULL, NULL, NULL, NULL};
int g_dma_debug[HOST_MAX_NUM]        = {0, 0, 0, 0, 0};
u32 latest_int_status[HOST_MAX_NUM]  = {0, 0, 0, 0, 0};

transfer_mode msdc_latest_transfer_mode[HOST_MAX_NUM] = // 0 for PIO; 1 for DMA; 2 for nothing
{
    TRAN_MOD_NUM,
    TRAN_MOD_NUM,
    TRAN_MOD_NUM,
    TRAN_MOD_NUM,
    TRAN_MOD_NUM
};

operation_type msdc_latest_operation_type[HOST_MAX_NUM] = // 0 for read; 1 for write; 2 for nothing
{
    OPER_TYPE_NUM,
    OPER_TYPE_NUM,
    OPER_TYPE_NUM,
    OPER_TYPE_NUM,
    OPER_TYPE_NUM
};

#elif (4 == HOST_MAX_NUM)
struct msdc_host *mtk_msdc_host[]    = {NULL, NULL, NULL, NULL};
int g_dma_debug[HOST_MAX_NUM]        = {0, 0, 0, 0};
u32 latest_int_status[HOST_MAX_NUM]  = {0, 0, 0, 0};

transfer_mode msdc_latest_transfer_mode[HOST_MAX_NUM] = // 0 for PIO; 1 for DMA; 2 for nothing
{
    TRAN_MOD_NUM,
    TRAN_MOD_NUM,
    TRAN_MOD_NUM,
    TRAN_MOD_NUM,
};

operation_type msdc_latest_operation_type[HOST_MAX_NUM] = // 0 for read; 1 for write; 2 for nothing
{
    OPER_TYPE_NUM,
    OPER_TYPE_NUM,
    OPER_TYPE_NUM,
    OPER_TYPE_NUM,
};

#elif (3 == HOST_MAX_NUM)
struct msdc_host *mtk_msdc_host[]    = {NULL, NULL, NULL};
int g_dma_debug[HOST_MAX_NUM]        = {0, 0, 0};
u32 latest_int_status[HOST_MAX_NUM]  = {0, 0, 0};

transfer_mode msdc_latest_transfer_mode[HOST_MAX_NUM] = // 0 for PIO; 1 for DMA; 2 for nothing
{
    TRAN_MOD_NUM,
    TRAN_MOD_NUM,
    TRAN_MOD_NUM,
};

operation_type msdc_latest_operation_type[HOST_MAX_NUM] = // 0 for read; 1 for write; 2 for nothing
{
    OPER_TYPE_NUM,
    OPER_TYPE_NUM,
    OPER_TYPE_NUM,
};

#endif /* end of HOST_MAX_NUM */



struct dma_addr msdc_latest_dma_address[MAX_BD_PER_GPD];

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

/* For Inhanced DMA */
#define msdc_init_gpd_ex(gpd,extlen,cmd,arg,blknum) \
    do { \
        ((gpd_t*)gpd)->extlen = extlen; \
        ((gpd_t*)gpd)->cmd    = cmd; \
        ((gpd_t*)gpd)->arg    = arg; \
        ((gpd_t*)gpd)->blknum = blknum; \
    }while(0)
    
#define msdc_init_bd(bd, blkpad, dwpad, dptr, dlen) \
    do { \
        BUG_ON(dlen > 0xFFFFUL); \
        ((bd_t*)bd)->blkpad = blkpad; \
        ((bd_t*)bd)->dwpad  = dwpad; \
        ((bd_t*)bd)->ptr    = (void*)dptr; \
        ((bd_t*)bd)->buflen = dlen; \
    }while(0)

#define msdc_txfifocnt()   ((sdr_read32(MSDC_FIFOCS) & MSDC_FIFOCS_TXCNT) >> 16)
#define msdc_rxfifocnt()   ((sdr_read32(MSDC_FIFOCS) & MSDC_FIFOCS_RXCNT) >> 0)
#define msdc_fifo_write32(v)   sdr_write32(MSDC_TXDATA, (v))
#define msdc_fifo_write8(v)    sdr_write8(MSDC_TXDATA, (v))
#define msdc_fifo_read32()   sdr_read32(MSDC_RXDATA)
#define msdc_fifo_read8()    sdr_read8(MSDC_RXDATA)  

#define msdc_dma_on()        sdr_clr_bits(MSDC_CFG, MSDC_CFG_PIO)
#define msdc_dma_off()       sdr_set_bits(MSDC_CFG, MSDC_CFG_PIO)
#define msdc_dma_status()    ((sdr_read32(MSDC_CFG) & MSDC_CFG_PIO) >> 3)

u32 msdc_dump_padctl0(u32 id)
{  
    u32 reg = 0;
    u32 tmp = 0;
    switch(id){
        case 0:
            sdr_get_field(MSDC0_GPIO_PAD_BASE, GPIO_PAD_RDSEL_MASK, tmp);
            reg |= tmp << 24;
            sdr_get_field(MSDC0_GPIO_PAD_BASE, GPIO_PAD_TDSEL_MASK, tmp);
            reg |= tmp << 20;
            sdr_get_field(MSDC0_GPIO_CLK_BASE, GPIO_IES_MASK, tmp);
            reg |= tmp << 19;
            sdr_get_field(MSDC0_GPIO_CLK_BASE, GPIO_SMT_MASK, tmp);
            reg |= tmp << 18;
            sdr_get_field(MSDC0_GPIO_CLK_BASE, GPIO_PUPD_MASK | GPIO_R1_MASK | GPIO_R0_MASK, tmp);
            reg |= tmp << 15;
            sdr_get_field(MSDC0_GPIO_CLK_BASE, GPIO_SR_MASK, tmp);
            reg |= tmp << 8;
            sdr_get_field(MSDC0_GPIO_CLK_BASE, GPIO_MSDC0_DRVN, tmp);
            reg |= tmp << 0;
            break;
        case 1:
            sdr_get_field(MSDC1_GPIO_PAD_BASE, GPIO_PAD_RDSEL_MASK, tmp);
            reg |= tmp << 24;
            sdr_get_field(MSDC1_GPIO_PAD_BASE, GPIO_PAD_TDSEL_MASK, tmp);
            reg |= tmp << 20;
            sdr_get_field(MSDC1_GPIO_CLK_BASE, GPIO_IES_MASK, tmp);
            reg |= tmp << 19;
            sdr_get_field(MSDC1_GPIO_CLK_BASE, GPIO_SMT_MASK, tmp);
            reg |= tmp << 18;
            sdr_get_field(MSDC1_GPIO_CLK_BASE,  GPIO_PD_MASK | GPIO_PU_MASK, tmp);
            reg |= tmp << 16;
            sdr_get_field(MSDC1_GPIO_CLK_BASE, GPIO_SR_MASK, tmp);
            reg |= tmp << 8;
            sdr_get_field(MSDC1_GPIO_CLK_BASE, GPIO_MSDC1_MSDC2_DRVN, tmp);
            reg |= tmp << 0;
            break;
        case 2:
            sdr_get_field(MSDC2_GPIO_PAD_BASE, GPIO_PAD_RDSEL_MASK, tmp);
            reg |= tmp << 24;
            sdr_get_field(MSDC2_GPIO_PAD_BASE, GPIO_PAD_TDSEL_MASK, tmp);
            reg |= tmp << 20;
            sdr_get_field(MSDC2_GPIO_CLK_BASE, GPIO_IES_MASK, tmp);
            reg |= tmp << 19;
            sdr_get_field(MSDC2_GPIO_CLK_BASE, GPIO_SMT_MASK, tmp);
            reg |= tmp << 18;
            sdr_get_field(MSDC2_GPIO_CLK_BASE,  GPIO_PD_MASK | GPIO_PU_MASK, tmp);
            reg |= tmp << 16;
            sdr_get_field(MSDC2_GPIO_CLK_BASE, GPIO_SR_MASK, tmp);
            reg |= tmp << 8;
            sdr_get_field(MSDC2_GPIO_CLK_BASE, GPIO_MSDC1_MSDC2_DRVN, tmp);
            reg |= tmp << 0;
            break;
        default:
            break;
    }
    return reg;
}
//EXPORT_SYMBOL(msdc_dump_padctl0);

u32 msdc_dump_padctl1(u32 id)
{    
    u32 reg = 0;
    u32 tmp = 0;
    switch(id){
        case 0:
            sdr_get_field(MSDC0_GPIO_PAD_BASE, GPIO_PAD_RDSEL_MASK, tmp);
            reg |= tmp << 24;
            sdr_get_field(MSDC0_GPIO_PAD_BASE, GPIO_PAD_TDSEL_MASK, tmp);
            reg |= tmp << 20;
            sdr_get_field(MSDC0_GPIO_CMD_BASE, GPIO_IES_MASK, tmp);
            reg |= tmp << 19;
            sdr_get_field(MSDC0_GPIO_CMD_BASE, GPIO_SMT_MASK, tmp);
            reg |= tmp << 18;
            sdr_get_field(MSDC0_GPIO_CMD_BASE, GPIO_PUPD_MASK | GPIO_R1_MASK | GPIO_R0_MASK, tmp);
            reg |= tmp << 15;
            sdr_get_field(MSDC0_GPIO_CMD_BASE, GPIO_SR_MASK, tmp);
            reg |= tmp << 8;
            sdr_get_field(MSDC0_GPIO_CMD_BASE, GPIO_MSDC0_DRVN, tmp);
            reg |= tmp << 0;
            break;
        case 1:
            sdr_get_field(MSDC1_GPIO_PAD_BASE, GPIO_PAD_RDSEL_MASK, tmp);
            reg |= tmp << 24;
            sdr_get_field(MSDC1_GPIO_PAD_BASE, GPIO_PAD_TDSEL_MASK, tmp);
            reg |= tmp << 20;
            sdr_get_field(MSDC1_GPIO_CMD_BASE, GPIO_IES_MASK, tmp);
            reg |= tmp << 19;
            sdr_get_field(MSDC1_GPIO_CMD_BASE, GPIO_SMT_MASK, tmp);
            reg |= tmp << 18;
            sdr_get_field(MSDC1_GPIO_CMD_BASE,  GPIO_PD_MASK | GPIO_PU_MASK, tmp);
            reg |= tmp << 16;
            sdr_get_field(MSDC1_GPIO_CMD_BASE, GPIO_SR_MASK, tmp);
            reg |= tmp << 8;
            sdr_get_field(MSDC1_GPIO_CMD_BASE, GPIO_MSDC1_MSDC2_DRVN, tmp);
            reg |= tmp << 0;
            break;
        case 2:
            sdr_get_field(MSDC2_GPIO_PAD_BASE, GPIO_PAD_RDSEL_MASK, tmp);
            reg |= tmp << 24;
            sdr_get_field(MSDC2_GPIO_PAD_BASE, GPIO_PAD_TDSEL_MASK, tmp);
            reg |= tmp << 20;
            sdr_get_field(MSDC2_GPIO_CMD_BASE, GPIO_IES_MASK, tmp);
            reg |= tmp << 19;
            sdr_get_field(MSDC2_GPIO_CMD_BASE, GPIO_SMT_MASK, tmp);
            reg |= tmp << 18;
            sdr_get_field(MSDC2_GPIO_CMD_BASE,  GPIO_PD_MASK | GPIO_PU_MASK, tmp);
            reg |= tmp << 16;
            sdr_get_field(MSDC2_GPIO_CMD_BASE, GPIO_SR_MASK, tmp);
            reg |= tmp << 8;
            sdr_get_field(MSDC2_GPIO_CMD_BASE, GPIO_MSDC1_MSDC2_DRVN, tmp);
            reg |= tmp << 0;
            break;
        default:
            break;
    }
    return reg;
}
//EXPORT_SYMBOL(msdc_dump_padctl1);

u32 msdc_dump_padctl2(u32 id)
{    
    u32 reg = 0;
    u32 tmp = 0;
    switch(id){
        case 0:
            sdr_get_field(MSDC0_GPIO_PAD_BASE, GPIO_PAD_RDSEL_MASK, tmp);
            reg |= tmp << 24;
            sdr_get_field(MSDC0_GPIO_PAD_BASE, GPIO_PAD_TDSEL_MASK, tmp);
            reg |= tmp << 20;
            sdr_get_field(MSDC0_GPIO_DAT_BASE, GPIO_IES_MASK, tmp);
            reg |= tmp << 19;
            sdr_get_field(MSDC0_GPIO_DAT_BASE, GPIO_SMT_MASK, tmp);
            reg |= tmp << 18;
            sdr_get_field(MSDC0_GPIO_DAT_BASE, GPIO_PUPD_MASK | GPIO_R1_MASK | GPIO_R0_MASK, tmp);
            reg |= tmp << 15;
            sdr_get_field(MSDC0_GPIO_DAT_BASE, GPIO_SR_MASK, tmp);
            reg |= tmp << 8;
            sdr_get_field(MSDC0_GPIO_DAT_BASE, GPIO_MSDC0_DRVN, tmp);
            reg |= tmp << 0;
            break;
        case 1:
            sdr_get_field(MSDC1_GPIO_PAD_BASE, GPIO_PAD_RDSEL_MASK, tmp);
            reg |= tmp << 24;
            sdr_get_field(MSDC1_GPIO_PAD_BASE, GPIO_PAD_TDSEL_MASK, tmp);
            reg |= tmp << 20;
            sdr_get_field(MSDC1_GPIO_DAT_BASE, GPIO_IES_MASK, tmp);
            reg |= tmp << 19;
            sdr_get_field(MSDC1_GPIO_DAT_BASE, GPIO_SMT_MASK, tmp);
            reg |= tmp << 18;
            sdr_get_field(MSDC1_GPIO_DAT_BASE,  GPIO_PD_MASK | GPIO_PU_MASK, tmp);
            reg |= tmp << 16;
            sdr_get_field(MSDC1_GPIO_DAT_BASE, GPIO_SR_MASK, tmp);
            reg |= tmp << 8;
            sdr_get_field(MSDC1_GPIO_DAT_BASE, GPIO_MSDC1_MSDC2_DRVN, tmp);
            reg |= tmp << 0;
            break;
        case 2:
            sdr_get_field(MSDC2_GPIO_PAD_BASE, GPIO_PAD_RDSEL_MASK, tmp);
            reg |= tmp << 24;
            sdr_get_field(MSDC2_GPIO_PAD_BASE, GPIO_PAD_TDSEL_MASK, tmp);
            reg |= tmp << 20;
            sdr_get_field(MSDC2_GPIO_DAT_BASE, GPIO_IES_MASK, tmp);
            reg |= tmp << 19;
            sdr_get_field(MSDC2_GPIO_DAT_BASE, GPIO_SMT_MASK, tmp);
            reg |= tmp << 18;
            sdr_get_field(MSDC2_GPIO_DAT_BASE,  GPIO_PD_MASK | GPIO_PU_MASK, tmp);
            reg |= tmp << 16;
            sdr_get_field(MSDC2_GPIO_DAT_BASE, GPIO_SR_MASK, tmp);
            reg |= tmp << 8;
            sdr_get_field(MSDC2_GPIO_DAT_BASE, GPIO_MSDC1_MSDC2_DRVN, tmp);
            reg |= tmp << 0;
            break;
        default:
            break;
    }
    return reg;

}
//EXPORT_SYMBOL(msdc_dump_padctl2);

static void msdc_dump_register(struct msdc_host *host)
{
    u32 base = host->base; 
    //u32 off = 0; 

    INIT_MSG("Reg[00] MSDC_CFG       = 0x%.8x", sdr_read32(base + 0x00));
    INIT_MSG("Reg[04] MSDC_IOCON     = 0x%.8x", sdr_read32(base + 0x04));
    INIT_MSG("Reg[08] MSDC_PS        = 0x%.8x", sdr_read32(base + 0x08));
    INIT_MSG("Reg[0C] MSDC_INT       = 0x%.8x", sdr_read32(base + 0x0C));
    INIT_MSG("Reg[10] MSDC_INTEN     = 0x%.8x", sdr_read32(base + 0x10));    
    INIT_MSG("Reg[14] MSDC_FIFOCS    = 0x%.8x", sdr_read32(base + 0x14));
    INIT_MSG("Reg[18] MSDC_TXDATA    = not read");                        
    INIT_MSG("Reg[1C] MSDC_RXDATA    = not read");
    INIT_MSG("Reg[30] SDC_CFG        = 0x%.8x", sdr_read32(base + 0x30));
    INIT_MSG("Reg[34] SDC_CMD        = 0x%.8x", sdr_read32(base + 0x34));
    INIT_MSG("Reg[38] SDC_ARG        = 0x%.8x", sdr_read32(base + 0x38));
    INIT_MSG("Reg[3C] SDC_STS        = 0x%.8x", sdr_read32(base + 0x3C));
    INIT_MSG("Reg[40] SDC_RESP0      = 0x%.8x", sdr_read32(base + 0x40));
    INIT_MSG("Reg[44] SDC_RESP1      = 0x%.8x", sdr_read32(base + 0x44));
    INIT_MSG("Reg[48] SDC_RESP2      = 0x%.8x", sdr_read32(base + 0x48));                                
    INIT_MSG("Reg[4C] SDC_RESP3      = 0x%.8x", sdr_read32(base + 0x4C));
    INIT_MSG("Reg[50] SDC_BLK_NUM    = 0x%.8x", sdr_read32(base + 0x50));
    INIT_MSG("Reg[58] SDC_CSTS       = 0x%.8x", sdr_read32(base + 0x58));
    INIT_MSG("Reg[5C] SDC_CSTS_EN    = 0x%.8x", sdr_read32(base + 0x5C));
    INIT_MSG("Reg[60] SDC_DATCRC_STS = 0x%.8x", sdr_read32(base + 0x60));
    INIT_MSG("Reg[70] EMMC_CFG0      = 0x%.8x", sdr_read32(base + 0x70));                        
    INIT_MSG("Reg[74] EMMC_CFG1      = 0x%.8x", sdr_read32(base + 0x74));
    INIT_MSG("Reg[78] EMMC_STS       = 0x%.8x", sdr_read32(base + 0x78));
    INIT_MSG("Reg[7C] EMMC_IOCON     = 0x%.8x", sdr_read32(base + 0x7C));            
    INIT_MSG("Reg[80] SD_ACMD_RESP   = 0x%.8x", sdr_read32(base + 0x80));
    INIT_MSG("Reg[84] SD_ACMD19_TRG  = 0x%.8x", sdr_read32(base + 0x84));      
    INIT_MSG("Reg[88] SD_ACMD19_STS  = 0x%.8x", sdr_read32(base + 0x88));
    INIT_MSG("Reg[90] DMA_SA         = 0x%.8x", sdr_read32(base + 0x90));
    INIT_MSG("Reg[94] DMA_CA         = 0x%.8x", sdr_read32(base + 0x94));
    INIT_MSG("Reg[98] DMA_CTRL       = 0x%.8x", sdr_read32(base + 0x98));
    INIT_MSG("Reg[9C] DMA_CFG        = 0x%.8x", sdr_read32(base + 0x9C));                        
    INIT_MSG("Reg[A0] SW_DBG_SEL     = 0x%.8x", sdr_read32(base + 0xA0));
    INIT_MSG("Reg[A4] SW_DBG_OUT     = 0x%.8x", sdr_read32(base + 0xA4));
    INIT_MSG("Reg[B0] PATCH_BIT0     = 0x%.8x", sdr_read32(base + 0xB0));            
    INIT_MSG("Reg[B4] PATCH_BIT1     = 0x%.8x", sdr_read32(base + 0xB4));
    INIT_MSG("Reg[C0] DAT0_TUNE_CRC  = 0x%.8x", sdr_read32(base + 0xC0));
    INIT_MSG("Reg[C4] DAT1_TUNE_CRC  = 0x%.8x", sdr_read32(base + 0xC4));
    INIT_MSG("Reg[C8] DAT2_TUNE_CRC  = 0x%.8x", sdr_read32(base + 0xC8));
    INIT_MSG("Reg[CC] DAT3_TUNE_CRC  = 0x%.8x", sdr_read32(base + 0xCC));
    INIT_MSG("Reg[D0] CMD_TUNE_CRC   = 0x%.8x", sdr_read32(base + 0xD0));
    INIT_MSG("Reg[D4] SDIO_TUNE_WIND = 0x%.8x", sdr_read32(base + 0xD4));
    INIT_MSG("Reg[E0] SD_PAD_CTL0    = 0x%.8x", msdc_dump_padctl0(host->id));        
    INIT_MSG("Reg[E4] SD_PAD_CTL1    = 0x%.8x", msdc_dump_padctl1(host->id));
    INIT_MSG("Reg[E8] SD_PAD_CTL2    = 0x%.8x", msdc_dump_padctl2(host->id));
    INIT_MSG("Reg[EC] PAD_TUNE       = 0x%.8x", sdr_read32(base + 0xEC));
    INIT_MSG("Reg[F0] DAT_RD_DLY0    = 0x%.8x", sdr_read32(base + 0xF0));                        
    INIT_MSG("Reg[F4] DAT_RD_DLY1    = 0x%.8x", sdr_read32(base + 0xF4));
    INIT_MSG("Reg[F8] HW_DBG_SEL     = 0x%.8x", sdr_read32(base + 0xF8));
    INIT_MSG("Rg[100] MAIN_VER       = 0x%.8x", sdr_read32(base + 0x100));     
    INIT_MSG("Rg[104] ECO_VER        = 0x%.8x", sdr_read32(base + 0x104));                   
   
    /* dump gpio register */
    INIT_MSG("Rg[C10] MSDC0_CTRL0_CLK        = 0x%.8x", sdr_read32(0xF0005000 + 0xC00));                   
    INIT_MSG("Rg[C20] MSDC0_CTRL1_CMD        = 0x%.8x", sdr_read32(0xF0005000 + 0xC10));                   
    INIT_MSG("Rg[C30] MSDC0_CTRL2_DAT        = 0x%.8x", sdr_read32(0xF0005000 + 0xC20));                   
    INIT_MSG("Rg[C40] MSDC0_CTRL3_PAD        = 0x%.8x", sdr_read32(0xF0005000 + 0xC30));                   
    INIT_MSG("Rg[C50] MSDC1_CTRL0_CLK        = 0x%.8x", sdr_read32(0xF0005000 + 0xC40));                   
    INIT_MSG("Rg[C60] MSDC1_CTRL1_CMD        = 0x%.8x", sdr_read32(0xF0005000 + 0xC50));                   
    INIT_MSG("Rg[C70] MSDC1_CTRL2_DAT        = 0x%.8x", sdr_read32(0xF0005000 + 0xC60));                   
    INIT_MSG("Rg[C80] MSDC1_CTRL3_PAD        = 0x%.8x", sdr_read32(0xF0005000 + 0xC70));                   
} 


#if 1
static void msdc_debug_reg(struct msdc_host *host)
{
    u32 base = host->base;  
    u32 i; 

    for (i=0; i < 26; i++) {
        sdr_write32(base + 0xA0, i);    
        INIT_MSG("SW_DBG_SEL: write reg[%x] to 0x%x", base + 0xA0, i);      
        INIT_MSG("SW_DBG_OUT: read  reg[%x] to 0x%x", base + 0xA4, sdr_read32(base + 0xA4));    
    }    
    
    sdr_write32(base + 0xA0, 0);    
}
#endif

#ifndef FPGA_PLATFORM

#if (0 == REMOVEED_FOR_MT6582)
static void msdc_key_unlock(void)
{
    sdr_write8(EN18IOKEY_BASE,0x58);
    sdr_write8(EN18IOKEY_BASE,0xfa);
    sdr_write8(EN18IOKEY_BASE,0x65);
    sdr_write8(EN18IOKEY_BASE,0x83);
}
static void msdc_key_lock(void)
{
    sdr_write8(EN18IOKEY_BASE,0x0);
    sdr_write8(EN18IOKEY_BASE,0x0);
    sdr_write8(EN18IOKEY_BASE,0x0);
    sdr_write8(EN18IOKEY_BASE,0x0);
}

static void msdc_hw_compara_enable(int id)
{  
    MSDC_POWER_DOMAIN power_domain;
    int msdc_en18io_sel;
/*
#ifdef MSDC_VIO28_MC1
#define MSDC1_EN18IO_SEL_VAL  (0x1)
#endif

#ifdef MSDC_VIO18_MC1
#define MSDC1_EN18IO_SEL_VAL  (0x1)
#endif

#ifdef MSDC_VMC
#define MSDC1_EN18IO_SEL_VAL  (0x0)
#endif
#endif

#ifdef MSDC_VIO28_MC2
#define MSDC2_EN18IO_SEL1_VAL  (0x1)
#endif
#ifdef MSDC_VIO18_MC2
#define MSDC2_EN18IO_SEL1_VAL  (0x1)
#endif
#ifdef MSDC_VGP6
#define MSDC2_EN18IO_SEL1_VAL  (0x0)
#endif
*/
    switch(id){
        case 1:
            power_domain = MSDC_POWER_MC1;
            if(power_domain == MSDC_VMC)
                msdc_en18io_sel = 0;
            else
                msdc_en18io_sel = 1;
            msdc_key_unlock();
            sdr_set_field(MSDC_EN18IO_CMP_SEL_BASE,MSDC1_EN18IO_CMP_EN,0x1);
            sdr_set_field(MSDC_EN18IO_CMP_SEL_BASE,MSDC1_EN18IO_SEL1,msdc_en18io_sel);
            msdc_key_lock();
            break;
        case 2:
            power_domain = MSDC_POWER_MC2;
            if(power_domain == MSDC_VGP6)
                msdc_en18io_sel = 0;
            else
                msdc_en18io_sel = 1;
            msdc_key_unlock();
            sdr_set_field(MSDC_EN18IO_CMP_SEL_BASE,MSDC2_EN18IO_CMP_EN,0x1);
            sdr_set_field(MSDC_EN18IO_CMP_SEL_BASE,MSDC2_EN18IO_SEL,msdc_en18io_sel);
            msdc_key_lock();
            break;
        default:
            break;
        }
}
#endif /* end of REMOVEED_FOR_MT6582 */

#endif
/*
 * for AHB read / write debug
 * return DMA status. 
 */
int msdc_get_dma_status(int host_id)
{
    int result = -1;

    if(host_id < 0 || host_id >= HOST_MAX_NUM)
    {
        printk("[%s] failed to get dma status, bad host_id %d\n", __func__, host_id);
        return result;
    }

    if(msdc_latest_transfer_mode[host_id] == TRAN_MOD_DMA)
    {
        switch(msdc_latest_operation_type[host_id])
        {
            case OPER_TYPE_READ:
                result = 1; // DMA read
                break;
            case OPER_TYPE_WRITE:
                result = 2; // DMA write
                break;
            default:
                break;
        }
    }else if(msdc_latest_transfer_mode[host_id] == TRAN_MOD_PIO){
        result = 0; // PIO mode
    }
  
    return result;
}
EXPORT_SYMBOL(msdc_get_dma_status);

struct dma_addr* msdc_get_dma_address(int host_id)
{
    bd_t* bd; 
    int i = 0;
    int mode = -1;
    struct msdc_host *host;
    u32 base;
    if(host_id < 0 || host_id >= HOST_MAX_NUM)
    {
        printk("[%s] failed to get dma status, bad host_id %d\n", __func__, host_id);
        return NULL;
    }
 
    if(!mtk_msdc_host[host_id]) 
    {
        printk("[%s] failed to get dma status, msdc%d is not exist\n", __func__, host_id);
        return NULL;
    }
   
    host = mtk_msdc_host[host_id];
    base = host->base;
    //spin_lock(&host->lock);
    sdr_get_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_MODE, mode);
    if(mode == 1){
        printk(KERN_CRIT "Desc.DMA\n");
        bd = host->dma.bd;
        i = 0;
        while(i < MAX_BD_PER_GPD){
            msdc_latest_dma_address[i].start_address = (u32)bd[i].ptr;
            msdc_latest_dma_address[i].size = bd[i].buflen; 
            msdc_latest_dma_address[i].end = bd[i].eol;
            if(i>0)
                msdc_latest_dma_address[i-1].next = &msdc_latest_dma_address[i];

            if(bd[i].eol){
                break;
            }
            i++;
        }
    }
    else if(mode == 0){
        printk(KERN_CRIT "Basic DMA\n");
        msdc_latest_dma_address[i].start_address = sdr_read32(MSDC_DMA_SA);
        sdr_get_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_XFERSZ, msdc_latest_dma_address[i].size);
        msdc_latest_dma_address[i].end = 1;
    }
  
    //spin_unlock(&host->lock);

    return msdc_latest_dma_address;

}
EXPORT_SYMBOL(msdc_get_dma_address);

static void msdc_dump_clock_sts(struct msdc_host* host)
{
    //INIT_MSG("MSDC pll status: Reg[0xF00071C0] = 0x%.8x\r\n", sdr_read32(0xF00071C0));      
    
#ifndef FPGA_PLATFORM
    //freq_meter(0xf, 0); 
    //INIT_MSG("clock select reg[%x] = 0x%.8x",  MSDC_CLKSRC_REG, sdr_read32(MSDC_CLKSRC_REG));      
    INIT_MSG("clock gate status reg[F0003018] = 0x%.8x", sdr_read32(0xF0003018));
    //INIT_MSG("clock mgr.clock_state[0] = 0x%x",clk_mgr.clock_state[0]);
#endif
}
void msdc_dump_info(u32 id)
{
    struct msdc_host *host = mtk_msdc_host[id];
    u32 base;
    u32 temp;

    //return;
        
    if(host == NULL) {
        printk("msdc host<%d> null\r\n", id);  
        return;
    }             
    base = host->base;       

    // 1: dump msdc hw register   
    msdc_dump_register(host);
    INIT_MSG("latest_INT_status<0x%.8x>",latest_int_status[id]);
    // 2: check msdc clock gate and clock source
    msdc_dump_clock_sts(host);

    // 3: For designer 
    msdc_debug_reg(host);

    // 4: check the register read_write 
    temp = sdr_read32(base + 0xB0);
    INIT_MSG("patch reg[%x] = 0x%.8x", (base + 0xB0), temp);  
      
    temp = (~temp);
    sdr_write32(base + 0xB0, temp);    
    temp = sdr_read32(base + 0xB0);
    INIT_MSG("patch reg[%x] = 0x%.8x second time", (base + 0xB0), temp);          
 
    temp = (~temp);
    sdr_write32(base + 0xB0, temp);    
    temp = sdr_read32(base + 0xB0);
    INIT_MSG("patch reg[%x] = 0x%.8x Third time", (base + 0xB0), temp);   
        
    //sd_debug_zone[id] = 0x3ff;        
}
EXPORT_SYMBOL(msdc_dump_info);

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
            msdc_dump_info(id); \
        } \
        WARN_ON(retry == 0); \
    } while(0)

#define msdc_reset(id) \
    do { \
        int retry = 3, cnt = 1000; \
        sdr_set_bits(MSDC_CFG, MSDC_CFG_RST); \
        mb(); \
        msdc_retry(sdr_read32(MSDC_CFG) & MSDC_CFG_RST, retry, cnt,id); \
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
        msdc_retry(sdr_read32(MSDC_FIFOCS) & MSDC_FIFOCS_CLR, retry, cnt,id); \
    } while(0)

#define msdc_reset_hw(id) \
    msdc_reset(id); \
    msdc_clr_fifo(id); \
    msdc_clr_int(); 

static int msdc_clk_stable(struct msdc_host *host,u32 mode, u32 div){
    u32 base = host->base;
        int retry = 0;
    int cnt = 1000; 
    int retry_cnt = 1;
    do{
        retry = 3;
        sdr_set_field(MSDC_CFG, MSDC_CFG_CKMOD|MSDC_CFG_CKDIV,(mode << 8)|((div + retry_cnt) % 0xff)); 
        //sdr_set_field(MSDC_CFG, MSDC_CFG_CKMOD, mode); 
        msdc_retry(!(sdr_read32(MSDC_CFG) & MSDC_CFG_CKSTB), retry, cnt,host->id);
        if(retry == 0){
            printk(KERN_ERR "msdc%d host->onclock(%d)\n",host->id,host->core_clkon);
            printk(KERN_ERR "msdc%d on clock failed ===> retry twice\n",host->id);
#ifndef FPGA_PLATFORM
            disable_clock(MT_CG_PERI_MSDC30_0 + host->id, "SD");
            enable_clock(MT_CG_PERI_MSDC30_0 + host->id, "SD");
#endif
            msdc_dump_info(host->id);
        }
        retry = 3;
        sdr_set_field(MSDC_CFG, MSDC_CFG_CKDIV, div); 
        msdc_retry(!(sdr_read32(MSDC_CFG) & MSDC_CFG_CKSTB), retry, cnt,host->id);
        if(retry == 0)
            msdc_dump_info(host->id);
        msdc_reset_hw(host->id);
        if(retry_cnt == 2)
            break;
        retry_cnt += 1;
    }while(!retry);
    return 0;
}

#define msdc_irq_save(val) \
    do { \
        val = sdr_read32(MSDC_INTEN); \
        sdr_clr_bits(MSDC_INTEN, val); \
    } while(0)
  
#define msdc_irq_restore(val) \
    do { \
        sdr_set_bits(MSDC_INTEN, val); \
    } while(0)

/* clock source for host: global */
//static u32 hclks[] = {26000000, 197000000, 208000000, 0};
// 50M -> 26/4 = 6.25 MHz
#ifdef FPGA_PLATFORM
static u32 hclks[] = {12000001, 12000000, 12000000, 0};
#else
static u32 hclks[] = {200000000, 200000001, 197000000, 0};
#endif
/* VMCH is for T-card main power.
 * VMC for T-card when no emmc, for eMMC when has emmc. 
 * VGP for T-card when has emmc.
 */
u32 g_msdc0_io     = 0;
u32 g_msdc0_flash  = 0;
u32 g_msdc1_io     = 0; 
u32 g_msdc1_flash  = 0;
u32 g_msdc2_io     = 0;
u32 g_msdc2_flash  = 0;
u32 g_msdc3_io     = 0;
u32 g_msdc3_flash  = 0;
u32 g_msdc4_io     = 0;
u32 g_msdc4_flash  = 0;

#ifdef FPGA_PLATFORM
#if 0
static u32 msdc_ldo_power(u32 on, int powerId, int powerVolt, u32 *status){
    if (on) { // want to power on
        if (*status == 0) {  // can power on 
            printk(KERN_ERR "msdc LDO<%d> power on<%d>\n", powerId, powerVolt);     
            hwPowerOn_fpga(); //powerId, powerVolt, "msdc");
            *status = powerVolt;             
        } else if (*status == powerVolt) {
            printk(KERN_ERR "LDO<%d><%d> power on again!\n", powerId, powerVolt);    
        } else { // for sd3.0 later
            printk(KERN_ERR "LDO<%d> change<%d> to <%d>\n", powerId, *status, powerVolt);
            hwPowerDown_fpga(); //(powerId, "msdc");
            hwPowerOn_fpga(); //powerId, powerVolt, "msdc");
            *status = powerVolt;    
        }
    } else {  // want to power off
        if (*status != 0) {  // has been powerred on
            printk(KERN_ERR "msdc LDO<%d> power off\n", powerId);    
            hwPowerDown_fpga(); //powerId, "msdc");
            *status = 0;
        } else {
            printk(KERN_ERR "LDO<%d> not power on\n", powerId); 
        }                
    }  
        
    return 0;     

}
#endif
#else

static u32 msdc_ldo_power(u32 on, MT65XX_POWER powerId, MT65XX_POWER_VOLTAGE powerVolt, u32 *status)
{
    if (on) { // want to power on
        if (*status == 0) {  // can power on 
            printk(KERN_WARNING "msdc LDO<%d> power on<%d>\n", powerId, powerVolt);   
            hwPowerOn(powerId, powerVolt, "msdc");
            *status = powerVolt;             
        } else if (*status == powerVolt) {
            printk(KERN_ERR "msdc LDO<%d><%d> power on again!\n", powerId, powerVolt);  
        } else { // for sd3.0 later
            printk(KERN_WARNING "msdc LDO<%d> change<%d> to <%d>\n", powerId, *status, powerVolt);
            hwPowerDown(powerId, "msdc");
            hwPowerOn(powerId, powerVolt, "msdc");
            *status = powerVolt;  
        }
    } else {  // want to power off
        if (*status != 0) {  // has been powerred on
            printk(KERN_WARNING "msdc LDO<%d> power off\n", powerId);   
            hwPowerDown(powerId, "msdc");
            *status = 0;
        } else {
            printk(KERN_ERR "LDO<%d> not power on\n", powerId);  
        }              
    }  
    
    return 0;        
}

// maintain the Power ID internal   
#if (0 == REMOVEED_FOR_MT6582)
static void msdc_set_enio18(struct msdc_host *host,u32 v18)
{
}
#endif

void msdc_set_tune(struct msdc_host *host,u32 value)
{
}

u32 msdc_get_tune(struct msdc_host *host)
{
    return 0;
}

void msdc_set_sr(struct msdc_host *host,int clk,int cmd, int dat)
{
    switch(host->id){
        case 0:
            sdr_set_field(MSDC0_GPIO_CLK_BASE, GPIO_SR_MASK, clk);
            sdr_set_field(MSDC0_GPIO_CMD_BASE, GPIO_SR_MASK, cmd);
            sdr_set_field(MSDC0_GPIO_DAT_BASE, GPIO_SR_MASK, dat);
            break;
        case 1:
            sdr_set_field(MSDC1_GPIO_CLK_BASE, GPIO_SR_MASK, clk);
            sdr_set_field(MSDC1_GPIO_CMD_BASE, GPIO_SR_MASK, cmd);
            sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_SR_MASK, dat);
            break;
        case 2:
            sdr_set_field(MSDC2_GPIO_CLK_BASE, GPIO_SR_MASK, clk);
            sdr_set_field(MSDC2_GPIO_CMD_BASE, GPIO_SR_MASK, cmd);
            sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_SR_MASK, dat);
            break;
        default:
            break;
    }
}

void msdc_set_rdtdsel_dbg(struct msdc_host *host,bool rdsel,u32 value)
{
    if(rdsel){
    switch(host->id){
        case 0:
            sdr_set_field(MSDC0_GPIO_PAD_BASE, GPIO_PAD_RDSEL_MASK, value);
            break;
        case 1:
            sdr_set_field(MSDC1_GPIO_PAD_BASE, GPIO_PAD_RDSEL_MASK, value);
            break;
        case 2:
            sdr_set_field(MSDC2_GPIO_PAD_BASE, GPIO_PAD_RDSEL_MASK, value);
            break;
        default:
            break;
        }
    } else{
        switch(host->id){
        case 0:
            sdr_set_field(MSDC0_GPIO_PAD_BASE, GPIO_PAD_TDSEL_MASK, value);
            break;
        case 1:
            sdr_set_field(MSDC1_GPIO_PAD_BASE, GPIO_PAD_TDSEL_MASK, value);
            break;
        case 2:
            sdr_set_field(MSDC2_GPIO_PAD_BASE, GPIO_PAD_TDSEL_MASK, value);
            break;
        default:
            break;
        }
        }
}

void msdc_set_rdtdsel(struct msdc_host *host,bool sd_18)
{
    /* need ask the IO designer for value */
    /* rdsel and tdsel is suggest to set 0 under MT6582 from DE */
    switch(host->id){
        case 0:
            sdr_set_field(MSDC0_GPIO_PAD_BASE, GPIO_PAD_RDSEL_MASK, 0x0);
            sdr_set_field(MSDC0_GPIO_PAD_BASE, GPIO_PAD_TDSEL_MASK, 0x0);
            break;
        case 1:
            if(sd_18){
                sdr_set_field(MSDC1_GPIO_PAD_BASE, GPIO_PAD_RDSEL_MASK, 0x0);
                sdr_set_field(MSDC1_GPIO_PAD_BASE, GPIO_PAD_TDSEL_MASK, 0x0);
            } else{
                sdr_set_field(MSDC1_GPIO_PAD_BASE, GPIO_PAD_RDSEL_MASK, 0x0);
                sdr_set_field(MSDC1_GPIO_PAD_BASE, GPIO_PAD_TDSEL_MASK, 0x0);
            }
            break;
        case 2:
            sdr_set_field(MSDC2_GPIO_PAD_BASE, GPIO_PAD_RDSEL_MASK, 0x0);
            sdr_set_field(MSDC2_GPIO_PAD_BASE, GPIO_PAD_TDSEL_MASK, 0x0);
            break;
        default:
            break;
    }
}

static void msdc_pin_pud(struct msdc_host *host, int mode)
{
    switch(host->id){
        case 0:
            switch (mode) {
                case MSDC0_PULL_NONE:
                    sdr_set_field(MSDC0_GPIO_CMD_BASE, GPIO_R0_MASK, 0);
                    sdr_set_field(MSDC0_GPIO_CMD_BASE, GPIO_R1_MASK, 0);
                    //sdr_set_field(MSDC0_GPIO_CMD_BASE, GPIO_PUPD_MASK, 0);
                    
                    sdr_set_field(MSDC0_GPIO_DAT_BASE, GPIO_R0_MASK, 0);
                    sdr_set_field(MSDC0_GPIO_DAT_BASE, GPIO_R1_MASK, 0);
                    //sdr_set_field(MSDC0_GPIO_DAT_BASE, GPIO_PUPD_MASK, 0);
                    break;
                case MSDC0_PU_10K:
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
                case MSDC0_PU_50K:
                    sdr_set_field(MSDC0_GPIO_CMD_BASE, GPIO_R0_MASK, 0);
                    sdr_set_field(MSDC0_GPIO_CMD_BASE, GPIO_R1_MASK, 1);
                    sdr_set_field(MSDC0_GPIO_CMD_BASE, GPIO_PUPD_MASK, 0);
                    
                    sdr_set_field(MSDC0_GPIO_DAT_BASE, GPIO_R0_MASK, 0);
                    sdr_set_field(MSDC0_GPIO_DAT_BASE, GPIO_R1_MASK, 1);
                    sdr_set_field(MSDC0_GPIO_DAT_BASE, GPIO_PUPD_MASK, 0);
                    break;
                case MSDC0_PU_8K:
                    sdr_set_field(MSDC0_GPIO_CMD_BASE, GPIO_R0_MASK, 1);
                    sdr_set_field(MSDC0_GPIO_CMD_BASE, GPIO_R1_MASK, 1);
                    sdr_set_field(MSDC0_GPIO_CMD_BASE, GPIO_PUPD_MASK, 0);
                    
                    sdr_set_field(MSDC0_GPIO_DAT_BASE, GPIO_R0_MASK, 1);
                    sdr_set_field(MSDC0_GPIO_DAT_BASE, GPIO_R1_MASK, 1);
                    sdr_set_field(MSDC0_GPIO_DAT_BASE, GPIO_PUPD_MASK, 0);
                    break;
                case MSDC0_PD_10K:
                    sdr_set_field(MSDC0_GPIO_CMD_BASE, GPIO_R0_MASK, 1);
                    sdr_set_field(MSDC0_GPIO_CMD_BASE, GPIO_R1_MASK, 0);
                    sdr_set_field(MSDC0_GPIO_CMD_BASE, GPIO_PUPD_MASK, 1);
                    
                    sdr_set_field(MSDC0_GPIO_DAT_BASE, GPIO_R0_MASK, 1);
                    sdr_set_field(MSDC0_GPIO_DAT_BASE, GPIO_R1_MASK, 0);
                    sdr_set_field(MSDC0_GPIO_DAT_BASE, GPIO_PUPD_MASK, 1);
                    break;
                case MSDC0_PD_50K:
                    sdr_set_field(MSDC0_GPIO_CMD_BASE, GPIO_R0_MASK, 0);
                    sdr_set_field(MSDC0_GPIO_CMD_BASE, GPIO_R1_MASK, 1);
                    sdr_set_field(MSDC0_GPIO_CMD_BASE, GPIO_PUPD_MASK, 1);
                    
                    sdr_set_field(MSDC0_GPIO_DAT_BASE, GPIO_R0_MASK, 0);
                    sdr_set_field(MSDC0_GPIO_DAT_BASE, GPIO_R1_MASK, 1);
                    sdr_set_field(MSDC0_GPIO_DAT_BASE, GPIO_PUPD_MASK, 1);
                    break;
                case MSDC0_PD_8K:
                    sdr_set_field(MSDC0_GPIO_CMD_BASE, GPIO_R0_MASK, 1);
                    sdr_set_field(MSDC0_GPIO_CMD_BASE, GPIO_R1_MASK, 1);
                    sdr_set_field(MSDC0_GPIO_CMD_BASE, GPIO_PUPD_MASK, 1);
                    
                    sdr_set_field(MSDC0_GPIO_DAT_BASE, GPIO_R0_MASK, 1);
                    sdr_set_field(MSDC0_GPIO_DAT_BASE, GPIO_R1_MASK, 1);
                    sdr_set_field(MSDC0_GPIO_DAT_BASE, GPIO_PUPD_MASK, 1);
                    break;
                default:
                    /* error mode! */
                    printk("\t[msdc_pin_pud] host0 mode error(%d)!\n", mode);
                    break;
            }
            break;
        case 1:
            if (MSDC1_PU_50K == mode){
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
            } else if (MSDC1_PD_50K == mode){
                sdr_set_field(MSDC1_GPIO_CMD_BASE, GPIO_PU_MASK, 0);
                sdr_set_field(MSDC1_GPIO_CMD_BASE, GPIO_PD_MASK, 1);
                
                sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT0_PU_MASK, 0);
                sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT0_PD_MASK, 1);

                sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT1_PU_MASK, 0);
                sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT1_PD_MASK, 1);

                sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT2_PU_MASK, 0);
                sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT2_PD_MASK, 1);

                sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT3_PU_MASK, 0);
                sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT3_PD_MASK, 1);
            } else if (MSDC1_PULL_NONE == mode){
                sdr_set_field(MSDC1_GPIO_CMD_BASE, GPIO_PU_MASK, 0);
                sdr_set_field(MSDC1_GPIO_CMD_BASE, GPIO_PD_MASK, 0);
                
                sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT0_PU_MASK, 0);
                sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT0_PD_MASK, 0);

                sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT1_PU_MASK, 0);
                sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT1_PD_MASK, 0);

                sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT2_PU_MASK, 0);
                sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT2_PD_MASK, 0);

                sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT3_PU_MASK, 0);
                sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT3_PD_MASK, 0);
            } else {
                /* error mode! */
                printk("\t[msdc_pin_pud] host1 mode error(%d)!\n", mode);
            }
            break;
        case 2:
            if (MSDC2_PU_50K == mode){
                sdr_set_field(MSDC2_GPIO_CMD_BASE, GPIO_PU_MASK, 1);
                sdr_set_field(MSDC2_GPIO_CMD_BASE, GPIO_PD_MASK, 0);
                
                sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT0_PU_MASK, 1);
                sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT0_PD_MASK, 0);

                sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT1_PU_MASK, 1);
                sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT1_PD_MASK, 0);

                sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT2_PU_MASK, 1);
                sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT2_PD_MASK, 0);

                sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT3_PU_MASK, 1);
                sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT3_PD_MASK, 0);
                
                /* clock pull down during card init */
                sdr_set_field(MSDC2_GPIO_CLK_BASE, GPIO_PU_MASK, 0);
                sdr_set_field(MSDC2_GPIO_CLK_BASE, GPIO_PD_MASK, 1);
            } else if (MSDC2_PD_50K == mode){
                sdr_set_field(MSDC2_GPIO_CMD_BASE, GPIO_PU_MASK, 0);
                sdr_set_field(MSDC2_GPIO_CMD_BASE, GPIO_PD_MASK, 1);
                
                sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT0_PU_MASK, 0);
                sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT0_PD_MASK, 1);

                sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT1_PU_MASK, 0);
                sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT1_PD_MASK, 1);
                
                sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT2_PU_MASK, 0);
                sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT2_PD_MASK, 1);
                
                sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT3_PU_MASK, 0);
                sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT3_PD_MASK, 1);
            } else if (MSDC2_PULL_NONE == mode){
                sdr_set_field(MSDC2_GPIO_CMD_BASE, GPIO_PU_MASK, 0);
                sdr_set_field(MSDC2_GPIO_CMD_BASE, GPIO_PD_MASK, 0);
                
                sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT0_PU_MASK, 0);
                sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT0_PD_MASK, 0);
                
                sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT1_PU_MASK, 0);
                sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT1_PD_MASK, 0);
                
                sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT2_PU_MASK, 0);
                sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT2_PD_MASK, 0);
                
                sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT3_PU_MASK, 0);
                sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT3_PD_MASK, 0);
            } else {
                /* error mode! */
                printk("\t[msdc_pin_pud] host2 mode error(%d)!\n", mode);
            }
            break;
        default:
            break;
    }
}

static void msdc_pin_pnul(struct msdc_host *host, int mode)
{
    switch(host->id){
        case 1:
            sdr_set_field(MSDC1_GPIO_CMD_BASE, GPIO_PU_MASK, 0);
            sdr_set_field(MSDC1_GPIO_CMD_BASE, GPIO_PD_MASK, 0);

            sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT0_PU_MASK, 0);
            sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT0_PD_MASK, 0);

            sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT1_PU_MASK, 0);
            sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT1_PD_MASK, 0);

            sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT2_PU_MASK, 0);
            sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT2_PD_MASK, 0);

            sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT3_PU_MASK, 0);
            sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_DAT3_PD_MASK, 0);
            break;
        case 2:
            sdr_set_field(MSDC2_GPIO_CMD_BASE, GPIO_PU_MASK, 0);
            sdr_set_field(MSDC2_GPIO_CMD_BASE, GPIO_PD_MASK, 0);

            sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT0_PU_MASK, 0);
            sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT0_PD_MASK, 0);

            sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT1_PU_MASK, 0);
            sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT1_PD_MASK, 0);

            sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT2_PU_MASK, 0);
            sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT2_PD_MASK, 0);

            sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT3_PU_MASK, 0);
            sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_DAT3_PD_MASK, 0);
            break;
        default:
            break;
    }
}

void msdc_set_smt(struct msdc_host *host,int set_smt)
{
    switch(host->id){
        case 0:
            sdr_set_field(MSDC0_GPIO_CLK_BASE, GPIO_SMT_MASK, set_smt);
            sdr_set_field(MSDC0_GPIO_CMD_BASE, GPIO_SMT_MASK, set_smt);
            sdr_set_field(MSDC0_GPIO_DAT_BASE, GPIO_SMT_MASK, set_smt); 
            break;
        case 1:
            sdr_set_field(MSDC1_GPIO_CLK_BASE, GPIO_SMT_MASK, set_smt);
            sdr_set_field(MSDC1_GPIO_CMD_BASE, GPIO_SMT_MASK, set_smt);
            sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_SMT_MASK, set_smt); 
            break;
        case 2:
            sdr_set_field(MSDC2_GPIO_CLK_BASE, GPIO_SMT_MASK, set_smt);
            sdr_set_field(MSDC2_GPIO_CMD_BASE, GPIO_SMT_MASK, set_smt);
            sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_SMT_MASK, set_smt); 
            break;
        default:
            break;
    }
}

/* host can modify from 0-7 */
void msdc_set_driving(struct msdc_host *host,struct msdc_hw *hw,bool sd_18)
{
    switch(host->id){
        case 0:
            sdr_set_field(MSDC0_GPIO_CLK_BASE, GPIO_MSDC0_DRVN, hw->clk_drv);
            sdr_set_field(MSDC0_GPIO_CMD_BASE, GPIO_MSDC0_DRVN, hw->cmd_drv);
            sdr_set_field(MSDC0_GPIO_DAT_BASE, GPIO_MSDC0_DRVN, hw->dat_drv);
            break;
        case 1:
            if(sd_18){
                sdr_set_field(MSDC1_GPIO_CLK_BASE, GPIO_MSDC1_MSDC2_DRVN, hw->clk_drv_sd_18);
                sdr_set_field(MSDC1_GPIO_CMD_BASE, GPIO_MSDC1_MSDC2_DRVN, hw->cmd_drv_sd_18);
                sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_MSDC1_MSDC2_DRVN, hw->dat_drv_sd_18);
            } else{
                sdr_set_field(MSDC1_GPIO_CLK_BASE, GPIO_MSDC1_MSDC2_DRVN, hw->clk_drv);
                sdr_set_field(MSDC1_GPIO_CMD_BASE, GPIO_MSDC1_MSDC2_DRVN, hw->cmd_drv);
                sdr_set_field(MSDC1_GPIO_DAT_BASE, GPIO_MSDC1_MSDC2_DRVN, hw->dat_drv);
            }
            break;
        case 2:
            sdr_set_field(MSDC2_GPIO_CLK_BASE, GPIO_MSDC1_MSDC2_DRVN, hw->clk_drv);
            sdr_set_field(MSDC2_GPIO_CMD_BASE, GPIO_MSDC1_MSDC2_DRVN, hw->cmd_drv);
            sdr_set_field(MSDC2_GPIO_DAT_BASE, GPIO_MSDC1_MSDC2_DRVN, hw->dat_drv);
            break;
        default:
            break;
    }
}

#ifdef MTK_MT8193_SUPPORT
static void msdc_emmc_power_8193(struct msdc_host *host,u32 on)
{
	unsigned long tmo = 0; 
	u32 base = host->base;

	/* if MMC_CAP_WAIT_WHILE_BUSY not set, mmc core layer will loop for wait sa_timeout */
	if (host->mmc && host->mmc->card && (host->mmc->caps & MMC_CAP_WAIT_WHILE_BUSY) && (on == 0)){
		/* max timeout: 1000ms */
		if ((DIV_ROUND_UP(host->mmc->card->ext_csd.sa_timeout, 10000)) < 1000){	
			if (100 == HZ){
				/* one tick equal to 10ms while (HZ == 100) */
				tmo = jiffies + DIV_ROUND_UP(host->mmc->card->ext_csd.sa_timeout, 100000);
			} else if (1000 == HZ) {
				/* one tick equal to 1ms while (HZ == 1000) */
				tmo = jiffies + DIV_ROUND_UP(host->mmc->card->ext_csd.sa_timeout, 10000);
			} else {
				tmo = jiffies + HZ;
			}
		} else {
			tmo = jiffies + HZ;
		}

		while ((sdr_read32(MSDC_PS) & 0x10000) != 0x10000) {
			if (time_after(jiffies, tmo)){
				ERR_MSG("Dat0 keep low before power off, sa_timeout = 0x%x\n", host->mmc->card->ext_csd.sa_timeout); 
				break; 
			}
		}
	}

}
#else
static void msdc_emmc_power(struct msdc_host *host,u32 on)
{
    unsigned long tmo = 0; 
    u32 base = host->base;

    /* if MMC_CAP_WAIT_WHILE_BUSY not set, mmc core layer will loop for wait sa_timeout */
    if (host->mmc && host->mmc->card && (host->mmc->caps & MMC_CAP_WAIT_WHILE_BUSY) && (on == 0)){
        /* max timeout: 1000ms */
        if ((DIV_ROUND_UP(host->mmc->card->ext_csd.sa_timeout, 10000)) < 1000){  
            if (100 == HZ){
                /* one tick equal to 10ms while (HZ == 100) */
                tmo = jiffies + DIV_ROUND_UP(host->mmc->card->ext_csd.sa_timeout, 100000);
            } else if (1000 == HZ) {
                /* one tick equal to 1ms while (HZ == 1000) */
                tmo = jiffies + DIV_ROUND_UP(host->mmc->card->ext_csd.sa_timeout, 10000);
            } else {
                tmo = jiffies + HZ;
            }
        } else {
            tmo = jiffies + HZ;
        }

        while ((sdr_read32(MSDC_PS) & 0x10000) != 0x10000) {
            if (time_after(jiffies, tmo)){
                ERR_MSG("Dat0 keep low before power off, sa_timeout = 0x%x\n", host->mmc->card->ext_csd.sa_timeout); 
                break; 
            }
        }
    }
  
    switch(host->id){
        case 0:
            msdc_set_smt(host,1);
            //msdc_ldo_power(on, MT6323_POWER_LDO_VIO18, VOL_1800, &g_msdc0_io);

            /* ask for used MT6323** instead of MT65XX */
            //msdc_ldo_power(on, MT65XX_POWER_LDO_VEMC_3V3, VOL_3300, &g_msdc0_flash);
            msdc_ldo_power(on, MT6323_POWER_LDO_VEMC_3V3, VOL_3300, &g_msdc0_flash);
            break;
#if (0 == REMOVEED_FOR_MT6582)
        case 4:
            msdc_set_smt(host,1);
            msdc_ldo_power(on, MT65XX_POWER_LDO_VEMC_1V8, VOL_1800, &g_msdc4_io);
            msdc_ldo_power(on, MT65XX_POWER_LDO_VEMC_3V3, VOL_3300, &g_msdc4_flash);//check me
            break;
#endif
        default:
            break;
    }

}
#endif /* end of MTK_MT8193_SUPPORT */

static void msdc_sd_power(struct msdc_host *host,u32 on)
{
  
    switch(host->id){
        case 1:
            msdc_set_smt(host,1);
            msdc_set_driving(host,host->hw,0);
            msdc_set_rdtdsel(host,0);
            //msdc_set_enio18(host,0);
            //msdc_ldo_power(on, MT65XX_POWER_LDO_VMC1, VOL_3300, &g_msdc1_io);
            msdc_ldo_power(on, MT6323_POWER_LDO_VMC, VOL_3300, &g_msdc1_io);
            //msdc_set_enio18(host,0);
            //msdc_ldo_power(on, MT65XX_POWER_LDO_VMCH1, VOL_3300, &g_msdc1_flash);
            msdc_ldo_power(on, MT6323_POWER_LDO_VMCH, VOL_3300, &g_msdc1_flash);
            break;
#if (0 == REMOVEED_FOR_MT6582)
        case 2:
            msdc_set_smt(host,1);
            msdc_set_driving(host,host->hw,0);
            msdc_set_rdtdsel(host,0);
            //msdc_set_enio18(host,0);
            msdc_ldo_power(on, MT65XX_POWER_LDO_VGP6, VOL_3300, &g_msdc2_io);//If MSDC2 is defined for 2nd SD card host power and card power is the same one--VGP6
            //msdc_set_enio18(host,0);
            g_msdc2_flash = g_msdc2_io;
            //msdc_ldo_power(on, MT65XX_POWER_LDO_VGP6, VOL_3300, &g_msdc2_flash);//Only for SD2.0,customer need use external LDO to support SD3.0
            break;
#endif
        default:
            break;
    }
}

static void msdc_sd_power_switch(struct msdc_host *host,u32 on)
{
    switch(host->id){
        case 1:
            //msdc_ldo_power(on, MT65XX_POWER_LDO_VMC1, VOL_1800, &g_msdc1_io);
            msdc_ldo_power(on, MT6323_POWER_LDO_VMC, VOL_1800, &g_msdc1_io);
            //msdc_set_enio18(host,1);
            msdc_set_smt(host,1);
            msdc_set_rdtdsel(host,1);
            msdc_set_driving(host,host->hw,1);
            break;
#if (0 == REMOVEED_FOR_MT6582)
        case 2:
            msdc_ldo_power(on, MT65XX_POWER_LDO_VGP6, VOL_1800, &g_msdc2_io);
            //msdc_set_enio18(host,1);
            msdc_set_smt(host,1);
            msdc_set_rdtdsel(host,1);
            msdc_set_driving(host,host->hw,1);
            break;
#endif
        default:
            break;
    }
}

static void msdc_sdio_power(struct msdc_host *host,u32 on)
{
    switch(host->id){
        case 2:
            if(MSDC_VIO18_MC2 == host->power_domain) {
                if(on) {
#if 0 //CHECK ME
                    msdc_ldo_power(on, MT65XX_POWER_LDO_VGP6, VOL_1800, &g_msdc2_io); // Bus & device keeps 1.8v
#endif
                    //msdc_set_enio18(host,1);
                } else {
#if 0 //CHECK ME
                    msdc_ldo_power(on, MT65XX_POWER_LDO_VGP6, VOL_3300, &g_msdc2_io); // Bus & device keeps 3.3v
#endif
                    //msdc_set_enio18(host,0);
                }

            } else if(MSDC_VIO28_MC2 == host->power_domain) {
                msdc_ldo_power(on, MT65XX_POWER_LDO_VIO28, VOL_2800, &g_msdc2_io); // Bus & device keeps 2.8v
            }
            g_msdc2_flash = g_msdc2_io;
            break;
        default:    // if host_id is 3, it uses default 1.8v setting, which always turns on
            break;
    }

}

static void msdc_sdio_power_switch(struct msdc_host *host,u32 on)
{
    N_MSG(CFG, "When connect to MTK SDIO 3.0 Device, The CMD11 will be performed but no need to change voltage really. ");    
}

#endif /* end of FPGA_PLATFORM */

#define sdc_is_busy()          (sdr_read32(SDC_STS) & SDC_STS_SDCBUSY)
#define sdc_is_cmd_busy()      (sdr_read32(SDC_STS) & SDC_STS_CMDBUSY)

#define sdc_send_cmd(cmd,arg) \
    do { \
        sdr_write32(SDC_ARG, (arg)); \
        mb(); \
        sdr_write32(SDC_CMD, (cmd)); \
    } while(0)

// can modify to read h/w register.
//#define is_card_present(h)   ((sdr_read32(MSDC_PS) & MSDC_PS_CDSTS) ? 0 : 1);
#define is_card_present(h)     (((struct msdc_host*)(h))->card_inserted)
#define is_card_sdio(h)        (((struct msdc_host*)(h))->hw->register_pm)     

typedef enum{
    cmd_counter = 0,
    read_counter,
    write_counter,
    all_counter,
}TUNE_COUNTER;

static void msdc_reset_pwr_cycle_counter(struct msdc_host *host)
{
    host->power_cycle = 0;
    host->power_cycle_enable = 1;
}

static void msdc_reset_tmo_tune_counter(struct msdc_host *host,TUNE_COUNTER index)
{
    if(index >= 0 && index <= all_counter)
    {
        switch (index)
        {
            case cmd_counter: 
                if(host->rwcmd_time_tune != 0)
                    ERR_MSG("TMO TUNE CMD Times(%d)", host->rwcmd_time_tune);
                host->rwcmd_time_tune = 0;
                break;
            case read_counter  : 
                if(host->read_time_tune != 0)
                    ERR_MSG("TMO TUNE READ Times(%d)", host->read_time_tune);
                host->read_time_tune = 0;
                break;
            case write_counter : 
                if(host->write_time_tune != 0)
                    ERR_MSG("TMO TUNE WRITE Times(%d)", host->write_time_tune);
                host->write_time_tune = 0;
                break;
            case all_counter   : 
                if(host->rwcmd_time_tune != 0)
                    ERR_MSG("TMO TUNE CMD Times(%d)", host->rwcmd_time_tune);
                if(host->read_time_tune != 0)
                    ERR_MSG("TMO TUNE READ Times(%d)", host->read_time_tune);
                if(host->write_time_tune != 0)
                    ERR_MSG("TMO TUNE WRITE Times(%d)", host->write_time_tune);
                host->rwcmd_time_tune = 0;
                host->read_time_tune = 0;
                host->write_time_tune = 0;
                break;
            default : 
                break;
        }
    }else{
        ERR_MSG("msdc%d ==> reset tmo counter index(%d) error!\n",host->id,index);
    }
}
static void msdc_reset_crc_tune_counter(struct msdc_host *host,TUNE_COUNTER index)
{
    if(index >= 0 && index <= all_counter)
    {
        switch (index)
        {
            case cmd_counter: 
                if(host->t_counter.time_cmd != 0)
                    ERR_MSG("CRC TUNE CMD Times(%d)", host->t_counter.time_cmd);
                host->t_counter.time_cmd = 0;
                break;
            case read_counter  : 
                if(host->t_counter.time_read != 0)
                    ERR_MSG("CRC TUNE READ Times(%d)", host->t_counter.time_read);
                host->t_counter.time_read = 0;
                break;
            case write_counter : 
                if(host->t_counter.time_write != 0)
                    ERR_MSG("CRC TUNE WRITE Times(%d)", host->t_counter.time_write);
                host->t_counter.time_write = 0;
                break;
            case all_counter   : 
                if(host->t_counter.time_cmd != 0)
                    ERR_MSG("CRC TUNE CMD Times(%d)", host->t_counter.time_cmd);
                if(host->t_counter.time_read != 0)
                    ERR_MSG("CRC TUNE READ Times(%d)", host->t_counter.time_read);
                if(host->t_counter.time_write != 0)
                    ERR_MSG("CRC TUNE WRITE Times(%d)", host->t_counter.time_write);
                host->t_counter.time_cmd = 0;
                host->t_counter.time_read = 0;
                host->t_counter.time_write = 0;
                break;
            default : 
                break;
        }
    }else{
        ERR_MSG("msdc%d ==> reset crc counter index(%d) error!\n",host->id,index);
    }
}

extern void mmc_remove_card(struct mmc_card *card);

static void msdc_set_bad_card_and_remove(struct msdc_host *host)
{
    int got_polarity = 0;
    unsigned long flags;
    
    if(host == NULL) {
        ERR_MSG("WARN: host is NULL"); 
        return;
    }
    host->card_inserted = 0;
    
    if((host->mmc == NULL) || (host->mmc->card == NULL)) {
        ERR_MSG("WARN: mmc or card is NULL"); 
        return;        
    }
    if(host->mmc->card){
        spin_lock_irqsave(&host->remove_bad_card,flags);
        got_polarity = host->sd_cd_polarity;
        host->block_bad_card = 1;

        mmc_card_set_removed(host->mmc->card);
        spin_unlock_irqrestore(&host->remove_bad_card,flags);
        if(((host->hw->flags & MSDC_CD_PIN_EN) && (got_polarity ^ host->hw->cd_level)) || (!(host->hw->flags & MSDC_CD_PIN_EN)))
        {
            tasklet_hi_schedule(&host->card_tasklet);
        }
        else
        {
            mmc_remove_card(host->mmc->card);
        }
        
        ERR_MSG("Do remove the bad card, block_bad_card=%d, card_inserted=%d", host->block_bad_card, host->card_inserted);
    }
}

unsigned int msdc_do_command(struct msdc_host   *host, 
                                      struct mmc_command *cmd,
                                      int                 tune,
                                      unsigned long       timeout);  
  
static int msdc_tune_cmdrsp(struct msdc_host *host);
static int msdc_get_card_status(struct mmc_host *mmc, struct msdc_host *host, u32 *status);
static void msdc_clksrc_onoff(struct msdc_host *host, u32 on);

//host doesn't need the clock on
void msdc_gate_clock(struct msdc_host* host, int delay) 
{
    unsigned long flags; 

    spin_lock_irqsave(&host->clk_gate_lock, flags);
    if(host->clk_gate_count > 0)
        host->clk_gate_count--; 
    if(delay) 
    {
        mod_timer(&host->timer, jiffies + CLK_TIMEOUT);                    
        N_MSG(CLK, "[%s]: msdc%d, clk_gate_count=%d, delay=%d\n", __func__, host->id, host->clk_gate_count, delay);
    } else if(host->clk_gate_count == 0)
    {
        del_timer(&host->timer);
        msdc_clksrc_onoff(host, 0);
        N_MSG(CLK, "[%s]: msdc%d, successfully gate clock, clk_gate_count=%d, delay=%d\n", __func__, host->id, host->clk_gate_count, delay);
    }else {
        ERR_MSG("[%s]: msdc%d, failed to gate clock, the clock is still needed by host, clk_gate_count=%d, delay=%d\n", __func__, host->id, host->clk_gate_count, delay);
    }
    spin_unlock_irqrestore(&host->clk_gate_lock, flags); 
}

static void msdc_suspend_clock(struct msdc_host* host) 
{
    unsigned long flags; 

    spin_lock_irqsave(&host->clk_gate_lock, flags);
    if(host->clk_gate_count == 0)
    {
        del_timer(&host->timer);
        msdc_clksrc_onoff(host, 0);
        N_MSG(CLK, "[%s]: msdc%d, successfully gate clock, clk_gate_count=%d\n", __func__, host->id, host->clk_gate_count);
    } else {
        ERR_MSG("[%s]: msdc%d, the clock is still needed by host, clk_gate_count=%d\n", __func__, host->id, host->clk_gate_count);
    }
    spin_unlock_irqrestore(&host->clk_gate_lock, flags); 
}

//host does need the clock on
void msdc_ungate_clock(struct msdc_host* host) 
{ 
    unsigned long flags;
    spin_lock_irqsave(&host->clk_gate_lock, flags);
    host->clk_gate_count++; 
    N_MSG(CLK, "[%s]: msdc%d, clk_gate_count=%d\n", __func__, host->id, host->clk_gate_count);
    if(host->clk_gate_count == 1)
        msdc_clksrc_onoff(host, 1);        
    spin_unlock_irqrestore(&host->clk_gate_lock, flags); 
}  

// do we need sync object or not 
void msdc_clk_status(int * status)
{
    int g_clk_gate = 0;
    int i=0; 
    unsigned long flags; 

    for(i=0; i<HOST_MAX_NUM; i++)
    {
        if(!mtk_msdc_host[i])
            continue;
         
        spin_lock_irqsave(&mtk_msdc_host[i]->clk_gate_lock, flags);
        if(mtk_msdc_host[i]->clk_gate_count > 0)
#ifndef FPGA_PLATFORM
            g_clk_gate |= 1 << ((i) + MT_CG_PERI_MSDC30_0);
#endif
            spin_unlock_irqrestore(&mtk_msdc_host[i]->clk_gate_lock, flags);
    }
    *status = g_clk_gate;      
}

#if 0
static void msdc_dump_card_status(struct msdc_host *host, u32 status)
{
    static char *state[] = {
        "Idle",        /* 0 */
        "Ready",       /* 1 */
        "Ident",       /* 2 */
        "Stby",        /* 3 */
        "Tran",        /* 4 */
        "Data",        /* 5 */
        "Rcv",         /* 6 */
        "Prg",         /* 7 */
        "Dis",         /* 8 */
        "Reserved",    /* 9 */
        "Reserved",    /* 10 */
        "Reserved",    /* 11 */
        "Reserved",    /* 12 */
        "Reserved",    /* 13 */
        "Reserved",    /* 14 */
        "I/O mode",    /* 15 */
    };
    if (status & R1_OUT_OF_RANGE)
        N_MSG(RSP, "[CARD_STATUS] Out of Range");
    if (status & R1_ADDRESS_ERROR)
        N_MSG(RSP, "[CARD_STATUS] Address Error");
    if (status & R1_BLOCK_LEN_ERROR)
        N_MSG(RSP, "[CARD_STATUS] Block Len Error");
    if (status & R1_ERASE_SEQ_ERROR)
        N_MSG(RSP, "[CARD_STATUS] Erase Seq Error");
    if (status & R1_ERASE_PARAM)
        N_MSG(RSP, "[CARD_STATUS] Erase Param");
    if (status & R1_WP_VIOLATION)
        N_MSG(RSP, "[CARD_STATUS] WP Violation");
    if (status & R1_CARD_IS_LOCKED)
        N_MSG(RSP, "[CARD_STATUS] Card is Locked");
    if (status & R1_LOCK_UNLOCK_FAILED)
        N_MSG(RSP, "[CARD_STATUS] Lock/Unlock Failed");
    if (status & R1_COM_CRC_ERROR)
        N_MSG(RSP, "[CARD_STATUS] Command CRC Error");
    if (status & R1_ILLEGAL_COMMAND)
        N_MSG(RSP, "[CARD_STATUS] Illegal Command");
    if (status & R1_CARD_ECC_FAILED)
        N_MSG(RSP, "[CARD_STATUS] Card ECC Failed");
    if (status & R1_CC_ERROR)
        N_MSG(RSP, "[CARD_STATUS] CC Error");
    if (status & R1_ERROR)
        N_MSG(RSP, "[CARD_STATUS] Error");
    if (status & R1_UNDERRUN)
        N_MSG(RSP, "[CARD_STATUS] Underrun");
    if (status & R1_OVERRUN)
        N_MSG(RSP, "[CARD_STATUS] Overrun");
    if (status & R1_CID_CSD_OVERWRITE)
        N_MSG(RSP, "[CARD_STATUS] CID/CSD Overwrite");
    if (status & R1_WP_ERASE_SKIP)
        N_MSG(RSP, "[CARD_STATUS] WP Eraser Skip");
    if (status & R1_CARD_ECC_DISABLED)
        N_MSG(RSP, "[CARD_STATUS] Card ECC Disabled");
    if (status & R1_ERASE_RESET)
        N_MSG(RSP, "[CARD_STATUS] Erase Reset");
    if ((status & R1_READY_FOR_DATA) == 0)
        N_MSG(RSP, "[CARD_STATUS] Not Ready for Data");
    if (status & R1_SWITCH_ERROR)
        N_MSG(RSP, "[CARD_STATUS] Switch error");
    if (status & R1_APP_CMD)
        N_MSG(RSP, "[CARD_STATUS] App Command");
    
    N_MSG(RSP, "[CARD_STATUS] '%s' State", state[R1_CURRENT_STATE(status)]);
} 
#endif

static void msdc_set_timeout(struct msdc_host *host, u32 ns, u32 clks)
{
    u32 base = host->base;
    u32 timeout, clk_ns;

    host->timeout_ns = ns;
    host->timeout_clks = clks;

    clk_ns  = 1000000000UL / host->sclk;
    timeout = ns / clk_ns + clks;
    timeout = timeout >> 20; /* in 1048576 sclk cycle unit (83/85)*/
    timeout = timeout > 1 ? timeout - 1 : 0;
    timeout = timeout > 255 ? 255 : timeout;

    sdr_set_field(SDC_CFG, SDC_CFG_DTOC, timeout);

    N_MSG(OPS, "Set read data timeout: %dns %dclks -> %d x 1048576  cycles", ns, clks, timeout + 1);
}

/* msdc_eirq_sdio() will be called when EIRQ(for WIFI) */
static void msdc_eirq_sdio(void *data)
{
    struct msdc_host *host = (struct msdc_host *)data;

    N_MSG(INT, "SDIO EINT");
#ifdef SDIO_ERROR_BYPASS 
    if(host->sdio_error != -EIO){ 	
#endif      
		mmc_signal_sdio_irq(host->mmc);
#ifdef SDIO_ERROR_BYPASS 
    } 
#endif    
}

/* msdc_eirq_cd will not be used!  We not using EINT for card detection. */
static void msdc_eirq_cd(void *data)
{
    struct msdc_host *host = (struct msdc_host *)data;

    N_MSG(INT, "CD EINT");

    tasklet_hi_schedule(&host->card_tasklet);
}

/* detect cd interrupt */

static void msdc_tasklet_card(unsigned long arg)
{
    struct msdc_host *host = (struct msdc_host *)arg;
    struct msdc_hw *hw = host->hw;
    //unsigned long flags;
    //u32 base = host->base;
    u32 inserted;  
    //u32 status = 0;

    //spin_lock_irqsave(&host->lock, flags);
    //msdc_ungate_clock(host);
    
    if (hw->get_cd_status) { // NULL
        inserted = hw->get_cd_status();
    } else {
        //status = sdr_read32(MSDC_PS);
        if(hw->cd_level)
            inserted = (host->sd_cd_polarity == 0) ? 1 : 0;
        else
            inserted = (host->sd_cd_polarity == 0) ? 0 : 1;
    }
    if(host->block_bad_card){
        inserted = 0;
        if(host->mmc->card)
            mmc_card_set_removed(host->mmc->card);
        IRQ_MSG("remove bad SD card");
    } else {
        IRQ_MSG("card found<%s>", inserted ? "inserted" : "removed");  
    }

    host->card_inserted = inserted;    
    host->mmc->f_max = HOST_MAX_MCLK; 
    host->hw->cmd_edge = 0; // new card tuning from 0
    host->hw->rdata_edge = 0;
    host->hw->wdata_edge = 0;
    if(hw->flags & MSDC_UHS1){
        host->mmc->caps |= MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 |
                           MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR104;// | MMC_CAP_SET_XPC_180;
        host->mmc->caps2 |= MMC_CAP2_HS200_1_8V_SDR;
    }
    if(hw->flags & MSDC_DDR)
        host->mmc->caps |= MMC_CAP_UHS_DDR50|MMC_CAP_1_8V_DDR;
    msdc_host_mode[host->id] = host->mmc->caps;
    msdc_host_mode2[host->id] = host->mmc->caps2;
    if((hw->flags & MSDC_CD_PIN_EN) && inserted){
        msdc_reset_pwr_cycle_counter(host);
        msdc_reset_crc_tune_counter(host, all_counter);
        msdc_reset_tmo_tune_counter(host, all_counter);
    }
    // [Fix me] if card remove during a request
    //msdc_gate_clock(host, 1); 
    //spin_unlock_irqrestore(&host->lock, flags);
    ERR_MSG("host->suspend(%d)",host->suspend);
    if (!host->suspend && host->sd_cd_insert_work) { 
        mmc_detect_change(host->mmc, msecs_to_jiffies(200)); 
    }
    ERR_MSG("insert_workqueue(%d)",host->sd_cd_insert_work);
}

#ifdef MTK_SDIOAUTOK_SUPPORT
extern void autok_claim_host(struct msdc_host *host);
extern void autok_release_host(struct msdc_host *host);

extern int mtk_wdt_swsysret_config(int bit,int set_value);
#endif  // MTK_SDIOAUTOK_SUPPORT

#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT

#ifdef ONLINE_TUNING_DVTTEST
static int mt_msdc_online_tuning_test(struct msdc_host *host, u32 rawcmd, u32 rawarg, u8 rw);
#endif

static int ot_thread_func(void *data)
{
    struct ot_work_t *ot_work = (struct ot_work_t *)data;
    struct msdc_host *host = ot_work->host;
    int    chg_volt = host->ot_work.chg_volt;
    int err = 0;
#ifdef MTK_SDIO30_DETECT_THERMAL
    int cur_temperature = 0;
#endif // MTK_SDIO30_DETECT_THERMAL
#ifdef ONLINE_TUNING_DVTTEST
    static int dvt_tested = 0;
#endif


    if(atomic_read(&host->ot_work.ot_disable))
	{
		xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "[%s] online tuning is disabled\n", __func__);
		if(chg_volt)
		{
			host->ot_work.chg_volt = 0;
			atomic_set(&host->sdio_stopping, 0);
			//mmc_release_host(host->mmc);
			autok_release_host(host);
			return -1;
		}
        goto out;
	}

	if(atomic_read(&host->ot_work.autok_done) == 0)
	{
		xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "[%s] auto-K have not done\n", __func__);
		if(chg_volt)
		{
			host->ot_work.chg_volt = 0;
			atomic_set(&host->sdio_stopping, 0);
			//mmc_release_host(host->mmc);
			autok_release_host(host);
			return -1;
		}
        goto out;
	}

    if(host->mmc->card == NULL)
    {
        xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "[%s] sdio card is not ready\n", __func__);
		if(chg_volt)
		{
		    host->ot_work.chg_volt = 0;
			atomic_set(&host->sdio_stopping, 0);
			//mmc_release_host(host->mmc);
			autok_release_host(host);
		}
        return -1;
    }

    if(host->hw->host_function != MSDC_SDIO)
    {
        xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "[%s] Non SDIO device enter msdc_online_tuning!!!! \n", __func__);
		if(chg_volt)
		{
		    host->ot_work.chg_volt = 0;
			atomic_set(&host->sdio_stopping, 0);
			//mmc_release_host(host->mmc);
			autok_release_host(host);
		}
        return -1;
    }

    if(host->suspend)
    {
        xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "[%s] MSDC%d Suspended!!!! \n", __func__, host->id);
		if(chg_volt)
		{
			host->ot_work.chg_volt = 0;
			atomic_set(&host->sdio_stopping, 0);
			//mmc_release_host(host->mmc);
			autok_release_host(host);
			return -1;
		}
        goto out;
    }

#ifdef ONLINE_TUNING_DVTTEST
    if(dvt_tested == 0)
    {
        xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "[%s] Start Online Tuning DVT test \n", __func__);
        mt_msdc_online_tuning_test(host, 0, 0, 0);
        xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "[%s] Finish Online Tuning DVT test \n", __func__);
        return 0;
    }
#else
    
    //xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "[%s] chg_volt = %d \n", __func__, chg_volt);
  #ifdef MTK_SDIO30_DETECT_THERMAL
    if(chg_volt == 0)
    {
        if(host->pre_temper == -127000)
            host->pre_temper = mtk_thermal_get_temp(MTK_THERMAL_SENSOR_CPU);
    
        cur_temperature = mtk_thermal_get_temp(MTK_THERMAL_SENSOR_CPU);
    
        xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "[%s] cur_temperature = %d, host->pre_temper = %d\n", __func__, cur_temperature, host->pre_temper);
    
        if(cur_temperature != -127000 && host->pre_temper != -127000 && abs(cur_temperature-host->pre_temper) > OT_TEMPDIFF_BOUNDRY) {
            if((err = msdc_online_tuning(host, 1, 0x00B0)) != 0) {
                //print err
            }
            host->pre_temper = cur_temperature;
        }
    }
    else
  #endif // MTK_SDIO30_DETECT_THERMAL
    {
        xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "[%s] change volt\n", __func__);
        if((err = msdc_online_tuning(host, 1, 0x00B0)) != 0) {
            //print err
        }
        xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "[%s] onine tuning done (change volt)\n", __func__);
        return 0;
    }
#endif

out:
#ifdef MTK_SDIO30_DETECT_THERMAL
    if(chg_volt == 0)
        mod_timer(&host->ot_timer, jiffies + OT_PERIOD);
#endif // #ifdef MTK_SDIO30_DETECT_THERMAL
    return 0;
}

static void start_online_tuning(unsigned long data)
{
    struct msdc_host *host = (struct msdc_host *)data;
    struct task_struct *task;
    
    task = kthread_run(&ot_thread_func,(void *)(&host->ot_work),"online tuning");
}

#endif // MTK_SDIO30_ONLINE_TUNING_SUPPORT

#if 0 //No need in MT6588 MSDC

static u8 clk_src_bit[4] = {
    0, 3, 5, 7      
};
#endif   

static void msdc_select_clksrc(struct msdc_host* host, int clksrc)
{
#if 0 //No need in MT6588 MSDC
    u32 val; 
    u32 base = host->base;
        
    BUG_ON(clksrc > 3);  
    INIT_MSG("set clock source to <%d>", clksrc);      

    val = sdr_read32(MSDC_CLKSRC_REG);      
    if (sdr_read32(MSDC_ECO_VER) >= 4) {
        val &= ~(0x3  << clk_src_bit[host->id]); 
        val |= clksrc << clk_src_bit[host->id];                     
    } else {        
        val &= ~0x3; val |= clksrc;
    }    
    sdr_write32(MSDC_CLKSRC_REG, val);
#endif        
    host->hclk = hclks[clksrc];     
    host->hw->clk_src = clksrc;
}

volatile int sdio_autok_processed = 0;

static void msdc_set_mclk(struct msdc_host *host, int ddr, u32 hz)
{
    //struct msdc_hw *hw = host->hw;
    u32 base = host->base;
    u32 mode;
    u32 flags;
    unsigned long flags_lock = 0;
    u32 div;
    u32 sclk;
    u32 hclk = host->hclk;
    //u8  clksrc = hw->clk_src;

    if (!hz) { // set mmc system clock to 0 
        printk(KERN_ERR "msdc%d -> set mclk to 0",host->id);  // fix me: need to set to 0        
        if (is_card_sdio(host) || (host->hw->flags & MSDC_SDIO_IRQ)) { 
            host->saved_para.hz = hz;
#ifdef SDIO_ERROR_BYPASS    
            host->sdio_error = 0; 
#endif
		}
        host->mclk = 0;        
        msdc_reset_hw(host->id);       
        return;
    }
    if(host->hw->host_function == MSDC_SDIO && hz >= 100*1000*1000 && sdio_autok_processed == 0)
        hz = 50*1000*1000;
    printk("[%s] hz = %d\n", __func__, hz);

    if((host->hw->flags & MSDC_SDIO_IRQ) && (hz > 25000000)){
        hz = hz/2;
    }
    if (host->hw->flags & MSDC_SDIO_IRQ)
        spin_lock_irqsave(&SDIO_IRQ_lock, flags_lock);
    msdc_irq_save(flags);
    if (host->hw->flags & MSDC_SDIO_IRQ)
        spin_unlock_irqrestore(&SDIO_IRQ_lock, flags_lock);
    
    if (ddr) { /* may need to modify later */
        mode = 0x2; /* ddr mode and use divisor */
        if (hz >= (hclk >> 2)) {
            div  = 0;         /* mean div = 1/4 */
            sclk = hclk >> 2; /* sclk = clk / 4 */
        } else {
            div  = (hclk + ((hz << 2) - 1)) / (hz << 2);
            sclk = (hclk >> 2) / div;
            div  = (div >> 1); 
        }
    } else if (hz >= hclk) {
        mode = 0x1; /* no divisor */
        div  = 0;
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

    msdc_clk_stable(host,mode, div);
    
    host->sclk = sclk;
    host->mclk = hz;
    host->ddr = ddr;
#if 0
    if (host->sclk > 100000000) {
        sdr_clr_bits(MSDC_PATCH_BIT0, CKGEN_RX_SDClKO_SEL);    
    } else {
        sdr_set_bits(MSDC_PATCH_BIT0, CKGEN_RX_SDClKO_SEL);         
    }     
#endif

    msdc_set_timeout(host, host->timeout_ns, host->timeout_clks); // need because clk changed.

    //printk(KERN_ERR "================");
    if(hz >= 25000000)
        printk(KERN_ERR "msdc%d -> !!! Set<%dKHz> Source<%dKHz> -> sclk<%dKHz> DDR<%d> mode<%d> div<%d>" , 
                        host->id, hz/1000, hclk/1000, sclk/1000, ddr, mode, div); 
    else
        printk(KERN_WARNING "msdc%d -> !!! Set<%dKHz> Source<%dKHz> -> sclk<%dKHz> DDR<%d> mode<%d> div<%d>" , 
                        host->id, hz/1000, hclk/1000, sclk/1000, ddr, mode, div); 
        
    //printk(KERN_ERR "================");  
    if (host->hw->flags & MSDC_SDIO_IRQ)
        spin_lock_irqsave(&SDIO_IRQ_lock, flags_lock);
    msdc_irq_restore(flags);
    if (host->hw->flags & MSDC_SDIO_IRQ)
        spin_unlock_irqrestore(&SDIO_IRQ_lock, flags_lock);
}
extern int mmc_sd_power_cycle(struct mmc_host *host, u32 ocr, struct mmc_card *card);

/* 0 means pass */
static u32 msdc_power_tuning(struct msdc_host *host)
{
    struct mmc_host *mmc = host->mmc;
    struct mmc_card *card;
    struct mmc_request *mrq;
    u32 power_cycle = 0;
    int read_timeout_tune = 0;
    int write_timeout_tune = 0;
    u32 rwcmd_timeout_tune = 0;
    u32 read_timeout_tune_uhs104 = 0;
    u32 write_timeout_tune_uhs104 = 0;
    u32 sw_timeout = 0;
    u32 ret = 1;
    u32 host_err = 0;
    u32 base = host->base;
    if (!mmc) return 1;            

    card = mmc->card;
    if (card == NULL) {
        ERR_MSG("mmc->card is NULL");
        return 1;        
    }

    // eMMC first 
#ifdef MTK_EMMC_SUPPORT
    if (mmc_card_mmc(card) && (host->hw->host_function == MSDC_EMMC)) { 
        /* Fixme: */
        return 1;        
    }
#endif
    
    if((host->sd_30_busy > 0) && (host->sd_30_busy <= MSDC_MAX_POWER_CYCLE)){
        host->power_cycle_enable = 1;
    }
    if (mmc_card_sd(card) && (host->hw->host_function == MSDC_SD)) {
        if((host->power_cycle < MSDC_MAX_POWER_CYCLE) && (host->power_cycle_enable))
        {
            // power cycle 
            ERR_MSG("the %d time, Power cycle start", host->power_cycle);
            spin_unlock(&host->lock);
#ifdef FPGA_PLATFORM
            hwPowerDown_fpga();
#else
            if(host->power_control)
                host->power_control(host,0);
            else
                ERR_MSG("No power control callback. Please check host_function<0x%lx>",host->hw->host_function);
#endif
            mdelay(10);
#ifdef FPGA_PLATFORM
            hwPowerOn_fpga();
#else
            if(host->power_control)
                host->power_control(host,1);
            else
                ERR_MSG("No power control callback. Please check host_function<0x%lx>",host->hw->host_function);
#endif

            spin_lock(&host->lock);
            sdr_get_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, host->hw->ddlsel); 
            sdr_get_field(MSDC_IOCON, MSDC_IOCON_RSPL, host->hw->cmd_edge);    // save the para
            sdr_get_field(MSDC_IOCON, MSDC_IOCON_DSPL, host->hw->rdata_edge); 
            sdr_get_field(MSDC_IOCON, MSDC_IOCON_W_DSPL, host->hw->wdata_edge);
            host->saved_para.pad_tune = sdr_read32(MSDC_PAD_TUNE);
            host->saved_para.ddly0 = sdr_read32(MSDC_DAT_RDDLY0);
            host->saved_para.ddly1 = sdr_read32(MSDC_DAT_RDDLY1);
            sdr_get_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_CMD_RSP,    host->saved_para.cmd_resp_ta_cntr);
            sdr_get_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_WRDAT_CRCS, host->saved_para.wrdat_crc_ta_cntr); 

            /* for write: 3T need wait before host check busy after crc status 
             * for write: host check timeout change to 16T */
            sdr_get_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_ECO_1, host->saved_para.write_timeout_eco1);
            sdr_get_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_ECO_2, host->saved_para.write_timeout_eco2); 
        
            if((host->sclk > 100000000) && (host->power_cycle >= 1))
                mmc->caps &= ~MMC_CAP_UHS_SDR104;
            if(((host->sclk <= 100000000) && ((host->sclk > 50000000) || host->ddr)) && (host->power_cycle >= 1)){
                mmc->caps &= ~(MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR104 | MMC_CAP_UHS_DDR50);
            }
            
            msdc_host_mode[host->id] = mmc->caps;
            msdc_host_mode2[host->id] = mmc->caps2;

            // clock should set to 260K 
            mmc->ios.clock = HOST_MIN_MCLK;  
            mmc->ios.bus_width = MMC_BUS_WIDTH_1;
            mmc->ios.timing = MMC_TIMING_LEGACY;          
            msdc_set_mclk(host, 0, HOST_MIN_MCLK);                 

            //zone_temp = sd_debug_zone[1]; 
            //sd_debug_zone[1] |= (DBG_EVT_NRW | DBG_EVT_RW);
        
            // re-init the card! 
            mrq = host->mrq;
            host->mrq = NULL;
            power_cycle = host->power_cycle;
            host->power_cycle = MSDC_MAX_POWER_CYCLE; 
            read_timeout_tune = host->read_time_tune;
            write_timeout_tune = host->write_time_tune;
            rwcmd_timeout_tune = host->rwcmd_time_tune;
            read_timeout_tune_uhs104 = host->read_timeout_uhs104;
            write_timeout_tune_uhs104 = host->write_timeout_uhs104;
            sw_timeout = host->sw_timeout;
            host_err = host->error;
            spin_unlock(&host->lock);
            ret = mmc_sd_power_cycle(mmc, mmc->ocr, card);
            spin_lock(&host->lock);
            host->mrq = mrq;
            host->power_cycle = power_cycle;
            host->read_time_tune = read_timeout_tune;
            host->write_time_tune = write_timeout_tune;
            host->rwcmd_time_tune = rwcmd_timeout_tune;
            if(host->sclk > 100000000){
                host->write_timeout_uhs104 = write_timeout_tune_uhs104;
            } else{
                host->read_timeout_uhs104 = 0;
                host->write_timeout_uhs104 = 0;
            }
            host->sw_timeout = sw_timeout;
            host->error = host_err;
            if(!ret)
                host->power_cycle_enable = 0;
            ERR_MSG("the %d time, Power cycle Done, host->error(0x%x), ret(%d)", host->power_cycle,host->error, ret);
            (host->power_cycle)++;
        }else if (host->power_cycle == MSDC_MAX_POWER_CYCLE){
            ERR_MSG("the %d time, exceed the max power cycle time %d, go to remove the bad card, power_cycle_enable=%d", host->power_cycle, MSDC_MAX_POWER_CYCLE, host->power_cycle_enable);
            msdc_set_bad_card_and_remove(host);
        }
    }
    return ret;
}


static void msdc_send_stop(struct msdc_host *host)
{
    struct mmc_command stop = {0};    
    struct mmc_request mrq = {0};
    u32 err = -1; 
  
    stop.opcode = MMC_STOP_TRANSMISSION;    
    stop.arg = 0;  
    stop.flags = MMC_RSP_R1B | MMC_CMD_AC;

    mrq.cmd = &stop; stop.mrq = &mrq;
    stop.data = NULL;        
     
    err = msdc_do_command(host, &stop, 0, CMD_TIMEOUT);
}

extern void mmc_detach_bus(struct mmc_host *host);
extern void mmc_power_off(struct mmc_host *host);

int msdc_reinit(struct msdc_host *host)
{
    struct mmc_host *mmc;
    struct mmc_card *card;
    //struct mmc_request *mrq; 
    int ret = -1;
    u32 err = 0;
    u32 status = 0;
    unsigned long tmo = 12;
    //u32 state = 0;
    if(!host){
        ERR_MSG("msdc_host is NULL");
        return -1;
    }
    mmc = host->mmc;
    if (!mmc) {
        ERR_MSG("mmc is NULL");
        return -1;
    }

    card = mmc->card;
    if (card == NULL) 
        ERR_MSG("mmc->card is NULL");
    if(host->block_bad_card)
        ERR_MSG("Need block this bad SD card from re-initialization");

    // eMMC first 
#ifdef MTK_EMMC_SUPPORT
    if (host->hw->host_function == MSDC_EMMC) { 
        /* Fixme: */
        return -1;        
    }
#endif
    if(host->hw->host_function == MSDC_SD){
        if ((!(host->hw->flags & MSDC_CD_PIN_EN)) && (host->block_bad_card == 0)) {
            // power cycle 
            ERR_MSG("SD card Re-Init!");
            mmc_claim_host(host->mmc);
            ERR_MSG("SD card Re-Init get host!");
            spin_lock(&host->lock);
            ERR_MSG("SD card Re-Init get lock!");
            msdc_clksrc_onoff(host, 1); 
            if(host->app_cmd_arg){
                while((err = msdc_get_card_status(mmc, host, &status))) {
                    ERR_MSG("SD card Re-Init in get card status!err(%d)",err);
                    if(err == (unsigned int)-EIO){
                        if (msdc_tune_cmdrsp(host)) {
                            ERR_MSG("update cmd para failed");  
                            break;
                        }
                    }else {
                        break;
                    }
                }          
                if(err == 0){
                    msdc_clksrc_onoff(host, 0); 
                    spin_unlock(&host->lock);  
                    mmc_release_host(host->mmc);
                    ERR_MSG("SD Card is ready.");
                    return 0;
                }
            }
            msdc_clksrc_onoff(host, 0);
            ERR_MSG("Reinit start..");
            mmc->ios.clock = HOST_MIN_MCLK;  
            mmc->ios.bus_width = MMC_BUS_WIDTH_1;
            mmc->ios.timing = MMC_TIMING_LEGACY;
            host->card_inserted = 1;
            msdc_clksrc_onoff(host, 1); 
            msdc_set_mclk(host, 0, HOST_MIN_MCLK);
            msdc_clksrc_onoff(host, 0);
            spin_unlock(&host->lock);
            mmc_release_host(host->mmc);
            if(host->mmc->card){
                mmc_remove_card(host->mmc->card);
                host->mmc->card = NULL;
                mmc_claim_host(host->mmc);
                mmc_detach_bus(host->mmc);
                mmc_release_host(host->mmc);
            }
            mmc_power_off(host->mmc);
            mmc_detect_change(host->mmc,0);
            while(tmo){
                if(host->mmc->card && mmc_card_present(host->mmc->card)){
                    ret = 0;
                    break;
                }
                msleep(50);
                tmo--;
            }
            ERR_MSG("Reinit %s",ret == 0 ? "success" : "fail");
    
       }
       if((host->hw->flags & MSDC_CD_PIN_EN) && (host->mmc->card) && mmc_card_present(host->mmc->card) && (!mmc_card_removed(host->mmc->card)) && (host->block_bad_card == 0))
           ret = 0;
    }
    return ret;
}

/* Fix me. when need to abort */
static u32 msdc_abort_data(struct msdc_host *host)
{
    struct mmc_host *mmc = host->mmc;     
    u32 base = host->base;    
    u32 status = 0;    
    u32 state = 0;
    u32 err = 0;   
    unsigned long tmo = jiffies + POLLING_BUSY;
          
    while (state != 4) { // until status to "tran"
        msdc_reset_hw(host->id);
        while ((err = msdc_get_card_status(mmc, host, &status))) {
            ERR_MSG("CMD13 ERR<%d>",err);
            if (err != (unsigned int)-EIO) {
                return msdc_power_tuning(host);
            } else if (msdc_tune_cmdrsp(host)) {        
                ERR_MSG("update cmd para failed");  
                return 1;
            }   
        } 

        state = R1_CURRENT_STATE(status);
        ERR_MSG("check card state<%d>", state);
        if (state == 5 || state == 6) {
            ERR_MSG("state<%d> need cmd12 to stop", state);  
            msdc_send_stop(host); // don't tuning
        } else if (state == 7) {  // busy in programing          
            ERR_MSG("state<%d> card is busy", state);  
            spin_unlock(&host->lock);             
            msleep(100);
            spin_lock(&host->lock);
        } else if (state != 4) {
            ERR_MSG("state<%d> ??? ", state);
            return msdc_power_tuning(host);    
        }
        
        if (time_after(jiffies, tmo)) {
            ERR_MSG("abort timeout. Do power cycle");
            if(host->hw->host_function == MSDC_SD && (host->sclk >= 100000000 || host->ddr))
                host->sd_30_busy++;
            return msdc_power_tuning(host);
        }
    }
    
    msdc_reset_hw(host->id); 
    return 0;   
}
static u32 msdc_polling_idle(struct msdc_host *host)
{
    struct mmc_host *mmc = host->mmc;     
    u32 status = 0;    
    u32 state = 0;
    u32 err = 0;   
    unsigned long tmo = jiffies + POLLING_BUSY;
          
    while (state != 4) { // until status to "tran"
        while ((err = msdc_get_card_status(mmc, host, &status))) {
            ERR_MSG("CMD13 ERR<%d>",err);
            if (err != (unsigned int)-EIO) {
                return msdc_power_tuning(host);
            } else if (msdc_tune_cmdrsp(host)) {        
                ERR_MSG("update cmd para failed");    
                return 1;
            }   
        } 

        state = R1_CURRENT_STATE(status);
        //ERR_MSG("check card state<%d>", state);
        if (state == 5 || state == 6) {
            ERR_MSG("state<%d> need cmd12 to stop", state);    
            msdc_send_stop(host); // don't tuning
        } else if (state == 7) {  // busy in programing            
            ERR_MSG("state<%d> card is busy", state);    
            spin_unlock(&host->lock);             
            msleep(100);
            spin_lock(&host->lock);
        } else if (state != 4) {
            ERR_MSG("state<%d> ??? ", state);
            return msdc_power_tuning(host);      
        }
        
        if (time_after(jiffies, tmo)) {
            ERR_MSG("abort timeout. Do power cycle");                                  
            return msdc_power_tuning(host);
        }
    }
    return 0;   
}



static void msdc_pin_config(struct msdc_host *host, int mode)
{
    struct msdc_hw *hw = host->hw;
#ifdef FPGA_PLATFORM
    u32 base = host->base;
#endif
    int pull = (mode == MSDC_PIN_PULL_UP) ? GPIO_PULL_UP : GPIO_PULL_DOWN;

    /* Config WP pin */
    if (hw->flags & MSDC_WP_PIN_EN) {
        if (hw->config_gpio_pin) /* NULL */
            hw->config_gpio_pin(MSDC_WP_PIN, pull);
    }

    switch (mode) {
        case MSDC_PIN_PULL_UP:
            //sdr_set_field(MSDC_PAD_CTL0, MSDC_PAD_CTL0_CLKPU, 1); /* Check & FIXME */
            //sdr_set_field(MSDC_PAD_CTL0, MSDC_PAD_CTL0_CLKPD, 0); /* Check & FIXME */
#ifdef FPGA_PLATFORM
            sdr_set_field(MSDC_PAD_CTL1, MSDC_PAD_CTL1_CMDPU, 1);
            sdr_set_field(MSDC_PAD_CTL1, MSDC_PAD_CTL1_CMDPD, 0);
            sdr_set_field(MSDC_PAD_CTL2, MSDC_PAD_CTL2_DATPU, 1);
            sdr_set_field(MSDC_PAD_CTL2, MSDC_PAD_CTL2_DATPD, 0);
#else
            msdc_pin_pud(host,1);
#endif
            break;
        case MSDC_PIN_PULL_DOWN:
            //sdr_set_field(MSDC_PAD_CTL0, MSDC_PAD_CTL0_CLKPU, 0); /* Check & FIXME */
            //sdr_set_field(MSDC_PAD_CTL0, MSDC_PAD_CTL0_CLKPD, 1); /* Check & FIXME */
#ifdef FPGA_PLATFORM
            sdr_set_field(MSDC_PAD_CTL1, MSDC_PAD_CTL1_CMDPU, 0);
            sdr_set_field(MSDC_PAD_CTL1, MSDC_PAD_CTL1_CMDPD, 1);
            sdr_set_field(MSDC_PAD_CTL2, MSDC_PAD_CTL2_DATPU, 0);
            sdr_set_field(MSDC_PAD_CTL2, MSDC_PAD_CTL2_DATPD, 1);
#else
            msdc_pin_pud(host,0);
#endif
            break;
        case MSDC_PIN_PULL_NONE:
        default:
            //sdr_set_field(MSDC_PAD_CTL0, MSDC_PAD_CTL0_CLKPU, 0); /* Check & FIXME */
            //sdr_set_field(MSDC_PAD_CTL0, MSDC_PAD_CTL0_CLKPD, 0); /* Check & FIXME */
#ifdef FPGA_PLATFORM
            sdr_set_field(MSDC_PAD_CTL1, MSDC_PAD_CTL1_CMDPU, 0);
            sdr_set_field(MSDC_PAD_CTL1, MSDC_PAD_CTL1_CMDPD, 0);
            sdr_set_field(MSDC_PAD_CTL2, MSDC_PAD_CTL2_DATPU, 0);
            sdr_set_field(MSDC_PAD_CTL2, MSDC_PAD_CTL2_DATPD, 0);
#else
            msdc_pin_pnul(host,0);
#endif
            break;
    }
    
    N_MSG(CFG, "Pins mode(%d), down(%d), up(%d)", mode, MSDC_PIN_PULL_DOWN, MSDC_PIN_PULL_UP);
}

static void msdc_pin_reset(struct msdc_host *host, int mode)
{
    struct msdc_hw *hw = (struct msdc_hw *)host->hw;
    u32 base = host->base;
    int pull = (mode == MSDC_PIN_PULL_UP) ? GPIO_PULL_UP : GPIO_PULL_DOWN;

    /* Config reset pin */
    if (hw->flags & MSDC_RST_PIN_EN) {
        if (hw->config_gpio_pin) /* NULL */
            hw->config_gpio_pin(MSDC_RST_PIN, pull);

        if (mode == MSDC_PIN_PULL_UP) {
            sdr_clr_bits(EMMC_IOCON, EMMC_IOCON_BOOTRST);
        } else {
            sdr_set_bits(EMMC_IOCON, EMMC_IOCON_BOOTRST);
        }
    }
}

static void msdc_set_power_mode(struct msdc_host *host, u8 mode)
{
    N_MSG(CFG, "Set power mode(%d)", mode);
    if (host->power_mode == MMC_POWER_OFF && mode != MMC_POWER_OFF) {
        msdc_pin_reset (host, MSDC_PIN_PULL_UP);
        msdc_pin_config(host, MSDC_PIN_PULL_UP);

#ifdef FPGA_PLATFORM
        hwPowerOn_fpga();
#else
        if(host->power_control)
            host->power_control(host,1);
        else
            ERR_MSG("No power control callback. Please check host_function<0x%lx> and Power_domain<%d>",host->hw->host_function,host->power_domain);
    
#endif
    
        mdelay(10);
    } else if (host->power_mode != MMC_POWER_OFF && mode == MMC_POWER_OFF) {
     
        if (is_card_sdio(host)|| (host->hw->flags & MSDC_SDIO_IRQ)) {
            msdc_pin_config(host, MSDC_PIN_PULL_DOWN);
        }else {
        
#ifdef FPGA_PLATFORM
            hwPowerDown_fpga();
#else
            if(host->power_control)
                host->power_control(host,0);
            else
                ERR_MSG("No power control callback. Please check host_function<0x%lx> and Power_domain<%d>",host->hw->host_function,host->power_domain);
#endif
            msdc_pin_config(host, MSDC_PIN_PULL_DOWN);
        }
        mdelay(10);
        msdc_pin_reset (host, MSDC_PIN_PULL_DOWN);
    }
    host->power_mode = mode;
}

#ifdef MTK_EMMC_ETT_TO_DRIVER
static int msdc_ett_offline_to_driver(struct msdc_host *host)
{
    int ret = 1;  // 1 means failed
    int size = sizeof(g_mmcTable) / sizeof(mmcdev_info);
    int i, temp; 
    u32 base = host->base;
    
    //printk(KERN_ERR "msdc_ett_offline_to_driver size<%d> \n", size);

    for (i = 0; i < size; i++) {
        //printk(KERN_ERR"msdc <%d> <%s> <%s>\n", i, g_mmcTable[i].pro_name, pro_name); 
                        
        if ((g_mmcTable[i].m_id == m_id) && (!strncmp(g_mmcTable[i].pro_name, pro_name, 6))) {
            printk(KERN_ERR "msdc ett index<%d>: <%d> <%d> <0x%x> <0x%x> <0x%x>\n", i, 
                g_mmcTable[i].r_smpl, g_mmcTable[i].d_smpl, 
                g_mmcTable[i].cmd_rxdly, g_mmcTable[i].rd_rxdly, g_mmcTable[i].wr_rxdly);  

            // set to msdc0 
            sdr_set_field(MSDC_IOCON, MSDC_IOCON_RSPL, g_mmcTable[i].r_smpl); 
            sdr_set_field(MSDC_IOCON, MSDC_IOCON_DSPL, g_mmcTable[i].d_smpl);
          
            sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, g_mmcTable[i].cmd_rxdly);
            sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATRRDLY, g_mmcTable[i].rd_rxdly); 
            sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATWRDLY, g_mmcTable[i].wr_rxdly); 

            temp = g_mmcTable[i].rd_rxdly; temp &= 0x1F;             
            sdr_write32(MSDC_DAT_RDDLY0, (temp<<0 | temp<<8 | temp<<16 | temp<<24)); 
            sdr_write32(MSDC_DAT_RDDLY1, (temp<<0 | temp<<8 | temp<<16 | temp<<24));
                                 
            ret = 0;
            break;
        }
    }
    
    //if (ret) printk(KERN_ERR "msdc failed to find\n");
    return ret;            
}
#endif


extern int mmc_card_sleepawake(struct mmc_host *host, int sleep);
//extern int mmc_send_status(struct mmc_card *card, u32 *status);  
extern int mmc_go_idle(struct mmc_host *host);
extern int mmc_send_op_cond(struct mmc_host *host, u32 ocr, u32 *rocr);
extern int mmc_all_send_cid(struct mmc_host *host, u32 *cid);
//extern int mmc_attach_mmc(struct mmc_host *host, u32 ocr);

typedef enum MMC_STATE_TAG
{
    MMC_IDLE_STATE,
    MMC_READ_STATE,
    MMC_IDENT_STATE,
    MMC_STBY_STATE,
    MMC_TRAN_STATE,
    MMC_DATA_STATE,
    MMC_RCV_STATE,
    MMC_PRG_STATE,
    MMC_DIS_STATE,
    MMC_BTST_STATE,
    MMC_SLP_STATE,
    MMC_RESERVED1_STATE,
    MMC_RESERVED2_STATE,
    MMC_RESERVED3_STATE,
    MMC_RESERVED4_STATE,
    MMC_RESERVED5_STATE,
}MMC_STATE_T;

typedef enum EMMC_CHIP_TAG{
    SAMSUNG_EMMC_CHIP = 0x15,
    SANDISK_EMMC_CHIP = 0x45,
    HYNIX_EMMC_CHIP   = 0x90,
} EMMC_VENDOR_T;
#if 0
#ifdef MTK_EMMC_SUPPORT
unsigned int sg_emmc_sleep = 0;
static void msdc_config_emmc_pad(int padEmmc)
{
    static int sg_gpio164_mode;
    static int sg_gpio165_mode;
    static int sg_gpio166_mode;
    static int sg_gpio168_mode;
    static int sg_gpio169_mode;
    static int sg_gpio170_mode;
    static int sg_gpio171_mode;
    static int sg_gpio173_mode;
    static int sg_gpio175_mode;
    
    if (padEmmc == 0){
        sg_gpio164_mode = mt_get_gpio_mode(GPIO164);
        sg_gpio165_mode = mt_get_gpio_mode(GPIO165);
        sg_gpio166_mode = mt_get_gpio_mode(GPIO166);
        sg_gpio168_mode = mt_get_gpio_mode(GPIO168);
        sg_gpio169_mode = mt_get_gpio_mode(GPIO169);
        sg_gpio170_mode = mt_get_gpio_mode(GPIO170);
        sg_gpio171_mode = mt_get_gpio_mode(GPIO171);
        sg_gpio173_mode = mt_get_gpio_mode(GPIO173);
        sg_gpio175_mode = mt_get_gpio_mode(GPIO175);

        mt_set_gpio_mode(GPIO164, GPIO_MODE_GPIO);
        mt_set_gpio_mode(GPIO165, GPIO_MODE_GPIO);
        mt_set_gpio_mode(GPIO166, GPIO_MODE_GPIO);
        mt_set_gpio_mode(GPIO168, GPIO_MODE_GPIO);
        mt_set_gpio_mode(GPIO169, GPIO_MODE_GPIO);
        mt_set_gpio_mode(GPIO170, GPIO_MODE_GPIO);
        mt_set_gpio_mode(GPIO171, GPIO_MODE_GPIO);
        mt_set_gpio_mode(GPIO173, GPIO_MODE_GPIO);
        mt_set_gpio_mode(GPIO175, GPIO_MODE_GPIO);

        mt_set_gpio_dir(GPIO164, GPIO_DIR_IN);
        mt_set_gpio_dir(GPIO165, GPIO_DIR_IN);
        mt_set_gpio_dir(GPIO166, GPIO_DIR_IN);
        mt_set_gpio_dir(GPIO168, GPIO_DIR_IN);
        mt_set_gpio_dir(GPIO169, GPIO_DIR_IN);
        mt_set_gpio_dir(GPIO170, GPIO_DIR_IN);
        mt_set_gpio_dir(GPIO171, GPIO_DIR_IN);
        mt_set_gpio_dir(GPIO173, GPIO_DIR_IN);
        mt_set_gpio_dir(GPIO175, GPIO_DIR_IN);

        mt_set_gpio_pull_enable(GPIO164, GPIO_PULL_DISABLE);
        mt_set_gpio_pull_enable(GPIO165, GPIO_PULL_DISABLE);
        mt_set_gpio_pull_enable(GPIO166, GPIO_PULL_DISABLE);
        mt_set_gpio_pull_enable(GPIO168, GPIO_PULL_DISABLE);
        mt_set_gpio_pull_enable(GPIO169, GPIO_PULL_DISABLE);
        mt_set_gpio_pull_enable(GPIO170, GPIO_PULL_DISABLE);
        mt_set_gpio_pull_enable(GPIO171, GPIO_PULL_DISABLE);
        mt_set_gpio_pull_enable(GPIO173, GPIO_PULL_DISABLE);
        mt_set_gpio_pull_enable(GPIO175, GPIO_PULL_DISABLE);
    } else { 
        mt_set_gpio_mode(GPIO164, sg_gpio164_mode);
        mt_set_gpio_mode(GPIO165, sg_gpio165_mode);
        mt_set_gpio_mode(GPIO166, sg_gpio166_mode);
        mt_set_gpio_mode(GPIO168, sg_gpio168_mode);
        mt_set_gpio_mode(GPIO169, sg_gpio169_mode);
        mt_set_gpio_mode(GPIO170, sg_gpio170_mode);
        mt_set_gpio_mode(GPIO171, sg_gpio171_mode);
        mt_set_gpio_mode(GPIO173, sg_gpio173_mode);
        mt_set_gpio_mode(GPIO175, sg_gpio175_mode);
    }
}

static void msdc_sleep_enter(struct msdc_host *host)
{
    //u32 l_status = MMC_IDLE_STATE;
    //unsigned long tmo;
    struct mmc_host *mmc;
    struct mmc_card *card;

    BUG_ON(!host->mmc);
    BUG_ON(!host->mmc->card);
    
    mmc = host->mmc;
    card = host->mmc->card;

    /* check card type */
    if (MMC_TYPE_MMC != card->type) {
        printk(KERN_WARNING"[EMMC] not a mmc card, pls check it before sleep\n");
        return ;
    }

    mmc_claim_host(mmc);
    mmc_go_idle(mmc); // Infinity: Ask eMMC into open-drain mode
    
    // add for hynix emcp chip 
    if (host->mmc->card->cid.manfid == HYNIX_EMMC_CHIP){
        u32 l_ocr = mmc->ocr;
        u32 l_cid[4];
        u32 l_rocr;
        u32 l_ret;

        // clk freq down, 26kHz for emmc card init
        msdc_set_mclk(host, 0, 400000);

        //send CMD1, will loop for card's busy state  
        l_ret = mmc_send_op_cond(mmc, l_ocr | (1 << 30), &l_rocr);
        if (l_ret != 0){
            ERR_MSG("send cmd1 error while emmc card enter low power state\n");       /* won't happen. */
        }

        //send CMD2
        l_cid[0] = 0;
        l_cid[1] = 0;
        l_cid[2] = 0;
        l_cid[3] = 0;
        l_ret = mmc_all_send_cid(mmc, l_cid);
        if (l_ret != 0){
            ERR_MSG("send cmd2 error while emmc card enter low power state\n");       /* won't happen. */
        }
    }

    msdc_config_emmc_pad(0);

    mmc_release_host(mmc);
}

static void msdc_sleep_out(struct msdc_host *host)
{
    struct mmc_host *mmc;
    struct mmc_card *card;

    BUG_ON(!host->mmc);
    BUG_ON(!host->mmc->card);
    
    mmc = host->mmc;
    card = host->mmc->card;
 
    
    /* check card type */
    if (MMC_TYPE_MMC != card->type) {
        printk(KERN_WARNING"[EMMC] not a mmc card, pls check it before sleep\n");
        return ;
    }

    mmc_claim_host(mmc);
    msdc_config_emmc_pad(1);
    mmc_release_host(mmc);
}

static void msdc_emmc_sleepawake(struct msdc_host *host, u32 awake)
{
    /* slot0 for emmc, sd/sdio do not support sleep state */
    if (host->id != EMMC_HOST_ID) {
        return;
    }

    /* for emmc card need to go sleep state, while suspend.
     * because we need emmc power always on to guarantee brom can 
     * boot from emmc */
    if ((awake == 1) && (sg_emmc_sleep == 1)) {
        msdc_sleep_out(host);
        sg_emmc_sleep = 0;
    } else if((awake == 0) && (sg_emmc_sleep == 0)) {
        msdc_sleep_enter(host);
        sg_emmc_sleep = 1;
    }          
}
#endif
#endif
static void msdc_clksrc_onoff(struct msdc_host *host, u32 on)
{
    u32 base = host->base; 
    u32 div, mode;         
    if (on) {
        if (0 == host->core_clkon) {
#ifndef FPGA_PLATFORM
            if(enable_clock(MT_CG_PERI_MSDC30_0 + host->id, "SD")){
                printk(KERN_ERR "msdc%d on clock failed ===> retry once\n",host->id);
                disable_clock(MT_CG_PERI_MSDC30_0 + host->id, "SD");
                enable_clock(MT_CG_PERI_MSDC30_0 + host->id, "SD");
            }                
#endif
            host->core_clkon = 1;      
            udelay(10);
           
            sdr_set_field(MSDC_CFG, MSDC_CFG_MODE, MSDC_SDMMC);
                         
                       
            sdr_get_field(MSDC_CFG, MSDC_CFG_CKMOD, mode);                
            sdr_get_field(MSDC_CFG, MSDC_CFG_CKDIV, div); 
            msdc_clk_stable(host,mode, div);  

            if (is_card_sdio(host)|| (host->hw->flags & MSDC_SDIO_IRQ)) { 
                //mdelay(1000);  // wait for WIFI stable.       
            }
                          
            //INIT_MSG("3G pll = 0x%x when clk<on>", sdr_read32(0xF00071C0));
#ifndef FPGA_PLATFORM 
            //freq_meter(0xf, 0); 
#endif              
        }        
    } else {
        if (!((host->hw->flags & MSDC_SDIO_IRQ) && src_clk_control))
        {
            if (1 == host->core_clkon) {    
                sdr_set_field(MSDC_CFG, MSDC_CFG_MODE, MSDC_MS);
                
#ifndef FPGA_PLATFORM                 
                disable_clock(MT_CG_PERI_MSDC30_0 + host->id, "SD");  
#endif           
                host->core_clkon = 0;   
                     
                //INIT_MSG("3G pll = 0x%x when clk<off>", sdr_read32(0xF00071C0));
#ifndef FPGA_PLATFORM 
                //freq_meter(0xf, 0);      
#endif                    
            }                
        }          
    }
}

/*
   register as callback function of WIFI(combo_sdio_register_pm) .    
   can called by msdc_drv_suspend/resume too. 
*/
static void msdc_save_emmc_setting(struct msdc_host *host)
{
    u32 base = host->base;
    host->saved_para.ddr = host->ddr;
    host->saved_para.hz = host->mclk;
    host->saved_para.sdc_cfg = sdr_read32(SDC_CFG);

    sdr_get_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, host->hw->ddlsel); 
    //sdr_get_field(MSDC_IOCON, MSDC_IOCON_RDSPLSEL, host->hw->rdsplsel); 
    //sdr_get_field(MSDC_IOCON, MSDC_IOCON_WDSPLSEL, host->hw->wdsplsel);

    sdr_get_field(MSDC_IOCON, MSDC_IOCON_RSPL, host->hw->cmd_edge); // save the para
    sdr_get_field(MSDC_IOCON, MSDC_IOCON_DSPL, host->hw->rdata_edge); 
    sdr_get_field(MSDC_IOCON, MSDC_IOCON_W_DSPL, host->hw->wdata_edge);
    host->saved_para.pad_tune = sdr_read32(MSDC_PAD_TUNE);
    host->saved_para.ddly0 = sdr_read32(MSDC_DAT_RDDLY0);
    host->saved_para.ddly1 = sdr_read32(MSDC_DAT_RDDLY1);
    sdr_get_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_CMD_RSP,    host->saved_para.cmd_resp_ta_cntr);
    sdr_get_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_WRDAT_CRCS, host->saved_para.wrdat_crc_ta_cntr); 

    /* for write: 3T need wait before host check busy after crc status 
     * for write: host check timeout change to 16T */
    sdr_get_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_ECO_1, host->saved_para.write_timeout_eco1);
    sdr_get_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_ECO_2, host->saved_para.write_timeout_eco2); 
}
static void msdc_restore_emmc_setting(struct msdc_host *host)
{
    u32 base = host->base;
    msdc_set_mclk(host,host->ddr,host->mclk);
    sdr_write32(SDC_CFG,host->saved_para.sdc_cfg);
    
    sdr_set_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, host->hw->ddlsel); 
    //sdr_set_field(MSDC_IOCON, MSDC_IOCON_RDSPLSEL, host->hw->rdsplsel); 
    //sdr_set_field(MSDC_IOCON, MSDC_IOCON_WDSPLSEL, host->hw->wdsplsel);
    
    sdr_set_field(MSDC_IOCON, MSDC_IOCON_RSPL, host->hw->cmd_edge); 
    sdr_set_field(MSDC_IOCON, MSDC_IOCON_DSPL, host->hw->rdata_edge);
    sdr_set_field(MSDC_IOCON, MSDC_IOCON_W_DSPL, host->hw->wdata_edge);
    sdr_write32(MSDC_PAD_TUNE,host->saved_para.pad_tune);
    sdr_write32(MSDC_DAT_RDDLY0,host->saved_para.ddly0);
    sdr_write32(MSDC_DAT_RDDLY1,host->saved_para.ddly1);
    sdr_set_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_WRDAT_CRCS, host->saved_para.wrdat_crc_ta_cntr); 
    sdr_set_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_CMD_RSP,    host->saved_para.cmd_resp_ta_cntr);

    /* for write: 3T need wait before host check busy after crc status 
     * for write: host check timeout change to 16T */
    sdr_set_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_ECO_1, host->saved_para.write_timeout_eco1);
    sdr_set_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_ECO_2, host->saved_para.write_timeout_eco2); 
}


static void msdc_pm(pm_message_t state, void *data)
{
    struct msdc_host *host = (struct msdc_host *)data;

    int evt = state.event;
    u32 base = host->base;

    // Un-gate clock first when resume. 
    
    msdc_ungate_clock(host);

    if (evt == PM_EVENT_SUSPEND || evt == PM_EVENT_USER_SUSPEND) {
        if (host->suspend) /* already suspend */  /* default 0*/
            goto end;

        /* for memory card. already power off by mmc */
        if (evt == PM_EVENT_SUSPEND && host->power_mode == MMC_POWER_OFF)  
            goto end;

        host->suspend = 1;
        host->pm_state = state;  /* default PMSG_RESUME */
#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
		atomic_set(&host->ot_work.autok_done, 0);
#endif
        
        printk(KERN_ERR "msdc%d -> %s Suspend",host->id, evt == PM_EVENT_SUSPEND ? "PM" : "USR");                      
        if(host->hw->flags & MSDC_SYS_SUSPEND){ /* set for card */
//#ifdef MTK_EMMC_SUPPORT
            //msdc_emmc_sleepawake(host, 0);
//#endif

#ifdef MTK_EMMC_SUPPORT
            if(host->hw->host_function == MSDC_EMMC && host->mmc->card && mmc_card_mmc(host->mmc->card)){
                if (g_emmc_mode_switch == 0){    
                    host->mmc->pm_flags |= MMC_PM_KEEP_POWER;
                } else {
                    host->mmc->pm_flags &= (~MMC_PM_KEEP_POWER);
                }
            }
#else
            if(host->hw->host_function == MSDC_EMMC && host->mmc->card && mmc_card_mmc(host->mmc->card))
                host->mmc->pm_flags |= MMC_PM_KEEP_POWER;
#endif
            
            (void)mmc_suspend_host(host->mmc);

#ifdef MTK_EMMC_SUPPORT
            if((g_emmc_mode_switch == 0) && host->hw->host_function == MSDC_EMMC && host->mmc->card && mmc_card_mmc(host->mmc->card)){
                msdc_save_emmc_setting(host);
                host->power_control(host,0);
                msdc_pin_config(host, MSDC_PIN_PULL_DOWN);
                msdc_pin_reset (host, MSDC_PIN_PULL_DOWN);
            }
#else
            if(host->hw->host_function == MSDC_EMMC && host->mmc->card && mmc_card_mmc(host->mmc->card)){
                msdc_save_emmc_setting(host);
                host->power_control(host,0);
                msdc_pin_config(host, MSDC_PIN_PULL_DOWN);
                msdc_pin_reset (host, MSDC_PIN_PULL_DOWN);
            }
#endif
        } else { 
            host->mmc->pm_flags |= MMC_PM_IGNORE_PM_NOTIFY;  /* just for double confirm */       
            mmc_remove_host(host->mmc);
        }
    } else if (evt == PM_EVENT_RESUME || evt == PM_EVENT_USER_RESUME) {
        if (!host->suspend){
            //ERR_MSG("warning: already resume");       
            goto end;
        }

        /* No PM resume when USR suspend */
        if (evt == PM_EVENT_RESUME && host->pm_state.event == PM_EVENT_USER_SUSPEND) {
            ERR_MSG("PM Resume when in USR Suspend");       /* won't happen. */
            goto end;
        }
        
        host->suspend = 0;
        host->pm_state = state;
        
        printk(KERN_ERR "msdc%d -> %s Resume",host->id,evt == PM_EVENT_RESUME ? "PM" : "USR");                

        if(host->hw->flags & MSDC_SYS_SUSPEND) { /* will not set for WIFI */
//#ifdef MTK_EMMC_SUPPORT
            //msdc_emmc_sleepawake(host, 1);
//#endif

#ifdef MTK_EMMC_SUPPORT
            if((g_emmc_mode_switch == 0) && host->hw->host_function == MSDC_EMMC && host->mmc->card && mmc_card_mmc(host->mmc->card)){
                msdc_reset_hw(host->id);
                msdc_pin_reset (host, MSDC_PIN_PULL_UP);
                msdc_pin_config(host, MSDC_PIN_PULL_UP);
                host->power_control(host,1);
                mdelay(10);
                msdc_restore_emmc_setting(host);
            }
#else
            if(host->hw->host_function == MSDC_EMMC && host->mmc->card && mmc_card_mmc(host->mmc->card)){
                msdc_reset_hw(host->id);
                msdc_pin_reset (host, MSDC_PIN_PULL_UP);
                msdc_pin_config(host, MSDC_PIN_PULL_UP);
                host->power_control(host,1);
                mdelay(10);
                msdc_restore_emmc_setting(host);
            }
#endif
            (void)mmc_resume_host(host->mmc);
        } else { 
            host->mmc->pm_flags |= MMC_PM_IGNORE_PM_NOTIFY;         
            mmc_add_host(host->mmc);
        }
    }

end:
#ifdef SDIO_ERROR_BYPASS    
	if(is_card_sdio(host)){
		host->sdio_error = 0;	
	}
#endif                	    
    // gate clock at the last step when suspend.
    if ((evt == PM_EVENT_SUSPEND) || (evt == PM_EVENT_USER_SUSPEND)) {
        msdc_gate_clock(host, 0);
        
    }else {
        msdc_gate_clock(host, 1);
    }           
}
static u64 msdc_get_user_capacity(struct msdc_host *host)
{
    u64 device_capacity = 0;
    u32 device_legacy_capacity = 0;
    struct mmc_host* mmc = NULL;
    BUG_ON(!host);
    BUG_ON(!host->mmc);
    BUG_ON(!host->mmc->card);
    mmc = host->mmc;
    if(mmc_card_mmc(mmc->card)){
        if(mmc->card->csd.read_blkbits){
            device_legacy_capacity = mmc->card->csd.capacity * (2 << (mmc->card->csd.read_blkbits - 1));
        } else{
            device_legacy_capacity = mmc->card->csd.capacity;
            ERR_MSG("XXX read_blkbits = 0 XXX");
        }
        device_capacity = (u64)(mmc->card->ext_csd.sectors)* 512 > device_legacy_capacity ? (u64)(mmc->card->ext_csd.sectors)* 512 : device_legacy_capacity;
    }
    else if(mmc_card_sd(mmc->card)){
        device_capacity = (u64)(mmc->card->csd.capacity) << (mmc->card->csd.read_blkbits);
    }
    return device_capacity;
}

#ifdef MTK_EMMC_SUPPORT
u8 ext_csd[512];
int offset = 0;
char partition_access = 0;

static int msdc_get_data(u8* dst,struct mmc_data *data)
{
    int left;
    u8* ptr;
    struct scatterlist *sg = data->sg;
    int num = data->sg_len;
    while (num) {
        left = sg_dma_len(sg);
        ptr = (u8*)sg_virt(sg);
        memcpy(dst,ptr,left);
        sg = sg_next(sg); 
        dst+=left;
        num--;    
    }
    return 0;
}

/* parse part_info struct, support otp & mtk reserve */
static struct excel_info* msdc_reserve_part_info(unsigned char* name)
{
    int i;

    /* find reserve partition */
    for (i = 0; i < PART_NUM; i++) {
        printk("name = %s\n", PartInfo[i].name);  //====================debug
        if (0 == strcmp(name, PartInfo[i].name)){
            printk("size = %llu\n", PartInfo[i].size);//=======================debug
            return &PartInfo[i];
        }
    }

    return NULL;
}
static u32 msdc_get_other_capacity(void)
{
    u32 device_other_capacity = 0;
    device_other_capacity = ext_csd[EXT_CSD_BOOT_SIZE_MULT]* 128 * 1024
                + ext_csd[EXT_CSD_BOOT_SIZE_MULT] * 128 * 1024
                + ext_csd[EXT_CSD_RPMB_SIZE_MULT] * 128 * 1024
                + ext_csd[EXT_CSD_GP1_SIZE_MULT + 2] * 256 * 256 
                + ext_csd[EXT_CSD_GP1_SIZE_MULT + 1] * 256 
                + ext_csd[EXT_CSD_GP1_SIZE_MULT + 0]
                + ext_csd[EXT_CSD_GP2_SIZE_MULT + 2] * 256 * 256 
                + ext_csd[EXT_CSD_GP2_SIZE_MULT + 1] * 256 
                + ext_csd[EXT_CSD_GP2_SIZE_MULT + 0]
                + ext_csd[EXT_CSD_GP3_SIZE_MULT + 2] * 256 * 256 
                + ext_csd[EXT_CSD_GP3_SIZE_MULT + 1] * 256 
                + ext_csd[EXT_CSD_GP3_SIZE_MULT + 0]
                + ext_csd[EXT_CSD_GP4_SIZE_MULT + 2] * 256 * 256 
                + ext_csd[EXT_CSD_GP4_SIZE_MULT + 1] * 256 
                + ext_csd[EXT_CSD_GP4_SIZE_MULT + 0];

    return device_other_capacity;
}

int msdc_get_offset(void)
{
    u32 l_offset;

    l_offset =  MBR_START_ADDRESS_BYTE - msdc_get_other_capacity();

    return (l_offset >> 9);
}
EXPORT_SYMBOL(msdc_get_offset);

int msdc_get_reserve(void)
{
    u32 l_offset;
    struct excel_info* lp_excel_info;
    u32 l_otp_reserve = 0;
    u32 l_mtk_reserve = 0;

    l_offset = msdc_get_offset(); //==========check me

    lp_excel_info = msdc_reserve_part_info("bmtpool");
    if (NULL == lp_excel_info) {
        printk("can't get otp info from part_info struct\n");
        return -1;
    }

    l_mtk_reserve = (unsigned int)(lp_excel_info->start_address & 0xFFFFUL) << 8; /* unit is 512B */

    printk("mtk reserve: start address = %llu\n", lp_excel_info->start_address); //============================debug
#ifdef MTK_EMMC_SUPPORT_OTP
    lp_excel_info = msdc_reserve_part_info("otp");
    if (NULL == lp_excel_info) {
        printk("can't get otp info from part_info struct\n");
        return -1;
    }

    l_otp_reserve = (unsigned int)(lp_excel_info->start_address & 0xFFFFUL) << 8; /* unit is 512B */

    printk("otp reserve: start address = %llu\n", lp_excel_info->start_address);//========================debug
    l_otp_reserve -= l_mtk_reserve;  /* the size info stored with total reserved size */
#endif 

    printk("total reserve: l_otp_reserve = 0x%x blocks, l_mtk_reserve = 0x%x blocks, l_offset = 0x%x blocks\n", 
             l_otp_reserve, l_mtk_reserve, l_offset);

    return (l_offset + l_otp_reserve + l_mtk_reserve);
}
EXPORT_SYMBOL(msdc_get_reserve);

static bool msdc_cal_offset(struct msdc_host *host)
{
    u64 device_capacity = 0;
    
    offset =  MBR_START_ADDRESS_BYTE - msdc_get_other_capacity();
    device_capacity = msdc_get_user_capacity(host);
    if(mmc_card_blockaddr(host->mmc->card))
        offset /= 512;
    ERR_MSG("Address offset in USER REGION(Capacity %lld MB) is 0x%x",device_capacity/(1024*1024),offset);
    if(offset < 0) {
        ERR_MSG("XXX Address offset error(%d),please check MBR start address!!",(int)offset);
        BUG();
    }
    return true;
}
#endif

u64 msdc_get_capacity(int get_emmc_total)
{
    u64 user_size = 0;
    u32 other_size = 0;
    u64 total_size = 0;
    int index = 0;
    for(index = 0;index < HOST_MAX_NUM;++index){
        if((mtk_msdc_host[index] != NULL) && (mtk_msdc_host[index]->hw->boot)){
            user_size = msdc_get_user_capacity(mtk_msdc_host[index]);
#ifdef MTK_EMMC_SUPPORT
            if(get_emmc_total){
                if(mmc_card_mmc(mtk_msdc_host[index]->mmc->card))
                    other_size = msdc_get_other_capacity();
            }
#endif
            break;
        }
    }
    total_size = user_size + (u64)other_size; 
    return total_size/512; 
}
EXPORT_SYMBOL(msdc_get_capacity);

u32 erase_start = 0;
u32 erase_end = 0;
u32 erase_bypass = 0;
extern     int mmc_erase_group_aligned(struct mmc_card *card, unsigned int from,unsigned int nr);

/*--------------------------------------------------------------------------*/
/* mmc_host_ops members */
/*--------------------------------------------------------------------------*/
static unsigned int msdc_command_start(struct msdc_host   *host, 
                                      struct mmc_command *cmd,
                                      int                 tune,   /* not used */
                                      unsigned long       timeout)
{
    u32 base = host->base;
    u32 opcode = cmd->opcode;
    u32 rawcmd;
    u32 rawarg;
    u32 resp;  
    u32 wints_cmd = MSDC_INT_CMDRDY  | MSDC_INT_RSPCRCERR  | MSDC_INT_CMDTMO  |  
                    MSDC_INT_ACMDRDY | MSDC_INT_ACMDCRCERR | MSDC_INT_ACMDTMO; 
    unsigned long tmo;
    unsigned long flags_lock = 0;

    /* Protocol layer does not provide response type, but our hardware needs 
     * to know exact type, not just size!
     */
    if (opcode == MMC_SEND_OP_COND || opcode == SD_APP_OP_COND){
        resp = RESP_R3;
    }else if (opcode == MMC_SET_RELATIVE_ADDR || opcode == SD_SEND_RELATIVE_ADDR){
        resp = (mmc_cmd_type(cmd) == MMC_CMD_BCR) ? RESP_R6 : RESP_R1;
    }else if (opcode == MMC_FAST_IO){
        resp = RESP_R4;
    }else if (opcode == MMC_GO_IRQ_STATE){
        resp = RESP_R5;
    }else if (opcode == MMC_SELECT_CARD) {
        resp = (cmd->arg != 0) ? RESP_R1B : RESP_NONE;
        host->app_cmd_arg = cmd->arg;
        printk(KERN_WARNING "msdc%d select card<0x%.8x>", host->id,cmd->arg);  // select and de-select                     
    } else if (opcode == SD_IO_RW_DIRECT || opcode == SD_IO_RW_EXTENDED){
        resp = RESP_R1; /* SDIO workaround. */
    }else if (opcode == SD_SEND_IF_COND && (mmc_cmd_type(cmd) == MMC_CMD_BCR)){
        resp = RESP_R1;
    }else {
        switch (mmc_resp_type(cmd)) {
        case MMC_RSP_R1:
            resp = RESP_R1;
            break;
        case MMC_RSP_R1B:
            resp = RESP_R1B;
            break;
        case MMC_RSP_R2:
            resp = RESP_R2;
            break;
        case MMC_RSP_R3:
            resp = RESP_R3;
            break;
        case MMC_RSP_NONE:
        default:
            resp = RESP_NONE;              
            break;
        }
    }

    cmd->error = 0;
    /* rawcmd :
     * vol_swt << 30 | auto_cmd << 28 | blklen << 16 | go_irq << 15 | 
     * stop << 14 | rw << 13 | dtype << 11 | rsptyp << 7 | brk << 6 | opcode
     */    
    rawcmd = opcode | msdc_rsp[resp] << 7 | host->blksz << 16;
    
    if (opcode == MMC_READ_MULTIPLE_BLOCK) {
        rawcmd |= (2 << 11);
        if (host->autocmd & MSDC_AUTOCMD12)
            rawcmd |= (1 << 28);
#ifdef MTK_MSDC_USE_CMD23
        else if((host->autocmd & MSDC_AUTOCMD23))
            rawcmd |= (1 << 29);
#endif /* end of MTK_MSDC_USE_CMD23 */
    } else if (opcode == MMC_READ_SINGLE_BLOCK) {
        rawcmd |= (1 << 11);
    } else if (opcode == MMC_WRITE_MULTIPLE_BLOCK) {
        rawcmd |= ((2 << 11) | (1 << 13));
        if (host->autocmd & MSDC_AUTOCMD12)
            rawcmd |= (1 << 28);
#ifdef MTK_MSDC_USE_CMD23
        else if((host->autocmd & MSDC_AUTOCMD23))
            rawcmd |= (1 << 29);
#endif /* end of MTK_MSDC_USE_CMD23 */
    } else if (opcode == MMC_WRITE_BLOCK) {
        rawcmd |= ((1 << 11) | (1 << 13));
    } else if (opcode == SD_IO_RW_EXTENDED) {
        if (cmd->data->flags & MMC_DATA_WRITE)
            rawcmd |= (1 << 13);
        if (cmd->data->blocks > 1)
            rawcmd |= (2 << 11);
        else
            rawcmd |= (1 << 11);
    } else if (opcode == SD_IO_RW_DIRECT && cmd->flags == (unsigned int)-1) {
        rawcmd |= (1 << 14);
    } else if (opcode == SD_SWITCH_VOLTAGE) {
        rawcmd |= (1 << 30);
    } else if ((opcode == SD_APP_SEND_SCR) || 
        (opcode == SD_APP_SEND_NUM_WR_BLKS) ||
        (opcode == SD_SWITCH && (mmc_cmd_type(cmd) == MMC_CMD_ADTC)) ||
        (opcode == SD_APP_SD_STATUS && (mmc_cmd_type(cmd) == MMC_CMD_ADTC)) ||
        (opcode == MMC_SEND_EXT_CSD && (mmc_cmd_type(cmd) == MMC_CMD_ADTC))) {
        rawcmd |= (1 << 11);
    } else if (opcode == MMC_STOP_TRANSMISSION) {
        rawcmd |= (1 << 14);
        rawcmd &= ~(0x0FFF << 16);
    }

    N_MSG(CMD, "CMD<%d><0x%.8x> Arg<0x%.8x>", opcode , rawcmd, cmd->arg);

    tmo = jiffies + timeout;

    if (opcode == MMC_SEND_STATUS) {
        for (;;) {
            if (!sdc_is_cmd_busy())
                break;
                
            if (time_after(jiffies, tmo)) {
                ERR_MSG("XXX cmd_busy timeout: before CMD<%d>", opcode);    
                cmd->error = (unsigned int)-ETIMEDOUT;
                msdc_reset_hw(host->id);
                return cmd->error;  /* Fix me: error handling */
            } 
        }
    } else {
        for (;;) {     
            if (!sdc_is_busy())
                break;
            if (time_after(jiffies, tmo)) {
                ERR_MSG("XXX sdc_busy timeout: before CMD<%d>", opcode);    
                cmd->error = (unsigned int)-ETIMEDOUT;
                msdc_reset_hw(host->id);
                return cmd->error;    
            }   
        }    
    }   
    
    //BUG_ON(in_interrupt());
    host->cmd     = cmd;
    host->cmd_rsp = resp;        

    /* use polling way */
    if (host->hw->flags & MSDC_SDIO_IRQ)
        spin_lock_irqsave(&SDIO_IRQ_lock, flags_lock);
    sdr_clr_bits(MSDC_INTEN, wints_cmd);             
    if (host->hw->flags & MSDC_SDIO_IRQ)
        spin_unlock_irqrestore(&SDIO_IRQ_lock, flags_lock);

    if((opcode == SD_IO_SEND_OP_COND) && (is_card_sdio(host))) {
        cmd->arg |= R4_18V_PRESENT;
    }
    rawarg = cmd->arg;
#ifdef MTK_EMMC_SUPPORT
    if(host->hw->host_function == MSDC_EMMC              &&
                host->hw->boot == MSDC_BOOT_EN           &&
                  (cmd->opcode == MMC_READ_SINGLE_BLOCK    || 
                   cmd->opcode == MMC_READ_MULTIPLE_BLOCK  ||
                   cmd->opcode == MMC_WRITE_BLOCK          ||
                   cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK ||
                   cmd->opcode == MMC_ERASE_GROUP_START    ||
                   cmd->opcode == MMC_ERASE_GROUP_END)   && 
                   (partition_access == 0)) {
        if(cmd->arg == 0)
            msdc_cal_offset(host);    
        rawarg  += offset;
        if(cmd->opcode == MMC_ERASE_GROUP_START)
            erase_start = rawarg;
        if(cmd->opcode == MMC_ERASE_GROUP_END)
            erase_end = rawarg;
    }   
     
    if(cmd->opcode == MMC_ERASE                                          && 
         (cmd->arg == MMC_SECURE_ERASE_ARG || cmd->arg == MMC_ERASE_ARG) && 
           host->mmc->card                                               && 
           host->hw->host_function == MSDC_EMMC                          &&
           host->hw->boot == MSDC_BOOT_EN                                && 
           (!mmc_erase_group_aligned(host->mmc->card, erase_start, erase_end - erase_start + 1))){
        if(mmc_can_trim(host->mmc->card)){
            if(cmd->arg == MMC_SECURE_ERASE_ARG && mmc_can_secure_erase_trim(host->mmc->card))
                rawarg = MMC_SECURE_TRIM1_ARG;
            else if(cmd->arg == MMC_ERASE_ARG ||(cmd->arg == MMC_SECURE_ERASE_ARG && !mmc_can_secure_erase_trim(host->mmc->card)))
                rawarg = MMC_TRIM_ARG;
        }else {
            erase_bypass = 1; 
            ERR_MSG("cancel format,cmd<%d> arg=0x%x, start=0x%x, end=0x%x, size=%d, erase_bypass=%d", 
                    cmd->opcode, rawarg, erase_start, erase_end, (erase_end - erase_start + 1), erase_bypass); 
            goto end; 
        }
    }
#endif

    sdc_send_cmd(rawcmd, rawarg);        

end:        
    return 0;  // irq too fast, then cmd->error has value, and don't call msdc_command_resp, don't tune. 
}

static unsigned int msdc_command_resp_polling(struct msdc_host   *host, 
        struct mmc_command *cmd,
        int                 tune,
        unsigned long       timeout)
{
    u32 base = host->base;
    u32 intsts;
    u32 resp;
    //u32 status;
    unsigned long tmo;
    //struct mmc_data   *data = host->data;
    u32 cmdsts = MSDC_INT_CMDRDY  | MSDC_INT_RSPCRCERR  | MSDC_INT_CMDTMO; 
#ifdef MTK_MSDC_USE_CMD23
    struct mmc_command *sbc =  NULL;
#endif    

    if(erase_bypass && (cmd->opcode == MMC_ERASE)){
        erase_bypass = 0; 
        ERR_MSG("bypass cmd<%d>, erase_bypass=%d", cmd->opcode, erase_bypass); 
        goto out; 
    }
    
#ifdef MTK_MSDC_USE_CMD23
    if (host->autocmd & MSDC_AUTOCMD23){
        if (host->data && host->data->mrq && host->data->mrq->sbc)
            sbc =  host->data->mrq->sbc;
        
        /* autocmd interupt disabled, used polling way */
        cmdsts |= MSDC_INT_ACMDCRCERR | MSDC_INT_ACMDTMO;
    }
#endif

    resp = host->cmd_rsp;

    /*polling*/
    tmo = jiffies + timeout;
    while (1){
        if (((intsts = sdr_read32(MSDC_INT)) & cmdsts) != 0){
            /* clear all int flag */
#ifdef MTK_MSDC_USE_CMD23
            // need clear autocmd23 comand ready interrupt
            intsts &= (cmdsts | MSDC_INT_ACMDRDY);
#else
            intsts &= cmdsts;
#endif
            sdr_write32(MSDC_INT, intsts);
            break;
        }
        
        if (time_after(jiffies, tmo)) {
            ERR_MSG("XXX CMD<%d> polling_for_completion timeout ARG<0x%.8x>", cmd->opcode, cmd->arg);    
            cmd->error = (unsigned int)-ETIMEDOUT;
            host->sw_timeout++;
            msdc_dump_info(host->id); 
            msdc_reset_hw(host->id);
            goto out;    
        }   
    }

    /* command interrupts */
    if (intsts & cmdsts) {
#ifdef MTK_MSDC_USE_CMD23
        if ((intsts & MSDC_INT_CMDRDY) || (intsts & MSDC_INT_ACMD19_DONE)) {
#else
        if ((intsts & MSDC_INT_CMDRDY) || (intsts & MSDC_INT_ACMDRDY) || 
            (intsts & MSDC_INT_ACMD19_DONE)) {
#endif
            u32 *rsp = NULL;
            rsp = &cmd->resp[0];
            switch (host->cmd_rsp) {
                case RESP_NONE:
                    break;
                case RESP_R2:
                    *rsp++ = sdr_read32(SDC_RESP3); *rsp++ = sdr_read32(SDC_RESP2);
                    *rsp++ = sdr_read32(SDC_RESP1); *rsp++ = sdr_read32(SDC_RESP0);
                    break;
                default: /* Response types 1, 3, 4, 5, 6, 7(1b) */
                    *rsp = sdr_read32(SDC_RESP0);    
                    break;
            }
        } else if (intsts & MSDC_INT_RSPCRCERR) {
            cmd->error = (unsigned int)-EIO;
            IRQ_MSG("XXX CMD<%d> MSDC_INT_RSPCRCERR Arg<0x%.8x>",cmd->opcode, cmd->arg);
            msdc_reset_hw(host->id); 
        } else if (intsts & MSDC_INT_CMDTMO) {
            cmd->error = (unsigned int)-ETIMEDOUT;
            IRQ_MSG("XXX CMD<%d> MSDC_INT_CMDTMO Arg<0x%.8x>",cmd->opcode, cmd->arg);
            msdc_reset_hw(host->id); 
        }
#ifdef MTK_MSDC_USE_CMD23
        if ((sbc != NULL) && (host->autocmd & MSDC_AUTOCMD23)) {
             if (intsts & MSDC_INT_ACMDRDY) {
                  u32 *arsp = &sbc->resp[0];
                  *arsp = sdr_read32(SDC_ACMD_RESP);
             } else if (intsts & MSDC_INT_ACMDCRCERR) {
                 printk("autocmd23 crc error\n");
                  sbc->error = (unsigned int)-EIO;
                  cmd->error = (unsigned int)-EIO; // record the error info in current cmd struct 
                  //host->error |= REQ_CMD23_EIO;
                  msdc_reset_hw(host->id); 
             } else if (intsts & MSDC_INT_ACMDTMO) {
                 printk("autocmd23 to error\n");
                  sbc->error =(unsigned int)-ETIMEDOUT;
                  cmd->error = (unsigned int)-ETIMEDOUT;  // record the error info in current cmd struct
                  //host->error |= REQ_CMD23_TMO;
                  msdc_reset_hw(host->id); 
             }
        }
#endif /* end of MTK_MSDC_USE_CMD23 */
    }
out:
    host->cmd = NULL;

    return cmd->error;
}
#if 0
static unsigned int msdc_command_resp(struct msdc_host   *host, 
                                      struct mmc_command *cmd,
                                      int                 tune,
                                      unsigned long       timeout)
{
    u32 base = host->base;
    u32 opcode = cmd->opcode;
    //u32 resp = host->cmd_rsp;    
    //u32 tmo;
    //u32 intsts;
  
    spin_unlock(&host->lock);   
    if(!wait_for_completion_timeout(&host->cmd_done, 10*timeout)){       
        ERR_MSG("XXX CMD<%d> wait_for_completion timeout ARG<0x%.8x>", opcode, cmd->arg);
        host->sw_timeout++;
        msdc_dump_info(host->id);          
        cmd->error = (unsigned int)-ETIMEDOUT;
        msdc_reset_hw(host->id);
    }    
    spin_lock(&host->lock);

    sdr_clr_bits(MSDC_INTEN, wints_cmd);
    host->cmd = NULL;
    /* if (resp == RESP_R1B) {
        while ((sdr_read32(MSDC_PS) & 0x10000) != 0x10000);       
    } */ 
                        
    return cmd->error;
}                                   
#endif
unsigned int msdc_do_command(struct msdc_host   *host, 
                                      struct mmc_command *cmd,
                                      int                 tune,
                                      unsigned long       timeout)
{
    if((cmd->opcode == MMC_GO_IDLE_STATE) && (host->hw->host_function == MSDC_SD)){
        mdelay(10);
    }
    
    if (msdc_command_start(host, cmd, tune, timeout)) 
        goto end;      
    if (msdc_command_resp_polling(host, cmd, tune, timeout)) 
        goto end;
end:    

    N_MSG(CMD, "        return<%d> resp<0x%.8x>", cmd->error, cmd->resp[0]);     
    return cmd->error;
}
    
/* The abort condition when PIO read/write 
   tmo: 
*/
static int msdc_pio_abort(struct msdc_host *host, struct mmc_data *data, unsigned long tmo)
{
    int  ret = 0;     
    u32  base = host->base;
    
    if (atomic_read(&host->abort)) {    
        ret = 1;
    }    

    if (time_after(jiffies, tmo)) {
        data->error = (unsigned int)-ETIMEDOUT;
        ERR_MSG("XXX PIO Data Timeout: CMD<%d>", host->mrq->cmd->opcode);
        msdc_dump_info(host->id);          
        ret = 1;        
    }

    if(ret) {
        msdc_reset_hw(host->id);         
        ERR_MSG("msdc pio find abort");      
    }
    return ret; 
}

/*
   Need to add a timeout, or WDT timeout, system reboot.      
*/
// pio mode data read/write
int msdc_pio_read(struct msdc_host *host, struct mmc_data *data)
{
    struct scatterlist *sg = data->sg;
    u32  base = host->base;
    u32  num = data->sg_len;
    u32 *ptr;
    u8  *u8ptr;
    u32  left = 0;
    u32  count, size = 0;
    u32  wints = MSDC_INTEN_DATTMO | MSDC_INTEN_DATCRCERR | MSDC_INTEN_XFER_COMPL;
    u32  ints = 0;
    bool get_xfer_done = 0;
    unsigned long tmo = jiffies + DAT_TIMEOUT;  
    struct page *hmpage=NULL;
    int i, subpage,totalpages=0;
    int flag=0;
    unsigned int kaddr[DIV_ROUND_UP(MAX_SGMT_SZ,PAGE_SIZE)];


    //sdr_clr_bits(MSDC_INTEN, wints);
    while (1) {
        if(!get_xfer_done){
            ints = sdr_read32(MSDC_INT);
            ints &= wints;
            sdr_write32(MSDC_INT,ints);
        }
        if(ints & MSDC_INT_DATTMO){
            data->error = (unsigned int)-ETIMEDOUT;
            msdc_reset_hw(host->id); 
            break;
        } else if(ints & MSDC_INT_DATCRCERR){
            data->error = (unsigned int)-EIO;
            msdc_reset_hw(host->id); 
            break;
        } else if(ints & MSDC_INT_XFER_COMPL){
            get_xfer_done = 1;
            if((num == 0) && (left == 0))    
                break;
        }
        if(msdc_pio_abort(host, data, tmo)) 
                goto end;     
        if((num == 0) && (left == 0))
            continue;
        left = sg_dma_len(sg);
        ptr = sg_virt(sg);
        flag = 0;
        
        if(ptr==NULL||(PageHighMem((struct page *)(sg->page_link & ~0x3))))
        {
            hmpage = (struct page *)(sg->page_link & ~0x3);
            totalpages = DIV_ROUND_UP((left+sg->offset), PAGE_SIZE);   
            subpage = (left + sg->offset)%PAGE_SIZE;
            
            if(subpage!=0|| (sg->offset!=0))
                N_MSG(OPS, "msdc0: This read size or start not align %x,%x, hmpage %x",subpage,left,(unsigned int)hmpage);
            
            
            for(i=0;i< totalpages;i++)
            {
                kaddr[i] = (unsigned int)kmap(hmpage + i);    
                if(i>0)
                {
                    if((kaddr[i]-kaddr[i-1])!=PAGE_SIZE)
                    {
                        //printk(KERN_ERR "msdc0: kmap not continous %x %x %x \n",left,kaddr[i],kaddr[i-1]);
                        flag =1;
                    }
                }
                if(!kaddr[i])
                    ERR_MSG("msdc0:kmap failed %x", kaddr[i]);
            
            }
            
            
            ptr = sg_virt(sg);
            
            if(ptr==NULL)
                ERR_MSG("msdc0:sg_virt %x", ptr);
            
            
            if (flag==1)  // High memory and more than 1 va address va and  not continous
            {
                for(i=0;i<totalpages;i++)
                {              
                    left = PAGE_SIZE;
                    ptr = (u32*)kaddr[i];
                    
                    if(i==0)
                    {
                        left = PAGE_SIZE-sg->offset;
                        ptr = (u32*)(kaddr[i]+sg->offset);
                    }
                    if(subpage!=0&&(i==(totalpages-1)))
                    {
                        left = subpage;
                    }
                    
                    
                    while (left) {
                        if ((left >=  MSDC_FIFO_THD) && (msdc_rxfifocnt() >= MSDC_FIFO_THD)) {
                            count = MSDC_FIFO_THD >> 2;
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
                                * u8ptr++ = msdc_fifo_read8();
                                left--;       
                            }
                        }else {
                            ints = sdr_read32(MSDC_INT);

                            if ((ints & MSDC_INT_DATCRCERR) || (ints & MSDC_INT_DATTMO)) {
                                if (ints & MSDC_INT_DATCRCERR){
                                    ERR_MSG("[msdc%d] DAT CRC error (0x%x), Left DAT: %d bytes\n",  host->id, ints, left);
                                    data->error = (unsigned int)-EIO;
                                }
                                if (ints & MSDC_INT_DATTMO){
                                    ERR_MSG("[msdc%d] DAT TMO error (0x%x), Left DAT: %d bytes\n",  host->id, ints, left);
                                    data->error = (unsigned int)-ETIMEDOUT;
                                }

                                msdc_dump_info(host->id);
                                sdr_write32(MSDC_INT, ints);
                                msdc_reset_hw(host->id);
                                goto end;
                            }
                        }
                        
                        if (msdc_pio_abort(host, data, tmo)) {
                            goto end;     
                        }
                    }    
                }
            }
        
        }
         while (left) {
            if ((left >=  MSDC_FIFO_THD) && (msdc_rxfifocnt() >= MSDC_FIFO_THD)) {
                count = MSDC_FIFO_THD >> 2;
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
                    * u8ptr++ = msdc_fifo_read8();
                    left--;       
                }
            }else {
                ints = sdr_read32(MSDC_INT);

                if ((ints & MSDC_INT_DATCRCERR) || (ints & MSDC_INT_DATTMO)) {
                    if (ints & MSDC_INT_DATCRCERR){
                        ERR_MSG("[msdc%d] DAT CRC error (0x%x), Left DAT: %d bytes\n",  host->id, ints, left);
                        data->error = (unsigned int)-EIO;
                   }
                   if (ints & MSDC_INT_DATTMO){
                        ERR_MSG("[msdc%d] DAT TMO error (0x%x), Left DAT: %d bytes\n",  host->id, ints, left);
                        data->error = (unsigned int)-ETIMEDOUT;
                   }

                   msdc_dump_info(host->id);
                   sdr_write32(MSDC_INT, ints);
                   msdc_reset_hw(host->id);
                   goto end;
               }
            }
            
            if (msdc_pio_abort(host, data, tmo)) {
                goto end;     
            }
        }
        if(hmpage !=NULL)
        {
            //printk(KERN_ERR "read msdc0:unmap %x\n", hmpage);
            for(i=0;i< totalpages;i++)
            kunmap(hmpage+i);
            
            hmpage = NULL;
        }
        size += sg_dma_len(sg);
        sg = sg_next(sg); 
        num--;
    }
end:
    if(hmpage !=NULL)
    {
        for(i=0;i< totalpages;i++)
        kunmap(hmpage + i);
        //printk(KERN_ERR "msdc0 read unmap:\n");
    }
    data->bytes_xfered += size;
    N_MSG(FIO, "        PIO Read<%d>bytes", size);
        
    //sdr_clr_bits(MSDC_INTEN, wints);    
#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT    
    if ((is_card_sdio(host)) && (atomic_read(&host->ot_work.autok_done) == 0))   // auto-K have not done or finished
        return data->error;
#endif

    if(data->error) 
        ERR_MSG("read pio data->error<%d> left<%d> size<%d>", data->error, left, size);
    return data->error;
}

/* please make sure won't using PIO when size >= 512 
   which means, memory card block read/write won't using pio
   then don't need to handle the CMD12 when data error. 
*/
int msdc_pio_write(struct msdc_host* host, struct mmc_data *data)
{
    u32  base = host->base;
    struct scatterlist *sg = data->sg;
    u32  num = data->sg_len;
    u32 *ptr;
    u8  *u8ptr;
    u32  left;
    u32  count, size = 0;
    u32  wints = MSDC_INTEN_DATTMO | MSDC_INTEN_DATCRCERR | MSDC_INTEN_XFER_COMPL; 
    bool get_xfer_done = 0;
    unsigned long tmo = jiffies + DAT_TIMEOUT;  
    u32 ints = 0;
    struct page *hmpage = NULL;
    int i, totalpages=0;
    int flag,subpage=0;
    unsigned int kaddr[DIV_ROUND_UP(MAX_SGMT_SZ,PAGE_SIZE)];

    //sdr_clr_bits(MSDC_INTEN, wints);
    while (1) {
        if(!get_xfer_done){
            ints = sdr_read32(MSDC_INT);
            ints &= wints;
            sdr_write32(MSDC_INT,ints);
        }
        if(ints & MSDC_INT_DATTMO){
            data->error = (unsigned int)-ETIMEDOUT;
            msdc_reset_hw(host->id); 
            break;
        }
        else if(ints & MSDC_INT_DATCRCERR){
            data->error = (unsigned int)-EIO;
            msdc_reset_hw(host->id); 
            break;
        }
        else if(ints & MSDC_INT_XFER_COMPL){
            get_xfer_done = 1;
            if((num == 0) && (left == 0))  
               break;
        }
        if(msdc_pio_abort(host, data, tmo)) 
            goto end;   
        if((num == 0) && (left == 0))
            continue;
        left = sg_dma_len(sg);
        ptr = sg_virt(sg);

        flag = 0;        
        // High memory must kmap, if already mapped, only add counter
        if(ptr==NULL||(PageHighMem((struct page *)(sg->page_link & ~0x3))))  
        {
            hmpage = (struct page *)(sg->page_link & ~0x3);
            totalpages = DIV_ROUND_UP(left+sg->offset, PAGE_SIZE);
            subpage = (left+sg->offset)%PAGE_SIZE;
            
            
            if( (subpage!=0)|| (sg->offset!=0))
                N_MSG(OPS, "msdc0: This write size or start not align %d,%d, hmpage %x,sg offset %x\n",subpage,left,(unsigned int)hmpage,sg->offset);
            
            
            // Kmap all need pages,  
            for(i=0;i< totalpages;i++)
            {
                kaddr[i] = (unsigned int)kmap(hmpage + i);
                if(i>0)
                {
                    if(kaddr[i]-kaddr[i-1]!=PAGE_SIZE)
                    {
                    //printk(KERN_ERR "msdc0:w kmap not continous %x ,%x, %x\n",left,kaddr[i],kaddr[i-1]);
                    flag =1;    
                    }
                }
                if(!kaddr[i])
                    ERR_MSG("msdc0:kmap failed %x\n", kaddr[i]);
            }
            
            ptr = sg_virt(sg);    
            
            if(ptr==NULL)
                ERR_MSG("msdc0:write sg_virt %x\n", ptr);
            
            
            // High memory and more than 1 va address va may be not continous
            if (flag==1)
            {            
                for(i=0;i< totalpages;i++)
                {
                
                    left = PAGE_SIZE;
                    ptr = (u32*)kaddr[i];
                    
                    if(i==0)
                    {
                        left = PAGE_SIZE-sg->offset;
                        ptr = (u32*)(kaddr[i]+sg->offset);
                    }
                    if (subpage!=0 && (i==(totalpages-1)))
                    {
                        left = subpage;
                    }
                    
                    while (left) {
                        if (left >= MSDC_FIFO_SZ && msdc_txfifocnt() == 0) {
                            count = MSDC_FIFO_SZ >> 2;
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
                            ints = sdr_read32(MSDC_INT);

                            if ((ints & MSDC_INT_DATCRCERR) || (ints & MSDC_INT_DATTMO)) {
                                if (ints & MSDC_INT_DATCRCERR){
                                    ERR_MSG("[msdc%d] DAT CRC error (0x%x), Left DAT: %d bytes\n",  host->id, ints, left);
                                    data->error = (unsigned int)-EIO;
                                }
                                if (ints & MSDC_INT_DATTMO){
                                    ERR_MSG("[msdc%d] DAT TMO error (0x%x), Left DAT: %d bytes\n",  host->id, ints, left);
                                    data->error = (unsigned int)-ETIMEDOUT;
                                }

                                msdc_dump_info(host->id);
                                sdr_write32(MSDC_INT, ints);
                                msdc_reset_hw(host->id);
                                goto end;
                            }
                        }
                        
                        if (msdc_pio_abort(host, data, tmo)) {
                            goto end;     
                        }                   
                    }    
                }
            }
        }
        while (left) {   
           if (left >= MSDC_FIFO_SZ && msdc_txfifocnt() == 0) {
               count = MSDC_FIFO_SZ >> 2;
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
                  msdc_fifo_write8(*u8ptr);  u8ptr++;
                  left--;
               }
           }else {
                ints = sdr_read32(MSDC_INT);

                if ((ints & MSDC_INT_DATCRCERR) || (ints & MSDC_INT_DATTMO)) {
                    if (ints & MSDC_INT_DATCRCERR){
                        ERR_MSG("[msdc%d] DAT CRC error (0x%x), Left DAT: %d bytes\n",  host->id, ints, left);
                        data->error = (unsigned int)-EIO;
                   }
                   if (ints & MSDC_INT_DATTMO){
                        ERR_MSG("[msdc%d] DAT TMO error (0x%x), Left DAT: %d bytes\n",  host->id, ints, left);
                        data->error = (unsigned int)-ETIMEDOUT;
                   }

                   msdc_dump_info(host->id);
                   sdr_write32(MSDC_INT, ints);
                   msdc_reset_hw(host->id);
                   goto end;
               }
           }
            
           if (msdc_pio_abort(host, data, tmo)) {
               goto end;   
           }                   
        }
        if(hmpage!=NULL)
        {
            for(i=0;i< totalpages;i++)
            kunmap(hmpage + i);
            
            hmpage = NULL;
        
        }
        size += sg_dma_len(sg);
        sg = sg_next(sg);
        num--;
    }
end:    
    if(hmpage !=NULL)
    {
        for(i=0;i< totalpages;i++)
        kunmap(hmpage + i);
        printk(KERN_ERR "msdc0 write unmap 0x%x:\n",left);
    }
    data->bytes_xfered += size;
    N_MSG(FIO, "        PIO Write<%d>bytes", size);
    
#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT    
    if ((is_card_sdio(host)) && (atomic_read(&host->ot_work.autok_done) == 0))   // auto-K have not done or finished
        return data->error;
#endif

    if(data->error) //ERR_MSG("write pio data->error<%d>", data->error);
        ERR_MSG("write pio data->error<%d> left<%d> size<%d>", data->error, left, size);
      
    //sdr_clr_bits(MSDC_INTEN, wints);  
    return data->error;  
}

#if 0
// DMA resume / start / stop 
static void msdc_dma_resume(struct msdc_host *host)
{
    u32 base = host->base;

    sdr_set_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_RESUME, 1);

    N_MSG(DMA, "DMA resume");
}
#endif

static void msdc_dma_start(struct msdc_host *host)
{
    unsigned long flags_lock = 0;
    u32 base = host->base;
    u32 wints = MSDC_INTEN_XFER_COMPL | MSDC_INTEN_DATTMO | MSDC_INTEN_DATCRCERR ; 
    /* handle autocmd12 error in msdc_irq */
    if(host->autocmd & MSDC_AUTOCMD12)
        wints |= MSDC_INT_ACMDCRCERR | MSDC_INT_ACMDTMO | MSDC_INT_ACMDRDY; 
    if (host->hw->flags & MSDC_SDIO_IRQ)
        spin_lock_irqsave(&SDIO_IRQ_lock, flags_lock);
    sdr_set_bits(MSDC_INTEN, wints);
    if (host->hw->flags & MSDC_SDIO_IRQ)
        spin_unlock_irqrestore(&SDIO_IRQ_lock, flags_lock);
    mb();
    sdr_set_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_START, 1);

    N_MSG(DMA, "DMA start");
}

static void msdc_dma_stop(struct msdc_host *host)
{
    unsigned long     flags_lock = 0;
    u32 base = host->base;
    int retry = 30;
    int count = 1000;
    //u32 retries=500;
    u32 wints = MSDC_INTEN_XFER_COMPL | MSDC_INTEN_DATTMO | MSDC_INTEN_DATCRCERR ; 
    /* handle autocmd12 error in msdc_irq */
    if(host->autocmd & MSDC_AUTOCMD12)
        wints |= MSDC_INT_ACMDCRCERR | MSDC_INT_ACMDTMO | MSDC_INT_ACMDRDY; 
    N_MSG(DMA, "DMA status: 0x%.8x",sdr_read32(MSDC_DMA_CFG));
    //while (sdr_read32(MSDC_DMA_CFG) & MSDC_DMA_CFG_STS);

    sdr_set_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_STOP, 1);
    //while (sdr_read32(MSDC_DMA_CFG) & MSDC_DMA_CFG_STS);
    msdc_retry((sdr_read32(MSDC_DMA_CFG) & MSDC_DMA_CFG_STS),retry,count,host->id);
    if(retry == 0){
        ERR_MSG("!!ASSERT!!");
        BUG();
    }
    mb();
    if (host->hw->flags & MSDC_SDIO_IRQ)
        spin_lock_irqsave(&SDIO_IRQ_lock, flags_lock);
    sdr_clr_bits(MSDC_INTEN, wints); /* Not just xfer_comp */
    if (host->hw->flags & MSDC_SDIO_IRQ)
        spin_unlock_irqrestore(&SDIO_IRQ_lock, flags_lock);

    N_MSG(DMA, "DMA stop");
}

#if 0
/* dump a gpd list */
static void msdc_dma_dump(struct msdc_host *host, struct msdc_dma *dma)
{
    gpd_t *gpd = dma->gpd; 
    bd_t   *bd = dma->bd;          
    bd_t   *ptr; 
    int i = 0; 
    int p_to_v; 
    
    if (dma->mode != MSDC_MODE_DMA_DESC) {
        return;     
    }    

    ERR_MSG("try to dump gpd and bd");

    /* dump gpd */
    ERR_MSG(".gpd<0x%.8x> gpd_phy<0x%.8x>", (int)gpd, (int)dma->gpd_addr);
    ERR_MSG("...hwo   <%d>", gpd->hwo );
    ERR_MSG("...bdp   <%d>", gpd->bdp );
    ERR_MSG("...chksum<0x%.8x>", gpd->chksum );
    //ERR_MSG("...intr  <0x%.8x>", gpd->intr );
    ERR_MSG("...next  <0x%.8x>", (int)gpd->next );
    ERR_MSG("...ptr   <0x%.8x>", (int)gpd->ptr );
    ERR_MSG("...buflen<0x%.8x>", gpd->buflen );
    //ERR_MSG("...extlen<0x%.8x>", gpd->extlen );
    //ERR_MSG("...arg   <0x%.8x>", gpd->arg );
    //ERR_MSG("...blknum<0x%.8x>", gpd->blknum );    
    //ERR_MSG("...cmd   <0x%.8x>", gpd->cmd );      

    /* dump bd */
    ERR_MSG(".bd<0x%.8x> bd_phy<0x%.8x> gpd_ptr<0x%.8x>", (int)bd, (int)dma->bd_addr, (int)gpd->ptr);  
    ptr = bd; 
    p_to_v = ((u32)bd - (u32)dma->bd_addr);
    while (1) {
        ERR_MSG(".bd[%d]", i); i++;              
        ERR_MSG("...eol   <%d>", ptr->eol );
        ERR_MSG("...chksum<0x%.8x>", ptr->chksum );
        //ERR_MSG("...blkpad<0x%.8x>", ptr->blkpad );
        //ERR_MSG("...dwpad <0x%.8x>", ptr->dwpad );
        ERR_MSG("...next  <0x%.8x>", (int)ptr->next );
        ERR_MSG("...ptr   <0x%.8x>", (int)ptr->ptr );
        ERR_MSG("...buflen<0x%.8x>", (int)ptr->buflen );
        
        if (ptr->eol == 1) {
            break;     
        }
                         
        /* find the next bd, virtual address of ptr->next */
        /* don't need to enable when use malloc */
        //BUG_ON( (ptr->next + p_to_v)!=(ptr+1) );         
        //ERR_MSG(".next bd<0x%.8x><0x%.8x>", (ptr->next + p_to_v), (ptr+1));
        ptr++;               
    }    
    
    ERR_MSG("dump gpd and bd finished");
}
#endif

/* calc checksum */
static u8 msdc_dma_calcs(u8 *buf, u32 len)
{
    u32 i, sum = 0;
    for (i = 0; i < len; i++) {
        sum += buf[i];
    }
    return 0xFF - (u8)sum;
}

#ifdef MSDC_DMA_BOUNDARY_LIMITAION

#define MSDC_DMA_BOUNDARY_SIZE                       (2048)
#define MSDC_DMA_START_ADDRESS_ALIGNMENT             (0)
#define MSDC_DMA_NOT_EXCEED_BOUNDARY                 (0)
#define MSDC_DMA_ACROSS_BOUNDARY                     (1)

u32 msdc_dma_address_check(u32 start, u32 len)
{
    /* mask bit0-1 */
    start &= (~0x3);

    /* two type of violation 2k boundary will be checked */
    if (0 == start % 64)
        return MSDC_DMA_START_ADDRESS_ALIGNMENT;
    
    if (len > (MSDC_DMA_BOUNDARY_SIZE - (start & 0x7FF))){
        /* cross boundary */
        return MSDC_DMA_ACROSS_BOUNDARY;
    }

    return MSDC_DMA_NOT_EXCEED_BOUNDARY;
}
#endif

/* gpd bd setup + dma registers */
static int msdc_dma_config(struct msdc_host *host, struct msdc_dma *dma)
{
    u32 base = host->base;
    u32 sglen = dma->sglen;
    //u32 i, j, num, bdlen, arg, xfersz;
    u32 j, num, bdlen;
    u32 dma_address, dma_len;
    u8  blkpad, dwpad, chksum;
    struct scatterlist *sg = dma->sg;
    gpd_t *gpd;
    bd_t *bd;
#ifdef MSDC_DMA_BOUNDARY_LIMITAION
    u32 ret;
    u32 dma_address_tmp, dma_len_tmp;
#endif

    switch (dma->mode) {
        case MSDC_MODE_DMA_BASIC:
            if (host->hw->host_function == MSDC_SDIO)
            {
                BUG_ON(dma->xfersz > 0xFFFFFFFF);
            }
            else
            {
                BUG_ON(dma->xfersz > 65535);
            }
            BUG_ON(dma->sglen != 1);
            dma_address = sg_dma_address(sg);
            dma_len = sg_dma_len(sg);
            sdr_write32(MSDC_DMA_SA, dma_address);

            sdr_set_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_LASTBUF, 1);
            sdr_write32(MSDC_DMA_LEN, dma_len);
            sdr_set_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_BRUSTSZ, dma->burstsz);
            sdr_set_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_MODE, 0);
            break;
        case MSDC_MODE_DMA_DESC:
            blkpad = (dma->flags & DMA_FLAG_PAD_BLOCK) ? 1 : 0;
            dwpad  = (dma->flags & DMA_FLAG_PAD_DWORD) ? 1 : 0;
            chksum = (dma->flags & DMA_FLAG_EN_CHKSUM) ? 1 : 0;

            /* calculate the required number of gpd */
            num = (sglen + MAX_BD_PER_GPD - 1) / MAX_BD_PER_GPD;        
            BUG_ON(num !=1 );        
        
            gpd = dma->gpd; 
            bd  = dma->bd; 
            bdlen = sglen; 

            /* modify gpd*/
            //gpd->intr = 0; 
            gpd->hwo = 1;  /* hw will clear it */
            gpd->bdp = 1;     
            gpd->chksum = 0;  /* need to clear first. */   
            gpd->chksum = (chksum ? msdc_dma_calcs((u8 *)gpd, 16) : 0);
        
            /* modify bd*/          
            for (j = 0; j < bdlen; j++) {
#ifdef MSDC_DMA_VIOLATION_DEBUG
                if (g_dma_debug[host->id] && (msdc_latest_operation_type[host->id] == OPER_TYPE_READ)){
                    printk("[%s] msdc%d do write 0x10000\n", __func__, host->id);
                    dma_address = 0x10000;
                } else{
                    dma_address = sg_dma_address(sg);
                }
#else
                dma_address = sg_dma_address(sg);
#endif

                dma_len = sg_dma_len(sg);

#ifdef MSDC_DMA_BOUNDARY_LIMITAION
               /* just for MT6582 */
               ret =  msdc_dma_address_check(dma_address, dma_len);
               if (ret == MSDC_DMA_ACROSS_BOUNDARY){
                   /* add debug info */
                   N_MSG(DMA, "across 2k boundary; start = 0x%x, len = %d\n", dma_address, dma_len);
                   
                   /* need spilt the BD to BD_1, BD_2, 
                    * rule: make sure the start address of BD_2 is 2K boundary */
                   dma_len_tmp = MSDC_DMA_BOUNDARY_SIZE - (dma_address & 0x7FF);
                   msdc_init_bd(&bd[j], blkpad, dwpad, dma_address, dma_len_tmp);           

                   N_MSG(DMA, "BD_1: start = 0x%x, len = %d\n", dma_address, dma_len_tmp);
               
                   bd[j].eol = 0;     
                   bd[j].chksum = 0; /* checksume need to clear first */
                   bd[j].chksum = (chksum ? msdc_dma_calcs((u8 *)(&bd[j]), 16) : 0);       
    
                   j += 1;
                   bdlen += 1;

                   dma_address_tmp = dma_address + dma_len_tmp;
                   dma_len_tmp = dma_len - dma_len_tmp;
                   msdc_init_bd(&bd[j], blkpad, dwpad, dma_address_tmp, dma_len_tmp);            

                   N_MSG(DMA, "BD_2: start = 0x%x, len = %d\n", dma_address_tmp, dma_len_tmp);
               
                   if(j == bdlen - 1) {
                       bd[j].eol = 1;         /* the last bd */
                   } else {
                       bd[j].eol = 0;     
                   }
                   bd[j].chksum = 0; /* checksume need to clear first */
                   bd[j].chksum = (chksum ? msdc_dma_calcs((u8 *)(&bd[j]), 16) : 0);       
               } else {
                   /* if no need change, take the old way */
                   msdc_init_bd(&bd[j], blkpad, dwpad, dma_address, dma_len);            

                   if(j == bdlen - 1) {
                       bd[j].eol = 1;         /* the last bd */
                   } else {
                       bd[j].eol = 0;     
                   }
                   bd[j].chksum = 0; /* checksume need to clear first */
                   bd[j].chksum = (chksum ? msdc_dma_calcs((u8 *)(&bd[j]), 16) : 0);       
               }
#else
               msdc_init_bd(&bd[j], blkpad, dwpad, dma_address, dma_len);            

               if(j == bdlen - 1) {
                   bd[j].eol = 1;         /* the last bd */
               } else {
                   bd[j].eol = 0;     
               }
               bd[j].chksum = 0; /* checksume need to clear first */
               bd[j].chksum = (chksum ? msdc_dma_calcs((u8 *)(&bd[j]), 16) : 0);         
#endif            
                sg++;
            }
#ifdef MSDC_DMA_VIOLATION_DEBUG
            if(g_dma_debug[host->id] && (msdc_latest_operation_type[host->id] == OPER_TYPE_READ))
                g_dma_debug[host->id] = 0;
#endif

            dma->used_gpd += 2;
            dma->used_bd += bdlen;  

            sdr_set_field(MSDC_DMA_CFG, MSDC_DMA_CFG_DECSEN, chksum);
            sdr_set_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_BRUSTSZ, dma->burstsz);
            sdr_set_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_MODE, 1);

            sdr_write32(MSDC_DMA_SA, (u32)dma->gpd_addr);               
            break;

        default:
            break;
    }
    
    N_MSG(DMA, "DMA_CTRL = 0x%x", sdr_read32(MSDC_DMA_CTRL));
    N_MSG(DMA, "DMA_CFG  = 0x%x", sdr_read32(MSDC_DMA_CFG));
    N_MSG(DMA, "DMA_SA   = 0x%x", sdr_read32(MSDC_DMA_SA));

    return 0;
} 

static void msdc_dma_setup(struct msdc_host *host, struct msdc_dma *dma, 
                           struct scatterlist *sg, unsigned int sglen)
{ 
#ifdef MSDC_DMA_BOUNDARY_LIMITAION
    u32 dma_address, dma_len, ret;
#endif
    u32 max_dma_len = 0;
    BUG_ON(sglen > MAX_BD_NUM); /* not support currently */

    dma->sg = sg;
    dma->flags = DMA_FLAG_EN_CHKSUM;
    //dma->flags = DMA_FLAG_NONE; /* CHECKME */
    dma->sglen = sglen;
    dma->xfersz = host->xfer_size;
    dma->burstsz = MSDC_BRUST_64B;

    if (host->hw->host_function == MSDC_SDIO)
    {
        max_dma_len = MAX_DMA_CNT_SDIO;
    }
    else
    {
        max_dma_len = MAX_DMA_CNT;
    }

    if (sglen == 1 && sg_dma_len(sg) <= max_dma_len){
#ifdef MSDC_DMA_BOUNDARY_LIMITAION
        dma_len = sg_dma_len(sg);
        dma_address = sg_dma_address(sg);
        
        ret = msdc_dma_address_check(dma_address, dma_len);
        if (ret == MSDC_DMA_ACROSS_BOUNDARY){
            dma->mode = MSDC_MODE_DMA_BASIC;
        } else 
#endif
            dma->mode = MSDC_MODE_DMA_BASIC;
    } else {
        dma->mode = MSDC_MODE_DMA_DESC;
    }

    N_MSG(DMA, "DMA mode<%d> sglen<%d> xfersz<%d>", dma->mode, dma->sglen, dma->xfersz);

    msdc_dma_config(host, dma);
}

/* set block number before send command */
static void msdc_set_blknum(struct msdc_host *host, u32 blknum)
{
    u32 base = host->base;

    sdr_write32(SDC_BLK_NUM, blknum);
}

#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
/* get block number before send command */
static int msdc_get_blknum(struct msdc_host *host)
{
    u32 base = host->base;
    u32 blknum;

    blknum = sdr_read32(SDC_BLK_NUM);

    return blknum;
}
#endif    // MTK_SDIO30_ONLINE_TUNING_SUPPORT


#define REQ_CMD_EIO  (0x1 << 0)
#define REQ_CMD_TMO  (0x1 << 1)
#define REQ_DAT_ERR  (0x1 << 2)
#define REQ_STOP_EIO (0x1 << 3)
#define REQ_STOP_TMO (0x1 << 4)
#define REQ_CMD23_EIO (0x1 << 5)
#define REQ_CMD23_TMO (0x1 << 6)
static void msdc_restore_info(struct msdc_host *host){
    u32 base = host->base;
    unsigned long  flags_lock = 0;
    int retry = 3;
    mb();
    msdc_reset_hw(host->id);
    host->saved_para.msdc_cfg = host->saved_para.msdc_cfg & 0xFFFFFFDF; //force bit5(BV18SDT) to 0
    sdr_write32(MSDC_CFG,host->saved_para.msdc_cfg);
    
    while(retry--){
        msdc_set_mclk(host, host->saved_para.ddr, host->saved_para.hz);
        if((sdr_read32(MSDC_CFG)&0xFFFFFF9F) != (host->saved_para.msdc_cfg&0xFFFFFF9F)){
            ERR_MSG("msdc set_mclk is unstable (cur_cfg=%x, save_cfg=%x, cur_hz=%d, save_hz=%d).", sdr_read32(MSDC_CFG), host->saved_para.msdc_cfg, host->mclk, host->saved_para.hz);
        } else { 
            break;
        }
    } 

    sdr_set_field(MSDC_PATCH_BIT0, MSDC_INT_DAT_LATCH_CK_SEL,    host->saved_para.int_dat_latch_ck_sel); //for SDIO 3.0
    sdr_set_field(MSDC_PATCH_BIT0, MSDC_CKGEN_MSDC_DLY_SEL, host->saved_para.ckgen_msdc_dly_sel); //for SDIO 3.0
    sdr_set_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_CMD_RSP,    host->saved_para.cmd_resp_ta_cntr);//for SDIO 3.0
    sdr_set_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_WRDAT_CRCS, host->saved_para.wrdat_crc_ta_cntr); //for SDIO 3.0
    sdr_write32(MSDC_DAT_RDDLY0,host->saved_para.ddly0);
    sdr_write32(MSDC_PAD_TUNE,host->saved_para.pad_tune);
    sdr_write32(SDC_CFG,host->saved_para.sdc_cfg);
    if (host->hw->flags & MSDC_SDIO_IRQ)
        spin_lock_irqsave(&SDIO_IRQ_lock, flags_lock);
    sdr_set_field(MSDC_INTEN, MSDC_INT_SDIOIRQ, host->saved_para.inten_sdio_irq); //get INTEN status for SDIO    
    if (host->hw->flags & MSDC_SDIO_IRQ)
        spin_unlock_irqrestore(&SDIO_IRQ_lock, flags_lock);
    sdr_write32(MSDC_IOCON,host->saved_para.iocon);
}

#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT


static unsigned int ot_find_next_gear(u32 CRCResult, const u32 tune_wind_size, const u32 cur_gear, u32 *fTestedGear)
{
    int i;
	u32 new_gear = cur_gear;
	u32 test_gear_range = 0xFFFFFFFF >> (32 - (2 * tune_wind_size + 1));
	
    if(CRCResult & 0x1)	// lsb bit CRC pass, go to smaller gear
    {
        for(i = 1; i <= 2*tune_wind_size; i++)	// Scan from least gear to most gear
        {
            if(((CRCResult>>i)&1) == 0)
            {
                break;
            }
        }

        new_gear = (cur_gear + 32 - (2 * tune_wind_size + 1 - i)) & 0x1F;
    }
    else	// msb bit CRC pass or CRC fail on both side, go to bigger gear
    {
        for(i = 2*tune_wind_size; i >= 0; i--)	// Scan from most gear to least gear
        {
            if(((CRCResult>>i)&1) == 0)
            {
                break;
            }
        }

        new_gear = (cur_gear + i + 1) & 0x1F;
    }

    if(new_gear >= tune_wind_size)
        *fTestedGear |= (test_gear_range << (new_gear - tune_wind_size));
    else
        *fTestedGear |= (test_gear_range >> (tune_wind_size - new_gear));

	xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "[%s], cur_gear = 0x%x, new_gear = 0x%x, tune_wind_size = 0x%x, CRCResult = 0x%x\n", __func__, cur_gear, new_gear, tune_wind_size, CRCResult);
    
    return new_gear;
}

static int ot_do_command(struct msdc_host *host, u32 rawcmd, u32 rawarg, u32 *pintsts);

static int ot_recover_device(struct msdc_host *host, unsigned fn, unsigned addr)
{
#if 0
    u32 base = host->base;
    
    xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "Enter [%s]\n", __func__);

    sdr_set_field(MSDC_CFG, MSDC_CFG_CKPDN, 1); // Clock is free running
    
    msleep(1);

    sdr_set_field(MSDC_CFG, MSDC_CFG_CKPDN, 0); // Clock is gated to 0 if no command or data is transmitted.
    msdc_reset_hw(host->id);
#endif
    return 0;
}

static int ot_command_resp_polling(struct msdc_host *host, struct mmc_command *cmd, int tune, unsigned long timeout, u32 *pintsts)
{
    u32 base = host->base;

    u32 intsts;
    u32 resp;
    unsigned long tmo;

    u32 acmd53done = MSDC_INT_ACMDTMO | MSDC_INT_DATTMO | MSDC_INT_ACMD53_DONE | MSDC_INT_CMDRDY | MSDC_INT_CMDTMO | MSDC_INT_RSPCRCERR;
    u32 cmdsts = MSDC_INT_CMDTMO | MSDC_INT_ACMDTMO | MSDC_INT_DATTMO | MSDC_INT_ACMD53_DONE | MSDC_INT_GEAR_OUT_BOUND | MSDC_INT_ACMDCRCERR | MSDC_INT_ACMD53_FAIL | MSDC_INT_CMDRDY | MSDC_INT_RSPCRCERR;
    u32 flagTmo = MSDC_INT_ACMDTMO | MSDC_INT_DATTMO;

    resp = host->cmd_rsp;

    /*polling*/
    tmo = jiffies + timeout;
    
    while (1){
        if (((intsts = sdr_read32(MSDC_INT)) & acmd53done) != 0){
            /* clear all int flag */
            intsts &= cmdsts;
            sdr_write32(MSDC_INT, intsts);
            break;
        }

        if (time_after(jiffies, tmo)) {
            cmd->error = (unsigned int)-ETIMEDOUT;
            host->sw_timeout++;
            xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "[%s] SW timeout, intsts = 0x%x\n", __func__, intsts);
            msdc_dump_info(host->id); 
            msdc_reset_hw(host->id);
            goto out;    
        }   
    }

    /* command interrupts */
    if((intsts & MSDC_INT_ACMD53_DONE) || (intsts & flagTmo))
    {
        cmd->resp[0] = sdr_read32(DAT0_TUNE_CRC);
        cmd->resp[1] = sdr_read32(DAT1_TUNE_CRC);
        cmd->resp[2] = sdr_read32(DAT2_TUNE_CRC);
        cmd->resp[3] = sdr_read32(DAT3_TUNE_CRC);
    }

    /* timeout */
    if (intsts & flagTmo)
    {
        xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "Device CMD/DAT timeout, MSDC_INT = 0x%x\n", intsts);
        cmd->error = (unsigned int)-ETIMEDOUT;
        msdc_reset_hw(host->id);
    }

out:
    *pintsts = intsts;

    return cmd->error;
}

static u32 wints_cmd_testmode = MSDC_INT_CMDRDY | MSDC_INT_RSPCRCERR | MSDC_INT_CMDTMO |  
                       MSDC_INT_ACMDRDY | MSDC_INT_ACMDTMO |
                       MSDC_INT_DATTMO | MSDC_INT_GEAR_OUT_BOUND |
                       MSDC_INT_ACMD53_DONE | MSDC_INT_ACMDCRCERR | MSDC_INT_DATCRCERR | MSDC_INT_ACMD53_FAIL;

static int ot_do_command(struct msdc_host *host, u32 rawcmd, u32 rawarg, u32 *pintsts)
{
    u32 base = host->base;
    struct mmc_command *cmd = host->cmd;

    unsigned long tmo;
    
    cmd->error = 0;

    tmo = jiffies + CMD_TIMEOUT;
    while(sdc_is_cmd_busy() || sdc_is_busy())
    {
        if (time_after(jiffies, tmo)) {
            if(sdc_is_cmd_busy())
                xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "XXX sdc_cmd_busy timeout\n");
            else if(sdc_is_busy())
                xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "XXX sdc_busy timeout\n");
            cmd->error = (unsigned int)-ETIMEDOUT;
            msdc_reset_hw(host->id);
            return cmd->error;  /* Fix me: error handling */
        } 
    }
    
    /* use polling way */
    sdr_clr_bits(MSDC_INTEN, wints_cmd_testmode);

    sdc_send_cmd(rawcmd, rawarg);
    if (ot_command_resp_polling(host, cmd, 0, CMD_TIMEOUT, pintsts))
    {
        xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "ot_command_resp_polling fail, err = %d, resp = 0x%.8x\n", cmd->error, cmd->resp[0]);
        return cmd->error;
    }

    return 0;
}

static unsigned int ot_init(struct msdc_host *host, struct ot_data *potData)
{
    u32 base = host->base;
    u32 rxdly0;
    //struct mmc_host *mmc = host->mmc;
    
    potData->retry = 0;

    /* reset dly pass */
    potData->cmddlypass = 0;
    potData->dat0rddlypass = 0;
    potData->dat1rddlypass = 0;
    potData->dat2rddlypass = 0;
    potData->dat3rddlypass = 0;

    /* reset testedGear */
    potData->fCmdTestedGear = 0;
    potData->fDatTestedGear = 0;
    potData->fDat0TestedGear = 0;
    potData->fDat1TestedGear = 0;
    potData->fDat2TestedGear = 0;
    potData->fDat3TestedGear = 0;

    /* Get block number */
    potData->orig_blknum = msdc_get_blknum(host);

    /* Get PATCH_BIT0 value */
    potData->orig_patch_bit0 = sdr_read32(MSDC_PATCH_BIT0);

    /* Get MSDC_IOCON value */
    potData->orig_iocon = sdr_read32(MSDC_IOCON);
    
    /* Get DMA status */
    potData->orig_dma = msdc_dma_status();

    /* Get eco_ver */
    potData->eco_ver = sdr_read32(MSDC_ECO_VER);
    
    /* Get current Data Timeout Counter setting and set to 1048576*0x30 ticks */
    sdr_get_field(SDC_CFG, SDC_CFG_DTOC, potData->orig_dtoc);    // Get Data Timeout Counter
    sdr_set_field(SDC_CFG, SDC_CFG_DTOC, 0x10);

    /* Set block number to 1 */
    msdc_set_blknum(host, 1);

    /* Set online tuning interrupt behavior */
    sdr_set_field(MSDC_PATCH_BIT0, MSDC_MASK_ACMD53_CRC_ERR_INTR, 1);
    sdr_set_field(MSDC_PATCH_BIT0, MSDC_ACMD53_FAIL_ONE_SHOT, 1);

    /* Get current data line delay select */
    sdr_get_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, potData->orig_ddlsel);    // Get cmd delay
    
    /* Set DAT line behavior */
    sdr_set_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);
    sdr_set_field(MSDC_IOCON, MSDC_IOCON_RDSPLSEL, 1);
    sdr_set_field(MSDC_IOCON, MSDC_IOCON_WDSPLSEL, 1);
    
    /* Set PIO mode */
    msdc_dma_off();

    /* Set gear window size */
    sdr_write32(SDIO_TUNE_WIND, potData->tune_wind_size);
    xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "gear window size = %d\n", potData->tune_wind_size);
    
    /* Get current delay settings */
    sdr_get_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, potData->orig_cmdrdly);    // Get cmd delay
    potData->cmdrdly = potData->orig_cmdrdly;
    xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "cmd delay = 0x%x\n", potData->orig_cmdrdly);
    
    rxdly0 = sdr_read32(MSDC_DAT_RDDLY0);
    xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "DAT read delay line 0 = 0x%x\n", rxdly0);
    if (potData->eco_ver >= 4) {
        potData->orig_dat0rddly = (rxdly0 >> 24) & 0x1F;
        potData->orig_dat1rddly = (rxdly0 >> 16) & 0x1F;
        potData->orig_dat2rddly = (rxdly0 >>  8) & 0x1F;
        potData->orig_dat3rddly = (rxdly0 >>  0) & 0x1F;
    } else {   
        potData->orig_dat0rddly = (rxdly0 >>  0) & 0x1F;
        potData->orig_dat1rddly = (rxdly0 >>  8) & 0x1F;
        potData->orig_dat2rddly = (rxdly0 >> 16) & 0x1F;
        potData->orig_dat3rddly = (rxdly0 >> 24) & 0x1F;
    }

    if(potData->orig_ddlsel == 0)
    {
        sdr_get_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATRRDLY, potData->orig_paddatrddly);    // Get read delay
        xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "PAD DAT RD RXDLY = 0x%x\n", potData->orig_paddatrddly);
        potData->dat0rddly = potData->orig_paddatrddly;
        potData->dat1rddly = potData->orig_paddatrddly;
        potData->dat2rddly = potData->orig_paddatrddly;
        potData->dat3rddly = potData->orig_paddatrddly;
    }
    else
    {
        potData->dat0rddly = potData->orig_dat0rddly;
        potData->dat1rddly = potData->orig_dat1rddly;
        potData->dat2rddly = potData->orig_dat2rddly;
        potData->dat3rddly = potData->orig_dat3rddly;
    }
    
    /* Setup rawcmd */
    potData->rawcmd = 0;
    potData->rawcmd |= SD_IO_RW_EXTENDED;
    potData->rawcmd |= msdc_rsp[RESP_R1] << 7;
    potData->rawcmd |= 1 << 11;                        // Single block transmission
    potData->rawcmd |= host->blksz << 16;              // Block length
    potData->rawcmd |= 1 << 31;                        // Autocmd53

    /* Setup rawarg */
    potData->rawarg = 0;
    potData->rawarg |= potData->fn << 28;
    potData->rawarg |= potData->addr << 9;
    potData->rawarg |= 0x08000000;                    // Block mode
    potData->rawarg |= 0x00000001;                    // Single block

    return 0;
}

static unsigned int ot_deinit(struct msdc_host *host, struct ot_data otData)
{
    u32 base = host->base;
    
    /* Set orignal block number */
    msdc_set_blknum(host, otData.orig_blknum);

    /* Set original PATCH_BIT0 value */
    sdr_write32(MSDC_PATCH_BIT0, otData.orig_patch_bit0);

    /* Set original MSDC_IOCON value */
    sdr_write32(MSDC_IOCON, otData.orig_iocon);
    
    /* Set original DTOC */
    sdr_set_field(SDC_CFG, SDC_CFG_DTOC, otData.orig_dtoc);
    
    /* Set original DMA status */
    if(otData.orig_dma == DMA_ON)
        msdc_dma_on();
    else if(otData.orig_dma == DMA_OFF)
        msdc_dma_off();
    
    return 0;
}

static int ot_adjust_tunewnd(struct msdc_host *host, u32 *fTestedGear, u32 *tuneWnd, u32 *CRCPass)
{
    u32 base = host->base;

    if(*fTestedGear == 0xFFFFFFFF)
    {
        *tuneWnd = *tuneWnd - 1;

        /* Set gear window size */
        sdr_write32(SDIO_TUNE_WIND, *tuneWnd);
        xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "[%s] gear window size = %d\n", __func__, *tuneWnd);

        if(*tuneWnd == 0)
            return -1;


        *CRCPass = 0xFFFFFFFF >> (32 - (2 * (*tuneWnd) + 1));
        //xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "CRCPass = %d\n", *CRCPass);

        *fTestedGear = 0;
    }

	return 0;
}

static int ot_process(struct msdc_host *host, struct ot_data *potData, u32 rawcmd, u32 rawarg, u32 CRCPass, bool rw)
{
    u32 base = host->base;
	u32 CRCStatus_CMD=0;
    u32 rxdly0;
    int reset;
    u32 intsts = 0;
	u32 ret = 0;
	
	return -1;
	
    if(rw == 0)	// Read
    {
        rawcmd &= ~(1 << 13);
        rawarg &= ~(1 << 31);
    }
    else	// Write
    {
        rawcmd |= 1 << 13;
        rawarg |= 1 << 31;
    }

    //xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "enter command start, rw = %d, rawcmd = 0x%x, rawarg = 0x%x\n", rw, rawcmd, rawarg);
    if (ot_do_command(host, rawcmd, rawarg, &intsts) != 0) {
        xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "command start fail, err = %d\n", host->cmd->error);
        xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "command = 0x%x, arg = 0x%x\n", rawcmd, rawarg);
        ret = host->cmd->error;

        if(ret == (unsigned int)-ETIMEDOUT)
        {
            int ret1 = 0;
            
            /* Get CMD CRC status */
            CRCStatus_CMD = sdr_read32(CMD_TUNE_CRC);
            
            if( (ret1 = ot_recover_device(host, potData->fn, potData->addr)) == 0)    // if ot_recover_device returns 0, means device recover success, continue online tuning
            {
                if((CRCStatus_CMD != CRCPass) && (potData->cmddlypass == 0))
                {
                    /* Reset delay gear */
                    potData->cmdrdly = (potData->cmdrdly + 1) & 0x1F;
                    sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, potData->cmdrdly);    // Set cmd delay
                    xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "(TMO) cmd delay = %d \n", potData->cmdrdly);
                    xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "(TMO) CRCStatus_CMD = 0x%x \n", CRCStatus_CMD);
                }
                
                reset = 0;
                
                if((host->cmd->resp[0] != CRCPass) && (potData->dat0rddlypass == 0))
                {
                    /* Reset delay gear */
                    if(potData->orig_ddlsel == 1)
                        potData->dat0rddly = (potData->dat0rddly + 1) & 0x1F;
                    reset++;
                    xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "(TMO) dat0 delay = %d \n", potData->dat0rddly);
                    xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "(TMO) host->cmd->resp[0] = 0x%x \n", host->cmd->resp[0]);
                }
                
                if((host->cmd->resp[1] != CRCPass) && (potData->dat1rddlypass == 0))
                {
                    /* Reset delay gear */
                    if(potData->orig_ddlsel == 1)
                        potData->dat1rddly = (potData->dat1rddly + 1) & 0x1F;
                    reset++;
                    xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "(TMO) dat1 delay = %d \n", potData->dat1rddly);
                    xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "(TMO) host->cmd->resp[1] = 0x%x \n", host->cmd->resp[1]);
                }
                
                if((host->cmd->resp[2] != CRCPass) && (potData->dat2rddlypass == 0))
                {
                    /* Reset delay gear */
                    if(potData->orig_ddlsel == 1)
                        potData->dat2rddly = (potData->dat2rddly + 1) & 0x1F;
                    reset++;
                    xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "(TMO) dat2 delay = %d \n", potData->dat2rddly);
                    xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "(TMO) host->cmd->resp[2] = 0x%x \n", host->cmd->resp[2]);
                }
                
                if((host->cmd->resp[3] != CRCPass) && (potData->dat3rddlypass == 0))
                {
                    /* Reset delay gear */
                    if(potData->orig_ddlsel == 1)
                        potData->dat3rddly = (potData->dat3rddly + 1) & 0x1F;
                    reset++;
                    xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "(TMO) dat3 delay = %d \n", potData->dat3rddly);
                    xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "(TMO) host->cmd->resp[3] = 0x%x \n", host->cmd->resp[3]);
                }
                
                if(reset) {
                    if(potData->orig_ddlsel == 0)
                    {
                        potData->dat0rddly = (potData->dat0rddly + 1) & 0x1F;
                        potData->dat1rddly = (potData->dat1rddly + 1) & 0x1F;
                        potData->dat2rddly = (potData->dat2rddly + 1) & 0x1F;
                        potData->dat3rddly = (potData->dat3rddly + 1) & 0x1F;
                    }
                    /* Reset delay gear */
                    if (potData->eco_ver >= 4) {
                        rxdly0 = (potData->dat0rddly << 24) | (potData->dat1rddly << 16) | (potData->dat2rddly << 8) | (potData->dat3rddly << 0);
                    } else {   
                        rxdly0 = (potData->dat0rddly << 0) | (potData->dat1rddly << 8) | (potData->dat2rddly << 16) | (potData->dat3rddly << 24);
                    }
                    sdr_write32(MSDC_DAT_RDDLY0, rxdly0);
                }

                xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "goto _end (TMO) \n");
                potData->retry = 0;
				return -1;
            }
        }
        return ret;
    }
    
    if((intsts & MSDC_INT_ACMD53_FAIL) == 0)    // All CRC Pass
    {
        xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "CRC Pass, intsts = 0x%x\n", intsts);
        return 0;
    }

    /* Get CMD CRC status */
    CRCStatus_CMD = sdr_read32(CMD_TUNE_CRC);
    
    /* Check cmd gear */
    if(CRCStatus_CMD != CRCPass)
    {
        /* Reset delay gear */
        //xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "cmdrdly = %d, CRCStatus_CMD = 0x%x, CRC Pass = 0x%x\n", cmdrdly, CRCStatus_CMD, CRCPass);

        potData->cmdrdly = ot_find_next_gear(CRCStatus_CMD, potData->tune_wind_size, potData->cmdrdly, &potData->fCmdTestedGear);
        sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, potData->cmdrdly);    // Set cmd delay
        
        xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "goto _retry (Cmd), cmd delay = %d \n", potData->cmdrdly);
        potData->retry = 1;
        return 0;
    }
    else
    {
        potData->cmddlypass = 1;
    }

    reset = 0;
    /* Check read gear */
    if(potData->orig_ddlsel == 0)
    {
        u32 crcsts = (host->cmd->resp[0] & host->cmd->resp[1] & host->cmd->resp[2] & host->cmd->resp[3]);
        if(crcsts != CRCPass)
        {
            u32 datrddly = ot_find_next_gear(crcsts, potData->tune_wind_size, potData->dat0rddly, &potData->fDatTestedGear);
            potData->dat0rddly = datrddly;
            potData->dat1rddly = datrddly;
            potData->dat2rddly = datrddly;
            potData->dat3rddly = datrddly;
        }
        else
        {
            potData->datrddlypass = 1;
        }
    }
    else
    {
        if(host->cmd->resp[0] != CRCPass)
        {
            //xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "dat0 delay = %d, host->cmd->resp[0] = 0x%x, CRC Pass = 0x%x\n", dat0rddly, host->cmd->resp[0], CRCPass);
            potData->dat0rddly = ot_find_next_gear(host->cmd->resp[0], potData->tune_wind_size, potData->dat0rddly, &potData->fDat0TestedGear);
            reset++;
        }
        else
        {
            potData->dat0rddlypass = 1;
        }
        
        if(host->cmd->resp[1] != CRCPass)
        {
            //xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "dat1 delay = %d, host->cmd->resp[1] = 0x%x, CRC Pass = 0x%x\n", dat1rddly, host->cmd->resp[1], CRCPass);
            potData->dat1rddly = ot_find_next_gear(host->cmd->resp[1], potData->tune_wind_size, potData->dat1rddly, &potData->fDat1TestedGear);
            reset++;
        }
        else
        {
            potData->dat1rddlypass = 1;
        }
        
        if(host->cmd->resp[2] != CRCPass)
        {
            //xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "dat2 delay = %d, host->cmd->resp[2] = 0x%x, CRC Pass = 0x%x\n", dat2rddly, host->cmd->resp[2], CRCPass);
            potData->dat2rddly = ot_find_next_gear(host->cmd->resp[2], potData->tune_wind_size, potData->dat2rddly, &potData->fDat2TestedGear);
            reset++;
        }
        else
        {
            potData->dat2rddlypass = 1;
        }
        
        if(host->cmd->resp[3] != CRCPass)
        {
            //xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "dat3 delay = %d, host->cmd->resp[3] = 0x%x, CRC Pass = 0x%x\n", dat3rddly, host->cmd->resp[3], CRCPass);
            potData->dat3rddly = ot_find_next_gear(host->cmd->resp[3], potData->tune_wind_size, potData->dat3rddly, &potData->fDat3TestedGear);
            reset++;
        }
        else
        {
            potData->dat3rddlypass = 1;
        }
    }
    
    if(reset) {
        /* Reset delay gear */
        if (potData->eco_ver >= 4) {
            rxdly0 = (potData->dat0rddly << 24) | (potData->dat1rddly << 16) | (potData->dat2rddly << 8) | (potData->dat3rddly << 0);
        } else {   
            rxdly0 = (potData->dat0rddly << 0) | (potData->dat1rddly << 8) | (potData->dat2rddly << 16) | (potData->dat3rddly << 24);
        }
        sdr_write32(MSDC_DAT_RDDLY0, rxdly0);
        
        xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "goto _retry (Read) \n");
        potData->retry = 1;
        return 0;
    }

	return 0;
}


extern unsigned int autok_get_current_vcore_offset(void);
extern int msdc_autok_stg1_data_get(void **ppData, int *pLen);
extern int msdc_autok_apply_param(struct msdc_host *host, unsigned int vcore_uv);
extern void mt_cpufreq_disable(unsigned int type, bool disabled);

static unsigned int msdc_online_tuning(struct msdc_host *host, unsigned fn, unsigned addr)
{
    u32 base = host->base;
    u32 rxdly0;
    u32 rawcmd;
    u32 rawarg;
    u32 tune_wind_size_cmd = OT_START_TUNEWND;
    u32 tune_wind_size_dat = OT_START_TUNEWND;
	u32 tune_wind_size_dat0 = OT_START_TUNEWND;
	u32 tune_wind_size_dat1 = OT_START_TUNEWND;
	u32 tune_wind_size_dat2 = OT_START_TUNEWND;
	u32 tune_wind_size_dat3 = OT_START_TUNEWND;
    u32 CRCPass = 0xFFFFFFFF >> (32 - (2 * OT_START_TUNEWND + 1));
    
    u32 vcore_uv = 0;
    int len = 0;
    int ret = 0;

    unsigned long tmo;

    struct mmc_host *mmc = host->mmc;
    struct ot_data otData;
    struct mmc_command *orig_cmd = host->cmd;
    struct mmc_command cmd;
    
    xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "Enter %s \n", __func__);
    
    /* ungate clock */
    msdc_ungate_clock(host);  // set sw flag 
    
    /* Claim host */
    if(host->ot_work.chg_volt == 0)
    {
        //mmc_claim_host(mmc);
        autok_claim_host(host);
    }
    
    atomic_set(&host->ot_done, 0);
	mt_cpufreq_disable(1, true);

    vcore_uv = autok_get_current_vcore_offset();
    // read auto-K result from auto-K callback function
    //msdc_autok_stg1_data_get(&msdc_param, &len);
    // apply MSDC parameter for current vcore
    //if(len != 0)
    msdc_autok_apply_param(host, vcore_uv);

    host->cmd = &cmd;
    host->blksz = OT_BLK_SIZE;

    otData.fn = fn;
    otData.addr = addr;
    otData.tune_wind_size = OT_START_TUNEWND;
    ot_init(host, &otData);
    rawcmd = otData.rawcmd;
    rawarg = otData.rawarg;

    /* Set timeout */
    tmo = jiffies + OT_TIMEOUT;
    
_retry:

    otData.retry = 0;

    if (time_after(jiffies, tmo))
    {
        xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "online tuning timeout\n");
        
        ret = -1;

        goto out;
    }

    if(otData.cmddlypass == 0)
    {
        xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "[%s]fCmdTestedGear = 0x%x\n", __func__, otData.fCmdTestedGear);
        ret = ot_adjust_tunewnd(host, &otData.fCmdTestedGear, &tune_wind_size_cmd, &CRCPass);
        otData.tune_wind_size = tune_wind_size_cmd;
        if(ret == -1)
            goto out;
    }
    else if(otData.orig_ddlsel == 0)
    {
        if(otData.datrddlypass == 0)
        {
            xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "[%s]fDatTestedGear = 0x%x\n", __func__, otData.fDatTestedGear);
            ret = ot_adjust_tunewnd(host, &otData.fDatTestedGear, &tune_wind_size_dat, &CRCPass);
            otData.tune_wind_size = tune_wind_size_dat;
            if(ret == -1)
                goto out;
        }
    }
    else
    {
        if(otData.dat0rddlypass == 0)
        {
            xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "[%s]fDat0TestedGear = 0x%x\n", __func__, otData.fDat0TestedGear);
            ret = ot_adjust_tunewnd(host, &otData.fDat0TestedGear, &tune_wind_size_dat0, &CRCPass);
            otData.tune_wind_size = tune_wind_size_dat0;
            if(ret == -1)
                goto out;
        }
        else if(otData.dat1rddlypass == 0)
        {
            xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "[%s]fDat1TestedGear = 0x%x\n", __func__, otData.fDat1TestedGear);
            ret = ot_adjust_tunewnd(host, &otData.fDat1TestedGear, &tune_wind_size_dat1, &CRCPass);
            otData.tune_wind_size = tune_wind_size_dat1;
            if(ret == -1)
                goto out;
        }
        else if(otData.dat2rddlypass == 0)
        {
            xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "[%s]fDat2TestedGear = 0x%x\n", __func__, otData.fDat2TestedGear);
            ret = ot_adjust_tunewnd(host, &otData.fDat2TestedGear, &tune_wind_size_dat2, &CRCPass);
            otData.tune_wind_size = tune_wind_size_dat2;
            if(ret == -1)
                goto out;
        }
        else if(otData.dat3rddlypass == 0)
        {
            xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "[%s]fDat3TestedGear = 0x%x\n", __func__, otData.fDat3TestedGear);
            ret = ot_adjust_tunewnd(host, &otData.fDat3TestedGear, &tune_wind_size_dat3, &CRCPass);
            otData.tune_wind_size = tune_wind_size_dat3;
            if(ret == -1)
                goto out;
        }
    }

    // Process read
    ret = ot_process(host, &otData, rawcmd, rawarg, CRCPass, 0);

    if(ret < 0)
        goto out;

    if(otData.retry == 1)
    {
        goto _retry;
    }

#if 0
    // Process write
    ret = ot_process(host, &otData, rawcmd, rawarg, CRCPass, 1);

    if(ret < 0)
        goto out;

    if(otData.retry == 1)
    {
        goto _retry;
    }
#endif

out:
    
    if(ret < 0)
    {
        sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, otData.orig_cmdrdly);    // Set orig cmd delay gear

        /* set orig dat delay gear */
        if (otData.eco_ver >= 4) {
            rxdly0 = (otData.orig_dat0rddly << 24) | (otData.orig_dat1rddly << 16) | (otData.orig_dat2rddly << 8) | (otData.orig_dat3rddly << 0);
        } else {   
            rxdly0 = (otData.orig_dat0rddly << 0) | (otData.orig_dat1rddly << 8) | (otData.orig_dat2rddly << 16) | (otData.orig_dat3rddly << 24);
        }
        sdr_write32(MSDC_DAT_RDDLY0, rxdly0);
        
        xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "ret = %d, vcore_uv = 0x%x, cmd delay = 0x%x\n", ret, vcore_uv, otData.orig_cmdrdly);
    }
    else if(otData.orig_ddlsel == 0)
    {
        sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATRRDLY, otData.dat0rddly);    // Set read delay
        xlog_printk(ANDROID_LOG_DEBUG, "SDIO_TEST_MODE", "ret = %d, dat delay = 0x%x \n", ret, otData.dat0rddly);
    }

    ot_deinit(host, otData);
    
    /* gate clock */
    msdc_gate_clock(host, 1); // clear flag. 
    
    host->cmd = orig_cmd;

    /* release host */
    host->ot_work.chg_volt = 0;
    atomic_set(&host->sdio_stopping, 0);
    //mmc_release_host(mmc);
    autok_release_host(host);
    
    atomic_set(&host->ot_done, 1);
    
	mt_cpufreq_disable(1, false);
    
    return ret;
}

#ifdef ONLINE_TUNING_DVTTEST
#include "mt_online_tuning_test.c"
#endif


#endif    // MTK_SDIO30_ONLINE_TUNING_SUPPORT

static int msdc_do_request(struct mmc_host*mmc, struct mmc_request*mrq)
{
    struct msdc_host *host = mmc_priv(mmc);
    struct mmc_command *cmd;
    struct mmc_data *data;    
    u32 l_autocmd23_is_set = 0;
#ifdef MTK_MSDC_USE_CMD23 
    u32 l_card_no_cmd23 = 0;
#endif
    u32 base = host->base;
    //u32 intsts = 0;     
    unsigned int left=0;
    int dma = 0, read = 1, dir = DMA_FROM_DEVICE, send_type=0;
    u32 map_sg = 0;  /* Fix the bug of dma_map_sg and dma_unmap_sg not match issue */
    unsigned long pio_tmo;
#define SND_DAT 0
#define SND_CMD 1
    
    if (is_card_sdio(host) || (host->hw->flags & MSDC_SDIO_IRQ)) {
        mb();
        if (host->saved_para.hz)
        {
            if (host->saved_para.suspend_flag)
            {
                ERR_MSG("msdc resume[s] cur_cfg=%x, save_cfg=%x, cur_hz=%d, save_hz=%d", sdr_read32(MSDC_CFG), host->saved_para.msdc_cfg, host->mclk, host->saved_para.hz);
                host->saved_para.suspend_flag = 0;
                msdc_restore_info(host);
            }
            else if ((host->saved_para.msdc_cfg !=0) && 
                ((sdr_read32(MSDC_CFG)&0xFFFFFF9F)!= (host->saved_para.msdc_cfg&0xFFFFFF9F)))
            {
                ERR_MSG("msdc resume[ns] cur_cfg=%x, save_cfg=%x, cur_hz=%d, save_hz=%d", sdr_read32(MSDC_CFG), host->saved_para.msdc_cfg, host->mclk, host->saved_para.hz);
                msdc_restore_info(host);        
            }  
        }
    }
#if (MSDC_DATA1_INT == 1)  
    if (host->hw->flags & MSDC_SDIO_IRQ)
    {
        //if((!u_sdio_irq_counter) && (!u_msdc_irq_counter))
            //ERR_MSG("Ahsin u_sdio_irq_counter=%d, u_msdc_irq_counter=%d  int_sdio_irq_enable=%d SDC_CFG=%x MSDC_INTEN=%x MSDC_INT=%x MSDC_PATCH_BIT0=%x", u_sdio_irq_counter, u_msdc_irq_counter, int_sdio_irq_enable, sdr_read32(SDC_CFG),sdr_read32(MSDC_INTEN),sdr_read32(MSDC_INT),sdr_read32(MSDC_PATCH_BIT0));
        if ((u_sdio_irq_counter>0) && ((u_sdio_irq_counter%800)==0))
            ERR_MSG("Ahsin sdio_irq=%d, msdc_irq=%d  SDC_CFG=%x MSDC_INTEN=%x MSDC_INT=%x ", u_sdio_irq_counter, u_msdc_irq_counter, sdr_read32(SDC_CFG), sdr_read32(MSDC_INTEN), sdr_read32(MSDC_INT));
    }
#endif  

    BUG_ON(mmc == NULL);
    BUG_ON(mrq == NULL);    

    host->error = 0;
    atomic_set(&host->abort, 0);
    
    cmd  = mrq->cmd;
    data = mrq->cmd->data;

    /* check msdc is work ok. rule is RX/TX fifocnt must be zero after last request 
     * if find abnormal, try to reset msdc first
     */
    if (msdc_txfifocnt() || msdc_rxfifocnt()) {
        printk("[SD%d] register abnormal,please check!\n",host->id);
        msdc_reset_hw(host->id);
    }
    
#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
  #ifdef MTK_SDIO30_DETECT_THERMAL
    if(host->hw->host_function == MSDC_SDIO) {
        spin_lock_irqsave(&host->clk_gate_lock, flags);
        if(host->ot_period_check_start == 0) {
            mod_timer(&host->ot_timer, jiffies + OT_PERIOD);
            host->ot_period_check_start = 1;
        }
        spin_unlock_irqrestore(&host->clk_gate_lock, flags); 
    }
  #endif // #ifdef MTK_SDIO30_DETECT_THERMAL
#endif    // MTK_SDIO30_ONLINE_TUNING_SUPPORT
         
    if (!data) {
        send_type=SND_CMD;    

        if (msdc_do_command(host, cmd, 0, CMD_TIMEOUT) != 0) {
            goto done;         
        }
        
#ifdef MTK_EMMC_SUPPORT
        if(host->hw->host_function == MSDC_EMMC &&
           host->hw->boot == MSDC_BOOT_EN && 
           cmd->opcode == MMC_SWITCH && 
           (((cmd->arg >> 16) & 0xFF) == EXT_CSD_PART_CFG)){
            partition_access = (char)((cmd->arg >> 8) & 0x07);
        }
#endif

#ifdef MTK_EMMC_ETT_TO_DRIVER
        if ((host->hw->host_function == MSDC_EMMC) && (cmd->opcode == MMC_ALL_SEND_CID)) {
            m_id           = UNSTUFF_BITS(cmd->resp, 120, 8);
            pro_name[0]    = UNSTUFF_BITS(cmd->resp,  96, 8);
            pro_name[1]    = UNSTUFF_BITS(cmd->resp,  88, 8);
            pro_name[2]    = UNSTUFF_BITS(cmd->resp,  80, 8);
            pro_name[3]    = UNSTUFF_BITS(cmd->resp,  72, 8);
            pro_name[4]    = UNSTUFF_BITS(cmd->resp,  64, 8);
            pro_name[5]    = UNSTUFF_BITS(cmd->resp,  56, 8);
            //pro_name[6]    = '\0';     
        }
#endif
        
    } else {
        BUG_ON(data->blksz > HOST_MAX_BLKSZ);
        send_type=SND_DAT;

        data->error = 0;
        read = data->flags & MMC_DATA_READ ? 1 : 0;
        msdc_latest_operation_type[host->id] = read ? OPER_TYPE_READ : OPER_TYPE_WRITE;  
        host->data = data;
        host->xfer_size = data->blocks * data->blksz;
        host->blksz = data->blksz;

        /* deside the transfer mode */
        if (drv_mode[host->id] == MODE_PIO) {
            host->dma_xfer = dma = 0;
            msdc_latest_transfer_mode[host->id] = TRAN_MOD_PIO;
        } else if (drv_mode[host->id] == MODE_DMA) {
            host->dma_xfer = dma = 1;            
            msdc_latest_transfer_mode[host->id] = TRAN_MOD_DMA;
        } else if (drv_mode[host->id] == MODE_SIZE_DEP) {
            host->dma_xfer = dma = ((host->xfer_size >= dma_size[host->id]) ? 1 : 0);    
            msdc_latest_transfer_mode[host->id] = dma ? TRAN_MOD_DMA: TRAN_MOD_PIO;
        }      

        if (read) {
            if ((host->timeout_ns != data->timeout_ns) ||
                (host->timeout_clks != data->timeout_clks)) {
                msdc_set_timeout(host, data->timeout_ns, data->timeout_clks);
            }
        }
        
        msdc_set_blknum(host, data->blocks);
        //msdc_clr_fifo();  /* no need */

#ifdef MTK_MSDC_USE_CMD23
        if (0 == (host->autocmd & MSDC_AUTOCMD23)){
            /* start the cmd23 first, mrq->sbc is NULL with single r/w */ 
            if (mrq->sbc){
                host->autocmd &= ~MSDC_AUTOCMD12;

                if(host->hw->host_function == MSDC_EMMC){
                    /* remove reliable bit & data tag bit */
                    mrq->sbc->arg &= ~(1 << 31);
                    mrq->sbc->arg &= ~(1 << 29);
                }

                if (msdc_command_start(host, mrq->sbc, 0, CMD_TIMEOUT) != 0)
                    goto done;            

                /* then wait command done */
                if (msdc_command_resp_polling(host, mrq->sbc, 0, CMD_TIMEOUT) != 0) {  
                    goto stop;                           
                }           
            } else {
                /* some sd card may not support cmd23, 
                 * some emmc card have problem with cmd23, so use cmd12 here */
                if(host->hw->host_function != MSDC_SDIO){
                    host->autocmd |= MSDC_AUTOCMD12;
                }
            } 
        } else {
            /* enable auto cmd23 */
            if (mrq->sbc){
                host->autocmd &= ~MSDC_AUTOCMD12;
            } else {
                /* some sd card may not support cmd23, 
                 * some emmc card have problem with cmd23, so use cmd12 here */
                if(host->hw->host_function != MSDC_SDIO){
                    host->autocmd &= ~MSDC_AUTOCMD23;
                    host->autocmd |= MSDC_AUTOCMD12;
                    l_card_no_cmd23 = 1;
                }
            } 
        }
#endif /* end of MTK_MSDC_USE_CMD23 */

        if (dma) {
            msdc_dma_on();  /* enable DMA mode first!! */
            init_completion(&host->xfer_done);
            
#ifndef MTK_MSDC_USE_CMD23 
            /* start the command first*/            
            if(host->hw->host_function != MSDC_SDIO){
                host->autocmd |= MSDC_AUTOCMD12;
            }
#endif
            if (msdc_command_start(host, cmd, 0, CMD_TIMEOUT) != 0)
                goto done;            

            dir = read ? DMA_FROM_DEVICE : DMA_TO_DEVICE;
            (void)dma_map_sg(mmc_dev(mmc), data->sg, data->sg_len, dir);
            map_sg = 1;

            /* then wait command done */
            if (msdc_command_resp_polling(host, cmd, 0, CMD_TIMEOUT) != 0){ //not tuning
                goto stop;                           
            }

            /* for read, the data coming too fast, then CRC error 
               start DMA no business with CRC. */
            //init_completion(&host->xfer_done);           
            msdc_dma_setup(host, &host->dma, data->sg, data->sg_len);  
            msdc_dma_start(host);
            if (unlikely(dumpMSDC()))
                AddStorageTrace(STORAGE_LOGGER_MSG_MSDC_DO,msdc_do_request,"msdc_dma_start",host->xfer_size); 
            spin_unlock(&host->lock);
            if(!wait_for_completion_timeout(&host->xfer_done, DAT_TIMEOUT)){
                ERR_MSG("XXX CMD<%d> ARG<0x%x> wait xfer_done<%d> timeout!!", cmd->opcode, cmd->arg,data->blocks * data->blksz);
            
                host->sw_timeout++;
                if (unlikely(dumpMSDC()))
                    AddStorageTrace(STORAGE_LOGGER_MSG_MSDC_DO,msdc_do_request,"msdc_dma ERR",host->xfer_size); 
                msdc_dump_info(host->id);           
                data->error = (unsigned int)-ETIMEDOUT;

                msdc_reset_hw(host->id);
            }
            spin_lock(&host->lock);
            msdc_dma_stop(host);
            if ((mrq->data && mrq->data->error) || ((host->autocmd & MSDC_AUTOCMD12) && mrq->stop && mrq->stop->error) ||
                (mrq->sbc && (mrq->sbc->error != 0) && (host->autocmd & MSDC_AUTOCMD23))){
                msdc_clr_fifo(host->id); 
                msdc_clr_int(); 
            }

            if (unlikely(dumpMSDC()))
                AddStorageTrace(STORAGE_LOGGER_MSG_MSDC_DO,msdc_do_request,"msdc_dma_stop"); 
        } else {
            /* Firstly: send command */
            host->autocmd &= ~MSDC_AUTOCMD12;  /* need ask the designer, how about autocmd12 or autocmd23 with pio mode */

            l_autocmd23_is_set = 0;   
            if (host->autocmd & MSDC_AUTOCMD23){
                l_autocmd23_is_set = 1;   
                host->autocmd &= ~MSDC_AUTOCMD23;  
            }

            host->dma_xfer = 0;
            if (msdc_do_command(host, cmd, 0, CMD_TIMEOUT) != 0) {
                goto stop;
            }

            /* Secondly: pio data phase */           
            if (read) {
                if (msdc_pio_read(host, data)){
                    goto stop;      // need cmd12. 
                }
            } else {
                if (msdc_pio_write(host, data)) {
                    goto stop;         
                }
            }

            /* For write case: make sure contents in fifo flushed to device */           
            if (!read) {
                pio_tmo = jiffies + DAT_TIMEOUT;
                while (1) {
                    left=msdc_txfifocnt();                    
                    if (left == 0) {
                        break;    
                    }  
                    if (msdc_pio_abort(host, data, pio_tmo)) {
                        break;
                        /* Fix me: what about if data error, when stop ? how to? */
                    }                                    
                }
            } else {
                /* Fix me: read case: need to check CRC error */    
            }

            /* For write case: SDCBUSY and Xfer_Comp will assert when DAT0 not busy. 
               For read case : SDCBUSY and Xfer_Comp will assert when last byte read out from FIFO.
               */                             

            /* try not to wait xfer_comp interrupt. 
               the next command will check SDC_BUSY. 
               SDC_BUSY means xfer_comp assert 
               */ 

        } // PIO mode 

stop:        
        /* pio mode will disable autocmd23 */
        if (l_autocmd23_is_set == 1){
            l_autocmd23_is_set = 0;
            host->autocmd |= MSDC_AUTOCMD23; 
        }

#ifndef MTK_MSDC_USE_CMD23
        /* Last: stop transfer */
        if (data && data->stop){ 
            if(!((cmd->error == 0) && (data->error == 0) && (host->autocmd & MSDC_AUTOCMD12) && 
                        (cmd->opcode == MMC_READ_MULTIPLE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK))){
                if (msdc_do_command(host, data->stop, 0, CMD_TIMEOUT) != 0) {
                    goto done; 
                }
            }
        }
#else
        if (host->hw->host_function == MSDC_EMMC){
            if (data && data->stop){ 
                /* multi r/w with no cmd23 and no autocmd12, need send cmd12 manual */
                /* if PIO mode and autocmd23 enable, cmd12 need send, because autocmd23 is disable under PIO */
                if ((((mrq->sbc == NULL) && !(host->autocmd & MSDC_AUTOCMD12)) ||
                    (!dma && mrq->sbc && (host->autocmd & MSDC_AUTOCMD23))) &&
                        (cmd->opcode == MMC_READ_MULTIPLE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK)){
                    if (msdc_do_command(host, data->stop, 0, CMD_TIMEOUT) != 0) {
                        goto done; 
                    }
                } 
            } 

        } else {
            /* for non emmc card, use old flow */
            if (data && data->stop){ 
                if(!((cmd->error == 0) && (data->error == 0) && (host->autocmd & MSDC_AUTOCMD12) && 
                     (cmd->opcode == MMC_READ_MULTIPLE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK))){
                    if (msdc_do_command(host, data->stop, 0, CMD_TIMEOUT) != 0) {
                        goto done; 
                    }
                }
            }
        }
#endif 

    }

done:

#ifdef MTK_MSDC_USE_CMD23 
    /* for msdc use cmd23, but card not supported(sbc is NULL), need enable autocmd23 for next request */
    if (1 == l_card_no_cmd23){
        if(host->hw->host_function != MSDC_SDIO){
            host->autocmd |= MSDC_AUTOCMD23;
            host->autocmd &= ~MSDC_AUTOCMD12;
            l_card_no_cmd23 = 0;
        }
    }
#endif

    if (data != NULL) {
        host->data = NULL;
        host->dma_xfer = 0;    

#if 0   //read MBR
#ifdef MTK_EMMC_SUPPORT
        {
            char *ptr = sg_virt(data->sg);
            int i;
            if (cmd->arg == 0x0 && (cmd->opcode == MMC_READ_SINGLE_BLOCK || cmd->opcode == MMC_READ_MULTIPLE_BLOCK)){
                printk("XXXX CMD<%d> ARG<%X> offset = <%d> data<%s %s> sg <%p> ptr <%p> blksz<%d> block<%d> error<%d>\n",cmd->opcode, cmd->arg, offset, (dma? "dma":"pio"), 
                       (read ? "read ":"write"), data->sg, ptr, data->blksz, data->blocks, data->error);             
              
                for(i = 0; i < 512; i ++){
                    if (i%32 == 0)
                        printk("\n");
                    printk(" %2x ", ptr[i]);
                } 
            }
        }
#endif    
#endif   //end read MBR
        if (dma != 0) {
            msdc_dma_off();     
            host->dma.used_bd = 0;
            host->dma.used_gpd = 0;
            if (map_sg == 1) {   
                /*if(data->error == 0){
                    int retry = 3;
                    int count = 1000;
                    msdc_retry(host->dma.gpd->hwo,retry,count,host->id);
                }*/
                dma_unmap_sg(mmc_dev(mmc), data->sg, data->sg_len, dir);
            }
        }
#ifdef MTK_EMMC_SUPPORT
        if(cmd->opcode == MMC_SEND_EXT_CSD){
            msdc_get_data(ext_csd,data);
        }
#endif
        host->blksz = 0;  


        N_MSG(OPS, "CMD<%d> data<%s %s> blksz<%d> block<%d> error<%d>",cmd->opcode, (dma? "dma":"pio"), 
                (read ? "read ":"write") ,data->blksz, data->blocks, data->error);                

        if (!(is_card_sdio(host)|| (host->hw->flags & MSDC_SDIO_IRQ))){
            if ((cmd->opcode != 17)&&(cmd->opcode != 18)&&(cmd->opcode != 24)&&(cmd->opcode != 25)) {             
                N_MSG(NRW, "CMD<%3d> arg<0x%8x> Resp<0x%8x> data<%s> size<%d>", cmd->opcode, cmd->arg, cmd->resp[0],
                    (read ? "read ":"write") ,data->blksz * data->blocks);  
            } else {
                N_MSG(RW,  "CMD<%3d> arg<0x%8x> Resp<0x%8x> block<%d>", cmd->opcode,
                    cmd->arg, cmd->resp[0], data->blocks);
            }
        }
    } else {
        if (!(is_card_sdio(host)|| (host->hw->flags & MSDC_SDIO_IRQ))) {
            if (cmd->opcode != 13) { // by pass CMD13
                N_MSG(NRW, "CMD<%3d> arg<0x%8x> resp<%8x %8x %8x %8x>", cmd->opcode, cmd->arg, 
                cmd->resp[0],cmd->resp[1], cmd->resp[2], cmd->resp[3]);                     
            }
        }
    }

    if (mrq->cmd->error == (unsigned int)-EIO) {
        host->error |= REQ_CMD_EIO;
        sdio_tune_flag |= 0x1;

        if( mrq->cmd->opcode == SD_IO_RW_EXTENDED )
            sdio_tune_flag |= 0x1;        
    }


    if (mrq->cmd->error == (unsigned int)-ETIMEDOUT) host->error |= REQ_CMD_TMO;

    if (mrq->data && mrq->data->error) {
        host->error |= REQ_DAT_ERR;        
        sdio_tune_flag |= 0x10;

        if (mrq->data->flags & MMC_DATA_READ)
            sdio_tune_flag |= 0x80;
        else    
            sdio_tune_flag |= 0x40;            
    }

#ifdef MTK_MSDC_USE_CMD23
    if (mrq->sbc && (mrq->sbc->error == (unsigned int)-EIO)) host->error |= REQ_CMD_EIO;
    if (mrq->sbc && (mrq->sbc->error == (unsigned int)-ETIMEDOUT)) host->error |= REQ_CMD_TMO;
#endif

    if (mrq->stop && (mrq->stop->error == (unsigned int)-EIO)) host->error |= REQ_STOP_EIO; 
    if (mrq->stop && (mrq->stop->error == (unsigned int)-ETIMEDOUT)) host->error |= REQ_STOP_TMO; 
    //if (host->error) ERR_MSG("host->error<%d>", host->error);     
#ifdef SDIO_ERROR_BYPASS  
    if(is_card_sdio(host) && !host->error){
        host->sdio_error = 0; 
    }	
#endif	
    return host->error;
}
static int msdc_tune_rw_request(struct mmc_host*mmc, struct mmc_request*mrq)
{
    struct msdc_host *host = mmc_priv(mmc);
    struct mmc_command *cmd;
    struct mmc_data *data;    

#ifdef MTK_MSDC_USE_CMD23
    u32 l_autocmd23_is_set = 0;
#endif

    u32 base = host->base;
    //u32 intsts = 0;     
      //unsigned int left=0;
    int read = 1,dma = 1;//dir = DMA_FROM_DEVICE, send_type=0,
    //u32 map_sg = 0;  /* Fix the bug of dma_map_sg and dma_unmap_sg not match issue */
    //u32 bus_mode = 0;    
#define SND_DAT 0
#define SND_CMD 1

    BUG_ON(mmc == NULL);
    BUG_ON(mrq == NULL);    

    //host->error = 0;
    atomic_set(&host->abort, 0);
    
    cmd  = mrq->cmd;
    data = mrq->cmd->data;

    /* check msdc is work ok. rule is RX/TX fifocnt must be zero after last request 
     * if find abnormal, try to reset msdc first
     */
    if (msdc_txfifocnt() || msdc_rxfifocnt()) {
        printk("[SD%d] register abnormal,please check!\n",host->id);
        msdc_reset_hw(host->id);
    }
           
    
    BUG_ON(data->blksz > HOST_MAX_BLKSZ);
    //send_type=SND_DAT;

    data->error = 0;
    read = data->flags & MMC_DATA_READ ? 1 : 0;
    msdc_latest_operation_type[host->id] = read ? OPER_TYPE_READ : OPER_TYPE_WRITE;  
    host->data = data;
    host->xfer_size = data->blocks * data->blksz;
    host->blksz = data->blksz;
    host->dma_xfer = 1;

    /* deside the transfer mode */
    /*
    if (drv_mode[host->id] == MODE_PIO) {
        host->dma_xfer = dma = 0;
        msdc_latest_transfer_mode[host->id] = TRAN_MOD_PIO;
    } else if (drv_mode[host->id] == MODE_DMA) {
        host->dma_xfer = dma = 1;            
        msdc_latest_transfer_mode[host->id] = TRAN_MOD_DMA;
    } else if (drv_mode[host->id] == MODE_SIZE_DEP) {
        host->dma_xfer = dma = ((host->xfer_size >= dma_size[host->id]) ? 1 : 0);    
        msdc_latest_transfer_mode[host->id] = dma ? TRAN_MOD_DMA: TRAN_MOD_PIO;
    }      
    */
    if (read) {
        if ((host->timeout_ns != data->timeout_ns) || (host->timeout_clks != data->timeout_clks)) 
        {
            msdc_set_timeout(host, data->timeout_ns, data->timeout_clks);
        }
    }
        
    msdc_set_blknum(host, data->blocks);
    //msdc_clr_fifo();  /* no need */
    msdc_dma_on();  /* enable DMA mode first!! */
    init_completion(&host->xfer_done);
            
    /* start the command first*/            
#ifndef MTK_MSDC_USE_CMD23
    if(host->hw->host_function != MSDC_SDIO){
        host->autocmd |= MSDC_AUTOCMD12;
    }
#else
    if(host->hw->host_function != MSDC_SDIO){
        host->autocmd |= MSDC_AUTOCMD12;

        /* disable autocmd23 in error tuning flow */
        l_autocmd23_is_set = 0; 
        if (host->autocmd & MSDC_AUTOCMD23){
            l_autocmd23_is_set = 1; 
            host->autocmd &= ~MSDC_AUTOCMD23;
        }
    }
#endif

    if (msdc_command_start(host, cmd, 0, CMD_TIMEOUT) != 0)
        goto done;            

    /* then wait command done */
    if (msdc_command_resp_polling(host, cmd, 0, CMD_TIMEOUT) != 0){ //not tuning
        goto stop;                           
    }
            
    /* for read, the data coming too fast, then CRC error 
     * start DMA no business with CRC. */         
    msdc_dma_setup(host, &host->dma, data->sg, data->sg_len);  
    msdc_dma_start(host);
    //ERR_MSG("1.Power cycle enable(%d)",host->power_cycle_enable);  
#ifdef STO_LOG  
    if (unlikely(dumpMSDC()))
        AddStorageTrace(STORAGE_LOGGER_MSG_MSDC_DO,msdc_tune_rw_request,"msdc_dma_start",host->xfer_size); 
#endif
    spin_unlock(&host->lock);
    if(!wait_for_completion_timeout(&host->xfer_done, DAT_TIMEOUT)){
        ERR_MSG("XXX CMD<%d> ARG<0x%x> wait xfer_done<%d> timeout!!", cmd->opcode, cmd->arg,data->blocks * data->blksz);
        host->sw_timeout++;
#ifdef STO_LOG  
        if (unlikely(dumpMSDC()))
            AddStorageTrace(STORAGE_LOGGER_MSG_MSDC_DO,msdc_tune_rw_request,"msdc_dma ERR",host->xfer_size); 
#endif
        msdc_dump_info(host->id);           
        data->error = (unsigned int)-ETIMEDOUT;
                
        msdc_reset_hw(host->id);
    }
    spin_lock(&host->lock);
    //ERR_MSG("2.Power cycle enable(%d)",host->power_cycle_enable);  
    msdc_dma_stop(host);
    if ((mrq->data && mrq->data->error)||(host->autocmd & MSDC_AUTOCMD12 && mrq->stop && mrq->stop->error) || 
            (mrq->sbc && (mrq->sbc->error != 0) && (host->autocmd & MSDC_AUTOCMD23))){
        msdc_clr_fifo(host->id); 
        msdc_clr_int(); 
    }
#ifdef STO_LOG  
    if (unlikely(dumpMSDC()))
        AddStorageTrace(STORAGE_LOGGER_MSG_MSDC_DO,msdc_tune_rw_request,"msdc_dma_stop"); 
#endif
          
stop:        
    /* Last: stop transfer */
       
    if (data->stop){ 
        if(!((cmd->error == 0) && (data->error == 0) && (host->autocmd == MSDC_AUTOCMD12) && (cmd->opcode == MMC_READ_MULTIPLE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK))){
            if (msdc_do_command(host, data->stop, 0, CMD_TIMEOUT) != 0) {
                goto done; 
            }
        }
    } 

done:
    host->data = NULL;
    host->dma_xfer = 0;    
    msdc_dma_off();     
    host->dma.used_bd  = 0;
    host->dma.used_gpd = 0;    
    host->blksz = 0;  
                

    N_MSG(OPS, "CMD<%d> data<%s %s> blksz<%d> block<%d> error<%d>",cmd->opcode, (dma? "dma":"pio"), 
                (read ? "read ":"write") ,data->blksz, data->blocks, data->error);                

    if (!(is_card_sdio(host)|| (host->hw->flags & MSDC_SDIO_IRQ))) {
        if ((cmd->opcode != 17)&&(cmd->opcode != 18)&&(cmd->opcode != 24)&&(cmd->opcode != 25)) {             
             N_MSG(NRW, "CMD<%3d> arg<0x%8x> Resp<0x%8x> data<%s> size<%d>", cmd->opcode, cmd->arg, cmd->resp[0],
                    (read ? "read ":"write") ,data->blksz * data->blocks);  
        } else {
             N_MSG(RW,  "CMD<%3d> arg<0x%8x> Resp<0x%8x> block<%d>", cmd->opcode,
                    cmd->arg, cmd->resp[0], data->blocks);
        }
    } else {
        if (!(is_card_sdio(host)|| (host->hw->flags & MSDC_SDIO_IRQ))) {
            if (cmd->opcode != 13) { // by pass CMD13
                N_MSG(NRW, "CMD<%3d> arg<0x%8x> resp<%8x %8x %8x %8x>", cmd->opcode, cmd->arg, 
                    cmd->resp[0],cmd->resp[1], cmd->resp[2], cmd->resp[3]);                   
            }
        }
    }
    host->error = 0; 
    if (mrq->cmd->error == (unsigned int)-EIO) host->error |= REQ_CMD_EIO;
    if (mrq->cmd->error == (unsigned int)-ETIMEDOUT) host->error |= REQ_CMD_TMO;
    if (mrq->data && (mrq->data->error)) host->error |= REQ_DAT_ERR;     
    if (mrq->stop && (mrq->stop->error == (unsigned int)-EIO)) host->error |= REQ_STOP_EIO; 
    if (mrq->stop && (mrq->stop->error == (unsigned int)-ETIMEDOUT)) host->error |= REQ_STOP_TMO;     
    
    
#ifdef MTK_MSDC_USE_CMD23
    if (l_autocmd23_is_set == 1){
        /* restore the value */
        host->autocmd |= MSDC_AUTOCMD23;
    }
#endif 
    return host->error;
}

static void msdc_pre_req(struct mmc_host *mmc, struct mmc_request *mrq, bool is_first_req)
{
    struct msdc_host *host = mmc_priv(mmc);
    struct mmc_data *data;
    struct mmc_command *cmd = mrq->cmd;
    int read = 1, dir = DMA_FROM_DEVICE;
    BUG_ON(!cmd);
    data = mrq->data;
    if(data)
    data->host_cookie = MSDC_COOKIE_ASYNC;
    if(data && (cmd->opcode == MMC_READ_SINGLE_BLOCK || cmd->opcode == MMC_READ_MULTIPLE_BLOCK || cmd->opcode == MMC_WRITE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK)){
        host->xfer_size = data->blocks * data->blksz;
        read = data->flags & MMC_DATA_READ ? 1 : 0;
        if (drv_mode[host->id] == MODE_PIO) {
            data->host_cookie |= MSDC_COOKIE_PIO;
            msdc_latest_transfer_mode[host->id] = TRAN_MOD_PIO;
        } else if (drv_mode[host->id] == MODE_DMA) {
                   
            msdc_latest_transfer_mode[host->id] = TRAN_MOD_DMA;
        } else if (drv_mode[host->id] == MODE_SIZE_DEP) {
        if(host->xfer_size < dma_size[host->id])
            {
                data->host_cookie |= MSDC_COOKIE_PIO;
                msdc_latest_transfer_mode[host->id] = TRAN_MOD_PIO;            
            }
            else
            {
                msdc_latest_transfer_mode[host->id] = TRAN_MOD_DMA;
            }    
        }       
        if (msdc_async_use_dma(data->host_cookie)){
            dir = read ? DMA_FROM_DEVICE : DMA_TO_DEVICE;
            (void)dma_map_sg(mmc_dev(mmc), data->sg, data->sg_len, dir);
        }
        N_MSG(OPS, "CMD<%d> ARG<0x%x>data<%s %s> blksz<%d> block<%d> error<%d>",mrq->cmd->opcode,mrq->cmd->arg, (data->host_cookie ? "dma":"pio"), 
                (read ? "read ":"write") ,data->blksz, data->blocks, data->error);  
    }
    return;
}
static void msdc_dma_clear(struct msdc_host *host)
{        
    u32 base = host->base;
    host->data = NULL;
    host->mrq = NULL;
    host->dma_xfer = 0;    
    msdc_dma_off();     
    host->dma.used_bd  = 0;
    host->dma.used_gpd = 0;
    host->blksz = 0;  
}
static void msdc_post_req(struct mmc_host *mmc, struct mmc_request *mrq, int err)
{
    struct msdc_host *host = mmc_priv(mmc);
    struct mmc_data *data;
    //struct mmc_command *cmd = mrq->cmd;
    int  read = 1, dir = DMA_FROM_DEVICE;
    data = mrq->data;
    if(data && (msdc_async_use_dma(data->host_cookie)))    {        
        host->xfer_size = data->blocks * data->blksz;
        read = data->flags & MMC_DATA_READ ? 1 : 0;        
        dir = read ? DMA_FROM_DEVICE : DMA_TO_DEVICE;
        dma_unmap_sg(mmc_dev(mmc), data->sg, data->sg_len, dir);
        data->host_cookie = 0;
        N_MSG(OPS, "CMD<%d> ARG<0x%x> blksz<%d> block<%d> error<%d>",mrq->cmd->opcode,mrq->cmd->arg, 
                data->blksz, data->blocks, data->error);  
    }
    data->host_cookie = 0; 
    return;
        
}
static int msdc_do_request_async(struct mmc_host*mmc, struct mmc_request*mrq)
{
    struct msdc_host *host = mmc_priv(mmc);
    struct mmc_command *cmd;
    struct mmc_data *data;    
    u32 base = host->base;

#ifdef MTK_MSDC_USE_CMD23 
    u32 l_card_no_cmd23 = 0;
#endif

    //u32 intsts = 0;     
    //unsigned int left=0;
    int dma = 0, read = 1;//, dir = DMA_FROM_DEVICE;
    //u32 map_sg = 0;  /* Fix the bug of dma_map_sg and dma_unmap_sg not match issue */
    //u32 bus_mode = 0;    
    
    BUG_ON(mmc == NULL);
    BUG_ON(mrq == NULL);
    if (!is_card_present(host) || host->power_mode == MMC_POWER_OFF) {
        ERR_MSG("cmd<%d> arg<0x%x> card<%d> power<%d>", mrq->cmd->opcode,mrq->cmd->arg,is_card_present(host), host->power_mode);
        mrq->cmd->error = (unsigned int)-ENOMEDIUM;
    if(mrq->done)
        mrq->done(mrq);         // call done directly.
        return 0;
    }
    msdc_ungate_clock(host);
    host->tune = 0;

    host->error = 0;
    atomic_set(&host->abort, 0);
    spin_lock(&host->lock);  
    cmd  = mrq->cmd;
    data = mrq->cmd->data;
    host->mrq = mrq;
    /* check msdc is work ok. rule is RX/TX fifocnt must be zero after last request 
     * if find abnormal, try to reset msdc first
     */
    if (msdc_txfifocnt() || msdc_rxfifocnt()) {
        printk("[SD%d] register abnormal,please check!\n",host->id);
        msdc_reset_hw(host->id);
    }
    
#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
  #ifdef MTK_SDIO30_DETECT_THERMAL
    if(host->hw->host_function == MSDC_SDIO) {
        spin_lock_irqsave(&host->clk_gate_lock, flags);
        if(host->ot_period_check_start == 0) {
            mod_timer(&host->ot_timer, jiffies + OT_PERIOD);
            host->ot_period_check_start = 1;
        }
        spin_unlock_irqrestore(&host->clk_gate_lock, flags); 
    }
  #endif // #ifdef MTK_SDIO30_DETECT_THERMAL
#endif    // MTK_SDIO30_ONLINE_TUNING_SUPPORT
           
    BUG_ON(data->blksz > HOST_MAX_BLKSZ);
    //send_type=SND_DAT;

    data->error = 0;
    read = data->flags & MMC_DATA_READ ? 1 : 0;
    msdc_latest_operation_type[host->id] = read ? OPER_TYPE_READ : OPER_TYPE_WRITE;  
    host->data = data;
    host->xfer_size = data->blocks * data->blksz;
    host->blksz = data->blksz;
    host->dma_xfer = 1;
    /* deside the transfer mode */
        
    if (read) {
        if ((host->timeout_ns != data->timeout_ns) ||
            (host->timeout_clks != data->timeout_clks)) {
             msdc_set_timeout(host, data->timeout_ns, data->timeout_clks);
        }
    }
        
    msdc_set_blknum(host, data->blocks);
    //msdc_clr_fifo();  /* no need */
    msdc_dma_on();  /* enable DMA mode first!! */
    //init_completion(&host->xfer_done);

#ifdef MTK_MSDC_USE_CMD23
        // if tuning flow run here, no problem?? need check!!!!!!!
        if (0 == (host->autocmd & MSDC_AUTOCMD23)){
            /* start the cmd23 first*/ 
            if (mrq->sbc){
                host->autocmd &= ~MSDC_AUTOCMD12;

                if(host->hw->host_function == MSDC_EMMC){
                    /* remove reliable bit & data tag bit */
                    mrq->sbc->arg &= ~(1 << 31);
                    mrq->sbc->arg &= ~(1 << 29);
                }

                if (msdc_command_start(host, mrq->sbc, 0, CMD_TIMEOUT) != 0)
                    goto done;            

                /* then wait command done */
                if (msdc_command_resp_polling(host, mrq->sbc, 0, CMD_TIMEOUT) != 0) {  
                    goto stop;                           
                }           
            } else {
                /* some sd card may not support cmd23, 
                 * some emmc card have problem with cmd23, so use cmd12 here */
                if(host->hw->host_function != MSDC_SDIO){
                    host->autocmd |= MSDC_AUTOCMD12;
                }
            }
        } else {
            if (mrq->sbc){
                host->autocmd &= ~MSDC_AUTOCMD12;
            } else {
                /* some sd card may not support cmd23, 
                 * some emmc card have problem with cmd23, so use cmd12 here */
                if(host->hw->host_function != MSDC_SDIO){
                    host->autocmd &= ~MSDC_AUTOCMD23;
                    host->autocmd |= MSDC_AUTOCMD12;
                    l_card_no_cmd23 = 1;
                }
            }
        }

#else
        /* start the command first*/        	
        if(host->hw->host_function != MSDC_SDIO){
            host->autocmd |= MSDC_AUTOCMD12;
        }
#endif /* end of MTK_MSDC_USE_CMD23 */

    if (msdc_command_start(host, cmd, 0, CMD_TIMEOUT) != 0)
        goto done;            
                      
    /* then wait command done */
    if (msdc_command_resp_polling(host, cmd, 0, CMD_TIMEOUT) != 0) { // not tuning. 
        goto stop;                           
    }
           
    /* for read, the data coming too fast, then CRC error 
    start DMA no business with CRC. */
    //init_completion(&host->xfer_done);           
    msdc_dma_setup(host, &host->dma, data->sg, data->sg_len);  
    msdc_dma_start(host);
    //ERR_MSG("0.Power cycle enable(%d)",host->power_cycle_enable);  
    spin_unlock(&host->lock);

#ifdef MTK_MSDC_USE_CMD23 
    /* for msdc use cmd23, but card not supported(sbc is NULL), need enable autocmd23 for next request */
    if (1 == l_card_no_cmd23){
        if(host->hw->host_function != MSDC_SDIO){
            host->autocmd |= MSDC_AUTOCMD23;
            host->autocmd &= ~MSDC_AUTOCMD12;
            l_card_no_cmd23 = 0;
        }
    }
#endif
    return 0;
                     
        
stop:        
#ifndef MTK_MSDC_USE_CMD23
    /* Last: stop transfer */
    if (data && data->stop){ 
        if(!((cmd->error == 0) && (data->error == 0) && (host->autocmd & MSDC_AUTOCMD12) && (cmd->opcode == MMC_READ_MULTIPLE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK))){
            if (msdc_do_command(host, data->stop, 0, CMD_TIMEOUT) != 0) {
                goto done; 
            }
        }
    } 
#else

    if (host->hw->host_function == MSDC_EMMC) { 
        /* error handle will do msdc_abort_data() */
        } else {
            if (data && data->stop){ 
                if(!((cmd->error == 0) && (data->error == 0) && (host->autocmd & MSDC_AUTOCMD12) && 
                     (cmd->opcode == MMC_READ_MULTIPLE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK))){
                    if (msdc_do_command(host, data->stop, 0, CMD_TIMEOUT) != 0) {
                        goto done; 
                    }
                }
            }
        }
#endif 

done:
#ifdef MTK_MSDC_USE_CMD23 
    /* for msdc use cmd23, but card not supported(sbc is NULL), need enable autocmd23 for next request */
    if (1 == l_card_no_cmd23){
        if(host->hw->host_function != MSDC_SDIO){
            host->autocmd |= MSDC_AUTOCMD23;
            host->autocmd &= ~MSDC_AUTOCMD12;
            l_card_no_cmd23 = 0;
        }
    }
#endif

    msdc_dma_clear(host);
        
    N_MSG(OPS, "CMD<%d> data<%s %s> blksz<%d> block<%d> error<%d>",cmd->opcode, (dma? "dma":"pio"), 
                (read ? "read ":"write") ,data->blksz, data->blocks, data->error);                

    if (!(is_card_sdio(host)|| (host->hw->flags & MSDC_SDIO_IRQ))) {
        if ((cmd->opcode != 17)&&(cmd->opcode != 18)&&(cmd->opcode != 24)&&(cmd->opcode != 25)) {             
            N_MSG(NRW, "CMD<%3d> arg<0x%8x> Resp<0x%8x> data<%s> size<%d>", cmd->opcode, cmd->arg, cmd->resp[0],
                   (read ? "read ":"write") ,data->blksz * data->blocks);  
        } else {
            N_MSG(RW,  "CMD<%3d> arg<0x%8x> Resp<0x%8x> block<%d>", cmd->opcode,cmd->arg, cmd->resp[0], data->blocks);
        }
    }
     
#ifdef MTK_MSDC_USE_CMD23
    if (mrq->sbc && (mrq->sbc->error == (unsigned int)-EIO)) host->error |= REQ_CMD_EIO;
    if (mrq->sbc && (mrq->sbc->error == (unsigned int)-ETIMEDOUT)) host->error |= REQ_CMD_TMO;
#endif

    if (mrq->cmd->error == (unsigned int)-EIO) host->error |= REQ_CMD_EIO;
    if (mrq->cmd->error == (unsigned int)-ETIMEDOUT) host->error |= REQ_CMD_TMO;
    if (mrq->stop && (mrq->stop->error == (unsigned int)-EIO)) host->error |= REQ_STOP_EIO; 
    if (mrq->stop && (mrq->stop->error == (unsigned int)-ETIMEDOUT)) host->error |= REQ_STOP_TMO;
    
    /* if cmd send error occur, dma not start yet, just call done here, msdc_tune_async_request() will apply  */ 
    if(mrq->done)
        mrq->done(mrq); 
    
    msdc_gate_clock(host,1);
    spin_unlock(&host->lock);
    return host->error;
}

static int msdc_app_cmd(struct mmc_host *mmc, struct msdc_host *host)
{
    struct mmc_command cmd = {0};    
    struct mmc_request mrq = {0};
    u32 err = -1; 
   
    cmd.opcode = MMC_APP_CMD;    
    cmd.arg = host->app_cmd_arg;  /* meet mmc->card is null when ACMD6 */     
    cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC;
    
    mrq.cmd = &cmd; cmd.mrq = &mrq;
    cmd.data = NULL;        

    err = msdc_do_command(host, &cmd, 0, CMD_TIMEOUT);     
    return err;        
}

static int msdc_lower_freq(struct msdc_host *host)
{
    u32 div, mode; 
    u32 base = host->base;

    ERR_MSG("need to lower freq"); 
    msdc_reset_crc_tune_counter(host,all_counter);
    sdr_get_field(MSDC_CFG, MSDC_CFG_CKMOD, mode);                
    sdr_get_field(MSDC_CFG, MSDC_CFG_CKDIV, div);

    if (div >= MSDC_MAX_FREQ_DIV) {
        ERR_MSG("but, div<%d> power tuning", div);    
        return msdc_power_tuning(host);    
    } else if(mode == 1){
        mode = 0;
        msdc_clk_stable(host,mode, div);
        host->sclk = (div == 0) ? hclks[host->hw->clk_src]/2 : hclks[host->hw->clk_src]/(4*div);
        
        ERR_MSG("new div<%d>, mode<%d> new freq.<%dKHz>", div, mode,host->sclk/1000);
        return 0;
    } else{
        msdc_clk_stable(host,mode, div + 1);
        host->sclk = (mode == 2) ? hclks[host->hw->clk_src]/(2*4*(div+1)) : hclks[host->hw->clk_src]/(4*(div+1));
        ERR_MSG("new div<%d>, mode<%d> new freq.<%dKHz>", div + 1, mode,host->sclk/1000);
        return 0;
    }              
}

static int msdc_tune_cmdrsp(struct msdc_host *host)
{
    int result = 0;
    u32 base = host->base;
    u32 sel = 0;
    u32 cur_rsmpl = 0, orig_rsmpl;
    u32 cur_rrdly = 0, orig_rrdly;
    u32 cur_cntr  = 0,orig_cmdrtc;
    u32 cur_dl_cksel = 0, orig_dl_cksel;

    sdr_get_field(MSDC_IOCON, MSDC_IOCON_RSPL, orig_rsmpl);
    sdr_get_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, orig_rrdly);
    sdr_get_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_CMD_RSP, orig_cmdrtc);
    sdr_get_field(MSDC_PATCH_BIT0, MSDC_INT_DAT_LATCH_CK_SEL, orig_dl_cksel);
    if (unlikely(dumpMSDC()))
    {
        AddStorageTrace(STORAGE_LOGGER_MSG_MSDC_DO,msdc_do_request,"sd_tune_ori RSPL",orig_rsmpl); 
        AddStorageTrace(STORAGE_LOGGER_MSG_MSDC_DO,msdc_do_request,"sd_tune_ori RRDLY",orig_rrdly); 
    }
#if 1
    if (host->mclk >= 100000000){
        sel = 1;
        //sdr_set_field(MSDC_PATCH_BIT0, MSDC_CKGEN_RX_SDCLKO_SEL,0);
    } else {
        sdr_set_field(MSDC_PATCH_BIT1,MSDC_PATCH_BIT1_CMD_RSP,1);
        //sdr_set_field(MSDC_PATCH_BIT0, MSDC_CKGEN_RX_SDCLKO_SEL,1);
        sdr_set_field(MSDC_PATCH_BIT0, MSDC_INT_DAT_LATCH_CK_SEL,0);
    }
        
    cur_rsmpl = (orig_rsmpl + 1);
    sdr_set_field(MSDC_IOCON, MSDC_IOCON_RSPL, cur_rsmpl % 2);
    if (host->mclk <= 400000){//In sd/emmc init flow, fix rising edge for latching cmd response
        sdr_set_field(MSDC_IOCON, MSDC_IOCON_RSPL, 0);
        cur_rsmpl = 2;
    }
    if(cur_rsmpl >= 2){  
        cur_rrdly = (orig_rrdly + 1);
        sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, cur_rrdly % 32);
    }
    if(cur_rrdly >= 32){
        if(sel){
            cur_cntr = (orig_cmdrtc + 1) ;
            sdr_set_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_CMD_RSP, cur_cntr % 8);
        }
    }
    if(cur_cntr >= 8){
        if(sel){
            cur_dl_cksel = (orig_dl_cksel +1);
            sdr_set_field(MSDC_PATCH_BIT0, MSDC_INT_DAT_LATCH_CK_SEL, cur_dl_cksel % 8);
        }
    }
    ++(host->t_counter.time_cmd);
    if((sel && host->t_counter.time_cmd == CMD_TUNE_UHS_MAX_TIME)||(sel == 0 && host->t_counter.time_cmd == CMD_TUNE_HS_MAX_TIME)){
#ifdef MSDC_LOWER_FREQ
        result = msdc_lower_freq(host);
#else
        result = 1;
#endif
        host->t_counter.time_cmd = 0;
    }
#else
    if (orig_rsmpl == 0) {
        cur_rsmpl = 1; 
        sdr_set_field(MSDC_IOCON, MSDC_IOCON_RSPL, cur_rsmpl);                      
    } else {
        cur_rsmpl = 0;   
        sdr_set_field(MSDC_IOCON, MSDC_IOCON_RSPL, cur_rsmpl);  // need second layer       
        cur_rrdly = (orig_rrdly + 1); 
        if (cur_rrdly >= 32) {
            ERR_MSG("failed to update rrdly<%d>", cur_rrdly); 
            sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, 0);
#ifdef MSDC_LOWER_FREQ
            return (msdc_lower_freq(host));                      
#else
            return 1;
#endif     
        }     
        sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, cur_rrdly);
    }

#endif
    sdr_get_field(MSDC_IOCON, MSDC_IOCON_RSPL, orig_rsmpl);
    sdr_get_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, orig_rrdly);
    sdr_get_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_CMD_RSP, orig_cmdrtc);
    sdr_get_field(MSDC_PATCH_BIT0, MSDC_INT_DAT_LATCH_CK_SEL, orig_dl_cksel);
    INIT_MSG("TUNE_CMD: rsmpl<%d> rrdly<%d> cmdrtc<%d> dl_cksel<%d> sfreq.<%d>", orig_rsmpl, orig_rrdly,orig_cmdrtc,orig_dl_cksel,host->sclk);      
    if (unlikely(dumpMSDC()))
    {
        AddStorageTrace(STORAGE_LOGGER_MSG_MSDC_DO,msdc_do_request,"sd_tune_ok RSPL",orig_rsmpl); 
        AddStorageTrace(STORAGE_LOGGER_MSG_MSDC_DO,msdc_do_request,"sd_tune_ok RRDLY",orig_rrdly); 
    }      
    return result;
}

static int msdc_tune_read(struct msdc_host *host)
{
    u32 base = host->base;
    u32 sel = 0;
    u32 ddr = 0;    
    u32 dcrc;
    u32 clkmode = 0;
    u32 cur_rxdly0, cur_rxdly1;
    u32 cur_dsmpl = 0, orig_dsmpl;
    u32 cur_dsel = 0,orig_dsel;
    u32 cur_dl_cksel = 0,orig_dl_cksel;
    u32 cur_dat0 = 0, cur_dat1 = 0, cur_dat2 = 0, cur_dat3 = 0, 
    cur_dat4 = 0, cur_dat5 = 0, cur_dat6 = 0, cur_dat7 = 0;
    u32 orig_dat0, orig_dat1, orig_dat2, orig_dat3, orig_dat4, orig_dat5, orig_dat6, orig_dat7;
    int result = 0;

#if 1
    if (host->mclk >= 100000000){
        sel = 1;
    } else{
        sdr_set_field(MSDC_PATCH_BIT0, MSDC_CKGEN_MSDC_DLY_SEL, 0);
    }
    sdr_get_field(MSDC_CFG,MSDC_CFG_CKMOD,clkmode);
    ddr = (clkmode == 2) ? 1 : 0;

    sdr_get_field(MSDC_PATCH_BIT0, MSDC_CKGEN_MSDC_DLY_SEL, orig_dsel);
    sdr_get_field(MSDC_PATCH_BIT0, MSDC_INT_DAT_LATCH_CK_SEL, orig_dl_cksel);
    sdr_get_field(MSDC_IOCON, MSDC_IOCON_DSPL, orig_dsmpl);
    
    sdr_set_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);
    cur_dsmpl = (orig_dsmpl + 1) ;
    sdr_set_field(MSDC_IOCON, MSDC_IOCON_DSPL, cur_dsmpl % 2);
    
    if(cur_dsmpl >= 2){
        sdr_get_field(SDC_DCRC_STS, SDC_DCRC_STS_POS | SDC_DCRC_STS_NEG, dcrc);        
        if (!ddr) dcrc &= ~SDC_DCRC_STS_NEG;
        cur_rxdly0 = sdr_read32(MSDC_DAT_RDDLY0);
        cur_rxdly1 = sdr_read32(MSDC_DAT_RDDLY1); 
        
            orig_dat0 = (cur_rxdly0 >> 24) & 0x1F;
            orig_dat1 = (cur_rxdly0 >> 16) & 0x1F;
            orig_dat2 = (cur_rxdly0 >>  8) & 0x1F;
            orig_dat3 = (cur_rxdly0 >>  0) & 0x1F;
            orig_dat4 = (cur_rxdly1 >> 24) & 0x1F;
            orig_dat5 = (cur_rxdly1 >> 16) & 0x1F;
            orig_dat6 = (cur_rxdly1 >>  8) & 0x1F;
            orig_dat7 = (cur_rxdly1 >>  0) & 0x1F;

        if (ddr) {
            cur_dat0 = (dcrc & (1 << 0) || dcrc & (1 <<  8)) ? (orig_dat0 + 1) : orig_dat0;
            cur_dat1 = (dcrc & (1 << 1) || dcrc & (1 <<  9)) ? (orig_dat1 + 1) : orig_dat1;
            cur_dat2 = (dcrc & (1 << 2) || dcrc & (1 << 10)) ? (orig_dat2 + 1) : orig_dat2;
            cur_dat3 = (dcrc & (1 << 3) || dcrc & (1 << 11)) ? (orig_dat3 + 1) : orig_dat3;
            cur_dat4 = (dcrc & (1 << 4) || dcrc & (1 << 12)) ? (orig_dat4 + 1) : orig_dat4;
            cur_dat5 = (dcrc & (1 << 5) || dcrc & (1 << 13)) ? (orig_dat5 + 1) : orig_dat5;
            cur_dat6 = (dcrc & (1 << 6) || dcrc & (1 << 14)) ? (orig_dat6 + 1) : orig_dat6;
            cur_dat7 = (dcrc & (1 << 7) || dcrc & (1 << 15)) ? (orig_dat7 + 1) : orig_dat7;
        } else {
            cur_dat0 = (dcrc & (1 << 0)) ? (orig_dat0 + 1) : orig_dat0;
            cur_dat1 = (dcrc & (1 << 1)) ? (orig_dat1 + 1) : orig_dat1;
            cur_dat2 = (dcrc & (1 << 2)) ? (orig_dat2 + 1) : orig_dat2;
            cur_dat3 = (dcrc & (1 << 3)) ? (orig_dat3 + 1) : orig_dat3;
            cur_dat4 = (dcrc & (1 << 4)) ? (orig_dat4 + 1) : orig_dat4;
            cur_dat5 = (dcrc & (1 << 5)) ? (orig_dat5 + 1) : orig_dat5;
            cur_dat6 = (dcrc & (1 << 6)) ? (orig_dat6 + 1) : orig_dat6;
            cur_dat7 = (dcrc & (1 << 7)) ? (orig_dat7 + 1) : orig_dat7;
        }
       
      
        cur_rxdly0 = ((cur_dat0 & 0x1F) << 24) | ((cur_dat1 & 0x1F) << 16) |
                     ((cur_dat2 & 0x1F) << 8)  | ((cur_dat3 & 0x1F) << 0);
        cur_rxdly1 = ((cur_dat4 & 0x1F) << 24) | ((cur_dat5 & 0x1F) << 16) |
                     ((cur_dat6 & 0x1F) << 8)  | ((cur_dat7 & 0x1F) << 0);
        
     
        sdr_write32(MSDC_DAT_RDDLY0, cur_rxdly0);
        sdr_write32(MSDC_DAT_RDDLY1, cur_rxdly1); 
        
    }
    if((cur_dat0 >= 32) || (cur_dat1 >= 32) || (cur_dat2 >= 32) || (cur_dat3 >= 32)||
        (cur_dat4 >= 32) || (cur_dat5 >= 32) || (cur_dat6 >= 32) || (cur_dat7 >= 32)){
        if(sel){
            sdr_write32(MSDC_DAT_RDDLY0, 0);
            sdr_write32(MSDC_DAT_RDDLY1, 0); 
            cur_dsel = (orig_dsel + 1) ;
            sdr_set_field(MSDC_PATCH_BIT0, MSDC_CKGEN_MSDC_DLY_SEL, cur_dsel % 32);
        }
    }
    if(cur_dsel >= 32 ){
        if(clkmode == 1 && sel){
            cur_dl_cksel = (orig_dl_cksel + 1);
            sdr_set_field(MSDC_PATCH_BIT0, MSDC_INT_DAT_LATCH_CK_SEL, cur_dl_cksel % 8);
        }
    }
    ++(host->t_counter.time_read);
    if((sel == 1 && clkmode == 1 && host->t_counter.time_read == READ_TUNE_UHS_CLKMOD1_MAX_TIME)||
        (sel == 1 && (clkmode == 0 ||clkmode == 2) && host->t_counter.time_read == READ_TUNE_UHS_MAX_TIME)||
        (sel == 0 && (clkmode == 0 ||clkmode == 2) && host->t_counter.time_read == READ_TUNE_HS_MAX_TIME)){
#ifdef MSDC_LOWER_FREQ
        result = msdc_lower_freq(host);         
#else
        result = 1;
#endif
        host->t_counter.time_read = 0;
    }    
#else
    sdr_set_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);
    
    cur_rxdly0 = sdr_read32(MSDC_DAT_RDDLY0);
    cur_rxdly1 = sdr_read32(MSDC_DAT_RDDLY1);
    sdr_get_field(MSDC_IOCON, MSDC_IOCON_DSPL, orig_dsmpl);   
    if (orig_dsmpl == 0) {
        cur_dsmpl = 1; 
        sdr_set_field(MSDC_IOCON, MSDC_IOCON_DSPL, cur_dsmpl);                        
    } else {
        cur_dsmpl = 0; 
        sdr_set_field(MSDC_IOCON, MSDC_IOCON_DSPL, cur_dsmpl); // need second layer

        sdr_get_field(SDC_DCRC_STS, SDC_DCRC_STS_POS | SDC_DCRC_STS_NEG, dcrc);        
        if (!ddr) dcrc &= ~SDC_DCRC_STS_NEG;

        if (sdr_read32(MSDC_ECO_VER) >= 4) {
            orig_dat0 = (cur_rxdly0 >> 24) & 0x1F;
            orig_dat1 = (cur_rxdly0 >> 16) & 0x1F;
            orig_dat2 = (cur_rxdly0 >>  8) & 0x1F;
            orig_dat3 = (cur_rxdly0 >>  0) & 0x1F;
            orig_dat4 = (cur_rxdly1 >> 24) & 0x1F;
            orig_dat5 = (cur_rxdly1 >> 16) & 0x1F;
            orig_dat6 = (cur_rxdly1 >>  8) & 0x1F;
            orig_dat7 = (cur_rxdly1 >>  0) & 0x1F;
        } else {   
            orig_dat0 = (cur_rxdly0 >>  0) & 0x1F;
            orig_dat1 = (cur_rxdly0 >>  8) & 0x1F;
            orig_dat2 = (cur_rxdly0 >> 16) & 0x1F;
            orig_dat3 = (cur_rxdly0 >> 24) & 0x1F;
            orig_dat4 = (cur_rxdly1 >>  0) & 0x1F;
            orig_dat5 = (cur_rxdly1 >>  8) & 0x1F;
            orig_dat6 = (cur_rxdly1 >> 16) & 0x1F;
            orig_dat7 = (cur_rxdly1 >> 24) & 0x1F;
        }
        
        if (ddr) {
            cur_dat0 = (dcrc & (1 << 0) || dcrc & (1 << 8))  ? (orig_dat0 + 1) : orig_dat0;
            cur_dat1 = (dcrc & (1 << 1) || dcrc & (1 << 9))  ? (orig_dat1 + 1) : orig_dat1;
            cur_dat2 = (dcrc & (1 << 2) || dcrc & (1 << 10)) ? (orig_dat2 + 1) : orig_dat2;
            cur_dat3 = (dcrc & (1 << 3) || dcrc & (1 << 11)) ? (orig_dat3 + 1) : orig_dat3;
        } else {
            cur_dat0 = (dcrc & (1 << 0)) ? (orig_dat0 + 1) : orig_dat0;
            cur_dat1 = (dcrc & (1 << 1)) ? (orig_dat1 + 1) : orig_dat1;
            cur_dat2 = (dcrc & (1 << 2)) ? (orig_dat2 + 1) : orig_dat2;
            cur_dat3 = (dcrc & (1 << 3)) ? (orig_dat3 + 1) : orig_dat3;
        }
        cur_dat4 = (dcrc & (1 << 4)) ? (orig_dat4 + 1) : orig_dat4;
        cur_dat5 = (dcrc & (1 << 5)) ? (orig_dat5 + 1) : orig_dat5;
        cur_dat6 = (dcrc & (1 << 6)) ? (orig_dat6 + 1) : orig_dat6;
        cur_dat7 = (dcrc & (1 << 7)) ? (orig_dat7 + 1) : orig_dat7;

        if (cur_dat0 >= 32 || cur_dat1 >= 32 || cur_dat2 >= 32 || cur_dat3 >= 32) {
            ERR_MSG("failed to update <%xh><%xh><%xh><%xh>", cur_dat0, cur_dat1, cur_dat2, cur_dat3);  
            sdr_write32(MSDC_DAT_RDDLY0, 0); 
            sdr_write32(MSDC_DAT_RDDLY1, 0); 
                          
#ifdef MSDC_LOWER_FREQ
            return (msdc_lower_freq(host));                       
#else
            return 1;
#endif     
        }

        if (cur_dat4 >= 32 || cur_dat5 >= 32 || cur_dat6 >= 32 || cur_dat7 >= 32) {
            ERR_MSG("failed to update <%xh><%xh><%xh><%xh>", cur_dat4, cur_dat5, cur_dat6, cur_dat7);
            sdr_write32(MSDC_DAT_RDDLY0, 0); 
            sdr_write32(MSDC_DAT_RDDLY1, 0); 
                          
#ifdef MSDC_LOWER_FREQ
            return (msdc_lower_freq(host));                      
#else
            return 1;
#endif
        }

        cur_rxdly0 = (cur_dat0 << 24) | (cur_dat1 << 16) | (cur_dat2 << 8) | (cur_dat3 << 0);
        cur_rxdly1 = (cur_dat4 << 24) | (cur_dat5 << 16) | (cur_dat6 << 8) | (cur_dat7 << 0);
        
        sdr_write32(MSDC_DAT_RDDLY0, cur_rxdly0);
        sdr_write32(MSDC_DAT_RDDLY1, cur_rxdly1);                             
    }

#endif
    sdr_get_field(MSDC_PATCH_BIT0, MSDC_CKGEN_MSDC_DLY_SEL, orig_dsel);
    sdr_get_field(MSDC_PATCH_BIT0, MSDC_INT_DAT_LATCH_CK_SEL, orig_dl_cksel);
    sdr_get_field(MSDC_IOCON, MSDC_IOCON_DSPL, orig_dsmpl);
    cur_rxdly0 = sdr_read32(MSDC_DAT_RDDLY0);
    cur_rxdly1 = sdr_read32(MSDC_DAT_RDDLY1); 
    INIT_MSG("TUNE_READ: dsmpl<%d> rxdly0<0x%x> rxdly1<0x%x> dsel<%d> dl_cksel<%d> sfreq.<%d>", orig_dsmpl, cur_rxdly0, cur_rxdly1,orig_dsel,orig_dl_cksel,host->sclk);

    return result;
}

static int msdc_tune_write(struct msdc_host *host)
{
    u32 base = host->base;

    //u32 cur_wrrdly = 0, orig_wrrdly;
    u32 cur_dsmpl = 0,  orig_dsmpl;
    u32 cur_rxdly0 = 0;
    u32 orig_dat0, orig_dat1, orig_dat2, orig_dat3;
    u32 cur_dat0 = 0,cur_dat1 = 0,cur_dat2 = 0,cur_dat3 = 0;
    u32 cur_d_cntr = 0 , orig_d_cntr;
    int result = 0;

    int sel = 0;
    int clkmode = 0;
    // MSDC_IOCON_DDR50CKD need to check. [Fix me] 
#if 1
    if (host->mclk >= 100000000){
        sel = 1;
    } else {
        sdr_set_field(MSDC_PATCH_BIT1,MSDC_PATCH_BIT1_WRDAT_CRCS,1);
    }
    sdr_get_field(MSDC_CFG,MSDC_CFG_CKMOD,clkmode);

    //sdr_get_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATWRDLY, orig_wrrdly);
    sdr_get_field(MSDC_IOCON, MSDC_IOCON_W_DSPL, orig_dsmpl);  
    sdr_get_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_WRDAT_CRCS, orig_d_cntr);

    sdr_set_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);
    cur_dsmpl = (orig_dsmpl + 1);
    sdr_set_field(MSDC_IOCON, MSDC_IOCON_W_DSPL, cur_dsmpl % 2);
#if 0
    if(cur_dsmpl >= 2){
        cur_wrrdly = (orig_wrrdly+1);
        sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATWRDLY, cur_wrrdly % 32);
    }
#endif
    if(cur_dsmpl >= 2){
        cur_rxdly0 = sdr_read32(MSDC_DAT_RDDLY0);
       
        orig_dat0 = (cur_rxdly0 >> 24) & 0x1F;
        orig_dat1 = (cur_rxdly0 >> 16) & 0x1F;
        orig_dat2 = (cur_rxdly0 >>  8) & 0x1F;
        orig_dat3 = (cur_rxdly0 >>  0) & 0x1F;
        
        cur_dat0 = (orig_dat0 + 1); /* only adjust bit-1 for crc */
        cur_dat1 = orig_dat1;
        cur_dat2 = orig_dat2;
        cur_dat3 = orig_dat3;
        
        cur_rxdly0 = ((cur_dat0 & 0x1F) << 24) | ((cur_dat1 & 0x1F) << 16) |
        ((cur_dat2 & 0x1F) << 8) | ((cur_dat3 & 0x1F) << 0);
        
        sdr_write32(MSDC_DAT_RDDLY0, cur_rxdly0);
    }
    if(cur_dat0 >= 32){
        if(sel){
            cur_d_cntr= (orig_d_cntr + 1 );
            sdr_set_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_WRDAT_CRCS, cur_d_cntr % 8);
        }
    }
    ++(host->t_counter.time_write);
    if((sel == 0 && host->t_counter.time_write == WRITE_TUNE_HS_MAX_TIME) || (sel && host->t_counter.time_write == WRITE_TUNE_UHS_MAX_TIME)){
#ifdef MSDC_LOWER_FREQ
        result = msdc_lower_freq(host);         
#else
        result = 1;
#endif
        host->t_counter.time_write = 0;
    }
        
#else
    
    /* Tune Method 2. just DAT0 */  
    sdr_set_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);
    cur_rxdly0 = sdr_read32(MSDC_DAT_RDDLY0);
    if (sdr_read32(MSDC_ECO_VER) >= 4) {
        orig_dat0 = (cur_rxdly0 >> 24) & 0x1F;
        orig_dat1 = (cur_rxdly0 >> 16) & 0x1F;
        orig_dat2 = (cur_rxdly0 >>  8) & 0x1F;
        orig_dat3 = (cur_rxdly0 >>  0) & 0x1F;
    } else {
        orig_dat0 = (cur_rxdly0 >>  0) & 0x1F;
        orig_dat1 = (cur_rxdly0 >>  8) & 0x1F;
        orig_dat2 = (cur_rxdly0 >> 16) & 0x1F;
        orig_dat3 = (cur_rxdly0 >> 24) & 0x1F;
    }

    sdr_get_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATWRDLY, orig_wrrdly);
    cur_wrrdly = orig_wrrdly;         
    sdr_get_field(MSDC_IOCON,    MSDC_IOCON_W_DSPL,        orig_dsmpl );
    if (orig_dsmpl == 0) {
        cur_dsmpl = 1;
        sdr_set_field(MSDC_IOCON, MSDC_IOCON_W_DSPL, cur_dsmpl);        
    } else {
        cur_dsmpl = 0;
        sdr_set_field(MSDC_IOCON, MSDC_IOCON_W_DSPL, cur_dsmpl);  // need the second layer 
        
        cur_wrrdly = (orig_wrrdly + 1);
        if (cur_wrrdly < 32) {
            sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATWRDLY, cur_wrrdly);             
        } else {
            cur_wrrdly = 0; 
            sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATWRDLY, cur_wrrdly);  // need third 

            cur_dat0 = orig_dat0 + 1; /* only adjust bit-1 for crc */
            cur_dat1 = orig_dat1;
            cur_dat2 = orig_dat2;
            cur_dat3 = orig_dat3;  
            
            if (cur_dat0 >= 32) {
                ERR_MSG("update failed <%xh>", cur_dat0);    
                sdr_write32(MSDC_DAT_RDDLY0, 0); 
                
#ifdef MSDC_LOWER_FREQ
                    return (msdc_lower_freq(host));                      
#else
                    return 1;
#endif 
            }
                       
            cur_rxdly0 = (cur_dat0 << 24) | (cur_dat1 << 16) | (cur_dat2 << 8) | (cur_dat3 << 0); 
            sdr_write32(MSDC_DAT_RDDLY0, cur_rxdly0);          
        }
                   
    }

#endif
    //sdr_get_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATWRDLY, orig_wrrdly);
    sdr_get_field(MSDC_IOCON, MSDC_IOCON_W_DSPL, orig_dsmpl);  
    sdr_get_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_WRDAT_CRCS, orig_d_cntr);
    cur_rxdly0 = sdr_read32(MSDC_DAT_RDDLY0);
    INIT_MSG("TUNE_WRITE: dsmpl<%d> rxdly0<0x%x> d_cntr<%d> sfreq.<%d>", orig_dsmpl,cur_rxdly0,orig_d_cntr,host->sclk);
    
    return result;
}

static int msdc_get_card_status(struct mmc_host *mmc, struct msdc_host *host, u32 *status)
{
    struct mmc_command cmd;    
    struct mmc_request mrq;
    u32 err; 

    memset(&cmd, 0, sizeof(struct mmc_command));    
    cmd.opcode = MMC_SEND_STATUS;    // CMD13       
    cmd.arg = host->app_cmd_arg;        
    cmd.flags = MMC_RSP_SPI_R2 | MMC_RSP_R1 | MMC_CMD_AC;

    memset(&mrq, 0, sizeof(struct mmc_request));
    mrq.cmd = &cmd; cmd.mrq = &mrq;
    cmd.data = NULL;        

    err = msdc_do_command(host, &cmd, 0, CMD_TIMEOUT);  // tune until CMD13 pass.      
    
    if (status) {
        *status = cmd.resp[0];
    }    
    
    return err;                    
}

//#define TUNE_FLOW_TEST
#ifdef TUNE_FLOW_TEST
static void msdc_reset_para(struct msdc_host *host)
{
    u32 base = host->base; 
    u32 dsmpl, rsmpl;

    // because we have a card, which must work at dsmpl<0> and rsmpl<0>

    sdr_get_field(MSDC_IOCON, MSDC_IOCON_DSPL, dsmpl);
    sdr_get_field(MSDC_IOCON, MSDC_IOCON_RSPL, rsmpl);

    if (dsmpl == 0) {
        sdr_set_field(MSDC_IOCON, MSDC_IOCON_DSPL, 1);  
        ERR_MSG("set dspl<0>");      
        sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, 0);        
    }
    
    if (rsmpl == 0) {
        sdr_set_field(MSDC_IOCON, MSDC_IOCON_RSPL, 1);
        ERR_MSG("set rspl<0>");      
        sdr_write32(MSDC_DAT_RDDLY0, 0);
        sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATWRDLY, 0);
    }       
}
#endif

static void msdc_dump_trans_error(struct msdc_host   *host,
                                  struct mmc_command *cmd,
                                  struct mmc_data    *data,
                                  struct mmc_command *stop,
                                  struct mmc_command *sbc)
{
    //u32 base = host->base; 
      
    if ((cmd->opcode == 52) && (cmd->arg == 0xc00)) return; 
    if ((cmd->opcode == 52) && (cmd->arg == 0x80000c08)) return;     

    if (!(is_card_sdio(host)|| (host->hw->flags & MSDC_SDIO_IRQ))) { // by pass the SDIO CMD TO for SD/eMMC 
        if ((host->hw->host_function == MSDC_SD) && (cmd->opcode == 5)) return;                 
    }else {
        if (cmd->opcode == 8) return;        
    }
#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT    
    if ((is_card_sdio(host)) && (atomic_read(&host->ot_work.autok_done) == 0))   // auto-K have not done or finished
    {
        return;
    }
#endif
    
    ERR_MSG("XXX CMD<%d><0x%x> Error<%d> Resp<0x%x>", cmd->opcode, cmd->arg, cmd->error, cmd->resp[0]);    
    
    if (data) {
        ERR_MSG("XXX DAT block<%d> Error<%d>", data->blocks, data->error);                                
    }   
    
    if (stop) {
        ERR_MSG("XXX STOP<%d><0x%x> Error<%d> Resp<0x%x>", stop->opcode, stop->arg, stop->error, stop->resp[0]);                            
    }

    if (sbc) {
        ERR_MSG("XXX SBC<%d><0x%x> Error<%d> Resp<0x%x>", sbc->opcode, sbc->arg, sbc->error, sbc->resp[0]);                            
    }

    if((host->hw->host_function == MSDC_SD) && 
            (host->sclk > 100000000) && 
            (data) &&
            (data->error != (unsigned int)-ETIMEDOUT))
    {
        if((data->flags & MMC_DATA_WRITE) && (host->write_timeout_uhs104))
            host->write_timeout_uhs104 = 0;
        if((data->flags & MMC_DATA_READ) && (host->read_timeout_uhs104))
            host->read_timeout_uhs104 = 0;
    }

    if((host->hw->host_function == MSDC_EMMC) && 
            (data) &&
            (data->error != (unsigned int)-ETIMEDOUT))
    {
        if((data->flags & MMC_DATA_WRITE) && (host->write_timeout_emmc))
            host->write_timeout_emmc = 0;
        if((data->flags & MMC_DATA_READ) && (host->read_timeout_emmc))
            host->read_timeout_emmc = 0;
    }
#ifdef SDIO_ERROR_BYPASS  		
    if(is_card_sdio(host)&&(host->sdio_error!=-EIO)&&(cmd->opcode==53)){
       host->sdio_error = -EIO;  
    }
#endif    		
}

/* ops.request */
static void msdc_ops_request_legacy(struct mmc_host *mmc, struct mmc_request *mrq)
{   
    struct msdc_host *host = mmc_priv(mmc);
    struct mmc_command *cmd;    
    struct mmc_data *data;
    struct mmc_command *stop = NULL;
    struct mmc_command *sbc = NULL;
    int data_abort = 0;
    //int got_polarity = 0;
    //unsigned long flags;
    //=== for sdio profile ===
    u32 old_H32 = 0, old_L32 = 0, new_H32 = 0, new_L32 = 0;
    u32 ticks = 0, opcode = 0, sizes = 0, bRx = 0; 
    msdc_reset_crc_tune_counter(host,all_counter);      
    if(host->mrq){
        ERR_MSG("XXX host->mrq<0x%.8x> cmd<%d>arg<0x%x>", (int)host->mrq,host->mrq->cmd->opcode,host->mrq->cmd->arg);   
        BUG();    
    }            
      
    if (!is_card_present(host) || host->power_mode == MMC_POWER_OFF) {
        ERR_MSG("cmd<%d> arg<0x%x> card<%d> power<%d>", mrq->cmd->opcode,mrq->cmd->arg,is_card_present(host), host->power_mode);
        mrq->cmd->error = (unsigned int)-ENOMEDIUM; 
        
#if 1        
        if(mrq->done)
            mrq->done(mrq);         // call done directly.
#else
        mrq->cmd->retries = 0;  // please don't retry.
        mmc_request_done(mmc, mrq);
#endif
        return;
    }
      
    /* start to process */
    spin_lock(&host->lock);  
    host->power_cycle_enable = 1;

    cmd = mrq->cmd;      
    data = mrq->cmd->data;
    if (data) stop = data->stop;

#ifdef MTK_MSDC_USE_CMD23 
    if (data) sbc = mrq->sbc;
#endif

    msdc_ungate_clock(host);  // set sw flag 
         
    if (sdio_pro_enable) {  //=== for sdio profile ===  
        if (mrq->cmd->opcode == 52 || mrq->cmd->opcode == 53) {    
            //GPT_GetCounter64(&old_L32, &old_H32); 
        }
    }
    
    host->mrq = mrq;    

    while (msdc_do_request(mmc,mrq)) { // there is some error     
        // becasue ISR executing time will be monitor, try to dump the info here. 
        msdc_dump_trans_error(host, cmd, data, stop, sbc); 
        data_abort = 0;     
        if (is_card_sdio(host)|| (host->hw->flags & MSDC_SDIO_IRQ)) {
            goto out;  // sdio not tuning         
        }       

#ifdef MTK_MSDC_USE_CMD23
        if ((sbc != NULL) && (sbc->error == (unsigned int)-ETIMEDOUT)) {
            if (cmd->opcode == MMC_READ_MULTIPLE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK ){
                /* not tuning, go out directly */
                printk("===[%s:%d]==cmd23 timeout==\n", __func__, __LINE__);
                goto out;
            }
        }
#endif


#ifdef MTK_MSDC_USE_CMD23
        /* cmd->error also set when autocmd23 crc error */
        if ((cmd->error == (unsigned int)-EIO) || (stop && (stop->error == (unsigned int)-EIO)) || 
                (sbc && (sbc->error == (unsigned int)-EIO))) {
#else 
        if ((cmd->error == (unsigned int)-EIO) || (stop && (stop->error == (unsigned int)-EIO))) {
#endif
            if (msdc_tune_cmdrsp(host)){
                ERR_MSG("failed to updata cmd para");
                goto out;    
            }     
        }        

        if (data && (data->error == (unsigned int)-EIO)) {
            if (data->flags & MMC_DATA_READ) {  // read 
                if (msdc_tune_read(host)) {
                    ERR_MSG("failed to updata read para");   
                    goto out; 
                }  
            } else {
                if (msdc_tune_write(host)) {
                    ERR_MSG("failed to updata write para");
                    goto out;
                }  
            }             
        }

        // bring the card to "tran" state
        if (data) {
            if (msdc_abort_data(host)) {
                ERR_MSG("abort failed");
                data_abort = 1;
                if(host->hw->host_function == MSDC_SD){
                    if(host->card_inserted){
                        ERR_MSG("go to remove the bad card"); 
                        msdc_set_bad_card_and_remove(host);
                    }
                    goto out;
                }
            }            
        }

        // CMD TO -> not tuning 
        if (cmd->error == (unsigned int)-ETIMEDOUT) {
            if(cmd->opcode == MMC_READ_SINGLE_BLOCK || cmd->opcode == MMC_READ_MULTIPLE_BLOCK || cmd->opcode == MMC_WRITE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK ){
                if(data_abort){
                    if(msdc_power_tuning(host))
                    goto out;
                }
            }else {
                goto out;
            }
        } 

        // [ALPS114710] Patch for data timeout issue.
        if (data && (data->error == (unsigned int)-ETIMEDOUT)) {  
            if (data->flags & MMC_DATA_READ) {
                if( !(host->sw_timeout) && 
                    (host->hw->host_function == MSDC_SD) && 
                    (host->sclk > 100000000) && 
                    (host->read_timeout_uhs104 < MSDC_MAX_R_TIMEOUT_TUNE))
                {
                    if(host->t_counter.time_read)
                        host->t_counter.time_read--;
                    host->read_timeout_uhs104++;
                    msdc_tune_read(host);
                }
                else if((host->sw_timeout) || (host->read_timeout_uhs104 >= MSDC_MAX_R_TIMEOUT_TUNE) || (++(host->read_time_tune) > MSDC_MAX_TIMEOUT_RETRY)){
                    ERR_MSG("msdc%d exceed max read timeout retry times(%d) or SW timeout(%d) or read timeout tuning times(%d),Power cycle\n",host->id,host->read_time_tune,host->sw_timeout,host->read_timeout_uhs104);
                    if(msdc_power_tuning(host))
                        goto out;
                }
            } else if(data->flags & MMC_DATA_WRITE){
                if( (!(host->sw_timeout)) &&
                   (host->hw->host_function == MSDC_SD) && 
                   (host->sclk > 100000000) && 
                   (host->write_timeout_uhs104 < MSDC_MAX_W_TIMEOUT_TUNE))
                {
                    if(host->t_counter.time_write)
                        host->t_counter.time_write--;
                    host->write_timeout_uhs104++;
                    msdc_tune_write(host);
                } else if ( !(host->sw_timeout) && 
                    (host->hw->host_function == MSDC_EMMC) && 
                    (host->write_timeout_emmc < MSDC_MAX_W_TIMEOUT_TUNE_EMMC))
                {
                    if(host->t_counter.time_write)
                        host->t_counter.time_write--;
                    host->write_timeout_emmc++;
                    msdc_tune_write(host);
                } else if((host->hw->host_function == MSDC_SD) && ((host->sw_timeout) || (host->write_timeout_uhs104 >= MSDC_MAX_W_TIMEOUT_TUNE) || (++(host->write_time_tune) > MSDC_MAX_TIMEOUT_RETRY))){
                    ERR_MSG("msdc%d exceed max write timeout retry times(%d) or SW timeout(%d) or write timeout tuning time(%d),Power cycle\n",host->id,host->write_time_tune,host->sw_timeout,host->write_timeout_uhs104);
                    if(!(host->sd_30_busy) && msdc_power_tuning(host))
                        goto out;
                } else if((host->hw->host_function == MSDC_EMMC) && ((host->sw_timeout) || (++(host->write_time_tune) > MSDC_MAX_TIMEOUT_RETRY_EMMC))){
                    ERR_MSG("msdc%d exceed max write timeout retry times(%d) or SW timeout(%d) or write timeout tuning time(%d),Power cycle\n",host->id,host->write_time_tune,host->sw_timeout,host->write_timeout_emmc);
                    host->write_timeout_emmc = 0;
                    goto out;
                }
            }
        }        

        // clear the error condition.
        cmd->error = 0; 
        if (data) data->error = 0;
        if (stop) stop->error = 0; 

#ifdef MTK_MSDC_USE_CMD23
        if (sbc) sbc->error = 0; 
#endif

        // check if an app commmand.  
        if (host->app_cmd) {
            while (msdc_app_cmd(host->mmc, host)) {
                if (msdc_tune_cmdrsp(host)){
                    ERR_MSG("failed to updata cmd para for app");
                    goto out;    
                }   
            } 
        } 
         
        if (!is_card_present(host)) {
            goto out;
        }        
    }
    
        
    if((host->read_time_tune)&&(cmd->opcode == MMC_READ_SINGLE_BLOCK || cmd->opcode == MMC_READ_MULTIPLE_BLOCK)){
        host->read_time_tune = 0;
        ERR_MSG("Read recover");
        msdc_dump_trans_error(host, cmd, data, stop, sbc); 
    }
    if((host->write_time_tune) && (cmd->opcode == MMC_WRITE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK)){
        host->write_time_tune = 0;
        ERR_MSG("Write recover");
        msdc_dump_trans_error(host, cmd, data, stop, sbc); 
    }
    host->sw_timeout = 0;
out: 
    msdc_reset_crc_tune_counter(host,all_counter);
    
#ifdef TUNE_FLOW_TEST
    if (!is_card_sdio(host)) {
        msdc_reset_para(host);   
    }   
#endif

    /* ==== when request done, check if app_cmd ==== */
    if (mrq->cmd->opcode == MMC_APP_CMD) {
        host->app_cmd = 1;       
        host->app_cmd_arg = mrq->cmd->arg;  /* save the RCA */
    } else {
        host->app_cmd = 0;      
        //host->app_cmd_arg = 0;        
    }
        
    host->mrq = NULL; 

    //=== for sdio profile ===
    if (sdio_pro_enable) {  
        if (mrq->cmd->opcode == 52 || mrq->cmd->opcode == 53) {     
            //GPT_GetCounter64(&new_L32, &new_H32);
            ticks = msdc_time_calc(old_L32, old_H32, new_L32, new_H32);
            
            opcode = mrq->cmd->opcode;    
            if (mrq->cmd->data) {
                sizes = mrq->cmd->data->blocks * mrq->cmd->data->blksz;     
                bRx = mrq->cmd->data->flags & MMC_DATA_READ ? 1 : 0 ;
            } else {
                bRx = mrq->cmd->arg    & 0x80000000 ? 1 : 0;  
            }
            
            if (!mrq->cmd->error) {
                msdc_performance(opcode, sizes, bRx, ticks);
            }
        }    
    } 

    msdc_gate_clock(host, 1); // clear flag. 
    spin_unlock(&host->lock);

    mmc_request_done(mmc, mrq);
     
   return;
}

static void msdc_tune_async_request(struct mmc_host *mmc, struct mmc_request *mrq)
{   
    struct msdc_host *host = mmc_priv(mmc);
    struct mmc_command *cmd;    
    struct mmc_data *data;
    struct mmc_command *stop = NULL;
    struct mmc_command *sbc = NULL;
    int data_abort = 0;
    //int got_polarity = 0;
    //unsigned long flags;
    //msdc_reset_crc_tune_counter(host,all_counter);      
    if(host->mrq){
#ifdef CONFIG_MTK_AEE_FEATURE
        aee_kernel_warning("MSDC","MSDC request not clear.\n host attached<0x%.8x> current<0x%.8x>.\n",(int)host->mrq,(int)mrq);
#else
        WARN_ON(host->mrq);
#endif
        ERR_MSG("XXX host->mrq<0x%.8x> cmd<%d>arg<0x%x>", (int)host->mrq,host->mrq->cmd->opcode,host->mrq->cmd->arg); 
        if(host->mrq->data){
            ERR_MSG("XXX request data size<%d>",host->mrq->data->blocks * host->mrq->data->blksz); 
            ERR_MSG("XXX request attach to host force data timeout and retry"); 
            host->mrq->data->error = (unsigned int)-ETIMEDOUT;
        } else{
            ERR_MSG("XXX request attach to host force cmd timeout and retry");
            host->mrq->cmd->error = (unsigned int)-ETIMEDOUT;
        }
        ERR_MSG("XXX current request <0x%.8x> cmd<%d>arg<0x%x>",(int)mrq,mrq->cmd->opcode,mrq->cmd->arg);
        if(mrq->data)
            ERR_MSG("XXX current request data size<%d>",mrq->data->blocks * mrq->data->blksz);
    }            
      
    if (!is_card_present(host) || host->power_mode == MMC_POWER_OFF) {
        ERR_MSG("cmd<%d> arg<0x%x> card<%d> power<%d>", mrq->cmd->opcode,mrq->cmd->arg, is_card_present(host), host->power_mode);
        mrq->cmd->error = (unsigned int)-ENOMEDIUM;
        //mrq->done(mrq);         // call done directly.
        return;
    }

    cmd = mrq->cmd;      
    data = mrq->cmd->data;
    if (msdc_async_use_pio(mrq->data->host_cookie)){   
        return;
    }
    if (data) stop = data->stop;
#ifdef MTK_MSDC_USE_CMD23
    if (data) sbc = mrq->sbc;
#endif

#ifdef MTK_MSDC_USE_CMD23
    if(((sbc == NULL) || (sbc && sbc->error == 0)) && (cmd->error == 0) && (data && data->error == 0) && (!stop || stop->error == 0)){
#else
    if((cmd->error == 0) && (data && data->error == 0) && (!stop || stop->error == 0)){
#endif
        if(cmd->opcode == MMC_READ_SINGLE_BLOCK || cmd->opcode == MMC_READ_MULTIPLE_BLOCK)
            host->read_time_tune = 0;
        if(cmd->opcode == MMC_WRITE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK)
            host->write_time_tune = 0;
        host->rwcmd_time_tune = 0;
        host->power_cycle_enable = 1;             
        return;
    }
    /* start to process */
    spin_lock(&host->lock);  
       
    /*if(host->error & REQ_CMD_EIO)
        cmd->error = (unsigned int)-EIO;
    else if(host->error & REQ_CMD_TMO)
        cmd->error = (unsigned int)-ETIMEDOUT;
    */
    
    msdc_ungate_clock(host);  // set sw flag 
    host->tune = 1;
    host->mrq = mrq;                 
    do{
        msdc_dump_trans_error(host, cmd, data, stop, sbc);         // becasue ISR executing time will be monitor, try to dump the info here. 
        /*if((host->t_counter.time_cmd % 16 == 15) 
        || (host->t_counter.time_read % 16 == 15) 
            || (host->t_counter.time_write % 16 == 15))
        {
            spin_unlock(&host->lock); 
            msleep(150);
            ERR_MSG("sleep 150ms here!");   //sleep in tuning flow, to avoid printk watchdong timeout
            spin_lock(&host->lock);
            goto out;
        }*/

#ifdef MTK_MSDC_USE_CMD23
        if ((sbc != NULL) && (sbc->error == (unsigned int)-ETIMEDOUT)) {
            if (cmd->opcode == MMC_READ_MULTIPLE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK ){
                /* not tuning, go out directly */
                printk("===[%s:%d]==cmd23 timeout==\n", __func__, __LINE__);
                goto out;
            }
        }
#endif

#ifdef MTK_MSDC_USE_CMD23
        /* cmd->error also set when autocmd23 crc error */
        if ((cmd->error == (unsigned int)-EIO) || (stop && (stop->error == (unsigned int)-EIO)) || 
                (sbc && (sbc->error == (unsigned int)-EIO))) {
#else 
        if ((cmd->error == (unsigned int)-EIO) || (stop && (stop->error == (unsigned int)-EIO))) {
#endif
            if (msdc_tune_cmdrsp(host)){
                ERR_MSG("failed to updata cmd para");
                goto out;    
            }     
        }        

        if (data && (data->error == (unsigned int)-EIO)) {
            if (data->flags & MMC_DATA_READ) {  // read 
                if (msdc_tune_read(host)) {
                    ERR_MSG("failed to updata read para");   
                    goto out; 
                }  
            } else {
                if (msdc_tune_write(host)) {
                    ERR_MSG("failed to updata write para");
                    goto out;
                }  
            }             
        }

        // bring the card to "tran" state
        // tuning param done if cmd crc error 
        if (data) {
            if (msdc_abort_data(host)) {
                ERR_MSG("abort failed");
                data_abort = 1;
                if(host->hw->host_function == MSDC_SD){                  
                    if(host->card_inserted){
                        ERR_MSG("go to remove the bad card"); 
                        msdc_set_bad_card_and_remove(host);
                    }    
                    goto out;    
                }
            }            
        }

        // CMD TO -> not tuning. cmd->error also set when autocmd23 TO error 
        if (cmd->error == (unsigned int)-ETIMEDOUT) {
            if(cmd->opcode == MMC_READ_SINGLE_BLOCK || cmd->opcode == MMC_READ_MULTIPLE_BLOCK || cmd->opcode == MMC_WRITE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK ){
                if((host->sw_timeout) || (++(host->rwcmd_time_tune) > MSDC_MAX_TIMEOUT_RETRY)){
                    ERR_MSG("msdc%d exceed max r/w cmd timeout tune times(%d) or SW timeout(%d),Power cycle\n",host->id,host->rwcmd_time_tune,host->sw_timeout);
                    if(!(host->sd_30_busy) && msdc_power_tuning(host))
                        goto out;
                }
            }else {
                goto out;
            }
        } 

        // [ALPS114710] Patch for data timeout issue.
        if (data && (data->error == (unsigned int)-ETIMEDOUT)) {  
            if (data->flags & MMC_DATA_READ) {
                if( !(host->sw_timeout) && 
                        (host->hw->host_function == MSDC_SD) && 
                        (host->sclk > 100000000) && 
                        (host->read_timeout_uhs104 < MSDC_MAX_R_TIMEOUT_TUNE)){
                    if(host->t_counter.time_read)
                        host->t_counter.time_read--;
                    host->read_timeout_uhs104++;
                    msdc_tune_read(host);
                }
                else if((host->sw_timeout) || (host->read_timeout_uhs104 >= MSDC_MAX_R_TIMEOUT_TUNE) || (++(host->read_time_tune) > MSDC_MAX_TIMEOUT_RETRY)){
                    ERR_MSG("msdc%d exceed max read timeout retry times(%d) or SW timeout(%d) or read timeout tuning times(%d),Power cycle\n",host->id,host->read_time_tune,host->sw_timeout,host->read_timeout_uhs104);
                    if(!(host->sd_30_busy) && msdc_power_tuning(host))
                        goto out;
                }
            }
            else if(data->flags & MMC_DATA_WRITE){
                if( !(host->sw_timeout) && 
                        (host->hw->host_function == MSDC_SD) && 
                        (host->sclk > 100000000) && 
                        (host->write_timeout_uhs104 < MSDC_MAX_W_TIMEOUT_TUNE)){
                    if(host->t_counter.time_write)
                        host->t_counter.time_write--;
                    host->write_timeout_uhs104++;
                    msdc_tune_write(host);
                } else if ( !(host->sw_timeout) && 
                        (host->hw->host_function == MSDC_EMMC) && 
                        (host->write_timeout_emmc < MSDC_MAX_W_TIMEOUT_TUNE_EMMC)){
                    if(host->t_counter.time_write)
                        host->t_counter.time_write--;
                    host->write_timeout_emmc++;
                    msdc_tune_write(host);
                } else if((host->hw->host_function == MSDC_SD) && ((host->sw_timeout) || (host->write_timeout_uhs104 >= MSDC_MAX_W_TIMEOUT_TUNE) || (++(host->write_time_tune) > MSDC_MAX_TIMEOUT_RETRY))){
                    ERR_MSG("msdc%d exceed max write timeout retry times(%d) or SW timeout(%d) or write timeout tuning time(%d),Power cycle\n",host->id,host->write_time_tune,host->sw_timeout,host->write_timeout_uhs104);
                    if(!(host->sd_30_busy) && msdc_power_tuning(host))
                        goto out;
                } else if((host->hw->host_function == MSDC_EMMC) && ((host->sw_timeout) || (++(host->write_time_tune) > MSDC_MAX_TIMEOUT_RETRY_EMMC))){
                    ERR_MSG("msdc%d exceed max write timeout retry times(%d) or SW timeout(%d) or write timeout tuning time(%d),Power cycle\n",host->id,host->write_time_tune,host->sw_timeout,host->write_timeout_emmc);
                        host->write_timeout_emmc = 0;
                        goto out;
                }
            }
        }        

        // clear the error condition.
        cmd->error = 0; 
        if (data) data->error = 0;
        if (stop) stop->error = 0; 

#ifdef MTK_MSDC_USE_CMD23
        if (sbc) sbc->error = 0; 
#endif

        host->sw_timeout = 0;
        if (!is_card_present(host)) {
            goto out;
        }
    }while(msdc_tune_rw_request(mmc,mrq));

    if((host->rwcmd_time_tune)&&(cmd->opcode == MMC_READ_SINGLE_BLOCK || cmd->opcode == MMC_READ_MULTIPLE_BLOCK || cmd->opcode == MMC_WRITE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK)){
        host->rwcmd_time_tune = 0;
        ERR_MSG("RW cmd recover");
        msdc_dump_trans_error(host, cmd, data, stop, sbc); 
    }
    if((host->read_time_tune)&&(cmd->opcode == MMC_READ_SINGLE_BLOCK || cmd->opcode == MMC_READ_MULTIPLE_BLOCK) ){
        host->read_time_tune = 0;
        ERR_MSG("Read recover");
        msdc_dump_trans_error(host, cmd, data, stop, sbc); 
    }
    if((host->write_time_tune) && (cmd->opcode == MMC_WRITE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK)){
        host->write_time_tune = 0;
        ERR_MSG("Write recover");
        msdc_dump_trans_error(host, cmd, data, stop, sbc); 
    }
    host->power_cycle_enable = 1;             
    host->sw_timeout = 0;

out:
    if(host->sclk <= 50000000 && (!host->ddr))
        host->sd_30_busy = 0;  
    msdc_reset_crc_tune_counter(host,all_counter);        
    host->mrq = NULL; 
    msdc_gate_clock(host, 1); // clear flag. 
    host->tune = 0;
    spin_unlock(&host->lock);

    //mmc_request_done(mmc, mrq);
    return;
}
static void msdc_ops_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
    struct mmc_data *data;
    int host_cookie = 0;
    BUG_ON(mmc == NULL);
    BUG_ON(mrq == NULL);
    data = mrq->data;
    if (data){
        host_cookie = data->host_cookie;
    }
    //Asyn only support  DMA and asyc CMD flow     
    if (msdc_async_use_dma(host_cookie))
    {
        msdc_do_request_async(mmc,mrq);
    }
    else
    {
        msdc_ops_request_legacy(mmc,mrq);
    }
    return;
}


/* called by ops.set_ios */
static void msdc_set_buswidth(struct msdc_host *host, u32 width)
{
    u32 base = host->base;
    u32 val = sdr_read32(SDC_CFG);
    
    val &= ~SDC_CFG_BUSWIDTH;
    
    switch (width) {
    default:
    case MMC_BUS_WIDTH_1:
        width = 1;
        val |= (MSDC_BUS_1BITS << 16);
        break;
    case MMC_BUS_WIDTH_4:
        val |= (MSDC_BUS_4BITS << 16);
        break;
    case MMC_BUS_WIDTH_8:
        val |= (MSDC_BUS_8BITS << 16);
        break;
    }
    
    sdr_write32(SDC_CFG, val);

    N_MSG(CFG, "Bus Width = %d", width);
}



/* ops.set_ios */

static void msdc_ops_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
    struct msdc_host *host = mmc_priv(mmc);
    struct msdc_hw *hw=host->hw;
    u32 base = host->base;
    u32 ddr = 0;
    //unsigned long flags;
    u32 cur_rxdly0, cur_rxdly1;

#ifdef MT_SD_DEBUG
    static char *vdd[] = {
        "1.50v", "1.55v", "1.60v", "1.65v", "1.70v", "1.80v", "1.90v",
        "2.00v", "2.10v", "2.20v", "2.30v", "2.40v", "2.50v", "2.60v",
        "2.70v", "2.80v", "2.90v", "3.00v", "3.10v", "3.20v", "3.30v",
        "3.40v", "3.50v", "3.60v"        
    };
    static char *power_mode[] = {
        "OFF", "UP", "ON"
    };
    static char *bus_mode[] = {
        "UNKNOWN", "OPENDRAIN", "PUSHPULL"
    };
    static char *timing[] = {
        "LEGACY", "MMC_HS", "SD_HS"
    };

    N_MSG(CFG, "SET_IOS: CLK(%dkHz), BUS(%s), BW(%u), PWR(%s), VDD(%s), TIMING(%s)",
        ios->clock / 1000, bus_mode[ios->bus_mode],
        (ios->bus_width == MMC_BUS_WIDTH_4) ? 4 : 1,
        power_mode[ios->power_mode], vdd[ios->vdd], timing[ios->timing]);
#endif

    spin_lock(&host->lock);
    if(ios->timing == MMC_TIMING_UHS_DDR50)
        ddr = 1;
    msdc_ungate_clock(host);
    
    msdc_set_buswidth(host, ios->bus_width);     
    
    /* Power control ??? */
    switch (ios->power_mode) {
        case MMC_POWER_OFF:
        case MMC_POWER_UP:
#ifndef FPGA_PLATFORM
            msdc_set_driving(host,hw,0);
#endif
            spin_unlock(&host->lock);
            msdc_set_power_mode(host, ios->power_mode);
            spin_lock(&host->lock);
            break;
        case MMC_POWER_ON:
            host->power_mode = MMC_POWER_ON;
            break;
        default:
            break;
    }
    if(msdc_host_mode[host->id] != mmc->caps || (msdc_host_mode2[host->id] != mmc->caps2)) {
        mmc->caps = msdc_host_mode[host->id];
        mmc->caps2 = msdc_host_mode2[host->id];
#ifdef MTK_EMMC_SUPPORT
        if (1 == g_emmc_mode_switch){
            if(!(mmc->caps2 & MMC_CAP2_HS200_1_8V_SDR)){
                host->mmc->f_max = 50000000;
                if(mmc->card && mmc->card->ext_csd.hs_max_dtr == 200000000){
                    mmc->card->state &= ~MMC_STATE_HIGHSPEED_200;
                    if((mmc->card->ext_csd.raw_card_type & EXT_CSD_CARD_TYPE_MASK) & EXT_CSD_CARD_TYPE_DDR_1_8V )
                        mmc->card->ext_csd.card_type |= EXT_CSD_CARD_TYPE_DDR_1_8V;
                }
            } else {
                host->mmc->f_max = HOST_MAX_MCLK;
                if(mmc->card && mmc->card->ext_csd.hs_max_dtr == 200000000){
                    mmc->card->ext_csd.card_type &= ~EXT_CSD_CARD_TYPE_DDR_1_8V;
                }
            }
        }
#endif

        sdr_write32(MSDC_PAD_TUNE,   0x00000000);
        sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATWRDLY, host->hw->datwrddly);        
        sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRRDLY, host->hw->cmdrrddly);
        sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, host->hw->cmdrddly);
        sdr_write32(MSDC_IOCON,      0x00000000);
#if 1
        sdr_set_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);
        cur_rxdly0 = ((host->hw->dat0rddly & 0x1F) << 24) | ((host->hw->dat1rddly & 0x1F) << 16) |
            ((host->hw->dat2rddly & 0x1F) << 8)  | ((host->hw->dat3rddly & 0x1F) << 0);
        cur_rxdly1 = ((host->hw->dat4rddly & 0x1F) << 24) | ((host->hw->dat5rddly & 0x1F) << 16) |
            ((host->hw->dat6rddly & 0x1F) << 8)  | ((host->hw->dat7rddly & 0x1F) << 0);
        sdr_write32(MSDC_DAT_RDDLY0, cur_rxdly0);
        sdr_write32(MSDC_DAT_RDDLY1, cur_rxdly1); 
#else
        sdr_write32(MSDC_DAT_RDDLY0, 0x00000000);
        sdr_write32(MSDC_DAT_RDDLY1, 0x00000000);
#endif
        //sdr_write32(MSDC_PATCH_BIT0, 0x403C004F); /* bit0 modified: Rx Data Clock Source: 1 -> 2.0*/
        if ((host->hw->host_function == MSDC_EMMC) || (host->hw->host_function == MSDC_SD)){
            sdr_write32(MSDC_PATCH_BIT1, 0xFFFF00C9);
        } else {
            sdr_write32(MSDC_PATCH_BIT1, 0xFFFF0009);    
        }
        //sdr_set_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_WRDAT_CRCS, 1); 
        //sdr_set_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_CMD_RSP,    1);


        if (!(is_card_sdio(host)|| (host->hw->flags & MSDC_SDIO_IRQ))) {  /* internal clock: latch read data, not apply to sdio */           
            //sdr_set_bits(MSDC_PATCH_BIT0, MSDC_PATCH_BIT_CKGEN_CK);
            host->hw->cmd_edge   = 0; // tuning from 0
            host->hw->rdata_edge = 0;
            host->hw->wdata_edge = 0;
        }else if (hw->flags & MSDC_INTERNAL_CLK) {
            //sdr_set_bits(MSDC_PATCH_BIT0, MSDC_PATCH_BIT_CKGEN_CK);
        }
    }
    if(msdc_clock_src[host->id] != hw->clk_src){
        hw->clk_src = msdc_clock_src[host->id];
        msdc_select_clksrc(host, hw->clk_src);
    }

    if (host->mclk != ios->clock || host->ddr != ddr) { /* not change when clock Freq. not changed ddr need set clock*/                
        if(ios->clock >= 25000000) {
            if(ios->clock > 100000000){
                hw->clk_drv_sd_18 += 1;
#ifdef FPGA_PLATFORM
                sdr_set_field(MSDC_PAD_CTL0, MSDC_PAD_CTL0_CLKDRVN, hw->clk_drv_sd_18);
                sdr_set_field(MSDC_PAD_CTL0, MSDC_PAD_CTL0_CLKDRVP, hw->clk_drv_sd_18);
                sdr_set_field(MSDC_PAD_CTL1, MSDC_PAD_CTL1_CMDDRVN, hw->cmd_drv_sd_18);
                sdr_set_field(MSDC_PAD_CTL1, MSDC_PAD_CTL1_CMDDRVP, hw->cmd_drv_sd_18);
                sdr_set_field(MSDC_PAD_CTL2, MSDC_PAD_CTL2_DATDRVN, hw->dat_drv_sd_18);
                sdr_set_field(MSDC_PAD_CTL2, MSDC_PAD_CTL2_DATDRVP, hw->dat_drv_sd_18);
#else
                msdc_set_driving(host,hw,1);
#endif

                hw->clk_drv_sd_18 -= 1;
            }

            if( is_card_sdio(host) && sdio_enable_tune) {
                //Only Enable when ETT is running
                u32 cur_rxdly0;//,cur_rxdly1;

                sdio_tune_flag = 0;                             
                sdr_set_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);            
                //Latch edge
                host->hw->cmd_edge = sdio_iocon_rspl;
                host->hw->rdata_edge = sdio_iocon_dspl; 
                host->hw->wdata_edge = sdio_iocon_w_dspl; 

                //CMD and DATA delay
                sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, sdio_pad_tune_rdly); 
                sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRRDLY, sdio_pad_tune_rrdly); 
                sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATWRDLY, sdio_pad_tune_wrdly);       
                sdr_set_field(MSDC_PAD_TUNE,MSDC_PAD_TUNE_DATRRDLY, sdio_dat_rd_dly0_0);        
                cur_rxdly0 = (sdio_dat_rd_dly0_0 << 24) | (sdio_dat_rd_dly0_1 << 16) |    
                    (sdio_dat_rd_dly0_2 << 8) | (sdio_dat_rd_dly0_3 << 0);

                host->saved_para.pad_tune = sdr_read32(MSDC_PAD_TUNE);
                host->saved_para.ddly0 = cur_rxdly0;                
            }

            //INIT_MSG("SD latch rdata<%d> wdatea<%d> cmd<%d>", hw->rdata_edge,hw->wdata_edge, hw->cmd_edge);
            sdr_set_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, hw->ddlsel); 
            sdr_set_field(MSDC_IOCON, MSDC_IOCON_RSPL, hw->cmd_edge); 
            sdr_set_field(MSDC_IOCON, MSDC_IOCON_DSPL, hw->rdata_edge);
            sdr_set_field(MSDC_IOCON, MSDC_IOCON_W_DSPL, hw->wdata_edge);
            sdr_write32(MSDC_PAD_TUNE,host->saved_para.pad_tune);
            sdr_write32(MSDC_DAT_RDDLY0,host->saved_para.ddly0);
            sdr_write32(MSDC_DAT_RDDLY1,host->saved_para.ddly1);
            sdr_set_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_WRDAT_CRCS, host->saved_para.wrdat_crc_ta_cntr); 
            sdr_set_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_CMD_RSP,    host->saved_para.cmd_resp_ta_cntr);
            if ((host->hw->host_function == MSDC_EMMC) || (host->hw->host_function == MSDC_SD)){
                sdr_set_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_ECO_1, host->saved_para.write_timeout_eco1);
                sdr_set_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_ECO_2, host->saved_para.write_timeout_eco2); 
            }
#ifdef MTK_EMMC_ETT_TO_DRIVER
            if (host->hw->host_function == MSDC_EMMC) {
                //INIT_MSG("m_id     <0x%x>", m_id);
                //INIT_MSG("pro_name <%s>",  pro_name);

                msdc_ett_offline_to_driver(host);
            }
#endif    
        }

        if (ios->clock == 0) {
            // [MT6290 SDIO] Temp add !is_card_sdio to avoid edge setting in struct msdc_hw is overwrited.
            if(ios->power_mode == MMC_POWER_OFF && !is_card_sdio(host)){
                sdr_get_field(MSDC_IOCON, MSDC_IOCON_RSPL, hw->cmd_edge);   // save the para
                sdr_get_field(MSDC_IOCON, MSDC_IOCON_DSPL, hw->rdata_edge); 
                sdr_get_field(MSDC_IOCON, MSDC_IOCON_W_DSPL, hw->wdata_edge);
                host->saved_para.pad_tune = sdr_read32(MSDC_PAD_TUNE);
                host->saved_para.ddly0 = sdr_read32(MSDC_DAT_RDDLY0);
                host->saved_para.ddly1 = sdr_read32(MSDC_DAT_RDDLY1);
                sdr_get_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_CMD_RSP,    host->saved_para.cmd_resp_ta_cntr);
                sdr_get_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_WRDAT_CRCS, host->saved_para.wrdat_crc_ta_cntr); 
                
                /* for write: 3T need wait before host check busy after crc status 
                 * for write: host check timeout change to 16T */
                sdr_get_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_ECO_1, host->saved_para.write_timeout_eco1);
                sdr_get_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_ECO_2, host->saved_para.write_timeout_eco2); 

                //INIT_MSG("save latch rdata<%d> wdata<%d> cmd<%d>", hw->rdata_edge,hw->wdata_edge,hw->cmd_edge); 
            }
            /* reset to default value */                
            sdr_write32(MSDC_IOCON,      0x00000000);           
            sdr_write32(MSDC_DAT_RDDLY0, 0x00000000);
            sdr_write32(MSDC_DAT_RDDLY1, 0x00000000);            
            sdr_write32(MSDC_PAD_TUNE,   0x00000000);
            sdr_set_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_CMD_RSP,1);
            sdr_set_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_WRDAT_CRCS,1);
            if ((host->hw->host_function == MSDC_EMMC) || (host->hw->host_function == MSDC_SD)){
                sdr_set_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_ECO_1, 1);
                sdr_set_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_ECO_2, 1); 
            }
        }
        msdc_set_mclk(host, ddr, ios->clock);        
    }

#if 0  // PM Resume -> set 0 -> 260KHz     
    if (ios->clock == 0) { // only gate clock when set 0Hz   
        msdc_gate_clock(host, 1);
    }
#endif       
    msdc_gate_clock(host, 1);
    spin_unlock(&host->lock);       
}

/* ops.get_ro */
static int msdc_ops_get_ro(struct mmc_host *mmc)
{
    struct msdc_host *host = mmc_priv(mmc);
    u32 base = host->base;
    unsigned long flags;
    int ro = 0;

    spin_lock_irqsave(&host->lock, flags);
    msdc_ungate_clock(host);
    if (host->hw->flags & MSDC_WP_PIN_EN) { /* set for card */
        ro = (sdr_read32(MSDC_PS) >> 31);
    }
    msdc_gate_clock(host, 1);
    spin_unlock_irqrestore(&host->lock, flags);       
    return ro;
}

/* ops.get_cd */
static int msdc_ops_get_cd(struct mmc_host *mmc)
{
    struct msdc_host *host = mmc_priv(mmc);
    u32 base;
    unsigned long flags;
    //int present = 1;

    base = host->base; 
    spin_lock_irqsave(&host->lock, flags);
    
    /* for sdio, depends on USER_RESUME */
    if (is_card_sdio(host)) { 
        if (!(host->hw->flags & MSDC_SDIO_IRQ))
        {
            host->card_inserted = (host->pm_state.event == PM_EVENT_USER_RESUME) ? 1 : 0; 
            //INIT_MSG("sdio ops_get_cd<%d>", host->card_inserted);
            goto end;        
        }
    }

    /* for emmc, MSDC_REMOVABLE not set, always return 1 */
    if (!(host->hw->flags & MSDC_REMOVABLE)) {
        host->card_inserted = 1;       
        goto end;
    }

    //msdc_ungate_clock(host);
//#if 1
    if (host->hw->flags & MSDC_CD_PIN_EN) { /* for card, MSDC_CD_PIN_EN set*/
        if(host->hw->cd_level)
            host->card_inserted = (host->sd_cd_polarity == 0) ? 1 : 0; 
        else
            host->card_inserted = (host->sd_cd_polarity == 0) ? 0 : 1; 
    } else {
        host->card_inserted = 1; /* TODO? Check DAT3 pins for card detection */
    }
//#endif
    //host->card_inserted = 1;
#if 0
    if (host->card_inserted == 0) {    
        msdc_gate_clock(host, 0);
    }else {
        msdc_gate_clock(host, 1);
    }
#endif
    if(host->hw->host_function == MSDC_SD && host->block_bad_card)
        host->card_inserted = 0;
    INIT_MSG("Card insert<%d> Block bad card<%d>", host->card_inserted,host->block_bad_card);
end:
    spin_unlock_irqrestore(&host->lock, flags);    
    return host->card_inserted;
}

/* ops.enable_sdio_irq */
static void msdc_ops_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
    struct msdc_host *host = mmc_priv(mmc);
    struct msdc_hw *hw = host->hw;
    u32 base = host->base;
    u32 tmp;
    unsigned long     flags_lock = 0;
if (host->hw->flags & MSDC_SDIO_IRQ)
    spin_lock_irqsave(&SDIO_IRQ_lock, flags_lock);

    if (hw->flags & MSDC_EXT_SDIO_IRQ) { /* yes for sdio */
        if (enable) {
            hw->enable_sdio_eirq();  /* combo_sdio_enable_eirq */
        } else {
            hw->disable_sdio_eirq(); /* combo_sdio_disable_eirq */
        }
    } else { 
       //ERR_MSG("XXX ");  /* so never enter here */
#if (MSDC_DATA1_INT == 1)
       if (host->hw->flags & MSDC_SDIO_IRQ)
       {
           int_sdio_irq_enable = enable;
           if (!u_sdio_irq_counter)
           {
               printk("msdc2 u_sdio_irq_counter=1 \n");  
           }
                    
           if (u_sdio_irq_counter < 0xFFFF)
               u_sdio_irq_counter = u_sdio_irq_counter + 1;
           else
               u_sdio_irq_counter = 1;
                
           if (u_sdio_irq_counter <7)
               printk("msdc2 sdio_irq enable: %d \n",int_sdio_irq_enable);

           //ERR_MSG("Ahsin int_sdio_irq_enable=%d  u_sdio_irq_counter=%d",int_sdio_irq_enable,u_sdio_irq_counter);
       }
#endif
       tmp = sdr_read32(SDC_CFG);
       /* FIXME. Need to interrupt gap detection */
       if (enable) {
           tmp |= (SDC_CFG_SDIOIDE | SDC_CFG_SDIOINTWKUP);           
#if (MSDC_DATA1_INT == 1)
           sdr_set_bits(MSDC_INTEN, MSDC_INT_SDIOIRQ);           
#endif
       } else {
           //tmp &= ~(SDC_CFG_SDIOIDE | SDC_CFG_SDIOINTWKUP);
#if (MSDC_DATA1_INT == 1)
           sdr_clr_bits(MSDC_INTEN, MSDC_INT_SDIOIRQ);
#endif
       }
       sdr_write32(SDC_CFG, tmp);      
    }
if (host->hw->flags & MSDC_SDIO_IRQ)
    spin_unlock_irqrestore(&SDIO_IRQ_lock, flags_lock);
}

static int msdc_ops_switch_volt(struct mmc_host *mmc, struct mmc_ios *ios)
{
    struct msdc_host *host = mmc_priv(mmc);
    u32 base = host->base;
    int err = 0;
    u32 timeout = 100;
    u32 retry = 10;
    u32 status;
    u32 sclk = host->sclk;

    if (host->hw->host_function == MSDC_EMMC)
        return 0;

    if(ios->signal_voltage != MMC_SIGNAL_VOLTAGE_330){
        /* make sure SDC is not busy (TBC) */
        //WAIT_COND(!SDC_IS_BUSY(), timeout, timeout);
        err = (unsigned int)-EIO;
        msdc_retry(sdc_is_busy(), retry, timeout,host->id);
        if (timeout == 0 && retry == 0) {
            err = (unsigned int)-ETIMEDOUT;
            goto out;
        }

        /* pull up disabled in CMD and DAT[3:0] to allow card drives them to low */
        /* check if CMD/DATA lines both 0 */
        if ((sdr_read32(MSDC_PS) & ((1 << 24) | (0xF << 16))) == 0) {

            /* pull up disabled in CMD and DAT[3:0] */
            msdc_pin_config(host, MSDC_PIN_PULL_NONE);

            /* change signal from 3.3v to 1.8v for FPGA this can not work*/
            if(ios->signal_voltage == MMC_SIGNAL_VOLTAGE_180){                
#ifdef FPGA_PLATFORM
                hwPowerSwitch_fpga();
#else
                if(host->power_switch)
                    host->power_switch(host,1);
                else
                    ERR_MSG("No power switch callback. Please check host_function<0x%lx>",host->hw->host_function);
#endif
            }
            /* wait at least 5ms for 1.8v signal switching in card */
            mdelay(10);

            /* config clock to 10~12MHz mode for volt switch detection by host. */

            msdc_set_mclk(host, 0, 12000000);/*For FPGA 13MHz clock,this not work*/

            /* pull up enabled in CMD and DAT[3:0] */
            msdc_pin_config(host, MSDC_PIN_PULL_UP);
            mdelay(105);

            /* start to detect volt change by providing 1.8v signal to card */
            sdr_set_bits(MSDC_CFG, MSDC_CFG_BV18SDT);

            /* wait at max. 1ms */
            mdelay(1);
            //ERR_MSG("before read status\n");

            while ((status = sdr_read32(MSDC_CFG)) & MSDC_CFG_BV18SDT);

            if (status & MSDC_CFG_BV18PSS)
                err = 0;
            //ERR_MSG("msdc V1800 status (0x%x),err(%d)\n",status,err);
            /* config clock back to init clk freq. */
            msdc_set_mclk(host, 0, sclk);
        }   
    }
out:
        
    return err;
}

/*                          mmc_pre_req()                                                                                     __mmc_start_req()
 *                              | |                                                                                                  | |
 * async way: mmc_start_req() ->  __mmc_start_req() -> mmc_start_request() -> request() -> mmc_wait_for_req_done() -> msdc_ops_stop()  -> mmc_post_req()
 * legacy way: mmc_wait_for_req() -> __mmc_start_req -> mmc_start_request() -> request() 
 * msdc_send_stop() just for async way. for when to trigger stop cmd(arg=0):
 * 1 aysnc way but used pio mode will call msdc_ops_request_legacy(), and pio mode will disable autocmd12, so cmd12 will send in msdc_do_request() 
 * 2 aysnc way with non-cmd23 mode: if host not used autocmd12, sw need send cmd12 after mrq->completion(polling under mmc_core.c) is done 
 * 3 aysnc way with cmd23 mode: no need to send cmd12 here 
 * sd card will not enable cmd23 */
static void msdc_ops_stop(struct mmc_host *mmc,struct mmc_request *mrq)
{
    //struct mmc_command stop = {0};    
    //struct mmc_request mrq_stop = {0};
    struct msdc_host *host = mmc_priv(mmc);
    u32 err = -1;
    //u32 bus_mode = 0;
    //u32 base = host->base;
    if (msdc_async_use_pio(mrq->data->host_cookie))
    {
        return ;
    }
    if(host->hw->host_function != MSDC_SDIO){
        if(!mrq->stop)
            return;
      
        //sdr_get_field(SDC_CFG ,SDC_CFG_BUSWIDTH ,bus_mode);
#ifndef MTK_MSDC_USE_CMD23    
        if((host->autocmd & MSDC_AUTOCMD12))
            return;
#else
        if (host->hw->host_function == MSDC_EMMC) { 
            /* if transfer error occur, cmd12 will send under msdc_abort_data() */
            if (mrq->sbc)
                return;
        } else {
            if ((host->autocmd & MSDC_AUTOCMD12))
                return;
        }
#endif /* end of MTK_MSDC_USE_CMD23 */

        N_MSG(OPS, "MSDC Stop for non-autocmd12 host->error(%d)host->autocmd(%d)",host->error,host->autocmd);
        err = msdc_do_command(host, mrq->stop, 0, CMD_TIMEOUT);
        if(err){
            if (mrq->stop->error == (unsigned int)-EIO) host->error |= REQ_STOP_EIO; 
            if (mrq->stop->error == (unsigned int)-ETIMEDOUT) host->error |= REQ_STOP_TMO; 
        }
    }
}

extern u32 __mmc_sd_num_wr_blocks(struct mmc_card* card);
static bool msdc_check_written_data(struct mmc_host *mmc,struct mmc_request *mrq)
{
    u32 result = 0;
    struct msdc_host *host = mmc_priv(mmc);
    struct mmc_card *card;
    if (msdc_async_use_pio(mrq->data->host_cookie))
    {
        return 0;
    }
    if (!is_card_present(host) || host->power_mode == MMC_POWER_OFF) {
        ERR_MSG("cmd<%d> arg<0x%x> card<%d> power<%d>", mrq->cmd->opcode,mrq->cmd->arg,is_card_present(host), host->power_mode);
        mrq->cmd->error = (unsigned int)-ENOMEDIUM;
        return 0;
    }
    if(mmc->card)
        card = mmc->card;
    else
        return 0;
    if((host->hw->host_function == MSDC_SD)
        && (host->sclk > 100000000)
        && mmc_card_sd(card)
        && (mrq->data) 
        && (mrq->data->flags & MMC_DATA_WRITE)
        && (host->error == 0)){

        msdc_ungate_clock(host); 
        
        spin_lock(&host->lock);
        if(msdc_polling_idle(host)){
            spin_unlock(&host->lock);
            
            msdc_gate_clock(host, 1);
            return 0;
        }
        spin_unlock(&host->lock);
        result = __mmc_sd_num_wr_blocks(card);
        if((result != mrq->data->blocks) && (is_card_present(host)) && (host->power_mode == MMC_POWER_ON)){
            mrq->data->error = (unsigned int)-EIO;
            host->error |= REQ_DAT_ERR;
            ERR_MSG("written data<%d> blocks isn't equal to request data blocks<%d>",result,mrq->data->blocks);
            
            msdc_gate_clock(host, 1);
            return 1;
        }

        msdc_gate_clock(host, 1);
    }
    return 0;        
}
static void msdc_dma_error_reset(struct mmc_host *mmc)
{
    struct msdc_host *host = mmc_priv(mmc);
    u32 base = host->base;
    struct mmc_data *data = host->data;
    if(data && host->dma_xfer && (msdc_async_use_dma(data->host_cookie)) && (host->tune == 0))
    {
        host->sw_timeout++;
        host->error |= REQ_DAT_ERR;
        msdc_dump_info(host->id);
        msdc_reset_hw(host->id); 
        msdc_dma_stop(host);
        msdc_clr_fifo(host->id); 
        msdc_clr_int();
        msdc_dma_clear(host);
        msdc_gate_clock(host, 1);
    }
}

static struct mmc_host_ops mt_msdc_ops = {
    .post_req                      = msdc_post_req,
    .pre_req                       = msdc_pre_req,
    .request                       = msdc_ops_request,
    .tuning                        = msdc_tune_async_request,
    .set_ios                       = msdc_ops_set_ios,
    .get_ro                        = msdc_ops_get_ro,
    .get_cd                        = msdc_ops_get_cd,
    .enable_sdio_irq               = msdc_ops_enable_sdio_irq,
    .start_signal_voltage_switch   = msdc_ops_switch_volt,
    .send_stop                     = msdc_ops_stop,
    .dma_error_reset               = msdc_dma_error_reset,
    .check_written_data            = msdc_check_written_data,
};

/*--------------------------------------------------------------------------*/
/* interrupt handler                 */
/*--------------------------------------------------------------------------*/
//static __tcmfunc irqreturn_t msdc_irq(int irq, void *dev_id)
#ifndef FPGA_PLATFORM
#ifdef CUST_EINT_MSDC1_INS_NUM
static void msdc1_eint_handler(void)
{
    struct msdc_host *host = mtk_msdc_host[1];
    int got_bad_card = 0;
    unsigned long flags;
    spin_lock_irqsave(&host->remove_bad_card,flags);
    if(host->hw->cd_level ^ host->sd_cd_polarity){
        got_bad_card = host->block_bad_card;
        host->card_inserted = 0;     
        if(host->mmc && host->mmc->card)
            mmc_card_set_removed(host->mmc->card);
    }
    host->sd_cd_polarity = (~(host->sd_cd_polarity))&0x1;
    spin_unlock_irqrestore(&host->remove_bad_card,flags);
    host->block_bad_card = 0;

    if (0 == host->sd_cd_polarity){
        mt_eint_set_polarity(CUST_EINT_MSDC1_INS_NUM, MT_POLARITY_LOW);
    } else {
        mt_eint_set_polarity(CUST_EINT_MSDC1_INS_NUM, MT_POLARITY_HIGH);
    }
     
    //mt_eint_set_polarity(EINT_MSDC1_INS_NUM, host->sd_cd_polarity);
    if(got_bad_card == 0)
        tasklet_hi_schedule(&host->card_tasklet);
    ERR_MSG("SD card %s(%x:%x)",(host->hw->cd_level ^ host->sd_cd_polarity) ? "insert":"remove", host->hw->cd_level, host->sd_cd_polarity);
}
#endif

#endif
static irqreturn_t msdc_irq(int irq, void *dev_id)
{
    struct msdc_host  *host = (struct msdc_host *)dev_id;
    struct mmc_data   *data = host->data;
    struct mmc_command *cmd = host->cmd;
    struct mmc_command *stop = NULL;
    struct mmc_request *mrq = NULL;
    u32 base = host->base;
        
    u32 cmdsts = MSDC_INT_RSPCRCERR  | MSDC_INT_CMDTMO  | MSDC_INT_CMDRDY  |
                 MSDC_INT_ACMDCRCERR | MSDC_INT_ACMDTMO | MSDC_INT_ACMDRDY |
                 MSDC_INT_ACMD19_DONE;                 
    u32 datsts = MSDC_INT_DATCRCERR  |MSDC_INT_DATTMO;
    u32 intsts, inten;

    if (0 == host->core_clkon) {
#ifndef FPGA_PLATFORM
        enable_clock(MT_CG_PERI_MSDC30_0 + host->id, "SD"); 
#endif
        host->core_clkon = 1;       
        sdr_set_field(MSDC_CFG, MSDC_CFG_MODE, MSDC_SDMMC);
        intsts = sdr_read32(MSDC_INT);
#if 0
        if (sdr_read32(MSDC_ECO_VER) >= 4) {
            sdr_set_field(MSDC_CFG, MSDC_CFG_MODE, MSDC_SDMMC);  /* E2 */    
            intsts = sdr_read32(MSDC_INT);
            sdr_set_field(MSDC_CLKSRC_REG, MSDC1_IRQ_SEL, 0);
        } else {
            intsts = sdr_read32(MSDC_INT);        
        }
#endif
    } else {
        intsts = sdr_read32(MSDC_INT);
    }

    latest_int_status[host->id] = intsts;
    inten  = sdr_read32(MSDC_INTEN);
#if (MSDC_DATA1_INT == 1)    
     if (host->hw->flags & MSDC_SDIO_IRQ)
     {
         intsts &= inten;            
     }
     else
#endif         
     {
          inten &= intsts; 
     }

    sdr_write32(MSDC_INT, intsts);  /* clear interrupts */
    
    
    
    /* MSG will cause fatal error */
#if 0    
    /* card change interrupt */
    if (intsts & MSDC_INT_CDSC){
        IRQ_MSG("MSDC_INT_CDSC irq<0x%.8x>", intsts); 
        tasklet_hi_schedule(&host->card_tasklet);
        /* tuning when plug card ? */
    }
#endif
    /* sdio interrupt */
    if (intsts & MSDC_INT_SDIOIRQ){
        IRQ_MSG("XXX MSDC_INT_SDIOIRQ");  /* seems not sdio irq */
#if (MSDC_DATA1_INT == 1)
        if (host->hw->flags & MSDC_SDIO_IRQ)
        {
            if (!u_msdc_irq_counter)
            {
                printk("msdc2 u_msdc_irq_counter=1 \n");
            }
            if (u_msdc_irq_counter <0xFFFF)
                u_msdc_irq_counter = u_msdc_irq_counter + 1;
            else
                u_msdc_irq_counter = 1;
                            
            //if (u_msdc_irq_counter < 3)
                //printk("msdc2 u_msdc_irq_counter=%d SDC_CFG=%x MSDC_INTEN=%x MSDC_INT=%x \n",u_msdc_irq_counter,sdr_read32(SDC_CFG),sdr_read32(MSDC_INTEN),sdr_read32(MSDC_INT));
            //ERR_MSG("Ahsin u_msdc_irq_counter=%d SDC_CFG=%x MSDC_INTEN=%x MSDC_INT=%x MSDC_PATCH_BIT0=%x",u_msdc_irq_counter,sdr_read32(SDC_CFG),sdr_read32(MSDC_INTEN),sdr_read32(MSDC_INT),sdr_read32(MSDC_PATCH_BIT0));
                        
            mmc_signal_sdio_irq(host->mmc);
        }
#endif                    
    }

    /* transfer complete interrupt */
    if (data != NULL) {
        stop = data->stop;
#if (MSDC_DATA1_INT == 1)        
        if (host->hw->flags & MSDC_SDIO_IRQ)
        {
            if (intsts & MSDC_INT_XFER_COMPL) {           
                data->bytes_xfered = host->dma.xfersz;
                if((data->host_cookie) && (host->tune == 0)){
                    msdc_dma_stop(host);
                    mrq = host->mrq;
                    msdc_dma_clear(host);
                    if(mrq->done)
                        mrq->done(mrq);
                    msdc_gate_clock(host, 1);
                    host->error &= ~REQ_DAT_ERR;
                } else {
                    complete(&host->xfer_done); 
                }          
            } 
        }else
#endif        
        {
            if (inten & MSDC_INT_XFER_COMPL) {           
                data->bytes_xfered = host->dma.xfersz;
                if ((msdc_async_use_dma(data->host_cookie)) && (host->tune == 0)){
                    msdc_dma_stop(host);
                    mrq = host->mrq;
                    msdc_dma_clear(host);
                    if(mrq->done)
                        mrq->done(mrq);
                    msdc_gate_clock(host, 1);
                    host->error &= ~REQ_DAT_ERR;
                }else {
                    complete(&host->xfer_done);     
                }      
            } 
        }   
        /* error handle with MSDC_INT_DATCRCERR, MSDC_INT_DATTMO*/
        if (intsts & datsts) {         
            /* do basic reset, or stop command will sdc_busy */
            msdc_reset_hw(host->id);           
            atomic_set(&host->abort, 1);  /* For PIO mode exit */
            
            if (intsts & MSDC_INT_DATTMO){
                data->error = (unsigned int)-ETIMEDOUT;                
                IRQ_MSG("XXX CMD<%d> Arg<0x%.8x> MSDC_INT_DATTMO", host->mrq->cmd->opcode, host->mrq->cmd->arg);
            }
            else if (intsts & MSDC_INT_DATCRCERR){
                data->error = (unsigned int)-EIO;            
                IRQ_MSG("XXX CMD<%d> Arg<0x%.8x> MSDC_INT_DATCRCERR, SDC_DCRC_STS<0x%x>", 
                      host->mrq->cmd->opcode, host->mrq->cmd->arg, sdr_read32(SDC_DCRC_STS));
            }
                                    
            //if(sdr_read32(MSDC_INTEN) & MSDC_INT_XFER_COMPL) {  
            if (host->dma_xfer) {
                 if ((msdc_async_use_dma(data->host_cookie)) && (host->tune == 0)){
                    msdc_dma_stop(host);
                    msdc_clr_fifo(host->id); 
                    msdc_clr_int(); 
                    mrq = host->mrq;
                    msdc_dma_clear(host);
                    if(mrq->done)
                        mrq->done(mrq);
                    msdc_gate_clock(host, 1);
                    host->error |= REQ_DAT_ERR;
                } else {
                    complete(&host->xfer_done); /* Read CRC come fast, XFER_COMPL not enabled */
                }
            } /* PIO mode can't do complete, because not init */
        }
        if ((stop != NULL) && (host->autocmd & MSDC_AUTOCMD12) && (intsts & cmdsts)) {
            if (intsts & MSDC_INT_ACMDRDY) {
                u32 *arsp = &stop->resp[0];
               *arsp = sdr_read32(SDC_ACMD_RESP);
            }else if (intsts & MSDC_INT_ACMDCRCERR) {
                stop->error =(unsigned int)-EIO;
                host->error |= REQ_STOP_EIO;
                msdc_reset_hw(host->id); 
            } else if (intsts & MSDC_INT_ACMDTMO) {
                stop->error =(unsigned int)-ETIMEDOUT;
                host->error |= REQ_STOP_TMO;
                msdc_reset_hw(host->id); 
            }
            if((intsts & MSDC_INT_ACMDCRCERR) || (intsts & MSDC_INT_ACMDTMO)){
                if (host->dma_xfer){
                     if ((msdc_async_use_dma(data->host_cookie)) && (host->tune == 0)){
                        msdc_dma_stop(host);
                        msdc_clr_fifo(host->id); 
                        msdc_clr_int();
                        mrq = host->mrq;
                        msdc_dma_clear(host);
                        if(mrq->done)
                            mrq->done(mrq);
                        msdc_gate_clock(host, 1);
                    } else {
                        complete(&host->xfer_done); //Autocmd12 issued but error occur, the data transfer done INT will not issue,so cmplete is need here 
                    }
                }/* PIO mode can't do complete, because not init */
            }
        }
    }
    /* command interrupts */
    if ((cmd != NULL) && (intsts & cmdsts)) {
        if (intsts & MSDC_INT_CMDRDY) {
            u32 *rsp = NULL;
            rsp = &cmd->resp[0];
            
            switch (host->cmd_rsp) {
                case RESP_NONE:
                    break;
                case RESP_R2:
                    *rsp++ = sdr_read32(SDC_RESP3); *rsp++ = sdr_read32(SDC_RESP2);
                    *rsp++ = sdr_read32(SDC_RESP1); *rsp++ = sdr_read32(SDC_RESP0);
                    break;
                default: /* Response types 1, 3, 4, 5, 6, 7(1b) */
                    *rsp = sdr_read32(SDC_RESP0);    
                    break;
            }
        } else if (intsts & MSDC_INT_RSPCRCERR) {
            cmd->error = (unsigned int)-EIO;
            IRQ_MSG("XXX CMD<%d> MSDC_INT_RSPCRCERR Arg<0x%.8x>",cmd->opcode, cmd->arg);
            msdc_reset_hw(host->id); 
        }else if (intsts & MSDC_INT_CMDTMO) {
            cmd->error = (unsigned int)-ETIMEDOUT;
            IRQ_MSG("XXX CMD<%d> MSDC_INT_CMDTMO Arg<0x%.8x>",cmd->opcode, cmd->arg);
            msdc_reset_hw(host->id);  
        }
        if(intsts & (MSDC_INT_CMDRDY | MSDC_INT_RSPCRCERR | MSDC_INT_CMDTMO))
            complete(&host->cmd_done);
    }

    /* mmc irq interrupts */
    if (intsts & MSDC_INT_MMCIRQ) {
        //printk(KERN_INFO "msdc[%d] MMCIRQ: SDC_CSTS=0x%.8x\r\n", host->id, sdr_read32(SDC_CSTS));    
    }
    latest_int_status[host->id] = 0;
    return IRQ_HANDLED;
}

/*--------------------------------------------------------------------------*/
/* platform_driver members                   */
/*--------------------------------------------------------------------------*/
/* called by msdc_drv_probe/remove */
static void msdc_enable_cd_irq(struct msdc_host *host, int enable)
{
#ifndef FPGA_PLATFORM
#ifdef CUST_EINT_MSDC1_INS_NUM
    struct msdc_hw *hw = host->hw;  // fixed build warning
#endif

    u32 base = host->base;
    unsigned long     flags_lock = 0;
    sdr_clr_bits(MSDC_PS, MSDC_PS_CDEN);
    if (host->hw->flags & MSDC_SDIO_IRQ)
        spin_lock_irqsave(&SDIO_IRQ_lock, flags_lock);
    sdr_clr_bits(MSDC_INTEN, MSDC_INTEN_CDSC);
    if (host->hw->flags & MSDC_SDIO_IRQ)
        spin_unlock_irqrestore(&SDIO_IRQ_lock, flags_lock);
    sdr_clr_bits(SDC_CFG, SDC_CFG_INSWKUP);
#ifdef CUST_EINT_MSDC1_INS_NUM
    //struct msdc_hw *hw = host->hw;  // fixed build warning
    if(enable){
        if(host->id == 1 && (MSDC_SD == hw->host_function) && (hw->flags & MSDC_CD_PIN_EN)){

            if(hw->cd_level)
                host->sd_cd_polarity = (~EINT_MSDC1_INS_POLARITY)&0x1;
            else
                host->sd_cd_polarity = EINT_MSDC1_INS_POLARITY;
      
            //mt_eint_set_sens(CUST_EINT_MSDC1_INS_NUM, EINT_MSDC1_INS_SENSITIVE);
            mt_eint_set_hw_debounce(CUST_EINT_MSDC1_INS_NUM, CUST_EINT_MSDC1_INS_DEBOUNCE_CN);
      
            if (0 == host->sd_cd_polarity){
                //mt_eint_registration(CUST_EINT_MSDC1_INS_NUM, CUST_EINTF_TRIGGER_LOW, msdc1_eint_handler, CUST_EINT_MSDC1_INS_DEBOUNCE_CN);
                mt_eint_registration(CUST_EINT_MSDC1_INS_NUM, CUST_EINTF_TRIGGER_LOW, msdc1_eint_handler, 1);
            } else {
                //mt_eint_registration(CUST_EINT_MSDC1_INS_NUM, CUST_EINTF_TRIGGER_HIGH, msdc1_eint_handler, CUST_EINT_MSDC1_INS_DEBOUNCE_CN);
                mt_eint_registration(CUST_EINT_MSDC1_INS_NUM, CUST_EINTF_TRIGGER_HIGH, msdc1_eint_handler, 1);
            }

            //mt_eint_set_sens(EINT_MSDC1_INS_NUM,EINT_MSDC1_INS_SENSITIVE);
            //mt_eint_set_hw_debounce(EINT_MSDC1_INS_NUM,EINT_MSDC1_INS_DEBOUNCE_CN);
            //mt_eint_registration(EINT_MSDC1_INS_NUM, host->sd_cd_polarity,msdc1_eint_handler,1);
            //mt_eint_unmask(CUST_EINT_MSDC1_INS_NUM);
      
            ERR_MSG("SD card detection eint resigter.");      
        }
        if(host->id == 2 && (MSDC_SD == hw->host_function) && (hw->flags & MSDC_CD_PIN_EN)){
            // sdio need handle eint11  
        }
    } else{
        if(host->id == 1){
            //mt_eint_mask(EINT_MSDC1_INS_NUM);
            mt_eint_mask(CUST_EINT_MSDC1_INS_NUM);
        }
        if(host->id == 2){
            // sdio need handle eint11  
        }
    }
#endif
#else 
    struct msdc_hw *hw = host->hw;
    u32 base = host->base;

    /* for sdio, not set */
    if ((hw->flags & MSDC_CD_PIN_EN) == 0) {
        /* Pull down card detection pin since it is not avaiable */
        /*
        if (hw->config_gpio_pin) 
            hw->config_gpio_pin(MSDC_CD_PIN, GPIO_PULL_DOWN);
        */
        sdr_clr_bits(MSDC_PS, MSDC_PS_CDEN);
        sdr_clr_bits(MSDC_INTEN, MSDC_INTEN_CDSC);
        sdr_clr_bits(SDC_CFG, SDC_CFG_INSWKUP);
        return;
    }

    N_MSG(CFG, "CD IRQ Eanable(%d)", enable);

    if (enable) {
        if (hw->enable_cd_eirq) { /* not set, never enter */
            hw->enable_cd_eirq();
        } else {
            /* card detection circuit relies on the core power so that the core power 
             * shouldn't be turned off. Here adds a reference count to keep 
             * the core power alive.
             */
            if (hw->config_gpio_pin) /* NULL */
                hw->config_gpio_pin(MSDC_CD_PIN, GPIO_PULL_UP);

            sdr_set_field(MSDC_PS, MSDC_PS_CDDEBOUNCE, DEFAULT_DEBOUNCE);
            sdr_set_bits(MSDC_PS, MSDC_PS_CDEN);
            sdr_set_bits(MSDC_INTEN, MSDC_INTEN_CDSC);
            sdr_set_bits(SDC_CFG, SDC_CFG_INSWKUP);  /* not in document! Fix me */
        }
    } else {
        if (hw->disable_cd_eirq) {
            hw->disable_cd_eirq();
        } else {
            if (hw->config_gpio_pin) /* NULL */
                hw->config_gpio_pin(MSDC_CD_PIN, GPIO_PULL_DOWN);

            sdr_clr_bits(SDC_CFG, SDC_CFG_INSWKUP);
            sdr_clr_bits(MSDC_PS, MSDC_PS_CDEN);
            sdr_clr_bits(MSDC_INTEN, MSDC_INTEN_CDSC);

            /* Here decreases a reference count to core power since card 
             * detection circuit is shutdown.
             */
        }
    }
#endif
}

/* called by msdc_drv_probe */

static void msdc_init_hw(struct msdc_host *host)
{
    u32 base = host->base;
    struct msdc_hw *hw = host->hw;
    u32 cur_rxdly0, cur_rxdly1;
    unsigned long flags_lock = 0;

#ifdef MT_SD_DEBUG  
    msdc_reg[host->id] = (struct msdc_regs *)host->base;
#endif

    /* Power on */ 
    msdc_pin_reset(host, MSDC_PIN_PULL_UP); 
#ifndef FPGA_PLATFORM  
    enable_clock(MT_CG_PERI_MSDC30_0 + host->id, "SD"); 
#endif
    host->core_clkon = 1;      
    /* Bug Fix: If clock is disabed, Version Register Can't be read. */    
    msdc_select_clksrc(host, hw->clk_src);     

    /* Configure to MMC/SD mode */
    sdr_set_field(MSDC_CFG, MSDC_CFG_MODE, MSDC_SDMMC); 
       
    /* Reset */
    msdc_reset_hw(host->id);

    /* Disable card detection */
    sdr_clr_bits(MSDC_PS, MSDC_PS_CDEN);

    /* Disable and clear all interrupts */
    if(host->hw->flags & MSDC_SDIO_IRQ)
        spin_lock_irqsave(&SDIO_IRQ_lock, flags_lock);
    sdr_clr_bits(MSDC_INTEN, sdr_read32(MSDC_INTEN));
    if(host->hw->flags & MSDC_SDIO_IRQ)
        spin_unlock_irqrestore(&SDIO_IRQ_lock, flags_lock);
    sdr_write32(MSDC_INT, sdr_read32(MSDC_INT));
    
#if 1
    /* reset tuning parameter */
    //sdr_write32(MSDC_PAD_CTL0,   0x00098000);
    //sdr_write32(MSDC_PAD_CTL1,   0x000A0000);
    //sdr_write32(MSDC_PAD_CTL2,   0x000A0000);
    sdr_write32(MSDC_PAD_TUNE,   0x00000000);
  
    sdr_write32(MSDC_IOCON,    0x00000000);

    sdr_set_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);
    cur_rxdly0 = ((hw->dat0rddly & 0x1F) << 24) | ((hw->dat1rddly & 0x1F) << 16) |
                      ((hw->dat2rddly & 0x1F) << 8)  | ((hw->dat3rddly & 0x1F) << 0);
    cur_rxdly1 = ((hw->dat4rddly & 0x1F) << 24) | ((hw->dat5rddly & 0x1F) << 16) |
                     ((hw->dat6rddly & 0x1F) << 8)  | ((hw->dat7rddly & 0x1F) << 0);

    if (MSDC_EMMC == host->hw->host_function){
        //for write tuning, only tuning DAT0
        sdr_write32(MSDC_DAT_RDDLY0, cur_rxdly0);
    } else {
        sdr_write32(MSDC_DAT_RDDLY0, 0x00000000);
    }

    sdr_write32(MSDC_DAT_RDDLY1, 0x00000000); 
    
    //sdr_write32(MSDC_PATCH_BIT0, 0x003C004F); 
    if ((host->hw->host_function == MSDC_EMMC) || (host->hw->host_function == MSDC_SD)){
        sdr_write32(MSDC_PATCH_BIT1, 0xFFFF00C9);
    } else {
        sdr_write32(MSDC_PATCH_BIT1, 0xFFFF0009);
    }
    //sdr_set_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_WRDAT_CRCS, 1); 
    //sdr_set_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_CMD_RSP,    1);
    //data delay settings should be set after enter high speed mode(now is at ios function >25MHz), detail information, please refer to P4 description
    host->saved_para.pad_tune = (((hw->cmdrrddly & 0x1F) << 22) | ((hw->cmdrddly & 0x1F) << 16) | ((hw->datwrddly & 0x1F) << 0));//sdr_read32(MSDC_PAD_TUNE);
    host->saved_para.ddly0 = cur_rxdly0; //sdr_read32(MSDC_DAT_RDDLY0);
    host->saved_para.ddly1 = cur_rxdly1; //sdr_read32(MSDC_DAT_RDDLY1);
    sdr_get_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_CMD_RSP,    host->saved_para.cmd_resp_ta_cntr);
    sdr_get_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_WRDAT_CRCS, host->saved_para.wrdat_crc_ta_cntr);     

    /* for write: 3T need wait before host check busy after crc status 
     * for write: host check timeout change to 16T */
    sdr_get_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_ECO_1, host->saved_para.write_timeout_eco1);
    sdr_get_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_ECO_2, host->saved_para.write_timeout_eco2); 

    if (!(is_card_sdio(host)|| (host->hw->flags & MSDC_SDIO_IRQ))) {  /* internal clock: latch read data, not apply to sdio */           
        //sdr_set_bits(MSDC_PATCH_BIT0, MSDC_PATCH_BIT_CKGEN_CK);//No feedback clock only internal clock in MT6589/MT6585 
        host->hw->cmd_edge  = 0; // tuning from 0
        host->hw->rdata_edge = 0;  
        host->hw->wdata_edge = 0;  
    }else if (hw->flags & MSDC_INTERNAL_CLK) {
        //sdr_set_bits(MSDC_PATCH_BIT0, MSDC_PATCH_BIT_CKGEN_CK);
    }
    //sdr_set_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);

#endif    

    /* for safety, should clear SDC_CFG.SDIO_INT_DET_EN & set SDC_CFG.SDIO in 
       pre-loader,uboot,kernel drivers. and SDC_CFG.SDIO_INT_DET_EN will be only
       set when kernel driver wants to use SDIO bus interrupt */
    /* Configure to enable SDIO mode. it's must otherwise sdio cmd5 failed */
    sdr_set_bits(SDC_CFG, SDC_CFG_SDIO);

    /* disable detect SDIO device interupt function */
    sdr_clr_bits(SDC_CFG, SDC_CFG_SDIOIDE);

    /* eneable SMT for glitch filter */
#ifdef FPGA_PLATFORM
    sdr_set_bits(MSDC_PAD_CTL0, MSDC_PAD_CTL0_CLKSMT);
    sdr_set_bits(MSDC_PAD_CTL1, MSDC_PAD_CTL1_CMDSMT);
    sdr_set_bits(MSDC_PAD_CTL2, MSDC_PAD_CTL2_DATSMT);
#else
    msdc_set_smt(host,1);
#endif
    /* set clk, cmd, dat pad driving */
#ifdef FPGA_PLATFORM
    sdr_set_field(MSDC_PAD_CTL0, MSDC_PAD_CTL0_CLKDRVN, hw->clk_drv);
    sdr_set_field(MSDC_PAD_CTL0, MSDC_PAD_CTL0_CLKDRVP, hw->clk_drv);
    sdr_set_field(MSDC_PAD_CTL1, MSDC_PAD_CTL1_CMDDRVN, hw->cmd_drv);
    sdr_set_field(MSDC_PAD_CTL1, MSDC_PAD_CTL1_CMDDRVP, hw->cmd_drv);
    sdr_set_field(MSDC_PAD_CTL2, MSDC_PAD_CTL2_DATDRVN, hw->dat_drv);
    sdr_set_field(MSDC_PAD_CTL2, MSDC_PAD_CTL2_DATDRVP, hw->dat_drv);
#else
    msdc_set_driving(host,hw,0);
#endif
    
    INIT_MSG("msdc drving<clk %d,cmd %d,dat %d>",hw->clk_drv,hw->cmd_drv,hw->dat_drv);
   
    /* write crc timeout detection */
    sdr_set_field(MSDC_PATCH_BIT0, 1 << 30, 1);

    /* Configure to default data timeout */
    sdr_set_field(SDC_CFG, SDC_CFG_DTOC, DEFAULT_DTOC);

    msdc_set_buswidth(host, MMC_BUS_WIDTH_1);

    N_MSG(FUC, "init hardware done!");
}

/* called by msdc_drv_remove */
static void msdc_deinit_hw(struct msdc_host *host)
{
    u32 base = host->base;
    unsigned long     flags_lock = 0;

    /* Disable and clear all interrupts */
    if (host->hw->flags & MSDC_SDIO_IRQ)
        spin_lock_irqsave(&SDIO_IRQ_lock, flags_lock);
    sdr_clr_bits(MSDC_INTEN, sdr_read32(MSDC_INTEN));
    if (host->hw->flags & MSDC_SDIO_IRQ)
        spin_unlock_irqrestore(&SDIO_IRQ_lock, flags_lock);
    sdr_write32(MSDC_INT, sdr_read32(MSDC_INT));

    /* Disable card detection */
    msdc_enable_cd_irq(host, 0);
    msdc_set_power_mode(host, MMC_POWER_OFF);   /* make sure power down */
}

/* init gpd and bd list in msdc_drv_probe */
static void msdc_init_gpd_bd(struct msdc_host *host, struct msdc_dma *dma)
{
    gpd_t *gpd = dma->gpd; 
    bd_t  *bd  = dma->bd;     
    bd_t  *ptr, *prev;
    
    /* we just support one gpd */     
    int bdlen = MAX_BD_PER_GPD;       

    /* init the 2 gpd */
    memset(gpd, 0, sizeof(gpd_t) * 2);
    //gpd->next = (void *)virt_to_phys(gpd + 1); /* pointer to a null gpd, bug! kmalloc <-> virt_to_phys */  
    //gpd->next = (dma->gpd_addr + 1);    /* bug */
    gpd->next = (void *)((u32)dma->gpd_addr + sizeof(gpd_t));    

    //gpd->intr = 0;
    gpd->bdp  = 1;   /* hwo, cs, bd pointer */      
    //gpd->ptr  = (void*)virt_to_phys(bd); 
    gpd->ptr = (void *)dma->bd_addr; /* physical address */
    
    memset(bd, 0, sizeof(bd_t) * bdlen);
    ptr = bd + bdlen - 1;
    //ptr->eol  = 1;  /* 0 or 1 [Fix me]*/
    //ptr->next = 0;    
    
    while (ptr != bd) {
        prev = ptr - 1;
        prev->next = (void *)(dma->bd_addr + sizeof(bd_t) *(ptr - bd));
        ptr = prev;
    }
}

static void msdc_init_dma_latest_address(void)
{
    struct dma_addr *ptr, *prev; 
    int bdlen = MAX_BD_PER_GPD;       

    memset(msdc_latest_dma_address, 0, sizeof(struct dma_addr) * bdlen);
    ptr = msdc_latest_dma_address + bdlen - 1;
    while(ptr != msdc_latest_dma_address){
        prev = ptr - 1; 
        prev->next = (void *)(msdc_latest_dma_address + sizeof(struct dma_addr) * (ptr - msdc_latest_dma_address));
        ptr = prev;
    }

}
struct msdc_host *msdc_get_host(int host_function,bool boot,bool secondary)
{
    int host_index = 0;
    struct msdc_host *host = NULL;
    
    for(;host_index < HOST_MAX_NUM;++host_index)
    {
        if(!mtk_msdc_host[host_index])
            continue;
        if((host_function == mtk_msdc_host[host_index]->hw->host_function)
            && (boot == mtk_msdc_host[host_index]->hw->boot))
        {
            host = mtk_msdc_host[host_index];
            break;
        }
    }
    if(secondary && (host_function == MSDC_SD))
        host = mtk_msdc_host[2];
    if(host == NULL){
        printk(KERN_ERR "[MSDC] This host(<host_function:%d> <boot:%d><secondary:%d>) isn't in MSDC host config list",host_function,boot,secondary);
        //BUG();
    }
    
    return host;        
}
EXPORT_SYMBOL(msdc_get_host);

struct gendisk * mmc_get_disk(struct mmc_card *card)
{
    struct mmc_blk_data *md;
    //struct gendisk *disk;

    BUG_ON(!card);
    md = mmc_get_drvdata(card);
    BUG_ON(!md);
    BUG_ON(!md->disk);

    return md->disk;
}

#if defined(MTK_EMMC_SUPPORT) && defined(CONFIG_PROC_FS)
static struct proc_dir_entry *proc_emmc;

static inline int emmc_proc_info(char *buf, struct hd_struct *this)
{
    int i = 0;
    char *no_partition_name = "n/a";

    for (i = 0; i < PART_NUM; i++) {
        if (PartInfo[i].partition_idx != 0 && PartInfo[i].partition_idx == this->partno) {
            break;
        }
    }            
    
    return sprintf(buf, "emmc_p%d: %8.8x %8.8x \"%s\"\n", this->partno,
                   (unsigned int)this->start_sect,
                   (unsigned int)this->nr_sects, (i >= PART_NUM ? no_partition_name : PartInfo[i].name));
}

static int emmc_read_proc (char *page, char **start, off_t off, int count,
              int *eof, void *data_unused)
{
    int len, l;
    off_t   begin = 0;
    struct disk_part_iter piter;
    struct hd_struct *part;
    struct msdc_host *host;
    struct gendisk *disk;

        /* emmc always in slot0 */
    host = msdc_get_host(MSDC_EMMC,MSDC_BOOT_EN,0);
    BUG_ON(!host);
    BUG_ON(!host->mmc);
    BUG_ON(!host->mmc->card);
    disk = mmc_get_disk(host->mmc->card);
    
    len = sprintf(page, "partno:    start_sect   nr_sects  partition_name\n");
    disk_part_iter_init(&piter, disk, 0);
    while ((part = disk_part_iter_next(&piter))){
        l = emmc_proc_info(page + len, part);
        len += l;
        if (len+begin > off+count)
            goto done;
        if (len+begin < off) {
            begin += len;
            len = 0;
        }
    }
    disk_part_iter_exit(&piter);
    printk("%s: %s \n", __func__, page);
    *eof = 1;

done:
    if (off >= len+begin)
        return 0;
    *start = page + (off-begin);
    return ((count < begin+len-off) ? count : begin+len-off);
}


#endif /* MTK_EMMC_SUPPORT && CONFIG_PROC_FS */

/* This is called by run_timer_softirq */
static void msdc_timer_pm(unsigned long data)
{
    struct msdc_host *host = (struct msdc_host *)data;   
    unsigned long flags;

    spin_lock_irqsave(&host->clk_gate_lock, flags);
    if (host->clk_gate_count == 0) { 
        msdc_clksrc_onoff(host, 0);
        N_MSG(CLK, "[%s]: msdc%d, time out, dsiable clock, clk_gate_count=%d\n", __func__, host->id, host->clk_gate_count);
    }        
    spin_unlock_irqrestore(&host->clk_gate_lock, flags); 
}

static u32 first_probe = 0; 
#ifndef FPGA_PLATFORM
#if (1 == MODIFY_FOR_MT6582)
static void msdc_set_host_power_control(struct msdc_host *host)
{
    
   switch(host->id){
      case 0:
          if(MSDC_EMMC == host->hw->host_function){
#ifdef MTK_MT8193_SUPPORT
              host->power_control = msdc_emmc_power_8193;
#else
              host->power_control = msdc_emmc_power;
#endif
          }else{
                ERR_MSG("Host function defination error. Please check host_function<0x%lx>",host->hw->host_function);
                BUG();
          }
          break;
      case 1:
          if(MSDC_SD == host->hw->host_function){
                host->power_control = msdc_sd_power;
                host->power_switch = msdc_sd_power_switch;
          }else{
                ERR_MSG("Host function defination error. Please check host_function<0x%lx>",host->hw->host_function);
                BUG();
          }
          break;
      case 2:
          if(MSDC_SDIO == host->hw->host_function){
                host->power_control = msdc_sdio_power;
                host->power_switch	= msdc_sdio_power_switch;
          }else{
                ERR_MSG("Host function defination error. Please check host_function<0x%lx>",host->hw->host_function);
                BUG();
          }
          break;    
      default:
          break;
    }
}
#else 
static void msdc_set_host_power_control(struct msdc_host *host)
{
    
    switch(host->id){
        case 0:
            if(MSDC_EMMC == host->hw->host_function){
#ifdef MTK_MT8193_SUPPORT
                host->power_control = msdc_emmc_power_8193;
#else
                host->power_control = msdc_emmc_power;
#endif
            }else{
                ERR_MSG("Host function defination error. Please check host_function<0x%lx>",host->hw->host_function);
                BUG();
            }
            break;
        case 1:
            if((MSDC_VMC == host->power_domain) && (MSDC_SD == host->hw->host_function )){
                host->power_control = msdc_sd_power;
                host->power_switch = msdc_sd_power_switch;
            }else{
                ERR_MSG("Host function defination error or power domain selection error. Please check host_function<0x%lx> and Power_domain<%d>",host->hw->host_function,host->power_domain);
                BUG();
            }
            break;
        case 2:
            if(( MSDC_VGP6 == host->power_domain) && (MSDC_SD == host->hw->host_function)){
                host->power_control = msdc_sd_power;
                //host->power_switch = msdc_sd_power_switch; //CHECK ME
                //If MSDC2 was defined to support SD card, its default power domian only support SD2.0. External power for SD card flash is needed for SD3.0
            }else if((MSDC_VIO18_MC2 == host->power_domain ) && (MSDC_SDIO == host->hw->host_function)){
                //CHECK ME
                host->power_control = msdc_sdio_power;
                host->power_switch	= msdc_sdio_power_switch;
            }else if((MSDC_VIO28_MC2 == host->power_domain ) && (MSDC_SDIO == host->hw->host_function)){
                //CHECK ME
                host->power_control = msdc_sdio_power;
                host->power_switch	= msdc_sdio_power_switch;
            }else{
                ERR_MSG("Host function defination error or power domain selection error. Please check host_function<0x%lx> and Power_domain<%d>",host->hw->host_function,host->power_domain);
                BUG();
            }
            break;
#if (0 == REMOVEED_FOR_MT6582)
        case 3:
            if(MSDC_SDIO == host->hw->host_function){
                host->power_control = msdc_sdio_power;
                host->power_switch	= msdc_sdio_power_switch;
            } else{
                ERR_MSG("Host function defination error. Please check host_function<0x%lx>",host->hw->host_function);
                BUG();
            }
            break;    
        case 4:
            if(MSDC_EMMC == host->hw->host_function){
#ifdef MTK_MT8193_SUPPORT
                host->power_control = msdc_emmc_power_8193;
#else
                host->power_control = msdc_emmc_power;
#endif
            }else{
                ERR_MSG("Host function defination error. Please check host_function<0x%lx>",host->hw->host_function);
                BUG();
            }
            break;
#endif
        default:
            break;
    }
}
#endif /* end of MODIFY_FOR_MT6582 */
#endif /* end of FPGA_PLATFORM */

#if (0 == REMOVEED_FOR_MT6582)
/* cause every module have it bus ID under MT6582, no need share bus with others, 
   so mask the callback function */
static void msdc_check_mpu_voilation(u32 addr,int wr_vio)
{
    int index = 0;
    int status = -1;
    struct dma_addr *msdc_dma_address = NULL;
    struct dma_addr *p_msdc_dma_address = NULL;
    printk(KERN_CRIT "MSDC check EMI MPU violation.\n");
    printk(KERN_CRIT "addr = 0x%x,%s violation.\n",addr,wr_vio? "Write memory<Read frome eMMC/SD>":"Read memory<Write to eMMC/SD>");
    printk(KERN_CRIT "MSDC DMA running status:\n");
    printk(KERN_CRIT "**************************************\n");
    for(index = 0; index < HOST_MAX_NUM; index++) {
        status = msdc_get_dma_status(index);
        printk(KERN_CRIT "MSDC%d: ", index);
        if(status == 0){
            printk(KERN_CRIT "DMA mode is disabled Now\n");
            continue;
        }else if (status == 1){
            printk(KERN_CRIT "Read data from eMMC/SD to DRAM with DMA mode\n");
        }else if (status == 2){
            printk(KERN_CRIT "Write data from DRAM to eMMC/SD with DMA mode\n");
        }else if (status == -1){
            printk(KERN_CRIT "No data transaction or the device is not present until now\n");
            continue;
        }

        msdc_dma_address =  msdc_get_dma_address(index);

        if(msdc_dma_address){
            p_msdc_dma_address = msdc_dma_address;
            printk(KERN_CRIT "MSDC%d: \n", index);
            while (p_msdc_dma_address != NULL){
                printk(KERN_CRIT "   addr=0x%x, size=%d\n", p_msdc_dma_address->start_address, p_msdc_dma_address->size);
                if(p_msdc_dma_address->end)
                    break;
                p_msdc_dma_address = p_msdc_dma_address->next;
            }
        }else {
            printk(KERN_CRIT "MSDC%d: BD count=0\n", index);
        }
      printk(KERN_CRIT "-------------------------------\n");
    }
    printk(KERN_CRIT "**************************************\n");
}
static void msdc_register_emi_mpu_callback(int id)
{
    switch(id){
        case 0:
        case 4:
            emi_mpu_notifier_register(MST_ID_MMPERI_1,msdc_check_mpu_voilation);
            printk(KERN_CRIT "msdc%d register emi_mpu_notifier\n",id);
            break;
        case 2:
            emi_mpu_notifier_register(MST_ID_MMPERI_2,msdc_check_mpu_voilation);
            printk(KERN_CRIT "msdc%d register emi_mpu_notifier\n",id);
            break;
        default:
            break;
    }
}
#endif
void SRC_trigger_signal(int i_on)
{
    if ((ghost!= NULL) && (ghost->hw->flags & MSDC_SDIO_IRQ))
    {
        printk("msdc2 SRC_trigger_signal %d \n",i_on);
        src_clk_control = i_on;
        if (src_clk_control)
        {
            msdc_clksrc_onoff(ghost, 1); 
            mb();
            if(ghost->mmc->sdio_irq_thread &&  (atomic_read(&ghost->mmc->sdio_irq_thread_abort) == 0))      //if (ghost->mmc->sdio_irq_thread)
            {
                mmc_signal_sdio_irq(ghost->mmc);
                if (u_msdc_irq_counter<3)
                    printk("msdc2 SRC_trigger_signal mmc_signal_sdio_irq \n");
            }
            //printk("msdc2 SRC_trigger_signal ghost->id=%d\n",ghost->id);
        }
    }
    
}
EXPORT_SYMBOL(SRC_trigger_signal);

#ifdef MTK_SDIOAUTOK_SUPPORT
/*************************************************************************
* FUNCTION
*  sdio_stop_transfer
*
* DESCRIPTION
*  This function for DVFS, stop SDIO transfer and wait for transfer complete
*
* PARAMETERS
*
* RETURN VALUES
*    0: transfer complete
*    <0: error code
*************************************************************************/

//static DEFINE_SPINLOCK(mt_sdio_stop_lock);
static int count = 0;

int sdio_stop_transfer(void)
{
    int hostidx = 0;
//    unsigned long flags;
    //printk("[%s] Enter\n", __func__);
	
	count++;
    
    for(hostidx = 0; hostidx < HOST_MAX_NUM; hostidx++)
    {
        if(mtk_msdc_host[hostidx]==NULL)
        {
            printk("[%s] mtk_msdc_host[%d] = NULL\n", __func__, hostidx);
            continue;
        }
        if(is_card_sdio(mtk_msdc_host[hostidx]))
        {
            struct msdc_host *host = mtk_msdc_host[hostidx];
#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
            while(1)
            {
                if (atomic_read(&host->ot_done)) {
                    printk("[%s] msdc%d online tuning done, host->ot_work.chg_volt = %d\n", __func__, host->id, host->ot_work.chg_volt);	
                    break;
                }
                
                msleep(1);    
            }
#endif  // MTK_SDIO30_ONLINE_TUNING_SUPPORT
            if(atomic_read(&host->sdio_stopping))
            {
                printk("[%s] msdc%d sdio stopping\n", __func__, host->id);
                return -1;
            }
#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT	
			else if(atomic_read(&host->ot_done) == 0)
			{
				printk("[%s] msdc%d online tuning is working\n", __func__, host->id);
				return -1;
			}
#endif
            else
            {
                atomic_set(&host->sdio_stopping, 1);
                //mmc_claim_host(host->mmc);
                autok_claim_host(host);
                //printk("[%s] msdc%d host claimed\n", __func__, host->id);
            }
        }
    }
    
    //printk("[%s] Exit\n", __func__);
    
    return 0;
}
EXPORT_SYMBOL(sdio_stop_transfer);

/*************************************************************************
* FUNCTION
*  sdio_start_ot_transfer
*
* DESCRIPTION
*  This function for DVFS, start online tuning and start SDIO transfer
*
* PARAMETERS
*    voltage: The voltage that DVFS changes to
*
* RETURN VALUES
*    0: success
*    <0: error code
*************************************************************************/

int sdio_start_ot_transfer(void)
{
    int hostidx = 0;
	
	count--;
	if(count != 0)
		printk("[%s] sdio stop and start are not pairwise, count = %d\n", __func__, count);
    
    for(hostidx = 0; hostidx < HOST_MAX_NUM; hostidx++)
    {
        if(mtk_msdc_host[hostidx]==NULL)
        {
            printk("[%s] mtk_msdc_host[%d] = NULL\n", __func__, hostidx);
            continue;
        }
        if(is_card_sdio(mtk_msdc_host[hostidx]))
        {
            struct msdc_host *host = mtk_msdc_host[hostidx];
#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
			host->ot_work.chg_volt = 1;
			start_online_tuning((unsigned long)host);
#else
            //printk("[%s] msdc%d release host, host->mmc = 0x%x, host->sdio_stopping = %d (1)\n", __func__, host->id, host->mmc, atomic_read(&host->sdio_stopping));
            atomic_set(&host->sdio_stopping, 0);
            //mmc_release_host(host->mmc);
            autok_release_host(host);
            //printk("[%s] msdc%d host released\n", __func__, host->id);
			//printk("[%s] msdc%d host->sdio_stopping = %d (0)\n", __func__, host->id, atomic_read(&host->sdio_stopping));
#endif  // MTK_SDIO30_ONLINE_TUNING_SUPPORT
        }
    }
    
    return 0;
}
EXPORT_SYMBOL(sdio_start_ot_transfer);

#endif  // MTK_SDIOAUTOK_SUPPORT

/*************************************************************************
* FUNCTION
*  sdio_get_rings
*
* DESCRIPTION
*  This function for AUTOK, return io ring value
*
* PARAMETERS
*  *io_ring: return io_ring value
*  *core_ring: return core_ring value
* RETURN VALUES
*  0
*************************************************************************/
unsigned int sdio_get_rings(unsigned int *io_ring, unsigned int *core_ring)
{
#ifdef MTK_SDIOAUTOK_SUPPORT
    *io_ring = ((osc_value & 0xFFFF0000) >> 16);
    *core_ring = (osc_value & 0x0000FFFF);
#else   // MTK_SDIOAUTOK_SUPPORT
    *io_ring = 0;
    *core_ring = 0;
#endif  // MTK_SDIOAUTOK_SUPPORT
    
    printk("[%s] io ring = 0x%x, core ring = 0x%x\n", __func__, *io_ring, *core_ring);
    return 0;
}

#ifdef CONFIG_MTK_HIBERNATION
extern unsigned int mt_eint_get_polarity(unsigned int eint_num);
int msdc_drv_pm_restore_noirq(struct device *device)
{
    struct platform_device *pdev = to_platform_device(device);
    struct mmc_host *mmc = NULL;
    struct msdc_host *host = NULL;
    u32 l_polarity = 0;
    BUG_ON(pdev == NULL);
    mmc = platform_get_drvdata(pdev);
    host = mmc_priv(mmc);
    if(host->hw->host_function == MSDC_SD){
        if((host->id == 1) && (host->hw->flags & MSDC_CD_PIN_EN)){

#ifdef CUST_EINT_MSDC1_INS_NUM   
            l_polarity = mt_eint_get_polarity(CUST_EINT_MSDC1_INS_NUM);
#endif
            if (l_polarity == MT_POLARITY_LOW){
                host->sd_cd_polarity = 0;
            } else {
                host->sd_cd_polarity = 1;
            }
            
            if(!(host->hw->cd_level ^ host->sd_cd_polarity) && host->mmc->card){
                mmc_card_set_removed(host->mmc->card);
                host->card_inserted = 0;
            }        
        } else if((host->id == 2) && (host->hw->flags & MSDC_CD_PIN_EN)){
            // sdio need handle here
        }
        host->block_bad_card = 0;
    }
    return 0;
}
#endif

static int msdc_drv_probe(struct platform_device *pdev)
{
    struct mmc_host *mmc;
    struct resource *mem;
    struct msdc_host *host;
    struct msdc_hw *hw;
    unsigned long base;
    int ret, irq;
    struct irq_data l_irq_data;


#ifndef FPGA_PLATFORM

#if (0 == REMOVEED_FOR_MT6582)
    msdc_hw_compara_enable(pdev->id);
#endif

#endif
#ifdef FPGA_PLATFORM
    volatile u16 l_val;
    l_val = sdr_read16(PWR_GPIO_EO);
    sdr_write16(PWR_GPIO_EO, (l_val | PWR_GPIO_L4_DIR | PWR_MASK_EN | PWR_MASK_VOL_33 | PWR_MASK_VOL_18));

    l_val = sdr_read16(PWR_GPIO_EO);
    printk("[%s]: pwr gpio dir = 0x%x\n", __func__, l_val);
#endif
    if (first_probe == 0) {
        first_probe ++;          

#ifndef FPGA_PLATFORM    
        // work aqround here. 
        enable_clock(MT_CG_PERI_MSDC30_0, "SD");          
        enable_clock(MT_CG_PERI_MSDC30_1, "SD");       
        enable_clock(MT_CG_PERI_MSDC30_2, "SD");       
        //enable_clock(PERI_MSDC3_PDN, "SD");       

        disable_clock(MT_CG_PERI_MSDC30_0, "SD");          
        disable_clock(MT_CG_PERI_MSDC30_1, "SD");       
        disable_clock(MT_CG_PERI_MSDC30_2, "SD");       
        //disable_clock(PERI_MSDC3_PDN, "SD");    
#endif     
        
    }

/*
    if (pdev->id == 0) {
#ifndef FPGA_PLATFORM
        msdc_sd0_power(1, VOL_3300);
#else
        msdc_sd0_power(1, 3300);
#endif

    } else if (pdev->id == 1) {
#ifndef FPGA_PLATFORM
        msdc_sd1_power(1, VOL_3300);      
#else
        msdc_sd1_power(1, 3300);      
#endif
    }*/
      
    /* Allocate MMC host for this device */
    mmc = mmc_alloc_host(sizeof(struct msdc_host), &pdev->dev);
    if (!mmc) return -ENOMEM;
#if 0
    workqueue = alloc_ordered_workqueue("kmmrmcd", 0);
    if (!workqueue){
        mmc_remove_host(mmc);
        return -ENOMEM;
    }
#endif
    hw   = (struct msdc_hw*)pdev->dev.platform_data;
    mem  = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    irq  = platform_get_irq(pdev, 0);
    base = IO_PHYS_TO_VIRT(mem->start);

    l_irq_data.irq = irq;

    BUG_ON((!hw) || (!mem) || (irq < 0));
    
    mem = request_mem_region(mem->start, mem->end - mem->start + 1, DRV_NAME);
    if (mem == NULL) {
        mmc_free_host(mmc);
        return -EBUSY;
    }

    /* Set host parameters to mmc */
    mmc->ops        = &mt_msdc_ops;
    mmc->f_min      = HOST_MIN_MCLK;
    mmc->f_max      = HOST_MAX_MCLK;    
    mmc->ocr_avail  = MSDC_OCR_AVAIL;
    
    /* For sd card: MSDC_SYS_SUSPEND | MSDC_WP_PIN_EN | MSDC_CD_PIN_EN | MSDC_REMOVABLE | MSDC_HIGHSPEED, 
       For sdio   : MSDC_EXT_SDIO_IRQ | MSDC_HIGHSPEED */
    if (hw->flags & MSDC_HIGHSPEED) {
        mmc->caps   = MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED;
    }
    if (hw->data_pins == 4) { 
        mmc->caps  |= MMC_CAP_4_BIT_DATA;
    } else if (hw->data_pins == 8) {
        mmc->caps  |= MMC_CAP_8_BIT_DATA | MMC_CAP_4_BIT_DATA;
    }
    if ((hw->flags & MSDC_SDIO_IRQ) || (hw->flags & MSDC_EXT_SDIO_IRQ))
        mmc->caps |= MMC_CAP_SDIO_IRQ;  /* yes for sdio */
    if(hw->flags & MSDC_UHS1){
        mmc->caps |= MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 |MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR104;    // | MMC_CAP_SET_XPC_180;
        mmc->caps2 |= MMC_CAP2_HS200_1_8V_SDR;
    }
    if(hw->flags & MSDC_DDR)
        mmc->caps |= MMC_CAP_UHS_DDR50|MMC_CAP_1_8V_DDR;
    if(!(hw->flags & MSDC_REMOVABLE))
        mmc->caps |= MMC_CAP_NONREMOVABLE;
    //else
        //mmc->caps2 |= MMC_CAP2_DETECT_ON_ERR;
    
#ifdef MTK_MSDC_USE_CMD23
    if (hw->host_function == MSDC_EMMC){
        mmc->caps |= MMC_CAP_ERASE | MMC_CAP_WAIT_WHILE_BUSY | MMC_CAP_CMD23;
    } else {
        mmc->caps |= MMC_CAP_ERASE | MMC_CAP_WAIT_WHILE_BUSY;
    }
#else
    mmc->caps |= MMC_CAP_ERASE | MMC_CAP_WAIT_WHILE_BUSY; 
#endif 

    /* MMC core transfer sizes tunable parameters */
    //mmc->max_hw_segs   = MAX_HW_SGMTS;
    mmc->max_segs   = MAX_HW_SGMTS;
    //mmc->max_phys_segs = MAX_PHY_SGMTS;
    if(hw->host_function == MSDC_SDIO)
        mmc->max_seg_size  = MAX_SGMT_SZ_SDIO;
    else
        mmc->max_seg_size  = MAX_SGMT_SZ;
    mmc->max_blk_size  = HOST_MAX_BLKSZ;
    mmc->max_req_size  = MAX_REQ_SZ; 
    mmc->max_blk_count = mmc->max_req_size;

    host = mmc_priv(mmc);
    host->hw             = hw;
    host->mmc            = mmc;  /* msdc_check_init_done() need */
    host->id             = pdev->id;
    host->error          = 0;
    host->irq            = irq;    
    host->base           = base;
    host->mclk           = 0;                   /* mclk: the request clock of mmc sub-system */
    host->hclk           = hclks[hw->clk_src];  /* hclk: clock of clock source to msdc controller */
    host->sclk           = 0;                   /* sclk: the really clock after divition */
    host->pm_state       = PMSG_RESUME;
    host->suspend        = 0;
    host->core_clkon     = 0;
    host->card_clkon     = 0;    
    host->clk_gate_count = 0;  
    host->core_power     = 0;
    host->power_mode     = MMC_POWER_OFF;
    host->power_control  = NULL;
    host->power_switch   = NULL;
#ifdef SDIO_ERROR_BYPASS      
    host->sdio_error = 0;
#endif    
#ifndef FPGA_PLATFORM
#if (0 == REMOVEED_FOR_MT6582)  
    if(host->id == 1)
        host->power_domain = MSDC_POWER_MC1;
    if(host->id == 2)
        host->power_domain = MSDC_POWER_MC2;
#endif
    msdc_set_host_power_control(host);
    if((host->hw->host_function == MSDC_SD) && (host->hw->flags & MSDC_CD_PIN_EN)){
        msdc_sd_power(host,1); //work around : hot-plug project SD card LDO alway on if no SD card insert
        msdc_sd_power(host,0);
    }
#endif
    if(host->hw->host_function == MSDC_EMMC && !(host->hw->flags & MSDC_UHS1))
       host->mmc->f_max = 50000000;
    //host->card_inserted = hw->flags & MSDC_REMOVABLE ? 0 : 1;
    host->timeout_ns = 0;
    host->timeout_clks = DEFAULT_DTOC * 1048576;

#ifndef MTK_MSDC_USE_CMD23
    if(host->hw->host_function != MSDC_SDIO)
        host->autocmd |= MSDC_AUTOCMD12;
    else
        host->autocmd &= ~MSDC_AUTOCMD12;
#else
    if (host->hw->host_function == MSDC_EMMC){
        host->autocmd &= ~MSDC_AUTOCMD12;

#if (1 == MSDC_USE_AUTO_CMD23)
        host->autocmd |= MSDC_AUTOCMD23;
#endif

    } else if (host->hw->host_function == MSDC_SD){
        host->autocmd |= MSDC_AUTOCMD12;
    } else {
        host->autocmd &= ~MSDC_AUTOCMD12;
    }
#endif /* end of MTK_MSDC_USE_CMD23 */

    host->mrq = NULL; 
    //init_MUTEX(&host->sem); /* we don't need to support multiple threads access */
   
    host->dma.used_gpd = 0;
    host->dma.used_bd = 0;

    /* using dma_alloc_coherent*/  /* todo: using 1, for all 4 slots */
    host->dma.gpd = dma_alloc_coherent(NULL, MAX_GPD_NUM * sizeof(gpd_t), &host->dma.gpd_addr, GFP_KERNEL); 
    host->dma.bd =  dma_alloc_coherent(NULL, MAX_BD_NUM  * sizeof(bd_t),  &host->dma.bd_addr,  GFP_KERNEL); 
    BUG_ON((!host->dma.gpd) || (!host->dma.bd));    
    msdc_init_gpd_bd(host, &host->dma);
    msdc_clock_src[host->id] = hw->clk_src;
    msdc_host_mode[host->id] = mmc->caps;
    msdc_host_mode2[host->id] = mmc->caps2;
    /*for emmc*/
    mtk_msdc_host[pdev->id] = host;
    host->write_timeout_uhs104 = 0;
    host->write_timeout_emmc = 0;
    host->read_timeout_uhs104 = 0;
    host->read_timeout_emmc = 0;
    host->sw_timeout = 0;
    host->tune = 0;
    host->ddr = 0;
    host->sd_cd_insert_work = 0;
    host->block_bad_card = 0;
    host->sd_30_busy = 0;
    msdc_reset_tmo_tune_counter(host, all_counter);
    msdc_reset_pwr_cycle_counter(host);
    if (is_card_sdio(host)|| (host->hw->flags & MSDC_SDIO_IRQ))
    {
        host->saved_para.suspend_flag = 0;
        host->saved_para.msdc_cfg = 0;
        host->saved_para.mode = 0;
        host->saved_para.div = 0;
        host->saved_para.sdc_cfg = 0;
        host->saved_para.iocon = 0;
        host->saved_para.ddr = 0;
        host->saved_para.hz = 0;
        host->saved_para.cmd_resp_ta_cntr = 0; //for SDIO 3.0
        host->saved_para.wrdat_crc_ta_cntr = 0; //for SDIO 3.0
        host->saved_para.int_dat_latch_ck_sel = 0; //for SDIO 3.0
        host->saved_para.ckgen_msdc_dly_sel = 0; //for SDIO 3.0
        host->saved_para.inten_sdio_irq = 0; //default disable
    }
    tasklet_init(&host->card_tasklet, msdc_tasklet_card, (ulong)host);
#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
    atomic_set(&host->ot_done, 1);
	atomic_set(&host->sdio_stopping, 0);
    host->ot_work.host = host;
    host->ot_work.chg_volt = 0;
    atomic_set(&host->ot_work.ot_disable, 0);
	atomic_set(&host->ot_work.autok_done, 0);
  #ifdef MTK_SDIO30_DETECT_THERMAL
    host->pre_temper = -127000;
    host->ot_period_check_start = 0;
    init_timer(&host->ot_timer);
    host->ot_timer.function	= start_online_tuning; 
    host->ot_timer.data		= (unsigned long)host;
  #endif // MTK_SDIO30_DETECT_THERMAL
#endif // MTK_SDIO30_ONLINE_TUNING_SUPPORT
    //INIT_DELAYED_WORK(&host->remove_card, msdc_remove_card);
    spin_lock_init(&host->lock);
    spin_lock_init(&host->clk_gate_lock);
    spin_lock_init(&host->remove_bad_card);
    /* init dynamtic timer */
    init_timer(&host->timer);
    //host->timer.expires = jiffies + HZ;
    host->timer.function = msdc_timer_pm; 
    host->timer.data = (unsigned long)host;
    
    msdc_init_hw(host);

    //mt65xx_irq_set_sens(irq, MT65xx_EDGE_SENSITIVE);
    //mt65xx_irq_set_polarity(irq, MT65xx_POLARITY_LOW);
    ret = request_irq((unsigned int)irq, msdc_irq, IRQF_TRIGGER_LOW, DRV_NAME, host);
    if (ret) goto release;
    //mt65xx_irq_unmask(&l_irq_data);
    //enable_irq(irq);
    
    if (hw->flags & MSDC_CD_PIN_EN) { /* not set for sdio */
        if (hw->request_cd_eirq) { /* not set for MT6589 */
            hw->request_cd_eirq(msdc_eirq_cd, (void*)host); /* msdc_eirq_cd will not be used! */
        }
    }

    if (hw->request_sdio_eirq) /* set to combo_sdio_request_eirq() for WIFI */
        hw->request_sdio_eirq(msdc_eirq_sdio, (void*)host); /* msdc_eirq_sdio() will be called when EIRQ */

    if (hw->register_pm) {/* yes for sdio */
        hw->register_pm(msdc_pm, (void*)host);  /* combo_sdio_register_pm() */
        if(hw->flags & MSDC_SYS_SUSPEND) { /* will not set for WIFI */
            ERR_MSG("MSDC_SYS_SUSPEND and register_pm both set");
        }
        mmc->pm_flags |= MMC_PM_IGNORE_PM_NOTIFY; /* pm not controlled by system but by client. */
    }
    
    platform_set_drvdata(pdev, mmc);

#if (0 == REMOVEED_FOR_MT6582)
    msdc_register_emi_mpu_callback(host->id);
#endif

#ifdef CONFIG_MTK_HIBERNATION
    if(pdev->id == 1)
       register_swsusp_restore_noirq_func(ID_M_MSDC, msdc_drv_pm_restore_noirq, &(pdev->dev));
#endif

    /* Config card detection pin and enable interrupts */
    if (hw->flags & MSDC_CD_PIN_EN) {  /* set for card */
        msdc_enable_cd_irq(host, 1);
    } else {
        msdc_enable_cd_irq(host, 0);
    }  

    ret = mmc_add_host(mmc);
    if (ret) goto free_irq;
    if(host->hw->flags & MSDC_SDIO_IRQ)
    {
        ghost = host;
        sdr_set_bits(SDC_CFG, SDC_CFG_SDIOIDE); //enable sdio detection
    }
    //if (hw->flags & MSDC_CD_PIN_EN)
    host->sd_cd_insert_work = 1;

#ifdef DEBUG_TEST_FOR_SIGNAL
    /* use EINT1 for trigger signal */
    /* need to remove gpio warning log at 
     * mediatek/kernel/include/mach/mt_gpio_core.h 
     * mediatek/platform/{project}/kernel/drivers/gpio/mt_gpio_affix.c */
    mt_set_gpio_mode(1, GPIO_MODE_00);    
    mt_set_gpio_dir(1, GPIO_DIR_OUT); 
    mt_set_gpio_pull_enable(1, 1);

    mt_set_gpio_out(1, 0); //1-high, 0-low
#endif    

    return 0;

free_irq:
    free_irq(irq, host);
release:
    platform_set_drvdata(pdev, NULL);
    msdc_deinit_hw(host);

    tasklet_kill(&host->card_tasklet);
#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
  #ifdef MTK_SDIO30_DETECT_THERMAL
    del_timer(&host->ot_timer);
  #endif // MTK_SDIO30_DETECT_THERMAL
#endif // MTK_SDIO30_ONLINE_TUNING_SUPPORT
    if (mem)
        release_mem_region(mem->start, mem->end - mem->start + 1);

    mmc_free_host(mmc);

    return ret;
}

/* 4 device share one driver, using "drvdata" to show difference */
static int msdc_drv_remove(struct platform_device *pdev)
{
    struct mmc_host *mmc;
    struct msdc_host *host;
    struct resource *mem;

    mmc = platform_get_drvdata(pdev);
    BUG_ON(!mmc);
    
    host = mmc_priv(mmc);   
    BUG_ON(!host);

    ERR_MSG("removed !!!");

    platform_set_drvdata(pdev, NULL);
    mmc_remove_host(host->mmc);
    msdc_deinit_hw(host);

    tasklet_kill(&host->card_tasklet);
#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
  #ifdef MTK_SDIO30_DETECT_THERMAL
    del_timer(&host->ot_timer);
  #endif // MTK_SDIO30_DETECT_THERMAL
#endif // MTK_SDIO30_ONLINE_TUNING_SUPPORT
    free_irq(host->irq, host);

    dma_free_coherent(NULL, MAX_GPD_NUM * sizeof(gpd_t), host->dma.gpd, host->dma.gpd_addr);
    dma_free_coherent(NULL, MAX_BD_NUM  * sizeof(bd_t),  host->dma.bd,  host->dma.bd_addr);

    mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);

    if (mem)
        release_mem_region(mem->start, mem->end - mem->start + 1);

    mmc_free_host(host->mmc);

    return 0;
}

/* Fix me: Power Flow */
#ifdef CONFIG_PM
static int msdc_drv_suspend(struct platform_device *pdev, pm_message_t state)
{
    int ret = 0;
    struct mmc_host *mmc = platform_get_drvdata(pdev);
    struct msdc_host *host = mmc_priv(mmc);
    u32  base = host->base;

    if (mmc && state.event == PM_EVENT_SUSPEND && (host->hw->flags & MSDC_SYS_SUSPEND)) { /* will set for card */
        msdc_pm(state, (void*)host);
    }

    // WIFI slot should be off when enter suspend
    if (mmc && state.event == PM_EVENT_SUSPEND && (!(host->hw->flags & MSDC_SYS_SUSPEND))) {
        msdc_suspend_clock(host);
        if(host->error == -EBUSY){
            ret = host->error;
            host->error = 0;
        }
    }

    if (is_card_sdio(host) || (host->hw->flags & MSDC_SDIO_IRQ)) 
    {
        if (host->saved_para.suspend_flag==0)
        {
            if(host->clk_gate_count > 0){
              host->error = 0;
              return -EBUSY;
            }
            host->saved_para.hz = host->mclk;
            if (host->saved_para.hz)
            {
                host->saved_para.suspend_flag = 1;
                mb();
                msdc_ungate_clock(host);
                sdr_get_field(MSDC_CFG, MSDC_CFG_CKMOD, host->saved_para.mode);                
                sdr_get_field(MSDC_CFG, MSDC_CFG_CKDIV, host->saved_para.div); 
                sdr_get_field(MSDC_PATCH_BIT0, MSDC_INT_DAT_LATCH_CK_SEL,  host->saved_para.int_dat_latch_ck_sel);  //for SDIO 3.0
                sdr_get_field(MSDC_PATCH_BIT0, MSDC_CKGEN_MSDC_DLY_SEL,    host->saved_para.ckgen_msdc_dly_sel); //for SDIO 3.0
                sdr_get_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_CMD_RSP,    host->saved_para.cmd_resp_ta_cntr); //for SDIO 3.0
                sdr_get_field(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_WRDAT_CRCS, host->saved_para.wrdat_crc_ta_cntr); //for SDIO 3.0
                sdr_get_field(MSDC_INTEN, MSDC_INT_SDIOIRQ, host->saved_para.inten_sdio_irq); //get INTEN status for SDIO                   
                host->saved_para.msdc_cfg = sdr_read32(MSDC_CFG);
                host->saved_para.ddly0 = sdr_read32(MSDC_DAT_RDDLY0);
                host->saved_para.pad_tune = sdr_read32(MSDC_PAD_TUNE);
                host->saved_para.sdc_cfg = sdr_read32(SDC_CFG);
                host->saved_para.iocon = sdr_read32(MSDC_IOCON);
                host->saved_para.ddr = host->ddr;
                msdc_gate_clock(host, 0);
                if(host->error == -EBUSY){
                    ret = host->error;
                    host->error = 0;
                }
            }
            ERR_MSG("msdc suspend cur_cfg=%x, save_cfg=%x, cur_hz=%d, save_hz=%d", sdr_read32(MSDC_CFG), host->saved_para.msdc_cfg, host->mclk, host->saved_para.hz);
        }
    }
    return ret;
}

static int msdc_drv_resume(struct platform_device *pdev)
{
    int ret = 0;
    struct mmc_host *mmc = platform_get_drvdata(pdev);
    struct msdc_host *host = mmc_priv(mmc);
    struct pm_message state;

    if(host->hw->flags & MSDC_SDIO_IRQ)
    {
        printk("msdc msdc_drv_resume \n");
    }
    state.event = PM_EVENT_RESUME;
    if (mmc && (host->hw->flags & MSDC_SYS_SUSPEND)) {/* will set for card */
        msdc_pm(state, (void*)host);
    }
    
    /* This mean WIFI not controller by PM */
    
    return ret;
}
#endif

static struct platform_driver mt_msdc_driver = {
    .probe   = msdc_drv_probe,
    .remove  = msdc_drv_remove,
#ifdef CONFIG_PM
    .suspend = msdc_drv_suspend,
    .resume  = msdc_drv_resume,
#endif
    .driver  = {
        .name  = DRV_NAME,
        .owner = THIS_MODULE,
    },
};

/*--------------------------------------------------------------------------*/
/* module init/exit                   */
/*--------------------------------------------------------------------------*/
static int __init mt_msdc_init(void)
{
    int ret;

    ret = platform_driver_register(&mt_msdc_driver);
    if (ret) {
        printk(KERN_ERR DRV_NAME ": Can't register driver");
        return ret;
    }

#if defined(MTK_EMMC_SUPPORT) && defined(CONFIG_PROC_FS)
    if ((proc_emmc = create_proc_entry( "emmc", 0, NULL )))
        proc_emmc->read_proc = emmc_read_proc;
#endif /* MTK_EMMC_SUPPORT && CONFIG_PROC_FS */

    printk(KERN_INFO DRV_NAME ": MediaTek MSDC Driver\n");

    msdc_debug_proc_init();
    msdc_init_dma_latest_address();

#ifdef MTK_SDIOAUTOK_SUPPORT    
    osc_value = mtk_wdt_swsysret_config(0x30000000, 0);        // Get io_ring for auto-K usage
#endif
    
    return 0;
}

static void __exit mt_msdc_exit(void)
{
    platform_driver_unregister(&mt_msdc_driver);

#ifdef CONFIG_MTK_HIBERNATION
    unregister_swsusp_restore_noirq_func(ID_M_MSDC);
#endif
}

module_init(mt_msdc_init);
module_exit(mt_msdc_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek SD/MMC Card Driver");
#ifdef MTK_EMMC_SUPPORT
EXPORT_SYMBOL(ext_csd);
EXPORT_SYMBOL(g_emmc_mode_switch);
#endif
EXPORT_SYMBOL(mtk_msdc_host);
