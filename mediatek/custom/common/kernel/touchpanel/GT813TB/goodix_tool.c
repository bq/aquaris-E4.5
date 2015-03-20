/*
 * 
 * Copyright (C) 2011 Goodix, Inc.
 * 
 * Author: Scott
 * Date: 2012.01.05
 */


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <asm/uaccess.h>

//#include "tpd_custom_gt827.h"
#include "tpd.h"
#include <cust_eint.h>


#define IC_TYPE_NAME        "GT813"
#define DATA_LENGTH_UINT    512
#define CMD_HEAD_LENGTH     (sizeof(st_cmd_head) - sizeof(u8*))
#define GOODIX_ENTRY_NAME   "goodix_tool"
#define ADDR_MAX_LENGTH     2
#define I2C_DEVICE_ADDRESS_LEN ADDR_MAX_LENGTH
#define MAX_I2C_MAX_TRANSFER_SIZE 8
#define MAX_I2C_TRANSFER_SIZE 6

#define MAX_TRANSACTION_LENGTH  8

#define fail    0
#define success 1

#define false   0
#define true    1

#if 0
#define DEBUG(fmt, arg...) printk("<--GT-DEBUG-->"fmt, ##arg)
#else
#define DEBUG(fmt, arg...)
#endif

#if 1
#define NOTICE(fmt, arg...) printk("<--GT-NOTICE-->"fmt, ##arg)
#else
#define NOTICE(fmt, arg...)
#endif

#if 1
#define WARNING(fmt, arg...) printk("<--GT-WARNING-->"fmt, ##arg)
#else
#define WARNING(fmt, arg...)
#endif

#if 0
#define DEBUG_ARRAY(array, num)   do{\
                                   typeof (array) a = array;\
                                   int i; \
                                   for (i = 0; i < (num); i++)\
                                   {\
                                       printk("%02x   ", (a)[i]);\
                                       if ((i + 1 ) %10 == 0)\
                                       {\
                                           printk("\n");\
                                       }\
                                   }\
                                   printk("\n");\
                                  }while(0)
#else
#define DEBUG_ARRAY(array, num) 
#endif 

#pragma pack(1)
typedef struct{
    u8  wr;         //��д��־λ��0:R  1:W  2:PID 3:
    u8  flag;       //0:����Ҫ��־λ/�ж� 1: ��Ҫ��־λ  2:��Ҫ�ж�
    u8 flag_addr[ADDR_MAX_LENGTH];  //��־λ��ַ
    u8  flag_val;   //��־λֵ
    u8  flag_relation;  //��־λ��ַ����ֵ���־λֵ��ϵ 0:������ 1:��� 2:���� 3:С��
    u16 circle;     //��ѯ����
    u8  times;      //��ѯ����
    u8  retry;      //I2Cͨ�����Դ���
    u16 delay;      //��ǰ��ʱ��д����ʱ
    u16 data_len;   //���ݳ���
    u8  addr_len;   //��ַ����
    u8  addr[ADDR_MAX_LENGTH];    //��ַ
    u8  res[3];     //����
    u8* data;       //����ָ��
}st_cmd_head;
#pragma pack()
st_cmd_head cmd_head;

struct i2c_client *gt_client = NULL;

static struct proc_dir_entry *goodix_proc_entry;

static s32 goodix_tool_write(struct file *filp, const char __user *buff, unsigned long len, void *data);
static s32 goodix_tool_read( char *page, char **start, off_t off, int count, int *eof, void *data );
static s32 (*tool_i2c_read)(u8 *, u16);
static s32 (*tool_i2c_write)(u8 *, u16);

//extern int i2c_read_bytes( struct i2c_client *, u16 , u8 *, int );
//extern int i2c_write_bytes( struct i2c_client *, u16 , u8 *, int );
//extern int i2c_enable_commands( struct i2c_client *, u16);

//extern void mt65xx_eint_unmask(unsigned int line);
//extern void mt65xx_eint_mask(unsigned int line);

s32 DATA_LENGTH = 0;
s8 IC_TYPE[16] = IC_TYPE_NAME;

static int i2c_enable_commands( struct i2c_client *client, u16 addr)
{
	u8 retry;
	u8 txbuf[2] = {0};

	if ( txbuf == NULL )
        return -1;

	txbuf[0] = ( addr >> 8 ) & 0xFF;
	txbuf[1] = addr & 0xFF;

	client->addr = client->addr & I2C_MASK_FLAG;// | I2C_ENEXT_FLAG;

	retry = 0;
    while ( i2c_master_send(client, &txbuf[0], I2C_DEVICE_ADDRESS_LEN ) < 0 )
    {
        retry++;

        if ( retry == 5 )
        {
            client->addr = client->addr & I2C_MASK_FLAG;
            //TPD_DEBUG("I2C read 0x%X length=%d failed\n", addr, I2C_DEVICE_ADDRESS_LEN);
            return -1;
        }
    }

    return 0;
}


static int i2c_write_bytes( struct i2c_client *client, u16 addr, u8 *txbuf, int len )
{
    u8 buffer[MAX_TRANSACTION_LENGTH];
    u16 left = len;
    u16 offset = 0;
    u8 retry = 0;

    struct i2c_msg msg = 
    {
        .addr = client->addr & I2C_MASK_FLAG,
        .flags = 0,
        .buf = buffer
    };


    if ( txbuf == NULL )
        return -1;

    TPD_DEBUG("i2c_write_bytes to device %02X address %04X len %d\n", client->addr, addr, len );

    while ( left > 0 )
    {
        retry = 0;

        //addr = addr + offset;

        buffer[0] = ( ( addr + offset ) >> 8 ) & 0xFF;
        buffer[1] = ( addr + offset ) & 0xFF;		

        if ( left > MAX_I2C_TRANSFER_SIZE )
        {
            memcpy( &buffer[I2C_DEVICE_ADDRESS_LEN], &txbuf[offset], MAX_I2C_TRANSFER_SIZE );
            msg.len = MAX_TRANSACTION_LENGTH;
            left -= MAX_I2C_TRANSFER_SIZE;
            offset += MAX_I2C_TRANSFER_SIZE;
        }
        else
        {
            memcpy( &buffer[I2C_DEVICE_ADDRESS_LEN], &txbuf[offset], left );
            msg.len = left + I2C_DEVICE_ADDRESS_LEN;
            left = 0;
        }

        TPD_DEBUG("byte left %d offset %d\n", left, offset );

        while ( i2c_transfer( client->adapter, &msg, 1 ) != 1 )
        {
            retry++;

            if ( retry == 5 )
            {
                TPD_DEBUG("I2C write 0x%X%X length=%d failed\n", buffer[0], buffer[1], len);
                return -1;
            }
            else
                 TPD_DEBUG("I2C write retry %d addr 0x%X%X\n", retry, buffer[0], buffer[1]);
        }
    }
    return 0;
}


static int i2c_read_bytes( struct i2c_client *client, u16 addr, u8 *rxbuf, int len )
{
    u8 retry;
	u8 addrBuffer[2] = {0};
    u16 left = len;
    u16 offset = 0;

    if ( rxbuf == NULL )
        return -1;

    TPD_DEBUG("i2c_read_bytes to device %02X address %04X len %d\n", client->addr, addr, len );

    while ( left > 0 )
    {
        if ( left > MAX_I2C_MAX_TRANSFER_SIZE )
        {
            //addr = addr + offset;

			addrBuffer[0] = ( ( addr + offset ) >> 8 ) & 0xFF;
			addrBuffer[1] = ( addr + offset ) & 0xFF;

            client->addr = client->addr & I2C_MASK_FLAG;

            retry = 0;
            while ( i2c_master_send(client, &addrBuffer[0], I2C_DEVICE_ADDRESS_LEN ) < 0 )
            {
                retry++;

                if ( retry == 5 )
                {
                    client->addr = client->addr & I2C_MASK_FLAG;
                    TPD_DEBUG("I2C read 0x%X length=%d failed\n", addr + offset, I2C_DEVICE_ADDRESS_LEN);
                    return -1;
                }
            }

            retry = 0;
            while ( i2c_master_recv(client, &rxbuf[offset], MAX_I2C_MAX_TRANSFER_SIZE ) < 0 )
            {
                retry++;

                if ( retry == 5 )
                {
                    client->addr = client->addr & I2C_MASK_FLAG;
                    TPD_DEBUG("I2C read 0x%X length=%d failed\n", addr + offset, MAX_I2C_MAX_TRANSFER_SIZE);
                    return -1;
                }
            }
            
            left -= MAX_I2C_MAX_TRANSFER_SIZE;
            offset += MAX_I2C_MAX_TRANSFER_SIZE;
        }
        else
        {
            //addr = addr + offset;

			addrBuffer[0] = ( ( addr + offset ) >> 8 ) & 0xFF;
			addrBuffer[1] = ( addr + offset ) & 0xFF;

            client->addr = client->addr & I2C_MASK_FLAG;			
            
            retry = 0;
            while ( i2c_master_send(client, &addrBuffer[0], I2C_DEVICE_ADDRESS_LEN ) < 0 )
            {
                retry++;

                if ( retry == 5 )
                {
                    client->addr = client->addr & I2C_MASK_FLAG;
                    TPD_DEBUG("I2C write 0x%X length=%d failed\n", addr + offset, I2C_DEVICE_ADDRESS_LEN);
                    return -1;
                }
            }

            retry = 0;
            while ( i2c_master_recv(client, &rxbuf[offset], left ) < 0 )
            {
                retry++;

                if ( retry == 5 )
                {
                    client->addr = client->addr & I2C_MASK_FLAG;
                    TPD_DEBUG("I2C read 0x%X length=%d failed\n", addr + offset, left);
                    return -1;
                }
            }
            
            offset += left;
            left = 0;
        }
    }
    client->addr = client->addr & I2C_MASK_FLAG;
    
    return 0;
}


static s32 tool_i2c_read_no_extra(u8* buf, u16 len)
{
    if (i2c_read_bytes(gt_client, buf[0] << 8 | buf[1], &buf[2], len))
    {
        return -1;
    }
    return 2;
#if 0    
    s32 ret = -1;
    s32 i = 0;
    struct i2c_msg msgs[2];
    
//    DEBUG("[I2C READ:]");
//    DEBUG_ARRAY(buf, len + cmd_head.addr_len);

    //����д��ַ
    msgs[0].flags = !I2C_M_RD; //д��Ϣ
    msgs[0].addr  = gt_client->addr;
    msgs[0].len   = cmd_head.addr_len;
    msgs[0].buf   = &buf[0];
    
    //��������
    msgs[1].flags = I2C_M_RD;//����Ϣ
    msgs[1].addr  = gt_client->addr;
    msgs[1].len   = len;
    msgs[1].buf   = &buf[ADDR_MAX_LENGTH];

    for (i = 0; i < cmd_head.retry; i++)
    {
        ret=i2c_transfer(gt_client->adapter, msgs, 2);
        if (ret > 0)
        {
            break;
        }
    }
    return ret;
#endif    
}



static s32 tool_i2c_write_no_extra(u8* buf, u16 len)
{
    int ret = 1;
    
    if (len <= ADDR_MAX_LENGTH)
    {
        if (i2c_enable_commands(gt_client, buf[0] << 8 | buf[1]))
        {
            ret = -1;
        }
    }
    else
    {
        if (i2c_write_bytes(gt_client, buf[0] << 8 | buf[1], &buf[2], len - ADDR_MAX_LENGTH))
        {
            ret = -1;
        }
    }

    return ret;
#if 0
    s32 ret = -1;
    s32 i = 0;
    struct i2c_msg msg;

//    DEBUG("[I2C WRITE:]");
//    DEBUG_ARRAY(buf, len);
    //�����豸��ַ
    msg.flags = !I2C_M_RD;//д��Ϣ
    msg.addr  = gt_client->addr;
    msg.len   = len;
    msg.buf   = buf;

    for (i = 0; i < cmd_head.retry; i++)
    {
        ret=i2c_transfer(gt_client->adapter, &msg, 1);
        if (ret > 0)
        {
            break;
        }
    }
    return ret;
#endif    
}

static s32 tool_i2c_read_with_extra(u8* buf, u16 len)
{
    s32 ret = -1;

    i2c_enable_commands(gt_client, 0x0fff);
    ret = tool_i2c_read_no_extra(buf, len);
    i2c_enable_commands(gt_client, 0x8000);

    return ret;
}

static s32 tool_i2c_write_with_extra(u8* buf, u16 len)
{
    s32 ret = -1;

    i2c_enable_commands(gt_client, 0x0fff);
    ret = tool_i2c_write_no_extra(buf, len);
    i2c_enable_commands(gt_client, 0x8000);

    return ret;
}

static void register_i2c_func(void)
{
    if (!strncmp(IC_TYPE,"GT818", 5) || !strncmp(IC_TYPE, "GT816", 5) 
        || !strncmp(IC_TYPE,"GT827", 5) || !strncmp(IC_TYPE,"GT828", 5)
        || !strncmp(IC_TYPE,"GT813", 5))
    {
        tool_i2c_read = tool_i2c_read_with_extra;
        tool_i2c_write = tool_i2c_write_with_extra;
        NOTICE("I2C function: with pre and end cmd!\n");
    }
    else
    {
        tool_i2c_read = tool_i2c_read_no_extra;
        tool_i2c_write = tool_i2c_write_no_extra;
        NOTICE("I2C function: without pre and end cmd!\n");
    }
}

static void unregister_i2c_func(void)
{
    tool_i2c_read = NULL;
    tool_i2c_write = NULL;
    NOTICE("I2C function: unregister i2c transfer function!\n");
}


s32 init_wr_node(struct i2c_client *client)
{
    s32 i;

    gt_client = client;
    memset(&cmd_head, 0, sizeof(cmd_head));
    cmd_head.data = NULL;

    i = 5;
    while ((!cmd_head.data) && i)
    {
        cmd_head.data = kzalloc(i * DATA_LENGTH_UINT, GFP_KERNEL);
        if (NULL != cmd_head.data)
        {
            break;
        }
        i--;
    }
    if (i)
    {
        DATA_LENGTH = i * DATA_LENGTH_UINT + ADDR_MAX_LENGTH;
        NOTICE("Applied memory size:%d.\n", DATA_LENGTH);
    }
    else
    {
        WARNING("Apply for memory failed.\n");
        return fail;
    }

    cmd_head.addr_len = 2;
    cmd_head.retry = 5;

    register_i2c_func();

#if 0 //linux-3.10 procfs API changed
    goodix_proc_entry = create_proc_entry(GOODIX_ENTRY_NAME, 0666, NULL);
    if (goodix_proc_entry == NULL)
    {
        WARNING("Couldn't create proc entry!\n");
        return fail;
    }
    else
    {
        NOTICE("Create proc entry success!\n");
        goodix_proc_entry->write_proc = goodix_tool_write;
        goodix_proc_entry->read_proc = goodix_tool_read;
        //goodix_proc_entry->owner =THIS_MODULE;
    }
#endif
    return success;
}

void uninit_wr_node(void)
{
    kfree(cmd_head.data);
    cmd_head.data = NULL;
    unregister_i2c_func();
    remove_proc_entry(GOODIX_ENTRY_NAME, NULL);
}

static u8 relation(u8 src, u8 dst, u8 rlt)
{
    u8 ret = 0;
    
    switch (rlt)
    {
    case 0:
        ret = (src != dst) ? true : false;
        break;

    case 1:
        ret = (src == dst) ? true : false;
        DEBUG("equal:src:0x%02x   dst:0x%02x   ret:%d\n", src, dst, (s32)ret);
        break;

    case 2:
        ret = (src > dst) ? true : false;
        break;

    case 3:
        ret = (src < dst) ? true : false;
        break;

    case 4:
        ret = (src & dst) ? true : false;
        break;

    case 5:
        ret = (!(src | dst)) ? true : false;
        break;

    default:
        ret = false;
        break;    
    }

    return ret;
}

static u8 comfirm(void)
{
    s32 i = 0;
    u8 buf[32];
    
//    memcpy(&buf[ADDR_MAX_LENGTH - cmd_head.addr_len], &cmd_head.flag_addr, cmd_head.addr_len);
    memcpy(buf, cmd_head.flag_addr, cmd_head.addr_len);
    
    for (i = 0; i < cmd_head.times; i++)
    {
        if (tool_i2c_read(buf, 1) <= 0)
        {
            WARNING("Read flag data failed!\n");
            return fail;
        }
        if (true == relation(buf[ADDR_MAX_LENGTH], cmd_head.flag_val, cmd_head.flag_relation))
        {
            DEBUG("value at flag addr:0x%02x\n", buf[ADDR_MAX_LENGTH]);
            DEBUG("flag value:0x%02x\n", cmd_head.flag_val);
            break;
        }

        DEBUG("<comfirm debug 1>\n");
        msleep(cmd_head.circle);
    }

    DEBUG("<comfirm debug 2>\n");
    if (i >= cmd_head.times)
    {
        WARNING("Didn't get the flag to continue!\n");
        return fail;
    }
    DEBUG("<comfirm debug 3>\n");

    return success;
}

static s32 goodix_tool_write(struct file *filp, const char __user *buff, unsigned long len, void *data)
{
    u64 ret = 0;
    DEBUG_ARRAY((u8*)buff, len);
    
    ret = copy_from_user(&cmd_head, buff, CMD_HEAD_LENGTH);
    
    if (ret)
    {
        WARNING("copy_from_user failed.");
    }
    
    DEBUG("wr  :0x%02x\n", cmd_head.wr);
    DEBUG("flag:0x%02x\n", cmd_head.flag);
    DEBUG("flag addr:0x%02x%02x\n", cmd_head.flag_addr[0], cmd_head.flag_addr[1]);
    DEBUG("flag val:0x%02x\n", cmd_head.flag_val);
    DEBUG("flag rel:0x%02x\n", cmd_head.flag_relation);
    DEBUG("circle  :%d\n", (s32)cmd_head.circle);
    DEBUG("times   :%d\n", (s32)cmd_head.times);
    DEBUG("retry   :%d\n", (s32)cmd_head.retry);
    DEBUG("delay   :%d\n", (s32)cmd_head.delay);
    DEBUG("data len:%d\n", (s32)cmd_head.data_len);
    DEBUG("addr len:%d\n", (s32)cmd_head.addr_len);
    DEBUG("addr:0x%02x%02x\n", cmd_head.addr[0], cmd_head.addr[1]);
    DEBUG("len:%d\n", (s32)len);
    DEBUG("buf[20]:0x%02x\n", buff[CMD_HEAD_LENGTH]);
    
    if (1 == cmd_head.wr)
    {
      //  copy_from_user(&cmd_head.data[cmd_head.addr_len], &buff[CMD_HEAD_LENGTH], cmd_head.data_len);
        ret = copy_from_user(&cmd_head.data[ADDR_MAX_LENGTH], &buff[CMD_HEAD_LENGTH], cmd_head.data_len);
        
        if (ret)
        {
            WARNING("copy_from_user failed.");
        }       
        
        memcpy(&cmd_head.data[ADDR_MAX_LENGTH - cmd_head.addr_len], cmd_head.addr, cmd_head.addr_len);

        DEBUG_ARRAY(cmd_head.data, cmd_head.data_len + cmd_head.addr_len);
        DEBUG_ARRAY((u8*)&buff[CMD_HEAD_LENGTH], cmd_head.data_len);
        
        if (1 == cmd_head.flag)
        {
            if (fail == comfirm())
            {
                WARNING("[WRITE]Comfirm fail!\n");
                return fail;
            }
        }
        else if (2 == cmd_head.flag)
        {
            //Need interrupt!
        }
        if (tool_i2c_write(&cmd_head.data[ADDR_MAX_LENGTH - cmd_head.addr_len],
            cmd_head.data_len + cmd_head.addr_len) <= 0)
        {
            WARNING("[WRITE]Write data failed!\n");
            return fail;
        }

        DEBUG_ARRAY(&cmd_head.data[ADDR_MAX_LENGTH - cmd_head.addr_len],cmd_head.data_len + cmd_head.addr_len);
        if (cmd_head.delay)
        {
            msleep(cmd_head.delay);
        }

        return cmd_head.data_len + CMD_HEAD_LENGTH;
    }
    else if (3 == cmd_head.wr)
    {
        memcpy(IC_TYPE, cmd_head.data, cmd_head.data_len);
        register_i2c_func();        

        return cmd_head.data_len + CMD_HEAD_LENGTH;
    }
    else if (5 == cmd_head.wr)
    {
        //memcpy(IC_TYPE, cmd_head.data, cmd_head.data_len);

        return cmd_head.data_len + CMD_HEAD_LENGTH;
    }
    else if (7 == cmd_head.wr)
    {
     //   mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
    }
    else if (9 == cmd_head.wr)
    {
     //   mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
    }

    return CMD_HEAD_LENGTH;
}

static s32 goodix_tool_read( char *page, char **start, off_t off, int count, int *eof, void *data )
{
    if (2 == cmd_head.wr)
    {
        //memcpy(page, IC_TYPE, cmd_head.data_len);
        memcpy(page, IC_TYPE, sizeof(IC_TYPE_NAME));
        page[sizeof(IC_TYPE_NAME)] = 0;

        DEBUG("Return ic type:%s len:%d\n", page, (s32)cmd_head.data_len);
        return cmd_head.data_len;
        //return sizeof(IC_TYPE_NAME);
    }
    else if (cmd_head.wr % 2)
    {
        return fail;
    }
    else if (!cmd_head.wr)
    {
        u16 len = 0;
        s16 data_len = 0;
        u16 loc = 0;
        
        if (1 == cmd_head.flag)
        {
            if (fail == comfirm())
            {
                WARNING("[READ]Comfirm fail!\n");
                return fail;
            }
        }
        else if (2 == cmd_head.flag)
        {
            //Need interrupt!
        }
        DEBUG("[CMD HEAD DATA] memcpy\n");
        memcpy(cmd_head.data, cmd_head.addr, cmd_head.addr_len);

        DEBUG("[CMD HEAD DATA] ADDR:0x%02x%02x\n", cmd_head.data[0], cmd_head.data[1]);
        DEBUG("[CMD HEAD ADDR] ADDR:0x%02x%02x\n", cmd_head.addr[0], cmd_head.addr[1]);
        
        if (cmd_head.delay)
        {
            msleep(cmd_head.delay);
        }

        data_len = cmd_head.data_len;
        while(data_len > 0)
        {
            if (data_len > DATA_LENGTH)
            {
                len = DATA_LENGTH;
            }
            else
            {
                len = data_len;
            }
            data_len -= DATA_LENGTH;

            if (tool_i2c_read(cmd_head.data, len) <= 0)
            {
                WARNING("[READ]Read data failed!\n");
                return fail;
            }

            memcpy(&page[loc], &cmd_head.data[ADDR_MAX_LENGTH], len);
            loc += len;

            //DEBUG_ARRAY(&cmd_head.data[ADDR_MAX_LENGTH], len);
            DEBUG_ARRAY(page, len);
        }
    }

    return cmd_head.data_len;
}

