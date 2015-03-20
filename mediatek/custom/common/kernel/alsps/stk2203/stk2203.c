/* drivers/hwmon/mt65xx/amit/stk2203.c - stk2203 ALS only driver
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
#include <linux/hwmsen_helper.h>
#include <asm/io.h>
#include <cust_eint.h>
#include <cust_alsps.h>
#include "stk2203.h"

//#include <mach/mt_devs.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>

/*-------------------------MT65xx define-------------------------------*/

#define POWER_NONE_MACRO MT65XX_POWER_NONE


/* TEMPLATE */
//#define GPIO_ALS_EINT_PIN         GPIO190
#define GPIO_ALS_EINT_PIN_M_GPIO  GPIO_MODE_00
#define GPIO_ALS_EINT_PIN_M_EINT  GPIO_MODE_01
//#define GPIO_ALS_EINT_PIN_M_PWM  GPIO_MODE_04
//#define CUST_EINT_ALS_NUM              3
#define CUST_EINT_ALS_DEBOUNCE_CN      0
#define CUST_EINT_ALS_POLARITY         CUST_EINT_POLARITY_LOW
#define CUST_EINT_ALS_SENSITIVE        CUST_EINT_LEVEL_SENSITIVE
#define CUST_EINT_ALS_DEBOUNCE_EN      CUST_EINT_DEBOUNCE_DISABLE



/******************************************************************************
 * configuration
*******************************************************************************/
#define STK2203_NO_SUPPORT_PS

/*----------------------------------------------------------------------------*/
#define STK2203_DEV_NAME     "STK2203"
/*----------------------------------------------------------------------------*/
#define APS_TAG                  "[ALS/PS] "
#define APS_FUN(f)               printk(KERN_INFO APS_TAG"%s\n", __FUNCTION__)
#define APS_ERR(fmt, args...)    printk(KERN_ERR  APS_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define APS_LOG(fmt, args...)    printk(KERN_INFO APS_TAG fmt, ##args)
#define APS_DBG(fmt, args...)    printk(KERN_INFO fmt, ##args)                 
/******************************************************************************
 * extern functions
*******************************************************************************/
static struct i2c_client *stk2203_i2c_client = NULL;
/*----------------------------------------------------------------------------*/
static const struct i2c_device_id stk2203_i2c_id[] = {{STK2203_DEV_NAME,0},{}};
static struct i2c_board_info __initdata i2c_stk2203={ I2C_BOARD_INFO("STK2203", (0x20>>1))};
/*the adapter id & i2c address will be available in customization*/
//static unsigned short stk2203_force[] = {0x00, 0x00, I2C_CLIENT_END, I2C_CLIENT_END};
//static const unsigned short *const stk2203_forces[] = { stk2203_force, NULL };
//static struct i2c_client_address_data stk2203_addr_data = { .forces = stk2203_forces,};
/*----------------------------------------------------------------------------*/
static int stk2203_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id); 
static int stk2203_i2c_remove(struct i2c_client *client);
//static int stk2203_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
/*----------------------------------------------------------------------------*/
static int stk2203_i2c_suspend(struct i2c_client *client, pm_message_t msg);
static int stk2203_i2c_resume(struct i2c_client *client);

static struct stk2203_priv *g_stk2203_ptr = NULL;
/*----------------------------------------------------------------------------*/
typedef enum {
    STK_TRC_ALS_DATA= 0x0001,
    STK_TRC_PS_DATA = 0x0002,
    STK_TRC_EINT    = 0x0004,
    STK_TRC_IOCTL   = 0x0008,
    STK_TRC_I2C     = 0x0010,
    STK_TRC_CVT_ALS = 0x0020,
    STK_TRC_CVT_PS  = 0x0040,
    STK_TRC_DEBUG   = 0x8000,
} STK_TRC;
/*----------------------------------------------------------------------------*/
typedef enum {
    STK_BIT_ALS    = 1,
    STK_BIT_PS     = 2,
} STK_BIT;
/*----------------------------------------------------------------------------*/
struct stk2203_i2c_addr {    /*define a series of i2c slave address*/
    u8  rar;           /*Alert Response Address*/
    u8  init;           /*device initialization */
    u8  als_cmd;   /*ALS command*/
    u8  als_dat1;   /*ALS MSB*/
    u8  als_dat0;   /*ALS LSB*/
    u8  als_intr;    /*ALS INT*/    
    u8  ps_cmd;    /*PS command*/
    u8  ps_dat;      /*PS data*/
    u8  ps_thd;     /*PS INT threshold*/
};
/*----------------------------------------------------------------------------*/
struct stk2203_priv {
    struct alsps_hw  *hw;
    struct i2c_client *client;
    struct delayed_work  eint_work;

    /*i2c address group*/
    struct stk2203_i2c_addr  addr;
    
    /*misc*/
    atomic_t    trace;
    atomic_t    i2c_retry;
    atomic_t    als_suspend;
    atomic_t    als_debounce;   /*debounce time after enabling als*/
    atomic_t    als_deb_on;     /*indicates if the debounce is on*/
    atomic_t    als_deb_end;    /*the jiffies representing the end of debounce*/
    atomic_t    ps_mask;        /*mask ps: always return far away*/
    atomic_t    ps_debounce;    /*debounce time after enabling ps*/
    atomic_t    ps_deb_on;      /*indicates if the debounce is on*/
    atomic_t    ps_deb_end;     /*the jiffies representing the end of debounce*/
    atomic_t    ps_suspend;


    /*data*/
    u16         als;
    u8          ps;
    u8          _align;
    u16         als_level_num;
    u16         als_value_num;
    u32         als_level[C_CUST_ALS_LEVEL-1];
    u32         als_value[C_CUST_ALS_LEVEL];

    atomic_t    als_cmd_val;   /*the cmd value can't be read, stored in ram*/
    atomic_t    als_intr_val;    /*the intr  value can't be read, stored in ram*/    
    atomic_t    ps_cmd_val;    /*the cmd value can't be read, stored in ram*/
    atomic_t    ps_thd_val;     /*the cmd value can't be read, stored in ram*/
    ulong       enable;              /*enable mask*/
    ulong       pending_intr;     /*pending interrupt*/

    /*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend    early_drv;
#endif     
};
/*----------------------------------------------------------------------------*/
static struct i2c_driver stk2203_i2c_driver = {	
	.probe      = stk2203_i2c_probe,
	.remove     = stk2203_i2c_remove,
//	.detect     = stk2203_i2c_detect,
	.suspend    = stk2203_i2c_suspend,
	.resume     = stk2203_i2c_resume,
	.id_table   = stk2203_i2c_id,
//	.address_list = &stk2203_forces,
	.driver = {
//		.owner          = THIS_MODULE,
		.name           = STK2203_DEV_NAME,
	},
};

static struct stk2203_priv *stk2203_obj = NULL;
static struct platform_driver stk2203_alsps_driver;
static int stk2203_get_ps_value(struct stk2203_priv *obj, u16 ps);
static int stk2203_get_als_value(struct stk2203_priv *obj, u16 als);
static int stk2203_read_als(struct i2c_client *client, u16 *data);
static int stk2203_read_ps(struct i2c_client *client, u8 *data);
/*----------------------------------------------------------------------------*/
int stk2203_get_addr(struct alsps_hw *hw, struct stk2203_i2c_addr *addr)
{
	if(!hw || !addr)
	{
		return -EFAULT;
	}
	addr->als_cmd   = ALS_CMD;         
	addr->als_dat1  = ALS_DT1;
	addr->als_dat0  = ALS_DT2;
	addr->als_intr   = ALS_INT;         
	addr->ps_cmd    = PS_CMD;
	addr->ps_thd    = PS_THDL;
	addr->ps_dat    = PS_DT;
	return 0;
}
/*----------------------------------------------------------------------------*/
int stk2203_get_timing(void)
{
return 200;
}
/*----------------------------------------------------------------------------*/
int stk2203_master_recv(struct i2c_client *client, u16 addr, u8 *buf ,int count)
{
	struct stk2203_priv *obj = i2c_get_clientdata(client);        
	//struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg = {};
	int ret = 0, retry = 0;
	int trc = atomic_read(&obj->trace);
	int max_try = atomic_read(&obj->i2c_retry);

	while(retry++ < max_try)
	{
		ret = hwmsen_read_block(client, addr, buf, count);
		if(ret == 0)
            break;
		udelay(100);
	}

	if(unlikely(trc))
	{
		if(trc & STK_TRC_I2C)
		{
			APS_LOG("(recv) %x %d %d %p [%02X]\n", msg.addr, msg.flags, msg.len, msg.buf, msg.buf[0]);    
		}

		if((retry != 1) && (trc & STK_TRC_DEBUG))
		{
			APS_LOG("(recv) %d/%d\n", retry-1, max_try); 

		}
	}

	/* If everything went ok (i.e. 1 msg transmitted), return #bytes
	transmitted, else error code. */
	return (ret == 0) ? count : ret;
}
/*----------------------------------------------------------------------------*/
int stk2203_master_send(struct i2c_client *client, u16 addr, u8 *buf ,int count)
{
	int ret = 0, retry = 0;
	struct stk2203_priv *obj = i2c_get_clientdata(client);        
	//struct i2c_adapter *adap=client->adapter;
	struct i2c_msg msg = {};
	int trc = atomic_read(&obj->trace);
	int max_try = atomic_read(&obj->i2c_retry);


	while(retry++ < max_try)
	{
		ret = hwmsen_write_block(client, addr, buf, count);
		if (ret == 0)
		    break;
		udelay(100);
	}

	if(unlikely(trc))
	{
		if(trc & STK_TRC_I2C)
		{
			APS_LOG("(send) %x %d %d %p [%02X]\n", msg.addr, msg.flags, msg.len, msg.buf, msg.buf[0]);    
		}

		if((retry != 1) && (trc & STK_TRC_DEBUG))
		{
			APS_LOG("(send) %d/%d\n", retry-1, max_try);
		}
	}
	/* If everything went ok (i.e. 1 msg transmitted), return #bytes
	transmitted, else error code. */
	return (ret == 0) ? count : ret;
}
/*----------------------------------------------------------------------------*/
int stk2203_read_als(struct i2c_client *client, u16 *data)
{
	struct stk2203_priv *obj = i2c_get_clientdata(client);    
	int ret = 0;
	u8 buf[2];

	if(1 != (ret = stk2203_master_recv(client, obj->addr.als_dat1, (char*)&buf[1], 1)))
	{
		APS_ERR("reads als data1 = %d\n", ret);
		return -EFAULT;
	}
	else if(1 != (ret = stk2203_master_recv(client, obj->addr.als_dat0, (char*)&buf[0], 1)))
	{
		APS_ERR("reads als data2 = %d\n", ret);
		return -EFAULT;
	}
	
	*data = (buf[1] << 4)|(buf[0] >>4);

	
	if(atomic_read(&obj->trace) & STK_TRC_ALS_DATA)
	{
		APS_DBG("ALS: 0x%04X\n", (u32)(*data));
	}
 
	return 0;    
}
/*----------------------------------------------------------------------------*/
int stk2203_write_als(struct i2c_client *client, u8 data)
{
	struct stk2203_priv *obj = i2c_get_clientdata(client);
	int ret = 0;
    
    ret = stk2203_master_send(client, obj->addr.als_cmd, &data, 1);
	if(ret < 0)
	{
		APS_ERR("write als = %d\n", ret);
		return -EFAULT;
	}
	
	return 0;    
}
/*----------------------------------------------------------------------------*/
int stk2203_write_als_intr(struct i2c_client *client, u8 data)
{
	struct stk2203_priv *obj = i2c_get_clientdata(client);
	int ret = 0;
    
    ret = stk2203_master_send(client, obj->addr.als_intr, &data, 1);
	if(ret < 0)
	{
		APS_ERR("write als intr= %d\n", ret);
		return -EFAULT;
	}
	
	return 0;    
}
/*----------------------------------------------------------------------------*/
int stk2203_read_ps(struct i2c_client *client, u8 *data)
{
	struct stk2203_priv *obj = i2c_get_clientdata(client);    
	int ret = 0;

	if(sizeof(*data) != (ret = stk2203_master_recv(client, obj->addr.ps_dat, (char*)data, sizeof(*data))))
	{
		APS_ERR("reads ps data = %d\n", ret);
		return -EFAULT;
	} 

	if(atomic_read(&obj->trace) & STK_TRC_PS_DATA)
	{
		APS_DBG("PS:  0x%04X\n", (u32)(*data));
	}

       #ifdef STK2203_NO_SUPPORT_PS
         *data = 0x00;
       #endif

	return 0;    
}
/*----------------------------------------------------------------------------*/
int stk2203_write_ps(struct i2c_client *client, u8 data)
{
	struct stk2203_priv *obj = i2c_get_clientdata(client);        
	int ret = 0;

    ret = stk2203_master_send(client, obj->addr.ps_cmd, &data, 1);
	if (ret < 0)
	{
		APS_ERR("write ps = %d\n", ret);
		return -EFAULT;
	} 
	return 0;    
}
/*----------------------------------------------------------------------------*/
int stk2203_write_ps_thd(struct i2c_client *client, u8 thd)
{
	struct stk2203_priv *obj = i2c_get_clientdata(client);        
	u8 buf = thd;
	int ret = 0;

	if(sizeof(buf) != (ret = stk2203_master_send(client, obj->addr.ps_thd, (char*)&buf, sizeof(buf))))
	{
		APS_ERR("write thd = %d\n", ret);
		return -EFAULT;
	} 
	return 0;    
}

/*----------------------------------------------------------------------------*/
static void stk2203_power(struct alsps_hw *hw, unsigned int on) 
{
	static unsigned int power_on = 0;

	//APS_LOG("power %s\n", on ? "on" : "off");

	if(hw->power_id != POWER_NONE_MACRO)
	{
		if(power_on == on)
		{
			APS_LOG("ignore power control: %d\n", on);
		}
		else if(on)
		{
			if(!hwPowerOn(hw->power_id, hw->power_vol, "stk2203")) 
			{
				APS_ERR("power on fails!!\n");
			}
		}
		else
		{
			if(!hwPowerDown(hw->power_id, "stk2203")) 
			{
				APS_ERR("power off fail!!\n");   
			}
		}
	}
	power_on = on;
}
/*----------------------------------------------------------------------------*/
static int stk2203_enable_als(struct i2c_client *client, int enable)
{
	struct stk2203_priv *obj = i2c_get_clientdata(client);
	int err, cur = 0, old = atomic_read(&obj->als_cmd_val);
	int trc = atomic_read(&obj->trace);

	if(enable)
	{
		cur = old & (~SD_ALS);   
	}
	else
	{
		cur = old | (SD_ALS); 
	}
	
	if(trc & STK_TRC_DEBUG)
	{
		APS_LOG("%s: %08X, %08X, %d\n", __func__, cur, old, enable);
	}
	
	if(0 == (cur ^ old))
	{
		return 0;
	}
	
	if(0 == (err = stk2203_write_als(client, cur))) 
	{
		atomic_set(&obj->als_cmd_val, cur);
	}
	
	if(enable)
	{
		atomic_set(&obj->als_deb_on, 1);
		atomic_set(&obj->als_deb_end, jiffies+atomic_read(&obj->als_debounce)/(1000/HZ));
		set_bit(STK_BIT_ALS,  &obj->pending_intr);
		schedule_delayed_work(&obj->eint_work,260); //after enable the value is not accurate
	}

	if(trc & STK_TRC_DEBUG)
	{
		APS_LOG("enable als (%d)\n", enable);
	}

	return err;
}
/*----------------------------------------------------------------------------*/
static int stk2203_enable_ps(struct i2c_client *client, int enable)
{
	struct stk2203_priv *obj = i2c_get_clientdata(client);
	int err, cur = 0, old = atomic_read(&obj->ps_cmd_val);
	int trc = atomic_read(&obj->trace);

	if(enable)
	{
		cur = old & (~SD_PS);   
	}
	else
	{
		cur = old | (SD_PS);
	}
	
	if(trc & STK_TRC_DEBUG)
	{
		APS_LOG("%s: %08X, %08X, %d\n", __func__, cur, old, enable);
	}
	
	if(0 == (cur ^ old))
	{
		return 0;
	}
	
	if(0 == (err = stk2203_write_ps(client, cur))) 
	{
		atomic_set(&obj->ps_cmd_val, cur);
	}
	
	if(enable)
	{
		atomic_set(&obj->ps_deb_on, 1);
		atomic_set(&obj->ps_deb_end, jiffies+atomic_read(&obj->ps_debounce)/(1000/HZ));
		set_bit(STK_BIT_PS,  &obj->pending_intr);
		schedule_delayed_work(&obj->eint_work,120);
	}

	if(trc & STK_TRC_DEBUG)
	{
		APS_LOG("enable ps  (%d)\n", enable);
	}

	return err;
}
/*----------------------------------------------------------------------------*/
static int stk2203_check_and_clear_intr(struct i2c_client *client) 
{
	struct stk2203_priv *obj = i2c_get_clientdata(client);
	int err;
	u8 status;

	if (mt_get_gpio_in(GPIO_ALS_EINT_PIN) == 1) /*skip if no interrupt*/  
	    return 0;

    err = stk2203_master_recv(client, obj->addr.rar, &status, 1);
	if (err < 0)
	{
		APS_ERR("WARNING: read status: %d\n", err);
		return 0;
	}
    
	if((status & 0x10) == (obj->addr.als_cmd ))
	{
		set_bit(STK_BIT_ALS, &obj->pending_intr);
	}
	else
	{
	   clear_bit(STK_BIT_ALS, &obj->pending_intr);
	}
	
	if((status & 0x20) == (obj->addr.ps_cmd ))
	{
		set_bit(STK_BIT_PS,  &obj->pending_intr);
	}
	else
	{
	    clear_bit(STK_BIT_PS, &obj->pending_intr);
	}
	
	if(atomic_read(&obj->trace) & STK_TRC_DEBUG)
	{
		APS_LOG("check intr: 0x%02X => 0x%08lX\n", status, obj->pending_intr);
	}

    status = 0;
    err = stk2203_master_send(client, obj->addr.rar, &status, 1);
	if (err < 0)
	{
		APS_ERR("WARNING: clear intrrupt: %d\n", err);
		return 0;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
void stk2203_eint_func(void)
{
	struct stk2203_priv *obj = g_stk2203_ptr;
	APS_LOG(" interrupt fuc\n");
	if(!obj)
	{
		return;
	}
	
	//schedule_work(&obj->eint_work);
	schedule_delayed_work(&obj->eint_work,0);
	if(atomic_read(&obj->trace) & STK_TRC_EINT)
	{
		APS_LOG("eint: als/ps intrs\n");
	}
}
/*----------------------------------------------------------------------------*/
static void stk2203_eint_work(struct work_struct *work)
{
	struct stk2203_priv *obj = g_stk2203_ptr;
	int err;
	hwm_sensor_data sensor_data;
	
	memset(&sensor_data, 0, sizeof(sensor_data));

	APS_LOG(" eint work\n");
	
	if((err = stk2203_check_and_clear_intr(obj->client)))
	{
		APS_ERR("check intrs: %d\n", err);
	}

    APS_LOG(" &obj->pending_intr =%lx\n",obj->pending_intr);
	
	if((1<<STK_BIT_ALS) & obj->pending_intr)
	{
	  //get raw data
	  APS_LOG(" als change\n");
	  if((err = stk2203_read_als(obj->client, &obj->als)))
	  {
		 APS_ERR("stk2203 read als data: %d\n", err);;
	  }
	  //map and store data to hwm_sensor_data
	 
 	  sensor_data.values[0] = stk2203_get_als_value(obj, obj->als);
	  sensor_data.value_divide = 1;
	  sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
	  APS_LOG("als raw %x -> value %d \n", obj->als,sensor_data.values[0]);
	  //let up layer to know
	  if((err = hwmsen_get_interrupt_data(ID_LIGHT, &sensor_data)))
	  {
		APS_ERR("call hwmsen_get_interrupt_data fail = %d\n", err);
	  }
	  
	}
	if((1<<STK_BIT_PS) &  obj->pending_intr)
	{
	  //get raw data
	  APS_LOG(" ps change\n");
	  if((err = stk2203_read_ps(obj->client, &obj->ps)))
	  {
		 APS_ERR("stk2203 read ps data: %d\n", err);
	  }
	  //map and store data to hwm_sensor_data
	  sensor_data.values[0] = stk2203_get_ps_value(obj, obj->ps);
	  sensor_data.value_divide = 1;
	  sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
	  //let up layer to know
	  if((err = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data)))
	  {
		APS_ERR("call hwmsen_get_interrupt_data fail = %d\n", err);
	  }
	}
}
/*----------------------------------------------------------------------------*/
int stk2203_setup_eint(struct i2c_client *client)
{
	struct stk2203_priv *obj = i2c_get_clientdata(client);        

	g_stk2203_ptr = obj;
	/*configure to GPIO function, external interrupt*/

	
	mt_set_gpio_mode(GPIO_ALS_EINT_PIN, GPIO_ALS_EINT_PIN_M_EINT);
       mt_set_gpio_dir(GPIO_ALS_EINT_PIN, GPIO_DIR_IN);
	mt_set_gpio_pull_enable(GPIO_ALS_EINT_PIN, GPIO_PULL_ENABLE);
	mt_set_gpio_pull_select(GPIO_ALS_EINT_PIN, GPIO_PULL_UP);
	
    return 0;
	
}
/*----------------------------------------------------------------------------*/
static int stk2203_init_client(struct i2c_client *client)
{
	struct stk2203_priv *obj = i2c_get_clientdata(client);
	int err;

	 //client->addr |= I2C_ENEXT_FLAG; //for EVB Borad

	if((err = stk2203_setup_eint(client)))
	{
		APS_ERR("setup eint: %d\n", err);
		return err;
	}
	if((err = stk2203_check_and_clear_intr(client)))
	{
		APS_ERR("check/clear intr: %d\n", err);
		//    return err;
	}
	
	if((err = stk2203_write_als(client, atomic_read(&obj->als_cmd_val))))
	{
		APS_ERR("write als: %d\n", err);
		return err;
	}

	if((err = stk2203_write_als_intr(client, atomic_read(&obj->als_intr_val))))
	{
		APS_ERR("write als intr: %d\n", err);
		return err;
	}
	
	if((err = stk2203_write_ps(client, atomic_read(&obj->ps_cmd_val))))
	{
		APS_ERR("write ps: %d\n", err);
		return err;        
	}
	
	if((err = stk2203_write_ps_thd(client, atomic_read(&obj->ps_thd_val))))
	{
		APS_ERR("write thd: %d\n", err);
		return err;        
	}
	return 0;
}
/******************************************************************************
 * Sysfs attributes
*******************************************************************************/
static ssize_t stk2203_show_config(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	
	if(!stk2203_obj)
	{
		APS_ERR("stk2203_obj is null!!\n");
		return 0;
	}
	
	res = snprintf(buf, PAGE_SIZE, "(%d %d %d %d %d)\n", 
		atomic_read(&stk2203_obj->i2c_retry), atomic_read(&stk2203_obj->als_debounce), 
		atomic_read(&stk2203_obj->ps_mask), atomic_read(&stk2203_obj->ps_thd_val), atomic_read(&stk2203_obj->ps_debounce));     
	return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t stk2203_store_config(struct device_driver *ddri, const char *buf, size_t count)
{
	int retry, als_deb, ps_deb, mask, thres;
	if(!stk2203_obj)
	{
		APS_ERR("stk2203_obj is null!!\n");
		return 0;
	}
	
	if(5 == sscanf(buf, "%d %d %d %d %d", &retry, &als_deb, &mask, &thres, &ps_deb))
	{ 
		atomic_set(&stk2203_obj->i2c_retry, retry);
		atomic_set(&stk2203_obj->als_debounce, als_deb);
		atomic_set(&stk2203_obj->ps_mask, mask);
		atomic_set(&stk2203_obj->ps_thd_val, thres);        
		atomic_set(&stk2203_obj->ps_debounce, ps_deb);
	}
	else
	{
		APS_ERR("invalid content: '%s', length = %d\n", buf, count);
	}
	return count;    
}
/*----------------------------------------------------------------------------*/
static ssize_t stk2203_show_trace(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	if(!stk2203_obj)
	{
		APS_ERR("stk2203_obj is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&stk2203_obj->trace));     
	return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t stk2203_store_trace(struct device_driver *ddri, const char *buf, size_t count)
{
    int trace;
    if(!stk2203_obj)
	{
		APS_ERR("stk2203_obj is null!!\n");
		return 0;
	}
	
	if(1 == sscanf(buf, "0x%x", &trace))
	{
		atomic_set(&stk2203_obj->trace, trace);
	}
	else 
	{
		APS_ERR("invalid content: '%s', length = %d\n", buf, count);
	}
	return count;    
}
/*----------------------------------------------------------------------------*/
static ssize_t stk2203_show_als(struct device_driver *ddri, char *buf)
{
	int res;
	
	if(!stk2203_obj)
	{
		APS_ERR("stk2203_obj is null!!\n");
		return 0;
	}
	if((res = stk2203_read_als(stk2203_obj->client, &stk2203_obj->als)))
	{
		return snprintf(buf, PAGE_SIZE, "ERROR: %d\n", res);
	}
	else
	{
		return snprintf(buf, PAGE_SIZE, "0x%04X\n", stk2203_obj->als);     
	}
}
/*----------------------------------------------------------------------------*/
static ssize_t stk2203_show_ps(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	if(!stk2203_obj)
	{
		APS_ERR("stk2203_obj is null!!\n");
		return 0;
	}
	
	if((res = stk2203_read_ps(stk2203_obj->client, &stk2203_obj->ps)))
	{
		return snprintf(buf, PAGE_SIZE, "ERROR: %d\n", res);
	}
	else
	{
		return snprintf(buf, PAGE_SIZE, "0x%04X\n", stk2203_obj->ps);     
	}
}
/*----------------------------------------------------------------------------*/
static ssize_t stk2203_show_reg(struct device_driver *ddri, char *buf)
{
	if(!stk2203_obj)
	{
		APS_ERR("stk2203_obj is null!!\n");
		return 0;
	}
	
	/*read*/
	stk2203_check_and_clear_intr(stk2203_obj->client);
	stk2203_read_ps(stk2203_obj->client, &stk2203_obj->ps);
	stk2203_read_als(stk2203_obj->client, &stk2203_obj->als);
	/*write*/
	stk2203_write_als(stk2203_obj->client, atomic_read(&stk2203_obj->als_cmd_val));
	stk2203_write_als_intr(stk2203_obj->client, atomic_read(&stk2203_obj->als_intr_val));
	stk2203_write_ps(stk2203_obj->client, atomic_read(&stk2203_obj->ps_cmd_val)); 
	stk2203_write_ps_thd(stk2203_obj->client, atomic_read(&stk2203_obj->ps_thd_val));
	return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t stk2203_show_send(struct device_driver *ddri, char *buf)
{
    return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t stk2203_store_send(struct device_driver *ddri, const char *buf, size_t count)
{
	int addr, cmd;
	u8 dat;

	if(!stk2203_obj)
	{
		APS_ERR("stk2203_obj is null!!\n");
		return 0;
	}
	else if(2 != sscanf(buf, "%x %x", &addr, &cmd))
	{
		APS_ERR("invalid format: '%s'\n", buf);
		return 0;
	}

	dat = (u8)cmd;
	APS_LOG("send(%02X, %02X) = %d\n", addr, cmd, 
	stk2203_master_send(stk2203_obj->client, (u16)addr, &dat, sizeof(dat)));
	
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t stk2203_show_recv(struct device_driver *ddri, char *buf)
{
    return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t stk2203_store_recv(struct device_driver *ddri, const char *buf, size_t count)
{
	int addr;
	u8 dat;
	if(!stk2203_obj)
	{
		APS_ERR("stk2203_obj is null!!\n");
		return 0;
	}
	else if(1 != sscanf(buf, "%x", &addr))
	{
		APS_ERR("invalid format: '%s'\n", buf);
		return 0;
	}

	APS_LOG("recv(%02X) = %d, 0x%02X\n", addr, 
	stk2203_master_recv(stk2203_obj->client, (u16)addr, (char*)&dat, sizeof(dat)), dat);
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t stk2203_show_status(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	
	if(!stk2203_obj)
	{
		APS_ERR("stk2203_obj is null!!\n");
		return 0;
	}
	
	if(stk2203_obj->hw)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d, (%d %d) (%02X %02X) (%02X %02X %02X) (%02X %02X %02X)\n", 
			stk2203_obj->hw->i2c_num, stk2203_obj->hw->power_id, stk2203_obj->hw->power_vol, stk2203_obj->addr.init, 
			stk2203_obj->addr.rar,stk2203_obj->addr.als_cmd, stk2203_obj->addr.als_dat0, stk2203_obj->addr.als_dat1,
			stk2203_obj->addr.ps_cmd, stk2203_obj->addr.ps_dat, stk2203_obj->addr.ps_thd);
	}
	else
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");
	}
	
	len += snprintf(buf+len, PAGE_SIZE-len, "REGS: %02X %02X %02X %02lX %02lX\n", 
				atomic_read(&stk2203_obj->als_cmd_val), atomic_read(&stk2203_obj->ps_cmd_val), 
				atomic_read(&stk2203_obj->ps_thd_val),stk2203_obj->enable, stk2203_obj->pending_intr);

	len += snprintf(buf+len, PAGE_SIZE-len, "MISC: %d %d\n", atomic_read(&stk2203_obj->als_suspend), atomic_read(&stk2203_obj->ps_suspend));

	return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t stk2203_show_i2c(struct device_driver *ddri, char *buf)
{
	return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t stk2203_store_i2c(struct device_driver *ddri, const char *buf, size_t count)
{
       return 0;
}
/*----------------------------------------------------------------------------*/
#define IS_SPACE(CH) (((CH) == ' ') || ((CH) == '\n'))
/*----------------------------------------------------------------------------*/
static int read_int_from_buf(struct stk2203_priv *obj, const char* buf, size_t count,
                             u32 data[], int len)
{
	int idx = 0;
	char *cur = (char*)buf, *end = (char*)(buf+count);

	while(idx < len)
	{
		while((cur < end) && IS_SPACE(*cur))
		{
			cur++;        
		}

		if(1 != sscanf(cur, "%d", &data[idx]))
		{
			break;
		}

		idx++; 
		while((cur < end) && !IS_SPACE(*cur))
		{
			cur++;
		}
	}
	return idx;
}
/*----------------------------------------------------------------------------*/
static ssize_t stk2203_show_alslv(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	int idx;
	if(!stk2203_obj)
	{
		APS_ERR("stk2203_obj is null!!\n");
		return 0;
	}
	
	for(idx = 0; idx < stk2203_obj->als_level_num; idx++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "%d ", stk2203_obj->hw->als_level[idx]);
	}
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	return len;    
}
/*----------------------------------------------------------------------------*/
static ssize_t stk2203_store_alslv(struct device_driver *ddri, const char *buf, size_t count)
{
//	struct stk2203_priv *obj;
	if(!stk2203_obj)
	{
		APS_ERR("stk2203_obj is null!!\n");
		return 0;
	}
	else if(!strcmp(buf, "def"))
	{
		memcpy(stk2203_obj->als_level, stk2203_obj->hw->als_level, sizeof(stk2203_obj->als_level));
	}
	else if(stk2203_obj->als_level_num != read_int_from_buf(stk2203_obj, buf, count, 
			stk2203_obj->hw->als_level, stk2203_obj->als_level_num))
	{
		APS_ERR("invalid format: '%s'\n", buf);
	}    
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t stk2203_show_alsval(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	int idx;
	if(!stk2203_obj)
	{
		APS_ERR("stk2203_obj is null!!\n");
		return 0;
	}
	
	for(idx = 0; idx < stk2203_obj->als_value_num; idx++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "%d ", stk2203_obj->hw->als_value[idx]);
	}
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	return len;    
}
/*----------------------------------------------------------------------------*/
static ssize_t stk2203_store_alsval(struct device_driver *ddri, const char *buf, size_t count)
{
	if(!stk2203_obj)
	{
		APS_ERR("stk2203_obj is null!!\n");
		return 0;
	}
	else if(!strcmp(buf, "def"))
	{
		memcpy(stk2203_obj->als_value, stk2203_obj->hw->als_value, sizeof(stk2203_obj->als_value));
	}
	else if(stk2203_obj->als_value_num != read_int_from_buf(stk2203_obj, buf, count, 
			stk2203_obj->hw->als_value, stk2203_obj->als_value_num))
	{
		APS_ERR("invalid format: '%s'\n", buf);
	}    
	return count;
}
/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(als,     S_IWUSR | S_IRUGO, stk2203_show_als,   NULL);
static DRIVER_ATTR(ps,      S_IWUSR | S_IRUGO, stk2203_show_ps,    NULL);
static DRIVER_ATTR(config,  S_IWUSR | S_IRUGO, stk2203_show_config,stk2203_store_config);
static DRIVER_ATTR(alslv,   S_IWUSR | S_IRUGO, stk2203_show_alslv, stk2203_store_alslv);
static DRIVER_ATTR(alsval,  S_IWUSR | S_IRUGO, stk2203_show_alsval,stk2203_store_alsval);
static DRIVER_ATTR(trace,   S_IWUSR | S_IRUGO, stk2203_show_trace, stk2203_store_trace);
static DRIVER_ATTR(status,  S_IWUSR | S_IRUGO, stk2203_show_status,  NULL);
static DRIVER_ATTR(send,    S_IWUSR | S_IRUGO, stk2203_show_send,  stk2203_store_send);
static DRIVER_ATTR(recv,    S_IWUSR | S_IRUGO, stk2203_show_recv,  stk2203_store_recv);
static DRIVER_ATTR(reg,     S_IWUSR | S_IRUGO, stk2203_show_reg,   NULL);
static DRIVER_ATTR(i2c,     S_IWUSR | S_IRUGO, stk2203_show_i2c,   stk2203_store_i2c);
/*----------------------------------------------------------------------------*/
static struct driver_attribute *stk2203_attr_list[] = {
    &driver_attr_als,
    &driver_attr_ps,    
    &driver_attr_trace,        /*trace log*/
    &driver_attr_config,
    &driver_attr_alslv,
    &driver_attr_alsval,
    &driver_attr_status,
    &driver_attr_send,
    &driver_attr_recv,
    &driver_attr_i2c,
    &driver_attr_reg,
};

/*----------------------------------------------------------------------------*/
static int stk2203_create_attr(struct device_driver *driver) 
{
	int idx, err = 0;
	int num = (int)(sizeof(stk2203_attr_list)/sizeof(stk2203_attr_list[0]));
	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if((err = driver_create_file(driver, stk2203_attr_list[idx])))
		{            
			APS_ERR("driver_create_file (%s) = %d\n", stk2203_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}
/*----------------------------------------------------------------------------*/
	static int stk2203_delete_attr(struct device_driver *driver)
	{
	int idx ,err = 0;
	int num = (int)(sizeof(stk2203_attr_list)/sizeof(stk2203_attr_list[0]));

	if (!driver)
	return -EINVAL;

	for (idx = 0; idx < num; idx++) 
	{
		driver_remove_file(driver, stk2203_attr_list[idx]);
	}
	
	return err;
}
/****************************************************************************** 
 * Function Configuration
******************************************************************************/
static int stk2203_get_als_value(struct stk2203_priv *obj, u16 als)
{
	int idx;
	int invalid = 0;
	for(idx = 0; idx < obj->als_level_num; idx++)
	{
		if(als < obj->hw->als_level[idx])
		{
			break;
		}
	}
	
	if(idx >= obj->als_value_num)
	{
		APS_ERR("exceed range\n"); 
		idx = obj->als_value_num - 1;
	}
	
	if(1 == atomic_read(&obj->als_deb_on))
	{
		unsigned long endt = atomic_read(&obj->als_deb_end);
		if(time_after(jiffies, endt))
		{
			atomic_set(&obj->als_deb_on, 0);
		}
		
		if(1 == atomic_read(&obj->als_deb_on))
		{
			invalid = 1;
		}
	}

	if(!invalid)
	{
#if defined(MTK_AAL_SUPPORT)
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
            value = (level_diff * value_low + (als - level_low) * value_diff + ((level_diff + 1) >> 1)) / level_diff;

		APS_DBG("ALS: %d [%d, %d] => %d [%d, %d] \n", als, level_low, level_high, value, value_low, value_high);
		return value;
#endif	
		if (atomic_read(&obj->trace) & STK_TRC_CVT_ALS)
		{
			APS_DBG("ALS: %05d => %05d\n", als, obj->hw->als_value[idx]);
		}
		
		return obj->hw->als_value[idx];
	}
	else
	{
		if(atomic_read(&obj->trace) & STK_TRC_CVT_ALS)
		{
			APS_DBG("ALS: %05d => %05d (-1)\n", als, obj->hw->als_value[idx]);    
		}
		return -1;
	}
}
/*----------------------------------------------------------------------------*/
static int stk2203_get_ps_value(struct stk2203_priv *obj, u16 ps)
{
	int val, mask = atomic_read(&obj->ps_mask);
	int invalid = 0;

	if(ps > atomic_read(&obj->ps_thd_val))
	{
		val = 0;  /*close*/
	}
	else
	{
		val = 1;  /*far away*/
	}
	
	if(atomic_read(&obj->ps_suspend))
	{
		invalid = 1;
	}
	else if(1 == atomic_read(&obj->ps_deb_on))
	{
		unsigned long endt = atomic_read(&obj->ps_deb_end);
		if(time_after(jiffies, endt))
		{
			atomic_set(&obj->ps_deb_on, 0);
		}
		
		if (1 == atomic_read(&obj->ps_deb_on))
		{
			invalid = 1;
		}
	}

	if(!invalid)
	{
		if(unlikely(atomic_read(&obj->trace) & STK_TRC_CVT_PS))
		{
			if(mask)
			{
				APS_DBG("PS:  %05d => %05d [M] \n", ps, val);
			}
			else
			{
				APS_DBG("PS:  %05d => %05d\n", ps, val);
			}
		}
		return val;
		
	}	
	else
	{
		if(unlikely(atomic_read(&obj->trace) & STK_TRC_CVT_PS))
		{
			APS_DBG("PS:  %05d => %05d (-1)\n", ps, val);    
		}
		return -1;
	}	
}
/****************************************************************************** 
 * Function Configuration
******************************************************************************/
static int stk2203_open(struct inode *inode, struct file *file)
{
	file->private_data = stk2203_i2c_client;

	if (!file->private_data)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}
	
	return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int stk2203_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}
/*----------------------------------------------------------------------------*/
//static int stk2203_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
//       unsigned long arg)
static long stk2203_unlocked_ioctl(struct file *file, unsigned int cmd,
       unsigned long arg)
{
	struct i2c_client *client = (struct i2c_client*)file->private_data;
	struct stk2203_priv *obj = i2c_get_clientdata(client);  
	long err = 0;
	void __user *ptr = (void __user*) arg;
	int dat;
	uint32_t enable;

	switch (cmd)
	{
		case ALSPS_SET_PS_MODE:
			if(copy_from_user(&enable, ptr, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			if(enable)
			{
				if((err = stk2203_enable_ps(obj->client, 1)))
				{
					APS_ERR("enable ps fail: %1ld\n", err); 
					goto err_out;
				}
				
				set_bit(STK_BIT_PS, &obj->enable);
			}
			else
			{
				if((err = stk2203_enable_ps(obj->client, 0)))
				{
					APS_ERR("disable ps fail: %1ld\n", err); 
					goto err_out;
				}
				
				clear_bit(STK_BIT_PS, &obj->enable);
			}
			break;

		case ALSPS_GET_PS_MODE:
			enable = test_bit(STK_BIT_PS, &obj->enable) ? (1) : (0);
			if(copy_to_user(ptr, &enable, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_GET_PS_DATA:    
			if((err = stk2203_read_ps(obj->client, &obj->ps)))
			{
				goto err_out;
			}
			
			dat = stk2203_get_ps_value(obj, obj->ps);
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}  
			break;

		case ALSPS_GET_PS_RAW_DATA:    
			if((err = stk2203_read_ps(obj->client, &obj->ps)))
			{
				goto err_out;
			}
			
			dat = obj->ps;
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
			{
				if((err = stk2203_enable_als(obj->client, 1)))
				{
					APS_ERR("enable als fail: %1ld\n", err); 
					goto err_out;
				}
				set_bit(STK_BIT_ALS, &obj->enable);
			}
			else
			{
				if((err = stk2203_enable_als(obj->client, 0)))
				{
					APS_ERR("disable als fail: %ld\n", err); 
					goto err_out;
				}
				clear_bit(STK_BIT_ALS, &obj->enable);
			}
			break;

		case ALSPS_GET_ALS_MODE:
			enable = test_bit(STK_BIT_ALS, &obj->enable) ? (1) : (0);
			if(copy_to_user(ptr, &enable, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_GET_ALS_DATA: 
			if((err = stk2203_read_als(obj->client, &obj->als)))
			{
				goto err_out;
			}

			dat = stk2203_get_als_value(obj, obj->als);
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}              
			break;

		case ALSPS_GET_ALS_RAW_DATA:    
			if((err = stk2203_read_als(obj->client, &obj->als)))
			{
				goto err_out;
			}

			dat = obj->als;
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}              
			break;

		default:
			APS_ERR("%s not supported = 0x%04x", __FUNCTION__, cmd);
			err = -ENOIOCTLCMD;
			break;
	}

	err_out:
	return err;    
}
/*----------------------------------------------------------------------------*/
static struct file_operations stk2203_fops = {
//	.owner = THIS_MODULE,
	.open = stk2203_open,
	.release = stk2203_release,
	.unlocked_ioctl = stk2203_unlocked_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice stk2203_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "als_ps",
	.fops = &stk2203_fops,
};
/*----------------------------------------------------------------------------*/
static int stk2203_i2c_suspend(struct i2c_client *client, pm_message_t msg) 
{
	//struct stk2203_priv *obj = i2c_get_clientdata(client);    
	//int err;
	APS_FUN();    
/*
	if(msg.event == PM_EVENT_SUSPEND)
	{   
		if(!obj)
		{
			APS_ERR("null pointer!!\n");
			return -EINVAL;
		}
		
		atomic_set(&obj->als_suspend, 1);
		if((err = stk2203_enable_als(client, 0)))
		{
			APS_ERR("disable als: %d\n", err);
			return err;
		}

		atomic_set(&obj->ps_suspend, 1);
		if((err = stk2203_enable_ps(client, 0)))
		{
			APS_ERR("disable ps:  %d\n", err);
			return err;
		}
		
		stk2203_power(obj->hw, 0);
	}
*/
	return 0;
}
/*----------------------------------------------------------------------------*/
static int stk2203_i2c_resume(struct i2c_client *client)
{
	//struct stk2203_priv *obj = i2c_get_clientdata(client);        
	//int err;
	APS_FUN();
/*
	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}

	stk2203_power(obj->hw, 1);
	if((err = stk2203_init_client(client)))
	{
		APS_ERR("initialize client fail!!\n");
		return err;        
	}
	atomic_set(&obj->als_suspend, 0);
	if(test_bit(STK_BIT_ALS, &obj->enable))
	{
		if((err = stk2203_enable_als(client, 1)))
		{
			APS_ERR("enable als fail: %d\n", err);        
		}
	}
	atomic_set(&obj->ps_suspend, 0);
	if(test_bit(STK_BIT_PS,  &obj->enable))
	{
		if((err = stk2203_enable_ps(client, 1)))
		{
			APS_ERR("enable ps fail: %d\n", err);                
		}
	}
*/
	return 0;
}
/*----------------------------------------------------------------------------*/
static void stk2203_early_suspend(struct early_suspend *h) 
{   /*early_suspend is only applied for ALS*/
	struct stk2203_priv *obj = container_of(h, struct stk2203_priv, early_drv);   
	int err;
	APS_FUN();    

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return;
	}
	
	atomic_set(&obj->als_suspend, 1);    
	if((err = stk2203_enable_als(obj->client, 0)))
	{
		APS_ERR("disable als fail: %d\n", err); 
	}
}
/*----------------------------------------------------------------------------*/
static void stk2203_late_resume(struct early_suspend *h)
{   /*early_suspend is only applied for ALS*/
	struct stk2203_priv *obj = container_of(h, struct stk2203_priv, early_drv);         
	int err;
	hwm_sensor_data sensor_data;
	
	memset(&sensor_data, 0, sizeof(sensor_data));
	APS_FUN();

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return;
	}

	atomic_set(&obj->als_suspend, 0);
	if(test_bit(STK_BIT_ALS, &obj->enable))
	{
		if((err = stk2203_enable_als(obj->client, 1)))
		{
			APS_ERR("enable als fail: %d\n", err);        

		}
	}
}

int stk2203_ps_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	hwm_sensor_data* sensor_data;
	struct stk2203_priv *obj = (struct stk2203_priv *)self;
	
	//APS_FUN(f);
	switch (command)
	{
		case SENSOR_DELAY:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Set delay parameter error!\n");
				err = -EINVAL;
			}
			// Do nothing
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
					if((err = stk2203_enable_ps(obj->client, 1)))
					{
						APS_ERR("enable ps fail: %d\n", err); 
						return -1;
					}
					set_bit(STK_BIT_PS, &obj->enable);
				}
				else
				{
					if((err = stk2203_enable_ps(obj->client, 0)))
					{
						APS_ERR("disable ps fail: %d\n", err); 
						return -1;
					}
					clear_bit(STK_BIT_PS, &obj->enable);
				}
			}
			break;

		case SENSOR_GET_DATA:
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				APS_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				sensor_data = (hwm_sensor_data *)buff_out;				
				
				if((err = stk2203_read_ps(obj->client, &obj->ps)))
				{
					err = -1;;
				}
				else
				{
					sensor_data->values[0] = stk2203_get_ps_value(obj, obj->ps);
					sensor_data->value_divide = 1;
					sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
				}				
			}
			break;
		default:
			APS_ERR("proxmy sensor operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}
	
	return err;
}

int stk2203_als_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	hwm_sensor_data* sensor_data;
	struct stk2203_priv *obj = (struct stk2203_priv *)self;
	
	//APS_FUN(f);
	switch (command)
	{
		case SENSOR_DELAY:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Set delay parameter error!\n");
				err = -EINVAL;
			}
			// Do nothing
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
					if((err = stk2203_enable_als(obj->client, 1)))
					{
						APS_ERR("enable als fail: %d\n", err); 
						return -1;
					}
					set_bit(STK_BIT_ALS, &obj->enable);
				}
				else
				{
					if((err = stk2203_enable_als(obj->client, 0)))
					{
						APS_ERR("disable als fail: %d\n", err); 
						return -1;
					}
					clear_bit(STK_BIT_ALS, &obj->enable);
				}
				
			}
			break;

		case SENSOR_GET_DATA:
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				APS_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				sensor_data = (hwm_sensor_data *)buff_out;
								
				if((err = stk2203_read_als(obj->client, &obj->als)))
				{
					err = -1;
				}
				else
				{
					sensor_data->values[0] = stk2203_get_als_value(obj, obj->als);
					sensor_data->value_divide = 1;
					sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
				}				
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
/*
static int stk2203_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) 
{    
	strcpy(info->type, STK2203_DEV_NAME);
	return 0;
}
*/
/*----------------------------------------------------------------------------*/
static int stk2203_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct stk2203_priv *obj;
	struct hwmsen_object obj_ps, obj_als;
	int err = 0;

	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}
	memset(obj, 0, sizeof(*obj));
	stk2203_obj = obj;

	obj->hw = get_cust_alsps_hw();
	stk2203_get_addr(obj->hw, &obj->addr);

	INIT_DELAYED_WORK(&obj->eint_work, stk2203_eint_work);
	obj->client = client;
	i2c_set_clientdata(client, obj);	
	atomic_set(&obj->als_debounce, 2000);
	atomic_set(&obj->als_deb_on, 0);
	atomic_set(&obj->als_deb_end, 0);
	atomic_set(&obj->ps_debounce, 1000);
	atomic_set(&obj->ps_deb_on, 0);
	atomic_set(&obj->ps_deb_end, 0);
	atomic_set(&obj->ps_mask, 0);
	atomic_set(&obj->trace, 0x00);
	atomic_set(&obj->als_suspend, 0);
	atomic_set(&obj->als_cmd_val, GAIN00_ALS|IT01_ALS|SD_ALS); //Gain:1/8     Refresh:100ms   
	atomic_set(&obj->als_intr_val, THD01_ALS|PRST01_ALS);             //THD : 0x00  Disable INTR      
	atomic_set(&obj->ps_cmd_val, 0x01);
       atomic_set(&obj->ps_thd_val,  obj->hw->ps_threshold);
	if(obj->hw->polling_mode == 0)
	{
        atomic_set(&obj->als_intr_val, THD01_ALS|PRST01_ALS|FLAG_ALS);  //THD : 0x00  Enablee INTR      
        //atomic_add(&obj->ps_cmd_val, 0x02);
        atomic_set(&obj->ps_cmd_val, 0x03);
        APS_LOG("enable interrupt\n");
	}
	
	obj->enable = 0;
	obj->pending_intr = 0;
	obj->als_level_num = sizeof(obj->hw->als_level)/sizeof(obj->hw->als_level[0]);
	obj->als_value_num = sizeof(obj->hw->als_value)/sizeof(obj->hw->als_value[0]);   
	BUG_ON(sizeof(obj->als_level) != sizeof(obj->hw->als_level));
	memcpy(obj->als_level, obj->hw->als_level, sizeof(obj->als_level));
	BUG_ON(sizeof(obj->als_value) != sizeof(obj->hw->als_value));
	memcpy(obj->als_value, obj->hw->als_value, sizeof(obj->als_value));
	atomic_set(&obj->i2c_retry, 3);
	if(!(atomic_read(&obj->als_cmd_val) & SD_ALS))
	{
		set_bit(STK_BIT_ALS, &obj->enable);
	}
	
	if(!(atomic_read(&obj->ps_cmd_val) & SD_PS))
	{
		set_bit(STK_BIT_PS, &obj->enable);
	}
	
	stk2203_i2c_client = client;

	
	if((err = stk2203_init_client(client)))
	{
		goto exit_init_failed;
	}
	
	if((err = misc_register(&stk2203_device)))
	{
		APS_ERR("stk2203_device register failed\n");
		goto exit_misc_device_register_failed;
	}

	if((err = stk2203_create_attr(&stk2203_alsps_driver.driver)))
	{
		APS_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}
	obj_ps.self = stk2203_obj;
	if(1 == obj->hw->polling_mode)
	{
	  obj_ps.polling = 1;
	}
	else
	{
	  obj_ps.polling = 0;//interrupt mode
	}
	obj_ps.sensor_operate = stk2203_ps_operate;
	if((err = hwmsen_attach(ID_PROXIMITY, &obj_ps)))
	{
		APS_ERR("attach fail = %d\n", err);
		goto exit_create_attr_failed;
	}
	
	obj_als.self = stk2203_obj;
	if(1 == obj->hw->polling_mode)
	{
	  obj_als.polling = 1;
	}
	else
	{
	  obj_als.polling = 0;//interrupt mode
	}
	obj_als.sensor_operate = stk2203_als_operate;
	if((err = hwmsen_attach(ID_LIGHT, &obj_als)))
	{
		APS_ERR("attach fail = %d\n", err);
		goto exit_create_attr_failed;
	}


#if defined(CONFIG_HAS_EARLYSUSPEND)
	obj->early_drv.level    = EARLY_SUSPEND_LEVEL_STOP_DRAWING - 2,
	obj->early_drv.suspend  = stk2203_early_suspend,
	obj->early_drv.resume   = stk2203_late_resume,    
	register_early_suspend(&obj->early_drv);
#endif

	APS_LOG("%s: OK\n", __func__);
	return 0;

	exit_create_attr_failed:
	misc_deregister(&stk2203_device);
	exit_misc_device_register_failed:
	exit_init_failed:
	//i2c_detach_client(client);
//	exit_kfree:
	kfree(obj);
	exit:
	stk2203_i2c_client = NULL;           
	APS_ERR("%s: err = %d\n", __func__, err);
	return err;
}
/*----------------------------------------------------------------------------*/
static int stk2203_i2c_remove(struct i2c_client *client)
{
	int err;	
	
	if((err = stk2203_delete_attr(&stk2203_i2c_driver.driver)))
	{
		APS_ERR("stk2203_delete_attr fail: %d\n", err);
	} 

	if((err = misc_deregister(&stk2203_device)))
	{
		APS_ERR("misc_deregister fail: %d\n", err);    
	}
	
	stk2203_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));

	return 0;
}
/*----------------------------------------------------------------------------*/
static int stk2203_probe(struct platform_device *pdev) 
{
	struct alsps_hw *hw = get_cust_alsps_hw();
	struct stk2203_i2c_addr addr;

	stk2203_power(hw, 1);    
	stk2203_get_addr(hw, &addr);
	//stk2203_force[0] = hw->i2c_num;
	//stk2203_force[1] = hw->i2c_addr[0];
	if(i2c_add_driver(&stk2203_i2c_driver))
	{
		APS_ERR("add driver error\n");
		return -1;
	} 
	return 0;
}
/*----------------------------------------------------------------------------*/
static int stk2203_remove(struct platform_device *pdev)
{
	struct alsps_hw *hw = get_cust_alsps_hw();
	APS_FUN();    
	stk2203_power(hw, 0);    
	i2c_del_driver(&stk2203_i2c_driver);
	return 0;
}
/*----------------------------------------------------------------------------*/
static struct platform_driver stk2203_alsps_driver = {
	.probe      = stk2203_probe,
	.remove     = stk2203_remove,    
	.driver     = {
		.name  = "als_ps",
//		.owner = THIS_MODULE,
	}
};
/*----------------------------------------------------------------------------*/
static int __init stk2203_init(void)
{
    struct alsps_hw *hw = get_cust_alsps_hw();
    APS_FUN();	
    APS_LOG("%s: i2c_number=%d\n", __func__,hw->i2c_num);
    i2c_register_board_info(hw->i2c_num, &i2c_stk2203, 1);
    if(platform_driver_register(&stk2203_alsps_driver))
    {
    	APS_ERR("failed to register driver");
    	return -ENODEV;
    }
    return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit stk2203_exit(void)
{
	APS_FUN();
	platform_driver_unregister(&stk2203_alsps_driver);
}
/*----------------------------------------------------------------------------*/
module_init(stk2203_init);
module_exit(stk2203_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("TC Chu");
MODULE_DESCRIPTION("STK2203 ALS driver");
MODULE_LICENSE("GPL");
