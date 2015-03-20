#include "accel.h"

static struct acc_context *acc_context_obj = NULL;


static struct acc_init_info* gsensor_init_list[MAX_CHOOSE_G_NUM]= {0}; //modified
static void acc_early_suspend(struct early_suspend *h);
static void acc_late_resume(struct early_suspend *h);

static void acc_work_func(struct work_struct *work)
{

	struct acc_context *cxt = NULL;
	int out_size;
	//hwm_sensor_data sensor_data;
	int x,y,z,status,div;
	int64_t  nt;
	struct timespec time; 
	int err, idx;	

	cxt  = acc_context_obj;
	
	if(NULL == cxt->acc_data.get_data)
	{
		ACC_LOG("acc driver not register data path\n");
	}

	
	time.tv_sec = time.tv_nsec = 0;    
	time = get_monotonic_coarse(); 
	nt = time.tv_sec*1000000000LL+time.tv_nsec;
	
	cxt->acc_data.get_data(&x,&y,&z,&status);

	if(err)
	{
		ACC_ERR("get acc data fails!!\n" );
		goto acc_loop;
	}
	else
	{
		if((x != cxt->drv_data.acc_data.values[0]) 
					|| (y != cxt->drv_data.acc_data.values[1])
					|| (z != cxt->drv_data.acc_data.values[2]))
		{	
			if( 0 == x && 0==y 
						&& 0 == z)
			{
				    goto acc_loop;
			}

			cxt->drv_data.acc_data.values[0] = x;
			cxt->drv_data.acc_data.values[1] = y;
			cxt->drv_data.acc_data.values[2] = z;
			cxt->drv_data.acc_data.status = status;
			cxt->drv_data.acc_data.time = nt;
					
		}			
	 }
    
	if(true ==  cxt->is_first_data_after_enable)
	{
		cxt->is_first_data_after_enable = false;
		//filter -1 value
	    if(ACC_INVALID_VALUE == cxt->drv_data.acc_data.values[0] ||
		   	     ACC_INVALID_VALUE == cxt->drv_data.acc_data.values[1] ||
		   	     ACC_INVALID_VALUE == cxt->drv_data.acc_data.values[2])
	    {
	        ACC_LOG(" read invalid data \n");
	       	goto acc_loop;
			
	    }
	}
	//report data to input device
	//printk("new acc work run....\n");
	//ACC_LOG("acc data[%d,%d,%d]  \n" ,cxt->drv_data.acc_data.values[0],
	//cxt->drv_data.acc_data.values[1],cxt->drv_data.acc_data.values[2]);

	acc_data_report(cxt->drv_data.acc_data.values[0],
		cxt->drv_data.acc_data.values[1],cxt->drv_data.acc_data.values[2],
		cxt->drv_data.acc_data.status);

	acc_loop:
	if(true == cxt->is_polling_run)
	{
		  mod_timer(&cxt->timer, jiffies + atomic_read(&cxt->delay)/(1000/HZ)); 
	}
}

static void acc_poll(unsigned long data)
{
	struct acc_context *obj = (struct acc_context *)data;
	if(obj != NULL)
	{
		schedule_work(&obj->report);
	}
}

static struct acc_context *acc_context_alloc_object(void)
{
	
	struct acc_context *obj = kzalloc(sizeof(*obj), GFP_KERNEL); 
    ACC_LOG("acc_context_alloc_object++++\n");
	if(!obj)
	{
		ACC_ERR("Alloc accel object error!\n");
		return NULL;
	}	
	atomic_set(&obj->delay, 200); /*5Hz*/// set work queue delay time 200ms
	atomic_set(&obj->wake, 0);
	INIT_WORK(&obj->report, acc_work_func);
	init_timer(&obj->timer);
	obj->timer.expires	= jiffies + atomic_read(&obj->delay)/(1000/HZ);
	obj->timer.function	= acc_poll;
	obj->timer.data		= (unsigned long)obj;
	obj->is_first_data_after_enable = false;
	obj->is_polling_run = false;
	mutex_init(&obj->acc_op_mutex);
	ACC_LOG("acc_context_alloc_object----\n");
	return obj;
}

static int acc_real_enable(int enable)
{
  int err =0;
  struct acc_context *cxt = NULL;
  cxt = acc_context_obj;
  if(1==enable)
  {
     
     if(true==cxt->is_active_data || true ==cxt->is_active_nodata)
     {
        err = cxt->acc_ctl.enable_nodata(1);
        if(err)
        { 
           err = cxt->acc_ctl.enable_nodata(1);
		   if(err)
		   {
		   		err = cxt->acc_ctl.enable_nodata(1);
				if(err)
					ACC_ERR("acc enable(%d) err 3 timers = %d\n", enable, err);
		   }
        }
		ACC_LOG("acc real enable  \n" );
     }
	 
  }
  if(0==enable)
  {
     if(false==cxt->is_active_data && false ==cxt->is_active_nodata)
     {
        err = cxt->acc_ctl.enable_nodata(0);
        if(err)
        { 
          ACC_ERR("acc enable(%d) err = %d\n", enable, err);
        }
		ACC_LOG("acc real disable  \n" );
     }
	 
  }

  return err;
}
static int acc_enable_data(int enable)
{
    struct acc_context *cxt = NULL;
	int err =0;
	cxt = acc_context_obj;
	if(NULL  == cxt->acc_ctl.open_report_data)
	{
	  ACC_ERR("no acc control path\n");
	  return -1;
	}
	
    if(1 == enable)
    {
       ACC_LOG("ACC enable data\n");
	   cxt->is_active_data =true;
       cxt->is_first_data_after_enable = true;
	   cxt->acc_ctl.open_report_data(1);
	   if(false == cxt->is_polling_run)
	   {
	      if(false == cxt->acc_ctl.is_report_input_direct)
	      {
	      	mod_timer(&cxt->timer, jiffies + atomic_read(&cxt->delay)/(1000/HZ));
		  	cxt->is_polling_run = true;
	      }
	   }
    }
	if(0 == enable)
	{
	   ACC_LOG("ACC disable \n");
	   
	   cxt->is_active_data =false;
	   cxt->acc_ctl.open_report_data(0);
	   if(true == cxt->is_polling_run)
	   {
	      if(false == cxt->acc_ctl.is_report_input_direct)
	      {
	      	cxt->is_polling_run = false;
	      	del_timer_sync(&cxt->timer);
	      	cancel_work_sync(&cxt->report);
			cxt->drv_data.acc_data.values[0] = ACC_INVALID_VALUE;
	   		cxt->drv_data.acc_data.values[1] = ACC_INVALID_VALUE;
	   		cxt->drv_data.acc_data.values[2] = ACC_INVALID_VALUE;
	      }
	   }
	   
	}
	acc_real_enable(enable);
	return 0;
}



int acc_enable_nodata(int enable)
{
    struct acc_context *cxt = NULL;
	int err =0;
	cxt = acc_context_obj;
	if(NULL  == cxt->acc_ctl.enable_nodata)
	{
	  ACC_ERR("acc_enable_nodata:acc ctl path is NULL\n");
	  return -1;
	}
	
    if(1 == enable)
    {
       cxt->is_active_nodata = true;
    }
	
	if(0 == enable)
	{
	  cxt->is_active_nodata = false;
	}
	acc_real_enable(enable);
	return 0;
}


static ssize_t acc_show_enable_nodata(struct device* dev, 
                                 struct device_attribute *attr, char *buf) 
{
    int len = 0;
	ACC_LOG(" not support now\n");
	return len;
}

static ssize_t acc_store_enable_nodata(struct device* dev, struct device_attribute *attr,
                                  const char *buf, size_t count)
{
	ACC_LOG("acc_store_enable nodata buf=%s\n",buf);
	mutex_lock(&acc_context_obj->acc_op_mutex);
	struct acc_context *cxt = NULL;
	int err =0;
	cxt = acc_context_obj;
	if(NULL == cxt->acc_ctl.enable_nodata)
	{
		ACC_LOG("acc_ctl enable nodata NULL\n");
		mutex_unlock(&acc_context_obj->acc_op_mutex);
	 	return count;
	}
	if (!strncmp(buf, "1", 1))
	{
		//cxt->acc_ctl.enable_nodata(1);
		acc_enable_nodata(1);
	}
	else if (!strncmp(buf, "0", 1))
	{
       //cxt->acc_ctl.enable_nodata(0);
       acc_enable_nodata(0);
    }
	else
	{
	  ACC_ERR(" acc_store enable nodata cmd error !!\n");
	}
	mutex_unlock(&acc_context_obj->acc_op_mutex);
}

static ssize_t acc_store_active(struct device* dev, struct device_attribute *attr,
                                  const char *buf, size_t count)
{
    ACC_LOG("acc_store_active buf=%s\n",buf);
	mutex_lock(&acc_context_obj->acc_op_mutex);
	struct acc_context *cxt = NULL;
	int err =0;
	cxt = acc_context_obj;
	if(NULL == cxt->acc_ctl.open_report_data)
	{
		ACC_LOG("acc_ctl enable NULL\n");
		mutex_unlock(&acc_context_obj->acc_op_mutex);
	 	return count;
	}
    if (!strncmp(buf, "1", 1)) 
	{
      // cxt->acc_ctl.enable(1);
      acc_enable_data(1);
       
    } 
	else if (!strncmp(buf, "0", 1))
	{
        
       //cxt->acc_ctl.enable(0);
       acc_enable_data(0);
    }
	else
	{
	  ACC_ERR(" acc_store_active error !!\n");
	}
	mutex_unlock(&acc_context_obj->acc_op_mutex);
	ACC_LOG(" acc_store_active done\n");
    return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t acc_show_active(struct device* dev, 
                                 struct device_attribute *attr, char *buf) 
{
	struct acc_context *cxt = NULL;
	cxt = acc_context_obj;

    int len = 0;
	ACC_LOG("acc show active not support now\n");
	int div=cxt->acc_data.vender_div;
	ACC_LOG("acc vender_div value: %d\n", div);
	return snprintf(buf, PAGE_SIZE, "%d\n", div); 
	
	//return len;
}

static ssize_t acc_store_delay(struct device* dev, struct device_attribute *attr,
                                  const char *buf, size_t count)
{
	mutex_lock(&acc_context_obj->acc_op_mutex);
    struct acc_context *devobj = (struct acc_context*)dev_get_drvdata(dev);
    int delay;
	int mdelay=0;
	struct acc_context *cxt = NULL;
	int err =0;
	cxt = acc_context_obj;
	if(NULL == cxt->acc_ctl.set_delay)
	{
		ACC_LOG("acc_ctl set_delay NULL\n");
		mutex_unlock(&acc_context_obj->acc_op_mutex);
	 	return count;
	}

    if (1 != sscanf(buf, "%d", &delay)) {
        ACC_ERR("invalid format!!\n");
		mutex_unlock(&acc_context_obj->acc_op_mutex);
        return count;
    }

    if(false == cxt->acc_ctl.is_report_input_direct)
    {
    	mdelay = (int)delay/1000/1000;
    	atomic_set(&acc_context_obj->delay, mdelay);
    }
    cxt->acc_ctl.set_delay(delay);
	ACC_LOG(" acc_delay %d ns\n",delay);
	mutex_unlock(&acc_context_obj->acc_op_mutex);
    return count;
}

static ssize_t acc_show_delay(struct device* dev, 
                                 struct device_attribute *attr, char *buf) 
{
    int len = 0;
	ACC_LOG(" not support now\n");
	return len;
}


static int gsensor_remove(struct platform_device *pdev)
{
	ACC_LOG("gsensor_remove\n");
	return 0;
}

static int gsensor_probe(struct platform_device *pdev) 
{
	ACC_LOG("gsensor_probe\n");
    return 0;
}

static struct platform_driver gsensor_driver = {
	.probe      = gsensor_probe,
	.remove     = gsensor_remove,    
	.driver     = 
	{
		.name  = "gsensor",
	}
};

static int acc_real_driver_init(void) 
{
    int i =0;
	int err=0;
	ACC_LOG(" acc_real_driver_init +\n");
	for(i = 0; i < MAX_CHOOSE_G_NUM; i++)
	{
	  ACC_LOG(" i=%d\n",i);
	  if(0 != gsensor_init_list[i])
	  {
	    ACC_LOG(" acc try to init driver %s\n", gsensor_init_list[i]->name);
	    err = gsensor_init_list[i]->init();
		if(0 == err)
		{
		   ACC_LOG(" acc real driver %s probe ok\n", gsensor_init_list[i]->name);
		   break;
		}
	  }
	}

	if(i == MAX_CHOOSE_G_NUM)
	{
	   ACC_LOG(" acc_real_driver_init fail\n");
	   err=-1;
	}
	return err;
}

  int acc_driver_add(struct acc_init_info* obj) 
{
    int err=0;
	int i =0;
	
	ACC_FUN();

	
	for(i =0; i < MAX_CHOOSE_G_NUM; i++ )
	{
		if(i == 0){
			ACC_LOG("register gensor driver for the first time\n");
			if(platform_driver_register(&gsensor_driver))
			{
				ACC_ERR("failed to register gensor driver already exist\n");
			}
		}
		
	    if(NULL == gsensor_init_list[i])
	    {
	      obj->platform_diver_addr = &gsensor_driver;
	      gsensor_init_list[i] = obj;
		  break;
	    }
	}
	if(NULL==gsensor_init_list[i])
	{
	   ACC_ERR("ACC driver add err \n");
	   err=-1;
	}
	
	return err;
}
EXPORT_SYMBOL_GPL(acc_driver_add);

static int acc_misc_init(struct acc_context *cxt)
{

    int err=0;
    cxt->mdev.minor = MISC_DYNAMIC_MINOR;
	cxt->mdev.name  = ACC_MISC_DEV_NAME;
	if((err = misc_register(&cxt->mdev)))
	{
		ACC_ERR("unable to register acc misc device!!\n");
	}
	//dev_set_drvdata(cxt->mdev.this_device, cxt);
	return err;
}

static void acc_input_destroy(struct acc_context *cxt)
{
	struct input_dev *dev = cxt->idev;

	input_unregister_device(dev);
	input_free_device(dev);
}

static int acc_input_init(struct acc_context *cxt)
{
	struct input_dev *dev;
	int err = 0;

	dev = input_allocate_device();
	if (NULL == dev)
		return -ENOMEM;

	dev->name = ACC_INPUTDEV_NAME;

	input_set_capability(dev, EV_ABS, EVENT_TYPE_ACCEL_X);
	input_set_capability(dev, EV_ABS, EVENT_TYPE_ACCEL_Y);
	input_set_capability(dev, EV_ABS, EVENT_TYPE_ACCEL_Z);
	input_set_capability(dev, EV_ABS, EVENT_TYPE_ACCEL_STATUS);
	
	input_set_abs_params(dev, EVENT_TYPE_ACCEL_X, ACC_VALUE_MIN, ACC_VALUE_MAX, 0, 0);
	input_set_abs_params(dev, EVENT_TYPE_ACCEL_Y, ACC_VALUE_MIN, ACC_VALUE_MAX, 0, 0);
	input_set_abs_params(dev, EVENT_TYPE_ACCEL_Z, ACC_VALUE_MIN, ACC_VALUE_MAX, 0, 0);
	input_set_abs_params(dev, EVENT_TYPE_ACCEL_STATUS, ACC_STATUS_MIN, ACC_STATUS_MAX, 0, 0);
	input_set_drvdata(dev, cxt);

	err = input_register_device(dev);
	if (err < 0) {
		input_free_device(dev);
		return err;
	}
	cxt->idev= dev;

	return 0;
}

DEVICE_ATTR(accenablenodata,     S_IWUSR | S_IRUGO, acc_show_enable_nodata, acc_store_enable_nodata);
DEVICE_ATTR(accactive,     S_IWUSR | S_IRUGO, acc_show_active, acc_store_active);
DEVICE_ATTR(accdelay,      S_IWUSR | S_IRUGO, acc_show_delay,  acc_store_delay);

static struct attribute *acc_attributes[] = {
	&dev_attr_accenablenodata.attr,
	&dev_attr_accactive.attr,
	&dev_attr_accdelay.attr,
	NULL
};

static struct attribute_group acc_attribute_group = {
	.attrs = acc_attributes
};

int acc_register_data_path(struct acc_data_path *data)
{
	struct acc_context *cxt = NULL;
	int err =0;
	cxt = acc_context_obj;
	cxt->acc_data.get_data = data->get_data;
	cxt->acc_data.vender_div = data->vender_div;
	ACC_LOG("acc register data path vender_div: %d\n", cxt->acc_data.vender_div);
	if(NULL == cxt->acc_data.get_data)
	{
		ACC_LOG("acc register data path fail \n");
	 	return -1;
	}
	return 0;
}

int acc_register_control_path(struct acc_control_path *ctl)
{
	struct acc_context *cxt = NULL;
	int err =0;
	cxt = acc_context_obj;
	cxt->acc_ctl.set_delay = ctl->set_delay;
	cxt->acc_ctl.open_report_data= ctl->open_report_data;
	cxt->acc_ctl.enable_nodata = ctl->enable_nodata;

	if(NULL==cxt->acc_ctl.set_delay || NULL==cxt->acc_ctl.open_report_data
		|| NULL==cxt->acc_ctl.enable_nodata)
	{
		ACC_LOG("acc register control path fail \n");
	 	return -1;
	}

	//add misc dev for sensor hal control cmd
	err = acc_misc_init(acc_context_obj);
	if(err)
	{
	   ACC_ERR("unable to register acc misc device!!\n");
	   return -2;
	}
	err = sysfs_create_group(&acc_context_obj->mdev.this_device->kobj,
			&acc_attribute_group);
	if (err < 0)
	{
	   ACC_ERR("unable to create acc attribute file\n");
	   return -3;
	}
	
	return 0;	
}

int acc_data_report(int x, int y, int z,int status)
{
	//ACC_LOG("+acc_data_report! %d, %d, %d, %d\n",x,y,z,status);
    struct acc_context *cxt = NULL;
	int err =0;
	cxt = acc_context_obj;
  	input_report_abs(cxt->idev, EVENT_TYPE_ACCEL_X, x);
	input_report_abs(cxt->idev, EVENT_TYPE_ACCEL_Y, y);
	input_report_abs(cxt->idev, EVENT_TYPE_ACCEL_Z, z);
	input_report_abs(cxt->idev, EVENT_TYPE_ACCEL_STATUS, status);
	input_sync(cxt->idev); 
}

static int acc_probe(struct platform_device *pdev) 
{

	int err;
	ACC_LOG("+++++++++++++accel_probe!!\n");

	acc_context_obj = acc_context_alloc_object();
	if (!acc_context_obj)
	{
		err = -ENOMEM;
		ACC_ERR("unable to allocate devobj!\n");
		goto exit_alloc_data_failed;
	}

	//init real acceleration driver
    err = acc_real_driver_init();
	if(err)
	{
		ACC_ERR("acc real driver init fail\n");
		goto real_driver_init_fail;
	}

	//init input dev
	err = acc_input_init(acc_context_obj);
	if(err)
	{
		ACC_ERR("unable to register acc input device!\n");
		goto exit_alloc_input_dev_failed;
	}

    atomic_set(&(acc_context_obj->early_suspend), 0);
	acc_context_obj->early_drv.level    = EARLY_SUSPEND_LEVEL_STOP_DRAWING - 1,
	acc_context_obj->early_drv.suspend  = acc_early_suspend,
	acc_context_obj->early_drv.resume   = acc_late_resume,    
	register_early_suspend(&acc_context_obj->early_drv);

	wake_lock_init(&(acc_context_obj->read_data_wake_lock),WAKE_LOCK_SUSPEND,"read_data_wake_lock");
   
	ACC_LOG("----accel_probe OK !!\n");
	return 0;

	exit_hwmsen_create_attr_failed:
	exit_misc_register_failed:    

	exit_err_sysfs:
	
	if (err)
	{
	   ACC_ERR("sysfs node creation error \n");
	   acc_input_destroy(acc_context_obj);
	}
	
	real_driver_init_fail:
	exit_alloc_input_dev_failed:    
	kfree(acc_context_obj);
	
	exit_alloc_data_failed:
	

	ACC_LOG("----accel_probe fail !!!\n");
	return err;
}



static int acc_remove(struct platform_device *pdev)
{
	ACC_FUN(f);
	int err=0;
	input_unregister_device(acc_context_obj->idev);        
	sysfs_remove_group(&acc_context_obj->idev->dev.kobj,
				&acc_attribute_group);
	
	if((err = misc_deregister(&acc_context_obj->mdev)))
	{
		ACC_ERR("misc_deregister fail: %d\n", err);
	}
	kfree(acc_context_obj);

	return 0;
}

static void acc_early_suspend(struct early_suspend *h) 
{
   atomic_set(&(acc_context_obj->early_suspend), 1);
   ACC_LOG(" acc_early_suspend ok------->hwm_obj->early_suspend=%d \n",atomic_read(&(acc_context_obj->early_suspend)));
   return ;
}
/*----------------------------------------------------------------------------*/
static void acc_late_resume(struct early_suspend *h)
{
   atomic_set(&(acc_context_obj->early_suspend), 0);
   ACC_LOG(" acc_late_resume ok------->hwm_obj->early_suspend=%d \n",atomic_read(&(acc_context_obj->early_suspend)));
   return ;
}

static int acc_suspend(struct platform_device *dev, pm_message_t state) 
{
	return 0;
}
/*----------------------------------------------------------------------------*/
static int acc_resume(struct platform_device *dev)
{
	return 0;
}

static struct platform_driver acc_driver =
{
	.probe      = acc_probe,
	.remove     = acc_remove,    
	.suspend    = acc_suspend,
	.resume     = acc_resume,
	.driver     = 
	{
		.name = ACC_PL_DEV_NAME,
	}
};

static int __init acc_init(void) 
{
	ACC_FUN();

	if(platform_driver_register(&acc_driver))
	{
		ACC_ERR("failed to register acc driver\n");
		return -ENODEV;
	}
	
	return 0;
}

static void __exit acc_exit(void)
{
	platform_driver_unregister(&acc_driver); 
	platform_driver_unregister(&gsensor_driver);      
}

module_init(acc_init);
module_exit(acc_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ACCELEROMETER device driver");
MODULE_AUTHOR("Mediatek");

