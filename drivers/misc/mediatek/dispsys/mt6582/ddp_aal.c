#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/leds-mt65xx.h>
#include "ddp_reg.h"
#include "ddp_drv.h"
#include "ddp_color.h"
#include "ddp_aal.h"
#include "ddp_bls.h"

static DECLARE_WAIT_QUEUE_HEAD(g_disp_hist_alarm);

static unsigned int g_Alarm = 0;
int g_AAL_NewFrameUpdate = 0;

/*
 *g_AAL_Param is protected by disp_set_needupdate
 */
static DISP_AAL_PARAM g_AAL_Param = 
{
    .lumaCurve = { 0, 32, 64, 96, 128, 160, 192, 224, 256, 
                   288, 320, 352, 384, 416, 448, 480, 511},
    .pwmDuty = 0,
    .setting = 0,
    .maxClrLimit = 0,
    .maxClrDistThd = 0,
    .preDistLimit = 0,
    .preDistThd = 0,
};

static int g_aal_service_running = 0;
static int g_Configured = 0;
static int g_ColorIntFlag = 0;

static unsigned long g_PrevPWMDuty = 0;

unsigned long long g_PrevTime = 0;
unsigned long long g_CurrTime = 0;

extern unsigned char aal_debug_flag;

int disp_wait_hist_update(unsigned long u4TimeOut_ms)
{
    int ret;

    g_aal_service_running = 1;
    ret = wait_event_interruptible(g_disp_hist_alarm , (1 == g_ColorIntFlag));
    g_ColorIntFlag = 0;

    return 0;
}


void disp_set_aal_alarm(unsigned int u4En)
{
    g_Alarm = u4En;
}

//Executed in ISR content
int disp_needWakeUp(void)
{
    if (aal_debug_flag == 1)
        return 0;
    else
        return (g_AAL_NewFrameUpdate || g_Alarm) ? 1 : 0;
}

//Executed in ISR content
void on_disp_aal_alarm_set(void)
{
    if(disp_needWakeUp())
    {
        // enable interrupt
        //DISP_REG_SET((DISPSYS_COLOR_BASE + 0xf04), 0x00000007);
        DISP_REG_SET(DISP_REG_BLS_INTEN, 0x0000000F);
        g_AAL_NewFrameUpdate = 0;
    }
    else
    {
        // disable interrupt
        //DISP_REG_SET((DISPSYS_COLOR_BASE + 0xf04), 0x00000000);
        if (g_PrevPWMDuty == DISP_REG_GET(DISP_REG_BLS_PWM_DUTY))
            DISP_REG_SET(DISP_REG_BLS_INTEN, 0x00000000);
        else
            g_PrevPWMDuty = DISP_REG_GET(DISP_REG_BLS_PWM_DUTY);
    }
}

unsigned int is_disp_aal_alarm_on(void)
{
    return g_Alarm;
}

//Executed in ISR content
void disp_wakeup_aal(void)
{
    unsigned long long u8Delta = 0;
    g_CurrTime = sched_clock();
    u8Delta = (g_CurrTime > g_PrevTime ? (g_CurrTime - g_PrevTime) : g_CurrTime);
    
//    if( 28000000 < u8Delta )
    {
        mb();
        g_ColorIntFlag = 1;
        wake_up_interruptible(&g_disp_hist_alarm);
        g_PrevTime = g_CurrTime;
    }

}

void disp_aal_reset()
{
    g_Configured = 0;
    g_Alarm = 0;
    g_PrevPWMDuty = 0;
    g_PrevTime = 0;
    g_CurrTime = 0;
}

DISP_AAL_PARAM * get_aal_config()
{
    g_Configured = 1;
    return &g_AAL_Param;
}

int disp_is_aal_config()
{
    return g_Configured;
}

void disp_onConfig_aal(int i4FrameUpdate)
{
    if (i4FrameUpdate)
        g_AAL_NewFrameUpdate = 1;
    
    if(0 == g_Configured)
    {
        return;
    }

    disp_onConfig_luma(g_AAL_Param.lumaCurve);

    disp_onConfig_bls(&g_AAL_Param);

    /*
     * Do not call LED API to set backlight here, bacause
     * the API may take mutex.
     */
}


static DISP_AAL_STATISTICS gHist = {
    .backlight = -1
};
static DEFINE_SPINLOCK(gHistLock);
static volatile int g_aal_hist_read_lock = 0;


void disp_set_hist_readlock(unsigned long bLock)
{
    unsigned long flag;

    spin_lock_irqsave(&gHistLock, flag);
    g_aal_hist_read_lock = (int)bLock;
    spin_unlock_irqrestore(&gHistLock, flag);
}

int disp_get_hist(unsigned int * pHist)
{
    disp_set_hist_readlock(1);

    memcpy(pHist , gHist.histogram  , (LUMA_HIST_BIN*sizeof(unsigned int)));
    
    disp_set_hist_readlock(0);
    return 0;
}

DISP_AAL_STATISTICS * disp_get_hist_ptr()
{
    return &gHist;
}

void disp_update_hist()
{
    int i = 0;

    if (!g_aal_hist_read_lock) {
        for (i = 0; i < LUMA_HIST_BIN; i++)
        {
            gHist.histogram[i] = DISP_REG_GET(DISPSYS_COLOR_BASE + 0x0520 + i * 4);
        }

        gHist.ChromHist = DISP_REG_GET(DISPSYS_COLOR_BASE + 0x74C);

        for (i = 0; i < BLS_HIST_BIN; i++)
        {
            gHist.BLSHist[i] = DISP_REG_GET(DISPSYS_BLS_BASE + 0x100 + i * 4);
        }
    }
}


void disp_onConfig_luma(unsigned long * luma)
{
    unsigned long u4Index;
    unsigned long u4Val;
    
    for(u4Index = 0 ; u4Index < 8; u4Index += 1)
    {
        u4Val = luma[u4Index*2] | luma[u4Index*2+1] << 16;
        DISP_REG_SET((DISPSYS_COLOR_BASE + 0x440 + (u4Index << 2)), u4Val);
    }
    DISP_REG_SET((DISPSYS_COLOR_BASE + 0x440 + (8 << 2)), luma[16]);  
}


void disp_aal_notify_backlight_changed(int bl_1024)
{
    unsigned long flag;
    int max_backlight;

    printk(KERN_DEBUG "[BLS] disp_aal_notify_backlight_changed: %d", bl_1024);

    max_backlight = disp_bls_get_max_backlight();
    if (bl_1024 > max_backlight)
        bl_1024 = max_backlight;

    spin_lock_irqsave(&gHistLock , flag);
    gHist.backlight = bl_1024;
    spin_unlock_irqrestore(&gHistLock , flag);

    if (bl_1024 == 0) { // Turn off backlight
        backlight_brightness_set(0);
    } else if (!g_aal_service_running) {
        backlight_brightness_set(bl_1024);
    }

    if (g_aal_service_running) {
        disp_set_aal_alarm(1);
    }
}


