/******************************************************************************
*
 *
 * Filename:
 * ---------
 *   AudDrv_Common.h
 *
 * Project:
 * --------
 *   MT6583 FPGA LDVT Audio Driver
 *
 * Description:
 * ------------
 *   Audio register
 *
 * Author:
 * -------
 *   Chipeng Chang (MTK02308)
 *
 *---------------------------------------------------------------------------
---
 * $Revision: #1 $
 * $Modtime:$
 * $Log:$
 *
 *

*******************************************************************************/

#ifndef AUDIO_GLOBAL_H
#define AUDIO_GLOBAL_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/jiffies.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <mach/irqs.h>
#include <mach/sync_write.h>
#include <linux/xlog.h>
#include <mach/mt_typedefs.h>
#include "AudDrv_Def.h"

typedef struct
{
    kal_uint32 pucPhysBufAddr;
    kal_uint8 *pucVirtBufAddr;
#if defined(MTK_AUDIO_DYNAMIC_SRAM_SUPPORT)
    kal_uint32 pucPhysBufAddrBackup;
    kal_uint8 *pucVirtBufAddrBackup;
#endif
    kal_int32 u4BufferSize;
    kal_int32 u4DataRemained;
    kal_uint32 u4SampleNumMask;    // sample number mask
    kal_uint32 u4SamplesPerInt;    // number of samples to play before interrupting
    kal_int32 u4WriteIdx;          // Previous Write Index.
    kal_int32 u4DMAReadIdx;        // Previous DMA Read Index.
    kal_uint32 u4fsyncflag;
    kal_uint32 uResetFlag;
} AFE_BLOCK_T;


typedef struct
{
    struct file *flip;
    AFE_BLOCK_T    rBlock;
    kal_bool       bRunning;
    kal_uint32   MemIfNum;
} AFE_MEM_CONTROL_T;

#endif

