#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/xlog.h>
#include <linux/types.h>
#include <linux/kobject.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>
#include "mach/mtk_thermal_monitor.h"
#include <mach/system.h>


#define MAX_NUM_INSTANCE_MTK_COOLER_CAM  1

#if 1
#define mtk_cooler_cam_dprintk(fmt, args...) \
  do { xlog_printk(ANDROID_LOG_DEBUG, "thermal/cooler/cam", fmt, ##args); } while(0)
#else
#define mtk_cooler_cam_dprintk(fmt, args...) 
#endif

static struct thermal_cooling_device *cl_cam_dev[MAX_NUM_INSTANCE_MTK_COOLER_CAM] = {0};
static unsigned long cl_cam_state[MAX_NUM_INSTANCE_MTK_COOLER_CAM] = {0};

static unsigned int _cl_cam = 0;

#define MAX_LEN (256)

static ssize_t _cl_cam_write( struct file *filp, const char __user *buf, size_t len, loff_t *data)
{
	int ret = 0;
	char tmp[MAX_LEN] = {0};

	/* write data to the buffer */
	if ( copy_from_user(tmp, buf, len) ) {
		return -EFAULT;
	}

	ret = kstrtouint(tmp, 10, &_cl_cam);
	if (ret)
		WARN_ON(1);

	mtk_cooler_cam_dprintk("[%s] %s = %d\n", __func__, tmp, _cl_cam);

	return len;
}

static int _cl_cam_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", _cl_cam);
	mtk_cooler_cam_dprintk("[%s] %d\n", __func__, _cl_cam);

	return 0;
}

static int _cl_cam_open(struct inode *inode, struct file *file)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
    return single_open(file, _cl_cam_read, PDE_DATA(inode));
#else
    return single_open(file, _cl_cam_read, PDE(inode)->data);
#endif
}

static const struct file_operations _cl_cam_fops = {
    .owner = THIS_MODULE,
    .open = _cl_cam_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .write = _cl_cam_write,
    .release = single_release,
};

static int 
mtk_cl_cam_get_max_state(struct thermal_cooling_device *cdev,
                              unsigned long *state)
{        
    *state = 1;
    //mtk_cooler_cam_dprintk("mtk_cl_cam_get_max_state() %s %d\n", cdev->type, *state);
    return 0;
}

static int 
mtk_cl_cam_get_cur_state(struct thermal_cooling_device *cdev,
                              unsigned long *state)
{
    *state = *((unsigned long*) cdev->devdata);
    //mtk_cooler_cam_dprintk("mtk_cl_cam_get_cur_state() %s %d\n", cdev->type, *state);
    return 0;
}

static int 
mtk_cl_cam_set_cur_state(struct thermal_cooling_device *cdev,
                              unsigned long state)
{
    //mtk_cooler_cam_dprintk("mtk_cl_cam_set_cur_state() %s %d\n", cdev->type, state);
    
    *((unsigned long*) cdev->devdata) = state;
    
    if(1 == state)
    {
        _cl_cam = 1;
    }
    else
    {
        _cl_cam = 0;
    }
    
    return 0;
}

/* bind fan callbacks to fan device */
static struct thermal_cooling_device_ops mtk_cl_cam_ops = {
    .get_max_state = mtk_cl_cam_get_max_state,
    .get_cur_state = mtk_cl_cam_get_cur_state,
    .set_cur_state = mtk_cl_cam_set_cur_state,
};

static int mtk_cooler_cam_register_ltf(void)
{
    int i;
    mtk_cooler_cam_dprintk("register ltf\n");
    
    for (i = MAX_NUM_INSTANCE_MTK_COOLER_CAM; i-- > 0; )
    {
        char temp[20] = {0};
        sprintf(temp, "mtk-cl-cam%02d", i);
        cl_cam_dev[i] = mtk_thermal_cooling_device_register(temp, 
                                                            (void*) &cl_cam_state[i],
                                                            &mtk_cl_cam_ops);
    }

    return 0;
}

static void mtk_cooler_cam_unregister_ltf(void)
{
    int i;
    mtk_cooler_cam_dprintk("unregister ltf\n");
    
    for (i = MAX_NUM_INSTANCE_MTK_COOLER_CAM; i-- > 0; )
    {
        if (cl_cam_dev[i])
        {
            mtk_thermal_cooling_device_unregister(cl_cam_dev[i]);
            cl_cam_dev[i] = NULL;
            cl_cam_state[i] = 0;
        }
    }
}


static int __init mtk_cooler_cam_init(void)
{
    int err = 0;
    int i;
    
    for (i = MAX_NUM_INSTANCE_MTK_COOLER_CAM; i-- > 0; )
    {
        cl_cam_dev[i] = NULL;
        cl_cam_state[i] = 0;
    }

    mtk_cooler_cam_dprintk("init\n");

    {
        struct proc_dir_entry *entry;

#if 0
        entry = create_proc_entry("driver/cl_cam", S_IRUGO | S_IWUSR, NULL);
        if (NULL != entry)
        {
            entry->read_proc = _cl_cam_read;
            entry->write_proc = _cl_cam_write;
        }
#endif
        entry = proc_create("driver/cl_cam", S_IRUGO | S_IWUSR, NULL, &_cl_cam_fops);
        if (!entry)
        {
            xlog_printk(ANDROID_LOG_DEBUG, "thermal/cooler/cam", "mtk_cooler_cam_init driver/cl_cam creation failed\n");
        }
    }

    err = mtk_cooler_cam_register_ltf();
    if (err)
        goto err_unreg;

    return 0;

err_unreg:
    mtk_cooler_cam_unregister_ltf();
    return err;
}

static void __exit mtk_cooler_cam_exit(void)
{
    mtk_cooler_cam_dprintk("exit\n");
    
    mtk_cooler_cam_unregister_ltf();
}

module_init(mtk_cooler_cam_init);
module_exit(mtk_cooler_cam_exit);



