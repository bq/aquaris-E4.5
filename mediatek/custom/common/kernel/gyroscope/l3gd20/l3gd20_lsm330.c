/* L3GD20 motion sensor driver
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

#include <cust_gyro.h>
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include "l3gd20_lsm330.h"
#include <linux/hwmsen_helper.h>
#include <linux/kernel.h>

#include <linux/time.h>
#include <linux/timex.h>

#if 1
#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>
#include <mach/mt_boot.h>

/*-------------------------MT6516&MT6573 define-------------------------------*/

#define POWER_NONE_MACRO MT65XX_POWER_NONE
#endif

/*
#if 0
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

#ifdef MT6575
#include <mach/mt6575_devs.h>
#include <mach/mt6575_typedefs.h>
#include <mach/mt6575_gpio.h>
#include <mach/mt6575_pm_ldo.h>
#include <mach/mt6575_boot.h>
#endif

#ifdef MT6577
#include <mach/mt6577_devs.h>
#include <mach/mt6577_typedefs.h>
#include <mach/mt6577_gpio.h>
#include <mach/mt6577_pm_ldo.h>
#include <mach/mt6577_boot.h>
#endif

#ifdef MT6516
#define POWER_NONE_MACRO MT6516_POWER_NONE
#endif

#ifdef MT6573
#define POWER_NONE_MACRO MT65XX_POWER_NONE
#endif

#ifdef MT6575
#define POWER_NONE_MACRO MT65XX_POWER_NONE
#endif

#ifdef MT6577
#define POWER_NONE_MACRO MT65XX_POWER_NONE
#endif


#endif

*/
/*----------------------------------------------------------------------------*/
#define I2C_DRIVERID_L3GD20	3000
/*----------------------------------------------------------------------------*/
//#define L3GD20_DEFAULT_FS		L3GD20_FS_1000
//#define L3GD20_DEFAULT_LSB		L3GD20_FS_250_LSB
/*---------------------------------------------------------------------------*/
#define DEBUG 0
/*----------------------------------------------------------------------------*/
#define CONFIG_L3GD20_LOWPASS   /*apply low pass filter on output*/
/*----------------------------------------------------------------------------*/
#define L3GD20_AXIS_X          0
#define L3GD20_AXIS_Y          1
#define L3GD20_AXIS_Z          2
#define L3GD20_AXES_NUM        3
#define L3GD20_DATA_LEN        6
#define L3GD20_DEV_NAME        "L3GD20"
/*----------------------------------------------------------------------------*/


struct lsm330_gyro_data {
    rwlock_t datalock;

    int yaw;
    int roll;
    int pitch;

    int Gyx;
    int Gyy;
    int Gyz;

    int gyro_data_buff[L3GD20_BUFSIZE];

    int Grx;
    int Gry;
    int Grz;
    int LAx;
    int LAy;
    int LAz;
    int RVx;
    int RVy;
    int RVz;
	int Scalar;
}lsm330_gy_data;

static DECLARE_WAIT_QUEUE_HEAD(open_wq);
static atomic_t open_flag = ATOMIC_INIT(0);
static atomic_t gy_flag = ATOMIC_INIT(0);
static atomic_t or_flag = ATOMIC_INIT(0);
static atomic_t gr_flag = ATOMIC_INIT(0);
static atomic_t rv_flag = ATOMIC_INIT(0);
static atomic_t la_flag = ATOMIC_INIT(0);

static int delay = 10;
// 苏 勇 2014年06月27日 10:52:26static int gyro_suspend =0;
static atomic_t gyro_suspend = ATOMIC_INIT(0);

static const struct i2c_device_id l3gd20_i2c_id[] = {{L3GD20_DEV_NAME,0},{}};
static struct i2c_board_info __initdata i2c_l3gd20={ I2C_BOARD_INFO(L3GD20_DEV_NAME, (L3GD20_I2C_SLAVE_ADDR>>1))};
/*the adapter id will be available in customization*/
//static unsigned short l3gd20_force[] = {0x00, L3GD20_I2C_SLAVE_ADDR, I2C_CLIENT_END, I2C_CLIENT_END};
//static const unsigned short *const l3gd20_forces[] = { l3gd20_force, NULL };
//static struct i2c_client_address_data l3gd20_addr_data = { .forces = l3gd20_forces,};

int packet_thresh = 75; // 600 ms / 8ms/sample

/*----------------------------------------------------------------------------*/
static int l3gd20_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int l3gd20_i2c_remove(struct i2c_client *client);
static int l3gd20_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
static int l3gd20_init_client(struct i2c_client *client, bool enable);

/*----------------------------------------------------------------------------*/
typedef enum {
    GYRO_TRC_FILTER  = 0x01,
    GYRO_TRC_RAWDATA = 0x02,
    GYRO_TRC_IOCTL   = 0x04,
    GYRO_TRC_CALI	= 0X08,
    GYRO_TRC_INFO	= 0X10,
    GYRO_TRC_DATA	= 0X20,
} GYRO_TRC;
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
    s16 raw[C_MAX_FIR_LENGTH][L3GD20_AXES_NUM];
    int sum[L3GD20_AXES_NUM];
    int num;
    int idx;
};
/*----------------------------------------------------------------------------*/
struct l3gd20_i2c_data {
    struct i2c_client *client;
    struct gyro_hw *hw;
    struct hwmsen_convert   cvt;

    /*misc*/
    struct data_resolution *reso;
    atomic_t                trace;
    atomic_t                suspend;
    atomic_t                selftest;
    atomic_t				filter;
    s16                     cali_sw[L3GD20_AXES_NUM+1];
	s16                     dyna_cali_sw[L3GD20_AXES_NUM+1];

    /*data*/
    s8                      offset[L3GD20_AXES_NUM+1];  /*+1: for 4-byte alignment*/
    s16                     data[L3GD20_AXES_NUM+1];

#if defined(CONFIG_L3GD20_LOWPASS)
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
static struct i2c_driver l3gd20_i2c_driver = {
    .driver = {
//      .owner          = THIS_MODULE,
        .name           = L3GD20_DEV_NAME,
    },
	.probe      		= l3gd20_i2c_probe,
	.remove    			= l3gd20_i2c_remove,
	.detect				= l3gd20_i2c_detect,
#if !defined(CONFIG_HAS_EARLYSUSPEND)
    .suspend            = l3gd20_suspend,
    .resume             = l3gd20_resume,
#endif
	.id_table = l3gd20_i2c_id,
//	.address_data = &l3gd20_addr_data,
};

/*----------------------------------------------------------------------------*/
static struct i2c_client *l3gd20_i2c_client = NULL;
static struct platform_driver l3gd20_gyro_driver;
static struct l3gd20_i2c_data *obj_i2c_data = NULL;
static bool sensor_power = false;



/*----------------------------------------------------------------------------*/
#define GYRO_TAG                  "[Gyroscope] "
//#define GYRO_FUN(f)               printk(KERN_INFO GYRO_TAG"%s\n", __FUNCTION__)
//#define GYRO_ERR(fmt, args...)    printk(KERN_ERR GYRO_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)

//#define GYRO_LOG(fmt, args...)    printk(KERN_INFO GYRO_TAG fmt, ##args)

#define GYRO_FUN(f)               printk(GYRO_TAG"%s\n", __FUNCTION__)
#define GYRO_ERR(fmt, args...)    printk(KERN_ERR GYRO_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define GYRO_LOG(fmt, args...)    printk(GYRO_TAG fmt, ##args)

/*----------------------------------------------------------------------------*/

void getTimestamp_l3gd20(long long *sec,long long *usec) {
    struct timeval tv;

    do_gettimeofday(&tv);

    *usec = tv.tv_usec;

    *sec  = tv.tv_sec;


}

/*----------------------------------------------------------------------------*/

static void L3GD20_dumpReg(struct i2c_client *client)
{
  int i=0;
  u8 addr = 0x20;
  u8 regdata=0;
  for(i=0; i<25 ; i++)
  {
    //dump all
    hwmsen_read_byte(client,addr,&regdata);
	HWM_LOG("Reg addr=%x regdata=%x\n",addr,regdata);
	addr++;

  }

}


/*--------------------gyroscopy power control function----------------------------------*/
static void L3GD20_power(struct gyro_hw *hw, unsigned int on)
{
	static unsigned int power_on = 0;

	if(hw->power_id != POWER_NONE_MACRO)		// have externel LDO
	{
		GYRO_LOG("power %s\n", on ? "on" : "off");
		if(power_on == on)	// power status not change
		{
			GYRO_LOG("ignore power control: %d\n", on);
		}
		else if(on)	// power on
		{
			if(!hwPowerOn(hw->power_id, hw->power_vol, "L3GD20"))
			{
				GYRO_ERR("power on fails!!\n");
			}
		}
		else	// power off
		{
			if (!hwPowerDown(hw->power_id, "L3GD20"))
			{
				GYRO_ERR("power off fail!!\n");
			}
		}
	}
	power_on = on;
}
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
static int L3GD20_write_rel_calibration(struct l3gd20_i2c_data *obj, int dat[L3GD20_AXES_NUM])
{
    obj->cali_sw[L3GD20_AXIS_X] = obj->cvt.sign[L3GD20_AXIS_X]*dat[obj->cvt.map[L3GD20_AXIS_X]];
    obj->cali_sw[L3GD20_AXIS_Y] = obj->cvt.sign[L3GD20_AXIS_Y]*dat[obj->cvt.map[L3GD20_AXIS_Y]];
    obj->cali_sw[L3GD20_AXIS_Z] = obj->cvt.sign[L3GD20_AXIS_Z]*dat[obj->cvt.map[L3GD20_AXIS_Z]];
#if DEBUG
		if(atomic_read(&obj->trace) & GYRO_TRC_CALI)
		{
			GYRO_LOG("test  (%5d, %5d, %5d) ->(%5d, %5d, %5d)->(%5d, %5d, %5d))\n",
				obj->cvt.sign[L3GD20_AXIS_X],obj->cvt.sign[L3GD20_AXIS_Y],obj->cvt.sign[L3GD20_AXIS_Z],
				dat[L3GD20_AXIS_X], dat[L3GD20_AXIS_Y], dat[L3GD20_AXIS_Z],
				obj->cvt.map[L3GD20_AXIS_X],obj->cvt.map[L3GD20_AXIS_Y],obj->cvt.map[L3GD20_AXIS_Z]);
			GYRO_LOG("write gyro calibration data  (%5d, %5d, %5d)\n",
				obj->cali_sw[L3GD20_AXIS_X],obj->cali_sw[L3GD20_AXIS_Y],obj->cali_sw[L3GD20_AXIS_Z]);
		}
#endif
    return 0;
}


/*----------------------------------------------------------------------------*/
static int L3GD20_ResetCalibration(struct i2c_client *client)
{
	struct l3gd20_i2c_data *obj = i2c_get_clientdata(client);

	memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));
	return 0;
}

static int L3GD20_ResetDynaCalibration(struct i2c_client *client)
{
	struct l3gd20_i2c_data *obj = i2c_get_clientdata(client);

	memset(obj->dyna_cali_sw, 0x00, sizeof(obj->dyna_cali_sw));
	return 0;
}
	
/*----------------------------------------------------------------------------*/
static int L3GD20_ReadCalibration(struct i2c_client *client, int dat[L3GD20_AXES_NUM])
{
    struct l3gd20_i2c_data *obj = i2c_get_clientdata(client);
    int mul = 1;
    dat[obj->cvt.map[L3GD20_AXIS_X]] = obj->cvt.sign[L3GD20_AXIS_X]*obj->cali_sw[L3GD20_AXIS_X] * mul;
    dat[obj->cvt.map[L3GD20_AXIS_Y]] = obj->cvt.sign[L3GD20_AXIS_Y]*obj->cali_sw[L3GD20_AXIS_Y] * mul;
    dat[obj->cvt.map[L3GD20_AXIS_Z]] = obj->cvt.sign[L3GD20_AXIS_Z]*obj->cali_sw[L3GD20_AXIS_Z] * mul;

#if DEBUG
		if(atomic_read(&obj->trace) & GYRO_TRC_CALI)
		{
			GYRO_LOG("Read gyro calibration data  (%5d, %5d, %5d)\n",
				dat[L3GD20_AXIS_X],dat[L3GD20_AXIS_Y],dat[L3GD20_AXIS_Z]);
		}
#endif
          //GYRO_LOG("Read gyro calibration data  (%5d, %5d, %5d)\n",
//				dat[L3GD20_AXIS_X],dat[L3GD20_AXIS_Y],dat[L3GD20_AXIS_Z]);
    return 0;
}
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
static int L3GD20_WriteCalibration(struct i2c_client *client, int dat[L3GD20_AXES_NUM])
{
	struct l3gd20_i2c_data *obj = i2c_get_clientdata(client);
	int err = 0;
	int cali[L3GD20_AXES_NUM];


	//GYRO_FUN();
	if(!obj || ! dat)
	{
		GYRO_ERR("null ptr!!\n");
		return -EINVAL;
	}
	else
	{
		cali[obj->cvt.map[L3GD20_AXIS_X]] = obj->cvt.sign[L3GD20_AXIS_X]*obj->cali_sw[L3GD20_AXIS_X];
		cali[obj->cvt.map[L3GD20_AXIS_Y]] = obj->cvt.sign[L3GD20_AXIS_Y]*obj->cali_sw[L3GD20_AXIS_Y];
		cali[obj->cvt.map[L3GD20_AXIS_Z]] = obj->cvt.sign[L3GD20_AXIS_Z]*obj->cali_sw[L3GD20_AXIS_Z];
		cali[L3GD20_AXIS_X] += dat[L3GD20_AXIS_X];
		cali[L3GD20_AXIS_Y] += dat[L3GD20_AXIS_Y];
		cali[L3GD20_AXIS_Z] += dat[L3GD20_AXIS_Z];
#if DEBUG
		if(atomic_read(&obj->trace) & GYRO_TRC_CALI)
		{
			GYRO_LOG("write gyro calibration data  (%5d, %5d, %5d)-->(%5d, %5d, %5d)\n",
				dat[L3GD20_AXIS_X], dat[L3GD20_AXIS_Y], dat[L3GD20_AXIS_Z],
				cali[L3GD20_AXIS_X],cali[L3GD20_AXIS_Y],cali[L3GD20_AXIS_Z]);
		}
#endif
		return L3GD20_write_rel_calibration(obj, cali);
	}

	return err;
}
/*----------------------------------------------------------------------------*/
static int L3GD20_CheckDeviceID(struct i2c_client *client)
{
	u8 databuf[10];
	int res = 0;

	memset(databuf, 0, sizeof(u8)*10);
	databuf[0] = L3GD20_FIXED_DEVID;

	res = hwmsen_read_byte(client,L3GD20_REG_DEVID,databuf);
    GYRO_LOG(" L3GD20  id %x!\n",databuf[0]);
	if(databuf[0]!=L3GD20_FIXED_DEVID)
	{
		return L3GD20_ERR_IDENTIFICATION;
	}

	//exit_MMA8453Q_CheckDeviceID:
	if (res < 0)
	{
		return L3GD20_ERR_I2C;
	}

	return L3GD20_SUCCESS;
}


//----------------------------------------------------------------------------//
static int L3GD20_SetPowerMode(struct i2c_client *client, bool enable)
{
	u8 databuf[2] = {0};
	int res = 0;

	if(enable == sensor_power)
	{
		GYRO_LOG("Sensor power status is newest!\n");
		return L3GD20_SUCCESS;
	}

	if(hwmsen_read_byte(client, L3GD20_CTL_REG1, databuf))
	{
		GYRO_ERR("read power ctl register err!\n");
		return L3GD20_ERR_I2C;
	}

	databuf[0] &= ~L3GD20_POWER_ON;//clear power on bit
	if(true == enable )
	{
		databuf[0] |= L3GD20_POWER_ON;
	}
	else
	{
		// do nothing
	}
	databuf[1] = databuf[0];
	databuf[0] = L3GD20_CTL_REG1;
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GYRO_LOG("set power mode failed!\n");
		return L3GD20_ERR_I2C;
	}
	else
	{
		GYRO_LOG("set power mode ok %d!\n", enable);
	}

	sensor_power = enable;

    msleep(200); // 确保状态改变后,gyro稳定 苏 勇 2014年07月21日 17:55:52
	return L3GD20_SUCCESS;
}

/*----------------------------------------------------------------------------*/


static int L3GD20_SetDataResolution(struct i2c_client *client, u8 dataResolution)
{
	u8 databuf[2] = {0};
	int res = 0;
	GYRO_FUN();

	if(hwmsen_read_byte(client, L3GD20_CTL_REG4, databuf))
	{
		GYRO_ERR("read L3GD20_CTL_REG4 err!\n");
		return L3GD20_ERR_I2C;
	}
	else
	{
		GYRO_LOG("read  L3GD20_CTL_REG4 register: 0x%x\n", databuf[0]);
	}

	databuf[0] &= 0xcf;//clear
	databuf[0] |= dataResolution;

	databuf[1] = databuf[0];
	databuf[0] = L3GD20_CTL_REG4;


	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GYRO_ERR("write SetDataResolution register err!\n");
		return L3GD20_ERR_I2C;
	}
	return L3GD20_SUCCESS;
}

// set the sample rate
static int L3GD20_SetSampleRate(struct i2c_client *client, u8 sample_rate)
{
	u8 databuf[2] = {0};
	int res = 0;
	GYRO_FUN();

	if(hwmsen_read_byte(client, L3GD20_CTL_REG1, databuf))
	{
		GYRO_ERR("read gyro data format register err!\n");
		return L3GD20_ERR_I2C;
	}
	else
	{
		GYRO_LOG("read  gyro data format register: 0x%x\n", databuf[0]);
	}

	databuf[0] &= 0x3f;//clear
	databuf[0] |= sample_rate;

	databuf[1] = databuf[0];
	databuf[0] = L3GD20_CTL_REG1;


	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GYRO_ERR("write sample rate register err!\n");
		return L3GD20_ERR_I2C;
	}

	return L3GD20_SUCCESS;
}

// set the HPF ENABLE
static int L3GD20_SetHPF_cutoff(struct i2c_client *client, u8  cutoff_val )
{
	u8 databuf[2] = {0};
	int res = 0;
	GYRO_FUN();


	databuf[1] = 0x10;
	databuf[0] = LSM330_CTL_REG5_G;


	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GYRO_ERR("-- ACC --write hpf rate register5 err!\n");
		return res;
	}


	databuf[1] = cutoff_val;
	databuf[0] = LSM330_CTL_REG2_G;


	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GYRO_ERR("--ACC-- write hpf rate register2 err!\n");
		return res;
	}

	return L3GD20_SUCCESS;
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static int L3GD20_ReadGyroData(struct i2c_client *client, char *buf, int bufsize)
{
	char databuf[6];
	int data[3];
	struct l3gd20_i2c_data *obj = i2c_get_clientdata(client);

	if((sensor_power == false)&&(atomic_read(&gyro_suspend) == 0 ))
	{
		L3GD20_SetPowerMode(client, true);
              // for gyro calibration
  	      atomic_set(&gy_flag, 1);
	      atomic_set(&open_flag, 1);
		// for gyro calibration end
	}

	if(hwmsen_read_block(client, AUTO_INCREMENT |L3GD20_REG_GYRO_XL, databuf, 6))
	{
		GYRO_ERR("L3GD20 read gyroscope data  error\n");
		return -2;
	}
	else
	{
		obj->data[L3GD20_AXIS_X] = (s16)((databuf[L3GD20_AXIS_X*2+1] << 8) | (databuf[L3GD20_AXIS_X*2]));
		obj->data[L3GD20_AXIS_Y] = (s16)((databuf[L3GD20_AXIS_Y*2+1] << 8) | (databuf[L3GD20_AXIS_Y*2]));
		obj->data[L3GD20_AXIS_Z] = (s16)((databuf[L3GD20_AXIS_Z*2+1] << 8) | (databuf[L3GD20_AXIS_Z*2]));

#if DEBUG
		if(atomic_read(&obj->trace) & GYRO_TRC_RAWDATA)
		{
			GYRO_LOG("read gyro register: %x, %x, %x, %x, %x, %x",
				databuf[0], databuf[1], databuf[2], databuf[3], databuf[4], databuf[5]);
			GYRO_LOG("get gyro raw data (0x%08X, 0x%08X, 0x%08X) -> (%5d, %5d, %5d)\n",
				obj->data[L3GD20_AXIS_X],obj->data[L3GD20_AXIS_Y],obj->data[L3GD20_AXIS_Z],
				obj->data[L3GD20_AXIS_X],obj->data[L3GD20_AXIS_Y],obj->data[L3GD20_AXIS_Z]);
		}
#endif
		obj->data[L3GD20_AXIS_X] = obj->data[L3GD20_AXIS_X] + obj->cali_sw[L3GD20_AXIS_X];
		obj->data[L3GD20_AXIS_Y] = obj->data[L3GD20_AXIS_Y] + obj->cali_sw[L3GD20_AXIS_Y];
		obj->data[L3GD20_AXIS_Z] = obj->data[L3GD20_AXIS_Z] + obj->cali_sw[L3GD20_AXIS_Z];
	
		/*remap coordinate*/
		data[obj->cvt.map[L3GD20_AXIS_X]] = obj->cvt.sign[L3GD20_AXIS_X]*obj->data[L3GD20_AXIS_X];
		data[obj->cvt.map[L3GD20_AXIS_Y]] = obj->cvt.sign[L3GD20_AXIS_Y]*obj->data[L3GD20_AXIS_Y];
		data[obj->cvt.map[L3GD20_AXIS_Z]] = obj->cvt.sign[L3GD20_AXIS_Z]*obj->data[L3GD20_AXIS_Z];

	
		//Out put the degree/second(o/s)
		data[L3GD20_AXIS_X] = data[L3GD20_AXIS_X] * L3GD20_OUT_MAGNIFY / L3GD20_FS_2000_LSB+ obj->dyna_cali_sw[L3GD20_AXIS_X];
		data[L3GD20_AXIS_Y] = data[L3GD20_AXIS_Y] * L3GD20_OUT_MAGNIFY / L3GD20_FS_2000_LSB+ obj->dyna_cali_sw[L3GD20_AXIS_Y];
		data[L3GD20_AXIS_Z] = data[L3GD20_AXIS_Z] * L3GD20_OUT_MAGNIFY / L3GD20_FS_2000_LSB+ obj->dyna_cali_sw[L3GD20_AXIS_Z];

	}

	GYRO_LOG("data= %d %d %d cali_sw= %d %d %d dyna_cali_sw= %d %d %d\n", 
	data[0], data[1], data[2],
	obj->cali_sw[0],obj->cali_sw[1],obj->cali_sw[2],
	obj->dyna_cali_sw[0],obj->dyna_cali_sw[1],obj->dyna_cali_sw[2]);
	
	sprintf(buf, "%04x %04x %04x", data[L3GD20_AXIS_X],data[L3GD20_AXIS_Y],data[L3GD20_AXIS_Z]);

#if DEBUG
	if(atomic_read(&obj->trace) & GYRO_TRC_DATA)
	{
		GYRO_LOG("get gyro data packet:[%d %d %d]\n", data[0], data[1], data[2]);
	}
#endif

	return 0;

}

static int L3GD20_ReadGyroData1(struct i2c_client *client, char *buf)    //, char *buf, int bufsize)
{
	char databuf[6];
	int data[3];
        long long cur_t1,cur_t2;
	s16 objdata[3];
	struct l3gd20_i2c_data *obj = i2c_get_clientdata(client);

	if((sensor_power == false)&&(atomic_read(&gyro_suspend) == 0 ))
	{
		L3GD20_SetPowerMode(client, true);

	}

         //getTimestamp_l3gd20(&cur_t1,&cur_t2);
         //GYRO_ERR("---- L3GD20 DRIVER ST current ready read reg regccur_time %lld %lld\n", cur_t1,cur_t2);
	if(hwmsen_read_block(client, AUTO_INCREMENT |L3GD20_REG_GYRO_XL, databuf, 6))
	{
		GYRO_ERR("L3GD20 read gyroscope data  error\n");
		return -2;
	}
	else
	{
		objdata[L3GD20_AXIS_X] = (s16)((databuf[L3GD20_AXIS_X*2+1] << 8) | (databuf[L3GD20_AXIS_X*2]));
		objdata[L3GD20_AXIS_Y] = (s16)((databuf[L3GD20_AXIS_Y*2+1] << 8) | (databuf[L3GD20_AXIS_Y*2]));
		objdata[L3GD20_AXIS_Z] = (s16)((databuf[L3GD20_AXIS_Z*2+1] << 8) | (databuf[L3GD20_AXIS_Z*2]));
	  //getTimestamp_l3gd20(&cur_t1,&cur_t2);
         //GYRO_ERR("---- L3GD20 DRIVER ST current end read data regccur_time %lld %lld\n", cur_t1,cur_t2);

#if DEBUG
		if(atomic_read(&obj->trace) & GYRO_TRC_RAWDATA)
		{
			GYRO_LOG("read gyro register: %x, %x, %x, %x, %x, %x",
				databuf[0], databuf[1], databuf[2], databuf[3], databuf[4], databuf[5]);
			GYRO_LOG("get gyro raw data (0x%08X, 0x%08X, 0x%08X) -> (%5d, %5d, %5d)\n",
				objdata[L3GD20_AXIS_X],objdata[L3GD20_AXIS_Y],objdata[L3GD20_AXIS_Z],
				objdata[L3GD20_AXIS_X],objdata[L3GD20_AXIS_Y],objdata[L3GD20_AXIS_Z]);
		}
#endif

		objdata[L3GD20_AXIS_X] = objdata[L3GD20_AXIS_X] + obj->cali_sw[L3GD20_AXIS_X];
		objdata[L3GD20_AXIS_Y] = objdata[L3GD20_AXIS_Y] + obj->cali_sw[L3GD20_AXIS_Y];
		objdata[L3GD20_AXIS_Z] = objdata[L3GD20_AXIS_Z] + obj->cali_sw[L3GD20_AXIS_Z];
	
		/*remap coordinate*/
		data[obj->cvt.map[L3GD20_AXIS_X]] = obj->cvt.sign[L3GD20_AXIS_X]*objdata[L3GD20_AXIS_X];
		data[obj->cvt.map[L3GD20_AXIS_Y]] = obj->cvt.sign[L3GD20_AXIS_Y]*objdata[L3GD20_AXIS_Y];
		data[obj->cvt.map[L3GD20_AXIS_Z]] = obj->cvt.sign[L3GD20_AXIS_Z]*objdata[L3GD20_AXIS_Z];
        }

#if DEBUG
	if(atomic_read(&obj->trace) & GYRO_TRC_DATA)
	{
		GYRO_LOG("get gyro data packet:[%d %d %d]\n", data[0], data[1], data[2]);
	}
#endif
	GYRO_LOG("1data= %d %d %d cali_sw= %d %d %d dyna_cali_sw= %d %d %d\n", 
	data[0], data[1], data[2],
	obj->cali_sw[0],obj->cali_sw[1],obj->cali_sw[2],
	obj->dyna_cali_sw[0],obj->dyna_cali_sw[1],obj->dyna_cali_sw[2]);
	
    return sprintf(buf, "%d %d %d\n", data[L3GD20_AXIS_X],data[L3GD20_AXIS_Y],data[L3GD20_AXIS_Z]);
}


// if we use internel fifo then compile the function L3GD20_SET_FIFO_MODE
#if 0

/*----------------------------------------------------------------------------*/
static int L3GD20_SET_FIFO_MODE(struct i2c_client *client,u8 config)
{
    u8 databuf[2] = {0};
	int res = 0;
	GYRO_FUN();

	if(hwmsen_read_byte(client, L3GD20_FIFO_CTL, databuf))
	{
		GYRO_ERR("read L3GD20_CTL_REG4 err!\n");
		return L3GD20_ERR_I2C;
	}
	else
	{
		GYRO_LOG("read  L3GD20_CTL_REG4 register: 0x%x\n", databuf[0]);
	}

	databuf[0] &= 0x1f;//clear
	databuf[0] |= config;

	databuf[1] = databuf[0];
	databuf[0] = L3GD20_FIFO_CTL;


	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GYRO_ERR("write L3GD20_SET_FIFO_MODE register err!\n");
		return L3GD20_ERR_I2C;
	}
	return L3GD20_SUCCESS;
}
#endif

static int L3GD20_SelfTest(struct i2c_client *client)
{
    int err =0;
	u8 data=0;
	char strbuf[L3GD20_BUFSIZE] = {0};
	int avgx_NOST,avgy_NOST,avgz_NOST;
	int sumx,sumy,sumz;
	int avgx_ST,avgy_ST,avgz_ST;
	int nost_x,nost_y,nost_z=0;
	int st_x,st_y,st_z=0;

	int resx,resy,resz=-1;
	int i=0;
	int testRes=0;
	int sampleNum =5;

	sumx=sumy=sumz=0;
	// 1 init
    err = l3gd20_init_client(client, true);
	if(err)
	{
		GYRO_ERR("initialize client fail! err code %d!\n", err);
		return - 2;
	}
	L3GD20_dumpReg(client);
	// 2 check ZYXDA bit
	hwmsen_read_byte(client,L3GD20_STATUS_REG,&data);
	GYRO_LOG("L3GD20_STATUS_REG=%d\n",data );
// 这里就不检查状态寄存器了,检查其实作用不大,更重要的是下面的循环中data一旦赋值就不会变化,
// 那么实际上就可能是永远不会退出了 苏 勇 2014年07月22日 18:15:02
// 苏 勇 2014年07月22日 18:14:12	while(0x04 != (data&0x04))
	{
	  msleep(10);
	}
	msleep(1000); //wait for stable
	// 3 read raw data no self test data
	for(i=0; i<sampleNum; i++)
	{
	  L3GD20_ReadGyroData(client, strbuf, L3GD20_BUFSIZE);
	  sscanf(strbuf, "%x %x %x", &nost_x, &nost_y, &nost_z);
	  GYRO_LOG("NOst %d %d %d!\n", nost_x,nost_y,nost_z);
	  sumx += nost_x;
	  sumy += nost_y;
	  sumz += nost_z;
	  msleep(10);
	}
	//calculate avg x y z
	avgx_NOST = sumx/sampleNum;
	avgy_NOST = sumy/sampleNum;
	avgz_NOST = sumz/sampleNum;
	GYRO_LOG("avg NOST %d %d %d!\n", avgx_NOST,avgy_NOST,avgz_NOST);

	// 4 enalbe selftest
	hwmsen_read_byte(client,L3GD20_CTL_REG4,&data);
	data = data | 0x02;
	hwmsen_write_byte(client,L3GD20_CTL_REG4,data);

	msleep(1000);//wait for stable

	L3GD20_dumpReg(client);
	// 5 check  ZYXDA bit

	//6 read raw data   self test data
	sumx=0;
	sumy=0;
	sumz=0;
	for(i=0; i<sampleNum; i++)
	{
	  L3GD20_ReadGyroData(client, strbuf, L3GD20_BUFSIZE);
	  sscanf(strbuf, "%x %x %x", &st_x, &st_y, &st_z);
	  GYRO_LOG("st %d %d %d!\n", st_x,st_y,st_z);

	  sumx += st_x;
	  sumy += st_y;
	  sumz += st_z;

	  msleep(10);
	}
	// 7 calc calculate avg x y z ST
	avgx_ST = sumx/sampleNum;
	avgy_ST = sumy/sampleNum;
	avgz_ST = sumz/sampleNum;
	//GYRO_LOG("avg ST %d %d %d!\n", avgx_ST,avgy_ST,avgz_ST);
	//GYRO_LOG("abs(avgx_ST-avgx_NOST): %ld \n", abs(avgx_ST-avgx_NOST));
	//GYRO_LOG("abs(avgy_ST-avgy_NOST): %ld \n", abs(avgy_ST-avgy_NOST));
	//GYRO_LOG("abs(avgz_ST-avgz_NOST): %ld \n", abs(avgz_ST-avgz_NOST));

	if((abs(avgx_ST-avgx_NOST)>=175*131) && (abs(avgx_ST-avgx_NOST)<=875*131))
	{
	  resx =0; //x axis pass
	  GYRO_LOG(" x axis pass\n" );
	}
	if((abs(avgy_ST-avgy_NOST)>=175*131) && (abs(avgy_ST-avgy_NOST)<=875*131))
	{
	  resy =0; //y axis pass
	  GYRO_LOG(" y axis pass\n" );
	}
	if((abs(avgz_ST-avgz_NOST)>=175*131) && (abs(avgz_ST-avgz_NOST)<=875*131))
	{
	  resz =0; //z axis pass
	  GYRO_LOG(" z axis pass\n" );
	}

	if(0==resx && 0==resy && 0==resz)
	{
	  testRes = 0;
	}
	else
	{
	  testRes = -1;
	}

    hwmsen_write_byte(client,L3GD20_CTL_REG4,0x00);
	err = l3gd20_init_client(client, false);
	if(err)
	{
		GYRO_ERR("initialize client fail! err code %d!\n", err);
		return -2;
	}
    GYRO_LOG("testRes %d!\n", testRes);
	return testRes;

}


//self test for factory
static int L3GD20_SMTReadSensorData(struct i2c_client *client)
{
	//S16 gyro[L3GD20_AXES_NUM*L3GD20_FIFOSIZE];
	int res = 0;

	GYRO_FUN();
	res = L3GD20_SelfTest(client);

	GYRO_LOG(" L3GD20_SMTReadSensorData %d", res );

	return res;
}

/*----------------------------------------------------------------------------*/
static int L3GD20_ReadChipInfo(struct i2c_client *client, char *buf, int bufsize)
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

	sprintf(buf, "L3GD20 Chip");
	return 0;
}


/*----------------------------------------------------------------------------*/
static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = l3gd20_i2c_client;
	char strbuf[L3GD20_BUFSIZE];
	if(NULL == client)
	{
		GYRO_ERR("i2c client is null!!\n");
		return 0;
	}

	L3GD20_ReadChipInfo(client, strbuf, L3GD20_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}
/*----------------------------------------------------------------------------*/
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = l3gd20_i2c_client;
	char strbuf[L3GD20_BUFSIZE];

	if(NULL == client)
	{
		GYRO_ERR("i2c client is null!!\n");
		return 0;
	}

	L3GD20_ReadGyroData(client, strbuf, L3GD20_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);;
}

/*----------------------------------------------------------------------------*/
static ssize_t show_trace_value(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	struct l3gd20_i2c_data *obj = obj_i2c_data;
	if (obj == NULL)
	{
		GYRO_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));
	return res;
}
/*----------------------------------------------------------------------------*/
static ssize_t store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct l3gd20_i2c_data *obj = obj_i2c_data;
	int trace;
	if (obj == NULL)
	{
		GYRO_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	if(1 == sscanf(buf, "0x%x", &trace))
	{
		atomic_set(&obj->trace, trace);
	}
	else
	{
		GYRO_ERR("invalid content: '%s', length = %d\n", buf, count);
	}

	return count;
}


static ssize_t show_power_status(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = l3gd20_i2c_client;
	struct lis3dh_i2c_data *obj;
	u8 data;

	if(NULL == client)
	{
		printk("i2c client is null!!\n");
		return 0;
	}

	obj = i2c_get_clientdata(client);
	hwmsen_read_byte(client, L3GD20_CTL_REG1, &data);
    	return snprintf(buf, PAGE_SIZE, "%x\n", data);
}
/*----------------------------------------------------------------------------*/
static ssize_t show_status_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct l3gd20_i2c_data *obj = obj_i2c_data;
	if (obj == NULL)
	{
		GYRO_ERR("i2c_data obj is null!!\n");
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

static ssize_t show_gyro_data(struct device_driver *ddri, char *buf)
{
	return L3GD20_ReadGyroData1(l3gd20_i2c_client, buf);
}
#ifdef FAC_CALI_GYRO
int fac_flag = 0;
static ssize_t show_set_fac(struct device_driver *ddri, char *buf)
{
	fac_flag = 1;
/*
	 struct l3gd20_i2c_data *obj = i2c_get_clientdata(l3gd20_i2c_client);
				obj->cali_sw[L3GD20_AXIS_X] = 0 ;
				obj->cali_sw[L3GD20_AXIS_Y] = 0 ;
				obj->cali_sw[L3GD20_AXIS_Z] =0 ;
				*/
	return 1;
}

static ssize_t show_reset_fac(struct device_driver *ddri, char *buf)
{
	fac_flag = 0;
	return 1;
}
#endif

static ssize_t show_reset_cali(struct device_driver *ddri, char *buf)
{
	struct l3gd20_i2c_data *obj = i2c_get_clientdata(l3gd20_i2c_client);
	obj->cali_sw[L3GD20_AXIS_X] = 0 ;
	obj->cali_sw[L3GD20_AXIS_Y] = 0 ;
	obj->cali_sw[L3GD20_AXIS_Z] =0 ;
	return 1;
}

/*----------------------------------------------------------------------------*/

//Modify by EminHuang 20120613   S_IWUGO | S_IRUGO -> 0664 
//[CTS Test] android.permission.cts.FileSystemPermissionTest#testAllFilesInSysAreNotWritable FAIL
//static DRIVER_ATTR(chipinfo,             S_IWUGO | S_IRUGO, show_chipinfo_value,      NULL);
static DRIVER_ATTR(chipinfo,             0664, show_chipinfo_value,      NULL);	
	
static DRIVER_ATTR(sensordata,           S_IRUGO, show_sensordata_value,    NULL);
//Modify by EminHuang 20120613   S_IWUGO | S_IRUGO -> 0664
//static DRIVER_ATTR(trace,      S_IWUGO | S_IRUGO, show_trace_value,         store_trace_value);
static DRIVER_ATTR(trace,      0664, show_trace_value,         store_trace_value);

static DRIVER_ATTR(status,               S_IRUGO, show_status_value,        NULL);
static DRIVER_ATTR(power,                S_IRUGO, show_power_status,          NULL);

static DRIVER_ATTR(gyro,               S_IRUGO, show_gyro_data,        NULL);
static DRIVER_ATTR(set_fac,               S_IRUGO, show_set_fac,        NULL);
static DRIVER_ATTR(reset_fac,               S_IRUGO, show_reset_fac,        NULL);
static DRIVER_ATTR(reset_cali,               S_IRUGO, show_reset_cali,        NULL);

/*----------------------------------------------------------------------------*/
static struct driver_attribute *L3GD20_attr_list[] = {
	&driver_attr_chipinfo,     /*chip information*/
	&driver_attr_sensordata,   /*dump sensor data*/
	&driver_attr_trace,        /*trace log*/
	&driver_attr_status,
	&driver_attr_power,

	&driver_attr_gyro,
	&driver_attr_set_fac,
	&driver_attr_reset_fac,
	&driver_attr_reset_cali,
};
/*----------------------------------------------------------------------------*/
static int l3gd20_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(L3GD20_attr_list)/sizeof(L3GD20_attr_list[0]));
	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if(0 != (err = driver_create_file(driver, L3GD20_attr_list[idx])))
		{
			GYRO_ERR("driver_create_file (%s) = %d\n", L3GD20_attr_list[idx]->attr.name, err);
			break;
		}
	}
	return err;
}
/*----------------------------------------------------------------------------*/
static int l3gd20_delete_attr(struct device_driver *driver)
{
	int idx ,err = 0;
	int num = (int)(sizeof(L3GD20_attr_list)/sizeof(L3GD20_attr_list[0]));

	if(driver == NULL)
	{
		return -EINVAL;
	}


	for(idx = 0; idx < num; idx++)
	{
		driver_remove_file(driver, L3GD20_attr_list[idx]);
	}


	return err;
}

/*----------------------------------------------------------------------------*/
static int l3gd20_init_client(struct i2c_client *client, bool enable)
{
	struct l3gd20_i2c_data *obj = i2c_get_clientdata(client);
	int res = 0;
	GYRO_FUN();
    	GYRO_LOG(" fwq l3gd20 addr %x!\n",client->addr);
	res = L3GD20_CheckDeviceID(client);
	GYRO_LOG("L3GD20_CheckDeviceID res = %x\n", res);

	if(res != L3GD20_SUCCESS)
	{
		return res;
	}

	res = L3GD20_SetPowerMode(client, enable);
	GYRO_LOG("L3GD20_SetPowerMode res = %x\n", res);

	if(res != L3GD20_SUCCESS)
	{
		return res;
	}

	// The range should at least be 17.45 rad/s (ie: ~1000 deg/s).
	res = L3GD20_SetDataResolution(client,L3GD20_RANGE_2000);//we have only this choice
	GYRO_LOG("L3GD20_SetDataResolution res = %x\n", res);

	if(res != L3GD20_SUCCESS)
	{
		return res;
	}

	//
	res = L3GD20_SetSampleRate(client, L3GD20_100HZ);
	GYRO_LOG("L3GD20_SetSampleRate res = %x\n", res);

	//res  = L3GD20_SetHPF_cutoff(client,LSM330_HPF_CUTOFF_018);

	if(res != L3GD20_SUCCESS )
	{
		return res;
	}


{

	u8 databuf[2] = {0};
	int res = 0;
	
	databuf[1]=0x02;
	databuf[0] = LSM330_CTL_REG5_G;


	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GYRO_ERR("write sample rate register err!\n");
		return L3GD20_ERR_I2C;
	}

}





	GYRO_LOG("l3gd20_init_client OK!\n");

#ifdef CONFIG_L3GD20_LOWPASS
	memset(&obj->fir, 0x00, sizeof(obj->fir));
#endif

	return L3GD20_SUCCESS;
}

static int lsm330gy_ReadOrientationData(char *buf, int bufsize)
{
	if((!buf)||(bufsize<=80))
	{
		return -1;
	}

	read_lock(&lsm330_gy_data.datalock);
	sprintf(buf, "%d %d %d", lsm330_gy_data.yaw, lsm330_gy_data.pitch,
		lsm330_gy_data.roll);
	read_unlock(&lsm330_gy_data.datalock);
	return 0;
}
static int lsm330gy_ReadRotationVectorData(char *buf, int bufsize)
{
	if((!buf)||(bufsize<=80))
	{
		return -1;
	}

	read_lock(&lsm330_gy_data.datalock);
	sprintf(buf, "%d %d %d %d", lsm330_gy_data.RVx, lsm330_gy_data.RVy,
		lsm330_gy_data.RVz,lsm330_gy_data.Scalar);
	read_unlock(&lsm330_gy_data.datalock);
	return 0;
}
static int lsm330gy_ReadGravityData(char *buf, int bufsize)
{
	if((!buf)||(bufsize<=80))
	{
		return -1;
	}

	read_lock(&lsm330_gy_data.datalock);
	sprintf(buf, "%d %d %d %d", lsm330_gy_data.Grx, lsm330_gy_data.Gry,
		lsm330_gy_data.Grz);
	read_unlock(&lsm330_gy_data.datalock);
	return 0;
}
static int lsm330gy_ReadLinearAccData(char *buf, int bufsize)
{
	if((!buf)||(bufsize<=80))
	{
		return -1;
	}

	read_lock(&lsm330_gy_data.datalock);
	sprintf(buf, "%d %d %d %d", lsm330_gy_data.LAx, lsm330_gy_data.LAy,
		lsm330_gy_data.LAz);
	read_unlock(&lsm330_gy_data.datalock);
	return 0;
}

/*----------------------------------------------------------------------------*/
int l3gd20_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	struct l3gd20_i2c_data *priv = (struct l3gd20_i2c_data*)self;
	hwm_sensor_data* gyro_data;
	char buff[L3GD20_BUFSIZE];

	switch (command)
	{
		case SENSOR_DELAY:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				GYRO_ERR("Set delay parameter error!\n");
				err = -EINVAL;
			}
			else
			{
			    if(atomic_read(&or_flag) == 1 || atomic_read(&gr_flag) == 1
                   || atomic_read(&rv_flag) == 1 || atomic_read(&la_flag) == 1)
                {
                    delay = 10;
                    L3GD20_SetSampleRate(priv->client, L3GD20_100HZ);
                }
                else
                {

			    delay = *(int *)buff_in;
			        if(delay>=7)
                        L3GD20_SetSampleRate(priv->client, L3GD20_100HZ);
			        else if(delay>=4)
                        L3GD20_SetSampleRate(priv->client, L3GD20_200HZ);
			        else
                        L3GD20_SetSampleRate(priv->client, L3GD20_400HZ);
                }
			}
			break;

		case SENSOR_ENABLE:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				GYRO_ERR("Enable gyroscope parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				value = *(int *)buff_in;
// 考虑以下的一种情况 苏 勇 2014年07月03日 11:13:31
// 先打开gyro,此时gy_flag和sensor_power都是1
// 关闭gyro,但是线性加速度,或者rv之类继续工作,这个时候gy_flag是0,但sensor_power是1了
// 然后再次打开gyro,这个时候 由于value是1 sensor_power也是1 则被注释的条件成立,也就是不会设置gy_flag
// 当然gy_flag的设置错误,问题不大,只是会影响到gy的校准
// 苏 勇 2014年07月03日 11:13:19				if(((value == 0) && (sensor_power == false)) ||((value == 1) && (sensor_power == true)))
// 苏 勇 2014年07月03日 11:13:19				{
// 苏 勇 2014年07月03日 11:13:19					GYRO_LOG("gyroscope device have updated!\n");
// 苏 勇 2014年07月03日 11:13:19				}
// 苏 勇 2014年07月03日 11:13:19				else
				{
					if(value==1)
					{
                                     L3GD20_SetPowerMode(priv->client, true);
					    atomic_set(&gy_flag, 1);
					    atomic_set(&open_flag, 1);

					}
					else
					{
					    atomic_set(&gy_flag, 0);
					    if(atomic_read(&or_flag) == 0 && atomic_read(&gr_flag) == 0
                           && atomic_read(&rv_flag) == 0 && atomic_read(&la_flag) == 0 )
					    {
						    atomic_set(&open_flag, 0);
                                            L3GD20_SetPowerMode(priv->client, false);
					    }
					}
				}
				wake_up(&open_wq);
			}
			break;

		case SENSOR_GET_DATA:
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				GYRO_ERR("get gyroscope data parameter error!\n");
				err = -EINVAL;
			}else{
				gyro_data = (hwm_sensor_data *)buff_out;
				L3GD20_ReadGyroData(priv->client, buff, L3GD20_BUFSIZE);
                           sscanf(buff, "%x %x %x", &gyro_data->values[0],   &gyro_data->values[1], &gyro_data->values[2]);

				gyro_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
				gyro_data->value_divide = DEGREE_TO_RAD;
			}
			break;
		default:
			GYRO_ERR("gyroscope operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}

	return err;
}
int lsm330gy_orientation_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value, sample_delay, status=0;
	hwm_sensor_data* orsensor_data=NULL;
    struct l3gd20_i2c_data *priv = (struct l3gd20_i2c_data*)self;

	GYRO_FUN(f);
	switch (command)
	{
		case SENSOR_DELAY:

			GYRO_LOG("***orientation SENSOR_DELAY****");
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				GYRO_ERR("Set delay parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				delay = 10;
				L3GD20_SetSampleRate(priv->client, L3GD20_100HZ);
			}
			break;

		case SENSOR_ENABLE:
			GYRO_LOG("***orientation SENSOR_ENABLE****");
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				GYRO_ERR("Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				value = *(int *)buff_in;
				if(value == 1)
				{
					L3GD20_SetPowerMode(priv->client, true);
					atomic_set(&or_flag, 1);
					atomic_set(&open_flag, 1);
				}
				else
				{
					atomic_set(&or_flag, 0);
					if(atomic_read(&gy_flag) == 0 && atomic_read(&gr_flag) == 0
                       && atomic_read(&rv_flag) == 0 && atomic_read(&la_flag) == 0)
					{
						atomic_set(&open_flag, 0);
						L3GD20_SetPowerMode(priv->client, false);
					}
				}
				wake_up(&open_wq);
			}
			break;

		case SENSOR_GET_DATA:
			//GYRO_LOG("************ORIENTATION SENSOR_GET_DATA***********\r\n");
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				GYRO_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				orsensor_data = (hwm_sensor_data *)buff_out;
				read_lock(&lsm330_gy_data.datalock);
				orsensor_data->values[0] = lsm330_gy_data.yaw;
				orsensor_data->values[1] = lsm330_gy_data.pitch;
				orsensor_data->values[2] = lsm330_gy_data.roll;
				orsensor_data->status = SENSOR_STATUS_ACCURACY_HIGH;
				read_unlock(&lsm330_gy_data.datalock);

				orsensor_data->value_divide = 1;

				//GYRO_LOG(" get osensor data: %d, %d, %d, %d!\n", osensor_data->values[0],osensor_data->values[1], osensor_data->values[2], osensor_data->status);
			}
			break;
		default:
			GYRO_ERR("osensor operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}

	return err;
}
int lsm330gy_gravity_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value, sample_delay, status=0;
	hwm_sensor_data* grsensor_data=NULL;
	struct l3gd20_i2c_data *priv = (struct l3gd20_i2c_data*)self;

	GYRO_FUN(f);
	switch (command)
	{
		case SENSOR_DELAY:

			GYRO_LOG("***GRAVITY SENSOR_DELAY****");
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				GYRO_ERR("Set delay parameter error!\n");
				err = -EINVAL;
			}
			else
			{
			    delay = 10;
				L3GD20_SetSampleRate(priv->client, L3GD20_100HZ);
			}
			break;

		case SENSOR_ENABLE:
			GYRO_LOG("***GRAVITY SENSOR_ENABLE****");
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				GYRO_ERR("Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				value = *(int *)buff_in;
				if(value == 1)
				{
					L3GD20_SetPowerMode(priv->client, true);
					atomic_set(&gr_flag, 1);
					atomic_set(&open_flag, 1);
				}
				else
				{
					atomic_set(&gr_flag, 0);
					if(atomic_read(&gy_flag) == 0 && atomic_read(&or_flag) == 0
                       && atomic_read(&rv_flag) == 0 && atomic_read(&la_flag) == 0)
					{
						atomic_set(&open_flag, 0);
						L3GD20_SetPowerMode(priv->client, false);
					}
				}
				wake_up(&open_wq);
			}
			break;

		case SENSOR_GET_DATA:
			//GYRO_LOG("************GRAVITY SENSOR_GET_DATA***********\r\n");
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				GYRO_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				grsensor_data = (hwm_sensor_data *)buff_out;
				read_lock(&lsm330_gy_data.datalock);
				grsensor_data->values[0] = lsm330_gy_data.Grx;
				grsensor_data->values[1] = lsm330_gy_data.Gry;
				grsensor_data->values[2] = lsm330_gy_data.Grz;
				grsensor_data->status = SENSOR_STATUS_ACCURACY_HIGH;
				read_unlock(&lsm330_gy_data.datalock);

				grsensor_data->value_divide = 10000;

				//GYRO_LOG(" get gravity data: %d, %d, %d, %d!\n", osensor_data->values[0],osensor_data->values[1], osensor_data->values[2], osensor_data->status);
			}
			break;
		default:
			GYRO_ERR("gravity sensor operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}

	return err;
}
int lsm330gy_rotation_vector_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value, sample_delay, status=0;
	hwm_sensor_data* rvsensor_data=NULL;
	struct l3gd20_i2c_data *priv = (struct l3gd20_i2c_data*)self;

	GYRO_FUN(f);
	switch (command)
	{
		case SENSOR_DELAY:

			GYRO_LOG("***rotation vector SENSOR_DELAY****");
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
//				printk("Set delay parameter error!\n");
				err = -EINVAL;
			}
			else
			{
			    delay = 10;
				L3GD20_SetSampleRate(priv->client, L3GD20_100HZ);
			}
			break;

		case SENSOR_ENABLE:
			printk("***rotation vector SENSOR_ENABLE****");
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				printk("Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				value = *(int *)buff_in;
				if(value == 1)
				{
					L3GD20_SetPowerMode(priv->client, true);
					atomic_set(&rv_flag, 1);
					atomic_set(&open_flag, 1);
				}
				else
				{
					atomic_set(&rv_flag, 0);
					if(atomic_read(&gy_flag) == 0 && atomic_read(&or_flag) == 0
                       && atomic_read(&gr_flag) == 0 && atomic_read(&la_flag) == 0)
					{
						atomic_set(&open_flag, 0);
						L3GD20_SetPowerMode(priv->client, false);
					}
				}

                          //printk("***rotation vector SENSOR_ENABLE**** open_flag = %d, rv_flag = %d, suspend = %d /n", atomic_read(&open_flag), atomic_read(&rv_flag), atomic_read(&(priv->suspend)));
				wake_up(&open_wq);
			}
			break;

		case SENSOR_GET_DATA:
			//printk("************rotation vector SENSOR_GET_DATA***********\r\n");
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				printk("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				rvsensor_data = (hwm_sensor_data *)buff_out;
				read_lock(&lsm330_gy_data.datalock);
				rvsensor_data->values[0] = lsm330_gy_data.RVx;
				rvsensor_data->values[1] = lsm330_gy_data.RVy;
				rvsensor_data->values[2] = lsm330_gy_data.RVz;
				rvsensor_data->values[3] = lsm330_gy_data.Scalar;
				rvsensor_data->status = SENSOR_STATUS_ACCURACY_HIGH;
				read_unlock(&lsm330_gy_data.datalock);

				rvsensor_data->value_divide = 1000000;

				//printk(" get rotation vector data: %d, %d, %d, %d!\n", osensor_data->values[0],osensor_data->values[1], osensor_data->values[2], osensor_data->status);
			}
			break;
		default:
			GYRO_ERR("rvsensor operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}

	return err;
}

int lsm330gy_linear_acc_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value, sample_delay, status=0;
	hwm_sensor_data* lasensor_data=NULL;
	struct l3gd20_i2c_data *priv = (struct l3gd20_i2c_data*)self;

	GYRO_FUN(f);
	switch (command)
	{
		case SENSOR_DELAY:

			GYRO_LOG("***linear acc SENSOR_DELAY****");
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				printk("Set delay parameter error!\n");
				err = -EINVAL;
			}
			else
			{
			      delay = 10;
				L3GD20_SetSampleRate(priv->client, L3GD20_100HZ);
			}
			break;

		case SENSOR_ENABLE:
			printk("***linear acc SENSOR_ENABLE****");
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				printk("Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				value = *(int *)buff_in;
				if(value == 1)
				{
					L3GD20_SetPowerMode(priv->client, true);
					atomic_set(&la_flag, 1);
					atomic_set(&open_flag, 1);
				}
				else
				{
					atomic_set(&la_flag, 0);
					if(atomic_read(&gy_flag) == 0 && atomic_read(&or_flag) == 0
                       && atomic_read(&gr_flag) == 0 && atomic_read(&rv_flag) == 0)
					{
						atomic_set(&open_flag, 0);
						L3GD20_SetPowerMode(priv->client, false);
					}
				}
				wake_up(&open_wq);
			}
			break;

		case SENSOR_GET_DATA:
			//printk("************linear acc SENSOR_GET_DATA***********\r\n");
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				printk("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				lasensor_data = (hwm_sensor_data *)buff_out;
				read_lock(&lsm330_gy_data.datalock);
				lasensor_data->values[0] = lsm330_gy_data.LAx;
				lasensor_data->values[1] = lsm330_gy_data.LAy;
				lasensor_data->values[2] = lsm330_gy_data.LAz;
				lasensor_data->status = SENSOR_STATUS_ACCURACY_HIGH;
				read_unlock(&lsm330_gy_data.datalock);

				lasensor_data->value_divide = 1000000;

				//GYRO_LOG(" get linear acc data: %d, %d, %d, %d!\n", osensor_data->values[0],osensor_data->values[1], osensor_data->values[2], osensor_data->status);
			}
			break;
		default:
			GYRO_ERR("lasensor operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}

	return err;
}

/******************************************************************************
 * Function Configuration
******************************************************************************/
static int l3gd20_open(struct inode *inode, struct file *file)
{
	file->private_data = l3gd20_i2c_client;

	if(file->private_data == NULL)
	{
		GYRO_ERR("null pointer!!\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int l3gd20_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static int lsm330gy_GetOpenStatus(void)
{
	struct l3gd20_i2c_data *obj = obj_i2c_data;
    int openflag = atomic_read(&open_flag);
    int objsuspend = atomic_read(&(obj->suspend ));

#ifdef FAC_CALI_GYRO
    wait_event_interruptible(open_wq, ( (atomic_read(&open_flag)  == 1) && (atomic_read(&gyro_suspend) == 0 ) && ( fac_flag == 0)));
#else
    wait_event_interruptible(open_wq, ( (atomic_read(&open_flag)  == 1) && (atomic_read(&gyro_suspend) == 0 ) ) );
#endif
	//GYRO_ERR("wait event to wake, block here ........ \n ");
    // printk("wait event to wake, block here ........ \n ");
	return atomic_read(&open_flag);
}

static int lsm330gy_GetSuspendStatus(void)
{
   struct l3gd20_i2c_data *obj = obj_i2c_data;
   int status_gy = -1;
   status_gy =  atomic_read(&gyro_suspend);
   //GYRO_ERR(" ===== --- obj->suspend  = %d \n",status_gy);
   return status_gy;
}

/*----------------------------------------------------------------------------*/
//static int l3gd20_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
//       unsigned long arg)
static long l3gd20_ioctl(struct file *file, unsigned int cmd,
       unsigned long arg)
{
	struct i2c_client *client = (struct i2c_client*)file->private_data;
	//struct l3gd20_i2c_data *obj = (struct l3gd20_i2c_data*)i2c_get_clientdata(client);
	char strbuf[L3GD20_BUFSIZE] = {0};

	int valuebuf[4];
	int sensor_status;
	void __user *data =  (void __user *)arg;

	long err = 0;
	int copy_cnt = 0;
	SENSOR_DATA sensor_data;
	int cali[3];
	int smtRes=0;
        int sensordata[2] ={0};
	//GYRO_FUN();
       //GYRO_ERR("== GYRO IOCTL\n");

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
		GYRO_ERR("== GYRO access error: %08X, (%2d, %2d)\n", cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
		return -EFAULT;
	}

	switch(cmd)
	{
		case GYROSCOPE_IOCTL_INIT:
			l3gd20_init_client(client, false);
			break;

		case GYROSCOPE_IOCTL_SMT_DATA:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;
			}

			smtRes = L3GD20_SMTReadSensorData(client);
			//GYRO_LOG("IOCTL smtRes: %d!\n", smtRes);
			copy_cnt = copy_to_user(data, &smtRes,  sizeof(smtRes));

			if(copy_cnt)
			{
				err = -EFAULT;
				GYRO_ERR("copy gyro data to user failed!\n");
			}
			//GYRO_LOG("copy gyro data to user OK: %d!\n", copy_cnt);
			break;


		case GYROSCOPE_IOCTL_READ_SENSORDATA:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;
			}
			L3GD20_ReadGyroData(client, strbuf, L3GD20_BUFSIZE);
			if(copy_to_user(data, strbuf, sizeof(strbuf)))
			{
				err = -EFAULT;
				break;
			}
			break;

		case GYROSCOPE_IOCTL_SET_CALI:
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

			else
			{
				cali[L3GD20_AXIS_X] = sensor_data.x * L3GD20_FS_2000_LSB / L3GD20_OUT_MAGNIFY;
				cali[L3GD20_AXIS_Y] = sensor_data.y * L3GD20_FS_2000_LSB / L3GD20_OUT_MAGNIFY;
				cali[L3GD20_AXIS_Z] = sensor_data.z * L3GD20_FS_2000_LSB / L3GD20_OUT_MAGNIFY;			  
				err = L3GD20_WriteCalibration(client, cali);
			}
			break;

		case GYROSCOPE_IOCTL_CLR_CALI:
			err = L3GD20_ResetCalibration(client);
			break;

		case GYROSCOPE_IOCTL_GET_CALI:
			data = (void __user*)arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;
			}
#if 1
			err = L3GD20_ReadCalibration(client, cali);
			if(err)
			{
				break;
			}

			sensor_data.x = cali[L3GD20_AXIS_X] * L3GD20_OUT_MAGNIFY / L3GD20_FS_2000_LSB;
			sensor_data.y = cali[L3GD20_AXIS_Y] * L3GD20_OUT_MAGNIFY / L3GD20_FS_2000_LSB;
			sensor_data.z = cali[L3GD20_AXIS_Z] * L3GD20_OUT_MAGNIFY / L3GD20_FS_2000_LSB;
#else
		       struct l3gd20_i2c_data *obj = i2c_get_clientdata(client);
			sensor_data.x = obj->cali_sw[L3GD20_AXIS_X] ;
			sensor_data.y = obj->cali_sw[L3GD20_AXIS_Y] ;
			sensor_data.z = obj->cali_sw[L3GD20_AXIS_Z];
#endif
			if(copy_to_user(data, &sensor_data, sizeof(sensor_data)))
			{
				err = -EFAULT;
				break;
			}
			break;
		case GYROSCOPE_IOCTL_CLEAR_DYNAMIC_CALI:
			err=L3GD20_ResetDynaCalibration(client);
			break;
		case GYROSCOPE_IOCTL_SET_DYNAMIC_CALI:
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

			else
			{
				struct l3gd20_i2c_data *obj = i2c_get_clientdata(client);
               obj->dyna_cali_sw[L3GD20_AXIS_X] = sensor_data.x ;
               obj->dyna_cali_sw[L3GD20_AXIS_Y] = sensor_data.y ;
               obj->dyna_cali_sw[L3GD20_AXIS_Z] = sensor_data.z ;
			}
			break;

		case GYROSCOPE_IOC_GET_OFLAG:
			sensor_status = atomic_read(&or_flag);
			if(copy_to_user(data, &sensor_status, sizeof(sensor_status)))
			{
				printk("copy_to_user failed. GYROSCOPE_IOC_GET_OFLAG ");
				return -EFAULT;
			}
			break;
		case GYROSCOPE_IOC_GET_GRFLAG:
			sensor_status = atomic_read(&gr_flag);
			if(copy_to_user(data, &sensor_status, sizeof(sensor_status)))
			{
				printk("copy_to_user failed. GYROSCOPE_IOC_GET_GRFLAG ");
				return -EFAULT;
			}
			break;
		case GYROSCOPE_IOC_GET_RVFLAG:
			sensor_status = atomic_read(&rv_flag);
			if(copy_to_user(data, &sensor_status, sizeof(sensor_status)))
			{
				printk("copy_to_user failed. GYROSCOPE_IOC_GET_RVFLAG ");
				return -EFAULT;
			}
			break;
		case GYROSCOPE_IOC_GET_GYFLAG:
			sensor_status = atomic_read(&gy_flag);
			//if(copy_to_user(data, &sensor_status, sizeof(sensor_status)))
                        //GYRO_ERR("===========GET__SUSPEND_gyro=======\n");
                        sensordata[0] = sensor_status;
                        sensordata[1] = lsm330gy_GetSuspendStatus();
                      //  GYRO_ERR("===========sensordata[0]= %d,sensordata[1] = %d,=======\n",sensordata[0],sensordata[1]);
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;
			}
			if(copy_to_user(data, sensordata, sizeof(sensordata)))
			{
				printk("copy_to_user failed. GYROSCOPE_IOC_GET_GYFLAG");
				return -EFAULT;
			}
			break;
		case GYROSCOPE_IOC_GET_LAFLAG:
			sensor_status = atomic_read(&la_flag);
			if(copy_to_user(data, &sensor_status, sizeof(sensor_status)))
			{
				printk("copy_to_user failed. GYROSCOPE_IOC_GET_LAFLAG");
				return -EFAULT;
			}
			break;
		case GYROSCOPE_IOC_GET_OPEN_STATUS:
			//GYRO_ERR("===========GET__OPEN_STATU=======\n");
			sensor_status =  lsm330gy_GetOpenStatus();
			if(copy_to_user(data, &sensor_status, sizeof(sensor_status)))
			{
				printk("copy_to_user failed.GYROSCOPE_IOC_GET_OPEN_STATUS  ");
				return -EFAULT;
			}
			//printk("===========GET__OPEN_STATU  DONE=======\r\n");
			break;
		case GYROSCOPE_IOCTL_SET_ORIENTATION:
			//printk("===========SET_ORIENTATION=======\r\n");
			data = (void __user *) arg;
			if(data == NULL)
			{
				printk("IO parameter pointer is NULL!\r\n");
				break;
			}

			if(copy_from_user(&valuebuf, data, sizeof(valuebuf)))
			{
				return -EFAULT;
			}

			write_lock(&lsm330_gy_data.datalock);
			lsm330_gy_data.yaw   = valuebuf[0];
			lsm330_gy_data.pitch = valuebuf[1];
		    lsm330_gy_data.roll  = valuebuf[2];
			write_unlock(&lsm330_gy_data.datalock);


			//GYRO_LOG("SET_ORIENTATION data: %d, %d, %d!\n", lsm303mmid_data.yaw ,lsm303mmid_data.pitch ,lsm303mmid_data.pitch);
			break;          //abandon for orientation
        //added by ST WU Yi 2013.11.25.
		case GYROSCOPE_IOCTL_SET_GRAVITY:
			//printk("===========SET_GRAVITY=======\r\n");
			data = (void __user *) arg;
			if(data == NULL)
			{
				printk("IO parameter pointer is NULL!\r\n");
				break;
			}

			if(copy_from_user(&valuebuf, data, sizeof(valuebuf)))
			{

				return -EFAULT;
            }

			write_lock(&lsm330_gy_data.datalock);
			lsm330_gy_data.Grx  = valuebuf[0];
            lsm330_gy_data.Gry  = valuebuf[1];
            lsm330_gy_data.Grz  = valuebuf[2];
            write_unlock(&lsm330_gy_data.datalock);

			//GYRO_LOG("SET_GRAVITY osensor data: %d, %d, %d!\n", lsm303mmid_data.simuGyrox ,lsm303mmid_data.simuGyroy ,lsm303mmid_data.simuGyroz);
			break;
		case GYROSCOPE_IOCTL_SET_ROTATION_VECTOR:
			//printk("===========SET_ROTATION_VECTOR=======\r\n");
			data = (void __user *) arg;
			if(data == NULL)
			{
				printk("IO parameter pointer is NULL!\r\n");
				break;
			}

			if(copy_from_user(&valuebuf, data, sizeof(valuebuf)))
			{
				return -EFAULT;
			}

			write_lock(&lsm330_gy_data.datalock);
            lsm330_gy_data.RVx = valuebuf[0];
            lsm330_gy_data.RVy = valuebuf[1];
            lsm330_gy_data.RVz = valuebuf[2];
			lsm330_gy_data.Scalar = valuebuf[3];
			write_unlock(&lsm330_gy_data.datalock);

			//GYRO_LOG("SET_ROTATION_VECTOR rvsensor data: %d, %d, %d!\n", lsm303mmid_data.RVx,lsm303mmid_data.RVy,lsm303mmid_data.RVz);
			break;
		case GYROSCOPE_IOCTL_SET_LINEAR_ACC:
			//printk("===========SET_LINEAR_ACC=======\r\n");
			data = (void __user *) arg;
			if(data == NULL)
			{
				printk("IO parameter pointer is NULL!\r\n");
				break;
			}

			if(copy_from_user(&valuebuf, data, sizeof(valuebuf)))
			{
				return -EFAULT;
			}

			write_lock(&lsm330_gy_data.datalock);
            lsm330_gy_data.LAx = valuebuf[0];
            lsm330_gy_data.LAy = valuebuf[1];
            lsm330_gy_data.LAz = valuebuf[2];
			write_unlock(&lsm330_gy_data.datalock);

			//GYRO_LOG("SET_LINEAR_ACC data: %d, %d, %d!\n", lsm303mmid_data.RVx,lsm303mmid_data.RVy,lsm303mmid_data.RVz);
			break;
	  	case GYROSCOPE_IOCTL_READ_ORIENTATION_DATA:
			//GYRO_LOG("===========IOCTL_READ_READ_ORIENTATION=======\r\n");
			data = (void __user *) arg;
			if(data == NULL)
			{
				printk("IO parameter pointer is NULL!\r\n");
				break;
			}

			lsm330gy_ReadOrientationData(strbuf, L3GD20_BUFSIZE);
			if(copy_to_user(data, strbuf, strlen(strbuf)+1))
			{
				return -EFAULT;
			}
			break;    // abandon for orientation
        case GYROSCOPE_IOCTL_READ_GRAVITY_DATA:
			//GYRO_LOG("===========IOCTL_READ_READ_GRAVITY_DATA=======\r\n");
			data = (void __user *) arg;
			if(data == NULL)
			{
				printk("IO parameter pointer is NULL!\r\n");
				break;
			}

			lsm330gy_ReadGravityData(strbuf, L3GD20_BUFSIZE);
			if(copy_to_user(data, strbuf, strlen(strbuf)+1))
			{
				return -EFAULT;
			}
			break;
        case GYROSCOPE_IOCTL_READ_ROTATION_VECTOR_DATA:
			//printk("===========IOCTL_READ_READ_ROTATION_VECTOR_DATA=======\r\n");
			data = (void __user *) arg;
			if(data == NULL)
			{
				printk("IO parameter pointer is NULL!\r\n");
				break;
			}

			lsm330gy_ReadRotationVectorData(strbuf, L3GD20_BUFSIZE);
			if(copy_to_user(data, strbuf, strlen(strbuf)+1))
			{
				return -EFAULT;
			}
			break;
        case GYROSCOPE_IOCTL_READ_LINEAR_ACC_DATA:
			//GYRO_LOG("===========IOCTL_READ_READ_LINEAR_ACC_DATA=======\r\n");
			data = (void __user *) arg;
			if(data == NULL)
			{
				printk("IO parameter pointer is NULL!\r\n");
				break;
			}

			lsm330gy_ReadLinearAccData(strbuf, L3GD20_BUFSIZE);
			if(copy_to_user(data, strbuf, strlen(strbuf)+1))
			{
				return -EFAULT;
			}
			break;

		default:
			GYRO_ERR("unknown IOCTL: 0x%08x\n", cmd);
			err = -ENOIOCTLCMD;
			break;
	}
	return err;
}


/*----------------------------------------------------------------------------*/
static struct file_operations l3gd20_fops = {
//	.owner = THIS_MODULE,
	.open = l3gd20_open,
	.release = l3gd20_release,
	.unlocked_ioctl = l3gd20_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice l3gd20_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gyroscope",
	.fops = &l3gd20_fops,
};
/*----------------------------------------------------------------------------*/
#ifndef CONFIG_HAS_EARLYSUSPEND
/*----------------------------------------------------------------------------*/
static int l3gd20_suspend(struct i2c_client *client, pm_message_t msg)
{
	struct l3gd20_i2c_data *obj = i2c_get_clientdata(client);
	GYRO_FUN();

	if(msg.event == PM_EVENT_SUSPEND)
	{
		if(obj == NULL)
		{
			GYRO_ERR("null pointer!!\n");
			return -EINVAL;
		}

		atomic_set(&obj->suspend, 1);
		atomic_set(&gyro_suspend, 1);
		err = L3GD20_SetPowerMode(client, false);
		if(err <= 0)
		{
			return err;
		}
	}

	return err;
}
/*----------------------------------------------------------------------------*/
static int l3gd20_resume(struct i2c_client *client)
{
	struct l3gd20_i2c_data *obj = i2c_get_clientdata(client);
	int err;
	GYRO_FUN();

	if(obj == NULL)
	{
		GYRO_ERR("null pointer!!\n");
		return -EINVAL;
	}

	L3GD20_power(obj->hw, 1);


	err = l3gd20_init_client(client, false);
	if(err)
	{
		GYRO_ERR("initialize client fail!!\n");
		return err;
	}


	atomic_set(&obj->suspend, 0);
    atomic_set(&gyro_suspend, 0);
        wake_up(&open_wq);

	return 0;
}
/*----------------------------------------------------------------------------*/
#else /*CONFIG_HAS_EARLY_SUSPEND is defined*/
/*----------------------------------------------------------------------------*/
static void l3gd20_early_suspend(struct early_suspend *h)
{
	struct l3gd20_i2c_data *obj = container_of(h, struct l3gd20_i2c_data, early_drv);
	int err;
	GYRO_FUN();

	if(obj == NULL)
	{
		GYRO_ERR("null pointer!!\n");
		return;
	}
	atomic_set(&obj->suspend, 1);
    atomic_set(&gyro_suspend, 1);
	err = L3GD20_SetPowerMode(obj->client, false);
	if(err)
	{
		GYRO_ERR("write power control fail!!\n");
		return;
	}
	if(err <= 0)
	{
		return;
	}

	sensor_power = false;

	L3GD20_power(obj->hw, 0);
}
/*----------------------------------------------------------------------------*/
static void l3gd20_late_resume(struct early_suspend *h)
{
	struct l3gd20_i2c_data *obj = container_of(h, struct l3gd20_i2c_data, early_drv);
	int err;
	GYRO_FUN();

	if(obj == NULL)
	{
		GYRO_ERR("null pointer!!\n");
		return;
	}

	L3GD20_power(obj->hw, 1);

	err = l3gd20_init_client(obj->client, false);
	if(err)
	{
		GYRO_ERR("initialize client fail! err code %d!\n", err);
		return;
	}
	atomic_set(&obj->suspend, 0);
    atomic_set(&gyro_suspend, 0);

    wake_up(&open_wq);
}
/*----------------------------------------------------------------------------*/
#endif /*CONFIG_HAS_EARLYSUSPEND*/
/*----------------------------------------------------------------------------*/
static int l3gd20_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info)
{
	strcpy(info->type, L3GD20_DEV_NAME);
	return 0;
}
#ifdef MTK_AUTO_DETECT_GYROSCOPE
static int i2c_probe_ok = 0;
#endif
/*----------------------------------------------------------------------------*/
static int l3gd20_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_client *new_client;
	struct l3gd20_i2c_data *obj;
	struct hwmsen_object sobj;
	int err = 0;

	struct hwmsen_object sobj_gr, sobj_la, sobj_rv;  //sobj_or,
// 苏 勇 2014年06月04日 16:16:32		struct hwmsen_object sobj_or;

	GYRO_FUN();
	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}

	memset(obj, 0, sizeof(struct l3gd20_i2c_data));
#ifdef MTK_AUTO_DETECT_GYROSCOPE
	obj->hw = lsm330_get_cust_gyro_hw();
#else
	obj->hw = get_cust_gyro_hw();
#endif
	err = hwmsen_get_convert(obj->hw->direction, &obj->cvt);
	if(err)
	{
		GYRO_ERR("invalid direction: %d\n", obj->hw->direction);
		goto exit;
	}

	obj_i2c_data = obj;
	obj->client = client;
	new_client = obj->client;
	i2c_set_clientdata(new_client,obj);

	atomic_set(&obj->trace, 0);
	atomic_set(&obj->suspend, 0);

    init_waitqueue_head(&open_wq);

	l3gd20_i2c_client = new_client;
	err = l3gd20_init_client(new_client, false);
	if(err)
	{
		goto exit_init_failed;
	}


	err = misc_register(&l3gd20_device);
	if(err)
	{
		GYRO_ERR("l3gd20_device misc register failed!\n");
		goto exit_misc_device_register_failed;
	}
#ifndef MTK_AUTO_DETECT_GYROSCOPE
	err = l3gd20_create_attr(&l3gd20_gyro_driver.driver);
	if(err)
	{
		GYRO_ERR("l3gd20 create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}
#endif

	sobj.self = obj;
    sobj.polling = 1;
    sobj.sensor_operate = l3gd20_operate;
//remark by fangchsh for w8850
#if 1
	err = hwmsen_attach(ID_GYROSCOPE, &sobj);
	if(err)
	{
		GYRO_ERR("hwmsen_attach fail = %d\n", err);
		goto exit_kfree;
	}
#endif

    sobj_gr.self = obj;
    sobj_gr.polling = 1;
	sobj_gr.sensor_operate = lsm330gy_gravity_operate;
    if((err = hwmsen_attach(ID_GRAVITY, &sobj_gr)))
	{
		printk("attach fail = %d\n", err);
		goto exit_kfree;
	}
// 苏 勇 2014年06月04日 16:16:32	sobj_or.self = obj;
// 苏 勇 2014年06月04日 16:16:32	sobj_or.polling = 1;
// 苏 勇 2014年06月04日 16:16:32	sobj_or.sensor_operate = lsm330gy_orientation_operate;
// 苏 勇 2014年06月04日 16:16:32	if((err = hwmsen_attach(ID_ORIENTATION, &sobj_or)))
// 苏 勇 2014年06月04日 16:16:32	{
// 苏 勇 2014年06月04日 16:16:32		printk("attach fail = %d\n", err);
// 苏 勇 2014年06月04日 16:16:32		goto exit_kfree;
// 苏 勇 2014年06月04日 16:16:32	}
	sobj_la.self = obj;
	sobj_la.polling = 1;
	sobj_la.sensor_operate = lsm330gy_linear_acc_operate;
	if((err = hwmsen_attach(ID_LINEAR_ACCELERATION, &sobj_la)))
	{
		printk("attach fail = %d\n", err);
		goto exit_kfree;
	}
	sobj_rv.self = obj;
	sobj_rv.polling = 1;
	sobj_rv.sensor_operate = lsm330gy_rotation_vector_operate;
	if((err = hwmsen_attach(ID_ROTATION_VECTOR, &sobj_rv)))
	{
		printk("attach fail = %d\n", err);
		goto exit_kfree;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	obj->early_drv.level    = EARLY_SUSPEND_LEVEL_STOP_DRAWING - 2,
	obj->early_drv.suspend  = l3gd20_early_suspend,
	obj->early_drv.resume   = l3gd20_late_resume,
	register_early_suspend(&obj->early_drv);
#endif

	GYRO_LOG("%s: OK\n", __func__);
#ifdef MTK_AUTO_DETECT_GYROSCOPE
	 i2c_probe_ok = 1;
#endif
	return 0;

	exit_create_attr_failed:
	misc_deregister(&l3gd20_device);
	exit_misc_device_register_failed:
	exit_init_failed:
	//i2c_detach_client(new_client);
	exit_kfree:
	kfree(obj);
	exit:
	GYRO_ERR("%s: err = %d\n", __func__, err);
#ifdef MTK_AUTO_DETECT_GYROSCOPE
	 i2c_probe_ok = -1;
#endif
	return err;
}

/*----------------------------------------------------------------------------*/
static int l3gd20_i2c_remove(struct i2c_client *client)
{
	int err = 0;

	err = l3gd20_delete_attr(&l3gd20_gyro_driver.driver);
	if(err)
	{
		GYRO_ERR("l3gd20_delete_attr fail: %d\n", err);
	}

	err = misc_deregister(&l3gd20_device);
	if(err)
	{
		GYRO_ERR("misc_deregister fail: %d\n", err);
	}

	err = hwmsen_detach(ID_ACCELEROMETER);
	if(err)
	{
		GYRO_ERR("hwmsen_detach fail: %d\n", err);
	}

	l3gd20_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));
	return 0;
}
#ifdef MTK_AUTO_DETECT_GYROSCOPE
#include <linux/dev_info.h>
/*----------------------------------------------------------------------------*/
static int l3gd20_probe_comp(void )
{
	struct gyro_hw *hw = lsm330_get_cust_gyro_hw();
	GYRO_FUN();

	L3GD20_power(hw, 1);
	struct devinfo_struct *dev = (struct devinfo_struct*)kmalloc(sizeof(struct devinfo_struct), GFP_KERNEL);;
	dev->device_type = "GYRO";
	dev->device_vendor = "ST";
	dev->device_ic = "lsm330";
	dev->device_version = DEVINFO_NULL;
	dev->device_module = DEVINFO_NULL;
	dev->device_info = DEVINFO_NULL;

//	l3gd20_force[0] = hw->i2c_num;
	if(i2c_add_driver(&l3gd20_i2c_driver))
	{
		GYRO_ERR("add driver error\n");
		dev->device_used = DEVINFO_UNUSED;
		DEVINFO_CHECK_ADD_DEVICE(dev);
		return -1;
	}



	if( -1 == i2c_probe_ok ){
		i2c_del_driver(&l3gd20_i2c_driver);
		//printk("delete l3gd20_i2c_driver \n");
		dev->device_used = DEVINFO_UNUSED;
		DEVINFO_CHECK_ADD_DEVICE(dev);
		return -1;
	}else{
		struct devinfo_struct *dev = (struct devinfo_struct*)kmalloc(sizeof(struct devinfo_struct), GFP_KERNEL);;
		dev->device_type = "GYRO";
		dev->device_vendor = "ST";
		dev->device_ic = "lsm330";
		dev->device_version = DEVINFO_NULL;
		dev->device_module = DEVINFO_NULL;
		dev->device_info = DEVINFO_NULL;
		dev->device_used = DEVINFO_USED;
		DEVINFO_CHECK_ADD_DEVICE(dev);
		return 0;
	}
}
/*----------------------------------------------------------------------------*/
static int l3gd20_remove_comp(void)
{
    struct gyro_hw *hw = lsm330_get_cust_gyro_hw();

    GYRO_FUN();
    L3GD20_power(hw, 0);
    i2c_del_driver(&l3gd20_i2c_driver);
    return 0;
}
/*----------------------------------------------------------------------------*/
static struct sensor_init_info mtk_sensor_driver = {
	.name      = "gyroscope",
	.init     = l3gd20_probe_comp,
	.uninit  = l3gd20_remove_comp,
	.create_sys_file = l3gd20_create_attr,
};

#else
/*----------------------------------------------------------------------------*/
static int l3gd20_probe(struct platform_device *pdev)
{
	struct gyro_hw *hw = get_cust_gyro_hw();
	GYRO_FUN();

	L3GD20_power(hw, 1);
//	l3gd20_force[0] = hw->i2c_num;
	rwlock_init(&lsm330_gy_data.datalock);

	if(i2c_add_driver(&l3gd20_i2c_driver))
	{
		GYRO_ERR("add driver error\n");
		return -1;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static int l3gd20_remove(struct platform_device *pdev)
{
    struct gyro_hw *hw = get_cust_gyro_hw();

    GYRO_FUN();
    L3GD20_power(hw, 0);
    i2c_del_driver(&l3gd20_i2c_driver);
    return 0;
}
/*----------------------------------------------------------------------------*/
static struct platform_driver l3gd20_gyro_driver = {
	.probe      = l3gd20_probe,
	.remove     = l3gd20_remove,
	.driver     = {
		.name  = "gyroscope",
//		.owner = THIS_MODULE,
	}
};
#endif
/*----------------------------------------------------------------------------*/
static int __init l3gd20_init(void)
{
#ifdef MTK_AUTO_DETECT_GYROSCOPE
	struct gyro_hw *hw = lsm330_get_cust_gyro_hw();
#else
	struct gyro_hw *hw = get_cust_gyro_hw();
#endif
//	GYRO_LOG("%s: i2c_number=%d\n", __func__,hw->i2c_num);
	i2c_register_board_info(hw->i2c_num, &i2c_l3gd20, 1);

#ifdef MTK_AUTO_DETECT_GYROSCOPE
	hwmsen_gyro_add(&mtk_sensor_driver);
#else
	if(platform_driver_register(&l3gd20_gyro_driver))
	{
		GYRO_ERR("failed to register driver");
		return -ENODEV;
	}
#endif
	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit l3gd20_exit(void)
{
	GYRO_FUN();
	platform_driver_unregister(&l3gd20_gyro_driver);
}
/*----------------------------------------------------------------------------*/
module_init(l3gd20_init);
module_exit(l3gd20_exit);
/*----------------------------------------------------------------------------*/
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("L3GD20 gyroscope driver");
MODULE_AUTHOR("Chunlei.Wang@mediatek.com");
