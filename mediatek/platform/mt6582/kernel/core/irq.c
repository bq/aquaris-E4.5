#include <linux/io.h>
#include <linux/module.h>
#include <linux/smp.h>
#include <linux/interrupt.h>
#include <linux/cpu.h>
#include <linux/notifier.h>
#include <linux/aee.h>
#include <linux/mtk_ram_console.h>
#include <asm/mach/irq.h>
#include <asm/hardware/gic.h>
#include <asm/fiq.h>
#include <asm/fiq_glue.h>
#include <mach/mt_reg_base.h>
#include <mach/irqs.h>
#include <mach/sync_write.h>
#include <mach/mt_spm_idle.h>

#define GIC_DIST_IGROUP           (0x080)
#define GIC_DIST_ACTIVE_SET             (0x300)

#define GIC_ICDISR (GIC_DIST_BASE + 0x80)
#define GIC_ICDISER0 (GIC_DIST_BASE + 0x100)
#define GIC_ICDISER1 (GIC_DIST_BASE + 0x104)
#define GIC_ICDISER2 (GIC_DIST_BASE + 0x108)
#define GIC_ICDISER3 (GIC_DIST_BASE + 0x10C)
#define GIC_ICDISER4 (GIC_DIST_BASE + 0x110)
#define GIC_ICDISER5 (GIC_DIST_BASE + 0x114)
#define GIC_ICDISER6 (GIC_DIST_BASE + 0x118)
#define GIC_ICDISER7 (GIC_DIST_BASE + 0x11C)
#define GIC_ICDICER0 (GIC_DIST_BASE + 0x180)
#define GIC_ICDICER1 (GIC_DIST_BASE + 0x184)
#define GIC_ICDICER2 (GIC_DIST_BASE + 0x188)
#define GIC_ICDICER3 (GIC_DIST_BASE + 0x18C)
#define GIC_ICDICER4 (GIC_DIST_BASE + 0x190)
#define GIC_ICDICER5 (GIC_DIST_BASE + 0x194)
#define GIC_ICDICER6 (GIC_DIST_BASE + 0x198)
#define GIC_ICDICER7 (GIC_DIST_BASE + 0x19C)
#define INT_POL_CTL0 (MCUSYS_CFGREG_BASE + 0x100)

static spinlock_t irq_lock;
/* irq_total_secondary_cpus will be initialized in smp_init_cpus() of mt-smp.c */
unsigned int irq_total_secondary_cpus = 0;
#if defined(__CHECK_IRQ_TYPE)
#define X_DEFINE_IRQ(__name, __num, __polarity, __sensitivity) \
        { .num = __num, .polarity = __polarity, .sensitivity = __sensitivity, },
#define L 0
#define H 1
#define EDGE MT_EDGE_SENSITIVE
#define LEVEL MT_LEVEL_SENSITIVE
struct __check_irq_type
{
    int num;
    int polarity;
    int sensitivity;
};
struct __check_irq_type __check_irq_type[] =
{
#include <mach/x_define_irq.h>
    { .num = -1, },
};
#undef X_DEFINE_IRQ
#undef L
#undef H
#undef EDGE
#undef LEVEL
#endif

/*
 * mt_irq_mask: enable an interrupt. 
 * @data: irq_data
 */
void mt_irq_mask(struct irq_data *data)
{
    const unsigned int irq = data->irq;
    u32 mask = 1 << (irq % 32);

    if (irq < NR_GIC_SGI) {
         /*Note: workaround for false alarm:"Fail to disable interrupt 14"*/
        if (irq != FIQ_DBG_SGI)
            printk(KERN_CRIT "Fail to disable interrupt %d\n", irq);
        return ;
    }

    *(volatile u32 *)(GIC_DIST_BASE + GIC_DIST_ENABLE_CLEAR + irq / 32 * 4) = mask;
    dsb();
}

/*
 * mt_irq_unmask: disable an interrupt. 
 * @data: irq_data
 */
void mt_irq_unmask(struct irq_data *data)
{
    const unsigned int irq = data->irq;
    u32 mask = 1 << (irq % 32);

    if (irq < NR_GIC_SGI) {
     /*Note: workaround for false alarm:"Fail to enable interrupt 14"*/
        if (irq != FIQ_DBG_SGI)
            printk(KERN_CRIT "Fail to enable interrupt %d\n", irq);
        return ;
    }

    *(volatile u32 *)(GIC_DIST_BASE + GIC_DIST_ENABLE_SET + irq / 32 * 4) = mask;
    dsb();
}

/*
 * mt_irq_ack: acknowledge an interrupt
 * @data: irq_data
 */
static void mt_irq_ack(struct irq_data *data)
{
    u32 irq = data->irq;

#if defined(CONFIG_FIQ_GLUE)
    *(volatile u32 *)(GIC_CPU_BASE + GIC_CPU_AEOI) = irq;
#else
    *(volatile u32 *)(GIC_CPU_BASE + GIC_CPU_EOI) = irq;
#endif
    dsb();
}

/*
 * mt_irq_set_sens: set the interrupt sensitivity
 * @irq: interrupt id
 * @sens: sensitivity
 */
void mt_irq_set_sens(unsigned int irq, unsigned int sens)
{
    unsigned long flags;
    u32 config;

    if (irq < (NR_GIC_SGI + NR_GIC_PPI)) {
        printk(KERN_CRIT "Fail to set sensitivity of interrupt %d\n", irq);
        return ;
    }

    spin_lock_irqsave(&irq_lock, flags);

    if (sens == MT_EDGE_SENSITIVE) {
        config = readl(IOMEM(GIC_DIST_BASE + GIC_DIST_CONFIG + (irq / 16) * 4));
        config |= (0x2 << (irq % 16) * 2);
        writel(config, IOMEM(GIC_DIST_BASE + GIC_DIST_CONFIG + (irq / 16) * 4));
    } else {
        config = readl(IOMEM(GIC_DIST_BASE + GIC_DIST_CONFIG + (irq / 16) * 4));
        config &= ~(0x2 << (irq % 16) * 2);
        writel(config, IOMEM(GIC_DIST_BASE + GIC_DIST_CONFIG + (irq / 16) * 4));
    }

    spin_unlock_irqrestore(&irq_lock, flags);

    dsb();
}

/*
 * mt_irq_set_polarity: set the interrupt polarity
 * @irq: interrupt id
 * @polarity: interrupt polarity
 */
void mt_irq_set_polarity(unsigned int irq, unsigned int polarity)
{
    unsigned long flags;
    u32 offset, reg_index, value;

    if (irq < (NR_GIC_SGI + NR_GIC_PPI)) {
        printk(KERN_CRIT "Fail to set polarity of interrupt %d\n", irq);
        return ;
    }

    offset = (irq - GIC_PRIVATE_SIGNALS) & 0x1F;
    reg_index = (irq - GIC_PRIVATE_SIGNALS) >> 5;

    spin_lock_irqsave(&irq_lock, flags);

    if (polarity == 0) {
        /* active low */
        value = readl(IOMEM(INT_POL_CTL0 + (reg_index * 4)));
        value |= (1 << offset);
        mt65xx_reg_sync_writel(value, (INT_POL_CTL0 + (reg_index * 4)));
    } else {
        /* active high */
        value = readl(IOMEM(INT_POL_CTL0 + (reg_index * 4)));
        value &= ~(0x1 << offset);
        mt65xx_reg_sync_writel(value, INT_POL_CTL0 + (reg_index * 4));
    }

    spin_unlock_irqrestore(&irq_lock, flags);
}

/*
 * mt_irq_set_type: set interrupt type
 * @irq: interrupt id
 * @flow_type: interrupt type
 * Always return 0.
 */
static int mt_irq_set_type(struct irq_data *data, unsigned int flow_type)
{
    const unsigned int irq = data->irq;

#if defined(__CHECK_IRQ_TYPE)
    if (irq > (NR_GIC_SGI + NR_GIC_PPI)) {
        int i = 0;

        while (__check_irq_type[i].num >= 0) {
            if (__check_irq_type[i].num == irq) {
                if (flow_type & (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING)) {
                    if (__check_irq_type[i].sensitivity != MT_EDGE_SENSITIVE) {
                        pr_err("irq_set_type_error: level-sensitive interrupt %d is set as edge-trigger\n", irq);
                    }
                } else if (flow_type & (IRQF_TRIGGER_HIGH | IRQF_TRIGGER_LOW)) {
                    if (__check_irq_type[i].sensitivity != MT_LEVEL_SENSITIVE) {
                        pr_err("irq_set_type_error: edge-trigger interrupt %d is set as level-sensitive\n", irq);
                    }
                }

                if (flow_type & (IRQF_TRIGGER_RISING | IRQF_TRIGGER_HIGH)) {
                    if (__check_irq_type[i].polarity != 1) {
                        pr_err("irq_set_type error: active-low interrupt %d is set as active-high\n", irq);
                    }
                } else if (flow_type & (IRQF_TRIGGER_FALLING | IRQF_TRIGGER_LOW)) {
                    if (__check_irq_type[i].polarity != 0) {
                        pr_err("irq_set_type error: active-high interrupt %d is set as active-low\n", irq);
                    }
                }

                break;
            } else {
                i++;
            }
        }
        if (__check_irq_type[i].num < 0) {
            pr_err("irq_set_type error: unknown interrupt %d is configured\n", irq);
        }
    }
#endif

    if (flow_type & (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING)) {
        mt_irq_set_sens(irq, MT_EDGE_SENSITIVE);
        mt_irq_set_polarity(irq, (flow_type & IRQF_TRIGGER_FALLING) ? 0 : 1);
        __irq_set_handler_locked(irq, handle_edge_irq);
    } else if (flow_type & (IRQF_TRIGGER_HIGH | IRQF_TRIGGER_LOW)) {
        mt_irq_set_sens(irq, MT_LEVEL_SENSITIVE);
        mt_irq_set_polarity(irq, (flow_type & IRQF_TRIGGER_LOW) ? 0 : 1);
        __irq_set_handler_locked(irq, handle_level_irq);
    }

    return 0;
}

static struct irq_chip mt_irq_chip = {
    .irq_disable = mt_irq_mask,
    .irq_enable = mt_irq_unmask,
    .irq_ack = mt_irq_ack,
    .irq_mask = mt_irq_mask,
    .irq_unmask = mt_irq_unmask,
    .irq_set_type = mt_irq_set_type,
};

static void mt_gic_dist_init(void)
{
    unsigned int i;
    u32 cpumask = 1 << smp_processor_id();

    cpumask |= cpumask << 8;
    cpumask |= cpumask << 16;

    writel(0, IOMEM(GIC_DIST_BASE + GIC_DIST_CTRL));

    /*
     * Set all global interrupts to be level triggered, active low.
     */
    for (i = 32; i < (MT_NR_SPI + 32); i += 16) {
        writel(0, IOMEM(GIC_DIST_BASE + GIC_DIST_CONFIG + i * 4 / 16));
    }

    /*
     * Set all global interrupts to this CPU only.
     */
    for (i = 32; i < (MT_NR_SPI + 32); i += 4) {
        writel(cpumask, IOMEM(GIC_DIST_BASE + GIC_DIST_TARGET + i * 4 / 4));
    }

    /*
     * Set priority on all global interrupts.
     */
    for (i = 32; i < NR_MT_IRQ_LINE; i += 4) {
        writel(0xA0A0A0A0, IOMEM(GIC_DIST_BASE + GIC_DIST_PRI + i * 4 / 4));
    }

    /*
     * Disable all global interrupts.
     */
    for (i = 32; i < NR_MT_IRQ_LINE; i += 32) {
        writel(0xFFFFFFFF, IOMEM(GIC_DIST_BASE + GIC_DIST_ENABLE_CLEAR + i * 4 / 32)); 
    }

    /*
     * Setup the Linux IRQ subsystem.
     */
    for (i = GIC_PPI_OFFSET; i < NR_MT_IRQ_LINE; i++) {
        if(i == GIC_PPI_PRIVATE_TIMER || i == GIC_PPI_NS_PRIVATE_TIMER) {
            irq_set_percpu_devid(i);
            irq_set_chip_and_handler(i, &mt_irq_chip, handle_percpu_devid_irq);
            set_irq_flags(i, IRQF_VALID | IRQF_NOAUTOEN);
        } else {
            irq_set_chip_and_handler(i, &mt_irq_chip, handle_level_irq);
            set_irq_flags(i, IRQF_VALID | IRQF_PROBE);
        }
    }
#ifdef CONFIG_FIQ_DEBUGGER
    irq_set_chip_and_handler(FIQ_DBG_SGI, &mt_irq_chip, handle_level_irq);
    set_irq_flags(FIQ_DBG_SGI, IRQF_VALID | IRQF_PROBE);
#endif

#if defined(CONFIG_FIQ_GLUE)
    /* when enable FIQ
     * set all global interrupts as non-secure interrupts
     */
    for (i = 32; i < NR_IRQS; i += 32)
    {
        writel(0xFFFFFFFF, IOMEM(GIC_ICDISR + 4 * (i / 32)));
    }
#endif
    /*
     * enable secure and non-secure interrupts on Distributor
     */
#if defined(CONFIG_FIQ_GLUE)
    writel(3, IOMEM(GIC_DIST_BASE + GIC_DIST_CTRL));
#else
    writel(1, IOMEM(GIC_DIST_BASE + GIC_DIST_CTRL));
#endif
}

static void mt_gic_cpu_init(void)
{
    int i;

    /*
     * Deal with the banked PPI and SGI interrupts - disable all
     * PPI interrupts, ensure all SGI interrupts are enabled.
     */
    writel(0xffff0000, IOMEM(GIC_DIST_BASE + GIC_DIST_ENABLE_CLEAR));
    writel(0x0000ffff, IOMEM(GIC_DIST_BASE + GIC_DIST_ENABLE_SET));

    /* set priority on PPI and SGI interrupts */
    for (i = 0; i < 32; i += 4)
        writel(0x80808080, IOMEM(GIC_DIST_BASE + GIC_DIST_PRI + i * 4 / 4));

#if defined(CONFIG_FIQ_GLUE)
    /* when enable FIQ*/
    /* set PPI and SGI interrupts as non-secure interrupts */
    writel(0xFFFFFFFF, IOMEM(GIC_ICDISR));
#endif
    writel(0xF0, IOMEM(GIC_CPU_BASE + GIC_CPU_PRIMASK));

#if defined(CONFIG_FIQ_GLUE)
    /* enable SBPR, FIQEn, EnableNS and EnableS */
    /* SBPR=1, IRQ and FIQ use the same BPR bit*/
    /* FIQEn=1,forward group 0 interrupts using the FIQ signal
     *         GIC always signals grp1 interrupt using IRQ
     * */
    /* EnableGrp1=1, enable signaling of grp 1*/
    /* EnableGrp0=1, enable signaling of grp 0*/
    writel(0x1B, IOMEM(GIC_CPU_BASE + GIC_CPU_CTRL));
#else
    /* enable SBPR, EnableNS and EnableS */
    writel(0x13, IOMEM(GIC_CPU_BASE + GIC_CPU_CTRL));
#endif
    dsb();
}

void __cpuinit mt_gic_secondary_init(void)
{
    mt_gic_cpu_init();
}

void irq_raise_softirq(const struct cpumask *mask, unsigned int irq)
{
    unsigned long map = *cpus_addr(*mask);
    int satt, cpu, cpu_bmask;
    u32 val;

    satt = 1 << 15;
    /* 
     * NoteXXX: CPU1 SGI is configured as secure as default.
     *          Need to use the secure SGI 1 which is for waking up cpu1.
     */
    if (irq == CPU_BRINGUP_SGI)
    {
        if (irq_total_secondary_cpus)
        {
            --irq_total_secondary_cpus;
            satt = 0;
        }
    }
    
    val = readl(IOMEM(GIC_ICDISR + 4 * (irq / 32)));
    if (!(val & (1 << (irq % 32)))) {   /*  secure interrupt? */
        satt = 0;
    }

    cpu = 0;
    cpu_bmask = 0;

#if defined(SPM_MCDI_FUNC)
    /*
     * Processors cannot receive interrupts during power-down.
     * Wait until the SPM checks status and returns. 
     */
    for_each_cpu(cpu, mask) {
        cpu_bmask |= 1 << cpu;
    }
    spm_check_core_status_before(cpu_bmask);
#endif

    /*
     * Ensure that stores to Normal memory are visible to the
     * other CPUs before issuing the IPI.
     */
    dsb();
    *(volatile u32 *)(GIC_DIST_BASE + 0xf00) = (map << 16) | satt | irq;
    dsb();

#if defined(SPM_MCDI_FUNC)
    spm_check_core_status_after(cpu_bmask);
#endif

}

int mt_irq_is_active(const unsigned int irq)
{
    // under GICv1, this is pending regs for irqs    
    //const unsigned int iActive = readl(IOMEM(GIC_DIST_BASE + 0x200 + irq / 32 * 4));
    // under GICv2, this is active regs for irqs
    const unsigned int iActive = readl(IOMEM(GIC_DIST_BASE + 0x300 + irq / 32 * 4));

    return iActive & (1 << (irq % 32)) ? 1 : 0;
}

/*
 * mt_enable_ppi: enable a private peripheral interrupt
 * @irq: interrupt id
 */
void mt_enable_ppi(int irq)
{
    u32 mask = 1 << (irq % 32);

    if (irq < NR_GIC_SGI) {
        printk(KERN_CRIT "Fail to enable PPI %d\n", irq);
        return ;
    }
    if (irq >= (NR_GIC_SGI + NR_GIC_PPI)) {
        printk(KERN_CRIT "Fail to enable PPI %d\n", irq);
        return ;
    }

    *(volatile u32 *)(GIC_DIST_BASE + GIC_DIST_ENABLE_SET + irq / 32 * 4) = mask;
    dsb();
}

/*
 * mt_PPI_mask_all: disable all PPI interrupts
 * @mask: pointer to struct mtk_irq_mask for storing the original mask value.
 * Return 0 for success; return negative values for failure.
 */
int mt_PPI_mask_all(struct mtk_irq_mask *mask)
{
    unsigned long flags;

    if(mask) {
#if defined(CONFIG_FIQ_GLUE)
        local_fiq_disable();
#endif
        spin_lock_irqsave(&irq_lock, flags);
        mask->mask0 = readl(IOMEM(GIC_ICDISER0));

        mt65xx_reg_sync_writel(0xFFFFFFFF, GIC_ICDICER0);

        spin_unlock_irqrestore(&irq_lock, flags);
#if defined(CONFIG_FIQ_GLUE)
        local_fiq_enable();
#endif

        mask->header = IRQ_MASK_HEADER;
        mask->footer = IRQ_MASK_FOOTER;

        return 0;
    } else {
        return -1;
    }
}

/*
 * mt_PPI_mask_restore: restore all PPI interrupts
 * @mask: pointer to struct mtk_irq_mask for storing the original mask value.
 * Return 0 for success; return negative values for failure.
 */
int mt_PPI_mask_restore(struct mtk_irq_mask *mask)
{
    unsigned long flags;

    if (!mask) {
        return -1;
    }
    if (mask->header != IRQ_MASK_HEADER) {
        return -1;
    }
    if (mask->footer != IRQ_MASK_FOOTER) {
        return -1;
    }

#if defined(CONFIG_FIQ_GLUE)
    local_fiq_disable();
#endif
    spin_lock_irqsave(&irq_lock, flags);

    mt65xx_reg_sync_writel(mask->mask0, GIC_ICDISER0);

    spin_unlock_irqrestore(&irq_lock, flags);
#if defined(CONFIG_FIQ_GLUE)
    local_fiq_enable();
#endif

    return 0;
}

/*
 * mt_SPI_mask_all: disable all SPI interrupts
 * @mask: pointer to struct mtk_irq_mask for storing the original mask value.
 * Return 0 for success; return negative values for failure.
 */
int mt_SPI_mask_all(struct mtk_irq_mask *mask)
{
    unsigned long flags;

    if (mask) {
#if defined(CONFIG_FIQ_GLUE)
        local_fiq_disable();
#endif
        spin_lock_irqsave(&irq_lock, flags);

        mask->mask1 = readl(IOMEM(GIC_ICDISER1));
        mask->mask2 = readl(IOMEM(GIC_ICDISER2));
        mask->mask3 = readl(IOMEM(GIC_ICDISER3));
        mask->mask4 = readl(IOMEM(GIC_ICDISER4));
        mask->mask5 = readl(IOMEM(GIC_ICDISER5));
        mask->mask6 = readl(IOMEM(GIC_ICDISER6));
        mask->mask7 = readl(IOMEM(GIC_ICDISER7));

        writel(0xFFFFFFFF, IOMEM(GIC_ICDICER1));
        writel(0xFFFFFFFF, IOMEM(GIC_ICDICER2));
        writel(0xFFFFFFFF, IOMEM(GIC_ICDICER3));
        writel(0xFFFFFFFF, IOMEM(GIC_ICDICER4));
        writel(0xFFFFFFFF, IOMEM(GIC_ICDICER5));
        writel(0xFFFFFFFF, IOMEM(GIC_ICDICER6));
        mt65xx_reg_sync_writel(0xFFFFFFFF, GIC_ICDICER7);

        spin_unlock_irqrestore(&irq_lock, flags);
#if defined(CONFIG_FIQ_GLUE)
        local_fiq_enable();
#endif

        mask->header = IRQ_MASK_HEADER;
        mask->footer = IRQ_MASK_FOOTER;

        return 0;
    } else {
        return -1;
    }
}

/*
 * mt_SPI_mask_restore: restore all SPI interrupts
 * @mask: pointer to struct mtk_irq_mask for storing the original mask value.
 * Return 0 for success; return negative values for failure.
 */
int mt_SPI_mask_restore(struct mtk_irq_mask *mask)
{
    unsigned long flags;

    if (!mask) {
        return -1;
    }
    if (mask->header != IRQ_MASK_HEADER) {
        return -1;
    }
    if (mask->footer != IRQ_MASK_FOOTER) {
        return -1;
    }

#if defined(CONFIG_FIQ_GLUE)
    local_fiq_disable();
#endif
    spin_lock_irqsave(&irq_lock, flags);

    writel(mask->mask1, IOMEM(GIC_ICDISER1));
    writel(mask->mask2, IOMEM(GIC_ICDISER2));
    writel(mask->mask3, IOMEM(GIC_ICDISER3));
    writel(mask->mask4, IOMEM(GIC_ICDISER4));
    writel(mask->mask5, IOMEM(GIC_ICDISER5));
    writel(mask->mask6, IOMEM(GIC_ICDISER6));
    mt65xx_reg_sync_writel(mask->mask7, GIC_ICDISER7);

    spin_unlock_irqrestore(&irq_lock, flags);
#if defined(CONFIG_FIQ_GLUE)
    local_fiq_enable();
#endif

    return 0;
}

/*
 * mt_irq_mask_all: disable all interrupts
 * @mask: pointer to struct mtk_irq_mask for storing the original mask value.
 * Return 0 for success; return negative values for failure.
 * (This is ONLY used for the idle current measurement by the factory mode.)
 */
int mt_irq_mask_all(struct mtk_irq_mask *mask)
{
    unsigned long flags;

    if (mask) {
#if defined(CONFIG_FIQ_GLUE)
        local_fiq_disable();
#endif
        spin_lock_irqsave(&irq_lock, flags);

        mask->mask0 = readl(IOMEM(GIC_ICDISER0));
        mask->mask1 = readl(IOMEM(GIC_ICDISER1));
        mask->mask2 = readl(IOMEM(GIC_ICDISER2));
        mask->mask3 = readl(IOMEM(GIC_ICDISER3));
        mask->mask4 = readl(IOMEM(GIC_ICDISER4));
        mask->mask5 = readl(IOMEM(GIC_ICDISER5));
        mask->mask6 = readl(IOMEM(GIC_ICDISER6));
        mask->mask7 = readl(IOMEM(GIC_ICDISER7));

        writel(0xFFFFFFFF, IOMEM(GIC_ICDICER0));
        writel(0xFFFFFFFF, IOMEM(GIC_ICDICER1));
        writel(0xFFFFFFFF, IOMEM(GIC_ICDICER2));
        writel(0xFFFFFFFF, IOMEM(GIC_ICDICER3));
        writel(0xFFFFFFFF, IOMEM(GIC_ICDICER4));
        writel(0xFFFFFFFF, IOMEM(GIC_ICDICER5));
        writel(0xFFFFFFFF, IOMEM(GIC_ICDICER6));
        mt65xx_reg_sync_writel(0xFFFFFFFF, GIC_ICDICER7);

        spin_unlock_irqrestore(&irq_lock, flags);
#if defined(CONFIG_FIQ_GLUE)
        local_fiq_enable();
#endif

        mask->header = IRQ_MASK_HEADER;
        mask->footer = IRQ_MASK_FOOTER;

        return 0;
    } else {
        return -1;
    }
}

/*
 * mt_irq_mask_restore: restore all interrupts 
 * @mask: pointer to struct mtk_irq_mask for storing the original mask value.
 * Return 0 for success; return negative values for failure.
 * (This is ONLY used for the idle current measurement by the factory mode.)
 */
int mt_irq_mask_restore(struct mtk_irq_mask *mask)
{
    unsigned long flags;

    if (!mask) {
        return -1;
    }
    if (mask->header != IRQ_MASK_HEADER) {
        return -1;
    }
    if (mask->footer != IRQ_MASK_FOOTER) {
        return -1;
    }

#if defined(CONFIG_FIQ_GLUE)
    local_fiq_disable();
#endif
    spin_lock_irqsave(&irq_lock, flags);

    writel(mask->mask0, IOMEM(GIC_ICDISER0));
    writel(mask->mask1, IOMEM(GIC_ICDISER1));
    writel(mask->mask2, IOMEM(GIC_ICDISER2));
    writel(mask->mask3, IOMEM(GIC_ICDISER3));
    writel(mask->mask4, IOMEM(GIC_ICDISER4));
    writel(mask->mask5, IOMEM(GIC_ICDISER5));
    writel(mask->mask6, IOMEM(GIC_ICDISER6));
    mt65xx_reg_sync_writel(mask->mask7, GIC_ICDISER7);

    spin_unlock_irqrestore(&irq_lock, flags);
#if defined(CONFIG_FIQ_GLUE)
    local_fiq_enable();
#endif

    return 0;
}
/*
#define GIC_DIST_PENDING_SET            0x200
#define GIC_DIST_PENDING_CLEAR          0x280

 * mt_irq_set_pending_for_sleep: pending an interrupt for the sleep manager's use
 * @irq: interrupt id
 * (THIS IS ONLY FOR SLEEP FUNCTION USE. DO NOT USE IT YOURSELF!)
 */
void mt_irq_set_pending_for_sleep(unsigned int irq)
{
    u32 mask = 1 << (irq % 32);

    if (irq < NR_GIC_SGI) {
        printk(KERN_CRIT "Fail to set a pending on interrupt %d\n", irq);
        return ;
    }

    *(volatile u32 *)(GIC_DIST_BASE + GIC_DIST_PENDING_SET + irq / 32 * 4) = mask;
    printk(KERN_CRIT "irq:%d, 0x%x=0x%x\n", irq, GIC_DIST_BASE + GIC_DIST_PENDING_SET + irq / 32 * 4,mask);
    dsb();
}

/*
 * mt_irq_unmask_for_sleep: enable an interrupt for the sleep manager's use
 * @irq: interrupt id
 * (THIS IS ONLY FOR SLEEP FUNCTION USE. DO NOT USE IT YOURSELF!)
 */
void mt_irq_unmask_for_sleep(unsigned int irq)
{
    u32 mask = 1 << (irq % 32);

    if (irq < NR_GIC_SGI) {
        printk(KERN_CRIT "Fail to enable interrupt %d\n", irq);
        return ;
    }

    *(volatile u32 *)(GIC_DIST_BASE + GIC_DIST_ENABLE_SET + irq / 32 * 4) = mask;
    dsb();
}

/*
 * mt_irq_mask_for_sleep: disable an interrupt for the sleep manager's use
 * @irq: interrupt id
 * (THIS IS ONLY FOR SLEEP FUNCTION USE. DO NOT USE IT YOURSELF!)
 */
void mt_irq_mask_for_sleep(unsigned int irq)
{
    u32 mask = 1 << (irq % 32);

    if (irq < NR_GIC_SGI) {
        printk(KERN_CRIT "Fail to enable interrupt %d\n", irq);
        return ;
    }

    *(volatile u32 *)(GIC_DIST_BASE + GIC_DIST_ENABLE_CLEAR + irq / 32 * 4) = mask;
    dsb();
}

#if defined(CONFIG_FIQ_GLUE)

struct irq2fiq
{
    int irq;
    fiq_isr_handler handler;
    void *arg;
};

static struct irq2fiq irqs_to_fiq[] =
{
    { .irq = MT_UART1_IRQ_ID, },
    { .irq = MT_UART2_IRQ_ID, },
    { .irq = MT_UART3_IRQ_ID, },
    { .irq = MT_UART4_IRQ_ID, },
    { .irq = MT_WDT_IRQ_ID, },
    { .irq = MT_SPM1_IRQ_ID, },
    { .irq = GIC_PPI_NS_PRIVATE_TIMER, },
    { .irq = FIQ_SMP_CALL_SGI, }
};

struct fiq_isr_log
{
    int in_fiq_isr;
    int irq;
    int smp_call_cnt;
};

static struct fiq_isr_log fiq_isr_logs[NR_CPUS];

static int fiq_glued;

/*
 * trigger_sw_irq: force to trigger a GIC SGI.
 * @irq: SGI number
 */
void trigger_sw_irq(int irq)
{
    if (irq < NR_GIC_SGI) {
        *((volatile unsigned int *)(GIC_DIST_BASE + GIC_DIST_SOFTINT)) = 0x18000 | irq;
    }
}

/*
 * mt_disable_fiq: disable an interrupt which is directed to FIQ.
 * @irq: interrupt id
 * Return error code.
 * NoteXXX: Not allow to suspend due to FIQ context.
 */
int mt_disable_fiq(int irq)
{
    int i;
    struct irq_data data;

    for (i = 0; i < ARRAY_SIZE(irqs_to_fiq); i++) {
        if (irqs_to_fiq[i].irq == irq) {
            data.irq = irq;
            mt_irq_mask(&data);
            return 0;
        }
    }

    return -1;
}

/*
 * mt_enable_fiq: enable an interrupt which is directed to FIQ.
 * @irq: interrupt id
 * Return error code.
 * NoteXXX: Not allow to suspend due to FIQ context.
 */
int mt_enable_fiq(int irq)
{
    int i;
    struct irq_data data;

    for (i = 0; i < ARRAY_SIZE(irqs_to_fiq); i++) {
        if (irqs_to_fiq[i].irq == irq) {
            data.irq = irq;
            mt_irq_unmask(&data);
            return 0;
        }
    }

    return -1;
}

/*
 * fiq_isr: FIQ handler.
 */
static void fiq_isr(struct fiq_glue_handler *h, void *regs, void *svc_sp)
{
    unsigned int iar, irq;
    int cpu, i;

    iar = *((volatile unsigned int*)(GIC_CPU_BASE + GIC_CPU_INTACK));
    irq = iar & 0x3FF;

    cpu = 0;
    asm volatile (                                                                                "MRC p15, 0, %0, c0, c0, 5\n"
        "AND %0, %0, #3\n"
        : "+r"(cpu)
        :
        : "cc"
    );

    fiq_isr_logs[cpu].irq = irq;

    if (irq >= NR_IRQS) {
        return;
    }

    fiq_isr_logs[cpu].in_fiq_isr = 1;

    if (irq == FIQ_SMP_CALL_SGI) {
        fiq_isr_logs[cpu].smp_call_cnt++;
    }
    if (irq == MT_WDT_IRQ_ID) {
        aee_rr_rec_fiq_step(AEE_FIQ_STEP_FIQ_ISR_BASE);
    }
    for (i = 0; i < ARRAY_SIZE(irqs_to_fiq); i++) {
        if (irqs_to_fiq[i].irq == irq) {
            if (irqs_to_fiq[i].handler) {
                (irqs_to_fiq[i].handler)(irqs_to_fiq[i].arg, regs, svc_sp);
            }
            break;
        }
    }
    if (i == ARRAY_SIZE(irqs_to_fiq)) {
        register int sp asm("sp");
        struct pt_regs *ptregs = (struct pt_regs *)regs;

        asm volatile("mov %0, %1\n"
                     "mov fp, %2\n"
                     : "=r" (sp)
                     : "r" (svc_sp), "r" (ptregs->ARM_fp)
                     : "cc"
                     );
        preempt_disable();
        pr_err("Interrupt %d triggers FIQ but it is not registered\n", irq);
    }

    fiq_isr_logs[cpu].in_fiq_isr = 0;

    *(volatile unsigned int *)(GIC_CPU_BASE + GIC_CPU_EOI) = iar;

    dsb();
}

/*
 * get_fiq_isr_log: get fiq_isr_log for debugging.
 * @cpu: processor id
 * @log: pointer to the address of fiq_isr_log
 * @len: length of fiq_isr_log
 * Return 0 for success or error code for failure.
 */
int get_fiq_isr_log(int cpu, unsigned int *log, unsigned int *len)
{
    if (log) {
        *log = (unsigned int)&(fiq_isr_logs[cpu]);
    }
    if (len) {
        *len = sizeof(struct fiq_isr_log);
    }

    return 0;
}

static void __set_security(int irq)
{
    u32 val;
    unsigned long flags;

    spin_lock_irqsave(&irq_lock, flags);

    val = readl(IOMEM(GIC_ICDISR + 4 * (irq / 32)));
    val &= ~(1 << (irq % 32));
    writel(val, IOMEM(GIC_ICDISR + 4 * (irq / 32)));

    spin_unlock_irqrestore(&irq_lock, flags);
}

static void __raise_priority(int irq)
{
    u32 val;
    unsigned long flags;
    
    spin_lock_irqsave(&irq_lock, flags);

    val = readl(IOMEM(GIC_DIST_BASE + GIC_DIST_PRI + 4 * (irq / 4)));
    val &= ~(0xFF << ((irq % 4) * 8));
    val |= (0x50 << ((irq % 4) * 8));
    writel(val, IOMEM(GIC_DIST_BASE + GIC_DIST_PRI + 4 * (irq / 4)));

    spin_unlock_irqrestore(&irq_lock, flags);
}

static int
restore_for_fiq(struct notifier_block *nfb, unsigned long action, void *hcpu)
{
    int i;

    switch (action) {
    case CPU_STARTING:
        for (i = 0; i < ARRAY_SIZE(irqs_to_fiq); i++) {
            if ((irqs_to_fiq[i].irq < (NR_GIC_SGI + NR_GIC_PPI))
               && (irqs_to_fiq[i].handler)) {
                __set_security(irqs_to_fiq[i].irq);
                __raise_priority(irqs_to_fiq[i].irq);
                dsb();
            }
        }
        break;

    default:
        break;
    }

    return NOTIFY_OK;
}

static struct notifier_block __cpuinitdata fiq_notifier = {
    .notifier_call = restore_for_fiq,
};

static struct fiq_glue_handler fiq_handler =
{
    .fiq = fiq_isr,
};

static int __init_fiq(void)
{
    int ret;

    register_cpu_notifier(&fiq_notifier);

    ret = fiq_glue_register_handler(&fiq_handler);
    if (ret) {
        pr_err("fail to register fiq_glue_handler\n");
    } else {
        fiq_glued = 1;
    }

    return ret;
}

/*
 * request_fiq: direct an interrupt to FIQ and register its handler.
 * @irq: interrupt id
 * @handler: FIQ handler
 * @irq_flags: IRQF_xxx
 * @arg: argument to the FIQ handler
 * Return error code.
 *
 * 1.set grp0
 * 2.raise priority
 */
int request_fiq(int irq, fiq_isr_handler handler, unsigned long irq_flags, void *arg)
{
    int i;
    unsigned long flags;
    struct irq_data data;

    if (!fiq_glued) {
        __init_fiq();
    }

    for (i = 0; i < ARRAY_SIZE(irqs_to_fiq); i++) {
        spin_lock_irqsave(&irq_lock, flags);

        if (irqs_to_fiq[i].irq == irq) {

            irqs_to_fiq[i].handler = handler;
            irqs_to_fiq[i].arg = arg;

            spin_unlock_irqrestore(&irq_lock, flags);

            __set_security(irq);
            __raise_priority(irq); 
            data.irq = irq;
            mt_irq_set_type(&data, irq_flags);

            mb();

            mt_irq_unmask(&data);

            return 0;
        }

        spin_unlock_irqrestore(&irq_lock, flags);
    }

    return -1;
}

#endif  /* CONFIG_FIQ_GLUE */

void __init mt_init_irq(void)
{
    spin_lock_init(&irq_lock);
    mt_gic_dist_init();
    mt_gic_cpu_init();
}

static unsigned int get_mask(int irq)
{
   unsigned int bit; 
   bit = 1 << (irq % 32);
   return ((readl(IOMEM(GIC_DIST_BASE + GIC_DIST_ENABLE_SET + irq / 32 * 4)) & bit)?1:0);
}

static unsigned int get_grp(int irq)
{
   unsigned int bit; 
   bit = 1 << (irq % 32);
   //0x1:irq,0x0:fiq
   return ((readl(IOMEM(GIC_DIST_BASE + GIC_DIST_IGROUP + irq / 32 * 4)) & bit)?1:0);
}
static unsigned int get_pri(int irq)
{
   unsigned int bit; 
   bit = 0xff << ((irq % 4)*8);
   return ((readl(IOMEM(GIC_DIST_BASE + GIC_DIST_PRI + irq / 4 * 4)) & bit) >> ((irq % 4)*8));
}
static unsigned int get_target(int irq)
{
   unsigned int bit; 
   bit = 0xff << ((irq % 4)*8);
   return ((readl(IOMEM(GIC_DIST_BASE + GIC_DIST_TARGET + irq / 4 * 4)) & bit) >> ((irq % 4)*8));
}
static unsigned int get_sens(int irq)
{
   unsigned int bit; 
   bit = 0x3 << ((irq % 16)*2);
   //edge:0x2, level:0x1
   return ((readl(IOMEM(GIC_DIST_BASE + GIC_DIST_CONFIG + irq / 16 * 4)) & bit) >> ((irq % 16)*2));
}
static unsigned int get_pending_status(int irq)
{
   unsigned int bit; 
   bit = 1 << (irq % 32);
   return ((readl(IOMEM(GIC_DIST_BASE + GIC_DIST_PENDING_SET + irq / 32 * 4)) & bit)?1:0);
}

static unsigned int get_active_status(int irq)
{

   unsigned int bit; 
   bit = 1 << (irq % 32);
   return ((readl(IOMEM(GIC_DIST_BASE + GIC_DIST_ACTIVE_SET + irq / 32 * 4)) & bit)?1:0);
}

static unsigned int get_pol(int irq)
{
 unsigned int bit;
 bit = 1 << (irq % 32);
 //0x0: high, 0x1:low
 return ((readl(IOMEM(INT_POL_CTL0 + ((irq-32) / 32 * 4))) & bit)?1:0);
}

void mt_irq_dump_status(int irq)
{
    if (irq >= NR_IRQS)
        return ;
    printk("[INT] irq:%d,enable:%x,active:%x,pending:%x,pri:%x,grp:%x,target:%x,sens:%x,pol:%x\n",irq,get_mask(irq),get_active_status(irq),get_pending_status(irq),get_pri(irq),get_grp(irq),get_target(irq),get_sens(irq),get_pol(irq));

}
# if 0
//# test code
int set_mask(int irq)
{
   unsigned int bit; 
   bit = 1 << (irq % 32);
   writel(bit, IOMEM(GIC_DIST_BASE + GIC_DIST_ENABLE_SET + irq / 32 * 4));
}
int set_pending_status(int irq)
{

   unsigned int bit; 
   bit = 1 << (irq % 32);
   writel(bit, IOMEM(GIC_DIST_BASE + GIC_DIST_PENDING_SET + irq / 32 * 4));
}

int set_active_status(int irq)
{

   unsigned int bit; 
   bit = 1 << (irq % 32);
   writel(bit, IOMEM(GIC_DIST_BASE + GIC_DIST_ACTIVE_SET + irq / 32 * 4));
}


void test_irq_dump_status(int irq)
{
    irq_dump_status(irq);
    set_active_status(irq);
    set_pending_status(irq);
    set_mask(irq);
    irq_dump_status(irq);
}
#endif
EXPORT_SYMBOL(mt_irq_dump_status);
EXPORT_SYMBOL(mt_irq_set_sens);
EXPORT_SYMBOL(mt_irq_set_polarity);
