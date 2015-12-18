/* drivers/hwmon/mt6516/amit/epl2182.c - EPL2182 ALS/PS driver
 *
 * Author: MingHsien Hsieh <minghsien.hsieh@mediatek.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/** VERSION: 1.05**/

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>

#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include <asm/io.h>
#include <cust_eint.h>
#include <cust_alsps.h>
#include <linux/hwmsen_helper.h>
#include "epl2182.h"

#include <alsps.h>
#ifdef MT6516
#include <mach/mt6516_devs.h>
#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_gpio.h>
#include <mach/mt6516_pll.h>
#endif

#ifdef MT6573
#include <mach/mt6573_devs.h>
#include <mach/mt6573_typedefs.h>
#include <mach/mt6573_gpio.h>
#include <mach/mt6573_pll.h>
#endif

#ifdef MT6513
#include <mach/mt6513_devs.h>
#include <mach/mt6513_typedefs.h>
#include <mach/mt6513_gpio.h>
#include <mach/mt6513_pll.h>
#endif

#ifdef MT6575
#include <mach/mt6575_devs.h>
#include <mach/mt6575_typedefs.h>
#include <mach/mt6575_gpio.h>
#include <mach/mt6575_pm_ldo.h>
#endif

#ifdef MT6577
#include <mach/mt6577_devs.h>
#include <mach/mt6577_typedefs.h>
#include <mach/mt6577_gpio.h>
#include <mach/mt6577_pm_ldo.h>
#endif

#ifdef MT6589
#include <mach/devs.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>
#endif

#ifdef MT6572
#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>
#endif

#ifdef MT6582
#include <mach/devs.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>
#endif

#ifdef MT6592
#include <mach/devs.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>
#endif

//#include <Mt6575.h>
//bob.chen add begin
//add for fix resume issue
#include <linux/earlysuspend.h>
#include <linux/wakelock.h>
//add for fix resume issue end
//bob.chen add end


/******************************************************************************
 * extern functions
*******************************************************************************/
#ifdef MT6516
extern void MT6516_EINTIRQUnmask(unsigned int line);
extern void MT6516_EINTIRQMask(unsigned int line);
extern void MT6516_EINT_Set_Polarity(kal_uint8 eintno, kal_bool ACT_Polarity);
extern void MT6516_EINT_Set_HW_Debounce(kal_uint8 eintno, kal_uint32 ms);
extern kal_uint32 MT6516_EINT_Set_Sensitivity(kal_uint8 eintno, kal_bool sens);
extern void MT6516_EINT_Registration(kal_uint8 eintno, kal_bool Dbounce_En,
                                     kal_bool ACT_Polarity, void (EINT_FUNC_PTR)(void),
                                     kal_bool auto_umask);
#endif


#ifdef MT6513
extern void mt65xx_eint_unmask(unsigned int line);
extern void mt65xx_eint_mask(unsigned int line);
extern void mt65xx_eint_set_polarity(kal_uint8 eintno, kal_bool ACT_Polarity);
extern void mt65xx_eint_set_hw_debounce(kal_uint8 eintno, kal_uint32 ms);
extern kal_uint32 mt65xx_eint_set_sens(kal_uint8 eintno, kal_bool sens);
extern void mt65xx_eint_registration(kal_uint8 eintno, kal_bool Dbounce_En,
                                     kal_bool ACT_Polarity, void (EINT_FUNC_PTR)(void),
                                     kal_bool auto_umask);
#endif

#ifdef MT6573
extern void mt65xx_eint_unmask(unsigned int line);
extern void mt65xx_eint_mask(unsigned int line);
extern void mt65xx_eint_set_polarity(kal_uint8 eintno, kal_bool ACT_Polarity);
extern void mt65xx_eint_set_hw_debounce(kal_uint8 eintno, kal_uint32 ms);
extern kal_uint32 mt65xx_eint_set_sens(kal_uint8 eintno, kal_bool sens);
extern void mt65xx_eint_registration(kal_uint8 eintno, kal_bool Dbounce_En,
                                     kal_bool ACT_Polarity, void (EINT_FUNC_PTR)(void),
                                     kal_bool auto_umask);
#endif


#ifdef MT6575
extern void mt65xx_eint_unmask(unsigned int line);
extern void mt65xx_eint_mask(unsigned int line);
extern void mt65xx_eint_set_polarity(kal_uint8 eintno, kal_bool ACT_Polarity);
extern void mt65xx_eint_set_hw_debounce(kal_uint8 eintno, kal_uint32 ms);
extern kal_uint32 mt65xx_eint_set_sens(kal_uint8 eintno, kal_bool sens);
extern void mt65xx_eint_registration(kal_uint8 eintno, kal_bool Dbounce_En,
                                     kal_bool ACT_Polarity, void (EINT_FUNC_PTR)(void),
                                     kal_bool auto_umask);
#endif

#ifdef MT6577
extern void mt65xx_eint_unmask(unsigned int line);
extern void mt65xx_eint_mask(unsigned int line);
extern void mt65xx_eint_set_polarity(kal_uint8 eintno, kal_bool ACT_Polarity);
extern void mt65xx_eint_set_hw_debounce(kal_uint8 eintno, kal_uint32 ms);
extern kal_uint32 mt65xx_eint_set_sens(kal_uint8 eintno, kal_bool sens);
extern void mt65xx_eint_registration(kal_uint8 eintno, kal_bool Dbounce_En,
                                     kal_bool ACT_Polarity, void (EINT_FUNC_PTR)(void),
                                     kal_bool auto_umask);
#endif

#ifdef MT6589
extern void mt65xx_eint_unmask(unsigned int line);
extern void mt65xx_eint_mask(unsigned int line);
extern void mt65xx_eint_set_polarity(kal_uint8 eintno, kal_bool ACT_Polarity);
extern void mt65xx_eint_set_hw_debounce(kal_uint8 eintno, kal_uint32 ms);
extern kal_uint32 mt65xx_eint_set_sens(kal_uint8 eintno, kal_bool sens);
extern void mt65xx_eint_registration(kal_uint8 eintno, kal_bool Dbounce_En,
                                     kal_bool ACT_Polarity, void (EINT_FUNC_PTR)(void),
                                     kal_bool auto_umask);
#endif

#ifdef MT6572
extern void mt65xx_eint_unmask(unsigned int line);
extern void mt65xx_eint_mask(unsigned int line);
extern void mt65xx_eint_set_polarity(kal_uint8 eintno, kal_bool ACT_Polarity);
extern void mt65xx_eint_set_hw_debounce(kal_uint8 eintno, kal_uint32 ms);
extern kal_uint32 mt65xx_eint_set_sens(kal_uint8 eintno, kal_bool sens);
extern void mt65xx_eint_registration(kal_uint8 eintno, kal_bool Dbounce_En,
                                     kal_bool ACT_Polarity, void (EINT_FUNC_PTR)(void),
                                     kal_bool auto_umask);
#endif

#ifdef MT6582
extern void mt_eint_unmask(unsigned int line);
extern void mt_eint_mask(unsigned int line);
extern void mt_eint_set_polarity(kal_uint8 eintno, kal_bool ACT_Polarity);
extern void mt_eint_set_hw_debounce(kal_uint8 eintno, kal_uint32 ms);
extern kal_uint32 mt_eint_set_sens(kal_uint8 eintno, kal_bool sens);
//extern void mt_eint_registration(kal_uint8 eintno, kal_bool Dbounce_En,
//                                     kal_bool ACT_Polarity, void (EINT_FUNC_PTR)(void),
//                                     kal_bool auto_umask);
extern void mt_eint_registration(unsigned int eint_num, unsigned int flow, void (EINT_FUNC_PTR)(void), unsigned int is_auto_umask);

#endif


#ifdef MT6592
extern void mt_eint_unmask(unsigned int line);
extern void mt_eint_mask(unsigned int line);
extern void mt_eint_set_polarity(kal_uint8 eintno, kal_bool ACT_Polarity);
extern void mt_eint_set_hw_debounce(kal_uint8 eintno, kal_uint32 ms);
extern kal_uint32 mt_eint_set_sens(kal_uint8 eintno, kal_bool sens);
//extern void mt_eint_registration(kal_uint8 eintno, kal_bool Dbounce_En,
//                                     kal_bool ACT_Polarity, void (EINT_FUNC_PTR)(void),
//                                     kal_bool auto_umask);
extern void mt_eint_registration(unsigned int eint_num, unsigned int flow, void (EINT_FUNC_PTR)(void), unsigned int is_auto_umask);

#endif

/*-------------------------MT6516&MT6573 define-------------------------------*/
#ifdef MT6516
#define POWER_NONE_MACRO MT6516_POWER_NONE
#endif
#ifdef MT6573
#define POWER_NONE_MACRO MT65XX_POWER_NONE
#endif
#ifdef MT6513
#define POWER_NONE_MACRO MT65XX_POWER_NONE
#endif
#ifdef MT6575
#define POWER_NONE_MACRO MT65XX_POWER_NONE
#endif
#ifdef MT6577
#define POWER_NONE_MACRO MT65XX_POWER_NONE
#endif
#ifdef MT6589
#define POWER_NONE_MACRO MT65XX_POWER_NONE
#endif
#ifdef MT6582
#define POWER_NONE_MACRO MT65XX_POWER_NONE
#endif
#ifdef MT6572
#define POWER_NONE_MACRO MT65XX_POWER_NONE
#endif
/******************************************************************************
 * configuration
*******************************************************************************/
#define LUX_PER_COUNT		1100              // 1100 = 1.1 * 1000
#define PS_DRIVE				EPL_DRIVE_120MA
#define POLLING_MODE_HS		0

#define PS_GES 0
#define ELAN_WRITE_CALI 0 // 1
#define QUEUE_RUN        1 //1: epl2182_polling_work always run
#define ALS_FACTORY_MODE  0 //for special customer

#define DYN_ENABLE			1

static int PS_INTT 				= EPL_INTT_PS_80; //4;
static int ALS_INTT 			= 7;

//static int HS_INTT 				= 0; // reset when enable

#define HS_INTT_CENTER			EPL_INTT_PS_80 //EPL_INTT_PS_48
static int HS_INTT 				= HS_INTT_CENTER;


#define PS_DELAY 			15
#define ALS_DELAY 			55
#define HS_DELAY 			30

#if DYN_ENABLE
#if defined(YK868_CUSTOMER_ZHONGPINSHANG_FWVGA_K08_50)
#define DYN_H_OFFSET 	 	1000 //600
#define DYN_L_OFFSET		300
#else
#define DYN_H_OFFSET 	 	600 //1500 //600
#define DYN_L_OFFSET		300 //500  //400
#endif
#define DYN_PS_CONDITION	30000
#endif

#define KEYCODE_LEFT			KEY_LEFTALT
/******************************************************************************
*******************************************************************************/

#define TXBYTES 				2
#define RXBYTES 				2

#define PACKAGE_SIZE 		2
#define I2C_RETRY_COUNT 	10

#if ELAN_WRITE_CALI
typedef struct _epl_ps_als_factory
{
    bool cal_file_exist;
    bool cal_finished;
    u16 ps_cal_h;
    u16 ps_cal_l;
    char s1[16];
    char s2[16];
};
#endif

#if ALS_FACTORY_MODE
bool    als_factory_flag = false;
#define ALS_FACTORY_DELAY 			25 //2 cycle, 21.026ms
#endif

#define EPL2182_DEV_NAME     "EPL2182"
// for heart rate
static struct mutex sensor_mutex;
static bool change_int_time = false;
static int hs_count=0;
static int hs_idx=0;
static int show_hs_raws_flag=0;
static int hs_als_flag=0;

typedef struct _epl_raw_data
{
    u8 raw_bytes[PACKAGE_SIZE];
    u16 renvo;
    u16 ps_state;
#if 1
    u16 ps_last_state;
#endif
#if DYN_ENABLE
	u16 ps_condition;
	u16 ps_min_raw;
	u16 ps_sta;
	u16 ps_dyn_high;
	u16 ps_dyn_low;
	bool ps_dny_ini_lock;
#endif

    u16 ps_raw;
    u16 als_ch0_raw;
    u16 als_ch1_raw;
    u16 als_lux;
    u16 hs_data[200];
    bool ps_suspend_flag;
#if ELAN_WRITE_CALI
    struct _epl_ps_als_factory ps_als_factory;
#endif
} epl_raw_data;

#if ELAN_WRITE_CALI
#define PS_CAL_FILE_PATH	"/data/data/com.eminent.ps.calibration/xtalk_cal"  //PS Calbration file path
static int PS_h_offset = 3000;
static int PS_l_offset = 2000;
static int PS_MAX_XTALK = 50000;
#endif

/*----------------------------------------------------------------------------*/
#define APS_TAG                  "[ALS/PS] "

//#define ALSPS_DEBUG
#ifdef ALSPS_DEBUG
#define APS_FUN(f)               printk(APS_TAG"%s\n", __FUNCTION__)
#define APS_ERR(fmt, args...)    printk(APS_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define APS_LOG(fmt, args...)    printk(APS_TAG fmt, ##args)
#define APS_DBG(fmt, args...)    printk(fmt, ##args)
#else
#define APS_FUN(f)               printk(KERN_INFO APS_TAG"%s\n", __FUNCTION__)
#define APS_ERR(fmt, args...)    printk(KERN_ERR  APS_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define APS_LOG(fmt, args...)    printk(KERN_INFO APS_TAG fmt, ##args)
#define APS_DBG(fmt, args...)    printk(KERN_INFO fmt, ##args)
#endif
#define FTM_CUST_ALSPS "/data/epl2182"

/*----------------------------------------------------------------------------*/
static struct i2c_client *epl2182_i2c_client = NULL;

static DEFINE_MUTEX(EPL2182_mutex);
/*----------------------------------------------------------------------------*/
static const struct i2c_device_id epl2182_i2c_id[] = {{"EPL2182",0},{}};
static struct i2c_board_info __initdata i2c_EPL2182= { I2C_BOARD_INFO("EPL2182", (0X92>>1))};
/*the adapter id & i2c address will be available in customization*/
//static unsigned short epl2182_force[] = {0x00, 0x92, I2C_CLIENT_END, I2C_CLIENT_END};
//static const unsigned short *const epl2182_forces[] = { epl2182_force, NULL };
//static struct i2c_client_address_data epl2182_addr_data = { .forces = epl2182_forces,};


/*----------------------------------------------------------------------------*/
static int epl2182_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int epl2182_i2c_remove(struct i2c_client *client);
static int epl2182_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);

/*----------------------------------------------------------------------------*/
static int epl2182_i2c_suspend(struct i2c_client *client, pm_message_t msg);
static int epl2182_i2c_resume(struct i2c_client *client);

static void epl2182_eint_func(void);
static int set_psensor_intr_threshold(uint16_t low_thd, uint16_t high_thd);
void epl2182_restart_polling(void);
int epl2182_ps_operate(void* self, uint32_t command, void* buff_in, int size_in,
                       void* buff_out, int size_out, int* actualout);
static int set_psensor_threshold(struct i2c_client *client);

extern struct alsps_hw *EPL2182_get_cust_alsps_hw(void);
static struct epl2182_priv *g_epl2182_ptr = NULL;


#define CKT_HALL_SWITCH_SUPPORT 0
 #if CKT_HALL_SWITCH_SUPPORT
extern int g_is_calling;
 #endif

static unsigned int ps_threshold_high = 550;
static unsigned int ps_threshold_low = 510;
#define Calibrate_num 10

#define EPL_LT_N_CT	150
#define EPL_HT_N_CT	450

/*----------------------------------------------------------------------------*/
typedef enum
{
    CMC_TRC_ALS_DATA 	= 0x0001,
    CMC_TRC_PS_DATA 	= 0X0002,
    CMC_TRC_EINT    		= 0x0004,
    CMC_TRC_IOCTL   		= 0x0008,
    CMC_TRC_I2C     		= 0x0010,
    CMC_TRC_CVT_ALS 	= 0x0020,
    CMC_TRC_CVT_PS  		= 0x0040,
    CMC_TRC_DEBUG   		= 0x0800,
} CMC_TRC;

/*----------------------------------------------------------------------------*/
typedef enum
{
    CMC_BIT_ALS    = 1,
    CMC_BIT_PS     = 2,
    CMC_BIT_HS  		= 8,
} CMC_BIT;

/*----------------------------------------------------------------------------*/
struct epl2182_i2c_addr      /*define a series of i2c slave address*/
{
    u8  write_addr;
    u8  ps_thd;     /*PS INT threshold*/
};

/*----------------------------------------------------------------------------*/
struct epl2182_priv
{
    struct alsps_hw  *hw;
    struct i2c_client *client;
    struct delayed_work  eint_work;
    struct delayed_work  polling_work;
    struct input_dev *input_dev;

    /*i2c address group*/
    struct epl2182_i2c_addr  addr;

    int 		polling_mode_hs;
    int		ir_type;

    /*misc*/
    atomic_t    trace;
    atomic_t    als_suspend;
    atomic_t    ps_suspend;
    atomic_t	hs_suspend;

    /*data*/
    u16		lux_per_count;
    ulong       enable;         /*record HAL enalbe status*/
    ulong       pending_intr;   /*pending interrupt*/

    /*data*/
    u16         als_level_num;
    u16         als_value_num;
    u32         als_level[C_CUST_ALS_LEVEL-1];
    u32         als_value[C_CUST_ALS_LEVEL];

    int			ps_cali;	
    atomic_t	ps_thd_val_high;	 /*the cmd value can't be read, stored in ram*/	
    atomic_t	ps_thd_val_low; 	/*the cmd value can't be read, stored in ram*/

    /*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend    early_drv;
#endif
};



static int  epl2182_local_init(void);
static int epl_remove(void);
static struct sensor_init_info epl2182_init_info = {		
	.name = "EPL2182",		
	.init =epl2182_local_init,		
	.uninit = epl_remove,
};
static int epl2182_init_flag = -1;

/*----------------------------------------------------------------------------*/
static struct i2c_driver epl2182_i2c_driver =
{
    .probe      	= epl2182_i2c_probe,
    .remove     = epl2182_i2c_remove,
    .detect     	= epl2182_i2c_detect,
    .suspend    = epl2182_i2c_suspend,
    .resume     = epl2182_i2c_resume,
    .id_table   	= epl2182_i2c_id,
    //.address_data = &epl2182_addr_data,
    .driver = {
        //.owner          = THIS_MODULE,
        .name           = EPL2182_DEV_NAME,
    },
};


static struct epl2182_priv *epl2182_obj = NULL;
static struct platform_driver epl2182_alsps_driver;
static struct wake_lock ps_lock;
static epl_raw_data	gRawData;


/*
//====================I2C write operation===============//
//regaddr: ELAN epl2182 Register Address.
//bytecount: How many bytes to be written to epl2182 register via i2c bus.
//txbyte: I2C bus transmit byte(s). Single byte(0X01) transmit only slave address.
//data: setting value.
//
// Example: If you want to write single byte to 0x1D register address, show below
//	      elan_epl2182_I2C_Write(client,0x1D,0x01,0X02,0xff);
//
*/
static int elan_epl2182_I2C_Write(struct i2c_client *client, uint8_t regaddr, uint8_t bytecount, uint8_t txbyte, uint8_t data)
{
    uint8_t buffer[2];
    int ret = 0;
    int retry;

    mutex_lock(&EPL2182_mutex);
    buffer[0] = (regaddr<<3) | bytecount ;
    buffer[1] = data;

    for(retry = 0; retry < I2C_RETRY_COUNT; retry++)
    {
        ret = i2c_master_send(client, buffer, txbyte);

        if (ret == txbyte)
        {
            break;
        }

        APS_ERR("i2c write error,TXBYTES %d\n",ret);
        mdelay(10);
    }


    if(retry>=I2C_RETRY_COUNT)
    {
        APS_ERR("i2c write retry over %d\n", I2C_RETRY_COUNT);
        mutex_unlock(&EPL2182_mutex);
        return -EINVAL;
    }

    mutex_unlock(&EPL2182_mutex);
    return ret;
}




/*
//====================I2C read operation===============//
*/
static int elan_epl2182_I2C_Read(struct i2c_client *client)
{
    uint8_t buffer[RXBYTES];
    int ret = 0, i =0;
    int retry;

    mutex_lock(&EPL2182_mutex);
    for(retry = 0; retry < I2C_RETRY_COUNT; retry++)
    {
        ret = i2c_master_recv(client, buffer, RXBYTES);

        if (ret == RXBYTES)
            break;

        APS_ERR("i2c read error,RXBYTES %d\r\n",ret);
        mdelay(10);
    }

    if(retry>=I2C_RETRY_COUNT)
    {
        APS_ERR("i2c read retry over %d\n", I2C_RETRY_COUNT);
        mutex_unlock(&EPL2182_mutex);
        return -EINVAL;
    }

    for(i=0; i<PACKAGE_SIZE; i++)
        gRawData.raw_bytes[i] = buffer[i];

    mutex_unlock(&EPL2182_mutex);
    return ret;
}
static int elan_epl2182_I2C_Read_long(struct i2c_client *client, int bytecount)
{
    uint8_t buffer[bytecount];
    int ret = 0, i =0;
    int retry;

    mutex_lock(&EPL2182_mutex);
    for(retry = 0; retry < I2C_RETRY_COUNT; retry++)
    {
        ret = i2c_master_recv(client, buffer, bytecount);

        if (ret == bytecount)
            break;

        APS_ERR("i2c read error,RXBYTES %d\r\n",ret);
        mdelay(10);
    }

    if(retry>=I2C_RETRY_COUNT)
    {
        APS_ERR("i2c read retry over %d\n", I2C_RETRY_COUNT);
        mutex_unlock(&EPL2182_mutex);
        return -EINVAL;
    }

    for(i=0; i<bytecount; i++)
        gRawData.raw_bytes[i] = buffer[i];

    mutex_unlock(&EPL2182_mutex);
    return ret;
}

static int elan_calibration_atoi(char* s)
{
    int num=0,flag=0;
    int i=0;
    //printk("[ELAN] %s\n", __func__);
    for(i=0; i<=strlen(s); i++)
    {
        if(s[i] >= '0' && s[i] <= '9')
            num = num * 10 + s[i] -'0';
        else if(s[0] == '-' && i==0)
            flag =1;
        else
            break;
    }
    if(flag == 1)
        num = num * -1;
    return num;
}

#if ELAN_WRITE_CALI
static int write_factory_calibration(struct epl2182_priv *epl_data, char* ps_data, int ps_cal_len)
{
    struct file *fp_cal;

	mm_segment_t fs;
	loff_t pos;

	APS_FUN();
    pos = 0;

	fp_cal = filp_open(PS_CAL_FILE_PATH, O_CREAT|O_RDWR|O_TRUNC, 0777);
	if (IS_ERR(fp_cal))
	{
		APS_ERR("[ELAN]create file error\n");
		return -1;
	}

    fs = get_fs();
	set_fs(KERNEL_DS);

	vfs_write(fp_cal, ps_data, ps_cal_len, &pos);

    filp_close(fp_cal, NULL);

	set_fs(fs);

	return 0;
}

static bool read_factory_calibration(struct epl2182_priv *epl_data)
{
	struct i2c_client *client = epl_data->client;
	struct file *fp;
	mm_segment_t fs;
	loff_t pos;
	char buffer[100]= {0};
	if(gRawData.ps_als_factory.cal_file_exist == 1)
	{
		fp = filp_open(PS_CAL_FILE_PATH, O_RDWR, S_IRUSR);

		if (IS_ERR(fp))
		{
			APS_ERR("NO PS calibration file(%d)\n", (int)IS_ERR(fp));
			gRawData.ps_als_factory.cal_file_exist =  0;
			return -EINVAL;
		}
		else
		{
		    int ps_hthr = 0, ps_lthr = 0;
			pos = 0;
			fs = get_fs();
			set_fs(KERNEL_DS);
			vfs_read(fp, buffer, sizeof(buffer), &pos);
			filp_close(fp, NULL);

			sscanf(buffer, "%d,%d", &ps_hthr, &ps_lthr);
			gRawData.ps_als_factory.ps_cal_h = ps_hthr;
			gRawData.ps_als_factory.ps_cal_l = ps_lthr;
			set_fs(fs);

			epl_data->hw->ps_threshold_high = gRawData.ps_als_factory.ps_cal_h;
		    epl_data->hw->ps_threshold_low = gRawData.ps_als_factory.ps_cal_l;

		    //atomic_set(&epl_data->ps_thd_val_high, epl_data->hw->ps_threshold_high);
	        //atomic_set(&epl_data->ps_thd_val_low, epl_data->hw->ps_threshold_low);
		}

		gRawData.ps_als_factory.cal_finished = 1;
	}
	return 0;
}

static int elan_epl2182_psensor_enable(struct epl2182_priv *epl_data, int enable);

static int elan_run_calibration(struct epl2182_priv *epl_data)
{

    struct epl2182_priv *obj = epl_data;
    u16 ch1;
    u32 ch1_all=0;
    int count =5;
    int i;
    uint8_t read_data[2];
    int ps_hthr=0, ps_lthr=0;
    int ps_cal_len = 0;
    char ps_calibration[20];
    bool enable_ps = test_bit(CMC_BIT_PS, &obj->enable) && atomic_read(&obj->ps_suspend)==0;

    APS_FUN();

    if(!epl_data)
    {
        APS_ERR("epl2182_obj is null!!\n");
        return -EINVAL;
    }

    if(PS_MAX_XTALK < 0)
    {
        APS_ERR("Failed: PS_MAX_XTALK < 0 \r\n");
        return -EINVAL;
    }

    if(enable_ps == 0)
    {
        set_bit(CMC_BIT_PS, &obj->enable);
        epl2182_restart_polling();
        msleep(ALS_DELAY+2*PS_DELAY+50);
    }

    for(i=0; i<count; i++)
    {
        u16 ps_cali_raw;
        msleep(PS_DELAY);


        ps_cali_raw = gRawData.ps_raw;
	    APS_LOG("[%s]: gRawData.ps_raw=%d \r\n", __func__, gRawData.ps_raw);

		ch1_all = ch1_all+ ps_cali_raw;
    }

    ch1 = (u16)ch1_all/count;
    if(ch1 > PS_MAX_XTALK)
    {
        APS_ERR("Failed: ch1 > max_xtalk(%d) \r\n", ch1);
        return -EINVAL;
    }
    else if(ch1 <= 0)
    {
        APS_ERR("Failed: ch1 = 0\r\n");
        return -EINVAL;
    }

    ps_hthr = ch1 + PS_h_offset;
    ps_lthr = ch1 + PS_l_offset;

    ps_cal_len = sprintf(ps_calibration, "%d,%d", ps_hthr, ps_lthr);

    if(write_factory_calibration(obj, ps_calibration, ps_cal_len) < 0)
    {
        APS_ERR("[%s] create file error \n", __func__);
        return -EINVAL;
    }

    gRawData.ps_als_factory.ps_cal_h = ps_hthr;
    gRawData.ps_als_factory.ps_cal_l = ps_lthr;
    epl_data->hw->ps_threshold_high = ps_hthr;
    epl_data->hw->ps_threshold_low = ps_lthr;
    //atomic_set(&epl_data->ps_thd_val_high, epl_data->hw->ps_threshold_high);
    //atomic_set(&epl_data->ps_thd_val_low, epl_data->hw->ps_threshold_low);

    set_psensor_intr_threshold(epl_data->hw->ps_threshold_low,epl_data->hw->ps_threshold_high);

	APS_LOG("[%s]: ch1 = %d\n", __func__, ch1);

	return ch1;
}

#endif


static void epl2182_notify_event(void)
{
    struct input_dev *idev = epl2182_obj->input_dev;

    APS_LOG("  --> LEFT\n\n");
    input_report_key(idev, KEYCODE_LEFT, 1);
    input_report_key(idev,  KEYCODE_LEFT, 0);
    input_sync(idev);
}

static void epl2182_hs_enable(struct epl2182_priv *epld, bool interrupt, bool full_enable)
{
    int ret;
    uint8_t regdata = 0;
    struct i2c_client *client = epld->client;

    if(full_enable)
    {

        regdata = PS_DRIVE | (interrupt? EPL_INT_FRAME_ENABLE : EPL_INT_DISABLE);
        ret = elan_epl2182_I2C_Write(client,REG_9,W_SINGLE_BYTE,0x02, regdata);

        regdata = EPL_SENSING_2_TIME | EPL_PS_MODE | EPL_L_GAIN | EPL_S_SENSING_MODE;
        ret = elan_epl2182_I2C_Write(client,REG_0,W_SINGLE_BYTE,0X02,regdata);

        regdata = HS_INTT<<4 | EPL_PST_1_TIME | EPL_12BIT_ADC;
        ret = elan_epl2182_I2C_Write(client,REG_1,W_SINGLE_BYTE,0X02,regdata);

        ret = elan_epl2182_I2C_Write(client,REG_7,W_SINGLE_BYTE,0X02,EPL_C_RESET);


    }

    ret = elan_epl2182_I2C_Write(client,REG_7,W_SINGLE_BYTE,0x02,EPL_C_START_RUN);

    if(epld->polling_mode_hs == 1){
        msleep(HS_DELAY);
    }

}

#if DYN_ENABLE
static void dyn_ps_cal(struct epl2182_priv *epl_data)
{
	if((gRawData.ps_raw < gRawData.ps_min_raw)
	&& (gRawData.ps_sta != 1)
	&& (gRawData.ps_condition <= DYN_PS_CONDITION))
	{
		gRawData.ps_min_raw = gRawData.ps_raw;
		epl_data->hw ->ps_threshold_low = gRawData.ps_raw + DYN_L_OFFSET;
		epl_data->hw ->ps_threshold_high = gRawData.ps_raw + DYN_H_OFFSET;
		set_psensor_intr_threshold(epl_data->hw ->ps_threshold_low,epl_data->hw ->ps_threshold_high);
		APS_LOG("dyn ps raw = %d, min = %d, condition = %d\n dyn h_thre = %d, l_thre = %d, ps_state = %d",
		gRawData.ps_raw, gRawData.ps_min_raw, gRawData.ps_condition,epl_data->hw ->ps_threshold_high,epl_data->hw ->ps_threshold_low, gRawData.ps_state);
	}
}
#endif

static int elan_epl2182_psensor_enable(struct epl2182_priv *epl_data, int enable)
{
    int ret = 0;
    u8 ps_state_tmp;
    uint8_t regdata;
    u16 ps_state;
    struct i2c_client *client = epl_data->client;
    hwm_sensor_data sensor_data;

    APS_LOG("[ELAN epl2182] %s enable = %d\n", __func__, enable);

    ret = elan_epl2182_I2C_Write(client,REG_9,W_SINGLE_BYTE,0x02,EPL_INT_DISABLE | PS_DRIVE);

    if(enable)
    {
        regdata = EPL_SENSING_2_TIME | EPL_PS_MODE | EPL_L_GAIN ;
        regdata = regdata | (epl_data->hw->polling_mode_ps == 0 ? EPL_C_SENSING_MODE : EPL_S_SENSING_MODE);
        ret = elan_epl2182_I2C_Write(client,REG_0,W_SINGLE_BYTE,0X02,regdata);

        regdata = PS_INTT<<4 | EPL_PST_1_TIME | EPL_10BIT_ADC;
        ret = elan_epl2182_I2C_Write(client,REG_1,W_SINGLE_BYTE,0X02,regdata);
#if ELAN_WRITE_CALI
        if(gRawData.ps_als_factory.cal_finished == 0 &&  gRawData.ps_als_factory.cal_file_exist ==1)
		    ret=read_factory_calibration(epl_data);

        APS_LOG("[ELAN epl2182] %s cal_finished = %d\, cal_file_exist = %d\n", __func__, gRawData.ps_als_factory.cal_finished , gRawData.ps_als_factory.cal_file_exist);
#endif

#ifndef DYN_ENABLE
        set_psensor_intr_threshold(epl_data->hw ->ps_threshold_low,epl_data->hw ->ps_threshold_high);
#endif

        ret = elan_epl2182_I2C_Write(client,REG_7,W_SINGLE_BYTE,0X02,EPL_C_RESET);
        ret = elan_epl2182_I2C_Write(client,REG_7,W_SINGLE_BYTE,0x02,EPL_C_START_RUN);

        msleep(PS_DELAY);



            elan_epl2182_I2C_Write(client,REG_13,R_SINGLE_BYTE,0x01,0);
            elan_epl2182_I2C_Read(client);
            ps_state_tmp = !((gRawData.raw_bytes[0]&0x04)>>2);
            gRawData.ps_sta = ((gRawData.raw_bytes[0] & 0x02) >> 1);

#if DYN_ENABLE
	    elan_epl2182_I2C_Write(client,REG_16,R_TWO_BYTE,0x01,0x00);
	    elan_epl2182_I2C_Read(client);
		gRawData.ps_raw = ((gRawData.raw_bytes[1]<<8) | gRawData.raw_bytes[0]);
    	elan_epl2182_I2C_Write(client,REG_14,R_TWO_BYTE,0x01,0x00);
	    elan_epl2182_I2C_Read(client);
		  gRawData.ps_condition = ((gRawData.raw_bytes[1] << 8) | gRawData.raw_bytes[0]);

		  dyn_ps_cal(epl_data);

		//APS_LOG("dyn k ps raw = %d, condition = %d\n, ps_state = %d",	gRawData.ps_raw, gRawData.ps_condition, ps_state);
#endif
        APS_LOG("[%s]:gRawData.ps_raw=%d \r\n", __func__, gRawData.ps_raw);
        if(epl_data->hw->polling_mode_ps == 0)
        {
            
            //elan_epl2182_I2C_Write(client,REG_13,R_SINGLE_BYTE,0x01,0);
            //elan_epl2182_I2C_Read(client);
            
            APS_LOG("[%s]:real ps_state = %d\n", __func__, ps_state_tmp);

            if(test_bit(CMC_BIT_ALS, &epl_data->enable))
            {
                APS_LOG("[%s]: ALS+PS mode \r\n", __func__);
                if((ps_state_tmp==0 && gRawData.ps_raw > epl_data->hw->ps_threshold_high) ||
                    (ps_state_tmp==1 && gRawData.ps_raw < epl_data->hw->ps_threshold_low))
                {
                    APS_LOG("change ps_state(ps_state_tmp=%d, gRawData.ps_state=%d) \r\n", ps_state_tmp, gRawData.ps_state);
                    ps_state = ps_state_tmp;
                }
                else
                {
                    ps_state = gRawData.ps_state;
                }
            }
            else
            {
                ps_state = ps_state_tmp;
                APS_LOG("[%s]: PS only \r\n", __func__);
            }

            if(gRawData.ps_state != ps_state)
            {
                gRawData.ps_state = ps_state;
#if PS_GES
                if( gRawData.ps_state==0)
                    epl2182_notify_event();
#endif
                sensor_data.values[0] = ps_state;
                sensor_data.value_divide = 1;
                sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
                //let up layer to know
                hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data);

                elan_epl2182_I2C_Write(client,REG_9,W_SINGLE_BYTE,0x02,EPL_INT_ACTIVE_LOW|PS_DRIVE);
                APS_LOG("[%s]: Driect report ps status .............\r\n", __func__);
                //APS_LOG("[%s]: EPL_INT_FRAME_ENABLE .............\r\n", __func__);
            }
            else
            {
                elan_epl2182_I2C_Write(client,REG_9,W_SINGLE_BYTE,0x02,EPL_INT_ACTIVE_LOW |PS_DRIVE);
                APS_LOG("[%s]: EPL_INT_ACTIVE_LOW .............\r\n", __func__);
            }
        }

    }
    else
    {
        regdata = EPL_SENSING_2_TIME | EPL_PS_MODE | EPL_L_GAIN | EPL_S_SENSING_MODE;
        ret = elan_epl2182_I2C_Write(client,REG_0,W_SINGLE_BYTE,0X02,regdata);
    }

    if(ret<0)
    {
        APS_ERR("[ELAN epl2182 error]%s: ps enable %d fail\n",__func__,ret);
    }
    else
    {
        ret = 0;
    }

    return ret;
}


static int elan_epl2182_lsensor_enable(struct epl2182_priv *epl_data, int enable)
{
    int ret = 0;
    uint8_t regdata;
    int mode;
    struct i2c_client *client = epl_data->client;

    APS_LOG("[ELAN epl2182] %s enable = %d\n", __func__, enable);

    if(enable)
    {
        regdata = EPL_INT_DISABLE;
        ret = elan_epl2182_I2C_Write(client,REG_9,W_SINGLE_BYTE,0x02, regdata);

#if ALS_FACTORY_MODE
        if(als_factory_flag == true)
        {
            regdata = EPL_S_SENSING_MODE | EPL_SENSING_2_TIME | EPL_ALS_MODE | EPL_AUTO_GAIN;
        }
        else
        {
            regdata = EPL_S_SENSING_MODE | EPL_SENSING_8_TIME | EPL_ALS_MODE | EPL_AUTO_GAIN;
        }
#else
        regdata = EPL_S_SENSING_MODE | EPL_SENSING_8_TIME | EPL_ALS_MODE | EPL_AUTO_GAIN;
#endif
        ret = elan_epl2182_I2C_Write(client,REG_0,W_SINGLE_BYTE,0X02,regdata);

        regdata = ALS_INTT<<4 | EPL_PST_1_TIME | EPL_10BIT_ADC;
        ret = elan_epl2182_I2C_Write(client,REG_1,W_SINGLE_BYTE,0X02,regdata);

        ret = elan_epl2182_I2C_Write(client,REG_10,W_SINGLE_BYTE,0X02,EPL_GO_MID);
        ret = elan_epl2182_I2C_Write(client,REG_11,W_SINGLE_BYTE,0x02,EPL_GO_LOW);

        ret = elan_epl2182_I2C_Write(client,REG_7,W_SINGLE_BYTE,0X02,EPL_C_RESET);
        ret = elan_epl2182_I2C_Write(client,REG_7,W_SINGLE_BYTE,0x02,EPL_C_START_RUN);
#if ALS_FACTORY_MODE
        if(als_factory_flag == true)
        {
            msleep(ALS_FACTORY_DELAY);
        }
        else
#endif
        {
            msleep(ALS_DELAY);
        }
    }

    if(ret<0)
    {
        APS_ERR("[ELAN epl2182 error]%s: als_enable %d fail\n",__func__,ret);
    }
    else
    {
        ret = 0;
    }

    return ret;
}

static void epl2182_read_hs(void)
{
    mutex_lock(&sensor_mutex);
    struct epl2182_priv *epld = epl2182_obj;
    struct i2c_client *client = epld->client;
    int max_frame = 200;
    int idx = hs_idx+hs_count;
    u16 data;


    elan_epl2182_I2C_Write(client,REG_16,R_TWO_BYTE,0x01,0x00);
    elan_epl2182_I2C_Read_long(client, 2);
    data=(gRawData.raw_bytes[1]<<8)|gRawData.raw_bytes[0];


    if(data>60800&& HS_INTT>HS_INTT_CENTER-5)
    {
        HS_INTT--;
        change_int_time=true;
    }
    else if(data>6400 && data <25600 && HS_INTT<HS_INTT_CENTER+5)
    {
        HS_INTT++;
        change_int_time=true;
    }
    else
    {
        change_int_time=false;

        if(idx>=max_frame)
            idx-=max_frame;

        gRawData.hs_data[idx] = data;

        if(hs_count>=max_frame)
        {
            hs_idx++;
            if(hs_idx>=max_frame)
                hs_idx=0;
        }

        hs_count++;
        if(hs_count>=max_frame)
            hs_count=max_frame;
    }
    mutex_unlock(&sensor_mutex);

}

static int epl2182_get_als_value(struct epl2182_priv *obj, u16 als)
{
    int idx;
    int report_type = 0; //0:table, 1:lux
    int lux = 0;

    lux = (als * obj->lux_per_count)/1000;

    for(idx = 0; idx < obj->als_level_num; idx++)
    {
        if(lux < obj->hw->als_level[idx])
        {
            break;
        }
    }

    if(idx >= obj->als_value_num)
    {
        APS_ERR("exceed range\n");
        idx = obj->als_value_num - 1;
    }

    if(report_type == 0)
    {
        //gRawData.als_lux = obj->hw->als_value[idx];
        //APS_LOG("ALS: %05d => %05d\n", als, obj->hw->als_value[idx]);
        //return gRawData.als_lux;
        int level_high = obj->hw->als_level[idx];
    	int level_low = (idx > 0) ? obj->hw->als_level[idx-1] : 0;
        int level_diff = level_high - level_low;
		int value_high = obj->hw->als_value[idx];
        int value_low = (idx > 0) ? obj->hw->als_value[idx-1] : 0;
        int value_diff = value_high - value_low;
        int value = 0;
        
        if ((level_low >= level_high) || (value_low >= value_high))
            value = value_low;
        else
            value = (level_diff * value_low + (lux - level_low) * value_diff + ((level_diff + 1) >> 1)) / level_diff;
        APS_DBG("LHJALS: %d:%d [%d, %d] => %d [%d, %d] \n", idx, lux, level_low, level_high, value, value_low, value_high);		
        return value;
    }
    else
    {
        gRawData.als_lux = lux;
        APS_LOG("ALS: %05d => %05d (lux=%d)\n", als, obj->hw->als_value[idx], lux);
        return gRawData.als_lux;
    }
}


static int set_psensor_intr_threshold(uint16_t low_thd, uint16_t high_thd)
{
    int ret = 0;
    struct epl2182_priv *epld = epl2182_obj;
    struct i2c_client *client = epld->client;

    uint8_t high_msb ,high_lsb, low_msb, low_lsb;

    APS_LOG("%s\n", __func__);

    high_msb = (uint8_t) (high_thd >> 8);
    high_lsb = (uint8_t) (high_thd & 0x00ff);
    low_msb  = (uint8_t) (low_thd >> 8);
    low_lsb  = (uint8_t) (low_thd & 0x00ff);

    APS_LOG("%s: low_thd = %d, high_thd = %d \n",__func__, low_thd, high_thd);

    elan_epl2182_I2C_Write(client,REG_2,W_SINGLE_BYTE,0x02,high_lsb);
    elan_epl2182_I2C_Write(client,REG_3,W_SINGLE_BYTE,0x02,high_msb);
    elan_epl2182_I2C_Write(client,REG_4,W_SINGLE_BYTE,0x02,low_lsb);
    elan_epl2182_I2C_Write(client,REG_5,W_SINGLE_BYTE,0x02,low_msb);

    return ret;
}



/*----------------------------------------------------------------------------*/
static void epl2182_dumpReg(struct i2c_client *client)
{
    APS_LOG("chip id REG 0x00 value = %8x\n", i2c_smbus_read_byte_data(client, 0x00));
    APS_LOG("chip id REG 0x01 value = %8x\n", i2c_smbus_read_byte_data(client, 0x08));
    APS_LOG("chip id REG 0x02 value = %8x\n", i2c_smbus_read_byte_data(client, 0x10));
    APS_LOG("chip id REG 0x03 value = %8x\n", i2c_smbus_read_byte_data(client, 0x18));
    APS_LOG("chip id REG 0x04 value = %8x\n", i2c_smbus_read_byte_data(client, 0x20));
    APS_LOG("chip id REG 0x05 value = %8x\n", i2c_smbus_read_byte_data(client, 0x28));
    APS_LOG("chip id REG 0x06 value = %8x\n", i2c_smbus_read_byte_data(client, 0x30));
    APS_LOG("chip id REG 0x07 value = %8x\n", i2c_smbus_read_byte_data(client, 0x38));
    APS_LOG("chip id REG 0x09 value = %8x\n", i2c_smbus_read_byte_data(client, 0x48));
    APS_LOG("chip id REG 0x0D value = %8x\n", i2c_smbus_read_byte_data(client, 0x68));
    APS_LOG("chip id REG 0x0E value = %8x\n", i2c_smbus_read_byte_data(client, 0x70));
    APS_LOG("chip id REG 0x0F value = %8x\n", i2c_smbus_read_byte_data(client, 0x71));
    APS_LOG("chip id REG 0x10 value = %8x\n", i2c_smbus_read_byte_data(client, 0x80));
    APS_LOG("chip id REG 0x11 value = %8x\n", i2c_smbus_read_byte_data(client, 0x88));
    APS_LOG("chip id REG 0x13 value = %8x\n", i2c_smbus_read_byte_data(client, 0x98));
}


/*----------------------------------------------------------------------------*/
int hw8k_init_device(struct i2c_client *client)
{
    APS_LOG("hw8k_init_device.........\r\n");

    epl2182_i2c_client=client;

    APS_LOG(" I2C Addr==[0x%x],line=%d\n",epl2182_i2c_client->addr,__LINE__);

    return 0;
}

/*----------------------------------------------------------------------------*/
int epl2182_get_addr(struct alsps_hw *hw, struct epl2182_i2c_addr *addr)
{
    if(!hw || !addr)
    {
        return -EFAULT;
    }
    addr->write_addr= hw->i2c_addr[0];
    return 0;
}


/*----------------------------------------------------------------------------*/
static void epl2182_power(struct alsps_hw *hw, unsigned int on)
{
    static unsigned int power_on = 0;

    //APS_LOG("power %s\n", on ? "on" : "off");
//#ifndef MT6582
    if(hw->power_id != POWER_NONE_MACRO)
    {
        if(power_on == on)
        {
            APS_LOG("ignore power control: %d\n", on);
        }
        else if(on)
        {
            if(!hwPowerOn(hw->power_id, hw->power_vol, "EPL2182"))
            {
                APS_ERR("power on fails!!\n");
            }
        }
        else
        {
            if(!hwPowerDown(hw->power_id, "EPL2182"))
            {
                APS_ERR("power off fail!!\n");
            }
        }
    }
    power_on = on;
//#endif
}



/*----------------------------------------------------------------------------*/
static int epl2182_check_intr(struct i2c_client *client)
{
    struct epl2182_priv *obj = i2c_get_clientdata(client);
    int mode;

    APS_LOG("int pin = %d\n", mt_get_gpio_in(GPIO_ALS_EINT_PIN));

    //if (mt_get_gpio_in(GPIO_ALS_EINT_PIN) == 1) /*skip if no interrupt*/
    //   return 0;

    elan_epl2182_I2C_Write(obj->client,REG_13,R_SINGLE_BYTE,0x01,0);
    elan_epl2182_I2C_Read(obj->client);
    mode = gRawData.raw_bytes[0]&(3<<4);
    APS_LOG("mode %d\n", mode);

    if(mode==0x10)// PS
    {
        set_bit(CMC_BIT_PS, &obj->pending_intr);
    }
    else
    {
        clear_bit(CMC_BIT_PS, &obj->pending_intr);
    }


    if(atomic_read(&obj->trace) & CMC_TRC_DEBUG)
    {
        APS_LOG("check intr: 0x%08X\n", (unsigned int)obj->pending_intr);
    }

    return 0;

}



/*----------------------------------------------------------------------------*/

int epl2182_read_als(struct i2c_client *client)
{
    struct epl2182_priv *obj = i2c_get_clientdata(client);
    uint8_t setting;
    u16 ch1;

    if(client == NULL)
    {
        APS_DBG("CLIENT CANN'T EQUL NULL\n");
        return -1;
    }

    elan_epl2182_I2C_Write(client,REG_13,R_SINGLE_BYTE,0x01,0);
    elan_epl2182_I2C_Read(client);
    setting = gRawData.raw_bytes[0];
    if((setting&(3<<4))!=0x00)
    {
        APS_ERR("read als data in wrong mode\n");
    }

    elan_epl2182_I2C_Write(obj->client,REG_16,R_TWO_BYTE,0x01,0x00);
    elan_epl2182_I2C_Read(obj->client);
    ch1 = (gRawData.raw_bytes[1]<<8) | gRawData.raw_bytes[0];

    // FIX: mid gain and low gain cannot report ff in auton gain
    if(setting>>7 ==0&& ch1==65535)
    {
        APS_LOG("setting %d, gain %x, als %d\n", setting, setting>>7,  ch1);
        APS_LOG("skip FF in auto gain\n\n");
    }
    else
    {
        if(ch1 < 15) //\B9\FD\C2˵\F4Сֵ\B2\BF\B7\D6,\D3Ż\AF\B9\A4\B3\A7\B7\B4\C0\A1\B9\A4\B3\A7ģʽ\B8\B2\B8\C7ps\BF\D7als\B2\BB\CEȶ\A8\B5\C4\CE\CA\CC\E2,\B4\CBоƬ16bits,\B8й\E2\B6\C8̫\B8\DF,\CE޷\A8\B3\B9\B5׽\E2\BE\F6
            ch1 = 0;
        gRawData.als_ch1_raw = ch1;
        APS_LOG("read als raw data = %d\n", gRawData.als_ch1_raw);
    }

    return 0;
}


/*----------------------------------------------------------------------------*/
long epl2182_read_ps(struct i2c_client *client, u16 *data)
{
    struct epl2182_priv *obj = i2c_get_clientdata(client);
    uint8_t setting;
    u16 new_ps_state;
    u16 ps_state;


    if(client == NULL)
    {
        APS_DBG("CLIENT CANN'T EQUL NULL\n");
        return -1;
    }

    elan_epl2182_I2C_Write(obj->client,REG_13,R_SINGLE_BYTE,0x01,0);
    elan_epl2182_I2C_Read(obj->client);
    setting = gRawData.raw_bytes[0];
    if((setting&(3<<4))!=0x10)
    {
        APS_ERR("read ps data in wrong mode\n");
    }
    new_ps_state= !((gRawData.raw_bytes[0]&0x04)>>2);
    APS_LOG("[%s]:real ps_state = %d\n", __func__, new_ps_state);
#if PS_GES
    if( new_ps_state==0 && gRawData.ps_state==1)
        epl2182_notify_event();
#endif

    elan_epl2182_I2C_Write(obj->client,REG_16,R_TWO_BYTE,0x01,0x00);
    elan_epl2182_I2C_Read(obj->client);
    gRawData.ps_raw = (gRawData.raw_bytes[1]<<8) | gRawData.raw_bytes[0];

    if(test_bit(CMC_BIT_ALS, &obj->enable))
    {
        APS_LOG("[%s]: ALS+PS mode \r\n", __func__);
        if((new_ps_state==0 && gRawData.ps_raw > obj->hw->ps_threshold_high) ||
            (new_ps_state==1 && gRawData.ps_raw < obj->hw->ps_threshold_low))
        {
            APS_LOG("[%s]:change ps_state(new_ps_state=%d, gRawData.ps_state=%d) \r\n", __func__, new_ps_state, gRawData.ps_state);
            ps_state = new_ps_state;
        }
        else
        {
            ps_state = gRawData.ps_state;
        }
    }
    else
    {
        ps_state= new_ps_state;
        APS_LOG("[%s]: PS only \r\n", __func__);
    }

    gRawData.ps_state = ps_state;




    *data = gRawData.ps_raw ;
    APS_LOG("read ps raw data = %d\n", gRawData.ps_raw);
    APS_LOG("read ps binary data = %d\n", gRawData.ps_state);

    return 0;
}

void epl2182_restart_polling(void)
{
    struct epl2182_priv *obj = epl2182_obj;
    cancel_delayed_work(&obj->polling_work);
    int queue_flag = work_busy(&obj->polling_work);
    APS_LOG("[%s]: queue_flag=%d \r\n", __func__, queue_flag);
    if(queue_flag == 0)
    {
        schedule_delayed_work(&obj->polling_work, msecs_to_jiffies(50));
    }
    else
    {
        schedule_delayed_work(&obj->polling_work, msecs_to_jiffies(ALS_DELAY+2*PS_DELAY+50));
    }

}


void epl2182_polling_work(struct work_struct *work)
{
    struct epl2182_priv *obj = epl2182_obj;
    struct i2c_client *client = obj->client;

    bool enable_ps = test_bit(CMC_BIT_PS, &obj->enable) && atomic_read(&obj->ps_suspend)==0;
    bool enable_als = test_bit(CMC_BIT_ALS, &obj->enable) && atomic_read(&obj->als_suspend)==0;
    bool enable_hs = test_bit(CMC_BIT_HS, &obj->enable) && atomic_read(&obj->hs_suspend)==0;

    APS_LOG("als / ps / hs enable: %d / %d / %d\n", enable_als, enable_ps, enable_hs);

    cancel_delayed_work(&obj->polling_work);
#if ALS_FACTORY_MODE
    if(enable_als && als_factory_flag == true)
    {
        schedule_delayed_work(&obj->polling_work, msecs_to_jiffies(ALS_FACTORY_DELAY+20));
        APS_LOG("[%s]: ALS_FACTORY_MODE............... \r\n", __func__);
    }
    else if((enable_ps&& obj->hw->polling_mode_ps == 1) || (enable_als==true  && enable_hs==false) || (enable_als==true && enable_ps==true) || (enable_als==true && enable_ps==false))
#else

#if QUEUE_RUN
    if((enable_ps==true) || (enable_als==true  && enable_hs==false) || (enable_als==true && enable_ps==true) || (enable_als==true && enable_ps==false))
#else
    if((enable_ps&& obj->hw->polling_mode_ps == 1) || (enable_als==true  && enable_hs==false) || (enable_als==true && enable_ps==true) || (enable_als==true && enable_ps==false))
#endif
#endif
    {
        schedule_delayed_work(&obj->polling_work, msecs_to_jiffies(ALS_DELAY+2*PS_DELAY+30));
    }


    if(enable_als)
    {
        elan_epl2182_lsensor_enable(obj, 1);
        epl2182_read_als(client);
    }

    if(enable_hs)
    {
        if (obj->polling_mode_hs==0)
        {
            epl2182_hs_enable(obj, true, true);
        }
        else
        {
            epl2182_read_hs();
            epl2182_hs_enable(obj, false, true);
            schedule_delayed_work(&obj->polling_work, msecs_to_jiffies(5));//HS_DELAY
        }
    }
    else if(enable_ps)
    {
        elan_epl2182_psensor_enable(obj, 1);
        if(obj->hw->polling_mode_ps == 1)
        {
            epl2182_read_ps(client, &gRawData.ps_raw);
        }
    }
#if 0
    if(gRawData.ps_suspend_flag)
    {
        cancel_delayed_work(&obj->polling_work);
    }
#endif
    if(enable_als==false && enable_ps==false && enable_hs==false)
    {
        APS_LOG("disable sensor\n");
        elan_epl2182_lsensor_enable(obj, 1);
        cancel_delayed_work(&obj->polling_work);
        elan_epl2182_I2C_Write(client,REG_9,W_SINGLE_BYTE,0x02,EPL_INT_DISABLE);
        elan_epl2182_I2C_Write(client,REG_0,W_SINGLE_BYTE,0X02,EPL_S_SENSING_MODE);
    }

}


/*----------------------------------------------------------------------------*/
void epl2182_eint_func(void)
{
    struct epl2182_priv *obj = g_epl2182_ptr;

    // APS_LOG(" interrupt fuc\n");

    if(!obj)
    {
        return;
    }
#if defined(MT6582) || defined(MT6592)
    mt_eint_mask(CUST_EINT_ALS_NUM);
#else
    mt65xx_eint_mask(CUST_EINT_ALS_NUM);
#endif
    schedule_delayed_work(&obj->eint_work, 0);
}



/*----------------------------------------------------------------------------*/
static void epl2182_eint_work(struct work_struct *work)
{
    struct epl2182_priv *epld = g_epl2182_ptr;
    int err;
    hwm_sensor_data sensor_data;
    u8 ps_state;


    if(test_bit(CMC_BIT_HS, &epld->enable) && atomic_read(&epld->hs_suspend)==0)
    {
        epl2182_read_hs();
        epl2182_hs_enable(epld, true, change_int_time);
    }
    else if(test_bit(CMC_BIT_PS, &epld->enable))
    {
        APS_LOG("xxxxx eint work\n");

        if((err = epl2182_check_intr(epld->client)))
        {
            APS_ERR("check intrs: %d\n", err);
        }

        if(epld->pending_intr)
        {
            elan_epl2182_I2C_Write(epld->client,REG_13,R_SINGLE_BYTE,0x01,0);
            elan_epl2182_I2C_Read(epld->client);
            ps_state = !((gRawData.raw_bytes[0]&0x04)>>2);
            APS_LOG("real ps_state = %d\n", ps_state);
            //gRawData.ps_state= !((gRawData.raw_bytes[0]&0x04)>>2);
            //APS_LOG("ps state = %d\n", gRawData.ps_state);

            elan_epl2182_I2C_Write(epld->client,REG_16,R_TWO_BYTE,0x01,0x00);
            elan_epl2182_I2C_Read(epld->client);
            gRawData.ps_raw = (gRawData.raw_bytes[1]<<8) | gRawData.raw_bytes[0];
            APS_LOG("ps raw_data = %d\n", gRawData.ps_raw);

            if(test_bit(CMC_BIT_ALS, &epld->enable))
            {
                APS_LOG("ALS+PS mode \r\n");
                if((ps_state==0 && gRawData.ps_raw > epld->hw->ps_threshold_high) ||
                    (ps_state==1 && gRawData.ps_raw < epld->hw->ps_threshold_low))
                {
                    APS_LOG("change ps_state(ps_state=%d, gRawData.ps_state=%d) \r\n", ps_state, gRawData.ps_state);
                    gRawData.ps_state = ps_state;
                }
            }
            else
            {
                gRawData.ps_state = ps_state;
                APS_LOG("PS only \r\n");
            }

            sensor_data.values[0] = gRawData.ps_state;
            sensor_data.value_divide = 1;
            sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
#if PS_GES
            if( gRawData.ps_state==0)
                epl2182_notify_event();
#endif
            //let up layer to know
            if((err = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data)))
            {
                APS_ERR("get interrupt data failed\n");
                APS_ERR("call hwmsen_get_interrupt_data fail = %d\n", err);
            }
        }

        elan_epl2182_I2C_Write(epld->client,REG_9,W_SINGLE_BYTE,0x02,EPL_INT_ACTIVE_LOW | PS_DRIVE);
        elan_epl2182_I2C_Write(epld->client,REG_7,W_SINGLE_BYTE,0x02,EPL_DATA_UNLOCK);
    }




exit:
#ifdef MT6516
    MT6516_EINTIRQUnmask(CUST_EINT_ALS_NUM);
#endif

#ifdef MT6573
    mt65xx_eint_unmask(CUST_EINT_ALS_NUM);
#endif

#ifdef MT6513
    mt65xx_eint_unmask(CUST_EINT_ALS_NUM);
#endif

#ifdef MT6575
    mt65xx_eint_unmask(CUST_EINT_ALS_NUM);
#endif

#ifdef MT6577
    mt65xx_eint_unmask(CUST_EINT_ALS_NUM);
#endif

#ifdef MT6589
    mt65xx_eint_unmask(CUST_EINT_ALS_NUM);
#endif

#ifdef MT6572
    mt65xx_eint_unmask(CUST_EINT_ALS_NUM);
#endif

#ifdef MT6582
    mt_eint_unmask(CUST_EINT_ALS_NUM);
#endif

}



/*----------------------------------------------------------------------------*/
int epl2182_setup_eint(struct i2c_client *client)
{
    struct epl2182_priv *obj = i2c_get_clientdata(client);

    APS_LOG("epl2182_setup_eint\n");


    g_epl2182_ptr = obj;

    /*configure to GPIO function, external interrupt*/

    mt_set_gpio_mode(GPIO_ALS_EINT_PIN, GPIO_ALS_EINT_PIN_M_EINT);
    mt_set_gpio_dir(GPIO_ALS_EINT_PIN, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_ALS_EINT_PIN, GPIO_PULL_ENABLE);
    mt_set_gpio_pull_select(GPIO_ALS_EINT_PIN, GPIO_PULL_UP);

#ifdef MT6516
    MT6516_EINT_Set_Sensitivity(CUST_EINT_ALS_NUM, CUST_EINT_EDGE_SENSITIVE);
    MT6516_EINT_Set_Polarity(CUST_EINT_ALS_NUM, CUST_EINT_ALS_POLARITY);
    MT6516_EINT_Set_HW_Debounce(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_CN);
    MT6516_EINT_Registration(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_EN, CUST_EINT_POLARITY_LOW, epl2182_eint_func, 0);
    MT6516_EINTIRQUnmask(CUST_EINT_ALS_NUM);
#endif

#ifdef MT6513
    mt65xx_eint_set_sens(CUST_EINT_ALS_NUM, CUST_EINT_EDGE_SENSITIVE);
    mt65xx_eint_set_polarity(CUST_EINT_ALS_NUM, CUST_EINT_ALS_POLARITY);
    mt65xx_eint_set_hw_debounce(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_CN);
    mt65xx_eint_registration(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_EN, CUST_EINT_POLARITY_LOW, epl2182_eint_func, 0);
    mt65xx_eint_unmask(CUST_EINT_ALS_NUM);
#endif

#ifdef MT6573
    mt65xx_eint_set_sens(CUST_EINT_ALS_NUM, CUST_EINT_EDGE_SENSITIVE);
    mt65xx_eint_set_polarity(CUST_EINT_ALS_NUM, CUST_EINT_ALS_POLARITY);
    mt65xx_eint_set_hw_debounce(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_CN);
    mt65xx_eint_registration(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_EN, CUST_EINT_POLARITY_LOW, epl2182_eint_func, 0);
    mt65xx_eint_unmask(CUST_EINT_ALS_NUM);
#endif

#ifdef  MT6575
    mt65xx_eint_set_sens(CUST_EINT_ALS_NUM, CUST_EINT_EDGE_SENSITIVE);
    mt65xx_eint_set_polarity(CUST_EINT_ALS_NUM, CUST_EINT_ALS_POLARITY);
    mt65xx_eint_set_hw_debounce(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_CN);
    mt65xx_eint_registration(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_EN, CUST_EINT_POLARITY_LOW, epl2182_eint_func, 0);
    mt65xx_eint_unmask(CUST_EINT_ALS_NUM);
#endif

#ifdef  MT6577
    mt65xx_eint_set_sens(CUST_EINT_ALS_NUM, CUST_EINT_EDGE_SENSITIVE);
    mt65xx_eint_set_polarity(CUST_EINT_ALS_NUM, CUST_EINT_ALS_POLARITY);
    mt65xx_eint_set_hw_debounce(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_CN);
    mt65xx_eint_registration(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_EN, CUST_EINT_POLARITY_LOW, epl2182_eint_func, 0);
    mt65xx_eint_unmask(CUST_EINT_ALS_NUM);
#endif

#ifdef  MT6589
    mt65xx_eint_set_sens(CUST_EINT_ALS_NUM, CUST_EINT_EDGE_SENSITIVE);
    mt65xx_eint_set_polarity(CUST_EINT_ALS_NUM, CUST_EINT_ALS_POLARITY);
    mt65xx_eint_set_hw_debounce(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_CN);
    mt65xx_eint_registration(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_EN, CUST_EINT_POLARITY_LOW, epl2182_eint_func, 0);
    mt65xx_eint_unmask(CUST_EINT_ALS_NUM);
#endif

#ifdef MT6572
    mt65xx_eint_set_sens(CUST_EINT_ALS_NUM, CUST_EINT_EDGE_SENSITIVE);
    mt65xx_eint_set_polarity(CUST_EINT_ALS_NUM, CUST_EINT_ALS_POLARITY);
    mt65xx_eint_set_hw_debounce(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_CN);
    mt65xx_eint_registration(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_EN, CUST_EINT_POLARITY_LOW, epl2182_eint_func, 0);
    mt65xx_eint_unmask(CUST_EINT_ALS_NUM);
#endif

#ifdef MT6582
	mt_eint_set_hw_debounce(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_CN);
	mt_eint_registration(CUST_EINT_ALS_NUM, CUST_EINT_ALS_TYPE, epl2182_eint_func, 0);

	mt_eint_unmask(CUST_EINT_ALS_NUM);
#endif

#ifdef MT6592
	mt_eint_set_hw_debounce(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_CN);
	mt_eint_registration(CUST_EINT_ALS_NUM, CUST_EINT_ALS_TYPE, epl2182_eint_func, 0);

	mt_eint_unmask(CUST_EINT_ALS_NUM);
#endif
    return 0;
}




/*----------------------------------------------------------------------------*/
static int epl2182_init_client(struct i2c_client *client)
{
    struct epl2182_priv *obj = i2c_get_clientdata(client);
    int err=0;

    APS_LOG("[Agold spl] I2C Addr==[0x%x],line=%d\n",epl2182_i2c_client->addr,__LINE__);

    /*  interrupt mode */


    APS_FUN();

    if(obj->hw->polling_mode_ps == 0)
    {
#if defined(MT6582) || defined(MT6592)
        mt_eint_mask(CUST_EINT_ALS_NUM);
#else
        mt65xx_eint_mask(CUST_EINT_ALS_NUM);
#endif

        if((err = epl2182_setup_eint(client)))
        {
            APS_ERR("setup eint: %d\n", err);
            return err;
        }
        APS_LOG("epl2182 interrupt setup\n");
    }


    if((err = hw8k_init_device(client)) != 0)
    {
        APS_ERR("init dev: %d\n", err);
        return err;
    }


    if((err = epl2182_check_intr(client)))
    {
        APS_ERR("check/clear intr: %d\n", err);
        return err;
    }


    /*  interrupt mode */
//if(obj->hw->polling_mode_ps == 0)
    //     mt65xx_eint_unmask(CUST_EINT_ALS_NUM);

    return err;
}


/*----------------------------------------------------------------------------*/
static ssize_t epl2182_show_reg(struct device_driver *ddri, char *buf)
{
    if(!epl2182_obj)
    {
        APS_ERR("epl2182_obj is null!!\n");
        return 0;
    }
    ssize_t len = 0;
    struct i2c_client *client = epl2182_obj->client;

    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x00 value = %8x\n", i2c_smbus_read_byte_data(client, 0x00));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x01 value = %8x\n", i2c_smbus_read_byte_data(client, 0x08));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x02 value = %8x\n", i2c_smbus_read_byte_data(client, 0x10));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x03 value = %8x\n", i2c_smbus_read_byte_data(client, 0x18));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x04 value = %8x\n", i2c_smbus_read_byte_data(client, 0x20));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x05 value = %8x\n", i2c_smbus_read_byte_data(client, 0x28));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x06 value = %8x\n", i2c_smbus_read_byte_data(client, 0x30));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x07 value = %8x\n", i2c_smbus_read_byte_data(client, 0x38));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x09 value = %8x\n", i2c_smbus_read_byte_data(client, 0x48));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x0D value = %8x\n", i2c_smbus_read_byte_data(client, 0x68));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x0E value = %8x\n", i2c_smbus_read_byte_data(client, 0x70));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x0F value = %8x\n", i2c_smbus_read_byte_data(client, 0x71));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x10 value = %8x\n", i2c_smbus_read_byte_data(client, 0x80));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x11 value = %8x\n", i2c_smbus_read_byte_data(client, 0x88));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x13 value = %8x\n", i2c_smbus_read_byte_data(client, 0x98));

    return len;

}

/*----------------------------------------------------------------------------*/
static ssize_t epl2182_show_status(struct device_driver *ddri, char *buf)
{
    ssize_t len = 0;
    struct epl2182_priv *epld = epl2182_obj;
    u16 ch0, ch1;

    if(!epl2182_obj)
    {
        APS_ERR("epl2182_obj is null!!\n");
        return 0;
    }
    elan_epl2182_I2C_Write(epld->client,REG_7,W_SINGLE_BYTE,0x02,EPL_DATA_LOCK);

    elan_epl2182_I2C_Write(epld->client,REG_14,R_TWO_BYTE,0x01,0x00);
    elan_epl2182_I2C_Read(epld->client);
    ch0 = (gRawData.raw_bytes[1]<<8) | gRawData.raw_bytes[0];
    elan_epl2182_I2C_Write(epld->client,REG_16,R_TWO_BYTE,0x01,0x00);
    elan_epl2182_I2C_Read(epld->client);
    ch1 = (gRawData.raw_bytes[1]<<8) | gRawData.raw_bytes[0];

    elan_epl2182_I2C_Write(epld->client,REG_7,W_SINGLE_BYTE,0x02,EPL_DATA_UNLOCK);

    len += snprintf(buf+len, PAGE_SIZE-len, "als/ps int time is %d-%d\n",ALS_INTT, PS_INTT);
    len += snprintf(buf+len, PAGE_SIZE-len, "ch0 ch1 raw is %d-%d\n",ch0, ch1);
    len += snprintf(buf+len, PAGE_SIZE-len, "threshold is %d/%d\n",epld->hw->ps_threshold_low, epld->hw->ps_threshold_high);
    len += snprintf(buf+len, PAGE_SIZE-len, "heart int time: %d\n", HS_INTT);

    return len;
}

/*----------------------------------------------------------------------------*/
static ssize_t epl2182_show_renvo(struct device_driver *ddri, char *buf)
{
    ssize_t len = 0;

    APS_FUN();
    APS_LOG("gRawData.renvo=0x%x \r\n", gRawData.renvo);

    len += snprintf(buf+len, PAGE_SIZE-len, "%x",gRawData.renvo);

    return len;
}

/*----------------------------------------------------------------------------*/
static ssize_t epl2182_store_als_int_time(struct device_driver *ddri, const char *buf, size_t count)
{
    if(!epl2182_obj)
    {
        APS_ERR("epl2182_obj is null!!\n");
        return 0;
    }

    sscanf(buf, "%d", &ALS_INTT);
    APS_LOG("als int time is %d\n", ALS_INTT);
    return count;
}


/*----------------------------------------------------------------------------*/
static ssize_t epl2182_show_ps_cal_raw(struct device_driver *ddri, char *buf)
{
  APS_FUN();

    struct epl2182_priv *obj = epl2182_obj;
    u16 ch1;
    u32 ch1_all=0;
    int count =1;
    int i;
    uint8_t read_data[2];
    ssize_t len = 0;

    bool enable_ps = test_bit(CMC_BIT_PS, &obj->enable) && atomic_read(&obj->ps_suspend)==0;
    bool enable_als = test_bit(CMC_BIT_ALS, &obj->enable) && atomic_read(&obj->als_suspend)==0;

    if(!epl2182_obj)
    {
        APS_ERR("epl2182_obj is null!!\n");
        return 0;
    }


    if(enable_ps == 0 || enable_als == 0)
    {
        set_bit(CMC_BIT_ALS, &obj->enable);
        set_bit(CMC_BIT_PS, &obj->enable);
        epl2182_restart_polling();
        msleep(ALS_DELAY+2*PS_DELAY+30+50);
    }

    for(i=0; i<count; i++)
    {
        //elan_epl2182_psensor_enable(obj, 1);
        msleep(PS_DELAY);
        APS_LOG("epl2182_show_ps_cal_raw: gRawData.ps_raw=%d \r\n", gRawData.ps_raw);


		ch1_all = ch1_all+ gRawData.ps_raw;

    }

    ch1 = (u16)ch1_all/count;
	APS_LOG("epl2182_show_ps_cal_raw =  %d\n", ch1);

    len += snprintf(buf+len, PAGE_SIZE-len, "%d \r\n", ch1);

	return len;
}

static ssize_t epl2182_show_ps_threshold(struct device_driver *ddri, char *buf)
{
    ssize_t len = 0;
    struct epl2182_priv *obj = epl2182_obj;
#if ELAN_WRITE_CALI
    len += snprintf(buf+len, PAGE_SIZE-len, "gRawData.ps_als_factory(H/L): %d/%d \r\n", gRawData.ps_als_factory.ps_cal_h, gRawData.ps_als_factory.ps_cal_l);
#endif
    len += snprintf(buf+len, PAGE_SIZE-len, "ps_threshold(H/L): %d/%d \r\n", epl2182_obj->hw->ps_threshold_high, epl2182_obj->hw->ps_threshold_low);
    return len;
}



/*----------------------------------------------------------------------------*/
static ssize_t epl2182_store_ps_int_time(struct device_driver *ddri, const char *buf, size_t count)
{
    if(!epl2182_obj)
    {
        APS_ERR("epl2182_obj is null!!\n");
        return 0;
    }
    sscanf(buf, "%d", &PS_INTT);
    APS_LOG("ps int time is %d\n", PS_INTT);
    return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t epl2182_store_ps_threshold(struct device_driver *ddri, const char *buf, size_t count)
{
    if(!epl2182_obj)
    {
        APS_ERR("epl2182_obj is null!!\n");
        return 0;
    }
    sscanf(buf, "%d,%d", &epl2182_obj->hw->ps_threshold_low, &epl2182_obj->hw->ps_threshold_high);
#if ELAN_WRITE_CALI
    gRawData.ps_als_factory.ps_cal_h = epl2182_obj->hw->ps_threshold_high;
    gRawData.ps_als_factory.ps_cal_l = epl2182_obj->hw->ps_threshold_low;
#endif
    epl2182_restart_polling();
    return count;
}

static ssize_t epl2182_store_hs_enable(struct device_driver *ddri, const char *buf, size_t count)
{
    uint16_t mode=0;
    struct epl2182_priv *obj = epl2182_obj;
    bool enable_als = test_bit(CMC_BIT_ALS, &obj->enable) && atomic_read(&obj->als_suspend)==0;
    APS_FUN();

    sscanf(buf, "%hu",&mode);


    if(mode){
        if(enable_als == true){
            atomic_set(&obj->als_suspend, 1);
            hs_als_flag = 1;
            if(obj->polling_mode_hs == 1)
                msleep(ALS_DELAY);
        }
        set_bit(CMC_BIT_HS, &obj->enable);
    }
    else{
        clear_bit(CMC_BIT_HS, &obj->enable);
        if(obj->polling_mode_hs == 1)
                msleep(HS_DELAY);

        if(hs_als_flag == 1){
            atomic_set(&obj->als_suspend, 0);
            hs_als_flag = 0;

        }

    }
    if(mode)
    {
        //HS_INTT = EPL_INTT_PS_272;
        hs_idx=0;
        hs_count=0;
    }

    epl2182_restart_polling();
    return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t epl2182_show_hs_raws(struct device_driver *ddri, char *buf)
{

    mutex_lock(&sensor_mutex);

    u16 *tmp = (u16*)buf;
    u16 length= hs_count;
    int byte_count=2+length*2;
    int i=0;
    int start = hs_idx;
    APS_FUN();
    tmp[0]= length;
    if(length == 0){
        tmp[0] = 1;
        length = 1;
        show_hs_raws_flag = 1;
    }
    for(i=start; i<length; i++){
        if(show_hs_raws_flag == 1){
            tmp[i+1] = 0;
            show_hs_raws_flag = 0;
        }
        else{
            tmp[i+1] = gRawData.hs_data[i];
        }

    }

    hs_count=0;
    hs_idx=0;
    mutex_unlock(&sensor_mutex);

    return byte_count;
}

static ssize_t epl2182_store_hs_polling(struct device_driver *ddri, const char *buf, size_t count)
{
    struct epl2182_priv *obj = epl2182_obj;
    APS_FUN();

    sscanf(buf, "%hu",(short unsigned int *)&obj->polling_mode_hs);

    APS_LOG("HS polling mode: %d\n", obj->polling_mode_hs);

    return count;
}

#if ELAN_WRITE_CALI
static ssize_t epl2182_store_ps_w_calfile(struct device_driver *ddri, const char *buf, size_t count)
{
	struct epl2182_priv *obj = epl2182_obj;
    int ps_hthr=0, ps_lthr=0;
    int ps_cal_len = 0;
    char ps_calibration[20];
	APS_FUN();

	if(!epl2182_obj)
    {
        APS_ERR("epl2182_obj is null!!\n");
        return 0;
    }
    sscanf(buf, "%d,%d", &ps_hthr, &ps_lthr);

    ps_cal_len = sprintf(ps_calibration, "%d,%d", ps_hthr, ps_lthr);

    write_factory_calibration(obj, ps_calibration, ps_cal_len);
	return count;
}

static ssize_t epl2182_show_ps_run_cali(struct device_driver *ddri, const char *buf, size_t count)
{
	struct epl2182_priv *obj = epl2182_obj;
	ssize_t len = 0;
    int ret;

    APS_FUN();

    ret = elan_run_calibration(obj);

    len += snprintf(buf+len, PAGE_SIZE-len, "ret = %d\r\n", ret);

	return len;
}

#endif

static ssize_t epl2182_store_ps_polling(struct device_driver *ddri, const char *buf, size_t count)
{
	struct epl2182_priv *obj = epl2182_obj;
    struct hwmsen_object obj_ps; //Arima alvinchen 20140922 added for ALS/PS cali (From Elan IceFang) +++
    if(!epl2182_obj)
    {
        APS_ERR("epl2182_obj is null!!\n");
        return 0;
    }
	//Arima alvinchen 20140922 added for ALS/PS cali (From Elan IceFang) +++
    hwmsen_detach(ID_PROXIMITY);

    sscanf(buf, "%d", &obj->hw->polling_mode_ps);
    APS_LOG("ps polling mode is %d\n", obj->hw->polling_mode_ps);

    obj_ps.self = epl2182_obj;
    obj_ps.polling = obj->hw->polling_mode_ps;
    obj_ps.sensor_operate = epl2182_ps_operate;

    if(hwmsen_attach(ID_PROXIMITY, &obj_ps))    {
        APS_ERR("[%s]: attach fail !\n", __FUNCTION__);
    }
    epl2182_restart_polling();

    return count;
}

static ssize_t epl2182_store_ps_enable(struct device_driver *ddri, const char *buf, size_t count)
{
    int mode=0;
    struct epl2182_priv *obj = epl2182_obj;

    APS_FUN();

    sscanf(buf, "%d", &mode);

    if(mode)
        set_bit(CMC_BIT_PS, &obj->enable);
    else
        clear_bit(CMC_BIT_PS, &obj->enable);

    epl2182_restart_polling();


    return count;
}
static ssize_t epl2182_store_als_enable(struct device_driver *ddri, const char *buf, size_t count)
{
    int mode=0;
    struct epl2182_priv *obj = epl2182_obj;

    APS_FUN();

    sscanf(buf, "%d", &mode);

    if(mode)
    {
#if ALS_FACTORY_MODE
        als_factory_flag = false;
#endif
        set_bit(CMC_BIT_ALS, &obj->enable);
    }
    else
        clear_bit(CMC_BIT_ALS, &obj->enable);

    epl2182_restart_polling();


    return count;
}
static ssize_t epl_show_als_data(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct epl2182_priv *obj = epl2182_obj;
    ssize_t len = 0;
    bool enable_als = test_bit(CMC_BIT_ALS, &obj->enable) && atomic_read(&obj->als_suspend)==0;
    APS_FUN();
#if ALS_FACTORY_MODE
    als_factory_flag = true;
    clear_bit(CMC_BIT_PS, &obj->enable);    //disable PS
#endif
    if(enable_als == 0)
    {
        set_bit(CMC_BIT_ALS, &obj->enable);
        epl2182_restart_polling();
#if ALS_FACTORY_MODE
        msleep(ALS_FACTORY_DELAY+20);
#else
        msleep(ALS_DELAY+2*PS_DELAY+30+50);
#endif
    }

    APS_LOG("[%s]: ALS RAW = %d\n", __func__, gRawData.als_ch1_raw);
    len += snprintf(buf + len, PAGE_SIZE - len, "%d", gRawData.als_ch1_raw);
    return len;

}


static int epl2182_ReadChipInfo(struct i2c_client *client, char *buf, int bufsize)
{
	u8 databuf[10];    

	memset(databuf, 0, sizeof(u8)*10);

	if((NULL == buf)||(bufsize<=30))
	{
		return -1;
	}
	
	if(NULL == client)
	{
		*buf = 0;
		return -2;
	}

	sprintf(buf, "EPL2182 Chip");
	return 0;
}
static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = epl2182_i2c_client;
	char strbuf[256];
	if(NULL == client)
	{
		printk("i2c client is null!!\n");
		return 0;
	}
	
	epl2182_ReadChipInfo(client, strbuf, 256);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);        
}

/*----------------------------------------------------------------------------*/
static ssize_t epl2182_show_pscalibrate(struct device_driver *ddri, char *buf)
{
	int32_t word_data;
	u8 r_buf[2];
	int ret,i = 0,err;
	unsigned int data = 0, sum = 0;
	if(!epl2182_obj)
	{
		APS_ERR("epl2182_obj is null!!\n");
		return 0;
	}
      if((err = elan_epl2182_psensor_enable(epl2182_obj, 1))!=0)
      {
      APS_ERR("enable ps fail: %d\n", err);
      return -1;
      }
	for(i=0; i<Calibrate_num; i++)
	{
		if(ret = epl2182_read_ps(epl2182_obj->client,&data))
		{
			APS_ERR("pscalibrate error!!!\n");
			return 0;
		}
		sum += data;
		data = 0;
		msleep(70);
	}
	data = (int)(sum / Calibrate_num) + EPL_HT_N_CT;
	return snprintf(buf, PAGE_SIZE, "%d\n", data);
}
/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(elan_status, 			S_IROTH | S_IWOTH, epl2182_show_status,  NULL);
static DRIVER_ATTR(elan_reg,     		    S_IROTH | S_IWOTH, epl2182_show_reg,   	 NULL);
static DRIVER_ATTR(elan_renvo,    			S_IROTH | S_IWOTH, epl2182_show_renvo,   				NULL);
static DRIVER_ATTR(als_int_time,     	    S_IROTH | S_IWOTH, NULL,   				 epl2182_store_als_int_time);
static DRIVER_ATTR(ps_cal_raw, 			    S_IROTH | S_IWOTH, epl2182_show_ps_cal_raw, 	  		NULL);
static DRIVER_ATTR(ps_int_time,    	        S_IROTH | S_IWOTH, NULL,   				 epl2182_store_ps_int_time);
static DRIVER_ATTR(ps_threshold,     		S_IROTH | S_IWOTH, epl2182_show_ps_threshold, epl2182_store_ps_threshold);
static DRIVER_ATTR(hs_enable,				S_IROTH | S_IWOTH, NULL, epl2182_store_hs_enable);
static DRIVER_ATTR(hs_raws,					S_IROTH | S_IWOTH, epl2182_show_hs_raws, NULL);
static DRIVER_ATTR(hs_polling,				S_IROTH | S_IWOTH, NULL, epl2182_store_hs_polling);
#if ELAN_WRITE_CALI
static DRIVER_ATTR(ps_calfile,				S_IROTH | S_IWOTH, NULL,			epl2182_store_ps_w_calfile);
static DRIVER_ATTR(ps_run_cali,				S_IROTH | S_IWOTH, epl2182_show_ps_run_cali, NULL);
#endif
static DRIVER_ATTR(ps_polling,				S_IROTH | S_IWOTH,  NULL,			epl2182_store_ps_polling);
static DRIVER_ATTR(ps_enable,				S_IROTH | S_IWOTH, NULL,			epl2182_store_ps_enable);
static DRIVER_ATTR(als_enable,				S_IROTH | S_IWOTH, NULL,			epl2182_store_als_enable);
static DRIVER_ATTR(als_data,				S_IROTH | S_IWOTH, epl_show_als_data, NULL);

static DRIVER_ATTR(chipinfo,   S_IWUSR | S_IRUGO, show_chipinfo_value,      NULL);
static DRIVER_ATTR(pscalibrate,  S_IWUSR | S_IRUGO, epl2182_show_pscalibrate,  NULL);
/*----------------------------------------------------------------------------*/
static struct device_attribute * epl2182_attr_list[] =
{
    &driver_attr_elan_status,
    &driver_attr_elan_reg,
    &driver_attr_elan_renvo,
    &driver_attr_als_int_time,
    &driver_attr_ps_cal_raw,
    &driver_attr_ps_int_time,
    &driver_attr_ps_threshold,
    &driver_attr_hs_enable,
    &driver_attr_hs_raws,
    &driver_attr_hs_polling,
#if ELAN_WRITE_CALI
    &driver_attr_ps_calfile,
    &driver_attr_ps_run_cali,
#endif
    &driver_attr_ps_polling,
&driver_attr_ps_enable,
    &driver_attr_als_enable,
	&driver_attr_als_data,
    
    &driver_attr_chipinfo,
    &driver_attr_pscalibrate,
};

/*----------------------------------------------------------------------------*/
static int epl2182_create_attr(struct device_driver *driver)
{
    int idx, err = 0;
    int num = (int)(sizeof(epl2182_attr_list)/sizeof(epl2182_attr_list[0]));
    if (driver == NULL)
    {
        return -EINVAL;
    }

    for(idx = 0; idx < num; idx++)
    {
        if(err = driver_create_file(driver, epl2182_attr_list[idx]))
        {
            APS_ERR("driver_create_file (%s) = %d\n", epl2182_attr_list[idx]->attr.name, err);
            break;
        }
    }
    return err;
}



/*----------------------------------------------------------------------------*/
static int epl2182_delete_attr(struct device_driver *driver)
{
    int idx ,err = 0;
    int num = (int)(sizeof(epl2182_attr_list)/sizeof(epl2182_attr_list[0]));

    if (!driver)
        return -EINVAL;

    for (idx = 0; idx < num; idx++)
    {
        driver_remove_file(driver, epl2182_attr_list[idx]);
    }

    return err;
}



/******************************************************************************
 * Function Configuration
******************************************************************************/
static int epl2182_open(struct inode *inode, struct file *file)
{
    file->private_data = epl2182_i2c_client;

    APS_FUN();

    if (!file->private_data)
    {
        APS_ERR("null pointer!!\n");
        return -EINVAL;
    }

    return nonseekable_open(inode, file);
}

/*----------------------------------------------------------------------------*/
static int epl2182_release(struct inode *inode, struct file *file)
{
    APS_FUN();
    file->private_data = NULL;
    return 0;
}

/*----------------------------------------------------------------------------*/

static int set_psensor_threshold(struct i2c_client *client)
{
	struct epl2182_priv *obj = i2c_get_clientdata(client);
	int databuf[2];    
	int res = 0;
	databuf[0] = atomic_read(&obj->ps_thd_val_low);
	databuf[1] = atomic_read(&obj->ps_thd_val_high);//threshold value need to confirm

	res = set_psensor_intr_threshold(databuf[0],databuf[1]);
	return res;
}
/*----------------------------------------------------------------------------*/
static long epl2182_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct i2c_client *client = (struct i2c_client*)file->private_data;
    struct epl2182_priv *obj = i2c_get_clientdata(client);
    int err = 0;
    void __user *ptr = (void __user*) arg;
    int dat;
    uint32_t enable;
    bool enable_ps = test_bit(CMC_BIT_PS, &obj->enable) && atomic_read(&obj->ps_suspend)==0;
    bool enable_als = test_bit(CMC_BIT_ALS, &obj->enable) && atomic_read(&obj->als_suspend)==0;

    int ps_result;	
    int ps_cali;	
    int threshold[2];       
    unsigned int ret,i = 0,sum = 0,data=0;


    APS_LOG("[%s], cmd = %x........\r\n", __func__, cmd);
    switch (cmd)
    {
        case ALSPS_SET_PS_MODE:

            if(copy_from_user(&enable, ptr, sizeof(enable)))
            {
                err = -EFAULT;
                goto err_out;
            }

            APS_LOG("[%s]:ALSPS_SET_PS_MODE => enable=%d \r\n", __func__, enable);
            if(enable)
            {
#if DYN_ENABLE
		        gRawData.ps_min_raw=0xffff;
#endif
                set_bit(CMC_BIT_PS, &obj->enable);
            }
            else
            {
                clear_bit(CMC_BIT_PS, &obj->enable);
            }
            epl2182_restart_polling();

            break;


        case ALSPS_GET_PS_MODE:

            enable=test_bit(CMC_BIT_PS, &obj->enable);
            if(copy_to_user(ptr, &enable, sizeof(enable)))
            {
                err = -EFAULT;
                goto err_out;
            }
            break;


        case ALSPS_GET_PS_DATA:

            if(enable_ps == 0)
            {
                set_bit(CMC_BIT_PS, &obj->enable);
                epl2182_restart_polling();
            }
            dat = gRawData.ps_state;

            APS_LOG("ioctl ps state value = %d \n", dat);

            if(copy_to_user(ptr, &dat, sizeof(dat)))
            {
                err = -EFAULT;
                goto err_out;
            }

            break;


        case ALSPS_GET_PS_RAW_DATA:

            if(enable_ps == 0)
            {
                set_bit(CMC_BIT_PS, &obj->enable);
                epl2182_restart_polling();
            }
            dat = gRawData.ps_raw;

            APS_LOG("ioctl ps raw value = %d \n", dat);

            if(copy_to_user(ptr, &dat, sizeof(dat)))
            {
                err = -EFAULT;
                goto err_out;
            }

            break;


        case ALSPS_SET_ALS_MODE:
            if(copy_from_user(&enable, ptr, sizeof(enable)))
            {
                err = -EFAULT;
                goto err_out;
            }
            if(enable)
                set_bit(CMC_BIT_ALS, &obj->enable);
            else
                clear_bit(CMC_BIT_ALS, &obj->enable);

            break;



        case ALSPS_GET_ALS_MODE:
            enable=test_bit(CMC_BIT_ALS, &obj->enable);
            if(copy_to_user(ptr, &enable, sizeof(enable)))
            {
                err = -EFAULT;
                goto err_out;
            }
            break;



        case ALSPS_GET_ALS_DATA:

            if(enable_als == 0)
            {
                set_bit(CMC_BIT_ALS, &obj->enable);
                epl2182_restart_polling();
            }

            dat = epl2182_get_als_value(obj, gRawData.als_ch1_raw);
            APS_LOG("ioctl get als data = %d\n", dat);

            if(copy_to_user(ptr, &dat, sizeof(dat)))
            {
                err = -EFAULT;
                goto err_out;
            }

            break;


        case ALSPS_GET_ALS_RAW_DATA:

            if(enable_als == 0)
            {
                set_bit(CMC_BIT_ALS, &obj->enable);
                epl2182_restart_polling();
            }

            dat = gRawData.als_ch1_raw;
            APS_LOG("ioctl get als raw data = %d\n", dat);

            if(copy_to_user(ptr, &dat, sizeof(dat)))
            {
                err = -EFAULT;
                goto err_out;
            }
            break;
//=========================================
         case ALSPS_GET_PS_CALI:
		 	
	 if((err = elan_epl2182_psensor_enable(obj, 1))!=0)
            {
                APS_ERR("enable ps fail: %d\n", err);
                return -1;
		    }
	 	
            for(i=0; i<Calibrate_num; i++)
            {
                if(ret = epl2182_read_ps(obj->client,&data))
                {
                    err = -EFAULT;
                    goto err_out;
                }
                sum += data;
		//   APS_LOG("ioctl ps raw CALI value = %d,sum=%d \n", dat,sum);
                data = 0;
                msleep(70);
            }
            data = (unsigned int)(sum / Calibrate_num) + EPL_HT_N_CT;
	     printk("epl2182 ALSPS_GET_PS_CALI data=%d,sum=%d\n",data,sum);
			if(copy_to_user(ptr, &data, sizeof(data)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;
			
		case ALSPS_SET_PS_CALI:
			if(copy_from_user(&dat, ptr, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}
			if(dat <= (EPL_HT_N_CT - EPL_LT_N_CT))
				dat = ps_threshold_high;
			atomic_set(&obj->ps_thd_val_high, dat);
			atomic_set(&obj->ps_thd_val_low, (dat - (EPL_HT_N_CT - EPL_LT_N_CT)));
			obj->hw->ps_threshold_high = dat;
			obj->hw->ps_threshold_low= dat - (EPL_HT_N_CT - EPL_LT_N_CT);
			set_psensor_threshold(obj->client);
			break;

			
			/*----------------------------------for factory mode test---------------------------------------*/
#if 0
						case ALSPS_GET_PS_TEST_RESULT:
#if 0
							if((err = epl2182_read_ps(obj->client, &obj->ps)))
							{
								goto err_out;
							}
							if(obj->ps > atomic_read(&obj->ps_thd_val_high))
								{
									ps_result = 0;
								}
							else	ps_result = 1;
#else
            if(enable_ps == 0)
            {
                set_bit(CMC_BIT_PS, &obj->enable);
                epl2182_restart_polling();
                msleep(ALS_DELAY+2*PS_DELAY+30);
            }

            if(gRawData.ps_raw > obj->hw->ps_threshold_high)
			{
			    ps_result = 0;
			}
			else
			    ps_result = 1;

			APS_LOG("[%s] ps_result = %d \r\n", __func__, ps_result);

#endif
			if(copy_to_user(ptr, &ps_result, sizeof(ps_result)))
							{
								err = -EFAULT;
								goto err_out;
							}			   
							break;
			
			
						case ALSPS_IOCTL_CLR_CALI:
#if 0
							if(copy_from_user(&dat, ptr, sizeof(dat)))
							{
								err = -EFAULT;
								goto err_out;
							}
							if(dat == 0)
								obj->ps_cali = 0;
#else

            APS_LOG("[%s]: ALSPS_IOCTL_CLR_CALI \r\n", __func__);
#endif
							break;
			
						case ALSPS_IOCTL_GET_CALI:
#if 0
							ps_cali = obj->ps_cali ;
							APS_ERR("%s set ps_calix%x\n", __func__, obj->ps_cali);
							if(copy_to_user(ptr, &ps_cali, sizeof(ps_cali)))
							{
								err = -EFAULT;
								goto err_out;
							}
#else
            APS_LOG("[%s]: ALSPS_IOCTL_GET_CALI \r\n", __func__);
#endif
							break;
			
						case ALSPS_IOCTL_SET_CALI:
#if 0
							if(copy_from_user(&ps_cali, ptr, sizeof(ps_cali)))
							{
								err = -EFAULT;
								goto err_out;
							}
			
							obj->ps_cali = ps_cali;
							APS_ERR("%s set ps_calix%x\n", __func__, obj->ps_cali); 
#else
            APS_LOG("[%s]: ALSPS_IOCTL_SET_CALI \r\n", __func__);
#endif
							break;

						case ALSPS_SET_PS_THRESHOLD:
							if(copy_from_user(threshold, ptr, sizeof(threshold)))
							{
								err = -EFAULT;
								goto err_out;
							}
#if 0
							APS_ERR("%s set threshold high: 0x%x, low: 0x%x\n", __func__, threshold[0],threshold[1]); 
							atomic_set(&obj->ps_thd_val_high,  (threshold[0]+obj->ps_cali));
							atomic_set(&obj->ps_thd_val_low,  (threshold[1]+obj->ps_cali));//need to confirm
							set_psensor_threshold(obj->client);
#else
            APS_LOG("[%s] set threshold high: %d, low: %d\n", __func__, threshold[0],threshold[1]);
            obj->hw->ps_threshold_high = threshold[0];
            obj->hw->ps_threshold_low = threshold[1];
            set_psensor_intr_threshold(obj->hw->ps_threshold_low, obj->hw->ps_threshold_high);
#endif
							break;
							
						case ALSPS_GET_PS_THRESHOLD_HIGH:
#if 0
							APS_ERR("%s get threshold high before cali: 0x%x\n", __func__, atomic_read(&obj->ps_thd_val_high)); 
							threshold[0] = atomic_read(&obj->ps_thd_val_high) - obj->ps_cali;
							APS_ERR("%s set ps_calix%x\n", __func__, obj->ps_cali);
							APS_ERR("%s get threshold high: 0x%x\n", __func__, threshold[0]); 
#else
            threshold[0] = obj->hw->ps_threshold_high;
            APS_LOG("[%s] get threshold high: %d\n", __func__, threshold[0]);
#endif
							if(copy_to_user(ptr, &threshold[0], sizeof(threshold[0])))
							{
								err = -EFAULT;
								goto err_out;
							}
							break;
							
						case ALSPS_GET_PS_THRESHOLD_LOW:
#if 0
							APS_ERR("%s get threshold low before cali: 0x%x\n", __func__, atomic_read(&obj->ps_thd_val_low)); 
							threshold[0] = atomic_read(&obj->ps_thd_val_low) - obj->ps_cali;
							APS_ERR("%s set ps_calix%x\n", __func__, obj->ps_cali);
							APS_ERR("%s get threshold low: 0x%x\n", __func__, threshold[0]); 
#else
            threshold[0] = obj->hw->ps_threshold_low;
            APS_LOG("[%s] get threshold low: %d\n", __func__, threshold[0]);
#endif
							if(copy_to_user(ptr, &threshold[0], sizeof(threshold[0])))
							{
								err = -EFAULT;
								goto err_out;
							}
							break;
						/*------------------------------------------------------------------------------------------*/
#endif
        default:
            APS_ERR("%s not supported = 0x%04x \r\n", __FUNCTION__, cmd);
            err = -ENOIOCTLCMD;
            break;
    }

err_out:
    return err;
}


/*----------------------------------------------------------------------------*/
static struct file_operations epl2182_fops =
{
    .owner = THIS_MODULE,
    .open = epl2182_open,
    .release = epl2182_release,
    .unlocked_ioctl = epl2182_unlocked_ioctl,
};


/*----------------------------------------------------------------------------*/
static struct miscdevice epl2182_device =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = "als_ps",
    .fops = &epl2182_fops,
};


/*----------------------------------------------------------------------------*/
static int epl2182_i2c_suspend(struct i2c_client *client, pm_message_t msg)
{
    struct epl2182_priv *obj = i2c_get_clientdata(client);
    int err;
    APS_FUN();
#if 0
    if(msg.event == PM_EVENT_SUSPEND)
    {
        if(!obj)
        {
            APS_ERR("null pointer!!\n");
            return -EINVAL;
        }

        atomic_set(&obj->als_suspend, 1);
        atomic_set(&obj->ps_suspend, 1);
        atomic_set(&obj->hs_suspend, 1);

        if(test_bit(CMC_BIT_PS,  &obj->enable) && obj->hw->polling_mode_ps==0)
            epl2182_restart_polling();

        epl2182_power(obj->hw, 0);
    }

    if(test_bit(CMC_BIT_PS, &obj->enable) == 0)
    {
        elan_epl2182_lsensor_enable(obj, 1);
    }
#endif
    return 0;

}



/*----------------------------------------------------------------------------*/
static int epl2182_i2c_resume(struct i2c_client *client)
{
    struct epl2182_priv *obj = i2c_get_clientdata(client);
    int err;
    APS_FUN();
#if 0
    if(!obj)
    {
        APS_ERR("null pointer!!\n");
        return -EINVAL;
    }

    epl2182_power(obj->hw, 1);

    msleep(50);

    atomic_set(&obj->ps_suspend, 0);

    if(err = epl2182_init_client(client))
    {
        APS_ERR("initialize client fail!!\n");
        return err;
    }

    if(obj->hw->polling_mode_ps == 0)
        epl2182_setup_eint(client);


    if(test_bit(CMC_BIT_PS,  &obj->enable))
        epl2182_restart_polling();
#endif
    return 0;
}



/*----------------------------------------------------------------------------*/
static void epl2182_early_suspend(struct early_suspend *h)
{
    /*early_suspend is only applied for ALS*/
    struct epl2182_priv *obj = container_of(h, struct epl2182_priv, early_drv);
    int err;
    APS_FUN();

    if(!obj)
    {
        APS_ERR("null pointer!!\n");
        return;
    }

    atomic_set(&obj->als_suspend, 1);
#if 0
    if(obj->hw->polling_mode_ps == 0)
		gRawData.ps_suspend_flag = true;
#endif
    if(test_bit(CMC_BIT_PS, &obj->enable) == 0)
    {
        elan_epl2182_lsensor_enable(obj, 1);
    }
}



/*----------------------------------------------------------------------------*/
static void epl2182_late_resume(struct early_suspend *h)
{
    /*late_resume is only applied for ALS*/
    struct epl2182_priv *obj = container_of(h, struct epl2182_priv, early_drv);
    int err;
    APS_FUN();

    if(!obj)
    {
        APS_ERR("null pointer!!\n");
        return;
    }

    atomic_set(&obj->als_suspend, 0);
    atomic_set(&obj->ps_suspend, 0);
    atomic_set(&obj->hs_suspend, 0);
#if 0
    if(obj->hw->polling_mode_ps == 0)
		gRawData.ps_suspend_flag = false;
#endif
    if(test_bit(CMC_BIT_ALS, &obj->enable)==1)
        epl2182_restart_polling();
}


/*----------------------------------------------------------------------------*/
int epl2182_ps_operate(void* self, uint32_t command, void* buff_in, int size_in,
                       void* buff_out, int size_out, int* actualout)
{
    int err = 0;
    int value;
    hwm_sensor_data* sensor_data;
    struct epl2182_priv *obj = (struct epl2182_priv *)self;

    APS_LOG("epl2182_ps_operate command = %x\n",command);
    switch (command)
    {
        case SENSOR_DELAY:
            if((buff_in == NULL) || (size_in < sizeof(int)))
            {
                APS_ERR("Set delay parameter error!\n");
                err = -EINVAL;
            }
            break;


        case SENSOR_ENABLE:
            if((buff_in == NULL) || (size_in < sizeof(int)))
            {
                APS_ERR("Enable sensor parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                value = *(int *)buff_in;
                APS_LOG("ps enable = %d\n", value);


                if(value)
                {
                    wake_lock(&ps_lock);
                    if(obj->hw->polling_mode_ps==0)
                        gRawData.ps_state=2;
#if DYN_ENABLE
		gRawData.ps_min_raw=0xffff;
#endif
                    set_bit(CMC_BIT_PS, &obj->enable);
                }
                else
                {
                    clear_bit(CMC_BIT_PS, &obj->enable);

                    //elan_epl2182_lsensor_enable(obj, 1);
                    wake_unlock(&ps_lock);

                }
                epl2182_restart_polling();
            }

            break;



        case SENSOR_GET_DATA:
            APS_LOG(" get ps data !!!!!!\n");
            if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
            {
                APS_ERR("get sensor data parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                APS_LOG("---SENSOR_GET_DATA---\n\n");

                sensor_data = (hwm_sensor_data *)buff_out;
                sensor_data->values[0] =gRawData.ps_state;
                sensor_data->values[1] =gRawData.ps_raw;
                sensor_data->value_divide = 1;
                sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
            }
            break;


        default:
            APS_ERR("proxmy sensor operate function no this parameter %d!\n", command);
            err = -1;
            break;



    }

    return err;

}



int epl2182_als_operate(void* self, uint32_t command, void* buff_in, int size_in,
                        void* buff_out, int size_out, int* actualout)
{
    int err = 0;
    int value;
    hwm_sensor_data* sensor_data;
    struct epl2182_priv *obj = (struct epl2182_priv *)self;

    APS_FUN();
    APS_LOG("epl2182_als_operate command = %x\n",command);

    switch (command)
    {
        case SENSOR_DELAY:
            if((buff_in == NULL) || (size_in < sizeof(int)))
            {
                APS_ERR("Set delay parameter error!\n");
                err = -EINVAL;
            }
            break;


        case SENSOR_ENABLE:
            if((buff_in == NULL) || (size_in < sizeof(int)))
            {
                APS_ERR("Enable sensor parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                value = *(int *)buff_in;
                if(value)
                {
#if ALS_FACTORY_MODE
                    als_factory_flag = false;
#endif
                    set_bit(CMC_BIT_ALS, &obj->enable);
                    epl2182_restart_polling();
                }
                else
                {
                    clear_bit(CMC_BIT_ALS, &obj->enable);
                }

            }
            break;


        case SENSOR_GET_DATA:
            APS_LOG("get als data !!!!!!\n");
            if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
            {
                APS_ERR("get sensor data parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                sensor_data = (hwm_sensor_data *)buff_out;
                sensor_data->values[0] = epl2182_get_als_value(obj, gRawData.als_ch1_raw);
                sensor_data->value_divide = 1;
                sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
                APS_LOG("get als data->values[0] = %d\n", sensor_data->values[0]);
            }
            break;

        default:
            APS_ERR("light sensor operate function no this parameter %d!\n", command);
            err = -1;
            break;



    }

    return err;

}


/*----------------------------------------------------------------------------*/

static int epl2182_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info)
{
    strcpy(info->type, EPL2182_DEV_NAME);
    return 0;
}


/*----------------------------------------------------------------------------*/
static int epl2182_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct epl2182_priv *obj;
    struct hwmsen_object obj_ps, obj_als;
    int err = 0;
    APS_FUN();

    if((i2c_smbus_read_byte_data(client, 0x98)) != 0x68)
    {
        APS_LOG("elan ALS/PS sensor is failed. \n");
        goto exit;
    }

    epl2182_dumpReg(client);

    if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
    {
        err = -ENOMEM;
        goto exit;
    }

    memset(obj, 0, sizeof(*obj));

    epl2182_obj = obj;
    obj->hw = EPL2182_get_cust_alsps_hw();

    epl2182_get_addr(obj->hw, &obj->addr);

    obj->als_level_num = sizeof(obj->hw->als_level)/sizeof(obj->hw->als_level[0]);
    obj->als_value_num = sizeof(obj->hw->als_value)/sizeof(obj->hw->als_value[0]);
    BUG_ON(sizeof(obj->als_level) != sizeof(obj->hw->als_level));
    memcpy(obj->als_level, obj->hw->als_level, sizeof(obj->als_level));
    BUG_ON(sizeof(obj->als_value) != sizeof(obj->hw->als_value));
    memcpy(obj->als_value, obj->hw->als_value, sizeof(obj->als_value));

    INIT_DELAYED_WORK(&obj->eint_work, epl2182_eint_work);
    INIT_DELAYED_WORK(&obj->polling_work, epl2182_polling_work);
    wake_lock_init(&ps_lock, WAKE_LOCK_SUSPEND, "ps wakelock");

    obj->client = client;

    obj->input_dev = input_allocate_device();
    set_bit(EV_KEY, obj->input_dev->evbit);
    set_bit(EV_REL, obj->input_dev->evbit);
    set_bit(EV_ABS, obj->input_dev->evbit);
    obj->input_dev->evbit[0] |= BIT_MASK(EV_REP);
    obj->input_dev->keycodemax = 500;
    obj->input_dev->name ="elan_gesture";
    obj->input_dev->keybit[BIT_WORD(KEYCODE_LEFT)] |= BIT_MASK(KEYCODE_LEFT);
    if (input_register_device(obj->input_dev))
        APS_ERR("register input error\n");

    mutex_init(&sensor_mutex);

    i2c_set_clientdata(client, obj);

    atomic_set(&obj->trace, 0x00);
    atomic_set(&obj->als_suspend, 0);
    atomic_set(&obj->ps_suspend, 0);
    atomic_set(&obj->hs_suspend, 0);

    obj->lux_per_count = LUX_PER_COUNT;
    obj->enable = 0;
    obj->pending_intr = 0;
    obj->polling_mode_hs = POLLING_MODE_HS;


    epl2182_i2c_client = client;

    elan_epl2182_I2C_Write(client,REG_0,W_SINGLE_BYTE,0x02, EPL_S_SENSING_MODE);
    elan_epl2182_I2C_Write(client,REG_9,W_SINGLE_BYTE,0x02,EPL_INT_DISABLE);

    elan_epl2182_I2C_Write(client,REG_19,R_TWO_BYTE,0x01,0x00);
    elan_epl2182_I2C_Read(client);
    gRawData.renvo = (gRawData.raw_bytes[1]<<8)|gRawData.raw_bytes[0];

    gRawData.ps_suspend_flag = false;

    if(err = epl2182_init_client(client))
    {
        goto exit_init_failed;
    }


    if(err = misc_register(&epl2182_device))
    {
        APS_ERR("epl2182_device register failed\n");
        goto exit_misc_device_register_failed;
    }

    #if 0
    if(err = epl2182_create_attr(&epl2182_alsps_driver.driver))
    #else
    if(err = epl2182_create_attr(&(epl2182_init_info.platform_diver_addr->driver)))
    #endif
    {
        APS_ERR("create attribute err = %d\n", err);
        goto exit_create_attr_failed;
    }

    obj_ps.self = epl2182_obj;

    if( obj->hw->polling_mode_ps)
    {
        obj_ps.polling = 1;
        APS_LOG("ps_interrupt == false\n");
    }
    else
    {
        obj_ps.polling = 0;//interrupt mode
        APS_LOG("ps_interrupt == true\n");
    }


    obj_ps.sensor_operate = epl2182_ps_operate;



    if(err = hwmsen_attach(ID_PROXIMITY, &obj_ps))
    {
        APS_ERR("attach fail = %d\n", err);
        goto exit_create_attr_failed;
    }


    obj_als.self = epl2182_obj;
    obj_als.polling = 1;
    obj_als.sensor_operate = epl2182_als_operate;
    APS_LOG("als polling mode\n");


    if(err = hwmsen_attach(ID_LIGHT, &obj_als))
    {
        APS_ERR("attach fail = %d\n", err);
        goto exit_create_attr_failed;
    }

#if ELAN_WRITE_CALI
    gRawData.ps_als_factory.cal_file_exist = 1;
    gRawData.ps_als_factory.cal_finished = 0;
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND)
    obj->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
    obj->early_drv.suspend  = epl2182_early_suspend,
    obj->early_drv.resume   = epl2182_late_resume,
    register_early_suspend(&obj->early_drv);
#endif

    if(obj->hw->polling_mode_ps == 0 || obj->polling_mode_hs == 0)
        epl2182_setup_eint(client);

    epl2182_init_flag = 0;

    APS_LOG("%s: OK\n", __func__);
    return 0;

exit_create_attr_failed:
    misc_deregister(&epl2182_device);
exit_misc_device_register_failed:
exit_init_failed:
    //i2c_detach_client(client);
//	exit_kfree:
    kfree(obj);
exit:
    epl2182_i2c_client = NULL;
#ifdef MT6516
    MT6516_EINTIRQMask(CUST_EINT_ALS_NUM);  /*mask interrupt if fail*/
#endif
    epl2182_init_flag = -1;
    APS_ERR("%s: err = %d\n", __func__, err);
    return err;



}



/*----------------------------------------------------------------------------*/
static int epl2182_i2c_remove(struct i2c_client *client)
{
    int err;

    #if 0
    if(err = epl2182_delete_attr(&epl2182_i2c_driver.driver))
    #else
    if((err = epl2182_delete_attr(&(epl2182_init_info.platform_diver_addr->driver))))
    #endif
    {
        APS_ERR("epl2182_delete_attr fail: %d\n", err);
    }

    if(err = misc_deregister(&epl2182_device))
    {
        APS_ERR("misc_deregister fail: %d\n", err);
    }

    epl2182_i2c_client = NULL;
    i2c_unregister_device(client);
    kfree(i2c_get_clientdata(client));

    return 0;
}



/*----------------------------------------------------------------------------*/



static int epl2182_probe(struct platform_device *pdev)
{
    struct alsps_hw *hw = EPL2182_get_cust_alsps_hw();

    epl2182_power(hw, 1);

    //epl2182_force[0] = hw->i2c_num;

    if(i2c_add_driver(&epl2182_i2c_driver))
    {
        APS_ERR("add driver error\n");
        return -1;
    }
    return 0;
}



/*----------------------------------------------------------------------------*/
static int epl2182_remove(struct platform_device *pdev)
{
    struct alsps_hw *hw = EPL2182_get_cust_alsps_hw();
    APS_FUN();
    epl2182_power(hw, 0);

    APS_ERR("EPL2182 remove \n");
    i2c_del_driver(&epl2182_i2c_driver);
    return 0;
}


static int epl_remove(void)
{	
    struct alsps_hw *hw = EPL2182_get_cust_alsps_hw();	
    epl2182_power(hw, 0);	
    i2c_del_driver(&epl2182_i2c_driver);	
    return 0;
}

static int  epl2182_local_init(void)
{    
    struct alsps_hw *hw = EPL2182_get_cust_alsps_hw();	
    APS_FUN();	
    epl2182_power(hw, 1);	
    if(i2c_add_driver(&epl2182_i2c_driver))	
    {		
        printk("add driver error\n");		
        return -1;	
    }	
    if(-1 == epl2182_init_flag)	
    {	   
        return -1;	
    }	
    return 0;
}


/*----------------------------------------------------------------------------*/
static struct platform_driver epl2182_alsps_driver =
{
    .probe      = epl2182_probe,
    .remove     = epl2182_remove,
    .driver     = {
        .name  = "als_ps",
        //.owner = THIS_MODULE,
    }
};
/*----------------------------------------------------------------------------*/
static int __init epl2182_init(void)
{
    struct alsps_hw *hw = EPL2182_get_cust_alsps_hw();
    APS_FUN();
    i2c_register_board_info(hw->i2c_num, &i2c_EPL2182, 1);
    #if 0
    if(platform_driver_register(&epl2182_alsps_driver))
    {
        APS_ERR("failed to register driver");
        return -ENODEV;
    }
    #else
    //hwmsen_alsps_sensor_add(&epl2182_init_info);
    alsps_driver_add(&epl2182_init_info);
    #endif

    return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit epl2182_exit(void)
{
    APS_FUN();
    #if 0
    platform_driver_unregister(&epl2182_alsps_driver);
    #endif
}
/*----------------------------------------------------------------------------*/
module_init(epl2182_init);
module_exit(epl2182_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("renato.pan@eminent-tek.com");
MODULE_DESCRIPTION("EPL2182 ALPsr driver");
MODULE_LICENSE("GPL");




