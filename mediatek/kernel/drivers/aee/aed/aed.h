#ifndef __aed_h
#define __aed_h

#include <generated/autoconf.h>
#include <linux/types.h>
#include <linux/bug.h>
#include <linux/aee.h>
#include <linux/kallsyms.h>
#include <linux/xlog.h>
#include <linux/ptrace.h>


#define AE_INVALID              0xAEEFF000
#define AE_NOT_AVAILABLE        0xAEE00000
#define AE_DEFAULT              0xAEE00001

typedef enum {
    AEE_MODE_MTK_ENG = 1,
    AEE_MODE_MTK_USER,
    AEE_MODE_CUSTOMER_ENG,
    AEE_MODE_CUSTOMER_USER 
} AEE_MODE;

typedef enum {
    AEE_FORCE_DISABLE_RED_SCREEN = 0,
    AEE_FORCE_RED_SCREEN,
    AEE_FORCE_NOT_SET
} AEE_FORCE_RED_SCREEN_VALUE;

typedef enum {
    AE_SUCC, 
    AE_FAIL
} AE_ERR;

typedef enum {
    AE_PASS_BY_MEM,
    AE_PASS_BY_FILE,
    AE_PASS_METHOD_END  
} AE_PASS_METHOD;

typedef enum {AE_REQ, AE_RSP, AE_IND, AE_CMD_TYPE_END}        AE_CMD_TYPE;

typedef enum  {
    AE_REQ_IDX,

    AE_REQ_CLASS,
    AE_REQ_TYPE,
    AE_REQ_PROCESS,
    AE_REQ_MODULE,
    AE_REQ_BACKTRACE,
    AE_REQ_DETAIL,    /* Content of response message rule:
                       *   if msg.arg1==AE_PASS_BY_FILE => msg->data=file path 
                       */
    
    AE_REQ_ROOT_LOG_DIR,
    AE_REQ_CURR_LOG_DIR,
    AE_REQ_DFLT_LOG_DIR,
    AE_REQ_MAIN_LOG_FILE_PATH,    

    AE_IND_FATAL_RAISED, // fatal event raised, indicate AED to notify users
    AE_IND_EXP_RAISED,   // exception event raised, indicate AED to notify users      
    AE_IND_WRN_RAISED,   // warning event raised, indicate AED to notify users    
    AE_IND_REM_RAISED,   // reminding event raised, indicate AED to notify users    

    AE_IND_LOG_STATUS, // arg = AE_ERR
    AE_IND_LOG_CLOSE,  // arg = AE_ERR

    AE_REQ_SWITCH_DAL_BEEP, // arg: dal on|off, seq: beep on|off
    AE_REQ_DB_COUNT, 		// arg: db count
    AE_REQ_DB_FORCE_PATH,   // arg: force db path yes\no
    AE_REQ_SWITCH_EXP_LEVEL,
    AE_REQ_IS_AED_READY,    // query if AED is ready for service
    AE_REQ_COREDUMP,        // msg->data=file path
    AE_REQ_SET_READFLAG,    // set read flag msg
    AE_REQ_E2S_INIT,        // Init notification of client side(application layer) of Exp2Server
    AE_CMD_ID_END
} AE_CMD_ID;

typedef struct {
    AE_CMD_TYPE cmdType;  // command type
    AE_CMD_ID   cmdId;    // command Id
    union {
        unsigned int seq; // sequence number for error checking
        int pid;          // process id
    };
    union {
        unsigned int arg; // simple argument        
        AE_EXP_CLASS cls; // exception/error/defect class
    };
    union {
        unsigned int len; // dynamic length argument
        int id;           // desired id
    };
    unsigned int dbOption;// DB dump option
} AE_Msg;

/* Kernel IOCTL interface */
struct aee_dal_show {
	char msg[1024];
};

struct aee_dal_setcolor {
	unsigned int foreground;
	unsigned int background;
	unsigned int screencolor;
};

#define MAX_AEE_KERNEL_BT 16
#define MAX_AEE_KERNEL_SYMBOL 256

struct aee_process_bt {
	pid_t pid;
	
	int nr_entries;
	struct {
		unsigned long pc;
		unsigned long lr;

		char pc_symbol[MAX_AEE_KERNEL_SYMBOL];
		char lr_symbol[MAX_AEE_KERNEL_SYMBOL];
	} entries[MAX_AEE_KERNEL_BT];
};

struct aee_thread_reg {
	pid_t tid;
	struct pt_regs regs;
};

#define AEEIOCTL_DAL_SHOW       _IOW('p', 0x01, struct aee_dal_show) /* Show string on DAL layer  */
#define AEEIOCTL_DAL_CLEAN      _IO('p', 0x02) /* Clear DAL layer */
#define AEEIOCTL_SETCOLOR       _IOW('p', 0x03, struct aee_dal_setcolor) /* RGB color 0x00RRGGBB */
#define AEEIOCTL_GET_PROCESS_BT _IOW('p', 0x04, struct aee_process_bt)
#define AEEIOCTL_GET_SMP_INFO   _IOR('p', 0x05, int)
#define AEEIOCTL_SET_AEE_MODE   _IOR('p', 0x06, int)
#define AEEIOCTL_GET_THREAD_REG _IOW('p', 0x07, struct aee_thread_reg)
#define AEEIOCTL_CHECK_SUID_DUMPABLE _IOR('p', 0x08, int)
/* AED debug support */

#define AEEIOCTL_RT_MON_Kick _IOR('p', 0x0A, int)
#define AEEIOCTL_SET_FORECE_RED_SCREEN _IOR('p', 0x0B, int)
#define AED_FILE_OPS(entry) \
  static const struct file_operations proc_##entry##_fops = { \
    .read = proc_##entry##_read, \
    .write = proc_##entry##_write, \
  }

#define AED_FILE_OPS_RO(entry) \
  static const struct file_operations proc_##entry##_fops = { \
    .read = proc_##entry##_read, \
  }

#define  AED_PROC_ENTRY(name, entry, mode)\
  if (proc_create(#name, S_IFREG | mode, aed_proc_dir, &proc_##entry##_fops)) \
    xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "%s: proc_create %s failed \n", __func__, #name)


struct proc_dir_entry;

int aed_proc_debug_init(struct proc_dir_entry *aed_proc_dir);
int aed_proc_debug_done(struct proc_dir_entry *aed_proc_dir);

int aed_get_process_bt(struct aee_process_bt *bt);

void aee_rr_proc_init(struct proc_dir_entry *aed_proc_dir);
void aee_rr_proc_done(struct proc_dir_entry *aed_proc_dir);

void dram_console_init(struct proc_dir_entry *aed_proc_dir);
void dram_console_done(struct proc_dir_entry *aed_proc_dir);

#define AEK_LOG_TAG "aee/aek"

#endif
