#include <linux/delay.h>
#include <mach/sync_write.h>
#include <mach/mt_boot_common.h>
#include <mach/mt_clkmgr.h>

extern BOOTMODE g_boot_mode;


#define sync_write32(v, a)			mt_reg_sync_writel(v, a)
#define sync_write16(v, a)			mt_reg_sync_writew(v, a)
#define sync_write8(v, a)			mt_reg_sync_writeb(v, a)

static void internal_md_power_down(void)
{
	printk("[ccci-off]TODO:power off MD when CCCI is disabled\n");
}

static int __init modem_off_init(void)
{
#ifndef CONFIG_MTK_CCCI_EXT
	printk("[ccci-off]power off MD when CCCI is disabled\n");
	internal_md_power_down();
#else
#ifdef MTK_KERNEL_POWER_OFF_CHARGING
	if ((g_boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT) || (g_boot_mode == LOW_POWER_OFF_CHARGING_BOOT)) {
		printk("[ccci-off]power off MD in charging mode %d\n", g_boot_mode);
		internal_md_power_down();
	}
#endif
#endif
	return 0;
}

module_init(modem_off_init);

