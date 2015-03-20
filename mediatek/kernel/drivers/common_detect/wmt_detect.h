#ifndef _WMT_DETECT_H_
#define _WMT_DETECT_H_

#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <asm/current.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/delay.h>
/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
#ifdef MTK_WCN_REMOVE_KERNEL_MODULE
#define MTK_WCN_REMOVE_KO 1
#else
#define MTK_WCN_REMOVE_KO 0
#endif

#include "sdio_detect.h"
#include "wmt_detect_pwr.h"
#include <mach/mtk_wcn_cmb_stub.h>


#define WMT_DETECT_LOG_LOUD    4
#define WMT_DETECT_LOG_DBG     3
#define WMT_DETECT_LOG_INFO    2
#define WMT_DETECT_LOG_WARN    1
#define WMT_DETECT_LOG_ERR     0

extern unsigned int gWmtDetectDbgLvl;

#define WMT_DETECT_LOUD_FUNC(fmt, arg...)   if (gWmtDetectDbgLvl >= WMT_DETECT_LOG_LOUD) { printk(KERN_WARNING DFT_TAG"[L]%s:"  fmt, __FUNCTION__ ,##arg);}
#define WMT_DETECT_DBG_FUNC(fmt, arg...)    if (gWmtDetectDbgLvl >= WMT_DETECT_LOG_DBG) { printk(KERN_WARNING DFT_TAG"[D]%s:"  fmt, __FUNCTION__ ,##arg);}
#define WMT_DETECT_INFO_FUNC(fmt, arg...)   if (gWmtDetectDbgLvl >= WMT_DETECT_LOG_INFO) { printk(KERN_WARNING DFT_TAG"[I]%s:"  fmt, __FUNCTION__ ,##arg);}
#define WMT_DETECT_WARN_FUNC(fmt, arg...)   if (gWmtDetectDbgLvl >= WMT_DETECT_LOG_WARN) { printk(KERN_WARNING DFT_TAG"[W]%s(%d):"  fmt, __FUNCTION__ , __LINE__, ##arg);}
#define WMT_DETECT_ERR_FUNC(fmt, arg...)    if (gWmtDetectDbgLvl >= WMT_DETECT_LOG_ERR) { printk(KERN_WARNING DFT_TAG"[E]%s(%d):"  fmt, __FUNCTION__ , __LINE__, ##arg);}

#define WMT_IOC_MAGIC        		'w'
#define COMBO_IOCTL_GET_CHIP_ID  	_IOR(WMT_IOC_MAGIC, 0, int)
#define COMBO_IOCTL_SET_CHIP_ID  	_IOW(WMT_IOC_MAGIC, 1, int)
#define COMBO_IOCTL_EXT_CHIP_DETECT _IOR(WMT_IOC_MAGIC, 2, int)
#define COMBO_IOCTL_GET_SOC_CHIP_ID  	_IOR(WMT_IOC_MAGIC, 3, int)
#define COMBO_IOCTL_DO_MODULE_INIT  	_IOR(WMT_IOC_MAGIC, 4, int)
#define COMBO_IOCTL_MODULE_CLEANUP  	_IOR(WMT_IOC_MAGIC, 5, int)





typedef unsigned int UINT32;
/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************/
extern int wmt_detect_ext_chip_detect(void);
#endif
