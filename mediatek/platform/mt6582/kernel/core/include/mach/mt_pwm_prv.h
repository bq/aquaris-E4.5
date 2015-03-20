/*******************************************************************************
* mt6575_pwm.h PWM Drvier                                                     
*                                                                              
* Copyright (c) 2010, Media Teck.inc                                           
*                                                                                                                                
* This program is free software; you can redistribute it and/or modify it     
* under the terms and conditions of the GNU General Public Licence,            
* version 2, as publish by the Free Software Foundation.                       
*                                                                                                                                
* This program is distributed and in hope it will be useful, but WITHOUT       
* ANY WARRNTY; without even the implied warranty of MERCHANTABITLITY or        
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for    
* more details.                                                                
*                                                                              
*                                                                           
********************************************************************************
* Author : Chagnlei Gao (changlei.gao@mediatek.com)                            
********************************************************************************
*/

#ifndef __MT_PWM_PRV_H__
#define __MT_PWM_PRV_H__

#ifdef OUTREG32
  #undef OUTREG32
  #define OUTREG32(x, y) mt65xx_reg_sync_writel(y, x)
#endif

/***********************************
* PWM register address             *
************************************/
#define PWM_ENABLE (PWM_BASE+0x0000)
                                                  
#define PWM4_DELAY (PWM_BASE+0x00004)                                                 
#define PWM5_DELAY (PWM_BASE+0x00008) 

#define PWM_3DLCM	(PWM_BASE+0x1D0)
#define PWM_3DLCM_ENABLE_OFFSET 0  
#define	PWM_3DLCM_2_INV	4
#define	PWM_3DLCM_1_INV	3                     
#define	PWM_3DLCM_0_INV	2  
                                                  
#define PWM_INT_ENABLE (PWM_BASE+0x0200)                                            
#define PWM_INT_STATUS (PWM_BASE+0x0204)                                      
#define PWM_INT_ACK (PWM_BASE+0x0208) 
#define PWM_EN_STATUS (PWM_BASE+0x020c)

/*PWM3,PWM4,PWM5_DELAY registers*/
#define PWM_DELAY_DURATION_MASK 0x0000FFFF
#define PWM_DELAY_CLK_MASK 0x00010000

#define PWM_ENABLE_SEQ_OFFSET 16
#define PWM_ENABLE_TEST_SEL_OFFSET 17

/*PWM1~PWM7 control registers*/
#define PWM_CON_CLKDIV_MASK 0x00000007
#define PWM_CON_CLKDIV_OFFSET 0
#define PWM_CON_CLKSEL_MASK 0x00000008
#define PWM_CON_CLKSEL_OFFSET 3

#define PWM_CON_SRCSEL_MASK 0x00000020
#define PWM_CON_SRCSEL_OFFSET 5

#define PWM_CON_MODE_MASK 0x00000040
#define PWM_CON_MODE_OFFSET 6

#define PWM_CON_IDLE_VALUE_MASK 0x00000080
#define PWM_CON_IDLE_VALUE_OFFSET 7

#define PWM_CON_GUARD_VALUE_MASK 0x00000100
#define PWM_CON_GUARD_VALUE_OFFSET 8

#define PWM_CON_STOP_BITS_MASK 0x00007E00
#define PWM_CON_STOP_BITS_OFFSET 9
#define PWM_CON_OLD_MODE_MASK 0x00008000
#define PWM_CON_OLD_MODE_OFFSET 15

#define BLOCK_CLK 65*1024*1024

#endif