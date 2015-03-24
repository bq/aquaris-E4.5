#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <mach/sync_write.h>
#include <mach/dbg_dump.h>
#include <linux/kallsyms.h>

unsigned int is_reg_dump_device_registered = 0;
unsigned int mcu_reg_base;
int dbg_reg_dump_probe(struct platform_device *pdev);

int mt_reg_dump(char *buf)
{
  /* Get core numbers */
	int ret = -1, cnt = num_possible_cpus();
	char *ptr = buf;
	unsigned int pc_value;
	unsigned int fp_value;
	unsigned int sp_value;
	unsigned long size = 0;
	unsigned long offset = 0;
	char str[KSYM_SYMBOL_LEN];
	int i;
	
  if(cnt < 0)
    return ret;
  
  if(is_reg_dump_device_registered)
  {
    /* Get PC, FP, SP and save to buf */
	  for (i = 0; i < cnt; i++) {
	    pc_value = readl(IOMEM(mcu_reg_base + (i << 4)));
	    fp_value = readl(IOMEM((mcu_reg_base+0x4) + (i << 4)));
	    sp_value = readl(IOMEM((mcu_reg_base+0x8) + (i << 4)));
	    kallsyms_lookup((unsigned long)pc_value, &size, &offset, NULL, str);	  
	    ptr += sprintf(ptr, "CORE_%d PC = 0x%x(%s + 0x%lx), FP = 0x%x, SP = 0x%x\n", i, pc_value, str, offset, fp_value, sp_value);
	    //printk("CORE_%d PC = 0x%x(%s), FP = 0x%x, SP = 0x%x\n", i, pc_value, str, fp_value, sp_value);
	  }
	  return 0;	
  }  
		
	return -1;
}

static struct platform_driver dbg_reg_dump_driver =
{
	.probe = dbg_reg_dump_probe,
	.driver = {
		.name = "dbg_reg_dump",
		.bus = &platform_bus_type,
		.owner = THIS_MODULE,
	},
};


static ssize_t last_pc_dump_show(struct device_driver *driver, char *buf)
{
	int ret = mt_reg_dump(buf);
	if (ret == -1)
		printk(KERN_CRIT "Dump error in %s, %d\n", __func__, __LINE__);
	
	return strlen(buf);;
}

static ssize_t last_pc_dump_store(struct device_driver * driver, const char *buf,
			   size_t count)
{
	return count;
}

DRIVER_ATTR(last_pc_dump, 0664, last_pc_dump_show, last_pc_dump_store);

int dbg_reg_dump_probe(struct platform_device *pdev)
{
  int ret;
  
  struct reg_dump_driver_data *data = dev_get_platdata(&pdev->dev);
  mcu_reg_base = data->mcu_regs;
  
  
  is_reg_dump_device_registered = 1;  
	

	ret = driver_create_file(&dbg_reg_dump_driver.driver,
			       &driver_attr_last_pc_dump);
	if (ret) {
		pr_err("Fail to create mt_reg_dump_drv sysfs files");
	}
	
	return 0;
}



/**
 * driver initialization entry point
 */
static int __init dbg_reg_dump_init(void)
{
	int err;

	err = platform_driver_register(&dbg_reg_dump_driver);
	if (err) {
		return err;
	}

		return 0;
}

/**
 * driver exit point
 */
static void __exit dbg_reg_dump_exit(void)
{	
}

module_init(dbg_reg_dump_init);
module_exit(dbg_reg_dump_exit);
