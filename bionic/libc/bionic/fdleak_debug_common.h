/*
 * Contains declarations of types and constants used by malloc leak
 * detection code in both, libc and libc_malloc_debug libraries.
 */
#ifndef FDLEAK_DEBUG_COMMON_H
#define FDLEAK_DEBUG_COMMON_H

#include "libc_logging.h"

#ifdef __cplusplus
extern "C" {
#endif

/* FD leakage debugging initialization and finalization routines.
 *
 * These routines must be implemented in libc_fdleak_debug_mtk.so that implement FD leak
 * debugging. They are called once per process from fdleak_init_impl and
 * fdleak_fini_impl respectively.
 *
 * FDLeakDebugInit returns:
 *    0 on success, -1 on failure.
 */
typedef int (*FDLeakDebugInit)(void);
typedef void (*FDLeakDebugFini)(void);


/* FD leakage debugging backtrace record and remove routines.
 *
 * These routines must be implemented in libc_fdleak_debug_mtk.so that implement FD leak
 * debugging. They are called once per process from fdleak_init_impl and
 * fdleak_fini_impl respectively.
 *
 */
typedef void (*FDLeak_Record_Backtrace)(int);
typedef void (*FDLeak_Remove_Backtrace)(int);


/* 
 *log functions
 */
#define fdleak_debug_log(format, ...)  \
    __libc_format_log(ANDROID_LOG_DEBUG, "fdleak_debug", (format), ##__VA_ARGS__ )
#define fdleak_error_log(format, ...)  \
    __libc_format_log(ANDROID_LOG_ERROR, "fdleak_debug", (format), ##__VA_ARGS__ )
#define fdleak_info_log(format, ...)  \
    __libc_format_log(ANDROID_LOG_INFO, "fdleak_debug", (format), ##__VA_ARGS__ )

/* 
 *global function pointer for FD allocate/close backtrace record/remove
 * these rountines will be provided in libc_fdleak_debug_mtk.so 
 */
#ifdef _FDLEAK_DEBUG_
FDLeak_Record_Backtrace fdleak_record_backtrace;
FDLeak_Remove_Backtrace fdleak_remove_backtrace;
#endif

#ifdef __cplusplus
};
#endif
#endif
