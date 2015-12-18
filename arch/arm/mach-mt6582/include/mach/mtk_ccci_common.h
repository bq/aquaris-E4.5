#ifndef __MTK_CCCI_COMMON_H__
#define __MTK_CCCI_COMMON_H__


//-------------META MD setting define------------------//
#define MD1_SETTING_ACTIVE	(1<<0)
#define MD2_SETTING_ACTIVE	(1<<1)
#define MD5_SETTING_ACTIVE	(1<<4)

#define MD_2G_FLAG    (1<<0)
#define MD_FDD_FLAG   (1<<1)
#define MD_TDD_FLAG   (1<<2)
#define MD_LTE_FLAG   (1<<3)
#define MD_SGLTE_FLAG (1<<4)

#define MD_WG_FLAG    (MD_FDD_FLAG|MD_2G_FLAG)
#define MD_TG_FLAG    (MD_TDD_FLAG|MD_2G_FLAG)
#define MD_LWG_FLAG   (MD_LTE_FLAG|MD_FDD_FLAG|MD_2G_FLAG)
#define MD_LTG_FLAG   (MD_LTE_FLAG|MD_TDD_FLAG|MD_2G_FLAG)

//-------------enum define---------------------------//
typedef enum {
	modem_invalid = 0,
	modem_2g = 1,
	modem_3g,
	modem_wg,
	modem_tg,
	modem_lwg,
	modem_ltg,
	modem_sglte,
	modem_max_type
}modem_type_t;


#endif
