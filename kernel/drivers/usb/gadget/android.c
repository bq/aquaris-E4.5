/*
 * Gadget Driver for Android
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
 *         Benoit Goby <benoit@android.com>
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/utsname.h>
#include <linux/platform_device.h>
#include <linux/xlog.h>

#ifdef CKT_MAKE_ADB_DEVICES_NAME_TO_RANDOM
#include <linux/time.h> //ckt helin 
#endif//CKT_MAKE_ADB_DEVICES_NAME_TO_RANDOM

#include <linux/usb/ch9.h>
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>
/* Add for HW/SW connect */
#include <linux/musb/mtk_musb.h>
/* Add for HW/SW connect */
#include "gadget_chips.h"
#include "logger.h"
/*
 * Kbuild is not very cooperative with respect to linking separately
 * compiled library objects into one module.  So for now we won't use
 * separate compilation ... ensuring init/exit sections work to shrink
 * the runtime footprint, and giving us at least some parts of what
 * a "gcc --combine ... part1.c part2.c part3.c ... " build would.
 */
#include "usbstring.c"
#include "config.c"
#include "epautoconf.c"
#include "composite.c"

#include "f_fs.c"
#include "f_audio_source.c"
#include "f_mass_storage.c"
#include "u_serial.c"
#include "f_serial.c"
#include "f_acm.c"
#include "f_adb.c"
#include "f_mtp.c"
#include "f_accessory.c"
#define USB_ETH_RNDIS y
#include "f_rndis.c"
#include "rndis.c"
#include "f_ecm.c"
#include "f_eem.c"
#include "u_ether.c"
#ifdef EVDO_DT_VIA_SUPPORT
#include <mach/viatel_rawbulk.h>
int rawbulk_bind_config(struct usb_configuration *c, int transfer_id);
int rawbulk_function_setup(struct usb_function *f, const struct usb_ctrlrequest *ctrl);
#endif

MODULE_AUTHOR("Mike Lockwood");
MODULE_DESCRIPTION("Android Composite USB Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");


#define SERIALNO_LEN 32
extern char serial_number[SERIALNO_LEN];
static const char longname[] = "Gadget Android";

/* Default vendor and product IDs, overridden by userspace */
#define VENDOR_ID		0x0BB4
#define PRODUCT_ID		0x0001

/* Default manufacturer and product string , overridden by userspace */
//#define MANUFACTURER_STRING "MediaTek"
//#define PRODUCT_STRING "MT65xx Android Phone"

#define MANUFACTURER_STRING USB_MANUFACTURER_STRING

#if defined(VEGETAHD)
  #define PRODUCT_STRING "Aquaris_E5_HD"
#else 
  #define PRODUCT_STRING "Aquaris_E4.5"
#endif

#define USB_LOG "USB"

#ifdef CKT_MAKE_ADB_DEVICES_NAME_TO_RANDOM
static char* g_p_adb_device=NULL;//ckt helin 
#define ADB_DEVICES_MAX_LENGTH	256
char g_ts[20]={'\0'};
long g_sec=0;
long g_usec=0;

extern u32 get_devinfo_with_index(u32 index);

#endif//CKT_MAKE_ADB_DEVICES_NAME_TO_RANDOM


struct android_usb_function {
	char *name;
	void *config;

	struct device *dev;
	char *dev_name;
	struct device_attribute **attributes;

	/* for android_dev.enabled_functions */
	struct list_head enabled_list;

	/* Optional: initialization during gadget bind */
	int (*init)(struct android_usb_function *, struct usb_composite_dev *);
	/* Optional: cleanup during gadget unbind */
	void (*cleanup)(struct android_usb_function *);
	/* Optional: called when the function is added the list of
	 *		enabled functions */
	void (*enable)(struct android_usb_function *);
	/* Optional: called when it is removed */
	void (*disable)(struct android_usb_function *);

	int (*bind_config)(struct android_usb_function *,
			   struct usb_configuration *);

	/* Optional: called when the configuration is removed */
	void (*unbind_config)(struct android_usb_function *,
			      struct usb_configuration *);
	/* Optional: handle ctrl requests before the device is configured */
	int (*ctrlrequest)(struct android_usb_function *,
					struct usb_composite_dev *,
					const struct usb_ctrlrequest *);
};

struct android_dev {
	struct android_usb_function **functions;
	struct list_head enabled_functions;
	struct usb_composite_dev *cdev;
	struct device *dev;

	bool enabled;
	int disable_depth;
	struct mutex mutex;
	bool connected;
	bool sw_connected;
	struct work_struct work;
	char ffs_aliases[256];
	int rezero_cmd;
};

static struct class *android_class;
static struct android_dev *_android_dev;
static int android_bind_config(struct usb_configuration *c);
static void android_unbind_config(struct usb_configuration *c);

/* string IDs are assigned dynamically */
#define STRING_MANUFACTURER_IDX		0
#define STRING_PRODUCT_IDX		1
#define STRING_SERIAL_IDX		2

static char manufacturer_string[256];
static char product_string[256];
static char serial_string[256];

/* String Table */
static struct usb_string strings_dev[] = {
	[STRING_MANUFACTURER_IDX].s = manufacturer_string,
	[STRING_PRODUCT_IDX].s = product_string,
	[STRING_SERIAL_IDX].s = USB_STRING_SERIAL_IDX,
	{  }			/* end of list */
};

static struct usb_gadget_strings stringtab_dev = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings_dev,
};

static struct usb_gadget_strings *dev_strings[] = {
	&stringtab_dev,
	NULL,
};

static struct usb_device_descriptor device_desc = {
	.bLength              = sizeof(device_desc),
	.bDescriptorType      = USB_DT_DEVICE,
	.bcdUSB               = __constant_cpu_to_le16(0x0200),
	.bDeviceClass         = USB_CLASS_PER_INTERFACE,
	.idVendor             = __constant_cpu_to_le16(VENDOR_ID),
	.idProduct            = __constant_cpu_to_le16(PRODUCT_ID),
	.bcdDevice            = __constant_cpu_to_le16(0xffff),
	.bNumConfigurations   = 1,
};

static struct usb_configuration android_config_driver = {
	.label		= "android",
	.unbind		= android_unbind_config,
	.bConfigurationValue = 1,
	.bmAttributes	= USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
	.bMaxPower	= 0xFA, /* 500ma */
};

static void android_work(struct work_struct *data)
{
	struct android_dev *dev = container_of(data, struct android_dev, work);
	struct usb_composite_dev *cdev = dev->cdev;
	char *disconnected[2] = { "USB_STATE=DISCONNECTED", NULL };
	char *connected[2]    = { "USB_STATE=CONNECTED", NULL };
	char *configured[2]   = { "USB_STATE=CONFIGURED", NULL };

	/* Add for HW/SW connect */
	char *hwdisconnected[2] = { "USB_STATE=HWDISCONNECTED", NULL };
	char *hwconnected[2]    = { "USB_STATE=HWCONNECTED", NULL };
	/* Add for HW/SW connect */

	char *rezero_event[2] = { "USB_STATE=REZEROCMD", NULL };
	char *showcdrom_event[2] = { "USB_STATE=SHOWCDROMCMD", NULL };

	char **uevent_envp = NULL;
	char **uevent_envp_cdrom = NULL;
	unsigned long flags;
	/* Add for HW/SW connect */
	bool is_hwconnected = true;

	/* patch for ALPS00345130, if the disconnect followed by hw_disconnect, then the hw_disconnect
	will not notify the UsbDeviceManager due to that musb->g.speed == USB_SPEED_UNKNOWN*/
	if (!cdev){
		return ;
	}

	if(usb_cable_connected())
			is_hwconnected = true;
	else
			is_hwconnected = false;

	xlog_printk(ANDROID_LOG_INFO, USB_LOG, "%s: is_hwconnected=%d \n", __func__, is_hwconnected);
	/* Add for HW/SW connect */

	spin_lock_irqsave(&cdev->lock, flags);
	if (cdev->config) {
		uevent_envp = configured;
	} else if (dev->connected != dev->sw_connected) {
		uevent_envp = dev->connected ? connected : disconnected;
	}

	dev->sw_connected = dev->connected;

	if (dev->rezero_cmd == 1) {
		uevent_envp_cdrom = rezero_event;
		dev->rezero_cmd = 0;
	} else if (dev->rezero_cmd == 2) {
		uevent_envp_cdrom = showcdrom_event;
		dev->rezero_cmd = 0;
	}

	spin_unlock_irqrestore(&cdev->lock, flags);

	if (uevent_envp) {
		kobject_uevent_env(&dev->dev->kobj, KOBJ_CHANGE, uevent_envp);
		xlog_printk(ANDROID_LOG_INFO, USB_LOG, "%s: sent uevent %s\n", __func__, uevent_envp[0]);
	}

	if (!is_hwconnected) {
		kobject_uevent_env(&dev->dev->kobj, KOBJ_CHANGE, hwdisconnected);
		xlog_printk(ANDROID_LOG_INFO, USB_LOG, "%s: sent uevent %s\n", __func__, hwdisconnected[0]);
	}

	if (uevent_envp_cdrom) {
		kobject_uevent_env(&dev->dev->kobj, KOBJ_CHANGE, uevent_envp_cdrom);
		xlog_printk(ANDROID_LOG_INFO, USB_LOG, "%s: sent uevent %s\n", __func__, uevent_envp_cdrom[0]);
	}

}

static void android_enable(struct android_dev *dev)
{
	struct usb_composite_dev *cdev = dev->cdev;

	if (WARN_ON(!dev->disable_depth))
		return;

	if (--dev->disable_depth == 0) {
		usb_add_config(cdev, &android_config_driver,
					android_bind_config);
	usb_gadget_connect(cdev->gadget);
	}
}

static void android_disable(struct android_dev *dev)
{
	struct usb_composite_dev *cdev = dev->cdev;

	if (dev->disable_depth++ == 0) {
		usb_gadget_disconnect(cdev->gadget);
		/* Cancel pending control requests */
		usb_ep_dequeue(cdev->gadget->ep0, cdev->req);
		usb_remove_config(cdev, &android_config_driver);
	}
}

/*-------------------------------------------------------------------------*/
/* Supported functions initialization */

struct functionfs_config {
	bool opened;
	bool enabled;
	struct ffs_data *data;
};

static int ffs_function_init(struct android_usb_function *f,
			     struct usb_composite_dev *cdev)
{
	f->config = kzalloc(sizeof(struct functionfs_config), GFP_KERNEL);
	if (!f->config)
		return -ENOMEM;

	return functionfs_init();
}

static void ffs_function_cleanup(struct android_usb_function *f)
{
	functionfs_cleanup();
	kfree(f->config);
}

static void ffs_function_enable(struct android_usb_function *f)
{
	struct android_dev *dev = _android_dev;
	struct functionfs_config *config = f->config;

	config->enabled = true;

	/* Disable the gadget until the function is ready */
	if (!config->opened)
		android_disable(dev);
}

static void ffs_function_disable(struct android_usb_function *f)
{
	struct android_dev *dev = _android_dev;
	struct functionfs_config *config = f->config;

	config->enabled = false;

	/* Balance the disable that was called in closed_callback */
	if (!config->opened)
		android_enable(dev);
}

static int ffs_function_bind_config(struct android_usb_function *f,
				    struct usb_configuration *c)
{
	struct functionfs_config *config = f->config;
	return functionfs_bind_config(c->cdev, c, config->data);
}

static ssize_t
ffs_aliases_show(struct device *pdev, struct device_attribute *attr, char *buf)
{
	struct android_dev *dev = _android_dev;
	int ret;

	mutex_lock(&dev->mutex);
	ret = sprintf(buf, "%s\n", dev->ffs_aliases);
	mutex_unlock(&dev->mutex);

	return ret;
}

static ssize_t
ffs_aliases_store(struct device *pdev, struct device_attribute *attr,
					const char *buf, size_t size)
{
	struct android_dev *dev = _android_dev;
	char buff[256];

	mutex_lock(&dev->mutex);

	if (dev->enabled) {
		mutex_unlock(&dev->mutex);
		return -EBUSY;
	}

	strlcpy(buff, buf, sizeof(buff));
	strlcpy(dev->ffs_aliases, strim(buff), sizeof(dev->ffs_aliases));

	mutex_unlock(&dev->mutex);

	return size;
}

static DEVICE_ATTR(aliases, S_IRUGO | S_IWUSR, ffs_aliases_show,
					       ffs_aliases_store);
static struct device_attribute *ffs_function_attributes[] = {
	&dev_attr_aliases,
	NULL
};

static struct android_usb_function ffs_function = {
	.name		= "ffs",
	.init		= ffs_function_init,
	.enable		= ffs_function_enable,
	.disable	= ffs_function_disable,
	.cleanup	= ffs_function_cleanup,
	.bind_config	= ffs_function_bind_config,
	.attributes	= ffs_function_attributes,
};

static int functionfs_ready_callback(struct ffs_data *ffs)
{
	struct android_dev *dev = _android_dev;
	struct functionfs_config *config = ffs_function.config;
	int ret = 0;

	mutex_lock(&dev->mutex);

	ret = functionfs_bind(ffs, dev->cdev);
	if (ret)
		goto err;

	config->data = ffs;
	config->opened = true;

	if (config->enabled)
		android_enable(dev);

err:
	mutex_unlock(&dev->mutex);
	return ret;
}

static void functionfs_closed_callback(struct ffs_data *ffs)
{
	struct android_dev *dev = _android_dev;
	struct functionfs_config *config = ffs_function.config;

	mutex_lock(&dev->mutex);

	if (config->enabled)
		android_disable(dev);

	config->opened = false;
	config->data = NULL;

	functionfs_unbind(ffs);

	mutex_unlock(&dev->mutex);
}

static int functionfs_check_dev_callback(const char *dev_name)
{
	return 0;
}


struct adb_data {
	bool opened;
	bool enabled;
};

static int
adb_function_init(struct android_usb_function *f,
		struct usb_composite_dev *cdev)
{
	f->config = kzalloc(sizeof(struct adb_data), GFP_KERNEL);
	if (!f->config)
		return -ENOMEM;

	return adb_setup();
}

static void adb_function_cleanup(struct android_usb_function *f)
{
	adb_cleanup();
	kfree(f->config);
}

static int
adb_function_bind_config(struct android_usb_function *f,
		struct usb_configuration *c)
{
	return adb_bind_config(c);
}

static void adb_android_function_enable(struct android_usb_function *f)
{
/* This patch cause WHQL fail */
#if 0
	struct android_dev *dev = _android_dev;
	struct adb_data *data = f->config;

	data->enabled = true;

	/* Disable the gadget until adbd is ready */
	if (!data->opened)
		android_disable(dev);
#endif
}

static void adb_android_function_disable(struct android_usb_function *f)
{
/* This patch cause WHQL fail */
#if 0
	struct android_dev *dev = _android_dev;
	struct adb_data *data = f->config;

	data->enabled = false;

	/* Balance the disable that was called in closed_callback */
	if (!data->opened)
		android_enable(dev);
#endif
}

static struct android_usb_function adb_function = {
	.name		= "adb",
	.enable		= adb_android_function_enable,
	.disable	= adb_android_function_disable,
	.init		= adb_function_init,
	.cleanup	= adb_function_cleanup,
	.bind_config	= adb_function_bind_config,
};

static void adb_ready_callback(void)
{
/* This patch cause WHQL fail */
#if 0
	struct android_dev *dev = _android_dev;
	struct adb_data *data = adb_function.config;

	mutex_lock(&dev->mutex);

	data->opened = true;

	if (data->enabled)
		android_enable(dev);

	mutex_unlock(&dev->mutex);
#endif
}

static void adb_closed_callback(void)
{
/* This patch cause WHQL fail */
#if 0
	struct android_dev *dev = _android_dev;
	struct adb_data *data = adb_function.config;

	mutex_lock(&dev->mutex);

	data->opened = false;

	if (data->enabled)
		android_disable(dev);

	mutex_unlock(&dev->mutex);
#endif
}

#define MAX_SERIAL_PORTS 4
static int serial_initialized = 0;

struct acm_function_config {
	int instances;
	int port_index[4];
};

static int
acm_function_init(struct android_usb_function *f,
		struct usb_composite_dev *cdev)
{
	f->config = kzalloc(sizeof(struct acm_function_config), GFP_KERNEL);
	if (!f->config)
		return -ENOMEM;

	if (!serial_initialized) {
		serial_initialized = 1;
		return gserial_setup(cdev->gadget, MAX_SERIAL_PORTS);
	} else {
		return 0;
	}
}

static void acm_function_cleanup(struct android_usb_function *f)
{
	if (serial_initialized) {
	gserial_cleanup();
		serial_initialized = 0;
	}
	kfree(f->config);
	f->config = NULL;
}

static int
acm_function_bind_config(struct android_usb_function *f,
		struct usb_configuration *c)
{
	int i;
	int ret = 0;
	struct acm_function_config *config = f->config;

	/*1st:Modem, 2nd:Modem, 3rd:BT, 4th:MD logger*/
	for (i = 0; i < MAX_SERIAL_PORTS; i++) {
		if(config->port_index[i] != 0) {
			ret = acm_bind_config(c, config->port_index[i]-1);
			if (ret) {
				pr_err("Could not bind acm%u config\n", i);
				break;
			}
			pr_info("%s Open /dev/ttyGS%d\n", __func__, i);
			config->port_index[i] = 0;
			config->instances = 0;
		}
	}

	for (i = 0; i < config->instances; i++) {
		ret = acm_bind_config(c, i);
		if (ret) {
			pr_err("Could not bind acm%u config\n", i);
			break;
		}
	}

	return ret;
}

static ssize_t acm_instances_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct acm_function_config *config = f->config;
	return sprintf(buf, "%d\n", config->instances);
}

static ssize_t acm_instances_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct acm_function_config *config = f->config;
	int value;

	sscanf(buf, "%d", &value);
	if (value > MAX_SERIAL_PORTS)
		value = MAX_SERIAL_PORTS;
	config->instances = value;
	return size;
}

static DEVICE_ATTR(instances, S_IRUGO | S_IWUSR, acm_instances_show,
						 acm_instances_store);

static ssize_t acm_port_index_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct acm_function_config *config = f->config;
	return sprintf(buf, "%d,%d,%d,%d\n", config->port_index[0], config->port_index[1],
		config->port_index[2], config->port_index[3]);
}

static ssize_t acm_port_index_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct acm_function_config *config = f->config;
	int val[4]={0};
	int num = 0;
	int tmp = 0;

	num = sscanf(buf, "%d,%d,%d,%d", &(val[0]), &(val[1]), &(val[2]), &(val[3]));

	pr_info("%s [0]=%d,[1]=%d,[2]=%d,[3]=%d, num=%d\n", __func__, val[0], val[1], \
								val[2], val[3], num);

	config->port_index[0] = 0;
	config->port_index[1] = 0;
	config->port_index[2] = 0;
	config->port_index[3] = 0;

	for(tmp = 0; tmp < num; tmp++) {
		config->port_index[tmp] = (val[tmp] > MAX_SERIAL_PORTS) ? 0 : val[tmp];
	}

	return size;
}

static DEVICE_ATTR(port_index, S_IRUGO | S_IWUSR, acm_port_index_show,
						 acm_port_index_store);

static struct device_attribute *acm_function_attributes[] = {
	&dev_attr_instances,
	&dev_attr_port_index, /*Only open the specific port*/
	NULL
};

static struct android_usb_function acm_function = {
	.name		= "acm",
	.init		= acm_function_init,
	.cleanup	= acm_function_cleanup,
	.bind_config	= acm_function_bind_config,
	.attributes	= acm_function_attributes,
};

struct serial_function_config {
	int port_num;
};

static int serial_function_init(struct android_usb_function *f, struct usb_composite_dev *cdev)
{
	f->config = kzalloc(sizeof(struct serial_function_config), GFP_KERNEL);
	if (!f->config)
		return -ENOMEM;

	if (!serial_initialized) {
		serial_initialized = 1;
		return gserial_setup(cdev->gadget, MAX_SERIAL_PORTS);
	} else {
		return 0;
	}
}

static void serial_function_cleanup(struct android_usb_function *f)
{
	if (serial_initialized) {
		gserial_cleanup();
		serial_initialized = 0;
	}
	kfree(f->config);
	f->config = NULL;
}

static int
serial_function_bind_config(struct android_usb_function *f, struct usb_configuration *c)
{
	int ret = 0;
	struct serial_function_config *config = f->config;

	/* /dev/ttyGS0 reserved for ACM */
	if (!config->port_num) {
		config->port_num = 1;
		pr_debug("change serial port num to 1");
	}

	ret = gser_bind_config(c, config->port_num);
	if (ret) {
		pr_err("Could not bind serial%u config\n", config->port_num);
	}

	return ret;
}

static ssize_t serial_port_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct serial_function_config *config = f->config;
	return sprintf(buf, "%d\n", config->port_num);
}

static ssize_t serial_port_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct serial_function_config *config = f->config;
	int value;

	sscanf(buf, "%d", &value);
	if (value > MAX_SERIAL_PORTS)
		value = MAX_SERIAL_PORTS;
	config->port_num = value;
	return size;
}

static DEVICE_ATTR(port, S_IRUGO | S_IWUSR, serial_port_show, serial_port_store);
static struct device_attribute *serial_function_attributes[] = { &dev_attr_port, NULL };

static struct android_usb_function serial_function = {
	.name		= "gser",
	.init		= serial_function_init,
	.cleanup	= serial_function_cleanup,
	.bind_config	= serial_function_bind_config,
	.attributes	= serial_function_attributes,
};

static int
mtp_function_init(struct android_usb_function *f,
		struct usb_composite_dev *cdev)
{
	return mtp_setup();
}

static void mtp_function_cleanup(struct android_usb_function *f)
{
	mtp_cleanup();
}

static int
mtp_function_bind_config(struct android_usb_function *f,
		struct usb_configuration *c)
{
	return mtp_bind_config(c, false);
}

static int
ptp_function_init(struct android_usb_function *f,
		struct usb_composite_dev *cdev)
{
	/* nothing to do - initialization is handled by mtp_function_init */
	return 0;
}

static void ptp_function_cleanup(struct android_usb_function *f)
{
	/* nothing to do - cleanup is handled by mtp_function_cleanup */
}

static int
ptp_function_bind_config(struct android_usb_function *f,
		struct usb_configuration *c)
{
	return mtp_bind_config(c, true);
}

static int mtp_function_ctrlrequest(struct android_usb_function *f,
					struct usb_composite_dev *cdev,
					const struct usb_ctrlrequest *c)
{
	//Added Modification for ALPS00272887, MTP MSFT OS Descriptor
	struct android_dev		*dev = _android_dev;
	struct android_usb_function	*f_count;
	int	   functions_no=0;
	char   usb_function_string[32];
	char * buff = usb_function_string;

	list_for_each_entry(f_count, &dev->enabled_functions, enabled_list)
	{
		functions_no++;
		buff += sprintf(buff, "%s,", f_count->name);
	}
	*(buff-1) = '\n';

	mtp_read_usb_functions(functions_no, usb_function_string);
	//Added Modification for ALPS00272887, MTP MSFT OS Descriptor
	return mtp_ctrlrequest(cdev, c);
}

static struct android_usb_function mtp_function = {
	.name		= "mtp",
	.init		= mtp_function_init,
	.cleanup	= mtp_function_cleanup,
	.bind_config	= mtp_function_bind_config,
	.ctrlrequest	= mtp_function_ctrlrequest,
};

/* PTP function is same as MTP with slightly different interface descriptor */
static struct android_usb_function ptp_function = {
	.name		= "ptp",
	.init		= ptp_function_init,
	.cleanup	= ptp_function_cleanup,
	.bind_config	= ptp_function_bind_config,
};

struct ecm_function_config {
	u8      ethaddr[ETH_ALEN];
};

static int ecm_function_init(struct android_usb_function *f, struct usb_composite_dev *cdev)
{
	struct ecm_function_config *ecm;
	f->config = kzalloc(sizeof(struct ecm_function_config), GFP_KERNEL);
	if (!f->config)
		return -ENOMEM;

	ecm = f->config;
	return 0;
}

static void ecm_function_cleanup(struct android_usb_function *f)
{
	kfree(f->config);
	f->config = NULL;
}

static int
ecm_function_bind_config(struct android_usb_function *f,
					struct usb_configuration *c)
{
	int ret;
	struct ecm_function_config *ecm = f->config;

	if (!ecm) {
		pr_err("%s: ecm_pdata\n", __func__);
		return -1;
	}


	pr_info("%s MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", __func__,
		ecm->ethaddr[0], ecm->ethaddr[1], ecm->ethaddr[2],
		ecm->ethaddr[3], ecm->ethaddr[4], ecm->ethaddr[5]);

	ret = gether_setup_name(c->cdev->gadget, ecm->ethaddr, "rndis");
	if (ret) {
		pr_err("%s: gether_setup failed\n", __func__);
		return ret;
	}


	return ecm_bind_config(c, ecm->ethaddr);
}

static void ecm_function_unbind_config(struct android_usb_function *f,
						struct usb_configuration *c)
{
	gether_cleanup();
}

static ssize_t ecm_ethaddr_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct ecm_function_config *ecm = f->config;
	return sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x\n",
		ecm->ethaddr[0], ecm->ethaddr[1], ecm->ethaddr[2],
		ecm->ethaddr[3], ecm->ethaddr[4], ecm->ethaddr[5]);
}

static ssize_t ecm_ethaddr_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct ecm_function_config *ecm = f->config;

	if (sscanf(buf, "%02x:%02x:%02x:%02x:%02x:%02x\n",
		    (int *)&ecm->ethaddr[0], (int *)&ecm->ethaddr[1],
		    (int *)&ecm->ethaddr[2], (int *)&ecm->ethaddr[3],
		    (int *)&ecm->ethaddr[4], (int *)&ecm->ethaddr[5]) == 6)
		return size;
	return -EINVAL;
}

static DEVICE_ATTR(ecm_ethaddr, S_IRUGO | S_IWUSR, ecm_ethaddr_show,
					       ecm_ethaddr_store);

static struct device_attribute *ecm_function_attributes[] = {
	&dev_attr_ecm_ethaddr,
	NULL
};

static struct android_usb_function ecm_function = {
	.name		= "ecm",
	.init		= ecm_function_init,
	.cleanup	= ecm_function_cleanup,
	.bind_config	= ecm_function_bind_config,
	.unbind_config	= ecm_function_unbind_config,
	.attributes	= ecm_function_attributes,
};

struct eem_function_config {
	u8      ethaddr[ETH_ALEN];
	char	manufacturer[256];
};

static int
eem_function_init(struct android_usb_function *f,
		struct usb_composite_dev *cdev)
{
	f->config = kzalloc(sizeof(struct eem_function_config), GFP_KERNEL);
	if (!f->config)
		return -ENOMEM;
	return 0;
}

static void eem_function_cleanup(struct android_usb_function *f)
{
	kfree(f->config);
	f->config = NULL;
}

static int
eem_function_bind_config(struct android_usb_function *f,
		struct usb_configuration *c)
{
	int ret;
	struct eem_function_config *eem = f->config;

	xlog_printk(ANDROID_LOG_DEBUG, USB_LOG, "%s: \n", __func__);

	if (!eem) {
		pr_err("%s: rndis_pdata\n", __func__);
		return -1;
	}

	pr_info("%s MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", __func__,
		eem->ethaddr[0], eem->ethaddr[1], eem->ethaddr[2],
		eem->ethaddr[3], eem->ethaddr[4], eem->ethaddr[5]);

	//ret = gether_setup_name(c->cdev->gadget, rndis->ethaddr, "eem");
	// emulate as rndis interface, this can help network framework to integrate without changing the binding interface name
	ret = gether_setup_name(c->cdev->gadget, eem->ethaddr, "rndis");
	if (ret) {
		pr_err("%s: gether_setup failed\n", __func__);
		return ret;
	}

	return eem_bind_config(c);
}

static void eem_function_unbind_config(struct android_usb_function *f,
						struct usb_configuration *c)
{
	gether_cleanup();
}

static ssize_t eem_ethaddr_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct eem_function_config *eem = f->config;
	return sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x\n",
		eem->ethaddr[0], eem->ethaddr[1], eem->ethaddr[2],
		eem->ethaddr[3], eem->ethaddr[4], eem->ethaddr[5]);
}

static ssize_t eem_ethaddr_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct eem_function_config *eem = f->config;

	if (sscanf(buf, "%02x:%02x:%02x:%02x:%02x:%02x\n",
		    (int *)&eem->ethaddr[0], (int *)&eem->ethaddr[1],
		    (int *)&eem->ethaddr[2], (int *)&eem->ethaddr[3],
		    (int *)&eem->ethaddr[4], (int *)&eem->ethaddr[5]) == 6)
		return size;
	return -EINVAL;
}

static DEVICE_ATTR(eem_ethaddr, S_IRUGO | S_IWUSR, eem_ethaddr_show,
					       eem_ethaddr_store);

static struct device_attribute *eem_function_attributes[] = {
	&dev_attr_eem_ethaddr,
	NULL
};

static struct android_usb_function eem_function = {
	.name		= "eem",
	.init		= eem_function_init,
	.cleanup	= eem_function_cleanup,
	.bind_config	= eem_function_bind_config,
	.unbind_config	= eem_function_unbind_config,
	.attributes	= eem_function_attributes,
};

struct rndis_function_config {
	u8      ethaddr[ETH_ALEN];
	u32     vendorID;
	char	manufacturer[256];
	/* "Wireless" RNDIS; auto-detected by Windows */
	bool	wceis;
};

static int
rndis_function_init(struct android_usb_function *f,
		struct usb_composite_dev *cdev)
{
	f->config = kzalloc(sizeof(struct rndis_function_config), GFP_KERNEL);
	if (!f->config)
		return -ENOMEM;
	return 0;
}

static void rndis_function_cleanup(struct android_usb_function *f)
{
	kfree(f->config);
	f->config = NULL;
}

static int
rndis_function_bind_config(struct android_usb_function *f,
		struct usb_configuration *c)
{
	int ret;
	struct rndis_function_config *rndis = f->config;

	xlog_printk(ANDROID_LOG_DEBUG, USB_LOG, "%s: \n", __func__);

	if (!rndis) {
		pr_err("%s: rndis_pdata\n", __func__);
		return -1;
	}

	pr_info("%s MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", __func__,
		rndis->ethaddr[0], rndis->ethaddr[1], rndis->ethaddr[2],
		rndis->ethaddr[3], rndis->ethaddr[4], rndis->ethaddr[5]);

	ret = gether_setup_name(c->cdev->gadget, rndis->ethaddr, "rndis");
	if (ret) {
		pr_err("%s: gether_setup failed\n", __func__);
		return ret;
	}

	if (rndis->wceis) {
		/* "Wireless" RNDIS; auto-detected by Windows */
		rndis_iad_descriptor.bFunctionClass =
						USB_CLASS_WIRELESS_CONTROLLER;
		rndis_iad_descriptor.bFunctionSubClass = 0x01;
		rndis_iad_descriptor.bFunctionProtocol = 0x03;
		rndis_control_intf.bInterfaceClass =
						USB_CLASS_WIRELESS_CONTROLLER;
		rndis_control_intf.bInterfaceSubClass =	 0x01;
		rndis_control_intf.bInterfaceProtocol =	 0x03;
	}

	return rndis_bind_config_vendor(c, rndis->ethaddr, rndis->vendorID,
					   rndis->manufacturer);
}

static void rndis_function_unbind_config(struct android_usb_function *f,
						struct usb_configuration *c)
{
	gether_cleanup();
}

static ssize_t rndis_manufacturer_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *config = f->config;
	return sprintf(buf, "%s\n", config->manufacturer);
}

static ssize_t rndis_manufacturer_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *config = f->config;

	if (size >= sizeof(config->manufacturer))
		return -EINVAL;
	if (sscanf(buf, "%s", config->manufacturer) == 1)
		return size;
	return -1;
}

static DEVICE_ATTR(manufacturer, S_IRUGO | S_IWUSR, rndis_manufacturer_show,
						    rndis_manufacturer_store);

static ssize_t rndis_wceis_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *config = f->config;
	return sprintf(buf, "%d\n", config->wceis);
}

static ssize_t rndis_wceis_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *config = f->config;
	int value;

	if (sscanf(buf, "%d", &value) == 1) {
		config->wceis = value;
		return size;
	}
	return -EINVAL;
}

static DEVICE_ATTR(wceis, S_IRUGO | S_IWUSR, rndis_wceis_show,
					     rndis_wceis_store);

static ssize_t rndis_ethaddr_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *rndis = f->config;
	return sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x\n",
		rndis->ethaddr[0], rndis->ethaddr[1], rndis->ethaddr[2],
		rndis->ethaddr[3], rndis->ethaddr[4], rndis->ethaddr[5]);
}

static ssize_t rndis_ethaddr_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *rndis = f->config;

	if (sscanf(buf, "%02x:%02x:%02x:%02x:%02x:%02x\n",
		    (int *)&rndis->ethaddr[0], (int *)&rndis->ethaddr[1],
		    (int *)&rndis->ethaddr[2], (int *)&rndis->ethaddr[3],
		    (int *)&rndis->ethaddr[4], (int *)&rndis->ethaddr[5]) == 6)
		return size;
	return -EINVAL;
}

static DEVICE_ATTR(ethaddr, S_IRUGO | S_IWUSR, rndis_ethaddr_show,
					       rndis_ethaddr_store);

static ssize_t rndis_vendorID_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *config = f->config;
	return sprintf(buf, "%04x\n", config->vendorID);
}

static ssize_t rndis_vendorID_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *config = f->config;
	int value;

	if (sscanf(buf, "%04x", &value) == 1) {
		config->vendorID = value;
		return size;
	}
	return -EINVAL;
}

static DEVICE_ATTR(vendorID, S_IRUGO | S_IWUSR, rndis_vendorID_show,
						rndis_vendorID_store);

static struct device_attribute *rndis_function_attributes[] = {
	&dev_attr_manufacturer,
	&dev_attr_wceis,
	&dev_attr_ethaddr,
	&dev_attr_vendorID,
	NULL
};

static struct android_usb_function rndis_function = {
	.name		= "rndis",
	.init		= rndis_function_init,
	.cleanup	= rndis_function_cleanup,
	.bind_config	= rndis_function_bind_config,
	.unbind_config	= rndis_function_unbind_config,
	.attributes	= rndis_function_attributes,
};

void mass_storage_callback(unsigned char cmd_type)
{
	struct android_dev *dev = _android_dev;
	if (cmd_type != 0)
		dev->rezero_cmd = cmd_type;
	schedule_work(&dev->work);
}

struct mass_storage_function_config {
	struct fsg_config fsg;
	struct fsg_common *common;
};

static int mass_storage_function_init(struct android_usb_function *f,
					struct usb_composite_dev *cdev)
{
	struct mass_storage_function_config *config;
	struct fsg_common *common;
	int err;
	int i;

	config = kzalloc(sizeof(struct mass_storage_function_config),
								GFP_KERNEL);
	if (!config)
		return -ENOMEM;

#ifdef MTK_MULTI_STORAGE_SUPPORT
#ifdef MTK_SHARED_SDCARD
#define NLUN_STORAGE 1
#else
#define NLUN_STORAGE 2
#endif
#else
#define NLUN_STORAGE 1
#endif

	config->fsg.nluns = NLUN_STORAGE;

	for(i = 0; i < config->fsg.nluns; i++) {
		config->fsg.luns[i].removable = 1;
		config->fsg.luns[i].nofua = 1;
	}

	common = fsg_common_init(NULL, cdev, &config->fsg);
	if (IS_ERR(common)) {
		kfree(config);
		return PTR_ERR(common);
	}

	err = sysfs_create_link(&f->dev->kobj,
				&common->luns[0].dev.kobj,
				"lun");

	if (err) {
		kfree(config);
		return err;
	}

	/*
	 * "i" starts from "1", cuz dont want to change the naming of
	 * the original path of "lun0".
	 */
	for(i = 1; i < config->fsg.nluns; i++) {
		char string_lun[5]={0};

		sprintf(string_lun, "lun%d",i);

		err = sysfs_create_link(&f->dev->kobj,
				&common->luns[i].dev.kobj,
				string_lun);
		if (err) {
			kfree(config);
			return err;
		}
	}

	common->android_callback = &mass_storage_callback;

	config->common = common;
	f->config = config;
	return 0;
}

static void mass_storage_function_cleanup(struct android_usb_function *f)
{
	kfree(f->config);
	f->config = NULL;
}

static int mass_storage_function_bind_config(struct android_usb_function *f,
						struct usb_configuration *c)
{
	struct mass_storage_function_config *config = f->config;
	return fsg_bind_config(c->cdev, c, config->common);
}

static ssize_t mass_storage_inquiry_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct mass_storage_function_config *config = f->config;
	return sprintf(buf, "%s\n", config->common->inquiry_string);
}

static ssize_t mass_storage_inquiry_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct mass_storage_function_config *config = f->config;
	if (size >= sizeof(config->common->inquiry_string))
		return -EINVAL;
	if (sscanf(buf, "%s", config->common->inquiry_string) != 1)
		return -EINVAL;
	return size;
}

static DEVICE_ATTR(inquiry_string, S_IRUGO | S_IWUSR,
					mass_storage_inquiry_show,
					mass_storage_inquiry_store);

#ifdef MTK_BICR_SUPPORT

static ssize_t mass_storage_bicr_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct mass_storage_function_config *config = f->config;
	return sprintf(buf, "%d\n", config->common->bicr);
}

static ssize_t mass_storage_bicr_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct mass_storage_function_config *config = f->config;
	if (size >= sizeof(config->common->bicr))
		return -EINVAL;
	if (sscanf(buf, "%d", &config->common->bicr) != 1)
		return -EINVAL;

	/* Set Lun[0] is a CDROM when enable bicr.*/
	if (!strcmp(buf, "1"))
		config->common->luns[0].cdrom = 1;
	else {
		/*Reset the value. Clean the cdrom's parameters*/
		config->common->luns[0].cdrom = 0;
		config->common->luns[0].blkbits = 0;
		config->common->luns[0].blksize = 0;
		config->common->luns[0].num_sectors = 0;
	}

	return size;
}

static DEVICE_ATTR(bicr, S_IRUGO | S_IWUSR,
					mass_storage_bicr_show,
					mass_storage_bicr_store);

#endif

static struct device_attribute *mass_storage_function_attributes[] = {
	&dev_attr_inquiry_string,
#ifdef MTK_BICR_SUPPORT
	&dev_attr_bicr,
#endif
	NULL
};

static struct android_usb_function mass_storage_function = {
	.name		= "mass_storage",
	.init		= mass_storage_function_init,
	.cleanup	= mass_storage_function_cleanup,
	.bind_config	= mass_storage_function_bind_config,
	.attributes	= mass_storage_function_attributes,
};


static int accessory_function_init(struct android_usb_function *f,
					struct usb_composite_dev *cdev)
{
	return acc_setup();
}

static void accessory_function_cleanup(struct android_usb_function *f)
{
	acc_cleanup();
}

static int accessory_function_bind_config(struct android_usb_function *f,
						struct usb_configuration *c)
{
	return acc_bind_config(c);
}

static int accessory_function_ctrlrequest(struct android_usb_function *f,
						struct usb_composite_dev *cdev,
						const struct usb_ctrlrequest *c)
{
	return acc_ctrlrequest(cdev, c);
}

static struct android_usb_function accessory_function = {
	.name		= "accessory",
	.init		= accessory_function_init,
	.cleanup	= accessory_function_cleanup,
	.bind_config	= accessory_function_bind_config,
	.ctrlrequest	= accessory_function_ctrlrequest,
};

static int audio_source_function_init(struct android_usb_function *f,
			struct usb_composite_dev *cdev)
{
	struct audio_source_config *config;

	config = kzalloc(sizeof(struct audio_source_config), GFP_KERNEL);
	if (!config)
		return -ENOMEM;
	config->card = -1;
	config->device = -1;
	f->config = config;
	return 0;
}

static void audio_source_function_cleanup(struct android_usb_function *f)
{
	kfree(f->config);
}

static int audio_source_function_bind_config(struct android_usb_function *f,
						struct usb_configuration *c)
{
	struct audio_source_config *config = f->config;

	return audio_source_bind_config(c, config);
}

static void audio_source_function_unbind_config(struct android_usb_function *f,
						struct usb_configuration *c)
{
	struct audio_source_config *config = f->config;

	config->card = -1;
	config->device = -1;
}

static ssize_t audio_source_pcm_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct audio_source_config *config = f->config;

	/* print PCM card and device numbers */
	return sprintf(buf, "%d %d\n", config->card, config->device);
}

static DEVICE_ATTR(pcm, S_IRUGO | S_IWUSR, audio_source_pcm_show, NULL);

static struct device_attribute *audio_source_function_attributes[] = {
	&dev_attr_pcm,
	NULL
};

#ifdef EVDO_DT_VIA_SUPPORT
static int rawbulk_function_init(struct android_usb_function *f,
					struct usb_composite_dev *cdev)
{
	return 0;
}

static void rawbulk_function_cleanup(struct android_usb_function *f)
{
	;
}

static int rawbulk_function_bind_config(struct android_usb_function *f,
						struct usb_configuration *c)
{
    char *i = f->name + strlen("via_");
    if (!strncmp(i, "modem", 5))
        return rawbulk_bind_config(c, RAWBULK_TID_MODEM);
    else if (!strncmp(i, "ets", 3))
        return rawbulk_bind_config(c, RAWBULK_TID_ETS);
    else if (!strncmp(i, "atc", 3))
        return rawbulk_bind_config(c, RAWBULK_TID_AT);
    else if (!strncmp(i, "pcv", 3))
        return rawbulk_bind_config(c, RAWBULK_TID_PCV);
    else if (!strncmp(i, "gps", 3))
        return rawbulk_bind_config(c, RAWBULK_TID_GPS);
    return -EINVAL;
}

static int rawbulk_function_modem_ctrlrequest(struct android_usb_function *f,
						struct usb_composite_dev *cdev,
						const struct usb_ctrlrequest *c)
{
    if ((c->bRequestType & USB_RECIP_MASK) == USB_RECIP_DEVICE &&
            (c->bRequestType & USB_TYPE_MASK) == USB_TYPE_VENDOR) {
        struct rawbulk_function *fn = rawbulk_lookup_function(RAWBULK_TID_MODEM);
        return rawbulk_function_setup(&fn->function, c);
    }
    return -1;
}

static struct android_usb_function rawbulk_modem_function = {
	.name		= "via_modem",
	.init		= rawbulk_function_init,
	.cleanup	= rawbulk_function_cleanup,
	.bind_config	= rawbulk_function_bind_config,
	.ctrlrequest	= rawbulk_function_modem_ctrlrequest,
};

static struct android_usb_function rawbulk_ets_function = {
	.name		= "via_ets",
	.init		= rawbulk_function_init,
	.cleanup	= rawbulk_function_cleanup,
	.bind_config	= rawbulk_function_bind_config,
};

static struct android_usb_function rawbulk_atc_function = {
	.name		= "via_atc",
	.init		= rawbulk_function_init,
	.cleanup	= rawbulk_function_cleanup,
	.bind_config	= rawbulk_function_bind_config,
};

static struct android_usb_function rawbulk_pcv_function = {
	.name		= "via_pcv",
	.init		= rawbulk_function_init,
	.cleanup	= rawbulk_function_cleanup,
	.bind_config	= rawbulk_function_bind_config,
};

static struct android_usb_function rawbulk_gps_function = {
	.name		= "via_gps",
	.init		= rawbulk_function_init,
	.cleanup	= rawbulk_function_cleanup,
	.bind_config	= rawbulk_function_bind_config,
};
#endif
static struct android_usb_function audio_source_function = {
	.name		= "audio_source",
	.init		= audio_source_function_init,
	.cleanup	= audio_source_function_cleanup,
	.bind_config	= audio_source_function_bind_config,
	.unbind_config	= audio_source_function_unbind_config,
	.attributes	= audio_source_function_attributes,
};

static struct android_usb_function *supported_functions[] = {
	&ffs_function,
	&adb_function,
	&acm_function,
	&mtp_function,
	&ptp_function,
	&ecm_function,
	&eem_function,
	&serial_function,
	&rndis_function,
	&mass_storage_function,
	&accessory_function,
	&audio_source_function,
#ifdef EVDO_DT_VIA_SUPPORT
	&rawbulk_modem_function,
	&rawbulk_ets_function,
	&rawbulk_atc_function,
	&rawbulk_pcv_function,
	&rawbulk_gps_function,
#endif
	NULL
};


static int android_init_functions(struct android_usb_function **functions,
				  struct usb_composite_dev *cdev)
{
	struct android_dev *dev = _android_dev;
	struct android_usb_function *f;
	struct device_attribute **attrs;
	struct device_attribute *attr;
	int err;
	int index = 0;

	for (; (f = *functions++); index++) {
		f->dev_name = kasprintf(GFP_KERNEL, "f_%s", f->name);
		/* Added for USB Develpment debug, more log for more debuging help */
		xlog_printk(ANDROID_LOG_DEBUG, USB_LOG, "%s: f->dev_name = %s, f->name = %s", __func__, f->dev_name, f->name);
		/* Added for USB Develpment debug, more log for more debuging help */
		f->dev = device_create(android_class, dev->dev,
				MKDEV(0, index), f, f->dev_name);
		if (IS_ERR(f->dev)) {
			pr_err("%s: Failed to create dev %s", __func__,
							f->dev_name);
			err = PTR_ERR(f->dev);
			goto err_create;
		}

		if (f->init) {
			err = f->init(f, cdev);
			if (err) {
				pr_err("%s: Failed to init %s", __func__,
								f->name);
				goto err_out;
			} else {
				xlog_printk(ANDROID_LOG_DEBUG, USB_LOG, "%s: init %s success!!", __func__, f->name);
			}

		}

		attrs = f->attributes;
		if (attrs) {
			while ((attr = *attrs++) && !err)
				err = device_create_file(f->dev, attr);
		}
		if (err) {
			pr_err("%s: Failed to create function %s attributes",
					__func__, f->name);
			goto err_out;
		}
	}
	return 0;

err_out:
	device_destroy(android_class, f->dev->devt);
err_create:
	kfree(f->dev_name);
	return err;
}

static void android_cleanup_functions(struct android_usb_function **functions)
{
	struct android_usb_function *f;

	while (*functions) {
		f = *functions++;

		if (f->dev) {
			device_destroy(android_class, f->dev->devt);
			kfree(f->dev_name);
		}

		if (f->cleanup)
			f->cleanup(f);
	}
}

static int
android_bind_enabled_functions(struct android_dev *dev,
			       struct usb_configuration *c)
{
	struct android_usb_function *f;
	int ret;

	/* Added for USB Develpment debug, more log for more debuging help */
	xlog_printk(ANDROID_LOG_DEBUG, USB_LOG, "%s: ", __func__);
	/* Added for USB Develpment debug, more log for more debuging help */

	list_for_each_entry(f, &dev->enabled_functions, enabled_list) {
		ret = f->bind_config(f, c);
		if (ret) {
			pr_err("%s: %s failed", __func__, f->name);
			return ret;
		}
	}
	return 0;
}

static void
android_unbind_enabled_functions(struct android_dev *dev,
			       struct usb_configuration *c)
{
	struct android_usb_function *f;

	list_for_each_entry(f, &dev->enabled_functions, enabled_list) {
		if (f->unbind_config)
			f->unbind_config(f, c);
	}
}

static int android_enable_function(struct android_dev *dev, char *name)
{
	struct android_usb_function **functions = dev->functions;
	struct android_usb_function *f;
	while ((f = *functions++)) {

		/* Added for USB Develpment debug, more log for more debuging help */
		xlog_printk(ANDROID_LOG_DEBUG, USB_LOG, "%s: name = %s, f->name=%s \n", __func__, name, f->name);
		/* Added for USB Develpment debug, more log for more debuging help */
		if (!strcmp(name, f->name)) {
			list_add_tail(&f->enabled_list,
						&dev->enabled_functions);
			return 0;
		}
	}
	return -EINVAL;
}

/*-------------------------------------------------------------------------*/
/* /sys/class/android_usb/android%d/ interface */

static ssize_t
functions_show(struct device *pdev, struct device_attribute *attr, char *buf)
{
	struct android_dev *dev = dev_get_drvdata(pdev);
	struct android_usb_function *f;
	char *buff = buf;

	/* Added for USB Develpment debug, more log for more debuging help */
	xlog_printk(ANDROID_LOG_DEBUG, USB_LOG, "%s: ", __func__);
	/* Added for USB Develpment debug, more log for more debuging help */

	mutex_lock(&dev->mutex);

	list_for_each_entry(f, &dev->enabled_functions, enabled_list)
		buff += sprintf(buff, "%s,", f->name);

	mutex_unlock(&dev->mutex);

	if (buff != buf)
		*(buff-1) = '\n';
	return buff - buf;
}

static ssize_t
functions_store(struct device *pdev, struct device_attribute *attr,
			       const char *buff, size_t size)
{
	struct android_dev *dev = dev_get_drvdata(pdev);
	char *name;
	char buf[256], *b;
	char aliases[256], *a;
	int err;
	int is_ffs;
	int ffs_enabled = 0;

	mutex_lock(&dev->mutex);

	if (dev->enabled) {
		mutex_unlock(&dev->mutex);
		return -EBUSY;
	}

	INIT_LIST_HEAD(&dev->enabled_functions);

	/* Added for USB Develpment debug, more log for more debuging help */
	xlog_printk(ANDROID_LOG_DEBUG, USB_LOG, "%s: \n", __func__);
	/* Added for USB Develpment debug, more log for more debuging help */

	strlcpy(buf, buff, sizeof(buf));
	b = strim(buf);

	while (b) {
		name = strsep(&b, ",");

		/* Added for USB Develpment debug, more log for more debuging help */
		xlog_printk(ANDROID_LOG_INFO, USB_LOG, "%s: name = %s \n", __func__, name);
		/* Added for USB Develpment debug, more log for more debuging help */

		if (!name)
			continue;

		is_ffs = 0;
		strlcpy(aliases, dev->ffs_aliases, sizeof(aliases));
		a = aliases;

		while (a) {
			char *alias = strsep(&a, ",");
			if (alias && !strcmp(name, alias)) {
				is_ffs = 1;
				break;
			}
		}

		if (is_ffs) {
			if (ffs_enabled)
				continue;
			err = android_enable_function(dev, "ffs");
			if (err)
				pr_err("android_usb: Cannot enable ffs (%d)",
									err);
			else
				ffs_enabled = 1;
			continue;
		}

		err = android_enable_function(dev, name);
		if (err)
			pr_err("android_usb: Cannot enable '%s' (%d)",
							   name, err);
	}

	mutex_unlock(&dev->mutex);

	return size;
}

static ssize_t enable_show(struct device *pdev, struct device_attribute *attr,
			   char *buf)
{
	struct android_dev *dev = dev_get_drvdata(pdev);
	return sprintf(buf, "%d\n", dev->enabled);
}

static ssize_t enable_store(struct device *pdev, struct device_attribute *attr,
			    const char *buff, size_t size)
{
	struct android_dev *dev = dev_get_drvdata(pdev);
	struct usb_composite_dev *cdev = dev->cdev;
	struct android_usb_function *f;
	int enabled = 0;


	if (!cdev)
		return -ENODEV;

	mutex_lock(&dev->mutex);

	/* Added for USB Develpment debug, more log for more debuging help */
	xlog_printk(ANDROID_LOG_DEBUG, USB_LOG, "%s: device_attr->attr.name: %s\n", __func__, attr->attr.name);
	/* Added for USB Develpment debug, more log for more debuging help */

	sscanf(buff, "%d", &enabled);
	if (enabled && !dev->enabled) {
		/*
		 * Update values in composite driver's copy of
		 * device descriptor.
		 */
		cdev->desc.idVendor = device_desc.idVendor;
		cdev->desc.idProduct = device_desc.idProduct;
		cdev->desc.bcdDevice = device_desc.bcdDevice;
		cdev->desc.bDeviceClass = device_desc.bDeviceClass;
		cdev->desc.bDeviceSubClass = device_desc.bDeviceSubClass;
		cdev->desc.bDeviceProtocol = device_desc.bDeviceProtocol;

		/* special case for meta mode */
		if (serial_string[0] == 0x0) {
			cdev->desc.iSerialNumber = 0;
		} else {
			cdev->desc.iSerialNumber = device_desc.iSerialNumber;
		}

		list_for_each_entry(f, &dev->enabled_functions, enabled_list) {
			if (f->enable)
				f->enable(f);
		}
		android_enable(dev);
		dev->enabled = true;

		/* Added for USB Develpment debug, more log for more debuging help */
		xlog_printk(ANDROID_LOG_INFO, USB_LOG, "%s: enable 0->1 case, device_desc.idVendor = 0x%x, device_desc.idProduct = 0x%x, ", __func__, device_desc.idVendor, device_desc.idProduct);
		/* Added for USB Develpment debug, more log for more debuging help */

	} else if (!enabled && dev->enabled) {

		/* Added for USB Develpment debug, more log for more debuging help */
		xlog_printk(ANDROID_LOG_INFO, USB_LOG, "%s: enable 1->0 case, device_desc.idVendor = 0x%x, device_desc.idProduct = 0x%x, ", __func__, device_desc.idVendor, device_desc.idProduct);
		/* Added for USB Develpment debug, more log for more debuging help */

		android_disable(dev);
		list_for_each_entry(f, &dev->enabled_functions, enabled_list) {
			if (f->disable)
				f->disable(f);
		}
		dev->enabled = false;
	} else {
		pr_err("android_usb: already %s\n",
				dev->enabled ? "enabled" : "disabled");
		/* Add for HW/SW connect */
		if (!usb_cable_connected()) {
			schedule_work(&dev->work);
			xlog_printk(ANDROID_LOG_VERBOSE, USB_LOG, "%s: enable 0->0 case - no usb cable", __func__);
		}
		/* Add for HW/SW connect */
	}

	mutex_unlock(&dev->mutex);
	return size;
}

static ssize_t state_show(struct device *pdev, struct device_attribute *attr,
			   char *buf)
{
	struct android_dev *dev = dev_get_drvdata(pdev);
	struct usb_composite_dev *cdev = dev->cdev;
	char *state = "DISCONNECTED";
	unsigned long flags;

	if (!cdev)
		goto out;

	spin_lock_irqsave(&cdev->lock, flags);
	if (cdev->config)
		state = "CONFIGURED";
	else if (dev->connected)
		state = "CONNECTED";
	spin_unlock_irqrestore(&cdev->lock, flags);
out:
	return sprintf(buf, "%s\n", state);
}

#define DESCRIPTOR_ATTR(field, format_string)				\
static ssize_t								\
field ## _show(struct device *dev, struct device_attribute *attr,	\
		char *buf)						\
{									\
	return sprintf(buf, format_string, device_desc.field);		\
}									\
static ssize_t								\
field ## _store(struct device *dev, struct device_attribute *attr,	\
		const char *buf, size_t size)				\
{									\
	int value;							\
	if (sscanf(buf, format_string, &value) == 1) {			\
		device_desc.field = value;				\
		return size;						\
	}								\
	return -1;							\
}									\
static DEVICE_ATTR(field, S_IRUGO | S_IWUSR, field ## _show, field ## _store);

#define DESCRIPTOR_STRING_ATTR(field, buffer)				\
static ssize_t								\
field ## _show(struct device *dev, struct device_attribute *attr,	\
		char *buf)						\
{									\
	return sprintf(buf, "%s", buffer);				\
}									\
static ssize_t								\
field ## _store(struct device *dev, struct device_attribute *attr,	\
		const char *buf, size_t size)				\
{									\
	if (size >= sizeof(buffer))					\
		return -EINVAL;						\
	if (sscanf(buf, "%s", buffer) == 1) {				\
		return size;						\
	}								\
	return -1;							\
}									\
static DEVICE_ATTR(field, S_IRUGO | S_IWUSR, field ## _show, field ## _store);


DESCRIPTOR_ATTR(idVendor, "%04x\n")
DESCRIPTOR_ATTR(idProduct, "%04x\n")
DESCRIPTOR_ATTR(bcdDevice, "%04x\n")
DESCRIPTOR_ATTR(bDeviceClass, "%d\n")
DESCRIPTOR_ATTR(bDeviceSubClass, "%d\n")
DESCRIPTOR_ATTR(bDeviceProtocol, "%d\n")
DESCRIPTOR_STRING_ATTR(iManufacturer, manufacturer_string)
DESCRIPTOR_STRING_ATTR(iProduct, product_string)
DESCRIPTOR_STRING_ATTR(iSerial, serial_string)

static DEVICE_ATTR(functions, S_IRUGO | S_IWUSR, functions_show,
						 functions_store);
static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR, enable_show, enable_store);
static DEVICE_ATTR(state, S_IRUGO, state_show, NULL);

static struct device_attribute *android_usb_attributes[] = {
	&dev_attr_idVendor,
	&dev_attr_idProduct,
	&dev_attr_bcdDevice,
	&dev_attr_bDeviceClass,
	&dev_attr_bDeviceSubClass,
	&dev_attr_bDeviceProtocol,
	&dev_attr_iManufacturer,
	&dev_attr_iProduct,
	&dev_attr_iSerial,
	&dev_attr_functions,
	&dev_attr_enable,
	&dev_attr_state,
	NULL
};

/*-------------------------------------------------------------------------*/
/* Composite driver */


static int android_bind_config(struct usb_configuration *c)
{
	struct android_dev *dev = _android_dev;
	int ret = 0;

	ret = android_bind_enabled_functions(dev, c);
	if (ret)
		return ret;

	return 0;
}

static void android_unbind_config(struct usb_configuration *c)
{
	struct android_dev *dev = _android_dev;

	android_unbind_enabled_functions(dev, c);
}

static int android_bind(struct usb_composite_dev *cdev)
{
	struct android_dev *dev = _android_dev;
	struct usb_gadget	*gadget = cdev->gadget;
	int			gcnum, id, ret;
	
	/*
	 * Start disconnected. Userspace will connect the gadget once
	 * it is done configuring the functions.
	 */
	usb_gadget_disconnect(gadget);

	ret = android_init_functions(dev->functions, cdev);
	if (ret)
		return ret;

	/* Allocate string descriptor numbers ... note that string
	 * contents can be overridden by the composite_dev glue.
	 */
	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_MANUFACTURER_IDX].id = id;
	device_desc.iManufacturer = id;

	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_PRODUCT_IDX].id = id;
	device_desc.iProduct = id;

	/* Default strings - should be updated by userspace */
	strncpy(manufacturer_string, MANUFACTURER_STRING, sizeof(manufacturer_string) - 1);
	strncpy(product_string, PRODUCT_STRING, sizeof(product_string) - 1);
	strncpy(serial_string, "0123456789ABCDEF", sizeof(serial_string) - 1);

	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_SERIAL_IDX].id = id;
	device_desc.iSerialNumber = id;
	strings_dev[STRING_SERIAL_IDX].s = serial_number;

	gcnum = usb_gadget_controller_number(gadget);
	if (gcnum >= 0)
		device_desc.bcdDevice = cpu_to_le16(0x0200 + gcnum);
	else {
		pr_warning("%s: controller '%s' not recognized\n",
			longname, gadget->name);
		device_desc.bcdDevice = __constant_cpu_to_le16(0x9999);
	}

	usb_gadget_set_selfpowered(gadget);
	dev->cdev = cdev;

	return 0;
}

static int android_usb_unbind(struct usb_composite_dev *cdev)
{
	struct android_dev *dev = _android_dev;

	cancel_work_sync(&dev->work);
	android_cleanup_functions(dev->functions);
	return 0;
}

static struct usb_composite_driver android_usb_driver = {
	.name		= "android_usb",
	.dev		= &device_desc,
	.strings	= dev_strings,
	.unbind		= android_usb_unbind,
	.max_speed	= USB_SPEED_HIGH,
};

static int
android_setup(struct usb_gadget *gadget, const struct usb_ctrlrequest *c)
{
	struct android_dev		*dev = _android_dev;
	struct usb_composite_dev	*cdev = get_gadget_data(gadget);
	struct usb_request		*req = cdev->req;
	struct android_usb_function	*f;
	int value = -EOPNOTSUPP;
	unsigned long flags;

	req->zero = 0;
	req->complete = composite_setup_complete;
	req->length = 0;
	gadget->ep0->driver_data = cdev;

	list_for_each_entry(f, &dev->enabled_functions, enabled_list) {
		if (f->ctrlrequest) {
			value = f->ctrlrequest(f, cdev, c);
			if (value >= 0)
				break;
		}
	}

	/* Special case the accessory function.
	 * It needs to handle control requests before it is enabled.
	 */
	if (value < 0)
		value = acc_ctrlrequest(cdev, c);

	if (value < 0)
		value = composite_setup(gadget, c);

	spin_lock_irqsave(&cdev->lock, flags);
	if (!dev->connected) {
		dev->connected = 1;
		schedule_work(&dev->work);
	} else if (c->bRequest == USB_REQ_SET_CONFIGURATION &&
						cdev->config) {
		schedule_work(&dev->work);
	}
	spin_unlock_irqrestore(&cdev->lock, flags);

	return value;
}

static void android_disconnect(struct usb_gadget *gadget)
{
	struct android_dev *dev = _android_dev;
	struct usb_composite_dev *cdev = get_gadget_data(gadget);
	unsigned long flags;

	/* Added for USB Develpment debug, more log for more debuging help */
	xlog_printk(ANDROID_LOG_VERBOSE, USB_LOG, "%s: \n", __func__);
	/* Added for USB Develpment debug, more log for more debuging help */

	composite_disconnect(gadget);
	/* accessory HID support can be active while the
	   accessory function is not actually enabled,
	   so we need to inform it when we are disconnected.
	 */
	acc_disconnect();

	spin_lock_irqsave(&cdev->lock, flags);
	dev->connected = 0;
	schedule_work(&dev->work);

	/* Added for USB Develpment debug, more log for more debuging help */
	xlog_printk(ANDROID_LOG_VERBOSE, USB_LOG, "%s: dev->connected = %d \n", __func__, dev->connected);
	/* Added for USB Develpment debug, more log for more debuging help */

	spin_unlock_irqrestore(&cdev->lock, flags);
}

static int android_create_device(struct android_dev *dev)
{
	struct device_attribute **attrs = android_usb_attributes;
	struct device_attribute *attr;
	int err;

	dev->dev = device_create(android_class, NULL,
					MKDEV(0, 0), NULL, "android0");
	if (IS_ERR(dev->dev))
		return PTR_ERR(dev->dev);

	dev_set_drvdata(dev->dev, dev);

	while ((attr = *attrs++)) {
		err = device_create_file(dev->dev, attr);
		if (err) {
			device_destroy(android_class, dev->dev->devt);
			return err;
		}
	}
	return 0;
}

static int __init init(void)
{
	struct android_dev *dev;
	int err;

	android_class = class_create(THIS_MODULE, "android_usb");
	if (IS_ERR(android_class))
		return PTR_ERR(android_class);

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->disable_depth = 1;
	dev->functions = supported_functions;
	INIT_LIST_HEAD(&dev->enabled_functions);
	INIT_WORK(&dev->work, android_work);
	mutex_init(&dev->mutex);

	err = android_create_device(dev);
	if (err) {
		class_destroy(android_class);
		kfree(dev);
		return err;
	}

	_android_dev = dev;

	/* Override composite driver functions */
	composite_driver.setup = android_setup;
	composite_driver.disconnect = android_disconnect;

	#ifdef CKT_MAKE_ADB_DEVICES_NAME_TO_RANDOM
	// CKT Helin 2013-11-29 18:29:09 modify start	
	g_p_adb_device = kzalloc(sizeof(char)*ADB_DEVICES_MAX_LENGTH, GFP_KERNEL);
	if (g_p_adb_device!=NULL)
	{
		struct timeval now;

		do_gettimeofday(&now);
		
		int i=0,j=0;
		
		g_sec=now.tv_sec;
		g_usec=now.tv_usec;
		//sprintf(g_ts,"_%d.%d",now.tv_sec,now.tv_usec);
		#if 0
			sprintf(g_ts,"_%08x",(0xffffffff & (now.tv_sec | now.tv_usec) ) ^ 0xffffffff);
		#else
			uint64_t key=0;

			key = get_devinfo_with_index(13);
			key = (key << 32) | get_devinfo_with_index(12);
			if(key!=0){
				sprintf(g_ts,"_%08x",((0xffffffff & key) ^ ((key>>32) & 0xffffffff)));
			}
			//xlog_printk(ANDROID_LOG_VERBOSE, USB_LOG, "[%s][%d][%s]:KEY OK,=%08x,%08x,g_ts=%s",__FILE__,__LINE__,__func__,0xffffffff^key,0xffffffff^(key>>32),g_ts);

		#endif

		char* tmp=g_p_adb_device;
		
		int len=ADB_DEVICES_MAX_LENGTH-1;
		
		char* s_serial=(&((android_usb_driver.strings)[0]->strings)[STRING_SERIAL_IDX])->s;//PRODUCT_STRING;//ckt helin 20131202

		for (i = 0; i < len; ++i)
		{
			if(*s_serial=='\0')
			{
				break;
			}
			*tmp++=*s_serial++;
		}
		
		len=len-i;

		for (i=0; i < len; ++i)
		{
			if(g_ts[i]=='\0')
			{
				break;
			}
			*tmp++=g_ts[i];	
		}

		(&((android_usb_driver.strings)[0]->strings)[STRING_SERIAL_IDX])->s=g_p_adb_device;//"HAHA_ANDROID_DEVICE";

	}

	#endif//CKT_MAKE_ADB_DEVICES_NAME_TO_RANDOM


	return usb_composite_probe(&android_usb_driver, android_bind);
}
late_initcall(init);

static void __exit cleanup(void)
{
	usb_composite_unregister(&android_usb_driver);
	class_destroy(android_class);
	kfree(_android_dev);
	_android_dev = NULL;

	#ifdef CKT_MAKE_ADB_DEVICES_NAME_TO_RANDOM
	//ckt helin 201311202
	if (g_p_adb_device!=NULL)
	{
		kfree(g_p_adb_device);
	}
	g_p_adb_device=NULL;
	//ckt helin 20131202
	#endif//CKT_MAKE_ADB_DEVICES_NAME_TO_RANDOM
		
}
module_exit(cleanup);
