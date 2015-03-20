/* binder.c
 *
 * Android IPC Subsystem
 *
 * Copyright (C) 2007-2008 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <asm/cacheflush.h>
#include <linux/fdtable.h>
#include <linux/file.h>
#include <linux/freezer.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/nsproxy.h>
#include <linux/poll.h>
#include <linux/debugfs.h>
#include <linux/rbtree.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/security.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/rtc.h>
#include <linux/aee.h>

#ifdef CONFIG_MT_PRIO_TRACER
 #include <linux/prio_tracer.h>
#endif

#include "binder.h"
#include "binder_trace.h"

static DEFINE_MUTEX(binder_main_lock);
static DEFINE_MUTEX(binder_deferred_lock);
static DEFINE_MUTEX(binder_mmap_lock);

static HLIST_HEAD(binder_procs);
static HLIST_HEAD(binder_deferred_list);
static HLIST_HEAD(binder_dead_nodes);

static struct dentry *binder_debugfs_dir_entry_root;
static struct dentry *binder_debugfs_dir_entry_proc;
static struct binder_node *binder_context_mgr_node;
static uid_t binder_context_mgr_uid = -1;
static int binder_last_id;
static struct workqueue_struct *binder_deferred_workqueue;
static pid_t system_server_pid;

#define RT_PRIO_INHERIT			"v1.7"

#define MTK_BINDER_DEBUG 		"v0.1" /* defined for mtk internal added debug code */

/************************************************************************************************************************/
/*	MTK Death Notify	|		  									*/
/*	Debug Log Prefix	|	Description									*/
/*	---------------------------------------------------------------------						*/
/*	[DN #1]			|	Some one requests Death Notify from upper layer.				*/
/*	[DN #2]			|	Some one cancels Death Notify from upper layer.					*/
/*	[DN #3]			|	Binder Driver sends Death Notify to all requesters' Binder Thread.		*/
/*	[DN #4]			|	Some requester's binder_thread_read() handles Death Notify works.		*/
/*	[DN #5]			|	Some requester sends confirmation to Binder Driver. (In IPCThreadState.cpp)	*/
/*	[DN #6]			|	Finally receive requester's confirmation from upper layer.			*/
/************************************************************************************************************************/
#define MTK_DEATH_NOTIFY_MONITOR	"v0.1"

/**
 * Revision history of binder monitor
 *
 * v0.1   - enhance debug log
 * v0.2   - transaction timeout log
 * v0.2.1 - buffer allocation debug
 */
#ifdef CONFIG_MT_ENG_BUILD
#define BINDER_MONITOR			"v0.2.1" /* BINDER_MONITOR only turn on for eng build */
#endif

#ifdef BINDER_MONITOR
#define MAX_SERVICE_NAME_LEN		32
/************************************************************************************************************************/
/*	Payload layout of addService():											*/
/*		| Parcel header | IServiceManager.descriptor | Parcel header | Service name | ...			*/
/*	(Please refer ServiceManagerNative.java:addService())								*/
/*	IServiceManager.descriptor is 'android.os.IServiceManager' interleaved with character '\0'.			*/
/*	that is, 'a', '\0', 'n', '\0', 'd', '\0', 'r', '\0', 'o', ...							*/
/*	so the offset of Service name = Parcel header x2 + strlen(android.os.IServiceManager) x2 = 8x2 + 26x2 = 68	*/
/************************************************************************************************************************/
#define MAGIC_SERVICE_NAME_OFFSET	68

#define MAX_ENG_TRANS_LOG_BUFF_LEN	10240

static int binder_check_buf_pid;
static int binder_check_buf_tid;
static unsigned long binder_log_level = 0;
char aee_msg[512];
char aee_word[100];
static int bt_folder = 0;//just for native backtrace
#define TRANS_LOG_LEN 210
char large_msg[TRANS_LOG_LEN];

#define BINDER_PERF_EVAL		"V0.1"
#endif

#ifdef BINDER_PERF_EVAL
/* binder_perf_evalue bitmap
 * bit: ...3210
 *           ||_ 1: send counter enable
 *           |__ 1: timeout counter enable 
 */   
static unsigned int binder_perf_evalue = 0;
#define BINDER_PERF_SEND_COUNTER	0x1
#define BINDER_PERF_TIMEOUT_COUNTER	0x2
#endif

#define BINDER_DEBUG_ENTRY(name) \
static int binder_##name##_open(struct inode *inode, struct file *file) \
{ \
	return single_open(file, binder_##name##_show, inode->i_private); \
} \
\
static const struct file_operations binder_##name##_fops = { \
	.owner = THIS_MODULE, \
	.open = binder_##name##_open, \
	.read = seq_read, \
	.llseek = seq_lseek, \
	.release = single_release, \
}

#ifdef BINDER_MONITOR
#define BINDER_DEBUG_SETTING_ENTRY(name) \
static int binder_##name##_open(struct inode *inode, struct file *file) \
{ \
	return single_open(file, binder_##name##_show, inode->i_private); \
} \
\
static const struct file_operations binder_##name##_fops = { \
	.owner = THIS_MODULE, \
	.open = binder_##name##_open, \
	.read = seq_read, \
	.write = binder_##name##_write, \
	.llseek = seq_lseek, \
	.release = single_release, \
}
#endif

//LCH add, for binder pages leakage debug
#ifdef CONFIG_MT_ENG_BUILD
#define MTK_BINDER_PAGE_USED_RECORD
#endif

#ifdef MTK_BINDER_PAGE_USED_RECORD
static unsigned int binder_page_used = 0;
static unsigned int binder_page_used_peak = 0;
#endif

static int binder_proc_show(struct seq_file *m, void *unused);
BINDER_DEBUG_ENTRY(proc);

/* This is only defined in include/asm-arm/sizes.h */
#ifndef SZ_1K
#define SZ_1K                               0x400
#endif

#ifndef SZ_4M
#define SZ_4M                               0x400000
#endif

#define FORBIDDEN_MMAP_FLAGS                (VM_WRITE)

#define BINDER_SMALL_BUF_SIZE (PAGE_SIZE * 64)

enum {
	BINDER_DEBUG_USER_ERROR             = 1U << 0,
	BINDER_DEBUG_FAILED_TRANSACTION     = 1U << 1,
	BINDER_DEBUG_DEAD_TRANSACTION       = 1U << 2,
	BINDER_DEBUG_OPEN_CLOSE             = 1U << 3,
	BINDER_DEBUG_DEAD_BINDER            = 1U << 4,
	BINDER_DEBUG_DEATH_NOTIFICATION     = 1U << 5,
	BINDER_DEBUG_READ_WRITE             = 1U << 6,
	BINDER_DEBUG_USER_REFS              = 1U << 7,
	BINDER_DEBUG_THREADS                = 1U << 8,
	BINDER_DEBUG_TRANSACTION            = 1U << 9,
	BINDER_DEBUG_TRANSACTION_COMPLETE   = 1U << 10,
	BINDER_DEBUG_FREE_BUFFER            = 1U << 11,
	BINDER_DEBUG_INTERNAL_REFS          = 1U << 12,
	BINDER_DEBUG_BUFFER_ALLOC           = 1U << 13,
	BINDER_DEBUG_PRIORITY_CAP           = 1U << 14,
	BINDER_DEBUG_BUFFER_ALLOC_ASYNC     = 1U << 15,
};
static uint32_t binder_debug_mask = BINDER_DEBUG_USER_ERROR |
	BINDER_DEBUG_FAILED_TRANSACTION | BINDER_DEBUG_DEAD_TRANSACTION;
module_param_named(debug_mask, binder_debug_mask, uint, S_IWUSR | S_IRUGO);

static bool binder_debug_no_lock;
module_param_named(proc_no_lock, binder_debug_no_lock, bool, S_IWUSR | S_IRUGO);

static DECLARE_WAIT_QUEUE_HEAD(binder_user_error_wait);
static int binder_stop_on_user_error;

static int binder_set_stop_on_user_error(const char *val,
					 struct kernel_param *kp)
{
	int ret;
	ret = param_set_int(val, kp);
	if (binder_stop_on_user_error < 2)
		wake_up(&binder_user_error_wait);
	return ret;
}
module_param_call(stop_on_user_error, binder_set_stop_on_user_error,
	param_get_int, &binder_stop_on_user_error, S_IWUSR | S_IRUGO);

#define binder_debug(mask, x...) \
	do { \
		if (binder_debug_mask & mask) \
			printk(KERN_INFO x); \
	} while (0)

#ifdef BINDER_MONITOR
#define binder_user_error(x...) \
	do { \
		if (binder_debug_mask & BINDER_DEBUG_USER_ERROR) \
			printk(KERN_ERR x); \
		if (binder_stop_on_user_error) \
			binder_stop_on_user_error = 2; \
	} while (0)
#else
#define binder_user_error(x...) \
	do { \
		if (binder_debug_mask & BINDER_DEBUG_USER_ERROR) \
			printk(KERN_INFO x); \
		if (binder_stop_on_user_error) \
			binder_stop_on_user_error = 2; \
	} while (0)
#endif

enum binder_stat_types {
	BINDER_STAT_PROC,
	BINDER_STAT_THREAD,
	BINDER_STAT_NODE,
	BINDER_STAT_REF,
	BINDER_STAT_DEATH,
	BINDER_STAT_TRANSACTION,
	BINDER_STAT_TRANSACTION_COMPLETE,
	BINDER_STAT_COUNT
};

struct binder_stats {
	int br[_IOC_NR(BR_FAILED_REPLY) + 1];
	int bc[_IOC_NR(BC_DEAD_BINDER_DONE) + 1];
	int obj_created[BINDER_STAT_COUNT];
	int obj_deleted[BINDER_STAT_COUNT];
};

static struct binder_stats binder_stats;

static inline void binder_stats_deleted(enum binder_stat_types type)
{
	binder_stats.obj_deleted[type]++;
}

static inline void binder_stats_created(enum binder_stat_types type)
{
	binder_stats.obj_created[type]++;
}

struct binder_transaction_log_entry {
	int debug_id;
	int call_type;
	int from_proc;
	int from_thread;
	int target_handle;
	int to_proc;
	int to_thread;
	int to_node;
	int data_size;
	int offsets_size;
#ifdef BINDER_MONITOR
	int code;
	struct timespec timestamp;
	char service[MAX_SERVICE_NAME_LEN];
	int fd;
	struct timeval tv;
	struct timespec readstamp;
	struct timespec endstamp;
#endif
};
struct binder_transaction_log {
	int next;
	int full;
#ifdef BINDER_MONITOR
	unsigned size;
	struct binder_transaction_log_entry *entry;
#else
	struct binder_transaction_log_entry entry[32];
#endif
};
static struct binder_transaction_log binder_transaction_log;
static struct binder_transaction_log binder_transaction_log_failed;

static struct binder_transaction_log_entry *binder_transaction_log_add(
	struct binder_transaction_log *log)
{
	struct binder_transaction_log_entry *e;
	e = &log->entry[log->next];
	memset(e, 0, sizeof(*e));
	log->next++;
#ifdef BINDER_MONITOR
	if (log->next == log->size) {
		log->next = 0;
		log->full = 1;
	}
#else
	if (log->next == ARRAY_SIZE(log->entry)) {
		log->next = 0;
		log->full = 1;
	}
#endif
	return e;
}

#ifdef BINDER_MONITOR
static struct binder_transaction_log_entry entry_failed[32];

/* log_disable bitmap
 * bit: 31...43210
 *       |   |||||_ 0: log enable / 1: log disable
 *       |   ||||__ 1: self resume 
 *       |   |||____2: manually trigger kernel warning for buffer allocation
 *       |   ||____ 3: 1:rt_inherit log enable / 0: rt_inherit log disable
 *       |   |       
 */          
static int log_disable;
#define BINDER_LOG_RESUME	0x2
#define BINDER_BUF_WARN		0x4
#ifdef RT_PRIO_INHERIT
#define BINDER_RT_LOG_ENABLE	0x8
#endif
#ifdef CONFIG_MTK_EXTMEM
extern void* extmem_malloc_page_align(size_t bytes);
#else
static struct binder_transaction_log_entry entry_t[MAX_ENG_TRANS_LOG_BUFF_LEN];
#endif
#endif

struct binder_work {
	struct list_head entry;
	enum {
		BINDER_WORK_TRANSACTION = 1,
		BINDER_WORK_TRANSACTION_COMPLETE,
		BINDER_WORK_NODE,
		BINDER_WORK_DEAD_BINDER,
		BINDER_WORK_DEAD_BINDER_AND_CLEAR,
		BINDER_WORK_CLEAR_DEATH_NOTIFICATION,
	} type;
};

struct binder_node {
	int debug_id;
	struct binder_work work;
	union {
		struct rb_node rb_node;
		struct hlist_node dead_node;
	};
	struct binder_proc *proc;
	struct hlist_head refs;
	int internal_strong_refs;
	int local_weak_refs;
	int local_strong_refs;
	void __user *ptr;
	void __user *cookie;
	unsigned has_strong_ref:1;
	unsigned pending_strong_ref:1;
	unsigned has_weak_ref:1;
	unsigned pending_weak_ref:1;
	unsigned has_async_transaction:1;
	unsigned accept_fds:1;
	unsigned min_priority:8;
	struct list_head async_todo;
#ifdef BINDER_MONITOR
	char name[MAX_SERVICE_NAME_LEN];
#endif
};

struct binder_ref_death {
	struct binder_work work;
	void __user *cookie;
};

struct binder_ref {
	/* Lookups needed: */
	/*   node + proc => ref (transaction) */
	/*   desc + proc => ref (transaction, inc/dec ref) */
	/*   node => refs + procs (proc exit) */
	int debug_id;
	struct rb_node rb_node_desc;
	struct rb_node rb_node_node;
	struct hlist_node node_entry;
	struct binder_proc *proc;
	struct binder_node *node;
	uint32_t desc;
	int strong;
	int weak;
	struct binder_ref_death *death;
};

struct binder_buffer {
	struct list_head entry; /* free and allocated entries by address */
	struct rb_node rb_node; /* free entry by size or allocated entry */
				/* by address */
	unsigned free:1;
	unsigned allow_user_free:1;
	unsigned async_transaction:1;
	unsigned debug_id:29;

	struct binder_transaction *transaction;
#ifdef BINDER_MONITOR
	struct binder_transaction_log_entry *log_entry;
#endif
	struct binder_node *target_node;
	size_t data_size;
	size_t offsets_size;
	uint8_t data[0];
};

enum binder_deferred_state {
	BINDER_DEFERRED_PUT_FILES    = 0x01,
	BINDER_DEFERRED_FLUSH        = 0x02,
	BINDER_DEFERRED_RELEASE      = 0x04,
};

#ifdef BINDER_MONITOR
enum wait_on_reason {
	WAIT_ON_NONE       = 0U,
	WAIT_ON_READ       = 1U,
	WAIT_ON_EXEC       = 2U,
	WAIT_ON_REPLY_READ = 3U
};
#endif

#ifdef BINDER_PERF_EVAL
#define BC_CODE_NR 60
#define BC_STATS_NR 30
struct binder_bc_stats {
	char service[MAX_SERVICE_NAME_LEN];
	int code[BC_CODE_NR];
	unsigned int code_num[BC_CODE_NR]; 
};
struct binder_timeout_stats {
	unsigned long bto[WAIT_ON_REPLY_READ];
	struct timespec read_t[32];
	struct timespec exec_t[32];
	struct timespec rrply_t[32];
};
#endif

struct binder_proc {
	struct hlist_node proc_node;
	struct rb_root threads;
	struct rb_root nodes;
	struct rb_root refs_by_desc;
	struct rb_root refs_by_node;
	int pid;
	struct vm_area_struct *vma;
	struct mm_struct *vma_vm_mm;
	struct task_struct *tsk;
	struct files_struct *files;
	struct hlist_node deferred_work_node;
	int deferred_work;
	void *buffer;
	ptrdiff_t user_buffer_offset;

	struct list_head buffers;
	struct rb_root free_buffers;
	struct rb_root allocated_buffers;
	size_t free_async_space;

	struct page **pages;
	size_t buffer_size;
	uint32_t buffer_free;
	struct list_head todo;
	wait_queue_head_t wait;
	struct binder_stats stats;
	struct list_head delivered_death;
	int max_threads;
	int requested_threads;
	int requested_threads_started;
	int ready_threads;
	long default_priority;
	struct dentry *debugfs_entry;
#ifdef RT_PRIO_INHERIT
	unsigned long default_rt_prio:16;
	unsigned long default_policy:16;
#endif
#ifdef BINDER_MONITOR
	struct binder_buffer *large_buffer;
#endif
#ifdef BINDER_PERF_EVAL
	int bc_t;
	struct binder_bc_stats *bc_stats[BC_STATS_NR];	
	struct binder_timeout_stats to_stats;
#endif
#ifdef MTK_BINDER_PAGE_USED_RECORD
	unsigned int page_used;
	unsigned int page_used_peak;
#endif
};

enum {
	BINDER_LOOPER_STATE_REGISTERED  = 0x01,
	BINDER_LOOPER_STATE_ENTERED     = 0x02,
	BINDER_LOOPER_STATE_EXITED      = 0x04,
	BINDER_LOOPER_STATE_INVALID     = 0x08,
	BINDER_LOOPER_STATE_WAITING     = 0x10,
	BINDER_LOOPER_STATE_NEED_RETURN = 0x20
};

struct binder_thread {
	struct binder_proc *proc;
	struct rb_node rb_node;
	int pid;
	int looper;
	struct binder_transaction *transaction_stack;
	struct list_head todo;
	uint32_t return_error; /* Write failed, return error code in read buf */
	uint32_t return_error2; /* Write failed, return error code in read */
		/* buffer. Used when sending a reply to a dead process that */
		/* we are also waiting on */
	wait_queue_head_t wait;
	struct binder_stats stats;
#ifdef BINDER_PERF_EVAL
	struct binder_timeout_stats to_stats;
#endif

};

struct binder_transaction {
	int debug_id;
	struct binder_work work;
	struct binder_thread *from;
	struct binder_transaction *from_parent;
	struct binder_proc *to_proc;
	struct binder_thread *to_thread;
	struct binder_transaction *to_parent;
	unsigned need_reply:1;
	/* unsigned is_dead:1; */	/* not used at the moment */

	struct binder_buffer *buffer;
	unsigned int	code;
	unsigned int	flags;
	long	priority;
	long	saved_priority;
	uid_t	sender_euid;
#ifdef RT_PRIO_INHERIT
	unsigned long rt_prio:16;
	unsigned long policy:16;
	unsigned long saved_rt_prio:16;
	unsigned long saved_policy:16;
#endif
#ifdef BINDER_MONITOR
	struct timespec timestamp;

	enum wait_on_reason wait_on;
	enum wait_on_reason bark_on;
	struct rb_node rb_node; /* by bark_time */
	struct timespec bark_time;
	struct timespec exe_timestamp;
	struct timeval tv;
	char service[MAX_SERVICE_NAME_LEN];
	pid_t fproc;
	pid_t fthrd;
	pid_t tproc;
	pid_t tthrd;
	unsigned int log_idx;
#endif
};

static void
binder_defer_work(struct binder_proc *proc, enum binder_deferred_state defer);
static inline void binder_lock(const char *tag);
static inline void binder_unlock(const char *tag);

#ifdef BINDER_MONITOR
/* work should be done within how many secs */
#define WAIT_BUDGET_READ  2
#define WAIT_BUDGET_EXEC  4
#define WAIT_BUDGET_MIN   min(WAIT_BUDGET_READ, WAIT_BUDGET_EXEC)

static struct rb_root bwdog_transacts;

static const char *binder_wait_on_str[] = {
	"none",
	"read",
	"exec",
	"rply"
};

struct binder_timeout_log_entry {
	enum wait_on_reason r;
	pid_t from_proc;
	pid_t from_thrd;
	pid_t to_proc;
	pid_t to_thrd;
	unsigned over_sec;
	struct timespec ts;
	struct timeval  tv;
	unsigned int code;
	char service[MAX_SERVICE_NAME_LEN];
};	

struct binder_timeout_log {
	int next;
	int full;
#ifdef BINDER_PERF_EVAL
	struct binder_timeout_log_entry entry[256];
#else
	struct binder_timeout_log_entry entry[64];
#endif
};

static struct binder_timeout_log binder_timeout_log_t;

/**
 * binder_timeout_log_add - Insert a timeout log
 */
static struct binder_timeout_log_entry *binder_timeout_log_add(void)
{
	struct binder_timeout_log *log = &binder_timeout_log_t;
	struct binder_timeout_log_entry *e;

	e = &log->entry[log->next];
	memset(e, 0, sizeof(*e));
	log->next++;
	if (log->next == ARRAY_SIZE(log->entry)) {
		log->next = 0;
		log->full = 1;
	}
	return e;
}

/**
 * binder_print_bwdog - Output info of a timeout transaction
 * @t:		pointer to the timeout transaction
 * @cur_in:	current timespec while going to print
 * @e:		timeout log entry to record
 * @r:		output reason, either while barking or after barked
 */
static void binder_print_bwdog(struct binder_transaction *t,
			       struct timespec *cur_in,
			       struct binder_timeout_log_entry *e,
			       enum wait_on_reason r)
{
	struct rtc_time tm;
	struct timespec *startime;
	struct timespec cur, sub_t;

	if (cur_in && e) {
		memcpy(&cur, cur_in, sizeof(struct timespec));
	} else {
		do_posix_clock_monotonic_gettime(&cur);
		//monotonic_to_bootbased(&cur);
	}
	startime = (r == WAIT_ON_EXEC) ? &t->exe_timestamp : &t->timestamp;
	sub_t = timespec_sub(cur, *startime);

#ifdef BINDER_PERF_EVAL
	if ( !(cur_in && e) && (binder_perf_evalue & BINDER_PERF_TIMEOUT_COUNTER))
	{
		switch (r)
		{
			case WAIT_ON_READ:
				{
					if (t->to_proc)
					{
						unsigned long proc_t = t->to_proc->to_stats.bto[r - 1] ++;
						t->to_proc->to_stats.read_t[(proc_t % 32)] = sub_t;
					}
					break;
				}
			case WAIT_ON_EXEC:
				{
					if (t->to_proc)
					{
						unsigned long proc_t = t->to_proc->to_stats.bto[r - 1] ++;
						t->to_proc->to_stats.exec_t[(proc_t % 32)] = sub_t;
					}
					if (t->to_thread)
					{
						unsigned long thread_t = t->to_thread->to_stats.bto[r - 1] ++;
						t->to_thread->to_stats.exec_t[(thread_t % 32)] = sub_t;
					}
					break;
				}
			case WAIT_ON_REPLY_READ:
				{
					if (t->to_proc)
					{
						unsigned long proc_t = t->to_proc->to_stats.bto[r - 1] ++;
						t->to_proc->to_stats.rrply_t[(proc_t % 32)] = sub_t;
					}
					if (t->to_thread)
					{
						unsigned long thread_t = t->to_thread->to_stats.bto[r - 1] ++;
						t->to_thread->to_stats.rrply_t[(thread_t % 32)] = sub_t;
					}

					break;
				}
			case WAIT_ON_NONE:
				break;
			default:
				break;
		}
	}
#endif
	rtc_time_to_tm(t->tv.tv_sec, &tm);
	printk(KERN_INFO "binder: %d %s %d:%d to %d:%d %s %u.%03ld "
			 "sec (%s) dex_code %u start_at %lu.%03ld android "
			 "%d-%02d-%02d %02d:%02d:%02d.%03lu\n",
			 t->debug_id, binder_wait_on_str[r],
			 t->fproc, t->fthrd, t->tproc, t->tthrd,
			 (cur_in && e) ? "over" : "total",
			 (unsigned)sub_t.tv_sec, (sub_t.tv_nsec / NSEC_PER_MSEC), 
			 t->service, t->code,

			 (unsigned long)startime->tv_sec,
	 		 (startime->tv_nsec / NSEC_PER_MSEC),
			 (tm.tm_year + 1900), (tm.tm_mon + 1), tm.tm_mday,
			 tm.tm_hour, tm.tm_min, tm.tm_sec,
			 (unsigned long)(t->tv.tv_usec / USEC_PER_MSEC));

	if (e) {
		e->over_sec = sub_t.tv_sec;
		memcpy(&e->ts, startime, sizeof(struct timespec));
	}
}

/**
 * binder_bwdog_safe - Check a transaction is monitor-free or not
 * @t:	pointer to the transaction to check
 *
 * Returns 1 means safe.
 */
static inline int binder_bwdog_safe(struct binder_transaction *t)
{
	return (t->wait_on == WAIT_ON_NONE) ? 1 : 0;
}

/**
 * binder_query_bwdog - Check a transaction is queued or not
 * @t:	pointer to the transaction to check
 * 
 * Returns a pointer points to t, or NULL if it's not queued. 
 */
static struct rb_node **binder_query_bwdog(struct binder_transaction *t)
{
	struct rb_node **p = &bwdog_transacts.rb_node;
	struct rb_node *parent = NULL;
	struct binder_transaction *transact = NULL;
	int comp;

	while (*p) {
		parent = *p;
		transact = rb_entry(parent, struct binder_transaction, rb_node);

		comp = timespec_compare(&t->bark_time, &transact->bark_time);
		if (comp < 0)
			p = &(*p)->rb_left;
		else if (comp > 0)
			p = &(*p)->rb_right;
		else
			break;
	}
	return p;
}

/**
 * binder_queue_bwdog - Queue a transaction to keep tracking
 * @t:		pointer to the transaction being tracked
 * @budget:	seconds, which this transaction can afford
 */
static void binder_queue_bwdog(struct binder_transaction *t, time_t budget)
{
	struct rb_node **p = &bwdog_transacts.rb_node;
	struct rb_node *parent = NULL;
	struct binder_transaction *transact = NULL;
	int ret;

	do_posix_clock_monotonic_gettime(&t->bark_time);
	//monotonic_to_bootbased(&t->bark_time);
	t->bark_time.tv_sec += budget;

	while (*p) {
		parent = *p;
		transact = rb_entry(parent, struct binder_transaction, rb_node);

		ret = timespec_compare(&t->bark_time, &transact->bark_time);
		if (ret < 0)
			p = &(*p)->rb_left;
		else if (ret > 0)
			p = &(*p)->rb_right;
		else {
			printk(KERN_INFO "binder: %d found same key\n",
					 t->debug_id);
			t->bark_time.tv_nsec += 1;
			p = &(*p)->rb_right;
		}
	}
	rb_link_node(&t->rb_node, parent, p);
	rb_insert_color(&t->rb_node, &bwdog_transacts);
}

/**
 * binder_cancel_bwdog - Cancel a transaction from tracking list
 * @t:		pointer to the transaction being cancelled
 */
static void binder_cancel_bwdog(struct binder_transaction *t)
{
	struct rb_node **p = NULL;

	if (binder_bwdog_safe(t)) {
		if (t->bark_on) {
			binder_print_bwdog(t, NULL, NULL, t->bark_on);
			t->bark_on = WAIT_ON_NONE;
		}
		return;
	}

	p = binder_query_bwdog(t);
	if (*p == NULL) {
		printk(KERN_ERR "binder: %d waits %s, but not queued...\n",
				t->debug_id, binder_wait_on_str[t->wait_on]);
		return;
	}

	rb_erase(&t->rb_node, &bwdog_transacts);
	t->wait_on = WAIT_ON_NONE;
}

/**
 * binder_bwdog_bark -
 *     Barking funcion while timeout. Record target process or thread, which
 * cannot handle transaction in time, including todo list. Also add a log
 * entry for AMS reference.
 *
 * @t:		pointer to the transaction, which triggers watchdog
 * @cur:	current kernel timespec
 */
static void binder_bwdog_bark(struct binder_transaction *t, struct timespec *cur)
{
	struct binder_timeout_log_entry *e;


	if (binder_bwdog_safe(t)) {
		printk(KERN_INFO "binder: %d watched, but wait nothing\n",
				 t->debug_id);
		return;
	}

	e = binder_timeout_log_add();
	binder_print_bwdog(t, cur, e, t->wait_on);

	e->r = t->wait_on;
	e->from_proc = t->fproc;
	e->from_thrd = t->fthrd;
	memcpy(&e->tv, &t->tv, sizeof(struct timeval));

	switch (t->wait_on) {
	case WAIT_ON_READ: {
		if (!t->to_proc) {
			printk(KERN_ERR "binder: %d has NULL target\n",
					t->debug_id);
			return;
		}
		e->to_proc = t->tproc;
		e->to_thrd = t->tthrd;
		e->code = t->code;
		strcpy(e->service, t->service);
		break;
	}

	case WAIT_ON_EXEC: {
		if (!t->to_thread) {
			printk(KERN_ERR "binder: %d has NULL target for "
					"execution\n", t->debug_id);
			return;
		}
		e->to_proc = t->tproc;
		e->to_thrd = t->tthrd;
		e->code = t->code; 
		strcpy(e->service, t->service);
		goto dumpBackTrace;
	}

	case WAIT_ON_REPLY_READ: {
		if (!t->to_thread) {
			printk(KERN_ERR "binder: %d has NULL target thread\n",
					t->debug_id);
			return;
		}
		e->to_proc = t->tproc;
		e->to_thrd = t->tthrd;
		strcpy(e->service, "");
		break;
	}

	default: {
		return;
	}
	}

dumpBackTrace:
	return;
}

/**
 * binder_bwdog_thread - Main thread to check timeout list periodically
 */
static int binder_bwdog_thread(void *__unused)
{
	unsigned long sleep_sec;
	struct rb_node *n = NULL;
	struct timespec cur_time;
	struct binder_transaction *t = NULL;

	for (;;) {
		binder_lock(__func__);
		do_posix_clock_monotonic_gettime(&cur_time);
		//monotonic_to_bootbased(&cur_time);

		for (n = rb_first(&bwdog_transacts); n != NULL; n = rb_next(n)) {
			t = rb_entry(n, struct binder_transaction, rb_node);
			if (timespec_compare(&cur_time, &t->bark_time) < 0)
				break;
	
			binder_bwdog_bark(t, &cur_time);
			rb_erase(&t->rb_node, &bwdog_transacts);
			t->bark_on = t->wait_on;
			t->wait_on = WAIT_ON_NONE;
		}

		if (!n)
			sleep_sec = WAIT_BUDGET_MIN;
		else
		 	sleep_sec = timespec_sub(t->bark_time, cur_time).tv_sec;
		binder_unlock(__func__);

		msleep(sleep_sec * MSEC_PER_SEC);
	}
	printk(KERN_INFO "binder: %s exit...\n", __func__);
	return 0;
}

/**
 * binder_usermodehelper - Call shell to do some command
 * @cmd:	string of command
 * @w:		wait type 
 */
static void binder_usermodehelper(char *cmd, int w)
{
	int ret;
	char *envp[] = {"HOME=/", "TERM=linux", "PATH=/sbin:/system/bin", NULL};
	char *argv[] = {"/system/bin/sh", "-c", "", NULL};

	argv[2] = cmd;
	printk(KERN_INFO "binder: %s\n", argv[2]);
	if ((ret = call_usermodehelper(argv[0], argv, envp, w)) != 0)
		printk(KERN_ERR "binder: %s: return %d\n", __func__, ret);
}

/**
 * binder_check_is_javap - check if tsk is a java process
 * @tsk:	task_struct of process which to check
 */
static int binder_check_is_javap(struct task_struct *tsk)
{
	char proc_name[TASK_COMM_LEN] = "null";
	if (NULL == tsk)
		return -1;
	if (tsk->real_parent)
		get_task_comm(proc_name, tsk->real_parent);
	if (0 == strcmp(proc_name, "zygote"))
		return 1;
	else
		return 0;
}

/**
 * binder_call_jbt - Dump java backtrace
 * @pid:	pid of target process
 */
static void binder_call_jbt(pid_t pid)
{
	char tmp[100];

	sprintf(tmp, "/system/bin/kill -16 %d", pid);
	binder_usermodehelper(tmp, UMH_WAIT_EXEC);
}

static void binder_call_nbt(pid_t pid, char* name)
{
	char tmp[100];
	char dname[20];

	sprintf(dname, "/data/binder");
	if (bt_folder == 0) {
		sprintf(tmp, "system/bin/mkdir %s", dname);
		binder_usermodehelper(tmp, UMH_WAIT_PROC);
		bt_folder = 1;
	}
	sprintf(tmp, "/system/bin/debuggerd -b %d > %s/binder_native%s.txt",
		pid, dname, name);
	binder_usermodehelper(tmp, UMH_WAIT_EXEC);
}

/**
 * find_process_by_pid - convert pid to task_struct
 * @pid:	pid for convert task
 */
static inline struct task_struct *find_process_by_pid(pid_t pid)
{
	return pid ? find_task_by_vpid(pid) : NULL; 
}
/**
 * binder_find_buffer_sender - find the sender task_struct of this buffer
 * @buf	binder buffer
 * @tsk	task_struct of buf sender
 */
static struct task_struct *binder_find_buffer_sender(struct binder_buffer *buf)
{
	struct binder_transaction *t;
	struct binder_transaction_log_entry *e;
	struct task_struct *tsk;
	t = buf->transaction;
	if (t && t->fproc)
		tsk = find_process_by_pid(t->fproc);

	else
	{
		e = buf->log_entry;
		if ((buf->debug_id == e->debug_id) && e->from_proc)
			tsk = find_process_by_pid(e->from_proc);
		else
			tsk = NULL;
	}
	return tsk;
}

/**
 * copy from /kernel/fs/proc/base.c and modified to get task full name
 */
static int binder_proc_pid_cmdline(struct task_struct *task, char * buf)
{
	int res = 0;
	unsigned int len;
	struct mm_struct *mm;
	/*============ add begin =============================*/
	char c = ' ';
	char *str;
	unsigned int size;
	char *buffer;

	if (NULL == task)
		goto out;
	/*============ add end  ===============================*/
	mm = get_task_mm(task);
	if (!mm)
		goto out;
	if (!mm->arg_end)
		goto out_mm;	/* Shh! No looking before we're done */
	/*============ add begin =============================*/
	buffer = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (NULL == buffer)
		goto out_mm;
	/*============ add end  ===============================*/

 	len = mm->arg_end - mm->arg_start;
 
	if (len > PAGE_SIZE)
		len = PAGE_SIZE;
 
	res = access_process_vm(task, mm->arg_start, buffer, len, 0);

	// If the nul at the end of args has been overwritten, then
	// assume application is using setproctitle(3).
	if (res > 0 && buffer[res-1] != '\0' && len < PAGE_SIZE) {
		len = strnlen(buffer, res);
		if (len < res) {
		    res = len;
		} else {
			len = mm->env_end - mm->env_start;
			if (len > PAGE_SIZE - res)
				len = PAGE_SIZE - res;
			res += access_process_vm(task, mm->env_start, buffer+res, len, 0);
			res = strnlen(buffer, res);
		}
	}
	/*============ add begin =============================*/
	str = strchr(buffer, c);
	if (NULL != str)
		size = (unsigned int)(str - buffer);
	else
		size = res;
	if (size > 256)
		size = 256;
	snprintf(buf, size, buffer);
	kfree(buffer);
	/*============ add end  ===============================*/
out_mm:
	mmput(mm);
out:
	return res;
}

/**
 * binder_print_buf - Print buffer info
 * @t:		transaction
 * @buffer:	target buffer
 * @dest:	dest string pointer
 * @success:	does this buffer allocate success
 * @check:	check this log for owner finding
 */
static void binder_print_buf(struct binder_buffer *buffer, char *dest, int success, int check)
{
	struct rtc_time tm;
	struct binder_transaction *t = buffer->transaction;
	char str[TRANS_LOG_LEN];
	struct task_struct *sender_tsk;
	struct task_struct *rec_tsk;
	char sender_name[256], rec_name[256];
	int len_s, len_r;
	if (NULL == t) {
		struct binder_transaction_log_entry *log_entry = buffer->log_entry;
		rtc_time_to_tm(log_entry->tv.tv_sec, &tm);
		if ((log_entry != NULL) && (buffer->debug_id == log_entry->debug_id))
		{
			sender_tsk = find_process_by_pid(log_entry->from_proc);
			rec_tsk = find_process_by_pid(log_entry->to_proc);
			len_s = binder_proc_pid_cmdline(sender_tsk, sender_name);
			len_r = binder_proc_pid_cmdline(rec_tsk, rec_name);
			snprintf(str, TRANS_LOG_LEN,
					"binder:check=%d,success=%d,id=%d,call=%s,type=%s,"
					"from=%d,tid=%d,name=%s,to=%d,name=%s,tid=%d,name=%s,"
					"size=%d,node=%d,handle=%d,dex=%u,auf=%d,start=%lu.%03ld,"
					"android=%d-%02d-%02d %02d:%02d:%02d.%03lu\n",
					check, success, buffer->debug_id,
					buffer->async_transaction ? "async" : "sync",
					(2 == log_entry->call_type) ? "reply" : 
					((1 == log_entry->call_type) ? "async" : "call"),
					log_entry->from_proc, log_entry->from_thread,
					len_s ? sender_name : ((sender_tsk != NULL) ? sender_tsk->comm : ""),
					log_entry->to_proc,
					len_r ? rec_name : ((rec_tsk != NULL) ? rec_tsk->comm : ""),
					log_entry->to_thread, log_entry->service,
					(buffer->data_size + buffer->offsets_size),
					log_entry->to_node, log_entry->target_handle, log_entry->code,
					buffer->allow_user_free,
					(unsigned long)log_entry->timestamp.tv_sec,
					(log_entry->timestamp.tv_nsec / NSEC_PER_MSEC),
					(tm.tm_year + 1900), (tm.tm_mon + 1), tm.tm_mday,
					tm.tm_hour, tm.tm_min, tm.tm_sec,
					(unsigned long)(log_entry->tv.tv_usec / USEC_PER_MSEC));
		}
		else
			snprintf(str, TRANS_LOG_LEN, "binder:check=%d,success=%d,id=%d,call=%s, ,"
					",,,,,,,size=%d,,,,"
					"auf=%d,,\n",
					check, success, buffer->debug_id,
					buffer->async_transaction ? "async" : "sync",
					(buffer->data_size + buffer->offsets_size),
					buffer->allow_user_free);
	}
	else
	{
		rtc_time_to_tm(t->tv.tv_sec, &tm);
		sender_tsk = find_process_by_pid(t->fproc);
		rec_tsk = find_process_by_pid(t->tproc);
		len_s = binder_proc_pid_cmdline(sender_tsk, sender_name);
		len_r = binder_proc_pid_cmdline(rec_tsk, rec_name);
		snprintf(str, TRANS_LOG_LEN,
				"binder:check=%d,success=%d,id=%d,call=%s,type=%s,"
				"from=%d,tid=%d,name=%s,to=%d,name=%s,tid=%d,name=%s,"
				"size=%d,,,dex=%u,auf=%d,start=%lu.%03ld,android="
				"%d-%02d-%02d %02d:%02d:%02d.%03lu\n",
				check, success, t->debug_id,
				buffer->async_transaction ? "async" : "sync ",
				binder_wait_on_str[t->wait_on],
				t->fproc, t->fthrd,
				len_s ? sender_name : ((sender_tsk != NULL) ? sender_tsk->comm : ""),
				t->tproc,
				len_r ? rec_name : ((rec_tsk != NULL) ? rec_tsk->comm : ""),
				t->tthrd, t->service,
				(buffer->data_size+buffer->offsets_size), t->code,
				buffer->allow_user_free,
				(unsigned long)t->timestamp.tv_sec,
				(t->timestamp.tv_nsec / NSEC_PER_MSEC),
				(tm.tm_year + 1900), (tm.tm_mon + 1), tm.tm_mday,
				tm.tm_hour, tm.tm_min, tm.tm_sec,
				(unsigned long)(t->tv.tv_usec / USEC_PER_MSEC));
	}
	printk(KERN_INFO "%s", str);
	if (dest != NULL)
		strcat(dest, str);
}

/**
 * binder_check_buf_checked -
 *     Consider buffer related issue usually makes a series of failure.
 * Only care about the first problem time to minimize debug overhead.
 */
static int binder_check_buf_checked(void)
{
	return (binder_check_buf_pid == -1);
}

static size_t binder_buffer_size(struct binder_proc *proc,
		struct binder_buffer *buffer);

/**
 * binder_check_buf - Dump necessary info for buffer usage analysis
 * @target_proc: 	receiver
 * @size:		requested size
 * @is_async:		1 if an async call
 */
static void binder_check_buf(struct binder_proc *target_proc,
		size_t size, int is_async)
{
	struct rb_node *n;
	struct binder_buffer *buffer;
	int i;
	int large_buffer_count = 0;
	size_t tmp_size, threshold;
	struct task_struct *sender;
	struct task_struct *larger;
	char sender_name[256], rec_name[256];
	struct timespec exp_timestamp;
	struct timeval tv;
	struct rtc_time tm;
	int db_flag = DB_OPT_BINDER_INFO;
	int len_s, len_r;

	printk(KERN_INFO "binder: buffer allocation failed on %d:0 "
			"%s from %d:%d size %zd\n",
			target_proc->pid,
			is_async ? "async" : "call ",
			binder_check_buf_pid, binder_check_buf_tid, size);

	if (binder_check_buf_checked())
		return;
	/* check blocked service for async call */
	if (is_async) {
		printk(KERN_INFO "binder: buffer allocation failed on %d:0 "
				"(%s) async service blocked\n",
				target_proc->pid,
				target_proc->tsk ? target_proc->tsk->comm : "");
	}

	printk(KERN_INFO "binder: %d:0 pending transactions:\n", target_proc->pid);
	threshold = target_proc->buffer_size/16;
	for (n = rb_last(&target_proc->allocated_buffers), i = 0;
			n; n = rb_prev(n), i++) 
	{
		buffer = rb_entry(n, struct binder_buffer, rb_node);
		tmp_size = binder_buffer_size(target_proc, buffer);
		BUG_ON(buffer->free);

		if (tmp_size > threshold)
		{
			if ((NULL == target_proc->large_buffer) ||
					(target_proc->large_buffer &&
					 (tmp_size > binder_buffer_size(target_proc, target_proc->large_buffer))))
				target_proc->large_buffer = buffer;
			large_buffer_count ++;
			binder_print_buf(buffer, NULL, 1, 0);
		}
		else
		{
			if (i < 20)
				binder_print_buf(buffer, NULL, 1, 0);
		}
	}
	printk(KERN_INFO "binder: %d:0 total pending trans: %d(%d large isze)\n",
			target_proc->pid, i, large_buffer_count);

	do_posix_clock_monotonic_gettime(&exp_timestamp);
	//monotonic_to_bootbased(&exp_timestamp);
	do_gettimeofday(&tv);
	/* consider time zone. translate to android time */
	tv.tv_sec -= (sys_tz.tz_minuteswest * 60);
	rtc_time_to_tm(tv.tv_sec, &tm);

	sender = find_process_by_pid(binder_check_buf_pid);
	len_s = binder_proc_pid_cmdline(sender, sender_name);
	len_r = binder_proc_pid_cmdline(target_proc->tsk, rec_name);
	if (size > threshold)
	{
		if (target_proc->large_buffer)
		{
			printk(KERN_INFO "binder: on %d:0 the largest pending trans is:\n",
					target_proc->pid);
			binder_print_buf(target_proc->large_buffer, large_msg, 1, 0);
		}		
		sprintf(aee_word,"check %s: large binder trans fail on %d:0 size %d",
				len_s ? sender_name : ((sender != NULL) ? sender->comm : ""),
				target_proc->pid, size);
		snprintf(aee_msg, 512, "BINDER_BUF_DEBUG\n%s"
				"binder:check=%d,success=%d,,call=%s,,from=%d,tid=%d,"
				"name=%s,to=%d,name=%s,,,size=%d,,,,"
				",start=%lu.%03ld,android="
				"%d-%02d-%02d %02d:%02d:%02d.%03lu\n"
				"large data size,check sender %d(%s)!\n"
				"check kernel log for more info\n", 
				large_msg, 1, 0, is_async ? "async" : "sync", 
				binder_check_buf_pid, binder_check_buf_tid, 
				len_s ? sender_name : ((sender != NULL) ? sender->comm : ""),
				target_proc->pid, 
				len_r ? rec_name :
				((target_proc->tsk != NULL) ? target_proc->tsk->comm : ""),
				size,
				(unsigned long)exp_timestamp.tv_sec,
				(exp_timestamp.tv_nsec / NSEC_PER_MSEC),
				(tm.tm_year + 1900), (tm.tm_mon + 1), tm.tm_mday,
				tm.tm_hour, tm.tm_min, tm.tm_sec,
				(unsigned long)(tv.tv_usec / USEC_PER_MSEC),
				binder_check_buf_pid, sender ? sender->comm : "");
		if (binder_check_is_javap(sender) > 0)
		{
			db_flag = db_flag | DB_OPT_SWT_JBT_TRACES;
			binder_call_jbt(binder_check_buf_pid);
		}
		else
		{
			db_flag = db_flag | DB_OPT_SWT_JBT_TRACES;
			binder_call_nbt(binder_check_buf_pid, "1");
			printk(KERN_INFO "binder:sender pid %d (%s) is not javap, pls check SYS_BINDER_BACKTRACE!\n",
					binder_check_buf_pid,
					(sender != NULL) ? sender->comm : "");
		}
	}
	else
	{
		if (target_proc->large_buffer)
		{
			printk(KERN_INFO "binder: on %d:0 the largest pending trans is:\n",
					target_proc->pid);
			binder_print_buf(target_proc->large_buffer, large_msg, 1, 1);
			larger = binder_find_buffer_sender(target_proc->large_buffer);
			sprintf(aee_word, "check %s: large binder trans",
					(larger != NULL) ? larger->comm : "");
			snprintf(aee_msg, 512, "BINDER_BUF_DEBUG:\n%s"
					"binder:check=%d,success=%d,,call=%s,,from=%d,tid=%d,name=%s,"
					"to=%d,name=%s,,,size=%d,,,,"
					",start=%lu.%03ld,android="
					"%d-%02d-%02d %02d:%02d:%02d.%03lu\n"
					"large data size,check sender %d(%s)!\n"
					"check kernel log for more info\n",
					large_msg, 0, 0, is_async ? "async" : "sync",
					binder_check_buf_pid, binder_check_buf_tid,
					len_s ? sender_name : ((sender != NULL) ? sender->comm : ""),
					target_proc->pid,
					len_r ? rec_name :
					((target_proc->tsk != NULL) ? target_proc->tsk->comm : ""),
					size,
					(unsigned long)exp_timestamp.tv_sec,
					(exp_timestamp.tv_nsec / NSEC_PER_MSEC),
					(tm.tm_year + 1900), (tm.tm_mon + 1), tm.tm_mday,
					tm.tm_hour, tm.tm_min, tm.tm_sec,
					(unsigned long)(tv.tv_usec / USEC_PER_MSEC),
					(larger != NULL) ? larger->pid : 0,
					(larger != NULL) ? larger->comm : "");

		}
		else
		{
			sprintf(aee_word, "check %s: binder buffer exhaust ",
					len_r ? rec_name :
					((target_proc->tsk != NULL) ? target_proc->tsk->comm : ""));
			snprintf(aee_msg, 512, "BINDER_BUF_DEBUG\n"
					"binder:check=%d,success=%d,,call=%s,,from=%d,tid=%d,name=%s,"
					"to=%d,name=%s,,,size=%d,,,,"
					",start=%lu.%03ld,android="
					"%d-%02d-%02d %02d:%02d:%02d.%03lu\n"
					"%d small trans pending, check receiver %d(%s)!\n"
					"check kernel log for more info\n",
					1, 0, is_async ? "async" : "sync",
					binder_check_buf_pid, binder_check_buf_tid,
					len_s ? sender_name : ((sender != NULL) ? sender->comm : ""),
					target_proc->pid,
					len_r ? rec_name :
					((target_proc->tsk != NULL) ? target_proc->tsk->comm : ""),
					size,
					(unsigned long)exp_timestamp.tv_sec,
					(exp_timestamp.tv_nsec / NSEC_PER_MSEC),
					(tm.tm_year + 1900), (tm.tm_mon + 1), tm.tm_mday,
					tm.tm_hour, tm.tm_min, tm.tm_sec,
					(unsigned long)(tv.tv_usec / USEC_PER_MSEC),
					i, target_proc->pid,
					target_proc->tsk ? target_proc->tsk->comm : "");
			if (binder_check_is_javap(sender) > 0)
			{
				db_flag = db_flag | DB_OPT_SWT_JBT_TRACES;
				binder_call_jbt(binder_check_buf_pid);
			}
			else
			{
				db_flag = db_flag | DB_OPT_SWT_JBT_TRACES;
				binder_call_nbt(binder_check_buf_pid, "1");
				printk(KERN_INFO "binder:sender pid %d (%s) is not javap, pls check SYS_BINDER_BACKTRACE\n",
						binder_check_buf_pid,
						(sender != NULL) ? sender->comm : "");
			}
		}

	}

	binder_check_buf_pid = -1;
	binder_check_buf_tid = -1;
	if (binder_check_is_javap(target_proc->tsk) > 0)
	{
		db_flag = db_flag | DB_OPT_SWT_JBT_TRACES;
		binder_call_jbt(target_proc->pid);
	}
	else
	{
		db_flag = db_flag | DB_OPT_SWT_JBT_TRACES;
		binder_call_nbt(target_proc->pid, "2");
		printk(KERN_INFO "binder:target_proc %p pid %d (%s) is not javap, pls check SYS_BINDER_BACKTRACE!\n",
				target_proc, target_proc->pid,
				(target_proc->tsk != NULL) ? target_proc->tsk->comm : "");
	}
	if (db_flag & DB_OPT_SWT_JBT_TRACES)
	{
		binder_unlock(__func__);
		msleep(500);
		binder_lock(__func__);
	}
	aee_kernel_warning_api(__FILE__, __LINE__, db_flag, &aee_word[0],&aee_msg[0]);

}
#endif

/*
 * copied from get_unused_fd_flags
 */
int task_get_unused_fd_flags(struct binder_proc *proc, int flags)
{
	struct files_struct *files = proc->files;
	int fd, error;
	struct fdtable *fdt;
	unsigned long rlim_cur;
	unsigned long irqs;

	if (files == NULL)
		return -ESRCH;

	error = -EMFILE;
	spin_lock(&files->file_lock);

repeat:
	fdt = files_fdtable(files);
	fd = find_next_zero_bit(fdt->open_fds, fdt->max_fds, files->next_fd);

	/*
	 * N.B. For clone tasks sharing a files structure, this test
	 * will limit the total number of files that can be opened.
	 */
	rlim_cur = 0;
	if (lock_task_sighand(proc->tsk, &irqs)) {
		rlim_cur = proc->tsk->signal->rlim[RLIMIT_NOFILE].rlim_cur;
		unlock_task_sighand(proc->tsk, &irqs);
	}
	if (fd >= rlim_cur)
		goto out;

	/* Do we need to expand the fd array or fd set?  */
	error = expand_files(files, fd);
	if (error < 0)
		goto out;

	if (error) {
		/*
		 * If we needed to expand the fs array we
		 * might have blocked - try again.
		 */
		error = -EMFILE;
		goto repeat;
	}

	__set_open_fd(fd, fdt);
	if (flags & O_CLOEXEC)
		__set_close_on_exec(fd, fdt);
	else
		__clear_close_on_exec(fd, fdt);
	files->next_fd = fd + 1;
#if 1
	/* Sanity check */
	if (fdt->fd[fd] != NULL) {
		printk(KERN_WARNING "get_unused_fd: slot %d not NULL!\n", fd);
		fdt->fd[fd] = NULL;
	}
#endif
	error = fd;

out:
	spin_unlock(&files->file_lock);
	return error;
}

/*
 * copied from fd_install
 */
static void task_fd_install(
		struct binder_proc *proc, unsigned int fd, struct file *file)
{
	struct files_struct *files = proc->files;
	struct fdtable *fdt;

	if (files == NULL)
		return;

	spin_lock(&files->file_lock);
	fdt = files_fdtable(files);
	BUG_ON(fdt->fd[fd] != NULL);
	rcu_assign_pointer(fdt->fd[fd], file);
	spin_unlock(&files->file_lock);
}

/*
 * copied from __put_unused_fd in open.c
 */
static void __put_unused_fd(struct files_struct *files, unsigned int fd)
{
	struct fdtable *fdt = files_fdtable(files);
	__clear_open_fd(fd, fdt);
	if (fd < files->next_fd)
		files->next_fd = fd;
}

/*
 * copied from sys_close
 */
static long task_close_fd(struct binder_proc *proc, unsigned int fd)
{
	struct file *filp;
	struct files_struct *files = proc->files;
	struct fdtable *fdt;
	int retval;

	if (files == NULL)
		return -ESRCH;

	spin_lock(&files->file_lock);
	fdt = files_fdtable(files);
	if (fd >= fdt->max_fds)
		goto out_unlock;
	filp = fdt->fd[fd];
	if (!filp)
		goto out_unlock;
	rcu_assign_pointer(fdt->fd[fd], NULL);
	__clear_close_on_exec(fd, fdt);
	__put_unused_fd(files, fd);
	spin_unlock(&files->file_lock);
	retval = filp_close(filp, files);

	/* can't restart close syscall because file table entry was cleared */
	if (unlikely(retval == -ERESTARTSYS ||
		     retval == -ERESTARTNOINTR ||
		     retval == -ERESTARTNOHAND ||
		     retval == -ERESTART_RESTARTBLOCK))
		retval = -EINTR;

	return retval;

out_unlock:
	spin_unlock(&files->file_lock);
	return -EBADF;
}

static inline void binder_lock(const char *tag)
{
	trace_binder_lock(tag);
	mutex_lock(&binder_main_lock);
	trace_binder_locked(tag);
}

static inline void binder_unlock(const char *tag)
{
	trace_binder_unlock(tag);
	mutex_unlock(&binder_main_lock);
}

static void binder_set_nice(long nice)
{
	long min_nice;
	if (can_nice(current, nice)) {
#ifdef CONFIG_MT_PRIO_TRACER
		set_user_nice_binder(current, nice);
#else
		set_user_nice(current, nice);
#endif
		return;
	}
	min_nice = 20 - current->signal->rlim[RLIMIT_NICE].rlim_cur;
	binder_debug(BINDER_DEBUG_PRIORITY_CAP,
		     "binder: %d: nice value %ld not allowed use "
		     "%ld instead\n", current->pid, nice, min_nice);
#ifdef CONFIG_MT_PRIO_TRACER
	set_user_nice_binder(current, min_nice);
#else
	set_user_nice(current, min_nice);
#endif
	if (min_nice < 20)
		return;
	binder_user_error("binder: %d RLIMIT_NICE not set\n", current->pid);
}

static size_t binder_buffer_size(struct binder_proc *proc,
				 struct binder_buffer *buffer)
{
	if (list_is_last(&buffer->entry, &proc->buffers))
		return proc->buffer + proc->buffer_size - (void *)buffer->data;
	else
		return (size_t)list_entry(buffer->entry.next,
			struct binder_buffer, entry) - (size_t)buffer->data;
}

static void binder_insert_free_buffer(struct binder_proc *proc,
				      struct binder_buffer *new_buffer)
{
	struct rb_node **p = &proc->free_buffers.rb_node;
	struct rb_node *parent = NULL;
	struct binder_buffer *buffer;
	size_t buffer_size;
	size_t new_buffer_size;

	BUG_ON(!new_buffer->free);

	new_buffer_size = binder_buffer_size(proc, new_buffer);

	binder_debug(BINDER_DEBUG_BUFFER_ALLOC,
		     "binder: %d: add free buffer, size %zd, "
		     "at %p\n", proc->pid, new_buffer_size, new_buffer);

	while (*p) {
		parent = *p;
		buffer = rb_entry(parent, struct binder_buffer, rb_node);
		BUG_ON(!buffer->free);

		buffer_size = binder_buffer_size(proc, buffer);

		if (new_buffer_size < buffer_size)
			p = &parent->rb_left;
		else
			p = &parent->rb_right;
	}
	rb_link_node(&new_buffer->rb_node, parent, p);
	rb_insert_color(&new_buffer->rb_node, &proc->free_buffers);
}

static void binder_insert_allocated_buffer(struct binder_proc *proc,
					   struct binder_buffer *new_buffer)
{
	struct rb_node **p = &proc->allocated_buffers.rb_node;
	struct rb_node *parent = NULL;
	struct binder_buffer *buffer;

	BUG_ON(new_buffer->free);

	while (*p) {
		parent = *p;
		buffer = rb_entry(parent, struct binder_buffer, rb_node);
		BUG_ON(buffer->free);

		if (new_buffer < buffer)
			p = &parent->rb_left;
		else if (new_buffer > buffer)
			p = &parent->rb_right;
		else
			BUG();
	}
	rb_link_node(&new_buffer->rb_node, parent, p);
	rb_insert_color(&new_buffer->rb_node, &proc->allocated_buffers);
}

static struct binder_buffer *binder_buffer_lookup(struct binder_proc *proc,
						  void __user *user_ptr)
{
	struct rb_node *n = proc->allocated_buffers.rb_node;
	struct binder_buffer *buffer;
	struct binder_buffer *kern_ptr;

	kern_ptr = user_ptr - proc->user_buffer_offset
		- offsetof(struct binder_buffer, data);

	while (n) {
		buffer = rb_entry(n, struct binder_buffer, rb_node);
		BUG_ON(buffer->free);

		if (kern_ptr < buffer)
			n = n->rb_left;
		else if (kern_ptr > buffer)
			n = n->rb_right;
		else
			return buffer;
	}
	return NULL;
}

static int binder_update_page_range(struct binder_proc *proc, int allocate,
				    void *start, void *end,
				    struct vm_area_struct *vma)
{
	void *page_addr;
	unsigned long user_page_addr;
	struct vm_struct tmp_area;
	struct page **page;
	struct mm_struct *mm;

	binder_debug(BINDER_DEBUG_BUFFER_ALLOC,
		     "binder: %d: %s pages %p-%p\n", proc->pid,
		     allocate ? "allocate" : "free", start, end);

	if (end <= start)
		return 0;

	trace_binder_update_page_range(proc, allocate, start, end);

	if (vma)
		mm = NULL;
	else
		mm = get_task_mm(proc->tsk);

	if (mm) {
		down_write(&mm->mmap_sem);
		vma = proc->vma;
		if (vma && mm != proc->vma_vm_mm) {
			pr_err("binder: %d: vma mm and task mm mismatch\n",
				proc->pid);
			vma = NULL;
		}
	}

	if (allocate == 0)
		goto free_range;

	if (vma == NULL) {
		printk(KERN_ERR "binder: %d: binder_alloc_buf failed to "
		       "map pages in userspace, no vma\n", proc->pid);
		goto err_no_vma;
	}

	for (page_addr = start; page_addr < end; page_addr += PAGE_SIZE) {
		int ret;
		struct page **page_array_ptr;
		page = &proc->pages[(page_addr - proc->buffer) / PAGE_SIZE];

		BUG_ON(*page);
		*page = alloc_page(GFP_KERNEL | __GFP_HIGHMEM | __GFP_ZERO);
		if (*page == NULL) {
			printk(KERN_ERR "binder: %d: binder_alloc_buf failed "
					"for page at %p\n", proc->pid, page_addr);
			goto err_alloc_page_failed;
		}
#ifdef MTK_BINDER_PAGE_USED_RECORD
		binder_page_used++;
		proc->page_used++;
		if(binder_page_used > binder_page_used_peak)
			binder_page_used_peak = binder_page_used;
		if (proc->page_used > proc->page_used_peak)
			proc->page_used_peak = proc->page_used;
#endif

		tmp_area.addr = page_addr;
		tmp_area.size = PAGE_SIZE + PAGE_SIZE /* guard page? */;
		page_array_ptr = page;
		ret = map_vm_area(&tmp_area, PAGE_KERNEL, &page_array_ptr);
		if (ret) {
			printk(KERN_ERR "binder: %d: binder_alloc_buf failed "
			       "to map page at %p in kernel\n",
			       proc->pid, page_addr);
			goto err_map_kernel_failed;
		}
		user_page_addr =
			(uintptr_t)page_addr + proc->user_buffer_offset;
		ret = vm_insert_page(vma, user_page_addr, page[0]);
		if (ret) {
			printk(KERN_ERR "binder: %d: binder_alloc_buf failed "
			       "to map page at %lx in userspace\n",
			       proc->pid, user_page_addr);
			goto err_vm_insert_page_failed;
		}
		/* vm_insert_page does not seem to increment the refcount */
	}
	if (mm) {
		up_write(&mm->mmap_sem);
		mmput(mm);
	}
	return 0;

free_range:
	for (page_addr = end - PAGE_SIZE; page_addr >= start;
	     page_addr -= PAGE_SIZE) {
		page = &proc->pages[(page_addr - proc->buffer) / PAGE_SIZE];
		if (vma)
			zap_page_range(vma, (uintptr_t)page_addr +
				proc->user_buffer_offset, PAGE_SIZE, NULL);
err_vm_insert_page_failed:
		unmap_kernel_range((unsigned long)page_addr, PAGE_SIZE);
err_map_kernel_failed:
		__free_page(*page);
		*page = NULL;
#ifdef MTK_BINDER_PAGE_USED_RECORD
		if(binder_page_used > 0)
			binder_page_used--;
		if (proc->page_used > 0)
			proc->page_used--;
#endif
err_alloc_page_failed:
		;
	}
err_no_vma:
	if (mm) {
		up_write(&mm->mmap_sem);
		mmput(mm);
	}
	return -ENOMEM;
}

static struct binder_buffer *binder_alloc_buf(struct binder_proc *proc,
					      size_t data_size,
					      size_t offsets_size, int is_async)
{
	struct rb_node *n = proc->free_buffers.rb_node;
	struct binder_buffer *buffer;
	size_t buffer_size;
	struct rb_node *best_fit = NULL;
	void *has_page_addr;
	void *end_page_addr;
	size_t size;
#ifdef MTK_BINDER_DEBUG	
	size_t proc_max_size;
#endif
	if (proc->vma == NULL) {
		printk(KERN_ERR "binder: %d: binder_alloc_buf, no vma\n",
		       proc->pid);
		return NULL;
	}

	size = ALIGN(data_size, sizeof(void *)) +
		ALIGN(offsets_size, sizeof(void *));

	if (size < data_size || size < offsets_size) {
		binder_user_error("binder: %d: got transaction with invalid "
			"size %zd-%zd\n", proc->pid, data_size, offsets_size);
		return NULL;
	}

#ifdef MTK_BINDER_DEBUG    
	proc_max_size = (is_async ? (proc->buffer_size/2) : proc->buffer_size);

	if(proc_max_size < size + sizeof(struct binder_buffer)){
		binder_user_error("binder: %d: got transaction with too large size "
				"%s alloc size %d-%d allowed size %d\n", proc->pid, 
				is_async ? "async" : "sync", data_size, offsets_size,
				(proc_max_size - sizeof(struct binder_buffer)));
		return NULL;
	}
#endif

	if (is_async &&
	    proc->free_async_space < size + sizeof(struct binder_buffer)) {
#ifdef MTK_BINDER_DEBUG
		printk(KERN_ERR "binder: %d: binder_alloc_buf size %zd "
				"failed, no async space left (%zd)\n", 
				proc->pid, size, proc->free_async_space);
#else		
		binder_debug(BINDER_DEBUG_BUFFER_ALLOC,
			     "%d: binder_alloc_buf size %zd failed, no async space left\n",
			      proc->pid, size);
#endif
#ifdef BINDER_MONITOR	
		binder_check_buf(proc, size, 1);
#endif
		return NULL;
	}

	while (n) {
		buffer = rb_entry(n, struct binder_buffer, rb_node);
		BUG_ON(!buffer->free);
		buffer_size = binder_buffer_size(proc, buffer);

		if (size < buffer_size) {
			best_fit = n;
			n = n->rb_left;
		} else if (size > buffer_size)
			n = n->rb_right;
		else {
			best_fit = n;
			break;
		}
	}
#ifdef BINDER_MONITOR
	if (log_disable & BINDER_BUF_WARN)
	{
		if (size > 64)
		{
			printk(KERN_ERR "binder: %d: binder_alloc_buf size %zd failed, UT auto triggerd!\n",
					proc->pid, size);
			binder_check_buf(proc, size, 0);
		}
	}
#endif
	if (best_fit == NULL) {
		printk(KERN_ERR "binder: %d: binder_alloc_buf size %zd failed, "
		       "no address space\n", proc->pid, size);
#ifdef BINDER_MONITOR	
		binder_check_buf(proc, size, 0);
#endif
		return NULL;
	}
	if (n == NULL) {
		buffer = rb_entry(best_fit, struct binder_buffer, rb_node);
		buffer_size = binder_buffer_size(proc, buffer);
	}

	binder_debug(BINDER_DEBUG_BUFFER_ALLOC,
		     "binder: %d: binder_alloc_buf size %zd got buff"
		     "er %p size %zd\n", proc->pid, size, buffer, buffer_size);

	has_page_addr =
		(void *)(((uintptr_t)buffer->data + buffer_size) & PAGE_MASK);
	if (n == NULL) {
		if (size + sizeof(struct binder_buffer) + 4 >= buffer_size)
			buffer_size = size; /* no room for other buffers */
		else
			buffer_size = size + sizeof(struct binder_buffer);
	}
	end_page_addr =
		(void *)PAGE_ALIGN((uintptr_t)buffer->data + buffer_size);
	if (end_page_addr > has_page_addr)
		end_page_addr = has_page_addr;
	if (binder_update_page_range(proc, 1,
	    (void *)PAGE_ALIGN((uintptr_t)buffer->data), end_page_addr, NULL))
		return NULL;

	rb_erase(best_fit, &proc->free_buffers);
	buffer->free = 0;
	binder_insert_allocated_buffer(proc, buffer);
	if (buffer_size != size) {
		struct binder_buffer *new_buffer = (void *)buffer->data + size;
		list_add(&new_buffer->entry, &buffer->entry);
		new_buffer->free = 1;
		binder_insert_free_buffer(proc, new_buffer);
	}
	binder_debug(BINDER_DEBUG_BUFFER_ALLOC,
		     "binder: %d: binder_alloc_buf size %zd got "
		     "%p\n", proc->pid, size, buffer);
	buffer->data_size = data_size;
	buffer->offsets_size = offsets_size;
	buffer->async_transaction = is_async;
	if (is_async) {
		proc->free_async_space -= size + sizeof(struct binder_buffer);
		binder_debug(BINDER_DEBUG_BUFFER_ALLOC_ASYNC,
			     "binder: %d: binder_alloc_buf size %zd "
			     "async free %zd\n", proc->pid, size,
			     proc->free_async_space);
	}

	return buffer;
}

static void *buffer_start_page(struct binder_buffer *buffer)
{
	return (void *)((uintptr_t)buffer & PAGE_MASK);
}

static void *buffer_end_page(struct binder_buffer *buffer)
{
	return (void *)(((uintptr_t)(buffer + 1) - 1) & PAGE_MASK);
}

static void binder_delete_free_buffer(struct binder_proc *proc,
				      struct binder_buffer *buffer)
{
	struct binder_buffer *prev, *next = NULL;
	int free_page_end = 1;
	int free_page_start = 1;

	BUG_ON(proc->buffers.next == &buffer->entry);
	prev = list_entry(buffer->entry.prev, struct binder_buffer, entry);
	BUG_ON(!prev->free);
	if (buffer_end_page(prev) == buffer_start_page(buffer)) {
		free_page_start = 0;
		if (buffer_end_page(prev) == buffer_end_page(buffer))
			free_page_end = 0;
		binder_debug(BINDER_DEBUG_BUFFER_ALLOC,
			     "binder: %d: merge free, buffer %p "
			     "share page with %p\n", proc->pid, buffer, prev);
	}

	if (!list_is_last(&buffer->entry, &proc->buffers)) {
		next = list_entry(buffer->entry.next,
				  struct binder_buffer, entry);
		if (buffer_start_page(next) == buffer_end_page(buffer)) {
			free_page_end = 0;
			if (buffer_start_page(next) ==
			    buffer_start_page(buffer))
				free_page_start = 0;
			binder_debug(BINDER_DEBUG_BUFFER_ALLOC,
				     "binder: %d: merge free, buffer"
				     " %p share page with %p\n", proc->pid,
				     buffer, prev);
		}
	}
	list_del(&buffer->entry);
	if (free_page_start || free_page_end) {
		binder_debug(BINDER_DEBUG_BUFFER_ALLOC,
			     "binder: %d: merge free, buffer %p do "
			     "not share page%s%s with with %p or %p\n",
			     proc->pid, buffer, free_page_start ? "" : " end",
			     free_page_end ? "" : " start", prev, next);
		binder_update_page_range(proc, 0, free_page_start ?
			buffer_start_page(buffer) : buffer_end_page(buffer),
			(free_page_end ? buffer_end_page(buffer) :
			buffer_start_page(buffer)) + PAGE_SIZE, NULL);
	}
}

static void binder_free_buf(struct binder_proc *proc,
			    struct binder_buffer *buffer)
{
	size_t size, buffer_size;

	buffer_size = binder_buffer_size(proc, buffer);

	size = ALIGN(buffer->data_size, sizeof(void *)) +
		ALIGN(buffer->offsets_size, sizeof(void *));

	binder_debug(BINDER_DEBUG_BUFFER_ALLOC,
		     "binder: %d: binder_free_buf %p size %zd buffer"
		     "_size %zd\n", proc->pid, buffer, size, buffer_size);

	BUG_ON(buffer->free);
	BUG_ON(size > buffer_size);
	BUG_ON(buffer->transaction != NULL);
	BUG_ON((void *)buffer < proc->buffer);
	BUG_ON((void *)buffer > proc->buffer + proc->buffer_size);
#ifdef BINDER_MONITOR
	buffer->log_entry = NULL;
#endif

	if (buffer->async_transaction) {
		proc->free_async_space += size + sizeof(struct binder_buffer);

		binder_debug(BINDER_DEBUG_BUFFER_ALLOC_ASYNC,
			     "binder: %d: binder_free_buf size %zd "
			     "async free %zd\n", proc->pid, size,
			     proc->free_async_space);
	}

	binder_update_page_range(proc, 0,
		(void *)PAGE_ALIGN((uintptr_t)buffer->data),
		(void *)(((uintptr_t)buffer->data + buffer_size) & PAGE_MASK),
		NULL);
	rb_erase(&buffer->rb_node, &proc->allocated_buffers);
	buffer->free = 1;
	if (!list_is_last(&buffer->entry, &proc->buffers)) {
		struct binder_buffer *next = list_entry(buffer->entry.next,
						struct binder_buffer, entry);
		if (next->free) {
			rb_erase(&next->rb_node, &proc->free_buffers);
			binder_delete_free_buffer(proc, next);
		}
	}
	if (proc->buffers.next != &buffer->entry) {
		struct binder_buffer *prev = list_entry(buffer->entry.prev,
						struct binder_buffer, entry);
		if (prev->free) {
			binder_delete_free_buffer(proc, buffer);
			rb_erase(&prev->rb_node, &proc->free_buffers);
			buffer = prev;
		}
	}
	binder_insert_free_buffer(proc, buffer);
}

static struct binder_node *binder_get_node(struct binder_proc *proc,
					   void __user *ptr)
{
	struct rb_node *n = proc->nodes.rb_node;
	struct binder_node *node;

	while (n) {
		node = rb_entry(n, struct binder_node, rb_node);

		if (ptr < node->ptr)
			n = n->rb_left;
		else if (ptr > node->ptr)
			n = n->rb_right;
		else
			return node;
	}
	return NULL;
}

static struct binder_node *binder_new_node(struct binder_proc *proc,
					   void __user *ptr,
					   void __user *cookie)
{
	struct rb_node **p = &proc->nodes.rb_node;
	struct rb_node *parent = NULL;
	struct binder_node *node;

	while (*p) {
		parent = *p;
		node = rb_entry(parent, struct binder_node, rb_node);

		if (ptr < node->ptr)
			p = &(*p)->rb_left;
		else if (ptr > node->ptr)
			p = &(*p)->rb_right;
		else
			return NULL;
	}

	node = kzalloc(sizeof(*node), GFP_KERNEL);
	if (node == NULL)
		return NULL;
	binder_stats_created(BINDER_STAT_NODE);
	rb_link_node(&node->rb_node, parent, p);
	rb_insert_color(&node->rb_node, &proc->nodes);
	node->debug_id = ++binder_last_id;
	node->proc = proc;
	node->ptr = ptr;
	node->cookie = cookie;
	node->work.type = BINDER_WORK_NODE;
	INIT_LIST_HEAD(&node->work.entry);
	INIT_LIST_HEAD(&node->async_todo);
	binder_debug(BINDER_DEBUG_INTERNAL_REFS,
		     "binder: %d:%d node %d u%p c%p created\n",
		     proc->pid, current->pid, node->debug_id,
		     node->ptr, node->cookie);
	return node;
}

static int binder_inc_node(struct binder_node *node, int strong, int internal,
			   struct list_head *target_list)
{
	if (strong) {
		if (internal) {
			if (target_list == NULL &&
			    node->internal_strong_refs == 0 &&
			    !(node == binder_context_mgr_node &&
			    node->has_strong_ref)) {
				printk(KERN_ERR "binder: invalid inc strong "
					"node for %d\n", node->debug_id);
				return -EINVAL;
			}
			node->internal_strong_refs++;
		} else
			node->local_strong_refs++;
		if (!node->has_strong_ref && target_list) {
			list_del_init(&node->work.entry);
			list_add_tail(&node->work.entry, target_list);
		}
	} else {
		if (!internal)
			node->local_weak_refs++;
		if (!node->has_weak_ref && list_empty(&node->work.entry)) {
			if (target_list == NULL) {
				printk(KERN_ERR "binder: invalid inc weak node "
					"for %d\n", node->debug_id);
				return -EINVAL;
			}
			list_add_tail(&node->work.entry, target_list);
		}
	}
	return 0;
}

static int binder_dec_node(struct binder_node *node, int strong, int internal)
{
	if (strong) {
		if (internal)
			node->internal_strong_refs--;
		else
			node->local_strong_refs--;
		if (node->local_strong_refs || node->internal_strong_refs)
			return 0;
	} else {
		if (!internal)
			node->local_weak_refs--;
		if (node->local_weak_refs || !hlist_empty(&node->refs))
			return 0;
	}
	if (node->proc && (node->has_strong_ref || node->has_weak_ref)) {
		if (list_empty(&node->work.entry)) {
			list_add_tail(&node->work.entry, &node->proc->todo);
			wake_up_interruptible(&node->proc->wait);
		}
	} else {
		if (hlist_empty(&node->refs) && !node->local_strong_refs &&
		    !node->local_weak_refs) {
			list_del_init(&node->work.entry);
			if (node->proc) {
				rb_erase(&node->rb_node, &node->proc->nodes);
				binder_debug(BINDER_DEBUG_INTERNAL_REFS,
					     "binder: refless node %d deleted\n",
					     node->debug_id);
			} else {
				hlist_del(&node->dead_node);
				binder_debug(BINDER_DEBUG_INTERNAL_REFS,
					     "binder: dead node %d deleted\n",
					     node->debug_id);
			}
			kfree(node);
			binder_stats_deleted(BINDER_STAT_NODE);
		}
	}

	return 0;
}


static struct binder_ref *binder_get_ref(struct binder_proc *proc,
					 uint32_t desc)
{
	struct rb_node *n = proc->refs_by_desc.rb_node;
	struct binder_ref *ref;

	while (n) {
		ref = rb_entry(n, struct binder_ref, rb_node_desc);

		if (desc < ref->desc)
			n = n->rb_left;
		else if (desc > ref->desc)
			n = n->rb_right;
		else
			return ref;
	}
	return NULL;
}

static struct binder_ref *binder_get_ref_for_node(struct binder_proc *proc,
						  struct binder_node *node)
{
	struct rb_node *n;
	struct rb_node **p = &proc->refs_by_node.rb_node;
	struct rb_node *parent = NULL;
	struct binder_ref *ref, *new_ref;

	while (*p) {
		parent = *p;
		ref = rb_entry(parent, struct binder_ref, rb_node_node);

		if (node < ref->node)
			p = &(*p)->rb_left;
		else if (node > ref->node)
			p = &(*p)->rb_right;
		else
			return ref;
	}
	new_ref = kzalloc(sizeof(*ref), GFP_KERNEL);
	if (new_ref == NULL)
		return NULL;
	binder_stats_created(BINDER_STAT_REF);
	new_ref->debug_id = ++binder_last_id;
	new_ref->proc = proc;
	new_ref->node = node;
	rb_link_node(&new_ref->rb_node_node, parent, p);
	rb_insert_color(&new_ref->rb_node_node, &proc->refs_by_node);

	new_ref->desc = (node == binder_context_mgr_node) ? 0 : 1;
	for (n = rb_first(&proc->refs_by_desc); n != NULL; n = rb_next(n)) {
		ref = rb_entry(n, struct binder_ref, rb_node_desc);
		if (ref->desc > new_ref->desc)
			break;
		new_ref->desc = ref->desc + 1;
	}

	p = &proc->refs_by_desc.rb_node;
	while (*p) {
		parent = *p;
		ref = rb_entry(parent, struct binder_ref, rb_node_desc);

		if (new_ref->desc < ref->desc)
			p = &(*p)->rb_left;
		else if (new_ref->desc > ref->desc)
			p = &(*p)->rb_right;
		else
			BUG();
	}
	rb_link_node(&new_ref->rb_node_desc, parent, p);
	rb_insert_color(&new_ref->rb_node_desc, &proc->refs_by_desc);
	if (node) {
		hlist_add_head(&new_ref->node_entry, &node->refs);

		binder_debug(BINDER_DEBUG_INTERNAL_REFS,
			     "binder: %d new ref %d desc %d for "
			     "node %d\n", proc->pid, new_ref->debug_id,
			     new_ref->desc, node->debug_id);
	} else {
		binder_debug(BINDER_DEBUG_INTERNAL_REFS,
			     "binder: %d new ref %d desc %d for "
			     "dead node\n", proc->pid, new_ref->debug_id,
			      new_ref->desc);
	}
	return new_ref;
}

static void binder_delete_ref(struct binder_ref *ref)
{
	binder_debug(BINDER_DEBUG_INTERNAL_REFS,
		     "binder: %d delete ref %d desc %d for "
		     "node %d\n", ref->proc->pid, ref->debug_id,
		     ref->desc, ref->node->debug_id);

	rb_erase(&ref->rb_node_desc, &ref->proc->refs_by_desc);
	rb_erase(&ref->rb_node_node, &ref->proc->refs_by_node);
	if (ref->strong)
		binder_dec_node(ref->node, 1, 1);
	hlist_del(&ref->node_entry);
	binder_dec_node(ref->node, 0, 1);
	if (ref->death) {
		binder_debug(BINDER_DEBUG_DEAD_BINDER,
			     "binder: %d delete ref %d desc %d "
			     "has death notification\n", ref->proc->pid,
			     ref->debug_id, ref->desc);
		list_del(&ref->death->work.entry);
		kfree(ref->death);
		binder_stats_deleted(BINDER_STAT_DEATH);
	}
	kfree(ref);
	binder_stats_deleted(BINDER_STAT_REF);
}

static int binder_inc_ref(struct binder_ref *ref, int strong,
			  struct list_head *target_list)
{
	int ret;
	if (strong) {
		if (ref->strong == 0) {
			ret = binder_inc_node(ref->node, 1, 1, target_list);
			if (ret)
				return ret;
		}
		ref->strong++;
	} else {
		if (ref->weak == 0) {
			ret = binder_inc_node(ref->node, 0, 1, target_list);
			if (ret)
				return ret;
		}
		ref->weak++;
	}
	return 0;
}


static int binder_dec_ref(struct binder_ref *ref, int strong)
{
	if (strong) {
		if (ref->strong == 0) {
			binder_user_error("binder: %d invalid dec strong, "
					  "ref %d desc %d s %d w %d\n",
					  ref->proc->pid, ref->debug_id,
					  ref->desc, ref->strong, ref->weak);
			return -EINVAL;
		}
		ref->strong--;
		if (ref->strong == 0) {
			int ret;
			ret = binder_dec_node(ref->node, strong, 1);
			if (ret)
				return ret;
		}
	} else {
		if (ref->weak == 0) {
			binder_user_error("binder: %d invalid dec weak, "
					  "ref %d desc %d s %d w %d\n",
					  ref->proc->pid, ref->debug_id,
					  ref->desc, ref->strong, ref->weak);
			return -EINVAL;
		}
		ref->weak--;
	}
	if (ref->strong == 0 && ref->weak == 0)
		binder_delete_ref(ref);
	return 0;
}

static void binder_pop_transaction(struct binder_thread *target_thread,
				   struct binder_transaction *t)
{
	if (target_thread) {
		BUG_ON(target_thread->transaction_stack != t);
		BUG_ON(target_thread->transaction_stack->from != target_thread);
		target_thread->transaction_stack =
			target_thread->transaction_stack->from_parent;
		t->from = NULL;
	}
	t->need_reply = 0;
	if (t->buffer)
		t->buffer->transaction = NULL;
#ifdef BINDER_MONITOR
	binder_cancel_bwdog(t);
#endif
	kfree(t);
	binder_stats_deleted(BINDER_STAT_TRANSACTION);
}

static void binder_send_failed_reply(struct binder_transaction *t,
				     uint32_t error_code)
{
	struct binder_thread *target_thread;
	BUG_ON(t->flags & TF_ONE_WAY);
	while (1) {
		target_thread = t->from;
		if (target_thread) {
			if (target_thread->return_error != BR_OK &&
			   target_thread->return_error2 == BR_OK) {
				target_thread->return_error2 =
					target_thread->return_error;
				target_thread->return_error = BR_OK;
			}
			if (target_thread->return_error == BR_OK) {
				binder_debug(BINDER_DEBUG_FAILED_TRANSACTION,
					     "binder: send failed reply for "
					     "transaction %d to %d:%d\n",
					      t->debug_id, target_thread->proc->pid,
					      target_thread->pid);

				binder_pop_transaction(target_thread, t);
				target_thread->return_error = error_code;
				wake_up_interruptible(&target_thread->wait);
			} else {
				printk(KERN_ERR "binder: reply failed, target "
					"thread, %d:%d, has error code %d "
					"already\n", target_thread->proc->pid,
					target_thread->pid,
					target_thread->return_error);
			}
			return;
		} else {
			struct binder_transaction *next = t->from_parent;

			binder_debug(BINDER_DEBUG_FAILED_TRANSACTION,
				     "binder: send failed reply "
				     "for transaction %d, target dead\n",
				     t->debug_id);

			binder_pop_transaction(target_thread, t);
			if (next == NULL) {
				binder_debug(BINDER_DEBUG_DEAD_BINDER,
					     "binder: reply failed,"
					     " no target thread at root\n");
				return;
			}
			t = next;
			binder_debug(BINDER_DEBUG_DEAD_BINDER,
				     "binder: reply failed, no target "
				     "thread -- retry %d\n", t->debug_id);
		}
	}
}

static void binder_transaction_buffer_release(struct binder_proc *proc,
					      struct binder_buffer *buffer,
					      size_t *failed_at)
{
	size_t *offp, *off_end;
	int debug_id = buffer->debug_id;

	binder_debug(BINDER_DEBUG_TRANSACTION,
		     "binder: %d buffer release %d, size %zd-%zd, failed at %p\n",
		     proc->pid, buffer->debug_id,
		     buffer->data_size, buffer->offsets_size, failed_at);

	if (buffer->target_node)
		binder_dec_node(buffer->target_node, 1, 0);

	offp = (size_t *)(buffer->data + ALIGN(buffer->data_size, sizeof(void *)));
	if (failed_at)
		off_end = failed_at;
	else
		off_end = (void *)offp + buffer->offsets_size;
	for (; offp < off_end; offp++) {
		struct flat_binder_object *fp;
		if (*offp > buffer->data_size - sizeof(*fp) ||
		    buffer->data_size < sizeof(*fp) ||
		    !IS_ALIGNED(*offp, sizeof(void *))) {
			printk(KERN_ERR "binder: transaction release %d bad"
					"offset %zd, size %zd\n", debug_id,
					*offp, buffer->data_size);
			continue;
		}
		fp = (struct flat_binder_object *)(buffer->data + *offp);
		switch (fp->type) {
		case BINDER_TYPE_BINDER:
		case BINDER_TYPE_WEAK_BINDER: {
			struct binder_node *node = binder_get_node(proc, fp->binder);
			if (node == NULL) {
				printk(KERN_ERR "binder: transaction release %d"
				       " bad node %p\n", debug_id, fp->binder);
				break;
			}
			binder_debug(BINDER_DEBUG_TRANSACTION,
				     "        node %d u%p\n",
				     node->debug_id, node->ptr);
			binder_dec_node(node, fp->type == BINDER_TYPE_BINDER, 0);
		} break;
		case BINDER_TYPE_HANDLE:
		case BINDER_TYPE_WEAK_HANDLE: {
			struct binder_ref *ref = binder_get_ref(proc, fp->handle);
			if (ref == NULL) {
				printk(KERN_ERR "binder: transaction release %d"
				       " bad handle %ld\n", debug_id,
				       fp->handle);
				break;
			}
			binder_debug(BINDER_DEBUG_TRANSACTION,
				     "        ref %d desc %d (node %d)\n",
				     ref->debug_id, ref->desc, ref->node->debug_id);
			binder_dec_ref(ref, fp->type == BINDER_TYPE_HANDLE);
		} break;

		case BINDER_TYPE_FD:
			binder_debug(BINDER_DEBUG_TRANSACTION,
				     "        fd %ld\n", fp->handle);
			if (failed_at)
				task_close_fd(proc, fp->handle);
			break;

		default:
			printk(KERN_ERR "binder: transaction release %d bad "
			       "object type %lx\n", debug_id, fp->type);
			break;
		}
	}
}

#ifdef RT_PRIO_INHERIT
static void mt_sched_setscheduler_nocheck(struct task_struct *p, int policy, struct sched_param *param)
{
	int ret;
	if (policy == SCHED_FIFO || policy == SCHED_RR)
		param->sched_priority |= MT_ALLOW_RT_PRIO_BIT;

 #ifdef CONFIG_MT_PRIO_TRACER
	ret = sched_setscheduler_nocheck_binder(p, policy, param);
 #else
	ret = sched_setscheduler_nocheck(p, policy, param);
 #endif
	if (ret)
		printk(KERN_ERR "binder: set scheduler fail, error code: %d\n", ret);
}
#endif

#ifdef BINDER_MONITOR
/* binder_update_transaction_time - update read/exec done time for transaction
** step:
** 			0: start // not used
**			1: read
**			2: reply
*/
static void binder_update_transaction_time(struct binder_transaction_log *t_log,
		struct binder_transaction *bt, int step)
{
	if (step < 1 || step > 2) {
		pr_err("update trans time fail, wrong step value for id %d\n",
				bt->debug_id);
		return;
	}

	if ((bt->log_idx == -1) || (NULL == bt))
		return;
	if (t_log->entry[bt->log_idx].debug_id == bt->debug_id) {
		if (step == 1)
			do_posix_clock_monotonic_gettime(&t_log->entry[bt->log_idx].readstamp);
		else if (step == 2)
			do_posix_clock_monotonic_gettime(&t_log->entry[bt->log_idx].endstamp);
	}
}
#endif

static void binder_transaction(struct binder_proc *proc,
			       struct binder_thread *thread,
			       struct binder_transaction_data *tr, int reply)
{
	struct binder_transaction *t;
	struct binder_work *tcomplete;
	size_t *offp, *off_end;
	struct binder_proc *target_proc;
	struct binder_thread *target_thread = NULL;
	struct binder_node *target_node = NULL;
	struct list_head *target_list;
	wait_queue_head_t *target_wait;
	struct binder_transaction *in_reply_to = NULL;
	struct binder_transaction_log_entry *e;
	uint32_t return_error;
#ifdef BINDER_MONITOR
	struct binder_transaction_log_entry log_entry;
	unsigned int log_idx = -1;

	if ((reply && (tr->data_size < (proc->buffer_size/16))) || log_disable) 
		e = &log_entry;
	else
	{
		e = binder_transaction_log_add(&binder_transaction_log);
		log_idx = binder_transaction_log.next ? (binder_transaction_log.next - 1) : (MAX_ENG_TRANS_LOG_BUFF_LEN - 1);
	}
#else
	e = binder_transaction_log_add(&binder_transaction_log);
#endif
	e->call_type = reply ? 2 : !!(tr->flags & TF_ONE_WAY);
	e->from_proc = proc->pid;
	e->from_thread = thread->pid;
	e->target_handle = tr->target.handle;
	e->data_size = tr->data_size;
	e->offsets_size = tr->offsets_size;
#ifdef BINDER_MONITOR
	e->code = tr->code;
	/* fd 0 is also valid... set initial value to -1 */
	e->fd = -1;
	do_posix_clock_monotonic_gettime(&e->timestamp);
	//monotonic_to_bootbased(&e->timestamp);
	
	do_gettimeofday(&e->tv);
	/* consider time zone. translate to android time */
	e->tv.tv_sec -= (sys_tz.tz_minuteswest * 60);
#endif

	if (reply) {
		in_reply_to = thread->transaction_stack;
		if (in_reply_to == NULL) {
			binder_user_error("binder: %d:%d got reply transaction "
					  "with no transaction stack\n",
					  proc->pid, thread->pid);
			return_error = BR_FAILED_REPLY;
			goto err_empty_call_stack;
		}
#ifdef BINDER_MONITOR
		binder_cancel_bwdog(in_reply_to);
#endif
		binder_set_nice(in_reply_to->saved_priority);
#ifdef RT_PRIO_INHERIT
		if (rt_task(current) && (MAX_RT_PRIO != in_reply_to->saved_rt_prio) && 
		    !(thread->looper & (BINDER_LOOPER_STATE_REGISTERED |
					BINDER_LOOPER_STATE_ENTERED))) {
			struct sched_param param = {
				.sched_priority = in_reply_to->saved_rt_prio,
			};
			mt_sched_setscheduler_nocheck(current,
				    in_reply_to->saved_policy, &param);
#ifdef BINDER_MONITOR
			if (log_disable & BINDER_RT_LOG_ENABLE)
			{
				printk(KERN_INFO "binder: reply reset %d sched_policy from %d to %d rt_prio from %d to %d\n",
						proc->pid, in_reply_to->policy, in_reply_to->saved_policy,
						in_reply_to->rt_prio, in_reply_to->saved_rt_prio);
			}
#endif
		}
#endif
		if (in_reply_to->to_thread != thread) {
			binder_user_error("binder: %d:%d got reply transaction "
				"with bad transaction stack,"
				" transaction %d has target %d:%d\n",
				proc->pid, thread->pid, in_reply_to->debug_id,
				in_reply_to->to_proc ?
				in_reply_to->to_proc->pid : 0,
				in_reply_to->to_thread ?
				in_reply_to->to_thread->pid : 0);
			return_error = BR_FAILED_REPLY;
			in_reply_to = NULL;
			goto err_bad_call_stack;
		}
		thread->transaction_stack = in_reply_to->to_parent;
		target_thread = in_reply_to->from;
		if (target_thread == NULL) {
#ifdef MTK_BINDER_DEBUG
			binder_user_error("binder: %d:%d got reply transaction "
				"with bad transaction reply_from, "
				"transaction %d has target %d:%d\n",
				proc->pid, thread->pid, in_reply_to->debug_id,
				in_reply_to->to_proc ?
				in_reply_to->to_proc->pid : 0,
				in_reply_to->to_thread ?
				in_reply_to->to_thread->pid : 0);
#endif
			return_error = BR_DEAD_REPLY;
			goto err_dead_binder;
		}
		if (target_thread->transaction_stack != in_reply_to) {
			binder_user_error("binder: %d:%d got reply transaction "
				"with bad target transaction stack %d, "
				"expected %d\n",
				proc->pid, thread->pid,
				target_thread->transaction_stack ?
				target_thread->transaction_stack->debug_id : 0,
				in_reply_to->debug_id);
			return_error = BR_FAILED_REPLY;
			in_reply_to = NULL;
			target_thread = NULL;
			goto err_dead_binder;
		}
		target_proc = target_thread->proc;
#ifdef BINDER_MONITOR
		e->service[0] = '\0';
#endif
	} else {
		if (tr->target.handle) {
			struct binder_ref *ref;
			ref = binder_get_ref(proc, tr->target.handle);
			if (ref == NULL) {
				binder_user_error("binder: %d:%d got "
					"transaction to invalid handle\n",
					proc->pid, thread->pid);
				return_error = BR_FAILED_REPLY;
				goto err_invalid_target_handle;
			}
			target_node = ref->node;
		} else {
			target_node = binder_context_mgr_node;
			if (target_node == NULL) {
#ifdef MTK_BINDER_DEBUG
				binder_user_error("binder: %d:%d "
					"binder_context_mgr_node is NULL\n",
					proc->pid, thread->pid);
#endif
				return_error = BR_DEAD_REPLY;
				goto err_no_context_mgr_node;
			}
		}
		e->to_node = target_node->debug_id;
#ifdef BINDER_MONITOR
		strcpy(e->service, target_node->name);
#endif
		target_proc = target_node->proc;
		if (target_proc == NULL) {
#ifdef MTK_BINDER_DEBUG
			binder_user_error("binder: %d:%d target_proc is NULL\n",
				proc->pid, thread->pid);
#endif
			return_error = BR_DEAD_REPLY;
			goto err_dead_binder;
		}
		if (security_binder_transaction(proc->tsk, target_proc->tsk) < 0) {
			return_error = BR_FAILED_REPLY;
			goto err_invalid_target_handle;
		}
		if (!(tr->flags & TF_ONE_WAY) && thread->transaction_stack) {
			struct binder_transaction *tmp;
			tmp = thread->transaction_stack;
			if (tmp->to_thread != thread) {
				binder_user_error("binder: %d:%d got new "
					"transaction with bad transaction stack"
					", transaction %d has target %d:%d\n",
					proc->pid, thread->pid, tmp->debug_id,
					tmp->to_proc ? tmp->to_proc->pid : 0,
					tmp->to_thread ?
					tmp->to_thread->pid : 0);
				return_error = BR_FAILED_REPLY;
				goto err_bad_call_stack;
			}
			while (tmp) {
				if (tmp->from && tmp->from->proc == target_proc)
					target_thread = tmp->from;
				tmp = tmp->from_parent;
			}
		}
	}
	if (target_thread) {
		e->to_thread = target_thread->pid;
		target_list = &target_thread->todo;
		target_wait = &target_thread->wait;
	} else {
		target_list = &target_proc->todo;
		target_wait = &target_proc->wait;
	}
	e->to_proc = target_proc->pid;

	/* TODO: reuse incoming transaction for reply */
	t = kzalloc(sizeof(*t), GFP_KERNEL);
	if (t == NULL) {
#ifdef MTK_BINDER_DEBUG
		binder_user_error("binder: %d:%d transaction allocation failed\n",
			proc->pid, thread->pid);
#endif
		return_error = BR_FAILED_REPLY;
		goto err_alloc_t_failed;
	}
#ifdef BINDER_MONITOR
	memcpy(&t->timestamp, &e->timestamp, sizeof(struct timespec));
	//do_gettimeofday(&t->tv);
	/* consider time zone. translate to android time */
	//t->tv.tv_sec -= (sys_tz.tz_minuteswest * 60);
	memcpy(&t->tv, &e->tv, sizeof(struct timeval));
	if (!reply)
		strcpy(t->service, target_node->name);
#endif
	binder_stats_created(BINDER_STAT_TRANSACTION);

	tcomplete = kzalloc(sizeof(*tcomplete), GFP_KERNEL);
	if (tcomplete == NULL) {
#ifdef MTK_BINDER_DEBUG
		binder_user_error("binder: %d:%d tcomplete allocation failed\n",
			proc->pid, thread->pid);
#endif
		return_error = BR_FAILED_REPLY;
		goto err_alloc_tcomplete_failed;
	}
	binder_stats_created(BINDER_STAT_TRANSACTION_COMPLETE);

	t->debug_id = ++binder_last_id;
	e->debug_id = t->debug_id;

#ifdef BINDER_PERF_EVAL
	if (!reply && (binder_perf_evalue & BINDER_PERF_SEND_COUNTER))
	{
		int i, j;
		int err_code = 0;
		proc->bc_t++;
		for (i = 0; i < BC_STATS_NR; i++)
		{
			if (proc->bc_stats[i] == NULL)
				proc->bc_stats[i] = kzalloc(sizeof(struct binder_bc_stats), GFP_KERNEL);
			if (proc->bc_stats[i] == NULL) {
				printk(KERN_ERR "binder: perf_e kzalloc fail for proc %d bc_stats[%d]\n", proc->pid, i);
				err_code = 1;
				goto out_err;
			}
			if(!strcmp(proc->bc_stats[i]->service, "") &&
					(0 == proc->bc_stats[i]->code[0]))
			{
				strcpy(proc->bc_stats[i]->service, e->service);
				break;
			}
			else if (!strcmp(proc->bc_stats[i]->service, e->service))
				break;
			else
				continue;
		}
		if (BC_STATS_NR == i){
			printk(KERN_ERR "binder: perf_e bc_Stats array size"
					" is not enough\n");
			err_code = 2;
			goto out_err;
		}
		for (j = 0; j < BC_CODE_NR; j++)
		{
			if (0 == proc->bc_stats[i]->code[j])
			{
				proc->bc_stats[i]->code[j] = e->code;
				proc->bc_stats[i]->code_num[j]++;
				break;
			}
			else if (proc->bc_stats[i]->code[j] == e->code)
			{
				proc->bc_stats[i]->code_num[j]++;
				break;
			}
			else
				continue;
		}
		if (BC_CODE_NR == j) {
			printk(KERN_ERR "binder: perf_e bc_code array size"
					" is not enough\n");
			err_code = 3;
		}
out_err:
		printk(KERN_ERR "binder: perf_e update proc %d bc_stats error %d\n", proc->pid, err_code);
	}
#endif
	if (reply)
		binder_debug(BINDER_DEBUG_TRANSACTION,
			     "binder: %d:%d BC_REPLY %d -> %d:%d, "
			     "data %p-%p size %zd-%zd\n",
			     proc->pid, thread->pid, t->debug_id,
			     target_proc->pid, target_thread->pid,
			     tr->data.ptr.buffer, tr->data.ptr.offsets,
			     tr->data_size, tr->offsets_size);
	else
		binder_debug(BINDER_DEBUG_TRANSACTION,
			     "binder: %d:%d BC_TRANSACTION %d -> "
			     "%d - node %d, data %p-%p size %zd-%zd\n",
			     proc->pid, thread->pid, t->debug_id,
			     target_proc->pid, target_node->debug_id,
			     tr->data.ptr.buffer, tr->data.ptr.offsets,
			     tr->data_size, tr->offsets_size);

#ifdef BINDER_MONITOR
	t->fproc = proc->pid;
	t->fthrd = thread->pid;
	t->tproc = target_proc->pid;
	t->tthrd = target_thread ? target_thread->pid : 0;
	t->log_idx = log_idx;

	if (!binder_check_buf_checked())
	{
		binder_check_buf_pid = proc->pid;
		binder_check_buf_tid = thread->pid;
	}
#endif
	if (!reply && !(tr->flags & TF_ONE_WAY))
		t->from = thread;
	else
		t->from = NULL;
	t->sender_euid = proc->tsk->cred->euid;
	t->to_proc = target_proc;
	t->to_thread = target_thread;
	t->code = tr->code;
	t->flags = tr->flags;
	t->priority = task_nice(current);
#ifdef RT_PRIO_INHERIT
	t->rt_prio = current->rt_priority;
	t->policy = current->policy;
	t->saved_rt_prio = MAX_RT_PRIO;
#endif

	trace_binder_transaction(reply, t, target_node);

	t->buffer = binder_alloc_buf(target_proc, tr->data_size,
		tr->offsets_size, !reply && (t->flags & TF_ONE_WAY));
	if (t->buffer == NULL) {
#ifdef MTK_BINDER_DEBUG
		binder_user_error("binder: %d:%d buffer allocation failed "
			"on %d:0\n", proc->pid, thread->pid, target_proc->pid);
#endif
		return_error = BR_FAILED_REPLY;
		goto err_binder_alloc_buf_failed;
	}
	t->buffer->allow_user_free = 0;
	t->buffer->debug_id = t->debug_id;
	t->buffer->transaction = t;
#ifdef BINDER_MONITOR
	t->buffer->log_entry = e;
#endif
	t->buffer->target_node = target_node;
	trace_binder_transaction_alloc_buf(t->buffer);
	if (target_node)
		binder_inc_node(target_node, 1, 0, NULL);

	offp = (size_t *)(t->buffer->data + ALIGN(tr->data_size, sizeof(void *)));

	if (copy_from_user(t->buffer->data, tr->data.ptr.buffer, tr->data_size)) {
		binder_user_error("binder: %d:%d got transaction with invalid "
			"data ptr\n", proc->pid, thread->pid);
		return_error = BR_FAILED_REPLY;
		goto err_copy_data_failed;
	}
	if (copy_from_user(offp, tr->data.ptr.offsets, tr->offsets_size)) {
		binder_user_error("binder: %d:%d got transaction with invalid "
			"offsets ptr\n", proc->pid, thread->pid);
		return_error = BR_FAILED_REPLY;
		goto err_copy_data_failed;
	}
	if (!IS_ALIGNED(tr->offsets_size, sizeof(size_t))) {
		binder_user_error("binder: %d:%d got transaction with "
			"invalid offsets size, %zd\n",
			proc->pid, thread->pid, tr->offsets_size);
		return_error = BR_FAILED_REPLY;
		goto err_bad_offset;
	}
	off_end = (void *)offp + tr->offsets_size;
	for (; offp < off_end; offp++) {
		struct flat_binder_object *fp;
		if (*offp > t->buffer->data_size - sizeof(*fp) ||
		    t->buffer->data_size < sizeof(*fp) ||
		    !IS_ALIGNED(*offp, sizeof(void *))) {
			binder_user_error("binder: %d:%d got transaction with "
				"invalid offset, %zd\n",
				proc->pid, thread->pid, *offp);
			return_error = BR_FAILED_REPLY;
			goto err_bad_offset;
		}
		fp = (struct flat_binder_object *)(t->buffer->data + *offp);
		switch (fp->type) {
		case BINDER_TYPE_BINDER:
		case BINDER_TYPE_WEAK_BINDER: {
			struct binder_ref *ref;
			struct binder_node *node = binder_get_node(proc, fp->binder);
			if (node == NULL) {
				node = binder_new_node(proc, fp->binder, fp->cookie);
				if (node == NULL) {
#ifdef MTK_BINDER_DEBUG
					binder_user_error("binder: %d:%d create new node failed\n",
						proc->pid, thread->pid);
#endif
					return_error = BR_FAILED_REPLY;
					goto err_binder_new_node_failed;
				}
				node->min_priority = fp->flags & FLAT_BINDER_FLAG_PRIORITY_MASK;
				node->accept_fds = !!(fp->flags & FLAT_BINDER_FLAG_ACCEPTS_FDS);
#ifdef BINDER_MONITOR
				{
				int i, len = 0;
				/* this is an addService() transaction identified by:
				 * fp->type == BINDER_TYPE_BINDER && tr->target.handle == 0
				 */
				if (tr->target.handle == 0) {
					/* hack into addService() payload:
					 * service name string is located at MAGIC_SERVICE_NAME_OFFSET,
					 * and interleaved with character '\0'.
					 * for example, 'p', '\0', 'h', '\0', 'o', '\0', 'n', '\0', 'e'
					 */
					for (i = 0; (2 * i) < tr->data_size; i++) {
						if ((2 * i) < MAGIC_SERVICE_NAME_OFFSET)
							continue;
						/* prevent array index overflow */
						if (len >= (MAX_SERVICE_NAME_LEN - 1))
							break;
						len += sprintf((node->name) + len, "%c",
							       *((char *)tr->data.ptr.buffer + (2 * i)));
					}
					node->name[len] = '\0';
				} else {
					node->name[0] = '\0';
				}
				/* via addService of activity service, identify
				 * system_server's process id.
				 */
				if (!strcmp(node->name, "activity")) {
					system_server_pid = proc->pid;
					printk(KERN_INFO "binder: system_server "
							 "%d\n", system_server_pid);
				}
				}
#endif
			}
			if (fp->cookie != node->cookie) {
				binder_user_error("binder: %d:%d sending u%p "
					"node %d, cookie mismatch %p != %p\n",
					proc->pid, thread->pid,
					fp->binder, node->debug_id,
					fp->cookie, node->cookie);
				goto err_binder_get_ref_for_node_failed;
			}
			if (security_binder_transfer_binder(proc->tsk, target_proc->tsk)) {
				return_error = BR_FAILED_REPLY;
				goto err_binder_get_ref_for_node_failed;
			}
			ref = binder_get_ref_for_node(target_proc, node);
			if (ref == NULL) {
#ifdef MTK_BINDER_DEBUG
				binder_user_error("binder: %d:%d get binder ref failed\n",
					proc->pid, thread->pid);
#endif
				return_error = BR_FAILED_REPLY;
				goto err_binder_get_ref_for_node_failed;
			}
			if (fp->type == BINDER_TYPE_BINDER)
				fp->type = BINDER_TYPE_HANDLE;
			else
				fp->type = BINDER_TYPE_WEAK_HANDLE;
			fp->handle = ref->desc;
			binder_inc_ref(ref, fp->type == BINDER_TYPE_HANDLE,
				       &thread->todo);

			trace_binder_transaction_node_to_ref(t, node, ref);
			binder_debug(BINDER_DEBUG_TRANSACTION,
				     "        node %d u%p -> ref %d desc %d\n",
				     node->debug_id, node->ptr, ref->debug_id,
				     ref->desc);
		} break;
		case BINDER_TYPE_HANDLE:
		case BINDER_TYPE_WEAK_HANDLE: {
			struct binder_ref *ref = binder_get_ref(proc, fp->handle);
			if (ref == NULL) {
				binder_user_error("binder: %d:%d got "
					"transaction with invalid "
					"handle, %ld\n", proc->pid,
					thread->pid, fp->handle);
				return_error = BR_FAILED_REPLY;
				goto err_binder_get_ref_failed;
			}
			if (security_binder_transfer_binder(proc->tsk, target_proc->tsk)) {
				return_error = BR_FAILED_REPLY;
				goto err_binder_get_ref_failed;
			}
			if (ref->node->proc == target_proc) {
				if (fp->type == BINDER_TYPE_HANDLE)
					fp->type = BINDER_TYPE_BINDER;
				else
					fp->type = BINDER_TYPE_WEAK_BINDER;
				fp->binder = ref->node->ptr;
				fp->cookie = ref->node->cookie;
				binder_inc_node(ref->node, fp->type == BINDER_TYPE_BINDER, 0, NULL);
				trace_binder_transaction_ref_to_node(t, ref);
				binder_debug(BINDER_DEBUG_TRANSACTION,
					     "        ref %d desc %d -> node %d u%p\n",
					     ref->debug_id, ref->desc, ref->node->debug_id,
					     ref->node->ptr);
			} else {
				struct binder_ref *new_ref;
				new_ref = binder_get_ref_for_node(target_proc, ref->node);
				if (new_ref == NULL) {
#ifdef MTK_BINDER_DEBUG
					binder_user_error("binder: %d:%d get new binder ref failed\n",
						proc->pid, thread->pid);
#endif
					return_error = BR_FAILED_REPLY;
					goto err_binder_get_ref_for_node_failed;
				}
				fp->handle = new_ref->desc;
				binder_inc_ref(new_ref, fp->type == BINDER_TYPE_HANDLE, NULL);
				trace_binder_transaction_ref_to_ref(t, ref,
								    new_ref);
				binder_debug(BINDER_DEBUG_TRANSACTION,
					     "        ref %d desc %d -> ref %d desc %d (node %d)\n",
					     ref->debug_id, ref->desc, new_ref->debug_id,
					     new_ref->desc, ref->node->debug_id);
			}
		} break;

		case BINDER_TYPE_FD: {
			int target_fd;
			struct file *file;

			if (reply) {
				if (!(in_reply_to->flags & TF_ACCEPT_FDS)) {
					binder_user_error("binder: %d:%d got reply with fd, %ld, but target does not allow fds\n",
						proc->pid, thread->pid, fp->handle);
					return_error = BR_FAILED_REPLY;
					goto err_fd_not_allowed;
				}
			} else if (!target_node->accept_fds) {
				binder_user_error("binder: %d:%d got transaction with fd, %ld, but target does not allow fds\n",
					proc->pid, thread->pid, fp->handle);
				return_error = BR_FAILED_REPLY;
				goto err_fd_not_allowed;
			}

			file = fget(fp->handle);
			if (file == NULL) {
				binder_user_error("binder: %d:%d got transaction with invalid fd, %ld\n",
					proc->pid, thread->pid, fp->handle);
				return_error = BR_FAILED_REPLY;
				goto err_fget_failed;
			}
			if (security_binder_transfer_file(proc->tsk, target_proc->tsk, file) < 0) {
				fput(file);
				return_error = BR_FAILED_REPLY;
				goto err_get_unused_fd_failed;
			}
			target_fd = task_get_unused_fd_flags(target_proc, O_CLOEXEC);
			if (target_fd < 0) {
				fput(file);
#ifdef MTK_BINDER_DEBUG
				binder_user_error("binder: %d:%d no unused fd available, %d\n",
					proc->pid, thread->pid, target_fd);
#endif
				return_error = BR_FAILED_REPLY;
				goto err_get_unused_fd_failed;
			}
			task_fd_install(target_proc, target_fd, file);
			trace_binder_transaction_fd(t, fp->handle, target_fd);
			binder_debug(BINDER_DEBUG_TRANSACTION,
				     "        fd %ld -> %d\n", fp->handle, target_fd);
			/* TODO: fput? */
			fp->handle = target_fd;
#ifdef BINDER_MONITOR
			e->fd = target_fd;
#endif
		} break;

		default:
			binder_user_error("binder: %d:%d got transactio"
				"n with invalid object type, %lx\n",
				proc->pid, thread->pid, fp->type);
			return_error = BR_FAILED_REPLY;
			goto err_bad_object_type;
		}
	}
	if (reply) {
		BUG_ON(t->buffer->async_transaction != 0);
#ifdef BINDER_MONITOR
		binder_update_transaction_time(&binder_transaction_log,in_reply_to, 2);
#endif
		binder_pop_transaction(target_thread, in_reply_to);
	} else if (!(t->flags & TF_ONE_WAY)) {
		BUG_ON(t->buffer->async_transaction != 0);
		t->need_reply = 1;
		t->from_parent = thread->transaction_stack;
		thread->transaction_stack = t;
	} else {
		BUG_ON(target_node == NULL);
		BUG_ON(t->buffer->async_transaction != 1);
		if (target_node->has_async_transaction) {
			target_list = &target_node->async_todo;
			target_wait = NULL;
		} else
			target_node->has_async_transaction = 1;
	}
	t->work.type = BINDER_WORK_TRANSACTION;
	list_add_tail(&t->work.entry, target_list);
	tcomplete->type = BINDER_WORK_TRANSACTION_COMPLETE;
	list_add_tail(&tcomplete->entry, &thread->todo);
#ifdef RT_PRIO_INHERIT
	if (target_wait) {
		unsigned long flag;
		wait_queue_t *curr, *next;
		bool is_lock = false;

		spin_lock_irqsave(&target_wait->lock, flag);
		is_lock = true;
		list_for_each_entry_safe(curr, next, &target_wait->task_list, task_list) {
			unsigned flags = curr->flags;
			struct task_struct *tsk = curr->private;
			if (tsk == NULL) {
				spin_unlock_irqrestore(&target_wait->lock, flag);
				is_lock = false;
				wake_up_interruptible(target_wait);
				break;
			}
# ifdef MTK_BINDER_DEBUG
			if (tsk->state == TASK_UNINTERRUPTIBLE) {
				printk(KERN_ERR "binder: from %d:%d to %d:%d target "
						"thread state: %ld\n",
						proc->pid, thread->pid,
						tsk->tgid, tsk->pid, tsk->state);
				show_stack(tsk, NULL);
			}
# endif
			if (!reply && (t->policy == SCHED_RR || t->policy == SCHED_FIFO)&&
			    t->rt_prio > tsk->rt_priority &&
			    !(t->flags & TF_ONE_WAY)) {
				struct sched_param param = {
					.sched_priority = t->rt_prio,
				};

				t->saved_rt_prio = tsk->rt_priority;
				t->saved_policy = tsk->policy;
				mt_sched_setscheduler_nocheck(tsk, t->policy, &param);
#ifdef BINDER_MONITOR
				if (log_disable & BINDER_RT_LOG_ENABLE)
				{	
					printk(KERN_INFO "binder: write set %d sched_policy from %d to %d rt_prio from %d to %d\n",
						tsk->pid, t->saved_policy, t->policy,
						t->saved_rt_prio, t->rt_prio);
				}
#endif
			}
			if (curr->func(curr, TASK_INTERRUPTIBLE, 0, NULL) &&
			    (flags & WQ_FLAG_EXCLUSIVE))
				break;
		}
		if (is_lock)
			spin_unlock_irqrestore(&target_wait->lock, flag);
	}
#else
	if (target_wait)
		wake_up_interruptible(target_wait);
#endif

#ifdef BINDER_MONITOR
	t->wait_on = reply ? WAIT_ON_REPLY_READ : WAIT_ON_READ;
	binder_queue_bwdog(t, (time_t)WAIT_BUDGET_READ);
#endif
	return;

err_get_unused_fd_failed:
err_fget_failed:
err_fd_not_allowed:
err_binder_get_ref_for_node_failed:
err_binder_get_ref_failed:
err_binder_new_node_failed:
err_bad_object_type:
err_bad_offset:
err_copy_data_failed:
	trace_binder_transaction_failed_buffer_release(t->buffer);
	binder_transaction_buffer_release(target_proc, t->buffer, offp);
	t->buffer->transaction = NULL;
	binder_free_buf(target_proc, t->buffer);
err_binder_alloc_buf_failed:
	kfree(tcomplete);
	binder_stats_deleted(BINDER_STAT_TRANSACTION_COMPLETE);
err_alloc_tcomplete_failed:
#ifdef BINDER_MONITOR
	binder_cancel_bwdog(t);
#endif
	kfree(t);
	binder_stats_deleted(BINDER_STAT_TRANSACTION);
err_alloc_t_failed:
err_bad_call_stack:
err_empty_call_stack:
err_dead_binder:
err_invalid_target_handle:
err_no_context_mgr_node:
	binder_debug(BINDER_DEBUG_FAILED_TRANSACTION,
		     "binder: %d:%d transaction failed %d, size %zd-%zd\n",
		     proc->pid, thread->pid, return_error,
		     tr->data_size, tr->offsets_size);

	{
		struct binder_transaction_log_entry *fe;
		fe = binder_transaction_log_add(&binder_transaction_log_failed);
		*fe = *e;
	}

	BUG_ON(thread->return_error != BR_OK);
	if (in_reply_to) {
		thread->return_error = BR_TRANSACTION_COMPLETE;
		binder_send_failed_reply(in_reply_to, return_error);
	} else
		thread->return_error = return_error;
}

int binder_thread_write(struct binder_proc *proc, struct binder_thread *thread,
			void __user *buffer, int size, signed long *consumed)
{
	uint32_t cmd;
	void __user *ptr = buffer + *consumed;
	void __user *end = buffer + size;

	while (ptr < end && thread->return_error == BR_OK) {
		if (get_user(cmd, (uint32_t __user *)ptr))
			return -EFAULT;
		ptr += sizeof(uint32_t);
		trace_binder_command(cmd);
		if (_IOC_NR(cmd) < ARRAY_SIZE(binder_stats.bc)) {
			binder_stats.bc[_IOC_NR(cmd)]++;
			proc->stats.bc[_IOC_NR(cmd)]++;
			thread->stats.bc[_IOC_NR(cmd)]++;
		}
		switch (cmd) {
		case BC_INCREFS:
		case BC_ACQUIRE:
		case BC_RELEASE:
		case BC_DECREFS: {
			uint32_t target;
			struct binder_ref *ref;
			const char *debug_string;

			if (get_user(target, (uint32_t __user *)ptr))
				return -EFAULT;
			ptr += sizeof(uint32_t);
			if (target == 0 && binder_context_mgr_node &&
			    (cmd == BC_INCREFS || cmd == BC_ACQUIRE)) {
				ref = binder_get_ref_for_node(proc,
					       binder_context_mgr_node);
				if (ref->desc != target) {
					binder_user_error("binder: %d:"
						"%d tried to acquire "
						"reference to desc 0, "
						"got %d instead\n",
						proc->pid, thread->pid,
						ref->desc);
				}
			} else
				ref = binder_get_ref(proc, target);
			if (ref == NULL) {
				binder_user_error("binder: %d:%d refcou"
					"nt change on invalid ref %d\n",
					proc->pid, thread->pid, target);
				break;
			}
			switch (cmd) {
			case BC_INCREFS:
				debug_string = "IncRefs";
				binder_inc_ref(ref, 0, NULL);
				break;
			case BC_ACQUIRE:
				debug_string = "Acquire";
				binder_inc_ref(ref, 1, NULL);
				break;
			case BC_RELEASE:
				debug_string = "Release";
				binder_dec_ref(ref, 1);
				break;
			case BC_DECREFS:
			default:
				debug_string = "DecRefs";
				binder_dec_ref(ref, 0);
				break;
			}
			binder_debug(BINDER_DEBUG_USER_REFS,
				     "binder: %d:%d %s ref %d desc %d s %d w %d for node %d\n",
				     proc->pid, thread->pid, debug_string, ref->debug_id,
				     ref->desc, ref->strong, ref->weak, ref->node->debug_id);
			break;
		}
		case BC_INCREFS_DONE:
		case BC_ACQUIRE_DONE: {
			void __user *node_ptr;
			void *cookie;
			struct binder_node *node;

			if (get_user(node_ptr, (void * __user *)ptr))
				return -EFAULT;
			ptr += sizeof(void *);
			if (get_user(cookie, (void * __user *)ptr))
				return -EFAULT;
			ptr += sizeof(void *);
			node = binder_get_node(proc, node_ptr);
			if (node == NULL) {
				binder_user_error("binder: %d:%d "
					"%s u%p no match\n",
					proc->pid, thread->pid,
					cmd == BC_INCREFS_DONE ?
					"BC_INCREFS_DONE" :
					"BC_ACQUIRE_DONE",
					node_ptr);
				break;
			}
			if (cookie != node->cookie) {
				binder_user_error("binder: %d:%d %s u%p node %d"
					" cookie mismatch %p != %p\n",
					proc->pid, thread->pid,
					cmd == BC_INCREFS_DONE ?
					"BC_INCREFS_DONE" : "BC_ACQUIRE_DONE",
					node_ptr, node->debug_id,
					cookie, node->cookie);
				break;
			}
			if (cmd == BC_ACQUIRE_DONE) {
				if (node->pending_strong_ref == 0) {
					binder_user_error("binder: %d:%d "
						"BC_ACQUIRE_DONE node %d has "
						"no pending acquire request\n",
						proc->pid, thread->pid,
						node->debug_id);
					break;
				}
				node->pending_strong_ref = 0;
			} else {
				if (node->pending_weak_ref == 0) {
					binder_user_error("binder: %d:%d "
						"BC_INCREFS_DONE node %d has "
						"no pending increfs request\n",
						proc->pid, thread->pid,
						node->debug_id);
					break;
				}
				node->pending_weak_ref = 0;
			}
			binder_dec_node(node, cmd == BC_ACQUIRE_DONE, 0);
			binder_debug(BINDER_DEBUG_USER_REFS,
				     "binder: %d:%d %s node %d ls %d lw %d\n",
				     proc->pid, thread->pid,
				     cmd == BC_INCREFS_DONE ? "BC_INCREFS_DONE" : "BC_ACQUIRE_DONE",
				     node->debug_id, node->local_strong_refs, node->local_weak_refs);
			break;
		}
		case BC_ATTEMPT_ACQUIRE:
			printk(KERN_ERR "binder: BC_ATTEMPT_ACQUIRE not supported\n");
			return -EINVAL;
		case BC_ACQUIRE_RESULT:
			printk(KERN_ERR "binder: BC_ACQUIRE_RESULT not supported\n");
			return -EINVAL;

		case BC_FREE_BUFFER: {
			void __user *data_ptr;
			struct binder_buffer *buffer;

			if (get_user(data_ptr, (void * __user *)ptr))
				return -EFAULT;
			ptr += sizeof(void *);

			buffer = binder_buffer_lookup(proc, data_ptr);
			if (buffer == NULL) {
				binder_user_error("binder: %d:%d "
					"BC_FREE_BUFFER u%p no match\n",
					proc->pid, thread->pid, data_ptr);
				break;
			}
			if (!buffer->allow_user_free) {
				binder_user_error("binder: %d:%d "
					"BC_FREE_BUFFER u%p matched "
					"unreturned buffer\n",
					proc->pid, thread->pid, data_ptr);
				break;
			}
			binder_debug(BINDER_DEBUG_FREE_BUFFER,
				     "binder: %d:%d BC_FREE_BUFFER u%p found buffer %d for %s transaction\n",
				     proc->pid, thread->pid, data_ptr, buffer->debug_id,
				     buffer->transaction ? "active" : "finished");

			if (buffer->transaction) {
				buffer->transaction->buffer = NULL;
				buffer->transaction = NULL;
			}
			if (buffer->async_transaction && buffer->target_node) {
				BUG_ON(!buffer->target_node->has_async_transaction);
				if (list_empty(&buffer->target_node->async_todo))
					buffer->target_node->has_async_transaction = 0;
				else
					list_move_tail(buffer->target_node->async_todo.next, &thread->todo);
			}
			trace_binder_transaction_buffer_release(buffer);
			binder_transaction_buffer_release(proc, buffer, NULL);
			binder_free_buf(proc, buffer);
			break;
		}

		case BC_TRANSACTION:
		case BC_REPLY: {
			struct binder_transaction_data tr;

			if (copy_from_user(&tr, ptr, sizeof(tr)))
				return -EFAULT;
			ptr += sizeof(tr);
			binder_transaction(proc, thread, &tr, cmd == BC_REPLY);
			break;
		}

		case BC_REGISTER_LOOPER:
			binder_debug(BINDER_DEBUG_THREADS,
				     "binder: %d:%d BC_REGISTER_LOOPER\n",
				     proc->pid, thread->pid);
			if (thread->looper & BINDER_LOOPER_STATE_ENTERED) {
				thread->looper |= BINDER_LOOPER_STATE_INVALID;
				binder_user_error("binder: %d:%d ERROR:"
					" BC_REGISTER_LOOPER called "
					"after BC_ENTER_LOOPER\n",
					proc->pid, thread->pid);
			} else if (proc->requested_threads == 0) {
				thread->looper |= BINDER_LOOPER_STATE_INVALID;
				binder_user_error("binder: %d:%d ERROR:"
					" BC_REGISTER_LOOPER called "
					"without request\n",
					proc->pid, thread->pid);
			} else {
				proc->requested_threads--;
				proc->requested_threads_started++;
			}
			thread->looper |= BINDER_LOOPER_STATE_REGISTERED;
			break;
		case BC_ENTER_LOOPER:
			binder_debug(BINDER_DEBUG_THREADS,
				     "binder: %d:%d BC_ENTER_LOOPER\n",
				     proc->pid, thread->pid);
			if (thread->looper & BINDER_LOOPER_STATE_REGISTERED) {
				thread->looper |= BINDER_LOOPER_STATE_INVALID;
				binder_user_error("binder: %d:%d ERROR:"
					" BC_ENTER_LOOPER called after "
					"BC_REGISTER_LOOPER\n",
					proc->pid, thread->pid);
			}
			thread->looper |= BINDER_LOOPER_STATE_ENTERED;
			break;
		case BC_EXIT_LOOPER:
			binder_debug(BINDER_DEBUG_THREADS,
				     "binder: %d:%d BC_EXIT_LOOPER\n",
				     proc->pid, thread->pid);
			thread->looper |= BINDER_LOOPER_STATE_EXITED;
			break;

		case BC_REQUEST_DEATH_NOTIFICATION:
		case BC_CLEAR_DEATH_NOTIFICATION: {
			uint32_t target;
			void __user *cookie;
			struct binder_ref *ref;
			struct binder_ref_death *death;

			if (get_user(target, (uint32_t __user *)ptr))
				return -EFAULT;
			ptr += sizeof(uint32_t);
			if (get_user(cookie, (void __user * __user *)ptr))
				return -EFAULT;
			ptr += sizeof(void *);
			ref = binder_get_ref(proc, target);
			if (ref == NULL) {
				binder_user_error("binder: %d:%d %s "
					"invalid ref %d\n",
					proc->pid, thread->pid,
					cmd == BC_REQUEST_DEATH_NOTIFICATION ?
					"BC_REQUEST_DEATH_NOTIFICATION" :
					"BC_CLEAR_DEATH_NOTIFICATION",
					target);
				break;
			}
#ifdef MTK_DEATH_NOTIFY_MONITOR
			binder_debug(BINDER_DEBUG_DEATH_NOTIFICATION,
				     "[DN #%s]binder: %d:%d %s %d cookie 0x%p\n",
				     cmd == BC_REQUEST_DEATH_NOTIFICATION ? "1" : "2",
				     proc->pid, thread->pid,
				     cmd == BC_REQUEST_DEATH_NOTIFICATION ?
				         "BC_REQUEST_DEATH_NOTIFICATION" :
				         "BC_CLEAR_DEATH_NOTIFICATION",
				     ref->node->proc ? ref->node->proc->pid : 0,
				     cookie);
#else
			binder_debug(BINDER_DEBUG_DEATH_NOTIFICATION,
				     "binder: %d:%d %s %p ref %d desc %d s %d w %d for node %d\n",
				     proc->pid, thread->pid,
				     cmd == BC_REQUEST_DEATH_NOTIFICATION ?
				     "BC_REQUEST_DEATH_NOTIFICATION" :
				     "BC_CLEAR_DEATH_NOTIFICATION",
				     cookie, ref->debug_id, ref->desc,
				     ref->strong, ref->weak, ref->node->debug_id);
#endif

			if (cmd == BC_REQUEST_DEATH_NOTIFICATION) {
				if (ref->death) {
					binder_user_error("binder: %d:%"
						"d BC_REQUEST_DEATH_NOTI"
						"FICATION death notific"
						"ation already set\n",
						proc->pid, thread->pid);
					break;
				}
				death = kzalloc(sizeof(*death), GFP_KERNEL);
				if (death == NULL) {
					thread->return_error = BR_ERROR;
					binder_debug(BINDER_DEBUG_FAILED_TRANSACTION,
						     "binder: %d:%d "
						     "BC_REQUEST_DEATH_NOTIFICATION failed\n",
						     proc->pid, thread->pid);
					break;
				}
				binder_stats_created(BINDER_STAT_DEATH);
				INIT_LIST_HEAD(&death->work.entry);
				death->cookie = cookie;
				ref->death = death;
				if (ref->node->proc == NULL) {
					ref->death->work.type = BINDER_WORK_DEAD_BINDER;
					if (thread->looper & (BINDER_LOOPER_STATE_REGISTERED | BINDER_LOOPER_STATE_ENTERED)) {
						list_add_tail(&ref->death->work.entry, &thread->todo);
					} else {
						list_add_tail(&ref->death->work.entry, &proc->todo);
						wake_up_interruptible(&proc->wait);
					}
				}
			} else {
				if (ref->death == NULL) {
					binder_user_error("binder: %d:%"
						"d BC_CLEAR_DEATH_NOTIFI"
						"CATION death notificat"
						"ion not active\n",
						proc->pid, thread->pid);
					break;
				}
				death = ref->death;
				if (death->cookie != cookie) {
					binder_user_error("binder: %d:%"
						"d BC_CLEAR_DEATH_NOTIFI"
						"CATION death notificat"
						"ion cookie mismatch "
						"%p != %p\n",
						proc->pid, thread->pid,
						death->cookie, cookie);
					break;
				}
				ref->death = NULL;
				if (list_empty(&death->work.entry)) {
					death->work.type = BINDER_WORK_CLEAR_DEATH_NOTIFICATION;
					if (thread->looper & (BINDER_LOOPER_STATE_REGISTERED | BINDER_LOOPER_STATE_ENTERED)) {
						list_add_tail(&death->work.entry, &thread->todo);
					} else {
						list_add_tail(&death->work.entry, &proc->todo);
						wake_up_interruptible(&proc->wait);
					}
				} else {
					BUG_ON(death->work.type != BINDER_WORK_DEAD_BINDER);
					death->work.type = BINDER_WORK_DEAD_BINDER_AND_CLEAR;
				}
			}
		} break;
		case BC_DEAD_BINDER_DONE: {
			struct binder_work *w;
			void __user *cookie;
			struct binder_ref_death *death = NULL;
			if (get_user(cookie, (void __user * __user *)ptr))
				return -EFAULT;

#ifdef MTK_DEATH_NOTIFY_MONITOR
			binder_debug(BINDER_DEBUG_DEATH_NOTIFICATION,
				     "[DN #6]binder: %d:%d cookie 0x%p\n", proc->pid, thread->pid, cookie);
#endif

			ptr += sizeof(void *);
			list_for_each_entry(w, &proc->delivered_death, entry) {
				struct binder_ref_death *tmp_death = container_of(w, struct binder_ref_death, work);
				if (tmp_death->cookie == cookie) {
					death = tmp_death;
					break;
				}
			}
			binder_debug(BINDER_DEBUG_DEAD_BINDER,
				     "binder: %d:%d BC_DEAD_BINDER_DONE %p found %p\n",
				     proc->pid, thread->pid, cookie, death);
			if (death == NULL) {
				binder_user_error("binder: %d:%d BC_DEAD"
					"_BINDER_DONE %p not found\n",
					proc->pid, thread->pid, cookie);
				break;
			}

			list_del_init(&death->work.entry);
			if (death->work.type == BINDER_WORK_DEAD_BINDER_AND_CLEAR) {
				death->work.type = BINDER_WORK_CLEAR_DEATH_NOTIFICATION;
				if (thread->looper & (BINDER_LOOPER_STATE_REGISTERED | BINDER_LOOPER_STATE_ENTERED)) {
					list_add_tail(&death->work.entry, &thread->todo);
				} else {
					list_add_tail(&death->work.entry, &proc->todo);
					wake_up_interruptible(&proc->wait);
				}
			}
		} break;

		default:
			printk(KERN_ERR "binder: %d:%d unknown command %d\n",
			       proc->pid, thread->pid, cmd);
			return -EINVAL;
		}
		*consumed = ptr - buffer;
	}
	return 0;
}

void binder_stat_br(struct binder_proc *proc, struct binder_thread *thread,
		    uint32_t cmd)
{
	trace_binder_return(cmd);
	if (_IOC_NR(cmd) < ARRAY_SIZE(binder_stats.br)) {
		binder_stats.br[_IOC_NR(cmd)]++;
		proc->stats.br[_IOC_NR(cmd)]++;
		thread->stats.br[_IOC_NR(cmd)]++;
	}
}

static int binder_has_proc_work(struct binder_proc *proc,
				struct binder_thread *thread)
{
	return !list_empty(&proc->todo) ||
		(thread->looper & BINDER_LOOPER_STATE_NEED_RETURN);
}

static int binder_has_thread_work(struct binder_thread *thread)
{
	return !list_empty(&thread->todo) || thread->return_error != BR_OK ||
		(thread->looper & BINDER_LOOPER_STATE_NEED_RETURN);
}

static int binder_thread_read(struct binder_proc *proc,
			      struct binder_thread *thread,
			      void  __user *buffer, int size,
			      signed long *consumed, int non_block)
{
	void __user *ptr = buffer + *consumed;
	void __user *end = buffer + size;

	int ret = 0;
	int wait_for_proc_work;

	if (*consumed == 0) {
		if (put_user(BR_NOOP, (uint32_t __user *)ptr))
			return -EFAULT;
		ptr += sizeof(uint32_t);
	}

retry:
	wait_for_proc_work = thread->transaction_stack == NULL &&
				list_empty(&thread->todo);

	if (thread->return_error != BR_OK && ptr < end) {
		if (thread->return_error2 != BR_OK) {
			if (put_user(thread->return_error2, (uint32_t __user *)ptr))
				return -EFAULT;
			ptr += sizeof(uint32_t);
			binder_stat_br(proc, thread, thread->return_error2);
			if (ptr == end)
				goto done;
			thread->return_error2 = BR_OK;
		}
		if (put_user(thread->return_error, (uint32_t __user *)ptr))
			return -EFAULT;
		ptr += sizeof(uint32_t);
		binder_stat_br(proc, thread, thread->return_error);
		thread->return_error = BR_OK;
		goto done;
	}


	thread->looper |= BINDER_LOOPER_STATE_WAITING;
	if (wait_for_proc_work)
		proc->ready_threads++;

	binder_unlock(__func__);

	trace_binder_wait_for_work(wait_for_proc_work,
				   !!thread->transaction_stack,
				   !list_empty(&thread->todo));
	if (wait_for_proc_work) {
		if (!(thread->looper & (BINDER_LOOPER_STATE_REGISTERED |
					BINDER_LOOPER_STATE_ENTERED))) {
			binder_user_error("binder: %d:%d ERROR: Thread waiting "
				"for process work before calling BC_REGISTER_"
				"LOOPER or BC_ENTER_LOOPER (state %x)\n",
				proc->pid, thread->pid, thread->looper);
			wait_event_interruptible(binder_user_error_wait,
						 binder_stop_on_user_error < 2);
		}
#ifdef RT_PRIO_INHERIT
		/* disable preemption to prevent from schedule-out immediately */
		preempt_disable();
#endif
		binder_set_nice(proc->default_priority);
#ifdef RT_PRIO_INHERIT
		if (rt_task(current) && !binder_has_proc_work(proc, thread)) {
			/* make sure binder has no work before setting priority back*/
			struct sched_param param = {
				.sched_priority = proc->default_rt_prio,
			};
#ifdef BINDER_MONITOR
			if (log_disable & BINDER_RT_LOG_ENABLE)
			{	
				printk(KERN_INFO "binder: enter threadpool reset %d sched_policy from %u to %d rt_prio from %u to %d\n",
						current->pid, current->policy, proc->default_policy,
						current->rt_priority, proc->default_rt_prio);
			}
#endif
			mt_sched_setscheduler_nocheck(current,
				    proc->default_policy, &param);
		}
		preempt_enable_no_resched();
#endif
		if (non_block) {
			if (!binder_has_proc_work(proc, thread))
				ret = -EAGAIN;
		} else
			ret = wait_event_freezable_exclusive(proc->wait, binder_has_proc_work(proc, thread));
	} else {
		if (non_block) {
			if (!binder_has_thread_work(thread))
				ret = -EAGAIN;
		} else
			ret = wait_event_freezable(thread->wait, binder_has_thread_work(thread));
	}

	binder_lock(__func__);

	if (wait_for_proc_work)
		proc->ready_threads--;
	thread->looper &= ~BINDER_LOOPER_STATE_WAITING;

	if (ret)
		return ret;

	while (1) {
		uint32_t cmd;
		struct binder_transaction_data tr;
		struct binder_work *w;
		struct binder_transaction *t = NULL;

		if (!list_empty(&thread->todo))
			w = list_first_entry(&thread->todo, struct binder_work, entry);
		else if (!list_empty(&proc->todo) && wait_for_proc_work)
			w = list_first_entry(&proc->todo, struct binder_work, entry);
		else {
			if (ptr - buffer == 4 && !(thread->looper & BINDER_LOOPER_STATE_NEED_RETURN)) /* no data added */
				goto retry;
			break;
		}

		if (end - ptr < sizeof(tr) + 4)
			break;

		switch (w->type) {
		case BINDER_WORK_TRANSACTION: {
			t = container_of(w, struct binder_transaction, work);
#ifdef BINDER_MONITOR
			binder_cancel_bwdog(t);
#endif
		} break;
		case BINDER_WORK_TRANSACTION_COMPLETE: {
			cmd = BR_TRANSACTION_COMPLETE;
			if (put_user(cmd, (uint32_t __user *)ptr))
				return -EFAULT;
			ptr += sizeof(uint32_t);

			binder_stat_br(proc, thread, cmd);
			binder_debug(BINDER_DEBUG_TRANSACTION_COMPLETE,
				     "binder: %d:%d BR_TRANSACTION_COMPLETE\n",
				     proc->pid, thread->pid);

			list_del(&w->entry);
			kfree(w);
			binder_stats_deleted(BINDER_STAT_TRANSACTION_COMPLETE);
		} break;
		case BINDER_WORK_NODE: {
			struct binder_node *node = container_of(w, struct binder_node, work);
			uint32_t cmd = BR_NOOP;
			const char *cmd_name;
			int strong = node->internal_strong_refs || node->local_strong_refs;
			int weak = !hlist_empty(&node->refs) || node->local_weak_refs || strong;
			if (weak && !node->has_weak_ref) {
				cmd = BR_INCREFS;
				cmd_name = "BR_INCREFS";
				node->has_weak_ref = 1;
				node->pending_weak_ref = 1;
				node->local_weak_refs++;
			} else if (strong && !node->has_strong_ref) {
				cmd = BR_ACQUIRE;
				cmd_name = "BR_ACQUIRE";
				node->has_strong_ref = 1;
				node->pending_strong_ref = 1;
				node->local_strong_refs++;
			} else if (!strong && node->has_strong_ref) {
				cmd = BR_RELEASE;
				cmd_name = "BR_RELEASE";
				node->has_strong_ref = 0;
			} else if (!weak && node->has_weak_ref) {
				cmd = BR_DECREFS;
				cmd_name = "BR_DECREFS";
				node->has_weak_ref = 0;
			}
			if (cmd != BR_NOOP) {
				if (put_user(cmd, (uint32_t __user *)ptr))
					return -EFAULT;
				ptr += sizeof(uint32_t);
				if (put_user(node->ptr, (void * __user *)ptr))
					return -EFAULT;
				ptr += sizeof(void *);
				if (put_user(node->cookie, (void * __user *)ptr))
					return -EFAULT;
				ptr += sizeof(void *);

				binder_stat_br(proc, thread, cmd);
				binder_debug(BINDER_DEBUG_USER_REFS,
					     "binder: %d:%d %s %d u%p c%p\n",
					     proc->pid, thread->pid, cmd_name, node->debug_id, node->ptr, node->cookie);
			} else {
				list_del_init(&w->entry);
				if (!weak && !strong) {
					binder_debug(BINDER_DEBUG_INTERNAL_REFS,
						     "binder: %d:%d node %d u%p c%p deleted\n",
						     proc->pid, thread->pid, node->debug_id,
						     node->ptr, node->cookie);
					rb_erase(&node->rb_node, &proc->nodes);
					kfree(node);
					binder_stats_deleted(BINDER_STAT_NODE);
				} else {
					binder_debug(BINDER_DEBUG_INTERNAL_REFS,
						     "binder: %d:%d node %d u%p c%p state unchanged\n",
						     proc->pid, thread->pid, node->debug_id, node->ptr,
						     node->cookie);
				}
			}
		} break;
		case BINDER_WORK_DEAD_BINDER:
		case BINDER_WORK_DEAD_BINDER_AND_CLEAR:
		case BINDER_WORK_CLEAR_DEATH_NOTIFICATION: {
			struct binder_ref_death *death;
			uint32_t cmd;

			death = container_of(w, struct binder_ref_death, work);

#ifdef MTK_DEATH_NOTIFY_MONITOR
			switch (w->type) {
				case BINDER_WORK_DEAD_BINDER:
					binder_debug(BINDER_DEBUG_DEATH_NOTIFICATION,
						     "[DN #4]binder: %d:%d BINDER_WORK_DEAD_BINDER cookie 0x%p\n",
						     proc->pid, thread->pid, death->cookie);
					break;
				case BINDER_WORK_DEAD_BINDER_AND_CLEAR:
					binder_debug(BINDER_DEBUG_DEATH_NOTIFICATION,
						     "[DN #4]binder: %d:%d BINDER_WORK_DEAD_BINDER_AND_CLEAR cookie "
						     "0x%p\n", proc->pid, thread->pid, death->cookie);
					break;
				case BINDER_WORK_CLEAR_DEATH_NOTIFICATION:
					binder_debug(BINDER_DEBUG_DEATH_NOTIFICATION,
						     "[DN #4]binder: %d:%d BINDER_WORK_CLEAR_DEATH_NOTIFICATION cookie "
						     "0x%p\n", proc->pid, thread->pid, death->cookie);
					break;
				default:
					binder_debug(BINDER_DEBUG_DEATH_NOTIFICATION,
						     "[DN #4]binder: %d:%d UNKNOWN-%d cookie 0x%p\n",
						     proc->pid, thread->pid, w->type, death->cookie);
					break;
			}
#endif

			if (w->type == BINDER_WORK_CLEAR_DEATH_NOTIFICATION)
				cmd = BR_CLEAR_DEATH_NOTIFICATION_DONE;
			else
				cmd = BR_DEAD_BINDER;
			if (put_user(cmd, (uint32_t __user *)ptr))
				return -EFAULT;
			ptr += sizeof(uint32_t);
			if (put_user(death->cookie, (void * __user *)ptr))
				return -EFAULT;
			ptr += sizeof(void *);
			binder_stat_br(proc, thread, cmd);
#ifndef MTK_DEATH_NOTIFY_MONITOR
			binder_debug(BINDER_DEBUG_DEATH_NOTIFICATION,
				     "binder: %d:%d %s %p\n",
				      proc->pid, thread->pid,
				      cmd == BR_DEAD_BINDER ?
				      "BR_DEAD_BINDER" :
				      "BR_CLEAR_DEATH_NOTIFICATION_DONE",
				      death->cookie);
#endif

			if (w->type == BINDER_WORK_CLEAR_DEATH_NOTIFICATION) {
				list_del(&w->entry);
				kfree(death);
				binder_stats_deleted(BINDER_STAT_DEATH);
			} else
				list_move(&w->entry, &proc->delivered_death);
			if (cmd == BR_DEAD_BINDER)
				goto done; /* DEAD_BINDER notifications can cause transactions */
		} break;
		}

		if (!t)
			continue;

		BUG_ON(t->buffer == NULL);
		if (t->buffer->target_node) {
			struct binder_node *target_node = t->buffer->target_node;
			tr.target.ptr = target_node->ptr;
			tr.cookie =  target_node->cookie;
			t->saved_priority = task_nice(current);
#ifdef RT_PRIO_INHERIT
			/* since we may fail the rt inherit due to target
			 * wait queue task_list is empty, check again here.
			 */ 
			if ((SCHED_RR == t->policy || SCHED_FIFO == t->policy) && t->rt_prio > current->rt_priority &&
			    !(t->flags & TF_ONE_WAY)) {
				struct sched_param param = {
					.sched_priority = t->rt_prio,
				};

				t->saved_rt_prio = current->rt_priority;
				t->saved_policy = current->policy;
				mt_sched_setscheduler_nocheck(current, t->policy, &param);
#ifdef BINDER_MONITOR
				if (log_disable & BINDER_RT_LOG_ENABLE)
				{	
					printk(KERN_INFO "binder: read set %d sched_policy from %d to %d rt_prio from %d to %d\n",
						proc->pid, t->saved_policy, t->policy,
						t->saved_rt_prio, t->rt_prio);
				}
#endif
			}
#endif
			if (t->priority < target_node->min_priority &&
			    !(t->flags & TF_ONE_WAY))
				binder_set_nice(t->priority);
			else if (!(t->flags & TF_ONE_WAY) ||
				 t->saved_priority > target_node->min_priority)
				binder_set_nice(target_node->min_priority);
			cmd = BR_TRANSACTION;
		} else {
			tr.target.ptr = NULL;
			tr.cookie = NULL;
			cmd = BR_REPLY;
		}
		tr.code = t->code;
		tr.flags = t->flags;
		tr.sender_euid = t->sender_euid;

		if (t->from) {
			struct task_struct *sender = t->from->proc->tsk;
			tr.sender_pid = task_tgid_nr_ns(sender,
							current->nsproxy->pid_ns);
		} else {
			tr.sender_pid = 0;
		}

		tr.data_size = t->buffer->data_size;
		tr.offsets_size = t->buffer->offsets_size;
		tr.data.ptr.buffer = (void *)t->buffer->data +
					proc->user_buffer_offset;
		tr.data.ptr.offsets = tr.data.ptr.buffer +
					ALIGN(t->buffer->data_size,
					    sizeof(void *));

		if (put_user(cmd, (uint32_t __user *)ptr))
			return -EFAULT;
		ptr += sizeof(uint32_t);
		if (copy_to_user(ptr, &tr, sizeof(tr)))
			return -EFAULT;
		ptr += sizeof(tr);

		trace_binder_transaction_received(t);
		binder_stat_br(proc, thread, cmd);
		binder_debug(BINDER_DEBUG_TRANSACTION,
			     "binder: %d:%d %s %d %d:%d, cmd %d"
			     "size %zd-%zd ptr %p-%p\n",
			     proc->pid, thread->pid,
			     (cmd == BR_TRANSACTION) ? "BR_TRANSACTION" :
			     "BR_REPLY",
			     t->debug_id, t->from ? t->from->proc->pid : 0,
			     t->from ? t->from->pid : 0, cmd,
			     t->buffer->data_size, t->buffer->offsets_size,
			     tr.data.ptr.buffer, tr.data.ptr.offsets);

		list_del(&t->work.entry);
		t->buffer->allow_user_free = 1;
		if (cmd == BR_TRANSACTION && !(t->flags & TF_ONE_WAY)) {
			t->to_parent = thread->transaction_stack;
			t->to_thread = thread;
			thread->transaction_stack = t;
#ifdef BINDER_MONITOR
			do_posix_clock_monotonic_gettime(&t->exe_timestamp);
			//monotonic_to_bootbased(&t->exe_timestamp);
			do_gettimeofday(&t->tv);
			/* consider time zone. translate to android time */
			t->tv.tv_sec -= (sys_tz.tz_minuteswest * 60);
			t->wait_on = WAIT_ON_EXEC;
			t->tthrd = thread->pid;
			binder_queue_bwdog(t, (time_t)WAIT_BUDGET_EXEC);
			binder_update_transaction_time(&binder_transaction_log, t, 1);
#endif
		} else {
			t->buffer->transaction = NULL;
#ifdef BINDER_MONITOR
			binder_cancel_bwdog(t);
			if (cmd == BR_TRANSACTION && (t->flags & TF_ONE_WAY))
				binder_update_transaction_time(&binder_transaction_log, t, 1);
#endif
			kfree(t);
			binder_stats_deleted(BINDER_STAT_TRANSACTION);
		}
		break;
	}

done:

	*consumed = ptr - buffer;
	if (proc->requested_threads + proc->ready_threads == 0 &&
	    proc->requested_threads_started < proc->max_threads &&
	    (thread->looper & (BINDER_LOOPER_STATE_REGISTERED |
	     BINDER_LOOPER_STATE_ENTERED)) /* the user-space code fails to */
	     /*spawn a new thread if we leave this out */) {
		proc->requested_threads++;
		binder_debug(BINDER_DEBUG_THREADS,
			     "binder: %d:%d BR_SPAWN_LOOPER\n",
			     proc->pid, thread->pid);
		if (put_user(BR_SPAWN_LOOPER, (uint32_t __user *)buffer))
			return -EFAULT;
		binder_stat_br(proc, thread, BR_SPAWN_LOOPER);
	}
	return 0;
}

static void binder_release_work(struct list_head *list)
{
	struct binder_work *w;
	while (!list_empty(list)) {
		w = list_first_entry(list, struct binder_work, entry);
		list_del_init(&w->entry);
		switch (w->type) {
		case BINDER_WORK_TRANSACTION: {
			struct binder_transaction *t;

			t = container_of(w, struct binder_transaction, work);
			if (t->buffer->target_node &&
			    !(t->flags & TF_ONE_WAY)) {
				binder_send_failed_reply(t, BR_DEAD_REPLY);
			} else {
				binder_debug(BINDER_DEBUG_DEAD_TRANSACTION,
					"binder: undelivered transaction %d\n",
					t->debug_id);
				t->buffer->transaction = NULL;
#ifdef BINDER_MONITOR
				binder_cancel_bwdog(t);
#endif
				kfree(t);
				binder_stats_deleted(BINDER_STAT_TRANSACTION);
			}
		} break;
		case BINDER_WORK_TRANSACTION_COMPLETE: {
			binder_debug(BINDER_DEBUG_DEAD_TRANSACTION,
				"binder: undelivered TRANSACTION_COMPLETE\n");
			kfree(w);
			binder_stats_deleted(BINDER_STAT_TRANSACTION_COMPLETE);
		} break;
		case BINDER_WORK_DEAD_BINDER_AND_CLEAR:
		case BINDER_WORK_CLEAR_DEATH_NOTIFICATION: {
			struct binder_ref_death *death;

			death = container_of(w, struct binder_ref_death, work);
			binder_debug(BINDER_DEBUG_DEAD_TRANSACTION,
				"binder: undelivered death notification, %p\n",
				death->cookie);
			kfree(death);
			binder_stats_deleted(BINDER_STAT_DEATH);
		} break;
		default:
			pr_err("binder: unexpected work type, %d, not freed\n",
				w->type);
			break;
		}
	}

}

static struct binder_thread *binder_get_thread(struct binder_proc *proc)
{
	struct binder_thread *thread = NULL;
	struct rb_node *parent = NULL;
	struct rb_node **p = &proc->threads.rb_node;

	while (*p) {
		parent = *p;
		thread = rb_entry(parent, struct binder_thread, rb_node);

		if (current->pid < thread->pid)
			p = &(*p)->rb_left;
		else if (current->pid > thread->pid)
			p = &(*p)->rb_right;
		else
			break;
	}
	if (*p == NULL) {
		thread = kzalloc(sizeof(*thread), GFP_KERNEL);
		if (thread == NULL)
			return NULL;
		binder_stats_created(BINDER_STAT_THREAD);
		thread->proc = proc;
		thread->pid = current->pid;
		init_waitqueue_head(&thread->wait);
		INIT_LIST_HEAD(&thread->todo);
		rb_link_node(&thread->rb_node, parent, p);
		rb_insert_color(&thread->rb_node, &proc->threads);
		thread->looper |= BINDER_LOOPER_STATE_NEED_RETURN;
		thread->return_error = BR_OK;
		thread->return_error2 = BR_OK;
	}
	return thread;
}

static int binder_free_thread(struct binder_proc *proc,
			      struct binder_thread *thread)
{
	struct binder_transaction *t;
	struct binder_transaction *send_reply = NULL;
	int active_transactions = 0;

	rb_erase(&thread->rb_node, &proc->threads);
	t = thread->transaction_stack;
	if (t && t->to_thread == thread)
		send_reply = t;
	while (t) {
		active_transactions++;
		binder_debug(BINDER_DEBUG_DEAD_TRANSACTION,
			     "binder: release %d:%d transaction %d "
			     "%s, still active\n", proc->pid, thread->pid,
			     t->debug_id,
			     (t->to_thread == thread) ? "in" : "out");

#ifdef MTK_BINDER_DEBUG
		printk(KERN_ERR "binder: %d: %p from %d:%d to %d:%d code %x flags %x "
				"pri %ld r%d "
#ifdef BINDER_MONITOR
				"start %lu.%06lu"
#endif
				,
				t->debug_id, t,
				t->from ? t->from->proc->pid : 0,
				t->from ? t->from->pid : 0,
				t->to_proc ? t->to_proc->pid : 0,
				t->to_thread ? t->to_thread->pid : 0,
				t->code, t->flags, t->priority, t->need_reply
#ifdef BINDER_MONITOR
				, (unsigned long)t->timestamp.tv_sec,
				(t->timestamp.tv_nsec / NSEC_PER_USEC)
#endif
				);
#endif
		if (t->to_thread == thread) {
			t->to_proc = NULL;
			t->to_thread = NULL;
			if (t->buffer) {
				t->buffer->transaction = NULL;
				t->buffer = NULL;
			}
			t = t->to_parent;
		} else if (t->from == thread) {
			t->from = NULL;
			t = t->from_parent;
		} else
			BUG();
	}
	if (send_reply)
		binder_send_failed_reply(send_reply, BR_DEAD_REPLY);
	binder_release_work(&thread->todo);
	kfree(thread);
	binder_stats_deleted(BINDER_STAT_THREAD);
	return active_transactions;
}

static unsigned int binder_poll(struct file *filp,
				struct poll_table_struct *wait)
{
	struct binder_proc *proc = filp->private_data;
	struct binder_thread *thread = NULL;
	int wait_for_proc_work;

	binder_lock(__func__);

	thread = binder_get_thread(proc);

	wait_for_proc_work = thread->transaction_stack == NULL &&
		list_empty(&thread->todo) && thread->return_error == BR_OK;

	binder_unlock(__func__);

	if (wait_for_proc_work) {
		if (binder_has_proc_work(proc, thread))
			return POLLIN;
		poll_wait(filp, &proc->wait, wait);
		if (binder_has_proc_work(proc, thread))
			return POLLIN;
	} else {
		if (binder_has_thread_work(thread))
			return POLLIN;
		poll_wait(filp, &thread->wait, wait);
		if (binder_has_thread_work(thread))
			return POLLIN;
	}
	return 0;
}

static long binder_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret;
	struct binder_proc *proc = filp->private_data;
	struct binder_thread *thread;
	unsigned int size = _IOC_SIZE(cmd);
	void __user *ubuf = (void __user *)arg;

	/*printk(KERN_INFO "binder_ioctl: %d:%d %x %lx\n", proc->pid, current->pid, cmd, arg);*/

	trace_binder_ioctl(cmd, arg);

	ret = wait_event_interruptible(binder_user_error_wait, binder_stop_on_user_error < 2);
	if (ret)
		goto err_unlocked;

	binder_lock(__func__);
	thread = binder_get_thread(proc);
	if (thread == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	switch (cmd) {
	case BINDER_WRITE_READ: {
		struct binder_write_read bwr;
		if (size != sizeof(struct binder_write_read)) {
			ret = -EINVAL;
			goto err;
		}
		if (copy_from_user(&bwr, ubuf, sizeof(bwr))) {
			ret = -EFAULT;
			goto err;
		}
		binder_debug(BINDER_DEBUG_READ_WRITE,
			     "binder: %d:%d write %ld at %08lx, read %ld at %08lx\n",
			     proc->pid, thread->pid, bwr.write_size, bwr.write_buffer,
			     bwr.read_size, bwr.read_buffer);

		if (bwr.write_size > 0) {
			ret = binder_thread_write(proc, thread, (void __user *)bwr.write_buffer, bwr.write_size, &bwr.write_consumed);
			trace_binder_write_done(ret);
			if (ret < 0) {
				bwr.read_consumed = 0;
				if (copy_to_user(ubuf, &bwr, sizeof(bwr)))
					ret = -EFAULT;
				goto err;
			}
		}
		if (bwr.read_size > 0) {
			ret = binder_thread_read(proc, thread, (void __user *)bwr.read_buffer, bwr.read_size, &bwr.read_consumed, filp->f_flags & O_NONBLOCK);
			trace_binder_read_done(ret);
			if (!list_empty(&proc->todo))
				wake_up_interruptible(&proc->wait);
			if (ret < 0) {
				if (copy_to_user(ubuf, &bwr, sizeof(bwr)))
					ret = -EFAULT;
				goto err;
			}
		}
		binder_debug(BINDER_DEBUG_READ_WRITE,
			     "binder: %d:%d wrote %ld of %ld, read return %ld of %ld\n",
			     proc->pid, thread->pid, bwr.write_consumed, bwr.write_size,
			     bwr.read_consumed, bwr.read_size);
		if (copy_to_user(ubuf, &bwr, sizeof(bwr))) {
			ret = -EFAULT;
			goto err;
		}
		break;
	}
	case BINDER_SET_MAX_THREADS:
		if (copy_from_user(&proc->max_threads, ubuf, sizeof(proc->max_threads))) {
			ret = -EINVAL;
			goto err;
		}
		break;
	case BINDER_SET_CONTEXT_MGR:
		if (binder_context_mgr_node != NULL) {
			printk(KERN_ERR "binder: BINDER_SET_CONTEXT_MGR already set\n");
			ret = -EBUSY;
			goto err;
		}
		ret = security_binder_set_context_mgr(proc->tsk);
		if (ret < 0)
			goto err;
		if (binder_context_mgr_uid != -1) {
			if (binder_context_mgr_uid != current->cred->euid) {
				printk(KERN_ERR "binder: BINDER_SET_"
				       "CONTEXT_MGR bad uid %d != %d\n",
				       current->cred->euid,
				       binder_context_mgr_uid);
				ret = -EPERM;
				goto err;
			}
		} else
			binder_context_mgr_uid = current->cred->euid;
		binder_context_mgr_node = binder_new_node(proc, NULL, NULL);
		if (binder_context_mgr_node == NULL) {
			ret = -ENOMEM;
			goto err;
		}
#ifdef BINDER_MONITOR
		strcpy(binder_context_mgr_node->name, "servicemanager");
		printk(KERN_INFO "binder: %d:%d set as servicemanager uid %d\n",
				proc->pid, thread->pid, binder_context_mgr_uid);
#endif
		binder_context_mgr_node->local_weak_refs++;
		binder_context_mgr_node->local_strong_refs++;
		binder_context_mgr_node->has_strong_ref = 1;
		binder_context_mgr_node->has_weak_ref = 1;
		break;
	case BINDER_THREAD_EXIT:
		binder_debug(BINDER_DEBUG_THREADS, "binder: %d:%d exit\n",
			     proc->pid, thread->pid);
		binder_free_thread(proc, thread);
		thread = NULL;
		break;
	case BINDER_VERSION:
		if (size != sizeof(struct binder_version)) {
			ret = -EINVAL;
			goto err;
		}
		if (put_user(BINDER_CURRENT_PROTOCOL_VERSION, &((struct binder_version *)ubuf)->protocol_version)) {
			ret = -EINVAL;
			goto err;
		}
		break;
	default:
		ret = -EINVAL;
		goto err;
	}
	ret = 0;
err:
	if (thread)
		thread->looper &= ~BINDER_LOOPER_STATE_NEED_RETURN;
	binder_unlock(__func__);
	wait_event_interruptible(binder_user_error_wait, binder_stop_on_user_error < 2);
	if (ret && ret != -ERESTARTSYS)
		printk(KERN_INFO "binder: %d:%d ioctl %x %lx returned %d\n", proc->pid, current->pid, cmd, arg, ret);
err_unlocked:
	trace_binder_ioctl_done(ret);
	return ret;
}

static void binder_vma_open(struct vm_area_struct *vma)
{
	struct binder_proc *proc = vma->vm_private_data;
	binder_debug(BINDER_DEBUG_OPEN_CLOSE,
		     "binder: %d open vm area %lx-%lx (%ld K) vma %lx pagep %lx\n",
		     proc->pid, vma->vm_start, vma->vm_end,
		     (vma->vm_end - vma->vm_start) / SZ_1K, vma->vm_flags,
		     (unsigned long)pgprot_val(vma->vm_page_prot));
}

static void binder_vma_close(struct vm_area_struct *vma)
{
	struct binder_proc *proc = vma->vm_private_data;
	binder_debug(BINDER_DEBUG_OPEN_CLOSE,
		     "binder: %d close vm area %lx-%lx (%ld K) vma %lx pagep %lx\n",
		     proc->pid, vma->vm_start, vma->vm_end,
		     (vma->vm_end - vma->vm_start) / SZ_1K, vma->vm_flags,
		     (unsigned long)pgprot_val(vma->vm_page_prot));
	proc->vma = NULL;
	proc->vma_vm_mm = NULL;
	binder_defer_work(proc, BINDER_DEFERRED_PUT_FILES);
}

static struct vm_operations_struct binder_vm_ops = {
	.open = binder_vma_open,
	.close = binder_vma_close,
};

static int binder_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int ret;
	struct vm_struct *area;
	struct binder_proc *proc = filp->private_data;
	const char *failure_string;
	struct binder_buffer *buffer;

	if ((vma->vm_end - vma->vm_start) > SZ_4M)
		vma->vm_end = vma->vm_start + SZ_4M;

	binder_debug(BINDER_DEBUG_OPEN_CLOSE,
		     "binder_mmap: %d %lx-%lx (%ld K) vma %lx pagep %lx\n",
		     proc->pid, vma->vm_start, vma->vm_end,
		     (vma->vm_end - vma->vm_start) / SZ_1K, vma->vm_flags,
		     (unsigned long)pgprot_val(vma->vm_page_prot));

	if (vma->vm_flags & FORBIDDEN_MMAP_FLAGS) {
		ret = -EPERM;
		failure_string = "bad vm_flags";
		goto err_bad_arg;
	}
	vma->vm_flags = (vma->vm_flags | VM_DONTCOPY) & ~VM_MAYWRITE;

	mutex_lock(&binder_mmap_lock);
	if (proc->buffer) {
		ret = -EBUSY;
		failure_string = "already mapped";
		goto err_already_mapped;
	}

	area = get_vm_area(vma->vm_end - vma->vm_start, VM_IOREMAP);
	if (area == NULL) {
		ret = -ENOMEM;
		failure_string = "get_vm_area";
		goto err_get_vm_area_failed;
	}
	proc->buffer = area->addr;
	proc->user_buffer_offset = vma->vm_start - (uintptr_t)proc->buffer;
	mutex_unlock(&binder_mmap_lock);

#ifdef CONFIG_CPU_CACHE_VIPT
	if (cache_is_vipt_aliasing()) {
		while (CACHE_COLOUR((vma->vm_start ^ (uint32_t)proc->buffer))) {
			printk(KERN_INFO "binder_mmap: %d %lx-%lx maps %p bad alignment\n", proc->pid, vma->vm_start, vma->vm_end, proc->buffer);
			vma->vm_start += PAGE_SIZE;
		}
	}
#endif
	proc->pages = kzalloc(sizeof(proc->pages[0]) * ((vma->vm_end - vma->vm_start) / PAGE_SIZE), GFP_KERNEL);
	if (proc->pages == NULL) {
		ret = -ENOMEM;
		failure_string = "alloc page array";
		goto err_alloc_pages_failed;
	}
	proc->buffer_size = vma->vm_end - vma->vm_start;

	vma->vm_ops = &binder_vm_ops;
	vma->vm_private_data = proc;

	if (binder_update_page_range(proc, 1, proc->buffer, proc->buffer + PAGE_SIZE, vma)) {
		ret = -ENOMEM;
		failure_string = "alloc small buf";
		goto err_alloc_small_buf_failed;
	}
	buffer = proc->buffer;
	INIT_LIST_HEAD(&proc->buffers);
	list_add(&buffer->entry, &proc->buffers);
	buffer->free = 1;
	binder_insert_free_buffer(proc, buffer);
	proc->free_async_space = proc->buffer_size / 2;
	barrier();
	proc->files = get_files_struct(proc->tsk);
	proc->vma = vma;
	proc->vma_vm_mm = vma->vm_mm;

	/*printk(KERN_INFO "binder_mmap: %d %lx-%lx maps %p\n",
		 proc->pid, vma->vm_start, vma->vm_end, proc->buffer);*/
	return 0;

err_alloc_small_buf_failed:
	kfree(proc->pages);
	proc->pages = NULL;
err_alloc_pages_failed:
	mutex_lock(&binder_mmap_lock);
	vfree(proc->buffer);
	proc->buffer = NULL;
err_get_vm_area_failed:
err_already_mapped:
	mutex_unlock(&binder_mmap_lock);
err_bad_arg:
	printk(KERN_ERR "binder_mmap: %d %lx-%lx %s failed %d\n",
	       proc->pid, vma->vm_start, vma->vm_end, failure_string, ret);
	return ret;
}

static int binder_open(struct inode *nodp, struct file *filp)
{
	struct binder_proc *proc;

	binder_debug(BINDER_DEBUG_OPEN_CLOSE, "binder_open: %d:%d\n",
		     current->group_leader->pid, current->pid);

	proc = kzalloc(sizeof(*proc), GFP_KERNEL);
	if (proc == NULL)
		return -ENOMEM;
	get_task_struct(current);
	proc->tsk = current;
	INIT_LIST_HEAD(&proc->todo);
	init_waitqueue_head(&proc->wait);
	proc->default_priority = task_nice(current);
#ifdef RT_PRIO_INHERIT
	proc->default_rt_prio = current->rt_priority;
	proc->default_policy = current->policy;
#endif

	binder_lock(__func__);

	binder_stats_created(BINDER_STAT_PROC);
	hlist_add_head(&proc->proc_node, &binder_procs);
	proc->pid = current->group_leader->pid;
	INIT_LIST_HEAD(&proc->delivered_death);
	filp->private_data = proc;

	binder_unlock(__func__);

	if (binder_debugfs_dir_entry_proc) {
		char strbuf[11];
		snprintf(strbuf, sizeof(strbuf), "%u", proc->pid);
		proc->debugfs_entry = debugfs_create_file(strbuf, S_IRUGO,
			binder_debugfs_dir_entry_proc, proc, &binder_proc_fops);
	}

	return 0;
}

static int binder_flush(struct file *filp, fl_owner_t id)
{
	struct binder_proc *proc = filp->private_data;

	binder_defer_work(proc, BINDER_DEFERRED_FLUSH);

	return 0;
}

static void binder_deferred_flush(struct binder_proc *proc)
{
	struct rb_node *n;
	int wake_count = 0;
	for (n = rb_first(&proc->threads); n != NULL; n = rb_next(n)) {
		struct binder_thread *thread = rb_entry(n, struct binder_thread, rb_node);
		thread->looper |= BINDER_LOOPER_STATE_NEED_RETURN;
		if (thread->looper & BINDER_LOOPER_STATE_WAITING) {
			wake_up_interruptible(&thread->wait);
			wake_count++;
		}
	}
	wake_up_interruptible_all(&proc->wait);

	binder_debug(BINDER_DEBUG_OPEN_CLOSE,
		     "binder_flush: %d woke %d threads\n", proc->pid,
		     wake_count);
}

static int binder_release(struct inode *nodp, struct file *filp)
{
	struct binder_proc *proc = filp->private_data;
	debugfs_remove(proc->debugfs_entry);
	binder_defer_work(proc, BINDER_DEFERRED_RELEASE);

	return 0;
}

static void binder_deferred_release(struct binder_proc *proc)
{
	struct hlist_node *pos;
	struct binder_transaction *t;
	struct rb_node *n;
	int threads, nodes, incoming_refs, outgoing_refs, buffers, active_transactions, page_count;
#ifdef MTK_BINDER_DEBUG
	bool sys_reg = false;
#endif

	BUG_ON(proc->vma);
	BUG_ON(proc->files);

	hlist_del(&proc->proc_node);
	if (binder_context_mgr_node && binder_context_mgr_node->proc == proc) {
		binder_debug(BINDER_DEBUG_DEAD_BINDER,
			     "binder_release: %d context_mgr_node gone\n",
			     proc->pid);
		binder_context_mgr_node = NULL;
	}

	threads = 0;
	active_transactions = 0;
	while ((n = rb_first(&proc->threads))) {
		struct binder_thread *thread = rb_entry(n, struct binder_thread, rb_node);
		threads++;
		active_transactions += binder_free_thread(proc, thread);
	}
	nodes = 0;
	incoming_refs = 0;
	while ((n = rb_first(&proc->nodes))) {
		struct binder_node *node = rb_entry(n, struct binder_node, rb_node);

		nodes++;
		rb_erase(&node->rb_node, &proc->nodes);
		list_del_init(&node->work.entry);
		binder_release_work(&node->async_todo);
		if (hlist_empty(&node->refs)) {
			kfree(node);
			binder_stats_deleted(BINDER_STAT_NODE);
		} else {
			struct binder_ref *ref;
			int death = 0;

			node->proc = NULL;
			node->local_strong_refs = 0;
			node->local_weak_refs = 0;
			hlist_add_head(&node->dead_node, &binder_dead_nodes);

			hlist_for_each_entry(ref, pos, &node->refs, node_entry) {
				incoming_refs++;
				if (ref->death) {
#ifdef MTK_DEATH_NOTIFY_MONITOR
					binder_debug(BINDER_DEBUG_DEATH_NOTIFICATION,
						     "[DN #3]binder: %d: cookie 0x%p\n",
						     proc->pid, ref->death->cookie);
#endif
#ifdef MTK_BINDER_DEBUG
					if (!sys_reg &&
					    ref->proc->pid == system_server_pid)
						sys_reg = true;
#endif
					death++;
					if (list_empty(&ref->death->work.entry)) {
						ref->death->work.type = BINDER_WORK_DEAD_BINDER;
						list_add_tail(&ref->death->work.entry, &ref->proc->todo);
						wake_up_interruptible(&ref->proc->wait);
					} else
						BUG();
				}
			}
			binder_debug(BINDER_DEBUG_DEAD_BINDER,
				     "binder: node %d now dead, "
				     "refs %d, death %d\n", node->debug_id,
				     incoming_refs, death);
		}
	}
#ifdef MTK_BINDER_DEBUG
	printk(KERN_INFO "binder: %d:%s exits %s system_server DeathNotify\n",
			 proc->pid, proc->tsk ? proc->tsk->comm : "",
			 sys_reg ? "with" : "W/O");
#endif
	outgoing_refs = 0;
	while ((n = rb_first(&proc->refs_by_desc))) {
		struct binder_ref *ref = rb_entry(n, struct binder_ref,
						  rb_node_desc);
		outgoing_refs++;
		binder_delete_ref(ref);
	}
	binder_release_work(&proc->todo);
	binder_release_work(&proc->delivered_death);
	buffers = 0;

	while ((n = rb_first(&proc->allocated_buffers))) {
		struct binder_buffer *buffer = rb_entry(n, struct binder_buffer,
							rb_node);
		t = buffer->transaction;
		if (t) {
			t->buffer = NULL;
			buffer->transaction = NULL;
			printk(KERN_ERR "binder: release proc %d, "
			       "transaction %d, not freed\n",
			       proc->pid, t->debug_id);
			/*BUG();*/
#ifdef MTK_BINDER_DEBUG
			printk(KERN_ERR "binder: %d: %p from %d:%d to %d:%d code %x flags %x "
					"pri %ld r%d "
#ifdef BINDER_MONITOR
					"start %lu.%06lu"
#endif
					,
					t->debug_id, t,
					t->from ? t->from->proc->pid : 0,
					t->from ? t->from->pid : 0,
					t->to_proc ? t->to_proc->pid : 0,
					t->to_thread ? t->to_thread->pid : 0,
					t->code, t->flags, t->priority, t->need_reply
#ifdef BINDER_MONITOR
					, (unsigned long)t->timestamp.tv_sec,
					(t->timestamp.tv_nsec / NSEC_PER_USEC)
#endif
					);
#endif
		}
		binder_free_buf(proc, buffer);
		buffers++;
	}

	binder_stats_deleted(BINDER_STAT_PROC);

	page_count = 0;
	if (proc->pages) {
		int i;
		for (i = 0; i < proc->buffer_size / PAGE_SIZE; i++) {
			if (proc->pages[i]) {
				void *page_addr = proc->buffer + i * PAGE_SIZE;
				binder_debug(BINDER_DEBUG_BUFFER_ALLOC,
					     "binder_release: %d: "
					     "page %d at %p not freed\n",
					     proc->pid, i,
					     page_addr);
				unmap_kernel_range((unsigned long)page_addr,
						PAGE_SIZE);
				__free_page(proc->pages[i]);
				page_count++;				
#ifdef MTK_BINDER_PAGE_USED_RECORD
				if(binder_page_used > 0)
					binder_page_used--;
				if (proc->page_used > 0)
					proc->page_used--;
#endif				
			}
		}
		kfree(proc->pages);
		vfree(proc->buffer);
	}

	put_task_struct(proc->tsk);

	binder_debug(BINDER_DEBUG_OPEN_CLOSE,
		     "binder_release: %d threads %d, nodes %d (ref %d), "
		     "refs %d, active transactions %d, buffers %d, "
		     "pages %d\n",
		     proc->pid, threads, nodes, incoming_refs, outgoing_refs,
		     active_transactions, buffers, page_count);

#ifdef BINDER_PERF_EVAL
	{
	int i;
	for (i = 0; i < BC_STATS_NR; i++) {
		if (proc->bc_stats[i] != NULL) {
			kfree(proc->bc_stats[i]);
			proc->bc_stats[i] = NULL;
			printk(KERN_INFO "binder: binder_release: release %d bc_stats[%d]\n", proc->pid, i);
		}
	}
	}
#endif
	kfree(proc);
}

static void binder_deferred_func(struct work_struct *work)
{
	struct binder_proc *proc;
	struct files_struct *files;

	int defer;
	do {
		binder_lock(__func__);
		mutex_lock(&binder_deferred_lock);
		if (!hlist_empty(&binder_deferred_list)) {
			proc = hlist_entry(binder_deferred_list.first,
					struct binder_proc, deferred_work_node);
			hlist_del_init(&proc->deferred_work_node);
			defer = proc->deferred_work;
			proc->deferred_work = 0;
		} else {
			proc = NULL;
			defer = 0;
		}
		mutex_unlock(&binder_deferred_lock);

		files = NULL;
		if (defer & BINDER_DEFERRED_PUT_FILES) {
			files = proc->files;
			if (files)
				proc->files = NULL;
		}

		if (defer & BINDER_DEFERRED_FLUSH)
			binder_deferred_flush(proc);

		if (defer & BINDER_DEFERRED_RELEASE)
			binder_deferred_release(proc); /* frees proc */

		binder_unlock(__func__);
		if (files)
			put_files_struct(files);
	} while (proc);
}

static DECLARE_WORK(binder_deferred_work, binder_deferred_func);

static void
binder_defer_work(struct binder_proc *proc, enum binder_deferred_state defer)
{
	mutex_lock(&binder_deferred_lock);
	proc->deferred_work |= defer;
	if (hlist_unhashed(&proc->deferred_work_node)) {
		hlist_add_head(&proc->deferred_work_node,
				&binder_deferred_list);
		queue_work(binder_deferred_workqueue, &binder_deferred_work);
	}
	mutex_unlock(&binder_deferred_lock);
}

static void print_binder_transaction(struct seq_file *m, const char *prefix,
				     struct binder_transaction *t)
{
#ifdef BINDER_MONITOR
	struct rtc_time tm;
	rtc_time_to_tm(t->tv.tv_sec, &tm);
#endif
	seq_printf(m,
		   "%s %d: %p from %d:%d to %d:%d code %x flags %x pri %ld r%d",
		   prefix, t->debug_id, t,
		   t->from ? t->from->proc->pid : 0,
		   t->from ? t->from->pid : 0,
		   t->to_proc ? t->to_proc->pid : 0,
		   t->to_thread ? t->to_thread->pid : 0,
		   t->code, t->flags, t->priority, t->need_reply);
	if (t->buffer == NULL) {
#ifdef BINDER_MONITOR
		seq_printf(m, " start %lu.%06lu android %d-%02d-%02d %02d:%02d:%02d.%03lu"
			" buffer free\n",
			(unsigned long)t->timestamp.tv_sec,
			(t->timestamp.tv_nsec / NSEC_PER_USEC),
			(tm.tm_year + 1900), (tm.tm_mon + 1), tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec,
			(unsigned long)(t->tv.tv_usec / USEC_PER_MSEC));
#else
		seq_puts(m, " buffer free\n");
#endif
		return;
	}
	if (t->buffer->target_node)
		seq_printf(m, " node %d",
			   t->buffer->target_node->debug_id);
#ifdef BINDER_MONITOR
	seq_printf(m, " size %zd:%zd data %p auf %d start %lu.%06lu"
		   " android %d-%02d-%02d %02d:%02d:%02d.%03lu\n",
		   t->buffer->data_size, t->buffer->offsets_size,
		   t->buffer->data, t->buffer->allow_user_free,
		   (unsigned long)t->timestamp.tv_sec,
		   (t->timestamp.tv_nsec / NSEC_PER_USEC),
		   (tm.tm_year + 1900), (tm.tm_mon + 1), tm.tm_mday,
		   tm.tm_hour, tm.tm_min, tm.tm_sec,
		   (unsigned long)(t->tv.tv_usec / USEC_PER_MSEC));
#else
	seq_printf(m, " size %zd:%zd data %p\n",
		   t->buffer->data_size, t->buffer->offsets_size,
		   t->buffer->data);
#endif	
}

static void print_binder_buffer(struct seq_file *m, const char *prefix,
				struct binder_buffer *buffer)
{
	seq_printf(m, "%s %d: %p size %zd:%zd %s\n",
		   prefix, buffer->debug_id, buffer->data,
		   buffer->data_size, buffer->offsets_size,
		   buffer->transaction ? "active" : "delivered");
}

static void print_binder_work(struct seq_file *m, const char *prefix,
			      const char *transaction_prefix,
			      struct binder_work *w)
{
	struct binder_node *node;
	struct binder_transaction *t;

	switch (w->type) {
	case BINDER_WORK_TRANSACTION:
		t = container_of(w, struct binder_transaction, work);
		print_binder_transaction(m, transaction_prefix, t);
		break;
	case BINDER_WORK_TRANSACTION_COMPLETE:
		seq_printf(m, "%stransaction complete\n", prefix);
		break;
	case BINDER_WORK_NODE:
		node = container_of(w, struct binder_node, work);
		seq_printf(m, "%snode work %d: u%p c%p\n",
			   prefix, node->debug_id, node->ptr, node->cookie);
		break;
	case BINDER_WORK_DEAD_BINDER:
		seq_printf(m, "%shas dead binder\n", prefix);
		break;
	case BINDER_WORK_DEAD_BINDER_AND_CLEAR:
		seq_printf(m, "%shas cleared dead binder\n", prefix);
		break;
	case BINDER_WORK_CLEAR_DEATH_NOTIFICATION:
		seq_printf(m, "%shas cleared death notification\n", prefix);
		break;
	default:
		seq_printf(m, "%sunknown work: type %d\n", prefix, w->type);
		break;
	}
}

static void print_binder_thread(struct seq_file *m,
				struct binder_thread *thread,
				int print_always)
{
	struct binder_transaction *t;
	struct binder_work *w;
	size_t start_pos = m->count;
	size_t header_pos;

	seq_printf(m, "  thread %d: l %02x\n", thread->pid, thread->looper);
	header_pos = m->count;
	t = thread->transaction_stack;
	while (t) {
		if (t->from == thread) {
			print_binder_transaction(m,
						 "    outgoing transaction", t);
			t = t->from_parent;
		} else if (t->to_thread == thread) {
			print_binder_transaction(m,
						 "    incoming transaction", t);
			t = t->to_parent;
		} else {
			print_binder_transaction(m, "    bad transaction", t);
			t = NULL;
		}
	}
	list_for_each_entry(w, &thread->todo, entry) {
		print_binder_work(m, "    ", "    pending transaction", w);
	}
	if (!print_always && m->count == header_pos)
		m->count = start_pos;
}

static void print_binder_node(struct seq_file *m, struct binder_node *node)
{
	struct binder_ref *ref;
	struct hlist_node *pos;
	struct binder_work *w;
	int count;

	count = 0;
	hlist_for_each_entry(ref, pos, &node->refs, node_entry)
		count++;

#ifdef BINDER_MONITOR
	seq_printf(m, "  node %d (%s): u%p c%p hs %d hw %d ls %d lw %d is %d iw %d",
		   node->debug_id, node->name, node->ptr, node->cookie,
		   node->has_strong_ref, node->has_weak_ref,
		   node->local_strong_refs, node->local_weak_refs,
		   node->internal_strong_refs, count);
#else
	seq_printf(m, "  node %d: u%p c%p hs %d hw %d ls %d lw %d is %d iw %d",
		   node->debug_id, node->ptr, node->cookie,
		   node->has_strong_ref, node->has_weak_ref,
		   node->local_strong_refs, node->local_weak_refs,
		   node->internal_strong_refs, count);
#endif
	if (count) {
		seq_puts(m, " proc");
		hlist_for_each_entry(ref, pos, &node->refs, node_entry)
			seq_printf(m, " %d", ref->proc->pid);
	}
	seq_puts(m, "\n");
	list_for_each_entry(w, &node->async_todo, entry)
		print_binder_work(m, "    ",
				  "    pending async transaction", w);
}

static void print_binder_ref(struct seq_file *m, struct binder_ref *ref)
{
	seq_printf(m, "  ref %d: desc %d %snode %d s %d w %d d %p\n",
		   ref->debug_id, ref->desc, ref->node->proc ? "" : "dead ",
		   ref->node->debug_id, ref->strong, ref->weak, ref->death);
}

static void print_binder_proc(struct seq_file *m,
			      struct binder_proc *proc, int print_all)
{
	struct binder_work *w;
	struct rb_node *n;
	size_t start_pos = m->count;
	size_t header_pos;

	seq_printf(m, "proc %d\n", proc->pid);
	header_pos = m->count;

	for (n = rb_first(&proc->threads); n != NULL; n = rb_next(n))
		print_binder_thread(m, rb_entry(n, struct binder_thread,
						rb_node), print_all);
	for (n = rb_first(&proc->nodes); n != NULL; n = rb_next(n)) {
		struct binder_node *node = rb_entry(n, struct binder_node,
						    rb_node);
		if (print_all || node->has_async_transaction)
			print_binder_node(m, node);
	}
	if (print_all) {
		for (n = rb_first(&proc->refs_by_desc);
		     n != NULL;
		     n = rb_next(n))
			print_binder_ref(m, rb_entry(n, struct binder_ref,
						     rb_node_desc));
	}
	for (n = rb_first(&proc->allocated_buffers); n != NULL; n = rb_next(n))
		print_binder_buffer(m, "  buffer",
				    rb_entry(n, struct binder_buffer, rb_node));
	list_for_each_entry(w, &proc->todo, entry)
		print_binder_work(m, "  ", "  pending transaction", w);
	list_for_each_entry(w, &proc->delivered_death, entry) {
		seq_puts(m, "  has delivered dead binder\n");
		break;
	}
	if (!print_all && m->count == header_pos)
		m->count = start_pos;
}

static const char *binder_return_strings[] = {
	"BR_ERROR",
	"BR_OK",
	"BR_TRANSACTION",
	"BR_REPLY",
	"BR_ACQUIRE_RESULT",
	"BR_DEAD_REPLY",
	"BR_TRANSACTION_COMPLETE",
	"BR_INCREFS",
	"BR_ACQUIRE",
	"BR_RELEASE",
	"BR_DECREFS",
	"BR_ATTEMPT_ACQUIRE",
	"BR_NOOP",
	"BR_SPAWN_LOOPER",
	"BR_FINISHED",
	"BR_DEAD_BINDER",
	"BR_CLEAR_DEATH_NOTIFICATION_DONE",
	"BR_FAILED_REPLY"
};

static const char *binder_command_strings[] = {
	"BC_TRANSACTION",
	"BC_REPLY",
	"BC_ACQUIRE_RESULT",
	"BC_FREE_BUFFER",
	"BC_INCREFS",
	"BC_ACQUIRE",
	"BC_RELEASE",
	"BC_DECREFS",
	"BC_INCREFS_DONE",
	"BC_ACQUIRE_DONE",
	"BC_ATTEMPT_ACQUIRE",
	"BC_REGISTER_LOOPER",
	"BC_ENTER_LOOPER",
	"BC_EXIT_LOOPER",
	"BC_REQUEST_DEATH_NOTIFICATION",
	"BC_CLEAR_DEATH_NOTIFICATION",
	"BC_DEAD_BINDER_DONE"
};

static const char *binder_objstat_strings[] = {
	"proc",
	"thread",
	"node",
	"ref",
	"death",
	"transaction",
	"transaction_complete"
};

static void print_binder_stats(struct seq_file *m, const char *prefix,
			       struct binder_stats *stats)
{
	int i;

	BUILD_BUG_ON(ARRAY_SIZE(stats->bc) !=
		     ARRAY_SIZE(binder_command_strings));
	for (i = 0; i < ARRAY_SIZE(stats->bc); i++) {
		if (stats->bc[i])
			seq_printf(m, "%s%s: %d\n", prefix,
				   binder_command_strings[i], stats->bc[i]);
	}

	BUILD_BUG_ON(ARRAY_SIZE(stats->br) !=
		     ARRAY_SIZE(binder_return_strings));
	for (i = 0; i < ARRAY_SIZE(stats->br); i++) {
		if (stats->br[i])
			seq_printf(m, "%s%s: %d\n", prefix,
				   binder_return_strings[i], stats->br[i]);
	}

	BUILD_BUG_ON(ARRAY_SIZE(stats->obj_created) !=
		     ARRAY_SIZE(binder_objstat_strings));
	BUILD_BUG_ON(ARRAY_SIZE(stats->obj_created) !=
		     ARRAY_SIZE(stats->obj_deleted));
	for (i = 0; i < ARRAY_SIZE(stats->obj_created); i++) {
		if (stats->obj_created[i] || stats->obj_deleted[i])
			seq_printf(m, "%s%s: active %d total %d\n", prefix,
				binder_objstat_strings[i],
				stats->obj_created[i] - stats->obj_deleted[i],
				stats->obj_created[i]);
	}
}

#ifdef BINDER_PERF_EVAL
static void print_binder_timeout_stats(struct seq_file *m, const char *prefix,
			       struct binder_timeout_stats *to_stats)
{
	int i, j;

	BUILD_BUG_ON(ARRAY_SIZE(to_stats->bto) !=
		     (ARRAY_SIZE(binder_wait_on_str) - 1));
	for (i = 0; i < ARRAY_SIZE(to_stats->bto); i++) 
	{
		if (to_stats->bto[i])
			seq_printf(m, "%s%s: %lu\n", prefix,
				   binder_wait_on_str[i + 1], to_stats->bto[i]);
	}
	for (i = 0, j = 0; i < ARRAY_SIZE(to_stats->read_t); i++) 
	{
		struct timespec sub_t = to_stats->read_t[i];
		if (sub_t.tv_sec) 
		{
			j++;
			if (!i)
				seq_printf(m, "%s%s:  timeout total time list:\n", 
					prefix, binder_wait_on_str[WAIT_ON_READ]);

			seq_printf(m, "  %s%u.%03ld", prefix,
				(unsigned)sub_t.tv_sec, (sub_t.tv_nsec / NSEC_PER_MSEC));
		}
	}
	if (j)
		seq_printf(m, "\n");
	for (i = 0, j = 0; i < ARRAY_SIZE(to_stats->exec_t); i++) 
	{
		struct timespec sub_t = to_stats->exec_t[i];
		if (sub_t.tv_sec)
		{
			j++;
			if (!i)
				seq_printf(m, "%s%s:  timeout total time list:\n", 
					prefix, binder_wait_on_str[WAIT_ON_EXEC]);

			seq_printf(m, "  %s%u.%03ld", prefix,
				(unsigned)sub_t.tv_sec, (sub_t.tv_nsec / NSEC_PER_MSEC));
		}
	}
	if (j)
		seq_printf(m, "\n");
	for (i = 0, j = 0; i < ARRAY_SIZE(to_stats->rrply_t); i++) 
	{
		struct timespec sub_t = to_stats->rrply_t[i];
		if (sub_t.tv_sec)
		{
			j++;
			if (!i)
				seq_printf(m, "%s%s:  timeout total time list:\n", 
					prefix, binder_wait_on_str[WAIT_ON_REPLY_READ]);

			seq_printf(m, "  %s%u.%03ld", prefix,
				(unsigned)sub_t.tv_sec, (sub_t.tv_nsec / NSEC_PER_MSEC));
		}
	}
	if (j)
		seq_printf(m, "\n");
}

static void print_binder_proc_perf_timeout_stats(struct seq_file *m,
				    struct binder_proc *proc)
{
	struct rb_node *n;
	int i;

	int proc_to_counter = 0;
	for (i = 0; i < ARRAY_SIZE(proc->to_stats.bto); i++)
	{
		proc_to_counter += proc->to_stats.bto[i];
		if (proc_to_counter > 0)
			break;
	}
	if (proc_to_counter > 0)
	{
		seq_printf(m, "proc %d(%s) timeout stats:\n",
				proc->pid, (proc->tsk != NULL) ? proc->tsk->comm : "");
		print_binder_timeout_stats(m, "  ", &proc->to_stats);

		seq_printf(m, "  threads timeout stats:\n");
		for (n = rb_first(&proc->threads); n != NULL; n = rb_next(n)) 
		{
			int thread_to_counter = 0;
			struct binder_thread *thread = rb_entry(n, struct binder_thread, rb_node);
			for (i = 0; i < ARRAY_SIZE(thread->to_stats.bto); i++)
			{
				thread_to_counter += thread->to_stats.bto[i];
				if (thread_to_counter > 0)
					break;
			}
			if (thread_to_counter > 0)
			{
				seq_printf(m, "  thread: %d\n", thread->pid);
				print_binder_timeout_stats(m, "    ", &thread->to_stats);
			}
		}
	}

}

#endif

static void print_binder_proc_stats(struct seq_file *m,
				    struct binder_proc *proc)
{
	struct binder_work *w;
	struct rb_node *n;
	int count, strong, weak;

	seq_printf(m, "proc %d\n", proc->pid);
	count = 0;
	for (n = rb_first(&proc->threads); n != NULL; n = rb_next(n))
		count++;
	seq_printf(m, "  threads: %d\n", count);
	seq_printf(m, "  requested threads: %d+%d/%d\n"
			"  ready threads %d\n"
			"  free async space %zd\n", proc->requested_threads,
			proc->requested_threads_started, proc->max_threads,
			proc->ready_threads, proc->free_async_space);
	count = 0;
	for (n = rb_first(&proc->nodes); n != NULL; n = rb_next(n))
		count++;
	seq_printf(m, "  nodes: %d\n", count);
	count = 0;
	strong = 0;
	weak = 0;
	for (n = rb_first(&proc->refs_by_desc); n != NULL; n = rb_next(n)) {
		struct binder_ref *ref = rb_entry(n, struct binder_ref,
						  rb_node_desc);
		count++;
		strong += ref->strong;
		weak += ref->weak;
	}
	seq_printf(m, "  refs: %d s %d w %d\n", count, strong, weak);

	count = 0;
	for (n = rb_first(&proc->allocated_buffers); n != NULL; n = rb_next(n))
		count++;
	seq_printf(m, "  buffers: %d\n", count);

	count = 0;
	list_for_each_entry(w, &proc->todo, entry) {
		switch (w->type) {
		case BINDER_WORK_TRANSACTION:
			count++;
			break;
		default:
			break;
		}
	}
	seq_printf(m, "  pending transactions: %d\n", count);

	print_binder_stats(m, "  ", &proc->stats);
}


static int binder_state_show(struct seq_file *m, void *unused)
{
	struct binder_proc *proc;
	struct hlist_node *pos;
	struct binder_node *node;
	int do_lock = !binder_debug_no_lock;

	if (do_lock)
		binder_lock(__func__);

	seq_puts(m, "binder state:\n");

	if (!hlist_empty(&binder_dead_nodes))
		seq_puts(m, "dead nodes:\n");
	hlist_for_each_entry(node, pos, &binder_dead_nodes, dead_node)
		print_binder_node(m, node);

	hlist_for_each_entry(proc, pos, &binder_procs, proc_node)
		print_binder_proc(m, proc, 1);
	if (do_lock)
		binder_unlock(__func__);
	return 0;
}

static int binder_stats_show(struct seq_file *m, void *unused)
{
	struct binder_proc *proc;
	struct hlist_node *pos;
	int do_lock = !binder_debug_no_lock;

	if (do_lock)
		binder_lock(__func__);

	seq_puts(m, "binder stats:\n");

	print_binder_stats(m, "", &binder_stats);

	hlist_for_each_entry(proc, pos, &binder_procs, proc_node)
		print_binder_proc_stats(m, proc);
	if (do_lock)
		binder_unlock(__func__);
	return 0;
}

#ifdef BINDER_PERF_EVAL
static int binder_perf_stats_show(struct seq_file *m, void *unused)
{
	struct binder_proc *proc;
	struct hlist_node *pos;
	int do_lock = !binder_debug_no_lock;

	if (do_lock)
		binder_lock(__func__);

	seq_puts(m, "binder stats:\n");
	//print_binder_stats(m, "", &binder_stats);

	if (binder_perf_evalue & BINDER_PERF_SEND_COUNTER)
	{
		seq_puts(m, "binder send transaction stats:\n");
		hlist_for_each_entry(proc, pos, &binder_procs, proc_node)
		{
			if (proc->bc_t)
			{
				int i, j;
				seq_printf(m, "proc %d(%s): %d\n", proc->pid,
					proc->tsk ? proc->tsk->comm : "",
					proc->bc_t);
				for (i = 0; i < BC_STATS_NR; i++)
				{
					if (proc->bc_stats[i] == NULL)
						break;
					if (!strcmp(proc->bc_stats[i]->service, "") &&
							(0 == proc->bc_stats[i]->code[0]))
						break;
					seq_printf(m, "  service %s\n", proc->bc_stats[i]->service);
					for (j = 0; j < BC_CODE_NR; j++)
					{
						if (0 == proc->bc_stats[i]->code_num[j])
							break;
						seq_printf(m, "    dex %d: %d\n", proc->bc_stats[i]->code[j], proc->bc_stats[i]->code_num[j]);
					}
				}

			}
		}

	}
	
	if (binder_perf_evalue & BINDER_PERF_TIMEOUT_COUNTER)
	{
		seq_puts(m, "binder transaction time out stats:\n");
		hlist_for_each_entry(proc, pos, &binder_procs, proc_node)
			print_binder_proc_perf_timeout_stats(m,proc);
	}
	if (do_lock)
		binder_unlock(__func__);
	return 0;
}
#endif
static int binder_transactions_show(struct seq_file *m, void *unused)
{
	struct binder_proc *proc;
	struct hlist_node *pos;
	int do_lock = !binder_debug_no_lock;

	if (do_lock)
		binder_lock(__func__);

	seq_puts(m, "binder transactions:\n");
	hlist_for_each_entry(proc, pos, &binder_procs, proc_node)
		print_binder_proc(m, proc, 0);
	if (do_lock)
		binder_unlock(__func__);
	return 0;
}

static int binder_proc_show(struct seq_file *m, void *unused)
{
	struct binder_proc *proc = m->private;
	int do_lock = !binder_debug_no_lock;
#ifdef MTK_BINDER_DEBUG
	struct binder_proc *tmp_proc;
	struct hlist_node *pos;
	bool find = false;
#endif

	if (do_lock)
		binder_lock(__func__);
	seq_puts(m, "binder proc state:\n");
#ifdef MTK_BINDER_DEBUG
	hlist_for_each_entry(tmp_proc, pos, &binder_procs, proc_node)
	{
		if (proc == tmp_proc)
		{
			find = true;
			break;
		}
	}
	if (find == true)
#endif
		print_binder_proc(m, proc, 1);
#ifdef MTK_BINDER_DEBUG
	else
		printk(KERN_INFO "binder: show proc addr 0x%p exit\n", proc);
#endif
	if (do_lock)
		binder_unlock(__func__);
	return 0;
}

static void print_binder_transaction_log_entry(struct seq_file *m,
					struct binder_transaction_log_entry *e)
{
#ifdef BINDER_MONITOR
	char tmp[30];
	struct rtc_time tm;
	struct timespec sub_read_t, sub_total_t;
	unsigned long read_ms =0;
	unsigned long total_ms = 0;

	if (e->fd != -1)
		sprintf(tmp, " (fd %d)", e->fd);
	else
		tmp[0] = '\0';

	if ((e->call_type == 0) && timespec_valid(&e->endstamp) &&
			(timespec_compare(&e->endstamp, &e->timestamp) > 0)) {
		sub_total_t = timespec_sub(e->endstamp, e->timestamp);
		total_ms = ((unsigned long)sub_total_t.tv_sec)*MSEC_PER_SEC +
			sub_total_t.tv_nsec / NSEC_PER_MSEC;
	} 
	if ((e->call_type == 1) && timespec_valid(&e->readstamp) &&
			(timespec_compare(&e->readstamp, &e->timestamp) > 0)) {
		sub_read_t = timespec_sub(e->readstamp, e->timestamp);
		read_ms = ((unsigned long)sub_read_t.tv_sec)*MSEC_PER_SEC +
			sub_read_t.tv_nsec / NSEC_PER_MSEC;
	}
		
	rtc_time_to_tm(e->tv.tv_sec, &tm);
	seq_printf(m,
		   "%d: %s from %d:%d to %d:%d node %d handle %d (%s) size %d:%d%s "
		   "dex %d start %lu.%06lu android %d-%02d-%02d %02d:%02d:%02d.%03lu read %lu.%06lu %s %lu.%06lu total %lu.%06lums\n",
		   e->debug_id, (e->call_type == 2) ? "reply" :
		                 ((e->call_type == 1) ? "async" : "call "),
		   e->from_proc, e->from_thread, e->to_proc, e->to_thread,
		   e->to_node, e->target_handle, e->service,
		   e->data_size, e->offsets_size, tmp, e->code,
		   (unsigned long)e->timestamp.tv_sec,
		   (e->timestamp.tv_nsec / NSEC_PER_USEC),
		   (tm.tm_year + 1900), (tm.tm_mon + 1), tm.tm_mday,
		   tm.tm_hour, tm.tm_min, tm.tm_sec,
		   (unsigned long)(e->tv.tv_usec / USEC_PER_MSEC),
		   (unsigned long)e->readstamp.tv_sec,
		   (e->readstamp.tv_nsec / NSEC_PER_USEC),
		   (e->call_type == 0) ? "end" : "",
		   (e->call_type == 0) ? ((unsigned long)e->endstamp.tv_sec) : 0,
		   (e->call_type == 0) ? (e->endstamp.tv_nsec / NSEC_PER_USEC) : 0,
		   (e->call_type == 0) ? total_ms : read_ms,
		   (e->call_type == 0) ? (sub_total_t.tv_nsec % NSEC_PER_MSEC) :
		   (sub_read_t.tv_nsec % NSEC_PER_MSEC));
#else
	seq_printf(m,
		   "%d: %s from %d:%d to %d:%d node %d handle %d size %d:%d\n",
		   e->debug_id, (e->call_type == 2) ? "reply" :
		   ((e->call_type == 1) ? "async" : "call "), e->from_proc,
		   e->from_thread, e->to_proc, e->to_thread, e->to_node,
		   e->target_handle, e->data_size, e->offsets_size);
#endif
}

#ifdef BINDER_MONITOR
static void log_resume_func(struct work_struct *w)
{
	printk(KERN_INFO "binder: transaction log is self resumed\n");
	log_disable = 0;
}

static DECLARE_DELAYED_WORK(log_resume_work, log_resume_func);

static int binder_transaction_log_show(struct seq_file *m, void *unused)
{
	struct binder_transaction_log *log = m->private;
	int i;

	if (!log->entry)
		return 0;

	if (log->full) {
		for (i = log->next; i < log->size; i++)
			print_binder_transaction_log_entry(m, &log->entry[i]);
	}
	for (i = 0; i < log->next; i++)
		print_binder_transaction_log_entry(m, &log->entry[i]);

	if (log_disable & BINDER_LOG_RESUME) {
		printk(KERN_INFO "binder: %d (%s) read transaction log and "
				 "resume\n",
				 task_pid_nr(current), current->comm);
		cancel_delayed_work(&log_resume_work);
		log_disable = 0;
	}
	return 0;
}
#else

static int binder_transaction_log_show(struct seq_file *m, void *unused)
{
	struct binder_transaction_log *log = m->private;
	int i;

	if (log->full) {
		for (i = log->next; i < ARRAY_SIZE(log->entry); i++)
			print_binder_transaction_log_entry(m, &log->entry[i]);
	}
	for (i = 0; i < log->next; i++)
		print_binder_transaction_log_entry(m, &log->entry[i]);
	return 0;
}
#endif

static const struct file_operations binder_fops = {
	.owner = THIS_MODULE,
	.poll = binder_poll,
	.unlocked_ioctl = binder_ioctl,
	.mmap = binder_mmap,
	.open = binder_open,
	.flush = binder_flush,
	.release = binder_release,
};

static struct miscdevice binder_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "binder",
	.fops = &binder_fops
};

#ifdef BINDER_PERF_EVAL
static void binder_perf_timeout_zero_init(struct binder_timeout_stats *to_stats)
{
	memset(&to_stats->bto[0], 0, sizeof(to_stats->bto));
	memset(&to_stats->read_t[0], 0, sizeof(to_stats->read_t));
	memset(&to_stats->exec_t[0], 0, sizeof(to_stats->exec_t));
	memset(&to_stats->rrply_t[0], 0, sizeof(to_stats->rrply_t));
}


static void binder_perf_stats_timeout_clean(void)
{
	struct binder_proc *proc;
	struct hlist_node *pos;
	struct rb_node *n;
	int do_lock = !binder_debug_no_lock;

	if (do_lock)
		binder_lock(__func__);
	hlist_for_each_entry(proc, pos, &binder_procs, proc_node)
	{
		binder_perf_timeout_zero_init(&proc->to_stats);
		for (n = rb_first(&proc->threads); n != NULL; n = rb_next(n)) 
		{
			struct binder_thread *thread = rb_entry(n, struct binder_thread, rb_node);
			binder_perf_timeout_zero_init(&thread->to_stats);
		}
	}
	if (do_lock)
		binder_unlock(__func__);
}

static void binder_perf_stats_bct_clean(void)
{
	struct binder_proc *proc;
	struct hlist_node *pos;
	int do_lock = !binder_debug_no_lock;

	if (do_lock)
		binder_lock(__func__);
	hlist_for_each_entry(proc, pos, &binder_procs, proc_node)
	{
		int i;
		proc->bc_t = 0;
		for (i = 0; i < BC_STATS_NR; i++)
		{
			if (proc->bc_stats[i] != NULL) {
				kfree(proc->bc_stats[i]);
				proc->bc_stats[i] = NULL;
			}
		}
	}
	if (do_lock)
		binder_unlock(__func__);
}

static int binder_perf_evalue_show(struct seq_file *m, void *unused)
{
	seq_printf(m, " Current binder performance evalue is: %u\n", binder_perf_evalue);
	return 0;
}

static ssize_t binder_perf_evalue_write(struct file *filp, const char *ubuf,
				      size_t cnt, loff_t *data)
{
	char buf[32];
	size_t copy_size = cnt;
	unsigned long val;
	int ret;
 
	if (cnt >= sizeof(buf))
		copy_size = 32 - 1;
	buf[copy_size] = '\0';

	if (copy_from_user(&buf, ubuf, copy_size))
		return -EFAULT;

	printk("[Binder] Set binder perf evalue:%u -> ", binder_perf_evalue);
	ret = strict_strtoul(buf, 10, &val);
	if (ret < 0 ) {
		printk("Null\ninvalid string, need number foramt, err:%d \n",ret);
		printk("perf evalue level:   0  ---- 3 \n");
		printk("           	   Less ---- More\n");
		return cnt; //string to unsined long fail
	}
	printk("%lu\n", val);
	if (val < 4) {
		binder_perf_evalue = val;
		if (0 == (val & BINDER_PERF_SEND_COUNTER))
			binder_perf_stats_bct_clean();
		if (0 == (val & BINDER_PERF_TIMEOUT_COUNTER))
			binder_perf_stats_timeout_clean();
	} else {
		printk("invalid value:%lu, should be 0 ~ 3\n", val);
	}
	printk(KERN_INFO "binder: %d (%s) set performance evaluate type %s %s\n",
			 task_pid_nr(current), current->comm,
			 (binder_perf_evalue & BINDER_PERF_SEND_COUNTER) ? "sender counter enable" : "",
			 (binder_perf_evalue & BINDER_PERF_TIMEOUT_COUNTER) ?
				"time out counter enable" : "");

	return cnt;
}
#endif
#ifdef BINDER_MONITOR
static int binder_log_level_show(struct seq_file *m, void *unused)
{
	seq_printf(m, " Current log level: %lu\n", binder_log_level);
	return 0;
}

static ssize_t binder_log_level_write(struct file *filp, const char *ubuf,
				      size_t cnt, loff_t *data)
{
	char buf[32];
	size_t copy_size = cnt;
	unsigned long val;
	int ret;
 
	if (cnt >= sizeof(buf))
		copy_size = 32 - 1;
	buf[copy_size] = '\0';

	if (copy_from_user(&buf, ubuf, copy_size))
		return -EFAULT;

	printk("[Binder] Set binder log level:%lu -> ", binder_log_level);
	ret = strict_strtoul(buf, 10, &val);
	if (ret < 0) {
		printk("Null\ninvalid string, need number foramt, err:%d \n",ret);
		printk("Log Level:   0  ---- 4 \n");
		printk("           Less ---- More\n");
		return cnt; //string to unsined long fail
	}
	printk("%lu\n", val);
	if (val == 0) {
		binder_debug_mask = 
			BINDER_DEBUG_USER_ERROR | BINDER_DEBUG_FAILED_TRANSACTION |
			BINDER_DEBUG_DEAD_TRANSACTION ;
		binder_log_level = val;
	} else if (val == 1) {
		binder_debug_mask = 
			BINDER_DEBUG_USER_ERROR | BINDER_DEBUG_FAILED_TRANSACTION |
			BINDER_DEBUG_DEAD_TRANSACTION | BINDER_DEBUG_DEAD_BINDER |
			BINDER_DEBUG_DEATH_NOTIFICATION ;
		binder_log_level = val;
	} else if (val == 2) {
		binder_debug_mask =
			BINDER_DEBUG_USER_ERROR | BINDER_DEBUG_FAILED_TRANSACTION |
			BINDER_DEBUG_DEAD_TRANSACTION | BINDER_DEBUG_DEAD_BINDER |
			BINDER_DEBUG_DEATH_NOTIFICATION |BINDER_DEBUG_THREADS |
			BINDER_DEBUG_TRANSACTION | BINDER_DEBUG_TRANSACTION_COMPLETE ;
		binder_log_level = val;
	} else if (val == 3) {
		binder_debug_mask = 
			BINDER_DEBUG_USER_ERROR | BINDER_DEBUG_FAILED_TRANSACTION |
			BINDER_DEBUG_DEAD_TRANSACTION | BINDER_DEBUG_DEAD_BINDER |
			BINDER_DEBUG_DEATH_NOTIFICATION | BINDER_DEBUG_THREADS |
			BINDER_DEBUG_TRANSACTION | BINDER_DEBUG_TRANSACTION_COMPLETE |
			BINDER_DEBUG_OPEN_CLOSE | BINDER_DEBUG_READ_WRITE ;
		binder_log_level = val;
	} else if (val == 4) {
		binder_debug_mask = 
			BINDER_DEBUG_USER_ERROR | BINDER_DEBUG_FAILED_TRANSACTION |
			BINDER_DEBUG_DEAD_TRANSACTION |	BINDER_DEBUG_DEAD_BINDER |
			BINDER_DEBUG_DEATH_NOTIFICATION | BINDER_DEBUG_THREADS |
			BINDER_DEBUG_OPEN_CLOSE | BINDER_DEBUG_READ_WRITE |
			BINDER_DEBUG_TRANSACTION | BINDER_DEBUG_TRANSACTION_COMPLETE |
			BINDER_DEBUG_USER_REFS | BINDER_DEBUG_INTERNAL_REFS |
			BINDER_DEBUG_PRIORITY_CAP |BINDER_DEBUG_FREE_BUFFER |
			BINDER_DEBUG_BUFFER_ALLOC ;
		binder_log_level = val;
	} else {
		printk("invalid value:%lu, should be 0 ~ 4\n", val);
	}
	return cnt;
}

static void print_binder_timeout_log_entry(struct seq_file *m,
					   struct binder_timeout_log_entry *e)
{
	struct rtc_time tm;

	rtc_time_to_tm(e->tv.tv_sec, &tm);
	seq_printf(m, "%d:%s %d:%d to %d:%d spends %u000 ms (%s) dex_code %u "
		      "start_at %lu.%03ld android %d-%02d-%02d %02d:%02d:%02d.%03lu\n",
		      e->r, binder_wait_on_str[e->r],
		      e->from_proc, e->from_thrd, e->to_proc, e->to_thrd,
		      e->over_sec, e->service, e->code,
		      (unsigned long)e->ts.tv_sec,
		      (e->ts.tv_nsec / NSEC_PER_MSEC),
		      (tm.tm_year + 1900), (tm.tm_mon + 1), tm.tm_mday,
		      tm.tm_hour, tm.tm_min, tm.tm_sec,
		      (unsigned long)(e->tv.tv_usec / USEC_PER_MSEC));
}

static int binder_timeout_log_show(struct seq_file *m, void *unused)
{
	struct binder_timeout_log *log = m->private;
	int i, latest;
	int end_idx = ARRAY_SIZE(log->entry) - 1;

	binder_lock(__func__);

	latest = log->next ? (log->next - 1) : end_idx;
	if (log->next == 0 && !log->full)
		goto timeout_log_show_unlock;

	if (latest >= ARRAY_SIZE(log->entry) || latest < 0) {
		int j;

		printk(KERN_ALERT "binder: timeout log index error, "
				  "log %p latest %d next %d end_idx %d\n",
				  log, latest, log->next, end_idx);
		for (j = -4; j <= 3; j++) {
			unsigned int *tmp = (unsigned int *)log + (j * 8);
			printk(KERN_ALERT "binder: 0x%08x %08x %08x %08x %08x  "
					  "%08x %08x %08x %08x\n",
					  (unsigned int)tmp,
					  *tmp, *(tmp + 1), *(tmp + 2), *(tmp + 3),
					  *(tmp + 4), *(tmp + 5), *(tmp + 6),
					  *(tmp + 7));
		}
		aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_SWT_JBT_TRACES|DB_OPT_BINDER_INFO, "binder: timeout log index error",
				     "detect for memory corruption\n\n"
				     "check kernel log for more details\n");
		goto timeout_log_show_unlock;
	}

	for (i = latest; i >= 0; i--)
		print_binder_timeout_log_entry(m, &log->entry[i]);
	if (log->full) {
		for (i = end_idx; i > latest; i--)
			print_binder_timeout_log_entry(m, &log->entry[i]);
	}

timeout_log_show_unlock:
	binder_unlock(__func__);
	return 0;
}

BINDER_DEBUG_SETTING_ENTRY(log_level);
#ifdef BINDER_PERF_EVAL
BINDER_DEBUG_SETTING_ENTRY(perf_evalue);
#endif
BINDER_DEBUG_ENTRY(timeout_log);
#ifdef BINDER_PERF_EVAL
BINDER_DEBUG_ENTRY(perf_stats);
#endif

static int binder_transaction_log_enable_show(struct seq_file *m, void *unused)
{
#ifdef BINDER_MONITOR
	seq_printf(m, " Current transaciton log is %s %s %s"
#ifdef RT_PRIO_INHERIT
		       " %s"
#endif
		       "\n",
			(log_disable & 0x1) ? "disabled" : "enabled",
			(log_disable & BINDER_LOG_RESUME) ? "(self resume)" : "",
			(log_disable & BINDER_BUF_WARN) ? "(buf warning enabled)" : ""
#ifdef RT_PRIO_INHERIT
			
			,(log_disable & BINDER_RT_LOG_ENABLE) ? "(rt inherit log enabled)" : ""
#endif
			);
#else
	seq_printf(m, " Current transaciton log is %s %s\n",
		      log_disable ? "disabled" : "enabled",
		      (log_disable & BINDER_LOG_RESUME) ? "(self resume)" : "");
#endif
	return 0;
}

static ssize_t binder_transaction_log_enable_write(struct file *filp,
						   const char *ubuf,
						   size_t cnt, loff_t *data)
{
	char buf[32];
	size_t copy_size = cnt;
	unsigned long val;
	int ret;

	if (cnt >= sizeof(buf))
		copy_size = 32 - 1;

	buf[copy_size] = '\0';

	if (copy_from_user(&buf, ubuf, copy_size))
		return -EFAULT;

    	ret = strict_strtoul(buf, 10, &val);
	if (ret < 0) {
		printk(KERN_INFO "binder: failed to switch logging, "
				 "need number format\n");
		return cnt;
	}

	log_disable = !(val & 0x1);
	if (log_disable && (val & BINDER_LOG_RESUME)) {
		log_disable |= BINDER_LOG_RESUME;
		queue_delayed_work(binder_deferred_workqueue,
				   &log_resume_work, (120 * HZ));
	}
#ifdef BINDER_MONITOR
	if (val & BINDER_BUF_WARN) {
		log_disable |= BINDER_BUF_WARN;
	}
#ifdef RT_PRIO_INHERIT
	if (val & BINDER_RT_LOG_ENABLE) {
		log_disable |= BINDER_RT_LOG_ENABLE;
	}
#endif
	printk(KERN_INFO "binder: %d (%s) set transaction log %s %s %s"
#ifdef RT_PRIO_INHERIT
			" %s"
#endif
			"\n",
			task_pid_nr(current), current->comm,
			(log_disable & 0x1) ? "disabled" : "enabled",
			(log_disable & BINDER_LOG_RESUME) ?
			"(self resume)" : "",
			(log_disable & BINDER_BUF_WARN) ? "(buf warning)" : ""
#ifdef RT_PRIO_INHERIT
			,(log_disable & BINDER_RT_LOG_ENABLE) ? "(rt inherit log enabled)" : ""
#endif
	      );
#else
	printk(KERN_INFO "binder: %d (%s) set transaction log %s %s\n",
			 task_pid_nr(current), current->comm,
			 log_disable ? "disabled" : "enabled",
			 (log_disable & BINDER_LOG_RESUME) ?
				"(self resume)" : "");
#endif
	return cnt;
}
BINDER_DEBUG_SETTING_ENTRY(transaction_log_enable);
# endif

#ifdef MTK_BINDER_PAGE_USED_RECORD
static int binder_page_used_show(struct seq_file *s, void *p)
{
	struct binder_proc *proc;
	struct hlist_node *pos;
	int do_lock = !binder_debug_no_lock;
	seq_printf(s, "page_used:%d[%dMB]\npage_used_peak:%d[%dMB]\n",
	               binder_page_used, binder_page_used>>8,
	               binder_page_used_peak, binder_page_used_peak>>8);

	if (do_lock)
		binder_lock(__func__);
	seq_puts(s, "binder page stats by binder_proc:\n");
	hlist_for_each_entry(proc, pos, &binder_procs, proc_node)
	{
		seq_printf(s, "    proc %d(%s):page_used:%d[%dMB] page_used_peak:%d[%dMB]\n",
				proc->pid, proc->tsk ? proc->tsk->comm : " ",
				proc->page_used, proc->page_used>>8,
				proc->page_used_peak, proc->page_used_peak>>8);
	}
	if (do_lock)
		binder_unlock(__func__);

	return 0;
}

BINDER_DEBUG_ENTRY(page_used);
#endif

BINDER_DEBUG_ENTRY(state);
BINDER_DEBUG_ENTRY(stats);
BINDER_DEBUG_ENTRY(transactions);
BINDER_DEBUG_ENTRY(transaction_log);

static int __init binder_init(void)
{
	int ret;
#ifdef BINDER_MONITOR
	struct task_struct *th;


	th = kthread_create(binder_bwdog_thread, NULL, "binder_watchdog");
	if (IS_ERR(th)) {
		printk(KERN_INFO "binder: fail to create watchdog thread "
				 "(err:%li)\n", PTR_ERR(th));
	} else {
		wake_up_process(th);
	}

	binder_transaction_log_failed.entry = &entry_failed[0];
	binder_transaction_log_failed.size  = ARRAY_SIZE(entry_failed);

#ifdef CONFIG_MTK_EXTMEM
	binder_transaction_log.entry = extmem_malloc_page_align(sizeof(struct binder_transaction_log_entry)*MAX_ENG_TRANS_LOG_BUFF_LEN);
	binder_transaction_log.size  = MAX_ENG_TRANS_LOG_BUFF_LEN;

	if(binder_transaction_log.entry == NULL)
		panic("%s[%s] memory alloc failed!!!\n", __FILE__, __FUNCTION__);
#else
	binder_transaction_log.entry = &entry_t[0];
	binder_transaction_log.size  = ARRAY_SIZE(entry_t);
#endif    
#endif

	binder_deferred_workqueue = create_singlethread_workqueue("binder");
	if (!binder_deferred_workqueue)
		return -ENOMEM;

	binder_debugfs_dir_entry_root = debugfs_create_dir("binder", NULL);
	if (binder_debugfs_dir_entry_root)
		binder_debugfs_dir_entry_proc = debugfs_create_dir("proc",
						 binder_debugfs_dir_entry_root);
	ret = misc_register(&binder_miscdev);
	if (binder_debugfs_dir_entry_root) {
		debugfs_create_file("state",
				    S_IRUGO,
				    binder_debugfs_dir_entry_root,
				    NULL,
				    &binder_state_fops);
		debugfs_create_file("stats",
				    S_IRUGO,
				    binder_debugfs_dir_entry_root,
				    NULL,
				    &binder_stats_fops);
		debugfs_create_file("transactions",
				    S_IRUGO,
				    binder_debugfs_dir_entry_root,
				    NULL,
				    &binder_transactions_fops);
		debugfs_create_file("transaction_log",
				    S_IRUGO,
				    binder_debugfs_dir_entry_root,
				    &binder_transaction_log,
				    &binder_transaction_log_fops);
		debugfs_create_file("failed_transaction_log",
				    S_IRUGO,
				    binder_debugfs_dir_entry_root,
				    &binder_transaction_log_failed,
				    &binder_transaction_log_fops);
#ifdef BINDER_MONITOR
		/* system_server is the main writer, remember to
		 * change group as "system" for write permission
		 * via related init.rc */
		debugfs_create_file("transaction_log_enable",
				    (S_IRUGO | S_IWUSR | S_IWGRP),
				    binder_debugfs_dir_entry_root,
				    NULL,
				    &binder_transaction_log_enable_fops);
		debugfs_create_file("log_level",
				    (S_IRUGO | S_IWUSR | S_IWGRP),
				    binder_debugfs_dir_entry_root,
				    NULL,
				    &binder_log_level_fops);
		debugfs_create_file("timeout_log",
				    S_IRUGO,
				    binder_debugfs_dir_entry_root,
				    &binder_timeout_log_t,
				    &binder_timeout_log_fops);
#endif
#ifdef BINDER_PERF_EVAL
		debugfs_create_file("perf_evalue",
				    (S_IRUGO | S_IWUSR | S_IWGRP),
				    binder_debugfs_dir_entry_root,
				    NULL,
				    &binder_perf_evalue_fops);
		debugfs_create_file("perf_stats",
				    S_IRUGO,
				    binder_debugfs_dir_entry_root,
				    NULL,
				    &binder_perf_stats_fops);

#endif
#ifdef MTK_BINDER_PAGE_USED_RECORD
        debugfs_create_file("page_used",
                    S_IRUGO,
                    binder_debugfs_dir_entry_root,
                    NULL,
                    &binder_page_used_fops);
#endif
	}
	return ret;
}

device_initcall(binder_init);

#define CREATE_TRACE_POINTS
#include "binder_trace.h"

MODULE_LICENSE("GPL v2");
