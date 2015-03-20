
#ifndef __mt6573_DISP_DRV_H__
#define __mt6573_DISP_DRV_H__

#include <platform/mt_typedefs.h>
#include <platform/disp_drv.h>
#include "lcm_drv.h"

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
//  UBoot Display Utility Macros
// ---------------------------------------------------------------------------

#define AS_UINT32(x)    (*(unsigned int *)(x))
#define AS_UINT16(x)    (*(unsigned short *)(x))

// ---------------------------------------------------------------------------

#define NOT_REFERENCED(x) {(x) = (x);}
#define msleep(x)    mdelay(x)
#define printk  printf

// ---------------------------------------------------------------------------
typedef enum {
   DISP_VERTICAL_PROG_BAR = 0,
   DISP_HORIZONTAL_PROG_BAR,
} DISP_PROG_BAR_DIRECT;



   

// ---------------------------------------------------------------------------
//  UBoot Display Export Functions
// ---------------------------------------------------------------------------

UINT32 mt_disp_get_vram_size(void);
void   mt_disp_init(void *lcdbase);
void   mt_disp_power(BOOL on);
void   mt_disp_update(UINT32 x, UINT32 y, UINT32 width, UINT32 height);
void   mt_disp_wait_idle(void);
UINT32 mt_disp_get_lcd_time(void);

BOOL DISP_DetectDevice(void);

// -- Utility Functions for Customization --

void*  mt_get_logo_db_addr(void);
void*  mt_get_fb_addr(void);
void*  mt_get_tempfb_addr(void);
UINT32 mt_get_fb_size(void);

void mt_disp_fill_rect(UINT32 left, UINT32 top,
                           UINT32 right, UINT32 bottom,
                           UINT32 color);

void mt_disp_draw_prog_bar(DISP_PROG_BAR_DIRECT direct,
                               UINT32 left, UINT32 top,
                               UINT32 right, UINT32 bottom,
                               UINT32 fgColor, UINT32 bgColor,
                               UINT32 start_div, UINT32 total_div,
                               UINT32 occupied_div);

#ifdef __cplusplus
}
#endif

#endif // __mt65xx_DISP_DRV_H__


