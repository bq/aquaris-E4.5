/******************* kernel\drivers\mmc\core\custom_mmc.h ************************
    ��    ��: �� �� 
    ��    ��: ��ʼ�汾   1.0
    ��    ��: ���ļ��Զ�����,�벻Ҫ�ֶ��޸�      
******************************************************************/
#ifndef CUSTOM_MMC_H
#define CUSTOM_MMC_H


//�궨��

//���ݽṹ����
typedef struct
{
    u32	raw_cid[4];	
	const char * name;
} EMMC_INFO ;

//��������
const char *GetEMMCName(const struct mmc_card const *card);

//ȫ�ֱ�������



#endif /* CUSTOM_MMC_H */
