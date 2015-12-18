#define LOG_TAG "MET" 

#include <linux/met_drv.h>
#include "ddp_hal.h"
#include "ddp_debug.h"
#include "ddp_reg.h"
#include "ddp_met.h"
#include "ddp_ovl.h"
#include "ddp_rdma.h"

#define DDP_IRQ_EER_ID 				(0xFFFF0000)
#define DDP_IRQ_FPS_ID 				(DDP_IRQ_EER_ID + 1)
#define DDP_IRQ_LAYER_FPS_ID 		(DDP_IRQ_EER_ID + 2)
#define DDP_IRQ_LAYER_SIZE_ID 		(DDP_IRQ_EER_ID + 3)
#define DDP_IRQ_LAYER_FORMAT_ID 	(DDP_IRQ_EER_ID + 4)

#define OVL_NUM (1)
#define RDMA_NUM (1)
#define MAX_OVL_LAYERS (4)
#define OVL_LAYER_NUM_PER_OVL (4)

unsigned int met_tag_on = 0;


 typedef struct _OVL_BASIC_STRUCT
 {
     unsigned int layer;
     unsigned int layer_en;
     unsigned int fmt;
     unsigned long addr;
     unsigned int src_w;
     unsigned int src_h;
     unsigned int src_pitch;
     unsigned int bpp;
 }OVL_BASIC_STRUCT;
 
 typedef struct _RDMA_BASIC_STRUCT
 {
     unsigned long addr;
     unsigned int src_w;
     unsigned int src_h;
     unsigned int bpp;
 }RDMA_BASIC_STRUCT;

 char* ddp_get_bpp(DISP_MODULE_ENUM module, unsigned int fmt)
 {

    if(module==DISP_MODULE_OVL)
    {
        switch(fmt)
        {
            case 0:  return 3;//"rgb888";
            case 1:  return 2;//"rgb565";
            case 2:  return 4;//"argb8888";
            case 3:  return 4;//"pargb8888";
            case 4:  return 4;//"xrgb8888";              
            case 8:  return 2;//"yuyv";
            case 9:  return 2;//"uvvy";
            case 10: return 2;//"yvyu"; 
            case 11: return 2;//"vyuy";
            case 15: return 3;//"yuv444";                                            
            default: 
                DISP_MSG("ddp_get_fmt_name, unknown fmt=%d, module=%d \n", fmt, module);
                return 4; 
        }   
    }
    else if(module==DISP_MODULE_RDMA) // todo: confirm with designers
    {
        switch(fmt)
        {
            case 0:  return 2;//"yuyv";
            case 1:  return 2;//"uyvy";
            case 2:  return 2;//"yvyu";
            case 3:  return 2;//"vyuy";
            case 4:  return 2;//"rgb565";
            case 8:  return 3;//"rgb888";
            case 16: return 4;//"argb8888";
            default: 
                DISP_MSG("ddp_get_fmt_name, unknown fmt=%d, module=%d \n", fmt, module);
                return 4; 
        }   
    }
}

 char* get_fmt_name(DISP_MODULE_ENUM module, unsigned int fmt)
{
   if(module==DISP_MODULE_OVL)
   {
       switch(fmt)
       {
           case 0:  return "rgb888";
           case 1:  return "rgb565";
           case 2:  return "argb8888";
           case 3:  return "pargb8888";
           case 4:  return "xrgb8888";           	
           case 8:  return "yuyv";
           case 9:  return "uvvy";
           case 10: return "yvyu"; 
           case 11: return "vyuy";
           case 15: return "yuv444";               	            	         	
           default: 
               DISP_MSG("ddp_get_fmt_name, unknown fmt=%d, module=%d \n", fmt, module);
               return "unknown"; 
       }   
   }
   else if(module==DISP_MODULE_RDMA) // todo: confirm with designers
   {
       switch(fmt)
       {
           case 0:  return "yuyv";
           case 1:  return "uyvy";
           case 2:  return "yvyu";
           case 3:  return "vyuy";
           case 4:  return "rgb565";
           case 8:  return "rgb888";
           case 16: return "argb8888";
           default: 
               DISP_MSG("ddp_get_fmt_name, unknown fmt=%d, module=%d \n", fmt, module);
               return "unknown"; 
       }   
   }  
   return "unknown";
}

 void rdma_get_info(int idx, RDMA_BASIC_STRUCT * info)
{
    RDMA_BASIC_STRUCT *p = info;
	p->addr  =     DISP_REG_GET(DISP_REG_RDMA_MEM_START_ADDR);
	p->src_w =     DISP_REG_GET(DISP_REG_RDMA_SIZE_CON_0)&0xfff;
	p->src_h =     DISP_REG_GET(DISP_REG_RDMA_SIZE_CON_1)&0xfffff,	   
    p->bpp   =     ddp_get_bpp(DISP_MODULE_RDMA, (DISP_REG_GET(DISP_REG_RDMA_MEM_CON)>>4)&0x3f);
	DISP_DBG("rdma_get_info:w %d,h %d, bpp %d,addr %lx\n",
	 p->src_w, p->src_h, p->bpp, p->addr);
    return ;
}

#define OVL_LAYER_OFFSET        (0x20)

void ovl_get_info(int idx, void *data)
{
	int i = 0;
	OVL_BASIC_STRUCT *pdata = (OVL_BASIC_STRUCT *)data;
	unsigned int idx_offset = 0;
	unsigned int layer_off = 0;
	unsigned int src_on = DISP_REG_GET(DISP_REG_OVL_SRC_CON + idx_offset);
	OVL_BASIC_STRUCT *p = NULL;

	memset(pdata, 0, sizeof(OVL_BASIC_STRUCT)*OVL_LAYER_NUM);    

	idx_offset = 0;
	src_on = DISP_REG_GET(DISP_REG_OVL_SRC_CON + idx_offset);
	for (i = 0; i < OVL_LAYER_NUM_PER_OVL; i++) {
		layer_off  = (i%4)*OVL_LAYER_OFFSET + idx_offset;
		p = &pdata[i];
		p->layer = i;
		p->layer_en = src_on & (0x1<<i);
		if (p->layer_en) {
			p->fmt = (DISP_REG_GET(layer_off+DISP_REG_OVL_L0_CON)>>12)&0xf;
			p->addr =  DISP_REG_GET(layer_off+DISP_REG_OVL_L0_ADDR);
			p->src_w = DISP_REG_GET(layer_off+DISP_REG_OVL_L0_SRC_SIZE)&0xfff;
			p->src_h = (DISP_REG_GET(layer_off+DISP_REG_OVL_L0_SRC_SIZE)>>16)&0xfff;
			p->src_pitch = DISP_REG_GET(layer_off+DISP_REG_OVL_L0_PITCH)&0xffff;
			p->bpp = ddp_get_bpp(DISP_MODULE_OVL, (DISP_REG_GET(layer_off+DISP_REG_OVL_L0_CON)>>12)&0xf);
		}
		DISP_DBG("ovl_get_info:layer%d,en %d,w %d,h %d, bpp %d,addr %lx\n",
			i, p->layer_en, p->src_w, p->src_h, p->bpp, p->addr);
	}

		
}

static const char* const parse_color_format(DpColorFormat fmt) {
	switch (fmt) {
	case eBGR565:
		return "eBGR565";
	case eRGB565:
		return "eRGB565";
	case eRGB888:
		return "eRGB888";
	case eBGR888:
		return "eBGR888";
	case eRGBA8888:
		return "eRGBA8888";
	case eBGRA8888:
		return "eBGRA8888";
	case eARGB8888:
		return "eARGB8888";
	case eABGR8888:
		return "eABGR8888";
	case eVYUY:
		return "eVYUY";
	case eUYVY:
		return "eUYVY";
	case eYVYU:
		return "eYVYU";
	case eYUY2:
		return "eYUY2";
	}
}

/**
 * Represent to LCM display refresh rate
 * Primary Display:  map to RDMA0 sof/eof ISR, for all display mode
 * External Display: map to RDMA1 sof/eof ISR, for all display mode
 * NOTICE:
 * 		for WFD, nothing we can do here
 */
static void ddp_disp_refresh_tag_start(unsigned int index) {
	static unsigned long sBufAddr[RDMA_NUM];
	static RDMA_BASIC_STRUCT rdmaInfo;
	char tag_name[30] = { '\0' };
	rdma_get_info(index, &rdmaInfo);
	if (rdmaInfo.addr == 0 || (rdmaInfo.addr != 0 && sBufAddr[index] != rdmaInfo.addr)) {
		sBufAddr[index] = rdmaInfo.addr;
		sprintf(tag_name, index ? "ExtDispRefresh" : "PrimDispRefresh");
		met_tag_oneshot(DDP_IRQ_FPS_ID, tag_name, 1);
	}
}

static void ddp_disp_refresh_tag_end(unsigned int index) {
	char tag_name[30] = { '\0' };
	sprintf(tag_name, index ? "ExtDispRefresh" : "PrimDispRefresh");
	met_tag_oneshot(DDP_IRQ_FPS_ID, tag_name, 0);
}

/**
 * Represent to OVL0/0VL1 each layer's refresh rate
 */
static void ddp_inout_info_tag(unsigned int index) {
	static unsigned long sLayerBufAddr[OVL_NUM][OVL_LAYER_NUM_PER_OVL];
	static unsigned int  sLayerBufFmt[OVL_NUM][OVL_LAYER_NUM_PER_OVL];
	static unsigned int  sLayerBufWidth[OVL_NUM][OVL_LAYER_NUM_PER_OVL];
	static unsigned int  sLayerBufHeight[OVL_NUM][OVL_LAYER_NUM_PER_OVL];

	OVL_BASIC_STRUCT ovlInfo[OVL_LAYER_NUM];
	unsigned int flag, i, idx, enLayerCnt, layerCnt;
	unsigned int width, height, bpp, fmt;
	char* fmtStr;
	char tag_name[30] = { '\0' };
	memset((void*)ovlInfo, 0, sizeof(ovlInfo));
	ovl_get_info(index, ovlInfo);

	//Any layer enable bit changes , new frame refreshes
    layerCnt = OVL_LAYER_NUM_PER_OVL;
    enLayerCnt = 0;

	for (i = 0; i < layerCnt; i++) {

		idx = i % OVL_LAYER_NUM_PER_OVL;

		fmtStr = get_fmt_name(DISP_MODULE_OVL, ovlInfo[i].fmt);

		if (ovlInfo[i].layer_en) {
            enLayerCnt++;
			if (sLayerBufAddr[index][idx] != ovlInfo[i].addr) {
				sLayerBufAddr[index][idx] = ovlInfo[i].addr;
				sprintf(tag_name, "OVL%dL%d_InFps", index, idx);
				met_tag_oneshot(DDP_IRQ_LAYER_FPS_ID, tag_name, i+1);
			}
			if (sLayerBufFmt[index][idx] != ovlInfo[i].fmt) {
				sLayerBufFmt[index][idx] = ovlInfo[i].fmt;
				sprintf(tag_name, "OVL%dL%d_Fmt_%s", index, idx, fmtStr);
				met_tag_oneshot(DDP_IRQ_LAYER_FORMAT_ID, tag_name, i+1);
			}
			if (sLayerBufWidth[index][idx] != ovlInfo[i].src_w) {
				sLayerBufWidth[index][idx] = ovlInfo[i].src_w;
				sprintf(tag_name, "OVL%dL%d_Width", index, idx);
				met_tag_oneshot(DDP_IRQ_LAYER_SIZE_ID, tag_name, ovlInfo[i].src_w);
			}
			if (sLayerBufHeight[index][idx] != ovlInfo[i].src_h) {
				sLayerBufHeight[index][idx] = ovlInfo[i].src_h;
				sprintf(tag_name, "OVL%dL%d_Height", index, idx);
				met_tag_oneshot(DDP_IRQ_LAYER_SIZE_ID, tag_name, ovlInfo[i].src_h);
			}

		} else {
			sLayerBufAddr[index][idx] = 0;
			sLayerBufFmt[index][idx] = 0;
			sLayerBufWidth[index][idx] = 0;
			sLayerBufHeight[index][idx] = 0;
		}
	}

    if (enLayerCnt) {
		enLayerCnt = 0;
		sprintf(tag_name, "OVL%d_OutFps", index);
		met_tag_oneshot(DDP_IRQ_LAYER_FPS_ID, tag_name, index);
	}
}

static void ddp_err_irq_met_tag(const char *name) {
	met_tag_oneshot(DDP_IRQ_EER_ID, name, 0);
	return;
}

static void met_irq_handler_rdma(unsigned int reg_val) {
	int index = 0;
	char tag_name[30] = { '\0' };
	if (reg_val & (1 << 1)) {
		ddp_disp_refresh_tag_start(index);
	}
	if (reg_val & (1 << 2)) {
		ddp_disp_refresh_tag_end(index);
	}
    if (reg_val & (1 << 3)) {
		sprintf(tag_name, "rdma%d_abnormal", index);
		ddp_err_irq_met_tag(tag_name);
	}
	if (reg_val & (1 << 4)) {
		sprintf(tag_name, "rdma%d_underflow", index);
		ddp_err_irq_met_tag(tag_name);
	}
	return;
}

static void met_irq_handler_mutex(unsigned int reg_val) {
	int index = 0;
	char tag_name[30] = { '\0' };
	if (reg_val & (1 << index)) {
		ddp_inout_info_tag(index);
	}
	return;
}


void ddp_init_met_tag(int state) {
	if ((!met_tag_on) && state) {
		met_tag_on = state;

        disp_register_irq(DISP_MODULE_RDMA,met_irq_handler_rdma);
        disp_register_irq(DISP_MODULE_MUTEX,met_irq_handler_mutex);
	}
	if (met_tag_on && (!state)) {
		met_tag_on = state;

        disp_unregister_irq(DISP_MODULE_RDMA,met_irq_handler_rdma);
        disp_unregister_irq(DISP_MODULE_MUTEX,met_irq_handler_mutex);   
	}
}
