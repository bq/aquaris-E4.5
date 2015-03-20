#undef TRACE_SYSTEM
#define TRACE_SYSTEM sched

#if !defined(_TRACE_SCHED_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_SCHED_H

#include <linux/sched.h>
#include <linux/tracepoint.h>
#include <linux/binfmts.h>

#ifdef CONFIG_MT65XX_TRACER
#include <trace/events/mt65xx_mon_trace.h>
#endif

#ifdef CONFIG_MTK_SCHED_TRACERS
/* mtk04259: states for tracking I/O & mutex events 
 * notice avoid to conflict with linux/sched.h
 *
 * A bug linux not fixed: 
 * 'K' for TASK_WAKEKILL specified in linux/sched.h
 * but marked 'K' in sched_switch will cause Android systrace parser confused
 * therefore for sched_switch events, these extra states will be printed
 * in the end of each line
 */
#define _MT_TASK_BLOCKED_RTMUX 1024
#define _MT_TASK_BLOCKED_MUTEX 2048
#define _MT_TASK_BLOCKED_IO 4096
#define _MT_TASK_BLOCKED_STATE_MASK ( _MT_TASK_BLOCKED_RTMUX | _MT_TASK_BLOCKED_MUTEX | _MT_TASK_BLOCKED_IO )
#endif
#define _MT_TASK_STATE_MASK		( (TASK_STATE_MAX-1) & ~TASK_WAKEKILL )

/*
 * Tracepoint for calling kthread_stop, performed to end a kthread:
 */
TRACE_EVENT(sched_kthread_stop,

	TP_PROTO(struct task_struct *t),

	TP_ARGS(t),

	TP_STRUCT__entry(
		__array(	char,	comm,	TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, t->comm, TASK_COMM_LEN);
		__entry->pid	= t->pid;
	),

	TP_printk("comm=%s pid=%d", __entry->comm, __entry->pid)
);

/*
 * Tracepoint for the return value of the kthread stopping:
 */
TRACE_EVENT(sched_kthread_stop_ret,

	TP_PROTO(int ret),

	TP_ARGS(ret),

	TP_STRUCT__entry(
		__field(	int,	ret	)
	),

	TP_fast_assign(
		__entry->ret	= ret;
	),

	TP_printk("ret=%d", __entry->ret)
);

#ifdef CREATE_TRACE_POINTS
static inline long __trace_sched_switch_state(struct task_struct *p)
{
	long state = p->state;

#ifdef CONFIG_PREEMPT
	/*
	 * For all intents and purposes a preempted task is a running task.
	 */
	if (task_thread_info(p)->preempt_count & PREEMPT_ACTIVE)
		state = TASK_RUNNING | TASK_STATE_MAX;
#endif

#ifdef CONFIG_MTK_SCHED_TRACERS
#ifdef CONFIG_RT_MUTEXES
    if(p->pi_blocked_on)
        state |= _MT_TASK_BLOCKED_RTMUX;
#endif
#ifdef CONFIG_DEBUG_MUTEXES
    if(p->blocked_on) 
        state |= _MT_TASK_BLOCKED_MUTEX;
#endif
    if((p->state & TASK_UNINTERRUPTIBLE) && p->in_iowait)
        state |= _MT_TASK_BLOCKED_IO;
#endif

	return state;
}
#endif

/*
 * Tracepoint for waking up a task:
 */
DECLARE_EVENT_CLASS(sched_wakeup_template,

	TP_PROTO(struct task_struct *p, int success),

	TP_ARGS(p, success),

	TP_STRUCT__entry(
		__array(	char,	comm,	TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
		__field(	int,	prio			)
		__field(	int,	success			)
		__field(	int,	target_cpu		)
#ifdef CONFIG_MTK_SCHED_TRACERS
        __field(    long,    state           )
#endif
	),

	TP_fast_assign(
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid		= p->pid;
		__entry->prio		= p->prio;
		__entry->success	= success;
		__entry->target_cpu	= task_cpu(p);
#ifdef CONFIG_MTK_SCHED_TRACERS
        __entry->state      =__trace_sched_switch_state(p);
#endif
	),

	TP_printk(
#ifdef CONFIG_MTK_SCHED_TRACERS
            "comm=%s pid=%d prio=%d success=%d target_cpu=%03d state=%s",
#else
            "comm=%s pid=%d prio=%d success=%d target_cpu=%03d",
#endif
		  __entry->comm, __entry->pid, __entry->prio,
		  __entry->success, __entry->target_cpu
#ifdef CONFIG_MTK_SCHED_TRACERS
        ,
		__entry->state & ~TASK_STATE_MAX ?
		  __print_flags(__entry->state & ~TASK_STATE_MAX, "|",
				{ 1, "S"} , { 2, "D" }, { 4, "T" }, { 8, "t" },
				{ 16, "Z" }, { 32, "X" }, { 64, "x" },
                { 128, "K" }, { 256, "W"}, {1024, "r"}, {2048, "m"}, {4096, "d"}) : "R"
#endif
          )
);

DEFINE_EVENT(sched_wakeup_template, sched_wakeup,
	     TP_PROTO(struct task_struct *p, int success),
	     TP_ARGS(p, success));

/*
 * Tracepoint for waking up a new task:
 */
DEFINE_EVENT(sched_wakeup_template, sched_wakeup_new,
	     TP_PROTO(struct task_struct *p, int success),
	     TP_ARGS(p, success));

/*
 * Tracepoint for task switches, performed by the scheduler:
 */
TRACE_EVENT(sched_switch,

	TP_PROTO(struct task_struct *prev,
		 struct task_struct *next),

	TP_ARGS(prev, next),

	TP_STRUCT__entry(
		__array(	char,	prev_comm,	TASK_COMM_LEN	)
		__field(	pid_t,	prev_pid			)
		__field(	int,	prev_prio			)
		__field(	long,	prev_state			)
		__array(	char,	next_comm,	TASK_COMM_LEN	)
		__field(	pid_t,	next_pid			)
		__field(	int,	next_prio			)
	),

	TP_fast_assign(
		memcpy(__entry->next_comm, next->comm, TASK_COMM_LEN);
		__entry->prev_pid	= prev->pid;
		__entry->prev_prio	= prev->prio;
		__entry->prev_state	= __trace_sched_switch_state(prev);
		memcpy(__entry->prev_comm, prev->comm, TASK_COMM_LEN);
		__entry->next_pid	= next->pid;
		__entry->next_prio	= next->prio;
	),

	TP_printk(
#ifdef CONFIG_MTK_SCHED_TRACERS
            "prev_comm=%s prev_pid=%d prev_prio=%d prev_state=%s%s ==> next_comm=%s next_pid=%d next_prio=%d%s%s",
#else
            "prev_comm=%s prev_pid=%d prev_prio=%d prev_state=%s%s ==> next_comm=%s next_pid=%d next_prio=%d",
#endif
		__entry->prev_comm, __entry->prev_pid, __entry->prev_prio,
		__entry->prev_state & (_MT_TASK_STATE_MASK) ?
	  __print_flags(__entry->prev_state & (_MT_TASK_STATE_MASK), "|",
				{ 1, "S"} , { 2, "D" }, { 4, "T" }, { 8, "t" },
				{ 16, "Z" }, { 32, "X" }, { 64, "x" },
                {128, "K"}, { 256, "W"}) : "R",
		__entry->prev_state & TASK_STATE_MAX ? "+" : "",
		__entry->next_comm, __entry->next_pid, __entry->next_prio
#ifdef CONFIG_MTK_SCHED_TRACERS
        ,
        (__entry->prev_state & (TASK_WAKEKILL | _MT_TASK_BLOCKED_STATE_MASK))?" extra_prev_state=":"",
        __print_flags(__entry->prev_state & (TASK_WAKEKILL | _MT_TASK_BLOCKED_STATE_MASK), "|",
            { TASK_WAKEKILL, "K" },
            { _MT_TASK_BLOCKED_RTMUX, "r" },
            { _MT_TASK_BLOCKED_MUTEX, "m" },
            { _MT_TASK_BLOCKED_IO, "d" })
#endif
        )
);

/*
 * Tracepoint for a task being migrated:
 */
TRACE_EVENT(sched_migrate_task,

	TP_PROTO(struct task_struct *p, int dest_cpu),

	TP_ARGS(p, dest_cpu),

	TP_STRUCT__entry(
		__array(	char,	comm,	TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
		__field(	int,	prio			)
		__field(	int,	orig_cpu		)
		__field(	int,	dest_cpu		)
#ifdef CONFIG_MTK_SCHED_TRACERS
        __field(    long,    state           )
#endif
	),

	TP_fast_assign(
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid		= p->pid;
		__entry->prio		= p->prio;
		__entry->orig_cpu	= task_cpu(p);
		__entry->dest_cpu	= dest_cpu;
#ifdef CONFIG_MTK_SCHED_TRACERS
        __entry->state      =__trace_sched_switch_state(p);
#endif
	),

#ifdef CONFIG_MTK_SCHED_TRACERS
	TP_printk("comm=%s pid=%d prio=%d orig_cpu=%d dest_cpu=%d state=%s",
#else
	TP_printk("comm=%s pid=%d prio=%d orig_cpu=%d dest_cpu=%d",
#endif
		  __entry->comm, __entry->pid, __entry->prio,
		  __entry->orig_cpu, __entry->dest_cpu
#ifdef CONFIG_MTK_SCHED_TRACERS
        ,
		__entry->state & ~TASK_STATE_MAX ?
		  __print_flags(__entry->state & ~TASK_STATE_MAX, "|",
				{ 1, "S"} , { 2, "D" }, { 4, "T" }, { 8, "t" },
				{ 16, "Z" }, { 32, "X" }, { 64, "x" },
                { 128, "K" }, { 256, "W"}, {1024, "r"}, {2048, "m"}, {4096, "d"}) : "R"
#endif
          )
);

DECLARE_EVENT_CLASS(sched_process_template,

	TP_PROTO(struct task_struct *p),

	TP_ARGS(p),

	TP_STRUCT__entry(
		__array(	char,	comm,	TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
		__field(	int,	prio			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid		= p->pid;
		__entry->prio		= p->prio;
	),

	TP_printk("comm=%s pid=%d prio=%d",
		  __entry->comm, __entry->pid, __entry->prio)
);

/*
 * Tracepoint for freeing a task:
 */
DEFINE_EVENT(sched_process_template, sched_process_free,
	     TP_PROTO(struct task_struct *p),
	     TP_ARGS(p));
	     

/*
 * Tracepoint for a task exiting:
 */
DEFINE_EVENT(sched_process_template, sched_process_exit,
	     TP_PROTO(struct task_struct *p),
	     TP_ARGS(p));

/*
 * Tracepoint for waiting on task to unschedule:
 */
DEFINE_EVENT(sched_process_template, sched_wait_task,
	TP_PROTO(struct task_struct *p),
	TP_ARGS(p));

/*
 * Tracepoint for a waiting task:
 */
TRACE_EVENT(sched_process_wait,

	TP_PROTO(struct pid *pid),

	TP_ARGS(pid),

	TP_STRUCT__entry(
		__array(	char,	comm,	TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
		__field(	int,	prio			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, current->comm, TASK_COMM_LEN);
		__entry->pid		= pid_nr(pid);
		__entry->prio		= current->prio;
	),

	TP_printk("comm=%s pid=%d prio=%d",
		  __entry->comm, __entry->pid, __entry->prio)
);

/*
 * Tracepoint for do_fork:
 */
TRACE_EVENT(sched_process_fork,

	TP_PROTO(struct task_struct *parent, struct task_struct *child),

	TP_ARGS(parent, child),

	TP_STRUCT__entry(
		__array(	char,	parent_comm,	TASK_COMM_LEN	)
		__field(	pid_t,	parent_pid			)
		__array(	char,	child_comm,	TASK_COMM_LEN	)
		__field(	pid_t,	child_pid			)
	),

	TP_fast_assign(
		memcpy(__entry->parent_comm, parent->comm, TASK_COMM_LEN);
		__entry->parent_pid	= parent->pid;
		memcpy(__entry->child_comm, child->comm, TASK_COMM_LEN);
		__entry->child_pid	= child->pid;
	),

	TP_printk("comm=%s pid=%d child_comm=%s child_pid=%d",
		__entry->parent_comm, __entry->parent_pid,
		__entry->child_comm, __entry->child_pid)
);

/*
 * Tracepoint for exec:
 */
TRACE_EVENT(sched_process_exec,

	TP_PROTO(struct task_struct *p, pid_t old_pid,
		 struct linux_binprm *bprm),

	TP_ARGS(p, old_pid, bprm),

	TP_STRUCT__entry(
		__string(	filename,	bprm->filename	)
		__field(	pid_t,		pid		)
		__field(	pid_t,		old_pid		)
	),

	TP_fast_assign(
		__assign_str(filename, bprm->filename);
		__entry->pid		= p->pid;
		__entry->old_pid	= old_pid;
	),

	TP_printk("filename=%s pid=%d old_pid=%d", __get_str(filename),
		  __entry->pid, __entry->old_pid)
);

/*
 * XXX the below sched_stat tracepoints only apply to SCHED_OTHER/BATCH/IDLE
 *     adding sched_stat support to SCHED_FIFO/RR would be welcome.
 */
DECLARE_EVENT_CLASS(sched_stat_template,

	TP_PROTO(struct task_struct *tsk, u64 delay),

	TP_ARGS(tsk, delay),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN	)
		__field( pid_t,	pid			)
		__field( u64,	delay			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid	= tsk->pid;
		__entry->delay	= delay;
	)
	TP_perf_assign(
		__perf_count(delay);
	),

	TP_printk("comm=%s pid=%d delay=%Lu [ns]",
			__entry->comm, __entry->pid,
			(unsigned long long)__entry->delay)
);


/*
 * Tracepoint for accounting wait time (time the task is runnable
 * but not actually running due to scheduler contention).
 */
DEFINE_EVENT(sched_stat_template, sched_stat_wait,
	     TP_PROTO(struct task_struct *tsk, u64 delay),
	     TP_ARGS(tsk, delay));

/*
 * Tracepoint for accounting sleep time (time the task is not runnable,
 * including iowait, see below).
 */
DEFINE_EVENT(sched_stat_template, sched_stat_sleep,
	     TP_PROTO(struct task_struct *tsk, u64 delay),
	     TP_ARGS(tsk, delay));

/*
 * Tracepoint for accounting iowait time (time the task is not runnable
 * due to waiting on IO to complete).
 */
DEFINE_EVENT(sched_stat_template, sched_stat_iowait,
	     TP_PROTO(struct task_struct *tsk, u64 delay),
	     TP_ARGS(tsk, delay));

/*
 * Tracepoint for accounting blocked time (time the task is in uninterruptible).
 */
DEFINE_EVENT(sched_stat_template, sched_stat_blocked,
	     TP_PROTO(struct task_struct *tsk, u64 delay),
	     TP_ARGS(tsk, delay));

/*
 * Tracepoint for accounting runtime (time the task is executing
 * on a CPU).
 */
TRACE_EVENT(sched_stat_runtime,

	TP_PROTO(struct task_struct *tsk, u64 runtime, u64 vruntime),

	TP_ARGS(tsk, runtime, vruntime),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN	)
		__field( pid_t,	pid			)
		__field( u64,	runtime			)
		__field( u64,	vruntime			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid		= tsk->pid;
		__entry->runtime	= runtime;
		__entry->vruntime	= vruntime;
	)
	TP_perf_assign(
		__perf_count(runtime);
	),

	TP_printk("comm=%s pid=%d runtime=%Lu [ns] vruntime=%Lu [ns]",
			__entry->comm, __entry->pid,
			(unsigned long long)__entry->runtime,
			(unsigned long long)__entry->vruntime)
);

/*
 * Tracepoint for showing priority inheritance modifying a tasks
 * priority.
 */
TRACE_EVENT(sched_pi_setprio,

	TP_PROTO(struct task_struct *tsk, int newprio),

	TP_ARGS(tsk, newprio),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN	)
		__field( pid_t,	pid			)
		__field( int,	oldprio			)
		__field( int,	newprio			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid		= tsk->pid;
		__entry->oldprio	= tsk->prio;
		__entry->newprio	= newprio;
	),

	TP_printk("comm=%s pid=%d oldprio=%d newprio=%d",
			__entry->comm, __entry->pid,
			__entry->oldprio, __entry->newprio)
);
#ifdef CONFIG_MT_RT_SCHED_CRIT 
TRACE_EVENT(sched_rt_crit,

	TP_PROTO(int cpu,
		 int rt_throttled),

	TP_ARGS(cpu, rt_throttled),

	TP_STRUCT__entry(
		__field(	int,	cpu				)
		__field(	int,	rt_throttled			)
	),

	TP_fast_assign(
		__entry->cpu		= cpu;
		__entry->rt_throttled 	= rt_throttled; 
	),

	TP_printk(
            "cpu=%d rt_throttled=%d",
		__entry->cpu, __entry->rt_throttled)
        
);
#endif

#ifdef CONFIG_MT_SCHED_NOTICE
TRACE_EVENT(sched_log,

    TP_PROTO(char *strings),

    TP_ARGS(strings),

    TP_STRUCT__entry(
        __array(    char,  strings, 128)
    ),

    TP_fast_assign(
        memcpy(__entry->strings, strings, 128);
    ),

    TP_printk("%s",__entry->strings)
);
#endif

#ifdef CONFIG_MTK_SCHED_TRACERS
TRACE_EVENT(int_switch,

    TP_PROTO(struct task_struct *curr,
	 int irq, int isr_enter),

    TP_ARGS(curr, irq, isr_enter),

    TP_STRUCT__entry(
	__array(    char,   prev_comm,	TASK_COMM_LEN	)
	__field(    pid_t,  prev_pid		)
	__field(    int,    prev_prio		)
	__field(    long,   prev_state		)
	__array(    char,   next_comm,	TASK_COMM_LEN	)
	__field(    pid_t,  next_pid		)
	__field(    int,    next_prio		)
    ),

    TP_fast_assign(
	memcpy(__entry->next_comm, curr->comm, TASK_COMM_LEN);
	__entry->prev_pid   = curr->pid;
	__entry->prev_prio  = curr->prio;
	__entry->prev_state = curr->state;
	memcpy(__entry->prev_comm, curr->comm, TASK_COMM_LEN);
	__entry->next_pid   = curr->pid;
	__entry->next_prio  = curr->prio;
    ),

    TP_printk("Int Switch info %d",__entry->prev_pid)
);
TRACE_EVENT(int_nest,

    TP_PROTO( int prev_irq, int next_irq),

    TP_ARGS(prev_irq, next_irq),

    TP_STRUCT__entry(
	__array(    char,   prev_comm,	TASK_COMM_LEN	)
	__field(    pid_t,  prev_pid		)
	__field(    int,    prev_prio		)
	__field(    long,   prev_state		)
	__array(    char,   next_comm,	TASK_COMM_LEN	)
	__field(    pid_t,  next_pid		)
	__field(    int,    next_prio		)
    ),

    TP_fast_assign(
	__entry->prev_pid   = prev_irq;
	__entry->prev_prio  = 120;
	__entry->prev_state = 0;
	__entry->next_pid   =next_irq;
	__entry->next_prio  = 120;
    ),

    TP_printk("Int nest info %d",__entry->prev_pid)
);
#endif
#ifdef CONFIG_MT_LOAD_BALANCE_PROFILER

TRACE_EVENT(sched_lbprof_status,

    TP_PROTO(char *strings),

    TP_ARGS(strings),

    TP_STRUCT__entry(
	__array(    char,  strings, 128)
    ),

    TP_fast_assign(
	memcpy(__entry->strings, strings, 128);
    ),

    TP_printk("%s",__entry->strings)
);

TRACE_EVENT(sched_lbprof_update,

    TP_PROTO(char *strings),

    TP_ARGS(strings),

    TP_STRUCT__entry(
	__array(    char,  strings, 128)
    ),

    TP_fast_assign(
	memcpy(__entry->strings, strings, 128);
    ),

    TP_printk("%s",__entry->strings)
);

TRACE_EVENT(sched_lbprof_log,

    TP_PROTO(char *strings),

    TP_ARGS(strings),

    TP_STRUCT__entry(
	__array(    char,  strings, 128)
    ),

    TP_fast_assign(
	memcpy(__entry->strings, strings, 128);
    ),

    TP_printk("%s",__entry->strings)
);

#endif

#ifdef CONFIG_MTK_SCHED_CMP
TRACE_EVENT(sched_task_entity_avg,

	TP_PROTO(struct task_struct *tsk, struct sched_avg *avg),

	TP_ARGS(tsk, avg),

	TP_STRUCT__entry(
		__array(	char, 		comm,	TASK_COMM_LEN	)
		__field(	pid_t, 		tgid			)
		__field(	pid_t, 		pid			)
		__field(	unsigned long, 	contrib			)
		__field(	unsigned long, 	ratio			)
		__field(	u32,		usage_sum		)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->tgid      = task_pid_nr(tsk->group_leader);
		__entry->pid       = task_pid_nr(tsk);
		__entry->contrib   = avg->load_avg_contrib;
		__entry->ratio     = avg->load_avg_ratio;
		__entry->usage_sum = avg->usage_avg_sum;
	),

	TP_printk("comm=%s tgid=%d pid=%d contrib=%lu ratio=%lu usage_sum=%d",
		  __entry->comm, __entry->tgid, __entry->pid,
		  __entry->contrib, __entry->ratio, __entry->usage_sum)
);
#endif
#endif /* _TRACE_SCHED_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
