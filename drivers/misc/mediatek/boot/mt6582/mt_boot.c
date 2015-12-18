#define pr_fmt(fmt) "["KBUILD_MODNAME"] " fmt
#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/kfifo.h>

#include <linux/firmware.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>

#include <mach/mt_boot.h>
#include <mach/mt_reg_base.h>
#include <mach/mt_typedefs.h>
#if defined(CONFIG_TRUSTONIC_TEE_SUPPORT)
#include <mach/mtk_boot_share_page.h>
#endif

/* hardware version register */
#define VER_BASE            (DEVINFO_BASE)
#define APHW_CODE           (VER_BASE)
#define APHW_SUBCODE        (VER_BASE + 0x04)
#define APHW_VER            (VER_BASE + 0x08)
#define APSW_VER            (VER_BASE + 0x0C)

/* this vairable will be set by mt_fixup.c */
META_COM_TYPE g_meta_com_type = META_UNKNOWN_COM;
unsigned int g_meta_com_id = 0;

struct meta_driver {
    struct device_driver driver;
    const struct platform_device_id *id_table;
};

static struct meta_driver meta_com_type_info = {
    .driver  = {
        .name = "meta_com_type_info",
        .bus = &platform_bus_type,
        .owner = THIS_MODULE,
    },
    .id_table = NULL,
};

static struct meta_driver meta_com_id_info = {
    .driver = {
        .name = "meta_com_id_info",
        .bus = &platform_bus_type,
        .owner = THIS_MODULE,
    },
    .id_table = NULL,
};


/* return hardware version */
unsigned int get_chip_code(void)
{     
#if defined(CONFIG_TRUSTONIC_TEE_SUPPORT)
    return *(unsigned int*)(BOOT_SHARE_BASE+BOOT_SHARE_DEV_INFO_OFST+0x00000000);    
#else     
    return DRV_Reg32(APHW_CODE);
#endif
}

unsigned int get_chip_hw_ver_code(void)
{   
#if defined(CONFIG_TRUSTONIC_TEE_SUPPORT)
    return *(unsigned int*)(BOOT_SHARE_BASE+BOOT_SHARE_DEV_INFO_OFST+0x00000008);    
#else      
    return DRV_Reg32(APHW_VER);
#endif
}

unsigned int get_chip_sw_ver_code(void)
{  
#if defined(CONFIG_TRUSTONIC_TEE_SUPPORT)
    return *(unsigned int*)(BOOT_SHARE_BASE+BOOT_SHARE_DEV_INFO_OFST+0x0000000c);  
#else     
    return DRV_Reg32(APSW_VER);
#endif
}

unsigned int get_chip_hw_subcode(void)
{
#if defined(CONFIG_TRUSTONIC_TEE_SUPPORT)
    return *(unsigned int*)(BOOT_SHARE_BASE+BOOT_SHARE_DEV_INFO_OFST+0x00000004);   
#else   
    return DRV_Reg32(APHW_SUBCODE);
#endif
}

unsigned int mt_get_chip_id(void)
{
    unsigned int chip_id = get_chip_code();
    /*convert id if necessary*/
    return chip_id;
}

CHIP_SW_VER mt_get_chip_sw_ver(void)
{
    return (CHIP_SW_VER)get_chip_sw_ver_code();
}

bool com_is_enable(void)  // usb android will check whether is com port enabled default. in normal boot it is default enabled. 
{
    if(get_boot_mode() == NORMAL_BOOT) {
        return false;
    } else {
        return true;
    }
}

void set_meta_com(META_COM_TYPE type, unsigned int id)
{
    g_meta_com_type = type;
    g_meta_com_id = id;
}

META_COM_TYPE get_meta_com_type(void)
{
    return g_meta_com_type;
}

unsigned int get_meta_com_id(void)
{
    return g_meta_com_id;
}

static ssize_t meta_com_type_show(struct device_driver *driver, char *buf)
{
    return sprintf(buf, "%d\n", g_meta_com_type);
}

static ssize_t meta_com_type_store(struct device_driver *driver, const char *buf, size_t count)
{
    /*Do nothing*/
    return count;
}

DRIVER_ATTR(meta_com_type_info, 0644, meta_com_type_show, meta_com_type_store);


static ssize_t meta_com_id_show(struct device_driver *driver, char *buf)
{
    return sprintf(buf, "%d\n", g_meta_com_id);
}

static ssize_t meta_com_id_store(struct device_driver *driver, const char *buf, size_t count)
{
    /*Do nothing*/
    return count;
}

DRIVER_ATTR(meta_com_id_info, 0644, meta_com_id_show, meta_com_id_store);


static int __init boot_mod_init(void)
{
    int ret;
    BOOTMODE bm = get_boot_mode();
    
    if(bm == META_BOOT || bm == ADVMETA_BOOT || bm == ATE_FACTORY_BOOT || bm == FACTORY_BOOT) {
        /* register driver and create sysfs files */
        ret = driver_register(&meta_com_type_info.driver);
        if (ret) {
            pr_warn("fail to register META COM TYPE driver\n");
        }
        ret = driver_create_file(&meta_com_type_info.driver, &driver_attr_meta_com_type_info);
        if (ret) {
            pr_warn("fail to create META COM TPYE sysfs file\n");
        }

        ret = driver_register(&meta_com_id_info.driver);
        if (ret) {
            pr_warn("fail to register META COM ID driver\n");
        }
        ret = driver_create_file(&meta_com_id_info.driver, &driver_attr_meta_com_id_info);
        if (ret) {
            pr_warn("fail to create META COM ID sysfs file\n");
        }
    }    
    
    return 0;
}

static void __exit boot_mod_exit(void)
{
}

module_init(boot_mod_init);
module_exit(boot_mod_exit);
MODULE_DESCRIPTION("MTK Boot Information Querying Driver");
MODULE_LICENSE("Proprietary");
EXPORT_SYMBOL(mt_get_chip_id);
EXPORT_SYMBOL(mt_get_chip_sw_ver);
