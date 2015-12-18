/* drivers/hwmon/mt6516/amit/cktps.c - CKTPS ALS/PS driver
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
#include <asm/io.h>
#include <cust_eint.h>
#include <cust_alsps.h>
#include <linux/hwmsen_helper.h>

#include <alsps.h>
#define POWER_NONE_MACRO MT65XX_POWER_NONE
#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>

#include <linux/earlysuspend.h>
#include <linux/wakelock.h>
#include <linux/sched.h>
#include "cktps.h"
/******************************************************************************
 * configuration
*******************************************************************************/
/*----------------------------------------------------------------------------*/

#define CKTPS_DEV_NAME     "cktps"
/*----------------------------------------------------------------------------*/
#define APS_TAG                  "[CKTPS] "
#define APS_FUN(f)               printk(APS_TAG"%s\n", __FUNCTION__)
#define APS_ERR(fmt, args...)    printk(KERN_ERR  APS_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define APS_LOG(fmt, args...)    printk(APS_TAG fmt, ##args)
#define APS_DBG(fmt, args...)    printk(APS_TAG fmt, ##args)                 
/******************************************************************************
 * extern functions
*******************************************************************************/
/*for interrup work mode support --add by liaoxl.lenovo 12.08.2011*/
extern void mt_eint_unmask(unsigned int eint_num);
extern void mt_eint_mask(unsigned int eint_num);
extern void mt_eint_set_hw_debounce(unsigned int eint_num, unsigned int ms);
extern void mt_eint_set_polarity(unsigned int eint_num, unsigned int pol);
extern unsigned int mt_eint_set_sens(unsigned int eint_num, unsigned int sens);
extern void mt_eint_registration(unsigned int eint_num, unsigned int flow, void (EINT_FUNC_PTR)(void), unsigned int is_auto_umask);

/*----------------------------------------------------------------------------*/
struct wake_lock ps_lock;//add xwenfeng 11_26
static volatile int call_status = 0;
static volatile int wakelock_locked =0;
static struct i2c_client *cktps_i2c_client = NULL;
/*----------------------------------------------------------------------------*/
static const struct i2c_device_id cktps_i2c_id[] = {{CKTPS_DEV_NAME,0},{}};
static struct i2c_board_info __initdata i2c_CKTPS={ I2C_BOARD_INFO(CKTPS_DEV_NAME, (0X88>>1))};
/*the adapter id & i2c address will be available in customization*/
//static unsigned short cktps_force[] = {0x02, 0X72, I2C_CLIENT_END, I2C_CLIENT_END};
//static const unsigned short *const cktps_forces[] = { cktps_force, NULL };
//static struct i2c_client_address_data cktps_addr_data = { .forces = cktps_forces,};
/*----------------------------------------------------------------------------*/
static int cktps_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id); 
static int cktps_i2c_remove(struct i2c_client *client);
static int cktps_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
/*----------------------------------------------------------------------------*/
#ifdef CKTPS_AUTO_DETECT
static int  cktps_local_init(void);
static int cktps_remove(void);

static struct sensor_init_info cktps_init_info = {
		.name = CKTPS_DEV_NAME,
		.init =cktps_local_init,
		.uninit = cktps_remove,
};
static int cktps_init_flag = -1;
#endif
/*----------------------------------------------------------------------------*/
static int cktps_i2c_suspend(struct i2c_client *client, pm_message_t msg);
static int cktps_i2c_resume(struct i2c_client *client);
//added by luosen 2012-11-30
#define TP_PROXIMITY_SENSOR_NEW
#if defined(TP_PROXIMITY_SENSOR_NEW)
//int ckt_tp_replace_ps_mod_on(void);
//int ckt_tp_replace_ps_mod_off(void);
//extern int  ckt_tp_replace_ps_enable(int enable);
extern u16  ckt_get_tp_replace_ps_value(void);
extern CTP_Face_Mode_Switch(int onoff_state);
//int ckt_tp_replace_ps_state= 0;
//int ckt_tp_replace_ps_close = 0;
#endif
//end

static struct cktps_priv *g_cktps_ptr = NULL;

 struct PS_CALI_DATA_STRUCT
{
    int close;
    int far_away;
    int valid;
} ;

static struct PS_CALI_DATA_STRUCT ps_cali={0,0,0};
static int intr_flag_value = 0;
/*----------------------------------------------------------------------------*/
typedef enum {
    CMC_BIT_ALS    = 1,
    CMC_BIT_PS     = 2,
} CMC_BIT;
/*----------------------------------------------------------------------------*/
struct cktps_i2c_addr {    /*define a series of i2c slave address*/
    u8  write_addr;  
    u8  ps_thd;     /*PS INT threshold*/
};
/*----------------------------------------------------------------------------*/
struct cktps_priv {
    struct alsps_hw  *hw;
    struct i2c_client *client;
    struct work_struct  eint_work;

    /*i2c address group*/
    struct cktps_i2c_addr  addr;
    
    /*misc*/
    u16		    als_modulus;
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
    u16          ps;
    u8          _align;
    u16         als_level_num;
    u16         als_value_num;
    u32         als_level[C_CUST_ALS_LEVEL-1];
    u32         als_value[C_CUST_ALS_LEVEL];

    atomic_t    als_cmd_val;    /*the cmd value can't be read, stored in ram*/
    atomic_t    ps_cmd_val;     /*the cmd value can't be read, stored in ram*/
    atomic_t    ps_thd_val_high;     /*the cmd value can't be read, stored in ram*/
	atomic_t    ps_thd_val_low;     /*the cmd value can't be read, stored in ram*/
    ulong       enable;         /*enable mask*/
    ulong       pending_intr;   /*pending interrupt*/

    /*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend    early_drv;
#endif     
};
/*----------------------------------------------------------------------------*/
static struct i2c_driver cktps_i2c_driver = {	
	.probe      = cktps_i2c_probe,
	.remove     = cktps_i2c_remove,
	.detect     = cktps_i2c_detect,
	.suspend    = cktps_i2c_suspend,
	.resume     = cktps_i2c_resume,
	.id_table   = cktps_i2c_id,
//	.address_data = &cktps_addr_data,
	.driver = {
//		.owner          = THIS_MODULE,
		.name           = CKTPS_DEV_NAME,
	},
};

static struct cktps_priv *cktps_obj = NULL;
#ifndef CKTPS_AUTO_DETECT
static struct platform_driver cktps_alsps_driver;
#endif
/*----------------------------------------------------------------------------*/
int cktps_get_addr(struct alsps_hw *hw, struct cktps_i2c_addr *addr)
{
	if(!hw || !addr)
	{
		return -EFAULT;
	}
	addr->write_addr= hw->i2c_addr[0];
	return 0;
}
/*----------------------------------------------------------------------------*/
static void cktps_power(struct alsps_hw *hw, unsigned int on) 
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
			if(!hwPowerOn(hw->power_id, hw->power_vol, "CKTPS")) 
			{
				APS_ERR("power on fails!!\n");
			}
		}
		else
		{
			if(!hwPowerDown(hw->power_id, "CKTPS")) 
			{
				APS_ERR("power off fail!!\n");   
			}
		}
	}
	power_on = on;
}
/*----------------------------------------------------------------------------*/
static long cktps_enable_als(struct i2c_client *client, int enable)
{
		struct cktps_priv *obj = i2c_get_clientdata(client);
		u8 databuf[2];	  
		long res = 0;
		//u8 buffer[1];
		//u8 reg_value[1];
		uint32_t testbit_PS;
		
	
		if(client == NULL)
		{
			APS_DBG("CLIENT CANN'T EQUL NULL\n");
			return -1;
		}
		if(enable)
		{
			APS_LOG("cktps enable als successed!\n");
		}
		else{
			APS_LOG("cktps disable als successed!\n");
		}
		return 0;
		
EXIT_ERR:
		APS_ERR("cktps_enable_als fail\n");
		return res;
}

/*----------------------------------------------------------------------------*/
static long cktps_enable_ps(struct i2c_client *client, int enable)
{
	struct cktps_priv *obj = i2c_get_clientdata(client);
	u8 databuf[2];    
	long res = 0;
//	u8 buffer[1];
//	u8 reg_value[1];
	uint32_t testbit_ALS;

	if(client == NULL)
	{
		APS_DBG("[luosen]CLIENT CANN'T EQUL NULL\n");
		return -1;
	}
	#if defined(TP_PROXIMITY_SENSOR_NEW)
	printk("[elan_luosen_1] package: %s  (%d)call_status=%d\n",__func__,enable, call_status);
	if(enable && 0==call_status)
	{
		printk("tp ps only can be enable in call mode, (%d)call_status=%d\n", enable, call_status);
		#if 1 //LiuHuojun 原则上只允许通话时打开,为了调试先放开ps
		return -1;   
		#endif
	}
        //if( 1== ckt_tp_replace_ps_enable(enable))
        if( 1== CTP_Face_Mode_Switch(enable))
	    {
	    printk("[luosen]open or close the ps mode successed\n");
	    }		
	#endif
	
		if(enable)
		{
			APS_LOG("[luosen]cktps enable ps successed!\n");
			if(0 == obj->hw->polling_mode_ps)
			{
				//mt_eint_unmask(CUST_EINT_ALS_NUM);
			}
		}
		else{
			APS_LOG("[luosen]cktps disable ps successed!\n");
			if(0 == obj->hw->polling_mode_ps)
			{
				cancel_work_sync(&obj->eint_work);
				//mt_eint_mask(CUST_EINT_ALS_NUM);
			}
		}
		return 0;
EXIT_ERR:
	APS_ERR("[luosen]cktps_enable_ps fail\n");
	return res;
}

/*for interrup work mode support -- by liaoxl.lenovo 12.08.2011*/
static int cktps_check_and_clear_intr(struct i2c_client *client) 
{
	//struct cktps_priv *obj = i2c_get_clientdata(client);
	int res,intp,intl;
	u8 buffer[2];

	//if (mt_get_gpio_in(GPIO_ALS_EINT_PIN) == 1) /*skip if no interrupt*/  
	//    return 0;

	buffer[0] = CKTPS_CMM_STATUS;
	res = i2c_master_send(client, buffer, 0x1);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	res = i2c_master_recv(client, buffer, 0x1);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	//printk("yucong cktps_check_and_clear_intr status=0x%x\n", buffer[0]);
	res = 1;
	intp = 0;
	intl = 0;
	if(0 != (buffer[0] & 0x20))
	{
		res = 0;
		intp = 1;
	}
	if(0 != (buffer[0] & 0x10))
	{
		res = 0;
		intl = 1;		
	}

	if(0 == res)
	{
		if((1 == intp) && (0 == intl))
		{
			buffer[0] = (TAOS_TRITON_CMD_REG|TAOS_TRITON_CMD_SPL_FN|0x05);
		}
		else if((0 == intp) && (1 == intl))
		{
			buffer[0] = (TAOS_TRITON_CMD_REG|TAOS_TRITON_CMD_SPL_FN|0x06);
		}
		else
		{
			buffer[0] = (TAOS_TRITON_CMD_REG|TAOS_TRITON_CMD_SPL_FN|0x07);
		}
		res = i2c_master_send(client, buffer, 0x1);
		if(res <= 0)
		{
			goto EXIT_ERR;
		}
		else
		{
			res = 0;
		}
	}

	return res;

EXIT_ERR:
	APS_ERR("cktps_check_and_clear_intr fail\n");
	return 1;
}
/*----------------------------------------------------------------------------*/

/*yucong add for interrupt mode support MTK inc 2012.3.7*/
static int cktps_check_intr(struct i2c_client *client) 
{
	return 0;
}

static int cktps_clear_intr(struct i2c_client *client) 
{
	//struct cktps_priv *obj = i2c_get_clientdata(client);
	int res;
	return 1;
EXIT_ERR:
	APS_ERR("cktps_check_and_clear_intr fail\n");
	return 1;
}


/*-----------------------------------------------------------------------------*/
void cktps_eint_func(void)
{
	struct cktps_priv *obj = g_cktps_ptr;
	if(!obj)
	{
		return;
	}
	if(0 == obj->hw->polling_mode_ps)
	{
		schedule_work(&obj->eint_work);
	}
}

/*----------------------------------------------------------------------------*/
/*for interrup work mode support -- by liaoxl.lenovo 12.08.2011*/
int cktps_setup_eint(struct i2c_client *client)
{
	struct cktps_priv *obj = i2c_get_clientdata(client);        

	g_cktps_ptr = obj;
#if 0
	mt_set_gpio_dir(GPIO_ALS_EINT_PIN, GPIO_DIR_IN);
	mt_set_gpio_mode(GPIO_ALS_EINT_PIN, GPIO_ALS_EINT_PIN_M_EINT);
	mt_set_gpio_pull_enable(GPIO_ALS_EINT_PIN, TRUE);
	mt_set_gpio_pull_select(GPIO_ALS_EINT_PIN, GPIO_PULL_UP);

	//mt_eint_set_sens(CUST_EINT_ALS_NUM, CUST_EINT_ALS_SENSITIVE);
	//mt_eint_set_polarity(CUST_EINT_ALS_NUM, CUST_EINT_ALS_POLARITY);
	mt_eint_set_hw_debounce(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_CN);
	mt_eint_registration(CUST_EINT_ALS_NUM, CUST_EINT_ALS_TYPE, cktps_eint_func, 0);

	mt_eint_unmask(CUST_EINT_ALS_NUM); 
#endif
    return 0;
}

/*----------------------------------------------------------------------------*/

static int cktps_init_client(struct i2c_client *client)
{
	struct cktps_priv *obj = i2c_get_clientdata(client);
	int res = 0;
	if(0 == obj->hw->polling_mode_ps)
	{
		if((res = cktps_setup_eint(client))!=0)
		{
			APS_ERR("setup eint: %d\n", res);
			return res;
		}
	}
	return 0;
}

/****************************************************************************** 
 * Function Configuration
******************************************************************************/
int cktps_read_als(struct i2c_client *client, u16 *data)
{
	struct cktps_priv *obj = i2c_get_clientdata(client);	 

	return 0;
}
/*----------------------------------------------------------------------------*/

static int cktps_get_als_value(struct cktps_priv *obj, u16 als)
{
	return 0;
}
/*----------------------------------------------------------------------------*/
u16 count=0;//for test
long cktps_read_ps(struct i2c_client *client, u16 *data)
{
//	struct cktps_priv *obj = i2c_get_clientdata(client);    
	//u16 ps_value;
//	if(count==1024)
//      count=0;
//	else
//	count+=32;
#if defined(TP_PROXIMITY_SENSOR_NEW)
	count=ckt_get_tp_replace_ps_value();
#endif
	APS_LOG("[luosen]cktps_read_ps =%d\n",count);
	
	*data=count;
	return 0;    
EXIT_ERR:
	APS_ERR("cktps_read_ps fail\n");
	return -1;
}
/*----------------------------------------------------------------------------*/
static int cktps_get_ps_value(struct cktps_priv *obj, u16 ps)
{
int val_temp,val;
	if((ps  > atomic_read(&obj->ps_thd_val_high)))
			{
				val = 0;  /*close*/
				val_temp = 0;
			}
	else if((ps  < atomic_read(&obj->ps_thd_val_low)))
			{
				val = 1;  /*far away*/
				val_temp = 1;
			}
	else
			       val = val_temp;	

return val;

}


/*----------------------------------------------------------------------------*/
/*for interrup work mode support -- by liaoxl.lenovo 12.08.2011*/
static void cktps_eint_work(struct work_struct *work)
{
	struct cktps_priv *obj = (struct cktps_priv *)container_of(work, struct cktps_priv, eint_work);
	int err;
	hwm_sensor_data sensor_data;
//	u8 buffer[1];
//	u8 reg_value[1];
	u8 databuf[2];
	int res = 0;

	if((err = cktps_check_intr(obj->client)))
	{
		APS_ERR("cktps_eint_work check intrs: %d\n", err);
	}
	else
	{
		//get raw data
		cktps_read_ps(obj->client, &obj->ps);
		//mdelay(160);
		APS_DBG("cktps_eint_work rawdata ps=%d als_ch0=%d!\n",obj->ps,obj->als);
		//printk("cktps_eint_work rawdata ps=%d als_ch0=%d!\n",obj->ps,obj->als);
		sensor_data.values[0] = cktps_get_ps_value(obj, obj->ps);
		sensor_data.value_divide = 1;
		sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;			

		//let up layer to know
		if((err = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data)))
		{
		  APS_ERR("call hwmsen_get_interrupt_data fail = %d\n", err);
		}
	}
	cktps_clear_intr(obj->client);
	//mt65xx_eint_unmask(CUST_EINT_ALS_NUM);      
}



//add begin
static ssize_t ps_store_call_state(struct device_driver *ddri, const char *buf, size_t count)
{
	if (sscanf(buf, "%u", &call_status) != 1) {
			printk("ps call state: Invalid values\n");
			return -EINVAL;
		}

	switch(call_status)
    	{
        	case 0 :
			printk("ps call state: Idle state!\n");
     		break;
		case 1 :
			printk("ps call state: ringing state!\n");
		break;
		case 2 :
			printk("ps call state: active or hold state!\n");	
		break;
            
		default:
   			printk("ps call state: Invalid values\n");
        	break;
  	}
	return count;
}

static ssize_t ps_show_call_state(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	res = snprintf(buf, PAGE_SIZE, "%d\n", call_status);     
	return res;   
}
//static DRIVER_ATTR(ps_call_state,   S_IWUSR | S_IRUGO, ps_show_call_state,      ps_store_call_state);
static DRIVER_ATTR(ps_call_state,   0664, ps_show_call_state,      ps_store_call_state);

static int cktps_ReadChipInfo(struct i2c_client *client, char *buf, int bufsize)
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

	sprintf(buf, "EKTF2K Chip");
	return 0;
}
static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = cktps_i2c_client;
	char strbuf[256];
	if(NULL == client)
	{
		printk("i2c client is null!!\n");
		return 0;
	}
	
	cktps_ReadChipInfo(client, strbuf, 256);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);        
}

static DRIVER_ATTR(chipinfo,   S_IWUSR | S_IRUGO, show_chipinfo_value,      NULL);
static struct driver_attribute *cktps_attr_list[] = {
	&driver_attr_ps_call_state,
	&driver_attr_chipinfo
};

static int cktps_create_attr(struct device_driver *driver) 
{
	int idx, err = 0;
	int num = (int)(sizeof(cktps_attr_list)/sizeof(cktps_attr_list[0]));
	if (driver == NULL)
	{
		return -EINVAL;
	}
	for(idx = 0; idx < num; idx++)
	{
		if((err = driver_create_file(driver, cktps_attr_list[idx])))
		{            
			APS_ERR("driver_create_file (%s) = %d\n", cktps_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}


static int cktps_delete_attr(struct device_driver *driver)
{
	int idx ,err = 0;
	int num = (int)(sizeof(cktps_attr_list)/sizeof(cktps_attr_list[0]));
	if (!driver)
	return -EINVAL;
	for (idx = 0; idx < num; idx++) 
	{
		driver_remove_file(driver, cktps_attr_list[idx]);
	}
	return err;
}


/****************************************************************************** 
 * Function Configuration
******************************************************************************/
static int cktps_open(struct inode *inode, struct file *file)
{
	file->private_data = cktps_i2c_client;

	if (!file->private_data)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}
	
	return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int cktps_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

/*----------------------------------------------------------------------------*/
static long cktps_unlocked_ioctl(struct file *file, unsigned int cmd,
       unsigned long arg)
{
	struct i2c_client *client = (struct i2c_client*)file->private_data;
	struct cktps_priv *obj = i2c_get_clientdata(client);  
	long err = 0;
	void __user *ptr = (void __user*) arg;
	int dat;
	uint32_t enable;
	//struct PS_CALI_DATA_STRUCT ps_cali_temp;

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
				if((err = cktps_enable_ps(obj->client, 1)))
				{
					APS_ERR("enable ps fail: %ld\n", err); 
					goto err_out;
				}
				
				set_bit(CMC_BIT_PS, &obj->enable);
			}
			else
			{
				if((err = cktps_enable_ps(obj->client, 0)))
				{
					APS_ERR("disable ps fail: %ld\n", err); 
					goto err_out;
				}
				
				clear_bit(CMC_BIT_PS, &obj->enable);
			}
			break;

		case ALSPS_GET_PS_MODE:
			enable = test_bit(CMC_BIT_PS, &obj->enable) ? (1) : (0);
			if(copy_to_user(ptr, &enable, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_GET_PS_DATA:    
			if((err = cktps_read_ps(obj->client, &obj->ps)))
			{
				goto err_out;
			}
			
			dat = cktps_get_ps_value(obj, obj->ps);
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}  
			break;

		case ALSPS_GET_PS_RAW_DATA:    
			if((err = cktps_read_ps(obj->client, &obj->ps)))
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
				if((err = cktps_enable_als(obj->client, 1)))
				{
					APS_ERR("enable als fail: %ld\n", err); 
					goto err_out;
				}
				set_bit(CMC_BIT_ALS, &obj->enable);
			}
			else
			{
				if((err = cktps_enable_als(obj->client, 0)))
				{
					APS_ERR("disable als fail: %ld\n", err); 
					goto err_out;
				}
				clear_bit(CMC_BIT_ALS, &obj->enable);
			}
			break;

		case ALSPS_GET_ALS_MODE:
			enable = test_bit(CMC_BIT_ALS, &obj->enable) ? (1) : (0);
			if(copy_to_user(ptr, &enable, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_GET_ALS_DATA: 
			if((err = cktps_read_als(obj->client, &obj->als)))
			{
				goto err_out;
			}

			dat = cktps_get_als_value(obj, obj->als);
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}              
			break;

		case ALSPS_GET_ALS_RAW_DATA:    
			if((err = cktps_read_als(obj->client, &obj->als)))
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

/*		case ALSPS_SET_PS_CALI:
			dat = (void __user*)arg;
			if(dat == NULL)
			{
				APS_LOG("dat == NULL\n");
				err = -EINVAL;
				break;	  
			}
			if(copy_from_user(&ps_cali_temp,dat, sizeof(ps_cali_temp)))
			{
				APS_LOG("copy_from_user\n");
				err = -EFAULT;
				break;	  
			}
			cktps_WriteCalibration(&ps_cali_temp);
			APS_LOG(" ALSPS_SET_PS_CALI %d,%d,%d\t",ps_cali_temp.close,ps_cali_temp.far_away,ps_cali_temp.valid);
			break;
		case ALSPS_GET_PS_RAW_DATA_FOR_CALI:
			cktps_init_client_for_cali(obj->client);
			err = cktps_read_data_for_cali(obj->client,&ps_cali_temp);
			if(err)
			{
			   goto err_out;
			}
			cktps_init_client(obj->client);
			// cktps_enable_ps(obj->client, 1);
			cktps_enable(obj->client, 0);
			if(copy_to_user(ptr, &ps_cali_temp, sizeof(ps_cali_temp)))
			{
				err = -EFAULT;
				goto err_out;
			}              
			break;
*/
		default:
			APS_ERR("%s not supported = 0x%04x", __FUNCTION__, cmd);
			err = -ENOIOCTLCMD;
			break;
	}

	err_out:
	return err;    
}
/*----------------------------------------------------------------------------*/
static struct file_operations cktps_fops = {
	.owner = THIS_MODULE,
	.open = cktps_open,
	.release = cktps_release,
	.unlocked_ioctl = cktps_unlocked_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice cktps_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "als_ps",//name for factory test
	.fops = &cktps_fops,
};
/*----------------------------------------------------------------------------*/
static int cktps_i2c_suspend(struct i2c_client *client, pm_message_t msg) 
{
	//struct cktps_priv *obj = i2c_get_clientdata(client);    
	//int err;
	APS_FUN();    
	return 0;
}
/*----------------------------------------------------------------------------*/
static int cktps_i2c_resume(struct i2c_client *client)
{
	//struct cktps_priv *obj = i2c_get_clientdata(client);        
	//int err;
	APS_FUN();
	return 0;
}
/*----------------------------------------------------------------------------*/
static void cktps_early_suspend(struct early_suspend *h) 
{   /*early_suspend is only applied for ALS*/
	struct cktps_priv *obj = container_of(h, struct cktps_priv, early_drv);   
	int err;
	APS_FUN();    

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return;
	}

	/*
	//add begin
	if((call_status!=0)&&(test_bit(CMC_BIT_PS, &obj->enable)))
	{
            wake_lock(&ps_lock);//add xwenfeng 11_26
            wakelock_locked = 1;
	}
	 //add end   
	 */

	#if 1
	atomic_set(&obj->als_suspend, 1);
	if(test_bit(CMC_BIT_ALS, &obj->enable))
	{
		if((err = cktps_enable_als(obj->client, 0)))
		{
			APS_ERR("disable als fail: %d\n", err); 
		}
	}
	#endif
}
/*----------------------------------------------------------------------------*/
static void cktps_late_resume(struct early_suspend *h)
{   /*early_suspend is only applied for ALS*/
	struct cktps_priv *obj = container_of(h, struct cktps_priv, early_drv);         
	int err;
	APS_FUN();

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return;
	}

        #if 1
	atomic_set(&obj->als_suspend, 0);
	if(test_bit(CMC_BIT_ALS, &obj->enable))
	{
		if((err = cktps_enable_als(obj->client, 1)))
		{
			APS_ERR("enable als fail: %d\n", err);        

		}
	}
	#endif
	/*
	//add begin
	if(wakelock_locked)
	wake_unlock(&ps_lock);//add xwenfeng 11_26
	//add end
	*/
}

int cktps_ps_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	hwm_sensor_data* sensor_data;
	struct cktps_priv *obj = (struct cktps_priv *)self;
	
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
					if((err = cktps_enable_ps(obj->client, 1)))
					{
						APS_ERR("enable ps fail: %d\n", err); 
						return -1;
					}
					set_bit(CMC_BIT_PS, &obj->enable);
					#if 0	
					if(err = cktps_enable_als(obj->client, 1))
					{
						APS_ERR("enable als fail: %d\n", err); 
						return -1;
					}
					set_bit(CMC_BIT_ALS, &obj->enable);
					#endif
				}
				else
				{
					if((err = cktps_enable_ps(obj->client, 0)))
					{
						APS_ERR("disable ps fail: %d\n", err); 
						return -1;
					}
					clear_bit(CMC_BIT_PS, &obj->enable);
					#if 0
					if(err = cktps_enable_als(obj->client, 0))
					{
						APS_ERR("disable als fail: %d\n", err); 
						return -1;
					}
					clear_bit(CMC_BIT_ALS, &obj->enable);
					#endif
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
				cktps_read_ps(obj->client, &obj->ps);
				
                                //mdelay(160);
				APS_ERR("cktps_ps_operate als data=%d!\n",obj->als);
				sensor_data->values[0] = cktps_get_ps_value(obj, obj->ps);
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

static int temp_als = 0;
int cktps_als_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	hwm_sensor_data* sensor_data;
	struct cktps_priv *obj = (struct cktps_priv *)self;

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
					if((err = cktps_enable_als(obj->client, 1)))
					{
						APS_ERR("enable als fail: %d\n", err); 
						return -1;
					}
					set_bit(CMC_BIT_ALS, &obj->enable);
				}
				else
				{
					if((err = cktps_enable_als(obj->client, 0)))
					{
						APS_ERR("disable als fail: %d\n", err); 
						return -1;
					}
					clear_bit(CMC_BIT_ALS, &obj->enable);
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
				/*yucong MTK add for fixing know issue*/
				#if 1
				cktps_read_als(obj->client, &obj->als);
				if(obj->als == 0)
				{
					sensor_data->values[0] = temp_als;				
				}else{
					u16 b[2];
					int i;
					for(i = 0;i < 2;i++){
					cktps_read_als(obj->client, &obj->als);
					b[i] = obj->als;
					}
					(b[1] > b[0])?(obj->als = b[0]):(obj->als = b[1]);
					sensor_data->values[0] = cktps_get_als_value(obj, obj->als);
					temp_als = sensor_data->values[0];
				}
				#endif
				sensor_data->value_divide = 1;
				sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
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
static int cktps_i2c_detect(struct i2c_client *client, struct i2c_board_info *info) 
{    
	strcpy(info->type, CKTPS_DEV_NAME);
	return 0;
}

/*----------------------------------------------------------------------------*/
static int cktps_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct cktps_priv *obj;
	struct hwmsen_object obj_ps, obj_als;
	int err = 0;
	printk("[Tyrael]cktps_i2c_probe ...");
	//wake_lock_init(&ps_lock,WAKE_LOCK_SUSPEND,"ps wakelock");
	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}
	memset(obj, 0, sizeof(*obj));
	cktps_obj = obj;

	obj->hw = get_cust_alsps_hw();
	cktps_get_addr(obj->hw, &obj->addr);

	/*for interrup work mode support -- by liaoxl.lenovo 12.08.2011*/
	if(0 == obj->hw->polling_mode_ps)
	{
		INIT_WORK(&obj->eint_work, cktps_eint_work);
	}
	obj->client = client;
	i2c_set_clientdata(client, obj);	
	atomic_set(&obj->als_debounce, 50);
	atomic_set(&obj->als_deb_on, 0);
	atomic_set(&obj->als_deb_end, 0);
	atomic_set(&obj->ps_debounce, 10);
	atomic_set(&obj->ps_deb_on, 0);
	atomic_set(&obj->ps_deb_end, 0);
	atomic_set(&obj->ps_mask, 0);
	atomic_set(&obj->als_suspend, 0);
	atomic_set(&obj->als_cmd_val, 0xDF);
	atomic_set(&obj->ps_cmd_val,  0xC1);
	atomic_set(&obj->ps_thd_val_high,  obj->hw->ps_threshold_high);
	atomic_set(&obj->ps_thd_val_low,  obj->hw->ps_threshold_low);
	obj->enable = 0;
	obj->pending_intr = 0;
	obj->als_level_num = sizeof(obj->hw->als_level)/sizeof(obj->hw->als_level[0]);
	obj->als_value_num = sizeof(obj->hw->als_value)/sizeof(obj->hw->als_value[0]);  
	/*Lenovo-sw chenlj2 add 2011-06-03,modified gain 16 to 1/5 accoring to actual thing */
	obj->als_modulus = (400*100*ZOOM_TIME)/(1*150);//(1/Gain)*(400/Tine), this value is fix after init ATIME and CONTROL register value
										//(400)/16*2.72 here is amplify *100 //16
	BUG_ON(sizeof(obj->als_level) != sizeof(obj->hw->als_level));
	memcpy(obj->als_level, obj->hw->als_level, sizeof(obj->als_level));
	BUG_ON(sizeof(obj->als_value) != sizeof(obj->hw->als_value));
	memcpy(obj->als_value, obj->hw->als_value, sizeof(obj->als_value));
	atomic_set(&obj->i2c_retry, 3);
	set_bit(CMC_BIT_ALS, &obj->enable);
	set_bit(CMC_BIT_PS, &obj->enable);

	
	cktps_i2c_client = client;

	
	if((err = cktps_init_client(client)))
	{
		goto exit_init_failed;
	}
	APS_LOG("cktps_init_client() OK!\n");

	if((err = misc_register(&cktps_device)))
	{
		APS_ERR("cktps_device register failed\n");
		goto exit_misc_device_register_failed;
	}

	#ifdef CKTPS_AUTO_DETECT
	if(err = cktps_create_attr(&cktps_init_info.platform_diver_addr->driver))
	#else
	if(err = cktps_create_attr(&cktps_alsps_driver.driver))
	#endif
	{
		APS_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}

	obj_ps.self = cktps_obj;
	/*for interrup work mode support -- by liaoxl.lenovo 12.08.2011*/
	if(1 == obj->hw->polling_mode_ps)
	//if (1)
	{
		obj_ps.polling = 1;
	}
	else
	{
		obj_ps.polling = 0;
	}

	obj_ps.sensor_operate = cktps_ps_operate;
	if((err = hwmsen_attach(ID_PROXIMITY, &obj_ps)))
	{
		APS_ERR("attach fail = %d\n", err);
		goto exit_create_attr_failed;
	}
	
	obj_als.self = cktps_obj;
	obj_als.polling = 1;
	obj_als.sensor_operate = cktps_als_operate;
#if 0 //not exist als 
	if((err = hwmsen_attach(ID_LIGHT, &obj_als)))
	{
		APS_ERR("attach fail = %d\n", err);
		goto exit_create_attr_failed;
	}
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND)
	obj->early_drv.level    = EARLY_SUSPEND_LEVEL_STOP_DRAWING - 2,
	obj->early_drv.suspend  = cktps_early_suspend,
	obj->early_drv.resume   = cktps_late_resume,    
	register_early_suspend(&obj->early_drv);
#endif
#ifdef CKTPS_AUTO_DETECT
	cktps_init_flag=0;
#endif

	APS_LOG("%s: OK\n", __func__);
	return 0;

	exit_create_attr_failed:
	misc_deregister(&cktps_device);
	exit_misc_device_register_failed:
	exit_init_failed:
	//i2c_detach_client(client);
	//exit_kfree:
	kfree(obj);
	exit:
	cktps_i2c_client = NULL;           
#ifdef CKTPS_AUTO_DETECT
	cktps_init_flag=-1;
#endif
//	MT6516_EINTIRQMask(CUST_EINT_ALS_NUM);  /*mask interrupt if fail*/
	APS_ERR("%s: err = %d\n", __func__, err);
	return err;
}
/*----------------------------------------------------------------------------*/
static int cktps_i2c_remove(struct i2c_client *client)
{
	int err;	
	#ifdef CKTPS_AUTO_DETECT
	if(err = cktps_delete_attr(&cktps_init_info.platform_diver_addr->driver))
	#else
	if(err = cktps_delete_attr(&cktps_alsps_driver.driver))
	#endif
	{
		APS_ERR("cktps_delete_attr fail: %d\n", err);
	} 

	if((err = misc_deregister(&cktps_device)))
	{
		APS_ERR("misc_deregister fail: %d\n", err);    
	}
	
	cktps_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));

	return 0;
}
/*----------------------------------------------------------------------------*/
#ifdef CKTPS_AUTO_DETECT
static int cktps_remove(void)
{
	struct alsps_hw *hw = get_cust_alsps_hw();
	cktps_power(hw, 0);
	i2c_del_driver(&cktps_i2c_driver);
	return 0;
}

static int  cktps_local_init(void)
{
    struct alsps_hw *hw = get_cust_alsps_hw();
	APS_FUN();
	cktps_power(hw, 1);
	if(i2c_add_driver(&cktps_i2c_driver))
	{
		printk("add driver error\n");
		return -1;
	}
	printk("cktps_init_flag=%d\n",cktps_init_flag);
	if(-1 == cktps_init_flag)
	{
	   return -1;
	}
	return 0;
}
#else
static int cktps_probe(struct platform_device *pdev) 
{
	struct alsps_hw *hw = get_cust_alsps_hw();
	printk("[Tyrael]cktps_probe..");
	cktps_power(hw, 1);    
	//cktps_force[0] = hw->i2c_num;
	//cktps_force[1] = hw->i2c_addr[0];
	//APS_DBG("I2C = %d, addr =0x%x\n",cktps_force[0],cktps_force[1]);
	if(i2c_add_driver(&cktps_i2c_driver))
	{
		APS_ERR("add driver error\n");
		return -1;
	} 
	return 0;
}
/*----------------------------------------------------------------------------*/
static int cktps_remove(struct platform_device *pdev)
{
	struct alsps_hw *hw = get_cust_alsps_hw();
	APS_FUN();    
	cktps_power(hw, 0);    
	i2c_del_driver(&cktps_i2c_driver);
	return 0;
}
/*----------------------------------------------------------------------------*/
static struct platform_driver cktps_alsps_driver = {
	.probe      = cktps_probe,
	.remove     = cktps_remove,    
	.driver     = {
		.name  = "als_ps",
//		.owner = THIS_MODULE,
	}
};
#endif
/*----------------------------------------------------------------------------*/
static int __init cktps_init(void)
{
	APS_FUN();
	printk("[Tyrael]cktps_init..");
	i2c_register_board_info(0, &i2c_CKTPS, 1);
	#ifndef CKTPS_AUTO_DETECT
	if(platform_driver_register(&cktps_alsps_driver))
	{
		APS_ERR("failed to register driver");
		return -ENODEV;
	}
	#else
	//hwmsen_alsps_sensor_add(&cktps_init_info);
	alsps_driver_add(&cktps_init_info);
	#endif
	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit cktps_exit(void)
{
	APS_FUN();
	#ifndef CKTPS_AUTO_DETECT
	platform_driver_unregister(&cktps_alsps_driver);
	#endif
}
/*----------------------------------------------------------------------------*/
module_init(cktps_init);
//late_initcall(cktps_init);
module_exit(cktps_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("Tyrael");
MODULE_DESCRIPTION("cktps driver");
MODULE_LICENSE("GPL");
