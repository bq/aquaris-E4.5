#ifndef __GED_LOG_H__
#define __GED_LOG_H__

#include "ged_type.h"

#if defined(__GNUC__)
#define GED_LOG_BUF_FORMAT_PRINTF(x,y)		__attribute__((format(printf,x,y)))
#else
#define GED_LOG_BUF_FORMAT_PRINTF(x,y)
#endif

#define GED_LOG_BUF_NAME_LENGTH 64
#define GED_LOG_BUF_NODE_NAME_LENGTH 64

typedef enum GED_LOG_BUF_TYPE_TAG
{
	GED_LOG_BUF_TYPE_RINGBUFFER,
	GED_LOG_BUF_TYPE_QUEUEBUFFER,
    GED_LOG_BUF_TYPE_QUEUEBUFFER_AUTO_INCREASE,
} GED_LOG_BUF_TYPE;

GED_LOG_BUF_HANDLE ged_log_buf_alloc(int i32LineCount, int i32LineBufferSize, GED_LOG_BUF_TYPE eType, const char* pszName, const char* pszNodeName);

GED_ERROR ged_log_buf_resize(GED_LOG_BUF_HANDLE hLogBuf, int i32NewLineCount);

GED_ERROR ged_log_buf_ignore_lines(GED_LOG_BUF_HANDLE hLogBuf, int i32LineCount);

GED_LOG_BUF_HANDLE ged_log_buf_get(const char* pszName);

void ged_log_buf_free(GED_LOG_BUF_HANDLE hLogBuf);

GED_ERROR ged_log_buf_print(GED_LOG_BUF_HANDLE hLogBuf, const char *fmt, ...) GED_LOG_BUF_FORMAT_PRINTF(2,3);

GED_ERROR ged_log_buf_reset(GED_LOG_BUF_HANDLE hLogBuf);

GED_ERROR ged_log_system_init(void);

void ged_log_system_exit(void);

int ged_log_buf_write(GED_LOG_BUF_HANDLE hLogBuf, const char __user *pszBuffer, int i32Count);

#endif
