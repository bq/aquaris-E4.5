/*
 * File: drivers/video/omap_new/debug.c
 *
 * Debug support for the omapfb driver
 *
 * Copyright (C) 2004 Nokia Corporation
 * Author: Imre Deak <imre.deak@nokia.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __MTKFB_DEBUG_H
#define __MTKFB_DEBUG_H


// global debug macro for DISP
#define DISP_DRV_DBG

#include <linux/met_drv.h>
#include "disp_svp.h"

typedef struct
{
	char* log_name;
	unsigned int begin_frm_seq;
	unsigned int end_frm_seq;
}met_log_map;

typedef enum {
    FRM_CONFIG = 0,
    FRM_TRIGGER ,
    FRM_WDMA0_EOF,
    FRM_RDMA0_SOF,
    FRM_RDMA0_EOF
} DISP_FRM_SEQ_STATE;

typedef struct 
{
	unsigned int 		mva;
	unsigned int 		max_offset;
	unsigned int		seq;
	DISP_SESSION_TYPE   session_type;
	DISP_FRM_SEQ_STATE	state;
}disp_frm_seq_info ;

extern size_t disp_drv_dbg_log;
extern size_t disp_drv_dbg_info_log;
extern size_t disp_drv_dbg_func_log;
extern size_t disp_drv_dbg_upd_log;
extern size_t mtkfb_dbg_log;
extern size_t mtkfb_dbg_fence_log;
extern size_t mtkfb_dbg_func_log;
extern size_t mtkfb_dbg_ioctl_log;


#define DISP_DRV_WRAN(fmt, arg...) \
    do { \
        if (disp_drv_dbg_log) DISP_LOG_PRINT(ANDROID_LOG_WARN, "DISP", fmt, ##arg); \
    }while (0)

#define DISP_DRV_INFO(fmt, arg...) \
    do { \
        if (disp_drv_dbg_info_log) DISP_LOG_PRINT(ANDROID_LOG_INFO, "DISP", fmt, ##arg); \
    }while (0)

#define DISP_DRV_FUNC()	\
    do { \
        if(disp_drv_dbg_func_log) DISP_LOG_PRINT(ANDROID_LOG_INFO, "DISP", "[Func]%s\n", __func__); \
    }while (0)

#define DISP_DRV_UPD(fmt, arg...) \
    do { \
        if (disp_drv_dbg_upd_log) DISP_LOG_PRINT(ANDROID_LOG_INFO, "DISP", fmt, ##arg); \
    }while (0)

#define MTKFB_WRAN(fmt, arg...) \
    do { \
        if (mtkfb_dbg_log) DISP_LOG_PRINT(ANDROID_LOG_WARN, "MTKFB", fmt, ##arg); \
    }while (0)

#define MTKFB_INFO(fmt, arg...) \
    do { \
        if (mtkfb_dbg_log) DISP_LOG_PRINT(ANDROID_LOG_INFO, "MTKFB", fmt, ##arg); \
    }while (0)

#define MTKFB_FENCE(fmt, arg...) \
    do { \
        if (mtkfb_dbg_fence_log) DISP_LOG_PRINT(ANDROID_LOG_INFO, "MTKFB", fmt, ##arg); \
    }while (0)

#define MTKFB_FUNC()	\
    do { \
        if(mtkfb_dbg_func_log) DISP_LOG_PRINT(ANDROID_LOG_INFO, "MTKFB", "[Func]%s\n", __func__); \
    }while (0)

#define MTKFB_IOCTL(fmt, arg...) \
    do { \
        if (mtkfb_dbg_ioctl_log) DISP_LOG_PRINT(ANDROID_LOG_INFO, "MTKFB", fmt, ##arg); \
    }while (0)


void DBG_Init(void);
void DBG_Deinit(void);

void DBG_OnTriggerLcd(void);
void DBG_OnTeDelayDone(void);
void DBG_OnLcdDone(void);

#include <linux/mmprofile.h>
extern struct MTKFB_MMP_Events_t
{
    MMP_Event MTKFB;
    MMP_Event CreateSyncTimeline;
    MMP_Event PanDisplay;
    MMP_Event SetOverlayLayer;
    MMP_Event SetVideoLayers;
    MMP_Event SetMultipleLayers;
    MMP_Event CreateSyncFence;
    MMP_Event IncSyncTimeline;
    MMP_Event SignalSyncFence;

    //Multiple Display
    MMP_Event SessionMgr;
    MMP_Event GetDispInfo;
    MMP_Event CreateSession;
    MMP_Event DestroySession;
    MMP_Event TriggerSession;
    MMP_Event PrepareInput;
    MMP_Event PrepareOutput;
    MMP_Event SetInput;
    MMP_Event SetOutput;

    MMP_Event JobQue;
    MMP_Event deque;
    MMP_Event enque;
    MMP_Event acquire;
    MMP_Event release;
    MMP_Event query;
    MMP_Event recycle;

    MMP_Event BufQue;
    MMP_Event deque_buf;
    MMP_Event enque_buf;
    MMP_Event acquire_buf;
    MMP_Event release_buf;
    MMP_Event request_buf;

    MMP_Event UpdateScreenImpl;
    MMP_Event VSync;
    MMP_Event UpdateConfig;
    MMP_Event ConfigOVL;
    MMP_Event ConfigWDMA;
    MMP_Event ConfigAAL;
    MMP_Event ConfigMemOut;
    MMP_Event ScreenUpdate;
    MMP_Event CaptureFramebuffer;
    MMP_Event TrigOverlayOut;
    MMP_Event RegUpdate;
    MMP_Event OverlayOutDone;
    MMP_Event SwitchMode;
    MMP_Event BypassOVL;
    MMP_Event UsingBufIdx[2][4];
    MMP_Event MaxCleanIdx[4];
    
    MMP_Event EarlySuspend;
    MMP_Event DispDone;
    MMP_Event DSICmd;
    MMP_Event DSIIRQ;
    MMP_Event EsdCheck;
    MMP_Event WaitVSync;
    MMP_Event LayerDump;
    MMP_Event Layer[4];
    MMP_Event OvlDump;
    MMP_Event FBDump;
    MMP_Event DSIRead;
    MMP_Event GetLayerInfo;
    MMP_Event LayerInfo[4];
    MMP_Event IOCtrl;
    MMP_Event Debug;
	MMP_Event primary_seq_config;
	MMP_Event primary_seq_trigger;
	MMP_Event primary_seq_wdma0_efo;
	MMP_Event primary_seq_rdma0_sof;
	MMP_Event primary_seq_rdma0_eof;
	MMP_Event external_seq_config;
	MMP_Event external_seq_trigger;
	MMP_Event external_seq_wdma0_efo;
} MTKFB_MMP_Events;

#ifdef MTKFB_DBG
#include "disp_drv_log.h"

#define DBG_BUF_SIZE		    2048
#define MAX_DBG_INDENT_LEVEL	5
#define DBG_INDENT_SIZE		    3
#define MAX_DBG_MESSAGES	    0

static int dbg_indent;
static int dbg_cnt;
static char dbg_buf[DBG_BUF_SIZE];
static spinlock_t dbg_spinlock = SPIN_LOCK_UNLOCKED;

static inline void dbg_print(int level, const char *fmt, ...)
{
	if (level <= MTKFB_DBG) {
		if (!MAX_DBG_MESSAGES || dbg_cnt < MAX_DBG_MESSAGES) {
			va_list args;
			int	ind = dbg_indent;
			unsigned long flags;

			spin_lock_irqsave(&dbg_spinlock, flags);
			dbg_cnt++;
			if (ind > MAX_DBG_INDENT_LEVEL)
				ind = MAX_DBG_INDENT_LEVEL;

			DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "%*s", ind * DBG_INDENT_SIZE, "");
			va_start(args, fmt);
			vsnprintf(dbg_buf, sizeof(dbg_buf), fmt, args);
			DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", dbg_buf);
			va_end(args);
			spin_unlock_irqrestore(&dbg_spinlock, flags);
		}
	}
}

#define DBGPRINT	dbg_print

#define DBGENTER(level)	do { \
		dbg_print(level, "%s: Enter\n", __FUNCTION__); \
		dbg_indent++; \
	} while (0)

#define DBGLEAVE(level)	do { \
		dbg_indent--; \
		dbg_print(level, "%s: Leave\n", __FUNCTION__); \
	} while (0)

// Debug Macros

#define MTKFB_DBG_EVT_NONE    0x00000000
#define MTKFB_DBG_EVT_FUNC    0x00000001  /* Function Entry     */
#define MTKFB_DBG_EVT_ARGU    0x00000002  /* Function Arguments */
#define MTKFB_DBG_EVT_INFO    0x00000003  /* Information        */

#define MTKFB_DBG_EVT_MASK    (MTKFB_DBG_EVT_NONE)

#define MSG(evt, fmt, args...)                              \
    do {                                                    \
        if ((MTKFB_DBG_EVT_##evt) & MTKFB_DBG_EVT_MASK) {   \
            DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", fmt, ##args);                            \
        }                                                   \
    } while (0)

#define MSG_FUNC_ENTER(f)   MSG(FUNC, "<FB_ENTER>: %s\n", __FUNCTION__)
#define MSG_FUNC_LEAVE(f)   MSG(FUNC, "<FB_LEAVE>: %s\n", __FUNCTION__)


#else	/* MTKFB_DBG */

#define DBGPRINT(level, format, ...)
#define DBGENTER(level)
#define DBGLEAVE(level)

// Debug Macros

#define MSG(evt, fmt, args...)
#define MSG_FUNC_ENTER()
#define MSG_FUNC_LEAVE()

#endif	/* MTKFB_DBG */

void update_frm_seq_info(unsigned int session_id, unsigned int addr, unsigned int addr_offset,unsigned int seq, DISP_FRM_SEQ_STATE state);
#endif /* __MTKFB_DEBUG_H */
