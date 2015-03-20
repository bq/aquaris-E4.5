#ifndef __DUMCHAR_H__
#define __DUMCHAR_H__


#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/mtd/mtd.h>
#include <linux/semaphore.h>
#include <linux/mmc/sd_misc.h>
/*
 * Macros to help debugging
 */
#define DUMCHAR_DEBUG
//#undef DUMCHAR_DEBUG             /* undef it, just in case */

#ifdef DUMCHAR_DEBUG
#define DDEBUG(fmt, args...) printk( KERN_DEBUG "dumchar_debug: " fmt, ## args)
#else
#define DDEBUG(fmt, args...) 
#endif


#define DUMCHAR_MAJOR        0   /* dynamic major by default */
#define MAX_SD_BUFFER		(512)
#define ALIE_LEN		512


//#define PrintBuff 1

struct dumchar_dev {
	char *dumname;	//nvram boot userdata...
	char actname[64];	//full act name /dev/mt6573_sd0 /dev/mtd/mtd1	
	struct semaphore sem;     /* Mutual exclusion */
	dev_type type; //nand device or emmc device?
	unsigned long long size; //partition size
	struct cdev cdev;	
	Region region; //for emmc
	unsigned long long start_address; //for emmc
	unsigned int mtd_index;	 //for nand
};

struct Region_Info {
	Region region;
	unsigned long long	size_Byte;
};

struct file_obj {
	struct file *act_filp; 
	int index; //index in dumchar_dev arry
};

#define REGION_NUM 						8
#define EXT_CSD_BOOT_SIZE_MULT          226 /* R */
#define EXT_CSD_RPMB_SIZE_MULT          168 /* R */

#define	MSDC_RAW_DEVICE					"/dev/misc-sd"

#define mtd_for_each_device(mtd)			\
	for ((mtd) = __mtd_next_device(0);		\
	     (mtd) != NULL;				\
	     (mtd) = __mtd_next_device(mtd->index + 1))

#endif /*__DUMCHAR_H__ */
#ifndef MTK_NEW_COMBO_EMMC_SUPPORT
#define EMMC_PART_BOOT1		(BOOT_1)
#define EMMC_PART_BOOT2		(BOOT_2)
#define EMMC_PART_RPMB		(RPMB)
#define EMMC_PART_GP1 		(GP_1)
#define EMMC_PART_GP2 		(GP_2)
#define EMMC_PART_GP3 		(GP_3)
#define EMMC_PART_GP4 		(GP_4)
#define EMMC_PART_USER 		(USER)
#define NAND_PART_USER 		(USER)

#endif
