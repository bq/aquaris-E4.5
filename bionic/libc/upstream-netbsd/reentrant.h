/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef _BIONIC_NETBSD_REENTRANT_H_included
#define _BIONIC_NETBSD_REENTRANT_H_included

#define _REENTRANT

#include <pthread.h>
#include <signal.h>

//
// Map NetBSD libc internal locking to pthread locking.
//

#define MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#define mutex_t pthread_mutex_t

#define RWLOCK_INITIALIZER PTHREAD_RWLOCK_INITIALIZER
#define rwlock_t pthread_rwlock_t
#define rwlock_rdlock pthread_rwlock_rdlock
#define rwlock_unlock pthread_rwlock_unlock
#define rwlock_wrlock pthread_rwlock_wrlock

#endif
