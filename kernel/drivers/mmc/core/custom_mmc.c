/******************* kernel\drivers\mmc\core\custom_mmc.c ************************
    作    者: 苏 勇 
    版    本: 初始版本   1.0
    其    它: 本文件自动产生,请不要手动修改      
******************************************************************/
//头文件包含
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/stat.h>

#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/mmc.h>

#include "core.h"
#include "bus.h"
#include "mmc_ops.h"
#include "sd_ops.h"

#include "custom_mmc.h"

//静态函数声明

//静态变量声明

//全局变量定义

static const EMMC_INFO CKT_EMMC[] =
{
	{
		{0x45010053,0x454d3038,0x47000000,0x00000000},
		"sandiskSDIN8DE1-8GB"
	},
	{
		{0x11010030,0x31364739,0x32000000,0x00000000},
		"THGBMAG7A2JBAIR-16G"
	},
	{
		{0x11010030,0x33324739,0x36000000,0x00000000},
		"THGBMAG8A4JBA4R-32G"
	},
	{
		{0x11010030,0x30384739,0x32000000,0x00000000},
		"THGBM5G6A2JBAIR-8G"
	},
	{
		{0x90014a48,0x41473265,0x05000000,0x00000000},
		"H26M52103FMR-16G"
	},
	{
		{0x00000000,0x00000000,0x00000000,0x00000000},
		"EDB8132B3MC_1D_F"
	},
	{
		{0x00000000,0x00000000,0x00000000,0x00000000},
		"EDBA232B1MA_1D_F"
	},
	{
		{0x00000000,0x00000000,0x00000000,0x00000000},
		"H9TCNNNBLDMMPR"
	},
	{
		{0x11010030,0x31364739,0x34000000,0x00000000},
		"TYD0HH251623RC"
	},
	{
		{0x90014A48,0x41473464,0x04000000,0x00000000},
		"H9TP18A8JDMCPR_KGM"
	},
	{
		{0x4B010865,0x4D435020,0x20000000,0x00000000},
		"AKPAB128EXN208MBB"
	},
};

/******************* 函数名: GetEMMCName **********************
    函数功能: 根据raw_id的对比找到对应的emmc的名字
    函数参数: 
        card :mmc_card,其中包含有raw_id
    返回类型: char * emmc的名字
    其    它: 
****************************************************************/
const char *GetEMMCName(const struct mmc_card const *card)
{	
	int i;
	const static char * unknown="unknown";
	for(i=0;i<sizeof(CKT_EMMC)/sizeof(CKT_EMMC[0]);i++ )
	{
		if(
			(CKT_EMMC[i].raw_cid[0] == card->raw_cid[0])
			&&(CKT_EMMC[i].raw_cid[1] == card->raw_cid[1])
			&&((CKT_EMMC[i].raw_cid[2]&0xff000000)==(card->raw_cid[2]&0xff000000))
		  )
		{
			return CKT_EMMC[i].name;
		}
	}
	return unknown;
}


