/* MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */
#ifndef TZ_MOD_H
#define TZ_MOD_H


 /*****************************************************************************
 * MODULE DEFINITION
 *****************************************************************************/
#define MODULE_NAME	    "[MTEE_MOD]"
#define TZ_DEV_NAME        "trustzone"
#define MAJOR_DEV_NUM   197

 /*****************************************************************************
 * IOCTL DEFINITION
 *****************************************************************************/
#define MTEE_IOC_MAGIC       'T'
#define MTEE_CMD_OPEN_SESSION   _IOWR(MTEE_IOC_MAGIC,  1, struct kree_session_cmd_param)
#define MTEE_CMD_CLOSE_SESSION  _IOWR(MTEE_IOC_MAGIC,  2, struct kree_session_cmd_param)
#define MTEE_CMD_TEE_SERVICE    _IOWR(MTEE_IOC_MAGIC,  3, struct kree_tee_service_cmd_param)
#define MTEE_CMD_SHM_REG        _IOWR(MTEE_IOC_MAGIC,  4, struct kree_tee_service_cmd_param)
#define MTEE_CMD_SHM_UNREG      _IOWR(MTEE_IOC_MAGIC,  5, struct kree_tee_service_cmd_param)


#define DEV_IOC_MAXNR       (10)

// param for open/close session
struct kree_session_cmd_param
{
    int ret;
    int handle;
    void *data;
};

// param for tee service call
struct kree_tee_service_cmd_param
{
    int ret;
    int handle;
    unsigned int command;
    unsigned int paramTypes;
    void *param;
};

// param for shared memory
struct kree_sharedmemory_cmd_param
{
    int ret;
    uint32_t session;
    uint32_t mem_handle;
    uint32_t command;
    void *buffer;
    uint32_t size;
    uint32_t control; // 0 = write, 1 = read only
};



#endif /* end of DEVFINO_H */


