
#ifndef __MTK_KEY_H__
#define __MTK_KEY_H__

#include "typedefs.h"
#include "platform.h"
#include <cust_kpd.h>

#define KP_STA          (KPD_BASE + 0x0000)
#define KP_MEM1         (KPD_BASE + 0x0004)
#define KP_MEM2         (KPD_BASE + 0x0008)
#define KP_MEM3         (KPD_BASE + 0x000C)
#define KP_MEM4         (KPD_BASE + 0x0010)
#define KP_MEM5         (KPD_BASE + 0x0014)
#define KP_DEBOUNCE     (KPD_BASE + 0x0018)
#define KP_SCAN_TIMING  (KPD_BASE + 0x001C)
#define KP_SEL          (KPD_BASE + 0x0020)
#define KP_EN           (KPD_BASE + 0x0024)

#define KPD_NUM_MEMS	5
#define KPD_MEM5_BITS	8

#define KPD_NUM_KEYS	72      /* 4 * 16 + KPD_MEM5_BITS */

//#define KPD_PMIC_LPRST_TD 3 /* timeout period. 0: 5sec; 1: 7sec; 2: 9sec; 3: 11sec */
//#define ONEKEY_REBOOT_NORMAL_MODE_PL
//#define TWOKEY_REBOOT_NORMAL_MODE_PL

void set_kpd_pmic_mode(void);
void disable_PMIC_kpd_clock(void);
void enable_PMIC_kpd_clock(void);
bool mtk_detect_key(unsigned short key);
bool mtk_detect_dl_keys(void);

bool mtk_detect_pmic_just_rst(void);

#endif /* __MTK_KEY_H__ */


