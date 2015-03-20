#ifndef BUILD_LK
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>

#include <cust_acc.h>
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include <linux/hwmsen_helper.h>
#include <linux/xlog.h>


#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>

//#include "sn65dsi83_i2c.h"
//#include "cust_charging.h"
//#include <mach/charging.h>


/**********************************************************
  *
  *   [I2C Slave Setting] 
  *
  *********************************************************/
#define sn65dsi83_SLAVE_ADDR_WRITE   0x58 //0xD4
#define sn65dsi83_SLAVE_ADDR_Read    0x59 //0xD5
#define sn65dsi83_SLAVE_ADDR    0x58


static struct i2c_client *new_client = NULL;
static const struct i2c_device_id sn65dsi83_i2c_id[] = {{"sn65dsi83",0},{}};   
//kal_bool chargin_hw_init_done = KAL_FALSE; 
static int sn65dsi83_driver_probe(struct i2c_client *client, const struct i2c_device_id *id);

static struct i2c_driver sn65dsi83_driver = {
    .driver = {
        .name    = "sn65dsi83",
    },
    .probe       = sn65dsi83_driver_probe,
    .id_table    = sn65dsi83_i2c_id,
};

/**********************************************************
  *
  *   [Global Variable] 
  *
  *********************************************************/
//kal_uint8 sn65dsi83_reg[sn65dsi83_REG_NUM] = {0};

static DEFINE_MUTEX(sn65dsi83_i2c_access);
/**********************************************************
  *
  *   [I2C Function For Read/Write fan5405] 
  *
  *********************************************************/
int sn65dsi83_read_byte(kal_uint8 cmd, kal_uint8 *returnData)
{
    char     cmd_buf[1]={0x00};
    char     readData = 0;
    int      ret=0;

    mutex_lock(&sn65dsi83_i2c_access);
    
    //new_client->addr = ((new_client->addr) & I2C_MASK_FLAG) | I2C_WR_FLAG;    
    new_client->ext_flag=((new_client->ext_flag ) & I2C_MASK_FLAG ) | I2C_WR_FLAG | I2C_DIRECTION_FLAG;

    cmd_buf[0] = cmd;
    ret = i2c_master_send(new_client, &cmd_buf[0], (1<<8 | 1));
    if (ret < 0) 
    {    
        //new_client->addr = new_client->addr & I2C_MASK_FLAG;
        new_client->ext_flag=0;

        mutex_unlock(&sn65dsi83_i2c_access);
        return 0;
    }
    
    readData = cmd_buf[0];
    *returnData = readData;

    // new_client->addr = new_client->addr & I2C_MASK_FLAG;
    new_client->ext_flag=0;
    
    mutex_unlock(&sn65dsi83_i2c_access);    
    return 1;
}

int sn65dsi83_write_byte(kal_uint8 cmd, kal_uint8 writeData)
{
    char    write_data[2] = {0};
    int     ret=0;
    
    mutex_lock(&sn65dsi83_i2c_access);
    
    write_data[0] = cmd;
    write_data[1] = writeData;
    
    new_client->ext_flag=((new_client->ext_flag ) & I2C_MASK_FLAG ) | I2C_DIRECTION_FLAG;
    
    ret = i2c_master_send(new_client, write_data, 2);
    if (ret < 0) 
    {
       
        new_client->ext_flag=0;
        mutex_unlock(&sn65dsi83_i2c_access);
        return 0;
    }
    
    new_client->ext_flag=0;
    mutex_unlock(&sn65dsi83_i2c_access);
    return 1;
}


static int sn65dsi83_driver_probe(struct i2c_client *client, const struct i2c_device_id *id) 
{             
    int err=0; 

    //battery_xlog_printk(BAT_LOG_CRTI,"[sn65dsi83_driver_probe] \n");
	printk("[sn65dsi83_driver_probe] \n");	
    if (!(new_client = kmalloc(sizeof(struct i2c_client), GFP_KERNEL))) {
        err = -ENOMEM;
        goto exit;
    }    
    memset(new_client, 0, sizeof(struct i2c_client));

    new_client = client;    

    //---------------------
  //  sn65dsi83_hw_init();
    //fan5405_dump_register();
    //chargin_hw_init_done = KAL_TRUE;
	
    return 0;                                                                                       

exit:
    return err;

}

#define SN65DSI83_BUSNUM 0

static struct i2c_board_info __initdata i2c_sn65dsi83 = { I2C_BOARD_INFO("sn65dsi83", /*sn65dsi83_SLAVE_ADDR_WRITE*/(sn65dsi83_SLAVE_ADDR_WRITE>>1))};

static int __init sn65dsi83_init(void)
{    
   // int ret=0;  //fixed for build warning
    
    //battery_xlog_printk(BAT_LOG_CRTI,"[sn65dsi83_init] init start\n");
    printk("[sn65dsi83_init] init start\n");
    
    i2c_register_board_info(SN65DSI83_BUSNUM, &i2c_sn65dsi83, 1);

    if(i2c_add_driver(&sn65dsi83_driver)!=0)
    {
        //battery_xlog_printk(BAT_LOG_CRTI,"[sn65dsi83_init] failed to register sn65dsi83 i2c driver.\n");
        printk("[sn65dsi83_init] failed to register sn65dsi83 i2c driver.\n");
    }
    else
    {
        //battery_xlog_printk(BAT_LOG_CRTI,"[sn65dsi83_init] Success to register sn65dsi83 i2c driver.\n");
        printk("[sn65dsi83_init] Success to register sn65dsi83 i2c driver.\n");
    }

    return 0;        
}

static void __exit sn65dsi83_exit(void)
{
    i2c_del_driver(&sn65dsi83_driver);
}

module_init(sn65dsi83_init);
module_exit(sn65dsi83_exit);
   
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C sn65dsi83 Driver");
MODULE_AUTHOR("James Lo<james.lo@mediatek.com>");
#endif
