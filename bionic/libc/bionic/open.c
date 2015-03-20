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
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include "libc_logging.h"

extern int  __open(const char*, int, int);

#ifdef _FDLEAK_DEBUG_
#include "fdleak_debug_common.h"

//mask due to set FD_RECORD_THD over 3
/*LCH add for logger main init done */ 
/*extern int logger_main_init_done;*/

int open(const char *pathname, int flags, ...)
{
    mode_t  mode = 0;
    int fd = -1;

    flags |= O_LARGEFILE;

    if (flags & O_CREAT)
    {
        va_list  args;

        va_start(args, flags);
        mode = (mode_t) va_arg(args, int);
        va_end(args);
    }
    
    fd = __open(pathname, flags, mode);
    if (fdleak_record_backtrace) {
        fdleak_record_backtrace(fd);
    }
    return fd;
}

int __open_2(const char *pathname, int flags) {
    int fd = -1;
    if (__predict_false(flags & O_CREAT)) {
        __fortify_chk_fail("open(O_CREAT) called without specifying a mode", 0);
    }

    flags |= O_LARGEFILE;

    fd = __open(pathname, flags, 0);
    if (fdleak_record_backtrace) {
        fdleak_record_backtrace(fd);
    }
    return fd;
}
#else
int open(const char *pathname, int flags, ...)
{
    mode_t  mode = 0;

    flags |= O_LARGEFILE;

    if (flags & O_CREAT)
    {
        va_list  args;

        va_start(args, flags);
        mode = (mode_t) va_arg(args, int);
        va_end(args);
    }

    return __open(pathname, flags, mode);
}

int __open_2(const char *pathname, int flags) {
    if (__predict_false(flags & O_CREAT)) {
        __fortify_chk_fail("open(O_CREAT) called without specifying a mode", 0);
    }

    flags |= O_LARGEFILE;

    return __open(pathname, flags, 0);
}
#endif
