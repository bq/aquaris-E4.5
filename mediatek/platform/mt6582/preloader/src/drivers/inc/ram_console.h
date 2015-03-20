
#ifndef __RAM_CONSOLE_H__
#define __RAM_CONSOLE_H__

#define RAM_CONSOLE_SIG (0x43474244) /* DBGC */

#define RC_CPU_COUNT 4
#define TASK_COMM_LEN 16

struct ram_console_buffer {
	u32    sig;
	u32    start;
	u32    size;

	u8     hw_status;
	u8     fiq_step;
	u8     shutdown_mode;
	u8     in_idle;
	u8     __pad3;

	u32    jiffies_current;
	u32    jiffies_idle;
	u32    jiffies_wdk_kick;
	u32    bin_log_count;

	u32    last_irq_enter[RC_CPU_COUNT];
	u64    jiffies_last_irq_enter[RC_CPU_COUNT];

	u32    last_irq_exit[RC_CPU_COUNT];
	u64    jiffies_last_irq_exit[RC_CPU_COUNT];

	u64    jiffies_last_sched[RC_CPU_COUNT];
	char   last_sched_comm[RC_CPU_COUNT][TASK_COMM_LEN];

	u8     data[0];
};

#endif // #ifndef __RAM_CONSOLE_H__


