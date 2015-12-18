/* drivers/i2c/chips/ist8303.c - IST8303 compass driver
 *
 * Copyright (C) 2009 Technology Inc.
 * Author: Minghung.chou
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

#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>


#include <cust_mag.h>
#include "ist8303.h"
#include <linux/hwmsen_helper.h>

#include <mach/devs.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>
/*--------------------------------------------------------*/

#define IST8303_M_NEW_ARCH   //susport kk new msensor arch

#ifdef IST8303_M_NEW_ARCH
#include "mag.h"
#include <linux/batch.h>

#endif

/*-------------------------MT6516&MT6573 define-------------------------------*/


#define POWER_NONE_MACRO MT65XX_POWER_NONE

/*--------------------------------------------------*/


//#define SOFT_GYRO
//#define REPLACE_ANDROID_VIRTUAL_SENSOR


/*----------------------------------------------------------------------------*/
#define I2C_DRIVERID_IST8303 304
#define DEBUG 1
#define IST8303_DEV_NAME          "ist8303"
#define DRIVER_VERSION            "1.0.0.1"
/*----------------------------------------------------------------------------*/
#define IST8303_AXIS_X            0
#define IST8303_AXIS_Y            1
#define IST8303_AXIS_Z            2
#define IST8303_AXES_NUM          3
/*----------------------------------------------------------------------------*/
// 0: augmented reality, stable but reponse slow
// 1: normal, medium stable and response medium
// 2: game, non-stable but response fast
// default is 0
#define ROTATION_VECTOR_RESPONSE_MODE   0

/*----------------------------------------------------------------------------*/
#define MSE_TAG                  "MSENSOR"
#define MSE_FUN(f)               printk(MSE_TAG" %s\r\n", __FUNCTION__)
#define MSE_ERR(fmt, args...)    printk(KERN_ERR MSE_TAG" %s %d : \r\n"fmt, __FUNCTION__, __LINE__, ##args)
#define MSE_LOG(fmt, args...)    printk(MSE_TAG fmt, ##args)
#define MSE_VER(fmt, args...)   ((void)0)
static DECLARE_WAIT_QUEUE_HEAD(data_ready_wq);
static DECLARE_WAIT_QUEUE_HEAD(open_wq);

static atomic_t open_flag = ATOMIC_INIT(0);
static atomic_t m_flag = ATOMIC_INIT(0);
static atomic_t o_flag = ATOMIC_INIT(0);
/*----------------------------------------------------------------------------*/
static struct i2c_client *ist8303_i2c_client = NULL;
unsigned char ist830x_msensor_raw_data[6];
struct delayed_work ist_get_raw_data_work;
struct mutex sensor_data_mutex;
atomic_t    ist830x_data_ready;
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
static const struct i2c_device_id ist8303_i2c_id[] = {{IST8303_DEV_NAME,0},{}};
static struct i2c_board_info __initdata i2c_ist8303={ I2C_BOARD_INFO("ist8303", 0x0C)};
/*the adapter id will be available in customization*/
//static unsigned short ist8303_force[] = {0x00, IST8303_I2C_ADDRESS, I2C_CLIENT_END, I2C_CLIENT_END};
//static const unsigned short *const ist8303_forces[] = { ist8303_force, NULL };
//static struct i2c_client_address_data ist8303_addr_data = { .forces = ist8303_forces,};

/*----------------------------------------------------------------------------*/
static int ist8303_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id); 
static int ist8303_i2c_remove(struct i2c_client *client);
//static int ist8303_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
static int ist8303_suspend(struct i2c_client *client, pm_message_t msg) ;
static int ist8303_resume(struct i2c_client *client);


/*----------------------------------------------------*/
#ifdef IST8303_M_NEW_ARCH
static int ist8303_local_init(void);
static int ist8303_remove(void);
#endif
/*-----------------------------------------------------*/
#ifndef IST8303_M_NEW_ARCH

static struct platform_driver ist_sensor_driver;
#endif
/*----------------------------------------------------------------------------*/
typedef enum {
    IST_TRC_DEBUG  = 0x01,
	IST_TRC_M_DATA  = 0x02,
	IST_TRC_O_DATA  = 0x04,
	IST_TRC_GYRO_DATA  = 0x08,
	IST_TRC_LINEAR_ACC_DATA  = 0x10,
	IST_TRC_GRAVITY_DATA  = 0x20,
	IST_TRC_ROTATION_VEC_DATA  = 0x40,
} IST_TRC;
/*----------------------------------------------------------------------------*/
struct _ist302_data {
    rwlock_t lock;
    int mode;
    int rate;
    volatile int updated;
} ist830x_data;
/*----------------------------------------------------------------------------*/
struct _ist8303mid_data {
    rwlock_t datalock;
    rwlock_t ctrllock;    
    int controldata[10];
    unsigned int debug;
    int nmx;
    int nmy;
    int nmz;
    int mag_status;
    int yaw;
    int roll;
    int pitch;
    int ori_status;
#ifdef SOFT_GYRO
    int ngx;
    int ngy;
    int ngz;
    int gyro_status;
    int rv[4];
    int rv_status;
#ifdef REPLACE_ANDROID_VIRTUAL_SENSOR
    int ngrx;
    int ngry;
    int ngrz;
    int gra_status;
    int nlax;
    int nlay;
    int nlaz;
    int la_status;
#endif
#endif
} ist8303mid_data;
/*----------------------------------------------------------------------------*/
struct ist8303_i2c_data {
    struct i2c_client *client;
    struct mag_hw *hw;
    struct hwmsen_convert   cvt;
    atomic_t layout;   
    atomic_t trace;
#if defined(CONFIG_HAS_EARLYSUSPEND)    
    struct early_suspend    early_drv;
#endif 
};
/*----------------------------------------------------------------------------*/
static struct i2c_driver ist8303_i2c_driver = {
    .driver = {
//        .owner = THIS_MODULE, 
        .name  = IST8303_DEV_NAME,
    },
    .probe      = ist8303_i2c_probe,
    .remove     = ist8303_i2c_remove,
//  .detect     = ist8303_i2c_detect,
//#if !defined(CONFIG_HAS_EARLYSUSPEND)
    .suspend    = ist8303_suspend,
    .resume     = ist8303_resume,
//#endif 
    .id_table = ist8303_i2c_id,
    //.address_list = ist8303_forces,//address_data->address_list
};
/*-----------------------------------------------*/
#ifdef IST8303_M_NEW_ARCH

static int ist8303_init_flag =-1; // 0<==>OK -1 <==> fail

static struct mag_init_info ist8303_init_info = {
		.name = "ist8303",
		.init = ist8303_local_init,
		.uninit = ist8303_remove,	
};
#endif
/*-------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static atomic_t dev_open_count;
/*----------------------------------------------------------------------------*/
static int ist830x_i2c_rxdata( struct i2c_client *i2c, unsigned char *rxData, int length)
{
    unsigned char addr;
    
    struct i2c_msg msgs[] = {
    {
        .addr = i2c->addr,
        .flags = 0,
        .len = 1,
        .buf = rxData,
    },
    {
        .addr = i2c->addr,
        .flags = I2C_M_RD,
        .len = length,
        .buf = rxData,
    }, };

    addr = rxData[0];

    if (i2c_transfer(i2c->adapter, msgs, 2) < 0) {
        dev_err(&i2c->dev, "%s: transfer failed.", __func__);
        return -EIO;
    }

    dev_vdbg(&i2c->dev, "RxData: len=%02x, addr=%02x, data=%02x", length, addr, rxData[0]);
    return 0;
}
/*----------------------------------------------------------------------------*/
static int ist830x_i2c_txdata(struct i2c_client *i2c, unsigned char *txData, int length)
{
    struct i2c_msg msg[] = {
    {
        .addr = i2c->addr,
        .flags = 0,
        .len = length,
        .buf = txData,
    }, };

    if (i2c_transfer(i2c->adapter, msg, 1) < 0) {
        dev_err(&i2c->dev, "%s: transfer failed.", __func__);
        return -EIO;
    }

    dev_vdbg(&i2c->dev, "TxData: len=%02x, addr=%02x data=%02x", length, txData[0], txData[1]);
    return 0;
}
/*----------------------------------------------------------------------------*/
static void ist8303_power(struct mag_hw *hw, unsigned int on) 
{
    static unsigned int power_on = 0;

    if(hw->power_id != POWER_NONE_MACRO)
    {        
        MSE_LOG("power %s\n", on ? "on" : "off");
        if(power_on == on)
        {
            MSE_LOG("ignore power control: %d\n", on);
        }
        else if(on)
        {
            if(!hwPowerOn(hw->power_id, hw->power_vol, "IST8303")) 
            {
                MSE_ERR("power on fails!!\n");
            }
        }
        else
        {
            if(!hwPowerDown(hw->power_id, "IST8303")) 
            {
                MSE_ERR("power off fail!!\n");
            }
        }
    }
    power_on = on;
}
/*----------------------------------------------------------------------------*/

static int ist8303_GetOpenStatus(void)
{
    wait_event_interruptible(open_wq, (atomic_read(&open_flag) != 0));
    return atomic_read(&open_flag);
}


static int ist8303_gpio_config(void)
{
    //because we donot use EINT ,to support low power
    // config to GPIO input mode + PD    
    //set   GPIO_MSE_EINT_PIN
#if 0
    mt_set_gpio_mode(GPIO_MSE_EINT_PIN, GPIO_MSE_EINT_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_MSE_EINT_PIN, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_MSE_EINT_PIN, GPIO_PULL_ENABLE);
    mt_set_gpio_pull_select(GPIO_MSE_EINT_PIN, GPIO_PULL_DOWN);
#endif
    return 0;
}
/*----------------------------------------------------------------------------*/
static int IST8303_CheckDataReady(struct i2c_client *client)  //  this is used for normal mode, not for force mode
{
    int cResult = 0;
    char buffer[1];
    int  ret;
    //MSE_FUN(f); //debug
    buffer[0] = IST8303_REG_STAT1;

    ret = ist830x_i2c_rxdata(client, buffer, 1);

    if (ret < 0) {
        printk(KERN_ERR "IST8303 IST8303_CheckDataReady : I2C failed \n");
        return ret;
    }
    
    cResult = (buffer[0] & 0x1);  //  if TRUE then read data

    return cResult;
}
/*----------------------------------------------------------------------------*/
static int IST8303_GetData(struct i2c_client *client, char *rbuf)
{
    int i;

    char buffer[6];
    int ret;
    memset(buffer, 0, 6);
    buffer[0] = IST8303_REG_DATAX_L;
    //MSE_FUN(f); //debug
    ret = ist830x_i2c_rxdata(client, buffer, 6);

    if (ret < 0) {
        printk(KERN_ERR "IST8303_GetXYZ : I2C failed \n");
        return ret;
    }else {
        for(i=0; i<6; i++)
            rbuf[i] = buffer[i];
    }
    
    return ret;
}
/*----------------------------------------------------------------------------*/
static void ist830x_prepare_raw_data(struct work_struct *work)
{
    char sData[6];  //  sensor raw data
    int err;    
    int status;    
    unsigned long start;    
	//MSE_FUN(f); //debug

    memset(sData, 0, sizeof(sData));

    /* Check DRDY bit */
    start = jiffies;
    do {
        status = IST8303_CheckDataReady(ist8303_i2c_client);
        if (status == 1)
            break;
//    } while (jiffies_to_msecs(jiffies - start) <= 25);
    } while (jiffies_to_msecs(jiffies - start) <= 50);
#if 0 //vender changed        
    if (!status && jiffies_to_msecs(jiffies - start) > 25)
    {
        printk("DRDY loop time out\n");
        goto data_not_ready;}
#endif
    if (status < 0) // -5 case
    {
        printk("ist830x_i2c_rxdata() fail \n");
        goto data_not_ready;
    }
    
    if (status == 0) // DRDY = 0 case
    {
        printk("DRDY is 0 \n");
        goto data_not_ready;
    }
 

    err = IST8303_GetData(ist8303_i2c_client, sData);   
    if (err < 0) {
        dev_err(&ist8303_i2c_client->dev, "%s failed.", __func__);
        goto data_not_ready;
    }

    mutex_lock(&sensor_data_mutex);
    memcpy(ist830x_msensor_raw_data, sData, 6);
    mutex_unlock(&sensor_data_mutex);

    atomic_set(&ist830x_data_ready, 1);
    wake_up(&data_ready_wq);

#if 0 //for debug
	int i = 0;
	for(i=0; i<sizeof(ist830x_msensor_raw_data)/sizeof(ist830x_msensor_raw_data[0]); i++)
	{
		MSE_LOG("ist830x_msensor_raw_data[%d] = 0x%x\n",i,ist830x_msensor_raw_data[i]);//debug
	}
#endif

data_not_ready:
    schedule_delayed_work(&ist_get_raw_data_work, msecs_to_jiffies(50));
}
/*----------------------------------------------------------------------------*/
static int IST8303_Chipset_Init(int mode)
{
    char wbuffer[2];
    int ret;

    wbuffer[0] = IST8303_REG_CNTRL1;
    wbuffer[1] = 0x00;  //  stand-by mode  
    ret = ist830x_i2c_txdata(ist8303_i2c_client, wbuffer, 2);
    if(ret<0) {
        printk(KERN_ERR "set register IST8303_REG_CNTRL1 failed.\n");
        return ret;
    }

    wbuffer[0] = IST8303_REG_SSR;
    wbuffer[1] = 0xC0;  //  set to 1us pulse duration + low power mode  
    ret = ist830x_i2c_txdata(ist8303_i2c_client, wbuffer, 2);
    if(ret<0) {
        printk(KERN_ERR "set register IST8303_REG_SSR failed.\n");
        return ret;
    }

    wbuffer[0] = IST8303_REG_CTR;
    wbuffer[1] = 0x00;  //  use temperature compensation mechanism
    ret = ist830x_i2c_txdata(ist8303_i2c_client, wbuffer, 2);
    if(ret<0) {
        printk(KERN_ERR "set register IST8303_REG_CTR failed.\n");
        return ret;
    }

    wbuffer[0] = 0x62;
    wbuffer[1] = 0x00;  //  only for 8303b pulse width
    ret = ist830x_i2c_txdata(ist8303_i2c_client, wbuffer, 2);
    if(ret<0) {
        printk(KERN_ERR "set register IST8303_REG_BTR failed.\n");
        return ret;
    }

    wbuffer[0] = IST8303_REG_CNTRL1;
    wbuffer[1] = 0x05;  //  continuous ODR 20Hz  
    ret = ist830x_i2c_txdata(ist8303_i2c_client, wbuffer, 2);
    if(ret<0) {
        printk(KERN_ERR "set register IST8303_REG_CNTRL1 failed.\n");
        return ret;
    }

    return 0;
}
/*----------------------------------------------------------------------------*/
static int IST8303_SetMode(int newmode)
{
    int mode = 0;

    read_lock(&ist830x_data.lock);
    mode = ist830x_data.mode;
    read_unlock(&ist830x_data.lock);        

    if(mode == newmode)
    {
        return 0;    
    }
    
    return IST8303_Chipset_Init(newmode);
}
/*----------------------------------------------------------------------------*/
static int IST8303_ReadChipInfo(char *buf, int bufsize)
{
    if((!buf)||(bufsize<=30))
    {
        return -1;
    }
    if(!ist8303_i2c_client)
    {
        *buf = 0;
        return -2;
    }

    sprintf(buf, "IST8303 Chip");
    return 0;
}
/*----------------------------------------------------------------------------*/
static int IST8303_ReadSensorData(char *buf, int bufsize)
{
    struct ist8303_i2c_data *data = i2c_get_clientdata(ist8303_i2c_client);
    char cmd;
    int mode = 0, err = 0;    
    unsigned char databuf[6];  //  for sensor raw data
    short B[IST8303_AXES_NUM];
    short output[IST8303_AXES_NUM];
    short temp = 0;
    int mag[IST8303_AXES_NUM], i;

    if((!buf)||(bufsize<=80))
    {
        return -1;
    }   
    if(NULL == ist8303_i2c_client)
    {
        *buf = 0;
        return -2;
    }

    read_lock(&ist830x_data.lock);    
    mode = ist830x_data.mode;
    read_unlock(&ist830x_data.lock);        

    if(mode == IST8303_FORCE_MODE)
    {
        //databuf[0] = IST8303_REG_CTRL3;
        //databuf[1] = IST8303_CTRL3_FORCE_BIT;
        i2c_master_send(ist8303_i2c_client, databuf, 2);    
        // We can read all measured data in once
        //cmd = IST8303_REG_DATAXH;
        i2c_master_send(ist8303_i2c_client, &cmd, 1);    
        i2c_master_recv(ist8303_i2c_client, &(databuf[0]), 6);
    }
    else  //  IST8303_NORMAL_MODE for ist830x
    {
       // int err;

        err = wait_event_interruptible_timeout(data_ready_wq, atomic_read(&ist830x_data_ready), msecs_to_jiffies(50));
#if 0 //vender changed
        if (err < 0) {
            dev_err(&ist8303_i2c_client->dev, "%s: wait_event failed (%d).", __func__, err);
            return -1;
        }
        
        if (!atomic_read(&ist830x_data_ready)) {
            dev_err(&ist8303_i2c_client->dev, "%s: DRDY is not set.", __func__);        
            return -1;
        }
#endif
        if (err == 0) {
            printk("wait 100ms timeout \n");
            return err;
        }

        if (err < 0) {
            printk("interrupted by other signal \n");
            return err;
        }

        mutex_lock(&sensor_data_mutex);
        memcpy(databuf, ist830x_msensor_raw_data, 6);  //  from : ist830x_prepare_raw_data()
        mutex_unlock(&sensor_data_mutex);

        atomic_set(&ist830x_data_ready, 0);
    }

    output[0] = ((int) databuf[1]) << 8 | ((int) databuf[0]);
    output[1] = ((int) databuf[3]) << 8 | ((int) databuf[2]);
    output[2] = ((int) databuf[5]) << 8 | ((int) databuf[4]);

    // swap x/y
    temp = output[0];
    output[0] = output[1];
    output[1] = temp;

    #if 0
    cmd = 0xDE;  //  only ist8303 needs temperature compensation
    i2c_master_send(ist8303_i2c_client, &cmd, 1);    
    i2c_master_recv(ist8303_i2c_client, &(databuf[0]), 6);
    B[0] = ((int) databuf[1]) << 8 | ((int)databuf[0]);
    B[1] = ((int) databuf[3]) << 8 | ((int)databuf[2]);
    B[2] = ((int) databuf[5]) << 8 | ((int)databuf[4]);

    for (i=0;i<IST8303_AXES_NUM;i++)
    {
        output[i] = output[i] >> 1;
        B[i] = B[i] >> 1;
        output[i] -= B[i];
    }
    #endif

    mag[data->cvt.map[IST8303_AXIS_X]] = data->cvt.sign[IST8303_AXIS_X]*output[IST8303_AXIS_X];
    mag[data->cvt.map[IST8303_AXIS_Y]] = data->cvt.sign[IST8303_AXIS_Y]*output[IST8303_AXIS_Y];
    mag[data->cvt.map[IST8303_AXIS_Z]] = data->cvt.sign[IST8303_AXIS_Z]*output[IST8303_AXIS_Z];

    sprintf(buf, "%d %d %d", mag[IST8303_AXIS_X], mag[IST8303_AXIS_Y], mag[IST8303_AXIS_Z]);

	if(atomic_read(&data->trace) & IST_TRC_DEBUG)
	{
		MSE_LOG("msensor raw data:%d %d %d\n", mag[IST8303_AXIS_X], mag[IST8303_AXIS_Y], mag[IST8303_AXIS_Z]);//debug
	}
    return err;
}
/*----------------------------------------------------------------------------*/
static int IST8303_ReadPostureData(char *buf, int bufsize)
{
    if((!buf)||(bufsize<=80))
    {
        return -1;
    }
    
    read_lock(&ist8303mid_data.datalock);
    sprintf(buf, "%d %d %d %d", ist8303mid_data.yaw, ist8303mid_data.pitch,
        ist8303mid_data.roll, ist8303mid_data.mag_status);
    read_unlock(&ist8303mid_data.datalock);
    return 0;
}
/*----------------------------------------------------------------------------*/
static int IST8303_ReadCaliData(char *buf, int bufsize)
{
    if((!buf)||(bufsize<=80))
    {
        return -1;
    }
    
    read_lock(&ist8303mid_data.datalock);
    sprintf(buf, "%d %d %d %d", ist8303mid_data.nmx, ist8303mid_data.nmy, 
        ist8303mid_data.nmz,ist8303mid_data.mag_status);
    read_unlock(&ist8303mid_data.datalock);
    return 0;
}
/*----------------------------------------------------------------------------*/
static int IST8303_ReadMiddleControl(char *buf, int bufsize)
{
    if ((!buf)||(bufsize<=80))
    {
        return -1;
    }
    
    read_lock(&ist8303mid_data.ctrllock);
    sprintf(buf, "%d %d %d %d %d %d %d %d %d %d",ist8303mid_data.controldata[0],    ist8303mid_data.controldata[1], 
        ist8303mid_data.controldata[2],ist8303mid_data.controldata[3],ist8303mid_data.controldata[4],
        ist8303mid_data.controldata[5], ist8303mid_data.controldata[6], ist8303mid_data.controldata[7],
        ist8303mid_data.controldata[8], ist8303mid_data.controldata[9]);
    read_unlock(&ist8303mid_data.ctrllock);
    return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_daemon_name(struct device_driver *ddri, char *buf)
{
    char strbuf[IST8303_BUFSIZE];
    sprintf(strbuf, "istd8303");
    return sprintf(buf, "%s", strbuf);      
}

static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
    char strbuf[IST8303_BUFSIZE];
    IST8303_ReadChipInfo(strbuf, IST8303_BUFSIZE);
    return sprintf(buf, "%s\n", strbuf);        
}
/*----------------------------------------------------------------------------*/
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
    char strbuf[IST8303_BUFSIZE];
    IST8303_ReadSensorData(strbuf, IST8303_BUFSIZE);
    return sprintf(buf, "%s\n", strbuf);
}
/*----------------------------------------------------------------------------*/
static ssize_t show_posturedata_value(struct device_driver *ddri, char *buf)
{
    char strbuf[IST8303_BUFSIZE];
    IST8303_ReadPostureData(strbuf, IST8303_BUFSIZE);
    return sprintf(buf, "%s\n", strbuf);            
}
/*----------------------------------------------------------------------------*/
static ssize_t show_calidata_value(struct device_driver *ddri, char *buf)
{
    char strbuf[IST8303_BUFSIZE];
    IST8303_ReadCaliData(strbuf, IST8303_BUFSIZE);
    return sprintf(buf, "%s\n", strbuf);            
}
/*----------------------------------------------------------------------------*/
static ssize_t show_midcontrol_value(struct device_driver *ddri, char *buf)
{
    char strbuf[IST8303_BUFSIZE];
    IST8303_ReadMiddleControl(strbuf, IST8303_BUFSIZE);
    return sprintf(buf, "%s\n", strbuf);            
}
/*----------------------------------------------------------------------------*/
static ssize_t store_midcontrol_value(struct device_driver *ddri, const char *buf, size_t count)
{   
    int p[10];
    if(10 == sscanf(buf, "%d %d %d %d %d %d %d %d %d %d",&p[0], &p[1], &p[2], &p[3], &p[4], 
        &p[5], &p[6], &p[7], &p[8], &p[9]))
    {
        write_lock(&ist8303mid_data.ctrllock);
        memcpy(&ist8303mid_data.controldata[0], &p, sizeof(int)*10);    
        write_unlock(&ist8303mid_data.ctrllock);        
    }
    else
    {
        MSE_ERR("invalid format\n");     
    }
    return sizeof(int)*10;            
}
/*----------------------------------------------------------------------------*/
static ssize_t show_middebug_value(struct device_driver *ddri, char *buf)
{
    ssize_t len;
    read_lock(&ist8303mid_data.ctrllock);
    len = sprintf(buf, "0x%08X\n", ist8303mid_data.debug);
    read_unlock(&ist8303mid_data.ctrllock);

    return len;            
}
/*----------------------------------------------------------------------------*/
static ssize_t store_middebug_value(struct device_driver *ddri, const char *buf, size_t count)
{   
    int debug;
    if(1 == sscanf(buf, "0x%x", &debug))
    {
        write_lock(&ist8303mid_data.ctrllock);
        ist8303mid_data.debug = debug;
        write_unlock(&ist8303mid_data.ctrllock);        
    }
    else
    {
        MSE_ERR("invalid format\n");     
    }
    return count;            
}
/*----------------------------------------------------------------------------*/
static ssize_t show_mode_value(struct device_driver *ddri, char *buf)
{
    int mode=0;
    read_lock(&ist830x_data.lock);
    mode = ist830x_data.mode;
    read_unlock(&ist830x_data.lock);        
    return sprintf(buf, "%d\n", mode);            
}
/*----------------------------------------------------------------------------*/
static ssize_t store_mode_value(struct device_driver *ddri, const char *buf, size_t count)
{
    int mode = 0;
    sscanf(buf, "%d", &mode);    
    IST8303_SetMode(mode);
    return count;            
}
/*----------------------------------------------------------------------------*/
static ssize_t show_layout_value(struct device_driver *ddri, char *buf)
{
    struct i2c_client *client = ist8303_i2c_client;  
    struct ist8303_i2c_data *data = i2c_get_clientdata(client);

    return sprintf(buf, "(%d, %d)\n[%+2d %+2d %+2d]\n[%+2d %+2d %+2d]\n",
        data->hw->direction,atomic_read(&data->layout), data->cvt.sign[0], data->cvt.sign[1],
        data->cvt.sign[2],data->cvt.map[0], data->cvt.map[1], data->cvt.map[2]);            
}
/*----------------------------------------------------------------------------*/
static ssize_t store_layout_value(struct device_driver *ddri, const char *buf, size_t count)
{
    struct i2c_client *client = ist8303_i2c_client;  
    struct ist8303_i2c_data *data = i2c_get_clientdata(client);
    int layout = 0;

    if(1 == sscanf(buf, "%d", &layout))
    {
        atomic_set(&data->layout, layout);
        if(!hwmsen_get_convert(layout, &data->cvt))
        {
            MSE_ERR("HWMSEN_GET_CONVERT function error!\r\n");
        }
        else if(!hwmsen_get_convert(data->hw->direction, &data->cvt))
        {
            MSE_ERR("invalid layout: %d, restore to %d\n", layout, data->hw->direction);
        }
        else
        {
            MSE_ERR("invalid layout: (%d, %d)\n", layout, data->hw->direction);
            hwmsen_get_convert(0, &data->cvt);
        }
    }
    else
    {
        MSE_ERR("invalid format = '%s'\n", buf);
    }
    
    return count;            
}
/*----------------------------------------------------------------------------*/
static ssize_t show_status_value(struct device_driver *ddri, char *buf)
{
    struct i2c_client *client = ist8303_i2c_client;  
    struct ist8303_i2c_data *data = i2c_get_clientdata(client);
    ssize_t len = 0;

    if(data->hw)
    {
        len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d %d (%d %d)\n", 
            data->hw->i2c_num, data->hw->direction, data->hw->power_id, data->hw->power_vol);
    }
    else
    {
        len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");
    }
    
    //len += snprintf(buf+len, PAGE_SIZE-len, "OPEN: %d\n", atomic_read(&dev_open_count));
	len += snprintf(buf+len, PAGE_SIZE-len, "M-OPEN: %d, O-OPEN: %d\n", atomic_read(&m_flag), atomic_read(&o_flag));//changed by wxj 2014.5.16
	return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_trace_value(struct device_driver *ddri, char *buf)
{
    ssize_t res;
    struct ist8303_i2c_data *obj = i2c_get_clientdata(ist8303_i2c_client);
    if(NULL == obj)
    {
        MSE_ERR("ist8303_i2c_data is null!!\n");
        return 0;
    }   
    
    res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));     
    return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
{
    struct ist8303_i2c_data *obj = i2c_get_clientdata(ist8303_i2c_client);
    int trace;
    if(NULL == obj)
    {
        MSE_ERR("ist8303_i2c_data is null!!\n");
        return 0;
    }
    
    if(1 == sscanf(buf, "0x%x", &trace))
    {
        atomic_set(&obj->trace, trace);
    }
    else 
    {
        MSE_ERR("invalid content: '%s', length = %d\n", buf, count);
    }
    
    return count;    
}
/*-------------------------------------------------------------*/
static ssize_t show_version_value(struct device_driver *ddri, char *buf)
{
    return sprintf(buf, "%s\n", DRIVER_VERSION); 
}


/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(daemon,      S_IRUGO, show_daemon_name, NULL);
static DRIVER_ATTR(chipinfo,    S_IRUGO, show_chipinfo_value, NULL);
static DRIVER_ATTR(sensordata,  S_IRUGO, show_sensordata_value, NULL);
static DRIVER_ATTR(posturedata, S_IRUGO, show_posturedata_value, NULL);
static DRIVER_ATTR(calidata,    S_IRUGO, show_calidata_value, NULL);
static DRIVER_ATTR(midcontrol,  S_IRUGO | S_IWUSR, show_midcontrol_value, store_midcontrol_value );
static DRIVER_ATTR(middebug,    S_IRUGO | S_IWUSR, show_middebug_value, store_middebug_value );
static DRIVER_ATTR(mode,        S_IRUGO | S_IWUSR, show_mode_value, store_mode_value );
static DRIVER_ATTR(layout,      S_IRUGO | S_IWUSR, show_layout_value, store_layout_value );
static DRIVER_ATTR(status,      S_IRUGO, show_status_value, NULL);
static DRIVER_ATTR(trace,       S_IRUGO | S_IWUSR, show_trace_value, store_trace_value );
static DRIVER_ATTR(version,       S_IRUGO | S_IWUSR, show_version_value, NULL);

/*----------------------------------------------------------------------------*/
static struct driver_attribute *ist8303_attr_list[] = {
    &driver_attr_daemon,
    &driver_attr_chipinfo,
    &driver_attr_sensordata,
    &driver_attr_posturedata,
   // &driver_attr_calidata,
    &driver_attr_midcontrol,
    //&driver_attr_middebug,
    &driver_attr_mode,
    &driver_attr_layout,
    &driver_attr_status,
    &driver_attr_trace,
    &driver_attr_version,
};
/*----------------------------------------------------------------------------*/
static int ist8303_create_attr(struct device_driver *driver) 
{
    int idx, err = 0;
    int num = (int)(sizeof(ist8303_attr_list)/sizeof(ist8303_attr_list[0]));
    if (driver == NULL)
    {
        return -EINVAL;
    }

    for(idx = 0; idx < num; idx++)
    {
        if((err = driver_create_file(driver, ist8303_attr_list[idx])))
        {            
            MSE_ERR("driver_create_file (%s) = %d\n", ist8303_attr_list[idx]->attr.name, err);
            break;
        }
    }    
    return err;
}
/*----------------------------------------------------------------------------*/
static int ist8303_delete_attr(struct device_driver *driver)
{
    int idx ,err = 0;
    int num = (int)(sizeof(ist8303_attr_list)/sizeof(ist8303_attr_list[0]));

    if(driver == NULL)
    {
        return -EINVAL;
    }
    

    for(idx = 0; idx < num; idx++)
    {
        driver_remove_file(driver, ist8303_attr_list[idx]);
    }
    

    return err;
}


/*----------------------------------------------------------------------------*/
static int ist8303_open(struct inode *inode, struct file *file)
{    
    struct ist8303_i2c_data *obj = i2c_get_clientdata(ist8303_i2c_client);    
    int ret = -1;
    atomic_inc(&dev_open_count);
    
    if(atomic_read(&obj->trace) & IST_TRC_DEBUG)
    {
        MSE_LOG("Open device node:ist8303\n");
    }
    ret = nonseekable_open(inode, file);
    
    return ret;
}
/*----------------------------------------------------------------------------*/
static int ist8303_release(struct inode *inode, struct file *file)
{
    struct ist8303_i2c_data *obj = i2c_get_clientdata(ist8303_i2c_client);
    atomic_dec(&dev_open_count);
    if(atomic_read(&obj->trace) & IST_TRC_DEBUG)
    {
        MSE_LOG("Release device node:ist8303\n");
    }   
    return 0;
}
/*----------------------------------------------------------------------------*/
//static int ist8303_ioctl(struct inode *inode, struct file *file, unsigned int cmd,unsigned long arg)//modified here
static long ist8303_unlocked_ioctl(struct file *file, unsigned int cmd,
       unsigned long arg)
{
    void __user *argp = (void __user *)arg;
    int valuebuf[4];
#ifdef SOFT_GYRO
    int calidata[30];
#else
    int calidata[30];
#endif
    int controlbuf[10];
    char strbuf[IST8303_BUFSIZE];
    void __user *data;
    long retval=0;
    int mode=0;
    hwm_sensor_data* osensor_data;
    uint32_t enable;
    char buff[512]; 
    int status;                 /* for OPEN/CLOSE_STATUS */
    short sensor_status;        /* for Orientation and Msensor status */
//  MSE_FUN(f);

    switch (cmd)
    {
        case MSENSOR_IOCTL_INIT:
            read_lock(&ist830x_data.lock);
            mode = ist830x_data.mode;
            read_unlock(&ist830x_data.lock);
            IST8303_Chipset_Init(mode);         
            break;

        case MSENSOR_IOCTL_SET_POSTURE:
            data = (void __user *) arg;
            if(data == NULL)
            {
                MSE_ERR("IO parameter pointer is NULL!\r\n");
                break;
            }
               
            if(copy_from_user(valuebuf, data, sizeof(valuebuf)))
            {
                retval = -EFAULT;
                goto err_out;
            }
            
            write_lock(&ist8303mid_data.datalock);
            ist8303mid_data.yaw   = valuebuf[0];
            ist8303mid_data.pitch = valuebuf[1];
            ist8303mid_data.roll  = valuebuf[2];
            ist8303mid_data.mag_status = valuebuf[3];
            write_unlock(&ist8303mid_data.datalock);    
            break;

        case ECOMPASS_IOC_GET_OFLAG:
            sensor_status = atomic_read(&o_flag);
            if(copy_to_user(argp, &sensor_status, sizeof(sensor_status)))
            {
                MSE_ERR("copy_to_user failed.");
                return -EFAULT;
            }
            break;

        case ECOMPASS_IOC_GET_MFLAG:
            sensor_status = atomic_read(&m_flag);
            if(copy_to_user(argp, &sensor_status, sizeof(sensor_status)))
            {
                MSE_ERR("copy_to_user failed.");
                return -EFAULT;
            }
            break;
            
        case ECOMPASS_IOC_GET_OPEN_STATUS:
            status = ist8303_GetOpenStatus();           
            if(copy_to_user(argp, &status, sizeof(status)))
            {
                MSE_LOG("copy_to_user failed.");
                return -EFAULT;
            }
            break;        

        case MSENSOR_IOCTL_SET_CALIDATA:
            data = (void __user *) arg;
            if (data == NULL)
            {
                MSE_ERR("IO parameter pointer is NULL!\r\n");
                break;
            }
            if(copy_from_user(calidata, data, sizeof(calidata)))
            {
                retval = -EFAULT;
                goto err_out;
            }
            
            write_lock(&ist8303mid_data.datalock);            
            ist8303mid_data.nmx = calidata[1];
            ist8303mid_data.nmy = calidata[2];
            ist8303mid_data.nmz = calidata[3];
            ist8303mid_data.mag_status = calidata[4];
            ist8303mid_data.yaw   = calidata[5];
            ist8303mid_data.pitch = calidata[6];
            ist8303mid_data.roll  = calidata[7];
            ist8303mid_data.ori_status = calidata[8];
        #ifdef SOFT_GYRO
            ist8303mid_data.ngx = calidata[9];
            ist8303mid_data.ngy = calidata[10];
            ist8303mid_data.ngz = calidata[11];
            ist8303mid_data.gyro_status = calidata[12];
            ist8303mid_data.rv[0] = calidata[13];
            ist8303mid_data.rv[1] = calidata[14];
            ist8303mid_data.rv[2] = calidata[15];
            ist8303mid_data.rv[3] = calidata[16];
            ist8303mid_data.rv_status = calidata[17];
#ifdef REPLACE_ANDROID_VIRTUAL_SENSOR
            ist8303mid_data.ngrx = calidata[18];
            ist8303mid_data.ngry = calidata[19];
            ist8303mid_data.ngrz = calidata[20];
            ist8303mid_data.gra_status = calidata[21];
            ist8303mid_data.nlax = calidata[22];
            ist8303mid_data.nlay = calidata[23];
            ist8303mid_data.nlaz = calidata[24];
            ist8303mid_data.la_status = calidata[25];
#endif
        #endif

            write_unlock(&ist8303mid_data.datalock);
            //printk("[qnmd] calidata[7] = %d,calidata[8] = %d,calidata[9] = %d\n",calidata[7],calidata[8],calidata[9]);    
            break;                                

        case MSENSOR_IOCTL_READ_CHIPINFO:
            data = (void __user *) arg;
            if(data == NULL)
            {
                MSE_ERR("IO parameter pointer is NULL!\r\n");
                break;
            }
            
            IST8303_ReadChipInfo(strbuf, IST8303_BUFSIZE);
            if(copy_to_user(data, strbuf, strlen(strbuf)+1))
            {
                retval = -EFAULT;
                goto err_out;
            }                
            break;

        case MSENSOR_IOCTL_SENSOR_ENABLE:
            
            data = (void __user *) arg;
            if (data == NULL)
            {
                MSE_ERR("IO parameter pointer is NULL!\r\n");
                break;
            }
            if(copy_from_user(&enable, data, sizeof(enable)))
            {
                MSE_ERR("copy_from_user failed.");
                return -EFAULT;
            }
            else
            {
                printk( "MSENSOR_IOCTL_SENSOR_ENABLE enable=%d!\r\n",enable);
                read_lock(&ist8303mid_data.ctrllock);
                if(enable == 1)
                {
                    ist8303mid_data.controldata[7] |= SENSOR_ORIENTATION;
                    atomic_set(&o_flag, 1);
                    atomic_set(&open_flag, 1);
                }
                else
                {
                    ist8303mid_data.controldata[7] &= ~SENSOR_ORIENTATION;
                    atomic_set(&o_flag, 0);
                    if(atomic_read(&m_flag) == 0)
                    {
                        atomic_set(&open_flag, 0);
                    }
                }
                wake_up(&open_wq);
                read_unlock(&ist8303mid_data.ctrllock);
                
            }
            
            break;
            
        case MSENSOR_IOCTL_READ_SENSORDATA:
            data = (void __user *) arg;
            if(data == NULL)
            {
                MSE_ERR("IO parameter pointer is NULL!\r\n");
                break;    
            }
            //IST8303_ReadSensorData(strbuf, IST8303_BUFSIZE);
            
            status = IST8303_ReadSensorData(strbuf, IST8303_BUFSIZE);
            if (status < 0)
            {
                status = IST8303_ReadSensorData(strbuf, IST8303_BUFSIZE);
            }

            if(status == 0) // wait 100ms timeout
            {
                retval = -ETIME;
                goto err_out;
            }
			
            if(copy_to_user(data, strbuf, strlen(strbuf)+1))
            {
                retval = -EFAULT;
                goto err_out;
            }                
            break;

        case MSENSOR_IOCTL_READ_FACTORY_SENSORDATA:
            
            data = (void __user *) arg;
            if (data == NULL)
            {
                MSE_ERR("IO parameter pointer is NULL!\r\n");
                break;
            }
            
            osensor_data = (hwm_sensor_data *)buff;

            read_lock(&ist8303mid_data.datalock);
            osensor_data->values[0] = ist8303mid_data.yaw;
            osensor_data->values[1] = ist8303mid_data.pitch;
            osensor_data->values[2] = ist8303mid_data.roll;
            //status = ist8303mid_data.mag_status;
            read_unlock(&ist8303mid_data.datalock); 
                        
            osensor_data->value_divide = ORIENTATION_ACCURACY_RATE; 

            switch (ist8303mid_data.mag_status)
            {
                    case 1: case 2:
                        osensor_data->status = SENSOR_STATUS_ACCURACY_HIGH;
                        break;
                    case 3:
                        osensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
                        break;
                    case 4:
                        osensor_data->status = SENSOR_STATUS_ACCURACY_LOW;
                        break;
                    default:        
                        osensor_data->status = SENSOR_STATUS_UNRELIABLE;
                        break;    
            }
     
            
            sprintf(buff, "%x %x %x %x %x", osensor_data->values[0], osensor_data->values[1],
                osensor_data->values[2],osensor_data->status,osensor_data->value_divide);
            if(copy_to_user(data, buff, strlen(buff)+1))
            {
                return -EFAULT;
            } 
            
            break;                

        case MSENSOR_IOCTL_READ_POSTUREDATA:
            data = (void __user *) arg;
            if(data == NULL)
            {
                MSE_ERR("IO parameter pointer is NULL!\r\n");
                break;
            }
            
            IST8303_ReadPostureData(strbuf, IST8303_BUFSIZE);
            if(copy_to_user(data, strbuf, strlen(strbuf)+1))
            {
                retval = -EFAULT;
                goto err_out;
            }                
            break;            

        case MSENSOR_IOCTL_READ_CALIDATA:
            data = (void __user *) arg;
            if(data == NULL)
            {
                break;    
            }
            IST8303_ReadCaliData(strbuf, IST8303_BUFSIZE);
            if(copy_to_user(data, strbuf, strlen(strbuf)+1))
            {
                retval = -EFAULT;
                goto err_out;
            }                
            break;

        case MSENSOR_IOCTL_READ_CONTROL:
            read_lock(&ist8303mid_data.ctrllock);
            memcpy(controlbuf, &ist8303mid_data.controldata[0], sizeof(controlbuf));
            read_unlock(&ist8303mid_data.ctrllock);            
            data = (void __user *) arg;
            if(data == NULL)
            {
                break;
            }
            if(copy_to_user(data, controlbuf, sizeof(controlbuf)))
            {
                retval = -EFAULT;
                goto err_out;
            }                                
            break;

        case MSENSOR_IOCTL_SET_CONTROL:
            data = (void __user *) arg;
            if(data == NULL)
            {
                break;
            }
            if(copy_from_user(controlbuf, data, sizeof(controlbuf)))
            {
                retval = -EFAULT;
                goto err_out;
            }    
            write_lock(&ist8303mid_data.ctrllock);
            memcpy(&ist8303mid_data.controldata[0], controlbuf, sizeof(controlbuf));
            write_unlock(&ist8303mid_data.ctrllock);        
            break;

        case MSENSOR_IOCTL_SET_MODE:
            data = (void __user *) arg;
            if(data == NULL)
            {
                break;
            }
            if(copy_from_user(&mode, data, sizeof(mode)))
            {
                retval = -EFAULT;
                goto err_out;
            }
            
            IST8303_SetMode(mode);                
            break;
            
        default:
            MSE_ERR("%s not supported = 0x%04x", __FUNCTION__, cmd);
            retval = -ENOIOCTLCMD;
            break;
        }

    err_out:
    return retval;    
}
/*----------------------------------------------------------------------------*/
static struct file_operations ist8303_fops = {
//  .owner = THIS_MODULE,
    .open = ist8303_open,
    .release = ist8303_release,
    .unlocked_ioctl = ist8303_unlocked_ioctl,//modified
};
/*----------------------------------------------------------------------------*/
static struct miscdevice ist8303_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "msensor",
    .fops = &ist8303_fops,
};
/*----------------------------------------------------------------------------*/

int ist8303_operate(void* self, uint32_t command, void* buff_in, int size_in,
        void* buff_out, int size_out, int* actualout)
{
    int err = 0;
    int value, sample_delay, status;
    hwm_sensor_data* msensor_data;
	struct ist8303_i2c_data *obj = (struct ist8303_i2c_data *)self;

	if(atomic_read(&obj->trace) & IST_TRC_DEBUG)
	{
		MSE_FUN(f);
	}
    
    switch (command)
    {
        case SENSOR_DELAY:
            if((buff_in == NULL) || (size_in < sizeof(int)))
            {
                MSE_ERR("Set delay parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                value = *(int *)buff_in;
                if(value <= 20)
                {
                    value = 20;
                }
                ist8303mid_data.controldata[0] = value;  // Loop Delay
            }   
            break;

        case SENSOR_ENABLE:
            if((buff_in == NULL) || (size_in < sizeof(int)))
            {
                MSE_ERR("Enable sensor parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                value = *(int *)buff_in;
                read_lock(&ist8303mid_data.ctrllock);
                if(value == 1)
                {
                    if (ist8303mid_data.controldata[7] == 0)
                    {
                        schedule_delayed_work(&ist_get_raw_data_work, msecs_to_jiffies(50));
                    }
                    ist8303mid_data.controldata[7] |= SENSOR_MAGNETIC;
                    atomic_set(&m_flag, 1);
                    atomic_set(&open_flag, 1);
                }
                else
                {
                    ist8303mid_data.controldata[7] &= ~SENSOR_MAGNETIC;
                    atomic_set(&m_flag, 0);
                    if(atomic_read(&o_flag) == 0)
                    {
                        atomic_set(&open_flag, 0);
                    }
                    if (ist8303mid_data.controldata[7] == 0)
                    {
                        cancel_delayed_work_sync(&ist_get_raw_data_work);
                    }
                }
                wake_up(&open_wq);
                read_unlock(&ist8303mid_data.ctrllock);

				MSE_LOG("msensor enable/disable ok!status = %d\n",atomic_read(&m_flag));//debug
                // TODO: turn device into standby or normal mode
            }
            break;

        case SENSOR_GET_DATA:
            //MSE_LOG("++++++++++++++++++++++++MSENSOR_GET_DATA");
            if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
            {
                MSE_ERR("get sensor data parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                msensor_data = (hwm_sensor_data *)buff_out;
                read_lock(&ist8303mid_data.datalock);
                msensor_data->values[0] = ist8303mid_data.nmx;
                msensor_data->values[1] = ist8303mid_data.nmy;
                msensor_data->values[2] = ist8303mid_data.nmz;
                status = ist8303mid_data.mag_status;
                read_unlock(&ist8303mid_data.datalock); 
                
                msensor_data->values[0] = msensor_data->values[0] * CONVERT_M;
                msensor_data->values[1] = msensor_data->values[1] * CONVERT_M;
                msensor_data->values[2] = msensor_data->values[2] * CONVERT_M;
                msensor_data->value_divide = 100;

                switch (status)
                {
                    case 1: case 2:
                        msensor_data->status = SENSOR_STATUS_ACCURACY_HIGH;
                        break;
                    case 3:
                        msensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
                        break;
                    case 4:
                        msensor_data->status = SENSOR_STATUS_ACCURACY_LOW;
                        break;
                    default:        
                        msensor_data->status = SENSOR_STATUS_UNRELIABLE;
                        break;    
                }
				if(atomic_read(&obj->trace) & IST_TRC_M_DATA)
				{
                	MSE_LOG("msensor data: %d, %d, %d, %d\n",msensor_data->values[0],msensor_data->values[1],msensor_data->values[2],msensor_data->status);//debug
				}
			}
            break;
        default:
            MSE_ERR("msensor operate function no this parameter %d!\n", command);
            err = -1;
            break;
    }
    
    return err;
}

/*----------------------------------------------------------------------------*/
int ist8303_orientation_operate(void* self, uint32_t command, void* buff_in, int size_in,
        void* buff_out, int size_out, int* actualout)
{
    int err = 0;
    int value, sample_delay, status=0;
    hwm_sensor_data* osensor_data=NULL;
	struct ist8303_i2c_data *obj = (struct ist8303_i2c_data *)self;

	if(atomic_read(&obj->trace) & IST_TRC_DEBUG)
	{
    	MSE_FUN(f);
	}
	
    switch (command)
    {
        case SENSOR_DELAY:
            if((buff_in == NULL) || (size_in < sizeof(int)))
            {
                MSE_ERR("Set delay parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                value = *(int *)buff_in;
                if(value <= 20)
                {
                    value = 20;
                }
                ist8303mid_data.controldata[0] = value;  // Loop Delay
            }   
            break;

        case SENSOR_ENABLE:
            if((buff_in == NULL) || (size_in < sizeof(int)))
            {
                MSE_ERR("Enable sensor parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                value = *(int *)buff_in;
                read_lock(&ist8303mid_data.ctrllock);
                if(value == 1)
                {
                    if (ist8303mid_data.controldata[7] == 0)
                    {
                        schedule_delayed_work(&ist_get_raw_data_work, msecs_to_jiffies(50));
                    }
                    ist8303mid_data.controldata[7] |= SENSOR_ORIENTATION;
                    atomic_set(&o_flag, 1);
                    atomic_set(&open_flag, 1);
                }
                else
                {
                    ist8303mid_data.controldata[7] &= ~SENSOR_ORIENTATION;
                    atomic_set(&o_flag, 0);
                    if(atomic_read(&m_flag) == 0)
                    {
                        atomic_set(&open_flag, 0);
                    }
                    if (ist8303mid_data.controldata[7] == 0)
                    {
                        cancel_delayed_work_sync(&ist_get_raw_data_work);
                    }
                }
                wake_up(&open_wq);
                read_unlock(&ist8303mid_data.ctrllock);

				MSE_LOG("osensor enable/disable ok!status = %d\n",atomic_read(&o_flag));//debug
                // Do nothing
            }
            break;

        case SENSOR_GET_DATA:
            //MSE_LOG("+++++++++++MSENSOR_GET_ORIENTATION_DATA");
            if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
            {
                MSE_ERR("get sensor data parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                osensor_data = (hwm_sensor_data *)buff_out;
                read_lock(&ist8303mid_data.datalock);
                osensor_data->values[0] = ist8303mid_data.yaw;
                osensor_data->values[1] = ist8303mid_data.pitch;
                osensor_data->values[2] = ist8303mid_data.roll;
                status = ist8303mid_data.mag_status;
                read_unlock(&ist8303mid_data.datalock); 
                
                
                osensor_data->value_divide = ORIENTATION_ACCURACY_RATE;             
            
	            switch (status)
	            {
	                case 1: case 2:
	                    osensor_data->status = SENSOR_STATUS_ACCURACY_HIGH;
	                    break;
	                case 3:
	                    osensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
	                    break;
	                case 4:
	                    osensor_data->status = SENSOR_STATUS_ACCURACY_LOW;
	                    break;
	                default:        
	                    osensor_data->status = SENSOR_STATUS_UNRELIABLE;
	                    break;    
	            }
				if(atomic_read(&obj->trace) & IST_TRC_O_DATA)
				{
					MSE_LOG("osensor data: %d, %d, %d, %d\n",osensor_data->values[0],osensor_data->values[1],osensor_data->values[2],osensor_data->status);//debug
            	}
            }
            break;
        default:
            MSE_ERR("gsensor operate function no this parameter %d!\n", command);
            err = -1;
            break;
    }

    return err;
}

/*----------------------------------------------------------------------------*/
#ifdef SOFT_GYRO
int ist8303_gyro_operate(void* self, uint32_t command, void* buff_in, int size_in,
        void* buff_out, int size_out, int* actualout)
{
    int err = 0;
    int value ,status;
    hwm_sensor_data* gysensor_data; 
	struct ist8303_i2c_data *obj = (struct ist8303_i2c_data *)self;

	if(atomic_read(&obj->trace) & IST_TRC_DEBUG)
	{
		MSE_FUN(f);
	}


    switch (command)
    {
        case SENSOR_DELAY:
            MSE_LOG("ist8303_gyro_delay");
            if((buff_in == NULL) || (size_in < sizeof(int)))
            {
                printk(KERN_ERR "Set delay parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                value = *(int *)buff_in;
                if(value <= 20)
                {
                    value = 20;
                }
                ist8303mid_data.controldata[0] = value;  // Loop Delay
            }   
            break;

        case SENSOR_ENABLE:
            MSE_LOG("ist8303_gyro_enable");
            if((buff_in == NULL) || (size_in < sizeof(int)))
            {
                printk(KERN_ERR "Enable sensor parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                
                value = *(int *)buff_in;
                read_lock(&ist8303mid_data.ctrllock);
                if(value == 1)
                {
                    if (ist8303mid_data.controldata[7] == 0)
                    {
                        schedule_delayed_work(&ist_get_raw_data_work, msecs_to_jiffies(50));
                    }
                    ist8303mid_data.controldata[7] |= SENSOR_GYROSCOPE;
                    atomic_set(&o_flag, 1);
                    atomic_set(&open_flag, 1);
                }
                else
                {
                    ist8303mid_data.controldata[7] &= ~SENSOR_GYROSCOPE;
                    atomic_set(&o_flag, 0);
                    if(atomic_read(&m_flag) == 0)
                    {
                        atomic_set(&open_flag, 0);
                    }
                    if (ist8303mid_data.controldata[7] == 0)
                    {
                        cancel_delayed_work_sync(&ist_get_raw_data_work);
                    }
                }
                wake_up(&open_wq);
                read_unlock(&ist8303mid_data.ctrllock);
            }
            break;

        case SENSOR_GET_DATA:
           // MSE_LOG("ist8303_gyro_data");
            if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
            {
                printk(KERN_ERR "get sensor data parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                gysensor_data = (hwm_sensor_data *)buff_out;

                read_lock(&ist8303mid_data.datalock);
                gysensor_data->values[0] = ist8303mid_data.ngx / 10;
                gysensor_data->values[1] = ist8303mid_data.ngy / 10;
                gysensor_data->values[2] = ist8303mid_data.ngz / 10;
                status = ist8303mid_data.gyro_status;
                read_unlock(&ist8303mid_data.datalock); 
				
                if(atomic_read(&obj->trace) & IST_TRC_GYRO_DATA)
                {
					MSE_LOG("[qnmd] gyrosensor x = %d ,y = %d z = %d\n",gysensor_data->values[0],gysensor_data->values[1],gysensor_data->values[2]);
                }
				
				gysensor_data->value_divide = 10;

                switch (status)
                {
                    case 1: case 2:
                        gysensor_data->status = SENSOR_STATUS_ACCURACY_HIGH;
                        break;
                    case 3:
                        gysensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
                        break;
                    case 4:
                        gysensor_data->status = SENSOR_STATUS_ACCURACY_LOW;
                        break;
                    default:        
                        gysensor_data->status = SENSOR_STATUS_UNRELIABLE;
                        break;    
                }
                
            }
            break;
        default:
            printk(KERN_ERR "gsensor operate function no this parameter %d!\n", command);
            err = -1;
            break;
    }
    
    return err;
}

int ist8303_rotation_vector_operate(void* self, uint32_t command, void* buff_in, int size_in,
        void* buff_out, int size_out, int* actualout)
{
    int err = 0;
    int value ,status;
    hwm_sensor_data* gysensor_data; 
	struct ist8303_i2c_data *obj = (struct ist8303_i2c_data *)self;

	if(atomic_read(&obj->trace) & IST_TRC_DEBUG)
	{
		MSE_FUN(f);
	}

    switch (command)
    {
        case SENSOR_DELAY:
            MSE_LOG("ist8303_rotation_vector_delay");
            if((buff_in == NULL) || (size_in < sizeof(int)))
            {
                printk(KERN_ERR "Set delay parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                value = *(int *)buff_in;
                if(value <= 20)
                {
                    value = 20;
                }
                ist8303mid_data.controldata[0] = value;  // Loop Delay
            }   
            break;

        case SENSOR_ENABLE:
            MSE_LOG("ist8303_rotation_vector_enable");
            if((buff_in == NULL) || (size_in < sizeof(int)))
            {
                printk(KERN_ERR "Enable sensor parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                value = *(int *)buff_in;
                read_lock(&ist8303mid_data.ctrllock);
                if(value == 1)
                {
                    if (ist8303mid_data.controldata[7] == 0)
                    {
                        schedule_delayed_work(&ist_get_raw_data_work, msecs_to_jiffies(50));
                    }
                    ist8303mid_data.controldata[7] |= SENSOR_ROTATION_VECTOR;
                    atomic_set(&o_flag, 1);
                    atomic_set(&open_flag, 1);
                }
                else
                {
                    ist8303mid_data.controldata[7] &= ~SENSOR_ROTATION_VECTOR;
                    atomic_set(&o_flag, 0);
                    if(atomic_read(&m_flag) == 0)
                    {
                        atomic_set(&open_flag, 0);
                    }
                    if (ist8303mid_data.controldata[7] == 0)
                    {
                        cancel_delayed_work_sync(&ist_get_raw_data_work);
                    }
                }
                wake_up(&open_wq);
                read_unlock(&ist8303mid_data.ctrllock);
            }
            break;

        case SENSOR_GET_DATA:
           // MSE_ERR("ist8303_rotation_vector_data");
            if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
            {
                printk(KERN_ERR "get sensor data parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                gysensor_data = (hwm_sensor_data *)buff_out;

                read_lock(&ist8303mid_data.datalock);
                gysensor_data->values[0] = ist8303mid_data.rv[0];
                gysensor_data->values[1] = ist8303mid_data.rv[1];
                gysensor_data->values[2] = ist8303mid_data.rv[2];
                status = ist8303mid_data.rv_status;
                read_unlock(&ist8303mid_data.datalock); 

				if(atomic_read(&obj->trace) & IST_TRC_LINEAR_ACC_DATA)
				{
                	MSE_LOG("[qnmd] rotation_vec_sensor x = %d ,y = %d z = %d\n",gysensor_data->values[0],gysensor_data->values[1],gysensor_data->values[2]);
				}
				gysensor_data->value_divide = 100000;

                switch (status)
                {
                    case 1: case 2:
                        gysensor_data->status = SENSOR_STATUS_ACCURACY_HIGH;
                        break;
                    case 3:
                        gysensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
                        break;
                    case 4:
                        gysensor_data->status = SENSOR_STATUS_ACCURACY_LOW;
                        break;
                    default:        
                        gysensor_data->status = SENSOR_STATUS_UNRELIABLE;
                        break;    
                }
                
            }
            break;
        default:
            printk(KERN_ERR "gsensor operate function no this parameter %d!\n", command);
            err = -1;
            break;
    }
    
    return err;
}

#ifdef REPLACE_ANDROID_VIRTUAL_SENSOR
int ist8303_gravity_operate(void* self, uint32_t command, void* buff_in, int size_in,
        void* buff_out, int size_out, int* actualout)
{
    int err = 0;
    int value ,status;
    hwm_sensor_data* gysensor_data; 
	struct ist8303_i2c_data *obj = (struct ist8303_i2c_data *)self;

	if(atomic_read(&obj->trace) & IST_TRC_DEBUG)
	{
		MSE_FUN(f);
	}


    switch (command)
    {
        case SENSOR_DELAY:
            MSE_ERR("ist8303_gravity_delay");
            if((buff_in == NULL) || (size_in < sizeof(int)))
            {
                printk(KERN_ERR "Set delay parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                value = *(int *)buff_in;
                if(value <= 20)
                {
                    value = 20;
                }
                ist8303mid_data.controldata[0] = value;  // Loop Delay
            }   
            break;

        case SENSOR_ENABLE:
            MSE_ERR("ist8303_gravity_enable");
            if((buff_in == NULL) || (size_in < sizeof(int)))
            {
                printk(KERN_ERR "Enable sensor parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                value = *(int *)buff_in;
                read_lock(&ist8303mid_data.ctrllock);
                if(value == 1)
                {
                    if (ist8303mid_data.controldata[7] == 0)
                    {
                        schedule_delayed_work(&ist_get_raw_data_work, msecs_to_jiffies(50));
                    }
                    ist8303mid_data.controldata[7] |= SENSOR_GRAVITY;
                    atomic_set(&o_flag, 1);
                    atomic_set(&open_flag, 1);
                }
                else
                {
                    ist8303mid_data.controldata[7] &= ~SENSOR_GRAVITY;
                    atomic_set(&o_flag, 0);
                    if(atomic_read(&m_flag) == 0)
                    {
                        atomic_set(&open_flag, 0);
                    }
                    if (ist8303mid_data.controldata[7] == 0)
                    {
                        cancel_delayed_work_sync(&ist_get_raw_data_work);
                    }
                }
                wake_up(&open_wq);
                read_unlock(&ist8303mid_data.ctrllock);
            }
            break;

        case SENSOR_GET_DATA:
            //MSE_ERR("ist8303_gravity_data");
            if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
            {
                printk(KERN_ERR "get sensor data parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                gysensor_data = (hwm_sensor_data *)buff_out;

                read_lock(&ist8303mid_data.datalock);
                gysensor_data->values[0] = ist8303mid_data.ngrx;
                gysensor_data->values[1] = ist8303mid_data.ngry;
                gysensor_data->values[2] = ist8303mid_data.ngrz;
                status = ist8303mid_data.gra_status;
                read_unlock(&ist8303mid_data.datalock); 
                
                //printk("[qnmd] gysensor x = %d ,y = %d z = %d\n",gysensor_data->values[0],gysensor_data->values[1],gysensor_data->values[2]);
                gysensor_data->value_divide = 1000;

                switch (status)
                {
                    case 1: case 2:
                        gysensor_data->status = SENSOR_STATUS_ACCURACY_HIGH;
                        break;
                    case 3:
                        gysensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
                        break;
                    case 4:
                        gysensor_data->status = SENSOR_STATUS_ACCURACY_LOW;
                        break;
                    default:        
                        gysensor_data->status = SENSOR_STATUS_UNRELIABLE;
                        break;    
                }
                if(atomic_read(&obj->trace) & IST_TRC_GRAVITY_DATA)
                {
					MSE_LOG("[qnmd] gravitysensor x = %d ,y = %d z = %d\n",gysensor_data->values[0],gysensor_data->values[1],gysensor_data->values[2]);
				}
            }
            break;
        default:
            printk(KERN_ERR "gravitysensor operate function no this parameter %d!\n", command);
            err = -1;
            break;
    }
    
    return err;
}

int ist8303_linear_acceleration_operate(void* self, uint32_t command, void* buff_in, int size_in,
        void* buff_out, int size_out, int* actualout)
{
    int err = 0;
    int value ,status;
    hwm_sensor_data* gysensor_data; 
	struct ist8303_i2c_data *obj = (struct ist8303_i2c_data *)self;

	if(atomic_read(&obj->trace) & IST_TRC_DEBUG)
	{
		MSE_FUN(f);
	}


    switch (command)
    {
        case SENSOR_DELAY:
            MSE_ERR("ist8303_linear_acceleration_delay");
            if((buff_in == NULL) || (size_in < sizeof(int)))
            {
                printk(KERN_ERR "Set delay parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                value = *(int *)buff_in;
                if(value <= 20)
                {
                    value = 20;
                }
                ist8303mid_data.controldata[0] = value;  // Loop Delay
            }   
            break;

        case SENSOR_ENABLE:
            MSE_ERR("ist8303_linear_acceleration_enable");
            if((buff_in == NULL) || (size_in < sizeof(int)))
            {
                printk(KERN_ERR "Enable sensor parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                value = *(int *)buff_in;
                read_lock(&ist8303mid_data.ctrllock);
                if(value == 1)
                {
                    if (ist8303mid_data.controldata[7] == 0)
                    {
                        schedule_delayed_work(&ist_get_raw_data_work, msecs_to_jiffies(50));
                    }
                    ist8303mid_data.controldata[7] |= SENSOR_LINEAR_ACCELERATION;
                    atomic_set(&o_flag, 1);
                    atomic_set(&open_flag, 1);
                }
                else
                {
                    ist8303mid_data.controldata[7] &= ~SENSOR_LINEAR_ACCELERATION;
                    atomic_set(&o_flag, 0);
                    if(atomic_read(&m_flag) == 0)
                    {
                        atomic_set(&open_flag, 0);
                    }
                    if (ist8303mid_data.controldata[7] == 0)
                    {
                        cancel_delayed_work_sync(&ist_get_raw_data_work);
                    }
                }
                wake_up(&open_wq);
                read_unlock(&ist8303mid_data.ctrllock);
            }
            break;

        case SENSOR_GET_DATA:
            //MSE_ERR("ist8303_linear_acceleration_data");
            if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
            {
                printk(KERN_ERR "get sensor data parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                gysensor_data = (hwm_sensor_data *)buff_out;

                read_lock(&ist8303mid_data.datalock);
                gysensor_data->values[0] = ist8303mid_data.nlax;
                gysensor_data->values[1] = ist8303mid_data.nlay;
                gysensor_data->values[2] = ist8303mid_data.nlaz;
                status = ist8303mid_data.la_status;
                read_unlock(&ist8303mid_data.datalock); 

				if(atomic_read(&obj->trace) & IST_TRC_LINEAR_ACC_DATA)
				{
                	MSE_LOG("[qnmd] linear_accsensor x = %d ,y = %d z = %d\n",gysensor_data->values[0],gysensor_data->values[1],gysensor_data->values[2]);
				}
				gysensor_data->value_divide = 1000;

                switch (status)
                {
                    case 1: case 2:
                        gysensor_data->status = SENSOR_STATUS_ACCURACY_HIGH;
                        break;
                    case 3:
                        gysensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
                        break;
                    case 4:
                        gysensor_data->status = SENSOR_STATUS_ACCURACY_LOW;
                        break;
                    default:        
                        gysensor_data->status = SENSOR_STATUS_UNRELIABLE;
                        break;    
                }
                
            }
            break;
        default:
            printk(KERN_ERR "gsensor operate function no this parameter %d!\n", command);
            err = -1;
            break;
    }
    
    return err;
}

#endif //#ifdef REPLACE_ANDROID_VIRTUAL_SENSOR
#endif 
/*--------------------------------------------------------------------*/
#ifdef IST8303_M_NEW_ARCH

static int ist8303_m_open_report_data(int en)
{
	return 0;
}
static int ist8303_m_set_delay(u64 delay)
{
	//int value = (int)delay/1000/1000;
	int value = (int)delay;
	
	if(value <= 20)
	{
		value = 20;
	}
	ist8303mid_data.controldata[0] = value;  // Loop Delay

	return 0;
}
static int ist8303_m_enable(int en)
{
	read_lock(&ist8303mid_data.ctrllock);
	if(en == 1)
	{
		if (ist8303mid_data.controldata[7] == 0)
		{
			schedule_delayed_work(&ist_get_raw_data_work, msecs_to_jiffies(50));
		}
		ist8303mid_data.controldata[7] |= SENSOR_MAGNETIC;
		atomic_set(&m_flag, 1);
		atomic_set(&open_flag, 1);
	}
	else
	{
		ist8303mid_data.controldata[7] &= ~SENSOR_MAGNETIC;
		atomic_set(&m_flag, 0);
		if(atomic_read(&o_flag) == 0)
		{
			atomic_set(&open_flag, 0);
		}
		if (ist8303mid_data.controldata[7] == 0)
		{
			cancel_delayed_work_sync(&ist_get_raw_data_work);
		}
	}
	wake_up(&open_wq);
	read_unlock(&ist8303mid_data.ctrllock);

	MSE_LOG("msensor enable/disable ok!status = %d\n",atomic_read(&m_flag));//debug

	return 0;
}
static int ist8303_o_open_report_data(int en)
{
	return 0;
}
static int ist8303_o_set_delay(u64 delay)
{
	//int value = (int)delay/1000/1000;
	int value = (int)delay;

	if(value <= 20)
	{
		value = 20;
	}
	ist8303mid_data.controldata[0] = value;  // Loop Delay

	return 0;
}
static int ist8303_o_enable(int en)
{
	read_lock(&ist8303mid_data.ctrllock);
	if(en == 1)
	{
		if (ist8303mid_data.controldata[7] == 0)
		{
			schedule_delayed_work(&ist_get_raw_data_work, msecs_to_jiffies(50));
		}
		ist8303mid_data.controldata[7] |= SENSOR_ORIENTATION;
		atomic_set(&o_flag, 1);
		atomic_set(&open_flag, 1);
	}
	else
	{
		ist8303mid_data.controldata[7] &= ~SENSOR_ORIENTATION;
		atomic_set(&o_flag, 0);
		if(atomic_read(&m_flag) == 0)
		{
			atomic_set(&open_flag, 0);
		}
		if (ist8303mid_data.controldata[7] == 0)
		{
			cancel_delayed_work_sync(&ist_get_raw_data_work);
		}
	}
	wake_up(&open_wq);
	read_unlock(&ist8303mid_data.ctrllock);

	MSE_LOG("osensor enable/disable ok!status = %d\n",atomic_read(&o_flag));//debug
	                // Do nothing
	return 0;
}

static int ist8303_o_get_data(int* x ,int* y,int* z, int* status)
{
	int status_temp = 0;
	
	read_lock(&ist8303mid_data.datalock);
	*x = ist8303mid_data.yaw;
	*y = ist8303mid_data.pitch;
	*z = ist8303mid_data.roll;
	status_temp = ist8303mid_data.mag_status;
	read_unlock(&ist8303mid_data.datalock); 
	
	switch (status_temp)
	{
		case 1: case 2:
			*status = SENSOR_STATUS_ACCURACY_HIGH;
			break;
		case 3:
			*status = SENSOR_STATUS_ACCURACY_MEDIUM;
			break;
		case 4:
			*status = SENSOR_STATUS_ACCURACY_LOW;
			break;
		default:		
			*status = SENSOR_STATUS_UNRELIABLE;
			break;	  
	}		
	return 0;
}

static int ist8303_m_get_data(int* x ,int* y,int* z, int* status)
{
	int status_temp = 0;
	
	read_lock(&ist8303mid_data.datalock);
	*x = ist8303mid_data.nmx;
	*y = ist8303mid_data.nmy;
	*z = ist8303mid_data.nmz;
	status_temp = ist8303mid_data.mag_status;
	read_unlock(&ist8303mid_data.datalock); 
	
	*x = *x * CONVERT_M;
	*y = *y * CONVERT_M;
	*z = *z * CONVERT_M;
		
	switch (status_temp)
	{
		case 1: case 2:
			*status = SENSOR_STATUS_ACCURACY_HIGH;
			break;
		case 3:
			*status = SENSOR_STATUS_ACCURACY_MEDIUM;
			break;
		case 4:
			*status = SENSOR_STATUS_ACCURACY_LOW;
			break;
		default:		
			*status = SENSOR_STATUS_UNRELIABLE;
			break;	  
	}

	return 0;
}

#endif

/*----------------------------------------------------------------------------*/
//#ifndef   CONFIG_HAS_EARLYSUSPEND
/*----------------------------------------------------------------------------*/
static int ist8303_suspend(struct i2c_client *client, pm_message_t msg) 
{
    int err;
    struct ist8303_i2c_data *obj = i2c_get_clientdata(client);
    MSE_FUN();

    cancel_delayed_work_sync(&ist_get_raw_data_work);    

    if(msg.event == PM_EVENT_SUSPEND)
    {   
        if((err = hwmsen_write_byte(client, IST8303_REG_CNTRL1, 0x00))!=0)
        {
            MSE_ERR("write power control fail!!\n");
            return err;
        }

        ist8303_power(obj->hw, 0);
    }
    return 0;
}
/*----------------------------------------------------------------------------*/
static int ist8303_resume(struct i2c_client *client)
{
    int err;
    struct ist8303_i2c_data *obj = i2c_get_clientdata(client);
    MSE_FUN();

    ist8303_power(obj->hw, 1);
    if((err = IST8303_Chipset_Init(IST8303_FORCE_MODE))!=0)
    {
        MSE_ERR("initialize client fail!!\n");
        return err;        
    }
	
	if (ist8303mid_data.controldata[7] != 0)
    {
    	schedule_delayed_work(&ist_get_raw_data_work, msecs_to_jiffies(50));
    }
    //schedule_delayed_work(&ist_get_raw_data_work, msecs_to_jiffies(50));

    return 0;
}
/*----------------------------------------------------------------------------*/
//#else /*CONFIG_HAS_EARLY_SUSPEND is defined*/
/*----------------------------------------------------------------------------*/
static void ist8303_early_suspend(struct early_suspend *h) 
{
    struct ist8303_i2c_data *obj = container_of(h, struct ist8303_i2c_data, early_drv);   
    int err;
    MSE_FUN();    

    if(NULL == obj)
    {
        MSE_ERR("null pointer!!\n");
        return;
    }

	cancel_delayed_work_sync(&ist_get_raw_data_work);  
	
    if((err = hwmsen_write_byte(obj->client, IST8303_REG_CNTRL1, 0x00)))
    {
        MSE_ERR("write power control fail!!\n");
        return;
    }        
	ist8303_power(obj->hw, 0);
}
/*----------------------------------------------------------------------------*/
static void ist8303_late_resume(struct early_suspend *h)
{
    struct ist8303_i2c_data *obj = container_of(h, struct ist8303_i2c_data, early_drv);         
    int err;
    MSE_FUN();

    if(NULL == obj)
    {
        MSE_ERR("null pointer!!\n");
        return;
    }

    ist8303_power(obj->hw, 1);
    if((err = IST8303_Chipset_Init(IST8303_FORCE_MODE)))
    {
        MSE_ERR("initialize client fail!!\n");
        return;        
    }    

	if (ist8303mid_data.controldata[7] != 0)
    {
    	schedule_delayed_work(&ist_get_raw_data_work, msecs_to_jiffies(50));
    }
}
/*----------------------------------------------------------------------------*/
//#endif /*CONFIG_HAS_EARLYSUSPEND*/
/*----------------------------------------------------------------------------*/
/*
static int ist8303_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) 
{    
    strcpy(info->type, IST8303_DEV_NAME);
    return 0;
}
*/

/*----------------------------------------------------------------------------*/
static int ist8303_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct i2c_client *new_client;
    struct ist8303_i2c_data *data;
    int err = 0;
#ifdef IST8303_M_NEW_ARCH
	struct mag_control_path ctl={0};
	struct mag_data_path mag_data={0}; 
#else
	struct hwmsen_object sobj_m, sobj_o;
#endif

#ifdef SOFT_GYRO
    struct hwmsen_object sobj_gy, sobj_rv;
#ifdef REPLACE_ANDROID_VIRTUAL_SENSOR
    struct hwmsen_object sobj_gra, sobj_la;
#endif //#ifdef REPLACE_ANDROID_VIRTUAL_SENSOR
#endif

    if (!(data = kmalloc(sizeof(struct ist8303_i2c_data), GFP_KERNEL)))
    {
        err = -ENOMEM;
        goto exit;
    }
    memset(data, 0, sizeof(struct ist8303_i2c_data));

    data->hw = get_cust_mag_hw();
    if((err = hwmsen_get_convert(data->hw->direction, &data->cvt)))
    {
        MSE_ERR("invalid direction: %d\n", data->hw->direction);
        goto exit_kfree;
    }
    
    atomic_set(&data->layout, data->hw->direction);
    atomic_set(&data->trace, 0);
    atomic_set(&ist830x_data_ready, 0);
    init_waitqueue_head(&data_ready_wq);
    init_waitqueue_head(&open_wq);

    data->client = client;
    new_client = data->client;
    i2c_set_clientdata(new_client, data);
    
    ist8303_i2c_client = new_client;

    //ist8303_i2c_client->addr = (ist8303_i2c_client->addr & I2C_MASK_FLAG )|(I2C_ENEXT_FLAG);
    //memset(ist830x_msensor_raw_data, 0, sizeof(ist830x_msensor_raw_data));

    write_lock(&ist830x_data.lock);
    ist830x_data.mode = IST8303_NORMAL_MODE;
    write_unlock(&ist830x_data.lock);

    mutex_init(&sensor_data_mutex);
    INIT_DELAYED_WORK(&ist_get_raw_data_work, ist830x_prepare_raw_data);

    if((err = IST8303_Chipset_Init(IST8303_NORMAL_MODE)))
    {
        goto exit_init_failed;
    }

    /* Register sysfs attribute */
#ifdef IST8303_M_NEW_ARCH

    if((err = ist8303_create_attr(&(ist8303_init_info.platform_diver_addr->driver))))
#else
	if((err = ist8303_create_attr(&ist_sensor_driver.driver)))
#endif
	{
        MSE_ERR("create attribute err = %d\n", err);
        goto exit_sysfs_create_group_failed;
    }

    if((err = misc_register(&ist8303_device)))
    {
        MSE_ERR("ist8303_device register failed\n");
        goto exit_misc_device_register_failed;
    } 
	
#ifndef IST8303_M_NEW_ARCH
    sobj_m.self = data;
    sobj_m.polling = 1;
#endif

#ifdef IST8303_M_NEW_ARCH	
#else
	sobj_m.sensor_operate = ist8303_operate;
	if(err = hwmsen_attach(ID_MAGNETIC, &sobj_m))
#endif
    {
        MSE_ERR("attach fail = %d\n", err);
        goto exit_kfree;
    }

#ifndef IST8303_M_NEW_ARCH    
    sobj_o.self = data;
    sobj_o.polling = 1;
#endif

#ifdef IST8303_M_NEW_ARCH
#else
	sobj_o.sensor_operate = ist8303_orientation_operate;
	if(err = hwmsen_attach(ID_ORIENTATION, &sobj_o))
#endif
    {
        MSE_ERR("attach fail = %d\n", err);
        goto exit_kfree;
    }

#ifdef IST8303_M_NEW_ARCH

	ctl.m_enable = ist8303_m_enable;
	ctl.m_set_delay  = ist8303_m_set_delay;
	ctl.m_open_report_data = ist8303_m_open_report_data;
	ctl.o_enable = ist8303_o_enable;
	ctl.o_set_delay  = ist8303_o_set_delay;
	ctl.o_open_report_data = ist8303_o_open_report_data;
	ctl.is_report_input_direct = false;

	ctl.is_support_batch = data->hw->is_batch_supported;
	
	err = mag_register_control_path(&ctl);
	if(err)
	{
	 	MAG_ERR("register mag control path err\n");
		goto exit_kfree;
	}

	mag_data.div_m = 100;
	mag_data.div_o = ORIENTATION_ACCURACY_RATE;
	mag_data.get_data_o = ist8303_o_get_data;
	mag_data.get_data_m = ist8303_m_get_data;
	
	err = mag_register_data_path(&mag_data);
	if(err)
	{
	 	MAG_ERR("register data control path err\n");
		goto exit_kfree;
	}
/*--------------------------------------------------------------*/
#if 0
	err = batch_register_support_info(ID_MAGNETIC,data->hw->is_batch_supported, 0);
	if(err)
	{
		MAG_ERR("%s register mag batch support err = %d\n", __func__, err);
		goto exit_kfree;
	}
	err = batch_register_support_info(ID_ORIENTATION,data->hw->is_batch_supported, 0);
	if(err)
	{
		MAG_ERR("%s register ori batch support err = %d\n", __func__, err);
		goto exit_kfree;
	}
#endif
/*--------------------------------------------------------------*/

#endif
	
#ifdef SOFT_GYRO  
    sobj_gy.self = data;
    sobj_gy.polling = 1;
    sobj_gy.sensor_operate = ist8303_gyro_operate;
    if(err = hwmsen_attach(ID_GYROSCOPE, &sobj_gy))
    {
        printk(KERN_ERR "attach fail = %d\n", err);
        goto exit_kfree;
    }

    sobj_rv.self = data;
    sobj_rv.polling = 1;
    sobj_rv.sensor_operate = ist8303_rotation_vector_operate;
    if(err = hwmsen_attach(ID_ROTATION_VECTOR, &sobj_rv))
    {
        printk(KERN_ERR "attach fail = %d\n", err);
        goto exit_kfree;
    }

#ifdef REPLACE_ANDROID_VIRTUAL_SENSOR
    sobj_gra.self = data;
    sobj_gra.polling = 1;
    sobj_gra.sensor_operate = ist8303_gravity_operate;
    if(err = hwmsen_attach(ID_GRAVITY, &sobj_gra))
    {
        printk(KERN_ERR "attach fail = %d\n", err);
        goto exit_kfree;
    }

    sobj_la.self = data;
    sobj_la.polling = 1;
    sobj_la.sensor_operate = ist8303_linear_acceleration_operate;
    if(err = hwmsen_attach(ID_LINEAR_ACCELERATION, &sobj_la))
    {
        printk(KERN_ERR "attach fail = %d\n", err);
        goto exit_kfree;
    }
#endif //#ifdef REPLACE_ANDROID_VIRTUAL_SENSOR
#endif
    
#if CONFIG_HAS_EARLYSUSPEND
    data->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
    data->early_drv.suspend  = ist8303_early_suspend,
    data->early_drv.resume   = ist8303_late_resume,    
    register_early_suspend(&data->early_drv);
#endif
#ifdef IST8303_M_NEW_ARCH

	ist8303_init_flag = 1;
#endif
    MSE_LOG("%s: OK\n", __func__);
    return 0;

    exit_sysfs_create_group_failed:   
    exit_init_failed:
    exit_misc_device_register_failed:
    exit_kfree:
    kfree(data);
    exit:
    MSE_ERR("%s: err = %d\n", __func__, err);
    return err;
}
/*----------------------------------------------------------------------------*/
static int ist8303_i2c_remove(struct i2c_client *client)
{
    int err;    
#ifdef IST8303_M_NEW_ARCH
	if((err = ist8303_delete_attr(&(ist8303_init_info.platform_diver_addr->driver))))
#else
    if((err = ist8303_delete_attr(&ist_sensor_driver.driver)))
#endif
    {
        MSE_ERR("ist8303_delete_attr fail: %d\n", err);
    }
    
    ist8303_i2c_client = NULL;
    i2c_unregister_device(client);
    kfree(i2c_get_clientdata(client));  
    misc_deregister(&ist8303_device);    
    return 0;
}
/*----------------------------------------------------------------------------*/
#ifdef IST8303_M_NEW_ARCH

static int ist8303_local_init(void)
{
	struct mag_hw *hw = get_cust_mag_hw();

    ist8303_power(hw, 1);    
    rwlock_init(&ist8303mid_data.ctrllock);
    rwlock_init(&ist8303mid_data.datalock);
    rwlock_init(&ist830x_data.lock);
    memset(&ist8303mid_data.controldata[0], 0, sizeof(int)*10);    
    ist8303mid_data.controldata[0] =    20;  // Loop Delay
    ist8303mid_data.controldata[1] =     0;  // Run   
    ist8303mid_data.controldata[2] =     0;  // Disable Start-AccCali
    ist8303mid_data.controldata[3] =     1;  // Enable Start-Cali
    ist8303mid_data.controldata[4] =   350;  // MW-Timout
    ist8303mid_data.controldata[5] =    10;  // MW-IIRStrength_M
    ist8303mid_data.controldata[6] =    10;  // MW-IIRStrength_G   
    ist8303mid_data.controldata[7] =     0;  // Active Sensors
    ist8303mid_data.controldata[8] =     ROTATION_VECTOR_RESPONSE_MODE; // Rotation Vector response mode
    ist8303mid_data.controldata[9] =     0;  // Wait for define   
    atomic_set(&dev_open_count, 0);
    //ist8303_force[0] = hw->i2c_num;

    if(i2c_add_driver(&ist8303_i2c_driver))
    {
        MSE_ERR("add driver error\n");
        return -1;
    }
	
	if(-1 == ist8303_init_flag)
	{
		MSE_ERR("%s failed!\n",__func__);
	   return -1;
	}
    return 0;
}

static int ist8303_remove(void)
{
	struct mag_hw *hw = get_cust_mag_hw();
	
	MSE_FUN(f);	 
	ist8303_power(hw, 0);	
	atomic_set(&dev_open_count, 0);	
	i2c_del_driver(&ist8303_i2c_driver);

	return 0;
}
#endif

/*---------------------------------------------------------------------------------*/
#ifndef IST8303_M_NEW_ARCH

static int ist_probe(struct platform_device *pdev) 
{
    struct mag_hw *hw = get_cust_mag_hw();

    ist8303_power(hw, 1);    
    rwlock_init(&ist8303mid_data.ctrllock);
    rwlock_init(&ist8303mid_data.datalock);
    rwlock_init(&ist830x_data.lock);
    memset(&ist8303mid_data.controldata[0], 0, sizeof(int)*10);    
    ist8303mid_data.controldata[0] =    20;  // Loop Delay
    ist8303mid_data.controldata[1] =     0;  // Run   
    ist8303mid_data.controldata[2] =     0;  // Disable Start-AccCali
    ist8303mid_data.controldata[3] =     1;  // Enable Start-Cali
    ist8303mid_data.controldata[4] =   350;  // MW-Timout
    ist8303mid_data.controldata[5] =    10;  // MW-IIRStrength_M
    ist8303mid_data.controldata[6] =    10;  // MW-IIRStrength_G   
    ist8303mid_data.controldata[7] =     0;  // Active Sensors
    ist8303mid_data.controldata[8] =     ROTATION_VECTOR_RESPONSE_MODE; // Rotation Vector response mode
    ist8303mid_data.controldata[9] =     0;  // Wait for define   
    atomic_set(&dev_open_count, 0);
    //ist8303_force[0] = hw->i2c_num;

    if(i2c_add_driver(&ist8303_i2c_driver))
    {
        MSE_ERR("add driver error\n");
        return -1;
    } 
    return 0;
}
/*----------------------------------------------------------------------------*/
static int ist_remove(struct platform_device *pdev)
{
    struct mag_hw *hw = get_cust_mag_hw();

    MSE_FUN();    
    ist8303_power(hw, 0);    
    atomic_set(&dev_open_count, 0);  
    i2c_del_driver(&ist8303_i2c_driver);
    return 0;
}
/*----------------------------------------------------------------------------*/


static struct platform_driver ist_sensor_driver = {
    .probe      = ist_probe,
    .remove     = ist_remove,    
    .driver     = {
        .name  = "msensor",
//      .owner = THIS_MODULE,
    }
};
#endif


/*----------------------------------------------------------------------------*/
static int __init ist8303_init(void)
{
    MSE_FUN();
    struct mag_hw *hw = get_cust_mag_hw();
    i2c_register_board_info(hw->i2c_num, &i2c_ist8303, 1);
	
#ifdef IST8303_M_NEW_ARCH
	mag_driver_add(&ist8303_init_info);
#else
    if(platform_driver_register(&ist_sensor_driver))
    {
        MSE_ERR("failed to register driver");
        return -ENODEV;
    }
#endif
		
    return 0;    
}
/*----------------------------------------------------------------------------*/
static void __exit ist8303_exit(void)
{
    MSE_FUN();
#ifndef IST8303_M_NEW_ARCH
    platform_driver_unregister(&ist_sensor_driver);
#endif
}
/*----------------------------------------------------------------------------*/
module_init(ist8303_init);
module_exit(ist8303_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("iSentek");
MODULE_DESCRIPTION("IST8303 M-Sensor driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);
