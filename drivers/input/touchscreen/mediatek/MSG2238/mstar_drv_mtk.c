////////////////////////////////////////////////////////////////////////////////
// (c) MStar Semiconductor, Inc. 2006-2014
////////////////////////////////////////////////////////////////////////////////

/**
 *
 * @file    mstar_drv_mtk.c
 *
 * @brief   This file defines the interface of touch screen
 *
 * @version v2.3.0.0
 *
 */

/*=============================================================*/
// INCLUDE FILE
/*=============================================================*/

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/hwmsen_helper.h>
//#include <linux/hw_module_info.h>

#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/namei.h>
#include <linux/vmalloc.h>

#include <mach/mt_pm_ldo.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_boot.h>
#include <mach/mt_gpio.h>

#include <cust_eint.h>

#include "tpd.h"
#include "cust_gpio_usage.h"

#include "mstar_drv_platform_interface.h"

/*=============================================================*/
// CONSTANT VALUE DEFINITION
/*=============================================================*/

#define MSG_TP_IC_NAME "msg2xxx" //"msg21xxA" or "msg22xx" or "msg26xxM" /* Please define the mstar touch ic name based on the mutual-capacitive ic or self capacitive ic that you are using */
#define I2C_BUS_ID   (0)       // i2c bus id : 0 or 1

#define TPD_OK (0)

/*=============================================================*/
// EXTERN VARIABLE DECLARATION
/*=============================================================*/

#ifdef CONFIG_TP_HAVE_KEY
extern const int g_TpVirtualKey[];

#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
extern const int g_TpVirtualKeyDimLocal[][4];
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
#endif //CONFIG_TP_HAVE_KEY

extern struct tpd_device *tpd;
#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
int g_gesture_enable = 0;
#endif
/*=============================================================*/
// LOCAL VARIABLE DEFINITION
/*=============================================================*/

/*
#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
static int tpd_wb_start_local[TPD_WARP_CNT] = TPD_WARP_START;
static int tpd_wb_end_local[TPD_WARP_CNT] = TPD_WARP_END;
#endif

#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
static int tpd_calmat_local[8] = TPD_CALIBRATION_MATRIX;
static int tpd_def_calmat_local[8] = TPD_CALIBRATION_MATRIX;
#endif
*/
struct i2c_client *g_I2cClient = NULL;

//static int boot_mode = 0;

/*=============================================================*/
// FUNCTION DECLARATION
/*=============================================================*/

/*=============================================================*/
// FUNCTION DEFINITION
/*=============================================================*/

/* probe function is used for matching and initializing input device */
static int/* __devinit*/ tpd_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    TPD_DMESG("TPD probe\n");   
    
    if (client == NULL)
    {
        TPD_DMESG("i2c client is NULL\n");
        return -1;
    }
    g_I2cClient = client;
    
    MsDrvInterfaceTouchDeviceSetIicDataRate(g_I2cClient, 100000); // 100 KHZ

    MsDrvInterfaceTouchDeviceProbe(g_I2cClient, id);

    tpd_load_status = 1;

    TPD_DMESG("TPD probe done\n");
    
    return TPD_OK;   
}

static int tpd_detect(struct i2c_client *client, struct i2c_board_info *info) 
{
    strcpy(info->type, TPD_DEVICE);    
//    strcpy(info->type, MSG_TP_IC_NAME);
    
    return TPD_OK;
}

static int tpd_remove(struct i2c_client *client)
{   
    TPD_DEBUG("TPD removed\n");
    
    MsDrvInterfaceTouchDeviceRemove(client);
    
    return TPD_OK;
}

static struct i2c_board_info __initdata i2c_tpd = {I2C_BOARD_INFO(MSG_TP_IC_NAME, (0x4C>>1))};

/* The I2C device list is used for matching I2C device and I2C device driver. */
static const struct i2c_device_id tpd_device_id[] =
{
    {MSG_TP_IC_NAME, 0},
    {}, /* should not omitted */ 
};

MODULE_DEVICE_TABLE(i2c, tpd_device_id);

static struct i2c_driver tpd_i2c_driver = {
    .driver = {
        .name = MSG_TP_IC_NAME,
    },
    .probe = tpd_probe,
    .remove =tpd_remove,
    .id_table = tpd_device_id,
    .detect = tpd_detect,
};

static int tpd_local_init(void)
{  
    TPD_DMESG("TPD init device driver (Built %s @ %s)\n", __DATE__, __TIME__);
/*
    // Software reset mode will be treated as normal boot
    boot_mode = get_boot_mode();
    if (boot_mode == 3) 
    {
        boot_mode = NORMAL_BOOT;    
    }
*/
    if (i2c_add_driver(&tpd_i2c_driver) != 0)
    {
        TPD_DMESG("unable to add i2c driver.\n");
         
        return -1;
    }
    
    if (tpd_load_status == 0) 
    {
        TPD_DMESG("add error touch panel driver.\n");

        i2c_del_driver(&tpd_i2c_driver);
        return -1;
    }

#ifdef CONFIG_TP_HAVE_KEY
#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE     
    // initialize tpd button data
    tpd_button_setting(3, g_TpVirtualKey, g_TpVirtualKeyDimLocal); //MAX_KEY_NUM
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE  
#endif //CONFIG_TP_HAVE_KEY  

/*
#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))    
    TPD_DO_WARP = 1;
    memcpy(tpd_wb_start, tpd_wb_start_local, TPD_WARP_CNT*4);
    memcpy(tpd_wb_end, tpd_wb_start_local, TPD_WARP_CNT*4);
#endif 

#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
    memcpy(tpd_calmat, tpd_def_calmat_local, 8*4);
    memcpy(tpd_def_calmat, tpd_def_calmat_local, 8*4);    
#endif  
*/
    TPD_DMESG("TPD init done %s, %d\n", __FUNCTION__, __LINE__);  
        tpd_type_cap = 1;//add by sen.luo 修改标志为电容屏
    return TPD_OK; 
}

static void tpd_resume(struct early_suspend *h)
{
    TPD_DMESG("TPD wake up\n");

#ifdef __TP_PROXIMITY_SUPPORT__
    if(g_call_state)
    {
        return;
    }
    else
    {
        g_bPsSensorOpen =0;
    }
    if(g_bPsSensorOpen == 1 && (g_bSuspend))
    {
        DBG("msg sensor resume in calling tp no need to resume\n");
        return 0;
    }
    g_bSuspend = 0;

#endif
    
    MsDrvInterfaceTouchDeviceResume(h);
    
    TPD_DMESG("TPD wake up done\n");
}

static void tpd_suspend(struct early_suspend *h)
{
    TPD_DMESG("TPD enter sleep\n");

    
#ifdef __TP_PROXIMITY_SUPPORT__
    if(g_call_state)
    {
        return;
    }
    else
    {
        g_bPsSensorOpen=0;
    }
    if(g_bPsSensorOpen == 1)
    {
        DBG("msg suspend in calling tp no need to suspend\n");
        return 0;
    }
    g_bSuspend = 1;

#endif

    MsDrvInterfaceTouchDeviceSuspend(h);

    TPD_DMESG("TPD enter sleep done\n");
} 

//add by sen.luo
extern const char* DISP_GetLCMId(void);

#define MSG2238_FM_VERSION_PATH               "/sys/devices/virtual/ms-touchscreen-msg20xx/device/version"
static ssize_t show_chipinfo(struct device *dev,struct device_attribute *attr, char *buf)
{
    s32 ret = 0;
    int fd=-1;
    int err = 0;
	ssize_t size;
    mm_segment_t fs;
    loff_t pos = 0;
    struct file *fd_file = NULL;
    struct file *flp = NULL;
    // int fd_file = -1;
    char cmd[] = {"1"};
    char fmverison[10]="unKnown";
    char verndorName[128]="unknown";
    const char* lcd_name_txd="ili9806e_fwvga_dsi_vdo_txd";
    const char* lcd_name_dj="nt35512_fwvga_dsi_vdo_dijing";	
    //read the fw version	

    fd_file = filp_open(MSG2238_FM_VERSION_PATH, O_CREAT|O_RDWR|O_TRUNC, 0777);
    if (IS_ERR(fd_file))
    {
        printk("ronson fail to open\n");
        return -1;
    }
    
    fs = get_fs();
    set_fs(KERNEL_DS);
    
    vfs_write(fd_file, cmd, sizeof(cmd), &pos);
    
    filp_close(fd_file, NULL);
    
    set_fs(fs);

    flp = filp_open(MSG2238_FM_VERSION_PATH, O_RDWR, S_IRUSR);
    if (IS_ERR(flp))
    {
        return -EINVAL;
    }
    else
    {
        pos = 0;
        fs = get_fs();
        set_fs(KERNEL_DS);
        vfs_read(flp, fmverison, sizeof(fmverison), &pos);
        filp_close(flp, NULL);	
        set_fs(fs);
    }
    //end
    if(!strcmp(lcd_name_txd,DISP_GetLCMId()))
    {
        size = sprintf(buf, "IC:msg2238 FW_ver:%s\nVENDOR:hrc\n",fmverison);
    }
    else if(!strcmp(lcd_name_dj,DISP_GetLCMId()))
    {
        size = sprintf(buf, "IC:msg2238 FW_ver:%s\nVENDOR:dijing\n",fmverison);
    }
    else
    {
        size = sprintf(buf, "IC:msg2238 FW_ver:%s\nVENDOR:%s\n",fmverison,verndorName);
    }
	return size;
}

static DEVICE_ATTR(chipinfo, 0444, show_chipinfo, NULL);

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
static ssize_t show_control_double_tap(struct device *dev,struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "gestrue state:%s \n",g_gesture_enable==0?"Disable":"Enable");
}

static ssize_t store_control_double_tap(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    char *pvalue = NULL;
    if(buf != NULL && size != 0)
    {
        printk("[msg2238]store_control_double_tap buf is %s and size is %d \n",buf,size);
        g_gesture_enable = simple_strtoul(buf,&pvalue,16);
        printk("[msg2238] store_control_double_tap : %s \n",g_gesture_enable==0?"Disable":"Enable");
    }        
    return size;
}


static DEVICE_ATTR(control_double_tap, 0664, show_control_double_tap, store_control_double_tap);
#endif

static struct device_attribute *msg2xxx_attrs[] =
{
	&dev_attr_chipinfo,
#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
	&dev_attr_control_double_tap,
#endif
};
//end by sen.luo



static struct tpd_driver_t tpd_device_driver = {
     .tpd_device_name = MSG_TP_IC_NAME,
     .tpd_local_init = tpd_local_init,
     .suspend = tpd_suspend,
     .resume = tpd_resume,
#ifdef CONFIG_TP_HAVE_KEY
#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
     .tpd_have_button = 1,
#else
     .tpd_have_button = 0,
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE        
#endif //CONFIG_TP_HAVE_KEY     
//add by sen.luo
.attrs = {
		.attr = msg2xxx_attrs, 
		.num  = ARRAY_SIZE(msg2xxx_attrs),
	},
//end by sen.luo
};

static int __init tpd_driver_init(void) 
{
    TPD_DMESG("touch panel driver init\n");

    i2c_register_board_info(I2C_BUS_ID, &i2c_tpd, 1);
    if (tpd_driver_add(&tpd_device_driver) < 0)
    {
        TPD_DMESG("TPD add driver failed\n");
    }
     
    return 0;
}
 
static void __exit tpd_driver_exit(void) 
{
    TPD_DMESG("touch panel driver exit\n");
    
    tpd_driver_remove(&tpd_device_driver);
}

module_init(tpd_driver_init);
module_exit(tpd_driver_exit);
MODULE_LICENSE("GPL");
