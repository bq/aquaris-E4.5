/* alps/ALPS_SW/TRUNK/MAIN/alps/kernel/drivers/hwmon/mt6516/hwmsen_dev.c
 *
 * (C) Copyright 2009 
 * MediaTek <www.MediaTek.com>
 *
 * Sensor devices
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/slab.h>


#include <linux/hwmsensor.h>
#include <linux/hwmsen_helper.h>
#include <linux/hwmsen_dev.h>
//add for fix resume issue
#include <linux/earlysuspend.h> 
#include <linux/wakelock.h>
//add for fix resume issue end

#include <cust_alsps.h>

#define SENSOR_INVALID_VALUE -1
#define MAX_CHOOSE_G_NUM 5
#define MAX_CHOOSE_M_NUM 5

static void hwmsen_early_suspend(struct early_suspend *h);
static void hwmsen_late_resume(struct early_suspend *h);
static void update_workqueue_polling_rate(int newDelay);

static struct workqueue_struct * sensor_workqueue = NULL;

/******************************************************************************
 * structure / enumeration / macro / definition
 *****************************************************************************/

struct sensor_delay 
{
   int handle;
   uint32_t delay;
};

struct hwmsen_context { /*sensor context*/
	atomic_t                enable;
	atomic_t delay;
	uint32_t delayCountSet;
	uint32_t delayCount;
	
	struct hwmsen_object    obj;
};

#if defined(MTK_AUTO_DETECT_ACCELEROMETER)
static char gsensor_name[25];
static struct sensor_init_info* gsensor_init_list[MAX_CHOOSE_G_NUM]= {0}; //modified
#endif
#if defined(MTK_AUTO_DETECT_MAGNETOMETER)
static char msensor_name[25];
static struct sensor_init_info* msensor_init_list[MAX_CHOOSE_G_NUM]= {0}; //modified
#endif
#if defined(MTK_AUTO_DETECT_ALSPS)
static char alsps_name[25];
static struct sensor_init_info* alsps_init_list[MAX_CHOOSE_G_NUM]= {0}; //modified
#endif

/*----------------------------------------------------------------------------*/
struct dev_context {
    int		polling_running;
    struct mutex lock;
    struct hwmsen_context* cxt[MAX_ANDROID_SENSOR_NUM+1];
};
/*-------------Sensor daa-----------------------------------------------------*/
struct hwmsen_data{
	hwm_sensor_data sensors_data[MAX_ANDROID_SENSOR_NUM+1];
	int data_updata[MAX_ANDROID_SENSOR_NUM+1];
	struct mutex lock;
};
/*----------------------------------------------------------------------------*/
typedef enum {
    HWM_TRC_REPORT_NUM = 0x0001,
    HWM_TRC_REPORT_EVT = 0x0002, 
    HWM_TRC_REPORT_INF = 0X0004,
} HWM_TRC;
/*----------------------------------------------------------------------------*/
#define C_MAX_OBJECT_NUM 1
struct hwmdev_object {
	struct input_dev   *idev;
	struct miscdevice   mdev;
	struct dev_context *dc;
	struct work_struct  report;
	atomic_t            delay; /*polling period for reporting input event*/
	atomic_t            wake;  /*user-space request to wake-up, used with stop*/
	struct timer_list   timer;  /* polling timer */
	atomic_t            trace;
	uint32_t			active_sensor;			// Active, but hwmsen don't need data sensor. Maybe other need it's data.
	uint32_t			active_data_sensor;		// Active and hwmsen need data sensor.
	//add for fix resume issue
	struct early_suspend    early_drv;
	struct wake_lock        read_data_wake_lock;
	atomic_t                early_suspend;
	//add for fix resume end
};

static bool enable_again = false;
static struct hwmdev_object *hwm_obj = NULL;
/******************************************************************************
 * static variables
 *****************************************************************************/

static struct hwmsen_data obj_data ={
	.lock =__MUTEX_INITIALIZER(obj_data.lock), 
};
static struct dev_context dev_cxt = {
    .lock = __MUTEX_INITIALIZER(dev_cxt.lock),   
};
/*----------------------------------------------------------------------------*/



/******************************************************************************
 * Local functions
 *****************************************************************************/
static void hwmsen_work_func(struct work_struct *work)
{

	//HWM_LOG("+++++++++++++++++++++++++hwmsen_work_func workqueue performed!+++++++++++++++++++++++++++++\n");
	//struct hwmdev_object *obj = container_of(work, struct hwmdev_object, report);
	struct hwmdev_object *obj = hwm_obj;
	struct hwmsen_context *cxt = NULL;
	int out_size;
	hwm_sensor_data sensor_data;
	uint32_t event_type = 0;
	int64_t  nt;
	struct timespec time; 
	int err, idx;	
	//int trc = atomic_read(&obj->trace);
	
	if (obj == NULL)
	{
		HWM_ERR("obj point is NULL!\n");
		return;
	}
	

	if(atomic_read(&obj->wake))
	{
		input_event(obj->idev, EV_SYN, SYN_CONFIG, 0);
		atomic_set(&obj->wake, 0);		
		return;
	}
	
	memset(&sensor_data, 0, sizeof(sensor_data));	
	time.tv_sec = time.tv_nsec = 0;    
	time = get_monotonic_coarse(); 
	nt = time.tv_sec*1000000000LL+time.tv_nsec;
	//mutex_lock(&obj_data.lock);
	for(idx = 0; idx < MAX_ANDROID_SENSOR_NUM; idx++)
	{
		cxt = obj->dc->cxt[idx];
		if((cxt == NULL) || (cxt->obj.sensor_operate == NULL)
			|| !(obj->active_data_sensor&(0x01<<idx)))
		{
			continue;
		}
		
		// Interrupt sensor
		if(cxt->obj.polling == 0)
		{
			if(obj_data.data_updata[idx] == 1)
			{
				mutex_lock(&obj_data.lock);
				event_type |= (1 << idx);
				obj_data.data_updata[idx] = 0;
				mutex_unlock(&obj_data.lock);
			}
			continue;
		}
		
		
		//added to surpport set delay to specified sensor
		if(cxt->delayCount > 0)
		{
		  //HWM_LOG("sensor(%d) delayCount = %d\n",idx,cxt->delayCount);
		  cxt->delayCount--;
		  if(0 == cxt->delayCount)
		  {
		    cxt->delayCount = cxt->delayCountSet;
			//HWM_LOG("sensor(%d) go to get data\n",idx);
		  }
		  else
		  { 
		    //HWM_LOG("sensor(%d) wait for next work\n",idx);
		    continue;
		  }
		}
		
		err = cxt->obj.sensor_operate(cxt->obj.self,SENSOR_GET_DATA, NULL, 0, 
			&sensor_data, sizeof(hwm_sensor_data), &out_size);
		
		if(err)
		{
			HWM_ERR("get data from sensor (%d) fails!!\n", idx);
			continue;
		}
		else
		{
			if((idx == ID_LIGHT) ||(idx == ID_PRESSURE) 
			||(idx == ID_PROXIMITY) || (idx == ID_TEMPRERATURE))
			{
				// data changed, update the data
				if(sensor_data.values[0] != obj_data.sensors_data[idx].values[0])
				{
					mutex_lock(&obj_data.lock);
					obj_data.sensors_data[idx].values[0] = sensor_data.values[0];
					obj_data.sensors_data[idx].value_divide = sensor_data.value_divide;
					obj_data.sensors_data[idx].status = sensor_data.status;
					obj_data.sensors_data[idx].time = nt;
					event_type |= (1 << idx);
					mutex_unlock(&obj_data.lock);
					//HWM_LOG("get %d sensor, values: %d!\n", idx, sensor_data.values[0]);
				}
			}
			else
			{
				// data changed, update the data
				if((sensor_data.values[0] != obj_data.sensors_data[idx].values[0]) 
					|| (sensor_data.values[1] != obj_data.sensors_data[idx].values[1])
					|| (sensor_data.values[2] != obj_data.sensors_data[idx].values[2]))
				{	
				    if( 0 == sensor_data.values[0] && 0==sensor_data.values[1] 
						&& 0 == sensor_data.values[2])
				    {
				    	
				       continue;
				    }
					mutex_lock(&obj_data.lock);
					obj_data.sensors_data[idx].values[0] = sensor_data.values[0];
					obj_data.sensors_data[idx].values[1] = sensor_data.values[1];
					obj_data.sensors_data[idx].values[2] = sensor_data.values[2];
					obj_data.sensors_data[idx].value_divide = sensor_data.value_divide;
					obj_data.sensors_data[idx].status = sensor_data.status;
					obj_data.sensors_data[idx].time = nt;
					event_type |= (1 << idx);
					mutex_unlock(&obj_data.lock);
					//HWM_LOG("get %d sensor, values: %d, %d, %d!\n", idx, 
						//sensor_data.values[0], sensor_data.values[1], sensor_data.values[2]);
				}
			}
		}			
	}

	//
	//mutex_unlock(&obj_data.lock);

	if(enable_again == true)
	{
		event_type = obj->active_data_sensor;
		enable_again = false;
		//filter -1 value
		for(idx = 0; idx <= MAX_ANDROID_SENSOR_NUM; idx++)
	    {
	       if(ID_ACCELEROMETER==idx || ID_MAGNETIC==idx || ID_ORIENTATION==idx
		   	  ||ID_GYROSCOPE==idx || ID_TEMPRERATURE==idx
		   	  ||ID_LINEAR_ACCELERATION==idx || ID_ROTATION_VECTOR==idx
		   	  ||ID_GRAVITY==idx)
	       {
	          if(SENSOR_INVALID_VALUE == obj_data.sensors_data[idx].values[0] ||
		   	     SENSOR_INVALID_VALUE == obj_data.sensors_data[idx].values[1] ||
		   	     SENSOR_INVALID_VALUE == obj_data.sensors_data[idx].values[2])
	       	  {
	       	     event_type &= ~(1 << idx);
			     //HWM_LOG("idx=%d,obj->active_sensor after clear: %d\n",idx);
	       	  }
	       }

		   if(ID_PROXIMITY==idx || ID_LIGHT==idx || ID_PRESSURE==idx)
	       {
	          if(SENSOR_INVALID_VALUE == obj_data.sensors_data[idx].values[0])
	          {
	       	   event_type &= ~(1 << idx);
			   //HWM_LOG("idx=%d,obj->active_sensor after clear: %d\n",idx);
	          }
	       }
	    }
		//HWM_LOG("event type after enable: %d\n", event_type);
	}
	
	if((event_type&(1 << ID_PROXIMITY))&& SENSOR_INVALID_VALUE == obj_data.sensors_data[ID_PROXIMITY].values[0])
	{
	    event_type &= ~(1 << ID_PROXIMITY);   
		//HWM_LOG("remove ps event!!!!!!!!!!!\n");
	}
	
	if(event_type != 0)
	{		
		input_report_rel(obj->idev, EVENT_TYPE_SENSOR, event_type);
		input_sync(obj->idev);//modified
		//HWM_LOG("event type: %d\n", event_type);
	}
	else
	{
		//HWM_LOG("no available sensor!!\n");
	}

	if(obj->dc->polling_running == 1)
	{
		mod_timer(&obj->timer, jiffies + atomic_read(&obj->delay)/(1000/HZ)); 
	}
}

/******************************************************************************
 * export functions
 *****************************************************************************/
int hwmsen_get_interrupt_data(int sensor, hwm_sensor_data *data) 
{
	//HWM_LOG("++++++++++++++++++++++++++++hwmsen_get_interrupt_data function sensor = %d\n",sensor);
	struct dev_context *mcxt = &dev_cxt;
	struct hwmdev_object *obj = hwm_obj;
	int64_t  nt;
	struct timespec time; 

	if((sensor > MAX_ANDROID_SENSOR_NUM) || (mcxt->cxt[sensor] == NULL) 
		|| (mcxt->cxt[sensor]->obj.polling != 0))
	{
		HWM_ERR("sensor %d!\n", sensor);
		return -EINVAL;
	}
	else
	{		
		time.tv_sec = time.tv_nsec = 0;    
		time = get_monotonic_coarse(); 
		nt = time.tv_sec*1000000000LL+time.tv_nsec;  
		if((sensor == ID_LIGHT) ||(sensor == ID_PRESSURE) 
			||(sensor == ID_PROXIMITY) || (sensor == ID_TEMPRERATURE))
		{
			// data changed, update the data
			if(data->values[0] != obj_data.sensors_data[sensor].values[0])
			{
				mutex_lock(&obj_data.lock);
				obj_data.data_updata[sensor] = 1;
				obj_data.sensors_data[sensor].values[0] = data->values[0];
				obj_data.sensors_data[sensor].time = nt;
				obj_data.sensors_data[sensor].value_divide = data->value_divide;
				mutex_unlock(&obj_data.lock);
			}
		}
		else
		{
			// data changed, update the data
			if((data->values[0] != obj_data.sensors_data[sensor].values[0]) 
				|| (data->values[1] != obj_data.sensors_data[sensor].values[1])
				|| (data->values[2] != obj_data.sensors_data[sensor].values[2]))
			{
				mutex_lock(&obj_data.lock);
				obj_data.sensors_data[sensor].values[0] = data->values[0];
				obj_data.sensors_data[sensor].values[1] = data->values[1];
				obj_data.sensors_data[sensor].values[2] = data->values[2];
				obj_data.sensors_data[sensor].value_divide = data->value_divide;
				obj_data.data_updata[sensor] = 1;
				obj_data.sensors_data[sensor].time = nt;
				mutex_unlock(&obj_data.lock);
			}
		}

		if(obj->dc->polling_running == 1)
		{
			hwmsen_work_func(NULL);
		}
		
		return 0;
	}
}
/*----------------------------------------------------------------------------*/
EXPORT_SYMBOL_GPL(hwmsen_get_interrupt_data);

/*----------------------------------------------------------------------------*/
static void hwmsen_poll(unsigned long data)
{
	struct hwmdev_object *obj = (struct hwmdev_object *)data;
	if(obj != NULL)
	{
		queue_work(sensor_workqueue, &obj->report);
	}
}
/*----------------------------------------------------------------------------*/
static struct hwmdev_object *hwmsen_alloc_object(void)
{
	
	struct hwmdev_object *obj = kzalloc(sizeof(*obj), GFP_KERNEL); 
	HWM_FUN(f);
	
	if(!obj)
	{
		HWM_ERR("Alloc hwmsen object error!\n");
		return NULL;
	}	

	obj->dc = &dev_cxt;
	obj->active_data_sensor = 0;
	obj->active_sensor = 0;
	atomic_set(&obj->delay, 200); /*5Hz*/// set work queue delay time 200ms
	atomic_set(&obj->wake, 0);
	sensor_workqueue = create_singlethread_workqueue("sensor_polling");
	INIT_WORK(&obj->report, hwmsen_work_func);
	init_timer(&obj->timer);
	obj->timer.expires	= jiffies + atomic_read(&obj->delay)/(1000/HZ);
	obj->timer.function	= hwmsen_poll;
	obj->timer.data		= (unsigned long)obj;
	return obj;
}

/*Sensor device driver attach to hwmsen device------------------------------------------------*/
int hwmsen_attach(int sensor, struct hwmsen_object *obj)
{
	
	struct dev_context *mcxt = &dev_cxt;
	int err = 0;
	HWM_FUN(f);
	if((mcxt == NULL) || (sensor > MAX_ANDROID_SENSOR_NUM))
	{
		err = -EINVAL;
		goto err_exit;
	}
	
	mutex_lock(&mcxt->lock);
	if(mcxt->cxt[sensor] != NULL)
	{
		err = -EEXIST;
		goto err_exit;
	}
	else
	{
		mcxt->cxt[sensor] = kzalloc(sizeof(struct hwmsen_context), GFP_KERNEL);
		if(mcxt->cxt[sensor] == NULL)
		{
			err = -EPERM;
			goto err_exit;
		}				
		atomic_set(&mcxt->cxt[sensor]->enable, 0);
		memcpy(&mcxt->cxt[sensor]->obj, obj, sizeof(*obj));
	
	    // add for android2.3 set  sensors default polling delay time is 200ms
	    atomic_set(&mcxt->cxt[sensor]->delay, 200);
	  
	
	}

	err_exit:
	mutex_unlock(&mcxt->lock);	
	return err;
}
/*----------------------------------------------------------------------------*/
EXPORT_SYMBOL_GPL(hwmsen_attach);
/*----------------------------------------------------------------------------*/
int hwmsen_detach(int sensor) 
{
	
	int err = 0;
	struct dev_context *mcxt = &dev_cxt; 
	HWM_FUN(f);
	if ((sensor > MAX_ANDROID_SENSOR_NUM) || (mcxt->cxt[sensor] == NULL))
	{
		err = -EINVAL;
		goto err_exit;
	}

	mutex_lock(&mcxt->lock);
	kfree(mcxt->cxt[sensor]);
	mcxt->cxt[sensor] = NULL;

	err_exit:
	mutex_unlock(&mcxt->lock);
	return 0;
}
/*----------------------------------------------------------------------------*/
EXPORT_SYMBOL_GPL(hwmsen_detach);
/*----------------------------------------------------------------------------*/
static int hwmsen_enable(struct hwmdev_object *obj, int sensor, int enable)
{
	struct hwmsen_context *cxt = NULL;
	int err = 0;
	uint32_t sensor_type;

	sensor_type = 1 << sensor;
	
	if(!obj)
	{
		HWM_ERR("hwmdev obj pointer is NULL!\n");
		return -EINVAL;
	}
	else if(obj->dc->cxt[sensor] == NULL)
	{
		HWM_ERR("the sensor (%d) is not attached!!\n", sensor);
		return -ENODEV;
	}
	

	mutex_lock(&obj->dc->lock);
	cxt = obj->dc->cxt[sensor];    
	
	
	if(enable == 1)
	{
//{@for mt6582 blocking issue work around		
		if(sensor == 7){
			HWM_LOG("P-sensor disable LDO low power\n");
			pmic_ldo_suspend_enable(0);
			}
//@}
		enable_again = true;
		obj->active_data_sensor |= sensor_type;
		if((obj->active_sensor & sensor_type) == 0)	// no no-data active
		{		
			if (cxt->obj.sensor_operate(cxt->obj.self, SENSOR_ENABLE, &enable,sizeof(int), NULL, 0, NULL) != 0)
			{
				if (cxt->obj.sensor_operate(cxt->obj.self, SENSOR_ENABLE, &enable,sizeof(int), NULL, 0, NULL) != 0)
				{
					if (cxt->obj.sensor_operate(cxt->obj.self, SENSOR_ENABLE, &enable,sizeof(int), NULL, 0, NULL) != 0)
					{
						HWM_ERR("activate sensor(%d) 3 times err = %d\n", sensor, err);
						err = -EINVAL;
						goto exit;
					}
				}
				
			}

			atomic_set(&cxt->enable, 1);			
		}

		// Need to complete the interrupt sensor work
		if((0 == obj->dc->polling_running) && (obj->active_data_sensor != 0))
		{
			obj->dc->polling_running = 1;
			//obj->timer.expires = jiffies + atomic_read(&obj->delay)/(1000/HZ);
			//add_timer(&obj->timer);
			mod_timer(&obj->timer, jiffies + atomic_read(&obj->delay)/(1000/HZ)); 
			
		}
		
	}
	else
	{
//{@for mt6582 blocking issue work around
		if(sensor == 7){
			HWM_LOG("P-sensor enable LDO low power\n");
			pmic_ldo_suspend_enable(1);

			}
//@}
		obj->active_data_sensor &= ~sensor_type;
		if((obj->active_sensor & sensor_type) == 0)	// no no-data active
		{
			if(cxt->obj.sensor_operate(cxt->obj.self, SENSOR_ENABLE, &enable,sizeof(int), NULL, 0, NULL) != 0)
			{
				HWM_ERR("deactiva sensor(%d) err = %d\n", sensor, err);
				err = -EINVAL;
				goto exit;
			}

			atomic_set(&cxt->enable, 0);
			update_workqueue_polling_rate(200);// re-update workqueue polling rate
		}		
			     
		if((1 == obj->dc->polling_running) && (obj->active_data_sensor == 0))
		{
			obj->dc->polling_running = 0;
			del_timer_sync(&obj->timer);
			cancel_work_sync(&obj->report);
			
		}

		obj_data.sensors_data[sensor].values[0] = SENSOR_INVALID_VALUE;
		obj_data.sensors_data[sensor].values[1] = SENSOR_INVALID_VALUE;
		obj_data.sensors_data[sensor].values[2] = SENSOR_INVALID_VALUE;
		
	}	
	   
	HWM_LOG("sensor(%d), flag(%d)\n", sensor, enable);

	exit:
		
	mutex_unlock(&obj->dc->lock); 
	return err;
}

/*-------------no data sensor enable/disable--------------------------------------*/
static int hwmsen_enable_nodata(struct hwmdev_object *obj, int sensor, int enable)
{
	struct hwmsen_context *cxt = NULL;
	int err = 0;
	uint32_t sensor_type;
	HWM_FUN(f);
	sensor_type = 1 << sensor;

	if(NULL == obj)
	{
		HWM_ERR("hwmdev obj pointer is NULL!\n");
		return -EINVAL;
	}
	else if(obj->dc->cxt[sensor] == NULL)
	{
		HWM_ERR("the sensor (%d) is not attached!!\n", sensor);
		return -ENODEV;
	}
	

	mutex_lock(&obj->dc->lock);
	cxt = obj->dc->cxt[sensor];

	if(enable == 1)
	{
		obj->active_sensor |= sensor_type;
		
		if((obj->active_data_sensor & sensor_type) == 0)	// no data active
		{
			if(cxt->obj.sensor_operate(cxt->obj.self, SENSOR_ENABLE, &enable, sizeof(int), NULL, 0, NULL) != 0)
			{
				HWM_ERR("activate sensor(%d) err = %d\n", sensor, err);
				err = -EINVAL;
				goto exit;
			}

			atomic_set(&cxt->enable, 1);
		}
	}
	else
	{
		obj->active_sensor &= ~sensor_type;
		
		if((obj->active_data_sensor & sensor_type) == 0)	// no data active
		{
			if(cxt->obj.sensor_operate(cxt->obj.self, SENSOR_ENABLE, &enable,sizeof(int), NULL, 0, NULL) != 0)
			{
				HWM_ERR("Deactivate sensor(%d) err = %d\n", sensor, err);
				err = -EINVAL;
				goto exit;
			}

			atomic_set(&cxt->enable, 0);
		}
		
	}
	
	exit:
		
	mutex_unlock(&obj->dc->lock);
	return err;
}
/*------------set delay--------------------------------------------------------*/
static int hwmsen_set_delay(int delay, int handle )
{
	int err = 0;
	struct hwmsen_context *cxt = NULL;

	cxt = hwm_obj->dc->cxt[handle];
	if(NULL == cxt ||(cxt->obj.sensor_operate == NULL))
	{
	  HWM_ERR("have no this sensor %d or operator point is null!\r\n", handle);
	}
	else if(atomic_read(&cxt->enable) != 0)
	{
		if(cxt->obj.sensor_operate(cxt->obj.self, SENSOR_DELAY, &delay,sizeof(int), NULL, 0, NULL) != 0)
		{
			HWM_ERR("%d sensor's sensor_operate function error %d!\r\n",handle,err);
			return err;
		}
		//record sensor delay
		atomic_set(&cxt->delay, delay);
	}
	return err;
}
/*----------------------------------------------------------------------------*/
static int hwmsen_wakeup(struct hwmdev_object *obj)
{
	HWM_FUN(f);
	if(obj == NULL)
	{
		HWM_ERR("null pointer!!\n");
		return -EINVAL;
	}

	input_event(obj->idev, EV_SYN, SYN_CONFIG, 0);
	
	return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t hwmsen_show_hwmdev(struct device* dev, 
                                 struct device_attribute *attr, char *buf) 
{
	
    //struct hwmdev_object *devobj = (struct hwmdev_object*)dev_get_drvdata(dev);
    int len = 0;
	printk("sensor test: hwmsen_show_hwmdev function!\n");
/*
    if (!devobj || !devobj->dc) {
        HWM_ERR("null pointer: %p, %p", devobj, (!devobj) ? (NULL) : (devobj->dc));
        return 0;
    }
    for (idx = 0; idx < C_MAX_HWMSEN_NUM; idx++) 
        len += snprintf(buf+len, PAGE_SIZE-len, "    %d", idx);
    len += snprintf(buf+len, PAGE_SIZE-len, "\n");
    for (idx = 0; idx < C_MAX_HWMSEN_NUM; idx++)
        len += snprintf(buf+len, PAGE_SIZE-len, "    %d", atomic_read(&devobj->dc->cxt[idx].enable));
    len += snprintf(buf+len, PAGE_SIZE-len, "\n");
    */
    return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t hwmsen_store_active(struct device* dev, struct device_attribute *attr,
                                  const char *buf, size_t count)
{
/*
	printk("sensor test: hwmsen_store_active function!\n");
    struct hwmdev_object *devobj = (struct hwmdev_object*)dev_get_drvdata(dev);
    int sensor, enable, err, idx;

    if (!devobj || !devobj->dc) {
        HWM_ERR("null pointer!!\n");
        return count;
    }

    if (!strncmp(buf, "all-start", 9)) {
        for (idx = 0; idx < C_MAX_HWMSEN_NUM; idx++)
            hwmsen_enable(devobj, idx, 1);
    } else if (!strncmp(buf, "all-stop", 8)) {
        for (idx = 0; idx < C_MAX_HWMSEN_NUM; idx++)
            hwmsen_enable(devobj, idx, 0);    
    } else if (2 == sscanf(buf, "%d %d", &sensor, &enable)) {
        if ((err = hwmsen_enable(devobj, sensor, enable)))
            HWM_ERR("sensor enable failed: %d\n", err);
    } 
*/
    return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t hwmsen_show_delay(struct device* dev, 
                                 struct device_attribute *attr, char *buf) 
{
/*
    struct hwmdev_object *devobj = (struct hwmdev_object*)dev_get_drvdata(dev);
printk("sensor test: hwmsen_show_delay function!\n");
    if (!devobj || !devobj->dc) {
        HWM_ERR("null pointer!!\n");
        return 0;
    }
    
    return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&devobj->delay));    
*/

	return 0;

}
/*----------------------------------------------------------------------------*/
static ssize_t hwmsen_store_delay(struct device* dev, struct device_attribute *attr,
                                  const char *buf, size_t count)
{
/*
    struct hwmdev_object *devobj = (struct hwmdev_object*)dev_get_drvdata(dev);
    int delay;
printk("sensor test: hwmsen_show_delay function!\n");
    if (!devobj || !devobj->dc) {
        HWM_ERR("null pointer!!\n");
        return count;
    }

    if (1 != sscanf(buf, "%d", &delay)) {
        HWM_ERR("invalid format!!\n");
        return count;
    }

    atomic_set(&devobj->delay, delay);
   */
    return count;
}                                 
/*----------------------------------------------------------------------------*/
static ssize_t hwmsen_show_wake(struct device* dev, 
                                 struct device_attribute *attr, char *buf) 
{
/*
	printk("sensor test: hwmsen_show_wake function!\n");
    struct hwmdev_object *devobj = (struct hwmdev_object*)dev_get_drvdata(dev);

    if (!devobj || !devobj->dc) {
        HWM_ERR("null pointer!!\n");
        return 0;
    }
    return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&devobj->wake));    
    */
    return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t hwmsen_store_wake(struct device* dev, struct device_attribute *attr,
                                  const char *buf, size_t count)
{
/*
    struct hwmdev_object *devobj = (struct hwmdev_object*)dev_get_drvdata(dev);
    int wake, err;
printk("sensor test: hwmsen_store_wake function!\n");
    if (!devobj || !devobj->dc) {
        HWM_ERR("null pointer!!\n");
        return count;
    }

    if (1 != sscanf(buf, "%d", &wake)) {
        HWM_ERR("invalid format!!\n");
        return count;
    }

    if ((err = hwmsen_wakeup(devobj))) {
        HWM_ERR("wakeup sensor fail, %d\n", err);
        return count;
    }
   */ 
    return count;
}  

/*----------------------------------------------------------------------------*/
static ssize_t hwmsen_show_trace(struct device *dev, 
                                  struct device_attribute *attr, char *buf)
{
	
	struct i2c_client *client = to_i2c_client(dev);
	struct hwmdev_object *obj = i2c_get_clientdata(client);
	HWM_FUN(f);

	return snprintf(buf, PAGE_SIZE, "0x%08X\n", atomic_read(&obj->trace));
}
/*----------------------------------------------------------------------------*/
static ssize_t hwmsen_store_trace(struct device* dev, 
                                   struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct hwmdev_object *obj = i2c_get_clientdata(client);    
	int trc;
	HWM_FUN(f);

	if (1 == sscanf(buf, "0x%x\n", &trc))
	{
		atomic_set(&obj->trace, trc);
	}
	else
	{
		HWM_ERR("set trace level fail!!\n");
	}
	return count;
}                                                      
/*----------------------------------------------------------------------------*/
DEVICE_ATTR(hwmdev,     S_IWUSR | S_IRUGO, hwmsen_show_hwmdev, NULL);
DEVICE_ATTR(active,     S_IWUSR | S_IRUGO, hwmsen_show_hwmdev, hwmsen_store_active);
DEVICE_ATTR(delay,      S_IWUSR | S_IRUGO, hwmsen_show_delay,  hwmsen_store_delay);
DEVICE_ATTR(wake,       S_IWUSR | S_IRUGO, hwmsen_show_wake,   hwmsen_store_wake);
DEVICE_ATTR(trace,      S_IWUSR | S_IRUGO, hwmsen_show_trace,  hwmsen_store_trace);
/*----------------------------------------------------------------------------*/
static struct device_attribute *hwmsen_attr_list[] =
{
	&dev_attr_hwmdev,
	&dev_attr_active,
	&dev_attr_delay,
	&dev_attr_wake,
	&dev_attr_trace,
};


/*----------------------------------------------------------------------------*/
static int hwmsen_create_attr(struct device *dev) 
{
	int idx, err = 0;
	int num = (int)(sizeof(hwmsen_attr_list)/sizeof(hwmsen_attr_list[0]));
	HWM_FUN();
	if(!dev)
	{
		return -EINVAL;
	}	

	for(idx = 0; idx < num; idx++)
	{
		if((err = device_create_file(dev, hwmsen_attr_list[idx])))
		{            
			HWM_ERR("device_create_file (%s) = %d\n", hwmsen_attr_list[idx]->attr.name, err);        
			break;
		}
	}

	return err;
}
/*----------------------------------------------------------------------------*/
static int hwmsen_delete_attr(struct device *dev)
{
	
	int idx ,err = 0;
	int num = (int)(sizeof(hwmsen_attr_list)/sizeof(hwmsen_attr_list[0]));
    HWM_FUN(f);
	if (!dev)
	{
		return -EINVAL;
	}
	

	for (idx = 0; idx < num; idx++)
	{
		device_remove_file(dev, hwmsen_attr_list[idx]);
	}	

	return err;
}

/*----------------------------------------------------------------*/
static int init_static_data(void)
{
	int i = 0;
//	memset(&obj_data, 0, sizeof(struct hwmsen_data));
//	obj_data.lock = __MUTEX_INITIALIZER(obj_data.lock);	
	for(i=0; i < MAX_ANDROID_SENSOR_NUM; i++)
	{
		dev_cxt.cxt[i] = NULL;		
		memset(&obj_data.sensors_data[i], SENSOR_INVALID_VALUE, sizeof(hwm_sensor_data));
		obj_data.sensors_data[i].sensor = i;
		
	}
	return 0;
}
/*----------------------------------------------------------------*/
static int hwmsen_open(struct inode *node , struct file *fp)
{
	HWM_FUN(f);
	//struct file_private* data = kzalloc(sizeof(struct file_private), GFP_KERNEL);
//	fp->private_data = data;
	fp->private_data = NULL;
	return nonseekable_open(node,fp);
}
/*----------------------------------------------------------------------------*/
static int hwmsen_release(struct inode *node, struct file *fp)
{
	HWM_FUN(f);
	kfree(fp->private_data);
	fp->private_data = NULL;
	return 0;
}
/*----------------------------------------------------------------------------*/
static void update_workqueue_polling_rate(int newDelay)
{
  atomic_t delaytemp;
  int i=0;
  int idx=0;
  struct hwmsen_context *cxt = NULL;
  struct hwmdev_object *obj = hwm_obj;
  HWM_FUN(f);
  atomic_set(&delaytemp, 200);//used to finding fastest sensor polling rate

  for(i = 0; i < MAX_ANDROID_SENSOR_NUM; i++)
  {
	if(hwm_obj->active_data_sensor & 1<<i)
	{
	  if(atomic_read(&delaytemp) > atomic_read(&(hwm_obj->dc->cxt[i]->delay)))
	  {
	     atomic_set(&delaytemp, atomic_read(&(hwm_obj->dc->cxt[i]->delay)));// work queue polling delay base time
	  }
	}
  }
  //use the fastest sensor polling delay as work queue polling delay base time
  if(atomic_read(&delaytemp) > newDelay)
  {
	atomic_set(&hwm_obj->delay, newDelay);// work queue polling delay base time
	HWM_LOG("set new workqueue base time=%d\n",atomic_read(&hwm_obj->delay));
  }
  else
  {
    atomic_set(&hwm_obj->delay, atomic_read(&delaytemp));
	HWM_LOG("set old fastest sensor delay as workqueue base time=%d\n",atomic_read(&hwm_obj->delay));
  }

  //upadate all sensors delayCountSet
  for(idx = 0; idx < MAX_ANDROID_SENSOR_NUM; idx++)
  {
		cxt = obj->dc->cxt[idx];
		if((cxt == NULL) || (cxt->obj.sensor_operate == NULL)
			|| !(obj->active_data_sensor&(0x01<<idx)))
		{
			continue;
		}
		
		if(0 == atomic_read(&cxt->delay))
		{
		   cxt->delayCount = cxt->delayCountSet = 0;
		   HWM_LOG("%s,set delayCountSet=0 delay =%d handle=%d\r\n",__func__, atomic_read(&cxt->delay), idx);
		}
		if(atomic_read(&cxt->delay) <= atomic_read(&hwm_obj->delay))
		{
		   cxt->delayCount = cxt->delayCountSet = 0;
		   HWM_LOG("%s,set delayCountSet=0 delay =%d handle=%d\r\n",__func__, atomic_read(&cxt->delay), idx);
		}
		else
		{
		   i= atomic_read(&cxt->delay)/atomic_read(&hwm_obj->delay);
		   cxt->delayCount = cxt->delayCountSet = i;
		   HWM_LOG("%s:set delayCountSet=%d delay =%d handle=%d\r\n",__func__, i, atomic_read(&cxt->delay), idx);
		#if 0
		   switch(i)
		   {
		     case 3:
			 	// 200/60 ;60/20
			 	cxt->delayCount = cxt->delayCountSet = 3;
				HWM_LOG("%s:set delayCountSet=3 delay =%d handle=%d\r\n",__func__,atomic_read(&cxt->delay), idx);
			 	break;
			 case 10:
			 	// 200/20
			 	cxt->delayCount = cxt->delayCountSet = 10;
				HWM_LOG("%s:set delayCountSet=10 delay =%d handle=%d\r\n",__func__, atomic_read(&cxt->delay), idx);
			 	break;
		   }
		#endif
		}
   }
}

//static int hwmsen_ioctl(struct inode *node, struct file *fp, 
//                        unsigned int cmd, unsigned long arg)
static long hwmsen_unlocked_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
	//HWM_LOG("IO parament %d!\r\n", cmd);	
	void __user *argp = (void __user*)arg;
	uint32_t flag;
	struct sensor_delay delayPara;
	hwm_trans_data hwm_sensors_data;	
	int i = 0;
	atomic_t delaytemp;
	atomic_set(&delaytemp, 200);//used to finding fastest sensor polling rate
	//int delaytemp=200;//used to finding fastest sensor polling rate

	if(!hwm_obj)
	{
		HWM_ERR("null pointer!!\n");
		return -EINVAL;
	}

	switch(cmd)
	{
		case HWM_IO_SET_DELAY:
			 // android2.3 sensor system has 4 sample delay 0ms 20ms 60ms 200ms
			if(copy_from_user(&delayPara, argp, sizeof(delayPara)))
			{
				HWM_ERR("copy_from_user fail!!\n");
				return -EFAULT;
			}
			HWM_LOG("ioctl delay handle=%d,delay =%d\n",delayPara.handle,delayPara.delay);
			hwmsen_set_delay(delayPara.delay,delayPara.handle);//modified for android2.3
			update_workqueue_polling_rate(delayPara.delay);
        	
        	break;

		case HWM_IO_SET_WAKE:
			hwmsen_wakeup(hwm_obj);
			break;

		case HWM_IO_ENABLE_SENSOR:
			if(copy_from_user(&flag, argp, sizeof(flag)))
			{
				HWM_ERR("copy_from_user fail!!\n");
				return -EFAULT;
			}
			hwmsen_enable(hwm_obj, flag, 1);
			break;

		case HWM_IO_DISABLE_SENSOR:
			if(copy_from_user(&flag, argp, sizeof(flag)))
			{
				HWM_ERR("copy_from_user fail!!\n");
				return -EFAULT;
			}
			hwmsen_enable(hwm_obj, flag, 0);
			break;

		case HWM_IO_GET_SENSORS_DATA:			
			if(copy_from_user(&hwm_sensors_data, argp, sizeof(hwm_sensors_data)))
			{
				HWM_ERR("copy_from_user fail!!\n");
				return -EFAULT;
			}
			mutex_lock(&obj_data.lock);			
			memcpy(hwm_sensors_data.data, &(obj_data.sensors_data),sizeof(hwm_sensor_data) * MAX_ANDROID_SENSOR_NUM);
			for(i = 0; i < MAX_ANDROID_SENSOR_NUM; i++)
			{
				if(hwm_sensors_data.date_type & 1<<i)
				{					
					hwm_sensors_data.data[i].update = 1;
				}
				else
				{
					hwm_sensors_data.data[i].update = 0;
				}
			}
			mutex_unlock(&obj_data.lock);
			if(copy_to_user(argp, &hwm_sensors_data, sizeof(hwm_sensors_data)))
			{
				HWM_ERR("copy_to_user fail!!\n");
				return -EFAULT;
			}
			break;
			
		case HWM_IO_ENABLE_SENSOR_NODATA:
			if(copy_from_user(&flag, argp, sizeof(flag)))
			{
				HWM_ERR("copy_from_user fail!!\n");
				return -EFAULT;
			}
			hwmsen_enable_nodata(hwm_obj, flag, 1);
			break;

		case HWM_IO_DISABLE_SENSOR_NODATA:
			if(copy_from_user(&flag, argp, sizeof(flag)))
			{
				HWM_ERR("copy_from_user fail!!\n");
				return -EFAULT;
			}
			hwmsen_enable_nodata(hwm_obj, flag, 0);
			break;
			
		default:
			HWM_ERR("have no this paramenter %d!!\n", cmd);
			return -ENOIOCTLCMD;
	}

	return 0;        
}
/*----------------------------------------------------------------------------*/
static struct file_operations hwmsen_fops = {
//	.owner  = THIS_MODULE,
	.open   = hwmsen_open,
	.release = hwmsen_release,
//	.ioctl  = hwmsen_ioctl,
	.unlocked_ioctl = hwmsen_unlocked_ioctl,
};
/*----------------------------------------------------------------------------*/
static int hwmsen_probe(struct platform_device *pdev) 
{

	int err;
	//HWM_LOG("+++++++++++++++++hwmsen_probe!!\n");

	HWM_FUN(f);
	init_static_data();	
	hwm_obj = hwmsen_alloc_object();
	if (!hwm_obj)
	{
		err = -ENOMEM;
		HWM_ERR("unable to allocate devobj!\n");
		goto exit_alloc_data_failed;
	}

	hwm_obj->idev = input_allocate_device();
	if (!hwm_obj->idev)
	{
		err = -ENOMEM;
		HWM_ERR("unable to allocate input device!\n");
		goto exit_alloc_input_dev_failed;
	}
	
	set_bit(EV_REL, hwm_obj->idev->evbit);
	set_bit(EV_SYN, hwm_obj->idev->evbit);

	input_set_capability(hwm_obj->idev, EV_REL, EVENT_TYPE_SENSOR);
	hwm_obj->idev->name = HWM_INPUTDEV_NAME;
	if((err = input_register_device(hwm_obj->idev)))
	{
		HWM_ERR("unable to register input device!\n");
		goto exit_input_register_device_failed;
	}
	input_set_drvdata(hwm_obj->idev, hwm_obj);

	hwm_obj->mdev.minor = MISC_DYNAMIC_MINOR;
	hwm_obj->mdev.name  = HWM_SENSOR_DEV_NAME;
	hwm_obj->mdev.fops  = &hwmsen_fops;
	if((err = misc_register(&hwm_obj->mdev)))
	{
		HWM_ERR("unable to register sensor device!!\n");
		goto exit_misc_register_failed;
	}
	dev_set_drvdata(hwm_obj->mdev.this_device, hwm_obj);
	
	if(hwmsen_create_attr(hwm_obj->mdev.this_device) != 0)
	{
		HWM_ERR("unable to create attributes!!\n");
		goto exit_hwmsen_create_attr_failed;
	}
	// add for fix resume bug
    atomic_set(&(hwm_obj->early_suspend), 0);
	hwm_obj->early_drv.level    = EARLY_SUSPEND_LEVEL_STOP_DRAWING - 1,
	hwm_obj->early_drv.suspend  = hwmsen_early_suspend,
	hwm_obj->early_drv.resume   = hwmsen_late_resume,    
	register_early_suspend(&hwm_obj->early_drv);
	wake_lock_init(&(hwm_obj->read_data_wake_lock),WAKE_LOCK_SUSPEND,"read_data_wake_lock");
	// add for fix resume bug end
	return 0;

	exit_hwmsen_create_attr_failed:
	exit_misc_register_failed:    
//	exit_get_hwmsen_info_failed:
	exit_input_register_device_failed:
	input_free_device(hwm_obj->idev);
	
	exit_alloc_input_dev_failed:    
	kfree(hwm_obj);
	
	exit_alloc_data_failed:
	return err;
}
/*----------------------------------------------------------------------------*/
static int hwmsen_remove(struct platform_device *pdev)
{
	HWM_FUN(f);

	input_unregister_device(hwm_obj->idev);        
	hwmsen_delete_attr(hwm_obj->mdev.this_device);
	misc_deregister(&hwm_obj->mdev);
	kfree(hwm_obj);

	return 0;
}
static void hwmsen_early_suspend(struct early_suspend *h) 
{
   //HWM_FUN(f);
   atomic_set(&(hwm_obj->early_suspend), 1);
   HWM_LOG(" hwmsen_early_suspend ok------->hwm_obj->early_suspend=%d \n",atomic_read(&hwm_obj->early_suspend));
   return ;
}
/*----------------------------------------------------------------------------*/
static void hwmsen_late_resume(struct early_suspend *h)
{
   //HWM_FUN(f);
   atomic_set(&(hwm_obj->early_suspend), 0);
   HWM_LOG(" hwmsen_late_resume ok------->hwm_obj->early_suspend=%d \n",atomic_read(&hwm_obj->early_suspend));
   return ;
}
/*----------------------------------------------------------------------------*/
static int hwmsen_suspend(struct platform_device *dev, pm_message_t state) 
{
	//HWM_FUN(f);
	return 0;
}
/*----------------------------------------------------------------------------*/
static int hwmsen_resume(struct platform_device *dev)
{
	//HWM_FUN(f);
	return 0;
}
/*----------------------------------------------------------------------------*/
static struct platform_driver hwmsen_driver =
{
	.probe      = hwmsen_probe,
	.remove     = hwmsen_remove,    
	.suspend    = hwmsen_suspend,
	.resume     = hwmsen_resume,
	.driver     = 
	{
		.name = HWM_SENSOR_DEV_NAME,
//		.owner = THIS_MODULE,
	}
};

/*----------------------------------------------------------------------------*/

#if defined(MTK_AUTO_DETECT_MAGNETOMETER)

int hwmsen_msensor_remove(struct platform_device *pdev)
{
    int err =0;
	int i=0;
	for(i = 0; i < MAX_CHOOSE_G_NUM; i++)
	{
	   if(0 ==  strcmp(msensor_name,msensor_init_list[i]->name))
	   {
	      if(NULL == msensor_init_list[i]->uninit)
	      {
	        HWM_LOG(" hwmsen_msensor_remove null pointer \n");
	        return -1;
	      }
	      msensor_init_list[i]->uninit();
	   }
	}
    return 0;
}

static int msensor_probe(struct platform_device *pdev) 
{
    int i =0;
	int err=0;
	HWM_LOG(" msensor_probe +\n");
	for(i = 0; i < MAX_CHOOSE_G_NUM; i++)
	{
	  if(NULL != msensor_init_list[i])
	  {
	    err = msensor_init_list[i]->init();
		if(0 == err)
		{
		   strcpy(msensor_name,msensor_init_list[i]->name);
		   HWM_LOG(" msensor %s probe ok\n", msensor_name);
		   break;
		}
	  }
	}
	return 0;
}


static struct platform_driver msensor_driver = {
	.probe      = msensor_probe,
	.remove     = hwmsen_msensor_remove,    
	.driver     = 
	{
		.name  = "msensor",
//		.owner = THIS_MODULE,
	}
};

int hwmsen_msensor_add(struct sensor_init_info* obj) 
{
    int err=0;
	int i =0;
	
	HWM_FUN(f);

	for(i =0; i < MAX_CHOOSE_G_NUM; i++ )
	{
	    if(NULL == msensor_init_list[i])
	    {
	      msensor_init_list[i] = kzalloc(sizeof(struct sensor_init_info), GFP_KERNEL);
		  if(NULL == msensor_init_list[i])
		  {
		     HWM_ERR("kzalloc error");
		     return -1;
		  }
		  obj->platform_diver_addr = &msensor_driver;
	      msensor_init_list[i] = obj;
		  
		  break;
	    }
	}
		
	return err;
}
EXPORT_SYMBOL_GPL(hwmsen_msensor_add);

#endif



#if defined(MTK_AUTO_DETECT_ACCELEROMETER)//

int hwmsen_gsensor_remove(struct platform_device *pdev)
{
	int i=0;
	for(i = 0; i < MAX_CHOOSE_G_NUM; i++)
	{
	   if(0 ==  strcmp(gsensor_name,gsensor_init_list[i]->name))
	   {
	      if(NULL == gsensor_init_list[i]->uninit)
	      {
	        HWM_LOG(" hwmsen_gsensor_remove null pointer +\n");
	        return -1;
	      }
	      gsensor_init_list[i]->uninit();
	   }
	}
    return 0;
}

static int gsensor_probe(struct platform_device *pdev) 
{
    int i =0;
	int err=0;
	HWM_LOG(" gsensor_probe +\n");

	//
/*
     for(i = 0; i < MAX_CHOOSE_G_NUM; i++)
     {
       HWM_LOG(" gsensor_init_list[i]=%d\n",gsensor_init_list[i]);
     }
*/
	//
	for(i = 0; i < MAX_CHOOSE_G_NUM; i++)
	{
	  HWM_LOG(" i=%d\n",i);
	  if(0 != gsensor_init_list[i])
	  {
	    HWM_LOG(" !!!!!!!!\n");
	    err = gsensor_init_list[i]->init();
		if(0 == err)
		{
		   strcpy(gsensor_name,gsensor_init_list[i]->name);
		   HWM_LOG(" gsensor %s probe ok\n", gsensor_name);
		   break;
		}
	  }
	}

	if(i == MAX_CHOOSE_G_NUM)
	{
	   HWM_LOG(" gsensor probe fail\n");
	}
	return 0;
}


static struct platform_driver gsensor_driver = {
	.probe      = gsensor_probe,
	.remove     = hwmsen_gsensor_remove,    
	.driver     = 
	{
		.name  = "gsensor",
//		.owner = THIS_MODULE,
	}
};

 int hwmsen_gsensor_add(struct sensor_init_info* obj) 
{
    int err=0;
	int i =0;
	
	HWM_FUN(f);

	for(i =0; i < MAX_CHOOSE_G_NUM; i++ )
	{
	    if(NULL == gsensor_init_list[i])
	    {
	      gsensor_init_list[i] = kzalloc(sizeof(struct sensor_init_info), GFP_KERNEL);
		  if(NULL == gsensor_init_list[i])
		  {
		     HWM_ERR("kzalloc error");
		     return -1;
		  }
		  obj->platform_diver_addr = &gsensor_driver;
	      gsensor_init_list[i] = obj;
		  
		  break;
	    }
	}
		
	return err;
}
EXPORT_SYMBOL_GPL(hwmsen_gsensor_add);

#endif

#if defined(MTK_AUTO_DETECT_ALSPS)

int hwmsen_alsps_sensor_remove(struct platform_device *pdev)
{
    int err =0;
	int i=0;
	for(i = 0; i < MAX_CHOOSE_G_NUM; i++)
	{
	   if(0 ==  strcmp(alsps_name,alsps_init_list[i]->name))
	   {
	      if(NULL == alsps_init_list[i]->uninit)
	      {
	        HWM_LOG(" hwmsen_alsps_sensor_remove null pointer \n");
	        return -1;
	      }
	      alsps_init_list[i]->uninit();
	   }
	}
    return 0;
}

static int alsps_sensor_probe(struct platform_device *pdev) 
{
    int i =0;
	int err=0;
	HWM_LOG(" als_ps sensor_probe +\n");
	for(i = 0; i < MAX_CHOOSE_G_NUM; i++)
	{
	  if(NULL != alsps_init_list[i])
	  {
	    err = alsps_init_list[i]->init();
		if(0 == err)
		{
		   strcpy(alsps_name,alsps_init_list[i]->name);
		   HWM_LOG(" alsps sensor %s probe ok\n", alsps_name);
		   break;
		}
	  }
	}
	return 0;
}


static struct platform_driver alsps_sensor_driver = {
	.probe      = alsps_sensor_probe,
	.remove     = hwmsen_alsps_sensor_remove,    
	.driver     = 
	{
		.name  = "als_ps",
	}
};

int hwmsen_alsps_sensor_add(struct sensor_init_info* obj) 
{
    int err=0;
	int i =0;

	HWM_FUN(f);

	for(i =0; i < MAX_CHOOSE_G_NUM; i++ )
	{
	    if(NULL == alsps_init_list[i])
	    {
	      alsps_init_list[i] = kzalloc(sizeof(struct sensor_init_info), GFP_KERNEL);
		  if(NULL == alsps_init_list[i])
		  {
		     HWM_ERR("kzalloc error");
		     return -1;
		  }
		  obj->platform_diver_addr = &alsps_sensor_driver;
	      alsps_init_list[i] = obj;
		  
		  break;
	    }
	}
		
	return err;
}
EXPORT_SYMBOL_GPL(hwmsen_alsps_sensor_add);

#endif

/*----------------------------------------------------------------------------*/
static int __init hwmsen_init(void) 
{
	HWM_FUN(f);
	if(platform_driver_register(&hwmsen_driver))
	{
		HWM_ERR("failed to register sensor driver");
		return -ENODEV;
	}    

	
#if defined(MTK_AUTO_DETECT_ACCELEROMETER)
    if(platform_driver_register(&gsensor_driver))
	{
		HWM_ERR("failed to register gensor driver");
		return -ENODEV;
	}
#endif

#if defined(MTK_AUTO_DETECT_MAGNETOMETER)
		if(platform_driver_register(&msensor_driver))
		{
			HWM_ERR("failed to register mensor driver");
			return -ENODEV;
		}
#endif

#if defined(MTK_AUTO_DETECT_ALSPS)
			if(platform_driver_register(&alsps_sensor_driver))
			{
				HWM_ERR("failed to register alsps_sensor_driver driver");
				return -ENODEV;
			}
#endif


	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit hwmsen_exit(void)
{
	platform_driver_unregister(&hwmsen_driver);    
}
/*----------------------------------------------------------------------------*/
module_init(hwmsen_init);
module_exit(hwmsen_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sensor device driver");
MODULE_AUTHOR("Chunlei Wang<chunlei.wang@mediatek.com");
