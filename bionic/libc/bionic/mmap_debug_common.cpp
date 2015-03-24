#if (!defined LIBC_STATIC) && (defined _MMAP_MTK_DEBUG_)
/*******************the Below is for  MMAP leakage Debug *************************/
#include <sys/system_properties.h>
#include <dlfcn.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
//#include "mmap_debug_common.h"
#include "libc_logging.h"

typedef void (*RecordMmap)(void *,size_t);
typedef void (*RecordMunmap)(void *, size_t);

extern RecordMmap recordMmapFunc;
extern RecordMunmap recordMunmapFunc;

extern const char* __progname;

static pthread_once_t  mmap_init_once_ctl = PTHREAD_ONCE_INIT;
static pthread_once_t  mmap_fini_once_ctl = PTHREAD_ONCE_INIT;
static void* libc_mmap_impl_handle = NULL;


// =============================================================================
// log functions
// =============================================================================
#define mmap_debug_log(format, ...)  \
    __libc_format_log(ANDROID_LOG_DEBUG, "mmap_mtk_debug", (format), ##__VA_ARGS__ )
#define mmap_error_log(format, ...)  \
    __libc_format_log(ANDROID_LOG_ERROR, "mmap_mtk_debug", (format), ##__VA_ARGS__ )
#define mmap_info_log(format,...)  \
    __libc_format_log(ANDROID_LOG_INFO, "mmap_mtk_debug", (format), ##__VA_ARGS__ )


/* Initializes memory allocation framework once per process. */
static void mmap_init_impl(void)
{
    const char* so_name = "/system/lib/lib_br.so";
    unsigned int mmap_debug_level = 0;
    char env[PROP_VALUE_MAX];
    char debug_program[PROP_VALUE_MAX];
	unsigned int unwind_method = 0; //0 :FP unwind, 1: GCC unwind

#ifdef _MTK_ENG_
    mmap_debug_level = 0;
#else
    mmap_debug_level = 0;
#endif

    /* If debug level has not been set by memcheck option in the emulator,
     * lets grab it from libc.debug.malloc system property. */
    if(__system_property_get("persist.libc.debug.mmap", env) 
		|| __system_property_get("libc.debug.mmap", env)) {
        mmap_debug_level = atoi(env);
    }
    
    if(!mmap_debug_level) {
        return;
    }
    
    /* Control for only one specific program to enable mmap leak debug.*/
    if(__system_property_get("persist.libc.debug.mmap.program", debug_program)) {
        if (strstr(__progname, debug_program)) {
            mmap_debug_level = 1;
        }
        else
            mmap_debug_level = 0;
    }
    
	if(!mmap_debug_level) {
        return;
    }
	
	if (__system_property_get("persist.libc.debug.mmap.unwind", env)) {
        unwind_method = atoi(env);
    }
        
     // Load .so that implements the required malloc debugging functionality.
    libc_mmap_impl_handle = dlopen(so_name, RTLD_LAZY);
    if (libc_mmap_impl_handle == NULL) {
        mmap_error_log("%s: Missing module %s required for mmap debug level %d\n",
                 __progname, so_name, mmap_debug_level);
        return;
    }
	
	recordMmapFunc = reinterpret_cast<RecordMmap>(dlsym(libc_mmap_impl_handle, "recordMmap"));
	if(recordMmapFunc == NULL){
		mmap_error_log("%s: MMAP_BT routine is not found in %s\n",
                  __progname, so_name);
        dlclose(libc_mmap_impl_handle);
        return;
	}

	recordMunmapFunc = reinterpret_cast<RecordMunmap>(dlsym(libc_mmap_impl_handle, "recordMunmap"));
	if(recordMunmapFunc == NULL){
		mmap_error_log("%s: MUNMAP_BT routine is not found in %s\n",
                  __progname, so_name);
        dlclose(libc_mmap_impl_handle);
        return;
	}
    mmap_info_log("%s,unwind:%d\n",__progname, unwind_method);
}

static void mmap_fini_impl(void)
{
#if 0
    if(libc_mmap_impl_handle) {
        dlclose(libc_mmap_impl_handle);
        libc_mmap_impl_handle = NULL;
    }
#endif
}

/* Initializes mmap Leakge debugging.
 * This routine is called from __libc_preinit routines
 */
extern "C" void mmap_debug_init(void)
{
    if (pthread_once(&mmap_init_once_ctl, mmap_init_impl)) {
        mmap_error_log("Unable to initialize mmap_debug component.");
    }
}

/* DeInitializes mmap Leakge debugging.
 * This routine is called from __libc_postfini routines
 */
extern "C" void mmap_debug_fini(void)
{
    if (pthread_once(&mmap_fini_once_ctl, mmap_fini_impl)) {
        mmap_error_log("Unable to initialize mmap_debug component.");
    }
}
#endif

