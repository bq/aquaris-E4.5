

#ifndef __META_GPIO_PARA_H__
#define __META_GPIO_PARA_H__

#include "FT_Public.h"
#include "gpio_exp.h"

typedef enum {
	GET_MODE_STA = 0,
	SET_MODE_0,  // 00
	SET_MODE_1,  // 01
	SET_MODE_2,  // 10
	SET_MODE_3,  // 11

	GET_DIR_STA, 
	SET_DIR_IN,  // 0
	SET_DIR_OUT, // 1

	GET_PULLEN_STA,  
	SET_PULLEN_DISABLE,  // 0
	SET_PULLEN_ENABLE,   // 1

	GET_PULL_STA,
	SET_PULL_DOWN,  // 0
	SET_PULL_UP,    // 1

//	GET_INV_STA,
//	SET_INV_ENABLE,  // 1
//	SET_INV_DISABLE, // 0

	GET_DATA_IN,
	GET_DATA_OUT,
	SET_DATA_LOW,  // 0
	SET_DATA_HIGH, // 1
}GPIO_OP;
	
 
typedef struct{
	FT_H	header;  //module do not need care it
	HW_GPIO	pin;	// pin number
	GPIO_OP	op;	// operation to GPIO	
}GPIO_REQ;

typedef struct{
	FT_H	header;  //module do not need care it
	unsigned int	status;
	unsigned int	data;
}GPIO_CNF;


bool Meta_GPIO_Init(void);
GPIO_CNF Meta_GPIO_OP(GPIO_REQ req, unsigned char* peer_buf, unsigned short peer_len); 
bool Meta_GPIO_Deinit(void);

#endif




