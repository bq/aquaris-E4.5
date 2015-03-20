/*! \file
    \brief brief description

    Detailed descriptions here.

*/



/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
#ifdef DFT_TAG
#undef DFT_TAG
#endif
#define DFT_TAG         "[WMT-DEV]"


/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

#include "osal_typedef.h"
#include "osal.h"
#include "wmt_dev.h"
#include "wmt_core.h"
#include "wmt_exp.h"
#include "wmt_lib.h"
#include "wmt_conf.h"
#include "psm_core.h"
#include "stp_core.h"
#include "stp_exp.h"
#include "bgw_desense.h"
#include <mach/mtk_wcn_cmb_stub.h>

#if CONSYS_WMT_REG_SUSPEND_CB_ENABLE
#include <linux/device.h>
#include <linux/platform_device.h>
#endif

#define MTK_WMT_VERSION  "SOC Consys WMT Driver - v1.0"
#define MTK_WMT_DATE     "2013/01/20"
#define WMT_DEV_MAJOR 190 // never used number
#define WMT_DEV_NUM 1
#define WMT_DEV_INIT_TO_MS (2 * 1000)

#if CFG_WMT_DBG_SUPPORT
#define WMT_DBG_PROCNAME "driver/wmt_dbg"
#endif

#define WMT_DRIVER_NAME "mtk_stp_wmt"


P_OSAL_EVENT gpRxEvent = NULL;

UINT32 u4RxFlag = 0x0;
static atomic_t gRxCount = ATOMIC_INIT(0);

/* Linux UCHAR device */
static int gWmtMajor = WMT_DEV_MAJOR;
static struct cdev gWmtCdev;
static atomic_t gWmtRefCnt = ATOMIC_INIT(0);
/* WMT driver information */
static UINT8 gLpbkBuf[1024] = {0};
static UINT32 gLpbkBufLog; // George LPBK debug
static INT32 gWmtInitDone = 0;
static wait_queue_head_t gWmtInitWq;

P_WMT_PATCH_INFO pPatchInfo = NULL;
UINT32 pAtchNum = 0;

static UINT8 gEmiBuf[1024*32];

#if CFG_WMT_PROC_FOR_AEE
static struct proc_dir_entry *gWmtAeeEntry = NULL;
#define WMT_AEE_PROCNAME "driver/wmt_aee"
#define WMT_PROC_AEE_SIZE 3072
static UINT32 g_buf_len = 0;
static UINT8 *pBuf = NULL;
static UINT32 passCnt = 0;
#endif

#if CFG_WMT_DBG_SUPPORT
static struct proc_dir_entry *gWmtDbgEntry = NULL;
COEX_BUF gCoexBuf;

#if CONSYS_WMT_REG_SUSPEND_CB_ENABLE
static int mtk_wmt_suspend(struct platform_device *pdev, pm_message_t state);
static int mtk_wmt_resume(struct platform_device *pdev);

static struct platform_device mtk_device_wmt = {
    .name			= "mtk_wmt",
    .id				= -1,
};
static struct platform_driver mtk_wmt_dev_drv = 
{
	.suspend = mtk_wmt_suspend,
    .resume  = mtk_wmt_resume,
    .driver  = {
    	.name    = "mtk_wmt",
        .owner   = THIS_MODULE,
	},
};
#endif
static INT32 wmt_dbg_psm_ctrl(INT32 par1, INT32 par2, INT32 par3);
static INT32 wmt_dbg_quick_sleep_ctrl(INT32 par1, INT32 par2, INT32 par3);
static INT32 wmt_dbg_dsns_ctrl(INT32 par1, INT32 par2, INT32 par3);
static INT32 wmt_dbg_hwver_get(INT32 par1, INT32 par2, INT32 par3);
static INT32 wmt_dbg_inband_rst(INT32 par1, INT32 par2, INT32 par3);
static INT32 wmt_dbg_chip_rst(INT32 par1, INT32 par2, INT32 par3);
static INT32 wmt_dbg_func_ctrl(INT32 par1, INT32 par2, INT32 par3);
static INT32 wmt_dbg_raed_chipid(INT32 par1, INT32 par2, INT32 par3);
static INT32 wmt_dbg_wmt_dbg_level(INT32 par1, INT32 par2, INT32 par3);
static INT32 wmt_dbg_stp_dbg_level(INT32 par1, INT32 par2, INT32 par3);
static INT32 wmt_dbg_reg_read(INT32 par1, INT32 par2, INT32 par3);
static INT32 wmt_dbg_reg_write(INT32 par1, INT32 par2, INT32 par3);
static INT32 wmt_dbg_coex_test(INT32 par1, INT32 par2, INT32 par3);
static INT32 wmt_dbg_assert_test(INT32 par1, INT32 par2, INT32 par3);
static INT32 wmt_dbg_cmd_test_api(ENUM_WMTDRV_CMD_T cmd);
static INT32 wmt_dbg_rst_ctrl(INT32 par1, INT32 par2, INT32 par3);
static INT32 wmt_dbg_ut_test(INT32 par1, INT32 par2, INT32 par3);
static INT32 wmt_dbg_efuse_read(INT32 par1, INT32 par2, INT32 par3);
static INT32 wmt_dbg_efuse_write(INT32 par1, INT32 par2, INT32 par3);
static INT32 wmt_dbg_sdio_ctrl(INT32 par1, INT32 par2, INT32 par3);
static INT32 wmt_dbg_stp_dbg_ctrl(INT32 par1, INT32 par2, INT32 par3);
static INT32 wmt_dbg_stp_dbg_log_ctrl(INT32 par1, INT32 par2, INT32 par3);
static INT32 wmt_dbg_wmt_assert_ctrl(INT32 par1, INT32 par2, INT32 par3);

#if CFG_CORE_INTERNAL_TXRX
static INT32 wmt_dbg_internal_lpbk_test(INT32 par1, INT32 par2, INT32 par3);
#endif
static INT32 wmt_dbg_fwinfor_from_emi(INT32 par1, INT32 par2, INT32 par3);
static INT32 wmt_dbg_set_mcu_clock(INT32 par1, INT32 par2, INT32 par3);
static INT32 wmt_dbg_poll_cpupcr(INT32 par1, INT32 par2, INT32 par3);
#if CONSYS_ENALBE_SET_JTAG
static INT32 wmt_dbg_jtag_flag_ctrl(INT32 par1, INT32 par2, INT32 par3);
#endif
#endif

/*******************************************************************************
*                          F U N C T I O N S
********************************************************************************
*/

#if CONSYS_WMT_REG_SUSPEND_CB_ENABLE
static int mtk_wmt_suspend(struct platform_device *pdev, pm_message_t state)
{
	WMT_INFO_FUNC("++\n");
	wmt_lib_consys_osc_en_ctrl(1);
	WMT_INFO_FUNC("--\n");
	return 0;
}
static int mtk_wmt_resume(struct platform_device *pdev)
{
	WMT_INFO_FUNC("++\n");
	wmt_lib_consys_osc_en_ctrl(0);
	WMT_INFO_FUNC("--\n");
	return 0;
}
#endif

#if CFG_WMT_DBG_SUPPORT

const static WMT_DEV_DBG_FUNC wmt_dev_dbg_func[] =
{
    [0] = wmt_dbg_psm_ctrl,
    [1] = wmt_dbg_quick_sleep_ctrl,
    [2] = wmt_dbg_dsns_ctrl,
    [3] = wmt_dbg_hwver_get,
    [4] = wmt_dbg_assert_test,
    [5] = wmt_dbg_inband_rst,
    [6] = wmt_dbg_chip_rst,
    [7] = wmt_dbg_func_ctrl,
    [8] = wmt_dbg_raed_chipid,
    [9] = wmt_dbg_wmt_dbg_level,
    [0xa] = wmt_dbg_stp_dbg_level,
    [0xb] = wmt_dbg_reg_read,
    [0xc] = wmt_dbg_reg_write,
    [0xd] = wmt_dbg_coex_test, 
    [0xe] = wmt_dbg_rst_ctrl,
    [0xf] = wmt_dbg_ut_test,
    [0x10] = wmt_dbg_efuse_read,
    [0x11] = wmt_dbg_efuse_write,
    [0x12] = wmt_dbg_sdio_ctrl,
    [0x13] = wmt_dbg_stp_dbg_ctrl,
    [0x14] = wmt_dbg_stp_dbg_log_ctrl,
    [0x15] = wmt_dbg_wmt_assert_ctrl,
    [0x16] = wmt_dbg_fwinfor_from_emi,
    [0x17] = wmt_dbg_set_mcu_clock,
    [0x18] = wmt_dbg_poll_cpupcr,
    [0x19] = wmt_dbg_jtag_flag_ctrl,
};

INT32 wmt_dbg_psm_ctrl(INT32 par1, INT32 par2, INT32 par3)
{
#if CFG_WMT_PS_SUPPORT
    if (0 == par2)
    {
        wmt_lib_ps_ctrl(0);
        WMT_INFO_FUNC("disable PSM\n");
    }
    else
    {
        par2 = (1 > par2 || 20000 < par2) ? STP_PSM_IDLE_TIME_SLEEP : par2;
        wmt_lib_ps_set_idle_time(par2);
        wmt_lib_ps_ctrl(1);
        WMT_INFO_FUNC("enable PSM, idle to sleep time = %d ms\n", par2);
    }
#else
    WMT_INFO_FUNC("WMT PS not supported\n");
#endif    
    return 0;
}

INT32 wmt_dbg_quick_sleep_ctrl(INT32 par1, INT32 par2, INT32 par3)
{
#if CFG_WMT_PS_SUPPORT
	UINT32 en_flag = par2;
	wmt_lib_quick_sleep_ctrl(en_flag);
#else
    WMT_INFO_FUNC("WMT PS not supported\n");
#endif    
    return 0;
}

INT32 wmt_dbg_dsns_ctrl(INT32 par1, INT32 par2, INT32 par3)
{
    if (WMTDSNS_FM_DISABLE <= par2 && WMTDSNS_MAX > par2 )
    {
        WMT_INFO_FUNC("DSNS type (%d)\n", par2);
        mtk_wcn_wmt_dsns_ctrl(par2);
    }
    else
    {
        WMT_WARN_FUNC("invalid DSNS type\n");
    }
    return 0;
}

INT32 wmt_dbg_hwver_get(INT32 par1, INT32 par2, INT32 par3)
{
    WMT_INFO_FUNC("query chip version\n");
    mtk_wcn_wmt_hwver_get();
    return 0;
}

INT32 wmt_dbg_assert_test(INT32 par1, INT32 par2, INT32 par3)
{
    if (0 == par3)
    {
    //par2 = 0:  send assert command
    //par2 != 0: send exception command
        return wmt_dbg_cmd_test_api(0 == par2 ? 0 : 1);
    } 
    else if (1 == par3)
    {
        //send noack command
        return wmt_dbg_cmd_test_api(18);
    }
    else if (2 == par3)
    {
        //warn reset test
         return wmt_dbg_cmd_test_api(19);
    }
    else if (3 == par3)
    {
        //firmware trace test
         return wmt_dbg_cmd_test_api(20);
    }
    else
    {
        INT32 sec = 8;
        INT32 times = 0;
        times = par3;
        do{
            WMT_INFO_FUNC("Send Assert Command per 8 secs!!\n");
            wmt_dbg_cmd_test_api(0);
            osal_sleep_ms(sec * 1000);
        }while(--times);
    }
    return 0;
}

INT32 wmt_dbg_cmd_test_api(ENUM_WMTDRV_CMD_T cmd)
{
    
    P_OSAL_OP pOp = NULL;
    MTK_WCN_BOOL bRet = MTK_WCN_BOOL_FALSE;
    P_OSAL_SIGNAL pSignal;
    
    pOp  = wmt_lib_get_free_op();
    if (!pOp ) {
        WMT_WARN_FUNC("get_free_lxop fail\n");
        return MTK_WCN_BOOL_FALSE;
    }

    pSignal = &pOp ->signal;

    pOp ->op.opId = WMT_OPID_CMD_TEST;
    
    pSignal->timeoutValue= MAX_EACH_WMT_CMD;
    /*this test command should be run with usb cable connected, so no host awake is needed*/
    //wmt_lib_host_awake_get();
    switch (cmd)
    {
        case WMTDRV_CMD_ASSERT:
            pOp->op.au4OpData[0] = 0;
        break;
        case WMTDRV_CMD_EXCEPTION:
            pOp->op.au4OpData[0] = 1;
        break;
        case WMTDRV_CMD_NOACK_TEST:
            pOp->op.au4OpData[0] = 3;
        break;
        case WMTDRV_CMD_WARNRST_TEST:
            pOp->op.au4OpData[0] = 4;
        break;
        case WMTDRV_CMD_FWTRACE_TEST:
            pOp->op.au4OpData[0] = 5;
        break;
        default:
            if (WMTDRV_CMD_COEXDBG_00 <= cmd && WMTDRV_CMD_COEXDBG_15 >= cmd)
            {
                pOp->op.au4OpData[0] = 2;
                pOp->op.au4OpData[1] = cmd - 2;
            }
            else
            {
                pOp->op.au4OpData[0] = 0xff;
                pOp->op.au4OpData[1] = 0xff;
            }
            pOp->op.au4OpData[2] = (ULONG)gCoexBuf.buffer;
            pOp->op.au4OpData[3] = osal_sizeof(gCoexBuf.buffer);
        break;
    }
    WMT_INFO_FUNC("CMD_TEST, opid(%d), par(%d, %d)\n", pOp->op.opId, pOp->op.au4OpData[0], pOp->op.au4OpData[1]);
    /*wake up chip first*/
    if (DISABLE_PSM_MONITOR()) {
        WMT_ERR_FUNC("wake up failed\n");
        wmt_lib_put_op_to_free_queue(pOp);
        return -1;
    }
    bRet = wmt_lib_put_act_op(pOp);
    ENABLE_PSM_MONITOR();
    if ((cmd != WMTDRV_CMD_ASSERT) && 
        (cmd != WMTDRV_CMD_EXCEPTION) && 
        (cmd != WMTDRV_CMD_NOACK_TEST) && 
        (cmd != WMTDRV_CMD_WARNRST_TEST) &&
        (cmd != WMTDRV_CMD_FWTRACE_TEST))
    {
        if (MTK_WCN_BOOL_FALSE == bRet)
        {
            gCoexBuf.availSize = 0;
        }
        else
        {
            gCoexBuf.availSize = pOp->op.au4OpData[3];
            WMT_INFO_FUNC("gCoexBuf.availSize = %d\n", gCoexBuf.availSize);
        }
    }
    //wmt_lib_host_awake_put();
    WMT_INFO_FUNC("CMD_TEST, opid (%d), par(%d, %d), ret(%d), result(%s)\n", \
    pOp->op.opId, \
    pOp->op.au4OpData[0], \
    pOp->op.au4OpData[1], \
    bRet, \
    MTK_WCN_BOOL_FALSE == bRet ? "failed" : "succeed"\
    );

    return 0;
}

INT32 wmt_dbg_inband_rst(INT32 par1, INT32 par2, INT32 par3)
{
    if (0 == par2)
    {
        WMT_INFO_FUNC("inband reset test!!\n");
        mtk_wcn_stp_inband_reset();
       }
    else
    {
        WMT_INFO_FUNC("STP context reset in host side!!\n");
        mtk_wcn_stp_flush_context();
    }
    
    return 0;
}

INT32 wmt_dbg_chip_rst(INT32 par1, INT32 par2, INT32 par3)
{
    if (0 == par2)
    {
        if (mtk_wcn_stp_is_ready())
        {
            WMT_INFO_FUNC("whole chip reset test\n");
            wmt_lib_cmb_rst(WMTRSTSRC_RESET_TEST);
        }
        else
        {
            WMT_INFO_FUNC("STP not ready , not to launch whole chip reset test\n");
        }
    }
    else if (1 == par2)
    {
        WMT_INFO_FUNC("chip hardware reset test\n");    
        wmt_lib_hw_rst();
    }
    else
    {
        WMT_INFO_FUNC("chip software reset test\n");
        wmt_lib_sw_rst(1);
    }
    return 0;
}

INT32 wmt_dbg_func_ctrl(INT32 par1, INT32 par2, INT32 par3)
{
    if (WMTDRV_TYPE_WMT > par2 || WMTDRV_TYPE_LPBK == par2)
    {
        if (0 == par3)
        {
            WMT_INFO_FUNC("function off test, type(%d)\n", par2);
            mtk_wcn_wmt_func_off(par2);
        }
        else
        {
            WMT_INFO_FUNC("function on test, type(%d)\n", par2);
            mtk_wcn_wmt_func_on(par2);
        }
    }
    else
    {
        WMT_INFO_FUNC("function ctrl test, invalid type(%d)\n", par2);
    }
    return 0;    
}

INT32 wmt_dbg_raed_chipid(INT32 par1, INT32 par2, INT32 par3)
{
    WMT_INFO_FUNC("chip version = %d\n", wmt_lib_get_icinfo(WMTCHIN_MAPPINGHWVER));
    return 0;
}

INT32 wmt_dbg_wmt_dbg_level(INT32 par1, INT32 par2, INT32 par3)
{
    par2 = (WMT_LOG_ERR <= par2 && WMT_LOG_LOUD >= par2) ? par2 : WMT_LOG_INFO;
    wmt_lib_dbg_level_set(par2);
    WMT_INFO_FUNC("set wmt log level to %d\n", par2);
    return 0;
}

INT32 wmt_dbg_stp_dbg_level(INT32 par1, INT32 par2, INT32 par3)
{
    par2 = (0 <= par2 && 4 >= par2) ? par2 : 2;
    mtk_wcn_stp_dbg_level(par2);
    WMT_INFO_FUNC("set stp log level to %d\n", par2);
    return 0;

}

INT32 wmt_dbg_reg_read(INT32 par1, INT32 par2, INT32 par3)
{
    //par2-->register address
    //par3-->register mask
    UINT32 value = 0x0;
    UINT32 iRet = -1;
#if 0    
    DISABLE_PSM_MONITOR();
    iRet = wmt_core_reg_rw_raw(0, par2, &value, par3);
    ENABLE_PSM_MONITOR();
#endif
    iRet = wmt_lib_reg_rw(0, par2, &value, par3);
    WMT_INFO_FUNC("read combo chip register (0x%08x) with mask (0x%08x) %s, value = 0x%08x\n", \
        par2, \
        par3, \
        iRet != 0 ? "failed" : "succeed", \
        iRet != 0 ?  -1: value\
        );
    return 0;
}

INT32 wmt_dbg_reg_write(INT32 par1, INT32 par2, INT32 par3)
{
    //par2-->register address    
    //par3-->value to set
    UINT32 iRet = -1;
    #if 0
    DISABLE_PSM_MONITOR();
    iRet = wmt_core_reg_rw_raw(1, par2, &par3, 0xffffffff);
    ENABLE_PSM_MONITOR();
    #endif
    iRet = wmt_lib_reg_rw(1, par2, &par3, 0xffffffff);
    WMT_INFO_FUNC("write combo chip register (0x%08x) with value (0x%08x) %s\n", \
        par2, \
        par3, \
        iRet != 0 ? "failed" : "succeed"\
        );
    return 0;
}

INT32 wmt_dbg_efuse_read(INT32 par1, INT32 par2, INT32 par3)
{
    //par2-->efuse address
    //par3-->register mask
    UINT32 value = 0x0;
    UINT32 iRet = -1;

    iRet = wmt_lib_efuse_rw(0, par2, &value, par3);
    WMT_INFO_FUNC("read combo chip efuse (0x%08x) with mask (0x%08x) %s, value = 0x%08x\n", \
        par2, \
        par3, \
        iRet != 0 ? "failed" : "succeed", \
        iRet != 0 ?  -1: value\
        );
    return 0;
}

INT32 wmt_dbg_efuse_write(INT32 par1, INT32 par2, INT32 par3)
{
    //par2-->efuse address    
    //par3-->value to set
    UINT32 iRet = -1;
    iRet = wmt_lib_efuse_rw(1, par2, &par3, 0xffffffff);
    WMT_INFO_FUNC("write combo chip efuse (0x%08x) with value (0x%08x) %s\n", \
        par2, \
        par3, \
        iRet != 0 ? "failed" : "succeed"\
        );
    return 0;
}


INT32 wmt_dbg_sdio_ctrl(INT32 par1, INT32 par2, INT32 par3)
{
/*remove sdio card detect/remove control because of btif is used*/
#if 0
    INT32 iRet = -1;
    iRet = wmt_lib_sdio_ctrl (0 != par2 ? 1 : 0);
    WMT_INFO_FUNC("ctrl SDIO function %s\n", 0 == iRet ? "succeed" : "failed");
#endif
	return 0;
}


INT32 wmt_dbg_stp_dbg_ctrl(INT32 par1, INT32 par2, INT32 par3)
{
    if (1 < par2)
    {
        mtk_wcn_stp_dbg_dump_package();
        return 0;
    }
    WMT_INFO_FUNC("%s stp debug function\n", 0 == par2 ? "disable" : "enable");
    if (0 == par2)
    {
        mtk_wcn_stp_dbg_disable();
    }
    else if (1 == par2)
    {
        mtk_wcn_stp_dbg_enable();
    }
    return 0;
}

INT32 wmt_dbg_stp_dbg_log_ctrl(INT32 par1, INT32 par2, INT32 par3)
{
    mtk_wcn_stp_dbg_log_ctrl(0 != par2 ? 1 : 0);
    return 0;
}



INT32 wmt_dbg_wmt_assert_ctrl(INT32 par1, INT32 par2, INT32 par3)
{
    mtk_wcn_stp_coredump_flag_ctrl(0 != par2 ? 1 : 0);
    return 0;
}

INT32 wmt_dbg_fwinfor_from_emi(INT32 par1, INT32 par2, INT32 par3)
{
	UINT32 offset = 0;
	UINT32 len = 0;
	UINT32 i = 0;
	UINT32 *pAddr = NULL;
    UINT32 cur_idx_pagedtrace;
    static UINT32 prev_idx_pagedtrace = 0;

	offset = par2;
	len = par3;

    osal_memset(&gEmiBuf[0],0,1024*32);
    wmt_lib_get_fwinfor_from_emi(0, offset,&gEmiBuf[0],0x100);

    if(offset == 1){        
        do {
            i = 0;
            pAddr = (UINT32 *) wmt_plat_get_emi_virt_add(0x24);

            cur_idx_pagedtrace = *pAddr;
            
           // printk("\n\n +++++ cur_idx = 0x%08x, pre_idx = 0x%08x ++++\n\n", 
           //         cur_idx_pagedtrace, prev_idx_pagedtrace);
            
            if(cur_idx_pagedtrace > prev_idx_pagedtrace){

                len = cur_idx_pagedtrace - prev_idx_pagedtrace;

            //    printk("\n\n ++ get emi ++ ss\n\n");
                wmt_lib_get_fwinfor_from_emi(1, prev_idx_pagedtrace,&gEmiBuf[0],len);
                
            //    printk("\n\n -- CONNSYS paged trace ascii output --\n\n");	
            	for(i = 0;i < len;i++)
            	{
            		if(i%64 == 0)
            			printk("\n");
            		printk("%c",gEmiBuf[i]);
            	}
                prev_idx_pagedtrace = cur_idx_pagedtrace;
            } 
            
            if(cur_idx_pagedtrace < prev_idx_pagedtrace){
                if(prev_idx_pagedtrace < 0x8000){

                    len = 0x8000 - prev_idx_pagedtrace;

                    wmt_lib_get_fwinfor_from_emi(1, prev_idx_pagedtrace,&gEmiBuf[0],len);                
                    printk("\n\n -- CONNSYS paged trace ascii output (cont...) --\n\n");	
                	for(i = 0;i < len;i++)
                	{
                		if(i%64 == 0)
                			printk("\n");
                		printk("%c",gEmiBuf[i]);
                	}
                    
                    len = cur_idx_pagedtrace;

                    wmt_lib_get_fwinfor_from_emi(1, 0x0 ,&gEmiBuf[0],len);                
                    printk("\n\n -- CONNSYS paged trace ascii output (end) --\n\n");	
                	for(i = 0;i < len;i++)
                	{
                		if(i%64 == 0)
                			printk("\n");
                		printk("%c",gEmiBuf[i]);
                	}
                } else {
                    printk("++ prev_idx_pagedtrace invalid ...++\n\\n");
                }

                prev_idx_pagedtrace = cur_idx_pagedtrace;
            } 
            
            msleep(100);
        }while(1);
    }

    printk("\n\n -- control word -- \n\n");
    for(i = 0;i < 0x100 ;i++)
	{
		if(i%16 == 0){
			printk("\n");
        }
		printk("%02x ",gEmiBuf[i]);
	}

	if(len > 1024*4)
		len = 1024*4;
	
	WMT_INFO_FUNC("get fw infor from emi at offset(0x%x),len(0x%x)\n",offset,len);

	osal_memset(&gEmiBuf[0],0,1024*4);
	wmt_lib_get_fwinfor_from_emi(1, offset,&gEmiBuf[0],len);

	printk("\n\n -- paged trace hex output --\n\n");
	for(i = 0;i < len;i++)
	{
		if(i%16 == 0){
			printk("\n");
        }
		printk("%02x ",gEmiBuf[i]);
	}

	printk("\n\n -- paged trace ascii output --\n\n");	
	for(i = 0;i < len;i++)
	{
		if(i%64 == 0)
			printk("\n");
		printk("%c",gEmiBuf[i]);
	}

	return 0;
}

INT32 wmt_dbg_coex_test(INT32 par1, INT32 par2, INT32 par3)
{
    WMT_INFO_FUNC("coexistance test cmd!!\n");
    return wmt_dbg_cmd_test_api(par2 + WMTDRV_CMD_COEXDBG_00);
}

INT32 wmt_dbg_rst_ctrl(INT32 par1, INT32 par2, INT32 par3)
{
    WMT_INFO_FUNC("%s audo rst\n", 0 == par2 ? "disable" : "enable");
    mtk_wcn_stp_set_auto_rst(0 == par2 ? 0 : 1);
    return 0;
}

static int wmt_dev_dbg_read(char *page, char **start, off_t off, int count, int *eof, void *data){
    INT32 len = 0;

    if(off > 0){
        len = 0;
    } else {
        /*len = sprintf(page, "%d\n", g_psm_enable);*/
        if ( gCoexBuf.availSize <= 0)
        {
            WMT_INFO_FUNC("no data available, please run echo 15 xx > /proc/driver/wmt_psm first\n");
            len = osal_sprintf(page, "no data available, please run echo 15 xx > /proc/driver/wmt_psm first\n");
        }
        else
        {
            INT32 i = 0;
            /*we do not check page buffer, because there are only 100 bytes in g_coex_buf, no reason page buffer is not enough, a bomb is placed here on unexpected condition*/
            for (i = 0; i < gCoexBuf.availSize; i++)
            {
                len += osal_sprintf(page + len, "0x%02x ", gCoexBuf.buffer[i]);
            }
            len += osal_sprintf(page + len, "\n");
        }
    }
    gCoexBuf.availSize = 0;
    return len;
}

INT32 wmt_dbg_ut_test(INT32 par1, INT32 par2, INT32 par3)
{

    INT32 i = 0;
    INT32 j = 0;
    INT32 iRet = 0;
   
    i = 20;          
    while((i--) > 0){
        WMT_INFO_FUNC("#### UT WMT and STP Function On/Off .... %d\n", i);
        j = 10;
        while((j--) > 0){
            WMT_INFO_FUNC("#### BT  On .... (%d, %d) \n", i, j);
            iRet = mtk_wcn_wmt_func_on(WMTDRV_TYPE_BT); 
            if(iRet == MTK_WCN_BOOL_FALSE){
                break;
            }
            WMT_INFO_FUNC("#### GPS On .... (%d, %d) \n", i, j);
            iRet = mtk_wcn_wmt_func_on(WMTDRV_TYPE_GPS); 
            if(iRet == MTK_WCN_BOOL_FALSE){
                break;
            }
            WMT_INFO_FUNC("#### FM  On .... (%d, %d) \n", i, j);
            iRet = mtk_wcn_wmt_func_on(WMTDRV_TYPE_FM);
            if(iRet == MTK_WCN_BOOL_FALSE){
                break;
            }
            WMT_INFO_FUNC("#### WIFI On .... (%d, %d) \n", i, j);
            iRet = mtk_wcn_wmt_func_on(WMTDRV_TYPE_WIFI); 
            if(iRet == MTK_WCN_BOOL_FALSE){
                break;
            }
            WMT_INFO_FUNC("#### BT  Off .... (%d, %d) \n", i, j);
            iRet = mtk_wcn_wmt_func_off(WMTDRV_TYPE_BT); 
            if(iRet == MTK_WCN_BOOL_FALSE){
                break;
            }
            WMT_INFO_FUNC("#### GPS  Off ....(%d, %d) \n", i, j);
            iRet = mtk_wcn_wmt_func_off(WMTDRV_TYPE_GPS); 
            if(iRet == MTK_WCN_BOOL_FALSE){
                break;
            }
            WMT_INFO_FUNC("#### FM  Off .... (%d, %d) \n", i, j);
            iRet = mtk_wcn_wmt_func_off(WMTDRV_TYPE_FM); 
            if(iRet == MTK_WCN_BOOL_FALSE){
                break;
            }
            WMT_INFO_FUNC("#### WIFI  Off ....(%d, %d) \n", i, j);
            iRet = mtk_wcn_wmt_func_off(WMTDRV_TYPE_WIFI);           
            if(iRet == MTK_WCN_BOOL_FALSE){
                break;
            }
        }
        if(iRet == MTK_WCN_BOOL_FALSE){
                break;
        }          
    }
    if(iRet == MTK_WCN_BOOL_FALSE){
        WMT_INFO_FUNC("#### UT FAIL!!\n");
    } else {
        WMT_INFO_FUNC("#### UT PASS!!\n");
    }
    return iRet;        
}

#if CFG_CORE_INTERNAL_TXRX

struct lpbk_package{
	long payload_length;
	unsigned char out_payload[2048];
	unsigned char in_payload[2048];
};

static INT32 wmt_internal_loopback(INT32 count, INT32 max)
{
    int ret = 0;
    int loop;
    int offset;
	struct lpbk_package lpbk_buffer;
	P_OSAL_OP pOp;
	P_OSAL_SIGNAL pSignal = NULL;
    
    for (loop = 0; loop < count; loop++) {
        //<1> init buffer
        osal_memset((void *)&lpbk_buffer, 0, sizeof(struct lpbk_package));
        lpbk_buffer.payload_length = max;
        for (offset = 0; offset < max; offset++) {
            lpbk_buffer.out_payload[offset] = (offset + 1)/*for test use: begin from 1*/  & 0xFF;
        }

		memcpy(&gLpbkBuf[0],&lpbk_buffer.out_payload[0],max);
		
        pOp = wmt_lib_get_free_op();
        if (!pOp) {
            WMT_WARN_FUNC("get_free_lxop fail \n");
            ret = -1;
            break;
        }
        pSignal = &pOp->signal;
        pOp->op.opId = WMT_OPID_LPBK;
        pOp->op.au4OpData[0] = lpbk_buffer.payload_length;    //packet length
        pOp->op.au4OpData[1] = (UINT32)&gLpbkBuf[0];        //packet buffer pointer
        pSignal->timeoutValue = MAX_EACH_WMT_CMD;
        WMT_INFO_FUNC("OPID(%d) type(%d) start\n",
                pOp->op.opId,
                pOp->op.au4OpData[0]);
        if (DISABLE_PSM_MONITOR()) {
            WMT_ERR_FUNC("wake up failed\n");
            wmt_lib_put_op_to_free_queue(pOp);
            ret = -2;
        }
            
        ret = wmt_lib_put_act_op(pOp);
        ENABLE_PSM_MONITOR();
        if (MTK_WCN_BOOL_FALSE == ret) {
            WMT_WARN_FUNC("OPID(%d) type(%d)fail\n",
            pOp->op.opId,
            pOp->op.au4OpData[0]);
            ret = -3;
            break;
        }
        else {
            WMT_INFO_FUNC("OPID(%d) length(%d) ok\n",
            pOp->op.opId, pOp->op.au4OpData[0]);

			memcpy(&lpbk_buffer.in_payload[0],&gLpbkBuf[0],max);
			
            ret = pOp->op.au4OpData[0] ;
        }
        /*<3> compare result*/
        if (memcmp(lpbk_buffer.in_payload, lpbk_buffer.out_payload, lpbk_buffer.payload_length)) {
            WMT_INFO_FUNC("[%s] WMT_TEST_LPBK_CMD payload compare error\n", __FUNCTION__);
			ret = -4;
            break;
        }
        WMT_ERR_FUNC("[%s] exec WMT_TEST_LPBK_CMD succeed(loop = %d, size = %ld) \n", __FUNCTION__, loop, lpbk_buffer.payload_length);

    }
    
    if (loop != count) {
        WMT_ERR_FUNC("fail at loop(%d) buf_length(%d)\n", loop, max);
    }


    return ret;
}

INT32 wmt_dbg_internal_lpbk_test(INT32 par1, INT32 par2, INT32 par3)
{
	UINT32 count;
	UINT32 length;

	count = par1;
	length = par2;

	WMT_INFO_FUNC("count[%d],length[%d]\n",count,length);

	wmt_core_lpbk_do_stp_init();

	wmt_internal_loopback(count,length);

	wmt_core_lpbk_do_stp_deinit();
	return 0;
}
#endif


static INT32 wmt_dbg_set_mcu_clock(INT32 par1, INT32 par2, INT32 par3)
{
    int ret = 0;
	P_OSAL_OP pOp;
	P_OSAL_SIGNAL pSignal = NULL;
	UINT32 kind = 0;

	kind = par2;
	pOp = wmt_lib_get_free_op();
    if (!pOp) {
        WMT_WARN_FUNC("get_free_lxop fail \n");
        return -1;
    }
    pSignal = &pOp->signal;
    pOp->op.opId = WMT_OPID_SET_MCU_CLK;
    pOp->op.au4OpData[0] = kind;
    pSignal->timeoutValue = MAX_EACH_WMT_CMD;
	
    WMT_INFO_FUNC("OPID(%d) kind(%d) start\n",pOp->op.opId,pOp->op.au4OpData[0]);
    if (DISABLE_PSM_MONITOR()) {
        WMT_ERR_FUNC("wake up failed\n");
        wmt_lib_put_op_to_free_queue(pOp);
        return -2;
    }
            
    ret = wmt_lib_put_act_op(pOp);
    ENABLE_PSM_MONITOR();
    if (MTK_WCN_BOOL_FALSE == ret) {
        WMT_WARN_FUNC("OPID(%d) kind(%d)fail(%d)\n",
        pOp->op.opId,
        pOp->op.au4OpData[0],ret);
        return -3;
    }
    else {
        WMT_INFO_FUNC("OPID(%d) kind(%d) ok\n",
        pOp->op.opId, pOp->op.au4OpData[0]);
    }

	return ret;
}
static INT32 wmt_dbg_poll_cpupcr(INT32 par1, INT32 par2, INT32 par3)
{
	UINT32 count = 0;
	UINT16 sleep = 0;
	UINT16 toAee = 0;
	count = par2;
	sleep = (par3 & 0xF0) >> 4;
	toAee = (par3 & 0x0F);

	WMT_INFO_FUNC("polling count[%d],polling sleep[%d],toaee[%d]\n",count,sleep,toAee);
	wmt_lib_poll_cpupcr(count,sleep,toAee);

	return 0;
}
#if CONSYS_ENALBE_SET_JTAG
static INT32 wmt_dbg_jtag_flag_ctrl(INT32 par1, INT32 par2, INT32 par3)
{
	UINT32 en_flag = par2;
	wmt_lib_jtag_flag_set(en_flag);
	return 0;
}
#endif
static int wmt_dev_dbg_write(struct file *file, const char *buffer, unsigned long count, void *data){
    
    CHAR buf[256];
    CHAR *pBuf;
    ULONG len = count;
    INT32 x = 0,y = 0, z=0;
    CHAR *pToken = NULL;
    CHAR *pDelimiter = " \t";

    WMT_INFO_FUNC("write parameter len = %d\n\r", (int)len);
    if(len >= osal_sizeof(buf)){
        WMT_ERR_FUNC("input handling fail!\n");
        len = osal_sizeof(buf) - 1;
        return -1;
    }    
    
    if(copy_from_user(buf, buffer, len)){
        return -EFAULT;
    }
    buf[len] = '\0';
    WMT_INFO_FUNC("write parameter data = %s\n\r", buf);

    pBuf = buf;
    pToken = osal_strsep(&pBuf, pDelimiter);
    x = NULL != pToken ? osal_strtol(pToken, NULL, 16) : 0; 

    pToken = osal_strsep(&pBuf, "\t\n ");
    if(pToken != NULL){
        y = osal_strtol(pToken, NULL, 16);
        WMT_INFO_FUNC("y = 0x%08x \n\r", y);
    } else {
        y = 3000;
         /*efuse, register read write default value*/
        if(0x11 == x || 0x12 == x || 0x13 == x) {
            y = 0x80000000;
        }
    }

    pToken = osal_strsep(&pBuf, "\t\n ");
    if(pToken != NULL){
        z = osal_strtol(pToken, NULL, 16);
    } else {
        z = 10;
        /*efuse, register read write default value*/
        if(0x11 == x || 0x12 == x || 0x13 == x) {
            z = 0xffffffff;
        }
    }
    
    WMT_INFO_FUNC("x(0x%08x), y(0x%08x), z(0x%08x)\n\r", x, y, z);

    if (osal_array_size(wmt_dev_dbg_func) > x && NULL != wmt_dev_dbg_func[x])
    {
        (*wmt_dev_dbg_func[x])(x, y, z);
    }
    else
    {
        WMT_WARN_FUNC("no handler defined for command id(0x%08x)\n\r", x);
    }
    return len;
}

INT32 wmt_dev_dbg_setup(VOID)
{
    gWmtDbgEntry = create_proc_entry(WMT_DBG_PROCNAME, 0664, NULL);
    if(gWmtDbgEntry == NULL){
        WMT_ERR_FUNC("Unable to create /proc entry\n\r");
        return -1;
    }
    gWmtDbgEntry->read_proc = wmt_dev_dbg_read;
    gWmtDbgEntry->write_proc = wmt_dev_dbg_write;
    return 0;
}

INT32 wmt_dev_dbg_remove(VOID)
{
    if (NULL != gWmtDbgEntry)
    {
        remove_proc_entry(WMT_DBG_PROCNAME, NULL);
    }
#if CFG_WMT_PS_SUPPORT
    wmt_lib_ps_deinit();
#endif
    return 0;
}
#endif

#if CFG_WMT_PROC_FOR_AEE

static int wmt_dev_proc_for_aee_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	UINT32 len = 0;

	WMT_INFO_FUNC("wmt-dev:wmt for aee page(%p)off(%d)count(%d)\n",page,off,count);

	if(off == 0)
	{
		pBuf = wmt_lib_get_cpupcr_xml_format(&len);
		g_buf_len = len;
		
		/*pass 3k buffer for each proc read*/
		passCnt = g_buf_len / WMT_PROC_AEE_SIZE;
		passCnt = (g_buf_len % WMT_PROC_AEE_SIZE) ? (passCnt + 1) : passCnt;
		WMT_INFO_FUNC("wmt_dev:wmt for aee buffer len(%d)passCnt(%d)\n",g_buf_len,passCnt);
	}

	if(passCnt)
	{
		if(g_buf_len > WMT_PROC_AEE_SIZE)
		{
			osal_memcpy(page,pBuf,WMT_PROC_AEE_SIZE);
			*start += WMT_PROC_AEE_SIZE;
			g_buf_len -= WMT_PROC_AEE_SIZE;
			pBuf += WMT_PROC_AEE_SIZE;
			WMT_INFO_FUNC("wmt_dev:after read,wmt for aee buffer len(%d)\n",g_buf_len);
			*eof = 1;
			passCnt--;
			return WMT_PROC_AEE_SIZE;
		}
		else
		{
			osal_memcpy(page,pBuf,g_buf_len);
			*start += g_buf_len;
			len = g_buf_len;
			g_buf_len = 0;
			*eof = 1;
			passCnt--;
			pBuf += len;
			return len;
		}
	}

	return 0;

}
static int wmt_dev_proc_for_aee_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	return 0;
}
INT32 wmt_dev_proc_for_aee_setup(VOID)
{
    gWmtAeeEntry= create_proc_entry(WMT_AEE_PROCNAME, 0664, NULL);
    if(gWmtAeeEntry == NULL){
        WMT_ERR_FUNC("Unable to create / wmt_aee proc entry\n\r");
        return -1;
    }
    gWmtAeeEntry->read_proc = wmt_dev_proc_for_aee_read;
    gWmtAeeEntry->write_proc = wmt_dev_proc_for_aee_write;
    return 0;
}

INT32 wmt_dev_proc_for_aee_remove(VOID)
{
    if (NULL != gWmtAeeEntry)
    {
        remove_proc_entry(WMT_AEE_PROCNAME, NULL);
    }
    return 0;
}
#endif

VOID wmt_dev_rx_event_cb (VOID)
{
    u4RxFlag = 1;
    atomic_inc(&gRxCount);
    if (NULL != gpRxEvent) {
        //u4RxFlag = 1;
        //atomic_inc(&gRxCount);
        wake_up_interruptible(&gpRxEvent->waitQueue);
    }
    else {
        //WMT_ERR_FUNC("null gpRxEvent, flush rx!\n");
       // wmt_lib_flush_rx();
    }
}


INT32 wmt_dev_rx_timeout (P_OSAL_EVENT pEvent)
{

    UINT32 ms = pEvent->timeoutValue;
    LONG lRet = 0;
    gpRxEvent = pEvent;
    if (0 != ms)
    {
        lRet = wait_event_interruptible_timeout(gpRxEvent->waitQueue,  0 != u4RxFlag, msecs_to_jiffies(ms));
    }
    else
    {
        lRet = wait_event_interruptible(gpRxEvent->waitQueue,  u4RxFlag != 0);
    }
    u4RxFlag = 0;
//    gpRxEvent = NULL;
    if (atomic_dec_return(&gRxCount)) {
        WMT_ERR_FUNC("gRxCount != 0 (%d), reset it!\n", atomic_read(&gRxCount));
        atomic_set(&gRxCount, 0);
    }

    return lRet;
}

INT32 wmt_dev_read_file (
    UCHAR *pName,
    const u8 **ppBufPtr,
    INT32 offset,
    INT32 padSzBuf
    )
{
    INT32 iRet = -1;
    struct file *fd;
    //ssize_t iRet;
    INT32 file_len;
    INT32 read_len;
    void *pBuf;

    //struct cred *cred = get_task_cred(current);
    const struct cred *cred = get_current_cred();

    if (!ppBufPtr ) {
        WMT_ERR_FUNC("invalid ppBufptr!\n");
        return -1;
    }
    *ppBufPtr = NULL;

    fd = filp_open(pName, O_RDONLY, 0);
    if (!fd || IS_ERR(fd) || !fd->f_op || !fd->f_op->read) {
        WMT_ERR_FUNC("failed to open or read!(0x%p, %d, %d, %d)\n", fd, PTR_ERR(fd),cred->fsuid, cred->fsgid);
        if(IS_ERR(fd))
        	WMT_ERR_FUNC("error code:%d\n",PTR_ERR(fd));
        return -1;
    }

    file_len = fd->f_path.dentry->d_inode->i_size;
    pBuf = vmalloc((file_len + BCNT_PATCH_BUF_HEADROOM + 3) & ~0x3UL);
    if (!pBuf) {
        WMT_ERR_FUNC("failed to vmalloc(%d)\n", (INT32)((file_len + 3) & ~0x3UL));
        goto read_file_done;
    }

    do {
        if (fd->f_pos != offset) {
            if (fd->f_op->llseek) {
                if (fd->f_op->llseek(fd, offset, 0) != offset) {
                    WMT_ERR_FUNC("failed to seek!!\n");
                    goto read_file_done;
                }
            }
            else {
                fd->f_pos = offset;
            }
        }

        read_len = fd->f_op->read(fd, pBuf + padSzBuf, file_len, &fd->f_pos);
        if (read_len != file_len) {
            WMT_WARN_FUNC("read abnormal: read_len(%d), file_len(%d)\n", read_len, file_len);
        }
    } while (false);

    iRet = 0;
    *ppBufPtr = pBuf;

read_file_done:
    if (iRet) {
        if (pBuf) {
            vfree(pBuf);
        }
    }

    filp_close(fd, NULL);

    return (iRet) ? iRet : read_len;
}

// TODO: [ChangeFeature][George] refine this function name for general filesystem read operation, not patch only.
INT32 wmt_dev_patch_get (
    UCHAR *pPatchName,
    osal_firmware **ppPatch,
    INT32 padSzBuf
    )
{
    INT32 iRet = -1;
    osal_firmware *pfw;
    uid_t orig_uid;
    gid_t orig_gid;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29))
    //struct cred *cred = get_task_cred(current);
    struct cred *cred = (struct cred *)get_current_cred();
#endif

    mm_segment_t orig_fs = get_fs();

    if (*ppPatch) {
        WMT_WARN_FUNC("f/w patch already exists \n");
        if ((*ppPatch)->data) {
            vfree((*ppPatch)->data);
        }
        kfree(*ppPatch);
        *ppPatch = NULL;
    }

    if (!osal_strlen(pPatchName)) {
        WMT_ERR_FUNC("empty f/w name\n");
        osal_assert((osal_strlen(pPatchName) > 0));
        return -1;
    }

    pfw = kzalloc(sizeof(osal_firmware), /*GFP_KERNEL*/GFP_ATOMIC);
    if (!pfw) {
        WMT_ERR_FUNC("kzalloc(%d) fail\n", sizeof(osal_firmware));
        return -2;
    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29))
    orig_uid = cred->fsuid;
    orig_gid = cred->fsgid;
    cred->fsuid = cred->fsgid = 0;
#else
    orig_uid = current->fsuid;
    orig_gid = current->fsgid;
    current->fsuid = current->fsgid = 0;
#endif

    set_fs(get_ds());

    /* load patch file from fs */
    iRet = wmt_dev_read_file(pPatchName, &pfw->data, 0, padSzBuf);
    set_fs(orig_fs);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29))
    cred->fsuid = orig_uid;
    cred->fsgid = orig_gid;
#else
    current->fsuid = orig_uid;
    current->fsgid = orig_gid;
#endif

    if (iRet > 0) {
        pfw->size = iRet;
        *ppPatch = pfw;
        WMT_DBG_FUNC("load (%s) to addr(0x%p) success\n", pPatchName, pfw->data);
        return 0;
    }
    else {
        kfree(pfw);
        *ppPatch = NULL;
        WMT_ERR_FUNC("load file (%s) fail, iRet(%d) \n", pPatchName, iRet);
        return -1;
    }
}


INT32 wmt_dev_patch_put(osal_firmware **ppPatch)
{
    if (NULL != *ppPatch ) {
        if ((*ppPatch)->data) {
            vfree((*ppPatch)->data);
        }
        kfree(*ppPatch);
        *ppPatch = NULL;
    }
    return 0;
}


VOID wmt_dev_patch_info_free(VOID)
{
	if (pPatchInfo) {
		kfree(pPatchInfo);
		pPatchInfo = NULL;
	}
}


MTK_WCN_BOOL wmt_dev_is_file_exist(UCHAR *pFileName)
{
    struct file *fd = NULL;
    //ssize_t iRet;
    INT32 fileLen = -1;
    const struct cred *cred = get_current_cred();
    if(pFileName == NULL)
    {
        WMT_ERR_FUNC("invalid file name pointer(%p)\n", pFileName);
        return MTK_WCN_BOOL_FALSE;
    }
    if (osal_strlen(pFileName) < osal_strlen(defaultPatchName))
    {
        WMT_ERR_FUNC("invalid file name(%s)\n", pFileName);
        return MTK_WCN_BOOL_FALSE;
    }


    //struct cred *cred = get_task_cred(current);

    fd = filp_open(pFileName, O_RDONLY, 0);
    if (!fd || IS_ERR(fd) || !fd->f_op || !fd->f_op->read) {
        WMT_ERR_FUNC("failed to open or read(%s)!(0x%p, %d, %d)\n", pFileName, fd, cred->fsuid, cred->fsgid);
        return MTK_WCN_BOOL_FALSE;
    }
    fileLen = fd->f_path.dentry->d_inode->i_size;
    filp_close(fd, NULL);
    fd = NULL;
    if(fileLen <= 0)
    {
        WMT_ERR_FUNC("invalid file(%s), length(%d)\n", pFileName, fileLen);
        return MTK_WCN_BOOL_FALSE;
    }
    WMT_ERR_FUNC("valid file(%s), length(%d)\n", pFileName, fileLen);
    return true;

}


//static unsigned long count_last_access_sdio = 0;    
static unsigned long count_last_access_btif = 0;
static unsigned long jiffies_last_poll = 0;

#if 0
static INT32 wmt_dev_tra_sdio_update(void)
{          
    count_last_access_sdio += 1;  
    //WMT_INFO_FUNC("jiffies_last_access_sdio: jiffies = %ul\n", jiffies);

    return 0;
}
#endif

extern INT32 wmt_dev_tra_bitf_update(void)
{          
    count_last_access_btif += 1;  
    //WMT_INFO_FUNC("jiffies_last_access_btif: jiffies = %ul\n", jiffies);

    return 0;
}

static UINT32 wmt_dev_tra_ahb_poll(void)
{    
    #define TIME_THRESHOLD_TO_TEMP_QUERY 3000 
    #define COUNT_THRESHOLD_TO_TEMP_QUERY 200

    unsigned long ahb_during_count = 0;
    unsigned long poll_during_time = 0;

    if(jiffies > jiffies_last_poll)
    {
        poll_during_time = jiffies - jiffies_last_poll;
    } 
    else 
    {
        poll_during_time = 0xffffffff;
    }

   WMT_DBG_FUNC("**jiffies_to_mesecs(0xffffffff) = %lu\n", 
            jiffies_to_msecs(0xffffffff));

    if(jiffies_to_msecs(poll_during_time) < TIME_THRESHOLD_TO_TEMP_QUERY)
    {
        WMT_DBG_FUNC("**poll_during_time = %lu < %lu, not to query\n", 
            jiffies_to_msecs(poll_during_time), TIME_THRESHOLD_TO_TEMP_QUERY);
        return -1;
    } 
      
    //ahb_during_count = count_last_access_sdio;
    if(NULL == mtk_wcn_wlan_bus_tx_cnt)
    {
    	WMT_ERR_FUNC("WMT-DEV:mtk_wcn_wlan_bus_tx_cnt null pointer\n");
		return -1;
    }else{
    	ahb_during_count = (*mtk_wcn_wlan_bus_tx_cnt)();
    }
	
    if(ahb_during_count < COUNT_THRESHOLD_TO_TEMP_QUERY)
    {
        WMT_DBG_FUNC("**ahb_during_count = %lu < %lu, not to query\n", 
            ahb_during_count, COUNT_THRESHOLD_TO_TEMP_QUERY);
        return -2;
    }

	if(NULL == mtk_wcn_wlan_bus_tx_cnt_clr)
	{
		WMT_ERR_FUNC("WMT-DEV:mtk_wcn_wlan_bus_tx_cnt_clr null pointer\n");
		return -3;
	}else{
		(*mtk_wcn_wlan_bus_tx_cnt_clr)();
	}
    //count_last_access_sdio = 0;
    jiffies_last_poll = jiffies;

    WMT_INFO_FUNC("**poll_during_time = %lu > %lu, ahb_during_count = %lu > %lu, query\n", 
            jiffies_to_msecs(poll_during_time), TIME_THRESHOLD_TO_TEMP_QUERY,
            jiffies_to_msecs(ahb_during_count) , COUNT_THRESHOLD_TO_TEMP_QUERY);

    return 0;
}


LONG wmt_dev_tm_temp_query(void)
{
    #define HISTORY_NUM       5
    #define TEMP_THRESHOLD   65
    #define REFRESH_TIME    300 //sec
    
    static INT32 temp_table[HISTORY_NUM] = {99}; //not query yet.
    static INT32 idx_temp_table = 0;
    static struct timeval query_time, now_time;

    INT8  query_cond = 0;
    INT32 current_temp = 0;
    INT32 index = 0;
	LONG return_temp = 0;
    //Query condition 1:
    // If we have the high temperature records on the past, we continue to query/monitor 
    // the real temperature until cooling
    for(index = 0; index < HISTORY_NUM ; index++)
    {
       if(temp_table[index] >= TEMP_THRESHOLD)
       {
            query_cond = 1;
            WMT_INFO_FUNC("temperature table is still intial value, we should query temp temperature..\n");
       }            
    }

    do_gettimeofday(&now_time);
#if 1
    // Query condition 2:
    // Moniter the ahb bus activity to decide if we have the need to query temperature.
    if(!query_cond)
    {
        if( wmt_dev_tra_ahb_poll()==0)
        {
            query_cond = 1;
            WMT_INFO_FUNC("ahb traffic , we must query temperature..\n");
        }
        else
        {
            WMT_DBG_FUNC("ahb idle traffic ....\n");
        }

        //only WIFI tx power might make temperature varies largely
        #if 0
        if(!query_cond)
        {
            last_access_time = wmt_dev_tra_uart_poll();
            if( jiffies_to_msecs(last_access_time) < TIME_THRESHOLD_TO_TEMP_QUERY)
            {
                query_cond = 1;
                WMT_DBG_FUNC("uart busy traffic , we must query temperature..\n");
            }
            else
            {
                WMT_DBG_FUNC("uart still idle traffic , we don't query temp temperature..\n");
            }
        }
        #endif
    }
 #endif   
    // Query condition 3:
    // If the query time exceeds the a certain of period, refresh temp table.
    //
    if(!query_cond)
    {
        if( (now_time.tv_sec < query_time.tv_sec) || //time overflow, we refresh temp table again for simplicity!
            ((now_time.tv_sec > query_time.tv_sec) && 
            (now_time.tv_sec - query_time.tv_sec) > REFRESH_TIME))
        {               
            query_cond = 1;

            WMT_INFO_FUNC("It is long time (> %d sec) not to query, we must query temp temperature..\n", REFRESH_TIME);
            for (index = 0; index < HISTORY_NUM ; index++)
            {
                temp_table[index] = 99;                
            }
        }
    }
        
    if(query_cond)
    {
        // update the temperature record
        mtk_wcn_wmt_therm_ctrl(WMTTHERM_ENABLE);
        current_temp = mtk_wcn_wmt_therm_ctrl(WMTTHERM_READ);
        mtk_wcn_wmt_therm_ctrl(WMTTHERM_DISABLE);
        wmt_lib_notify_stp_sleep();
        idx_temp_table = (idx_temp_table + 1) % HISTORY_NUM;
        temp_table[idx_temp_table] = current_temp;
        do_gettimeofday(&query_time);

        WMT_INFO_FUNC("[Thermal] current_temp = 0x%x \n", (current_temp & 0xFF));
    }
    else
    {
        current_temp = temp_table[idx_temp_table];
        idx_temp_table = (idx_temp_table + 1) % HISTORY_NUM;
        temp_table[idx_temp_table] = current_temp;             
    }

    //
    // Dump information
    //    
    WMT_DBG_FUNC("[Thermal] idx_temp_table = %d \n", idx_temp_table);
    WMT_DBG_FUNC("[Thermal] now.time = %d, query.time = %d, REFRESH_TIME = %d\n", now_time.tv_sec, query_time.tv_sec, REFRESH_TIME);

    WMT_DBG_FUNC("[0] = %d, [1] = %d, [2] = %d, [3] = %d, [4] = %d \n----\n", 
        temp_table[0], temp_table[1], temp_table[2], temp_table[3], temp_table[4]);

	return_temp = ((current_temp & 0x80) == 0x0)? current_temp :(-1)*(current_temp & 0x7f);
	
    return return_temp;
}



ssize_t
WMT_write (
    struct file *filp,
    const char __user *buf,
    size_t count,
    loff_t *f_pos
    )
{
    INT32 iRet = 0;
    UCHAR wrBuf[NAME_MAX+1] = {0};
    INT32 copySize = (count < NAME_MAX) ? count : NAME_MAX;

    WMT_LOUD_FUNC("count:%d copySize:%d\n", count, copySize);

    if (copySize > 0) {
        if (copy_from_user(wrBuf, buf, copySize)) {
            iRet = -EFAULT;
            goto write_done;
        }
        iRet = copySize;
        wrBuf[NAME_MAX] = '\0';

        if (!strncasecmp(wrBuf, "ok", NAME_MAX)) {
            WMT_DBG_FUNC("resp str ok\n");
            //pWmtDevCtx->cmd_result = 0;
            wmt_lib_trigger_cmd_signal(0);
        }
        else {
            WMT_WARN_FUNC("warning resp str (%s)\n", wrBuf);
            //pWmtDevCtx->cmd_result = -1;
            wmt_lib_trigger_cmd_signal(-1);
        }
        //complete(&pWmtDevCtx->cmd_comp);

    }

write_done:
    return iRet;
}

ssize_t
WMT_read (
    struct file *filp,
    char __user *buf,
    size_t count,
    loff_t *f_pos
    )
{
    INT32 iRet = 0;
    PUCHAR pCmd = NULL;
    UINT32 cmdLen = 0;
    pCmd = wmt_lib_get_cmd();

    if (pCmd != NULL)
    {
        cmdLen = osal_strlen(pCmd) < NAME_MAX ? osal_strlen(pCmd) : NAME_MAX;
        WMT_DBG_FUNC("cmd str(%s)\n", pCmd);
        if (copy_to_user(buf, pCmd, cmdLen)) {
            iRet = -EFAULT;
        }
        else
        {
            iRet = cmdLen;
        }
    }
#if 0
    if (test_and_clear_bit(WMT_STAT_CMD, &pWmtDevCtx->state)) {
        iRet = osal_strlen(localBuf) < NAME_MAX ? osal_strlen(localBuf) : NAME_MAX;
        // we got something from STP driver
        WMT_DBG_FUNC("copy cmd to user by read:%s\n", localBuf);
        if (copy_to_user(buf, localBuf, iRet)) {
            iRet = -EFAULT;
            goto read_done;
        }
    }
#endif
    return iRet;
}

unsigned int WMT_poll(struct file *filp, poll_table *wait)
{
    UINT32 mask = 0;
    P_OSAL_EVENT pEvent = wmt_lib_get_cmd_event();

    poll_wait(filp, &pEvent->waitQueue,  wait);
    /* empty let select sleep */
    if (MTK_WCN_BOOL_TRUE == wmt_lib_get_cmd_status())
    {
        mask |= POLLIN | POLLRDNORM;  /* readable */
    }
#if 0
    if (test_bit(WMT_STAT_CMD, &pWmtDevCtx->state)) {
        mask |= POLLIN | POLLRDNORM;  /* readable */
    }
#endif
    mask |= POLLOUT | POLLWRNORM; /* writable */
    return mask;
}

//INT32 WMT_ioctl(struct inode *inode, struct file *filp, UINT32 cmd, unsigned long arg)
long
WMT_unlocked_ioctl (
    struct file *filp,
    unsigned int cmd,
    unsigned long arg
    )
{
#define WMT_IOC_MAGIC        0xa0
#define WMT_IOCTL_SET_PATCH_NAME	 	_IOW(WMT_IOC_MAGIC,4,char*)
#define WMT_IOCTL_SET_STP_MODE		 	_IOW(WMT_IOC_MAGIC,5,int)
#define WMT_IOCTL_FUNC_ONOFF_CTRL		_IOW(WMT_IOC_MAGIC,6,int)
#define WMT_IOCTL_LPBK_POWER_CTRL		_IOW(WMT_IOC_MAGIC,7,int)
#define WMT_IOCTL_LPBK_TEST				_IOWR(WMT_IOC_MAGIC,8,char*)
#define WMT_IOCTL_GET_CHIP_INFO			_IOR(WMT_IOC_MAGIC,12,int)
#define WMT_IOCTL_SET_LAUNCHER_KILL		_IOW(WMT_IOC_MAGIC,13,int)
#define WMT_IOCTL_SET_PATCH_NUM			_IOW(WMT_IOC_MAGIC,14,int)
#define WMT_IOCTL_SET_PATCH_INFO		_IOW(WMT_IOC_MAGIC,15,char*)
#define WMT_IOCTL_PORT_NAME       		_IOWR(WMT_IOC_MAGIC, 20, char*)
#define WMT_IOCTL_WMT_CFG_NAME     		_IOWR(WMT_IOC_MAGIC, 21, char*)
#define WMT_IOCTL_WMT_QUERY_CHIPID     	_IOR(WMT_IOC_MAGIC, 22, int)
#define WMT_IOCTL_WMT_TELL_CHIPID     	_IOW(WMT_IOC_MAGIC, 23, int)
#define WMT_IOCTL_WMT_COREDUMP_CTRL     _IOW(WMT_IOC_MAGIC, 24, int)
#define WMT_IOCTL_SEND_BGW_DS_CMD		_IOW(WMT_IOC_MAGIC,25,char*)
#define WMT_IOCTL_ADIE_LPBK_TEST		_IOWR(WMT_IOC_MAGIC,26,char*)


    INT32 iRet = 0;
    UCHAR pBuffer[NAME_MAX + 1];
    WMT_DBG_FUNC("cmd (%u), arg (0x%lx)\n", cmd, arg);
    switch(cmd) {
    case WMT_IOCTL_SET_PATCH_NAME: /* patch location */
        {
            
            if (copy_from_user(pBuffer, (void *)arg, NAME_MAX)) {
                iRet = -EFAULT;
                break;
            }
            pBuffer[NAME_MAX] = '\0';
            wmt_lib_set_patch_name(pBuffer);
        }
        break;

    case WMT_IOCTL_SET_STP_MODE: /* stp/hif/fm mode */

        /* set hif conf */
        do {
            P_OSAL_OP pOp;
            MTK_WCN_BOOL bRet;
            P_OSAL_SIGNAL pSignal = NULL;
            P_WMT_HIF_CONF pHif = NULL;

            iRet = wmt_lib_set_hif(arg);
            if (0 != iRet)
            {
                WMT_INFO_FUNC("wmt_lib_set_hif fail\n");
                break;
            }

            pOp = wmt_lib_get_free_op();
            if (!pOp) {
                WMT_INFO_FUNC("get_free_lxop fail\n");
                break;
            }
            pSignal = &pOp->signal;
            pOp->op.opId = WMT_OPID_HIF_CONF;

            pHif = wmt_lib_get_hif();

            osal_memcpy(&pOp->op.au4OpData[0], pHif, sizeof(WMT_HIF_CONF));
            pOp->op.u4InfoBit = WMT_OP_HIF_BIT;
            pSignal->timeoutValue = 0;

            bRet = wmt_lib_put_act_op(pOp);
            WMT_DBG_FUNC("WMT_OPID_HIF_CONF result(%d) \n", bRet);
            iRet = (MTK_WCN_BOOL_FALSE == bRet) ? -EFAULT : 0;
        } while (0);

        break;

    case WMT_IOCTL_FUNC_ONOFF_CTRL: /* test turn on/off func */

        do {
            MTK_WCN_BOOL bRet = MTK_WCN_BOOL_FALSE;
            if (arg & 0x80000000)
            {
                bRet = mtk_wcn_wmt_func_on(arg & 0xF);
            }
            else
            {
                bRet = mtk_wcn_wmt_func_off(arg & 0xF);
            }
            iRet = (MTK_WCN_BOOL_FALSE == bRet) ? -EFAULT : 0;
         } while (0);

        break;

        case WMT_IOCTL_LPBK_POWER_CTRL:
        /*switch Loopback function on/off
                  arg:     bit0 = 1:turn loopback function on
                  bit0 = 0:turn loopback function off
                */
        do{
            MTK_WCN_BOOL bRet = MTK_WCN_BOOL_FALSE;
            if (arg & 0x01)
            {
                bRet = mtk_wcn_wmt_func_on(WMTDRV_TYPE_LPBK);
            }
            else
            {
                bRet = mtk_wcn_wmt_func_off(WMTDRV_TYPE_LPBK);
            }
            iRet = (MTK_WCN_BOOL_FALSE == bRet) ? -EFAULT : 0;
          }while(0);


          break;


        case WMT_IOCTL_LPBK_TEST:
        do {
            P_OSAL_OP pOp;
            MTK_WCN_BOOL bRet;
            UINT32 u4Wait;
            //UINT8 lpbk_buf[1024] = {0};
            UINT32 effectiveLen = 0;
            P_OSAL_SIGNAL pSignal = NULL;

            if (copy_from_user(&effectiveLen, (void *)arg, sizeof(effectiveLen))) {
                iRet = -EFAULT;
                WMT_ERR_FUNC("copy_from_user failed at %d\n", __LINE__);
                break;
            }
            if(effectiveLen > sizeof(gLpbkBuf))
            {
                iRet = -EFAULT;
                WMT_ERR_FUNC("length is too long\n");
                break;
            }
            WMT_DBG_FUNC("len = %d\n", effectiveLen);

            pOp = wmt_lib_get_free_op();
            if (!pOp) {
                WMT_WARN_FUNC("get_free_lxop fail \n");
                iRet = -EFAULT;
                break;
            }
            u4Wait = 2000;
            if (copy_from_user(&gLpbkBuf[0], (void *)arg + sizeof(unsigned long), effectiveLen)) {
                WMT_ERR_FUNC("copy_from_user failed at %d\n", __LINE__);
                iRet = -EFAULT;
                break;
            }
            pSignal = &pOp->signal;
            pOp->op.opId = WMT_OPID_LPBK;
            pOp->op.au4OpData[0] = effectiveLen;    //packet length
            pOp->op.au4OpData[1] = (UINT32)&gLpbkBuf[0];        //packet buffer pointer
            memcpy(&gLpbkBufLog, &gLpbkBuf[((effectiveLen >=4) ? effectiveLen-4:0)], 4);
            pSignal->timeoutValue = MAX_EACH_WMT_CMD;
            WMT_INFO_FUNC("OPID(%d) type(%d) start\n",
                pOp->op.opId,
                pOp->op.au4OpData[0]);
            if (DISABLE_PSM_MONITOR()) {
                WMT_ERR_FUNC("wake up failed\n");
                wmt_lib_put_op_to_free_queue(pOp);
                return -1;
            }
            
            bRet = wmt_lib_put_act_op(pOp);
            ENABLE_PSM_MONITOR();
            if (MTK_WCN_BOOL_FALSE == bRet) {
                WMT_WARN_FUNC("OPID(%d) type(%d) buf tail(0x%08x) fail\n",
                pOp->op.opId,
                    pOp->op.au4OpData[0],
                    gLpbkBufLog);
                iRet = -1;
                break;
            }
            else {
                WMT_INFO_FUNC("OPID(%d) length(%d) ok\n",
                    pOp->op.opId, pOp->op.au4OpData[0]);
                iRet = pOp->op.au4OpData[0] ;
                if (copy_to_user((void *)arg + sizeof(ULONG) + sizeof(UCHAR[2048]), gLpbkBuf, iRet)) {
                    iRet = -EFAULT;
                    break;
                }
            }
        }while(0);

        break;

        case WMT_IOCTL_ADIE_LPBK_TEST:
		do {
            P_OSAL_OP pOp;
            MTK_WCN_BOOL bRet;
            P_OSAL_SIGNAL pSignal = NULL;

            pOp = wmt_lib_get_free_op();
            if (!pOp) {
                WMT_WARN_FUNC("get_free_lxop fail \n");
                iRet = -EFAULT;
                break;
            }
			
            pSignal = &pOp->signal;
            pOp->op.opId = WMT_OPID_ADIE_LPBK_TEST;
			pOp->op.au4OpData[0] = 0;
            pOp->op.au4OpData[1] = (UINT32)&gLpbkBuf[0];   
            pSignal->timeoutValue = MAX_EACH_WMT_CMD;
            WMT_INFO_FUNC("OPID(%d) start\n",pOp->op.opId);
            if (DISABLE_PSM_MONITOR()) {
                WMT_ERR_FUNC("wake up failed\n");
                wmt_lib_put_op_to_free_queue(pOp);
                return -1;
            }
            
            bRet = wmt_lib_put_act_op(pOp);
            ENABLE_PSM_MONITOR();
            if (MTK_WCN_BOOL_FALSE == bRet) {
                WMT_WARN_FUNC("OPID(%d) fail\n",pOp->op.opId);
                iRet = -1;
                break;
            }
            else {
                WMT_INFO_FUNC("OPID(%d) length(%d) ok\n",
                    pOp->op.opId, pOp->op.au4OpData[0]);
                iRet = pOp->op.au4OpData[0] ;
				if (copy_to_user((void *)arg + sizeof(ULONG), gLpbkBuf, iRet)) {
                    iRet = -EFAULT;
                    break;
                }
            }
        }while(0);

        break;
        
        case 10:
        {
			wmt_lib_host_awake_get();
            mtk_wcn_stp_coredump_start_ctrl(1);
			osal_strcpy(pBuffer, "MT662x f/w coredump start-");
            if (copy_from_user(pBuffer + osal_strlen(pBuffer), (void *)arg, NAME_MAX - osal_strlen(pBuffer))) {
                //osal_strcpy(pBuffer, "MT662x f/w assert core dump start");
                WMT_ERR_FUNC("copy assert string failed\n");
            }
            pBuffer[NAME_MAX] = '\0';
            osal_dbg_assert_aee(pBuffer, pBuffer);
        }
            break;
        case 11:
        {
            osal_dbg_assert_aee("MT662x f/w coredump end", "MT662x firmware coredump ends");
			wmt_lib_host_awake_put();
        }
        break;
        
            
        case WMT_IOCTL_GET_CHIP_INFO:
        {
            if (0 == arg)
            {
                return wmt_lib_get_icinfo(WMTCHIN_CHIPID);
            }
            else if (1 == arg)
            {
                return wmt_lib_get_icinfo(WMTCHIN_HWVER);
            }
			else if (2 == arg)
			{
			    return wmt_lib_get_icinfo(WMTCHIN_FWVER);
			}
        }
        break;

        case WMT_IOCTL_SET_LAUNCHER_KILL: {
            if (1 == arg) {
			    WMT_INFO_FUNC("launcher may be killed,block abnormal stp tx. \n");
			    wmt_lib_set_stp_wmt_last_close(1);
            } else {
			    wmt_lib_set_stp_wmt_last_close(0);
            }

		}
		break;
		
        case WMT_IOCTL_SET_PATCH_NUM: {
			pAtchNum = arg;
			WMT_INFO_FUNC(" get patch num from launcher = %d\n",pAtchNum);
			wmt_lib_set_patch_num(pAtchNum);
			pPatchInfo = kzalloc(sizeof(WMT_PATCH_INFO)*pAtchNum,GFP_ATOMIC);
			if (!pPatchInfo) {
				WMT_ERR_FUNC("allocate memory fail!\n");
				break;
			}
		}
		break;

		case WMT_IOCTL_SET_PATCH_INFO: {
			WMT_PATCH_INFO wMtPatchInfo;
			P_WMT_PATCH_INFO pTemp = NULL;
			UINT32 dWloadSeq;
			static UINT32 counter = 0;
			
			if (!pPatchInfo) {
				WMT_ERR_FUNC("NULL patch info pointer\n");
				break;
			}
            
            if (copy_from_user(&wMtPatchInfo, (void *)arg, sizeof(WMT_PATCH_INFO))) {
                WMT_ERR_FUNC("copy_from_user failed at %d\n", __LINE__);
                iRet = -EFAULT;
                break;
            }

			dWloadSeq = wMtPatchInfo.dowloadSeq;
			WMT_DBG_FUNC("current download seq no is %d,patch name is %s,addres info is 0x%02x,0x%02x,0x%02x,0x%02x\n",dWloadSeq,wMtPatchInfo.patchName,wMtPatchInfo.addRess[0],wMtPatchInfo.addRess[1],wMtPatchInfo.addRess[2],wMtPatchInfo.addRess[3]);
			osal_memcpy(pPatchInfo + dWloadSeq - 1,&wMtPatchInfo,sizeof(WMT_PATCH_INFO));
			pTemp = pPatchInfo + dWloadSeq - 1;
			if (++counter == pAtchNum) {
				wmt_lib_set_patch_info(pPatchInfo);
				counter = 0;
			}
		}
		break; 

		case WMT_IOCTL_WMT_COREDUMP_CTRL:
		{
		    if (0 == arg)
		    {
		        mtk_wcn_stp_coredump_flag_ctrl(0);
		    }
			else
			{
			    mtk_wcn_stp_coredump_flag_ctrl(1);
			}
		}
		break;
		case WMT_IOCTL_WMT_QUERY_CHIPID:
		{
			iRet = mtk_wcn_wmt_chipid_query();
			WMT_INFO_FUNC("chipid = 0x%x\n",iRet);
		}
		break;
		case WMT_IOCTL_SEND_BGW_DS_CMD:
		do {
            P_OSAL_OP pOp;
            MTK_WCN_BOOL bRet;
            UINT8 desense_buf[14] = {0};
            UINT32 effectiveLen = 14;
            P_OSAL_SIGNAL pSignal = NULL;

            pOp = wmt_lib_get_free_op();
            if (!pOp) {
                WMT_WARN_FUNC("get_free_lxop fail \n");
                iRet = -EFAULT;
                break;
            }
            if (copy_from_user(&desense_buf[0], (void *)arg, effectiveLen)) {
                WMT_ERR_FUNC("copy_from_user failed at %d\n", __LINE__);
                iRet = -EFAULT;
                break;
            }
            pSignal = &pOp->signal;
            pOp->op.opId = WMT_OPID_BGW_DS;
            pOp->op.au4OpData[0] = effectiveLen;    //packet length
            pOp->op.au4OpData[1] = (UINT32)&desense_buf[0];        //packet buffer pointer
            pSignal->timeoutValue = MAX_EACH_WMT_CMD;
            WMT_INFO_FUNC("OPID(%d) start\n",pOp->op.opId);
            if (DISABLE_PSM_MONITOR()) {
                WMT_ERR_FUNC("wake up failed\n");
                wmt_lib_put_op_to_free_queue(pOp);
                return -1;
            }
            
            bRet = wmt_lib_put_act_op(pOp);
            ENABLE_PSM_MONITOR();
            if (MTK_WCN_BOOL_FALSE == bRet) {
                WMT_WARN_FUNC("OPID(%d) fail\n",pOp->op.opId);
                iRet = -1;
                break;
            }
            else {
                WMT_INFO_FUNC("OPID(%d) length(%d) ok\n",
                    pOp->op.opId, pOp->op.au4OpData[0]);
                iRet = pOp->op.au4OpData[0] ;
            }
        }while(0);

		break;
    default:
        iRet = -EINVAL;
        WMT_WARN_FUNC("unknown cmd (%d)\n", cmd);
        break;
    }


    return iRet;
}

static int WMT_open(struct inode *inode, struct file *file)
{
	LONG ret;
	
    WMT_INFO_FUNC("major %d minor %d (pid %d)\n",
        imajor(inode),
        iminor(inode),
        current->pid
        );
	ret = wait_event_timeout(gWmtInitWq,gWmtInitDone != 0,msecs_to_jiffies(WMT_DEV_INIT_TO_MS));
	if(!ret)
	{
		WMT_WARN_FUNC("wait_event_timeout (%d)ms,(%d)jiffies,return -EIO\n",
			WMT_DEV_INIT_TO_MS,msecs_to_jiffies(WMT_DEV_INIT_TO_MS));
		return -EIO;
	}

    if (atomic_inc_return(&gWmtRefCnt) == 1) {
        WMT_INFO_FUNC("1st call \n");
    }

    return 0;
}

static int WMT_close(struct inode *inode, struct file *file)
{
    WMT_INFO_FUNC("major %d minor %d (pid %d)\n",
        imajor(inode),
        iminor(inode),
        current->pid
        );

    if (atomic_dec_return(&gWmtRefCnt) == 0) {
        WMT_INFO_FUNC("last call \n");
    }

    return 0;
}


struct file_operations gWmtFops = {
    .open = WMT_open,
    .release = WMT_close,
    .read = WMT_read,
    .write = WMT_write,
//    .ioctl = WMT_ioctl,
    .unlocked_ioctl = WMT_unlocked_ioctl,
    .poll = WMT_poll,
};

void wmt_dev_bgw_desense_init(VOID)
{
	bgw_init_socket();
}
void wmt_dev_bgw_desense_deinit(VOID)
{
	bgw_destory_netlink_kernel();
}
void wmt_dev_send_cmd_to_daemon(UINT32 cmd)
{
	send_command_to_daemon(cmd);
}
static int WMT_init(void)
{
    dev_t devID = MKDEV(gWmtMajor, 0);
    INT32 cdevErr = -1;
    INT32 ret = -1;
    WMT_INFO_FUNC("WMT Version= %s DATE=%s\n" , MTK_WMT_VERSION, MTK_WMT_DATE);
    /* Prepare a UCHAR device */
    /*static allocate chrdev*/
	gWmtInitDone = 0;
	init_waitqueue_head((wait_queue_head_t *)&gWmtInitWq);
	
    stp_drv_init();

    ret = register_chrdev_region(devID, WMT_DEV_NUM, WMT_DRIVER_NAME);
    if (ret) {
        WMT_ERR_FUNC("fail to register chrdev\n");
        return ret;
    }

    cdev_init(&gWmtCdev, &gWmtFops);
    gWmtCdev.owner = THIS_MODULE;

    cdevErr = cdev_add(&gWmtCdev, devID, WMT_DEV_NUM);
    if (cdevErr) {
        WMT_ERR_FUNC("cdev_add() fails (%d) \n", cdevErr);
        goto error;
    }
    WMT_INFO_FUNC("driver(major %d) installed \n", gWmtMajor);


#if 0
    pWmtDevCtx = wmt_drv_create();
    if (!pWmtDevCtx) {
        WMT_ERR_FUNC("wmt_drv_create() fails \n");
        goto error;
    }

    ret = wmt_drv_init(pWmtDevCtx);
    if (ret) {
        WMT_ERR_FUNC("wmt_drv_init() fails (%d) \n", ret);
        goto error;
    }

    WMT_INFO_FUNC("stp_btmcb_reg\n");
    wmt_cdev_btmcb_reg();

    ret = wmt_drv_start(pWmtDevCtx);
    if (ret) {
        WMT_ERR_FUNC("wmt_drv_start() fails (%d) \n", ret);
        goto error;
    }
#endif
    ret = wmt_lib_init();
    if (ret) {
        WMT_ERR_FUNC("wmt_lib_init() fails (%d) \n", ret);
        goto error;
    }
#if CFG_WMT_DBG_SUPPORT
    wmt_dev_dbg_setup();
#endif

#if CFG_WMT_PROC_FOR_AEE
	wmt_dev_proc_for_aee_setup();
#endif

    WMT_INFO_FUNC("wmt_dev register thermal cb\n");
    wmt_lib_register_thermal_ctrl_cb(wmt_dev_tm_temp_query);

	wmt_dev_bgw_desense_init();

#if CONSYS_WMT_REG_SUSPEND_CB_ENABLE
	ret = platform_device_register(&mtk_device_wmt);
	WMT_INFO_FUNC("mtk_device_wmt register ret %d", ret);
	if (ret != 0){
		WMT_ERR_FUNC("WMT platform device registered failed(%d)\n",ret);
	}
	
	ret = platform_driver_register(&mtk_wmt_dev_drv);
	if(ret)
	{
		WMT_ERR_FUNC("WMT platform driver registered failed(%d)\n",ret);
	}
#endif
    
	gWmtInitDone = 1;
	wake_up(&gWmtInitWq);

	WMT_INFO_FUNC("success \n");
    return 0;

error:
    wmt_lib_deinit();
#if CFG_WMT_DBG_SUPPORT    
    wmt_dev_dbg_remove();
#endif
    if (cdevErr == 0) {
        cdev_del(&gWmtCdev);
    }

    if (ret == 0) {
        unregister_chrdev_region(devID, WMT_DEV_NUM);
        gWmtMajor = -1;
    }

    WMT_ERR_FUNC("fail \n");

    return -1;
}

static void WMT_exit (void)
{
    dev_t dev = MKDEV(gWmtMajor, 0);
	
#if CONSYS_WMT_REG_SUSPEND_CB_ENABLE
	platform_driver_unregister(&mtk_wmt_dev_drv);
#endif

	wmt_dev_bgw_desense_deinit();
	
	wmt_lib_register_thermal_ctrl_cb(NULL);

    wmt_lib_deinit();
    
#if CFG_WMT_DBG_SUPPORT
    wmt_dev_dbg_remove();
#endif

#if CFG_WMT_PROC_FOR_AEE
	wmt_dev_proc_for_aee_remove();
#endif
    cdev_del(&gWmtCdev);
    unregister_chrdev_region(dev, WMT_DEV_NUM);
    gWmtMajor = -1;
#ifdef MTK_WMT_WAKELOCK_SUPPORT
    WMT_WARN_FUNC("destroy func_on_off_wake_lock\n");
    wake_lock_destroy(&func_on_off_wake_lock);
#endif

    stp_drv_exit();

    WMT_INFO_FUNC("done\n");
}
#ifdef MTK_WCN_REMOVE_KERNEL_MODULE

int mtk_wcn_soc_common_drv_init(void)
{
	return WMT_init();

}

void mtk_wcn_soc_common_drv_exit (void)
{
	return WMT_exit();
}


EXPORT_SYMBOL(mtk_wcn_soc_common_drv_init);
EXPORT_SYMBOL(mtk_wcn_soc_common_drv_exit);
#else
module_init(WMT_init);
module_exit(WMT_exit);
#endif
//MODULE_LICENSE("Proprietary");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("MediaTek Inc WCN");
MODULE_DESCRIPTION("MTK WCN combo driver for WMT function");

module_param(gWmtMajor, uint, 0);


