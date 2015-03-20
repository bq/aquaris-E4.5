#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/xlog.h>
#include <linux/types.h>
#include <linux/kobject.h>
#include "mach/mtk_thermal_monitor.h"
#include <mach/system.h>

#define MAX_NUM_INSTANCE_MTK_COOLER_SHUTDOWN  3

//#define MTK_COOLER_SHUTDOWN_UEVENT
#define MTK_COOLER_SHUTDOWN_SIGNAL

#if defined(MTK_COOLER_SHUTDOWN_SIGNAL)
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>
#include <linux/pid.h>
#include <linux/signal.h>
#include <linux/sched.h>

#define MAX_LEN	256
#endif

#if 1
#define mtk_cooler_shutdown_dprintk(fmt, args...) \
  do { xlog_printk(ANDROID_LOG_DEBUG, "thermal/cooler/shutdown", fmt, ##args); } while(0)
#else
#define mtk_cooler_shutdown_dprintk(fmt, args...) 
#endif

static struct thermal_cooling_device *cl_shutdown_dev[MAX_NUM_INSTANCE_MTK_COOLER_SHUTDOWN] = {0};
static unsigned long cl_shutdown_state[MAX_NUM_INSTANCE_MTK_COOLER_SHUTDOWN] = {0};

#if defined(MTK_COOLER_SHUTDOWN_SIGNAL)

static unsigned int tm_pid = 0;
static unsigned int tm_input_pid = 0;
static unsigned int mtk_cl_sd_rst = 0;
static struct task_struct g_task;
static struct task_struct *pg_task = &g_task;

static ssize_t _mtk_cl_sd_rst_write(struct file *filp, const char __user *buf, size_t len, loff_t *data)
{
	int ret = 0;
	char tmp[MAX_LEN] = {0};

	/* write data to the buffer */
	if ( copy_from_user(tmp, buf, len) ) {
		return -EFAULT;
	}

	ret = kstrtouint(tmp, 10, &mtk_cl_sd_rst);
	if (ret)
		WARN_ON(1);


    if (1 == mtk_cl_sd_rst)
    {
        int i;
        for (i = MAX_NUM_INSTANCE_MTK_COOLER_SHUTDOWN; i-- > 0; )
        {
            cl_shutdown_state[i] = 0;
        }
        mtk_cl_sd_rst = 0;
    }
	mtk_cooler_shutdown_dprintk("[%s] %s = %d\n", __func__, tmp, mtk_cl_sd_rst);

	return len;
}

int _mtk_cl_sd_rst_read(struct seq_file *m, void *v)
{
	return 0;
}

static int _mtk_cl_sd_rst_open(struct inode *inode, struct file *file)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
    return single_open(file, _mtk_cl_sd_rst_read, PDE_DATA(inode));
#else
    return single_open(file, _mtk_cl_sd_rst_read, PDE(inode)->data);
#endif
}

static const struct file_operations _cl_sd_rst_fops = {
    .owner = THIS_MODULE,
    .open = _mtk_cl_sd_rst_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .write = _mtk_cl_sd_rst_write,
    .release = single_release,
};

static ssize_t _mtk_cl_sd_pid_write(struct file *filp, const char __user *buf, size_t len, loff_t *data)
{
	int ret = 0;
	char tmp[MAX_LEN] = {0};

	/* write data to the buffer */
	if ( copy_from_user(tmp, buf, len) ) {
		return -EFAULT;
	}

	ret = kstrtouint(tmp, 10, &tm_input_pid);
	if (ret)
		WARN_ON(1);

	mtk_cooler_shutdown_dprintk("[%s] %s = %d\n", __func__, tmp, tm_input_pid);

	return len;
}

static int _mtk_cl_sd_pid_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", tm_input_pid);
    mtk_cooler_shutdown_dprintk("[%s] %d\n", __func__, tm_input_pid);

	return 0;
}

static int _mtk_cl_sd_pid_open(struct inode *inode, struct file *file)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
    return single_open(file, _mtk_cl_sd_pid_read, PDE_DATA(inode));
#else
    return single_open(file, _mtk_cl_sd_pid_read, PDE(inode)->data);
#endif
}

static const struct file_operations _cl_sd_pid_fops = {
    .owner = THIS_MODULE,
    .open = _mtk_cl_sd_pid_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .write = _mtk_cl_sd_pid_write,
    .release = single_release,
};

static int _mtk_cl_sd_send_signal(void)
{
	int ret = 0;

	if (tm_input_pid == 0) {
		mtk_cooler_shutdown_dprintk("[%s] pid is empty\n", __func__);
		ret = -1;
	}

	mtk_cooler_shutdown_dprintk("[%s] pid is %d, %d\n", __func__, tm_pid, tm_input_pid);

	if (ret == 0 && tm_input_pid != tm_pid) {
		tm_pid = tm_input_pid;
		pg_task = get_pid_task(find_vpid(tm_pid), PIDTYPE_PID);
	}

	if (ret == 0 && pg_task) {
		siginfo_t info;
		info.si_signo = SIGIO;
		info.si_errno = 0;
		info.si_code = 1;
		info.si_addr = NULL;
		ret = send_sig_info(SIGIO, &info, pg_task);
	}

	if (ret != 0) mtk_cooler_shutdown_dprintk("[%s] ret=%d\n", __func__, ret);

	return ret;
}

#endif

static int 
mtk_cl_shutdown_get_max_state(struct thermal_cooling_device *cdev,
                              unsigned long *state)
{        
    *state = 1;
    //mtk_cooler_shutdown_dprintk("mtk_cl_shutdown_get_max_state() %s %d\n", cdev->type, *state);
    return 0;
}

static int 
mtk_cl_shutdown_get_cur_state(struct thermal_cooling_device *cdev,
                              unsigned long *state)
{
    *state = *((unsigned long*) cdev->devdata);
    //mtk_cooler_shutdown_dprintk("mtk_cl_shutdown_get_cur_state() %s %d\n", cdev->type, *state);
    return 0;
}

static int 
mtk_cl_shutdown_set_cur_state(struct thermal_cooling_device *cdev,
                              unsigned long state)
{
#if defined(MTK_COOLER_SHUTDOWN_SIGNAL)
    volatile unsigned long original_state;
#endif
    //mtk_cooler_shutdown_dprintk("mtk_cl_shutdown_set_cur_state() %s %d\n", cdev->type, state);
#if defined(MTK_COOLER_SHUTDOWN_SIGNAL)
    original_state = *((unsigned long*) cdev->devdata);
#endif
    
    *((unsigned long*) cdev->devdata) = state;
    
    if(1 == state)
    {
#if defined(MTK_COOLER_SHUTDOWN_UEVENT)
        {
            // send uevent to notify current call must be dropped
            char event[] = "SHUTDOWN=1";
            char *envp[] = { event, NULL };
        
            kobject_uevent_env(&(cdev->device.kobj), KOBJ_CHANGE, envp);
        }
#endif

#if defined(MTK_COOLER_SHUTDOWN_SIGNAL)
        if (0 == original_state) // make this an edge trigger instead of level trigger
        {
            // send signal to target process
            _mtk_cl_sd_send_signal();
        }
#endif
    }
    
    return 0;
}

/* bind fan callbacks to fan device */
static struct thermal_cooling_device_ops mtk_cl_shutdown_ops = {
    .get_max_state = mtk_cl_shutdown_get_max_state,
    .get_cur_state = mtk_cl_shutdown_get_cur_state,
    .set_cur_state = mtk_cl_shutdown_set_cur_state,
};

static int mtk_cooler_shutdown_register_ltf(void)
{
    int i;
    mtk_cooler_shutdown_dprintk("register ltf\n");
    
    for (i = MAX_NUM_INSTANCE_MTK_COOLER_SHUTDOWN; i-- > 0; )
    {
        char temp[20] = {0};
        sprintf(temp, "mtk-cl-shutdown%02d", i);
        cl_shutdown_dev[i] = mtk_thermal_cooling_device_register(temp, 
                                                                 (void*) &cl_shutdown_state[i],
                                                                 &mtk_cl_shutdown_ops);
    }

    return 0;
}

static void mtk_cooler_shutdown_unregister_ltf(void)
{
    int i;
    mtk_cooler_shutdown_dprintk("unregister ltf\n");
    
    for (i = MAX_NUM_INSTANCE_MTK_COOLER_SHUTDOWN; i-- > 0; )
    {
        if (cl_shutdown_dev[i])
        {
            mtk_thermal_cooling_device_unregister(cl_shutdown_dev[i]);
            cl_shutdown_dev[i] = NULL;
            cl_shutdown_state[i] = 0;
        }
    }
}


static int __init mtk_cooler_shutdown_init(void)
{
    int err = 0;
    int i;
    
    for (i = MAX_NUM_INSTANCE_MTK_COOLER_SHUTDOWN; i-- > 0; )
    {
        cl_shutdown_dev[i] = NULL;
        cl_shutdown_state[i] = 0;
    }

    mtk_cooler_shutdown_dprintk("init\n");
  
#if defined(MTK_COOLER_SHUTDOWN_SIGNAL)
    {
        struct proc_dir_entry *entry;

#if 0
        entry = create_proc_entry("driver/mtk_cl_sd_pid", S_IRUGO | S_IWUSR, NULL);
        if (NULL != entry)
        {
            entry->read_proc = _mtk_cl_sd_pid_read;
            entry->write_proc = _mtk_cl_sd_pid_write;
        }
#endif

        entry = proc_create("driver/mtk_cl_sd_pid", S_IRUGO | S_IWUSR, NULL, &_cl_sd_pid_fops);
        if (!entry)
        {
            xlog_printk(ANDROID_LOG_DEBUG, "thermal/cooler/shutdown", "mtk_cooler_shutdown_init driver/mtk_cl_sd_pid creation failed\n");
        }
    }
    
    {
        struct proc_dir_entry *entry;

#if 0
        entry = create_proc_entry("driver/mtk_cl_sd_rst", S_IRUGO | S_IWUSR | S_IWGRP, NULL);
        if (NULL != entry)
        {
            entry->write_proc = _mtk_cl_sd_rst_write;
            entry->gid = 1000;
        }
#endif
        entry = proc_create("driver/mtk_cl_sd_rst", S_IRUGO | S_IWUSR | S_IWGRP, NULL, &_cl_sd_rst_fops);
        if (!entry)
        {
            xlog_printk(ANDROID_LOG_DEBUG, "thermal/cooler/shutdown", "mtk_cooler_shutdown_init driver/mtk_cl_sd_rst creation failed\n");
        }
        else
        {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
            proc_set_user(entry, 0, 1000);
#else
            entry->gid = 1000;
#endif
        }
    }
#endif

    err = mtk_cooler_shutdown_register_ltf();
    if (err)
        goto err_unreg;

    return 0;

err_unreg:
    mtk_cooler_shutdown_unregister_ltf();
    return err;
}

static void __exit mtk_cooler_shutdown_exit(void)
{
    mtk_cooler_shutdown_dprintk("exit\n");
    
    mtk_cooler_shutdown_unregister_ltf();
}

module_init(mtk_cooler_shutdown_init);
module_exit(mtk_cooler_shutdown_exit);



