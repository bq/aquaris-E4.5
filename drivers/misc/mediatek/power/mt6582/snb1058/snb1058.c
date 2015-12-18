#include <mach/charging.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <mach/mt_gpio.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/hrtimer.h>
#include <linux/proc_fs.h>      // might need to get fuel gauge info
#include <linux/power_supply.h>     // might need to get fuel gauge info
#include <linux/xlog.h>
#include <linux/kobject.h>

#include <cust_gpio_usage.h>
#include <cust_gpio_boot.h>

#include "snb1058.h"

#define TESTCODE 0

//void charger_enable();

enum
{
    SNB1058_UNINITIALIZE_CABLE     = -1,
    SNB1058_NORMAL_MODE            = 0,
    SNB1058_FACTORY_MODE           = 1
} SNB1058_PTM_MODE;

static int snb1058_is_PTM = SNB1058_NORMAL_MODE;

static int snb1058_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int snb1058_i2c_remove(struct i2c_client *client);

static struct i2c_client *snb1058_i2c_client = NULL;
static struct platform_driver snb1058_driver;
static struct snb1058_i2c_data *i2c_data = NULL;
static const struct i2c_device_id snb1058_i2c_id[] = {{SNB1058_DEV_NAME,0},{}};
static struct i2c_board_info __initdata i2c_snb1058 = {I2C_BOARD_INFO(SNB1058_DEV_NAME, 0x44)};

kal_bool chargin_hw_init_done = KAL_FALSE;

struct snb1058_i2c_data {
    struct i2c_client *client;
    int blue;
    int green;
    int red;
    int keyleds;
#if TESTCODE
    int main_cam_dvdd;
    int cam_avdd;
    int cam_iovdd;
    int mtk_sensor;
    int chg_current;
    int chg_stop;
#endif
};

static void SNB1058_CheckDevID(struct i2c_client *client)
{
    int tempvalue;

    if((!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))) {
        printk("[SNB1058] I2C check ERROR!!\n");
    }

    tempvalue = 0;
    tempvalue = i2c_smbus_read_word_data(client,0x00);
    if(tempvalue < 0){
        printk("[SNB1058] Check ID error!!\n");
    }
    else {
        printk("[SNB1058]Device ID = [%02x]\n",tempvalue);
    }
}

static void SNB1058_SetGpio(void)
{
    // i2c0 pin setting
    mt_set_gpio_mode(GPIO_COMMON_I2C_SCL, GPIO_COMMON_I2C_SCL_M_SCL);
    mt_set_gpio_mode(GPIO_COMMON_I2C_SDA, GPIO_COMMON_I2C_SDA_M_SDA);

    // snb1058_int pin setting
    mt_set_gpio_mode(GPIO_MINIABB_INT, GPIO_MINIABB_INT_M_EINT);

    printk("[SNB1058]GPIO_SCL[137] Mode:%d\nGPIO_SDA[138] Mode:%d\n"
            ,mt_get_gpio_mode(137),mt_get_gpio_mode(138));

}

/////////////////////////////////////////////////////////////
/*  CAM LDO control API                                    */
/////////////////////////////////////////////////////////////
void main_cam_dvdd_power(int on_off)
{
    struct i2c_client *client = snb1058_i2c_client;
    u8 data;

    if(client==NULL){
    printk("main_cam_dvdd_power client NULL\n");
        return ;
    }

    if(on_off) {
        // enable
        data = i2c_smbus_read_word_data(client,EN_CTRL);
        data |= LDO1_EN;
        i2c_smbus_write_byte_data(client,EN_CTRL, data);
        // setting
        data = i2c_smbus_read_word_data(client,LDO_VSET1);
        data |= LDO1_VSET;
        i2c_smbus_write_byte_data(client,LDO_VSET1, data);

    } else {

        data = i2c_smbus_read_word_data(client,EN_CTRL);
        data &= ~LDO1_EN;
        i2c_smbus_write_byte_data(client,EN_CTRL,data);

    }
}
EXPORT_SYMBOL(main_cam_dvdd_power);

void cam_avdd_power(int on_off)
{
    struct i2c_client *client = snb1058_i2c_client;
    u8 data;

    if(client==NULL){
    printk("cam_avdd_power client NULL\n");
        return ;
    }

    if(on_off) {
        // enable
        data = i2c_smbus_read_word_data(client,EN_CTRL);
        data |= LDO2_EN;
        i2c_smbus_write_byte_data(client,EN_CTRL, data);
        // setting
        data = i2c_smbus_read_word_data(client,LDO_VSET1);
        data |= LDO2_VSET;
        i2c_smbus_write_byte_data(client,LDO_VSET1, data);

    } else {

        data = i2c_smbus_read_word_data(client,EN_CTRL);
        data &= ~LDO2_EN;
        i2c_smbus_write_byte_data(client,EN_CTRL,data);

    }
}
EXPORT_SYMBOL(cam_avdd_power);

void cam_iovdd_power(int on_off)
{
    struct i2c_client *client = snb1058_i2c_client;
    u8 data;

    if(client==NULL){
    printk("cam_iovdd_power client NULL\n");
        return ;
    }

    if(on_off) {
        // enable
        data = i2c_smbus_read_word_data(client,EN_CTRL);
        data |= LDO3_EN;
        i2c_smbus_write_byte_data(client,EN_CTRL, data);
        // setting
        data = i2c_smbus_read_word_data(client,LDO_VSET2);
        data |= LDO3_VSET;
        i2c_smbus_write_byte_data(client,LDO_VSET2, data);

    } else {

        data = i2c_smbus_read_word_data(client,EN_CTRL);
        data &= ~LDO3_EN;
        i2c_smbus_write_byte_data(client,EN_CTRL,data);

    }
}
EXPORT_SYMBOL(cam_iovdd_power);

void mtk_sensor_power(int on_off)
{
    struct i2c_client *client = snb1058_i2c_client;
    u8 data;

    if(client==NULL){
    printk("mtk_sensor_power client NULL\n");
        return ;
    }

    if(on_off) {
        // enable
        data = i2c_smbus_read_word_data(client,EN_CTRL);
        data |= LDO4_EN;
        i2c_smbus_write_byte_data(client,EN_CTRL, data);
        // setting
        data = i2c_smbus_read_word_data(client,LDO_VSET2);
        data |= LDO4_VSET;
        i2c_smbus_write_byte_data(client,LDO_VSET2, data);

    } else {

        data = i2c_smbus_read_word_data(client,EN_CTRL);
        data &= ~LDO4_EN;
        i2c_smbus_write_byte_data(client,EN_CTRL,data);

    }
}
EXPORT_SYMBOL(mtk_sensor_power);


/////////////////////////////////////////////////////////////
/*  RGB LED backlight API                                  */
/////////////////////////////////////////////////////////////
void Blue_LED_control(int on_off)
{
    struct i2c_client *client = snb1058_i2c_client;
    u8 data;

    if(on_off) {

        data = i2c_smbus_read_word_data(client,EN_CTRL);
        data |= LED1_EN;
        i2c_smbus_write_byte_data(client,EN_CTRL, data);

        data = i2c_smbus_read_word_data(client,LED_SET);
        data = LED_DIM;
        i2c_smbus_write_byte_data(client,LED_SET, data);

    } else {

        data = i2c_smbus_read_word_data(client,EN_CTRL);
        data &= ~LED1_EN;
        i2c_smbus_write_byte_data(client,EN_CTRL,data);

    }
}
EXPORT_SYMBOL(Blue_LED_control);

void Green_LED_control(int on_off)
{
    struct i2c_client *client = snb1058_i2c_client;
    u8 data;

    if(on_off) {

        data = i2c_smbus_read_word_data(client,EN_CTRL);
        data |= LED2_EN;
        i2c_smbus_write_byte_data(client,EN_CTRL, data);

        data = i2c_smbus_read_word_data(client,LED_SET);
        data = LED_DIM;
        i2c_smbus_write_byte_data(client,LED_SET, data);

    } else {

        data = i2c_smbus_read_word_data(client,EN_CTRL);
        data &= ~LED2_EN;
        i2c_smbus_write_byte_data(client,EN_CTRL,data);
    }
}
EXPORT_SYMBOL(Green_LED_control);

void Red_LED_control(int on_off)
{
    struct i2c_client *client = snb1058_i2c_client;
    u8 data;

    if(on_off) {

        data = i2c_smbus_read_word_data(client,EN_CTRL);
        data |= LED3_EN;
        i2c_smbus_write_byte_data(client,EN_CTRL, data);

        data = i2c_smbus_read_word_data(client,LED_SET);
        data = LED_DIM;
        i2c_smbus_write_byte_data(client,LED_SET, data);

    } else {

        data = i2c_smbus_read_word_data(client,EN_CTRL);
        data &= ~LED3_EN;
        i2c_smbus_write_byte_data(client,EN_CTRL,data);
    }
}
EXPORT_SYMBOL(Red_LED_control);

void Button_LED_control(int on_off)
{
    struct i2c_client *client = snb1058_i2c_client;
    u8 data;

    if(on_off) {

        data = i2c_smbus_read_word_data(client,EN_CTRL);
        data |= LED4_EN;
        i2c_smbus_write_byte_data(client,EN_CTRL, data);

        data = i2c_smbus_read_word_data(client,LED_SET);
        data = LED_DIM;
        i2c_smbus_write_byte_data(client,LED_SET, data);

    } else {

        data = i2c_smbus_read_word_data(client,EN_CTRL);
        data &= ~LED4_EN;
        i2c_smbus_write_byte_data(client,EN_CTRL,data);
    }
}
EXPORT_SYMBOL(Button_LED_control);


/////////////////////////////////////////////////////////////
/*  charging API                                           */
/////////////////////////////////////////////////////////////
void check_snb1058_status(void){

	struct i2c_client *client = snb1058_i2c_client;
	int data;

	data = i2c_smbus_read_word_data(client,0x08);
	printk("[SNB1058] %s : 0x08(%d) \n",__func__, data);
	data = i2c_smbus_read_word_data(client,0x09);
	printk("[SNB1058] %s : 0x09(%d) \n",__func__, data);	

}

int check_EOC_status(void)
{
    struct i2c_client *client = snb1058_i2c_client;
    int data;

    // EOC check
    data = i2c_smbus_read_word_data(client,0x06);

    printk("[SNB1058] %s : %d \n",__func__, data);

    if(data == 48) {
        return TRUE;
    }
    else {
        return FALSE;
    }

}

void charger_enable(void)
{
    struct i2c_client *client = snb1058_i2c_client;
    u8 data;

#if 1
    if (snb1058_is_PTM == SNB1058_NORMAL_MODE)
    {
       	//charging block off
        i2c_smbus_write_byte_data(client,CHG_CTRL1, 0x50);
        data = 0xC0; //|= CHGEN|EXPDETEN;
        i2c_smbus_write_byte_data(client,CHG_CTRL1, data);
    }
#else
    //charger enable

    if(snb1058_is_PTM)
    {
        data = 0xE0; //|= CHGEN|EXPDETEN|PTM;// factory mode setting
    } else {
        data = 0xC0; //|= CHGEN|EXPDETEN;
    }

    i2c_smbus_write_byte_data(client,CHG_CTRL1, data);
#endif

}
EXPORT_SYMBOL(charger_enable);

int is_charging_enable(void)
{
    struct i2c_client *client = snb1058_i2c_client;
    u8 data;
    
    // CHGEN check
    data = i2c_smbus_read_word_data(client,CHG_CTRL1);
    
    printk("[SNB1058] %s : %x \n",__func__, data);
    
    if((data & CHGEN)== CHGEN) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

#if 1
void set_charger_start_mode(CHG_TYPE chg_type)
{
    struct i2c_client *client = snb1058_i2c_client;
    u8 data;

    if (snb1058_is_PTM == SNB1058_NORMAL_MODE)
    {
        switch(chg_type) {
            case CHG_TA :
                data = CHG_CURR_800mA |CHG_VSET | EOC_LVL_20PER;
                break;
            case CHG_600 :
                data = CHG_CURR_600mA | CHG_VSET| EOC_LVL_10PER;
                break;
            case CHG_500 :
                data = CHG_CURR_500mA |CHG_VSET | EOC_LVL_10PER;
                break;
            case CHG_USB :
                data = CHG_CURR_400mA |CHG_VSET | EOC_LVL_33PER;
                break;
            case CHG_100 :
                data = CHG_CURR_100mA |CHG_VSET | EOC_LVL_10PER;
                break;
            case CHG_90 :
                data = CHG_CURR_90mA |CHG_VSET | EOC_LVL_10PER;
                break;
            default :
                data = CHG_CURR_400mA |CHG_VSET | EOC_LVL_33PER;
                break;
        }

        i2c_smbus_write_byte_data(client,CHG_CTRL2, data);

        charger_enable();
    }
    printk("[SNB1058] Charging Current %dmA\n",chg_type);

}
#else
void set_charger_start_mode(int value)
{
    struct i2c_client *client = snb1058_i2c_client;
    u8 data;

    if (snb1058_is_PTM == SNB1058_NORMAL_MODE)
    {
        if(value == CHG_TA) {
            data = 0x63|CHG_VSET; //CHG_CURR_700mA|EOC_LVL_20PER;
        }
        else {
            data = 0x25|CHG_VSET; //CHG_CURR_400mA|EOC_LVL_33PER;
        }
        i2c_smbus_write_byte_data(client,CHG_CTRL2, data);

        charger_enable();
    }
    printk("[SNB1058] Charging Current %dmA\n",value);
}
#endif
EXPORT_SYMBOL(set_charger_start_mode);

void set_charger_factory_mode(void)
{
    //struct i2c_client *client = snb1058_i2c_client;
    u8 data = 0;

#if 0
    data = 0x91|CHG_VSET; //CHG_CURR_1000mA|EOC_LVL_10PER;
    i2c_smbus_write_byte_data(client,CHG_CTRL2, data);
    data = i2c_smbus_read_word_data(client,CHG_CTRL2);

    snb1058_is_PTM = 1;

    charger_enable();
#endif

    printk("[SNB1058] FACTORY charging :  0x%02x \n",data);
}
EXPORT_SYMBOL(set_charger_factory_mode);


void set_charger_stop_mode(void)
{
    struct i2c_client *client = snb1058_i2c_client;
    //u8 data;

    if (snb1058_is_PTM == SNB1058_NORMAL_MODE)
    {
        i2c_smbus_write_byte_data(client,CHG_CTRL1, 0x40);
    }
    
    printk("[SNB1058]charging off!\n");

}
EXPORT_SYMBOL(set_charger_stop_mode);

/******************************************************************************/

#ifdef LED_FUNC
static ssize_t show_ledblue_value(struct device_driver *ddri, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%d\n", i2c_data->blue);
}

static ssize_t store_ledblue_value(struct device_driver *ddri, char *buf, size_t count)
{
    int onoff;

    sscanf(buf, "%d", &onoff);

    if(onoff) {
        Blue_LED_control(ON);
    } else {
        Blue_LED_control(OFF);
    }
    i2c_data->blue = onoff;

    return count;
}

static ssize_t show_ledgreen_value(struct device_driver *ddri, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%d\n", i2c_data->green);
}

static ssize_t store_ledgreen_value(struct device_driver *ddri, char *buf, size_t count)
{
    int onoff;

    sscanf(buf, "%d", &onoff);

    if(onoff) {
        Green_LED_control(ON);
    } else {
        Green_LED_control(OFF);
    }
    i2c_data->green = onoff;

    return count;
}

static ssize_t show_ledred_value(struct device_driver *ddri, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%d\n", i2c_data->red);
}

static ssize_t store_ledred_value(struct device_driver *ddri, char *buf, size_t count)
{
    int onoff;

    sscanf(buf, "%d", &onoff);

    if(onoff) {
        Red_LED_control(ON);
    } else {
        Red_LED_control(OFF);
    }
    i2c_data->red = onoff;

    return count;
}

static ssize_t show_keyleds_value(struct device_driver *ddri, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%d\n", i2c_data->keyleds);
}

static ssize_t store_keyleds_value(struct device_driver *ddri, char *buf, size_t count)
{
    int onoff;

    sscanf(buf, "%d", &onoff);

    if(onoff) {
        Button_LED_control(ON);
    } else {
        Button_LED_control(OFF);
    }
    i2c_data->keyleds = onoff;

    return count;
}

#if TESTCODE
static ssize_t show_main_cam_dvdd_value(struct device_driver *ddri, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%d\n", i2c_data->main_cam_dvdd);
}

static ssize_t store_main_cam_dvdd_value(struct device_driver *ddri, char *buf, size_t count)
{
    int onoff;

    sscanf(buf, "%d", &onoff);

    if(onoff) {
        main_cam_dvdd_power(ON);
        printk("main_cam_dvdd power on!\n");
    } else {
        main_cam_dvdd_power(OFF);
        printk("main_cam_dvdd power off!\n");
    }
    i2c_data->cam_avdd = onoff;

    return count;
}

static ssize_t show_cam_avdd_value(struct device_driver *ddri, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%d\n", i2c_data->cam_avdd);
}

static ssize_t store_cam_avdd_value(struct device_driver *ddri, char *buf, size_t count)
{
    int onoff;

    sscanf(buf, "%d", &onoff);

    if(onoff) {
        cam_avdd_power(ON);
        printk("cam_avdd power on!\n");
    } else {
        cam_avdd_power(OFF);
        printk("cam_avdd power off!\n");
    }
    i2c_data->cam_avdd = onoff;

    return count;
}


static ssize_t show_cam_iovdd_value(struct device_driver *ddri, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%d\n", i2c_data->cam_iovdd);
}

static ssize_t store_cam_iovdd_value(struct device_driver *ddri, char *buf, size_t count)
{
    int onoff;

    sscanf(buf, "%d", &onoff);

    if(onoff) {
        cam_iovdd_power(ON);
        printk("cam_iovdd power on!\n");
    } else {
        cam_iovdd_power(OFF);
        printk("cam_iovdd power off!\n");
    }
    i2c_data->cam_iovdd = onoff;

    return count;
}


static ssize_t show_mtk_sensor_value(struct device_driver *ddri, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%d\n", i2c_data->mtk_sensor);
}

static ssize_t store_mtk_sensor_value(struct device_driver *ddri, char *buf, size_t count)
{
    int onoff;

    sscanf(buf, "%d", &onoff);

    if(onoff) {
        mtk_sensor_power(ON);
        printk("mtk_sensor power on!\n");
    } else {
        mtk_sensor_power(OFF);
        printk("mtk_sensor power off!\n");
    }
    i2c_data->mtk_sensor = onoff;

    return count;
}

static ssize_t show_chg_start_value(struct device_driver *ddri, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%d\n", i2c_data->chg_current);
}

static ssize_t store_chg_start_value(struct device_driver *ddri, char *buf, size_t count)
{
    int chg_curr;

    sscanf(buf, "%d", &chg_curr);

    if(chg_curr == CHG_USB ) {
        set_charger_start_mode(CHG_USB);
    } else {
        set_charger_start_mode(CHG_TA);
    }
    i2c_data->chg_current = chg_curr;

    return count;
}

static ssize_t show_chg_stop_value(struct device_driver *ddri, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%d\n", i2c_data->chg_stop);
}


static ssize_t store_chg_stop_value(struct device_driver *ddri, char *buf, size_t count)
{
    int chg_stop;

    sscanf(buf, "%d", &chg_stop);

    if(chg_stop) {
        set_charger_stop_mode();
    }
    i2c_data->chg_stop = chg_stop;

    return count;
}

#endif


/******************************************************************************/
static DRIVER_ATTR(ledblue,    S_IWUSR | S_IRUGO | S_IWGRP, show_ledblue_value, store_ledblue_value);
static DRIVER_ATTR(ledgreen,    S_IWUSR | S_IRUGO | S_IWGRP, show_ledgreen_value, store_ledgreen_value);
static DRIVER_ATTR(ledred,    S_IWUSR | S_IRUGO | S_IWGRP, show_ledred_value, store_ledred_value);
static DRIVER_ATTR(keyleds,    S_IWUSR | S_IRUGO | S_IWGRP, show_keyleds_value, store_keyleds_value);
#if TESTCODE
static DRIVER_ATTR(main_cam_dvdd,    S_IWUSR | S_IRUGO | S_IWGRP, show_main_cam_dvdd_value, store_main_cam_dvdd_value);
static DRIVER_ATTR(cam_avdd,    S_IWUSR | S_IRUGO | S_IWGRP, show_cam_avdd_value, store_cam_avdd_value);
static DRIVER_ATTR(cam_iovdd,    S_IWUSR | S_IRUGO | S_IWGRP, show_cam_iovdd_value, store_cam_iovdd_value);
static DRIVER_ATTR(mtk_sensor,    S_IWUSR | S_IRUGO | S_IWGRP, show_mtk_sensor_value, store_mtk_sensor_value);
static DRIVER_ATTR(chg_start,    S_IWUSR | S_IRUGO | S_IWGRP, show_chg_start_value, store_chg_start_value);
static DRIVER_ATTR(chg_stop,    S_IWUSR | S_IRUGO | S_IWGRP, show_chg_stop_value, store_chg_stop_value);
#endif

static struct driver_attribute *snb1058_attr_list[] = {
    &driver_attr_ledblue,   /* blue led control */
    &driver_attr_ledgreen,   /* blue led control */
    &driver_attr_ledred,   /* blue led control */
    &driver_attr_keyleds,   /* button led control */
#if TESTCODE
    &driver_attr_main_cam_dvdd,   /* +1V2_MAIN_CAM_DVDD */
    &driver_attr_cam_avdd,   /* +2V8_CAM_AVDD */
    &driver_attr_cam_iovdd,   /* +1V8_CAM_IOVDD */
    &driver_attr_mtk_sensor,   /* +2V8_MTK_SENSOR */
    &driver_attr_chg_start,   /* start charger */
    &driver_attr_chg_stop,   /* stop charger */
#endif
};

/***********************************************************************************/
static int snb1058_create_attr(struct device_driver *driver)
{
    int idx, err = 0;
    int num = (int)(sizeof(snb1058_attr_list)/sizeof(snb1058_attr_list[0]));
    if (driver == NULL)
    {
        return -EINVAL;
    }

    for(idx = 0; idx < num; idx++)
    {
        if((err = driver_create_file(driver, snb1058_attr_list[idx])))
        {
            printk("driver_create_file (%s) = %d\n", snb1058_attr_list[idx]->attr.name, err);
            break;
        }
    }
    return err;
}

/***********************************************************************************/
static int snb1058_delete_attr(struct device_driver *driver)
{
    int idx ,err = 0;
    int num = (int)(sizeof(snb1058_attr_list)/sizeof(snb1058_attr_list[0]));

    if(driver == NULL)
    {
        return -EINVAL;
    }


    for(idx = 0; idx < num; idx++)
    {
        driver_remove_file(driver, snb1058_attr_list[idx]);
    }


    return err;
}
#endif /* LED_FUNC */

/***********************************************************************************/
static int snb1058_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id) {

    struct i2c_client *new_client;
    struct snb1058_i2c_data *obj;

    int err = 0;

    printk("[SNB1058] :: snb1058 i2c probe \n");
    
    if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
    {
        err = -ENOMEM;
        goto exit;
    }
    SNB1058_SetGpio();

    memset(obj, 0, sizeof(struct snb1058_i2c_data));

    i2c_data = obj;
    obj->client = client;
    new_client = obj->client;
    i2c_set_clientdata(new_client,obj);

    snb1058_i2c_client = client;

    //printk("snb1058_i2c_client number = 0x%x.\n", snb1058_i2c_client);

    SNB1058_CheckDevID(client);

#ifdef LED_FUNC
    if((err = snb1058_create_attr(&snb1058_driver.driver)))
    {
        printk("SNB1058 create attribute err = %d\n", err);
    }
#endif /* LED_FUNC */

	snb1058_is_PTM = SNB1058_NORMAL_MODE;

	#if 0
    if (Is_Not_FactoryCable_PowerOn() == 0)
    {
        snb1058_is_PTM = SNB1058_FACTORY_MODE;
    }
    else
    {
        snb1058_is_PTM = SNB1058_NORMAL_MODE;
    }
	#endif

    chargin_hw_init_done = KAL_TRUE;
    
    return 0;

exit:
    snb1058_i2c_client = NULL;
    return err;

}

static int snb1058_i2c_remove(struct i2c_client *client)
{
    int err=0;

#ifdef LED_FUNC
    if((err = snb1058_delete_attr(&snb1058_driver.driver)))
    {
        printk("SNB1058 delete attribute err = %d\n", err);
    }
#endif /* LED_FUNC */

    return err;
}


static int snb1058_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
    printk("[SNB1058] ::  snb1058_i2c_detect\n");
    strcpy(info->type, SNB1058_DEV_NAME);
    return 0;
}


struct i2c_driver snb1058_i2c_driver = {
    .probe = snb1058_i2c_probe,
    .remove = snb1058_i2c_remove,
    .detect = snb1058_i2c_detect,
    .driver.name = SNB1058_DEV_NAME,
    .id_table = snb1058_i2c_id,
};



static int snb1058_probe(struct platform_device *dev)
{

    printk("[SNB1058] :: charging IC Initialization is done\n");

#if 0
    if(i2c_add_driver(&snb1058_i2c_driver))
    {
        printk("[SNB1058] :: add snb1058 i2c driver error !\n");
        return -1;
    }
#endif	

    return 0;
}

static int snb1058_remove(struct platform_device *dev)
{
    //
    i2c_del_driver(&snb1058_i2c_driver);
    return 0;
}
/*
static int snb1058_suspend(struct platform_device *dev)
{
    return 0;
}

static int snb1058_resume(struct platform_device *dev)
{
    return 0;
}
*/


static struct platform_driver snb1058_driver = {
    .probe = snb1058_probe,
    .remove = snb1058_remove,
//    .suspend = snb1058_suspend,
//    .resume = snb1058_resume,
    .driver = {
        .name = "charger_ic",
        .owner = THIS_MODULE,
    },
};

static int __init snb1058_driver_init(void) {
    int ret ;

    printk("SNB1058 driver init!!\n");

    ret = i2c_register_board_info(1, &i2c_snb1058, 1);
    if(ret)
    {
        printk("failed to i2c register driver. (%d) \n",ret);
        return ret;
    } else {
        printk("success to i2c register driver. (%d) \n",ret);
    }
    ret = platform_driver_register(&snb1058_driver);
    if(ret)
    {
        printk("failed to register driver. (%d) \n",ret);
        return ret;
    } else {
        printk("success to register driver. (%d) \n",ret);
    }
   if(i2c_add_driver(&snb1058_i2c_driver))
    {
        printk("[SNB1058] :: add snb1058 i2c driver error !\n");
        return -1;
    }

    /*In order to prevent sequence that SNB1058 doesn't be
      initialized and sensor try to start power on sequence for initialization.*/
	  
	#if 0
    CAMERA_HW_i2C_init();
	#endif
    return ret;
}

static void __exit snb1058_driver_exit(void) {
    printk("SNB1058 driver exit!!\n");
    platform_driver_unregister(&snb1058_driver);
}

module_init(snb1058_driver_init);
module_exit(snb1058_driver_exit);

MODULE_AUTHOR("LG Electronics");
MODULE_DESCRIPTION("SNB1058 Driver");
MODULE_LICENSE("GPL");
