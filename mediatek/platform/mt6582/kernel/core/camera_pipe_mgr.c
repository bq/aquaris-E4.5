//-----------------------------------------------------------------------------
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/xlog.h>
#include <asm/io.h>
#include <mach/camera_pipe_mgr.h>
#include <mach/camera_pipe_mgr_imp.h>
//-----------------------------------------------------------------------------
static CAM_PIPE_MGR_STRUCT CamPipeMgr;
//------------------------------------------------------------------------------
static void CamPipeMgr_GetTime(MUINT32* pSec, MUINT32* pUSec)
{
    ktime_t Time;
    MUINT64 TimeSec;
    //
    Time = ktime_get();//ns
    TimeSec = Time.tv64;
    do_div( TimeSec, 1000 );
    *pUSec = do_div( TimeSec, 1000000);
    //
    *pSec = (MUINT64)TimeSec;
}
//-----------------------------------------------------------------------------
static inline void CamPipeMgr_SpinLock(void)
{
    //LOG_MSG("");
    spin_lock(&(CamPipeMgr.SpinLock));
}
//-----------------------------------------------------------------------------
static inline void CamPipeMgr_SpinUnlock(void)
{
    //LOG_MSG("");
    spin_unlock(&(CamPipeMgr.SpinLock));
}
//-----------------------------------------------------------------------------
static unsigned long CamPipeMgr_MsToJiffies(MUINT32 Ms)
{
    return ((Ms*HZ + 512) >> 10);
}
//-----------------------------------------------------------------------------
static void CamPipeMgr_DumpPipeInfo(void)
{
    MUINT32 i;
    //
    LOG_MSG("E");
    for(i=0;i<CAM_PIPE_MGR_PIPE_AMOUNT;i++)
    {
        if( CamPipeMgr.PipeInfo[i].Pid != 0 &&
            CamPipeMgr.PipeInfo[i].Tgid != 0)
        {
            LOG_MSG("Pipe(%ld,%s),Proc:Name(%s),Pid(%d),Tgid(%d),Time(%ld.%06ld)",
                    i,
                    CamPipeMgr.PipeName[i],
                    CamPipeMgr.PipeInfo[i].ProcName,
                    CamPipeMgr.PipeInfo[i].Pid,
                    CamPipeMgr.PipeInfo[i].Tgid,
                    CamPipeMgr.PipeInfo[i].TimeS,
                    CamPipeMgr.PipeInfo[i].TimeUS);
        }
    }
    LOG_MSG("X");
}
//----------------------------------------------------------------------------
static MUINT32 CamPipeMgr_GtePipeLockTable(
    CAM_PIPE_MGR_SCEN_SW_ENUM   ScenSw,
    CAM_PIPE_MGR_SCEN_HW_ENUM   ScenHw)
{
    MUINT32 PipeLockTable = 0;
    //
    switch(ScenHw)
    {
        case CAM_PIPE_MGR_SCEN_HW_NONE:
        {
            if(ScenSw == CAM_PIPE_MGR_SCEN_SW_NONE)
            {
                PipeLockTable = CAM_PIPE_MGR_LOCK_TABLE_NONE;
            }
            break;
        }
        case CAM_PIPE_MGR_SCEN_HW_IC:
        {
            PipeLockTable = CAM_PIPE_MGR_LOCK_TABLE_IC;
            break;
        }
        case CAM_PIPE_MGR_SCEN_HW_VR:
        {
            PipeLockTable = CAM_PIPE_MGR_LOCK_TABLE_VR;
            break;
        }
        case CAM_PIPE_MGR_SCEN_HW_ZSD:
        {
            PipeLockTable = CAM_PIPE_MGR_LOCK_TABLE_ZSD;
            break;
        }
        case CAM_PIPE_MGR_SCEN_HW_IP:
        {
            PipeLockTable = CAM_PIPE_MGR_LOCK_TABLE_IP;
            break;
        }
        case CAM_PIPE_MGR_SCEN_HW_N3D:
        {
            PipeLockTable = CAM_PIPE_MGR_LOCK_TABLE_N3D;
            break;
        }
        case CAM_PIPE_MGR_SCEN_HW_VSS:
        {
            PipeLockTable = CAM_PIPE_MGR_LOCK_TABLE_VSS;
            break;
        }
        default:
        {
            LOG_ERR("Unknown ScenHw(%d)",ScenHw);
            break;
        }
    }
    //
    switch(ScenSw)
    {
        case CAM_PIPE_MGR_SCEN_SW_CAM_PRV:
        case CAM_PIPE_MGR_SCEN_SW_VIDEO_PRV:
        {
            if(ScenHw == CAM_PIPE_MGR_SCEN_HW_VSS)
            {
                //do nothing
            }
            break;
        }
        case CAM_PIPE_MGR_SCEN_SW_ZSD:
        {
            if(ScenHw == CAM_PIPE_MGR_SCEN_HW_ZSD)
            {
                //do nothing
            }
            break;
        }
        //
        case CAM_PIPE_MGR_SCEN_SW_NONE:
        case CAM_PIPE_MGR_SCEN_SW_CAM_IDLE:
        case CAM_PIPE_MGR_SCEN_SW_CAM_CAP:
        case CAM_PIPE_MGR_SCEN_SW_VIDEO_REC:
        case CAM_PIPE_MGR_SCEN_SW_VIDEO_VSS:
        case CAM_PIPE_MGR_SCEN_SW_N3D:
        {
            //Do nothing.
            break;
        }
        default:
        {
            LOG_ERR("Unknown ScenSw(%d)",ScenSw);
            break;
        }
    }
    //
    return PipeLockTable;
}
//----------------------------------------------------------------------------
static void CamPipeMgr_UpdatePipeLockTable(CAM_PIPE_MGR_SCEN_SW_ENUM ScenSw)
{
    MUINT32 i;
    //
    for(i=0;i<CAM_PIPE_MGR_SCEN_HW_AMOUNT;i++)
    {
        CamPipeMgr.PipeLockTable[i] = CamPipeMgr_GtePipeLockTable(ScenSw,i);
    }
}
//----------------------------------------------------------------------------
static void CamPipeMgr_StorePipeInfo(MUINT32 PipeMask)
{
    MUINT32 i;
    //
    //LOG_MSG("PipeMask(0x%08X)",PipeMask);
    //
    CamPipeMgr.PipeMask |= PipeMask;
    //
    for(i=0;i<CAM_PIPE_MGR_PIPE_AMOUNT;i++)
    {
        if((1<<i) & PipeMask)
        {
            if( CamPipeMgr.PipeInfo[i].Pid == 0 &&
                CamPipeMgr.PipeInfo[i].Tgid == 0)
            {
                CamPipeMgr.PipeInfo[i].Pid = current->pid;
                CamPipeMgr.PipeInfo[i].Tgid = current->tgid;
                strcpy(CamPipeMgr.PipeInfo[i].ProcName,current->comm);
                CamPipeMgr_GetTime(&(CamPipeMgr.PipeInfo[i].TimeS), &(CamPipeMgr.PipeInfo[i].TimeUS));
            }
            else
            {
                LOG_ERR("PipeMask(0x%lX),Pipe(%ld,%s),Pid(%d),Tgid(%d),Time(%ld.%06ld)",
                        PipeMask,
                        i,
                        CamPipeMgr.PipeInfo[i].ProcName,
                        CamPipeMgr.PipeInfo[i].Pid,
                        CamPipeMgr.PipeInfo[i].Tgid,
                        CamPipeMgr.PipeInfo[i].TimeS,
                        CamPipeMgr.PipeInfo[i].TimeUS);
            }
        }
    }
}
//-----------------------------------------------------------------------------
static void CamPipeMgr_RemovePipeInfo(MUINT32 PipeMask)
{
    MUINT32 i;
    //
    //LOG_MSG("PipeMask(0x%08X)",PipeMask);
    //
    CamPipeMgr.PipeMask &= (~PipeMask);
    //
    for(i=0;i<CAM_PIPE_MGR_PIPE_AMOUNT;i++)
    {
        if((1<<i) & PipeMask)
        {
            if( CamPipeMgr.PipeInfo[i].Pid != 0 &&
                CamPipeMgr.PipeInfo[i].Tgid != 0)
            {
                CamPipeMgr.PipeInfo[i].Pid = 0;
                CamPipeMgr.PipeInfo[i].Tgid = 0;
                strcpy(CamPipeMgr.PipeInfo[i].ProcName,CAM_PIPE_MGR_PROC_NAME);
            }
            else
            {
                LOG_WRN("PipeMask(0x%lX),Pipe(%ld,%s),Pid(%d),Tgid(%d),Time(%ld.%06ld)",
                        PipeMask,
                        i,
                        CamPipeMgr.PipeInfo[i].ProcName,
                        CamPipeMgr.PipeInfo[i].Pid,
                        CamPipeMgr.PipeInfo[i].Tgid,
                        CamPipeMgr.PipeInfo[i].TimeS,
                        CamPipeMgr.PipeInfo[i].TimeUS);
            }
        }
    }
}
//-----------------------------------------------------------------------------
static CAM_PIPE_MGR_STATUS_ENUM CamPipeMgr_LockPipe(CAM_PIPE_MGR_LOCK_STRUCT* pLock)
{
    MUINT32 Timeout;
    CAM_PIPE_MGR_STATUS_ENUM Result = CAM_PIPE_MGR_STATUS_OK;
    //
    if((CamPipeMgr.PipeMask & pLock->PipeMask) == 0)
    {
        if((pLock->PipeMask & CamPipeMgr.PipeLockTable[CamPipeMgr.Mode.ScenHw]) != pLock->PipeMask)
        {
            Result = CAM_PIPE_MGR_STATUS_TIMEOUT;
        }
        else
        {
            CamPipeMgr_StorePipeInfo(pLock->PipeMask);
            Result = CAM_PIPE_MGR_STATUS_OK;
        }
    }
    else
    {
        CamPipeMgr_SpinUnlock();
        if(pLock->Timeout > CAM_PIPE_MGR_TIMEOUT_MAX)
        {
            pLock->Timeout = CAM_PIPE_MGR_TIMEOUT_MAX;
        }
        Timeout = wait_event_interruptible_timeout(
                    CamPipeMgr.WaitQueueHead, 
                    (CamPipeMgr.PipeMask & pLock->PipeMask) == 0,
                    CamPipeMgr_MsToJiffies(pLock->Timeout));
        CamPipeMgr_SpinLock();
        if((CamPipeMgr.PipeMask & pLock->PipeMask) == 0)
        {
            if((pLock->PipeMask & CamPipeMgr.PipeLockTable[CamPipeMgr.Mode.ScenHw]) != pLock->PipeMask)
            {
                Result = CAM_PIPE_MGR_STATUS_TIMEOUT;
            }
            else
            {
                CamPipeMgr_StorePipeInfo(pLock->PipeMask);
                Result = CAM_PIPE_MGR_STATUS_OK;
            }
        }
        else
        if( Timeout == 0 &&
            (CamPipeMgr.PipeMask & pLock->PipeMask) != 0)
        {
            Result = CAM_PIPE_MGR_STATUS_TIMEOUT;
        }
        else
        {
            Result = CAM_PIPE_MGR_STATUS_UNKNOW;
        }
    }
    //
    return Result;
}
//-----------------------------------------------------------------------------
static void CamPipeMgr_UnlockPipe(CAM_PIPE_MGR_UNLOCK_STRUCT* pUnlock)
{
    CamPipeMgr_RemovePipeInfo(pUnlock->PipeMask);
    wake_up_interruptible(&(CamPipeMgr.WaitQueueHead));
}
//-----------------------------------------------------------------------------
static int CamPipeMgr_Open(
    struct inode*   pInode,
    struct file*    pFile)
{
    int Ret = 0;
    MUINT32 Sec = 0,USec = 0;
    CAM_PIPE_MGR_PROC_STRUCT*   pProc;
    //
    CamPipeMgr_GetTime(&Sec, &USec);
    //
    LOG_MSG("Cur:Name(%s),pid(%d),tgid(%d),Time(%ld.%06ld)",
            current->comm,
            current->pid,
            current->tgid,
            Sec,
            USec);
    //
    CamPipeMgr_SpinLock();
    //
    pFile->private_data = NULL;
    pFile->private_data = kmalloc(sizeof(CAM_PIPE_MGR_PROC_STRUCT),GFP_ATOMIC);
    if(pFile->private_data == NULL)
    {
        Ret = -ENOMEM;
    }
    else
    {
        pProc = (CAM_PIPE_MGR_PROC_STRUCT*)pFile->private_data;
        pProc->Pid = 0;
        pProc->Tgid = 0;
        strcpy(pProc->ProcName,CAM_PIPE_MGR_PROC_NAME);
        pProc->PipeMask = 0;
        pProc->TimeS = Sec;
        pProc->TimeUS = USec;
    }
    //
    CamPipeMgr_SpinUnlock();
    //
    if(Ret == (-ENOMEM))
    {
        LOG_ERR("No enough memory");
        /*
        LOG_ERR("Cur:Name(%s),pid(%d),tgid(%d),Time(%ld.%06ld)",
                current->comm,
                current->pid,
                current->tgid,
                Sec,
                USec);
        */
    }
    //
    //LOG_MSG("OK");
    return Ret;
}
//-----------------------------------------------------------------------------
static int CamPipeMgr_Release(
    struct inode*   pInode,
    struct file*    pFile)
{
    MUINT32 Sec = 0,USec = 0;
    CAM_PIPE_MGR_PROC_STRUCT*   pProc;
    CAM_PIPE_MGR_UNLOCK_STRUCT  Unlock;
    //
    CamPipeMgr_GetTime(&Sec, &USec);
    //
    LOG_MSG("Cur:Name(%s),pid(%d),tgid(%d),Time(%ld.%06ld)",
            current->comm,
            current->pid,
            current->tgid,
            Sec,
            USec);
    //
    if(pFile->private_data != NULL)
    {
        pProc = (CAM_PIPE_MGR_PROC_STRUCT*)pFile->private_data;
        //
        if( pProc->Pid != 0 ||
            pProc->Tgid != 0 ||
            pProc->PipeMask != 0)
        {
            //
            LOG_WRN("Proc:Name(%s),Pid(%d),Tgid(%d),PipeMask(0x%lX),Time(%ld.%06ld)",
                    pProc->ProcName,
                    pProc->Pid,
                    pProc->Tgid,
                    pProc->PipeMask,
                    pProc->TimeS,
                    pProc->TimeUS);
            //
            if(pProc->PipeMask)
            {
                LOG_WRN("Force to unlock pipe");
                /*
                LOG_WRN("Proc:Name(%s),Pid(%d),Tgid(%d),PipeMask(0x%08lX),Time(%ld.%06ld)",
                        pProc->ProcName,
                        pProc->Pid,
                        pProc->Tgid,
                        pProc->PipeMask,
                        pProc->TimeS,
                        pProc->TimeUS);
                */
                CamPipeMgr_SpinLock();
                Unlock.PipeMask = pProc->PipeMask;
                CamPipeMgr_UnlockPipe(&Unlock);
                CamPipeMgr_SpinUnlock();
            }
        }
        //
        kfree(pFile->private_data);
        pFile->private_data = NULL;
    }
    else
    {
        LOG_WRN("private_data is NULL");
        /*
        LOG_WRN("Cur:Name(%s),pid(%d),tgid(%d),Time(%ld.%06ld)",
                current->comm,
                current->pid,
                current->tgid,
                Sec,
                USec);
        */
    }
    //
    //LOG_MSG("OK");
    return 0;
}
//-----------------------------------------------------------------------------
static int CamPipeMgr_Flush(
    struct file*    pFile,
    fl_owner_t      Id)
{
    MUINT32 Sec = 0,USec = 0;
    CAM_PIPE_MGR_PROC_STRUCT*   pProc;
    CAM_PIPE_MGR_UNLOCK_STRUCT  Unlock;
    //
     CamPipeMgr_GetTime(&Sec, &USec);
    //
    LOG_MSG("Cur:Name(%s),pid(%d),tgid(%d),Time(%ld.%06ld)",
            current->comm,
            current->pid,
            current->tgid,
            Sec,
            USec);
    //
    if(pFile->private_data != NULL)
    {
        pProc = (CAM_PIPE_MGR_PROC_STRUCT*)pFile->private_data;
        //
        if( pProc->Pid != 0 ||
            pProc->Tgid != 0 ||
            pProc->PipeMask != 0)
        {
            //
            LOG_WRN("Proc:Name(%s),Pid(%d),Tgid(%d),PipeMask(0x%lX),Time(%ld.%06ld)",
                    pProc->ProcName,
                    pProc->Pid,
                    pProc->Tgid,
                    pProc->PipeMask,
                    pProc->TimeS,
                    pProc->TimeUS);
            //
            if( pProc->Tgid == 0 &&
                pProc->PipeMask != 0)
            {
                LOG_ERR("No Tgid info");
                /*
                LOG_ERR("Cur:Name(%s),pid(%d),tgid(%d),Time(%ld.%06ld)",
                        current->comm,
                        current->pid,
                        current->tgid,
                        Sec,
                        USec);
                LOG_ERR("Proc:Name(%s),Pid(%d),Tgid(%d),PipeMask(0x%08lX),Time(%ld.%06ld)",
                        pProc->ProcName,
                        pProc->Pid,
                        pProc->Tgid,
                        pProc->PipeMask,
                        pProc->TimeS,
                        pProc->TimeUS);
                */
            }
            else
            if( (pProc->Tgid == current->tgid) ||
                ((pProc->Tgid != current->tgid) && (strcmp(current->comm, "binder") == 0)))
            {
                if(pProc->PipeMask)
                {
                    LOG_WRN("Force to unlock pipe");
                    /*
                    LOG_WRN("Proc:Name(%s),Pid(%d),Tgid(%d),PipeMask(0x%08lX),Time(%ld.%06ld)",
                            pProc->ProcName,
                            pProc->Pid,
                            pProc->Tgid,
                            pProc->PipeMask,
                            pProc->TimeS,
                            pProc->TimeUS);
                    */
                    CamPipeMgr_SpinLock();
                    Unlock.PipeMask = pProc->PipeMask;
                    CamPipeMgr_UnlockPipe(&Unlock);
                    pProc->PipeMask = 0;
                    CamPipeMgr_SpinUnlock();
                }
            }
        }
    }
    else
    {
        LOG_WRN("private_data is NULL");
        /*
        LOG_WRN("Cur:Name(%s),pid(%d),tgid(%d),Time(%ld.%06ld)",
                current->comm,
                current->pid,
                current->tgid,
                Sec,
                USec);
        */
    }
    //
    //LOG_MSG("OK");
    return 0;
}
//-----------------------------------------------------------------------------
static long CamPipeMgr_Ioctl(
    struct file*    pFile,
    unsigned int    Cmd,
    unsigned long   Param)
{
    MINT32  Ret = 0;
    MUINT32 Sec = 0,USec = 0;
    pid_t   Pid;
    pid_t   Tgid;
    char    ProcName[TASK_COMM_LEN];
    CAM_PIPE_MGR_LOCK_STRUCT    Lock;
    CAM_PIPE_MGR_UNLOCK_STRUCT  Unlock;
    CAM_PIPE_MGR_MODE_STRUCT    Mode;
    CAM_PIPE_MGR_ENABLE_STRUCT  Enable;
    CAM_PIPE_MGR_DISABLE_STRUCT Disable;
    CAM_PIPE_MGR_PROC_STRUCT*   pProc = (CAM_PIPE_MGR_PROC_STRUCT*)pFile->private_data;
    CAM_PIPE_MGR_STATUS_ENUM    Status;
    //
    CamPipeMgr_GetTime(&Sec, &USec);
    /*
    LOG_MSG("Cur:Name(%s),pid(%d),tgid(%d),Time(%ld.%06ld)",
            current->comm,
            current->pid,
            current->tgid,
            Sec,
            USec);
    */
    if(pFile->private_data == NULL)
    {
        LOG_ERR("private_data is NULL");
        Ret = -EFAULT;
        goto EXIT;
    }
    //
    switch(Cmd)
    {
        case CAM_PIPE_MGR_LOCK:
        {
            if(copy_from_user(&Lock, (void*)Param, sizeof(CAM_PIPE_MGR_LOCK_STRUCT)) == 0)
            {
                if((Lock.PipeMask & CamPipeMgr.PipeLockTable[CamPipeMgr.Mode.ScenHw]) != Lock.PipeMask)
                {
                    LOG_ERR("LOCK:Sw(%d),Hw(%d),LPM(0x%lX),PLT(0x%lX) fail",
                            CamPipeMgr.Mode.ScenSw,
                            CamPipeMgr.Mode.ScenHw,
                            Lock.PipeMask,
                            CamPipeMgr.PipeLockTable[CamPipeMgr.Mode.ScenHw]);
                    Ret = -EFAULT;
                }
                else
                {
                    CamPipeMgr_SpinLock();
                    Status = CamPipeMgr_LockPipe(&Lock);
                    if(Status == CAM_PIPE_MGR_STATUS_OK)
                    {
                        pProc->PipeMask |= Lock.PipeMask;
                        if(pProc->Tgid == 0)
                        {
                            pProc->Pid = current->pid;
                            pProc->Tgid = current->tgid;
                            strcpy(pProc->ProcName,current->comm);
                            CamPipeMgr_SpinUnlock();
                            if(CamPipeMgr.LogMask & Lock.PipeMask)
                            {
                                LOG_MSG("LOCK:Sw(%d),Hw(%d),LPM(0x%lX),PLT(0x%lX) OK",
                                        CamPipeMgr.Mode.ScenSw,
                                        CamPipeMgr.Mode.ScenHw,
                                        Lock.PipeMask,
                                        CamPipeMgr.PipeLockTable[CamPipeMgr.Mode.ScenHw]);
                                LOG_MSG("LOCK:Proc:Name(%s),Pid(%d),Tgid(%d),PipeMask(0x%lX)",
                                        pProc->ProcName,
                                        pProc->Pid,
                                        pProc->Tgid,
                                        pProc->PipeMask);
                            }
                        }
                        else
                        {
                            CamPipeMgr_SpinUnlock();
                            if(pProc->Tgid != current->tgid)
                            {
                                LOG_ERR("LOCK:Tgid is inconsistent");
                                Ret = -EFAULT;
                            }
                        }
                    }
                    else
                    {
                        CamPipeMgr_SpinUnlock();
                        if( (CamPipeMgr.LogMask & Lock.PipeMask) ||
                            (CamPipeMgr.Mode.ScenSw == CAM_PIPE_MGR_SCEN_SW_NONE))
                        {
                            LOG_ERR("LOCK:Sw(%d),Hw(%d),LPM(0x%lX),PLT(0x%lX) fail,Status(%d)",
                                    CamPipeMgr.Mode.ScenSw,
                                    CamPipeMgr.Mode.ScenHw,
                                    Lock.PipeMask,
                                    CamPipeMgr.PipeLockTable[CamPipeMgr.Mode.ScenHw],
                                    Status);
                        }
                        Ret = -EFAULT;
                    }
                }
            }
            else
            {
                LOG_ERR("LOCK:copy_from_user fail");
                Ret = -EFAULT;
            }
            break;
        }
        //
        case CAM_PIPE_MGR_UNLOCK:
        {
            if(copy_from_user(&Unlock, (void*)Param, sizeof(CAM_PIPE_MGR_UNLOCK_STRUCT)) == 0)
            {
                CamPipeMgr_SpinLock();
                if(pProc->PipeMask & Unlock.PipeMask)
                {
                    CamPipeMgr_UnlockPipe(&Unlock);
                    //Store info before clear.
                    Pid = pProc->Pid;
                    Tgid = pProc->Tgid;
                    strcpy(ProcName,pProc->ProcName);
                    //
                    pProc->PipeMask &= (~Unlock.PipeMask);
                    if(pProc->PipeMask == 0)
                    {
                        pProc->Pid = 0;
                        pProc->Tgid = 0;
                        strcpy(pProc->ProcName,CAM_PIPE_MGR_PROC_NAME);
                    }
                    CamPipeMgr_SpinUnlock();
                    if(CamPipeMgr.LogMask & Unlock.PipeMask)
                    {
                        LOG_MSG("UNLOCK:Sw(%d),Hw(%d),UPM(0x%lX),PLT(0x%lX) OK",
                                CamPipeMgr.Mode.ScenSw,
                                CamPipeMgr.Mode.ScenHw,
                                Unlock.PipeMask,
                                CamPipeMgr.PipeLockTable[CamPipeMgr.Mode.ScenHw]);
                        LOG_MSG("UNLOCK:Proc:Name(%s),Pid(%d),Tgid(%d),PipeMask(0x%lX)",
                                ProcName,
                                Pid,
                                Tgid,
                                pProc->PipeMask);
                    }
                }
                else
                {
                    CamPipeMgr_SpinUnlock();
                    if( (CamPipeMgr.LogMask & Unlock.PipeMask) ||
                        (CamPipeMgr.Mode.ScenSw == CAM_PIPE_MGR_SCEN_SW_NONE))
                    {
                        LOG_ERR("UNLOCK:Sw(%d),Hw(%d),UPM(0x%lX),PLT(0x%lX) fail, it was not locked before",
                                CamPipeMgr.Mode.ScenSw,
                                CamPipeMgr.Mode.ScenHw,
                                Unlock.PipeMask,
                                CamPipeMgr.PipeLockTable[CamPipeMgr.Mode.ScenHw]);
                    }
                    Ret = -EFAULT;
                }
            }
            else
            {
                LOG_ERR("UNLOCK:copy_from_user fail");
                Ret = -EFAULT;
            }
            break;
        }
        //
        case CAM_PIPE_MGR_DUMP:
        {
            CamPipeMgr_DumpPipeInfo();
            break;
        }
        //
        case CAM_PIPE_MGR_SET_MODE:
        {
            if(copy_from_user(&Mode, (void*)Param, sizeof(CAM_PIPE_MGR_MODE_STRUCT)) == 0)
            {
                LOG_MSG("SET_MODE:Sw(%d),Hw(%d)",Mode.ScenSw,Mode.ScenHw);
                if((CamPipeMgr.PipeMask | CamPipeMgr.PipeLockTable[Mode.ScenHw]) ^ CamPipeMgr.PipeLockTable[Mode.ScenHw])
                {
                    LOG_ERR("SET_MODE:PM(0x%lX),PLT(0x%lX), some pipe should be unlock",
                            CamPipeMgr.PipeMask,
                            CamPipeMgr.PipeLockTable[Mode.ScenHw]);
                    Ret = -EFAULT;
                }
                //
                CamPipeMgr_SpinLock();
                memcpy(
                    &(CamPipeMgr.Mode),
                    &Mode,
                    sizeof(CAM_PIPE_MGR_MODE_STRUCT));
                CamPipeMgr_UpdatePipeLockTable(CamPipeMgr.Mode.ScenSw);
                CamPipeMgr_SpinUnlock();
                LOG_MSG("SET_MODE:done");

            }
            else
            {
                LOG_ERR("SET_MODE:copy_from_user fail");
                Ret = -EFAULT;
            }
            break;
        }
        //
        case CAM_PIPE_MGR_GET_MODE:
        {
            if(copy_to_user((void*)Param, &(CamPipeMgr.Mode),  sizeof(CAM_PIPE_MGR_MODE_STRUCT)) == 0)
            {
                //do nothing.
            }
            else
            {
                LOG_ERR("GET_MODE:copy_to_user fail");
                Ret = -EFAULT;
            }
            break;
        }
        //
        case CAM_PIPE_MGR_ENABLE_PIPE:
        {
            if(copy_from_user(&Enable, (void*)Param, sizeof(CAM_PIPE_MGR_ENABLE_STRUCT)) == 0)
            {
                LOG_MSG("ENABLE_PIPE:Sw(%d),Hw(%d):EPM(0x%lX),PLT(0x%lX)",
                        CamPipeMgr.Mode.ScenSw,
                        CamPipeMgr.Mode.ScenHw,
                        Enable.PipeMask,
                        CamPipeMgr_GtePipeLockTable(CamPipeMgr.Mode.ScenSw,CamPipeMgr.Mode.ScenHw));
                if((Enable.PipeMask & CamPipeMgr_GtePipeLockTable(CamPipeMgr.Mode.ScenSw,CamPipeMgr.Mode.ScenHw)) != Enable.PipeMask)
                {
                    LOG_ERR("ENABLE_PIPE:Some pipe are not available");
                    Ret = -EFAULT;
                }
                else
                {
                    CamPipeMgr_SpinLock();
                    CamPipeMgr.PipeLockTable[CamPipeMgr.Mode.ScenHw] |= Enable.PipeMask;
                    CamPipeMgr_SpinUnlock();
                }
            }
            else
            {
                LOG_ERR("ENABLE_PIPE:copy_from_user fail");
                Ret = -EFAULT;
            }
            break;
        }
        //
        case CAM_PIPE_MGR_DISABLE_PIPE:
        {
            if(copy_from_user(&Disable, (void*)Param, sizeof(CAM_PIPE_MGR_DISABLE_STRUCT)) == 0)
            {
                LOG_MSG("DISABLE_PIPE:Sw(%d),Hw(%d):DPM(0x%lX),PLT(0x%lX)",
                        CamPipeMgr.Mode.ScenSw,
                        CamPipeMgr.Mode.ScenHw,
                        Disable.PipeMask,
                        CamPipeMgr_GtePipeLockTable(CamPipeMgr.Mode.ScenSw,CamPipeMgr.Mode.ScenHw));
                if((Disable.PipeMask & CamPipeMgr_GtePipeLockTable(CamPipeMgr.Mode.ScenSw,CamPipeMgr.Mode.ScenHw)) != Disable.PipeMask)
                {
                    LOG_ERR("DISABLE_PIPE:Some pipe are not available");
                    Ret = -EFAULT;
                }
                else
                {
                    CamPipeMgr_SpinLock();
                    CamPipeMgr.PipeLockTable[CamPipeMgr.Mode.ScenHw] &= (~Disable.PipeMask);
                    CamPipeMgr_SpinUnlock();
                }
            }
            else
            {
                LOG_ERR("DISABLE_PIPE:copy_from_user fail");
                Ret = -EFAULT;
            }
            break;
        }
        //
        default:
        {
            LOG_ERR("Unknown cmd");
            Ret = -EFAULT;
            break;
        }
    }
    //
    EXIT:
    if(Ret != 0)
    {
        if( (CamPipeMgr.LogMask & Lock.PipeMask) ||
            (CamPipeMgr.Mode.ScenSw == CAM_PIPE_MGR_SCEN_SW_NONE))
        {
            LOG_ERR("Fail");
            LOG_ERR("Cur:Name(%s),pid(%d),tgid(%d),Time(%ld.%06ld)",
                    current->comm,
                    current->pid,
                    current->tgid,
                    Sec,
                    USec);
            if(pFile->private_data != NULL)
            {
                LOG_ERR("Proc:Name(%s),Pid(%d),Tgid(%d),PipeMask(0x%lX),Time(%ld.%06ld)",
                        pProc->ProcName,
                        pProc->Pid,
                        pProc->Tgid,
                        pProc->PipeMask,
                        Sec,
                        USec);
            }
            CamPipeMgr_DumpPipeInfo();
        }
    }
    return Ret;
}
//-----------------------------------------------------------------------------
static const struct file_operations CamPipeMgr_FileOper = 
{
    .owner          = THIS_MODULE,
    .open           = CamPipeMgr_Open,
    .release        = CamPipeMgr_Release,
    .flush          = CamPipeMgr_Flush,
    .unlocked_ioctl = CamPipeMgr_Ioctl
};
//-----------------------------------------------------------------------------
static int CamPipeMgr_RegCharDev(void)
{
    MINT32 Ret = 0;
    //
    LOG_MSG("E");
    //
    CamPipeMgr.DevNo = 0;
    Ret = alloc_chrdev_region(
            &(CamPipeMgr.DevNo),
            CAM_PIPE_MGR_DEV_NO_MINOR,
            CAM_PIPE_MGR_DEV_NUM,
            CAM_PIPE_MGR_DEV_NAME);
    if(Ret < 0)
    {
        LOG_ERR("alloc_chrdev_region fail:Ret(%ld)",Ret);
        return Ret;
    }
    //Allocate memory for driver
    CamPipeMgr.pCharDrv = cdev_alloc();
    if(CamPipeMgr.pCharDrv == NULL)
    {
        unregister_chrdev_region(
            CamPipeMgr.DevNo,
            CAM_PIPE_MGR_DEV_NUM);
        LOG_ERR("Allocate mem for kobject failed");
        return -ENOMEM;
    }
    //Attatch file operation.
    cdev_init(
        CamPipeMgr.pCharDrv,
        &CamPipeMgr_FileOper);
    CamPipeMgr.pCharDrv->owner = THIS_MODULE;
    //Add to system
    if(cdev_add(CamPipeMgr.pCharDrv, CamPipeMgr.DevNo, CAM_PIPE_MGR_DEV_MINOR_NUM))
    {
        LOG_ERR("Attatch file operation failed");
        unregister_chrdev_region(
            CamPipeMgr.DevNo,
            CAM_PIPE_MGR_DEV_NUM);
        return -EAGAIN;
    }
    //
    LOG_MSG("X");
    return Ret;
}
//-----------------------------------------------------------------------------
static inline void CamPipeMgr_UnregCharDev(void)
{
    LOG_MSG("E");
    //Release char driver
    cdev_del(CamPipeMgr.pCharDrv);
    unregister_chrdev_region(
        CamPipeMgr.DevNo,
        CAM_PIPE_MGR_DEV_NUM);
    //
    LOG_MSG("X");
}
//-----------------------------------------------------------------------------
static int CamPipeMgr_Probe(struct platform_device *pDev)
{
    MINT32 Ret = 0;
    MUINT32 i;
    struct device* pDevice = NULL;
    //
    LOG_MSG("E");
    //
    Ret = CamPipeMgr_RegCharDev();
    if(Ret < 0)
    {
        LOG_ERR("RegCharDev fail:Ret(%ld)",Ret);
        return Ret;
    }

    CamPipeMgr.pClass = class_create(
                            THIS_MODULE,
                            CAM_PIPE_MGR_DEV_NAME);
    if(IS_ERR(CamPipeMgr.pClass))
    {
        Ret = PTR_ERR(CamPipeMgr.pClass);
        LOG_ERR("class_create fail:Ret(%ld)",Ret);
        return Ret;            
    }
    pDevice = device_create(
                CamPipeMgr.pClass,
                NULL,
                CamPipeMgr.DevNo,
                NULL,
                CAM_PIPE_MGR_DEV_NAME);
    if(IS_ERR(pDevice))
    {
        LOG_ERR("device_create fail");
        return (int)pDevice;
    }
    //Initial variable
    spin_lock_init(&(CamPipeMgr.SpinLock));
    init_waitqueue_head(&(CamPipeMgr.WaitQueueHead));
    CamPipeMgr.Mode.ScenSw = CAM_PIPE_MGR_SCEN_SW_NONE;
    CamPipeMgr.Mode.ScenHw = CAM_PIPE_MGR_SCEN_HW_NONE;
    //
    for(i=0;i<CAM_PIPE_MGR_PIPE_AMOUNT;i++)
    {
        CamPipeMgr.PipeInfo[i].Pid = 0;
        CamPipeMgr.PipeInfo[i].Tgid = 0;
        strcpy(CamPipeMgr.PipeInfo[i].ProcName,CAM_PIPE_MGR_PROC_NAME);
        CamPipeMgr.PipeInfo[i].TimeS = 0;
        CamPipeMgr.PipeInfo[i].TimeUS = 0;
    }
    //
    strcpy(
        CamPipeMgr.PipeName[CAM_PIPE_MGR_PIPE_CAM_IO],
        CAM_PIPE_MGR_PIPE_NAME_CAM_IO);
    strcpy(
        CamPipeMgr.PipeName[CAM_PIPE_MGR_PIPE_POST_PROC],
        CAM_PIPE_MGR_PIPE_NAME_POST_PROC);
    strcpy(
        CamPipeMgr.PipeName[CAM_PIPE_MGR_PIPE_XDP_CAM],
        CAM_PIPE_MGR_PIPE_NAME_XDP_CAM);
    //
    CamPipeMgr_UpdatePipeLockTable(CamPipeMgr.Mode.ScenSw);
    CamPipeMgr.LogMask = (  CAM_PIPE_MGR_PIPE_MASK_CAM_IO|
                            CAM_PIPE_MGR_PIPE_MASK_POST_PROC|
                            CAM_PIPE_MGR_PIPE_MASK_XDP_CAM);
    //
    LOG_MSG("X");
    return Ret;
}
//-----------------------------------------------------------------------------
static int CamPipeMgr_Remove(struct platform_device *pdev)
{
    LOG_MSG("E");
    //unregister char driver.
    CamPipeMgr_UnregCharDev();
    //
    device_destroy(
        CamPipeMgr.pClass,
        CamPipeMgr.DevNo);
    class_destroy(CamPipeMgr.pClass);
    //
    LOG_MSG("X");
    return 0;
}
//-----------------------------------------------------------------------------
static int CamPipeMgr_Suspend(
    struct platform_device* pDev,
    pm_message_t            Mesg)
{
    LOG_MSG("");
    return 0;
}
//-----------------------------------------------------------------------------
static int CamPipeMgr_Resume(struct platform_device *pDev)
{
    LOG_MSG("");
    return 0;
}
//-----------------------------------------------------------------------------
static struct platform_driver CamPipeMgr_PlatformDriver =
{
    .probe      = CamPipeMgr_Probe,
    .remove     = CamPipeMgr_Remove,
    .suspend    = CamPipeMgr_Suspend,
    .resume     = CamPipeMgr_Resume,
    .driver     =
    {
        .name   = CAM_PIPE_MGR_DEV_NAME,
        .owner  = THIS_MODULE,
    }
};
//-----------------------------------------------------------------------------
static int __init CamPipeMgr_Init(void)
{
    MINT32 Ret = 0;
    //
    LOG_MSG("E");
    //
    Ret = platform_driver_register(&CamPipeMgr_PlatformDriver);
    if(Ret < 0)
    {
        LOG_ERR("Failed to register driver:Ret(%ld)",Ret);
        return Ret;
    }
    //
    LOG_MSG("X");
    return Ret;
}
//-----------------------------------------------------------------------------
static void __exit CamPipeMgr_Exit(void)
{
    LOG_MSG("E");
    platform_driver_unregister(&CamPipeMgr_PlatformDriver);
    LOG_MSG("X");
}
//-----------------------------------------------------------------------------
module_init(CamPipeMgr_Init);
module_exit(CamPipeMgr_Exit);
MODULE_DESCRIPTION("Camera Pipe Manager Driver");
MODULE_AUTHOR("Marx <Marx.Chiu@Mediatek.com>");
MODULE_LICENSE("GPL");
//-----------------------------------------------------------------------------

