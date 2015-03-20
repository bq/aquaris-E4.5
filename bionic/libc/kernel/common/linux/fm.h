/* 
 *
 * (C) Copyright 2009 
 * MediaTek <www.MediaTek.com>
 * William Chung <William.Chung@MediaTek.com>
 *
 * MT6516 AR10x0 FM Radio Driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __FM_H__
#define __FM_H__

//#define FMDEBUG

#include <linux/ioctl.h>
#include <linux/time.h>
//fm type define
typedef signed char fm_s8;
typedef signed short fm_s16;
typedef signed int fm_s32;
typedef signed long long fm_s64;
typedef unsigned char fm_u8;
typedef unsigned short fm_u16;
typedef unsigned int fm_u32;
typedef unsigned long long fm_u64;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef signed short int16_t;

typedef enum fm_bool {
    fm_false = 0,
    fm_true  = 1
} fm_bool;

//scan sort algorithm 
enum{
    FM_SCAN_SORT_NON = 0,
    FM_SCAN_SORT_UP,
    FM_SCAN_SORT_DOWN,
    FM_SCAN_SORT_MAX
};

//scan methods
enum{
    FM_SCAN_SEL_HW = 0, //select hardware scan, advantage: fast
    FM_SCAN_SEL_SW,     //select software scan, advantage: more accurate
    FM_SCAN_SEL_MAX
};

#ifdef MTK_FM_50KHZ_SUPPORT
//*****************************************************************************************
//***********************************FM config for customer ***********************************
//*****************************************************************************************
//RX
#define FMR_RSSI_TH_LONG    0x02D8      //FM radio long antenna RSSI threshold(-4dBuV)
#define FMR_RSSI_TH_SHORT   0x02D8      //FM radio short antenna RSSI threshold(-4dBuV)
#define FMR_CQI_TH          0x00E9      //FM radio Channel quality indicator threshold(0x0000~0x00FF)
#define FMR_SEEK_SPACE      5           //FM radio seek space,5:50KHZ; 1:100KHZ; 2:200KHZ
#define FMR_SCAN_CH_SIZE    80          //FM radio scan max channel size
#define FMR_BAND            1           //FM radio band, 1:87.5MHz~108.0MHz; 2:76.0MHz~90.0MHz; 3:76.0MHz~108.0MHz; 4:special
#define FMR_BAND_FREQ_L     8750         //FM radio special band low freq(Default 87.50MHz)
#define FMR_BAND_FREQ_H     10800        //FM radio special band high freq(Default 108.00MHz)
#define FM_SCAN_SORT_SELECT FM_SCAN_SORT_NON
#define FM_SCAN_SELECT      FM_SCAN_SEL_HW
#define FM_SCAN_SOFT_MUTE_GAIN_TH  3    //soft-mute threshold when software scan, rang: 0~3, 0 means better audio quality but less channel 
#define FM_CHIP_DESE_RSSI_TH (-102)      // rang: -102 ~ -72
//TX
#define FMTX_PWR_LEVEL_MAX  120         //FM transmitter power level, rang: 85db~120db, default 120db

//*****************************************************************************************
//***********************************FM config for engineer ***********************************
//*****************************************************************************************
//RX
#define FMR_MR_TH           0x01BD      //FM radio MR threshold
#define ADDR_SCAN_TH        0xE0        //scan thrshold register
#define ADDR_CQI_TH         0xE1        //scan CQI register

//TX
#define FMTX_SCAN_HOLE_LOW  9230         //92.30MHz~95.40MHz should not show to user
#define FMTX_SCAN_HOLE_HIGH 9540         //92.30MHz~95.40MHz should not show to user
//*****************************************************************************************

#else

//*****************************************************************************************
//***********************************FM config for customer ***********************************
//*****************************************************************************************
//RX
#define FMR_RSSI_TH_LONG    0x0301      //FM radio long antenna RSSI threshold(11.375dBuV)
#define FMR_RSSI_TH_SHORT   0x02E0      //FM radio short antenna RSSI threshold(-1dBuV)
#define FMR_CQI_TH          0x00E9      //FM radio Channel quality indicator threshold(0x0000~0x00FF)
#define FMR_SEEK_SPACE      1           //FM radio seek space,1:100KHZ; 2:200KHZ
#define FMR_SCAN_CH_SIZE    40          //FM radio scan max channel size
#define FMR_BAND            1           //FM radio band, 1:87.5MHz~108.0MHz; 2:76.0MHz~90.0MHz; 3:76.0MHz~108.0MHz; 4:special
#define FMR_BAND_FREQ_L     875         //FM radio special band low freq(Default 87.5MHz)
#define FMR_BAND_FREQ_H     1080        //FM radio special band high freq(Default 108.0MHz)
#define FM_SCAN_SORT_SELECT FM_SCAN_SORT_NON
#define FM_SCAN_SELECT      FM_SCAN_SEL_HW
#define FM_SCAN_SOFT_MUTE_GAIN_TH  3    //soft-mute threshold when software scan, rang: 0~3, 0 means better audio quality but less channel 
#define FM_CHIP_DESE_RSSI_TH (-102)      // rang: -102 ~ -72

//TX
#define FMTX_PWR_LEVEL_MAX  120         //FM transmitter power level, rang: 85db~120db, default 120db

//*****************************************************************************************
//***********************************FM config for engineer ***********************************
//*****************************************************************************************
//RX
#define FMR_MR_TH           0x01BD      //FM radio MR threshold
#define ADDR_SCAN_TH        0xE0        //scan thrshold register
#define ADDR_CQI_TH         0xE1        //scan CQI register

//TX
#define FMTX_SCAN_HOLE_LOW  923         //92.3MHz~95.4MHz should not show to user
#define FMTX_SCAN_HOLE_HIGH 954         //92.3MHz~95.4MHz should not show to user
//*****************************************************************************************

#endif //MTK_FM_50KHZ_SUPPORT

#define FM_NAME             "fm"
#define FM_DEVICE_NAME      "/dev/fm"

// errno
#define FM_SUCCESS      0
#define FM_FAILED       1
#define FM_EPARM        2
#define FM_BADSTATUS    3
#define FM_TUNE_FAILED  4
#define FM_SEEK_FAILED  5
#define FM_BUSY         6
#define FM_SCAN_FAILED  7

// band

#define FM_BAND_UNKNOWN 0
#define FM_BAND_UE      1 // US/Europe band  87.5MHz ~ 108MHz (DEFAULT)
#define FM_BAND_JAPAN   2 // Japan band      76MHz   ~ 90MHz
#define FM_BAND_JAPANW  3 // Japan wideband  76MHZ   ~ 108MHz
#define FM_BAND_SPECIAL 4 // special   band  between 76MHZ   and  108MHz
#define FM_BAND_DEFAULT FM_BAND_UE

#ifdef MTK_FM_50KHZ_SUPPORT
#define FM_UE_FREQ_MIN  8750
#define FM_UE_FREQ_MAX  10800
#define FM_JP_FREQ_MIN  7600
#define FM_JP_FREQ_MAX  10800
#define FM_FREQ_MIN  FMR_BAND_FREQ_L
#define FM_FREQ_MAX  FMR_BAND_FREQ_H
#define FM_RAIDO_BAND FM_BAND_UE
#else
#define FM_UE_FREQ_MIN  875
#define FM_UE_FREQ_MAX  1080
#define FM_JP_FREQ_MIN  760
#define FM_JP_FREQ_MAX  1080
#define FM_FREQ_MIN  FMR_BAND_FREQ_L
#define FM_FREQ_MAX  FMR_BAND_FREQ_H
#define FM_RAIDO_BAND FM_BAND_UE
#endif //MTK_FM_50KHZ_SUPPORT

// space
#define FM_SPACE_UNKNOWN    0
#define FM_SPACE_100K       1
#define FM_SPACE_200K       2
#define FM_SPACE_50K        5
#ifdef MTK_FM_50KHZ_SUPPORT
#define FM_SPACE_DEFAULT    FM_SPACE_50K
#else
#define FM_SPACE_DEFAULT    FM_SPACE_100K
#endif

#define FM_SEEK_SPACE FMR_SEEK_SPACE

//max scan chl num
#define FM_MAX_CHL_SIZE FMR_SCAN_CH_SIZE
// auto HiLo
#define FM_AUTO_HILO_OFF    0
#define FM_AUTO_HILO_ON     1

// seek direction
#define FM_SEEK_UP          0
#define FM_SEEK_DOWN        1

#define FM_CHIP_AR1000 0x1000
#define FM_CHIP_MT5192 0x91
#define FM_CHIP_MT5193 0x92
#define FM_CHIP_MT6616 0x6616
#define FM_CHIP_MT6620 0x6620
#define FM_CHIP_MT6626 0x6626
#define FM_CHIP_MT6628 0x6628
#define FM_CHIP_MT6627 0x6627
#define FM_CHIP_MT6630 0x6630

#define FM_CHIP_UNSUPPORTED -1

// seek threshold
#define FM_SEEKTH_LEVEL_DEFAULT 4

struct fm_tune_parm {
    uint8_t err;
    uint8_t band;
    uint8_t space;
    uint8_t hilo;
    uint16_t freq; // IN/OUT parameter
};

struct fm_seek_parm {
    uint8_t err;
    uint8_t band;
    uint8_t space;
    uint8_t hilo;
    uint8_t seekdir;
    uint8_t seekth;
    uint16_t freq; // IN/OUT parameter
};

#ifdef MTK_FM_50KHZ_SUPPORT
struct fm_scan_parm {
    uint8_t  err;
    uint8_t  band;
    uint8_t  space;
    uint8_t  hilo;
    uint16_t freq; // OUT parameter
    uint16_t ScanTBL[26]; //need no less than the chip
    uint16_t ScanTBLSize; //IN/OUT parameter
};
#else
struct fm_scan_parm {
    uint8_t  err;
    uint8_t  band;
    uint8_t  space;
    uint8_t  hilo;
    uint16_t freq; // OUT parameter
    uint16_t ScanTBL[16]; //need no less than the chip
    uint16_t ScanTBLSize; //IN/OUT parameter
};
#endif

struct fm_ch_rssi{
    uint16_t freq;
    int rssi;
};

enum fm_scan_cmd_t {
    FM_SCAN_CMD_INIT = 0,
    FM_SCAN_CMD_START,
    FM_SCAN_CMD_GET_NUM,
    FM_SCAN_CMD_GET_CH,
    FM_SCAN_CMD_GET_RSSI,
    FM_SCAN_CMD_GET_CH_RSSI,
    FM_SCAN_CMD_MAX
};

struct fm_scan_t {
    enum fm_scan_cmd_t cmd;    
    int ret;    // 0, success; else error code
    uint16_t lower;             // lower band, Eg, 7600 -> 76.0Mhz
    uint16_t upper;             // upper band, Eg, 10800 -> 108.0Mhz
    int space;                  // 5: 50KHz, 10: 100Khz, 20: 200Khz
    int num;                    // valid channel number 
    void *priv;
    int sr_size;                // scan result buffer size in bytes
    union {
        uint16_t *ch_buf;       // channel buffer
        int *rssi_buf;          // rssi buffer
        struct fm_ch_rssi *ch_rssi_buf;  //channel and RSSI buffer 
    } sr;    
};

struct fm_seek_t {  
    int ret;                    // 0, success; else error code
    uint16_t freq;
    uint16_t lower;             // lower band, Eg, 7600 -> 76.0Mhz
    uint16_t upper;             // upper band, Eg, 10800 -> 108.0Mhz
    int space;                  // 5: 50KHz, 10: 100Khz, 20: 200Khz
    int dir;                    // 0: up; 1: down
    int th;                     // seek threshold in dbm(Eg, -95dbm)
    void *priv;
};

struct fm_tune_t {  
    int ret;                    // 0, success; else error code
    uint16_t freq;
    uint16_t lower;             // lower band, Eg, 7600 -> 76.0Mhz
    uint16_t upper;             // upper band, Eg, 10800 -> 108.0Mhz
    int space;                  // 5: 50KHz, 10: 100Khz, 20: 200Khz
    void *priv;
};
struct fm_softmute_tune_t 
{  
	fm_s32 rssi;              // RSSI of current channel
	fm_u16 freq;				//current frequency
	fm_bool valid;				    //current channel is valid(true) or not(false)
};

#ifdef MTK_FM_50KHZ_SUPPORT
struct fm_rssi_req{
    uint16_t num;
    uint16_t read_cnt;
    struct fm_ch_rssi cr[26*16];
};
#else
struct fm_rssi_req{
    uint16_t num;
    uint16_t read_cnt;
    struct fm_ch_rssi cr[16*16];
};
#endif
struct fm_hw_info{
    int chip_id; //chip ID, eg. 6620
    int eco_ver; //chip ECO version, eg. E3
    int rom_ver; //FM DSP rom code version, eg. V2
    int patch_ver; //FM DSP patch version, eg. 1.11
    int reserve;
};

struct fm_search_threshold_t 
{  
	fm_s32 th_type;// 0, RSSI. 1,desense RSSI. 2,SMG.
	fm_s32 th_val; //threshold value
	fm_s32 reserve;
};

#if 1
#define NEED_DEF_RDS 1
#else
#define NEED_DEF_RDS 0
#endif

#if NEED_DEF_RDS
//For RDS feature
typedef struct
{
   uint8_t TP;
   uint8_t TA;
   uint8_t Music;
   uint8_t Stereo;
   uint8_t Artificial_Head;
   uint8_t Compressed;
   uint8_t Dynamic_PTY;
   uint8_t Text_AB;
   uint32_t flag_status;
}RDSFlag_Struct;

typedef struct
{
   uint16_t Month;
   uint16_t Day;
   uint16_t Year;
   uint16_t Hour;
   uint16_t Minute;
   uint8_t Local_Time_offset_signbit;
   uint8_t Local_Time_offset_half_hour;
}CT_Struct;

typedef struct
{
   int16_t AF_Num;
   int16_t AF[2][25];  //100KHz
   uint8_t Addr_Cnt;
   uint8_t isMethod_A;
   uint8_t isAFNum_Get;
}AF_Info;

typedef struct
{
   uint8_t PS[4][8];
   uint8_t Addr_Cnt;
}PS_Info;

typedef struct
{
   uint8_t TextData[4][64];
   uint8_t GetLength;
   uint8_t isRTDisplay;
   uint8_t TextLength;
   uint8_t isTypeA;
   uint8_t BufCnt;
   uint16_t Addr_Cnt;
}RT_Info;

struct rds_raw_data
{
    int dirty; //indicate if the data changed or not
    int len; //the data len form chip
    uint8_t data[146];
};

struct rds_group_cnt
{
    unsigned long total;
    unsigned long groupA[16]; //RDS groupA counter
    unsigned long groupB[16]; //RDS groupB counter
};

enum rds_group_cnt_opcode
{
    RDS_GROUP_CNT_READ = 0,
    RDS_GROUP_CNT_WRITE,
    RDS_GROUP_CNT_RESET,
    RDS_GROUP_CNT_MAX
};

struct rds_group_cnt_req
{
    int err;
    enum rds_group_cnt_opcode op;
    struct rds_group_cnt gc;
};

typedef struct
{
   CT_Struct CT;
   RDSFlag_Struct RDSFlag;
   uint16_t PI;
   uint8_t Switch_TP;
   uint8_t PTY;
   AF_Info AF_Data;
   AF_Info AFON_Data;
   uint8_t Radio_Page_Code;
   uint16_t Program_Item_Number_Code;
   uint8_t Extend_Country_Code;
   uint16_t Language_Code;
   PS_Info PS_Data;
   uint8_t PS_ON[8];   
   RT_Info RT_Data;
   uint16_t event_status; //will use RDSFlag_Struct RDSFlag->flag_status to check which event, is that ok? 
   struct rds_group_cnt gc;
} RDSData_Struct;


//Need care the following definition.
//valid Rds Flag for notify
typedef enum {
   RDS_FLAG_IS_TP              = 0x0001, // Program is a traffic program
   RDS_FLAG_IS_TA              = 0x0002, // Program currently broadcasts a traffic ann.
   RDS_FLAG_IS_MUSIC           = 0x0004, // Program currently broadcasts music
   RDS_FLAG_IS_STEREO          = 0x0008, // Program is transmitted in stereo
   RDS_FLAG_IS_ARTIFICIAL_HEAD = 0x0010, // Program is an artificial head recording
   RDS_FLAG_IS_COMPRESSED      = 0x0020, // Program content is compressed
   RDS_FLAG_IS_DYNAMIC_PTY     = 0x0040, // Program type can change 
   RDS_FLAG_TEXT_AB            = 0x0080  // If this flag changes state, a new radio text 					 string begins
} RdsFlag;

typedef enum {
   RDS_EVENT_FLAGS          = 0x0001, // One of the RDS flags has changed state
   RDS_EVENT_PI_CODE        = 0x0002, // The program identification code has changed
   RDS_EVENT_PTY_CODE       = 0x0004, // The program type code has changed
   RDS_EVENT_PROGRAMNAME    = 0x0008, // The program name has changed
   RDS_EVENT_UTCDATETIME    = 0x0010, // A new UTC date/time is available
   RDS_EVENT_LOCDATETIME    = 0x0020, // A new local date/time is available
   RDS_EVENT_LAST_RADIOTEXT = 0x0040, // A radio text string was completed
   RDS_EVENT_AF             = 0x0080, // Current Channel RF signal strength too weak, need do AF switch  
   RDS_EVENT_AF_LIST        = 0x0100, // An alternative frequency list is ready
   RDS_EVENT_AFON_LIST      = 0x0200, // An alternative frequency list is ready
   RDS_EVENT_TAON           = 0x0400,  // Other Network traffic announcement start
   RDS_EVENT_TAON_OFF       = 0x0800, // Other Network traffic announcement finished.
   RDS_EVENT_RDS            = 0x2000, // RDS Interrupt had arrived durint timer period  
   RDS_EVENT_NO_RDS         = 0x4000, // RDS Interrupt not arrived durint timer period  
   RDS_EVENT_RDS_TIMER      = 0x8000 // Timer for RDS Bler Check. ---- BLER  block error rate
} RdsEvent;
#endif

struct fm_rds_tx_parm {
    uint8_t err;
    uint16_t pi;
    uint16_t ps[12]; // 4 ps
    uint16_t other_rds[87];  // 0~29 other groups
    uint8_t other_rds_cnt; // # of other group
};

typedef struct fm_rds_tx_req{
    unsigned char pty;         // 0~31 integer
    unsigned char rds_rbds;    // 0:RDS, 1:RBDS
    unsigned char dyn_pty;     // 0:static, 1:dynamic
    unsigned short pi_code;    // 2-byte hex
    unsigned char ps_buf[8];     // hex buf of PS
    unsigned char ps_len;      // length of PS, must be 0 / 8"
    unsigned char af;          // 0~204, 0:not used, 1~204:(87.5+0.1*af)MHz
    unsigned char ah;          // Artificial head, 0:no, 1:yes
    unsigned char stereo;      // 0:mono, 1:stereo
    unsigned char compress;    // Audio compress, 0:no, 1:yes
    unsigned char tp;          // traffic program, 0:no, 1:yes
    unsigned char ta;          // traffic announcement, 0:no, 1:yes
    unsigned char speech;      // 0:music, 1:speech
}fm_rds_tx_req;

#define TX_SCAN_MAX 10
#define TX_SCAN_MIN 1
struct fm_tx_scan_parm {
    uint8_t  err;
    uint8_t  band;	//87.6~108MHz
    uint8_t  space;
    uint8_t  hilo;
    uint16_t freq; 	// start freq, if less than band min freq, then will use band min freq
    uint8_t	 scandir;
    uint16_t ScanTBL[TX_SCAN_MAX]; 	//need no less than the chip
    uint16_t ScanTBLSize; //IN: desired size, OUT: scan result size 
};

struct fm_gps_rtc_info{
    int             err;            //error number, 0: success, other: err code
    int             retryCnt;       //GPS mnl can decide retry times
    int             ageThd;         //GPS 3D fix time diff threshold
    int             driftThd;       //GPS RTC drift threshold
    struct timeval  tvThd;          //time value diff threshold
    int             age;            //GPS 3D fix time diff
    int             drift;          //GPS RTC drift
    union{
        unsigned long stamp;        //time stamp in jiffies
        struct timeval  tv;         //time stamp value in RTC
    };
    int             flag;           //rw flag
};

typedef enum
{
	FM_I2S_ON = 0,
	FM_I2S_OFF,
    FM_I2S_STATE_ERR
}fm_i2s_state_e;

typedef enum
{
	FM_I2S_MASTER = 0,
	FM_I2S_SLAVE,
    FM_I2S_MODE_ERR
}fm_i2s_mode_e;

typedef enum
{
	FM_I2S_32K = 0,
	FM_I2S_44K,
	FM_I2S_48K,
    FM_I2S_SR_ERR
}fm_i2s_sample_e;

struct fm_i2s_setting{
    int onoff;
    int mode;
    int sample;
};

typedef enum{
    FM_RX = 0,
    FM_TX = 1
}FM_PWR_T;

typedef struct fm_i2s_info
{
    int status; /*0:FM_I2S_ON, 1:FM_I2S_OFF,2:error*/
    int mode;   /*0:FM_I2S_MASTER, 1:FM_I2S_SLAVE,2:error*/
    int rate;   /*0:FM_I2S_32K:32000,1:FM_I2S_44K:44100,2:FM_I2S_48K:48000,3:error*/
} fm_i2s_info_t;

typedef enum 
{
    FM_AUD_ANALOG = 0,
    FM_AUD_I2S = 1,    
    FM_AUD_MRGIF = 2,
    FM_AUD_ERR
}fm_audio_path_e;

typedef enum 
{
    FM_I2S_PAD_CONN = 0, //sco fm chip: e.g.6627
    FM_I2S_PAD_IO = 1,   //combo fm chip: e.g.6628
    FM_I2S_PAD_ERR
}fm_i2s_pad_sel_e;

typedef struct fm_audio_info
{
    fm_audio_path_e aud_path;
    fm_i2s_info_t i2s_info;
    fm_i2s_pad_sel_e i2s_pad;
} fm_audio_info_t;

struct fm_cqi 
{
    int ch;
    int rssi;
    int reserve;
};

struct fm_cqi_req 
{
    uint16_t ch_num;
    int buf_size;
    char *cqi_buf;
};
typedef struct
{
	int freq;
	int rssi;
}fm_desense_check_t;

typedef struct 
{
    uint16_t lower;             // lower band, Eg, 7600 -> 76.0Mhz
    uint16_t upper;             // upper band, Eg, 10800 -> 108.0Mhz
    int space;                  // 0x1: 50KHz, 0x2: 100Khz, 0x4: 200Khz
    int cycle;                  // repeat times 
}fm_full_cqi_log_t;

// ********** ***********FM IOCTL define start *******************************

#define FM_IOC_MAGIC        0xf5 // FIXME: any conflict?

#define FM_IOCTL_POWERUP       _IOWR(FM_IOC_MAGIC, 0, struct fm_tune_parm*)
#define FM_IOCTL_POWERDOWN     _IOWR(FM_IOC_MAGIC, 1, int32_t*)
#define FM_IOCTL_TUNE          _IOWR(FM_IOC_MAGIC, 2, struct fm_tune_parm*)
#define FM_IOCTL_SEEK          _IOWR(FM_IOC_MAGIC, 3, struct fm_seek_parm*)
#define FM_IOCTL_SETVOL        _IOWR(FM_IOC_MAGIC, 4, uint32_t*)
#define FM_IOCTL_GETVOL        _IOWR(FM_IOC_MAGIC, 5, uint32_t*)
#define FM_IOCTL_MUTE          _IOWR(FM_IOC_MAGIC, 6, uint32_t*)
#define FM_IOCTL_GETRSSI       _IOWR(FM_IOC_MAGIC, 7, int32_t*)
#define FM_IOCTL_SCAN          _IOWR(FM_IOC_MAGIC, 8, struct fm_scan_parm*)
#define FM_IOCTL_STOP_SCAN     _IO(FM_IOC_MAGIC,   9)

//IOCTL and struct for test
#define FM_IOCTL_GETCHIPID     _IOWR(FM_IOC_MAGIC, 10, uint16_t*)
#define FM_IOCTL_EM_TEST       _IOWR(FM_IOC_MAGIC, 11, struct fm_em_parm*)
#define FM_IOCTL_RW_REG        _IOWR(FM_IOC_MAGIC, 12, struct fm_ctl_parm*)
#define FM_IOCTL_GETMONOSTERO  _IOWR(FM_IOC_MAGIC, 13, uint16_t*)
#define FM_IOCTL_GETCURPAMD    _IOWR(FM_IOC_MAGIC, 14, uint16_t*)
#define FM_IOCTL_GETGOODBCNT   _IOWR(FM_IOC_MAGIC, 15, uint16_t*)
#define FM_IOCTL_GETBADBNT     _IOWR(FM_IOC_MAGIC, 16, uint16_t*)
#define FM_IOCTL_GETBLERRATIO  _IOWR(FM_IOC_MAGIC, 17, uint16_t*)

//IOCTL for RDS 
#define FM_IOCTL_RDS_ONOFF     _IOWR(FM_IOC_MAGIC, 18, uint16_t*)
#define FM_IOCTL_RDS_SUPPORT   _IOWR(FM_IOC_MAGIC, 19, int32_t*)

#define FM_IOCTL_POWERUP_TX    _IOWR(FM_IOC_MAGIC, 20, struct fm_tune_parm*)
#define FM_IOCTL_TUNE_TX       _IOWR(FM_IOC_MAGIC, 21, struct fm_tune_parm*)
#define FM_IOCTL_RDS_TX        _IOWR(FM_IOC_MAGIC, 22, struct fm_rds_tx_parm*)

#define FM_IOCTL_RDS_SIM_DATA  _IOWR(FM_IOC_MAGIC, 23, uint32_t*)
#define FM_IOCTL_IS_FM_POWERED_UP  _IOWR(FM_IOC_MAGIC, 24, uint32_t*)

//IOCTL for FM Tx
#define FM_IOCTL_TX_SUPPORT    _IOWR(FM_IOC_MAGIC, 25, int32_t*)
#define FM_IOCTL_RDSTX_SUPPORT _IOWR(FM_IOC_MAGIC, 26, int32_t*)
#define FM_IOCTL_RDSTX_ENABLE  _IOWR(FM_IOC_MAGIC, 27, int32_t*)
#define FM_IOCTL_TX_SCAN       _IOWR(FM_IOC_MAGIC, 28, struct fm_tx_scan_parm*)

//IOCTL for FM over BT
#define FM_IOCTL_OVER_BT_ENABLE  _IOWR(FM_IOC_MAGIC, 29, int32_t*)

//IOCTL for FM ANTENNA SWITCH
#define FM_IOCTL_ANA_SWITCH     _IOWR(FM_IOC_MAGIC, 30, int32_t*)
#define FM_IOCTL_GETCAPARRAY  	_IOWR(FM_IOC_MAGIC, 31, int32_t*)

//IOCTL for FM compensation by GPS RTC
#define FM_IOCTL_GPS_RTC_DRIFT  _IOWR(FM_IOC_MAGIC, 32, struct fm_gps_rtc_info*)

//IOCTL for FM I2S Setting
#define FM_IOCTL_I2S_SETTING  _IOWR(FM_IOC_MAGIC, 33, struct fm_i2s_setting*)

#define FM_IOCTL_RDS_GROUPCNT   _IOWR(FM_IOC_MAGIC, 34, struct rds_group_cnt_req*)
#define FM_IOCTL_RDS_GET_LOG    _IOWR(FM_IOC_MAGIC, 35, struct rds_raw_data*)

#define FM_IOCTL_SCAN_GETRSSI   _IOWR(FM_IOC_MAGIC, 36, struct fm_rssi_req*)
#define FM_IOCTL_SETMONOSTERO   _IOWR(FM_IOC_MAGIC, 37, int32_t)
#define FM_IOCTL_RDS_BC_RST     _IOWR(FM_IOC_MAGIC, 38, int32_t*)
#define FM_IOCTL_CQI_GET     _IOWR(FM_IOC_MAGIC, 39, struct fm_cqi_req*)
#define FM_IOCTL_GET_HW_INFO    _IOWR(FM_IOC_MAGIC, 40, struct fm_hw_info*)
#define FM_IOCTL_GET_I2S_INFO   _IOWR(FM_IOC_MAGIC, 41, fm_i2s_info_t*)
#define FM_IOCTL_IS_DESE_CHAN   _IOWR(FM_IOC_MAGIC, 42, int32_t*)
#define FM_IOCTL_TOP_RDWR _IOWR(FM_IOC_MAGIC, 43, struct fm_top_rw_parm*)
#define FM_IOCTL_HOST_RDWR  _IOWR(FM_IOC_MAGIC, 44, struct fm_host_rw_parm*)

#define FM_IOCTL_PRE_SEARCH _IOWR(FM_IOC_MAGIC, 45,int32_t)
#define FM_IOCTL_RESTORE_SEARCH _IOWR(FM_IOC_MAGIC, 46,int32_t)

#define FM_IOCTL_SET_SEARCH_THRESHOLD   _IOWR(FM_IOC_MAGIC, 47, fm_search_threshold_t*)

#define FM_IOCTL_GET_AUDIO_INFO _IOWR(FM_IOC_MAGIC, 48, fm_audio_info_t*)

#define FM_IOCTL_SCAN_NEW       _IOWR(FM_IOC_MAGIC, 60, struct fm_scan_t*)
#define FM_IOCTL_SEEK_NEW       _IOWR(FM_IOC_MAGIC, 61, struct fm_seek_t*)
#define FM_IOCTL_TUNE_NEW       _IOWR(FM_IOC_MAGIC, 62, struct fm_tune_t*)

#define FM_IOCTL_SOFT_MUTE_TUNE _IOWR(FM_IOC_MAGIC, 63, struct fm_softmute_tune_t*)/*for soft mute tune*/
#define FM_IOCTL_DESENSE_CHECK   _IOWR(FM_IOC_MAGIC, 64, fm_desense_check_t*)

//IOCTL for EM
#define FM_IOCTL_FULL_CQI_LOG _IOWR(FM_IOC_MAGIC, 70, fm_full_cqi_log_t *)

#define FM_IOCTL_DUMP_REG   _IO(FM_IOC_MAGIC, 0xFF)

// ********** ***********FM IOCTL define end *******************************


enum group_idx {
    mono=0,
    stereo,
    RSSI_threshold,
    HCC_Enable,
    PAMD_threshold,
    Softmute_Enable,
    De_emphasis,
    HL_Side,
    Demod_BW,
    Dynamic_Limiter,
    Softmute_Rate,
    AFC_Enable,
    Softmute_Level,
    Analog_Volume,
    GROUP_TOTAL_NUMS
};
	
enum item_idx {
    Sblend_OFF=0,
    Sblend_ON,  
    ITEM_TOTAL_NUMS
};

struct fm_ctl_parm {
    uint8_t err;
    uint8_t addr;
    uint16_t val;
    uint16_t rw_flag;//0:write, 1:read
};

struct fm_em_parm {
	uint16_t group_idx;
	uint16_t item_idx;
	uint32_t item_value;	
};
#endif // __FM_H__
