/*
 *  HID support for Linux
 *
 *  Copyright (c) 1999 Andreas Gal
 *  Copyright (c) 2000-2005 Vojtech Pavlik <vojtech@suse.cz>
 *  Copyright (c) 2005 Michael Haboustak <mike-@cinci.rr.com> for Concept2, Inc
 *  Copyright (c) 2007-2008 Oliver Neukum
 *  Copyright (c) 2006-2012 Jiri Kosina
 *  Copyright (c) 2012 Henrik Rydberg
 *  Copyright (c) 2014 Andrzej Kaczmarek
 */

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <asm/unaligned.h>
#include <asm/byteorder.h>

#include <linux/hid.h>

static const struct hid_device_id hid_generic_devices[] = {
	{ HID_DEVICE(HID_BUS_ANY, HID_ANY_ID, HID_ANY_ID) },
	{ }
};
MODULE_DEVICE_TABLE(hid, hid_generic_devices);

static struct hid_driver hid_generic_driver = {
	.name = "hid-generic",
	.id_table = hid_generic_devices,
};

static int __init hid_generic_init2(void)
{
	int ret;

	ret = hid_register_driver(&hid_generic_driver);
	if (ret)
		pr_err("can't register hid-generic driver\n");

	return ret;
}

static void __exit hid_generic_exit2(void)
{
	hid_unregister_driver(&hid_generic_driver);
}

module_init(hid_generic_init2);
module_exit(hid_generic_exit2);

MODULE_AUTHOR("Henrik Rydberg");
MODULE_DESCRIPTION("HID generic driver");
MODULE_LICENSE("GPL");
