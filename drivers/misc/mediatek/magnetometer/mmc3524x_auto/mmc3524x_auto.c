/************************************************************************************
 ** File: - mmc3524x.c
 ** VENDOR_EDIT
 ** Copyright (C), 2008-2012, OPPO Mobile Comm Corp., Ltd
 ** 
 ** Description: 
 **      Sensor driver, we need to replace but compare because the difference is most.
 ** 
 ** Version: 0.1
 ** Date created: 17:00:00,09/03/2013
 ** Author: Prd.BasicDrv.Sensor
 ** 
 ** --------------------------- Revision History: --------------------------------
 ** 	<author>	                    <data>			<desc>
 ** 
 ************************************************************************************/

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/earlysuspend.h>
#include <linux/time.h>
#include <linux/hrtimer.h>

//#include <mach/mt_devs.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>


#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>

#include <cust_mag.h>
#include "mmc3524x_auto.h"
#include <linux/hwmsen_helper.h>

#include "mag.h"

#ifdef PSEUDOGYRO
#include <linux/oppo_devices_list.h>
extern volatile GYRO_DEV gyro_dev;
#endif /*PSEUDOGYRO*/

#define MMC3524x_DEBUG_MSG	1
#if MMC3524x_DEBUG_MSG
#define MMCDBG(format, ...)	printk( "mmc3524x " format "\n", ## __VA_ARGS__)
#else
#define MMCDBG(format, ...)
#endif

struct mmc3524x_i2c_data 
{
    struct i2c_client *client;
    struct mag_hw *hw; 
    struct hwmsen_convert   cvt;
    atomic_t layout;   
    atomic_t trace;
    atomic_t sensor_suspend_flag;
    atomic_t suspend;
    atomic_t enable_before_resume; 	

#if defined(CONFIG_HAS_EARLYSUSPEND)    
    struct early_suspend    early_drv;
#endif 
};

//#define SENSOR_MOTION
#ifdef SENSOR_MOTION
extern unsigned int sensor_suspend; 
#endif

#define MMC3524X_DEV_NAME	"mmc3524x"

static u32 read_idx = 0;
static struct i2c_client *this_client = NULL;
static struct i2c_board_info __initdata i2c_mmc3524x={ I2C_BOARD_INFO("mmc3524x", (0x60>>1))};

// calibration msensor and orientation data
static int sensor_data[CALIBRATION_DATA_SIZE];

static struct mutex sensor_data_mutex;
static struct mutex read_i2c_xyz;

static DECLARE_WAIT_QUEUE_HEAD(open_wq);

static int I2C_RxData(char *rxData, int length);
static int I2C_TxData(char *txData, int length);

static int mmcd_delay = MMC3524X_DEFAULT_DELAY;

static DEFINE_MUTEX(ecompass_lock);

static atomic_t open_flag = ATOMIC_INIT(0);
static atomic_t m_flag = ATOMIC_INIT(0);
static atomic_t o_flag = ATOMIC_INIT(0);
static int o_flag_num = 0;	// pg and orientation use o_flag, we need count for o_flag

static int platform_mmc3524x_probe(struct platform_device *pdev) ;
static int platform_mmc3524x_remove(struct platform_device *pdev) ;
static struct platform_driver mmc3524x_platform_driver = 
{
    .probe  = platform_mmc3524x_probe,
    .remove = platform_mmc3524x_remove,
    .driver = 
    {
        .name = "msensor_mmc3524x",
    }

};

/*----------------------------------------------------------------------------*/
 static int  mmc3524x_local_init(void);
 static int mmc3524x_remove(void);

extern struct mag_hw* get_cust_mag_hw_mmc3524(void) ;
 static int mmc3524x_init_flag =-1; // 0<==>OK -1 <==> fail
 
 static struct mag_init_info mmc3524x_init_info = {
		 .name = "mmc3524x",
		 .init = mmc3524x_local_init,
		 .uninit = mmc3524x_remove,
	 
 };



/*----------------------------------------------------------------------------*/
static int debugflag = 0;
static ssize_t debugflag_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{	
    char page[8]; 	
    char *p = page;	
    int len = 0; 	
    p += sprintf(p, "%d\n", debugflag);	
    len = p - page;	
    if (len > *pos)		
        len -= *pos;	
    else		
        len = 0;	

    if (copy_to_user(buf,page,len < count ? len  : count))		
        return -EFAULT;	
    *pos = *pos + (len < count ? len  : count);	
    return len < count ? len  : count;
}

static ssize_t debugflag_write(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{	
    char tmp[2] = {0, 0};	
    int ret;		
    if (count > 2)		
        return -EINVAL;		
    ret = copy_from_user(tmp, buf, 2);	
    if ('1' == tmp[0])		
        debugflag = 1;	
    else if ('0' == tmp[0])		
        debugflag = 0;	
    else		
        return -EINVAL;		
    return count;	
}

static struct file_operations debug_fops = 
{
    .read = debugflag_read,
    .write = debugflag_write,
};

static int I2C_RxData(char *rxData, int length)
{
    uint8_t loop_i;

    /* Caller should check parameter validity.*/
    if((rxData == NULL) || (length < 1))
    {
        return -EINVAL;
    }

    for(loop_i = 0; loop_i < MMC3524X_RETRY_COUNT; loop_i++)
    {
        this_client->addr = this_client->addr & I2C_MASK_FLAG | I2C_WR_FLAG;
        if(i2c_master_send(this_client, (const char*)rxData, ((length<<0X08) | 0X01)))
        {
            break;
        }
        printk("I2C_RxData delay!\n");
        mdelay(10);
    }

    if(loop_i >= MMC3524X_RETRY_COUNT)
    {
        printk(KERN_ERR "%s retry over %d\n", __func__, MMC3524X_RETRY_COUNT);
        return -EIO;
    }

    return 0;
}

static int I2C_TxData(char *txData, int length)
{
    uint8_t loop_i;	

    /* Caller should check parameter validity.*/
    if ((txData == NULL) || (length < 2))
    {
        return -EINVAL;
    }

    this_client->addr = this_client->addr & I2C_MASK_FLAG;
    for(loop_i = 0; loop_i < MMC3524X_RETRY_COUNT; loop_i++)
    {
        if(i2c_master_send(this_client, (const char*)txData, length) > 0)
        {
            break;
        }
        printk("I2C_TxData delay!\n");
        mdelay(10);
    }

    if(loop_i >= MMC3524X_RETRY_COUNT) 
    {
        printk(KERN_ERR "%s retry over %d\n", __func__, MMC3524X_RETRY_COUNT);
        return -EIO;
    }

    return 0;
}

static int mmc3xxx_i2c_rx_data(char *buf, int len)
{
    return I2C_RxData(buf,len);
}

static int mmc3xxx_i2c_tx_data(char *buf, int len)
{
    return I2C_TxData(buf,len);
}

static int mmc3524x_dev_init(struct i2c_client *client)
{
    u8 data[2];

    printk("%s!\n", __func__);

    data[0] = MMC3524X_REG_CTRL;
    data[1] = MMC3524X_CTRL_REFILL;
    if (mmc3xxx_i2c_tx_data( data, 2) < 0) {
        return MMC3524X_I2C_ERR;
    }
    msleep(MMC3524X_DELAY_SET);

    data[0] = MMC3524X_REG_CTRL;
    data[1] = MMC3524X_CTRL_SET;
    if (mmc3xxx_i2c_tx_data(  data, 2) < 0) {
        return MMC3524X_I2C_ERR;
    }
    msleep(MMC3524X_I2C_DELAY);

    data[0] = MMC3524X_REG_CTRL;
    data[1] = 0;
    if (mmc3xxx_i2c_tx_data( data, 2) < 0) {
        return MMC3524X_I2C_ERR;
    }
    msleep(MMC3524X_I2C_DELAY);

    data[0] = MMC3524X_REG_CTRL;
    data[1] = MMC3524X_CTRL_REFILL;
    if (mmc3xxx_i2c_tx_data(  data, 2) < 0) {
        return MMC3524X_I2C_ERR;
    }
    msleep(MMC3524X_DELAY_RESET);
    data[0] = MMC3524X_REG_CTRL;
    data[1] = MMC3524X_CTRL_RESET;
    if (mmc3xxx_i2c_tx_data(  data, 2) < 0) {
        return MMC3524X_I2C_ERR;
    }
    msleep(MMC3524X_I2C_DELAY);
    data[0] = MMC3524X_REG_CTRL;
    data[1] = 0;
    if (mmc3xxx_i2c_tx_data( data, 2) < 0) {
        return MMC3524X_I2C_ERR;
    }
    msleep(MMC3524X_I2C_DELAY);

    data[0] = MMC3524X_REG_BITS;
    data[1] = MMC3524X_BITS_SLOW_16;
    if (mmc3xxx_i2c_tx_data(  data, 2) < 0) {
        return MMC3524X_I2C_ERR;
    }
    msleep(MMC3524X_I2C_DELAY);

    data[0] = MMC3524X_REG_CTRL;
    data[1] = MMC3524X_CTRL_TM;
    if (mmc3xxx_i2c_tx_data( data, 2) < 0) {
        return MMC3524X_I2C_ERR;
    }
    msleep(MMC3524X_DELAY_TM);

    return MMC3524X_SUCCESS;
}

// Daemon application save the data
static int ECS_SaveData(int buf[CALIBRATION_DATA_SIZE])
{
    mutex_lock(&sensor_data_mutex);
    memcpy(sensor_data, buf, sizeof(sensor_data));	
    mutex_unlock(&sensor_data_mutex);

    MMCDBG("Get daemon data: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d!\n",
            sensor_data[0],sensor_data[1],sensor_data[2],sensor_data[3],
            sensor_data[4],sensor_data[5],sensor_data[6],sensor_data[7],
            sensor_data[8],sensor_data[9],sensor_data[10],sensor_data[11],
            sensor_data[12],sensor_data[13],sensor_data[14],sensor_data[15]);

    return 0;
}

static int ECS_ReadXYZData(int *vec, int size)
{
    unsigned char data[6] = {0,0,0,0,0,0};
    int MD_times = 0;
    static int last_data[3];
    struct timespec time1, time2, time3;	

    time1 = current_kernel_time();    
    if (vec == NULL || size < 3) 
    {
        return -1;
    }
    mutex_lock(&read_i2c_xyz);
    time2 = current_kernel_time();

    if (!(read_idx % MMC3524X_SET_INTV)) 
    {
        data[0] = MMC3524X_REG_CTRL;
        data[1] = MMC3524X_CTRL_REFILL;
        mmc3xxx_i2c_tx_data(data, 2);
        msleep(MMC3524X_DELAY_SET);

        data[0] = MMC3524X_REG_CTRL;
        data[1] = MMC3524X_CTRL_SET;
        mmc3xxx_i2c_tx_data(data, 2);
        msleep(1);

        data[0] = MMC3524X_REG_CTRL;
        data[1] = 0;
        mmc3xxx_i2c_tx_data(data, 2);
        msleep(1);

        data[0] = MMC3524X_REG_CTRL;
        data[1] = MMC3524X_CTRL_REFILL;
        mmc3xxx_i2c_tx_data(data, 2);
        msleep(MMC3524X_DELAY_RESET);

        data[0] = MMC3524X_REG_CTRL;
        data[1] = MMC3524X_CTRL_RESET;
        mmc3xxx_i2c_tx_data(data, 2);
        msleep(1);

        data[0] = MMC3524X_REG_CTRL;
        data[1] = 0;
        mmc3xxx_i2c_tx_data(data, 2);
        msleep(1); 	
    }
    time3 = current_kernel_time();

    /* send TM cmd before read */
    data[0] = MMC3524X_REG_CTRL;
    data[1] = MMC3524X_CTRL_TM;
    /* not check return value here, assume it always OK */
    if (I2C_TxData(data, 2) )
    {
        msleep(MMC3524X_I2C_DELAY);		

        data[0] = MMC3524X_REG_CTRL;
        data[1] = MMC3524X_CTRL_TM;		
        if ( I2C_TxData( data, 2)) 
        {
            printk("write MMC3524X_CTRL_TM failed!\n");
            mutex_unlock(&read_i2c_xyz);
            return MMC3524X_I2C_ERR;
        }
    }
    msleep(MMC3524X_DELAY_TM);

    /* Read MD */
    data[0] = MMC3524X_REG_DS;
    I2C_RxData(data, 1);
    while (!(data[0] & 0x01)) 
    {
        msleep(1);

        /* Read MD again*/
        data[0] = MMC3524X_REG_DS;
        I2C_RxData(data, 1);
        if (data[0] & 0x01) 
            break;
        MD_times++;
        if (MD_times > 3) 
        {
            printk("TM not work!!");
            mutex_unlock(&read_i2c_xyz);
            return -EFAULT;
        }
    }

    read_idx++;

    data[0] = MMC3524X_REG_DATA;
    if(I2C_RxData(data, 6) < 0)
    {
        mutex_unlock(&read_i2c_xyz);
        return -EFAULT;
    }
    vec[0] = data[1] << 8 | data[0];
    vec[1] = data[3] << 8 | data[2];
    vec[2] = data[5] << 8 | data[4];
    vec[2] = 65536 - vec[2];

    MMCDBG("[X - %d] [Y - %d] [Z - %d]\n", vec[0], vec[1], vec[2]);

    mutex_unlock(&read_i2c_xyz);
    last_data[0] = vec[0];
    last_data[1] = vec[1];
    last_data[2] = vec[2];

    return 0;
}

static int ECS_GetRawData(int data[3])
{
    int err = 0;
    err = ECS_ReadXYZData(data, 3);
    if(err !=0 )
    {
        printk(KERN_ERR "MMC328x_IOC_TM failed\n");
        return -1;
    }

    // sensitivity 512 count = 1 Guass = 100uT
    data[0] = (data[0] - MMC3524X_OFFSET_X) * 100 / MMC3524X_SENSITIVITY_X;
    data[1] = (data[1] - MMC3524X_OFFSET_X) * 100 / MMC3524X_SENSITIVITY_X;
    data[2] = (data[2] - MMC3524X_OFFSET_X) * 100 / MMC3524X_SENSITIVITY_X;

    return err;
}

static int ECS_GetOpenStatus(void)
{
    wait_event_interruptible(open_wq, (atomic_read(&open_flag) != 0));
    return atomic_read(&open_flag);
}

void mmc3524_power(struct mag_hw *hw, unsigned int on) 
{
	static unsigned int power_on = 0;

	if(hw->power_id != MT65XX_POWER_NONE)
	{        
		MMCDBG("power %s\n", on ? "on" : "off");
		if(power_on == on)
		{
			MMCDBG("ignore power control: %d\n", on);
		}
		else if(on)
		{
			if(!hwPowerOn(hw->power_id, hw->power_vol, "mmc3524x")) 
			{
				printk(KERN_ERR "power on fails!!\n");
			}
		}
		else
		{
			if(!hwPowerDown(hw->power_id, "mmc3524x")) 
			{
				printk(KERN_ERR "power off fail!!\n");
			}
		}
	}
	power_on = on;
}

/*----------------------------------------------------------------------------*/
static int mmc3524x_ReadChipInfo(char *buf, int bufsize)
{
    if((!buf)||(bufsize <= MMC3524x_BUFSIZE -1))
    {
        return -1;
    }
    if(!this_client)
    {
        *buf = 0;
        return -2;
    }
    sprintf(buf, "mmc3524x Chip");
    return 0;
}

/*----------------------------------------------------------------------------*/
static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
    char strbuf[MMC3524x_BUFSIZE];
    memset(strbuf,0,MMC3524x_BUFSIZE);
    mmc3524x_ReadChipInfo(strbuf, MMC3524x_BUFSIZE);
    return sprintf(buf, "%s\n", strbuf);        
}
/*----------------------------------------------------------------------------*/
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
    int sensordata[3];
    char strbuf[MMC3524x_BUFSIZE];

    ECS_GetRawData(sensordata);	
    sprintf(strbuf, "%d %d %d\n", sensordata[0],sensordata[1],sensordata[2]);
    return sprintf(buf, "%s\n", strbuf);
}
/*----------------------------------------------------------------------------*/
static ssize_t show_posturedata_value(struct device_driver *ddri, char *buf)
{
    int tmp[3];
    char strbuf[MMC3524x_BUFSIZE];
    tmp[0] = sensor_data[0] * CONVERT_O / CONVERT_O_DIV;				
    tmp[1] = sensor_data[1] * CONVERT_O / CONVERT_O_DIV;
    tmp[2] = sensor_data[2] * CONVERT_O / CONVERT_O_DIV;
    sprintf(strbuf, "%d, %d, %d\n", tmp[0],tmp[1], tmp[2]);		
    return sprintf(buf, "%s\n", strbuf);;           
}

/*----------------------------------------------------------------------------*/
static ssize_t show_direction_value(struct device_driver *ddri, char *buf)
{
    struct i2c_client *client = this_client;  
    struct mmc3524x_i2c_data *data = i2c_get_clientdata(client);
    return sprintf(buf, "(%d, %d)\n[%+2d %+2d %+2d]\n[%+2d %+2d %+2d]\n",
            data->hw->direction,atomic_read(&data->layout),	data->cvt.sign[0], data->cvt.sign[1],
            data->cvt.sign[2],data->cvt.map[0], data->cvt.map[1], data->cvt.map[2]);            
}

/*----------------------------------------------------------------------------*/
static ssize_t store_direction_value(struct device_driver *ddri, char *buf, size_t count)
{
    struct i2c_client *client = this_client;  
    struct mmc3524x_i2c_data *data = i2c_get_clientdata(client);
    int layout = 0;

    if(1 == sscanf(buf, "%d", &layout))
    {
        atomic_set(&data->layout, layout);
        if(!hwmsen_get_convert(layout, &data->cvt))
        {
            printk(KERN_ERR "HWMSEN_GET_CONVERT function error!\r\n");
        } 
        else if(!hwmsen_get_convert(data->hw->direction, &data->cvt))
        {
            printk(KERN_ERR "invalid layout: %d, restore to %d\n", layout, data->hw->direction);
        } 
        else 
        {
            printk(KERN_ERR "invalid layout: (%d, %d)\n", layout, data->hw->direction);
            hwmsen_get_convert(0, &data->cvt);
        }
    } 
    else 
    {
        printk(KERN_ERR "invalid format = '%s'\n", buf);
    }

    return count;            
}

/*----------------------------------------------------------------------------*/
static ssize_t show_status_value(struct device_driver *ddri, char *buf)
{
    char strbuf[MMC3524x_BUFSIZE];

    sprintf(strbuf, "%d\n", sensor_data[11]);	// status

    printk("The status of M-sensor is %d\n", sensor_data[11]);

    return sprintf(buf, "%s\n", strbuf);
}

/*----------------------------------------------------------------------------*/
static ssize_t show_trace_value(struct device_driver *ddri, char *buf)
{
    ssize_t res;
    struct mmc3524x_i2c_data *obj = i2c_get_clientdata(this_client);
    if(NULL == obj)
    {
        printk(KERN_ERR "mmc328x_i2c_data is null!!\n");
        return 0;
    }		
    res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));     
    return res;    
}

/*----------------------------------------------------------------------------*/
static ssize_t store_trace_value(struct device_driver *ddri, char *buf, size_t count)
{
    struct mmc3524x_i2c_data *obj = i2c_get_clientdata(this_client);
    int trace;
    if(NULL == obj)
    {
        printk(KERN_ERR "mmc328x_i2c_data is null!!\n");
        return 0;
    }

    if(1 == sscanf(buf, "0x%x", &trace)) 
    {
        atomic_set(&obj->trace, trace);
    } 
    else 
    {
        printk(KERN_ERR "invalid content: '%s', length = %d\n", buf, count);
    }

    return count;    
}

static ssize_t show_daemon_name(struct device_driver *ddri, char *buf)
{
    char strbuf[MMC3524x_BUFSIZE];
    sprintf(strbuf, "memsicd35240");
    //sprintf(strbuf, "memsicd3416x");
    return sprintf(buf, "%s", strbuf);		
}

//for msesensor engineer mode auto test
#define abs_msensor(a) (((a) < 0) ? -(a) : (a))
int data_initial[3] = {0, 0, 0};
int data_magnet_close[3] = {0, 0, 0};
int data_magnet_leave[3] = {0, 0, 0};

static ssize_t show_autotest_testID(struct device_driver *ddri, char *buf)
{
    mmc3524x_dev_init(NULL);

    /*Read initial data*/
    ECS_ReadXYZData(data_initial, 3);

    /*Data availble??*/
    if ((65535 == data_initial[0])
            || (65535 == data_initial[1])
            || (65535 == data_initial[2]))
        goto READ_DATA_FAIL;

    printk("%s : data_initial[x] = %d, data_initial[y] = %d, data_initial[z] = %d\n",
            __func__, data_initial[0], data_initial[1], data_initial[2]);

    return 0;

READ_ID_FAIL:
    printk(KERN_ERR"Auto Test: read id error!!");
    return -1;
RESET_DEV_FAIL:
    printk(KERN_ERR"Auto Test: reset device error!!");
    return -1;
READ_DATA_FAIL:
    printk(KERN_ERR"Auto Test: read data error!!");
    return -1;
}


static ssize_t show_autotest_magnetclose(struct device_driver *ddri, char *buf)
{
    int i ;
    /*50 loops which take about 5 seconds*/
    for (i = 0; i < 50; i ++)
    {
        if (ECS_ReadXYZData(data_magnet_close, 3))
        {
            MMCDBG(KERN_ERR"Auto Test:close read data fail");
            break;
        }

        /*Data availble??*/
        if ((65535 == data_magnet_close[0])
                || (65535 == data_magnet_close[1])
                || (65535 == data_magnet_close[2]))
        {
            MMCDBG(KERN_ERR"Auto Test:close read data fail");
            break;
        }

        MMCDBG(KERN_ERR"%d, %d, %d\n", data_magnet_close[0], data_magnet_close[1], data_magnet_close[2]);

        if ((abs_msensor(data_magnet_close[0] - data_initial[0]) >= 50)
                && (abs_msensor(data_magnet_close[1] - data_initial[1]) >= 50)
                && (abs_msensor(data_magnet_close[2] - data_initial[2]) >= 50))
            return 0;

        msleep(100);
    }
    return -1;
}

static ssize_t show_autotest_magnetleave(struct device_driver *ddri, char *buf)
{
    return 0;
}

static ssize_t show_autotest_get_ic_mode(struct device_driver *ddri, char *buf)	
{
    printk("msensor is mmc416x\n");
    return 0;
}

/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(daemon,      S_IRUGO, show_daemon_name, NULL);
static DRIVER_ATTR(chipinfo,    S_IRUGO, show_chipinfo_value, NULL);
static DRIVER_ATTR(sensordata,  S_IRUGO, show_sensordata_value, NULL);
static DRIVER_ATTR(posturedata, S_IRUGO, show_posturedata_value, NULL);
static DRIVER_ATTR(direction,      S_IRUGO | S_IWUSR, show_direction_value, store_direction_value );
static DRIVER_ATTR(status,      S_IRUGO, show_status_value, NULL);
static DRIVER_ATTR(trace,       S_IRUGO | S_IWUSR, show_trace_value, store_trace_value);

//add for msensor engineer auto test
static DRIVER_ATTR(test_id,       S_IRUGO, show_autotest_testID, NULL);
static DRIVER_ATTR(magnet_close,  S_IRUGO, show_autotest_magnetclose, NULL);
static DRIVER_ATTR(magnet_leave,  S_IRUGO, show_autotest_magnetleave, NULL);
static DRIVER_ATTR(get_ic_modle,  S_IRUGO, show_autotest_get_ic_mode, NULL);

/*----------------------------------------------------------------------------*/
static struct driver_attribute *mmc3524x_attr_list[] = {
    &driver_attr_daemon,
    &driver_attr_chipinfo,
    &driver_attr_sensordata,
    &driver_attr_posturedata,
    &driver_attr_direction,
    &driver_attr_status,
    &driver_attr_trace,

    //add for msensor engineer auto test
    &driver_attr_test_id,
    &driver_attr_magnet_close,
    &driver_attr_magnet_leave,
    &driver_attr_get_ic_modle,	
};

/*----------------------------------------------------------------------------*/
static int mmc3524x_create_attr(struct device_driver *driver) 
{
    int idx, err = 0;
    int num = (int)(sizeof(mmc3524x_attr_list)/sizeof(mmc3524x_attr_list[0]));
    if (driver == NULL)
    {
        return -EINVAL;
    }

    for(idx = 0; idx < num; idx++)
    {
        if((err = driver_create_file(driver, mmc3524x_attr_list[idx])))
        {            
            printk(KERN_ERR "driver_create_file (%s) = %d\n", mmc3524x_attr_list[idx]->attr.name, err);
            break;
        }
    }

    return err;
}
/*----------------------------------------------------------------------------*/
static int mmc3524x_delete_attr(struct device_driver *driver)
{
    int idx;
    int num = (int)(sizeof(mmc3524x_attr_list)/sizeof(mmc3524x_attr_list[0]));

    if(driver == NULL)
    {
        return -EINVAL;
    }

    for(idx = 0; idx < num; idx++)
    {
        driver_remove_file(driver, mmc3524x_attr_list[idx]);
    }

    return 0;
}

static int mmc3524x_open(struct inode *inode, struct file *file)
{
    printk("mmc3524x_open !!\n");
    return nonseekable_open(inode, file);
}

static int mmc3524x_release(struct inode *inode, struct file *file)
{
    return 0;
}
#define MMC3524X_ECOMPASS_IOC_SET_YPR	_IOW(MSENSOR,  0x21, int[16])
static long mmc3524x_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    void __user *pa = (void __user *)arg;
    unsigned char data[16] = {0};
    char buff[MMC3524x_BUFSIZE];
    int vec[3] = {0};
    int value[16];			/* for SET_YPR */
    int status;
    short sensor_status;	/* for Orientation and Msensor status */
    short delay;
//    hwm_sensor_data* osensor_data;
    uint32_t enable;
    struct i2c_client *client = this_client;  
    struct mmc3524x_i2c_data *clientdata = i2c_get_clientdata(client);
	//++add by zyx
	unsigned char reg_addr;
	unsigned char reg_value;
	read_reg_str reg_str;
	int data_reg[3] = {0};
	//--add by zyx

    //MMCDBG("mmc3524x_ioctl enter! cmd=%x\n",cmd);

    //mutex_lock(&ecompass_lock);
    switch (cmd) 
    {
        case MMC31XX_IOC_TM:
            printk("MMC3524X_IOC_TM \n");
            data[0] = MMC3524X_REG_CTRL;
            data[1] = MMC3524X_CTRL_TM;
            if (mmc3xxx_i2c_tx_data(data, 2) < 0) 
            {
                //mutex_unlock(&ecompass_lock);
                return -EFAULT;
            }
            /* wait TM done for coming data read */
            //msleep(MMC3524X_DELAY_TM);
            break;

        case MMC31XX_IOC_SET:
            printk("MMC3524X_IOC_SET \n");
            data[0] = MMC3524X_REG_CTRL;
            data[1] = MMC3524X_CTRL_REFILL;
            if (mmc3xxx_i2c_tx_data(data, 2) < 0) 
            {
                //mutex_unlock(&ecompass_lock);
                return -EFAULT;
            }
            msleep(MMC3524X_DELAY_SET);
            data[0] = MMC3524X_REG_CTRL;
            data[1] = MMC3524X_CTRL_SET;
            if (mmc3xxx_i2c_tx_data(data, 2) < 0) 
            {
                //mutex_unlock(&ecompass_lock);
                return -EFAULT;
            }
            msleep(1);
            data[0] = MMC3524X_REG_CTRL;
            data[1] = 0;
            if (mmc3xxx_i2c_tx_data(data, 2) < 0) 
            {
                // mutex_unlock(&ecompass_lock);
                return -EFAULT;
            }
            msleep(MMC3524X_I2C_DELAY);
            break;

        case MMC31XX_IOC_RESET:
            printk("MMC3524X_IOC_RESET \n");
            data[0] = MMC3524X_REG_CTRL;
            data[1] = MMC3524X_CTRL_REFILL;
            if (mmc3xxx_i2c_tx_data(data, 2) < 0) 
            {
                // mutex_unlock(&ecompass_lock);
                return -EFAULT;
            }
            msleep(MMC3524X_DELAY_RESET);
            data[0] = MMC3524X_REG_CTRL;
            data[1] = MMC3524X_CTRL_RESET;
            if (mmc3xxx_i2c_tx_data(data, 2) < 0) 
            {
                // mutex_unlock(&ecompass_lock);
                return -EFAULT;
            }
            msleep(MMC3524X_I2C_DELAY);
            data[0] = MMC3524X_REG_CTRL;
            data[1] = 0;
            if (mmc3xxx_i2c_tx_data(data, 2) < 0) 
            {
                //mutex_unlock(&ecompass_lock);
                return -EFAULT;
            }
            msleep(MMC3524X_I2C_DELAY);
            break;

        case MMC31XX_IOC_READ:
            printk("MMC3524X_IOC_READ \n");
			data[0] = MMC3524X_REG_DATA;
			if (mmc3xxx_i2c_rx_data(data, 6) < 0) {
					//	mutex_unlock(&ecompass_lock);
				return -EFAULT;
			}
			data_reg[0] = data[1] << 8 | data[0];
			data_reg[1] = data[3] << 8 | data[2];
			data_reg[2] = data[5] << 8 | data[4];
			
			vec[0] = data_reg[0];
			vec[1] = data_reg[1]  - data_reg[2]  + 32768;
			vec[2] = data_reg[1]  + data_reg[2]  - 32768;

            //vec[2] = 65536 - vec[2];

            printk(" mmc3524 raw data [X - %d] [Y - %d] [Z - %d]\n", vec[0], vec[1], vec[2]);
            if (copy_to_user(pa, vec, sizeof(vec)))
            {
                //mutex_unlock(&ecompass_lock);
                return -EFAULT;
            }
            break;

        case MMC31XX_IOC_READXYZ:
            MMCDBG("MMC3524X_IOC_READXYZ \n");
            if (ECS_ReadXYZData(vec, 3) )
            {
                //mutex_unlock(&ecompass_lock);
                return -EFAULT;
            }

            MMCDBG("[X - %04x] [Y - %04x] [Z - %04x]\n", vec[0], vec[1], vec[2]);
            if (copy_to_user(pa, vec, sizeof(vec))) 
            {
                // mutex_unlock(&ecompass_lock);
                return -EFAULT;
            }
            break;
		//++add by zyx
		
		case MMC3524X_IOC_READ_REG:
		     printk("MMC3524X_IOC_READ_REG \n");
			if (copy_from_user(&reg_addr, pa, sizeof(delay)))
				return -EFAULT;
			data[0] = reg_addr;
			if (mmc3xxx_i2c_rx_data(data, 1) < 0) {
				//mutex_unlock(&ecompass_lock);
				return -EFAULT;
			}
			printk("<7>planar Read register No. 0x%0x\n", data[0]);
			reg_value = data[0];
			if (copy_to_user(pa, &reg_value, sizeof(reg_value))) {
				//mutex_unlock(&ecompass_lock);
				return -EFAULT;
			}		
			break;
		case MMC3524X_IOC_WRITE_REG:
		     printk(" MMC3524X_IOC_WRITE_REG \n");
			if (copy_from_user(&data, pa, sizeof(data)))
			return -EFAULT;
			if (mmc3xxx_i2c_tx_data(data, 2) < 0) {
			//mutex_unlock(&ecompass_lock);
			return -EFAULT;
			}
			printk("<7>planar Write '0x%0x' to  register No. 0x%0x\n", data[0], data[1]);
			
		    break; 
		case MMC3524X_IOC_READ_REGS:
		     printk(" MMC3524X_IOC_READ_REGS \n");
		if (copy_from_user(&data, pa, sizeof(data)))
			return -EFAULT;
		printk("<7> planar Read %d registers from 0x%0x\n", data[1], data[0]);	
		if (mmc3xxx_i2c_rx_data(data, data[1]) < 0) {
			//mutex_unlock(&ecompass_lock);
			return -EFAULT;
		}
		printk("<7> data: %x %x %x \n%x %x %x\n", data[0], data[1], data[2], data[3], data[4], data[5]);	
		if (copy_to_user(pa, data, sizeof(data))) {
			//mutex_unlock(&ecompass_lock);
			return -EFAULT;
		}		
		break; 

		//--add by zyx

        case ECOMPASS_IOC_GET_DELAY:			
            delay = mmcd_delay;
            if(copy_to_user(pa, &delay, sizeof(delay)))
            {
                // mutex_unlock(&ecompass_lock);
                printk(KERN_ERR "copy_to_user failed.");
                return -EFAULT;
            }
            break;		
		case MMC3524X_ECOMPASS_IOC_SET_YPR:
        case ECOMPASS_IOC_SET_YPR:
            MMCDBG("ECOMPASS_IOC_SET_YPR \n");			
            if(pa == NULL)
            {
                //mutex_unlock(&ecompass_lock);
                MMCDBG("invalid argument.");
                return -EINVAL;
            }
            if(copy_from_user(value, pa, sizeof(value)))
            {
                // mutex_unlock(&ecompass_lock);
                MMCDBG("copy_from_user failed.");
                return -EFAULT;
            }
            ECS_SaveData(value);	// get Daemon Data 
            break;

        case ECOMPASS_IOC_GET_OPEN_STATUS:
            MMCDBG("ECOMPASS_IOC_GET_OPEN_STATUS \n");				
            status = ECS_GetOpenStatus();			
            if(copy_to_user(pa, &status, sizeof(status)))
            {
                // mutex_unlock(&ecompass_lock);
                MMCDBG("copy_to_user failed.");
                return -EFAULT;
            }
            break;

        case ECOMPASS_IOC_GET_MFLAG:
            sensor_status = atomic_read(&m_flag);
            if(copy_to_user(pa, &sensor_status, sizeof(sensor_status)))
            {
                //mutex_unlock(&ecompass_lock);
                MMCDBG("copy_to_user failed.");
                return -EFAULT;
            }
            break;

        case ECOMPASS_IOC_GET_OFLAG:
            sensor_status = atomic_read(&o_flag);
            if(copy_to_user(pa, &sensor_status, sizeof(sensor_status)))
            {
                //mutex_unlock(&ecompass_lock);
                MMCDBG("copy_to_user failed.");
                return -EFAULT;
            }
            break;	

        case MSENSOR_IOCTL_READ_CHIPINFO:
            if(pa == NULL)
            {
                //mutex_unlock(&ecompass_lock);
                printk(KERN_ERR "IO parameter pointer is NULL!\r\n");
                break;
            }

            mmc3524x_ReadChipInfo(buff, MMC3524x_BUFSIZE);
            if(copy_to_user(pa, buff, strlen(buff)+1))
            {
                //mutex_unlock(&ecompass_lock);
                return -EFAULT;
            }                
            break;

        case MSENSOR_IOCTL_READ_SENSORDATA:	
            MMCDBG("MSENSOR_IOCTL_READ_SENSORDATA \n");				
            if(pa == NULL)
            {
                //mutex_unlock(&ecompass_lock);
                printk(KERN_ERR "IO parameter pointer is NULL!\r\n");
                break;    
            }
            ECS_GetRawData(vec);			
            sprintf(buff, "%x %x %x", vec[0], vec[1], vec[2]);
            if(copy_to_user(pa, buff, strlen(buff)+1))
            {
                //mutex_unlock(&ecompass_lock);
                return -EFAULT;
            }                
            break;

        case ECOMPASS_IOC_GET_LAYOUT:
            status = atomic_read(&clientdata->layout);
            if(copy_to_user(pa, &status, sizeof(status)))
            {
                MMCDBG("copy_to_user failed.");
                //mutex_unlock(&ecompass_lock);
                return -EFAULT;
            }
            break;

        case MSENSOR_IOCTL_SENSOR_ENABLE:
            if(pa == NULL)
            {
                printk(KERN_ERR "IO parameter pointer is NULL!\r\n");
                break;
            }
            if(copy_from_user(&enable, pa, sizeof(enable)))
            {
                MMCDBG("copy_from_user failed.");
                //mutex_unlock(&ecompass_lock);
                return -EFAULT;
            }
            else
            {
                printk( "MSENSOR_IOCTL_SENSOR_ENABLE enable=%d!\r\n",enable);
                if(1 == enable)
                {
                    atomic_set(&o_flag, 1);
                    atomic_set(&open_flag, 1);
                }
                else
                {
                    atomic_set(&o_flag, 0);
                    if(atomic_read(&m_flag) == 0)
                    {
                        atomic_set(&open_flag, 0);
                    }			
                }
                wake_up(&open_wq);
            }

            break;

        case MSENSOR_IOCTL_READ_FACTORY_SENSORDATA:			
            if(pa == NULL)
            {
                printk(KERN_ERR "IO parameter pointer is NULL!\r\n");
                break;    
            }
/*
            //AKECS_GetRawData(buff, AKM8975_BUFSIZE);
            osensor_data = (hwm_sensor_data *)buff;
            mutex_lock(&sensor_data_mutex);

            osensor_data->values[0] = sensor_data[8] * CONVERT_O;
            osensor_data->values[1] = sensor_data[9] * CONVERT_O;
            osensor_data->values[2] = sensor_data[10] * CONVERT_O;
            osensor_data->status = sensor_data[11];
            osensor_data->value_divide = CONVERT_O_DIV;

            mutex_unlock(&sensor_data_mutex);
*/
            sprintf(buff, "%x %x %x %x %x", sensor_data[8] * CONVERT_O, sensor_data[9] * CONVERT_O,
                    sensor_data[10] * CONVERT_O,sensor_data[11],CONVERT_O_DIV);

            if(copy_to_user(pa, buff, strlen(buff)+1))
            {
                //mutex_unlock(&ecompass_lock);
                return -EFAULT;
            } 

            break;
	case CKT_MSENSOR_IOCTL_READ_FACTORY_SENSORDATA:
		MMCDBG("MSENSOR_IOCTL_READ_SENSORDATA \n");				
            if(pa == NULL)
            {
                //mutex_unlock(&ecompass_lock);
                printk(KERN_ERR "IO parameter pointer is NULL!\r\n");
                break;    
            }
            ECS_GetRawData(vec);			
            sprintf(buff, "%04x %04x %04x", vec[0], vec[1], vec[2]);
            if(copy_to_user(pa, buff, strlen(buff)+1))
            {
                //mutex_unlock(&ecompass_lock);
                return -EFAULT;
            }                
		break;

        default:
            MMCDBG("No command failed.\n");
            break;
    }
    //mutex_unlock(&ecompass_lock);
    return 0;
}

static struct file_operations mmc3524x_fops = {
    .owner		= THIS_MODULE,
    .open		= mmc3524x_open,
    .release	= mmc3524x_release,
    .unlocked_ioctl = mmc3524x_ioctl,
};

static struct miscdevice mmc3524x_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "msensor",
    .fops = &mmc3524x_fops,
};

/*----------------------------------------------------------------------------*/
#ifndef	CONFIG_HAS_EARLYSUSPEND
static int mmc3524x_suspend(struct i2c_client *client, pm_message_t msg) 
{
    struct mmc3524x_i2c_data *obj = i2c_get_clientdata(client);
    if(msg.event == PM_EVENT_SUSPEND)
    {
        mmc3524_power(obj->hw, 0);
    }
    return 0;
}

static int mmc3524x_resume(struct i2c_client *client)
{
    struct mmc3524x_i2c_data *obj = i2c_get_clientdata(client);
    mmc3524_power(obj->hw, 1);	
    return 0;
}
#else /*CONFIG_HAS_EARLY_SUSPEND is defined*/

static void mmc3524x_early_suspend(struct early_suspend *h) 
{
    struct mmc3524x_i2c_data *obj = container_of(h, struct mmc3524x_i2c_data, early_drv);   

    if(NULL == obj) {
        printk(KERN_ERR "null pointer!!\n");
        return;
    }

#ifdef SENSOR_MOTION
    if(sensor_suspend == 0)
    {
        atomic_set(&obj->sensor_suspend_flag, 1);

        printk("%s, Need motion sensing, PG not suspend and works normally !\n", __func__);

        return ;
    }
    else
    {
        printk("%s, Motion sensing switch closed , PG will suspend !\n", __func__);
    }
#endif

    atomic_set(&obj->suspend, 1);	
    mmc3524_power(obj->hw, 0);      

    printk("%s\n", __func__);
}
/*----------------------------------------------------------------------------*/
static void mmc3524x_late_resume(struct early_suspend *h)
{
    struct mmc3524x_i2c_data *obj = container_of(h, struct mmc3524x_i2c_data, early_drv);         

    if(NULL == obj) {
        printk(KERN_ERR "null pointer!!\n");
        return;
    }

#ifdef SENSOR_MOTION
    if(atomic_read(&obj->sensor_suspend_flag))	
    {
        atomic_set(&obj->sensor_suspend_flag, 0);

        printk("%s: Need motion sensing , PG not suspend and works normally !\n", __func__);

        return ;
    }
    else
    {
        printk("%s, Motion sensing switch closed , PG will resume !\n", __func__);
    }																		
#endif

    mmc3524_power(obj->hw, 1);

    if (atomic_read(&obj->enable_before_resume)) 
    {
        wake_up(&open_wq);	
        atomic_set(&obj->enable_before_resume, 0);
    }

    atomic_set(&obj->suspend, 0);

    printk("%s\n", __func__);	
}

#endif /*CONFIG_HAS_EARLYSUSPEND*/

/*----------------------------------------------------------------------------*/
int mmc3524x_operate(void* self, uint32_t command, void* buff_in, int size_in,
        void* buff_out, int size_out, int* actualout)
{
    int err = 0;
    int value;
    hwm_sensor_data* msensor_data;	
    struct i2c_client *client = this_client;  
    struct mmc3524x_i2c_data *data = i2c_get_clientdata(client);

    if (atomic_read(&data->suspend)) 
    {
        if (SENSOR_ENABLE == command) 
        {
            value = *(int *)buff_in;
            if(value == 1)
            {
                atomic_set(&m_flag, 1);
                atomic_set(&open_flag, 1);
            } 
            else 
            {
                atomic_set(&m_flag, 0);
                if(atomic_read(&o_flag) == 0)
                {
                    atomic_set(&open_flag, 0);					
                }
            }
            atomic_set(&data->enable_before_resume, 1);

            printk("enable_before_resume, value = %d\n", value);
        }
        return 0;
    }

    switch (command)
    {
        case SENSOR_DELAY:
            if((buff_in == NULL) || (size_in < sizeof(int)))
            {
                printk(KERN_ERR "Set delay parameter error!\n");
                err = -EINVAL;
            } 
            else 
            {
                value = *(int *)buff_in;
                printk("%s, SENSOR_DELAY = %d\n", __func__, value);

                mmcd_delay = value;
                if(value <= 20)
                {
                    mmcd_delay = 20;
                }				
            }	
            break;

        case SENSOR_ENABLE:
            if((buff_in == NULL) || (size_in < sizeof(int))){
                printk(KERN_ERR "Enable sensor parameter error!\n");
                err = -EINVAL;
            } 
            else 
            {
                value = *(int *)buff_in;
                printk("%s, SENSOR_ENABLE = %d\n", __func__, value);

                if(value == 1)
                {
                    atomic_set(&m_flag, 1);
                    atomic_set(&open_flag, 1);
                } 
                else 
                {
                    atomic_set(&m_flag, 0);
                    if(atomic_read(&o_flag) == 0)
                    {
                        atomic_set(&open_flag, 0);
                    }
                }
                wake_up(&open_wq);				
                // TODO: turn device into standby or normal mode
            }
            break;

        case SENSOR_GET_DATA:
            if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data))){
                printk(KERN_ERR "get sensor data parameter error!\n");
                err = -EINVAL;
            } else {
                msensor_data = (hwm_sensor_data *)buff_out;
                mutex_lock(&sensor_data_mutex);

                // unit : ut
                msensor_data->values[0] = sensor_data[4] * CONVERT_M;	// CONVERT_M = 25
                msensor_data->values[1] = sensor_data[5] * CONVERT_M;
                msensor_data->values[2] = sensor_data[6] * CONVERT_M;
                msensor_data->status = sensor_data[11];

                msensor_data->value_divide = CONVERT_M_DIV;				// CONVERT_M_DIV = 8192
                mutex_unlock(&sensor_data_mutex);

                MMCDBG("Hwm get m-sensor data: x = %d, y = %d, z = %d. divide = %d, status = %d!\n",
                        msensor_data->values[0],msensor_data->values[1],msensor_data->values[2],
                        msensor_data->value_divide,msensor_data->status);
            }
            break;
        default:
            printk(KERN_ERR "msensor operate function no this parameter %d!\n", command);
            err = -1;
            break;
    }

    return err;
}

/*----------------------------------------------------------------------------*/
int mmc3524x_orientation_operate(void* self, uint32_t command, void* buff_in, int size_in,
        void* buff_out, int size_out, int* actualout)
{
    int err = 0;
    int value;
//    hwm_sensor_data* osensor_data;		
    struct i2c_client *client = this_client;  
    struct mmc3524x_i2c_data *data = i2c_get_clientdata(client);

    // when reboot, hal will disable every sensor once, and o_flag_num will be negative
    if(o_flag_num < 0)	
        o_flag_num = 0;

    if (atomic_read(&data->suspend))
    {
        if (SENSOR_ENABLE == command)
        {
            value = *(int *)buff_in;
            if(value == 1)
            {
                atomic_set(&o_flag, 1);
                atomic_set(&open_flag, 1);
            } 
            else 
            {
                atomic_set(&o_flag, 0);
                if(atomic_read(&m_flag) == 0)
                {
                    atomic_set(&open_flag, 0);
                }									
            }	
            atomic_set(&data->enable_before_resume, 1);

            printk("enable_before_resume, value = %d\n", value);
        }
        return 0;
    }

    switch (command)
    {
        case SENSOR_DELAY:
            if((buff_in == NULL) || (size_in < sizeof(int)))
            {
                printk(KERN_ERR "Set delay parameter error!\n");
                err = -EINVAL;
            } 
            else 
            {
                value = *(int *)buff_in;
                printk("%s, SENSOR_DELAY = %d\n", __func__, value);

                mmcd_delay = value;
                if(value <= 20)
                {
                    mmcd_delay = 20;
                }				
            }	
            break;

        case SENSOR_ENABLE:
            if((buff_in == NULL) || (size_in < sizeof(int))) 
            {
                printk(KERN_ERR "Enable sensor parameter error!\n");
                err = -EINVAL;
            } 
            else 
            {				
                value = *(int *)buff_in;
                printk("%s, SENSOR_ENABLE = %d\n", __func__, value);

                if(value == 1)
                {
                    o_flag_num ++;
                    atomic_set(&o_flag, 1);
                    atomic_set(&open_flag, 1);
                } 
                else 
                {
                    o_flag_num --;
                    if(o_flag_num == 0)
                    {
                        atomic_set(&o_flag, 0);
                        if(atomic_read(&m_flag) == 0)
                        {
                            atomic_set(&open_flag, 0);
                        }
                    }
                }	
                wake_up(&open_wq);
            }
            break;

        case SENSOR_GET_DATA:
            if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data))){
                printk(KERN_ERR "get sensor data parameter error!\n");
                err = -EINVAL;
            } else {
/*                osensor_data = (hwm_sensor_data *)buff_out;
                mutex_lock(&sensor_data_mutex);

                // unit : degree
                osensor_data->values[0] = sensor_data[8] * CONVERT_O;	// Yaw // CONVERT_O = 45
                osensor_data->values[1] = sensor_data[9] * CONVERT_O;	// Pitch
                osensor_data->values[2] = sensor_data[10] * CONVERT_O;	// Roll
                osensor_data->status = sensor_data[11];					
                osensor_data->value_divide = CONVERT_O_DIV;				// CONVERT_O_DIV = 8192
                mutex_unlock(&sensor_data_mutex);
*/
                MMCDBG("Hwm get o-sensor data: x = %d, y = %d, z = %d. divide = %d, status = %d!\n",
                        sensor_data[8] * CONVERT_O,sensor_data[9] * CONVERT_O,sensor_data[10] * CONVERT_O,
                        CONVERT_O_DIV,sensor_data[11]);
            }
            break;
        default:
            printk(KERN_ERR "gsensor operate function no this parameter %d!\n", command);
            err = -1;
            break;
    }

    return err;
}

#ifdef PSEUDOGYRO
int mmc3524x_gyro_operate(void* self, uint32_t command, void* buff_in, int size_in,
        void* buff_out, int size_out, int* actualout)
{
    int err = 0;
    int value;
    hwm_sensor_data* gysensor_data;	

    // when reboot, hal will disable every sensor once, and o_flag_num will be negative
    if(o_flag_num < 0)	
        o_flag_num = 0;

    switch (command)
    {
        case SENSOR_DELAY:
            if((buff_in == NULL) || (size_in < sizeof(int)))
            {
                printk(KERN_ERR "Set delay parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                value = *(int *)buff_in;
                printk("%s, SENSOR_DELAY = %d\n", __func__, value);

                if(value <= 20)
                {
                    mmcd_delay = 20;
                }
                mmcd_delay = value;
            }	
            break;

        case SENSOR_ENABLE:
            if((buff_in == NULL) || (size_in < sizeof(int)))
            {
                printk(KERN_ERR "Enable sensor parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                value = *(int *)buff_in;
                printk("%s, SENSOR_ENABLE = %d\n", __func__, value);

                if(value == 1)
                {
                    o_flag_num ++;
                    atomic_set(&o_flag, 1);                
                    atomic_set(&open_flag, 1);
                }
                else
                {
                    o_flag_num --;
                    if(o_flag_num == 0)
                    {
                        atomic_set(&o_flag, 0);       
                        if(atomic_read(&m_flag) == 0)
                        {
                            atomic_set(&open_flag, 0);
                        }
                    }
                }	
                wake_up(&open_wq);
            }
            break;

        case SENSOR_GET_DATA:
            if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
            {
                printk(KERN_ERR "get sensor data parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                gysensor_data = (hwm_sensor_data *)buff_out;
                mutex_lock(&sensor_data_mutex);

                gysensor_data->values[0] = sensor_data[12] * CONVERT_GY;
                gysensor_data->values[1] = sensor_data[13] * CONVERT_GY;
                gysensor_data->values[2] = sensor_data[14] * CONVERT_GY;
                gysensor_data->status = sensor_data[15];
                gysensor_data->value_divide = CONVERT_GY_DIV;

                mutex_unlock(&sensor_data_mutex);

                MMCDBG("Hwm get gyro-sensor data: x = %d, y = %d, z = %d. divide = %d, status = %d!\n",
                        gysensor_data->values[0],gysensor_data->values[1],gysensor_data->values[2],
                        gysensor_data->value_divide,gysensor_data->status);

            }
            break;
        default:
            printk(KERN_ERR "gsensor operate function no this parameter %d!\n", command);
            err = -1;
            break;
    }

    return err;
}
#endif/*PSEUDOGYRO*/

static int mmc3524x_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct i2c_client *new_client;
    struct mmc3524x_i2c_data *data;
    int err = 0;
    struct hwmsen_object sobj_m, sobj_o, sobj_gy;

    MMCDBG("%s! \n", __func__);
    if(!(data = kmalloc(sizeof(struct mmc3524x_i2c_data), GFP_KERNEL)))
    {
        err = -ENOMEM;
        goto exit;
    }

    memset(data, 0, sizeof(struct mmc3524x_i2c_data));
    data->hw = get_cust_mag_hw_mmc3524();	 

    atomic_set(&data->layout, data->hw->direction);
    atomic_set(&data->trace, 0);
    atomic_set(&data->suspend, 0);
    atomic_set(&data->enable_before_resume, 0);

    mutex_init(&sensor_data_mutex);
    mutex_init(&read_i2c_xyz);	

    init_waitqueue_head(&open_wq);

    data->client = client;
    new_client = data->client;
    i2c_set_clientdata(new_client, data);	
    this_client = new_client;	

    if ((err = mmc3524x_dev_init(client)) < 0) 
    {
        MMCDBG("init device error!\n");
        goto exit_init_failed;
    }

    /* Register sysfs attribute */
    //if((err = mmc3524x_create_attr(&mmc3524x_platform_driver.driver)))
    if((err = mmc3524x_create_attr(&(mmc3524x_init_info.platform_diver_addr->driver))))
    {
        printk(KERN_ERR "create attribute err = %d\n", err);
        goto exit_sysfs_create_group_failed;
    }

    if((err = misc_register(&mmc3524x_device)))
    {
        printk(KERN_ERR "mmc3524x_device register failed\n");
        goto exit_misc_device_register_failed;	
    }    

    sobj_m.self = data;
    sobj_m.polling = 1;
    sobj_m.sensor_operate = mmc3524x_operate;
    if((err = hwmsen_attach(ID_MAGNETIC, &sobj_m)))
    {
        printk(KERN_ERR "attach fail = %d\n", err);
        goto exit_kfree;
    }

    sobj_o.self = data;
    sobj_o.polling = 1;
    sobj_o.sensor_operate = mmc3524x_orientation_operate;
    if((err = hwmsen_attach(ID_ORIENTATION, &sobj_o)))
    {
        printk(KERN_ERR "attach fail = %d\n", err);
        goto exit_kfree;
    }

 #ifdef PSEUDOGYRO
    sobj_gy.self = data;
    sobj_gy.polling = 1;
    sobj_gy.sensor_operate = mmc3524x_gyro_operate;
    if((err = hwmsen_attach(ID_GYROSCOPE, &sobj_gy)))
    {
        printk(KERN_ERR "attach fail = %d\n", err);
        goto exit_kfree;
    }

    gyro_dev = GYRO_MMC_PG;	

    printk("%s: mmc pg !\n", __func__);
#endif /*PSEUDOGYRO*/

#if CONFIG_HAS_EARLYSUSPEND
    data->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
        data->early_drv.suspend  = mmc3524x_early_suspend,
        data->early_drv.resume   = mmc3524x_late_resume,    
        register_early_suspend(&data->early_drv);
#endif

    //hwmsen_make_debug_flag(&debug_fops, "mmc3524x_msensor");
    mmc3524x_init_flag =1;
    printk("%s: OK\n", __func__);

    return 0;

exit_kfree:
    misc_deregister(&mmc3524x_device);
exit_sysfs_create_group_failed:	
exit_misc_device_register_failed:
exit_init_failed:
    this_client = NULL;
exit:
    mmc3524_power(data->hw, 0);
    kfree(data);
    mmc3524x_init_flag =0;
    printk(KERN_ERR "%s: err = %d\n", __func__, err);
    return err;
}


static int mmc3524x_i2c_remove(struct i2c_client *client)
{
	int err;	
	/*
	if(err = mmc3524x_delete_attr(&mmc_sensor_driver.driver))
	{
		printk(KERN_ERR "mmc3524x_delete_attr fail: %d\n", err);
	}
	*/
	if(mmc3524x_delete_attr(&(mmc3524x_init_info.platform_diver_addr->driver)))
	{
		MMCDBG( "mmc3524x_delete_attr fail");
       }
	
	this_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));	
	misc_deregister(&mmc3524x_device);    
	return 0;
}

static const struct i2c_device_id mmc3524x_id[] = 
{
    { MMC3524X_I2C_NAME, 0 },
    { }
};

static struct i2c_driver mmc3524x_i2c_driver = 
{
    .probe 		= mmc3524x_probe,
    .remove 	= mmc3524x_i2c_remove,
    .id_table	= mmc3524x_id,
    .driver 	= {
        .owner	= THIS_MODULE,
        .name	= MMC3524X_I2C_NAME,
    },
};

/*----------------------------------------------------------------------------*/
static int	mmc3524x_local_init(void)
{
	struct mag_hw *hw = get_cust_mag_hw_mmc3524();
	//printk("fwq loccal init+++\n");

	mmc3524_power(hw, 1);
	if(i2c_add_driver(&mmc3524x_i2c_driver))
	{
		MMCDBG("add driver error\n");
		return -1;
	}
	if(-1 == mmc3524x_init_flag)
	{
	   return -1;
	}
	//printk("fwq loccal init---\n");
	return 0;
}


static int mmc3524x_remove(void)
{
	struct mag_hw *hw = get_cust_mag_hw_mmc3524();
 
	mmc3524_power(hw, 0);    
	i2c_del_driver(&mmc3524x_i2c_driver);
	return 0;
}
/*----------------------------------------------------------------------------*/
static int platform_mmc3524x_probe(struct platform_device *pdev) 
{
    struct mag_hw *hw = get_cust_mag_hw_mmc3524();

    mmc3524_power(hw, 1);
    if(i2c_add_driver(&mmc3524x_i2c_driver))
    {
        printk(KERN_ERR "add driver error\n");
        return -1;
    } 
    return 0;
}

/*----------------------------------------------------------------------------*/
static int platform_mmc3524x_remove(struct platform_device *pdev)
{
    struct mag_hw *hw = get_cust_mag_hw_mmc3524();

    mmc3524_power(hw, 0);    
    i2c_del_driver(&mmc3524x_i2c_driver);
    return 0;
}

static int __init mmc3524x_init(void)
{
    struct mag_hw *hw = get_cust_mag_hw_mmc3524();

    i2c_register_board_info(hw->i2c_num, &i2c_mmc3524x, 1);
    #if 1
    mag_driver_add(&mmc3524x_init_info);
    #else
    if(platform_driver_register(&mmc3524x_platform_driver))
    {
        printk(KERN_ERR "failed to register driver");
        return -ENODEV;
    }
    #endif
    return 0;    
}

static void __exit mmc3524x_exit(void)
{
    //platform_driver_unregister(&mmc3524x_platform_driver);
}

module_init(mmc3524x_init);
module_exit(mmc3524x_exit);

MODULE_DESCRIPTION("MEMSIC MMC3524X Magnetic Sensor Driver");
MODULE_LICENSE("GPL");

