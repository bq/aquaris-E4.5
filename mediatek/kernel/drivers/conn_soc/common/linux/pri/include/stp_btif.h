#ifndef _STP_BTIF_H_
#define _STP_BTIF_H_

#include "osal_typedef.h"
#include "mtk_btif_exp.h"

extern INT32 mtk_wcn_consys_stp_btif_open(VOID);
extern INT32 mtk_wcn_consys_stp_btif_close(VOID);
extern INT32 mtk_wcn_consys_stp_btif_rx_cb_register(MTK_WCN_BTIF_RX_CB rx_cb);
extern INT32 mtk_wcn_consys_stp_btif_tx(const UINT8 *pBuf,const UINT32 len,UINT32 *written_len);
extern INT32 mtk_wcn_consys_stp_btif_wakeup(VOID);
extern INT32 mtk_wcn_consys_stp_btif_dpidle_ctrl(ENUM_BTIF_DPIDLE_CTRL en_flag);
extern INT32 mtk_wcn_consys_stp_btif_lpbk_ctrl(ENUM_BTIF_LPBK_MODE mode);
extern INT32 mtk_wcn_consys_stp_btif_logger_ctrl(ENUM_BTIF_DBG_ID flag);

#endif
