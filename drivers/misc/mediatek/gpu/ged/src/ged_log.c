#include <linux/version.h>
#include <asm/io.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/genalloc.h>
#include <linux/mutex.h>
#include <linux/xlog.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "ged_base.h"
#include "ged_log.h"
#include "ged_debugFS.h"
#include "ged_profile_dvfs.h"
#include "ged_hashtable.h"

typedef struct GED_LOG_BUF_TAG
{
    GED_LOG_BUF_TYPE    eType;
	char                *pcBuffer;

	int                 i32LineCountOrg;
	int                 i32LineCount;
	int                 i32LineBufferSize;
	int                 i32LineCurrent;
    int                 i32LineValidCount;

	spinlock_t          sSpinLock;
	unsigned long       ui32IRQFlags;

    char                acName[GED_LOG_BUF_NAME_LENGTH];
    char                acNodeName[GED_LOG_BUF_NODE_NAME_LENGTH];

    struct dentry*      psEntry;

    struct list_head    sList;

    unsigned int        ui32HashNodeID;

} GED_LOG_BUF;

typedef struct GED_LOG_BUF_LIST_TAG
{
	rwlock_t sLock;
	struct list_head sList;
} GED_LOG_BUF_LIST;

static GED_LOG_BUF_LIST gsGEDLogBufList;

static struct dentry* gpsGEDLogEntry = NULL;
static struct dentry* gpsGEDLogBufsDir = NULL;

static GED_HASHTABLE_HANDLE ghHashTable = NULL;

//-----------------------------------------------------------------------------
//
//  GED Log Buf
//
//-----------------------------------------------------------------------------
static GED_LOG_BUF* ged_log_buf_from_handle(GED_LOG_BUF_HANDLE hLogBuf)
{
    return ged_hashtable_find(ghHashTable, (unsigned int)hLogBuf);
}

static int __ged_log_buf_write(GED_LOG_BUF *psGEDLogBuf, const char __user *pszBuffer, int i32Count)
{
    bool bUpdate = false;
    char *buf;
    int cnt;

    if (!psGEDLogBuf)
    {
        return 0;
    }

    cnt = i32Count < psGEDLogBuf->i32LineBufferSize ? i32Count : psGEDLogBuf->i32LineBufferSize;

    spin_lock_irqsave(&psGEDLogBuf->sSpinLock, psGEDLogBuf->ui32IRQFlags);

    if (psGEDLogBuf->i32LineCurrent < psGEDLogBuf->i32LineCount)
    {
        buf = psGEDLogBuf->pcBuffer + psGEDLogBuf->i32LineCurrent * psGEDLogBuf->i32LineBufferSize;

        ged_copy_from_user(buf, pszBuffer, cnt);

        buf[cnt - 1] = '\0';

        psGEDLogBuf->i32LineCurrent ++;
        if (GED_LOG_BUF_TYPE_RINGBUFFER == psGEDLogBuf->eType)
        {
            if (psGEDLogBuf->i32LineCurrent >= psGEDLogBuf->i32LineCount)
            {
                psGEDLogBuf->i32LineCurrent = 0;
            }
        }

        if (psGEDLogBuf->i32LineValidCount < psGEDLogBuf->i32LineCount)
        {
            psGEDLogBuf->i32LineValidCount ++;
        }

        bUpdate = true;
    }

    spin_unlock_irqrestore(&psGEDLogBuf->sSpinLock, psGEDLogBuf->ui32IRQFlags);

    if ((psGEDLogBuf->i32LineValidCount == psGEDLogBuf->i32LineCount) &&
        (psGEDLogBuf->eType == GED_LOG_BUF_TYPE_QUEUEBUFFER_AUTO_INCREASE))
    {
        ged_log_buf_resize(psGEDLogBuf->ui32HashNodeID, psGEDLogBuf->i32LineCount + psGEDLogBuf->i32LineCountOrg);
    }

    if (false == bUpdate)
    {
        GED_LOGE("gedlog: not update in ged_log_buf_write()!\n");
    }

    return cnt;
}

static ssize_t ged_log_buf_write_entry(const char __user *pszBuffer, size_t uiCount, loff_t uiPosition, void *pvData)
{
    return (ssize_t)__ged_log_buf_write(pvData, pszBuffer, (int)uiCount);
}
//-----------------------------------------------------------------------------
static void* ged_log_buf_seq_start(struct seq_file *psSeqFile, loff_t *puiPosition)
{
    GED_LOG_BUF *psGEDLogBuf = (GED_LOG_BUF *)psSeqFile->private;

    if (0 == *puiPosition)
    {
        return psGEDLogBuf;
    }
    return NULL;
}
//-----------------------------------------------------------------------------
static void ged_log_buf_seq_stop(struct seq_file *psSeqFile, void *pvData)
{

}
//-----------------------------------------------------------------------------
static void* ged_log_buf_seq_next(struct seq_file *psSeqFile, void *pvData, loff_t *puiPosition)
{
    (*puiPosition)++;

    return NULL;
}
//-----------------------------------------------------------------------------
static int ged_log_buf_seq_show(struct seq_file *psSeqFile, void *pvData)
{
    GED_LOG_BUF *psGEDLogBuf = (GED_LOG_BUF *)pvData;

	if (psGEDLogBuf != NULL)
	{
        char *buf;
        int i, i32Count;

        spin_lock_irqsave(&psGEDLogBuf->sSpinLock, psGEDLogBuf->ui32IRQFlags);

        if (psGEDLogBuf->acName[0] != '\0')
        {
            seq_printf(psSeqFile, "---------- %s ----------\n", psGEDLogBuf->acName);
        }

        if (GED_LOG_BUF_TYPE_RINGBUFFER == psGEDLogBuf->eType)
        {
            i32Count = psGEDLogBuf->i32LineValidCount;
            i = psGEDLogBuf->i32LineCurrent - i32Count;
            if (i < 0)
            {
                i += psGEDLogBuf->i32LineCount;
                buf = psGEDLogBuf->pcBuffer + i * psGEDLogBuf->i32LineBufferSize;
                while ((i32Count > 0) && (i < psGEDLogBuf->i32LineCount))
                {
                    if (0 != seq_printf(psSeqFile, "%s\n", buf))
                    {
                        break;
                    }
                    buf += psGEDLogBuf->i32LineBufferSize;
                    i32Count --;
                    i ++;
                }
            }
            buf = psGEDLogBuf->pcBuffer;
            while (i32Count > 0)
            {
                if (0 != seq_printf(psSeqFile, "%s\n", buf))
                {
                    break;
                }
                buf += psGEDLogBuf->i32LineBufferSize;
                i32Count --;
            }
        }
        else if ((GED_LOG_BUF_TYPE_QUEUEBUFFER == psGEDLogBuf->eType) || 
                 (GED_LOG_BUF_TYPE_QUEUEBUFFER_AUTO_INCREASE == psGEDLogBuf->eType))
        {
            i32Count = psGEDLogBuf->i32LineValidCount;
            buf = psGEDLogBuf->pcBuffer;
            for (i = 0; i < i32Count; ++i)
            {
                if (0 != seq_printf(psSeqFile, "%s\n", buf))
                {
                    break;
                }
                buf += psGEDLogBuf->i32LineBufferSize;
            }
        }

        spin_unlock_irqrestore(&psGEDLogBuf->sSpinLock, psGEDLogBuf->ui32IRQFlags);
	}

	return 0;
}
//-----------------------------------------------------------------------------
static struct seq_operations gsGEDLogBufReadOps = 
{
	.start = ged_log_buf_seq_start,
	.stop = ged_log_buf_seq_stop,
	.next = ged_log_buf_seq_next,
	.show = ged_log_buf_seq_show,
};
//-----------------------------------------------------------------------------
GED_LOG_BUF_HANDLE ged_log_buf_alloc(
    int i32LineCount, 
    int i32LineBufferSize, 
    GED_LOG_BUF_TYPE eType, 
    const char* pszName,
    const char* pszNodeName)
{
    void *pvBuf;
    GED_LOG_BUF *psGEDLogBuf;
    GED_ERROR error;

    int i32BufSize = i32LineCount * i32LineBufferSize;

    if (((!pszName) && (!pszNodeName)) || (i32LineCount <= 0) || (i32LineBufferSize <= 0))
    {
        return (GED_LOG_BUF_HANDLE)0;
    }

    psGEDLogBuf = (GED_LOG_BUF*)ged_alloc(sizeof(GED_LOG_BUF));
    if (NULL == psGEDLogBuf)
    {
        GED_LOGE("ged: failed to allocate log buf!\n");
        return (GED_LOG_BUF_HANDLE)0;
    }

    pvBuf = ged_alloc(i32BufSize);
    if (NULL == pvBuf)
    {
        ged_free(psGEDLogBuf, sizeof(GED_LOG_BUF));
        GED_LOGE("ged: failed to allocate log buf!\n");
        return (GED_LOG_BUF_HANDLE)0;
    }

    psGEDLogBuf->eType = eType;
    psGEDLogBuf->pcBuffer = pvBuf;
    psGEDLogBuf->i32LineCountOrg = i32LineCount;
    psGEDLogBuf->i32LineCount = i32LineCount;
    psGEDLogBuf->i32LineBufferSize = i32LineBufferSize;
    psGEDLogBuf->i32LineCurrent = 0;
    psGEDLogBuf->i32LineValidCount = 0;
    psGEDLogBuf->psEntry = NULL;
    spin_lock_init(&psGEDLogBuf->sSpinLock);
    psGEDLogBuf->acName[0] = '\0';
    psGEDLogBuf->acNodeName[0] = '\0';

    if (pszName)
    {
        snprintf(psGEDLogBuf->acName, GED_LOG_BUF_NAME_LENGTH, "%s", pszName);
    }

    // Add into the global list
    INIT_LIST_HEAD(&psGEDLogBuf->sList);
    write_lock_bh(&gsGEDLogBufList.sLock);
    list_add(&psGEDLogBuf->sList, &gsGEDLogBufList.sList);
    write_unlock_bh(&gsGEDLogBufList.sLock);

    if (pszNodeName)
    {
        int err;
        snprintf(psGEDLogBuf->acNodeName, GED_LOG_BUF_NODE_NAME_LENGTH, "%s", pszNodeName);
        err = ged_debugFS_create_entry(
                psGEDLogBuf->acNodeName,
                gpsGEDLogBufsDir,
                &gsGEDLogBufReadOps,
                ged_log_buf_write_entry,
                psGEDLogBuf,
                &psGEDLogBuf->psEntry);

        if (unlikely(err)) 
        {
            GED_LOGE("ged: failed to create %s entry, err(%d)!\n", pszNodeName, err);
            ged_log_buf_free(psGEDLogBuf->ui32HashNodeID);
            return (GED_LOG_BUF_HANDLE)0;
        }
    }

    error = ged_hashtable_insert(ghHashTable, psGEDLogBuf, &psGEDLogBuf->ui32HashNodeID);
    if (GED_OK != error)
    {
        GED_LOGE("ged: failed to insert into a hash table, err(%d)!\n", error);
        ged_log_buf_free(psGEDLogBuf->ui32HashNodeID);
        return (GED_LOG_BUF_HANDLE)0;
    }

    GED_LOGI("ged_log_buf_alloc OK\n");

    return (GED_LOG_BUF_HANDLE)psGEDLogBuf->ui32HashNodeID;
}

GED_ERROR ged_log_buf_resize(
    GED_LOG_BUF_HANDLE hLogBuf,
    int i32NewLineCount)
{
    GED_LOG_BUF *psGEDLogBuf = ged_log_buf_from_handle(hLogBuf);
    int i32OldLineCount, i32OldBufSize, i32NewBufSize;
    void *pvNewBuf, *pvOldBuf;

    if ((NULL == psGEDLogBuf) || (i32NewLineCount <= 0))
    {
        return GED_ERROR_INVALID_PARAMS;
    }

    i32OldLineCount = psGEDLogBuf->i32LineCount;
    i32OldBufSize = i32OldLineCount * psGEDLogBuf->i32LineBufferSize;
    i32NewBufSize = i32NewLineCount * psGEDLogBuf->i32LineBufferSize;
    pvNewBuf = ged_alloc(i32NewBufSize);

    if (NULL == pvNewBuf)
    {
        return GED_ERROR_OOM;
    }

    spin_lock_irqsave(&psGEDLogBuf->sSpinLock, psGEDLogBuf->ui32IRQFlags);
    pvOldBuf = (void*)psGEDLogBuf->pcBuffer;
    memcpy(pvNewBuf, pvOldBuf, i32OldBufSize);
    psGEDLogBuf->i32LineCount = i32NewLineCount;
    psGEDLogBuf->pcBuffer = pvNewBuf;
    spin_unlock_irqrestore(&psGEDLogBuf->sSpinLock, psGEDLogBuf->ui32IRQFlags);
    ged_free(pvOldBuf, i32OldBufSize);
    return GED_OK;
}

GED_ERROR ged_log_buf_ignore_lines(GED_LOG_BUF_HANDLE hLogBuf, int i32LineCount)
{
    GED_LOG_BUF *psGEDLogBuf = ged_log_buf_from_handle(hLogBuf);
    if (psGEDLogBuf)
    {
        int i32NewLineValidCount;

        spin_lock_irqsave(&psGEDLogBuf->sSpinLock, psGEDLogBuf->ui32IRQFlags);

        i32NewLineValidCount = psGEDLogBuf->i32LineValidCount - i32LineCount;
        if (i32NewLineValidCount < 0)
        {
            i32NewLineValidCount = 0;
        }

        if ((GED_LOG_BUF_TYPE_QUEUEBUFFER == psGEDLogBuf->eType) ||
            (GED_LOG_BUF_TYPE_QUEUEBUFFER_AUTO_INCREASE == psGEDLogBuf->eType))
        {
            if (i32NewLineValidCount > 0)
            {
                void *pvNewBuf = (void*)psGEDLogBuf->pcBuffer;
                void *pvOldBuf = (void*)(psGEDLogBuf->pcBuffer + 
                    (psGEDLogBuf->i32LineCurrent - i32NewLineValidCount) * 
                    psGEDLogBuf->i32LineBufferSize);
                int i32NewBufSize = i32NewLineValidCount * psGEDLogBuf->i32LineBufferSize;
                memcpy(pvNewBuf, pvOldBuf, i32NewBufSize);
            }
            psGEDLogBuf->i32LineCurrent = i32NewLineValidCount;
        }

        psGEDLogBuf->i32LineValidCount = i32NewLineValidCount;

        spin_unlock_irqrestore(&psGEDLogBuf->sSpinLock, psGEDLogBuf->ui32IRQFlags);
    }

    return GED_OK;
}

GED_LOG_BUF_HANDLE ged_log_buf_get(const char* pszName)
{
    struct list_head *psListEntry, *psListEntryTemp, *psList;
    GED_LOG_BUF* psFound = NULL, *psLogBuf;

    if (!pszName)
    {
        return (GED_LOG_BUF_HANDLE)0;
    }

    read_lock_bh(&gsGEDLogBufList.sLock);

    psList = &gsGEDLogBufList.sList;
    list_for_each_safe(psListEntry, psListEntryTemp, psList)
    {
        psLogBuf = list_entry(psListEntry, GED_LOG_BUF, sList);
        if (0 == strcmp(psLogBuf->acName, pszName))
        {
            psFound = psLogBuf;
            break;
        }
    }

    read_unlock_bh(&gsGEDLogBufList.sLock);

    if (!psFound)
    {
        return (GED_LOG_BUF_HANDLE)0;
    }

    return (GED_LOG_BUF_HANDLE)psFound->ui32HashNodeID;
}
//-----------------------------------------------------------------------------
void ged_log_buf_free(GED_LOG_BUF_HANDLE hLogBuf)
{
    GED_LOG_BUF *psGEDLogBuf = ged_log_buf_from_handle(hLogBuf);
    if (psGEDLogBuf)
    {
        int i32BufSize = psGEDLogBuf->i32LineCount * psGEDLogBuf->i32LineBufferSize;

        ged_hashtable_remove(ghHashTable, psGEDLogBuf->ui32HashNodeID);

		write_lock_bh(&gsGEDLogBufList.sLock);
		list_del(&psGEDLogBuf->sList);
        write_unlock_bh(&gsGEDLogBufList.sLock);

        if (psGEDLogBuf->psEntry)
        {
            ged_debugFS_remove_entry(psGEDLogBuf->psEntry);
        }

        ged_free(psGEDLogBuf->pcBuffer, i32BufSize);
        ged_free(psGEDLogBuf, sizeof(GED_LOG_BUF));

        GED_LOGI("ged_log_buf_free OK\n");
    }
}
//-----------------------------------------------------------------------------
GED_ERROR ged_log_buf_print(GED_LOG_BUF_HANDLE hLogBuf, const char *fmt, ...)
{
    GED_LOG_BUF *psGEDLogBuf = ged_log_buf_from_handle(hLogBuf);
    if (psGEDLogBuf)
    {
        GED_BOOL bUpdate = GED_FALSE;
        va_list args;
        char *buf;
        
        spin_lock_irqsave(&psGEDLogBuf->sSpinLock, psGEDLogBuf->ui32IRQFlags);

        if (psGEDLogBuf->i32LineCurrent < psGEDLogBuf->i32LineCount)
        {
            buf = psGEDLogBuf->pcBuffer + psGEDLogBuf->i32LineCurrent * psGEDLogBuf->i32LineBufferSize;
            va_start(args, fmt);
            vsnprintf(buf, psGEDLogBuf->i32LineBufferSize - 1, fmt, args);
            va_end(args);
            buf[psGEDLogBuf->i32LineBufferSize - 1] = '\0';

            psGEDLogBuf->i32LineCurrent ++;
            if (GED_LOG_BUF_TYPE_RINGBUFFER == psGEDLogBuf->eType)
            {
                if (psGEDLogBuf->i32LineCurrent >= psGEDLogBuf->i32LineCount)
                {
                    psGEDLogBuf->i32LineCurrent = 0;
                }
            }

            if (psGEDLogBuf->i32LineValidCount < psGEDLogBuf->i32LineCount)
            {
                psGEDLogBuf->i32LineValidCount ++;
            }

            bUpdate = GED_TRUE;
        }

        spin_unlock_irqrestore(&psGEDLogBuf->sSpinLock, psGEDLogBuf->ui32IRQFlags);

        if ((psGEDLogBuf->i32LineValidCount == psGEDLogBuf->i32LineCount) &&
            (psGEDLogBuf->eType == GED_LOG_BUF_TYPE_QUEUEBUFFER_AUTO_INCREASE))
        {
            ged_log_buf_resize(psGEDLogBuf->ui32HashNodeID, psGEDLogBuf->i32LineCount + psGEDLogBuf->i32LineCountOrg);
        }

        if (GED_FALSE == bUpdate)
        {
            GED_LOGE("gedlog: out of buffer!\n");
        }
    }

    return GED_OK;
}
//-----------------------------------------------------------------------------
GED_ERROR ged_log_buf_reset(GED_LOG_BUF_HANDLE hLogBuf)
{
    GED_LOG_BUF *psGEDLogBuf = ged_log_buf_from_handle(hLogBuf);
    if (psGEDLogBuf)
    {
        if ((psGEDLogBuf->eType == GED_LOG_BUF_TYPE_QUEUEBUFFER_AUTO_INCREASE) &&
            (psGEDLogBuf->i32LineCount != psGEDLogBuf->i32LineCountOrg))
        {
            ged_log_buf_resize(psGEDLogBuf->ui32HashNodeID, psGEDLogBuf->i32LineCountOrg);
        }

        spin_lock_irqsave(&psGEDLogBuf->sSpinLock, psGEDLogBuf->ui32IRQFlags);
        psGEDLogBuf->i32LineCurrent = 0;
        psGEDLogBuf->i32LineValidCount = 0;
        spin_unlock_irqrestore(&psGEDLogBuf->sSpinLock, psGEDLogBuf->ui32IRQFlags);
    }

    return GED_OK;
}

//-----------------------------------------------------------------------------
//
//  GED Log System
//
//-----------------------------------------------------------------------------
static ssize_t ged_log_write_entry(const char __user *pszBuffer, size_t uiCount, loff_t uiPosition, void *pvData)
{
    #define GED_LOG_CMD_SIZE 64
    char acBuffer[GED_LOG_CMD_SIZE];

    int i32Value;

    if ((0 < uiCount) && (uiCount < GED_LOG_CMD_SIZE))
    {
        if (0 == ged_copy_from_user(acBuffer, pszBuffer, uiCount))
        {
            acBuffer[uiCount - 1] = '\0';
            if (strcmp(acBuffer, "reset") == 0)
            {
                struct list_head *psListEntry, *psListEntryTemp, *psList;
                write_lock_bh(&gsGEDLogBufList.sLock);
                psList = &gsGEDLogBufList.sList;
                list_for_each_safe(psListEntry, psListEntryTemp, psList)
                {
                    GED_LOG_BUF* psGEDLogBuf = (GED_LOG_BUF*)list_entry(psListEntry, GED_LOG_BUF, sList);
                    ged_log_buf_reset(psGEDLogBuf);
                }
                write_unlock_bh(&gsGEDLogBufList.sLock);
            }
            else if (strcmp(acBuffer, "profile_dvfs_enable") == 0)
            {
                ged_profile_dvfs_enable();
            }
            else if (strcmp(acBuffer, "profile_dvfs_disable") == 0)
            {
                ged_profile_dvfs_disable();
            }
            else if (strcmp(acBuffer, "profile_dvfs_start") == 0)
            {
                ged_profile_dvfs_start();
            }
            else if (strcmp(acBuffer, "profile_dvfs_stop") == 0)
            {
                ged_profile_dvfs_stop();
            }
            else if (sscanf(acBuffer, "profile_dvfs_ignore_lines %d", &i32Value) == 1)
            {
                ged_profile_dvfs_ignore_lines(i32Value);
            }
            //else if (...) //for other commands
            //{
            //}
        }
    }

    return uiCount;
}
//-----------------------------------------------------------------------------
static void* ged_log_seq_start(struct seq_file *psSeqFile, loff_t *puiPosition)
{
    struct list_head *psListEntry, *psListEntryTemp, *psList;
    loff_t uiCurrentPosition = 0;

    read_lock_bh(&gsGEDLogBufList.sLock);

    psList = &gsGEDLogBufList.sList;
    list_for_each_safe(psListEntry, psListEntryTemp, psList)
    {
        GED_LOG_BUF* psGEDLogBuf = (GED_LOG_BUF*)list_entry(psListEntry, GED_LOG_BUF, sList);
        if (psGEDLogBuf->acName[0] != '\0')
        {
            if (uiCurrentPosition == *puiPosition)
            {
                return psGEDLogBuf;
            }
            uiCurrentPosition ++;
        }
    }

    return NULL;
}
//-----------------------------------------------------------------------------
static void ged_log_seq_stop(struct seq_file *psSeqFile, void *pvData)
{
    read_unlock_bh(&gsGEDLogBufList.sLock);
}
//-----------------------------------------------------------------------------
static void* ged_log_seq_next(struct seq_file *psSeqFile, void *pvData, loff_t *puiPosition)
{
    struct list_head *psListEntry, *psListEntryTemp, *psList;
	loff_t uiCurrentPosition = 0;

	(*puiPosition)++;

    psList = &gsGEDLogBufList.sList;
    list_for_each_safe(psListEntry, psListEntryTemp, psList)
    {
        GED_LOG_BUF* psGEDLogBuf = (GED_LOG_BUF*)list_entry(psListEntry, GED_LOG_BUF, sList);
        if (psGEDLogBuf->acName[0] != '\0')
        {
            if (uiCurrentPosition == *puiPosition)
            {
                return psGEDLogBuf;
            }
            uiCurrentPosition ++;
        }
    }

    return NULL;
}
//-----------------------------------------------------------------------------
static int ged_log_seq_show(struct seq_file *psSeqFile, void *pvData)
{
    GED_LOG_BUF *psGEDLogBuf = (GED_LOG_BUF *)pvData;

	if (psGEDLogBuf != NULL)
	{
        char *buf;
        int i, i32Count;

        spin_lock_irqsave(&psGEDLogBuf->sSpinLock, psGEDLogBuf->ui32IRQFlags);

        seq_printf(psSeqFile, "---------- %s ----------\n", psGEDLogBuf->acName);

        if (GED_LOG_BUF_TYPE_RINGBUFFER == psGEDLogBuf->eType)
        {
            i32Count = psGEDLogBuf->i32LineValidCount;
            i = psGEDLogBuf->i32LineCurrent - i32Count;
            if (i < 0)
            {
                i += psGEDLogBuf->i32LineCount;
                buf = psGEDLogBuf->pcBuffer + i * psGEDLogBuf->i32LineBufferSize;
                while ((i32Count > 0) && (i < psGEDLogBuf->i32LineCount))
                {
                    if (0 != seq_printf(psSeqFile, "%s\n", buf))
                    {
                       break;
                    }
                    buf += psGEDLogBuf->i32LineBufferSize;
                    i32Count --;
                    i ++;
                }
            }
            buf = psGEDLogBuf->pcBuffer;
            while (i32Count > 0)
            {
                if (0 != seq_printf(psSeqFile, "%s\n", buf))
                {
                    break;
                }
                buf += psGEDLogBuf->i32LineBufferSize;
                i32Count --;
            }
        }
        else if ((GED_LOG_BUF_TYPE_QUEUEBUFFER == psGEDLogBuf->eType) || 
                 (GED_LOG_BUF_TYPE_QUEUEBUFFER_AUTO_INCREASE == psGEDLogBuf->eType))
        {
            i32Count = psGEDLogBuf->i32LineValidCount;
            buf = psGEDLogBuf->pcBuffer;
            for (i = 0; i < i32Count; ++i)
            {
                if (0 != seq_printf(psSeqFile, "%s\n", buf))
                {
                    break;
                }
                buf += psGEDLogBuf->i32LineBufferSize;
            }
        }

        spin_unlock_irqrestore(&psGEDLogBuf->sSpinLock, psGEDLogBuf->ui32IRQFlags);
	}

	return 0;
}
//-----------------------------------------------------------------------------
static struct seq_operations gsGEDLogReadOps = 
{
	.start = ged_log_seq_start,
	.stop = ged_log_seq_stop,
	.next = ged_log_seq_next,
	.show = ged_log_seq_show,
};
//-----------------------------------------------------------------------------
GED_ERROR ged_log_system_init(void)
{
    GED_ERROR err = GED_OK;

	INIT_LIST_HEAD(&gsGEDLogBufList.sList);
	rwlock_init(&gsGEDLogBufList.sLock);

    err = ged_debugFS_create_entry(
            "gedlog",
            NULL,
            &gsGEDLogReadOps,
            ged_log_write_entry,
            NULL,
            &gpsGEDLogEntry);

    if (unlikely(err != GED_OK))
    {
        GED_LOGE("ged: failed to create gedlog entry!\n");
        goto ERROR;
    }

    err = ged_debugFS_create_entry_dir(
            "logbufs",
            NULL,
            &gpsGEDLogBufsDir);

    if (unlikely(err != GED_OK))
    {
        err = GED_ERROR_FAIL;
        GED_LOGE("ged: failed to create logbufs dir!\n");
        goto ERROR;
    }

    ghHashTable = ged_hashtable_create(5);
    if (!ghHashTable) 
    {
        err = GED_ERROR_OOM;
        GED_LOGE("ged: failed to create a hash table!\n");
        goto ERROR;
    }

    return err;

ERROR:

    ged_log_system_exit();

    return err;
}
//-----------------------------------------------------------------------------
void ged_log_system_exit(void)
{
    ged_hashtable_destroy(ghHashTable);

    ged_debugFS_remove_entry(gpsGEDLogEntry);
}
//-----------------------------------------------------------------------------
int ged_log_buf_write(GED_LOG_BUF_HANDLE hLogBuf, const char __user *pszBuffer, int i32Count)
{
    GED_LOG_BUF *psGEDLogBuf = ged_log_buf_from_handle(hLogBuf);
    return __ged_log_buf_write(psGEDLogBuf, pszBuffer, i32Count);
}
