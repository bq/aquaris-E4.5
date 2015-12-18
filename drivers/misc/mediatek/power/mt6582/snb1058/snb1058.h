#define SNB1058_BUFSIZE     256
#define SNB1058_DEV_NAME    "charger_ic_i2c"


#define CHG_CTRL1        0x08
#define CHGEN           0x80
#define EXPDETEN        0x40
#define PTM             0x20
#define CHG_OFF         0x10
#define CHG_CTRL2        0x09
//#define CHG_VSET        0x08  /* kisung.song : charging voltage 4.35V  */
#define CHG_VSET        0x00    /*ykdk.kim : charging voltage 4.2V EVB  */

#define CONTROL1        0x01
#define CP_EN           0x01

#define CONTROL2        0x02
#define INTPOL          0x80
#define INT_EN          0x40
#define MIC_LP          0x20
#define CP_AUD          0x10
#define MB_200          0x08
#define INT2_EN         0x04
#define CHG_TYP         0x02
#define USB_DET_DIS     0x01

#define INTSTATUS1       0x04
#define CHGDET          0x80
#define MR_COMP     0x40
#define SENDEND     0x20
#define VBUS        0x10
#define IDNO_MASK   0x0F

#define INTSTATUS2      0x05
#define CHG             0x80


typedef enum
{
    CHG_90 = 90,
    CHG_100 = 100,
    CHG_USB = 400,
    CHG_500 = 500,
    CHG_600 = 600,
    CHG_TA = 700,
}CHG_TYPE;

typedef enum
{
    IPRECHG_40mA = 0x00,
    IPRECHG_60mA,
    IPRECHG_80mA,
    IPRECHG_100mA,
}IPRECHG_TYPE;

typedef enum
{
    CHG_CURR_90mA = 0x00,
    CHG_CURR_100mA = 0x10,
    CHG_CURR_400mA = 0x20,
    CHG_CURR_450mA = 0x30,
    CHG_CURR_500mA = 0x40,
    CHG_CURR_600mA = 0x50,
    CHG_CURR_700mA = 0x60,
    CHG_CURR_800mA = 0x70,
    CHG_CURR_900mA = 0x80,
    CHG_CURR_1000mA = 0x90,
}CHG_SET_TYPE;

typedef enum
{
    EOC_LVL_5PER = 0x00,
    EOC_LVL_10PER = 0x01,
    EOC_LVL_16PER = 0x02,
    EOC_LVL_20PER = 0x03,
    EOC_LVL_25PER = 0x04,
    EOC_LVL_33PER = 0x05,
    EOC_LVL_50PER = 0x06,
}IMIN_SET_TYPE;

enum {
    OFF,
    ON,
};

//#define TRUE    1
//#define FALSE   0

#define EN_CTRL         0x0A
//LED setting
#define LED1_EN         0x08
#define LED2_EN         0x04
#define LED3_EN         0x02
#define LED4_EN         0x01

#define LED_SET         0x0D
#define LED_DIM         0xE0

//LDO setting
#define LDO1_EN         0x80
#define LDO2_EN         0x40
#define LDO3_EN         0x20
#define LDO4_EN         0x10

#define LDO_VSET1       0x0B 
//#define LDO1_VSET       0x90 //2.8V  //AVDD
//#define LDO2_VSET       0x02 //1.5V for hi542 //DVDD

//#define LDO2_VSET       0x03 //1.8V for hi543  //DVDD

#define LDO_VSET2       0x0C 
//#define LDO3_VSET       0x30 //1.8V  //VIO
//#define LDO4_VSET       0x09 //2.8V  //AF

#define LDO1_VSET       0x10    // 1.2V
#define LDO2_VSET       0x09    // 2.8V
#define LDO3_VSET       0x30    // 1.8V
#define LDO4_VSET       0x09    // 2.8V


/* Function Prototype */
void check_snb1058_status(void);
int check_EOC_status(void);
void set_charger_start_mode(CHG_TYPE chg_type);
void set_charger_factory_mode(void);
void set_charger_stop_mode(void);
int is_charging_enable(void);

