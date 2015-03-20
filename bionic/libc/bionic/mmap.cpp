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

#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>

#include "private/ErrnoRestorer.h"

#define ENABLE_MMAP_BT_NAME

#ifdef ENABLE_MMAP_BT_NAME
#define BIONIC_PR_SET_VMA               0x53564d41
#define BIONIC_PR_SET_VMA_ANON_NAME     0
typedef char* (*RecordMmap)(void *,size_t);
#else
typedef void (*RecordMmap)(void *,size_t);
#endif

RecordMmap recordMmapFunc = NULL;

// mmap2(2) is like mmap(2), but the offset is in 4096-byte blocks, not bytes.
extern "C" void*  __mmap2(void*, size_t, int, int, int, size_t);

#define MMAP2_SHIFT 12 // 2**12 == 4096

void* mmap(void* addr, size_t size, int prot, int flags, int fd, off_t offset) {
  if (offset & ((1UL << MMAP2_SHIFT)-1)) {
    errno = EINVAL;
    return MAP_FAILED;
  }

  size_t unsigned_offset = static_cast<size_t>(offset); // To avoid sign extension.
  void* result = __mmap2(addr, size, prot, flags, fd, unsigned_offset >> MMAP2_SHIFT);

  if (result != MAP_FAILED && (flags & (MAP_PRIVATE | MAP_ANONYMOUS)) != 0) {
    ErrnoRestorer errno_restorer;
    madvise(result, size, MADV_MERGEABLE);
  }

#if (!defined LIBC_STATIC) && (defined _MMAP_MTK_DEBUG_)
	if(result != MAP_FAILED && (flags&MAP_ANONYMOUS) && !(prot&PROT_MALLOCFROMBIONIC)) {
		if (recordMmapFunc != NULL) {
#ifdef ENABLE_MMAP_BT_NAME
			char* btStr = recordMmapFunc(result, size);
			if (btStr)
				prctl(BIONIC_PR_SET_VMA, BIONIC_PR_SET_VMA_ANON_NAME, result, size, btStr);
#else
			recordMmapFunc(result, size);
#endif
		}
	}
#endif

  return result;
}
