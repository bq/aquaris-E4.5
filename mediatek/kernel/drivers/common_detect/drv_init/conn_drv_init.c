
#ifdef DFT_TAG
#undef DFT_TAG
#endif
#define DFT_TAG         "[WCN-MOD-INIT]"

#include "wmt_detect.h"
#include "conn_drv_init.h"
#include "common_drv_init.h"
#include "fm_drv_init.h"
#include "wlan_drv_init.h"
#include "bluetooth_drv_init.h"
#include "gps_drv_init.h"





int do_connectivity_driver_init(int chip_id)
{
	int i_ret = 0;
	int tmp_ret = 0;

	tmp_ret = do_common_drv_init(chip_id);
	i_ret += tmp_ret;
	if (tmp_ret)
	{
		WMT_DETECT_ERR_FUNC("do common driver init failed, ret:%d\n", tmp_ret);
		WMT_DETECT_ERR_FUNC("abort connectivity driver init, because common part is not ready\n");
		return i_ret;
	}
	
	tmp_ret = do_bluetooth_drv_init(chip_id);
	i_ret += tmp_ret;
	if (tmp_ret)
	{
		WMT_DETECT_ERR_FUNC("do common driver init failed, ret:%d\n", tmp_ret);
	}
		
	tmp_ret = do_gps_drv_init(chip_id);
	i_ret += tmp_ret;
	if (tmp_ret)
	{
		WMT_DETECT_ERR_FUNC("do common driver init failed, ret:%d\n", tmp_ret);
	}
			

	tmp_ret = do_fm_drv_init(chip_id);
	i_ret += tmp_ret;
	if (tmp_ret)
	{
		WMT_DETECT_ERR_FUNC("do fm module init failed, ret:%d\n", tmp_ret);
	}

	tmp_ret = do_wlan_drv_init(chip_id);
	i_ret += tmp_ret;
	if (tmp_ret)
	{
		WMT_DETECT_ERR_FUNC("do wlan module init failed, ret:%d\n", tmp_ret);
	}
	
	return i_ret;
	
}

