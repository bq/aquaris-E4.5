#include <linux/spinlock.h>
#include <linux/errno.h>
#include <linux/module.h>

//#define WATCH_DBG
#ifdef WATCH_DBG
// Test for online cpu
#include <linux/device.h>
#include <linux/platform_device.h>
#endif

#include <asm/system.h>
#include <asm/signal.h>
#include <asm/ptrace.h>
#include <mach/hw_watchpoint.h>

#define DBGWVR_BASE 0xF0170180
#define DBGWCR_BASE 0xF01701C0

#define DBGLAR 0xF0170FB0
#define DBGDSCR 0xF0170088
#define DBGWFAR 0xF0170018
#define DBGOSLAR 0xF0170300
#define DBGOSSAR 0xF0170304
#define MAX_NR_WATCH_POINT 4
#define NUM_CPU    4

#define UNLOCK_KEY 0xC5ACCE55
#define HDBGEN (1 << 14)
#define MDBGEN (1 << 15)
#define DBGWCR_VAL 0x000001E7
/**/
#define WP_EN (1 << 0)
#define LSC_LDR (1 << 3)
#define LSC_STR (2 << 3)
#define LSC_ALL (3 << 3)
#if 0
 struct wp_event wp_event;
 int err;
 volatile int my_watch_data;
 volatile int my_watch_data2;
 volatile int my_watch_data3;

 int my_wp_handler1(unsigned int addr)
 {
     printk("Access my data from an instruction at 0x%x\n", addr);
     return 0;
 }

 int my_wp_handler2(unsigned int addr)
 {
     printk("Access my data from an instruction at 0x%x\n", addr);
     /* trigger exception */
     return 1;
 }

 void foo(void)
 {
     int test;
     init_wp_event(&wp_event, &my_watch_data, &my_watch_data, WP_EVENT_TYPE_ALL, my_wp_handler1);

     err = add_hw_watchpoint(&wp_event);
     if (err) {
         printk("add hw watch point failed...\n");
         /* fail to add watchpoing */
     } else {
         /* the memory address is under watching */
         printk("add hw watch point success...\n");

         //del_hw_watchpoint(&wp_event);
     }
     /* test watchpoint */
     my_watch_data = 1;
 }
 
 void foo2(void)
 {
     int test;
     init_wp_event(&wp_event, &my_watch_data2, &my_watch_data2, WP_EVENT_TYPE_ALL, my_wp_handler1);

     err = add_hw_watchpoint(&wp_event);
     if (err) {
         printk("add hw watch point failed...\n");
         /* fail to add watchpoing */
     } else {
         /* the memory address is under watching */
         printk("add hw watch point success...\n");

         //del_hw_watchpoint(&wp_event);
     }
     /* test watchpoint */
     my_watch_data2 = 2;
 }

void foo3(void)
{
     int test;
     init_wp_event(&wp_event, &my_watch_data3, &my_watch_data3, WP_EVENT_TYPE_ALL, my_wp_handler1);

     err = add_hw_watchpoint(&wp_event);
     if (err) {
         printk("add hw watch point failed...\n");
         /* fail to add watchpoing */
     } else {
         /* the memory address is under watching */
         printk("add hw watch point success...\n");

         //del_hw_watchpoint(&wp_event);
     }
     /* test watchpoint */
     my_watch_data3 = 3;
}

#endif

static struct wp_event wp_events[MAX_NR_WATCH_POINT];
static spinlock_t wp_lock;

/*
 * enable_hw_watchpoint: Enable the H/W watchpoint.
 * Return error code.
 */
int enable_hw_watchpoint(void)
{
    int i;

    //for(i = 0 ; i < NUM_CPU; i++) {
    for_each_online_cpu(i) {
        if (*(volatile unsigned int *)(DBGDSCR + i * 0x2000) & HDBGEN) {
            printk(KERN_ALERT "halting debug mode enabled. Unable to access hardware resources.\n");
            return -EPERM;
        }

        if (*(volatile unsigned int *)(DBGDSCR + i * 0x2000) & MDBGEN) {
            /* already enabled */
            printk(KERN_ALERT "already enabled, DBGDSCR = 0x%x\n", *(volatile unsigned int *)(DBGDSCR + i * 0x2000));
            //return 0;
        }

        *(volatile unsigned int *)(DBGLAR + i * 0x2000) = UNLOCK_KEY;
        //printk(KERN_ALERT "DBGLAR + 4 = 0x%x\n", *(volatile unsigned int *)(DBGDSCR + i * 0x2000));
        *(volatile unsigned int *)(DBGOSLAR + i * 0x2000) = ~UNLOCK_KEY;
        *(volatile unsigned int *)(DBGDSCR + i * 0x2000) |= MDBGEN;

    }
    return 0;
}

/*
 * add_hw_watchpoint: add a watch point.
 * @wp_event: pointer to the struct wp_event.
 * Return error code.
 */
int add_hw_watchpoint(struct wp_event *wp_event)
{
    int ret, i, j;
    unsigned long flags;
    unsigned int ctl;

    if (!wp_event) {
        return -EINVAL;
    }
    if (!(wp_event->handler)) {
        return -EINVAL;
    }

    ret = enable_hw_watchpoint();
    if (ret) {
        return ret;
    }

    ctl = DBGWCR_VAL;
    if (wp_event->type == WP_EVENT_TYPE_READ) {
        ctl |= LSC_LDR;
    } else if (wp_event->type == WP_EVENT_TYPE_WRITE) {
        ctl |= LSC_STR;
    } else if (wp_event->type == WP_EVENT_TYPE_ALL) {
        ctl |= LSC_ALL;
    } else {
        return -EINVAL;
    }

    spin_lock_irqsave(&wp_lock, flags);
    for (i = 0; i < MAX_NR_WATCH_POINT; i++) {
        if (!wp_events[i].in_use) {
            wp_events[i].in_use = 1;
            break;
        }
    }
    spin_unlock_irqrestore(&wp_lock, flags);

    if (i == MAX_NR_WATCH_POINT) {
        return -EAGAIN;
    }

    wp_events[i].virt = wp_event->virt & ~3;    /* enforce word-aligned */
    wp_events[i].phys = wp_event->phys; /* no use currently */
    wp_events[i].type = wp_event->type;
    wp_events[i].handler = wp_event->handler;
    wp_events[i].auto_disable = wp_event->auto_disable;


    printk(KERN_ALERT "Add watchpoint %d at address 0x%x\n", i, wp_events[i].virt);
    //for(j = 0; j < NUM_CPU; j++) {
    for_each_online_cpu(j) {
        printk("cpu %d is online %d\n", smp_processor_id(), j);
        *(((volatile unsigned int *)(DBGWVR_BASE + 0x2000 * j)) + i) = wp_events[i].virt;
        *(((volatile unsigned int *)(DBGWCR_BASE + 0x2000 * j)) + i) = ctl;

        printk(KERN_ALERT "*(((volatile unsigned int *)(DBGWVR_BASE + 0x%4lx)) + %d) = 0x%x\n",(unsigned long)(0x2000 * j), i, *(((volatile unsigned int *)(DBGWVR_BASE + 0x2000 * j)) + i));
        printk(KERN_ALERT "*(((volatile unsigned int *)(DBGWCR_BASE + 0x%4lx)) + %d) = 0x%x\n", (unsigned long)(0x2000 * j), i, *(((volatile unsigned int *)(DBGWCR_BASE + 0x2000 * j)) + i));
    }
    return 0;
}

/*
 * del_hw_watchpoint: delete a watch point.
 * @wp_event: pointer to the struct wp_event.
 * Return error code.
 */
int del_hw_watchpoint(struct wp_event *wp_event)
{
    unsigned long flags;
    int i, j;

    if (!wp_event) {
        return -EINVAL;
    }

    spin_lock_irqsave(&wp_lock, flags);
    for (i = 0; i < MAX_NR_WATCH_POINT; i++) {
        if (wp_events[i].in_use && (wp_events[i].virt == wp_event->virt)) {
            wp_events[i].virt = 0;
            wp_events[i].phys = 0;
            wp_events[i].type = 0;
            wp_events[i].handler = NULL;
            wp_events[i].in_use = 0;
            //for(j = 0; j < NUM_CPU; j++)
            for_each_online_cpu(j)
                *(((volatile unsigned int *)(DBGWCR_BASE + j * 0x2000)) + i) &= ~WP_EN;
            break;
        }
    }
    spin_unlock_irqrestore(&wp_lock, flags);

    if (i == MAX_NR_WATCH_POINT) {
        return -EINVAL;
    } else {
        return 0;
    }
}

int watchpoint_handler(unsigned long addr, unsigned int fsr, struct pt_regs *regs)
{
    unsigned int wfar, daddr, iaddr;
    int i, ret, j;
#if defined CONFIG_ARCH_MT6582
/* Notes
 *v7 Debug the address of instruction that triggered the watchpoint is in DBGWFAR
 *v7.1 Debug the address is in DFAR
*/
    asm volatile(
    "MRC p15, 0, %0, c6, c0, 0\n"
    : "=r" (wfar)
    :
    : "cc"
    );
#else
    wfar = *(volatile unsigned int*)(DBGWFAR +  raw_smp_processor_id() * 0x2000);
#endif
    daddr = addr & ~3;
    iaddr = regs->ARM_pc;
    printk(KERN_ALERT "addr = 0x%x, DBGWFAR/DFAR = 0x%x\n", (unsigned int)addr, wfar);
    printk(KERN_ALERT "daddr = 0x%x, iaddr = 0x%x\n", daddr, iaddr);


    /* update PC to avoid re-execution of the instruction under watching */
    regs->ARM_pc += thumb_mode(regs)? 2: 4;

    for (i = 0; i < MAX_NR_WATCH_POINT; i++) {
        if (wp_events[i].in_use && wp_events[i].virt == (daddr)) {
            printk(KERN_ALERT "Watchpoint %d triggers.\n", i);
            if (wp_events[i].handler) {
                if (wp_events[i].auto_disable) {
                    //for(j = 0; j < NUM_CPU; j++)
                    for_each_online_cpu(j)
                        *(((volatile unsigned int *)(DBGWCR_BASE + j * 0x2000)) + i) &= ~WP_EN;
                }
                ret = wp_events[i].handler(iaddr);
                if (wp_events[i].auto_disable) {
                    //for(j = 0; j < NUM_CPU; j++)
                    for_each_online_cpu(j)
                        *(((volatile unsigned int *)(DBGWCR_BASE + j * 0x2000)) + i) |= WP_EN;
                }
                return ret;
            } else {
                printk(KERN_ALERT "No watchpoint handler. Ignore.\n");                
                return 0;
            }
        }
    }

    return 0;
}

#ifdef WATCH_BUG
static struct device_driver hw_drv = 
{
    .name       = "hw_watchpoint",
    .bus        = &platform_bus_type,
    .owner      = THIS_MODULE
};

static ssize_t watchpoint_test_show(struct device_driver *driver, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "currently run on cpu: %d\n", smp_processor_id());
}

static ssize_t watchpoint_test_store(struct device_driver *driver,
                     const char *buf, size_t count)
{
    foo3();
    return count;
}

DRIVER_ATTR(watchpoint_test, 0644, watchpoint_test_show, watchpoint_test_store);
#endif

static int __init hw_watchpoint_init(void)
{
    int ret;
    spin_lock_init(&wp_lock);

    #ifdef WATCH_DBG
    ret = driver_register(&hw_drv);
    if (ret) {
        pr_err("Fail to register mt_hw_drv");
    }

    ret = driver_create_file(&hw_drv, &driver_attr_watchpoint_test);
    if (ret) {
        pr_err("Fail to create mt_hw_drv sysfs files");
    }
    
    driver_create_file(&hw_drv, &driver_attr_watchpoint_test);
    #endif
   
    hook_fault_code(2, watchpoint_handler, SIGTRAP, 0, "watchpoint debug exception");
    printk(KERN_ALERT "watchpoint handler init. \n");
    //foo();
    //foo2();

    return 0;
}

//EXPORT_SYMBOL(add_hw_watchpoint);

arch_initcall(hw_watchpoint_init);
