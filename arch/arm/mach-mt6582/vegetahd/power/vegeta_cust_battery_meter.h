// This file converted from ZCV (2) of V:/szgit/82kk/mediatek/custom/mt6582/kernel/battery/battery/vegeta_zcv.xls

#if 0
#define Q_MAX_POS_50    2583
#define Q_MAX_POS_25    2591
#define Q_MAX_POS_0     2584
#else
#define Q_MAX_POS_50    (2583-150)
#define Q_MAX_POS_25    (2591-150)
#define Q_MAX_POS_0     (2584-150)
#endif
#define Q_MAX_NEG_10    2241

#if 0
#define Q_MAX_POS_50_H_CURRENT    2568
#define Q_MAX_POS_25_H_CURRENT    2567
#define Q_MAX_POS_0_H_CURRENT     2273
#else
#define Q_MAX_POS_50_H_CURRENT    (2568-150)
#define Q_MAX_POS_25_H_CURRENT    (2567-150)
#define Q_MAX_POS_0_H_CURRENT     (2273-150)
#endif
#define Q_MAX_NEG_10_H_CURRENT    1010



/* Discharge Percentage */
#undef OAM_D5
#define OAM_D5		 1		//  1 : D5,   0: D2
