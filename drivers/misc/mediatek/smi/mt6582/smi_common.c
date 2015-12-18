#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/aee.h>
#include <linux/xlog.h>
#include <linux/delay.h>
#include <mach/mt_clkmgr.h>
#include <asm/io.h>

#include <mach/m4u.h>
#include <mach/mt_smi.h>
#include "smi_reg.h"
#include "smi_common.h"


#define SMI_LOG_TAG "SMI"

typedef struct
{
    spinlock_t SMI_lock;
    unsigned long pu4ConcurrencyTable[SMI_BWC_SCEN_CNT];  //one bit represent one module
} SMI_struct;

static SMI_struct g_SMIInfo;

unsigned int gLarbBaseAddr[SMI_LARB_NR] = 
    {LARB0_BASE, LARB1_BASE, LARB2_BASE}; 

char *smi_port_name[][17] = 
{
    {
        "disp_ovl",
        "disp_rdma",
        "disp_wdma",
        "mm_cmdq",
        "mdp_rdma",
        "mdp_wdma",
        "mdp_rot_y",
        "mdp_rot_u",
        "mdp_rot_v",
    },
    {
        "vdec_mc",
        "vdec_pp",
        "vdec_avc_mv",
        "vdec_pred_rd",
        "vdec_pred_wr",
        "vdec_vld",
        "vdec_ppwrap",
    },
    {
        "cam_imgo",
        "cam_img2o",
        "cam_lsci",
        "cam_imgi",
        "cam_esfko",
        "cam_aao",
        "jpgenc_rdma",
        "jpgenc_bsdma",
        "venc_rd_comv",
        "venc_sv_comv",
        "venc_rcpu",
        "venc_rec_frm",
        "venc_ref_luma",
        "venc_ref_chroma",
        "venc_bsdma",
        "venc_cur_luma",
        "venc_cur_chroma",
    },
};

static void initSetting(void);

int larb_clock_on(int larb_id) 
{

    char name[30];
    sprintf(name, "smi+%d", larb_id);  

    switch(larb_id)
    {
        case 0: 
           enable_clock(MT_CG_DISP0_SMI_COMMON, name);
           enable_clock(MT_CG_DISP0_SMI_LARB0, name);
           break;
        case 1:
           enable_clock(MT_CG_DISP0_SMI_COMMON, name);
           enable_clock(MT_CG_VDEC1_LARB, name);
           break;
        case 2: 
           enable_clock(MT_CG_DISP0_SMI_COMMON, name);
           enable_clock(MT_CG_IMAGE_LARB2_SMI, name);
           break;
        default: 
            break;
    }

  return 0;
}

int larb_clock_off(int larb_id) 
{

    char name[30];
    sprintf(name, "smi+%d", larb_id);


    switch(larb_id)
    {
        case 0: 
           disable_clock(MT_CG_DISP0_SMI_LARB0, name);
           disable_clock(MT_CG_DISP0_SMI_COMMON, name);
           break;
        case 1:
           disable_clock(MT_CG_VDEC1_LARB, name);
           disable_clock(MT_CG_DISP0_SMI_COMMON, name);
           break;
        case 2: 
           disable_clock(MT_CG_IMAGE_LARB2_SMI, name);
           disable_clock(MT_CG_DISP0_SMI_COMMON, name);
           break;
        default: 
            break;
    }

    return 0;

}


#define LARB_BACKUP_REG_SIZE 128
//static unsigned int* pLarbRegBackUp[SMI_LARB_NR];
static int g_bInited = 0;
int larb_reg_backup(int larb)
{
/*
    unsigned int* pReg = pLarbRegBackUp[larb];
    int i;
    unsigned int larb_base = gLarbBaseAddr[larb];
    
    *(pReg++) = M4U_ReadReg32(larb_base, SMI_LARB_CON);
    *(pReg++) = M4U_ReadReg32(larb_base, SMI_SHARE_EN);
    *(pReg++) = M4U_ReadReg32(larb_base, SMI_ROUTE_SEL);

    for(i=0; i<3; i++)
    {
        *(pReg++) = M4U_ReadReg32(larb_base, SMI_MAU_ENTR_START(i));
        *(pReg++) = M4U_ReadReg32(larb_base, SMI_MAU_ENTR_END(i));
        *(pReg++) = M4U_ReadReg32(larb_base, SMI_MAU_ENTR_GID(i));
    }
*/
    if(0 == larb)
    {
        g_bInited = 0;
    }

    return 0;
}

int larb_reg_restore(int larb)
{
    unsigned int regval,regval1,regval2;
    unsigned int larb_base = gLarbBaseAddr[larb];

/*
    unsigned int* pReg = pLarbRegBackUp[larb];
    int i;
    
    //warning: larb_con is controlled by set/clr
    regval = *(pReg++);
    M4U_WriteReg32(larb_base, SMI_LARB_CON_CLR, ~(regval));
    M4U_WriteReg32(larb_base, SMI_LARB_CON_SET, (regval));
    
    M4U_WriteReg32(larb_base, SMI_SHARE_EN, *(pReg++) );
    M4U_WriteReg32(larb_base, SMI_ROUTE_SEL, *(pReg++) );

    for(i=0; i<3; i++)
    {
        M4U_WriteReg32(larb_base, SMI_MAU_ENTR_START(i), *(pReg++));
        M4U_WriteReg32(larb_base, SMI_MAU_ENTR_END(i), *(pReg++));
        M4U_WriteReg32(larb_base, SMI_MAU_ENTR_GID(i), *(pReg++));
    }
*/

    //Clock manager enable LARB clock before call back restore already, it will be disabled after restore call back returns
    //Got to enable OSTD before engine starts
    regval = M4U_ReadReg32(larb_base , SMI_LARB_STAT);
    regval1 = M4U_ReadReg32(larb_base , SMI_LARB_MON_BUS_REQ0);
    regval2 = M4U_ReadReg32(larb_base , SMI_LARB_MON_BUS_REQ1);
    if(0 == regval)
    {
        int retry_count = 0;
        
        SMIMSG("Init OSTD for larb_base: 0x%x\n" , larb_base);
        // Write 0x60 = 0xFFFF_FFFF, enable BW limiter
        M4U_WriteReg32(larb_base , 0x60 , 0xffffffff);
        // Polling 0x600 = 0xaaaa        
        for(retry_count= 0; retry_count<64; retry_count++)
        {
            if(M4U_ReadReg32(larb_base , 0x600) == 0xaaaa)
            {
                //Step3.   Once it is found 0x600 == 0xaaaa, we can start to enable outstanding limiter and set outstanding limit
                break;
            }
            SMIMSG("Larb: 0x%x busy : waiting for idle\n" , larb_base);
            udelay(500);
        }

        // Write 0x60 = 0x0, disable BW limiter
        M4U_WriteReg32(larb_base , 0x60 , 0x0);
        // enable ISTD
        M4U_WriteReg32(larb_base , SMI_LARB_OSTD_CTRL_EN , 0xffffffff);
    }
    else
    {
        SMIMSG("Larb%d is busy : 0x%x , port:0x%x,0x%x ,fail to set OSTD\n" , larb , regval , regval1 , regval2);
        smi_dumpDebugMsg();
        SMIERR("DISP_MDP LARB%d OSTD cannot be set:0x%x,port:0x%x,0x%x\n" , larb , regval , regval1 , regval2);
    }

    if(0 == g_bInited)
    {
        initSetting();
        g_bInited = 1;
        SMIMSG("SMI init\n");
    }

    return 0;
}

// callback after larb clock is enabled
void on_larb_power_on(struct larb_monitor *h, int larb_idx)
{
    //M4ULOG("on_larb_power_on(), larb_idx=%d \n", larb_idx);
    larb_reg_restore(larb_idx);
    
    return;
}
// callback before larb clock is disabled
void on_larb_power_off(struct larb_monitor *h, int larb_idx)
{
    //M4ULOG("on_larb_power_off(), larb_idx=%d \n", larb_idx);
    larb_reg_backup(larb_idx);
}

//Make sure clock is on
static void initSetting(void)
{
    M4U_WriteReg32(REG_SMI_M4U_TH , 0 , ((0x2 << 15) + (0x3 << 10) + (0x4 << 5) + 0x5));// 2 non-ultra write, 3 write command , 4 non-ultra read , 5 ultra read
    M4U_WriteReg32(REG_SMI_L1LEN , 0 , 0xB);//Level 1 LARB, apply new outstanding control method, 1/4 bandwidth limiter overshoot control , enable warb channel
    M4U_WriteReg32(REG_SMI_READ_FIFO_TH , 0 , 0xC8F);//total 8 commnads between smi common to M4U, 12 non ultra commands between smi common to M4U, 1 commnads can in write AXI slice for all LARBs

//    M4U_WriteReg32(REG_SMI_L1ARB0 , 0 , 0xC57);//1111/4096 maximum grant counts, soft limiter
    M4U_WriteReg32(REG_SMI_L1ARB0 , 0 , 0);//disable BW limiter on display
    M4U_WriteReg32(REG_SMI_L1ARB1 , 0 , 0x9F7);//503/4096 maximum grant counts, soft limiter
    M4U_WriteReg32(REG_SMI_L1ARB2 , 0 , 0x961);//353/4096 maximum grant counts, soft limiter
    M4U_WriteReg32(REG_SMI_L1ARB3 , 0 , 0xA11FFF);// 4096/4096 maximum grant counts, soft limiter, 8 read 8 write outstanding limit
//    M4U_WriteReg32(REG_SMI_L1ARB3 , 0 , 0xA11D00);// 4096/4096 maximum grant counts, soft limiter, 8 read 8 write outstanding limit

    M4U_WriteReg32(LARB0_BASE , 0x200 , 0x8);//OVL
    M4U_WriteReg32(LARB0_BASE , 0x204 , 0x8);//RDMA
    M4U_WriteReg32(LARB0_BASE , 0x208 , 0x3);//WDMA
    M4U_WriteReg32(LARB0_BASE , 0x20C , 0x1);//CMDQ
    M4U_WriteReg32(LARB0_BASE , 0x210 , 0x2);//MDP_RDMA
    M4U_WriteReg32(LARB0_BASE , 0x214 , 0x1);//MDP_WDMA
    M4U_WriteReg32(LARB0_BASE , 0x218 , 0x4);//MDP_ROTO
    M4U_WriteReg32(LARB0_BASE , 0x21C , 0x2);//MDP_ROTCO
    M4U_WriteReg32(LARB0_BASE , 0x220 , 0x2);//MDP_ROTVO

    M4U_WriteReg32(LARB1_BASE , 0x200 , 0x1);//MC
    M4U_WriteReg32(LARB1_BASE , 0x204 , 0x1);//PP
    M4U_WriteReg32(LARB1_BASE , 0x208 , 0x1);//AVC MV
    M4U_WriteReg32(LARB1_BASE , 0x20C , 0x1);//RD
    M4U_WriteReg32(LARB1_BASE , 0x210 , 0x1);//WR
    M4U_WriteReg32(LARB1_BASE , 0x214 , 0x1);//VLD
    M4U_WriteReg32(LARB1_BASE , 0x218 , 0x1);//PPWRAP

    M4U_WriteReg32(LARB2_BASE , 0x200 , 0x1);//IMGO
    M4U_WriteReg32(LARB2_BASE , 0x204 , 0x1);//IMG2O
    M4U_WriteReg32(LARB2_BASE , 0x208 , 0x1);//LSCI
    M4U_WriteReg32(LARB2_BASE , 0x20C , 0x1);//IMGI
    M4U_WriteReg32(LARB2_BASE , 0x210 , 0x1);//ESFKO
    M4U_WriteReg32(LARB2_BASE , 0x214 , 0x1);//AAO
    M4U_WriteReg32(LARB2_BASE , 0x218 , 0x1);//JPG_RDMA
    M4U_WriteReg32(LARB2_BASE , 0x21C , 0x1);//JPG_BSDMA
    M4U_WriteReg32(LARB2_BASE , 0x220 , 0x1);//VENC_RD_COMV
    M4U_WriteReg32(LARB2_BASE , 0x224 , 0x1);//VENC_SV_COMV
    M4U_WriteReg32(LARB2_BASE , 0x228 , 0x1);//VENC_RCPU
    M4U_WriteReg32(LARB2_BASE , 0x22C , 0x1);//VENC_REC_FRM
    M4U_WriteReg32(LARB2_BASE , 0x230 , 0x1);//VENC_REF_LUMA
    M4U_WriteReg32(LARB2_BASE , 0x234 , 0x1);//VENC_REF_CHROMA
    M4U_WriteReg32(LARB2_BASE , 0x238 , 0x1);//VENC_BSDMA
    M4U_WriteReg32(LARB2_BASE , 0x23C , 0x1);//VENC_CUR_LUMA
    M4U_WriteReg32(LARB2_BASE , 0x240 , 0x1);//VENC_CUR_CHROMA
}

static int smi_bwc_config( MTK_SMI_BWC_CONFIG* p_conf , unsigned long * pu4LocalCnt)
{
    int i;
    unsigned long u4Concurrency = 0;
    MTK_SMI_BWC_SCEN eFinalScen;
    static MTK_SMI_BWC_SCEN ePreviousFinalScen = SMI_BWC_SCEN_CNT;

    if((SMI_BWC_SCEN_CNT <= p_conf->scenario) || (0 > p_conf->scenario))
    {
        SMIERR("Incorrect SMI BWC config : 0x%x, how could this be...\n" , p_conf->scenario);
        return -1;
    }
//Debug - S
//SMIMSG("SMI setTo%d,%s,%d\n" , p_conf->scenario , (p_conf->b_on_off ? "on" : "off") , ePreviousFinalScen);
//Debug - E

    spin_lock(&g_SMIInfo.SMI_lock);

    if(p_conf->b_on_off)
    {
        //turn on certain scenario
        g_SMIInfo.pu4ConcurrencyTable[p_conf->scenario] += 1;

        if(NULL != pu4LocalCnt)
        {
            pu4LocalCnt[p_conf->scenario] += 1;
        }
    }
    else
    {
        //turn off certain scenario
        if(0 == g_SMIInfo.pu4ConcurrencyTable[p_conf->scenario])
        {
            SMIMSG("Too many turning off for global SMI profile:%d,%d\n" , p_conf->scenario , g_SMIInfo.pu4ConcurrencyTable[p_conf->scenario]);
        }
        else
        {
            g_SMIInfo.pu4ConcurrencyTable[p_conf->scenario] -= 1;
        }

        if(NULL != pu4LocalCnt)
        {
            if(0 == pu4LocalCnt[p_conf->scenario])
            {
                SMIMSG("Process : %s did too many turning off for local SMI profile:%d,%d\n" , current->comm ,p_conf->scenario , pu4LocalCnt[p_conf->scenario]);
            }
            else
            {
                pu4LocalCnt[p_conf->scenario] -= 1;
            }
        }
    }

    for(i=0 ; i < SMI_BWC_SCEN_CNT ; i++)
    {
        if(g_SMIInfo.pu4ConcurrencyTable[i])
        {
            u4Concurrency |= (1 << i);
        }
    }

    if((1 << SMI_BWC_SCEN_VR) & u4Concurrency)
    {
        eFinalScen = SMI_BWC_SCEN_VR;
    }
    else if((1 << SMI_BWC_SCEN_VP) & u4Concurrency)
    {
        eFinalScen = SMI_BWC_SCEN_VP;
    }
    else
    {
        eFinalScen = SMI_BWC_SCEN_NORMAL;
    }

    if(ePreviousFinalScen == eFinalScen)
    {
        SMIMSG("Scen equal%d,don't change\n" , eFinalScen);
        spin_unlock(&g_SMIInfo.SMI_lock);
        return 0;
    }
    else
    {
        ePreviousFinalScen = eFinalScen;
    }

    /*turn on larb clock*/
    for( i=0 ; i < SMI_LARB_NR ; i++){
        larb_clock_on(i);
    }

    /*Bandwidth Limiter*/
    switch( eFinalScen )
    {
    case SMI_BWC_SCEN_VP:
        SMIMSG( "[SMI_PROFILE] : %s\n", "SMI_BWC_VP");
#if 1
        M4U_WriteReg32(REG_SMI_M4U_TH , 0 , ((0x2 << 15) + (0x3 << 10) + (0x4 << 5) + 0x5));// 2 non-ultra write, 3 write command , 4 non-ultra read , 5 ultra read
        M4U_WriteReg32(REG_SMI_L1LEN , 0 , 0xB);//Level 1 LARB, apply new outstanding control method, 1/4 bandwidth limiter overshoot control , enable warb channel
        M4U_WriteReg32(REG_SMI_READ_FIFO_TH , 0 , 0xC8F);//total 8 commnads between smi common to M4U, 12 non ultra commands between smi common to M4U, 1 commnads can in write AXI slice for all LARBs

        M4U_WriteReg32(REG_SMI_L1ARB0 , 0 , 0xC57);//1111/4096 maximum grant counts, soft limiter
        M4U_WriteReg32(REG_SMI_L1ARB1 , 0 , 0x9F7);//503/4096 maximum grant counts, soft limiter
        M4U_WriteReg32(REG_SMI_L1ARB2 , 0 , 0x961);//353/4096 maximum grant counts, soft limiter
        M4U_WriteReg32(REG_SMI_L1ARB3 , 0 , 0xA11FFF);

        M4U_WriteReg32(LARB0_BASE , 0x200 , 0x8);//OVL
        M4U_WriteReg32(LARB0_BASE , 0x204 , 0x8);//RDMA
        M4U_WriteReg32(LARB0_BASE , 0x208 , 0x3);//WDMA
        M4U_WriteReg32(LARB0_BASE , 0x20C , 0x1);//CMDQ
        M4U_WriteReg32(LARB0_BASE , 0x210 , 0x2);//MDP_RDMA
        M4U_WriteReg32(LARB0_BASE , 0x214 , 0x1);//MDP_WDMA
        M4U_WriteReg32(LARB0_BASE , 0x218 , 0x4);//MDP_ROTO
        M4U_WriteReg32(LARB0_BASE , 0x21C , 0x2);//MDP_ROTCO
        M4U_WriteReg32(LARB0_BASE , 0x220 , 0x2);//MDP_ROTVO

        M4U_WriteReg32(LARB1_BASE , 0x200 , 0x6);//MC
        M4U_WriteReg32(LARB1_BASE , 0x204 , 0x2);//PP
        M4U_WriteReg32(LARB1_BASE , 0x208 , 0x1);//AVC MV
        M4U_WriteReg32(LARB1_BASE , 0x20C , 0x3);//RD
        M4U_WriteReg32(LARB1_BASE , 0x210 , 0x3);//WR
        M4U_WriteReg32(LARB1_BASE , 0x214 , 0x1);//VLD
        M4U_WriteReg32(LARB1_BASE , 0x218 , 0x1);//PPWRAP

        M4U_WriteReg32(LARB2_BASE , 0x200 , 0x1);//IMGO
        M4U_WriteReg32(LARB2_BASE , 0x204 , 0x1);//IMG2O
        M4U_WriteReg32(LARB2_BASE , 0x208 , 0x1);//LSCI
        M4U_WriteReg32(LARB2_BASE , 0x20C , 0x1);//IMGI
        M4U_WriteReg32(LARB2_BASE , 0x210 , 0x1);//ESFKO
        M4U_WriteReg32(LARB2_BASE , 0x214 , 0x1);//AAO
        M4U_WriteReg32(LARB2_BASE , 0x218 , 0x1);//JPG_RDMA
        M4U_WriteReg32(LARB2_BASE , 0x21C , 0x1);//JPG_BSDMA
        M4U_WriteReg32(LARB2_BASE , 0x220 , 0x2);//VENC_RD_COMV
        M4U_WriteReg32(LARB2_BASE , 0x224 , 0x1);//VENC_SV_COMV
        M4U_WriteReg32(LARB2_BASE , 0x228 , 0x4);//VENC_RCPU
        M4U_WriteReg32(LARB2_BASE , 0x22C , 0x2);//VENC_REC_FRM
        M4U_WriteReg32(LARB2_BASE , 0x230 , 0x2);//VENC_REF_LUMA
        M4U_WriteReg32(LARB2_BASE , 0x234 , 0x1);//VENC_REF_CHROMA
        M4U_WriteReg32(LARB2_BASE , 0x238 , 0x1);//VENC_BSDMA
        M4U_WriteReg32(LARB2_BASE , 0x23C , 0x1);//VENC_CUR_LUMA
        M4U_WriteReg32(LARB2_BASE , 0x240 , 0x1);//VENC_CUR_CHROMA

#endif
        break;

    case SMI_BWC_SCEN_VR:
        SMIMSG( "[SMI_PROFILE] : %s\n", "SMI_BWC_VR");
#if 1
        M4U_WriteReg32(REG_SMI_M4U_TH , 0 , ((0x2 << 15) + (0x3 << 10) + (0x4 << 5) + 0x5));// 2 non-ultra write, 3 write command , 4 non-ultra read , 5 ultra read
        M4U_WriteReg32(REG_SMI_L1LEN , 0 , 0xB);//Level 1 LARB, apply new outstanding control method, 1/4 bandwidth limiter overshoot control , enable warb channel
        M4U_WriteReg32(REG_SMI_READ_FIFO_TH , 0 , 0xC8F);//total 8 commnads between smi common to M4U, 12 non ultra commands between smi common to M4U, 1 commnads can in write AXI slice for all LARBs

        M4U_WriteReg32(REG_SMI_L1ARB0 , 0 , 0xC57);//1111/4096 maximum grant counts, soft limiter
        M4U_WriteReg32(REG_SMI_L1ARB1 , 0 , 0x9F7);//503/4096 maximum grant counts, soft limiter
        M4U_WriteReg32(REG_SMI_L1ARB2 , 0 , 0xD4F);//1359/4096 maximum grant counts, soft limiter
        M4U_WriteReg32(REG_SMI_L1ARB3 , 0 , 0xA11FFF);// 274/4096 maximum grant counts, soft limiter, 2 read 2 write outstanding limit

        M4U_WriteReg32(LARB0_BASE , 0x200 , 0x8);//OVL
        M4U_WriteReg32(LARB0_BASE , 0x204 , 0x8);//RDMA
        M4U_WriteReg32(LARB0_BASE , 0x208 , 0x1);//WDMA
        M4U_WriteReg32(LARB0_BASE , 0x20C , 0x1);//CMDQ
        M4U_WriteReg32(LARB0_BASE , 0x210 , 0x2);//MDP_RDMA
        M4U_WriteReg32(LARB0_BASE , 0x214 , 0x2);//MDP_WDMA
        M4U_WriteReg32(LARB0_BASE , 0x218 , 0x2);//MDP_ROTO
        M4U_WriteReg32(LARB0_BASE , 0x21C , 0x4);//MDP_ROTCO
        M4U_WriteReg32(LARB0_BASE , 0x220 , 0x1);//MDP_ROTVO

        M4U_WriteReg32(LARB1_BASE , 0x200 , 0x1);//MC
        M4U_WriteReg32(LARB1_BASE , 0x204 , 0x1);//PP
        M4U_WriteReg32(LARB1_BASE , 0x208 , 0x1);//AVC MV
        M4U_WriteReg32(LARB1_BASE , 0x20C , 0x1);//RD
        M4U_WriteReg32(LARB1_BASE , 0x210 , 0x1);//WR
        M4U_WriteReg32(LARB1_BASE , 0x214 , 0x1);//VLD
        M4U_WriteReg32(LARB1_BASE , 0x218 , 0x1);//PPWRAP

        M4U_WriteReg32(LARB2_BASE , 0x200 , 0x6);//IMGO
        M4U_WriteReg32(LARB2_BASE , 0x204 , 0x1);//IMG2O
        M4U_WriteReg32(LARB2_BASE , 0x208 , 0x1);//LSCI
        M4U_WriteReg32(LARB2_BASE , 0x20C , 0x4);//IMGI
        M4U_WriteReg32(LARB2_BASE , 0x210 , 0x1);//ESFKO
        M4U_WriteReg32(LARB2_BASE , 0x214 , 0x1);//AAO
        M4U_WriteReg32(LARB2_BASE , 0x218 , 0x1);//JPG_RDMA
        M4U_WriteReg32(LARB2_BASE , 0x21C , 0x1);//JPG_BSDMA
        M4U_WriteReg32(LARB2_BASE , 0x220 , 0x1);//VENC_RD_COMV
        M4U_WriteReg32(LARB2_BASE , 0x224 , 0x1);//VENC_SV_COMV
        M4U_WriteReg32(LARB2_BASE , 0x228 , 0x1);//VENC_RCPU
        M4U_WriteReg32(LARB2_BASE , 0x22C , 0x2);//VENC_REC_FRM
        M4U_WriteReg32(LARB2_BASE , 0x230 , 0x4);//VENC_REF_LUMA
        M4U_WriteReg32(LARB2_BASE , 0x234 , 0x2);//VENC_REF_CHROMA
        M4U_WriteReg32(LARB2_BASE , 0x238 , 0x1);//VENC_BSDMA
        M4U_WriteReg32(LARB2_BASE , 0x23C , 0x2);//VENC_CUR_LUMA
        M4U_WriteReg32(LARB2_BASE , 0x240 , 0x1);//VENC_CUR_CHROMA

#endif
        break;
        
    case SMI_BWC_SCEN_NORMAL:
        SMIMSG( "[SMI_PROFILE] : %s\n", "SMI_BWC_SCEN_NORMAL");

        initSetting();

    default:
        break;
    }

    /*turn off larb clock*/    
    for(i = 0 ; i < SMI_LARB_NR ; i++){
        larb_clock_off(i);
    }

    spin_unlock(&g_SMIInfo.SMI_lock);

    SMIMSG("ScenTo:%d,turn %s,Curr Scen:%d,%d,%d,%d\n" , p_conf->scenario , (p_conf->b_on_off ? "on" : "off") , eFinalScen , 
        g_SMIInfo.pu4ConcurrencyTable[SMI_BWC_SCEN_NORMAL] , g_SMIInfo.pu4ConcurrencyTable[SMI_BWC_SCEN_VR] , g_SMIInfo.pu4ConcurrencyTable[SMI_BWC_SCEN_VP]);

//Debug usage - S
//smi_dumpDebugMsg();
//SMIMSG("Config:%d,%d,%d\n" , eFinalScen , g_SMIInfo.pu4ConcurrencyTable[SMI_BWC_SCEN_NORMAL] , (NULL == pu4LocalCnt ? (-1) : pu4LocalCnt[p_conf->scenario]));
//Debug usage - E

    return 0;
    
}



struct larb_monitor larb_monitor_handler =
{
    .level = LARB_MONITOR_LEVEL_HIGH,
    .backup = on_larb_power_off,
    .restore = on_larb_power_on	
};



int smi_common_init(void)
{
    int i;
/*
    for( i=0 ; i < SMI_LARB_NR ; i++)
    {
        pLarbRegBackUp[i] = (unsigned int*)kmalloc(LARB_BACKUP_REG_SIZE, GFP_KERNEL|__GFP_ZERO);
        if(pLarbRegBackUp[i]==NULL)
        {
        	  SMIERR("pLarbRegBackUp kmalloc fail %d \n", i);
        }  
    }
*/
    /** make sure all larb power is on before we register callback func.
        then, when larb power is first off, default register value will be backed up.
    **/     

    for( i=0 ; i < SMI_LARB_NR ; i++)
    {
        larb_clock_on(i);
    }

    register_larb_monitor(&larb_monitor_handler);
    
    for( i=0 ; i < SMI_LARB_NR ; i++)
    {
        larb_clock_off(i);
    }
    
    return 0;
}

static int smi_open(struct inode *inode, struct file *file)
{
    file->private_data = kmalloc(SMI_BWC_SCEN_CNT*sizeof(unsigned long) , GFP_ATOMIC);

    if(NULL == file->private_data)
    {
        SMIMSG("Not enough entry for DDP open operation\n");
        return -ENOMEM;
    }

    memset(file->private_data , 0 , SMI_BWC_SCEN_CNT*sizeof(unsigned long));

    return 0;
}

static int smi_release(struct inode *inode, struct file *file)
{

#if 0
    unsigned long u4Index = 0 ;
    unsigned long u4AssignCnt = 0;
    unsigned long * pu4Cnt = (unsigned long *)file->private_data;
    MTK_SMI_BWC_CONFIG config;

    for(; u4Index < SMI_BWC_SCEN_CNT ; u4Index += 1)
    {
        if(pu4Cnt[u4Index])
        {
            SMIMSG("Process:%s does not turn off BWC properly , force turn off %d\n" , current->comm , u4Index);
            u4AssignCnt = pu4Cnt[u4Index];
            config.b_on_off = 0;
            config.scenario = (MTK_SMI_BWC_SCEN)u4Index;
            do
            {
                smi_bwc_config( &config , pu4Cnt);
            }
            while(0 < u4AssignCnt);
        }
    }
#endif

    if(NULL != file->private_data)
    {
        kfree(file->private_data);
        file->private_data = NULL;
    }

    return 0;
}

// GMP start 
static MTK_SMI_BWC_MM_INFO g_smi_bwc_mm_info= {
    0, 
    0,
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    0,
    0,
    0
};
    
    void smi_bwc_mm_info_set(int property_id, long val1, long val2){
        SMIMSG("Set BWC INFO:%d, val=[%d,%d]", val1, val2);
        switch(property_id){
            case SMI_BWC_INFO_CON_PROFILE:
                g_smi_bwc_mm_info.concurrent_profile = (int)val1;
                break;
            case SMI_BWC_INFO_SENSOR_SIZE:
                g_smi_bwc_mm_info.sensor_size[0] = val1;
                g_smi_bwc_mm_info.sensor_size[1] = val2;
                break;
            case SMI_BWC_INFO_VIDEO_RECORD_SIZE: 
                g_smi_bwc_mm_info.video_record_size[0]= val1;
                g_smi_bwc_mm_info.video_record_size[1]= val2;
                break;
            case SMI_BWC_INFO_DISP_SIZE:
                g_smi_bwc_mm_info.display_size[0]= val1;
                g_smi_bwc_mm_info.display_size[1]= val2;
                break;
            case SMI_BWC_INFO_TV_OUT_SIZE:
                g_smi_bwc_mm_info.tv_out_size[0]= val1;
                g_smi_bwc_mm_info.tv_out_size[1]= val2;
                break;
            case SMI_BWC_INFO_FPS:
                g_smi_bwc_mm_info.fps = (int)val1;
                break;
            case SMI_BWC_INFO_VIDEO_ENCODE_CODEC:
                g_smi_bwc_mm_info.video_encode_codec = (int)val1;
                break;
            case SMI_BWC_INFO_VIDEO_DECODE_CODEC:  
                g_smi_bwc_mm_info.video_decode_codec = (int)val1;
                break;
        }    
    }

// GMP end
 
static long smi_ioctl( struct file * pFile,
						 unsigned int cmd,
						 unsigned long param)
{
    int ret = 0;

//    unsigned long * pu4Cnt = (unsigned long *)pFile->private_data;

    switch (cmd)
    {
#ifdef __MAU_SPC_ENABLE__ 
        case MTK_CONFIG_MM_MAU:
        {
        	MTK_MAU_CONFIG b;
       		if(copy_from_user(&b, (void __user *)param, sizeof(b)))
        	{
            	SMIERR("copy_from_user failed!");
            	ret = -EFAULT;
        	} else {
                mau_config(&b);
			}
        	return ret;
    	}
        case MTK_IOC_SPC_CONFIG :
        {
            MTK_SPC_CONFIG cfg;
            ret = copy_from_user(&cfg, (void*)param , sizeof(MTK_SPC_CONFIG));
            if(ret)
            {
            	SMIMSG(" SPC_CONFIG, copy_from_user failed: %d\n", ret);
            	return -EFAULT;
            }  

            spc_config(&cfg);

        }
            break;

        case MTK_IOC_SPC_DUMP_REG :
            spc_dump_reg();
            break;


        case MTK_IOC_SPC_DUMP_STA :
            spc_status_check();    	
            break;

        case MTK_IOC_SPC_CMD :
            spc_test(param);
            break;
#endif

        case MTK_IOC_SMI_BWC_CONFIG:
            {
                MTK_SMI_BWC_CONFIG cfg;
                ret = copy_from_user(&cfg, (void*)param , sizeof(MTK_SMI_BWC_CONFIG));
                if(ret)
                {
                    SMIMSG(" SMI_BWC_CONFIG, copy_from_user failed: %d\n", ret);
                    return -EFAULT;
                }  

//                ret = smi_bwc_config( &cfg , pu4Cnt);
                ret = smi_bwc_config( &cfg , NULL);
            
            }
            break;
    // GMP start 
    case MTK_IOC_SMI_BWC_INFO_SET:
        {
            MTK_SMI_BWC_INFO_SET cfg;
            SMIMSG("Handle MTK_IOC_SMI_BWC_INFO_SET request... start");
            ret = copy_from_user(&cfg, (void*)param , sizeof(MTK_SMI_BWC_INFO_SET));
            if(ret)
            {
                SMIMSG(" MTK_IOC_SMI_BWC_INFO_SET, copy_to_user failed: %d\n", ret);
                return -EFAULT;
            }  
            // Set the address to the value assigned by user space program
            smi_bwc_mm_info_set(cfg.property, cfg.value1, cfg.value2);
            SMIMSG("Handle MTK_IOC_SMI_BWC_INFO_SET request... finish");
            break;  
        }
    case MTK_IOC_SMI_BWC_INFO_GET:
        {
            MTK_SMI_BWC_INFO_GET cfg;
            MTK_SMI_BWC_MM_INFO * return_address = NULL;
            
            SMIMSG("Handle MTK_IOC_SMI_BWC_INFO_GET request... start");
            ret = copy_from_user(&cfg, (void*)param , sizeof(MTK_SMI_BWC_INFO_GET));

            if(ret)
            {
                SMIMSG(" MTK_IOC_SMI_BWC_INFO_GET, copy_to_user failed: %d\n", ret);
                return -EFAULT;
            }  

            return_address = (MTK_SMI_BWC_MM_INFO *)cfg.return_address;
            if( return_address != NULL){
                ret = copy_to_user((void*) return_address, (void*)&g_smi_bwc_mm_info, sizeof(MTK_SMI_BWC_MM_INFO));

                if(ret)
                {
                    SMIMSG(" MTK_IOC_SMI_BWC_INFO_GET, copy_to_user failed: %d\n", ret);
                    return -EFAULT;
                }
            }
            SMIMSG("Handle MTK_IOC_SMI_BWC_INFO_GET request... finish");
            break;
        }
     // GMP end   
        
        default:
            return -1;
    }

	return ret;
}


static const struct file_operations smiFops =
{
	.owner = THIS_MODULE,
	.open = smi_open,
	.release = smi_release,
	.unlocked_ioctl = smi_ioctl,
};

static struct cdev * pSmiDev = NULL;
static dev_t smiDevNo = MKDEV(MTK_SMI_MAJOR_NUMBER,0);
static inline int smi_register(void)
{
    if (alloc_chrdev_region(&smiDevNo, 0, 1,"MTK_SMI")){
        SMIERR("Allocate device No. failed");
        return -EAGAIN;
    }
    //Allocate driver
    pSmiDev = cdev_alloc();

    if (NULL == pSmiDev) {
        unregister_chrdev_region(smiDevNo, 1);
        SMIERR("Allocate mem for kobject failed");
        return -ENOMEM;
    }

    //Attatch file operation.
    cdev_init(pSmiDev, &smiFops);
    pSmiDev->owner = THIS_MODULE;

    //Add to system
    if (cdev_add(pSmiDev, smiDevNo, 1)) {
        SMIERR("Attatch file operation failed");
        unregister_chrdev_region(smiDevNo, 1);
        return -EAGAIN;
    }
    return 0;
}


static struct class *pSmiClass = NULL;
static int smi_probe(struct platform_device *pdev)
{
    struct device* smiDevice = NULL;

    if (NULL == pdev) {
        SMIERR("platform data missed");
        return -ENXIO;
    }

    if (smi_register()) {
        dev_err(&pdev->dev,"register char failed\n");
        return -EAGAIN;
    }

    pSmiClass = class_create(THIS_MODULE, "MTK_SMI");
    if (IS_ERR(pSmiClass)) {
        int ret = PTR_ERR(pSmiClass);
        SMIERR("Unable to create class, err = %d", ret);
        return ret;
    }
    smiDevice = device_create(pSmiClass, NULL, smiDevNo, NULL, "MTK_SMI");

    smi_common_init();

#ifdef __MAU_SPC_ENABLE__
    mau_init();

    MTK_SPC_Init(&(pdev->dev));

#endif

    SMI_DBG_Init();

#ifdef __MAU_SPC_ENABLE__
    //init mau to monitor mva 0~0x2ffff & 0x40000000~0xffffffff
#if 0
    {
        MTK_MAU_CONFIG cfg;
        int i;
        for( i=0 ; i < SMI_LARB_NR ; i++)
        {
            cfg.larb = i;
            cfg.entry = 0;
            cfg.port_msk = 0xffffffff;
            cfg.virt = 1;
            cfg.monitor_read = 1;
            cfg.monitor_write = 1;
            cfg.start = 0;
            cfg.end = 0x2ffff;
            mau_config(&cfg);

            cfg.entry = 1;
            cfg.start = 0x40000000;
            cfg.end = 0xffffffff;
            mau_config(&cfg);
        }
    }
#endif 
#endif
    return 0;
}



static int smi_remove(struct platform_device *pdev)
{
    cdev_del(pSmiDev);
    unregister_chrdev_region(smiDevNo, 1);
    device_destroy(pSmiClass, smiDevNo);
    class_destroy(pSmiClass);
    return 0;
}


static int smi_suspend(struct platform_device *pdev, pm_message_t mesg)
{
    return 0;
}

static int smi_resume(struct platform_device *pdev)
{
    return 0;
}

static struct platform_driver smiDrv = {
    .probe	= smi_probe,
    .remove	= smi_remove,
    .suspend= smi_suspend,
    .resume	= smi_resume,
    .driver	= {
    .name	= "MTK_SMI",
    .owner	= THIS_MODULE,
    }
};


static int __init smi_init(void)
{
    spin_lock_init(&g_SMIInfo.SMI_lock);

    memset(g_SMIInfo.pu4ConcurrencyTable , 0 , SMI_BWC_SCEN_CNT*sizeof(unsigned long));

    if(platform_driver_register(&smiDrv)){
        SMIERR("failed to register MAU driver");
        return -ENODEV;
    }
    return 0;
}

static void __exit smi_exit(void)
{
    platform_driver_unregister(&smiDrv);

}

void smi_dumpDebugMsg(void)
{
    unsigned int u4Index;
    unsigned int u4Base;
    unsigned int u4Offset;

    //SMI COMMON dump
    SMIMSG("===SMI common reg dump===\n");
    u4Base = SMI_COMMON_EXT_BASE;
    u4Offset = 0x400;
    SMIMSG("+0x%x=0x%x \n" , u4Offset , M4U_ReadReg32(u4Base , u4Offset));
    u4Offset = 0x404;
    SMIMSG("+0x%x=0x%x \n" , u4Offset , M4U_ReadReg32(u4Base , u4Offset));

    u4Offset = 0x234;
    SMIMSG("+0x%x=0x%x \n" , u4Offset , M4U_ReadReg32(u4Base , u4Offset));
    u4Offset = 0x200;
    SMIMSG("+0x%x=0x%x \n" , u4Offset , M4U_ReadReg32(u4Base , u4Offset));
    u4Offset = 0x204;
    SMIMSG("+0x%x=0x%x \n" , u4Offset , M4U_ReadReg32(u4Base , u4Offset));
    u4Offset = 0x208;
    SMIMSG("+0x%x=0x%x \n" , u4Offset , M4U_ReadReg32(u4Base , u4Offset));
    u4Offset = 0x20C;
    SMIMSG("+0x%x=0x%x \n" , u4Offset , M4U_ReadReg32(u4Base , u4Offset));
    u4Offset = 0x210;
    SMIMSG("+0x%x=0x%x \n" , u4Offset , M4U_ReadReg32(u4Base , u4Offset));
    u4Offset = 0x230;
    SMIMSG("+0x%x=0x%x \n" , u4Offset , M4U_ReadReg32(u4Base , u4Offset));


    //SMI LARB dump
    for( u4Index=0 ; u4Index < SMI_LARB_NR ; u4Index++)
    {
        if(0 == u4Index)
        {
//            if((0 == clock_is_on(MT_CG_DISP0_SMI_LARB0)) || (0 == clock_is_on(MT_CG_DISP0_SMI_COMMON)))
            if(0x3 & M4U_ReadReg32(0xF4000000 , 0x100))
            {
                SMIMSG("===SMI%d is off===\n" , u4Index);
                continue;
            }
        }
        else if(1 == u4Index)
        {
//            if(0 == clock_is_on(MT_CG_VDEC1_LARB))
            if(0x1 & M4U_ReadReg32(0xF6000000 , 0x4))
            {
                SMIMSG("===SMI%d is off===\n" , u4Index);
                continue;
            }
        }
        else if(2 == u4Index)
        {
//            if(0 == clock_is_on(MT_CG_IMAGE_LARB2_SMI))
            if(0x1 & M4U_ReadReg32(0xF5000000 , 0))
            {
                SMIMSG("===SMI%d is off===\n" , u4Index);
                continue;
            }
        }
        else
        {
            continue;
        }

        SMIMSG("===SMI%d reg dump===\n" , u4Index);    
        u4Base = gLarbBaseAddr[u4Index];
        u4Offset = 0;
        SMIMSG("+0x%x=0x%x \n" , u4Offset , M4U_ReadReg32(u4Base , u4Offset));
        u4Offset = 0x10;
        SMIMSG("+0x%x=0x%x \n" , u4Offset , M4U_ReadReg32(u4Base , u4Offset));        
        u4Offset = 0x450;
        SMIMSG("+0x%x=0x%x \n" , u4Offset , M4U_ReadReg32(u4Base , u4Offset));
        u4Offset = 0x454;
        SMIMSG("+0x%x=0x%x \n" , u4Offset , M4U_ReadReg32(u4Base , u4Offset));
        u4Offset = 0x600;
        SMIMSG("+0x%x=0x%x \n" , u4Offset , M4U_ReadReg32(u4Base , u4Offset));
        u4Offset = 0x604;
        SMIMSG("+0x%x=0x%x \n" , u4Offset , M4U_ReadReg32(u4Base , u4Offset));
        u4Offset = 0x610;
        SMIMSG("+0x%x=0x%x \n" , u4Offset , M4U_ReadReg32(u4Base , u4Offset));
        u4Offset = 0x614;
        SMIMSG("+0x%x=0x%x \n" , u4Offset , M4U_ReadReg32(u4Base , u4Offset));
        for(u4Offset = 0x200; u4Offset < 0x244 ; u4Offset += 4)
        {
            SMIMSG("+0x%x=0x%x \n" , u4Offset , M4U_ReadReg32(u4Base , u4Offset));
        }
    }
    
}

// HAL function to notify SMI when engine state is changed
// Don't remove it.
void smi_dynamic_adj_hint_mhl(int mhl_enable)
{
}


void smi_dynamic_adj_hint(unsigned int dsi2smi_total_pixel)
{
}

module_init(smi_init);
module_exit(smi_exit);

MODULE_DESCRIPTION("MTK SMI driver");
MODULE_AUTHOR("K_zhang<k.zhang@mediatek.com>");
MODULE_LICENSE("GPL");

