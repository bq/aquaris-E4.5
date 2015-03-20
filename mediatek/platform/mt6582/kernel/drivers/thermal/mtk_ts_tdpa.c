#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/dmi.h>
#include <linux/acpi.h>
#include <linux/thermal.h>
#include <linux/platform_device.h>
#include <linux/aee.h>
#include <linux/xlog.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/writeback.h>
#include <asm/uaccess.h>
#include <asm/string.h>
#include <linux/spinlock.h>

#include <mach/system.h>

#include "mach/mtk_thermal_monitor.h"
#include "mach/mt_typedefs.h"
#include "mach/mt_thermal.h"
#include "mach/mtk_mdm_monitor.h"

static unsigned int interval = 0; /* seconds, 0 : no auto polling */
static unsigned int trip_temp[10] = {85000,80000,70000,60000,50000,40000,30000,20000,10000,5000};
static int g_THERMAL_TRIP[10] = {0,0,0,0,0,0,0,0,0,0};
static unsigned int cl_dev_sysrst_state = 0;
static struct thermal_zone_device *thz_dev;
static struct thermal_cooling_device *cl_dev_sysrst;
static int mtktstdpa_debug_log = 0;
static int kernelmode = 0;

static int num_trip=0;
static char g_bind0[20]="mtktstdpa-sysrst";
static char g_bind1[20]={0};
static char g_bind2[20]={0};
static char g_bind3[20]={0};
static char g_bind4[20]={0};
static char g_bind5[20]={0};
static char g_bind6[20]={0};
static char g_bind7[20]={0};
static char g_bind8[20]={0};
static char g_bind9[20]={0};

#define mtktstdpa_TEMP_CRIT 85000 /* 85.000 degree Celsius */

#define mtktstdpa_dprintk(fmt, args...)   \
do {                                    \
	if (mtktstdpa_debug_log) {                \
		xlog_printk(ANDROID_LOG_INFO, "Power/TDPA_Thermal", fmt, ##args); \
	}                                   \
} while(0)


/*
struct md_info{
		char *attribute;
		int value;
		char *unit;
		int invalid_value;
		int index;
};
struct md_info g_pinfo_list[] = 
{{"TXPWR_MD1", -127, "db", -127, 0}, 
 {"TXPWR_MD2", -127, "db", -127, 1},
 {"RFTEMP_2G_MD1", -32767, "�XC", -32767, 2},
 {"RFTEMP_2G_MD2", -32767, "�XC", -32767, 3},
 {"RFTEMP_3G_MD1", -32767, "�XC", -32767, 4},
 {"RFTEMP_3G_MD2", -32767, "�XC", -32767, 5}};
*/
static DEFINE_MUTEX(TSTDPA_lock);
static int mtktstdpa_get_hw_temp(void)
{
	struct md_info *p_info;
	int size, i;
	
	mutex_lock(&TSTDPA_lock);	
	mtk_mdm_get_md_info(&p_info, &size);
	for(i=0; i<size; i++) 
	{
		mtktstdpa_dprintk("TDPA temperature: name:%s, vaule:%d, invalid_value=%d \n",p_info[i].attribute, p_info[i].value, p_info[i].invalid_value);
		
		if(!strcmp(p_info[i].attribute, "RFTEMP_2G_MD2"))
		{
			mtktstdpa_dprintk("TDPA temperature: RFTEMP_2G_MD2\n");
			if(p_info[i].value != p_info[i].invalid_value)
				break;
		}
		else if(!strcmp(p_info[i].attribute, "RFTEMP_3G_MD2"))
		{
			mtktstdpa_dprintk("TDPA temperature: RFTEMP_3G_MD2\n");
			if(p_info[i].value != p_info[i].invalid_value)
				break;
		}
	}
	
	if(i==size)
	{
		mtktstdpa_dprintk("TDPA temperature: not ready\n");
		mutex_unlock(&TSTDPA_lock);
		
		return -127000;
	}
	else
	{	
		mtktstdpa_dprintk("TDPA temperature: %d\n",p_info[i].value);
		
		if((p_info[i].value>100000) || (p_info[i].value<-30000))
			printk("[Power/TDPA_Thermal] TDPA T=%d\n",p_info[i].value);
		
		mutex_unlock(&TSTDPA_lock);
		return (p_info[i].value);
	}	
	
}
    
static int mtktstdpa_get_temp(struct thermal_zone_device *thermal,
             unsigned long *t)
{
	*t = mtktstdpa_get_hw_temp();
	return 0;
}

static int mtktstdpa_bind(struct thermal_zone_device *thermal,
                        struct thermal_cooling_device *cdev)
{
	int table_val=0;

	if(!strcmp(cdev->type, g_bind0))
	{
		table_val = 0;
		mtktstdpa_dprintk("[mtktstdpa_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind1))
	{
		table_val = 1;
		mtktstdpa_dprintk("[mtktstdpa_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind2))
	{
		table_val = 2;
		mtktstdpa_dprintk("[mtktstdpa_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind3))
	{
		table_val = 3;
		mtktstdpa_dprintk("[mtktstdpa_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind4))
	{
		table_val = 4;
		mtktstdpa_dprintk("[mtktstdpa_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind5))
	{
		table_val = 5;
		mtktstdpa_dprintk("[mtktstdpa_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind6))
	{
		table_val = 6;
		mtktstdpa_dprintk("[mtktstdpa_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind7))
	{
		table_val = 7;
		mtktstdpa_dprintk("[mtktstdpa_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind8))
	{
		table_val = 8;
		mtktstdpa_dprintk("[mtktstdpa_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind9))
	{
		table_val = 9;
		mtktstdpa_dprintk("[mtktstdpa_bind] %s\n", cdev->type);
	}
	else
		return 0;


	if (mtk_thermal_zone_bind_cooling_device(thermal, table_val, cdev)) 
	{
		mtktstdpa_dprintk("[mtktstdpa_bind] error binding cooling dev\n");
		return -EINVAL;
	} 
	else 
	{
		mtktstdpa_dprintk("[mtktstdpa_bind] binding OK\n");
	}

	return 0;
}

static int mtktstdpa_unbind(struct thermal_zone_device *thermal,
        struct thermal_cooling_device *cdev)
{
	int table_val=0;

	if(!strcmp(cdev->type, g_bind0))
	{
		table_val = 0;
		mtktstdpa_dprintk("[mtktstdpa_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind1))
	{
		table_val = 1;
		mtktstdpa_dprintk("[mtktstdpa_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind2))
	{
		table_val = 2;
		mtktstdpa_dprintk("[mtktstdpa_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind3))
	{
		table_val = 3;
		mtktstdpa_dprintk("[mtktstdpa_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind4))
	{
		table_val = 4;
		mtktstdpa_dprintk("[mtktstdpa_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind5))
	{
		table_val = 5;
		mtktstdpa_dprintk("[mtktstdpa_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind6))
	{
		table_val = 6;
		mtktstdpa_dprintk("[mtktstdpa_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind7))
	{
		table_val = 7;
		mtktstdpa_dprintk("[mtktstdpa_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind8))
	{
		table_val = 8;
		mtktstdpa_dprintk("[mtktstdpa_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind9))
	{
		table_val = 9;
		mtktstdpa_dprintk("[mtktstdpa_unbind] %s\n", cdev->type);
	}
	else
		return 0;

	if (thermal_zone_unbind_cooling_device(thermal, table_val, cdev)) 
	{
		mtktstdpa_dprintk("[mtktstdpa_unbind] error unbinding cooling dev\n");
		return -EINVAL;
	} 
	else 
	{
		mtktstdpa_dprintk("[mtktstdpa_unbind] unbinding OK\n");
	}

	return 0;
}

static int mtktstdpa_get_mode(struct thermal_zone_device *thermal,
          enum thermal_device_mode *mode)
{
	*mode = (kernelmode) ? THERMAL_DEVICE_ENABLED
		: THERMAL_DEVICE_DISABLED;

	return 0;
}

static int mtktstdpa_set_mode(struct thermal_zone_device *thermal,
          enum thermal_device_mode mode)
{
	kernelmode = mode;
	return 0;
}

static int mtktstdpa_get_trip_type(struct thermal_zone_device *thermal, int trip,
         enum thermal_trip_type *type)
{
	*type = g_THERMAL_TRIP[trip];
	return 0;
}

static int mtktstdpa_get_trip_temp(struct thermal_zone_device *thermal, int trip,
         unsigned long *temp)
{
	*temp = trip_temp[trip];
	return 0;
}

static int mtktstdpa_get_crit_temp(struct thermal_zone_device *thermal,
         unsigned long *temperature)
{
	*temperature = mtktstdpa_TEMP_CRIT;
	return 0;
}

/* bind callback functions to thermalzone */
static struct thermal_zone_device_ops mtktstdpa_dev_ops = {
	.bind = mtktstdpa_bind,
	.unbind = mtktstdpa_unbind,
	.get_temp = mtktstdpa_get_temp,
	.get_mode = mtktstdpa_get_mode,
	.set_mode = mtktstdpa_set_mode,
	.get_trip_type = mtktstdpa_get_trip_type,
	.get_trip_temp = mtktstdpa_get_trip_temp,
	.get_crit_temp = mtktstdpa_get_crit_temp,
};

/*
 * cooling device callback functions (mtktstdpa_cooling_sysrst_ops)
 * 1 : ON and 0 : OFF
 */
static int tstdpa_sysrst_get_max_state(struct thermal_cooling_device *cdev,
         unsigned long *state)
{        
	*state = 1;    
	return 0;
}
static int tstdpa_sysrst_get_cur_state(struct thermal_cooling_device *cdev,
         unsigned long *state)
{        
	*state = cl_dev_sysrst_state;
	return 0;
}
static int tstdpa_sysrst_set_cur_state(struct thermal_cooling_device *cdev,
         unsigned long state)
{
	cl_dev_sysrst_state = state;
	if(cl_dev_sysrst_state == 1)
	{
		printk("Power/TDPA_Thermal: reset, reset, reset!!!");
		printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
		printk("*****************************************");
		printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");

		BUG();
		//arch_reset(0,NULL);   
	}    
	return 0;
}

/* bind fan callbacks to fan device */
static struct thermal_cooling_device_ops mtktstdpa_cooling_sysrst_ops = {
	.get_max_state = tstdpa_sysrst_get_max_state,
	.get_cur_state = tstdpa_sysrst_get_cur_state,
	.set_cur_state = tstdpa_sysrst_set_cur_state,
};

int mtktstdpa_register_thermal(void);
void mtktstdpa_unregister_thermal(void);

static int mtktstdpa_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
	int len = 0;
	char *p = buf;
    
		p += sprintf(p, "[ mtktstdpa_read] trip_0_temp=%d,trip_1_temp=%d,trip_2_temp=%d,trip_3_temp=%d,trip_4_temp=%d,\n\
trip_5_temp=%d,trip_6_temp=%d,trip_7_temp=%d,trip_8_temp=%d,trip_9_temp=%d,\n\
g_THERMAL_TRIP_0=%d,g_THERMAL_TRIP_1=%d,g_THERMAL_TRIP_2=%d,g_THERMAL_TRIP_3=%d,g_THERMAL_TRIP_4=%d,\n\
g_THERMAL_TRIP_5=%d,g_THERMAL_TRIP_6=%d,g_THERMAL_TRIP_7=%d,g_THERMAL_TRIP_8=%d,g_THERMAL_TRIP_9=%d,\n\
cooldev0=%s,cooldev1=%s,cooldev2=%s,cooldev3=%s,cooldev4=%s,\n\
cooldev5=%s,cooldev6=%s,cooldev7=%s,cooldev8=%s,cooldev9=%s,time_ms=%d\n",
		trip_temp[0],trip_temp[1],trip_temp[2],trip_temp[3],trip_temp[4],
		trip_temp[5],trip_temp[6],trip_temp[7],trip_temp[8],trip_temp[9],
		g_THERMAL_TRIP[0],g_THERMAL_TRIP[1],g_THERMAL_TRIP[2],g_THERMAL_TRIP[3],g_THERMAL_TRIP[4],
		g_THERMAL_TRIP[5],g_THERMAL_TRIP[6],g_THERMAL_TRIP[7],g_THERMAL_TRIP[8],g_THERMAL_TRIP[9],
		g_bind0,g_bind1,g_bind2,g_bind3,g_bind4,g_bind5,g_bind6,g_bind7,g_bind8,g_bind9,
		interval*1000);
    
	*start = buf + off;
    
	len = p - buf;
	if (len > off)
		len -= off;
	else
		len = 0;
    
	return len < count ? len  : count;
}

static ssize_t mtktstdpa_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	int len=0,time_msec=0;
	int trip[10]={0};
	int t_type[10]={0};
	int i;
	char bind0[20],bind1[20],bind2[20],bind3[20],bind4[20];
	char bind5[20],bind6[20],bind7[20],bind8[20],bind9[20];
	char desc[512];


	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
	{
		return 0;
	}
	desc[len] = '\0';
		
	if (sscanf(desc, "%d %d %d %s %d %d %s %d %d %s %d %d %s %d %d %s %d %d %s %d %d %s %d %d %s %d %d %s %d %d %s %d",
			&num_trip, &trip[0],&t_type[0],bind0, &trip[1],&t_type[1],bind1,
			&trip[2],&t_type[2],bind2, &trip[3],&t_type[3],bind3,
			&trip[4],&t_type[4],bind4, &trip[5],&t_type[5],bind5,
			&trip[6],&t_type[6],bind6, &trip[7],&t_type[7],bind7,
			&trip[8],&t_type[8],bind8, &trip[9],&t_type[9],bind9,
			&time_msec) == 32)
	{
		mtktstdpa_dprintk("[mtktstdpa_write] mtktstdpa_unregister_thermal\n");
		mtktstdpa_unregister_thermal();
	
		for(i=0; i<num_trip; i++)
			g_THERMAL_TRIP[i] = t_type[i];	

		g_bind0[0]=g_bind1[0]=g_bind2[0]=g_bind3[0]=g_bind4[0]=g_bind5[0]=g_bind6[0]=g_bind7[0]=g_bind8[0]=g_bind9[0]='\0';
				
		for(i=0; i<20; i++)
		{
			g_bind0[i]=bind0[i]; 
			g_bind1[i]=bind1[i]; 
			g_bind2[i]=bind2[i]; 
			g_bind3[i]=bind3[i]; 
			g_bind4[i]=bind4[i];
			g_bind5[i]=bind5[i]; 
			g_bind6[i]=bind6[i]; 
			g_bind7[i]=bind7[i]; 
			g_bind8[i]=bind8[i]; 
			g_bind9[i]=bind9[i];
		}

		mtktstdpa_dprintk("[mtktstdpa_write] g_THERMAL_TRIP_0=%d,g_THERMAL_TRIP_1=%d,g_THERMAL_TRIP_2=%d,g_THERMAL_TRIP_3=%d,g_THERMAL_TRIP_4=%d,\
g_THERMAL_TRIP_5=%d,g_THERMAL_TRIP_6=%d,g_THERMAL_TRIP_7=%d,g_THERMAL_TRIP_8=%d,g_THERMAL_TRIP_9=%d,\n",
				g_THERMAL_TRIP[0],g_THERMAL_TRIP[1],g_THERMAL_TRIP[2],g_THERMAL_TRIP[3],g_THERMAL_TRIP[4],
				g_THERMAL_TRIP[5],g_THERMAL_TRIP[6],g_THERMAL_TRIP[7],g_THERMAL_TRIP[8],g_THERMAL_TRIP[9]);
		mtktstdpa_dprintk("[mtktstdpa_write] cooldev0=%s,cooldev1=%s,cooldev2=%s,cooldev3=%s,cooldev4=%s,\
cooldev5=%s,cooldev6=%s,cooldev7=%s,cooldev8=%s,cooldev9=%s\n",
				g_bind0,g_bind1,g_bind2,g_bind3,g_bind4,g_bind5,g_bind6,g_bind7,g_bind8,g_bind9);

		for(i=0; i<num_trip; i++)
		{
			trip_temp[i]=trip[i];
		}

		interval=time_msec / 1000;

		mtktstdpa_dprintk("[mtktstdpa_write] trip_0_temp=%d,trip_1_temp=%d,trip_2_temp=%d,trip_3_temp=%d,trip_4_temp=%d,\
trip_5_temp=%d,trip_6_temp=%d,trip_7_temp=%d,trip_8_temp=%d,trip_9_temp=%d,time_ms=%d\n", 
				trip_temp[0],trip_temp[1],trip_temp[2],trip_temp[3],trip_temp[4],
				trip_temp[5],trip_temp[6],trip_temp[7],trip_temp[8],trip_temp[9],interval*1000);

		mtktstdpa_dprintk("[mtktstdpa_write] mtktstdpa_register_thermal\n");
		mtktstdpa_register_thermal();

		return count;
	}
	else
	{
		mtktstdpa_dprintk("[mtktstdpa_write] bad argument\n");
	}
		
	return -EINVAL;
		
}

int mtktstdpa_register_cooler(void)
{
	/* cooling devices */
	cl_dev_sysrst = mtk_thermal_cooling_device_register("mtktstdpa-sysrst", NULL,
		&mtktstdpa_cooling_sysrst_ops);
	return 0;
}

int mtktstdpa_register_thermal(void)
{
	mtktstdpa_dprintk("[mtktstdpa_register_thermal] \n");

	/* trips */
	thz_dev = mtk_thermal_zone_device_register("mtktstdpa", num_trip, NULL,
		&mtktstdpa_dev_ops, 0, 0, 0, interval*1000);

	return 0;
}

void mtktstdpa_unregister_cooler(void)
{
	if (cl_dev_sysrst)
	{
		mtk_thermal_cooling_device_unregister(cl_dev_sysrst);
		cl_dev_sysrst = NULL;
	}
}

void mtktstdpa_unregister_thermal(void)
{
	mtktstdpa_dprintk("[mtktstdpa_unregister_thermal] \n");
    
	if (thz_dev) 
	{
		mtk_thermal_zone_device_unregister(thz_dev);
		thz_dev = NULL;
	}
}

static int __init mtktstdpa_init(void)
{
	int err = 0;
	struct proc_dir_entry *entry = NULL;
	struct proc_dir_entry *mtktstdpa_dir = NULL;

	mtktstdpa_dprintk("[mtktstdpa_init] \n");

	err = mtktstdpa_register_cooler();
	if(err)
		return err;
	
	err = mtktstdpa_register_thermal();
	if (err)
		goto err_unreg;

	mtktstdpa_dir = proc_mkdir("mtktstdpa", NULL);
	if (!mtktstdpa_dir)
	{
		mtktstdpa_dprintk("[mtktstdpa_init]: mkdir /proc/mtktstdpa failed\n");
	}
	else
	{
		entry = create_proc_entry("mtktstdpa", S_IRUGO | S_IWUSR, mtktstdpa_dir);
		if (entry)
		{
			entry->read_proc = mtktstdpa_read;
			entry->write_proc = mtktstdpa_write;
		}
	}

	return 0;

err_unreg:
	mtktstdpa_unregister_cooler();
	return err;
}

static void __exit mtktstdpa_exit(void)
{
	mtktstdpa_dprintk("[mtktstdpa_exit] \n");
	mtktstdpa_unregister_thermal();
	mtktstdpa_unregister_cooler();
}

module_init(mtktstdpa_init);
module_exit(mtktstdpa_exit);
