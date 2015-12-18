#ifndef __SW_TX_POWER__
#define __SW_TX_POWER__

// description.
// 
//--------------------------------------------------------------
//              : 3G                 :2G
//--------------------------------------------------------------
//  ap_param    : Table 1 : Table 0  : Table 1 : Table 0  
//--------------------------------------------------------------
//  0x00        :    X    :     x    :    x    :    x   
//  0x01        :    X    :     x    :    x    :    o  
//  0x02        :    X    :     x    :    o    :    x  
//  0x03        :    X    :     x    :    o    :    o   
//  0x04        :    X    :     o    :    x    :    x   
//  0x05        :    X    :     o    :    x    :    o  
//  0x06        :    X    :     o    :    o    :    x   
//  0x07        :    X    :     o    :    o    :    o   
//  0x08        :    o    :     x    :    x    :    x   
//  0x09        :    o    :     x    :    x    :    o   
//  0x0A        :    o    :     x    :    o    :    x   
//  0x0B        :    o    :     x    :    o    :    o   
//  0x0C        :    o    :     o    :    x    :    x   
//  0x0D        :    o    :     o    :    x    :    o   
//  0x0E        :    o    :     o    :    o    :    x   
//  0x0F        :    o    :     o    :    o    :    o   
//--------------------------------------------------------------
//  others      :    invalid input
//--------------------------------------------------------------

// ap_param
#define ENABLE_SW_TX_POWER_2G_NONE	0x00000000
#define ENABLE_SW_TX_POWER_2G_TABLE0	0x00000001
#define ENABLE_SW_TX_POWER_2G_TABLE1	0x00000002
#define ENABLE_SW_TX_POWER_2G_TABLEX	0x00000003

#define ENABLE_SW_TX_POWER_3G_NONE	0x00000000
#define ENABLE_SW_TX_POWER_3G_TABLE0	0x00000004
#define ENABLE_SW_TX_POWER_3G_TABLE1	0x00000008
#define ENABLE_SW_TX_POWER_3G_TABLEX	0x0000000C

#define MODE_SWTP(v,x)		((ENABLE_SW_TX_POWER_##v)|(ENABLE_SW_TX_POWER_##x))

#define	 SWTP_SUPER_MODE 	0x00001FFF
#define  SWTP_NORMAL_MODE	0x000001FF

#define  SWTP_MODE_ON		0x00000001
#define  SWTP_MODE_OFF		0x00000000

enum {
	SWTP_CTRL_USER_SET0,
	SWTP_CTRL_USER_SET1,
	SWTP_CTRL_USER_SET2,
	SWTP_CTRL_USER_SET3,
	SWTP_CTRL_USER_SET4,
	SWTP_CTRL_USER_SET5,
	SWTP_CTRL_USER_SET6,
	SWTP_CTRL_USER_SET7,
	SWTP_CTRL_USER_SET8,
	SWTP_CTRL_USER_SET9,
	SWTP_CTRL_USER_SET10,
	SWTP_CTRL_USER_SET11,
	SWTP_CTRL_USER_SET12,
	SWTP_CTRL_USER_SET13,
	SWTP_CTRL_USER_SET14,
	SWTP_CTRL_USER_SET15,
	SWTP_CTRL_SUPER_SET,
	SWTP_CTRL_MAX_STATE 		
};

typedef struct swtp_state {
	unsigned int enable;
	unsigned int mode;
	unsigned int setvalue;
} swtp_state_type;

extern int switch_2G_Tx_Power(int md_id, unsigned int mode);
extern int switch_3G_Tx_Power(int md_id, unsigned int mode);

#endif // __SW_TX_POWER__
