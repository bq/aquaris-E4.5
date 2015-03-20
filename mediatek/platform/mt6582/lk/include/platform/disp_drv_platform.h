
#ifndef __DISP_DRV_PLATFORM_H__
#define __DISP_DRV_PLATFORM_H__

#include <platform/mt_gpio.h>
#include <platform/boot_mode.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_typedefs.h>
#include <platform/disp_drv.h>
#include <platform/lcd_drv.h>
#include <platform/dpi_drv.h>
#include <platform/dsi_drv.h>
#include <platform/lcd_reg.h>
#include <platform/dpi_reg.h>
#include <platform/dsi_reg.h>
#include <platform/disp_assert_layer.h>
#include <platform/disp_drv_log.h>
#include <platform/mt_disp_drv.h>
#include "lcm_drv.h"
#include <platform/sync_write.h>

#define BUILD_UBOOT


#define MTK_OVL_DECOUPLE_SUPPORT
#undef MTK_M4U_SUPPORT
#undef MTK_HDMI_SUPPORT
#define DEFINE_SEMAPHORE(x)  
#define down_interruptible(x) 0
#define up(x)                
#define DBG_OnTriggerLcd()

///LCD HW feature options for MT6575
#define MTK_LCD_HW_SIF_VERSION      2       ///for MT6575, we naming it is V2 because MT6516/73 is V1...
#define MTK_LCD_HW_3D_SUPPORT
#define MT65XX_NEW_DISP
#define MTK_FB_ALIGNMENT	16
#define ALIGN_TO(x, n)  	(((x) + ((n) - 1)) & ~((n) - 1))


/*****************************************************************************/
/* fix warning: dereferencing type-punned pointer will break strict-aliasing */
/*              rules [-Wstrict-aliasing]                                    */
/*****************************************************************************/
#define DISP_OUTREG32_R(type, addr2, addr1) 	                             \
		{                                                                    \
			union p_regs                                                     \
			{                                                                \
				type p_reg;                                         	     \
				unsigned int * p_uint;                              		 \
			}p_temp1,p_temp2;                                                \
			p_temp1.p_reg  = (type)(addr2);                                  \
			p_temp2.p_reg  = (type)(addr1);                                  \
			OUTREG32(p_temp1.p_uint,INREG32(p_temp2.p_uint));}

#define DISP_OUTREG32_V(type, addr2, var) 	                                 \
		{                                                                    \
			union p_regs                                                     \
			{                                                                \
				type p_reg;                                    			     \
				unsigned int * p_uint;                          		     \
			}p_temp1;                                                        \
			p_temp1.p_reg  = (type)(addr2);                                  \
			OUTREG32(p_temp1.p_uint,var);}

#define DISP_OUTREG16_R(type, addr2, addr1) 	                             \
		{                                                                    \
			union p_regs                                                     \
			{                                                                \
				type p_reg;                                       			 \
				unsigned short * p_uint;                         			 \
			}p_temp1,p_temp2;                                                \
			p_temp1.p_reg  = (type)(addr2);                                  \
			p_temp2.p_reg  = (type)(addr1);                                  \
			OUTREG32(p_temp1.p_uint,INREG32(p_temp2.p_uint));}

#define DISP_OUTREGBIT(TYPE,REG,bit,value)									 \
		do {    															 \
			union p_regs													 \
			{													 			 \
				TYPE* p_reg;									 			 \
				unsigned int * p_uint; 							 			 \
			}p_temp1,p_temp2;									 			 \
			p_temp1.p_reg  = (TYPE*)(&REG);						 			 \
			unsigned int reg_var = INREG32(p_temp1.p_uint);					 \
			p_temp2.p_uint  = &reg_var;							 			 \
			p_temp2.p_reg->bit = value;										 \
			OUTREG32(p_temp1.p_uint, INREG32(p_temp2.p_uint));	 			 \
		} while (0)

#define DISP_INREG32(type,addr)		  										 \
		({	    															 \
		    unsigned int var = 0;                                            \
			union p_regs													 \
			{													 			 \
				type p_reg;										 			 \
				unsigned int * p_uint; 							 			 \
			}p_temp1;									 					 \
			p_temp1.p_reg  = (type)(addr);						 			 \
			var = INREG32(p_temp1.p_uint);	 								 \
			var;															 \
		})

#define DISP_MASKREG32_T(type, addr, mask, data)	                         \
		{                                                                    \
			union p_regs                                                     \
			{                                                                \
				type p_reg;                                    			     \
				unsigned int * p_uint;                                       \
			}p_temp1;                                                        \
			p_temp1.p_reg  = (type)(addr);                                   \
			MASKREG32(p_temp1.p_uint, mask, data);}

#endif //__DISP_DRV_PLATFORM_H__


