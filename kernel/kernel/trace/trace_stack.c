/*
 * Copyright (C) 2008 Steven Rostedt <srostedt@redhat.com>
 *
 */
#include <linux/stacktrace.h>
#include <linux/kallsyms.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/ftrace.h>
#include <linux/module.h>
#include <linux/sysctl.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/earlysuspend.h>

#include <asm/setup.h>

#include "trace.h"

#define STACK_TRACE_ENTRIES 500

#ifdef CC_USING_FENTRY
# define fentry		1
#else
# define fentry		0
#endif

static unsigned long stack_dump_trace[STACK_TRACE_ENTRIES+1] =
	 { [0 ... (STACK_TRACE_ENTRIES)] = ULONG_MAX };
static unsigned stack_dump_index[STACK_TRACE_ENTRIES];

/*
 * Reserve one entry for the passed in ip. This will allow
 * us to remove most or all of the stack size overhead
 * added by the stack tracer itself.
 */
static struct stack_trace max_stack_trace = {
	.max_entries		= STACK_TRACE_ENTRIES - 1,
	.entries		= &stack_dump_trace[1],
};

static unsigned long max_stack_size;
static arch_spinlock_t max_stack_lock =
	(arch_spinlock_t)__ARCH_SPIN_LOCK_UNLOCKED;

static int stack_trace_disabled __read_mostly;
static DEFINE_PER_CPU(int, trace_active);
static DEFINE_MUTEX(stack_sysctl_mutex);

int stack_tracer_enabled;
static int last_stack_tracer_enabled;

/*LCH add for stack overflow debug */
#if defined (CONFIG_MTK_AEE_FEATURE) && defined (CONFIG_MT_ENG_BUILD)
#include <linux/aee.h>
#include <linux/thread_info.h>
/*768=sizeof(struct thread_info),1600 for buffer*/ 
static unsigned long stack_overflow_thd = THREAD_SIZE-768-1600;
module_param_named(stack_overflow_thd, stack_overflow_thd, ulong, S_IRUGO | S_IWUSR);

static void dump_max_stack_trace() {
    int i = 0;
    int size;
    printk(KERN_INFO "        Depth    Size   Location"
			   "    (%d entries)\n"
			   "        -----    ----   --------\n",
			   max_stack_trace.nr_entries - 1);
	
	for (i=0; i<max_stack_trace.nr_entries; i++) {
	    if (stack_dump_trace[i] == ULONG_MAX)
		    continue;

	    if (i+1 == max_stack_trace.nr_entries ||
	        stack_dump_trace[i+1] == ULONG_MAX)
		    size = stack_dump_index[i];
	    else
		    size = stack_dump_index[i] - stack_dump_index[i+1];

	    printk(KERN_INFO "%3ld) %8d   %5d   %pS\n", i, stack_dump_index[i], size, stack_dump_trace[i]);
    }
}
#endif

static inline void
check_stack(unsigned long ip, unsigned long *stack)
{
	unsigned long this_size, flags;
	unsigned long *p, *top, *start;
	static int tracer_frame;
	int frame_size = ACCESS_ONCE(tracer_frame);
	int i;

	this_size = ((unsigned long)stack) & (THREAD_SIZE-1);
	this_size = THREAD_SIZE - this_size;
	/* Remove the frame of the tracer */
	this_size -= frame_size;

    /*LCH add for stack overflow debug, stack_tracer_enabled:
     * 0:disable, 
     * 1:record stack_max_size and stack_trace
     * 2:only for overflow trigger kernel warning */
#if defined (CONFIG_MTK_AEE_FEATURE) && defined (CONFIG_MT_ENG_BUILD)
    if (stack_tracer_enabled == 2) {
	    if (this_size < stack_overflow_thd) {
	        return;
	    }
	    stack_trace_disabled = 1;
	    max_stack_size = 0;
    }
#endif

	if (this_size <= max_stack_size)
		return;

	/* we do not handle interrupt stacks yet */
	if (!object_is_on_stack(stack))
		return;

	local_irq_save(flags);
	arch_spin_lock(&max_stack_lock);

	/* In case another CPU set the tracer_frame on us */
	if (unlikely(!frame_size))
		this_size -= tracer_frame;

	/* a race could have already updated it */
	if (this_size <= max_stack_size)
		goto out;

	max_stack_size = this_size;

	max_stack_trace.nr_entries	= 0;
	max_stack_trace.skip		= 3;

	save_stack_trace(&max_stack_trace);

	/*
	 * Add the passed in ip from the function tracer.
	 * Searching for this on the stack will skip over
	 * most of the overhead from the stack tracer itself.
	 */
	stack_dump_trace[0] = ip;
	max_stack_trace.nr_entries++;

	/*
	 * Now find where in the stack these are.
	 */
	i = 0;
	start = stack;
	top = (unsigned long *)
		(((unsigned long)start & ~(THREAD_SIZE-1)) + THREAD_SIZE);

	/*
	 * Loop through all the entries. One of the entries may
	 * for some reason be missed on the stack, so we may
	 * have to account for them. If they are all there, this
	 * loop will only happen once. This code only takes place
	 * on a new max, so it is far from a fast path.
	 */
	while (i < max_stack_trace.nr_entries) {
		int found = 0;

		stack_dump_index[i] = this_size;
		p = start;

		for (; p < top && i < max_stack_trace.nr_entries; p++) {
			if (*p == stack_dump_trace[i]) {
				this_size = stack_dump_index[i++] =
					(top - p) * sizeof(unsigned long);
				found = 1;
				/* Start the search from here */
				start = p + 1;
				/*
				 * We do not want to show the overhead
				 * of the stack tracer stack in the
				 * max stack. If we haven't figured
				 * out what that is, then figure it out
				 * now.
				 */
				if (unlikely(!tracer_frame) && i == 1) {
					tracer_frame = (p - stack) *
						sizeof(unsigned long);
					max_stack_size -= tracer_frame;
				}
			}
		}

		if (!found)
			i++;
	}

 out:
	arch_spin_unlock(&max_stack_lock);
	local_irq_restore(flags);
#if defined (CONFIG_MTK_AEE_FEATURE) && defined (CONFIG_MT_ENG_BUILD)
    if (stack_tracer_enabled == 2) {
        dump_max_stack_trace();
	    aee_kernel_warning("[STACK_OVERFLOW_DEBUG]", "stack_size:%d", max_stack_size);
	}
#endif
}

static void
stack_trace_call(unsigned long ip, unsigned long parent_ip)
{
	unsigned long stack;
	int cpu;

	if (unlikely(!ftrace_enabled || stack_trace_disabled))
		return;

	preempt_disable_notrace();

	cpu = raw_smp_processor_id();
	/* no atomic needed, we only modify this variable by this cpu */
	if (per_cpu(trace_active, cpu)++ != 0)
		goto out;

	/*
	 * When fentry is used, the traced function does not get
	 * its stack frame set up, and we lose the parent.
	 * The ip is pretty useless because the function tracer
	 * was called before that function set up its stack frame.
	 * In this case, we use the parent ip.
	 *
	 * By adding the return address of either the parent ip
	 * or the current ip we can disregard most of the stack usage
	 * caused by the stack tracer itself.
	 *
	 * The function tracer always reports the address of where the
	 * mcount call was, but the stack will hold the return address.
	 */
	if (fentry)
		ip = parent_ip;
	else
		ip += MCOUNT_INSN_SIZE;

	check_stack(ip, &stack);

 out:
	per_cpu(trace_active, cpu)--;
	/* prevent recursion in schedule */
	preempt_enable_notrace();
}

static struct ftrace_ops trace_ops __read_mostly =
{
	.func = stack_trace_call,
};

static ssize_t
stack_max_size_read(struct file *filp, char __user *ubuf,
		    size_t count, loff_t *ppos)
{
	unsigned long *ptr = filp->private_data;
	char buf[64];
	int r;

	r = snprintf(buf, sizeof(buf), "%ld\n", *ptr);
	if (r > sizeof(buf))
		r = sizeof(buf);
	return simple_read_from_buffer(ubuf, count, ppos, buf, r);
}

static ssize_t
stack_max_size_write(struct file *filp, const char __user *ubuf,
		     size_t count, loff_t *ppos)
{
	long *ptr = filp->private_data;
	unsigned long val, flags;
	int ret;
	int cpu;

	ret = kstrtoul_from_user(ubuf, count, 10, &val);
	if (ret)
		return ret;

	local_irq_save(flags);

	/*
	 * In case we trace inside arch_spin_lock() or after (NMI),
	 * we will cause circular lock, so we also need to increase
	 * the percpu trace_active here.
	 */
	cpu = smp_processor_id();
	per_cpu(trace_active, cpu)++;

	arch_spin_lock(&max_stack_lock);
	*ptr = val;
	arch_spin_unlock(&max_stack_lock);

	per_cpu(trace_active, cpu)--;
	local_irq_restore(flags);

	return count;
}

static const struct file_operations stack_max_size_fops = {
	.open		= tracing_open_generic,
	.read		= stack_max_size_read,
	.write		= stack_max_size_write,
	.llseek		= default_llseek,
};

static void *
__next(struct seq_file *m, loff_t *pos)
{
	long n = *pos - 1;

	if (n >= max_stack_trace.nr_entries || stack_dump_trace[n] == ULONG_MAX)
		return NULL;

	m->private = (void *)n;
	return &m->private;
}

static void *
t_next(struct seq_file *m, void *v, loff_t *pos)
{
	(*pos)++;
	return __next(m, pos);
}

static void *t_start(struct seq_file *m, loff_t *pos)
{
	int cpu;

	local_irq_disable();

	cpu = smp_processor_id();
	per_cpu(trace_active, cpu)++;

	arch_spin_lock(&max_stack_lock);

	if (*pos == 0)
		return SEQ_START_TOKEN;

	return __next(m, pos);
}

static void t_stop(struct seq_file *m, void *p)
{
	int cpu;

	arch_spin_unlock(&max_stack_lock);

	cpu = smp_processor_id();
	per_cpu(trace_active, cpu)--;

	local_irq_enable();
}

static int trace_lookup_stack(struct seq_file *m, long i)
{
	unsigned long addr = stack_dump_trace[i];

	return seq_printf(m, "%pS\n", (void *)addr);
}

static void print_disabled(struct seq_file *m)
{
	seq_puts(m, "#\n"
		 "#  Stack tracer disabled\n"
		 "#\n"
		 "# To enable the stack tracer, either add 'stacktrace' to the\n"
		 "# kernel command line\n"
		 "# or 'echo 1 > /proc/sys/kernel/stack_tracer_enabled'\n"
		 "#\n");
}

static int t_show(struct seq_file *m, void *v)
{
	long i;
	int size;

	if (v == SEQ_START_TOKEN) {
		seq_printf(m, "        Depth    Size   Location"
			   "    (%d entries)\n"
			   "        -----    ----   --------\n",
			   max_stack_trace.nr_entries - 1);

		if (!stack_tracer_enabled && !max_stack_size)
			print_disabled(m);

		return 0;
	}

	i = *(long *)v;

	if (i >= max_stack_trace.nr_entries ||
	    stack_dump_trace[i] == ULONG_MAX)
		return 0;

	if (i+1 == max_stack_trace.nr_entries ||
	    stack_dump_trace[i+1] == ULONG_MAX)
		size = stack_dump_index[i];
	else
		size = stack_dump_index[i] - stack_dump_index[i+1];

	seq_printf(m, "%3ld) %8d   %5d   ", i, stack_dump_index[i], size);

	trace_lookup_stack(m, i);

	return 0;
}

static const struct seq_operations stack_trace_seq_ops = {
	.start		= t_start,
	.next		= t_next,
	.stop		= t_stop,
	.show		= t_show,
};

static int stack_trace_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &stack_trace_seq_ops);
}

static const struct file_operations stack_trace_fops = {
	.open		= stack_trace_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

static int
stack_trace_filter_open(struct inode *inode, struct file *file)
{
	return ftrace_regex_open(&trace_ops, FTRACE_ITER_FILTER,
				 inode, file);
}

static const struct file_operations stack_trace_filter_fops = {
	.open = stack_trace_filter_open,
	.read = seq_read,
	.write = ftrace_filter_write,
	.llseek = ftrace_filter_lseek,
	.release = ftrace_regex_release,
};

#ifdef CONFIG_HAS_EARLYSUSPEND
/*add for stack_trace suspend-resume issue*/
static int stack_tracer_enabled_backup;
static int stack_trace_early_suspend_register = 0;
static void stack_trace_early_suspend(struct early_suspend *h)
{
	stack_tracer_enabled_backup = stack_tracer_enabled;
	printk("[STACK_TRACER]stack_trace_early_suspend:%d\n", stack_tracer_enabled_backup);
	if(stack_tracer_enabled) {
	    mutex_lock(&stack_sysctl_mutex);
	    
	    stack_trace_disabled = 1;
	    stack_tracer_enabled = 0;
	    last_stack_tracer_enabled = !!stack_tracer_enabled;
		unregister_ftrace_function(&trace_ops);
		
	    mutex_unlock(&stack_sysctl_mutex);
	}
}

static void stack_trace_late_resume(struct early_suspend *h)
{
	printk("[STACK_TRACER]stack_trace_late_resume:%d\n", stack_tracer_enabled_backup);
	if(stack_tracer_enabled_backup) {
	    mutex_lock(&stack_sysctl_mutex);
	    
	    stack_tracer_enabled = stack_tracer_enabled_backup;
	    last_stack_tracer_enabled = !!stack_tracer_enabled;
		register_ftrace_function(&trace_ops);
		stack_trace_disabled = 0;
		
	    mutex_unlock(&stack_sysctl_mutex);	
	}
}

static struct early_suspend stack_tracer_early_suspend_handler =
{
    .level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN,
    .suspend = stack_trace_early_suspend,
    .resume  = stack_trace_late_resume,
};
#endif

int
stack_trace_sysctl(struct ctl_table *table, int write,
		   void __user *buffer, size_t *lenp,
		   loff_t *ppos)
{
	int ret;

	mutex_lock(&stack_sysctl_mutex);

	ret = proc_dointvec(table, write, buffer, lenp, ppos);

	if (ret || !write ||
	    (last_stack_tracer_enabled == !!stack_tracer_enabled))
		goto out;

	last_stack_tracer_enabled = !!stack_tracer_enabled;

	if (stack_tracer_enabled) {
    #ifdef CONFIG_HAS_EARLYSUSPEND
        if (!stack_trace_early_suspend_register) {
            register_early_suspend(&stack_tracer_early_suspend_handler);
            stack_trace_early_suspend_register = 1;
        }
    #endif
		register_ftrace_function(&trace_ops);
    }
	else {
		unregister_ftrace_function(&trace_ops);
    #ifdef CONFIG_HAS_EARLYSUSPEND
        if (stack_trace_early_suspend_register) {
            unregister_early_suspend(&stack_tracer_early_suspend_handler);
            stack_trace_early_suspend_register = 0;
        }
    #endif
    }
 out:
	mutex_unlock(&stack_sysctl_mutex);
	return ret;
}

static char stack_trace_filter_buf[COMMAND_LINE_SIZE+1] __initdata;

static __init int enable_stacktrace(char *str)
{
	if (strncmp(str, "_filter=", 8) == 0)
		strncpy(stack_trace_filter_buf, str+8, COMMAND_LINE_SIZE);
		
/*LCH add for stack overflow debug */
#if defined (CONFIG_MTK_AEE_FEATURE) && defined (CONFIG_MT_ENG_BUILD)
	stack_tracer_enabled = 2;
	last_stack_tracer_enabled = 2;
#else
	stack_tracer_enabled = 1;
	last_stack_tracer_enabled = 1;
#endif
	return 1;
}
__setup("stacktrace", enable_stacktrace);

static __init int stack_trace_init(void)
{
	struct dentry *d_tracer;

	d_tracer = tracing_init_dentry();
	if (!d_tracer)
		return 0;

	trace_create_file("stack_max_size", 0644, d_tracer,
			&max_stack_size, &stack_max_size_fops);

	trace_create_file("stack_trace", 0444, d_tracer,
			NULL, &stack_trace_fops);

	trace_create_file("stack_trace_filter", 0444, d_tracer,
			NULL, &stack_trace_filter_fops);

	if (stack_trace_filter_buf[0])
		ftrace_set_early_filter(&trace_ops, stack_trace_filter_buf, 1);

	if (stack_tracer_enabled) {
		register_ftrace_function(&trace_ops);
		
    #ifdef CONFIG_HAS_EARLYSUSPEND
        if (!stack_trace_early_suspend_register) {
            register_early_suspend(&stack_tracer_early_suspend_handler);
            stack_trace_early_suspend_register = 1;
        }
    #endif
    }

	return 0;
}

device_initcall(stack_trace_init);
