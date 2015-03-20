#ifdef DFT_TAG
#undef DFT_TAG
#endif
#define DFT_TAG         "[WMT-MOD-INIT]"


#include "wmt_detect.h"
#include "common_drv_init.h"

int static do_combo_common_drv_init(int chip_id)
{
	int i_ret = 0;
	int i_ret_tmp = 0;
	WMT_DETECT_INFO_FUNC("start to do combo driver init, chipid:0x%08x\n", chip_id);
	
#ifdef MTK_WCN_COMBO_CHIP_SUPPORT
	/*HIF-SDIO driver init*/
	i_ret_tmp = mtk_wcn_hif_sdio_drv_init();
	WMT_DETECT_INFO_FUNC("HIF-SDIO driver init, i_ret:%d\n", i_ret);
	i_ret += i_ret_tmp;
	
	/*WMT driver init*/
	i_ret_tmp = mtk_wcn_combo_common_drv_init();
	WMT_DETECT_INFO_FUNC("COMBO COMMON driver init, i_ret:%d\n", i_ret);
	i_ret += i_ret_tmp;

	/*STP-UART driver init*/
	i_ret_tmp = mtk_wcn_stp_uart_drv_init();
	WMT_DETECT_INFO_FUNC("STP-UART driver init, i_ret:%d\n", i_ret);
	i_ret += i_ret_tmp;

	/*STP-SDIO driver init*/
	i_ret_tmp = mtk_wcn_stp_sdio_drv_init();
	WMT_DETECT_INFO_FUNC("STP-SDIO driver init, i_ret:%d\n", i_ret);
	i_ret += i_ret_tmp;

#else
	i_ret = -1;
	WMT_DETECT_INFO_FUNC("COMBO chip is not supported, please check kernel makefile or project fonfig, i_ret:%d\n", i_ret);
#endif
	WMT_DETECT_INFO_FUNC("finish combo driver init\n");
	return i_ret;
}

int static do_soc_common_drv_init(int chip_id)
{
	int i_ret = 0;
	int i_ret_tmp = 0;
	WMT_DETECT_INFO_FUNC("start to do soc common driver init, chipid:0x%08x\n", chip_id);
	
#ifdef MTK_WCN_SOC_CHIP_SUPPORT
	
	/*WMT driver init*/
	i_ret_tmp = mtk_wcn_soc_common_drv_init();
	WMT_DETECT_INFO_FUNC("COMBO COMMON driver init, i_ret:%d\n", i_ret);
	i_ret += i_ret_tmp;

#else
	i_ret = -1;
	WMT_DETECT_INFO_FUNC("SOC chip is not supported, please check kernel makefile or project fonfig, i_ret:%d\n", i_ret);
#endif

	WMT_DETECT_INFO_FUNC("TBD........\n");
	return i_ret;
}


int do_common_drv_init(int chip_id)
{
	int i_ret = 0;
	
	WMT_DETECT_INFO_FUNC("start to do common driver init, chipid:0x%08x\n", chip_id);

	switch (chip_id)
	{
		case 0x6620:
		case 0x6628:
		case 0x6630:
			i_ret = do_combo_common_drv_init(chip_id);
			break;
		case 0x6572:
		case 0x6582:
		case 0x6592:
		case 0x6571:
		case 0x8127:
			i_ret = do_soc_common_drv_init(chip_id);
			break;
	}
	
	WMT_DETECT_INFO_FUNC("finish common driver init\n");

	return i_ret;
}

