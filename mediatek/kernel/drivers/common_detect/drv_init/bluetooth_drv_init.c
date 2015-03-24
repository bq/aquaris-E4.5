#ifdef DFT_TAG
#undef DFT_TAG
#endif
#define DFT_TAG         "[BT-MOD-INIT]"

#include "wmt_detect.h"
#include "bluetooth_drv_init.h"
#include <mach/mt_boot_common.h>
static int boot_mode = 0;

int do_bluetooth_drv_init(int chip_id)
{
	int i_ret = -1;
  /*CKT kevin modify for factory/meta mode can use MTK origin BT driver for factory mode test */
  boot_mode = get_boot_mode();
  if(boot_mode!=0)
    {/*Other boot*/
      WMT_DETECT_INFO_FUNC("start to do bluetooth driver init other boot %d \n",boot_mode);
      i_ret = mtk_wcn_stpbt_drv_init();
      WMT_DETECT_INFO_FUNC("finish bluetooth driver init, i_ret:%d\n", i_ret);
    }else
    {/*Normal boot*/
      WMT_DETECT_INFO_FUNC("start to do bluetooth driver init normal boot %d\n",boot_mode);
      i_ret = ckt_hci_init();
      WMT_DETECT_INFO_FUNC("finish bluetooth driver init, i_ret:%d\n", i_ret);
    }
	return i_ret;
}


