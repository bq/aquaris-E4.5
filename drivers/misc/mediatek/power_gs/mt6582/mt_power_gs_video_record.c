/*
* Copyright (C) 2011-2014 MediaTek Inc.
* 
* This program is free software: you can redistribute it and/or modify it under the terms of the 
* GNU General Public License version 2 as published by the Free Software Foundation.
* 
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/seq_file.h>

#include <mach/mt_typedefs.h>
#include <mach/mt_power_gs.h>

extern unsigned int *mt6582_power_gs_video_record;
extern unsigned int mt6582_power_gs_video_record_len;

extern unsigned int *mt6323_power_gs_video_record;
extern unsigned int mt6323_power_gs_video_record_len;

unsigned int mt6333_power_gs_video_record[] = {
    // Buck
    0x009F, 0x0080, 0x0000,
    0x00A0, 0x0007, 0x0000,
    0x006D, 0x007f, 0x0010,
};

void mt_power_gs_dump_video_record(void)
{
    mt_power_gs_compare("Video Record",                                                 \
                        mt6582_power_gs_video_record, mt6582_power_gs_video_record_len, \
                        mt6323_power_gs_video_record, mt6323_power_gs_video_record_len, \
                        mt6333_power_gs_video_record, sizeof(mt6333_power_gs_video_record));
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)
static int dump_video_record_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    char *p = buf;

    p += sprintf(p, "mt_power_gs : video_record\n");

    mt_power_gs_dump_video_record();

    len = p - buf;
    return len;
}
#else
static int dump_video_record_read(struct seq_file *m, void *v)
{
    seq_printf(m, "mt_power_gs : video_record\n");
    mt_power_gs_dump_video_record();
    return 0;
}

static int proc_mt_power_gs_dump_video_record_open(struct inode *inode, struct file *file)
{
    return single_open(file, dump_video_record_read, NULL);
}
static const struct file_operations mt_power_gs_dump_video_record_proc_fops = {
    .owner = THIS_MODULE,
    .open  = proc_mt_power_gs_dump_video_record_open, 
    .read  = seq_read,
};
#endif

static void __exit mt_power_gs_video_record_exit(void)
{
    //return 0;
}

static int __init mt_power_gs_video_record_init(void)
{
    struct proc_dir_entry *mt_entry = NULL;

    if (!mt_power_gs_dir)
    {
        printk("[%s]: mkdir /proc/mt_power_gs failed\n", __FUNCTION__);
    }
    else
    {
        #if LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)
        mt_entry = create_proc_entry("dump_video_record", S_IRUGO | S_IWUSR | S_IWGRP, mt_power_gs_dir);
        if (mt_entry)
        {
            mt_entry->read_proc = dump_video_record_read;
        }
        #else
        if (proc_create("dump_video_record", S_IRUGO | S_IWUSR | S_IWGRP, mt_power_gs_dir, &mt_power_gs_dump_video_record_proc_fops) == NULL) {
            pr_err("%s: create_proc_entry dump_video_record failed\n", __FUNCTION__);
        }
        #endif
    }

    return 0;
}

module_init(mt_power_gs_video_record_init);
module_exit(mt_power_gs_video_record_exit);

MODULE_DESCRIPTION("MT6582 Power Golden Setting - Video Record");
