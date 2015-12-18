#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/module.h>
#include <linux/poll.h>

int __weak exec_ccci_kern_func_by_md_id(int md_id, unsigned int id, char *buf, unsigned int len)
{
printk("[ccci/dummy] exec_ccci_kern_func_by_md_id no support!\n");
return 0;
}
int __weak switch_sim_mode(int id, char *buf, unsigned int len)
{
printk("[ccci0/port] switch_sim_mode no support!\n");
return 0;
}
unsigned int __weak get_sim_switch_type(void)
{
printk("[ccci0/port] get_sim_switch_type no support!\n");
return 0;
}