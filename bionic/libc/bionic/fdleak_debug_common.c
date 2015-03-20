#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

/*
 * fdleak_debug_common.c only called by libc.so build
 */
#ifdef _FDLEAK_DEBUG_
#include <sys/system_properties.h>
#include <dlfcn.h>
#include "fdleak_debug_common.h"


static void* libc_fdleak_impl_handle = NULL;
extern char*  __progname;

#ifdef MTK_USE_RESERVED_EXT_MEM
typedef void (*FDLeakDebugPrepare)();
typedef void (*FDLeakDebugParent)();
typedef void (*FDLeakDebugChild)();

FDLeakDebugPrepare fdleak_debug_prepare = NULL;
FDLeakDebugParent fdleak_debug_parent = NULL;
FDLeakDebugChild fdleak_debug_child = NULL;
#endif

/* Initializes fdleak debug once per process. */
static void fdleak_init_impl(void) 
{
    const char* so_name = NULL;
    FDLeakDebugInit fdleak_debug_initialize = NULL;
    unsigned int fdleak_debug_enable = 0;
    char env[PROP_VALUE_MAX];
    char debug_program[PROP_VALUE_MAX];

    /* Default enable. */
#ifdef _MTK_ENG_
    fdleak_debug_enable = 1;
#else
    fdleak_debug_enable = 0;
#endif
    if (__system_property_get("persist.debug.fdleak", env)) {
        fdleak_debug_enable = atoi(env);
    }

    if(!fdleak_debug_enable) {
        return;
    }

    /* Process need bypass */
    if (strstr(__progname, "system/bin/aee")) {/*bypass aee and aee_dumpstate*/
        fdleak_debug_enable = 0;
    }
    
    /* Control for only one specific program to enable fdleak debug. */
    if (__system_property_get("persist.debug.fdleak.program", debug_program)) {
        if (strstr(__progname, debug_program)) {
            fdleak_debug_enable = 1;
        }
        else {
            fdleak_debug_enable = 0;
        }
    }
    
    if(!fdleak_debug_enable) {
        return;
    }

    so_name = "/system/lib/libc_fdleak_debug_mtk.so";
    libc_fdleak_impl_handle = dlopen(so_name, RTLD_LAZY);
    if (libc_fdleak_impl_handle == NULL) {
        fdleak_error_log("%s: Missing module %s required for fdleak debug %d\n",
                 __progname, so_name, fdleak_debug_enable);
        return;
    }

    /* Initialize fdleak debugging in the loaded module.*/
    fdleak_debug_initialize =
            dlsym(libc_fdleak_impl_handle, "fdleak_debug_initialize_mtk");
    if (fdleak_debug_initialize == NULL) {
        fdleak_error_log("%s: fdleak_debug_initialize not found in %s\n",
                  __progname, so_name);
        goto FDLEAK_INIT_FAILURE;
    }
    
    if (fdleak_debug_initialize()) {
        goto FDLEAK_INIT_FAILURE;
    }

    fdleak_record_backtrace = 
            dlsym(libc_fdleak_impl_handle, "fdleak_record_backtrace_mtk");
    fdleak_remove_backtrace = 
            dlsym(libc_fdleak_impl_handle, "fdleak_remove_backtrace_mtk");
    if ((fdleak_record_backtrace == NULL) || 
        (fdleak_remove_backtrace == NULL)) {
        fdleak_error_log("%s: Can't get bactrace record/remove function:%p, %p",
                          __progname,fdleak_record_backtrace,fdleak_remove_backtrace); 
        goto FDLEAK_INIT_FAILURE;
    }
    
#ifdef MTK_USE_RESERVED_EXT_MEM
    fdleak_debug_prepare = dlsym(libc_fdleak_impl_handle, "fdleak_debug_prepare_mtk");
	fdleak_debug_parent = dlsym(libc_fdleak_impl_handle, "fdleak_debug_parent_mtk");
	fdleak_debug_child = dlsym(libc_fdleak_impl_handle, "fdleak_debug_child_mtk");

    if (fdleak_debug_prepare == NULL
    	|| fdleak_debug_parent == NULL
    	|| fdleak_debug_child == NULL) {
        fdleak_error_log("%s: load debug func for pthread_atfork fail\n", __progname);
        goto FDLEAK_INIT_FAILURE;
    } else {
    	int ret = pthread_atfork(fdleak_debug_prepare, fdleak_debug_parent, fdleak_debug_child);
    	if (ret != 0) {
    		fdleak_error_log("%s: pthread_atfork fail\n", __progname);
    		goto FDLEAK_INIT_FAILURE;
        }
    }
#endif
    fdleak_debug_log("[%s] %d\n", __progname, fdleak_debug_enable);
    return;
    
FDLEAK_INIT_FAILURE:
    fdleak_record_backtrace = NULL;
    fdleak_remove_backtrace = NULL;
    dlclose(libc_fdleak_impl_handle);
    libc_fdleak_impl_handle = NULL;
}

static void fdleak_fini_impl(void)
{
    if (libc_fdleak_impl_handle) {
        FDLeakDebugFini fdleak_debug_finalize = NULL;
        fdleak_record_backtrace = NULL;
        fdleak_remove_backtrace = NULL;
        fdleak_debug_finalize = dlsym(libc_fdleak_impl_handle, "fdleak_debug_finalize_mtk");
        if (fdleak_debug_finalize)
            fdleak_debug_finalize();
        dlclose(libc_fdleak_impl_handle);
        libc_fdleak_impl_handle = NULL;
    }
}

static pthread_once_t  fdleak_init_once_ctl = PTHREAD_ONCE_INIT;
static pthread_once_t  fdleak_fini_once_ctl = PTHREAD_ONCE_INIT;


/* Initializes FD Leakge debugging.
 * This routine is called from __libc_preinit routines
 */
void fdleak_debug_init(void)
{
    if (pthread_once(&fdleak_init_once_ctl, fdleak_init_impl)) {
        fdleak_error_log("Unable to initialize fdleak_debug component.");
    }
}

/* DeInitializes FD Leakge debugging.
 * This routine is called from __libc_postfini routines
 */
void fdleak_debug_fini(void)
{
#if 0 //no unload fdleak debug lib for race condition
    if (pthread_once(&fdleak_fini_once_ctl, fdleak_fini_impl)) {
        fdleak_error_log("Unable to finalize fdleak_debug component.");
    }
#endif
}
#endif

