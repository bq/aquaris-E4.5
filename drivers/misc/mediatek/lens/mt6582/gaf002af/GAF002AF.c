/*
 * General voice coil motor driver
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
#include "GAF002AF.h"
#include "../camera/kd_camera_hw.h"

#define LENS_I2C_BUSNUM 1
static struct i2c_board_info __initdata kd_lens_dev={ I2C_BOARD_INFO("GAF002AF", 0x48)};

#define GAF002AF_DRVNAME "GAF002AF"

#define GAF002AF_DEBUG
#ifdef GAF002AF_DEBUG
#define GAF002AFDB printk
#else
#define GAF002AFDB(x,...)
#endif

static spinlock_t g_GAF002AF_SpinLock;
static struct i2c_client * g_pstGAF002AF_I2Cclient = NULL;
static dev_t g_GAF002AF_devno;
static struct cdev * g_pGAF002AF_CharDrv = NULL;
static struct class *actuator_class = NULL;
static s16 g_s4GAF002AF_Opened = 0;
static s16 g_i4MotorStatus = 0;
static s16 g_i4Dir = 0;
static u16 gAF_drvID = 0x9714; //default
static u16 g_u4GAF002AF_INF = 0;
static u16 g_u4GAF002AF_MACRO = 1023;
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
    g_pstGAF002AF_I2Cclient->addr = (0x48 >> 1);
    if (i2c_master_send(g_pstGAF002AF_I2Cclient, pBuff, 2) < 0 ) 
    {
        GAF002AFDB("[RUMBAAF] read I2C send failed!!\n");
        return -1;
    }

    GAF002AFDB("[RUMBAAFDB]I2C r (%x %x) \n",RamAddr,*RegData);
    if (i2c_master_recv(g_pstGAF002AF_I2Cclient, (u8*)RegData, 1) != 1) 
    {
        GAF002AFDB("[RUMBAAF] I2C read failed!! \n");
        return  -1;
    }
    return 0;
}

static s16 s4RUMBAAF_WriteReg(u16 a_u2Add, u16 a_u2Data)
{
    u8 puSendCmd[3] = {(u8)(a_u2Add>>8), (u8)(a_u2Add&0xFF), (u8)(a_u2Data&0xFF)};
    g_pstGAF002AF_I2Cclient->addr = (0x48 >> 1);
    if (i2c_master_send(g_pstGAF002AF_I2Cclient, puSendCmd, 3) < 0) 
    {
        GAF002AFDB("[RUMBAAF] I2C send failed!! %d\n",a_u2Add);
        return -1;
    }
    return 0;
}

static s16 s4RUMBAAF_WriteReg2(u16 a_u2Add, u16 a_u2Data)
{
    u8 puSendCmd[4] = {(u8)(a_u2Add>>8), (u8)(a_u2Add&0xFF), (u8)(a_u2Data>>8), (u8)(a_u2Data&0xFF)};
    g_pstGAF002AF_I2Cclient->addr = (0x48 >> 1);
    if (i2c_master_send(g_pstGAF002AF_I2Cclient, puSendCmd, 4) < 0) 
    {
        GAF002AFDB("[RUMBAAF] I2C send failed!! %d\n",a_u2Add);
        return -1;
    }
    return 0;
}
static s16 s4RUMBAAF_SrWriteReg(u16 a_u2Add, u16 a_u2Data)
{
    u8 puSendCmd[3] = { (u8)(a_u2Add>>8),  (u8)(a_u2Add&0xFF),(u8)(a_u2Data&0xFF) };
    g_pstGAF002AF_I2Cclient->addr = (0x20 >> 1);
    if (i2c_master_send(g_pstGAF002AF_I2Cclient, puSendCmd, 3) < 0) 
    {
        GAF002AFDB("[RUMBAAF] I2C sr send failed!! 1\n");
        return -1;
    }
    return 0;
}


/***********************************************************
  Reg position 
  ************************************************************/
static s16 s4DW9714AF_ReadReg(u16 * a_pu2Result)
{
    u8 pBuff[2];
    g_pstGAF002AF_I2Cclient->addr = (0x18 >> 1);
    if (i2c_master_recv(g_pstGAF002AF_I2Cclient, pBuff , 2) < 0) 
    {
        GAF002AFDB("[DW9714AF] I2C read failed!! \n");
        return -1;
    }
    *a_pu2Result = (((u16)pBuff[0]) << 4) + (u16)(pBuff[1] >> 4);
    return 0;
}

static s16 s4FM50AF_ReadReg(u16 * a_pu2Result)
{
    u8 pBuff[2];
    g_pstGAF002AF_I2Cclient->addr = (0x18 >> 1);
    if (i2c_master_recv(g_pstGAF002AF_I2Cclient, pBuff , 2) < 0) 
    {
        GAF002AFDB("[FM50AF] I2C read failed!! \n");
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
    g_pstGAF002AF_I2Cclient->addr = (0x18 >> 1);
    if (i2c_master_recv(g_pstGAF002AF_I2Cclient, pBuff , 2) < 0) 
    {
        GAF002AFDB("[BU6424AF]I2C read failed!! \n");
        return -1;
    }
    *a_pu2Result = (((u16)(pBuff[0] & 0x03)) << 8) + (u16)pBuff[1];
    return 0;
}
static s16 s4BU6429AF_ReadReg(u16 * a_pu2Result)
{
    u8 pBuff[2];    
    g_pstGAF002AF_I2Cclient->addr = (0x18 >> 1);
    if (i2c_master_recv(g_pstGAF002AF_I2Cclient, pBuff , 2) < 0) 
    {
        GAF002AFDB("[BU6429AF] I2C read failed!! \n");
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
    g_pstGAF002AF_I2Cclient->addr = (0x18 >> 1);
    if (i2c_master_send(g_pstGAF002AF_I2Cclient, &pBuff1, 1) < 0 ) 
    {
        GAF002AFDB("[DW9718AF] read I2C send failed!!\n");
        return -1;
    }
    if (i2c_master_recv(g_pstGAF002AF_I2Cclient, &RegData1, 1) != 1) 
    {
        GAF002AFDB("[DW9718AF] I2C read failed!! \n");
        return  -1;
    }

    if (i2c_master_send(g_pstGAF002AF_I2Cclient, &pBuff2, 1) < 0 ) 
    {
        GAF002AFDB("[DW9718AF] read I2C send failed!!\n");
        return -1;
    }

    if (i2c_master_recv(g_pstGAF002AF_I2Cclient, &RegData2, 1) != 1) 
    {
        GAF002AFDB("[DW9718AF] I2C read failed!! \n");
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
    g_pstGAF002AF_I2Cclient->addr = (0x6c >> 1);
    if (i2c_master_send(g_pstGAF002AF_I2Cclient, pBuff1, 2) < 0 ) 
    {
        GAF002AFDB("[OV8825AF] read I2C send failed!!\n");
        return -1;
    }
    if (i2c_master_recv(g_pstGAF002AF_I2Cclient, &RegData1, 1) != 1) 
    {
        GAF002AFDB("[OV8825AF] I2C read failed!! \n");
        return  -1;
    }

    if (i2c_master_send(g_pstGAF002AF_I2Cclient, pBuff2, 2) < 0 ) 
    {
        GAF002AFDB("[OV8825AF] read I2C send failed!!\n");
        return -1;
    }

    if (i2c_master_recv(g_pstGAF002AF_I2Cclient, &RegData2, 1) != 1) 
    {
        GAF002AFDB("[OV8825AF] I2C read failed!! \n");
        return  -1;
    }

    *a_pu2Result= ((u16)RegData1 +  (((u16)RegData2)<<8))>>4;

    return 0;
}



/***********************************************************
  Write position 
  ************************************************************/
static s16 s4DW9718AF_WriteReg(u16 a_u2Data)
{
    u8 puSendCmd[3] = {0x02,(u8)(a_u2Data >> 8) , (u8)(a_u2Data & 0xFF)};
    g_pstGAF002AF_I2Cclient->addr = (0x18 >> 1);
    if (i2c_master_send(g_pstGAF002AF_I2Cclient, puSendCmd, 3) < 0) 
    {
        GAF002AFDB("[DW9718AF] I2C send failed!! \n");
        return -1;
    }
    return 0;
}

static s16 s4DW9714AF_WriteReg(u16 a_u2Data)
{
    u8 puSendCmd[2] = {(u8)(a_u2Data >> 4) , (u8)((a_u2Data & 0xF) << 4)};
    g_pstGAF002AF_I2Cclient->addr = (0x18 >> 1);
    if (i2c_master_send(g_pstGAF002AF_I2Cclient, puSendCmd, 2) < 0) 
    {
        GAF002AFDB("[DW9714AF] I2C send failed!!\n");
        return -1;
    }
    return 0;
}
static s16 s4FM50AF_WriteReg(u16 a_u2Data)
{

    u8 puSendCmd[2] = {(u8)(a_u2Data >> 4) , (u8)(((a_u2Data & 0xF) << 4)+3)};
    g_pstGAF002AF_I2Cclient->addr = (0x18 >> 1);
    if (i2c_master_send(g_pstGAF002AF_I2Cclient, puSendCmd, 2) < 0) 
    {
        GAF002AFDB("[FM50AF] I2C send failed!! \n");
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
    g_pstGAF002AF_I2Cclient->addr = (0x18 >> 1);
    if (i2c_master_send(g_pstGAF002AF_I2Cclient, puSendCmd, 2) < 0) 
    {
        GAF002AFDB("[BU6424AF]I2C send failed!! \n");
        return -1;
    }
    return 0;
}
static s16 s4BU6429AF_WriteReg(u16 a_u2Data)
{
    u8 puSendCmd[2] = {(u8)(((a_u2Data >> 8) & 0x03) | 0xF4), (u8)(a_u2Data & 0xFF)};
    g_pstGAF002AF_I2Cclient->addr = (0x18 >> 1);
    if (i2c_master_send(g_pstGAF002AF_I2Cclient, puSendCmd, 2) < 0) 
    {
        GAF002AFDB("[BU6429AF] I2C send failed!! \n");
        return -1;
    }
    return 0;
}

static s16 s4AD5823AF_WriteReg(u16 a_u2Data)
{
    u8 VCMMSB[2] = {(u8)(0x04) , (u8)((a_u2Data >>8)&0x03)};
    u8 VCMLSB[2] = {(u8)(0x05) , (u8)(a_u2Data & 0xFF)};
    g_pstGAF002AF_I2Cclient->addr = (0x18 >> 1);    
    if (i2c_master_send(g_pstGAF002AF_I2Cclient, VCMMSB, 2) < 0) 
    {
        GAF002AFDB("[AD5823] I2C send failed!! \n");
        return -1;
    }
    if (i2c_master_send(g_pstGAF002AF_I2Cclient, VCMLSB, 2) < 0) 
    {
        GAF002AFDB("[AD5823] I2C send failed!! \n");
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

    g_pstGAF002AF_I2Cclient->addr = (0x6c >> 1);
    if (i2c_master_send(g_pstGAF002AF_I2Cclient, puSendCmd1, 3) < 0) 
    {
        GAF002AFDB("[OV8825AF] I2C sr send failed!! 1\n");
        return -1;
    }
    if (i2c_master_send(g_pstGAF002AF_I2Cclient, puSendCmd2, 3) < 0) 
    {
        GAF002AFDB("[OV8825AF] I2C sr send failed!! 1\n");
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

        default :
            ret = s4DW9714AF_WriteReg(a_u2Data);  
        break;
    }

    return ret;
}



/***********************************************************
  init action
  ************************************************************/
static s16 s4RUMBAAF_init()
{
    //power on OIS
    GAF002AFDB("[RUMBAAF] init \n");
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

static s16 s4RUMBAAF_uninit()
{
    GAF002AFDB("[RUMBAAF] feee \n");
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

/***********************************************************
  do init /uninit driver
  ************************************************************/
static s16 s4AF_init()
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

        default :
            break;
    }
    return 0;
}
static s16 s4AF_uninit()
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
            s4AF_WritePosition(200);
            msleep(10);
            s4AF_WritePosition(100);
            msleep(10);
            break;
        case 0x6334 : /* Rumba */
            s4RUMBAAF_uninit();
            break;
        default :
            break;
    }
    return 0;
}


inline static void getGAF002AFInfo(__user stGAF002AF_MotorInfo * pstMotorInfo)
{
    stGAF002AF_MotorInfo stMotorInfo;
    stMotorInfo.u4MacroPosition   = g_u4GAF002AF_MACRO;
    stMotorInfo.u4InfPosition     = g_u4GAF002AF_INF;
    stMotorInfo.u4CurrentPosition = g_u4CurrPosition;
    stMotorInfo.bIsSupportSR      = TRUE;

    if (g_i4MotorStatus == 1)    {stMotorInfo.bIsMotorMoving = 1;}
    else                        {stMotorInfo.bIsMotorMoving = 0;}

    if (g_s4GAF002AF_Opened >= 1)    {stMotorInfo.bIsMotorOpen = 1;}
    else                        {stMotorInfo.bIsMotorOpen = 0;}

    if(copy_to_user(pstMotorInfo , &stMotorInfo , sizeof(stGAF002AF_MotorInfo)))
    {
        GAF002AFDB("[GAF002AF] copy to user failed when getting motor information \n");
    }    
}

inline static s16 moveGAF002AF(u16 a_u4Position)
{
    s16 ret = 0;
    u16 InitPos = 0;
    if((a_u4Position>0x1000))
    {    
        gAF_drvID=a_u4Position;
        s4AF_init();
        return 0;
    }

    if((a_u4Position > g_u4GAF002AF_MACRO) || (a_u4Position < g_u4GAF002AF_INF))
    {
        GAF002AFDB("[GAF002AF] out of range \n");
        return -EINVAL;
    }

    if (g_s4GAF002AF_Opened == 1)
    {
        ret = s4AF_ReadPosition(&InitPos);
        if(ret == 0)
        {
            GAF002AFDB("[GAF002AF] Init Pos %6d \n", InitPos);
            spin_lock(&g_GAF002AF_SpinLock);
            g_u4CurrPosition = InitPos;
            spin_unlock(&g_GAF002AF_SpinLock);
        }
        else
        {    
            spin_lock(&g_GAF002AF_SpinLock);
            g_u4CurrPosition = 0;
            spin_unlock(&g_GAF002AF_SpinLock);
        }
        spin_lock(&g_GAF002AF_SpinLock);
        g_s4GAF002AF_Opened = 2;
        spin_unlock(&g_GAF002AF_SpinLock);
    }

    if (g_u4CurrPosition < a_u4Position)
    {
        spin_lock(&g_GAF002AF_SpinLock);    
        g_i4Dir = 1;
        spin_unlock(&g_GAF002AF_SpinLock);    
    }
    else if (g_u4CurrPosition > a_u4Position)
    {
        spin_lock(&g_GAF002AF_SpinLock);    
        g_i4Dir = -1;
        spin_unlock(&g_GAF002AF_SpinLock);            
    }
    else {return 0;}

    spin_lock(&g_GAF002AF_SpinLock);    
    g_u4TargetPosition = a_u4Position;
    spin_unlock(&g_GAF002AF_SpinLock);    
    
    spin_lock(&g_GAF002AF_SpinLock);
    g_i4MotorStatus = 0;
    spin_unlock(&g_GAF002AF_SpinLock);    

    if(s4AF_WritePosition(g_u4TargetPosition) == 0)
    {
        spin_lock(&g_GAF002AF_SpinLock);        
        g_u4CurrPosition = g_u4TargetPosition;
        spin_unlock(&g_GAF002AF_SpinLock);                
    }
    else
    {
        GAF002AFDB("[GAF002AF] set I2C failed when moving the motor \n");            
        spin_lock(&g_GAF002AF_SpinLock);
        g_i4MotorStatus = -1;
        spin_unlock(&g_GAF002AF_SpinLock);                
    }

    return 0;
}

inline static void setGAF002AFInf(u16 a_u4Position)
{
    spin_lock(&g_GAF002AF_SpinLock);
    g_u4GAF002AF_INF = a_u4Position;
    spin_unlock(&g_GAF002AF_SpinLock);    
}

inline static void setGAF002AFMacro(u16 a_u4Position)
{
    spin_lock(&g_GAF002AF_SpinLock);
    g_u4GAF002AF_MACRO = a_u4Position;
    spin_unlock(&g_GAF002AF_SpinLock);         
}

////////////////////////////////////////////////////////////////
static long GAF002AF_Ioctl(
struct file * a_pstFile,
unsigned int a_u4Command,
unsigned long a_u4Param)
{
    long i4RetValue = 0;

    switch(a_u4Command)
    {
        case GAF002AFIOC_G_MOTORINFO :
            getGAF002AFInfo((__user stGAF002AF_MotorInfo *)(a_u4Param));
        break;
        case GAF002AFIOC_T_MOVETO :
            i4RetValue = moveGAF002AF(a_u4Param);
        break;
 
        case GAF002AFIOC_T_SETINFPOS :
            setGAF002AFInf(a_u4Param);
        break;
        case GAF002AFIOC_T_SETMACROPOS :
            setGAF002AFMacro(a_u4Param);
        break;
        case GAF002AFIOC_T_SETPARA :
           // i4RetValue = setPara(a_u4Param);
        break;

        default :
              GAF002AFDB("[GAF002AF] No CMD \n");
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
static s16 GAF002AF_Open(struct inode * a_pstInode, struct file * a_pstFile)
{
    GAF002AFDB("[GAF002AF] GAF002AF_Open - Start\n");
    if(g_s4GAF002AF_Opened)
    {
        GAF002AFDB("[GAF002AF] the device is opened \n");
        return -EBUSY;
    }
    spin_lock(&g_GAF002AF_SpinLock);
    g_s4GAF002AF_Opened = 1;
    spin_unlock(&g_GAF002AF_SpinLock);
    GAF002AFDB("[GAF002AF] GAF002AF_Open - End\n");
    return 0;
}

//Main jobs:
// 1.Deallocate anything that "open" allocated in private_data.
// 2.Shut down the device on last close.
// 3.Only called once on last time.
// Q1 : Try release multiple times.
static s16 GAF002AF_Release(struct inode * a_pstInode, struct file * a_pstFile)
{
    GAF002AFDB("[GAF002AF] GAF002AF_Release - Start\n");
    if (g_s4GAF002AF_Opened)
    {
        GAF002AFDB("[GAF002AF] feee \n");
         s4AF_uninit();                          
        spin_lock(&g_GAF002AF_SpinLock);
        g_s4GAF002AF_Opened = 0;
        spin_unlock(&g_GAF002AF_SpinLock);
    }

    GAF002AFDB("[GAF002AF] GAF002AF_Release - End\n");
    return 0;
}

static const struct file_operations g_stGAF002AF_fops = 
{
    .owner = THIS_MODULE,
    .open = GAF002AF_Open,
    .release = GAF002AF_Release,
    .unlocked_ioctl = GAF002AF_Ioctl
};

inline static s16 Register_GAF002AF_CharDrv(void)
{
    struct device* vcm_device = NULL;

    GAF002AFDB("[GAF002AF] Register_GAF002AF_CharDrv - Start\n");

    //Allocate char driver no.
    if( alloc_chrdev_region(&g_GAF002AF_devno, 0, 1,GAF002AF_DRVNAME) )
    {
        GAF002AFDB("[GAF002AF] Allocate device no failed\n");
        return -EAGAIN;
    }

    //Allocate driver
    g_pGAF002AF_CharDrv = cdev_alloc();

    if(NULL == g_pGAF002AF_CharDrv)
    {
        unregister_chrdev_region(g_GAF002AF_devno, 1);
        GAF002AFDB("[GAF002AF] Allocate mem for kobject failed\n");
        return -ENOMEM;
    }

    //Attatch file operation.
    cdev_init(g_pGAF002AF_CharDrv, &g_stGAF002AF_fops);
    g_pGAF002AF_CharDrv->owner = THIS_MODULE;

    //Add to system
    if(cdev_add(g_pGAF002AF_CharDrv, g_GAF002AF_devno, 1))
    {
        GAF002AFDB("[GAF002AF] Attatch file operation failed\n");
        unregister_chrdev_region(g_GAF002AF_devno, 1);
        return -EAGAIN;
    }
    actuator_class = class_create(THIS_MODULE, "actuatordrv2");
    if (IS_ERR(actuator_class)) {
        int ret = PTR_ERR(actuator_class);
        GAF002AFDB("Unable to create class, err = %d\n", ret);
        return ret;            
    }
    vcm_device = device_create(actuator_class, NULL, g_GAF002AF_devno, NULL, GAF002AF_DRVNAME);

    if(NULL == vcm_device)
    {
        return -EIO;
    }
    
    GAF002AFDB("[GAF002AF] Register_GAF002AF_CharDrv - End\n");    
    return 0;
}

inline static void Unregister_GAF002AF_CharDrv(void)
{
    GAF002AFDB("[GAF002AF] Unregister_GAF002AF_CharDrv - Start\n");

    //Release char driver
    cdev_del(g_pGAF002AF_CharDrv);
    unregister_chrdev_region(g_GAF002AF_devno, 1);
    device_destroy(actuator_class, g_GAF002AF_devno);
    class_destroy(actuator_class);

    GAF002AFDB("[GAF002AF] Unregister_GAF002AF_CharDrv - End\n");    
}

//////////////////////////////////////////////////////////////////////

static s16 GAF002AF_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static s16 GAF002AF_i2c_remove(struct i2c_client *client);
static const struct i2c_device_id GAF002AF_i2c_id[] = {{GAF002AF_DRVNAME,0},{}};   
struct i2c_driver GAF002AF_i2c_driver = {                       
    .probe = GAF002AF_i2c_probe,                                   
    .remove = GAF002AF_i2c_remove,                           
    .driver.name = GAF002AF_DRVNAME,                 
    .id_table = GAF002AF_i2c_id,                             
};  
static s16 GAF002AF_i2c_remove(struct i2c_client *client) {
    return 0;
}

/* Kirby: add new-style driver {*/
static s16 GAF002AF_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    s16 i4RetValue = 0;

    GAF002AFDB("[GAF002AF] GAF002AF_i2c_probe\n");
    /* Kirby: add new-style driver { */
    g_pstGAF002AF_I2Cclient = client;
    g_pstGAF002AF_I2Cclient->addr = (0x18 >> 1);

    //Register char driver
    i4RetValue = Register_GAF002AF_CharDrv();
    if(i4RetValue)
    {
        GAF002AFDB("[GAF002AF] register char device failed!\n");
        return i4RetValue;
    }
    spin_lock_init(&g_GAF002AF_SpinLock);
    GAF002AFDB("[GAF002AF] Attached!! \n");

    return 0;
}

static s16 GAF002AF_probe(struct platform_device *pdev)
{
    return i2c_add_driver(&GAF002AF_i2c_driver);
}

static s16 GAF002AF_remove(struct platform_device *pdev)
{
    i2c_del_driver(&GAF002AF_i2c_driver);
    return 0;
}

static s16 GAF002AF_suspend(struct platform_device *pdev, pm_message_t mesg)
{
    return 0;
}

static s16 GAF002AF_resume(struct platform_device *pdev)
{
    return 0;
}

// platform structure
static struct platform_driver g_stGAF002AF_Driver = {
    .probe        = GAF002AF_probe,
    .remove    = GAF002AF_remove,
    .suspend    = GAF002AF_suspend,
    .resume    = GAF002AF_resume,
    .driver        = {
        .name    = "lens_actuator2",
        .owner    = THIS_MODULE,
    }
};
static s16 __init GAF002AF_i2C_init(void)
{
    i2c_register_board_info(LENS_I2C_BUSNUM, &kd_lens_dev, 1);
    if(platform_driver_register(&g_stGAF002AF_Driver)){
        GAF002AFDB("failed to register GAF002AF driver\n");
        return -ENODEV;
    }
    return 0;
}

static void __exit GAF002AF_i2C_exit(void)
{
    platform_driver_unregister(&g_stGAF002AF_Driver);
}

module_init(GAF002AF_i2C_init);
module_exit(GAF002AF_i2C_exit);

MODULE_DESCRIPTION("GAF002AF lens module driver");
MODULE_AUTHOR("James-cc wu <james-cc.wu@Mediatek.com>");
MODULE_LICENSE("GPL");


