/******************************************************************************
 * pmic_wrapper.c - Linux pmic_wrapper Driver
 *
 *
 * DESCRIPTION:
 *     This file provid the other drivers PMIC wrapper relative functions
 *
 ******************************************************************************/

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <mach/mt_typedefs.h>
#include <linux/timer.h>
#include <mach/mt_pmic_wrap.h>
#include <linux/syscore_ops.h>

#define PMIC_WRAP_DEVICE "pmic_wrap"
#define VERSION     "$Revision$"

static struct mt_pmic_wrap_driver mt_wrp={
    .driver = {
        .name = "pmic_wrap",
        .bus = &platform_bus_type,
        .owner = THIS_MODULE,
    },
};
struct mt_pmic_wrap_driver *get_mt_pmic_wrap_drv(void)
{
    return &mt_wrp;
}
//******************************************************************************
//--external API for pmic_wrap user-------------------------------------------------
//******************************************************************************
S32 pwrap_wacs2( U32  write, U32  adr, U32  wdata, U32 *rdata )
{
	if(mt_wrp.wacs2_hal != NULL)
		return mt_wrp.wacs2_hal(write, adr,wdata,rdata );

	printk(KERN_ERR "[WRAP]""driver need registered!!");
	return -5;

}
EXPORT_SYMBOL(pwrap_wacs2);
S32 pwrap_read( U32  adr, U32 *rdata )
{
	return pwrap_wacs2( PWRAP_READ, adr,0,rdata );
}
EXPORT_SYMBOL(pwrap_read);

S32 pwrap_write( U32  adr, U32  wdata )
{
	return pwrap_wacs2( PWRAP_WRITE, adr,wdata,0 );
}
EXPORT_SYMBOL(pwrap_write);
/********************************************************************/
/********************************************************************/
// return value : EINT_STA: [0]: CPU IRQ status in MT6331
//						[1]: MD32 IRQ status in MT6331
//						[2]: CPU IRQ status in MT6332
//						[3]: RESERVED
/********************************************************************/
U32 pmic_wrap_eint_status(void)
{
	return mt_pmic_wrap_eint_status();
}
EXPORT_SYMBOL(pmic_wrap_eint_status);

/********************************************************************/
// set value(W1C) : EINT_CLR:       [0]: CPU IRQ status in MT6331
//							   [1]: MD32 IRQ status in MT6331
//						          [2]: CPU IRQ status in MT6332
//						          [3]: RESERVED
//para: offset is shift of clear bit which needs to clear 
/********************************************************************/
void pmic_wrap_eint_clr(int offset)
{
	mt_pmic_wrap_eint_clr(offset);
}
EXPORT_SYMBOL(pmic_wrap_eint_clr);
/*******************************************************************/
static ssize_t mt_pwrap_show(struct device_driver *driver, char *buf)
{
	if(mt_wrp.show_hal != NULL)
		return mt_wrp.show_hal(buf);

	return snprintf(buf, PAGE_SIZE, "%s\n","[WRAP]driver need registered!! ");
}

static ssize_t mt_pwrap_store(struct device_driver *driver, const char *buf,
                  size_t count)
{
	if(mt_wrp.store_hal != NULL)
		return mt_wrp.store_hal(buf, count);

	printk(KERN_ERR "[WRAP]""driver need registered!!");
	return count;
}

DRIVER_ATTR(pwrap, 0664, mt_pwrap_show, mt_pwrap_store);
/*-----suspend/resume for pmic_wrap-------------------------------------------*/
//infra power down while suspend,pmic_wrap will gate clock after suspend.
//so,need to init PPB when resume.
//only affect PWM and I2C
static int pwrap_suspend(void)
{
	//PWRAPLOG("pwrap_suspend\n");
	if(mt_wrp.suspend != NULL)
		return mt_wrp.suspend();
	return 0;
}
static void pwrap_resume(void)
{
	if(mt_wrp.resume != NULL)
		mt_wrp.resume();
	return;
}
static struct syscore_ops pwrap_syscore_ops = {
  .resume   = pwrap_resume,
  .suspend  = pwrap_suspend,
};
static int __init mt_pwrap_init(void)
{
	U32 ret = 0;
	printk("[PWRAP] common driver init, version %s\n", VERSION);

//	if(mt_wrp.init != NULL)
//		mt_wrp.init();
	//ret = platform_driver_register(&mt_pwrap_driver);
	ret = driver_register(&mt_wrp.driver);
	if (ret) {
		printk(KERN_ERR "[WRAP]""Fail to register mt_wrp");
	}

	ret = driver_create_file(&mt_wrp.driver, &driver_attr_pwrap);
	if (ret) {
		printk(KERN_ERR "[WRAP]""Fail to create mt_wrp sysfs files");
	}
	//PWRAPLOG("pwrap_init_ops\n");
	register_syscore_ops(&pwrap_syscore_ops);

	////gating eint/i2c/pwm/kpd clock@PMIC
	//pwrap_read_nochk(PMIC_WRP_CKPDN,&rdata);
	////disable clock,except kpd(bit[4] kpd and bit[6] 32k);
	//ret= pwrap_write_nochk(PMIC_WRP_CKPDN,  rdata | 0x2F);//set dewrap clock bit
	return ret;

}
arch_initcall(mt_pwrap_init);
//device_initcall(mt_pwrap_init);
///*---------------------------------------------------------------------------*/
//static void __exit mt_pwrap_exit(void)
//{
//    platform_driver_unregister(&mt_pwrap_driver);
//    return;
//}
///*---------------------------------------------------------------------------*/
//postcore_initcall(mt_pwrap_init);
//module_exit(mt_pwrap_exit);
//#define PWRAP_EARLY_PORTING
/*-----suspend/resume for pmic_wrap-------------------------------------------*/
//infra power down while suspend,pmic_wrap will gate clock after suspend.
//so,need to init PPB when resume.
//only affect PWM and I2C

//static struct syscore_ops pwrap_syscore_ops = {
//  .resume   = pwrap_resume,
//  .suspend  = pwrap_suspend,
//};
//
//static int __init pwrap_init_ops(void)
//{
//  PWRAPLOG("pwrap_init_ops\n");
//  register_syscore_ops(&pwrap_syscore_ops);
//  return 0;
//}
//device_initcall(pwrap_init_ops);

MODULE_AUTHOR("Ranran Lu <ranran.lu@mediatek.com>");
MODULE_DESCRIPTION("pmic_wrapper Driver  $Revision$");
MODULE_LICENSE("GPL");
