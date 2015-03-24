/* drivers/i2c/chips/lsm330.c - LSM330 motion sensor driver
 *
 *
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
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>


#include <cust_acc.h>
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include "lsm330.h"
#include <linux/hwmsen_helper.h>

//#include <mach/mt_devs.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>


#define POWER_NONE_MACRO MT65XX_POWER_NONE



/*----------------------------------------------------------------------------*/
//#define I2C_DRIVERID_LSM330 345
/*----------------------------------------------------------------------------*/
#define DEBUG 1
/*----------------------------------------------------------------------------*/
#define CONFIG_LSM330_LOWPASS   /*apply low pass filter on output*/       
/*----------------------------------------------------------------------------*/
#define LSM330_AXIS_X          0
#define LSM330_AXIS_Y          1
#define LSM330_AXIS_Z          2
#define LSM330_AXES_NUM        3
#define LSM330_DATA_LEN        6
#define LSM330_DEV_NAME        "LSM330"
/*----------------------------------------------------------------------------*/
static const struct i2c_device_id lsm330_i2c_id[] = {{LSM330_DEV_NAME,0},{}};
/*the adapter id will be available in customization*/
static struct i2c_board_info __initdata i2c_LSM330={ I2C_BOARD_INFO("LSM330", (LSM330_I2C_SLAVE_ADDR>>1))};

//static unsigned short lsm330_force[] = {0x00, LSM330_I2C_SLAVE_ADDR, I2C_CLIENT_END, I2C_CLIENT_END};
//static const unsigned short *const lsm330_forces[] = { lsm330_force, NULL };
//static struct i2c_client_address_data lsm330_addr_data = { .forces = lsm330_forces,};

/*----------------------------------------------------------------------------*/
static int lsm330_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id); 
static int lsm330_i2c_remove(struct i2c_client *client);
static int lsm330_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
/*----------------------------------------------------------------------------*/
typedef enum {
    ADX_TRC_FILTER  = 0x01,
    ADX_TRC_RAWDATA = 0x02,
    ADX_TRC_IOCTL   = 0x04,
    ADX_TRC_CALI	= 0X08,
    ADX_TRC_INFO	= 0X10,
} ADX_TRC;
/*----------------------------------------------------------------------------*/
struct scale_factor{
    u8  whole;
    u8  fraction;
};
/*----------------------------------------------------------------------------*/
struct data_resolution {
    struct scale_factor scalefactor;
    int                 sensitivity;
};
/*----------------------------------------------------------------------------*/
#define C_MAX_FIR_LENGTH (32)
/*----------------------------------------------------------------------------*/
struct data_filter {
    s16 raw[C_MAX_FIR_LENGTH][LSM330_AXES_NUM];
    int sum[LSM330_AXES_NUM];
    int num;
    int idx;
};
/*----------------------------------------------------------------------------*/
struct lsm330_i2c_data {
    struct i2c_client *client;
    struct acc_hw *hw;
    struct hwmsen_convert   cvt;
    
    /*misc*/
    struct data_resolution *reso;
    atomic_t                trace;
    atomic_t                suspend;
    atomic_t                selftest;
	atomic_t				filter;
    s16                     cali_sw[LSM330_AXES_NUM+1];

    /*data*/
    s8                      offset[LSM330_AXES_NUM+1];  /*+1: for 4-byte alignment*/
    s16                     data[LSM330_AXES_NUM+1];

#if defined(CONFIG_LSM330_LOWPASS)
    atomic_t                firlen;
    atomic_t                fir_en;
    struct data_filter      fir;
#endif 
    /*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend    early_drv;
#endif     
};
/*----------------------------------------------------------------------------*/
static struct i2c_driver lsm330_i2c_driver = {
    .driver = {
//        .owner          = THIS_MODULE,
        .name           = LSM330_DEV_NAME,
    },
	.probe      		= lsm330_i2c_probe,
	.remove    			= lsm330_i2c_remove,
	.detect				= lsm330_i2c_detect,
#if !defined(CONFIG_HAS_EARLYSUSPEND)    
    .suspend            = lsm330_suspend,
    .resume             = lsm330_resume,
#endif
	.id_table = lsm330_i2c_id,
//	.address_data = &lsm330_addr_data,
};

/*----------------------------------------------------------------------------*/
static struct i2c_client *lsm330_i2c_client = NULL;
static struct platform_driver lsm330_gsensor_driver;
static struct lsm330_i2c_data *obj_i2c_data = NULL;
static bool sensor_power = false; //true;
static GSENSOR_VECTOR3D gsensor_gain, gsensor_offset;
//static char selftestRes[10] = {0};



/*----------------------------------------------------------------------------*/
#define GSE_TAG                  "LHJ [Gsensor] "

/*#define GSE_FUN(f)             printk(KERN_INFO GSE_TAG"%s\n", __FUNCTION__)
#define GSE_ERR(fmt, args...)    printk(KERN_ERR GSE_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define GSE_LOG(fmt, args...)    printk(KERN_INFO GSE_TAG fmt, ##args)*/

//#define GSE_DEBUG_MSG

#ifdef GSE_DEBUG_MSG
#define GSE_FUN(f)             printk(KERN_ERR GSE_TAG"%s\n", __FUNCTION__) // printk(KERN_INFO GSE_TAG"%s\n", __FUNCTION__)
#define GSE_ERR(fmt, args...)    printk(KERN_ERR GSE_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define GSE_LOG(fmt, args...)    printk(KERN_ERR GSE_TAG fmt, ##args)//printk(KERN_INFO GSE_TAG fmt, ##args)
#else
#define GSE_FUN(f)             do {} while (0)
#define GSE_ERR(fmt, args...)    do {} while (0)
#define GSE_LOG(fmt, args...)    do {} while (0)
#endif


/*----------------------------------------------------------------------------*/
static struct data_resolution lsm330_data_resolution[] = {
     /* combination by {FULL_RES,RANGE}*/
    {{ 1, 0}, 1024},   // dataformat +/-2g  in 12-bit resolution;  { 1, 0} = 1.0 = (2*2*1000)/(2^12);  1024 = (2^12)/(2*2) 
    {{ 1, 9}, 512},   // dataformat +/-4g  in 12-bit resolution;  { 1, 9} = 1.9 = (2*4*1000)/(2^12);  512 = (2^12)/(2*4) 
	{{ 2, 0}, 341},    // dataformat +/-6g  in 12-bit resolution 
	 {{ 3, 9}, 256},   // dataformat +/-8g  in 12-bit resolution;  { 1, 0} = 1.0 = (2*8*1000)/(2^12);  1024 = (2^12)/(2*8) 
};
/*----------------------------------------------------------------------------*/
static struct data_resolution lsm330_offset_resolution = {{15, 6}, 64};



static void LSM330_Power_GS_CS_Setting(unsigned int on);
static void LSM330_Power_GY_CS_Setting(unsigned int on);
static void LSM330_Power_GYDEN_Setting(unsigned int on);
#define LSM330_GPIO_DEFINED 0
static void LSM330_Power_GS_CS_Setting(unsigned int on)
{
#if LSM330_GPIO_DEFINED
	GSE_ERR("%s\n", on ? "on" : "off");
	if(1==on)
	{
		mt_set_gpio_mode(GPIO_GS_CS_PIN,GPIO_GS_CS_PIN_M_GPIO);
		mt_set_gpio_dir(GPIO_GS_CS_PIN,GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO_GS_CS_PIN,GPIO_OUT_ONE);
		mdelay(10);
		//LSM330_Power_GY_CS_Setting(0);
	}
	else
	{
		//LSM330_Power_GY_CS_Setting(1);
		mdelay(10);
		mt_set_gpio_mode(GPIO_GS_CS_PIN,GPIO_GS_CS_PIN_M_GPIO);
		mt_set_gpio_dir(GPIO_GS_CS_PIN,GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO_GS_CS_PIN,GPIO_OUT_ZERO);
	}
#endif
}

static void LSM330_Power_GY_CS_Setting(unsigned int on)
{
#if LSM330_GPIO_DEFINED
	GSE_ERR("%s\n", on ? "on" : "off");
	if(1==on)
	{
		mt_set_gpio_mode(GPIO_GY_CS_PIN,GPIO_GY_CS_PIN_M_GPIO);
		mt_set_gpio_dir(GPIO_GY_CS_PIN,GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO_GY_CS_PIN,GPIO_OUT_ONE);
	}
	else
	{
		mt_set_gpio_mode(GPIO_GY_CS_PIN,GPIO_GY_CS_PIN_M_GPIO);
		mt_set_gpio_dir(GPIO_GY_CS_PIN,GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO_GY_CS_PIN,GPIO_OUT_ZERO);
	}
#endif
}

static void LSM330_Power_GYDEN_Setting(unsigned int on)
{
	GSE_ERR("%s\n", on ? "on" : "off");
#if 0//LSM330_GPIO_DEFINED
	if(1==on)
	{
		mt_set_gpio_mode(GPIO_GYRO_EN_PIN,GPIO_GYRO_EN_PIN_M_GPIO);
		mt_set_gpio_dir(GPIO_GYRO_EN_PIN,GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO_GYRO_EN_PIN,GPIO_OUT_ONE);
		mdelay(10);
		mt_set_gpio_mode(GPIO_GS_CS_PIN,GPIO_GS_CS_PIN_M_GPIO);
		mt_set_gpio_dir(GPIO_GS_CS_PIN,GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO_GS_CS_PIN,GPIO_OUT_ONE);
		mdelay(10);
	}
	else
	{
		mt_set_gpio_mode(GPIO_GS_CS_PIN,GPIO_GS_CS_PIN_M_GPIO);
		mt_set_gpio_dir(GPIO_GS_CS_PIN,GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO_GS_CS_PIN,GPIO_OUT_ZERO);
		mdelay(10);
		mt_set_gpio_mode(GPIO_GYRO_EN_PIN,GPIO_GYRO_EN_PIN_M_GPIO);
		mt_set_gpio_dir(GPIO_GYRO_EN_PIN,GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO_GYRO_EN_PIN,GPIO_OUT_ZERO);
		mdelay(10);
	}
#endif
}

static void LSM330_Power_Gpio_Setting(unsigned int on)
{
	GSE_LOG("LHJ power %s\n", on ? "on" : "off");
#if LSM330_GPIO_DEFINED
	if(1 == on)
	{
		mt_set_gpio_mode(GPIO_GYRO_EN_PIN,GPIO_GYRO_EN_PIN_M_GPIO);
		mt_set_gpio_dir(GPIO_GYRO_EN_PIN,GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO_GYRO_EN_PIN,GPIO_OUT_ZERO);
		mdelay(10);
		
		//#if defined(GPIO_GY_CS_PIN)
		mt_set_gpio_mode(GPIO_GY_CS_PIN,GPIO_GY_CS_PIN_M_GPIO);
		mt_set_gpio_dir(GPIO_GY_CS_PIN,GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO_GY_CS_PIN,GPIO_OUT_ZERO);
		//#endif
		//#if defined(GPIO_GS_CS_PIN)
		mt_set_gpio_mode(GPIO_GS_CS_PIN,GPIO_GS_CS_PIN_M_GPIO);
		mt_set_gpio_dir(GPIO_GS_CS_PIN,GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO_GS_CS_PIN,GPIO_OUT_ONE);
		mdelay(10);
		//#endif
	}
	else
	{
		//#if defined(GPIO_GS_CS_PIN)
		mt_set_gpio_mode(GPIO_GS_CS_PIN,GPIO_GS_CS_PIN_M_GPIO);
		mt_set_gpio_dir(GPIO_GS_CS_PIN,GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO_GS_CS_PIN,GPIO_OUT_ZERO);
		mdelay(10);
		//#endif
		//#if defined(GPIO_GY_CS_PIN)
		mt_set_gpio_mode(GPIO_GY_CS_PIN,GPIO_GY_CS_PIN_M_GPIO);
		mt_set_gpio_dir(GPIO_GY_CS_PIN,GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO_GY_CS_PIN,GPIO_OUT_ONE);
		mdelay(10);
		//#endif

		mt_set_gpio_mode(GPIO_GYRO_EN_PIN,GPIO_GYRO_EN_PIN_M_GPIO);
		mt_set_gpio_dir(GPIO_GYRO_EN_PIN,GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO_GYRO_EN_PIN,GPIO_OUT_ONE);
		mdelay(10);
	}
#endif
}

/*
static int hwmsen_read_byte_sr(struct i2c_client *client, u8 addr, u8 *data)
{
   u8 buf;
    int ret = 0;
	
    client->addr = client->addr& I2C_MASK_FLAG | I2C_WR_FLAG |I2C_RS_FLAG;
    buf = addr;
	ret = i2c_master_send(client, (const char*)&buf, 1<<8 | 1);
    //ret = i2c_master_send(client, (const char*)&buf, 1);
    if (ret < 0) {
        GSE_ERR("send command error!!\n");
        return -EFAULT;
    }

    *data = buf;
	client->addr = client->addr& I2C_MASK_FLAG;
    return 0;
}
*/
/*--------------------read function----------------------------------*/
static int lis_i2c_read_block(struct i2c_client *client, u8 addr, u8 *data, u8 len)
{
        u8 beg = addr;
	struct i2c_msg msgs[2] = {
		{
			.addr = client->addr,	.flags = 0,
			.len = 1,	.buf = &beg
		},
		{
			.addr = client->addr,	.flags = I2C_M_RD,
			.len = len,	.buf = data,
		}
	};
	int err;

	if (!client)
		return -EINVAL;
	else if (len > C_I2C_FIFO_SIZE) {
		GSE_ERR(" length %d exceeds %d\n", len, C_I2C_FIFO_SIZE);
		return -EINVAL;
	}

	err = i2c_transfer(client->adapter, msgs, sizeof(msgs)/sizeof(msgs[0]));
	if (err != 2) {
		GSE_ERR("i2c_transfer error: (%d %p %d) %d\n",
			addr, data, len, err);
		err = -EIO;
	} else {
		err = 0;
	}
	return err;

}
/*--------------------read function----------------------------------*/
static void dumpReg(struct i2c_client *client)
{
  int i=0;
  u8 addr = 0x20;
  u8 regdata=0;
  for(i=0; i<3 ; i++)
  {
    //dump all
    lis_i2c_read_block(client,addr,&regdata,1);
	GSE_LOG("Reg addr=%x regdata=%x\n",addr,regdata);
	addr++;
  }
}

/*--------------------ADXL power control function----------------------------------*/
static void LSM330_power(struct acc_hw *hw, unsigned int on) 
{
	static unsigned int power_on = 0;
	GSE_ERR("%s\n", on ? "on" : "off");
	LSM330_Power_GYDEN_Setting(on);

	if(hw->power_id != POWER_NONE_MACRO)		// have externel LDO
	{        
		GSE_LOG("power %s\n", on ? "on" : "off");
		if(power_on == on)	// power status not change
		{
			GSE_LOG("ignore power control: %d\n", on);
		}
		else if(on)	// power on
		{
			if(!hwPowerOn(hw->power_id, hw->power_vol, "LSM330"))
			{
				GSE_ERR("power on fails!!\n");
			}
		}
		else	// power off
		{
			if (!hwPowerDown(hw->power_id, "LSM330"))
			{
				GSE_ERR("power off fail!!\n");
			}			  
		}
	}
	power_on = on;    
}
/*----------------------------------------------------------------------------*/
static int LSM330_SetDataResolution(struct lsm330_i2c_data *obj)
{
	int err;
	u8  dat, reso;

	if((err = lis_i2c_read_block(obj->client, LSM330_REG_CTL_REG6, &dat,0x01)))
	{
		GSE_ERR("write data format fail!!\n");
		return err;
	}
GSE_LOG("SetDataResolution from register is %x",dat);

	/*the data_reso is combined by 3 bits: {FULL_RES, DATA_RANGE}*/
	reso  = (dat & 0x38)>>3;
	if(reso >= 0x3)
		reso = 0x3;//do not use +/- 16g
	
GSE_LOG("after handle is %x",reso);

	if(reso < sizeof(lsm330_data_resolution)/sizeof(lsm330_data_resolution[0]))
	{        
		obj->reso = &lsm330_data_resolution[reso];
		return 0;
	}
	else
	{
		return -EINVAL;
	}
}
/*----------------------------------------------------------------------------*/
static int LSM330_ReadData(struct i2c_client *client, s16 data[LSM330_AXES_NUM])
{
	struct lsm330_i2c_data *priv = i2c_get_clientdata(client);        
	//u8 addr = LSM330_REG_DATAX0;
	u8 buf[LSM330_DATA_LEN] = {0};
	int err = 0;

	if(NULL == client)
	{
		err = -EINVAL;
	}
	
	else
	{
		if(lis_i2c_read_block(client, LSM330_REG_OUT_X, buf, 0x01))
	    {
		   GSE_ERR("read  G sensor data register err!\n");
		     return -1;
	    }
		if(lis_i2c_read_block(client, LSM330_REG_OUT_X+1, &buf[1], 0x01))
	    {
		   GSE_ERR("read  G sensor data register err!\n");
		     return -1;
	    }
		
	    data[LSM330_AXIS_X] = (s16)((buf[0]+(buf[1]<<8))>>4);
	if(lis_i2c_read_block(client, LSM330_REG_OUT_Y, &buf[2], 0x01))
	    {
		   GSE_ERR("read  G sensor data register err!\n");
		     return -1;
	    }
	if(lis_i2c_read_block(client, LSM330_REG_OUT_Y+1, &buf[3], 0x01))
	    {
		   GSE_ERR("read  G sensor data register err!\n");
		     return -1;
	    }
		
	    data[LSM330_AXIS_Y] =  (s16)((s16)(buf[2] +( buf[3]<<8))>>4);
		
	if(lis_i2c_read_block(client, LSM330_REG_OUT_Z, &buf[4], 0x01))
	    {
		   GSE_ERR("read  G sensor data register err!\n");
		     return -1;
	    }

	if(lis_i2c_read_block(client, LSM330_REG_OUT_Z+1, &buf[5], 0x01))
	    {
		   GSE_ERR("read  G sensor data register err!\n");
		     return -1;
	    }
		
	    data[LSM330_AXIS_Z] =(s16)((buf[4]+(buf[5]<<8))>>4);

	//GSE_LOG("[%08X %08X %08X %08x %08x %08x]\n",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);

	
		data[LSM330_AXIS_X] &= 0xfff;
		data[LSM330_AXIS_Y] &= 0xfff;
		data[LSM330_AXIS_Z] &= 0xfff;


		if(atomic_read(&priv->trace) & ADX_TRC_RAWDATA)
		{
			GSE_LOG("[%08X %08X %08X] => [%5d %5d %5d]\n", data[LSM330_AXIS_X], data[LSM330_AXIS_Y], data[LSM330_AXIS_Z],
		                               data[LSM330_AXIS_X], data[LSM330_AXIS_Y], data[LSM330_AXIS_Z]);
		}

		if(data[LSM330_AXIS_X]&0x800)
		{
				data[LSM330_AXIS_X] = ~data[LSM330_AXIS_X];
				data[LSM330_AXIS_X] &= 0xfff;
				data[LSM330_AXIS_X]+=1;
				data[LSM330_AXIS_X] = -data[LSM330_AXIS_X];
		}
		if(data[LSM330_AXIS_Y]&0x800)
		{
				data[LSM330_AXIS_Y] = ~data[LSM330_AXIS_Y];
				data[LSM330_AXIS_Y] &= 0xfff;
				data[LSM330_AXIS_Y]+=1;
				data[LSM330_AXIS_Y] = -data[LSM330_AXIS_Y];
		}
		if(data[LSM330_AXIS_Z]&0x800)
		{
				data[LSM330_AXIS_Z] = ~data[LSM330_AXIS_Z];
				data[LSM330_AXIS_Z] &= 0xfff;
				data[LSM330_AXIS_Z]+=1;
				data[LSM330_AXIS_Z] = -data[LSM330_AXIS_Z];
		}

		if(atomic_read(&priv->trace) & ADX_TRC_RAWDATA)
		{
			GSE_LOG("[%08X %08X %08X] => [%5d %5d %5d] after\n", data[LSM330_AXIS_X], data[LSM330_AXIS_Y], data[LSM330_AXIS_Z],
		                               data[LSM330_AXIS_X], data[LSM330_AXIS_Y], data[LSM330_AXIS_Z]);
		}
		GSE_LOG("[%08X %08X %08X] => [%5d %5d %5d] after\n", data[LSM330_AXIS_X], data[LSM330_AXIS_Y], data[LSM330_AXIS_Z],
		                               data[LSM330_AXIS_X], data[LSM330_AXIS_Y], data[LSM330_AXIS_Z]);
		
#ifdef CONFIG_LSM330_LOWPASS
		if(atomic_read(&priv->filter))
		{
			if(atomic_read(&priv->fir_en) && !atomic_read(&priv->suspend))
			{
				int idx, firlen = atomic_read(&priv->firlen);   
				if(priv->fir.num < firlen)
				{                
					priv->fir.raw[priv->fir.num][LSM330_AXIS_X] = data[LSM330_AXIS_X];
					priv->fir.raw[priv->fir.num][LSM330_AXIS_Y] = data[LSM330_AXIS_Y];
					priv->fir.raw[priv->fir.num][LSM330_AXIS_Z] = data[LSM330_AXIS_Z];
					priv->fir.sum[LSM330_AXIS_X] += data[LSM330_AXIS_X];
					priv->fir.sum[LSM330_AXIS_Y] += data[LSM330_AXIS_Y];
					priv->fir.sum[LSM330_AXIS_Z] += data[LSM330_AXIS_Z];
					if(atomic_read(&priv->trace) & ADX_TRC_FILTER)
					{
						GSE_LOG("add [%2d] [%5d %5d %5d] => [%5d %5d %5d]\n", priv->fir.num,
							priv->fir.raw[priv->fir.num][LSM330_AXIS_X], priv->fir.raw[priv->fir.num][LSM330_AXIS_Y], priv->fir.raw[priv->fir.num][LSM330_AXIS_Z],
							priv->fir.sum[LSM330_AXIS_X], priv->fir.sum[LSM330_AXIS_Y], priv->fir.sum[LSM330_AXIS_Z]);
					}
					priv->fir.num++;
					priv->fir.idx++;
				}
				else
				{
					idx = priv->fir.idx % firlen;
					priv->fir.sum[LSM330_AXIS_X] -= priv->fir.raw[idx][LSM330_AXIS_X];
					priv->fir.sum[LSM330_AXIS_Y] -= priv->fir.raw[idx][LSM330_AXIS_Y];
					priv->fir.sum[LSM330_AXIS_Z] -= priv->fir.raw[idx][LSM330_AXIS_Z];
					priv->fir.raw[idx][LSM330_AXIS_X] = data[LSM330_AXIS_X];
					priv->fir.raw[idx][LSM330_AXIS_Y] = data[LSM330_AXIS_Y];
					priv->fir.raw[idx][LSM330_AXIS_Z] = data[LSM330_AXIS_Z];
					priv->fir.sum[LSM330_AXIS_X] += data[LSM330_AXIS_X];
					priv->fir.sum[LSM330_AXIS_Y] += data[LSM330_AXIS_Y];
					priv->fir.sum[LSM330_AXIS_Z] += data[LSM330_AXIS_Z];
					priv->fir.idx++;
					data[LSM330_AXIS_X] = priv->fir.sum[LSM330_AXIS_X]/firlen;
					data[LSM330_AXIS_Y] = priv->fir.sum[LSM330_AXIS_Y]/firlen;
					data[LSM330_AXIS_Z] = priv->fir.sum[LSM330_AXIS_Z]/firlen;
					if(atomic_read(&priv->trace) & ADX_TRC_FILTER)
					{
						GSE_LOG("add [%2d] [%5d %5d %5d] => [%5d %5d %5d] : [%5d %5d %5d]\n", idx,
						priv->fir.raw[idx][LSM330_AXIS_X], priv->fir.raw[idx][LSM330_AXIS_Y], priv->fir.raw[idx][LSM330_AXIS_Z],
						priv->fir.sum[LSM330_AXIS_X], priv->fir.sum[LSM330_AXIS_Y], priv->fir.sum[LSM330_AXIS_Z],
						data[LSM330_AXIS_X], data[LSM330_AXIS_Y], data[LSM330_AXIS_Z]);
					}
				}
			}
		}	
#endif         
	}
	return err;
}
/*----------------------------------------------------------------------------*/
/*
static int LSM330_ReadOffset(struct i2c_client *client, s8 ofs[LSM330_AXES_NUM])
{    
	int err;

	return err;    
}
*/
/*----------------------------------------------------------------------------*/
static int LSM330_ResetCalibration(struct i2c_client *client)
{
	struct lsm330_i2c_data *obj = i2c_get_clientdata(client);	

	memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));
	return 0;     
}
/*----------------------------------------------------------------------------*/
static int LSM330_ReadCalibration(struct i2c_client *client, int dat[LSM330_AXES_NUM])
{
    struct lsm330_i2c_data *obj = i2c_get_clientdata(client);

    dat[obj->cvt.map[LSM330_AXIS_X]] = obj->cvt.sign[LSM330_AXIS_X]*obj->cali_sw[LSM330_AXIS_X];
    dat[obj->cvt.map[LSM330_AXIS_Y]] = obj->cvt.sign[LSM330_AXIS_Y]*obj->cali_sw[LSM330_AXIS_Y];
    dat[obj->cvt.map[LSM330_AXIS_Z]] = obj->cvt.sign[LSM330_AXIS_Z]*obj->cali_sw[LSM330_AXIS_Z];                        
                                       
    return 0;
}
/*----------------------------------------------------------------------------*/
/*
static int LSM330_ReadCalibrationEx(struct i2c_client *client, int act[LSM330_AXES_NUM], int raw[LSM330_AXES_NUM])
{  
	
	struct lsm330_i2c_data *obj = i2c_get_clientdata(client);
	int err;
	int mul;

	if(err = LSM330_ReadOffset(client, obj->offset))
	{
		GSE_ERR("read offset fail, %d\n", err);
		return err;
	}    

	mul = obj->reso->sensitivity/lsm330_offset_resolution.sensitivity;
	raw[LSM330_AXIS_X] = obj->offset[LSM330_AXIS_X]*mul + obj->cali_sw[LSM330_AXIS_X];
	raw[LSM330_AXIS_Y] = obj->offset[LSM330_AXIS_Y]*mul + obj->cali_sw[LSM330_AXIS_Y];
	raw[LSM330_AXIS_Z] = obj->offset[LSM330_AXIS_Z]*mul + obj->cali_sw[LSM330_AXIS_Z];

	act[obj->cvt.map[LSM330_AXIS_X]] = obj->cvt.sign[LSM330_AXIS_X]*raw[LSM330_AXIS_X];
	act[obj->cvt.map[LSM330_AXIS_Y]] = obj->cvt.sign[LSM330_AXIS_Y]*raw[LSM330_AXIS_Y];
	act[obj->cvt.map[LSM330_AXIS_Z]] = obj->cvt.sign[LSM330_AXIS_Z]*raw[LSM330_AXIS_Z];                        
	                       
	return 0;
}
*/
/*----------------------------------------------------------------------------*/
static int LSM330_WriteCalibration(struct i2c_client *client, int dat[LSM330_AXES_NUM])
{
	struct lsm330_i2c_data *obj = i2c_get_clientdata(client);
	int err = 0;
	//int cali[LSM330_AXES_NUM];


	GSE_FUN();
	if(!obj || ! dat)
	{
		GSE_ERR("null ptr!!\n");
		return -EINVAL;
	}
	else
	{        
		s16 cali[LSM330_AXES_NUM];
		cali[obj->cvt.map[LSM330_AXIS_X]] = obj->cvt.sign[LSM330_AXIS_X]*obj->cali_sw[LSM330_AXIS_X];
		cali[obj->cvt.map[LSM330_AXIS_Y]] = obj->cvt.sign[LSM330_AXIS_Y]*obj->cali_sw[LSM330_AXIS_Y];
		cali[obj->cvt.map[LSM330_AXIS_Z]] = obj->cvt.sign[LSM330_AXIS_Z]*obj->cali_sw[LSM330_AXIS_Z]; 
		cali[LSM330_AXIS_X] += dat[LSM330_AXIS_X];
		cali[LSM330_AXIS_Y] += dat[LSM330_AXIS_Y];
		cali[LSM330_AXIS_Z] += dat[LSM330_AXIS_Z];

		obj->cali_sw[LSM330_AXIS_X] += obj->cvt.sign[LSM330_AXIS_X]*dat[obj->cvt.map[LSM330_AXIS_X]];
        obj->cali_sw[LSM330_AXIS_Y] += obj->cvt.sign[LSM330_AXIS_Y]*dat[obj->cvt.map[LSM330_AXIS_Y]];
        obj->cali_sw[LSM330_AXIS_Z] += obj->cvt.sign[LSM330_AXIS_Z]*dat[obj->cvt.map[LSM330_AXIS_Z]];
	} 

	return err;
}
/*----------------------------------------------------------------------------*/
static int LSM330_CheckDeviceID(struct i2c_client *client)
{
	u8 databuf[2];    
	int res = 0;
	u8 addr = LSM330_REG_DEVID;
	memset(databuf, 0, sizeof(u8)*2);    
	databuf[0] = LSM330_REG_DEVID;  
	
	if(lis_i2c_read_block(client, addr, databuf, 0x01))
	{
		GSE_ERR("read DeviceID register err!\n");
		return LSM330_ERR_IDENTIFICATION;
		
	}
	GSE_LOG("LSM330 Device ID=0x%x,respect=0x%x\n",databuf[0],WHO_AM_I);
/*DO Not meet the datasheet!!! abort   */
/*
	if(databuf[0]!= WHO_AM_I)
	{
		return LSM330_ERR_IDENTIFICATION;
	}

	exit_LSM330_CheckDeviceID:
	if (res <= 0)
	{
		return LSM330_ERR_I2C;
	}
*/
	return LSM330_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int LSM330_SetBWRate(struct i2c_client *client, u8 bwrate)
{
	u8 databuf[10];
	u8 addr = LSM330_REG_CTL_REG5;
	int res = 0;

	memset(databuf, 0, sizeof(u8)*10);
	
	if(lis_i2c_read_block(client, addr, databuf, 0x01))
	{
		GSE_ERR("read reg_ctl_reg5 register err!\n");
		return LSM330_ERR_I2C;
	}
	GSE_LOG("LSM330_SetBWRate read from REG5 is 0x%x\n",databuf[0]);

	databuf[0] &= ~0xF0;
	databuf[0] |= bwrate;

	databuf[1] = databuf[0];
	databuf[0] = LSM330_REG_CTL_REG5;

	res = i2c_master_send(client, databuf, 0x2);
	GSE_LOG("LSM330_SetBWRate:write 0x%x to REG5",databuf[1]);

	if(res <= 0)
	{
		GSE_ERR("setBWRate failed!\n");
		return LSM330_ERR_I2C;
	}

	
	return LSM330_SUCCESS;    
}

/*----------------------------------------------------------------------------*/
static int LSM330_SetPowerMode(struct i2c_client *client, bool enable)
{
	u8 databuf[2];    
	int res = 0;
	u8 addr = LSM330_REG_CTL_REG5;
	struct lsm330_i2c_data *obj = i2c_get_clientdata(client);
	
	GSE_ERR("sensor_power = %d,enable=%d\n",sensor_power,enable);

	if(enable == sensor_power)
	{
		GSE_LOG("Sensor power status is newest!\n");
		return LSM330_SUCCESS;
	}


	if(enable == TRUE)
	{
		res = LSM330_SetBWRate(client, LSM330_BW_100HZ);//400 or 100 no other choice
	}
	else
	{
		if(lis_i2c_read_block(client, addr, databuf, 0x01))
		{
			GSE_ERR("read power ctl register err!\n");
			return LSM330_ERR_I2C;
		}
		GSE_LOG("LSM330_SetPowerMode read from REG5 is 0x%x\n",databuf[0]);
		//databuf[0] &= ~0xF0;
		//databuf[0] |= 0x00;

		//databuf[1] = databuf[0];
		databuf[1] = 0x07;
		databuf[0] = LSM330_REG_CTL_REG5;
		res = i2c_master_send(client, databuf, 0x2);
		GSE_LOG("SetPowerMode:write 0x%x to REG5 \n",databuf[1]);
		if(res <= 0)
		{
			GSE_ERR("set power mode failed!\n");
			return LSM330_ERR_I2C;
		}

	}
/*
	if(enable == TRUE)
	{
		databuf[0] &=  ~LSM330_MEASURE_MODE;
	}
	else
	{
		databuf[0] |= LSM330_MEASURE_MODE;
	}
	databuf[1] = databuf[0];
	databuf[0] = LSM330_REG_CTL_REG5;
	
	
	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		GSE_LOG("set power mode failed!\n");
		return LSM330_ERR_I2C;
	}
	else*/ if(atomic_read(&obj->trace) & ADX_TRC_INFO)
	{
		GSE_LOG("set power mode ok %d!\n", databuf[1]);
	}
	sensor_power = enable;

	GSE_LOG("leave SetPowerMode(),sensor_power = %d\n",sensor_power);
	return LSM330_SUCCESS;    
}
/*----------------------------------------------------------------------------*/
static int LSM330_SetDataFormat(struct i2c_client *client, u8 dataformat)
{
	struct lsm330_i2c_data *obj = i2c_get_clientdata(client);
	u8 databuf[10];
	u8 addr = LSM330_REG_CTL_REG6;
	int res = 0;

	memset(databuf, 0, sizeof(u8)*10);

	if(lis_i2c_read_block(client, addr, databuf, 0x01))
	{
		GSE_ERR("read reg_ctl_reg1 register err!\n");
		return LSM330_ERR_I2C;
	}

	databuf[0] &= ~0x38;
	databuf[0] |=dataformat;

	databuf[1] = databuf[0];
	databuf[0] = LSM330_REG_CTL_REG6;
	

	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		return LSM330_ERR_I2C;
	}
	

	return LSM330_SetDataResolution(obj);    
}
/*----------------------------------------------------------------------------*/
//enalbe data ready interrupt
static int LSM330_SetIntEnable(struct i2c_client *client, u8 intenable)
{
	u8 databuf[10];
	u8 addr = LSM330_REG_CTL_REG4;
	int res = 0;

	memset(databuf, 0, sizeof(u8)*10); 

	if(lis_i2c_read_block(client, addr, databuf, 0x01))
	{
		GSE_ERR("read reg_ctl_reg1 register err!\n");
		return LSM330_ERR_I2C;
	}

	databuf[0] = 0x00;//disable EINT
	databuf[1] = databuf[0];
	databuf[0] = LSM330_REG_CTL_REG4;
	
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		return LSM330_ERR_I2C;
	}
	
	return LSM330_SUCCESS;    
}
/*----------------------------------------------------------------------------*/
static int LSM330_Init(struct i2c_client *client, int reset_cali)
{
	struct lsm330_i2c_data *obj = i2c_get_clientdata(client);
	int res = 0;

	res = LSM330_CheckDeviceID(client); 
	if(res != LSM330_SUCCESS)
	{
		GSE_LOG("check device ID fail!!\n");
		return res;
	}	

    // first clear reg1
    res = hwmsen_write_byte(client,LSM330_REG_CTL_REG5,0x07);
	if(res != LSM330_SUCCESS)
	{
		return res;
	}

	res = LSM330_SetBWRate(client, LSM330_BW_100HZ);//400 or 100 no other choice
	if(res != LSM330_SUCCESS )
	{
		return res;
	}

	res = LSM330_SetDataFormat(client, LSM330_RANGE_2G);//8g or 2G no oher choise
	if(res != LSM330_SUCCESS) 
	{
		return res;
	}
	gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = obj->reso->sensitivity;

	res = LSM330_SetIntEnable(client, false);        
	if(res != LSM330_SUCCESS)
	{
		return res;
	}
	

	if(0 != reset_cali)
	{ 
		//reset calibration only in power on
		res = LSM330_ResetCalibration(client);
		if(res != LSM330_SUCCESS)
		{
			return res;
		}
	}

#ifdef CONFIG_LSM330_LOWPASS
	memset(&obj->fir, 0x00, sizeof(obj->fir));  
#endif

	return LSM330_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int LSM330_ReadChipInfo(struct i2c_client *client, char *buf, int bufsize)
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

	sprintf(buf, "LSM330 Chip");
	return 0;
}
/*----------------------------------------------------------------------------*/
static int LSM330_ReadSensorData(struct i2c_client *client, char *buf, int bufsize)
{
	struct lsm330_i2c_data *obj = (struct lsm330_i2c_data*)i2c_get_clientdata(client);
	u8 databuf[20];
	int acc[LSM330_AXES_NUM];
	int res = 0;
	memset(databuf, 0, sizeof(u8)*10);

	if(NULL == buf)
	{
		return -1;
	}
	if(NULL == client)
	{
		*buf = 0;
		return -2;
	}

	if(sensor_power == FALSE)
	{
		res = LSM330_SetPowerMode(client, true);
		if(res)
		{
			GSE_ERR("Power on lsm330 error %d!\n", res);
		}
		msleep(20);
	}

	if((res = LSM330_ReadData(client, obj->data)))
	{        
		GSE_ERR("I2C error: ret value=%d", res);
		return -3;
	}
	else
	{
		obj->data[LSM330_AXIS_X] += obj->cali_sw[LSM330_AXIS_X];
		obj->data[LSM330_AXIS_Y] += obj->cali_sw[LSM330_AXIS_Y];
		obj->data[LSM330_AXIS_Z] += obj->cali_sw[LSM330_AXIS_Z];
		
		/*remap coordinate*/
		acc[obj->cvt.map[LSM330_AXIS_X]] = obj->cvt.sign[LSM330_AXIS_X]*obj->data[LSM330_AXIS_X];
		acc[obj->cvt.map[LSM330_AXIS_Y]] = obj->cvt.sign[LSM330_AXIS_Y]*obj->data[LSM330_AXIS_Y];
		acc[obj->cvt.map[LSM330_AXIS_Z]] = obj->cvt.sign[LSM330_AXIS_Z]*obj->data[LSM330_AXIS_Z];

		//GSE_LOG("Mapped gsensor data: %d, %d, %d!\n", acc[LSM330_AXIS_X], acc[LSM330_AXIS_Y], acc[LSM330_AXIS_Z]);

		//Out put the mg
		acc[LSM330_AXIS_X] = acc[LSM330_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		acc[LSM330_AXIS_Y] = acc[LSM330_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		acc[LSM330_AXIS_Z] = acc[LSM330_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;		
		

		sprintf(buf, "%04x %04x %04x", acc[LSM330_AXIS_X], acc[LSM330_AXIS_Y], acc[LSM330_AXIS_Z]);
		if(atomic_read(&obj->trace) & ADX_TRC_IOCTL)//atomic_read(&obj->trace) & ADX_TRC_IOCTL
		{
			GSE_LOG("gsensor data: %s!\n", buf);
			dumpReg(client);
		}
	}
	
	return 0;
}
/*----------------------------------------------------------------------------*/
static int LSM330_ReadRawData(struct i2c_client *client, char *buf)
{
	struct lsm330_i2c_data *obj = (struct lsm330_i2c_data*)i2c_get_clientdata(client);
	int res = 0;

	if (!buf || !client)
	{
		return EINVAL;
	}
	
	if((res = LSM330_ReadData(client, obj->data)))
	{        
		GSE_ERR("I2C error: ret value=%d", res);
		return EIO;
	}
	else
	{
		sprintf(buf, "%04x %04x %04x", obj->data[LSM330_AXIS_X], 
			obj->data[LSM330_AXIS_Y], obj->data[LSM330_AXIS_Z]);
	
	}
	
	return 0;
}

/*----------------------------------------------------------------------------*/
static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = lsm330_i2c_client;
	char strbuf[LSM330_BUFSIZE];
	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	
	LSM330_ReadChipInfo(client, strbuf, LSM330_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);        
}
/*----------------------------------------------------------------------------*/
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = lsm330_i2c_client;
	char strbuf[LSM330_BUFSIZE];
	
	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	GSE_ERR("\n");
	LSM330_ReadSensorData(client, strbuf, LSM330_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);            
}
/*----------------------------------------------------------------------------*/
static ssize_t show_cali_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = lsm330_i2c_client;
	struct lsm330_i2c_data *obj;
	int err, len, mul;
	int tmp[LSM330_AXES_NUM];	
	len = 0;

	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	obj = i2c_get_clientdata(client);

	

	
	if((err = LSM330_ReadCalibration(client, tmp)))
	{
		return -EINVAL;
	}
	else
	{    
		mul = obj->reso->sensitivity/lsm330_offset_resolution.sensitivity;
		len += snprintf(buf+len, PAGE_SIZE-len, "[HW ][%d] (%+3d, %+3d, %+3d) : (0x%02X, 0x%02X, 0x%02X)\n", mul,                        
			obj->offset[LSM330_AXIS_X], obj->offset[LSM330_AXIS_Y], obj->offset[LSM330_AXIS_Z],
			obj->offset[LSM330_AXIS_X], obj->offset[LSM330_AXIS_Y], obj->offset[LSM330_AXIS_Z]);
		len += snprintf(buf+len, PAGE_SIZE-len, "[SW ][%d] (%+3d, %+3d, %+3d)\n", 1, 
			obj->cali_sw[LSM330_AXIS_X], obj->cali_sw[LSM330_AXIS_Y], obj->cali_sw[LSM330_AXIS_Z]);

		len += snprintf(buf+len, PAGE_SIZE-len, "[ALL]    (%+3d, %+3d, %+3d) : (%+3d, %+3d, %+3d)\n", 
			obj->offset[LSM330_AXIS_X]*mul + obj->cali_sw[LSM330_AXIS_X],
			obj->offset[LSM330_AXIS_Y]*mul + obj->cali_sw[LSM330_AXIS_Y],
			obj->offset[LSM330_AXIS_Z]*mul + obj->cali_sw[LSM330_AXIS_Z],
			tmp[LSM330_AXIS_X], tmp[LSM330_AXIS_Y], tmp[LSM330_AXIS_Z]);
		
		return len;
    }
}
/*----------------------------------------------------------------------------*/
static ssize_t store_cali_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct i2c_client *client = lsm330_i2c_client;  
	int err, x, y, z;
	int dat[LSM330_AXES_NUM];

	if(!strncmp(buf, "rst", 3))
	{
		if((err = LSM330_ResetCalibration(client)))
		{
			GSE_ERR("reset offset err = %d\n", err);
		}	
	}
	else if(3 == sscanf(buf, "0x%02X 0x%02X 0x%02X", &x, &y, &z))
	{
		dat[LSM330_AXIS_X] = x;
		dat[LSM330_AXIS_Y] = y;
		dat[LSM330_AXIS_Z] = z;
		if((err = LSM330_WriteCalibration(client, dat)))
		{
			GSE_ERR("write calibration err = %d\n", err);
		}		
	}
	else
	{
		GSE_ERR("invalid format\n");
	}
	
	return count;
}
/*----------------------------------------------------------------------------*/

static ssize_t show_power_status(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = lsm330_i2c_client;
	struct lsm330_i2c_data *obj;
	u8 data;

	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	obj = i2c_get_clientdata(client);
	lis_i2c_read_block(client,LSM330_REG_CTL_REG5,&data,0x01);
    	return snprintf(buf, PAGE_SIZE, "%x\n", data);
}

static ssize_t store_power_status(struct device_driver *ddri, const char *buf, size_t count)
{
	struct i2c_client *client = lsm330_i2c_client;  
	int err, x, y, z;
	u8 dat[2];


	dat[0] = LSM330_REG_CTL_REG5;

	if(1 == sscanf(buf, "0x%x", &dat[1]))
	{
		err = i2c_master_send(client, dat, 0x2);
	}	
	else
	{
		GSE_ERR("invalid content: '%s', length = %d\n", buf, count);
	}
	
	GSE_LOG("LSM330_SetBWRate:write 0x%x to REG5",dat[1]);
	
	return count;

}


/*----------------------------------------------------------------------------*/
static ssize_t show_firlen_value(struct device_driver *ddri, char *buf)
{
#ifdef CONFIG_LSM330_LOWPASS
	struct i2c_client *client = lsm330_i2c_client;
	struct lsm330_i2c_data *obj = i2c_get_clientdata(client);
	if(atomic_read(&obj->firlen))
	{
		int idx, len = atomic_read(&obj->firlen);
		GSE_LOG("len = %2d, idx = %2d\n", obj->fir.num, obj->fir.idx);

		for(idx = 0; idx < len; idx++)
		{
			GSE_LOG("[%5d %5d %5d]\n", obj->fir.raw[idx][LSM330_AXIS_X], obj->fir.raw[idx][LSM330_AXIS_Y], obj->fir.raw[idx][LSM330_AXIS_Z]);
		}
		
		GSE_LOG("sum = [%5d %5d %5d]\n", obj->fir.sum[LSM330_AXIS_X], obj->fir.sum[LSM330_AXIS_Y], obj->fir.sum[LSM330_AXIS_Z]);
		GSE_LOG("avg = [%5d %5d %5d]\n", obj->fir.sum[LSM330_AXIS_X]/len, obj->fir.sum[LSM330_AXIS_Y]/len, obj->fir.sum[LSM330_AXIS_Z]/len);
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&obj->firlen));
#else
	return snprintf(buf, PAGE_SIZE, "not support\n");
#endif
}
/*----------------------------------------------------------------------------*/
static ssize_t store_firlen_value(struct device_driver *ddri, const char *buf, size_t count)
{
#ifdef CONFIG_LSM330_LOWPASS
	struct i2c_client *client = lsm330_i2c_client;  
	struct lsm330_i2c_data *obj = i2c_get_clientdata(client);
	int firlen;

	if(1 != sscanf(buf, "%d", &firlen))
	{
		GSE_ERR("invallid format\n");
	}
	else if(firlen > C_MAX_FIR_LENGTH)
	{
		GSE_ERR("exceeds maximum filter length\n");
	}
	else
	{ 
		atomic_set(&obj->firlen, firlen);
		if(0 == firlen)
		{
			atomic_set(&obj->fir_en, 0);
		}
		else
		{
			memset(&obj->fir, 0x00, sizeof(obj->fir));
			atomic_set(&obj->fir_en, 1);
		}
	}
#endif    
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_trace_value(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	struct lsm330_i2c_data *obj = obj_i2c_data;
	if (obj == NULL)
	{
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}
	
	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));     
	return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct lsm330_i2c_data *obj = obj_i2c_data;
	int trace;
	if (obj == NULL)
	{
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}
	
	if(1 == sscanf(buf, "0x%x", &trace))
	{
		atomic_set(&obj->trace, trace);
	}	
	else
	{
		GSE_ERR("invalid content: '%s', length = %d\n", buf, count);
	}
	
	return count;    
}
/*----------------------------------------------------------------------------*/
static ssize_t show_status_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;    
	struct lsm330_i2c_data *obj = obj_i2c_data;
	if (obj == NULL)
	{
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}	
	
	if(obj->hw)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d %d (%d %d)\n", 
	            obj->hw->i2c_num, obj->hw->direction, obj->hw->power_id, obj->hw->power_vol);   
	}
	else
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");
	}
	return len;    
}
static ssize_t show_acc_data(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = lsm330_i2c_client;
	char strbuf[LSM330_BUFSIZE];
	int x,y,z;
	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	GSE_ERR("\n");
	LSM330_ReadSensorData(client, strbuf, LSM330_BUFSIZE);
	sscanf(strbuf, "%x %x %x", &x, &y,&z);
	
	return snprintf(buf, PAGE_SIZE, "%d %d %d\n", x,y,z);            

}
/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(chipinfo,             S_IRUGO, show_chipinfo_value,      NULL);
static DRIVER_ATTR(sensordata,           S_IRUGO, show_sensordata_value,    NULL);
static DRIVER_ATTR(cali,       S_IWUSR | S_IRUGO, show_cali_value,          store_cali_value);
static DRIVER_ATTR(power,      S_IWUSR | S_IRUGO, show_power_status,          store_power_status);
static DRIVER_ATTR(firlen,     S_IWUSR | S_IRUGO, show_firlen_value,        store_firlen_value);
static DRIVER_ATTR(trace,      S_IWUSR | S_IRUGO, show_trace_value,         store_trace_value);
static DRIVER_ATTR(status,               S_IRUGO, show_status_value,        NULL);
static DRIVER_ATTR(accdata,               S_IRUGO, show_acc_data,        NULL);
/*----------------------------------------------------------------------------*/
static struct driver_attribute *lsm330_attr_list[] = {
	&driver_attr_chipinfo,     /*chip information*/
	&driver_attr_sensordata,   /*dump sensor data*/
	&driver_attr_cali,         /*show calibration data*/
	&driver_attr_power,         /*show power reg*/
	&driver_attr_firlen,       /*filter length: 0: disable, others: enable*/
	&driver_attr_trace,        /*trace log*/
	&driver_attr_status,        

	&driver_attr_accdata,

};
/*----------------------------------------------------------------------------*/
static int lsm330_create_attr(struct device_driver *driver) 
{
	int idx, err = 0;
	int num = (int)(sizeof(lsm330_attr_list)/sizeof(lsm330_attr_list[0]));
	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if((err = driver_create_file(driver, lsm330_attr_list[idx])))
		{            
			GSE_ERR("driver_create_file (%s) = %d\n", lsm330_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}
/*----------------------------------------------------------------------------*/
static int lsm330_delete_attr(struct device_driver *driver)
{
	int idx ,err = 0;
	int num = (int)(sizeof(lsm330_attr_list)/sizeof(lsm330_attr_list[0]));

	if(driver == NULL)
	{
		return -EINVAL;
	}
	

	for(idx = 0; idx < num; idx++)
	{
		driver_remove_file(driver, lsm330_attr_list[idx]);
	}
	

	return err;
}

/*----------------------------------------------------------------------------*/
int gsensor_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value, sample_delay;	
	struct lsm330_i2c_data *priv = (struct lsm330_i2c_data*)self;
	hwm_sensor_data* gsensor_data;
	char buff[LSM330_BUFSIZE];
	
	//GSE_FUN(f);
	switch (command)
	{
		case SENSOR_DELAY:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				GSE_ERR("Set delay parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				value = *(int *)buff_in;
				if(value <= 5)
				{
					sample_delay = LSM330_BW_400HZ;
				}
				else if(value <= 10)
				{
					sample_delay = LSM330_BW_100HZ;
				}
				else
				{
					sample_delay = LSM330_BW_50HZ;
				}
				
				err = LSM330_SetBWRate(priv->client, sample_delay);
				if(err != LSM330_SUCCESS ) //0x2C->BW=100Hz
				{
					GSE_ERR("Set delay parameter error!\n");
				}

				if(value >= 50)
				{
					atomic_set(&priv->filter, 0);
				}
				else
				{					
					priv->fir.num = 0;
					priv->fir.idx = 0;
					priv->fir.sum[LSM330_AXIS_X] = 0;
					priv->fir.sum[LSM330_AXIS_Y] = 0;
					priv->fir.sum[LSM330_AXIS_Z] = 0;
					atomic_set(&priv->filter, 1);
				}
			}
			break;

		case SENSOR_ENABLE:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				GSE_ERR("Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{
			    
				value = *(int *)buff_in;
				GSE_LOG("enable value=%d, sensor_power =%d\n",value,sensor_power);
				if(((value == 0) && (sensor_power == false)) ||((value == 1) && (sensor_power == true)))
				{
					GSE_LOG("Gsensor device have updated!\n");
				}
				else
				{
					err = LSM330_SetPowerMode( priv->client, !sensor_power);
					}
			}
			break;

		case SENSOR_GET_DATA:
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				GSE_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				gsensor_data = (hwm_sensor_data *)buff_out;
				LSM330_ReadSensorData(priv->client, buff, LSM330_BUFSIZE);
				sscanf(buff, "%x %x %x", &gsensor_data->values[0], 
					&gsensor_data->values[1], &gsensor_data->values[2]);				
				gsensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;				
				gsensor_data->value_divide = 1000;
				
			}
			break;
		default:
			GSE_ERR("gsensor operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}
	
	return err;
}

/****************************************************************************** 
 * Function Configuration
******************************************************************************/
static int lsm330_open(struct inode *inode, struct file *file)
{
	file->private_data = lsm330_i2c_client;

	if(file->private_data == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int lsm330_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}
/*----------------------------------------------------------------------------*/
//static int lsm330_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
//       unsigned long arg)
static long lsm330_unlocked_ioctl(struct file *file, unsigned int cmd,
       unsigned long arg)

{
	struct i2c_client *client = (struct i2c_client*)file->private_data;
	struct lsm330_i2c_data *obj = (struct lsm330_i2c_data*)i2c_get_clientdata(client);	
	char strbuf[LSM330_BUFSIZE];
	void __user *data;
	SENSOR_DATA sensor_data;
	long err = 0;
	int cali[3];

	//GSE_FUN(f);
	if(_IOC_DIR(cmd) & _IOC_READ)
	{
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	}
	else if(_IOC_DIR(cmd) & _IOC_WRITE)
	{
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	}

	if(err)
	{
		GSE_ERR("access error: %08X, (%2d, %2d)\n", cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
		return -EFAULT;
	}

	switch(cmd)
	{
		case GSENSOR_IOCTL_INIT:
			LSM330_Init(client, 0);			
			break;

		case GSENSOR_IOCTL_READ_CHIPINFO:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			
			LSM330_ReadChipInfo(client, strbuf, LSM330_BUFSIZE);
			if(copy_to_user(data, strbuf, strlen(strbuf)+1))
			{
				err = -EFAULT;
				break;
			}				 
			break;	  

		case GSENSOR_IOCTL_READ_SENSORDATA:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			LSM330_SetPowerMode(client,true);
			LSM330_ReadSensorData(client, strbuf, LSM330_BUFSIZE);
			if(copy_to_user(data, strbuf, strlen(strbuf)+1))
			{
				err = -EFAULT;
				break;	  
			}				 
			break;

		case GSENSOR_IOCTL_READ_GAIN:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}			
			
			if(copy_to_user(data, &gsensor_gain, sizeof(GSENSOR_VECTOR3D)))
			{
				err = -EFAULT;
				break;
			}				 
			break;

		case GSENSOR_IOCTL_READ_OFFSET:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			
			if(copy_to_user(data, &gsensor_offset, sizeof(GSENSOR_VECTOR3D)))
			{
				err = -EFAULT;
				break;
			}				 
			break;

		case GSENSOR_IOCTL_READ_RAW_DATA:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			LSM330_ReadRawData(client, strbuf);
			if(copy_to_user(data, &strbuf, strlen(strbuf)+1))
			{
				err = -EFAULT;
				break;	  
			}
			break;	  

		case GSENSOR_IOCTL_SET_CALI:
			data = (void __user*)arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			if(copy_from_user(&sensor_data, data, sizeof(sensor_data)))
			{
				err = -EFAULT;
				break;	  
			}
			if(atomic_read(&obj->suspend))
			{
				GSE_ERR("Perform calibration in suspend state!!\n");
				err = -EINVAL;
			}
			else
			{
				cali[LSM330_AXIS_X] = sensor_data.x * obj->reso->sensitivity / GRAVITY_EARTH_1000;
				cali[LSM330_AXIS_Y] = sensor_data.y * obj->reso->sensitivity / GRAVITY_EARTH_1000;
				cali[LSM330_AXIS_Z] = sensor_data.z * obj->reso->sensitivity / GRAVITY_EARTH_1000;			  
				err = LSM330_WriteCalibration(client, cali);			 
			}
			break;

		case GSENSOR_IOCTL_CLR_CALI:
			err = LSM330_ResetCalibration(client);
			break;

		case GSENSOR_IOCTL_GET_CALI:
			data = (void __user*)arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			if((err = LSM330_ReadCalibration(client, cali)))
			{
				break;
			}
			
			sensor_data.x = cali[LSM330_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
			sensor_data.y = cali[LSM330_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
			sensor_data.z = cali[LSM330_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
			if(copy_to_user(data, &sensor_data, sizeof(sensor_data)))
			{
				err = -EFAULT;
				break;
			}		
			break;
		

		default:
			GSE_ERR("unknown IOCTL: 0x%08x\n", cmd);
			err = -ENOIOCTLCMD;
			break;
			
	}

	return err;
}


/*----------------------------------------------------------------------------*/
static struct file_operations lsm330_fops = {
	.owner = THIS_MODULE,
	.open = lsm330_open,
	.release = lsm330_release,
	//.ioctl = lsm330_ioctl,
	.unlocked_ioctl = lsm330_unlocked_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice lsm330_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gsensor",
	.fops = &lsm330_fops,
};
/*----------------------------------------------------------------------------*/
#ifndef CONFIG_HAS_EARLYSUSPEND
/*----------------------------------------------------------------------------*/
static int lsm330_suspend(struct i2c_client *client, pm_message_t msg) 
{
	struct lsm330_i2c_data *obj = i2c_get_clientdata(client);    
	int err = 0;
	u8 dat;
	GSE_FUN();    

	if(msg.event == PM_EVENT_SUSPEND)
	{   
		if(obj == NULL)
		{
			GSE_ERR("null pointer!!\n");
			return -EINVAL;
		}
		//read old data
		if ((err = lis_i2c_read_block(client, LSM330_REG_CTL_REG1, &dat, 0x01))) 
		{
           GSE_ERR("write data format fail!!\n");
           return err;
        }
		dat = dat&0b10111111;
		atomic_set(&obj->suspend, 1);
		if(err = hwmsen_write_byte(client, LSM330_REG_CTL_REG1, dat))
		{
			GSE_ERR("write power control fail!!\n");
			return err;
		}        
		LSM330_power(obj->hw, 0);
	}
	return err;
}
/*----------------------------------------------------------------------------*/
static int lsm330_resume(struct i2c_client *client)
{
	struct lsm330_i2c_data *obj = i2c_get_clientdata(client);        
	int err;
	GSE_FUN();

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}

	LSM330_power(obj->hw, 1);
	if(err = LSM330_Init(client, 0))
	{
		GSE_ERR("initialize client fail!!\n");
		return err;        
	}
	atomic_set(&obj->suspend, 0);

	return 0;
}
/*----------------------------------------------------------------------------*/
#else /*CONFIG_HAS_EARLY_SUSPEND is defined*/
/*----------------------------------------------------------------------------*/

static void lsm330_early_suspend(struct early_suspend *h) 
{
	struct lsm330_i2c_data *obj = container_of(h, struct lsm330_i2c_data, early_drv);   
	u8 databuf[2]; 
	int err = 0;
	u8 addr = LSM330_REG_CTL_REG5;

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return;
	}

	atomic_set(&obj->suspend, 1); 
	/*
	GSE_FUN(); 
	*/
	if((err = LSM330_SetPowerMode(obj->client, false)))
	{
		GSE_ERR("write power control fail!!\n");
		
		return;        
	}
	sensor_power = false;
	
	LSM330_power(obj->hw, 0);
}
/*----------------------------------------------------------------------------*/
static void lsm330_late_resume(struct early_suspend *h)
{
	struct lsm330_i2c_data *obj = container_of(h, struct lsm330_i2c_data, early_drv);         
	int err;

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return;
	}

	LSM330_power(obj->hw, 1);
	if((err = LSM330_Init(obj->client, 0)))
	{
		GSE_ERR("initialize client fail!!\n");
		return;        
	}
	atomic_set(&obj->suspend, 0);    
}
/*----------------------------------------------------------------------------*/
#endif /*CONFIG_HAS_EARLYSUSPEND*/
/*----------------------------------------------------------------------------*/
static int lsm330_i2c_detect(struct i2c_client *client, struct i2c_board_info *info) 
{    
	strcpy(info->type, LSM330_DEV_NAME);
	return 0;
}

/*----------------------------------------------------------------------------*/
static int lsm330_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_client *new_client;
	struct lsm330_i2c_data *obj;
	struct hwmsen_object sobj;
	int err = 0;
	GSE_FUN();

	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}
	
	memset(obj, 0, sizeof(struct lsm330_i2c_data));

	obj->hw = get_cust_acc_hw();
	
	if((err = hwmsen_get_convert(obj->hw->direction, &obj->cvt)))
	{
		GSE_ERR("invalid direction: %d\n", obj->hw->direction);
		goto exit;
	}

	obj_i2c_data = obj;
	obj->client = client;
	new_client = obj->client;
	i2c_set_clientdata(new_client,obj);
	
	atomic_set(&obj->trace, 0);
	atomic_set(&obj->suspend, 0);
	
#ifdef CONFIG_LSM330_LOWPASS
	if(obj->hw->firlen > C_MAX_FIR_LENGTH)
	{
		atomic_set(&obj->firlen, C_MAX_FIR_LENGTH);
	}	
	else
	{
		atomic_set(&obj->firlen, obj->hw->firlen);
	}
	
	if(atomic_read(&obj->firlen) > 0)
	{
		atomic_set(&obj->fir_en, 1);
	}
	
#endif

	lsm330_i2c_client = new_client;	

	if((err = LSM330_Init(new_client, 1)))
	{
		goto exit_init_failed;
	}
	

	if((err = misc_register(&lsm330_device)))
	{
		GSE_ERR("lsm330_device register failed\n");
		goto exit_misc_device_register_failed;
	}

	if((err = lsm330_create_attr(&lsm330_gsensor_driver.driver)))
	{
		GSE_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}

	sobj.self = obj;
    sobj.polling = 1;
    sobj.sensor_operate = gsensor_operate;
	if((err = hwmsen_attach(ID_ACCELEROMETER, &sobj)))
	{
		GSE_ERR("attach fail = %d\n", err);
		goto exit_kfree;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	obj->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
	obj->early_drv.suspend  = lsm330_early_suspend,
	obj->early_drv.resume   = lsm330_late_resume,    
	register_early_suspend(&obj->early_drv);
#endif 

	GSE_LOG("%s: OK\n", __func__);    
	return 0;

	exit_create_attr_failed:
	misc_deregister(&lsm330_device);
	exit_misc_device_register_failed:
	exit_init_failed:
	//i2c_detach_client(new_client);
	exit_kfree:
	kfree(obj);
	exit:
	GSE_ERR("%s: err = %d\n", __func__, err);        
	return err;
}

/*----------------------------------------------------------------------------*/
static int lsm330_i2c_remove(struct i2c_client *client)
{
	int err = 0;	
	
	if((err = lsm330_delete_attr(&lsm330_gsensor_driver.driver)))
	{
		GSE_ERR("lsm330_delete_attr fail: %d\n", err);
	}
	
	if((err = misc_deregister(&lsm330_device)))
	{
		GSE_ERR("misc_deregister fail: %d\n", err);
	}

	if((err = hwmsen_detach(ID_ACCELEROMETER)))
	    

	lsm330_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));
	return 0;
}
/*----------------------------------------------------------------------------*/
static int lsm330_probe(struct platform_device *pdev) 
{
	struct acc_hw *hw = get_cust_acc_hw();
	GSE_FUN();

	LSM330_power(hw, 1);
	//lsm330_force[0] = hw->i2c_num;
	if(i2c_add_driver(&lsm330_i2c_driver))
	{
		GSE_ERR("add driver error\n");
		return -1;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static int lsm330_remove(struct platform_device *pdev)
{
    struct acc_hw *hw = get_cust_acc_hw();

    GSE_FUN();    
    LSM330_power(hw, 0);    
    i2c_del_driver(&lsm330_i2c_driver);
    return 0;
}
/*----------------------------------------------------------------------------*/
static struct platform_driver lsm330_gsensor_driver = {
	.probe      = lsm330_probe,
	.remove     = lsm330_remove,    
	.driver     = {
		.name  = "gsensor",
		.owner = THIS_MODULE,
	}
};

/*----------------------------------------------------------------------------*/
static int __init lsm330_init(void)
{
	GSE_FUN();//jy
	struct acc_hw *hw = get_cust_acc_hw();
	GSE_LOG("%s: i2c_number=%d\n", __func__,hw->i2c_num); 
	i2c_register_board_info(hw->i2c_num, &i2c_LSM330, 1);
	if(platform_driver_register(&lsm330_gsensor_driver))
	{
		GSE_ERR("failed to register driver");
		return -ENODEV;
	}
	return 0;    
}
/*----------------------------------------------------------------------------*/
static void __exit lsm330_exit(void)
{
	GSE_FUN();
	platform_driver_unregister(&lsm330_gsensor_driver);
}
/*----------------------------------------------------------------------------*/
module_init(lsm330_init);
module_exit(lsm330_exit);
/*----------------------------------------------------------------------------*/
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("LSM330 I2C driver");
MODULE_AUTHOR("Chunlei.Wang@mediatek.com");
