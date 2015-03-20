
#ifndef _MTK_MFBMM_H
#define _MTK_MFBMM_H

#include "MTKMfbmmType.h"
#include "MTKMfbmmErrCode.h"



/*****************************************************************************
    GLOBAL CONSTANTS
******************************************************************************/
#define MAX_BLD_NUM 8
#define MIN_BLD_NUM 4
#define MAX_SEQ_NUM 20
#define MAX_SEQ_NUM_DATA_IN 120
#define SE_DATA_LEN 32
#define MAX_AUTO_NUM 4

typedef enum DRVMfbmmObject_s 
{
    DRV_MFBMM_OBJ_NONE = 0,
    DRV_MFBMM_OBJ_SW,
    DRV_MFBMM_OBJ_SW_NEON,    
    DRV_MFBMM_OBJ_UNKNOWN = 0xFF,
} DrvMfbmmObject_e;

/*****************************************************************************
	Main Module
******************************************************************************/
typedef enum MFBMM_PROC_ENUM 
{
    MFBMM_PROC1 = 0,
    MFBMM_PROC2,
    MFBMM_PROC3,
    MFBMM_UNKNOWN_PROC,
} MFBMM_PROC_ENUM;

typedef enum MFBMM_STATE_ENUM
{
	MFBMM_STATE_IDLE=0,		
	MFBMM_STATE_STANDBY,
	MFBMM_STATE_INITED,
	MFBMM_STATE_ADD_INPUT,
	MFBMM_STATE_PROC1,
	MFBMM_STATE_READY1,
    MFBMM_STATE_PROC2,
	MFBMM_STATE_READY2,	
    MFBMM_STATE_PROC3,
} MFBMM_STATE_ENUM;

typedef enum MFBMM_FTCTRL_ENUM
{
	MFBMM_FTCTRL_GET_LOG,
    MFBMM_FTCTRL_GET_PROC_INFO,
    MFBMM_FTCTRL_GET_REF_IMAGE,
    MFBMM_FTCTRL_SET_WORKBUF_INFO,
    MFBMM_FTCTRL_SET_INTERMEDIATE,
    MFBMM_FTCTRL_GET_INTERMEDIATE,
    MFBMM_FTCTRL_ADD_IMAGE,
	MFBMM_FTCTRL_MAX
}	MFBMM_FTCTRL_ENUM;

/*****************************************************************************
    MOTION & BLENDER INIT MODULE
******************************************************************************/

typedef enum
{
    MFBMM_MODE_FIRST_ON_TOP,
    MFBMM_MODE_LAST_ON_TOP,    
}MFBMM_MODE_ENUM;

typedef enum
{
    MFBMM_USEMODE_AUTO,
    MFBMM_USEMODE_MANUAL,
}MFBMM_USEMODE_ENUM;

typedef struct
{
    MFBMM_MODE_ENUM mode; 
    MUINT8 maxMoveRange;
} MFBMM_TUNING_STRUCT, *P_MFBMM_TUNING_STRUCT;

typedef struct
{
    MINT32 img_width;
    MINT32 img_height;
    MFBMM_USEMODE_ENUM mode;
    MUINT32 thread_num;
    MFBMM_TUNING_STRUCT tuning_param;
} MFBMM_INIT_PARAM_STRUCT, *P_MFBMM_INIT_PARAM_STRUCT;

// for getting work buffer size
typedef struct
{
    MUINT32 workbuf_size;
} MFBMM_GET_PROC_INFO_STRUCT, *P_MFBMM_GET_PROC_INFO_STRUCT;

// for setting work buffer base address and size
typedef struct
{
    MUINT8*  workbuf_addr;
    MUINT32  workbuf_size;
} MFBMM_SET_WORKBUF_INFO_STRUCT, *P_MFBMM_SET_WORKBUF_INFO_STRUCT;

// for setting work buffer base address and size
typedef struct
{
    MUINT8   img_index;
    MUINT8*  thbImageAddr;
    MUINT32  width;
    MUINT32  height;
    MUINT32  stride_y;
    MUINT32  stride_uv;
    MINT32   mvx;
    MINT32   mvy;    
} MFBMM_ADD_IMAGE_STRUCT, *P_MFBMM_ADD_IMAGE_STRUCT;

typedef struct
{
    MINT32 auto_idx[MAX_AUTO_NUM];
    MUINT32 img_num;
    MINT32 se_data[MAX_SEQ_NUM][2];
} MFBMM_INTERMEDIATE_STRUCT, *P_MFBMM_INTERMEDIATE_STRUCT;

typedef struct
{
    MUINT8 manual_num;
    MUINT8 manual_idx[MAX_BLD_NUM];
} MFBMM_PROC1_INFO_STRUCT, *P_MFBMM_PROC1_INFO_STRUCT;

typedef struct
{
    MUINT32 can_num;    //candidate number returned;
    MUINT8 can_img_idx[MAX_BLD_NUM];
} MFBMM_PROC1_RESULT_STRUCT, *P_MFBMM_PROC1_RESULT_STRUCT;

typedef struct
{
	MUINT8 bld_num;
    MINT32 img_idx[MAX_BLD_NUM];
    MUINT8* srcImgYUV420[MAX_BLD_NUM];
} MFBMM_PROC2_INFO_STRUCT, *P_MFBMM_PROC2_INFO_STRUCT;

typedef struct
{
    MUINT8* outImgYUV420;
} MFBMM_PROC3_INFO_STRUCT, *P_MFBMM_PROC3_INFO_STRUCT;

typedef struct
{
    MINT32 out_img_width;
    MINT32 out_img_height;
} MFBMM_PROC3_RESULT_STRUCT, *P_MFBMM_PROC3_RESULT_STRUCT;


/*******************************************************************************
*
********************************************************************************/
class MTKMfbmm 
{
public:
    static MTKMfbmm* createInstance(DrvMfbmmObject_e eobject);
    virtual void   destroyInstance() = 0;
    virtual ~MTKMfbmm(){}
    virtual MRESULT MfbmmInit(void* pParaIn, void* pParaOut);
    virtual MRESULT MfbmmReset(void);
    virtual MRESULT MfbmmMain(MFBMM_PROC_ENUM ProcId, void* pParaIn, void* pParaOut);
	virtual MRESULT MfbmmFeatureCtrl(MFBMM_FTCTRL_ENUM FcId, void* pParaIn, void* pParaOut);
private:
    
};

class AppMfbmmTmp : public MTKMfbmm 
{
public:

    static MTKMfbmm* getInstance();
    virtual void destroyInstance();

    AppMfbmmTmp() {}; 
    virtual ~AppMfbmmTmp() {};
};
#endif



