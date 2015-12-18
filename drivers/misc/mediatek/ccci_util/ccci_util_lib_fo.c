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
#include <linux/seq_file.h>
#include <linux/memblock.h>
#include <asm/memblock.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_fdt.h>
#endif

#include <asm/setup.h>
#include <asm/atomic.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_boot_common.h>

#include <mach/ccci_config.h>
#include <mach/mt_ccci_common.h>

#include "ccci_util_log.h"

//======================================================
// DFO support section
//======================================================
typedef struct fos_item  // Feature Option Setting
{
	char         *name;
	volatile int value;
}fos_item_t;
// DFO table
// TODO : the following macro can be removed sometime
// MD1
#ifdef  CONFIG_MTK_ENABLE_MD1
#define MTK_MD1_EN	(1)
#else
#define MTK_MD1_EN	(0)
#endif

#ifdef CONFIG_MTK_MD1_SUPPORT
#define MTK_MD1_SUPPORT	(CONFIG_MTK_MD1_SUPPORT)
#else
#define MTK_MD1_SUPPORT	(5)
#endif

#ifdef CONFIG_MD1_SIZE
#define MD1_SIZE (CONFIG_MD1_SIZE)
#else
#define MD1_SIZE	(80*1024*1024)
#endif

#ifdef CONFIG_MD1_SMEM_SIZE
#define MD1_SMEM_SIZE (CONFIG_MD1_SMEM_SIZE)
#else
#define MD1_SMEM_SIZE	(2*1024*1024)
#endif

// MD2
#ifdef  CONFIG_MTK_ENABLE_MD2
#define MTK_MD2_EN	(1)
#else
#define MTK_MD2_EN	(0)
#endif

#ifdef CONFIG_MTK_MD2_SUPPORT
#define MTK_MD2_SUPPORT	(CONFIG_MTK_MD2_SUPPORT)
#else
#define MTK_MD2_SUPPORT	(1)
#endif

#ifdef CONFIG_MD2_SIZE
#define MD2_SIZE (CONFIG_MD2_SIZE)
#else
#define MD2_SIZE	(16*1024*1024)
#endif

#ifdef CONFIG_MD2_SMEM_SIZE
#define MD2_SMEM_SIZE (CONFIG_MD2_SMEM_SIZE)
#else
#define MD2_SMEM_SIZE	(4*1024*1024)
#endif
// MD3
#ifdef  CONFIG_MTK_ENABLE_MD3
#define MTK_MD3_EN	(1)
#else
#define MTK_MD3_EN	(0)
#endif

#ifdef CONFIG_MTK_MD3_SUPPORT
#define MTK_MD3_SUPPORT	(CONFIG_MTK_MD3_SUPPORT)
#else
#define MTK_MD3_SUPPORT	(3)
#endif
#ifdef CONFIG_MD3_SIZE
#define MD3_SIZE (CONFIG_MD3_SIZE)
#else
#define MD3_SIZE	(0*1024*1024)
#endif

#ifdef CONFIG_MD3_SMEM_SIZE
#define MD3_SMEM_SIZE (CONFIG_MD3_SMEM_SIZE)
#else
#define MD3_SMEM_SIZE	(0*1024*1024)
#endif

// MD5
#ifdef  CONFIG_MTK_ENABLE_MD5
#define MTK_MD5_EN	(1)
#else
#define MTK_MD5_EN	(0)
#endif
#ifdef CONFIG_MTK_MD5_SUPPORT
#define MTK_MD5_SUPPORT	(CONFIG_MTK_MD5_SUPPORT)
#else
#define MTK_MD5_SUPPORT	(3)
#endif

#ifdef CONFIG_MD5_SIZE
#define MD5_SIZE (CONFIG_MD5_SIZE)
#else
#define MD5_SIZE	(0*1024*1024)
#endif

#ifdef CONFIG_MD5_SMEM_SIZE
#define MD5_SMEM_SIZE (CONFIG_MD5_SMEM_SIZE)
#else
#define MD5_SMEM_SIZE	(0*1024*1024)
#endif


//#define FEATURE_DFO_EN
static fos_item_t ccci_fos_default_setting[] =
{
	{"MTK_ENABLE_MD1",	MTK_MD1_EN},
	{"MTK_MD1_SUPPORT", MTK_MD1_SUPPORT},
	{"MD1_SIZE",		MD1_SIZE},
	{"MD1_SMEM_SIZE",	MD1_SMEM_SIZE},

	{"MTK_ENABLE_MD2",	MTK_MD2_EN},
	{"MTK_MD2_SUPPORT", MTK_MD2_SUPPORT},
	{"MD2_SIZE",		MD2_SIZE},
	{"MD2_SMEM_SIZE",	MD2_SMEM_SIZE},

	{"MTK_ENABLE_MD1",	MTK_MD3_EN},
	{"MTK_MD3_SUPPORT", MTK_MD3_SUPPORT},
	{"MD3_SIZE",		MD3_SIZE},
	{"MD3_SMEM_SIZE",	MD3_SMEM_SIZE},

	{"MTK_ENABLE_MD5",	MTK_MD5_EN},
	{"MTK_MD5_SUPPORT", MTK_MD5_SUPPORT},
	{"MD5_SIZE",		MD5_SIZE},
	{"MD5_SMEM_SIZE",	MD5_SMEM_SIZE},
};

// Tag value from LK
static unsigned char md_info_tag_val[4];
static unsigned int md_support[MAX_MD_NUM];
static unsigned int meta_md_support[MAX_MD_NUM];

int ccci_get_fo_setting(char item[], unsigned int *val)
{
	char *ccci_name;
	int  ccci_value;
	int  i;

	for (i=0; i<ARRAY_SIZE(ccci_fos_default_setting); i++) {
		ccci_name = ccci_fos_default_setting[i].name;
		ccci_value = ccci_fos_default_setting[i].value;
		if(!strcmp(ccci_name, item)) {
			CCCI_UTIL_ERR_MSG("FO:%s -> %08x\n", item, ccci_value);
			*val = (unsigned int)ccci_value;
			return 0;
		}
	}
	CCCI_UTIL_ERR_MSG("FO:%s not found\n", item);
	return -CCCI_ERR_INVALID_PARAM;
}

//--- LK tag and device tree -----
#if defined(CONFIG_OF)
static unsigned long dt_chosen_node;
static int __init early_init_dt_get_chosen(unsigned long node, const char *uname, int depth, void *data)
{
	if (depth != 1 ||
	    (strcmp(uname, "chosen") != 0 && strcmp(uname, "chosen@0") != 0))
		return 0;
    dt_chosen_node=node;
	return 1;
}

static void lk_meta_tag_info_collect(void)
{
	// Device tree method
	char	*tags;
    int ret;

	ret = of_scan_flat_dt(early_init_dt_get_chosen, NULL);
    if(ret==0){
        CCCI_UTIL_INF_MSG("device node no chosen node\n");
        return;
    }
	tags = (char*)of_get_flat_dt_prop(dt_chosen_node, "atag,mdinfo", NULL);
	tags+=8; // Fix me, Arm64 doesn't have atag defination now
	if (tags) {
		md_info_tag_val[0] = tags[0];
		md_info_tag_val[1] = tags[1];
		md_info_tag_val[2] = tags[2];
		md_info_tag_val[3] = tags[3];
		CCCI_UTIL_INF_MSG("Get MD info Tags\n");
		CCCI_UTIL_INF_MSG("md_inf[0]=%d\n", md_info_tag_val[0]);
		CCCI_UTIL_INF_MSG("md_inf[1]=%d\n", md_info_tag_val[1]);
		CCCI_UTIL_INF_MSG("md_inf[2]=%d\n", md_info_tag_val[2]);
		CCCI_UTIL_INF_MSG("md_inf[3]=%d\n", md_info_tag_val[3]);
	}else{
        CCCI_UTIL_INF_MSG("atag,mdinfo=NULL\n");
	}
}
#endif

#if defined(FEATURE_DFO_EN) && defined(CONFIG_OF)
static int update_fos_table_item(char name[], unsigned int val)
{
	int  i;

	for (i=0; i<ARRAY_SIZE(ccci_fos_default_setting); i++) {
		if(strcmp(ccci_fos_default_setting[i].name, name) == 0) {
			ccci_fos_default_setting[i].value = val;
			return 0;
		}
	}

	return -CCCI_ERR_INVALID_PARAM;
}

static void lk_dfo_tag_info_collect(void)
{
	// Device tree method
	struct tag	*tags;
	int 		ret, dfo_nr;
	int		i;

	ret = of_scan_flat_dt(early_init_dt_get_chosen, NULL);
    if(ret==0){
        CCCI_UTIL_INF_MSG("device node no chosen node\n");
        return;
    }
	tags = of_get_flat_dt_prop(dt_chosen_node, "atag,dfo", NULL);
	if (tags) {
		dfo_nr = ((tags->hdr.size << 2) - sizeof(struct tag_header)) / sizeof(tag_dfo_boot);
		for(i=0; i<dfo_nr; i++)
			update_fos_table_item(tags->u.dfo_data.info[i].name, tags->u.dfo_data.info[i].value);
	}else{
        CCCI_UTIL_INF_MSG("atag,dfo=NULL\n");
	}
}
#endif

#if defined(FEATURE_DFO_EN)
#if (!defined (CONFIG_OF)) // Using legacy dfo
extern int dfo_query(const char *s, unsigned long *v);
void ccci_parse_dfo_setting(void *dfo_tbl, int num)   //Export to mt_devs if don't using device tree
{
	int i = 0;
	unsigned long dfo_val = 0;

	for(i=0; i< (sizeof(ccci_fos_default_setting)/sizeof(fos_item_t)); i++) {
		if(dfo_query(ccci_fos_default_setting[i].name, &dfo_val) == 0) {
			ccci_fos_default_setting[i].value = (unsigned int)dfo_val;
		}
	}
}
#endif
#else
void ccci_parse_dfo_setting(void *dfo_tbl, int num) {}
#endif


//--- META arguments parse -------
static int ccci_parse_meta_md_setting(unsigned char args[])
{
	unsigned char md_active_setting = args[1];
	unsigned char md_setting_flag = args[0];
	int active_id =  -1;

	if(md_active_setting & MD1_EN)
		active_id = MD_SYS1;
	else if(md_active_setting & MD2_EN)
		active_id = MD_SYS2;
	else if(md_active_setting & MD3_EN)
		active_id = MD_SYS3;
	else if(md_active_setting & MD5_EN)
		active_id = MD_SYS5;
	else
		CCCI_UTIL_ERR_MSG("META MD setting not found [%d][%d]\n", args[0], args[1]);

	switch(active_id) 
	{
	case MD_SYS1:
	case MD_SYS2:
	case MD_SYS3:
	case MD_SYS5:
		if(md_setting_flag == MD_2G_FLAG) {
			meta_md_support[active_id] = modem_2g;
		} else if(md_setting_flag == MD_WG_FLAG) {
			meta_md_support[active_id] = modem_wg;
		} else if(md_setting_flag == MD_TG_FLAG) {
			meta_md_support[active_id] = modem_tg;
		} else if(md_setting_flag == MD_LWG_FLAG){
			meta_md_support[active_id] = modem_lwg;
		} else if(md_setting_flag == MD_LTG_FLAG){
			meta_md_support[active_id] = modem_ltg;
		} else if(md_setting_flag & MD_SGLTE_FLAG){
			meta_md_support[active_id] = modem_sglte;
		}
		CCCI_UTIL_INF_MSG("META MD%d to type:%d\n", active_id+1, meta_md_support[active_id]);
		break;
	}
	return 0;	
}

int get_modem_support_cap(int md_id)
{
	if(md_id < MAX_MD_NUM) {
		if(((get_boot_mode()==META_BOOT) || (get_boot_mode()==ADVMETA_BOOT)) && (meta_md_support[md_id]!=0))
			return meta_md_support[md_id];
		else
			return md_support[md_id];
	}

	return -1;
}

int set_modem_support_cap(int md_id, int new_val)
{
	if(md_id < MAX_MD_NUM) {
        if(((get_boot_mode()==META_BOOT) || (get_boot_mode()==ADVMETA_BOOT)) && (meta_md_support[md_id]!=0))
            meta_md_support[md_id] = new_val;
        else
            md_support[md_id] = new_val;
        return 0;
	}

	return -1;
}

//--- MD setting collect
// modem index is not continuous, so there may be gap in this arrays
static unsigned int md_usage_case = 0;
static unsigned int modem_num = 0;

static unsigned int md_resv_mem_size[MAX_MD_NUM]; // MD ROM+RAM
static unsigned int md_resv_smem_size[MAX_MD_NUM]; // share memory
static unsigned int modem_size_list[MAX_MD_NUM];

static phys_addr_t md_resv_mem_list[MAX_MD_NUM];
static phys_addr_t md_resv_mem_addr[MAX_MD_NUM]; 
static phys_addr_t md_resv_smem_addr[MAX_MD_NUM]; 
static phys_addr_t md_resv_smem_base;

int get_md_resv_mem_info(int md_id, phys_addr_t *r_rw_base, unsigned int *r_rw_size, phys_addr_t *srw_base, unsigned int *srw_size)
{
	if(md_id > MAX_MD_NUM)
		return -1;

	if(r_rw_base!=NULL)
		*r_rw_base = md_resv_mem_addr[md_id];

	if(r_rw_size!=NULL)
		*r_rw_size = md_resv_mem_size[md_id];

	if(srw_base!=NULL)
		*srw_base = md_resv_smem_addr[md_id];

	if(srw_size!=NULL)
		*srw_size = md_resv_smem_size[md_id];

	return 0;
}

unsigned int get_md_smem_align(int md_id)
{
	return 0x4000;
}

unsigned int get_modem_is_enabled(int md_id)
{
	return !!(md_usage_case & (1<<md_id));
}

static void v1_layout_cal(void) // Layout: MD1 R+RW | MD1 S_RW | MD2 R+RW | MD2 S_RW
{
	if( (md_usage_case&(MD1_EN|MD2_EN))==(MD1_EN|MD2_EN)) { // Both two MD enabled
		modem_size_list[0] = md_resv_mem_size[MD_SYS1]+md_resv_smem_size[MD_SYS1];
		modem_size_list[1] = md_resv_mem_size[MD_SYS2]+md_resv_smem_size[MD_SYS2];
	} else if( (md_usage_case&(MD1_EN|MD2_EN))==(MD1_EN)) { //Only MD1 enabled
		modem_size_list[0] = md_resv_mem_size[MD_SYS1]+md_resv_smem_size[MD_SYS1];
		modem_size_list[1] = 0;
	} else if( (md_usage_case&(MD1_EN|MD2_EN))==(MD2_EN)) { //Only MD2 enabled
		modem_size_list[0] = md_resv_mem_size[MD_SYS2]+md_resv_smem_size[MD_SYS2];
		modem_size_list[1] = 0;
	} else { // No MD is enabled
		modem_size_list[0] = 0;
		modem_size_list[1] = 0;
	}

	modem_size_list[2] = 0; // No MD3
	modem_size_list[3] = 0; // No MD4
}

static void default_layout_cal(void) // Layout: MD1 R+RW | MD1 S_RW | MD2 S_RW | MD2 R+RW
{
	if( (md_usage_case&(MD1_EN|MD2_EN))==(MD1_EN|MD2_EN)) { // Both two MD enabled
		modem_size_list[0] = md_resv_mem_size[MD_SYS1]+md_resv_smem_size[MD_SYS1]+md_resv_smem_size[MD_SYS2];
		modem_size_list[1] = md_resv_mem_size[MD_SYS2];
	} else if( (md_usage_case&(MD1_EN|MD2_EN))==(MD1_EN)) { //Only MD1 enabled
		modem_size_list[0] = md_resv_mem_size[MD_SYS1]+md_resv_smem_size[MD_SYS1];
		modem_size_list[1] = 0;
	} else if( (md_usage_case&(MD1_EN|MD2_EN))==(MD2_EN)) { //Only MD2 enabled
		modem_size_list[0] = md_resv_mem_size[MD_SYS2]+md_resv_smem_size[MD_SYS2];
		modem_size_list[1] = 0;
	} else { // No MD is enabled
		modem_size_list[0] = 0;
		modem_size_list[1] = 0;
	}

	modem_size_list[2] = 0; // No MD3
	modem_size_list[3] = 0; // No MD4
}


static void memory_layout_cal(int version)
{
	switch(version)
	{
	case 1:  //V1
		v1_layout_cal();
		break;
	default: //default version
		default_layout_cal();
		break;
	}
}

static void collect_md_settings(void)
{
	unsigned int tmp;
	unsigned int md1_en = 0;
	unsigned int md2_en = 0;
	unsigned int md3_en = 0;
	unsigned int md5_en = 0;
	md_usage_case = 0;

	printk("[ccci] collect_md_settings\n");

	// MTK_ENABLE_MD*
	if(ccci_get_fo_setting("MTK_ENABLE_MD1", &tmp) == 0) {
		if(tmp > 0)
			md1_en = 1;
	}
	if(ccci_get_fo_setting("MTK_ENABLE_MD2", &tmp) == 0) {
		if(tmp > 0) 
			md2_en = 1;
	}
	if(ccci_get_fo_setting("MTK_ENABLE_MD3", &tmp) == 0) {
		if(tmp > 0) 
			md3_en = 1;
	}
	if(ccci_get_fo_setting("MTK_ENABLE_MD5", &tmp) == 0) {
		if(tmp > 0) 
			md5_en = 1;
	}
	// MTK_MD*_SUPPORT
	if(ccci_get_fo_setting("MTK_MD1_SUPPORT", &tmp) == 0) {
		md_support[MD_SYS1] = tmp;
	}
	if(ccci_get_fo_setting("MTK_MD2_SUPPORT", &tmp) == 0) {
		md_support[MD_SYS2] = tmp;
	}
	if(ccci_get_fo_setting("MTK_MD3_SUPPORT", &tmp) == 0) {
		md_support[MD_SYS3] = tmp;
	}
	if(ccci_get_fo_setting("MTK_MD5_SUPPORT", &tmp) == 0) {
		md_support[MD_SYS5] = tmp;
	}
	// MD*_SIZE
	/*
	 * for legacy CCCI: make share memory start address to be 2MB align, as share 
	 * memory size is 2MB - requested by MD MPU.
	 * for ECCCI: ROM+RAM size will be align to 1M, and share memory is 2K,
	 * 1M alignment is also 2K alignment.
	 */
	if(ccci_get_fo_setting("MD1_SIZE", &tmp) == 0) {
		tmp = round_up(tmp, get_md_smem_align(MD_SYS1));
		md_resv_mem_size[MD_SYS1] = tmp;
	}
	if(ccci_get_fo_setting("MD2_SIZE", &tmp) == 0) {
		tmp = round_up(tmp, get_md_smem_align(MD_SYS2));
		md_resv_mem_size[MD_SYS2] = tmp;
	}
	if(ccci_get_fo_setting("MD3_SIZE", &tmp) == 0) {
		tmp = round_up(tmp, get_md_smem_align(MD_SYS3));
		md_resv_mem_size[MD_SYS3] = tmp;
	}
	// MD*_SMEM_SIZE
	#if 0
	if(ccci_get_fo_setting("MD1_SMEM_SIZE", &tmp) == 0) {
		md_resv_smem_size[MD_SYS1] = tmp;
	}
	#else
	md_resv_smem_size[MD_SYS1] = 2*1024*1024;
	#endif
	#if 0
	if(ccci_get_fo_setting("MD2_SMEM_SIZE", &tmp) == 0) {
		md_resv_smem_size[MD_SYS2] = tmp;
	}
	#else
	md_resv_smem_size[MD_SYS2] = 4*1024*1024;
	#endif
	if(ccci_get_fo_setting("MD3_SMEM_SIZE", &tmp) == 0) {
		md_resv_smem_size[MD_SYS3] = tmp;
	}
	
	// Setting conflict checking
	if(md1_en && (md_resv_smem_size[MD_SYS1]>0) && (md_resv_mem_size[MD_SYS1]>0)) {
		// Setting is OK
	} else if (md1_en && ((md_resv_smem_size[MD_SYS1]<=0) || (md_resv_mem_size[MD_SYS1]<=0))) {
		CCCI_UTIL_ERR_MSG_WITH_ID(MD_SYS1,"FO Setting for md1 wrong: <%d:0x%08X:0x%08X>\n", 
				md1_en, md_resv_mem_size[MD_SYS1], md_resv_smem_size[MD_SYS1]);
		md1_en = 0;
		md_resv_smem_size[MD_SYS1] = 0;
		md_resv_mem_size[MD_SYS1] = 0;
	}

	if(md2_en && (md_resv_smem_size[MD_SYS2]>0) && (md_resv_mem_size[MD_SYS2]>0)) {
		// Setting is OK
	} else if (md2_en && ((md_resv_smem_size[MD_SYS2]<=0) || (md_resv_mem_size[MD_SYS2]<=0))) {
		CCCI_UTIL_ERR_MSG_WITH_ID(MD_SYS2,"FO Setting for md2 wrong: <%d:0x%08X:0x%08X>\n", 
				md2_en, md_resv_mem_size[MD_SYS2], md_resv_smem_size[MD_SYS2]);
		md2_en = 0;
		md_resv_smem_size[MD_SYS2] = 0;
		md_resv_mem_size[MD_SYS2] = 0;
	}

	if(md3_en && (md_resv_smem_size[MD_SYS3]>0) && (md_resv_mem_size[MD_SYS3]>0)) {
		// Setting is OK
	} else if (md3_en && ((md_resv_smem_size[MD_SYS3]<=0) || (md_resv_mem_size[MD_SYS3]<=0))) {
		CCCI_UTIL_ERR_MSG_WITH_ID(MD_SYS3,"FO Setting for md3 wrong: <%d:0x%08X:0x%08X>\n", 
				md3_en, md_resv_mem_size[MD_SYS3], md_resv_smem_size[MD_SYS3]);
		md3_en = 0;
		md_resv_smem_size[MD_SYS2] = 0;
		md_resv_mem_size[MD_SYS2] = 0;
	}

	if(md1_en) {
		md_usage_case |= MD1_EN;
		modem_num++;
	}
	if(md2_en) {
		md_usage_case |= MD2_EN;
		modem_num++;
	}
	if(md3_en) {
		md_usage_case |= MD3_EN;
		modem_num++;
	}
	if(md5_en) {
		md_usage_case |= MD5_EN;
		modem_num++;
	}

	memory_layout_cal(MEM_LAY_OUT_VER);
}


static void default_cal_md_mem_setting(void)
{
	phys_addr_t *addr;
	unsigned int md1_en, md2_en;

	addr = md_resv_mem_list; // MD ROM start address should be 32M align as remap hardware limitation

	if( (md_usage_case&(MD1_EN|MD2_EN))==(MD1_EN|MD2_EN)) { // Both two MD enabled
		md1_en = 1;
		md2_en = 1;
		md_resv_mem_addr[MD_SYS1] = addr[0];
		md_resv_mem_addr[MD_SYS2] = addr[1];
		md_resv_smem_addr[MD_SYS1] = addr[0] + md_resv_mem_size[MD_SYS1];
		md_resv_smem_addr[MD_SYS2] = addr[0] + md_resv_mem_size[MD_SYS1] + md_resv_smem_size[MD_SYS1];
		md_resv_smem_base = addr[0]; // attention, share memory's base is not where share memory actually starts, but the same as MD ROM, check ccci_set_mem_remap()
	} else if( (md_usage_case&(MD1_EN|MD2_EN))==(MD1_EN)) { //Only MD1 enabled
		md1_en = 1;
		md2_en = 0;
		md_resv_mem_addr[MD_SYS1] = addr[0];
		md_resv_mem_addr[MD_SYS2] = 0;
		md_resv_smem_addr[MD_SYS1] = addr[0] + md_resv_mem_size[MD_SYS1];
		md_resv_smem_addr[MD_SYS2] = 0;
		md_resv_smem_base = addr[0];
	} else if( (md_usage_case&(MD1_EN|MD2_EN))==(MD2_EN)) { //Only MD2 enabled
		md1_en = 0;
		md2_en = 1;
		md_resv_mem_addr[MD_SYS1] = 0;
		md_resv_mem_addr[MD_SYS2] = addr[0];
		md_resv_smem_addr[MD_SYS1] = 0;
		md_resv_smem_addr[MD_SYS2] = addr[0] + md_resv_mem_size[MD_SYS2];
		md_resv_smem_base = addr[0];
	} else { // No MD is enabled
		md1_en = 0;
		md2_en = 0;
		md_resv_mem_addr[MD_SYS1] = 0;
		md_resv_mem_addr[MD_SYS2] = 0;
		md_resv_smem_addr[MD_SYS1] = 0;
		md_resv_smem_addr[MD_SYS2] = 0;
		md_resv_smem_base = 0;
	}

	if (md1_en && ((md_resv_mem_addr[MD_SYS1]&(CCCI_MEM_ALIGN - 1)) != 0))
		CCCI_UTIL_ERR_MSG("md1 memory addr is not 32M align!!!\n");

	if (md2_en && ((md_resv_mem_addr[MD_SYS2]&(CCCI_MEM_ALIGN - 1)) != 0))
		CCCI_UTIL_ERR_MSG("md2 memory addr is not 32M align!!!\n");

	if (md1_en && ((md_resv_smem_addr[MD_SYS1]&(CCCI_SMEM_ALIGN_MD1 - 1)) != 0))
		CCCI_UTIL_ERR_MSG("md1 share memory addr 0x%pa is not %x align!!\n", &md_resv_smem_addr[MD_SYS1], CCCI_SMEM_ALIGN_MD1);

	if (md2_en && ((md_resv_smem_addr[MD_SYS2]&(CCCI_SMEM_ALIGN_MD2 - 1)) != 0))
		CCCI_UTIL_ERR_MSG("md2 share memory addr 0x%pa is not %x align!!\n", &md_resv_smem_addr[MD_SYS2], CCCI_SMEM_ALIGN_MD2);

	CCCI_UTIL_INF_MSG("MD1_EN(%d):MD2_EN(%d):MemBase(0x%pa)\n", md1_en, md2_en, &md_resv_smem_base);

	CCCI_UTIL_INF_MSG("MemStart(0x%pa:0x%pa):MemSize(0x%08X:0x%08X)\n", \
		&md_resv_mem_addr[MD_SYS1], &md_resv_mem_addr[MD_SYS2], \
		md_resv_mem_size[MD_SYS1], md_resv_mem_size[MD_SYS2]);

	CCCI_UTIL_INF_MSG("SmemStart(0x%pa:0x%pa):SmemSize(0x%08X:0x%08X)\n", \
		&md_resv_smem_addr[MD_SYS1], &md_resv_smem_addr[MD_SYS2], \
		md_resv_smem_size[MD_SYS1], md_resv_smem_size[MD_SYS2]);
}

static void v1_cal_md_mem_setting(void)
{
	phys_addr_t *addr;
	unsigned int md1_en, md2_en;

	addr = md_resv_mem_list; // MD ROM start address should be 32M align as remap hardware limitation

	if( (md_usage_case&(MD1_EN|MD2_EN))==(MD1_EN|MD2_EN)) { // Both two MD enabled
		md1_en = 1;
		md2_en = 1;
		md_resv_mem_addr[MD_SYS1] = addr[0];
		md_resv_mem_addr[MD_SYS2] = addr[1];
		md_resv_smem_addr[MD_SYS1] = addr[0] + md_resv_mem_size[MD_SYS1];
		md_resv_smem_addr[MD_SYS2] = addr[1] + md_resv_mem_size[MD_SYS2];
		md_resv_smem_base = addr[0]; // attention, share memory's base is not where share memory actually starts, but the same as MD ROM, check ccci_set_mem_remap()
	} else if( (md_usage_case&(MD1_EN|MD2_EN))==(MD1_EN)) { //Only MD1 enabled
		md1_en = 1;
		md2_en = 0;
		md_resv_mem_addr[MD_SYS1] = addr[0];
		md_resv_mem_addr[MD_SYS2] = 0;
		md_resv_smem_addr[MD_SYS1] = addr[0] + md_resv_mem_size[MD_SYS1];
		md_resv_smem_addr[MD_SYS2] = 0;
		md_resv_smem_base = addr[0];
	} else if( (md_usage_case&(MD1_EN|MD2_EN))==(MD2_EN)) { //Only MD2 enabled
		md1_en = 0;
		md2_en = 1;
		md_resv_mem_addr[MD_SYS1] = 0;
		md_resv_mem_addr[MD_SYS2] = addr[0];
		md_resv_smem_addr[MD_SYS1] = 0;
		md_resv_smem_addr[MD_SYS2] = addr[0] + md_resv_mem_size[MD_SYS2];
		md_resv_smem_base = addr[0];
	} else { // No MD is enabled
		md1_en = 0;
		md2_en = 0;
		md_resv_mem_addr[MD_SYS1] = 0;
		md_resv_mem_addr[MD_SYS2] = 0;
		md_resv_smem_addr[MD_SYS1] = 0;
		md_resv_smem_addr[MD_SYS2] = 0;
		md_resv_smem_base = 0;
	}

	if (md1_en && ((md_resv_mem_addr[MD_SYS1]&(CCCI_MEM_ALIGN - 1)) != 0))
		CCCI_UTIL_ERR_MSG("md1 memory addr is not 32M align!!!\n");

	if (md2_en && ((md_resv_mem_addr[MD_SYS2]&(CCCI_MEM_ALIGN - 1)) != 0))
		CCCI_UTIL_ERR_MSG("md2 memory addr is not 32M align!!!\n");

	if (md1_en && ((md_resv_smem_addr[MD_SYS1]&(CCCI_SMEM_ALIGN_MD1 - 1)) != 0))
		CCCI_UTIL_ERR_MSG("md1 share memory addr %pa is not %x align!!\n", &md_resv_smem_addr[MD_SYS1], CCCI_SMEM_ALIGN_MD1);

	if (md2_en && ((md_resv_smem_addr[MD_SYS2]&(CCCI_SMEM_ALIGN_MD2 - 1)) != 0))
		CCCI_UTIL_ERR_MSG("md2 share memory addr %pa is not %x align!!\n", &md_resv_smem_addr[MD_SYS2], CCCI_SMEM_ALIGN_MD2);

	CCCI_UTIL_INF_MSG("MD1_EN(%d):MD2_EN(%d):MemBase(0x%pa)\n", md1_en, md2_en, &md_resv_smem_base);

	CCCI_UTIL_INF_MSG("MemStart(0x%pa:0x%pa):MemSize(0x%08X:0x%08X)\n", \
		&md_resv_mem_addr[MD_SYS1], &md_resv_mem_addr[MD_SYS2], \
		md_resv_mem_size[MD_SYS1], md_resv_mem_size[MD_SYS2]);

	CCCI_UTIL_INF_MSG("SmemStart(0x%pa:0x%pa):SmemSize(0x%08X:0x%08X)\n", \
		&md_resv_smem_addr[MD_SYS1], &md_resv_smem_addr[MD_SYS2], \
		md_resv_smem_size[MD_SYS1], md_resv_smem_size[MD_SYS2]);
}

static void cal_md_mem_setting(int version)
{
	switch(version)
	{
	case 1:  //V1
		v1_cal_md_mem_setting();
		break;
	default: //default version
		default_cal_md_mem_setting();
		break;
	}
}

    
void ccci_md_mem_reserve(void)
{
	int reserved_size = 0;
	phys_addr_t ptr = 0;
	int i;
    CCCI_UTIL_INF_MSG("ccci_md_mem_reserve===1.\n");
	#if defined(CONFIG_OF)
	lk_meta_tag_info_collect();
    CCCI_UTIL_INF_MSG("ccci_md_mem_reserve===2.\n");

	#if defined(FEATURE_DFO_EN) // DFO enable and using device tree
	lk_dfo_tag_info_collect();
	#endif

	#endif
    CCCI_UTIL_INF_MSG("ccci_md_mem_reserve===3.\n");

	// Get MD memory requirements
	collect_md_settings();
    CCCI_UTIL_INF_MSG("ccci_md_mem_reserve===4.\n");

	// For internal MD

	for(i=0; i<4; i++) {// 0~3 for internal
		if(modem_size_list[i] == 0)
			continue;
		reserved_size = ALIGN(modem_size_list[MD_SYS1+i], SZ_2M);
		memblock_set_current_limit(0xFFFFFFFF);
    #ifdef CONFIG_ARM64
    ptr = arm64_memblock_steal(reserved_size, CCCI_MEM_ALIGN);
    #else
    ptr = arm_memblock_steal(reserved_size, CCCI_MEM_ALIGN);
    #endif
		memblock_set_current_limit(MEMBLOCK_ALLOC_ANYWHERE);
		if(ptr) {
			md_resv_mem_list[i] = ptr;
			CCCI_UTIL_INF_MSG("md%d mem reserve successfully, ptr=0x%pa, size=0x%x\n", i+1, &ptr, reserved_size);
		}else{
			CCCI_UTIL_INF_MSG("md%d mem reserve fail.\n", i+1);
		}
	}
#if 0//def CONFIG_ARM64
        memblock_set_current_limit(0xFFFFFFFF);
        ptr = arm64_memblock_steal(90*1024*1024, CCCI_MEM_ALIGN);
        md_resv_mem_list[0] = ptr;
        ptr = arm64_memblock_steal(32*1024*1024, CCCI_MEM_ALIGN);
        md_resv_mem_list[1] = ptr;
        memblock_set_current_limit(MEMBLOCK_ALLOC_ANYWHERE);
#endif

	// Parse META setting
	ccci_parse_meta_md_setting(md_info_tag_val);
    CCCI_UTIL_INF_MSG("ccci_md_mem_reserve===5.\n");

	// Calculate memory layout
	cal_md_mem_setting(MEM_LAY_OUT_VER);
    CCCI_UTIL_INF_MSG("ccci_md_mem_reserve===6.\n");

}
