/*
* Copyright (C) 2011-2014 MediaTek Inc.
* 
* This program is free software: you can redistribute it and/or modify it under the terms of the 
* GNU General Public License version 2 as published by the Free Software Foundation.
* 
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * MD218A voice coil motor driver
 *
 *
 */

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include "GAF001AF.h"
#include "../camera/kd_camera_hw.h"

#define LENS_I2C_BUSNUM 1
static struct i2c_board_info __initdata kd_lens_dev={ I2C_BOARD_INFO("GAF001AF", 0x18)};

#define GAF001AF_DRVNAME "GAF001AF"

#define GAF001AF_DEBUG
#ifdef GAF001AF_DEBUG
#define GAF001AFDB printk
#else
#define GAF001AFDB(x,...)
#endif

static spinlock_t g_GAF001AF_SpinLock;
static struct i2c_client * g_pstGAF001AF_I2Cclient = NULL;
static dev_t g_GAF001AF_devno;
static struct cdev * g_pGAF001AF_CharDrv = NULL;
static struct class *actuator_class = NULL;
static s16 g_s4GAF001AF_Opened = 0;
static s16 g_i4MotorStatus = 0;
static s16 g_i4Dir = 0;
static u16 gAF_drvID = 0x9714; //default
static u16 g_u4GAF001AF_INF = 0;
static u16 g_u4GAF001AF_MACRO = 1023;
static u16 g_u4TargetPosition = 0;
static u16 g_u4CurrPosition   = 0;

/*extern int mt_set_gpio_mode(unsigned long pin, unsigned long mode);
extern int mt_set_gpio_out(unsigned long pin, unsigned long dir);
extern int mt_set_gpio_dir(unsigned long pin, unsigned long dir);*/


/***********************************************************
  Reg r/w 
  ************************************************************/
static s16 s4RUMBAAF_ReadReg( u16 RamAddr, u8 *RegData )
{
    u8 pBuff[2] = {(u8)(RamAddr >> 8) , (u8)(RamAddr & 0xFF)};
    g_pstGAF001AF_I2Cclient->addr = (0x48 >> 1);
    if (i2c_master_send(g_pstGAF001AF_I2Cclient, pBuff, 2) < 0 ) 
    {
        GAF001AFDB("[RUMBAAF] read I2C send failed!!\n");
        return -1;
    }

    GAF001AFDB("[RUMBAAFDB]I2C r (%x %x) \n",RamAddr,*RegData);
    if (i2c_master_recv(g_pstGAF001AF_I2Cclient, (u8*)RegData, 1) != 1) 
    {
        GAF001AFDB("[RUMBAAF] I2C read failed!! \n");
        return  -1;
    }
    return 0;
}

static s16 s4RUMBAAF_WriteReg(u16 a_u2Add, u16 a_u2Data)
{
    u8 puSendCmd[3] = {(u8)(a_u2Add>>8), (u8)(a_u2Add&0xFF), (u8)(a_u2Data&0xFF)};
    g_pstGAF001AF_I2Cclient->addr = (0x48 >> 1);
    if (i2c_master_send(g_pstGAF001AF_I2Cclient, puSendCmd, 3) < 0) 
    {
        GAF001AFDB("[RUMBAAF] I2C send failed!! %d\n",a_u2Add);
        return -1;
    }
    return 0;
}

static s16 s4RUMBAAF_WriteReg2(u16 a_u2Add, u16 a_u2Data)
{
    u8 puSendCmd[4] = {(u8)(a_u2Add>>8), (u8)(a_u2Add&0xFF), (u8)(a_u2Data>>8), (u8)(a_u2Data&0xFF)};
    g_pstGAF001AF_I2Cclient->addr = (0x48 >> 1);
    if (i2c_master_send(g_pstGAF001AF_I2Cclient, puSendCmd, 4) < 0) 
    {
        GAF001AFDB("[RUMBAAF] I2C send failed!! %d\n",a_u2Add);
        return -1;
    }
    return 0;
}
static s16 s4RUMBAAF_SrWriteReg(u16 a_u2Add, u16 a_u2Data)
{
    u8 puSendCmd[3] = { (u8)(a_u2Add>>8),  (u8)(a_u2Add&0xFF),(u8)(a_u2Data&0xFF) };
    g_pstGAF001AF_I2Cclient->addr = (0x20 >> 1);
    if (i2c_master_send(g_pstGAF001AF_I2Cclient, puSendCmd, 3) < 0) 
    {
        GAF001AFDB("[RUMBAAF] I2C sr send failed!! 1\n");
        return -1;
    }
    return 0;
}

static s16 s4BU6474AF_SrWriteReg( u8 u08_adr, u16 u16_dat) //I2C_write_FBAF
{
    u8   puSendCmd[3] = {(u8)(u08_adr&0xFF),(u8)((u16_dat>>8)&0xFF),(u8)(u16_dat&0xFF)};
    GAF001AFDB("[BU6474AF]I2C w4 (%x %x) \n",u08_adr,u16_dat);
    spin_lock(&g_GAF001AF_SpinLock);
    g_pstGAF001AF_I2Cclient->ext_flag = (g_pstGAF001AF_I2Cclient->ext_flag)&(~I2C_DMA_FLAG);
    g_pstGAF001AF_I2Cclient->addr = (0xE8 >> 1);
    spin_unlock(&g_GAF001AF_SpinLock);
    if (i2c_master_send(g_pstGAF001AF_I2Cclient, puSendCmd,3) < 0 ) 
    {
        GAF001AFDB("[BU6474AF]I2C send failed!! \n");
        return -1;
    }
    return 0;
}
static s16 s4BU6474AF_CEFWriteReg( u8 u08_adr, u16 u16_dat)
{
    return s4BU6474AF_SrWriteReg( u08_adr | 0x80, u16_dat );
}

/***********************************************************
  Reg position 
  ************************************************************/
static s16 s4DW9714AF_ReadReg(u16 * a_pu2Result)
{
    u8 pBuff[2];
    g_pstGAF001AF_I2Cclient->addr = (0x18 >> 1);
    if (i2c_master_recv(g_pstGAF001AF_I2Cclient, pBuff , 2) < 0) 
    {
        GAF001AFDB("[DW9714AF] I2C read failed!! \n");
        return -1;
    }
    *a_pu2Result = (((u16)pBuff[0]) << 4) + (u16)(pBuff[1] >> 4);
    return 0;
}

static s16 s4FM50AF_ReadReg(u16 * a_pu2Result)
{
    u8 pBuff[2];
    g_pstGAF001AF_I2Cclient->addr = (0x18 >> 1);
    if (i2c_master_recv(g_pstGAF001AF_I2Cclient, pBuff , 2) < 0) 
    {
        GAF001AFDB("[FM50AF] I2C read failed!! \n");
        return -1;
    }
    *a_pu2Result = (((u16)pBuff[0]) << 4) + (u16)(pBuff[1] >> 4);
    return 0;
}
static s16 s4BU6424AF_ReadReg(u16 * a_pu2Result)
{
    u8 pBuff[2];
   /* mt_set_gpio_mode(GPIO_CAMERA_AF_EN_PIN,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_CAMERA_AF_EN_PIN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_CAMERA_AF_EN_PIN,1);*/
    g_pstGAF001AF_I2Cclient->addr = (0x18 >> 1);
    if (i2c_master_recv(g_pstGAF001AF_I2Cclient, pBuff , 2) < 0) 
    {
        GAF001AFDB("[BU6424AF]I2C read failed!! \n");
        return -1;
    }
    *a_pu2Result = (((u16)(pBuff[0] & 0x03)) << 8) + (u16)pBuff[1];
    return 0;
}
static s16 s4BU6429AF_ReadReg(u16 * a_pu2Result)
{
    u8 pBuff[2];    
    g_pstGAF001AF_I2Cclient->addr = (0x18 >> 1);
    if (i2c_master_recv(g_pstGAF001AF_I2Cclient, pBuff , 2) < 0) 
    {
        GAF001AFDB("[BU6429AF] I2C read failed!! \n");
        return -1;
    }
    *a_pu2Result = (((u16)pBuff[0]) << 4) + (u16)(pBuff[1] >> 4);
    return 0;
}
static s16 s4AD5823AF_ReadReg(u16 * a_pu2Result)
{
    return 0;
}
static s16 s4DW9718AF_ReadPosition(u16 * a_pu2Result)
{
    u8 pBuff1 = 0x02;
    u8 pBuff2 = 0x03;
    u8 RegData1=0, RegData2=0;
    g_pstGAF001AF_I2Cclient->addr = (0x18 >> 1);
    if (i2c_master_send(g_pstGAF001AF_I2Cclient, &pBuff1, 1) < 0 ) 
    {
        GAF001AFDB("[DW9718AF] read I2C send failed!!\n");
        return -1;
    }
    if (i2c_master_recv(g_pstGAF001AF_I2Cclient, &RegData1, 1) != 1) 
    {
        GAF001AFDB("[DW9718AF] I2C read failed!! \n");
        return  -1;
    }

    if (i2c_master_send(g_pstGAF001AF_I2Cclient, &pBuff2, 1) < 0 ) 
    {
        GAF001AFDB("[DW9718AF] read I2C send failed!!\n");
        return -1;
    }

    if (i2c_master_recv(g_pstGAF001AF_I2Cclient, &RegData2, 1) != 1) 
    {
        GAF001AFDB("[DW9718AF] I2C read failed!! \n");
        return  -1;
    }
    *a_pu2Result= (((u16)RegData1)<<8) +  (u16)(RegData2);
    return 0;
}

static s16 s4RUMBAAF_ReadPosition(u16 * a_pu2Result)
{
    u8 posL, posH;
    if(s4RUMBAAF_ReadReg(0x0062, &posL)==0) 
    {
        if(s4RUMBAAF_ReadReg(0x0063, &posH)==0) 
        {    
            *a_pu2Result = (((u16)posH)<<8) + (u16)posL;
        }
        else
        {    
            *a_pu2Result=0;
            return -1;
        }
    }
    else
    {    
        *a_pu2Result=0;
        return -1;
    }
    return 0;
}


static s16 s4OV8825AF_ReadPosition(u16 *a_pu2Result)
{
    u8 pBuff1[2] = {0x36, 0x18};
    u8 pBuff2[2] = {0x36, 0x19};
    u8 RegData1=0, RegData2=0;
    g_pstGAF001AF_I2Cclient->addr = (0x6c >> 1);
    if (i2c_master_send(g_pstGAF001AF_I2Cclient, pBuff1, 2) < 0 ) 
    {
        GAF001AFDB("[OV8825AF] read I2C send failed!!\n");
        return -1;
    }
    if (i2c_master_recv(g_pstGAF001AF_I2Cclient, &RegData1, 1) != 1) 
    {
        GAF001AFDB("[OV8825AF] I2C read failed!! \n");
        return  -1;
    }

    if (i2c_master_send(g_pstGAF001AF_I2Cclient, pBuff2, 2) < 0 ) 
    {
        GAF001AFDB("[OV8825AF] read I2C send failed!!\n");
        return -1;
    }

    if (i2c_master_recv(g_pstGAF001AF_I2Cclient, &RegData2, 1) != 1) 
    {
        GAF001AFDB("[OV8825AF] I2C read failed!! \n");
        return  -1;
    }

    *a_pu2Result= ((u16)RegData1 +  (((u16)RegData2)<<8))>>4;

    return 0;
}

static s16 s4BU6474AF_ReadReg(u8 u08_adr, u16 *a_pu2Result)
{
    u8 pBuff[1] = {(u8)(u08_adr & 0xFF)};
    s16 vRcvBuff=0;
    spin_lock(&g_GAF001AF_SpinLock);
    g_pstGAF001AF_I2Cclient->addr = (0xE8 >> 1);
    g_pstGAF001AF_I2Cclient->ext_flag = (g_pstGAF001AF_I2Cclient->ext_flag)&(~I2C_DMA_FLAG);
    spin_unlock(&g_GAF001AF_SpinLock);

    if (i2c_master_send(g_pstGAF001AF_I2Cclient, pBuff, 1) < 0 ) 
    {
        GAF001AFDB("[BU6474AF] read I2C send failed!!\n");
        return -1;
    }

    if (i2c_master_recv(g_pstGAF001AF_I2Cclient, (u8*)&vRcvBuff, 2) != 2) 
    {
        GAF001AFDB("[BU6474AF] I2C read failed!! \n");
        return -1;
    }
    *a_pu2Result= ((vRcvBuff<<8)&0xff00)|((vRcvBuff>>8)&0x00ff);
    
    GAF001AFDB("[BU6474AF]I2C r2 (%x %x) \n",u08_adr,vRcvBuff);
    return 0;
}
static s16 s4BU6474AF_NVLREAD(u8 u08_adr, u16 *u16_dat)
{
    s4BU6474AF_SrWriteReg(u08_adr,0x4000);
    return s4BU6474AF_ReadReg(u08_adr,u16_dat);
}
/***********************************************************
  Write position 
  ************************************************************/
static s16 s4DW9718AF_WriteReg(u16 a_u2Data)
{
    u8 puSendCmd[3] = {0x02,(u8)(a_u2Data >> 8) , (u8)(a_u2Data & 0xFF)};
    g_pstGAF001AF_I2Cclient->addr = (0x18 >> 1);
    if (i2c_master_send(g_pstGAF001AF_I2Cclient, puSendCmd, 3) < 0) 
    {
        GAF001AFDB("[DW9718AF] I2C send failed!! \n");
        return -1;
    }
    return 0;
}

static s16 s4DW9714AF_WriteReg(u16 a_u2Data)
{
    u8 puSendCmd[2] = {(u8)(a_u2Data >> 4) , (u8)((a_u2Data & 0xF) << 4)};
    g_pstGAF001AF_I2Cclient->addr = (0x18 >> 1);
    if (i2c_master_send(g_pstGAF001AF_I2Cclient, puSendCmd, 2) < 0) 
    {
        GAF001AFDB("[DW9714AF] I2C send failed!!\n");
        return -1;
    }
    return 0;
}
static s16 s4FM50AF_WriteReg(u16 a_u2Data)
{
    u8 puSendCmd[2] = {(u8)(a_u2Data >> 4) , (u8)(((a_u2Data & 0xF) << 4)+3)};
    g_pstGAF001AF_I2Cclient->addr = (0x18 >> 1);
    if (i2c_master_send(g_pstGAF001AF_I2Cclient, puSendCmd, 2) < 0) 
    {
        GAF001AFDB("[FM50AF] I2C send failed!! \n");
        return -1;
    }
    return 0;
}

static s16 s4BU6424AF_WriteReg(u16 a_u2Data)
{
    u8 puSendCmd[2] = {(u8)(((a_u2Data >> 8) & 0x03) | 0xc0), (u8)(a_u2Data & 0xff)};
    /*mt_set_gpio_mode(GPIO_CAMERA_AF_EN_PIN,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_CAMERA_AF_EN_PIN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_CAMERA_AF_EN_PIN,1);*/
    g_pstGAF001AF_I2Cclient->addr = (0x18 >> 1);
    if (i2c_master_send(g_pstGAF001AF_I2Cclient, puSendCmd, 2) < 0) 
    {
        GAF001AFDB("[BU6424AF]I2C send failed!! \n");
        return -1;
    }
    return 0;
}
static s16 s4BU6429AF_WriteReg(u16 a_u2Data)
{
    u8 puSendCmd[2] = {(u8)(((a_u2Data >> 8) & 0x03) | 0xF4), (u8)(a_u2Data & 0xFF)};
    g_pstGAF001AF_I2Cclient->addr = (0x18 >> 1);
    if (i2c_master_send(g_pstGAF001AF_I2Cclient, puSendCmd, 2) < 0) 
    {
        GAF001AFDB("[BU6429AF] I2C send failed!! \n");
        return -1;
    }
    return 0;
}

static s16 s4AD5823AF_WriteReg(u16 a_u2Data)
{
    u8 VCMMSB[2] = {(u8)(0x04) , (u8)((a_u2Data >>8)&0x03)};
    u8 VCMLSB[2] = {(u8)(0x05) , (u8)(a_u2Data & 0xFF)};
    g_pstGAF001AF_I2Cclient->addr = (0x18 >> 1);    
    if (i2c_master_send(g_pstGAF001AF_I2Cclient, VCMMSB, 2) < 0) 
    {
        GAF001AFDB("[AD5823] I2C send failed!! \n");
        return -1;
    }
    if (i2c_master_send(g_pstGAF001AF_I2Cclient, VCMLSB, 2) < 0) 
    {
        GAF001AFDB("[AD5823] I2C send failed!! \n");
        return -1;
    }
    return 0;
}

static s16 s4RUMBAAF_WritePosition(u16 a_u2Data)
{
    u16 pos =  ((a_u2Data&0xFF)<<8) + ((a_u2Data>>8)&0xFF);
    return s4RUMBAAF_WriteReg2(0x0062, pos);
}

static s16 s4OV8825AF_WritePosition(u16 a_u2Data)
{
    u16 temp=(a_u2Data<<4);
    u8 puSendCmd1[3] = {0x36, 0x19, (u8)((temp>>8)&0xFF) }; //(0x3619,(temp>>8)&0xff)
    u8 puSendCmd2[3] = {0x36, 0x18, (u8)(temp&0xFF)};   //(0x3618,temp&0xff);

    g_pstGAF001AF_I2Cclient->addr = (0x6c >> 1);
    if (i2c_master_send(g_pstGAF001AF_I2Cclient, puSendCmd1, 3) < 0) 
    {
        GAF001AFDB("[OV8825AF] I2C sr send failed!! 1\n");
        return -1;
    }
    if (i2c_master_send(g_pstGAF001AF_I2Cclient, puSendCmd2, 3) < 0) 
    {
        GAF001AFDB("[OV8825AF] I2C sr send failed!! 1\n");
        return -1;
    }
    return 0;
}

/***********************************************************
  Write position 
  ************************************************************/

static s16 s4AF_ReadPosition(u16 *a_u2Data)
{
    s16 ret=0;
    switch(gAF_drvID)
    {
        case 0x5820 :  /* AD5820 */
        case 0x9714 :  /* DW9714 */
            ret = s4DW9714AF_ReadReg(a_u2Data);  
            break;
        case 0xF050 : /* FM50AF */
            ret = s4FM50AF_ReadReg(a_u2Data);    
            break;
        break;
        case 0x6424 : /* BU6424 */
            ret = s4BU6424AF_ReadReg(a_u2Data);
            break;
        case 0x6429 : /* BU6429 */
            ret = s4BU6429AF_ReadReg(a_u2Data);
            break;
        case 0x9718 : /* DW9718 */
            ret = s4DW9718AF_ReadPosition(a_u2Data);
            break;
        case 0x5823 : /* AD5823 */
            ret = s4AD5823AF_ReadReg(a_u2Data);
            break;
        case 0x8825 : /* OV8825 */
            ret = s4OV8825AF_ReadPosition(a_u2Data);
            break;
        case 0x6334 : /* Rumba */
            ret = s4RUMBAAF_ReadPosition(a_u2Data);
            break;
        case 0x6474 : /* BU6474 */
            *a_u2Data =100;
            break;
        default :
            ret = s4DW9714AF_ReadReg(a_u2Data);  
        break;
    }

    return ret;
}

static s16 s4AF_WritePosition(u16 a_u2Data)
{
    s16 ret=0;
    switch(gAF_drvID)
    {
        case 0x5820 :  /* AD5820 */
        case 0x9714 :  /* DW9714 */
            ret = s4DW9714AF_WriteReg(a_u2Data);  
            break;
        case 0xF050 : /* FM50AF */
            ret = s4FM50AF_WriteReg(a_u2Data);  
            break;
        break;
        case 0x6424 : /* BU6424 */
            ret = s4BU6424AF_WriteReg(a_u2Data);
            break;
        case 0x6429 : /* BU6429 */
            ret = s4BU6429AF_WriteReg(a_u2Data);
            break;
        case 0x9718 : /* DW9718 */
            ret = s4DW9718AF_WriteReg(a_u2Data);
            break;
        case 0x5823 : /* AD5823 */
            ret = s4AD5823AF_WriteReg(a_u2Data);
            break;
        case 0x8825 : /* OV8825 */
            ret = s4OV8825AF_WritePosition(a_u2Data);
            break;            
        case 0x6334 : /* Rumba */
            ret = s4RUMBAAF_WritePosition(a_u2Data);
            break;
        case 0x6474 : /* BU6474 */
            ret = s4BU6474AF_SrWriteReg(0x11,a_u2Data);
            break;
        default :
            ret = s4DW9714AF_WriteReg(a_u2Data);  
        break;
    }
    return ret;
}



/***********************************************************
  init action
  ************************************************************/
static s16 s4RUMBAAF_init(void)
{
    //power on OIS
    GAF001AFDB("[RUMBAAF] init \n");
    s4RUMBAAF_SrWriteReg(0x3104, 0x00);//0x3104          0x00       ( Flash Pin enable )
    s4RUMBAAF_SrWriteReg(0x304F, 0x0D);//0x304F          0x0D       (Set Reset Low / Disable OIS)
    msleep(10);
    s4RUMBAAF_SrWriteReg(0x304F, 0x0E);//0x304F          0x0E       (Set Reset High /  Enable OIS ) 
    msleep(10);

    s4RUMBAAF_WriteReg(0x0061,0x02);//0x48        0x0061       0x02 ring control enable/ linear current mode
    s4RUMBAAF_WriteReg(0x0060,0x01);//0x48      0x0060     0x01 enable
    return 0;
}
/***********************************************************
  uninit action
  ************************************************************/

static s16 s4RUMBAAF_uninit(void)
{
    GAF001AFDB("[RUMBAAF] feee \n");
    s4RUMBAAF_WriteReg2(0x0062, 200 );
    msleep(10);
    s4RUMBAAF_WriteReg2(0x0062, 100 );
    msleep(10);
    
    //power off AF
    s4RUMBAAF_WriteReg(0x0060,0x00);//0x48      0x0060     0x01 enable
    s4RUMBAAF_WriteReg(0x0061,0x00);//0x48      0x0061     0x02 ring control enable/ linear current mode
    //power off OIS
    s4RUMBAAF_SrWriteReg(0x3104, 0x00);//0x3104          0x00       ( Flash Pin enable )
    s4RUMBAAF_SrWriteReg(0x304F, 0x0D);//0x304F          0x0D       (Set Reset Low / Disable OIS)    
    return 0;   
}
static s16 s4BU6474AF_init(void)
{    
    u16 dat_H,dat_L;
    GAF001AFDB("[BU6474AF] BU6474AF_Open - Start\n");
    s4BU6474AF_SrWriteReg(0xFF,0x0000);     //I2C_func_POFF_____( );     // reset the IC
    s4BU6474AF_SrWriteReg(0xFF,0x8000);     //I2C_func_PON______( );     // Power ON
    
    //  **** Initialize Peripheral
    s4BU6474AF_SrWriteReg(0x37,0x0008);     // Analog Block Power ON
    s4BU6474AF_SrWriteReg(0x25,0x0003);     // ADC Averaging ON
    s4BU6474AF_SrWriteReg(0x26,0x0100);     // Ch0 Hall Current DAC 1.6mA
    s4BU6474AF_SrWriteReg(0x27,0x0200);     // Ch1 Hall PreAmp  OFFSET DAC
    s4BU6474AF_SrWriteReg(0x28,0x0200);     // Ch2 Hall PostAmp OFFSET DAC
    s4BU6474AF_SrWriteReg(0x29,0x0200);     // Ch3 Current Amp  OFFSET DAC
    s4BU6474AF_SrWriteReg(0x24,0x40F0);     // ADC Enable & Hall Gain
    s4BU6474AF_SrWriteReg(0x22,0xC2F8);     // BTL Modulation Setting
    s4BU6474AF_SrWriteReg(0x23,0x0311);     // BTL Driver Control
    s4BU6474AF_CEFWriteReg(0x27,0x0000);   // M_wGAS
    s4BU6474AF_CEFWriteReg(0x20,0x0000);   // M_HOFS
    s4BU6474AF_CEFWriteReg(0x25,0xE000);    // M_KgLPG
    s4BU6474AF_CEFWriteReg(0x21,0xC000);    // M_KgHG
    s4BU6474AF_CEFWriteReg(0x2D,0x8800);    // Kg2D
    s4BU6474AF_CEFWriteReg(0x00,0x7FFF);    // Kg00    // HIGH 2nd
    s4BU6474AF_CEFWriteReg(0x01,0x5000);    // Kg01
    s4BU6474AF_CEFWriteReg(0x02,0x4000);    // Kg02
    s4BU6474AF_CEFWriteReg(0x03,0x4000);    // Kg03
    s4BU6474AF_CEFWriteReg(0x08,0x0000);    // Kg08    // LOW 1st
    s4BU6474AF_CEFWriteReg(0x09,0x6000);    // Kg09
    s4BU6474AF_CEFWriteReg(0x0A,0x4000);    // Kg0A
    s4BU6474AF_CEFWriteReg(0x0B,0x4000);    // Kg0B
    s4BU6474AF_CEFWriteReg(0x10,0x0000);    // Kg10    // LOW 2nd
    s4BU6474AF_CEFWriteReg(0x11,0x7FFE);    // Kg11
    s4BU6474AF_CEFWriteReg(0x12,0x4000);    // Kg12
    s4BU6474AF_CEFWriteReg(0x13,0xD000);    // Kg13
    s4BU6474AF_CEFWriteReg(0x38,0x1000);    // M_PRFCEF
    s4BU6474AF_CEFWriteReg(0x27,0x0000);   // Delay FF
    s4BU6474AF_CEFWriteReg(0x2B,0x0000);   // Delay FF
    s4BU6474AF_CEFWriteReg(0x04,0x0000);   // Delay FF
    s4BU6474AF_CEFWriteReg(0x0C,0x0000);   // Delay FF
    s4BU6474AF_CEFWriteReg(0x14,0x0000);   // Delay FF
    s4BU6474AF_CEFWriteReg(0x1D,0x0000);   // Delay FF
    s4BU6474AF_CEFWriteReg(0x1E,0x0000);   // Delay FF
    s4BU6474AF_CEFWriteReg(0x2A,0x0000);    // Delay FF
    s4BU6474AF_CEFWriteReg(0x18,0x7FFF);    // Kg18
    s4BU6474AF_CEFWriteReg(0x19,0x0000);   // Kg19
    s4BU6474AF_CEFWriteReg(0x1A,0x0000);   // Kg1A
    s4BU6474AF_CEFWriteReg(0x1B,0x0000);   // Kg1B
    s4BU6474AF_CEFWriteReg(0x1C,0x0000);   // Kg1C
    s4BU6474AF_CEFWriteReg(0x0E,0x1000);    // EXCEL); C_LMT 90mA
    s4BU6474AF_CEFWriteReg(0x16,0x0000);   //        CUROFS
    s4BU6474AF_CEFWriteReg(0x17,0x36E2);    // EXCEL); Kf0A    Current Compensation 100mA Full Scale
    s4BU6474AF_CEFWriteReg(0x29,0x4000);    //        Kf0C    Integral Gain
    s4BU6474AF_CEFWriteReg(0x31,0x0000);   //        M_PRFCNT     8bit  b15:8
    s4BU6474AF_CEFWriteReg(0x32,0x0800);    //        M_PRFNUM     8bit  b15:8 KMT
    s4BU6474AF_CEFWriteReg(0x06,0x0000);   // TRILMT
    s4BU6474AF_CEFWriteReg(0x07,0x0000);   // KgWv
    s4BU6474AF_CEFWriteReg(0x34,0x0002);    // PRE    SHIFT 4bit b3:0
    s4BU6474AF_CEFWriteReg(0x35,0x0002);    // Post   SHIFT 4bit b3:0
    s4BU6474AF_CEFWriteReg(0x37,0x0008);    // OUTPUT SHIFT 4bit b3:0
    s4BU6474AF_CEFWriteReg(0x36,0x0002);    // fs control   4bit b3:0
    s4BU6474AF_SrWriteReg(0x60,0x0100);     //I2C_func_NVL_ENABLE( );
    s4BU6474AF_NVLREAD(0x40,&dat_L);        // TABLE_FORMAT
    if((dat_L==0xFF)||(dat_L==0)) return -1;

    s4BU6474AF_ReadReg(0x24,&dat_H);    dat_H &= 0x0FFF;
    s4BU6474AF_NVLREAD(0x41,&dat_L);    dat_H |= (dat_L<<8);
    s4BU6474AF_SrWriteReg(0x24,dat_H);  // Analog Amp Gain

    s4BU6474AF_NVLREAD(0x42,&dat_L);
    s4BU6474AF_SrWriteReg(0x27,dat_L<<2);  // Pre Amp offset

    s4BU6474AF_NVLREAD(0x43,&dat_L);
    s4BU6474AF_SrWriteReg(0x28,dat_L<<2);  // Pre Amp offset
    
    s4BU6474AF_NVLREAD(0x44,&dat_H);    s4BU6474AF_NVLREAD(0x45,&dat_L);
    s4BU6474AF_CEFWriteReg(0x20,(dat_H<<8)|dat_L);    // Digital Offset
                                                
    s4BU6474AF_NVLREAD(0x46,&dat_H);    s4BU6474AF_NVLREAD(0x47,&dat_L);
    s4BU6474AF_CEFWriteReg(0x21,(dat_H<<8)|dat_L);  // Digital Amplifier
                                            
    s4BU6474AF_NVLREAD(0x48,&dat_H);    s4BU6474AF_NVLREAD(0x49,&dat_L);
    s4BU6474AF_CEFWriteReg(0x25,(dat_H<<8)|dat_L );    // Digital Amplifier
                                                
    s4BU6474AF_NVLREAD(0x4A,&dat_H);    s4BU6474AF_NVLREAD(0x4B,&dat_L);
    s4BU6474AF_CEFWriteReg(0x17,(dat_H<<8)|dat_L);    // Digital Amplifier
    
    s4BU6474AF_NVLREAD(0x4C,&dat_H);    s4BU6474AF_NVLREAD(0x4D,&dat_L);        // CUROFS__L
    s4BU6474AF_CEFWriteReg(0x16,(dat_H<<8)|dat_L);
    
    s4BU6474AF_NVLREAD(0x4E,&dat_H);    s4BU6474AF_NVLREAD(0x4F,&dat_L);            // PARITY_2

    s4BU6474AF_SrWriteReg(0x7E,0);          //I2C_func_DSP_START( ); // CLAF Function Enable
    s4BU6474AF_CEFWriteReg(0x30,0x0004);    //EQ_CONTROL(  );
    GAF001AFDB("[BU6474AF] BU6474AF_Open - End\n");
    return 0;
}

/***********************************************************
  do init /uninit driver
  ************************************************************/
static s16 s4AF_init(void)
{
    switch(gAF_drvID)
    {
        case 0x5820 :  /* AD5820 */
        case 0x9714 :  /* DW9714 */
        case 0xF050 : /* FM50AF */
        case 0x6424 : /* BU6424 */
        case 0x6429 : /* BU6429 */
        case 0x9718 : /* DW9718 */
        case 0x5823 : /* AD5823 */
        case 0x8825 : /* OV8825 */
            break;
        case 0x6334 : /* Rumba */
            s4RUMBAAF_init();
            break;
        case 0x6474 : /* BU6474 */
            s4BU6474AF_init();
            break;

        default :
            break;
    }
    return 0;
}
static s16 s4AF_uninit(void)
{
    switch(gAF_drvID)
    {
        case 0x5820 : /* AD5820 */
        case 0x9714 : /* DW9714 */
        case 0xF050 : /* FM50AF */
        case 0x6424 : /* BU6424 */
        case 0x6429 : /* BU6429 */
        case 0x9718 : /* DW9718 */
        case 0x5823 : /* AD5823 */
        case 0x8825 : /* OV8825 */
            s4AF_WritePosition(200);
            msleep(10);
            s4AF_WritePosition(100);
            msleep(10);
            break;
        case 0x6334 : /* Rumba */
            s4RUMBAAF_uninit();
            break;
        case 0x6474 : /* BU6474 */
            break;

        default :
            break;
    }
    return 0;
}


inline static void getGAF001AFInfo(__user stGAF001AF_MotorInfo * pstMotorInfo)
{
    stGAF001AF_MotorInfo stMotorInfo;
    stMotorInfo.u4MacroPosition   = g_u4GAF001AF_MACRO;
    stMotorInfo.u4InfPosition     = g_u4GAF001AF_INF;
    stMotorInfo.u4CurrentPosition = g_u4CurrPosition;
    stMotorInfo.bIsSupportSR      = TRUE;

    if (g_i4MotorStatus == 1)    {stMotorInfo.bIsMotorMoving = 1;}
    else                        {stMotorInfo.bIsMotorMoving = 0;}

    if (g_s4GAF001AF_Opened >= 1)    {stMotorInfo.bIsMotorOpen = 1;}
    else                        {stMotorInfo.bIsMotorOpen = 0;}

    if(copy_to_user(pstMotorInfo , &stMotorInfo , sizeof(stGAF001AF_MotorInfo)))
    {
        GAF001AFDB("[GAF001AF] copy to user failed when getting motor information \n");
    }    
}

inline static s16 moveGAF001AF(u16 a_u4Position)
{
    s16 ret = 0;
    u16 InitPos = 0;
    if((a_u4Position>0x1000))
    {    
        gAF_drvID=a_u4Position;
        s4AF_init();
        return 0;
    }

    if((a_u4Position > g_u4GAF001AF_MACRO) || (a_u4Position < g_u4GAF001AF_INF))
    {
        GAF001AFDB("[GAF001AF] out of range \n");
        return -EINVAL;
    }

    if (g_s4GAF001AF_Opened == 1)
    {
        ret = s4AF_ReadPosition(&InitPos);
        if(ret == 0)
        {
            GAF001AFDB("[GAF001AF] Init Pos %6d \n", InitPos);
            spin_lock(&g_GAF001AF_SpinLock);
            g_u4CurrPosition = InitPos;
            spin_unlock(&g_GAF001AF_SpinLock);
        }
        else
        {    
            spin_lock(&g_GAF001AF_SpinLock);
            g_u4CurrPosition = 0;
            spin_unlock(&g_GAF001AF_SpinLock);
        }
        spin_lock(&g_GAF001AF_SpinLock);
        g_s4GAF001AF_Opened = 2;
        spin_unlock(&g_GAF001AF_SpinLock);
    }

    if (g_u4CurrPosition < a_u4Position)
    {
        spin_lock(&g_GAF001AF_SpinLock);    
        g_i4Dir = 1;
        spin_unlock(&g_GAF001AF_SpinLock);    
    }
    else if (g_u4CurrPosition > a_u4Position)
    {
        spin_lock(&g_GAF001AF_SpinLock);    
        g_i4Dir = -1;
        spin_unlock(&g_GAF001AF_SpinLock);            
    }
    else {return 0;}

    spin_lock(&g_GAF001AF_SpinLock);    
    g_u4TargetPosition = a_u4Position;
    spin_unlock(&g_GAF001AF_SpinLock);    
    
    spin_lock(&g_GAF001AF_SpinLock);
    g_i4MotorStatus = 0;
    spin_unlock(&g_GAF001AF_SpinLock);    

    if(s4AF_WritePosition(g_u4TargetPosition) == 0)
    {
        spin_lock(&g_GAF001AF_SpinLock);        
        g_u4CurrPosition = g_u4TargetPosition;
        spin_unlock(&g_GAF001AF_SpinLock);                
    }
    else
    {
        GAF001AFDB("[GAF001AF] set I2C failed when moving the motor \n");            
        spin_lock(&g_GAF001AF_SpinLock);
        g_i4MotorStatus = -1;
        spin_unlock(&g_GAF001AF_SpinLock);                
    }

    return 0;
}

inline static void setGAF001AFInf(u16 a_u4Position)
{
    spin_lock(&g_GAF001AF_SpinLock);
    g_u4GAF001AF_INF = a_u4Position;
    spin_unlock(&g_GAF001AF_SpinLock);    
}

inline static void setGAF001AFMacro(u16 a_u4Position)
{
    spin_lock(&g_GAF001AF_SpinLock);
    g_u4GAF001AF_MACRO = a_u4Position;
    spin_unlock(&g_GAF001AF_SpinLock);         
}

////////////////////////////////////////////////////////////////
static long GAF001AF_Ioctl(
struct file * a_pstFile,
unsigned int a_u4Command,
unsigned long a_u4Param)
{
    long i4RetValue = 0;

    switch(a_u4Command)
    {
        case GAF001AFIOC_G_MOTORINFO :
            getGAF001AFInfo((__user stGAF001AF_MotorInfo *)(a_u4Param));
        break;
        case GAF001AFIOC_T_MOVETO :
            i4RetValue = moveGAF001AF(a_u4Param);
        break;
 
        case GAF001AFIOC_T_SETINFPOS :
            setGAF001AFInf(a_u4Param);
        break;
        case GAF001AFIOC_T_SETMACROPOS :
            setGAF001AFMacro(a_u4Param);
        break;
        case GAF001AFIOC_T_SETPARA :
           // i4RetValue = setPara(a_u4Param);
        break;

        default :
              GAF001AFDB("[GAF001AF] No CMD \n");
            i4RetValue = -EPERM;
        break;
    }

    return i4RetValue;
}

//Main jobs:
// 1.check for device-specified errors, device not ready.
// 2.Initialize the device if it is opened for the first time.
// 3.Update f_op pointer.
// 4.Fill data structures into private_data
static int GAF001AF_Open(struct inode * a_pstInode, struct file * a_pstFile)
{
    GAF001AFDB("[GAF001AF] GAF001AF_Open - Start\n");
    if(g_s4GAF001AF_Opened)
    {
        GAF001AFDB("[GAF001AF] the device is opened \n");
        return -EBUSY;
    }
    spin_lock(&g_GAF001AF_SpinLock);
    g_s4GAF001AF_Opened = 1;
    spin_unlock(&g_GAF001AF_SpinLock);
    GAF001AFDB("[GAF001AF] GAF001AF_Open - End\n");
    return 0;
}

//Main jobs:
// 1.Deallocate anything that "open" allocated in private_data.
// 2.Shut down the device on last close.
// 3.Only called once on last time.
// Q1 : Try release multiple times.
static int GAF001AF_Release(struct inode * a_pstInode, struct file * a_pstFile)
{
    GAF001AFDB("[GAF001AF] GAF001AF_Release - Start\n");
    if (g_s4GAF001AF_Opened)
    {
        GAF001AFDB("[GAF001AF] feee \n");
         s4AF_uninit();                          
        spin_lock(&g_GAF001AF_SpinLock);
        g_s4GAF001AF_Opened = 0;
        spin_unlock(&g_GAF001AF_SpinLock);
    }

    GAF001AFDB("[GAF001AF] GAF001AF_Release - End\n");
    return 0;
}

static const struct file_operations g_stGAF001AF_fops = 
{
    .owner = THIS_MODULE,
    .open = GAF001AF_Open,
    .release = GAF001AF_Release,
    .unlocked_ioctl = GAF001AF_Ioctl
};

inline static s16 Register_GAF001AF_CharDrv(void)
{
    struct device* vcm_device = NULL;

    GAF001AFDB("[GAF001AF] Register_GAF001AF_CharDrv - Start\n");

    //Allocate char driver no.
    if( alloc_chrdev_region(&g_GAF001AF_devno, 0, 1,GAF001AF_DRVNAME) )
    {
        GAF001AFDB("[GAF001AF] Allocate device no failed\n");
        return -EAGAIN;
    }

    //Allocate driver
    g_pGAF001AF_CharDrv = cdev_alloc();

    if(NULL == g_pGAF001AF_CharDrv)
    {
        unregister_chrdev_region(g_GAF001AF_devno, 1);
        GAF001AFDB("[GAF001AF] Allocate mem for kobject failed\n");
        return -ENOMEM;
    }

    //Attatch file operation.
    cdev_init(g_pGAF001AF_CharDrv, &g_stGAF001AF_fops);
    g_pGAF001AF_CharDrv->owner = THIS_MODULE;

    //Add to system
    if(cdev_add(g_pGAF001AF_CharDrv, g_GAF001AF_devno, 1))
    {
        GAF001AFDB("[GAF001AF] Attatch file operation failed\n");
        unregister_chrdev_region(g_GAF001AF_devno, 1);
        return -EAGAIN;
    }
    actuator_class = class_create(THIS_MODULE, "actuatordrv");
    if (IS_ERR(actuator_class)) {
        int ret = PTR_ERR(actuator_class);
        GAF001AFDB("Unable to create class, err = %d\n", ret);
        return ret;            
    }
    vcm_device = device_create(actuator_class, NULL, g_GAF001AF_devno, NULL, GAF001AF_DRVNAME);

    if(NULL == vcm_device)
    {
        return -EIO;
    }
    
    GAF001AFDB("[GAF001AF] Register_GAF001AF_CharDrv - End\n");    
    return 0;
}

inline static void Unregister_GAF001AF_CharDrv(void)
{
    GAF001AFDB("[GAF001AF] Unregister_GAF001AF_CharDrv - Start\n");

    //Release char driver
    cdev_del(g_pGAF001AF_CharDrv);
    unregister_chrdev_region(g_GAF001AF_devno, 1);
    device_destroy(actuator_class, g_GAF001AF_devno);
    class_destroy(actuator_class);

    GAF001AFDB("[GAF001AF] Unregister_GAF001AF_CharDrv - End\n");    
}

//////////////////////////////////////////////////////////////////////

static int GAF001AF_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int GAF001AF_i2c_remove(struct i2c_client *client);
static const struct i2c_device_id GAF001AF_i2c_id[] = {{GAF001AF_DRVNAME,0},{}};   
struct i2c_driver GAF001AF_i2c_driver = {                       
    .probe = GAF001AF_i2c_probe,                                   
    .remove = GAF001AF_i2c_remove,                           
    .driver.name = GAF001AF_DRVNAME,                 
    .id_table = GAF001AF_i2c_id,                             
};  
static int GAF001AF_i2c_remove(struct i2c_client *client) {
    return 0;
}

/* Kirby: add new-style driver {*/
static int GAF001AF_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    s16 i4RetValue = 0;

    GAF001AFDB("[GAF001AF] GAF001AF_i2c_probe\n");
    /* Kirby: add new-style driver { */
    g_pstGAF001AF_I2Cclient = client;
    g_pstGAF001AF_I2Cclient->addr = (0x18 >> 1);

    //Register char driver
    i4RetValue = Register_GAF001AF_CharDrv();
    if(i4RetValue)
    {
        GAF001AFDB("[GAF001AF] register char device failed!\n");
        return i4RetValue;
    }
    spin_lock_init(&g_GAF001AF_SpinLock);
    GAF001AFDB("[GAF001AF] Attached!! \n");

    return 0;
}

static int GAF001AF_probe(struct platform_device *pdev)
{
    return i2c_add_driver(&GAF001AF_i2c_driver);
}

static int GAF001AF_remove(struct platform_device *pdev)
{
    i2c_del_driver(&GAF001AF_i2c_driver);
    return 0;
}

static int GAF001AF_suspend(struct platform_device *pdev, pm_message_t mesg)
{
    return 0;
}

static int GAF001AF_resume(struct platform_device *pdev)
{
    return 0;
}

// platform structure
static struct platform_driver g_stGAF001AF_Driver = {
    .probe        = GAF001AF_probe,
    .remove    = GAF001AF_remove,
    .suspend    = GAF001AF_suspend,
    .resume    = GAF001AF_resume,
    .driver        = {
        .name    = "lens_actuator",
        .owner    = THIS_MODULE,
    }
};
static int __init GAF001AF_i2C_init(void)
{
    i2c_register_board_info(LENS_I2C_BUSNUM, &kd_lens_dev, 1);
    if(platform_driver_register(&g_stGAF001AF_Driver)){
        GAF001AFDB("failed to register GAF001AF driver\n");
        return -ENODEV;
    }

    return 0;
}

static void __exit GAF001AF_i2C_exit(void)
{
    platform_driver_unregister(&g_stGAF001AF_Driver);
}

module_init(GAF001AF_i2C_init);
module_exit(GAF001AF_i2C_exit);

MODULE_DESCRIPTION("GAF001AF lens module driver");
MODULE_AUTHOR("James-cc wu <james-cc.wu@Mediatek.com>");
MODULE_LICENSE("GPL");
