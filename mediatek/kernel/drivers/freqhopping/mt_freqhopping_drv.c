/*
 * Copyright (C) 2011 MediaTek, Inc.
 *
 * Author: Holmes Chiou <holmes.chiou@mediatek.com>
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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <asm/sched_clock.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <board-custom.h>
#include <linux/seq_file.h>

#include "mach/mt_freqhopping.h"
#include "mach/mt_fhreg.h"

#include <linux/xlog.h> //ccyeh: for 6572

#if 0 //ccyeh: we don't need these header files here. 
#include "mach/mt_clkmgr.h"
#include "mach/mt_typedefs.h"
#include "mach/mt_gpio.h"
#include "mach/mt_gpufreq.h"
#include "mach/mt_cpufreq.h"
#include "mach/emi_bwl.h"
#include "mach/sync_write.h"
#include "mach/mt_sleep.h"
#endif 

#include <mach/mt_freqhopping_drv.h>


#define FREQ_HOPPING_DEVICE "mt-freqhopping"

#define FH_PLL_COUNT		(g_p_fh_hal_drv->pll_cnt)

//static unsigned int		g_resume_mempll_ssc=false;
static struct mt_fh_hal_driver 	*g_p_fh_hal_drv;

static fh_pll_t 		*g_fh_drv_pll;
static struct freqhopping_ssc 	*g_fh_drv_usr_def;
static unsigned int		g_drv_pll_count;
	
static struct miscdevice mt_fh_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "mtfreqhopping",
	//.fops = &mt_fh_fops, //TODO: Interface for UI maybe in the future...
};


static int mt_freqhopping_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static int mt_fh_drv_probe(struct platform_device *dev)
{
	int	err;
   
   	FH_MSG("EN: mt_fh_probe()");
 
	if ((err = misc_register(&mt_fh_device)))
		FH_MSG("register fh driver error!");    

	return err;
}

static int mt_fh_drv_remove(struct platform_device *dev)
{
	int	err;    

	if ((err = misc_deregister(&mt_fh_device)))
		FH_MSG("deregister fh driver error!");
    
	return err;
}

static void mt_fh_drv_shutdown(struct platform_device *dev)
{
	FH_MSG("mt_fh_shutdown");
}

static int mt_fh_drv_suspend(struct platform_device *dev, pm_message_t state)
{
//	struct 		freqhopping_ioctl fh_ctl;

	FH_MSG("-supd-");

	return 0;
}

static int mt_fh_drv_resume(struct platform_device *dev)
{
//	struct 		freqhopping_ioctl fh_ctl;

	FH_MSG("+resm+");

	return 0;
}

static struct platform_driver freqhopping_driver = {
	.probe		= mt_fh_drv_probe,
	.remove		= mt_fh_drv_remove,
	.shutdown	= mt_fh_drv_shutdown,
	.suspend	= mt_fh_drv_suspend,
	.resume		= mt_fh_drv_resume,
	.driver		= {
		.name	= FREQ_HOPPING_DEVICE,
		.owner	= THIS_MODULE,
	},
};

static int mt_fh_enable_usrdef(struct freqhopping_ioctl* fh_ctl)
{
	unsigned long flags;
	
	FH_MSG("EN: %s",__func__);
	FH_MSG("pll_id: %d",fh_ctl->pll_id);

	//Check the wrong Frequency hopping PLL ID
	FH_BUG_ON(fh_ctl->pll_id > FH_PLL_COUNT);
	
	//we don't care the PLL status , we just change the flag & update the table
	//the setting will be applied during the following FH enable

	g_p_fh_hal_drv->mt_fh_lock(&flags);
	memcpy(&g_fh_drv_usr_def[fh_ctl->pll_id],&(fh_ctl->ssc_setting),sizeof(struct freqhopping_ssc));
	g_fh_drv_pll[fh_ctl->pll_id].user_defined = true;
	g_p_fh_hal_drv->mt_fh_unlock(&flags);

	//FH_MSG("Exit");

	return 0;

}

static int mt_fh_disable_usrdef(struct freqhopping_ioctl* fh_ctl)
{
	unsigned long flags;

	FH_MSG("EN: %s",__func__);
	FH_MSG("id: %d",fh_ctl->pll_id);

	//Check the wrong Frequency hopping PLL ID
	FH_BUG_ON(fh_ctl->pll_id > FH_PLL_COUNT);

	g_p_fh_hal_drv->mt_fh_lock(&flags);
	memset(&g_fh_drv_usr_def[fh_ctl->pll_id], 0,sizeof(struct freqhopping_ssc));
	g_fh_drv_pll[fh_ctl->pll_id].user_defined = false;
	g_p_fh_hal_drv->mt_fh_unlock(&flags);

	//FH_MSG("Exit");

	return 0;
}


static int mt_fh_hal_ctrl_lock(struct freqhopping_ioctl* fh_ctl,bool enable)
{
	int		retVal=1;
	unsigned long 	flags;

	FH_MSG("EN: _fctr_lck %d:%d",fh_ctl->pll_id,enable);

	g_p_fh_hal_drv->mt_fh_lock(&flags);
	retVal = g_p_fh_hal_drv->mt_fh_hal_ctrl(fh_ctl, enable);
	g_p_fh_hal_drv->mt_fh_unlock(&flags);
	
	return retVal;
}


static int mt_freqhopping_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	//Get the structure of ioctl
	int ret = 0;

	struct freqhopping_ioctl *freqhopping_ctl = (struct freqhopping_ioctl *)arg;
	
	FH_MSG("EN:CMD:%d id:%d",cmd,freqhopping_ctl->pll_id);

	if(FH_CMD_ENABLE == cmd) {
		ret = mt_fh_hal_ctrl_lock(freqhopping_ctl,true);
	}else if(FH_CMD_DISABLE == cmd) { 
		ret = mt_fh_hal_ctrl_lock(freqhopping_ctl,false);
	}
	else if(FH_CMD_ENABLE_USR_DEFINED == cmd) { 
		ret = mt_fh_enable_usrdef(freqhopping_ctl);
	}
	else if(FH_CMD_DISABLE_USR_DEFINED == cmd) { 
		ret = mt_fh_disable_usrdef(freqhopping_ctl);
	}else {
		//Invalid command is not acceptable!!	
		WARN_ON(1);
	}
	
	//FH_MSG("Exit");

	return ret;
}

void mt_fh_popod_save(void)
{
	FH_MSG("EN: %s",__func__);
	
	g_p_fh_hal_drv->mt_fh_popod_save();
}
EXPORT_SYMBOL(mt_fh_popod_save);

void mt_fh_popod_restore(void)
{
	FH_MSG("EN: %s",__func__);

	g_p_fh_hal_drv->mt_fh_popod_restore();
}
EXPORT_SYMBOL(mt_fh_popod_restore);

//static int freqhopping_userdefine_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
static int freqhopping_userdefine_proc_read(struct seq_file* m, void* v)
{
	int 	i=0;

	FH_MSG("EN: %s",__func__);

	seq_printf(m, "user defined settings:\n");

	seq_printf(m, "===============================================\r\n" );
	seq_printf(m, "     freq ==  delta t ==  delta f ==  up bond == low bond ==      dds ==\r\n" );

	for(i=0;i<g_drv_pll_count ;i++) {
		seq_printf(m, "%10d  0x%08x  0x%08x  0x%08x  0x%08x  0x%08x\r\n"
				,g_fh_drv_usr_def[i].freq
				,g_fh_drv_usr_def[i].dt
				,g_fh_drv_usr_def[i].df
				,g_fh_drv_usr_def[i].upbnd
				,g_fh_drv_usr_def[i].lowbnd
				,g_fh_drv_usr_def[i].dds);
	}
        return 0;

#if 0
	char 	*p = page;
	int 	len = 0;
	int 	i=0;

	FH_MSG("EN: %s",__func__);

	p += sprintf(p, "user defined settings:\n");

	p += sprintf(p, "===============================================\r\n" );
	p += sprintf(p, "     freq ==  delta t ==  delta f ==  up bond == low bond ==      dds ==\r\n" );

	for(i=0;i<g_drv_pll_count ;i++) {
		p += sprintf(p, "%10d  0x%08x  0x%08x  0x%08x  0x%08x  0x%08x\r\n"
				,g_fh_drv_usr_def[i].freq
				,g_fh_drv_usr_def[i].dt
				,g_fh_drv_usr_def[i].df
				,g_fh_drv_usr_def[i].upbnd
				,g_fh_drv_usr_def[i].lowbnd
				,g_fh_drv_usr_def[i].dds);
	}

	*start = page + off;

	len = p - page;

	if (len > off)
		len -= off;
	else
		len = 0;

	return len < count ? len : count;
#endif
}

static ssize_t freqhopping_userdefine_proc_write(struct file *file, const char *buffer, size_t count, loff_t *data)
{
	int 		ret;
	char 		kbuf[256];
	size_t 	len = 0;
	unsigned int 	p1,p2,p3,p4,p5,p6,p7;
	struct 		freqhopping_ioctl fh_ctl;
	
	p1=0;
	p2=0;
	p3=0;
	p4=0;
	p5=0;
	p6=0;
	p7=0;

	FH_MSG("EN: %s",__func__);
	
	len = min(count, (sizeof(kbuf)-1));

	if (count == 0)return -1;
	if(count > 255)count = 255;

	ret = copy_from_user(kbuf, buffer, count);
	if (ret < 0)return -1;
	
	kbuf[count] = '\0';

	sscanf(kbuf, "%x %x %x %x %x %x %x", &p1, &p2, &p3, &p4, &p5, &p6, &p7);

	fh_ctl.pll_id  			= p2;
	fh_ctl.ssc_setting.df		= p3;
	fh_ctl.ssc_setting.dt		= p4;
	fh_ctl.ssc_setting.upbnd	= p5;
	fh_ctl.ssc_setting.lowbnd	= p6;
	fh_ctl.ssc_setting.dds 		= p7;
	fh_ctl.ssc_setting.freq		= 0;

	
	if( p1 == FH_CMD_ENABLE){
		ret = mt_fh_enable_usrdef(&fh_ctl);
		if(ret){
			FH_MSG("__EnableUsrSetting() fail!");
		}
	}
	else{
		ret = mt_fh_disable_usrdef(&fh_ctl);
		if(ret){
			FH_MSG("__DisableUsrSetting() fail!");
		}
	}
		
	return count;
}

//static int freqhopping_status_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
static int freqhopping_status_proc_read(struct seq_file* m, void* v)
{
	int 	i=0;

	FH_MSG("EN: %s",__func__);

	seq_printf(m, "FH status:\r\n");

	seq_printf(m, "===============================================\r\n" );
	seq_printf(m, "id == fh_status == pll_status == setting_id == curr_freq == user_defined ==\r\n" );


	for(i=0;i<g_drv_pll_count ;i++) {
		seq_printf(m, "%2d    %8d      %8d",
			i,
			g_fh_drv_pll[i].fh_status,
			g_fh_drv_pll[i].pll_status);
		seq_printf(m, "      %8d     %8d ",
			g_fh_drv_pll[i].setting_id,
			g_fh_drv_pll[i].curr_freq);

		seq_printf(m, "        %d\r\n",
			g_fh_drv_pll[i].user_defined);
	}
	seq_printf(m, "\r\n");
        return 0;


#if 0
	char 	*p = page;
	int 	len = 0;
	int 	i=0;

	FH_MSG("EN: %s",__func__);

	p += sprintf(p, "FH status:\r\n");

	p += sprintf(p, "===============================================\r\n" );
	p += sprintf(p, "id == fh_status == pll_status == setting_id == curr_freq == user_defined ==\r\n" );


	for(i=0;i<g_drv_pll_count ;i++) {
		p += sprintf(p, "%2d    %8d      %8d",
			i,
			g_fh_drv_pll[i].fh_status,
			g_fh_drv_pll[i].pll_status);
		p += sprintf(p, "      %8d     %8d ",
			g_fh_drv_pll[i].setting_id,
			g_fh_drv_pll[i].curr_freq);

		p += sprintf(p, "        %d\r\n",
			g_fh_drv_pll[i].user_defined);
	}
#if 0
	p += sprintf(p, "\r\nPLL status:\r\n");
	for(i=0;i<g_drv_pll_count ;i++) {
		p += sprintf(p, "%d ",pll_is_on(i));
	}
#endif	
	p += sprintf(p, "\r\n");

	//TODO: unsigned int mt_get_cpu_freq(void)


	*start = page + off;

	len = p - page;

	if (len > off)
		len -= off;
	else
		len = 0;

	return len < count ? len : count;
#endif
}

static ssize_t freqhopping_status_proc_write(struct file *file, const char *buffer, size_t count, loff_t *data)
{
	int 		ret;
	char 		kbuf[256];
	size_t  len = 0;
	unsigned int	p1,p2,p3,p4,p5,p6;
	struct 		freqhopping_ioctl fh_ctl;
	
	p1=0;
	p2=0;
	p3=0;
	p4=0;
	p5=0;
	p6=0;

	FH_MSG("EN: %s",__func__);
	
	len = min(count, (sizeof(kbuf)-1));

	if (count == 0)return -1;
	if(count > 255)count = 255;

	ret = copy_from_user(kbuf, buffer, count);
	if (ret < 0)return -1;
	
	kbuf[count] = '\0';

	sscanf(kbuf, "%x %x", &p1, &p2);
	
	fh_ctl.pll_id  = p2;
	fh_ctl.ssc_setting.df= 0;
	fh_ctl.ssc_setting.dt= 0;
	fh_ctl.ssc_setting.upbnd= 0;
	fh_ctl.ssc_setting.lowbnd= 0;
	
	if( p1 == 0){
		mt_freqhopping_ioctl(NULL,FH_CMD_DISABLE,(unsigned long)(&fh_ctl));	
	}
	else{
		mt_freqhopping_ioctl(NULL,FH_CMD_ENABLE,(unsigned long)(&fh_ctl));	
	}
		
	return count;
}

//static int freqhopping_debug_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
static int freqhopping_debug_proc_read(struct seq_file* m, void* v)
{

	FH_MSG("EN: %s",__func__);

	seq_printf(m, "\r\n[freqhopping debug flag]\r\n");
	seq_printf(m, "===============================================\r\n" );
	seq_printf(m, "id==ARMPLL==MAINPLL==MEMPLL==MSDCPLL==MMPLL==VENCPLL\r\n" );
	seq_printf(m, "   == %04d====%04d====%04d====%04d====%04d====%04d=\r\n" ,
				g_fh_drv_pll[MT658X_FH_ARM_PLL].fh_status, g_fh_drv_pll[MT658X_FH_MAIN_PLL].fh_status,
				g_fh_drv_pll[MT658X_FH_MEM_PLL].fh_status, g_fh_drv_pll[MT658X_FH_MSDC_PLL].fh_status,
				g_fh_drv_pll[MT658X_FH_MM_PLL].fh_status, g_fh_drv_pll[MT658X_FH_VENC_PLL].fh_status);
	seq_printf(m, "   == %04d====%04d====%04d====%04d====%04d====%04d=\r\n" ,
				g_fh_drv_pll[MT658X_FH_ARM_PLL].setting_id, g_fh_drv_pll[MT658X_FH_MAIN_PLL].setting_id,
				g_fh_drv_pll[MT658X_FH_MEM_PLL].setting_id, g_fh_drv_pll[MT658X_FH_MSDC_PLL].setting_id,
				g_fh_drv_pll[MT658X_FH_MM_PLL].setting_id, g_fh_drv_pll[MT658X_FH_VENC_PLL].setting_id);
  return 0;
#if 0
	char *p = page;
	int len = 0;
	
	FH_MSG("EN: %s",__func__);

	p += sprintf(p, "\r\n[freqhopping debug flag]\r\n");
	p += sprintf(p, "===============================================\r\n" );
	p += sprintf(p, "id==ARMPLL==MAINPLL==MEMPLL==MSDCPLL==MMPLL==VENCPLL\r\n" );
	p += sprintf(p, "   == %04d====%04d====%04d====%04d====%04d====%04d=\r\n" ,
				g_fh_drv_pll[MT658X_FH_ARM_PLL].fh_status, g_fh_drv_pll[MT658X_FH_MAIN_PLL].fh_status,
				g_fh_drv_pll[MT658X_FH_MEM_PLL].fh_status, g_fh_drv_pll[MT658X_FH_MSDC_PLL].fh_status,
				g_fh_drv_pll[MT658X_FH_MM_PLL].fh_status, g_fh_drv_pll[MT658X_FH_VENC_PLL].fh_status);
	p += sprintf(p, "   == %04d====%04d====%04d====%04d====%04d====%04d=\r\n" ,
				g_fh_drv_pll[MT658X_FH_ARM_PLL].setting_id, g_fh_drv_pll[MT658X_FH_MAIN_PLL].setting_id,
				g_fh_drv_pll[MT658X_FH_MEM_PLL].setting_id, g_fh_drv_pll[MT658X_FH_MSDC_PLL].setting_id,
				g_fh_drv_pll[MT658X_FH_MM_PLL].setting_id, g_fh_drv_pll[MT658X_FH_VENC_PLL].setting_id);

	*start = page + off;

	len = p - page;

	if (len > off)
		len -= off;
	else
		len = 0;

	return len < count ? len : count;
#endif
}

static ssize_t freqhopping_debug_proc_write(struct file *file, const char *buffer, size_t count, loff_t *data)
{
	int 				ret;
	char 				kbuf[256];
	size_t 			len = 0;
	unsigned int 			cmd,p1,p2,p3,p4,p5,p6,p7;
	struct freqhopping_ioctl 	fh_ctl;
	
	p1=0;
	p2=0;
	p3=0;
	p4=0;
	p5=0;
	p6=0;
	p7=0;

	FH_MSG("EN: %s",__func__);
	
	len = min(count, (sizeof(kbuf)-1));

	if (count == 0)return -1;
	if(count > 255)count = 255;

	ret = copy_from_user(kbuf, buffer, count);
	if (ret < 0)return -1;
	
	kbuf[count] = '\0';

	sscanf(kbuf, "%x %x %x %x %x %x %x %x", &cmd, &p1, &p2, &p3, &p4, &p5, &p6, &p7);

	//ccyeh fh_ctl.opcode = p1;
	fh_ctl.pll_id  = p2;
	//ccyeh removed fh_ctl.ssc_setting_id = p3;
	fh_ctl.ssc_setting.dds		= p3;
	fh_ctl.ssc_setting.df		= p4;
	fh_ctl.ssc_setting.dt		= p5;
	fh_ctl.ssc_setting.upbnd	= p6;
	fh_ctl.ssc_setting.lowbnd	= p7;
	fh_ctl.ssc_setting.freq		= 0;
	

	if (cmd < FH_CMD_INTERNAL_MAX_CMD) {
		mt_freqhopping_ioctl(NULL,cmd,(unsigned long)(&fh_ctl));
	}
	else {
		FH_MSG("CMD error!");		
	}
		
	
	
	return count;
}

static int freqhopping_debug_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, freqhopping_debug_proc_read, NULL);
}

static int freqhopping_dramc_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, g_p_fh_hal_drv->proc.dramc_read, NULL);
}
static ssize_t freqhopping_dramc_proc_write(struct file *file, const char *buffer, size_t count, loff_t *data)
{
    return (ssize_t)(g_p_fh_hal_drv->proc.dramc_write(file, buffer, count, data));
}

static int freqhopping_dvfs_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, g_p_fh_hal_drv->proc.dvfs_read, NULL);
}
static ssize_t freqhopping_dvfs_proc_write(struct file *file, const char *buffer, size_t count, loff_t *data)
{
    return (ssize_t)(g_p_fh_hal_drv->proc.dvfs_write(file, buffer, count, data));
}

static int freqhopping_dumpregs_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, g_p_fh_hal_drv->proc.dumpregs_read, NULL);
}

static int freqhopping_status_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, freqhopping_status_proc_read, NULL);
}
static int freqhopping_userdefine_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, freqhopping_userdefine_proc_read, NULL);
}

static const struct file_operations freqhopping_debug_fops = {
	.owner		= THIS_MODULE,	
	.open       = freqhopping_debug_proc_open,
	.read       = seq_read, 
	.write      = freqhopping_debug_proc_write,
};
static const struct file_operations dramc_fops = {
	.owner		= THIS_MODULE,	
	.open       = freqhopping_dramc_proc_open,
	.read       = seq_read, 
	.write      = freqhopping_dramc_proc_write,
};
static const struct file_operations dvfs_fops = {
	.owner		= THIS_MODULE,	
	.open       = freqhopping_dvfs_proc_open,
	.read       = seq_read, 
	.write      = freqhopping_dvfs_proc_write,
};
static const struct file_operations dumpregs_fops = {
	.owner		= THIS_MODULE,	
	.open       = freqhopping_dumpregs_proc_open,
	.read       = seq_read, 
};
static const struct file_operations status_fops = {
	.owner			= THIS_MODULE,
	.open				=	freqhopping_status_proc_open,
	.read       = seq_read, 
	.write      = freqhopping_status_proc_write,
};
static const struct file_operations userdef_fops = {
	.owner		= THIS_MODULE,	
	.open       = freqhopping_userdefine_proc_open,
	.read       = seq_read, 
	.write      = freqhopping_userdefine_proc_write,
};

static int freqhopping_debug_proc_init(void) 
{   	
	struct proc_dir_entry *prDebugEntry;
	struct proc_dir_entry *prDramcEntry;
	struct proc_dir_entry *prDumpregEntry;
	struct proc_dir_entry *prStatusEntry;
	struct proc_dir_entry *prUserdefEntry;
	struct proc_dir_entry *fh_proc_dir = NULL;
	
	//TODO: check the permission!!

	FH_MSG("EN: %s",__func__);
	
	fh_proc_dir = proc_mkdir("freqhopping", NULL);
	if (!fh_proc_dir){
		FH_MSG("proc_mkdir fail!");
		return 1;
	}
	else{


		/* /proc/freqhopping/freqhopping_debug */
		//prDebugEntry = create_proc_entry("freqhopping_debug",  S_IRUGO | S_IWUSR | S_IWGRP, fh_proc_dir);
		prDebugEntry = proc_create("freqhopping_debug",  S_IRUGO | S_IWUSR | S_IWGRP, fh_proc_dir, &freqhopping_debug_fops);
		if(prDebugEntry)
		{
			//prDebugEntry->read_proc  = freqhopping_debug_proc_read;
			//prDebugEntry->write_proc = freqhopping_debug_proc_write;
			FH_MSG("[%s]: successfully create /proc/freqhopping_debug", __func__);
		}else{
			FH_MSG("[%s]: failed to create /proc/freqhopping/freqhopping_debug", __func__);
			return 1;
		}
		
	
		/* /proc/freqhopping/dramc */
		//prDramcEntry = create_proc_entry("dramc",  S_IRUGO | S_IWUSR | S_IWGRP, fh_proc_dir);
		prDramcEntry = proc_create("dramc",  S_IRUGO | S_IWUSR | S_IWGRP, fh_proc_dir, &dramc_fops);
		if(prDramcEntry)
		{
			//prDramcEntry->read_proc  = g_p_fh_hal_drv->proc.dramc_read;
			//prDramcEntry->write_proc = g_p_fh_hal_drv->proc.dramc_write;
			FH_MSG("[%s]: successfully create /proc/freqhopping/prDramcEntry", __func__);
		}else{
			FH_MSG("[%s]: failed to create /proc/freqhopping/prDramcEntry", __func__);
			return 1;
		}
		/* /proc/freqhopping/dvfs */
		//prDramcEntry = create_proc_entry("dvfs",  S_IRUGO | S_IWUSR | S_IWGRP, fh_proc_dir);
		prDramcEntry = proc_create("dvfs",  S_IRUGO | S_IWUSR | S_IWGRP, fh_proc_dir, &dvfs_fops);
		if(prDramcEntry)
		{
			//prDramcEntry->read_proc  = g_p_fh_hal_drv->proc.dvfs_read;
			//prDramcEntry->write_proc = g_p_fh_hal_drv->proc.dvfs_write;
			FH_MSG("[%s]: successfully create /proc/freqhopping/dvfs", __func__);
		}else{
			FH_MSG("[%s]: failed to create /proc/freqhopping/dvfs", __func__);
			return 1;
		}


		/* /proc/freqhopping/dumpregs */
		//prDumpregEntry = create_proc_entry("dumpregs",  S_IRUGO | S_IWUSR | S_IWGRP, fh_proc_dir);
		prDumpregEntry = proc_create("dumpregs",  S_IRUGO | S_IWUSR | S_IWGRP, fh_proc_dir, &dumpregs_fops);
		if(prDumpregEntry)
		{
			//prDumpregEntry->read_proc  =  g_p_fh_hal_drv->proc.dumpregs_read;
			//prDumpregEntry->write_proc = NULL;
			FH_MSG("[%s]: successfully create /proc/freqhopping/dumpregs", __func__);
		}else{
			FH_MSG("[%s]: failed to create /proc/freqhopping/dumpregs", __func__);
			return 1;
		}


		/* /proc/freqhopping/status */
		//prStatusEntry = create_proc_entry("status",  S_IRUGO | S_IWUSR | S_IWGRP, fh_proc_dir);
		prStatusEntry = proc_create("status",  S_IRUGO | S_IWUSR | S_IWGRP, fh_proc_dir, &status_fops);
		if(prStatusEntry)
		{
			//prStatusEntry->read_proc  = freqhopping_status_proc_read;
			//prStatusEntry->write_proc = freqhopping_status_proc_write;
			FH_MSG("[%s]: successfully create /proc/freqhopping/status", __func__);
		}else{
			FH_MSG("[%s]: failed to create /proc/freqhopping/status", __func__);
			return 1;
		}
		

		/* /proc/freqhopping/userdefine */
		//prUserdefEntry = create_proc_entry("userdef",  S_IRUGO | S_IWUSR | S_IWGRP, fh_proc_dir);
		prUserdefEntry = proc_create("userdef",  S_IRUGO | S_IWUSR | S_IWGRP, fh_proc_dir, &userdef_fops);
		if(prUserdefEntry)
		{
			//prUserdefEntry->read_proc  = freqhopping_userdefine_proc_read;
			//prUserdefEntry->write_proc = freqhopping_userdefine_proc_write;
			FH_MSG("[%s]: successfully create /proc/freqhopping/userdef", __func__);
		}else{
			FH_MSG("[%s]: failed to create /proc/freqhopping/userdef", __func__);
			return 1;
		}

#if 0// MT_FH_CLK_GEN
		/* /proc/freqhopping/clkgen */
		prUserdefEntry = create_proc_entry("clkgen",  S_IRUGO | S_IWUSR | S_IWGRP, fh_proc_dir);
		if(prUserdefEntry)
		{
			prUserdefEntry->read_proc  = g_p_fh_hal_drv->proc.clk_gen_read;
			prUserdefEntry->write_proc = g_p_fh_hal_drv->proc.clk_gen_write;
			FH_MSG("[%s]: successfully create /proc/freqhopping/clkgen", __func__);
		}else{
			FH_MSG("[%s]: failed to create /proc/freqhopping/clkgen", __func__);
			return 1;
		}
#endif //MT_FH_CLK_GEN		
	}

	return 0 ;
}

int freqhopping_config(unsigned int pll_id, unsigned long vco_freq, unsigned int enable)
{
	struct freqhopping_ioctl 	fh_ctl;
	unsigned int			fh_status;
	unsigned long 			flags=0;
	unsigned int			skip_flag=0;
	
	FH_MSG("conf() id: %d f: %d, e: %d",(int)pll_id, (int)vco_freq, (int)enable);
	
	if( (g_p_fh_hal_drv->mt_fh_get_init()) == 0){
		FH_MSG("Not init yet, init first.");
		return 1;
	}				
	
	g_p_fh_hal_drv->mt_fh_lock(&flags);

	//backup
	fh_status = g_fh_drv_pll[pll_id].fh_status;

	g_fh_drv_pll[pll_id].curr_freq = vco_freq;
	g_fh_drv_pll[pll_id].pll_status = (enable > 0) ? FH_PLL_ENABLE:FH_PLL_DISABLE;


	//prepare freqhopping_ioctl
	fh_ctl.pll_id = pll_id;
	
	if(g_fh_drv_pll[pll_id].fh_status != FH_FH_DISABLE){
		//FH_MSG("+fh");
		g_p_fh_hal_drv->mt_fh_hal_ctrl(&fh_ctl,enable);
		
	}
	else{
		skip_flag = 1;
		//FH_MSG("-fh,skip");		
	}
	
	//restore
	g_fh_drv_pll[pll_id].fh_status = fh_status;

	g_p_fh_hal_drv->mt_fh_unlock(&flags);
	
	if(skip_flag)
		FH_MSG("-fh,skip");		
  	
	return 0;
}
EXPORT_SYMBOL(freqhopping_config);


int mt_l2h_mempll(void)
{
	return(g_p_fh_hal_drv->mt_l2h_mempll());
}
EXPORT_SYMBOL(mt_l2h_mempll);

int mt_h2l_mempll(void)
{
	return(g_p_fh_hal_drv->mt_h2l_mempll());
}
EXPORT_SYMBOL(mt_h2l_mempll);

int mt_dfs_armpll(unsigned int current_freq, unsigned int target_freq)
{
	return(g_p_fh_hal_drv->mt_dfs_armpll(current_freq, target_freq));
}
EXPORT_SYMBOL(mt_dfs_armpll);
int mt_dfs_mmpll(unsigned int target_freq)
{
	return(g_p_fh_hal_drv->mt_dfs_mmpll(target_freq));
}
EXPORT_SYMBOL(mt_dfs_mmpll);
int mt_dfs_vencpll(unsigned int target_freq)
{
	return(g_p_fh_hal_drv->mt_dfs_vencpll(target_freq));
}
EXPORT_SYMBOL(mt_dfs_vencpll);

int mt_is_support_DFS_mode(void)
{
	return(g_p_fh_hal_drv->mt_is_support_DFS_mode());
}
EXPORT_SYMBOL(mt_is_support_DFS_mode);
int mt_l2h_dvfs_mempll(void)
{
	return(g_p_fh_hal_drv->mt_l2h_dvfs_mempll());
}
EXPORT_SYMBOL(mt_l2h_dvfs_mempll);

int mt_h2l_dvfs_mempll(void)
{
	return(g_p_fh_hal_drv->mt_h2l_dvfs_mempll());
}
EXPORT_SYMBOL(mt_h2l_dvfs_mempll);
int mt_fh_dram_overclock(int clk)
{
	return (g_p_fh_hal_drv->mt_dram_overclock(clk));
}
EXPORT_SYMBOL(mt_fh_dram_overclock);

int mt_fh_get_dramc(void)
{
	return (g_p_fh_hal_drv->mt_get_dramc());
}
EXPORT_SYMBOL(mt_fh_get_dramc);

void mt_freqhopping_init(void)
{
	g_p_fh_hal_drv 		= mt_get_fh_hal_drv();

	g_p_fh_hal_drv->mt_fh_hal_init();

	g_fh_drv_pll   		= g_p_fh_hal_drv->fh_pll;
	g_fh_drv_usr_def	= g_p_fh_hal_drv->fh_usrdef;
	g_drv_pll_count		= g_p_fh_hal_drv->pll_cnt;

	freqhopping_debug_proc_init(); 
	platform_driver_register(&freqhopping_driver);
	
	mt_freqhopping_pll_init(); //TODO_HAL: wait for clkmgr to invoke this function
}
EXPORT_SYMBOL(mt_freqhopping_init);

void mt_freqhopping_pll_init(void)
{
	g_p_fh_hal_drv->mt_fh_default_conf();
// move to /mediatek/platform/prj for prj config.
#if 0	
	FH_MSG("EN: %s",__func__);
#if (MEMPLL_SSC)
	freqhopping_config(FH_MEM_PLL, 266000, true); //Enable MEMPLL SSC
#endif
	freqhopping_config(FH_MM_PLL, 1188000, true); //TODO: test only
	freqhopping_config(FH_VENC_PLL, 1200000, true); //TODO: test only
#if (MAINPLL_SSC)	
	freqhopping_config(FH_MAIN_PLL, 1612000, true); //TODO: test only
#endif	
	freqhopping_config(FH_MSDC_PLL, 1599000, true); //TODO: test only
#endif
}
EXPORT_SYMBOL(mt_freqhopping_pll_init);
