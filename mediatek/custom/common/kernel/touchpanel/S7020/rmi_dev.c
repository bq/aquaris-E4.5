/*
 * Synaptics DSX touchscreen driver
 *
 * Copyright (C) 2012 Synaptics Incorporated
 *
 * Copyright (C) 2012 Alexandra Chin <alexandra.chin@tw.synaptics.com>
 * Copyright (C) 2012 Scott Lin <scott.lin@tw.synaptics.com>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
//#include <linux/input/synaptics_dsx.h>
#include "synaptics_dsx_rmi4_i2c.h"
#include "tpd.h"
#include "cust_gpio_usage.h"
#include <cust_eint.h>



#define SYN_I2C_RETRY_TIMES 10

#define CHAR_DEVICE_NAME "rmi"
#define DEVICE_CLASS_NAME "rmidev"
#define DEV_NUMBER 1
#define REG_ADDR_LIMIT 0xFFFF

extern struct tpd_device *tpd;
extern struct kobject *properties_kobj_synap;
struct kobject *properties_kobj_rmidev;


static ssize_t rmidev_sysfs_open_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static ssize_t rmidev_sysfs_release_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static ssize_t rmidev_sysfs_address_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static ssize_t rmidev_sysfs_length_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static ssize_t rmidev_sysfs_data_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t rmidev_sysfs_data_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static ssize_t rmidev_sysfs_attn_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t rmidev_sysfs_attn_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

extern int tpd_i2c_read_data(struct i2c_client *client, unsigned short addr, unsigned char *data, unsigned short length);

extern int tpd_i2c_write_data(struct i2c_client *client, unsigned short addr, unsigned char *data, unsigned short length);


struct rmidev_handle {
	dev_t dev_no;
	unsigned short address;
	unsigned int length;
	struct device dev;
	struct synaptics_rmi4_data *rmi4_data;
	struct synaptics_rmi4_exp_fn_ptr *fn_ptr;
	struct kobject *sysfs_dir;
	void *data;
};

struct rmidev_data {
	int ref_count;
	struct cdev main_dev;
	struct class *device_class;
	struct mutex file_mutex;
	struct rmidev_handle *rmi_dev;
};

static struct device_attribute attrs[] = {
	__ATTR(open, (S_IRUGO | S_IWUSR),
			synaptics_rmi4_show_error,
			rmidev_sysfs_open_store),
	__ATTR(release, (S_IRUGO | S_IWUSR),
			synaptics_rmi4_show_error,
			rmidev_sysfs_release_store),
	__ATTR(address, (S_IRUGO | S_IWUSR),
			synaptics_rmi4_show_error,
			rmidev_sysfs_address_store),
	__ATTR(length, (S_IRUGO | S_IWUSR),
			synaptics_rmi4_show_error,
			rmidev_sysfs_length_store),
	__ATTR(data, (S_IRUGO | S_IWUSR),
			rmidev_sysfs_data_show,
			rmidev_sysfs_data_store),
	__ATTR(attn, (S_IRUGO | S_IWUSR),
			rmidev_sysfs_attn_show,
			rmidev_sysfs_attn_store),
};

static int rmidev_major_num;

static struct class *rmidev_device_class;

static struct rmidev_handle *rmidev;

static struct completion remove_complete;

static ssize_t rmidev_sysfs_open_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int input;

	if (sscanf(buf, "%u", &input) != 1)
			return -EINVAL;

	if (input != 1)
		return -EINVAL;

	rmidev->fn_ptr->enable(rmidev->rmi4_data, false);
	dev_dbg(&rmidev->rmi4_data->i2c_client->dev,
			"%s: Attention interrupt disabled\n",
			__func__);

	return count;
}

static ssize_t rmidev_sysfs_release_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int input;

	if (sscanf(buf, "%u", &input) != 1)
			return -EINVAL;

	if (input != 1)
		return -EINVAL;

	rmidev->fn_ptr->enable(rmidev->rmi4_data, true);
	dev_dbg(&rmidev->rmi4_data->i2c_client->dev,
			"%s: Attention interrupt enabled\n",
			__func__);

	return count;
}

static ssize_t rmidev_sysfs_address_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int input;

	if (sscanf(buf, "%u", &input) != 1)
			return -EINVAL;

	if (input > REG_ADDR_LIMIT)
		return -EINVAL;

	rmidev->address = (unsigned short)input;

	return count;
}

static ssize_t rmidev_sysfs_length_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int input;

	if (sscanf(buf, "%u", &input) != 1)
			return -EINVAL;

	if (input > REG_ADDR_LIMIT)
		return -EINVAL;

	rmidev->length = input;

	return count;
}

static ssize_t rmidev_sysfs_data_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int retval;
	unsigned int data_length = rmidev->length;

	if (data_length > (REG_ADDR_LIMIT - rmidev->address))
		data_length = REG_ADDR_LIMIT - rmidev->address;

	if (data_length) {
		retval = rmidev->fn_ptr->read(rmidev->rmi4_data,
				rmidev->address,
				(unsigned char *)buf,
				data_length);
		if (retval < 0) {
			dev_err(&rmidev->rmi4_data->i2c_client->dev,
					"%s: Failed to read data\n",
					__func__);
			return retval;
		}
	} else {
		return -EINVAL;
	}

	return data_length;
}

static ssize_t rmidev_sysfs_data_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned int data_length = rmidev->length;

	if (data_length > (REG_ADDR_LIMIT - rmidev->address))
		data_length = REG_ADDR_LIMIT - rmidev->address;

	if (data_length) {
		retval = rmidev->fn_ptr->write(rmidev->rmi4_data,
				rmidev->address,
				(unsigned char *)buf,
				data_length);
		if (retval < 0) {
			dev_err(&rmidev->rmi4_data->i2c_client->dev,
					"%s: Failed to write data\n",
					__func__);
			return retval;
		}
	} else {
		return -EINVAL;
	}

	return data_length;
}

static ssize_t rmidev_sysfs_attn_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int attn = mt_get_gpio_in(GPIO_CTP_EINT_PIN);
	return sprintf(buf, "%d %d\n", EINTF_TRIGGER_FALLING, attn);
}

static ssize_t rmidev_sysfs_attn_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	
	return 0;
}


/*
 * rmidev_llseek - used to set up register address
 *
 * @filp: file structure for seek
 * @off: offset
 *   if whence == SEEK_SET,
 *     high 16 bits: page address
 *     low 16 bits: register address
 *   if whence == SEEK_CUR,
 *     offset from current position
 *   if whence == SEEK_END,
 *     offset from end position (0xFFFF)
 * @whence: SEEK_SET, SEEK_CUR, or SEEK_END
 */
static loff_t rmidev_llseek(struct file *filp, loff_t off, int whence)
{
	loff_t newpos;
	struct rmidev_data *dev_data = filp->private_data;

	if (IS_ERR(dev_data)) {
		pr_err("%s: Pointer of char device data is invalid", __func__);
		return -EBADF;
	}

	mutex_lock(&(dev_data->file_mutex));

	switch (whence) {
	case SEEK_SET:
		newpos = off;
		break;
	case SEEK_CUR:
		newpos = filp->f_pos + off;
		break;
	case SEEK_END:
		newpos = REG_ADDR_LIMIT + off;
		break;
	default:
		newpos = -EINVAL;
		goto clean_up;
	}

	if (newpos < 0 || newpos > REG_ADDR_LIMIT) {
		dev_err(&rmidev->rmi4_data->i2c_client->dev,
				"%s: New position 0x%04x is invalid\n",
				__func__, (unsigned int)newpos);
		newpos = -EINVAL;
		goto clean_up;
	}

	filp->f_pos = newpos;

clean_up:
	mutex_unlock(&(dev_data->file_mutex));

	return newpos;
}

/*
 * rmidev_read: - use to read data from rmi device
 *
 * @filp: file structure for read
 * @buf: user space buffer pointer
 * @count: number of bytes to read
 * @f_pos: offset (starting register address)
 */
static ssize_t rmidev_read(struct file *filp, char __user *buf,
		size_t count, loff_t *f_pos)
{
	ssize_t retval;
	unsigned char tmpbuf[count + 1];
	struct rmidev_data *dev_data = filp->private_data;

	if (IS_ERR(dev_data)) {
		pr_err("%s: Pointer of char device data is invalid", __func__);
		return -EBADF;
	}

	if (count == 0)
		return 0;

	if (count > (REG_ADDR_LIMIT - *f_pos))
		count = REG_ADDR_LIMIT - *f_pos;

	mutex_lock(&(dev_data->file_mutex));

	retval = rmidev->fn_ptr->read(rmidev->rmi4_data,
			*f_pos,
			tmpbuf,
			count);
	if (retval < 0)
		goto clean_up;

	if (copy_to_user(buf, tmpbuf, count))
		retval = -EFAULT;
	else
		*f_pos += retval;

clean_up:
	mutex_unlock(&(dev_data->file_mutex));

	return retval;
}

/*
 * rmidev_write: - used to write data to rmi device
 *
 * @filep: file structure for write
 * @buf: user space buffer pointer
 * @count: number of bytes to write
 * @f_pos: offset (starting register address)
 */
static ssize_t rmidev_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *f_pos)
{
	ssize_t retval;
	unsigned char tmpbuf[count + 1];
	struct rmidev_data *dev_data = filp->private_data;

	if (IS_ERR(dev_data)) {
		pr_err("%s: Pointer of char device data is invalid", __func__);
		return -EBADF;
	}

	if (count == 0)
		return 0;

	if (count > (REG_ADDR_LIMIT - *f_pos))
		count = REG_ADDR_LIMIT - *f_pos;

	if (copy_from_user(tmpbuf, buf, count))
		return -EFAULT;

	mutex_lock(&(dev_data->file_mutex));

	retval = rmidev->fn_ptr->write(rmidev->rmi4_data,
			*f_pos,
			tmpbuf,
			count);
	if (retval >= 0)
		*f_pos += retval;

	mutex_unlock(&(dev_data->file_mutex));

	return retval;
}

/*
 * rmidev_open: enable access to rmi device
 * @inp: inode struture
 * @filp: file structure
 */
static int rmidev_open(struct inode *inp, struct file *filp)
{
	int retval = 0;
	struct rmidev_data *dev_data =
			container_of(inp->i_cdev, struct rmidev_data, main_dev);

	if (!dev_data)
		return -EACCES;

	filp->private_data = dev_data;

	mutex_lock(&(dev_data->file_mutex));

	rmidev->fn_ptr->enable(rmidev->rmi4_data, false);
	dev_dbg(&rmidev->rmi4_data->i2c_client->dev,
			"%s: Attention interrupt disabled\n",
			__func__);

	if (dev_data->ref_count < 1)
		dev_data->ref_count++;
	else
		retval = -EACCES;

	mutex_unlock(&(dev_data->file_mutex));

	return retval;
}

/*
 * rmidev_release: - release access to rmi device
 * @inp: inode structure
 * @filp: file structure
 */
static int rmidev_release(struct inode *inp, struct file *filp)
{
	struct rmidev_data *dev_data =
			container_of(inp->i_cdev, struct rmidev_data, main_dev);

	if (!dev_data)
		return -EACCES;

	mutex_lock(&(dev_data->file_mutex));

	dev_data->ref_count--;
	if (dev_data->ref_count < 0)
		dev_data->ref_count = 0;

	rmidev->fn_ptr->enable(rmidev->rmi4_data, true);
	dev_dbg(&rmidev->rmi4_data->i2c_client->dev,
			"%s: Attention interrupt enabled\n",
			__func__);

	mutex_unlock(&(dev_data->file_mutex));

	return 0;
}

static const struct file_operations rmidev_fops = {
	.owner = THIS_MODULE,
	.llseek = rmidev_llseek,
	.read = rmidev_read,
	.write = rmidev_write,
	.open = rmidev_open,
	.release = rmidev_release,
};

static void rmidev_device_cleanup(struct rmidev_data *dev_data)
{
	dev_t devno;

	if (dev_data) {
		devno = dev_data->main_dev.dev;

		if (dev_data->device_class)
			device_destroy(dev_data->device_class, devno);

		cdev_del(&dev_data->main_dev);

		unregister_chrdev_region(devno, 1);

		dev_dbg(&rmidev->rmi4_data->i2c_client->dev,
				"%s: rmidev device removed\n",
				__func__);
	}

	return;
}

static char *rmi_char_devnode(struct device *dev, mode_t *mode)
{
	if (!mode)
		return NULL;

	*mode = (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

	return kasprintf(GFP_KERNEL, "rmi/%s", dev_name(dev));
}

static int rmidev_create_device_class(void)
{
	rmidev_device_class = class_create(THIS_MODULE, DEVICE_CLASS_NAME);

	if (IS_ERR(rmidev_device_class)) {
		pr_err("%s: Failed to create /dev/%s\n",
				__func__, CHAR_DEVICE_NAME);
		return -ENODEV;
	}

	rmidev_device_class->devnode = rmi_char_devnode;

	return 0;
}

 /**
 * synaptics_rmi4_set_page()
 *
 * Called by synaptics_rmi4_i2c_read() and synaptics_rmi4_i2c_write().
 *
 * This function writes to the page select register to switch to the
 * assigned page.
 */
#if 0 
static int synaptics_rmi4_set_page(struct synaptics_rmi4_data *rmi4_data,
		unsigned int address)
{
	int retval = 0;
	unsigned char retry;
	unsigned char buf[PAGE_SELECT_LEN];
	unsigned char page;
	struct i2c_client *i2c = rmi4_data->i2c_client;

	page = ((address >> 8) & MASK_8BIT);
	if (page != rmi4_data->current_page) {
		buf[0] = MASK_8BIT;
		buf[1] = page;
		for (retry = 0; retry < SYN_I2C_RETRY_TIMES; retry++) {
			retval = i2c_master_send(i2c, buf, PAGE_SELECT_LEN);
			if (retval != PAGE_SELECT_LEN) {
				dev_err(&i2c->dev,
						"%s: I2C retry %d\n",
						__func__, retry + 1);
				msleep(20);
			} else {
				rmi4_data->current_page = page;
				break;
			}
		}
	} else {
		retval = PAGE_SELECT_LEN;
	}

	return retval;
}
#endif
 /**
 * synaptics_rmi4_i2c_read()
 *
 * Called by various functions in this driver, and also exported to
 * other expansion Function modules such as rmi_dev.
 *
 * This function reads data of an arbitrary length from the sensor,
 * starting from an assigned register address of the sensor, via I2C
 * with a retry mechanism.
 */
static int synaptics_rmi4_i2c_read(struct synaptics_rmi4_data *rmi4_data,
		unsigned short addr, unsigned char *data, unsigned short length)
{
	return tpd_i2c_read_data(rmi4_data->i2c_client, addr, data, length);
	
/*	int retval;
	unsigned char retry;
	unsigned char buf;
	struct i2c_msg msg[] = {
		{
			.addr = rmi4_data->i2c_client->addr,
			.flags = 0,
			.len = 1,
			.buf = &buf,
		},
		{
			.addr = rmi4_data->i2c_client->addr,
			.flags = I2C_M_RD,
			.len = length,
			.buf = data,
		},
	};

	buf = addr & MASK_8BIT;

	mutex_lock(&(rmi4_data->rmi4_io_ctrl_mutex));

	retval = synaptics_rmi4_set_page(rmi4_data, addr);
	if (retval != PAGE_SELECT_LEN)
		goto exit;

	for (retry = 0; retry < SYN_I2C_RETRY_TIMES; retry++) {
		if (i2c_transfer(rmi4_data->i2c_client->adapter, msg, 2) == 2) {
			retval = length;
			break;
		}
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: I2C retry %d\n",
				__func__, retry + 1);
		msleep(20);
	}

	if (retry == SYN_I2C_RETRY_TIMES) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: I2C read over retry limit\n",
				__func__);
		retval = -EIO;
	}

exit:
	mutex_unlock(&(rmi4_data->rmi4_io_ctrl_mutex));

	return retval;*/

}

 /**
 * synaptics_rmi4_i2c_write()
 *
 * Called by various functions in this driver, and also exported to
 * other expansion Function modules such as rmi_dev.
 *
 * This function writes data of an arbitrary length to the sensor,
 * starting from an assigned register address of the sensor, via I2C with
 * a retry mechanism.
 */
static int synaptics_rmi4_i2c_write(struct synaptics_rmi4_data *rmi4_data,
		unsigned short addr, unsigned char *data, unsigned short length)
{
	return tpd_i2c_write_data(rmi4_data->i2c_client, addr, data, length);
	
/*	int retval;
	unsigned char retry;
	unsigned char buf[length + 1];
	struct i2c_msg msg[] = {
		{
			.addr = rmi4_data->i2c_client->addr,
			.flags = 0,
			.len = length + 1,
			.buf = buf,
		}
	};

	mutex_lock(&(rmi4_data->rmi4_io_ctrl_mutex));

	retval = synaptics_rmi4_set_page(rmi4_data, addr);
	if (retval != PAGE_SELECT_LEN)
		goto exit;

	buf[0] = addr & MASK_8BIT;
	memcpy(&buf[1], &data[0], length);

	for (retry = 0; retry < SYN_I2C_RETRY_TIMES; retry++) {
		if (i2c_transfer(rmi4_data->i2c_client->adapter, msg, 1) == 1) {
			retval = length;
			break;
		}
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: I2C retry %d\n",
				__func__, retry + 1);
		msleep(20);
	}

	if (retry == SYN_I2C_RETRY_TIMES) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: I2C write over retry limit\n",
				__func__);
		retval = -EIO;
	}

exit:
	mutex_unlock(&(rmi4_data->rmi4_io_ctrl_mutex));

	return retval;*/
}

/**
 * synaptics_rmi4_irq_enable()
 *
 * Called by synaptics_rmi4_probe() and the power management functions
 * in this driver and also exported to other expansion Function modules
 * such as rmi_dev.
 *
 * This function handles the enabling and disabling of the attention
 * irq including the setting up of the ISR thread.
 */
static int synaptics_rmi4_irq_enable(struct synaptics_rmi4_data *rmi4_data,
		bool enable)
{
	int retval = 0;
	unsigned char intr_status;
	//const struct synaptics_dsx_platform_data *platform_data = rmi4_data->i2c_client->dev.platform_data;

	if (enable) {
		if (rmi4_data->irq_enabled)
			return retval;

		/* Clear interrupts first */
		retval = synaptics_rmi4_i2c_read(rmi4_data,
				rmi4_data->f01_data_base_addr + 1,
				&intr_status,
				rmi4_data->num_of_intr_regs);
		if (retval < 0)
			return retval;

		/*retval = request_threaded_irq(rmi4_data->irq, NULL,
				synaptics_rmi4_irq, platform_data->irq_type,
				DRIVER_NAME, rmi4_data);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to create irq thread\n",
					__func__);
			return retval;
		}*/

		rmi4_data->irq_enabled = true;
	} else {
		if (rmi4_data->irq_enabled) {
			disable_irq(rmi4_data->irq);
			free_irq(rmi4_data->irq, rmi4_data);
			rmi4_data->irq_enabled = false;
		}
	}

	return retval;
}


static int rmidev_init_device(struct i2c_client *client)
{
	int retval;
	dev_t dev_no;
	unsigned char attr_count;
	struct rmidev_data *dev_data;
	struct device *device_ptr;

	TPD_DMESG("%s:enter \n",__func__);
	
	rmidev = kzalloc(sizeof(*rmidev), GFP_KERNEL);
	if (!rmidev) {
		dev_err(&client->dev,
				"%s: Failed to alloc mem for rmidev\n",
				__func__);
		retval = -ENOMEM;
		goto err_rmidev;
	}

	rmidev->rmi4_data = kzalloc(sizeof(struct synaptics_rmi4_data), GFP_KERNEL);
	if (!rmidev->rmi4_data) {
		dev_err(&client->dev,
				"%s: Failed to alloc mem for rmi4_data\n",
				__func__);
		retval = -ENOMEM;
		goto err_fn_ptr;
	}

	rmidev->fn_ptr =  kzalloc(sizeof(*(rmidev->fn_ptr)), GFP_KERNEL);
	if (!rmidev) {
		dev_err(&client->dev,
				"%s: Failed to alloc mem for fn_ptr\n",
				__func__);
		retval = -ENOMEM;
		goto exit_free_rmi4;
	}

	rmidev->rmi4_data->input_dev = tpd->dev;
	rmidev->rmi4_data->i2c_client = client;
	rmidev->fn_ptr->read = synaptics_rmi4_i2c_read;
	rmidev->fn_ptr->write = synaptics_rmi4_i2c_write;
	rmidev->fn_ptr->enable = synaptics_rmi4_irq_enable;

	mutex_init(&(rmidev->rmi4_data->rmi4_io_ctrl_mutex));


	retval = rmidev_create_device_class();
	if (retval < 0) {
		dev_err(&client->dev,
				"%s: Failed to create device class\n",
				__func__);
		goto err_device_class;
	}

    TPD_DMESG("%s:after create device class \n",__func__);
	if (rmidev_major_num) {
		dev_no = MKDEV(rmidev_major_num, DEV_NUMBER);
		retval = register_chrdev_region(dev_no, 1, CHAR_DEVICE_NAME);
	} else {
		retval = alloc_chrdev_region(&dev_no, 0, 1, CHAR_DEVICE_NAME);
		if (retval < 0) {
			dev_err(&client->dev,
					"%s: Failed to allocate char device region\n",
					__func__);
			goto err_device_region;
		}

		rmidev_major_num = MAJOR(dev_no);
		dev_dbg(&client->dev,
				"%s: Major number of rmidev = %d\n",
				__func__, rmidev_major_num);
	}

	dev_data = kzalloc(sizeof(*dev_data), GFP_KERNEL);
	if (!dev_data) {
		dev_err(&client->dev,
				"%s: Failed to alloc mem for dev_data\n",
				__func__);
		retval = -ENOMEM;
		goto err_dev_data;
	}

	mutex_init(&dev_data->file_mutex);
	dev_data->rmi_dev = rmidev;
	rmidev->data = dev_data;

	cdev_init(&dev_data->main_dev, &rmidev_fops);

	retval = cdev_add(&dev_data->main_dev, dev_no, 1);
	if (retval < 0) {
		dev_err(&client->dev,
				"%s: Failed to add rmi char device\n",
				__func__);
		goto err_char_device;
	}

	dev_set_name(&rmidev->dev, "rmidev%d", MINOR(dev_no));
	dev_data->device_class = rmidev_device_class;

	device_ptr = device_create(dev_data->device_class, NULL, dev_no,
			NULL, CHAR_DEVICE_NAME"%d", MINOR(dev_no));
	
	TPD_DMESG("%s:after devicecreate  \n",__func__);
	
	if (IS_ERR(device_ptr)) {
		dev_err(&client->dev,
				"%s: Failed to create rmi char device\n",
				__func__);
		retval = -ENODEV;
		goto err_char_device;
	}

/*	retval = gpio_export(rmidev->rmi4_data->board->gpio, false);
	if (retval < 0) {
		dev_err(&client->dev,
				"%s: Failed to export gpio (%d)\n",
				__func__, rmidev->rmi4_data->board->gpio);
	} else {
		retval = gpio_export_link(&(rmidev->rmi4_data->input_dev->dev),
				"attn", rmidev->rmi4_data->board->gpio);
		if (retval < 0) {
			dev_err(&rmidev->rmi4_data->input_dev->dev,
					"%s Failed to create gpio symlink\n",
					__func__);
		} else {
			dev_dbg(&rmidev->rmi4_data->input_dev->dev,
					"%s: Exported gpio %d\n",
					__func__, rmi4_data->board->gpio);
		}
	}*/

/*	rmidev->sysfs_dir = kobject_create_and_add("rmidev",
			&rmidev->rmi4_data->input_dev->dev.kobj);

	TPD_DMESG("%s:rmidev after kobject_create_and_add  \n",__func__);
	
	if (!rmidev->sysfs_dir) {
		dev_err(&rmidev->rmi4_data->i2c_client->dev,
				"%s: Failed to create sysfs directory\n",
				__func__);
		goto err_sysfs_dir;
	}*/

	properties_kobj_rmidev = kobject_create_and_add("rmidev", properties_kobj_synap);
	if (!properties_kobj_rmidev) {
			dev_err(&rmidev->rmi4_data->i2c_client->dev,
					"%s: Failed to create sysfs directory\n",
					__func__);
			goto err_sysfs_dir;
	}


	for (attr_count = 0; attr_count < ARRAY_SIZE(attrs); attr_count++) {
		//retval = sysfs_create_file(rmidev->sysfs_dir, &attrs[attr_count].attr);
		retval = sysfs_create_file(properties_kobj_rmidev, &attrs[attr_count].attr);
		
		if (retval < 0) {
			dev_err(&rmidev->rmi4_data->input_dev->dev,
					"%s: Failed to create sysfs attributes\n",
					__func__);
			retval = -ENODEV;
			goto err_sysfs_attrs;
		}
	}

	return 0;

err_sysfs_attrs:
	for (attr_count--; attr_count >= 0; attr_count--) {
		sysfs_remove_file(properties_kobj_rmidev, &attrs[attr_count].attr);
	}

	kobject_put(properties_kobj_rmidev);

err_sysfs_dir:
err_char_device:
	rmidev_device_cleanup(dev_data);
	kfree(dev_data);

err_dev_data:
	unregister_chrdev_region(dev_no, 1);

err_device_region:
	class_destroy(rmidev_device_class);

err_device_class:
	kfree(rmidev->rmi4_data);
	
exit_free_rmi4:
	kfree(rmidev);

err_fn_ptr:
	kfree(rmidev);

err_rmidev:
	return retval;
}

static void rmidev_remove_device(struct i2c_client *client)
{
	unsigned char attr_count;
	struct rmidev_data *dev_data;

	if (!rmidev)
		return;

	for (attr_count = 0; attr_count < ARRAY_SIZE(attrs); attr_count++)
		sysfs_remove_file(rmidev->sysfs_dir, &attrs[attr_count].attr);

	kobject_put(rmidev->sysfs_dir);

	dev_data = rmidev->data;
	if (dev_data) {
		rmidev_device_cleanup(dev_data);
		kfree(dev_data);
	}

	unregister_chrdev_region(rmidev->dev_no, 1);

	class_destroy(rmidev_device_class);

	kfree(rmidev->fn_ptr);
	kfree(rmidev);

	complete(&remove_complete);

	return;
}

static int __init rmidev_module_init(void)
{
	synaptics_rmi4_new_function(RMI_DEV, true,
			rmidev_init_device,
			rmidev_remove_device,
			NULL);
	return 0;
}

static void __exit rmidev_module_exit(void)
{
	init_completion(&remove_complete);
	synaptics_rmi4_new_function(RMI_DEV, false,
			rmidev_init_device,
			rmidev_remove_device,
			NULL);
	wait_for_completion(&remove_complete);
	return;
}

module_init(rmidev_module_init);
module_exit(rmidev_module_exit);

MODULE_AUTHOR("Synaptics, Inc.");
MODULE_DESCRIPTION("RMI4 RMI_Dev Module");
MODULE_LICENSE("GPL");
MODULE_VERSION(SYNAPTICS_RMI4_DRIVER_VERSION);
