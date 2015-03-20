/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <dlfcn.h>
/* These are stubs for functions that are actually defined
 * in the dynamic linker (dlfcn.c), and hijacked at runtime.
 */
void *dlopen(const char *filename, int flag) { return 0; }
const char *dlerror(void) { return 0; }
void *dlsym(void *handle, const char *symbol) { return 0; }
int dladdr(const void *addr, Dl_info *info) { return 0; }
int dlclose(void *handle) { return 0; }

void android_update_LD_LIBRARY_PATH(const char* ld_library_path) { }

#if defined(__arm__)

void *dl_unwind_find_exidx(void *pc, int *pcount) { return 0; }

#elif defined(__i386__) || defined(__mips__)

/* we munge the cb definition so we don't have to include any headers here.
 * It won't affect anything since these are just symbols anyway */
int dl_iterate_phdr(int (*cb)(void *info, void *size, void *data), void *data) { return 0; }

#else
#error Unsupported architecture. Only mips, arm and x86 are supported.
#endif
void dl_register_notify_function(int (*load_notify_function) (const char *name,uintptr_t address,uintptr_t size ),int (*unload_notify_function) (const char *name, uintptr_t address)){return 0;}
void dl_unregister_notify_function(void){return 0;}