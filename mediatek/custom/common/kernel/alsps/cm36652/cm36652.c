/* 
 * Author: yucong xiong <yucong.xion@mediatek.com>
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

#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>

#define POWER_NONE_MACRO MT65XX_POWER_NONE

#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include <asm/io.h>
#include <cust_eint.h>
#include <cust_alsps.h>
#include "cm36652.h"
#include <linux/sched.h>
/******************************************************************************
 * configuration
*******************************************************************************/
/*----------------------------------------------------------------------------*/

#define CM36652_DEV_NAME     "cm36652"
/*----------------------------------------------------------------------------*/
#define APS_TAG                  "[ALS/PS] "
#define APS_FUN(f)               printk(KERN_INFO 	APS_TAG"%s\n", __FUNCTION__)
#define APS_ERR(fmt, args...)    printk(KERN_ERR  	APS_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define APS_LOG(fmt, args...)    printk(KERN_ERR	APS_TAG fmt, ##args)
#define APS_DBG(fmt, args...)    printk(KERN_INFO 	APS_TAG fmt, ##args)    

#define I2C_FLAG_WRITE	0
#define I2C_FLAG_READ	1

/******************************************************************************
 * extern functions
*******************************************************************************/
#ifdef CUST_EINT_ALS_TYPE
extern void mt_eint_mask(unsigned int eint_num);
extern void mt_eint_unmask(unsigned int eint_num);
extern void mt_eint_set_hw_debounce(unsigned int eint_num, unsigned int ms);
extern void mt_eint_set_polarity(unsigned int eint_num, unsigned int pol);
extern unsigned int mt_eint_set_sens(unsigned int eint_num, unsigned int sens);
extern void mt_eint_registration(unsigned int eint_num, unsigned int flow, void (EINT_FUNC_PTR)(void), unsigned int is_auto_umask);
extern void mt_eint_print_status(void);
#else
extern void mt65xx_eint_mask(unsigned int line);
extern void mt65xx_eint_unmask(unsigned int line);
extern void mt65xx_eint_set_hw_debounce(unsigned int eint_num, unsigned int ms);
extern void mt65xx_eint_set_polarity(unsigned int eint_num, unsigned int pol);
extern unsigned int mt65xx_eint_set_sens(unsigned int eint_num, unsigned int sens);
extern void mt65xx_eint_registration(unsigned int eint_num, unsigned int is_deb_en, unsigned int pol, void (EINT_FUNC_PTR)(void), unsigned int is_auto_umask);
#endif
/*----------------------------------------------------------------------------*/
static int cm36652_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id); 
static int cm36652_i2c_remove(struct i2c_client *client);
static int cm36652_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
static int cm36652_i2c_suspend(struct i2c_client *client, pm_message_t msg);
static int cm36652_i2c_resume(struct i2c_client *client);

/*----------------------------------------------------------------------------*/
static const struct i2c_device_id cm36652_i2c_id[] = {{CM36652_DEV_NAME,0},{}};
static struct i2c_board_info __initdata i2c_cm36652={ I2C_BOARD_INFO(CM36652_DEV_NAME, 0x60)};
static unsigned long long int_top_time = 0;
/*----------------------------------------------------------------------------*/
struct cm36652_priv {
	struct alsps_hw  *hw;
	struct i2c_client *client;
	struct work_struct	eint_work;

	/*misc*/
	u16 		als_modulus;
	atomic_t	i2c_retry;
	atomic_t	als_suspend;
	atomic_t	als_debounce;	/*debounce time after enabling als*/
	atomic_t	als_deb_on; 	/*indicates if the debounce is on*/
	atomic_t	als_deb_end;	/*the jiffies representing the end of debounce*/
	atomic_t	ps_mask;		/*mask ps: always return far away*/
	atomic_t	ps_debounce;	/*debounce time after enabling ps*/
	atomic_t	ps_deb_on;		/*indicates if the debounce is on*/
	atomic_t	ps_deb_end; 	/*the jiffies representing the end of debounce*/
	atomic_t	ps_suspend;
	atomic_t 	trace;
	
	
	/*data*/
	u16			als;
	u8 			ps;
	u8			_align;
	u16			als_level_num;
	u16			als_value_num;
	u32			als_level[C_CUST_ALS_LEVEL-1];
	u32			als_value[C_CUST_ALS_LEVEL];
	int			ps_cali;
	
	atomic_t	als_cmd_val;	/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_cmd_val; 	/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_thd_val_high;	 /*the cmd value can't be read, stored in ram*/
	atomic_t	ps_thd_val_low; 	/*the cmd value can't be read, stored in ram*/
	atomic_t	als_thd_val_high;	 /*the cmd value can't be read, stored in ram*/
	atomic_t	als_thd_val_low; 	/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_thd_val;
	ulong		enable; 		/*enable mask*/
	ulong		pending_intr;	/*pending interrupt*/
	
	/*early suspend*/
	#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend	early_drv;
	#endif     
};
/*----------------------------------------------------------------------------*/

static struct i2c_driver cm36652_i2c_driver = {	
	.probe      = cm36652_i2c_probe,
	.remove     = cm36652_i2c_remove,
	.detect     = cm36652_i2c_detect,
	.suspend    = cm36652_i2c_suspend,
	.resume     = cm36652_i2c_resume,
	.id_table   = cm36652_i2c_id,
	.driver = {
		.name = CM36652_DEV_NAME,
	},
};

/*----------------------------------------------------------------------------*/
struct PS_CALI_DATA_STRUCT
{
	int close;
	int far_away;
	int valid;
};

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static struct i2c_client *cm36652_i2c_client = NULL;
static struct cm36652_priv *g_cm36652_ptr = NULL;
static struct cm36652_priv *cm36652_obj = NULL;
static struct platform_driver cm36652_alsps_driver;
//static struct PS_CALI_DATA_STRUCT ps_cali={0,0,0};
/*----------------------------------------------------------------------------*/

static DEFINE_MUTEX(cm36652_mutex);


/*----------------------------------------------------------------------------*/
typedef enum {
	CMC_BIT_ALS    = 1,
	CMC_BIT_PS	   = 2,
}CMC_BIT;
/*-----------------------------CMC for debugging-------------------------------*/
typedef enum {
    CMC_TRC_ALS_DATA= 0x0001,
    CMC_TRC_PS_DATA = 0x0002,
    CMC_TRC_EINT    = 0x0004,
    CMC_TRC_IOCTL   = 0x0008,
    CMC_TRC_I2C     = 0x0010,
    CMC_TRC_CVT_ALS = 0x0020,
    CMC_TRC_CVT_PS  = 0x0040,
    CMC_TRC_DEBUG   = 0x8000,
} CMC_TRC;
/*-----------------------------------------------------------------------------*/

int CM36652_i2c_master_operate(struct i2c_client *client, const char *buf, int count, int i2c_flag)
{
	int res = 0;
	mutex_lock(&cm36652_mutex);
	switch(i2c_flag){	
	case I2C_FLAG_WRITE:
	client->addr &=I2C_MASK_FLAG;
	res = i2c_master_send(client, buf, count);
	client->addr &=I2C_MASK_FLAG;
	break;
	
	case I2C_FLAG_READ:
	client->addr &=I2C_MASK_FLAG;
	client->addr |=I2C_WR_FLAG;
	client->addr |=I2C_RS_FLAG;
	res = i2c_master_send(client, buf, count);
	client->addr &=I2C_MASK_FLAG;
	break;
	default:
	APS_LOG("CM36652_i2c_master_operate i2c_flag command not support!\n");
	break;
	}
	if(res < 0)
	{
		goto EXIT_ERR;
	}
	mutex_unlock(&cm36652_mutex);
	return res;
	EXIT_ERR:
	mutex_unlock(&cm36652_mutex);
	APS_ERR("CM36652_i2c_master_operate fail\n");
	return res;
}
/*----------------------------------------------------------------------------*/
static void cm36652_power(struct alsps_hw *hw, unsigned int on) 
{
	static unsigned int power_on = 0;

	APS_LOG("power %s\n", on ? "on" : "off");

	if(hw->power_id != POWER_NONE_MACRO)
	{
		if(power_on == on)
		{
			APS_LOG("ignore power control: %d\n", on);
		}
		else if(on)
		{
			if(!hwPowerOn(hw->power_id, hw->power_vol, "CM36652")) 
			{
				APS_ERR("power on fails!!\n");
			}
		}
		else
		{
			if(!hwPowerDown(hw->power_id, "CM36652")) 
			{
				APS_ERR("power off fail!!\n");   
			}
		}
	}
	power_on = on;
}
/********************************************************************/
int cm36652_enable_ps(struct i2c_client *client, int enable)
{
	struct cm36652_priv *obj = i2c_get_clientdata(client);
	int res;
	u8 databuf[3];

	if(enable == 1){
		
		APS_LOG("cm36652_enable_ps enable_ps\n");
	
		databuf[0]= CM36652_REG_PS_CONF1_2;
		res = CM36652_i2c_master_operate(client, databuf, 0x201, I2C_FLAG_READ);
		if(res < 0)
		{
			APS_ERR("i2c_master_send function err\n");
			goto ENABLE_PS_EXIT_ERR;
		}
		APS_LOG("CM36652_REG_PS_CONF1_2 value value_low = %x, value_high = %x\n",databuf[0],databuf[1]);
		databuf[2] = databuf[1];
		databuf[1] = databuf[0]&0xFE;
		
		databuf[0]= CM36652_REG_PS_CONF1_2;
		res = CM36652_i2c_master_operate(client, databuf, 0x3, I2C_FLAG_WRITE);
		if(res < 0)
		{
			APS_ERR("i2c_master_send function err\n");
			goto ENABLE_PS_EXIT_ERR;
		}
		atomic_set(&obj->ps_deb_on, 1);
		atomic_set(&obj->ps_deb_end, jiffies+atomic_read(&obj->ps_debounce)/(1000/HZ));
		
	}else{
			APS_LOG("cm36652_enable_ps disable_ps\n");
			databuf[0]= CM36652_REG_PS_CONF1_2;
			res = CM36652_i2c_master_operate(client, databuf, 0x201, I2C_FLAG_READ);
			if(res < 0)
			{
				APS_ERR("i2c_master_send function err\n");
				goto ENABLE_PS_EXIT_ERR;
			}
			
			APS_LOG("CM36652_REG_PS_CONF1_2 value value_low = %x, value_high = %x\n",databuf[0],databuf[1]);

			databuf[2] = databuf[1];
			databuf[1] = databuf[0]|0x01;	
			databuf[0]= CM36652_REG_PS_CONF1_2;
			
			res = CM36652_i2c_master_operate(client, databuf, 0x3, I2C_FLAG_WRITE);
			if(res < 0)
			{
				APS_ERR("i2c_master_send function err\n");
				goto ENABLE_PS_EXIT_ERR;
			}
			atomic_set(&obj->ps_deb_on, 0);

		}
	
	return 0;
	ENABLE_PS_EXIT_ERR:
	return res;
}
/********************************************************************/
int cm36652_enable_als(struct i2c_client *client, int enable)
{
	struct cm36652_priv *obj = i2c_get_clientdata(client);
	int res;
	u8 databuf[3];

	if(enable == 1)
		{
			APS_LOG("cm36652_enable_als enable_als\n");
			databuf[0] = CM36652_REG_CS_CONF;
			res = CM36652_i2c_master_operate(client, databuf, 0x201, I2C_FLAG_READ);
			if(res < 0)
			{
				APS_ERR("i2c_master_send function err\n");
				goto ENABLE_ALS_EXIT_ERR;
			}
			
			APS_LOG("CM36652_REG_CS_CONF value value_low = %x, value_high = %x\n",databuf[0],databuf[1]);

			databuf[2] = databuf[1];
			databuf[1] = databuf[0]&0xFE;		
			databuf[0] = CM36652_REG_CS_CONF;
			
			res = CM36652_i2c_master_operate(client, databuf, 0x3, I2C_FLAG_WRITE);
			if(res < 0)
			{
				APS_ERR("i2c_master_send function err\n");
				goto ENABLE_ALS_EXIT_ERR;
			}
			atomic_set(&obj->als_deb_on, 1);
			atomic_set(&obj->als_deb_end, jiffies+atomic_read(&obj->als_debounce)/(1000/HZ));
		}
	else{
			APS_LOG("cm36652_enable_als disable_als\n");
			databuf[0] = CM36652_REG_CS_CONF;
			res = CM36652_i2c_master_operate(client, databuf, 0x201, I2C_FLAG_READ);
			if(res < 0)
			{
				APS_ERR("i2c_master_send function err\n");
				goto ENABLE_ALS_EXIT_ERR;
			}
			
			APS_LOG("CM36652_REG_CS_CONF value value_low = %x, value_high = %x\n",databuf[0],databuf[1]);

			databuf[2] = databuf[1];
			databuf[1] = databuf[0]|0x01;
			databuf[0] = CM36652_REG_CS_CONF;

			res = CM36652_i2c_master_operate(client, databuf, 0x3, I2C_FLAG_WRITE);
			if(res < 0)
			{
				APS_ERR("i2c_master_send function err\n");
				goto ENABLE_ALS_EXIT_ERR;
			}
			atomic_set(&obj->als_deb_on, 0);
		}
	return 0;
	ENABLE_ALS_EXIT_ERR:
	return res;
}
/********************************************************************/
long cm36652_read_ps(struct i2c_client *client, u8 *data)
{
	long res;
	u8 databuf[2];
	struct cm36652_priv *obj = i2c_get_clientdata(client);

	databuf[0] = CM36652_REG_PS_DATA;
	res = CM36652_i2c_master_operate(client, databuf, 0x201, I2C_FLAG_READ);
	if(res < 0)
	{
		APS_ERR("i2c_master_send function err\n");
		goto READ_PS_EXIT_ERR;
	}
	if(atomic_read(&obj->trace) & CMC_TRC_DEBUG){	
	APS_LOG("CM36652_REG_PS_DATA value value_low = %x, value_high = %x\n",databuf[0],databuf[1]);
	}
	
	if(databuf[0] < obj->ps_cali)
		*data = 0;
	else
		*data = databuf[0] - obj->ps_cali;
	return 0;
	READ_PS_EXIT_ERR:
	return res;
}
/********************************************************************/
long cm36652_read_als(struct i2c_client *client, u16 *data)
{
	long res;
	u8 databuf[2];
	struct cm36652_priv *obj = i2c_get_clientdata(client);
	
	databuf[0] = CM36652_REG_ALS_DATA;
	res = CM36652_i2c_master_operate(client, databuf, 0x201, I2C_FLAG_READ);
	if(res < 0)
	{
		APS_ERR("i2c_master_send function err\n");
		goto READ_ALS_EXIT_ERR;
	}
	
	if(atomic_read(&obj->trace) & CMC_TRC_DEBUG){
		APS_LOG("CM36652_REG_ALS_DATA value: %d\n", ((databuf[1]<<8)|databuf[0]));
	}
	
	*data = ((databuf[1]<<8)|databuf[0]);
	
	return 0;
	READ_ALS_EXIT_ERR:
	return res;
}
/********************************************************************/
static int cm36652_get_ps_value(struct cm36652_priv *obj, u8 ps)
{
	int val, mask = atomic_read(&obj->ps_mask);
	int invalid = 0;
	val = 0;

	if(ps > atomic_read(&obj->ps_thd_val_high))
	{
		val = 0;  /*close*/
	}
	else if(ps < atomic_read(&obj->ps_thd_val_low))
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
		if(unlikely(atomic_read(&obj->trace) & CMC_TRC_CVT_PS))
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
		if(0 == test_bit(CMC_BIT_PS,  &obj->enable))
		{
		  //if ps is disable do not report value
		  APS_DBG("PS: not enable and do not report this value\n");
		  return -1;
		}
		else
		{
		   return val;
		}
		
	}	
	else
	{
		if(unlikely(atomic_read(&obj->trace) & CMC_TRC_CVT_PS))
		{
			APS_DBG("PS:  %05d => %05d (-1)\n", ps, val);    
		}
		return -1;
	}	
}
/********************************************************************/
static int cm36652_get_als_value(struct cm36652_priv *obj, u16 als)
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
			if (atomic_read(&obj->trace) & CMC_TRC_CVT_ALS)
			{
				APS_DBG("ALS: %05d => %05d\n", als, obj->hw->als_value[idx]);
			}
			
			return obj->hw->als_value[idx];
		}
		else
		{
			if(atomic_read(&obj->trace) & CMC_TRC_CVT_ALS)
			{
				APS_DBG("ALS: %05d => %05d (-1)\n", als, obj->hw->als_value[idx]);	  
			}
			return -1;
		}

}


/*-------------------------------attribute file for debugging----------------------------------*/

/******************************************************************************
 * Sysfs attributes
*******************************************************************************/
static ssize_t cm36652_show_config(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	
	if(!cm36652_obj)
	{
		APS_ERR("cm36652_obj is null!!\n");
		return 0;
	}
	
	res = snprintf(buf, PAGE_SIZE, "(%d %d %d %d %d)\n", 
		atomic_read(&cm36652_obj->i2c_retry), atomic_read(&cm36652_obj->als_debounce), 
		atomic_read(&cm36652_obj->ps_mask), atomic_read(&cm36652_obj->ps_thd_val), atomic_read(&cm36652_obj->ps_debounce));     
	return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t cm36652_store_config(struct device_driver *ddri, const char *buf, size_t count)
{
	int retry, als_deb, ps_deb, mask, thres;
	if(!cm36652_obj)
	{
		APS_ERR("cm36652_obj is null!!\n");
		return 0;
	}
	
	if(5 == sscanf(buf, "%d %d %d %d %d", &retry, &als_deb, &mask, &thres, &ps_deb))
	{ 
		atomic_set(&cm36652_obj->i2c_retry, retry);
		atomic_set(&cm36652_obj->als_debounce, als_deb);
		atomic_set(&cm36652_obj->ps_mask, mask);
		atomic_set(&cm36652_obj->ps_thd_val, thres);        
		atomic_set(&cm36652_obj->ps_debounce, ps_deb);
	}
	else
	{
		APS_ERR("invalid content: '%s', length = %d\n", buf, count);
	}
	return count;    
}
/*----------------------------------------------------------------------------*/
static ssize_t cm36652_show_trace(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	if(!cm36652_obj)
	{
		APS_ERR("cm36652_obj is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&cm36652_obj->trace));     
	return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t cm36652_store_trace(struct device_driver *ddri, const char *buf, size_t count)
{
    int trace;
    if(!cm36652_obj)
	{
		APS_ERR("cm36652_obj is null!!\n");
		return 0;
	}
	
	if(1 == sscanf(buf, "0x%x", &trace))
	{
		atomic_set(&cm36652_obj->trace, trace);
	}
	else 
	{
		APS_ERR("invalid content: '%s', length = %d\n", buf, count);
	}
	return count;    
}
/*----------------------------------------------------------------------------*/
static ssize_t cm36652_show_als(struct device_driver *ddri, char *buf)
{
	int res;
	
	if(!cm36652_obj)
	{
		APS_ERR("cm36652_obj is null!!\n");
		return 0;
	}
	if((res = cm36652_read_als(cm36652_obj->client, &cm36652_obj->als)))
	{
		return snprintf(buf, PAGE_SIZE, "ERROR: %d\n", res);
	}
	else
	{
		return snprintf(buf, PAGE_SIZE, "0x%04X\n", cm36652_obj->als);     
	}
}
/*----------------------------------------------------------------------------*/
static ssize_t cm36652_show_ps(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	if(!cm36652_obj)
	{
		APS_ERR("cm3623_obj is null!!\n");
		return 0;
	}
	
	if((res = cm36652_read_ps(cm36652_obj->client, &cm36652_obj->ps)))
	{
		return snprintf(buf, PAGE_SIZE, "ERROR: %d\n", res);
	}
	else
	{
		return snprintf(buf, PAGE_SIZE, "0x%04X\n", cm36652_obj->ps);     
	}
}
/*----------------------------------------------------------------------------*/
static ssize_t cm36652_show_reg(struct device_driver *ddri, char *buf)
{
	if(!cm36652_obj)
	{
		APS_ERR("cm36652_obj is null!!\n");
		return 0;
	}
	
	
	return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t cm36652_show_send(struct device_driver *ddri, char *buf)
{
    return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t cm36652_store_send(struct device_driver *ddri, const char *buf, size_t count)
{
	int addr, cmd;
	u8 dat;

	if(!cm36652_obj)
	{
		APS_ERR("cm36652_obj is null!!\n");
		return 0;
	}
	else if(2 != sscanf(buf, "%x %x", &addr, &cmd))
	{
		APS_ERR("invalid format: '%s'\n", buf);
		return 0;
	}

	dat = (u8)cmd;
	//****************************
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t cm36652_show_recv(struct device_driver *ddri, char *buf)
{
    return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t cm36652_store_recv(struct device_driver *ddri, const char *buf, size_t count)
{
	int addr;
	//u8 dat;
	if(!cm36652_obj)
	{
		APS_ERR("cm36652_obj is null!!\n");
		return 0;
	}
	else if(1 != sscanf(buf, "%x", &addr))
	{
		APS_ERR("invalid format: '%s'\n", buf);
		return 0;
	}

	//****************************
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t cm36652_show_status(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	
	if(!cm36652_obj)
	{
		APS_ERR("cm36652_obj is null!!\n");
		return 0;
	}
	
	if(cm36652_obj->hw)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d, (%d %d)\n", 
			cm36652_obj->hw->i2c_num, cm36652_obj->hw->power_id, cm36652_obj->hw->power_vol);
	}
	else
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");
	}
	
	len += snprintf(buf+len, PAGE_SIZE-len, "REGS: %02X %02X %02X %02lX %02lX\n", 
				atomic_read(&cm36652_obj->als_cmd_val), atomic_read(&cm36652_obj->ps_cmd_val), 
				atomic_read(&cm36652_obj->ps_thd_val),cm36652_obj->enable, cm36652_obj->pending_intr);
	
	len += snprintf(buf+len, PAGE_SIZE-len, "MISC: %d %d\n", atomic_read(&cm36652_obj->als_suspend), atomic_read(&cm36652_obj->ps_suspend));

	return len;
}
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
#define IS_SPACE(CH) (((CH) == ' ') || ((CH) == '\n'))
/*----------------------------------------------------------------------------*/
static int read_int_from_buf(struct cm36652_priv *obj, const char* buf, size_t count, u32 data[], int len)
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
static ssize_t cm36652_show_alslv(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	int idx;
	if(!cm36652_obj)
	{
		APS_ERR("cm36652_obj is null!!\n");
		return 0;
	}
	
	for(idx = 0; idx < cm36652_obj->als_level_num; idx++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "%d ", cm36652_obj->hw->als_level[idx]);
	}
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	return len;    
}
/*----------------------------------------------------------------------------*/
static ssize_t cm36652_store_alslv(struct device_driver *ddri, const char *buf, size_t count)
{
	if(!cm36652_obj)
	{
		APS_ERR("cm36652_obj is null!!\n");
		return 0;
	}
	else if(!strcmp(buf, "def"))
	{
		memcpy(cm36652_obj->als_level, cm36652_obj->hw->als_level, sizeof(cm36652_obj->als_level));
	}
	else if(cm36652_obj->als_level_num != read_int_from_buf(cm36652_obj, buf, count, 
			cm36652_obj->hw->als_level, cm36652_obj->als_level_num))
	{
		APS_ERR("invalid format: '%s'\n", buf);
	}    
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t cm36652_show_alsval(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	int idx;
	if(!cm36652_obj)
	{
		APS_ERR("cm36652_obj is null!!\n");
		return 0;
	}
	
	for(idx = 0; idx < cm36652_obj->als_value_num; idx++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "%d ", cm36652_obj->hw->als_value[idx]);
	}
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	return len;    
}
/*----------------------------------------------------------------------------*/
static ssize_t cm36652_store_alsval(struct device_driver *ddri, const char *buf, size_t count)
{
	if(!cm36652_obj)
	{
		APS_ERR("cm36652_obj is null!!\n");
		return 0;
	}
	else if(!strcmp(buf, "def"))
	{
		memcpy(cm36652_obj->als_value, cm36652_obj->hw->als_value, sizeof(cm36652_obj->als_value));
	}
	else if(cm36652_obj->als_value_num != read_int_from_buf(cm36652_obj, buf, count, 
			cm36652_obj->hw->als_value, cm36652_obj->als_value_num))
	{
		APS_ERR("invalid format: '%s'\n", buf);
	}    
	return count;
}
/*---------------------------------------------------------------------------------------*/
static DRIVER_ATTR(als,     S_IWUSR | S_IRUGO, cm36652_show_als, NULL);
static DRIVER_ATTR(ps,      S_IWUSR | S_IRUGO, cm36652_show_ps, NULL);
static DRIVER_ATTR(config,  S_IWUSR | S_IRUGO, cm36652_show_config,	cm36652_store_config);
static DRIVER_ATTR(alslv,   S_IWUSR | S_IRUGO, cm36652_show_alslv, cm36652_store_alslv);
static DRIVER_ATTR(alsval,  S_IWUSR | S_IRUGO, cm36652_show_alsval, cm36652_store_alsval);
static DRIVER_ATTR(trace,   S_IWUSR | S_IRUGO, cm36652_show_trace,		cm36652_store_trace);
static DRIVER_ATTR(status,  S_IWUSR | S_IRUGO, cm36652_show_status, NULL);
static DRIVER_ATTR(send,    S_IWUSR | S_IRUGO, cm36652_show_send, cm36652_store_send);
static DRIVER_ATTR(recv,    S_IWUSR | S_IRUGO, cm36652_show_recv, cm36652_store_recv);
static DRIVER_ATTR(reg,     S_IWUSR | S_IRUGO, cm36652_show_reg, NULL);
/*----------------------------------------------------------------------------*/
static struct driver_attribute *cm36652_attr_list[] = {
    &driver_attr_als,
    &driver_attr_ps,    
    &driver_attr_trace,        /*trace log*/
    &driver_attr_config,
    &driver_attr_alslv,
    &driver_attr_alsval,
    &driver_attr_status,
    &driver_attr_send,
    &driver_attr_recv,
    &driver_attr_reg,
};

/*----------------------------------------------------------------------------*/
static int cm36652_create_attr(struct device_driver *driver) 
{
	int idx, err = 0;
	int num = (int)(sizeof(cm36652_attr_list)/sizeof(cm36652_attr_list[0]));
	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if((err = driver_create_file(driver, cm36652_attr_list[idx])))
		{            
			APS_ERR("driver_create_file (%s) = %d\n", cm36652_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}
/*----------------------------------------------------------------------------*/
	static int cm36652_delete_attr(struct device_driver *driver)
	{
	int idx ,err = 0;
	int num = (int)(sizeof(cm36652_attr_list)/sizeof(cm36652_attr_list[0]));

	if (!driver)
	return -EINVAL;

	for (idx = 0; idx < num; idx++) 
	{
		driver_remove_file(driver, cm36652_attr_list[idx]);
	}
	
	return err;
}
/*----------------------------------------------------------------------------*/

/*----------------------------------interrupt functions--------------------------------*/
static int intr_flag = 0;
/*----------------------------------------------------------------------------*/
static int cm36652_check_intr(struct i2c_client *client) 
{
	int res;
	u8 databuf[2];
	//u8 intr;
	
	databuf[0] = CM36652_REG_PS_DATA;
	res = CM36652_i2c_master_operate(client, databuf, 0x201, I2C_FLAG_READ);
	if(res<0)
	{
		APS_ERR("i2c_master_send function err res = %d\n",res);
		goto EXIT_ERR;
	}

	APS_LOG("CM36652_REG_PS_DATA value value_low = %x, value_reserve = %x\n",databuf[0],databuf[1]);
	
	databuf[0] = CM36652_REG_INT_FLAG;
	res = CM36652_i2c_master_operate(client, databuf, 0x201, I2C_FLAG_READ);
	if(res<0)
	{
		APS_ERR("i2c_master_send function err res = %d\n",res);
		goto EXIT_ERR;
	}
	
	APS_LOG("CM36652_REG_INT_FLAG value value_low = %x, value_high = %x\n",databuf[0],databuf[1]);
	
	if(databuf[1]&0x02)
	{
		intr_flag = 0;//for close
	}else if(databuf[1]&0x01)
	{
		intr_flag = 1;//for away
	}else{
		res = -1;
		APS_ERR("cm36652_check_intr fail databuf[1]&0x01: %d\n", res);
		goto EXIT_ERR;
	}
	
	return 0;
	EXIT_ERR:
	APS_ERR("cm36652_check_intr dev: %d\n", res);
	return res;
}
/*----------------------------------------------------------------------------*/
static void cm36652_eint_work(struct work_struct *work)
{
	struct cm36652_priv *obj = (struct cm36652_priv *)container_of(work, struct cm36652_priv, eint_work);
	hwm_sensor_data sensor_data;
	int res = 0;

	APS_LOG("cm36652 int top half time = %lld\n", int_top_time);

	res = cm36652_check_intr(obj->client);
	if(res != 0){
		goto EXIT_INTR_ERR;
	}else{
		sensor_data.values[0] = intr_flag;
		sensor_data.value_divide = 1;
		sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;	

	}
	if((res = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data)))
		{
		  APS_ERR("call hwmsen_get_interrupt_data fail = %d\n", res);
		  goto EXIT_INTR_ERR;
		}

#ifdef CUST_EINT_ALS_TYPE
	mt_eint_unmask(CUST_EINT_ALS_NUM);
#else
	mt65xx_eint_unmask(CUST_EINT_ALS_NUM);
#endif
	return;
	EXIT_INTR_ERR:
#ifdef CUST_EINT_ALS_TYPE
	mt_eint_unmask(CUST_EINT_ALS_NUM);
#else
	mt65xx_eint_unmask(CUST_EINT_ALS_NUM);
#endif
	APS_ERR("cm36652_eint_work err: %d\n", res);
}
/*----------------------------------------------------------------------------*/
static void cm36652_eint_func(void)
{
	struct cm36652_priv *obj = g_cm36652_ptr;
	if(!obj)
	{
		return;
	}	
	int_top_time = sched_clock();
	schedule_work(&obj->eint_work);
}

int cm36652_setup_eint(struct i2c_client *client)
{
#if 1
	struct cm36652_priv *obj = i2c_get_clientdata(client);        

	g_cm36652_ptr = obj;
	
	mt_set_gpio_dir(GPIO_ALS_EINT_PIN, GPIO_DIR_IN);
	mt_set_gpio_mode(GPIO_ALS_EINT_PIN, GPIO_ALS_EINT_PIN_M_EINT);
	mt_set_gpio_pull_enable(GPIO_ALS_EINT_PIN, TRUE);
	mt_set_gpio_pull_select(GPIO_ALS_EINT_PIN, GPIO_PULL_UP);

#ifdef CUST_EINT_ALS_TYPE
	mt_eint_set_hw_debounce(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_CN);
	mt_eint_registration(CUST_EINT_ALS_NUM, CUST_EINT_ALS_TYPE, cm36652_eint_func, 0);
#else
	mt65xx_eint_set_sens(CUST_EINT_ALS_NUM, CUST_EINT_ALS_SENSITIVE);
	mt65xx_eint_set_polarity(CUST_EINT_ALS_NUM, CUST_EINT_ALS_POLARITY);
	mt65xx_eint_set_hw_debounce(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_CN);
	mt65xx_eint_registration(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_EN, CUST_EINT_ALS_POLARITY, cm36652_eint_func, 0);
#endif

#ifdef CUST_EINT_ALS_TYPE
	mt_eint_unmask(CUST_EINT_ALS_NUM);
#else
	mt65xx_eint_unmask(CUST_EINT_ALS_NUM);  
#endif

#endif
    return 0;
}
/*-------------------------------MISC device related------------------------------------------*/



/************************************************************/
static int cm36652_open(struct inode *inode, struct file *file)
{
	file->private_data = cm36652_i2c_client;

	if (!file->private_data)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}
/************************************************************/
static int cm36652_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}
/************************************************************/
static int set_psensor_threshold(struct i2c_client *client)
{
	struct cm36652_priv *obj = i2c_get_clientdata(client);
	u8 databuf[3];    
	int res = 0;
	APS_ERR("set_psensor_threshold function high: 0x%x, low:0x%x\n",atomic_read(&obj->ps_thd_val_high),atomic_read(&obj->ps_thd_val_low));
	databuf[0] = CM36652_REG_PS_THD;
	databuf[1] = atomic_read(&obj->ps_thd_val_low);
	databuf[2] = atomic_read(&obj->ps_thd_val_high);//threshold value need to confirm
	res = CM36652_i2c_master_operate(client, databuf, 0x3, I2C_FLAG_WRITE);
	if(res <= 0)
	{
		APS_ERR("i2c_master_send function err\n");
		return -1;
	}
	return 0;

}
static long cm36652_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
		struct i2c_client *client = (struct i2c_client*)file->private_data;
		struct cm36652_priv *obj = i2c_get_clientdata(client);  
		long err = 0;
		void __user *ptr = (void __user*) arg;
		int dat;
		uint32_t enable;
		int ps_result;
		int ps_cali;
		int threshold[2];
		
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
					if((err = cm36652_enable_ps(obj->client, 1)))
					{
						APS_ERR("enable ps fail: %ld\n", err); 
						goto err_out;
					}
					
					set_bit(CMC_BIT_PS, &obj->enable);
				}
				else
				{
					if((err = cm36652_enable_ps(obj->client, 0)))
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
				if((err = cm36652_read_ps(obj->client, &obj->ps)))
				{
					goto err_out;
				}
				
				dat = cm36652_get_ps_value(obj, obj->ps);
				if(copy_to_user(ptr, &dat, sizeof(dat)))
				{
					err = -EFAULT;
					goto err_out;
				}  
				break;
	
			case ALSPS_GET_PS_RAW_DATA:    
				if((err = cm36652_read_ps(obj->client, &obj->ps)))
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
					if((err = cm36652_enable_als(obj->client, 1)))
					{
						APS_ERR("enable als fail: %ld\n", err); 
						goto err_out;
					}
					set_bit(CMC_BIT_ALS, &obj->enable);
				}
				else
				{
					if((err = cm36652_enable_als(obj->client, 0)))
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
				if((err = cm36652_read_als(obj->client, &obj->als)))
				{
					goto err_out;
				}
	
				dat = cm36652_get_als_value(obj, obj->als);
				if(copy_to_user(ptr, &dat, sizeof(dat)))
				{
					err = -EFAULT;
					goto err_out;
				}			   
				break;
	
			case ALSPS_GET_ALS_RAW_DATA:	
				if((err = cm36652_read_als(obj->client, &obj->als)))
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

			/*----------------------------------for factory mode test---------------------------------------*/
			case ALSPS_GET_PS_TEST_RESULT:
				if((err = cm36652_read_ps(obj->client, &obj->ps)))
				{
					goto err_out;
				}
				if(obj->ps > atomic_read(&obj->ps_thd_val_high))
					{
						ps_result = 0;
					}
				else	ps_result = 1;
				
				if(copy_to_user(ptr, &ps_result, sizeof(ps_result)))
				{
					err = -EFAULT;
					goto err_out;
				}			   
				break;

			case ALSPS_IOCTL_CLR_CALI:
				if(copy_from_user(&dat, ptr, sizeof(dat)))
				{
					err = -EFAULT;
					goto err_out;
				}
				if(dat == 0)
					obj->ps_cali = 0;
				break;

			case ALSPS_IOCTL_GET_CALI:
				ps_cali = obj->ps_cali ;
				if(copy_to_user(ptr, &ps_cali, sizeof(ps_cali)))
				{
					err = -EFAULT;
					goto err_out;
				}
				break;

			case ALSPS_IOCTL_SET_CALI:
				if(copy_from_user(&ps_cali, ptr, sizeof(ps_cali)))
				{
					err = -EFAULT;
					goto err_out;
				}

				obj->ps_cali = ps_cali;
				break;

			case ALSPS_SET_PS_THRESHOLD:
				if(copy_from_user(threshold, ptr, sizeof(threshold)))
				{
					err = -EFAULT;
					goto err_out;
				}
				APS_ERR("%s set threshold high: 0x%x, low: 0x%x\n", __func__, threshold[0],threshold[1]); 
				atomic_set(&obj->ps_thd_val_high,  (threshold[0]+obj->ps_cali));
				atomic_set(&obj->ps_thd_val_low,  (threshold[1]+obj->ps_cali));//need to confirm

				set_psensor_threshold(obj->client);
				
				break;
				
			case ALSPS_GET_PS_THRESHOLD_HIGH:
				threshold[0] = atomic_read(&obj->ps_thd_val_high) - obj->ps_cali;
				APS_ERR("%s get threshold high: 0x%x\n", __func__, threshold[0]); 
				if(copy_to_user(ptr, &threshold[0], sizeof(threshold[0])))
				{
					err = -EFAULT;
					goto err_out;
				}
				break;
				
			case ALSPS_GET_PS_THRESHOLD_LOW:
				threshold[0] = atomic_read(&obj->ps_thd_val_low) - obj->ps_cali;
				APS_ERR("%s get threshold low: 0x%x\n", __func__, threshold[0]); 
				if(copy_to_user(ptr, &threshold[0], sizeof(threshold[0])))
				{
					err = -EFAULT;
					goto err_out;
				}
				break;
			/*------------------------------------------------------------------------------------------*/
			
			default:
				APS_ERR("%s not supported = 0x%04x", __FUNCTION__, cmd);
				err = -ENOIOCTLCMD;
				break;
		}
	
		err_out:
		return err;    
	}
/********************************************************************/
/*------------------------------misc device related operation functions------------------------------------*/
static struct file_operations cm36652_fops = {
	.owner = THIS_MODULE,
	.open = cm36652_open,
	.release = cm36652_release,
	.unlocked_ioctl = cm36652_unlocked_ioctl,
};

static struct miscdevice cm36652_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "als_ps",
	.fops = &cm36652_fops,
};

/*--------------------------------------------------------------------------------------*/
static void cm36652_early_suspend(struct early_suspend *h)
{
		struct cm36652_priv *obj = container_of(h, struct cm36652_priv, early_drv);	
		int err;
		APS_FUN();	  
	
		if(!obj)
		{
			APS_ERR("null pointer!!\n");
			return;
		}
		
		atomic_set(&obj->als_suspend, 1);
		if((err = cm36652_enable_als(obj->client, 0)))
		{
			APS_ERR("disable als fail: %d\n", err); 
		}
}

static void cm36652_late_resume(struct early_suspend *h) 
{
		struct cm36652_priv *obj = container_of(h, struct cm36652_priv, early_drv);		  
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
		if(test_bit(CMC_BIT_ALS, &obj->enable))
		{
			if((err = cm36652_enable_als(obj->client, 1)))
			{
				APS_ERR("enable als fail: %d\n", err);		  
	
			}
		}
}
/*--------------------------------------------------------------------------------*/
static int cm36652_init_client(struct i2c_client *client)
{
	struct cm36652_priv *obj = i2c_get_clientdata(client);
	u8 databuf[3];    
	int res = 0;
	APS_FUN();
	databuf[0] = CM36652_REG_CS_CONF;
	if(1 == obj->hw->polling_mode_als)
	databuf[1] = 0b00010001;
	else
	databuf[1] = 0b00010111;	
	databuf[2] = 0b00000000;
	res = CM36652_i2c_master_operate(client, databuf, 0x3, I2C_FLAG_WRITE);
	if(res <= 0)
	{
		APS_ERR("i2c_master_send function err\n");
		goto EXIT_ERR;
	}
	APS_LOG("cm36652 ps CM36652_REG_CS_CONF command!\n");
	
	databuf[0] = CM36652_REG_PS_CONF1_2;
	databuf[1] = 0b00011001;
	if(1 == obj->hw->polling_mode_ps)
	databuf[2] = 0b01100000;
	else
	databuf[2] = 0b01100010;
	res = CM36652_i2c_master_operate(client, databuf, 0x3, I2C_FLAG_WRITE);
	if(res <= 0)
	{
		APS_ERR("i2c_master_send function err\n");
		goto EXIT_ERR;
	}
	APS_LOG("cm36652 ps CM36652_REG_PS_CONF1_2 command!\n");
	
	databuf[0] = CM36652_REG_PS_CANC;//value need to confirm
	databuf[1] = 0x00;
	databuf[2] = 0x00;
	res = CM36652_i2c_master_operate(client, databuf, 0x3, I2C_FLAG_WRITE);
	if(res <= 0)
	{
		APS_ERR("i2c_master_send function err\n");
		goto EXIT_ERR;
	}
	
	APS_LOG("cm36652 ps CM36652_REG_PS_CANC command!\n");
	
	if(0 == obj->hw->polling_mode_als){
			databuf[0] = CM36652_REG_ALS_THDH;
			databuf[1] = 0x00;
			databuf[2] = atomic_read(&obj->als_thd_val_high);
			res = CM36652_i2c_master_operate(client, databuf, 0x3, I2C_FLAG_WRITE);
			if(res <= 0)
			{
				APS_ERR("i2c_master_send function err\n");
				goto EXIT_ERR;
			}
			databuf[0] = CM36652_REG_ALS_THDL;
			databuf[1] = 0x00;
			databuf[2] = atomic_read(&obj->als_thd_val_low);//threshold value need to confirm
			res = CM36652_i2c_master_operate(client, databuf, 0x3, I2C_FLAG_WRITE);
			if(res <= 0)
			{
				APS_ERR("i2c_master_send function err\n");
				goto EXIT_ERR;
			}
		}
	if(0 == obj->hw->polling_mode_ps){
			databuf[0] = CM36652_REG_PS_THD;
			databuf[1] = atomic_read(&obj->ps_thd_val_low);
			databuf[2] = atomic_read(&obj->ps_thd_val_high);//threshold value need to confirm
			res = CM36652_i2c_master_operate(client, databuf, 0x3, I2C_FLAG_WRITE);
			if(res <= 0)
			{
				APS_ERR("i2c_master_send function err\n");
				goto EXIT_ERR;
			}
		}
	res = cm36652_setup_eint(client);
	if(res!=0)
	{
		APS_ERR("setup eint: %d\n", res);
		return res;
	}
	
	return CM36652_SUCCESS;
	
	EXIT_ERR:
	APS_ERR("init dev: %d\n", res);
	return res;
}
/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/
int cm36652_ps_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
		int err = 0;
		int value;
		hwm_sensor_data* sensor_data;
		struct cm36652_priv *obj = (struct cm36652_priv *)self;		
		//APS_FUN(f);
		switch (command)
		{
			case SENSOR_DELAY:
				//APS_ERR("cm36652 ps delay command!\n");
				if((buff_in == NULL) || (size_in < sizeof(int)))
				{
					APS_ERR("Set delay parameter error!\n");
					err = -EINVAL;
				}
				break;
	
			case SENSOR_ENABLE:
				//APS_ERR("cm36652 ps enable command!\n");
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
						if((err = cm36652_enable_ps(obj->client, 1)))
						{
							APS_ERR("enable ps fail: %d\n", err); 
							return -1;
						}
						set_bit(CMC_BIT_PS, &obj->enable);
					}
					else
					{
						if((err = cm36652_enable_ps(obj->client, 0)))
						{
							APS_ERR("disable ps fail: %d\n", err); 
							return -1;
						}
						clear_bit(CMC_BIT_PS, &obj->enable);
					}
				}
				break;
	
			case SENSOR_GET_DATA:
				//APS_ERR("cm36652 ps get data command!\n");
				if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
				{
					APS_ERR("get sensor data parameter error!\n");
					err = -EINVAL;
				}
				else
				{
					sensor_data = (hwm_sensor_data *)buff_out;				
					
					if((err = cm36652_read_ps(obj->client, &obj->ps)))
					{
						err = -1;;
					}
					else
					{
						sensor_data->values[0] = cm36652_get_ps_value(obj, obj->ps);
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

int cm36652_als_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
		int err = 0;
		int value;
		hwm_sensor_data* sensor_data;
		struct cm36652_priv *obj = (struct cm36652_priv *)self;
		//APS_FUN(f);
		switch (command)
		{
			case SENSOR_DELAY:
				//APS_ERR("cm36652 als delay command!\n");
				if((buff_in == NULL) || (size_in < sizeof(int)))
				{
					APS_ERR("Set delay parameter error!\n");
					err = -EINVAL;
				}
				break;
	
			case SENSOR_ENABLE:
				//APS_ERR("cm36652 als enable command!\n");
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
						if((err = cm36652_enable_als(obj->client, 1)))
						{
							APS_ERR("enable als fail: %d\n", err); 
							return -1;
						}
						set_bit(CMC_BIT_ALS, &obj->enable);
					}
					else
					{
						if((err = cm36652_enable_als(obj->client, 0)))
						{
							APS_ERR("disable als fail: %d\n", err); 
							return -1;
						}
						clear_bit(CMC_BIT_ALS, &obj->enable);
					}
					
				}
				break;
	
			case SENSOR_GET_DATA:
				//APS_ERR("cm36652 als get data command!\n");
				if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
				{
					APS_ERR("get sensor data parameter error!\n");
					err = -EINVAL;
				}
				else
				{
					sensor_data = (hwm_sensor_data *)buff_out;
									
					if((err = cm36652_read_als(obj->client, &obj->als)))
					{
						err = -1;;
					}
					else
					{
						sensor_data->values[0] = cm36652_get_als_value(obj, obj->als);
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
/*--------------------------------------------------------------------------------*/


/*-----------------------------------i2c operations----------------------------------*/
static int cm36652_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct cm36652_priv *obj;
	struct hwmsen_object obj_ps, obj_als;
	int err = 0;
	APS_FUN();
	
	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}
	
	memset(obj, 0, sizeof(*obj));
	cm36652_obj = obj;
	
	obj->hw = get_cust_alsps_hw();//get custom file data struct
	
	INIT_WORK(&obj->eint_work, cm36652_eint_work);

	obj->client = client;
	i2c_set_clientdata(client, obj);
	
	/*-----------------------------value need to be confirmed-----------------------------------------*/
	atomic_set(&obj->als_debounce, 200);
	atomic_set(&obj->als_deb_on, 0);
	atomic_set(&obj->als_deb_end, 0);
	atomic_set(&obj->ps_debounce, 200);
	atomic_set(&obj->ps_deb_on, 0);
	atomic_set(&obj->ps_deb_end, 0);
	atomic_set(&obj->ps_mask, 0);
	atomic_set(&obj->als_suspend, 0);
	atomic_set(&obj->als_cmd_val, 0xDF);
	atomic_set(&obj->ps_cmd_val,  0xC1);
	atomic_set(&obj->ps_thd_val_high,  obj->hw->ps_threshold_high);
	atomic_set(&obj->ps_thd_val_low,  obj->hw->ps_threshold_low);
	atomic_set(&obj->als_thd_val_high,  obj->hw->als_threshold_high);
	atomic_set(&obj->als_thd_val_low,  obj->hw->als_threshold_low);
	
	obj->enable = 0;
	obj->pending_intr = 0;
	obj->ps_cali = 0;
	obj->als_level_num = sizeof(obj->hw->als_level)/sizeof(obj->hw->als_level[0]);
	obj->als_value_num = sizeof(obj->hw->als_value)/sizeof(obj->hw->als_value[0]);
	/*-----------------------------value need to be confirmed-----------------------------------------*/
	
	BUG_ON(sizeof(obj->als_level) != sizeof(obj->hw->als_level));
	memcpy(obj->als_level, obj->hw->als_level, sizeof(obj->als_level));
	BUG_ON(sizeof(obj->als_value) != sizeof(obj->hw->als_value));
	memcpy(obj->als_value, obj->hw->als_value, sizeof(obj->als_value));
	atomic_set(&obj->i2c_retry, 3);
	set_bit(CMC_BIT_ALS, &obj->enable);
	set_bit(CMC_BIT_PS, &obj->enable);

	cm36652_i2c_client = client;

	if((err = cm36652_init_client(client)))
	{
		goto exit_init_failed;
	}
	APS_LOG("cm36652_init_client() OK!\n");

	if((err = misc_register(&cm36652_device)))
	{
		APS_ERR("cm36652_device register failed\n");
		goto exit_misc_device_register_failed;
	}
	APS_LOG("cm36652_device misc_register OK!\n");

	/*------------------------cm36652 attribute file for debug--------------------------------------*/
	if((err = cm36652_create_attr(&cm36652_alsps_driver.driver)))
	{
		APS_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}
	/*------------------------cm36652 attribute file for debug--------------------------------------*/

	obj_ps.self = cm36652_obj;
	obj_ps.polling = obj->hw->polling_mode_ps;	
	obj_ps.sensor_operate = cm36652_ps_operate;
	if((err = hwmsen_attach(ID_PROXIMITY, &obj_ps)))
	{
		APS_ERR("attach fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}
		
	obj_als.self = cm36652_obj;
	obj_als.polling = obj->hw->polling_mode_als;;
	obj_als.sensor_operate = cm36652_als_operate;
	if((err = hwmsen_attach(ID_LIGHT, &obj_als)))
	{
		APS_ERR("attach fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	#if defined(CONFIG_HAS_EARLYSUSPEND)
	obj->early_drv.level    = EARLY_SUSPEND_LEVEL_STOP_DRAWING - 2,
	obj->early_drv.suspend  = cm36652_early_suspend,
	obj->early_drv.resume   = cm36652_late_resume,    
	register_early_suspend(&obj->early_drv);
	#endif

	APS_LOG("%s: OK\n", __func__);
	return 0;

	exit_create_attr_failed:
	exit_sensor_obj_attach_fail:
	exit_misc_device_register_failed:
		misc_deregister(&cm36652_device);
	exit_init_failed:
		kfree(obj);
	exit:
	cm36652_i2c_client = NULL;           
	APS_ERR("%s: err = %d\n", __func__, err);
	return err;
}

static int cm36652_i2c_remove(struct i2c_client *client)
{
	int err;	
	/*------------------------cm36652 attribute file for debug--------------------------------------*/	
	if((err = cm36652_delete_attr(&cm36652_i2c_driver.driver)))
	{
		APS_ERR("cm36652_delete_attr fail: %d\n", err);
	} 
	/*----------------------------------------------------------------------------------------*/
	
	if((err = misc_deregister(&cm36652_device)))
	{
		APS_ERR("misc_deregister fail: %d\n", err);    
	}
		
	cm36652_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));
	return 0;

}

static int cm36652_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	strcpy(info->type, CM36652_DEV_NAME);
	return 0;

}

static int cm36652_i2c_suspend(struct i2c_client *client, pm_message_t msg)
{
	APS_FUN();
	return 0;
}

static int cm36652_i2c_resume(struct i2c_client *client)
{
	APS_FUN();
	return 0;
}

/*----------------------------------------------------------------------------*/

static int cm36652_probe(struct platform_device *pdev) 
{
	//APS_FUN();  
	struct alsps_hw *hw = get_cust_alsps_hw();

	cm36652_power(hw, 1); //*****************   
	
	if(i2c_add_driver(&cm36652_i2c_driver))
	{
		APS_ERR("add driver error\n");
		return -1;
	} 
	return 0;
}
/*----------------------------------------------------------------------------*/
static int cm36652_remove(struct platform_device *pdev)
{
	//APS_FUN(); 
	struct alsps_hw *hw = get_cust_alsps_hw();
	
	cm36652_power(hw, 0);//*****************  
	
	i2c_del_driver(&cm36652_i2c_driver);
	return 0;
}



/*----------------------------------------------------------------------------*/
static struct platform_driver cm36652_alsps_driver = {
	.probe      = cm36652_probe,
	.remove     = cm36652_remove,    
	.driver     = {
		.name  = "als_ps",
	}
};

/*----------------------------------------------------------------------------*/
static int __init cm36652_init(void)
{
	//APS_FUN();
	struct alsps_hw *hw = get_cust_alsps_hw();
	APS_LOG("%s: i2c_number=%d, i2c_addr: 0x%x\n", __func__, hw->i2c_num, hw->i2c_addr[0]);
	i2c_register_board_info(hw->i2c_num, &i2c_cm36652, 1);
	if(platform_driver_register(&cm36652_alsps_driver))
	{
		APS_ERR("failed to register driver");
		return -ENODEV;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit cm36652_exit(void)
{
	APS_FUN();
	platform_driver_unregister(&cm36652_alsps_driver);
}
/*----------------------------------------------------------------------------*/
module_init(cm36652_init);
module_exit(cm36652_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("yucong xiong");
MODULE_DESCRIPTION("cm36652 driver");
MODULE_LICENSE("GPL");

