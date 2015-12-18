#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/kallsyms.h>
#include <linux/utsname.h>
#include <asm/uaccess.h>
#include <linux/stacktrace.h>
#include <asm/stacktrace.h>

#include <linux/pid.h>
#include <linux/debug_locks.h>

#include <linux/delay.h>

#define pr_fmt(fmt) "["KBUILD_MODNAME"]" fmt


#define SEQ_printf(m, x...)	    \
 do {			    \
    if (m) {		    \
	seq_printf(m, x);	\
	pr_debug(x);	    \
    } else		    \
	pr_debug(x);	    \
 } while (0)

#define MT_DEBUG_ENTRY(name) \
static int mt_##name##_show(struct seq_file *m, void *v);\
static int mt_##name##_write(struct file *filp, const char *ubuf, size_t cnt, loff_t *data);\
static int mt_##name##_open(struct inode *inode, struct file *file) \
{ \
    return single_open(file, mt_##name##_show, inode->i_private); \
} \
\
static const struct file_operations mt_##name##_fops = { \
    .open = mt_##name##_open, \
    .write = mt_##name##_write,\
    .read = seq_read, \
    .llseek = seq_lseek, \
    .release = single_release, \
};\
void mt_##name##_switch(int on);

#include <linux/mt_export.h>

static void enable_RA_mode(void)
{
	int temp;
	asm volatile ("MRC p15, 0, %0, c1, c0, 1\n" "BIC %0, %0, #1 << 11\n"	/* enable */
		      "BIC %0, %0, #1 << 12\n"	/* enable */
		      "MCR p15, 0, %0, c1, c0, 1\n":"+r" (temp)
 :  : "cc");
	/* printk(KERN_EMERG"temp = 0x%x\n", temp); */
	pr_debug("temp = 0x%x\n", temp);
}

static void disable_RA_mode(void)
{
	int temp;
	asm volatile ("MRC p15, 0, %0, c1, c0, 1\n" "ORR %0, %0, #1 << 11\n"	/* disable */
		      "ORR %0, %0, #1 << 12\n"	/* disable */
		      "MCR p15, 0, %0, c1, c0, 1\n":"+r" (temp)
 :  : "cc");
}

extern void inner_dcache_flush_all(void);
static char buffer_src[4 * 1024 * 1024 + 16];
static char buffer_dst[4 * 1024 * 1024 + 16];
static int flush_cache;

#include "attu_test_instr.h"
static void test_instr(int printlog)
{
	unsigned long copy_size;
	unsigned long flags;
	int i, j, avg, pld_dst;
	int k;
	int temp;
	int result[10];
	copy_size = 256;
	/* copy_size = 1024*8; */
	/* copy_size = 1024*64; */
	/* copy_size = 1024*1024; */
	if (printlog == 1)
		pr_debug("\n\n\r == Start test Pattern 2 ===\n\r");
	/* for(i = 0; i< 256; i++, copy_size += 256){ */
	/* for(i = 0; i< 8; i++, copy_size += 1024*8){ */
	/* for(i = 0; i< 16; i++, copy_size += 1024*64){ */
	/* for(i = 0; i< 4; i++, copy_size += 1024*1024){ */
	/* printk(KERN_EMERG"\n\r %lu ",copy_size); */
	i = 0;
	while (i < 256 + 16 + 4) {
		if (i < 256) {
			copy_size = 256 + i * 256;	/* inc 256 byte from 0~64 KBytes */
		} else if (i < 256 + 16) {
			copy_size = 1024 * 64 + (i - 256) * 1024 * 64;	/* inc 64Kbyte form 64KB~1MB */
		} else if (i < 256 + 16 + 4) {
			copy_size = 1024 * 1024 + (i - 256 - 16) * 1024 * 1024;	/* inc 1MB from 1MB~4MB */
		}
		i++;
		mdelay(5);
		preempt_disable();
		local_irq_save(flags);

		inner_dcache_flush_all();
		if (printlog == 1)
			pr_debug(" %lu :", copy_size);
		avg = 0;
		for (j = 0; j < 8; j++) {
			mdelay(3);
			if (flush_cache == 1)
				inner_dcache_flush_all();
#if 1
			/* reset RA mode */
#if 0
			asm volatile ("MRC p15, 0, %0, c1, c0, 1\n" "ORR %0, %0, #1 << 11\n"	/* disable */
				      "ORR %0, %0, #1 << 12\n"	/* disable */
				      "MCR p15, 0, %0, c1, c0, 1\n" "MRC p15, 0, %0, c1, c0, 1\n" "BIC %0, %0, #1 << 11\n"	/* enable */
				      "BIC %0, %0, #1 << 12\n"	/* enable */
				      "MCR p15, 0, %0, c1, c0, 1\n":"+r" (temp)
 :  : "cc");
#endif
			/* enable ARM CPU PMU */
			asm volatile ("MRC p15, 0, %0, c9, c12, 0\n" "BIC %0, %0, #1 << 0\n"	/* disable */
				      "ORR %0, %0, #1 << 2\n"	/* reset cycle count */
				      "BIC %0, %0, #1 << 3\n"	/* count every clock cycle */
				      "MCR p15, 0, %0, c9, c12, 0\n":"+r" (temp)
 :  : "cc");
			asm volatile ("MRC p15, 0, %0, c9, c12, 0\n" "ORR %0, %0, #1 << 0\n"	/* enable */
				      "MCR p15, 0, %0, c9, c12, 0\n"
				      "MRC p15, 0, %0, c9, c12, 1\n"
				      "ORR %0, %0, #1 << 31\n"
				      "MCR p15, 0, %0, c9, c12, 1\n":"+r" (temp)
 :  : "cc");
#endif
			asm volatile ("push {r0,r1,r2,r9}\n"
				      "mov r0, %0\n"
				      "mov r1, %1\n"
				      "mov r2, %2\n"
				      "mov r9, %3\n"
				      "stmfd        sp!, {r3, r4, r5, r6, r7, r8, r12, lr}\n" "1:\n"
				      /* "pld         [r1, r9]\n" */
				      /* "pldw        [r0, r9]\n" */
				      /* ".word 0xf790f009\n" */
				      "ldmia       r1!, {r3, r4, r5, r6, r7, r8, r12, lr}\n"
				      "subs        r2, r2, #32 \n"
				      "stmia       r0!, {r3, r4, r5, r6, r7, r8, r12, lr}\n"
				      "blt 2f\n"
				      "ldmia       r1!, {r3, r4, r5, r6, r7, r8, r12, lr}\n"
				      "subs        r2, r2, #32 \n"
				      "stmia       r0!, {r3, r4, r5, r6, r7, r8, r12, lr}\n"
				      "bge 1b\n"
				      "2:\n"
				      "ldmfd       sp!, {r3, r4, r5, r6, r7, r8, r12, lr}\n"
				      "pop {r0,r1,r2,r9}\n" :  : "r" (&buffer_dst), "r"(&buffer_src),
				      "r"(copy_size), "r"(pld_dst)
 : );


			/* #if defined(DEBUG_DRAMC_CALIB) */
#if 1
			/* get CPU cycle count from the ARM CPU PMU */
			asm volatile ("MRC p15, 0, %0, c9, c12, 0\n" "BIC %0, %0, #1 << 0\n"	/* disable */
				      "MCR p15, 0, %0, c9, c12, 0\n"
				      "MRC p15, 0, %0, c9, c13, 0\n":"+r" (temp)
 :  : "cc");
#endif
			result[j] = temp;
			/* printk(KERN_CONT" %d ", temp); */
		}
		avg = 0;
		for (j = 0; j < 8; j++) {
			avg += result[j];
		}
		avg = avg >> 3;
		if (printlog == 1)
			pr_debug(" %d ", avg);
		for (pld_dst = 0; pld_dst < 64 * 16; pld_dst += 64) {
			for (j = 0; j < 8; j++) {
				mdelay(3);
				if (flush_cache == 1)
					inner_dcache_flush_all();
#if 1
#if 0
				/* reset RA mode */
				asm volatile ("MRC p15, 0, %0, c1, c0, 1\n" "ORR %0, %0, #1 << 11\n"	/* disable */
					      "ORR %0, %0, #1 << 12\n"	/* disable */
					      "MCR p15, 0, %0, c1, c0, 1\n" "MRC p15, 0, %0, c1, c0, 1\n" "BIC %0, %0, #1 << 11\n"	/* enable */
					      "BIC %0, %0, #1 << 12\n"	/* enable */
					      "MCR p15, 0, %0, c1, c0, 1\n":"+r" (temp)
 :  : "cc");
#endif
				/* enable ARM CPU PMU */
				asm volatile ("MRC p15, 0, %0, c9, c12, 0\n" "BIC %0, %0, #1 << 0\n"	/* disable */
					      "ORR %0, %0, #1 << 2\n"	/* reset cycle count */
					      "BIC %0, %0, #1 << 3\n"	/* count every clock cycle */
					      "MCR p15, 0, %0, c9, c12, 0\n":"+r" (temp)
 :  : "cc");
				asm volatile ("MRC p15, 0, %0, c9, c12, 0\n" "ORR %0, %0, #1 << 0\n"	/* enable */
					      "MCR p15, 0, %0, c9, c12, 0\n"
					      "MRC p15, 0, %0, c9, c12, 1\n"
					      "ORR %0, %0, #1 << 31\n"
					      "MCR p15, 0, %0, c9, c12, 1\n":"+r" (temp)
 :  : "cc");
#endif
				asm volatile ("push {r0,r1,r2,r9}\n"
					      "mov r0, %0\n"
					      "mov r1, %1\n"
					      "mov r2, %2\n"
					      "mov r9, %3\n"
					      "stmfd        sp!, {r3, r4, r5, r6, r7, r8, r12, lr}\n"
					      "1:\n"
					      /* "pld         [r1, r9]\n" */
					      /* "pldw        [r0, r9]\n" */
					      /* ".word 0xf790f009\n" */
					      "ldmia       r1!, {r3, r4, r5, r6, r7, r8, r12, lr}\n"
					      "subs        r2, r2, #32 \n"
					      "stmia       r0!, {r3, r4, r5, r6, r7, r8, r12, lr}\n"
					      "blt 2f\n"
					      "ldmia       r1!, {r3, r4, r5, r6, r7, r8, r12, lr}\n"
					      "subs        r2, r2, #32 \n"
					      "stmia       r0!, {r3, r4, r5, r6, r7, r8, r12, lr}\n"
					      "bge 1b\n"
					      "2:\n"
					      "ldmfd       sp!, {r3, r4, r5, r6, r7, r8, r12, lr}\n"
					      "pop {r0,r1,r2,r9}\n"
					      /* :: "r"(&buffer_dst+(pld_dst>>6)), "r"(&buffer_src+(pld_dst>>6)), "r"(copy_size), "r"(pld_dst) */
 :  : "r" (&buffer_dst), "r"(&buffer_src), "r"(copy_size),
					      "r"(pld_dst)
 : );


				/* #if defined(DEBUG_DRAMC_CALIB) */
#if 1
				/* get CPU cycle count from the ARM CPU PMU */
				asm volatile ("MRC p15, 0, %0, c9, c12, 0\n" "BIC %0, %0, #1 << 0\n"	/* disable */
					      "MCR p15, 0, %0, c9, c12, 0\n"
					      "MRC p15, 0, %0, c9, c13, 0\n":"+r" (temp)
 :  : "cc");
#endif
				result[j] = temp;
				/* printk(KERN_CONT" %d ", temp); */
			}
			avg = 0;
			for (j = 0; j < 8; j++) {
				avg += result[j];
			}
			avg = avg >> 3;
			if (printlog == 1)
				pr_debug(" %d ", avg);
		}
		local_irq_restore(flags);
		preempt_enable();
	}
	if (printlog == 1)
		pr_debug("\n\r ==== test done ==== flush_cache:%d\n", flush_cache);
}

static void test_instr2(int printlog)
{
	unsigned long copy_size;
	unsigned long flags;
	unsigned long i, j, avg, pld_dst;
	unsigned long temp;
	unsigned long result[10];
	/* copy_size = 256; */
	copy_size = 1024 * 1024;

	preempt_disable();
	local_irq_save(flags);
	if (printlog == 1)
		pr_debug("\n\n\r == Start test Pattern 1 ===\n\r");
	/* for(i = 0; i< 256; i++, copy_size += 256){ */
	for (i = 0; i < 4; i++, copy_size += 1024 * 1024) {
		inner_dcache_flush_all();
		if (printlog == 1)
			pr_debug(" %lu :", copy_size);
		avg = 0;
		/* no pld */
		for (j = 0; j < 8; j++) {
			if (flush_cache == 1)
				inner_dcache_flush_all();
#if 1
			temp = 0;

			/* enable ARM CPU PMU */
			asm volatile ("mov %0, #0\n" "MRC p15, 0, %0, c9, c12, 0\n" "BIC %0, %0, #1 << 0\n"	/* disable */
				      "ORR %0, %0, #1 << 2\n"	/* reset cycle count */
				      "BIC %0, %0, #1 << 3\n"	/* count every clock cycle */
				      "MCR p15, 0, %0, c9, c12, 0\n":"+r" (temp)
 :  : "cc");
			asm volatile ("MRC p15, 0, %0, c9, c12, 0\n" "ORR %0, %0, #1 << 0\n"	/* enable */
				      "MCR p15, 0, %0, c9, c12, 0\n"
				      "MRC p15, 0, %0, c9, c12, 1\n"
				      "ORR %0, %0, #1 << 31\n"
				      "MCR p15, 0, %0, c9, c12, 1\n":"+r" (temp)
 :  : "cc");
#endif

			asm volatile ("push {r0,r1,r2,r9}\n"
				      "stmfd        sp!, {r3, r4, r5, r6, r7, r8, r12, lr}\n"
				      "mov r0, %0\n"
				      "mov r1, %1\n"
				      "mov r2, %2\n"
				      "mov r3, %3\n"
				      "1:\n"
				      "ldrd    r4, r5, [r1, #0]\n"
				      "strd    r4, r5, [r0, #0]\n"
				      "ldrd    r4, r5, [r1, #8]\n"
				      "strd    r4, r5, [r0, #8]\n"
				      "ldrd    r4, r5, [r1, #16]\n"
				      "strd    r4, r5, [r0, #16]\n"
				      "ldrd    r4, r5, [r1, #24]\n"
				      "strd    r4, r5, [r0, #24]\n"
				      "ldrd    r4, r5, [r1, #32]\n"
				      "strd    r4, r5, [r0, #32]\n"
				      "ldrd    r4, r5, [r1, #40]\n"
				      "ldrd    r6, r7, [r1, #48]\n" "ldrd    r8, r9, [r1, #56]\n"
				      /* Keep the pld as far from the next load as possible. */
				      /* The amount to prefetch was determined experimentally using */
				      /* large sizes, and verifying the prefetch size does not affect */
				      /* the smaller copies too much. */
				      /* WARNING: If the ldrd and strd instructions get too far away */
				      /* from each other, performance suffers. Three loads */
				      /* in a row is the best tradeoff. */
				      /* "pld     [r1, r3]\n" */
				      /* "pldw    [r0, r3]\n" */
				      /* ".word 0xf790f003\n" */
				      "strd    r4, r5, [r0, #40]\n"
				      "strd    r6, r7, [r0, #48]\n"
				      "strd    r8, r9, [r0, #56]\n"
				      "add     r0, r0, #64\n"
				      "add     r1, r1, #64\n"
				      "subs    r2, r2, #64\n"
				      "bge 1b\n"
				      "ldmfd       sp!, {r3, r4, r5, r6, r7, r8, r12, lr}\n"
				      "pop {r0,r1,r2, r9}\n"
				      /* "pop r2\n" */
 :  : "r" (&buffer_dst), "r"(&buffer_src), "r"(copy_size),
				      "r"(pld_dst)
 : );

			/* #if defined(DEBUG_DRAMC_CALIB) */
#if 1
			/* get CPU cycle count from the ARM CPU PMU */
			asm volatile ("MRC p15, 0, %0, c9, c12, 0\n" "BIC %0, %0, #1 << 0\n"	/* disable */
				      "MCR p15, 0, %0, c9, c12, 0\n"
				      "MRC p15, 0, %0, c9, c13, 0\n":"+r" (temp)
 :  : "cc");
			result[j] = temp;
#endif
		}		/* 10 times loop */
		avg = 0;
		for (j = 0; j < 8; j++) {
			avg += result[j];
		}
		avg = avg >> 3;
		if (printlog == 1)
			pr_debug(" %lu ", avg);
		for (pld_dst = 0; pld_dst < 64 * 16; pld_dst += 64) {
			avg = 0;
			for (j = 0; j < 8; j++) {
				if (flush_cache == 1)
					inner_dcache_flush_all();
#if 1
				temp = 0;

				/* enable ARM CPU PMU */
				asm volatile ("mov %0, #0\n" "MRC p15, 0, %0, c9, c12, 0\n" "BIC %0, %0, #1 << 0\n"	/* disable */
					      "ORR %0, %0, #1 << 2\n"	/* reset cycle count */
					      "BIC %0, %0, #1 << 3\n"	/* count every clock cycle */
					      "MCR p15, 0, %0, c9, c12, 0\n":"+r" (temp)
 :  : "cc");
				asm volatile ("MRC p15, 0, %0, c9, c12, 0\n" "ORR %0, %0, #1 << 0\n"	/* enable */
					      "MCR p15, 0, %0, c9, c12, 0\n"
					      "MRC p15, 0, %0, c9, c12, 1\n"
					      "ORR %0, %0, #1 << 31\n"
					      "MCR p15, 0, %0, c9, c12, 1\n":"+r" (temp)
 :  : "cc");
#endif

				asm volatile ("push {r0,r1,r2,r9}\n"
					      "stmfd        sp!, {r3, r4, r5, r6, r7, r8, r12, lr}\n"
					      "mov r0, %0\n"
					      "mov r1, %1\n"
					      "mov r2, %2\n"
					      "mov r3, %3\n"
					      "1:\n"
					      "ldrd    r4, r5, [r1, #0]\n"
					      "strd    r4, r5, [r0, #0]\n"
					      "ldrd    r4, r5, [r1, #8]\n"
					      "strd    r4, r5, [r0, #8]\n"
					      "ldrd    r4, r5, [r1, #16]\n"
					      "strd    r4, r5, [r0, #16]\n"
					      "ldrd    r4, r5, [r1, #24]\n"
					      "strd    r4, r5, [r0, #24]\n"
					      "ldrd    r4, r5, [r1, #32]\n"
					      "strd    r4, r5, [r0, #32]\n"
					      "ldrd    r4, r5, [r1, #40]\n"
					      "ldrd    r6, r7, [r1, #48]\n"
					      "ldrd    r8, r9, [r1, #56]\n"
					      /* Keep the pld as far from the next load as possible. */
					      /* The amount to prefetch was determined experimentally using */
					      /* large sizes, and verifying the prefetch size does not affect */
					      /* the smaller copies too much. */
					      /* WARNING: If the ldrd and strd instructions get too far away */
					      /* from each other, performance suffers. Three loads */
					      /* in a row is the best tradeoff. */
					      /* "pld     [r1, r3]\n" */
					      /* "pldw    [r0, r3]\n" */
					      /* ".word 0xf790f003\n" */
					      "strd    r4, r5, [r0, #40]\n"
					      "strd    r6, r7, [r0, #48]\n"
					      "strd    r8, r9, [r0, #56]\n"
					      "add     r0, r0, #64\n"
					      "add     r1, r1, #64\n"
					      "subs    r2, r2, #64\n"
					      "bge 1b\n"
					      "ldmfd       sp!, {r3, r4, r5, r6, r7, r8, r12, lr}\n"
					      "pop {r0,r1,r2, r9}\n"
					      /* "pop r2\n" */
 :  : "r" (&buffer_dst), "r"(&buffer_src), "r"(copy_size),
					      "r"(pld_dst)
 : );

				/* #if defined(DEBUG_DRAMC_CALIB) */
#if 1
				/* get CPU cycle count from the ARM CPU PMU */
				asm volatile ("MRC p15, 0, %0, c9, c12, 0\n" "BIC %0, %0, #1 << 0\n"	/* disable */
					      "MCR p15, 0, %0, c9, c12, 0\n"
					      "MRC p15, 0, %0, c9, c13, 0\n":"+r" (temp)
 :  : "cc");
				/* printk("%lu bytes takes %d CPU cycles\n\r", copy_size,temp); */
				/* printk(KERN_EMERG" %d ", temp); */
				/* printk(KERN_CONT" %d ", temp); */
				result[j] = temp;
				/* printk(KERN_CONT" result[%d]=%d :", j, result[j]); */
#endif
			}	/* 10 times loop */
			avg = 0;
			for (j = 0; j < 8; j++) {
				avg += result[j];
			}
			avg = avg >> 3;
			if (printlog == 1)
				pr_debug(" %d ", avg);
		}		/* pld dist loop */
	}
	if (printlog == 1)
		pr_debug("\n\r ====Single load/store test done ==== flush_cache:%d\n",
			 flush_cache);
	local_irq_restore(flags);
	preempt_enable();
}

extern bool printk_disable_uart;
int hander_debug = 0;
static void (*test_callback) (int log);
static atomic_t thread_exec_count;


static set_affinity(int cpu)
{
	int mask_len = DIV_ROUND_UP(NR_CPUS, 32) * 9;
	char *mask_str = kmalloc(mask_len, GFP_KERNEL);
	cpumask_t mask = { CPU_BITS_NONE };
	cpumask_set_cpu(cpu, &mask);
	cpumask_scnprintf(mask_str, mask_len, &mask);

	sched_setaffinity(current->pid, &mask);
	/* printk(KERN_EMERG"set cpu %d affinity, mask [%s]\n",cpu, mask_str); */

	kfree(mask_str);
}

static void temp_thread(int cpu)
{
	pr_err("[%d] >>>> Start at CPU:%d\n", cpu, cpu);
	set_affinity(cpu);
	pr_err("[%d] SET affinity done\n", cpu);
	/* Sync exec */

	atomic_inc(&thread_exec_count);
	pr_err("[%d] wait for exec...\n", cpu);
	while (atomic_read(&thread_exec_count) < 8);
	/* Start teh Evaluation. Running the test */
	if (cpu == 0) {
		pr_err("[%d]do the test\n", cpu);
		test_callback(1);
	} else {
		pr_err("[%d]do the test\n", cpu);
		test_callback(0);
	}
	/* End */
	pr_err("[%d] End\n", cpu);
}

MT_DEBUG_ENTRY(attu);
static int mt_attu_show(struct seq_file *m, void *v)
{
	pr_debug(" debug_locks = %d\n", debug_locks);

	return 0;
}

static ssize_t mt_attu_write(struct file *filp, const char *ubuf, size_t cnt, loff_t *data)
{
	char buf[64];
	unsigned long val;
	int ret;
	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(&buf, ubuf, cnt))
		return -EFAULT;

	buf[cnt] = 0;

	printk_disable_uart = 0;
	ret = strict_strtoul(buf, 10, &val);
	if (ret < 0)
		return ret;
	if (val == 0) {
		/* debug_locks_off(); */
		flush_cache ^= 0x1;
		printk(KERN_EMERG "switch flusch_cache = %d\n", flush_cache);
	} else if (val == 1) {
		atomic_set(&thread_exec_count, 0);
		kernel_thread(temp_thread, 0, CLONE_FS | CLONE_FILES | SIGCHLD);
		kernel_thread(temp_thread, 1, CLONE_FS | CLONE_FILES | SIGCHLD);
		kernel_thread(temp_thread, 2, CLONE_FS | CLONE_FILES | SIGCHLD);
		kernel_thread(temp_thread, 3, CLONE_FS | CLONE_FILES | SIGCHLD);
		kernel_thread(temp_thread, 4, CLONE_FS | CLONE_FILES | SIGCHLD);
		kernel_thread(temp_thread, 5, CLONE_FS | CLONE_FILES | SIGCHLD);
		kernel_thread(temp_thread, 6, CLONE_FS | CLONE_FILES | SIGCHLD);
		kernel_thread(temp_thread, 7, CLONE_FS | CLONE_FILES | SIGCHLD);
	} else if (val == 2) {
		test_instr(1);
		test_callback = test_instr;
	} else if (val == 3) {
		test_instr_only_pld(1);
		test_callback = test_instr_only_pld;
	} else if (val == 4) {
		test_instr_pld_pldw(1);
		test_callback = test_instr_pld_pldw;
	} else if (val == 5) {
		test_instr_NEON(1);
		test_callback = test_instr_NEON;
	} else if (val == 6) {
		enable_RA_mode();
		pr_err("enable RA mode\n");
	} else if (val == 7) {
		disable_RA_mode();
		pr_err("disable RA mode\n");
	} else if (val == 8) {
		hander_debug = 1;
	} else if (val == 9) {
		hander_debug = 0;
	}
	return cnt;
}

static int __init init_auto_tune(void)
{
	struct proc_dir_entry *pe;
	flush_cache = 1;
	test_callback = test_instr;
	pe = proc_create("mtprof/attu", 0664, NULL, &mt_attu_fops);
	if (!pe)
		return -ENOMEM;
	return 0;
}
late_initcall(init_auto_tune);
