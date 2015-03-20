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
#include <linux/spinlock.h>

#include "mach/mtk_thermal_monitor.h"
#include <mach/system.h>

#include "mach/mt_typedefs.h"
#include "mach/mt_thermal.h"

#define MTK_TS_ABB_SW_FILTER         (1)

static unsigned int interval = 0; /* seconds, 0 : no auto polling */
static unsigned int trip_temp[10] = {120000,110000,100000,90000,80000,70000,65000,60000,55000,50000};

static unsigned int cl_dev_sysrst_state = 0;
static struct thermal_zone_device *thz_dev;

static struct thermal_cooling_device *cl_dev_sysrst;
static int mtktsabb_debug_log = 0;
static int kernelmode = 0;
static int g_THERMAL_TRIP[10] = {0,0,0,0,0,0,0,0,0,0};
static int num_trip=0;
static char g_bind0[20]={0};
static char g_bind1[20]={0};
static char g_bind2[20]={0};
static char g_bind3[20]={0};
static char g_bind4[20]={0};
static char g_bind5[20]={0};
static char g_bind6[20]={0};
static char g_bind7[20]={0};
static char g_bind8[20]={0};
static char g_bind9[20]={0};

#define MTKTSabb_TEMP_CRIT 120000 /* 120.000 degree Celsius */
//#define THERMAL_CHANNEL 0x5

#define mtktsabb_dprintk(fmt, args...)   \
do {                                    \
    if (mtktsabb_debug_log) {                \
        xlog_printk(ANDROID_LOG_INFO, "Power/ABB_Thermal", fmt, ##args); \
    }                                   \
} while(0)

extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);
extern int IMM_IsAdcInitReady(void);
//extern int last_abb_t;
//extern int last_CPU2_t;
extern int get_immediate_temp2_wrap(void);
static int mtktsabb_get_hw_temp(void)
{
	int t_ret = 0;

	t_ret = get_immediate_temp2_wrap();//last_CPU2_t;
	//mtktsabb_dprintk("[mtktsabb_get_hw_temp] T_CPU2, %d\n", t_ret);
	return t_ret;
}

static int mtktsabb_get_temp(struct thermal_zone_device *thermal,
			       unsigned long *t)
{
#if MTK_TS_ABB_SW_FILTER == 1
	int curr_temp;
    int temp_temp;
    int ret = 0;
    static int last_abb_read_temp = 0;
  
	curr_temp = mtktsabb_get_hw_temp();
    mtktsabb_dprintk("mtktsabb_get_temp CPU T2=%d\n", curr_temp);

    if ((curr_temp > (trip_temp[0] - 15000)) || (curr_temp < -30000)) // abnormal high temp
        printk("[Power/ABB_Thermal] ABB T=%d\n", curr_temp);

	temp_temp = curr_temp;
	if (curr_temp != 0) // not the first temp read after resume from suspension
	{
		if ((curr_temp > 150000) || (curr_temp < -20000)) // invalid range
		{
			printk("[Power/ABB_Thermal] ABB temp invalid=%d\n", curr_temp);			
			temp_temp = 50000;
			ret = -1;
		}
	    else if (last_abb_read_temp != 0)
	    {
	    	if ((curr_temp - last_abb_read_temp > 20000) ||(last_abb_read_temp - curr_temp > 20000)) //delta 20C, invalid change
	    	{
	      		printk("[Power/ABB_Thermal] ABB temp float hugely temp=%d, lasttemp=%d\n", curr_temp, last_abb_read_temp);
				temp_temp = 50000;
				ret = -1;			
	       	}
    	}
	}
	
	last_abb_read_temp = curr_temp;
	curr_temp = temp_temp;
	*t = (unsigned long) curr_temp;
	return ret;
#else
	int curr_temp;
	curr_temp = mtktsabb_get_hw_temp();
    mtktsabb_dprintk(" mtktsabb_get_temp CPU T2=%d\n", curr_temp);

    if ((curr_temp > (trip_temp[0] - 15000)) || (curr_temp < -30000))
        printk("[Power/ABB_Thermal] ABB T=%d\n", curr_temp);
	
    *t  = curr_temp;

	return 0;
#endif
}

static int mtktsabb_bind(struct thermal_zone_device *thermal,
			struct thermal_cooling_device *cdev)
{
    int table_val=0;

	if(!strcmp(cdev->type, g_bind0))
	{
			table_val = 0;
			mtktsabb_dprintk("[mtktsabb_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind1))
	{
			table_val = 1;
			mtktsabb_dprintk("[mtktsabb_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind2))
	{
			table_val = 2;
			mtktsabb_dprintk("[mtktsabb_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind3))
	{
			table_val = 3;
			mtktsabb_dprintk("[mtktsabb_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind4))
	{
			table_val = 4;
			mtktsabb_dprintk("[mtktsabb_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind5))
	{
			table_val = 5;
			mtktsabb_dprintk("[mtktsabb_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind6))
	{
			table_val = 6;
			mtktsabb_dprintk("[mtktsabb_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind7))
	{
			table_val = 7;
			mtktsabb_dprintk("[mtktsabb_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind8))
	{
			table_val = 8;
			mtktsabb_dprintk("[mtktsabb_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind9))
	{
			table_val = 9;
			mtktsabb_dprintk("[mtktsabb_bind] %s\n", cdev->type);
	}
	else
	{
		return 0;
	}

    if (mtk_thermal_zone_bind_cooling_device(thermal, table_val, cdev)) {
	    mtktsabb_dprintk("[mtktsabb_bind] error binding cooling dev\n");
		return -EINVAL;
	} else {
	    mtktsabb_dprintk("[mtktsabb_bind] binding OK, %d\n", table_val);
    }

	return 0;
}

static int mtktsabb_unbind(struct thermal_zone_device *thermal,
			  struct thermal_cooling_device *cdev)
{
    int table_val=0;

		if(!strcmp(cdev->type, g_bind0))
		{
				table_val = 0;
				mtktsabb_dprintk("[mtktsabb_unbind] %s\n", cdev->type);
		}
		else if(!strcmp(cdev->type, g_bind1))
		{
				table_val = 1;
				mtktsabb_dprintk("[mtktsabb_unbind] %s\n", cdev->type);
		}
		else if(!strcmp(cdev->type, g_bind2))
		{
				table_val = 2;
				mtktsabb_dprintk("[mtktsabb_unbind] %s\n", cdev->type);
		}
		else if(!strcmp(cdev->type, g_bind3))
		{
				table_val = 3;
				mtktsabb_dprintk("[mtktsabb_unbind] %s\n", cdev->type);
		}
		else if(!strcmp(cdev->type, g_bind4))
		{
				table_val = 4;
				mtktsabb_dprintk("[mtktsabb_unbind] %s\n", cdev->type);
		}
		else if(!strcmp(cdev->type, g_bind5))
		{
				table_val = 5;
				mtktsabb_dprintk("[mtktsabb_unbind] %s\n", cdev->type);
		}
		else if(!strcmp(cdev->type, g_bind6))
		{
				table_val = 6;
				mtktsabb_dprintk("[mtktsabb_unbind] %s\n", cdev->type);
		}
		else if(!strcmp(cdev->type, g_bind7))
		{
				table_val = 7;
				mtktsabb_dprintk("[mtktsabb_unbind] %s\n", cdev->type);
		}
		else if(!strcmp(cdev->type, g_bind8))
		{
				table_val = 8;
				mtktsabb_dprintk("[mtktsabb_unbind] %s\n", cdev->type);
		}
		else if(!strcmp(cdev->type, g_bind9))
		{
				table_val = 9;
				mtktsabb_dprintk("[mtktsabb_unbind] %s\n", cdev->type);
		}
		else
				return 0;

    if (thermal_zone_unbind_cooling_device(thermal, table_val, cdev)) {
	    mtktsabb_dprintk("[mtktsabb_unbind] error unbinding cooling dev\n");
		return -EINVAL;
	} else {
	    mtktsabb_dprintk("[mtktsabb_unbind] unbinding OK\n");
    }

	return 0;
}

static int mtktsabb_get_mode(struct thermal_zone_device *thermal,
			    enum thermal_device_mode *mode)
{
    *mode = (kernelmode) ? THERMAL_DEVICE_ENABLED
			     : THERMAL_DEVICE_DISABLED;
		return 0;
}

static int mtktsabb_set_mode(struct thermal_zone_device *thermal,
			    enum thermal_device_mode mode)
{
		kernelmode = mode;
		return 0;
}

static int mtktsabb_get_trip_type(struct thermal_zone_device *thermal, int trip,
				 enum thermal_trip_type *type)
{
		*type = g_THERMAL_TRIP[trip];
		return 0;
}

static int mtktsabb_get_trip_temp(struct thermal_zone_device *thermal, int trip,
				 unsigned long *temp)
{
		*temp = trip_temp[trip];
		return 0;
}

static int mtktsabb_get_crit_temp(struct thermal_zone_device *thermal,
				 unsigned long *temperature)
{
		*temperature = MTKTSabb_TEMP_CRIT;
		return 0;
}

/* bind callback functions to thermalzone */
static struct thermal_zone_device_ops mtktsabb_dev_ops = {
	.bind = mtktsabb_bind,
	.unbind = mtktsabb_unbind,
	.get_temp = mtktsabb_get_temp,
	.get_mode = mtktsabb_get_mode,
	.set_mode = mtktsabb_set_mode,
	.get_trip_type = mtktsabb_get_trip_type,
	.get_trip_temp = mtktsabb_get_trip_temp,
	.get_crit_temp = mtktsabb_get_crit_temp,
};


static int tsabb_sysrst_get_max_state(struct thermal_cooling_device *cdev,
				 unsigned long *state)
{
		*state = 1;
		return 0;
}
static int tsabb_sysrst_get_cur_state(struct thermal_cooling_device *cdev,
				 unsigned long *state)
{
		*state = cl_dev_sysrst_state;
		return 0;
}
static int tsabb_sysrst_set_cur_state(struct thermal_cooling_device *cdev,
				 unsigned long state)
{
    cl_dev_sysrst_state = state;
    if(cl_dev_sysrst_state == 1)
		{
				printk("Power/abb_Thermal: reset, reset, reset!!!");
				printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
				printk("*****************************************");
				printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");

				BUG();
				//arch_reset(0,NULL);
		}
		return 0;
}

static struct thermal_cooling_device_ops mtktsabb_cooling_sysrst_ops = {
	.get_max_state = tsabb_sysrst_get_max_state,
	.get_cur_state = tsabb_sysrst_get_cur_state,
	.set_cur_state = tsabb_sysrst_set_cur_state,
};

static int mtktsabb_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    char *p = buf;

		p += sprintf(p, "[ mtktsabb_read] trip_0_temp=%d,trip_1_temp=%d,trip_2_temp=%d,trip_3_temp=%d,trip_4_temp=%d,\n\
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

int mtktsabb_register_thermal(void);
void mtktsabb_unregister_thermal(void);

static ssize_t mtktsabb_write(struct file *file, const char *buffer, unsigned long count, void *data)
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
				mtktsabb_dprintk("[mtktsabb_write] mtktsabb_unregister_thermal\n");
				mtktsabb_unregister_thermal();

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

				mtktsabb_dprintk("[mtktsabb_write] g_THERMAL_TRIP_0=%d,g_THERMAL_TRIP_1=%d,g_THERMAL_TRIP_2=%d,g_THERMAL_TRIP_3=%d,g_THERMAL_TRIP_4=%d,\
g_THERMAL_TRIP_5=%d,g_THERMAL_TRIP_6=%d,g_THERMAL_TRIP_7=%d,g_THERMAL_TRIP_8=%d,g_THERMAL_TRIP_9=%d,\n",
													g_THERMAL_TRIP[0],g_THERMAL_TRIP[1],g_THERMAL_TRIP[2],g_THERMAL_TRIP[3],g_THERMAL_TRIP[4],
													g_THERMAL_TRIP[5],g_THERMAL_TRIP[6],g_THERMAL_TRIP[7],g_THERMAL_TRIP[8],g_THERMAL_TRIP[9]);
				mtktsabb_dprintk("[mtktsabb_write] cooldev0=%s,cooldev1=%s,cooldev2=%s,cooldev3=%s,cooldev4=%s,\
cooldev5=%s,cooldev6=%s,cooldev7=%s,cooldev8=%s,cooldev9=%s\n",
													g_bind0,g_bind1,g_bind2,g_bind3,g_bind4,g_bind5,g_bind6,g_bind7,g_bind8,g_bind9);

				for(i=0; i<num_trip; i++)
				{
						trip_temp[i]=trip[i];
				}

				interval=time_msec / 1000;

				mtktsabb_dprintk("[mtktsabb_write] trip_0_temp=%d,trip_1_temp=%d,trip_2_temp=%d,trip_3_temp=%d,trip_4_temp=%d,\
trip_5_temp=%d,trip_6_temp=%d,trip_7_temp=%d,trip_8_temp=%d,trip_9_temp=%d,time_ms=%d\n",
						trip_temp[0],trip_temp[1],trip_temp[2],trip_temp[3],trip_temp[4],
						trip_temp[5],trip_temp[6],trip_temp[7],trip_temp[8],trip_temp[9],interval*1000);

				mtktsabb_dprintk("[mtktsabb_write] mtktsabb_register_thermal\n");
				mtktsabb_register_thermal();

				return count;
		}
		else
		{
				mtktsabb_dprintk("[mtktsabb_write] bad argument\n");
		}

		return -EINVAL;
}

int mtktsabb_register_cooler(void)
{
    cl_dev_sysrst = mtk_thermal_cooling_device_register("mtktsabb-sysrst", NULL,
                    &mtktsabb_cooling_sysrst_ops);
    return 0;
}
int mtktsabb_register_thermal(void)
{
    mtktsabb_dprintk("[mtktsabb_register_thermal] \n");

    /* trips : trip 0~3 */
    thz_dev = mtk_thermal_zone_device_register("mtktsabb", num_trip, NULL,
                    &mtktsabb_dev_ops, 0, 0, 0, interval*1000);

    return 0;
}

void mtktsabb_unregister_cooler(void)
{
		if (cl_dev_sysrst) {
        mtk_thermal_cooling_device_unregister(cl_dev_sysrst);
        cl_dev_sysrst = NULL;
    }
}

void mtktsabb_unregister_thermal(void)
{
    mtktsabb_dprintk("[mtktsabb_unregister_thermal] \n");

	  if (thz_dev) {
        mtk_thermal_zone_device_unregister(thz_dev);
        thz_dev = NULL;
    }
}

static int __init mtktsabb_init(void)
{
		int err = 0;
		struct proc_dir_entry *entry = NULL;
    struct proc_dir_entry *mtktsabb_dir = NULL;

    mtktsabb_dprintk("[mtktsabb_init] \n");

		err = mtktsabb_register_cooler();
		if(err)
			return err;
		err = mtktsabb_register_thermal();
		if (err)
			goto err_unreg;

    mtktsabb_dprintk("[mtktsabb_init] \n");

    mtktsabb_dir = proc_mkdir("mtktsabb", NULL);
    if (!mtktsabb_dir)
    {
        mtktsabb_dprintk("[mtktsabb_init]: mkdir /proc/mtktsabb failed\n");
    }
    else
    {
        entry = create_proc_entry("mtktsabb", S_IRUGO | S_IWUSR, mtktsabb_dir);
        if (entry)
        {
            entry->read_proc = mtktsabb_read;
            entry->write_proc = mtktsabb_write;
        }
    }

		return 0;

err_unreg:
		mtktsabb_unregister_cooler();
		return err;
}

static void __exit mtktsabb_exit(void)
{
    mtktsabb_dprintk("[mtktsabb_exit] \n");
		mtktsabb_unregister_thermal();
		mtktsabb_unregister_cooler();
}

module_init(mtktsabb_init);
module_exit(mtktsabb_exit);
