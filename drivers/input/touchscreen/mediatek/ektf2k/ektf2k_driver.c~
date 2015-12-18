//TP driver
#include "tpd.h"
#include "ektf2k_driver.h"
#include "tpd_custom_ektf2k.h"
#include "ektf2k_firmware.h"
#include <linux/dma-mapping.h>

#ifdef TP_PROXIMITY_SENSOR_NEW
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
static int PROXIMITY =0;
static int PROXIMITY_STATE = -1;
static U8 PROXIMITY_SLEEP = 0;

static int CTP_Face_Mode_State(void);
int CTP_Face_Mode_Switch(int onoff_state);
static  int Get_Ctp_Face_Mode(void);
s32 ektf2k_ps_operate(void *self, u32 command, void *buff_in, s32 size_in,
                      void *buff_out, s32 size_out, s32 *actualout);

extern void cktps_eint_func(void);
#endif

static int FW_AllInfor[4][3]=
{//ID / VER / WHICH 务必与升级后的版本号对应
   //HRC
  	{0x14f0,  0xf02,  0},

   //DM
  	{0x14f1,  0xf101,  1},

   //LC
  	{0x14f2,  0x0f01,  2},

   //HLT
   	{0x14f3,  0x1101,  3},

  	{0,  0,  0},
};


#ifdef ESD_CHECK
int have_interrupts = 0;
struct workqueue_struct *esd_wq = NULL;
struct delayed_work esd_work;
unsigned long delay = 2*HZ;
static void elan_touch_esd_func(struct work_struct *work);
#endif

#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
static int tpd_wb_start_local[TPD_WARP_CNT] = TPD_WARP_START;
static int tpd_wb_end_local[TPD_WARP_CNT]   = TPD_WARP_END;
#endif
#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
static int tpd_calmat_local[8]     = TPD_CALIBRATION_MATRIX;
static int tpd_def_calmat_local[8] = TPD_CALIBRATION_MATRIX;
#endif

//#define MTK_ELAN_DEBUG
#ifdef MTK_ELAN_DEBUG
#define MTK_TP_DEBUG(fmt, args ...) printk("Elan: %5d: " fmt, __LINE__,##args)

#define pr_tp(format, args...) printk("<0>" format, ##args)
#define pr_k(format, args...) printk("<0>" format, ##args)
#define pr_ch(format, args...)                      \
    printk("<0>" "%s <%d>,%s(),cheehwa_print:\n\t"  \
           format,__FILE__,__LINE__,__func__, ##args)
#else
#define MTK_TP_DEBUG(fmt, args ...)

#define pr_tp(format, args...)  do {} while (0)
#define pr_ch(format, args...)  do {} while (0)
#undef pr_k(format, args...)
#define pr_k(format, args...)  do {} while (0)
#endif


#ifdef TPD_HAVE_BUTTON
#define TPD_BUTTON_HEIGH                100
#define TPD_KEY_COUNT           3
#define TPD_KEYS                { KEY_MENU, KEY_HOMEPAGE, KEY_BACK}
#define TPD_KEYS_DIM            {{80,950,109,TPD_BUTTON_HEIGH},{240,950,109,TPD_BUTTON_HEIGH},{400,950,102,TPD_BUTTON_HEIGH}}

static int tpd_keys_local[TPD_KEY_COUNT] = TPD_KEYS;
static int tpd_keys_dim_local[TPD_KEY_COUNT][4] = TPD_KEYS_DIM;
#endif

// modify
#define SYSTEM_RESET_PIN_SR   135

//Add these Define

#ifdef IAP_PORTION                         //upgrade  FW DMA mode
#define _DMA_FW_UPGRADE_MODE_
#endif
#define PAGERETRY                   30
#define IAPRESTART                  5


#ifdef _DMA_MODE_
static uint8_t *gpDMABuf_va = NULL;
static uint32_t *gpDMABuf_pa = NULL;
#endif

#ifdef _DMA_FW_UPGRADE_MODE_
static uint8_t *gpDMAFWBuf_va = NULL;
static uint32_t *gpDMAFWBuf_pa = NULL;
static int elan_i2c_dma_fw_recv_data(struct i2c_client *client, uint8_t *buf,uint8_t len);
static int elan_i2c_dma_fw_send_data(struct i2c_client *client, uint8_t *buf,uint8_t len);
#endif

// For Firmware Update
#define ELAN_IOCTLID                            0xD0
#define IOCTL_I2C_SLAVE                 _IOW(ELAN_IOCTLID, 1, int)
#define IOCTL_MAJOR_FW_VER                  _IOR(ELAN_IOCTLID, 2, int)
#define IOCTL_MINOR_FW_VER                  _IOR(ELAN_IOCTLID, 3, int)
#define IOCTL_RESET                                 _IOR(ELAN_IOCTLID, 4, int)
#define IOCTL_IAP_MODE_LOCK                 _IOR(ELAN_IOCTLID, 5, int)
#define IOCTL_CHECK_RECOVERY_MODE   _IOR(ELAN_IOCTLID, 6, int)
#define IOCTL_FW_VER                            _IOR(ELAN_IOCTLID, 7, int)
#define IOCTL_X_RESOLUTION                  _IOR(ELAN_IOCTLID, 8, int)
#define IOCTL_Y_RESOLUTION                  _IOR(ELAN_IOCTLID, 9, int)
#define IOCTL_FW_ID                                 _IOR(ELAN_IOCTLID, 10, int)
#define IOCTL_ROUGH_CALIBRATE           _IOR(ELAN_IOCTLID, 11, int)
#define IOCTL_IAP_MODE_UNLOCK           _IOR(ELAN_IOCTLID, 12, int)
#define IOCTL_I2C_INT                           _IOR(ELAN_IOCTLID, 13, int)
#define IOCTL_RESUME                            _IOR(ELAN_IOCTLID, 14, int)
#define IOCTL_POWER_LOCK                    _IOR(ELAN_IOCTLID, 15, int)
#define IOCTL_POWER_UNLOCK                  _IOR(ELAN_IOCTLID, 16, int)
#define IOCTL_FW_UPDATE                         _IOR(ELAN_IOCTLID, 17, int)
#define IOCTL_BC_VER                            _IOR(ELAN_IOCTLID, 18, int)
#define IOCTL_2WIREICE                          _IOR(ELAN_IOCTLID, 19, int)

#define CUSTOMER_IOCTLID                        0xA0
#define IOCTL_CIRCUIT_CHECK                 _IOR(CUSTOMER_IOCTLID, 1, int)
#define IOCTL_GET_UPDATE_PROGREE    _IOR(CUSTOMER_IOCTLID, 2, int)

extern struct tpd_device *tpd;

uint8_t RECOVERY=0x00;
int FW_VERSION=0x00;
int X_RESOLUTION=0x00;
int Y_RESOLUTION=0x00;
int FW_ID=0x00;
int BC_VERSION = 0x00;
int work_lock=0x00;
int power_lock=0x00;
int circuit_ver=0x01;
int button_state = 0;
static int probe_flage=0;

/*++++i2c transfer start+++++++*/
#ifdef ELAN_3K_IC_SOLUTION
int file_fops_addr=0x10;
#else
int file_fops_addr=0x15;
#endif
/*++++i2c transfer end+++++++*/

int tpd_down_flag=0;
int tpd_reg_flag=0;// 1 -->elan; 0 -->other;

struct i2c_client *i2c_client = NULL;
struct task_struct *thread = NULL;
struct task_struct *update_thread = NULL;

static DECLARE_WAIT_QUEUE_HEAD(waiter);
static inline int elan_ktf2k_ts_parse_xy(uint8_t *data,
        uint16_t *x, uint16_t *y);
extern void mt_eint_unmask(unsigned int line);
extern void mt_eint_mask(unsigned int line);
extern void mt_eint_set_hw_debounce(unsigned int eintno, unsigned int ms);
extern unsigned int mt_eint_set_sens(unsigned int eintno, unsigned int sens);
extern void mt_eint_registration(unsigned int eint_num, unsigned int flag,
                                 void (EINT_FUNC_PTR) (void), unsigned int is_auto_umask);


static int tpd_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tpd_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
static int tpd_remove(struct i2c_client *client);
static int touch_event_handler(void *unused);


static int tpd_flag = 0;

#ifdef IAP_PORTION
uint8_t ic_status=0x00;  //0:OK 1:master fail 2:slave fail
int update_progree=0;

#ifdef ELAN_3K_IC_SOLUTION
uint8_t I2C_DATA[3] = {0x10, 0x20, 0x21};/*I2C devices address*/
#else
uint8_t I2C_DATA[3] = {0x15, 0x20, 0x21};/*I2C devices address*/
#endif

int is_OldBootCode = 0; // 0:new 1:old

static uint8_t* file_fw_data = NULL;

enum
{
    PageSize           = 132,
    PageNum            = 249,
    ACK_Fail           = 0x00,
    ACK_OK             = 0xAA,
    ACK_REWRITE        = 0x55,
};

enum
{
    E_FD               = -1,
};
#endif

static const struct i2c_device_id tpd_id[] =
{
    { "ektf2k", 0 },
    { }
};

#ifdef ELAN_3K_IC_SOLUTION
static struct i2c_board_info __initdata ektf2k_i2c_tpd = { I2C_BOARD_INFO("ektf2k", (0x20>>1))};
#else
static struct i2c_board_info __initdata ektf2k_i2c_tpd = { I2C_BOARD_INFO("ektf2k", (0x2a>>1))};
#endif

static struct i2c_driver tpd_i2c_driver =
{
    .driver = {
        .name = "ektf2k",
//        .owner = THIS_MODULE,
    },
    .probe = tpd_probe,
    .remove =  tpd_remove,
    .id_table = tpd_id,
    .detect = tpd_detect,
//    .address_data = &addr_data,
};

struct elan_ktf2k_ts_data
{
    struct i2c_client *client;
    struct input_dev *input_dev;
    struct workqueue_struct *elan_wq;
    struct work_struct work;
    struct early_suspend early_suspend;
    int intr_gpio;
// Firmware Information
    int fw_ver;
    int fw_id;
    int bc_ver;
    int x_resolution;
    int y_resolution;
// For Firmare Update
    struct miscdevice firmware;
    struct hrtimer timer;
};

static struct elan_ktf2k_ts_data *private_ts;
static int __hello_packet_handler(struct i2c_client *client);
static int __fw_packet_handler(struct i2c_client *client);
static int elan_ktf2k_ts_rough_calibrate(struct i2c_client *client);
static int tpd_resume(struct i2c_client *client);

#ifdef IAP_PORTION
static int update_fw_handler(void *unused);
int Update_FW_One(/*struct file *filp,*/ struct i2c_client *client, int recovery);
int IAPReset();
#endif

void tp_write_reg0(void){}
void tp_write_reg1(void){}


#ifdef _DMA_MODE_
static int elan_i2c_dma_recv_data(struct i2c_client *client, uint8_t *buf,uint8_t len)
{
    int rc;
    uint8_t *pReadData = 0;
    unsigned short addr = 0;
    addr = client->addr ;
    client->addr |= I2C_DMA_FLAG;
    pReadData = gpDMABuf_va;
    if(!pReadData)
    {
        pr_k("[elan] dma_alloc_coherent failed!\n");
        return -1;
    }
    rc = i2c_master_recv(client, gpDMABuf_pa, len);
    pr_k("[elan] elan_i2c_dma_recv_data rc=%d!\n",rc);
    copy_to_user(buf, pReadData, len);
    client->addr = addr;
    return rc;
}

static int elan_i2c_dma_send_data(struct i2c_client *client, uint8_t *buf,uint8_t len)
{
    int rc;
    unsigned short addr = 0;
    addr = client->addr ;
    client->addr |= I2C_DMA_FLAG;
    uint8_t *pWriteData = gpDMABuf_va;
    if(!pWriteData)
    {
        pr_k("[elan] dma_alloc_coherent failed!\n");
        return -1;
    }
    copy_from_user(pWriteData, ((void*)buf), len);

    rc = i2c_master_send(client, gpDMABuf_pa, len);
    pr_k("[elan] elan_i2c_dma_send_data rc=%d!\n",rc);
    client->addr = addr;
    return rc;
}
#endif

//DMA_FW_Upgrade Start Function
#ifdef _DMA_FW_UPGRADE_MODE_
static int elan_i2c_dma_fw_recv_data(struct i2c_client *client, uint8_t *buf,uint8_t len)
{
    int rc;
    uint8_t *pReadData = 0;
    unsigned short addr = 0;
    addr = client->addr ;
    client->addr |= I2C_DMA_FLAG;
    pReadData = gpDMAFWBuf_va;
    if(!pReadData)
    {
        pr_k("[elan] dma_alloc_coherent failed!\n");
        return -1;
    }
    rc = i2c_master_recv(client, gpDMAFWBuf_pa, len);
    pr_k("[elan] elan_i2c_dma_recv_data rc=%d!\n",rc);
    copy_to_user(buf, pReadData, len);
    client->addr = addr;
    return rc;
}

static int elan_i2c_dma_fw_send_data(struct i2c_client *client, uint8_t *buf,uint8_t len)
{
    int rc;
    unsigned short addr = 0;
    addr = client->addr ;
    client->addr |= I2C_DMA_FLAG;
    uint8_t *pWriteData = gpDMAFWBuf_va;
    if(!pWriteData)
    {
        pr_k("[elan] dma_alloc_coherent failed!\n");
        return -1;
    }
    copy_from_user(pWriteData, ((void*)buf), len);

    rc = i2c_master_send(client, gpDMAFWBuf_pa, len);
    pr_k("[elan] elan_i2c_dma_send_data rc=%d!\n",rc);
    client->addr = addr;
    return rc;
}
#endif
//DMA_FW_Upgrade End Function

// For Firmware Update
int elan_iap_open(struct inode *inode, struct file *filp)
{

    pr_k("[ELAN]into elan_iap_open\n");
    if (private_ts == NULL)  pr_k("private_ts is NULL~~~");

    return 0;
}

int elan_iap_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t elan_iap_write(struct file *filp, const char *buff, size_t count, loff_t *offp)
{
    int ret;
    char *tmp;

    pr_k("[ELAN]into elan_iap_write\n");
    if (count > 8192)
        count = 8192;

    tmp = kmalloc(count, GFP_KERNEL);

    if (tmp == NULL)
        return -ENOMEM;

    if (copy_from_user(tmp, buff, count))
    {
        return -EFAULT;
    }
#ifdef _DMA_MODE_
    ret = elan_i2c_dma_send_data(private_ts->client, tmp, count);
#else
    ret = i2c_master_send(private_ts->client, tmp, count);
#endif
    kfree(tmp);
    return (ret == 1) ? count : ret;

}

ssize_t elan_iap_read(struct file *filp, char *buff, size_t count, loff_t *offp)
{
    char *tmp;
    int ret;
    long rc;

    pr_k("[ELAN]into elan_iap_read\n");
    if (count > 8192)
        count = 8192;

    tmp = kmalloc(count, GFP_KERNEL);

    if (tmp == NULL)
        return -ENOMEM;
#ifdef _DMA_MODE_
    ret = elan_i2c_dma_recv_data(private_ts->client, tmp, count);
#else
    ret = i2c_master_recv(private_ts->client, tmp, count);
#endif
    if (ret >= 0)
        rc = copy_to_user(buff, tmp, count);

    kfree(tmp);

    //return ret;
    return (ret == 1) ? count : ret;

}

static long elan_iap_ioctl(/*struct inode *inode,*/ struct file *filp,    unsigned int cmd, unsigned long arg)
{

    int __user *ip = (int __user *)arg;
    pr_k("[ELAN]into elan_iap_ioctl\n");
    pr_k("[ELAN]cmd value %x\n",cmd);

    switch (cmd)
    {
        case IOCTL_I2C_SLAVE:
            private_ts->client->addr = (int __user)arg;
            private_ts->client->addr &= I2C_MASK_FLAG;
            private_ts->client->addr |= I2C_ENEXT_FLAG;
            //file_fops_addr = 0x15;
            break;
        case IOCTL_MAJOR_FW_VER:
            break;
        case IOCTL_MINOR_FW_VER:
            break;
        case IOCTL_RESET:

            mt_set_gpio_mode( GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO );
            mt_set_gpio_dir( GPIO_CTP_RST_PIN, GPIO_DIR_OUT );
            mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );
            msleep(10);
            //#if !defined(EVB)
            mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
            //#endif
            msleep(10);
            mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );

            break;
        case IOCTL_IAP_MODE_LOCK:
            if(work_lock==0)
            {
                pr_k("[elan]%s %x=IOCTL_IAP_MODE_LOCK\n", __func__,IOCTL_IAP_MODE_LOCK);
                work_lock=1;
                //disable_irq(CUST_EINT_TOUCH_PANEL_NUM);
                mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
                //cancel_work_sync(&private_ts->work);
            }
            break;
        case IOCTL_IAP_MODE_UNLOCK:
            if(work_lock==1)
            {
                work_lock=0;
                //enable_irq(CUST_EINT_TOUCH_PANEL_NUM);
                mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
            }
            break;
        case IOCTL_CHECK_RECOVERY_MODE:
            return RECOVERY;
            break;
        case IOCTL_FW_VER:
            __fw_packet_handler(private_ts->client);
            return FW_VERSION;
            break;
        case IOCTL_X_RESOLUTION:
            __fw_packet_handler(private_ts->client);
            return X_RESOLUTION;
            break;
        case IOCTL_Y_RESOLUTION:
            __fw_packet_handler(private_ts->client);
            return Y_RESOLUTION;
            break;
        case IOCTL_FW_ID:
            __fw_packet_handler(private_ts->client);
            return FW_ID;
            break;
        case IOCTL_ROUGH_CALIBRATE:
            return elan_ktf2k_ts_rough_calibrate(private_ts->client);
        case IOCTL_I2C_INT:
            put_user(mt_get_gpio_in(GPIO_CTP_EINT_PIN),ip);
            pr_k("[elan]GPIO_CTP_EINT_PIN = %d\n", mt_get_gpio_in(GPIO_CTP_EINT_PIN));

            break;
        case IOCTL_RESUME:
            tpd_resume(private_ts->client);
            break;
        case IOCTL_CIRCUIT_CHECK:
            return circuit_ver;
            break;
        case IOCTL_POWER_LOCK:
            power_lock=1;
            break;
        case IOCTL_POWER_UNLOCK:
            power_lock=0;
            break;
#ifdef IAP_PORTION
        case IOCTL_GET_UPDATE_PROGREE:
            update_progree=(int __user)arg;
            break;

        case IOCTL_FW_UPDATE:
            //RECOVERY = IAPReset(private_ts->client);
            RECOVERY=0;
            Update_FW_One(private_ts->client, RECOVERY);
#endif
        case IOCTL_BC_VER:
            __fw_packet_handler(private_ts->client);
            return BC_VERSION;
            break;
        default:
            break;
    }
    return 0;
}

struct file_operations elan_touch_fops =
{
    .open =             elan_iap_open,
    .write =            elan_iap_write,
    .read =             elan_iap_read,
    .release =              elan_iap_release,
    .unlocked_ioctl = elan_iap_ioctl,
};

#ifdef IAP_PORTION
int EnterISPMode(struct i2c_client *client, uint8_t  *isp_cmd)
{
    char buff[4] = {0};
    int len = 0;

#ifdef _DMA_FW_UPGRADE_MODE_
    len = elan_i2c_dma_fw_send_data(private_ts->client,isp_cmd,  sizeof(isp_cmd));
#else
    len = i2c_master_send(private_ts->client, isp_cmd,  sizeof(isp_cmd));
#endif

    if (len != sizeof(buff))
    {
        pr_k("[ELAN] ERROR: EnterISPMode fail! len=%d\r\n", len);
        return -1;
    }
    else
        pr_k("[ELAN] IAPMode write data successfully! cmd = [%2x, %2x, %2x, %2x]\n", isp_cmd[0], isp_cmd[1], isp_cmd[2], isp_cmd[3]);
    return 0;
}

int ExtractPage(struct file *filp, uint8_t * szPage, int byte)
{
    int len = 0;

    len = filp->f_op->read(filp, szPage,byte, &filp->f_pos);
    if (len != byte)
    {
        pr_k("[ELAN] ExtractPage ERROR: read page error, read error. len=%d\r\n", len);
        return -1;
    }

    return 0;
}

int WritePage(uint8_t * szPage, int byte)
{
    int len = 0;

#ifdef _DMA_FW_UPGRADE_MODE_
    len = elan_i2c_dma_fw_send_data(private_ts->client, szPage,  byte);
#else
    len = i2c_master_send(private_ts->client, szPage,  byte);
#endif

    if (len != byte)
    {
        pr_k("[ELAN] ERROR: write page error, write error. len=%d\r\n", len);
        return -1;
    }

    return 0;
}

int GetAckData(struct i2c_client *client)
{
    int len = 0;

    char buff[2] = {0};

#ifdef _DMA_FW_UPGRADE_MODE_
    len = elan_i2c_dma_fw_recv_data(private_ts->client, buff, sizeof(buff));
#else
    len = i2c_master_recv(private_ts->client, buff, sizeof(buff));
#endif

    if (len != sizeof(buff))
    {
        pr_k("[ELAN] ERROR: read data error, write 50 times error. len=%d\r\n", len);
        return -1;
    }

    pr_k("[ELAN] GetAckData:%x,%x\n",buff[0],buff[1]);
    if (buff[0] == 0xaa/* && buff[1] == 0xaa*/)
        return ACK_OK;
    else if (buff[0] == 0x55 && buff[1] == 0x55)
        return ACK_REWRITE;
    else
        return ACK_Fail;

    return 0;
}

void print_progress(int page, int ic_num, int j)
{
    int i, percent,page_tatol,percent_tatol;
    char str[256];
    str[0] = '\0';
    for (i=0; i<((page)/10); i++)
    {
        str[i] = '#';
        str[i+1] = '\0';
    }

    page_tatol=page+249*(ic_num-j);
    percent = ((100*page)/(249));
    percent_tatol = ((100*page_tatol)/(249*ic_num));

    if ((page) == (249))
        percent = 100;

    if ((page_tatol) == (249*ic_num))
        percent_tatol = 100;

    pr_k("\rprogress %s| %d%%", str, percent);

    if (page == (249))
        pr_k("\n");
}

/*
* Restet and (Send normal_command ?)
* Get Hello Packet
*/
int  IAPReset()
{
    int res;

    mt_set_gpio_mode( GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO );
    mt_set_gpio_dir( GPIO_CTP_RST_PIN, GPIO_DIR_OUT );
    mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );
    msleep(10);
    //#if !defined(EVB)
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
    //#endif
    msleep(10);
    mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );
    return 1;

#if 0
    pr_k("[ELAN] read Hello packet data!\n");
    res= __hello_packet_handler(client);
    return res;
#endif
}

/* Check Master & Slave is "55 aa 33 cc" */
int CheckIapMode(void)
{
    char buff[4] = {0},len = 0;
    //WaitIAPVerify(1000000);
    //len = read(fd, buff, sizeof(buff));


#ifdef _DMA_FW_UPGRADE_MODE_
    len = elan_i2c_dma_fw_recv_data(private_ts->client, buff, sizeof(buff));
#else
    len = i2c_master_recv(private_ts->client, buff, sizeof(buff));
#endif

    if (len != sizeof(buff))
    {
        pr_k("[ELAN] CheckIapMode ERROR: read data error,len=%d\r\n", len);
        return -1;
    }
    else
    {

        if (buff[0] == 0x55 && buff[1] == 0xaa && buff[2] == 0x33 && buff[3] == 0xcc)
        {
            //pr_k("[ELAN] CheckIapMode is 55 aa 33 cc\n");
            return 0;
        }
        else// if ( j == 9 )
        {
            pr_k("[ELAN] Mode= 0x%x 0x%x 0x%x 0x%x\r\n", buff[0], buff[1], buff[2], buff[3]);
            pr_k("[ELAN] ERROR:  CheckIapMode error\n");
            return -1;
        }
    }
    pr_k("\n");
}

int Update_FW_One(struct i2c_client *client, int recovery)
{
    int res = 0,ic_num = 1;
    int iPage = 0, rewriteCnt = 0; //rewriteCnt for PAGE_REWRITE
    int i = 0;
    uint8_t data;

    int restartCnt = 0, checkCnt = 0; // For IAP_RESTART
    //uint8_t recovery_buffer[4] = {0};
    int byte_count;
    uint8_t *szBuff = NULL;
    int curIndex = 0;
#ifdef ELAN_3K_IC_SOLUTION
    uint8_t isp_cmd[] = {0x45, 0x49, 0x41, 0x50};         //45 49 41 50
#else
    uint8_t isp_cmd[] = {0x54, 0x00, 0x12, 0x34};         //54 00 12 34
#endif
    uint8_t recovery_buffer[4] = {0};

IAP_RESTART:

    data=I2C_DATA[0];//Master
    pr_k("[ELAN] %s: address data=0x%x \r\n", __func__, data);

    if(RECOVERY != 0x80)
    {
        pr_k("[ELAN] Firmware upgrade normal mode !\n");

        IAPReset();
        msleep(20);

        res = EnterISPMode(private_ts->client, isp_cmd); //enter ISP mode


#ifdef _DMA_FW_UPGRADE_MODE_
        res = elan_i2c_dma_fw_recv_data(private_ts->client, recovery_buffer, 4);
#else
        res = i2c_master_recv(private_ts->client, recovery_buffer, 4);   //55 aa 33 cc
#endif

        pr_k("[ELAN] recovery byte data:%x,%x,%x,%x \n",recovery_buffer[0],recovery_buffer[1],recovery_buffer[2],recovery_buffer[3]);

        msleep(10);
    }
    else
        pr_k("[ELAN] Firmware upgrade recovery mode !\n");
    // Send Dummy Byte
    pr_k("[ELAN] send one byte data:%x,%x",private_ts->client->addr,data);

#ifdef _DMA_FW_UPGRADE_MODE_
    res = elan_i2c_dma_fw_send_data(private_ts->client, &data,  sizeof(data));
#else
    res = i2c_master_send(private_ts->client, &data,  sizeof(data));
#endif

    if(res!=sizeof(data))
    {
        pr_k("[ELAN] dummy error code = %d\n",res);
    }
    msleep(50);

    // Start IAP
    for( iPage = 1; iPage <= PageNum; iPage++ )
    {
    PAGE_REWRITE:

#if 1 // 132byte mode                
        szBuff = file_fw_data + curIndex;
        curIndex =  curIndex + PageSize;
        res = WritePage(szBuff, PageSize);
#endif

        msleep(50);
        res = GetAckData(private_ts->client);

        if (ACK_OK != res)
        {
            msleep(50);
            pr_k("[ELAN] ERROR: GetAckData fail! res=%d\r\n", res);
            if ( res == ACK_REWRITE )
            {
                rewriteCnt = rewriteCnt + 1;
                if (rewriteCnt == PAGERETRY)
                {
                    pr_k("[ELAN] ID 0x%02x %dth page ReWrite %d times fails!\n", data, iPage, PAGERETRY);
                    return E_FD;
                }
                else
                {
                    pr_k("[ELAN] ---%d--- page ReWrite %d times!\n",  iPage, rewriteCnt);
                    curIndex = curIndex - PageSize;
                    goto PAGE_REWRITE;
                }
            }
            else
            {
                restartCnt = restartCnt + 1;
                if (restartCnt >= 5)
                {
                    pr_k("[ELAN] ID 0x%02x ReStart %d times fails!\n", data, IAPRESTART);
                    return E_FD;
                }
                else
                {
                    pr_k("[ELAN] ===%d=== page ReStart %d times!\n",  iPage, restartCnt);
                    goto IAP_RESTART;
                }
            }
        }
        else
        {
            pr_k("  data : 0x%02x ",  data);
            rewriteCnt=0;
            print_progress(iPage,ic_num,i);
        }

        msleep(10);
    } // end of for(iPage = 1; iPage <= PageNum; iPage++)

    //if (IAPReset() > 0)
    pr_k("[ELAN] Update ALL Firmware successfully!\n");
    return 0;
}

#endif
// End Firmware Update


#if 0
static void elan_ktf2k_ts_early_suspend(struct early_suspend *h);
static void elan_ktf2k_ts_late_resume(struct early_suspend *h);
#endif

static ssize_t elan_ktf2k_gpio_show(struct device *dev,
                                    struct device_attribute *attr, char *buf)
{
    int ret = 0;
    struct elan_ktf2k_ts_data *ts = private_ts;

    //ret = gpio_get_value(ts->intr_gpio);
    ret = mt_get_gpio_in(GPIO_CTP_EINT_PIN);
    pr_k(KERN_DEBUG "GPIO_TP_INT_N=%d\n", ts->intr_gpio);
    sprintf(buf, "GPIO_TP_INT_N=%d\n", ret);
    ret = strlen(buf) + 1;
    return ret;
}

static DEVICE_ATTR(gpio, S_IRUGO, elan_ktf2k_gpio_show, NULL);

static ssize_t elan_ktf2k_vendor_show(struct device *dev,
                                      struct device_attribute *attr, char *buf)
{
    ssize_t ret = 0;
    struct elan_ktf2k_ts_data *ts = private_ts;

    sprintf(buf, "%s_x%4.4x\n", "ELAN_KTF2K", ts->fw_ver);
    ret = strlen(buf) + 1;
    return ret;
}

static int __elan_ktf2k_ts_poll(struct i2c_client *client)
{
    struct elan_ktf2k_ts_data *ts = i2c_get_clientdata(client);
    int status = 0, retry = 10;

    do
    {
        //status = gpio_get_value(ts->intr_gpio);
        status = mt_get_gpio_in(GPIO_CTP_EINT_PIN);
        pr_k("mtk-tpd:[elan]: %s: status = %d\n", __func__, status);
        retry--;
        msleep(20);
    }
    while (status == 1 && retry > 0);

    pr_k( "mtk-tpd:[elan]%s: poll interrupt status %s\n",
            __func__, status == 1 ? "high" : "low");

    //status=0;
    //pr_k("[elan]: %s: force status = 0\n", __func__);

    return (status == 0 ? 0 : -ETIMEDOUT);
}

static int elan_ktf2k_ts_poll(struct i2c_client *client)
{
    return __elan_ktf2k_ts_poll(client);
}

static int elan_ktf2k_ts_get_data(struct i2c_client *client, uint8_t *cmd,
                                  uint8_t *buf, size_t size)
{
    int rc;

    dev_dbg(&client->dev, "[elan]%s: enter\n", __func__);

    if (buf == NULL)
        return -EINVAL;


    if ((i2c_master_send(client, cmd, 4)) != 4)
    {
        dev_err(&client->dev,
                "[elan]%s: i2c_master_send failed\n", __func__);
        return -EINVAL;
    }

    rc = elan_ktf2k_ts_poll(client);
    if (rc < 0)
        return -EINVAL;
    else
    {
        if ((i2c_master_recv(client, buf, size) != size) || (buf[0] != CMD_S_PKT))
        {
            pr_k("mtk-tpd:[elan_ktf2k_ts_get_data] buf[0]=%x buf[1]=%x buf[2]=%x buf[3]=%x\n", buf[0], buf[1], buf[2], buf[3]);
            return -EINVAL;
        }
    }

    return 0;
}

static int __hello_packet_handler(struct i2c_client *client)
{
    int rc;
    uint8_t buf_recv[8] = { 0 };
    //uint8_t buf_recv1[4] = { 0 };

    //msleep(1500);
    msleep(100);
    rc = elan_ktf2k_ts_poll(client);
    if (rc < 0)
    {
        pr_k( "mtk-tpd:[elan] %s: Int poll failed!\n", __func__);
        RECOVERY=0x80;
        return RECOVERY;
    }

    rc = i2c_master_recv(client, buf_recv, 8);

    pr_k("mtk-tpd:[elan] %s: Hello Packet %2x:%2X:%2x:%2x\n", __func__, buf_recv[0], buf_recv[1], buf_recv[2], buf_recv[3]);
    /*  Received 8 bytes data will let TP die on old firmware on ektf21xx carbon player and MK5
        rc = i2c_master_recv(client, buf_recv, 8);
             pr_k("[elan] %s: hello packet %2x:%2X:%2x:%2x:%2x:%2X:%2x:%2x\n", __func__, buf_recv[0], buf_recv[1], buf_recv[2], buf_recv[3] , buf_recv[4], buf_recv[5], buf_recv[6], buf_recv[7]);
    */
    if((buf_recv[0]==0x55 && buf_recv[1]==0x55) ||(buf_recv[0]==0xCC && buf_recv[1]==0xCC))
    {
        tpd_reg_flag=1;// 1 -->elan; 0 -->other;
    }
    if(buf_recv[0]==0x55 && buf_recv[1]==0x55 && buf_recv[2]==0x80 && buf_recv[3]==0x80)
    {
        RECOVERY=0x80;
        FW_ID =  buf_recv[5] << 8 | buf_recv[4];// RECOVERY MODE for yeji TP 2013/11/27 no need to read FW ID
        pr_k("[elan] FW_ID = %x\r\n", FW_ID);

        rc = i2c_master_recv(client, buf_recv, 8);

        pr_k("mtk-tpd:[elan] %s: Bootcode Verson %2x:%2X:%2x:%2x\n", __func__, buf_recv[0], buf_recv[1], buf_recv[2], buf_recv[3]);
        return RECOVERY;
    }

    return 0;
}
//record the FW ID in IC section
static int write_check_fwid_to_rom(struct i2c_client *client, uint8_t *cmd, size_t size)
{
    int rc;
    uint8_t get_cmd[] = {0x53, 0xD3, 0x00, 0x01}; /* Get CHECK FWID */
    uint8_t buf_recv[4] = { 0 };
    uint8_t retry = 0;
	
    pr_k("[elan] check cmd: %02x, %02x, %02x, %02x\n", cmd[0], cmd[1], cmd[2], cmd[3]);

check_id:
    rc = elan_ktf2k_ts_get_data(client, get_cmd, buf_recv, 4);//get the infomation and show out
    if (rc < 0)
    {
        return rc;
    }
    pr_k("[elan] read SENSOR option: %02x, %02x, %02x, %02x\n", buf_recv[0], buf_recv[1], buf_recv[2], buf_recv[3]);

    if((buf_recv[2] == cmd[2]) && (buf_recv[3] == cmd[3]) )
        return 0;

    if ((i2c_master_send(client, cmd, sizeof(cmd))) != sizeof(cmd))
    {
        pr_k("[elan] %s: i2c_master_send failed\n", __func__);
        return -1;
    }
    msleep(50);
    if((++retry) < 5 )
    {
        pr_k("[elan] %s: retry %d times for sensor option!\n", __func__, retry);
	    goto check_id;
    }
	
    pr_k("[elan] %s: retry %d times failed !\n", __func__, retry);
    return -1;
}

static int __fw_packet_handler(struct i2c_client *client)
{
    int rc;
    int major, minor;
    uint8_t cmd[] = {CMD_R_PKT, 0x00, 0x00, 0x01}; /* Get Firmware Version*/
    uint8_t cmd_x[] = {0x53, 0x60, 0x00, 0x00};        /*Get x resolution*/
    uint8_t cmd_y[] = {0x53, 0x63, 0x00, 0x00};        /*Get y resolution*/
    uint8_t cmd_id[] = {0x53, 0xf0, 0x00, 0x01};   /*Get firmware ID*/
    uint8_t cmd_check_fwid[] = { 0x54, 0XD2, 0xFF,0xFF };  /* Get Check FWID */
    //uint8_t cmd_bc[] = {CMD_R_PKT, 0x01, 0x00, 0x01};/* Get BootCode Version*/
    uint8_t cmd_bc[] = {CMD_R_PKT, 0x10, 0x00, 0x01};/* Get BootCode Version*/
    uint8_t buf_recv[8] = {0};

    pr_k( "mtk-tpd:[elan] %s: n", __func__);

#if 1
// Firmware version
    rc = elan_ktf2k_ts_get_data(client, cmd, buf_recv, 4);
    if (rc < 0)
        return rc;
    major = ((buf_recv[1] & 0x0f) << 4) | ((buf_recv[2] & 0xf0) >> 4);
    minor = ((buf_recv[2] & 0x0f) << 4) | ((buf_recv[3] & 0xf0) >> 4);
//      ts->fw_ver = major << 8 | minor;
    FW_VERSION = major << 8 | minor;

#endif

#if 1
// Firmware ID
    rc = elan_ktf2k_ts_get_data(client, cmd_id, buf_recv, 4);
    if (rc < 0)
        return rc;
    major = ((buf_recv[1] & 0x0f) << 4) | ((buf_recv[2] & 0xf0) >> 4);
    minor = ((buf_recv[2] & 0x0f) << 4) | ((buf_recv[3] & 0xf0) >> 4);
    //ts->fw_id = major << 8 | minor;
    cmd_check_fwid[2] = major;
    cmd_check_fwid[3] = minor;
    rc = write_check_fwid_to_rom(client, cmd_check_fwid , 4);  // write check fwid info

    FW_ID = major << 8 | minor;
#endif

#if 1
// X Resolution
    rc = elan_ktf2k_ts_get_data(client, cmd_x, buf_recv, 4);
    if (rc < 0)
        return rc;
    minor = ((buf_recv[2])) | ((buf_recv[3] & 0xf0) << 4);
    //ts->x_resolution =minor;
    X_RESOLUTION = minor;
#endif

#if 1
// Y Resolution
    rc = elan_ktf2k_ts_get_data(client, cmd_y, buf_recv, 4);
    if (rc < 0)
        return rc;
    minor = ((buf_recv[2])) | ((buf_recv[3] & 0xf0) << 4);
    //ts->y_resolution =minor;
    Y_RESOLUTION = minor;
#endif

#if 1
// Bootcode version
    rc = elan_ktf2k_ts_get_data(client, cmd_bc, buf_recv, 4);
    if (rc < 0)
        return rc;
    major = ((buf_recv[1] & 0x0f) << 4) | ((buf_recv[2] & 0xf0) >> 4);
    minor = ((buf_recv[2] & 0x0f) << 4) | ((buf_recv[3] & 0xf0) >> 4);
    //ts->bc_ver = major << 8 | minor;
    BC_VERSION = major << 8 | minor;
#endif

    pr_k( "mtk-tpd:[elan] %s: firmware version: 0x%4.4x\n",
            __func__, FW_VERSION);
    pr_k( "mtk-tpd:[elan] %s: firmware ID: 0x%4.4x\n",
            __func__, FW_ID);
    pr_k( "mtk-tpd:[elan] %s: x resolution: %d, y resolution: %d\n",
            __func__, X_RESOLUTION, Y_RESOLUTION);
    pr_k( "mtk-tpd:[elan] %s: bootcode version: 0x%4.4x\n",
            __func__, BC_VERSION);
    return 0;
}

static inline int elan_ktf2k_ts_parse_xy(uint8_t *data,
        uint16_t *x, uint16_t *y)
{
    *x = *y = 0;

    *x = (data[0] & 0xf0);
    *x <<= 4;
    *x |= data[1];

    *y = (data[0] & 0x0f);
    *y <<= 8;
    *y |= data[2];

    return 0;
}

static int elan_ktf2k_ts_setup(struct i2c_client *client)
{
    int rc;

    rc = __hello_packet_handler(client);
    pr_k("[elan] hellopacket's rc = %d\n",rc);

    msleep(10);
    if (rc != 0x80)
    {
        rc = __fw_packet_handler(client);
        if (rc < 0)
            pr_k("mtk-tpd:[elan] %s, fw_packet_handler fail, rc = %d", __func__, rc);
        else
            pr_k("mtk-tpd:[elan] %s: firmware checking done.\n", __func__);
    }
    return rc; /* Firmware need to be update if rc equal to 0x80(Recovery mode)   */
}

static int elan_ktf2k_ts_rough_calibrate(struct i2c_client *client)
{
    uint8_t cmd[] = {CMD_W_PKT, 0x29, 0x00, 0x01};

    pr_k("[elan] %s: enter\n", __func__);
    pr_k("[elan] dump cmd: %02x, %02x, %02x, %02x\n",
           cmd[0], cmd[1], cmd[2], cmd[3]);

    if ((i2c_master_send(client, cmd, sizeof(cmd))) != sizeof(cmd))
    {
        dev_err(&client->dev,
                "[elan] %s: i2c_master_send failed\n", __func__);
        return -EINVAL;
    }

    return 0;
}

static int elan_ktf2k_ts_set_power_state(struct i2c_client *client, int state)
{
    uint8_t cmd[] = {CMD_W_PKT, 0x50, 0x00, 0x01};

    dev_dbg(&client->dev, "[elan] %s: enter\n", __func__);

    cmd[1] |= (state << 3);

    dev_dbg(&client->dev,
            "[elan] dump cmd: %02x, %02x, %02x, %02x\n",
            cmd[0], cmd[1], cmd[2], cmd[3]);

    if ((i2c_master_send(client, cmd, sizeof(cmd))) != sizeof(cmd))
    {
        dev_err(&client->dev,
                "[elan] %s: i2c_master_send failed\n", __func__);
        return -EINVAL;
    }

    return 0;
}

static int elan_ktf2k_ts_get_power_state(struct i2c_client *client)
{
    int rc = 0;
    uint8_t cmd[] = {CMD_R_PKT, 0x50, 0x00, 0x01};
    uint8_t buf[4], power_state;

    rc = elan_ktf2k_ts_get_data(client, cmd, buf, 4);
    if (rc)
        return rc;

    power_state = buf[1];
    dev_dbg(&client->dev, "[elan] dump repsponse: %0x\n", power_state);
    power_state = (power_state & PWR_STATE_MASK) >> 3;
    dev_dbg(&client->dev, "[elan] power state = %s\n",power_state == PWR_STATE_DEEP_SLEEP ? "Deep Sleep" : "Normal/Idle");

    return power_state;
}

static int elan_ktf2k_read_block(struct i2c_client *client, u8 addr, u8 *data, u8 len)
{
    int err;
    u8 beg = addr;
    struct i2c_msg msgs[2] =
    {
        {
            .addr = client->addr,
            .flags = 0,
            .len = 1,
            .buf= &beg
        },
        {
            .addr = client->addr,
            .flags = I2C_M_RD,
            .len = len,
            .buf = data,
            .ext_flag = I2C_DMA_FLAG,
        }
    };

    if (!client)
        return -EINVAL;

    err = i2c_transfer(client->adapter, msgs, sizeof(msgs)/sizeof(msgs[0]));
    if (err != len)
    {
        pr_k("[elan] elan_ktf2k_read_block err=%d\n", err);
        err = -EIO;
    }
    else
    {
        pr_k("[elan] elan_ktf2k_read_block ok\n");
        err = 0;    /*no error*/
    }
    return err;
}


static int elan_ktf2k_ts_recv_data(struct i2c_client *client, uint8_t *buf)
{
    int rc, bytes_to_recv=PACKET_SIZE;
    uint8_t *pReadData = 0;
    unsigned short addr = 0;

    if (buf == NULL)
        return -EINVAL;
    memset(buf, 0, bytes_to_recv);

#ifdef _DMA_MODE_
    addr = client->addr ;
    client->addr |= I2C_DMA_FLAG;
    pReadData = gpDMABuf_va;
    if(!pReadData)
    {
        pr_k("mtk-tpd:[elan] dma_alloc_coherent failed!\n");
    }
    rc = i2c_master_recv(client, gpDMABuf_pa, bytes_to_recv);
    copy_to_user(buf, pReadData, bytes_to_recv);
    client->addr = addr;
#ifdef ELAN_DEBUG
    MTK_TP_DEBUG("[elan_debug] %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15],buf[16], buf[17]);
#endif

#else
#ifdef NON_MTK_MODE    //I2C support > 8bits transfer
    rc = i2c_master_recv(client, buf, bytes_to_recv);      //for two finger and non-mtk five finger and ten finger
    if (rc != bytes_to_recv)
        pr_k("mtk-tpd:[elan_debug] The package error.\n");
    MTK_TP_DEBUG("[elan_recv] %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15],buf[16], buf[17]);
#else
    rc = i2c_master_recv(client, buf, 8);  //for two finger and non-mtk five finger and ten finger
    if (rc != 8)
        pr_k("mtk-tpd:[elan_debug] The first package error.\n");
    MTK_TP_DEBUG("[elan_recv] %x %x %x %x %x %x %x %x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);

#if 0
    //msleep(1);

    if (buf[0] == MTK_FINGERS_PKT)             //for mtk five finger
    {
        rc = i2c_master_recv(client, buf+ 8, 8);
        if (rc != 8)
            pr_k("mtk-tpd:[elan_debug] The second package error.\n");
        MTK_TP_DEBUG("[elan_recv] %x %x %x %x %x %x %x %x\n", buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);

        rc = i2c_master_recv(client, buf+ 16, 2);
        if (rc != 2)
            pr_k("mtk-tpd:[elan_debug] The third package error.\n");
        MTK_TP_DEBUG("[elan_recv] %x %x \n", buf[16], buf[17]);

    }
    else if (buf[0] == TEN_FINGERS_PKT)        //for ten finger
    {
        rc = i2c_master_recv(client, buf+ 8, 8);
        if (rc != 8)
            pr_k("[elan_debug] The second package error.\n");
        MTK_TP_DEBUG("[elan_recv] %x %x %x %x %x %x %x %x\n", buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);

        rc = i2c_master_recv(client, buf+ 16, 8);
        if (rc != 8)
            pr_k("mtk-tpd:[elan_debug] The third package error.\n");
        MTK_TP_DEBUG("[elan_recv] %x %x %x %x %x %x %x %x\n", buf[16], buf[17], buf[18], buf[19], buf[20], buf[21], buf[22], buf[23]);

        rc = i2c_master_recv(client, buf+ 24, 8);
        if (rc != 8)
            pr_k("[elan_debug] The four package error.\n");
        MTK_TP_DEBUG("[elan_recv] %x %x %x %x %x %x %x %x\n", buf[24], buf[25], buf[26], buf[27], buf[28], buf[29], buf[30], buf[31]);

        rc = i2c_master_recv(client, buf+ 32, 8);
        if (rc != 8)
            pr_k("mtk-tpd:[elan_debug] The five package error.\n");
        MTK_TP_DEBUG("mtk-tpd:[elan_recv] %x %x %x %x %x %x %x %x\n", buf[32], buf[33], buf[34], buf[35], buf[36], buf[37], buf[38], buf[39]);

        rc = i2c_master_recv(client, buf+ 40, 4);
        if (rc != 4)
            pr_k("mtk-tpd:[elan_debug] The six package error.\n");
        MTK_TP_DEBUG("mtk-tpd:[elan_recv] %x %x %x %x\n", buf[40], buf[41], buf[42], buf[43]);

    }
#endif
#endif
#endif

    return rc;
}

static  void tpd_down(int x, int y, int p) 
{
    // input_report_abs(tpd->dev, ABS_PRESSURE, p);
    if (RECOVERY_BOOT != get_boot_mode())
    {
        input_report_key(tpd->dev, BTN_TOUCH, 1);
    }
	 input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 1);
	 input_report_abs(tpd->dev, ABS_MT_PRESSURE, 1);
	 input_report_abs(tpd->dev, ABS_MT_POSITION_X, x);
	 input_report_abs(tpd->dev, ABS_MT_POSITION_Y, y);
	 //pr_tp(TPD_DEVICE "D[%4d %4d %4d] ", x, y, p);
	 pr_tp(TPD_DEVICE "[elan]: Touch Down[%4d %4d %4d]\n", x, y, p);
	 input_mt_sync(tpd->dev);
     if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
     {   
       tpd_button(x, y, 1);  
     }
	 if(y > TPD_RES_Y) //virtual key debounce to avoid android ANR issue
	 {
         msleep(50);
		 pr_tp(TPD_DEVICE "D virtual key \n");
	 }
	 TPD_EM_PRINT(x, y, x, y, p-1, 1);
 }

 static  void tpd_up(int x, int y,int *count)
{
	 //if(*count>0) {
		 //input_report_abs(tpd->dev, ABS_PRESSURE, 0);
		 input_report_key(tpd->dev, BTN_TOUCH, 0);
	 //input_report_abs(tpd->dev, ABS_MT_PRESSURE, 0);
    //input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 0);
		 //input_report_abs(tpd->dev, ABS_MT_POSITION_X, x);
		 //input_report_abs(tpd->dev, ABS_MT_POSITION_Y, y);
		 //pr_tp(TPD_DEVICE "U[%4d %4d %4d] ", x, y, 0);	

		   pr_tp(TPD_DEVICE "[elan]: Touch Up[%4d %4d %4d]\n", x, y, 0);

		 input_mt_sync(tpd->dev);
		 TPD_EM_PRINT(x, y, x, y, 0, 0);
	//	 (*count)--;
     if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
     {   
        tpd_button(x, y, 0); 
     }   		 
}

#ifdef SOFTKEY_AXIS_VER //SOFTKEY is reported via AXI
static void elan_ktf2k_ts_report_data(struct i2c_client *client, uint8_t *buf)
{
    //struct elan_ktf2k_ts_data *ts = i2c_get_clientdata(client);
    struct input_dev *idev = tpd->dev;
    uint16_t x, y;
    uint16_t fbits=0;
    uint8_t i, num, reported = 0;
    uint8_t idx, btn_idx;
    int finger_num;
    int limitY = Y_RESOLUTION - 100; // limitY need define by Case!

#ifdef TP_PROXIMITY_SENSOR_NEW
    hwm_sensor_data sensor_data;
    int ret = 0;

    if (buf[0] == 0x55)
    {
        if (PROXIMITY == 1)
        {
            CTP_Face_Mode_Switch(1);
            MTK_TP_DEBUG("mtk-tpd:[elan] 55 55 55 55--PS ON---\n", __func__);
        }
    }
    else if(buf[0] == 0xFA)
    {
        if(buf[2] == 0xAA && PROXIMITY == 1 ) // close to
        {
            PROXIMITY_STATE = 0;
            MTK_TP_DEBUG("tpd_touchinfo PROXIMITY_STATE %d\n",PROXIMITY_STATE );
        }
        else if(buf[2] == 0x55 && PROXIMITY == 1 ) // leave
        {
            PROXIMITY_STATE = 1;
            MTK_TP_DEBUG("tpd_touchinfo PROXIMITY_STATE %d\n",PROXIMITY_STATE );
        }
        if (PROXIMITY == 1)
        {
            //get raw data
            MTK_TP_DEBUG(" ps change\n");
            //map and store data to hwm_sensor_data
            sensor_data.values[0] = Get_Ctp_Face_Mode();
            sensor_data.value_divide = 1;
            sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
            //report to the up-layer
            ret = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data);

            if (ret)
            {
                MTK_TP_DEBUG("Call hwmsen_get_interrupt_data fail = %d\n", ret);
            }
        }
    }
#endif
    /* for 10 fingers       */
    if (buf[0] == TEN_FINGERS_PKT)
    {
        finger_num = 10;
        num = buf[2] & 0x0f;
        fbits = buf[2] & 0x30;
        fbits = (fbits << 4) | buf[1];
        idx=3;
        btn_idx=33;
    }
// for 5 fingers
    else if ((buf[0] == MTK_FINGERS_PKT) || (buf[0] == FIVE_FINGERS_PKT))
    {
        finger_num = 5;
        num = buf[1] & 0x07;
        fbits = buf[1] >>3;
        idx=2;
        btn_idx=17;
    }
    else
    {
// for 2 fingers
        finger_num = 2;
        num = buf[7] & 0x03;
        fbits = buf[7] & 0x03;
        idx=1;
        btn_idx=7;
    }

    switch (buf[0])
    {
        case MTK_FINGERS_PKT:
        case TWO_FINGERS_PKT:
        case FIVE_FINGERS_PKT:
        case TEN_FINGERS_PKT:
            //input_report_key(idev, BTN_TOUCH, 1);
            if (num == 0)
            {
                //dev_dbg(&client->dev, "no press\n");
                if(key_pressed < 0)
                {
                    input_report_key(tpd->dev, BTN_TOUCH, 0);
                    input_report_abs(tpd->dev, ABS_MT_PRESSURE, 0);
                    input_report_abs(idev, ABS_MT_TOUCH_MAJOR, 0);
                    input_report_abs(idev, ABS_MT_WIDTH_MAJOR, 0);
                    input_mt_sync(idev);
                    if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
                    {
                        tpd_button(x, y, 0);
                    }
                    TPD_EM_PRINT(x, y, x, y, 0, 0);
                }
                else
                {
                    //dev_err(&client->dev, "[elan] KEY_RELEASE: key_code:%d\n",OSD_mapping[key_pressed].key_event);
                    input_report_key(idev, OSD_mapping[key_pressed].key_event, 0);
                    key_pressed = -1;
                }
            }
            else
            {
                //dev_dbg(&client->dev, "[elan] %d fingers\n", num);
                //input_report_key(idev, BTN_TOUCH, 1);
                for (i = 0; i < finger_num; i++)
                {
                    if ((fbits & 0x01))
                    {
                        elan_ktf2k_ts_parse_xy(&buf[idx], &x, &y);
                        //elan_ktf2k_ts_parse_xy(&buf[idx], &y, &x);
                        //x = X_RESOLUTION-x;
                        //y = Y_RESOLUTION-y;
#if 1
                        if(X_RESOLUTION > 0 && Y_RESOLUTION > 0)
                        {
                            x = ( x * LCM_X_MAX )/X_RESOLUTION;
                            y = ( y * LCM_Y_MAX )/Y_RESOLUTION;
                        }
                        else
                        {
                            x = ( x * LCM_X_MAX )/ELAN_X_MAX;
                            y = ( y * LCM_Y_MAX )/ELAN_Y_MAX;
                        }
#endif
                        MTK_TP_DEBUG("[elan_debug SOFTKEY_AXIS_VER] %s, x=%d, y=%d\n",__func__, x , y);

                        if (!((x<=0) || (y<=0) || (x>=X_RESOLUTION) || (y>=Y_RESOLUTION)))
                        {
                            if ( y < limitY )
                            {
                                MTK_TP_DEBUG("mtk-tpd elan_ktf2k_ts_report_data x=%d y=%d id=%d \n",x,y,i);
                                input_report_key(tpd->dev, BTN_TOUCH, 1);
                                input_report_abs(tpd->dev, ABS_MT_PRESSURE, 1);
                                input_report_abs(idev, ABS_MT_TRACKING_ID, i);
                                input_report_abs(idev, ABS_MT_TOUCH_MAJOR, 8);
                                input_report_abs(idev, ABS_MT_POSITION_X, x);
                                input_report_abs(idev, ABS_MT_POSITION_Y, y);
                                input_mt_sync(idev);
                                if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
                                {
                                    tpd_button(x, y, 1);
                                }
                                TPD_EM_PRINT(x, y, x, y, i-1, 1);
                            }
                            else
                            {
                                int i=0;
                                for(i=0; i<4; i++)
                                {
                                    if((x > OSD_mapping[i].left_x) && (x < OSD_mapping[i].right_x))
                                    {
                                        //dev_err(&client->dev, "[elan] KEY_PRESS: key_code:%d\n",OSD_mapping[i].key_event);
                                        //pr_k("[elan] %d KEY_PRESS: key_code:%d\n", i, OSD_mapping[i].key_event);
                                        input_report_key(idev, OSD_mapping[i].key_event, 1);
                                        key_pressed = i;
                                    }
                                }
                            }
                            reported++;

                        } // end if border
                    } // end if finger status
                    fbits = fbits >> 1;
                    idx += 3;
                } // end for
            }

            if (reported)
                input_sync(idev);
            else
            {
                input_mt_sync(idev);
                input_sync(idev);
                MTK_TP_DEBUG("mtk-tpd elan_ktf2k_ts_report_data up\n");
            }

            break;
        default:
            MTK_TP_DEBUG("[elan] %s: unknown packet type: %0x\n", __func__, buf[0]);
#if 0
            if(buf[0]==0x66)
            {
                uint8_t cmd[] = {CMD_W_PKT, 0x50, 0x00, 0x01};

                pr_k("[elan] TP enter into sleep mode\n");
                if ((i2c_master_send(private_ts->client, cmd, sizeof(cmd))) != sizeof(cmd))
                {
                    pr_k("[elan] %s: i2c_master_send failed\n", __func__);
                    //return -retval;
                }
                mt_set_gpio_mode( GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO );
                mt_set_gpio_dir( GPIO_CTP_RST_PIN, GPIO_DIR_OUT );
                mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );
                msleep(10);
                mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
                msleep(10);
                mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );
                mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
                pr_k("mtk-tpd elan_ktf2k_ts_report_data packet 0x66 reset\n");

            }
#endif
            break;
    } // end switch
    return;
}
#else //SOFTKEY is reported via BTN bit
static void elan_ktf2k_ts_report_data(struct i2c_client *client, uint8_t *buf)
{
    /*struct elan_ktf2k_ts_data *ts = i2c_get_clientdata(client);*/
    struct input_dev *idev = tpd->dev;
    uint16_t x, y;
    uint16_t fbits=0;
    uint8_t i, num, reported = 0;
    uint8_t idx, btn_idx;
    int finger_num;
    static	int tmp_x = 0;
    static int tmp_y = 0;
#ifdef TP_PROXIMITY_SENSOR_NEW
    hwm_sensor_data sensor_data;
    int ret = 0;
#endif

    if (buf[0] == 0x55)
    {
    	//for finger up 
        input_report_key(idev, BTN_TOUCH, 0);
        input_report_abs(idev, ABS_MT_TOUCH_MAJOR, 0);
        input_report_abs(idev, ABS_MT_WIDTH_MAJOR, 0);
        input_mt_sync(idev);
        input_sync(idev);
		
	#ifdef TP_PROXIMITY_SENSOR_NEW
	MTK_TP_DEBUG("[ektf_ps][%s],PROXIMITY=%d\n",__func__,PROXIMITY);
        if (PROXIMITY == 1)
        {
            CTP_Face_Mode_Switch(1);
            MTK_TP_DEBUG("mtk-tpd:[elan] 55 55 55 55--PS ON---\n", __func__);
        }
	#endif
    }
#ifdef TP_PROXIMITY_SENSOR_NEW
    else if(buf[0] == 0xFA)
    {
        MTK_TP_DEBUG("[ektf_ps][%s],buf[2]=0x%x,PROXIMITY=%d\n",__func__,buf[2],PROXIMITY);
        if(buf[2] == 0xAA && PROXIMITY == 1 ) // close to
        {
            PROXIMITY_STATE = 0;
            MTK_TP_DEBUG("tpd_touchinfo PROXIMITY_STATE %d\n",PROXIMITY_STATE );
            //cktps_eint_func();
        }
        else if(buf[2] == 0x55 && PROXIMITY == 1 ) // leave
        {
            PROXIMITY_STATE = 1;
            MTK_TP_DEBUG("tpd_touchinfo PROXIMITY_STATE %d\n",PROXIMITY_STATE );
            //cktps_eint_func();
        }
        if (PROXIMITY == 1)
        {
            //get raw data
            MTK_TP_DEBUG(" ps change\n");
            //map and store data to hwm_sensor_data
            sensor_data.values[0] = Get_Ctp_Face_Mode();
            sensor_data.value_divide = 1;
            sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
            //report to the up-layer
            ret = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data);
            MTK_TP_DEBUG("[ektf_ps][%s],ret=%d\n",__func__,ret);

            if (ret)
            {
                MTK_TP_DEBUG("Call hwmsen_get_interrupt_data fail = %d\n", ret);
            }
        }
    }
#endif

    /* for 10 fingers       */
    if (buf[0] == TEN_FINGERS_PKT)
    {
        finger_num = 10;
        num = buf[2] & 0x0f;
        fbits = buf[2] & 0x30;
        fbits = (fbits << 4) | buf[1];
        idx=3;
        btn_idx=33;
    }
// for 5 fingers
    else if ((buf[0] == MTK_FINGERS_PKT) || (buf[0] == FIVE_FINGERS_PKT))
    {
        finger_num = 5;
        num = buf[1] & 0x07;
        fbits = buf[1] >>3;
        idx=2;
        btn_idx=17;
    }
    else
    {
// for 2 fingers
        finger_num = 2;
        num = buf[7] & 0x03;
        fbits = buf[7] & 0x03;
        idx=1;
        btn_idx=7;
    }


    switch (buf[0])
    {
        case MTK_FINGERS_PKT:
        case TWO_FINGERS_PKT:
        case FIVE_FINGERS_PKT:
        case TEN_FINGERS_PKT:
            //input_report_key(idev, BTN_TOUCH, 1);
            if (num == 0)
            {
                dev_dbg(&client->dev, "no press\n");
#ifdef ELAN_DEBUG
                MTK_TP_DEBUG("button_state0 = %x\n",button_state);
                MTK_TP_DEBUG("buf[btn_idx] = %x KEY_MENU=%x KEY_HOME=%x KEY_BACK=%x KEY_SEARCH =%x\n",buf[btn_idx], KEY_MENU, KEY_HOME, KEY_BACK, KEY_SEARCH);
#endif

                //if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
                //{
                //  tpd_button(x, y, 0);
                //}
                //TPD_EM_PRINT(x, y, x, y, 0, 0);

#ifdef ELAN_BUTTON
                if(button_state==0)
                {
                switch (buf[btn_idx]&0xFC)
                {
                    case ELAN_KEY_BACK:
                        MTK_TP_DEBUG("KEY back 1\n");
#if 0
#ifndef LCT_VIRTUAL_KEY
                        input_report_key(idev, KEY_BACK, 1);
#else
                        if(RECOVERY_BOOT!=get_boot_mode())
                            input_report_key(idev, BTN_TOUCH, 1);
                        input_report_abs(tpd->dev, ABS_MT_PRESSURE, 1);
                        input_report_abs(idev, ABS_MT_TOUCH_MAJOR, 20);
                        input_report_abs(idev, ABS_MT_POSITION_X, 400);
                        input_report_abs(idev, ABS_MT_POSITION_Y, 950);
                        input_mt_sync(tpd->dev);
#endif
                        if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
                        {
                            x=400;
                            y=950;
                            tpd_button(x, y, 1);
                            MTK_TP_DEBUG("[elan] FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode()\n\n",button_state);
                        }
                        button_state = KEY_BACK;
#else
                        tmp_x = tpd_keys_dim_local[2][0];
                        tmp_y = tpd_keys_dim_local[2][1];
                        tpd_down(tmp_x, tmp_y, 1);
                        button_state = KEY_BACK;
#endif
                        break;

                    case ELAN_KEY_HOME:
                        MTK_TP_DEBUG("KEY home 1\n");
#if 0
#ifndef LCT_VIRTUAL_KEY
                        input_report_key(idev, KEY_HOMEPAGE, 1);
#else
                        if(RECOVERY_BOOT!=get_boot_mode())
                            input_report_key(idev, BTN_TOUCH, 1);
                        input_report_abs(tpd->dev, ABS_MT_PRESSURE, 1);
                        input_report_abs(idev, ABS_MT_TOUCH_MAJOR, 20);
                        input_report_abs(idev, ABS_MT_POSITION_X, 240);
                        input_report_abs(idev, ABS_MT_POSITION_Y, 950);
                        input_mt_sync(tpd->dev);
#endif
                        if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
                        {
                            x=240;
                            y=950;
                            tpd_button(x, y, 1);
                            MTK_TP_DEBUG("[elan] FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode()\n\n",button_state);
                        }

                        button_state = KEY_HOMEPAGE;
#else
                        tmp_x = tpd_keys_dim_local[1][0];
                        tmp_y = tpd_keys_dim_local[1][1];
                        tpd_down(tmp_x, tmp_y, 1);
                        button_state = KEY_HOMEPAGE;
#endif
                        break;

                    case ELAN_KEY_MENU:
                        MTK_TP_DEBUG("KEY menu 1\n");
#if 0
#ifndef LCT_VIRTUAL_KEY
                        input_report_key(idev, KEY_MENU, 1);
#else
                        if(RECOVERY_BOOT!=get_boot_mode())
                            input_report_key(idev, BTN_TOUCH, 1);
                        input_report_abs(tpd->dev, ABS_MT_PRESSURE, 1);
                        input_report_abs(idev, ABS_MT_TOUCH_MAJOR, 20);
                        input_report_abs(idev, ABS_MT_POSITION_X, 80);
                        input_report_abs(idev, ABS_MT_POSITION_Y, 950);
                        input_mt_sync(tpd->dev);
#endif
                        if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
                        {
                            x=80;
                            y=950;
                            tpd_button(x, y, 1);
                            MTK_TP_DEBUG("[elan] FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode()\n\n",button_state);
                        }
                        button_state = KEY_MENU;
#else
                        tmp_x = tpd_keys_dim_local[0][0];
                        tmp_y = tpd_keys_dim_local[0][1];
                        tpd_down(tmp_x, tmp_y, 1);
                        button_state = KEY_MENU;
#endif
                        break;

                    // TOUCH release
                    default:
                        MTK_TP_DEBUG("mtk-tpd:[ELAN ] test tpd up\n");
#if 0
                        input_report_key(idev, BTN_TOUCH, 0);
                        //input_report_abs(tpd->dev, ABS_MT_PRESSURE, 0);
                        //input_report_abs(idev, ABS_MT_TOUCH_MAJOR, 0);
                        //input_report_abs(idev, ABS_MT_WIDTH_MAJOR, 0);
                        input_mt_sync(idev);
                        tpd_down_flag = 0;
                        if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
                        {
                            tpd_button(x, y, 0);
                            MTK_TP_DEBUG("[elan] FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode()\n\n",button_state);
                        }

                        button_state = 0;
#else
                        tpd_up(tmp_x, tmp_y, 0);
                        tpd_down_flag = 0;
#endif
                        break;
                }
                }
                else
                {
                    pr_tp("KEY tpd up 2\n");
                    tpd_up(tmp_x, tmp_y, 0);
                    tmp_x = 0;
                    tmp_y = 0;
                    button_state = 0;
                }
                //input_sync(idev);
#endif
            }
            else
            {
                //dev_dbg(&client->dev, "[elan] %d fingers\n", num);
                input_report_key(idev, BTN_TOUCH, 1);
                for (i = 0; i < finger_num; i++)
                {
                    if ((fbits & 0x01))
                    {
                        elan_ktf2k_ts_parse_xy(&buf[idx], &x, &y);
                        //elan_ktf2k_ts_parse_xy(&buf[idx], &y, &x);
#if 1
                        if(X_RESOLUTION > 0 && Y_RESOLUTION > 0)
                        {
                            x = ( x * LCM_X_MAX )/X_RESOLUTION;
                            y = ( y * LCM_Y_MAX )/Y_RESOLUTION;
                        }
                        else
                        {
                            x = ( x * LCM_X_MAX )/ELAN_X_MAX;
                            y = ( y * LCM_Y_MAX )/ELAN_Y_MAX;
                        }
#endif

                        //x = ( x * LCM_X_MAX )/ELAN_X_MAX;
                        //y = ( y * LCM_Y_MAX )/ELAN_Y_MAX;
#ifdef ELAN_DEBUG
                        MTK_TP_DEBUG("mtk-tpd:[elan_debug  BTN bit] %s, x=%d, y=%d\n",__func__, x , y);
#endif
                        //x = LCM_X_MAX-x;
                        //y = Y_RESOLUTION-y;
                        if (!((x<0) || (y<0) || (x>=LCM_X_MAX) || (y>=LCM_Y_MAX)))
                        {
                            input_report_key(idev, BTN_TOUCH, 1);
                            input_report_abs(tpd->dev, ABS_MT_PRESSURE, 1);
                            input_report_abs(idev, ABS_MT_TRACKING_ID, i);
                            input_report_abs(idev, ABS_MT_TOUCH_MAJOR, 20);
                            input_report_abs(idev, ABS_MT_POSITION_X, x);
                            input_report_abs(idev, ABS_MT_POSITION_Y, y);
                            input_mt_sync(idev);

                            //if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
                            //{
                            //    tpd_button(x, y, 1);
                            //}
                            //TPD_EM_PRINT(x, y, x, y, i-1, 1);

                            reported++;
                            tpd_down_flag=1;
                        } // end if border
                    } // end if finger status
                    fbits = fbits >> 1;
                    idx += 3;
                } // end for
            }
            if (reported)
                input_sync(idev);
            else
            {
                input_mt_sync(idev);
                input_sync(idev);
            }
            break;
        default:
            MTK_TP_DEBUG("mtk-tpd:[elan] %s: unknown packet type: %0x\n", __func__, buf[0]);
            break;
    } // end switch
    return;
}
#endif

static void elan_ktf2k_ts_work_func(struct work_struct *work)
{
    int rc;
    struct elan_ktf2k_ts_data *ts =
        container_of(work, struct elan_ktf2k_ts_data, work);
    uint8_t buf[PACKET_SIZE] = { 0 };

//               if (gpio_get_value(ts->intr_gpio))
    if (mt_get_gpio_in(GPIO_CTP_EINT_PIN))
    {
        //enable_irq(ts->client->irq);
        pr_k("[elan]: Detected Jitter at INT pin. \n");
        mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
        return;
    }

    rc = elan_ktf2k_ts_recv_data(ts->client, buf);

    if (rc < 0)
    {
        //enable_irq(ts->client->irq);
        pr_k("[elan] elan_ktf2k_ts_recv_data Error, Error code %d \n", rc);
        mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
        return;
    }

    //pr_k("[elan] %2x,%2x,%2x,%2x,%2x,%2x\n",buf[0],buf[1],buf[2],buf[3],buf[5],buf[6]);
    elan_ktf2k_ts_report_data(ts->client, buf);

    //enable_irq(ts->client->irq);
    mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);

    return;
}

static irqreturn_t elan_ktf2k_ts_irq_handler(int irq, void *dev_id)
{
    struct elan_ktf2k_ts_data *ts = dev_id;
    struct i2c_client *client = ts->client;

    dev_dbg(&client->dev, "[elan] %s\n", __func__);
    mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
    tpd_flag = 1;
    wake_up_interruptible(&waiter);
    return IRQ_HANDLED;
}

static int elan_ktf2k_ts_register_interrupt(struct i2c_client *client)
{
    struct elan_ktf2k_ts_data *ts = i2c_get_clientdata(client);
    int err = 0;

    err = request_irq(client->irq, elan_ktf2k_ts_irq_handler,
                      IRQF_TRIGGER_LOW, client->name, ts);
    if (err)
        dev_err(&client->dev, "[elan] %s: request_irq %d failed\n",
                __func__, client->irq);

    return err;
}

#ifdef IAP_PORTION
static int update_fw_handler(void *unused)
{
    int New_FW_ID;
    int New_FW_VER;
    //struct i2c_client client= private_ts->client;

    struct sched_param param = { .sched_priority = 4 };
    sched_setscheduler(current, SCHED_RR, &param);

    work_lock=1;
    //disable_irq(CUST_EINT_TOUCH_PANEL_NUM);
    power_lock=1;
    while(probe_flage == 0){
        msleep(20);//2014/7/8 for update
    }
    mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
    pr_k("[elan] start fw update\n");

    /* FW ID & FW VER*/
#ifdef ELAN_3K_IC_SOLUTION
    /*For ektf31xx iap ekt file   */
    pr_k("[ELAN] [0x7d64]=0x%02x,  [0x7d65]=0x%02x, [0x7d66]=0x%02x, [0x7d67]=0x%02x\n",  file_fw_data[32100],file_fw_data[32101],file_fw_data[32102],file_fw_data[32103]);
    New_FW_ID = file_fw_data[0x7d67]<<8  | file_fw_data[0x7d66] ;
    New_FW_VER = file_fw_data[0x7d65]<<8  | file_fw_data[0x7d64] ;

    pr_k(" FW_ID=0x%x,   New_FW_ID=0x%x \n",  FW_ID, New_FW_ID);
    pr_k(" FW_VERSION=0x%x,   New_FW_VER=0x%x \n",  FW_VERSION  , New_FW_VER);
#else
    /* For ektf21xx and ektf20xx iap ekt file  */
    pr_k("[ELAN]  [7bd0]=0x%02x,  [7bd1]=0x%02x, [7bd2]=0x%02x, [7bd3]=0x%02x\n",  file_fw_data[31696],file_fw_data[31697],file_fw_data[31698],file_fw_data[31699]);
    New_FW_ID = file_fw_data[31699]<<8  | file_fw_data[31698] ;
    New_FW_VER = file_fw_data[31697]<<8  | file_fw_data[31696] ;
    pr_k(" FW_ID=0x%x,   New_FW_ID=0x%x \n",  FW_ID, New_FW_ID);
    pr_k(" FW_VERSION=0x%x,   New_FW_VER=0x%x \n",  FW_VERSION  , New_FW_VER);
#endif

    /* for firmware auto-upgrade*/
    if (New_FW_ID   ==  FW_ID)
    {
        if (New_FW_VER > (FW_VERSION))
            Update_FW_One(private_ts->client, RECOVERY);

    }
    else
    {
        pr_k("FW_ID is different!");
        Update_FW_One(private_ts->client, RECOVERY);
    }

    //Update_FW_One(private_ts->client, RECOVERY);
    // Reset Touch Pannel
    mt_set_gpio_mode( GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO );
    mt_set_gpio_dir( GPIO_CTP_RST_PIN, GPIO_DIR_OUT );
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
    msleep(50);
    mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );
    msleep(300);
    // End Reset Touch Pannel

	//read TP infomation again for update new FW 20140708
    elan_ktf2k_ts_setup(private_ts->client);

    power_lock=0;
    work_lock=0;
    //enable_irq(CUST_EINT_TOUCH_PANEL_NUM);
    mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
    pr_k("[elan] end fw update\n");

    kthread_should_stop();
#ifdef _DMA_FW_UPGRADE_MODE_
    if(gpDMAFWBuf_va)
    {
        dma_free_coherent(NULL, 4096, gpDMAFWBuf_va, gpDMAFWBuf_pa);
        gpDMAFWBuf_va = NULL;
        gpDMAFWBuf_pa = NULL;
    }
#endif
    return 0;
}
#endif

static int touch_event_handler(void *unused)
{
    int rc;
    uint8_t buf[PACKET_SIZE] = { 0 };

    int touch_state = 3;
//      int button_state = 0;
    unsigned long time_eclapse;
    struct sched_param param = { .sched_priority = RTPM_PRIO_TPD };
    sched_setscheduler(current, SCHED_RR, &param);
    int last_key = 0;
    int key;
    int index = 0;
    int i =0;
    MTK_TP_DEBUG("mtk-tpd interrupt touch_event_handler\n");

    do
    {
        mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
        MTK_TP_DEBUG("mtk-tpd touch_event_handler mt_eint_unmask\n");
        set_current_state(TASK_INTERRUPTIBLE);
        wait_event_interruptible(waiter, tpd_flag != 0);
        tpd_flag = 0;
        set_current_state(TASK_RUNNING);
        mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
        MTK_TP_DEBUG("mtk-tpd touch_event_handler mt_eint_mask\n");
        rc = elan_ktf2k_ts_recv_data(private_ts->client, buf);

        if (rc < 0)
        {
            pr_k("mtk-tpd:[elan] rc<0\n");

            continue;
        }

        elan_ktf2k_ts_report_data(/*ts*/private_ts->client, buf);

    }
    while(!kthread_should_stop());

    return 0;
}

static int tpd_detect(struct i2c_client *client, int kind, struct i2c_board_info *info)
{
    strcpy(info->type, TPD_DEVICE);

    return 0;
}

static void tpd_eint_interrupt_handler(void)
{
    MTK_TP_DEBUG("TPD interrupt has been triggered\n");
    mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
#ifdef ESD_CHECK
    have_interrupts = 1;
#endif
    tpd_flag = 1;
    wake_up_interruptible(&waiter);
}

static int __RE_K_handler(struct i2c_client *client)
{
    int rc;
    uint8_t buf_recv[4] = { 0 };

    rc = elan_ktf2k_ts_poll(client);
    if (rc < 0)
    {
        pr_k( "[elan] %s: Int poll failed!\n", __func__);
    }

    i2c_master_recv(client, buf_recv, 4);

    pr_k("[elan] %s: RE-K Packet %2x:%2X:%2x:%2x\n", __func__, buf_recv[0], buf_recv[1], buf_recv[2], buf_recv[3]);

    return 0;
}

static void ctp_power_on(void)
{
    //power on, need confirm with SA
#ifdef TPD_POWER_SOURCE_CUSTOM
    hwPowerOn(TPD_POWER_SOURCE_CUSTOM, VOL_2800, "TP");
#else
    hwPowerOn(MT6323_POWER_LDO_VGP2, VOL_2800, "TP");
#endif

#ifdef TPD_POWER_SOURCE_1800
    hwPowerOn(TPD_POWER_SOURCE_1800, VOL_1800, "TP");
#endif

}

#ifdef IAP_PORTION
extern char tpd_desc[50];
static void get_vendor_info()
{
    int i,vendor_num = 0;
    //FW ID CHECK ----start by baojun.fu
    pr_k("KERN_ERR [elan] %s:  FW_ID: 0x%4.4x \n", __func__, FW_ID);
    vendor_num = sizeof(g_vendor_map)/sizeof(g_vendor_map[0]);
    for(i=0; i < vendor_num; i++)
    {
        if(FW_ID == g_vendor_map[i].vendor_id)
        {
            file_fw_data = g_vendor_map[i].fw_array;
#ifdef TINNO_DEVICE_INFO
            sprintf(tpd_desc, "%s",g_vendor_map[i].vendor_name);
            pr_k("[elan] %s:  tpd_desc=%s \n", __func__, g_vendor_map[i].vendor_name);
#endif
            return;
        }
    }
    pr_k(KERN_ERR "[elan] TP ID is error: no support!\n");
}
#endif

static int tpd_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int fw_err = 0;
    int New_FW_ID;
    int New_FW_VER;
    int retval = TPD_OK;
    static struct elan_ktf2k_ts_data ts;
    int retry_read = 0;

#if 0//def TP_PROXIMITY_SENSOR_NEW
    struct hwmsen_object obj_ps;
    s32 err = 0;
#endif
    client->addr |= I2C_ENEXT_FLAG;

    printk("mtk-tpd:[elan] %s:client addr is %x, TPD_DEVICE = ektf2k\n",__func__,client->addr);
    client->timing =  100;

#if 1
    i2c_client = client;
    private_ts = &ts;
    private_ts->client = client;
#endif
    ctp_power_on();
    msleep(10);

    MTK_TP_DEBUG("[elan] ELAN enter tpd_probe ,the i2c addr=0x%x\n", client->addr);


read_retry:
    // Reset Touch Pannel
    mt_set_gpio_mode( GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO );
    mt_set_gpio_dir( GPIO_CTP_RST_PIN, GPIO_DIR_OUT );
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
    msleep(50);
    mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );
    msleep(300);
    // End Reset Touch Pannel


    fw_err = elan_ktf2k_ts_setup(client);
    if ((fw_err < 0) || (FW_ID == 0))
    {
	    if((++retry_read) < 3 )
	    {
	        printk("[elan] %s: retry %d times for fw info !\n", __func__, retry_read);
	    	 goto read_retry;
	    }else
	    {
		    printk("[elan] %s: retry fw info %d times failed !\n", __func__, retry_read);
	    }
	
        printk(KERN_INFO "[elan] No Elan chip inside\n");
    }
	if(tpd_reg_flag == 0)//// 1 -->elan; 0 -->other;
	{
		return -1;
	}

#ifdef _DMA_MODE_
    gpDMABuf_va = (u8 *)dma_alloc_coherent(NULL, 4096, &gpDMABuf_pa, GFP_KERNEL);
    if(!gpDMABuf_va)
    {
        printk(KERN_INFO "[elan] Allocate DMA I2C Buffer failed\n");
    }
#endif
#ifdef _DMA_FW_UPGRADE_MODE_
    gpDMAFWBuf_va = (u8 *)dma_alloc_coherent(NULL, 4096, &gpDMAFWBuf_pa, GFP_KERNEL);
    if(!gpDMAFWBuf_va)
    {
        printk(KERN_INFO "[elan] Allocate DMA I2C Buffer failed\n");
    }
#endif

#ifndef LCT_VIRTUAL_KEY
    set_bit( KEY_BACK,  tpd->dev->keybit );
    set_bit( KEY_HOMEPAGE,  tpd->dev->keybit );
    set_bit( KEY_MENU,  tpd->dev->keybit );
#endif

    // Setup Interrupt Pin
    mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_EINT);
    mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_CTP_EINT_PIN, GPIO_PULL_DISABLE);
    //mt_eint_set_hw_debounce(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_CN);
    mt_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, EINTF_TRIGGER_FALLING, tpd_eint_interrupt_handler, 1);
    mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
    msleep(10);
    // End Setup Interrupt Pin

    tpd_load_status = 1;

    thread = kthread_run(touch_event_handler, 0, TPD_DEVICE);
    if(IS_ERR(thread))
    {
        retval = PTR_ERR(thread);
        printk(TPD_DEVICE "mtk-tpd:[elan]  failed to create kernel thread: %d\n", retval);
    }

    printk("mtk-tpd:[elan]  ELAN Touch Panel Device Probe %s\n", (retval < TPD_OK) ? "FAIL" : "PASS");

    // Firmware Update
    // MISC
    ts.firmware.minor = MISC_DYNAMIC_MINOR;
    ts.firmware.name = "elan-iap";
    ts.firmware.fops = &elan_touch_fops;
    ts.firmware.mode = S_IRWXUGO;

    if (misc_register(&ts.firmware) < 0)
        printk("mtk-tpd:[elan] misc_register failed!!\n");
    else
        MTK_TP_DEBUG("[elan] misc_register finished!!\n");
    // End Firmware Update

#ifdef IAP_PORTION
    get_vendor_info();
    //if no matched FW ID , DO NOT to update
    if (file_fw_data != NULL)
    {
    #if 0
        update_thread = kthread_run(update_fw_handler, 0, TPD_DEVICE);
        if(IS_ERR(update_thread))
        {
            retval = PTR_ERR(update_thread);
            printk(TPD_DEVICE "failed to create kernel update thread: %ld\n", retval);
        }
    #endif
    }
#endif

#if 0//def TP_PROXIMITY_SENSOR_NEW
    obj_ps.polling = 0;        //0--interrupt mode;1--polling mode;
    obj_ps.sensor_operate = ektf2k_ps_operate;

    if ((err = hwmsen_attach(ID_PROXIMITY, &obj_ps)))
    {
        TPD_DEBUG("hwmsen attach fail, return:%d.", err);
    }
	printk("[ektf_ps][%s],err=%d\n",__func__,err);
#endif

#ifdef ESD_CHECK
    INIT_DELAYED_WORK(&esd_work, elan_touch_esd_func);
    esd_wq = create_singlethread_workqueue("esd_wq");
    if (!esd_wq)
    {
        return -ENOMEM;

    }
    queue_delayed_work(esd_wq, &esd_work, delay);
#endif

    mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);

    probe_flage = 1;
    return 0;
}

#ifdef TP_PROXIMITY_SENSOR_NEW
u16  ckt_get_tp_replace_ps_value(void)
{
    if(1==PROXIMITY_STATE)
    {
        MTK_TP_DEBUG("[elan]ckt_get_tp_replace_ps_value 500\n");
        return 500;
    }
    else
    {
        MTK_TP_DEBUG("[elan]ckt_get_tp_replace_ps_value 100\n");
        return 100;
    }
}
static int CTP_Face_Mode_State(void)
{
    return PROXIMITY;
}

int CTP_Face_Mode_Switch(int onoff_state)
{

    if(work_lock == 1)
    {
        return -1;
    }
    U8 bWriteData[4] =
    {
        0x54, 0xC1, 0x00, 0x01
    };

    if(onoff_state==1)
    {
        PROXIMITY =1;
        bWriteData[1] = 0xC1;
        PROXIMITY_STATE = 1;
        PROXIMITY_SLEEP = 0;
    }
    else
    {
        PROXIMITY =0;
        bWriteData[1] = 0xC0;
        PROXIMITY_STATE = -1;
        PROXIMITY_SLEEP = 0;
    }

    pr_k("CTP_Face_Mode_Switch  onoff_state %d, PROXIMITY %d\n",onoff_state, PROXIMITY);

    return i2c_master_send( i2c_client, &bWriteData[0], 4);
}
static  int Get_Ctp_Face_Mode(void)
{
    pr_k("Get_Ctp_Face_Mode PROXIMITY_STATE %d\n",PROXIMITY_STATE);

    return PROXIMITY_STATE;
}


s32 ektf2k_ps_operate(void *self, u32 command, void *buff_in, s32 size_in,
                      void *buff_out, s32 size_out, s32 *actualout)
{
    s32 err = 0;
    s32 value;
    hwm_sensor_data *sensor_data;

    switch (command)
    {
        case SENSOR_DELAY:
            pr_k("hdx ektf2k_ps_operate SENSOR_DELAY\n");
            if ((buff_in == NULL) || (size_in < sizeof(int)))
            {
                TPD_DEBUG("Set delay parameter error!");
                err = -EINVAL;
            }

            // Do nothing
            break;

        case SENSOR_ENABLE:
            pr_k("hdx ektf2k_ps_operate SENSOR_ENABLE");
            if ((buff_in == NULL) || (size_in < sizeof(int)))
            {
                TPD_DEBUG("Enable sensor parameter error!");
                err = -EINVAL;
            }
            else
            {
                value = *(int *)buff_in;
                CTP_Face_Mode_Switch(value);
            }

            break;

        case SENSOR_GET_DATA:
            pr_k("hdx ektf2k_ps_operate SENSOR_GET_DATA\n");
            if ((buff_out == NULL) || (size_out < sizeof(hwm_sensor_data)))
            {
                TPD_DEBUG("Get sensor data parameter error!");
                err = -EINVAL;
            }
            else
            {
                sensor_data = (hwm_sensor_data *)buff_out;
                sensor_data->values[0] = Get_Ctp_Face_Mode();
                sensor_data->value_divide = 1;
                sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
            }

            break;

        default:
            TPD_DEBUG("proxmy sensor operate function no this parameter %d!\n", command);
            err = -1;
            break;
    }

    return err;
}
#endif

#ifdef ESD_CHECK
static void elan_touch_esd_func(struct work_struct *work)
{
    int res;
    uint8_t cmd[] = {0x53, 0x00, 0x00, 0x01};
    struct i2c_client *client = private_ts->client;
    //add by baojun.fu for i'm alive
    static int por_cnt = 0;

    pr_k("[elan esd] %s: enter.......\n", __FUNCTION__);      /* elan_dlx */
    if(work_lock == 1) //updating or doing something else
    {
        pr_k("[elan esd] %s: work locked ..\n", __FUNCTION__);        /* elan_dlx */
	    queue_delayed_work(esd_wq, &esd_work, delay);//add queue for IAP
        return;
    }

    if(have_interrupts == 1)
    {
        pr_k("[elan esd] %s: had interrup not need check\n", __func__);
    }
    else
    {
        if ((++por_cnt) >= 2)
        {
            por_cnt = 0;
            pr_k("[elan esd] %s: i'm alive failed need reset!\n", __func__);
            //reset here
            mt_set_gpio_mode(GPIO_CTP_RST_PIN, 0);
            mt_set_gpio_dir(GPIO_CTP_RST_PIN, 1);
            mt_set_gpio_out(GPIO_CTP_RST_PIN, 0);
            msleep(10);

            // for enable/reset pin
            mt_set_gpio_mode(GPIO_CTP_RST_PIN, 0);
            mt_set_gpio_dir(GPIO_CTP_RST_PIN, 1);
            mt_set_gpio_out(GPIO_CTP_RST_PIN, 1);
            msleep(10);
        }
        else
        {
            res = i2c_master_send(client, cmd, sizeof(cmd));
            if (res != sizeof(cmd))
            {
                pr_k("[elan esd] %s: i2c_master_send failed reset now\n", __func__);
                //reset here
                mt_set_gpio_mode(GPIO_CTP_RST_PIN, 0);
                mt_set_gpio_dir(GPIO_CTP_RST_PIN, 1);
                mt_set_gpio_out(GPIO_CTP_RST_PIN, 0);
                msleep(10);

                // for enable/reset pin
                mt_set_gpio_mode(GPIO_CTP_RST_PIN, 0);
                mt_set_gpio_dir(GPIO_CTP_RST_PIN, 1);
                mt_set_gpio_out(GPIO_CTP_RST_PIN, 1);
                msleep(10);

            }
            else
            {
                pr_k("[elan esd] %s: i2c_master_send successful\n", __func__);

                msleep(20);

                if(have_interrupts == 1)
                {
                    pr_k("[elan esd] %s: i2c_master_send successful, had response\n", __func__);
                }
                else
                {
                    pr_k("[elan esd] %s: i2c_master_send successful, no response need reset\n", __func__);
                    //reset here
                    mt_set_gpio_mode(GPIO_CTP_RST_PIN, 0);
                    mt_set_gpio_dir(GPIO_CTP_RST_PIN, 1);
                    mt_set_gpio_out(GPIO_CTP_RST_PIN, 0);
                    msleep(10);

                    // for enable/reset pin
                    mt_set_gpio_mode(GPIO_CTP_RST_PIN, 0);
                    mt_set_gpio_dir(GPIO_CTP_RST_PIN, 1);
                    mt_set_gpio_out(GPIO_CTP_RST_PIN, 1);
                    msleep(10);
                }
            }
        }
    }

    have_interrupts = 0;
    queue_delayed_work(esd_wq, &esd_work, delay);
    pr_k("[elan esd] %s: exit.......\n", __FUNCTION__);       /* elan_dlx */
}
#endif

#ifdef TINNO_DEVICE_INFO
static int ektf2k_tpd_get_fw_version( void )
{
    return FW_VERSION;
}

static void ektf2k_tpd_get_fw_vendor_name(char * fw_vendor_name)
{
    sprintf(fw_vendor_name, "%s", tpd_desc);
}
#endif

static int tpd_remove(struct i2c_client *client)
{
    pr_k("mtk-tpd:[elan] TPD removed\n");

#ifdef _DMA_MODE_
    if(gpDMABuf_va)
    {
        dma_free_coherent(NULL, 4096, gpDMABuf_va, gpDMABuf_pa);
        gpDMABuf_va = NULL;
        gpDMABuf_pa = NULL;
    }
#endif

    return 0;
}


static int tpd_suspend(struct i2c_client *client, pm_message_t message)
{
    int retval = TPD_OK;
    static char data = 0x3;
    uint8_t cmd[] = {CMD_W_PKT, 0x50, 0x00, 0x01};

    pr_k("mtk-tpd:[elan] TP enter into sleep mode\n");
    if(work_lock == 1) //updating or doing something else
    {
        pr_k(" [elan]%s: TP work locked \n", __func__);
        return -1;
    }

#if defined(TP_PROXIMITY_SENSOR_NEW)
    MTK_TP_DEBUG("[ektf_ps][%s],PROXIMITY=%d,PROXIMITY_SLEEP=%d\n",__func__,PROXIMITY,PROXIMITY_SLEEP);
    if( PROXIMITY == 1 )//PROXIMITY_STATE
    {
        PROXIMITY_SLEEP = 1;
        MTK_TP_DEBUG("[ektf_ps]mtk-tpd:[elan] TP enter into sleep mode  ps return!\n");
        return ;
    }
#endif //TP_PROXIMITY_SENSOR_NEW

#ifdef ESD_CHECK
    cancel_delayed_work_sync(&esd_work);
#endif

    if ((i2c_master_send(private_ts->client, cmd, sizeof(cmd))) != sizeof(cmd))
    {
        pr_k("mtk-tpd:[elan] %s: i2c_master_send failed\n", __func__);
        return -retval;
    }
    mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);

    return retval;
}


static int tpd_resume(struct i2c_client *client)
{
    int retval = TPD_OK;
    uint8_t cmd[] = {CMD_W_PKT, 0x58, 0x00, 0x01};
    pr_k("mtk-tpd:[elan]tpd_resume TPD wake up,FW_ID: 0x%4.4x\n",FW_ID);

    if(work_lock == 1) //updating or doing something else
    {
        pr_k(" [elan]%s: TP work locked \n", __func__);
        return -1;
    }

#if defined(TP_PROXIMITY_SENSOR_NEW)
    MTK_TP_DEBUG("[ektf_ps][%s],PROXIMITY=%d,PROXIMITY_SLEEP=%d\n",__func__,PROXIMITY,PROXIMITY_SLEEP);
    if( PROXIMITY== 1)//PROXIMITY_SLEEP
    {
        if (PROXIMITY_SLEEP == 1)
        {
            PROXIMITY_SLEEP = 0;
            MTK_TP_DEBUG("[ektf_ps]mtk-tpd:[elan]tpd_resume TPD wake up ps return!\n");
            return ;
        }
    }
#endif //TP_PROXIMITY_SENSOR_NEW

#ifdef ESD_CHECK
    queue_delayed_work(esd_wq, &esd_work, delay);
#endif

#if 1
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
    msleep(10);
    mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );
//    msleep(10);
#else
    if ((i2c_master_send(private_ts->client, cmd, sizeof(cmd))) != sizeof(cmd))
    {
        pr_k("[elan] %s: i2c_master_send failed\n", __func__);
        return -retval;
    }
    msleep(200);
#endif

    mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
    return retval;
}

static int tpd_local_init(void)
{
    printk("[mtk-tpd]: ektf I2C Touchscreen Driver init\n");
    if(i2c_add_driver(&tpd_i2c_driver) != 0)
    {
        printk("[mtk-tpd]: unable to add i2c driver.\n");
        return -1;
    }

    if(tpd_load_status == 0)
    {
        printk("ektf2k add error touch panel driver.\n");
        i2c_del_driver(&tpd_i2c_driver);
        return -1;
    }

#ifdef TPD_HAVE_BUTTON
    tpd_button_setting(TPD_KEY_COUNT, tpd_keys_local, tpd_keys_dim_local);// initialize tpd button data
#endif

#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
    TPD_DO_WARP = 1;
    memcpy(tpd_wb_start, tpd_wb_start_local, TPD_WARP_CNT * 4);
    memcpy(tpd_wb_end, tpd_wb_start_local, TPD_WARP_CNT * 4);
#endif
#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
    memcpy(tpd_calmat, tpd_def_calmat_local, 8*4);
    memcpy(tpd_def_calmat, tpd_def_calmat_local, 8*4);
#endif

    printk("mtk-tpd:end %s, %d\n", __FUNCTION__, __LINE__);
    tpd_type_cap = 1;
    return 0;
}


static ssize_t show_chipinfo(struct device *dev,struct device_attribute *attr, char *buf)
{
	struct i2c_client *client =i2c_client;
//	char strbuf[256];
	if(NULL == client)
	{
		printk("i2c client is null!!\n");
		return 0;
	}
	if(FW_ID == FW_AllInfor[0][0])
	{
		return sprintf(buf,"ID:0x%x VER:0x%x IC:ektf2232_era VENDOR:HRC\n",FW_ID, FW_VERSION);
    }
	else if(FW_ID == FW_AllInfor[1][0])
	{
		return sprintf(buf,"ID:0x%x VER:0x%x IC:ektf2232_era VENDOR:DM\n",FW_ID, FW_VERSION);
	}
	else if(FW_ID == FW_AllInfor[2][0])
	{
		return sprintf(buf,"ID:0x%x VER:0x%x IC:ektf2232_era VENDOR:LC\n",FW_ID, FW_VERSION);
	}
	else if(FW_ID == FW_AllInfor[3][0])
	{
		return sprintf(buf,"ID:0x%x VER:0x%x IC:ektf2232_era VENDOR:HLT\n",FW_ID, FW_VERSION);
	}
	else
	{
		return sprintf(buf,"[unknown] ID:0x%x VER:0x%x\n",FW_ID, FW_VERSION);		
	}

}

static DEVICE_ATTR(chipinfo, 0444, show_chipinfo, NULL);

static const struct device_attribute * const ctp_attributes[] = {
	&dev_attr_chipinfo
};

static struct tpd_driver_t tpd_device_driver =
{
    .tpd_device_name = "ektf2k",
    .tpd_local_init = tpd_local_init,
    .suspend = tpd_suspend,
    .resume = tpd_resume,
#ifdef TPD_HAVE_BUTTON
    .tpd_have_button = 1,
#else
    .tpd_have_button = 0,
#endif
#ifdef TINNO_DEVICE_INFO
    .tpd_get_fw_version = ektf2k_tpd_get_fw_version,
    .tpd_get_fw_vendor_name = ektf2k_tpd_get_fw_vendor_name,
#endif
    .attrs=
		 {
			.attr=ctp_attributes,
			.num=1
		 },
};

static int __init tpd_driver_init(void)
{
    printk("mtk-tpd ektf2k touch panel driver init\n");

    i2c_register_board_info(I2C_NUM, &ektf2k_i2c_tpd, 1);

    if(tpd_driver_add(&tpd_device_driver) < 0)
    {
        printk("[mtk-tpd]: %s driver failed\n", __func__);
    }
    return 0;
}


static void __exit tpd_driver_exit(void)
{
    printk("[mtk-tpd]: %s elan ektf touch panel driver exit\n", __func__);
    tpd_driver_remove(&tpd_device_driver);
}

module_init(tpd_driver_init);
module_exit(tpd_driver_exit);
