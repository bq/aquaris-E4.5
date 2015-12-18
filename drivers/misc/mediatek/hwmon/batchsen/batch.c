		
#include <linux/batch.h>

static DEFINE_MUTEX(batch_data_mutex);
static struct batch_context *batch_context_obj = NULL;

static struct batch_init_info* batch_init_list[MAX_CHOOSE_BATCH_NUM]= {0}; 

#if defined(CONFIG_HAS_EARLYSUSPEND)
static void batch_early_suspend(struct early_suspend *h);
static void batch_late_resume(struct early_suspend *h);
#endif //#if defined(CONFIG_HAS_EARLYSUSPEND)

static int IDToSensorType(int id)
    {
    int sensorType;
    switch(id){
        case ID_ACCELEROMETER:
            sensorType = SENSOR_TYPE_ACCELEROMETER;
            break;
        case ID_MAGNETIC:
            sensorType = SENSOR_TYPE_MAGNETIC_FIELD;
            break;
        case ID_ORIENTATION:
            sensorType = SENSOR_TYPE_ORIENTATION;
            break;
        case ID_GYROSCOPE:
            sensorType = SENSOR_TYPE_GYROSCOPE;
            break;
        case ID_LIGHT:
            sensorType = SENSOR_TYPE_LIGHT;
            break;
        case ID_PROXIMITY:
            sensorType = SENSOR_TYPE_PROXIMITY;
            break;
        case ID_PRESSURE:
            sensorType = SENSOR_TYPE_PRESSURE;
            break;
        case ID_TEMPRERATURE:
            sensorType = SENSOR_TYPE_TEMPERATURE;
            break;
        case ID_SIGNIFICANT_MOTION:
            sensorType = SENSOR_TYPE_SIGNIFICANT_MOTION;
            break;
        case ID_STEP_DETECTOR:
            sensorType = SENSOR_TYPE_STEP_DETECTOR;
            break;
        case ID_STEP_COUNTER:
            sensorType = SENSOR_TYPE_STEP_COUNTER;
            break;
        default:
            sensorType = -1;
    }

    return sensorType;
}

//static int register_eint_unmask(void)
//{
//	return 0;
//}
static int batch_update_polling_rate(void)
{
	struct batch_context *obj = batch_context_obj; 
	int idx=0;
	int mindelay =65535;
	
	//mindelay = obj->dev_list.data_dev[0].delay;
	for(idx = 0; idx < MAX_ANDROID_SENSOR_NUM; idx++)//choose first MAX_ANDROID_SENSOR_NUM sensors for different sensor type 
	{
		if((obj->active_sensor & (0x01<< idx)))
		{
			mindelay = (obj->dev_list.data_dev[idx].delay < mindelay) ? obj->dev_list.data_dev[idx].delay:mindelay;
		}
		
	}
	BATCH_LOG("get polling rate min value (%d) !\n", mindelay);
	return mindelay;
}

static int get_fifo_data(struct batch_context *obj)
{
	
	hwm_sensor_data sensor_data={0};
	int idx, err = 0;
	int fifo_len=-1;
	int fifo_status=-1;
	int i=0;
    int64_t  nt;
    struct timeval time;
    struct batch_timestamp_info *pt;

    time.tv_sec = 0;
    time.tv_usec = 0;
    do_gettimeofday(&time);
    nt = time.tv_sec*1000000000LL+time.tv_usec*1000;
    for(i=0;i<=MAX_ANDROID_SENSOR_NUM;i++)
    {
        obj->timestamp_info[i].num = 1;
        obj->timestamp_info[i].end_t = nt;
    }
    
	BATCH_LOG("fwq!! get_fifo_data +++++++++	!\n");
	if((obj->dev_list.ctl_dev[MAX_ANDROID_SENSOR_NUM].flush != NULL) 
		&& (obj->dev_list.data_dev[MAX_ANDROID_SENSOR_NUM].get_data != NULL)
		&& (obj->dev_list.data_dev[MAX_ANDROID_SENSOR_NUM].get_fifo_status)!=NULL)
	{
		obj->dev_list.data_dev[MAX_ANDROID_SENSOR_NUM].get_fifo_status(&fifo_len,&fifo_status,0, obj->timestamp_info);
		if(-1 == fifo_len)
		{
				//we use fifo_status
				if(1 == fifo_status)
				{
					err = obj->dev_list.data_dev[MAX_ANDROID_SENSOR_NUM].get_data(0, &sensor_data);
					if(err)
					{
						BATCH_LOG("batch get fifoA data error\n");
					}
				}
		}
		else if(fifo_len>=0)
		{
				for(i=0;i<fifo_len;i++)
				{
					err = obj->dev_list.data_dev[MAX_ANDROID_SENSOR_NUM].get_data(0, &sensor_data);
					if(err)
					{
						BATCH_LOG("batch get fifoB data error\n");
					}
					else
					{
                            if(obj->dev_list.data_dev[sensor_data.sensor].is_timestamp_supported == 0)
                            {
                                pt = &obj->timestamp_info[sensor_data.sensor];
                                if(pt->total_count == 0)
                                    BATCH_LOG("sensor %d total_count = 0\n", sensor_data.sensor);
                                else
                                {
                                    sensor_data.time = (pt->end_t - pt->start_t)*pt->num;
                                    do_div(sensor_data.time, pt->total_count);
                                    sensor_data.time += pt->start_t;
                                    BATCH_ERR("sensor_data.time = %lld, start_t = %lld, end_t = %lld, total_count = %d, num = %d\n", 
                                        sensor_data.time, pt->start_t, pt->end_t, pt->total_count, pt->num);
                                    pt->num++;
                                }
                            }
						report_batch_data(obj->idev, &sensor_data);
					}
				}
		}
		else
		{
				BATCH_LOG("can not handle this fifo\n");
		}
	}
		
	for(idx = 0; idx < MAX_ANDROID_SENSOR_NUM; idx++)
	{
			//BATCH_LOG("get data from sensor (%d) !\n", idx);
		if((obj->dev_list.ctl_dev[idx].flush == NULL) || (obj->dev_list.data_dev[idx].get_data == NULL))
		{
				continue;
		}
			
		if((obj->active_sensor & (0x01<< idx)))
		{
			do{
					err = obj->dev_list.data_dev[idx].get_data(idx, &sensor_data);
					//sensor_data.value_divide = obj->dev_list.data_dev[sensor_data.sensor-1].div;
					if(err == 0)
						report_batch_data(obj->idev, &sensor_data);
			  }
			  while(err == 0);
			
		}
	}

    for(i=0;i<=MAX_ANDROID_SENSOR_NUM;i++)
    {
        obj->timestamp_info[i].start_t = obj->timestamp_info[i].end_t;
    }

	return err;
}

int  batch_notify(BATCH_NOTIFY_TYPE type)
{
	int err=0;
	if(type == TYPE_BATCHFULL)
	{
		BATCH_LOG("fwq batch full notify\n");
		err = get_fifo_data(batch_context_obj);
		if(err)
		{
			BATCH_LOG("fwq!! get fifo data error !\n");
		}
	}
	if(type == TYPE_BATCHTIMEOUT)
	{
		BATCH_LOG("fwq batch timeout notify do nothing\n");
	}
	return err;
}

static void batch_work_func(struct work_struct *work)
{
	struct batch_context *obj = batch_context_obj;
	//hwm_sensor_data *sensor_data = NULL;	//
	int err;
	BATCH_LOG("fwq!! get data from sensor+++++++++  !\n");
	err = get_fifo_data(obj);
	if(err)
	{
		BATCH_LOG("fwq!! get fifo data error !\n");
	}
	
	if(obj->is_polling_run){
		mod_timer(&obj->timer, jiffies + atomic_read(&obj->delay)/(1000/HZ));
	}
	BATCH_LOG("fwq!! get data from sensor obj->delay=%d ---------  !\n",atomic_read(&obj->delay));
}

static void batch_poll(unsigned long data)
{
	struct batch_context *obj = (struct batch_context *)data;
	if(obj != NULL)
	{
		schedule_work(&obj->report);
	}
}

static void report_data_once(int handle)
{
	struct batch_context *obj = batch_context_obj;
	//hwm_sensor_data *sensor_data = NULL;	
	hwm_sensor_data sensor_data ={0};
	int err;
	//BATCH_LOG("fwq ++++++++++++++++++\n" );
	//BATCH_LOG("fwq batch mode  (%x,%x)) !!!!\n",obj->dev_list.ctl_dev[MAX_ANDROID_SENSOR_NUM].flush,
//		obj->dev_list.data_dev[MAX_ANDROID_SENSOR_NUM].get_data);
	if((obj->dev_list.ctl_dev[MAX_ANDROID_SENSOR_NUM].flush != NULL) && (obj->dev_list.data_dev[MAX_ANDROID_SENSOR_NUM].get_data != NULL))
	{
		if((obj->active_sensor & (0x01<< handle)))
		{
			//BATCH_LOG("fwq batch mode   enabled(%d) !!!!\n",obj->active_sensor);
			/*
			do{//need mutex against store active and report data once
				err = obj->dev_list.data_dev[MAX_ANDROID_SENSOR_NUM].get_data(handle, &sensor_data);
				//sensor_data.value_divide = obj->dev_list.data_dev[sensor_data.sensor-1].div;
				if(err == 0)
					report_batch_data(obj->idev, &sensor_data);
			}while(err == 0);
			*/
			get_fifo_data(obj);
			report_batch_finish(obj->idev, handle);
			obj->flush_result = obj->dev_list.ctl_dev[MAX_ANDROID_SENSOR_NUM].flush(handle);
			return;
		}else{
			BATCH_LOG("batch mode is not enabled for this sensor!\n");
			obj->flush_result= -1;
			return;
		}
	}
	
	if((obj->active_sensor & (0x01<< handle))){
		if((obj->dev_list.ctl_dev[handle].flush != NULL) && (obj->dev_list.data_dev[handle].get_data != NULL))
		{
			do{
				err = obj->dev_list.data_dev[handle].get_data(handle, &sensor_data);
				//sensor_data.value_divide = obj->dev_list.data_dev[sensor_data.sensor-1].div;
				if(err == 0)
					report_batch_data(obj->idev, &sensor_data);
			}while(err == 0);
			report_batch_finish(obj->idev, handle);
			obj->flush_result = obj->dev_list.ctl_dev[handle].flush(handle);
			return;
		}else{
			BATCH_LOG("batch mode is not enabled for this sensor!\n");
			obj->flush_result = -1;
			return;
		}
	}
	else{
			BATCH_LOG("batch mode is not enabled for this sensor!\n");
			obj->flush_result= -1;
			return;
		}
	obj->flush_result = 0;
	//BATCH_LOG("fwq --------------\n" );
}

static struct batch_context *batch_context_alloc_object(void)
{
	
	struct batch_context *obj = kzalloc(sizeof(*obj), GFP_KERNEL); 
    	BATCH_LOG("batch_context_alloc_object++++\n");
	if(!obj)
	{
		BATCH_ERR("Alloc batch object error!\n");
		return NULL;
	}	
	atomic_set(&obj->delay, 200); /*5Hz*/// set work queue delay time 200ms
	atomic_set(&obj->wake, 0);
	INIT_WORK(&obj->report, batch_work_func);
	init_timer(&obj->timer);
	obj->timer.expires	= jiffies + atomic_read(&obj->delay)/(1000/HZ);
	obj->timer.function	= batch_poll;
	obj->timer.data = (unsigned long)obj;
	obj->is_first_data_after_enable = false;
	obj->is_polling_run = false;
	obj->active_sensor = 0;
	obj->div_flag= 0;
	mutex_init(&obj->batch_op_mutex);
	BATCH_LOG("batch_context_alloc_object----\n");
	return obj;
}

static ssize_t batch_store_active(struct device* dev, struct device_attribute *attr,
                                  const char *buf, size_t count)
{
	struct batch_context *cxt = NULL;
	int res=0;
	int handle=0;
	int en=0;
	
    cxt = batch_context_obj;
	
	if((res = sscanf(buf, "%d,%d", &handle, &en))!=2)
	{
		BATCH_ERR(" batch_store_active param error: res = %d\n", res);
	}
	BATCH_LOG(" batch_store_active handle=%d ,en=%d\n",handle,en);

    if (handle < 0 || MAX_ANDROID_SENSOR_NUM < handle)
    {
        cxt->batch_result = -1;
        return count;
    }
    
	
	if(cxt->dev_list.data_dev[handle].is_batch_supported == 0)
	{
		cxt->batch_result = 0;
		return count;
	}
	
	if(0 == en)
	{
		cxt->active_sensor = cxt->active_sensor & (~(0x01 << handle));
		if(cxt->active_sensor == 0)
		{
			cxt->is_polling_run = false;
			del_timer_sync(&cxt->timer);
			cancel_work_sync(&cxt->report);
			if(cxt->dev_list.ctl_dev[handle].enable_hw_batch != NULL)
			{
				res = cxt->dev_list.ctl_dev[handle].enable_hw_batch(handle, 0,0,0);
				if(res < 0)
				{
					cxt->batch_result = -1;
					return count;
				}
			}else if(cxt->dev_list.ctl_dev[MAX_ANDROID_SENSOR_NUM].enable_hw_batch != NULL){
				res = cxt->dev_list.ctl_dev[MAX_ANDROID_SENSOR_NUM].enable_hw_batch(handle, 0,0,0);
				if(res < 0)
				{
					cxt->batch_result = -1;
					return count;
				}
			}
		}
	}

	if(2==en)
	{
		cxt->div_flag = handle;
		BATCH_LOG(" batch_hal want read %d div\n", handle);
	}
	
    return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t batch_show_active(struct device* dev, 
                                 struct device_attribute *attr, char *buf) 
{
	int div=0;
	int count =0;
	struct batch_context *cxt = NULL;
	cxt = batch_context_obj;
	//display now enabling sensors of batch mode
	div = cxt->dev_list.data_dev[cxt->div_flag].div;
	BATCH_LOG("batch %d_div value: %d\n",cxt->div_flag, div);
	count =  snprintf(buf, PAGE_SIZE, "%d\n", div); 
	BATCH_LOG("fwq count=%d,buf=%s \n",count,buf);
	return count; 
}

static ssize_t batch_store_delay(struct device* dev, struct device_attribute *attr,
                                  const char *buf, size_t count)
{
	BATCH_LOG(" batch_store_delay not support now\n");
    	return count;
}

static ssize_t batch_show_delay(struct device* dev, 
                                 struct device_attribute *attr, char *buf) 
{
    	int len = 0;
	BATCH_LOG(" batch_show_delay not support now\n");
	return len;
}

static ssize_t batch_store_batch(struct device* dev, struct device_attribute *attr,
                                  const char *buf, size_t count)
{
	int handle, flags=0;
	long long samplingPeriodNs, maxBatchReportLatencyNs=0;
	int res, delay=0;
	struct batch_context *cxt = NULL;
    int64_t  nt;
    struct timeval time;
    
    cxt = batch_context_obj;
    
	BATCH_LOG("write value: buf = %s\n", buf);
	if((res = sscanf(buf, "%d,%d,%lld,%lld", &handle, &flags, &samplingPeriodNs, &maxBatchReportLatencyNs))!=4)
	{
		BATCH_ERR(" batch_store_delay param error: res = %d\n", res);
	}
	BATCH_LOG(" batch_store_delay param: handle %d, flag:%d samplingPeriodNs:%lld, maxBatchReportLatencyNs: %lld\n",handle, flags, samplingPeriodNs, maxBatchReportLatencyNs);	

    if (handle < 0 || MAX_ANDROID_SENSOR_NUM < handle)
    {
        cxt->batch_result = -1;
        return count;
    }

	if(flags & SENSORS_BATCH_DRY_RUN )
	{
		if(maxBatchReportLatencyNs == 0)
		{
			cxt->batch_result = 0;
		}
		else
		{
			if(cxt->dev_list.data_dev[handle].is_batch_supported != 0)
			{
				cxt->batch_result = 0;
			}else{
				cxt->batch_result = -1;
			}
		}
		return count;
	}else if(flags & SENSORS_BATCH_WAKE_UPON_FIFO_FULL){
		//register_eint_unmask();
	}


	if((cxt->dev_list.ctl_dev[MAX_ANDROID_SENSOR_NUM].enable_hw_batch == NULL)&&(cxt->dev_list.ctl_dev[handle].enable_hw_batch == NULL)){
		cxt->batch_result = -1;
		return count;
	}
	
	do_div(maxBatchReportLatencyNs,1000000);
	do_div(samplingPeriodNs,1000000);
	
	if(maxBatchReportLatencyNs == 0){
		if(cxt->dev_list.data_dev[handle].is_batch_supported == 0)
		{
			cxt->batch_result = 0;
			return count;
		}
		if(cxt->dev_list.ctl_dev[handle].enable_hw_batch != NULL)
		{
			res = cxt->dev_list.ctl_dev[handle].enable_hw_batch(handle, 0,samplingPeriodNs,maxBatchReportLatencyNs);
			if(res < 0)
			{
				cxt->batch_result = -1;
				return count;
			}
		}else if(cxt->dev_list.ctl_dev[MAX_ANDROID_SENSOR_NUM].enable_hw_batch != NULL){
			res = cxt->dev_list.ctl_dev[MAX_ANDROID_SENSOR_NUM].enable_hw_batch(handle, 0,samplingPeriodNs,maxBatchReportLatencyNs);
			if(res < 0)
			{
				cxt->batch_result = -1;
				return count;
			}
		}

		if (cxt->active_sensor & (0x01 << handle)) //flush batch data when change from batch mode to normal mode.
		{
			BATCH_LOG("%d from batch to normal\n",handle);
			report_data_once(handle);
		}
		cxt->active_sensor = cxt->active_sensor & (~(0x01 << handle));//every active_sensor bit stands for a sensor type, bit = 0 stands for batch disabled
		if(cxt->active_sensor == 0){
			cxt->is_polling_run = false;
			del_timer_sync(&cxt->timer);
			cancel_work_sync(&cxt->report);
		}else{
			cxt->dev_list.data_dev[handle].delay = 0;
			delay = batch_update_polling_rate();
			atomic_set(&cxt->delay, delay);
			mod_timer(&cxt->timer, jiffies + atomic_read(&cxt->delay)/(1000/HZ));
			cxt->is_polling_run = true;
		}
	}else if(maxBatchReportLatencyNs != 0){
		if(cxt->dev_list.ctl_dev[handle].enable_hw_batch != NULL)
		{
			res = cxt->dev_list.ctl_dev[handle].enable_hw_batch(handle, 1,samplingPeriodNs,maxBatchReportLatencyNs);
			if(res < 0)
			{
				cxt->batch_result = -1;
				return count;
			}
		}else if(cxt->dev_list.ctl_dev[MAX_ANDROID_SENSOR_NUM].enable_hw_batch != NULL){
			res = cxt->dev_list.ctl_dev[MAX_ANDROID_SENSOR_NUM].enable_hw_batch(handle, 1,samplingPeriodNs,maxBatchReportLatencyNs);
			if(res < 0)
			{
				cxt->batch_result = -1;
				return count;
			}
		}
		cxt->active_sensor = cxt->active_sensor |(0x01 << handle);
		//do_div(maxBatchReportLatencyNs,1000000);
		cxt->dev_list.data_dev[handle].delay = maxBatchReportLatencyNs;//(int)maxBatchReportLatencyNs/1000/1000;
		BATCH_ERR(" fwq batch delay=%d \n", cxt->dev_list.data_dev[handle].delay);
		delay = batch_update_polling_rate();
		atomic_set(&cxt->delay, delay);
		mod_timer(&cxt->timer, jiffies + atomic_read(&cxt->delay)/(1000/HZ));
		cxt->is_polling_run = true;

        time.tv_sec = 0;
        time.tv_usec = 0;
        do_gettimeofday(&time);
        nt = time.tv_sec*1000000000LL+time.tv_usec*1000;
        cxt->timestamp_info[handle].start_t = nt;
	}
	
	cxt->batch_result = 0;
	
	return count;
}

static ssize_t batch_show_batch(struct device* dev, 
                                 struct device_attribute *attr, char *buf) 
{
	struct batch_context *cxt = NULL;
	int res = 0;
	cxt = batch_context_obj;
	res = cxt->batch_result;
	BATCH_LOG(" batch_show_delay batch result: %d\n", res);
	return snprintf(buf, PAGE_SIZE, "%d\n", res);
}

static ssize_t batch_store_flush(struct device* dev, struct device_attribute *attr,
                                  const char *buf, size_t count)
{
	struct batch_context *cxt = NULL;
	int handle;
	BATCH_LOG("fwq  flush_store_delay +++\n");
	cxt = batch_context_obj;
	
    	if (1 != sscanf(buf, "%d", &handle)) 
		{
			BATCH_LOG("fwq flush_ err......\n");
        	BATCH_ERR("invalid format!!\n");
        	return count;
    	}

	report_data_once(handle);//handle need to use of this function 
	
	BATCH_LOG(" flush_store_delay sucess------\n");
    	return count;
}

static ssize_t batch_show_flush(struct device* dev, 
                                 struct device_attribute *attr, char *buf) 
{
	struct batch_context *cxt = NULL;
	int res = 0;
	cxt = batch_context_obj;
	res = cxt->flush_result;
	BATCH_LOG(" batch_show_flush flush result: %d\n", res);
	return snprintf(buf, PAGE_SIZE, "%d\n", res);

}

static ssize_t batch_show_devnum(struct device* dev, 
                                 struct device_attribute *attr, char *buf) 
{
	struct batch_context *cxt = NULL;
	const char *devname = NULL;
	cxt = batch_context_obj;
	devname = dev_name(&cxt->idev->dev);
	return snprintf(buf, PAGE_SIZE, "%s\n", devname+5);
}

static int batch_misc_init(struct batch_context *cxt)
{

    int err=0;
    cxt->mdev.minor = MISC_DYNAMIC_MINOR;
	cxt->mdev.name  = BATCH_MISC_DEV_NAME;
	if((err = misc_register(&cxt->mdev)))
	{
		BATCH_ERR("unable to register batch misc device!!\n");
	}
	return err;
}

static void batch_input_destroy(struct batch_context *cxt)
{
	struct input_dev *dev = cxt->idev;

	input_unregister_device(dev);
	input_free_device(dev);
}

static int batch_input_init(struct batch_context *cxt)
{
	struct input_dev *dev;
	int err = 0;

	dev = input_allocate_device();
	if (NULL == dev)
		return -ENOMEM;

	dev->name = BATCH_INPUTDEV_NAME;

	input_set_capability(dev, EV_ABS, EVENT_TYPE_BATCH_X);
	input_set_capability(dev, EV_ABS, EVENT_TYPE_BATCH_Y);
	input_set_capability(dev, EV_ABS, EVENT_TYPE_BATCH_Z);
	//input_set_capability(dev, EV_ABS, EVENT_TYPE_SENSORTYPE);
	input_set_capability(dev, EV_REL, EVENT_TYPE_SENSORTYPE);
	input_set_capability(dev, EV_ABS, EVENT_TYPE_BATCH_VALUE);
	input_set_capability(dev, EV_ABS, EVENT_TYPE_BATCH_STATUS);
	//input_set_capability(dev, EV_ABS, EVENT_TYPE_END_FLAG);
	input_set_capability(dev, EV_REL, EVENT_TYPE_END_FLAG);
	input_set_capability(dev, EV_REL, EVENT_TYPE_TIMESTAMP_HI);
	input_set_capability(dev, EV_REL, EVENT_TYPE_TIMESTAMP_LO);
	
	input_set_abs_params(dev, EVENT_TYPE_BATCH_X, BATCH_VALUE_MIN, BATCH_VALUE_MAX, 0, 0);
	input_set_abs_params(dev, EVENT_TYPE_BATCH_Y, BATCH_VALUE_MIN, BATCH_VALUE_MAX, 0, 0);
	input_set_abs_params(dev, EVENT_TYPE_BATCH_Z, BATCH_VALUE_MIN, BATCH_VALUE_MAX, 0, 0);
	input_set_abs_params(dev, EVENT_TYPE_BATCH_STATUS, BATCH_STATUS_MIN, BATCH_STATUS_MAX, 0, 0);
	input_set_abs_params(dev, EVENT_TYPE_BATCH_VALUE, BATCH_VALUE_MIN, BATCH_VALUE_MAX, 0, 0);
	//input_set_abs_params(dev, EVENT_TYPE_SENSORTYPE, BATCH_TYPE_MIN, BATCH_TYPE_MAX, 0, 0);
	//input_set_abs_params(dev, EVENT_TYPE_END_FLAG, BATCH_STATUS_MIN, BATCH_STATUS_MAX, 0, 0);
	set_bit(EV_REL, dev->evbit);
	input_set_drvdata(dev, cxt);

	err = input_register_device(dev);
	if (err < 0) {
		input_free_device(dev);
		return err;
	}
	cxt->idev= dev;

	return 0;
}


DEVICE_ATTR(batchactive,     S_IWUSR | S_IRUGO, batch_show_active, batch_store_active);
DEVICE_ATTR(batchdelay,      S_IWUSR | S_IRUGO, batch_show_delay,  batch_store_delay);
DEVICE_ATTR(batchbatch,      S_IWUSR | S_IRUGO, batch_show_batch,  batch_store_batch);
DEVICE_ATTR(batchflush,      S_IWUSR | S_IRUGO, batch_show_flush,  batch_store_flush);
DEVICE_ATTR(batchdevnum,      S_IWUSR | S_IRUGO, batch_show_devnum,  NULL);


static struct attribute *batch_attributes[] = {
	&dev_attr_batchactive.attr,
	&dev_attr_batchdelay.attr,
	&dev_attr_batchbatch.attr,
	&dev_attr_batchflush.attr,
	&dev_attr_batchdevnum.attr,
	NULL
};

static struct attribute_group batch_attribute_group = {
	.attrs = batch_attributes
};

int batch_register_data_path(int handle, struct batch_data_path *data)
{
	struct batch_context *cxt = NULL;
	//int err =0;
	cxt = batch_context_obj;
	if(data == NULL){
		BATCH_ERR("data pointer is null!\n");
		return -1;
		}
	if(handle >= 0 && handle <=(MAX_ANDROID_SENSOR_NUM)){
		cxt ->dev_list.data_dev[handle].get_data = data->get_data;
		cxt ->dev_list.data_dev[handle].flags = data->flags;
		cxt ->dev_list.data_dev[handle].get_fifo_status= data->get_fifo_status;
		//cxt ->dev_list.data_dev[handle].is_batch_supported = data->is_batch_supported;
		return 0;
	}
	return -1;
}

int batch_register_control_path(int handle, struct batch_control_path *ctl)
{
	struct batch_context *cxt = NULL;
	cxt = batch_context_obj;
	if(ctl == NULL){
		BATCH_ERR("ctl pointer is null!\n");
		return -1;
		}
	if(handle >= 0 && handle <=(MAX_ANDROID_SENSOR_NUM)){
		cxt ->dev_list.ctl_dev[handle].enable_hw_batch = ctl->enable_hw_batch;
		cxt ->dev_list.ctl_dev[handle].flush= ctl->flush;
		return 0;	
		}
	return -1;
}

int batch_register_support_info(int handle, int support,int div, int timestamp_supported)
{
	struct batch_context *cxt = NULL;
	//int err =0;
	cxt = batch_context_obj;
	if(cxt == NULL){
		if (0 == support)
			return 0;
		else
		{
			BATCH_ERR("yucong cxt pointer is null!\n");
			return -1;
		}
	}
	if(handle >= 0 && handle <=(MAX_ANDROID_SENSOR_NUM)){
		cxt ->dev_list.data_dev[handle].is_batch_supported = support;
		cxt ->dev_list.data_dev[handle].div = div;
		cxt ->dev_list.data_dev[handle].is_timestamp_supported = timestamp_supported;
		return 0;
	}
	return -1;
}

void report_batch_data(struct input_dev *dev, hwm_sensor_data *data)
{	
	hwm_sensor_data report_data;

	memcpy(&report_data, data, sizeof(hwm_sensor_data)); 

	if(report_data.sensor == ID_ACCELEROMETER
	||report_data.sensor == ID_MAGNETIC
	||report_data.sensor == ID_ORIENTATION
	||report_data.sensor == ID_GYROSCOPE){
		input_report_rel(dev, EVENT_TYPE_SENSORTYPE, IDToSensorType(report_data.sensor));
		input_report_abs(dev, EVENT_TYPE_BATCH_X, report_data.values[0]);
		input_report_abs(dev, EVENT_TYPE_BATCH_Y, report_data.values[1]);
		input_report_abs(dev, EVENT_TYPE_BATCH_Z, report_data.values[2]);
		input_report_abs(dev, EVENT_TYPE_BATCH_STATUS, report_data.status);
 		input_report_rel(dev, EVENT_TYPE_TIMESTAMP_HI, (uint32_t) (report_data.time>>32)&0xFFFFFFFF);
 		input_report_rel(dev, EVENT_TYPE_TIMESTAMP_LO, (uint32_t)(report_data.time&0xFFFFFFFF));
		input_sync(dev); 
	}else{
		input_report_rel(dev, EVENT_TYPE_SENSORTYPE, IDToSensorType(report_data.sensor));
		input_report_abs(dev, EVENT_TYPE_BATCH_VALUE, report_data.values[0]);
		input_report_abs(dev, EVENT_TYPE_BATCH_STATUS, report_data.status);
 		input_report_rel(dev, EVENT_TYPE_TIMESTAMP_HI, (uint32_t) (report_data.time>>32)&0xFFFFFFFF);
 		input_report_rel(dev, EVENT_TYPE_TIMESTAMP_LO, (uint32_t)(report_data.time&0xFFFFFFFF));
		input_sync(dev); 
	}
}

void report_batch_finish(struct input_dev *dev, int handle)
{
	BATCH_LOG("fwq report_batch_finish rel+++++\n");
	input_report_rel(dev, EVENT_TYPE_END_FLAG, 1<<16|handle);
	input_sync(dev); 
	BATCH_LOG("fwq report_batch_finish----\n");
}

static int sensorHub_remove(struct platform_device *pdev)
{
	BATCH_LOG("sensorHub_remove\n");
	return 0;
}

static int sensorHub_probe(struct platform_device *pdev) 
{
	BATCH_LOG("sensorHub_probe\n");
    return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id sensorHub_of_match[] = {
	{ .compatible = "mediatek,sensorHub", },
	{},
};
#endif

static struct platform_driver sensorHub_driver = {
	.probe      = sensorHub_probe,
	.remove     = sensorHub_remove,    
	.driver     = 
	{
		.name  = "sensorHub",
        #ifdef CONFIG_OF
		.of_match_table = sensorHub_of_match,
		#endif
	}
};

static int batch_real_driver_init(void) 
{
    int i =0;
	int err=0;
	BATCH_LOG(" batch_real_driver_init +\n");
	for(i = 0; i < MAX_CHOOSE_BATCH_NUM; i++)
	{
	  BATCH_LOG(" i=%d\n",i);
	  if(0 != batch_init_list[i])
	  {
	    BATCH_LOG(" batch try to init driver %s\n", batch_init_list[i]->name);
	    err = batch_init_list[i]->init();
		if(0 == err)
		{
		   BATCH_LOG(" batch real driver %s probe ok\n", batch_init_list[i]->name);
		   break;
		}
	  }
	}

	if(i == MAX_CHOOSE_BATCH_NUM)
	{
	   BATCH_LOG(" batch_real_driver_init fail\n");
	   err = -1;
	}
	return err;
}

int  batch_driver_add(struct batch_init_info* obj) 
{
    int err=0;
	int i =0;
	
	BATCH_FUN();
	if (!obj) {
		BATCH_ERR("batch driver add fail, batch_init_info is NULL \n");
		return -1;
	}

	for(i =0; i < MAX_CHOOSE_BATCH_NUM; i++ )
	{
        if(i == 0){
			BATCH_LOG("register sensorHub driver for the first time\n");
			if(platform_driver_register(&sensorHub_driver))
			{
				BATCH_ERR("failed to register sensorHub driver already exist\n");
			}
		}
        
	    if(NULL == batch_init_list[i])
	    {
	      obj->platform_diver_addr = &sensorHub_driver;
	      batch_init_list[i] = obj;
		  break;
	    }
	}
	if(i >= MAX_CHOOSE_BATCH_NUM)
	{
	   BATCH_LOG("batch driver add err \n");
	   err=-1;
	}
	
	return err;
}
EXPORT_SYMBOL_GPL(batch_driver_add);

static int batch_probe(struct platform_device *pdev) 
{

	int err;
	BATCH_LOG("+++++++++++++batch_probe!!\n");

	batch_context_obj = batch_context_alloc_object();
	if (!batch_context_obj)
	{
		err = -ENOMEM;
		BATCH_ERR("unable to allocate devobj!\n");
		goto exit_alloc_data_failed;
	}

	err = batch_real_driver_init();
	if(err)
	{
		BATCH_ERR("batch real driver init fail\n");
		goto real_driver_init_fail;
	}
	
	//init input dev
	err = batch_input_init(batch_context_obj);
	if(err)
	{
		BATCH_ERR("unable to register batch input device!\n");
		goto exit_alloc_input_dev_failed;
	}

#if defined(CONFIG_HAS_EARLYSUSPEND)
    	atomic_set(&(batch_context_obj->early_suspend), 0);
	batch_context_obj->early_drv.level    = EARLY_SUSPEND_LEVEL_STOP_DRAWING - 1,
	batch_context_obj->early_drv.suspend  = batch_early_suspend,
	batch_context_obj->early_drv.resume   = batch_late_resume,    
	register_early_suspend(&batch_context_obj->early_drv);

	wake_lock_init(&(batch_context_obj->read_data_wake_lock),WAKE_LOCK_SUSPEND,"read_data_wake_lock");
#endif //#if defined(CONFIG_HAS_EARLYSUSPEND)

	//add misc dev for sensor hal control cmd
	err = batch_misc_init(batch_context_obj);
	if(err)
	{
	   BATCH_ERR("unable to register batch misc device!!\n");
	   goto exit_err_sysfs;
	}
	err = sysfs_create_group(&batch_context_obj->mdev.this_device->kobj,
			&batch_attribute_group);
	if (err < 0)
	{
	   BATCH_ERR("unable to create batch attribute file\n");
	   goto exit_misc_register_failed;
	}
	
	kobject_uevent(&batch_context_obj->mdev.this_device->kobj, KOBJ_ADD);

	BATCH_LOG("----batch_probe OK !!\n");
	return 0;

	//exit_hwmsen_create_attr_failed:
	exit_misc_register_failed:    
	if((err = misc_deregister(&batch_context_obj->mdev)))
	{
		BATCH_ERR("misc_deregister fail: %d\n", err);
	}
	
	exit_err_sysfs:
	
	if (err)
	{
	   BATCH_ERR("sysfs node creation error \n");
	   batch_input_destroy(batch_context_obj);
	}
	
	real_driver_init_fail:
	exit_alloc_input_dev_failed:    
	kfree(batch_context_obj);
	batch_context_obj = NULL;
	
	exit_alloc_data_failed:
	

	BATCH_LOG("----batch_probe fail !!!\n");
	return err;
}



static int batch_remove(struct platform_device *pdev)
{
	int err=0;
	BATCH_FUN(f);
	input_unregister_device(batch_context_obj->idev);        
	sysfs_remove_group(&batch_context_obj->idev->dev.kobj,
				&batch_attribute_group);
	
	if((err = misc_deregister(&batch_context_obj->mdev)))
	{
		BATCH_ERR("misc_deregister fail: %d\n", err);
	}
	kfree(batch_context_obj);

	return 0;
}

#if defined(CONFIG_HAS_EARLYSUSPEND)
static void batch_early_suspend(struct early_suspend *h) 
{
   atomic_set(&(batch_context_obj->early_suspend), 1);
   BATCH_LOG(" batch_early_suspend ok------->hwm_obj->early_suspend=%d \n",atomic_read(&(batch_context_obj->early_suspend)));
   return ;
}
/*----------------------------------------------------------------------------*/
static void batch_late_resume(struct early_suspend *h)
{
   atomic_set(&(batch_context_obj->early_suspend), 0);
   BATCH_LOG(" batch_late_resume ok------->hwm_obj->early_suspend=%d \n",atomic_read(&(batch_context_obj->early_suspend)));
   return ;
}
#endif //#if defined(CONFIG_HAS_EARLYSUSPEND)

static int batch_suspend(struct platform_device *dev, pm_message_t state) 
{
	return 0;
}
/*----------------------------------------------------------------------------*/
static int batch_resume(struct platform_device *dev)
{
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id m_batch_pl_of_match[] = {
	{ .compatible = "mediatek,m_batch_pl", },
	{},
};
#endif

static struct platform_driver batch_driver =
{
	.probe      = batch_probe,
	.remove     = batch_remove,    
	.suspend    = batch_suspend,
	.resume     = batch_resume,
	.driver     = 
	{
		.name = BATCH_PL_DEV_NAME,
        #ifdef CONFIG_OF
		.of_match_table = m_batch_pl_of_match,
		#endif
	}
};

static int __init batch_init(void) 
{
	BATCH_FUN();

	if(platform_driver_register(&batch_driver))
	{
		BATCH_ERR("failed to register batch driver\n");
		return -ENODEV;
	}
	
	return 0;
}

static void __exit batch_exit(void)
{
	platform_driver_unregister(&batch_driver);      
    platform_driver_unregister(&sensorHub_driver);
}

late_initcall(batch_init);
//module_init(batch_init);
//module_exit(batch_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("batch device driver");
MODULE_AUTHOR("Mediatek");

