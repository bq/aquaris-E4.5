/*
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * libc_init_dynamic.c
 *
 * This source files provides two important functions for dynamic
 * executables:
 *
 * - a C runtime initializer (__libc_preinit), which is called by
 *   the dynamic linker when libc.so is loaded. This happens before
 *   any other initializer (e.g. static C++ constructors in other
 *   shared libraries the program depends on).
 *
 * - a program launch function (__libc_init), which is called after
 *   all dynamic linking has been performed. Technically, it is called
 *   from arch-$ARCH/bionic/crtbegin_dynamic.S which is itself called
 *   by the dynamic linker after all libraries have been loaded and
 *   initialized.
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <elf.h>
#include "atexit.h"
#include "KernelArgumentBlock.h"
#include "libc_init_common.h"
#include <bionic_tls.h>

extern "C" {
  extern void pthread_debug_init(void);
  extern void malloc_debug_init(void);
  extern void malloc_debug_fini(void);
#ifdef _FDLEAK_DEBUG_
  extern void fdleak_debug_init(void);
  extern void fdleak_debug_fini(void);
#endif

#ifdef _MMAP_MTK_DEBUG_
  extern void mmap_debug_init(void);
  extern void mmap_debug_fini(void);
#endif

};

#if defined(HAVE_AEE_FEATURE)
#include <signal.h>
#include <sys/system_properties.h>
#include <pthread.h>
static void _core_direct_debug_init(void)
{
  char env[PROP_VALUE_MAX];
  int len = 0;

  len = __system_property_get("persist.aee.core.direct", env);
  if (len > 0 && !strncmp(env, "enable", 6)) {
    signal(SIGABRT, SIG_DFL);
    signal(SIGBUS, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
    signal(SIGILL, SIG_DFL);
    signal(SIGPIPE, SIG_DFL);
    signal(SIGSEGV, SIG_DFL);
#if defined(SIGSTKFLT)
    signal(SIGSTKFLT, SIG_DFL);
#endif
    signal(SIGTRAP, SIG_DFL);
  }
}
static pthread_once_t  core_direct_init_once_ctl = PTHREAD_ONCE_INIT;
void core_direct_debug_init(void)
{
    pthread_once(&core_direct_init_once_ctl, _core_direct_debug_init);
}
#endif

// We flag the __libc_preinit function as a constructor to ensure
// that its address is listed in libc.so's .init_array section.
// This ensures that the function is called by the dynamic linker
// as soon as the shared library is loaded.
__attribute__((constructor)) static void __libc_preinit() {
  // Read the kernel argument block pointer from TLS.
  void* tls = const_cast<void*>(__get_tls());
  KernelArgumentBlock** args_slot = &reinterpret_cast<KernelArgumentBlock**>(tls)[TLS_SLOT_BIONIC_PREINIT];
  KernelArgumentBlock* args = *args_slot;

  // Clear the slot so no other initializer sees its value.
  // __libc_init_common() will change the TLS area so the old one won't be accessible anyway.
  *args_slot = NULL;

  __libc_init_common(*args);

  // Hooks for the debug malloc and pthread libraries to let them know that we're starting up.
  pthread_debug_init();
  malloc_debug_init();
#ifdef _FDLEAK_DEBUG_
  fdleak_debug_init();
#endif

#ifdef _MMAP_MTK_DEBUG_
  mmap_debug_init();
#endif

#if defined(HAVE_AEE_FEATURE)
  core_direct_debug_init();
#endif
}

__LIBC_HIDDEN__ void __libc_postfini() {
  // A hook for the debug malloc library to let it know that we're shutting down.
#ifdef _FDLEAK_DEBUG_
  fdleak_debug_fini();
#endif
  malloc_debug_fini();

#ifdef _MMAP_MTK_DEBUG_
  mmap_debug_fini();
#endif

}

// This function is called from the executable's _start entry point
// (see arch-$ARCH/bionic/crtbegin_dynamic.S), which is itself
// called by the dynamic linker after it has loaded all shared
// libraries the executable depends on.
//
// Note that the dynamic linker has also run all constructors in the
// executable at this point.
__noreturn void __libc_init(void* raw_args,
                            void (*onexit)(void),
                            int (*slingshot)(int, char**, char**),
                            structors_array_t const * const structors) {

  KernelArgumentBlock args(raw_args);

  // Several Linux ABIs don't pass the onexit pointer, and the ones that
  // do never use it.  Therefore, we ignore it.

  // The executable may have its own destructors listed in its .fini_array
  // so we need to ensure that these are called when the program exits
  // normally.
  if (structors->fini_array) {
    __cxa_atexit(__libc_fini,structors->fini_array,NULL);
  }

  exit(slingshot(args.argc, args.argv, args.envp));
}
