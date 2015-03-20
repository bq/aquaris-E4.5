#ifndef __FM_DBG_H__
#define __FM_DBG_H__

//#include <linux/kernel.h> //for printk()
#include <linux/xlog.h>

//DBG zone
#define BASE	4
#define MAIN	(1 << (BASE+0))
#define LINK	(1 << (BASE+1))
#define EINT	(1 << (BASE+2))
#define CHIP	(1 << (BASE+3))
#define RDSC	(1 << (BASE+4))
#define G0		(1 << (BASE+5))
#define G1		(1 << (BASE+6))
#define G2		(1 << (BASE+7))
#define G3		(1 << (BASE+8))
#define G4		(1 << (BASE+9))
#define G14		(1 << (BASE+10))
#define RAW		(1 << (BASE+11))
#define OPEN	(1 << (BASE+12))
#define IOCTL	(1 << (BASE+13))
#define READ_	(1 << (BASE+14))
#define CLOSE	(1 << (BASE+15))
#define CQI     (1 << (BASE+16))
#define ALL		0xfffffff0

//DBG level
#define L0 0x00000000 // EMERG, system will crush 
#define L1 0x00000001 // ALERT, need action in time
#define L2 0x00000002 // CRIT, important HW or SW operation failed 
#define L3 0x00000003 // ERR, normal HW or SW ERR 
#define L4 0x00000004 // WARNING, importan path or somewhere may occurs err
#define L5 0x00000005 // NOTICE, normal case 
#define L6 0x00000006 // INFO, print info if need 
#define L7 0x00000007 // DEBUG, for debug info

#define FM_EMG	L0
#define FM_ALT	L1
#define FM_CRT	L2
#define FM_ERR	L3
#define FM_WAR	L4
#define FM_NTC	L5
#define FM_INF	L6
#define FM_DBG	L7

extern fm_u32 g_dbg_level;
#if 0
#define WCN_DBG(flag, fmt, args...) \
	do { \
		if ((((flag)&0x0000000f)<=(g_dbg_level&0x0000000f)) && ((flag)&0xfffffff0)& g_dbg_level) { \
			xlog_printk(ANDROID_LOG_INFO, "[" #flag "]", fmt, ## args);\
		} \
	} while(0)
#else	
#define WCN_DBG(flag, fmt, args...) \
    do { \
        if ((((flag)&0x0000000f)<=(g_dbg_level&0x0000000f)) && ((flag)&0xfffffff0)& g_dbg_level) { \
            printk(KERN_DEBUG "[" #flag "]" fmt, ## args); \
        } \
    } while(0)
#endif

#define FM_USE_XLOG

#ifdef FM_USE_XLOG
#define FM_DRV_LOG_TAG "FM_DRV"

#define FM_LOG_DBG(flag, fmt, args...) \
	do{ \
		if((FM_DBG<= (g_dbg_level&0x0000000f)) && ((flag)&0xfffffff0)& g_dbg_level) { \
            xlog_printk(ANDROID_LOG_INFO, FM_DRV_LOG_TAG, "[" #flag "]" fmt, ## args); \
		} \
	} while(0)

#define FM_LOG_INF(flag, fmt, args...) \
    do{ \
        if((FM_INF <= (g_dbg_level&0x0000000f)) && ((flag)&0xfffffff0)& g_dbg_level) { \
            xlog_printk(ANDROID_LOG_INFO, FM_DRV_LOG_TAG, "[" #flag "]" fmt, ## args); \
        } \
    } while(0)

#define FM_LOG_NTC(flag, fmt, args...) \
    do{ \
        if((FM_NTC<= (g_dbg_level&0x0000000f)) && ((flag)&0xfffffff0)& g_dbg_level) { \
            xlog_printk(ANDROID_LOG_WARN, FM_DRV_LOG_TAG, "[" #flag "]" fmt, ## args); \
        } \
    } while(0)

#define FM_LOG_WAR(flag, fmt, args...) \
    do{ \
        if((FM_WAR <= (g_dbg_level&0x0000000f)) && ((flag)&0xfffffff0)& g_dbg_level) { \
            xlog_printk(ANDROID_LOG_WARN, FM_DRV_LOG_TAG, "[" #flag "]" fmt, ## args); \
        } \
    } while(0)

#define FM_LOG_ERR(flag, fmt, args...) \
    do{ \
        if((FM_ERR <= (g_dbg_level&0x0000000f)) && ((flag)&0xfffffff0)& g_dbg_level) { \
            xlog_printk(ANDROID_LOG_ERROR, FM_DRV_LOG_TAG, "[" #flag "]" fmt, ## args); \
        } \
    } while(0)

#define FM_LOG_CRT(flag, fmt, args...) \
    do{ \
        if((FM_CRT <= (g_dbg_level&0x0000000f)) && ((flag)&0xfffffff0)& g_dbg_level) { \
            xlog_printk(ANDROID_LOG_FATAL, FM_DRV_LOG_TAG, "[" #flag "]" fmt, ## args); \
        } \
    } while(0)

#define FM_LOG_ALT(flag, fmt, args...) \
    do{ \
        if((FM_ALT <= (g_dbg_level&0x0000000f)) && ((flag)&0xfffffff0)& g_dbg_level) { \
            xlog_printk(ANDROID_LOG_FATAL, FM_DRV_LOG_TAG, "[" #flag "]" fmt, ## args); \
        } \
    } while(0)

#define FM_LOG_EMG(flag, fmt, args...) \
    do{ \
        if((FM_EMG<= (g_dbg_level&0x0000000f)) && ((flag)&0xfffffff0)& g_dbg_level) { \
            xlog_printk(ANDROID_LOG_FATAL, FM_DRV_LOG_TAG, "[" #flag "]" fmt, ## args); \
        } \
    } while(0)

#else

#define FM_LOG_DBG(flag, fmt, args...) \
            do{ \
                if((FM_DBG<= (g_dbg_level&0x0000000f)) && ((flag)&0xfffffff0)& g_dbg_level) { \
                    printk(KERN_DEBUG "[" #flag "]" fmt, ## args); \
                } \
            } while(0)
        
#define FM_LOG_INF(flag, fmt, args...) \
            do{ \
                if((FM_INF <= (g_dbg_level&0x0000000f)) && ((flag)&0xfffffff0)& g_dbg_level) { \
                    printk(KERN_INFO "[" #flag "]" fmt, ## args); \
                } \
            } while(0)
        
#define FM_LOG_NTC(flag, fmt, args...) \
            do{ \
                if((FM_NTC <= (g_dbg_level&0x0000000f)) && ((flag)&0xfffffff0)& g_dbg_level) { \
                    printk(KERN_NOTICE "[" #flag "]" fmt, ## args); \
                } \
            } while(0)
        
#define FM_LOG_WAR(flag, fmt, args...) \
            do{ \
                if((FM_WAR <= (g_dbg_level&0x0000000f)) && ((flag)&0xfffffff0)& g_dbg_level) { \
                    printk(KERN_WARNING "[" #flag "]" fmt, ## args); \
                } \
            } while(0)
        
#define FM_LOG_ERR(flag, fmt, args...) \
            do{ \
                if((FM_ERR <= (g_dbg_level&0x0000000f)) && ((flag)&0xfffffff0)& g_dbg_level) { \
                    printk(KERN_ERR "[" #flag "]" fmt, ## args); \
                } \
            } while(0)
        
#define FM_LOG_CRT(flag, fmt, args...) \
            do{ \
                if((FM_CRT <= (g_dbg_level&0x0000000f)) && ((flag)&0xfffffff0)& g_dbg_level) { \
                    printk(KERN_CRIT "[" #flag "]" fmt, ## args); \
                } \
            } while(0)
        
#define FM_LOG_ALT(flag, fmt, args...) \
            do{ \
                if((FM_ALT <= (g_dbg_level&0x0000000f)) && ((flag)&0xfffffff0)& g_dbg_level) { \
                    printk(KERN_ALERT "[" #flag "]" fmt, ## args); \
                } \
            } while(0)
        
#define FM_LOG_EMG(flag, fmt, args...) \
            do{ \
                if((FM_EMG <= (g_dbg_level&0x0000000f)) && ((flag)&0xfffffff0)& g_dbg_level) { \
                    printk(KERN_EMERG "[" #flag "]" fmt, ## args); \
                } \
            } while(0)
#endif

#endif //__FM_DBG_H__

