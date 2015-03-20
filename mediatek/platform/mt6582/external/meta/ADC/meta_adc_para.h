

#ifndef __META_AUXADC_PARA_H__
#define __META_AUXADC_PARA_H__

#include "FT_Public.h"
 
typedef struct{
	FT_H	    	header;  //module do not need care it
	unsigned char	dwChannel;	// channel 0-8
	unsigned short	dwCount;	// Detect number	
}AUXADC_REQ;

typedef struct{
	FT_H	    header;  //module do not need care it
	//DWORD			dwData;
	int			dwData;
	BOOL			ADCStatus;
	unsigned char	status;
}AUXADC_CNF;

/*the testcase enum define of adc module*/
typedef enum {
	ADC_OP_GET_CHANNE_NUM = 0,		//V 0
	ADC_OP_QUERY_EFUSE_CAL_EXIST,	//V 1
	ADC_OP_BAT_VOL,
	ADC_OP_BAT_CAPACITY,
	ADC_OP_END						//
} ADC_OP;

//=============Request==========================
typedef union {
	unsigned int dummy;
} META_ADC_REQ_U;

typedef struct {
	FT_H header;	//module do not need care it
	ADC_OP type;
	META_ADC_REQ_U 	req;
} ADC_REQ;

//============confirm=================================
typedef struct {
	unsigned int num;		//the number of channel available
} ADC_CHANNEL_NUM_T;

typedef struct {
	unsigned int isExist;	//Exist (1); Not exist(0)
} ADC_EFUSE_CAL_T;

typedef struct {
	int vol;	//battery voltage
} ADC_BAT_VOL;

typedef struct {
	int capacity;	//battery capacity
} ADC_BAT_CAPACITY;

typedef union {
	ADC_CHANNEL_NUM_T m_channel_num;
	ADC_EFUSE_CAL_T	m_efuse_cal;
	ADC_BAT_VOL m_bat_vol;
	ADC_BAT_CAPACITY m_bat_capacity;
	unsigned int dummy;
}META_ADC_CNF_U;

typedef struct {
	FT_H header;	//module do not need care it
	ADC_OP type;
	unsigned int status;
	unsigned int dummy;
	META_ADC_CNF_U cnf;	//fm->FT
} ADC_CNF;

/* please implement this function   */
BOOL 			Meta_AUXADC_Init(void);
void Meta_AUXADC_OP(AUXADC_REQ *req, char *peer_buff, unsigned short peer_len); 
BOOL 			Meta_AUXADC_Deinit(void);
void Meta_ADC_OP(ADC_REQ *req, char *peer_buff, unsigned short peer_len);

#endif


