/* 
 * Copyright (C) 2012 Senodia Corporation.
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

#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>
#include "st480.h"

#include "mag.h"
/*----------------------------------------------------------------------------*/
#define MSE_TAG                  "MSENSOR"
#define MSE_FUN(f)               printk(MSE_TAG" %s\r\n", __FUNCTION__)
#define MSE_ERR(fmt, args...)    printk(KERN_ERR MSE_TAG" %s %d : \r\n"fmt, __FUNCTION__, __LINE__, ##args)
#define MSE_LOG(fmt, args...)    printk(KERN_INFO MSE_TAG fmt, ##args)

#define MSE_VER(fmt, args...)   ((void)0)

#define POWER_NONE_MACRO MT65XX_POWER_NONE


/*----------------------------------------------------------------------------*/
static int  st480_local_init(void);
static int  st_remove(void);
extern struct mag_hw* st480_get_cust_mag_hw(void);

static int st480_init_flag =0; // 0<==>OK -1 <==> fail

static struct mag_init_info st480_init_info = {
		.name = "st480",
		.init = st480_local_init,
		.uninit = st_remove,
	
};
#define ST480_AXIS_X            0
#define ST480_AXIS_Y            1
#define ST480_AXIS_Z            2
#define ST480_AXES_NUM          3

#define ST480_BUFSIZE          256

/*----------------------------------------------------------------------------*/
static struct i2c_board_info __initdata i2c_st480={ I2C_BOARD_INFO("st480", (0X0c))};
static struct platform_driver st480_sensor_driver;


static DECLARE_WAIT_QUEUE_HEAD(open_wq);

struct st480_data {
	struct i2c_client *client; 
	struct mag_hw *hw; 
	struct hwmsen_convert   cvt;
	atomic_t layout;   
	atomic_t trace;
};

struct SensorData st480sensordata;


static struct st480_data *st480;

static atomic_t m_flag;
static atomic_t o_flag ;
static atomic_t open_flag;



static atomic_t dev_open_count;	

volatile static short st480d_delay = ST480_DEFAULT_DELAY;

struct mag_3{
	s16  mag_x,
	mag_y,
	mag_z;
};
volatile static struct mag_3 mag;

/*----------------------------------------------------------------------------*/
static void st480_power(struct mag_hw *hw, unsigned int on) 
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
			if(!hwPowerOn(hw->power_id, hw->power_vol, "st480")) 
			{
				MSE_ERR("power on fails!!\n");
			}
		}
		else
		{
			if(!hwPowerDown(hw->power_id, "st480")) 
			{
				MSE_ERR("power off fail!!\n");
			}
		}
	}
	power_on = on;
}

/*----------------------------------------------------------------------------*/

static int st480_GetOpenStatus(void)
{
	wait_event_interruptible(open_wq, (atomic_read(&open_flag) != 0));
	return atomic_read(&open_flag);
}


/*
 * st480 i2c transfer
 * read/write
 */
static int st480_i2c_transfer_data(struct i2c_client *client, int len, char *buf, int length)
{
        struct i2c_msg msgs[] = {
                {
                        .addr  =  client->addr,
                        .flags  =  0,
                        .len  =  len,
                        .buf  =  buf,
                },
                {
                        .addr  =  client->addr,
                        .flags  = I2C_M_RD,
                        .len  =  length,
                        .buf  =  buf,
                },
        };

        if(i2c_transfer(client->adapter, msgs, 2) < 0){
                pr_err("megnetic_i2c_read_data: transfer error\n");
                return EIO;
        }
        else
                return 0;
}

/*
 * Device detect and init
 * 
 */
static int client_init(struct i2c_client *client)
{
	MSE_FUN();

	int ret;
	unsigned char buf[5];
	unsigned char data[1];

	memset(buf, 0, 5);
	memset(data, 0, 1);	

	buf[0] = READ_REGISTER_CMD;
	buf[1] = 0x00;	
	ret = 0;

	while(st480_i2c_transfer_data(client, 2, buf, 3)!=0)
        {
                ret++;
                msleep(1);
                if(st480_i2c_transfer_data(client, 2, buf, 3)==0)
                {
                        break;
                }
                if(ret > MAX_FAILURE_COUNT)
                {
                        return -EIO;
                }
        }

/***
	if(buf[2] != ST480_DEVICE_ID)
	{
		printk("st480 ic not exist!");
		return -ENODEV;
	}
**/

printk("st480 device id is %x \n", buf[2]);

//init register step 1
	buf[0] = WRITE_REGISTER_CMD;
	buf[1] = ONE_INIT_DATA_HIGH;
        buf[2] = ONE_INIT_DATA_LOW;
        buf[3] = ONE_INIT_REG;  
        ret = 0;
	while(st480_i2c_transfer_data(client, 4, buf, 1)!=0)
        {
                ret++;
                msleep(1);
                if(st480_i2c_transfer_data(client, 4, buf, 1)==0)
                {
                        break;
                }
                if(ret > MAX_FAILURE_COUNT)
                {
                        return -EIO;
                }
        }

//init register step 2
	buf[0] = WRITE_REGISTER_CMD;
	buf[1] = TWO_INIT_DATA_HIGH;
        buf[2] = TWO_INIT_DATA_LOW;
        buf[3] = TWO_INIT_REG;
        ret = 0;
	while(st480_i2c_transfer_data(client, 4, buf, 1)!=0)
        {
                ret++;
                msleep(1);
                if(st480_i2c_transfer_data(client, 4, buf, 1)==0)
                {
                        break;
                }
                if(ret > MAX_FAILURE_COUNT)
                {
                        return -EIO;
                }
        }

//set calibration register
	buf[0] = WRITE_REGISTER_CMD;
	buf[1] = CALIBRATION_DATA_HIGH;
        buf[2] = CALIBRATION_DATA_LOW;
        buf[3] = CALIBRATION_REG;
        ret = 0;
	while(st480_i2c_transfer_data(client, 4, buf, 1)!=0)
        {
                ret++;
                msleep(1);
                if(st480_i2c_transfer_data(client, 4, buf, 1)==0)
                {
                        break;
                }
                if(ret > MAX_FAILURE_COUNT)
                {
                        return -EIO;
                }
        }

//set mode config
	buf[0] = SINGLE_MEASUREMENT_MODE_CMD;	
	ret=0;
        while(st480_i2c_transfer_data(client, 1, buf, 1)!=0)
        {
                ret++;
                msleep(1);
                if(st480_i2c_transfer_data(client, 1, buf, 1)==0)
                {
                        break;
                }
                if(ret > MAX_FAILURE_COUNT)
                {
                        return -EIO;
                }
        }

	return 0;
}

static void st480_work_func(void)
{
	char buffer[7];
	int ret;
	//int mag[ST480_AXES_NUM];

	memset(buffer, 0, 7);

	buffer[0] = READ_MEASUREMENT_CMD;
	ret=0;
	while(st480_i2c_transfer_data(st480->client, 1, buffer, 7)!=0)
	{
		ret++;
		
		if(st480_i2c_transfer_data(st480->client, 1, buffer, 7)==0)
                {
                        break;
                }
                if(ret > MAX_FAILURE_COUNT)
                {
                        break;
                }
	}

	if(!((buffer[0]>>4) & 0X01))
	{
		mag.mag_x = ((buffer[1]<<8)|buffer[2]);
        mag.mag_y = ((buffer[3]<<8)|buffer[4]);
        mag.mag_z = ((buffer[5]<<8)|buffer[6]);

		//printk("st480 raw data: x = %d, y = %d, z = %d \n",mag.mag_x,mag.mag_y,mag.mag_z);
	}
	mag.mag_x = st480->cvt.sign[ST480_AXIS_X]*mag.mag_x;
	mag.mag_y = st480->cvt.sign[ST480_AXIS_Y]*mag.mag_y;
	mag.mag_z = st480->cvt.sign[ST480_AXIS_Z]*mag.mag_z;

    buffer[0] = SINGLE_MEASUREMENT_MODE_CMD;	
	ret=0;
        while(st480_i2c_transfer_data(st480->client, 1, buffer, 1)!=0)
        {
                ret++;
                msleep(1);
                if(st480_i2c_transfer_data(st480->client, 1, buffer, 1)==0)
                {
                        break;
                }
                if(ret > MAX_FAILURE_COUNT)
                {
                        break;
                }
        }
}
/*----------------------------------------------------------------------------*/
static int st480_ReadChipInfo(char *buf, int bufsize)
{
	if((!buf)||(bufsize<=30))
	{
		return -1;
	}
	if(!(st480->client))
	{
		*buf = 0;
		return -2;
	}

	sprintf(buf, "st480 Chip");
	return 0;
}

/*----------------------------------------------------------------------------*/
static int st480_ReadPostureData(char *buf, int bufsize)
{
	if((!buf)||(bufsize<=80))
	{
		return -1;
	}
	
	read_lock(&st480sensordata.datalock);
	sprintf(buf, "%d %d %d %d", st480sensordata.yaw, st480sensordata.pitch,
		st480sensordata.roll, st480sensordata.mag_status);
	read_unlock(&st480sensordata.datalock);
	return 0;
}

/*----------------------------------------------------------------------------*/
static int st480_ReadCaliData(char *buf, int bufsize)
{
	if((!buf)||(bufsize<=80))
	{
		return -1;
	}
	
	read_lock(&st480sensordata.datalock);
	sprintf(buf, "%d %d %d %d ", st480sensordata.nmx, st480sensordata.nmy, 
		st480sensordata.nmz,st480sensordata.mag_status);
	read_unlock(&st480sensordata.datalock);
	return 0;
}


/*----------------------------------------------------------------------------*/
static ssize_t show_daemon_name(struct device_driver *ddri, char *buf)
{
	char strbuf[256];
	sprintf(strbuf, "st480d");
	return sprintf(buf, "%s", strbuf);		
}


static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	char strbuf[256];
	st480_ReadChipInfo(strbuf, 256);
	return sprintf(buf, "%s\n", strbuf);        
}

/*----------------------------------------------------------------------------*/
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
	char strbuf[ST480_BUFSIZE];
	st480_work_func();
	sprintf(strbuf, "%04x %04x %04x", mag.mag_x, mag.mag_y, mag.mag_z);
	//st480_ReadSensorData(strbuf, ST480_BUFSIZE);
	return sprintf(buf, "%s\n", strbuf);
}


/*----------------------------------------------------------------------------*/
static ssize_t show_posturedata_value(struct device_driver *ddri, char *buf)
{
	char strbuf[ST480_BUFSIZE];
	st480_ReadPostureData(strbuf, ST480_BUFSIZE);
	return sprintf(buf, "%s\n", strbuf);            
}
/*----------------------------------------------------------------------------*/
static ssize_t show_calidata_value(struct device_driver *ddri, char *buf)
{
	char strbuf[ST480_BUFSIZE];
	st480_ReadCaliData(strbuf, ST480_BUFSIZE);
	return sprintf(buf, "%s\n", strbuf);            
}


static ssize_t  show_sensordata_calidata(struct device_driver *ddri, char *buf)
{
	int tmp[3];
	char strbuf[ST480_BUFSIZE];
	
	read_lock(&st480sensordata.datalock);
	sprintf(strbuf, "%d %d %d\n", st480sensordata.nmx, st480sensordata.nmy, 
		st480sensordata.nmz);
	read_unlock(&st480sensordata.datalock);
	return sprintf(buf, "%s\n", strbuf);;           
}


/*----------------------------------------------------------------------------*/
static ssize_t show_layout_value(struct device_driver *ddri, char *buf)
{
	struct st480_data *data = st480;

	return sprintf(buf, "(%d, %d)\n[%+2d %+2d %+2d]\n[%+2d %+2d %+2d]\n",
		data->hw->direction,atomic_read(&data->layout),	data->cvt.sign[0], data->cvt.sign[1],
		data->cvt.sign[2],data->cvt.map[0], data->cvt.map[1], data->cvt.map[2]);            
}
/*----------------------------------------------------------------------------*/
static ssize_t store_layout_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct st480_data *data = st480;
	int layout = 0;

	if(1 == sscanf(buf, "%d", &layout))
	{
		atomic_set(&data->layout, layout);
		if(!hwmsen_get_convert(layout, &data->cvt))
		{
			MSE_ERR("HWMSEN_GET_CONVERT function OK!\r\n");
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
	struct st480_data *data = st480;
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
	
	len += snprintf(buf+len, PAGE_SIZE-len, "OPEN: %d\n", atomic_read(&dev_open_count));
	return len;
}

/*----------------------------------------------------------------------------*/
static ssize_t show_trace_value(struct device_driver *ddri, char *buf)
{
	ssize_t res;
		struct st480_data *obj = st480;
	
	if(NULL == obj)
	{
		MSE_ERR("ST480_i2c_data is null!!\n");
		return 0;
	}	
	
	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));     
	return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct st480_data *obj = st480;
	int trace;
	if(NULL == obj )
	{
		MSE_ERR("st480_i2c_data is null!!\n");
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


/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(daemon,      S_IRUGO, show_daemon_name, NULL);
static DRIVER_ATTR(chipinfo,    S_IRUGO, show_chipinfo_value, NULL);
static DRIVER_ATTR(sensordata,  S_IRUGO, show_sensordata_value, NULL);
static DRIVER_ATTR(posturedata, S_IRUGO, show_posturedata_value, NULL);
static DRIVER_ATTR(calidata,    S_IRUGO, show_calidata_value, NULL);
static DRIVER_ATTR(layout,      S_IRUGO | S_IWUSR, show_layout_value, store_layout_value );
static DRIVER_ATTR(status,      S_IRUGO, show_status_value, NULL);
static DRIVER_ATTR(trace,       S_IRUGO | S_IWUSR, show_trace_value, store_trace_value );
static DRIVER_ATTR(calidata1,  S_IRUGO, show_sensordata_calidata, NULL);
/*----------------------------------------------------------------------------*/
static struct driver_attribute *st480_attr_list[] = {
    &driver_attr_daemon,
	&driver_attr_chipinfo,
	&driver_attr_sensordata,
	&driver_attr_posturedata,
	&driver_attr_calidata,
	&driver_attr_layout,
	&driver_attr_status,
	&driver_attr_trace,
	&driver_attr_calidata1
};
/*----------------------------------------------------------------------------*/
static int st480_create_attr(struct device_driver *driver) 
{
	int idx, err = 0;
	int num = (int)(sizeof(st480_attr_list)/sizeof(st480_attr_list[0]));
	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if((err = driver_create_file(driver, st480_attr_list[idx])))
		{            
			MSE_ERR("driver_create_file (%s) = %d\n", st480_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}
/*----------------------------------------------------------------------------*/
static int st480_delete_attr(struct device_driver *driver)
{
	int idx ,err = 0;
	int num = (int)(sizeof(st480_attr_list)/sizeof(st480_attr_list[0]));

	if(driver == NULL)
	{
		return -EINVAL;
	}
	

	for(idx = 0; idx < num; idx++)
	{
		driver_remove_file(driver, st480_attr_list[idx]);
	}
	

	return err;
}


static int st480d_open(struct inode *inode, struct file *file)
{
	MSE_FUN();
	return nonseekable_open(inode, file);
}

static int st480d_release(struct inode *inode, struct file *file)
{
	MSE_FUN();
	return 0;
}

static long st480d_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int i;
	int status=0;
	int delay;
	int calidata[4];
	int valuebuf[4];
	int retval=0;
	int enable = 0;
	char buff[20];
	
	switch (cmd) {

		case MSENSOR_IOCTL_SENSOR_ENABLE:
			if (argp == NULL)
			{
				MSE_ERR("IO parameter pointer is NULL!\r\n");
				break;
			}
			if(copy_from_user(&enable, argp, sizeof(enable)))
			{
				MSE_ERR("copy_from_user failed.");
				return -EFAULT;
			}
			else
			{
			    printk( "MSENSOR_IOCTL_SENSOR_ENABLE enable=%d!\r\n",enable);
				read_lock(&st480sensordata.ctrllock);
				if(enable == 1)
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
				read_unlock(&st480sensordata.ctrllock);
			}
			
			break;
			
		case MSENSOR_IOCTL_READ_SENSORDATA:	
			st480_work_func();
			if(copy_to_user(argp, (void *)&mag,sizeof(mag))!=0)
            {
            	printk("copy to user error.\n");
            	retval = -EFAULT;
				goto err_out;
				
            }
            break;
			
		//add by sen.luo
              case CKT_MSENSOR_IOCTL_READ_FACTORY_SENSORDATA:			
			st480_work_func();
			sprintf(buff, "%04x %04x %04x", mag.mag_x, mag.mag_y, mag.mag_z);
			if(copy_to_user(argp, buff, strlen(buff)+1))
            {
            	printk("copy to user error.\n");
            	retval = -EFAULT;
				goto err_out;			
            }
            break;
		//end
	
		case MSENSOR_IOCTL_SET_POSTURE:
			if(argp == NULL)
			{
				MSE_ERR("IO parameter pointer is NULL!\r\n");
				break;
			}
			   
			if(copy_from_user(&valuebuf, argp, sizeof(valuebuf)))
			{
				retval = -EFAULT;
				goto err_out;
			}
			
			write_lock(&st480sensordata.datalock);
			st480sensordata.yaw   = valuebuf[0];
			st480sensordata.pitch = valuebuf[1];
			st480sensordata.roll  = valuebuf[2];
			st480sensordata.mag_status = valuebuf[3];
			write_unlock(&st480sensordata.datalock);    
			break;

		case MSENSOR_IOCTL_SET_CALIDATA:
			//argp = (void __user *) arg;
			if (argp == NULL)
			{
				MSE_ERR("IO parameter pointer is NULL!\r\n");
				break;
			}
			if(copy_from_user(&calidata, argp, sizeof(calidata)))
			{
				retval = -EFAULT;
				goto err_out;	
			}
			
			write_lock(&st480sensordata.datalock);            
			st480sensordata.nmx = calidata[0];
			st480sensordata.nmy = calidata[1];
			st480sensordata.nmz = calidata[2];
			
			st480sensordata.mag_status = calidata[3];
			write_unlock(&st480sensordata.datalock);    
			break; 

		case IOCTL_SENSOR_GET_COMPASS_FLAG:
			status = st480_GetOpenStatus();			
			if(copy_to_user(argp, &status, sizeof(status)))
			{
				MSE_LOG("copy_to_user failed.");
				retval = -EFAULT;
				goto err_out;
			}
			break;
			
 
		case IOCTL_SENSOR_GET_COMPASS_DELAY:
			if(copy_to_user(argp, (void *)&st480d_delay, sizeof(st480d_delay))!=0)
                        {
                                printk("copy to user error.\n");
                                retval = -EFAULT;
								goto err_out;
                        }
			break;
		}
	return 0; 

	err_out:
	return retval;  
}

static struct file_operations st480d_fops = {
	.open = st480d_open,
	.release = st480d_release,
	.unlocked_ioctl = st480d_ioctl,
};

static struct miscdevice st480d_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "msensor",  
	.fops = &st480d_fops,
};



int st480_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	hwm_sensor_data* msensor_data;

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
				if(value < 10)
					value = 10;
				st480d_delay = value;
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
				read_lock(&st480sensordata.ctrllock);

				if(value == 1)
				{
					atomic_set(&m_flag, 1);
					atomic_set(&open_flag, 1);
				}
				else
				{
					atomic_set(&m_flag, 0);
					if(atomic_read(&o_flag ) == 0)
					{
						atomic_set(&open_flag, 0);
					}
				}	
				wake_up(&open_wq);
				read_unlock(&st480sensordata.ctrllock);
				// TODO: turn device into standby or normal mode
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
				msensor_data = (hwm_sensor_data *)buff_out;
				
				msensor_data->values[0] = st480sensordata.nmx;
				msensor_data->values[1] = st480sensordata.nmy;
				msensor_data->values[2] = st480sensordata.nmz;
				msensor_data->status = SENSOR_STATUS_ACCURACY_HIGH;
				msensor_data->value_divide = 1000;
			}
			break;
		default:
			printk(KERN_ERR "st480 operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}
	
	return err;
}

int st480_orientation_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	hwm_sensor_data* osensor_data;

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
				if(value < 10)
					value = 10;
				st480d_delay = value;
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
				read_lock(&st480sensordata.ctrllock);
				if(value == 1)
				{
					atomic_set(&o_flag , 1);
					atomic_set(&open_flag, 1);
				}
				else
				{
					atomic_set(&o_flag , 0);
					if(atomic_read(&m_flag) == 0)
					{
						atomic_set(&open_flag, 0);
					}									
				}
				wake_up(&open_wq);
				read_unlock(&st480sensordata.ctrllock);
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
				osensor_data = (hwm_sensor_data *)buff_out;
				
				osensor_data->values[0] = st480sensordata.yaw;
				osensor_data->values[1] = st480sensordata.pitch;
				osensor_data->values[2] = st480sensordata.roll;
				osensor_data->status = SENSOR_STATUS_ACCURACY_HIGH;
				osensor_data->value_divide = 1000;
			}
			break;
		default:
			printk(KERN_ERR "st480d operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}
	
	return err;
}


#if SENSOR_AUTO_TEST
static int sensor_test_read(void)
{
        st480_work_func();
        return 0;
}

static int auto_test_read(void *unused)
{
        while(1){
                sensor_test_read();
                msleep(200);
        }
        return 0;
}
#endif

static int st480_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err = 0;
	struct hwmsen_object sobj_m, sobj_o;

	MSE_FUN();

#if SENSOR_AUTO_TEST
        struct task_struct *thread;
#endif

	
	/* Allocate memory for driver data */
	st480 = kzalloc(sizeof(struct st480_data), GFP_KERNEL);
	if (!st480) {
		printk(KERN_ERR "SENODIA st480_i2c_probe: memory allocation failed.\n");
		err = -ENOMEM;
		goto exit1;
	}
	
	memset(st480, 0, sizeof(struct st480_data));

	st480->hw = st480_get_cust_mag_hw();
	if((err = hwmsen_get_convert(st480->hw->direction, &st480->cvt)))
	{
		MSE_ERR("invalid direction: %d\n", st480->hw->direction);
		goto exit1;
	}
	
	atomic_set(&st480->layout, st480->hw->direction);
	atomic_set(&st480->trace, 0);
	init_waitqueue_head(&open_wq);

	st480->client = client;
	i2c_set_clientdata(client, st480);

	if(client_init(st480->client) != 0)
    {
        printk("st480 setup error!\n");
		goto exit2;
    }

//	if((err = st480_create_attr(&st480_sensor_driver.driver)))
//	{
//		MSE_ERR("create attribute err = %d\n", err);
//		goto exit3;
//	}
	if(err = st480_create_attr(&(st480_init_info.platform_diver_addr->driver)))
	{
		MSE_ERR("create attribute err = %d\n", err);
		goto exit3;
	}

	err = misc_register(&st480d_device);
	if (err) {
		printk(KERN_ERR
			   "SENODIA st480_i2c_probe: st480d device register failed\n");
		goto exit3;
	}

		
	sobj_m.self = st480;
	sobj_m.polling = 1;
	sobj_m.sensor_operate = st480_operate;
	if(err = hwmsen_attach(ID_MAGNETIC, &sobj_m))
	{
		printk(KERN_ERR "st480 magnetic attach fail = %d\n", err);
		goto exit5;
	}

	sobj_o.self = st480;
	sobj_o.polling = 1;
	sobj_o.sensor_operate = st480_orientation_operate;
	if(err = hwmsen_attach(ID_ORIENTATION, &sobj_o))
	{
		printk(KERN_ERR "attach fail = %d\n", err);
		goto exit6;
	}
	
	/* As default, report all information */
	atomic_set(&m_flag, 0);
	atomic_set(&o_flag , 0);
	atomic_set(&open_flag, 0);

#if SENSOR_AUTO_TEST
	thread=kthread_run(auto_test_read,NULL,"st480_read_test");
#endif

	printk("st480 compass probed successfully.");
	st480_init_flag = 0;
	return 0;

exit6:	
exit5:
	
exit4:
	misc_deregister(&st480d_device);
exit3:
exit2:
	kfree(st480);
exit1:
exit0:
st480_init_flag = -1;
	return err;
	
}

static int st480_i2c_remove(struct i2c_client *client)
{
	int err;	
	MSE_FUN();	
//	if((err = st480_delete_attr(&st480_sensor_driver.driver)))
//	{
//		MSE_ERR("st480_delete_attr fail: %d\n", err);
//	}	
	if(err = st480_delete_attr(&(st480_init_info.platform_diver_addr->driver)))
	{
		MSE_ERR("st480_delete_attr fail: %d\n", err);
	}
	i2c_unregister_device(st480->client);
	kfree(i2c_get_clientdata(st480->client));	
	misc_deregister(&st480d_device); 
	
	st480 = NULL;

	MSE_LOG("st480 i2c successfully removed."); 
	return 0;
}

static int st480_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) 
{    
	strcpy(info->type, ST480_I2C_NAME);
	return 0;
}

static const struct i2c_device_id st480_i2c_id[] = {{ST480_I2C_NAME,0},{}};

static struct i2c_driver st480_i2c_driver = {
    .driver = {
        .name  = ST480_I2C_NAME,
    },
	.probe      = st480_i2c_probe,
	.remove     = st480_i2c_remove,
	//.detect     = st480_i2c_detect,
	.id_table = st480_i2c_id,
};

static int st480_probe(struct platform_device *pdev) 
{
	struct mag_hw *hw = st480_get_cust_mag_hw();

	MSE_FUN();

	st480_power(hw, 1); 

	rwlock_init(&st480sensordata.ctrllock);
	rwlock_init(&st480sensordata.datalock);

	atomic_set(&dev_open_count, 0);
	
	if(i2c_add_driver(&st480_i2c_driver))
	{
		printk(KERN_ERR "add driver error\n");
		return -1;
	} 
	return 0;
}

static int st480_remove(struct platform_device *pdev)
{ 
	atomic_set(&dev_open_count, 0);  
	i2c_del_driver(&st480_i2c_driver);
	return 0;
}

static struct platform_driver st480_sensor_driver = {
	.probe      = st480_probe,
	.remove     = st480_remove,    
	.driver     = {
		.name  = "msensor",
	}
};


static int	st480_local_init(void)
{
	struct mag_hw *hw = st480_get_cust_mag_hw();
     MSE_FUN();
	
	st480_power(hw, 1);
	atomic_set(&dev_open_count, 0);
	//mmc328x_force[0] = hw->i2c_num;

	if(i2c_add_driver(&st480_i2c_driver))
	{
		printk(KERN_ERR "add driver error\n");
		return -1;
	} 
	if(-1 == st480_init_flag)
	{
	   return -1;
	}
	return 0;
}

/*----------------------------------------------------------------------------*/
static int st_remove(void)
{
	struct mag_hw *hw = st480_get_cust_mag_hw();
	st480_power(hw, 0);    
	atomic_set(&dev_open_count, 0);  
	i2c_del_driver(&st480_i2c_driver);
	return 0;
}


static int __init st480_init(void)
{

	MSE_FUN();
	struct mag_hw *hw = st480_get_cust_mag_hw();
	MSE_LOG("%s: i2c_number=%d\n", __func__,hw->i2c_num); 
		//hwmsen_msensor_add(&st480_init_info);
	i2c_register_board_info(0, &i2c_st480, 1);
	mag_driver_add(&st480_init_info);
//	i2c_register_board_info(hw->i2c_num, &i2c_st480, 1);

//	if(platform_driver_register(&st480_sensor_driver))
//	{
//		printk(KERN_ERR "failed to register driver");
//		return -ENODEV;
//	}
	return 0; 
}

static void __exit st480_exit(void)
{
//	platform_driver_unregister(&st480_sensor_driver);
}

module_init(st480_init);
module_exit(st480_exit);

MODULE_AUTHOR("MTK");
MODULE_DESCRIPTION("Senodia ST480 linux driver for MTK");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");
