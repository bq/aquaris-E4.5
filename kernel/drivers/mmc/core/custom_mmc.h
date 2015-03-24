/******************* kernel\drivers\mmc\core\custom_mmc.h ************************
    作    者: 苏 勇 
    版    本: 初始版本   1.0
    其    它: 本文件自动产生,请不要手动修改      
******************************************************************/
#ifndef CUSTOM_MMC_H
#define CUSTOM_MMC_H


//宏定义

//数据结构定义
typedef struct
{
    u32	raw_cid[4];	
	const char * name;
} EMMC_INFO ;

//函数定义
const char *GetEMMCName(const struct mmc_card const *card);

//全局变量声明



#endif /* CUSTOM_MMC_H */
