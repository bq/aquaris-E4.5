#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/module.h>
#include <generated/autoconf.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/param.h>
#include <linux/uaccess.h>
#include <linux/sched.h>

#include <linux/workqueue.h>
#include <linux/semaphore.h>
#include <linux/slab.h>

#include "mtkfb_vsync.h"

#include <linux/xlog.h>

#define VSYNC_DBG(...) xlog_printk(ANDROID_LOG_DEBUG, MTKFB_VSYNC_DEVNAME, __VA_ARGS__)

#define VSYNC_INF(...) xlog_printk(ANDROID_LOG_INFO,  MTKFB_VSYNC_DEVNAME, __VA_ARGS__)
#define VSYNC_WRN(...) xlog_printk(ANDROID_LOG_WARN,  MTKFB_VSYNC_DEVNAME, __VA_ARGS__)
#define VSYNC_ERR(...) xlog_printk(ANDROID_LOG_ERROR, MTKFB_VSYNC_DEVNAME, __VA_ARGS__)

static size_t mtkfb_vsync_on = false;
#define MTKFB_VSYNC_LOG(fmt, arg...) \
    do { \
        if (mtkfb_vsync_on) VSYNC_WRN(fmt, ##arg); \
    }while (0)

#define MTKFB_VSYNC_FUNC()	\
	do { \
		if(mtkfb_vsync_on) VSYNC_WRN("[Func]%s\n", __func__); \
	}while (0)

static dev_t mtkfb_vsync_devno;
static struct cdev *mtkfb_vsync_cdev;
static struct class *mtkfb_vsync_class = NULL;

DEFINE_SEMAPHORE(mtkfb_vsync_sem);

extern void mtkfb_waitVsync(void);
extern void mtkfb_disable_non_fb_layer(void);

#if defined(MTK_HDMI_SUPPORT)
extern void hdmi_waitVsync();
#endif

void mtkfb_vsync_log_enable(int enable)
{
    mtkfb_vsync_on = enable;
	MTKFB_VSYNC_LOG("mtkfb_vsync log %s\n", enable?"enabled":"disabled");
}

static int mtkfb_vsync_open(struct inode *inode, struct file *file)
{
    VSYNC_DBG("driver open\n");
    return 0;
}

static ssize_t mtkfb_vsync_read(struct file *file, char __user *data, size_t len, loff_t *ppos)
{
    VSYNC_DBG("driver read\n");
    return 0;
}

static int mtkfb_vsync_release(struct inode *inode, struct file *file)
{
    VSYNC_DBG("driver release\n");
    VSYNC_DBG("reset overlay engine\n");
    mtkfb_disable_non_fb_layer();
	return 0;
}

static int mtkfb_vsync_flush(struct file * a_pstFile , fl_owner_t a_id)
{
    // To Do : error handling here
    return 0;
}

static long mtkfb_vsync_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
	MTKFB_VSYNC_FUNC();
    switch (cmd)
    {
        case MTKFB_VSYNC_IOCTL:
        {
            vsync_src sync_type;            
			MTKFB_VSYNC_LOG("[MTKFB_VSYNC]: enter MTKFB_VSYNC_IOCTL %d\n", arg);			

			if(arg == MTKFB_VSYNC_SOURCE_HDMI)
			{
#if defined(MTK_HDMI_SUPPORT)
                if (down_interruptible(&mtkfb_vsync_sem)) {
         			printk("[mtkfb_vsync_ioctl] can't get semaphore,%d\n", __LINE__);
    				msleep(20);
         			return ret;
        		}
                hdmi_waitVsync();
    			up(&mtkfb_vsync_sem);
    			MTKFB_VSYNC_LOG("[MTKFB_VSYNC]: leave MTKFB_VSYNC_IOCTL, %d\n", __LINE__);
#else
                MTKFB_VSYNC_LOG("[MTKFB_VSYNC]: NS leave MTKFB_VSYNC_IOCTL, %d\n", __LINE__);
                ret = -EFAULT;
#endif                
                return ret;
			}
			
			if (down_interruptible(&mtkfb_vsync_sem)) {
     			printk("[mtkfb_vsync_ioctl] can't get semaphore,%d\n", __LINE__);
				msleep(20);
     			return ret;
    		}
            mtkfb_waitVsync();
			up(&mtkfb_vsync_sem);
			MTKFB_VSYNC_LOG("[MTKFB_VSYNC]: leave MTKFB_VSYNC_IOCTL\n");
        }
        break;
    }
    return ret;
}


static struct file_operations mtkfb_vsync_fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = mtkfb_vsync_unlocked_ioctl,
    .open           = mtkfb_vsync_open,
    .release        = mtkfb_vsync_release,
    .flush          = mtkfb_vsync_flush,
    .read           = mtkfb_vsync_read,
};

static int mtkfb_vsync_probe(struct platform_device *pdev)
{
    struct class_device;
    struct class_device *class_dev = NULL;
    
    printk("\n=== MTKFB_VSYNC probe ===\n");

    if (alloc_chrdev_region(&mtkfb_vsync_devno, 0, 1, MTKFB_VSYNC_DEVNAME))
    {
        VSYNC_ERR("can't get device major number...\n");
        return -EFAULT;
    }

    printk("get device major number (%d)\n", mtkfb_vsync_devno);

    mtkfb_vsync_cdev = cdev_alloc();
    mtkfb_vsync_cdev->owner = THIS_MODULE;
    mtkfb_vsync_cdev->ops = &mtkfb_vsync_fops;

    cdev_add(mtkfb_vsync_cdev, mtkfb_vsync_devno, 1);

    mtkfb_vsync_class = class_create(THIS_MODULE, MTKFB_VSYNC_DEVNAME);
    class_dev = (struct class_device *)device_create(mtkfb_vsync_class, NULL, mtkfb_vsync_devno, NULL, MTKFB_VSYNC_DEVNAME);

    VSYNC_INF("probe is done\n");
    return 0;
}

static int mtkfb_vsync_remove(struct platform_device *pdev)
{
    VSYNC_INF("device remove\n");
    return 0;
}

static void mtkfb_vsync_shutdown(struct platform_device *pdev)
{
    printk("mtkfb_vsync device shutdown\n");
}

static int mtkfb_vsync_suspend(struct platform_device *pdev, pm_message_t mesg)
{
    return 0;
}

static int mtkfb_vsync_resume(struct platform_device *pdev)
{
    return 0;
}

static struct platform_driver mtkfb_vsync_driver = {
    .probe		= mtkfb_vsync_probe,
    .remove		= mtkfb_vsync_remove,
    .shutdown	= mtkfb_vsync_shutdown,
    .suspend	= mtkfb_vsync_suspend,
    .resume		= mtkfb_vsync_resume,
    .driver     = {
        .name = MTKFB_VSYNC_DEVNAME,
    },
};

static void mtkfb_vsync_device_release(struct device *dev)
{
}

static u64 mtkfb_vsync_dmamask = ~(u32)0;

static struct platform_device mtkfb_vsync_device = {
    .name	 = MTKFB_VSYNC_DEVNAME,
    .id      = 0,
    .dev     = {
        .release = mtkfb_vsync_device_release,
        .dma_mask = &mtkfb_vsync_dmamask,
        .coherent_dma_mask = 0xffffffff,
    },
    .num_resources = 0,
};

static int __init mtkfb_vsync_init(void)
{
    VSYNC_INF("initializeing driver...\n");
	
    if (platform_device_register(&mtkfb_vsync_device))
    {
        VSYNC_ERR("failed to register device\n");
        return -ENODEV;
    }

    if (platform_driver_register(&mtkfb_vsync_driver))
    {
        VSYNC_ERR("failed to register driver\n");
        platform_device_unregister(&mtkfb_vsync_device);
        return -ENODEV;
    }

    return 0;
}

static void __exit mtkfb_vsync_exit(void)
{
    cdev_del(mtkfb_vsync_cdev);
    unregister_chrdev_region(mtkfb_vsync_devno, 1);

    platform_driver_unregister(&mtkfb_vsync_driver);
    platform_device_unregister(&mtkfb_vsync_device);
	
    device_destroy(mtkfb_vsync_class, mtkfb_vsync_devno);
    class_destroy(mtkfb_vsync_class);
	
    VSYNC_INF("exit driver...\n");
}

module_init(mtkfb_vsync_init);
module_exit(mtkfb_vsync_exit);

MODULE_DESCRIPTION("MediaTek FB VSYNC Driver");
MODULE_AUTHOR("Zaikuo Wang <Zaikuo.Wang@mediatek.com>");
