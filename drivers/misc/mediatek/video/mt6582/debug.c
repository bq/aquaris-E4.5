#include <linux/string.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/fb.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/debugfs.h>
#include <linux/wait.h>
#include <mach/mt_typedefs.h>
#include "disp_hal.h"
#include "disp_drv_log.h"
#include "mtkfb.h"
#include "debug.h"
#include "lcm_drv.h"
#include "ddp_ovl.h"
#include "ddp_path.h"
#include "disp_drv.h"

#if defined(DISP_DRV_DBG)
    size_t disp_drv_dbg_log = true;
    size_t disp_drv_dbg_info_log = false;    // info log, default off
    size_t disp_drv_dbg_func_log = false;    // function entry log, default off
    size_t disp_drv_dbg_upd_log = false;    // upd log, default off
    size_t mtkfb_dbg_log = true;
    size_t mtkfb_dbg_fence_log = false;    // fence log, default off
    size_t mtkfb_dbg_func_log = false;    // function entry log, default off
    size_t mtkfb_dbg_ioctl_log = false;    // ioctl log, default off
#else
    size_t disp_drv_dbg_log = false;
    size_t disp_drv_dbg_info_log = false;    // info log, default off
    size_t disp_drv_dbg_func_log = false;    // function entry log, default off
    size_t disp_drv_dbg_upd_log = false;    // upd log, default off
    size_t mtkfb_dbg_log = false;
    size_t mtkfb_dbg_fence_log = false;    // fence log, default off
    size_t mtkfb_dbg_func_log = false;    // function entry log, default off
    size_t mtkfb_dbg_ioctl_log = false;    // ioctl log, default off
#endif


struct MTKFB_MMP_Events_t MTKFB_MMP_Events;

extern LCM_DRIVER *lcm_drv;
extern unsigned int EnableVSyncLog;

#define MTKFB_DEBUG_FS_CAPTURE_LAYER_CONTENT_SUPPORT

#include "disp_mgr.h"
#include <linux/ftrace_event.h>


static met_log_map dprec_met_info[DISP_SESSION_MEMORY+2] =
{
    {"DSI-LCM",     0,  0},
    {"OVL0-DSI",    0,  0},
    {"OVL0-MHL",      0,  0},
    {"OVL0-SMS",      0,  0},
};

#define absabs(a) (((a) < 0) ? -(a) : (a))
#define DISP_INTERNAL_BUFFER_COUNT 3
#define FRM_UPDATE_SEQ_CACHE_NUM (DISP_INTERNAL_BUFFER_COUNT+1)
static disp_frm_seq_info frm_update_sequence[FRM_UPDATE_SEQ_CACHE_NUM];
static disp_frm_seq_info frm_update_sequence_mem[FRM_UPDATE_SEQ_CACHE_NUM];
static unsigned int frm_update_cnt;
static unsigned int frm_update_cnt_mem;

static unsigned long __read_mostly tracing_mark_write_addr = 0;
static void inline __mt_update_tracing_mark_write_addr(void)
{
    	if(unlikely(0 == tracing_mark_write_addr))
        	tracing_mark_write_addr = kallsyms_lookup_name("tracing_mark_write");
}

static void dprec_logger_frame_seq_begin(DISP_SESSION_TYPE session_type, unsigned frm_sequence)
{  
    if(frm_sequence <= 0 || session_type <0)
        return ;
	
    if(session_type > DISP_SESSION_MEMORY)
    {
        printk("seq_begin session_type(0x%x) error, seq(%d) \n",session_type, frm_sequence);
        return;
    }
   
    if(dprec_met_info[session_type].begin_frm_seq != frm_sequence)
	{
	    __mt_update_tracing_mark_write_addr();
        event_trace_printk(tracing_mark_write_addr, "S|%d|%s|%d\n", current->tgid, dprec_met_info[session_type].log_name, frm_sequence);
	    dprec_met_info[session_type].begin_frm_seq = frm_sequence;
    }
}

static void dprec_logger_frame_seq_end(DISP_SESSION_TYPE session_type, unsigned frm_sequence)
{
    if(frm_sequence <= 0 || session_type < 0)
        return ;

    if(session_type > DISP_SESSION_MEMORY)
    {
        printk("seq_end session_type(0x%x) , seq(%d) \n",session_type, frm_sequence);
        return;
    }
    
    if(dprec_met_info[session_type].end_frm_seq != frm_sequence)
	{
    	__mt_update_tracing_mark_write_addr();
        event_trace_printk(tracing_mark_write_addr, "F|%d|%s|%d\n", current->tgid, dprec_met_info[session_type].log_name, frm_sequence); 
    	dprec_met_info[session_type].end_frm_seq = frm_sequence;
	}
}

static char* get_frame_seq_state_string(DISP_FRM_SEQ_STATE state)
{
	switch(state)
	{
		case FRM_CONFIG:
			return "FRM_CONFIG";
		case FRM_TRIGGER:
			return "FRM_TRIGGER";
		case FRM_WDMA0_EOF:
			return "FRM_WDMA0_EOF";
		case FRM_RDMA0_SOF:
			return "FRM_RDMA0_SOF";
		case FRM_RDMA0_EOF:
			return "FRM_RDMA0_EOF";
		default:
			return "UNKNOW Frame State";
	}

	return "";
}

void update_frm_seq_info(unsigned int session_id, unsigned int addr, unsigned int addr_offset,unsigned int seq, DISP_FRM_SEQ_STATE state)
{
    int i= 0;
    unsigned device_type = DISP_SESSION_TYPE(session_id);
	//printk("update_frm_seq_info, 0x%x/0x%x/0x%x/%d/%s/%d\n", session_id, addr, addr_offset, seq, get_frame_seq_state_string(state), frm_update_cnt);
	
    if(seq < 0 || session_id < 0 || state > FRM_RDMA0_EOF)
        return ;
    if(device_type > DISP_SESSION_MEMORY)
    {
        printk("seq_end session_id(0x%x) , seq(%d) \n",session_id, seq);
        return;
    }   
	
	if(device_type == DISP_SESSION_PRIMARY || device_type == DISP_SESSION_PRIMARY - 1)
	{
		if(FRM_CONFIG == state)
		{
			frm_update_sequence[frm_update_cnt].state = state;
			frm_update_sequence[frm_update_cnt].session_type = device_type;
			frm_update_sequence[frm_update_cnt].mva= addr;
			frm_update_sequence[frm_update_cnt].max_offset= addr_offset;
			if(seq > 0)
				frm_update_sequence[frm_update_cnt].seq= seq;
			MMProfileLogEx(MTKFB_MMP_Events.primary_seq_config, MMProfileFlagPulse, addr, seq);
		}
		else if(FRM_TRIGGER == state)
		{
			frm_update_sequence[frm_update_cnt].state = FRM_TRIGGER;
			MMProfileLogEx(MTKFB_MMP_Events.primary_seq_trigger, MMProfileFlagPulse, addr, seq);
			dprec_logger_frame_seq_begin(device_type,  frm_update_sequence[frm_update_cnt].seq);
	
			frm_update_cnt++;
			frm_update_cnt%=FRM_UPDATE_SEQ_CACHE_NUM;
		}
		else if(FRM_WDMA0_EOF == state)
		{
			for(i= 0; i< FRM_UPDATE_SEQ_CACHE_NUM; i++)
			{			 
	//			 if((absabs(addr -frm_update_sequence[i].mva) <=  frm_update_sequence[i].max_offset) && (frm_update_sequence[i].state == FRM_TRIGGER))
				if(frm_update_sequence[i].state == FRM_TRIGGER)
				{
					frm_update_sequence[i].state = FRM_WDMA0_EOF;
					frm_update_sequence[i].mva = addr;
					MMProfileLogEx(MTKFB_MMP_Events.primary_seq_wdma0_efo, MMProfileFlagPulse, frm_update_sequence[i].mva, frm_update_sequence[i].seq);
					///break;
				}
			}
		}
		else if(FRM_RDMA0_SOF == state)
		{
			for(i= 0; i< FRM_UPDATE_SEQ_CACHE_NUM; i++)
			{
				if(FRM_WDMA0_EOF == frm_update_sequence[i].state && device_type == DISP_SESSION_PRIMARY && frm_update_sequence[i].mva == addr)
				{
					frm_update_sequence[i].state = FRM_RDMA0_SOF;
					dprec_logger_frame_seq_end(device_type,	frm_update_sequence[i].seq);
					dprec_logger_frame_seq_begin(DISP_SESSION_PRIMARY-1,  frm_update_sequence[i].seq);
					MMProfileLogEx(MTKFB_MMP_Events.primary_seq_rdma0_sof, MMProfileFlagPulse, frm_update_sequence[i].mva, frm_update_sequence[i].seq); 
				}
			}
		} 
		else if(FRM_RDMA0_EOF == state)
		{
			for(i= 0; i< FRM_UPDATE_SEQ_CACHE_NUM; i++)
			{
				if(FRM_RDMA0_SOF == frm_update_sequence[i].state && device_type == DISP_SESSION_PRIMARY && frm_update_sequence[i].mva == addr)
				{
					frm_update_sequence[i].state = FRM_RDMA0_EOF;
					dprec_logger_frame_seq_end(DISP_SESSION_PRIMARY-1,	frm_update_sequence[i].seq );
					MMProfileLogEx(MTKFB_MMP_Events.primary_seq_rdma0_eof, MMProfileFlagPulse, frm_update_sequence[i].mva, frm_update_sequence[i].seq);
					
				}
			}
		}
	}
	else if(device_type == DISP_SESSION_MEMORY)
	{
		if(FRM_CONFIG == state)
		{
			frm_update_sequence_mem[frm_update_cnt_mem].state = state;
			frm_update_sequence_mem[frm_update_cnt_mem].session_type = device_type;
			frm_update_sequence_mem[frm_update_cnt_mem].mva= addr;
			frm_update_sequence_mem[frm_update_cnt_mem].max_offset= addr_offset;
			if(seq > 0)
				frm_update_sequence_mem[frm_update_cnt_mem].seq= seq;
	
			MMProfileLogEx(MTKFB_MMP_Events.external_seq_config, MMProfileFlagPulse, addr, seq);
		}
		else if(FRM_TRIGGER == state)
		{
			frm_update_sequence_mem[frm_update_cnt_mem].state = FRM_TRIGGER;
			MMProfileLogEx(MTKFB_MMP_Events.external_seq_trigger, MMProfileFlagPulse, addr, seq);
			dprec_logger_frame_seq_begin(device_type,  frm_update_sequence_mem[frm_update_cnt_mem].seq);
	
			frm_update_cnt_mem++;
			frm_update_cnt_mem%=FRM_UPDATE_SEQ_CACHE_NUM;
		}
		else if(FRM_WDMA0_EOF == state)
		{
			for(i= 0; i< FRM_UPDATE_SEQ_CACHE_NUM; i++)
			{			 
	//			  if((absabs(addr -frm_update_sequence_mem[i].mva) <=  frm_update_sequence_mem[i].max_offset) && (frm_update_sequence_mem[i].state == FRM_TRIGGER))
				if(frm_update_sequence_mem[i].state == FRM_TRIGGER)
				{
					MMProfileLogEx(MTKFB_MMP_Events.external_seq_wdma0_efo, MMProfileFlagPulse, frm_update_sequence_mem[i].mva, frm_update_sequence_mem[i].seq);
	
					frm_update_sequence_mem[i].state = FRM_WDMA0_EOF;
					frm_update_sequence_mem[i].mva = addr;
					dprec_logger_frame_seq_end(device_type,	frm_update_sequence_mem[i].seq);
					///break;
				}
			}
		}
	}

}
// ---------------------------------------------------------------------------
//  External variable declarations
// ---------------------------------------------------------------------------

extern long tpd_last_down_time;
extern int  tpd_start_profiling;
extern void mtkfb_log_enable(int enable);
extern void disp_log_enable(int enable);
extern void mtkfb_vsync_log_enable(int enable);
extern void mtkfb_capture_fb_only(bool enable);
extern void esd_recovery_pause(BOOL en);
extern int mtkfb_set_backlight_mode(unsigned int mode);
extern void mtkfb_pan_disp_test(void);
extern void mtkfb_show_sem_cnt(void);
extern void mtkfb_hang_test(bool en);
extern void mtkfb_switch_normal_to_factory(void);
extern void mtkfb_switch_factory_to_normal(void);

extern unsigned int gCaptureLayerRawData;
extern unsigned int gCaptureLayerEnable;
extern unsigned int gCaptureLayerDownX;
extern unsigned int gCaptureLayerDownY;

extern unsigned int gCaptureOvlRawData;
extern unsigned int gCaptureOvlEnable;
extern unsigned int gCaptureOvlDownX;
extern unsigned int gCaptureOvlDownY;

extern unsigned int gCaptureFBRawData;
extern unsigned int gCaptureFBEnable;
extern unsigned int gCaptureFBDownX;
extern unsigned int gCaptureFBDownY;

#ifdef MTKFB_DEBUG_FS_CAPTURE_LAYER_CONTENT_SUPPORT
struct dentry *mtkfb_layer_dbgfs[DDP_OVL_LAYER_MUN];

extern OVL_CONFIG_STRUCT cached_layer_config[DDP_OVL_LAYER_MUN];

typedef struct {
    UINT32 layer_index;
    UINT32 working_buf;
    UINT32 working_size;
} MTKFB_LAYER_DBG_OPTIONS;

MTKFB_LAYER_DBG_OPTIONS mtkfb_layer_dbg_opt[DDP_OVL_LAYER_MUN];

#endif    
extern LCM_DRIVER *lcm_drv;
// ---------------------------------------------------------------------------
//  Debug Options
// ---------------------------------------------------------------------------

static const long int DEFAULT_LOG_FPS_WND_SIZE = 30;

typedef struct {
    unsigned int en_fps_log;
    unsigned int en_touch_latency_log;
    unsigned int log_fps_wnd_size;
    unsigned int force_dis_layers;
} DBG_OPTIONS;

static DBG_OPTIONS dbg_opt = {0};

static char STR_HELP[] =
    "\n"
    "USAGE\n"
    "        echo [ACTION]... > /d/mtkfb\n"
    "\n"
    "ACTION\n"
	"        decouple:[on|off]\n"
	"             dynamic switch to OVL de-couple/direct-link mode\n"
	"\n"
    "        mtkfblog:[on|off]\n"
    "             enable/disable [MTKFB] log\n"
    "\n"
    "        displog:[on|off]\n"
    "             enable/disable [DISP] log\n"
    "\n"
    "        mtkfb_vsynclog:[on|off]\n"
    "             enable/disable [VSYNC] log\n"
    "\n"
    "        log:[on|off]\n"
    "             enable/disable above all log\n"
    "\n"
    "        fps:[on|off]\n"
    "             enable fps and lcd update time log\n"
    "\n"
    "        tl:[on|off]\n"
    "             enable touch latency log\n"
    "\n"
    "        layer\n"
    "             dump lcd layer information\n"
    "\n"
    "        suspend\n"
    "             enter suspend mode\n"
    "\n"
    "        resume\n"
    "             leave suspend mode\n"
    "\n"
    "        lcm:[on|off|init]\n"
    "             power on/off lcm\n"
    "\n"
    "        cabc:[ui|mov|still]\n"
    "             cabc mode, UI/Moving picture/Still picture\n"
    "\n"
    "        lcd:[on|off]\n"
    "             power on/off display engine\n"
    "\n"
    "        te:[on|off]\n"
    "             turn on/off tearing-free control\n"
    "\n"
    "        tv:[on|off]\n"
    "             turn on/off tv-out\n"
    "\n"
    "        tvsys:[ntsc|pal]\n"
    "             switch tv system\n"
    "\n"
    "        reg:[lcd|dpi|dsi|tvc|tve]\n"
    "             dump hw register values\n"
    "\n"
    "        regw:addr=val\n"
    "             write hw register\n"
    "\n"
    "        regr:addr\n"
    "             read hw register\n"
    "\n"     
    "       cpfbonly:[on|off]\n"
    "             capture UI layer only on/off\n"
    "\n"     
    "       esd:[on|off]\n"
    "             esd kthread on/off\n"
    "       HQA:[NormalToFactory|FactoryToNormal]\n"
    "             for HQA requirement\n"
    "\n"     
    "       mmp\n"
    "             Register MMProfile events\n"
    "\n"
    "       dump_fb:[on|off|raw[,down_sample_x[,down_sample_y]]]\n"
    "             Start/end to capture framebuffer every frame(OVL input buffer)\n"
    "\n"
    "       dump_ovl:[on|off|raw[,down_sample_x[,down_sample_y]]]\n"
    "             Start to capture OVL output buffer every frame\n"
    "\n"
    "       dump_layer:[on|off|raw[,down_sample_x[,down_sample_y]]]\n"
    "             Start/end to capture current enabled OVL layer input buffer every frame\n"
    ;


// ---------------------------------------------------------------------------
//  Information Dump Routines
// ---------------------------------------------------------------------------

void init_mtkfb_mmp_events(void)
{
    if (MTKFB_MMP_Events.MTKFB == 0)
    {
        MTKFB_MMP_Events.MTKFB = MMProfileRegisterEvent(MMP_RootEvent, "MTKFB");
        MTKFB_MMP_Events.PanDisplay = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "PanDisplay");
        MTKFB_MMP_Events.CreateSyncTimeline = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "CreateSyncTimeline");
        MTKFB_MMP_Events.SetOverlayLayer = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "SetOverlayLayer");
        MTKFB_MMP_Events.SetVideoLayers = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "SetVideoLayers");
        MTKFB_MMP_Events.SetMultipleLayers = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "SetMultipleLayers");
        MTKFB_MMP_Events.CreateSyncFence = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "CreateSyncFence");
        MTKFB_MMP_Events.IncSyncTimeline = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "IncSyncTimeline");
        MTKFB_MMP_Events.SignalSyncFence = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "SignalSyncFence");

        MTKFB_MMP_Events.SessionMgr = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "SessionMgr");
        MTKFB_MMP_Events.GetDispInfo = MMProfileRegisterEvent(MTKFB_MMP_Events.SessionMgr, "GetDispInfo");
        //TODO: Prepare Input/Output is different
        MTKFB_MMP_Events.PrepareInput = MMProfileRegisterEvent(MTKFB_MMP_Events.SessionMgr, "PrepareBuffer");
        MTKFB_MMP_Events.PrepareOutput = MMProfileRegisterEvent(MTKFB_MMP_Events.SessionMgr, "PrepareBuffer");
        MTKFB_MMP_Events.CreateSession = MMProfileRegisterEvent(MTKFB_MMP_Events.SessionMgr, "CreateSession");
        MTKFB_MMP_Events.DestroySession = MMProfileRegisterEvent(MTKFB_MMP_Events.SessionMgr, "DestroySession");
        MTKFB_MMP_Events.TriggerSession = MMProfileRegisterEvent(MTKFB_MMP_Events.SessionMgr, "TriggerSession");
        MTKFB_MMP_Events.SetInput = MMProfileRegisterEvent(MTKFB_MMP_Events.SessionMgr, "SetInput");
        MTKFB_MMP_Events.SetOutput = MMProfileRegisterEvent(MTKFB_MMP_Events.SessionMgr, "SetOutput");

		MTKFB_MMP_Events.primary_seq_config = MMProfileRegisterEvent(MTKFB_MMP_Events.SessionMgr, "primary_seq_config");
		MTKFB_MMP_Events.primary_seq_trigger=  MMProfileRegisterEvent(MTKFB_MMP_Events.SessionMgr, "primary_seq_trigger");
		MTKFB_MMP_Events.primary_seq_wdma0_efo=  MMProfileRegisterEvent(MTKFB_MMP_Events.SessionMgr, "primary_seq_wdma0_efo");
		MTKFB_MMP_Events.primary_seq_rdma0_sof=  MMProfileRegisterEvent(MTKFB_MMP_Events.SessionMgr, "primary_seq_rdma0_sof");
		MTKFB_MMP_Events.primary_seq_rdma0_eof=  MMProfileRegisterEvent(MTKFB_MMP_Events.SessionMgr, "primary_seq_rdma0_eof");
		MTKFB_MMP_Events.external_seq_config = MMProfileRegisterEvent(MTKFB_MMP_Events.SessionMgr, "external_seq_config");
		MTKFB_MMP_Events.external_seq_trigger=  MMProfileRegisterEvent(MTKFB_MMP_Events.SessionMgr, "external_seq_trigger");
		MTKFB_MMP_Events.external_seq_wdma0_efo=  MMProfileRegisterEvent(MTKFB_MMP_Events.SessionMgr, "external_seq_wdma0_efo");
        MTKFB_MMP_Events.JobQue = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "JobQue");
        MTKFB_MMP_Events.deque = MMProfileRegisterEvent(MTKFB_MMP_Events.JobQue, "deque");
        MTKFB_MMP_Events.enque = MMProfileRegisterEvent(MTKFB_MMP_Events.JobQue, "enque");
        MTKFB_MMP_Events.acquire = MMProfileRegisterEvent(MTKFB_MMP_Events.JobQue, "acquire");
        MTKFB_MMP_Events.release = MMProfileRegisterEvent(MTKFB_MMP_Events.JobQue, "release");
        MTKFB_MMP_Events.query = MMProfileRegisterEvent(MTKFB_MMP_Events.JobQue, "query");
        MTKFB_MMP_Events.recycle = MMProfileRegisterEvent(MTKFB_MMP_Events.JobQue, "recycle");

        MTKFB_MMP_Events.BufQue = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "BufQue");
        MTKFB_MMP_Events.deque_buf = MMProfileRegisterEvent(MTKFB_MMP_Events.BufQue, "deque");
        MTKFB_MMP_Events.enque_buf = MMProfileRegisterEvent(MTKFB_MMP_Events.BufQue, "enque");
        MTKFB_MMP_Events.acquire_buf = MMProfileRegisterEvent(MTKFB_MMP_Events.BufQue, "acquire");
        MTKFB_MMP_Events.release_buf = MMProfileRegisterEvent(MTKFB_MMP_Events.BufQue, "release");
        MTKFB_MMP_Events.request_buf = MMProfileRegisterEvent(MTKFB_MMP_Events.BufQue, "request");

        MTKFB_MMP_Events.UpdateScreenImpl = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "UpdateScreenImpl");
        MTKFB_MMP_Events.VSync = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "VSync");
        MTKFB_MMP_Events.UpdateConfig = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "UpdateConfig");
        MTKFB_MMP_Events.BypassOVL = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "BypassOVL");

        MTKFB_MMP_Events.UsingBufIdx[0][0] = MMProfileRegisterEvent(MTKFB_MMP_Events.BypassOVL, "ovl_layer0");
        MTKFB_MMP_Events.UsingBufIdx[0][1] = MMProfileRegisterEvent(MTKFB_MMP_Events.BypassOVL, "ovl_layer1");
        MTKFB_MMP_Events.UsingBufIdx[0][2] = MMProfileRegisterEvent(MTKFB_MMP_Events.BypassOVL, "ovl_layer2");
        MTKFB_MMP_Events.UsingBufIdx[0][3] = MMProfileRegisterEvent(MTKFB_MMP_Events.BypassOVL, "ovl_layer3");
        MTKFB_MMP_Events.UsingBufIdx[1][0] = MMProfileRegisterEvent(MTKFB_MMP_Events.BypassOVL, "rdma_layer0");
        MTKFB_MMP_Events.UsingBufIdx[1][1] = MMProfileRegisterEvent(MTKFB_MMP_Events.BypassOVL, "rdma_layer1");
        MTKFB_MMP_Events.UsingBufIdx[1][2] = MMProfileRegisterEvent(MTKFB_MMP_Events.BypassOVL, "rdma_layer2");
        MTKFB_MMP_Events.UsingBufIdx[1][3] = MMProfileRegisterEvent(MTKFB_MMP_Events.BypassOVL, "rdma_layer3");
        
        MTKFB_MMP_Events.MaxCleanIdx[0] = MMProfileRegisterEvent(MTKFB_MMP_Events.BypassOVL, "MaxCleanIdx0");
        MTKFB_MMP_Events.MaxCleanIdx[1] = MMProfileRegisterEvent(MTKFB_MMP_Events.BypassOVL, "MaxCleanIdx1");
        MTKFB_MMP_Events.MaxCleanIdx[2] = MMProfileRegisterEvent(MTKFB_MMP_Events.BypassOVL, "MaxCleanIdx2");
        MTKFB_MMP_Events.MaxCleanIdx[3] = MMProfileRegisterEvent(MTKFB_MMP_Events.BypassOVL, "MaxCleanIdx3");

        
        MTKFB_MMP_Events.EsdCheck = MMProfileRegisterEvent(MTKFB_MMP_Events.UpdateConfig, "EsdCheck");
        MTKFB_MMP_Events.ConfigOVL = MMProfileRegisterEvent(MTKFB_MMP_Events.UpdateConfig, "ConfigOVL");
        MTKFB_MMP_Events.ConfigWDMA = MMProfileRegisterEvent(MTKFB_MMP_Events.UpdateConfig, "ConfigWDMA");
        MTKFB_MMP_Events.ConfigAAL = MMProfileRegisterEvent(MTKFB_MMP_Events.UpdateConfig, "ConfigAAL");
        MTKFB_MMP_Events.ConfigMemOut = MMProfileRegisterEvent(MTKFB_MMP_Events.UpdateConfig, "ConfigMemOut");
        MTKFB_MMP_Events.ScreenUpdate = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "ScreenUpdate");
        MTKFB_MMP_Events.CaptureFramebuffer = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "CaptureFB");
        MTKFB_MMP_Events.TrigOverlayOut = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "TrigOverlayOut");
        MTKFB_MMP_Events.RegUpdate = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "RegUpdate");
        MTKFB_MMP_Events.OverlayOutDone = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "OverlayOutDone");
        MTKFB_MMP_Events.SwitchMode = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "SwitchMode");
        MTKFB_MMP_Events.EarlySuspend = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "EarlySuspend");
        MTKFB_MMP_Events.DispDone = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "DispDone");
        MTKFB_MMP_Events.DSICmd = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "DSICmd");
        MTKFB_MMP_Events.DSIIRQ = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "DSIIrq");
        MTKFB_MMP_Events.WaitVSync = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "WaitVSync");
        MTKFB_MMP_Events.LayerDump = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "LayerDump");
        MTKFB_MMP_Events.Layer[0] = MMProfileRegisterEvent(MTKFB_MMP_Events.LayerDump, "Layer0");
        MTKFB_MMP_Events.Layer[1] = MMProfileRegisterEvent(MTKFB_MMP_Events.LayerDump, "Layer1");
        MTKFB_MMP_Events.Layer[2] = MMProfileRegisterEvent(MTKFB_MMP_Events.LayerDump, "Layer2");
        MTKFB_MMP_Events.Layer[3] = MMProfileRegisterEvent(MTKFB_MMP_Events.LayerDump, "Layer3");
        MTKFB_MMP_Events.OvlDump = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "OvlDump");
        MTKFB_MMP_Events.FBDump = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "FBDump");
        MTKFB_MMP_Events.DSIRead = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "DSIRead");
        MTKFB_MMP_Events.GetLayerInfo = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "GetLayerInfo");
        MTKFB_MMP_Events.LayerInfo[0] = MMProfileRegisterEvent(MTKFB_MMP_Events.GetLayerInfo, "LayerInfo0");
        MTKFB_MMP_Events.LayerInfo[1] = MMProfileRegisterEvent(MTKFB_MMP_Events.GetLayerInfo, "LayerInfo1");
        MTKFB_MMP_Events.LayerInfo[2] = MMProfileRegisterEvent(MTKFB_MMP_Events.GetLayerInfo, "LayerInfo2");
        MTKFB_MMP_Events.LayerInfo[3] = MMProfileRegisterEvent(MTKFB_MMP_Events.GetLayerInfo, "LayerInfo3");
        MTKFB_MMP_Events.IOCtrl = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "IOCtrl");
        MTKFB_MMP_Events.Debug = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "Debug");
        MMProfileEnableEventRecursive(MTKFB_MMP_Events.MTKFB, 1);

    }
}

static __inline int is_layer_enable(unsigned int roi_ctl, unsigned int layer)
{
    return (roi_ctl >> (31 - layer)) & 0x1;
}

static void dump_layer_info(void)
{
    unsigned int i;
    for(i=0;i<4;i++){
        printk("LayerInfo in LCD driver, layer=%d,layer_en=%d, source=%d, fmt=%d, addr=0x%x, x=%d, y=%d \n\
                    w=%d, h=%d, pitch=%d, keyEn=%d, key=%d, aen=%d, alpha=%d \n ", 
                    cached_layer_config[i].layer,   // layer
                    cached_layer_config[i].layer_en,
                    cached_layer_config[i].source,   // data source (0=memory)
                    cached_layer_config[i].fmt, 
                    cached_layer_config[i].addr, // addr 
                    cached_layer_config[i].dst_x,  // x
                    cached_layer_config[i].dst_y,  // y
                    cached_layer_config[i].dst_w, // width
                    cached_layer_config[i].dst_h, // height
                    cached_layer_config[i].src_pitch, //pitch, pixel number
                    cached_layer_config[i].keyEn,  //color key
                    cached_layer_config[i].key,  //color key
                    cached_layer_config[i].aen, // alpha enable
                    cached_layer_config[i].alpha);	
    }
}


// ---------------------------------------------------------------------------
//  FPS Log
// ---------------------------------------------------------------------------

typedef struct {
    long int current_lcd_time_us;
    long int current_te_delay_time_us;
    long int total_lcd_time_us;
    long int total_te_delay_time_us;
    long int start_time_us;
    long int trigger_lcd_time_us;
    unsigned int trigger_lcd_count;

    long int current_hdmi_time_us;
    long int total_hdmi_time_us;
    long int hdmi_start_time_us;
    long int trigger_hdmi_time_us;
    unsigned int trigger_hdmi_count;
} FPS_LOGGER;

static FPS_LOGGER fps = {0};
static FPS_LOGGER hdmi_fps = {0};

static long int get_current_time_us(void)
{
    struct timeval t;
    do_gettimeofday(&t);
    return (t.tv_sec & 0xFFF) * 1000000 + t.tv_usec;
}


static void __inline reset_fps_logger(void)
{
    memset(&fps, 0, sizeof(fps));
}

static void __inline reset_hdmi_fps_logger(void)
{
    memset(&hdmi_fps, 0, sizeof(hdmi_fps));
}

void DBG_OnTriggerLcd(void)
{
    if (!dbg_opt.en_fps_log && !dbg_opt.en_touch_latency_log) return;
    
    fps.trigger_lcd_time_us = get_current_time_us();
    if (fps.trigger_lcd_count == 0) {
        fps.start_time_us = fps.trigger_lcd_time_us;
    }
}

void DBG_OnTriggerHDMI(void)
{
    if (!dbg_opt.en_fps_log && !dbg_opt.en_touch_latency_log) return;
    
    hdmi_fps.trigger_hdmi_time_us = get_current_time_us();
    if (hdmi_fps.trigger_hdmi_count == 0) {
        hdmi_fps.hdmi_start_time_us = hdmi_fps.trigger_hdmi_time_us;
    }
}

void DBG_OnTeDelayDone(void)
{
    long int time;
    
    if (!dbg_opt.en_fps_log && !dbg_opt.en_touch_latency_log) return;

    time = get_current_time_us();
    fps.current_te_delay_time_us = (time - fps.trigger_lcd_time_us);
    fps.total_te_delay_time_us += fps.current_te_delay_time_us;
}


void DBG_OnLcdDone(void)
{
    long int time;

    if (!dbg_opt.en_fps_log && !dbg_opt.en_touch_latency_log) return;

    // deal with touch latency log

    time = get_current_time_us();
    fps.current_lcd_time_us = (time - fps.trigger_lcd_time_us);

#if 0   // FIXME
    if (dbg_opt.en_touch_latency_log && tpd_start_profiling) {

        DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "Touch Latency: %ld ms\n", 
               (time - tpd_last_down_time) / 1000);

        DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "LCD update time %ld ms (TE delay %ld ms + LCD %ld ms)\n",
               fps.current_lcd_time_us / 1000,
               fps.current_te_delay_time_us / 1000,
               (fps.current_lcd_time_us - fps.current_te_delay_time_us) / 1000);
        
        tpd_start_profiling = 0;
    }
#endif

    if (!dbg_opt.en_fps_log) return;

    // deal with fps log
        
    fps.total_lcd_time_us += fps.current_lcd_time_us;
    ++ fps.trigger_lcd_count;

    if (fps.trigger_lcd_count >= dbg_opt.log_fps_wnd_size) {

        long int f = fps.trigger_lcd_count * 100 * 1000 * 1000 
                     / (time - fps.start_time_us);

        long int update = fps.total_lcd_time_us * 100 
                          / (1000 * fps.trigger_lcd_count);

        long int te = fps.total_te_delay_time_us * 100 
                      / (1000 * fps.trigger_lcd_count);

        long int lcd = (fps.total_lcd_time_us - fps.total_te_delay_time_us) * 100
                       / (1000 * fps.trigger_lcd_count);

        DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "MTKFB FPS: %ld.%02ld, Avg. update time: %ld.%02ld ms "
               "(TE delay %ld.%02ld ms, LCD %ld.%02ld ms)\n",
               f / 100, f % 100,
               update / 100, update % 100,
               te / 100, te % 100,
               lcd / 100, lcd % 100);
        reset_fps_logger();
    }
}

void DBG_OnHDMIDone(void)
{
    long int time;

    if (!dbg_opt.en_fps_log && !dbg_opt.en_touch_latency_log) return;

    // deal with touch latency log

    time = get_current_time_us();
    hdmi_fps.current_hdmi_time_us = (time - hdmi_fps.trigger_hdmi_time_us);


    if (!dbg_opt.en_fps_log) return;

    // deal with fps log
        
    hdmi_fps.total_hdmi_time_us += hdmi_fps.current_hdmi_time_us;
    ++ hdmi_fps.trigger_hdmi_count;

    if (hdmi_fps.trigger_hdmi_count >= dbg_opt.log_fps_wnd_size) {

        long int f = hdmi_fps.trigger_hdmi_count * 100 * 1000 * 1000 
                     / (time - hdmi_fps.hdmi_start_time_us);

        long int update = hdmi_fps.total_hdmi_time_us * 100 
                          / (1000 * hdmi_fps.trigger_hdmi_count);

        DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "[HDMI] FPS: %ld.%02ld, Avg. update time: %ld.%02ld ms\n",
               f / 100, f % 100,
               update / 100, update % 100);
        
        reset_hdmi_fps_logger();
    }
}

// ---------------------------------------------------------------------------
//  Command Processor
// ---------------------------------------------------------------------------
extern void mtkfb_clear_lcm(void);
extern void hdmi_force_init(void);
extern unsigned int mtkfb_fm_auto_test(void);
extern UINT32 color;
// 苏 勇 2012年09月27日 15:21:33void LCDWriteIC(unsigned long COM, unsigned short Reg);
// 苏 勇 2012年09月28日 14:50:38void LCDReadIC(unsigned char reg ,unsigned char num ,unsigned char * buffer);

static void process_dbg_opt(const char *opt)
{
    if (0 == strncmp(opt, "hdmion", 6))
    {
        //	hdmi_force_init();
    }
    else if (0 == strncmp(opt, "fps:", 4))
    {
        if (0 == strncmp(opt + 4, "on", 2)) {
            dbg_opt.en_fps_log = 1;
        } else if (0 == strncmp(opt + 4, "off", 3)) {
            dbg_opt.en_fps_log = 0;
        } else {
            goto Error;
        }
        reset_fps_logger();
    }
    else if (0 == strncmp(opt, "tl:", 3))
    {
        if (0 == strncmp(opt + 3, "on", 2)) {
            dbg_opt.en_touch_latency_log = 1;
        } else if (0 == strncmp(opt + 3, "off", 3)) {
            dbg_opt.en_touch_latency_log = 0;
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "black", 5))
    {
        mtkfb_clear_lcm();
    }
    else if (0 == strncmp(opt, "suspend", 4))
    {
        DISP_PanelEnable(FALSE);
        DISP_PowerEnable(FALSE);
    }
    else if (0 == strncmp(opt, "resume", 4))
    {
        DISP_PowerEnable(TRUE);
        DISP_PanelEnable(TRUE);
    }
    else if (0 == strncmp(opt, "lcm:", 4))
    {
        if (0 == strncmp(opt + 4, "on", 2)) {
            DISP_PanelEnable(TRUE);
        } else if (0 == strncmp(opt + 4, "off", 3)) {
            DISP_PanelEnable(FALSE);
        }
        else if (0 == strncmp(opt + 4, "init", 4)) {
            if (NULL != lcm_drv && NULL != lcm_drv->init) {
                lcm_drv->init();
            }
        }else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "cabc:", 5))
    {
        if (0 == strncmp(opt + 5, "ui", 2)) {
            mtkfb_set_backlight_mode(1);
        }else if (0 == strncmp(opt + 5, "mov", 3)) {
            mtkfb_set_backlight_mode(3);
        }else if (0 == strncmp(opt + 5, "still", 5)) {
            mtkfb_set_backlight_mode(2);
        }else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "lcd:", 4))
    {
        if (0 == strncmp(opt + 4, "on", 2)) {
            DISP_PowerEnable(TRUE);
        } else if (0 == strncmp(opt + 4, "off", 3)) {
            DISP_PowerEnable(FALSE);
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "vsynclog:", 9))
    {
        if (0 == strncmp(opt + 9, "on", 2))
        {
            EnableVSyncLog = 1;
        } else if (0 == strncmp(opt + 9, "off", 3)) 
        {
            EnableVSyncLog = 0;
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "layer", 5))
    {
        dump_layer_info();
    }
    else if (0 == strncmp(opt, "fm_test:", 8))
    {
        unsigned int ret = 0;
        char *p = (char *)opt + 8;
        unsigned int level = (unsigned int) simple_strtoul(p, &p, 10);
        color = level;
        ret = mtkfb_fm_auto_test();
        printk("ret = %d, 0x%x\n", ret,color);
    }
    else if (0 == strncmp(opt, "regw:", 5))
    {
        char *p = (char *)opt + 5;
        unsigned long addr = simple_strtoul(p, &p, 16);
        unsigned long val  = simple_strtoul(p + 1, &p, 16);

        if (addr) {
            OUTREG32(addr, val);
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "regr:", 5))
    {
        char *p = (char *)opt + 5;
        unsigned int addr = (unsigned int) simple_strtoul(p, &p, 16);

        if (addr) {
            DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "Read register 0x%08x: 0x%08x\n", addr, INREG32(addr));
        } else {
            goto Error;
        }
    }
    else if(0 == strncmp(opt, "bkl:", 4))
    {
        char *p = (char *)opt + 4;
        unsigned int level = (unsigned int) simple_strtoul(p, &p, 10);

        DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "process_dbg_opt(), set backlight level = %d\n", level);
        DISP_SetBacklight(level);
    }
    else if(0 == strncmp(opt, "dither:", 7))
    {
        unsigned lrs, lgs, lbs, dbr, dbg, dbb;
        char *p = (char *)opt + 7;

        lrs = (unsigned int) simple_strtoul(p, &p, 16);
        p++;
        lgs = (unsigned int) simple_strtoul(p, &p, 16);
        p++;
        lbs = (unsigned int) simple_strtoul(p, &p, 16);
        p++;
        dbr = (unsigned int) simple_strtoul(p, &p, 16);
        p++;
        dbg = (unsigned int) simple_strtoul(p, &p, 16);
        p++;
        dbb = (unsigned int) simple_strtoul(p, &p, 16);
        
        DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "process_dbg_opt(), %d %d %d %d %d %d\n", lrs, lgs, lbs, dbr, dbg, dbb);
    }
    else if (0 == strncmp(opt, "mtkfblog:", 9))
    {
        if (0 == strncmp(opt + 9, "on", 2)) {
            mtkfb_log_enable(true);
        } else if (0 == strncmp(opt + 9, "off", 3)) {
            mtkfb_log_enable(false);
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "displog:", 8))
    {
        if (0 == strncmp(opt + 8, "on", 2)) {
            disp_log_enable(true);
        } else if (0 == strncmp(opt + 8, "off", 3)) {
            disp_log_enable(false);
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "mtkfb_vsynclog:", 15))
    {
        if (0 == strncmp(opt + 15, "on", 2)) {
            mtkfb_vsync_log_enable(true);
        } else if (0 == strncmp(opt + 15, "off", 3)) {
            mtkfb_vsync_log_enable(false);
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "log:", 4))
    {
        if (0 == strncmp(opt + 4, "on", 2)) {
            mtkfb_log_enable(true);
            disp_log_enable(true);
        } else if (0 == strncmp(opt + 4, "off", 3)) {
            mtkfb_log_enable(false);
            disp_log_enable(false);
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "update", 6))
    {
        DISP_UpdateScreen(0, 0, DISP_GetScreenWidth(), DISP_GetScreenHeight());
    }
    else if (0 == strncmp(opt, "pan_disp", 8))
    {
        mtkfb_pan_disp_test();
    }
    else if (0 == strncmp(opt, "sem_cnt", 7))
    {
        mtkfb_show_sem_cnt();
    }
    else if (0 == strncmp(opt, "hang:", 5))
    {
        if (0 == strncmp(opt + 5, "on", 2)) {
            mtkfb_hang_test(true);
        } else if (0 == strncmp(opt + 5, "off", 3)) {
            mtkfb_hang_test(false);
        } else{
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "cpfbonly:", 9))
    {
        if (0 == strncmp(opt + 9, "on", 2))
        {
            mtkfb_capture_fb_only(true);
        }
        else if (0 == strncmp(opt + 9, "off", 3))
        {
            mtkfb_capture_fb_only(false);
        }
    }
    else if (0 == strncmp(opt, "esd:", 4))
    {
        if (0 == strncmp(opt + 4, "on", 2))
        {
            esd_recovery_pause(FALSE);
        }
        else if (0 == strncmp(opt + 4, "off", 3))
        {
            esd_recovery_pause(TRUE);
        }
    }
    else if (0 == strncmp(opt, "HQA:", 4))
    {
        if (0 == strncmp(opt + 4, "NormalToFactory", 15))
        {
            mtkfb_switch_normal_to_factory();
        }
        else if (0 == strncmp(opt + 4, "FactoryToNormal", 15))
        {
            mtkfb_switch_factory_to_normal();
        }
    }
    else if (0 == strncmp(opt, "mmp", 3))
    {
        init_mtkfb_mmp_events();
    }
    else if (0 == strncmp(opt, "dump_layer:", 11))
    {
        if (0 == strncmp(opt + 11, "on", 2))
        {
            char *p = (char *)opt + 14;
            gCaptureLayerDownX = simple_strtoul(p, &p, 10);
            gCaptureLayerDownY = simple_strtoul(p+1, &p, 10);
            gCaptureLayerEnable = 1;
            gCaptureLayerRawData = 0;
        }
        else if (0 == strncmp(opt + 11, "raw", 3))
        {
            gCaptureLayerDownX = 1;
            gCaptureLayerDownY = 1;
            gCaptureLayerEnable = 1;
            gCaptureLayerRawData = 1;
        }
        else if (0 == strncmp(opt + 11, "off", 3))
        {
            gCaptureLayerEnable = 0;
            gCaptureLayerRawData = 0;
        }
    }
    else if (0 == strncmp(opt, "dump_ovl:", 9))
    {
        if (0 == strncmp(opt + 9, "on", 2))
        {
            char *p = (char *)opt + 12;
            gCaptureOvlDownX = simple_strtoul(p, &p, 10);
            gCaptureOvlDownY = simple_strtoul(p+1, &p, 10);
            gCaptureOvlEnable = 1;
            gCaptureOvlRawData = 0;
        }
        else if (0 == strncmp(opt + 9, "raw", 3))
        {
            gCaptureOvlDownX = 1;
            gCaptureOvlDownY = 1;
            gCaptureOvlEnable = 1;
            gCaptureOvlRawData = 1;
        }
        else if (0 == strncmp(opt + 9, "off", 3))
        {
            gCaptureOvlEnable = 0;
            gCaptureOvlRawData = 0;
        }
    }
    else if (0 == strncmp(opt, "dump_fb:", 8))
    {
        if (0 == strncmp(opt + 8, "on", 2))
        {
            char *p = (char *)opt + 11;
            gCaptureFBDownX = simple_strtoul(p, &p, 10);
            gCaptureFBDownY = simple_strtoul(p+1, &p, 10);
            gCaptureFBEnable = 1;
            gCaptureFBRawData = 0;
        }
        else if (0 == strncmp(opt + 8, "raw", 3))
        {
            gCaptureFBDownX = 1;
            gCaptureFBDownY = 1;
            gCaptureFBEnable = 1;
            gCaptureFBRawData = 1;
        }
        else if (0 == strncmp(opt + 8, "off", 3))
        {
            gCaptureFBEnable = 0;
            gCaptureFBRawData = 0;
        }   
    }
    else if (0 == strncmp(opt, "decouple:", 9))
    {
        if (0 == strncmp(opt + 9, "on", 2))
        {
            struct fb_overlay_mode mode;

            mode.mode = DISP_DECOUPLE_MODE;
            DISP_SwitchDisplayMode(&mode);
        }
        else if (0 == strncmp(opt + 9, "off", 3))
        {
            struct fb_overlay_mode mode;

            mode.mode = DISP_DIRECT_LINK_MODE;
            DISP_SwitchDisplayMode(&mode);
        }
    }
	else if (0 == strncmp(opt, "wic:", 4))// 写lcd的ic的寄存器,或者你想做的其他事情LCDWriteIC,通常在对应的驱动中去定义 苏 勇 2012年09月27日15:10:38 
    {
        char *p = (char *)opt + 4;
        unsigned long addr = simple_strtoul(p, &p, 16);
        unsigned long val  = simple_strtoul(p + 1, &p, 16);
		DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG","write ic register 0x%08x: 0x%08x\n", addr, val);
        if (addr) 
        {
// 苏 勇 2012年09月27日 15:20:21            LCDWriteIC(addr, val);
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "ric:", 4)) // 读取ic,dsi的接口可以使用类似echo ric:4=3>mtkfb命令读取,但log在kmsg中 苏 勇 2012年09月28日 14:50:21
    {
        char *p = (char *)opt + 4;
        unsigned int addr = (unsigned int) simple_strtoul(p, &p, 16);
        unsigned long num  = simple_strtoul(p + 1, &p, 16);
        unsigned char buffer[10]={0};
        
        if(num > (sizeof(buffer)/sizeof(unsigned char)) )
    	{
    		goto Error;
    	}
        if (addr) {
        	int i;
// 苏 勇 2012年09月28日 14:49:26        	LCDReadIC(addr,num,buffer);
        	
        	DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "Read register 0x%08x", addr);
        	for (i=0;i<num;i++)
        	{
            	DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "0x%08x ", buffer[i]);
            }
        } else {
            goto Error;
        }
    }

    else
    {
        if (disphal_process_dbg_opt(opt))
            goto Error;
    }

    return;

Error:
    DISP_LOG_PRINT(ANDROID_LOG_INFO, "ERROR", "parse command error!\n\n%s", STR_HELP);
}


static void process_dbg_cmd(char *cmd)
{
    char *tok;
    
    DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "[mtkfb_dbg] %s\n", cmd);
    
    while ((tok = strsep(&cmd, " ")) != NULL)
    {
        process_dbg_opt(tok);
    }
}


// ---------------------------------------------------------------------------
//  Debug FileSystem Routines
// ---------------------------------------------------------------------------

struct dentry *mtkfb_dbgfs = NULL;


static ssize_t debug_open(struct inode *inode, struct file *file)
{
    file->private_data = inode->i_private;
    return 0;
}


static char debug_buffer[2048];

static ssize_t debug_read(struct file *file,
                          char __user *ubuf, size_t count, loff_t *ppos)
{
    const int debug_bufmax = sizeof(debug_buffer) - 1;
    int n = 0;

    n += scnprintf(debug_buffer + n, debug_bufmax - n, STR_HELP);
    debug_buffer[n++] = 0;

    return simple_read_from_buffer(ubuf, count, ppos, debug_buffer, n);
}


static ssize_t debug_write(struct file *file,
                           const char __user *ubuf, size_t count, loff_t *ppos)
{
    const int debug_bufmax = sizeof(debug_buffer) - 1;
    size_t ret;

    ret = count;

    if (count > debug_bufmax) 
        count = debug_bufmax;

    if (copy_from_user(&debug_buffer, ubuf, count))
        return -EFAULT;

    debug_buffer[count] = 0;

    process_dbg_cmd(debug_buffer);

    return ret;
}


static struct file_operations debug_fops = {
    .read  = debug_read,
    .write = debug_write,
    .open  = debug_open,
};

#ifdef MTKFB_DEBUG_FS_CAPTURE_LAYER_CONTENT_SUPPORT

static ssize_t layer_debug_open(struct inode *inode, struct file *file)
{
    MTKFB_LAYER_DBG_OPTIONS *dbgopt;
    ///record the private data
    file->private_data = inode->i_private;
    dbgopt = (MTKFB_LAYER_DBG_OPTIONS *)file->private_data;

    dbgopt->working_size = DISP_GetScreenWidth()*DISP_GetScreenHeight()*2 + 32;
    dbgopt->working_buf = (UINT32)vmalloc(dbgopt->working_size);
    if(dbgopt->working_buf == 0)
        DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "Vmalloc to get temp buffer failed\n");

    return 0;
}


static ssize_t layer_debug_read(struct file *file,
                          char __user *ubuf, size_t count, loff_t *ppos)
{
    return 0;
}


static ssize_t layer_debug_write(struct file *file,
                           const char __user *ubuf, size_t count, loff_t *ppos)
{
    MTKFB_LAYER_DBG_OPTIONS *dbgopt = (MTKFB_LAYER_DBG_OPTIONS *)file->private_data;

    DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "mtkfb_layer%d write is not implemented yet \n", dbgopt->layer_index);

    return count;
}

static int layer_debug_release(struct inode *inode, struct file *file)
{
    MTKFB_LAYER_DBG_OPTIONS *dbgopt;
    dbgopt = (MTKFB_LAYER_DBG_OPTIONS *)file->private_data;

    if(dbgopt->working_buf != 0)
        vfree((void *)dbgopt->working_buf);

    dbgopt->working_buf = 0;

    return 0;
}


static struct file_operations layer_debug_fops = {
    .read  = layer_debug_read,
    .write = layer_debug_write,
    .open  = layer_debug_open,
    .release = layer_debug_release,
};

#endif

void DBG_Init(void)
{
    mtkfb_dbgfs = debugfs_create_file("mtkfb",
                                                               S_IFREG|S_IRUGO, NULL, (void *)0, &debug_fops);

    memset(&dbg_opt, sizeof(dbg_opt), 0);
    memset(&fps, sizeof(fps), 0);
    dbg_opt.log_fps_wnd_size = DEFAULT_LOG_FPS_WND_SIZE;

#ifdef MTKFB_DEBUG_FS_CAPTURE_LAYER_CONTENT_SUPPORT
    {
        unsigned int i;
        unsigned char a[13];

        a[0] = 'm';
        a[1] = 't';
        a[2] = 'k';
        a[3] = 'f';
        a[4] = 'b';
        a[5] = '_';
        a[6] = 'l';
        a[7] = 'a';
        a[8] = 'y';
        a[9] = 'e';
        a[10] = 'r';
        a[11] = '0';
        a[12] = '\0';
        
        for(i=0;i<DDP_OVL_LAYER_MUN;i++)
        {
            a[11] = '0' + i;
            mtkfb_layer_dbg_opt[i].layer_index = i;
            mtkfb_layer_dbgfs[i] = debugfs_create_file(a,
                S_IFREG|S_IRUGO, NULL, (void *)&mtkfb_layer_dbg_opt[i], &layer_debug_fops);
        }
    }
#endif    
}


void DBG_Deinit(void)
{
    debugfs_remove(mtkfb_dbgfs);
#ifdef MTKFB_DEBUG_FS_CAPTURE_LAYER_CONTENT_SUPPORT
    {
        unsigned int i;
        
        for(i=0;i<DDP_OVL_LAYER_MUN;i++)
            debugfs_remove(mtkfb_layer_dbgfs[i]);
    }
#endif
}

