

#ifndef META_PMIC_PARA_H
#define META_PMIC_PARA_H
#include "FT_Public.h"

/************************PMIC***********************/

#if 0 
/* The TestCase Enum define of XXX_module */
typedef enum{
	WM_CMD_WritePMICRegister,
	WM_CMD_ReadPMICRegister
}WM_PMIC_CMD_TYPE;
#endif

/* The command  parameter define of every TestCase */
typedef struct{
	BYTE registernumber;   //the valid value: 20~~ 96
	BYTE registervalue;
}WM_CMD_WritePMICRegister_REQ_T;


typedef struct{
	BYTE	registernumber;	/// the valid value: 0--96
}WM_CMD_ReadPMICRegister_REQ_T;


typedef struct{
	int value;   //not used
	BOOL		status;
}WM_CMD_WriteRegister_CNF_T;

typedef struct{
	BYTE value;
	BOOL		status;
}WM_CMD_ReadRegister_CNF_T;

#if 0
typedef union{
	WM_CMD_WritePMICRegister_REQ_T						m_rWmCmdWriteReq;
	WM_CMD_ReadPMICRegister_REQ_T						m_rWmCmdReadReq;
}WM_PMIC_REQ_CMD_U;

typedef union{
	WM_CMD_WriteRegister_CNF_T						m_rWmCmdWriteResult;
	WM_CMD_ReadRegister_CNF_T						m_rWmCmdReadResult;
}WM_PMIC_CNF_CMD_U;

typedef struct{
	FT_H				header;
	WM_PMIC_CMD_TYPE	type;
	WM_PMIC_REQ_CMD_U	cmd;
}FT_PMIC_COMMAND_REQ;

typedef struct{
	FT_H				header;
	WM_PMIC_CMD_TYPE	type;
	WM_PMIC_CNF_CMD_U	cmd;
	unsigned char		status;
}FT_PMIC_COMMAND_CNF;

#endif

typedef struct 
{
	FT_H							header;
	WM_CMD_ReadPMICRegister_REQ_T	m_rWmCmdReadReq;
}FT_PMIC_REG_READ;

typedef struct 
{
	FT_H						header;
	WM_CMD_ReadRegister_CNF_T	m_rWmCmdReadResult;
	unsigned char				status;
}FT_PMIC_REG_READ_CNF;

typedef struct 
{
	FT_H							header;
	WM_CMD_WritePMICRegister_REQ_T	m_rWmCmdWriteReq;
}FT_PMIC_REG_WRITE;

typedef struct 
{
	FT_H						header;
	WM_CMD_WriteRegister_CNF_T	m_rWmCmdWriteResult;	
	unsigned char				status;
}FT_PMIC_REG_WRITE_CNF;


//Low Power
 typedef enum{
    WM_CMD_POWER_DOWN =0, 
    WM_CMD_POWER_UP,
}  WM_LOW_POWER_TYPE;

 typedef enum{
    LOW_POWER_FAILED=0, 
    LOW_POWER_SUCCESS,
}  WM_LOW_POWER_STATUS;

typedef struct 
{
	FT_H							header;
	WM_LOW_POWER_TYPE	            type;
}FT_LOW_POWER_REQ;

typedef struct 
{
	FT_H							header;
	WM_LOW_POWER_TYPE               type;
	WM_LOW_POWER_STATUS             power_status;
	unsigned char				    status;		
}FT_LOW_POWER_CNF;


FT_LOW_POWER_CNF  		META_LOW_POWER_OP( FT_LOW_POWER_REQ  *req );
FT_PMIC_REG_READ_CNF   	META_PMICR_OP( FT_PMIC_REG_READ  *req );
FT_PMIC_REG_WRITE_CNF	META_PMICW_OP( FT_PMIC_REG_WRITE  *req );

BOOL Meta_Pmic_Init();
BOOL Meta_Pmic_Deinit();
BOOL FM_Low_Power(BOOL bOn);

#endif


