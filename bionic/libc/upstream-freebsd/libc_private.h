/*
 * Copyright (C) 2013 The Android Open Source Project
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

#ifndef _BIONIC_FREEBSD_LIBC_PRIVATE_H_included
#define _BIONIC_FREEBSD_LIBC_PRIVATE_H_included

#define FLOCKFILE(fp)   do { if (__isthreaded) flockfile(fp); } while (0)
#define FUNLOCKFILE(fp) do { if (__isthreaded) funlockfile(fp); } while (0)

#define STDIO_THREAD_LOCK()   /* TODO: until we have the FreeBSD findfp.c, this is useless. */
#define STDIO_THREAD_UNLOCK() /* TODO: until we have the FreeBSD findfp.c, this is useless. */

#define ORIENT(fp, o) /* Only needed for wide-character stream support. */

#endif
